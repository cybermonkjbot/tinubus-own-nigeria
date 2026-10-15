import { routeSegmentClear, type NavigationObstacle, type RoomNavigation, type ScenePoint } from "./companionNavigation";

export const companionCityRefinery = {
  id: "refinery", name: "Refinery", district: "Industrial Harbour",
  position: [2300, 0, 1570] as const,
  bounds: { minX: -2700, maxX: 2700, minZ: -1800, maxZ: 2100 },
  arrival: [2300, 0, 1810] as const,
  // A public loop surrounds the accepted source site. Process equipment remains reserved.
  site: { minX: 2100, maxX: 2500, minZ: 1320, maxZ: 1770 },
  height: 100,
  roads: [
    { id: "industrial-harbour-approach", name: "Industrial Harbour Road", kind: "avenue", width: 20, pavement: 3,
      points: [[1340, 1220], [1470, 1245], [1710, 1245], [1920, 1245], [2070, 1245], [2300, 1245]] },
    { id: "refinery-ring-road", name: "Refinery Drive", kind: "avenue", width: 20, pavement: 3,
      points: [[2300, 1245], [2580, 1245], [2580, 1840], [2030, 1840], [2030, 1245], [2300, 1245]] },
  ],
} as const;

export const companionCityRefineryObstacle: NavigationObstacle = companionCityRefinery.site;

/** New extension links attach to the existing estate road without moving its plots. */
export function companionCityRefineryGraph(base: RoomNavigation, navigation: RoomNavigation) {
  const points: ScenePoint[] = base.streetGraph!.points.map(p => [...p]);
  const edges: [number, number][] = base.streetGraph!.edges.map(edge => [...edge]);
  const index = (x: number, z: number) => {
    const found = points.findIndex(p => Math.hypot(p[0] - x, p[2] - z) < .03);
    if (found >= 0) return found;
    points.push([Math.round(x * 1000) / 1000, navigation.floorY, Math.round(z * 1000) / 1000]);
    return points.length - 1;
  };
  const link = (a: number, b: number) => {
    if (a === b || edges.some(e => e[0] === a && e[1] === b || e[0] === b && e[1] === a)) return;
    if (!routeSegmentClear(navigation, points[a], points[b])) throw new Error("Industrial Harbour access is obstructed.");
    edges.push([a, b]);
  };
  for (const road of companionCityRefinery.roads) {
    let previous = index(road.points[0][0], road.points[0][1]);
    for (let i = 1; i < road.points.length; i++) {
      const a = road.points[i - 1], b = road.points[i], count = Math.ceil(Math.hypot(b[0] - a[0], b[1] - a[1]) / 40);
      for (let j = 1; j <= count; j++) {
        const next = index(a[0] + (b[0] - a[0]) * j / count, a[1] + (b[1] - a[1]) * j / count);
        link(previous, next); previous = next;
      }
    }
  }
  const start = companionCityRefinery.roads[0].points[0], entrance = index(...start);
  const join = base.streetGraph!.points.map((p, i) => ({ p, i, distance: Math.hypot(p[0] - start[0], p[2] - start[1]) }))
    .filter(p => p.distance < 45 && p.i !== entrance).sort((a, b) => a.distance - b.distance)
    .find(p => routeSegmentClear(navigation, points[entrance], p.p));
  if (join) link(entrance, join.i);
  if (!edges.some(([a, b]) => a === entrance && b < base.streetGraph!.points.length || b === entrance && a < base.streetGraph!.points.length))
    throw new Error("Industrial Harbour must connect to the existing city.");
  if (points.length > 2048 || edges.length > 8192) throw new Error("Industrial Harbour exceeds the city graph budget.");
  return { points, edges };
}
