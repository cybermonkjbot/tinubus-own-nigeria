import type { CompanionDrivingHandling, CompanionDrivingSurface } from "./companionPlayerDriving";
import { sameCompanionRoadScope, validCompanionRoadFrame, validCompanionRoadScope,
  type CompanionRoadAcceptedFrame, type CompanionRoadScope } from "./companionRoadMechanics";

/** Noncash game tuning. Heat is normalized, not a Celsius reading. */
export const companionVehicleStrainRules = Object.freeze({
  version: "lagos-vehicle-strain-v1", maxStepMs: 100, acceptedAgeMs: 250, receiptLimit: 16,
  warning: 55, derate: 75, stall: 92, restart: 68,
  middayIdlePerSecond: .45, runningPerSecond: .08, airflowPerSecond: .08,
  engineOffCoolingPerSecond: .2, serviceCoolingPerSecond: 1.2,
  maxBrakeFade: .18, brakeFadePerSecond: .12, brakeRecoveryPerSecond: .06,
  maxAlignment: .035, minimumCondition: 25, maxRoadLoss: 12,
  maxImpactLoss: 2, impactCooldownMs: 500, minimumImpactSpeed: 2,
  serviceRecoveryPerSecond: .1,
});
export type CompanionVehicleStrain = Readonly<{
  vehicleId: string; revision: number; scope: CompanionRoadScope | null;
  lastSequence: number; observedAt: number; temperature: number; stalled: boolean;
  brakeFade: number; alignment: number; roadConditionLoss: number; wear: number;
  serviceRecovery: number; lastImpactAt: number | null; lastContact: string | null;
  receipts: readonly string[];
}>;
export type CompanionRoadWheelImpact = Readonly<{
  id: string; scope: CompanionRoadScope; sequence: number; at: number;
  contactId: string; speed: number; strength: number; side: -1 | 0 | 1;
}>;
export type CompanionRoadWheelSample = Readonly<{ surface: CompanionDrivingSurface; contactId: string | null }>;
const roadGrip: Record<CompanionDrivingSurface, { longitudinal: number; lateral: number; resistance: number }> = {
  clean_asphalt: { longitudinal: .85, lateral: .85, resistance: .035 },
  laterite_mud: { longitudinal: .4, lateral: .4, resistance: .28 },
  pothole: { longitudinal: .85, lateral: .85, resistance: .09 },
  gutter_wood: { longitudinal: .7, lateral: .35, resistance: .07 },
};
export type CompanionRoadCoolingService = Readonly<{
  id: string; scope: CompanionRoadScope; at: number; expiresAt: number;
  kind: "mechanic" | "water-seller"; reachable: boolean; pulledOver: boolean;
}>;
export type CompanionVehicleStrainFrame = Readonly<{
  accepted: CompanionRoadAcceptedFrame;
  /** Accepted active elapsed time. Never derive this from a background/offline wall-clock gap. */
  activeMs: number; middayHeat: boolean; surface: CompanionDrivingSurface;
  wheelContactId: string | null; impact: CompanionRoadWheelImpact | null;
  service: CompanionRoadCoolingService | null; condition: number;
}>;
export type CompanionVehicleStrainProjection = Readonly<{
  temperature: number; stage: "normal" | "warm" | "limited" | "stalled";
  warning: string | null; brakeFade: number; alignment: number; roadConditionLoss: number;
  handling: CompanionDrivingHandling;
}>;
const clamp = (value: number, low: number, high: number) => Math.min(high, Math.max(low, value));
const timeValid = (at: number) => Number.isSafeInteger(at) && at >= 0;
const idValid = (id: string) => typeof id === "string" && id.length > 0 && id.length <= 240 && id.trim() === id;
const surfaces = new Set<CompanionDrivingSurface>(["clean_asphalt", "laterite_mud", "pothole", "gutter_wood"]);
/** Four accepted wheel positions, shared by owner-side consequences and renderer suspension. */
export function sampleCompanionRoadWheels(frame: CompanionRoadAcceptedFrame, dimensions: Readonly<{
  wheelbase: number; halfWidth: number;
}>, sample: (point: Readonly<{ x: number; y: number; z: number }>) => CompanionRoadWheelSample) {
  if (!validCompanionRoadFrame(frame) || !Number.isFinite(dimensions.wheelbase) || dimensions.wheelbase <= 0
    || dimensions.wheelbase > 8 || !Number.isFinite(dimensions.halfWidth) || dimensions.halfWidth <= 0 || dimensions.halfWidth > 3)
    throw new Error("Use the accepted chassis and wheel dimensions.");
  let surface: CompanionDrivingSurface = "clean_asphalt", longitudinal = .85, lateral = .85, resistance = .035;
  let contactId: string | null = null, side: -1 | 0 | 1 = 0;
  const c = Math.cos(frame.pose.yaw), s = Math.sin(frame.pose.yaw);
  for (const wheelSide of [-1, 1]) for (const axle of [-1, 1]) {
    const x = wheelSide * dimensions.halfWidth * .75, z = axle * dimensions.wheelbase * .5;
    const wheel = sample({ x: frame.pose.x + x * c + z * s, y: frame.pose.y,
      z: frame.pose.z - x * s + z * c });
    if (!wheel || !surfaces.has(wheel.surface) || wheel.contactId !== null && !idValid(wheel.contactId))
      throw new Error("Use the current road collider or published patch sample.");
    const grip = roadGrip[wheel.surface];
    longitudinal = Math.min(longitudinal, grip.longitudinal); lateral = Math.min(lateral, grip.lateral);
    resistance = Math.max(resistance, grip.resistance);
    if (wheel.surface === "laterite_mud" || surface !== "laterite_mud" && wheel.surface === "gutter_wood"
      || surface === "clean_asphalt" && wheel.surface === "pothole") surface = wheel.surface;
    if (wheel.surface === "pothole" && wheel.contactId !== null) {
      if (contactId === null || wheel.contactId < contactId) {
        contactId = wheel.contactId; side = wheelSide < 0 ? -1 : 1;
      } else if (wheel.contactId === contactId && side !== (wheelSide < 0 ? -1 : 1)) side = 0;
    }
  }
  const impact: CompanionRoadWheelImpact | null = contactId && frame.active && !frame.externalHold && Math.abs(frame.speed) >= 2
    ? Object.freeze({ id: `${frame.scope.drivingId}:wheel:${frame.sequence}`, scope: frame.scope,
      sequence: frame.sequence, at: frame.at, contactId, speed: Math.abs(frame.speed),
      strength: Math.min(1, Math.abs(frame.speed) / 14), side }) : null;
  return Object.freeze({ surface, longitudinalGrip: longitudinal, lateralGrip: lateral, resistance, contactId, impact });
}
export function createCompanionVehicleStrain(vehicleId: string, now: number): CompanionVehicleStrain {
  if (!idValid(vehicleId) || !timeValid(now)) throw new Error("Use the saved vehicle and current owner time.");
  return Object.freeze({ vehicleId, revision: 0, scope: null, lastSequence: -1, observedAt: now,
    temperature: 0, stalled: false, brakeFade: 0, alignment: 0, roadConditionLoss: 0, wear: 0,
    serviceRecovery: 0, lastImpactAt: null, lastContact: null, receipts: Object.freeze([]) });
}
export function validCompanionVehicleStrain(state: CompanionVehicleStrain): boolean {
  return !!state && idValid(state.vehicleId) && Number.isSafeInteger(state.revision) && state.revision >= 0
    && (state.scope === null || validCompanionRoadScope(state.scope) && state.scope.vehicleId === state.vehicleId)
    && Number.isSafeInteger(state.lastSequence) && state.lastSequence >= -1 && timeValid(state.observedAt)
    && Number.isFinite(state.temperature) && state.temperature >= 0 && state.temperature <= 100
    && typeof state.stalled === "boolean" && Number.isFinite(state.brakeFade) && state.brakeFade >= 0
    && state.brakeFade <= companionVehicleStrainRules.maxBrakeFade
    && Number.isFinite(state.alignment) && Math.abs(state.alignment) <= companionVehicleStrainRules.maxAlignment
    && Number.isInteger(state.roadConditionLoss) && state.roadConditionLoss >= 0
    && state.roadConditionLoss <= companionVehicleStrainRules.maxRoadLoss
    && Number.isFinite(state.wear) && state.wear >= 0 && state.wear < 1
    && Number.isFinite(state.serviceRecovery) && state.serviceRecovery >= 0 && state.serviceRecovery < 1
    && (state.lastImpactAt === null || timeValid(state.lastImpactAt))
    && (state.lastContact === null || idValid(state.lastContact))
    && Array.isArray(state.receipts) && state.receipts.length <= companionVehicleStrainRules.receiptLimit
    && state.receipts.every(idValid) && new Set(state.receipts).size === state.receipts.length;
}
/** Rebind only after accepted occupancy. Heat, alignment and road loss survive exit, life change and reconnect. */
export function bindCompanionVehicleStrain(state: CompanionVehicleStrain, scope: CompanionRoadScope,
  sequence: number, now: number): CompanionVehicleStrain {
  if (!validCompanionVehicleStrain(state) || !validCompanionRoadScope(scope) || scope.vehicleId !== state.vehicleId
    || !Number.isSafeInteger(sequence) || sequence < 0 || !timeValid(now) || now < state.observedAt)
    throw new Error("Use the accepted vehicle occupancy and strain state.");
  if (state.scope && sameCompanionRoadScope(state.scope, scope)) return state;
  return Object.freeze({ ...state, revision: state.revision + 1, scope: Object.freeze({ ...scope }),
    lastSequence: sequence, observedAt: now });
}
export function projectCompanionVehicleStrain(state: CompanionVehicleStrain, surface: CompanionDrivingSurface,
  baseSpeedLimit: number): CompanionVehicleStrainProjection {
  if (!validCompanionVehicleStrain(state) || !surfaces.has(surface) || !Number.isFinite(baseSpeedLimit) || baseSpeedLimit <= 0)
    throw new Error("Use the current vehicle strain and supported road surface.");
  const rules = companionVehicleStrainRules;
  const stage = state.stalled ? "stalled" : state.temperature >= rules.derate ? "limited"
    : state.temperature >= rules.warning ? "warm" : "normal";
  const scale = stage === "stalled" ? 0 : stage === "limited"
    ? clamp(.95 - (state.temperature - rules.derate) / (rules.stall - rules.derate) * .4, .55, .95) : 1;
  return Object.freeze({ temperature: state.temperature, stage,
    warning: stage === "stalled" ? "Engine overheated. Wait for it to cool."
      : stage === "limited" ? "Engine hot. Power is reduced."
      : stage === "warm" ? "Engine warming. Keep airflow or pull over." : null,
    brakeFade: state.brakeFade, alignment: state.alignment, roadConditionLoss: state.roadConditionLoss,
    handling: Object.freeze({ accelerationScale: scale, brakingScale: 1 - state.brakeFade,
      speedLimit: baseSpeedLimit * scale, steeringBias: state.alignment, surface }) });
}
/** Combine current wheel grip with saved heat/fade exactly once before the driving owner integrates. */
export function companionRoadHandling(state: CompanionVehicleStrain,
  wheels: Readonly<{ surface: CompanionDrivingSurface; longitudinalGrip: number }>, baseSpeedLimit: number): CompanionDrivingHandling {
  if (!Number.isFinite(wheels.longitudinalGrip) || wheels.longitudinalGrip < .35 || wheels.longitudinalGrip > .85)
    throw new Error("Use the current accepted wheel grip.");
  const projection = projectCompanionVehicleStrain(state, wheels.surface, baseSpeedLimit), grip = wheels.longitudinalGrip / .85;
  return Object.freeze({ ...projection.handling, accelerationScale: projection.handling.accelerationScale * grip,
    brakingScale: Math.max(.5, projection.handling.brakingScale * grip) });
}
function serviceLive(service: CompanionRoadCoolingService | null, frame: CompanionRoadAcceptedFrame) {
  return !!service && idValid(service.id) && sameCompanionRoadScope(service.scope, frame.scope)
    && ["mechanic", "water-seller"].includes(service.kind) && service.reachable && service.pulledOver
    && timeValid(service.at) && timeValid(service.expiresAt) && service.at <= frame.at && service.expiresAt > frame.at
    && service.expiresAt - service.at <= 30_000 && Math.abs(frame.speed) <= .15;
}
function impactLive(impact: CompanionRoadWheelImpact | null, frame: CompanionVehicleStrainFrame) {
  const accepted = frame.accepted;
  return !!impact && idValid(impact.id) && idValid(impact.contactId) && sameCompanionRoadScope(impact.scope, accepted.scope)
    && impact.sequence === accepted.sequence && impact.at === accepted.at && impact.contactId === frame.wheelContactId
    && Number.isFinite(impact.speed) && impact.speed >= companionVehicleStrainRules.minimumImpactSpeed
    && impact.speed <= 35 && Math.abs(impact.speed - Math.abs(accepted.speed)) <= .5
    && Number.isFinite(impact.strength) && impact.strength > 0 && impact.strength <= 1 && [-1, 0, 1].includes(impact.side);
}
/** The owner persists this result with the driving receipt and condition delta in one transaction. */
export function advanceCompanionVehicleStrain(state: CompanionVehicleStrain, frame: CompanionVehicleStrainFrame) {
  const accepted = frame.accepted, rules = companionVehicleStrainRules;
  const unchanged = () => ({ state, conditionDelta: 0, accepted: false } as const);
  if (!validCompanionVehicleStrain(state) || !validCompanionRoadFrame(accepted) || !state.scope
    || !sameCompanionRoadScope(state.scope, accepted.scope) || accepted.sequence <= state.lastSequence
    || accepted.at < state.observedAt || !Number.isInteger(frame.activeMs) || frame.activeMs < 0 || frame.activeMs > rules.maxStepMs
    || !surfaces.has(frame.surface) || !Number.isInteger(frame.condition) || frame.condition < 0 || frame.condition > 100
    || typeof frame.middayHeat !== "boolean" || frame.wheelContactId !== null && !idValid(frame.wheelContactId)) return unchanged();
  const next = { ...state, revision: state.revision + 1, lastSequence: accepted.sequence, observedAt: accepted.at };
  // A late receipt consumes no missed time. Hidden, modal, custody and crash holds freeze strain.
  const active = accepted.active && !accepted.externalHold && accepted.at - state.observedAt <= rules.acceptedAgeMs
    && frame.activeMs <= accepted.at - state.observedAt;
  if (!active) return { state: Object.freeze(next), conditionDelta: 0, accepted: true } as const;
  const dt = frame.activeMs / 1000, speed = Math.abs(accepted.speed), service = serviceLive(frame.service, accepted);
  const running = accepted.engineRunning && !state.stalled;
  let thermalRate = running ? frame.middayHeat && speed < 1 ? rules.middayIdlePerSecond : rules.runningPerSecond
    : -rules.engineOffCoolingPerSecond;
  // Forward flow cools; reversing around a stalled queue does not create free airflow.
  thermalRate -= Math.max(0, accepted.speed - 1.5) * rules.airflowPerSecond;
  if (service) thermalRate -= rules.serviceCoolingPerSecond;
  next.temperature = clamp(state.temperature + thermalRate * dt, 0, 100);
  next.stalled = state.stalled ? next.temperature >= rules.restart : next.temperature >= rules.stall;
  next.brakeFade = clamp(state.brakeFade + (running && accepted.brake > .2 && speed >= 5
    ? rules.brakeFadePerSecond * accepted.brake : -rules.brakeRecoveryPerSecond) * dt, 0, rules.maxBrakeFade);
  let conditionDelta = 0;
  const contactChanged = frame.wheelContactId !== null && frame.wheelContactId !== state.lastContact;
  if (frame.activeMs > 0 && contactChanged && impactLive(frame.impact, frame) && !state.receipts.includes(frame.impact!.id)
    && (state.lastImpactAt === null || accepted.at - state.lastImpactAt >= rules.impactCooldownMs)) {
    const impact = frame.impact!;
    next.lastImpactAt = accepted.at;
    next.receipts = Object.freeze([...state.receipts, impact.id].slice(-rules.receiptLimit));
    const accumulated = state.wear + Math.min(rules.maxImpactLoss, impact.strength * speed / 8);
    const loss = Math.min(Math.floor(accumulated), rules.maxRoadLoss - state.roadConditionLoss,
      Math.max(0, frame.condition - rules.minimumCondition));
    conditionDelta = -loss; next.roadConditionLoss += loss;
    next.wear = accumulated - Math.floor(accumulated);
    next.alignment = clamp(state.alignment + impact.side * impact.strength * .008, -rules.maxAlignment, rules.maxAlignment);
  }
  next.lastContact = frame.wheelContactId;
  // Recovery restores only loss attributed to this adapter. External crash damage remains owned elsewhere.
  if (service && dt > 0) {
    next.alignment = state.alignment * Math.max(0, 1 - dt * .12);
    next.brakeFade = Math.max(0, next.brakeFade - dt * .1);
    const accumulated = state.serviceRecovery + rules.serviceRecoveryPerSecond * dt;
    const restore = Math.min(Math.floor(accumulated), next.roadConditionLoss, Math.max(0, 100 - frame.condition));
    conditionDelta += restore; next.roadConditionLoss -= restore;
    next.serviceRecovery = next.roadConditionLoss > 0 ? accumulated - Math.floor(accumulated) : 0;
  } else next.serviceRecovery = 0;
  return { state: Object.freeze(next), conditionDelta, accepted: true } as const;
}
