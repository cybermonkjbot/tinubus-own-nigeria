import plan from "../../assets/city/lagos-city-v3.json";
import { companionCityLandscapeReservedFootprint, companionCityLandscapeSeed, type CityLandscapeAmenity } from "./companionCityLandscapeDetails";
import { navigationObstacleBounds, routeSegmentClear, type NavigationObstacle, type RoomNavigation, type ScenePoint } from "./companionNavigation";

export const COMPANION_CITY_PARK_REVISION = `${plan.version}:parks-v1`;
type Point = [number, number];
export type CityParkPath = { id: string; name: string; kind: string; width: number; pavement: number; points: number[][] };
const path = (id: string, name: string, points: number[][], width = 3.6): CityParkPath => ({ id, name, kind: "path", width, pavement: 0, points });

// One lake with two broad basins and a narrow connecting reach, entirely inside the garden reserve.
// Horizontal bank sections also supply exact, small collision polygons around the bridge openings.
const banks = [
  { z: -615, left: 862, right: 878 }, { z: -604, left: 842, right: 895 },
  { z: -584, left: 834, right: 904 }, { z: -570, left: 850, right: 891 },
  { z: -560, left: 858, right: 882 }, { z: -550, left: 857, right: 883 },
  { z: -539, left: 842, right: 899 }, { z: -521, left: 838, right: 900 },
  { z: -506, left: 853, right: 887 }, { z: -498, left: 866, right: 874 },
];
export const companionCityParkLake = {
  id: "botanic-connected-lake", spaceId: "botanic-gardens", level: -.24,
  banks,
  points: [...banks.map(b => [b.left, b.z] as Point), ...[...banks].reverse().map(b => [b.right, b.z] as Point)],
};
export const companionCityParkBridges = [
  { id: "botanic-neck-bridge", x1: 850, x2: 891, z: -555, width: 3.6 },
  { id: "botanic-north-bridge", x1: 845, x2: 896, z: -506, width: 3.6 },
];
export const companionCityParkPaths = [
  path("botanic-crossing", "Botanic Gardens lake crossing", [[795.32,-555],[850,-555],[891,-555],[948,-555]]),
  path("botanic-north-crossing", "Botanic Gardens north crossing", [[797.56,-508.36],[822,-506],[845,-506],[896,-506],[920,-506],[940.15,-509.83]]),
  path("botanic-woodland-path", "Botanic Gardens woodland path", [[802.15,-598.7],[814,-646],[870,-660],[925,-648],[931.54,-594.62]], 3.2),
  path("west-grove-woodland-path", "West Grove woodland path", [[-1230.21,-990],[-1210,-1004],[-1190,-990],[-1170,-977],[-1152.23,-972.55]], 3.2),
  path("college-green-crossing", "College Green crossing", [[493.23,-339.41],[511,-349],[530,-360],[551,-372],[561.24,-377.49]], 3.2),
  path("southern-common-crossing", "Southern Common crossing", [[695.43,-1262.31],[718,-1272],[740,-1280],[766,-1290],[777.87,-1295.03]], 3.2),
];
export const companionCityParkRemovedRoads = new Set(["botanic-gardens-garden-path"]);
const parkIds = new Set(["botanic-gardens", "west-grove", "college-green", "southern-common"]);
export const companionCityParkWalkingRoads = [...plan.roads.filter(r => !companionCityParkRemovedRoads.has(r.id)), ...companionCityParkPaths];
const round = (n: number) => Math.round(n * 1000) / 1000;
const scenePoint = (p: readonly number[]): ScenePoint => [round(p[0]), 0, round(p[1])];
export function cityParkDistanceToPath(x: number, z: number, points: readonly (readonly number[])[]) {
  let nearest = Infinity;
  for (let i = 1; i < points.length; i++) {
    const a = points[i-1], b = points[i], dx = b[0]-a[0], dz = b[1]-a[1], length = dx*dx+dz*dz;
    const t = length ? Math.max(0, Math.min(1, ((x-a[0])*dx+(z-a[1])*dz)/length)) : 0;
    nearest = Math.min(nearest, Math.hypot(x-a[0]-dx*t, z-a[1]-dz*t));
  }
  return nearest;
}
function inside(x: number, z: number, polygon: readonly (readonly number[])[]) {
  let result = false;
  for (let i = 0, j = polygon.length-1; i < polygon.length; j = i++) {
    const a = polygon[i], b = polygon[j];
    if ((a[1]>z)!==(b[1]>z) && x<(b[0]-a[0])*(z-a[1])/(b[1]-a[1])+a[0]) result = !result;
  }
  return result;
}
function bankAt(z: number, side: "left" | "right") {
  for (let i = 1; i < banks.length; i++) if (z <= banks[i].z) {
    const a = banks[i-1], b = banks[i];
    return round(a[side]+(b[side]-a[side])*(z-a.z)/(b.z-a.z));
  }
  return banks[banks.length-1][side];
}
const obstacle = (polygon: Point[]): NavigationObstacle => ({ minX: Math.min(...polygon.map(p=>p[0])),
  maxX: Math.max(...polygon.map(p=>p[0])), minZ: Math.min(...polygon.map(p=>p[1])), maxZ: Math.max(...polygon.map(p=>p[1])), polygon });
export function companionCityParkWaterObstacles(): NavigationObstacle[] {
  const cuts = [...new Set([...banks.map(b=>b.z), ...companionCityParkBridges.flatMap(b=>[b.z-b.width/2,b.z+b.width/2])])].sort((a,b)=>a-b);
  const values: NavigationObstacle[] = [];
  for (let i = 1; i < cuts.length; i++) {
    const a = cuts[i-1], b = cuts[i];
    if (companionCityParkBridges.some(bridge => (a+b)/2>bridge.z-bridge.width/2 && (a+b)/2<bridge.z+bridge.width/2)) continue;
    values.push(obstacle([[bankAt(a,"left"),a],[bankAt(b,"left"),b],[bankAt(b,"right"),b],[bankAt(a,"right"),a]]));
  }
  return values;
}
export function companionCityParkRailObstacles(): NavigationObstacle[] {
  return companionCityParkBridges.flatMap(b => [-1,1].map(side => ({
    minX: b.x1, maxX: b.x2, minZ: b.z+side*(b.width/2-.06)-.06, maxZ: b.z+side*(b.width/2-.06)+.06,
  })));
}
export type CityParkRoute = { id: string; points: ScenePoint[]; lengths: number[]; length: number };
export type CityParkPlan = { amenities: CityLandscapeAmenity[]; removedAmenityIndices: Set<number>;
  trees: CityLandscapeAmenity[]; obstacles: NavigationObstacle[]; routes: CityParkRoute[] };

/** Placement, walk clearance and ambient routes are derived once from the same frozen road revision. */
export function createCompanionCityParkPlan(base: RoomNavigation): CityParkPlan {
  const removedAmenityIndices = new Set<number>();
  const walking = companionCityParkWalkingRoads.filter(r=>["path","boardwalk","sand-path"].includes(r.kind));
  const lakeEdge = [...companionCityParkLake.points, companionCityParkLake.points[0]];
  plan.amenities.forEach((a,i) => {
    if (!plan.greenSpaces.some(s=>parkIds.has(s.id)&&a.id.startsWith(s.id+"-"))) return;
    const footprint = navigationObstacleBounds(companionCityLandscapeReservedFootprint(a));
    const reserve = Math.hypot(footprint.maxX-footprint.minX, footprint.maxZ-footprint.minZ)/2;
    if (inside(a.x,a.z,companionCityParkLake.points) || cityParkDistanceToPath(a.x,a.z,lakeEdge)<reserve+1.2
      || walking.some(r=>cityParkDistanceToPath(a.x,a.z,r.points)<r.width/2+reserve+1.2)) removedAmenityIndices.add(i);
  });
  // Amenity reserves are an authored contiguous section, before lagoon/detail/road additions.
  // Resolve it by matching the first reserve rather than relying on the number of later additions.
  const first = companionCityLandscapeReservedFootprint(plan.amenities[0]);
  const firstIndex = base.obstacles.findIndex(o=>o.minX===first.minX&&o.maxX===first.maxX&&o.minZ===first.minZ&&o.maxZ===first.maxZ&&o.yaw===first.yaw);
  if (firstIndex < 0 || !plan.amenities.every((a,i)=>{
    const expected=companionCityLandscapeReservedFootprint(a),actual=base.obstacles[firstIndex+i];
    return actual&&actual.minX===expected.minX&&actual.maxX===expected.maxX&&actual.minZ===expected.minZ&&actual.maxZ===expected.maxZ&&actual.yaw===expected.yaw;
  })) throw new Error("City park amenity reserves are unavailable.");
  const obstacles = base.obstacles.filter((_,i)=>!removedAmenityIndices.has(i-firstIndex));
  obstacles.push(...companionCityParkWaterObstacles(), ...companionCityParkRailObstacles());
  const nav = { ...base, obstacles };
  const trees: CityLandscapeAmenity[] = [];
  for (const [id, spacing, cap] of [["west-grove",8,64],["college-green",12,16]] as const) {
    const space = plan.greenSpaces.find(s=>s.id===id)!;
    const xs=space.points.map(p=>p[0]), zs=space.points.map(p=>p[1]);
    for (let z=Math.min(...zs)+5;z<Math.max(...zs)-5&&trees.filter(t=>t.id.startsWith(id+"-")).length<cap;z+=spacing)
      for (let x=Math.min(...xs)+5;x<Math.max(...xs)-5&&trees.filter(t=>t.id.startsWith(id+"-")).length<cap;x+=spacing) {
        const seed = companionCityLandscapeSeed(`${id}:${x}:${z}`), px=round(x+(seed%100)/100*3), pz=round(z+((seed>>>8)%100)/100*3);
        if (!inside(px,pz,space.points) || cityParkDistanceToPath(px,pz,[...space.points,space.points[0]])<4
          || walking.some(r=>cityParkDistanceToPath(px,pz,r.points)<r.width/2+2.5)) continue;
        const p: ScenePoint = [px,0,pz];
        if (!routeSegmentClear({...nav,radius:1.6},p,p)) continue;
        const footprint = companionCityLandscapeReservedFootprint({id:"",kind:"tree",x:px,z:pz,yaw:0,variant:0});
        // Added trunks also preserve every old graph edge that survives the lake restriction.
        if (base.streetGraph?.edges.some(([a,b])=>!routeSegmentClear({...nav,obstacles:[footprint]},base.streetGraph!.points[a],base.streetGraph!.points[b]))) continue;
        const tree = {id:`${id}-park-tree-${trees.length}`,kind:"tree",x:px,z:pz,yaw:0,variant:seed%4};
        trees.push(tree); obstacles.push(footprint);
      }
  }
  const amenities = [...plan.amenities.filter((_,i)=>!removedAmenityIndices.has(i)), ...trees];
  const routes: CityParkRoute[] = [];
  const routeRoads = [...plan.roads.filter(r=>parkIds.has(r.id.replace(/-loop$/,""))&&r.id.endsWith("-loop")),
    ...companionCityParkPaths];
  for (const road of routeRoads) {
    const closed = road.points[0][0]===road.points.at(-1)![0]&&road.points[0][1]===road.points.at(-1)![1];
    const points = (closed ? road.points : [...road.points,...road.points.slice(0,-1).reverse()]).map(scenePoint);
    if (!points.slice(1).every((p,i)=>routeSegmentClear(nav,points[i],p))) continue;
    const lengths = points.slice(1).map((p,i)=>Math.hypot(p[0]-points[i][0],p[2]-points[i][2]));
    routes.push({id:road.id,points,lengths,length:lengths.reduce((sum,n)=>sum+n,0)});
  }
  return { amenities, removedAmenityIndices, trees, obstacles, routes };
}

/** Add sampled park routes to the existing city graph; older revisions retain their exact topology. */
export function companionCityParkGraph(base: RoomNavigation, nav: RoomNavigation) {
  const points: ScenePoint[] = [...base.streetGraph!.points];
  const edges: [number,number][] = base.streetGraph!.edges.filter(([a,b])=>routeSegmentClear(nav,points[a],points[b])).map(([a,b])=>[a,b]);
  const index = (p: ScenePoint) => {
    const existing = points.findIndex(q=>Math.hypot(p[0]-q[0],p[2]-q[2])<.02);
    if (existing>=0) return existing;
    points.push(p); return points.length-1;
  };
  const walking = companionCityParkWalkingRoads.filter(r=>["path","boardwalk","sand-path"].includes(r.kind)
    && (companionCityParkPaths.some(p=>p.id===r.id) || [...parkIds].some(id=>r.id.startsWith(id+"-"))));
  for (const road of walking) {
    let previous = index(scenePoint(road.points[0]));
    for (let i=1;i<road.points.length;i++) {
      const a=road.points[i-1],b=road.points[i],count=Math.ceil(Math.hypot(b[0]-a[0],b[1]-a[1])/18);
      for (let j=1;j<=count;j++) {
        const p=scenePoint([a[0]+(b[0]-a[0])*j/count,a[1]+(b[1]-a[1])*j/count]), next=index(p);
        if (!routeSegmentClear(nav,points[previous],p)) throw new Error(`The ${road.name} is obstructed.`);
        if (previous!==next) edges.push([previous,next]); previous=next;
      }
    }
    for (const endpoint of [scenePoint(road.points[0]),scenePoint(road.points.at(-1)!)]) {
      const at=index(endpoint);
      const nearby=base.streetGraph!.points.map((p,i)=>({p,i,d:Math.hypot(p[0]-endpoint[0],p[2]-endpoint[2])}))
        .filter(p=>p.i!==at&&p.d<30).sort((a,b)=>a.d-b.d);
      const join=nearby.find(p=>routeSegmentClear(nav,endpoint,p.p));
      if (join) edges.push([at,join.i]);
    }
  }
  if (points.length>2048||edges.length>8192) throw new Error("Park paths exceed the bounded city graph.");
  return {points,edges};
}
