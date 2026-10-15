import * as THREE from "three";
import { createTownSquareLife } from "../asset-review/townSquareLife";
import { createTownSquareNpcs } from "../asset-review/townSquareNpcs";
import type { EnvironmentAsset, EnvironmentMotion, EnvironmentParticipant } from "./environments";
import { companionTownSquareVendors, resolveCompanionTownSquareResidentBinding } from "../../../../../packages/utils/src/companionPlacesTownSquare";
import { admitCompanionTownSquareResidentBodies, companionTownSquareAgberoSourcePoint, sampleCompanionTownSquareAgberoResident,
  type CompanionTownSquareAgberoAdmission, type CompanionTownSquareAgberoResident,
  type CompanionTownSquareResidentBody } from "../../../../../packages/utils/src/companionTownSquareResidentBodies";
import { planRoomRoute, type ScenePoint } from "../../../../../packages/utils/src/companionNavigation";
import { companionWalkingBody } from "../../../../../packages/utils/src/companionWalkingNavigation";
import type { CompanionResidentWorldView } from "../../../../../packages/utils/src/companionResidentSimulation";
import type { CompanionWorldInstance } from "../../../../../packages/utils/src/companionWorld";
import type { LagosPlaceFocus } from "../lagos-places/scene";
import type { CombatFighter } from "../../../../../packages/utils/src/companionCombat";
import { companionCombatAgberoBody, companionCombatAgberoSupported } from "../../../../../packages/utils/src/companionCombatAgbero";
import type { CompanionAgberoBodyLease } from "../combat/mountCompanionAgbero";
import type { CompanionGunfireState } from "../../../../../packages/utils/src/companionGunfire";

export type TownSquareGameScope = Readonly<{ accountId: string; gameRoomId: string }>;
const validScopeId = (key: unknown): key is string => typeof key === "string"
  && key.length > 0 && key.length <= 240 && key.trim() === key;
type AgberoPresentation = NonNullable<Parameters<ReturnType<typeof createTownSquareNpcs>["setResidentAgbero"]>[0]>;

export type TownSquareRuntime = EnvironmentMotion & {
  /** Null instance returns to ordinary authored call scenery. A missing world
   * inside an admitted game hides residents instead of restoring doubles. */
  setResidents(world: CompanionResidentWorldView | null, instance: CompanionWorldInstance | null, sharedNow: number, reducedMotion?: boolean, visible?: boolean, gameScope?: TownSquareGameScope | null): void;
  /** Acquire only for an admitted fighter and the exact current instance object.
   * Null leaves the original untouched. Scope replacement revokes old leases. */
  leaseAgbero(fighter: CombatFighter, instance: CompanionWorldInstance, gameScope: TownSquareGameScope): CompanionAgberoBodyLease | null;
  resident(object: THREE.Object3D): string | null;
  focus(object: THREE.Object3D, player: ScenePoint): LagosPlaceFocus | null;
  issues(): readonly string[];
};

/** Authored scenery and read-only resident bodies. Uses the owning renderer's
 * clock and never owns player, population, inventory or call state. */
export function createTownSquareRuntime(assets: readonly EnvironmentAsset[]): TownSquareRuntime {
  if (assets.length !== 2) throw new Error("Town Square requires its district and NPC assets.");
  let participants: readonly EnvironmentParticipant[] | null = null;
  let npcs: ReturnType<typeof createTownSquareNpcs> | undefined;
  const life = createTownSquareLife(assets[0].scene, () => {
    if (!participants) throw new Error("Participant positions are unavailable.");
    return [...participants, ...(npcs?.participants() ?? [])];
  });
  try {
    npcs = createTownSquareNpcs(assets[1].scene, assets[1].animations, () =>
      participants !== null && participants.every(person => Number.isFinite(person.x) && Number.isFinite(person.z)
        && Number.isFinite(person.radius) && person.radius >= 0
        && (person.x < -8.8 || person.x > -3.65 || Math.abs(person.z + 19.2) > person.radius + .65))
      && life.vehicleFootprints().every(vehicle => Math.abs(vehicle.z + 19.2) > vehicle.radius + .65 + vehicle.speed * 1.3 || vehicle.x < -10.5));
  } catch (error) { life.dispose(); throw error; }
  let disposed = false;
  let population: CompanionResidentWorldView | null = null, instance: CompanionWorldInstance | null = null;
  let sharedNow = 0, reducedMotion = false, sharedIssue: string | null = null;
  let gunfire: CompanionGunfireState | null = null;
  let visible = true, motionEnabled = true, admitted: readonly CompanionTownSquareResidentBody[] = [];
  let agberoAdmission: CompanionTownSquareAgberoAdmission | null = null;
  let agberoResident: CompanionTownSquareAgberoResident | null = null;
  let frozenInstance: CompanionWorldInstance | null = null;
  let scope: string | null = null, epoch: string | null = null, epochConfiguration: string | null = null;
  let lastSharedNow = 0;
  let gameScope: string | null = null;
  let scopeSnapshot: (TownSquareGameScope & { key: string }) | null = null;
  let clockSnapshot: { worldKey: string; clock: CompanionResidentWorldView["clock"]; epoch: string; configuration: string } | null = null;
  // Setters copy these fields. Only this runtime owns the reusable presentation records.
  const vendorPresentation = { visible: false, sharedSeconds: 0, reducedMotion: false };
  const agberoPresentation: AgberoPresentation = { ...vendorPresentation, pose: null };
  const agberoPose: NonNullable<AgberoPresentation["pose"]> = { point: companionTownSquareAgberoSourcePoint, yaw: 0, moving: false };
  let agberoLease: CompanionAgberoBodyLease | null = null;
  const releaseAgbero = () => { agberoLease?.release(); agberoLease = null; };
  const revisions = new Map<string, { revision: number; gameMinute: number }>();
  const presented = new Map<THREE.Object3D, CompanionTownSquareResidentBody>();
  function gameScopeKey(value: TownSquareGameScope | null): string | null {
    if (!value || !validScopeId(value.accountId) || !validScopeId(value.gameRoomId)) {
      scopeSnapshot = null;
      return null;
    }
    if (!scopeSnapshot || scopeSnapshot.accountId !== value.accountId || scopeSnapshot.gameRoomId !== value.gameRoomId) {
      scopeSnapshot = { accountId: value.accountId, gameRoomId: value.gameRoomId,
        key: JSON.stringify([value.accountId, value.gameRoomId]) };
    }
    return scopeSnapshot.key;
  }
  const artIssues: string[] = [];
  for (const vendor of companionTownSquareVendors) {
    const body = npcs!.vendorBody(vendor.asset.npcId);
    const root = body?.parent;
    const origin = root?.getWorldPosition(new THREE.Vector3());
    const forward = root ? new THREE.Vector3(0, 0, 1).applyQuaternion(root.getWorldQuaternion(new THREE.Quaternion())) : null;
    const scale = root?.getWorldScale(new THREE.Vector3());
    if (!origin || !forward || !scale || ![...origin.toArray(), ...forward.toArray(), ...scale.toArray()].every(Number.isFinite)
      || Math.hypot(origin.x - vendor.asset.point[0], origin.y - vendor.asset.point[1], origin.z - vendor.asset.point[2]) > .01
      || Math.hypot(scale.x - 1, scale.y - 1, scale.z - 1) > .01
      || Math.abs(Math.atan2(forward.x, forward.z) - vendor.asset.sourceYaw) > .01) {
      artIssues.push(`The ${vendor.npc.id} body does not match the authored Town Square origin.`);
    }
  }
  const agberoRoot = npcs!.agberoBody()?.parent;
  const agberoOrigin = agberoRoot?.getWorldPosition(new THREE.Vector3());
  const agberoForward = agberoRoot ? new THREE.Vector3(0, 0, 1).applyQuaternion(agberoRoot.getWorldQuaternion(new THREE.Quaternion())) : null;
  const agberoScale = agberoRoot?.getWorldScale(new THREE.Vector3());
  const agberoArtIssue = !agberoOrigin || !agberoForward || !agberoScale
    || ![...agberoOrigin.toArray(), ...agberoForward.toArray(), ...agberoScale.toArray()].every(Number.isFinite)
    || Math.hypot(...agberoOrigin.toArray().map((value, index) => value - companionTownSquareAgberoSourcePoint[index])) > .01
    || Math.hypot(agberoScale.x - 1, agberoScale.y - 1, agberoScale.z - 1) > .01
    || Math.abs(Math.atan2(agberoForward.x, agberoForward.z) - companionCombatAgberoBody.sourceYaw) > .01
    ? "The Ganiu Ojo body does not match the authored Town Square origin." : null;
  function updateResidents(): boolean {
    presented.clear();
    agberoResident = null;
    agberoPresentation.pose = null;
    agberoPose.point = companionTownSquareAgberoSourcePoint;
    let changed = false;
    if (!instance) {
      for (const vendor of companionTownSquareVendors) changed = npcs!.setResidentVendor(vendor.asset.npcId, null) || changed;
      changed = npcs!.setResidentAgbero(null) || changed;
      return changed;
    }
    const showResidents = visible && population !== null && !sharedIssue && !artIssues.length;
    const seconds = showResidents && population ? (sharedNow - population.clock.epochRealMs) / 1000 : 0;
    vendorPresentation.sharedSeconds = agberoPresentation.sharedSeconds = seconds;
    vendorPresentation.reducedMotion = agberoPresentation.reducedMotion = reducedMotion || !motionEnabled;
    for (const vendor of companionTownSquareVendors) {
      const resident = showResidents ? admitted.find(value => value.npcId === vendor.npc.id) : undefined;
      const show = !!resident?.visible;
      vendorPresentation.visible = show;
      changed = npcs!.setResidentVendor(vendor.asset.npcId, vendorPresentation) || changed;
      if (show && resident) {
        const body = npcs!.vendorBody(vendor.asset.npcId);
        if (body) presented.set(body, resident);
      }
    }
    agberoResident = showResidents && agberoAdmission && !agberoArtIssue
      ? sampleCompanionTownSquareAgberoResident(agberoAdmission, instance, sharedNow) : null;
    agberoPresentation.visible = !!agberoResident?.visible;
    if (agberoResident?.pose) {
      agberoPose.point = agberoResident.pose.point; agberoPose.yaw = agberoResident.pose.yaw; agberoPose.moving = agberoResident.moving;
      agberoPresentation.pose = agberoPose;
    }
    changed = npcs!.setResidentAgbero(agberoPresentation) || changed;
    if (agberoResident?.visible) {
      const body = npcs!.agberoBody();
      if (body) presented.set(body, agberoResident);
    }
    return changed;
  }
  function picked(object: THREE.Object3D) {
    if (disposed || !visible || !instance || !population || sharedIssue) return null;
    let found: CompanionTownSquareResidentBody | undefined, attached = false;
    for (let node: THREE.Object3D | null = object; node; node = node.parent) {
      if (!node.visible) return null;
      if (node === assets[1].scene) attached = true;
      if (!attached && !found) found = presented.get(node);
    }
    return attached ? found ?? null : null;
  }
  return {
    walkingObstacles() {
      if (disposed || !visible) return [];
      return [...life.vehicleFootprints().map(companionWalkingBody), ...npcs!.participants(false).map(person =>
        companionWalkingBody({ ...person, yaw: 0, halfWidth: person.radius, halfLength: person.radius }))];
    },
    update(dt, nextParticipants, enabled) {
      if (disposed) return false;
      participants = nextParticipants;
      motionEnabled = enabled;
      const sceneryMotion = enabled && visible && !reducedMotion;
      life.setEnabled(sceneryMotion); npcs!.setEnabled(sceneryMotion);
      const residentChanged = updateResidents();
      npcs!.setGunfire(visible ? gunfire : null, instance?.navigation ?? null, sharedNow);
      // The NPC reserves its road crossing before traffic reads the footprints.
      const changed = npcs!.update(dt);
      return life.update(dt) || changed || residentChanged;
    },
    setStreetCrashes(view, nextScope) { return life.setStreetCrashes(view, nextScope); },
    setGunfire(state) { if (!disposed) gunfire = state; },
    setStreetPedestrians(samples, expectedInstance, now) {
      if (disposed) return;
      if (!expectedInstance) { npcs!.setStreetPedestrians([], null, now, null); return; }
      if (!instance || expectedInstance.id !== instance.id || expectedInstance.sharedStartedAt !== instance.sharedStartedAt
        || expectedInstance.navigation.revision !== instance.navigation.revision || now !== sharedNow || !gameScope || sharedIssue) return;
      npcs!.setStreetPedestrians(samples, instance.navigation, now, instance.id);
    },
    setResidents(world, nextInstance, now, nextReducedMotion = false, nextVisible = true, nextGameScope: TownSquareGameScope | null = null) {
      if (disposed) return;
      let snapshotChanged = population !== world || instance !== nextInstance;
      population = world; instance = nextInstance;
      sharedNow = now; reducedMotion = nextReducedMotion; visible = nextVisible;
      const nextScope = nextInstance?.id ?? null;
      const nextGameScopeKey = gameScopeKey(nextGameScope);
      if (nextScope !== scope || nextGameScopeKey !== gameScope) {
        npcs!.setGunfire(null, null, now); npcs!.reset();
        snapshotChanged = true;
        gameScope = nextGameScopeKey;
        scope = nextScope; epoch = null; epochConfiguration = null; clockSnapshot = null; lastSharedNow = 0;
        frozenInstance = null; admitted = []; agberoAdmission = null; sharedIssue = null; revisions.clear();
        updateResidents(); releaseAgbero();
      }
      const reject = (issue: string) => { sharedIssue = issue; admitted = []; agberoAdmission = null; updateResidents(); releaseAgbero(); };
      if (!nextInstance) { clockSnapshot = null; releaseAgbero(); updateResidents(); return; }
      if (!Number.isSafeInteger(now) || now < lastSharedNow) { reject("Use a nondecreasing shared clock for Town Square."); return; }
      lastSharedNow = now;
      if (!world) { clockSnapshot = null; admitted = []; agberoAdmission = null; updateResidents(); releaseAgbero(); return; }
      const clock = world.clock;
      // Compare copied scalar fields, not snapshot identity. Changed fields still use
      // the original serialization, including JSON's numeric equivalences.
      const savedClock = clockSnapshot?.clock;
      if (!clock) clockSnapshot = null;
      else if (!clockSnapshot || !savedClock || clockSnapshot.worldKey !== world.worldKey || savedClock.worldKey !== clock.worldKey
        || savedClock.epochKey !== clock.epochKey || !Object.is(savedClock.version, clock.version)
        || !Object.is(savedClock.epochRealMs, clock.epochRealMs) || !Object.is(savedClock.epochGameMinute, clock.epochGameMinute)
        || !Object.is(savedClock.realMsPerGameMinute, clock.realMsPerGameMinute)) {
        clockSnapshot = { worldKey: world.worldKey, clock: { version: clock.version, worldKey: clock.worldKey,
          epochKey: clock.epochKey, epochRealMs: clock.epochRealMs, epochGameMinute: clock.epochGameMinute,
          realMsPerGameMinute: clock.realMsPerGameMinute }, epoch: `${world.worldKey}:${clock.epochKey}`,
          configuration: JSON.stringify([clock.version, clock.worldKey, clock.epochKey, clock.epochRealMs,
            clock.epochGameMinute, clock.realMsPerGameMinute]) };
      }
      const nextEpoch = clockSnapshot?.epoch ?? null;
      const configuration = clockSnapshot?.configuration ?? null;
      if (epoch !== null && (nextEpoch !== epoch || configuration !== epochConfiguration)) {
        reject("Reload Town Square after the resident world epoch changes."); return;
      }
      epoch = nextEpoch; epochConfiguration = configuration;
      if (snapshotChanged) {
        if (frozenInstance && (frozenInstance.sharedStartedAt !== nextInstance.sharedStartedAt
          || !resolveCompanionTownSquareResidentBinding(frozenInstance, nextInstance))) {
          reject("Reload Town Square after its frozen instance changes."); return;
        }
        const result = admitCompanionTownSquareResidentBodies(world, nextInstance, now);
        if (result.error) { reject(result.error); return; }
        const nextAgbero = result.agbero ? sampleCompanionTownSquareAgberoResident(result.agbero, nextInstance, now) : null;
        const nextResidents: readonly CompanionTownSquareResidentBody[] = nextAgbero ? [...result.residents, nextAgbero] : result.residents;
        if (nextResidents.some(resident => {
          const previous = revisions.get(resident.residentId);
          return previous && (resident.revision < previous.revision || resident.gameMinute < previous.gameMinute);
        })) { reject("Reload the current Town Square resident snapshot."); return; }
        frozenInstance ??= nextInstance; admitted = nextResidents; agberoAdmission = result.agbero; sharedIssue = null;
        for (const resident of admitted) revisions.set(resident.residentId, { revision: resident.revision, gameMinute: resident.gameMinute });
      }
      updateResidents();
    },
    leaseAgbero(fighter, expectedInstance, expectedGameScope) {
      if (disposed || !visible || !population || sharedIssue || !frozenInstance || !instance
        || artIssues.length || agberoArtIssue || !agberoResident?.visible
        || expectedInstance !== instance || !gameScope || gameScopeKey(expectedGameScope) !== gameScope
        || !epoch || !companionCombatAgberoSupported(fighter) || typeof fighter.id !== "string"
        || !fighter.id.length || fighter.id.length > 240 || fighter.id.trim() !== fighter.id
        || fighter.status !== "active" && fighter.status !== "defeated"
        || fighter.id !== agberoResident.residentId && fighter.id !== `agbero:${fighter.targetId}`) return null;
      if (agberoLease?.current()) return null;
      releaseAgbero();
      const body = npcs!.leaseAgbero();
      if (!body) return null;
      const claimScope = gameScope, claimEpoch = epoch, instanceId = instance.id, startedAt = instance.sharedStartedAt;
      const claimInstance = frozenInstance;
      let released = false;
      const lease: CompanionAgberoBodyLease = {
        npcId: companionCombatAgberoBody.npcId, instanceId, fighterId: fighter.id,
        current() {
          if (released || agberoLease !== lease) return false;
          if (disposed || gameScope !== claimScope || epoch !== claimEpoch || !population || sharedIssue
            || frozenInstance !== claimInstance || instance?.id !== instanceId || instance.sharedStartedAt !== startedAt || !body.current()) {
            lease.release(); return false;
          }
          return true;
        },
        release() {
          if (released) return;
          released = true;
          body.release();
          if (agberoLease === lease) agberoLease = null;
        },
      };
      agberoLease = lease;
      return lease;
    },
    resident(object) { return picked(object)?.residentId ?? null; },
    focus(object, player) {
      const resident = picked(object);
      if (!resident?.available || !instance) return null;
      const vendor = companionTownSquareVendors.find(value => value.npc.id === resident.npcId);
      if (!vendor || npcs!.reacting(vendor.asset.npcId) || npcs!.streetOwned(vendor.asset.npcId)) return null;
      return { id: vendor.service.id, kind: "service", label: vendor.service.label, point: vendor.service.approach.point,
        reachable: !!planRoomRoute(instance.navigation, player, vendor.service.approach.point) };
    },
    issues: () => [...npcs!.issues(), ...life.streetIssues(), ...artIssues, ...(agberoArtIssue ? [agberoArtIssue] : []), ...(sharedIssue ? [sharedIssue] : [])],
    dispose() {
      if (disposed) return;
      releaseAgbero(); gameScope = null;
      disposed = true; npcs!.dispose(); life.dispose(); participants = null; population = null; instance = null;
      frozenInstance = null; admitted = []; agberoAdmission = null; agberoResident = null; presented.clear(); revisions.clear();
      gunfire = null; scopeSnapshot = null; clockSnapshot = null; scope = epoch = epochConfiguration = null;
      agberoPresentation.pose = null; agberoPose.point = companionTownSquareAgberoSourcePoint;
    },
  };
}
