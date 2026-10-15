import * as THREE from "three";
import { RoundedBoxGeometry } from "three/addons/geometries/RoundedBoxGeometry.js";
import { mergeGeometries } from "three/addons/utils/BufferGeometryUtils.js";
import { roomSurfaces, roomObstacles } from "../room/placement";
import type { PersonalRoom } from "../../../../../packages/utils/src/companion";
import { registerRoomPowerBindings } from "./roomPowerBindings";

type Point = readonly [number, number, number];
type PaletteSlot = "plaster" | "upholstery" | "accent" | "stone";
export const terracePalettes = {
  rose: { plaster: "#e1d6c4", upholstery: "#c6a196", accent: "#b78379", stone: "#d7c5ac" },
  sage: { plaster: "#d6d9c8", upholstery: "#a9b59e", accent: "#7f9179", stone: "#d5ccba" },
  midnight: { plaster: "#77798c", upholstery: "#797b95", accent: "#b5969d", stone: "#b1a9a8" },
} satisfies Record<PersonalRoom["palette"], Record<PaletteSlot, string>>;

/** Lightweight geometry for the current renderer. The editable Blender source stays outside the app. */
export function makeTerrace(palette: PersonalRoom["palette"] = "rose", light: PersonalRoom["light"] = "daylight") {
  const group = new THREE.Group();
  group.name = "Terrace v03";
  const chunks = new Map<THREE.Material, THREE.BufferGeometry[]>();
  const material = (name: string, color: string, roughness: number, slot?: PaletteSlot, metalness = 0) => {
    const result = new THREE.MeshStandardMaterial({ color: slot ? terracePalettes[palette][slot] : color, roughness, metalness });
    result.name = name;
    if (slot) result.userData.paletteSlot = slot;
    return result;
  };
  const wood = material("Room oak", "#aa825b", .56);
  const woodEnd = material("Room oak end grain", "#94714f", .66);
  const cloth = material("Room upholstery", "#c6a196", .93, "upholstery");
  const accent = material("Room accent", "#b78379", .91, "accent");
  const linen = material("Room linen", "#e8dcc5", .96); linen.side = THREE.DoubleSide;
  const rail = material("Room bronze railing", "#70665a", .42, undefined, .65);
  const brass = material("Room warm brass", "#a1855f", .38, undefined, 1);
  const stone = material("Room stone", "#d7c5ac", .89, "stone");
  const grout = material("Room paving joints", "#a39b8b", 1);
  const pot = material("Room terracotta planter", "#b18769", .94);
  const soil = material("Room soil", "#43362b", 1);
  const stems = material("Room plant stems", "#686348", .9);
  const leaves = [material("Room foliage", "#556c47", .72), material("Room young foliage", "#859a6a", .77)];
  leaves.forEach(value => { value.side = THREE.DoubleSide; });
  const glow = material("Room practical glow", "#ffe2b9", .55);
  glow.emissive.set("#ffe2b9"); glow.emissiveIntensity = light === "evening" ? 2.5 : light === "golden" ? .65 : .08;

  // Bake part transforms into material batches. Repeated slats/leaves do not each make a draw call.
  const add = (geometry: THREE.BufferGeometry, mat: THREE.Material, position: Point = [0, 0, 0], rotation: Point = [0, 0, 0], scale: Point = [1, 1, 1]) => {
    const data = geometry.index ? geometry.toNonIndexed() : geometry;
    if (data !== geometry) geometry.dispose();
    data.clearGroups();
    // All batches use the same attribute set, including procedural leaf/canvas meshes.
    for (const name of Object.keys(data.attributes)) if (name !== "position" && name !== "normal") data.deleteAttribute(name);
    if (!data.getAttribute("normal")) data.computeVertexNormals();
    data.applyMatrix4(new THREE.Matrix4().compose(new THREE.Vector3(...position), new THREE.Quaternion().setFromEuler(new THREE.Euler(...rotation)), new THREE.Vector3(...scale)));
    const parts = chunks.get(mat) ?? []; parts.push(data); chunks.set(mat, parts);
  };
  const box = (size: Point, at: Point, mat = wood, radius = .015, rotation: Point = [0, 0, 0]) =>
    add(new RoundedBoxGeometry(...size, 2, Math.min(radius, Math.min(...size) * .3)), mat, at, rotation);
  const cylinder = (radius: number, height: number, at: Point, mat: THREE.Material, top = radius, segments = 20) =>
    add(new THREE.CylinderGeometry(top, radius, height, segments), mat, at);
  const oval = (scale: Point, at: Point, mat: THREE.Material, rotation: Point = [0, 0, 0]) =>
    add(new THREE.SphereGeometry(1, 20, 12), mat, at, rotation, scale);
  const tube = (points: Point[], radius: number, mat: THREE.Material, closed = false) => {
    const path = new THREE.CatmullRomCurve3(points.map(p => new THREE.Vector3(...p)), closed, "centripetal");
    add(new THREE.TubeGeometry(path, Math.max(4, points.length * 2), radius, 6, closed), mat);
  };
  const sheet = (rows: number, columns: number, sample: (row: number, column: number) => Point, mat: THREE.Material) => {
    const positions: number[] = [], indices: number[] = [];
    for (let r = 0; r <= rows; r++) for (let c = 0; c <= columns; c++) positions.push(...sample(r / rows, c / columns));
    for (let r = 0; r < rows; r++) for (let c = 0; c < columns; c++) {
      const a = r * (columns + 1) + c, b = a + columns + 1;
      indices.push(a, a + 1, b, a + 1, b + 1, b);
    }
    const data = new THREE.BufferGeometry();
    data.setAttribute("position", new THREE.Float32BufferAttribute(positions, 3)); data.setIndex(indices); data.computeVertexNormals(); add(data, mat);
  };

  cylinder(4.12, .2, [0, -.112, 0], stone, 4.12, 80);
  cylinder(4.04, .015, [0, -.0175, 0], grout, 4.04, 80);
  const pavers = [-.02, -.007, .007, .02].map((variation, i) => {
    const value = stone.clone(); value.name = `Room paving ${i + 1}`; value.color.offsetHSL(0, 0, variation); return value;
  });
  for (let ring = 0; ring < 6; ring++) for (let segment = 0; segment < 32; segment++) {
    const inner = ring * .67 + .007, outer = (ring + 1) * .67 - .007;
    const start = segment / 32 * Math.PI * 2 + .003, end = (segment + 1) / 32 * Math.PI * 2 - .003;
    sheet(1, 3, (r, c) => {
      const radius = inner + r * (outer - inner), angle = start + c * (end - start);
      return [Math.cos(angle) * radius, 0, Math.sin(angle) * radius];
    }, pavers[Math.floor((Math.sin((ring * 32 + segment + 1) * 127.1) * 43758.5453 % 1 + 1) % 1 * pavers.length)]);
  }
  // Raised perimeter coping is outside the editable floor and walking bounds.
  add(new THREE.TorusGeometry(4.06, .035, 8, 96), stone, [0, -.015, 0], [Math.PI / 2, 0, 0]);
  for (let i = 0; i < 25; i++) {
    const angle = Math.PI * (.06 + i / 24 * .88), x = Math.cos(angle) * 3.72, z = -Math.sin(angle) * 3.72;
    cylinder(.022, 1.1, [x, .55, z], rail, .022, 12);
    cylinder(.045, .055, [x, .0275, z], rail, .045, 12);
  }
  for (const [height, radius] of [[1.1, .038], [.14, .018]]) tube(Array.from({ length: 41 }, (_, i) => {
    const angle = Math.PI * (.06 + i / 40 * .88); return [Math.cos(angle) * 3.72, height, -Math.sin(angle) * 3.72];
  }), radius, rail);

  for (const x of [-2.9, 2.9]) for (const z of [-2.3, -.3]) {
    box([.13, 2.9, .13], [x, 1.45, z], wood, .025);
    box([.19, .08, .19], [x, .04, z], rail, .008);
  }
  for (const z of [-2.3, -.3]) box([5.96, .17, .16], [0, 2.88, z], wood, .025);
  for (let i = 0; i < 15; i++) box([.07, .12, 2.25], [-2.8 + i * .4, 3, -1.3], wood, .01);
  for (let panel = 0; panel < 4; panel++) sheet(16, 4, (t, u) =>
    [-2.62 + panel * 1.35 + u * 1.18, 2.93 - .095 * Math.sin(Math.PI * t), -2.27 + 1.92 * t], linen);
  tube(Array.from({ length: 25 }, (_, i) => [-2.75 + i / 24 * 5.5, 2.78 - .15 * Math.sin(i / 24 * Math.PI), -2.25]), .006, stems);
  for (let i = 0; i < 9; i++) {
    const x = -2.6 + i * .65, y = 2.78 - .15 * Math.sin((x + 2.75) / 5.5 * Math.PI);
    tube([[x, y, -2.25], [x, y - .075, -2.25]], .009, brass);
    oval([.035, .035, .035], [x, y - .11, -2.25], glow);
  }
  // One unshadowed fill for the whole string, not nine shadow-casting point lights.
  if (light !== "daylight") {
    const practical = new THREE.PointLight("#ffe2b9", light === "evening" ? 1.1 : .25, 5, 2);
    practical.position.set(0, 2.4, -2.25); group.add(practical);
    registerRoomPowerBindings(group, [{ kind: "light", target: practical }]);
  }

  for (const x of [-1.12, 1.12]) for (const z of [-3.2, -2.87]) cylinder(.043, .38, [x, .19, z], wood);
  for (const x of [-1.12, 1.12]) box([.09, .08, .56], [x, .38, -3.04], wood, .01);
  for (let i = 0; i < 5; i++) box([2.8, .1, .098], [0, .44, -3.28 + i * .12], wood, .012);
  for (const x of [-1.25, 0, 1.25]) box([.04, .57, .04], [x, .665, -3.26], wood, .01);
  for (const y of [.63, .77, .91]) box([2.77, .09, .038], [0, y, -3.26], wood, .01);
  for (const x of [-.72, .72]) {
    box([1.3, .13, .49], [x, .545, -3.04], cloth, .06);
    tube(Array.from({ length: 48 }, (_, i) => { const a = i / 48 * Math.PI * 2; return [x + .625 * Math.sign(Math.cos(a)) * Math.pow(Math.abs(Math.cos(a)), .25), .565, -3.04 + .231 * Math.sign(Math.sin(a)) * Math.pow(Math.abs(Math.sin(a)), .25)]; }), .0035, linen, true);
    box([.58, .35, .10], [x, .765, -3.17], x < 0 ? accent : cloth, .045, [-.12, 0, x < 0 ? -.08 : .07]);
  }

  // These top surfaces retain the saved placement contract, including their exact heights.
  const console = roomSurfaces.find(surface => surface.id === "console")!;
  const shelf = roomSurfaces.find(surface => surface.id === "shelf")!;
  box([console.width, .075, console.depth], [console.x, console.y - .0375, console.z], wood, .03);
  box([1.58, .15, .7], [console.x, .63, console.z], wood, .02);
  for (const x of [.36, 1.74]) for (const z of [-1.04, -.4]) cylinder(.045, .63, [x, .315, z], wood, .065);
  box([1.35, .12, .035], [console.x, .64, -.35], woodEnd, .012);
  for (const x of [.68, 1.42]) tube([[x - .055, .645, -.325], [x + .055, .645, -.325]], .008, brass);
  box([shelf.width, .065, shelf.depth], [shelf.x, shelf.y - .0325, shelf.z], wood, .025);
  for (const x of [-1.7, -.2]) {
    for (const z of [-1.48, -.92]) cylinder(.025, 1.415, [x, .7075, z], brass, .025, 12);
    tube([[x, .18, -1.48], [x, .18, -.92]], .021, brass);
  }
  tube([[-1.7, .22, -1.48], [-.2, .22, -1.48]], .021, brass);
  const chair = roomObstacles.find(obstacle => obstacle.id === "chair")!;
  for (const x of [-.31, .31]) for (const z of [-.32, .32]) cylinder(.032, .42, [chair.x + x, .21, chair.z + z], wood, .043, 12);
  box([.8, .075, .78], [chair.x, .415, chair.z], wood, .035);
  box([.81, .15, .82], [chair.x, .52, chair.z], cloth, .07);
  const rim: Point[] = [];
  for (let i = 0; i < 25; i++) {
    const a = -Math.PI / 2 + i / 24 * Math.PI, x = chair.x + .43 * Math.sin(a), z = chair.z - .46 * Math.cos(a), y = .76 + .29 * Math.cos(a);
    rim.push([x, y, z]); tube([[x, .42, z], [x, .67, z - .014], [x, y, z]], .01, wood);
  }
  tube(rim, .018, wood); tube(rim.map(([x, , z]) => [x, .46, z]), .014, wood);
  box([.53, .36, .11], [chair.x, .77, chair.z - .3], accent, .05, [-.14, 0, -.08]);

  for (const [x, z, size] of [[-3.05, .4, 1.45], [3, .1, 1.25], [2.2, -2.65, 1.15]]) {
    cylinder(.18 * size, .33 * size, [x, .165 * size, z], pot, .235 * size, 28);
    cylinder(.218 * size, .02 * size, [x, .326 * size, z], soil, .218 * size, 24);
    add(new THREE.TorusGeometry(.231 * size, .012 * size, 6, 32), pot, [x, .331 * size, z], [Math.PI / 2, 0, 0]);
    for (let branch = 0; branch < 3; branch++) {
      const a = branch * 2.4, dx = Math.cos(a) * .07, dz = Math.sin(a) * .07, top = 1 + branch * .07;
      tube([[x, .33 * size, z], [x + dx * size, top * size, z + dz * size]], .009 * size, stems);
      for (let j = 0; j < 7; j++) {
        const angle = a + j * 2.37, level = .46 + j * .082, tStem = (level - .33) / (top - .33);
        const bx = x + dx * tStem * size, bz = z + dz * tStem * size;
        const length = (.22 + .07 * ((j * 7 + branch * 3) % 11) / 11) * size;
        sheet(8, 4, (t, u) => {
          const q = u * 2 - 1, width = Math.pow(Math.sin(Math.PI * t), .8) * length * .32;
          return [bx + Math.cos(angle) * length * t - Math.sin(angle) * width * q,
            level * size + length * (.3 * t + .11 * (1 - q * q) * Math.sin(Math.PI * t) - .18 * t * t),
            bz + Math.sin(angle) * length * t + Math.cos(angle) * width * q];
        }, leaves[j % 3 === 0 ? 1 : 0]);
      }
    }
  }

  const facade = material("Room distant plaster", "#bcad9d", .95);
  const trim = material("Room limestone trim", "#eee4d2", .85);
  const glass = material("Room distant glazing", "#657477", .48);
  const roof = material("Room distant roof", "#8a7e72", .8);
  for (const [x, z, width, height] of [[-6, -9, 2.1, 3], [-3.6, -10, 2, 3.9], [-1, -9.3, 2.4, 3.2], [1.9, -11, 2.5, 4.1], [4.8, -10, 2, 3.2], [7.2, -12, 2.3, 4.3]]) {
    const bottom = -1.5;
    box([width, height, 1.6], [x, bottom + height / 2, z], facade, .025);
    box([width + .1, .1, 1.72], [x, bottom + height, z], trim, .015);
    box([width + .08, .16, 1.68], [x, bottom + height + .13, z], roof, .015);
    for (let row = 0; row < 2; row++) for (const offset of [-width * .24, width * .24]) {
      const y = bottom + .85 + row;
      box([.46, .66, .06], [x + offset, y, z + .825], trim, .008);
      box([.37, .56, .018], [x + offset, y, z + .863], glass, .006);
      box([.5, .055, .14], [x + offset, y - .35, z + .87], trim, .01);
    }
  }
  for (const [mat, pieces] of chunks) {
    const geometry = mergeGeometries(pieces, false);
    pieces.forEach(value => value.dispose());
    if (!geometry) { mat.dispose(); continue; }
    const mesh = new THREE.Mesh(geometry, mat); mesh.name = mat.name;
    mesh.castShadow = !mat.name.startsWith("Room distant"); mesh.receiveShadow = true; group.add(mesh);
    if (mat === glow) registerRoomPowerBindings(group, [{ kind: "emissive", target: glow }]);
  }
  return group;
}
