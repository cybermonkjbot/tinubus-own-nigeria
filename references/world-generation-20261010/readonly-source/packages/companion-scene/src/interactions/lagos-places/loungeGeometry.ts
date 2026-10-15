import * as THREE from "three";
import { clone } from "three/addons/utils/SkeletonUtils.js";
import type { CompanionPlaceLayout } from "../../../../../packages/utils/src/companionPlaces";
import type { EnvironmentAsset } from "../scenes/environments";

type PowerState = {
  factor: number; group: THREE.Group;
  materials: { material: THREE.MeshStandardMaterial; intensity: number }[];
  lights: { light: THREE.PointLight; intensity: number }[];
  dispose(): void;
};
const powerStates = new WeakMap<THREE.Group, PowerState>();

/** The cache owns GLB geometry/textures. This instance owns only cloned materials and lights. */
export function createLoungeGeometry(layout: CompanionPlaceLayout, parent: THREE.Group, asset: EnvironmentAsset) {
  if (layout.kind !== "lounge" || layout.placeId !== "dance-again") throw new Error("Use the Dance Again lounge layout.");
  const group = clone(asset.scene);
  if (!(group instanceof THREE.Group)) throw new Error("The lounge asset has no scene group.");
  group.name = `${layout.id}:lounge`;
  const copies = new Map<THREE.Material, THREE.Material>();
  const powered = new Map<THREE.MeshStandardMaterial, number>();
  try {
    let meshes = 0;
    group.traverse(node => {
      if (!(node instanceof THREE.Mesh)) return;
      if (node.userData.baddie_architecture_asset !== "bestieDanceAgainInterior") throw new Error("Unexpected lounge geometry.");
      meshes++;
      const copy = (source: THREE.Material) => {
        let value = copies.get(source);
        if (!value) { value = source.clone(); copies.set(source, value); }
        if (value instanceof THREE.MeshStandardMaterial) {
          const baseline = value.userData.baddie_emission_baseline;
          if (typeof baseline === "number" && Number.isFinite(baseline) && baseline > 0) powered.set(value, baseline);
        }
        return value;
      };
      node.material = Array.isArray(node.material) ? node.material.map(copy) : copy(node.material);
      node.castShadow = node.userData.baddie_casts_shadow === true;
      node.receiveShadow = node.castShadow;
    });
    if (!meshes) throw new Error("Empty lounge geometry.");
    const lights = [
      [-3.7, -2, "#ff559e", 6, 9], [3.7, 2, "#ff559e", 6, 9],
      [0, -6.6, "#ff559e", 8, 8], [0, 4.5, "#f2ac74", 4, 8],
    ] as const;
    const state: PowerState = { factor: 1, group,
      materials: [...powered].map(([material, intensity]) => ({ material, intensity })),
      lights: lights.map(([x, z, colour, intensity, distance]) => {
        const light = new THREE.PointLight(colour, intensity, distance, 2);
        light.position.set(x, 2.5, z); light.castShadow = false; group.add(light);
        return { light, intensity };
      }),
      dispose() { group.removeFromParent(); copies.forEach(material => material.dispose()); group.traverse(node => { if (node instanceof THREE.PointLight) node.dispose(); }); },
    };
    powerStates.set(parent, state); parent.add(group); return group;
  } catch (error) { group.removeFromParent(); copies.forEach(material => material.dispose()); throw error; }
}

/** Steady mains lighting retains the existing venue power factor. */
export function updateLoungeLighting(parent: THREE.Group, visible: boolean, powerFactor: number) {
  const state = powerStates.get(parent);
  if (!state || !visible || !Number.isFinite(powerFactor)) return false;
  const factor = THREE.MathUtils.clamp(powerFactor, 0, 1);
  if (factor === state.factor) return false;
  state.factor = factor;
  state.materials.forEach(channel => { channel.material.emissiveIntensity = channel.intensity * factor; });
  state.lights.forEach(channel => { channel.light.intensity = channel.intensity * factor; });
  return true;
}

/** Detach the cached geometry before generated-resource traversal. */
export function releaseLoungeLighting(parent: THREE.Group) {
  powerStates.get(parent)?.dispose(); powerStates.delete(parent);
}
