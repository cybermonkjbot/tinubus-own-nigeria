import { planRoomRoute, routeSegmentClear, type NavigationObstacle, type RoomNavigation, type ScenePoint } from "./companionNavigation";
import { companionCityHasRoadTreatments, companionCityPlan, companionCityRoadTreatments } from "./companionCity";
import { sameCompanionStreetCrashScope, validCompanionStreetVehiclePose, type CompanionStreetCrashScope, type CompanionStreetVehiclePose } from "./companionStreetCrashes";
import { companionRoadHornExposure, type CompanionRoadHornPulse } from "./companionRoadMechanics";

export const companionStreetPedestrianRules = Object.freeze({ actors: 24, routes: 8, points: 32, gapMs: 4000,
  sprintSpeed: 4.5, walkSpeed: 1.4, hornSpeed: 4, maxRouteMs: 30_000, acceptedAgeMs: 250, vehicleLimit: 128 });
export type CompanionStreetPedestrianActor = Readonly<{ id: string; npcId: string; revision: number; point: ScenePoint;
  yaw: number; radius: number; acceptedAt: number; available: boolean;
  motionOwner: "routine" | "hawker" | "crossing" | "horn" | "gunfire" | "service" | "crowd" | "combat" }>;
/** The movement owner supplies a complete, accepted linear window. Turns require another window. */
export type CompanionPedestrianVehicleEnvelope = Readonly<{ pose: CompanionStreetVehiclePose; velocityX: number; velocityZ: number;
  startsAt: number; endsAt: number }>;
export type CompanionPedestrianRefuge = Readonly<{ id: string; kind: "curb" | "median"; point: ScenePoint;
  footprint: NavigationObstacle; island?: NavigationObstacle }>;
export type CompanionMedianCrossing = Readonly<{ id: string; roadId: string; curb: CompanionPedestrianRefuge;
  median: CompanionPedestrianRefuge; navigation: RoomNavigation }>;
export type CompanionPedestrianRoute = Readonly<{ id: string; scope: CompanionStreetCrashScope; actorId: string; npcId: string;
  actorRevision: number; kind: "horn" | "median" | "approach" | "hawker" | "return"; from: ScenePoint; points: readonly ScenePoint[];
  radius: number; speed: number; startsAt: number; arrivesAt: number; reservedUntil: number; refuge: CompanionPedestrianRefuge;
  returnRefuge: CompanionPedestrianRefuge; departureRefuge: CompanionPedestrianRefuge | null;
  settledPose: Readonly<{ point: ScenePoint; yaw: number; at: number }> | null;
  sourceId: string; status: "active" | "arrived" | "interrupted"; revision: number }>;
export type CompanionPedestrianRouteSample = Readonly<{ routeId: string; actorId: string; npcId: string; point: ScenePoint;
  instanceId: string; layoutRevision: string; medianId: string | null; returnPoint: ScenePoint;
  yaw: number; moving: boolean; radius: number; at: number; kind: CompanionPedestrianRoute["kind"];
  routeStartedAt?: number; routeRevision?: number; released?: true }>;
export type CompanionPedestrianReservation = Readonly<{ id: string; actorId: string; startsAt: number; expiresAt: number;
  from: ScenePoint; to: ScenePoint; radius: number }>;
const pointValid = (p: ScenePoint) => Array.isArray(p) && p.length === 3 && p.every(Number.isFinite);
const timeValid = (n: number) => Number.isSafeInteger(n) && n >= 0;
const idValid = (s: string) => typeof s === "string" && s.length > 0 && s.length <= 240 && s.trim() === s;
const copy = (p: ScenePoint): ScenePoint => Object.freeze([p[0], p[1], p[2]]);
const distance = (a: ScenePoint, b: ScenePoint) => Math.hypot(a[0] - b[0], a[2] - b[2]);
const floorPoint = (p: ScenePoint, nav: RoomNavigation): ScenePoint => [p[0], nav.floorY, p[2]];
const routeClear = (nav: RoomNavigation, a: ScenePoint, b: ScenePoint) => routeSegmentClear(nav, floorPoint(a, nav), floorPoint(b, nav));
const validBox = (b: NavigationObstacle) => !!b && [b.minX, b.maxX, b.minZ, b.maxZ, b.yaw ?? 0].every(Number.isFinite)
  && b.minX < b.maxX && b.minZ < b.maxZ && !b.polygon;
function inside(p: ScenePoint, box: NavigationObstacle, radius = 0) {
  const x = (box.minX + box.maxX) / 2, z = (box.minZ + box.maxZ) / 2, c = Math.cos(box.yaw ?? 0), s = Math.sin(box.yaw ?? 0);
  const localX = (p[0] - x) * c - (p[2] - z) * s, localZ = (p[0] - x) * s + (p[2] - z) * c;
  return Math.abs(localX) + radius < (box.maxX - box.minX) / 2 && Math.abs(localZ) + radius < (box.maxZ - box.minZ) / 2;
}
function sameBox(a: NavigationObstacle, b: NavigationObstacle) {
  return a.minX === b.minX && a.maxX === b.maxX && a.minZ === b.minZ && a.maxZ === b.maxZ && (a.yaw ?? 0) === (b.yaw ?? 0);
}
const freezeRefuge = (r: CompanionPedestrianRefuge): CompanionPedestrianRefuge => Object.freeze({ ...r, point: copy(r.point),
  footprint: Object.freeze({ ...r.footprint }), ...(r.island ? { island: Object.freeze({ ...r.island }) } : {}) });
const validRefuge = (r: CompanionPedestrianRefuge) => !!r && idValid(r.id) && ["curb", "median"].includes(r.kind)
  && pointValid(r.point) && validBox(r.footprint) && (r.kind === "curb" || !!r.island && validBox(r.island));
export function validCompanionPedestrianRoute(route: CompanionPedestrianRoute): boolean {
  return !!route && [route.id, route.sourceId, route.actorId, route.npcId].every(idValid)
    && !!route.scope && [route.scope.worldKey, route.scope.epochKey, route.scope.gameRoomId, route.scope.instanceId, route.scope.layoutRevision].every(idValid)
    && timeValid(route.scope.sharedStartedAt) && route.scope.sharedStartedAt <= route.startsAt
    && pointValid(route.from) && Array.isArray(route.points) && route.points.length > 0 && route.points.length <= 32 && route.points.every(pointValid)
    && [route.actorRevision, route.revision, route.startsAt, route.arrivesAt, route.reservedUntil].every(timeValid)
    && route.arrivesAt >= route.startsAt && route.reservedUntil >= route.arrivesAt && route.reservedUntil - route.startsAt <= 30_000
    && Number.isFinite(route.radius) && route.radius >= .2 && route.radius <= .8
    && Number.isFinite(route.speed) && route.speed > 0 && route.speed <= 4.5
    && route.speed === (route.kind === "median" ? companionStreetPedestrianRules.sprintSpeed
      : route.kind === "horn" ? companionStreetPedestrianRules.hornSpeed : companionStreetPedestrianRules.walkSpeed)
    && ["horn", "median", "approach", "hawker", "return"].includes(route.kind) && ["active", "arrived", "interrupted"].includes(route.status)
    && route.arrivesAt === route.startsAt + route.points.reduce((ms, to, i) => ms
      + Math.ceil(distance(i ? route.points[i - 1] : route.from, to) / route.speed * 1000), 0)
    && route.reservedUntil === (route.kind === "median" ? route.startsAt + companionStreetPedestrianRules.gapMs : route.arrivesAt)
    && validRefuge(route.refuge) && distance(route.points[route.points.length - 1], route.refuge.point) <= .001
    && route.points[route.points.length - 1][1] === route.refuge.point[1]
    && validRefuge(route.returnRefuge) && (route.departureRefuge === null || validRefuge(route.departureRefuge))
    && (route.status === "active" ? route.settledPose === null : !!route.settledPose && pointValid(route.settledPose.point)
      && Number.isFinite(route.settledPose.yaw) && timeValid(route.settledPose.at) && route.settledPose.at >= route.startsAt
      && (route.status !== "arrived" || route.settledPose.at >= route.arrivesAt
        && distance(route.settledPose.point, route.refuge.point) <= .1 && Math.abs(route.settledPose.point[1] - route.refuge.point[1]) <= .01));
}
/** A settled owner can retain its current clear point without inventing an authored destination. */
export function companionPedestrianPointRefuge(id: string, point: ScenePoint, radius: number): CompanionPedestrianRefuge {
  if (!idValid(id) || !pointValid(point) || !Number.isFinite(radius) || radius < .2 || radius > .8)
    throw new Error("Use the accepted pedestrian point and radius.");
  const half = Math.max(.75, radius + .25);
  return Object.freeze({ id, kind: "curb", point: copy(point), footprint: Object.freeze({
    minX: point[0] - half, maxX: point[0] + half, minZ: point[2] - half, maxZ: point[2] + half }) });
}
function segmentDistance(a: ScenePoint, b: ScenePoint, c: ScenePoint, d: ScenePoint) {
  const cross = (p: ScenePoint, q: ScenePoint, r: ScenePoint) => (q[0] - p[0]) * (r[2] - p[2]) - (q[2] - p[2]) * (r[0] - p[0]);
  if (cross(a, b, c) * cross(a, b, d) < 0 && cross(c, d, a) * cross(c, d, b) < 0) return 0;
  const toSegment = (p: ScenePoint, u: ScenePoint, v: ScenePoint) => {
    const x = v[0] - u[0], z = v[2] - u[2], squared = x * x + z * z;
    const t = squared ? Math.max(0, Math.min(1, ((p[0] - u[0]) * x + (p[2] - u[2]) * z) / squared)) : 0;
    return Math.hypot(p[0] - u[0] - t * x, p[2] - u[2] - t * z);
  };
  return Math.min(toSegment(a, c, d), toSegment(b, c, d), toSegment(c, a, b), toSegment(d, a, b));
}
function routesClear(from: ScenePoint, points: readonly ScenePoint[], radius: number, startsAt: number, endsAt: number,
  actorId: string, routes: readonly CompanionPedestrianRoute[]) {
  return routes.every(route => {
    if (route.actorId === actorId) return true;
    const endpoint = route.settledPose?.point ?? route.refuge.point;
    if (route.startsAt <= endsAt && points.some((to, i) => segmentDistance(i ? points[i - 1] : from, to, endpoint, endpoint) <= radius + route.radius + .1)) return false;
    if (route.status === "interrupted" || route.reservedUntil <= startsAt || route.startsAt >= endsAt) return true;
    return points.every((to, i) => route.points.every((other, j) =>
      segmentDistance(i ? points[i - 1] : from, to, j ? route.points[j - 1] : route.from, other) > radius + route.radius + .1));
  });
}
/** Conservative continuous moving-disc versus moving oriented full chassis. No radius-only car approximation. */
function vehicleClears(from: ScenePoint, to: ScenePoint, radius: number, startsAt: number, endsAt: number, envelope: CompanionPedestrianVehicleEnvelope) {
  const p = envelope.pose, c = Math.cos(p.yaw), s = Math.sin(p.yaw), seconds = (endsAt - startsAt) / 1000;
  const offset = (startsAt - envelope.startsAt) / 1000;
  const x = from[0] - p.point[0] - envelope.velocityX * offset, z = from[2] - p.point[2] - envelope.velocityZ * offset;
  const dx = to[0] - from[0] - envelope.velocityX * seconds, dz = to[2] - from[2] - envelope.velocityZ * seconds;
  let enter = 0, leave = 1;
  for (const [origin, delta, half] of [[x * c - z * s, dx * c - dz * s, p.halfWidth + radius],
    [x * s + z * c, dx * s + dz * c, p.halfLength + radius]]) {
    if (Math.abs(delta) < 1e-9) { if (Math.abs(origin) > half) return true; continue; }
    const first = (-half - origin) / delta, second = (half - origin) / delta;
    enter = Math.max(enter, Math.min(first, second)); leave = Math.min(leave, Math.max(first, second));
    if (enter > leave) return true;
  }
  return false;
}
export function companionPedestrianVehicleEnvelopesValid(vehicles: readonly CompanionPedestrianVehicleEnvelope[], startsAt: number, endsAt: number) {
  const ids = new Set<string>();
  return timeValid(startsAt) && timeValid(endsAt) && endsAt >= startsAt && Array.isArray(vehicles)
    && vehicles.length <= companionStreetPedestrianRules.vehicleLimit && vehicles.every(v => {
    if (!v || !validCompanionStreetVehiclePose(v.pose) || ids.has(v.pose.id) || ![v.velocityX, v.velocityZ].every(Number.isFinite)
      || Math.hypot(v.velocityX, v.velocityZ) > 55 || !timeValid(v.startsAt) || !timeValid(v.endsAt)
      || v.startsAt > startsAt || startsAt - v.startsAt > companionStreetPedestrianRules.acceptedAgeMs
      || v.endsAt < endsAt || v.endsAt - v.startsAt > 30_000) return false;
    ids.add(v.pose.id); return true;
  });
}
/** Refuges derive from retained road islands. The narrow route may enter only that island. */
export function companionCityMedianCrossings(navigation: RoomNavigation): readonly CompanionMedianCrossing[] {
  if (!companionCityHasRoadTreatments(navigation)) return Object.freeze([]);
  const treatments = companionCityRoadTreatments(), results: CompanionMedianCrossing[] = [];
  for (const island of treatments.islands) {
    if (results.length >= 16) break;
    const road = companionCityPlan.roads.find(r => r.id === island.roadId);
    if (!road || !navigation.obstacles.some(o => sameBox(o, island.footprint))) continue;
    const s = Math.sin(island.yaw), c = Math.cos(island.yaw);
    // Use the same rounded trunk centres as the existing instanced street kit.
    const physicalPalms = treatments.props.filter(prop => prop.id.startsWith(`${island.id}:palm:`)).map(prop => ({
      minX: prop.x - .48, maxX: prop.x + .48, minZ: prop.z - .48, maxZ: prop.z + .48 }));
    if (physicalPalms.length !== 2) continue;
    const nav = { ...navigation, obstacles: [...navigation.obstacles.filter(o => !sameBox(o, island.footprint)), ...physicalPalms], radius: .35 };
    const medianPoint: ScenePoint = [island.x, navigation.floorY + .134, island.z];
    const median = Object.freeze({ id: island.id, kind: "median" as const, point: copy(medianPoint), island: Object.freeze({ ...island.footprint }),
      footprint: Object.freeze({ minX: island.x - 1.2, maxX: island.x + 1.2, minZ: island.z - .75, maxZ: island.z + .75, yaw: island.yaw }) });
    for (const side of [-1, 1]) {
      if (results.length >= 16) break;
      const offset = side * (road.width / 2 + .75), curbPoint: ScenePoint = [island.x + s * offset, navigation.floorY, island.z + c * offset];
      const curb = Object.freeze({ id: `${island.id}:curb:${side}`, kind: "curb" as const, point: copy(curbPoint),
        footprint: Object.freeze({ minX: curbPoint[0] - 1, maxX: curbPoint[0] + 1, minZ: curbPoint[2] - .65, maxZ: curbPoint[2] + .65, yaw: island.yaw }) });
      if (distance(curbPoint, medianPoint) / companionStreetPedestrianRules.sprintSpeed > 4
        || !routeSegmentClear({ ...navigation, radius: .35 }, curbPoint, curbPoint)
        || !routeClear(nav, curbPoint, medianPoint)) continue;
      results.push(Object.freeze({ id: `${island.id}:dash:${side}`, roadId: road.id, curb, median, navigation: nav }));
    }
  }
  return Object.freeze(results);
}
export function companionPedestrianRouteNavigation(navigation: RoomNavigation, refuge: CompanionPedestrianRefuge): RoomNavigation | null {
  if (!refuge || !idValid(refuge.id) || !["curb", "median"].includes(refuge.kind) || !pointValid(refuge.point) || !validBox(refuge.footprint)) return null;
  if (refuge.kind !== "median") return navigation;
  const crossing = companionCityMedianCrossings(navigation).find(c => c.median.id === refuge.id && sameBox(c.median.footprint, refuge.footprint)
    && !!refuge.island && sameBox(c.median.island!, refuge.island) && distance(c.median.point, refuge.point) < .001 && c.median.point[1] === refuge.point[1]);
  return crossing?.navigation ?? null;
}
export function admitCompanionPedestrianRoute(args: { id: string; sourceId: string; scope: CompanionStreetCrashScope;
  navigation: RoomNavigation; actor: CompanionStreetPedestrianActor; kind: CompanionPedestrianRoute["kind"];
  refuge: CompanionPedestrianRefuge; vehicles: readonly CompanionPedestrianVehicleEnvelope[]; routes: readonly CompanionPedestrianRoute[];
  now: number; corridor?: NavigationObstacle; departureRefuge?: CompanionPedestrianRefuge; returnRefuge?: CompanionPedestrianRefuge }): CompanionPedestrianRoute | null {
  const { actor, navigation, refuge, now } = args;
  if (!idValid(args.id) || !idValid(args.sourceId) || !timeValid(now) || !args.scope
    || ![args.scope.worldKey, args.scope.epochKey, args.scope.gameRoomId, args.scope.instanceId, args.scope.layoutRevision].every(idValid)
    || !timeValid(args.scope.sharedStartedAt) || args.scope.sharedStartedAt > now || args.scope.layoutRevision !== navigation.revision
    || !actor || !idValid(actor.id) || !idValid(actor.npcId) || !pointValid(actor.point) || !Number.isFinite(actor.yaw)
    || !Number.isSafeInteger(actor.revision) || actor.revision < 0 || !timeValid(actor.acceptedAt) || actor.acceptedAt > now
    || now - actor.acceptedAt > companionStreetPedestrianRules.acceptedAgeMs || !actor.available
    || !["routine", "hawker", "crossing", "horn"].includes(actor.motionOwner) || !Number.isFinite(actor.radius) || actor.radius < .2 || actor.radius > .8
    || !["horn", "median", "approach", "hawker", "return"].includes(args.kind)
    || !validRefuge(refuge) || args.departureRefuge && !validRefuge(args.departureRefuge)
    || args.returnRefuge && !validRefuge(args.returnRefuge)
    || !Array.isArray(args.routes) || args.routes.length >= companionStreetPedestrianRules.routes || args.routes.some(r => !validCompanionPedestrianRoute(r) || !sameCompanionStreetCrashScope(r.scope, args.scope)
      || r.id === args.id || r.actorId === actor.id || r.npcId === actor.npcId)) return null;
  const targetNav = companionPedestrianRouteNavigation(navigation, refuge);
  const departure = args.departureRefuge;
  const nav = departure?.kind === "median" ? companionPedestrianRouteNavigation(navigation, departure) : targetNav;
  const returnRefuge = args.returnRefuge ?? departure ?? companionPedestrianPointRefuge(`${args.id.slice(0, 230)}:origin`, actor.point, actor.radius);
  if (args.kind !== "median" && (refuge.kind === "median" || departure?.kind === "median")) return null;
  if (!targetNav || !nav || !companionPedestrianRouteNavigation(navigation, returnRefuge)
    || refuge.kind === "median" && departure?.kind === "median" && refuge.id !== departure.id
    || !inside(refuge.point, refuge.footprint, actor.radius) || !inside(returnRefuge.point, returnRefuge.footprint, actor.radius)
    || returnRefuge.kind === "curb" && !routeClear({ ...navigation, radius: actor.radius }, returnRefuge.point, returnRefuge.point)
    || departure && (!inside(actor.point, departure.kind === "median" ? departure.island! : departure.footprint,
      departure.kind === "median" ? 0 : actor.radius) || Math.abs(actor.point[1] - departure.point[1]) > .01)
    || Math.abs(actor.point[1] - (departure?.kind === "median" ? departure.point[1] : navigation.floorY)) > .01
    || refuge.kind === "curb" && Math.abs(refuge.point[1] - navigation.floorY) > .01) return null;
  let points: readonly ScenePoint[] | null;
  if (args.kind === "median") {
    const crossing = companionCityMedianCrossings(navigation).find(c => departure?.kind === "median"
      ? c.median.id === departure.id && c.curb.id === refuge.id && inside(actor.point, c.median.island!)
      : c.median.id === refuge.id && distance(c.curb.point, actor.point) <= .45 && inside(actor.point, c.curb.footprint, actor.radius));
    if (!crossing || !routeClear({ ...nav, radius: actor.radius }, actor.point, refuge.point)) return null;
    points = [refuge.point];
  } else points = planRoomRoute({ ...nav, radius: actor.radius }, floorPoint(actor.point, nav), floorPoint(refuge.point, nav));
  if (!points || !points.length || points.length > companionStreetPedestrianRules.points) return null;
  points = [...points.slice(0, -1), refuge.point];
  const speed = args.kind === "median" ? companionStreetPedestrianRules.sprintSpeed : args.kind === "horn" ? companionStreetPedestrianRules.hornSpeed : companionStreetPedestrianRules.walkSpeed;
  const arrivesAt = now + points.reduce((ms, to, i) => ms + Math.ceil(distance(i ? points![i - 1] : actor.point, to) / speed * 1000), 0);
  const reservedUntil = Math.max(arrivesAt, now + (args.kind === "median" ? companionStreetPedestrianRules.gapMs : 0));
  if (reservedUntil - now > companionStreetPedestrianRules.maxRouteMs || args.kind === "median" && arrivesAt > now + 4000
    || !companionPedestrianVehicleEnvelopesValid(args.vehicles, now, reservedUntil)
    || !routesClear(actor.point, points, actor.radius, now, reservedUntil, actor.id, args.routes)) return null;
  let from = actor.point, at = now;
  for (const to of points) {
    const until = at + Math.ceil(distance(from, to) / speed * 1000);
    if (!routeClear({ ...nav, radius: actor.radius }, from, to) || args.corridor && (!validBox(args.corridor)
      || !inside(from, args.corridor, actor.radius) || !inside(to, args.corridor, actor.radius))
      || args.vehicles.some(v => !vehicleClears(from, to, actor.radius, at, until, v))) return null;
    from = to; at = until;
  }
  // Reserve every point of the crossing against each full four-second chassis sweep.
  if (args.kind === "median" && args.vehicles.some(v => {
    const c = Math.cos(v.pose.yaw), s = Math.sin(v.pose.yaw), offset = (now - v.startsAt) / 1000 + 2;
    const swept = { ...v, startsAt: now, velocityX: 0, velocityZ: 0, pose: { ...v.pose,
      point: [v.pose.point[0] + v.velocityX * offset, v.pose.point[1], v.pose.point[2] + v.velocityZ * offset] as ScenePoint,
      halfWidth: v.pose.halfWidth + Math.abs(v.velocityX * c - v.velocityZ * s) * 2,
      halfLength: v.pose.halfLength + Math.abs(v.velocityX * s + v.velocityZ * c) * 2 } };
    return !vehicleClears(actor.point, refuge.point, actor.radius, now, now, swept);
  })) return null;
  if (args.vehicles.some(v => !vehicleClears(refuge.point, refuge.point, actor.radius, arrivesAt, reservedUntil, v))) return null;
  return Object.freeze({ id: args.id, sourceId: args.sourceId, scope: Object.freeze({ ...args.scope }), actorId: actor.id, npcId: actor.npcId,
    actorRevision: actor.revision, kind: args.kind, from: copy(actor.point), points: Object.freeze(points.map(copy)), radius: actor.radius,
    speed, startsAt: now, arrivesAt, reservedUntil, refuge: freezeRefuge(refuge), returnRefuge: freezeRefuge(returnRefuge),
    departureRefuge: departure ? freezeRefuge(departure) : null, settledPose: null, status: "active", revision: 0 });
}
export function sampleCompanionPedestrianRoute(route: CompanionPedestrianRoute, now: number): CompanionPedestrianRouteSample | null {
  if (!validCompanionPedestrianRoute(route) || !timeValid(now) || now < route.startsAt || route.settledPose && now < route.settledPose.at) return null;
  let from = route.from, segmentAt = route.startsAt, yaw = 0;
  for (const to of route.points) {
    const metres = distance(from, to); yaw = Math.atan2(to[0] - from[0], to[2] - from[2]);
    const until = segmentAt + Math.ceil(metres / route.speed * 1000);
    if (now < until && metres > .001) { const t = Math.max(0, (now - segmentAt) / (until - segmentAt));
      from = [from[0] + (to[0] - from[0]) * t, from[1], from[2] + (to[2] - from[2]) * t]; break; }
    from = to; segmentAt = until;
  }
  const median = route.refuge.kind === "median" ? route.refuge : route.departureRefuge?.kind === "median" ? route.departureRefuge : null;
  if (median?.island) from = [from[0], inside(from, median.island) ? median.point[1] : route.refuge.kind === "median" ? route.from[1] : route.refuge.point[1], from[2]];
  if (route.settledPose) { from = route.settledPose.point; yaw = route.settledPose.yaw; }
  return Object.freeze({ routeId: route.id, actorId: route.actorId, npcId: route.npcId,
    instanceId: route.scope.instanceId, layoutRevision: route.scope.layoutRevision, medianId: median?.id ?? null,
    returnPoint: copy(route.returnRefuge.point), point: copy(from), yaw,
    moving: route.status === "active" && now < route.arrivesAt, radius: route.radius, at: now, kind: route.kind,
    routeStartedAt: route.startsAt, routeRevision: route.revision });
}
/** The existing owner acknowledges arrival or interruption at its actual accepted point. */
export function settleCompanionPedestrianRoute(route: CompanionPedestrianRoute, actor: CompanionStreetPedestrianActor,
  kind: "arrived" | "interrupted", now: number): CompanionPedestrianRoute | null {
  if (!validCompanionPedestrianRoute(route) || !actor || route.status !== "active" || !timeValid(now) || now < route.startsAt || actor.id !== route.actorId || actor.npcId !== route.npcId
    || !Number.isSafeInteger(actor.revision) || actor.revision < route.actorRevision || actor.acceptedAt !== now || !pointValid(actor.point)
    || !Number.isFinite(actor.yaw) || !["arrived", "interrupted"].includes(kind)
    || kind === "arrived" && (now < route.arrivesAt || distance(actor.point, route.refuge.point) > .1 || Math.abs(actor.point[1] - route.refuge.point[1]) > .01)) return null;
  return Object.freeze({ ...route, status: kind, settledPose: Object.freeze({ point: copy(actor.point), yaw: actor.yaw, at: now }), revision: route.revision + 1 });
}
/** Approach the actual curb with the existing actor, then request its dash separately with fresh traffic. */
export function admitCompanionMedianApproach(args: Omit<Parameters<typeof admitCompanionPedestrianRoute>[0], "kind" | "refuge">): CompanionPedestrianRoute | null {
  const crossings = [...companionCityMedianCrossings(args.navigation)].sort((a, b) => distance(a.curb.point, args.actor.point) - distance(b.curb.point, args.actor.point));
  for (const crossing of crossings) {
    const route = admitCompanionPedestrianRoute({ ...args, kind: "approach", refuge: crossing.curb });
    if (route) return route;
  }
  return null;
}
export function admitCompanionMedianDash(args: Omit<Parameters<typeof admitCompanionPedestrianRoute>[0], "kind" | "refuge"> & {
  previous: CompanionPedestrianRoute }): CompanionPedestrianRoute | null {
  const { previous, actor } = args;
  if (!validCompanionPedestrianRoute(previous) || !actor || previous.kind !== "approach" || previous.status !== "arrived" || !previous.settledPose
    || previous.actorId !== actor.id || previous.npcId !== actor.npcId || actor.revision < previous.actorRevision
    || !sameCompanionStreetCrashScope(previous.scope, args.scope) || distance(previous.settledPose.point, actor.point) > .1
    || Math.abs(previous.settledPose.point[1] - actor.point[1]) > .01) return null;
  const crossing = companionCityMedianCrossings(args.navigation).find(c => c.curb.id === previous.refuge.id);
  return crossing ? admitCompanionPedestrianRoute({ ...args, kind: "median", refuge: crossing.median, returnRefuge: previous.returnRefuge,
    routes: args.routes.filter(route => route.id !== previous.id) }) : null;
}
/** Replace the actor's terminal route only after its next safe route has been accepted. */
export function admitCompanionPedestrianReturn(args: Omit<Parameters<typeof admitCompanionPedestrianRoute>[0], "kind" | "refuge"> & {
  previous: CompanionPedestrianRoute }): CompanionPedestrianRoute | null {
  const { previous, actor } = args;
  if (!validCompanionPedestrianRoute(previous) || !actor || previous.status === "active" || !previous.settledPose || previous.actorId !== actor.id
    || previous.npcId !== actor.npcId || actor.revision < previous.actorRevision || !sameCompanionStreetCrashScope(previous.scope, args.scope)
    || distance(previous.settledPose.point, actor.point) > .1 || Math.abs(previous.settledPose.point[1] - actor.point[1]) > .01) return null;
  const median = previous.refuge.kind === "median" ? previous.refuge : previous.departureRefuge;
  const departureRefuge = median?.kind === "median" && inside(actor.point, median.island!) ? median : undefined;
  if (departureRefuge) {
    const curbs = [...companionCityMedianCrossings(args.navigation)].filter(c => c.median.id === departureRefuge.id)
      .sort((a, b) => distance(a.curb.point, previous.returnRefuge.point) - distance(b.curb.point, previous.returnRefuge.point));
    for (const crossing of curbs) {
      const route = admitCompanionPedestrianRoute({ ...args, kind: "median", refuge: crossing.curb, departureRefuge,
        returnRefuge: previous.returnRefuge, routes: args.routes.filter(route => route.id !== previous.id) });
      if (route) return route;
    }
    return null;
  }
  return admitCompanionPedestrianRoute({ ...args, kind: "return", refuge: previous.returnRefuge, returnRefuge: previous.returnRefuge,
    departureRefuge, routes: args.routes.filter(route => route.id !== previous.id) });
}
export function companionPedestrianRouteCanRelease(route: CompanionPedestrianRoute, actor: CompanionStreetPedestrianActor, now: number): boolean {
  return validCompanionPedestrianRoute(route) && !!actor && timeValid(now) && route.status !== "active" && !!route.settledPose && actor.id === route.actorId && actor.npcId === route.npcId
    && actor.acceptedAt === now && actor.revision >= route.actorRevision && distance(actor.point, route.returnRefuge.point) <= .1
    && Math.abs(actor.point[1] - route.returnRefuge.point[1]) <= .01;
}
export type CompanionPedestrianContinuation = Readonly<{ action: "retain" | "replace" | "release";
  previousId: string; route: CompanionPedestrianRoute | null }>;
/** Advance only from the canonical owner's accepted body point. Failed admission retains the terminal body. */
export function continueCompanionPedestrianRoute(args: Omit<Parameters<typeof admitCompanionPedestrianRoute>[0],
  "kind" | "refuge" | "returnRefuge" | "departureRefuge" | "sourceId"> & {
    previous: CompanionPedestrianRoute; intent: "cross" | "hold" | "return";
  }): CompanionPedestrianContinuation | null {
  const { previous, actor, now } = args;
  if (!validCompanionPedestrianRoute(previous) || !actor || !timeValid(now) || now < previous.startsAt
    || previous.settledPose && now < previous.settledPose.at || actor.acceptedAt !== now
    || actor.id !== previous.actorId || actor.npcId !== previous.npcId || !pointValid(actor.point) || !Number.isFinite(actor.yaw)
    || !timeValid(actor.revision) || actor.revision < previous.actorRevision || actor.radius !== previous.radius
    || previous.revision >= Number.MAX_SAFE_INTEGER
    || !sameCompanionStreetCrashScope(previous.scope, args.scope) || args.navigation.revision !== args.scope.layoutRevision
    || !["cross", "hold", "return"].includes(args.intent) || !Array.isArray(args.routes) || args.routes.length > 8
    || args.routes.filter(route => route?.id === previous.id).length !== 1
    || new Set(args.routes.map(route => route?.id)).size !== args.routes.length
    || new Set(args.routes.map(route => route?.actorId)).size !== args.routes.length
    || new Set(args.routes.map(route => route?.npcId)).size !== args.routes.length
    || args.routes.some(route => !validCompanionPedestrianRoute(route) || !sameCompanionStreetCrashScope(route.scope, args.scope)
      || route.id === previous.id && (route.revision !== previous.revision || route.actorId !== previous.actorId || route.npcId !== previous.npcId)
      || route.id !== previous.id && (route.actorId === actor.id || route.npcId === actor.npcId))) return null;
  let route = previous;
  const retain = (): CompanionPedestrianContinuation => Object.freeze({ action: "retain", previousId: previous.id, route });
  const held = ["gunfire", "service", "crowd", "combat"].includes(actor.motionOwner);
  if (held) {
    if (route.status === "active") route = settleCompanionPedestrianRoute(route, actor, "interrupted", now) ?? route;
    else if (route.status !== "interrupted" || distance(route.settledPose!.point, actor.point) > .001
      || Math.abs(route.settledPose!.point[1] - actor.point[1]) > .001 || route.settledPose!.yaw !== actor.yaw)
      route = Object.freeze({ ...route, status: "interrupted", revision: route.revision + 1,
        settledPose: Object.freeze({ point: copy(actor.point), yaw: actor.yaw, at: now }) });
    return retain();
  }
  if (route.status === "active") {
    const settled = !actor.available ? settleCompanionPedestrianRoute(route, actor, "interrupted", now)
      : now >= route.arrivesAt ? settleCompanionPedestrianRoute(route, actor, "arrived", now) : null;
    if (settled) route = settled;
    if (route.status === "active") return retain();
  }
  if (!actor.available || !["routine", "hawker", "crossing", "horn"].includes(actor.motionOwner) || args.intent === "hold") return retain();
  if (distance(route.settledPose!.point, actor.point) > .1 || Math.abs(route.settledPose!.point[1] - actor.point[1]) > .01) return retain();
  if (args.intent === "return" && companionPedestrianRouteCanRelease(route, actor, now))
    return Object.freeze({ action: "release", previousId: previous.id, route: null });
  const nextArgs = { ...args, sourceId: previous.sourceId, previous: route };
  const next = args.intent === "cross" ? route.kind === "approach" && route.status === "arrived"
    ? admitCompanionMedianDash(nextArgs) : null : admitCompanionPedestrianReturn(nextArgs);
  return next ? Object.freeze({ action: "replace", previousId: previous.id, route: next }) : retain();
}
export function companionPedestrianReservations(routes: readonly CompanionPedestrianRoute[], scope: CompanionStreetCrashScope,
  now: number): readonly CompanionPedestrianReservation[] {
  if (!timeValid(now) || routes.length > companionStreetPedestrianRules.routes || !routes.every(validCompanionPedestrianRoute)) return Object.freeze([]);
  return Object.freeze(routes.filter(r => sameCompanionStreetCrashScope(r.scope, scope) && r.status !== "interrupted"
    && r.startsAt <= now && now < r.reservedUntil).flatMap(r => r.points.map((to, i) => Object.freeze({ id: `${r.id}:${i}`,
      actorId: r.actorId, startsAt: r.startsAt, expiresAt: r.reservedUntil, from: copy(i ? r.points[i - 1] : r.from), to: copy(to), radius: r.radius }))));
}

/** One accepted pulse routes reachable NPCs only. Human controls and existing gunfire leases stay with their owners. */
export function admitCompanionPedestrianHornRoutes(args: { pulse: CompanionRoadHornPulse; scope: CompanionStreetCrashScope;
  navigation: RoomNavigation; actors: readonly CompanionStreetPedestrianActor[]; refuges: readonly CompanionPedestrianRefuge[];
  routes: readonly CompanionPedestrianRoute[]; vehicles: readonly CompanionPedestrianVehicleEnvelope[]; corridor: NavigationObstacle;
  boundaryGain: (from: ScenePoint, to: ScenePoint) => number; now: number }): readonly CompanionPedestrianRoute[] {
  if (!args.pulse?.scope || !args.scope || args.pulse.scope.instanceId !== args.scope.instanceId || args.pulse.scope.layoutRevision !== args.scope.layoutRevision
    || args.scope.layoutRevision !== args.navigation.revision || args.pulse.at < args.scope.sharedStartedAt
    || !Array.isArray(args.actors) || !Array.isArray(args.refuges) || !Array.isArray(args.routes)
    || args.actors.length > companionStreetPedestrianRules.actors || args.refuges.length > 32 || args.routes.length > 8
    || !validBox(args.corridor) || args.actors.some(a => !a) || args.refuges.some(r => !r || !idValid(r.id))
    || new Set(args.refuges.map(r => r.id)).size !== args.refuges.length
    || new Set(args.actors.map(a => a.id)).size !== args.actors.length) return Object.freeze([]);
  const routes: CompanionPedestrianRoute[] = [];
  for (const actor of args.actors) {
    if (routes.length + args.routes.length >= 8) break;
    if (!actor.available || actor.motionOwner !== "routine" && actor.motionOwner !== "hawker"
      || companionRoadHornExposure(args.pulse, actor.point, args.now, args.boundaryGain) <= 0) continue;
    const refuges = args.refuges.filter(r => r.kind === "curb" && pointValid(r.point) && distance(r.point, actor.point) <= 8)
      .sort((a, b) => distance(a.point, actor.point) - distance(b.point, actor.point) || a.id.localeCompare(b.id));
    for (const refuge of refuges) {
      const route = admitCompanionPedestrianRoute({ ...args, id: `${args.pulse.id}:${actor.npcId}`, sourceId: args.pulse.id,
        actor, kind: "horn", refuge, routes: [...args.routes, ...routes] });
      if (route) { routes.push(route); break; }
    }
  }
  return Object.freeze(routes);
}
