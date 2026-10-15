import * as THREE from "three";
import { RoundedBoxGeometry } from "three/addons/geometries/RoundedBoxGeometry.js";
import design from "../../../../assets/venues/pool-courtyard-v1.json";
import type { CompanionPlaceLayout } from "../../../../../packages/utils/src/companionPlaces";

type Shape = "box" | "pipe" | "pot" | "leaf" | "cushion";
type Part = typeof design.parts[number];
type MaterialSpec = { colour: string; roughness: number; metalness: number; emission?: number };
type CourtyardState = {
  power: number; time: number; waterNormal: THREE.DataTexture;
  practicals: { material: THREE.MeshStandardMaterial; intensity: number }[];
};
const states = new WeakMap<THREE.Group, CourtyardState>();

/** A seamless tangent-space ripple tile. No photo textures or extra render pass. */
function waterNormalTexture() {
  const size = 128, data = new Uint8Array(size * size * 4);
  for (let y = 0; y < size; y++) for (let x = 0; x < size; x++) {
    const u = x / size * Math.PI * 2, v = y / size * Math.PI * 2;
    const nx = .36 * Math.cos(u * 3 + v * 2) + .16 * Math.cos(u * 5 - v * 3);
    const ny = .24 * Math.cos(u * 3 + v * 2) - .096 * Math.cos(u * 5 - v * 3);
    const length = Math.hypot(nx, ny, 1), i = (y * size + x) * 4;
    data[i] = Math.round((nx / length * .5 + .5) * 255);
    data[i + 1] = Math.round((ny / length * .5 + .5) * 255);
    data[i + 2] = Math.round((1 / length * .5 + .5) * 255); data[i + 3] = 255;
  }
  const texture = new THREE.DataTexture(data, size, size);
  texture.name = "Pool courtyard ripple normals";
  texture.wrapS = texture.wrapT = THREE.RepeatWrapping;
  texture.repeat.set(design.pool.width * .65, design.pool.depth * .65);
  texture.minFilter = THREE.LinearMipmapLinearFilter; texture.magFilter = THREE.LinearFilter;
  texture.generateMipmaps = true; texture.needsUpdate = true;
  return texture;
}

/** Shared native/web scenery, instanced by shape and finish. The place owner
 * retains navigation, lighting, audio, power and the existing render loop. */
export function createPoolCourtyardGeometry(layout: CompanionPlaceLayout, parent: THREE.Group,
  ownedMaterials: THREE.Material[], ownedTextures: THREE.Texture[]) {
  if (layout.kind !== "pool-courtyard" || layout.assetVersion !== design.id) throw new Error("Use the matching authored courtyard layout.");
  const root = new THREE.Group(); root.name = `${layout.id}:pool-courtyard`; parent.add(root);
  const materials = new Map<string, THREE.MeshStandardMaterial>();
  const practicals: CourtyardState["practicals"] = [];
  for (const [key, spec] of Object.entries(design.materials)) {
    const value: MaterialSpec = spec;
    const material = new THREE.MeshStandardMaterial({ color: value.colour, roughness: value.roughness, metalness: value.metalness,
      emissive: value.emission ? value.colour : "#000000", emissiveIntensity: 0 });
    material.name = `Pool courtyard ${key}`;
    materials.set(key, material); ownedMaterials.push(material);
    if (value.emission) practicals.push({ material, intensity: value.emission });
  }
  const shapes: Record<Shape, THREE.BufferGeometry> = {
    box: new THREE.BoxGeometry(1, 1, 1),
    pipe: new THREE.CylinderGeometry(.5, .5, 1, 10),
    pot: new THREE.CylinderGeometry(.5, .36, 1, 16),
    leaf: new THREE.IcosahedronGeometry(.5, 1),
    cushion: new RoundedBoxGeometry(1, 1, 1, 1, .075),
  };
  const attachedShapes = new Set<Shape>();
  try {
    const bins = new Map<string, Part[]>();
    for (const part of design.parts) {
      if (part.shape === "water") continue;
      const key = `${part.shape}:${part.material}`;
      const values = bins.get(key) ?? []; values.push(part); bins.set(key, values);
    }
    const matrix = new THREE.Matrix4(), position = new THREE.Vector3(), scale = new THREE.Vector3();
    const rotation = new THREE.Euler(), quaternion = new THREE.Quaternion();
    for (const parts of bins.values()) {
      const first = parts[0], shape = first.shape as Shape;
      if (!shapes[shape] || !materials.has(first.material)) throw new Error("Unknown courtyard geometry finish or shape.");
      const mesh = new THREE.InstancedMesh(shapes[shape], materials.get(first.material)!, parts.length);
      mesh.name = `Pool courtyard ${first.shape} ${first.material}`;
      root.add(mesh); attachedShapes.add(shape);
      for (const [index, part] of parts.entries()) {
        rotation.set(part.rotation[0], part.rotation[1], part.rotation[2], "XYZ"); quaternion.setFromEuler(rotation);
        matrix.compose(position.fromArray(part.position), quaternion, scale.fromArray(part.size));
        mesh.setMatrixAt(index, matrix);
      }
      mesh.instanceMatrix.needsUpdate = true; mesh.computeBoundingSphere();
      mesh.castShadow = first.material !== "light" && first.material !== "glass";
      mesh.receiveShadow = first.material !== "glass" && first.material !== "light";
    }
    const normal = waterNormalTexture(); ownedTextures.push(normal);
    const waterMaterial = materials.get("water")!;
    waterMaterial.normalMap = normal; waterMaterial.normalScale.set(.22, .22);
    waterMaterial.transparent = true; waterMaterial.opacity = .9; waterMaterial.depthWrite = false;
    const waterGeometry = new THREE.PlaneGeometry(design.pool.width, design.pool.depth);
    waterGeometry.rotateX(-Math.PI / 2);
    const water = new THREE.Mesh(waterGeometry, waterMaterial);
    water.name = "Pool courtyard water";
    water.position.set(design.pool.x, design.pool.waterY, design.pool.z);
    water.receiveShadow = true; root.add(water);
    states.set(parent, { power: 0, time: 0, waterNormal: normal, practicals });
  } finally {
    // Attached resources are released by the generated-place owner, including
    // its exception path. Unused templates have no mesh owner to traverse.
    for (const [shape, geometry] of Object.entries(shapes)) if (!attachedShapes.has(shape as Shape)) geometry.dispose();
  }
}

export function updatePoolCourtyard(group: THREE.Group, elapsedSeconds: number, reducedMotion: boolean, visible: boolean, powerFactor: number) {
  const state = states.get(group);
  if (!state || !visible) return false;
  const power = Number.isFinite(powerFactor) ? THREE.MathUtils.clamp(powerFactor, 0, 1) : 0;
  let changed = power !== state.power;
  if (changed) {
    state.power = power;
    for (const practical of state.practicals) practical.material.emissiveIntensity = practical.intensity * power;
  }
  // Follow the shared clock without another RAF/timer. Reduced motion keeps
  // the same still water tile and lets the settled renderer stop drawing.
  const time = reducedMotion || !Number.isFinite(elapsedSeconds) ? 0 : Math.max(0, elapsedSeconds);
  if (time !== state.time) {
    state.time = time;
    state.waterNormal.offset.set(time * .012 % 1, time * .006 % 1); changed = true;
  }
  return changed;
}

export function releasePoolCourtyard(group: THREE.Group) { states.delete(group); }
