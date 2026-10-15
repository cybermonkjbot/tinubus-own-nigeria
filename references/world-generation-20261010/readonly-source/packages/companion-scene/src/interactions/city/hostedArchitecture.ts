import * as THREE from "three";
import type { CompanionAssetDelivery, CompanionAssetKey } from "../../../../../packages/utils/src/companionAssets";
import { acquireCompanionGltf, instantiateGltf, type GltfLease } from "../appearance/gltfAssets";

export type ArchitecturePlacement = {
  assetKey: CompanionAssetKey; parcelId: string; label: string;
  position: readonly number[]; yaw: number;
  focusPosition?: readonly number[];
};
type Thresholds = { enter: number; exit: number; shadowEnter: number; shadowExit: number; retain: number };
type InstanceBinding = { mounted(placement: ArchitecturePlacement, root: THREE.Group, meshes: readonly THREE.Mesh[]): (() => void) | undefined };

/** Nearby individual downloads share the existing cache, with at most two transfers per collection. */
export function createHostedArchitecture(placements: readonly ArchitecturePlacement[],
  fallback: (placement: ArchitecturePlacement) => THREE.Group, mains: Readonly<{ value: number }>,
  delivery: () => CompanionAssetDelivery, changed: () => void, thresholds: Thresholds,
  emission: (daylight: number) => number = daylight => 1 - daylight, binding?: InstanceBinding) {
  const group = new THREE.Group(); group.name = "Individually hosted architecture";
  const picking = new WeakMap<THREE.Object3D, string>();
  // These local poses stay fixed. Keep world-matrix updates enabled so parent
  // placement, reattachment, raycasting and sound queries still propagate.
  const cacheLocalTransform = (node: THREE.Object3D) => {
    if (!node.matrixAutoUpdate) return;
    node.updateMatrix(); node.matrixAutoUpdate = false;
  };
  const createProxy = (placement: ArchitecturePlacement) => {
    const proxy = fallback(placement);
    proxy.traverse(node => { cacheLocalTransform(node); if (node instanceof THREE.Mesh) picking.set(node, placement.parcelId); });
    return proxy;
  };
  const entries = placements.map(placement => {
    const root = new THREE.Group(), proxy = createProxy(placement);
    root.name = placement.label; root.position.fromArray(placement.position); root.rotation.y = placement.yaw;
    cacheLocalTransform(root);
    root.add(proxy); group.add(root);
    return { placement, root, proxy: proxy as THREE.Group | undefined, near: false, shadows: false, failed: false, available: false,
      distance: Infinity, focusX: NaN, focusZ: NaN, instanceNeedsSync: false, power: NaN,
      lease: undefined as GltfLease | undefined,
      instance: undefined as ReturnType<typeof instantiateGltf> | undefined,
      meshes: [] as THREE.Mesh[], powered: [] as { material: THREE.MeshStandardMaterial; baseline: number }[],
      releaseBinding: undefined as (() => void) | undefined };
  });
  let provider: CompanionAssetDelivery | undefined, disposed = false, dirty = false, targetX = NaN, targetZ = NaN;
  const releaseProxy = (entry: typeof entries[number]) => {
    const proxy = entry.proxy;
    if (!proxy) return;
    entry.proxy = undefined; proxy.removeFromParent();
    proxy.traverse(node => {
      if (node instanceof THREE.Mesh) picking.delete(node);
      // Only the instance buffers belong here. Factories own the shared shapes
      // and materials needed to rebuild the same fallback on model release.
      if (node instanceof THREE.InstancedMesh) node.dispose();
    });
  };
  const release = (entry: typeof entries[number]) => {
    // Prepare before retiring a live model so a failed allocation leaves that
    // model intact. Final disposal must not construct replacement fallbacks.
    const proxy = !disposed && !entry.proxy ? createProxy(entry.placement) : undefined;
    entry.releaseBinding?.(); entry.releaseBinding = undefined;
    entry.meshes.forEach(mesh => picking.delete(mesh));
    entry.instance?.dispose(); entry.instance = undefined; entry.meshes = []; entry.powered = [];
    entry.instanceNeedsSync = false; entry.power = NaN;
    entry.lease?.release(); entry.lease = undefined;
    if (proxy) { entry.proxy = proxy; entry.root.add(proxy); }
  };
  const wake = () => { dirty = true; changed(); };
  function pump() {
    if (disposed || !provider) return;
    let pending = 0;
    for (const entry of entries) if (entry.lease && !entry.instance) pending++;
    // At most two open slots need candidates. A strict comparison keeps the
    // original placement order for equal distances without a sorted copy.
    while (pending < 2) {
      let nearest: typeof entries[number] | undefined;
      for (const entry of entries) {
        if (!entry.near || !entry.available || entry.lease || entry.failed) continue;
        if (!nearest || entry.distance < nearest.distance) nearest = entry;
      }
      if (!nearest) break;
      const entry = nearest;
      const lease = acquireCompanionGltf(entry.placement.assetKey, provider);
      entry.lease = lease; pending++;
      lease.promise.then(asset => {
        if (disposed || entry.lease !== lease) return;
        const instance = instantiateGltf(asset);
        try {
          const meshes: THREE.Mesh[] = [], powered = new Map<THREE.MeshStandardMaterial, number>();
          let stationary = asset.animations.length === 0;
          instance.scene.traverse(node => {
            if (!(node instanceof THREE.Mesh)) return;
            if (node instanceof THREE.SkinnedMesh) stationary = false;
            if (node.userData.baddie_architecture_asset !== entry.placement.assetKey
              || node.userData.city_parcel_id !== entry.placement.parcelId) throw new Error("Architecture parcel metadata differs.");
            meshes.push(node);
            for (const material of Array.isArray(node.material) ? node.material : [node.material]) {
              if (!(material instanceof THREE.MeshStandardMaterial)) continue;
              const baseline = material.userData.baddie_emission_baseline;
              if (typeof baseline === "number" && Number.isFinite(baseline) && baseline > 0) {
                powered.set(material, baseline); material.emissiveIntensity = 0;
              }
            }
          });
          if (!meshes.length) throw new Error("Empty architecture asset.");
          entry.meshes = meshes; entry.powered = [...powered].map(([material, baseline]) => ({ material, baseline }));
          meshes.forEach(mesh => picking.set(mesh, entry.placement.parcelId));
          entry.instance = instance; entry.root.add(instance.scene);
          entry.releaseBinding = binding?.mounted(entry.placement, entry.root, meshes);
          if (stationary) instance.scene.traverse(cacheLocalTransform);
          entry.instanceNeedsSync = true; entry.power = NaN;
          releaseProxy(entry);
        } catch (error) {
          // Once installed, the lease catch below owns instance cleanup.
          if (entry.instance !== instance) instance.dispose();
          throw error;
        }
        wake(); pump();
      }).catch(() => {
        if (disposed || entry.lease !== lease) return;
        release(entry); entry.failed = true; wake(); pump();
      });
    }
  }
  return {
    group,
    parcel: (object: THREE.Object3D) => picking.get(object),
    update(target: THREE.Vector3, daylight: number) {
      if (disposed || !Number.isFinite(target.x) || !Number.isFinite(target.z)) return false;
      const moved = target.x !== targetX || target.z !== targetZ;
      targetX = target.x; targetZ = target.z;
      const next = delivery();
      if (next !== provider) {
        entries.forEach(entry => { release(entry); entry.failed = false; }); provider = next; dirty = true;
      }
      const light = THREE.MathUtils.clamp(Number.isFinite(daylight) ? daylight : 1, 0, 1);
      const power = THREE.MathUtils.clamp(Number.isFinite(mains.value) ? mains.value : 0, 0, 1) * emission(light);
      for (const entry of entries) {
        const focus = entry.placement.focusPosition ?? entry.placement.position;
        // Keep exact distances while stationary, including changed placement
        // coordinates. Availability, hysteresis and power still reconcile below.
        if (moved || focus[0] !== entry.focusX || focus[2] !== entry.focusZ) {
          entry.focusX = focus[0]; entry.focusZ = focus[2];
          entry.distance = Math.hypot(target.x - entry.focusX, target.z - entry.focusZ);
        }
        const near = entry.distance < (entry.near ? thresholds.exit : thresholds.enter);
        const shadows = near && light > .12 && entry.distance < (entry.shadows ? thresholds.shadowExit : thresholds.shadowEnter);
        const available = next.available(entry.placement.assetKey);
        const visibilityChanged = near !== entry.near, shadowsChanged = shadows !== entry.shadows;
        if (visibilityChanged || shadowsChanged) dirty = true;
        if ((!entry.near && near) || (available && !entry.available)) entry.failed = false;
        entry.near = near; entry.shadows = shadows; entry.available = available;
        if (entry.lease && (entry.distance > thresholds.retain || (!entry.instance && !near) || !available)) { release(entry); dirty = true; }
        if (entry.instanceNeedsSync || visibilityChanged || shadowsChanged) {
          for (const mesh of entry.meshes) {
            if (entry.instanceNeedsSync || visibilityChanged) mesh.visible = near || mesh.userData.game_distant_silhouette === true;
            if (entry.instanceNeedsSync || shadowsChanged) mesh.castShadow = shadows && mesh.userData.baddie_casts_shadow === true;
            if (entry.instanceNeedsSync) mesh.receiveShadow = mesh.userData.baddie_casts_shadow === true;
          }
          entry.instanceNeedsSync = false;
        }
        if (entry.power !== power) {
          for (const { material, baseline } of entry.powered) material.emissiveIntensity = baseline * power;
          entry.power = power;
        }
      }
      pump();
      const result = dirty; dirty = false; return result;
    },
    dispose() {
      if (disposed) return;
      disposed = true; entries.forEach(entry => { release(entry); releaseProxy(entry); });
      group.removeFromParent();
    },
  };
}
