export const COMPANION_WORLD_CLOCK_VERSION = 1 as const;
/** Approved 4x game time: one real minute advances four game minutes. */
export const COMPANION_WORLD_REAL_MS_PER_GAME_MINUTE = 15_000 as const;

/** One immutable configuration per world epoch. All timestamps are real milliseconds. */
export type CompanionWorldClock = Readonly<{
  version: typeof COMPANION_WORLD_CLOCK_VERSION;
  worldKey: string;
  epochKey: string;
  epochRealMs: number;
  /** Whole game minutes since game day zero at epochRealMs. */
  epochGameMinute: number;
  /** Stored epoch scale. New worlds use COMPANION_WORLD_REAL_MS_PER_GAME_MINUTE. */
  realMsPerGameMinute: number;
}>;

export type CompanionWorldClockPhase = "night" | "morning" | "afternoon" | "evening";
export type CompanionWorldClockSample = Readonly<{
  version: typeof COMPANION_WORLD_CLOCK_VERSION;
  worldKey: string;
  epochKey: string;
  sampledAtRealMs: number;
  gameMinute: number;
  /** Zero-based game day, unrelated to the device's date or timezone. */
  day: number;
  minuteOfDay: number;
  hour: number;
  minute: number;
  minuteProgress: number;
  phase: CompanionWorldClockPhase;
}>;

const validKey = (value: unknown, limit: number): value is string =>
  typeof value === "string" && value.length <= limit && value.trim() === value
  && /^[A-Za-z0-9][A-Za-z0-9._:-]*$/.test(value);
const nonnegativeInteger = (value: unknown): value is number =>
  typeof value === "number" && Number.isSafeInteger(value) && value >= 0;

/** Pure projection, never a settlement clock for payments, leases or auth expiry.
 * Preserve each stored epoch's scale. Travel and movement use server real milliseconds.
 * Reject unsupported epochs and pre-epoch samples instead of resetting or clamping time.
 * The owner must reject stale server observations and migrate state when an epoch changes.
 */
export function sampleCompanionWorldClock(clock: CompanionWorldClock, sharedNowRealMs: number): CompanionWorldClockSample {
  if (!clock || clock.version !== COMPANION_WORLD_CLOCK_VERSION
    || !validKey(clock.worldKey, 80) || !validKey(clock.epochKey, 160)
    || !nonnegativeInteger(clock.epochRealMs) || !nonnegativeInteger(clock.epochGameMinute)
    || !nonnegativeInteger(clock.realMsPerGameMinute) || clock.realMsPerGameMinute === 0
    || !nonnegativeInteger(sharedNowRealMs) || sharedNowRealMs < clock.epochRealMs) {
    throw new Error("Provide a supported shared world epoch, positive time scale and a timestamp at or after that epoch.");
  }
  const elapsed = sharedNowRealMs - clock.epochRealMs;
  const remainder = elapsed % clock.realMsPerGameMinute;
  const elapsedMinutes = (elapsed - remainder) / clock.realMsPerGameMinute;
  const gameMinute = clock.epochGameMinute + elapsedMinutes;
  if (!Number.isSafeInteger(gameMinute)) throw new Error("Shared game time exceeds the safe integer range.");
  const minuteOfDay = gameMinute % 1440;
  // Authored display phases, not geographic sunrise or venue opening rules.
  const phase: CompanionWorldClockPhase = minuteOfDay < 360 ? "night"
    : minuteOfDay < 720 ? "morning" : minuteOfDay < 1080 ? "afternoon" : "evening";
  return Object.freeze<CompanionWorldClockSample>({
    version: COMPANION_WORLD_CLOCK_VERSION, worldKey: clock.worldKey, epochKey: clock.epochKey,
    sampledAtRealMs: sharedNowRealMs, gameMinute, day: Math.floor(gameMinute / 1440), minuteOfDay,
    hour: Math.floor(minuteOfDay / 60), minute: minuteOfDay % 60,
    minuteProgress: remainder / clock.realMsPerGameMinute, phase,
  });
}

/** Determines whether a 24-hour time or sample represents dark Lagos night (19:00 - 05:59). */
export function isCompanionNight(sampleOrHour: CompanionWorldClockSample | number): boolean {
  const hour = typeof sampleOrHour === "number" ? sampleOrHour : sampleOrHour.hour;
  return hour >= 19 || hour < 6;
}

export type CompanionLagosShiftModifier = Readonly<{
  kind: "clear" | "heavy-rain" | "fuel-scarcity" | "night-market";
  label: string;
  hazardMultiplier: number;
  crimeMultiplier: number;
  courierSurgePercent: number;
}>;

/** Deterministic game-day environmental modifiers driven by the shared 4x clock. */
export function companionLagosShiftModifier(day: number, phase: CompanionWorldClockPhase): CompanionLagosShiftModifier {
  if (phase === "night") {
    return Object.freeze({
      kind: "night-market",
      label: "Night Market Hours",
      hazardMultiplier: 1.25,
      crimeMultiplier: 2.0,
      courierSurgePercent: 35,
    });
  }
  const roll = (day * 37 + 11) % 10;
  if (roll <= 2) {
    return Object.freeze({
      kind: "heavy-rain",
      label: "Flash Flood",
      hazardMultiplier: 1.2,
      crimeMultiplier: 1.25,
      courierSurgePercent: 30,
    });
  }
  if (roll === 3) {
    return Object.freeze({
      kind: "fuel-scarcity",
      label: "Fuel Scarcity",
      hazardMultiplier: 1.15,
      crimeMultiplier: 1.15,
      courierSurgePercent: 20,
    });
  }
  return Object.freeze({
    kind: "clear",
    label: "Normal Conditions",
    hazardMultiplier: 1.0,
    crimeMultiplier: 1.0,
    courierSurgePercent: 0,
  });
}
