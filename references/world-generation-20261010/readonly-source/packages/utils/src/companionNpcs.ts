import { planRoomRoute, routeSegmentClear, type NavigationObstacle, type RoomNavigation, type ScenePoint } from "./companionNavigation";
import type { CompanionPlaceLayout, CompanionPlaceNpc, CompanionPlacePose } from "./companionPlaces";

export const companionNpcLimits = { npcs: 24, stepMs: 250, catchupSteps: 8, routePoints: 32, routeRetryMs: 1000, routineMs: 12_000, observations: 64 } as const;
export type CompanionNpcBehaviour = "idle" | "walk" | "work" | "attend" | "gather" | "flee" | "seek-help" | "wait-traffic" | "hawk" | "conduct-danfo" | "pos-cash" | "collect-dues";
export type CompanionNpcServiceCue = "prepare-food" | "handover-food" | "board" | "recover" | "release" | "mission" | "dance" | "gather" | "capture" | "football" | "arena" | "hawk-wares" | "call-passengers" | "pos-transaction" | "tout-settle";
export type CompanionNpcTask = {
  id: string; npcId: string; point: ScenePoint; kind: "work" | "gather" | "seek-help" | "hawk" | "conduct-danfo" | "pos-cash";
  startsAt: number; expiresAt: number;
};
/** Shared observations contain facts, never instructions to charge, arrest or heal. */
export type CompanionNpcObservation = {
  instanceId: string; layoutRevision: string; observedAt: number; expiresAt: number;
  danger: readonly { id: string; point: ScenePoint; radius: number }[];
  traffic: readonly NavigationObstacle[];
  queues: readonly { serviceId: string; waiting: number; stock: "available" | "empty" | "unknown" }[];
  tasks: readonly CompanionNpcTask[];
  /** Only the owning command's acknowledged result may publish a handover/recovery cue. */
  cues: readonly { id: string; npcId: string; serviceId: string; kind: CompanionNpcServiceCue; startsAt: number; expiresAt: number }[];
};
export type CompanionNpcSample = {
  id: string; pose: CompanionPlacePose; behaviour: CompanionNpcBehaviour;
  moving: boolean; serviceId: string | null; cue: CompanionNpcServiceCue | null;
  target: ScenePoint; targetKey: string; route: readonly ScenePoint[]; plannedAt: number;
  /** Asset part references only. Never a player's animal base, appearance or skeleton. */
  assetPartId?: string; issue: string | null;
};
export type CompanionNpcRuntime = {
  instanceId: string; layoutRevision: string; simulatedAt: number;
  npcs: readonly CompanionNpcSample[]; issues: readonly string[];
};
export type CompanionNpcSamplingOptions = { active: boolean; nearbyNpcIds: readonly string[] };
export type CompanionNpcDecorativeLeg = {
  readonly from: ScenePoint; readonly to: ScenePoint; readonly startsAt: number; readonly arrivesAt: number; readonly endsAt: number;
};
export type CompanionNpcDecorativePlan = {
  readonly instanceId: string; readonly layoutRevision: string; readonly sharedStartedAt: number;
  readonly npcs: readonly {
    readonly id: string; readonly home: CompanionPlacePose; readonly role: CompanionPlaceNpc["role"];
    readonly assetPartId?: string; readonly legs: readonly CompanionNpcDecorativeLeg[];
    readonly cycleMs: number; readonly offsetMs: number; readonly issue: string | null;
  }[];
};

const finitePoint = (point: ScenePoint) => point.length === 3 && point.every(Number.isFinite);
const distance = (a: ScenePoint, b: ScenePoint) => Math.hypot(a[0] - b[0], a[2] - b[2]);
const validBox = (box: NavigationObstacle) => [box.minX, box.maxX, box.minZ, box.maxZ].every(Number.isFinite) && box.minX <= box.maxX && box.minZ <= box.maxZ;
const active = (startsAt: number, expiresAt: number, now: number) => Number.isFinite(startsAt) && Number.isFinite(expiresAt) && startsAt <= now && now < expiresAt;
const compareId = (a: { id: string }, b: { id: string }) => a.id < b.id ? -1 : a.id > b.id ? 1 : 0;
function seedNumber(seed: string) {
  let value = 2166136261;
  for (let index = 0; index < seed.length; index++) value = Math.imul(value ^ seed.charCodeAt(index), 16777619);
  return value >>> 0;
}
function npcNavigation(layout: CompanionPlaceLayout, npc: CompanionPlaceNpc, traffic: readonly NavigationObstacle[]): RoomNavigation {
  return { ...layout.navigation, radius: Math.max(npc.radius, layout.navigation.radius), obstacles: [...layout.navigation.obstacles, ...traffic] };
}

const frozenPoint = (point: ScenePoint): ScenePoint => Object.freeze([...point]) as ScenePoint;
/** Detached and deeply frozen. A consumer cannot alter another scene's phase or routes. */
export function immutableCompanionNpcSnapshot(runtime: CompanionNpcRuntime): CompanionNpcRuntime {
  return Object.freeze({ ...runtime, issues: Object.freeze([...runtime.issues]), npcs: Object.freeze(runtime.npcs.map(sample => Object.freeze({
    ...sample, pose: Object.freeze({ point: frozenPoint(sample.pose.point), yaw: sample.pose.yaw }),
    target: frozenPoint(sample.target), route: Object.freeze(sample.route.map(frozenPoint)),
  }))) });
}

/** Compile once per resolved instance. Paths are collision-checked at NPC radius,
 * capped to 32 segments per actor. No time replay, server worker or subscription. */
export function createCompanionNpcDecorativePlan(layout: CompanionPlaceLayout, instanceId: string, sharedStartedAt: number): CompanionNpcDecorativePlan {
  const initial = createCompanionNpcRuntime(layout, instanceId, sharedStartedAt);
  if (initial.issues.length) throw new Error(initial.issues.join("; "));
  const npcs = initial.npcs.map(sample => {
    const npc = layout.npcs.find(npc => npc.id === sample.id)!;
    const nav = npcNavigation(layout, npc, []), legs: CompanionNpcDecorativeLeg[] = [];
    let time = 0, from = npc.home.point, issue: string | null = null;
    const targets = [...npc.route, npc.home.point];
    if (npc.route.length > companionNpcLimits.routePoints) issue = `NPC ${npc.id} routine exceeds the route limit.`;
    for (const target of issue ? [] : targets) {
      if (!finitePoint(target)) { issue = `NPC ${npc.id} has an invalid routine point.`; break; }
      if (distance(from, target) < .001) continue;
      const route = planRoomRoute(nav, from, target);
      if (!route || legs.length + route.length > companionNpcLimits.routePoints) { issue = `NPC ${npc.id} needs a bounded collision-safe routine.`; break; }
      for (let index = 0; index < route.length; index++) {
        const to = route[index], duration = Math.max(companionNpcLimits.stepMs,
          Math.ceil(distance(from, to) / .9 * 1000 / companionNpcLimits.stepMs) * companionNpcLimits.stepMs);
        const arrivesAt = time + duration, endsAt = arrivesAt + (index === route.length - 1 ? companionNpcLimits.routineMs : 0);
        legs.push(Object.freeze({ from: frozenPoint(from), to: frozenPoint(to), startsAt: time, arrivesAt, endsAt }));
        time = endsAt; from = to;
      }
    }
    if (issue) { legs.length = 0; time = 0; }
    const cycleMs = time || companionNpcLimits.routineMs;
    return Object.freeze({ id: npc.id, home: Object.freeze({ point: frozenPoint(npc.home.point), yaw: npc.home.yaw }),
      role: npc.role, assetPartId: npc.assetPartId, legs: Object.freeze(legs), cycleMs,
      offsetMs: seedNumber(`${layout.seed}:${npc.id}`) % (cycleMs / companionNpcLimits.stepMs) * companionNpcLimits.stepMs, issue });
  });
  return Object.freeze({ instanceId, layoutRevision: layout.navigation.revision, sharedStartedAt, npcs: Object.freeze(npcs) });
}

/** Direct shared phase, independent of join time, frame count, hiding or culling.
 * Empty observations use this path. Reactive state never comes from decoration. */
export function sampleCompanionNpcDecoration(plan: CompanionNpcDecorativePlan, sharedNow: number): CompanionNpcRuntime {
  if (!Number.isSafeInteger(sharedNow) || sharedNow < plan.sharedStartedAt) throw new Error("Use the shared clock at or after instance creation.");
  const simulatedAt = plan.sharedStartedAt + Math.floor((sharedNow - plan.sharedStartedAt) / companionNpcLimits.stepMs) * companionNpcLimits.stepMs;
  const npcs: CompanionNpcSample[] = plan.npcs.map(npc => {
    const phase = (simulatedAt - plan.sharedStartedAt + npc.offsetMs) % npc.cycleMs;
    const leg = npc.legs.find(leg => phase < leg.endsAt);
    const moving = !!leg && phase < leg.arrivesAt;
    const fraction = leg ? Math.min(1, Math.max(0, (phase - leg.startsAt) / (leg.arrivesAt - leg.startsAt))) : 0;
    const point: ScenePoint = leg ? [leg.from[0] + (leg.to[0] - leg.from[0]) * fraction, leg.from[1], leg.from[2] + (leg.to[2] - leg.from[2]) * fraction] : npc.home.point;
    const yaw = leg && distance(point, npc.home.point) > .001 ? Math.atan2(leg.to[0] - leg.from[0], leg.to[2] - leg.from[2]) : npc.home.yaw;
    return { id: npc.id, pose: { point, yaw }, behaviour: moving ? "walk" : npc.role === "resident" ? "idle" : npc.role === "host" ? "gather" : "work",
      moving, serviceId: null, cue: null, target: leg?.to ?? npc.home.point, targetKey: "decoration", route: [], plannedAt: simulatedAt,
      assetPartId: npc.assetPartId, issue: npc.issue };
  });
  return immutableCompanionNpcSnapshot({ instanceId: plan.instanceId, layoutRevision: plan.layoutRevision, simulatedAt, npcs,
    issues: plan.npcs.flatMap(npc => npc.issue ? [npc.issue] : []) });
}

export function companionNpcObservationIsDecorative(observation: CompanionNpcObservation): boolean {
  return observation.danger.length === 0 && observation.traffic.length === 0 && observation.queues.length === 0
    && observation.tasks.length === 0 && observation.cues.length === 0;
}

/** Predictions never authorize outcomes. Join/reconnect supplies the world owner's validated snapshot. */
export function createCompanionNpcRuntime(layout: CompanionPlaceLayout, instanceId: string, sharedStartedAt: number, snapshot?: CompanionNpcRuntime): CompanionNpcRuntime {
  if (!instanceId || !Number.isSafeInteger(sharedStartedAt) || sharedStartedAt < 0) throw new Error("Provide the shared instance ID and start time.");
  if (snapshot) {
    const ids = new Set<string>();
    if (snapshot.instanceId !== instanceId || snapshot.layoutRevision !== layout.navigation.revision || !Number.isSafeInteger(snapshot.simulatedAt)
      || snapshot.simulatedAt < sharedStartedAt || (snapshot.simulatedAt - sharedStartedAt) % companionNpcLimits.stepMs !== 0
      || layout.npcs.length > companionNpcLimits.npcs || snapshot.npcs.length !== layout.npcs.length
      || snapshot.issues.length > companionNpcLimits.npcs + 1 || snapshot.issues.some(issue => typeof issue !== "string")) throw new Error("Request a complete current NPC snapshot for this instance and layout.");
    for (const sample of snapshot.npcs) {
      const npc = layout.npcs.find(item => item.id === sample.id);
      if (!npc || ids.has(sample.id) || !finitePoint(sample.pose.point) || !Number.isFinite(sample.pose.yaw) || !finitePoint(sample.target)
        || !Number.isSafeInteger(sample.plannedAt) || sample.plannedAt < sharedStartedAt || sample.plannedAt > snapshot.simulatedAt || sample.route.length > companionNpcLimits.routePoints
        || !["idle", "walk", "work", "attend", "gather", "flee", "seek-help", "wait-traffic", "hawk", "conduct-danfo", "pos-cash", "collect-dues"].includes(sample.behaviour)
        || typeof sample.moving !== "boolean" || typeof sample.targetKey !== "string" || !sample.targetKey
        || sample.issue !== null && typeof sample.issue !== "string"
        || sample.cue !== null && (!sample.serviceId || !["prepare-food", "handover-food", "board", "recover", "release", "mission", "dance", "gather", "capture", "football", "arena", "hawk-wares", "call-passengers", "pos-transaction", "tout-settle"].includes(sample.cue))
        || sample.assetPartId !== npc.assetPartId || sample.serviceId !== null && !layout.services.some(service => service.id === sample.serviceId && service.npcId === npc.id)) throw new Error("Request a bounded NPC snapshot with valid staff and service IDs.");
      ids.add(sample.id);
      const nav = npcNavigation(layout, npc, []);
      let point = sample.pose.point;
      if (!routeSegmentClear(nav, point, point)) throw new Error("NPC snapshot needs a collision-safe pose.");
      for (const next of sample.route) {
        if (!finitePoint(next) || !routeSegmentClear(nav, point, next)) throw new Error("NPC snapshot needs collision-safe routes.");
        point = next;
      }
    }
    return immutableCompanionNpcSnapshot({ ...snapshot, npcs: [...snapshot.npcs].sort(compareId) });
  }
  const issues: string[] = [], ids = new Set<string>();
  const npcs = [...layout.npcs].sort(compareId).slice(0, companionNpcLimits.npcs).map(npc => {
    const safe = !!npc.id && !ids.has(npc.id) && finitePoint(npc.home.point) && Number.isFinite(npc.home.yaw)
      && Number.isFinite(npc.radius) && npc.radius > 0 && routeSegmentClear(npcNavigation(layout, npc, []), npc.home.point, npc.home.point);
    ids.add(npc.id);
    const issue = safe ? null : `NPC ${npc.id || "without an ID"} needs a unique ID and collision-safe home pose.`;
    if (issue) issues.push(issue);
    return { id: npc.id, pose: { point: [...npc.home.point] as ScenePoint, yaw: npc.home.yaw }, behaviour: "idle" as const,
      moving: false, serviceId: null, cue: null, target: npc.home.point, targetKey: "home", route: [], plannedAt: sharedStartedAt,
      assetPartId: npc.assetPartId, issue };
  });
  if (layout.npcs.length > companionNpcLimits.npcs) issues.push(`Only the first ${companionNpcLimits.npcs} NPC IDs are sampled. Reduce the layout population.`);
  return { instanceId, layoutRevision: layout.navigation.revision, simulatedAt: sharedStartedAt, npcs, issues };
}

function npcIntent(layout: CompanionPlaceLayout, npc: CompanionPlaceNpc, sample: CompanionNpcSample, observation: CompanionNpcObservation, now: number) {
  const dangers = observation.danger.slice(0, companionNpcLimits.observations).filter(danger => finitePoint(danger.point) && Number.isFinite(danger.radius) && danger.radius > 0).sort(compareId);
  const danger = dangers.find(item => distance(sample.pose.point, item.point) <= item.radius + npc.radius);
  if (danger) {
    const candidates = [npc.refuge, npc.home.point, ...npc.route.slice(0, companionNpcLimits.routePoints)];
    const refuge = candidates.find(point => finitePoint(point) && dangers.every(item => distance(point, item.point) > item.radius + npc.radius));
    return { point: refuge ?? sample.pose.point, key: `danger:${danger.id}`, behaviour: refuge ? "flee" as const : "seek-help" as const, serviceId: null };
  }
  const task = observation.tasks.slice(0, companionNpcLimits.observations)
    .filter(item => item.npcId === npc.id && active(item.startsAt, item.expiresAt, now) && finitePoint(item.point)).sort(compareId)[0];
  if (task) return { point: task.point, key: `task:${task.id}:${task.point.join(",")}`, behaviour: task.kind, serviceId: null };
  const service = layout.services.find(item => item.npcId === npc.id && observation.queues.slice(0, companionNpcLimits.observations)
    .some(queue => queue.serviceId === item.id && Number.isSafeInteger(queue.waiting) && queue.waiting > 0));
  if (service) {
    const queue = observation.queues.find(item => item.serviceId === service.id)!;
    return { point: npc.home.point, key: `queue:${service.id}:${queue.stock}`, behaviour: "attend" as const, serviceId: service.id };
  }
  const phase = Math.floor(now / companionNpcLimits.routineMs) + seedNumber(`${layout.seed}:${npc.id}`);
  const points = [npc.home.point, ...npc.route.slice(0, companionNpcLimits.routePoints)];
  const point = points[phase % points.length];
  const behaviour: CompanionNpcBehaviour = phase % points.length === 0 ? npc.role === "resident" ? "idle" : "work" : npc.role === "host" ? "gather" : "walk";
  return { point, key: `routine:${phase}`, behaviour, serviceId: null };
}

function stepNpc(layout: CompanionPlaceLayout, npc: CompanionPlaceNpc, previous: CompanionNpcSample, observation: CompanionNpcObservation, now: number): CompanionNpcSample {
  if (previous.issue && previous.targetKey === "home") return previous;
  const traffic = observation.traffic.slice(0, companionNpcLimits.observations).filter(validBox);
  const nav = npcNavigation(layout, npc, traffic), intent = npcIntent(layout, npc, previous, observation, now);
  let route = previous.route, plannedAt = previous.plannedAt, issue: string | null = null;
  const changed = previous.targetKey !== intent.key || distance(previous.target, intent.point) > .001;
  if (changed) route = [];
  if ((!previous.issue || now - plannedAt >= companionNpcLimits.routeRetryMs)
    && (changed || !route.length && distance(previous.pose.point, intent.point) > .01 && now - plannedAt >= companionNpcLimits.routeRetryMs)) {
    route = finitePoint(intent.point) ? planRoomRoute(nav, previous.pose.point, intent.point) ?? [] : [];
    plannedAt = now;
    if (route.length > companionNpcLimits.routePoints) route = [];
  }
  let point = previous.pose.point, yaw = previous.pose.yaw, remaining = (intent.behaviour === "flee" ? 1.5 : .9) * companionNpcLimits.stepMs / 1000;
  const nextRoute = [...route];
  // Consume a bounded polyline, checking moving traffic before each swept step.
  for (let index = 0; nextRoute.length && index < companionNpcLimits.routePoints; index++) {
    const next = nextRoute[0], length = distance(point, next);
    if (length < .001) { nextRoute.shift(); continue; }
    const fraction = Math.min(1, remaining / length);
    const destination: ScenePoint = [point[0] + (next[0] - point[0]) * fraction, nav.floorY, point[2] + (next[2] - point[2]) * fraction];
    if (!routeSegmentClear(nav, point, destination)) { nextRoute.length = 0; plannedAt = now; issue = "Route blocked. Wait for traffic or a reachable task."; break; }
    yaw = Math.atan2(next[0] - point[0], next[2] - point[2]); point = destination; remaining -= length * fraction;
    if (fraction === 1) nextRoute.shift();
    if (remaining <= 0) break;
  }
  const reached = distance(point, intent.point) <= .01;
  if (!reached && !nextRoute.length) issue ??= "No collision-safe route. Adjust the NPC target or layout.";
  const cue = reached && !issue && intent.behaviour !== "flee" && intent.behaviour !== "seek-help"
    ? observation.cues.slice(0, companionNpcLimits.observations).filter(item => item.npcId === npc.id
      && layout.services.some(service => service.id === item.serviceId && service.npcId === npc.id)
      && active(item.startsAt, item.expiresAt, now)).sort(compareId)[0] : undefined;
  return { ...previous, pose: { point, yaw: reached && distance(point, npc.home.point) < .01 ? npc.home.yaw : yaw },
    behaviour: issue ? "wait-traffic" : intent.behaviour, moving: distance(previous.pose.point, point) > .0001,
    target: intent.point, targetKey: intent.key, route: nextRoute, plannedAt, serviceId: cue?.serviceId ?? intent.serviceId, cue: cue?.kind ?? null, issue };
}

/** Replay fixed shared-time steps. Large gaps return caughtUp=false; callers resume, not skip time. */
export function sampleCompanionNpcs(layout: CompanionPlaceLayout, runtime: CompanionNpcRuntime, observation: CompanionNpcObservation, sharedNow: number, options: CompanionNpcSamplingOptions = { active: true, nearbyNpcIds: runtime.npcs.map(npc => npc.id) }): { runtime: CompanionNpcRuntime; caughtUp: boolean; error: string | null } {
  if (!Number.isSafeInteger(sharedNow) || sharedNow < runtime.simulatedAt) return { runtime, caughtUp: false, error: "Use a nondecreasing shared clock." };
  if (layout.navigation.revision !== runtime.layoutRevision) return { runtime, caughtUp: false, error: "Layout changed. Reinitialize NPCs from the instance owner's safe poses." };
  if (observation.instanceId !== runtime.instanceId || observation.layoutRevision !== runtime.layoutRevision) return { runtime, caughtUp: false, error: "NPC observations belong to another instance or layout." };
  if (!active(observation.observedAt, observation.expiresAt, sharedNow) || sharedNow - observation.observedAt > 5000
    || observation.expiresAt - observation.observedAt > 5000) return { runtime, caughtUp: false, error: "Refresh the bounded NPC observation." };
  const queues = new Set<string>();
  for (const queue of observation.queues.slice(0, companionNpcLimits.observations)) {
    if (!layout.services.some(service => service.id === queue.serviceId) || queues.has(queue.serviceId) || !Number.isSafeInteger(queue.waiting) || queue.waiting < 0 || queue.waiting > 64
      || !["available", "empty", "unknown"].includes(queue.stock)) return { runtime, caughtUp: false, error: "Provide unique queues for resolved layout services." };
    queues.add(queue.serviceId);
  }
  if (observation.traffic.slice(0, companionNpcLimits.observations).some(box => !validBox(box))) return { runtime, caughtUp: false, error: "Traffic needs finite, ordered collision footprints." };
  const nearby = new Set(options.nearbyNpcIds.slice(0, companionNpcLimits.npcs));
  if (!options.active || !nearby.size) return { runtime, caughtUp: false, error: null };
  let next = runtime;
  for (let step = 0; step < companionNpcLimits.catchupSteps && next.simulatedAt + companionNpcLimits.stepMs <= sharedNow; step++) {
    const at = next.simulatedAt + companionNpcLimits.stepMs;
    if (!active(observation.observedAt, observation.expiresAt, at)) return { runtime: next, caughtUp: false, error: "Resume from the world owner's current NPC snapshot before sampling fresh observations." };
    next = { ...next, simulatedAt: at, npcs: next.npcs.map(sample => {
      const npc = layout.npcs.find(item => item.id === sample.id);
      return npc && nearby.has(npc.id) ? stepNpc(layout, npc, sample, observation, at) : { ...sample, moving: false, cue: null };
    }) };
  }
  return { runtime: next, caughtUp: next.simulatedAt + companionNpcLimits.stepMs > sharedNow, error: null };
}
