import plan from "../../assets/city/lagos-city-v3.json";
import { navigationObstacleBounds, routeSegmentClear, type NavigationObstacle, type RoomNavigation, type ScenePoint } from "./companionNavigation";

/** Authored game geography. Transit does not request GPS, routing or traffic providers. */
export const COMPANION_CITY_TRANSIT_VERSION = "lagos-city-transit-v1";
export type CompanionTransitKind = "brt" | "danfo" | "keke";
export type TransitPoint = { x: number; z: number; yaw: number };
export type TransitRoad = { id: string; name: string; kind: string; width: number; pavement: number; points: readonly (readonly number[])[];
  /** Through corridors retain each physical road's lane and stop alignment. */
  segmentWidths?: readonly number[] };
export type TransitStop = {
  id: string; label: string; roadId: string; roadMetres: number; direction: 1 | -1;
  berth: TransitPoint; approach: ScenePoint; shelter: TransitPoint; obstacles: NavigationObstacle[];
};
export type TransitRoute = { id: string; label: string; roadId: string; points: TransitPoint[]; lengths: number[]; metres: number;
  stops: { stopId: string | null; metres: number; approach?: ScenePoint }[]; loop: boolean };
export type TransitVehicle = { id: string; kind: CompanionTransitKind; routeId: string; offsetMs: number };
export type TransitPlan = { version: typeof COMPANION_CITY_TRANSIT_VERSION; navigationRevision: string;
  stops: TransitStop[]; routes: TransitRoute[]; vehicles: TransitVehicle[] };
export type TransitPause = { id: string; startsAt: number; endsAt: number; point: TransitPoint; approach: ScenePoint; routeMetres: number };
export type TransitVehicleState = { vehicleId: string; delayMs: number; pauses: TransitPause[] };
export type TransitRide = { id: string; vehicleId: string; seat: number; boardedAt: number; visitRevision: number;
  /** Server-selected stopped exit. Null means the passenger has not requested an exit. */
  exit: { at: number; point: TransitPoint; stopId: string | null } | null };
export type TransitPassenger = { id: string; kind: "player" | "npc"; ride: TransitRide };
export type TransitSnapshot = { version: typeof COMPANION_CITY_TRANSIT_VERSION; instanceId: string; navigationRevision: string;
  epochMs: number; revision: number; states: TransitVehicleState[]; passengers: TransitPassenger[] };
export type TransitSample = { vehicleId: string; kind: CompanionTransitKind; point: TransitPoint; metres: number;
  cycle: number; stopped: boolean; stopId: string | null; approach: ScenePoint | null; doorsOpen: boolean;
  nextStopId: string | null; nextStopAt: number; phaseEndsAt: number; speed: number };
export const transitRules = Object.freeze({
  dwellMs: 18_000, roadsideDwellMs: 16_000, boardingReach: 2.1, maxPassengers: 64,
  maxPauses: 24, maxRoutes: 192, maxVehicles: 256, visibleVehicles: 24, maxStops: 48,
  kinds: Object.freeze({ brt: { seats: 28, npcSeats: 8, speed: 11, width: 2.55, length: 12, floor: .725,seatHeight:1.06,doorZ:4.805 },
    danfo: { seats: 10, npcSeats: 3, speed: 9, width: 1.9, length: 4.8, floor: .6275,seatHeight:.945,doorZ:-.05 },
    keke: { seats: 3, npcSeats: 1, speed: 7, width: 1.45, length: 2.8, floor: .4725,seatHeight:.775,doorZ:0 } }),
});
const floorPoint = (point: TransitPoint, y = 0): ScenePoint => [point.x, y, point.z];
const distance = (a: { x: number; z: number }, b: { x: number; z: number }) => Math.hypot(b.x - a.x, b.z - a.z);
export const transitRoads: readonly TransitRoad[] = plan.roads.filter(road => road.width >= 8
  && !["path", "boardwalk", "sand-path"].includes(road.kind));

export function transitRoadLength(road: TransitRoad) {
  let metres = 0;
  for (let i = 1; i < road.points.length; i++) metres += Math.hypot(road.points[i][0] - road.points[i - 1][0], road.points[i][1] - road.points[i - 1][1]);
  return metres;
}
export function transitRoadWidth(road: TransitRoad, metres: number) {
  let remaining = metres;
  for (let i=1;i<road.points.length;i++) {
    const length=Math.hypot(road.points[i][0]-road.points[i-1][0],road.points[i][1]-road.points[i-1][1]);
    if(remaining<=length||i===road.points.length-1)return road.segmentWidths?.[i-1]??road.width;
    remaining-=length;
  }
  return road.width;
}
/** Heading follows travel direction. Positive lateral distance is the door-side verge. */
export function transitRoadPoint(road: TransitRoad, metres: number, direction: 1 | -1, lateral = 0): TransitPoint {
  let remaining = Math.max(0, Math.min(transitRoadLength(road), metres));
  for (let i = 1; i < road.points.length; i++) {
    const a = road.points[i - 1], b = road.points[i], length = Math.hypot(b[0] - a[0], b[1] - a[1]);
    if (remaining > length && i < road.points.length - 1) { remaining -= length; continue; }
    const t = length ? Math.min(1, remaining / length) : 0;
    const yaw = Math.atan2((b[0] - a[0]) * direction, (b[1] - a[1]) * direction);
    return { x: a[0] + (b[0] - a[0]) * t + Math.cos(yaw) * lateral,
      z: a[1] + (b[1] - a[1]) * t - Math.sin(yaw) * lateral, yaw };
  }
  throw new Error("A transit road needs two points.");
}
function footprint(point: TransitPoint, width: number, depth: number): NavigationObstacle {
  return { minX: point.x - width / 2, maxX: point.x + width / 2,
    minZ: point.z - depth / 2, maxZ: point.z + depth / 2, yaw: point.yaw };
}
export function transitLocalPoint(point: TransitPoint, x: number, z: number, y = 0): ScenePoint {
  return [point.x + x * Math.cos(point.yaw) + z * Math.sin(point.yaw), y,
    point.z - x * Math.sin(point.yaw) + z * Math.cos(point.yaw)];
}
export function createTransitRoadRoute(road: TransitRoad, kind: CompanionTransitKind, stops: readonly TransitStop[], navigation: RoomNavigation): TransitRoute | null {
  const lengths: number[] = [0], points: TransitPoint[] = [], total = transitRoadLength(road);
  const checkpoints: TransitRoute["stops"] = [];
  const lane = (at: number) => transitRoadWidth(road,at) / 2 - transitRules.kinds[kind].width / 2 - .65;
  const swept = { ...navigation, radius: transitRules.kinds[kind].width / 2 + .25 };
  const closed = distance({ x: road.points[0][0], z: road.points[0][1] },
    { x: road.points.at(-1)![0], z: road.points.at(-1)![1] }) < .02;
  const arc = (at: number, direction: 1 | -1) => {
    const lateral=lane(at);
    const centre = transitRoadPoint(road, at, direction);
    return Array.from({ length: 13 }, (_, step) => {
      const angle = step * Math.PI / 12, p = transitLocalPoint(centre, lateral * Math.cos(angle), lateral * Math.sin(angle));
      return { x: p[0], z: p[2], yaw: centre.yaw + angle };
    });
  };
  // Turn only at clear road sections. Planted medians and roadside structures are retained.
  const terminal = (direction: 1 | -1) => {
    for (const inset of [35, 55, 75, 95, 115, 135]) {
      if (inset >= total / 3) break;
      const at = direction === 1 ? total - inset : inset, values = arc(at, direction);
      if (values.slice(1).every((p, i) => routeSegmentClear(swept, floorPoint(values[i], navigation.floorY), floorPoint(p, navigation.floorY)))) return at;
    }
    return null;
  };
  const first = closed ? 0 : terminal(-1), last = closed ? total : terminal(1);
  if (first === null || last === null || first >= last) return null;
  const samples: number[] = [first, last, ...stops.map(stop => stop.roadMetres), total * .2, total * .8];
  for(let at=first+16;at<last;at+=16)samples.push(at);
  let metres = 0;
  for (let i = 1; i < road.points.length; i++) {
    metres += Math.hypot(road.points[i][0] - road.points[i - 1][0], road.points[i][1] - road.points[i - 1][1]);
    if (metres > first && metres < last) samples.push(metres);
  }
  const positions = [...new Set(samples)].filter(at => at >= first && at <= last).sort((a, b) => a - b);
  const obstacleBounds=navigation.obstacles.map(obstacle=>({obstacle,bounds:navigationObstacleBounds(obstacle)}));
  const add = (point: TransitPoint) => {
    if(points.length&&distance(points.at(-1)!,point)<.001)return;
    if (points.length) lengths.push(lengths.at(-1)! + distance(points.at(-1)!, point));
    points.push(point);
  };
  for (const direction of (closed ? [1] : [1, -1]) as readonly (1 | -1)[]) {
    const ordered=direction===1?positions:[...positions].reverse();
    // Move within the travel lane around retained fixtures. Stops and turnbacks keep their exact berth.
    type Choice={point:TransitPoint;cost:number;previous:number};
    const choices:Choice[][]=[];
    for(let index=0;index<ordered.length;index++){
      const at=ordered[index],preferred=lane(at),fixed=index===0||index===ordered.length-1
        ||stops.some(stop=>stop.direction===direction&&stop.roadMetres===at);
      const centre=transitRoadPoint(road,at,direction),previousCentre=transitRoadPoint(road,ordered[Math.max(0,index-1)],direction);
      const margin=Math.max(transitRoadWidth(road,at),transitRoadWidth(road,ordered[Math.max(0,index-1)]))+5;
      const section={...swept,obstacles:obstacleBounds.filter(({bounds:b})=>b.maxX>=Math.min(centre.x,previousCentre.x)-margin
        &&b.minX<=Math.max(centre.x,previousCentre.x)+margin&&b.maxZ>=Math.min(centre.z,previousCentre.z)-margin
        &&b.minZ<=Math.max(centre.z,previousCentre.z)+margin).map(value=>value.obstacle)};
      const preferredPoint=transitRoadPoint(road,at,direction,preferred);
      const previousPoint=transitRoadPoint(road,ordered[Math.max(0,index-1)],direction,lane(ordered[Math.max(0,index-1)]));
      const nextAt=ordered[Math.min(ordered.length-1,index+1)],nextPoint=transitRoadPoint(road,nextAt,direction,lane(nextAt));
      const preferredClear=routeSegmentClear(section,floorPoint(previousPoint,navigation.floorY),floorPoint(preferredPoint,navigation.floorY))
        &&routeSegmentClear(swept,floorPoint(preferredPoint,navigation.floorY),floorPoint(nextPoint,navigation.floorY));
      const offsets=fixed||preferredClear?[preferred]:[preferred,...[.75,1.5,2.25,3,4,5,6,7,8].map(shift=>preferred-shift).filter(value=>value>=transitRules.kinds[kind].width/2+.3)];
      const candidates:Choice[]=offsets.map(offset=>({point:transitRoadPoint(road,at,direction,offset),cost:Infinity,previous:-1}));
      for(const candidate of candidates){
        if(!routeSegmentClear(section,floorPoint(candidate.point,navigation.floorY),floorPoint(candidate.point,navigation.floorY)))continue;
        const penalty=Math.abs(distance(candidate.point,transitRoadPoint(road,at,direction,preferred)))*.02;
        if(index===0){candidate.cost=penalty;continue;}
        choices[index-1].forEach((previous,j)=>{
          if(!Number.isFinite(previous.cost)||!routeSegmentClear(section,floorPoint(previous.point,navigation.floorY),floorPoint(candidate.point,navigation.floorY)))return;
          const cost=previous.cost+distance(previous.point,candidate.point)+penalty;
          if(cost<candidate.cost){candidate.cost=cost;candidate.previous=j;}
        });
      }
      if(candidates.every(choice=>!Number.isFinite(choice.cost)))return null;
      choices.push(candidates);
    }
    let selected=choices.at(-1)!.reduce((best,choice,i,all)=>choice.cost<all[best].cost?i:best,0);
    const selectedPoints:TransitPoint[]=new Array(ordered.length);
    for(let index=ordered.length-1;index>=0;index--){const choice=choices[index][selected];selectedPoints[index]=choice.point;selected=choice.previous;}
    for (let index=0;index<ordered.length;index++) {
      const at=ordered[index];
      const stop = stops.find(value => value.direction === direction && value.roadMetres === at);
      const checkpoint=!!stop||kind!=="brt"&&(at===total*.2||at===total*.8);
      if(!checkpoint&&index>0&&index<ordered.length-1){
        const a=points.at(-1)??selectedPoints[index-1],b=selectedPoints[index],c=selectedPoints[index+1];
        const length=distance(a,c),cross=Math.abs((b.x-a.x)*(c.z-a.z)-(b.z-a.z)*(c.x-a.x));
        if(length>.001&&cross/length<.001)continue;
      }
      add(selectedPoints[index]);
      if (stop || kind !== "brt" && (at === total * .2 || at === total * .8)) checkpoints.push({ stopId: stop?.id ?? null,
        metres: lengths.at(-1)!, approach: stop?.approach ?? floorPoint(transitRoadPoint(road, at, direction, road.width / 2 + .8)) });
    }
    // Continuous in-road turnbacks. Invalid arcs are rejected with the rest of the route.
    if (!closed) for (const point of arc(direction === 1 ? last : first, direction).slice(1)) add(point);
  }
  add({ ...points[0] });
  return { id: `${kind}:${road.id}`, label: road.name, roadId: road.id, points, lengths, metres: lengths.at(-1)!, loop: true,
    stops: checkpoints.sort((a, b) => a.metres - b.metres) };
}

/** Only admit shelters with an unobstructed boarding apron and footway. No occupied world is rewritten. */
export function createTransitPlan(navigation: RoomNavigation, roads: readonly TransitRoad[] = transitRoads): TransitPlan {
  const stops: TransitStop[] = [], routes: TransitRoute[] = [], vehicles: TransitVehicle[] = [];
  // Broad avenues and neighbourhood collector streets serve every district. Never stop on bridges.
  const brtRoadIds = new Set(["market-road", "garden-avenue", "unity-avenue", "palm-crescent",
    "cross-town-road", "marina-drive", "financial-avenue", "civic-boulevard", "lagoon-gardens-drive",
    "theatre-marina-approach", "theatre-ring-drive", "industrial-harbour-approach", "refinery-ring-road"]);
  for (const road of roads) {
    const length = transitRoadLength(road);
    if (length < 100 || road.kind === "bridge") continue;
    const roadStops: TransitStop[] = [];
    const closed=distance({x:road.points[0][0],z:road.points[0][1]},{x:road.points.at(-1)![0],z:road.points.at(-1)![1]})<.02;
    if (brtRoadIds.has(road.id)) for (const direction of (closed?[1]:[1,-1]) as readonly (1|-1)[]) {
      for (const fraction of closed?[.25,.65]:["market-road","garden-avenue","unity-avenue"].includes(road.id)?[.35,.75]:[.35]) {
        if(stops.length>=transitRules.maxStops)break;
        const metres=length*fraction;
        let candidate: TransitStop | null = null;
        // Move a proposed shelter away from trees, utilities and property boundaries, never delete them.
        for (const shift of [0, 16, -16, 32, -32, 48, -48]) {
          const at = metres + shift;
          if (at < 40 || at > length - 40) continue;
          const verge = transitRoadPoint(road, at, direction, road.width / 2 + 1.65);
          // The source approach socket is local Z -4.7. Align it with the bus front door +4.805.
          const centre = transitLocalPoint(verge,0,transitRules.kinds.brt.doorZ+4.7);
          const shelter = {x:centre[0],z:centre[2],yaw:verge.yaw};
          const berth = transitRoadPoint(road, at, direction, road.width / 2 - 1.925);
          const approach = transitLocalPoint(shelter, -1.4, -4.7, navigation.floorY);
          const corners = [[-1.3,-5.2],[1.3,-5.2],[1.3,5.2],[-1.3,5.2]].map(([x,z])=>transitLocalPoint(shelter,x,z,navigation.floorY));
          if (corners.some((point, index) => !routeSegmentClear(navigation, point, corners[(index + 1) % corners.length]))
            || !routeSegmentClear(navigation, approach, transitLocalPoint(shelter, -1.4, 0, navigation.floorY))) continue;
          const obstacles = [-3.8, 3.8].flatMap(z => [-1, 1].map(x => footprint(
            { x: transitLocalPoint(shelter, x, z)[0], z: transitLocalPoint(shelter, x, z)[2], yaw: shelter.yaw }, .16, .16)));
          obstacles.push(footprint({ x: transitLocalPoint(shelter, .72, 1.3)[0],
            z: transitLocalPoint(shelter, .72, 1.3)[2], yaw: shelter.yaw }, .55, 3.2));
          obstacles.push(footprint({x:transitLocalPoint(shelter,1.05,0)[0],z:transitLocalPoint(shelter,1.05,0)[2],yaw:shelter.yaw},.08,8.4),
            footprint({x:transitLocalPoint(shelter,.65,-3)[0],z:transitLocalPoint(shelter,.65,-3)[2],yaw:shelter.yaw},.48,.48));
          const proposed = { ...navigation, obstacles: [...navigation.obstacles, ...stops.flatMap(stop => stop.obstacles), ...obstacles] };
          if (!routeSegmentClear(proposed, approach, transitLocalPoint(shelter, -1.4, 6.8, navigation.floorY))
            || navigation.streetGraph?.edges.some(([a,b]) => !routeSegmentClear({ ...navigation, obstacles },
              navigation.streetGraph!.points[a], navigation.streetGraph!.points[b]))) continue;
          candidate = { id: `brt-stop:${road.id}:${direction}:${Math.round(at)}`, label: `${road.name} ${roadStops.length + 1}`,
            roadId: road.id, roadMetres: at, direction, berth, approach, shelter, obstacles };
          break;
        }
        if (candidate) { stops.push(candidate); roadStops.push(candidate); }
      }
    }
      for (const kind of ["brt", "danfo", "keke"] as const) {
        if (kind === "brt" && roadStops.length < 2) continue;
        if (kind === "brt" && road.width < 10) continue;
        if (routes.length >= transitRules.maxRoutes) break;
        const route = createTransitRoadRoute(road, kind, kind === "brt" ? roadStops : [], navigation);
        if (!route) continue;
        // A swept vehicle disc excludes parcels, fixtures and water. Invalid services stay absent.
        if (route.points.some((point, index) => index > 0 && !routeSegmentClear({...navigation,radius:transitRules.kinds[kind].width/2+.25},
          floorPoint(route.points[index - 1], navigation.floorY), floorPoint(point, navigation.floorY)))) continue;
        routes.push(route);
        if (vehicles.length < transitRules.maxVehicles) vehicles.push({ id: `${kind}:${road.id}`, kind, routeId: route.id, offsetMs: kind === "brt" ? 0 : 25_000 });
        if (kind === "brt" && vehicles.length < transitRules.maxVehicles) vehicles.push({ id: `brt:${road.id}:second`, kind,
          routeId: route.id, offsetMs: Math.round((route.metres / transitRules.kinds.brt.speed * 1000 + route.stops.length * transitRules.dwellMs) / 2) });
      }
  }
  // Every admitted road gets local roadside services. The renderer mounts only nearby vehicles.
  const servedStops=new Set(routes.flatMap(route=>route.stops.flatMap(stop=>stop.stopId?[stop.stopId]:[])));
  return { version: COMPANION_CITY_TRANSIT_VERSION, navigationRevision: navigation.revision,
    stops:stops.filter(stop=>servedStops.has(stop.id)), routes, vehicles };
}
export function transitRoutePoint(route: TransitRoute, metres: number): TransitPoint {
  const at = Math.max(0, Math.min(route.metres, metres));
  for (let i = 1; i < route.points.length; i++) if (at <= route.lengths[i] || i === route.points.length - 1) {
    const a = route.points[i - 1], b = route.points[i], t = (at - route.lengths[i - 1]) / Math.max(.001, route.lengths[i] - route.lengths[i - 1]);
    return { x: a.x + (b.x - a.x) * t, z: a.z + (b.z - a.z) * t, yaw: Math.atan2(b.x - a.x, b.z - a.z) };
  }
  return { ...route.points[0] };
}

/** A schedule is displayable without periodic writes. Authority always passes its own clock. */
export function sampleTransitVehicle(plan: TransitPlan, vehicle: TransitVehicle, epochMs: number, now: number,
  state?: TransitVehicleState): TransitSample | null {
  const route = plan.routes.find(value => value.id === vehicle.routeId);
  return sampleTransitRoute(plan, route, vehicle, epochMs, now, state);
}
function sampleTransitRoute(plan: TransitPlan, route: TransitRoute | undefined, vehicle: TransitVehicle,
  epochMs: number, now: number, state?: TransitVehicleState): TransitSample | null {
  if (!route || !Number.isFinite(now) || now < epochMs) return null;
  const rules = transitRules.kinds[vehicle.kind];
  const travelMs = route.metres / rules.speed * 1000, dwell = vehicle.kind === "brt" ? transitRules.dwellMs : transitRules.roadsideDwellMs;
  const duration = travelMs + route.stops.length * dwell;
  let active: TransitPause | undefined, pending: TransitPause | undefined, delay = state?.delayMs ?? 0;
  if (state) for (const pause of state.pauses) {
    if (pause.startsAt <= now) {
      if (!active && now < pause.endsAt) active = pause;
      // Keep the reduction's arithmetic and input order, including overlapping pauses.
      delay = delay + Math.min(now, pause.endsAt) - pause.startsAt;
    } else if (pause.startsAt > now && !pending) pending = pause;
  }
  const pendingAt = pending?.startsAt ?? Infinity;
  const elapsed = Math.max(0, now - epochMs + vehicle.offsetMs - delay), cycle = Math.floor(elapsed / duration);
  let phase = elapsed % duration, travelled = 0;
  const base = { vehicleId: vehicle.id, kind: vehicle.kind, cycle, speed: rules.speed };
  if (active) return { ...base, point: active.point, metres: active.routeMetres, stopped: true, stopId: null,
    approach: active.approach, doorsOpen: true, nextStopId: null, nextStopAt: active.endsAt, phaseEndsAt: active.endsAt, speed: 0 };
  for (let index = 0; index <= route.stops.length; index++) {
    const stop = route.stops[index], stopId = stop ? stop.stopId : null, metres = stop ? stop.metres : route.metres;
    const leg = (metres - travelled) / rules.speed * 1000;
    if (phase < leg) return { ...base, point: transitRoutePoint(route, travelled + phase * rules.speed / 1000),
      metres: travelled + phase * rules.speed / 1000, stopped: false, stopId: null, approach: null, doorsOpen: false,
      nextStopId: stopId, nextStopAt: now + leg - phase, phaseEndsAt: Math.min(pendingAt, now + leg - phase) };
    phase -= leg;
    if (metres === route.metres) break;
    if (phase < dwell) {
      const stopRecord = plan.stops.find(value => value.id === stopId);
      return { ...base, point: transitRoutePoint(route, metres), metres, stopped: true,
        stopId, approach: stopRecord?.approach ?? stop?.approach ?? null, doorsOpen: true,
        nextStopId: stopId, nextStopAt: now, phaseEndsAt: Math.min(pendingAt, now + dwell - phase), speed: 0 };
    }
    phase -= dwell; travelled = metres;
  }
  return null;
}
const brtSeatX = [-.94, -.48, .48, .94] as const, brtSeatZ = [2.65, 1.65, .65, -1.9, -2.9, -3.9, -4.9] as const;
const danfoSeatX = [-.5, 0, .5] as const, danfoSeatZ = [.05, -.89, -1.72] as const;
export function transitSeatPoint(sample: Pick<TransitSample,"kind"|"point">, seat: number): ScenePoint {
  const rules = transitRules.kinds[sample.kind], columns = sample.kind === "brt" ? 4 : 3;
  const row = Math.floor(seat / columns), column = seat % columns;
  // Original cushion centres, excluding the driver. The Danfo front bench has one passenger beside the driver.
  const x = sample.kind === "keke" ? (column-1)*.32 : sample.kind === "brt" ? brtSeatX[column]
    : seat===0?.5:danfoSeatX[(seat-1)%3];
  const z = sample.kind === "brt" ? brtSeatZ[row]
    : sample.kind === "danfo" ? seat===0?1.18:danfoSeatZ[Math.floor((seat-1)/3)] : -.71;
  return transitLocalPoint(sample.point, x, z, rules.floor);
}
export type TransitNpcActor = { id: string; vehicleId: string; seat: number; phase: "waiting" | "boarding" | "riding" | "alighting";
  point: ScenePoint; yaw: number; seated: boolean };
/** Non-economic riders move from the apron through the doorway and leave at a later stop. */
export function transitNpcActors(plan: TransitPlan, vehicle: TransitVehicle, epochMs: number, now: number, state?: TransitVehicleState): TransitNpcActor[] {
  const route = plan.routes.find(value=>value.id===vehicle.routeId);
  const sample = sampleTransitRoute(plan, route, vehicle, epochMs, now, state);
  return sampleTransitNpcActors(plan, route, vehicle, sample, epochMs, now, state);
}
/** Renderer presentation samples a vehicle and its riders together, without accepting a caller-supplied pose. */
export function sampleTransitPresentation(plan: TransitPlan, vehicle: TransitVehicle, epochMs: number, now: number,
  state?: TransitVehicleState): { sample: TransitSample | null; actors: TransitNpcActor[] } {
  const route = plan.routes.find(value=>value.id===vehicle.routeId);
  const sample = sampleTransitRoute(plan, route, vehicle, epochMs, now, state);
  return { sample, actors: sampleTransitNpcActors(plan, route, vehicle, sample, epochMs, now, state) };
}
function sampleTransitNpcActors(plan: TransitPlan, route: TransitRoute | undefined, vehicle: TransitVehicle,
  sample: TransitSample | null, epochMs: number, now: number, state?: TransitVehicleState): TransitNpcActor[] {
  if(!sample||!route||route.stops.length<2)return [];
  const first=route.stops[0],last=route.stops.at(-1)!,rules=transitRules.kinds[vehicle.kind];
  const firstStop=plan.stops.find(stop=>stop.id===first.stopId),lastStop=plan.stops.find(stop=>stop.id===last.stopId);
  const firstApproach=firstStop?.approach??first.approach;
  const lastApproach=lastStop?.approach??last.approach;
  if(!firstApproach||!lastApproach)return [];
  const dwell=vehicle.kind==="brt"?transitRules.dwellMs:transitRules.roadsideDwellMs;
  let pauseDelay = 0;
  if (state) for (const pause of state.pauses) if (pause.startsAt <= now)
    pauseDelay = pauseDelay + Math.min(now, pause.endsAt) - pause.startsAt;
  // NPC scheduling adds the stored delay after the pause reduction.
  const delay=(state?.delayMs??0)+pauseDelay;
  const duration=route.metres/rules.speed*1000+route.stops.length*dwell;
  const cycleStart=epochMs-vehicle.offsetMs+sample.cycle*duration+delay;
  const boardAt=cycleStart+first.metres/rules.speed*1000;
  const leaveAt=cycleStart+last.metres/rules.speed*1000+(route.stops.length-1)*dwell;
  if(now<boardAt-12_000||now>leaveAt+dwell)return [];
  let firstYaw: number | undefined, lastYaw: number | undefined, door: ScenePoint | undefined;
  return Array.from({length:rules.npcSeats},(_,index)=>{
    const seat=rules.seats-1-index;
    const starts=boardAt+index*1000,leaves=leaveAt+index*800;
    let phase:TransitNpcActor["phase"]="riding",point:ScenePoint,seated=true,yaw=sample.point.yaw;
    if(now<starts||now<starts+2500){
      firstYaw??=firstStop?.berth.yaw??transitRoutePoint(route,first.metres).yaw;
      const queue=transitLocalPoint({x:firstApproach[0],z:firstApproach[2],yaw:firstYaw},0,index*.48,firstApproach[1]);
      seated=false;
      if(now<starts){phase="waiting";point=queue;yaw=firstYaw;}
      else{
        phase="boarding";
        door??=transitLocalPoint(sample.point,rules.width/2+.1,rules.doorZ,rules.floor);
        point=transitPassengerPath([queue,door,transitSeatPoint(sample,seat)],(now-starts)/2500);
      }
    }else if(now>=leaves){
      phase="alighting";seated=false;const t=Math.min(1,(now-leaves)/2500);
      lastYaw??=lastStop?.berth.yaw??transitRoutePoint(route,last.metres).yaw;
      yaw=lastYaw;
      const seatPoint=transitSeatPoint(sample,seat);
      const end=transitLocalPoint({x:lastApproach[0],z:lastApproach[2],yaw:lastYaw},0,index*.48,lastApproach[1]);
      door??=transitLocalPoint(sample.point,rules.width/2+.1,rules.doorZ,rules.floor);
      point=transitPassengerPath([seatPoint,door,end],t);
    }else point=transitSeatPoint(sample,seat);
    return{id:`transit-npc:${vehicle.id}:${sample.cycle}:${index}`,vehicleId:vehicle.id,seat,phase,point,yaw,seated};
  });
}
export function transitPassengerPath(points:readonly ScenePoint[],progress:number):ScenePoint{
  const at=Math.max(0,Math.min(1,progress))*(points.length-1),index=Math.min(points.length-2,Math.floor(at)),t=at-index;
  const a=points[index],b=points[index+1];
  return[a[0]+(b[0]-a[0])*t,a[1]+(b[1]-a[1])*t,a[2]+(b[2]-a[2])*t];
}
/** Safe pavement projection at any road distance, without requiring a formal stop. */
export function transitRoadside(navigation: RoomNavigation, position: { x: number; z: number }, kind: "danfo" | "keke",
  roads: readonly TransitRoad[] = transitRoads) {
  let best: { road: TransitRoad; metres: number; direction: 1 | -1; point: TransitPoint; approach: ScenePoint; distance: number } | null = null;
  for (const road of roads) {
    if (road.kind === "bridge") continue;
    let prefix = 0;
    for (let i = 1; i < road.points.length; i++) {
      const a = road.points[i - 1], b = road.points[i], dx = b[0] - a[0], dz = b[1] - a[1], length = Math.hypot(dx, dz);
      const t = Math.max(0, Math.min(1, ((position.x - a[0]) * dx + (position.z - a[1]) * dz) / Math.max(.001, length * length)));
      const at = prefix + t * length;
      prefix += length;
      if (at < 35 || at > transitRoadLength(road) - 35) continue;
      for (const direction of [1, -1] as const) {
        const approach = floorPoint(transitRoadPoint(road, at, direction, road.width / 2 + .8), navigation.floorY);
        const point = transitRoadPoint(road, at, direction, road.width / 2 - transitRules.kinds[kind].width / 2 - .65);
        const d = Math.hypot(position.x - approach[0], position.z - approach[2]);
        if (best && d >= best.distance || !routeSegmentClear(navigation, approach, approach)) continue;
        best = { road, metres: at, direction, point, approach, distance: d };
      }
    }
  }
  return best;
}

/** Named resident actors keep their existing owner. These are bounded, non-economic service riders. */
export function transitNpcPassengers(plan: TransitPlan, vehicles: readonly TransitVehicle[], epochMs: number, now: number,
  states: readonly TransitVehicleState[] = []): TransitPassenger[] {
  return vehicles.flatMap(vehicle => {
    return transitNpcActors(plan,vehicle,epochMs,now,states.find(state=>state.vehicleId===vehicle.id)).filter(actor=>actor.phase==="riding")
      .map(actor=> ({ id: actor.id, kind: "npc" as const,
      ride: { id: `npc-ride:${actor.id}`, vehicleId: vehicle.id, seat: actor.seat,
        boardedAt: epochMs, visitRevision: 0, exit: null } }));
  }).slice(0, transitRules.maxPassengers);
}
