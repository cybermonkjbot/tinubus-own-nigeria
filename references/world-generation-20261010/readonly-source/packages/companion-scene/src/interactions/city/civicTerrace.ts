import * as THREE from "three";
import design from "../../../../../packages/assets/venues/civic-terrace-v1.json";
import { companionCityPlan, companionCityParcelPoint } from "../../../../../packages/utils/src/companionCity";

type Shape = "box" | "pipe" | "crown";
type Part = typeof design.parts[number];

/** One reference-led building on an existing non-residential plot. The city
 * keeps ownership of navigation, power, picking and render scheduling. */
export function createCivicTerrace() {
  const parcel = companionCityPlan.parcels.find(value => value.id === design.parcelId);
  if (!parcel || parcel.units !== 0 || parcel.kind !== "shop") throw new Error("The terrace needs its existing commercial plot.");
  const group = new THREE.Group();
  group.name = "Civic Street terrace";
  group.position.set(parcel.x, 0, parcel.z); group.rotation.y = parcel.yaw;
  group.updateMatrix(); group.matrixAutoUpdate = false;
  const geometries = new Map<Shape, THREE.BufferGeometry>([
    ["box", new THREE.BoxGeometry(1, 1, 1)],
    ["pipe", new THREE.CylinderGeometry(.5, .5, 1, 10)],
    ["crown", new THREE.IcosahedronGeometry(.5, 1)],
  ]);
  const materials = new Map<string, THREE.MeshStandardMaterial>();
  for (const [name, value] of Object.entries(design.materials)) {
    materials.set(name, new THREE.MeshStandardMaterial({ color: value.colour, roughness: value.roughness, metalness: value.metalness }));
  }
  const near = new THREE.Group(), distant = new THREE.Group();
  near.name = "Terrace construction and planting"; distant.name = "Terrace distant silhouette";
  near.matrixAutoUpdate = false; distant.matrixAutoUpdate = false;
  group.add(near, distant);
  const matrix = new THREE.Matrix4(), position = new THREE.Vector3(), scale = new THREE.Vector3();
  const quaternion = new THREE.Quaternion(), euler = new THREE.Euler();
  const meshes: THREE.InstancedMesh[] = [];
  const release = () => {
    meshes.forEach(mesh => mesh.dispose());
    meshes.length = 0; near.clear(); distant.clear(); group.clear();
    geometries.forEach(geometry => geometry.dispose()); geometries.clear();
    materials.forEach(material => material.dispose()); materials.clear();
  };
  function batch(parent: THREE.Group, parts: readonly Part[], shadow: boolean) {
    const bins = new Map<string, Part[]>();
    for (const part of parts) {
      const key = `${part.shape}:${part.material}`;
      const values = bins.get(key) ?? []; values.push(part); bins.set(key, values);
    }
    for (const parts of bins.values()) {
      const first = parts[0];
      const mesh = new THREE.InstancedMesh(geometries.get(first.shape as Shape)!, materials.get(first.material)!, parts.length);
      meshes.push(mesh);
      mesh.name = `Civic terrace ${first.shape} ${first.material}`;
      mesh.matrixAutoUpdate = false;
      const parcelIds: string[] = new Array(parts.length);
      mesh.userData.cityParcelIds = parcelIds;
      for (const [index, part] of parts.entries()) {
        euler.set(part.rotation[0], part.rotation[1], part.rotation[2]); quaternion.setFromEuler(euler);
        matrix.compose(position.fromArray(part.position), quaternion, scale.fromArray(part.size));
        mesh.setMatrixAt(index, matrix);
        parcelIds[index] = design.parcelId;
      }
      mesh.instanceMatrix.needsUpdate = true; mesh.computeBoundingSphere();
      mesh.castShadow = shadow; mesh.receiveShadow = first.material !== "glass" && first.material !== "paleGlass";
      parent.add(mesh);
    }
  }
  try {
    batch(near, design.parts, true);
    batch(distant, design.parts.filter(part => part.distant), false);
  } catch (error) { release(); throw error; }
  near.visible = false;
  const focus = companionCityParcelPoint(parcel, 0, 6);
  let nearVisible = false, shadowEnabled = true, disposed = false;
  let viewX = NaN, viewZ = NaN, distance = Infinity;
  return {
    group,
    parcelId: design.parcelId,
    update(target: THREE.Vector3, daylight: number): boolean {
      if (disposed || !Number.isFinite(target.x) || !Number.isFinite(target.z)) return false;
      if (target.x !== viewX || target.z !== viewZ) {
        viewX = target.x; viewZ = target.z;
        distance = Math.hypot(viewX - focus[0], viewZ - focus[2]);
      }
      const nextNear = distance < (nearVisible ? 280 : 230);
      const nextVisible = distance < 1500;
      const shadows = daylight > .12 && distance < (shadowEnabled ? 180 : 140);
      const changed = nextNear !== nearVisible || nextVisible !== group.visible || shadows !== shadowEnabled;
      if (!changed) return false;
      group.visible = nextVisible; near.visible = nextNear; distant.visible = !nextNear;
      if (shadows !== shadowEnabled) for (const child of near.children) if (child instanceof THREE.InstancedMesh) child.castShadow = shadows;
      nearVisible = nextNear; shadowEnabled = shadows;
      return changed;
    },
    dispose() {
      if (disposed) return;
      disposed = true; group.removeFromParent();
      release();
    },
  };
}
