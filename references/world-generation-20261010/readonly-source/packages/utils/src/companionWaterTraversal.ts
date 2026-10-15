import { companionBoatAssetEntries, companionBoatBounds, companionBoatModelId } from "./companionBoatAssets";
import { planRoomRoute, routeSegmentClear, type NavigationObstacle, type ScenePoint } from "./companionNavigation";
import type { CompanionPlaceLayout } from "./companionPlaces";
import type { CompanionWaterfrontProp } from "./companionWaterfront";

/** Noncash handling defaults. Uses real server milliseconds, never the accelerated world clock. */
export const companionWaterTuning = Object.freeze({
  version: "cms-water-control-v1", maxForward: 6, maxReverse: 1.8, acceleration: 2.2,
  turnRate: .65, coastDrag: .34, lateralDrag: 1.35, brakeDrag: 3.2,
  hyacinthDrag: 2.4, snagAfterMs: 1800, clearSnagMs: 900, clearance: .12,
  stepMs: 25, maxStepMs: 250, commandIntervalMs: 160, inputHoldMs: 350,
  leaseMs: 5000, episodeMs: 20 * 60_000, dockSpeed: .45,
});
export type CompanionWaterScope = Readonly<{
  gameRoomId: string; instanceId: string; placeId: string; layoutId: string; layoutRevision: string;
  userId: string; characterId: string; lifeId: string; revision: number;
}>;
export type CompanionWaterInput = Readonly<{ throttle: number; steer: number; brake: boolean }>;
export type CompanionWaterPose = Readonly<{ point: ScenePoint; yaw: number }>;
export type CompanionWaterDomain = Readonly<{
  id: string; placeId: "cms-jetty"; layoutId: string; layoutRevision: string;
  craftId: "cms-jetty:ferry-a"; propId: "ferry-a"; model: "passenger-speedboat";
  assetKey: "bestieBoatPassengerSpeedboat"; assetSha256: string;
  bounds: NavigationObstacle; obstacles: readonly NavigationObstacle[];
  hyacinth: readonly (NavigationObstacle & { id: string })[];
  dock: CompanionWaterPose; landing: ScenePoint; driverSeat: ScenePoint;
}>;
export type CompanionWaterControl = Readonly<{
  id: string; scope: CompanionWaterScope; craftId: string; craftRevision: number; domainId: string;
  phase: "active" | "landed"; point: ScenePoint; yaw: number; velocity: readonly [number, number];
  simulatedAt: number; startedAt: number; expiresAt: number; endsAt: number;
  sequence: number; lastInputAt: number; input: CompanionWaterInput;
  snag: Readonly<{ zoneId: string; exposureMs: number; clearMs: number; held: boolean }> | null;
  feedback: "clear" | "hyacinth" | "snagged" | "blocked";
}>;
export type CompanionWaterCraft = Readonly<{
  id: string; revision: number; domainId: string; pose: CompanionWaterPose;
  occupant: Readonly<{ controlId: string; userId: string; characterId: string; lifeId: string }> | null;
  control: CompanionWaterControl | null;
}>;
/** Stored with the existing world instance. One record, one supported seat, no second session. */
export type CompanionWaterFleet = Readonly<{ domainId: string; craft: CompanionWaterCraft }>;
export type CompanionWaterAdmission = Readonly<{
  scope: CompanionWaterScope; point: ScenePoint; health: number; ready: boolean;
  occupancy: "walking" | "road" | "water" | "transit" | "journey";
  blocked: boolean;
}>;
export type CompanionWaterCommand = Readonly<{
  controlId: string; scope: CompanionWaterScope; craftRevision: number; sequence: number; input: CompanionWaterInput;
}>;
export type CompanionWaterLandingReceipt = Readonly<{
  controlId: string; scope: CompanionWaterScope; craftId: string; craftRevision: number;
  point: ScenePoint; yaw: number; endedAt: number;
  reason: "dock" | "stop" | "expired" | "reconnect" | "death" | "custody" | "presentation-failed";
}>;
const neutral: CompanionWaterInput = Object.freeze({ throttle: 0, steer: 0, brake: true });
const key = (value: string, max = 240) => typeof value === "string" && value.length > 0 && value.length <= max;
const integer = (value: number) => Number.isSafeInteger(value) && value >= 0 && value < Number.MAX_SAFE_INTEGER - 1;
const pointValid = (p: ScenePoint) => Array.isArray(p) && p.length === 3 && p.every(Number.isFinite);
const boxValid = (b: NavigationObstacle) => Number.isFinite(b.minX) && Number.isFinite(b.maxX)
  && Number.isFinite(b.minZ) && Number.isFinite(b.maxZ)
  && b.minX < b.maxX && b.minZ < b.maxZ && (b.yaw === undefined || Number.isFinite(b.yaw)) && !b.polygon;
export function validCompanionWaterScope(scope: CompanionWaterScope): boolean {
  return !!scope && [scope.gameRoomId, scope.instanceId, scope.placeId, scope.userId, scope.characterId, scope.lifeId].every(v => key(v))
    && key(scope.layoutId, 160_000) && key(scope.layoutRevision, 160_000) && integer(scope.revision);
}
export function sameCompanionWaterScope(a: CompanionWaterScope, b: CompanionWaterScope): boolean {
  return validCompanionWaterScope(a) && validCompanionWaterScope(b)
    && a.gameRoomId === b.gameRoomId && a.instanceId === b.instanceId && a.placeId === b.placeId
    && a.layoutId === b.layoutId && a.layoutRevision === b.layoutRevision && a.userId === b.userId
    && a.characterId === b.characterId && a.lifeId === b.lifeId && a.revision === b.revision;
}
export function validCompanionWaterInput(input: CompanionWaterInput): boolean {
  return !!input && Number.isFinite(input.throttle) && Math.abs(input.throttle) <= 1
    && Number.isFinite(input.steer) && Math.abs(input.steer) <= 1 && typeof input.brake === "boolean";
}
type Point2 = readonly [number, number];
type RectangleScratch = [number, number][];
function rectangle(box: NavigationObstacle, points: RectangleScratch = [[0, 0], [0, 0], [0, 0], [0, 0]]): RectangleScratch {
  const x = (box.minX + box.maxX) / 2, z = (box.minZ + box.maxZ) / 2;
  const c = Math.cos(box.yaw ?? 0), s = Math.sin(box.yaw ?? 0);
  const minX = box.minX - x, maxX = box.maxX - x, minZ = box.minZ - z, maxZ = box.maxZ - z;
  points[0][0] = x + minX * c + minZ * s; points[0][1] = z - minX * s + minZ * c;
  points[1][0] = x + maxX * c + minZ * s; points[1][1] = z - maxX * s + minZ * c;
  points[2][0] = x + maxX * c + maxZ * s; points[2][1] = z - maxX * s + maxZ * c;
  points[3][0] = x + minX * c + maxZ * s; points[3][1] = z - minX * s + maxZ * c;
  return points;
}
/** SAT against whole rectangles, including grazing contact. No point-only hull admission. */
function overlaps(a: readonly Point2[], b: readonly Point2[]): boolean {
  for (let shape = 0; shape < 2; shape++) {
    const polygon = shape === 0 ? a : b;
    for (let i = 0; i < polygon.length; i++) {
      const p = polygon[i], q = polygon[(i + 1) % polygon.length], x = q[1] - p[1], z = p[0] - q[0];
      let minA = Infinity, maxA = -Infinity, minB = Infinity, maxB = -Infinity;
      // Only the projection extrema are used. Retain Math.min/max semantics
      // for signed zero and nonfinite values without allocating projection arrays.
      for (const point of a) {
        const projection = point[0] * x + point[1] * z;
        minA = Math.min(minA, projection); maxA = Math.max(maxA, projection);
      }
      for (const point of b) {
        const projection = point[0] * x + point[1] * z;
        minB = Math.min(minB, projection); maxB = Math.max(maxB, projection);
      }
      if (maxA < minB - 1e-9 || maxB < minA - 1e-9) return false;
    }
  }
  return true;
}
/** Retains the scenery's PI art rotation. Heading zero faces negative scene Z. */
export function companionWaterHull(pose: CompanionWaterPose, padding: number = companionWaterTuning.clearance): readonly Point2[] {
  const b = companionBoatBounds["passenger-speedboat"], c = Math.cos(pose.yaw + Math.PI), s = Math.sin(pose.yaw + Math.PI);
  const x = pose.point[0], z = pose.point[2];
  const minX = b.min[0] - padding, maxX = b.max[0] + padding, minZ = b.min[2] - padding, maxZ = b.max[2] + padding;
  return [
    [x + minX * c + minZ * s, z - minX * s + minZ * c],
    [x + maxX * c + minZ * s, z - maxX * s + minZ * c],
    [x + maxX * c + maxZ * s, z - maxX * s + maxZ * c],
    [x + minX * c + maxZ * s, z - minX * s + maxZ * c],
  ];
}
function propBounds(prop: CompanionWaterfrontProp): NavigationObstacle {
  if (prop.kind === "boat") {
    const b = companionBoatBounds[companionBoatModelId(prop)];
    const c = Math.cos(prop.yaw + Math.PI), s = Math.sin(prop.yaw + Math.PI);
    const points = [b.min[0], b.max[0]].flatMap(x => [b.min[2], b.max[2]].map(z =>
      [prop.point[0] + x * c + z * s, prop.point[2] - x * s + z * c]));
    return { minX: Math.min(...points.map(p => p[0])), maxX: Math.max(...points.map(p => p[0])),
      minZ: Math.min(...points.map(p => p[1])), maxZ: Math.max(...points.map(p => p[1])) };
  }
  return { minX: prop.point[0] - prop.size[0] / 2, maxX: prop.point[0] + prop.size[0] / 2,
    minZ: prop.point[2] - prop.size[2] / 2, maxZ: prop.point[2] + prop.size[2] / 2, yaw: prop.yaw };
}
export function companionWaterPoseClear(domain: CompanionWaterDomain, pose: CompanionWaterPose, extraPadding = 0): boolean {
  if (!pose || !pointValid(pose.point) || !Number.isFinite(pose.yaw) || Math.abs(pose.point[1] + .18) > .001
    || !Number.isFinite(extraPadding) || extraPadding < 0 || !boxValid(domain.bounds)) return false;
  const hull = companionWaterHull(pose, companionWaterTuning.clearance + extraPadding), bounds = domain.bounds;
  // Each synchronous query owns its scratch. Returned hulls remain independent.
  let obstacleRectangle: RectangleScratch | undefined;
  return hull.every(([x, z]) => x >= bounds.minX && x <= bounds.maxX && z >= bounds.minZ && z <= bounds.maxZ)
    && !domain.obstacles.some(box => {
      if (!boxValid(box)) return true;
      obstacleRectangle = rectangle(box, obstacleRectangle);
      return overlaps(hull, obstacleRectangle);
    });
}
/** Derives only from an admitted stored CMS v2 recipe. Never regenerates or edits a saved layout. */
export function resolveCompanionWaterDomain(layout: CompanionPlaceLayout): CompanionWaterDomain | null {
  const scenery = layout.waterfront;
  if (layout.placeId !== "cms-jetty" || layout.kind !== "water-terminal" || scenery?.version !== "lagos-waterfront-v2"
    || scenery.zone !== "lagoon" || scenery.props.length > 96 || scenery.surfaces.length > 16
    || !key(layout.id, 160_000) || !key(layout.navigation.revision, 160_000)) return null;
  const craft = scenery.props.find(p => p.id === "ferry-a" && p.kind === "boat");
  if (!craft || companionBoatModelId(craft) !== "passenger-speedboat" || craft.point[0] !== -8 || craft.point[2] !== -23 || craft.yaw !== 0
    || !scenery.props.some(p => p.id === "boarding-jetty" && p.kind === "jetty" && p.point[0] === 0 && p.point[2] === -19)
    || !scenery.surfaces.some(s => s.id === "boarding-pier" && s.minX === -2.5 && s.maxX === 2.5 && s.minZ === -29 && s.maxZ === -10)) return null;
  if (!scenery.props.every(p => pointValid(p.point) && pointValid(p.size) && p.size.every(v => v > 0) && Number.isFinite(p.yaw))
    || !scenery.surfaces.every(boxValid)) return null;
  const solid = scenery.props.filter(p => p.id !== craft.id && !["hyacinth", "debris", "oil-sheen", "reeds", "sign", "laundry"].includes(p.kind));
  const obstacles = [...scenery.surfaces.map(s => ({ minX: s.minX, maxX: s.maxX, minZ: s.minZ, maxZ: s.maxZ })), ...solid.map(propBounds)];
  if (obstacles.length > 80 || !obstacles.every(boxValid)) return null;
  const domain: CompanionWaterDomain = {
    id: `${layout.navigation.revision}:${companionWaterTuning.version}`, placeId: "cms-jetty", layoutId: layout.id,
    layoutRevision: layout.navigation.revision, craftId: "cms-jetty:ferry-a", propId: "ferry-a", model: "passenger-speedboat",
    assetKey: "bestieBoatPassengerSpeedboat", assetSha256: companionBoatAssetEntries.bestieBoatPassengerSpeedboat.sha256,
    bounds: { minX: -40, maxX: -2.8, minZ: -100, maxZ: -12.2 }, obstacles,
    hyacinth: scenery.props.filter(p => p.kind === "hyacinth").map(p => ({ ...propBounds(p), id: p.id })),
    dock: { point: [-4.3, -.18, -23], yaw: 0 }, landing: [-2.15, 0, -21.53],
    // Export uses [x,z,-y]. The retained PI rotation maps the exact seat anchor to this root-local pose.
    driverSeat: [.62, .74, -1.6],
  };
  if (!companionWaterPoseClear(domain, domain.dock) || !routeSegmentClear(layout.navigation, domain.landing, domain.landing)
    || !layout.spawns.some(spawn => !!planRoomRoute(layout.navigation, spawn.point, domain.landing))) return null;
  return domain;
}
export function createCompanionWaterFleet(domain: CompanionWaterDomain): CompanionWaterFleet {
  return { domainId: domain.id, craft: { id: domain.craftId, domainId: domain.id, revision: 0, pose: domain.dock, occupant: null, control: null } };
}
function domainScope(domain: CompanionWaterDomain, scope: CompanionWaterScope): boolean {
  return validCompanionWaterScope(scope) && scope.placeId === domain.placeId && scope.layoutId === domain.layoutId && scope.layoutRevision === domain.layoutRevision;
}
/** The stored fleet and the renderer use the same complete reservation contract. Expiry is checked against server time by callers. */
export function validCompanionWaterFleet(domain: CompanionWaterDomain, fleet: CompanionWaterFleet): boolean {
  const craft = fleet?.craft;
  if (!craft || fleet.domainId !== domain.id || craft.domainId !== domain.id || craft.id !== domain.craftId || !integer(craft.revision)
    || !companionWaterPoseClear(domain, craft.pose)) return false;
  if (craft.control === null) return craft.occupant === null;
  const control = craft.control, occupant = craft.occupant;
  return validCompanionWaterControl(domain, control) && !!occupant && control.craftRevision === craft.revision
    && control.id === occupant.controlId && control.scope.userId === occupant.userId
    && control.scope.lifeId === occupant.lifeId && control.scope.characterId === occupant.characterId
    && control.point.every((value, index) => value === craft.pose.point[index]) && control.yaw === craft.pose.yaw;
}
function checkFleet(domain: CompanionWaterDomain, fleet: CompanionWaterFleet): void {
  if (!validCompanionWaterFleet(domain, fleet)) throw new Error("The craft reservation needs recovery at its landing.");
}
export function admitCompanionWaterBoarding(domain: CompanionWaterDomain, layout: CompanionPlaceLayout, fleet: CompanionWaterFleet,
  admission: CompanionWaterAdmission, controlId: string, expectedCraftRevision: number, now: number): { fleet: CompanionWaterFleet; control: CompanionWaterControl } {
  checkFleet(domain, fleet);
  if (!domainScope(domain, admission.scope) || layout.id !== domain.layoutId || layout.navigation.revision !== domain.layoutRevision
    || layout.placeId !== domain.placeId || !key(controlId) || !integer(now) || !integer(expectedCraftRevision)
    || !admission.ready || admission.blocked || !Number.isFinite(admission.health) || admission.health <= 0
    || !pointValid(admission.point)) throw new Error("Reach the CMS landing and release other transport before boarding.");
  if (now + companionWaterTuning.episodeMs >= Number.MAX_SAFE_INTEGER - 1) throw new Error("Invalid water-control time.");
  const existing = fleet.craft.control;
  if (existing?.id === controlId && sameCompanionWaterScope(existing.scope, admission.scope)) {
    if (admission.occupancy !== "water" || expectedCraftRevision > existing.craftRevision || now < existing.simulatedAt
      || now >= existing.expiresAt || now >= existing.endsAt) throw new Error("Return to the CMS landing before resuming water control.");
    return { fleet, control: existing };
  }
  if (expectedCraftRevision !== fleet.craft.revision || !integer(fleet.craft.revision + 1) || admission.occupancy !== "walking" || fleet.craft.occupant
    || Math.hypot(admission.point[0] - domain.landing[0], admission.point[2] - domain.landing[2]) > .85
    || !routeSegmentClear(layout.navigation, admission.point, domain.landing)) throw new Error("Reach the CMS landing and release other transport before boarding.");
  const control: CompanionWaterControl = { id: controlId, scope: { ...admission.scope }, craftId: domain.craftId, domainId: domain.id,
    craftRevision: fleet.craft.revision + 1, phase: "active", ...domain.dock, velocity: [0, 0], simulatedAt: now, startedAt: now,
    expiresAt: now + companionWaterTuning.leaseMs, endsAt: now + companionWaterTuning.episodeMs,
    sequence: 0, lastInputAt: now, input: neutral, snag: null, feedback: "clear" };
  return { control, fleet: { domainId: domain.id, craft: { ...fleet.craft, revision: control.craftRevision, pose: domain.dock,
    occupant: { controlId, userId: admission.scope.userId, characterId: admission.scope.characterId, lifeId: admission.scope.lifeId }, control } } };
}
export function validCompanionWaterControl(domain: CompanionWaterDomain, state: CompanionWaterControl): boolean {
  return !!state && state.phase === "active" && key(state.id) && state.domainId === domain.id && state.craftId === domain.craftId
    && domainScope(domain, state.scope) && integer(state.craftRevision) && integer(state.sequence)
    && [state.simulatedAt, state.startedAt, state.expiresAt, state.endsAt, state.lastInputAt].every(integer)
    && state.startedAt <= state.lastInputAt && state.lastInputAt <= state.simulatedAt
    && state.endsAt - state.startedAt === companionWaterTuning.episodeMs
    && state.expiresAt === Math.min(state.endsAt, state.lastInputAt + companionWaterTuning.leaseMs)
    && state.expiresAt <= state.endsAt && state.expiresAt > state.simulatedAt && validCompanionWaterInput(state.input)
    && Array.isArray(state.velocity) && state.velocity.length === 2 && state.velocity.every(Number.isFinite)
    && Math.hypot(...state.velocity) <= companionWaterTuning.maxForward + .001
    && companionWaterPoseClear(domain, state) && ["clear", "hyacinth", "snagged", "blocked"].includes(state.feedback)
    && (state.snag === null || !!state.snag && key(state.snag.zoneId) && domain.hyacinth.some(zone => zone.id === state.snag?.zoneId)
      && Number.isFinite(state.snag.exposureMs) && state.snag.exposureMs >= 0 && state.snag.exposureMs <= companionWaterTuning.snagAfterMs
      && Number.isFinite(state.snag.clearMs) && state.snag.clearMs >= 0 && state.snag.clearMs <= companionWaterTuning.clearSnagMs
      && typeof state.snag.held === "boolean");
}
/** Shared deterministic step for accepted server controls and local prediction. Never settles occupancy or rewards. */
export function stepCompanionWaterControl(domain: CompanionWaterDomain, current: CompanionWaterControl, input: CompanionWaterInput, now: number): CompanionWaterControl {
  if (!validCompanionWaterControl(domain, current) || !validCompanionWaterInput(input) || !integer(now) || now < current.simulatedAt
    || now >= current.expiresAt || now >= current.endsAt) throw new Error("Water control is stale.");
  const elapsed = Math.min(now - current.simulatedAt, companionWaterTuning.maxStepMs);
  let state = current;
  let zoneRectangle: RectangleScratch | undefined;
  for (let ms = 0; ms < elapsed; ms += companionWaterTuning.stepMs) {
    const stepMs = Math.min(companionWaterTuning.stepMs, elapsed - ms), dt = stepMs / 1000;
    const speed = Math.hypot(...state.velocity), hull = companionWaterHull(state);
    const zone = domain.hyacinth.find(p => {
      zoneRectangle = rectangle(p, zoneRectangle);
      return overlaps(hull, zoneRectangle);
    });
    let snag = state.snag;
    if (snag?.held) {
      const clearMs = input.throttle < -.25 || input.brake ? Math.min(companionWaterTuning.clearSnagMs, snag.clearMs + stepMs) : 0;
      snag = clearMs >= companionWaterTuning.clearSnagMs ? { ...snag, exposureMs: 0, clearMs, held: false } : { ...snag, clearMs };
    } else if (zone) {
      // Once cleared, the same patch cannot immediately snag again while reversing out of its full-hull overlap.
      if (snag?.zoneId !== zone.id || snag.clearMs < companionWaterTuning.clearSnagMs) {
        const exposureMs = Math.min(companionWaterTuning.snagAfterMs, (snag?.zoneId === zone.id ? snag.exposureMs : 0) + stepMs);
        snag = { zoneId: zone.id, exposureMs, clearMs: 0, held: exposureMs >= companionWaterTuning.snagAfterMs && speed < 1.2 };
      }
    } else snag = null;
    const held = !!snag?.held;
    const yawDelta = held ? 0 : -input.steer * companionWaterTuning.turnRate * dt * Math.min(1, .2 + speed / 2)
      * (input.throttle < 0 ? -1 : 1) * (zone ? .6 : 1);
    const yaw = Math.atan2(Math.sin(state.yaw + yawDelta), Math.cos(state.yaw + yawDelta));
    const fx = -Math.sin(yaw), fz = -Math.cos(yaw), forward = state.velocity[0] * fx + state.velocity[1] * fz;
    const lateral = state.velocity[0] * fz - state.velocity[1] * fx;
    const drag = companionWaterTuning.coastDrag + (input.brake ? companionWaterTuning.brakeDrag : 0) + (zone ? companionWaterTuning.hyacinthDrag : 0);
    const nextForward = held ? 0 : Math.max(-companionWaterTuning.maxReverse, Math.min(companionWaterTuning.maxForward,
      forward * Math.exp(-drag * dt) + input.throttle * companionWaterTuning.acceleration * dt));
    const nextLateral = held ? 0 : lateral * Math.exp(-(companionWaterTuning.lateralDrag + drag) * dt);
    let vx = fx * nextForward + fz * nextLateral, vz = fz * nextForward - fx * nextLateral;
    const magnitude = Math.hypot(vx, vz);
    if (magnitude > companionWaterTuning.maxForward) { vx *= companionWaterTuning.maxForward / magnitude; vz *= companionWaterTuning.maxForward / magnitude; }
    const point: ScenePoint = [state.point[0] + vx * dt, -.18, state.point[2] + vz * dt];
    // Every segment is enclosed by the previous full hull plus maximum translation and corner rotation.
    const sweepPadding = Math.hypot(vx, vz) * dt + Math.abs(yawDelta) * 6.2;
    const clear = companionWaterPoseClear(domain, state, sweepPadding) && companionWaterPoseClear(domain, { point, yaw });
    state = { ...state, ...(clear ? { point, yaw, velocity: [vx, vz] as const } : { velocity: [0, 0] as const }), snag,
      feedback: !clear ? "blocked" : held ? "snagged" : zone ? "hyacinth" : "clear" };
  }
  return { ...state, simulatedAt: now, input: { ...input } };
}
export function advanceCompanionWaterControl(domain: CompanionWaterDomain, fleet: CompanionWaterFleet,
  command: CompanionWaterCommand, admission: CompanionWaterAdmission, now: number): { fleet: CompanionWaterFleet; control: CompanionWaterControl } {
  checkFleet(domain, fleet);
  const current = fleet.craft.control;
  if (!current || !sameCompanionWaterScope(current.scope, command.scope)
    || !sameCompanionWaterScope(current.scope, admission.scope) || command.controlId !== current.id
    || !integer(command.sequence) || !integer(command.craftRevision) || !validCompanionWaterInput(command.input)
    || !integer(now) || now < current.simulatedAt || now >= current.expiresAt || now >= current.endsAt
    || admission.health <= 0 || !Number.isFinite(admission.health) || !admission.ready || admission.blocked || admission.occupancy !== "water")
    throw new Error("Return to the CMS landing before resuming water control.");
  // Duplicate sequences return the accepted state without extending a lease or applying another step.
  if (command.sequence <= current.sequence) return { fleet, control: current };
  if (command.sequence !== current.sequence + 1 || command.craftRevision !== current.craftRevision
    || !integer(current.craftRevision + 1)) throw new Error("Refresh the accepted craft state.");
  const stop = command.input.throttle === 0 && command.input.steer === 0 && command.input.brake;
  const wasStopped = current.input.throttle === 0 && current.input.steer === 0 && current.input.brake;
  if ((!stop || wasStopped) && current.sequence > 0 && now - current.lastInputAt < companionWaterTuning.commandIntervalMs) throw new Error("Water input arrived too soon.");
  // Previous held input covers the elapsed interval. New input starts now, never retroactively.
  const heldInput = now - current.lastInputAt <= companionWaterTuning.inputHoldMs ? current.input : neutral;
  const stepped = stepCompanionWaterControl(domain, current, heldInput, now);
  const control: CompanionWaterControl = { ...stepped, input: { ...command.input }, sequence: command.sequence,
    lastInputAt: now, expiresAt: Math.min(current.endsAt, now + companionWaterTuning.leaseMs), craftRevision: current.craftRevision + 1 };
  return { control, fleet: { domainId: domain.id, craft: { ...fleet.craft, revision: control.craftRevision, pose: { point: control.point, yaw: control.yaw }, control } } };
}
/** Dock is an actual approach. Forced cleanup returns to the captured reachable landing, never the current mid-water point. */
export function finishCompanionWaterControl(domain: CompanionWaterDomain, layout: CompanionPlaceLayout, fleet: CompanionWaterFleet,
  scope: CompanionWaterScope, controlId: string, reason: CompanionWaterLandingReceipt["reason"], now: number): {
    fleet: CompanionWaterFleet; receipt: CompanionWaterLandingReceipt;
  } {
  checkFleet(domain, fleet);
  const current = fleet.craft.control;
  if (!current || current.id !== controlId || !sameCompanionWaterScope(current.scope, scope) || !integer(now)
    || now < current.simulatedAt || !domainScope(domain, scope) || layout.id !== domain.layoutId
    || layout.placeId !== domain.placeId || layout.navigation.revision !== domain.layoutRevision || !integer(fleet.craft.revision + 1)
    || !["dock", "stop", "expired", "reconnect", "death", "custody", "presentation-failed"].includes(reason)
    || !routeSegmentClear(layout.navigation, domain.landing, domain.landing))
    throw new Error("Resolve the original craft and landing before releasing water occupancy.");
  if (reason === "dock" && (Math.hypot(current.point[0] - domain.dock.point[0], current.point[2] - domain.dock.point[2]) > .3
    || Math.abs(Math.atan2(Math.sin(current.yaw - domain.dock.yaw), Math.cos(current.yaw - domain.dock.yaw))) > .12
    || Math.hypot(...current.velocity) > companionWaterTuning.dockSpeed)) throw new Error("Slow down beside the CMS boarding pier to land.");
  if (reason === "expired" && now < current.expiresAt && now < current.endsAt) throw new Error("The craft control has not expired.");
  const revision = fleet.craft.revision + 1;
  return { fleet: { domainId: domain.id, craft: { ...fleet.craft, revision, pose: domain.dock, occupant: null, control: null } },
    receipt: { controlId, scope: { ...scope }, craftId: domain.craftId, craftRevision: revision, point: domain.landing,
      yaw: Math.PI, endedAt: now, reason } };
}
export function companionWaterSeatPose(domain: CompanionWaterDomain, pose: CompanionWaterPose): CompanionWaterPose {
  const [x, y, z] = domain.driverSeat, c = Math.cos(pose.yaw), s = Math.sin(pose.yaw);
  return { point: [pose.point[0] + x * c + z * s, pose.point[1] + y, pose.point[2] - x * s + z * c], yaw: pose.yaw };
}
