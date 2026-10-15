import plan from "../../assets/city/lagos-city-v3.json";
import { connectedCityData, streetDensityCityData } from "./companionCityData";
import { navigationObstacleBounds, routeSegmentClear, type NavigationObstacle, type RoomNavigation, type ScenePoint } from "./companionNavigation";
import { townSquareNavigation } from "./companionTownSquare";
import { companionCityLandscapeAdditionalFootprints, companionCityLandscapeReservedFootprint } from "./companionCityLandscapeDetails";
import { createCompanionCityStreetDetailsPlan } from "./companionCityStreetDetails";
import { COMPANION_CITY_ROAD_REVISION, createCompanionCityRoadTreatments, companionCityRoadGraph, type CityRoadTreatments } from "./companionCityRoadTreatments";
export { COMPANION_CITY_ROAD_REVISION } from "./companionCityRoadTreatments";
import { COMPANION_CITY_PARK_REVISION, createCompanionCityParkPlan, companionCityParkGraph, companionCityParkRailObstacles, type CityParkPlan } from "./companionCityParks";
export { COMPANION_CITY_PARK_REVISION } from "./companionCityParks";
import { COMPANION_CITY_THEATRE_REVISION, companionCityTheatre, companionCityTheatreFootprint, companionCityTheatreGraph, companionCityTheatreObstacles, theatrePolygonObstacle } from "./companionCityTheatre";
export { COMPANION_CITY_THEATRE_REVISION } from "./companionCityTheatre";
import { COMPANION_CITY_CONNECTED_REVISION, createConnectedCityTransitPlan } from "./companionCityTransitNetwork";
import { companionCityRefinery, companionCityRefineryGraph, companionCityRefineryObstacle } from "./companionCityRefinery";
import {transitRules,type TransitPlan} from "./companionCityTransit";
import { COMPANION_CITY_STREET_DENSITY_REVISION, createCityStreetDensityPlan, cityStreetDensityGraph, cityMiniParkObstacle, type CityStreetDensityPlan } from "./companionCityMiniParks";
export { COMPANION_CITY_STREET_DENSITY_REVISION } from "./companionCityMiniParks";
export { COMPANION_CITY_CONNECTED_REVISION, companionCityTransitRoads } from "./companionCityTransitNetwork";

export type CompanionCityParcel = typeof plan.parcels[number];
/** Fictional metric city plan. These coordinates are not surveyed Lagos streets. */
export const companionCityPlan = plan;
export const COMPANION_CITY_VERSION = plan.version;
// Layout identity and saved Home addresses remain v3. Only newly created collision records advance.
export const COMPANION_CITY_DETAIL_REVISION = `${plan.version}:details-v1`;
export const isCompanionCity = (navigation?: RoomNavigation) => navigation?.revision === COMPANION_CITY_VERSION
  || navigation?.revision === COMPANION_CITY_DETAIL_REVISION || navigation?.revision === COMPANION_CITY_ROAD_REVISION
  || navigation?.revision === COMPANION_CITY_PARK_REVISION || navigation?.revision === COMPANION_CITY_THEATRE_REVISION
  || navigation?.revision === COMPANION_CITY_CONNECTED_REVISION || navigation?.revision === COMPANION_CITY_STREET_DENSITY_REVISION;
export const companionCityHasStreetDensity = (navigation?: RoomNavigation) => navigation?.revision === COMPANION_CITY_STREET_DENSITY_REVISION;
export const companionCityHasTransit = (navigation?: RoomNavigation) => navigation?.revision === COMPANION_CITY_CONNECTED_REVISION
  || companionCityHasStreetDensity(navigation);
export const companionCityHasRefinery = companionCityHasTransit;
export const companionCityHasTheatre = (navigation?: RoomNavigation) => navigation?.revision === COMPANION_CITY_THEATRE_REVISION
  || companionCityHasTransit(navigation);
export const companionCityHasParkPaths = (navigation?: RoomNavigation) => navigation?.revision === COMPANION_CITY_PARK_REVISION
  || companionCityHasTheatre(navigation);
export const companionCityHasRoadTreatments = (navigation?: RoomNavigation) => navigation?.revision === COMPANION_CITY_ROAD_REVISION
  || companionCityHasParkPaths(navigation);
export const companionCityHasDetailCollision = (navigation?: RoomNavigation) => navigation?.revision === COMPANION_CITY_DETAIL_REVISION
  || companionCityHasRoadTreatments(navigation);

export function companionCityParcelPoint(p: CompanionCityParcel, x: number, z: number): ScenePoint {
  const c = Math.cos(p.yaw), s = Math.sin(p.yaw);
  return [p.x + x * c + z * s, 0, p.z - x * s + z * c];
}

export function companionCityParcelObstacle(p: CompanionCityParcel): NavigationObstacle {
  return { minX: p.x - p.width / 2 - p.garden - .15, maxX: p.x + p.width / 2 + p.garden + .15,
    minZ: p.z - p.depth / 2, maxZ: p.z + p.depth / 2, yaw: p.yaw };
}

/** Same piecewise banks used by sand geometry, water shading and the local map. */
export function companionCityBankZ(bank: readonly (readonly number[])[], x: number): number {
  for (let i = 1; i < bank.length; i++) if (x <= bank[i][0]) {
    const a = bank[i-1], b = bank[i];
    return a[1]+(b[1]-a[1])*Math.max(0,Math.min(1,(x-a[0])/(b[0]-a[0])));
  }
  return bank[bank.length-1][1];
}

function waterObstacles(): NavigationObstacle[] {
  const { southBank, northBank } = plan.lagoon;
  const corridors = plan.bridges.map(b => [b.x-b.width/2-3, b.x+b.width/2+3]);
  const cuts = [...new Set([plan.bounds.minX, plan.bounds.maxX,
    ...southBank.map(p=>p[0]), ...northBank.map(p=>p[0]), ...corridors.flat()])]
    .filter(x=>x>=plan.bounds.minX&&x<=plan.bounds.maxX).sort((a,b)=>a-b);
  const result: NavigationObstacle[] = [];
  for (let i = 1; i < cuts.length; i++) {
    const a = cuts[i-1], b = cuts[i], middle = (a+b)/2;
    if (corridors.some(([low, high])=>middle>low&&middle<high)) continue;
    const polygon: [number, number][] = [[a,companionCityBankZ(southBank,a)], [b,companionCityBankZ(southBank,b)],
      [b,companionCityBankZ(northBank,b)], [a,companionCityBankZ(northBank,a)]];
    result.push({ minX:a, maxX:b, minZ:Math.min(...polygon.map(p=>p[1])), maxZ:Math.max(...polygon.map(p=>p[1])), polygon });
  }
  return result;
}

const navigations = new Map<string, RoomNavigation>();
const transitPlans = new Map<string, TransitPlan>();
export function companionCityTransitPlan(navigation: RoomNavigation): TransitPlan | null {
  if (!companionCityHasTransit(navigation)) return null;
  if (!transitPlans.has(navigation.revision)) companionCityNavigation(navigation.revision);
  return transitPlans.get(navigation.revision) ?? null;
}
let detailObstacles: NavigationObstacle[] | undefined;
let roadTreatments: CityRoadTreatments | undefined;
let parkPlan: CityParkPlan | undefined;
let densityPlan: CityStreetDensityPlan | undefined;
export function companionCityStreetDensity() {
  return densityPlan ??= createCityStreetDensityPlan(companionCityNavigation(COMPANION_CITY_CONNECTED_REVISION),
    companionCityRoadTreatments().props.filter(prop => prop.kind === "light"));
}
export function companionCityParks() {
  return parkPlan ??= createCompanionCityParkPlan(companionCityNavigation(COMPANION_CITY_ROAD_REVISION));
}
export function companionCityRoadTreatments() {
  return roadTreatments ??= createCompanionCityRoadTreatments(companionCityNavigation(COMPANION_CITY_DETAIL_REVISION));
}
function cityDetailObstacles() {
  // Shallow kerbs stay flush with the current flat-floor renderer.
  return detailObstacles ??= [...createCompanionCityStreetDetailsPlan().streets.flatMap(street => street.fixtures.flatMap(f => f.footprint ? [f.footprint] : [])),
    ...companionCityLandscapeAdditionalFootprints().map(({ minX, maxX, minZ, maxZ, yaw }) => ({ minX, maxX, minZ, maxZ, yaw }))];
}
/** Shared, versioned collision and street topology. Never expands an existing frozen game. */
export function companionCityNavigation(revision = COMPANION_CITY_STREET_DENSITY_REVISION, authoring = false): RoomNavigation {
  if (![COMPANION_CITY_VERSION, COMPANION_CITY_DETAIL_REVISION, COMPANION_CITY_ROAD_REVISION, COMPANION_CITY_PARK_REVISION, COMPANION_CITY_THEATRE_REVISION, COMPANION_CITY_CONNECTED_REVISION, COMPANION_CITY_STREET_DENSITY_REVISION].includes(revision)) throw new Error("Unknown city collision revision.");
  const cached = navigations.get(revision);
  if (cached) return cached;
  if (revision === COMPANION_CITY_STREET_DENSITY_REVISION && authoring) {
    const base = companionCityNavigation(COMPANION_CITY_CONNECTED_REVISION), density = companionCityStreetDensity();
    const nav = { ...base, revision, obstacles: [...base.obstacles, ...density.obstacles] };
    const navigation = { ...nav, streetGraph: cityStreetDensityGraph(base, nav, density.parks) };
    if (navigation.streetGraph.points.length > 2048 || navigation.streetGraph.edges.length > 8192)
      throw new Error("Street density exceeds the active navigation graph budget.");
    const transit = companionCityTransitPlan(base)!;
    transitPlans.set(revision, { ...transit, navigationRevision: revision });
    navigations.set(revision, navigation); return navigation;
  }
  if ((revision === COMPANION_CITY_CONNECTED_REVISION || revision === COMPANION_CITY_STREET_DENSITY_REVISION) && !authoring) {
    const recipe = revision === COMPANION_CITY_STREET_DENSITY_REVISION ? streetDensityCityData() : connectedCityData();
    if(recipe.revision!==revision||recipe.navigation.revision!==revision||recipe.transit.navigationRevision!==revision)
      throw new Error("The connected city data differs from its revision.");
    // JSON arrays have no tuple type. Preserve their coordinates through the typed boundary.
    const scenePoint = (point: readonly number[]): ScenePoint => {
      if (point.length !== 3 || !point.every(Number.isFinite)) throw new Error("Invalid connected city point.");
      return [point[0], point[1], point[2]];
    };
    const navigation: RoomNavigation = { ...recipe.navigation,
      obstacles: recipe.navigation.obstacles.map((obstacle): NavigationObstacle => {
        if (!obstacle.polygon) return obstacle;
        return { ...obstacle, polygon: obstacle.polygon.map(point => {
          if (point.length !== 2 || !point.every(Number.isFinite)) throw new Error("Invalid connected city obstacle.");
          return [point[0], point[1]] as const;
        }) };
      }),
      streetGraph: { points: recipe.navigation.streetGraph.points.map(scenePoint),
        edges: recipe.navigation.streetGraph.edges.map(edge => {
          if (edge.length !== 2 || !edge.every(Number.isSafeInteger)) throw new Error("Invalid connected city edge.");
          return [edge[0], edge[1]] as const;
        }) } };
    if (recipe.transit.version !== "lagos-city-transit-v1") throw new Error("Invalid connected transit version.");
    const transit: TransitPlan = { ...recipe.transit, version: recipe.transit.version,
      stops: recipe.transit.stops.map(stop => {
        if (stop.direction !== 1 && stop.direction !== -1) throw new Error("Invalid connected transit direction.");
        return { ...stop, direction: stop.direction, approach: scenePoint(stop.approach) };
      }),
      routes: recipe.transit.routes.map(route => ({ ...route, stops: route.stops.map(stop =>
        "approach" in stop ? { ...stop, approach: scenePoint(stop.approach) } : stop) })),
      vehicles: recipe.transit.vehicles.map(vehicle => {
        if (vehicle.kind !== "brt" && vehicle.kind !== "danfo" && vehicle.kind !== "keke") throw new Error("Invalid connected transit vehicle.");
        return { ...vehicle, kind: vehicle.kind };
      }) };
    transitPlans.set(revision,transit);navigations.set(revision,navigation);return navigation;
  }
  if (revision === COMPANION_CITY_CONNECTED_REVISION) {
    const parks = companionCityNavigation(COMPANION_CITY_PARK_REVISION);
    const left = companionCityTheatre.bounds.minX, right = plan.bounds.minX;
    const shore = theatrePolygonObstacle([[left, companionCityBankZ(plan.lagoon.southBank, left)],
      [right, companionCityBankZ(plan.lagoon.southBank, right)], [right, companionCityBankZ(plan.lagoon.northBank, right)],
      [left, companionCityBankZ(plan.lagoon.northBank, left)]]);
    const theatreNav = { ...parks, revision, bounds: { ...companionCityTheatre.bounds },
      obstacles: [...parks.obstacles, shore, ...companionCityTheatreObstacles] };
    const base = { ...theatreNav, streetGraph: companionCityTheatreGraph(parks, theatreNav) };
    // Cover the accepted theatre foundation overhang without rewriting older frozen worlds.
    const theatreReserve = { minX: companionCityTheatre.center[0] - 106.2, maxX: companionCityTheatre.center[0] + 106.2,
      minZ: companionCityTheatre.center[1] - 95.2, maxZ: companionCityTheatre.center[1] + 95.2 };
    const nav = { ...base, revision, bounds: { ...companionCityRefinery.bounds },
      obstacles: [...base.obstacles, theatreReserve, companionCityRefineryObstacle] };
    const expanded = { ...nav, streetGraph: companionCityRefineryGraph(base, nav) };
    const transit = createConnectedCityTransitPlan(expanded);
    const round = (value: number) => Math.round(value * 1000) / 1000;
    const compact = (o: NavigationObstacle): NavigationObstacle => ({
      minX: round(o.minX), maxX: round(o.maxX), minZ: round(o.minZ), maxZ: round(o.maxZ),
      ...(o.yaw ? { yaw: Math.round(o.yaw * 1e6) / 1e6 } : {}),
      ...(o.polygon ? { polygon: o.polygon.map(p => [round(p[0]), round(p[1])] as const) } : {}),
    });
    const navigation = { ...expanded, obstacles: [...expanded.obstacles, ...transit.stops.flatMap(stop => stop.obstacles)].map(compact),
      streetGraph: { points: expanded.streetGraph.points.map(p => [round(p[0]), p[1], round(p[2])] as const),
        edges: expanded.streetGraph.edges } };
    // Shelters are now part of collision. Retain only services clear of the final, rounded geometry.
    const routes=transit.routes.filter(route=>{
      const kind=transit.vehicles.find(vehicle=>vehicle.routeId===route.id)!.kind;
      const swept={...navigation,radius:transitRules.kinds[kind].width/2+.25};
      return route.points.slice(1).every((point,index)=>routeSegmentClear(swept,
        [route.points[index].x,navigation.floorY,route.points[index].z],[point.x,navigation.floorY,point.z]));
    });
    const routeIds=new Set(routes.map(route=>route.id));
    transitPlans.set(revision,{...transit,routes,vehicles:transit.vehicles.filter(vehicle=>routeIds.has(vehicle.routeId))});
    navigations.set(revision, navigation); return navigation;
  }
  if (revision === COMPANION_CITY_THEATRE_REVISION) {
    const base = companionCityNavigation(COMPANION_CITY_PARK_REVISION);
    const left = companionCityTheatre.bounds.minX, right = plan.bounds.minX;
    const shore = theatrePolygonObstacle([[left, companionCityBankZ(plan.lagoon.southBank, left)],
      [right, companionCityBankZ(plan.lagoon.southBank, right)], [right, companionCityBankZ(plan.lagoon.northBank, right)],
      [left, companionCityBankZ(plan.lagoon.northBank, left)]]);
    const nav = { ...base, revision, bounds: { ...companionCityTheatre.bounds },
      obstacles: [...base.obstacles, shore, ...companionCityTheatreObstacles] };
    const navigation = { ...nav, streetGraph: companionCityTheatreGraph(base, nav) };
    navigations.set(revision, navigation); return navigation;
  }
  if (revision === COMPANION_CITY_PARK_REVISION) {
    const base = companionCityNavigation(COMPANION_CITY_ROAD_REVISION);
    const nav = { ...base, revision, obstacles: companionCityParks().obstacles };
    const navigation = { ...nav, streetGraph: companionCityParkGraph(base, nav) };
    navigations.set(revision, navigation);
    return navigation;
  }
  if (revision === COMPANION_CITY_ROAD_REVISION) {
    const base = companionCityNavigation(COMPANION_CITY_DETAIL_REVISION), treatments = companionCityRoadTreatments();
    const nav = { ...base, revision, obstacles: [...base.obstacles, ...treatments.islands.map(i => i.footprint),
      ...treatments.props.filter(p => !p.id.startsWith("median:")).map(p => p.footprint)] };
    const navigation = { ...nav, streetGraph: companionCityRoadGraph(base, nav, treatments) };
    navigations.set(revision, navigation);
    return navigation;
  }
  const obstacles: NavigationObstacle[] = [
    ...townSquareNavigation.obstacles.map(({ minX, maxX, minZ, maxZ }) => ({ minX, maxX, minZ, maxZ })),
    ...plan.parcels.map(companionCityParcelObstacle),
    ...plan.landmarkFootprints,
    ...plan.amenities.map(companionCityLandscapeReservedFootprint),
  ];
  // Exact shoreline strips leave the beaches dry and both bridge pavements open.
  obstacles.push(...waterObstacles());
  if (revision === COMPANION_CITY_DETAIL_REVISION) obstacles.push(...cityDetailObstacles());
  const nav: RoomNavigation = { revision, bounds: plan.bounds, obstacles, radius: .3, floorY: 0, gridSize: .8 };
  const points: ScenePoint[] = plan.streetGraph.points.map(([x, z]) => [x, 0, z]);
  const edges: [number, number][] = plan.streetGraph.edges
    .filter(([a, b]) => points[a] && points[b] && routeSegmentClear(nav, points[a], points[b]))
    .map(([a, b]) => [a, b]);
  const navigation = { ...nav, streetGraph: { points, edges } };
  navigations.set(revision, navigation);
  return navigation;
}

/** Camera volumes use the same visible reserves. Water remains a floor restriction, not a camera wall. */
export function companionCityCameraObstacles(navigation: RoomNavigation) {
  const volume = (id: string, obstacle: NavigationObstacle, height: number) => {
    const bounds = navigationObstacleBounds(obstacle);
    return { id, x: (bounds.minX + bounds.maxX) / 2, z: (bounds.minZ + bounds.maxZ) / 2,
      width: bounds.maxX - bounds.minX, depth: bounds.maxZ - bounds.minZ, height };
  };
  const values = plan.parcels.map(p => volume(p.id, companionCityParcelObstacle(p), p.floors * 3.3 + (p.kind === "tower" ? 6.6 : 3)));
  const amenities = companionCityHasParkPaths(navigation) ? companionCityParks().amenities : plan.amenities;
  values.push(...amenities.map(a => volume(a.id, companionCityLandscapeReservedFootprint(a),
    a.kind === "tree" ? 5 + a.variant * .7 : a.kind === "palm" ? 10 : a.kind === "market-canopy" ? 4.55
      : a.kind === "clock-tower" ? 9.5 : a.kind === "beach-umbrella" ? 3.2 : a.kind === "fountain" ? 1.5 : 1.1)));
  values.push(...plan.landmarkPieces.filter(p => p.y - p.h / 2 < 2).map((p, i) => ({
    ...volume(`city-landmark-${i}`, { minX: p.x - p.w / 2, maxX: p.x + p.w / 2,
      minZ: p.z - p.d / 2, maxZ: p.z + p.d / 2, yaw: p.yaw }, p.h), y: Math.max(0, p.y - p.h / 2),
  })));
  if (companionCityHasDetailCollision(navigation)) {
    const street = createCompanionCityStreetDetailsPlan();
    for (const entry of street.streets) for (const f of entry.fixtures) if (f.footprint) values.push(volume(f.id, f.footprint, f.height));
    values.push(...companionCityLandscapeAdditionalFootprints().map(p => volume(p.id, p, 1.5)));
  }
  if (companionCityHasRoadTreatments(navigation)) {
    const treatments = companionCityRoadTreatments();
    values.push(...treatments.islands.map(i => volume(i.id, i.footprint, .16)));
    values.push(...treatments.props.map(p => volume(p.id,
      { minX: p.x - .48, maxX: p.x + .48, minZ: p.z - .48, maxZ: p.z + .48 }, p.kind === "palm" ? 8.2 : 7.6)));
  }
  if (companionCityHasParkPaths(navigation)) values.push(...companionCityParkRailObstacles().map((o,i)=>volume(`park-bridge-rail-${i}`,o,1.1)));
  if (companionCityHasTheatre(navigation)) {
    values.push(volume("national-theatre", companionCityHasTransit(navigation)
      ? { minX: companionCityTheatre.center[0]-106.2, maxX: companionCityTheatre.center[0]+106.2,
        minZ: companionCityTheatre.center[1]-95.2, maxZ: companionCityTheatre.center[1]+95.2 } : companionCityTheatreFootprint,
      companionCityHasTransit(navigation) ? 47 : 46));
    values.push(...companionCityTheatre.trees.map((tree, i) => volume(`theatre-tree-${i}`,
      { minX: tree.x - .5, maxX: tree.x + .5, minZ: tree.z - .5, maxZ: tree.z + .5 }, tree.height)));
  }
  if (companionCityHasRefinery(navigation)) values.push(volume("refinery", companionCityRefineryObstacle, companionCityRefinery.height));
  if (companionCityHasTransit(navigation)) values.push(...(companionCityTransitPlan(navigation)?.stops ?? [])
    .flatMap(stop => stop.obstacles.map((o,i)=>volume(`${stop.id}:${i}`,o,3.2))));
  if (companionCityHasStreetDensity(navigation)) {
    const density = companionCityStreetDensity();
    values.push(...density.parks.flatMap(park => [
      ...park.buses.map(bus => volume(bus.id, cityMiniParkObstacle(park, bus.x, bus.z, 2.6, 5.2, bus.yaw), 2.2)),
      volume(`${park.id}:waiting-area`, cityMiniParkObstacle(park, -10.1, 0, .9, 6.7), 3.4),
      volume(`${park.id}:bin`, cityMiniParkObstacle(park, 10.2, -.5, .7, .7), .9),
    ]));
    values.push(...density.lights.map(light => volume(light.id, light.footprint!, light.height + .3)));
  }
  return values;
}

export function companionCityParcel(id: string) { return plan.parcels.find(parcel => parcel.id === id); }

/** Virtual placement only; never reads or publishes device location. */
export function companionCityGeoPoint(x: number, z: number) {
  return { latitude: plan.origin.latitude - z / 111_320,
    longitude: plan.origin.longitude + x / (111_320 * Math.cos(plan.origin.latitude * Math.PI / 180)) };
}
