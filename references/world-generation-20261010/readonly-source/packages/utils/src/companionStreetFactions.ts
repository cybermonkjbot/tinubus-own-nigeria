import { companionRoadHornExposure, sameCompanionRoadScope, validCompanionRoadFrame,
  type CompanionRoadAcceptedFrame, type CompanionRoadHornPulse } from "./companionRoadMechanics";
import { routeSegmentClear, type RoomNavigation, type ScenePoint } from "./companionNavigation";
import { companionEnforcementDistrictAt, isCompanionEnforcementDistrict, type CompanionEnforcementDistrict } from "./companionPolicingDistricts";
import type { CompanionEnforcementAgency } from "./companionPolicing";
import type { CompanionWorldClockSample } from "./companionWorldClock";

/** Fictional noncash tuning. Pressure is neither an offence nor a damage receipt. */
export const companionStreetFactionRules = Object.freeze({ version: 1, maximum: 100, decayMs: 20_000,
  evidenceAgeMs: 2_000, witnessAgeMs: 250, observationRange: 24, maximumWitnesses: 8,
  maximumEvidence: 64, noise: 7, violation: 24, terminal: 4, evasion: 18, incident: 20,
  unionApproach: 18, trafficApproach: 24, tacticalApproach: 20 });
export type CompanionStreetFaction = "union" | "traffic" | "tactical";
export type CompanionStreetFactionScope = Readonly<{ worldKey: string; epochKey: string; userId: string }>;
export type CompanionStreetPressure = Readonly<{ union: number; traffic: number; tactical: number; at: number }>;
export type CompanionStreetFactionState = Readonly<{
  version: 1; scope: CompanionStreetFactionScope; revision: number; lastEvidenceAt: number;
  /** IDs at lastEvidenceAt only. Older evidence is rejected even after pruning. */
  evidenceIds: readonly string[]; districts: Partial<Record<CompanionEnforcementDistrict, CompanionStreetPressure>>;
}>;
export type CompanionStreetWitness = Readonly<{
  id: string; faction: CompanionStreetFaction; agency: CompanionEnforcementAgency | null;
  point: ScenePoint; at: number; reserved: boolean;
}>;
export type CompanionStreetEvidence = Readonly<{
  id: string; kind: "noise" | "prohibited-direction" | "dedicated-lane" | "terminal-passage" | "levy-evasion" | "incident";
  frame: CompanionRoadAcceptedFrame; district: CompanionEnforcementDistrict;
  horn?: CompanionRoadHornPulse; terminalEncounterId?: string;
}>;
const integer = (value: number) => Number.isSafeInteger(value) && value >= 0 && value < Number.MAX_SAFE_INTEGER - 1;
export const companionStreetKey = (value: unknown): value is string => typeof value === "string"
  && value.length > 0 && value.length <= 240 && value.trim() === value;
export const sameCompanionStreetFactionScope = (a: CompanionStreetFactionScope, b: CompanionStreetFactionScope) =>
  a.worldKey === b.worldKey && a.epochKey === b.epochKey && a.userId === b.userId;
export function createCompanionStreetFactionState(scope: CompanionStreetFactionScope, now: number): CompanionStreetFactionState {
  if (![scope.worldKey, scope.epochKey, scope.userId].every(companionStreetKey) || !integer(now)) throw new Error("Use the admitted street scope.");
  return { version: 1, scope: { ...scope }, revision: 0, lastEvidenceAt: now, evidenceIds: [], districts: {} };
}
export function validCompanionStreetFactionState(state: CompanionStreetFactionState): boolean {
  return !!state && state.version === 1 && !!state.scope && [state.scope.worldKey, state.scope.epochKey, state.scope.userId].every(companionStreetKey)
    && integer(state.revision) && integer(state.lastEvidenceAt) && Array.isArray(state.evidenceIds)
    && state.evidenceIds.length <= companionStreetFactionRules.maximumEvidence && state.evidenceIds.every(companionStreetKey)
    && new Set(state.evidenceIds).size === state.evidenceIds.length && !!state.districts
    && typeof state.districts === "object" && !Array.isArray(state.districts)
    && Object.keys(state.districts).length <= 7
    && Object.entries(state.districts).every(([district, pressure]) => isCompanionEnforcementDistrict(district) && !!pressure
      && integer(pressure.at) && pressure.at <= state.lastEvidenceAt
      && [pressure.union, pressure.traffic, pressure.tactical].every(value => integer(value) && value <= companionStreetFactionRules.maximum));
}
/** Lazy real-time decay. No scheduler, offline penalties or reset on joining another game. */
export function projectCompanionStreetPressure(state: CompanionStreetFactionState, district: CompanionEnforcementDistrict, now: number): CompanionStreetPressure {
  if (!validCompanionStreetFactionState(state) || !isCompanionEnforcementDistrict(district) || !integer(now) || now < state.lastEvidenceAt)
    throw new Error("Street pressure needs current saved evidence.");
  const value = state.districts[district] ?? { union: 0, traffic: 0, tactical: 0, at: now };
  if (now < value.at) throw new Error("Street pressure time moved backwards.");
  const decay = Math.floor((now - value.at) / companionStreetFactionRules.decayMs);
  return { union: Math.max(0, value.union - decay), traffic: Math.max(0, value.traffic - decay), tactical: Math.max(0, value.tactical - decay),
    at: value.at + decay * companionStreetFactionRules.decayMs };
}
const pointValid = (point: ScenePoint) => Array.isArray(point) && point.length === 3 && point.every(Number.isFinite);
export function companionStreetWitnesses(evidence: CompanionStreetEvidence, witnesses: readonly CompanionStreetWitness[], now: number,
  navigation: RoomNavigation, boundaryGain: (from: ScenePoint, to: ScenePoint) => number): readonly CompanionStreetWitness[] {
  if (!validCompanionRoadFrame(evidence.frame) || !integer(now) || now < evidence.frame.at
    || now - evidence.frame.at > companionStreetFactionRules.evidenceAgeMs || witnesses.length > companionStreetFactionRules.maximumWitnesses
    || new Set(witnesses.map(witness => witness.id)).size !== witnesses.length
    || evidence.frame.scope.layoutRevision !== navigation.revision) return [];
  const from: ScenePoint = [evidence.frame.pose.x, evidence.frame.pose.y, evidence.frame.pose.z];
  return witnesses.filter(witness => companionStreetKey(witness.id) && pointValid(witness.point) && witness.reserved === false
    && integer(witness.at) && witness.at <= now && now - witness.at <= companionStreetFactionRules.witnessAgeMs
    && Math.abs(witness.at - evidence.frame.at) <= companionStreetFactionRules.witnessAgeMs
    && Math.abs(from[1] - witness.point[1]) <= .5
    && (witness.faction === "union" ? witness.agency === null : witness.faction === "traffic" ? witness.agency === "lastma"
      : witness.faction === "tactical" && ["police", "efcc", "ndlea"].includes(witness.agency ?? ""))
    && (evidence.kind === "noise" ? !!evidence.horn && sameCompanionRoadScope(evidence.frame.scope, evidence.horn.scope)
      && evidence.horn.sequence === evidence.frame.sequence && evidence.horn.at === evidence.frame.at
      && evidence.horn.point[0] === from[0] && evidence.horn.point[1] === from[1] && evidence.horn.point[2] === from[2]
      && companionRoadHornExposure(evidence.horn, witness.point, now, boundaryGain) >= .1
      : Math.hypot(from[0] - witness.point[0], from[2] - witness.point[2]) <= companionStreetFactionRules.observationRange
        && routeSegmentClear({ ...navigation, radius: .001 }, from, witness.point)));
}
/** Internal producer input only. The world owner validates the actual manoeuvre/terminal/incident first. */
export function admitCompanionStreetEvidence(state: CompanionStreetFactionState, scope: CompanionStreetFactionScope,
  evidence: CompanionStreetEvidence, witnesses: readonly CompanionStreetWitness[], now: number,
  navigation: RoomNavigation, boundaryGain: (from: ScenePoint, to: ScenePoint) => number) {
  const unchanged = { state, accepted: false, witnesses: [] as readonly CompanionStreetWitness[] };
  if (!validCompanionStreetFactionState(state) || !sameCompanionStreetFactionScope(state.scope, scope)
    || !companionStreetKey(evidence.id) || !isCompanionEnforcementDistrict(evidence.district) || !validCompanionRoadFrame(evidence.frame)
    || evidence.district !== companionEnforcementDistrictAt(evidence.frame.pose.x, evidence.frame.pose.z)
    || !evidence.frame.active || evidence.frame.externalHold || !integer(now) || evidence.frame.at < state.lastEvidenceAt
    || evidence.frame.at === state.lastEvidenceAt && (state.evidenceIds.includes(evidence.id) || state.evidenceIds.length >= companionStreetFactionRules.maximumEvidence)
    || ["terminal-passage", "levy-evasion"].includes(evidence.kind) && !companionStreetKey(evidence.terminalEncounterId)) return unchanged;
  const seen = companionStreetWitnesses(evidence, witnesses, now, navigation, boundaryGain);
  const factions = new Set(seen.map(witness => witness.faction));
  const current = projectCompanionStreetPressure(state, evidence.district, evidence.frame.at);
  const changes = { union: 0, traffic: 0, tactical: 0 };
  if (evidence.kind === "noise") {
    if (factions.has("union")) changes.union = companionStreetFactionRules.noise;
    if (factions.has("tactical")) changes.tactical = companionStreetFactionRules.noise;
  } else if (evidence.kind === "prohibited-direction" || evidence.kind === "dedicated-lane") {
    if (factions.has("traffic")) changes.traffic = companionStreetFactionRules.violation;
  } else if (evidence.kind === "terminal-passage" || evidence.kind === "levy-evasion") {
    if (factions.has("union")) changes.union = evidence.kind === "terminal-passage" ? companionStreetFactionRules.terminal : companionStreetFactionRules.evasion;
  } else if (evidence.kind === "incident" && factions.has("tactical")) changes.tactical = companionStreetFactionRules.incident;
  if (!Object.values(changes).some(value => value > 0)) return unchanged;
  const pressure = { at: current.at, union: Math.min(100, current.union + changes.union),
    traffic: Math.min(100, current.traffic + changes.traffic), tactical: Math.min(100, current.tactical + changes.tactical) };
  return { state: { ...state, revision: state.revision + 1, lastEvidenceAt: evidence.frame.at,
    evidenceIds: evidence.frame.at === state.lastEvidenceAt ? [...state.evidenceIds, evidence.id] : [evidence.id],
    districts: { ...state.districts, [evidence.district]: pressure } } satisfies CompanionStreetFactionState, accepted: true, witnesses: seen };
}
export type CompanionFictionalVehicleMetadata = Readonly<{ modelId: string; revision: number;
  class: "ordinary" | "worn-sedan" | "sports-coupe" | "luxury-suv"; provenance: "fictional-catalogue" }>;
/** Unknown/generic cars remain ordinary. This multiplier never establishes an offence. */
export function companionVehicleSuspicion(metadata: CompanionFictionalVehicleMetadata | null,
  district: CompanionEnforcementDistrict, clock: CompanionWorldClockSample) {
  const valid = !!metadata && metadata.provenance === "fictional-catalogue" && companionStreetKey(metadata.modelId)
    && integer(metadata.revision) && ["ordinary", "worn-sedan", "sports-coupe", "luxury-suv"].includes(metadata.class);
  const nightlife = ["marina", "skyline", "palm-courts"].includes(district);
  const night = !!clock && Number.isInteger(clock.hour) && clock.hour >= 0 && clock.hour < 24 && (clock.hour >= 19 || clock.hour < 6);
  const highValue = valid && (metadata.class === "sports-coupe" || metadata.class === "luxury-suv");
  return { multiplier: highValue ? nightlife && night ? 1.75 : nightlife || night ? 1.35 : 1.1 : valid && metadata.class === "worn-sedan" ? .5 : 1,
    offence: false as const, metadataKnown: valid };
}

/** Select an existing free agency body for an approach request. Contact still belongs to policing. */
export function selectCompanionStreetTacticalApproach(args: {
  scope: CompanionStreetFactionScope; pressure: CompanionStreetPressure; point: ScenePoint;
  district: CompanionEnforcementDistrict; clock: CompanionWorldClockSample;
  metadata: CompanionFictionalVehicleMetadata | null; witnesses: readonly CompanionStreetWitness[];
  navigation: RoomNavigation; now: number;
}) {
  if (!integer(args.now) || !pointValid(args.point) || !args.scope || !args.clock || args.clock.version !== 1
    || ![args.scope.worldKey, args.scope.epochKey, args.scope.userId].every(companionStreetKey)
    || !isCompanionEnforcementDistrict(args.district) || args.district !== companionEnforcementDistrictAt(args.point[0], args.point[2])
    || args.witnesses.length > companionStreetFactionRules.maximumWitnesses
    || new Set(args.witnesses.map(witness => witness.id)).size !== args.witnesses.length
    || args.clock.worldKey !== args.scope.worldKey || args.clock.epochKey !== args.scope.epochKey
    || args.clock.sampledAtRealMs !== args.now || !integer(args.pressure.at) || args.pressure.at > args.now
    || ![args.pressure.union, args.pressure.traffic, args.pressure.tactical].every(value => integer(value) && value <= 100)) return null;
  const suspicion = companionVehicleSuspicion(args.metadata, args.district, args.clock);
  if (args.pressure.tactical < companionStreetFactionRules.tacticalApproach && suspicion.multiplier <= 1) return null;
  const range = Math.min(36, companionStreetFactionRules.observationRange * suspicion.multiplier);
  const witness = [...args.witnesses].filter(value => value.faction === "tactical" && value.reserved === false && companionStreetKey(value.id)
    && ["police", "efcc", "ndlea"].includes(value.agency ?? "") && pointValid(value.point) && integer(value.at)
    && value.at <= args.now && args.now - value.at <= companionStreetFactionRules.witnessAgeMs
    && Math.abs(value.point[1] - args.point[1]) <= .5
    && Math.hypot(value.point[0] - args.point[0], value.point[2] - args.point[2]) <= range
    && routeSegmentClear({ ...args.navigation, radius: .001 }, value.point, args.point))
    .sort((a, b) => a.id.localeCompare(b.id))[0];
  if (!witness || witness.agency !== "police" && witness.agency !== "efcc" && witness.agency !== "ndlea") return null;
  return { actorId: witness.id, agency: witness.agency, point: witness.point,
    reason: suspicion.multiplier > 1 ? "vehicle-suspicion" as const : "local-noise" as const,
    offence: false as const, range };
}
