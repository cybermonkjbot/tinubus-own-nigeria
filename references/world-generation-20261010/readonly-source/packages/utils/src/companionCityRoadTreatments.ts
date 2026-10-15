import plan from "../../assets/city/lagos-city-v3.json";
import { createCityStreetCrossingLookup, cityStreetLength, sampleCityStreet } from "./companionCityStreetDetails";
import { navigationObstacleBounds, routeSegmentClear, type NavigationObstacle, type RoomNavigation, type ScenePoint } from "./companionNavigation";

export const COMPANION_CITY_ROAD_REVISION = `${plan.version}:roads-v1`;
export const companionCityDividedRoads = new Set(["garden-avenue", "financial-avenue", "civic-boulevard"]);
export const companionCityPalmStreets = new Set([...companionCityDividedRoads, "palm-crescent", "marina-drive"]);
const earthRoads = new Set(["produce-road", "spice-lane", "tailors-lane"]);
const gravelRoads = new Set(["west-service-road", "workshop-way", "depot-road", "loading-lane"]);
export function companionCityRoadSurface(id: string): "asphalt" | "earth" | "gravel" {
  return earthRoads.has(id) ? "earth" : gravelRoads.has(id) ? "gravel" : "asphalt";
}
export type CityRoadProp = { id: string; roadId: string; kind: "palm" | "light"; x: number; z: number; yaw: number; variant: number; footprint: NavigationObstacle };
export type CityRoadIsland = { id: string; roadId: string; x: number; z: number; yaw: number; length: number; width: number; footprint: NavigationObstacle };
export type CityRoadTreatments = { islands: CityRoadIsland[]; props: CityRoadProp[] };
const round = (n: number) => Math.round(n * 1000) / 1000;
const box = (x: number, z: number, length: number, width: number, yaw = 0): NavigationObstacle => ({
  minX: round(x - length / 2), maxX: round(x + length / 2), minZ: round(z - width / 2), maxZ: round(z + width / 2), yaw: round(yaw),
});
const point = (x: number, z: number): ScenePoint => [round(x), 0, round(z)];
function overlaps(a: NavigationObstacle, b: NavigationObstacle, margin = .8) {
  a = navigationObstacleBounds(a); b = navigationObstacleBounds(b);
  return a.minX - margin < b.maxX && a.maxX + margin > b.minX && a.minZ - margin < b.maxZ && a.maxZ + margin > b.minZ;
}

/** New frozen worlds only. Original plots, authored bends and old fixture placement remain unchanged. */
export function createCompanionCityRoadTreatments(base: RoomNavigation): CityRoadTreatments {
  const islands: CityRoadIsland[] = [], props: CityRoadProp[] = [], crossing = createCityStreetCrossingLookup();
  const admitted = new Set(base.streetGraph?.edges.flatMap(edge => [...edge]));
  const routePoints = base.streetGraph?.points.filter((_, index) => admitted.has(index)) ?? [];
  const clear = (x: number, z: number, radius: number) => routeSegmentClear({ ...base, radius }, point(x, z), point(x, z));
  const add = (roadId: string, kind: CityRoadProp["kind"], x: number, z: number, yaw: number, variant: number) => {
    const radius = kind === "palm" ? .48 : .24, footprint = box(x, z, radius * 2, radius * 2);
    if (!clear(x, z, radius + .4) || props.some(p => overlaps(p.footprint, footprint, 2))) return;
    // Sidewalk fixtures cannot obstruct any previously admitted walking edge.
    const test = { ...base, obstacles: [footprint] };
    if (base.streetGraph?.edges.some(([a, b]) => !routeSegmentClear(test, base.streetGraph!.points[a], base.streetGraph!.points[b]))) return;
    props.push({ id: `road:${roadId}:${kind}:${props.length}`, roadId, kind, x: round(x), z: round(z), yaw: round(yaw), variant, footprint });
  };
  for (const road of plan.roads) {
    if (!companionCityPalmStreets.has(road.id)) continue;
    if (companionCityDividedRoads.has(road.id)) {
      let travelled = 0;
      for (let i = 1; i < road.points.length; i++) {
        const a = road.points[i - 1], b = road.points[i], span = Math.hypot(b[0] - a[0], b[1] - a[1]);
        const segmentStart = travelled;
        travelled += span;
        if (span < 36) continue;
        const candidates = Array.from({ length: Math.floor((span - 28) / 4) + 1 }, (_, k) => 14 + k * 4)
          .sort((a, b) => Math.abs(a - span / 2) - Math.abs(b - span / 2));
        for (const offset of candidates) {
          const distance = segmentStart + offset, sample = sampleCityStreet(road, distance);
          if (crossing(road, distance - 9, 0) || crossing(road, distance + 9, 0) || crossing(road, distance, 0)) continue;
          const length = 12, width = 2.6, footprint = box(sample.x, sample.z, length, width, sample.yaw);
          const test = { ...base, obstacles: [footprint] };
          if (routePoints.some(p => !routeSegmentClear(test, p, p))) continue;
          const c = Math.cos(sample.yaw), s = Math.sin(sample.yaw);
          if (!routeSegmentClear({ ...base, radius: 1.65 }, point(sample.x - 6 * c, sample.z + 6 * s),
            point(sample.x + 6 * c, sample.z - 6 * s))) continue;
          if (![-1, 1].every(side => [-1, 1].every(end => clear(sample.x + end * 7.2 * c + side * 2.6 * s,
            sample.z - end * 7.2 * s + side * 2.6 * c, .4)))) continue;
          islands.push({ id: `median:${road.id}:${i}`, roadId: road.id, x: round(sample.x), z: round(sample.z),
            yaw: round(sample.yaw), length, width, footprint });
          for (const end of [-1, 1]) props.push({ id: `median:${road.id}:${i}:palm:${end}`, roadId: road.id, kind: "palm",
            x: round(sample.x + end * 3.2 * c), z: round(sample.z - end * 3.2 * s), yaw: round(sample.yaw + end * .36), variant: i % 2, footprint });
          break;
        }
      }
    }
    const length = cityStreetLength(road);
    for (let distance = 32, index = 0; distance < length - 24; distance += 42, index++) {
      const sample = sampleCityStreet(road, distance);
      for (const side of [-1, 1]) {
        if (crossing(road, distance, side)) continue;
        const kind = index % 2 ? "palm" : "light";
        const offset = side * (road.width / 2 + road.pavement - .8);
        add(road.id, kind, sample.x + sample.nx * offset, sample.z + sample.nz * offset,
          sample.yaw + (side > 0 ? Math.PI : 0), index % 2);
      }
    }
  }
  return { islands, props };
}

/** Preserve every admitted old edge, replacing median-crossing edges with short, swept-clear detours. */
export function companionCityRoadGraph(base: RoomNavigation, nav: RoomNavigation, treatments: CityRoadTreatments) {
  const graph = base.streetGraph!;
  const points: ScenePoint[] = [...graph.points], edges: [number, number][] = [];
  for (const [a, b] of graph.edges) {
    if (routeSegmentClear(nav, points[a], points[b])) { edges.push([a, b]); continue; }
    const start = points[a], end = points[b], dx = end[0] - start[0], dz = end[2] - start[2];
    const hits = treatments.islands.filter(island => !routeSegmentClear({ ...base, obstacles: [island.footprint] }, start, end))
      .sort((p, q) => (p.x - q.x) * dx + (p.z - q.z) * dz);
    let route: ScenePoint[] | undefined;
    for (const side of [-1, 1]) {
      const candidate: ScenePoint[] = [];
      for (const island of hits) {
        const c = Math.cos(island.yaw), s = Math.sin(island.yaw);
        const forward = dx * c - dz * s >= 0 ? 1 : -1;
        for (const along of [-forward, forward]) candidate.push(point(island.x + along * (island.length / 2 + 1.2) * c + side * 2.5 * s,
          island.z - along * (island.length / 2 + 1.2) * s + side * 2.5 * c));
      }
      const sequence = [start, ...candidate, end];
      if (hits.length && sequence.slice(1).every((p, i) => routeSegmentClear(nav, sequence[i], p))) { route = candidate; break; }
    }
    if (!route) throw new Error("Road planting would disconnect an admitted city route.");
    let previous = a;
    for (const p of route) { const index = points.length; points.push(p); edges.push([previous, index]); previous = index; }
    edges.push([previous, b]);
  }
  if (points.length > 2048 || edges.length > 8192) throw new Error("Road planting exceeds the bounded city graph.");
  return { points, edges };
}
