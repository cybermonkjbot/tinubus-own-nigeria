import { planRoomRoute, routeSegmentClear, type NavigationObstacle, type RoomNavigation, type ScenePoint } from "./companionNavigation";
import { companionPolicePatrols, sampleCompanionPolicePatrol, type CompanionPolicePatrol } from "./companionPolicingPatrol";
import type { CompanionWorldInstance } from "./companionWorld";

export const companionGunfireTuning = { hearingRange: 48, fleeSpeed: 4, returnSpeed: 2, calmMs: 12_000,
  dispatchMs: 2500, dispatchVariationMs: 1500, policeSpeed: 4.2, searchMs: 15_000, incidents: 12, routePoints: 32 } as const;
export type CompanionGunfireOfficer = { id: string; from: ScenePoint; route: readonly ScenePoint[];
  returnRoute: readonly ScenePoint[]; responseAt: number; arrivesAt: number; patrolOffsetMs: number; redirectedAt?: number };
export type CompanionGunfireIncident = { id: string; actorId: string; lifeId?: string; point: ScenePoint;
  startedAt: number; lastShotAt: number; officers: readonly CompanionGunfireOfficer[] };
export type CompanionGunfireState = { incidents: readonly CompanionGunfireIncident[];
  patrolOffsets: readonly { id: string; milliseconds: number }[] };
const distance = (a: ScenePoint, b: ScenePoint) => Math.hypot(a[0] - b[0], a[2] - b[2]);
const length = (from: ScenePoint, route: readonly ScenePoint[]) => route.reduce((sum, point, index) =>
  sum + distance(index ? route[index - 1] : from, point), 0);
function along(from: ScenePoint, route: readonly ScenePoint[], metres: number): ScenePoint {
  for (const point of route) {
    const step = distance(from, point);
    if (step > metres && step > 0) return [from[0] + (point[0] - from[0]) * metres / step, from[1], from[2] + (point[2] - from[2]) * metres / step];
    metres -= step; from = point;
  }
  return from;
}
const returnsAt = (incident: CompanionGunfireIncident, officer: CompanionGunfireOfficer) =>
  Math.max(incident.lastShotAt + companionGunfireTuning.searchMs, officer.arrivesAt + companionGunfireTuning.searchMs);
const returnedAt = (incident: CompanionGunfireIncident, officer: CompanionGunfireOfficer) => Math.min(officer.redirectedAt ?? Infinity,
  returnsAt(incident, officer) + Math.ceil(length(officer.route.at(-1) ?? officer.from, officer.returnRoute) / companionGunfireTuning.policeSpeed * 1000));
export function companionGunfireSearchEndsAt(incident: CompanionGunfireIncident): number {
  return Math.max(incident.lastShotAt + companionGunfireTuning.searchMs, ...incident.officers.map(officer => returnsAt(incident, officer)));
}

/** Standby officers reuse the existing beat's safe staging points and approved rig. */
export function companionGunfirePolicePatrols(instance: CompanionWorldInstance): CompanionPolicePatrol[] {
  const patrols = companionPolicePatrols(instance), beat = patrols[0];
  const standby: ScenePoint = [-13.5, instance.navigation.floorY, -10];
  return beat && routeSegmentClear(instance.navigation, standby, standby)
    ? [...patrols, { ...beat, id: "town-square:police-response", label: "Police checkpoint support", route: [standby], yaw: Math.PI / 2 }] : patrols;
}
export function sampleCompanionGunfireOfficer(patrol: CompanionPolicePatrol, instance: CompanionWorldInstance,
  state: CompanionGunfireState | null | undefined, now: number): ScenePoint | null {
  let offset = state?.patrolOffsets.find(value => value.id === patrol.id)?.milliseconds ?? 0;
  for (const incident of state?.incidents ?? []) {
    const officer = incident.officers.find(value => value.id === patrol.id);
    if (!officer || now < officer.responseAt) continue;
    const returnAt = returnsAt(incident, officer), end = returnedAt(incident, officer);
    if (now < end && now < returnAt) return along(officer.from, officer.route, Math.max(0, now - officer.responseAt) / 1000 * companionGunfireTuning.policeSpeed);
    if (now < end) return along(officer.route.at(-1) ?? officer.from, officer.returnRoute, (now - returnAt) / 1000 * companionGunfireTuning.policeSpeed);
    offset = Math.max(offset, officer.patrolOffsetMs + end - officer.responseAt);
  }
  return sampleCompanionPolicePatrol(patrol, instance.sharedStartedAt, Math.max(instance.sharedStartedAt, now - offset), instance.navigation);
}
export function companionGunfireOfficerResponding(state: CompanionGunfireState | null | undefined, id: string, now: number): boolean {
  return !!state?.incidents.some(incident => incident.officers.some(officer => officer.id === id
    && now >= officer.responseAt && now < returnedAt(incident, officer)));
}
/** Called only for an accepted firearm attack. Coalesce bursts without resetting dispatch. */
export function recordCompanionGunfire(previous: CompanionGunfireState | undefined, instance: CompanionWorldInstance,
  shot: { id: string; actorId: string; lifeId?: string; point: ScenePoint; at: number }): CompanionGunfireState {
  const patrols = companionGunfirePolicePatrols(instance);
  const offsets = new Map((previous?.patrolOffsets ?? []).map(value => [value.id, value.milliseconds]));
  const incidents: CompanionGunfireIncident[] = [];
  for (const saved of previous?.incidents ?? []) {
    const incident = { ...saved, officers: saved.officers.map(officer => ({ ...officer })) };
    if (shot.at < companionGunfireSearchEndsAt(incident)
      || incident.officers.some(officer => shot.at < returnedAt(incident, officer))) { incidents.push(incident); continue; }
    for (const officer of incident.officers) offsets.set(officer.id, Math.max(offsets.get(officer.id) ?? 0,
      officer.patrolOffsetMs + returnedAt(incident, officer) - officer.responseAt));
  }
  const existing = incidents.find(value => value.actorId === shot.actorId && value.lifeId === shot.lifeId && shot.at - value.lastShotAt < companionGunfireTuning.calmMs
    && distance(value.point, shot.point) < 8 && value.officers.every(officer => !officer.redirectedAt && shot.at < returnsAt(value, officer)));
  if (existing) {
    // Investigate the reported location rather than knowing an unseen player's position.
    existing.lastShotAt = Math.max(existing.lastShotAt, shot.at);
  } else if (incidents.length < companionGunfireTuning.incidents) {
    let hash = 0; for (const char of shot.id) hash = Math.imul(hash, 31) + char.charCodeAt(0) | 0;
    const responseAt = Math.max(shot.at + companionGunfireTuning.dispatchMs + (hash >>> 0) % (companionGunfireTuning.dispatchVariationMs + 1),
      ...incidents.flatMap(incident => incident.officers.map(officer => officer.responseAt)));
    const officers: CompanionGunfireOfficer[] = [];
    for (const patrol of patrols) {
      const busy = incidents.find(incident => incident.officers.some(officer => officer.id === patrol.id && responseAt >= officer.responseAt
        && responseAt < returnedAt(incident, officer)));
      // Fresh reports can redirect an officer already on the way, with the same short dispatch delay.
      const from = sampleCompanionGunfireOfficer(patrol, instance,
        { incidents, patrolOffsets: [...offsets].map(([id, milliseconds]) => ({ id, milliseconds })) }, responseAt);
      const offset = offsets.get(patrol.id) ?? 0;
      if (!from) continue;
      const previousOfficer = busy?.officers.find(officer => officer.id === patrol.id);
      const returnPoint = previousOfficer?.returnRoute.at(-1) ?? from;
      const route = planRoomRoute(instance.navigation, from, shot.point), returnRoute = route && planRoomRoute(instance.navigation, shot.point, returnPoint);
      if (!route || !returnRoute || route.length > companionGunfireTuning.routePoints || returnRoute.length > companionGunfireTuning.routePoints) continue;
      officers.push({ id: patrol.id, from, route, returnRoute, responseAt,
        arrivesAt: responseAt + Math.ceil(length(from, route) / companionGunfireTuning.policeSpeed * 1000),
        patrolOffsetMs: previousOfficer ? previousOfficer.patrolOffsetMs + responseAt - previousOfficer.responseAt : offset });
      if (previousOfficer) previousOfficer.redirectedAt = responseAt;
    }
    incidents.push({ id: shot.id, actorId: shot.actorId, ...(shot.lifeId ? { lifeId: shot.lifeId } : {}),
      point: [...shot.point], startedAt: shot.at, lastShotAt: shot.at, officers });
  }
  return { incidents, patrolOffsets: [...offsets].map(([id, milliseconds]) => ({ id, milliseconds })) };
}

/** Cosmetic NPC movement from accepted reports. No damage, membership or economy writes. */
export function createCompanionGunfireReactions(navigation: RoomNavigation) {
  type Reaction = { incidentId: string; from: ScenePoint; route: readonly ScenePoint[]; startsAt: number;
    lastShotAt: number; returning: boolean; retryAt: number; yaw: number };
  const reactions = new Map<string, Reaction>();
  const actorNavigation = new Map<string, RoomNavigation>();
  const poseAt = (reaction: Reaction, now: number) => along(reaction.from, reaction.route,
    Math.max(0, now - reaction.startsAt) / 1000 * (reaction.returning ? companionGunfireTuning.returnSpeed : companionGunfireTuning.fleeSpeed));
  return {
    sample(id: string, pose: { point: ScenePoint; yaw: number }, state: CompanionGunfireState | null | undefined, now: number,
      ignoredObstacles: readonly NavigationObstacle[] = []) {
      if (!Number.isFinite(now)) return { ...pose, moving: false, fleeing: false };
      let nav = actorNavigation.get(id);
      if (!nav) {
        // Frozen city snapshots strip obstacle IDs. Match only the actor's exact authored reserve.
        nav = ignoredObstacles.length ? { ...navigation, obstacles: navigation.obstacles.filter(box => !ignoredObstacles.some(own =>
          own.minX === box.minX && own.maxX === box.maxX && own.minZ === box.minZ && own.maxZ === box.maxZ
          && own.yaw === box.yaw && own.polygon === box.polygon)) } : navigation;
        if (actorNavigation.size >= 64) actorNavigation.delete(actorNavigation.keys().next().value!);
        actorNavigation.set(id, nav);
      }
      let reaction = reactions.get(id);
      const current = reaction ? poseAt(reaction, now) : pose.point;
      const incident = state?.incidents.filter(value => now >= value.startedAt && now < value.lastShotAt + companionGunfireTuning.calmMs
        && (value.id === reaction?.incidentId || distance(current, value.point) <= companionGunfireTuning.hearingRange))
        .reduce<CompanionGunfireIncident | undefined>((latest, value) => !latest || value.lastShotAt > latest.lastShotAt ? value : latest, undefined);
      if (incident && (!reaction || reaction.returning || reaction.incidentId !== incident.id || !reaction.route.length && now >= reaction.retryAt)) {
        const heading = distance(current, incident.point) > .01 ? Math.atan2(current[0] - incident.point[0], current[2] - incident.point[2]) : id.length;
        let route: ScenePoint[] | null = null;
        for (const metres of [28, 18, 10]) {
          for (const turn of [0, -.6, .6, -1.2, 1.2]) {
            const target: ScenePoint = [current[0] + Math.sin(heading + turn) * metres, navigation.floorY, current[2] + Math.cos(heading + turn) * metres];
            if (distance(target, incident.point) <= distance(current, incident.point) + 5) continue;
            const candidate = planRoomRoute(nav, current, target);
            if (candidate && candidate.length <= companionGunfireTuning.routePoints) { route = candidate; break; }
          }
          if (route) break;
        }
        reaction = { incidentId: incident.id, from: current, route: route ?? [], startsAt: now,
          lastShotAt: incident.lastShotAt, returning: false, retryAt: now + 1500, yaw: reaction?.yaw ?? pose.yaw };
        if (reactions.size >= 64 && !reactions.has(id)) reactions.delete(reactions.keys().next().value!);
        reactions.set(id, reaction);
      }
      if (!reaction) return { ...pose, moving: false, fleeing: !!incident };
      if (incident) reaction.lastShotAt = Math.max(reaction.lastShotAt, incident.lastShotAt);
      if (!incident && distance(current, pose.point) < .1) {
        reactions.delete(id); return { ...pose, moving: false, fleeing: false };
      }
      const routeFinished = distance(current, reaction.route.at(-1) ?? reaction.from) < .01;
      if (!incident && now >= reaction.lastShotAt + companionGunfireTuning.calmMs && now >= reaction.retryAt
        && (!reaction.returning || routeFinished)) {
        const route = planRoomRoute(nav, current, pose.point);
        reaction.retryAt = now + 1500;
        if (route && route.length <= companionGunfireTuning.routePoints) {
          reaction = { ...reaction, from: current, route, startsAt: now, returning: true }; reactions.set(id, reaction);
        }
      }
      const point = poseAt(reaction, now), before = poseAt(reaction, now - 40), moving = distance(before, point) > .001;
      if (reaction.returning && distance(point, pose.point) < .1) { reactions.delete(id); return { ...pose, moving: false, fleeing: false }; }
      if (moving) reaction.yaw = Math.atan2(point[0] - before[0], point[2] - before[2]);
      return { point, yaw: reaction.yaw, moving, fleeing: !reaction.returning, incidentId: reaction.incidentId };
    },
    clear() { reactions.clear(); actorNavigation.clear(); },
  };
}
