import { companionRouteLength, type CompanionGeoPoint } from "./companionJourney";
import { validCompanionGameEconomyKey, validCompanionGameEconomyRequestId } from "./companionGameEconomy";
import {
  companionResidentLimits, createCompanionResidentIdentity,
  type CompanionResidentIdentity, type CompanionResidentPlace, type CompanionResidentState,
  type CompanionResidentTravelMode,
} from "./companionResidents";
import { sampleCompanionWorldClock, type CompanionWorldClock } from "./companionWorldClock";

/** Storage/calculation ceilings, not prices, fuel defaults or real-world specifications. */
export const companionVehicleLimits = Object.freeze({
  routePoints: 128, routeMetres: 1_000_000, fuelUnits: 1_000_000, metresPerFuelUnit: 1_000_000,
  amount: companionResidentLimits.amount,
});

/** Noncash Lagos game tuning. Installation is explicit and never funds residents or stock. */
export const companionResidentVehicleRules = Object.freeze({
  catalogueId: "lagos-resident-transport", catalogueRevision: 1, unitKey: "lagos-credit",
  sourceKey: "lagos-noncash-transport-v1",
  models: Object.freeze([
    Object.freeze({ modelId: "car", kind: "car" as const, tankCapacityUnits: 60, metresPerFuelUnit: 800, metresPerRealSecond: 10, purchase: 2400, fuel: 3, fare: 18 }),
    Object.freeze({ modelId: "okada", kind: "okada" as const, tankCapacityUnits: 16, metresPerFuelUnit: 1000, metresPerRealSecond: 8, purchase: 700, fuel: 2, fare: 8 }),
    Object.freeze({ modelId: "keke", kind: "keke" as const, tankCapacityUnits: 24, metresPerFuelUnit: 700, metresPerRealSecond: 7, purchase: 1200, fuel: 2, fare: 12 }),
    Object.freeze({ modelId: "danfo", kind: "danfo" as const, tankCapacityUnits: 80, metresPerFuelUnit: 500, metresPerRealSecond: 8, purchase: 3000, fuel: 3, fare: 10 }),
  ]),
});
export type CompanionVehicleKind = Exclude<CompanionResidentTravelMode, "walk">;

/** Rebalance haul transport utility across vehicle tiers without mutating schema. */
export function companionVehicleCargoSlots(kind: CompanionVehicleKind): number {
  switch (kind) {
    case "okada": return 2;
    case "keke": return 4;
    case "car": return 8;
    case "danfo": return 16;
  }
}

/** An immutable server catalogue snapshot. Neither ID identifies generated art. */
export type CompanionVehicleModel = Readonly<{
  catalogueId: string;
  modelId: string;
  catalogueRevision: number;
  kind: CompanionVehicleKind;
  tankCapacityUnits: number;
  metresPerFuelUnit: number;
}>;
export type CompanionVehicleRoute = Readonly<{
  worldKey: string;
  id: string;
  revision: number;
  /** Server-resolved route, never a caller's claimed distance. */
  points: readonly Readonly<CompanionGeoPoint>[];
}>;
export type CompanionVehicleRouteQuote = Readonly<{
  routeId: string;
  routeRevision: number;
  routeMetres: number;
  fuelUnits: number;
}>;

/** Immutable binding to the resident backend's authoritative journey, not player danfo state.
 * Route IDs/revisions must identify immutable geometry in the same world.
 * Travel readiness uses server milliseconds, independent of accelerated game time.
 */
export type CompanionVehicleJourney = Readonly<{
  id: string;
  requestId: string;
  vehicleId: string;
  ownerResidentId: string;
  worldKey: string;
  mode: CompanionVehicleKind;
  clockEpochKey: string;
  from: CompanionResidentPlace;
  to: CompanionResidentPlace;
  routeId: string;
  routeRevision: number;
  timeBasis: "server-ms";
  startedAtRealMs: number;
  arrivesAtRealMs: number;
}>;
export type CompanionVehicleActivity =
  | Readonly<{
      kind: "parked";
      location: CompanionResidentPlace;
      /** Blocks immediate reuse. Older request/journey uniqueness belongs to persistence. */
      lastJourney: Readonly<{
        id: string; requestId: string; clockEpochKey: string; completedAtRealMs: number;
      }> | null;
    }>
  | Readonly<{
      kind: "traveling";
      journey: CompanionVehicleJourney;
      routeMetres: number;
      /** Already removed from fuelUnits at departure. Arrival never charges fuel again. */
      reservedFuelUnits: number;
    }>;
export type CompanionVehicleState = Readonly<{
  version: 1;
  id: string;
  ownerResidentId: string;
  worldKey: string;
  model: CompanionVehicleModel;
  /** Available integer fuel units, excluding an active journey's reservation. */
  fuelUnits: number;
  odometerMetres: number;
  revision: number;
  activity: CompanionVehicleActivity;
}>;

/** Trusted server terms only. Approval here concerns simulated value, not live finance.
 * Fuel is priced per fuel unit; fare and purchase are priced per journey and vehicle.
 */
export type CompanionVehicleTerms = Readonly<{
  termsKey: string;
  revision: number;
  worldKey: string;
  catalogueId: string;
  modelId: string;
  catalogueRevision: number;
  purpose: "fare" | "fuel" | "purchase";
  price:
    | Readonly<{ status: "unavailable"; reason: "approval_required" }>
    | Readonly<{
        status: "approved";
        approvalKey: string;
        unit: "game-value-unit";
        unitKey: string;
        amount: number;
      }>;
}>;
export type CompanionVehicleDebit = Readonly<{
  ownerResidentId: string;
  payerResidentId: string;
  worldKey: string;
  vehicleId: string;
  expectedVehicleRevision: number;
  /** The payer's revision, not the vehicle owner's revision. */
  expectedResidentRevision: number;
  requestId: string;
  journeyId: string | null;
  purpose: CompanionVehicleTerms["purpose"];
  quantity: number;
  termsKey: string;
  termsRevision: number;
  approvalKey: string;
  unit: "game-value-unit";
  unitKey: string;
  source: "banked" | "carried";
  amount: number;
  balanceBefore: number;
  balanceAfter: number;
}>;
export type CompanionVehicleTransition = Readonly<{
  expectedRevision: number;
  vehicle: CompanionVehicleState;
}>;
type JourneyPlanInput = Readonly<{
  vehicle: CompanionVehicleState;
  owner: CompanionResidentIdentity;
  expectedRevision: number;
  journey: CompanionVehicleJourney;
  clock: CompanionWorldClock;
  sharedNowRealMs: number;
}>;

function requireRecord(value: unknown, label: string): asserts value is Record<string, unknown> {
  if (!value || typeof value !== "object" || Array.isArray(value)) throw new Error(`Provide a ${label} record.`);
}
function requireKey(value: unknown, label: string, maximum = 160): asserts value is string {
  if (!validCompanionGameEconomyKey(value) || value.length > maximum) throw new Error(`Provide a bounded ${label} key.`);
}
function requireRequest(value: unknown): asserts value is string {
  if (!validCompanionGameEconomyRequestId(value)) throw new Error("Provide a bounded transport request ID.");
}
function requireInteger(value: unknown, label: string, minimum = 0, maximum = Number.MAX_SAFE_INTEGER): asserts value is number {
  if (typeof value !== "number" || !Number.isSafeInteger(value) || value < minimum || value > maximum) {
    throw new Error(`Provide a bounded integer ${label}.`);
  }
}
function requireKind(value: unknown): asserts value is CompanionVehicleKind {
  if (value !== "car" && value !== "okada" && value !== "keke" && value !== "danfo") throw new Error("Choose a supported vehicle kind.");
}
function requirePlace(value: unknown): asserts value is CompanionResidentPlace {
  requireRecord(value, "resolved place");
  requireKey(value.placeId, "place"); requireKey(value.instanceId, "instance");
}
function requireModel(value: unknown): asserts value is CompanionVehicleModel {
  requireRecord(value, "catalogue model");
  requireKey(value.catalogueId, "catalogue"); requireKey(value.modelId, "model");
  requireInteger(value.catalogueRevision, "catalogue revision", 1);
  requireKind(value.kind);
  requireInteger(value.tankCapacityUnits, "tank capacity", 1, companionVehicleLimits.fuelUnits);
  requireInteger(value.metresPerFuelUnit, "metres per fuel unit", 1, companionVehicleLimits.metresPerFuelUnit);
}
function requireJourney(value: unknown): asserts value is CompanionVehicleJourney {
  requireRecord(value, "authoritative journey");
  for (const field of ["id", "vehicleId", "ownerResidentId", "clockEpochKey", "routeId"] as const) requireKey(value[field], field);
  requireKey(value.worldKey, "world", 80); requireRequest(value.requestId); requireKind(value.mode);
  requirePlace(value.from); requirePlace(value.to);
  requireInteger(value.routeRevision, "route revision", 1);
  requireInteger(value.startedAtRealMs, "departure timestamp");
  requireInteger(value.arrivesAtRealMs, "arrival timestamp");
  if (value.timeBasis !== "server-ms" || samePlace(value.from, value.to) || value.arrivesAtRealMs <= value.startedAtRealMs) {
    throw new Error("A vehicle journey needs server-ms timing, different resolved places and a later arrival timestamp.");
  }
}
function requireJourneyScope(vehicle: CompanionVehicleState, journey: CompanionVehicleJourney) {
  if (journey.vehicleId !== vehicle.id || journey.ownerResidentId !== vehicle.ownerResidentId
    || journey.worldKey !== vehicle.worldKey || journey.mode !== vehicle.model.kind) {
    throw new Error("Vehicle and journey must have the same owner, world, vehicle ID and kind.");
  }
}
const samePlace = (a: CompanionResidentPlace, b: CompanionResidentPlace) => a.placeId === b.placeId && a.instanceId === b.instanceId;
const freezePlace = (place: CompanionResidentPlace): CompanionResidentPlace => Object.freeze({ placeId: place.placeId, instanceId: place.instanceId });
const fuelForMetres = (metres: number, model: CompanionVehicleModel) => Math.ceil(metres / model.metresPerFuelUnit);

/** Throws on malformed state or cross-owner/world access; does not repair or seed records. */
export function assertCompanionVehicleState(value: unknown, owner: CompanionResidentIdentity): asserts value is CompanionVehicleState {
  if (!owner || owner.actorKind !== "npc" || owner.rosterVersion !== 1
    || owner.id !== createCompanionResidentIdentity(owner.worldKey, owner.actorKey).id) {
    throw new Error("Provide a durable resident owner from the world roster.");
  }
  requireRecord(value, "vehicle");
  requireKey(value.id, "vehicle"); requireKey(value.ownerResidentId, "resident"); requireKey(value.worldKey, "world", 80);
  if (value.version !== 1 || value.ownerResidentId !== owner.id || value.worldKey !== owner.worldKey) {
    throw new Error("Vehicle and resident must belong to the same supported world and owner.");
  }
  requireModel(value.model);
  requireInteger(value.fuelUnits, "available fuel", 0, value.model.tankCapacityUnits);
  requireInteger(value.odometerMetres, "odometer metres"); requireInteger(value.revision, "vehicle revision");
  const activity = value.activity;
  requireRecord(activity, "vehicle activity");
  if (activity.kind === "parked") {
    if ("journey" in activity || "reservedFuelUnits" in activity || "routeMetres" in activity) {
      throw new Error("A parked vehicle cannot also occupy a journey.");
    }
    requirePlace(activity.location);
    if (activity.lastJourney !== null) {
      requireRecord(activity.lastJourney, "last journey");
      requireKey(activity.lastJourney.id, "last journey"); requireRequest(activity.lastJourney.requestId);
      requireKey(activity.lastJourney.clockEpochKey, "last journey epoch");
      requireInteger(activity.lastJourney.completedAtRealMs, "last journey completion timestamp");
    }
  } else if (activity.kind === "traveling") {
    if ("location" in activity || "lastJourney" in activity) throw new Error("A traveling vehicle cannot also be parked.");
    requireJourney(activity.journey);
    if (activity.journey.vehicleId !== value.id || activity.journey.ownerResidentId !== value.ownerResidentId
      || activity.journey.worldKey !== value.worldKey || activity.journey.mode !== value.model.kind) {
      throw new Error("The saved journey must belong to this vehicle and resident.");
    }
    requireInteger(activity.routeMetres, "reserved route metres", 1, companionVehicleLimits.routeMetres);
    requireInteger(activity.reservedFuelUnits, "reserved fuel", 1, value.model.tankCapacityUnits);
    if (activity.reservedFuelUnits !== fuelForMetres(activity.routeMetres, value.model)
      || value.fuelUnits + activity.reservedFuelUnits > value.model.tankCapacityUnits) {
      throw new Error("Reserve the full route's fuel once, within the vehicle's tank capacity.");
    }
    requireInteger(value.odometerMetres + activity.routeMetres, "arrival odometer metres");
  } else {
    throw new Error("A vehicle must be parked or on exactly one authoritative journey.");
  }
}

/** Round distance up once, then reserve whole fuel units for the full route. */
export function quoteCompanionVehicleRoute(
  vehicle: CompanionVehicleState, owner: CompanionResidentIdentity, route: CompanionVehicleRoute,
): CompanionVehicleRouteQuote {
  assertCompanionVehicleState(vehicle, owner);
  if (route?.worldKey !== vehicle.worldKey) throw new Error("Provide a route in the vehicle's world.");
  return quoteCompanionVehicleModelRoute(vehicle.model, route);
}

/** Owner-neutral arithmetic. Account and NPC adapters each validate their own admission. */
export function quoteCompanionVehicleModelRoute(model: CompanionVehicleModel, route: CompanionVehicleRoute): CompanionVehicleRouteQuote {
  requireModel(model);
  requireRecord(route, "route");
  requireKey(route.id, "route"); requireKey(route.worldKey, "route world", 80);
  requireInteger(route.revision, "route revision", 1);
  if (!Array.isArray(route.points)
    || route.points.length < 2 || route.points.length > companionVehicleLimits.routePoints) {
    throw new Error("Provide a bounded route in the vehicle's world.");
  }
  for (const point of route.points) {
    if (!point || !Number.isFinite(point.latitude) || Math.abs(point.latitude) > 90
      || !Number.isFinite(point.longitude) || Math.abs(point.longitude) > 180) {
      throw new Error("Provide finite geographic route points within latitude/longitude bounds.");
    }
  }
  const { routeMetres, fuelUnits } = quoteCompanionVehicleModelDistance(model, companionRouteLength(route.points));
  return Object.freeze<CompanionVehicleRouteQuote>({ routeId: route.id, routeRevision: route.revision, routeMetres, fuelUnits });
}

/** Server-validated geographic distance. Shared by bounded NPC and provider-road adapters. */
export function quoteCompanionVehicleModelDistance(model: CompanionVehicleModel, metres: number) {
  requireModel(model);
  if (!Number.isFinite(metres) || metres <= 0) throw new Error("Provide a positive finite route distance.");
  const routeMetres = Math.ceil(metres);
  requireInteger(routeMetres, "route metres", 1, companionVehicleLimits.routeMetres);
  const fuelUnits = fuelForMetres(routeMetres, model);
  requireInteger(fuelUnits, "route fuel", 1, companionVehicleLimits.fuelUnits);
  return Object.freeze({ routeMetres, fuelUnits });
}

function requireRevision(actual: number, expected: number, label: string) {
  requireInteger(actual, `${label} revision`, 0, Number.MAX_SAFE_INTEGER - 1);
  requireInteger(expected, `expected ${label} revision`, 0, Number.MAX_SAFE_INTEGER - 1);
  if (actual !== expected) throw new Error(`Refresh the ${label} before planning this change.`);
}

/** Quote a debit against the payer's selected simulated cash bucket. No credit, fallback bucket,
 * assets, real balances, transfers or conversions. The backend resolves and verifies approvalKey.
 * Only fares allow a different payer and vehicle owner, both in the same world.
 * This quote alone neither purchases a vehicle nor adds fuel nor permits a journey.
 * Purchase uses a server-resolved candidate vehicle; issuance/ownership writes stay in the backend.
 */
export function quoteCompanionVehicleDebit(input: Readonly<{
  vehicle: CompanionVehicleState;
  vehicleOwner: CompanionResidentIdentity;
  /** The payer's identity, revision and holdings. */
  resident: Readonly<Pick<CompanionResidentState, "identity" | "revision" | "holdings">>;
  expectedVehicleRevision: number;
  expectedResidentRevision: number;
  terms: CompanionVehicleTerms;
  purpose: CompanionVehicleTerms["purpose"];
  quantity: number;
  source: CompanionVehicleDebit["source"];
  requestId: string;
  /** Required for a fare, null for fuel or purchase. */
  journeyId: string | null;
}>): CompanionVehicleDebit {
  const { vehicle, vehicleOwner, resident, terms, purpose, quantity, source, requestId, journeyId } = input;
  assertCompanionVehicleState(vehicle, vehicleOwner);
  const payer = resident?.identity;
  if (!payer || payer.actorKind !== "npc" || payer.rosterVersion !== 1
    || payer.id !== createCompanionResidentIdentity(payer.worldKey, payer.actorKey).id) {
    throw new Error("Provide a durable resident payer from the world roster.");
  }
  if (payer.worldKey !== vehicle.worldKey) throw new Error("Payer and vehicle owner must belong to the same world.");
  if (purpose !== "fare" && payer.id !== vehicle.ownerResidentId) {
    throw new Error("Fuel and purchase require the payer to be the vehicle owner.");
  }
  requireRevision(vehicle.revision, input.expectedVehicleRevision, "vehicle");
  requireRevision(resident.revision, input.expectedResidentRevision, "resident");
  requireRequest(requestId); requireRecord(terms, "vehicle terms");
  requireKey(terms.termsKey, "terms"); requireKey(terms.worldKey, "terms world", 80);
  requireKey(terms.catalogueId, "terms catalogue"); requireKey(terms.modelId, "terms model");
  requireInteger(terms.revision, "terms revision", 1);
  requireInteger(terms.catalogueRevision, "terms catalogue revision", 1);
  if (terms.worldKey !== vehicle.worldKey || terms.catalogueId !== vehicle.model.catalogueId || terms.modelId !== vehicle.model.modelId
    || terms.catalogueRevision !== vehicle.model.catalogueRevision
    || terms.purpose !== purpose || purpose !== "fare" && purpose !== "fuel" && purpose !== "purchase") {
    throw new Error("Use the server terms for this world, catalogue model and transport purpose.");
  }
  requireInteger(quantity, "priced quantity", 1, purpose === "fuel" ? companionVehicleLimits.fuelUnits : 1);
  if (purpose === "fare") {
    requireKey(journeyId, "fare journey");
    if (vehicle.activity.kind === "traveling") {
      if (vehicle.activity.journey.id !== journeyId || vehicle.activity.journey.requestId !== requestId) {
        throw new Error("Fare and active vehicle journey must share their request and journey IDs.");
      }
    } else if (vehicle.activity.lastJourney?.id === journeyId || vehicle.activity.lastJourney?.requestId === requestId) {
      throw new Error("This vehicle journey has already completed.");
    }
  } else if (journeyId !== null) {
    throw new Error("Fuel and purchase quotes must not claim a fare journey.");
  }
  if (purpose === "fuel" && (vehicle.activity.kind !== "parked" || quantity > vehicle.model.tankCapacityUnits - vehicle.fuelUnits)) {
    throw new Error("Refuel only a parked vehicle and only up to its tank capacity.");
  }
  if (purpose === "purchase" && vehicle.activity.kind !== "parked") throw new Error("A purchase candidate must be parked.");
  requireRecord(terms.price, "approved price");
  if (terms.price.status !== "approved" || terms.price.unit !== "game-value-unit") {
    throw new Error("Approved game-value-unit terms are required; unavailable pricing is not free.");
  }
  const price = terms.price;
  requireKey(price.approvalKey, "approval"); requireKey(price.unitKey, "game unit");
  requireInteger(price.amount, "unit price", 1, companionVehicleLimits.amount);
  const holdings = resident.holdings;
  if (!holdings || holdings.unit !== "game-value-unit" || holdings.unitKey !== price.unitKey || !holdings.cash
    || source !== "banked" && source !== "carried") throw new Error("Use one matching resident game unit and an explicit cash bucket.");
  requireInteger(holdings.cash.banked, "banked game value", 0, companionVehicleLimits.amount);
  requireInteger(holdings.cash.carried, "carried game value", 0, companionVehicleLimits.amount);
  const amount = price.amount * quantity;
  requireInteger(amount, "total debit", 1, companionVehicleLimits.amount);
  const balanceBefore = holdings.cash[source];
  if (amount > balanceBefore) throw new Error("Insufficient game value in the selected resident cash bucket.");
  return Object.freeze<CompanionVehicleDebit>({
    ownerResidentId: vehicle.ownerResidentId, payerResidentId: payer.id, worldKey: vehicle.worldKey, vehicleId: vehicle.id,
    expectedVehicleRevision: vehicle.revision, expectedResidentRevision: resident.revision,
    requestId, journeyId, purpose, quantity, termsKey: terms.termsKey, termsRevision: terms.revision,
    approvalKey: price.approvalKey, unit: "game-value-unit", unitKey: price.unitKey, source,
    amount, balanceBefore, balanceAfter: balanceBefore - amount,
  });
}

function freezeJourney(journey: CompanionVehicleJourney): CompanionVehicleJourney {
  return Object.freeze({
    id: journey.id, requestId: journey.requestId, vehicleId: journey.vehicleId, ownerResidentId: journey.ownerResidentId,
    worldKey: journey.worldKey, mode: journey.mode, clockEpochKey: journey.clockEpochKey,
    from: freezePlace(journey.from), to: freezePlace(journey.to), routeId: journey.routeId, routeRevision: journey.routeRevision,
    timeBasis: journey.timeBasis, startedAtRealMs: journey.startedAtRealMs, arrivesAtRealMs: journey.arrivesAtRealMs,
  });
}
function journeyIdentity(journey: CompanionVehicleJourney): string {
  return JSON.stringify([
    journey.id, journey.requestId, journey.vehicleId, journey.ownerResidentId, journey.worldKey, journey.mode,
    journey.clockEpochKey, journey.from.placeId, journey.from.instanceId, journey.to.placeId, journey.to.instanceId,
    journey.routeId, journey.routeRevision, journey.timeBasis, journey.startedAtRealMs, journey.arrivesAtRealMs,
  ]);
}
function requirePlanContext(input: JourneyPlanInput) {
  assertCompanionVehicleState(input.vehicle, input.owner);
  requireRevision(input.vehicle.revision, input.expectedRevision, "vehicle");
  requireJourney(input.journey); requireJourneyScope(input.vehicle, input.journey);
  // Validate the world and epoch here; travel readiness uses sharedNowRealMs directly.
  const time = sampleCompanionWorldClock(input.clock, input.sharedNowRealMs);
  if (time.worldKey !== input.vehicle.worldKey || time.epochKey !== input.journey.clockEpochKey) {
    throw new Error("Use the authoritative journey's world clock and epoch.");
  }
  return time;
}
function transition(vehicle: CompanionVehicleState, activity: CompanionVehicleActivity, fuelUnits: number, odometerMetres: number): CompanionVehicleTransition {
  const model = vehicle.model;
  return Object.freeze<CompanionVehicleTransition>({ expectedRevision: vehicle.revision, vehicle: Object.freeze<CompanionVehicleState>({
    version: 1, id: vehicle.id, ownerResidentId: vehicle.ownerResidentId, worldKey: vehicle.worldKey,
    model: Object.freeze({ catalogueId: model.catalogueId, modelId: model.modelId, catalogueRevision: model.catalogueRevision,
      kind: model.kind, tankCapacityUnits: model.tankCapacityUnits, metresPerFuelUnit: model.metresPerFuelUnit }),
    fuelUnits, odometerMetres, revision: vehicle.revision + 1, activity,
  }) });
}

/** Reserve existing fuel once. Commit with the resident departure, authoritative journey,
 * any required debit and request receipt in one revision-checked backend transaction.
 * Read/sampling paths must not apply plans. Replays return the backend's saved result.
 */
export function planCompanionVehicleDeparture(input: JourneyPlanInput & Readonly<{ route: CompanionVehicleRoute }>): CompanionVehicleTransition {
  const time = requirePlanContext(input);
  const { vehicle, journey } = input;
  if (vehicle.activity.kind !== "parked") throw new Error("This vehicle already occupies a journey.");
  // Leave a revision for arrival as well as departure.
  requireInteger(vehicle.revision, "departure revision", 0, Number.MAX_SAFE_INTEGER - 2);
  if (!samePlace(vehicle.activity.location, journey.from) || input.sharedNowRealMs !== journey.startedAtRealMs) {
    throw new Error("Depart from the vehicle's saved place at the supplied authoritative server timestamp.");
  }
  if (vehicle.activity.lastJourney?.id === journey.id || vehicle.activity.lastJourney?.requestId === journey.requestId) {
    throw new Error("Use a new request and journey after a completed trip.");
  }
  const lastJourney = vehicle.activity.lastJourney;
  if (lastJourney && (lastJourney.clockEpochKey !== time.epochKey || lastJourney.completedAtRealMs > input.sharedNowRealMs)) {
    throw new Error("Use a current shared clock observation; epoch changes need a backend state migration.");
  }
  const quote = quoteCompanionVehicleRoute(vehicle, input.owner, input.route);
  if (quote.routeId !== journey.routeId || quote.routeRevision !== journey.routeRevision) {
    throw new Error("Resolve the authoritative journey's exact route revision.");
  }
  if (quote.fuelUnits > vehicle.fuelUnits) throw new Error("Insufficient fuel for the full route.");
  requireInteger(vehicle.odometerMetres + quote.routeMetres, "arrival odometer metres");
  return transition(vehicle, Object.freeze({
    kind: "traveling", journey: freezeJourney(journey), routeMetres: quote.routeMetres, reservedFuelUnits: quote.fuelUnits,
  }), vehicle.fuelUnits - quote.fuelUnits, vehicle.odometerMetres);
}

/** Complete only the saved journey once it is due. No additional fuel or game-value debit. */
export function planCompanionVehicleArrival(input: JourneyPlanInput): CompanionVehicleTransition {
  requirePlanContext(input);
  const { vehicle, journey } = input;
  if (vehicle.activity.kind !== "traveling") throw new Error("This vehicle has no active journey to complete.");
  if (journeyIdentity(vehicle.activity.journey) !== journeyIdentity(journey)) {
    throw new Error("Request, journey or immutable travel details do not match the vehicle's reservation.");
  }
  if (input.sharedNowRealMs < journey.arrivesAtRealMs) throw new Error("The vehicle has not reached its authoritative arrival timestamp.");
  const odometerMetres = vehicle.odometerMetres + vehicle.activity.routeMetres;
  requireInteger(odometerMetres, "arrival odometer metres");
  return transition(vehicle, Object.freeze({ kind: "parked", location: freezePlace(journey.to),
    lastJourney: Object.freeze({ id: journey.id, requestId: journey.requestId,
      clockEpochKey: journey.clockEpochKey, completedAtRealMs: input.sharedNowRealMs }),
  }), vehicle.fuelUnits, odometerMetres);
}
