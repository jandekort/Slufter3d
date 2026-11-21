// Slufter3D - browser platform layer: loads the WebAssembly program and the world file, feeds it input,
// draws its vertex buffer with WebGL and shows where the visitor is.
// Author: Jan de Kort
// Created: 2026-11-16
"use strict";

(function () {
    const FLOATS_PER_VERTEX = 6;
    const LOOK_RATE = 0.0035;   // radians per pixel dragged
    const WIDE_LENS = 68.0;     // the view the visit starts with, from top to bottom [deg]
    const WHEEL_NOTCH = 100.0;  // wheel movement that counts as one step higher or lower
    const MAX_PIXEL_RATIO = 2.0;
    const TILES_FADE = [300.0, 600.0];        // the tiles on the ground fade out over these distances [m] ...
    const TILES_FADE_UNFILTERED = [15.0, 50.0]; // ... or these, where they cannot be filtered against shimmer
    // Weather. The light on the land stays as the world was built, with the sun in the south-west; the page fogs
    // the land over a shorter distance, takes some of its colour away, tints it and gives the sky other colours.
    // fog: distance over which the land fades into the horizon [m]; grey: part of the colour taken away
    const WEATHERS = {
        fog:      { fog: 90.0,   horizon: [0.80, 0.81, 0.82], zenith: [0.80, 0.81, 0.82], tint: [0.98, 0.99, 1.00], grey: 0.30 },
        overcast: { fog: 2500.0, horizon: [0.72, 0.74, 0.77], zenith: [0.80, 0.82, 0.85], tint: [0.85, 0.87, 0.90], grey: 0.35 },
        // The blue hour after sunset: a soft blue sky, the land dark, almost grey, with a blue cast, a third as
        // bright as the sky, fading into blue haze; matched to a photograph of the Wadden coast
        bluehour: { fog: 1800.0, horizon: [0.42, 0.50, 0.82], zenith: [0.27, 0.40, 0.82], tint: [0.44, 0.44, 0.62], grey: 0.70 }
    };

    const canvas = document.getElementById("canvas");
    const hud = {
        where: document.getElementById("where"),
        house: document.getElementById("house"),
        error: document.getElementById("error")
    };
    const paceButtons = document.querySelectorAll("#panel button[data-pace]");
    const weatherSelect = document.getElementById("weather");
    const eraSelect = document.getElementById("era");
    const lensSelect = document.getElementById("lens");
    const settings = document.getElementById("settings");
    const settingsButton = document.getElementById("settingsButton");
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
    let lens = WIDE_LENS;    // the angle the picture spans from top to bottom [deg]
    let lensChanged = false;

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
        } else if (event.code === "KeyG") {
            toggleTiles();
        } else if (event.code === "KeyF") {
            nextWeather();
        } else if (event.code === "KeyZ") {
            lensSelect.selectedIndex = (lensSelect.selectedIndex + 1) % lensSelect.options.length;
            setLens(Number(lensSelect.value));
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
    // A press on higher or lower is a step; holding it keeps climbing, as Space and C do. Taken at the press
    // itself, so that a phone cannot turn it into a selection; a click without a pointer is the keyboard.
    function climbButton(button, notch, key) {
        button.addEventListener("pointerdown", function (event) {
            event.preventDefault();
            riseNotches += notch;
            keys[key] = true;
            button.setPointerCapture(event.pointerId);
        });
        const release = function () { keys[key] = false; };
        button.addEventListener("pointerup", release);
        button.addEventListener("pointercancel", release);
        button.addEventListener("lostpointercapture", release);
        button.addEventListener("click", function (event) {
            if (event.detail === 0) {
                riseNotches += notch;
            }
        });
    }
    climbButton(document.getElementById("up"), 1.0, "up");
    climbButton(document.getElementById("down"), -1.0, "down");
    const treesSelect = document.getElementById("trees");
    let showTrees = true;    // true->the trees are drawn; on at the start, phones draw them smoothly
    function setTrees(on) {
        showTrees = on;
        treesSelect.value = on ? "on" : "off";
    }
    function toggleTrees() {
        setTrees(!showTrees);
    }
    setTrees(showTrees);
    treesSelect.addEventListener("change", function () {
        setTrees(treesSelect.value === "on");
        treesSelect.blur();     // so that the arrow keys walk again
    });
    const tilesSelect = document.getElementById("tiles");
    let showTiles = true;    // true->the tiles of the park's grid are drawn on the land
    function setTiles(on) {
        showTiles = on;
        tilesSelect.value = on ? "on" : "off";
    }
    function toggleTiles() {
        setTiles(!showTiles);
    }
    setTiles(showTiles);
    tilesSelect.addEventListener("change", function () {
        setTiles(tilesSelect.value === "on");
        tilesSelect.blur();
    });
    let weather = "day";     // a name of WEATHERS, or "day": as the world was built
    function setWeather(name) {
        weather = name;
        weatherSelect.value = name;
    }
    function nextWeather() {
        const names = Array.from(weatherSelect.options).map(function (option) { return option.value; });
        setWeather(names[(names.indexOf(weather) + 1) % names.length]);
    }
    setWeather(weather);
    // Once chosen, the drop-down lets go of the keys, which walk again
    weatherSelect.addEventListener("change", function () {
        setWeather(weatherSelect.value);
        weatherSelect.blur();
    });
    document.getElementById("land").addEventListener("click", function () { landRequested = true; });
    // The lens: a longer one sees less, larger, as the photographs of the old park do
    function setLens(degrees) {
        lens = degrees;
        lensChanged = true;
    }
    lensSelect.addEventListener("change", function () {
        setLens(Number(lensSelect.value));
        lensSelect.blur();
    });
    // Settings: what is not needed all the time opens and closes under its button
    settingsButton.addEventListener("click", function () {
        settings.hidden = !settings.hidden;
        settingsButton.classList.toggle("on", !settings.hidden);
        settingsButton.setAttribute("aria-expanded", String(!settings.hidden));
    });

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

    // No selection of text and no long-press menu, which on a phone would take the press away from a button;
    // only the house number field and links keep them
    function keepsSelection(target) {
        const element = target instanceof Element ? target : target.parentElement;
        return element !== null && element.closest("input, a") !== null;
    }
    ["selectstart", "contextmenu"].forEach(function (type) {
        document.addEventListener(type, function (event) {
            if (!keepsSelection(event.target)) {
                event.preventDefault();
            }
        });
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
            const rate = LOOK_RATE * lens / WIDE_LENS;       // through a long lens the picture turns as far as the hand
            lookTurn -= (event.clientX - dragX) * rate;      // the picture follows the hand
            lookLift += (event.clientY - dragY) * rate;
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

    // With derivatives the tiles on the ground are averaged over each pixel, so that far away they blend into
    // their mean colour instead of shimmering; without, they fade out close by
    const filtered = gl.getExtension("OES_standard_derivatives") !== null;

    const program = gl.createProgram();
    gl.attachShader(program, compile(gl.VERTEX_SHADER,
        "attribute vec3 aPosition;" +
        "attribute vec3 aColor;" +
        "uniform mat4 uViewProjection;" +
        "uniform vec3 uOffset;" +   // added to every vertex: the eye for the sky, nothing for the land
        "uniform vec3 uEye;" +
        "uniform float uFog;" +     // 1 / fog range, 0->no fog
        "uniform vec2 uTileOrigin;" +   // corner of a tile near the eye, in metres east and north
        "uniform vec4 uTileArea;" +     // west, south, east and north of the grid that has the tiles [m]
        "uniform float uTileSize;" +    // [m]
        "uniform float uTileContrast;" + // 0->no tiles
        "uniform vec2 uTileFade;" +     // the tiles fade out between these distances [m]
        "uniform vec3 uTint;" +         // the colours are multiplied by this ...
        "uniform float uGrey;" +        // ... after this part of them is taken away, 0->as built
        "uniform float uSkyMix;" +      // 1->the sky in the colours below, 0->as built
        "uniform vec3 uSkyLow;" +       // at the horizon
        "uniform vec3 uSkyHigh;" +      // overhead
        "varying vec3 vColor;" +
        "varying float vFade;" +
        "varying vec2 vTile;" +         // place in tiles from uTileOrigin
        "varying float vContrast;" +
        "void main() {" +
        "    vec3 world = aPosition + uOffset;" +
        "    float away = length(world - uEye);" +
        "    vec2 place = vec2(world.x, -world.z);" +
        "    float inside = step(uTileArea.x, place.x) * step(place.x, uTileArea.z)" +
        "        * step(uTileArea.y, place.y) * step(place.y, uTileArea.w);" +
        "    vec3 color = mix(aColor, vec3(dot(aColor, vec3(0.299, 0.587, 0.114))), uGrey) * uTint;" +
        // The sky as Scene::addSky colours it: rings from 0.12 rad below the horizon, crowding near it
        "    if (uSkyMix > 0.0) {" +
        "        float t = sqrt(clamp((asin(clamp(normalize(aPosition).y, -1.0, 1.0)) + 0.12) / 1.6907963, 0.0, 1.0));" +
        "        color = mix(color, mix(uSkyLow, uSkyHigh, sqrt(clamp(1.6 * t, 0.0, 1.0))), uSkyMix);" +
        "    }" +
        "    vColor = color;" +
        "    vFade = 1.0 - exp(-away * uFog);" +
        "    vTile = (place - uTileOrigin) / uTileSize;" +
        "    vContrast = uTileContrast * inside * (1.0 - smoothstep(uTileFade.x, uTileFade.y, away));" +
        "    gl_Position = uViewProjection * vec4(world, 1.0);" +
        "}"));
    gl.attachShader(program, compile(gl.FRAGMENT_SHADER,
        (filtered ? "#extension GL_OES_standard_derivatives : enable\n" : "") +
        "#ifdef GL_FRAGMENT_PRECISION_HIGH\nprecision highp float;\n#else\nprecision mediump float;\n#endif\n" +
        "uniform vec3 uHorizon;" +
        "varying vec3 vColor;" +
        "varying float vFade;" +
        "varying vec2 vTile;" +
        "varying float vContrast;" +
        // 1 on the odd tiles, 0 on the even ones; filtered, the share of odd tile within the pixel
        "float odd(vec2 p) {" +
        (filtered ?
        "    vec2 w = fwidth(p) + 0.001;" +
        "    vec2 i = 2.0 * (abs(fract((p - 0.5 * w) * 0.5) - 0.5) - abs(fract((p + 0.5 * w) * 0.5) - 0.5)) / w;" +
        "    return 0.5 - 0.5 * i.x * i.y;" :
        "    vec2 c = floor(p);" +
        "    return mod(c.x + c.y, 2.0);") +
        "}" +
        "void main() {" +
        "    vec3 color = vColor;" +
        "    if (vContrast > 0.0) { color *= 1.0 - vContrast * (2.0 * odd(vTile) - 1.0); }" +
        "    gl_FragColor = vec4(mix(color, uHorizon, vFade), 1.0);" +
        "}"));
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
    const uTileOrigin = gl.getUniformLocation(program, "uTileOrigin");
    const uTileArea = gl.getUniformLocation(program, "uTileArea");
    const uTileSize = gl.getUniformLocation(program, "uTileSize");
    const uTileContrast = gl.getUniformLocation(program, "uTileContrast");
    const uTileFade = gl.getUniformLocation(program, "uTileFade");
    const uTint = gl.getUniformLocation(program, "uTint");
    const uGrey = gl.getUniformLocation(program, "uGrey");
    const uSkyMix = gl.getUniformLocation(program, "uSkyMix");
    const uSkyLow = gl.getUniformLocation(program, "uSkyLow");
    const uSkyHigh = gl.getUniformLocation(program, "uSkyHigh");
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

        // The park stands still within an era, so its mesh is uploaded once for each era that is chosen
        let count = 0;
        let skyCount = 0;
        let flatCount = 0;
        let treeCount = 0;      // just before the flat ones
        let groundCount = 0;    // just after the sky
        function upload() {
            count = world.slufter_vertex_count();
            skyCount = world.slufter_sky_vertex_count();
            flatCount = world.slufter_flat_vertex_count();
            treeCount = world.slufter_tree_vertex_count();
            groundCount = world.slufter_ground_vertex_count();
            gl.bufferData(gl.ARRAY_BUFFER,
                new Float32Array(world.memory.buffer, world.slufter_vertices(), count * FLOATS_PER_VERTEX), gl.STATIC_DRAW);
        }
        upload();
        // The era: 0->1957 to 1971, 1->1971 to 1993, 2->1993 to 2013, 3->2013 to today
        eraSelect.value = String(world.slufter_era());
        eraSelect.addEventListener("change", function () {
            world.slufter_set_era(Number(eraSelect.value));
            upload();
            eraSelect.blur();   // so that the arrow keys walk again
        });
        // The day as the world was built: its haze, its horizon, its sky
        const day = { fog: world.slufter_fog_range(),
            horizon: Array.from(new Float32Array(world.memory.buffer, world.slufter_horizon(), 3)),
            zenith: null, tint: [1.0, 1.0, 1.0], grey: 0.0 };
        // The tiles of the finest grid; a centimetre more all round, so that its edge counts as inside
        const tiles = Array.from(new Float32Array(world.memory.buffer, world.slufter_tiles(), 6));
        const tilePair = 2.0 * tiles[4];   // origins a pair of tiles apart keep odd tiles odd
        gl.uniform4f(uTileArea, tiles[0] - 0.01, tiles[1] - 0.01, tiles[2] + 0.01, tiles[3] + 0.01);
        gl.uniform1f(uTileSize, tiles[4]);
        gl.uniform2fv(uTileFade, filtered ? TILES_FADE : TILES_FADE_UNFILTERED);
        gl.uniform1f(uTileContrast, 0.0);

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
            if (lensChanged) {
                lensChanged = false;
                world.slufter_set_field_of_view(lens);
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

            const w = weather === "day" ? day : WEATHERS[weather];
            gl.viewport(0, 0, canvas.width, canvas.height);
            gl.clearColor(w.horizon[0], w.horizon[1], w.horizon[2], 1.0);
            gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
            gl.uniform3fv(uHorizon, w.horizon);
            gl.uniformMatrix4fv(uViewProjection, false, matrix);
            gl.uniform3f(uEye, eye[0], eye[1], eye[2]);

            // Sky: around the eye, behind everything
            gl.depthMask(false);
            gl.uniform3f(uOffset, eye[0], eye[1], eye[2]);
            gl.uniform1f(uFog, 0.0);
            gl.uniform3f(uTint, 1.0, 1.0, 1.0);
            gl.uniform1f(uGrey, 0.0);
            gl.uniform1f(uSkyMix, w.zenith !== null ? 1.0 : 0.0);
            if (w.zenith !== null) {
                gl.uniform3fv(uSkyLow, w.horizon);
                gl.uniform3fv(uSkyHigh, w.zenith);
            }
            gl.drawArrays(gl.TRIANGLES, 0, skyCount);
            // The land, with its tiles counted from a corner near the eye, so that the numbers stay small
            gl.depthMask(true);
            gl.uniform3f(uOffset, 0.0, 0.0, 0.0);
            gl.uniform1f(uFog, 1.0 / w.fog);
            gl.uniform3fv(uTint, w.tint);
            gl.uniform1f(uGrey, w.grey);
            gl.uniform1f(uSkyMix, 0.0);
            gl.uniform2f(uTileOrigin, tiles[0] + tilePair * Math.floor((eye[0] - tiles[0]) / tilePair),
                tiles[1] + tilePair * Math.floor((-eye[2] - tiles[1]) / tilePair));
            gl.uniform1f(uTileContrast, showTiles ? tiles[5] : 0.0);
            gl.drawArrays(gl.TRIANGLES, skyCount, groundCount);
            gl.uniform1f(uTileContrast, 0.0);
            // Everything on it
            gl.drawArrays(gl.TRIANGLES, skyCount + groundCount,
                count - skyCount - groundCount - flatCount - (showTrees ? 0 : treeCount));
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
