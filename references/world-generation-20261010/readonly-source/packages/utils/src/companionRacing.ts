import type { CompanionDrivingInput } from "./companionPlayerDriving";
import type { CompanionPlaceLayout } from "./companionPlaces";
import type { ScenePoint } from "./companionNavigation";

export const racingRules = Object.freeze({
  placeId: "race-circuit", version: "lagos-circuit-v1", players: 8, laps: 3,
  countdownMs: 4000, raceMs: 180000, tickMs: 200, inputTtlMs: 400,
  predictionMs: 250, maxSpeed: 36, acceleration: 9, braking: 22,
  wheelbase: 2.45, steering: .5, halfWidth: 8,
  // Open land northeast of the existing neighbourhoods. No saved parcels move.
  mapX: 1620, mapZ: -1460, halfX: 155, halfZ: 95,
});
export type RaceTransmission = "automatic" | "manual";
export type RacePhase = "lobby" | "countdown" | "racing" | "finished" | "cancelled";
export type RaceInput = CompanionDrivingInput & { handbrake: boolean; gear: number };
export const neutralRaceInput: RaceInput = Object.freeze({ throttle: 0, brake: 1, steering: 0, handbrake: false, gear: 1 });
export type RaceCar = {
  userId: string; name: string; vehicleId: string; transmission: RaceTransmission;
  x: number; z: number; yaw: number; speed: number; gear: number;
  sequence: number; input: RaceInput; inputAt: number; simulatedAt: number; recoveredAt?: number;
  distance: number; progress: number; nextCheckpoint: number; laps: number;
  lapStartedAt: number; lastLapMs: number | null; bestLapMs: number | null;
  finishedAt: number | null; status: "ready" | "racing" | "finished" | "withdrawn";
};
export type RaceState = {
  id: string; instanceId: string; hostId: string; phase: RacePhase;
  startsAt: number | null; endsAt: number | null; updatedAt: number; cars: RaceCar[];
};
export type RacingSnapshot = {
  race: RaceState | null;
  car: { vehicleId: string; revision: number; fuelUnits: number; condition: number; here: boolean } | null;
  eligibility: "eligible" | "no-car" | "car-away" | "car-busy" | "no-fuel" | "damaged";
};
export type RaceAction =
  | { action: "join"; vehicleId: string; vehicleRevision: number; transmission: RaceTransmission }
  | { action: "start" | "leave"; raceId: string }
  | { action: "recover"; raceId: string; sequence: number }
  | { action: "input"; raceId: string; sequence: number; input: RaceInput };

// A closed, clockwise circuit with clipped corners and a long start straight.
export const racingPoints: readonly ScenePoint[] = Object.freeze([
  [-95, 0, 72], [95, 0, 72], [130, 0, 35], [130, 0, -35],
  [95, 0, -72], [-95, 0, -72], [-130, 0, -35], [-130, 0, 35], [-95, 0, 72],
]);
const lengths = racingPoints.slice(1).map((point, i) => Math.hypot(point[0] - racingPoints[i][0], point[2] - racingPoints[i][2]));
export const racingLapMetres = lengths.reduce((sum, length) => sum + length, 0);
export const racingFuelMetres = racingLapMetres * racingRules.laps * 1.25;
export function racingGrid(seat: number) {
  return { x: -84 - Math.floor(seat / 2) * 5, z: 72 + (seat % 2 ? 2 : -2), yaw: Math.PI / 2 };
}
export function racingTrackProjection(x: number, z: number) {
  let result = { distance: Infinity, progress: 0, segment: 0 };
  let offset = 0;
  for (let i = 0; i < lengths.length; i++) {
    const a = racingPoints[i], b = racingPoints[i + 1], dx = b[0] - a[0], dz = b[2] - a[2];
    const t = Math.max(0, Math.min(1, ((x - a[0]) * dx + (z - a[2]) * dz) / lengths[i] ** 2));
    const distance = Math.hypot(x - a[0] - dx * t, z - a[2] - dz * t);
    if (distance < result.distance) result = { distance, progress: offset + lengths[i] * t, segment: i };
    offset += lengths[i];
  }
  return result;
}
export function validRaceInput(input: RaceInput): boolean {
  return !!input && [input.throttle, input.brake, input.steering].every(Number.isFinite)
    && input.throttle >= 0 && input.throttle <= 1 && input.brake >= 0 && input.brake <= 1
    && Math.abs(input.steering) <= 1 && typeof input.handbrake === "boolean"
    && Number.isSafeInteger(input.gear) && input.gear >= 1 && input.gear <= 6;
}
export function validRaceAction(value: unknown): value is RaceAction {
  if (!value || typeof value !== "object" || Array.isArray(value)) return false;
  const action = value as Record<string, unknown>;
  const key = (value: unknown) => typeof value === "string" && value.length > 0 && value.length <= 300;
  if (action.action === "join") return key(action.vehicleId) && Number.isSafeInteger(action.vehicleRevision)
    && Number(action.vehicleRevision) >= 0 && ["automatic", "manual"].includes(String(action.transmission));
  if (!key(action.raceId)) return false;
  if (action.action === "start" || action.action === "leave") return true;
  if (action.action === "recover") return Number.isSafeInteger(action.sequence) && Number(action.sequence) > 0 && Number(action.sequence) <= 1000000;
  return action.action === "input" && Number.isSafeInteger(action.sequence) && Number(action.sequence) > 0
    && Number(action.sequence) <= 1000000 && validRaceInput(action.input as RaceInput);
}
/** Bounded server receipt, scoped to the current visit. Stale responses cannot replace a newer tick. */
export function readRacingSnapshot(json: unknown, instanceId: string | undefined, previous?: RacingSnapshot | null): RacingSnapshot | null {
  if (typeof json !== "string" || json.length > 32768 || !instanceId) return null;
  try {
    const value = JSON.parse(json) as RacingSnapshot;
    if (!value || !["eligible", "no-car", "car-away", "car-busy", "no-fuel", "damaged"].includes(value.eligibility)) return null;
    if (value.car && (typeof value.car.vehicleId !== "string" || value.car.vehicleId.length > 240
      || ![value.car.revision, value.car.fuelUnits, value.car.condition].every(Number.isFinite) || typeof value.car.here !== "boolean")) return null;
    const race = value.race;
    if (race) {
      if (race.instanceId !== instanceId || typeof race.id !== "string" || race.id.length > 300 || typeof race.hostId !== "string"
        || !Number.isSafeInteger(race.updatedAt) || !["lobby", "countdown", "racing", "finished", "cancelled"].includes(race.phase)
        || ![race.startsAt, race.endsAt].every(value => value === null || Number.isSafeInteger(value))
        || !Array.isArray(race.cars) || race.cars.length > racingRules.players || new Set(race.cars.map(car => car.userId)).size !== race.cars.length) return null;
      for (const car of race.cars) if (!car || typeof car.name !== "string" || car.name.length > 60 || typeof car.userId !== "string"
        || typeof car.vehicleId !== "string" || !["automatic", "manual"].includes(car.transmission)
        || ![car.x, car.z, car.yaw, car.speed, car.laps, car.distance, car.progress, car.simulatedAt, car.inputAt, car.sequence, car.nextCheckpoint, car.lapStartedAt].every(Number.isFinite)
        || ![car.finishedAt, car.lastLapMs, car.bestLapMs].every(value => value === null || Number.isFinite(value))
        || Math.abs(car.x) > racingRules.halfX || Math.abs(car.z) > racingRules.halfZ || car.speed < 0 || car.speed > racingRules.maxSpeed
        || !Number.isSafeInteger(car.laps) || car.laps < 0 || car.laps > racingRules.laps
        || !Number.isSafeInteger(car.nextCheckpoint) || car.nextCheckpoint < 0 || car.nextCheckpoint >= 8
        || car.recoveredAt !== undefined && !Number.isSafeInteger(car.recoveredAt)
        || !["ready", "racing", "finished", "withdrawn"].includes(car.status) || !validRaceInput(car.input)) return null;
      if (previous?.race?.instanceId === instanceId && previous.race.updatedAt > race.updatedAt) return previous;
    }
    return value;
  } catch { return null; }
}

/** Fixed server-time steps. The client predicts pose only and never owns laps or results. */
export function advanceRaceCar(car: RaceCar, now: number, startsAt: number, prediction = false): RaceCar {
  if (!Number.isSafeInteger(now) || now < car.simulatedAt || car.status !== "racing") return car;
  const end = Math.min(now, car.simulatedAt + (prediction ? racingRules.predictionMs : 1000));
  let next = { ...car, input: { ...car.input } }, clock = car.simulatedAt;
  while (clock < end) {
    const ms = Math.min(20, end - clock), dt = ms / 1000;
    const input = clock < startsAt || clock > car.inputAt + racingRules.inputTtlMs ? neutralRaceInput : car.input;
    const gear = car.transmission === "automatic" ? Math.min(6, Math.max(1, Math.floor(next.speed / 6) + 1)) : input.gear;
    const cap = Math.min(racingRules.maxSpeed, gear * 6.5);
    const acceleration = racingRules.acceleration * input.throttle * (car.transmission === "manual" && next.speed < (gear - 1) * 4 ? .45 : 1);
    next.speed = Math.max(0, Math.min(cap, next.speed + (acceleration - racingRules.braking * input.brake - (input.handbrake ? 28 : 0) - .4) * dt));
    // Cars face local +Z. The driver's right turns toward -X, so yaw decreases.
    const yaw = next.yaw - next.speed / racingRules.wheelbase * Math.tan(input.steering * racingRules.steering) * dt;
    const x = next.x + Math.sin(yaw) * next.speed * dt, z = next.z + Math.cos(yaw) * next.speed * dt;
    const projection = racingTrackProjection(x, z);
    if (projection.distance <= racingRules.halfWidth - 1.1) {
      next.distance += Math.hypot(x - next.x, z - next.z);
      next.x = x; next.z = z; next.yaw = Math.atan2(Math.sin(yaw), Math.cos(yaw));
      const previousSegment = racingTrackProjection(next.x - Math.sin(yaw) * next.speed * dt, next.z - Math.cos(yaw) * next.speed * dt).segment;
      const a = racingPoints[projection.segment], b = racingPoints[projection.segment + 1];
      const forward = Math.sin(yaw) * (b[0] - a[0]) + Math.cos(yaw) * (b[2] - a[2]) > 0;
      next.progress = projection.progress;
      // Ordered route sectors reject shortcuts, reversing and repeated finish-line crossings.
      if (!prediction && projection.segment === next.nextCheckpoint && forward
        && (projection.segment === 0 ? previousSegment === 0 && next.x >= -65 && next.x - Math.sin(yaw) * next.speed * dt < -65
          : previousSegment === (projection.segment + lengths.length - 1) % lengths.length)) {
        next.nextCheckpoint = (next.nextCheckpoint + 1) % lengths.length;
        if (projection.segment === 0) {
          const lapMs = clock + ms - next.lapStartedAt;
          next.laps++;
          next.lastLapMs = lapMs;
          next.bestLapMs = next.bestLapMs === null ? lapMs : Math.min(next.bestLapMs, lapMs);
          next.lapStartedAt = clock + ms;
          if (next.laps >= racingRules.laps) { next.status = "finished"; next.finishedAt = clock + ms; next.speed = 0; }
        }
      }
    } else next.speed = 0;
    next.gear = gear; clock += ms;
    if (next.status === "finished") break;
  }
  return { ...next, simulatedAt: now };
}
/** A server-selected reset behind the last validated sector. It never changes checkpoint or lap credit. */
export function recoverRaceCar(car: RaceCar, cars: readonly RaceCar[], now: number): RaceCar | null {
  const sector = car.nextCheckpoint === 0 ? lengths.length - 1 : car.nextCheckpoint - 1;
  const a = racingPoints[sector], b = racingPoints[sector + 1];
  const dx = (b[0] - a[0]) / lengths[sector], dz = (b[2] - a[2]) / lengths[sector];
  for (const offset of [4, 9, 14, 19]) for (const lateral of [0, -3, 3]) {
    const x = a[0] + dx * offset - dz * lateral, z = a[2] + dz * offset + dx * lateral;
    if (cars.some(other => other.userId !== car.userId && other.status !== "withdrawn" && Math.hypot(other.x - x, other.z - z) < 4.8)) continue;
    return { ...car, x, z, yaw: Math.atan2(dx, dz), speed: 0, input: { ...neutralRaceInput }, inputAt: now,
      simulatedAt: now, recoveredAt: now, progress: racingTrackProjection(x, z).progress };
  }
  return null;
}
export function rankedRaceCars(cars: readonly RaceCar[]): RaceCar[] {
  return [...cars].sort((a, b) => {
    if (a.finishedAt !== null || b.finishedAt !== null) return (a.finishedAt ?? Infinity) - (b.finishedAt ?? Infinity);
    if (a.status === "withdrawn" || b.status === "withdrawn") return Number(a.status === "withdrawn") - Number(b.status === "withdrawn");
    const sectors = (car: RaceCar) => car.nextCheckpoint === 0 ? 8 : car.nextCheckpoint;
    const progress = (car: RaceCar) => car.nextCheckpoint === 0 && car.progress < lengths[0] ? car.progress + racingLapMetres : car.progress;
    return b.laps - a.laps || sectors(b) - sectors(a) || progress(b) - progress(a) || a.userId.localeCompare(b.userId);
  });
}
/** Metre-scale car envelopes, using both cars' longitudinal and lateral axes. */
export function raceCarsTouch(a: RaceCar, b: RaceCar): boolean {
  const axes = (car: RaceCar) => [[Math.sin(car.yaw), Math.cos(car.yaw)], [Math.cos(car.yaw), -Math.sin(car.yaw)]];
  const [af, ar] = axes(a), [bf, br] = axes(b), dx = b.x - a.x, dz = b.z - a.z;
  const dot = (p: number[], q: number[]) => p[0] * q[0] + p[1] * q[1];
  return [af, ar, bf, br].every(axis => Math.abs(dx * axis[0] + dz * axis[1])
    < 2.1 * (Math.abs(dot(af, axis)) + Math.abs(dot(bf, axis))) + 1.05 * (Math.abs(dot(ar, axis)) + Math.abs(dot(br, axis))));
}
export function raceTime(ms: number | null) {
  if (ms === null) return "--:--.--";
  const value = Math.max(0, ms);
  return `${Math.floor(value / 60000)}:${String(Math.floor(value / 1000) % 60).padStart(2, "0")}.${String(Math.floor(value / 10) % 100).padStart(2, "0")}`;
}
export function racingPlaceLayout(seed: string): CompanionPlaceLayout {
  const revision = `${racingRules.version}:${seed}`;
  return {
    id: revision, placeId: racingRules.placeId, kind: "race-circuit", label: "Lagos Circuit", seed,
    generatorVersion: "lagos-places-v1", assetVersion: racingRules.version,
    navigation: { revision, floorY: 0, radius: .24, gridSize: 3,
      bounds: { minX: -155, maxX: 155, minZ: -95, maxZ: 95 }, obstacles: [] },
    fixtures: [], assets: [], npcs: [], services: [], zones: [], homeRooms: [],
    entrances: [{ id: "circuit-entry", label: "City", point: [0, 0, 90], outside: [0, 0, 94], width: 4,
      role: "world", access: "place-membership" }],
    spawns: [{ point: [0, 0, 90], yaw: Math.PI }], recoverySpawns: [], detentionSpawns: [],
    cameras: { wide: { eye: [0, 12, 88], target: [0, 0, 35] }, close: { eye: [0, 3, 88], target: [0, 0, 70] } },
    theme: { background: "#b9ccdc", floor: "#718166", wall: "#eeeae0", accent: "#d4dbcf", openAir: true },
    qualityGaps: ["Race cockpit and circuit require native and authenticated multiplayer visual acceptance."],
  };
}
