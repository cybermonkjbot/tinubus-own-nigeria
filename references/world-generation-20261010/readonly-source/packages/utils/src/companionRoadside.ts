import { companionGeoDistance, companionRouteLength, sampleCompanionJourney, type CompanionGeoPoint, type CompanionJourney } from "./companionJourney";
import type { CompanionWorldInstance } from "./companionWorld";
import { companionCityPlan, companionCityGeoPoint, isCompanionCity } from "./companionCity";
import { planRoomRoute, routeSegmentClear, type ScenePoint } from "./companionNavigation";

export type CompanionTravelEndpoint =
  | { kind: "place"; id: string; point: CompanionGeoPoint }
  | { kind: "road"; id: string; point: CompanionGeoPoint; vehicleId: string | null; vehicleRevision: number | null };

/** An admitted character location, independent of a vehicle's retained ownership. */
export type CompanionRoadsideVisit = {
  endpoint: Extract<CompanionTravelEndpoint, { kind: "road" }>;
  sourceJourneyId: string;
  stoppedAt: number;
  /** Retained road geometry provides a non-payment walking exit without another route request. */
  exit: { placeId: string; route: CompanionJourney["route"] };
};

export type CompanionRoadsideJourney = CompanionJourney & {
  fromEndpoint?: CompanionTravelEndpoint;
  toEndpoint?: CompanionTravelEndpoint;
  travelMode?: "walk" | "vehicle" | "boat";
};
export type CompanionRoadsideInstance = CompanionWorldInstance & { roadside: Extract<CompanionTravelEndpoint, { kind: "road" }> };

export const companionRoadsidePlaceId = (endpoint: CompanionTravelEndpoint) => endpoint.kind === "road" ? `roadside:${endpoint.id}` : endpoint.id;
export function assertCompanionTravelEndpoint(endpoint: CompanionTravelEndpoint) {
  if (!endpoint.id || endpoint.id.length > (endpoint.kind === "road" ? 85 : 128) || !Number.isFinite(endpoint.point.latitude) || Math.abs(endpoint.point.latitude) > 90
    || !Number.isFinite(endpoint.point.longitude) || Math.abs(endpoint.point.longitude) > 180
    || endpoint.kind === "road" && (endpoint.vehicleId !== null && (!endpoint.vehicleId || endpoint.vehicleId.length > 160)
      || endpoint.vehicleRevision !== null && (!Number.isSafeInteger(endpoint.vehicleRevision) || endpoint.vehicleRevision < 0 || endpoint.vehicleRevision >= Number.MAX_SAFE_INTEGER)
      || (endpoint.vehicleId === null) !== (endpoint.vehicleRevision === null)))
    throw new Error("Use a saved travel endpoint.");
}

/** A small roadside boarding area anchored at the exact retained geographic point. */
export function resolveCompanionRoadsideInstance(scope: string, endpoint: Extract<CompanionTravelEndpoint, { kind: "road" }>): CompanionRoadsideInstance {
  assertCompanionTravelEndpoint(endpoint);
  const placeId = companionRoadsidePlaceId(endpoint);
  return { id: `${scope}:${placeId}`, placeId, sharedStartedAt: 0, seed: endpoint.id, roadside: endpoint,
    generatorVersion: "roadside-v1", layoutId: "roadside-v1", room: null, layout: null,
    navigation: { revision: "roadside-v1", bounds: { minX: -8, maxX: 8, minZ: -8, maxZ: 8 },
      obstacles: [], radius: .3, floorY: 0 }, spawns: [[0, 0, 0]], exit: [0, 0, 0] };
}

export function companionRoadsideEndpoint(instance: CompanionWorldInstance) {
  if (instance.generatorVersion !== "roadside-v1" || !("roadside" in instance)) return null;
  const endpoint = instance.roadside as Extract<CompanionTravelEndpoint, { kind: "road" }>;
  assertCompanionTravelEndpoint(endpoint);
  if (instance.placeId !== companionRoadsidePlaceId(endpoint)) throw new Error("The roadside instance changed.");
  return endpoint;
}

/** No snapping. Only a full clear route in the current frozen metric city permits local recovery. */
export function companionRoadsideCityPosition(instance: CompanionWorldInstance, point: CompanionGeoPoint): ScenePoint | null {
  if (!isCompanionCity(instance.navigation)) return null;
  const origin = companionCityPlan.origin;
  const target: ScenePoint = [(point.longitude - origin.longitude) * 111_320 * Math.cos(origin.latitude * Math.PI / 180),
    instance.navigation.floorY, (origin.latitude - point.latitude) * 111_320];
  return routeSegmentClear(instance.navigation, target, target) ? target : null;
}
export function companionRoadsideCityWalk(instance: CompanionWorldInstance, position: { x: number; z: number },
  endpoint: Extract<CompanionTravelEndpoint, { kind: "road" }>) {
  const target = companionRoadsideCityPosition(instance, endpoint.point);
  if (!target) return null;
  const start: ScenePoint = [position.x, instance.navigation.floorY, position.z];
  const path = planRoomRoute(instance.navigation, start, target);
  if (!path || path.length > 32) return null;
  const points = [companionCityGeoPoint(position.x, position.z), ...path.map(([x, , z]) => companionCityGeoPoint(x, z))];
  points[points.length - 1] = { ...endpoint.point };
  return { points, lengthMetres: companionRouteLength(points) };
}

/** Slice retained geometry at the exact sampled point. Never replace it with the closest venue. */
export function companionRoadsideExit(journey: CompanionRoadsideJourney, now: number,
  exitPlaceId: string, forward: boolean): CompanionRoadsideVisit["exit"] {
  const sample = sampleCompanionJourney({ ...journey, status: "traveling" }, now), point = sample.point;
  const points = journey.route.points;
  if (!point || points.length < 2 || points.length > 4096) throw new Error("The retained road route needs review.");
  const weights = journey.route.version === "mapbox-directions-v5" ? journey.route.segmentDurationsMs
    : points.slice(1).map((value, index) => companionGeoDistance(points[index], value));
  if (weights.length !== points.length - 1 || weights.some(value => !Number.isFinite(value) || value < 0))
    throw new Error("The retained road timing needs review.");
  let remainingWeight = sample.progress * weights.reduce((sum, value) => sum + value, 0), segment = weights.length - 1;
  for (let index = 0; index < points.length - 1; index++) {
    if (sample.progress >= 1) break;
    if (remainingWeight <= weights[index]) { segment = index; break; }
    remainingWeight -= weights[index];
  }
  const remaining = forward ? [point, ...points.slice(segment + 1)] : [point, ...points.slice(0, segment + 1).reverse()];
  const distinct = remaining.filter((value, index) => index === 0 || companionGeoDistance(value, remaining[index - 1]) > 0);
  // Keep a valid zero-length endpoint segment for a stop at a venue's road anchor.
  if (distinct.length === 1) distinct.push({ ...distinct[0] });
  const lengthMetres = companionRouteLength(distinct);
  if (journey.route.version === "lagos-corridors-v1") return { placeId: exitPlaceId,
    route: { ...journey.route, id: `walk-exit:${journey.requestId}`, points: distinct, lengthMetres } };
  const route = { id: `walk-exit:${journey.requestId}`, version: "mapbox-directions-v5" as const,
    placement: "road-network" as const, points: distinct, lengthMetres,
    segmentDurationsMs: distinct.slice(1).map((value, index) => Math.max(1, Math.ceil(companionGeoDistance(distinct[index], value) / .65 * 1000))) };
  return { placeId: exitPlaceId, route };
}
