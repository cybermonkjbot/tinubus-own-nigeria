import plan from "../../assets/city/lagos-city-v3.json";
import { navigationObstacleBounds, routeSegmentClear, type NavigationObstacle, type RoomNavigation, type ScenePoint } from "./companionNavigation";
import { cityDetailStreets, cityStreetLength, sampleCityStreet, cityStreetFixtureClear, createCompanionCityStreetDetailsPlan, type CityStreetFixture } from "./companionCityStreetDetails";

export const COMPANION_CITY_STREET_DENSITY_REVISION = `${plan.version}:street-density-v1`;
export type CityMiniPark = {
  id: string; name: string; roadId: string; x: number; z: number; yaw: number;
  width: number; depth: number; approach: ScenePoint;
  buses: { id: string; x: number; z: number; yaw: number; variant: number }[];
};
export type CityStreetDensityPlan = { parks: CityMiniPark[]; lights: CityStreetFixture[]; obstacles: NavigationObstacle[] };
const round = (value: number) => Math.round(value * 1000) / 1000;
export function cityMiniParkPoint(park: Pick<CityMiniPark, "x" | "z" | "yaw">, x: number, z: number): ScenePoint {
  const c = Math.cos(park.yaw), s = Math.sin(park.yaw);
  return [round(park.x + x * c + z * s), 0, round(park.z - x * s + z * c)];
}
function rectangle(x: number, z: number, width: number, depth: number, yaw = 0): NavigationObstacle {
  return { minX: round(x - width / 2), maxX: round(x + width / 2), minZ: round(z - depth / 2), maxZ: round(z + depth / 2), yaw };
}
export function cityMiniParkObstacle(park: CityMiniPark, x: number, z: number, width: number, depth: number, yaw = 0) {
  const at = cityMiniParkPoint(park, x, z);
  return rectangle(at[0], at[2], width, depth, park.yaw + yaw);
}
function overlaps(a: NavigationObstacle, b: NavigationObstacle, margin = 1) {
  a = navigationObstacleBounds(a); b = navigationObstacleBounds(b);
  return a.minX - margin < b.maxX && a.maxX + margin > b.minX && a.minZ - margin < b.maxZ && a.maxZ + margin > b.minZ;
}
function segmentDistance(x: number, z: number, a: readonly number[], b: readonly number[]) {
  const dx = b[0] - a[0], dz = b[1] - a[1], square = dx * dx + dz * dz;
  const t = square ? Math.max(0, Math.min(1, ((x - a[0]) * dx + (z - a[1]) * dz) / square)) : 0;
  return Math.hypot(x - a[0] - t * dx, z - a[1] - t * dz);
}

/** Additive open lots and light gaps. Existing plots, roads and frozen revisions are unchanged. */
export function createCityStreetDensityPlan(base: RoomNavigation, existingLights: readonly { x: number; z: number }[] = []): CityStreetDensityPlan {
  const parks: CityMiniPark[] = [], lights: CityStreetFixture[] = [], obstacles: NavigationObstacle[] = [];
  const reserves = [...base.obstacles];
  const protectedLand = [...plan.greenSpaces, ...plan.beaches].map(space => ({
    minX: Math.min(...space.points.map(p => p[0])), maxX: Math.max(...space.points.map(p => p[0])),
    minZ: Math.min(...space.points.map(p => p[1])), maxZ: Math.max(...space.points.map(p => p[1])),
  }));
  const requests = [
    { id: "market", name: "Market Mini park", roads: ["produce-road", "spice-lane", "market-road"] },
    { id: "unity", name: "Unity Mini park", roads: ["unity-avenue", "tailors-lane"] },
    { id: "depot", name: "Depot Mini park", roads: ["depot-road", "workshop-way", "loading-lane"] },
    { id: "marina", name: "Marina Mini park", roads: ["marina-drive", "harbour-road", "west-service-road"] },
    { id: "palm", name: "Palm Mini park", roads: ["palm-crescent", "market-road", "cross-town-road"] },
  ];
  for (const request of requests) {
    let chosen: CityMiniPark | undefined;
    for (const roadId of request.roads) {
      const road = cityDetailStreets.find(r => r.id === roadId);
      if (!road) continue;
      const length = cityStreetLength(road);
      for (let distance = 45; distance < length - 40 && !chosen; distance += 18) {
        const sample = sampleCityStreet(road, distance), normalLength = Math.hypot(sample.nx, sample.nz);
        const nx = sample.nx / normalLength, nz = sample.nz / normalLength;
        placement: for (const side of [-1, 1]) for (const setback of [0, 4, 8]) {
          const offset = side * (road.width / 2 + road.pavement + 11 + setback);
          const x = round(sample.x + nx * offset), z = round(sample.z + nz * offset);
          const yaw = Math.atan2(-nx * side, -nz * side);
          const reserve = rectangle(x, z, 24, 20, yaw), bounds = navigationObstacleBounds(reserve);
          if (reserves.some(o => overlaps(reserve, o, 1.5)) || protectedLand.some(o => overlaps(reserve, o))) continue;
          if (plan.roads.some(other => other.points.slice(1).some((point, index) =>
            segmentDistance(x, z, other.points[index], point) < Math.hypot(12, 10) + other.width / 2 + other.pavement + 1))) continue;
          if (bounds.minX < base.bounds.minX + 2 || bounds.maxX > base.bounds.maxX - 2
            || bounds.minZ < base.bounds.minZ + 2 || bounds.maxZ > base.bounds.maxZ - 2) continue;
          // The whole lot and its link must be clear of the admitted pedestrian graph.
          const lotNav = { ...base, obstacles: [reserve] };
          if (base.streetGraph?.edges.some(([a, b]) => !routeSegmentClear(lotNav, base.streetGraph!.points[a], base.streetGraph!.points[b]))) continue;
          const approach: ScenePoint = [round(sample.x + nx * side * (road.width / 2 + road.pavement * .55)), 0,
            round(sample.z + nz * side * (road.width / 2 + road.pavement * .55))];
          const candidate: CityMiniPark = { id: `mini-park-${request.id}`, name: request.name, roadId,
            x, z, yaw, width: 22, depth: 18, approach, buses: [] };
          if (!routeSegmentClear(base, approach, cityMiniParkPoint(candidate, 0, 7.8))) continue;
          candidate.buses = [-5, 3.1].flatMap((row, r) => [-7.2, 0, 7.2].map((column, c) => ({
            id: `${candidate.id}:danfo:${r * 3 + c}`, x: column, z: row,
            yaw: (c - 1) * .025, variant: r * 3 + c,
          })));
          chosen = candidate; break placement;
        }
      }
      if (chosen) break;
    }
    if (!chosen) continue;
    parks.push(chosen); reserves.push(rectangle(chosen.x, chosen.z, 24, 20, chosen.yaw));
    obstacles.push(...chosen.buses.map(bus => cityMiniParkObstacle(chosen, bus.x, bus.z, 2.6, 5.2, bus.yaw)),
      cityMiniParkObstacle(chosen, -10.1, 0, .9, 6.7), cityMiniParkObstacle(chosen, 10.2, -.5, .7, .7));
    for (const x of [-10, 10]) {
      const at = cityMiniParkPoint(chosen, x, -8);
      const footprint = rectangle(at[0], at[2], .6, .6);
      lights.push({ id: `${chosen.id}:light:${x}`, roadId: chosen.roadId, distance: 0,
        x: at[0], z: at[2], yaw: chosen.yaw + Math.PI / 2, kind: "lamp", style: "twin-arm", height: 6.4, variant: 0, side: 1, footprint });
      obstacles.push(footprint);
    }
  }
  const oldLights = [...existingLights,
    ...createCompanionCityStreetDetailsPlan().streets.flatMap(street => street.fixtures.filter(f => f.kind !== "wall-sign"))];
  for (const road of cityDetailStreets) {
    const length = cityStreetLength(road);
    for (let distance = 30; distance < length - 18; distance += road.width >= 20 ? 38 : 30) {
      const sample = sampleCityStreet(road, distance);
      if ([...oldLights, ...lights].some(light => Math.hypot(light.x - sample.x, light.z - sample.z) < 34)) continue;
      for (const side of [-1, 1]) {
        const offset = side * (road.width / 2 + (road.kind === "bridge" ? 2.65 : road.pavement + .8));
        const x = round(sample.x + sample.nx * offset), z = round(sample.z + sample.nz * offset);
        if (!cityStreetFixtureClear(road, sample, x, z) || reserves.some(o => overlaps(rectangle(x, z, .6, .6), o, .6))) continue;
        const at: ScenePoint = [x, 0, z], footprint = rectangle(x, z, .6, .6);
        if (!routeSegmentClear({ ...base, radius: .7 }, at, at)) continue;
        const lampNav = { ...base, obstacles: [footprint] };
        if (base.streetGraph?.edges.some(([a, b]) => !routeSegmentClear(lampNav, base.streetGraph!.points[a], base.streetGraph!.points[b]))) continue;
        lights.push({ id: `density-light:${road.id}:${distance}`, roadId: road.id, distance, x, z, yaw: sample.yaw,
          kind: "lamp", style: road.width >= 20 ? "twin-arm" : "short-arm", height: 7.8, variant: 0, side, footprint });
        obstacles.push(footprint); reserves.push(footprint); break;
      }
    }
  }
  return { parks, lights, obstacles };
}

export function cityStreetDensityGraph(base: RoomNavigation, navigation: RoomNavigation, parks: readonly CityMiniPark[]) {
  const points: ScenePoint[] = [...base.streetGraph!.points];
  const edges: (readonly [number, number])[] = [...base.streetGraph!.edges];
  for (const park of parks) {
    const approach = points.length; points.push(park.approach);
    for (const index of base.streetGraph!.points.map((point, index) => ({ index, distance: Math.hypot(point[0] - park.approach[0], point[2] - park.approach[2]) }))
      .filter(p => p.distance < 100).sort((a, b) => a.distance - b.distance).slice(0, 8).map(p => p.index)) {
      if (routeSegmentClear(navigation, points[index], park.approach)) { edges.push([index, approach]); break; }
    }
    const entry = points.length; points.push(cityMiniParkPoint(park, -3.6, 7.8));
    if (routeSegmentClear(navigation, park.approach, points[entry])) edges.push([approach, entry]);
    const aisle = points.length; points.push(cityMiniParkPoint(park, -3.6, -.9));
    if (routeSegmentClear(navigation, points[entry], points[aisle])) edges.push([entry, aisle]);
  }
  return { points, edges };
}
