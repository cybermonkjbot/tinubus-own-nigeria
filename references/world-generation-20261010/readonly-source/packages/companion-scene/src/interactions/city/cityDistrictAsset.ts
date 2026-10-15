import * as THREE from "three";
import type { CompanionAssetDelivery, CompanionAssetKey } from "../../../../../packages/utils/src/companionAssets";
import { acquireCompanionGltf, instantiateGltf, type GltfLease } from "../appearance/gltfAssets";

/** One nearby landmark lease, using the existing integrity-checked public asset cache. */
export function createCityDistrictAsset(key: CompanionAssetKey, position: readonly number[],
  delivery: () => CompanionAssetDelivery, changed: () => void, fallback?: THREE.Group,
  distance = 850) {
  const group = new THREE.Group(); group.name = key;
  const assetRoot = new THREE.Group(); assetRoot.position.fromArray(position); group.add(assetRoot);
  assetRoot.updateMatrix(); assetRoot.matrixAutoUpdate = false;
  if (fallback) assetRoot.add(fallback);
  let provider: CompanionAssetDelivery | undefined, lease: GltfLease | undefined;
  let instance: ReturnType<typeof instantiateGltf> | undefined, disposed = false, near = false, failed = false, dirty = false;
  let meshes:THREE.Mesh[]=[],shadows:boolean|undefined;
  const release = () => {
    instance?.dispose(); instance = undefined; lease?.release(); lease = undefined;
    meshes=[];shadows=undefined;
    if (fallback) fallback.visible = true;
  };
  return {
    group,
    update(target: THREE.Vector3, daylight: number) {
      if (disposed) return false;
      const next = delivery(), within = Math.hypot(target.x - position[0], target.z - position[2]) < distance;
      if (provider !== next) { release(); failed = false; provider = next; dirty = true; }
      if (within && !near) failed = false;
      near = within;
      if (!within && lease) { release(); dirty = true; }
      if (within && !lease && !failed && next.available(key)) {
        const request = acquireCompanionGltf(key, next); lease = request;
        request.promise.then(asset => {
          if (disposed || lease !== request) return;
          const candidate = instantiateGltf(asset);
          const rendered:THREE.Mesh[]=[];
          let stationary = asset.animations.length === 0;
          candidate.scene.traverse(node => {
            if (node instanceof THREE.SkinnedMesh) stationary = false;
            if (node instanceof THREE.Mesh) { rendered.push(node); node.castShadow = true; node.receiveShadow = true; }
          });
          if (!rendered.length) { candidate.dispose(); throw new Error("The district model is empty."); }
          meshes=rendered;shadows=undefined;
          instance = candidate; assetRoot.add(candidate.scene);
          // Landmark clones have fixed local poses. Animated/skinned assets keep
          // their existing updates, and parent/world transforms stay automatic.
          if (stationary) candidate.scene.traverse(node => {
            if (node.matrixAutoUpdate) { node.updateMatrix(); node.matrixAutoUpdate = false; }
          });
          if (fallback) fallback.visible = false;
          dirty = true; changed();
        }).catch(() => {
          if (disposed || lease !== request) return;
          release(); failed = true; dirty = true; changed();
        });
      }
      const castShadow=within&&daylight>.12&&Math.hypot(target.x-position[0],target.z-position[2])<300;
      if(instance&&castShadow!==shadows){meshes.forEach(mesh=>{mesh.castShadow=castShadow;});shadows=castShadow;dirty=true;}
      const result = dirty; dirty = false; return result;
    },
    get loaded() { return !!instance; },
    dispose() { if (disposed) return; disposed = true; release(); group.removeFromParent(); group.clear(); },
  };
}
