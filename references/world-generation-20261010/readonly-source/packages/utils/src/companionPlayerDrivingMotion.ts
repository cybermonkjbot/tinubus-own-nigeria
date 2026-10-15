import plan from "../../assets/city/lagos-city-v3.json";
import { isCompanionCity } from "./companionCity";
import { routeSegmentClear, type RoomNavigation, type ScenePoint } from "./companionNavigation";
import { assertCompanionPlayerVehicle, sameCompanionPlayerVehiclePlace, type CompanionPlayerVehicle } from "./companionPlayerVehicles";
import { castVehicleRoadPlane, copyVehicleDynamics, createVehicleDynamics, createVehicleDynamicsConfig,
  holdVehicleDynamics, stepVehicleDynamics, validVehicleDynamics, vehicleDynamicsPose } from "./companionVehicleDynamics";
import { companionPlayerDrivingRules as rules, neutralCompanionDrivingInput,
  type CompanionDrivingAuthority, type CompanionDrivingCommand, type CompanionDrivingHandling,
  type CompanionDrivingInput, type CompanionDrivingPose, type CompanionDrivingResult,
  type CompanionDrivingRoute, type CompanionPlayerDrivingState } from "./companionPlayerDriving";

const radius = Math.hypot(rules.halfWidth, rules.halfLength) + rules.clearance;
// Current keke wheel pivots, exported from author_vehicles.py into +Z-forward metres.
export const companionDrivingDynamicsConfig = createVehicleDynamicsConfig({ kind: "keke", wheelbase: 1.67, halfWidth: rules.halfWidth,
  wheelRadius: .245, wheelLayout: [{ x: 0, z: .94, front: true, radius: .245 },
    { x: -.57, z: -.73, front: false, radius: .245 }, { x: .57, z: -.73, front: false, radius: .245 }] });
const point = (pose: CompanionDrivingPose): ScenePoint => [pose.x, pose.y, pose.z];
const key = (value: string) => typeof value === "string" && value.length > 0 && value.length <= 240 && value.trim() === value;
const integer = (value: number) => Number.isSafeInteger(value) && value >= 0 && value < Number.MAX_SAFE_INTEGER - 1;
const poseValid = (pose: CompanionDrivingPose) => pose && [pose.x, pose.y, pose.z, pose.yaw].every(Number.isFinite);
const wrap = (yaw: number) => Math.atan2(Math.sin(yaw), Math.cos(yaw));
const distance = (a: CompanionDrivingPose, b: CompanionDrivingPose) => Math.hypot(a.x - b.x, a.z - b.z);
const baseHandling: CompanionDrivingHandling = Object.freeze({ accelerationScale: 1, brakingScale: 1,
  speedLimit: rules.maxSpeed, steeringBias: 0, surface: "clean_asphalt" });
export function validCompanionDrivingInput(input: CompanionDrivingInput): boolean {
  return !!input && [input.throttle, input.brake, input.steering].every(Number.isFinite)
    && input.throttle >= 0 && input.throttle <= 1 && input.brake >= 0 && input.brake <= 1
    && input.steering >= -1 && input.steering <= 1;
}
function routeDistance(route: CompanionDrivingRoute, p: CompanionDrivingPose) {
  let best = Infinity;
  for (let i = 1; i < route.points.length; i++) {
    const a = route.points[i - 1], b = route.points[i], x = b[0] - a[0], z = b[2] - a[2];
    const length = x * x + z * z;
    const t = length ? Math.max(0, Math.min(1, ((p.x - a[0]) * x + (p.z - a[2]) * z) / length)) : 0;
    best = Math.min(best, Math.hypot(p.x - a[0] - t * x, p.z - a[2] - t * z));
  }
  return best;
}
/** Enclosing disc covers the complete rotating chassis, not just its centre or wheels. */
export function companionDrivingSegmentClear(navigation: RoomNavigation, from: CompanionDrivingPose, to: CompanionDrivingPose) {
  return poseValid(from) && poseValid(to) && routeSegmentClear({ ...navigation, radius }, point(from), point(to));
}
/** Freeze once per exact world navigation. No provider, new geometry, asset or revision migration. */
export function createCompanionCourierDrivingRoute(navigation: RoomNavigation): CompanionDrivingRoute | null {
  if (!isCompanionCity(navigation) || !Number.isFinite(navigation.floorY)) return null;
  const road = plan.roads.find(value => value.id === "unity-avenue");
  if (!road) return null;
  const vertices = road.points.slice(18, 25);
  const poses = vertices.map((p, index) => {
    const a = vertices[Math.max(0, index - 1)], b = vertices[Math.min(vertices.length - 1, index + 1)];
    const yaw = Math.atan2(b[0] - a[0], b[1] - a[1]);
    return Object.freeze({ x: p[0] + Math.cos(yaw) * 3, y: navigation.floorY,
      z: p[1] - Math.sin(yaw) * 3, yaw });
  });
  const berthOrigins = [poses[1], poses[3], poses.at(-1)!];
  const pullOvers = berthOrigins.map(p => Object.freeze({ ...p,
    x: p.x + Math.cos(p.yaw) * 3, z: p.z - Math.sin(p.yaw) * 3 }));
  if (poses.some((p, index) => !companionDrivingSegmentClear(navigation, poses[Math.max(0, index - 1)], p))
    || pullOvers.some((p, index) => !companionDrivingSegmentClear(navigation, berthOrigins[index], p))) return null;
  return Object.freeze({ id: rules.routeId, navigationRevision: navigation.revision,
    points: Object.freeze(poses.map(point)), halfWidth: 5, start: poses[0], pullOvers: Object.freeze(pullOvers) });
}
function assertAuthority(authority: CompanionDrivingAuthority, vehicle: CompanionPlayerVehicle, state?: CompanionPlayerDrivingState) {
  assertCompanionPlayerVehicle(vehicle);
  const { route, navigation } = authority;
  if (![authority.userId, authority.characterId, authority.lifeId, authority.gameRoomId, authority.instanceId].every(key)
    || !integer(authority.now) || !integer(authority.worldRevision) || !authority.present || !authority.ready
    || authority.movementOwned || authority.journeyActive || authority.transitPassenger)
    throw new Error("Driving is unavailable for this visit.");
  if (vehicle.ownerUserId !== authority.userId || vehicle.model.kind !== rules.kind || vehicle.activeTripId
    || vehicle.condition <= 0 || vehicle.forSale || vehicle.forHire || !state && vehicle.driving
    || !sameCompanionPlayerVehiclePlace(vehicle.location, authority.location))
    throw new Error("Choose your parked, working keke.");
  if (route.id !== rules.routeId || route.navigationRevision !== navigation.revision || route.points.length < 2
    || route.points.length > 32 || route.halfWidth !== 5 || !poseValid(route.start)
    || route.points.some(p => p.length !== 3 || !p.every(Number.isFinite) || Math.abs(p[1] - navigation.floorY) > .01))
    throw new Error("This driving route is unavailable.");
  if (authority.bodies.length > rules.maxBodies || new Set(authority.bodies.map(body => body.id)).size !== authority.bodies.length
    || authority.bodies.some(body => !key(body.id) || !poseValid(body.pose) || ![body.halfWidth, body.halfLength, body.speed].every(Number.isFinite)
      || body.halfWidth <= 0 || body.halfWidth > 3 || body.halfLength <= 0 || body.halfLength > 8 || body.speed < 0 || body.speed > 35))
    throw new Error("Refresh road occupancy before driving.");
  const occupants = authority.representedOccupants.filter(occupant => occupant.userId === authority.userId || occupant.vehicleId === vehicle.vehicleId);
  if (authority.representedOccupants.length > 256 || (state ? occupants.length !== 1
    || occupants[0].userId !== authority.userId || occupants[0].vehicleId !== vehicle.vehicleId || occupants[0].role !== "driver" : occupants.length !== 0))
    throw new Error("Leave the other vehicle or seat first.");
  if (state && (state.vehicleId !== vehicle.vehicleId || state.vehicleRevision !== vehicle.revision || state.driver.userId !== authority.userId
    || vehicle.driving?.id !== state.id || vehicle.driving.revision !== state.revision || vehicle.driving.sequence !== state.sequence
    || state.driver.characterId !== authority.characterId || state.driver.lifeId !== authority.lifeId
    || state.gameRoomId !== authority.gameRoomId || state.instanceId !== authority.instanceId
    || state.navigationRevision !== navigation.revision || state.routeId !== route.id || state.occupants.length !== 1
    || state.driver.role !== "driver" || state.driver.seat !== 0
    || JSON.stringify(state.occupants[0]) !== JSON.stringify(state.driver) || !poseValid(state.pose)
    || !Number.isFinite(state.speed) || state.speed < 0 || state.speed > rules.maxSpeed || authority.now < state.acceptedAt
    || !integer(state.revision) || !integer(state.sequence) || !integer(state.acceptedAt) || !integer(state.inputExpiresAt)
    || ![state.drivenMetres, state.fuelMetres].every(value => Number.isFinite(value) && value >= 0)
    || state.fuelMetres >= vehicle.model.metresPerFuelUnit || !validCompanionDrivingInput(state.input)
    || state.dynamics && !validVehicleDynamics(state.dynamics, companionDrivingDynamicsConfig))) throw new Error("Refresh the accepted driving state.");
  if (state?.dynamics) {
    const bodyPose = vehicleDynamicsPose(state.dynamics, companionDrivingDynamicsConfig);
    if (distance(bodyPose, state.pose) > .001 || Math.abs(wrap(bodyPose.yaw - state.pose.yaw)) > .001
      || Math.abs(bodyPose.y - state.pose.y) > 2) throw new Error("Refresh the accepted chassis state.");
  }
  if (state && (routeDistance(route, state.pose) > route.halfWidth - radius
    || !companionDrivingSegmentClear(navigation, state.pose, state.pose)
    || authority.bodies.some(body => body.id === authority.userId || body.id === authority.characterId || body.id === `player:${authority.userId}`)))
    throw new Error("Refresh the vehicle's road and occupant representation.");
}
/** Resolve an existing geographic road point without snapping, relocating or refueling the vehicle. */
export function resolveCompanionDrivingParkedPose(vehicle: CompanionPlayerVehicle, route: CompanionDrivingRoute): CompanionDrivingPose | null {
  if (vehicle.location.kind !== "road" || vehicle.model.kind !== rules.kind) return null;
  const x = (vehicle.location.point.longitude - plan.origin.longitude) * 111_320 * Math.cos(plan.origin.latitude * Math.PI / 180);
  const z = (plan.origin.latitude - vehicle.location.point.latitude) * 111_320;
  const parked = vehicle.drivingParked;
  if (parked) {
    if (parked.navigationRevision !== route.navigationRevision || !poseValid(parked.pose)
      || Math.hypot(parked.pose.x - x, parked.pose.z - z) > .01 || Math.abs(parked.pose.y - route.start.y) > .01
      || routeDistance(route, parked.pose) > route.halfWidth - radius) return null;
    return parked.pose;
  }
  let yaw = 0, closest = Infinity;
  for (let i = 1; i < route.points.length; i++) {
    const a = route.points[i - 1], b = route.points[i], dx = b[0] - a[0], dz = b[2] - a[2], length = dx * dx + dz * dz;
    const t = length ? Math.max(0, Math.min(1, ((x - a[0]) * dx + (z - a[2]) * dz) / length)) : 0;
    const gap = Math.hypot(x - a[0] - t * dx, z - a[2] - t * dz);
    if (gap < closest) { closest = gap; yaw = Math.atan2(dx, dz); }
  }
  return closest <= route.halfWidth - radius ? Object.freeze({ x, y: route.start.y, z, yaw }) : null;
}
function blockedByBody(authority: CompanionDrivingAuthority, vehicleId: string, from: CompanionDrivingPose,
  to: CompanionDrivingPose, dt: number) {
  for (const body of authority.bodies) {
    if (body.id === vehicleId || Math.abs(body.pose.y - from.y) > 1.5) continue;
    const x = to.x - from.x, z = to.z - from.z, length = x * x + z * z;
    const t = length ? Math.max(0, Math.min(1, ((body.pose.x - from.x) * x + (body.pose.z - from.z) * z) / length)) : 0;
    const clearance = radius + Math.hypot(body.halfWidth, body.halfLength) + body.speed * dt;
    if (Math.hypot(body.pose.x - from.x - t * x, body.pose.z - from.z - t * z) <= clearance) return body;
  }
  return null;
}
const freeze = (state: CompanionPlayerDrivingState): CompanionPlayerDrivingState => {
  const dynamics = state.dynamics && copyVehicleDynamics(state.dynamics);
  if (dynamics) {
    Object.freeze(dynamics.position); Object.freeze(dynamics.orientation); Object.freeze(dynamics.velocity);
    Object.freeze(dynamics.angularVelocity); dynamics.wheels.forEach(Object.freeze); Object.freeze(dynamics.wheels); Object.freeze(dynamics);
  }
  return Object.freeze({ ...state, ...(dynamics ? { dynamics } : {}),
    pose: Object.freeze({ ...state.pose }), input: Object.freeze({ ...state.input }), driver: Object.freeze({ ...state.driver }),
    occupants: Object.freeze(state.occupants.map(occupant => Object.freeze({ ...occupant }))) });
};

export function beginCompanionPlayerDriving(vehicle: CompanionPlayerVehicle, authority: CompanionDrivingAuthority,
  binding: Readonly<{ id: string; vehicleRevision: number; pose: CompanionDrivingPose; fuelMetres?: number }>): CompanionPlayerDrivingState {
  assertAuthority(authority, vehicle);
  if (!key(binding.id) || binding.vehicleRevision !== vehicle.revision || vehicle.fuelUnits <= 0
    || !poseValid(binding.pose) || routeDistance(authority.route, binding.pose) > authority.route.halfWidth - radius
    || Math.hypot(authority.position[0] - binding.pose.x, authority.position[2] - binding.pose.z) > rules.boardingReach
    || Math.abs(authority.position[1] - binding.pose.y) > .3
    || !companionDrivingSegmentClear(authority.navigation, binding.pose, binding.pose)
    || blockedByBody(authority, vehicle.vehicleId, binding.pose, binding.pose, 0)
    || !Number.isFinite(binding.fuelMetres ?? 0) || (binding.fuelMetres ?? 0) < 0
    || (binding.fuelMetres ?? 0) >= vehicle.model.metresPerFuelUnit) throw new Error("Reach this keke on the supported road first.");
  if ((binding.fuelMetres ?? 0) !== (vehicle.drivingFuelMetres ?? vehicle.drivingParked?.fuelMetres ?? 0))
    throw new Error("Retain the vehicle's accepted fuel distance.");
  const resolved = resolveCompanionDrivingParkedPose(vehicle, authority.route);
  if (!resolved || distance(resolved, binding.pose) > .01 || Math.abs(resolved.y - binding.pose.y) > .01
    || Math.abs(wrap(resolved.yaw - binding.pose.yaw)) > .01)
    throw new Error("Use the vehicle's exact saved road position.");
  const driver = { userId: authority.userId, characterId: authority.characterId, lifeId: authority.lifeId, role: "driver" as const, seat: 0 as const };
  return freeze({ id: binding.id, vehicleId: vehicle.vehicleId, vehicleRevision: vehicle.revision,
    gameRoomId: authority.gameRoomId, instanceId: authority.instanceId, navigationRevision: authority.navigation.revision,
    routeId: authority.route.id, driver, occupants: [driver], revision: 0, sequence: 0, pose: binding.pose,
    speed: 0, mode: "stopped", input: neutralCompanionDrivingInput, acceptedAt: authority.now,
    inputExpiresAt: authority.now, drivenMetres: 0, fuelMetres: binding.fuelMetres ?? 0, surface: "clean_asphalt", contact: null,
    dynamics: createVehicleDynamics(companionDrivingDynamicsConfig, binding.pose) });
}

/** Called from the existing world transaction. Prediction uses the same math but cannot settle effects. */
export function advanceCompanionPlayerDriving(vehicle: CompanionPlayerVehicle, state: CompanionPlayerDrivingState,
  command: CompanionDrivingCommand, authority: CompanionDrivingAuthority, handling: CompanionDrivingHandling = baseHandling,
  prediction = false): CompanionDrivingResult {
  assertAuthority(authority, vehicle, state);
  if (command.drivingId !== state.id || command.vehicleId !== vehicle.vehicleId || command.vehicleRevision !== vehicle.revision
    || command.drivingRevision !== state.revision || command.gameRoomId !== authority.gameRoomId
    || command.instanceId !== authority.instanceId || command.lifeId !== authority.lifeId || command.worldRevision !== authority.worldRevision
    || command.sequence !== state.sequence + 1 || !key(command.requestId) || !validCompanionDrivingInput(command.input)
    || !["input", "stop", "pull-over", "exit", "horn"].includes(command.action)) throw new Error("This driving command is stale.");
  if (![handling.accelerationScale, handling.brakingScale, handling.speedLimit, handling.steeringBias].every(Number.isFinite)
    || handling.accelerationScale < 0 || handling.accelerationScale > 1 || handling.brakingScale < .5 || handling.brakingScale > 1
    || handling.speedLimit < 0 || handling.speedLimit > rules.maxSpeed || Math.abs(handling.steeringBias) > .12
    || !["clean_asphalt", "laterite_mud", "pothole", "gutter_wood"].includes(handling.surface)) throw new Error("Road handling is unavailable.");
  let pose = state.pose, speed = state.speed, metres = 0;
  let dynamics = state.dynamics ?? createVehicleDynamics(companionDrivingDynamicsConfig, pose, speed);
  // A legacy external hold must not leave wheel momentum behind for the next input.
  if (speed === 0) dynamics = holdVehicleDynamics(dynamics);
  // A command never accelerates retroactively. Only the previously accepted, unexpired input owns elapsed motion.
  const fresh = authority.now - state.acceptedAt <= rules.inputTtlMs && state.mode !== "suspended";
  const elapsed = fresh && handling.speedLimit > 0
    ? Math.min(rules.stepMs, authority.now - state.acceptedAt, Math.max(0, state.inputExpiresAt - state.acceptedAt)) : 0;
  const input = command.action === "stop" || command.action === "exit" || command.action === "pull-over"
    ? neutralCompanionDrivingInput : state.input;
  const contact: CompanionPlayerDrivingState["contact"] = null;
  const ground = (origin: { x: number; y: number; z: number }, direction: { x: number; y: number; z: number }, length: number) =>
    castVehicleRoadPlane(origin, direction, length, authority.navigation.floorY, handling.surface);
  for (let left = elapsed; left > 0; left -= rules.substepMs) {
    const dt = Math.min(rules.substepMs, left) / 1000;
    const limit = Math.min(handling.speedLimit, vehicle.fuelUnits > 0 ? rules.maxSpeed : 0);
    const driver = { ...input, steering: input.steering + handling.steeringBias,
      accelerationScale: handling.accelerationScale, brakingScale: handling.brakingScale, speedLimit: limit };
    let advanced = stepVehicleDynamics(dynamics, companionDrivingDynamicsConfig, driver, dt, ground);
    let integrated = vehicleDynamicsPose(advanced, companionDrivingDynamicsConfig);
    const remainingFuelMetres = Math.max(0, vehicle.fuelUnits * vehicle.model.metresPerFuelUnit - state.fuelMetres - metres);
    const travelled = distance(pose, integrated);
    const fuelEnded = travelled >= remainingFuelMetres;
    if (fuelEnded) {
      // Fuel exhaustion is an authority boundary, not an extra engine/brake force.
      const fraction = travelled > 0 ? remainingFuelMetres / travelled : 0;
      const x = pose.x + (integrated.x - pose.x) * fraction, z = pose.z + (integrated.z - pose.z) * fraction;
      advanced.orientation = { ...dynamics.orientation };
      const clipped = vehicleDynamicsPose(advanced, companionDrivingDynamicsConfig);
      advanced.position.x += x - clipped.x; advanced.position.z += z - clipped.z;
      advanced = holdVehicleDynamics(advanced);
      integrated = vehicleDynamicsPose(advanced, companionDrivingDynamicsConfig);
    }
    const next = { x: integrated.x, y: pose.y, z: integrated.z, yaw: integrated.yaw };
    const body = blockedByBody(authority, vehicle.vehicleId, pose, next, elapsed / 1000);
    const clear = companionDrivingSegmentClear(authority.navigation, pose, next)
      && routeDistance(authority.route, next) <= authority.route.halfWidth - radius;
    if (body || !clear) {
      // Clearance rejection stops before contact. Existing crash owner alone proves physical impacts.
      speed = 0; dynamics = holdVehicleDynamics(dynamics); break;
    }
    metres += fuelEnded ? remainingFuelMetres : distance(pose, next); pose = next; dynamics = advanced;
    speed = Math.min(rules.maxSpeed, Math.hypot(dynamics.velocity.x, dynamics.velocity.z));
    if (fuelEnded) { speed = 0; break; }
  }
  if (!fresh || command.action === "stop" || handling.speedLimit === 0) { speed = 0; dynamics = holdVehicleDynamics(dynamics); }
  let mode: CompanionPlayerDrivingState["mode"] = speed > rules.stoppedSpeed ? "driving" : "stopped";
  let exitPoint: ScenePoint | null = null;
  if (command.action === "pull-over") {
    if (speed > rules.stoppedSpeed || !authority.route.pullOvers.some(p => distance(p, pose) <= .9))
      throw new Error("Stop in a marked pull-over first.");
    speed = 0; dynamics = holdVehicleDynamics(dynamics); mode = "pulled-over";
  }
  if (command.action === "exit") {
    if (speed > rules.stoppedSpeed) throw new Error("Stop before leaving the keke.");
    const offsets = [1, -1, 2];
    for (const offset of offsets) {
      const candidate: ScenePoint = [pose.x + Math.cos(pose.yaw) * (rules.halfWidth + .65) * offset,
        pose.y, pose.z - Math.sin(pose.yaw) * (rules.halfWidth + .65) * offset];
      const candidatePose = { x: candidate[0], y: candidate[1], z: candidate[2], yaw: pose.yaw };
      if (routeSegmentClear(authority.navigation, candidate, candidate)
        && routeSegmentClear(authority.navigation, point(pose), candidate)
        && !blockedByBody(authority, vehicle.vehicleId, candidatePose, candidatePose, 0)) { exitPoint = candidate; break; }
    }
    if (!exitPoint) throw new Error("Move to a clear stopping point before exiting.");
    speed = 0; dynamics = holdVehicleDynamics(dynamics); mode = "suspended";
  }
  const fuelMetres = state.fuelMetres + metres, spent = Math.floor(fuelMetres / vehicle.model.metresPerFuelUnit);
  const nextVehicle = Object.freeze({ ...vehicle, fuelUnits: Math.max(0, vehicle.fuelUnits - spent),
    drivingFuelMetres: fuelMetres % vehicle.model.metresPerFuelUnit,
    revision: vehicle.revision + 1,
    odometerMetres: vehicle.odometerMetres + Math.floor(state.fuelMetres + metres) - Math.floor(state.fuelMetres) });
  const next = freeze({ ...state, vehicleRevision: nextVehicle.revision, revision: state.revision + 1, sequence: command.sequence,
    pose, speed, mode, dynamics, drivenMetres: state.drivenMetres + metres,
    fuelMetres: fuelMetres % vehicle.model.metresPerFuelUnit, surface: handling.surface, contact,
    input: command.action === "input" && mode !== "suspended" ? command.input
      : command.action === "horn" && fresh ? state.input : neutralCompanionDrivingInput,
    acceptedAt: authority.now, inputExpiresAt: authority.now + rules.inputTtlMs });
  return Object.freeze({ state: next, vehicle: Object.freeze({ ...nextVehicle, driving: command.action === "exit" ? null : next }),
    exitPoint, driverAction: command.action, accepted: !prediction });
}

/** Forced stop preserves vehicle/cargo/location and never applies offline fuel or damage. */
export function suspendCompanionPlayerDriving(state: CompanionPlayerDrivingState, now: number): CompanionPlayerDrivingState {
  if (!integer(now) || now < state.acceptedAt) throw new Error("Invalid driving stop time.");
  return freeze({ ...state, speed: 0, mode: "suspended", input: neutralCompanionDrivingInput,
    ...(state.dynamics ? { dynamics: holdVehicleDynamics(state.dynamics) } : {}),
    acceptedAt: now, inputExpiresAt: now, revision: state.revision + 1 });
}
