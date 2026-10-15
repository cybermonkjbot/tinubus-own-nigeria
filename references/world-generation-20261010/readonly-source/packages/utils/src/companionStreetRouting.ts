import { navigationObstacleBounds, type RoomNavigation, type ScenePoint } from "./companionNavigation";

type ClearSegment = (nav: RoomNavigation, a: ScenePoint, b: ScenePoint) => boolean;
type LocalRoute = (nav: RoomNavigation, a: ScenePoint, b: ScenePoint) => ScenePoint[] | null;
const distance = (a: ScenePoint, b: ScenePoint) => Math.hypot(a[0] - b[0], a[2] - b[2]);

/** A bounded street A* joins collision-checked local approaches at either end. */
export function planStreetRoute(nav: RoomNavigation, start: ScenePoint, destination: ScenePoint,
  clear: ClearSegment, local: LocalRoute): ScenePoint[] | null {
  const graph = nav.streetGraph;
  if (!graph || graph.points.length > 2048 || graph.edges.length > 8192) return null;
  const points = graph.points;
  const approach = (a: ScenePoint, b: ScenePoint): ScenePoint[] | null => {
    if (clear(nav, a, b)) return [b];
    // Local precision stays at the room scale. Never allocate a city-sized grid.
    if (Math.abs(a[0] - b[0]) > 60 || Math.abs(a[2] - b[2]) > 60) return null;
    const bounds = { minX: Math.max(nav.bounds.minX, Math.min(a[0], b[0]) - 6),
      maxX: Math.min(nav.bounds.maxX, Math.max(a[0], b[0]) + 6),
      minZ: Math.max(nav.bounds.minZ, Math.min(a[2], b[2]) - 6),
      maxZ: Math.min(nav.bounds.maxZ, Math.max(a[2], b[2]) + 6) };
    const obstacles = nav.obstacles.filter(obstacle => {
      const o = navigationObstacleBounds(obstacle);
      return o.maxX + nav.radius >= bounds.minX && o.minX - nav.radius <= bounds.maxX
        && o.maxZ + nav.radius >= bounds.minZ && o.minZ - nav.radius <= bounds.maxZ;
    });
    return local({ ...nav, bounds, obstacles, streetGraph: undefined }, a, b);
  };
  // Each approach loop consumes these keys before the next query reuses them.
  const nearestKeys: number[] = [], nearestDistances = new Float64Array(12);
  const nearby = (at: ScenePoint) => {
    nearestKeys.length = 0;
    points.forEach((point, key) => {
      const metres = distance(at, point);
      if (!(metres <= 300) || nearestKeys.length === 12 && metres >= nearestDistances[11]) return;
      let index = nearestKeys.length;
      // Strict comparison keeps source order for equal distances, as the stable sort did.
      while (index > 0 && metres < nearestDistances[index - 1]) index--;
      for (let next = Math.min(nearestKeys.length, 11); next > index; next--) {
        nearestKeys[next] = nearestKeys[next - 1]; nearestDistances[next] = nearestDistances[next - 1];
      }
      nearestKeys[index] = key; nearestDistances[index] = metres;
    });
    return nearestKeys;
  };
  const starts = new Map<number, ScenePoint[]>(), ends = new Map<number, ScenePoint[]>();
  for (const key of nearby(start)) { const route = approach(start, points[key]); if (route) starts.set(key, route); }
  for (const key of nearby(destination)) { const route = approach(points[key], destination); if (route) ends.set(key, route); }
  const length = (from: ScenePoint, route: ScenePoint[]) => route.reduce((sum, p, i) => sum + distance(i ? route[i - 1] : from, p), 0);
  // Pack neighbours by node, retaining both directions and original edge order.
  // Recheck capacity after the caller-supplied approach functions have returned.
  if (points.length > 2048 || graph.edges.length > 8192) return null;
  const offsets = new Uint32Array(points.length + 1);
  for (const [a, b] of graph.edges) {
    if (!Number.isInteger(a) || !Number.isInteger(b) || a < 0 || b < 0
      || a >= points.length || b >= points.length || !points[a] || !points[b]) return null;
    offsets[a + 1]++; offsets[b + 1]++;
  }
  for (let index = 1; index < offsets.length; index++) offsets[index] += offsets[index - 1];
  // The admitted graph has at most 2,048 nodes, so neighbour IDs fit in 16 bits.
  const adjacent = new Uint16Array(offsets[points.length]), cursors = offsets.slice(0, points.length);
  for (const [a, b] of graph.edges) {
    adjacent[cursors[a]++] = b; adjacent[cursors[b]++] = a;
  }
  const cost = new Map<number, number>(), parents = new Map<number, number>();
  const open = new Set<number>(), closed = new Set<number>();
  for (const [key, route] of starts) { cost.set(key, length(start, route)); open.add(key); }
  while (open.size) {
    let key = -1, score = Infinity;
    for (const candidate of open) {
      const next = cost.get(candidate)! + distance(points[candidate], destination);
      if (next < score) { key = candidate; score = next; }
    }
    // An unresolved nonfinite score has no valid packed neighbour span.
    if (key < 0) return null;
    open.delete(key); closed.add(key);
    if (ends.has(key)) {
      const middle = [key];
      while (parents.has(middle[0])) middle.unshift(parents.get(middle[0])!);
      const path = [...starts.get(middle[0])!, ...middle.slice(1).map(i => points[i]), ...ends.get(key)!];
      const result: ScenePoint[] = [];
      let anchor = start, index = 0;
      while (index < path.length) {
        let next = path.length - 1;
        while (next > index && !clear(nav, anchor, path[next])) next--;
        if (!clear(nav, anchor, path[next])) return null;
        result.push(path[next]); anchor = path[next]; index = next + 1;
      }
      return result;
    }
    for (let index = offsets[key]; index < offsets[key + 1]; index++) {
      const next = adjacent[index];
      if (closed.has(next)) continue;
      const nextCost = cost.get(key)! + distance(points[key], points[next]);
      if (nextCost >= (cost.get(next) ?? Infinity) || !clear(nav, points[key], points[next])) continue;
      cost.set(next, nextCost); parents.set(next, key); open.add(next);
    }
  }
  return null;
}
