import type { CombatPoint } from "./companionCombat";
import type { CompanionStreetWeaponId, CompanionEquipment } from "./companionCombatEquipment";
import { carriedCompanionWeapon } from "./companionCombatEquipment";
import { sampleCompanionRoute, type CompanionJourney, type CompanionJourneyRoute } from "./companionJourney";
import { companionEnforcementDistrictForPoint, companionEnforcementDistrictProfiles, companionEnforcementPlaceDistrict,
  companionEnforcementNightlifeRoute, isCompanionEnforcementDistrict, type CompanionEnforcementDistrict } from "./companionPolicingDistricts";

export type CompanionEnforcementAgency = "police" | "lastma" | "efcc" | "ndlea";
export type CompanionJourneyEnforcementDecision = Readonly<{
  version: 1; agency: CompanionEnforcementAgency; stopKind: "checkpoint" | "roadside";
  district: CompanionEnforcementDistrict; isNight: boolean;
}>;
export type CompanionPoliceFinding = "checkpoint" | "harassment" | "validated-assault" | "weapon-possession" | "observed-weapon" | "gunfire";
export type CompanionEnforcementExtortion = {
  amount: number; unitKey: "lagos-credit"; status: "demanded" | "paid" | "refused";
  outingId: string; entries: { entryId: string; amount: number }[];
};
export type CompanionPoliceEncounter = { id: string; checkpointId: string; instanceId: string; officerId: string; phase: "searching" | "checking" | "cleared" | "arrested" | "detained" | "released"; reason: CompanionPoliceFinding; createdAt: number; resolvesAt: number; releaseAt: number | null; stationPlaceId: string | null; stationInstanceId: string | null; position: CombatPoint; evidenceKey: string; revision: number;
  weaponId?: CompanionStreetWeaponId | null; journeyId?: string | null; officerPosition?: CombatPoint; lastObservedAt?: number;
  agency?: CompanionEnforcementAgency; stopKind?: "checkpoint" | "roadside"; extortion?: CompanionEnforcementExtortion | null };
export type CompanionPoliceAction = "cooperate" | "request-release" | "pay-extortion" | "refuse-extortion";
export type CompanionPolicingSnapshot = { encounter: CompanionPoliceEncounter | null; ready: boolean; busy: boolean; error: string | null };
/** Local consent receipt. The backend still prices and settles the saved encounter. */
export type CompanionPoliceDemandConfirmation = Readonly<{
  encounterId: string; instanceId: string; revision: number; resolvesAt: number;
  amount: number; unitKey: "lagos-credit"; outingId: string;
}>;
export interface CompanionPolicingStore {
  getSnapshot(): CompanionPolicingSnapshot;
  subscribe(listener: () => void): () => void;
  cooperate(): Promise<void>;
  requestRelease(): Promise<void>;
  payExtortion(confirmation: CompanionPoliceDemandConfirmation): Promise<void>;
  refuseExtortion(): Promise<void>;
  dispose(): void;
}
export const proposedPolicingTuning = { checkMs: 1500, arrestRange: 2.2, observationRange: 12, detentionMs: 30000, maxActiveEncounters: 1 } as const;
export const companionEnforcementTuning = { stopMs: 12_000, refusalDelayMs: 8_000, cooldownMs: 300_000,
  extortionPercent: 10, extortionMaximum: 100, extortionChancePercent: 25 } as const;

export function captureCompanionPoliceDemand(encounter: CompanionPoliceEncounter | null): CompanionPoliceDemandConfirmation | null {
  const demand = encounter?.extortion;
  if (!encounter || encounter.phase !== "checking" || demand?.status !== "demanded" || demand.unitKey !== "lagos-credit"
    || !Number.isSafeInteger(demand.amount) || demand.amount < 1 || demand.amount > companionEnforcementTuning.extortionMaximum
    || !Number.isSafeInteger(encounter.revision) || encounter.revision < 0
    || !Number.isSafeInteger(encounter.resolvesAt) || encounter.resolvesAt < 0) return null;
  return { encounterId: encounter.id, instanceId: encounter.instanceId, revision: encounter.revision, resolvesAt: encounter.resolvesAt,
    amount: demand.amount, unitKey: demand.unitKey, outingId: demand.outingId };
}

export function matchesCompanionPoliceDemand(encounter: CompanionPoliceEncounter | null, confirmed: CompanionPoliceDemandConfirmation): boolean {
  const current = captureCompanionPoliceDemand(encounter);
  return !!current && current.encounterId === confirmed.encounterId && current.instanceId === confirmed.instanceId
    && current.revision === confirmed.revision && current.resolvesAt === confirmed.resolvesAt && current.amount === confirmed.amount
    && current.unitKey === confirmed.unitKey && current.outingId === confirmed.outingId;
}
export function companionEnforcementHash(key: string): number {
  return [...key].reduce((hash, value) => Math.imul(hash ^ value.charCodeAt(0), 16777619), 2166136261) >>> 0;
}
export function companionEnforcementAgencyLabel(agency: CompanionEnforcementAgency = "police"): string {
  return agency === "efcc" ? "EFCC" : agency === "ndlea" ? "NDLEA" : agency === "lastma" ? "LASTMA" : "Police";
}
/** Street pressure can request a check, never manufacture an offence or replace an existing finding. */
export function companionStreetTacticalFinding(existing: CompanionPoliceEncounter | null,
  weaponId: CompanionStreetWeaponId | null): CompanionPoliceFinding {
  return existing?.reason ?? (weaponId ? "weapon-possession" : "harassment");
}
/** Optional departure receipt. Older checkpoints retain deterministic legacy planning. */
export function readCompanionJourneyEnforcementDecision(checkpoint: unknown): CompanionJourneyEnforcementDecision | null {
  if (!checkpoint || typeof checkpoint !== "object" || !("enforcement" in checkpoint)) return null;
  const value = checkpoint.enforcement;
  if (!value || typeof value !== "object" || !("version" in value) || value.version !== 1
    || !("agency" in value) || value.agency !== "police" && value.agency !== "lastma" && value.agency !== "efcc" && value.agency !== "ndlea"
    || !("stopKind" in value) || value.stopKind !== "checkpoint" && value.stopKind !== "roadside"
    || !("district" in value) || !isCompanionEnforcementDistrict(value.district)
    || !("isNight" in value) || typeof value.isNight !== "boolean") return null;
  return { version: 1, agency: value.agency, stopKind: value.stopKind, district: value.district, isNight: value.isNight };
}
export function companionPoliceHasOffence(encounter: Pick<CompanionPoliceEncounter, "reason">): boolean {
  return encounter.reason !== "checkpoint" && encounter.reason !== "harassment";
}
/** Shared corridor/time-window choice. Clients cannot reroll a stop through request IDs. */
export function planCompanionJourneyEnforcement(args: {
  gameRoomId: string; fromPlaceId: string; toPlaceId: string; startedAt: number; durationMs: number;
  isNight?: boolean; route?: CompanionJourneyRoute; vehicle?: CompanionJourney["vehicle"];
  travelMode?: "walk" | "vehicle" | "boat";
}) {
  if (!Number.isSafeInteger(args.startedAt) || args.startedAt < 0 || !Number.isSafeInteger(args.durationMs) || args.durationMs <= 0
    || args.travelMode === "boat" || args.vehicle === "canoe" || args.vehicle === "speedboat") return null;
  const key = JSON.stringify([args.gameRoomId, args.fromPlaceId, args.toPlaceId, Math.floor(args.startedAt / 300_000)]);
  const progress = .25 + (companionEnforcementHash(`position:${key}`) % 51) / 100;
  const point = args.route ? sampleCompanionRoute(args.route, progress) : null;
  const district = point ? companionEnforcementDistrictForPoint(point)
    : companionEnforcementPlaceDistrict(args.toPlaceId) ?? companionEnforcementPlaceDistrict(args.fromPlaceId) ?? "town-centre";
  const profile = companionEnforcementDistrictProfiles[district], isNight = args.isNight === true;
  const police = isNight ? profile.policeNight : profile.policeDay;
  // Walking and unknown transport never imply a traffic offence or a LASTMA stop.
  const lastma = !isNight && args.travelMode === "vehicle" && !!args.vehicle ? profile.trafficDay : 0;
  const efcc = companionEnforcementNightlifeRoute(args.fromPlaceId, args.toPlaceId) ? profile.nightlifeEfcc * (isNight ? 1 : .5) : 0;
  const roll = (companionEnforcementHash(`agency:${key}`) % 10_000) / 100;
  if (roll >= police + lastma + profile.ndlea + efcc) return null;
  const agency: CompanionEnforcementAgency = roll < police ? "police" : roll < police + lastma ? "lastma"
    : roll < police + lastma + profile.ndlea ? "ndlea" : "efcc";
  const stopKind = agency === "lastma" || agency === "efcc" ? "roadside" as const
    : companionEnforcementHash(`kind:${key}`) % 3 === 0 ? "checkpoint" as const : "roadside" as const;
  const startsAt = args.startedAt + Math.floor(args.durationMs * progress);
  const endsAt = startsAt + companionEnforcementTuning.stopMs;
  if (!Number.isSafeInteger(endsAt)) return null;
  const decision: CompanionJourneyEnforcementDecision = { version: 1, agency, stopKind, district, isNight };
  // Keep timing-only checkpoints compatible until the world owner admits the receipt field.
  return { agency, stopKind, decision, checkpoint: { progress, startsAt, endsAt } };
}
/** Harassment and refusing an extortion demand never create a criminal finding. */
export function resolvePoliceCheck(encounter: CompanionPoliceEncounter, now: number, validatedOffence: boolean): CompanionPoliceEncounter {
  if (!Number.isFinite(now) || now < encounter.createdAt) throw new Error("Invalid police encounter time.");
  if (encounter.phase !== "checking" || now < encounter.resolvesAt) return encounter;
  return { ...encounter, phase: validatedOffence || companionPoliceHasOffence(encounter) ? "arrested" : "cleared", revision: encounter.revision + 1 };
}
export function discoveredCompanionWeapon(equipment: CompanionEquipment, observation: "search" | "sight") {
  const carried = carriedCompanionWeapon(equipment);
  return carried && (observation === "search" || carried.carry === "drawn") ? carried.weaponId : null;
}
export function policingLabel(encounter: CompanionPoliceEncounter): string {
  const agency = companionEnforcementAgencyLabel(encounter.agency);
  return encounter.phase === "searching" ? `${agency} are looking for you` : encounter.phase === "checking"
    ? `${agency} ${encounter.extortion?.status === "demanded" ? "cash demand" : encounter.agency === "lastma" ? "traffic stop" : encounter.reason === "harassment" ? "stop" : "search"}`
    : encounter.phase === "arrested" || encounter.phase === "detained" ? "At the station" : encounter.phase === "released" ? "Released" : "Check complete";
}
export function policingReason(encounter: CompanionPoliceEncounter): string {
  if (encounter.extortion?.status === "demanded") return `The officer demands ${encounter.extortion.amount} carried credits to let you through.`;
  if (encounter.extortion?.status === "paid") return `The officer took ${encounter.extortion.amount} carried credits.`;
  if (encounter.extortion?.status === "refused") return "The cash demand wasn't paid. The officer is delaying you.";
  return encounter.reason === "gunfire" ? "Gunfire reported nearby" : encounter.reason === "weapon-possession" ? "Carried weapon found" : encounter.reason === "observed-weapon" ? "Weapon seen by an officer"
    : encounter.reason === "validated-assault" ? "Assault witnessed" : encounter.reason === "harassment"
      ? encounter.agency === "efcc" ? "Agents question you near a nightlife route." : encounter.agency === "ndlea" ? "Officers insist on checking your bags."
        : encounter.agency === "lastma" ? "A traffic officer asks for your vehicle papers." : "An officer stops you and demands your papers."
      : encounter.stopKind === "roadside" ? "Roadside search" : "Routine checkpoint";
}
