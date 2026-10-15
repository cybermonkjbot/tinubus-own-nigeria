import * as THREE from "three";
import type { GLTF } from "three/addons/loaders/GLTFLoader.js";

export type SceneAsset = Omit<GLTF, "parser">;

const preparationCaches = new Set<WeakMap<THREE.Group, unknown>>();

/** Create once per module for immutable source preparation. Last-lease disposal
 * evicts the source even when a released promise still retains its scene root. */
export function createSceneAssetCache<Value>(): WeakMap<THREE.Group, Value> {
  const cache = new WeakMap<THREE.Group, Value>();
  preparationCaches.add(cache);
  return cache;
}

/** Keep render resources, not the parser's binary body and dependency caches. */
export function retainSceneAsset(asset: GLTF): SceneAsset {
  const { scene, scenes, animations, cameras, asset: metadata, userData } = asset;
  return { scene, scenes, animations, cameras, asset: metadata, userData };
}

/** Only after graph consumers stop and owned resources are disposed. A retained
 * bone or helper must not reach sibling meshes through its old parent chain. */
export function clearDisposedSceneGraph(root: THREE.Object3D) {
  const parents: THREE.Object3D[] = [];
  // Snapshot parents before detaching anything; traversal needs the intact tree.
  root.traverse(node => { if (node.children.length) parents.push(node); });
  for (const parent of parents) parent.clear();
}

/** Call after the last asset lease, never for a clone sharing its resources.
 * The released asset cannot be used again. */
export function disposeSceneAsset(asset: SceneAsset) {
  const geometries = new Set<THREE.BufferGeometry>();
  const materials = new Set<THREE.Material>();
  const textures = new Set<THREE.Texture>();
  const skeletons = new Set<THREE.Skeleton>();
  const instances = new Set<THREE.InstancedMesh>();
  // GLTFLoader prepares every scene, including scenes other than the default.
  // Those resources need the same last-owner cleanup as the displayed scene.
  const scenes = new Set([asset.scene, ...asset.scenes]);
  for (const scene of scenes) for (const cache of preparationCaches) cache.delete(scene);
  for (const scene of scenes) scene.traverse(node => {
    if (!(node instanceof THREE.Mesh)) return;
    geometries.add(node.geometry);
    if (node instanceof THREE.SkinnedMesh) skeletons.add(node.skeleton);
    if (node instanceof THREE.InstancedMesh) instances.add(node);
    for (const material of Array.isArray(node.material) ? node.material : [node.material]) {
      if (materials.has(material)) continue;
      materials.add(material);
      for (const value of Object.values(material)) if (value instanceof THREE.Texture) textures.add(value);
    }
  });
  instances.forEach(value => value.dispose());
  geometries.forEach(value => value.dispose());
  materials.forEach(value => value.dispose());
  textures.forEach(value => value.dispose());
  skeletons.forEach(value => value.dispose());
  const images = new Set(Array.from(textures, texture => texture.image));
  images.forEach(image => { if (typeof ImageBitmap !== "undefined" && image instanceof ImageBitmap) image.close(); });
  // Fulfilled lease promises can retain this result after GPU disposal. Drop
  // its graph and collection ownership without mutating shared clip objects.
  scenes.forEach(clearDisposedSceneGraph);
  asset.scenes = [];
  asset.animations = [];
  asset.cameras = [];
}
