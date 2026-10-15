import * as THREE from "three";
import { createSceneAssetCache } from "../../../../../packages/character-runtime/src/sceneAsset";
import type { CompanionAssetDelivery, CompanionAssetKey } from "../../../../../packages/utils/src/companionAssets";
import { acquireCompanionGltf, instantiateGltf, type GltfLease } from "./gltfAssets";

type PreparedScenery = {
  x: number; y: number; z: number; yaw: number; modelId?: string;
  bounds: THREE.Box3; matches: boolean; meshes: number;
};
type SceneryMount = {
  parent: THREE.Group; fallback: THREE.Group; admitted: THREE.Box3; changed: () => void;
  offset: THREE.Vector3; yaw: number; modelId?: string;
};
// One placement record per immutable source, without retaining its clone or resources.
const preparedSources = createSceneAssetCache<PreparedScenery>();

/** Cosmetic replacement only. The frozen placement and navigation remain authoritative. */
export function mountOptionalSceneryModel(parent: THREE.Group, fallback: THREE.Group, key: CompanionAssetKey,
  delivery: CompanionAssetDelivery, admitted: THREE.Box3, changed: () => void = () => {},
  offset: THREE.Vector3 = new THREE.Vector3(), yaw = 0,
  modelId?: string) {
  const lease = delivery.available(key) ? acquireCompanionGltf(key, delivery) : undefined;
  return createOptionalSceneryMount({ parent, fallback, admitted, changed, offset, yaw, modelId }, lease);
}

// Pending source callbacks retain only this clearable owner, not the public
// factory's scene, fallback and notification arguments after disposal.
function createOptionalSceneryMount(mount: SceneryMount | undefined, lease: GltfLease | undefined) {
  let detachedFallback = false;
  let instance: ReturnType<typeof instantiateGltf> | undefined;
  function restoreFallback(current: SceneryMount) {
    current.fallback.visible = true;
    if (detachedFallback) { current.parent.add(current.fallback); detachedFallback = false; }
  }
  const releaseLease = () => {
    // A retained mount must not keep a released lease's fulfilled asset promise.
    const current = lease; lease = undefined; current?.release();
  };
  lease?.promise.then(asset => {
    if (!mount) return;
    const { parent, fallback, admitted, changed, offset, yaw, modelId } = mount;
    const create = () => {
      const candidate = instantiateGltf(asset); instance = candidate;
      candidate.scene.position.copy(offset); candidate.scene.rotation.y = yaw;
      return candidate;
    };
    let prepared = preparedSources.get(asset.scene);
    if (!prepared || !Object.is(prepared.x, offset.x) || !Object.is(prepared.y, offset.y)
      || !Object.is(prepared.z, offset.z) || !Object.is(prepared.yaw, yaw) || prepared.modelId !== modelId) {
      const candidate = create();
      const bounds = new THREE.Box3().setFromObject(candidate.scene, true);
      let matches = true, meshes = 0;
      candidate.scene.traverse(node => {
        if (!(node instanceof THREE.Mesh)) return;
        meshes++;
        if (modelId && node.userData.baddie_boat_id !== modelId) matches = false;
      });
      prepared = { x: offset.x, y: offset.y, z: offset.z, yaw, modelId, bounds, matches, meshes };
      preparedSources.set(asset.scene, prepared);
    }
    const { bounds, matches, meshes } = prepared;
    // Preserve metre scale and the ground/waterline origin. Do not squeeze larger art into an old slot.
    const fits = !bounds.isEmpty() && [bounds.min.x, bounds.max.x, bounds.min.y, bounds.max.y, bounds.min.z, bounds.max.z].every(Number.isFinite)
      && bounds.min.x >= admitted.min.x - .04 && bounds.max.x <= admitted.max.x + .04
      && bounds.min.y >= admitted.min.y - .04 && bounds.max.y <= admitted.max.y + .04
      && bounds.min.z >= admitted.min.z - .04 && bounds.max.z <= admitted.max.z + .04;
    if (!fits || !matches || !meshes) { instance?.dispose(); instance = undefined; releaseLease(); return; }
    const candidate = instance ?? create();
    // Bounds preparation already updated these matrices on the first mount.
    // A cache hit needs the same pre-mount transforms, without a vertex scan.
    candidate.scene.updateWorldMatrix(false, true);
    candidate.scene.traverse(node => {
      if (node instanceof THREE.Mesh) {
        node.castShadow = modelId ? node.userData.baddie_casts_shadow === true : true; node.receiveShadow = true;
      }
    });
    parent.add(candidate.scene); fallback.visible = false;
    // Hidden children still incur scene-matrix traversal. Keep this graph out
    // until disposal restores it for the caller's generated-resource cleanup.
    detachedFallback = true; fallback.removeFromParent();
    try { changed(); } catch { /* Host invalidation does not own the mounted model. */ }
  }).catch(() => {
    // Disposal already restored the fallback for its owner's resource sweep.
    // A late rejection must not touch that released graph again.
    if (!mount) return;
    instance?.dispose(); instance = undefined; restoreFallback(mount); releaseLease();
  });
  return {
    dispose() {
      const current = mount;
      if (!current) return;
      mount = undefined;
      instance?.dispose(); instance = undefined;
      releaseLease(); restoreFallback(current);
    },
  };
}
