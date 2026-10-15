import type { NavigationObstacle, ScenePoint } from "./companionNavigation";

/** Incident deadlines use real server milliseconds, independently of the 4x day. */
export const companionStreetCrashTuning = {
  vehicles: 16, incidents: 2, movementWindowMs: 250, admissionWindowMs: 2000,
  minimumImpactSpeed: 1.8, ambulanceDispatchMs: 14_000, policeDispatchMs: 22_000,
  opportunistDelayMs: 8000, attendanceMs: 12_000, disperseMs: 8000,
  expiresMs: 120_000, vehicleCooldownMs: 180_000,
} as const;

export type CompanionStreetCrashScope = Readonly<{
  worldKey: string; epochKey: string; gameRoomId: string; instanceId: string;
  layoutRevision: string; sharedStartedAt: number;
}>;
export type CompanionStreetVehiclePose = Readonly<{
  id: string; point: ScenePoint; yaw: number; halfWidth: number; halfLength: number;
}>;
/** Resolved by the movement owner. These are not client-reported collision inputs.
 * A sweep has fixed yaw; split a turning path into accepted straight segments. */
export type CompanionStreetVehicleSweep = CompanionStreetVehiclePose & Readonly<{
  instanceId: string; layoutRevision: string; sequence: number;
  from: ScenePoint; startsAt: number; endsAt: number;
}>;
export type CompanionStreetCollision = Readonly<{
  id: string; instanceId: string; layoutRevision: string; occurredAt: number;
  point: ScenePoint; impactSpeed: number;
  vehicles: readonly [CompanionStreetVehiclePose, CompanionStreetVehiclePose];
  sequences: readonly [number, number];
}>;
export type CompanionStreetCrashResponder = Readonly<{
  kind: "ambulance" | "police"; requestedAt: number;
  actorId: string | null; dispatchedAt: number | null; arrivedAt: number | null;
}>;
export type CompanionStreetCrashRobbery = Readonly<{
  npcId: string; availableAt: number; encounterId: string | null;
  startedAt: number | null; resolvedAt: number | null;
  outcome: "escaped" | "repelled" | "surrendered" | "defeated" | "interrupted" | "expired" | null;
}>;
export type CompanionStreetCrash = Readonly<{
  id: string; point: ScenePoint; occurredAt: number; expiresAt: number; revision: number;
  vehicles: readonly [CompanionStreetVehiclePose, CompanionStreetVehiclePose];
  responders: readonly [CompanionStreetCrashResponder, CompanionStreetCrashResponder];
  robbery: CompanionStreetCrashRobbery | null; clearedAt: number | null;
  recovery: CompanionStreetCrashRecovery | null;
}>;
export type CompanionStreetCrashState = Readonly<{
  scope: CompanionStreetCrashScope; observedAt: number; revision: number;
  fleet: readonly Readonly<{ id: string; lastSequence: number; nextCrashAt: number }>[];
  incidents: readonly CompanionStreetCrash[];
}>;
export type CompanionStreetCrashView = Readonly<{
  scope: CompanionStreetCrashScope; observedAt: number; revision: number;
  incidents: readonly CompanionStreetCrash[]; vehicles: readonly CompanionStreetVehiclePose[];
  obstructions: readonly NavigationObstacle[];
  dispatch: readonly Readonly<{ incidentId: string; revision: number; kind: "ambulance" | "police" }>[];
  robberyContacts: readonly Readonly<{ incidentId: string; revision: number; npcId: string; point: ScenePoint }>[];
  interruptEncounters: readonly string[];
  clearCrashes: readonly Readonly<{ incidentId: string; revision: number }>[];
  expiredIncidents: readonly string[];
}>;

const validId = (id: string) => typeof id === "string" && id.length > 0 && id.length <= 240 && id.trim() === id;
const validTime = (at: number) => Number.isSafeInteger(at) && at >= 0;
const validPoint = (point: ScenePoint) => Array.isArray(point) && point.length === 3 && point.every(Number.isFinite);
const pointCopy = (point: ScenePoint): ScenePoint => Object.freeze([point[0], point[1], point[2]]);
const poseCopy = (pose: CompanionStreetVehiclePose): CompanionStreetVehiclePose => Object.freeze({
  id: pose.id, point: pointCopy(pose.point), yaw: pose.yaw, halfWidth: pose.halfWidth, halfLength: pose.halfLength,
});
const compareId = (a: { id: string }, b: { id: string }) => a.id < b.id ? -1 : a.id > b.id ? 1 : 0;
const validPose = (pose: CompanionStreetVehiclePose) => validId(pose.id) && validPoint(pose.point)
  && Number.isFinite(pose.yaw) && Number.isFinite(pose.halfWidth) && Number.isFinite(pose.halfLength)
  && pose.halfWidth >= .3 && pose.halfWidth <= 3 && pose.halfLength >= .5 && pose.halfLength <= 8;
export const sameCompanionStreetCrashScope = (a: CompanionStreetCrashScope, b: CompanionStreetCrashScope) =>
  a.worldKey === b.worldKey && a.epochKey === b.epochKey && a.gameRoomId === b.gameRoomId
  && a.instanceId === b.instanceId && a.layoutRevision === b.layoutRevision && a.sharedStartedAt === b.sharedStartedAt;
export const validCompanionStreetVehiclePose = validPose;
const activeAt = (incident: CompanionStreetCrash, now: number) => now >= incident.occurredAt
  && (incident.clearedAt === null || now < incident.clearedAt + companionStreetCrashTuning.disperseMs);
const roadBlockedAt = (incident: CompanionStreetCrash, now: number) => activeAt(incident, now) && incident.clearedAt === null;
const robberyActive = (robbery: CompanionStreetCrashRobbery | null) => !!robbery?.encounterId && robbery.resolvedAt === null;
const policePresent = (incident: CompanionStreetCrash) => incident.responders[1].arrivedAt !== null;
function freezeIncident(incident: CompanionStreetCrash): CompanionStreetCrash {
  return Object.freeze({ ...incident, point: pointCopy(incident.point),
    vehicles: Object.freeze([poseCopy(incident.vehicles[0]), poseCopy(incident.vehicles[1])]) as CompanionStreetCrash["vehicles"],
    responders: Object.freeze([Object.freeze({ ...incident.responders[0] }), Object.freeze({ ...incident.responders[1] })]) as CompanionStreetCrash["responders"],
    robbery: incident.robbery ? Object.freeze({ ...incident.robbery }) : null,
    recovery: incident.recovery ? Object.freeze({ ...incident.recovery,
      vehicleIds: Object.freeze([...incident.recovery.vehicleIds]), releasedActorIds: Object.freeze([...incident.recovery.releasedActorIds]) }) : null,
  });
}
function freezeState(state: CompanionStreetCrashState): CompanionStreetCrashState {
  return Object.freeze({ ...state, scope: Object.freeze({ ...state.scope }),
    fleet: Object.freeze(state.fleet.map(item => Object.freeze({ ...item }))),
    incidents: Object.freeze(state.incidents.map(freezeIncident)),
  });
}

type StreetContact = Readonly<{ fraction: number; impactSpeed: number; penetrating: boolean }>;
const validSweep = (sweep: CompanionStreetVehicleSweep) => validPose(sweep) && validPoint(sweep.from)
  && validId(sweep.instanceId) && validId(sweep.layoutRevision) && validTime(sweep.startsAt) && validTime(sweep.endsAt)
  && sweep.endsAt > sweep.startsAt && sweep.endsAt - sweep.startsAt <= companionStreetCrashTuning.movementWindowMs
  && Number.isSafeInteger(sweep.sequence) && sweep.sequence >= 0 && Math.abs(sweep.from[1] - sweep.point[1]) <= .05
  && Math.hypot(sweep.point[0] - sweep.from[0], sweep.point[2] - sweep.from[2]) / (sweep.endsAt - sweep.startsAt) * 1000 <= 35;

/** Continuous SAT. Fraction stays unrounded while resolving simultaneous stops. */
function streetContact(a: CompanionStreetVehicleSweep, b: CompanionStreetVehicleSweep): StreetContact | null {
  const duration = a.endsAt - a.startsAt;
  const adx = a.point[0] - a.from[0], adz = a.point[2] - a.from[2];
  const bdx = b.point[0] - b.from[0], bdz = b.point[2] - b.from[2];
  const axes = [
    [Math.sin(a.yaw), Math.cos(a.yaw)], [Math.cos(a.yaw), -Math.sin(a.yaw)],
    [Math.sin(b.yaw), Math.cos(b.yaw)], [Math.cos(b.yaw), -Math.sin(b.yaw)],
  ];
  const extent = (pose: CompanionStreetVehiclePose, x: number, z: number) =>
    Math.abs(x * Math.sin(pose.yaw) + z * Math.cos(pose.yaw)) * pose.halfLength
    + Math.abs(x * Math.cos(pose.yaw) - z * Math.sin(pose.yaw)) * pose.halfWidth;
  const dx = b.from[0] - a.from[0], dz = b.from[2] - a.from[2];
  let enter = 0, leave = 1, penetrating = true, normalX = 0, normalZ = 0;
  for (const [x, z] of axes) {
    const separation = dx * x + dz * z, delta = (bdx - adx) * x + (bdz - adz) * z;
    const reach = extent(a, x, z) + extent(b, x, z);
    if (Math.abs(separation) >= reach - 1e-6) penetrating = false;
    if (Math.abs(delta) < 1e-9) { if (Math.abs(separation) > reach) return null; continue; }
    const t1 = (-reach - separation) / delta, t2 = (reach - separation) / delta;
    const entry = Math.min(t1, t2);
    if (entry >= enter) {
      enter = entry;
      const direction = separation + delta * entry >= 0 ? 1 : -1;
      normalX = x * direction; normalZ = z * direction;
    }
    leave = Math.min(leave, Math.max(t1, t2));
    if (enter > leave) return null;
  }
  if (enter < 0 || enter > 1 || leave < 0) return null;
  const impactSpeed = ((adx - bdx) * normalX + (adz - bdz) * normalZ) / duration * 1000;
  // Touching while separating is not a new obstruction. Penetration rejects the
  // requested movement instead of being reported as a fresh crash.
  if (!penetrating && impactSpeed <= 1e-9) return null;
  return { fraction: enter, impactSpeed: Math.max(0, impactSpeed), penetrating };
}

function collisionAt(a: CompanionStreetVehicleSweep, b: CompanionStreetVehicleSweep, contact: StreetContact): CompanionStreetCollision {
  const atPose = (sweep: CompanionStreetVehicleSweep): CompanionStreetVehiclePose => poseCopy({ ...sweep,
    point: [sweep.from[0] + (sweep.point[0] - sweep.from[0]) * contact.fraction, sweep.point[1],
      sweep.from[2] + (sweep.point[2] - sweep.from[2]) * contact.fraction],
  });
  const pa = atPose(a), pb = atPose(b), occurredAt = Math.ceil(a.startsAt + (a.endsAt - a.startsAt) * contact.fraction);
  return Object.freeze({ id: JSON.stringify([a.instanceId, a.layoutRevision, a.id, a.sequence, b.id, b.sequence, a.startsAt, a.endsAt]),
    instanceId: a.instanceId, layoutRevision: a.layoutRevision, occurredAt, impactSpeed: contact.impactSpeed,
    point: pointCopy([(pa.point[0] + pb.point[0]) / 2, pa.point[1], (pa.point[2] + pb.point[2]) / 2]),
    vehicles: Object.freeze([pa, pb]) as CompanionStreetCollision["vehicles"],
    sequences: Object.freeze([a.sequence, b.sequence]) as CompanionStreetCollision["sequences"],
  });
}

/** Fresh strong contact only. Use the batch resolver to stop weak/occupied
 * contacts too; a null incident is never permission to cross another vehicle. */
export function detectCompanionStreetCollision(first: CompanionStreetVehicleSweep, second: CompanionStreetVehicleSweep): CompanionStreetCollision | null {
  const [a, b] = [first, second].sort(compareId);
  const duration = a.endsAt - a.startsAt;
  if (!validPose(a) || !validPose(b) || !validPoint(a.from) || !validPoint(b.from) || a.id === b.id
    || !validId(a.instanceId) || !validId(a.layoutRevision) || a.instanceId !== b.instanceId || a.layoutRevision !== b.layoutRevision
    || !validTime(a.startsAt) || !validTime(a.endsAt) || a.startsAt !== b.startsAt || a.endsAt !== b.endsAt
    || duration <= 0 || duration > companionStreetCrashTuning.movementWindowMs
    || !Number.isSafeInteger(a.sequence) || a.sequence < 0 || !Number.isSafeInteger(b.sequence) || b.sequence < 0
    || Math.max(Math.abs(a.from[1] - a.point[1]), Math.abs(b.from[1] - b.point[1]), Math.abs(a.point[1] - b.point[1])) > .05) return null;
  if (!validSweep(a) || !validSweep(b)) return null;
  const contact = streetContact(a, b);
  return contact && !contact.penetrating && contact.impactSpeed >= companionStreetCrashTuning.minimumImpactSpeed
    ? collisionAt(a, b, contact) : null;
}

export type CompanionStreetSweepResolution = Readonly<{
  vehicles: readonly Readonly<{ pose: CompanionStreetVehiclePose; sequence: number; stoppedAt: number | null; contactId: string | null }>[];
  contacts: readonly Readonly<{ collision: CompanionStreetCollision; incidentEligible: boolean }>[];
  error: string | null;
}>;

/** Recompute remaining paths after each stop. A follower can hit an already
 * stopped car later in the same window. Every contact stops movement, even
 * weak impacts and contacts that cannot occupy one of the two incident slots.
 * At most 16 passes of 120 pairs; no time-step sampling or database writes. */
export function resolveCompanionStreetVehicleSweeps(sweeps: readonly CompanionStreetVehicleSweep[]): CompanionStreetSweepResolution {
  if (!sweeps.length) return { vehicles: [], contacts: [], error: null };
  if (sweeps.length > companionStreetCrashTuning.vehicles || new Set(sweeps.map(sweep => sweep.id)).size !== sweeps.length
    || sweeps.some(sweep => !validSweep(sweep))
    || sweeps.some(sweep => sweep.instanceId !== sweeps[0].instanceId || sweep.layoutRevision !== sweeps[0].layoutRevision
      || sweep.startsAt !== sweeps[0].startsAt || sweep.endsAt !== sweeps[0].endsAt
      || Math.abs(sweep.point[1] - sweeps[0].point[1]) > .05)) return { vehicles: [], contacts: [], error: "Use one bounded simultaneous fixed-yaw fleet window." };
  const paths = [...sweeps].sort(compareId).map(sweep => ({ sweep, pose: poseCopy({ ...sweep, point: sweep.from }),
    stopped: sweep.from[0] === sweep.point[0] && sweep.from[2] === sweep.point[2], stoppedAt: null as number | null, contactId: null as string | null }));
  const contacts: CompanionStreetSweepResolution["contacts"][number][] = [];
  const startsAt = sweeps[0].startsAt, endsAt = sweeps[0].endsAt;
  let fraction = 0;
  for (let pass = 0; pass < companionStreetCrashTuning.vehicles && fraction < 1; pass++) {
    const remaining = paths.map(path => ({ ...path.sweep, from: path.pose.point,
      point: path.stopped ? path.pose.point : path.sweep.point }));
    let nextFraction = Infinity;
    const simultaneous: { a: number; b: number; contact: StreetContact }[] = [];
    for (let a = 0; a < paths.length; a++) for (let b = a + 1; b < paths.length; b++) {
      if (paths[a].stopped && paths[b].stopped) continue;
      const contact = streetContact(remaining[a], remaining[b]);
      if (!contact) continue;
      if (contact.fraction < nextFraction - 1e-9) { nextFraction = contact.fraction; simultaneous.length = 0; }
      if (Math.abs(contact.fraction - nextFraction) <= 1e-9) simultaneous.push({ a, b, contact });
    }
    if (!simultaneous.length) {
      for (let index = 0; index < paths.length; index++) paths[index].pose = poseCopy(remaining[index]);
      fraction = 1; break;
    }
    const previous = fraction;
    fraction += (1 - fraction) * nextFraction;
    for (let index = 0; index < paths.length; index++) {
      const path = paths[index], sweep = remaining[index];
      path.pose = poseCopy({ ...sweep, point: [sweep.from[0] + (sweep.point[0] - sweep.from[0]) * nextFraction,
        sweep.point[1], sweep.from[2] + (sweep.point[2] - sweep.from[2]) * nextFraction] });
    }
    const occurredAt = Math.ceil(startsAt + (endsAt - startsAt) * fraction);
    for (const next of simultaneous) {
      const a = paths[next.a], b = paths[next.b];
      const base = collisionAt(a.sweep, b.sweep, { ...next.contact, fraction });
      const collision = Object.freeze({ ...base, occurredAt,
        impactSpeed: next.contact.impactSpeed / (1 - previous),
        vehicles: Object.freeze([a.pose, b.pose]) as CompanionStreetCollision["vehicles"],
        point: pointCopy([(a.pose.point[0] + b.pose.point[0]) / 2, a.pose.point[1], (a.pose.point[2] + b.pose.point[2]) / 2]),
      });
      contacts.push(Object.freeze({ collision, incidentEligible: !next.contact.penetrating
        && collision.impactSpeed >= companionStreetCrashTuning.minimumImpactSpeed }));
      for (const path of [a, b]) {
        if (!path.stopped) { path.stoppedAt = occurredAt; path.contactId = collision.id; }
        path.stopped = true;
      }
    }
  }
  return Object.freeze({ vehicles: Object.freeze(paths.map(path => Object.freeze({ pose: path.pose,
    sequence: path.sweep.sequence, stoppedAt: path.stoppedAt, contactId: path.contactId }))), contacts: Object.freeze(contacts), error: null });
}

export function detectCompanionStreetCollisions(sweeps: readonly CompanionStreetVehicleSweep[]): readonly CompanionStreetCollision[] {
  return Object.freeze(resolveCompanionStreetVehicleSweeps(sweeps).contacts.filter(contact => contact.incidentEligible).map(contact => contact.collision));
}

export function createCompanionStreetCrashState(scope: CompanionStreetCrashScope, vehicleIds: readonly string[]): CompanionStreetCrashState {
  if (![scope.worldKey, scope.epochKey, scope.gameRoomId, scope.instanceId, scope.layoutRevision].every(validId)
    || !validTime(scope.sharedStartedAt) || !validTime(scope.sharedStartedAt + companionStreetCrashTuning.vehicleCooldownMs)
    || vehicleIds.length > companionStreetCrashTuning.vehicles || vehicleIds.some(id => !validId(id))
    || new Set(vehicleIds).size !== vehicleIds.length) throw new Error("Use the resolved instance scope and a unique bounded traffic fleet.");
  return freezeState({ scope, observedAt: scope.sharedStartedAt, revision: 0,
    fleet: vehicleIds.map(id => ({ id, lastSequence: -1, nextCrashAt: scope.sharedStartedAt })).sort(compareId), incidents: [] });
}

export type CompanionStreetCrashResult = Readonly<{ state: CompanionStreetCrashState; changed: boolean; error: string | null }>;
const unchanged = (state: CompanionStreetCrashState, error: string | null = null): CompanionStreetCrashResult => ({ state, changed: false, error });
const currentTime = (state: CompanionStreetCrashState, now: number) => validTime(now) && now >= state.observedAt
  && validTime(now + companionStreetCrashTuning.vehicleCooldownMs);

/** Call only with a contact re-derived from accepted server movement, in the same
 * transaction that stops the vehicles. This module grants no client authority. */
export function admitCompanionStreetCrash(state: CompanionStreetCrashState, collision: CompanionStreetCollision, now: number,
  availableOpportunistNpcId: string | null = null): CompanionStreetCrashResult {
  if (!currentTime(state, now) || collision.instanceId !== state.scope.instanceId || collision.layoutRevision !== state.scope.layoutRevision
    || !validId(collision.instanceId) || !validTime(collision.occurredAt) || collision.occurredAt < state.scope.sharedStartedAt
    || collision.occurredAt > now || now - collision.occurredAt > companionStreetCrashTuning.admissionWindowMs
    || !validPoint(collision.point) || !Number.isFinite(collision.impactSpeed) || collision.impactSpeed < companionStreetCrashTuning.minimumImpactSpeed
    || collision.vehicles.length !== 2 || collision.vehicles.some(pose => !validPose(pose))
    || collision.vehicles[0].id === collision.vehicles[1].id
    || collision.sequences.length !== 2 || collision.sequences.some(sequence => !Number.isSafeInteger(sequence) || sequence < 0)
    || typeof collision.id !== "string" || !collision.id || collision.id.length > 1600
    || availableOpportunistNpcId !== null && !validId(availableOpportunistNpcId)) return unchanged(state, "Use a fresh accepted collision in this instance.");
  if (state.incidents.some(incident => incident.id === collision.id)) return unchanged(state);
  const fleet = collision.vehicles.map(pose => state.fleet.find(vehicle => vehicle.id === pose.id));
  if (fleet.some((vehicle, index) => !vehicle || collision.sequences[index] <= vehicle.lastSequence
    || collision.occurredAt < vehicle.nextCrashAt)) return unchanged(state, "The vehicle contact is stale or cooling down.");
  // Retain stopped vehicles through expiry until recovery is acknowledged.
  const retained = state.incidents.filter(incident => activeAt(incident, now) || robberyActive(incident.robbery));
  if (retained.length >= companionStreetCrashTuning.incidents
    || retained.some(incident => incident.vehicles.some(pose => collision.vehicles.some(vehicle => vehicle.id === pose.id)))) return unchanged(state, "The incident or vehicle is already occupied.");
  let seed = 2166136261;
  for (let index = 0; index < collision.id.length; index++) seed = Math.imul(seed ^ collision.id.charCodeAt(index), 16777619);
  const robbery = availableOpportunistNpcId && (seed >>> 0) % 4 === 0
    && !retained.some(incident => incident.robbery?.npcId === availableOpportunistNpcId && incident.robbery.resolvedAt === null)
    ? { npcId: availableOpportunistNpcId, availableAt: collision.occurredAt + companionStreetCrashTuning.opportunistDelayMs,
      encounterId: null, startedAt: null, resolvedAt: null, outcome: null } : null;
  const responder = (kind: CompanionStreetCrashResponder["kind"], delay: number): CompanionStreetCrashResponder => ({
    kind, requestedAt: collision.occurredAt + delay, actorId: null, dispatchedAt: null, arrivedAt: null,
  });
  const incident: CompanionStreetCrash = {
    id: collision.id, point: collision.point, occurredAt: collision.occurredAt,
    expiresAt: collision.occurredAt + companionStreetCrashTuning.expiresMs, revision: 0, vehicles: collision.vehicles,
    responders: [responder("ambulance", companionStreetCrashTuning.ambulanceDispatchMs), responder("police", companionStreetCrashTuning.policeDispatchMs)],
    robbery, clearedAt: null, recovery: null,
  };
  return { state: freezeState({ ...state, observedAt: now, revision: state.revision + 1,
    fleet: state.fleet.map(vehicle => {
      const index = collision.vehicles.findIndex(pose => pose.id === vehicle.id);
      return index < 0 ? vehicle : { ...vehicle, lastSequence: collision.sequences[index], nextCrashAt: collision.occurredAt + companionStreetCrashTuning.vehicleCooldownMs };
    }), incidents: [...retained, incident],
  }), changed: true, error: null };
}

export type CompanionStreetCrashCommand =
  | Readonly<{ kind: "dispatch"; responder: "ambulance" | "police"; actorId: string }>
  | Readonly<{ kind: "arrive"; responder: "ambulance" | "police"; actorId: string; acceptedPoint: ScenePoint; stagingPoint: ScenePoint }>
  | Readonly<{ kind: "robbery-started"; npcId: string; encounterId: string }>
  | Readonly<{ kind: "robbery-resolved"; encounterId: string; outcome: NonNullable<CompanionStreetCrashRobbery["outcome"]> }>
  | Readonly<{ kind: "clear"; recovery: CompanionStreetCrashRecovery }>
  /** The world owner has retained road-located vehicles and released responders.
   * Expiry alone must not resume traffic or reset an owned vehicle's position. */
  | Readonly<{ kind: "expire"; recovery: CompanionStreetCrashRecovery }>;

/** Receipt from the actual roadside/vehicle owner, never a timer or client claim.
 * The world transaction preserves location, fuel and condition and releases
 * all assigned responder actors before supplying this acknowledgement. */
export type CompanionStreetCrashRecovery = Readonly<{
  receiptId: string; vehicleIds: readonly string[]; releasedActorIds: readonly string[];
}>;

/** Commands are acknowledged owner outcomes, not RPC args. The owner resolves
 * staging/navigation, health and combat before calling this pure reducer. */
export function updateCompanionStreetCrash(state: CompanionStreetCrashState, incidentId: string, revision: number,
  command: CompanionStreetCrashCommand, now: number): CompanionStreetCrashResult {
  const incident = state.incidents.find(item => item.id === incidentId);
  if (!currentTime(state, now) || !incident || !roadBlockedAt(incident, now)
    && !(command.kind === "robbery-resolved" && robberyActive(incident.robbery))) return unchanged(state, "Resume the current active crash.");
  if (!Number.isSafeInteger(revision) || incident.revision !== revision) return unchanged(state, "The crash changed. Read its current receipt.");
  if (now >= incident.expiresAt && command.kind !== "expire" && command.kind !== "robbery-resolved")
    return unchanged(state, "Settle the expired crash through its world recovery owner.");
  let next = incident;
  if (command.kind === "dispatch" || command.kind === "arrive") {
    if (command.responder !== "ambulance" && command.responder !== "police") return unchanged(state, "Use a supported responder.");
    const index = command.responder === "ambulance" ? 0 : 1;
    const responder = incident.responders[index];
    if (!validId(command.actorId) || now < responder.requestedAt) return unchanged(state, "The responder is not ready for dispatch.");
    if (command.kind === "dispatch") {
      if (responder.actorId === command.actorId) return unchanged(state);
      if (responder.actorId || state.incidents.some(item => roadBlockedAt(item, now)
        && item.responders.some(other => other.actorId === command.actorId))) return unchanged(state, "The responder is already assigned.");
    } else {
      if (responder.actorId !== command.actorId || responder.dispatchedAt === null
        || !validPoint(command.acceptedPoint) || !validPoint(command.stagingPoint)
        || Math.abs(command.acceptedPoint[1] - command.stagingPoint[1]) > .05
        || Math.hypot(command.acceptedPoint[0] - command.stagingPoint[0], command.acceptedPoint[2] - command.stagingPoint[2]) > .8
        || Math.hypot(command.stagingPoint[0] - incident.point[0], command.stagingPoint[2] - incident.point[2]) > 14)
        return unchanged(state, "Accept the responder's arrival at its resolved crash staging point.");
      if (responder.arrivedAt !== null) return unchanged(state);
    }
    const responders = [...incident.responders] as [CompanionStreetCrashResponder, CompanionStreetCrashResponder];
    responders[index] = command.kind === "dispatch" ? { ...responder, actorId: command.actorId, dispatchedAt: now }
      : { ...responder, arrivedAt: now };
    next = { ...incident, responders };
  } else if (command.kind === "robbery-started") {
    const robbery = incident.robbery;
    if (!robbery || policePresent(incident) || now < robbery.availableAt || robbery.resolvedAt !== null
      || command.npcId !== robbery.npcId || !validId(command.encounterId)) return unchanged(state, "This crash is not available for an agbero encounter.");
    if (robbery.encounterId === command.encounterId) return unchanged(state);
    if (robbery.encounterId || state.incidents.some(item => robberyActive(item.robbery))) return unchanged(state, "An agbero encounter already owns this body.");
    next = { ...incident, robbery: { ...robbery, encounterId: command.encounterId, startedAt: now } };
  } else if (command.kind === "robbery-resolved") {
    const robbery = incident.robbery;
    if (!robbery || !robbery.encounterId || robbery.encounterId !== command.encounterId
      || !["escaped", "repelled", "surrendered", "defeated", "interrupted", "expired"].includes(command.outcome))
      return unchanged(state, "Use the owning combat encounter's result.");
    if (robbery.resolvedAt !== null) return unchanged(state, robbery.outcome === command.outcome ? null : "The encounter is already resolved.");
    next = { ...incident, robbery: { ...robbery, resolvedAt: now, outcome: command.outcome } };
  } else if (command.kind === "clear") {
    const arrivals = incident.responders.map(responder => responder.arrivedAt);
    if (arrivals.some(at => at === null) || now < Math.max(arrivals[0]!, arrivals[1]!) + companionStreetCrashTuning.attendanceMs
      || robberyActive(incident.robbery)) return unchanged(state, "Wait for both responders and the acknowledged encounter outcome.");
    if (!validRecovery(incident, command.recovery)) return unchanged(state, "Acknowledge both vehicle recoveries and all responder releases.");
    next = { ...incident, clearedAt: now, recovery: command.recovery };
  } else if (command.kind === "expire") {
    if (now < incident.expiresAt || robberyActive(incident.robbery))
      return unchanged(state, "Wait for crash expiry and the acknowledged encounter outcome.");
    if (!validRecovery(incident, command.recovery)) return unchanged(state, "Acknowledge both vehicle recoveries and all responder releases.");
    next = { ...incident, clearedAt: now, recovery: command.recovery };
  } else return unchanged(state, "Unknown crash command.");
  next = { ...next, revision: incident.revision + 1 };
  return { state: freezeState({ ...state, observedAt: now, revision: state.revision + 1,
    incidents: state.incidents.map(item => item.id === incident.id ? next : item),
  }), changed: true, error: null };
}

function validRecovery(incident: CompanionStreetCrash, recovery: CompanionStreetCrashRecovery) {
  return !!recovery && validId(recovery.receiptId) && recovery.vehicleIds.length === 2
    && new Set(recovery.vehicleIds).size === 2 && incident.vehicles.every(vehicle => recovery.vehicleIds.includes(vehicle.id))
    && recovery.releasedActorIds.length <= 2 && recovery.releasedActorIds.every(validId)
    && new Set(recovery.releasedActorIds).size === recovery.releasedActorIds.length
    && incident.responders.filter(responder => responder.actorId !== null).every(responder => recovery.releasedActorIds.includes(responder.actorId!))
    && recovery.releasedActorIds.every(id => incident.responders.some(responder => responder.actorId === id));
}

export function companionStreetCrashFootprint(vehicle: CompanionStreetVehiclePose): NavigationObstacle {
  return Object.freeze({ minX: vehicle.point[0] - vehicle.halfWidth, maxX: vehicle.point[0] + vehicle.halfWidth,
    minZ: vehicle.point[2] - vehicle.halfLength, maxZ: vehicle.point[2] + vehicle.halfLength, yaw: vehicle.yaw });
}

/** Read-only local projection. Due dispatch/contact entries request owner work;
 * neither a timer nor a rendered body can acknowledge arrival or transfer loot. */
export function projectCompanionStreetCrashes(state: CompanionStreetCrashState, now: number): CompanionStreetCrashView | null {
  if (!currentTime(state, now)) return null;
  const incidents = state.incidents.filter(incident => activeAt(incident, now));
  const blocked = incidents.filter(incident => roadBlockedAt(incident, now));
  const vehicles = blocked.flatMap(incident => incident.vehicles);
  return Object.freeze({ scope: state.scope, observedAt: now, revision: state.revision,
    incidents: Object.freeze(incidents), vehicles: Object.freeze(vehicles), obstructions: Object.freeze(vehicles.map(companionStreetCrashFootprint)),
    dispatch: Object.freeze(blocked.filter(incident => now < incident.expiresAt).flatMap(incident => incident.responders.filter(responder => responder.actorId === null && now >= responder.requestedAt)
      .map(responder => Object.freeze({ incidentId: incident.id, revision: incident.revision, kind: responder.kind })))),
    robberyContacts: Object.freeze(blocked.filter(incident => now < incident.expiresAt && incident.robbery && now >= incident.robbery.availableAt
      && !policePresent(incident) && !incident.robbery.encounterId && incident.robbery.resolvedAt === null)
      .map(incident => Object.freeze({ incidentId: incident.id, revision: incident.revision, npcId: incident.robbery!.npcId, point: incident.point }))),
    // An expired crash still needs its active combat receipt settled by its owner.
    interruptEncounters: Object.freeze(state.incidents.filter(incident => robberyActive(incident.robbery)
      && (policePresent(incident) || now >= incident.expiresAt)).map(incident => incident.robbery!.encounterId!)),
    clearCrashes: Object.freeze(blocked.filter(incident => now < incident.expiresAt && incident.responders.every(responder => responder.arrivedAt !== null)
      && now >= Math.max(incident.responders[0].arrivedAt!, incident.responders[1].arrivedAt!) + companionStreetCrashTuning.attendanceMs
      && !robberyActive(incident.robbery)).map(incident => Object.freeze({ incidentId: incident.id, revision: incident.revision }))),
    expiredIncidents: Object.freeze(state.incidents.filter(incident => now >= incident.expiresAt && incident.clearedAt === null).map(incident => incident.id)),
  });
}
