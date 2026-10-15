import * as THREE from "three";
import { makeStudio, makeRoomObject } from "../room/studioGeometry";
import type { RoomLayout, RoomObstacle } from "../room/placement";
import { roomNavigationFor } from "../../../../../packages/utils/src/companionNavigation";
export { legacyRoomNavigation } from "../../../../../packages/utils/src/companionNavigation";
import type { RoomNavigation } from "./navigation";
import { makeTerrace } from "./terraceGeometry";
import { companionAssetEntries, type CompanionAssetKey } from "../../../../../packages/utils/src/companionAssets";
import { townSquareNavigation, townSquareStanding } from "../../../../../packages/utils/src/companionTownSquare";
import { createTownSquareRuntime } from "./townSquareRuntime";
import type { CompanionResidentWorldView } from "../../../../../packages/utils/src/companionResidentSimulation";
import type { CompanionWorldInstance } from "../../../../../packages/utils/src/companionWorld";
import type { LagosPlaceFocus } from "../lagos-places/scene";
import type { ScenePowerBinding } from "./scenePower";
import type { TownSquareGameScope } from "./townSquareRuntime";
import type { CompanionAgberoBodyLease } from "../combat/mountCompanionAgbero";
import type { CombatFighter } from "../../../../../packages/utils/src/companionCombat";
import type { CompanionStreetCrashScope } from "../../../../../packages/utils/src/companionStreetCrashes";
import type { CompanionStreetCrashTrafficView } from "../../../../../packages/utils/src/companionStreetCrashTraffic";
import type { CompanionGunfireState } from "../../../../../packages/utils/src/companionGunfire";
import type { CompanionPedestrianRouteSample } from "../../../../../packages/utils/src/companionStreetPedestrians";

export type ScenePoint = readonly [number, number, number];
export type EnvironmentParticipant = { x: number; z: number; radius: number };
export type EnvironmentAsset = { scene: THREE.Group; animations: THREE.AnimationClip[] };
export type EnvironmentMotion = {
  update(dt: number, participants: readonly EnvironmentParticipant[] | null, enabled: boolean): boolean;
  walkingObstacles?(): readonly import("./navigation").NavigationObstacle[];
  setStreetCrashes?(view: CompanionStreetCrashTrafficView | null, scope: CompanionStreetCrashScope | null): boolean;
  setGunfire?(state: CompanionGunfireState | null): void;
  setStreetPedestrians?(samples: readonly CompanionPedestrianRouteSample[], instance: CompanionWorldInstance | null, sharedNow: number): void;
  setResidents?(world: CompanionResidentWorldView | null, instance: CompanionWorldInstance | null, sharedNow: number, reducedMotion?: boolean, visible?: boolean, gameScope?: TownSquareGameScope | null): void;
  leaseAgbero?(fighter: CombatFighter, instance: CompanionWorldInstance, gameScope: TownSquareGameScope): CompanionAgberoBodyLease | null;
  resident?(object: THREE.Object3D): string | null;
  focus?(object: THREE.Object3D, player: ScenePoint): LagosPlaceFocus | null;
  issues?(): readonly string[];
  dispose(): void;
};
export interface CompanionEnvironment {
  powerBindings?: (group: THREE.Group) => readonly ScenePowerBinding[];
  id: string;
  instanceId?: string;
  name: string;
  background: string;
  lighting?: "daylight" | "golden" | "evening";
  characterScale: number;
  wide: { eye: ScenePoint; target: ScenePoint };
  close: { eye: ScenePoint; target: ScenePoint };
  /** Solo portrait framing keeps the character above the lower room controls. */
  portraitWide?: { eye: ScenePoint; target: ScenePoint };
  walkingPoints: readonly ScenePoint[];
  spawn?: ScenePoint;
  spawnYaw?: number;
  layout?: RoomLayout;
  /** Fixed furnishings plus currently placed decor at their real support heights. */
  cameraObstacles?: readonly RoomObstacle[];
  fov?: number;
  cameraOrbitLimit?: number;
  cameraHeightMax?: number;
  /** Game spaces follow the player; Terrace retains its open room orbit. */
  cameraPolicy?: "open-orbit" | "third-person";
  navigation?: RoomNavigation;
  /** Set only with the matching authored layout, navigation and support surfaces. */
  authored?: {
    key: CompanionAssetKey;
    overlays?: readonly CompanionAssetKey[];
    configure?: (group: THREE.Group) => void | (() => void);
    /** Configured source nodes keep fixed local poses. Added props and scene roots stay automatic. */
    fixedSourceTransforms?: boolean;
    animate?: (assets: readonly EnvironmentAsset[]) => EnvironmentMotion;
  };
  create: () => THREE.Group;
}

const studio: CompanionEnvironment = {
  id: "studio", name: "The Nook", background: "#d9ccba", characterScale: .57,
  wide: { eye: [5.8, 5.2, 12.8], target: [0, 1, 0] },
  close: { eye: [1.6, 2.5, 6.5], target: [0, 1.2, .7] },
  // The front floor strip is clear of the room editor's furniture surfaces.
  walkingPoints: [[-.8, 0, 1.15], [.8, 0, 1.15], [0, 0, .6]],
  navigation: roomNavigationFor(),
  create: () => {
    const group = new THREE.Group();
    const { enclosure, furniture } = makeStudio();
    group.add(enclosure, furniture);
    const plant = makeRoomObject("plant"); plant.position.set(1.9, 0, -.15); group.add(plant);
    return group;
  },
};

const rooftop: CompanionEnvironment = {
  id: "terrace", name: "Sky Terrace", background: "#e9d7c6", characterScale: .57,
  cameraPolicy: "open-orbit", fov: 42,
  wide: { eye: [5.2, 5.4, 13.8], target: [0, .7, 0] },
  close: { eye: [1.8, 2.7, 6.8], target: [0, 1.1, .5] },
  walkingPoints: [[-.8, 0, 1.4], [.8, 0, 1.4], [0, 0, .6]], spawn: [0, 0, .6], create: () => makeTerrace(),
  navigation: roomNavigationFor(undefined, "terrace"),
  authored: companionAssetEntries.bestieTerrace.published ? { key: "bestieTerrace", fixedSourceTransforms: true, configure(group) {
    group.traverse(node => { if (node instanceof THREE.Mesh) { node.castShadow = false; node.receiveShadow = false; } });
  } } : undefined,
};

const townSquare: CompanionEnvironment = {
  id: "town-square", name: "Town Square", background: "#b9ccdc", characterScale: .57,
  cameraPolicy: "third-person", fov: 72,
  wide: { eye: [-6.5, 2.4, 31], target: [-10, 1.8, -22] },
  portraitWide: { eye: [-5.2, 2.2, 19], target: [-10, 1.6, -24] },
  close: { eye: [1.5, 2.1, 8], target: [0, 1.1, 4] },
  spawn: [0, 0, 4], spawnYaw: Math.PI, cameraHeightMax: 6,
  walkingPoints: townSquareStanding.map(p => [p.x, 0, p.z] as const),
  navigation: townSquareNavigation,
  cameraObstacles: townSquareNavigation.obstacles.map((o, i) => ({ id: `square-obstacle-${i}`, x: (o.minX + o.maxX) / 2, z: (o.minZ + o.maxZ) / 2, width: o.maxX - o.minX, depth: o.maxZ - o.minZ, height: 1.4 })),
  authored: {
    key: "bestieTownSquare", overlays: ["bestieTownSquareNpcs"], animate: createTownSquareRuntime,
    configure(group) {
      group.traverse(node => { if (node instanceof THREE.Mesh) { node.castShadow = false; node.receiveShadow = false; } });
    },
  },
  create: () => new THREE.Group(),
};

export const companionEnvironments = [studio, rooftop] as const;
export const companionCallEnvironments = [...companionEnvironments, townSquare] as const;

export function usesBoundedCamera(world?: CompanionEnvironment) {
  return world?.cameraPolicy === "third-person" || (!!world?.authored && world.cameraPolicy !== "open-orbit");
}

export function disposeEnvironment(group: THREE.Group) {
  const geometries = new Set<THREE.BufferGeometry>(), materials = new Set<THREE.Material>();
  group.traverse(object => {
    if (!(object instanceof THREE.Mesh)) return;
    geometries.add(object.geometry);
    (Array.isArray(object.material) ? object.material : [object.material]).forEach(material => materials.add(material));
  });
  geometries.forEach(geometry => geometry.dispose()); materials.forEach(material => material.dispose());
}
