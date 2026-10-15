import type { CompanionPosition } from "./companionCall";
import { planRoomRoute, routeSegmentClear, type RoomNavigation, type ScenePoint } from "./companionNavigation";

export const CALL_MOVEMENT_INTERVAL_MS = 250;
export const CALL_MOVEMENT_MAX_POINTS = 32;
export type CompanionPace = "walk" | "run" | "fast";
// Player commands name their pace. Unpaced saved/NPC routes retain their timing.
export const companionPaceSpeed = (pace?: CompanionPace) => pace === "fast" ? 5.6 : pace === "run" ? 3.6 : pace === "walk" ? 1.4 : .65;
export const companionPaceAcceleration = (pace?: CompanionPace) => pace === "fast" ? 28.8 : pace ? 7.2 : 1.8;
const TURN_SPEED = 4;
const angle = (value: number) => Math.atan2(Math.sin(value), Math.cos(value));

/** Times use the server clock on the wire and the viewer's clock after receipt. */
export type CompanionMovement = {
  version: 1;
  sequence: number;
  roomRevision: number;
  startedAt: number;
  from: CompanionPosition;
  initialSpeed?: number;
  pace?: CompanionPace;
  path: { x: number; z: number }[];
  yaw: number;
};
export type CompanionWalkPath = readonly { x: number; z: number }[];
export type CompanionCollisionStop = { sequence: number; elapsedMs: number };
export type CompanionMoveRequest = { sequence: number; roomRevision: number; destination: CompanionPosition; stop?: boolean; pace?: CompanionPace; path?: CompanionWalkPath; collision?: CompanionCollisionStop };
export type CompanionMoveResult = {
  ok: boolean; sequence: number; roomRevision: number; position: CompanionPosition; movement: CompanionMovement | null;
  retryAfterMs: number; reason?: string;
};

function travel(distance: number, initialSpeed = 0, pace?: CompanionPace) {
  const SPEED = companionPaceSpeed(pace);
  const ACCELERATION = companionPaceAcceleration(pace);
  const start = Math.max(0, Math.min(SPEED, initialSpeed, Math.sqrt(2 * ACCELERATION * distance)));
  const peak = Math.min(SPEED, Math.sqrt(ACCELERATION * distance + start * start / 2));
  const ramp = (peak - start) / ACCELERATION, rampDistance = (start + peak) * ramp / 2;
  const cruise = peak > 0 ? Math.max(0, distance - rampDistance - peak * peak / (2 * ACCELERATION)) / peak : 0;
  return { start, ramp, rampDistance, peak, cruise, acceleration: ACCELERATION, duration: ramp + cruise + peak / ACCELERATION };
}

/** Turns stay on the spot. Translation stays on the collision-checked polyline. */
export function sampleCompanionMovement(movement: CompanionMovement, now: number) {
  let seconds = Math.max(0, (now - movement.startedAt) / 1000);
  let position = { ...movement.from };
  let initialSpeed = movement.initialSpeed ?? 0;
  for (const point of movement.path) {
    const dx = point.x - position.x, dz = point.z - position.z, distance = Math.hypot(dx, dz);
    if (distance < .000001) continue;
    const heading = Math.atan2(dx, dz), turn = angle(heading - position.yaw), turnDuration = Math.abs(turn) / TURN_SPEED;
    if (seconds < turnDuration) return { position: { ...position, yaw: angle(position.yaw + turn * seconds / turnDuration) }, moving: true, speed: 0 };
    seconds -= turnDuration;
    const profile = travel(distance, turnDuration < .001 ? initialSpeed : 0, movement.pace);
    const ACCELERATION = profile.acceleration;
    if (seconds < profile.duration) {
      const speed = seconds < profile.ramp ? profile.start + ACCELERATION * seconds : seconds < profile.ramp + profile.cruise ? profile.peak : ACCELERATION * (profile.duration - seconds);
      const walked = seconds < profile.ramp ? profile.start * seconds + .5 * ACCELERATION * seconds ** 2
        : seconds < profile.ramp + profile.cruise ? profile.rampDistance + profile.peak * (seconds - profile.ramp)
        : distance - .5 * ACCELERATION * (profile.duration - seconds) ** 2;
      return { position: { x: position.x + dx * walked / distance, z: position.z + dz * walked / distance, yaw: heading }, moving: true, speed };
    }
    seconds -= profile.duration;
    initialSpeed = 0;
    position = { ...point, yaw: heading };
  }
  const turn = angle(movement.yaw - position.yaw), duration = Math.abs(turn) / TURN_SPEED;
  if (seconds < duration) return { position: { ...position, yaw: angle(position.yaw + turn * seconds / duration) }, moving: true, speed: 0 };
  return { position: { ...position, yaw: movement.yaw }, moving: false, speed: 0 };
}

export function companionMovementEndsAt(movement: CompanionMovement) {
  let seconds = 0, position = movement.from, initialSpeed = movement.initialSpeed ?? 0;
  for (const point of movement.path) {
    const distance = Math.hypot(point.x - position.x, point.z - position.z);
    if (distance < .000001) continue;
    const heading = Math.atan2(point.x - position.x, point.z - position.z);
    const turnDuration = Math.abs(angle(heading - position.yaw)) / TURN_SPEED;
    seconds += turnDuration + travel(distance, turnDuration < .001 ? initialSpeed : 0, movement.pace).duration;
    initialSpeed = 0;
    position = { ...point, yaw: heading };
  }
  return movement.startedAt + (seconds + Math.abs(angle(movement.yaw - position.yaw)) / TURN_SPEED) * 1000;
}

/** Include every corner crossed in a time window, even between rendered frames. */
export function companionMovementWindow(movement: CompanionMovement, fromAt: number, toAt: number): CompanionPosition[] {
  const points = [sampleCompanionMovement(movement, fromAt).position];
  let at = movement.startedAt, position = movement.from, initialSpeed = movement.initialSpeed ?? 0;
  for (const point of movement.path) {
    const distance = Math.hypot(point.x - position.x, point.z - position.z);
    if (distance < .000001) continue;
    const heading = Math.atan2(point.x - position.x, point.z - position.z);
    const turnDuration = Math.abs(angle(heading - position.yaw)) / TURN_SPEED;
    at += (turnDuration + travel(distance, turnDuration < .001 ? initialSpeed : 0, movement.pace).duration) * 1000;
    initialSpeed = 0;
    position = { ...point, yaw: heading };
    if (at > fromAt && at < toAt) points.push(position);
    if (at >= toAt) break;
  }
  points.push(sampleCompanionMovement(movement, toAt).position);
  return points;
}

/** Collision receipts can only stop on the same accepted route, at or behind
 * its current progress. Bound latency correction to two seconds of travel. */
export function companionCollisionStopPosition(movement: CompanionMovement | null | undefined,
  collision: CompanionCollisionStop | undefined, now: number): CompanionPosition | null {
  if (!movement || !collision || collision.sequence !== movement.sequence || !Number.isFinite(now)
    || !Number.isFinite(collision.elapsedMs) || collision.elapsedMs < 0) return null;
  const elapsed = Math.max(0, now - movement.startedAt);
  const stoppedAt = movement.startedAt + Math.max(0, elapsed - 2000, Math.min(elapsed, collision.elapsedMs));
  return sampleCompanionMovement(movement, stoppedAt).position;
}

export function planCompanionMovement(navigation: RoomNavigation, from: CompanionPosition, request: CompanionMoveRequest, startedAt: number, initialSpeed = 0): CompanionMovement | null {
  if (![from.x, from.z, from.yaw, request.destination.x, request.destination.z, request.destination.yaw, startedAt].every(Number.isFinite)
    || Math.abs(from.yaw) > Math.PI || Math.abs(request.destination.yaw) > Math.PI
    || !Number.isSafeInteger(request.sequence) || request.sequence < 1 || request.sequence > 1_000_000_000
    || !Number.isSafeInteger(request.roomRevision) || request.roomRevision < 0
    || request.pace !== undefined && request.pace !== "walk" && request.pace !== "run" && request.pace !== "fast") return null;
  const start: ScenePoint = [from.x, navigation.floorY, from.z];
  let route: ScenePoint[] | null;
  if (request.path !== undefined && !request.stop) {
    // Preserve the renderer's detour around moving scenery. A proposed route
    // never grants clearance: the authority checks every segment again.
    if (!Array.isArray(request.path) || !request.path.length || request.path.length > CALL_MOVEMENT_MAX_POINTS
      || request.path.some(p => !p || !Number.isFinite(p.x) || !Number.isFinite(p.z))) return null;
    const last = request.path[request.path.length - 1];
    if (Math.hypot(last.x - request.destination.x, last.z - request.destination.z) > .001) return null;
    route = request.path.map<ScenePoint>(p => [p.x, navigation.floorY, p.z]);
    let previous = start;
    for (const next of route) {
      if (!routeSegmentClear(navigation, previous, next)) return null;
      previous = next;
    }
  } else route = planRoomRoute(navigation, start, [request.destination.x, navigation.floorY, request.destination.z]);
  if (!route || route.length > CALL_MOVEMENT_MAX_POINTS) return null;
  const movement: CompanionMovement = { version: 1, sequence: request.sequence, roomRevision: request.roomRevision, startedAt,
    from: { ...from }, initialSpeed: Number.isFinite(initialSpeed) ? Math.max(0, Math.min(companionPaceSpeed(request.pace), initialSpeed)) : 0,
    ...(request.pace && !request.stop ? { pace: request.pace } : {}),
    path: route.map(([x, , z]) => ({ x, z })), yaw: request.destination.yaw };
  return companionMovementEndsAt(movement) - startedAt <= 90_000 ? movement : null;
}
