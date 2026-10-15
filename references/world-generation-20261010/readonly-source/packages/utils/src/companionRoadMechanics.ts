import { routeSegmentClear, type RoomNavigation, type ScenePoint } from "./companionNavigation";
import type { CompanionDrivingPose, CompanionPlayerDrivingState } from "./companionPlayerDriving";

/** Simulated, noncash tuning in real milliseconds. */
export const companionRoadMechanicsRules = Object.freeze({
  version: "lagos-road-mechanics-v1", gapMs: 1250, mergeYieldMs: 1800,
  hornCooldownMs: 900, hornPulseMs: 450, hornRadius: 24, hornYieldMs: 1800,
  reactionCooldownMs: 3500, maxGaps: 8, maxReactions: 16, maxBodies: 128,
  maxCorridorPoints: 32, maxCorridorMs: 2500, clearance: .15,
  matchingSpeed: 2, minimumThrottle: .05, maximumThrottle: .8,
});
export type CompanionRoadScope = Readonly<{
  instanceId: string; layoutRevision: string; drivingId: string; vehicleId: string; lifeId: string;
}>;
export type CompanionRoadAcceptedFrame = Readonly<{
  scope: CompanionRoadScope; sequence: number; at: number; pose: CompanionDrivingPose; speed: number;
  active: boolean; engineRunning: boolean; externalHold: boolean;
  throttle: number; brake: number; steering: number;
}>;
export type CompanionRoadBody = Readonly<{
  id: string; pose: CompanionDrivingPose; speed: number; halfWidth: number; halfLength: number;
  owner: "ambient" | "player" | "responder"; kind: "vehicle" | "pedestrian"; held: boolean;
  /** Current accepted offset for ambient lane owners. Required for a shoulder reaction. */
  laneLateral?: number;
}>;
export type CompanionRoadCorridor = readonly Readonly<{ at: number; pose: CompanionDrivingPose }>[];
/** Checks the complete swept chassis against current fixed geometry, road edges and reservations.
 * Dynamic bodies are also checked here. Passing a centre-point test is insufficient. */
export type CompanionRoadCorridorClear = (body: CompanionRoadBody, corridor: CompanionRoadCorridor) => boolean;
/** Reuses the accepted frozen navigation. The enclosing disc covers the chassis throughout every turn. */
export function companionRoadNavigationClear(navigation: RoomNavigation): CompanionRoadCorridorClear {
  return (body, corridor) => {
    if (!bodyValid(body) || corridor.length < 2 || corridor.length > companionRoadMechanicsRules.maxCorridorPoints) return false;
    const radius = Math.hypot(body.halfWidth, body.halfLength) + companionRoadMechanicsRules.clearance;
    return corridor.every((sample, index) => poseValid(sample.pose) && routeSegmentClear({ ...navigation, radius },
      point(corridor[Math.max(0, index - 1)].pose), point(sample.pose)));
  };
}
export type CompanionRoadHornPulse = Readonly<{
  id: string; scope: CompanionRoadScope; sequence: number; at: number; expiresAt: number;
  point: ScenePoint; yaw: number; radius: number; noise: number;
}>;
export type CompanionRoadHornState = Readonly<{
  scope: CompanionRoadScope; lastSequence: number; lastAt: number | null;
}>;
export type CompanionRoadGap = Readonly<{
  id: string; leadingId: string; trailingId: string; opensAt: number; closesAt: number;
  corridor: CompanionRoadCorridor; yieldedAt: number | null; cooldownUntil: number;
}>;
export type CompanionRoadReaction = Readonly<{
  id: string; vehicleId: string; causeId: string; kind: "merge" | "horn";
  startsAt: number; endsAt: number; cooldownUntil: number;
  /** Absolute, accepted lane offset. The lane owner validates every outward and return sweep. */
  lateral: number; speedLimit: number;
}>;
export type CompanionRoadTrafficState = Readonly<{
  scope: CompanionRoadScope; revision: number; observedAt: number; lastGapSequence: number; gaps: readonly CompanionRoadGap[];
  reactions: readonly CompanionRoadReaction[];
}>;

const idValid = (id: string) => typeof id === "string" && id.length > 0 && id.length <= 240 && id.trim() === id;
const timeValid = (at: number) => Number.isSafeInteger(at) && at >= 0;
const clamp = (value: number, low: number, high: number) => Math.min(high, Math.max(low, value));
const angle = (yaw: number) => Math.atan2(Math.sin(yaw), Math.cos(yaw));
const point = (pose: CompanionDrivingPose): ScenePoint => Object.freeze([pose.x, pose.y, pose.z]);
const poseValid = (pose: CompanionDrivingPose) => !!pose && [pose.x, pose.y, pose.z, pose.yaw].every(Number.isFinite);
export const sameCompanionRoadScope = (a: CompanionRoadScope, b: CompanionRoadScope) =>
  !!a && !!b && a.instanceId === b.instanceId && a.layoutRevision === b.layoutRevision && a.drivingId === b.drivingId
  && a.vehicleId === b.vehicleId && a.lifeId === b.lifeId;
export const validCompanionRoadScope = (scope: CompanionRoadScope) => !!scope
  && [scope.instanceId, scope.layoutRevision, scope.drivingId, scope.vehicleId, scope.lifeId].every(idValid);
export const validCompanionRoadFrame = (frame: CompanionRoadAcceptedFrame) => !!frame && validCompanionRoadScope(frame.scope)
  && Number.isSafeInteger(frame.sequence) && frame.sequence >= 0 && timeValid(frame.at) && poseValid(frame.pose)
  && Number.isFinite(frame.speed) && Math.abs(frame.speed) <= 35
  && [frame.throttle, frame.brake, frame.steering].every(Number.isFinite)
  && frame.throttle >= 0 && frame.throttle <= 1 && frame.brake >= 0 && frame.brake <= 1 && Math.abs(frame.steering) <= 1
  && [frame.active, frame.engineRunning, frame.externalHold].every(value => typeof value === "boolean");
const bodyValid = (body: CompanionRoadBody) => !!body && idValid(body.id) && poseValid(body.pose)
  && Number.isFinite(body.speed) && Math.abs(body.speed) <= 35
  && Number.isFinite(body.halfWidth) && body.halfWidth >= .15 && body.halfWidth <= 3
  && Number.isFinite(body.halfLength) && body.halfLength >= .15 && body.halfLength <= 8
  && ["ambient", "player", "responder"].includes(body.owner) && ["vehicle", "pedestrian"].includes(body.kind)
  && typeof body.held === "boolean" && (body.laneLateral === undefined || Number.isFinite(body.laneLateral) && Math.abs(body.laneLateral) <= 4);
const frameLive = (frame: CompanionRoadAcceptedFrame) => validCompanionRoadFrame(frame) && frame.active
  && frame.engineRunning && !frame.externalHold;

export function companionRoadFrameFromDriving(state: CompanionPlayerDrivingState,
  lifecycle: Readonly<{ active: boolean; engineRunning: boolean; externalHold: boolean }>): CompanionRoadAcceptedFrame {
  return Object.freeze({ scope: Object.freeze({ instanceId: state.instanceId, layoutRevision: state.navigationRevision,
    drivingId: state.id, vehicleId: state.vehicleId, lifeId: state.driver.lifeId }), sequence: state.sequence,
    at: state.acceptedAt, pose: Object.freeze({ ...state.pose }), speed: state.speed,
    ...lifecycle, active: lifecycle.active && state.mode !== "suspended",
    throttle: state.input.throttle, brake: state.input.brake, steering: state.input.steering });
}
export function createCompanionRoadHornState(scope: CompanionRoadScope): CompanionRoadHornState {
  if (!validCompanionRoadScope(scope)) throw new Error("Use the accepted driving scope.");
  return Object.freeze({ scope: Object.freeze({ ...scope }), lastSequence: -1, lastAt: null });
}
/** Invoke only after the driving owner accepts the finite horn command. Playback never admits a pulse. */
export function admitCompanionRoadHorn(state: CompanionRoadHornState, frame: CompanionRoadAcceptedFrame) {
  if (!frameLive(frame) || !sameCompanionRoadScope(state.scope, frame.scope)
    || frame.sequence <= state.lastSequence || state.lastAt !== null && frame.at < state.lastAt)
    return { state, pulse: null, accepted: false } as const;
  // Consume the input sequence even when cooldown rejects it, so it cannot replay later.
  const next = Object.freeze({ ...state, lastSequence: frame.sequence });
  if (state.lastAt !== null && frame.at - state.lastAt < companionRoadMechanicsRules.hornCooldownMs)
    return { state: next, pulse: null, accepted: false } as const;
  const pulse: CompanionRoadHornPulse = Object.freeze({ id: `${frame.scope.drivingId}:horn:${frame.sequence}`,
    scope: Object.freeze({ ...frame.scope }), sequence: frame.sequence, at: frame.at,
    expiresAt: frame.at + companionRoadMechanicsRules.hornPulseMs, point: point(frame.pose), yaw: frame.pose.yaw,
    radius: companionRoadMechanicsRules.hornRadius, noise: 1 });
  return { state: Object.freeze({ ...next, lastAt: frame.at }), pulse, accepted: true } as const;
}
/** Local gameplay evidence. Supply actual sound-boundary gain, with zero for a blocked approach. */
export function companionRoadHornExposure(pulse: CompanionRoadHornPulse, target: ScenePoint, now: number,
  boundaryGain: (from: ScenePoint, to: ScenePoint) => number): number {
  if (!pulse || !timeValid(now) || !timeValid(pulse.at) || !timeValid(pulse.expiresAt)
    || pulse.expiresAt - pulse.at !== companionRoadMechanicsRules.hornPulseMs
    || pulse.radius !== companionRoadMechanicsRules.hornRadius || pulse.noise !== 1
    || !Array.isArray(pulse.point) || pulse.point.length !== 3 || !pulse.point.every(Number.isFinite)
    || now < pulse.at || now >= pulse.expiresAt || !validCompanionRoadScope(pulse.scope)
    || !Array.isArray(target) || target.length !== 3 || !target.every(Number.isFinite)) return 0;
  const distance = Math.hypot(target[0] - pulse.point[0], target[1] - pulse.point[1], target[2] - pulse.point[2]);
  if (!Number.isFinite(distance) || distance >= pulse.radius) return 0;
  const gain = boundaryGain(pulse.point, target);
  return Number.isFinite(gain) ? pulse.noise * clamp(gain, 0, 1) * (1 - distance / pulse.radius) : 0;
}

const freezeCorridor = (corridor: CompanionRoadCorridor): CompanionRoadCorridor => Object.freeze(corridor.map(sample =>
  Object.freeze({ at: sample.at, pose: Object.freeze({ ...sample.pose }) })));
const interpolate = (a: CompanionDrivingPose, b: CompanionDrivingPose, t: number): CompanionDrivingPose => ({
  x: a.x + (b.x - a.x) * t, y: a.y + (b.y - a.y) * t, z: a.z + (b.z - a.z) * t,
  yaw: a.yaw + angle(b.yaw - a.yaw) * t,
});
function overlaps(a: CompanionDrivingPose, body: CompanionRoadBody, b: CompanionDrivingPose,
  other: CompanionRoadBody, padding: number) {
  if (Math.abs(a.y - b.y) > .5) return false;
  for (const yaw of [a.yaw, b.yaw]) for (const axis of [yaw, yaw + Math.PI / 2]) {
    const x = Math.sin(axis), z = Math.cos(axis), dx = b.x - a.x, dz = b.z - a.z;
    const radius = (pose: CompanionDrivingPose, value: CompanionRoadBody) =>
      Math.abs(x * Math.sin(pose.yaw) + z * Math.cos(pose.yaw)) * value.halfLength
      + Math.abs(x * Math.cos(pose.yaw) - z * Math.sin(pose.yaw)) * value.halfWidth;
    if (Math.abs(dx * x + dz * z) > radius(a, body) + radius(b, other) + padding) return false;
  }
  return true;
}
function corridorValid(body: CompanionRoadBody, corridor: CompanionRoadCorridor, now: number) {
  if (!bodyValid(body) || !timeValid(now) || corridor.length < 2
    || corridor.length > companionRoadMechanicsRules.maxCorridorPoints || corridor[0].at !== now
    || corridor[corridor.length - 1].at - now > companionRoadMechanicsRules.maxCorridorMs) return false;
  const first = corridor[0].pose;
  if (!poseValid(first) || Math.hypot(first.x - body.pose.x, first.z - body.pose.z) > .05
    || Math.abs(angle(first.yaw - body.pose.yaw)) > .02 || Math.abs(first.y - body.pose.y) > .05) return false;
  return corridor.every((sample, index) => {
    if (!timeValid(sample.at) || !poseValid(sample.pose)) return false;
    if (index === 0) return true;
    const previous = corridor[index - 1], dt = (sample.at - previous.at) / 1000;
    return dt > 0 && dt <= .25 && Math.abs(sample.pose.y - previous.pose.y) <= .05
      && Math.hypot(sample.pose.x - previous.pose.x, sample.pose.z - previous.pose.z) <= 35 * dt
      && Math.abs(angle(sample.pose.yaw - previous.pose.yaw)) <= 1.5 * dt;
  });
}
/** Conservative sampled SAT with travel/turn padding covering motion between samples. */
export function companionRoadCorridorClear(body: CompanionRoadBody, corridor: CompanionRoadCorridor,
  bodies: readonly CompanionRoadBody[], now: number, fixedClear: CompanionRoadCorridorClear): boolean {
  if (!corridorValid(body, corridor, now) || bodies.length > companionRoadMechanicsRules.maxBodies
    || !bodies.every(bodyValid) || new Set(bodies.map(value => value.id)).size !== bodies.length
    || !fixedClear(body, corridor)) return false;
  for (let segment = 1; segment < corridor.length; segment++) {
    const from = corridor[segment - 1], to = corridor[segment], duration = (to.at - from.at) / 1000;
    const steps = Math.max(1, Math.ceil(duration / .05));
    const travel = Math.hypot(to.pose.x - from.pose.x, to.pose.z - from.pose.z) / steps;
    const turn = Math.abs(angle(to.pose.yaw - from.pose.yaw)) * Math.hypot(body.halfWidth, body.halfLength) / steps;
    for (let step = 0; step <= steps; step++) {
      const pose = interpolate(from.pose, to.pose, step / steps), at = from.at + (to.at - from.at) * step / steps;
      for (const other of bodies) {
        if (other.id === body.id) continue;
        const speed = other.held ? 0 : other.speed, time = (at - now) / 1000;
        const projected = { ...other.pose, x: other.pose.x + Math.sin(other.pose.yaw) * speed * time,
          z: other.pose.z + Math.cos(other.pose.yaw) * speed * time };
        const margin = companionRoadMechanicsRules.clearance + (travel + turn + Math.abs(speed) * duration / steps) / 2;
        if (overlaps(pose, body, projected, other, margin)) return false;
      }
    }
  }
  return true;
}
export function createCompanionRoadTrafficState(scope: CompanionRoadScope): CompanionRoadTrafficState {
  if (!validCompanionRoadScope(scope)) throw new Error("Use the accepted driving scope.");
  return Object.freeze({ scope: Object.freeze({ ...scope }), revision: 0, observedAt: 0, lastGapSequence: -1,
    gaps: Object.freeze([]), reactions: Object.freeze([]) });
}
function trafficState(state: CompanionRoadTrafficState, now: number) {
  return { ...state, observedAt: now, gaps: state.gaps.filter(gap => gap.cooldownUntil > now),
    reactions: state.reactions.filter(reaction => reaction.cooldownUntil > now) };
}
const trafficCopy = (state: CompanionRoadTrafficState): CompanionRoadTrafficState => Object.freeze({ ...state,
  revision: state.revision + 1, gaps: Object.freeze([...state.gaps]), reactions: Object.freeze([...state.reactions]) });
function gapPair(body: CompanionRoadBody, leading: CompanionRoadBody, trailing: CompanionRoadBody,
  corridor: CompanionRoadCorridor) {
  if (!bodyValid(leading) || !bodyValid(trailing) || corridor.length < 2
    || leading.id === trailing.id || leading.id === body.id || trailing.id === body.id
    || leading.kind !== "vehicle" || trailing.kind !== "vehicle" || trailing.owner !== "ambient" || trailing.held
    || Math.abs(angle(leading.pose.yaw - trailing.pose.yaw)) > .25) return false;
  const last = corridor[corridor.length - 1].pose, yaw = trailing.pose.yaw;
  const across = (pose: CompanionDrivingPose) => pose.x * Math.cos(yaw) - pose.z * Math.sin(yaw);
  if (Math.abs(across(leading.pose) - across(trailing.pose)) > .5
    || Math.abs(across(last) - across(trailing.pose)) > .5) return false;
  const station = (pose: CompanionDrivingPose) => pose.x * Math.sin(yaw) + pose.z * Math.cos(yaw);
  return station(leading.pose) - leading.halfLength - station(trailing.pose) - trailing.halfLength
    >= body.halfLength * 2 + companionRoadMechanicsRules.clearance * 2
    && station(last) > station(trailing.pose) + trailing.halfLength + body.halfLength
    && station(last) < station(leading.pose) - leading.halfLength - body.halfLength;
}
function acceptedBody(frame: CompanionRoadAcceptedFrame, body: CompanionRoadBody) {
  return bodyValid(body) && body.id === frame.scope.vehicleId && body.kind === "vehicle" && body.owner === "player" && !body.held
    && Math.hypot(body.pose.x - frame.pose.x, body.pose.z - frame.pose.z) <= .01
    && Math.abs(body.pose.y - frame.pose.y) <= .01 && Math.abs(angle(body.pose.yaw - frame.pose.yaw)) <= .01
    && Math.abs(body.speed - frame.speed) <= .01;
}
export function openCompanionRoadGap(state: CompanionRoadTrafficState, frame: CompanionRoadAcceptedFrame,
  body: CompanionRoadBody, leadingId: string, trailingId: string, corridor: CompanionRoadCorridor,
  bodies: readonly CompanionRoadBody[], fixedClear: CompanionRoadCorridorClear) {
  if (!frameLive(frame) || !sameCompanionRoadScope(state.scope, frame.scope) || !acceptedBody(frame, body)
    || frame.at < state.observedAt || frame.sequence <= state.lastGapSequence || !corridorValid(body, corridor, frame.at))
    return { state, gap: null } as const;
  const next = trafficState(state, frame.at), leading = bodies.find(value => value.id === leadingId),
    trailing = bodies.find(value => value.id === trailingId);
  if (!leading || !trailing || !gapPair(body, leading, trailing, corridor)
    || next.gaps.length >= companionRoadMechanicsRules.maxGaps
    || next.gaps.some(gap => gap.trailingId === trailingId)
    || next.reactions.some(reaction => reaction.vehicleId === trailingId)
    || !companionRoadCorridorClear(body, corridor, bodies, frame.at, fixedClear)) return { state, gap: null } as const;
  const gap: CompanionRoadGap = Object.freeze({ id: `${frame.scope.drivingId}:gap:${frame.sequence}:${trailingId}`,
    leadingId, trailingId, opensAt: frame.at, closesAt: frame.at + companionRoadMechanicsRules.gapMs,
    corridor: freezeCorridor(corridor), yieldedAt: null,
    cooldownUntil: frame.at + companionRoadMechanicsRules.gapMs + companionRoadMechanicsRules.reactionCooldownMs });
  return { state: trafficCopy({ ...next, lastGapSequence: frame.sequence, gaps: [...next.gaps, gap] }), gap } as const;
}
export function admitCompanionRoadMerge(state: CompanionRoadTrafficState, frame: CompanionRoadAcceptedFrame,
  gapId: string, body: CompanionRoadBody, corridor: CompanionRoadCorridor, bodies: readonly CompanionRoadBody[],
  fixedClear: CompanionRoadCorridorClear) {
  if (!frameLive(frame) || !sameCompanionRoadScope(state.scope, frame.scope) || !acceptedBody(frame, body)
    || frame.at < state.observedAt)
    return { state, reaction: null } as const;
  const next = trafficState(state, frame.at), gap = next.gaps.find(value => value.id === gapId),
    leading = bodies.find(value => value.id === gap?.leadingId), trailing = bodies.find(value => value.id === gap?.trailingId);
  if (!gap || !leading || !trailing || gap.yieldedAt !== null || frame.at < gap.opensAt || frame.at >= gap.closesAt
    || frame.throttle < companionRoadMechanicsRules.minimumThrottle || frame.throttle > companionRoadMechanicsRules.maximumThrottle
    || frame.brake > .2 || Math.abs(frame.steering) < .05 || frame.speed < 0
    || Math.abs(frame.speed - trailing.speed) > companionRoadMechanicsRules.matchingSpeed
    || Math.abs(angle(frame.pose.yaw - trailing.pose.yaw)) > .45
    || next.reactions.length >= companionRoadMechanicsRules.maxReactions
    || next.reactions.some(value => value.vehicleId === trailing.id)
    || !corridorValid(body, corridor, frame.at) || !gapPair(body, leading, trailing, corridor)
    || !companionRoadCorridorClear(body, corridor, bodies, frame.at, fixedClear)) return { state, reaction: null } as const;
  const openedTarget = gap.corridor[gap.corridor.length - 1].pose, target = corridor[corridor.length - 1].pose;
  if (Math.hypot(target.x - openedTarget.x, target.z - openedTarget.z) > 2)
    return { state, reaction: null } as const;
  const noseX = frame.pose.x + Math.sin(frame.pose.yaw) * body.halfLength,
    noseZ = frame.pose.z + Math.cos(frame.pose.yaw) * body.halfLength;
  const along = (noseX - trailing.pose.x) * Math.sin(trailing.pose.yaw) + (noseZ - trailing.pose.z) * Math.cos(trailing.pose.yaw);
  const across = Math.abs((noseX - trailing.pose.x) * Math.cos(trailing.pose.yaw) - (noseZ - trailing.pose.z) * Math.sin(trailing.pose.yaw));
  if (along < trailing.halfLength || across > trailing.halfWidth + body.halfWidth + .8)
    return { state, reaction: null } as const;
  const reaction: CompanionRoadReaction = Object.freeze({ id: `${gap.id}:yield`, vehicleId: trailing.id,
    causeId: gap.id, kind: "merge", startsAt: frame.at, endsAt: frame.at + companionRoadMechanicsRules.mergeYieldMs,
    cooldownUntil: frame.at + companionRoadMechanicsRules.reactionCooldownMs, lateral: 0,
    speedLimit: Math.max(0, Math.min(trailing.speed, frame.speed) - 1) });
  return { state: trafficCopy({ ...next, gaps: next.gaps.map(value => value.id === gap.id
    ? Object.freeze({ ...gap, yieldedAt: frame.at }) : value), reactions: [...next.reactions, reaction] }), reaction } as const;
}
/** The owning lane adapter supplies a collision-clear shoulder, not an arbitrary lateral teleport. */
export function admitCompanionRoadHornYield(state: CompanionRoadTrafficState, pulse: CompanionRoadHornPulse,
  body: CompanionRoadBody, corridor: CompanionRoadCorridor, lateral: number, bodies: readonly CompanionRoadBody[],
  now: number, fixedClear: CompanionRoadCorridorClear, boundaryGain: (from: ScenePoint, to: ScenePoint) => number) {
  if (!pulse || !bodyValid(body) || !sameCompanionRoadScope(state.scope, pulse.scope) || now < state.observedAt
    || body.owner !== "ambient" || body.kind !== "vehicle" || body.held
    || body.laneLateral === undefined
    || !Number.isFinite(lateral) || Math.abs(lateral) > 4 || Math.abs(lateral) < .2
    || companionRoadHornExposure(pulse, point(body.pose), now, boundaryGain) <= .05)
    return { state, reaction: null } as const;
  const next = trafficState(state, now);
  if (next.reactions.length >= companionRoadMechanicsRules.maxReactions
    || next.gaps.some(gap => gap.trailingId === body.id)
    || next.reactions.some(reaction => reaction.vehicleId === body.id)
    || !companionRoadCorridorClear(body, corridor, bodies, now, fixedClear)) return { state, reaction: null } as const;
  const target = corridor[corridor.length - 1], dx = target.pose.x - body.pose.x, dz = target.pose.z - body.pose.z;
  const across = dx * Math.cos(body.pose.yaw) - dz * Math.sin(body.pose.yaw);
  const forward = dx * Math.sin(body.pose.yaw) + dz * Math.cos(body.pose.yaw);
  if (target.at > now + companionRoadMechanicsRules.hornYieldMs || forward < 0
    || Math.abs(angle(target.pose.yaw - body.pose.yaw)) > .15
    || Math.abs(across - (lateral - body.laneLateral)) > .1) return { state, reaction: null } as const;
  const reaction: CompanionRoadReaction = Object.freeze({ id: `${pulse.id}:yield:${body.id}`, vehicleId: body.id,
    causeId: pulse.id, kind: "horn", startsAt: now, endsAt: now + companionRoadMechanicsRules.hornYieldMs,
    cooldownUntil: now + companionRoadMechanicsRules.reactionCooldownMs, lateral,
    speedLimit: Math.max(0, Math.min(body.speed, 1.4)) });
  return { state: trafficCopy({ ...next, reactions: [...next.reactions, reaction] }), reaction } as const;
}
