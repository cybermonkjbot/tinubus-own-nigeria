import * as THREE from "three";
import type { CompanionPlaceLayout } from "../../../../../packages/utils/src/companionPlaces";
import type { CompanionWaterfrontProp } from "../../../../../packages/utils/src/companionWaterfront";
import { createWaterfrontWater } from "../city/waterfrontWater";
import type { CompanionAssetDelivery, CompanionAssetKey } from "../../../../../packages/utils/src/companionAssets";
import { mountOptionalSceneryModel } from "../appearance/optionalSceneryModel";
import { companionBoatAssetKey, companionBoatModelId, companionBoatBounds, companionBoatClearsDrySurfaces } from "../../../../../packages/utils/src/companionBoatAssets";

type Runtime = { water: ReturnType<typeof createWaterfrontWater> | null; floaters: { root: THREE.Group; y: number; phase: number }[];
  haze: { root: THREE.Group; base: number; phase: number }[]; lamps: THREE.MeshStandardMaterial[];
  unusedGeometries: Set<THREE.BufferGeometry>; models: ReturnType<typeof mountOptionalSceneryModel>[] };
const runtimes = new WeakMap<THREE.Group, Runtime>();

/** Original metre-scale parts. The stored layout owns every solid footprint and dry walking surface. */
export function createWaterfrontGeometry(layout: CompanionPlaceLayout, parent: THREE.Group,
  materials: THREE.Material[], textures: THREE.Texture[], includeWater = true,
  delivery?: CompanionAssetDelivery, changed: () => void = () => {},
  modelFits?: (bounds: THREE.Box3) => boolean) {
  const design = layout.waterfront;
  if (!design) throw new Error("The waterfront scenery recipe is missing.");
  const water = includeWater ? createWaterfrontWater(design.zone) : null;
  if (water) parent.add(water.mesh);
  const runtime: Runtime = { water, floaters: [], haze: [], lamps: [], unusedGeometries: new Set(), models: [] }; runtimes.set(parent, runtime);
  const palette = {
    timber: "#78604a", darkWood: "#443a2c", freshWood: "#b28e62", concrete: "#9a9a8d", steel: "#56625d",
    rust: "#79624b", black: "#232825", white: "#e5e2d7", blue: "#42718a", tarp: "#426f91", green: "#667b3e",
    coconut: "#816a41", sand: "#d4bd8a", mud: "#76694c", glass: "#52727b", yellow: "#cfa52d", orange: "#bd7550",
  };
  const m = Object.fromEntries(Object.entries(palette).map(([key, color]) => {
    const material = new THREE.MeshStandardMaterial({ color, roughness: key === "glass" ? .22 : key === "steel" ? .55 : .86,
      metalness: key === "steel" ? .35 : 0 }); materials.push(material); return [key, material];
  })) as Record<keyof typeof palette, THREE.MeshStandardMaterial>;
  // Metric grain gives timber, concrete and sand a stable finish without repeated giant UVs.
  for (const material of [m.timber, m.darkWood, m.freshWood, m.concrete, m.sand, m.mud]) {
    const timber = [m.timber, m.darkWood, m.freshWood].includes(material);
    material.onBeforeCompile = shader => {
      shader.vertexShader = "varying vec3 waterfrontPoint;\n" + shader.vertexShader;
      shader.vertexShader = shader.vertexShader.replace("#include <begin_vertex>", `#include <begin_vertex>
        vec4 waterfrontPosition=vec4(transformed,1.);
        #ifdef USE_INSTANCING
          waterfrontPosition=instanceMatrix*waterfrontPosition;
        #endif
        waterfrontPoint=(modelMatrix*waterfrontPosition).xyz;`);
      shader.fragmentShader = "varying vec3 waterfrontPoint;\n" + shader.fragmentShader;
      shader.fragmentShader = shader.fragmentShader.replace("#include <color_fragment>", `#include <color_fragment>
        float grain=fract(sin(dot(floor(waterfrontPoint.${timber ? "xy*vec2(19.,3.)" : "xz*17."}),vec2(127.1,311.7)))*43758.5453);
        diffuseColor.rgb*=.88+grain*.16;`);
    };
    material.customProgramCacheKey = () => timber ? "waterfront-timber-v2" : "waterfront-ground-v2";
  }
  const cube = new THREE.BoxGeometry(1, 1, 1), cylinder = new THREE.CylinderGeometry(1, 1, 1, 12), sphere = new THREE.SphereGeometry(1, 12, 8);
  const ring = new THREE.TorusGeometry(1, .23, 8, 20);
  [cube, cylinder, sphere, ring].forEach(geometry => runtime.unusedGeometries.add(geometry));
  const mesh = (owner: THREE.Object3D, geometry: THREE.BufferGeometry, material: THREE.Material, x: number, y: number, z: number,
    w = 1, h = 1, d = 1) => {
    runtime.unusedGeometries.delete(geometry);
    const object = new THREE.Mesh(geometry, material); object.position.set(x, y, z); object.scale.set(w, h, d);
    object.castShadow = true; object.receiveShadow = true; owner.add(object); return object;
  };
  // Recipes finish rotating their pieces before batching. Prop roots still own
  // movement, service picking and local culling; transparent pieces stay separate.
  const batchParts = (owner: THREE.Group) => {
    const batches = new Map<string, THREE.Mesh[]>();
    for (const part of owner.children) {
      if (!(part instanceof THREE.Mesh) || part instanceof THREE.InstancedMesh || part.children.length
        || Array.isArray(part.material) || part.material.transparent || !part.visible
        || part.scale.x <= 0 || part.scale.y <= 0 || part.scale.z <= 0
        || part.geometry !== cube && part.geometry !== cylinder && part.geometry !== sphere && part.geometry !== ring) continue;
      const key = `${part.geometry.id}:${part.material.id}:${part.castShadow}:${part.receiveShadow}`;
      const pieces = batches.get(key);
      if (pieces) pieces.push(part); else batches.set(key, [part]);
    }
    for (const pieces of batches.values()) {
      if (pieces.length < 2) continue;
      const first = pieces[0];
      const batch = new THREE.InstancedMesh(first.geometry, first.material, pieces.length);
      try {
        batch.castShadow = first.castShadow; batch.receiveShadow = first.receiveShadow;
        batch.matrixAutoUpdate = false;
        for (let i = 0; i < pieces.length; i++) {
          pieces[i].updateMatrix(); batch.setMatrixAt(i, pieces[i].matrix);
        }
        batch.instanceMatrix.needsUpdate = true;
        batch.computeBoundingBox(); batch.computeBoundingSphere();
      } catch (error) {
        batch.dispose(); throw error;
      }
      owner.add(batch);
      for (const part of pieces) owner.remove(part);
    }
  };
  const box = (g: THREE.Object3D, mat: THREE.Material, x: number, y: number, z: number, w: number, h: number, d: number) =>
    mesh(g, cube, mat, x, y, z, w, h, d);
  const tubeDirection = new THREE.Vector3(), tubeUp = new THREE.Vector3(0, 1, 0);
  const tube = (g: THREE.Object3D, mat: THREE.Material, a: readonly number[], b: readonly number[], radius = .05) => {
    tubeDirection.set(b[0] - a[0], b[1] - a[1], b[2] - a[2]);
    const object = mesh(g, cylinder, mat, (a[0] + b[0]) / 2, (a[1] + b[1]) / 2, (a[2] + b[2]) / 2, radius, tubeDirection.length(), radius);
    object.quaternion.setFromUnitVectors(tubeUp, tubeDirection.normalize()); return object;
  };
  const wheel = (g: THREE.Object3D, x: number, y: number, z: number, radius: number, width = .16) => {
    const tyre = mesh(g, ring, m.black, x, y, z, radius, radius, width); tyre.rotation.y = Math.PI / 2;
    const hub = mesh(g, cylinder, m.steel, x, y, z, radius * .46, width * 1.8, radius * .46); hub.rotation.z = Math.PI / 2;
    return tyre;
  };
  const roof = (g: THREE.Object3D, w: number, d: number, y: number, mat = m.steel) => {
    for (const side of [-1, 1]) {
      const panel = box(g, mat, 0, y + .55, side * d / 4, w, .1, Math.hypot(d / 2, 1.1)); panel.rotation.x = side * .23;
      for (let x = -w / 2; x < w / 2; x += .42) {
        const seam = box(g, mat, x, y + .61, side * d / 4, .035, .025, Math.hypot(d / 2, 1.1)); seam.rotation.x = side * .23;
      }
    }
  };
  const chair = (g: THREE.Object3D, x: number, z: number, colour = m.white) => {
    box(g, colour, x, .5, z, .5, .08, .5); box(g, colour, x, .85, z - .22, .5, .6, .08);
    for (const dx of [-.2, .2]) for (const dz of [-.2, .2]) tube(g, colour, [x + dx, .05, z + dz], [x + dx, .5, z + dz], .035);
  };
  const net = (g: THREE.Object3D, w: number, h: number, y = .2) => {
    for (let x = -w / 2; x <= w / 2; x += .28) tube(g, m.white, [x, y, 0], [x, y + h, 0], .008);
    for (let level = y; level <= y + h; level += .28) tube(g, m.white, [-w / 2, level, 0], [w / 2, level, 0], .008);
  };
  const smokeMaterial = new THREE.MeshBasicMaterial({ color: "#a79873", transparent: true, opacity: .08, depthWrite: false });
  materials.push(smokeMaterial);
  const smoke = (g: THREE.Group, x: number, y: number, z: number, heavy = false) => {
    const plume = new THREE.Group(); plume.position.set(x, y, z); g.add(plume);
    for (let i = 0; i < (heavy ? 12 : 6); i++) {
      const puff = mesh(plume, sphere, smokeMaterial, Math.sin(i * 1.8) * .4 + i * .14, i * .65, Math.cos(i) * .25,
        .3 + i * .1, .55 + i * .12, .4 + i * .1);
      puff.castShadow = false; puff.receiveShadow = false;
    }
    runtime.haze.push({ root: plume, base: y, phase: runtime.haze.length * 1.3 });
  };
  function boat(g: THREE.Group, p: CompanionWaterfrontProp) {
    const kind = p.boat ?? "canoe", [w, , length] = p.size, canoe = kind === "canoe", barge = kind === "sand-barge", ski = kind === "jet-ski";
    const wood = canoe || barge, mat = wood ? m.darkWood : m.white;
    // Open hull strips with curved chines and tapered bows, not a closed block.
    const vertices: number[] = [], indices: number[] = [], rows = 24;
    for (let i = 0; i <= rows; i++) {
      const t = i / rows, z = (t - .5) * length;
      const taper = barge ? .93 : Math.pow(Math.max(.02, Math.sin(Math.PI * t)), canoe ? .48 : .3);
      const profile = [[-w * taper / 2, .58], [-w * taper * .37, .1], [0, -.12], [w * taper * .37, .1], [w * taper / 2, .58]];
      profile.forEach(([x, y]) => vertices.push(x, y + (t < .18 ? (.18 - t) * 2 : 0), z));
      if (i) for (let j = 0; j < 4; j++) { const a = (i - 1) * 5 + j, b = i * 5 + j; indices.push(a, b, a + 1, a + 1, b, b + 1); }
    }
    const hull = new THREE.BufferGeometry(); hull.setAttribute("position", new THREE.Float32BufferAttribute(vertices, 3)); hull.setIndex(indices); hull.computeVertexNormals();
    const hullMaterial = mat.clone(); hullMaterial.side = THREE.DoubleSide; materials.push(hullMaterial);
    mesh(g, hull, hullMaterial, 0, 0, 0);
    for (const side of [-1, 1]) for (let i = 1; i < rows; i++) {
      const z = (i / rows - .5) * length, taper = barge ? .93 : Math.pow(Math.sin(Math.PI * i / rows), canoe ? .48 : .3);
      box(g, mat, side * w * taper / 2, .58, z, .08, .1, length / rows + .025);
    }
    for (const z of [-length * .3, 0, length * .28]) box(g, wood ? m.timber : m.white, 0, .2, z, w * .78, .09, .65);
    if (canoe) {
      for (let z = -length * .35; z < length * .35; z += .58) {
        tube(g, m.timber, [-w * .3, .14, z], [w * .3, .14, z], .035);
      }
      tube(g, m.freshWood, [-.28, .55, -1.8], [.32, .62, 1.6], .035);
      box(g, m.freshWood, .33, .62, 1.8, .19, .05, .46);
    } else if (barge) {
      box(g, m.timber, 0, .18, 0, w * .86, .15, length * .84);
      for (let i = 0; i < 28; i++) mesh(g, sphere, m.mud, Math.sin(i * 2.4) * w * .28,
        .7 + Math.sin(i * .7) ** 2 * 1.7, Math.cos(i * 1.3) * length * .32, 1.4, 1.1, 1.8);
      box(g, m.steel, 0, .75, length * .4, 2.5, 1.2, 1.8);
      tube(g, m.black, [.8, 1.2, length * .4], [.8, 3.4, length * .4], .12);
      tube(g, m.rust, [-2, .5, -5], [-3, -.6, -7], .2);
      smoke(g, .8, 3.4, length * .4);
    } else if (ski) {
      box(g, m.black, 0, .69, .2, w * .6, .32, length * .5);
      tube(g, m.steel, [0, .7, -.65], [0, 1.05, -.65], .045);
      tube(g, m.black, [-.35, 1.05, -.65], [.35, 1.05, -.65], .04);
    } else {
      box(g, m.white, 0, .28, -length * .4, w * .6, .12, length * .17);
      const cabin = kind === "yacht";
      if (cabin) {
        box(g, m.white, 0, 1.02, -2, w * .8, 1.4, length * .3);
        box(g, m.glass, 0, 1.49, -length * .16 - 2, w * .7, .58, .08);
        box(g, m.white, 0, 1.85, -2, w * .84, .12, length * .33);
        const led = new THREE.MeshStandardMaterial({ color: "#afcacc", emissive: "#7097a8", emissiveIntensity: .12 });
        materials.push(led); runtime.lamps.push(led);
        for (const side of [-1, 1]) box(g, led, side * w * .47, .61, .3, .03, .03, length * .62);
      } else {
        for (const x of [-w * .23, w * .23]) for (let z = -2.3; z < 3; z += .58) {
          box(g, m.blue, x, .63, z, w * .38, .13, .42); box(g, m.blue, x, .91, z + .18, w * .38, .46, .07);
        }
        box(g, m.tarp, 0, 1.95, 0, w * .98, .06, length * .7);
        for (const x of [-w * .44, w * .44]) for (const z of [-length * .32, length * .32])
          tube(g, m.steel, [x, .55, z], [x, 1.95, z], .045);
      }
      for (const x of cabin ? [-1, 0, 1] : [-.55, .55]) {
        box(g, m.black, x, .45, length * .5, .44, .62, .5);
        box(g, m.steel, x, -.1, length * .53, .12, .7, .17);
        tube(g, m.steel, [x - .18, -.4, length * .54], [x + .18, -.4, length * .54], .045);
      }
    }
    runtime.floaters.push({ root: g, y: g.position.y, phase: p.variant * 1.8 });
  }
  function hut(g: THREE.Group, p: CompanionWaterfrontProp, stilt: boolean, white = false) {
    const [w, h, d] = p.size, floor = stilt ? 1.4 : .15, mat = white ? m.white : p.variant % 2 ? m.timber : m.darkWood;
    const wallHeight = stilt ? h * .46 : h - 1;
    box(g, m.timber, 0, floor, 0, w, .18, d);
    if (stilt) for (const x of [-w * .43, w * .43]) for (const z of [-d * .43, d * .43])
      tube(g, m.darkWood, [x, -1.8, z], [x, floor + wallHeight, z], .12);
    for (const side of [-1, 1]) {
      box(g, mat, side * w * .49, floor + wallHeight / 2, 0, .14, wallHeight, d);
      box(g, mat, 0, floor + wallHeight / 2, -d * .49, w, wallHeight, .14);
      box(g, mat, side * (w / 4 + .42), floor + wallHeight / 2, d * .49, w / 2 - .84, wallHeight, .14);
      const window = box(g, m.black, side * w * .3, floor + wallHeight * .61, d * .501, 1.1, .75, .025);
      box(g, m.blue, window.position.x - .57, window.position.y, window.position.z + .04, .36, .8, .055);
    }
    for (let x = -w / 2; x <= w / 2; x += .25) box(g, mat, x, floor + wallHeight / 2, d * .498, .018, wallHeight, .024);
    roof(g, w + .7, d + .7, floor + wallHeight, white ? m.white : m.rust);
    if (stilt && h > 5) {
      box(g, m.timber, 0, floor + wallHeight + .2, 0, w, .15, d);
      for (const x of [-w * .44, w * .44]) tube(g, m.timber, [x, floor + wallHeight, d / 2], [x, h - .4, d / 2], .09);
      net(g, w * .85, .75, floor + wallHeight + .15);
    }
  }
  function palm(g: THREE.Group, p: CompanionWaterfrontProp, mangrove = false) {
    const height = p.size[1];
    for (let i = 0; i < 8; i++) tube(g, m.coconut, [Math.sin(i * .15) * .18, i * height / 8, 0],
      [Math.sin((i + 1) * .15) * .18, (i + 1) * height / 8, .03 * i], mangrove ? .2 - i * .012 : .19 - i * .009);
    if (mangrove) for (let i = 0; i < 9; i++) {
      const angle = i * Math.PI * 2 / 9;
      const x = Math.cos(angle) * 2.6, z = Math.sin(angle) * 2.6;
      tube(g, m.darkWood, [0, 2.1, 0], [x * .7, .4, z * .7], .1); tube(g, m.darkWood, [x * .7, .4, z * .7], [x, -.6, z], .055);
      mesh(g, sphere, m.green, x * .55, height - 1 + i % 3 * .35, z * .55, 1.6, 1.1, 1.5);
    } else for (let i = 0; i < 9; i++) {
      const angle = i * Math.PI * 2 / 9, dx = Math.cos(angle), dz = Math.sin(angle);
      tube(g, m.green, [0, height, .2], [dx * 3.2, height - .8, dz * 3.2], .045);
      for (let j = 1; j < 9; j++) for (const side of [-1, 1]) {
        const t = j / 9, leaf = box(g, m.green, dx * t * 3.2 - dz * side * .35, height - t * .8 - .1,
          dz * t * 3.2 + dx * side * .35, .1, .035, .8 * (1 - t * .55)); leaf.rotation.set(.28, -angle + side * .5, -.3);
      }
      mesh(g, sphere, m.coconut, dx * .35, height - .3, dz * .35, .13, .16, .13);
    }
  }
  function market(g: THREE.Group, p: CompanionWaterfrontProp) {
    const [w, h, d] = p.size, phone = p.kind === "phone-stall";
    box(g, m.timber, 0, .88, 0, w, .11, d);
    for (const x of [-w / 2 + .12, w / 2 - .12]) for (const z of [-d / 2 + .12, d / 2 - .12]) tube(g, m.timber, [x, 0, z], [x, h, z], .065);
    box(g, m.tarp, 0, h, 0, w + .12, .055, d + .15);
    box(g, m.timber, 0, .43, d * .45, w, .82, .08);
    if (phone) {
      box(g, m.blue, 0, 1.9, -d * .42, w * .8, 1.7, .1);
      for (let x = -w * .35; x <= w * .35; x += .38) for (const y of [1.4, 1.8, 2.2, 2.6])
        box(g, p.variant % 2 ? m.white : m.black, x, y, -d * .39, .19, .27, .06);
    } else for (let i = 0; i < 8; i++) {
      const x = (i % 4 - 1.5) * w / 5, z = (Math.floor(i / 4) - .5) * d / 2;
      box(g, m.freshWood, x, 1.04, z, w / 5 - .12, .24, d / 3);
      for (let j = 0; j < 7; j++) mesh(g, sphere, i % 2 ? m.orange : m.green,
        x + Math.sin(j * 2.4) * .35, 1.24 + (j % 2) * .07, z + Math.cos(j) * .25, .12, .1, .1);
    }
  }
  function tyreStack(g: THREE.Group, w: number, h: number, d: number) {
    for (let y = .2; y < h - .1; y += .36) for (const x of [-w * .24, w * .24]) {
      const tyre = mesh(g, ring, m.black, x, y, 0, Math.min(w * .21, .55), Math.min(w * .21, .55), .23); tyre.rotation.x = Math.PI / 2;
    }
    box(g, m.steel, 0, .05, 0, w, .1, d);
  }
  function prop(p: CompanionWaterfrontProp) {
    const g = new THREE.Group(); g.name = p.id; g.position.fromArray(p.point); g.rotation.y = p.yaw;
    if (p.serviceId) g.userData.lagosServiceId = p.serviceId; parent.add(g);
    const [w, h, d] = p.size;
    switch (p.kind) {
      case "boat": boat(g, p); break;
      case "hut": case "stilt-house": hut(g, p, p.kind === "stilt-house"); break;
      case "palm": case "mangrove": palm(g, p, p.kind === "mangrove"); break;
      case "beach-club": {
        for (const side of [-1, 1]) box(g, m.white, side * w * .46, h * .45, 0, w * .08, h * .9, d);
        box(g, m.white, 0, h * .45, -d * .46, w, h * .9, d * .08); box(g, m.white, 0, h, 0, w + .3, .22, d + .3);
        for (const x of [-w * .31, 0, w * .31]) {
          box(g, m.freshWood, x, .8, 1, 3.3, .12, 1.5); for (const z of [-.4, 2.3]) for (const dx of [-.7, .7]) chair(g, x + dx, z);
          tube(g, m.timber, [x, .1, -d * .1], [x, h - .1, -d * .1], .08);
        }
        box(g, m.white, 0, .5, -d * .34, w * .82, 1, 1.4); break;
      }
      case "lounger": {
        const seat = box(g, m.white, 0, .55, 0, .72, .11, 1.9); seat.rotation.x = -.07;
        const back = box(g, m.white, 0, .82, -.75, .72, .1, .7); back.rotation.x = -.65;
        for (const x of [-.29, .29]) for (const z of [-.65, .65]) tube(g, m.timber, [x, .05, z], [x, .55, z], .045);
        tube(g, m.timber, [.5, 0, -.7], [.5, 2.2, -.7], .035);
        const canopy = mesh(g, new THREE.ConeGeometry(.9, .32, 12, 1, true), m.white, .5, 2.1, -.7); canopy.material = m.white; break;
      }
      case "quad-bike": {
        box(g, m.black, 0, .6, 0, .85, .22, 1.8); box(g, m.yellow, 0, .77, -.2, 1.05, .26, 1.1);
        box(g, m.black, 0, .98, .25, .45, .15, .75);
        for (const x of [-.57, .57]) for (const z of [-.72, .72]) wheel(g, x, .35, z, .28, .25);
        tube(g, m.steel, [0, .85, -.55], [0, 1.2, -.55]); tube(g, m.black, [-.38, 1.2, -.55], [.38, 1.2, -.55]); break;
      }
      case "log-raft": {
        for (let x = -w / 2 + .4; x < w / 2; x += .8) {
          const log = mesh(g, cylinder, m.darkWood, x, .15, 0, .39, d, .39); log.rotation.x = Math.PI / 2;
          for (const z of [-d / 2, d / 2]) { const end = mesh(g, cylinder, m.freshWood, x, .15, z, .36, .02, .36); end.rotation.x = Math.PI / 2; }
        }
        for (const z of [-d * .31, d * .31]) tube(g, m.black, [-w / 2, .58, z], [w / 2, .58, z], .025);
        runtime.floaters.push({ root: g, y: p.point[1], phase: p.variant }); break;
      }
      case "wood-pile": {
        for (let y = .2; y < h; y += .32) for (let x = -w * .44 + y * .4; x < w * .44 - y * .4; x += .38) {
          const log = mesh(g, cylinder, y % .6 < .3 ? m.timber : m.freshWood, x, y, 0, .15, d * (1 - y / h * .3), .15); log.rotation.x = Math.PI / 2;
        }
        if (p.id === "offcut-pyramid") {
          const embers = new THREE.MeshStandardMaterial({ color: "#765139", emissive: "#b45b25", emissiveIntensity: .45, roughness: 1 });
          materials.push(embers);
          for (let i = 0; i < 16; i++) box(g, embers, Math.sin(i * 2.4) * w * .24, .12, Math.cos(i * 1.8) * d * .22, .24, .08, .33);
          smoke(g, 0, h * .4, 0, true);
        }
        break;
      }
      case "sawmill": {
        roof(g, w, d, h - 1, m.rust);
        for (const x of [-w * .46, w * .46]) for (const z of [-d * .46, d * .46]) tube(g, m.timber, [x, 0, z], [x, h - 1, z], .12);
        box(g, m.steel, 0, 1, 0, w * .8, .18, 1.9);
        for (let x = -w * .34; x < w * .35; x += .8) { const roller = mesh(g, cylinder, m.steel, x, 1.2, 0, .12, 1.6, .12); roller.rotation.x = Math.PI / 2; }
        const saw = mesh(g, new THREE.CylinderGeometry(.8, .8, .06, 48), m.steel, 0, 1.6, 0); saw.rotation.z = Math.PI / 2;
        box(g, m.yellow, 0, 1.8, -.65, 1.5, 1, .22);
        for (const x of [-w * .34, w * .34]) box(g, m.darkWood, x, .5, 0, .16, 1, 1.8);
        box(g, m.black, w * .32, .7, 2, 2, 1.2, 1.4); break;
      }
      case "generator": {
        box(g, m.rust, 0, h * .42, 0, w, h * .8, d); box(g, m.black, 0, .13, 0, w + .12, .26, d + .12);
        for (let x = -w * .35; x < w * .36; x += .17) box(g, m.black, x, h * .48, d / 2 + .02, .075, h * .48, .03);
        tube(g, m.black, [w * .25, h * .75, 0], [w * .25, h + .5, 0], .07);
        smoke(g, w * .25, h + .5, 0); break;
      }
      case "jetty": {
        for (let z = -d / 2 + .16; z < d / 2; z += .32) box(g, m.timber, 0, -.08, z, w, .16, .3);
        for (const x of [-w / 2 + .2, w / 2 - .2]) for (let z = -d / 2; z <= d / 2; z += 3) tube(g, m.darkWood, [x, -2, z], [x, .35, z], .12);
        // Open boarding edges. Rails never close the approach or arrival corridor.
        for (const x of [-w / 2 + .12, w / 2 - .12]) box(g, m.darkWood, x, -.27, 0, .16, .2, d); break;
      }
      case "bollard": {
        mesh(g, cylinder, m.black, 0, .3, 0, .16, .6, .16); box(g, m.black, 0, .62, 0, .55, .1, .16); break;
      }
      case "fish-rack": case "net-rack": {
        for (const x of [-w * .43, w * .43]) tube(g, m.timber, [x, 0, 0], [x, h, 0], .055);
        tube(g, m.timber, [-w * .43, h, 0], [w * .43, h, 0], .055);
        if (p.kind === "net-rack") net(g, w * .8, h * .8);
        else for (let x = -w * .36; x <= w * .36; x += .22) for (const y of [.6, 1.1, 1.5])
          mesh(g, sphere, m.rust, x, Math.min(y, h - .1), 0, .06, .17, .035);
        break;
      }
      case "bridge-deck": {
        box(g, m.concrete, 0, h / 2, 0, w, h, d);
        for (let x = -w * .44; x <= w * .44; x += 4) box(g, m.concrete, x, -.5, 0, 1.1, 1.1, d);
        for (let z = -d / 2; z < d / 2; z += 7.5) box(g, m.black, 0, h + .002, z, w, .006, .07);
        for (const x of [-w / 2 + .15, w / 2 - .15]) box(g, m.concrete, x, h + .55, 0, .3, 1.1, d);
        break;
      }
      case "bridge-pier": {
        box(g, m.concrete, 0, h / 2, 0, w, h, d); box(g, m.concrete, 0, .12, 0, w + 1, .24, d + 1);
        box(g, m.concrete, 0, h - .2, 0, w + 2, .6, d + 1); for (const x of [-w * .4, w * .4]) box(g, m.rust, x, h * .43, d / 2 + .005, .05, h * .58, .01); break;
      }
      case "market-stall": case "phone-stall": market(g, p); break;
      case "food-grill": {
        market(g, p); box(g, m.black, 0, 1.12, 0, w * .7, .2, d * .6);
        for (let x = -w * .32; x < w * .33; x += .15) tube(g, m.steel, [x, 1.25, -d * .25], [x, 1.25, d * .25], .012);
        for (let i = 0; i < 12; i++) mesh(g, sphere, m.rust, (i % 6 - 2.5) * .28, 1.31, Math.floor(i / 6) * .45 - .2, .1, .05, .12);
        mesh(g, cylinder, m.black, w * .3, 1.2, d * .27, .34, .2, .34);
        smoke(g, 0, 1.5, 0); break;
      }
      case "mechanic-bay": {
        roof(g, w, d, h - .75, m.tarp);
        for (const x of [-w * .46, w * .46]) for (const z of [-d * .46, d * .46]) tube(g, m.steel, [x, 0, z], [x, h - .75, z], .06);
        box(g, m.steel, -w * .3, .75, 0, 2.8, 1.5, 1.4);
        for (let i = 0; i < 5; i++) box(g, m.black, -w * .3, .2 + i * .28, .72, 2.5, .04, .025);
        for (let i = 0; i < 6; i++) box(g, m.rust, 1 + (i % 3) * .46, .25, Math.floor(i / 3) * .7, .4, .5, .6);
        tyreStack(g, w * .27, 1.3, 1.5); break;
      }
      case "tyre-stack": tyreStack(g, w, h, d); break;
      case "danfo": {
        box(g, m.yellow, 0, 1.25, 0, w * .9, 1.5, d * .9); box(g, m.yellow, 0, 2.25, 0, w * .88, .6, d * .85);
        for (const side of [-1, 1]) {
          box(g, m.black, side * w * .457, 1.2, 0, .03, .14, d * .86);
          for (let z = -d * .31; z < d * .36; z += .85) box(g, m.glass, side * w * .453, 2.02, z, .035, .53, .65);
          for (const z of [-d * .3, d * .3]) wheel(g, side * w * .43, .43, z, .35, .22);
        }
        box(g, m.glass, 0, 2, -d * .456, w * .74, .55, .04); box(g, m.steel, 0, .65, -d * .47, w * .9, .17, .15);
        for (const x of [-w * .32, w * .32]) box(g, m.white, x, .98, -d * .459, .32, .21, .03); break;
      }
      case "cart": {
        box(g, m.timber, 0, .8, 0, w, .14, d); for (const x of [-w * .41, w * .41]) wheel(g, x, .35, 0, .27);
        for (const x of [-w * .35, w * .35]) tube(g, m.steel, [x, .65, 0], [x, 1.3, -d * .7], .04);
        box(g, m.freshWood, 0, 1.06, 0, w * .7, .4, d * .7); break;
      }
      case "sleeping-platform": {
        for (const x of [-w * .42, w * .42]) box(g, m.concrete, x, h * .3, 0, .5, h * .6, d * .7);
        for (let x = -w / 2; x < w / 2; x += .35) box(g, m.timber, x, h * .64, 0, .33, .1, d);
        box(g, m.blue, 0, h * .73, 0, w * .88, .11, d * .8); box(g, m.white, -w * .3, h * .84, 0, .55, .15, d * .65); break;
      }
      case "laundry": {
        tube(g, m.black, [-w / 2, h, 0], [w / 2, h, 0], .012);
        for (let x = -w * .4, i = 0; x < w * .42; x += 1, i++) box(g, i % 2 ? m.blue : m.white, x, h - .45, 0, .65, .9, .025); break;
      }
      case "stair": {
        for (let i = 0; i < 14; i++) box(g, m.concrete, 0, (i + 1) * h / 28, -d / 2 + (i + .5) * d / 14, w, (i + 1) * h / 14, d / 14);
        for (const x of [-w / 2 + .12, w / 2 - .12]) tube(g, m.steel, [x, .9, -d / 2], [x, h + .9, d / 2], .045); break;
      }
      case "plaza": {
        for (let y = 0; y < h; y += 3) {
          box(g, m.concrete, 0, y + .15, 0, w, .3, d);
          for (const x of [-w * .45, w * .45]) for (const z of [-d * .44, 0, d * .44]) box(g, m.concrete, x, y + 1.5, z, .6, 3, .6);
          for (let z = -d * .35; z < d * .4; z += 4) box(g, m.blue, w / 2 + .01, y + 1.45, z, .04, 2.4, 3.1);
        } break;
      }
      case "tetrapod": {
        for (const end of [[0, 2, 0], [-1.4, -.1, .8], [1.4, -.1, .8], [0, -.1, -1.5]]) tube(g, m.concrete, [0, .6, 0], end, .43); break;
      }
      case "rock-groyne": {
        for (let i = 0; i < 28; i++) mesh(g, sphere, m.concrete, Math.sin(i * 2.4) * w * .25,
          Math.sin(i) ** 2 * 1.1, -d / 2 + i * d / 28, 1 + i % 3 * .2, .7, 1.3); break;
      }
      case "container-ship": {
        box(g, m.black, 0, 2, 0, w, 5, d); box(g, m.rust, 0, 5, 0, w, .5, d);
        for (let x = -w * .35; x < w * .4; x += 4) for (let z = -d * .4; z < d * .3; z += 10) for (let level = 0; level < 3; level++)
          box(g, (Math.round(x) + level) % 2 ? m.blue : m.orange, x, 6.8 + level * 2.6, z, 3.8, 2.45, 9.7);
        box(g, m.white, 0, 12, d * .35, w * .7, 12, d * .13); box(g, m.glass, 0, 17, d * .281, w * .6, 1.4, .1); break;
      }
      case "hyacinth": case "reeds": {
        for (let i = 0; i < 22; i++) {
          const angle = i * 2.4, spread = Math.sqrt(i / 22), x = Math.cos(angle) * w * spread / 2, z = Math.sin(angle) * d * spread / 2;
          const leaf = mesh(g, sphere, m.green, x, .04, z, .18, .03, .12); leaf.rotation.y = angle;
          if (i % 4 === 0) tube(g, m.green, [x, .03, z], [x, .22, z], .016);
        }
        runtime.floaters.push({ root: g, y: p.point[1] - .16, phase: p.variant }); break;
      }
      case "debris": {
        for (let i = 0; i < 8; i++) box(g, i % 2 ? m.white : m.blue, Math.sin(i * 2.4) * w * .4, .04, Math.cos(i * 1.2) * d * .4, .16, .035, .23);
        runtime.floaters.push({ root: g, y: -.17, phase: p.variant }); break;
      }
      case "oil-sheen": {
        const sheen = new THREE.MeshStandardMaterial({ color: "#3c3342", roughness: .1, metalness: .6, transparent: true, opacity: .28 }); materials.push(sheen);
        const disc = mesh(g, new THREE.CircleGeometry(1, 24), sheen, 0, -.15, 0, w / 2, d / 2, 1); disc.rotation.x = -Math.PI / 2; break;
      }
      case "sign": {
        for (const x of [-w * .4, w * .4]) tube(g, m.steel, [x, 0, 0], [x, h, 0], .04);
        box(g, m.blue, 0, h - .4, 0, w, .8, .08);
        const canvas = document.createElement("canvas"); canvas.width = 1024; canvas.height = 256;
        const context = canvas.getContext("2d");
        if (context) {
          context.fillStyle = "#42718a"; context.fillRect(0, 0, 1024, 256); context.fillStyle = "#f1eee3";
          context.font = "600 66px sans-serif"; context.textAlign = "center"; context.textBaseline = "middle";
          context.fillText(p.text ?? layout.label, 512, 128, 940);
          const texture = new THREE.CanvasTexture(canvas); texture.colorSpace = THREE.SRGBColorSpace; textures.push(texture);
          const signMaterial = new THREE.MeshStandardMaterial({ map: texture, roughness: .8 }); materials.push(signMaterial);
          const face = mesh(g, new THREE.PlaneGeometry(w - .04, .76), signMaterial, 0, h - .4, .045); face.castShadow = false;
        } break;
      }
      case "drain": {
        box(g, m.black, 0, -.08, 0, w, .1, d);
        for (const x of [-w / 2, w / 2]) box(g, m.concrete, x, .04, 0, .14, .08, d); break;
      }
      case "prayer-space": {
        box(g, m.tarp, 0, h, 0, w, .08, d);
        for (const x of [-w * .45, w * .45]) tube(g, m.timber, [x, 0, 0], [x, h, 0], .05);
        for (let x = -w * .4; x < w * .45; x += .9) box(g, m.green, x, .012, 0, .65, .024, d * .7); break;
      }
    }
    batchParts(g);
    if (delivery && (p.kind === "danfo" || p.kind === "boat" && companionBoatClearsDrySurfaces(p, design.surfaces))) {
      const key: CompanionAssetKey = p.kind === "danfo" ? "bestieVehicleDanfo" : companionBoatAssetKey(p);
      if (key) {
        const fallback = new THREE.Group(); fallback.name = `${p.id}:procedural-fallback`;
        [...g.children].forEach(child => fallback.add(child));
        const envelope = companionBoatBounds[companionBoatModelId(p)];
        // A saved model-specific slot admits its full authored envelope. Older
        // recipes retain their procedural reserve and may keep the fallback.
        const modelSlot = p.kind === "boat" && p.boatModel
          && p.size[0] >= envelope.max[0] - envelope.min[0] - .04
          && p.size[2] >= envelope.max[2] - envelope.min[2] - .04;
        const admitted = modelSlot
          ? new THREE.Box3(new THREE.Vector3(-envelope.max[0], envelope.min[1], -envelope.max[2]),
            new THREE.Vector3(-envelope.min[0], envelope.max[1], -envelope.min[2]))
          : new THREE.Box3(new THREE.Vector3(-w / 2, 0, -d / 2), new THREE.Vector3(w / 2, h, d / 2));
        g.add(fallback);
        if (p.kind === "boat") {
          const placement = new THREE.Matrix4().compose(g.position, g.quaternion, g.scale);
          const footprint = admitted.clone().applyMatrix4(placement);
          const dry = design.surfaces.some(surface => footprint.max.x > surface.minX - .08 && footprint.min.x < surface.maxX + .08
            && footprint.max.z > surface.minZ - .08 && footprint.min.z < surface.maxZ + .08);
          parent.updateWorldMatrix(true, false);
          if (dry || modelFits && !modelFits(footprint.clone().applyMatrix4(parent.matrixWorld))) return;
        }
        runtime.models.push(mountOptionalSceneryModel(g, fallback, key, delivery, admitted, changed,
          new THREE.Vector3(), Math.PI, p.kind === "boat" ? companionBoatModelId(p) : undefined));
      }
    }
  }
  for (const surface of design.surfaces) {
    const width = surface.maxX - surface.minX, depth = surface.maxZ - surface.minZ;
    if (surface.kind === "wood") {
      for (let z = surface.minZ + .16; z < surface.maxZ; z += .32)
        box(parent, m.timber, (surface.minX + surface.maxX) / 2, -.06, z, width, .12, Math.min(.3, surface.maxZ - z));
    } else box(parent, surface.kind === "sand" ? m.sand : surface.kind === "mud" ? m.mud : m.concrete,
      (surface.minX + surface.maxX) / 2, -.06, (surface.minZ + surface.maxZ) / 2, width, .12, depth);
  }
  design.props.forEach(prop);
}

export function updateWaterfrontGeometry(group: THREE.Group, seconds: number, reducedMotion: boolean, visible: boolean, power: number, daylight = 1) {
  const runtime = runtimes.get(group);
  if (!runtime || !visible) return false;
  const time = reducedMotion ? 0 : seconds;
  runtime.water?.update(time, daylight);
  for (const floater of runtime.floaters) {
    floater.root.position.y = floater.y + Math.sin(time * .65 + floater.phase) * .035;
    floater.root.rotation.z = Math.sin(time * .43 + floater.phase) * .012;
  }
  for (const plume of runtime.haze) {
    plume.root.position.y = plume.base + Math.sin(time * .16 + plume.phase) * .12;
    plume.root.rotation.y = Math.sin(time * .09 + plume.phase) * .08;
  }
  let changed = false;
  runtime.lamps.forEach(material => {
    const next = (.08 + (1 - daylight) * .3) * power;
    changed ||= material.emissiveIntensity !== next;
    material.emissiveIntensity = next;
  });
  return !reducedMotion || changed;
}

export function releaseWaterfrontGeometry(group: THREE.Group) {
  const runtime = runtimes.get(group); if (!runtime) return;
  runtime.models.forEach(model => model.dispose()); runtime.models = [];
  runtime.water?.dispose();
  runtime.unusedGeometries.forEach(geometry => geometry.dispose());
  runtime.unusedGeometries.clear(); runtimes.delete(group);
}
