import * as THREE from "three";
import { RoundedBoxGeometry } from "three/addons/geometries/RoundedBoxGeometry.js";
import { mergeGeometries } from "three/addons/utils/BufferGeometryUtils.js";
import type { CompanionPlaceFixture, CompanionPlaceLayout } from "../../../../../packages/utils/src/companionPlaces";
import { nightclubLimits } from "../../../../../packages/utils/src/companionNightclub";

type LightingChannel = { material: THREE.MeshStandardMaterial; base: number; amplitude: number; period: number; phase: number };
type FixturePart = { geometry: THREE.BufferGeometry; material: THREE.Material };
type PreparedFixture = { parts: readonly FixturePart[]; pieces: number; vertices: number };
type BeamState = { mesh: THREE.Mesh<THREE.ConeGeometry, THREE.ShaderMaterial>; source: THREE.Vector3; target: THREE.Vector3;
  baseX: number; baseZ: number; travel: number; period: number; phase: number; opacity: number };
type LightingState = {
  channels: LightingChannel[];
  lights: { light: THREE.PointLight | THREE.SpotLight; base: number; amplitude: number; period: number; phase: number }[];
  beams: BeamState[];
  screens: THREE.InstancedMesh[];
  equalizerTransform: THREE.Object3D;
  direction: THREE.Vector3;
  up: THREE.Vector3;
  sample: number | null;
  reducedMotion: boolean;
  powerFactor: number;
};
const lightingStates = new WeakMap<THREE.Group, LightingState>();
const limits = { fixtures: 64, pieces: 1600, vertices: 180000, meshes: 192, lights: 9, beams: 4 } as const;

/** Adds local meshes to the generated parent's traversal and material registry.
 * No cache-owned assets, textures, timers, shadows from lights or separate disposer. */
export function createNightclubGeometry(layout: CompanionPlaceLayout, parent: THREE.Group, ownedMaterials: THREE.Material[]): THREE.Group {
  if (layout.kind !== "nightclub" || layout.fixtures.length > limits.fixtures) throw new Error("Use a bounded nightclub layout.");
  const group = new THREE.Group(); group.name = `${layout.id}:nightclub`; parent.add(group);
  const material = (parameters: THREE.MeshStandardMaterialParameters) => {
    const value = new THREE.MeshStandardMaterial(parameters); ownedMaterials.push(value); return value;
  };
  const m = {
    wall: material({ color: layout.theme.wall, roughness: .89 }),
    walnut: material({ color: "#46322e", roughness: .64 }),
    upholstery: material({ color: "#6d4657", roughness: .98 }),
    cushion: material({ color: "#a98686", roughness: 1 }),
    seam: material({ color: "#c4a5a0", roughness: 1 }),
    brass: material({ color: "#ab8655", metalness: .76, roughness: .35 }),
    black: material({ color: "#15171d", roughness: .58 }),
    grille: material({ color: "#33343b", metalness: .32, roughness: .88 }),
    stone: material({ color: "#514c54", metalness: .12, roughness: .26 }),
    floor: material({ color: "#242731", metalness: .18, roughness: .34 }),
    glass: material({ color: "#5e7f77", metalness: .25, roughness: .24 }),
    champagne: material({ color: "#284b3c", metalness: .36, roughness: .16 }),
    label: material({ color: "#eee4ce", roughness: .6 }),
    ice: material({ color: "#c1e1e5", metalness: .15, roughness: .15 }),
    amber: material({ color: "#c6a478", emissive: "#edbe86", emissiveIntensity: .7, roughness: .5 }),
    lilac: material({ color: "#b279dd", emissive: "#c25aff", emissiveIntensity: .85, roughness: .5 }),
    jade: material({ color: "#59b6bd", emissive: "#39e2d2", emissiveIntensity: .8, roughness: .5 }),
  };
  let pieces = 0, vertices = 0, meshes = 0;
  const preparedFixtures = new Map<string, PreparedFixture>();
  // Merge within each fixture/material so details do not each add a draw call.
  // Keep fixture roots intact for picking and use resolved positions and yaw.
  function build(value: CompanionPlaceFixture) {
    const root = new THREE.Group(); root.name = value.id; root.userData.lagosFixtureId = value.id;
    root.position.fromArray(value.point); root.rotation.y = value.yaw; group.add(root);
    const serviceId = value.id === "dance-floor" || value.id === "dj-booth" || value.id === "dj-stage" ? "dance-host"
      : value.id.startsWith("club-lounge") || value.id.startsWith("club-table") ? "club-host"
        : value.id === "club-bar" || value.id === "club-backbar" || value.id.startsWith("club-bar-stool") ? "club-water" : undefined;
    if (serviceId && layout.services.some(service => service.id === serviceId)) root.userData.lagosServiceId = serviceId;
    const [w, h, d] = value.size;
    // These recipes depend only on size and this venue's material palette.
    // Each placement keeps its own root, picking identity and lighting children.
    const screenHousing = value.id.startsWith("club-side-panel") && value.id.endsWith("-0");
    const recipe = value.id.startsWith("club-lounge") ? "lounge" : value.id.startsWith("club-table") ? "table"
      : value.id.startsWith("club-speaker") ? "speaker" : value.id.startsWith("club-bar-stool") ? "bar-stool"
        : value.id.startsWith("club-side-panel") || value.id.startsWith("club-back-panel") ? screenHousing ? "panel-screen" : "panel" : undefined;
    const key = recipe && value.size.every(size => Number.isFinite(size) && size > 0) ? `${recipe}:${w}:${h}:${d}` : undefined;
    const prepared = key ? preparedFixtures.get(key) : undefined;
    function attach(geometry: THREE.BufferGeometry, mat: THREE.Material) {
      const mesh = new THREE.Mesh(geometry, mat); mesh.name = `${value.id}:detail`;
      mesh.castShadow = mat !== m.amber && mat !== m.lilac && mat !== m.jade;
      mesh.receiveShadow = mesh.castShadow; root.add(mesh);
    }
    if (prepared) {
      // Admission still counts every placed part and vertex, including copies.
      pieces += prepared.pieces; vertices += prepared.vertices; meshes += prepared.parts.length;
      if (pieces > limits.pieces || vertices > limits.vertices) throw new Error("Nightclub geometry exceeds its detail budget.");
      if (meshes > limits.meshes) throw new Error("Nightclub geometry exceeds its mesh budget.");
      for (const part of prepared.parts) attach(part.geometry, part.material);
      return;
    }
    const batches = new Map<THREE.Material, THREE.BufferGeometry[]>(), parts: FixturePart[] = [];
    const firstPiece = pieces, firstVertex = vertices;
    function part(source: THREE.BufferGeometry, mat: THREE.Material, x: number, y: number, z: number, rotation?: readonly [number, number, number]) {
      let geometry = source;
      if (source.index) {
        try { geometry = source.toNonIndexed(); } finally { source.dispose(); }
      }
      pieces++; vertices += geometry.getAttribute("position").count;
      if (pieces > limits.pieces || vertices > limits.vertices) { geometry.dispose(); throw new Error("Nightclub geometry exceeds its detail budget."); }
      const batch = batches.get(mat) ?? []; batches.set(mat, batch); batch.push(geometry);
      if (rotation) { geometry.rotateX(rotation[0]); geometry.rotateY(rotation[1]); geometry.rotateZ(rotation[2]); }
      geometry.translate(x, y, z);
    }
    const box = (mat: THREE.Material, width: number, height: number, depth: number, x: number, y: number, z: number) =>
      part(new THREE.BoxGeometry(width, height, depth), mat, x, y, z);
    const soft = (mat: THREE.Material, width: number, height: number, depth: number, radius: number, x: number, y: number, z: number) =>
      part(new RoundedBoxGeometry(width, height, depth, 2, radius), mat, x, y, z);
    const cylinder = (mat: THREE.Material, top: number, bottom: number, height: number, x: number, y: number, z: number, rotation?: readonly [number, number, number]) =>
      part(new THREE.CylinderGeometry(top, bottom, height, 24), mat, x, y, z, rotation);
    const ring = (mat: THREE.Material, radius: number, tube: number, x: number, y: number, z: number, rotation?: readonly [number, number, number]) =>
      part(new THREE.TorusGeometry(radius, tube, 6, 24), mat, x, y, z, rotation);
    function sofa() {
      const seatY = .48, inner = w - .36, count = Math.min(6, Math.max(2, Math.round(inner / .9)));
      for (const x of [-w / 2 + .23, w / 2 - .23]) for (const z of [-d / 2 + .2, d / 2 - .2])
        cylinder(m.brass, .035, .027, .13, x, .065, z);
      soft(m.walnut, w - .1, .18, d - .12, .035, 0, .22, 0);
      soft(m.upholstery, w, .2, d, .075, 0, .32, 0);
      soft(m.upholstery, w, h - .4, .22, .07, 0, (h + .4) / 2, -d / 2 + .11);
      for (const x of [-w / 2 + .09, w / 2 - .09])
        soft(m.upholstery, .18, .52, d - .04, .065, x, .61, .01);
      for (let i = 0; i < count; i++) {
        const width = inner / count - .018, x = -inner / 2 + (i + .5) * inner / count;
        soft(m.upholstery, width, .18, d - .32, .065, x, seatY - .03, .085);
        soft(m.cushion, width, .38, .17, .055, x, .75, -d / 2 + .29);
        // Welt seams and narrow channel divisions remain separate geometry.
        box(m.seam, width - .075, .006, .009, x, seatY + .063, d / 2 - .10);
        box(m.seam, .006, .27, .008, x, .75, -d / 2 + .381);
      }
      box(m.amber, w - .45, .016, .012, 0, .18, d / 2 - .028);
    }
    function table() {
      const radius = Math.min(w, d) * .47;
      cylinder(m.black, radius * .64, radius * .76, .055, 0, .0275, 0);
      cylinder(m.brass, .065, .11, h - .085, 0, (h - .085) / 2, 0);
      cylinder(m.stone, radius, radius, .07, 0, h - .035, 0);
      ring(m.brass, radius - .009, .009, 0, h - .011, 0, [Math.PI / 2, 0, 0]);
      cylinder(m.brass, .065, .072, .085, 0, h + .0425, 0);
      cylinder(m.amber, .052, .052, .035, 0, h + .0975, 0);
      // Premium bottle service stays inside the existing table footprint.
      const bx = -radius * .36, bz = -radius * .22;
      cylinder(m.brass, .19, .135, .2, bx, h + .1, bz);
      cylinder(m.black, .17, .17, .008, bx, h + .203, bz);
      ring(m.brass, .187, .011, bx, h + .20, bz, [Math.PI / 2, 0, 0]);
      for (let i = 0; i < 5; i++) box(m.ice, .065, .045, .055, bx + Math.cos(i * 1.9) * .10, h + .20, bz + Math.sin(i * 1.9) * .10);
      function bottle(x: number, z: number, y: number, cognac = false) {
        const body = cognac ? m.glass : m.champagne;
        cylinder(body, .052, .067, .27, x, y + .135, z);
        cylinder(body, .024, .052, .07, x, y + .305, z);
        cylinder(cognac ? m.brass : body, .023, .024, .12, x, y + .40, z);
        cylinder(m.brass, .025, .025, .034, x, y + .463, z);
        cylinder(m.label, .068, .068, .082, x, y + .16, z);
        box(m.brass, .028, .044, .005, x, y + .16, z + .069);
      }
      bottle(bx - .045, bz, h + .16);
      bottle(radius * .34, -radius * .28, h, true);
      for (const sign of [-1, 1]) {
        const x = sign * radius * .47, z = radius * .4;
        cylinder(m.glass, .052, .032, .085, x, h + .045, z);
        cylinder(m.brass, .009, .009, .11, x, h + .143, z);
        cylinder(m.glass, .042, .054, .12, x, h + .255, z);
      }
    }
    function speaker() {
      // Two stacked cabinets with recessed drivers, ports, seams and handles.
      for (const [i, height] of [h * .46, h * .52].entries()) {
        const y = i === 0 ? height / 2 : h * .46 + .025 + height / 2;
        soft(m.black, w - .008, height, d - .075, .035, 0, y, -.0375);
        box(m.grille, w - .09, height - .095, .025, 0, y, d / 2 - .065);
        const radius = w * (i === 0 ? .34 : .27), driverY = y + (i === 0 ? .07 : -.16);
        cylinder(m.black, radius, radius, .02, 0, driverY, d / 2 - .049, [Math.PI / 2, 0, 0]);
        cylinder(m.grille, radius * .60, radius * .88, .035, 0, driverY, d / 2 - .027, [Math.PI / 2, 0, 0]);
        ring(m.black, radius * .90, .018, 0, driverY, d / 2 - .020);
        cylinder(m.black, radius * .32, radius * .34, .02, 0, driverY, d / 2 - .013, [Math.PI / 2, 0, 0]);
        box(m.black, w * .56, .075, .035, 0, y - height * .36, d / 2 - .043);
        if (i === 1) box(m.black, w * .6, .17, .03, 0, y + height * .27, d / 2 - .047);
        for (const x of [-w / 2 + .004, w / 2 - .004]) box(m.grille, .008, .12, .27, x, y + .09, 0);
        box(m.brass, .06, .025, .008, w * .31, y - height * .35, d / 2 - .02);
      }
    }
    function dj() {
      soft(m.walnut, w, h - .14, d - .035, .04, 0, (h - .14) / 2, 0);
      soft(m.black, w, .075, d, .025, 0, h - .1025, 0);
      for (let i = 0; i < 16; i++) box(m.brass, .012, h - .33, .014, (i - 7.5) * (w - .23) / 16, (h - .33) / 2 + .10, d / 2 - .02);
      box(m.lilac, w - .3, .018, .012, 0, .13, d / 2 - .01);
      for (const x of [-.85, .85]) {
        soft(m.black, .71, .045, .57, .025, x, h - .044, 0);
        cylinder(m.grille, .217, .217, .018, x, h - .0125, -.045);
        cylinder(m.black, .17, .17, .009, x, h - .004, -.045);
        ring(m.brass, .204, .005, x, h - .002, -.045, [Math.PI / 2, 0, 0]);
        box(m.jade, .18, .006, .07, x + .23, h - .014, -.18);
        for (let i = 0; i < 4; i++) box(i === 0 ? m.amber : m.grille, .041, .01, .041, x - .13 + i * .063, h - .015, .21);
      }
      soft(m.black, .5, .06, .6, .018, 0, h - .033, 0);
      for (let i = 0; i < 4; i++) {
        const x = (i - 1.5) * .097;
        box(m.grille, .009, .006, .18, x, h - .001, .10);
        box(m.brass, .04, .012, .018, x, h - .004, .11 - i * .026);
        cylinder(m.grille, .017, .017, .014, x, h - .007, -.1);
      }
    }
    function bar() {
      soft(m.walnut, w - .05, h - .075, d - .12, .035, 0, (h - .075) / 2, 0);
      soft(m.stone, w, .075, d, .024, 0, h - .0375, 0);
      const count = 24;
      for (let i = 0; i < count; i++) box(m.brass, .014, h - .2, .013, (i + .5) * (w - .24) / count - (w - .24) / 2, (h - .2) / 2 + .065, -d / 2 + .018);
      box(m.amber, w - .23, .02, .012, 0, h - .09, -d / 2 + .009);
      // Foot rail is fully inside the counter's declared collision footprint.
      box(m.brass, w - .3, .035, .035, 0, .19, -d / 2 + .045);
      for (const x of [-w / 2 + .4, 0, w / 2 - .4]) box(m.brass, .025, .16, .035, x, .105, -d / 2 + .045);
      box(m.black, 1, .018, .31, -w * .2, h + .009, 0);
      for (let i = 0; i < 4; i++) {
        const x = -w * .2 - .3 + i * .2;
        cylinder(m.glass, .035, .03, .12, x, h + .078, 0);
      }
      // Suspended shades stay above the counter, outside player circulation.
      for (const x of [-w * .3, 0, w * .3]) {
        cylinder(m.black, .012, .012, .3, x, 2.98, 0);
        cylinder(m.brass, .075, .21, .18, x, 2.74, 0);
        cylinder(m.amber, .17, .17, .008, x, 2.649, 0);
      }
    }
    function backbar() {
      box(m.walnut, w, h, .08, 0, h / 2, -d / 2 + .04);
      box(m.black, w - .13, h - .16, .035, 0, h / 2, -d / 2 + .0975);
      for (const x of [-w / 2 + .04, w / 2 - .04]) box(m.brass, .08, h, d, x, h / 2, 0);
      for (const y of [.12, .85, 1.58, 2.31]) {
        box(m.walnut, w - .12, .05, d, 0, y, 0);
        box(m.amber, w - .2, .015, .015, 0, y - .035, d / 2 - .025);
        if (y > 2) continue;
        for (let i = 0; i < 12; i++) {
          const x = (i - 5.5) * (w - .6) / 12, bottleH = .26 + (i * 7 % 5) * .025;
          cylinder(i % 3 === 0 ? m.stone : m.glass, .048, .057, bottleH, x, y + .026 + bottleH / 2, 0);
          cylinder(m.glass, .024, .024, .065, x, y + .026 + bottleH + .0325, 0);
          cylinder(m.brass, .025, .025, .018, x, y + .026 + bottleH + .074, 0);
        }
      }
      for (const x of [-w / 6, w / 6]) box(m.brass, .035, h - .14, d - .04, x, h / 2, 0);
    }
    function panel() {
      box(m.walnut, w, h, d - .035, 0, h / 2, -.0175);
      const count = Math.min(16, Math.max(6, Math.round(w / .2)));
      for (let i = 0; i < count; i++) {
        const x = (i + .5) * (w - .14) / count - (w - .14) / 2;
        box(i % 4 === 0 ? m.brass : m.black, .023, h - .15, .028, x, h / 2, d / 2 - .014);
      }
      box(m.amber, w - .12, .025, .018, 0, h - .07, d / 2 - .009);
      // Recessed wall lights retain the panel's declared footprint.
      for (const x of [-w * .35, w * .35]) {
        box(m.black, .11, .35, .035, x, h * .64, d / 2 - .021);
        box(m.brass, .13, .055, .035, x, h * .64 + .15, d / 2 - .021);
        box(m.amber, .035, .24, .009, x, h * .64, d / 2 - .005);
      }
      if (screenHousing) {
        box(m.brass, w * .83, 1.24, .07, 0, 1.87, d / 2 + .035);
        box(m.black, w * .80, 1.14, .075, 0, 1.87, d / 2 + .075);
      }
    }
    function danceFloor() {
      box(m.black, w, h - .009, d, 0, (h - .009) / 2, 0);
      const count = 6, cellW = (w - .14) / count, cellD = (d - .14) / count;
      for (let x = 0; x < count; x++) for (let z = 0; z < count; z++) {
        const px = (x - 2.5) * cellW, pz = (z - 2.5) * cellD;
        box(m.floor, cellW - .017, .009, cellD - .017, px, h - .0045, pz);
        box((x + z) % 2 ? m.lilac : m.jade, cellW * .72, .002, .018, px, h + .001, pz + cellD * .30);
      }
      for (const x of [-w / 2 + .018, w / 2 - .018]) box(m.jade, .016, .002, d - .08, x, h + .001, 0);
      for (const z of [-d / 2 + .018, d / 2 - .018]) box(m.jade, w - .08, .002, .016, 0, h + .001, z);
    }
    function ceiling() {
      for (const x of [-w / 2 + .09, w / 2 - .09]) {
        box(m.black, .18, h, d, x, h / 2, 0);
        box(m.lilac, .025, .012, d - .1, x, .006, 0);
      }
      for (const z of [-d / 2 + .09, d / 2 - .09]) {
        box(m.black, w - .18, h, .18, 0, h / 2, z);
        box(m.amber, w - .22, .012, .025, 0, .006, z);
      }
      // No supports appear in a walkable zone. The frame is overhead only.
      for (const z of [-d * .22, d * .22]) {
        box(m.black, w - .18, h * .55, .08, 0, h * .7, z);
        for (const x of [-w * .3, 0, w * .3]) {
          cylinder(m.black, .105, .13, .095, x, -.0175, z);
          ring(m.brass, .105, .009, x, -.06, z, [Math.PI / 2, 0, 0]);
          cylinder(z < 0 && x !== 0 ? x < 0 ? m.lilac : m.jade : m.amber, .093, .093, .006, x, -.068, z);
        }
      }
      cylinder(m.black, .105, .13, .095, 0, -.0175, -d / 2 + .09);
      ring(m.brass, .105, .009, 0, -.06, -d / 2 + .09, [Math.PI / 2, 0, 0]);
      cylinder(m.amber, .093, .093, .006, 0, -.068, -d / 2 + .09);
    }
    try {
      if (value.id.startsWith("club-lounge")) sofa();
      else if (value.id.startsWith("club-table")) table();
      else if (value.id.startsWith("club-speaker")) speaker();
      else if (value.id === "dj-booth") dj();
      else if (value.id === "dance-floor") danceFloor();
      else if (value.id === "club-bar") bar();
      else if (value.id === "club-backbar") backbar();
      else if (value.id.startsWith("club-side-panel") || value.id.startsWith("club-back-panel")) panel();
      else if (value.id === "club-ceiling-frame") ceiling();
      else if (value.id.startsWith("club-bar-stool")) {
        cylinder(m.black, w * .46, w * .46, .035, 0, .0175, 0);
        cylinder(m.brass, .035, .035, h - .11, 0, (h - .11) / 2, 0);
        ring(m.brass, w * .35, .012, 0, .23, 0, [Math.PI / 2, 0, 0]);
        cylinder(m.upholstery, w * .48, w * .46, .1, 0, h - .05, 0);
      } else if (value.id === "dj-stage") {
        box(m.walnut, w, h, d, 0, h / 2, 0);
        box(m.lilac, w - .15, .012, .012, 0, h - .009, d / 2 - .015);
      } else {
        if (value.id.startsWith("wall-")) {
          box(m.wall, w, h - .15, d, 0, (h + .03) / 2, 0);
          box(m.black, w, .09, d, 0, .045, 0);
          box(m.black, w, .06, d, 0, h - .03, 0);
          if (value.id === "wall-back") {
            box(m.brass, 6.6, 1.24, .075, 0, 2.12, d / 2 + .03);
            box(m.black, 6.42, 1.08, .08, 0, 2.12, d / 2 + .07);
            box(m.jade, 6.3, .018, .012, 0, 1.62, d / 2 + .116);
          }
        } else box(m.wall, w, h, d, 0, h / 2, 0);
      }
      for (const [mat, batch] of batches) {
        const geometry = mergeGeometries(batch, false);
        if (!geometry) throw new Error(`Unable to merge nightclub fixture ${value.id}.`);
        meshes++;
        if (meshes > limits.meshes) { geometry.dispose(); throw new Error("Nightclub geometry exceeds its mesh budget."); }
        attach(geometry, mat);
        geometry.computeBoundingBox(); geometry.computeBoundingSphere();
        if (key) parts.push({ geometry, material: mat });
      }
      if (key) preparedFixtures.set(key, { parts, pieces: pieces - firstPiece, vertices: vertices - firstVertex });
    } finally {
      // Temporary merge inputs never enter the parent traversal.
      batches.forEach(batch => batch.forEach(geometry => geometry.dispose())); batches.clear();
    }
  }
  try { layout.fixtures.forEach(build); }
  finally { preparedFixtures.clear(); }
  // Light positions come from resolved fixture roots, including mirrored yaw.
  // Warm social areas and focused dance/stage lights need no shadow maps,
  // bloom, textures, extra render passes or a separate scheduling loop.
  const state: LightingState = { channels: [], lights: [], beams: [], screens: [], equalizerTransform: new THREE.Object3D(), direction: new THREE.Vector3(),
    up: new THREE.Vector3(0, -1, 0), sample: null, reducedMotion: false, powerFactor: 1 };
  let hash = 2166136261;
  for (let i = 0; i < layout.seed.length; i++) hash = Math.imul(hash ^ layout.seed.charCodeAt(i), 16777619);
  const phase = (hash >>> 0) / 4294967296 * Math.PI * 2;
  state.channels.push(
    { material: m.amber, base: .7, amplitude: 0, period: 32, phase },
    { material: m.lilac, base: .85, amplitude: .32, period: nightclubLimits.beatMs / 1000, phase: 0 },
    { material: m.jade, base: .8, amplitude: .3, period: nightclubLimits.beatMs / 1000, phase: Math.PI },
  );
  for (const definition of layout.fixtures.filter(fixture => fixture.id === "wall-back"
    || fixture.id.startsWith("club-side-panel") && fixture.id.endsWith("-0"))) {
    const owner = group.children.find(root => root.userData.lagosFixtureId === definition.id);
    if (!owner) continue;
    const main = definition.id === "wall-back";
    const count = main ? 24 : 12, width = main ? 6.1 : definition.size[0] * .74;
    const barGeometry = new THREE.BoxGeometry(width / count * .72, 1, .018);
    const equalizer = new THREE.InstancedMesh(barGeometry, main ? m.lilac : m.jade, count);
    equalizer.name = `${definition.id}:screen`; equalizer.position.z = definition.size[2] / 2 + .13;
    equalizer.position.y = main ? 1.66 : 1.38;
    equalizer.userData.screenWidth = width;
    equalizer.instanceMatrix.setUsage(THREE.DynamicDrawUsage); equalizer.raycast = () => {};
    equalizer.frustumCulled = false; owner.add(equalizer); state.screens.push(equalizer);
    pieces++; meshes++; vertices += barGeometry.getAttribute("position").count * equalizer.count;
    if (pieces > limits.pieces || meshes > limits.meshes || vertices > limits.vertices) throw new Error("Nightclub display exceeds its geometry budget.");
  }
  for (const definition of [
    { id: "club-bar", color: "#edbd86", base: 10, height: 2.5, distance: 7 },
    { id: "club-lounge--1", color: "#e4bc94", base: 5.5, height: 1.9, distance: 5.5 },
    { id: "club-lounge-1", color: "#e4bc94", base: 5.5, height: 1.9, distance: 5.5 },
    { id: "club-lounge-gather", color: "#e4bc94", base: 5.5, height: 1.9, distance: 5.5 },
  ]) {
    const fixture = group.children.find(root => root.userData.lagosFixtureId === definition.id);
    if (!fixture) continue;
    const light = new THREE.PointLight(definition.color, definition.base, definition.distance, 2);
    light.name = `${definition.id}:spill`; light.position.y = definition.height; light.castShadow = false; fixture.add(light);
    // Keep faces and seating steady while the dance lighting moves.
    state.lights.push({ light, base: definition.base, amplitude: 0, period: 32, phase });
  }
  const frame = group.children.find(root => root.userData.lagosFixtureId === "club-ceiling-frame");
  const danceFloor = layout.fixtures.find(fixture => fixture.id === "dance-floor");
  const frameDefinition = layout.fixtures.find(fixture => fixture.id === "club-ceiling-frame");
  const boothDefinition = layout.fixtures.find(fixture => fixture.id === "dj-booth");
  // Cone gradients suggest a light haze only inside the beams. They do
  // not cover the scene with fog or intercept floor/object/participant taps.
  function beamMaterial(color: string) {
    const value = new THREE.ShaderMaterial({
      uniforms: { tint: { value: new THREE.Color(color) }, strength: { value: .12 } },
      transparent: true, depthWrite: false, blending: THREE.AdditiveBlending, side: THREE.DoubleSide, forceSinglePass: true,
      vertexShader: `
        varying vec2 beamUv;
        varying vec3 beamNormal;
        varying vec3 beamView;
        void main() {
          beamUv = uv;
          vec4 viewPosition = modelViewMatrix * vec4(position, 1.0);
          beamNormal = normalize(normalMatrix * normal);
          beamView = -viewPosition.xyz;
          gl_Position = projectionMatrix * viewPosition;
        }
      `,
      fragmentShader: `
        uniform vec3 tint;
        uniform float strength;
        varying vec2 beamUv;
        varying vec3 beamNormal;
        varying vec3 beamView;
        void main() {
          float axialFade = smoothstep(0.0, 0.18, beamUv.y) * (1.0 - smoothstep(0.72, 1.0, beamUv.y));
          float edgeFade = pow(abs(dot(normalize(beamNormal), normalize(beamView))), 1.4);
          gl_FragColor = vec4(tint, strength * axialFade * edgeFade);
          #include <tonemapping_fragment>
          #include <colorspace_fragment>
        }
      `,
    });
    ownedMaterials.push(value); return value;
  }
  function addSpot(owner: THREE.Object3D, name: string, source: THREE.Vector3, target: THREE.Vector3,
    color: string, base: number, angle: number, period: number, lightPhase: number, haze: boolean) {
    const light = new THREE.SpotLight(color, base, 9, angle, .8, 2);
    light.name = name; light.position.copy(source); light.target.position.copy(target);
    light.castShadow = false; owner.add(light, light.target);
    state.lights.push({ light, base, amplitude: haze ? base * .32 : 0,
      period: haze ? nightclubLimits.beatMs / 1000 : period, phase: lightPhase });
    if (!haze) return;
    const geometry = new THREE.ConeGeometry(Math.tan(angle), 1, 24, 1, true);
    geometry.translate(0, -.5, 0);
    pieces++; vertices += geometry.getAttribute("position").count; meshes++;
    if (pieces > limits.pieces || vertices > limits.vertices || meshes > limits.meshes || state.beams.length >= limits.beams) {
      geometry.dispose(); throw new Error("Nightclub lighting exceeds its geometry budget.");
    }
    let material: THREE.ShaderMaterial;
    try { material = beamMaterial(color); } catch (error) { geometry.dispose(); throw error; }
    const mesh = new THREE.Mesh(geometry, material); mesh.name = `${name}:beam`;
    mesh.raycast = () => {}; mesh.position.copy(source); mesh.renderOrder = 1; owner.add(mesh);
    const beam: BeamState = { mesh, source: source.clone(), target: light.target.position,
      baseX: target.x, baseZ: target.z, travel: 1.8, period, phase: lightPhase, opacity: .12 };
    state.beams.push(beam); positionBeam(beam, state);
  }
  function frameTarget(frameFixture: CompanionPlaceFixture, fixture: CompanionPlaceFixture, height: number) {
    const target = new THREE.Vector3().fromArray(fixture.point).sub(new THREE.Vector3().fromArray(frameFixture.point));
    target.applyAxisAngle(new THREE.Vector3(0, 1, 0), -frameFixture.yaw); target.y += height;
    return target;
  }
  if (frame && frameDefinition && danceFloor) {
    for (const row of [-1, 1]) for (const sign of [-1, 1]) {
      // Convert the resolved floor centre into the ceiling frame's space.
      // Reflect the floor target with each lamp rather than guessing world X.
      const target = frameTarget(frameDefinition, danceFloor, danceFloor.size[1] + .035);
      target.x += sign * danceFloor.size[0] * .2;
      target.z += row * danceFloor.size[2] * .18;
      addSpot(frame, `dance-light-${row}-${sign}`, new THREE.Vector3(sign * frameDefinition.size[0] * .3, -.071, row * frameDefinition.size[2] * .22),
        target, sign < 0 ? "#bc71f4" : "#53dddc", 32, .48, row < 0 ? 4.8 : 7.2,
        phase + (sign < 0 ? 0 : Math.PI) + (row < 0 ? 0 : Math.PI / 2), true);
    }
  }
  if (frame && frameDefinition && boothDefinition) {
    addSpot(frame, "dj-light", new THREE.Vector3(0, -.071, -frameDefinition.size[2] / 2 + .09), frameTarget(frameDefinition, boothDefinition, boothDefinition.size[1]),
      "#e6c6a5", 12, .72, 32, phase, false);
  }
  if (state.lights.length > limits.lights) throw new Error("Nightclub lighting exceeds its light budget.");
  lightingStates.set(parent, state);
  // The ordinary parent disposer triggers this event. Release animation refs
  // even when an already-disposed parent remains in the caller's history.
  let cleanupGeometry: THREE.BufferGeometry | undefined;
  group.traverse(node => { if (!cleanupGeometry && node instanceof THREE.Mesh) cleanupGeometry = node.geometry; });
  if (cleanupGeometry) {
    const cleanup = () => {
      if (lightingStates.get(parent) === state) lightingStates.delete(parent);
      state.lights.forEach(channel => channel.light.dispose());
      state.screens.forEach(screen => screen.dispose()); state.screens.length = 0;
      state.channels.length = 0; state.lights.length = 0; state.beams.length = 0;
      cleanupGeometry?.removeEventListener("dispose", cleanup);
    };
    cleanupGeometry.addEventListener("dispose", cleanup);
  }
  group.userData.nightclubResources = { pieces, vertices, meshes, materials: Object.keys(m).length + state.beams.length,
    pointLights: state.lights.filter(channel => channel.light instanceof THREE.PointLight).length,
    spotLights: state.lights.filter(channel => channel.light instanceof THREE.SpotLight).length, beams: state.beams.length,
    screens: state.screens.length };
  return group;
}

function positionBeam(beam: BeamState, state: LightingState) {
  state.direction.subVectors(beam.target, beam.source);
  const length = state.direction.length();
  if (length <= .001) return;
  beam.mesh.scale.setScalar(length);
  beam.mesh.quaternion.setFromUnitVectors(state.up, state.direction.multiplyScalar(1 / length));
}

/** Call on createLagosPlaceGeometry's returned group, using absolute elapsed
 * seconds from the shared instance clock. Returns whether values changed,
 * not whether the caller should schedule another frame. The scene-power adapter
 * supplies the fifth argument; this function alone writes animated intensities. */
export function updateNightclubLighting(geometry: THREE.Group, elapsedSeconds: number, reducedMotion = false, visible = true, powerFactor = 1): boolean {
  const state = lightingStates.get(geometry);
  if (!state || !visible || !Number.isFinite(elapsedSeconds) || elapsedSeconds < 0 || !Number.isFinite(powerFactor)) return false;
  const power = Math.min(1, Math.max(0, powerFactor));
  const sample = reducedMotion ? 0 : Math.floor(elapsedSeconds * 20);
  // Invisible beams need no target motion or redraw during a settled outage.
  if (state.powerFactor === power && state.reducedMotion === reducedMotion && (state.sample === sample || power === 0)) return false;
  state.sample = sample; state.reducedMotion = reducedMotion; state.powerFactor = power;
  const sampledSeconds = sample / 20;
  for (const [screenIndex, screen] of state.screens.entries()) {
    const width = screen.userData.screenWidth as number;
    const mode = Math.floor(sampledSeconds / 9.6) % 3;
    for (let i = 0; i < screen.count; i++) {
      const wave = Math.sin(sampledSeconds * Math.PI * 2 / 1.2 + i * .73 + screenIndex);
      const pattern = mode === 0 ? wave : mode === 1 ? Math.cos(i * .8 - sampledSeconds * 2) : Math.sin(i * .37 + sampledSeconds) * wave;
      const height = reducedMotion ? .38 + (i % 5) * .09 : .13 + .75 * (.5 + .5 * pattern);
      state.equalizerTransform.position.set((i - (screen.count - 1) / 2) * width / screen.count, height / 2, 0);
      state.equalizerTransform.scale.set(1, height, 1); state.equalizerTransform.updateMatrix();
      screen.setMatrixAt(i, state.equalizerTransform.matrix);
    }
    screen.instanceMatrix.needsUpdate = true;
  }
  // Preserve the original clock curve and reduced-motion baseline on restore.
  for (const channel of state.channels) channel.material.emissiveIntensity = power * (channel.base + (reducedMotion ? 0
    : Math.sin((sampledSeconds % channel.period) / channel.period * Math.PI * 2 + channel.phase) * channel.amplitude));
  for (const channel of state.lights) channel.light.intensity = power * (channel.base + (reducedMotion ? 0
    : Math.sin((sampledSeconds % channel.period) / channel.period * Math.PI * 2 + channel.phase) * channel.amplitude));
  for (const beam of state.beams) {
    const cycle = (sampledSeconds % beam.period) / beam.period * Math.PI * 2 + beam.phase;
    beam.target.x = beam.baseX + (reducedMotion ? 0 : Math.sin(cycle) * beam.travel);
    beam.target.z = beam.baseZ + (reducedMotion ? 0 : Math.cos(cycle) * beam.travel * .65);
    beam.mesh.material.uniforms.strength.value = power * beam.opacity * (reducedMotion ? 1 : 1 + Math.sin(sampledSeconds * Math.PI * 2 / .6 + beam.phase) * .32);
    positionBeam(beam, state);
  }
  return true;
}
