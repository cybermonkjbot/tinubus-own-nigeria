import * as THREE from "three";
import type { PersonalRoom } from "../../../../../packages/utils/src/companion";
import { companionHomeBox, companionHomePoint, companionHomeShellZ, companionHomeYaw } from "../../../../../packages/utils/src/companionHomeDesign";
import type { CompanionEnvironment } from "./environments";

export function homeDesignEnvironment(base: CompanionEnvironment, room: PersonalRoom): CompanionEnvironment {
  if (!room.homeDesign) return base;
  // Keep the camera near the cutaway edge so additional floor space is visible.
  const camera = (view: CompanionEnvironment["wide"]) => ({ ...view,
    eye: companionHomePoint(room, [view.eye[0], view.eye[1], view.eye[2] + room.homeDesign!.extension]),
    target: companionHomePoint(room, view.target) });
  return { ...base, wide: camera(base.wide), close: camera(base.close),
    ...(base.portraitWide ? { portraitWide: camera(base.portraitWide) } : {}),
    ...(base.spawn ? { spawn: companionHomePoint(room, base.spawn) } : {}),
    spawnYaw: (base.spawnYaw ?? 0) + companionHomeYaw(room),
    ...(base.cameraObstacles ? { cameraObstacles: base.cameraObstacles.map(box => companionHomeBox(room, box)) } : {}) };
}

/** Source geometry stays shared unless the empty floor strip needs more depth. */
export function configureHomeDesign(group: THREE.Group, room: PersonalRoom): () => void {
  const owned: { mesh: THREE.Mesh; original: THREE.BufferGeometry; geometry: THREE.BufferGeometry }[] = [];
  const restore = () => { for (const { mesh, original, geometry } of owned) { mesh.geometry = original; geometry.dispose(); } owned.length = 0; };
  if (!room.homeDesign) return restore;
  try {
    if (room.homeDesign.extension) {
      group.updateWorldMatrix(true, true);
      const rootInverse = group.matrixWorld.clone().invert();
      const point = new THREE.Vector3();
      group.traverse(node => {
        if (!(node instanceof THREE.Mesh)) return;
        const toRoom = rootInverse.clone().multiply(node.matrixWorld), fromRoom = toRoom.clone().invert();
        const original = node.geometry, source = original.getAttribute("position");
        if (!source) return;
        // Neither room has furniture beyond z=1.9m. Floor, tiles and the cutaway
        // side wall extend together; seats, baths and counters do not stretch.
        let geometry: THREE.BufferGeometry | undefined;
        for (let i = 0; i < source.count; i++) {
          point.fromBufferAttribute(source, i).applyMatrix4(toRoom);
          const z = companionHomeShellZ(room, point.z);
          if (z === point.z) continue;
          if (!geometry) { geometry = original.clone(); owned.push({ mesh: node, original, geometry }); node.geometry = geometry; }
          point.z = z; point.applyMatrix4(fromRoom);
          geometry.getAttribute("position").setXYZ(i, point.x, point.y, point.z);
        }
        if (geometry) {
          geometry.getAttribute("position").needsUpdate = true;
          geometry.computeVertexNormals(); geometry.computeBoundingBox(); geometry.computeBoundingSphere();
        }
      });
    }
    const rotation = new THREE.Matrix4().makeRotationY(companionHomeYaw(room));
    // Keep the root in world coordinates. Saved decor is added after this step.
    for (const child of group.children) child.applyMatrix4(rotation);
    group.updateWorldMatrix(true, true);
    return restore;
  } catch (error) { restore(); throw error; }
}
