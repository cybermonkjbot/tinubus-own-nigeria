import * as THREE from "three";
import { companionRoadsideEndpoint } from "../../../../../packages/utils/src/companionRoadside";
import type { CompanionWorldInstance } from "../../../../../packages/utils/src/companionWorld";
import type { CompanionEnvironment } from "../scenes/environments";

/** The saved road point is local origin. No nearby venue or street reconstruction. */
export function roadsideEnvironment(instance: CompanionWorldInstance): CompanionEnvironment {
  if (!companionRoadsideEndpoint(instance)) throw new Error("Use an admitted roadside visit.");
  return {
    id: `roadside-view:${instance.id}`, instanceId: instance.id, name: "Roadside",
    background: "#b9ccdc", characterScale: .57, cameraPolicy: "third-person", fov: 72,
    wide: { eye: [7, 5.5, 9], target: [0, 1, 0] },
    close: { eye: [1.8, 2.6, 6], target: [0, 1.1, 0] },
    walkingPoints: instance.spawns, spawn: instance.spawns[0], navigation: instance.navigation,
    create() {
      const group = new THREE.Group();
      const bounds = instance.navigation.bounds;
      const ground = new THREE.Mesh(new THREE.PlaneGeometry(bounds.maxX - bounds.minX, bounds.maxZ - bounds.minZ),
        new THREE.MeshStandardMaterial({ color: "#666561", roughness: .95 }));
      ground.name = "Roadside ground";
      ground.rotation.x = -Math.PI / 2;
      ground.position.set((bounds.minX + bounds.maxX) / 2, instance.navigation.floorY, (bounds.minZ + bounds.maxZ) / 2);
      ground.receiveShadow = true;
      group.add(ground);
      return group;
    },
  };
}
