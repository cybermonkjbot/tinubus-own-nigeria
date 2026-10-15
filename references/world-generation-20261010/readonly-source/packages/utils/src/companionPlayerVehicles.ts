import { companionResidentVehicleRules, quoteCompanionVehicleModelRoute,
  type CompanionVehicleKind, type CompanionVehicleModel, type CompanionVehicleRoute } from "./companionVehicles";
import { validCompanionGameEconomyKey, type CompanionGameEconomyQuote } from "./companionGameEconomy";
import type { CompanionGeoPoint } from "./companionJourney";
import type { CompanionPlayerDrivingState, CompanionDrivingPose } from "./companionPlayerDriving";

/** Noncash source tuning. No stock, fuel or account funds are created on import. */
export const companionPlayerVehicleRules = Object.freeze({
  version: 1, worldKey: "lagos-v1", catalogueId: "lagos-player-transport", catalogueRevision: 1,
  unitKey: "lagos-credit", sourceKey: "lagos-player-transport-v1", maxOwned: 4,
  fuelPacks: Object.freeze([1, 5, 10, 20]), quoteTtlMs: 120_000,
  models: Object.freeze(companionResidentVehicleRules.models.map(model => Object.freeze({
    ...model, purchase: model.kind === "car" ? 300 : model.kind === "okada" ? 120 : model.kind === "keke" ? 180 : 400,
  }))),
});
export const companionPlayerVehicleModel = (kind: CompanionVehicleKind): CompanionVehicleModel => {
  const model = companionPlayerVehicleRules.models.find(value => value.kind === kind);
  if (!model) throw new Error("Choose a supported vehicle.");
  return Object.freeze({ catalogueId: companionPlayerVehicleRules.catalogueId, catalogueRevision: 1,
    modelId: model.modelId, kind, tankCapacityUnits: model.tankCapacityUnits, metresPerFuelUnit: model.metresPerFuelUnit });
};
export type CompanionPlayerVehicleLocation =
  | Readonly<{ kind: "place"; worldKey: string; placeId: string; layoutId: string;
      homeOwnerId: string | null; homeRoomId: string | null }>
  | Readonly<{ kind: "road"; worldKey: string; point: Readonly<CompanionGeoPoint> }>;
/** Ownership is an authenticated account ID, never a fake resident identity. */
export type CompanionPlayerVehicle = Readonly<{
  vehicleId: string; ownerUserId: string | null; model: CompanionVehicleModel; revision: number;
  fuelUnits: number; condition: number; odometerMetres: number; location: CompanionPlayerVehicleLocation;
  activeTripId: string | null; forSale: boolean; forHire: boolean; asset: null;
  driving?: CompanionPlayerDrivingState | null;
  /** Fractional fuel debit survives driving exits and ordinary timed journeys. */
  drivingFuelMetres?: number;
  drivingParked?: Readonly<{ navigationRevision: string; pose: CompanionDrivingPose; fuelMetres: number; instanceId: string }> | null;
}>;
export function sameCompanionPlayerVehiclePlace(a: CompanionPlayerVehicleLocation, b: CompanionPlayerVehicleLocation): boolean {
  if (a.kind === "road" && b.kind === "road") return a.worldKey === b.worldKey
    && a.point.latitude === b.point.latitude && a.point.longitude === b.point.longitude;
  return a.kind === "place" && b.kind === "place" && a.worldKey === b.worldKey && a.placeId === b.placeId
    && a.layoutId === b.layoutId && a.homeOwnerId === b.homeOwnerId && a.homeRoomId === b.homeRoomId;
}
export function assertCompanionPlayerVehicle(vehicle: CompanionPlayerVehicle): void {
  const integer = (value: number, max = Number.MAX_SAFE_INTEGER - 1) => Number.isSafeInteger(value) && value >= 0 && value <= max;
  if (!vehicle || !validCompanionGameEconomyKey(vehicle.vehicleId) || vehicle.location?.worldKey !== companionPlayerVehicleRules.worldKey
    || vehicle.ownerUserId !== null && !validCompanionGameEconomyKey(vehicle.ownerUserId)
    || !integer(vehicle.revision) || !integer(vehicle.condition, 100) || !integer(vehicle.fuelUnits, vehicle.model?.tankCapacityUnits)
    || !integer(vehicle.odometerMetres) || vehicle.asset !== null || typeof vehicle.forSale !== "boolean" || typeof vehicle.forHire !== "boolean"
    || vehicle.activeTripId !== null && (typeof vehicle.activeTripId !== "string" || !vehicle.activeTripId || vehicle.activeTripId.length > 240)
    || vehicle.ownerUserId !== null && (vehicle.forSale || vehicle.forHire)) throw new Error("Vehicle state needs review.");
  const model = companionPlayerVehicleModel(vehicle.model.kind);
  if (Object.keys(model).some(key => model[key as keyof typeof model] !== vehicle.model[key as keyof typeof model]))
    throw new Error("Use the vehicle's immutable catalogue model.");
  if (vehicle.drivingFuelMetres !== undefined && (!Number.isFinite(vehicle.drivingFuelMetres)
    || vehicle.drivingFuelMetres < 0 || vehicle.drivingFuelMetres >= model.metresPerFuelUnit))
    throw new Error("Vehicle fuel distance needs review.");
  const location = vehicle.location;
  if (location.kind === "place") {
    if (!validCompanionGameEconomyKey(location.placeId) || typeof location.layoutId !== "string" || !location.layoutId || location.layoutId.length > 160_000
      || location.placeId === "own-home" && (!validCompanionGameEconomyKey(location.homeOwnerId) || !validCompanionGameEconomyKey(location.homeRoomId))
      || location.placeId !== "own-home" && (location.homeOwnerId !== null || location.homeRoomId !== null))
      throw new Error("Use the vehicle's resolved parked place.");
  } else if (location.kind !== "road" || !Number.isFinite(location.point?.latitude) || Math.abs(location.point.latitude) > 90
    || !Number.isFinite(location.point?.longitude) || Math.abs(location.point.longitude) > 180)
    throw new Error("Use a finite parked road position.");
  if (vehicle.driving && (vehicle.activeTripId || vehicle.driving.vehicleId !== vehicle.vehicleId
    || vehicle.driving.vehicleRevision !== vehicle.revision || vehicle.driving.driver.userId !== vehicle.ownerUserId
    || vehicle.driving.occupants.length !== 1 || vehicle.model.kind !== "keke"))
    throw new Error("Vehicle driving ownership needs review.");
}
export function quoteCompanionPlayerVehicleRoute(vehicle: CompanionPlayerVehicle, route: CompanionVehicleRoute) {
  assertCompanionPlayerVehicle(vehicle);
  if (route.worldKey !== vehicle.location.worldKey || vehicle.activeTripId || vehicle.driving || vehicle.condition <= 0)
    throw new Error("Choose a parked, working vehicle in this world.");
  return quoteCompanionVehicleModelRoute(vehicle.model, route);
}

export type CompanionPlayerVehicleScope = Readonly<{
  gameRoomId: string; instanceId: string; lifeId: string; revision: number; serviceId: string;
}>;
export type CompanionPlayerVehicleCatalog = Readonly<{
  owned: readonly CompanionPlayerVehicle[]; sale: readonly CompanionPlayerVehicle[]; hire: readonly CompanionPlayerVehicle[];
}>;
/** The server supplies the destination identity; clients never build it from coordinates. */
export type CompanionPlayerVehicleRecoveryTarget = Readonly<{
  vehicleId: string; vehicleRevision: number; placeId: string;
}>;
export type CompanionPlayerVehicleRecovery = Readonly<{
  owned: readonly CompanionPlayerVehicle[]; targets: readonly CompanionPlayerVehicleRecoveryTarget[];
}>;
export type CompanionPlayerVehicleRecoverySelection = Readonly<{
  scope: CompanionPlayerVehicleScope; target: CompanionPlayerVehicleRecoveryTarget;
}>;
export type CompanionPlayerVehiclePurchaseSelection =
  | Readonly<{ purpose: "purchase"; vehicleId: string }>
  | Readonly<{ purpose: "fuel"; vehicleId: string; quantity: 1 | 5 | 10 | 20 }>;
/** A selection asks the existing host to prepare travel. It grants no boarding or payment authority. */
export type CompanionPlayerVehicleTransportSelection = Readonly<{
  scope: CompanionPlayerVehicleScope; vehicleId: string; vehicleRevision: number; mode: "owned" | "fare"; serviceId: string;
}>;
/** Display binding supplied by the host only after its existing departure preparation succeeds. */
export type CompanionPlayerVehiclePreparedDeparture = Readonly<{
  scope: CompanionPlayerVehicleScope; departureId: string; vehicleId: string; vehicleRevision: number;
  mode: "fare"; serviceId: string; destination: CompanionPlayerVehicleLocation; routeMetres: number; expiresAt: number;
  /** Captured by world preparation and retained with an uncertain fare request. */
  worldTravel?: Readonly<{ placeId: string; roadsideTarget?: CompanionPlayerVehicleRecoveryTarget }>;
}>;
export type CompanionPlayerVehicleConfirmation = Readonly<{
  quote: CompanionGameEconomyQuote; vehicleId: string; vehicleRevision: number; purpose: "purchase" | "fuel" | "fare";
  quantity: number; fuelBefore: number; fuelAfter: number; tankCapacityUnits: number;
  departureId: string | null; destination: CompanionPlayerVehicleLocation | null; routeMetres: number | null;
}>;
export type CompanionPlayerVehicleFareCommit = Readonly<{
  scope: CompanionPlayerVehicleScope; departure: CompanionPlayerVehiclePreparedDeparture;
  confirmation: CompanionPlayerVehicleConfirmation; requestId: string;
}>;
/** Throws mean an uncertain outcome. Return rejected only for a known authoritative rejection. */
export type CompanionPlayerVehicleFareCommitter = (request: CompanionPlayerVehicleFareCommit) => Promise<
  { quote: CompanionGameEconomyQuote } | { rejected: string }
>;
export type CompanionPlayerVehicleAction =
  | Readonly<{ type: "quote"; selection: CompanionPlayerVehiclePurchaseSelection }>
  | Readonly<{ type: "fare"; departureId: string }>
  | Readonly<{ type: "confirm" | "cancel"; quoteId: string }>;
export type CompanionPlayerVehicleSnapshot = Readonly<{
  scope: CompanionPlayerVehicleScope | null; catalog: CompanionPlayerVehicleCatalog | null;
  recovery: CompanionPlayerVehicleRecovery | null; recoveryOnly: boolean;
  confirmation: CompanionPlayerVehicleConfirmation | null; confirmationScope: CompanionPlayerVehicleScope | null;
  quoteId: string | null; departure: CompanionPlayerVehiclePreparedDeparture | null;
  ready: boolean; loading: boolean; busy: boolean; retryable: boolean; fareReady: boolean; error: string | null; notice: string | null;
}>;
export const emptyCompanionPlayerVehicleSnapshot: CompanionPlayerVehicleSnapshot = Object.freeze({
  scope: null, catalog: null, recovery: null, recoveryOnly: false, confirmation: null, confirmationScope: null, quoteId: null, departure: null,
  ready: false, loading: false, busy: false, retryable: false, fareReady: false, error: null, notice: null,
});
export interface CompanionPlayerVehicleStore {
  getSnapshot(): CompanionPlayerVehicleSnapshot;
  subscribe(listener: () => void): () => void;
  select(scope: CompanionPlayerVehicleScope | null, recoveryOnly?: boolean): void;
  selectDeparture(departure: CompanionPlayerVehiclePreparedDeparture | null): void;
  send(action: CompanionPlayerVehicleAction, requestId: string): Promise<CompanionGameEconomyQuote | null>;
  retry(): Promise<CompanionGameEconomyQuote | null>;
  reload(): void;
  recoverQuote(quoteId: string): void;
  dispose(): void;
}
export const companionPlayerVehicleScopeKey = (scope: CompanionPlayerVehicleScope | null): string => scope
  ? JSON.stringify([scope.gameRoomId, scope.instanceId, scope.lifeId, scope.revision, scope.serviceId]) : "none";
export const companionPlayerVehicleDepartureKey = (departure: CompanionPlayerVehiclePreparedDeparture | null): string => departure
  ? JSON.stringify([companionPlayerVehicleScopeKey(departure.scope), departure.departureId, departure.vehicleId, departure.vehicleRevision,
    departure.mode, departure.serviceId, departure.routeMetres, departure.expiresAt, departure.destination.kind, departure.destination.worldKey,
    departure.destination.kind === "place" ? [departure.destination.placeId, departure.destination.layoutId, departure.destination.homeOwnerId, departure.destination.homeRoomId]
      : [departure.destination.point.latitude, departure.destination.point.longitude], departure.worldTravel ?? null]) : "none";
export const companionPlayerVehicleRequestId = () => `transport-${Date.now().toString(36)}-${Math.random().toString(36).slice(2)}-action`;
export const companionPlayerVehicleLabel = (kind: CompanionVehicleKind): string => ({ car: "Car", okada: "Okada", keke: "Keke", danfo: "Danfo" })[kind];
export function companionPlayerVehicleLocationLabel(location: CompanionPlayerVehicleLocation): string {
  if (location.kind === "road") return `Roadside at ${location.point.latitude.toFixed(5)}, ${location.point.longitude.toFixed(5)}`;
  return location.placeId === "own-home" ? `Home, room ${location.homeRoomId}` : location.placeId.replace(/-/g, " ");
}
export function companionPlayerVehiclePositionLabel(vehicle: CompanionPlayerVehicle): string {
  const location = companionPlayerVehicleLocationLabel(vehicle.location);
  if (vehicle.driving) return vehicle.driving.mode === "suspended" ? "Driving paused" : "On the road";
  return vehicle.activeTripId ? `Journey from ${location}` : vehicle.location.kind === "road" ? location : `Parked at ${location}`;
}
export function companionPlayerVehicleUnavailable(vehicle: CompanionPlayerVehicle, here: CompanionPlayerVehicleLocation | null): string | null {
  if (vehicle.driving) return "Stop and exit driving first";
  if (vehicle.activeTripId) return "Journey in progress";
  if (!here || !sameCompanionPlayerVehiclePlace(vehicle.location, here))
    return vehicle.location.kind === "road" ? "Reach this vehicle to use it" : "Parked elsewhere";
  if (vehicle.condition <= 0) return "Repair required";
  return null;
}
export function companionPlayerVehicleRecoveryTarget(recovery: CompanionPlayerVehicleRecovery | null,
  vehicle: CompanionPlayerVehicle, here: CompanionPlayerVehicleLocation | null): CompanionPlayerVehicleRecoveryTarget | null {
  if (vehicle.location.kind !== "road" || vehicle.activeTripId || here && sameCompanionPlayerVehiclePlace(vehicle.location, here)) return null;
  return recovery?.targets.find(target => target.vehicleId === vehicle.vehicleId && target.vehicleRevision === vehicle.revision) ?? null;
}
export function companionPlayerVehicleConfirmationTitle(confirmation: CompanionPlayerVehicleConfirmation | null): string {
  return confirmation?.purpose === "purchase" ? "Confirm vehicle purchase" : confirmation?.purpose === "fuel" ? "Confirm refueling"
    : confirmation?.purpose === "fare" ? "Confirm fare" : "Transport";
}
export type CompanionVehicleSheetProps = {
  store: CompanionPlayerVehicleStore | null; scope: CompanionPlayerVehicleScope | null;
  here: CompanionPlayerVehicleLocation | null; preparedDeparture?: CompanionPlayerVehiclePreparedDeparture | null;
  /** Browses owned vehicles without a counter. Purchase/refuel/hire controls stay hidden. */
  recoveryOnly?: boolean;
  visible: boolean; available?: boolean; now: number; onClose(): void;
  onSelectTransport?: (selection: CompanionPlayerVehicleTransportSelection) => void | Promise<void>;
  onSelectRecovery?: (selection: CompanionPlayerVehicleRecoverySelection) => void | Promise<void>;
};
/** Render/action gating only. The server rechecks all permissions, prices, stock and fuel. */
export function companionPlayerVehicleSheetView(snapshot: CompanionPlayerVehicleSnapshot, props: CompanionVehicleSheetProps) {
  const current = props.visible && snapshot.recoveryOnly === (props.recoveryOnly ?? false)
    && companionPlayerVehicleScopeKey(snapshot.scope) === companionPlayerVehicleScopeKey(props.scope);
  const confirmation = current ? snapshot.confirmation : null;
  const quote = confirmation?.quote ?? null;
  const price = quote?.service.price;
  const gamePrice = price?.unit === "game-value-unit" && price.unitKey === "lagos-credit"
    && price.source.kind === "banked_game_value" && price.source.key === "haul" ? price : null;
  const paused = snapshot.busy || snapshot.retryable;
  const enabled = current && !!props.scope && !!props.store && props.available !== false && !paused;
  const departure = current && companionPlayerVehicleDepartureKey(snapshot.departure) === companionPlayerVehicleDepartureKey(props.preparedDeparture ?? null)
    ? snapshot.departure : null;
  const expiry = quote ? Math.min(quote.expiresAt, confirmation?.purpose === "fare" ? departure?.expiresAt ?? 0 : quote.expiresAt) : 0;
  const expired = !Number.isFinite(props.now) || props.now >= expiry;
  const confirming = !!snapshot.quoteId && current;
  const canConfirm = enabled && !!quote && !!gamePrice && quote.status === "quoted" && quote.service.available && !expired
    && (!snapshot.recoveryOnly || confirmation?.purpose === "fare")
    && companionPlayerVehicleScopeKey(snapshot.confirmationScope) === companionPlayerVehicleScopeKey(props.scope)
    && (confirmation?.purpose !== "fare" || snapshot.fareReady && !!departure && departure.departureId === confirmation.departureId
      && departure.vehicleId === confirmation.vehicleId && departure.vehicleRevision === confirmation.vehicleRevision);
  const browsing = current && !confirming && !props.preparedDeparture && !snapshot.departure;
  const catalog = browsing && !snapshot.recoveryOnly ? snapshot.catalog : null;
  const recovery = browsing ? snapshot.recovery : null;
  return { current, confirmation, quote, gamePrice, paused, enabled, departure, expiry, expired, confirming, canConfirm,
    catalog, recovery, owned: recovery?.owned ?? catalog?.owned ?? null,
    canRefuel: !!catalog && props.here?.kind === "place" };
}
