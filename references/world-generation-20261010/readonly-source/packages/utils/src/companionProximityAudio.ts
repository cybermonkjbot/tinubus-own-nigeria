import type { CompanionCallRoom, CompanionPosition } from "./companionCall";
import type { CompanionMovement } from "./companionMovement";
import { CALL_MOVEMENT_MAX_POINTS, sampleCompanionMovement } from "./companionMovement";
import { footballLimits, type FootballSnapshot } from "./companionFootball";
import type { CompanionPlaceLayout } from "./companionPlaces";
import { footballNextAdvanceAt } from "./companionFootballEngine";
import { footballPitchFromPlace } from "./companionFootballPlaces";

export const COMPANION_FULL_VOLUME_METRES = 2;
export const COMPANION_HEARING_RADIUS_METRES = 12;

export type CompanionAudioPose = { x: number; y?: number; z: number; yaw: number };
export type CompanionSpatialAudioMix = {
  /** An authorized shared place instance or the current call-room revision. */
  scopeId: string;
  listener: CompanionAudioPose;
  remotes: Array<{ uid: number; position: CompanionAudioPose; level: number }>;
};
export type CompanionSoundZone = {
  id: string;
  center: { x: number; z: number };
  halfWidth: number;
  halfDepth: number;
  yaw: number;
  /** Local playback only, never a privacy/access grant. */
  boundaryGain: number;
  /** Optional world instance binding; coordinates from another place must not apply. */
  instanceId?: string;
};
export type CompanionListenerPose = {
  callSessionId: string;
  roomRevision: number;
  pose: CompanionAudioPose;
  measuredAt: number;
};
export type CompanionPlaceAudioFrame = {
  callSessionId: string;
  roomRevision: number;
  /** Null during travel or before the destination has audio admission. */
  instanceId: string | null;
  listener: CompanionAudioPose | null;
  participants: Array<{ userId: string; instanceId: string; position: CompanionAudioPose }>;
  measuredAt: number;
};
export type CompanionPlaceAudioSource = (context: {
  room: CompanionCallRoom; localUid: number; now: number; listenerPose: CompanionListenerPose | null;
}) => CompanionPlaceAudioFrame | null;
export type CompanionAudioWorldPlayer = {
  userId: string; gameRoomId: string; instanceId: string; revision: number;
  status: "present" | "traveling" | "recovering";
  position: CompanionPosition; movement: CompanionMovement | null;
};
/** Read-only input from the existing world owner, not a second world snapshot or store. */
export type CompanionAudioWorld = {
  active: boolean; ready: boolean; error: string | null; serverOffset: number;
  /** Continuing hearing admission is independent of interactive renderer visibility. */
  audioInstanceId?: string | null;
  renderedInstanceId: string | null;
  instance: { id: string; navigation: { floorY: number }; layout?: CompanionPlaceLayout | null } | null;
  player: CompanionAudioWorldPlayer | null; peers: readonly CompanionAudioWorldPlayer[];
};
export type CompanionSpatialCallTransport = {
  setSpatialAudioMix(mix: CompanionSpatialAudioMix | null): void;
};
export type CompanionSpatialCallController = {
  updateListenerPose(value: CompanionListenerPose | null): void;
  updatePlaceAudio(value: CompanionPlaceAudioFrame | null): void;
  /** Sample world state inside the existing hearing timer instead of adding another timer. */
  setPlaceAudioSource(source: CompanionPlaceAudioSource | null): void;
  setSoundZones(value: readonly CompanionSoundZone[]): void;
};

export function validCompanionAudioPose(value: CompanionAudioPose): boolean {
  return [value.x, value.y ?? 0, value.z, value.yaw].every(Number.isFinite);
}

/** Renderer yaw 0 faces +Z. The right axis is -X in this orientation. */
export function companionRelativeAudioPosition(listener: CompanionAudioPose, source: CompanionAudioPose) {
  const dx = source.x - listener.x, dz = source.z - listener.z;
  return {
    right: -dx * Math.cos(listener.yaw) + dz * Math.sin(listener.yaw),
    up: (source.y ?? 0) - (listener.y ?? 0),
    forward: dx * Math.sin(listener.yaw) + dz * Math.cos(listener.yaw),
  };
}

export function companionDistanceAudioGain(distance: number): number {
  if (!Number.isFinite(distance) || distance < 0) return 0;
  const fade = Math.max(0, Math.min(1, (COMPANION_HEARING_RADIUS_METRES - distance)
    / (COMPANION_HEARING_RADIUS_METRES - COMPANION_FULL_VOLUME_METRES)));
  return fade * fade * (3 - 2 * fade);
}

export function validCompanionSoundZone(zone: CompanionSoundZone): boolean {
  return !!zone.id && [zone.center.x, zone.center.z, zone.halfWidth, zone.halfDepth, zone.yaw, zone.boundaryGain].every(Number.isFinite)
    && zone.halfWidth > 0 && zone.halfDepth > 0 && zone.boundaryGain >= 0 && zone.boundaryGain <= 1
    && (zone.instanceId === undefined || typeof zone.instanceId === "string" && zone.instanceId.length > 0);
}

/** Copy, validate and deduplicate layout-owned boundaries without deriving zones from activity areas. */
export function companionValidatedSoundZones(zones: readonly CompanionSoundZone[], instanceId?: string): CompanionSoundZone[] {
  const result: CompanionSoundZone[] = [], ids = new Set<string>();
  for (const zone of zones.slice(0, 32)) {
    if (!validCompanionSoundZone(zone) || ids.has(zone.id) || instanceId !== undefined && zone.instanceId && zone.instanceId !== instanceId) continue;
    ids.add(zone.id);
    result.push({ ...zone, center: { ...zone.center }, ...(instanceId ? { instanceId } : {}) });
  }
  return result;
}

function zoneContains(zone: CompanionSoundZone, position: CompanionAudioPose): boolean {
  const dx = position.x - zone.center.x, dz = position.z - zone.center.z;
  return Math.abs(dx * Math.cos(zone.yaw) - dz * Math.sin(zone.yaw)) <= zone.halfWidth
    && Math.abs(dx * Math.sin(zone.yaw) + dz * Math.cos(zone.yaw)) <= zone.halfDepth;
}

/** One shared gain calculation for native and browser sound-zone boundaries. */
export function companionSoundZoneGain(listener: CompanionAudioPose, source: CompanionAudioPose,
  zones: readonly CompanionSoundZone[]): number {
  if (!validCompanionAudioPose(listener) || !validCompanionAudioPose(source)) return 0;
  let gain = 1;
  const seen = new Set<string>();
  for (const zone of zones.slice(0, 32)) {
    if (validCompanionSoundZone(zone) && !seen.has(zone.id)) {
      seen.add(zone.id);
      if (zoneContains(zone, listener) !== zoneContains(zone, source)) gain *= zone.boundaryGain;
    }
  }
  return gain;
}

export function companionSilentPlaceAudioFrame(room: CompanionCallRoom, now: number): CompanionPlaceAudioFrame {
  return { callSessionId: room.callSessionId ?? "", roomRevision: room.roomRevision, measuredAt: now,
    instanceId: null, listener: null, participants: [] };
}

function worldAudioPosition(person: CompanionAudioWorldPlayer, sharedNow: number): CompanionAudioPose | null {
  if (!validCompanionAudioPose(person.position)) return null;
  const route = person.movement;
  if (!route) return person.position;
  if (route.version !== 1 || route.roomRevision !== person.revision || !Number.isFinite(route.startedAt)
    || route.initialSpeed !== undefined && (!Number.isFinite(route.initialSpeed) || route.initialSpeed < 0)
    || !Number.isFinite(route.yaw) || !validCompanionAudioPose(route.from) || !Array.isArray(route.path)
    || route.path.length > CALL_MOVEMENT_MAX_POINTS || route.path.some(point => !Number.isFinite(point.x) || !Number.isFinite(point.z))) return null;
  const position = sampleCompanionMovement(route, sharedNow).position;
  return validCompanionAudioPose(position) ? position : null;
}

/** Match poses replace only admitted human roster positions, never world or RTC membership. */
function footballAudioPositions(room: CompanionCallRoom, world: CompanionAudioWorld,
  football: FootballSnapshot | null | undefined, sharedNow: number): Map<string, CompanionAudioPose | null> | null {
  const match = football?.match, instance = world.instance, player = world.player;
  if (!football || !match || !instance || !player || !["playing", "restart"].includes(match.phase)
    || football.callSessionId !== room.callSessionId || football.playerId !== player.userId
    || match.id !== room.gameRoomId || match.pitch.instanceId !== instance.id) return null;
  const positions = new Map<string, CompanionAudioPose | null>();
  const roster = match.players.slice(0, footballLimits.players);
  const humans = roster.filter(person => person.bot === false && person.connected);
  for (const person of humans) positions.set(person.id, null);
  if (!football.ready || football.error || match.players.length > footballLimits.players
    || new Set(roster.map(person => person.id)).size !== roster.length
    || !Number.isSafeInteger(match.epoch) || match.epoch < 1 || !Number.isSafeInteger(match.revision) || match.revision < 1
    || !Number.isFinite(match.simulatedAt) || match.simulatedAt > sharedNow || !instance.layout) return positions;
  try {
    const pitch = footballPitchFromPlace(instance.layout, instance.id, match.pitch.revision);
    if (pitch.id !== match.pitch.id || pitch.floorY !== instance.navigation.floorY || pitch.floorY !== match.pitch.floorY
      || pitch.centre.x !== match.pitch.centre.x || pitch.centre.z !== match.pitch.centre.z
      || pitch.halfWidth !== match.pitch.halfWidth || pitch.halfLength !== match.pitch.halfLength
      || pitch.goalHalfWidth !== match.pitch.goalHalfWidth || pitch.goalHeight !== match.pitch.goalHeight) return positions;
    // A settled match may legitimately await a distant deadline without another snapshot.
    // Moving or restarting matches must not retain positions after their next update is overdue.
    const nextAt = football.advancement ? football.advancement.nextAt : footballNextAdvanceAt(match, [], match.simulatedAt);
    if (nextAt === null || !Number.isFinite(nextAt) || nextAt < match.simulatedAt || sharedNow - nextAt >= 1000) return positions;
    for (const person of humans) {
      const pose = { ...person.position, y: pitch.floorY, yaw: person.yaw };
      if (person.connected && validCompanionAudioPose(pose)
        && Math.abs(pose.x - pitch.centre.x) <= pitch.halfWidth && Math.abs(pose.z - pitch.centre.z) <= pitch.halfLength)
        positions.set(person.id, pose);
    }
  } catch { /* Invalid resolved pitch data keeps the matched humans silent. */ }
  return positions;
}

/** Active but loading/revoked worlds stay silent. Only explicitly inactive world mode returns null. */
export function companionWorldAudioFrame(room: CompanionCallRoom, localUid: number, now: number,
  world: CompanionAudioWorld, listenerPose?: CompanionListenerPose | null,
  football?: FootballSnapshot | null): CompanionPlaceAudioFrame | null {
  if (world.active === false) return null;
  const silent = companionSilentPlaceAudioFrame(room, now);
  const own = room.participants.find(person => person.agoraUid === localUid && person.expiresAt > now);
  const player = world.player, instance = world.instance;
  const admittedInstanceId = world.audioInstanceId === undefined ? world.renderedInstanceId : world.audioInstanceId;
  if (world.active !== true || !Number.isFinite(now) || !Number.isFinite(now + world.serverOffset) || !world.ready || world.error || !player || !instance
    || !own || (room.expiresAt ?? 0) <= now || player.userId !== own.userId || player.gameRoomId !== room.gameRoomId
    || player.status === "traveling" || instance.id !== player.instanceId || admittedInstanceId !== player.instanceId
    || !Number.isFinite(instance.navigation.floorY)) return silent;
  const sharedNow = now + world.serverOffset;
  const footballPositions = footballAudioPositions(room, world, football, sharedNow);
  const positionOf = (person: CompanionAudioWorldPlayer) => footballPositions?.has(person.userId)
    ? person.status === "present" ? footballPositions.get(person.userId) ?? null : null
    : worldAudioPosition(person, sharedNow);
  const origin = positionOf(player);
  if (!origin) return silent;
  const freshListener = listenerPose && listenerPose.callSessionId === room.callSessionId && listenerPose.roomRevision === room.roomRevision
    && Number.isFinite(listenerPose.measuredAt) && listenerPose.measuredAt <= now && now - listenerPose.measuredAt < 1000
    && validCompanionAudioPose(listenerPose.pose);
  const members = new Set(room.participants.filter(person => person.expiresAt > now).map(person => person.userId));
  const seen = new Set<string>();
  return { ...silent, instanceId: instance.id,
    listener: { ...origin, y: instance.navigation.floorY, yaw: freshListener ? listenerPose.pose.yaw : origin.yaw },
    participants: world.peers.slice(0, 50).flatMap(peer => {
      if (seen.has(peer.userId) || !members.has(peer.userId) || peer.gameRoomId !== player.gameRoomId
        || peer.instanceId !== instance.id || peer.status === "traveling") return [];
      seen.add(peer.userId);
      const position = positionOf(peer);
      return position ? [{ userId: peer.userId, instanceId: peer.instanceId, position: { ...position, y: instance.navigation.floorY } }] : [];
    }),
  };
}

function sampledPosition(room: CompanionCallRoom, person: CompanionCallRoom["participants"][number], now: number,
  localMovement?: CompanionMovement | null): CompanionPosition {
  const route = localMovement ?? person.movement;
  return route?.roomRevision === room.roomRevision ? sampleCompanionMovement(route, now).position : person.position;
}

/** Null restores ordinary call audio. Invalid/expired public rooms have an empty managed mix. */
export function companionRoomSpatialAudioMix(room: CompanionCallRoom, localUid: number, now: number,
  localMovement?: CompanionMovement | null, listenerPose?: CompanionListenerPose | null,
  zones: readonly CompanionSoundZone[] = []): CompanionSpatialAudioMix | null {
  if ((room.voiceMode ?? (room.gameRoomId ? "proximity" : "call")) === "call") return null;
  const own = room.participants.find(person => person.agoraUid === localUid && person.expiresAt > now);
  const origin = own ? sampledPosition(room, own, now, localMovement) : { x: 0, z: 0, yaw: 0 };
  // Camera direction may change locally. Never let a camera event move the hearing origin.
  const freshPose = listenerPose && listenerPose.callSessionId === room.callSessionId && listenerPose.roomRevision === room.roomRevision
    && listenerPose.measuredAt <= now && now - listenerPose.measuredAt < 1000 && validCompanionAudioPose(listenerPose.pose);
  const listener = { ...origin, yaw: freshPose ? listenerPose.pose.yaw : origin.yaw };
  const live = !!own && (room.expiresAt ?? 0) > now && validCompanionAudioPose(listener);
  return {
    scopeId: `${room.callSessionId ?? "expired"}:${room.roomRevision}`,
    listener,
    remotes: room.participants.filter(person => person.agoraUid !== localUid).map(person => {
      const position = sampledPosition(room, person, now);
      const level = live && person.expiresAt > now && validCompanionAudioPose(position)
        ? companionDistanceAudioGain(Math.hypot(position.x - origin.x, position.z - origin.z))
          * companionSoundZoneGain(listener, position, zones) : 0;
      return { uid: person.agoraUid, position, level: Math.round(level * 100) / 100 };
    }),
  };
}

/** World membership is supplied by its owner; RTC identity and expiry still come from the call. */
export function companionPlaceSpatialAudioMix(room: CompanionCallRoom, localUid: number, now: number,
  frame: CompanionPlaceAudioFrame, zones: readonly CompanionSoundZone[] = []): CompanionSpatialAudioMix | null {
  if ((room.voiceMode ?? (room.gameRoomId ? "proximity" : "call")) === "call") return null;
  const own = room.participants.find(person => person.agoraUid === localUid && person.expiresAt > now);
  const fresh = frame.callSessionId === room.callSessionId && frame.roomRevision === room.roomRevision
    && Number.isFinite(frame.measuredAt) && frame.measuredAt <= now && now - frame.measuredAt < 1000;
  const listener = frame.listener && validCompanionAudioPose(frame.listener) ? frame.listener : { x: 0, z: 0, yaw: 0 };
  const live = fresh && !!own && (room.expiresAt ?? 0) > now && !!frame.instanceId && !!frame.listener && validCompanionAudioPose(frame.listener);
  const people = new Map(frame.participants.map(person => [person.userId, person]));
  const scopedZones = zones.filter(zone => zone.instanceId === undefined || zone.instanceId === frame.instanceId);
  return {
    scopeId: `${room.callSessionId ?? "expired"}:${room.roomRevision}:${frame.instanceId ?? "travel"}`,
    listener,
    remotes: room.participants.filter(person => person.agoraUid !== localUid).map(person => {
      const peer = people.get(person.userId);
      const position = peer?.position ?? person.position;
      const level = live && person.expiresAt > now && peer?.instanceId === frame.instanceId && validCompanionAudioPose(position)
        ? companionDistanceAudioGain(Math.hypot(position.x - listener.x, position.z - listener.z))
          * companionSoundZoneGain(listener, position, scopedZones) : 0;
      return { uid: person.agoraUid, position, level: Math.round(level * 100) / 100 };
    }),
  };
}

/** Virtual metres only. Group calls retain full volume at any character distance. */
export function companionRemoteAudioLevels(room: CompanionCallRoom, localUid: number, now: number,
  localMovement?: CompanionMovement | null): Array<{ uid: number; level: number }> {
  const own = room.participants.find(person => person.agoraUid === localUid && person.expiresAt > now);
  const live = !!own && (room.expiresAt ?? 0) > now;
  // Older servers already distinguish hosted rooms from real chat calls.
  const mode = room.voiceMode ?? (room.gameRoomId ? "proximity" : "call");
  const position = (person: CompanionCallRoom["participants"][number]): CompanionPosition => {
    const route = person === own ? localMovement ?? person.movement : person.movement;
    return route?.roomRevision === room.roomRevision ? sampleCompanionMovement(route, now).position : person.position;
  };
  const origin = own ? position(own) : null;
  return room.participants.filter(person => person.agoraUid !== localUid).map(person => {
    let level = 0;
    if (live && person.expiresAt > now) {
      if (mode === "call") level = 1;
      else if (mode === "proximity" && origin) {
        const target = position(person);
        const distance = Math.hypot(target.x - origin.x, target.z - origin.z);
        level = companionDistanceAudioGain(distance);
      }
    }
    return { uid: person.agoraUid, level: Math.round(level * 100) / 100 };
  });
}
