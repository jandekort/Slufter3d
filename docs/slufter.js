// Slufter3D - browser platform layer: loads the WebAssembly program and the world file, feeds it input,
// draws its vertex buffer with WebGL and shows where the visitor is.
// Author: Jan de Kort
// Created: 2026-11-16
"use strict";

(function () {
    const FLOATS_PER_VERTEX = 6;
    const LOOK_RATE = 0.0035;   // radians per pixel dragged
    const WHEEL_NOTCH = 100.0;  // wheel movement that counts as one step higher or lower
    const MAX_PIXEL_RATIO = 2.0;

    const canvas = document.getElementById("canvas");
    const hud = {
        where: document.getElementById("where"),
        house: document.getElementById("house"),
        error: document.getElementById("error")
    };
    const paceButtons = document.querySelectorAll("#panel button[data-pace]");
    const numberInput = document.getElementById("number");
    const keys = { ahead: false, back: false, left: false, right: false, stepLeft: false, stepRight: false,
        up: false, down: false };

    function fail(text) {
        hud.error.textContent = text;
    }

    // Input
    const keyMap = {
        ArrowUp: "ahead", KeyW: "ahead",
        ArrowDown: "back", KeyS: "back",
        ArrowLeft: "left", KeyA: "left",
        ArrowRight: "right", KeyD: "right",
        KeyQ: "stepLeft", KeyE: "stepRight",
        Space: "up", KeyC: "down"
    };
    let resetRequested = false;
    let gotoRequested = 0;
    let pace = 1;            // 1->slow (20 km/h), 2->fast (50 km/h)
    let paceChanged = true;
    let lookTurn = 0.0;
    let lookLift = 0.0;
    let riseNotches = 0.0;   // steps higher (+) or lower (-) asked for since the last frame
    let landRequested = false;

    function setPace(value) {
        pace = value;
        paceChanged = true;
        paceButtons.forEach(function (button) {
            button.classList.toggle("on", Number(button.dataset.pace) === pace);
        });
    }
    function requestGoto() {
        const number = parseInt(numberInput.value, 10);
        if (number > 0) {
            gotoRequested = number;
        }
        numberInput.blur();
    }
    paceButtons.forEach(function (button) {
        button.addEventListener("click", function () { setPace(Number(button.dataset.pace)); });
    });
    document.getElementById("go").addEventListener("click", requestGoto);
    numberInput.addEventListener("keydown", function (event) {
        if (event.code === "Enter" || event.code === "NumpadEnter") {
            requestGoto();
        }
        event.stopPropagation();
    });
    numberInput.addEventListener("keyup", function (event) { event.stopPropagation(); });

    window.addEventListener("keydown", function (event) {
        if (keyMap[event.code]) {
            keys[keyMap[event.code]] = true;
            event.preventDefault();
        } else if (event.code === "KeyR") {
            resetRequested = true;
        } else if (event.code === "KeyL") {
            landRequested = true;
        } else if (event.code === "KeyT") {
            toggleTrees();
        } else if (/^Digit[1-2]$/.test(event.code)) {
            setPace(Number(event.code.slice(5)));   // 1->slow, 2->fast
        }
    });
    window.addEventListener("keyup", function (event) {
        if (keyMap[event.code]) {
            keys[keyMap[event.code]] = false;
            event.preventDefault();
        }
    });
    window.addEventListener("blur", function () {
        Object.keys(keys).forEach(function (name) { keys[name] = false; });
    });
    // Flying: the wheel, or the buttons of the panel
    canvas.addEventListener("wheel", function (event) {
        event.preventDefault();
        riseNotches -= event.deltaY / WHEEL_NOTCH;
    }, { passive: false });
    document.getElementById("up").addEventListener("click", function () { riseNotches += 1.0; });
    document.getElementById("down").addEventListener("click", function () { riseNotches -= 1.0; });
    const treesButton = document.getElementById("trees");
    let showTrees = true;   // true->the trees are drawn
    function toggleTrees() {
        showTrees = !showTrees;
        treesButton.classList.toggle("on", showTrees);
    }
    treesButton.addEventListener("click", toggleTrees);
    document.getElementById("land").addEventListener("click", function () { landRequested = true; });

    document.querySelectorAll(".pad button").forEach(function (button) {
        const name = button.dataset.key;
        const set = function (down) {
            return function (event) {
                event.preventDefault();
                keys[name] = down;
            };
        };
        button.addEventListener("pointerdown", set(true));
        button.addEventListener("pointerup", set(false));
        button.addEventListener("pointercancel", set(false));
        button.addEventListener("pointerleave", set(false));
    });

    // Dragging over the picture turns the head
    let dragging = -1;
    let dragX = 0;
    let dragY = 0;
    canvas.addEventListener("pointerdown", function (event) {
        dragging = event.pointerId;
        dragX = event.clientX;
        dragY = event.clientY;
        canvas.setPointerCapture(event.pointerId);
        canvas.style.cursor = "grabbing";
    });
    canvas.addEventListener("pointermove", function (event) {
        if (event.pointerId === dragging) {
            lookTurn -= (event.clientX - dragX) * LOOK_RATE; // the picture follows the hand
            lookLift += (event.clientY - dragY) * LOOK_RATE;
            dragX = event.clientX;
            dragY = event.clientY;
        }
    });
    const endDrag = function (event) {
        if (event.pointerId === dragging) {
            dragging = -1;
            canvas.style.cursor = "grab";
        }
    };
    canvas.addEventListener("pointerup", endDrag);
    canvas.addEventListener("pointercancel", endDrag);

    // WebGL
    const gl = canvas.getContext("webgl", { antialias: true, alpha: false });
    if (!gl) {
        fail("WebGL is not available in this browser");
        return;
    }

    function compile(type, source) {
        const shader = gl.createShader(type);
        gl.shaderSource(shader, source);
        gl.compileShader(shader);
        if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
            throw new Error(gl.getShaderInfoLog(shader));
        }
        return shader;
    }

    const program = gl.createProgram();
    gl.attachShader(program, compile(gl.VERTEX_SHADER,
        "attribute vec3 aPosition;" +
        "attribute vec3 aColor;" +
        "uniform mat4 uViewProjection;" +
        "uniform vec3 uOffset;" +   // added to every vertex: the eye for the sky, nothing for the land
        "uniform vec3 uEye;" +
        "uniform float uFog;" +     // 1 / fog range, 0->no fog
        "varying vec3 vColor;" +
        "varying float vFade;" +
        "void main() {" +
        "    vec3 world = aPosition + uOffset;" +
        "    vColor = aColor;" +
        "    vFade = 1.0 - exp(-length(world - uEye) * uFog);" +
        "    gl_Position = uViewProjection * vec4(world, 1.0);" +
        "}"));
    gl.attachShader(program, compile(gl.FRAGMENT_SHADER,
        "precision mediump float;" +
        "uniform vec3 uHorizon;" +
        "varying vec3 vColor;" +
        "varying float vFade;" +
        "void main() { gl_FragColor = vec4(mix(vColor, uHorizon, vFade), 1.0); }"));
    gl.linkProgram(program);
    if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
        fail("Shader link failed");
        return;
    }
    gl.useProgram(program);

    const aPosition = gl.getAttribLocation(program, "aPosition");
    const aColor = gl.getAttribLocation(program, "aColor");
    const uViewProjection = gl.getUniformLocation(program, "uViewProjection");
    const uOffset = gl.getUniformLocation(program, "uOffset");
    const uEye = gl.getUniformLocation(program, "uEye");
    const uFog = gl.getUniformLocation(program, "uFog");
    const uHorizon = gl.getUniformLocation(program, "uHorizon");
    const buffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, buffer);
    gl.enableVertexAttribArray(aPosition);
    gl.enableVertexAttribArray(aColor);
    gl.vertexAttribPointer(aPosition, 3, gl.FLOAT, false, 4 * FLOATS_PER_VERTEX, 0);
    gl.vertexAttribPointer(aColor, 3, gl.FLOAT, false, 4 * FLOATS_PER_VERTEX, 12);
    gl.enable(gl.DEPTH_TEST);

    function resize() {
        const ratio = Math.min(window.devicePixelRatio || 1.0, MAX_PIXEL_RATIO);
        const width = Math.max(1, Math.floor(canvas.clientWidth * ratio));
        const height = Math.max(1, Math.floor(canvas.clientHeight * ratio));
        if (canvas.width !== width || canvas.height !== height) {
            canvas.width = width;
            canvas.height = height;
        }
    }

    function readString(world, pointer) {
        const bytes = new Uint8Array(world.memory.buffer);
        let end = pointer;
        while (bytes[end] !== 0) {
            ++end;
        }
        return new TextDecoder().decode(bytes.subarray(pointer, end));
    }

    const COMPASS = ["N", "NE", "E", "SE", "S", "SW", "W", "NW"];

    function start(world, file) {
        if (world._initialize) {
            world._initialize();
        }
        // The world file goes into the program's memory, which then builds the world from it
        const address = world.slufter_alloc(file.byteLength);
        new Uint8Array(world.memory.buffer, address, file.byteLength).set(new Uint8Array(file));
        world.slufter_init();
        window.slufter = world; // handy in the browser console

        // The park stands still, so its mesh is uploaded once
        const count = world.slufter_vertex_count();
        const skyCount = world.slufter_sky_vertex_count();
        const flatCount = world.slufter_flat_vertex_count();
        const treeCount = world.slufter_tree_vertex_count();   // just before the flat ones
        const fog = 1.0 / world.slufter_fog_range();
        gl.bufferData(gl.ARRAY_BUFFER,
            new Float32Array(world.memory.buffer, world.slufter_vertices(), count * FLOATS_PER_VERTEX), gl.STATIC_DRAW);
        const horizon = new Float32Array(world.memory.buffer, world.slufter_horizon(), 3);
        gl.uniform3f(uHorizon, horizon[0], horizon[1], horizon[2]);
        gl.clearColor(horizon[0], horizon[1], horizon[2], 1.0);

        let last = performance.now();
        function tick(now) {
            const elapsed = Math.min(0.1, (now - last) / 1000.0);
            last = now;

            if (resetRequested) {
                resetRequested = false;
                world.slufter_reset();
            }
            if (gotoRequested > 0) {
                if (!world.slufter_goto(gotoRequested)) {
                    numberInput.value = "";
                    numberInput.placeholder = "no " + gotoRequested;
                }
                gotoRequested = 0;
            }
            if (paceChanged) {
                paceChanged = false;
                world.slufter_set_pace(pace);
            }
            if (lookTurn !== 0.0 || lookLift !== 0.0) {
                world.slufter_look(lookTurn, lookLift);
                lookTurn = 0.0;
                lookLift = 0.0;
            }
            if (riseNotches !== 0.0) {
                world.slufter_rise(riseNotches);
                riseNotches = 0.0;
            }
            if (landRequested) {
                landRequested = false;
                world.slufter_land();
            }
            world.slufter_set_controls((keys.ahead ? 1.0 : 0.0) - (keys.back ? 1.0 : 0.0),
                (keys.stepRight ? 1.0 : 0.0) - (keys.stepLeft ? 1.0 : 0.0),
                (keys.right ? 1.0 : 0.0) - (keys.left ? 1.0 : 0.0),
                (keys.up ? 1.0 : 0.0) - (keys.down ? 1.0 : 0.0));
            world.slufter_step(elapsed);

            resize();
            // Views are recreated every frame because the memory may have grown
            const matrix = new Float32Array(world.memory.buffer,
                world.slufter_view_projection(canvas.width / canvas.height), 16);
            const eye = new Float32Array(world.memory.buffer, world.slufter_eye(), 3);

            gl.viewport(0, 0, canvas.width, canvas.height);
            gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
            gl.uniformMatrix4fv(uViewProjection, false, matrix);
            gl.uniform3f(uEye, eye[0], eye[1], eye[2]);

            // Sky: around the eye, behind everything
            gl.depthMask(false);
            gl.uniform3f(uOffset, eye[0], eye[1], eye[2]);
            gl.uniform1f(uFog, 0.0);
            gl.drawArrays(gl.TRIANGLES, 0, skyCount);
            // Land and everything on it
            gl.depthMask(true);
            gl.uniform3f(uOffset, 0.0, 0.0, 0.0);
            gl.uniform1f(uFog, fog);
            gl.drawArrays(gl.TRIANGLES, skyCount, count - skyCount - flatCount - (showTrees ? 0 : treeCount));
            // Roads and water lie flat on the land: nudged towards the eye, so that they never flicker against it
            gl.enable(gl.POLYGON_OFFSET_FILL);
            gl.polygonOffset(-2.0, -4.0);
            gl.drawArrays(gl.TRIANGLES, count - flatCount, flatCount);
            gl.disable(gl.POLYGON_OFFSET_FILL);

            const f = readString(world, world.slufter_hud()).split("|");
            const compass = COMPASS[Math.round(Number(f[8]) / 45.0) % 8];
            const flying = Number(f[10]) > 0;
            hud.where.innerHTML = "<b>" + f[0] + " km/h</b><br>" +
                (flying ? f[10] + " m above the " : "on the ") + f[9] + " &middot; ground " + f[7] + " m NAP<br>" +
                "heading " + compass + " &middot; RD " + f[5] + ", " + f[6];
            hud.house.innerHTML = f[2] !== "0" && f[2] !== ""
                ? "<b>" + f[2] + "</b><br>type " + f[3] + (f[3] !== f[4] ? "<br>drawn as " + f[4] : "")
                : "";

            window.slufterFrames = (window.slufterFrames || 0) + 1;
            requestAnimationFrame(tick);
        }
        requestAnimationFrame(tick);
    }

    // Emscripten's standalone build asks for a few WASI calls for stdio, which the
    // world never uses, and a notice when its memory grows; all are answered with 0.
    function stubs(module) {
        const imports = {};
        WebAssembly.Module.imports(module).forEach(function (entry) {
            if (entry.kind === "function") {
                imports[entry.module] = imports[entry.module] || {};
                imports[entry.module][entry.name] = function () { return 0; };
            }
        });
        return imports;
    }

    function load(path) {
        return fetch(path).then(function (response) {
            if (!response.ok) {
                throw new Error(path + ": HTTP " + response.status);
            }
            return response.arrayBuffer();
        });
    }

    Promise.all([load("slufter.wasm"), load("data/world.bin")])
        .then(function (loaded) {
            return WebAssembly.compile(loaded[0]).then(function (module) {
                return WebAssembly.instantiate(module, stubs(module));
            }).then(function (instance) {
                start(instance.exports, loaded[1]);
            });
        })
        .catch(function (error) {
            fail("Could not start Slufter3D: " + error.message);
        });
})();
