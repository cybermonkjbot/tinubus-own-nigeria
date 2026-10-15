import { companionAssetEntries } from "./companionAssets";
import { companionCombatAgberoBody } from "./companionCombatAgbero";
import { CALL_MOVEMENT_MAX_POINTS } from "./companionMovement";
import { routeSegmentClear } from "./companionNavigation";
import { sampleCompanionTownSquareResidents, type CompanionTownSquareResident } from "./companionPlacesTownSquare";
import { sampleCompanionResidentInPlace, type CompanionResidentPlaceBinding, type CompanionResidentView,
  type CompanionResidentWorldView } from "./companionResidentSimulation";
import { companionPlaceKinds } from "./companionPlaces";
import { createCompanionResidentIdentity } from "./companionResidents";
import type { CompanionWorldInstance } from "./companionWorld";

/** Registered v07 source origin, separate from the resident's playable work point. */
export const companionTownSquareAgberoSourcePoint = [-4.3, 0, -5] as const;

export type CompanionTownSquareAgberoAdmission = Readonly<{
  resident: CompanionResidentView;
  binding: CompanionResidentPlaceBinding;
}>;
export type CompanionTownSquareAgberoResident = Readonly<{
  residentId: string; revision: number; gameMinute: number;
  npcId: typeof companionCombatAgberoBody.npcId; serviceId: null;
  visible: boolean; available: boolean;
  pose: NonNullable<ReturnType<typeof sampleCompanionResidentInPlace>>["pose"] | null;
  moving: boolean;
}>;
export type CompanionTownSquareResidentBody = CompanionTownSquareResident | CompanionTownSquareAgberoResident;

/** Admit the existing shirt body from the same world and frozen binding as vendors. */
export function admitCompanionTownSquareResidentBodies(world: CompanionResidentWorldView, instance: CompanionWorldInstance,
  sharedNow: number): { residents: readonly CompanionTownSquareResident[]; agbero: CompanionTownSquareAgberoAdmission | null; error: string | null } {
  const vendors = sampleCompanionTownSquareResidents(world, instance, sharedNow);
  const invalid = (error: string) => ({ residents: Object.freeze([]) as readonly CompanionTownSquareResident[], agbero: null, error });
  if (vendors.error) return invalid(vendors.error);
  const resident = world.residents.find(value => value.identity.actorKey === "area-tout");
  if (!resident) return { ...vendors, agbero: null };
  let expected: ReturnType<typeof createCompanionResidentIdentity>;
  try { expected = createCompanionResidentIdentity(world.worldKey, "area-tout"); }
  catch { return invalid("Use the canonical Town Square resident world."); }
  const identity = resident.identity, art = resident.renderAsset;
  if (identity.id !== expected.id || identity.name !== expected.name || identity.role !== expected.role
    || identity.occupation !== expected.occupation || !identity.appearance || identity.appearance.seed !== expected.appearance.seed
    || identity.appearance.assetNpcId !== companionCombatAgberoBody.npcId
    || !Array.isArray(identity.personality) || identity.personality.length !== expected.personality.length
    || identity.personality.some((trait, index) => trait !== expected.personality[index])
    || resident.serviceActor !== null || !art || art.key !== "bestieTownSquareNpcs"
    || art.npcId !== companionCombatAgberoBody.npcId || art.sourceYaw !== companionCombatAgberoBody.sourceYaw
    || !companionAssetEntries.bestieTownSquareNpcs.published)
    return invalid("Bind Ganiu Ojo to his canonical Town Square body.");
  const binding = world.placeBindings!.find(value => value.placeId === "town-square")!;
  const activity = resident.activity, health = resident.health;
  const residentHome = `resident-home:${expected.actorKey}`;
  const validKey = (value: unknown) => typeof value === "string" && value.length > 0 && value.length <= 240 && value.trim() === value;
  const validPoint = (value: unknown) => Array.isArray(value) && value.length === 3 && value.every(Number.isFinite);
  const validPlace = (value: { placeId: string; instanceId: string } | null | undefined) => !!value
    && validKey(value.placeId) && validKey(value.instanceId)
    && (value.placeId === "town-square" ? value.instanceId === binding.residentInstanceId
      // NPC commutes use their own frozen home, never the player's Home or another resident's.
      : value.placeId === residentHome ? value.instanceId === `${world.worldKey}:${residentHome}`
        : companionPlaceKinds.some(kind => kind === value.placeId));
  if (!health || !Number.isFinite(health.health) || !Number.isSafeInteger(health.maxHealth)
    || health.maxHealth < 1 || health.maxHealth > 100 || health.health < 0 || health.health > health.maxHealth
    || !activity || !Number.isSafeInteger(activity.sinceGameMinute) || activity.sinceGameMinute < 0
    || activity.sinceGameMinute > resident.updatedAtGameMinute)
    return invalid("Use Ganiu Ojo's current health and activity.");
  // Validate location independently of visibility. Zero health may accompany
  // hospital recovery, but cannot turn malformed street state into absence.
  const checkedResident = health.health > 0 ? resident : { ...resident, health: { ...health, health: 1 } };
  if (activity.kind === "at-place") {
    const location = activity.location;
    if (!location || !validPlace(location) || !location.pose
      || !validPoint(location.pose.point) || !Number.isFinite(location.pose.yaw) || Math.abs(location.pose.yaw) > Math.PI
      || !["rest", "work", "eat", "socialize", "recover"].includes(activity.action)
      || activity.interactionId !== null && !validKey(activity.interactionId) || resident.journey !== null)
      return invalid("Use Ganiu Ojo's accepted place activity.");
    const locationBinding = location.placeId === "town-square" ? binding : {
      placeId: location.placeId, residentInstanceId: location.instanceId, gameInstanceId: instance.id, layoutId: instance.layoutId,
    };
    const routeTime = resident.localMove?.movement?.startedAt;
    const sampleAt = typeof routeTime === "number" && Number.isFinite(routeTime) ? Math.max(sharedNow, routeTime) : sharedNow;
    if (!sampleCompanionResidentInPlace(checkedResident, locationBinding, sampleAt)
      || location.placeId === "town-square" && !routeSegmentClear(instance.navigation, location.pose.point, location.pose.point))
      return invalid("Use Ganiu Ojo's accepted place pose and route.");
  } else if (activity.kind === "traveling") {
    const journey = resident.journey;
    if (!journey || !validPlace(journey.from) || !validPlace(journey.to) || !journey.departure || !journey.arrival)
      return invalid("Use Ganiu Ojo's accepted journey.");
    const departureBinding = { placeId: journey.from.placeId, residentInstanceId: journey.from.instanceId,
      gameInstanceId: instance.id, layoutId: instance.layoutId };
    if (!sampleCompanionResidentInPlace(checkedResident, departureBinding, journey.departure.startedAt))
      return invalid("Use Ganiu Ojo's accepted journey activity.");
  } else return invalid("Use Ganiu Ojo's accepted activity.");
  if (resident.localMove && resident.activity?.kind === "at-place"
    && resident.activity.location?.placeId === binding.placeId && resident.activity.location.instanceId === binding.residentInstanceId) {
    const movement = resident.localMove.movement;
    if (!movement || !Array.isArray(movement.path) || movement.path.length > CALL_MOVEMENT_MAX_POINTS)
      return invalid("Use Ganiu Ojo's accepted Town Square route.");
    let previous = resident.activity.location?.pose?.point;
    if (!Array.isArray(previous) || previous.length !== 3 || !previous.every(Number.isFinite))
      return invalid("Use Ganiu Ojo's accepted Town Square pose.");
    for (const point of movement.path) {
      if (!point) return invalid("Use Ganiu Ojo's accepted Town Square route.");
      const next = [point.x, previous[1], point.z] as const;
      if (!routeSegmentClear(instance.navigation, previous, next)) return invalid("Use Ganiu Ojo's clear Town Square route.");
      previous = next;
    }
  }
  return { ...vendors, agbero: Object.freeze({ resident, binding }) };
}

/** Sample accepted local movement only. Journeys and recovery never show a street double. */
export function sampleCompanionTownSquareAgberoResident(admission: CompanionTownSquareAgberoAdmission,
  instance: CompanionWorldInstance, sharedNow: number): CompanionTownSquareAgberoResident {
  const { resident, binding } = admission;
  const activity = resident.activity;
  const sampled = activity?.kind === "at-place" && activity.action !== "recover" && resident.journey === null
    ? sampleCompanionResidentInPlace(resident, binding, sharedNow) : null;
  const visible = !!sampled && binding.gameInstanceId === instance.id && binding.layoutId === instance.layoutId
    && routeSegmentClear(instance.navigation, sampled.pose.point, sampled.pose.point);
  return Object.freeze({ residentId: resident.identity.id, revision: resident.revision, gameMinute: resident.updatedAtGameMinute,
    npcId: companionCombatAgberoBody.npcId, serviceId: null, visible,
    available: visible && activity.kind === "at-place" && activity.interactionId === null && resident.localMove === null,
    pose: visible && sampled ? sampled.pose : null, moving: visible && !!sampled?.moving });
}
