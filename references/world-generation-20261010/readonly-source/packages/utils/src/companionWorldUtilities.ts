import { sampleCompanionWorldClock, type CompanionWorldClock } from "./companionWorldClock";

export const COMPANION_WORLD_UTILITIES_VERSION = 1 as const;
/** Fictional game tuning, not grid/carrier data. Preserve this version within an epoch. */
export const companionWorldUtilityRules = Object.freeze({
  power: Object.freeze({ cycleGameMinutes: 180, minimumGameMinutes: 8, maximumGameMinutes: 16, goodMarginGameMinutes: 30 }),
  network: Object.freeze({ cycleGameMinutes: 90, minimumGameMinutes: 2, maximumGameMinutes: 5, goodMarginGameMinutes: 20 }),
  maximumIntervalGameMinutes: 1440,
  maximumDisplayExtrapolationRealMs: 60_000,
  maximumNoticeAgeRealMs: 5000,
});

/** The server resolves this zone from admitted place/household ownership. Use an
 * opaque household key, not a physical address, room ID or temporary call ID. */
export type CompanionWorldUtilityScope = Readonly<{
  version: typeof COMPANION_WORLD_UTILITIES_VERSION;
  worldKey: string;
  clockEpochKey: string;
  zoneKey: string;
}>;
export type CompanionWorldUtilityAxis = "power" | "network";
type UtilityWindow = Readonly<{
  /** Stable transition identity, including initial service. No user or raw zone ID. */
  eventId: string;
  episodeId: string | null;
  startedAtRealMs: number;
  endsAtRealMs: number;
  nextChangeAtRealMs: number;
}>;
export type CompanionWorldPower = UtilityWindow & Readonly<{ status: "on" | "outage" }>;
export type CompanionWorldSimulatedNetwork = UtilityWindow & Readonly<{ status: "good" | "slow" }>;
export type CompanionWorldUtilities = Readonly<{
  version: typeof COMPANION_WORLD_UTILITIES_VERSION;
  scope: CompanionWorldUtilityScope;
  sampledAtRealMs: number;
  gameMinute: number;
  power: CompanionWorldPower;
  network: CompanionWorldSimulatedNetwork;
  nextChangeAtRealMs: number;
}>;
export type CompanionWorldUtilityObservation = Readonly<{
  scope: CompanionWorldUtilityScope;
  clock: CompanionWorldClock;
  serverNowRealMs: number;
  /** Owner's admitted server-time watermark. Never reset it to accept an older frame. */
  notBeforeRealMs: number;
}>;
type Episode = Readonly<{ id: string; startsAtRealMs: number; endsAtRealMs: number }>;

const integerTime = (value: unknown): value is number => typeof value === "number" && Number.isSafeInteger(value) && value >= 0;
const validKey = (value: unknown, maximum: number): value is string =>
  typeof value === "string" && value.length <= maximum && /^[A-Za-z0-9][A-Za-z0-9._:-]*$/.test(value);
function safeTime(value: number) {
  if (!integerTime(value)) throw new Error("Utility time exceeds the supported server-millisecond range.");
  return value;
}
function admit({ scope, clock, serverNowRealMs, notBeforeRealMs }: CompanionWorldUtilityObservation) {
  if (!scope || scope.version !== COMPANION_WORLD_UTILITIES_VERSION || !validKey(scope.zoneKey, 160)
    || !clock || scope.worldKey !== clock.worldKey || scope.clockEpochKey !== clock.epochKey
    || !integerTime(notBeforeRealMs) || notBeforeRealMs < clock.epochRealMs || serverNowRealMs < notBeforeRealMs) {
    throw new Error("Provide the admitted utility zone, current epoch and non-stale server observation.");
  }
  return sampleCompanionWorldClock(clock, serverNowRealMs);
}
function sameScope(a: CompanionWorldUtilityScope, b: CompanionWorldUtilityScope) {
  return a.version === b.version && a.worldKey === b.worldKey && a.clockEpochKey === b.clockEpochKey && a.zoneKey === b.zoneKey;
}
function savedScope(scope: CompanionWorldUtilityScope): CompanionWorldUtilityScope {
  return Object.freeze({ version: scope.version, worldKey: scope.worldKey, clockEpochKey: scope.clockEpochKey, zoneKey: scope.zoneKey });
}
function hash(text: string, seed = 2166136261) {
  let value = seed;
  for (let index = 0; index < text.length; index++) value = Math.imul(value ^ text.charCodeAt(index), 16777619) >>> 0;
  return value;
}
function scopeSeed(scope: CompanionWorldUtilityScope) {
  return JSON.stringify([scope.version, scope.worldKey, scope.clockEpochKey, scope.zoneKey]);
}
/** Stable non-cryptographic labels. Never use these hashes for authorization. */
function scopeLabel(seed: string) {
  return hash(seed).toString(16).padStart(8, "0") + hash(seed, 2246822519).toString(16).padStart(8, "0");
}
function cycleMs(clock: CompanionWorldClock, axis: CompanionWorldUtilityAxis) {
  const value = safeTime(companionWorldUtilityRules[axis].cycleGameMinutes * clock.realMsPerGameMinute);
  if (value === 0) throw new Error("Utility cycles require a positive stored game-time scale.");
  return value;
}
function episode(seed: string, label: string, clock: CompanionWorldClock, axis: CompanionWorldUtilityAxis, block: number): Episode {
  const rules = companionWorldUtilityRules[axis];
  const duration = rules.minimumGameMinutes
    + hash(JSON.stringify([seed, axis, block, "duration"])) % (rules.maximumGameMinutes - rules.minimumGameMinutes + 1);
  const possibleOffsets = rules.cycleGameMinutes - duration - rules.goodMarginGameMinutes * 2 + 1;
  const offset = rules.goodMarginGameMinutes + hash(JSON.stringify([seed, axis, block, "offset"])) % possibleOffsets;
  const startMinute = safeTime(block * rules.cycleGameMinutes + offset);
  const startsAtRealMs = safeTime(clock.epochRealMs + safeTime(startMinute * clock.realMsPerGameMinute));
  const endsAtRealMs = safeTime(startsAtRealMs + safeTime(duration * clock.realMsPerGameMinute));
  return Object.freeze({ id: `utilities-v1:${label}:${axis}:${block}`, startsAtRealMs, endsAtRealMs });
}
function axisWindow(seed: string, label: string, clock: CompanionWorldClock, axis: CompanionWorldUtilityAxis, now: number) {
  const block = Math.floor((now - clock.epochRealMs) / cycleMs(clock, axis));
  const current = episode(seed, label, clock, axis, block);
  if (now >= current.startsAtRealMs && now < current.endsAtRealMs) return {
    disrupted: true, eventId: `${current.id}:started`, episodeId: current.id,
    startedAtRealMs: current.startsAtRealMs, endsAtRealMs: current.endsAtRealMs, nextChangeAtRealMs: current.endsAtRealMs,
  };
  const previous = now >= current.endsAtRealMs ? current : block > 0 ? episode(seed, label, clock, axis, block - 1) : null;
  const next = now < current.startsAtRealMs ? current : episode(seed, label, clock, axis, block + 1);
  // A good interval spans cycle boundaries without creating another transition.
  return { disrupted: false, eventId: previous ? `${previous.id}:restored` : `utilities-v1:${label}:${axis}:initial`,
    episodeId: null, startedAtRealMs: previous?.endsAtRealMs ?? clock.epochRealMs,
    endsAtRealMs: next.startsAtRealMs, nextChangeAtRealMs: next.startsAtRealMs };
}

/** Pure authoritative projection. Invalid or stale data throws, never defaults to
 * an outage or good service. It reads no wall clock and schedules no work. */
export function sampleCompanionWorldUtilities(observation: CompanionWorldUtilityObservation): CompanionWorldUtilities {
  const time = admit(observation), { scope, clock, serverNowRealMs } = observation;
  // Both axes and their adjacent episodes use the same admitted scope. Keep
  // this preparation local to the sample, without retaining a world cache.
  const seed = scopeSeed(scope), label = scopeLabel(seed);
  const { disrupted: powerOff, ...power } = axisWindow(seed, label, clock, "power", serverNowRealMs);
  const { disrupted: networkSlow, ...network } = axisWindow(seed, label, clock, "network", serverNowRealMs);
  return Object.freeze({ version: COMPANION_WORLD_UTILITIES_VERSION, scope: savedScope(scope),
    sampledAtRealMs: serverNowRealMs, gameMinute: time.gameMinute,
    power: Object.freeze({ ...power, status: powerOff ? "outage" : "on" }),
    network: Object.freeze({ ...network, status: networkSlow ? "slow" : "good" }),
    nextChangeAtRealMs: Math.min(power.nextChangeAtRealMs, network.nextChangeAtRealMs) });
}

/** Display only, from the existing server-offset clock. An old observation cannot
 * invent indefinitely continuing episodes during a real connection failure. */
export function projectCompanionWorldUtilities(observation: CompanionWorldUtilityObservation, displayNowRealMs: number): CompanionWorldUtilities {
  admit(observation);
  if (!integerTime(displayNowRealMs) || displayNowRealMs < observation.serverNowRealMs
    || displayNowRealMs - observation.serverNowRealMs > companionWorldUtilityRules.maximumDisplayExtrapolationRealMs) {
    throw new Error("Refresh the admitted server observation before projecting utility conditions.");
  }
  return sampleCompanionWorldUtilities({ ...observation, serverNowRealMs: displayNowRealMs,
    notBeforeRealMs: observation.serverNowRealMs });
}

export type CompanionWorldUtilityNotice = Readonly<{
  eventId: string;
  axis: CompanionWorldUtilityAxis;
  startedAtRealMs: number;
  text: string;
}>;
function validNoticeSample(sample: CompanionWorldUtilities) {
  if (!sample || sample.version !== COMPANION_WORLD_UTILITIES_VERSION || !sample.scope
    || sample.scope.version !== COMPANION_WORLD_UTILITIES_VERSION || !validKey(sample.scope.worldKey, 80)
    || !validKey(sample.scope.clockEpochKey, 160) || !validKey(sample.scope.zoneKey, 160)
    || !integerTime(sample.sampledAtRealMs) || !integerTime(sample.gameMinute)
    || !sample.power || !sample.network || !["on", "outage"].includes(sample.power.status)
    || !["good", "slow"].includes(sample.network.status)) return false;
  const label = scopeLabel(scopeSeed(sample.scope));
  for (const axis of ["power", "network"] as const) {
    const value = sample[axis], disrupted = value.status === "outage" || value.status === "slow";
    if (!validKey(value.eventId, 100) || !value.eventId.startsWith(`utilities-v1:${label}:${axis}:`)
      || ![value.startedAtRealMs, value.endsAtRealMs, value.nextChangeAtRealMs].every(integerTime)
      || value.startedAtRealMs > sample.sampledAtRealMs || value.endsAtRealMs <= sample.sampledAtRealMs
      || value.nextChangeAtRealMs !== value.endsAtRealMs
      || disrupted && (!validKey(value.episodeId, 100) || value.eventId !== `${value.episodeId}:started`)
      || !disrupted && value.episodeId !== null) return false;
  }
  return sample.nextChangeAtRealMs === Math.min(sample.power.nextChangeAtRealMs, sample.network.nextChangeAtRealMs);
}
/** Consecutive admitted displays only. Late joins, zone changes and missed episodes
 * do not replay jokes. The existing notice owner retains deduplication and priority. */
export function companionWorldUtilityNotices(previous: CompanionWorldUtilities | null, current: CompanionWorldUtilities): readonly CompanionWorldUtilityNotice[] {
  if (!validNoticeSample(current) || previous && !validNoticeSample(previous)) {
    throw new Error("Utility notices require supported, admitted condition samples.");
  }
  if (!previous || !sameScope(previous.scope, current.scope)) return Object.freeze([]);
  if (!integerTime(previous.sampledAtRealMs) || !integerTime(current.sampledAtRealMs) || current.sampledAtRealMs < previous.sampledAtRealMs) {
    throw new Error("Utility notices require ordered admitted display samples.");
  }
  const notices: CompanionWorldUtilityNotice[] = [];
  for (const axis of ["power", "network"] as const) {
    const before = previous[axis], after = current[axis];
    if (before.eventId === after.eventId || before.status === after.status || after.startedAtRealMs <= previous.sampledAtRealMs
      || after.startedAtRealMs > current.sampledAtRealMs
      || current.sampledAtRealMs - after.startedAtRealMs > companionWorldUtilityRules.maximumNoticeAgeRealMs) continue;
    notices.push(Object.freeze({ eventId: after.eventId, axis, startedAtRealMs: after.startedAtRealMs,
      text: axis === "power" ? after.status === "outage" ? "NEPA took the light" : "Up NEPA!"
        : after.status === "slow" ? "4G is moving like 2G." : "Network is back." }));
  }
  return Object.freeze(notices);
}

export type CompanionWorldUtilityInterval = Readonly<{
  version: typeof COMPANION_WORLD_UTILITIES_VERSION;
  scope: CompanionWorldUtilityScope;
  fromServerNowRealMs: number;
  toServerNowRealMs: number;
  powerOnRealMs: number;
  powerOutageRealMs: number;
  networkGoodRealMs: number;
  networkSlowRealMs: number;
  /** No task delay is applied here. Owners decide which simulated equipment uses these totals. */
  disruptions: readonly Readonly<{ axis: CompanionWorldUtilityAxis; episodeId: string; startsAtRealMs: number;
    endsAtRealMs: number; overlapRealMs: number }>[];
}>;
/** Half-open interval [from, to), at most one game day and 26 candidate episodes.
 * Authoritative owners persist their settlement cursor and results atomically.
 * Splitting an interval neither repeats nor loses utility time. */
export function measureCompanionWorldUtilityInterval(args: Readonly<{
  scope: CompanionWorldUtilityScope;
  clock: CompanionWorldClock;
  fromServerNowRealMs: number;
  toServerNowRealMs: number;
  notBeforeRealMs: number;
}>): CompanionWorldUtilityInterval {
  const { scope, clock, fromServerNowRealMs: from, toServerNowRealMs: to, notBeforeRealMs } = args;
  admit({ scope, clock, serverNowRealMs: from, notBeforeRealMs });
  admit({ scope, clock, serverNowRealMs: to, notBeforeRealMs: from });
  const duration = to - from;
  if (duration / clock.realMsPerGameMinute > companionWorldUtilityRules.maximumIntervalGameMinutes) {
    throw new Error("Settle utility time in intervals of at most one game day.");
  }
  const disruptions: { axis: CompanionWorldUtilityAxis; episodeId: string; startsAtRealMs: number; endsAtRealMs: number; overlapRealMs: number }[] = [];
  let powerOutageRealMs = 0, networkSlowRealMs = 0;
  if (duration > 0) {
    const seed = scopeSeed(scope), label = scopeLabel(seed);
    for (const axis of ["power", "network"] as const) {
      const cycle = cycleMs(clock, axis), first = Math.floor((from - clock.epochRealMs) / cycle);
      const last = Math.floor((to - 1 - clock.epochRealMs) / cycle);
      for (let block = first; block <= last; block++) {
        const value = episode(seed, label, clock, axis, block);
        const overlapRealMs = Math.max(0, Math.min(to, value.endsAtRealMs) - Math.max(from, value.startsAtRealMs));
        if (overlapRealMs === 0) continue;
        disruptions.push(Object.freeze({ axis, episodeId: value.id, startsAtRealMs: value.startsAtRealMs, endsAtRealMs: value.endsAtRealMs, overlapRealMs }));
        if (axis === "power") powerOutageRealMs += overlapRealMs;
        else networkSlowRealMs += overlapRealMs;
      }
    }
  }
  return Object.freeze({ version: COMPANION_WORLD_UTILITIES_VERSION, scope: savedScope(scope),
    fromServerNowRealMs: from, toServerNowRealMs: to, powerOnRealMs: duration - powerOutageRealMs, powerOutageRealMs,
    networkGoodRealMs: duration - networkSlowRealMs, networkSlowRealMs, disruptions: Object.freeze(disruptions) });
}
