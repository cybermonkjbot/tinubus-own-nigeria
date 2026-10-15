import type { CompanionJourneyEnforcementDecision } from "./companionPolicing";

export type CompanionGeoPoint = { latitude: number; longitude: number };
export type CompanionCorridorRoute = {
  id: string; version: "lagos-corridors-v1"; placement: "proposed";
  points: readonly CompanionGeoPoint[]; lengthMetres: number;
};
export type CompanionRoadRoute = {
  id: string; version: "mapbox-directions-v5"; placement: "road-network";
  points: readonly CompanionGeoPoint[]; lengthMetres: number;
  /** Real travel time between successive road points, never accelerated game time. */
  segmentDurationsMs: readonly number[];
};
export type CompanionJourneyRoute = CompanionCorridorRoute | CompanionRoadRoute;
export type CompanionJourneyTraffic = {
  provider: "mapbox"; fetchedAtRealMs: number; durationMs: number; typicalDurationMs: number | null;
  coverage: "complete" | "partial" | "unavailable";
  congestion: "low" | "moderate" | "heavy" | "severe" | null;
};
export type CompanionJourney = {
  id: string; requestId: string; fromPlaceId: string; toPlaceId: string;
  route: CompanionJourneyRoute; vehicle: "car" | "okada" | "keke" | "danfo" | "canoe" | "speedboat"; startedAt: number; arrivesAt: number;
  checkpoint: { progress: number; startsAt: number; endsAt: number;
    /** Null waits for the saved receipt; absent retains legacy planned timing. */
    resumedAt?: number | null; enforcement?: CompanionJourneyEnforcementDecision } | null;
  /** A departure-time estimate, not a continuously refreshed traffic feed. Absent on older journeys. */
  traffic?: CompanionJourneyTraffic;
  /** Server-owned travel acceleration. Departure/fare timestamps remain immutable. */
  travelTiming?: { rate: 1 | 4; anchorAt: number; anchorProgress: number; durationMs: number };
  status: "traveling" | "arrived" | "interrupted";
};
export function companionGeoDistance(a: CompanionGeoPoint, b: CompanionGeoPoint) {
  const radians = Math.PI / 180, lat = (b.latitude - a.latitude) * radians, lng = (b.longitude - a.longitude) * radians;
  const h = Math.sin(lat / 2) ** 2 + Math.cos(a.latitude * radians) * Math.cos(b.latitude * radians) * Math.sin(lng / 2) ** 2;
  return 6_371_000 * 2 * Math.atan2(Math.sqrt(h), Math.sqrt(Math.max(0, 1 - h)));
}
export function companionRouteLength(points: readonly CompanionGeoPoint[]) {
  return points.slice(1).reduce((total, point, index) => total + companionGeoDistance(points[index], point), 0);
}
/** Shared geographic sampling for walking and motorized journeys. */
export function sampleCompanionRoute(route: CompanionJourneyRoute, progress: number) {
  const points = route.points;
  let remaining = Math.max(0, Math.min(1, progress)) * route.lengthMetres;
  for (let index = 1; index < points.length; index++) {
    const from = points[index - 1], to = points[index], length = companionGeoDistance(from, to);
    if (remaining <= length || index === points.length - 1) {
      const ratio = length ? Math.min(1, remaining / length) : 1;
      return { latitude: from.latitude + (to.latitude - from.latitude) * ratio, longitude: from.longitude + (to.longitude - from.longitude) * ratio };
    }
    remaining -= length;
  }
  return points[0];
}
function timedCompanionRoutePosition(route: CompanionJourneyRoute, progress: number) {
  const timings = route.version === "mapbox-directions-v5" ? route.segmentDurationsMs : undefined;
  if (!timings || route.points.length < 2 || timings.length !== route.points.length - 1) return null;
  if (progress <= 0) return { index: 0, ratio: 0 };
  if (progress >= 1) return { index: timings.length - 1, ratio: 1 };
  const total = timings.reduce((sum, time) => sum + time, 0);
  if (total <= 0) return null;
  let remaining = progress * total;
  for (let index = 0; index < timings.length; index++) {
    if (remaining <= timings[index] || index === timings.length - 1) {
      const ratio = timings[index] > 0 ? Math.min(1, remaining / timings[index]) : 1;
      return { index, ratio };
    }
    remaining -= timings[index];
  }
  return null;
}
function sampleTimedCompanionRoute(route: CompanionJourneyRoute, progress: number) {
  const position = timedCompanionRoutePosition(route, progress);
  if (!position) return null;
  if (progress <= 0) return route.points[0];
  if (progress >= 1) return route.points[route.points.length - 1];
  const from = route.points[position.index], to = route.points[position.index + 1];
  return { latitude: from.latitude + (to.latitude - from.latitude) * position.ratio,
    longitude: from.longitude + (to.longitude - from.longitude) * position.ratio };
}
/** Convert the journey's checkpoint-adjusted time fraction to traveled distance.
 * Settlement only. The render sampler does not need this geometry scan. */
export function companionRouteDistanceProgress(route: CompanionJourneyRoute, timeProgress: number): number {
  if (!Number.isFinite(timeProgress) || route.points.length < 2 || !Number.isFinite(route.lengthMetres) || route.lengthMetres <= 0)
    throw new Error("Provide a valid journey route and finite progress.");
  const progress = Math.max(0, Math.min(1, timeProgress));
  if (route.version !== "mapbox-directions-v5") return progress;
  const duration = route.segmentDurationsMs.reduce((sum, time) => sum + time, 0);
  if (route.segmentDurationsMs.length !== route.points.length - 1
    || route.segmentDurationsMs.some(time => !Number.isFinite(time) || time < 0)
    || !Number.isFinite(duration) || duration <= 0)
    throw new Error("Provide the saved route's segment travel times.");
  const position = timedCompanionRoutePosition(route, progress);
  if (!position) throw new Error("The journey position is unavailable.");
  let total = 0, traveled = 0;
  for (let index = 0; index < route.points.length - 1; index++) {
    const distance = companionGeoDistance(route.points[index], route.points[index + 1]);
    total += distance;
    if (index < position.index) traveled += distance;
    else if (index === position.index) traveled += distance * position.ratio;
  }
  if (!Number.isFinite(total) || total <= 0 || !Number.isFinite(traveled)) throw new Error("The road geometry needs review.");
  // Apply the fraction to the trip's retained distance, including its original rounding.
  return Math.max(0, Math.min(1, traveled / total));
}
/** Saved receipt timing only, also used by arrival gates without scanning route geometry. */
export function companionJourneyTiming(journey: CompanionJourney, now: number) {
  const pause = journey.checkpoint, pauseMs = pause ? pause.endsAt - pause.startsAt : 0;
  if (journey.travelTiming) {
    const { rate, anchorAt, anchorProgress, durationMs } = journey.travelTiming;
    const waiting = pause?.resumedAt === null;
    const pauseEnd = pause ? typeof pause.resumedAt === "number" ? Math.max(pause.startsAt, pause.resumedAt) : pause.endsAt : 0;
    const elapsedPause = pause ? Math.max(0, Math.min(now, waiting ? now : pauseEnd) - Math.max(anchorAt, pause.startsAt)) : 0;
    const elapsed = Math.max(0, now - anchorAt - elapsedPause);
    const progress = journey.status === "arrived" ? 1 : Math.max(0, Math.min(1, anchorProgress + elapsed * rate / durationMs));
    const remainingPause = pause ? Math.max(0, (waiting ? pause.endsAt : pauseEnd) - Math.max(now, pause.startsAt)) : 0;
    return { progress, atCheckpoint: !!pause && now >= pause.startsAt && (waiting || now < pauseEnd),
      remainingMs: Math.max(0, (1 - progress) * durationMs / rate) + remainingPause,
      arrivalDue: journey.status === "traveling" && !waiting && progress >= 1 };
  }
  const duration = Math.max(1, journey.arrivesAt - journey.startedAt - pauseMs);
  const waiting = pause?.resumedAt === null;
  const pauseEnd = pause ? typeof pause.resumedAt === "number" ? Math.max(pause.startsAt, pause.resumedAt) : pause.endsAt : 0;
  const elapsedPause = pause ? Math.max(0, waiting ? now - pause.startsAt : Math.min(pauseEnd - pause.startsAt, now - pause.startsAt)) : 0;
  const arrivesAt = pause && !waiting ? journey.arrivesAt + pauseEnd - pause.endsAt : journey.arrivesAt;
  const elapsedTravel = now - journey.startedAt - elapsedPause;
  const progress = journey.status === "arrived" ? 1 : Math.max(0, Math.min(1, elapsedTravel / duration));
  const remainingMs = pause && waiting ? Math.max(0, duration - elapsedTravel) + Math.max(0, pause.endsAt - Math.max(now, pause.startsAt)) : Math.max(0, arrivesAt - now);
  const arrivalDue = journey.status === "traveling" && !waiting && now >= arrivesAt;
  return { progress, atCheckpoint: !!pause && now >= pause.startsAt && (waiting || now < pauseEnd), remainingMs, arrivalDue };
}

/** Change only the remaining travel time, preserving position and checkpoint decisions. */
export function changeCompanionJourneySpeed<T extends CompanionJourney>(journey: T, rate: 1 | 4, now: number): T {
  if (rate !== 1 && rate !== 4 || !Number.isSafeInteger(now) || now < journey.startedAt)
    throw new Error("Choose a valid journey speed.");
  if (journey.status !== "traveling") throw new Error("This journey is no longer traveling.");
  const timing = companionJourneyTiming(journey, now);
  // Server-owned interruptions may return to normal while progress is paused.
  if (timing.atCheckpoint && rate !== 1) throw new Error("Finish the checkpoint before changing travel speed.");
  if (timing.arrivalDue || (journey.travelTiming?.rate ?? 1) === rate) return journey;
  const pause = journey.checkpoint;
  const durationMs = journey.travelTiming?.durationMs ?? Math.max(1, journey.arrivesAt - journey.startedAt - (pause ? pause.endsAt - pause.startsAt : 0));
  let checkpoint = pause;
  if (pause && now < pause.startsAt) {
    const startsAt = now + Math.ceil(Math.max(0, pause.progress - timing.progress) * durationMs / rate);
    checkpoint = { ...pause, startsAt, endsAt: startsAt + pause.endsAt - pause.startsAt };
  }
  return { ...journey, checkpoint, travelTiming: { rate, anchorAt: now, anchorProgress: timing.progress, durationMs } };
}
/** No routing request or database write. The route and timing were supplied by the server. */
export function sampleCompanionJourney(journey: CompanionJourney, now: number) {
  const timing = companionJourneyTiming(journey, now), { progress, remainingMs, arrivalDue } = timing;
  if (journey.route.points.length < 2) return { point: journey.route.points[0], progress, atCheckpoint: false, remainingMs, arrivalDue };
  const point = sampleTimedCompanionRoute(journey.route, progress) ?? sampleCompanionRoute(journey.route, progress);
  return { point, ...timing };
}

export function companionJourneyEta(remainingMs: number) {
  if (remainingMs < 60_000) return "Under 1 min";
  const minutes = Math.ceil(remainingMs / 60_000);
  return minutes < 60 ? `${minutes} min` : `${Math.floor(minutes / 60)}h${minutes % 60 ? ` ${minutes % 60}m` : ""}`;
}

/** Keep missing coverage distinct from light traffic and date the estimate to departure. */
export function companionJourneyTrafficNotice(journey: CompanionJourney) {
  const traffic = journey.traffic;
  if (!traffic) return "Simulated journey";
  if (traffic.coverage === "unavailable" || !traffic.congestion) return "Traffic unavailable · road estimate";
  const label = traffic.congestion === "low" ? "Light traffic" : traffic.congestion === "moderate" ? "Moderate traffic"
    : traffic.congestion === "heavy" ? "Heavy traffic" : "Severe traffic";
  return `${label} at departure${traffic.coverage === "partial" ? " · partial coverage" : ""}`;
}
