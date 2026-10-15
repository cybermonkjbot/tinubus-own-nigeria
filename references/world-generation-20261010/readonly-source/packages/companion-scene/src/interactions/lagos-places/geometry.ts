import * as THREE from "three";
import { makeRoomObject } from "../room/studioGeometry";
import type { CompanionPlaceFixture, CompanionPlaceLayout } from "../../../../../packages/utils/src/companionPlaces";
import { createNightclubGeometry, updateNightclubLighting as updateClubLighting } from "./nightclubGeometry";
import { createLoungeGeometry, releaseLoungeLighting, updateLoungeLighting } from "./loungeGeometry";
import { createPoolCourtyardGeometry, releasePoolCourtyard, updatePoolCourtyard } from "./poolCourtyardGeometry";
import { createCivicInteriorGeometry, releaseCivicInterior, updateCivicInterior } from "./civicInteriorGeometry";
import { createWaterfrontGeometry, releaseWaterfrontGeometry, updateWaterfrontGeometry } from "./waterfrontGeometry";
import type { EnvironmentAsset } from "../scenes/environments";
import type { CompanionAssetDelivery } from "../../../../../packages/utils/src/companionAssets";

export function updateNightclubLighting(group: THREE.Group, elapsedSeconds: number, reducedMotion = false, visible = true, powerFactor = 1, daylight = 1) {
  const clubChanged = updateClubLighting(group, elapsedSeconds, reducedMotion, visible, powerFactor);
  const loungeChanged = updateLoungeLighting(group, visible, powerFactor);
  const courtyardChanged = updatePoolCourtyard(group, elapsedSeconds, reducedMotion, visible, powerFactor);
  const waterfrontChanged = updateWaterfrontGeometry(group, elapsedSeconds, reducedMotion, visible, powerFactor, daylight);
  return updateCivicInterior(group, visible, powerFactor) || waterfrontChanged || courtyardChanged || loungeChanged || clubChanged;
}

const generatedMaterials = new WeakMap<THREE.Group, THREE.Material[]>();
const generatedTextures = new WeakMap<THREE.Group, THREE.Texture[]>();

/** Generated fixtures use explicit metre-scale construction. Published room
 * parts are added separately, without changing their materials or scale. */
export function createLagosPlaceGeometry(layout: CompanionPlaceLayout, loungeAsset?: EnvironmentAsset,
  delivery?: CompanionAssetDelivery, changed: () => void = () => {}) {
  const group = new THREE.Group(); group.name = `${layout.id}:geometry`;
  const materials = {
    plaster: new THREE.MeshStandardMaterial({ color: layout.theme.wall, roughness: .88 }),
    wood: new THREE.MeshStandardMaterial({ color: "#92735b", roughness: .68 }),
    stone: new THREE.MeshStandardMaterial({ color: "#ded7ca", roughness: .46 }),
    metal: new THREE.MeshStandardMaterial({ color: "#657078", metalness: .5, roughness: .4 }),
    linen: new THREE.MeshStandardMaterial({ color: "#eeeae0", roughness: .98 }),
    sage: new THREE.MeshStandardMaterial({ color: "#819072", roughness: .9 }),
    rose: new THREE.MeshStandardMaterial({ color: layout.theme.accent, roughness: .85 }),
  };
  const place = (owner: THREE.Group, geometry: THREE.BufferGeometry, material: THREE.Material, x: number, y: number, z: number) => {
    const mesh = new THREE.Mesh(geometry, material); mesh.position.set(x, y, z); mesh.castShadow = true; mesh.receiveShadow = true; owner.add(mesh); return mesh;
  };
  const box = (owner: THREE.Group, material: THREE.Material, w: number, h: number, d: number, x: number, y: number, z: number) =>
    place(owner, new THREE.BoxGeometry(w, h, d), material, x, y, z);
  const bounds = layout.navigation.bounds, width = bounds.maxX - bounds.minX, depth = bounds.maxZ - bounds.minZ;
  const floor = new THREE.MeshStandardMaterial({ color: layout.theme.floor, roughness: layout.kind === "nightclub" ? .52 : .78 });
  const ownedMaterials: THREE.Material[] = [...Object.values(materials), floor];
  const ownedTextures: THREE.Texture[] = [];
  generatedMaterials.set(group, ownedMaterials);
  generatedTextures.set(group, ownedTextures);
  try {
    if (!layout.waterfront && layout.kind !== "pool-courtyard" && layout.kind !== "civic-interior" && layout.kind !== "lounge") box(group, floor, width, .1, depth, 0, -.05, 0);
    // Physical floor joints, bounded to at most 75 strips per axis.
    if (!layout.waterfront && layout.kind !== "soccer-pitch" && layout.kind !== "nightclub" && layout.kind !== "lounge" && layout.kind !== "pool-courtyard" && layout.kind !== "civic-interior") for (let x = bounds.minX + 1; x < bounds.maxX; x += 1.2) box(group, materials.plaster, .006, .002, depth, x, .001, 0);
    function fixture(value: CompanionPlaceFixture) {
      const root = new THREE.Group(); root.name = value.id; root.userData.lagosFixtureId = value.id;
      group.add(root);
      const [w, h, d] = value.size, m = materials[value.material];
      root.position.fromArray(value.point); root.rotation.y = value.yaw;
      switch (value.kind) {
        case "table": case "bench": case "counter": {
          box(root, m, w, .085, d, 0, h - .0425, 0);
          if (value.kind === "counter") box(root, materials.wood, w - .09, h - .09, d - .09, 0, (h - .09) / 2, 0);
          else for (const x of [-w / 2 + .1, w / 2 - .1]) for (const z of [-d / 2 + .1, d / 2 - .1]) box(root, materials.wood, .075, h - .085, .075, x, (h - .085) / 2, z);
          break;
        }
        case "treatment-bed": {
          box(root, materials.metal, w, .12, d, 0, .48, 0);
          box(root, materials.linen, w - .06, .16, d - .1, 0, .62, 0);
          box(root, materials.linen, w * .62, .12, .42, 0, .76, -d * .33);
          for (const sign of [-1, 1]) {
            box(root, materials.metal, w, .055, .055, 0, .92, sign * (d / 2 - .06));
            for (const x of [-w / 2 + .08, w / 2 - .08]) box(root, materials.metal, .055, .86, .055, x, .43, sign * (d / 2 - .06));
          }
          break;
        }
        case "planter": {
          const plant = makeRoomObject("plant"); plant.scale.setScalar(Math.min(w, d) / .65); root.add(plant); break;
        }
        case "canopy": {
          box(root, m, w, .08, d, 0, h, 0);
          // Uprights are outside the working counter footprint. They are still
          // supplied as collision fixtures by the resolved layout generator.
          break;
        }
        case "market-stall": {
          box(root, materials.wood, w, .09, d, 0, .87, 0);
          box(root, materials.wood, w, .78, .09, 0, .42, d / 2 - .08);
          box(root, m, w + .12, .09, d + .2, 0, h, 0);
          for (const x of [-w / 2 + .08, w / 2 - .08]) for (const z of [-d / 2 + .08, d / 2 - .08])
            box(root, materials.wood, .09, h, .09, x, h / 2, z);
          for (const x of [-w * .28, 0, w * .28]) {
            box(root, materials.wood, w * .23, .2, d * .65, x, 1.02, 0);
            box(root, m, w * .2, .06, d * .58, x, 1.15, 0);
          }
          break;
        }
        case "shelf": case "tool-cabinet": {
          const levels = value.kind === "shelf" ? 4 : 3;
          for (const x of [-w / 2 + .04, w / 2 - .04]) box(root, m, .08, h, d, x, h / 2, 0);
          box(root, m, w, h, .06, 0, h / 2, -d / 2 + .03);
          for (let level = 0; level <= levels; level++) {
            const y = .08 + level * (h - .16) / levels;
            box(root, m, w - .08, .055, d, 0, y, 0);
            if (level < levels) for (let item = 0; item < 5; item++) {
              const itemHeight = (h - .16) / levels * (.48 + (item % 3) * .1);
              const colour = item % 2 ? materials.sage : materials.linen;
              box(root, colour, w / 9, itemHeight, d * .42, (item - 2) * w / 6,
                y + .03 + itemHeight / 2, -.06);
            }
          }
          break;
        }
        case "fuel-pump": {
          box(root, materials.stone, w, .12, d, 0, .06, 0);
          box(root, m, w * .58, h * .73, d * .58, 0, h * .365 + .12, 0);
          box(root, materials.linen, w * .72, h * .24, d * .66, 0, h * .84, 0);
          box(root, materials.metal, w * .42, h * .09, .025, 0, h * .86, d * .34);
          for (const x of [-w * .4, w * .4]) {
            const hose = place(root, new THREE.TorusGeometry(.22, .035, 8, 20), materials.metal, x, .75, d * .16);
            hose.scale.y = 1.75;
            box(root, materials.metal, .07, .24, .09, x, 1.08, d * .19);
          }
          break;
        }
        case "tyre-rack": {
          for (const x of [-w / 2 + .05, w / 2 - .05]) box(root, m, .08, h, d, x, h / 2, 0);
          for (const y of [.15, h / 2, h - .12]) box(root, m, w, .07, d, 0, y, 0);
          for (const y of [.48, h * .7]) for (const x of [-w * .3, 0, w * .3])
            place(root, new THREE.TorusGeometry(.28, .11, 10, 24), materials.metal, x, y, 0);
          break;
        }
        case "salon-chair": case "cinema-seat": {
          const seatedY = h * .43;
          box(root, m, w * .82, .17, d * .7, 0, seatedY, .06);
          box(root, m, w * .82, h * .56, .18, 0, seatedY + h * .24, -d * .34);
          for (const x of [-w * .43, w * .43]) {
            box(root, m, .12, .12, d * .6, x, seatedY + .18, .02);
            box(root, materials.metal, .06, seatedY, .06, x, seatedY / 2, 0);
          }
          place(root, new THREE.CylinderGeometry(w * .28, w * .34, .06, 20), materials.metal, 0, .03, 0);
          box(root, materials.metal, .12, seatedY - .08, .12, 0, seatedY / 2, 0);
          if (value.kind === "salon-chair") box(root, materials.metal, w * .48, .055, .23, 0, .17, d * .38);
          break;
        }
        case "mirror": case "screen": {
          box(root, materials.wood, w, h, d, 0, h / 2, 0);
          // Static pale glass and blank cinema screen. No reflected render,
          // private media texture or video stream is created here.
          box(root, value.kind === "mirror" ? materials.stone : m, w - .12, h - .12, .025, 0, h / 2, d / 2 + .014);
          break;
        }
        case "exercise-bike": {
          box(root, m, w * .85, .09, d * .72, 0, .045, 0);
          for (const z of [-d * .3, d * .3]) box(root, m, w * .85, .065, .12, 0, .08, z);
          const wheel = place(root, new THREE.CylinderGeometry(.3, .3, .18, 24), m, 0, .44, -d * .15);
          wheel.rotation.z = Math.PI / 2;
          box(root, m, .09, .8, .09, 0, .6, d * .19);
          box(root, materials.linen, .37, .08, .3, 0, 1.03, d * .19);
          box(root, m, .08, h * .79, .08, 0, h * .395, -d * .32);
          box(root, m, w * .65, .065, .08, 0, h * .8, -d * .32);
          for (const x of [-.21, .21]) box(root, m, .18, .04, .12, x, .35, 0);
          break;
        }
        case "goal": {
          for (const x of [-w / 2, w / 2]) box(root, m, .12, h, .12, x, h / 2, 0);
          box(root, m, w, .12, .12, 0, h, 0);
          for (let x = -w / 2; x <= w / 2; x += .35) box(root, m, .012, h, .012, x, h / 2, -d / 2);
          for (let y = 0; y <= h; y += .35) box(root, m, w, .012, .012, 0, y, -d / 2);
          break;
        }
        case "cell-bars": {
          for (let z = -d / 2; z <= d / 2; z += .18) box(root, m, w, h, .025, 0, h / 2, z);
          for (const y of [.08, h - .08]) box(root, m, w, .07, d, 0, y, 0);
          break;
        }
        case "skyline": {
          for (let i = 0; i < 12; i++) {
            const height = h * (.4 + (i * 7 % 11) / 18);
            box(root, m, 1.1, height, d, (i - 5.5) * w / 12, height / 2 - 3, 0);
          }
          break;
        }
        default: box(root, m, w, h, d, 0, h / 2, 0);
      }
    }
    if (layout.waterfront) createWaterfrontGeometry(layout, group, ownedMaterials, ownedTextures, true, delivery, changed);
    else if (layout.kind === "nightclub") createNightclubGeometry(layout, group, ownedMaterials);
    else if (layout.kind === "lounge") {
      if (!loungeAsset) throw new Error("The lounge interior must download before entry.");
      createLoungeGeometry(layout, group, loungeAsset);
    }
    else if (layout.kind === "pool-courtyard") createPoolCourtyardGeometry(layout, group, ownedMaterials, ownedTextures);
    else if (layout.kind === "civic-interior") createCivicInteriorGeometry(layout, group, ownedMaterials);
    else layout.fixtures.forEach(fixture);
    if (layout.kind === "soccer-pitch") {
      const white = new THREE.MeshStandardMaterial({ color: "#f2efe4", roughness: .95 });
      ownedMaterials.push(white);
      for (const x of [-14, 14]) box(group, white, .08, .004, 44, x, .005, 0);
      for (const z of [-22, 0, 22]) box(group, white, 28, .004, .08, 0, .005, z);
      const circle = new THREE.Mesh(new THREE.RingGeometry(4.46, 4.54, 64), white); circle.rotation.x = -Math.PI / 2; circle.position.y = .006; group.add(circle);
      for (const sign of [-1, 1]) {
        box(group, white, 14, .004, .08, 0, .005, sign * 15);
        for (const x of [-7, 7]) box(group, white, .08, .004, 7, x, .005, sign * 18.5);
      }
    }
    return group;
  } catch (error) { disposeLagosPlaceGeometry(group); group.clear(); throw error; }
}

/** Generated resources only. Published/shared asset geometry is cache-owned. */
export function disposeLagosPlaceGeometry(group: THREE.Group) {
  // Detach cached clones before collecting generated geometry for disposal.
  releaseWaterfrontGeometry(group);
  releaseLoungeLighting(group);
  const geometries = new Set<THREE.BufferGeometry>(), materials = new Set<THREE.Material>(), textures = new Set<THREE.Texture>();
  group.traverse(node => {
    if (node instanceof THREE.Group) {
      generatedMaterials.get(node)?.forEach(material => materials.add(material)); generatedMaterials.delete(node);
      generatedTextures.get(node)?.forEach(texture => textures.add(texture)); generatedTextures.delete(node); releaseLoungeLighting(node);
      releasePoolCourtyard(node);
      releaseCivicInterior(node);
      releaseWaterfrontGeometry(node);
    }
    if (node instanceof THREE.InstancedMesh) node.dispose();
    if (node instanceof THREE.Mesh) { geometries.add(node.geometry); (Array.isArray(node.material) ? node.material : [node.material]).forEach(m => materials.add(m)); }
  });
  geometries.forEach(g => g.dispose()); materials.forEach(m => m.dispose()); textures.forEach(texture => texture.dispose());
}
