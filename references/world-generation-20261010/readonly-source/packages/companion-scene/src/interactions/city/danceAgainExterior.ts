import * as THREE from "three";
import manifest from "../../../../../packages/assets/city/lagos-architecture-delivery-v1.json";
import { companionCityPlan } from "../../../../../packages/utils/src/companionCity";
import type { CompanionAssetDelivery } from "../../../../../packages/utils/src/companionAssets";
import { publicCompanionAssets } from "../appearance/CompanionAssets";
import { createHostedArchitecture } from "./hostedArchitecture";

const design = manifest.exterior;
type Shape = "box" | "pipe" | "crown";

/** Exterior geometry is independently hosted. City navigation and power keep ownership. */
export function createDanceAgainExterior(mains: Readonly<{ value: number }>,
  delivery: () => CompanionAssetDelivery = () => publicCompanionAssets, changed: () => void = () => {}) {
  const parcel = companionCityPlan.parcels.find(value => value.id === design.parcelId);
  if (!parcel || parcel.kind !== "shop" || parcel.units !== 0
    || design.footprint.width > parcel.width || design.footprint.depth > parcel.depth)
    throw new Error("Dance Again needs its bounded commercial plot.");
  const geometries: Record<Shape, THREE.BufferGeometry> = {
    box: new THREE.BoxGeometry(1, 1, 1), pipe: new THREE.CylinderGeometry(.5, .5, 1, 10),
    crown: new THREE.IcosahedronGeometry(.5, 1),
  };
  const materials = new Map(Object.entries(design.materials).map(([name, spec]) => [name,
    new THREE.MeshStandardMaterial({ color: spec.colour, roughness: spec.roughness, metalness: spec.metalness })]));
  const hosted = createHostedArchitecture([{ assetKey: "bestieDanceAgainExterior", parcelId: parcel.id,
    label: "Dance Again Market Road exterior", position: [parcel.x, 0, parcel.z], focusPosition: parcel.entrance, yaw: parcel.yaw }], () => {
    const proxy = new THREE.Group(), matrix = new THREE.Matrix4(), position = new THREE.Vector3(), scale = new THREE.Vector3();
    const rotation = new THREE.Quaternion(), euler = new THREE.Euler();
    const bins = new Map<string, typeof design.parts>();
    for (const part of design.parts) {
      const key = part.shape + ":" + part.material, parts = bins.get(key) ?? [];
      parts.push(part); bins.set(key, parts);
    }
    for (const parts of bins.values()) {
      const first = parts[0], mesh = new THREE.InstancedMesh(geometries[first.shape as Shape], materials.get(first.material)!, parts.length);
      for (const [index, part] of parts.entries()) {
        euler.set(part.rotation[0], part.rotation[1], part.rotation[2]); rotation.setFromEuler(euler);
        matrix.compose(position.fromArray(part.position), rotation, scale.fromArray(part.size)); mesh.setMatrixAt(index, matrix);
      }
      mesh.instanceMatrix.needsUpdate = true; mesh.computeBoundingSphere(); proxy.add(mesh);
    }
    return proxy;
  }, mains, delivery, changed, { enter: 230, exit: 280, shadowEnter: 140, shadowExit: 180, retain: 1000 },
  daylight => .18 + .82 * (1 - daylight));
  const lights = [-1.73, 1.73].map(x => {
    const light = new THREE.PointLight(design.materials.amber.colour, 0, 6, 2);
    const c = Math.cos(parcel.yaw), s = Math.sin(parcel.yaw);
    light.position.set(parcel.x + x * c + 9.04 * s, 2.08, parcel.z - x * s + 9.04 * c);
    light.name = "Dance Again entrance practical"; light.castShadow = false; hosted.group.add(light); return light;
  });
  let disposed = false;
  return {
    group: hosted.group, parcelId: parcel.id, parcel: hosted.parcel,
    update(target: THREE.Vector3, daylight: number) {
      if (disposed || !Number.isFinite(target.x) || !Number.isFinite(target.z)) return false;
      const result = hosted.update(target, daylight);
      const distance = Math.hypot(target.x - parcel.entrance[0], target.z - parcel.entrance[2]);
      const visible = distance < 1500, visibilityChanged = visible !== hosted.group.visible;
      hosted.group.visible = visible;
      if (Number.isFinite(daylight) && Number.isFinite(mains.value)) {
        const intensity = 3 * (1 - THREE.MathUtils.clamp(daylight, 0, 1)) * THREE.MathUtils.clamp(mains.value, 0, 1);
        lights.forEach(light => { light.visible = visible && distance < 75; light.intensity = intensity; });
      }
      return result || visibilityChanged;
    },
    dispose() {
      if (disposed) return;
      disposed = true; hosted.dispose(); lights.forEach(light => light.dispose());
      Object.values(geometries).forEach(geometry => geometry.dispose()); materials.forEach(material => material.dispose());
    },
  };
}
