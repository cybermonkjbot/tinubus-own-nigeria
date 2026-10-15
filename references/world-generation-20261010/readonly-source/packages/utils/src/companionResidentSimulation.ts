import type { CompanionCorridorRoute } from "./companionJourney";
import { sampleCompanionRoute } from "./companionJourney";
import type { CompanionMovement } from "./companionMovement";
import { CALL_MOVEMENT_MAX_POINTS, companionMovementEndsAt, sampleCompanionMovement } from "./companionMovement";
import { companionPlaceKinds, type CompanionPlaceAssetPart, type CompanionPlacePose } from "./companionPlaces";
import { companionPersistentResidentKeys, companionResidentKeys, type CompanionResidentAnchor, type CompanionResidentIdentity, type CompanionResidentState,
  type CompanionResidentTravelMode, type CompanionResidentWorth } from "./companionResidents";
import { companionVehicleLimits, type CompanionVehicleModel } from "./companionVehicles";
import type { CompanionWorldInstance } from "./companionWorld";
import { sampleCompanionWorldClock, type CompanionWorldClock } from "./companionWorldClock";

/** One journey includes leaving the building, map travel and entering the destination.
 * It is sampled locally; only the backend may complete it or change place membership.
 */
export type CompanionResidentJourney = Readonly<{
  id: string;
  from: CompanionResidentAnchor;
  to: CompanionResidentAnchor;
  mode: CompanionResidentTravelMode;
  vehicleId: string | null;
  transport?: Readonly<{
    vehicleId: string;
    ownerResidentId: string;
    riderResidentId: string | null;
    model: CompanionVehicleModel;
    asset: Readonly<{ key: string; sourceYaw: number }> | null;
  }>;
  timeBasis: "server-ms";
  departure: CompanionMovement;
  route: CompanionCorridorRoute;
  routeStartedAt: number;
  routeEndsAt: number;
  arrival: CompanionMovement;
  arrivesAt: number;
  arrivalAction: "rest" | "work" | "socialize";
}>;

export type CompanionResidentView = Readonly<{
  identity: CompanionResidentIdentity;
  revision: number;
  clockEpochKey: string;
  updatedAtGameMinute: number;
  health: CompanionResidentState["health"];
  needs: CompanionResidentState["needs"];
  worth: CompanionResidentWorth;
  activity: CompanionResidentState["activity"];
  journey: CompanionResidentJourney | null;
  localMove: Readonly<{ movement: CompanionMovement; to: CompanionResidentAnchor; action: "rest" | "work" | "socialize" }> | null;
  /** Replace this existing actor when resident rendering is connected; never append a duplicate. */
  serviceActor: Readonly<{ placeId: string; npcId: string }> | null;
  /** Supplied from the frozen work actor or explicitly saved published NPC art. */
  renderAsset?: Readonly<Pick<CompanionPlaceAssetPart, "key" | "sourceYaw"> & {
    npcId: NonNullable<CompanionPlaceAssetPart["npcId"]>;
  }> | null;
}>;
export type CompanionResidentPlaceBinding = Readonly<{
  placeId: string;
  residentInstanceId: string;
  gameInstanceId: string;
  layoutId: string;
}>;
export type CompanionResidentWorldView = Readonly<{
  worldKey: string;
  clock: CompanionWorldClock;
  /** One due event for the whole population, not one timer per resident. */
  nextAdvanceAtRealMs: number | null;
  residents: readonly CompanionResidentView[];
  placeBindings?: readonly CompanionResidentPlaceBinding[];
}>;

const nonemptyString = (value: unknown): value is string => typeof value === "string" && value.length > 0 && value.trim() === value;
const finiteTime = (value: number) => Number.isFinite(value) && value >= 0 && value <= Number.MAX_SAFE_INTEGER;
const nonnegativeInteger = (value: number) => Number.isSafeInteger(value) && value >= 0;
const validYaw = (value: number) => Number.isFinite(value) && Math.abs(value) <= Math.PI;
const validPoint = (value: unknown): value is CompanionPlacePose["point"] => Array.isArray(value) && value.length === 3
  && [value[0], value[1], value[2]].every(Number.isFinite);
const validPose = (value: Readonly<CompanionPlacePose>) => !!value && validPoint(value.point) && validYaw(value.yaw);
const validPlace = (value: Pick<CompanionResidentAnchor, "placeId" | "instanceId">) =>
  !!value && nonemptyString(value.placeId) && nonemptyString(value.instanceId);
const validAnchor = (value: CompanionResidentAnchor) => validPlace(value) && validPose(value.pose);
const samePlace = (a: Pick<CompanionResidentAnchor, "placeId" | "instanceId">, b: Pick<CompanionResidentAnchor, "placeId" | "instanceId">) =>
  a.placeId === b.placeId && a.instanceId === b.instanceId;
const boundPlace = (place: Pick<CompanionResidentAnchor, "placeId" | "instanceId">, binding: CompanionResidentPlaceBinding) =>
  place.placeId === binding.placeId && place.instanceId === binding.residentInstanceId;
const routineAction = (value: string) => value === "rest" || value === "work" || value === "socialize";
const boundedKey = (value: unknown): value is string =>
  typeof value === "string" && /^[A-Za-z0-9][A-Za-z0-9._:-]{0,159}$/.test(value);
const residentInWorld = (residentId: unknown, worldKey: string) => boundedKey(residentId)
  && companionResidentKeys.some(actorKey => residentId === `npc:${worldKey.length}:${worldKey}:${actorKey}`);

function validJourneyTransport(journey: CompanionResidentJourney, identity: CompanionResidentIdentity) {
  if (journey.mode === "walk") return journey.vehicleId === null && journey.transport === undefined;
  if (journey.mode !== "car" && journey.mode !== "okada" && journey.mode !== "keke" && journey.mode !== "danfo") return false;
  const transport = journey.transport;
  if (!boundedKey(journey.vehicleId) || !transport || typeof transport !== "object" || Array.isArray(transport)
    || transport.vehicleId !== journey.vehicleId || !boundedKey(identity.worldKey) || identity.worldKey.length > 80
    || !residentInWorld(transport.ownerResidentId, identity.worldKey)
    || transport.riderResidentId !== null && !residentInWorld(transport.riderResidentId, identity.worldKey)
    || transport.ownerResidentId !== identity.id && transport.riderResidentId !== identity.id) return false;
  const model = transport.model;
  if (!model || typeof model !== "object" || Array.isArray(model) || model.kind !== journey.mode
    || !boundedKey(model.catalogueId) || !boundedKey(model.modelId)
    || !nonnegativeInteger(model.catalogueRevision) || model.catalogueRevision < 1
    || !nonnegativeInteger(model.tankCapacityUnits) || model.tankCapacityUnits < 1
    || model.tankCapacityUnits > companionVehicleLimits.fuelUnits
    || !nonnegativeInteger(model.metresPerFuelUnit) || model.metresPerFuelUnit < 1
    || model.metresPerFuelUnit > companionVehicleLimits.metresPerFuelUnit) return false;
  const asset = transport.asset;
  return asset === null || !!asset && typeof asset === "object" && !Array.isArray(asset)
    && nonemptyString(asset.key) && asset.key.length <= 160 && validYaw(asset.sourceYaw);
}

/** Compare full frozen JSON without a layout-size quota. Reject unsafe structures. */
function canonicalFrozenJson(value: unknown): string | null {
  const active = new Set<object>();
  const token = (text: string) => text;
  const join = (entries: string[], open: string, close: string) => token(open)
    + entries.map((entry, index) => (index ? token(",") : "") + entry).join("") + token(close);
  const encode = (item: unknown, depth: number): string => {
    if (depth > 32) throw new Error("Frozen layout exceeds its structural depth limit.");
    if (item === null || typeof item === "boolean" || typeof item === "string"
      || typeof item === "number" && Number.isFinite(item)) return token(JSON.stringify(item));
    if (typeof item !== "object" || active.has(item)) throw new Error("Frozen layout must contain finite JSON data.");
    const array = Array.isArray(item), prototype = Object.getPrototypeOf(item);
    if (!array && prototype !== Object.prototype && prototype !== null) throw new Error("Frozen layout must use plain records.");
    active.add(item);
    let result: string;
    if (array) {
      const entries: string[] = [];
      for (let index = 0; index < item.length; index++) entries.push(encode(item[index], depth + 1));
      result = join(entries, "[", "]");
    } else {
      const record = item as Record<string, unknown>, keys = Object.keys(record).sort();
      const entries: string[] = [];
      for (const key of keys) {
        // Optional fields with undefined values are absent in both frozen JSON documents.
        if (record[key] !== undefined) entries.push(token(JSON.stringify(key)) + token(":") + encode(record[key], depth + 1));
      }
      result = join(entries, "{", "}");
    }
    active.delete(item);
    return result;
  };
  try { return encode(value, 0); } catch { return null; }
}

function publicGeneratedInstance(instance: CompanionWorldInstance) {
  const layout = instance?.layout, navigation = instance?.navigation;
  if (!instance || !layout || instance.room !== null
    || ![instance.id, instance.placeId, instance.seed, instance.generatorVersion, instance.layoutId].every(nonemptyString)
    || instance.placeId === "own-home" || instance.placeId.startsWith("resident-home:")
    || layout.kind === "housing" || !companionPlaceKinds.includes(layout.kind)
    || layout.placeId !== instance.placeId || layout.seed !== instance.seed
    || layout.generatorVersion !== instance.generatorVersion || layout.id !== instance.layoutId
    || !navigation || !nonemptyString(navigation.revision) || !Number.isFinite(navigation.floorY)
    || !Number.isFinite(navigation.radius) || navigation.radius <= 0 || !navigation.bounds || !Array.isArray(navigation.obstacles)
    || navigation.gridSize !== undefined && (!Number.isFinite(navigation.gridSize) || navigation.gridSize <= 0)
    || !Array.isArray(instance.spawns) || !instance.spawns.length || !instance.spawns.every(validPoint) || !validPoint(instance.exit)
    || ![layout.fixtures, layout.assets, layout.entrances, layout.spawns, layout.recoverySpawns, layout.detentionSpawns,
      layout.services, layout.npcs, layout.zones, layout.homeRooms, layout.qualityGaps].every(Array.isArray)
    || !layout.spawns.every(validPose)) return false;
  const bounds = [navigation.bounds, ...navigation.obstacles];
  if (!bounds.every(box => !!box && [box.minX, box.maxX, box.minZ, box.maxZ].every(Number.isFinite)
    && box.minX <= box.maxX && box.minZ <= box.maxZ)) return false;
  const navigationJson = canonicalFrozenJson(navigation), spawnsJson = canonicalFrozenJson(instance.spawns);
  return navigationJson !== null && navigationJson === canonicalFrozenJson(layout.navigation)
    && spawnsJson !== null && spawnsJson === canonicalFrozenJson(layout.spawns.map(spawn => spawn.point))
    && layout.entrances.some(entrance => entrance?.role === "world" && validPoint(entrance.point)
      && entrance.point.every((coordinate, index) => coordinate === instance.exit[index]));
}

/** Bind two already-frozen public layouts. Only the enclosing ID and start time may differ. */
export function resolveCompanionResidentPlaceBinding(
  residentInstance: CompanionWorldInstance,
  gameInstance: CompanionWorldInstance,
): CompanionResidentPlaceBinding | null {
  if (!publicGeneratedInstance(residentInstance) || !publicGeneratedInstance(gameInstance)) return null;
  const { id: residentInstanceId, sharedStartedAt: residentStart, ...residentLayout } = residentInstance;
  const { id: gameInstanceId, sharedStartedAt: gameStart, ...gameLayout } = gameInstance;
  // Start times do not determine geometry, but malformed timestamps are not admitted.
  if (!finiteTime(residentStart) || !finiteTime(gameStart)) return null;
  const residentJson = canonicalFrozenJson(residentLayout), gameJson = canonicalFrozenJson(gameLayout);
  if (residentJson === null || gameJson === null || residentJson !== gameJson) return null;
  return { placeId: residentInstance.placeId, residentInstanceId, gameInstanceId, layoutId: residentInstance.layoutId };
}

function validMovement(movement: CompanionMovement) {
  if (!movement || movement.version !== 1 || !nonnegativeInteger(movement.sequence) || movement.sequence < 1 || movement.sequence > 1_000_000_000
    || !nonnegativeInteger(movement.roomRevision) || !finiteTime(movement.startedAt) || !movement.from
    || ![movement.from.x, movement.from.z].every(Number.isFinite) || !validYaw(movement.from.yaw) || !validYaw(movement.yaw)
    || movement.initialSpeed !== undefined && (!Number.isFinite(movement.initialSpeed) || movement.initialSpeed < 0)
    || !Array.isArray(movement.path) || movement.path.length > CALL_MOVEMENT_MAX_POINTS) return false;
  for (const point of movement.path) if (!point || ![point.x, point.z].every(Number.isFinite)) return false;
  const endsAt = companionMovementEndsAt(movement);
  return finiteTime(endsAt) && endsAt >= movement.startedAt && endsAt - movement.startedAt <= 90_000;
}
const startsAtPose = (movement: CompanionMovement, pose: Readonly<CompanionPlacePose>) =>
  movement.from.x === pose.point[0] && movement.from.z === pose.point[2] && movement.from.yaw === pose.yaw;
function endsAtPose(movement: CompanionMovement, pose: Readonly<CompanionPlacePose>) {
  const last = movement.path[movement.path.length - 1] ?? movement.from;
  return last.x === pose.point[0] && last.z === pose.point[2] && movement.yaw === pose.yaw;
}

function samplePlaceMovement(movement: CompanionMovement, floorY: number, serverNow: number) {
  if (serverNow < movement.startedAt) return null;
  const sample = sampleCompanionMovement(movement, serverNow);
  const pose: CompanionPlacePose = { point: [sample.position.x, floorY, sample.position.z], yaw: sample.position.yaw };
  if (!validPose(pose) || !Number.isFinite(sample.speed)) return null;
  return { pose, moving: sample.moving && sample.speed > 0 && serverNow < companionMovementEndsAt(movement), working: false };
}

/** The caller admits the resident's world/epoch; bindings carry neither.
 * This projection never settles membership or work, including after an arrival is due.
 * Moving means translation, not an in-place turn or an expired movement.
 */
export function sampleCompanionResidentInPlace(
  resident: CompanionResidentView,
  binding: CompanionResidentPlaceBinding,
  serverNow: number,
): { pose: CompanionPlacePose; moving: boolean; working: boolean } | null {
  if (!resident || !binding || !finiteTime(serverNow)
    || ![binding.placeId, binding.residentInstanceId, binding.gameInstanceId, binding.layoutId].every(nonemptyString)
    || !resident.identity || resident.identity.actorKind !== "npc" || resident.identity.rosterVersion !== 1
    || ![resident.identity.id, resident.identity.worldKey, resident.identity.actorKey, resident.clockEpochKey].every(nonemptyString)
    || resident.identity.id !== `npc:${resident.identity.worldKey.length}:${resident.identity.worldKey}:${resident.identity.actorKey}`
    || !nonnegativeInteger(resident.revision) || !nonnegativeInteger(resident.updatedAtGameMinute)
    || !resident.health || !Number.isFinite(resident.health.health) || !Number.isFinite(resident.health.maxHealth)
    || resident.health.health <= 0 || resident.health.health > resident.health.maxHealth) return null;
  const activity = resident.activity;
  if (!activity || !nonnegativeInteger(activity.sinceGameMinute) || activity.sinceGameMinute > resident.updatedAtGameMinute) return null;
  if (activity.kind === "at-place") {
    if (resident.journey !== null || !validAnchor(activity.location) || !boundPlace(activity.location, binding)
      || !["rest", "work", "eat", "socialize", "recover"].includes(activity.action)
      || activity.interactionId !== null && !nonemptyString(activity.interactionId)) return null;
    if (resident.localMove === null) return {
      pose: { point: [...activity.location.pose.point], yaw: activity.location.pose.yaw }, moving: false, working: activity.action === "work",
    };
    const move = resident.localMove;
    if (!move || activity.interactionId !== null || !validAnchor(move.to) || !samePlace(activity.location, move.to) || !routineAction(move.action)
      || move.to.pose.point[1] !== activity.location.pose.point[1] || !validMovement(move.movement)
      || !startsAtPose(move.movement, activity.location.pose) || !endsAtPose(move.movement, move.to.pose)) return null;
    return samplePlaceMovement(move.movement, activity.location.pose.point[1], serverNow);
  }
  const journey = resident.journey;
  if (activity.kind !== "traveling" || resident.localMove !== null || !journey || !nonemptyString(journey.id) || activity.journeyId !== journey.id
    || !validPlace(activity.from) || !validPlace(activity.to) || !validAnchor(journey.from) || !validAnchor(journey.to)
    || !samePlace(activity.from, journey.from) || !samePlace(activity.to, journey.to) || journey.from.placeId === journey.to.placeId
    || journey.from.instanceId === journey.to.instanceId || journey.mode !== activity.mode || journey.vehicleId !== activity.vehicleId
    || !validJourneyTransport(journey, resident.identity) || journey.timeBasis !== "server-ms" || !routineAction(journey.arrivalAction)
    || !validMovement(journey.departure) || !validMovement(journey.arrival)
    || journey.departure.sequence !== journey.arrival.sequence || journey.departure.roomRevision !== journey.arrival.roomRevision
    || !startsAtPose(journey.departure, journey.from.pose) || !endsAtPose(journey.arrival, journey.to.pose)
    || ![journey.routeStartedAt, journey.routeEndsAt, journey.arrivesAt].every(finiteTime)
    || companionMovementEndsAt(journey.departure) > journey.routeStartedAt || journey.routeEndsAt <= journey.routeStartedAt
    || journey.arrival.startedAt !== journey.routeEndsAt || companionMovementEndsAt(journey.arrival) > journey.arrivesAt
    || serverNow < journey.departure.startedAt) return null;
  const route = journey.route;
  if (!route || !nonemptyString(route.id) || route.version !== "lagos-corridors-v1" || route.placement !== "proposed"
    || !Number.isFinite(route.lengthMetres) || route.lengthMetres <= 0 || !Array.isArray(route.points)
    || route.points.length < 2 || route.points.length > 256) return null;
  for (const point of route.points) if (!point || !Number.isFinite(point.latitude) || Math.abs(point.latitude) > 90
    || !Number.isFinite(point.longitude) || Math.abs(point.longitude) > 180) return null;
  if (serverNow < journey.routeStartedAt) return boundPlace(journey.from, binding)
    ? samplePlaceMovement(journey.departure, journey.from.pose.point[1], serverNow) : null;
  if (serverNow < journey.routeEndsAt) return null;
  return boundPlace(journey.to, binding) ? samplePlaceMovement(journey.arrival, journey.to.pose.point[1], serverNow) : null;
}

/** A due arrival is display information, not a receipt for money or occupancy. */
export function sampleCompanionResidentJourney(journey: CompanionResidentJourney, serverNow: number) {
  if (!Number.isFinite(serverNow)) throw new Error("Provide the shared server time.");
  if (serverNow < journey.routeStartedAt) return {
    phase: "leaving" as const, mode: journey.mode, vehicleId: journey.vehicleId, location: journey.from,
    position: sampleCompanionMovement(journey.departure, serverNow).position, mapPoint: null, arrivalDue: false,
  };
  if (serverNow < journey.routeEndsAt) {
    const progress = (serverNow - journey.routeStartedAt) / Math.max(1, journey.routeEndsAt - journey.routeStartedAt);
    return { phase: "on-map" as const, mode: journey.mode, vehicleId: journey.vehicleId, location: null, position: null,
      mapPoint: sampleCompanionRoute(journey.route, progress), arrivalDue: false };
  }
  return { phase: "entering" as const, mode: journey.mode, vehicleId: journey.vehicleId, location: journey.to,
    position: sampleCompanionMovement(journey.arrival, serverNow).position, mapPoint: null,
    arrivalDue: serverNow >= journey.arrivesAt };
}

export type CompanionResidentVehicleMapMarker = Readonly<{
  /** Map controllers deduplicate on worldKey + vehicleId, never passenger or model ID. */
  worldKey: string; clockEpochKey: string; vehicleId: string; journeyId: string;
  ownerResidentId: string; riderResidentId: string | null; mode: Exclude<CompanionResidentTravelMode, "walk">;
  modelId: string; asset: Readonly<{ key: string; sourceYaw: number }> | null;
  mapPoint: Readonly<{ latitude: number; longitude: number }>; routeStartedAt: number; routeEndsAt: number;
}>;

/** Bounded owner projections only. No routing call, membership settlement or invented art. */
export function sampleCompanionResidentVehicleMapMarkers(world: CompanionResidentWorldView | null, serverNow: number): readonly CompanionResidentVehicleMapMarker[] {
  if (!world || !finiteTime(serverNow) || world.residents.length > companionPersistentResidentKeys.length || world.clock.worldKey !== world.worldKey) return [];
  let gameMinute: number;
  try { gameMinute = sampleCompanionWorldClock(world.clock, serverNow).gameMinute; } catch { return []; }
  const markers: CompanionResidentVehicleMapMarker[] = [], vehicles = new Set<string>(), residents = new Set<string>();
  for (const resident of world.residents) {
    if (residents.has(resident.identity.id)) return [];
    residents.add(resident.identity.id);
    const trip = resident.journey, transport = trip?.transport;
    if (!trip || !transport || trip.mode === "walk" || transport.ownerResidentId !== resident.identity.id
      || resident.identity.actorKind !== "npc" || resident.identity.rosterVersion !== 1 || !nonnegativeInteger(resident.revision)
      || resident.identity.worldKey !== world.worldKey || resident.clockEpochKey !== world.clock.epochKey
      || !nonnegativeInteger(resident.updatedAtGameMinute) || resident.updatedAtGameMinute > gameMinute
      || !resident.health || !Number.isFinite(resident.health.health) || !Number.isFinite(resident.health.maxHealth)
      || resident.health.health <= 0 || resident.health.health > resident.health.maxHealth || resident.localMove !== null
      || !nonemptyString(trip.id) || !validAnchor(trip.from) || !validAnchor(trip.to)
      || resident.activity.kind !== "traveling" || resident.activity.journeyId !== trip.id
      || !nonnegativeInteger(resident.activity.sinceGameMinute) || resident.activity.sinceGameMinute > resident.updatedAtGameMinute
      || resident.activity.vehicleId !== trip.vehicleId || resident.activity.mode !== trip.mode
      || !samePlace(resident.activity.from, trip.from) || !samePlace(resident.activity.to, trip.to)
      || !validJourneyTransport(trip, resident.identity) || trip.timeBasis !== "server-ms"
      || ![trip.routeStartedAt, trip.routeEndsAt, trip.arrivesAt].every(finiteTime)
      || trip.routeEndsAt <= trip.routeStartedAt || trip.arrivesAt < trip.routeEndsAt
      || serverNow < trip.routeStartedAt || serverNow >= trip.routeEndsAt
      || !trip.route || !nonemptyString(trip.route.id) || trip.route.version !== "lagos-corridors-v1" || trip.route.placement !== "proposed"
      || !Number.isFinite(trip.route.lengthMetres) || trip.route.lengthMetres <= 0 || trip.route.lengthMetres > 20_000
      || !Array.isArray(trip.route.points) || trip.route.points.length < 2 || trip.route.points.length > 128
      || trip.route.points.some(point => !point || !Number.isFinite(point.latitude) || Math.abs(point.latitude) > 90
        || !Number.isFinite(point.longitude) || Math.abs(point.longitude) > 180)) continue;
    if (vehicles.has(transport.vehicleId)) return [];
    vehicles.add(transport.vehicleId);
    const mapPoint = sampleCompanionRoute(trip.route, (serverNow - trip.routeStartedAt) / (trip.routeEndsAt - trip.routeStartedAt));
    markers.push({ worldKey: world.worldKey, clockEpochKey: world.clock.epochKey, vehicleId: transport.vehicleId, journeyId: trip.id,
      ownerResidentId: transport.ownerResidentId, riderResidentId: transport.riderResidentId, mode: trip.mode, modelId: transport.model.modelId,
      asset: transport.asset, mapPoint, routeStartedAt: trip.routeStartedAt, routeEndsAt: trip.routeEndsAt });
  }
  return markers;
}
