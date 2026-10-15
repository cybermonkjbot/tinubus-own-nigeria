import type { NavigationObstacle, ScenePoint } from "./companionNavigation";
import { admitCompanionStreetCrash, companionStreetCrashFootprint, companionStreetCrashTuning,
  createCompanionStreetCrashState, projectCompanionStreetCrashes, resolveCompanionStreetVehicleSweeps,
  sameCompanionStreetCrashScope, validCompanionStreetVehiclePose,
  type CompanionStreetCrashScope, type CompanionStreetCrashState, type CompanionStreetCrashView,
  type CompanionStreetVehiclePose, type CompanionStreetVehicleSweep } from "./companionStreetCrashes";

export type CompanionStreetTrafficVehicle = Readonly<{
  pose: CompanionStreetVehiclePose; sequence: number;
  owner: "ambient" | "player" | "resident" | "responder";
  speed: number;
  /** Parked/recovered vehicles keep an obstacle without restarting an ambient lap. */
  parked: boolean;
  hold: Readonly<{ contactId: string; stoppedAt: number }> | null;
}>;
export type CompanionStreetCrashTrafficState = Readonly<{
  scope: CompanionStreetCrashScope; observedAt: number; revision: number;
  vehicles: readonly CompanionStreetTrafficVehicle[]; crashes: CompanionStreetCrashState;
}>;
/** Accepted participant paths for this exact movement window, including leased
 * NPC/combat bodies. A missing/overflow snapshot rejects the whole window. */
export type CompanionStreetTrafficParticipant = Readonly<{ id: string; from: ScenePoint; point: ScenePoint; radius: number }>;
export type CompanionStreetCrashTrafficView = Readonly<{
  scope: CompanionStreetCrashScope; observedAt: number; revision: number;
  vehicles: readonly CompanionStreetTrafficVehicle[]; obstructions: readonly NavigationObstacle[];
  crashes: CompanionStreetCrashView;
}>;
const key = (value: string) => typeof value === "string" && value.length > 0 && value.length <= 240 && value.trim() === value;
const time = (value: number) => Number.isSafeInteger(value) && value >= 0;
const point = (value: ScenePoint) => Array.isArray(value) && value.length === 3 && value.every(Number.isFinite);
const samePoint = (a: ScenePoint, b: ScenePoint) => a.every((value, index) => Math.abs(value - b[index]) <= 1e-6);
const copyPose = (pose: CompanionStreetVehiclePose): CompanionStreetVehiclePose => Object.freeze({ ...pose,
  point: Object.freeze([...pose.point]) as ScenePoint });
const copyVehicle = (vehicle: CompanionStreetTrafficVehicle): CompanionStreetTrafficVehicle => Object.freeze({ ...vehicle,
  pose: copyPose(vehicle.pose), hold: vehicle.hold ? Object.freeze({ ...vehicle.hold }) : null });
const freeze = (state: CompanionStreetCrashTrafficState): CompanionStreetCrashTrafficState => Object.freeze({ ...state,
  scope: Object.freeze({ ...state.scope }), vehicles: Object.freeze(state.vehicles.map(copyVehicle)) });

/** Bounded late-join/reconnect decoder. The canonical crash field may be newer
 * than the traffic envelope after combat/responder acknowledgement. Reject a
 * stale/mismatched canonical field, never initialize a new fleet on read. */
export function resumeCompanionStreetCrashTraffic(scope: CompanionStreetCrashScope,
  trafficJson: string, canonicalCrashesJson: string): CompanionStreetCrashTrafficState | null {
  const record = (value: unknown): value is Record<string, unknown> => !!value && typeof value === "object" && !Array.isArray(value);
  const count = (value: unknown, limit: number): value is unknown[] => Array.isArray(value) && value.length <= limit;
  const timestamp = (value: unknown): value is number => typeof value === "number" && time(value);
  const number = (value: unknown): value is number => typeof value === "number" && Number.isFinite(value);
  const string = (value: unknown): value is string => typeof value === "string" && key(value);
  const nullableTime = (value: unknown) => value === null || timestamp(value);
  const validScope = (value: unknown): value is CompanionStreetCrashScope => record(value)
    && [value.worldKey, value.epochKey, value.gameRoomId, value.instanceId, value.layoutRevision].every(string)
    && timestamp(value.sharedStartedAt) && sameCompanionStreetCrashScope(value as CompanionStreetCrashScope, scope);
  const pose = (value: unknown): value is CompanionStreetVehiclePose => record(value) && string(value.id)
    && Array.isArray(value.point) && value.point.length === 3 && value.point.every(number)
    && number(value.yaw) && number(value.halfWidth) && number(value.halfLength)
    && validCompanionStreetVehiclePose(value as CompanionStreetVehiclePose);
  const contactId = (value: unknown): value is string => typeof value === "string" && !!value && value.length <= 1600;
  const validCrashes = (value: unknown): value is CompanionStreetCrashState => {
    if (!record(value) || !validScope(value.scope) || !timestamp(value.observedAt) || value.observedAt < scope.sharedStartedAt
      || !timestamp(value.revision) || !count(value.fleet, 16) || !count(value.incidents, 2)) return false;
    if (value.fleet.some(item => !record(item) || !string(item.id) || !number(item.lastSequence) || !Number.isSafeInteger(item.lastSequence)
      || item.lastSequence < -1 || !timestamp(item.nextCrashAt))
      || new Set(value.fleet.map(item => (item as { id: string }).id)).size !== value.fleet.length) return false;
    for (const incident of value.incidents) {
      if (!record(incident) || !contactId(incident.id) || !Array.isArray(incident.point) || incident.point.length !== 3 || !incident.point.every(number)
        || !timestamp(incident.occurredAt) || incident.occurredAt < scope.sharedStartedAt || incident.occurredAt > value.observedAt
        || !timestamp(incident.expiresAt) || incident.expiresAt !== incident.occurredAt + companionStreetCrashTuning.expiresMs
        || !timestamp(incident.revision) || !nullableTime(incident.clearedAt)
        || !count(incident.vehicles, 2) || incident.vehicles.length !== 2 || !incident.vehicles.every(pose)
        || incident.vehicles[0].id === incident.vehicles[1].id
        || incident.vehicles.some(vehicle => !(value.fleet as { id: string }[]).some(item => item.id === vehicle.id))
        || !count(incident.responders, 2) || incident.responders.length !== 2) return false;
      for (let index = 0; index < 2; index++) {
        const responder = incident.responders[index];
        if (!record(responder) || responder.kind !== (index === 0 ? "ambulance" : "police") || !timestamp(responder.requestedAt)
          || responder.requestedAt !== incident.occurredAt + (index === 0 ? companionStreetCrashTuning.ambulanceDispatchMs : companionStreetCrashTuning.policeDispatchMs)
          || !(responder.actorId === null || string(responder.actorId)) || !nullableTime(responder.dispatchedAt) || !nullableTime(responder.arrivedAt)
          || (responder.actorId === null) !== (responder.dispatchedAt === null)
          || responder.dispatchedAt !== null && ((responder.dispatchedAt as number) < responder.requestedAt || (responder.dispatchedAt as number) > value.observedAt)
          || responder.arrivedAt !== null && (responder.dispatchedAt === null || (responder.arrivedAt as number) < (responder.dispatchedAt as number)
            || (responder.arrivedAt as number) > value.observedAt)) return false;
      }
      const robbery = incident.robbery;
      if (robbery !== null && (!record(robbery) || !string(robbery.npcId) || !timestamp(robbery.availableAt)
        || robbery.availableAt !== incident.occurredAt + companionStreetCrashTuning.opportunistDelayMs
        || !(robbery.encounterId === null || string(robbery.encounterId)) || !nullableTime(robbery.startedAt) || !nullableTime(robbery.resolvedAt)
        || (robbery.encounterId === null) !== (robbery.startedAt === null)
        || robbery.startedAt !== null && ((robbery.startedAt as number) < robbery.availableAt || (robbery.startedAt as number) >= incident.expiresAt
          || (robbery.startedAt as number) > value.observedAt)
        || robbery.resolvedAt !== null && (robbery.startedAt === null || (robbery.resolvedAt as number) < (robbery.startedAt as number)
          || (robbery.resolvedAt as number) > value.observedAt)
        || !(robbery.outcome === null || ["escaped", "repelled", "surrendered", "defeated", "interrupted", "expired"].includes(robbery.outcome as string))
        || (robbery.resolvedAt === null) !== (robbery.outcome === null))) return false;
      const recovery = incident.recovery;
      const incidentVehicles = incident.vehicles, incidentResponders = incident.responders;
      if (incident.clearedAt === null) { if (recovery !== null) return false; }
      else {
        if (!record(recovery) || !string(recovery.receiptId) || !count(recovery.vehicleIds, 2)
          || recovery.vehicleIds.length !== 2 || !recovery.vehicleIds.every(string)
          || !count(recovery.releasedActorIds, 2) || !recovery.releasedActorIds.every(string)) return false;
        const vehicleIds = recovery.vehicleIds, actorIds = recovery.releasedActorIds;
        if (new Set(vehicleIds).size !== 2 || incidentVehicles.some(vehicle => !vehicleIds.includes(vehicle.id))
          || new Set(actorIds).size !== actorIds.length || incidentResponders.some(responder => record(responder)
            && string(responder.actorId) && !actorIds.includes(responder.actorId))
          || actorIds.some(id => !incidentResponders.some(responder => record(responder) && responder.actorId === id))) return false;
      }
      if (incident.clearedAt !== null && ((incident.clearedAt as number) < incident.occurredAt || (incident.clearedAt as number) > value.observedAt
        || record(robbery) && robbery.encounterId !== null && robbery.resolvedAt === null)) return false;
    }
    return new Set(value.incidents.map(item => (item as { id: string }).id)).size === value.incidents.length;
  };
  try {
    if (typeof trafficJson !== "string" || typeof canonicalCrashesJson !== "string" || new TextEncoder().encode(trafficJson).length > 32_768
      || canonicalCrashesJson.length > 64_000) return null;
    const parsed: unknown = JSON.parse(trafficJson), crashes: unknown = JSON.parse(canonicalCrashesJson);
    // Pre-recovery source records retain their obstruction. Missing evidence
    // becomes null only for uncleared incidents, never a clearance receipt.
    for (const value of [record(parsed) ? parsed.crashes : null, crashes]) {
      if (record(value) && count(value.incidents, 2)) for (const incident of value.incidents) {
        if (record(incident) && incident.clearedAt === null && incident.recovery === undefined) incident.recovery = null;
      }
    }
    if (!record(parsed) || !validScope(parsed.scope) || !timestamp(parsed.observedAt) || parsed.observedAt < scope.sharedStartedAt
      || !timestamp(parsed.revision) || !count(parsed.vehicles, 16) || !validCrashes(parsed.crashes) || !validCrashes(crashes)) return null;
    for (const vehicle of parsed.vehicles) {
      if (!record(vehicle) || !pose(vehicle.pose) || !number(vehicle.sequence) || !Number.isSafeInteger(vehicle.sequence) || vehicle.sequence < -1
        || !["ambient", "player", "resident", "responder"].includes(vehicle.owner as string)
        || !number(vehicle.speed) || vehicle.speed < 0 || vehicle.speed > 35 || typeof vehicle.parked !== "boolean"
        || vehicle.hold !== null && (!record(vehicle.hold) || !contactId(vehicle.hold.contactId) || !timestamp(vehicle.hold.stoppedAt)
          || vehicle.hold.stoppedAt < scope.sharedStartedAt || vehicle.hold.stoppedAt > parsed.observedAt)
        || (vehicle.hold !== null || vehicle.parked) && vehicle.speed !== 0) return null;
    }
    const state = parsed as unknown as CompanionStreetCrashTrafficState;
    if (new Set(state.vehicles.map(vehicle => vehicle.pose.id)).size !== state.vehicles.length
      || state.crashes.fleet.length !== state.vehicles.length
      || state.crashes.fleet.some(item => !state.vehicles.some(vehicle => vehicle.pose.id === item.id && vehicle.sequence >= item.lastSequence))
      || crashes.fleet.some(item => !state.vehicles.some(vehicle => vehicle.pose.id === item.id && vehicle.sequence >= item.lastSequence))
      || crashes.incidents.some(incident => incident.clearedAt === null && incident.vehicles.some(pose => !state.vehicles.some(vehicle =>
        vehicle.pose.id === pose.id && vehicle.hold && samePoint(vehicle.pose.point, pose.point)
        && vehicle.pose.yaw === pose.yaw && vehicle.pose.halfWidth === pose.halfWidth && vehicle.pose.halfLength === pose.halfLength)))) return null;
    const resumed = observeCompanionStreetCrashTrafficState(state, crashes);
    if (!resumed) return null;
    // Public presentation includes only the defined receipt fields.
    const publicPose = ({ id, point, yaw, halfWidth, halfLength }: CompanionStreetVehiclePose): CompanionStreetVehiclePose =>
      ({ id, point: [point[0], point[1], point[2]], yaw, halfWidth, halfLength });
    const publicResponder = ({ kind, requestedAt, actorId, dispatchedAt, arrivedAt }: CompanionStreetCrashState["incidents"][number]["responders"][number]) =>
      ({ kind, requestedAt, actorId, dispatchedAt, arrivedAt });
    const publicCrashes: CompanionStreetCrashState = {
      scope: { worldKey: scope.worldKey, epochKey: scope.epochKey, gameRoomId: scope.gameRoomId,
        instanceId: scope.instanceId, layoutRevision: scope.layoutRevision, sharedStartedAt: scope.sharedStartedAt },
      observedAt: resumed.crashes.observedAt, revision: resumed.crashes.revision,
      fleet: resumed.crashes.fleet.map(({ id, lastSequence, nextCrashAt }) => ({ id, lastSequence, nextCrashAt })),
      incidents: resumed.crashes.incidents.map(incident => ({ id: incident.id, point: [...incident.point],
        occurredAt: incident.occurredAt, expiresAt: incident.expiresAt, revision: incident.revision, clearedAt: incident.clearedAt,
        vehicles: [publicPose(incident.vehicles[0]), publicPose(incident.vehicles[1])],
        responders: [publicResponder(incident.responders[0]), publicResponder(incident.responders[1])],
        robbery: incident.robbery ? { npcId: incident.robbery.npcId, availableAt: incident.robbery.availableAt,
          encounterId: incident.robbery.encounterId, startedAt: incident.robbery.startedAt,
          resolvedAt: incident.robbery.resolvedAt, outcome: incident.robbery.outcome } : null,
        recovery: incident.recovery ? { receiptId: incident.recovery.receiptId, vehicleIds: [...incident.recovery.vehicleIds],
          releasedActorIds: [...incident.recovery.releasedActorIds] } : null })),
    };
    const publicState: CompanionStreetCrashTrafficState = { scope: publicCrashes.scope, observedAt: resumed.observedAt,
      revision: resumed.revision, crashes: publicCrashes,
      vehicles: resumed.vehicles.map(({ pose, sequence, owner, speed, parked, hold }) => ({
        pose: publicPose(pose),
        sequence, owner, speed, parked, hold: hold ? { contactId: hold.contactId, stoppedAt: hold.stoppedAt } : null })) };
    // Freeze only these small decoded records, never a borrowed world layout.
    const deepFreeze = (value: unknown): void => {
      if (!value || typeof value !== "object") return;
      Object.values(value).forEach(deepFreeze); Object.freeze(value);
    };
    deepFreeze(publicState); return publicState;
  } catch { return null; }
}

/** No timer, random fleet reset or movement simulation. Initialize once from
 * the movement owner's stable instance fleet, then persist this bounded row. */
export function createCompanionStreetCrashTrafficState(scope: CompanionStreetCrashScope,
  fleet: readonly Readonly<{ pose: CompanionStreetVehiclePose; owner: CompanionStreetTrafficVehicle["owner"]; parked?: boolean }>[]): CompanionStreetCrashTrafficState {
  const crashes = createCompanionStreetCrashState(scope, fleet.map(vehicle => vehicle.pose.id));
  if (fleet.some(vehicle => !validCompanionStreetVehiclePose(vehicle.pose)
    || !["ambient", "player", "resident", "responder"].includes(vehicle.owner))) throw new Error("Use reviewed dimensions and the actual movement owner for every fleet ID.");
  return freeze({ scope, observedAt: scope.sharedStartedAt, revision: 0, crashes,
    vehicles: [...fleet].sort((a, b) => a.pose.id < b.pose.id ? -1 : 1).map(vehicle => ({ pose: vehicle.pose,
      owner: vehicle.owner, parked: vehicle.parked ?? false, sequence: -1, speed: 0, hold: null })) });
}

/** Conservative swept disc against a translating fixed-yaw rectangle. The
 * square-expanded corners may brake early but never skip a participant contact. */
function participantConflict(sweep: CompanionStreetVehicleSweep, participant: CompanionStreetTrafficParticipant) {
  const c = Math.cos(sweep.yaw), s = Math.sin(sweep.yaw);
  const dx = participant.from[0] - sweep.from[0], dz = participant.from[2] - sweep.from[2];
  const vx = participant.point[0] - participant.from[0] - sweep.point[0] + sweep.from[0];
  const vz = participant.point[2] - participant.from[2] - sweep.point[2] + sweep.from[2];
  let enter = 0, leave = 1;
  for (const [origin, delta, extent] of [[dx * c - dz * s, vx * c - vz * s, sweep.halfWidth + participant.radius + .2],
    [dx * s + dz * c, vx * s + vz * c, sweep.halfLength + participant.radius + .2]]) {
    if (Math.abs(delta) < 1e-9) { if (Math.abs(origin) > extent) return false; continue; }
    const a = (-extent - origin) / delta, b = (extent - origin) / delta;
    enter = Math.max(enter, Math.min(a, b)); leave = Math.min(leave, Math.max(a, b));
    if (enter > leave) return false;
  }
  return leave >= 0 && enter <= 1;
}

export type CompanionStreetCrashTrafficResult = Readonly<{
  state: CompanionStreetCrashTrafficState; changed: boolean; error: string | null;
  declinedIncidentIds: readonly string[]; participantBlockedIds: readonly string[];
}>;
const rejected = (state: CompanionStreetCrashTrafficState, error: string): CompanionStreetCrashTrafficResult =>
  ({ state, changed: false, error, declinedIncidentIds: [], participantBlockedIds: [] });

/** Called inside the accepted movement transaction. The caller re-derives all
 * paths, timestamps and NPC availability; nothing here admits client evidence.
 * Commit EVERY returned pose/hold, including declined incidents and pileups.
 * A rejected result accepts no proposed movement. Stationary fleet members must
 * be included so stopped cars participate in the same continuous sweep. */
export function acceptCompanionStreetCrashTrafficWindow(state: CompanionStreetCrashTrafficState,
  scope: CompanionStreetCrashScope, sweeps: readonly CompanionStreetVehicleSweep[],
  participants: readonly CompanionStreetTrafficParticipant[] | null, now: number,
  availableOpportunistNpcId: string | null = null): CompanionStreetCrashTrafficResult {
  if (!sameCompanionStreetCrashScope(scope, state.scope) || !time(now) || now < state.observedAt || now < state.crashes.observedAt
    || !sweeps.length || sweeps.length !== state.vehicles.length || sweeps.length > companionStreetCrashTuning.vehicles
    || !participants || participants.length > 64 || new Set(participants.map(person => person.id)).size !== participants.length
    || participants.some(person => !key(person.id) || !point(person.from) || !point(person.point)
      || !Number.isFinite(person.radius) || person.radius < 0 || person.radius > 4)
    || availableOpportunistNpcId !== null && !key(availableOpportunistNpcId)) return rejected(state, "Use the exact fleet scope and a complete accepted participant window.");
  const first = sweeps[0];
  if (!time(first.startsAt) || !time(first.endsAt) || first.startsAt < state.observedAt || first.endsAt > now
    || now - first.endsAt > companionStreetCrashTuning.admissionWindowMs) return rejected(state, "Use a fresh nondecreasing accepted movement window.");
  for (const sweep of sweeps) {
    const prior = state.vehicles.find(vehicle => vehicle.pose.id === sweep.id);
    if (!prior || !point(sweep.from) || !validCompanionStreetVehiclePose(sweep)
      || sweep.instanceId !== scope.instanceId || sweep.layoutRevision !== scope.layoutRevision
      || sweep.startsAt !== first.startsAt || sweep.endsAt !== first.endsAt
      || !Number.isSafeInteger(sweep.sequence) || sweep.sequence <= prior.sequence || !samePoint(sweep.from, prior.pose.point)
      || sweep.halfWidth !== prior.pose.halfWidth || sweep.halfLength !== prior.pose.halfLength
      // Rotation has its own checked turn boundary. A translated sweep cannot
      // change the accepted orientation or rotate a retained crashed car.
      || Math.abs(Math.atan2(Math.sin(sweep.yaw - prior.pose.yaw), Math.cos(sweep.yaw - prior.pose.yaw))) > 1e-6)
      return rejected(state, "Resume each vehicle's accepted pose, dimensions, yaw and movement sequence.");
  }
  const participantBlockedIds: string[] = [];
  const proposed = sweeps.map(sweep => {
    const prior = state.vehicles.find(vehicle => vehicle.pose.id === sweep.id)!;
    if (prior.hold || prior.parked) return { ...sweep, point: prior.pose.point };
    if (participants.some(person => participantConflict(sweep, person))) {
      participantBlockedIds.push(sweep.id); return { ...sweep, point: sweep.from };
    }
    return sweep;
  });
  const resolved = resolveCompanionStreetVehicleSweeps(proposed);
  if (resolved.error) return rejected(state, resolved.error);
  // If braking leaves a body in the participant's already-accepted path, the
  // owner must reject/replan that participant path too. Do not hide penetration.
  if (resolved.vehicles.some(vehicle => participants.some(person => participantConflict({ ...first, ...vehicle.pose,
    from: vehicle.pose.point, point: vehicle.pose.point }, person)))) return rejected(state, "Replan the participant path around the stopped vehicle before accepting this window.");
  let crashes = state.crashes;
  const declinedIncidentIds: string[] = [];
  for (const contact of resolved.contacts) {
    if (!contact.incidentEligible) continue;
    const admitted = admitCompanionStreetCrash(crashes, contact.collision, now, availableOpportunistNpcId);
    crashes = admitted.state;
    if (!admitted.changed) declinedIncidentIds.push(contact.collision.id);
  }
  const vehicles = resolved.vehicles.map(result => {
    const prior = state.vehicles.find(vehicle => vehicle.pose.id === result.pose.id)!;
    const sweep = sweeps.find(item => item.id === result.pose.id)!;
    const heldContact = resolved.contacts.find(contact => contact.collision.vehicles.some(pose => pose.id === result.pose.id));
    const hold = prior.hold ?? (heldContact ? { contactId: result.contactId ?? heldContact.collision.id,
      stoppedAt: result.stoppedAt ?? heldContact.collision.occurredAt } : null);
    return { ...prior, pose: result.pose, sequence: result.sequence, hold,
      speed: hold || prior.parked || participantBlockedIds.includes(result.pose.id) ? 0
        : Math.hypot(result.pose.point[0] - sweep.from[0], result.pose.point[2] - sweep.from[2]) / (sweep.endsAt - sweep.startsAt) * 1000 };
  });
  return { state: freeze({ ...state, crashes, vehicles, observedAt: first.endsAt, revision: state.revision + 1 }),
    changed: true, error: null, declinedIncidentIds: Object.freeze(declinedIncidentIds), participantBlockedIds: Object.freeze(participantBlockedIds) };
}

export type CompanionStreetTrafficTurn = Readonly<{
  id: string; sequence: number; yaw: number;
}>;
/** A rotation boundary has no translation. The circumscribed body disc checks
 * every intermediate yaw conservatively against current cars, participants and
 * fixed road geometry. Tight turns return null and need a wider authored route.
 * The caller supplies complete, admitted obstacles for this exact layout. */
export function turnCompanionStreetCrashTraffic(state: CompanionStreetCrashTrafficState,
  scope: CompanionStreetCrashScope, turn: CompanionStreetTrafficTurn, participants: readonly CompanionStreetTrafficParticipant[] | null,
  obstacles: readonly NavigationObstacle[], now: number): CompanionStreetCrashTrafficState | null {
  const vehicle = state.vehicles.find(item => item.pose.id === turn.id);
  if (!sameCompanionStreetCrashScope(scope, state.scope) || !vehicle || vehicle.hold || vehicle.parked
    || !time(now) || now < state.observedAt || now < state.crashes.observedAt
    || !Number.isSafeInteger(turn.sequence) || turn.sequence <= vehicle.sequence
    || !Number.isFinite(turn.yaw) || !participants || participants.length > 64 || obstacles.length > 512
    || new Set(participants.map(person => person.id)).size !== participants.length
    || participants.some(person => !key(person.id) || !point(person.from) || !point(person.point) || !Number.isFinite(person.radius) || person.radius < 0 || person.radius > 4)
    || obstacles.some(box => ![box.minX, box.maxX, box.minZ, box.maxZ, box.yaw ?? 0].every(Number.isFinite)
      || box.minX > box.maxX || box.minZ > box.maxZ || !!box.polygon)) return null;
  const radius = Math.hypot(vehicle.pose.halfWidth, vehicle.pose.halfLength) + .2;
  const intersectsDisc = (pose: CompanionStreetVehiclePose) => {
    const dx = vehicle.pose.point[0] - pose.point[0], dz = vehicle.pose.point[2] - pose.point[2];
    const c = Math.cos(pose.yaw), s = Math.sin(pose.yaw);
    return Math.hypot(Math.max(0, Math.abs(dx * c - dz * s) - pose.halfWidth),
      Math.max(0, Math.abs(dx * s + dz * c) - pose.halfLength)) <= radius;
  };
  if (state.vehicles.some(other => other !== vehicle && intersectsDisc(other.pose))
    || obstacles.some(box => intersectsDisc({ id: "geometry", point: [(box.minX + box.maxX) / 2, vehicle.pose.point[1], (box.minZ + box.maxZ) / 2],
      yaw: box.yaw ?? 0, halfWidth: (box.maxX - box.minX) / 2, halfLength: (box.maxZ - box.minZ) / 2 }))) return null;
  for (const person of participants) {
    const dx = person.point[0] - person.from[0], dz = person.point[2] - person.from[2], length = dx * dx + dz * dz;
    const fraction = length ? Math.max(0, Math.min(1, ((vehicle.pose.point[0] - person.from[0]) * dx
      + (vehicle.pose.point[2] - person.from[2]) * dz) / length)) : 0;
    if (Math.hypot(person.from[0] + fraction * dx - vehicle.pose.point[0], person.from[2] + fraction * dz - vehicle.pose.point[2]) <= radius + person.radius) return null;
  }
  return freeze({ ...state, observedAt: now, revision: state.revision + 1,
    vehicles: state.vehicles.map(item => item === vehicle ? { ...item, pose: { ...item.pose, yaw: turn.yaw }, sequence: turn.sequence, speed: 0 } : item) });
}

/** After the responder/combat owner updates its receipt in the same world row. */
export function observeCompanionStreetCrashTrafficState(state: CompanionStreetCrashTrafficState,
  crashes: CompanionStreetCrashState): CompanionStreetCrashTrafficState | null {
  if (!sameCompanionStreetCrashScope(state.scope, crashes.scope) || crashes.revision < state.crashes.revision
    || crashes.observedAt < state.crashes.observedAt || crashes.fleet.length !== state.vehicles.length
    || crashes.revision === state.crashes.revision && JSON.stringify(crashes) !== JSON.stringify(state.crashes)
    || crashes.fleet.some(vehicle => !state.vehicles.some(item => item.pose.id === vehicle.id))) return null;
  if (crashes.revision === state.crashes.revision) return state;
  return freeze({ ...state, crashes, revision: state.revision + 1 });
}

export type CompanionStreetTrafficRecovery = Readonly<{
  receiptId: string; now: number;
  vehicles: readonly Readonly<{ id: string; contactId: string; sequence: number; acceptedPose: CompanionStreetVehiclePose;
    nextSequence: number;
    /** Only decorative traffic can resume its route after accepted recovery. */
    resumeAmbientRoute: boolean }>[];
}>;

/** No ordinary expiry/reset can call this. Acknowledge actual vehicle recovery,
 * with final poses checked by the existing road/navigation owner. Player and
 * resident vehicles remain parked in their saved roadside locations. */
export function recoverCompanionStreetCrashTraffic(state: CompanionStreetCrashTrafficState,
  recovery: CompanionStreetTrafficRecovery): CompanionStreetCrashTrafficState | null {
  if (!key(recovery.receiptId) || !time(recovery.now) || recovery.now < state.observedAt || recovery.now < state.crashes.observedAt || !recovery.vehicles.length
    || recovery.vehicles.length > companionStreetCrashTuning.vehicles
    || new Set(recovery.vehicles.map(vehicle => vehicle.id)).size !== recovery.vehicles.length) return null;
  for (const recovered of recovery.vehicles) {
    const vehicle = state.vehicles.find(item => item.pose.id === recovered.id);
    if (!vehicle?.hold || vehicle.hold.contactId !== recovered.contactId || recovered.sequence !== vehicle.sequence
      || !Number.isSafeInteger(recovered.nextSequence) || recovered.nextSequence <= vehicle.sequence
      || !validCompanionStreetVehiclePose(recovered.acceptedPose) || recovered.acceptedPose.id !== recovered.id
      || recovered.acceptedPose.halfWidth !== vehicle.pose.halfWidth || recovered.acceptedPose.halfLength !== vehicle.pose.halfLength
      || recovered.resumeAmbientRoute && vehicle.owner !== "ambient"
      || vehicle.owner !== "ambient" && !samePoint(recovered.acceptedPose.point, vehicle.pose.point)
      || state.crashes.incidents.some(incident => incident.clearedAt === null && incident.vehicles.some(pose => pose.id === recovered.id))) return null;
  }
  return freeze({ ...state, observedAt: recovery.now, revision: state.revision + 1,
    vehicles: state.vehicles.map(vehicle => {
      const recovered = recovery.vehicles.find(item => item.id === vehicle.pose.id);
      return recovered ? { ...vehicle, pose: recovered.acceptedPose, sequence: recovered.nextSequence,
        hold: null, speed: 0, parked: !recovered.resumeAmbientRoute } : vehicle;
    }) });
}

/** Late join/reconnect projection. Due owner requests never mutate the fleet.
 * Every held/parked car obstructs navigation, including over-cap pileups. */
export function projectCompanionStreetCrashTraffic(state: CompanionStreetCrashTrafficState, now: number): CompanionStreetCrashTrafficView | null {
  if (!time(now) || now < state.observedAt) return null;
  const crashes = projectCompanionStreetCrashes(state.crashes, now);
  if (!crashes) return null;
  return Object.freeze({ scope: state.scope, observedAt: now, revision: state.revision, vehicles: state.vehicles,
    obstructions: Object.freeze(state.vehicles.filter(vehicle => vehicle.hold || vehicle.parked).map(vehicle => companionStreetCrashFootprint(vehicle.pose))), crashes });
}
