import type { PersonalRoom } from "./companion";
import { companionHomeDoor } from "./companionHomeDoor";
import type { CompanionPosition } from "./companionCall";
import type { CompanionMovement, CompanionPace, CompanionWalkPath, CompanionCollisionStop } from "./companionMovement";
import type { CompanionPlaceKind, CompanionPlaceLayout } from "./companionPlaces";
import type { CompanionNightclubActivity, CompanionNightclubCommand } from "./companionNightclub";
import type { CompanionHaulDrop } from "./companionHaulDrops";
import type { CompanionResidentWorldView } from "./companionResidentSimulation";
import { generateCompanionPlaceLayout } from "./companionPlacesLayouts";
import { roomNavigationFor, navigableStandingPoints, type RoomNavigation, type ScenePoint } from "./companionNavigation";
import { townSquareNavigation, townSquareStanding } from "./companionTownSquare";
import { companionCityNavigation, COMPANION_CITY_VERSION } from "./companionCity";
import type { TransitPlan, TransitRide, TransitSnapshot } from "./companionCityTransit";
import type { CompanionCityResidence } from "./companionCityHousing";
import { companionRouteLength, type CompanionGeoPoint, type CompanionJourney, type CompanionJourneyRoute } from "./companionJourney";
import { sampleCompanionWorldClock, companionLagosShiftModifier } from "./companionWorldClock";
import type { CompanionWorldUtilityObservation, CompanionWorldUtilities } from "./companionWorldUtilities";
import type { CompanionPlayerVehicleFareCommitter, CompanionPlayerVehiclePreparedDeparture, CompanionPlayerVehicleTransportSelection } from "./companionPlayerVehicles";
import type { CompanionTransitFare } from "./companionTransitFares";
import type { CompanionRoadsideJourney, CompanionRoadsideVisit, CompanionTravelEndpoint } from "./companionRoadside";
import type { CompanionStreetCrashScope } from "./companionStreetCrashes";
import type { CompanionStreetCrashTrafficState } from "./companionStreetCrashTraffic";
import { companionPlaceExpansionDestinations, type CompanionPlaceExpansionDestination } from "./companionPlacesExpansion";
import { companionWaterfrontPlaces, companionWaterfrontPlace, companionWaterfrontRoute } from "./companionWaterfront";
import type { CompanionDrivingCommand, CompanionDrivingRoute, CompanionPlayerDrivingState } from "./companionPlayerDriving";
import type { CompanionDrivingAcknowledgement } from "./companionPlayerDrivingControls";
import type { CompanionPlayerVehicle } from "./companionPlayerVehicles";
import type { CompanionRoadHornPulse, CompanionRoadTrafficState } from "./companionRoadMechanics";
import type { CompanionVehicleStrain } from "./companionVehicleStrain";
import type { CompanionStreetFactionState } from "./companionStreetFactions";
import type { CompanionVehicleStopInput, CompanionVehicleStopState } from "./companionVehicleStops";
import type { CompanionMovingHawkerOffer } from "./companionMovingHawkers";
import type { CompanionPedestrianRoute } from "./companionStreetPedestrians";
import type { CompanionWaterControl, CompanionWaterDomain, CompanionWaterFleet, CompanionWaterInput } from "./companionWaterTraversal";
import type { RaceAction, RacingSnapshot } from "./companionRacing";

export const COMPANION_WORLD_ID = "lagos-v1";
export type CompanionWorldPlace = { id: string; label: string; kind: CompanionPlaceKind | "town-square" | "own-home";
  anchor: CompanionGeoPoint; placement: "proposed"; seed: string; city?: CompanionPlaceExpansionDestination["city"] };
/** Proposed virtual anchors, not surveyed buildings, physical homes or real incident locations. */
export const companionWorldPlaces: readonly CompanionWorldPlace[] = [
  { id: "town-square", label: "Town Square", kind: "town-square", anchor: { latitude: 6.4506, longitude: 3.4074 }, placement: "proposed", seed: "town-square-v06" },
  { id: "mama-put", label: "Mama put", kind: "mama-put", anchor: { latitude: 6.4524, longitude: 3.4101 }, placement: "proposed", seed: "mama-put-v1" },
  { id: "hospital", label: "Hospital", kind: "hospital", anchor: { latitude: 6.4546, longitude: 3.4114 }, placement: "proposed", seed: "hospital-v1" },
  { id: "own-home", label: "Home", kind: "own-home", anchor: { latitude: 6.4488, longitude: 3.4101 }, placement: "proposed", seed: "own-home-v1" },
  { id: "content-house", label: "Content house", kind: "content-house", anchor: { latitude: 6.4506, longitude: 3.4122 }, placement: "proposed", seed: "content-house-v1" },
  { id: "soccer-pitch", label: "Football pitch", kind: "soccer-pitch", anchor: { latitude: 6.4558, longitude: 3.4074 }, placement: "proposed", seed: "pitch-v1" },
  { id: "arena", label: "Arena", kind: "arena", anchor: { latitude: 6.4564, longitude: 3.4122 }, placement: "proposed", seed: "arena-v1" },
  { id: "station", label: "Police station", kind: "station", anchor: { latitude: 6.4536, longitude: 3.4074 }, placement: "proposed", seed: "station-v1" },
  { id: "skyline", label: "Skyline restaurant", kind: "skyline", anchor: { latitude: 6.4475, longitude: 3.4122 }, placement: "proposed", seed: "skyline-v1" },
  { id: "fine-dining", label: "Fine dining", kind: "fine-dining", anchor: { latitude: 6.4468, longitude: 3.4101 }, placement: "proposed", seed: "dining-v1" },
  { id: "nightclub", label: "Nightclub", kind: "nightclub", anchor: { latitude: 6.4488, longitude: 3.4074 }, placement: "proposed", seed: "nightclub-v2" },
  ...companionPlaceExpansionDestinations,
  ...companionWaterfrontPlaces,
];
export const companionWorldPlace = (id: string) => companionWorldPlaces.find(place => place.id === id);
export function companionWorldRoute(fromId: string, toId: string): CompanionJourneyRoute | null {
  const from = companionWorldPlace(fromId), to = companionWorldPlace(toId);
  if (!from || !to || from.id === to.id) return null;
  if (companionWaterfrontPlace(fromId)?.level === "abutment" || companionWaterfrontPlace(toId)?.level === "abutment") return null;
  const water = companionWaterfrontRoute(fromId, toId);
  if (water) return water.route;
  if (companionWaterfrontPlace(fromId)?.boatOnly || companionWaterfrontPlace(toId)?.boatOnly) return null;
  // Frozen proposed corridors avoid a new paid Directions API. Geographic road verification is outstanding.
  const points = [from.anchor, { latitude: from.anchor.latitude, longitude: 3.4101 }, { latitude: to.anchor.latitude, longitude: 3.4101 }, to.anchor]
    .filter((point, index, all) => index === 0 || point.latitude !== all[index - 1].latitude || point.longitude !== all[index - 1].longitude);
  return { id: `${from.id}:${to.id}`, version: "lagos-corridors-v1", placement: "proposed", points, lengthMetres: companionRouteLength(points) };
}
export type CompanionWorldInstance = { id: string; placeId: string; seed: string; generatorVersion: string; layoutId: string;
  sharedStartedAt: number;
  /** Optional hydrated view. Production JSON derives the plan from navigation.revision. */
  transitPlan?: TransitPlan;
  roadside?: Extract<CompanionTravelEndpoint, { kind: "road" }>;
  navigation: RoomNavigation; spawns: readonly ScenePoint[]; exit: ScenePoint; layout: CompanionPlaceLayout | null; room: PersonalRoom | null };
export function resolveCompanionWorldInstance(scope: string, placeId: string, home?: PersonalRoom | null, homeOwnerId?: string): CompanionWorldInstance {
  const place = companionWorldPlace(placeId);
  if (!place) throw new Error("Choose a mapped game place.");
  const id = `${scope}:${placeId}${placeId === "own-home" ? `:${homeOwnerId ?? "owner"}` : ""}`;
  if (place.kind === "town-square") {
    const spawns: ScenePoint[] = townSquareStanding.map(point => [point.x, townSquareNavigation.floorY, point.z]);
    const navigation = companionCityNavigation();
    return { id, placeId, sharedStartedAt: 0, seed: COMPANION_CITY_VERSION, generatorVersion: COMPANION_CITY_VERSION, layoutId: navigation.revision,
      navigation,
      spawns, exit: spawns[0], layout: null, room: null };
  }
  if (place.kind === "own-home") {
    if (!home) throw new Error("Your saved Home is unavailable.");
    const navigation = roomNavigationFor(home);
    const spawns: ScenePoint[] = navigableStandingPoints(home).map(p => [p.x, navigation.floorY, p.z]);
    const exit = companionHomeDoor(home).point;
    return { id: `${id}:${home.id}`, placeId, sharedStartedAt: 0, seed: home.id, generatorVersion: "saved-room-v1", layoutId: navigation.revision,
      navigation, spawns, exit, layout: null, room: home };
  }
  const layout = generateCompanionPlaceLayout({ placeId, kind: place.kind, seed: place.seed });
  return { id, placeId, sharedStartedAt: 0, seed: place.seed, generatorVersion: layout.generatorVersion, layoutId: layout.id,
    navigation: layout.navigation, spawns: layout.spawns.map(spawn => spawn.point), exit: layout.entrances.find(entrance => entrance.role === "world")!.point,
    layout, room: null };
}
export type CompanionWorldPlayer = { userId: string; characterId: string; gameRoomId: string; placeId: string; instanceId: string;
  revision: number; position: CompanionPosition; movement: CompanionMovement | null; moveSequence: number;
  status: "present" | "traveling" | "recovering"; health: number; journey: CompanionRoadsideJourney | null; homeRoomId: string;
  roadside?: CompanionRoadsideVisit | null;
  transitRide?: TransitRide | null;
  driving?: CompanionPlayerDrivingState | null;
  waterControl?: CompanionWaterControl | null;
  nightclub?: CompanionNightclubActivity | null;
  residence?: CompanionCityResidence | null;
  drawnWeaponId?: "bat" | "pistol" | null };
export type CompanionWorldFriendResidence = { userId: string; name: string; residence: CompanionCityResidence };
export type CompanionWorldUtilityReceipt = { serverNow: number; gameRoomId: string | null; instanceId: string; revision: number;
  observation: CompanionWorldUtilityObservation | null };
export type CompanionWorldDrivingCatalog = { route: CompanionDrivingRoute; vehicle: CompanionPlayerVehicle | null };
/** Decoded private receipts from the existing world endpoint, never client outcome authority. */
export type CompanionWorldRoad = { scope: import("./companionRoadMechanics").CompanionRoadScope;
  traffic: CompanionRoadTrafficState; horn: CompanionRoadHornPulse | null; strain: CompanionVehicleStrain };
export type CompanionWorldFactions = { state: CompanionStreetFactionState; stops: CompanionVehicleStopState | null };
export type CompanionWorldHawkers = { offers: readonly CompanionMovingHawkerOffer[]; routes: readonly CompanionPedestrianRoute[];
  /** Latest accepted return per body survives missed snapshots and reconnect. */
  releasedRoutes?: readonly CompanionPedestrianRoute[] };
export type CompanionWorldWater = { domain: CompanionWaterDomain; fleet: CompanionWaterFleet };
export type CompanionWorldStreetFields = { drivingCatalog?: CompanionWorldDrivingCatalog | null;
  road?: CompanionWorldRoad | null; factions?: CompanionWorldFactions | null;
  hawkers?: CompanionWorldHawkers | null; water?: CompanionWorldWater | null };
export type CompanionWorldSnapshot = { player: CompanionWorldPlayer | null; instance: CompanionWorldInstance | null;
  racing?: RacingSnapshot | null;
  /** Local-clock presentation only. Gameplay and reach checks keep using player. */
  localMovement?: { instanceId: string; characterId: string; movement: CompanionMovement; stop?: true } | null;
  friendResidences?: readonly CompanionWorldFriendResidence[];
  /** Null scope keeps legacy scenery. A retained scope without a receipt holds traffic. */
  streetScope?: CompanionStreetCrashScope | null;
  streetTraffic: CompanionStreetCrashTrafficState | null;
  transit?: TransitSnapshot | null;
  transitAvailability?: { boarding: "available"; farePolicy: "lagos-city-seat-fares-v1" }
    | { boarding: "unavailable"; reason: "fare-unconfigured"; message: string } | null;
  transitFare?: CompanionTransitFare | null;
  transitLifeId?: string | null;
  /** Retain the exact quoted action until an ambiguous server outcome is resolved. */
  transitFareRetryable?: boolean;
  transitFareRetryAction?: "board" | "cancel" | null;
  utilityObservation?: CompanionWorldUtilityObservation | null;
  /** Display time from the original admitted receipt's monotonic age. Never an authority timestamp. */
  displayNow?: number;
  displayRemainingMs?: number;
  utilities?: CompanionWorldUtilities | null;
  /** Include in the host's display identity so reload requests a fresh rendered receipt for the same place. */
  renderAttempt: number;
  peers: CompanionWorldPlayer[]; drops: CompanionHaulDrop[]; moreDrops: boolean; residentWorld: CompanionResidentWorldView | null;
  /** Local page controls only. The renderer continues to consume the bounded drops array. */
  dropBrowse?: { ready: boolean; more: boolean; error: string | null } | null;
  /** Account-private receipt. Cleared checkpoints are no longer active police encounters. */
  checkpointStatus?: { journeyId: string; phase: "checking" | "cleared" | "searching" | "detained" } | null;
  /** Last admitted place for continuing audio. Interactive rendering may be hidden independently. */
  audioInstanceId?: string | null;
  renderedInstanceId: string | null; serverNow: number; ready: boolean; busy: boolean; error: string | null; serverOffset: number } & CompanionWorldStreetFields;
export const emptyCompanionWorld: CompanionWorldSnapshot = { player: null, instance: null, friendResidences: [], streetScope: null, streetTraffic: null,
  racing: null,
  drivingCatalog: null, road: null, factions: null, hawkers: null, water: null,
  renderAttempt: 0, peers: [], drops: [], moreDrops: false, residentWorld: null, audioInstanceId: null, renderedInstanceId: null, serverNow: 0, ready: false, busy: false, error: null, serverOffset: 0 };
/** Display only. Existing view timers supply server-adjusted time; this never advances outcomes. */
export function companionWorldTimeLabel(world: CompanionResidentWorldView | null | undefined, serverNow: number): string | null {
  if (!world || world.worldKey !== world.clock?.worldKey) return null;
  try {
    const time = sampleCompanionWorldClock(world.clock, serverNow);
    return `Day ${time.day + 1} · ${String(time.hour).padStart(2, "0")}:${String(time.minute).padStart(2, "0")}`;
  } catch { return null; }
}
export function companionWorldConditionLabel(world: CompanionResidentWorldView | null | undefined, serverNow: number): string | null {
  if (!world || world.worldKey !== world.clock?.worldKey) return null;
  try {
    const time = sampleCompanionWorldClock(world.clock, serverNow);
    const shift = companionLagosShiftModifier(time.day, time.phase);
    return shift.label;
  } catch { return null; }
}
export type CompanionWorldTravelResult = { kind: "prepared"; departure: CompanionPlayerVehiclePreparedDeparture }
  | { kind: "committed"; departureId: string; journey: CompanionJourney; quote: import("./companionGameEconomy").CompanionGameEconomyQuote | null };
export type CompanionWorldDrivingAction = { type: "driving-board"; vehicleId: string; vehicleRevision?: number; routeId?: string }
  | { type: "driving"; command: CompanionDrivingCommand };
export type CompanionWorldWaterAction = { type: "water"; action: "board"; craftId?: string }
  | { type: "water"; action: "input" | "stop" | "dock" | "recover"; controlId: string; input?: CompanionWaterInput };
export type CompanionWorldStreetAction = { type: "streetAction"; command:
  | ({ kind: "stop" } & Omit<CompanionVehicleStopInput, "requestId" | "sequence">)
  | { kind: "terminal-confirm"; stopId: string; revision: number }
  | { kind: "cool"; serviceId: string }
  | { kind: "hawker-cancel"; offerId: string; revision: number } };
export type CompanionWorldCommand = { type: "enter" } | { type: "travel"; placeId: string; transport?: CompanionPlayerVehicleTransportSelection;
  roadsideTarget?: { vehicleId: string; vehicleRevision: number }; walk?: boolean; walkingExit?: boolean } | { type: "arrive"; journeyId: string }
  | { type: "journeySpeed"; journeyId: string; rate: 1 | 4 }
  | { type: "door"; roomId: string } | { type: "service"; serviceId: string; revision: number; fareQuoteId?: string }
  | { type: "collectDrop"; dropId: string; instanceId: string; revision: number }
  | { type: "nightclub"; instanceId: string; revision: number; command: CompanionNightclubCommand }
  | CompanionWorldDrivingAction | CompanionWorldWaterAction | CompanionWorldStreetAction
  | { type: "race"; command: RaceAction };
export interface CompanionWorldStore {
  getSnapshot(): CompanionWorldSnapshot; subscribe(listener: () => void): () => void; select(gameRoomId: string | null): void;
  getDisplaySnapshot(): CompanionWorldSnapshot;
  observeUtilities(receipt: CompanionWorldUtilityReceipt | null): void;
  /** Browse the next nearby page, retry it after an error, or reset after the last page. */
  nextDrops(): void;
  send(command: Extract<CompanionWorldCommand, { type: "driving" }>): Promise<CompanionDrivingAcknowledgement>;
  send(command: Exclude<CompanionWorldCommand, { type: "driving" }>): Promise<CompanionWorldTravelResult | undefined>;
  send(command: CompanionWorldCommand): Promise<CompanionWorldTravelResult | CompanionDrivingAcknowledgement | undefined>;
  move(position: CompanionPosition, pace?: CompanionPace, path?: CompanionWalkPath): Promise<void>; stop(collision?: CompanionCollisionStop): Promise<void>;
  /** Captured payer fare receipt, independent of the currently selected visit. */
  commitFare: CompanionPlayerVehicleFareCommitter;
  /** A rendered receipt starts one acknowledgement; failures allow the next receipt to retry. */
  ready(instanceId: string): void;
  /** Report a failed body/scene for this exact visit. Ordinary coverage never calls this. */
  presentationFailed(instanceId: string, revision: number): void;
  /** Revoke local rendering and pending acknowledgements, preserving the call and journey. Redundant calls do nothing. */
  revokeRendered(options?: { preserveAudio?: boolean }): void;
  /** Replace the subscription and advance renderAttempt. Keep cached layouts; require a new frame receipt and acknowledgement. */
  reload(): void;
  dispose(): void;
}

export function companionWorldPlaceLabel(placeId: string): string {
  return placeId.startsWith("roadside:") ? "Roadside" : companionWorldPlace(placeId)?.label ?? "Destination";
}

/** Endpoint coordinates come from the admitted journey, never a nearest-place fallback. */
export function companionWorldJourneyPlace(journey: CompanionRoadsideJourney, side: "from" | "to") {
  const id = side === "from" ? journey.fromPlaceId : journey.toPlaceId;
  const endpoint = side === "from" ? journey.fromEndpoint : journey.toEndpoint;
  const point = endpoint?.point ?? companionWorldPlace(id)?.anchor
    ?? journey.route.points[side === "from" ? 0 : journey.route.points.length - 1];
  return { id, label: companionWorldPlaceLabel(id), point };
}
