import recipe from "../../assets/city/lagos-national-theatre-v1.json";
import { routeSegmentClear, type NavigationObstacle, type RoomNavigation, type ScenePoint } from "./companionNavigation";

/** Reference-led game geometry, placed in the fictional city rather than surveyed Lagos. */
export const companionCityTheatre = recipe;
export const COMPANION_CITY_THEATRE_REVISION = recipe.revision;
const round = (value: number) => Math.round(value * 1000) / 1000;
export function theatreEllipse(rx: number, rz: number, count = 48): [number, number][] {
  return Array.from({ length: count }, (_, i) => [round(recipe.center[0] + rx * Math.cos(i / count * Math.PI * 2)),
    round(recipe.center[1] + rz * Math.sin(i / count * Math.PI * 2))]);
}
export function theatrePolygonObstacle(polygon: number[][]): NavigationObstacle {
  return { minX: Math.min(...polygon.map(p => p[0])), maxX: Math.max(...polygon.map(p => p[0])),
    minZ: Math.min(...polygon.map(p => p[1])), maxZ: Math.max(...polygon.map(p => p[1])), polygon: polygon.map(p => [p[0], p[1]]) };
}
export const companionCityTheatreFootprint = theatrePolygonObstacle(theatreEllipse(...recipe.building.podium as [number, number]));
// Navigation admits at most eight vertices per polygon. Split the oval into bounded sectors.
const podiumRing = theatreEllipse(...recipe.building.podium as [number, number]);
const podiumObstacles = Array.from({ length: 8 }, (_, sector) => theatrePolygonObstacle([recipe.center,
  ...Array.from({ length: 7 }, (_, i) => podiumRing[(sector * 6 + i) % podiumRing.length])]));
function lakeObstacles(polygon: number[][]): NavigationObstacle[] {
  if (polygon.length <= 8) return [theatrePolygonObstacle(polygon)];
  const area = polygon.reduce((sum, a, i) => { const b = polygon[(i + 1) % polygon.length]; return sum + a[0] * b[1] - b[0] * a[1]; }, 0);
  const vertices = area > 0 ? [...polygon] : [...polygon].reverse(), result: NavigationObstacle[] = [];
  const cross = (a: number[], b: number[], p: number[]) => (b[0] - a[0]) * (p[1] - a[1]) - (b[1] - a[1]) * (p[0] - a[0]);
  while (vertices.length > 3) {
    const ear = vertices.findIndex((b, i) => {
      const a = vertices[(i + vertices.length - 1) % vertices.length], c = vertices[(i + 1) % vertices.length];
      return cross(a, b, c) > 0 && !vertices.some(p => p !== a && p !== b && p !== c
        && cross(a, b, p) >= 0 && cross(b, c, p) >= 0 && cross(c, a, p) >= 0);
    });
    if (ear < 0) throw new Error("Theatre basin boundary cannot be triangulated.");
    result.push(theatrePolygonObstacle([vertices[(ear + vertices.length - 1) % vertices.length], vertices[ear], vertices[(ear + 1) % vertices.length]]));
    vertices.splice(ear, 1);
  }
  result.push(theatrePolygonObstacle(vertices)); return result;
}
export const companionCityTheatreObstacles: NavigationObstacle[] = [...podiumObstacles,
  ...recipe.lakes.flatMap(lake => lakeObstacles(lake.points)),
  ...recipe.trees.map(tree => ({ minX: tree.x - .45, maxX: tree.x + .45, minZ: tree.z - .45, maxZ: tree.z + .45 }))];

/** Clone the admitted graph. Every new link is swept-disc checked against its own revision. */
export function companionCityTheatreGraph(base: RoomNavigation, navigation: RoomNavigation) {
  const points: ScenePoint[] = base.streetGraph!.points.map(p => [...p]);
  const edges: [number, number][] = base.streetGraph!.edges.map(edge => [...edge]);
  const index = (x: number, z: number) => {
    const existing = points.findIndex(p => Math.hypot(p[0] - x, p[2] - z) < .02);
    if (existing >= 0) return existing;
    points.push([round(x), 0, round(z)]); return points.length - 1;
  };
  const link = (a: number, b: number) => {
    if (a === b) return;
    if (!routeSegmentClear(navigation, points[a], points[b])) throw new Error("Theatre access route is obstructed.");
    if (!edges.some(edge => edge[0] === a && edge[1] === b || edge[0] === b && edge[1] === a)) edges.push([a, b]);
  };
  for (const road of [...recipe.roads, ...recipe.paths]) {
    let previous = index(road.points[0][0], road.points[0][1]);
    for (let i = 1; i < road.points.length; i++) {
      const a = road.points[i - 1], b = road.points[i], count = Math.ceil(Math.hypot(b[0] - a[0], b[1] - a[1]) / 30);
      for (let j = 1; j <= count; j++) {
        const next = index(a[0] + (b[0] - a[0]) * j / count, a[1] + (b[1] - a[1]) * j / count);
        link(previous, next); previous = next;
      }
    }
  }
  const start = recipe.roads[0].points[0], at = index(start[0], start[1]);
  const join = base.streetGraph!.points.map((p, i) => ({ p, i, distance: Math.hypot(p[0] - start[0], p[2] - start[1]) }))
    .filter(p => p.distance < 35 && p.i !== at).sort((a, b) => a.distance - b.distance)
    .find(p => routeSegmentClear(navigation, points[at], p.p));
  if (join) link(at, join.i);
  if (!edges.some(([a, b]) => a === at && b < base.streetGraph!.points.length || b === at && a < base.streetGraph!.points.length))
    throw new Error("Theatre road must connect to the existing city graph.");
  if (points.length > 2048 || edges.length > 8192) throw new Error("Theatre district exceeds the bounded city graph.");
  return { points, edges };
}
