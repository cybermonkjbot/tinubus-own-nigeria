import * as THREE from "three";
import geometry from "../../../../../packages/assets/city/lagos-national-theatre-geometry-v1.json";
import { companionCityTheatre as recipe } from "../../../../../packages/utils/src/companionCityTheatre";
import type { CompanionAssetDelivery } from "../../../../../packages/utils/src/companionAssets";
import { publicCompanionAssets } from "../appearance/CompanionAssets";
import { createCityDistrictAsset } from "./cityDistrictAsset";
import { applyCityStreetFinish } from "./cityStreetFinish";

/** The editable Blender scene and game use the same authored vertices and district recipe. */
export function createCityTheatre(delivery: () => CompanionAssetDelivery = () => publicCompanionAssets,
  changed: () => void = () => {}, hostedBuilding = false) {
  const group = new THREE.Group(); group.name = "National Theatre and gardens";
  const building = new THREE.Group(); building.name = "National Theatre fallback building"; group.add(building);
  const shapes: THREE.BufferGeometry[] = [], materials: THREE.Material[] = [];
  const palette = new Map(Object.entries(geometry.materials).map(([key, color]) => {
    const material = new THREE.MeshStandardMaterial({ color, roughness: key === "glass" ? .22 : .86,
      metalness: key === "glass" ? .3 : key === "trim" ? .2 : 0, side: THREE.DoubleSide });
    materials.push(material); return [key, material] as const;
  }));
  const roadMaterials = new Map(["asphalt", "path"].map(key => {
    const material = palette.get(key)!.clone(); materials.push(material);
    applyCityStreetFinish(material, key === "asphalt" ? "asphalt" : "path");
    return [key, material] as const;
  }));
  let releaseFallback: (() => void) | undefined = createTheatreFallback(building, palette, false);
  const add = (name: string, positions: number[], indices: number[], material: string, uvs: number[]) => {
    const shape = new THREE.BufferGeometry(); shapes.push(shape);
    shape.setAttribute("position", new THREE.Float32BufferAttribute(positions, 3)); shape.setIndex(indices);
    shape.setAttribute("uv", new THREE.Float32BufferAttribute(uvs, 2));
    shape.computeVertexNormals(); shape.computeBoundingSphere();
    const mesh = new THREE.Mesh(shape, roadMaterials.get(material)); mesh.name = name;
    mesh.userData.theatreShadow = false;
    mesh.castShadow = false; mesh.receiveShadow = true;
    group.add(mesh);
  };
  const transform = new THREE.Matrix4(), rotation = new THREE.Quaternion(), scale = new THREE.Vector3(), position = new THREE.Vector3();
  for (const road of [...recipe.roads, ...recipe.paths]) {
    const ribbon = (width: number, y: number, material: string) => {
      const vertices: number[] = [], faces: number[] = [], uvs: number[] = [];
      let distance = 0;
      road.points.forEach((p, i) => {
        if (i) distance += Math.hypot(p[0] - road.points[i - 1][0], p[1] - road.points[i - 1][1]);
        const closed = road.points[0][0] === road.points.at(-1)![0] && road.points[0][1] === road.points.at(-1)![1];
        const a = road.points[i === 0 && closed ? road.points.length - 2 : Math.max(0, i - 1)];
        const b = road.points[i === road.points.length - 1 && closed ? 1 : Math.min(road.points.length - 1, i + 1)];
        const length = Math.hypot(b[0] - a[0], b[1] - a[1]) || 1;
        const nx = -(b[1] - a[1]) / length * width / 2, nz = (b[0] - a[0]) / length * width / 2;
        vertices.push(p[0] + nx, y, p[1] + nz, p[0] - nx, y, p[1] - nz);
        uvs.push(width / 2, distance, -width / 2, distance);
        if (i) { const j = (i - 1) * 2; faces.push(j, j + 2, j + 1, j + 1, j + 2, j + 3); }
      }); add(road.name, vertices, faces, material, uvs);
    };
    if (road.kind !== "path") ribbon(road.width + road.pavement * 2, -.019, "path");
    ribbon(road.width, -.008, road.kind === "path" ? "path" : "asphalt");
  }
  const trunk = new THREE.CylinderGeometry(.35, .5, 1, 6), crown = new THREE.IcosahedronGeometry(1, 1);
  shapes.push(trunk, crown);
  const stems = new THREE.InstancedMesh(trunk, palette.get("trunk"), recipe.trees.length);
  const crowns = new THREE.InstancedMesh(crown, palette.get("leaves"), recipe.trees.length * 3);
  recipe.trees.forEach((tree, i) => {
    transform.compose(position.set(tree.x, tree.height / 2, tree.z), rotation.identity(), scale.set(1, tree.height, 1)); stems.setMatrixAt(i, transform);
    for (let j = 0; j < 3; j++) {
      const angle = j / 3 * Math.PI * 2;
      transform.compose(position.set(tree.x + Math.cos(angle) * tree.radius * .32, tree.height - j * .7, tree.z + Math.sin(angle) * tree.radius * .32),
        rotation.identity(), scale.set(tree.radius, tree.palm ? 1.2 : tree.radius * .7, tree.radius)); crowns.setMatrixAt(i * 3 + j, transform);
    }
  });
  for (const mesh of [stems, crowns]) { mesh.computeBoundingSphere(); mesh.userData.theatreShadow = true; mesh.castShadow = false; mesh.receiveShadow = true; group.add(mesh); }
  const hosted = hostedBuilding ? createCityDistrictAsset("bestieNationalTheatre", [recipe.center[0],0,recipe.center[1]], delivery, changed) : undefined;
  if(hosted)group.add(hosted.group);
  let disposed = false, near = false;
  return { group, update(target: THREE.Vector3, daylight: number) {
    if(disposed)return false;
    const assetChanged=hosted?.update(target,daylight)??false;
    const needsFallback = !hosted?.loaded;
    let fallbackChanged = false;
    if (needsFallback && !releaseFallback) {
      // Restore current shadows even when the 600 m state did not change.
      releaseFallback = createTheatreFallback(building, palette, near); fallbackChanged = true;
    } else if (!needsFallback && releaseFallback) {
      releaseFallback(); releaseFallback = undefined; fallbackChanged = true;
    }
    building.visible = needsFallback;
    const next = Math.hypot(target.x - recipe.center[0], target.z - recipe.center[1]) < 600;
    if (next === near) return assetChanged || fallbackChanged; near = next;
    group.traverse(object => { if (object instanceof THREE.Mesh) object.castShadow = next && object.userData.theatreShadow === true; });
    return true;
  }, dispose() {
    if (disposed) return; disposed = true;
    hosted?.dispose();
    releaseFallback?.(); releaseFallback = undefined;
    group.traverse(object => { if (object instanceof THREE.InstancedMesh) object.dispose(); });
    group.removeFromParent(); group.clear(); shapes.forEach(shape => shape.dispose()); materials.forEach(material => material.dispose());
  } };
}

/** Roads, trees and their shared palette outlive the hosted building's fallback. */
function createTheatreFallback(building: THREE.Group, palette: Map<string, THREE.MeshStandardMaterial>, near: boolean) {
  const shapes: THREE.BufferGeometry[] = [];
  let signTexture: THREE.CanvasTexture | undefined, signMaterial: THREE.MeshStandardMaterial | undefined;
  let canvas: HTMLCanvasElement | undefined, disposed = false;
  const release = () => {
    if (disposed) return; disposed = true;
    building.traverse(object => { if (object instanceof THREE.InstancedMesh) object.dispose(); });
    building.clear(); shapes.forEach(shape => shape.dispose()); shapes.length = 0;
    signMaterial?.dispose(); signMaterial = undefined;
    signTexture?.dispose(); signTexture = undefined;
    if (canvas) { canvas.width = 0; canvas.height = 0; canvas = undefined; }
  };
  try {
    for (const part of geometry.parts) {
      const shape = new THREE.BufferGeometry(); shapes.push(shape);
      shape.setAttribute("position", new THREE.Float32BufferAttribute(part.vertices.flat(), 3));
      shape.setIndex(part.faces.flatMap(face =>
        Array.from({ length: face.length - 2 }, (_, i) => [face[0], face[i + 1], face[i + 2]]).flat()));
      shape.computeVertexNormals(); shape.computeBoundingSphere();
      const mesh = new THREE.Mesh(shape, palette.get(part.material)); mesh.name = part.name;
      mesh.userData.theatreShadow = !["asphalt", "path", "green"].includes(part.material);
      mesh.castShadow = near && mesh.userData.theatreShadow; mesh.receiveShadow = true; building.add(mesh);
    }
    const cube = new THREE.BoxGeometry(1, 1, 1); shapes.push(cube);
    const transform = new THREE.Matrix4(), rotation = new THREE.Quaternion(), scale = new THREE.Vector3(), position = new THREE.Vector3();
    const up = new THREE.Vector3(0, 1, 0), a = new THREE.Vector3(), b = new THREE.Vector3(), direction = new THREE.Vector3();
    const counts = new Map<string, number>();
    for (const box of geometry.boxes) counts.set(box.material, (counts.get(box.material) ?? 0) + 1);
    for (const beam of geometry.beams) counts.set(beam.material, (counts.get(beam.material) ?? 0) + 1);
    const batches = new Map<string, { mesh: THREE.InstancedMesh; next: number }>();
    for (const [key, count] of counts) {
      const mesh = new THREE.InstancedMesh(cube, palette.get(key), count); building.add(mesh);
      mesh.userData.theatreShadow = true; mesh.castShadow = near; mesh.receiveShadow = true;
      batches.set(key, { mesh, next: 0 });
    }
    // Write each transform once into its final instance buffer, in authored order.
    const record = (material: string) => {
      const batch = batches.get(material)!;
      batch.mesh.setMatrixAt(batch.next++, transform);
    };
    for (const box of geometry.boxes) {
      rotation.setFromAxisAngle(up, box.yaw); transform.compose(position.set(box.x, box.y, box.z), rotation, scale.set(box.w, box.h, box.d)); record(box.material);
    }
    for (const beam of geometry.beams) {
      a.fromArray(beam.a); b.fromArray(beam.b);
      rotation.setFromUnitVectors(up, direction.copy(b).sub(a).normalize());
      transform.compose(position.copy(a).add(b).multiplyScalar(.5), rotation, scale.set(beam.width, a.distanceTo(b), beam.depth)); record(beam.material);
    }
    for (const { mesh } of batches.values()) { mesh.instanceMatrix.needsUpdate = true; mesh.computeBoundingSphere(); }
    if (typeof document !== "undefined") {
      canvas = document.createElement("canvas"); canvas.width = 1024; canvas.height = 64;
      const context = canvas.getContext("2d");
      if (context) {
        context.fillStyle = geometry.materials.sign; context.fillRect(0, 0, canvas.width, canvas.height);
        context.font = "600 40px sans-serif"; context.textAlign = "center"; context.textBaseline = "middle";
        context.fillStyle = geometry.materials.concrete; context.fillText("NATIONAL THEATRE", 512, 32, 980);
        signTexture = new THREE.CanvasTexture(canvas); signTexture.colorSpace = THREE.SRGBColorSpace;
        signMaterial = new THREE.MeshStandardMaterial({ map: signTexture, roughness: .86 });
        const signShape = new THREE.PlaneGeometry(17, .8); shapes.push(signShape);
        const sign = new THREE.Mesh(signShape, signMaterial); sign.position.set(recipe.center[0], 4, recipe.center[1] + 93.29); building.add(sign);
      }
    }
    return release;
  } catch (error) { release(); throw error; }
}
