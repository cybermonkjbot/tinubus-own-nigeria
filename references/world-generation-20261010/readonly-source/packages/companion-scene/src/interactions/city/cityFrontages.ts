import * as THREE from "three";
import { companionCityPlan, companionCityParcelPoint } from "../../../../../packages/utils/src/companionCity";
import { companionCityFrontageProps } from "../../../../../packages/utils/src/companionCityFrontages";
import type { CompanionAssetDelivery } from "../../../../../packages/utils/src/companionAssets";
import { acquireCompanionGltf, instantiateGltf, type GltfLease } from "../appearance/gltfAssets";

// Keep JS-number precision until the final instance matrix is written.
const placementStride = 6; // x, z, yaw, scale x/y/z
type Placements = { transforms: Float64Array; parcelIds: string[] };
type Cell = { x: number; z: number; distance: number; placements: Map<string, Placements>; root?: THREE.Group; shadows: boolean };

/** One optional shared kit, instanced locally, with no independent render or account owner. */
export function createCityFrontages(delivery: () => CompanionAssetDelivery, changed: () => void) {
  const group = new THREE.Group(); group.name = "Individual landscaped frontages";
  const cells = new Map<string, Cell>(), parts = new Map<string, THREE.Mesh[]>();
  // Keep planting visible during a slow or failed kit download. These small
  // trees fit the same 1.8 m garden reservations as the delivered tree.
  const fallbackTrunk = new THREE.Mesh(new THREE.CylinderGeometry(.1, .15, 3.1, 7),
    new THREE.MeshStandardMaterial({ color: "#776650", roughness: .95 }));
  fallbackTrunk.position.y = 1.55; fallbackTrunk.updateMatrixWorld(true);
  const fallbackCrown = new THREE.Mesh(new THREE.IcosahedronGeometry(1, 1),
    new THREE.MeshStandardMaterial({ color: "#637b4e", roughness: .94 }));
  fallbackCrown.position.y = 3.35; fallbackCrown.scale.set(.88, 1.05, .88); fallbackCrown.updateMatrixWorld(true);
  const fallbackTrees = [fallbackTrunk, fallbackCrown];
  const useFallback = () => { parts.clear(); parts.set("tree", fallbackTrees); };
  useFallback();
  for (const parcel of companionCityPlan.parcels) {
    const cellX = Math.floor(parcel.x / 160), cellZ = Math.floor(parcel.z / 160), key = `${cellX}:${cellZ}`;
    for (const prop of companionCityFrontageProps(parcel)) {
      const at = companionCityParcelPoint(parcel, prop.x, prop.z);
      let cell = cells.get(key);
      if (!cell) { cell = { x: cellX * 160 + 80, z: cellZ * 160 + 80, distance: Infinity, placements: new Map(), shadows: false }; cells.set(key,cell); }
      let batch = cell.placements.get(prop.kind);
      if (!batch) { batch = { transforms: new Float64Array(8 * placementStride), parcelIds: [] }; cell.placements.set(prop.kind,batch); }
      const offset = batch.parcelIds.length * placementStride;
      if (offset === batch.transforms.length) {
        const grown = new Float64Array(batch.transforms.length * 2);
        grown.set(batch.transforms); batch.transforms = grown;
      }
      const values = batch.transforms;
      values[offset] = at[0]; values[offset + 1] = at[2]; values[offset + 2] = parcel.yaw + prop.yaw;
      values[offset + 3] = prop.scale[0]; values[offset + 4] = prop.scale[1]; values[offset + 5] = prop.scale[2];
      batch.parcelIds.push(parcel.id);
    }
  }
  for (const cell of cells.values()) for (const batch of cell.placements.values()) {
    const length = batch.parcelIds.length * placementStride;
    if (length !== batch.transforms.length) batch.transforms = batch.transforms.slice(0,length);
  }
  const matrix = new THREE.Matrix4(), local = new THREE.Matrix4(), position = new THREE.Vector3(), scale = new THREE.Vector3();
  const rotation = new THREE.Quaternion(), up = new THREE.Vector3(0,1,0);
  let provider: CompanionAssetDelivery | undefined, lease: GltfLease | undefined;
  let instance: ReturnType<typeof instantiateGltf> | undefined, disposed = false, failed = false, near = false, dirty = false;
  let lastX = Infinity, lastZ = Infinity, sunShadows = false;
  const clear = (cell: Cell) => { cell.root?.traverse(o => { if (o instanceof THREE.InstancedMesh) o.dispose(); }); cell.root?.removeFromParent(); cell.root = undefined; };
  const release = () => { cells.forEach(clear); instance?.dispose(); instance = undefined; lease?.release(); lease = undefined; useFallback(); };
  const load = () => {
    if (!near || lease || failed || !provider?.available("bestieCityFrontageKit")) return;
    const current = acquireCompanionGltf("bestieCityFrontageKit",provider); lease = current;
    current.promise.then(asset => {
      if (disposed || lease !== current) return;
      const candidate = instantiateGltf(asset), bins = new Map<string,THREE.Mesh[]>();
      try {
        candidate.scene.updateMatrixWorld(true);
        candidate.scene.traverse(o => {
          if (!(o instanceof THREE.Mesh)) return;
          const kind = o.userData.frontage_kit_part;
          if (typeof kind !== "string" || !["lawn","paving","shrub","flowers","croton","agave","tree","planter","bench","bin","hose"].includes(kind)
            || o instanceof THREE.SkinnedMesh) throw new Error("Invalid frontage kit.");
          const values = bins.get(kind) ?? []; values.push(o); bins.set(kind,values);
        });
        if (bins.size !== 11) throw new Error("Incomplete frontage kit.");
        cells.forEach(clear); parts.clear();
        instance = candidate; bins.forEach((v,k) => parts.set(k,v)); dirty = true; changed();
      } catch (error) { candidate.dispose(); throw error; }
    }).catch(() => { if (disposed || lease !== current) return; release(); failed = true; dirty = true; changed(); });
  };
  const assemble = (cell: Cell) => {
    const root = new THREE.Group(); root.name = `Frontage detail ${cell.x}:${cell.z}`;
    // Local transforms stay identity; parent/world transforms still update normally.
    root.matrixAutoUpdate = false;
    try {
      for (const [kind, sources] of parts) {
        const batch = cell.placements.get(kind);
        if (!batch) continue;
        const count = batch.parcelIds.length, values = batch.transforms;
        const instances = sources.map(source => {
          const mesh = new THREE.InstancedMesh(source.geometry, source.material, count); mesh.name = kind;
          mesh.matrixAutoUpdate = false;
          // Picking only reads this table, which stays ordered with the packed transforms.
          root.add(mesh); mesh.userData.cityParcelIds = batch.parcelIds; return mesh;
        });
        for (let i = 0, offset = 0; i < count; i++, offset += placementStride) {
          rotation.setFromAxisAngle(up, values[offset + 2]);
          matrix.compose(position.set(values[offset], 0, values[offset + 1]), rotation,
            scale.set(values[offset + 3], values[offset + 4], values[offset + 5]));
          for (let index = 0; index < sources.length; index++) {
            local.multiplyMatrices(matrix, sources[index].matrixWorld); instances[index].setMatrixAt(i, local);
          }
        }
        for (const mesh of instances) {
          mesh.instanceMatrix.needsUpdate = true; mesh.computeBoundingSphere(); mesh.computeBoundingBox();
          mesh.castShadow = cell.shadows && kind !== "lawn" && kind !== "paving"; mesh.receiveShadow = true;
        }
      }
    } catch (error) {
      root.traverse(node => { if (node instanceof THREE.InstancedMesh) node.dispose(); });
      root.clear(); throw error;
    }
    cell.root = root; group.add(root);
  };
  return {
    group,
    update(target: THREE.Vector3, daylight: number) {
      if (disposed || !Number.isFinite(target.x) || !Number.isFinite(target.z)) return false;
      const next = delivery();
      if (next !== provider) { release(); provider = next; failed = false; dirty = true; }
      const moved = target.x !== lastX || target.z !== lastZ;
      if (moved) {
        lastX = target.x; lastZ = target.z;
        let active = false;
        for (const cell of cells.values()) {
          cell.distance = Math.hypot(cell.x - target.x, cell.z - target.z);
          if (cell.distance < 620) active = true;
        }
        if (active && !near) failed = false; near = active;
      }
      if (lease && (!near || !next.available("bestieCityFrontageKit"))) { release(); dirty = true; }
      load();
      const nextSun = daylight > .12;
      if (moved || dirty || nextSun !== sunShadows) {
        sunShadows = nextSun;
        for (const cell of cells.values()) {
          const distance = cell.distance;
          if (cell.root && distance > 560) { clear(cell); dirty = true; }
          const shadows = nextSun && distance < (cell.shadows ? 205 : 150);
          if (cell.shadows !== shadows) {
            cell.shadows = shadows; cell.root?.traverse(o => { if (o instanceof THREE.InstancedMesh) o.castShadow = shadows && o.name !== "lawn" && o.name !== "paving"; }); dirty = true;
          }
          if (!cell.root && distance < 420) { dirty = true; assemble(cell); }
        }
      }
      const result = dirty; dirty = false; return result;
    },
    dispose() {
      if (disposed) return; disposed = true; release(); cells.clear(); parts.clear();
      for (const tree of fallbackTrees) { tree.geometry.dispose(); tree.material.dispose(); }
      group.removeFromParent();
    },
  };
}
