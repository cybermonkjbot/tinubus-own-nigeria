import * as THREE from "three";
import design from "../../../../../packages/assets/city/lagos-architecture-delivery-v1.json";
import { companionCityPlan } from "../../../../../packages/utils/src/companionCity";
import type { CompanionAssetDelivery, CompanionAssetKey } from "../../../../../packages/utils/src/companionAssets";
import { companionArchitectureAssetEntries } from "../../../../../packages/utils/src/companionArchitectureAssets";
import { publicCompanionAssets } from "../appearance/CompanionAssets";
import { createHostedArchitecture } from "./hostedArchitecture";

type Shape = "box" | "pipe" | "crown" | "gable" | "arch" | "sawtooth" | "rounded-box" | "rounded-ring";
type Finish = keyof typeof design.materials;
type Building = typeof design.buildings[number];

// This catalog replaces exterior geometry only. Parcel identity and Home ownership stay in the city plan.
export const signatureBuildingParcelIds: ReadonlySet<string> = new Set(design.buildings.map(building => building.parcelId));

function roofGeometry(sawtooth = false) {
  const roof = new THREE.BufferGeometry();
  const ridge = sawtooth ? .5 : 0;
  const points = [[-.5, -.5, -.5], [.5, -.5, -.5], [-.5, -.5, .5], [.5, -.5, .5],
    [ridge, .5, -.5], [ridge, .5, .5]];
  roof.setAttribute("position", new THREE.Float32BufferAttribute(
    [0, 4, 1, 2, 3, 5, 0, 2, 5, 0, 5, 4, 1, 4, 5, 1, 5, 3, 0, 1, 3, 0, 3, 2].flatMap(i => points[i]), 3));
  roof.computeVertexNormals();
  return roof;
}

function archGeometry() {
  // Extruded ring only, retaining the opening and real reveal thickness.
  const shape = new THREE.Shape();
  shape.absellipse(0, -.5, .5, 1, 0, Math.PI, false, 0);
  shape.lineTo(-.425, -.5);
  shape.absellipse(0, -.5, .425, .82, Math.PI, 0, true, 0);
  shape.closePath();
  const arch = new THREE.ExtrudeGeometry(shape, { depth: 1, bevelEnabled: false, steps: 1, curveSegments: 12 });
  arch.translate(0, 0, -.5);
  return arch;
}

function roundedGeometry(ring = false) {
  // Extruded rounded balcony plan, with an open centre for rails and illuminated edges.
  const shape = new THREE.Shape();
  const outline = (path: THREE.Shape | THREE.Path, extent: number, radius: number) => {
    path.moveTo(extent - radius, extent);
    for (const [x, y, angle] of [
      [-extent + radius, extent - radius, Math.PI / 2],
      [-extent + radius, -extent + radius, Math.PI],
      [extent - radius, -extent + radius, Math.PI * 1.5],
      [extent - radius, extent - radius, 0],
    ]) {
      path.lineTo(x + radius * Math.cos(angle), y + radius * Math.sin(angle));
      path.absarc(x, y, radius, angle, angle + Math.PI / 2, false);
    }
    path.closePath();
  };
  outline(shape, .5, .16);
  if (ring) { const hole = new THREE.Path(); outline(hole, .4875, .1475); shape.holes.push(hole); }
  const geometry = new THREE.ExtrudeGeometry(shape, { depth: 1, bevelEnabled: false, steps: 1, curveSegments: 6 });
  geometry.translate(0, 0, -.5); geometry.rotateX(-Math.PI / 2);
  return geometry;
}

/** Only small silhouette proxies remain bundled; full buildings download individually. */
export function createSignatureBuildings(mains: Readonly<{ value: number }>,
  delivery: () => CompanionAssetDelivery = () => publicCompanionAssets, changed: () => void = () => {}) {
  const geometries: Record<Shape, THREE.BufferGeometry> = {
    box: new THREE.BoxGeometry(1, 1, 1), pipe: new THREE.CylinderGeometry(.5, .5, 1, 10),
    crown: new THREE.IcosahedronGeometry(.5, 1), gable: roofGeometry(),
    arch: archGeometry(), sawtooth: roofGeometry(true),
    "rounded-box": roundedGeometry(), "rounded-ring": roundedGeometry(true),
  };
  const materials = new Map<Finish, THREE.MeshStandardMaterial>();
  for (const [name, spec] of Object.entries(design.materials)) materials.set(name as Finish,
    new THREE.MeshStandardMaterial({ color: "#ffffff", roughness: spec.roughness, metalness: spec.metalness }));
  for (const building of design.buildings) {
    const parcel = companionCityPlan.parcels.find(parcel => parcel.id === building.parcelId);
    if (!parcel || parcel.x !== building.position[0] || parcel.z !== building.position[2] || parcel.yaw !== building.yaw
      || parcel.units !== building.units || parcel.width !== building.footprint[0] || parcel.depth !== building.footprint[1]
      || building.maximumHeight !== parcel.floors * 3.3 + (parcel.kind === "tower" ? 6.6 : 3)
      || parcel.entrance.some((coordinate, index) => coordinate !== building.entrance[index]))
      throw new Error("Signature buildings must retain their existing plots.");
  }
  const placements = design.buildings.map(building => {
    if (!Object.hasOwn(companionArchitectureAssetEntries, building.assetKey)) throw new Error("Missing signature asset registration.");
    return { ...building, assetKey: building.assetKey as CompanionAssetKey };
  });
  const hosted = createHostedArchitecture(placements, placement => {
    const building = design.buildings.find(building => building.parcelId === placement.parcelId)!;
    const proxy = new THREE.Group(); proxy.name = "Download fallback silhouette";
    const bins = new Map<string, Building["parts"]>();
    for (const part of building.parts) {
      const key = part.shape + ":" + part.material;
      const parts = bins.get(key) ?? []; parts.push(part); bins.set(key, parts);
    }
    const matrix = new THREE.Matrix4(), rotation = new THREE.Quaternion(), euler = new THREE.Euler();
    const position = new THREE.Vector3(), scale = new THREE.Vector3(), colour = new THREE.Color();
    for (const parts of bins.values()) {
      const first = parts[0];
      const mesh = new THREE.InstancedMesh(geometries[first.shape as Shape], materials.get(first.material as Finish)!, parts.length);
      for (const [index, part] of parts.entries()) {
        euler.set(part.rotation[0], part.rotation[1], part.rotation[2]); rotation.setFromEuler(euler);
        matrix.compose(position.fromArray(part.position), rotation, scale.fromArray(part.size)); mesh.setMatrixAt(index, matrix);
        mesh.setColorAt(index, colour.set(part.colour));
      }
      mesh.instanceMatrix.needsUpdate = true;
      if (mesh.instanceColor) mesh.instanceColor.needsUpdate = true;
      mesh.computeBoundingSphere(); proxy.add(mesh);
    }
    return proxy;
  }, mains, delivery, changed, { enter: 420, exit: 560, shadowEnter: 150, shadowExit: 205, retain: 1000 });
  let disposed = false;
  return {
    group: hosted.group, parcel: hosted.parcel, update: hosted.update,
    dispose() {
      if (disposed) return;
      disposed = true; hosted.dispose(); Object.values(geometries).forEach(geometry => geometry.dispose()); materials.forEach(material => material.dispose());
    },
  };
}
