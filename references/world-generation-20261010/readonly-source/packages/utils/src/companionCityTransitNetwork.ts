import plan from "../../assets/city/lagos-city-v3.json";
import { companionCityTheatre } from "./companionCityTheatre";
import { companionCityRefinery } from "./companionCityRefinery";
import { createTransitPlan, createTransitRoadRoute, transitRules, type TransitPlan, type TransitRoad } from "./companionCityTransit";
import { routeSegmentClear, type RoomNavigation } from "./companionNavigation";

export const COMPANION_CITY_CONNECTED_REVISION = `${plan.version}:connected-transit-v1`;

const districtRoads: readonly TransitRoad[] = [...plan.roads.filter(r => r.width >= 8
  && !["path", "boardwalk", "sand-path", "retained"].includes(r.kind)),
  ...companionCityTheatre.roads, ...companionCityRefinery.roads];

type Leg = { road: string; from?: readonly number[]; to?: readonly number[]; reverse?: boolean };
const nearby = (a: readonly number[], b: readonly number[]) => Math.hypot(a[0] - b[0], a[1] - b[1]) < .04;
function corridor(id: string, name: string, legs: readonly Leg[]): TransitRoad {
  const points: number[][] = [];
  const segmentWidths:number[]=[];
  let width = Infinity;
  for (const leg of legs) {
    const road = districtRoads.find(road => road.id === leg.road);
    if (!road) throw new Error("A BRT corridor road is missing.");
    const vertices = leg.reverse ? [...road.points].reverse() : [...road.points];
    const first = leg.from ? vertices.findIndex(p => nearby(p, leg.from!)) : 0;
    const last = leg.to ? vertices.findIndex(p => nearby(p, leg.to!)) : vertices.length - 1;
    if (first < 0 || last < first) throw new Error("BRT corridors must use existing road junctions.");
    const section = vertices.slice(first, last + 1);
    if (points.length && !nearby(points.at(-1)!, section[0])) throw new Error("A BRT corridor is disconnected.");
    for (const p of section) if (!points.length || !nearby(points.at(-1)!, p)) {
      if(points.length)segmentWidths.push(road.width);
      points.push([p[0], p[1]]);
    }
    width = Math.min(width, road.width);
  }
  return { id, name, kind: "avenue", width, pavement: 3, points,segmentWidths };
}

/** Same road coordinates as the city, including bridge crossings. No straight-line travel over water or parcels. */
const corridors = [
  corridor("brt-palm-theatre", "Palm Courts · Marina · Theatre Gardens", [
    { road: "market-road", reverse: true, to: [-480, -260] },
    { road: "west-bridge-approach" }, { road: "west-lagoon-bridge" }, { road: "marina-drive" },
    { road: "theatre-marina-approach" }, {road:"theatre-ring-drive"},
  ]),
  corridor("brt-garden-industrial", "Garden Quarter · Lagoon Gardens · Industrial Harbour", [
    { road: "garden-avenue", reverse: true, to: [390, -50] },
    { road: "east-bridge-approach" }, { road: "east-lagoon-bridge" }, { road: "lagoon-gardens-drive" },
    { road: "industrial-harbour-approach" },{road:"refinery-ring-road"},
  ]),
  corridor("brt-south-theatre", "South Market · Town Centre · Marina · Theatre Gardens", [
    {road:"unity-avenue",reverse:true,to:[-90,-810]},
    {road:"cross-town-road",reverse:true,from:[-90,-810]},
    {road:"market-road",reverse:true,from:[-900,-680],to:[-480,-260]},
    {road:"west-bridge-approach"},{road:"west-lagoon-bridge"},{road:"marina-drive"},
    {road:"theatre-marina-approach"},{road:"theatre-ring-drive"},
  ]),
  corridor("brt-skyline-industrial", "Skyline · Lagoon Gardens · Industrial Harbour", [
    {road:"financial-avenue",reverse:true},{road:"lagoon-gardens-drive"},{road:"industrial-harbour-approach"},{road:"refinery-ring-road"},
  ]),
  corridor("brt-west-garden", "Palm Courts · Town Centre · Garden Quarter", [
    {road:"market-road",reverse:true,to:[-900,-680]},
    {road:"cross-town-road",to:[920,-900]},
    {road:"botanic-road",reverse:true},
    {road:"garden-avenue",from:[610,-170]},
  ]),
];

export const companionCityTransitRoads = districtRoads;

/** Plans are derived once per collision revision and reused by authority, navigation, renderer and map. */
export function createConnectedCityTransitPlan(navigation: RoomNavigation): TransitPlan {
  const local = createTransitPlan(navigation, districtRoads);
  const routes = [...local.routes], vehicles = [...local.vehicles];
  for (const road of corridors) {
    // Reuse physical local shelters instead of creating overlapping corridor-only shelters.
    const stops = local.stops.flatMap(stop => {
      let prefix = 0, nearest = { distance: Infinity, metres: 0, direction: 1 as 1 | -1,width:road.width };
      for (let i = 1; i < road.points.length; i++) {
        const a = road.points[i-1], b = road.points[i], dx=b[0]-a[0], dz=b[1]-a[1], length=Math.hypot(dx,dz);
        const t = Math.max(0,Math.min(1,((stop.berth.x-a[0])*dx+(stop.berth.z-a[1])*dz)/Math.max(.001,length*length)));
        const d=Math.hypot(a[0]+dx*t-stop.berth.x,a[1]+dz*t-stop.berth.z);
        const yaw=Math.atan2(dx,dz), heading=Math.cos(yaw-stop.berth.yaw);
        if(d<nearest.distance)nearest={distance:d,metres:prefix+t*length,direction:heading>=0?1:-1,width:road.segmentWidths?.[i-1]??road.width};
        prefix+=length;
      }
      return nearest.distance < nearest.width / 2 + .5
        ? [{ ...stop, roadMetres: nearest.metres, direction: nearest.direction }] : [];
    });
    const route = createTransitRoadRoute(road,"brt",stops,navigation);
    if (!route || route.stops.length < 2 || route.points.slice(1).some((point,index)=>!routeSegmentClear(
      {...navigation,radius:transitRules.kinds.brt.width/2+.25},[route.points[index].x,navigation.floorY,route.points[index].z],
      [point.x,navigation.floorY,point.z]))) continue;
    if(routes.length>=transitRules.maxRoutes||vehicles.length+3>transitRules.maxVehicles)break;
    routes.push(route);
    const duration = route.metres/transitRules.kinds.brt.speed*1000+route.stops.length*transitRules.dwellMs;
    for(let index=0;index<3;index++)vehicles.push({id:`${route.id}:${index}`,kind:"brt",routeId:route.id,offsetMs:Math.round(duration*index/3)});
  }
  return { ...local, routes, vehicles };
}
