import plan from "../../assets/city/lagos-city-v3.json";
import signatures from "../../assets/city/lagos-architecture-delivery-v1.json";
import { cityStreetLength, sampleCityStreet, type CityStreetRoad } from "./companionCityStreetDetails";
import { companionCityResidentialFrontageSpaces } from "./companionCityFrontages";

type Point = readonly [number, number];
type Parcel = typeof plan.parcels[number];
export type CityNeighbourhoodSurface = {
  id: string;
  parcelId: string;
  kind: "compound" | "forecourt" | "garden" | "path" | "border";
  points: Point[];
  colour: string;
  yaw: number;
};

const point = (p: Parcel, x: number, z: number): Point => {
  const c = Math.cos(p.yaw), s = Math.sin(p.yaw);
  return [p.x + x * c + z * s, p.z - x * s + z * c];
};
const local = (p: Parcel, x: number, z: number): Point => {
  const dx = x - p.x, dz = z - p.z, c = Math.cos(p.yaw), s = Math.sin(p.yaw);
  return [dx * c - dz * s, dx * s + dz * c];
};
const rectangle = (p: Parcel, left: number, right: number, back: number, front: number): Point[] =>
  [point(p, left, back), point(p, right, back), point(p, right, front), point(p, left, front)];

/** Convex separating axes, with a clearance in metres for each occupied footprint. */
type Bounds = { minX: number; maxX: number; minZ: number; maxZ: number };
const polygonBounds = new WeakMap<readonly Point[], Bounds>();
function bounds(polygon: readonly Point[]) {
  let result = polygonBounds.get(polygon);
  if (!result) {
    result = { minX: Infinity, maxX: -Infinity, minZ: Infinity, maxZ: -Infinity };
    for (const [x, z] of polygon) {
      result.minX = Math.min(result.minX, x); result.maxX = Math.max(result.maxX, x);
      result.minZ = Math.min(result.minZ, z); result.maxZ = Math.max(result.maxZ, z);
    }
    polygonBounds.set(polygon, result);
  }
  return result;
}
function overlaps(a: readonly Point[], b: readonly Point[], clearance = 0) {
  const ab = bounds(a), bb = bounds(b);
  if (ab.maxX + clearance <= bb.minX || bb.maxX + clearance <= ab.minX
    || ab.maxZ + clearance <= bb.minZ || bb.maxZ + clearance <= ab.minZ) return false;
  for (const polygon of [a, b]) for (let i = 0; i < polygon.length; i++) {
    const from = polygon[i], to = polygon[(i + 1) % polygon.length];
    const length = Math.hypot(to[0] - from[0], to[1] - from[1]);
    if (length < .001) continue;
    const nx = -(to[1] - from[1]) / length, nz = (to[0] - from[0]) / length;
    const aa = a.map(p => p[0] * nx + p[1] * nz), bb = b.map(p => p[0] * nx + p[1] * nz);
    if (Math.max(...aa) + clearance <= Math.min(...bb) || Math.max(...bb) + clearance <= Math.min(...aa)) return false;
  }
  return true;
}

function nearestStreet(p: Parcel, road: CityStreetRoad) {
  let station = 0, nearest = Infinity, selected = 0;
  for (let i = 1; i < road.points.length; i++) {
    const a = road.points[i - 1], b = road.points[i], dx = b[0] - a[0], dz = b[1] - a[1];
    const length = Math.hypot(dx, dz);
    if (length < .001) continue;
    const t = Math.max(0, Math.min(1, ((p.x - a[0]) * dx + (p.z - a[1]) * dz) / (length * length)));
    const distance = Math.hypot(p.x - a[0] - dx * t, p.z - a[1] - dz * t);
    if (distance < nearest) { nearest = distance; selected = station + t * length; }
    station += length;
  }
  return sampleCityStreet(road, Math.max(0, Math.min(cityStreetLength(road), selected)));
}

let surfaces: readonly CityNeighbourhoodSurface[] | undefined;

/** Ground finishes only: existing addresses, solid footprints and frozen routes stay unchanged. */
export function companionCityNeighbourhoodSurfaces(): readonly CityNeighbourhoodSurface[] {
  if (surfaces) return surfaces;
  const result: CityNeighbourhoodSurface[] = [];
  const roads = new Map(plan.roads.filter(r => !["path", "sand-path", "boardwalk", "bridge", "retained"].includes(r.kind))
    .map(r => [r.name, r]));
  const footprints = plan.parcels.map(p => ({ p, points: rectangle(p, -p.width / 2 - p.garden, p.width / 2 + p.garden, -p.depth / 2, p.depth / 2) }));
  const roadReserves = plan.roads.flatMap(road => road.points.slice(1).flatMap((b, i) => {
    const a = road.points[i], dx = b[0] - a[0], dz = b[1] - a[1], length = Math.hypot(dx, dz);
    if (length < .001) return [];
    const reserve = road.width / 2 + road.pavement + .4, nx = -dz / length * reserve, nz = dx / length * reserve;
    const points: Point[] = [[a[0] + nx, a[1] + nz], [b[0] + nx, b[1] + nz],
      [b[0] - nx, b[1] - nz], [a[0] - nx, a[1] - nz]];
    return [{ roadId: road.id, points }];
  }));
  // AABB reserves deliberately err on the side of leaving public parks and shores unchanged.
  const protectedGround: Point[][] = [...plan.greenSpaces, ...plan.beaches].map(space => {
    const x = space.points.map(p => p[0]), z = space.points.map(p => p[1]);
    return [[Math.min(...x), Math.min(...z)], [Math.max(...x), Math.min(...z)],
      [Math.max(...x), Math.max(...z)], [Math.min(...x), Math.max(...z)]];
  });
  protectedGround.push([[-42, -53], [42, -53], [42, 42], [-42, 42]]);
  const accepted: Point[][] = [];
  const signatureIds = new Set(signatures.buildings.map(building => building.parcelId));
  for (const p of plan.parcels) {
    if (!p.units || p.garden < 2 || signatureIds.has(p.id)) continue;
    const affluent = p.kind === "tower" || ["skyline", "marina", "lagoon-gardens"].includes(p.district);
    const paving = affluent ? "#b9b6a9" : p.district === "palm-courts" ? "#b9ad95" : "#beb6a2";
    const garden = affluent ? "#71855c" : "#7f8b61", border = affluent ? "#d0ccbb" : "#d0c2a5";
    const add = (suffix: string, kind: CityNeighbourhoodSurface["kind"], points: Point[], colour: string) =>
      result.push({ id: `${p.id}:${suffix}`, parcelId: p.id, kind, points, colour, yaw: p.yaw });
    const half = p.width / 2 + p.garden - .25, back = -p.depth / 2 + .25, front = p.depth / 2 - .25;
    add("grounds", "compound", rectangle(p, -half, half, back, front), paving);
    const residentialSpaces = companionCityResidentialFrontageSpaces(p);
    for (const [index, space] of residentialSpaces.entries()) {
      add(`residential-${space.kind}-${index}`, space.kind === "garden" ? "garden" : "path",
        rectangle(p, space.x - space.w / 2, space.x + space.w / 2, space.z - space.d / 2, space.z + space.d / 2),
        space.kind === "garden" ? garden : border);
    }
    // Other homes retain their planted side strips; shared courts use the exact kit plan above.
    if (!residentialSpaces.length) for (const side of [-1, 1]) {
      const a = side * (p.width / 2 + .25), b = side * (half - .35);
      add(`side-garden-${side}`, "garden", rectangle(p, Math.min(a, b), Math.max(a, b), back + .35, front - 1.2), garden);
    }
    const road = roads.get(p.street);
    if (!road) continue;
    const street = nearestStreet(p, road), c = Math.cos(p.yaw), s = Math.sin(p.yaw);
    const side = Math.sign((p.x - street.x) * -street.tz + (p.z - street.z) * street.tx);
    const edge = local(p, street.x - street.tz * side * (road.width / 2 + road.pavement + .18),
      street.z + street.tx * side * (road.width / 2 + road.pavement + .18));
    const tx = street.tx * c - street.tz * s, tz = street.tx * s + street.tz * c;
    // Reject a street behind the plot, hairpins, junction corners and excessive setbacks.
    if (!side || Math.abs(tx) < .88 || edge[1] < front + 1.4 || edge[1] > front + 38) continue;
    const end = (x: number) => edge[1] + (x - edge[0]) * tz / tx;
    const entry = local(p, p.entrance[0], p.entrance[2])[0];
    let width = half;
    let court: Point[] | undefined;
    const nearby = footprints.filter(other => other.p.id !== p.id && Math.hypot(other.p.x - p.x, other.p.z - p.z) < 140);
    for (const scale of [1, .8, .6]) {
      width = half * scale;
      if (width < Math.abs(entry) + 3 || Math.min(end(-width), end(width)) < front + 1.2) continue;
      const candidate = [point(p, -width, front + .3), point(p, width, front + .3),
        point(p, width, end(width)), point(p, -width, end(-width))];
      if (nearby.some(other => overlaps(candidate, other.points, .2))
        || protectedGround.some(ground => overlaps(candidate, ground, .3))
        || accepted.some(ground => overlaps(candidate, ground, .2))) continue;
      // Another street or walking path is never turned into a private forecourt.
      const crossesRoad = roadReserves.some(other => other.roadId !== road.id && overlaps(candidate, other.points));
      if (crossesRoad) continue;
      court = candidate; break;
    }
    if (!court) continue;
    accepted.push(court);
    add("arrival-court", "forecourt", court, paving);
    // A continuous door-to-pavement route splits the space into two legible gardens.
    const pathHalf = p.kind === "tower" ? 2.5 : 2.1;
    const laneLeft = Math.max(-width + .45, entry - pathHalf), laneRight = Math.min(width - .45, entry + pathHalf);
    const rear = front + .75;
    add("entrance-path", "path", [point(p, laneLeft, rear), point(p, laneRight, rear),
      point(p, laneRight, end(laneRight)), point(p, laneLeft, end(laneLeft))], border);
    for (const [index, left, right] of [[0, -width + .65, laneLeft - .8], [1, laneRight + .8, width - .65]]) {
      const far = Math.min(end(left), end(right)) - 1.5;
      if (right - left < 2 || far - rear < 2.6) continue;
      add(`forecourt-garden-${index}`, "garden", rectangle(p, left, right, rear + .5, far), garden);
      // Flush stone edging defines the bed without introducing an unregistered obstacle.
      add(`garden-edge-${index}`, "border", rectangle(p, left, right, far, far + .18), border);
      if (far - rear > 8) add(`garden-crossing-${index}`, "path", rectangle(p, left, right,
        rear + (far - rear) * .55, rear + (far - rear) * .55 + 1.3), paving);
    }
  }
  surfaces = result;
  return surfaces;
}
