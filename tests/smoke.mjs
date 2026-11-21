// Slufter3D - smoke test of the WebAssembly build: a short visit in Node
// Usage: node tests/smoke.mjs [path to slufter.wasm] [path to world.bin]
import { readFileSync } from "node:fs";

const path = process.argv[2] || "docs/slufter.wasm";
const file = readFileSync(process.argv[3] || "docs/data/world.bin");
const module = await WebAssembly.compile(readFileSync(path));
const imports = {};
for (const entry of WebAssembly.Module.imports(module)) {
    if (entry.kind === "function") {
        imports[entry.module] = imports[entry.module] || {};
        imports[entry.module][entry.name] = () => 0;
    }
}
const world = (await WebAssembly.instantiate(module, imports)).exports;

function hud() {
    const bytes = new Uint8Array(world.memory.buffer);
    let end = world.slufter_hud();
    const begin = end;
    while (bytes[end] !== 0) {
        ++end;
    }
    return new TextDecoder().decode(bytes.subarray(begin, end)).split("|");
}

function run(seconds) {
    for (let frame = 0; frame < Math.round(seconds * 60); ++frame) {
        world.slufter_step(1.0 / 60.0);
    }
}

let failures = 0;
function check(condition, what) {
    console.log((condition ? "  ok    " : "  FAIL  ") + what);
    failures += condition ? 0 : 1;
}

if (world._initialize) {
    world._initialize();
}
// As the page does: the world file goes into the program's memory
const address = world.slufter_alloc(file.byteLength);
new Uint8Array(world.memory.buffer, address, file.byteLength).set(file);
world.slufter_init();

const count = world.slufter_vertex_count();
const sky = world.slufter_sky_vertex_count();
const flat = world.slufter_flat_vertex_count();
const trees = world.slufter_tree_vertex_count();
check(trees > 0 && sky + trees + flat < count, "trees just before the roads and the water (" + trees + ")");
check(count > 500000, "mesh holds the world: " + count + " vertices");
check(sky > 0 && flat > 0 && sky + flat < count, "sky first (" + sky + "), roads and water last (" + flat + ")");
const vertices = new Float32Array(world.memory.buffer, world.slufter_vertices(), count * 6);
check(vertices.every(Number.isFinite), "all vertices are finite");
check(world.slufter_fog_range() > 1000.0, "the land fades over kilometres");
const ground = world.slufter_ground_vertex_count();
check(ground > 100000 && ground % 3 === 0 && sky + ground + trees + flat < count, "the land right after the sky (" + ground + ")");
const tiles = new Float32Array(world.memory.buffer, world.slufter_tiles(), 6);
check(tiles[0] < 0.0 && tiles[2] > 0.0 && tiles[1] < tiles[3] && tiles[4] === 3.0 && tiles[5] > 0.0 && tiles[5] < 0.1,
    "tiles of 3 m on the park grid, " + tiles[5] + " lighter and darker");

check(hud()[1] === "20", "sets out at 20 km/h");
world.slufter_set_pace(0);
world.slufter_step(1.0 / 60.0);
const start = hud();
check(start[5] === "118691" && start[6] === "574881" && start[8] === "90" && start[10] === "8",
    "starts 8 m up, over the lookout tower at RD 118691, 574881, heading east");
run(3.0);
check(hud()[10] === "8", "stands on the tower");
world.slufter_set_controls(1.0, 0.0, 0.0, 0.0);
run(5.0);
check(Number(hud()[5]) - Number(start[5]) > 5, "walks off it to the east: " + hud()[5]);
world.slufter_set_controls(0.0, 0.0, 0.0, 0.0);
run(10.0);
const before = hud();
check(before[10] === "0", "and comes down beside it by itself");
world.slufter_set_controls(1.0, 0.0, 0.0, 0.0);
run(5.0);
const walked = hud();
check(walked[1] === "5", "pace is 5 km/h");
const distance = Math.hypot(Number(walked[5]) - Number(before[5]), Number(walked[6]) - Number(before[6]));
check(distance > 1.0 && distance < 9.0, "walks: " + distance.toFixed(1) + " m in 5 s");
world.slufter_set_controls(0.0, 0.0, 0.0, 0.0);

check(world.slufter_goto(449) === 1, "goes to house 449");
const there = hud();
check(there[2] === "449" && there[3] === "8L" && there[4] === "8L", "449 is an 8L, drawn as 8L");
check(world.slufter_goto(3) === 1 && hud()[4] === "8C", "3 is drawn as 8C");
check(world.slufter_goto(26) === 1 && hud()[3] === "6CK" && hud()[4] === "8C", "26 is a 6CK, drawn as 8C");
check(world.slufter_goto(9999) === 0, "refuses a house that does not exist");

world.slufter_rise(10.0);
run(6.0);
const up = hud();
check(Number(up[10]) > 25 && Number(up[1]) >= 5, "flies: " + up[10] + " m up, at " + up[1] + " km/h");
world.slufter_set_controls(0.0, 0.0, 0.0, 1.0);
run(2.0);
check(Number(hud()[10]) > Number(up[10]), "the climb control takes it higher: " + hud()[10] + " m");
world.slufter_set_controls(0.0, 0.0, 0.0, 0.0);
world.slufter_land();
run(10.0);
check(hud()[10] === "0", "and lands again");

const matrix = new Float32Array(world.memory.buffer, world.slufter_view_projection(1.6), 16);
check(matrix.every(Number.isFinite), "view-projection matrix is finite");
world.slufter_reset();
world.slufter_set_controls(0.0, 0.0, 0.0, 0.0);
run(3.0);
check(hud()[5] === "118691" && hud()[6] === "574881" && hud()[10] === "8", "reset puts the visitor back on the tower, and he stays");
// From the tower the climb shows at once, not after the first metres under the tower top are spent
world.slufter_set_controls(0.0, 0.0, 0.0, 1.0);
run(1.0);
world.slufter_set_controls(0.0, 0.0, 0.0, 0.0);
check(Number(hud()[10]) > 9.0, "the climb from the tower shows within a second: " + hud()[10] + " m");
world.slufter_reset();
world.slufter_rise(1.0);
run(1.0);
check(Number(hud()[10]) > 9.0, "the first notch from the tower goes above its top: " + hud()[10] + " m");

// The eras: another era is another mesh, and today's comes back as it was
const verticesToday = world.slufter_vertex_count();
check(world.slufter_era() === 3, "the visit starts today");
world.slufter_set_era(0);
const verticesOld = world.slufter_vertex_count();
check(world.slufter_era() === 0 && verticesOld !== verticesToday && verticesOld > 0, "1957 to 1971 is another mesh: " + verticesOld + " vertices");
check(world.slufter_goto(449) === 0, "house 449 does not stand in 1957");
check(world.slufter_goto(12) === 1, "goes to number 12 of the old park");
run(0.2);
check(hud()[2] === "12 Grutto" && hud()[3] === "B4", "which shows as 12 Grutto, type B4: " + hud()[2] + ", " + hud()[3]);
check(world.slufter_goto(30) === 1, "and to number 30");
run(0.2);
check(hud()[2] === "30" && hud()[3] === "A6", "a bungalow without a name shows its number and type: " + hud()[2] + ", " + hud()[3]);
world.slufter_set_era(1);
check(world.slufter_vertex_count() > verticesOld, "1971 to 1993 adds the chalets");
run(0.5);
world.slufter_set_era(3);
check(world.slufter_vertex_count() === verticesToday && world.slufter_goto(17) === 1, "today is back as it was");

// The lens: a longer one is another projection, and the wide one comes back
const matrixOf = () => Array.from(new Float32Array(world.memory.buffer, world.slufter_view_projection(1.5), 16));
const wide = matrixOf();
world.slufter_set_field_of_view(27.0);
const long = matrixOf();
check(Math.abs(world.slufter_field_of_view() - 27.0) < 1e-4 && long[5] > 2.5 * wide[5], "a 50 mm lens enlarges the picture: "
    + (long[5] / wide[5]).toFixed(2) + " times");
world.slufter_set_field_of_view(68.0);
check(matrixOf().every((v, k) => Math.abs(v - wide[k]) < 1e-5), "the wide lens is back as it was");

console.log(failures === 0 ? "Smoke test passed" : "Smoke test failed: " + failures);
process.exit(failures === 0 ? 0 : 1);
