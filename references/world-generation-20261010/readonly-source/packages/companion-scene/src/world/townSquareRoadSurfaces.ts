import * as THREE from "three";
import type { LagosSurface } from "./LagosScatter";
import { castVehicleRoadPlane, type VehicleGroundCast } from "../../../../packages/utils/src/companionVehicleDynamics";

// Authored roadwear.py centres/radii, converted from Blender [x,y,z] to runtime [x,z,-y].
// These sample existing bowls only. They do not add collision or alter the walking floor.
const potholes = [
  { x: -10.5, z: 15, rx: .7, rz: 1.02 }, { x: -8.1, z: 10.6, rx: .58, rz: .82 },
  { x: -12.55, z: 18.9, rx: .46, rz: .7 }, { x: -8.4, z: 4.2, rx: .41, rz: .62 },
  { x: -10.6, z: -2.6, rx: .69, rz: .91 },
];
const tags = new Set<LagosSurface>(["clean_asphalt", "pothole", "laterite_mud", "gutter_wood"]);

function potholeAt(x: number, z: number) {
  for (const patch of potholes)
    if (((x - patch.x) / patch.rx) ** 2 + ((z - patch.z) / patch.rz) ** 2 < 1) return true;
  return false;
}

export function createTownSquareRoadSurfaces(model: THREE.Group) {
  const ray = new THREE.Raycaster(), origin = new THREE.Vector3(), down = new THREE.Vector3(0, -1, 0);
  const meshes: THREE.Object3D[] = [];
  const intersections: THREE.Intersection[] = [], normalMatrix = new THREE.Matrix3(), normal = new THREE.Vector3();
  let authoredRoadwear = false;
  model.traverse(node => {
    if (/Road[ _]wear[ _]v05/.test(node.name)) authoredRoadwear = true;
    if (tags.has(node.userData.surface) && meshes.length < 64) meshes.push(node);
  });
  const visibleHit = (hit: THREE.Intersection) => {
    for (let node: THREE.Object3D | null = hit.object; node; node = node.parent) if (!node.visible) return false;
    return true;
  };
  const groundHit = (hit: THREE.Intersection) => {
    for (let node: THREE.Object3D | null = hit.object; node; node = node.parent)
      if (!node.visible || node.userData.lifeRole === "vehicle") return false;
    return !!hit.face;
  };
  function sample(wheel: THREE.Vector3): LagosSurface {
    origin.copy(wheel); origin.y += 1; down.set(0, -1, 0); ray.set(origin, down); ray.far = 2;
    intersections.length = 0;
    try {
      const hit = meshes.length ? ray.intersectObjects(meshes, true, intersections).find(visibleHit) : undefined;
      for (let node: THREE.Object3D | null = hit?.object ?? null; node; node = node.parent)
        if (tags.has(node.userData.surface)) return node.userData.surface;
      return authoredRoadwear && potholeAt(wheel.x, wheel.z) ? "pothole" : "clean_asphalt";
    } finally {
      // Completed queries must not retain hit meshes and their parent graphs.
      intersections.length = 0;
    }
  }
  return Object.assign(sample, {
    cast(floor: number): VehicleGroundCast {
      return (point, direction, length) => {
        origin.set(point.x, point.y, point.z); down.set(direction.x, direction.y, direction.z);
        ray.set(origin, down); ray.near = 0; ray.far = length; intersections.length = 0;
        try {
          ray.intersectObjects(meshes, true, intersections);
          const hit = intersections.find(groundHit);
          if (hit?.face) {
            normal.copy(hit.face.normal).applyNormalMatrix(normalMatrix.getNormalMatrix(hit.object.matrixWorld));
            let surface: LagosSurface = "clean_asphalt";
            for (let node: THREE.Object3D | null = hit.object; node; node = node.parent)
              if (tags.has(node.userData.surface)) { surface = node.userData.surface; break; }
            return { distance: hit.distance, normal: { x: normal.x, y: normal.y, z: normal.z }, velocity: { x: 0, y: 0, z: 0 }, surface };
          }
          // The admitted road rectangles retain their existing flat contact plane.
          // Painted roadwear can change grip but does not invent a depression in that plane.
          const surface = authoredRoadwear && potholeAt(point.x, point.z) ? "pothole" : "clean_asphalt";
          return castVehicleRoadPlane(point, direction, length, floor, surface);
        } finally {
          intersections.length = 0;
        }
      };
    },
  });
}
