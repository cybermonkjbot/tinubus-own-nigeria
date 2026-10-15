import type { CombatFighter } from "./companionCombat";
import type { CompanionNeeds } from "./companionLife";
import type { CompanionNpcBehaviour } from "./companionNpcs";
import type { CompanionPlaceAssetPart, CompanionPlaceLayout, CompanionPlaceNpc, CompanionPlacePose } from "./companionPlaces";
import { sampleCompanionWorldClock, type CompanionWorldClock } from "./companionWorldClock";

/** Only these residents have connected saved places, holdings and transport. */
export const companionPersistentResidentKeys = Object.freeze([
  "mai-suya", "akara-woman", "courier", "danfo-driver", "medic", "tailor",
  "street-akara-vendor", "area-tout",
] as const);
export const companionResidentKeys = Object.freeze([
  ...companionPersistentResidentKeys,
  "traffic-hawker", "pos-agent", "danfo-conductor",
] as const);
export type CompanionResidentKey = typeof companionResidentKeys[number];
export const companionResidentLimits = Object.freeze({ assets: 32, debts: 8, quantity: 10_000, amount: 1_000_000_000 });

export type CompanionResidentOccupation = "suya-cook" | "akara-cook" | "courier" | "danfo-driver" | "medic" | "tailor"
  | "traffic-hawker" | "pos-operator" | "conductor" | "union-collector";
export type CompanionResidentTrait = "sociable" | "reserved" | "patient" | "brisk" | "practical" | "cautious";
export type CompanionResidentIdentity = Readonly<{
  actorKind: "npc";
  id: string;
  worldKey: string;
  actorKey: CompanionResidentKey;
  rosterVersion: 1;
  name: string;
  occupation: CompanionResidentOccupation;
  role: CompanionPlaceNpc["role"];
  personality: readonly [CompanionResidentTrait, CompanionResidentTrait];
  /** Seed is visual variation only. Missing art stays explicit. */
  appearance: Readonly<{ seed: number; assetNpcId: NonNullable<CompanionPlaceAssetPart["npcId"]> | null }>;
}>;

export type CompanionResidentPlace = Readonly<Pick<CompanionPlaceLayout, "placeId"> & { instanceId: string }>;
export type CompanionResidentAnchor = CompanionResidentPlace & Readonly<{ pose: Readonly<CompanionPlacePose> }>;
export type CompanionResidentPlaces = Readonly<{
  /** Resolved NPC housing, never a player's implicit own-home destination. */
  home: CompanionResidentAnchor;
  work: CompanionResidentAnchor;
  social: CompanionResidentAnchor;
}>;
export type CompanionResidentTravelMode = "walk" | "car" | "okada" | "keke" | "danfo";
export type CompanionResidentActivity =
  | Readonly<{
      kind: "at-place";
      location: CompanionResidentAnchor;
      action: "rest" | "work" | "eat" | "socialize" | "recover";
      /** A live encounter or service keeps this actor in place until its owner releases it. */
      interactionId: string | null;
      sinceGameMinute: number;
    }>
  | Readonly<{
      kind: "traveling";
      journeyId: string;
      from: CompanionResidentPlace;
      to: CompanionResidentPlace;
      mode: CompanionResidentTravelMode;
      vehicleId: string | null;
      sinceGameMinute: number;
    }>;

export type CompanionResidentAsset = Readonly<{
  /** Durable item/stack, vehicle or home record, not a model or catalogue ID. */
  id: string;
  kind: "item" | "vehicle" | "home";
  quantity: number;
  custody: "banked" | "carried";
  /** Null means unvalued, not free. Values come from the economy owner. */
  valuation: Readonly<{ unit: "game-value-unit"; unitKey: string; unitAmount: number; sourceKey: string }> | null;
}>;
export type CompanionResidentHoldings = Readonly<{
  unit: "game-value-unit";
  unitKey: string;
  cash: Readonly<{ banked: number; carried: number }>;
  assets: readonly CompanionResidentAsset[];
  /** Outstanding total already supplied by its owner. No interest accrues here. */
  debts: readonly Readonly<{ id: string; outstandingAmount: number; sourceKey: string }>[];
}>;
export type CompanionResidentWorth = Readonly<{
  unit: "game-value-unit";
  unitKey: string;
  knownBankedValue: number;
  knownCarriedValue: number;
  outstandingDebt: number;
  /** Excludes carried property. Null if any banked asset lacks a valuation. */
  netWorth: number | null;
  /** At risk, not owned/banked worth. Null if any carried item lacks a valuation. */
  carriedValue: number | null;
  unvaluedAssetIds: readonly string[];
}>;

/** Persistence contract only. Initialization and consequential writes belong to the backend. */
export type CompanionResidentState = Readonly<{
  version: 1;
  identity: CompanionResidentIdentity;
  revision: number;
  clockEpochKey: string;
  updatedAtGameMinute: number;
  needs: Readonly<CompanionNeeds>;
  health: Readonly<Pick<CombatFighter, "health" | "maxHealth">>;
  holdings: CompanionResidentHoldings;
  places: CompanionResidentPlaces;
  /** The only actual location: either one place or one journey, never both. */
  activity: CompanionResidentActivity;
}>;
export type CompanionResidentRoutine = Readonly<{
  residentId: string;
  clockEpochKey: string;
  sampledAtGameMinute: number;
  action: "rest" | "work" | "socialize";
  behaviour: Extract<CompanionNpcBehaviour, "idle" | "work" | "gather">;
  destination: CompanionResidentAnchor;
  nextChangeAtGameMinute: number;
  /** A plan cannot change membership or override travel, recovery or an interaction. */
  disposition: "same-place" | "journey-required" | "defer";
}>;

type ResidentProfile = {
  name: string;
  occupation: CompanionResidentOccupation;
  role: CompanionPlaceNpc["role"];
  personality: readonly [CompanionResidentTrait, CompanionResidentTrait];
  assetNpcId: NonNullable<CompanionPlaceAssetPart["npcId"]> | null;
  /** Proposed work start, work end and social end, in game minutes after midnight. */
  hours: readonly [number, number, number];
};
const profiles: Record<CompanionResidentKey, ResidentProfile> = {
  "mai-suya": { name: "Musa Bello", occupation: "suya-cook", role: "vendor", personality: ["sociable", "patient"], assetNpcId: "mai-suya", hours: [900, 1320, 1380] },
  "akara-woman": { name: "Bisi Adebayo", occupation: "akara-cook", role: "vendor", personality: ["brisk", "practical"], assetNpcId: "akara-woman", hours: [360, 780, 900] },
  "street-akara-vendor": { name: "Yemisi Akinwale", occupation: "akara-cook", role: "vendor", personality: ["patient", "sociable"], assetNpcId: "akara-woman", hours: [360, 840, 960] },
  courier: { name: "Tunde Bakare", occupation: "courier", role: "mission-contact", personality: ["sociable", "cautious"], assetNpcId: null, hours: [480, 1020, 1140] },
  "danfo-driver": { name: "Emeka Okafor", occupation: "danfo-driver", role: "driver", personality: ["brisk", "practical"], assetNpcId: null, hours: [300, 1080, 1140] },
  medic: { name: "Amaka Okeke", occupation: "medic", role: "medic", personality: ["patient", "reserved"], assetNpcId: null, hours: [480, 960, 1020] },
  tailor: { name: "Zainab Lawal", occupation: "tailor", role: "resident", personality: ["reserved", "practical"], assetNpcId: null, hours: [540, 1080, 1200] },
  "traffic-hawker": { name: "Kafayat Alabi", occupation: "traffic-hawker", role: "vendor", personality: ["brisk", "cautious"], assetNpcId: null, hours: [420, 1140, 1260] },
  "pos-agent": { name: "Chidinma Nwosu", occupation: "pos-operator", role: "vendor", personality: ["practical", "patient"], assetNpcId: null, hours: [480, 1200, 1260] },
  "danfo-conductor": { name: "Femi Adeleke", occupation: "conductor", role: "driver", personality: ["sociable", "brisk"], assetNpcId: null, hours: [300, 1080, 1200] },
  "area-tout": { name: "Ganiu Ojo", occupation: "union-collector", role: "resident", personality: ["practical", "reserved"], assetNpcId: "street-tout-shirt", hours: [360, 1260, 1380] },
};

function requireKey(value: unknown, label: string, limit = 160): asserts value is string {
  if (typeof value !== "string" || value.length > limit || value.trim() !== value || !/^[A-Za-z0-9][A-Za-z0-9._:-]*$/.test(value)) {
    throw new Error(`Provide a bounded ${label} key.`);
  }
}
function requireInteger(value: number, label: string, maximum = Number.MAX_SAFE_INTEGER) {
  if (!Number.isSafeInteger(value) || value < 0 || value > maximum) throw new Error(`Provide a bounded nonnegative ${label}.`);
}
function profileFor(key: CompanionResidentKey): ResidentProfile {
  if (!companionResidentKeys.some(candidate => candidate === key)) throw new Error("Choose a resident from the named roster.");
  return profiles[key];
}
function seedNumber(key: string) {
  let value = 2166136261;
  for (let index = 0; index < key.length; index++) value = Math.imul(value ^ key.charCodeAt(index), 16777619);
  return value >>> 0;
}

/** Use the durable world key, never a place-instance, viewer or join-time seed.
 * The length-prefixed identity cannot collide when keys contain separators.
 */
export function createCompanionResidentIdentity(worldKey: string, actorKey: CompanionResidentKey): CompanionResidentIdentity {
  requireKey(worldKey, "world", 80);
  const profile = profileFor(actorKey);
  const id = `npc:${worldKey.length}:${worldKey}:${actorKey}`;
  return Object.freeze<CompanionResidentIdentity>({
    actorKind: "npc", id, worldKey, actorKey, rosterVersion: 1,
    name: profile.name, occupation: profile.occupation, role: profile.role,
    personality: Object.freeze([profile.personality[0], profile.personality[1]] as const),
    appearance: Object.freeze({ seed: seedNumber(id), assetNpcId: profile.assetNpcId }),
  });
}

/** Sum one explicit game unit. This neither initializes holdings nor changes them. */
export function companionResidentWorth(holdings: CompanionResidentHoldings): CompanionResidentWorth {
  if (!holdings || holdings.unit !== "game-value-unit" || !holdings.cash
    || !Array.isArray(holdings.assets) || holdings.assets.length > companionResidentLimits.assets
    || !Array.isArray(holdings.debts) || holdings.debts.length > companionResidentLimits.debts) {
    throw new Error("Provide bounded simulated holdings in one game-value unit.");
  }
  requireKey(holdings.unitKey, "game unit");
  requireInteger(holdings.cash.banked, "banked cash", companionResidentLimits.amount);
  requireInteger(holdings.cash.carried, "carried cash", companionResidentLimits.amount);
  let banked = holdings.cash.banked, carried = holdings.cash.carried, debt = 0;
  let bankedComplete = true, carriedComplete = true;
  const assetIds = new Set<string>(), debtIds = new Set<string>(), unvalued: string[] = [];
  for (const asset of holdings.assets) {
    if (!asset) throw new Error("Provide a resident asset record.");
    requireKey(asset.id, "asset");
    requireInteger(asset.quantity, "asset quantity", companionResidentLimits.quantity);
    if (assetIds.has(asset.id) || asset.quantity === 0 || !["item", "vehicle", "home"].includes(asset.kind)
      || !["banked", "carried"].includes(asset.custody)
      || asset.kind !== "item" && (asset.quantity !== 1 || asset.custody !== "banked")) {
      throw new Error("Use unique assets, positive quantities and single banked vehicle/home records.");
    }
    assetIds.add(asset.id);
    if (asset.valuation === null) {
      unvalued.push(asset.id);
      if (asset.custody === "banked") bankedComplete = false;
      else carriedComplete = false;
      continue;
    }
    const valuation = asset.valuation;
    if (!valuation || valuation.unit !== "game-value-unit" || valuation.unitKey !== holdings.unitKey) {
      throw new Error("Each asset valuation must use the holdings' game unit.");
    }
    requireKey(valuation.sourceKey, "valuation source");
    requireInteger(valuation.unitAmount, "asset value", companionResidentLimits.amount);
    const value = asset.quantity * valuation.unitAmount;
    requireInteger(value, "extended asset value");
    if (asset.custody === "banked") banked += value;
    else carried += value;
    requireInteger(banked, "banked value");
    requireInteger(carried, "carried value");
  }
  for (const liability of holdings.debts) {
    if (!liability) throw new Error("Provide a resident liability record.");
    requireKey(liability.id, "debt");
    requireKey(liability.sourceKey, "debt source");
    requireInteger(liability.outstandingAmount, "outstanding debt", companionResidentLimits.amount);
    if (debtIds.has(liability.id)) throw new Error("Use one entry per outstanding liability.");
    debtIds.add(liability.id);
    debt += liability.outstandingAmount;
    requireInteger(debt, "total debt");
  }
  return Object.freeze<CompanionResidentWorth>({
    unit: "game-value-unit", unitKey: holdings.unitKey, knownBankedValue: banked, knownCarriedValue: carried,
    outstandingDebt: debt, netWorth: bankedComplete ? banked - debt : null,
    carriedValue: carriedComplete ? carried : null, unvaluedAssetIds: Object.freeze(unvalued),
  });
}

function requirePlace(place: CompanionResidentPlace) {
  if (!place) throw new Error("Provide a resolved resident place.");
  requireKey(place.placeId, "place");
  requireKey(place.instanceId, "instance");
}
function requireAnchor(anchor: CompanionResidentAnchor) {
  requirePlace(anchor);
  if (!anchor.pose || !Array.isArray(anchor.pose.point) || anchor.pose.point.length !== 3
    || !anchor.pose.point.every(Number.isFinite) || !Number.isFinite(anchor.pose.yaw)) {
    throw new Error("Provide a finite resident pose from the resolved layout.");
  }
}
const samePlace = (a: CompanionResidentPlace, b: CompanionResidentPlace) => a.placeId === b.placeId && a.instanceId === b.instanceId;

/** Direct schedule lookup with no missed-day replay or automatic actor relocation.
 * Consequential state wins over this proposal. Only a validated journey may change place.
 */
export function planCompanionResidentRoutine(
  state: Pick<CompanionResidentState, "identity" | "clockEpochKey" | "updatedAtGameMinute" | "places" | "activity">,
  clock: CompanionWorldClock,
  sharedNowRealMs: number,
): CompanionResidentRoutine {
  const time = sampleCompanionWorldClock(clock, sharedNowRealMs);
  const identity = state.identity;
  const expected = createCompanionResidentIdentity(identity.worldKey, identity.actorKey);
  if (identity.actorKind !== "npc" || identity.rosterVersion !== 1 || identity.id !== expected.id
    || identity.worldKey !== time.worldKey || state.clockEpochKey !== time.epochKey) {
    throw new Error("Resident and clock must belong to the same supported world and epoch.");
  }
  requireInteger(state.updatedAtGameMinute, "resident update minute");
  if (time.gameMinute < state.updatedAtGameMinute) throw new Error("Refresh the shared clock before planning this resident.");
  for (const anchor of [state.places.home, state.places.work, state.places.social]) requireAnchor(anchor);
  const activity = state.activity;
  requireInteger(activity.sinceGameMinute, "activity start minute");
  if (activity.sinceGameMinute > state.updatedAtGameMinute) throw new Error("Activity cannot begin after the saved resident update.");
  if (activity.kind === "at-place") {
    requireAnchor(activity.location);
    if (!["rest", "work", "eat", "socialize", "recover"].includes(activity.action)) throw new Error("Provide a supported resident action.");
    if (activity.interactionId !== null) requireKey(activity.interactionId, "interaction");
  } else if (activity.kind === "traveling") {
    requireKey(activity.journeyId, "journey");
    requirePlace(activity.from); requirePlace(activity.to);
    if (samePlace(activity.from, activity.to) || !["walk", "car", "okada", "keke", "danfo"].includes(activity.mode)
      || activity.mode === "walk" && activity.vehicleId !== null
      || activity.mode !== "walk" && activity.vehicleId === null) throw new Error("Provide one resolved cross-place journey and its vehicle when riding.");
    if (activity.vehicleId !== null) requireKey(activity.vehicleId, "vehicle");
  } else {
    throw new Error("A resident must occupy exactly one place or journey.");
  }

  // Stable 0..15 minute variation, independent of join time and current place.
  const offset = seedNumber(`${identity.id}:routine-v1`) % 16;
  const [workStart, workEnd, socialEnd] = profiles[identity.actorKey].hours.map(minute => minute + offset);
  const minute = time.minuteOfDay;
  const action = minute >= workStart && minute < workEnd ? "work"
    : minute >= workEnd && minute < socialEnd ? "socialize" : "rest";
  const destination = action === "work" ? state.places.work : action === "socialize" ? state.places.social : state.places.home;
  const nextMinute = minute < workStart ? workStart : minute < workEnd ? workEnd : minute < socialEnd ? socialEnd : 1440 + workStart;
  const nextChangeAtGameMinute = time.gameMinute - minute + nextMinute;
  requireInteger(nextChangeAtGameMinute, "next routine minute");
  const busy = activity.kind === "traveling" || activity.interactionId !== null || activity.action === "recover" || activity.action === "eat";
  return Object.freeze<CompanionResidentRoutine>({
    residentId: identity.id, clockEpochKey: time.epochKey, sampledAtGameMinute: time.gameMinute,
    action, behaviour: action === "work" ? "work" : action === "socialize" ? "gather" : "idle",
    destination: Object.freeze({ placeId: destination.placeId, instanceId: destination.instanceId,
      pose: Object.freeze({ point: Object.freeze([...destination.pose.point] as [number, number, number]), yaw: destination.pose.yaw }) }),
    nextChangeAtGameMinute,
    disposition: busy ? "defer" : activity.kind === "at-place" && samePlace(activity.location, destination) ? "same-place" : "journey-required",
  });
}
