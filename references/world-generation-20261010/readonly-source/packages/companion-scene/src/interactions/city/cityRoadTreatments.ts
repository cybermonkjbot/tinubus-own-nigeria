import * as THREE from "three";
import { companionCityRoadTreatments } from "../../../../../packages/utils/src/companionCity";
import type { CompanionAssetDelivery } from "../../../../../packages/utils/src/companionAssets";
import { publicCompanionAssets } from "../appearance/CompanionAssets";
import { acquireCompanionGltf, instantiateGltf, type GltfLease } from "../appearance/gltfAssets";

// Keep JS-number precision until the final instance matrix is written.
const placementStride = 4; // x, y, z, yaw
type Placements = { transforms: Float64Array; count: number };
type Tile = { x: number; z: number; distance: number; placements: Map<string, Placements>; root?: THREE.Group; shadows: boolean };

/** One shared download. Repeated palms and luminaires use per-cell instances, not cloned geometry. */
export function createCityRoadTreatments(mains: Readonly<{ value: number }>,
  delivery: () => CompanionAssetDelivery = () => publicCompanionAssets, changed: () => void = () => {}) {
  const group = new THREE.Group(); group.name = "Palm-lined streets and divided roads";
  const source = companionCityRoadTreatments(), cells = new Map<string, Tile>(), parts = new Map<string, THREE.Mesh[]>();
  const fallbackGeometry = new THREE.BoxGeometry(1, 1, 1);
  const fallbackConcrete = new THREE.MeshStandardMaterial({ color: "#c9c6b5", roughness: .96 });
  const fallbackTrunk = new THREE.MeshStandardMaterial({ color: "#887154", roughness: .94 });
  const fallbackSteel = new THREE.MeshStandardMaterial({ color: "#394b50", roughness: .6, metalness: .3 });
  const add = (kind: string, x: number, y: number, z: number, yaw: number) => {
    const cellX = Math.floor(x / 160), cellZ = Math.floor(z / 160), key = `${cellX}:${cellZ}`;
    let tile = cells.get(key);
    if (!tile) { tile = { x: cellX * 160 + 80, z: cellZ * 160 + 80, distance: Infinity, placements: new Map(), shadows: false }; cells.set(key, tile); }
    let batch = tile.placements.get(kind);
    if (!batch) { batch = { transforms: new Float64Array(8 * placementStride), count: 0 }; tile.placements.set(kind, batch); }
    const offset = batch.count * placementStride;
    if (offset === batch.transforms.length) {
      const grown = new Float64Array(batch.transforms.length * 2);
      grown.set(batch.transforms); batch.transforms = grown;
    }
    const values = batch.transforms;
    values[offset] = x; values[offset + 1] = y; values[offset + 2] = z; values[offset + 3] = yaw;
    batch.count++;
  };
  for (const island of source.islands) add("island", island.x, 0, island.z, island.yaw);
  for (const prop of source.props) add(prop.kind === "palm" ? `palm${prop.variant}` : "light",
    prop.x, prop.id.startsWith("median:") ? .16 : 0, prop.z, prop.yaw);
  for (const tile of cells.values()) for (const batch of tile.placements.values()) {
    const length = batch.count * placementStride;
    if (length !== batch.transforms.length) batch.transforms = batch.transforms.slice(0, length);
  }
  const powered: { material: THREE.MeshStandardMaterial; baseline: number }[] = [];
  // Only six nearest practical lights illuminate nearby ground. No shadow maps or light per palm.
  const practicals = Array.from({ length: 6 }, () => {
    const light = new THREE.SpotLight("#ffe2af", 0, 24, .9, .6, 2);
    light.castShadow = false; light.visible = false; group.add(light, light.target); return light;
  });
  const lampPositions = source.props.filter(p => p.kind === "light").map(p => ({
    x: p.x + Math.sin(p.yaw) * 1.4, z: p.z + Math.cos(p.yaw) * 1.4,
  }));
  const nearest: typeof lampPositions = [];
  const nearestDistances = new Float64Array(practicals.length);
  let lightX = Infinity, lightZ = Infinity;
  const selectLights = (x: number, z: number) => {
    if (x === lightX && z === lightZ) return;
    lightX = x; lightZ = z; nearest.length = 0;
    for (const point of lampPositions) {
      const distance = Math.hypot(point.x - x, point.z - z);
      if (!(distance < 45)) continue;
      let index = nearest.length;
      // Keep source order for equal distances, matching the previous stable sort.
      while (index > 0 && distance < nearestDistances[index - 1]) index--;
      if (index === practicals.length) continue;
      for (let next = Math.min(nearest.length, practicals.length - 1); next > index; next--) {
        nearest[next] = nearest[next - 1]; nearestDistances[next] = nearestDistances[next - 1];
      }
      nearest[index] = point; nearestDistances[index] = distance;
    }
  };
  const up = new THREE.Vector3(0, 1, 0), matrix = new THREE.Matrix4(), local = new THREE.Matrix4();
  const position = new THREE.Vector3(), rotation = new THREE.Quaternion(), scale = new THREE.Vector3(1, 1, 1);
  let provider: CompanionAssetDelivery | undefined, lease: GltfLease | undefined;
  let instance: ReturnType<typeof instantiateGltf> | undefined, failed = false, near = false, disposed = false, dirty = false;
  let lastX = Infinity, lastZ = Infinity, sunShadows = false;
  const clearTile = (tile: Tile) => {
    tile.root?.traverse(o => { if (o instanceof THREE.InstancedMesh) o.dispose(); });
    tile.root?.removeFromParent(); tile.root = undefined;
  };
  const release = () => {
    cells.forEach(clearTile); parts.clear(); powered.length = 0;
    instance?.dispose(); instance = undefined; lease?.release(); lease = undefined;
  };
  const wake = () => { dirty = true; changed(); };
  const load = () => {
    if (lease || failed || !near || !provider?.available("bestieCityRoadKit")) return;
    const current = acquireCompanionGltf("bestieCityRoadKit", provider); lease = current;
    current.promise.then(asset => {
      if (disposed || lease !== current) return;
      const candidate = instantiateGltf(asset), bins = new Map<string, THREE.Mesh[]>(), emission = new Map<THREE.MeshStandardMaterial, number>();
      try {
        candidate.scene.updateMatrixWorld(true);
        candidate.scene.traverse(o => {
          if (!(o instanceof THREE.Mesh)) return;
          const kind = o.userData.road_kit_part;
          if (!["palm0", "palm1", "light", "island"].includes(kind) || o instanceof THREE.SkinnedMesh) throw new Error("Invalid street kit part.");
          const values = bins.get(kind) ?? []; values.push(o); bins.set(kind, values);
          for (const m of Array.isArray(o.material) ? o.material : [o.material]) {
            if (!(m instanceof THREE.MeshStandardMaterial) || !m.userData.baddie_mains_powered) continue;
            const baseline = m.userData.baddie_emission_baseline;
            if (!Number.isFinite(baseline) || baseline <= 0) throw new Error("Missing streetlight emission baseline.");
            emission.set(m, baseline); m.emissiveIntensity = 0;
          }
        });
        if (bins.size !== 4 || emission.size !== 1) throw new Error("Incomplete street kit.");
        bins.forEach((v, k) => parts.set(k, v)); emission.forEach((baseline, material) => powered.push({ material, baseline }));
        instance = candidate; cells.forEach(clearTile); wake();
      } catch (error) { candidate.dispose(); throw error; }
    }).catch(() => { if (disposed || lease !== current) return; release(); failed = true; wake(); });
  };
  const assemble = (tile: Tile) => {
    const root = new THREE.Group(); root.name = `Street planting ${tile.x}:${tile.z}`;
    // Local transforms stay identity; parent/world transforms still update normally.
    root.matrixAutoUpdate = false;
    try {
      for (const [kind, sources] of parts) {
        const batch = tile.placements.get(kind);
        if (!batch) continue;
        const { count, transforms: values } = batch;
        const instances = sources.map(source => {
          const mesh = new THREE.InstancedMesh(source.geometry, source.material, count);
          mesh.matrixAutoUpdate = false;
          root.add(mesh); return mesh;
        });
        // Share one double-precision placement matrix across source meshes.
        // Only each final instance buffer receives the float32 result.
        for (let i = 0, offset = 0; i < count; i++, offset += placementStride) {
          rotation.setFromAxisAngle(up, values[offset + 3]);
          matrix.compose(position.set(values[offset], values[offset + 1], values[offset + 2]), rotation, scale);
          for (let index = 0; index < sources.length; index++) {
            local.multiplyMatrices(matrix, sources[index].matrixWorld); instances[index].setMatrixAt(i, local);
          }
        }
        for (const mesh of instances) {
          mesh.instanceMatrix.needsUpdate = true; mesh.computeBoundingBox(); mesh.computeBoundingSphere();
          mesh.castShadow = tile.shadows; mesh.receiveShadow = true;
        }
      }
    } catch (error) {
      root.traverse(node => { if (node instanceof THREE.InstancedMesh) node.dispose(); });
      root.clear(); throw error;
    }
    tile.root = root; group.add(root);
  };
  const assembleFallback = (tile: Tile) => {
    const root = new THREE.Group(); root.name = "Street planting download fallback";
    root.matrixAutoUpdate = false;
    // Visible exact reserves keep unavailable downloads from creating invisible collision walls.
    try {
      for (const kind of ["island", "palm0", "palm1", "light"]) {
        const batch = tile.placements.get(kind);
        if (!batch) continue;
        const { count, transforms: values } = batch;
        const island = kind === "island", palm = kind.startsWith("palm");
        const mesh = new THREE.InstancedMesh(fallbackGeometry, island ? fallbackConcrete : palm ? fallbackTrunk : fallbackSteel, count);
        mesh.matrixAutoUpdate = false; root.add(mesh);
        const height = island ? .16 : palm ? 6.1 : 7.15;
        for (let i = 0, offset = 0; i < count; i++, offset += placementStride) {
          rotation.setFromAxisAngle(up, values[offset + 3]);
          matrix.compose(position.set(values[offset], values[offset + 1] + height / 2, values[offset + 2]), rotation,
            scale.set(island ? 12 : palm ? .52 : .24, height, island ? 2.6 : palm ? .52 : .24));
          mesh.setMatrixAt(i, matrix);
        }
        mesh.instanceMatrix.needsUpdate = true; mesh.computeBoundingSphere(); mesh.castShadow = tile.shadows; mesh.receiveShadow = true;
      }
    } catch (error) {
      root.traverse(node => { if (node instanceof THREE.InstancedMesh) node.dispose(); });
      root.clear(); throw error;
    } finally { scale.set(1, 1, 1); }
    tile.root = root; group.add(root);
  };
  return {
    group,
    update(target: THREE.Vector3, daylight: number) {
      if (disposed || !Number.isFinite(target.x) || !Number.isFinite(target.z)) return false;
      const next = delivery();
      if (provider !== next) { release(); provider = next; failed = false; dirty = true; }
      const light = THREE.MathUtils.clamp(Number.isFinite(daylight) ? daylight : 1, 0, 1);
      const moved = target.x !== lastX || target.z !== lastZ;
      if (moved) {
        lastX = target.x; lastZ = target.z;
        let active = false;
        for (const tile of cells.values()) {
          tile.distance = Math.hypot(tile.x - target.x, tile.z - target.z);
          if (tile.distance < 420) active = true;
        }
        if (active && !near) failed = false; near = active;
      }
      if (lease && (!next.available("bestieCityRoadKit") || !near)) { release(); dirty = true; }
      load();
      const power = THREE.MathUtils.clamp(Number.isFinite(mains.value) ? mains.value : 0, 0, 1) * (1 - light);
      powered.forEach(p => { p.material.emissiveIntensity = p.baseline * power; });
      const nextSun = light > .12;
      // Asset completion, provider/availability changes and sun transitions must
      // still reconcile while stationary. Power and light intensity update below.
      if (moved || dirty || nextSun !== sunShadows) {
        sunShadows = nextSun;
        for (const tile of cells.values()) {
          const distance = tile.distance;
          if (tile.root && distance > 430) { clearTile(tile); dirty = true; }
          const shadows = nextSun && distance < (tile.shadows ? 205 : 150);
          if (shadows !== tile.shadows) {
            tile.shadows = shadows; tile.root?.traverse(o => { if (o instanceof THREE.InstancedMesh) o.castShadow = shadows; }); dirty = true;
          }
          if (!tile.root && distance < 305) { dirty = true; if (instance) assemble(tile); else assembleFallback(tile); }
        }
      }
      const lit = !!instance && power > .02;
      if (lit) selectLights(target.x, target.z);
      practicals.forEach((lamp, i) => {
        const p = lit ? nearest[i] : undefined; lamp.visible = !!p; lamp.intensity = p ? 70 * power : 0;
        if (p) { lamp.position.set(p.x, 7.3, p.z); lamp.target.position.set(p.x, 0, p.z); }
      });
      const result = dirty; dirty = false; return result;
    },
    dispose() { if (disposed) return; disposed = true; release(); cells.clear(); practicals.forEach(light => light.dispose());
      fallbackGeometry.dispose(); fallbackConcrete.dispose(); fallbackTrunk.dispose(); fallbackSteel.dispose(); group.removeFromParent(); },
  };
}
