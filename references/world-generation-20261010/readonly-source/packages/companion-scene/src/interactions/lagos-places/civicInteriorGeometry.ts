import * as THREE from "three";
import { RoundedBoxGeometry } from "three/addons/geometries/RoundedBoxGeometry.js";
import design from "../../../../assets/venues/civic-interiors-v1.json";
import type { CompanionPlaceLayout } from "../../../../../packages/utils/src/companionPlaces";

type Shape = "box" | "pipe" | "pot" | "leaf" | "cushion";
type Part = typeof design.floors[number]["parts"][number];
type MaterialSpec = { colour: string; roughness: number; metalness: number; emission?: number };
type Practical = { material: THREE.MeshStandardMaterial; intensity: number };
const states = new WeakMap<THREE.Group, { factor: number; practicals: Practical[] }>();

/** The selected floor uses the same authored positions as its navigation.
 * No second floor, generic fixture geometry or floor beneath the upper terrace. */
export function createCivicInteriorGeometry(layout: CompanionPlaceLayout, parent: THREE.Group, ownedMaterials: THREE.Material[]) {
  const floor = design.floors.find(value => value.id === layout.placeId);
  if (layout.kind !== "civic-interior" || layout.assetVersion !== design.id || !floor)
    throw new Error("Use the matching Civic Street interior layout.");
  const root = new THREE.Group(); root.name = `${layout.id}:civic-interior`; parent.add(root);
  root.position.y = -floor.floorY;
  const materials = new Map<string, THREE.MeshStandardMaterial>(), practicals: Practical[] = [];
  for (const [key, spec] of Object.entries(design.materials)) {
    const value: MaterialSpec = spec;
    const material = new THREE.MeshStandardMaterial({ color: value.colour, roughness: value.roughness, metalness: value.metalness,
      emissive: value.emission ? value.colour : "#000000", emissiveIntensity: 0 });
    material.name = `Civic interior ${key}`;
    materials.set(key, material); ownedMaterials.push(material);
    if (value.emission) practicals.push({ material, intensity: value.emission });
  }
  const shapes: Record<Shape, THREE.BufferGeometry> = {
    box: new THREE.BoxGeometry(1, 1, 1), pipe: new THREE.CylinderGeometry(.5, .5, 1, 12),
    pot: new THREE.CylinderGeometry(.5, .36, 1, 16), leaf: new THREE.IcosahedronGeometry(.5, 1),
    cushion: new RoundedBoxGeometry(1, 1, 1, 1, .06),
  };
  const attached = new Set<Shape>();
  try {
    const bins = new Map<string, Part[]>();
    for (const part of floor.parts) {
      const key = `${part.shape}:${part.material}`;
      const values = bins.get(key) ?? []; values.push(part); bins.set(key, values);
    }
    const matrix = new THREE.Matrix4(), position = new THREE.Vector3(), scale = new THREE.Vector3();
    const rotation = new THREE.Euler(), quaternion = new THREE.Quaternion();
    for (const parts of bins.values()) {
      const first = parts[0], shape = first.shape as Shape;
      if (!shapes[shape] || !materials.has(first.material)) throw new Error("Unknown Civic Street shape or finish.");
      const mesh = new THREE.InstancedMesh(shapes[shape], materials.get(first.material)!, parts.length);
      mesh.name = `Civic interior ${first.shape} ${first.material}`; root.add(mesh); attached.add(shape);
      for (const [index, part] of parts.entries()) {
        rotation.set(part.rotation[0], part.rotation[1], part.rotation[2], "XYZ"); quaternion.setFromEuler(rotation);
        matrix.compose(position.fromArray(part.position), quaternion, scale.fromArray(part.size)); mesh.setMatrixAt(index, matrix);
      }
      mesh.instanceMatrix.needsUpdate = true; mesh.computeBoundingSphere();
      mesh.castShadow = first.material !== "glass" && first.material !== "light";
      mesh.receiveShadow = first.material !== "glass" && first.material !== "light";
    }
    states.set(parent, { factor: 0, practicals });
  } finally {
    for (const [shape, geometry] of Object.entries(shapes)) if (!attached.has(shape as Shape)) geometry.dispose();
  }
}

/** Practical emission follows the existing place power and render scheduler. */
export function updateCivicInterior(group: THREE.Group, visible: boolean, powerFactor: number) {
  const state = states.get(group);
  if (!state || !visible) return false;
  const factor = Number.isFinite(powerFactor) ? THREE.MathUtils.clamp(powerFactor, 0, 1) : 0;
  if (factor === state.factor) return false;
  state.factor = factor;
  for (const practical of state.practicals) practical.material.emissiveIntensity = practical.intensity * factor;
  return true;
}

export function releaseCivicInterior(group: THREE.Group) { states.delete(group); }
