import { companionStreetKey, type CompanionStreetFactionScope, type CompanionStreetWitness } from "./companionStreetFactions";
import { sameCompanionRoadScope, validCompanionRoadFrame, type CompanionRoadAcceptedFrame, type CompanionRoadScope } from "./companionRoadMechanics";
import { companionMovementEndsAt, sampleCompanionMovement, type CompanionMovement } from "./companionMovement";
import { planRoomRoute, routeSegmentClear, type RoomNavigation, type ScenePoint } from "./companionNavigation";
import { companionPlaceExpansionDestinations } from "./companionPlacesExpansion";
import type { CompanionDrivingPose, CompanionPlayerDrivingState } from "./companionPlayerDriving";
import type { CompanionVehicleKind } from "./companionVehicles";

/** Real-time, noncash tuning. No forced fine, banked debit or animation-owned damage. */
export const companionVehicleStopRules = Object.freeze({ terminalReach: 5, stopSpeed: 2, levyKeke: 6, levyDanfo: 10,
  approachMs: 20_000, telegraphMs: 900, responseMs: 1_250, assistedResponseMs: 8_000, reviewMs: 8_000,
  threatMs: 12_000, contactReach: .9, panelDamage: 2, panelMaximum: 8, contactCooldownMs: 1_500,
  escapeReach: 8, escapeBreakMs: 1_500, maximumContacts: 4, maximumStops: 8, maximumTerminals: 8,
  observationAgeMs: 250, observationGapMs: 500, maximumApproachPoints: 32 });
export type CompanionVehicleStopScope = CompanionStreetFactionScope & CompanionRoadScope & Readonly<{
  gameRoomId: string; sharedStartedAt: number;
}>;
export type CompanionStreetActorApproach = Readonly<{
  actorId: string; reservationId: string; scope: CompanionRoadScope; movement: CompanionMovement;
  acceptedAt: number; navigationRevision: string; actorRadius: number;
}>;
export type CompanionVehicleStopPhase = "approaching" | "levy" | "threat" | "response" | "review" | "released" | "escaped" | "interrupted";
export type CompanionVehicleStop = Readonly<{
  id: string; scope: CompanionVehicleStopScope; kind: "union" | "lastma"; revision: number; phase: CompanionVehicleStopPhase;
  evidenceId: string; actor: CompanionStreetActorApproach; createdAt: number; expiresAt: number; updatedAt: number;
  terminal: Readonly<{ terminalId: string; visitSequence: number; payerUserId: string; amount: number; unitKey: "lagos-credit" }> | null;
  violation: "prohibited-direction" | "dedicated-lane" | null; responseMode: "timed" | "assisted";
  telegraphAt: number | null; responseOpensAt: number | null; responseClosesAt: number | null;
  response: Readonly<{ requestId: string; sequence: number; at: number; result: "guarded" | "cooperated" | "missed" }> | null;
  ignitionInterruptedAt: number | null; reviewEndsAt: number | null; paymentReceiptId: string | null;
  contactIds: readonly string[]; lastContactAt: number | null; panelDamageRequested: number;
  lastObservationAt: number | null; escapeBreakAt: number | null; lastInputSequence: number; lastRequestId: string | null;
}>;
export type CompanionVehicleStopState = Readonly<{
  version: 1; scope: CompanionVehicleStopScope; revision: number;
  stops: readonly CompanionVehicleStop[]; terminalVisits: Readonly<Record<string, number>>;
  lastViolationSequence: number;
}>;
export type CompanionTerminalNode = Readonly<{ id: string; navigationRevision: string; point: ScenePoint;
  kinds: readonly ("keke" | "danfo")[]; exempt: boolean }>;
/** Existing named terminal only. Ordinary bus stops do not become chargeable terminals. */
export function companionStreetTerminalNodes(navigation: RoomNavigation): readonly CompanionTerminalNode[] {
  return companionPlaceExpansionDestinations.filter(place => place.kind === "bus-terminal")
    .filter(place => routeSegmentClear(navigation, place.city.entrance, place.city.entrance))
    .map(place => ({ id: place.id, navigationRevision: navigation.revision, point: place.city.entrance,
      kinds: ["keke", "danfo"] as const, exempt: false }));
}
export type CompanionTerminalLevyConfirmation = Readonly<{ stopId: string; scope: CompanionVehicleStopScope;
  revision: number; terminalId: string; visitSequence: number; payerUserId: string;
  amount: number; unitKey: "lagos-credit"; expiresAt: number }>;
export type CompanionVehicleStopInput = Readonly<{ stopId: string; revision: number; requestId: string;
  sequence: number; action: "run-stop" | "decline-levy" | "guard-key" | "cooperate" | "review" | "recover" }>;
export type CompanionVehiclePanelDamageRequest = Readonly<{ eventKey: string; stopId: string; vehicleId: string;
  actorId: string; contactId: string; amount: number; at: number }>;
export type CompanionVehicleStopObservation = Readonly<{ frame: CompanionRoadAcceptedFrame;
  actorReservationId: string; actorPoint: ScenePoint; actorAt: number;
  /** True only while the driver can answer, including the dedicated stop sheet. Other overlays/focus loss revoke it. */
  responseEligible: boolean;
  /** Existing collision/combat owner issues actual contact. Scene animation never supplies it. */
  contact: Readonly<{ id: string; actorId: string; vehicleId: string; at: number }> | null }>;
const integer = (value: number) => Number.isSafeInteger(value) && value >= 0 && value < Number.MAX_SAFE_INTEGER - 1;
// Navigation revisions can include the frozen layout. Compare them in full, not as short IDs.
const navigationRevisionValid = (value: unknown): value is string => typeof value === "string" && value.length > 0 && value.trim() === value;
const pointValid = (point: ScenePoint) => Array.isArray(point) && point.length === 3 && point.every(Number.isFinite);
const posePoint = (pose: CompanionDrivingPose): ScenePoint => [pose.x, pose.y, pose.z];
const terminalPhase = (phase: CompanionVehicleStopPhase) => ["released", "escaped", "interrupted"].includes(phase);
export const sameCompanionVehicleStopScope = (a: CompanionVehicleStopScope, b: CompanionVehicleStopScope) =>
  sameCompanionRoadScope(a, b) && a.worldKey === b.worldKey && a.epochKey === b.epochKey && a.userId === b.userId
    && a.gameRoomId === b.gameRoomId && a.sharedStartedAt === b.sharedStartedAt;
export function validCompanionVehicleStopScope(scope: CompanionVehicleStopScope) {
  return !!scope && [scope.worldKey, scope.epochKey, scope.userId, scope.instanceId,
    scope.drivingId, scope.vehicleId, scope.lifeId, scope.gameRoomId].every(companionStreetKey)
    && navigationRevisionValid(scope.layoutRevision) && integer(scope.sharedStartedAt);
}
export function companionVehicleStopScopeFromDriving(driving: CompanionPlayerDrivingState,
  world: Readonly<{ worldKey: string; epochKey: string; sharedStartedAt: number }>): CompanionVehicleStopScope {
  return { ...world, userId: driving.driver.userId, lifeId: driving.driver.lifeId, gameRoomId: driving.gameRoomId,
    instanceId: driving.instanceId, layoutRevision: driving.navigationRevision, drivingId: driving.id, vehicleId: driving.vehicleId };
}
export function createCompanionVehicleStopState(scope: CompanionVehicleStopScope): CompanionVehicleStopState {
  if (!validCompanionVehicleStopScope(scope)) throw new Error("Use the current driver and frozen street scope.");
  return { version: 1, scope: { ...scope }, revision: 0, stops: [], terminalVisits: {}, lastViolationSequence: -1 };
}
function movementValid(actor: CompanionStreetActorApproach) {
  const movement = actor?.movement;
  return !!actor && companionStreetKey(actor.actorId) && companionStreetKey(actor.reservationId) && integer(actor.acceptedAt)
    && Number.isFinite(actor.actorRadius) && actor.actorRadius >= .2 && actor.actorRadius <= .5
    && !!movement && movement.version === 1 && integer(movement.sequence) && integer(movement.roomRevision)
    && movement.startedAt === actor.acceptedAt && [movement.from?.x, movement.from?.z, movement.from?.yaw, movement.yaw].every(Number.isFinite)
    && (movement.initialSpeed === undefined || Number.isFinite(movement.initialSpeed) && movement.initialSpeed >= 0 && movement.initialSpeed <= .65)
    && Array.isArray(movement.path) && movement.path.length > 0 && movement.path.length <= companionVehicleStopRules.maximumApproachPoints
    && movement.path.every(point => [point.x, point.z].every(Number.isFinite));
}
/** Validate the actor's full admitted route against current dynamic geometry before contact. */
export function companionStreetApproachPoint(actor: CompanionStreetActorApproach, navigation: RoomNavigation, now: number): ScenePoint | null {
  if (!movementValid(actor) || !integer(now) || now < actor.acceptedAt || navigation.revision !== actor.navigationRevision
    || actor.scope.layoutRevision !== navigation.revision) return null;
  const nav = { ...navigation, radius: actor.actorRadius };
  let from: ScenePoint = [actor.movement.from.x, navigation.floorY, actor.movement.from.z];
  if (!routeSegmentClear(nav, from, from) || !actor.movement.path.every(value => {
    const to: ScenePoint = [value.x, navigation.floorY, value.z];
    const clear = routeSegmentClear(nav, from, to); from = to; return clear;
  })) return null;
  const sample = sampleCompanionMovement(actor.movement, now);
  return [sample.position.x, navigation.floorY, sample.position.z];
}
/** Driver-window point uses the driving owner's actual chassis, including keke dimensions. */
export function companionVehicleDriverWindow(pose: CompanionDrivingPose, halfWidth: number, halfLength: number): ScenePoint | null {
  if (!pointValid(posePoint(pose)) || !Number.isFinite(pose.yaw) || !Number.isFinite(halfWidth) || halfWidth < .4 || halfWidth > 2
    || !Number.isFinite(halfLength) || halfLength < .8 || halfLength > 4) return null;
  const side = halfWidth + .4, forward = halfLength * .25;
  return [pose.x - Math.cos(pose.yaw) * side + Math.sin(pose.yaw) * forward, pose.y,
    pose.z + Math.sin(pose.yaw) * side + Math.cos(pose.yaw) * forward];
}
/** Plan from a real free body. The world/NPC owner must reserve and present this same route. */
export function planCompanionStreetActorApproach(args: { actorId: string; reservationId: string; scope: CompanionRoadScope;
  actorPoint: ScenePoint; actorYaw: number; pose: CompanionDrivingPose; halfWidth: number; halfLength: number;
  navigation: RoomNavigation; now: number; sequence: number; roomRevision: number }): CompanionStreetActorApproach | null {
  const window = companionVehicleDriverWindow(args.pose, args.halfWidth, args.halfLength);
  if (!window || !pointValid(args.actorPoint) || !Number.isFinite(args.actorYaw) || !integer(args.now)
    || !integer(args.sequence) || !integer(args.roomRevision) || !companionStreetKey(args.actorId) || !companionStreetKey(args.reservationId)
    || args.scope.layoutRevision !== args.navigation.revision) return null;
  const route = planRoomRoute({ ...args.navigation, radius: .3 }, args.actorPoint, window);
  if (!route || route.length === 0 || route.length > companionVehicleStopRules.maximumApproachPoints) return null;
  const actor: CompanionStreetActorApproach = { actorId: args.actorId, reservationId: args.reservationId, scope: args.scope,
    acceptedAt: args.now, navigationRevision: args.navigation.revision, actorRadius: .3,
    movement: { version: 1, sequence: args.sequence, roomRevision: args.roomRevision, startedAt: args.now,
      from: { x: args.actorPoint[0], z: args.actorPoint[2], yaw: args.actorYaw }, initialSpeed: 0,
      path: route.map(point => ({ x: point[0], z: point[2] })), yaw: Math.atan2(args.pose.x - window[0], args.pose.z - window[2]) } };
  return companionMovementEndsAt(actor.movement) <= args.now + companionVehicleStopRules.approachMs ? actor : null;
}
function stateValid(state: CompanionVehicleStopState) {
  return !!state && state.version === 1 && validCompanionVehicleStopScope(state.scope) && integer(state.revision)
    && Array.isArray(state.stops) && state.stops.length <= companionVehicleStopRules.maximumStops
    && !!state.terminalVisits && typeof state.terminalVisits === "object" && !Array.isArray(state.terminalVisits)
    && Object.keys(state.terminalVisits).length <= companionVehicleStopRules.maximumTerminals
    && Object.entries(state.terminalVisits).every(([id, sequence]) => companionStreetKey(id) && integer(sequence))
    && Number.isSafeInteger(state.lastViolationSequence) && state.lastViolationSequence >= -1
    && state.stops.every(stop => validCompanionVehicleStop(stop) && sameCompanionVehicleStopScope(state.scope, stop.scope))
    && new Set(state.stops.map(stop => stop.id)).size === state.stops.length
    && state.stops.filter(stop => !terminalPhase(stop.phase)).length <= 1;
}
export const validCompanionVehicleStopState = stateValid;
export function validCompanionVehicleStop(stop: CompanionVehicleStop): boolean {
  return !!stop && companionStreetKey(stop.id) && validCompanionVehicleStopScope(stop.scope) && integer(stop.revision)
    && ["union", "lastma"].includes(stop.kind) && ["approaching", "levy", "threat", "response", "review", "released", "escaped", "interrupted"].includes(stop.phase)
    && companionStreetKey(stop.evidenceId) && movementValid(stop.actor) && sameCompanionRoadScope(stop.scope, stop.actor.scope)
    && stop.actor.navigationRevision === stop.scope.layoutRevision && [stop.createdAt, stop.expiresAt, stop.updatedAt].every(integer)
    && stop.createdAt === stop.actor.acceptedAt && stop.expiresAt > stop.createdAt && stop.updatedAt >= stop.createdAt
    && ["timed", "assisted"].includes(stop.responseMode)
    && (stop.kind === "union" ? !["response", "review"].includes(stop.phase) : !["levy", "threat"].includes(stop.phase))
    && (stop.kind === "union" ? !!stop.terminal && stop.violation === null && companionStreetKey(stop.terminal.terminalId)
      && integer(stop.terminal.visitSequence) && stop.terminal.payerUserId === stop.scope.userId && stop.terminal.unitKey === "lagos-credit"
      && (stop.terminal.amount === companionVehicleStopRules.levyKeke || stop.terminal.amount === companionVehicleStopRules.levyDanfo)
      : stop.terminal === null && ["prohibited-direction", "dedicated-lane"].includes(stop.violation ?? ""))
    && [stop.telegraphAt, stop.responseOpensAt, stop.responseClosesAt, stop.ignitionInterruptedAt, stop.reviewEndsAt,
      stop.lastContactAt, stop.lastObservationAt, stop.escapeBreakAt].every(value => value === null || integer(value) && value >= stop.createdAt)
    && (stop.lastObservationAt === null || stop.lastObservationAt <= stop.updatedAt)
    && (stop.lastContactAt === null || stop.lastContactAt <= stop.updatedAt)
    && (stop.response === null || stop.kind === "lastma" && companionStreetKey(stop.response.requestId)
      && integer(stop.response.sequence) && integer(stop.response.at) && stop.response.at >= stop.createdAt && stop.response.at <= stop.updatedAt
      && ["guarded", "cooperated", "missed"].includes(stop.response.result))
    && (stop.paymentReceiptId === null || companionStreetKey(stop.paymentReceiptId)) && Array.isArray(stop.contactIds)
    && stop.contactIds.length <= companionVehicleStopRules.maximumContacts && stop.contactIds.every(companionStreetKey)
    && new Set(stop.contactIds).size === stop.contactIds.length && integer(stop.panelDamageRequested)
    && stop.panelDamageRequested === stop.contactIds.length * companionVehicleStopRules.panelDamage
    && stop.panelDamageRequested <= companionVehicleStopRules.panelMaximum
    && (stop.lastContactAt === null) === (stop.contactIds.length === 0) && Number.isSafeInteger(stop.lastInputSequence)
    && stop.lastInputSequence >= -1 && (stop.lastRequestId === null || companionStreetKey(stop.lastRequestId));
}
function replace(state: CompanionVehicleStopState, stop: CompanionVehicleStop): CompanionVehicleStopState {
  return { ...state, revision: state.revision + 1, stops: state.stops.map(value => value.id === stop.id ? stop : value) };
}
function baseStop(state: CompanionVehicleStopState, id: string, evidenceId: string, actor: CompanionStreetActorApproach,
  now: number, kind: "union" | "lastma", responseMode: "timed" | "assisted"): CompanionVehicleStop {
  return { id, scope: { ...state.scope }, kind, revision: 0, phase: "approaching", evidenceId, actor, createdAt: now,
    expiresAt: now + companionVehicleStopRules.approachMs, updatedAt: now, terminal: null, violation: null, responseMode,
    telegraphAt: null, responseOpensAt: null, responseClosesAt: null, response: null, ignitionInterruptedAt: null,
    reviewEndsAt: null, paymentReceiptId: null, contactIds: [], lastContactAt: null, panelDamageRequested: 0,
    lastObservationAt: null, escapeBreakAt: null, lastInputSequence: -1, lastRequestId: null };
}
function admissionReady(state: CompanionVehicleStopState, frame: CompanionRoadAcceptedFrame, actor: CompanionStreetActorApproach,
  navigation: RoomNavigation, now: number, window: ScenePoint | null) {
  if (!stateValid(state) || !validCompanionRoadFrame(frame) || !sameCompanionRoadScope(state.scope, frame.scope)
    || !frame.active || frame.externalHold || !integer(now) || now < frame.at || now - frame.at > companionVehicleStopRules.observationAgeMs
    || state.stops.some(stop => !terminalPhase(stop.phase)) || !window || !movementValid(actor)
    || !sameCompanionRoadScope(frame.scope, actor.scope) || actor.acceptedAt !== now
    || !companionStreetApproachPoint(actor, navigation, now) || companionMovementEndsAt(actor.movement) > now + companionVehicleStopRules.approachMs) return false;
  const last = actor.movement.path.at(-1)!;
  return Math.hypot(last.x - window[0], last.z - window[2]) <= companionVehicleStopRules.contactReach;
}
/** A terminal visit is admitted by world occupancy, not inferred from repeated client contact. */
export function beginCompanionTerminalLevy(state: CompanionVehicleStopState, args: {
  frame: CompanionRoadAcceptedFrame; terminal: CompanionTerminalNode; visitSequence: number; vehicleKind: CompanionVehicleKind;
  driverUserId: string; payerUserId: string; passenger: boolean; actor: CompanionStreetActorApproach;
  navigation: RoomNavigation; halfWidth: number; halfLength: number; now: number;
}): CompanionVehicleStopState {
  const { terminal, frame, now } = args;
  const namedTerminal = terminal && companionPlaceExpansionDestinations.find(place => place.kind === "bus-terminal" && place.id === terminal.id);
  if (!terminal || !companionStreetKey(terminal.id) || !integer(args.visitSequence) || !pointValid(terminal.point)
    || !namedTerminal || namedTerminal.city.entrance.some((value, index) => value !== terminal.point[index])
    || terminal.navigationRevision !== state.scope.layoutRevision || terminal.exempt || !["keke", "danfo"].includes(args.vehicleKind)
    || !terminal.kinds.includes(args.vehicleKind as "keke" | "danfo") || args.passenger
    || args.driverUserId !== state.scope.userId || args.payerUserId !== args.driverUserId
    || Math.abs(frame.speed) > companionVehicleStopRules.stopSpeed || (state.terminalVisits[terminal.id] ?? -1) >= args.visitSequence
    || !Object.prototype.hasOwnProperty.call(state.terminalVisits, terminal.id) && Object.keys(state.terminalVisits).length >= companionVehicleStopRules.maximumTerminals
    || Math.hypot(frame.pose.x - terminal.point[0], frame.pose.z - terminal.point[2]) > companionVehicleStopRules.terminalReach
    || !admissionReady(state, frame, args.actor, args.navigation, now, companionVehicleDriverWindow(frame.pose, args.halfWidth, args.halfLength))) return state;
  const id = `union:${frame.scope.drivingId}:${terminal.id}:${args.visitSequence}`;
  if (!companionStreetKey(id) || `terminal:${id}`.length > 160) return state;
  const stop: CompanionVehicleStop = { ...baseStop(state, id, id, args.actor, now, "union", "assisted"),
    terminal: { terminalId: terminal.id, visitSequence: args.visitSequence, payerUserId: args.payerUserId,
      amount: args.vehicleKind === "danfo" ? companionVehicleStopRules.levyDanfo : companionVehicleStopRules.levyKeke, unitKey: "lagos-credit" } };
  return { ...state, revision: state.revision + 1, terminalVisits: { ...state.terminalVisits, [terminal.id]: args.visitSequence },
    stops: [...state.stops.slice(-(companionVehicleStopRules.maximumStops - 1)), stop] };
}
export function beginCompanionLastmaStop(state: CompanionVehicleStopState, args: {
  frame: CompanionRoadAcceptedFrame; evidenceId: string; violation: "prohibited-direction" | "dedicated-lane";
  witness: CompanionStreetWitness; actor: CompanionStreetActorApproach; responseMode: "timed" | "assisted";
  navigation: RoomNavigation; halfWidth: number; halfLength: number; now: number;
}): CompanionVehicleStopState {
  const { frame, witness } = args;
  if (!companionStreetKey(args.evidenceId) || !["prohibited-direction", "dedicated-lane"].includes(args.violation)
    || !["timed", "assisted"].includes(args.responseMode) || frame.sequence <= state.lastViolationSequence || witness.faction !== "traffic"
    || witness.agency !== "lastma" || witness.reserved || witness.id !== args.actor.actorId
    || !integer(witness.at) || args.now < witness.at || args.now - witness.at > companionVehicleStopRules.observationAgeMs
    || Math.abs(witness.at - frame.at) > companionVehicleStopRules.observationAgeMs
    || !pointValid(witness.point) || Math.hypot(witness.point[0] - args.actor.movement.from.x, witness.point[2] - args.actor.movement.from.z) > .1
    || !admissionReady(state, frame, args.actor, args.navigation, args.now, companionVehicleDriverWindow(frame.pose, args.halfWidth, args.halfLength))) return state;
  const id = `lastma:${frame.scope.drivingId}:${frame.sequence}`;
  if (!companionStreetKey(id)) return state;
  const stop = { ...baseStop(state, id, args.evidenceId, args.actor, args.now, "lastma", args.responseMode), violation: args.violation };
  return { ...state, revision: state.revision + 1, lastViolationSequence: frame.sequence,
    stops: [...state.stops.slice(-(companionVehicleStopRules.maximumStops - 1)), stop] };
}
export function captureCompanionTerminalLevy(stop: CompanionVehicleStop | null): CompanionTerminalLevyConfirmation | null {
  if (!stop || !validCompanionVehicleStop(stop) || stop.kind !== "union" || stop.phase !== "levy" || !stop.terminal || stop.paymentReceiptId) return null;
  return { stopId: stop.id, scope: { ...stop.scope }, revision: stop.revision, ...stop.terminal, expiresAt: stop.expiresAt };
}
export function validCompanionTerminalLevyConfirmation(confirmation: CompanionTerminalLevyConfirmation): boolean {
  return !!confirmation && companionStreetKey(confirmation.stopId) && validCompanionVehicleStopScope(confirmation.scope)
    && integer(confirmation.revision) && companionStreetKey(confirmation.terminalId) && integer(confirmation.visitSequence)
    && confirmation.payerUserId === confirmation.scope.userId && confirmation.unitKey === "lagos-credit"
    && (confirmation.amount === companionVehicleStopRules.levyKeke || confirmation.amount === companionVehicleStopRules.levyDanfo)
    && integer(confirmation.expiresAt);
}
export function matchesCompanionTerminalLevy(stop: CompanionVehicleStop | null, confirmation: CompanionTerminalLevyConfirmation) {
  const current = captureCompanionTerminalLevy(stop);
  return !!current && validCompanionTerminalLevyConfirmation(confirmation)
    && sameCompanionVehicleStopScope(current.scope, confirmation.scope) && current.stopId === confirmation.stopId
    && current.revision === confirmation.revision && current.terminalId === confirmation.terminalId
    && current.visitSequence === confirmation.visitSequence && current.payerUserId === confirmation.payerUserId
    && current.amount === confirmation.amount && current.unitKey === confirmation.unitKey && current.expiresAt === confirmation.expiresAt;
}
/** Exact committed terms remain readable after the driver leaves. This grants no new passage or debit. */
export function matchesSettledCompanionTerminalLevy(stop: CompanionVehicleStop | null, confirmation: CompanionTerminalLevyConfirmation) {
  return !!stop && validCompanionVehicleStop(stop) && validCompanionTerminalLevyConfirmation(confirmation)
    && stop.kind === "union" && stop.phase === "released" && stop.paymentReceiptId !== null && stop.id === confirmation.stopId
    && sameCompanionVehicleStopScope(stop.scope, confirmation.scope) && stop.revision === confirmation.revision + 1
    && stop.terminal?.amount === confirmation.amount && stop.terminal.terminalId === confirmation.terminalId
    && stop.terminal.visitSequence === confirmation.visitSequence && stop.terminal.payerUserId === confirmation.payerUserId
    && stop.terminal.unitKey === confirmation.unitKey && stop.expiresAt === confirmation.expiresAt;
}
/** Called in the same transaction as the eligible carried-value settlement. */
export function settleCompanionTerminalLevy(state: CompanionVehicleStopState, confirmation: CompanionTerminalLevyConfirmation,
  receiptId: string, now: number): CompanionVehicleStopState {
  const stop = state.stops.find(value => value.id === confirmation.stopId);
  if (stateValid(state) && stop?.paymentReceiptId === receiptId && matchesSettledCompanionTerminalLevy(stop, confirmation)) return state;
  if (!stateValid(state) || !companionStreetKey(receiptId) || !stop || !matchesCompanionTerminalLevy(stop, confirmation)
    || !integer(now) || now < stop.updatedAt || now >= stop.expiresAt) throw new Error("This terminal levy ended or changed.");
  return replace(state, { ...stop, paymentReceiptId: receiptId, phase: "released", updatedAt: now, revision: stop.revision + 1 });
}
/** One accepted response. Assistance widens the window; it never sends a retained key press. */
export function respondCompanionVehicleStop(state: CompanionVehicleStopState, input: CompanionVehicleStopInput,
  now: number, activeInput: boolean): CompanionVehicleStopState {
  const stop = state.stops.find(value => value.id === input.stopId);
  if (!stateValid(state) || !stop || activeInput !== true || !integer(now) || now < stop.updatedAt || !integer(input.sequence)
    || !companionStreetKey(input.requestId) || input.sequence <= stop.lastInputSequence || stop.lastRequestId === input.requestId
    || input.revision !== stop.revision || terminalPhase(stop.phase)) return state;
  if (["guard-key", "cooperate"].includes(input.action) && (stop.lastObservationAt === null
    || now - stop.lastObservationAt > companionVehicleStopRules.observationAgeMs)) return state;
  let patch: Partial<CompanionVehicleStop> | null = null;
  if (stop.kind === "union" && stop.phase === "levy" && now < stop.expiresAt) {
    if (input.action === "run-stop") patch = { phase: "threat", expiresAt: now + companionVehicleStopRules.threatMs,
      escapeBreakAt: null, lastObservationAt: null };
    if (input.action === "decline-levy") patch = { phase: "released" };
  } else if (stop.kind === "lastma" && (stop.phase === "approaching" || stop.phase === "response")
    && input.action === "cooperate" && stop.response === null && now < stop.expiresAt
    && (stop.responseClosesAt === null || now < stop.responseClosesAt)) {
    patch = { phase: "review", response: { requestId: input.requestId, sequence: input.sequence, at: now, result: "cooperated" },
      reviewEndsAt: now + companionVehicleStopRules.reviewMs, expiresAt: now + companionVehicleStopRules.reviewMs };
  } else if (stop.kind === "lastma" && stop.phase === "response" && input.action === "guard-key" && stop.response === null
    && stop.responseOpensAt !== null && stop.responseClosesAt !== null && now >= stop.responseOpensAt && now < stop.responseClosesAt) {
    patch = { phase: "released", response: { requestId: input.requestId, sequence: input.sequence, at: now, result: "guarded" } };
  } else if (stop.phase === "review" && stop.reviewEndsAt !== null && now >= stop.reviewEndsAt && ["review", "recover"].includes(input.action)) {
    patch = { phase: "released" };
  }
  if (!patch) return state;
  return replace(state, { ...stop, ...patch, revision: stop.revision + 1, updatedAt: now,
    lastInputSequence: input.sequence, lastRequestId: input.requestId });
}
/** Existing world cadence invokes this only with current accepted actor and driver observations. */
export function observeCompanionVehicleStop(state: CompanionVehicleStopState, observation: CompanionVehicleStopObservation,
  navigation: RoomNavigation, halfWidth: number, halfLength: number, now: number) {
  const result = { state, panelDamage: null as CompanionVehiclePanelDamageRequest | null, ignition: "unchanged" as "unchanged" | "interrupt" | "release" };
  if (!stateValid(state) || !integer(now) || state.stops.some(stop => now < stop.updatedAt)) return result;
  // A lost body or stale observation cannot extend this stop's deadline.
  result.state = expireCompanionVehicleStops(state, now);
  if (result.state !== state) {
    if (companionVehicleStopIgnitionHeld(state) && !companionVehicleStopIgnitionHeld(result.state)) result.ignition = "release";
    return result;
  }
  const stop = state.stops.find(value => !terminalPhase(value.phase));
  const { frame } = observation;
  if (!stop || !validCompanionRoadFrame(frame)
    || !sameCompanionRoadScope(stop.scope, frame.scope) || now < frame.at
    || now - frame.at > companionVehicleStopRules.observationAgeMs) return result;
  if (!frame.active || stop.kind === "lastma" && ["approaching", "response"].includes(stop.phase)
    && (observation.responseEligible !== true || stop.lastObservationAt !== null
      && now - stop.lastObservationAt > companionVehicleStopRules.observationGapMs)) {
    result.state = interruptCompanionVehicleStops(state, now);
    if (companionVehicleStopIgnitionHeld(state)) result.ignition = "release";
    return result;
  }
  if (!integer(observation.actorAt) || now < observation.actorAt
    || now - observation.actorAt > companionVehicleStopRules.observationAgeMs
    || observation.actorReservationId !== stop.actor.reservationId || !pointValid(observation.actorPoint)
    || stop.lastObservationAt !== null && now <= stop.lastObservationAt) return result;
  const sampled = companionStreetApproachPoint(stop.actor, navigation, now);
  if (!sampled) {
    result.state = interruptCompanionVehicleStops(state, now);
    if (companionVehicleStopIgnitionHeld(state)) result.ignition = "release";
    return result;
  }
  if (Math.hypot(sampled[0] - observation.actorPoint[0], sampled[2] - observation.actorPoint[2]) > .1
    || Math.abs(sampled[1] - observation.actorPoint[1]) > .1) return result;
  const window = companionVehicleDriverWindow(frame.pose, halfWidth, halfLength);
  if (!window) return result;
  const distance = Math.hypot(window[0] - observation.actorPoint[0], window[2] - observation.actorPoint[2]);
  const contact = distance <= companionVehicleStopRules.contactReach && Math.abs(window[1] - observation.actorPoint[1]) <= .1
    && routeSegmentClear({ ...navigation, radius: .001 }, observation.actorPoint, window);
  // Observing the same demand must not invalidate an open captured payment sheet.
  let next: CompanionVehicleStop = { ...stop, lastObservationAt: now, updatedAt: now };
  if (stop.phase === "review" && stop.reviewEndsAt !== null && now >= stop.reviewEndsAt) next = { ...next, phase: "released" };
  else if (stop.phase === "threat") {
    const broken = distance >= companionVehicleStopRules.escapeReach
      || !routeSegmentClear({ ...navigation, radius: .001 }, observation.actorPoint, window);
    const contiguous = stop.lastObservationAt !== null && now - stop.lastObservationAt <= companionVehicleStopRules.observationGapMs;
    next = { ...next, escapeBreakAt: broken ? contiguous && stop.escapeBreakAt !== null ? stop.escapeBreakAt : now : null };
    if (next.escapeBreakAt !== null && now - next.escapeBreakAt >= companionVehicleStopRules.escapeBreakMs) next = { ...next, phase: "escaped" };
    else if (!frame.externalHold && contact && observation.contact && companionStreetKey(observation.contact.id)
      && observation.contact.actorId === stop.actor.actorId && observation.contact.vehicleId === stop.scope.vehicleId
      && integer(observation.contact.at) && observation.contact.at >= stop.expiresAt - companionVehicleStopRules.threatMs
      && now >= observation.contact.at
      && now - observation.contact.at <= companionVehicleStopRules.observationAgeMs && !stop.contactIds.includes(observation.contact.id)
      && stop.contactIds.length < companionVehicleStopRules.maximumContacts
      && (stop.lastContactAt === null || observation.contact.at - stop.lastContactAt >= companionVehicleStopRules.contactCooldownMs)) {
      result.panelDamage = { eventKey: `union-panel:${stop.id}:${stop.contactIds.length}`, stopId: stop.id, vehicleId: stop.scope.vehicleId,
        actorId: stop.actor.actorId, contactId: observation.contact.id, amount: companionVehicleStopRules.panelDamage, at: observation.contact.at };
      next = { ...next, contactIds: [...stop.contactIds, observation.contact.id], lastContactAt: observation.contact.at,
        panelDamageRequested: stop.panelDamageRequested + companionVehicleStopRules.panelDamage };
    }
  } else if (stop.phase === "approaching" && stop.kind === "union" && contact && Math.abs(frame.speed) <= companionVehicleStopRules.stopSpeed) {
    next = { ...next, phase: "levy", expiresAt: now + companionVehicleStopRules.threatMs };
  } else if (stop.phase === "approaching" && stop.kind === "lastma" && distance <= 3
    && routeSegmentClear({ ...navigation, radius: stop.actor.actorRadius }, observation.actorPoint, window)) {
    const opensAt = now + companionVehicleStopRules.telegraphMs;
    const closesAt = opensAt + (stop.responseMode === "assisted" ? companionVehicleStopRules.assistedResponseMs : companionVehicleStopRules.responseMs);
    next = { ...next, phase: "response", telegraphAt: now, responseOpensAt: opensAt, responseClosesAt: closesAt, expiresAt: closesAt + 3_000 };
  } else if (stop.phase === "response" && stop.responseClosesAt !== null && now >= stop.responseClosesAt && contact
    && stop.response === null && Math.abs(frame.speed) <= companionVehicleStopRules.stopSpeed && observation.responseEligible === true) {
    next = { ...next, phase: "review", response: { requestId: `missed:${frame.sequence}`, sequence: frame.sequence, at: now, result: "missed" },
      ignitionInterruptedAt: now, reviewEndsAt: now + companionVehicleStopRules.reviewMs, expiresAt: now + companionVehicleStopRules.reviewMs };
    result.ignition = "interrupt";
  }
  if (next.phase !== stop.phase || next.panelDamageRequested !== stop.panelDamageRequested) next = { ...next, revision: stop.revision + 1 };
  if (next.phase === "released" && stop.ignitionInterruptedAt !== null) result.ignition = "release";
  result.state = replace(state, next);
  return result;
}
/** Finite deadline cleanup works even when a route/body becomes unavailable. It imposes no penalty. */
export function expireCompanionVehicleStops(state: CompanionVehicleStopState, now: number): CompanionVehicleStopState {
  if (!stateValid(state) || !integer(now) || state.stops.some(stop => now < stop.updatedAt)) return state;
  const stops = state.stops.map(stop => terminalPhase(stop.phase) || now < stop.expiresAt ? stop : { ...stop,
    phase: "released" as const, updatedAt: now, revision: stop.revision + 1 });
  return stops.every((stop, index) => stop === state.stops[index]) ? state : { ...state, revision: state.revision + 1, stops };
}
/** Lifecycle owner calls on focus/overlay loss, death, custody, departure or changed device. */
export function interruptCompanionVehicleStops(state: CompanionVehicleStopState, now: number): CompanionVehicleStopState {
  if (!stateValid(state) || !integer(now) || state.stops.some(stop => now < stop.updatedAt)) return state;
  const stops = state.stops.map(stop => terminalPhase(stop.phase) ? stop : { ...stop, phase: "interrupted" as const,
    updatedAt: now, revision: stop.revision + 1 });
  return stops.every((stop, index) => stop === state.stops[index]) ? state : { ...state, revision: state.revision + 1, stops };
}
export const companionVehicleStopIgnitionHeld = (state: CompanionVehicleStopState) => state.stops.some(stop =>
  stop.kind === "lastma" && stop.ignitionInterruptedAt !== null && !terminalPhase(stop.phase));
/** Advanced outcomes require their actor/collision/recovery owners. Default support is empty. */
export function companionStreetEscalationAvailability(support: Readonly<{ mirrorContact: boolean; mobActors: boolean;
  plankProjectile?: boolean; doorFrameAttachment?: boolean;
  panelContact: boolean; spikeCollision: boolean; towReservation: boolean; armedCombat: boolean; recovery: boolean }>) {
  return { "panel-threat": support.panelContact && support.recovery,
    "plank-throw": support.plankProjectile === true && support.panelContact && support.recovery,
    "door-frame-latch": support.doorFrameAttachment === true && support.panelContact && support.recovery,
    "broken-mirrors": support.mirrorContact && support.recovery,
    "mob-swarm": support.mobActors && support.recovery, spikes: support.spikeCollision && support.recovery,
    towing: support.towReservation && support.recovery, "armed-standoff": support.armedCombat && support.recovery };
}

export type CompanionStreetAuthoritySnapshot = Readonly<{
  scope: CompanionVehicleStopScope | null; stop: CompanionVehicleStop | null;
  pressure: Readonly<{ union: number; traffic: number; tactical: number }> | null;
  serverNow: number; ready: boolean; busy: boolean; error: string | null;
}>;
export const emptyCompanionStreetAuthoritySnapshot: CompanionStreetAuthoritySnapshot = {
  scope: null, stop: null, pressure: null, serverNow: 0, ready: false, busy: false, error: null,
};
export interface CompanionStreetAuthorityStore {
  getSnapshot(): CompanionStreetAuthoritySnapshot; subscribe(listener: () => void): () => void;
  select(scope: CompanionVehicleStopScope | null): void;
  respond(action: CompanionVehicleStopInput["action"]): Promise<void>;
  pay(confirmation: CompanionTerminalLevyConfirmation): Promise<void>; dispose(): void;
}
