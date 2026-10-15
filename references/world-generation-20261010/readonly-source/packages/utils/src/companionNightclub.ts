import type { CharacterCommand } from "./companionExpression";
import type { CompanionPlaceLayout } from "./companionPlaces";
import { routeSegmentClear, type ScenePoint } from "./companionNavigation";
import { evaluateCompanionDance, type CompanionDanceInput, type CompanionDanceRound } from "./companionPlacesInteractions";

export const nightclubLimits = { beats: 24, beatMs: 600, windowMs: 240, requiredHits: 18, countdownMs: 3000 } as const;
export const nightclubDifficulties = {
  easy: { label: "Easy", windowMs: 280, requiredHits: 12 },
  standard: { label: "Standard", windowMs: 240, requiredHits: 18 },
  hard: { label: "Hard", windowMs: 160, requiredHits: 21 },
} as const;
export type NightclubDifficulty = keyof typeof nightclubDifficulties;
export type CompanionNightclubActivity =
  | { kind: "dance"; id: string; visitRevision: number; difficulty?: NightclubDifficulty; round: CompanionDanceRound; inputs: readonly CompanionDanceInput[]; stoppedAt: number | null;
      skillCharacterId?: string; skillsSettled?: boolean }
  | { kind: "lounge"; id: string; visitRevision: number; serviceId: string; joinedAt: number; seatId?: string };
export type CompanionNightclubCommand =
  | { type: "start"; serviceId: string; difficulty?: NightclubDifficulty }
  | { type: "lounge"; serviceId: string }
  | { type: "difficulty"; roundId: string; difficulty: NightclubDifficulty }
  | { type: "sit"; activityId: string }
  | { type: "stand"; activityId: string }
  | { type: "hit"; roundId: string; beatIndex: number; lane: number }
  | { type: "leave"; activityId: string };

type NightclubPlayer = { userId: string; gameRoomId: string; instanceId: string; placeId: string;
  status: string; health: number; revision: number; nightclub?: CompanionNightclubActivity | null };
/** Retain a visit during reconnect, but exclude stale, departed and defeated participants. */
export function nightclubPlayerActivity(player: NightclubPlayer | null | undefined): CompanionNightclubActivity | null {
  const activity = player?.nightclub;
  if (!player || !activity || player.placeId !== "nightclub" || player.status !== "present" || !Number.isFinite(player.health) || player.health <= 0
    || !Number.isSafeInteger(player.revision) || activity.visitRevision !== player.revision
    || activity.kind === "dance" && (activity.round.instanceId !== player.instanceId || activity.round.id !== activity.id)) return null;
  return activity;
}
/** Same-round snapshots can arrive out of order. Owner-received attempts are append-only. */
export function retainNightclubReceivedInputs(previous: CompanionNightclubActivity | null | undefined,
  incoming: CompanionNightclubActivity | null | undefined, userId: string): CompanionNightclubActivity | null | undefined {
  if (previous?.kind !== "dance" || incoming?.kind !== "dance" || previous.id !== incoming.id || previous.visitRevision !== incoming.visitRevision
    || previous.round.instanceId !== incoming.round.instanceId || previous.round.serviceId !== incoming.round.serviceId
    || previous.round.startsAt !== incoming.round.startsAt || previous.round.beatMs !== incoming.round.beatMs
    || previous.round.windowMs !== incoming.round.windowMs || previous.round.requiredHits !== incoming.round.requiredHits
    || previous.round.lanes.length !== incoming.round.lanes.length || previous.round.lanes.some((lane, index) => lane !== incoming.round.lanes[index])) return incoming;
  const inputs = new Map<number, CompanionDanceInput>();
  for (const input of [...incoming.inputs.slice(0, nightclubLimits.beats), ...previous.inputs.slice(0, nightclubLimits.beats)]) {
    if (input.userId !== userId || !Number.isSafeInteger(input.beatIndex) || input.beatIndex < 0 || input.beatIndex >= incoming.round.lanes.length
      || !Number.isSafeInteger(input.lane) || input.lane < 0 || input.lane > 3 || !Number.isFinite(input.receivedAt)) continue;
    const current = inputs.get(input.beatIndex);
    if (!current || input.receivedAt < current.receivedAt || input.receivedAt === current.receivedAt && input.lane < current.lane) inputs.set(input.beatIndex, input);
  }
  if (inputs.size === incoming.inputs.length && incoming.inputs.every(input => inputs.get(input.beatIndex) === input)) return incoming;
  return { ...incoming, inputs: [...inputs.values()].sort((a, b) => a.beatIndex - b.beatIndex).slice(0, nightclubLimits.beats) };
}
export function nightclubLoungeParticipants(player: NightclubPlayer, peers: readonly NightclubPlayer[]) {
  const activity = nightclubPlayerActivity(player);
  if (activity?.kind !== "lounge") return 0;
  return new Set([player, ...peers.slice(0, 13)].filter(peer => {
    const current = nightclubPlayerActivity(peer);
    return peer.gameRoomId === player.gameRoomId && peer.instanceId === player.instanceId
      && current?.kind === "lounge" && current.serviceId === activity.serviceId;
  }).map(peer => peer.userId)).size;
}
export function nightclubRankings(player: NightclubPlayer, peers: readonly NightclubPlayer[], now: number) {
  const dance = nightclubPlayerActivity(player);
  if (dance?.kind !== "dance") return [];
  const own = nightclubProgress(dance, player.userId, dance.stoppedAt === null ? now : Math.min(now, dance.stoppedAt));
  if (!own || own.phase === "invalid" || dance.stoppedAt === null && own.phase === "playing") return [];
  const seen = new Set<string>();
  const scores = [player, ...peers.slice(0, 13)].flatMap((peer, index) => {
    if (seen.has(peer.userId) || peer.gameRoomId !== player.gameRoomId || peer.instanceId !== player.instanceId) return [];
    seen.add(peer.userId);
    const activity = nightclubPlayerActivity(peer);
    if (activity?.kind !== "dance" || activity.round.serviceId !== dance.round.serviceId
      || (activity.difficulty ?? "standard") !== (dance.difficulty ?? "standard")
      || activity.round.windowMs !== dance.round.windowMs || activity.round.requiredHits !== dance.round.requiredHits
      || activity.round.beatMs !== dance.round.beatMs || activity.round.lanes.length !== dance.round.lanes.length) return [];
    const progress = nightclubProgress(activity, peer.userId, activity.stoppedAt === null ? now : Math.min(now, activity.stoppedAt));
    const detail = nightclubScore(activity, peer.userId, now);
    if (!progress || progress.phase === "invalid" || !detail || activity.stoppedAt === null && progress.phase === "playing") return [];
    return [{ userId: peer.userId, label: peer.userId === player.userId ? "You" : `Dancer ${index}`, points: detail.points }];
  }).sort((a, b) => b.points - a.points || a.userId.localeCompare(b.userId));
  return scores.map((score, index) => ({ ...score, rank: scores.findIndex(other => other.points === score.points) + 1, index }))
    .filter(score => score.index < 3 || score.userId === player.userId);
}

export function nightclubProgress(activity: CompanionNightclubActivity | null | undefined, userId: string, now: number) {
  return activity?.kind === "dance" ? evaluateCompanionDance(activity.round, activity.inputs, userId, now) : null;
}
export function nightclubBeatIndex(round: CompanionDanceRound, now: number): number | null {
  if (!Number.isFinite(now) || !Number.isFinite(round.beatMs) || round.beatMs <= 0) return null;
  const index = Math.round((now - round.startsAt) / round.beatMs);
  return index >= 0 && index < round.lanes.length && Math.abs(now - (round.startsAt + index * round.beatMs)) <= round.windowMs ? index : null;
}
export function nightclubDanceActive(activity: CompanionNightclubActivity | null | undefined, now: number) {
  return activity?.kind === "dance" && activity.stoppedAt === null && Number.isFinite(now) && Number.isFinite(activity.round.startsAt)
    && Number.isFinite(activity.round.beatMs) && activity.round.beatMs >= 250 && activity.round.beatMs <= 2000
    && Number.isFinite(activity.round.windowMs) && activity.round.windowMs >= 30 && activity.round.windowMs < activity.round.beatMs / 2
    && activity.round.lanes.length > 0 && activity.round.lanes.length <= nightclubLimits.beats
    && now <= activity.round.startsAt + (activity.round.lanes.length - 1) * activity.round.beatMs + activity.round.windowMs;
}
export function createNightclubDance(id: string, instanceId: string, serviceId: string, visitRevision: number, now: number, sharedStartedAt = now,
  difficulty: NightclubDifficulty = "standard"): Extract<CompanionNightclubActivity, { kind: "dance" }> {
  const settings = nightclubDifficulties[difficulty];
  let seed = 2166136261;
  for (const char of id) seed = Math.imul(seed ^ char.charCodeAt(0), 16777619) >>> 0;
  const lanes = Array.from({ length: nightclubLimits.beats }, (_, index) => {
    seed = (Math.imul(seed, 1664525) + 1013904223) >>> 0;
    // Easy repeats each direction for two beats. Hard never repeats a lane.
    return difficulty === "easy" ? Math.floor(index / 2) % 4
      : difficulty === "hard" ? (index + (seed >>> 16) % 3 + 1) % 4 : ((seed >>> 16) + index) % 4;
  });
  if (difficulty === "hard") for (let index = 1; index < lanes.length; index++)
    if (lanes[index] === lanes[index - 1]) lanes[index] = (lanes[index] + 1) % 4;
  const startsAt = sharedStartedAt + Math.ceil((now + nightclubLimits.countdownMs - sharedStartedAt) / nightclubLimits.beatMs) * nightclubLimits.beatMs;
  return { kind: "dance", id, visitRevision, difficulty, round: { id, instanceId, serviceId, startsAt,
    beatMs: nightclubLimits.beatMs, windowMs: settings.windowMs, lanes, requiredHits: settings.requiredHits }, inputs: [], stoppedAt: null };
}

/** Project only owner-received attempts. Missed and wrong beats break the combo. */
export function nightclubScore(activity: CompanionNightclubActivity | null | undefined, userId: string, now: number) {
  if (activity?.kind !== "dance") return null;
  const at = activity.stoppedAt === null ? now : Math.min(now, activity.stoppedAt);
  const progress = nightclubProgress(activity, userId, at);
  if (!progress || progress.phase === "invalid") return null;
  const attempts = new Map<number, CompanionDanceInput>();
  for (const input of activity.inputs.slice(0, nightclubLimits.beats)) {
    if (input.userId !== userId || !Number.isSafeInteger(input.beatIndex) || input.beatIndex < 0 || input.beatIndex >= activity.round.lanes.length
      || !Number.isFinite(input.receivedAt) || input.receivedAt > at || input.receivedAt < activity.round.startsAt - activity.round.windowMs) continue;
    const old = attempts.get(input.beatIndex);
    if (!old || input.receivedAt < old.receivedAt || input.receivedAt === old.receivedAt && input.lane < old.lane) attempts.set(input.beatIndex, input);
  }
  let combo = 0, bestCombo = 0, perfect = 0, good = 0, misses = 0, points = 0;
  let latest: { beatIndex: number; at: number; rating: "Perfect" | "Good" | "Miss"; timingMs: number | null } | null = null;
  for (let index = 0; index < activity.round.lanes.length; index++) {
    const beatAt = activity.round.startsAt + index * activity.round.beatMs, input = attempts.get(index);
    if (!input && at <= beatAt + activity.round.windowMs) continue;
    const timing = input ? input.receivedAt - beatAt : null;
    const hit = !!input && input.lane === activity.round.lanes[index] && Math.abs(timing!) <= activity.round.windowMs;
    const rating = hit ? Math.abs(timing!) <= Math.min(90, activity.round.windowMs / 2) ? "Perfect" : "Good" : "Miss";
    if (hit) { combo++; bestCombo = Math.max(bestCombo, combo); if (rating === "Perfect") perfect++; else good++; points += (rating === "Perfect" ? 100 : 60) + Math.min(combo - 1, 10) * 5; }
    else { misses++; combo = 0; }
    const eventAt = input?.receivedAt ?? beatAt + activity.round.windowMs;
    if (!latest || eventAt >= latest.at) latest = { beatIndex: index, at: eventAt, rating, timingMs: timing };
  }
  return { combo, bestCombo, perfect, good, misses, points, latest };
}

export type NightclubSeat = { id: string; position: ScenePoint; approach: ScenePoint; yaw: number };
const seatCache = new WeakMap<CompanionPlaceLayout, readonly NightclubSeat[]>();
/** Resolve cushion slots from the frozen layout, including mirrored sofas. */
export function nightclubSeats(layout: CompanionPlaceLayout): readonly NightclubSeat[] {
  if (layout.kind !== "nightclub") return [];
  const cached = seatCache.get(layout);
  if (cached) return cached;
  const seats = layout.fixtures.filter(fixture => fixture.kind === "bench" && fixture.id.startsWith("club-lounge")).slice(0, 3).flatMap(sofa => {
    const [width, height, depth] = sofa.size;
    if (![...sofa.point, width, height, depth, sofa.yaw].every(Number.isFinite) || width < 2 || width > 6 || height < .8 || height > 1.2 || depth < 1 || depth > 1.5) return [];
    const inner = width - .36, count = Math.min(6, Math.max(2, Math.round(inner / .9)));
    const transform = (x: number, y: number, z: number): ScenePoint => [sofa.point[0] + x * Math.cos(sofa.yaw) + z * Math.sin(sofa.yaw),
      y, sofa.point[2] - x * Math.sin(sofa.yaw) + z * Math.cos(sofa.yaw)];
    return Array.from({ length: count }, (_, index) => {
      const x = -inner / 2 + (index + .5) * inner / count;
      return { id: `${sofa.id}:${index}`, position: transform(x, sofa.point[1] + .54, .085),
        approach: transform(x, layout.navigation.floorY, depth / 2 + layout.navigation.radius + .12), yaw: sofa.yaw };
    }).filter(seat => routeSegmentClear(layout.navigation, seat.approach, seat.approach));
  });
  seatCache.set(layout, seats);
  return seats;
}
export function nightclubInsideDanceFloor(layout: CompanionPlaceLayout, x: number, z: number) {
  const zone = layout.zones.find(zone => zone.activity === "dance");
  return !!zone && Number.isFinite(x) && Number.isFinite(z) && x >= zone.bounds.minX && x <= zone.bounds.maxX && z >= zone.bounds.minZ && z <= zone.bounds.maxZ;
}

/** A saved activity drives the existing rig. No new clips, timers or cue writes. */
export function nightclubExpression(activity: CompanionNightclubActivity | null | undefined, characterId: string | undefined, sessionId: string | undefined, now: number): CharacterCommand | null {
  if (!characterId || !sessionId || !nightclubDanceActive(activity, now) || activity?.kind !== "dance" || now < activity.round.startsAt) return null;
  const index = Math.floor((now - activity.round.startsAt) / (activity.round.beatMs * 4));
  const issuedAt = activity.round.startsAt + index * activity.round.beatMs * 4;
  const durationMs = Math.min(activity.round.beatMs * 4, activity.round.startsAt + (activity.round.lanes.length - 1) * activity.round.beatMs + activity.round.windowMs - issuedAt);
  const requestId = `club.${activity.id}.${index}`;
  return { requestId, sessionId: `club.${sessionId}`, turnId: requestId, characterId, action: "gesture", value: index % 2 ? "sway" : "groove",
    intensity: .8, durationMs, issuedAt, expiresAt: issuedAt + durationMs };
}
