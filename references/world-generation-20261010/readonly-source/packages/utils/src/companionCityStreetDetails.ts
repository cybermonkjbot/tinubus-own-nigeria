import plan from "../../assets/city/lagos-city-v3.json";
import type { NavigationObstacle } from "./companionNavigation";
import { cityUtilityPoleProfile, cityUtilityPoleSpan, type CityUtilityPoleProfile } from "./companionCityUtilityPoles";

export type CityStreetRoad = {
  id: string; name: string; kind: string; width: number; pavement: number;
  points: readonly (readonly number[])[];
};
export type CityStreetFamily = "civic" | "palm-courts" | "garden" | "market" | "service" | "quay" | "skyline" | "estate" | "bridge";
export type CityStreetProfile = {
  family: CityStreetFamily;
  asphalt: string;
  paving: string;
  kerb: string;
  drain: "open" | "covered";
  paintedKerb: boolean;
  lamp: "short-arm" | "lantern" | "twin-arm" | "quay";
  utilities: boolean;
  repairSpacing: number;
  jointSpacing: number;
};
export type CityStreetSample = {
  x: number; z: number; tx: number; tz: number; nx: number; nz: number; yaw: number;
};
export type CityStreetFixture = {
  id: string; roadId: string; distance: number; x: number; z: number; yaw: number;
  kind: "lamp" | "utility" | "compound-lamp" | "wall-sign";
  style: CityStreetProfile["lamp"]; height: number; variant: number; side: number;
  /** Signs contain only the fictional street names already in the authored city. */
  streetName?: string;
  /** Omitted only for fixtures wholly inside an existing plot obstacle or above head height on its wall. */
  footprint?: NavigationObstacle;
  reservedParcelId?: string;
  utility?: CityUtilityPoleProfile;
};
export type CityStreetWire = { id: string; roadId: string; from: [number, number, number]; to: [number, number, number];
  sag: number; strands?: number; diameter?: number };
export type CityStreetConstructionSpan = { start: number; end: number; side: number; footprint: NavigationObstacle };
export type CityStreetDetailsPlan = {
  version: string;
  streets: { road: CityStreetRoad; length: number; family: CityStreetFamily; fixtures: CityStreetFixture[] }[];
  wires: CityStreetWire[];
};
export type CityStreetCrossingLookup = (road: CityStreetRoad, distance: number, side: number) => "junction" | "driveway" | null;

const excludedKinds = new Set(["retained", "path", "sand-path", "boardwalk"]);
export const cityDetailStreets = plan.roads.filter(road => !excludedKinds.has(road.kind));

/** Stable variation. No clocks, random state, Three.js, browser or filesystem access. */
export function cityStreetSeed(value: string): number {
  let hash = 2166136261;
  for (let i = 0; i < value.length; i++) hash = Math.imul(hash ^ value.charCodeAt(i), 16777619);
  return hash >>> 0;
}

export function cityStreetLength(road: CityStreetRoad): number {
  let length = 0;
  for (let i = 1; i < road.points.length; i++) length += Math.hypot(road.points[i][0] - road.points[i - 1][0], road.points[i][1] - road.points[i - 1][1]);
  return length;
}

/** Short spans never cut a chord across an authored road vertex. */
export function cityStreetDetailSegments(road: CityStreetRoad): { start: number; end: number }[] {
  const spans: { start: number; end: number }[] = [];
  let travelled = 0;
  for (let i = 1; i < road.points.length; i++) {
    const length = Math.hypot(road.points[i][0] - road.points[i - 1][0], road.points[i][1] - road.points[i - 1][1]);
    for (let offset = 0; offset < length; offset += 18) {
      spans.push({ start: travelled + offset, end: travelled + Math.min(length, offset + 18) });
    }
    travelled += length;
  }
  return spans;
}

/** Uses the joined ribbon's endpoint normals, including across authored bends. */
export function sampleCityStreet(road: CityStreetRoad, distance: number): CityStreetSample {
  const normal = (i: number) => {
    const last = road.points.length - 1;
    // Keep the original frozen-city sampler unchanged. Extension loops use their joined closure.
    const closed = (road.id === "theatre-ring-drive" || road.id === "refinery-ring-road")
      && road.points[0][0] === road.points[last][0] && road.points[0][1] === road.points[last][1];
    const a = road.points[i === 0 && closed ? last - 1 : Math.max(0, i - 1)];
    const b = road.points[i === last && closed ? 1 : Math.min(last, i + 1)];
    const length = Math.hypot(b[0] - a[0], b[1] - a[1]) || 1;
    return [-(b[1] - a[1]) / length, (b[0] - a[0]) / length];
  };
  let travelled = 0;
  for (let i = 1; i < road.points.length; i++) {
    const a = road.points[i - 1], b = road.points[i], length = Math.hypot(b[0] - a[0], b[1] - a[1]);
    if (length < .001) continue;
    if (distance <= travelled + length || i === road.points.length - 1) {
      const t = Math.max(0, Math.min(1, (distance - travelled) / length));
      const na = normal(i - 1), nb = normal(i), tx = (b[0] - a[0]) / length, tz = (b[1] - a[1]) / length;
      return { x: a[0] + (b[0] - a[0]) * t, z: a[1] + (b[1] - a[1]) * t, tx, tz,
        nx: na[0] + (nb[0] - na[0]) * t, nz: na[1] + (nb[1] - na[1]) * t, yaw: Math.atan2(-tz, tx) };
    }
    travelled += length;
  }
  return { x: road.points[0][0], z: road.points[0][1], tx: 1, tz: 0, nx: 0, nz: 1, yaw: 0 };
}

const profiles: Record<CityStreetFamily, CityStreetProfile> = {
  civic: { family: "civic", asphalt: "#535750", paving: "#bbb7aa", kerb: "#c9c5b6", drain: "covered", paintedKerb: true, lamp: "short-arm", utilities: false, repairSpacing: 145, jointSpacing: 2.2 },
  "palm-courts": { family: "palm-courts", asphalt: "#55594e", paving: "#bcb6a5", kerb: "#c3beac", drain: "open", paintedKerb: true, lamp: "lantern", utilities: true, repairSpacing: 112, jointSpacing: 1.8 },
  garden: { family: "garden", asphalt: "#565c51", paving: "#c6c0ad", kerb: "#c9c5b5", drain: "open", paintedKerb: false, lamp: "lantern", utilities: false, repairSpacing: 180, jointSpacing: 2.4 },
  market: { family: "market", asphalt: "#5d594e", paving: "#b6a78e", kerb: "#bdb5a2", drain: "open", paintedKerb: false, lamp: "short-arm", utilities: true, repairSpacing: 78, jointSpacing: 1.5 },
  service: { family: "service", asphalt: "#56574e", paving: "#afa99a", kerb: "#b9b7a7", drain: "open", paintedKerb: false, lamp: "short-arm", utilities: true, repairSpacing: 88, jointSpacing: 3.1 },
  quay: { family: "quay", asphalt: "#505951", paving: "#c8c0a9", kerb: "#c3c3b3", drain: "covered", paintedKerb: true, lamp: "quay", utilities: false, repairSpacing: 160, jointSpacing: 2.6 },
  skyline: { family: "skyline", asphalt: "#4e5653", paving: "#c5c6b9", kerb: "#d0d0c3", drain: "covered", paintedKerb: true, lamp: "twin-arm", utilities: false, repairSpacing: 245, jointSpacing: 2.8 },
  estate: { family: "estate", asphalt: "#545b52", paving: "#cbc6b4", kerb: "#d0cab8", drain: "covered", paintedKerb: false, lamp: "lantern", utilities: false, repairSpacing: 270, jointSpacing: 2.5 },
  bridge: { family: "bridge", asphalt: "#505650", paving: "#c2c4b6", kerb: "#c8c9bb", drain: "covered", paintedKerb: false, lamp: "twin-arm", utilities: false, repairSpacing: 0, jointSpacing: 4.5 },
};

/** Long roads change finish with their district, while retaining their road hierarchy. */
export function cityStreetProfile(road: CityStreetRoad, sample = sampleCityStreet(road, cityStreetLength(road) / 2)): CityStreetProfile {
  if (road.id === "industrial-harbour-approach" || road.id === "refinery-ring-road")
    return { ...profiles.service, drain: "covered", paintedKerb: true, utilities: false };
  if (road.id === "theatre-marina-approach" || road.id === "theatre-ring-drive") return profiles.civic;
  if (road.kind === "bridge") return profiles.bridge;
  if (road.kind === "quay") return profiles.quay;
  if (road.kind === "market") return profiles.market;
  if (road.kind === "service") return profiles.service;
  // Match the plan author's land-use split, including the coastal plan's 400 m northward shift.
  const family: CityStreetFamily = sample.z > 880 ? sample.x < -520 ? "quay" : sample.x > 520 ? "estate" : "skyline"
    : sample.z < -980 ? "market" : sample.x < -550 ? "palm-courts" : sample.x > 550 ? "garden" : "civic";
  const profile = profiles[family];
  if (road.kind !== "boulevard" && road.kind !== "avenue") return profile;
  return { ...profile, drain: "covered", paintedKerb: true, lamp: road.width >= 22 ? "twin-arm" : "short-arm", utilities: false };
}

function segmentDistance(x: number, z: number, a: readonly number[], b: readonly number[]) {
  const dx = b[0] - a[0], dz = b[1] - a[1], square = dx * dx + dz * dz;
  const t = square ? Math.max(0, Math.min(1, ((x - a[0]) * dx + (z - a[1]) * dz) / square)) : 0;
  return Math.hypot(x - a[0] - t * dx, z - a[1] - t * dz);
}

function nearRoad(x: number, z: number, road: CityStreetRoad, clearance: number) {
  for (let i = 1; i < road.points.length; i++) {
    if (segmentDistance(x, z, road.points[i - 1], road.points[i]) < clearance) return true;
  }
  return false;
}

function inPolygon(x: number, z: number, polygon: readonly (readonly number[])[]) {
  let inside = false;
  for (let i = 0, j = polygon.length - 1; i < polygon.length; j = i++) {
    const a = polygon[i], b = polygon[j];
    if ((a[1] > z) !== (b[1] > z) && x < (b[0] - a[0]) * (z - a[1]) / (b[1] - a[1]) + a[0]) inside = !inside;
  }
  return inside;
}

function nearObstacle(x: number, z: number, box: NavigationObstacle, margin: number) {
  const cx = (box.minX + box.maxX) / 2, cz = (box.minZ + box.maxZ) / 2;
  const c = Math.cos(box.yaw ?? 0), s = Math.sin(box.yaw ?? 0);
  const dx = (x - cx) * c - (z - cz) * s, dz = (x - cx) * s + (z - cz) * c;
  return Math.abs(dx) < (box.maxX - box.minX) / 2 + margin && Math.abs(dz) < (box.maxZ - box.minZ) / 2 + margin;
}

function bankZ(bank: readonly (readonly number[])[], x: number) {
  for (let i = 1; i < bank.length; i++) if (x <= bank[i][0]) {
    const a = bank[i - 1], b = bank[i];
    return a[1] + (b[1] - a[1]) * Math.max(0, Math.min(1, (x - a[0]) / (b[0] - a[0])));
  }
  return bank[bank.length - 1][1];
}

/** A renderer-owned spatial lookup limits crossing queries to nearby authored segments and entrances. */
export function createCityStreetCrossingLookup(roads: readonly CityStreetRoad[] = plan.roads): CityStreetCrossingLookup {
  type Candidate = { roadId: string; a: readonly number[]; b: readonly number[]; width: number; kind: "junction" | "driveway" };
  const size = 48, cells = new Map<string, Candidate[]>();
  const add = (candidate: Candidate) => {
    const lowX = Math.floor((Math.min(candidate.a[0], candidate.b[0]) - candidate.width) / size);
    const highX = Math.floor((Math.max(candidate.a[0], candidate.b[0]) + candidate.width) / size);
    const lowZ = Math.floor((Math.min(candidate.a[1], candidate.b[1]) - candidate.width) / size);
    const highZ = Math.floor((Math.max(candidate.a[1], candidate.b[1]) + candidate.width) / size);
    for (let x = lowX; x <= highX; x++) for (let z = lowZ; z <= highZ; z++) {
      const key = `${x}:${z}`, values = cells.get(key) ?? [];
      values.push(candidate); cells.set(key, values);
    }
  };
  for (const road of roads) for (let i = 1; i < road.points.length; i++) add({ roadId: road.id,
    a: road.points[i - 1], b: road.points[i], width: road.width / 2 + road.pavement + (excludedKinds.has(road.kind) ? 1.1 : 2), kind: "junction" });
  for (const parcel of plan.parcels) {
    const road = cityDetailStreets.find(value => value.name === parcel.street);
    if (road) add({ roadId: road.id, a: [parcel.entrance[0], parcel.entrance[2]],
      b: [parcel.entrance[0] + Math.sin(parcel.yaw) * 42, parcel.entrance[2] + Math.cos(parcel.yaw) * 42], width: 2.8, kind: "driveway" });
  }
  return (road, distance, side) => {
    const point = sampleCityStreet(road, distance), offset = side * (road.width / 2 + .25);
    const x = point.x + point.nx * offset, z = point.z + point.nz * offset;
    if (Math.abs(x) < 88 && z > -102 && z < 76) return "junction";
    let driveway = false;
    for (const candidate of cells.get(`${Math.floor(x / size)}:${Math.floor(z / size)}`) ?? []) {
      if (candidate.kind === "junction" && candidate.roadId === road.id || candidate.kind === "driveway" && candidate.roadId !== road.id) continue;
      if (segmentDistance(x, z, candidate.a, candidate.b) >= candidate.width) continue;
      if (candidate.kind === "junction") return "junction";
      driveway = true;
    }
    return driveway ? "driveway" : null;
  };
}

/** Kerb/gutter solids share exact run boundaries with navigation. Crossings remain flush and unblocked. */
export function cityStreetConstructionSpans(road: CityStreetRoad, start: number, end: number,
  crossingAt: CityStreetCrossingLookup): CityStreetConstructionSpan[] {
  const result: CityStreetConstructionSpan[] = [];
  for (const side of [-1, 1]) {
    let run: number | null = null;
    const close = (finish: number) => {
      if (run === null || finish - run < .04) { run = null; return; }
      const a = sampleCityStreet(road, run), b = sampleCityStreet(road, finish);
      const inside = side * (road.width / 2 + .015), outside = side * (road.width / 2 + .73);
      const polygon: [number, number][] = [[a.x + a.nx * inside, a.z + a.nz * inside],
        [b.x + b.nx * inside, b.z + b.nz * inside], [b.x + b.nx * outside, b.z + b.nz * outside],
        [a.x + a.nx * outside, a.z + a.nz * outside]];
      result.push({ start: run, end: finish, side, footprint: {
        minX: Math.min(...polygon.map(point => point[0])), maxX: Math.max(...polygon.map(point => point[0])),
        minZ: Math.min(...polygon.map(point => point[1])), maxZ: Math.max(...polygon.map(point => point[1])), polygon,
      } });
      run = null;
    };
    for (let distance = start; distance < end; distance += .8) {
      const finish = Math.min(end, distance + .8);
      // Include the edges of each interval so a run cannot intrude into a small access gap.
      if (crossingAt(road, distance, side) || crossingAt(road, finish, side) || crossingAt(road, (distance + finish) / 2, side)) close(distance);
      else if (run === null) run = distance;
    }
    close(end);
  }
  return result;
}

export function cityStreetFixtureClear(road: CityStreetRoad, point: CityStreetSample, x: number, z: number): boolean {
  if (x < plan.bounds.minX + 2 || x > plan.bounds.maxX - 2 || z < plan.bounds.minZ + 2 || z > plan.bounds.maxZ - 2) return false;
  if (Math.abs(x) < 88 && z > -102 && z < 76) return false;
  for (const other of plan.roads) {
    if (other.id === road.id) continue;
    const path = excludedKinds.has(other.kind);
    if (nearRoad(x, z, other, other.width / 2 + other.pavement + (path ? 2 : 8))) return false;
  }
  if (road.kind !== "bridge") {
    if (z > bankZ(plan.lagoon.southBank, x) - 5 && z < bankZ(plan.lagoon.northBank, x) + 5) return false;
    if (plan.beaches.some(beach => inPolygon(x, z, beach.points))) return false;
  }
  if (plan.greenSpaces.some(space => inPolygon(x, z, space.points))) return false;
  if (plan.landmarkFootprints.some(box => nearObstacle(x, z, box, 2))) return false;
  if (plan.parcels.some(parcel => {
    if (nearObstacle(x, z, { minX: parcel.x - parcel.width / 2 - parcel.garden - .15,
      maxX: parcel.x + parcel.width / 2 + parcel.garden + .15, minZ: parcel.z - parcel.depth / 2,
      maxZ: parcel.z + parcel.depth / 2, yaw: parcel.yaw }, 1)) return true;
    if (Math.hypot(x - parcel.entrance[0], z - parcel.entrance[2]) < 8) return true;
    const far = [parcel.entrance[0] + Math.sin(parcel.yaw) * 42, parcel.entrance[2] + Math.cos(parcel.yaw) * 42];
    return segmentDistance(x, z, [parcel.entrance[0], parcel.entrance[2]], far) < 3;
  })) return false;
  if (plan.amenities.some(amenity => Math.hypot(x - amenity.x, z - amenity.z) <
    (amenity.kind === "market-canopy" ? 13 : amenity.kind === "fountain" ? 8 : amenity.kind === "tree" || amenity.kind === "palm" ? 4 : 3))) return false;
  for (const [a, b] of plan.streetGraph.edges) {
    if (segmentDistance(x, z, plan.streetGraph.points[a], plan.streetGraph.points[b]) < 1.35) return false;
  }
  // The column occupies the outer verge only. Never move it into the road when a candidate is rejected.
  return Math.hypot(x - point.x, z - point.z) >= road.width / 2 + (road.kind === "bridge" ? 2.2 : road.pavement + .45);
}

function parcelPoint(parcel: typeof plan.parcels[number], x: number, z: number): [number, number] {
  const c = Math.cos(parcel.yaw), s = Math.sin(parcel.yaw);
  return [parcel.x + x * c + z * s, parcel.z - x * s + z * c];
}

/** Shared source for visual fixtures and authoritative collision admission. It never mutates the base plan. */
export function createCompanionCityStreetDetailsPlan(): CityStreetDetailsPlan {
  const wires: CityStreetWire[] = [], crossingAt = createCityStreetCrossingLookup();
  const streets = cityDetailStreets.map(road => {
    const length = cityStreetLength(road), seed = cityStreetSeed(road.id), fixtures: CityStreetFixture[] = [];
    const baseProfile = cityStreetProfile(road);
    let station = road.kind === "bridge" ? 42 : 26 + seed % 23, index = 0;
    while (station < length - 18) {
      const point = sampleCityStreet(road, station), profile = cityStreetProfile(road, point);
      const variant = cityStreetSeed(`${road.id}:${index}`) % 6;
      const side = profile.utilities ? seed % 2 ? 1 : -1 : index % 2 ? 1 : -1;
      const offset = side * (road.width / 2 + (road.kind === "bridge" ? 2.65 : road.pavement + .8));
      const x = point.x + point.nx * offset, z = point.z + point.nz * offset;
      if (cityStreetFixtureClear(road, point, x, z)) {
        const utility = profile.utilities && index % 3 !== 1;
        const radius = utility ? .3 : .24;
        fixtures.push({ id: `street:${road.id}:${index}`, roadId: road.id, distance: station, x, z,
          yaw: point.yaw, kind: utility ? "utility" : "lamp", style: utility ? "short-arm" : profile.lamp, side, variant,
          height: utility ? 10.4 + variant * .12 : profile.lamp === "lantern" ? 4.8 : 7.8 + variant * .22,
          ...(utility ? { utility: cityUtilityPoleProfile(cityStreetSeed(`${road.id}:${index}:pole`),
            ["market", "service", "palm-courts"].includes(profile.family) ? "dense" : "mixed") } : {}),
          footprint: { minX: x - radius, maxX: x + radius, minZ: z - radius, maxZ: z + radius } });
      }
      station += road.kind === "bridge" ? 66 : profile.family === "market" ? 46 + variant * 4 : 64 + variant * 5;
      index++;
    }
    const parcels = plan.parcels.filter(parcel => parcel.street === road.name);
    for (const parcel of parcels) {
      // A slim compound lamp sits behind the existing front wall, within the reserved garden wing.
      if (parcel.garden < 3 || cityStreetSeed(parcel.id) % 3 !== 0) continue;
      const side = parcel.variant % 2 ? 1 : -1;
      const [x, z] = parcelPoint(parcel, side * (parcel.width / 2 + parcel.garden * .48), parcel.depth / 2 - 1.2);
      fixtures.push({ id: `street:compound:${parcel.id}`, roadId: road.id, distance: 0, x, z, yaw: parcel.yaw,
        kind: "compound-lamp", style: "lantern", side, height: 3.7, variant: parcel.variant, reservedParcelId: parcel.id });
    }
    // Name both approaches when possible. Facade plates have no new ground footprint.
    const external = fixtures.filter(fixture => !!fixture.footprint);
    if (external.length) {
      external[0].streetName = road.name;
      if (external.length > 1) external[external.length - 1].streetName = road.name;
    }
    if (parcels.length) {
      for (const parcel of [parcels[0], ...(parcels.length > 8 ? [parcels[parcels.length - 1]] : [])]) {
        const [x, z] = parcelPoint(parcel, -parcel.width * .3, parcel.depth / 2 + .055);
        fixtures.push({ id: `street:name:${parcel.id}`, roadId: road.id, distance: 0, x, z, yaw: parcel.yaw,
          kind: "wall-sign", style: "short-arm", side: 1, height: Math.min(3.9, parcel.floors * 3.3 - .65), variant: parcel.variant,
          streetName: road.name, reservedParcelId: parcel.id });
      }
    }
    const poles = external.filter(fixture => fixture.kind === "utility");
    for (let i = 1; i < poles.length; i++) {
      const a = poles[i - 1], b = poles[i], distance = Math.hypot(b.x - a.x, b.z - a.z);
      if (a.side !== b.side || distance > 155 || distance < 12) continue;
      const middle = sampleCityStreet(road, (a.distance + b.distance) / 2);
      if (crossingAt(road, (a.distance + b.distance) / 2, a.side)) continue;
      // Don't string across a sharp bend or cut diagonally through a compound.
      if (segmentDistance(middle.x, middle.z, [a.x, a.z], [b.x, b.z]) < road.width / 2) continue;
      if (plan.parcels.some(parcel => segmentDistance(parcel.x, parcel.z, [a.x, a.z], [b.x, b.z]) < Math.max(parcel.width, parcel.depth) / 2 + parcel.garden)) continue;
      wires.push(...cityUtilityPoleSpan(a, b, `street:wire:${a.id}:${b.id}`));
    }
    return { road, length, family: baseProfile.family, fixtures };
  });
  return { version: `${plan.version}:street-details-v1`, streets, wires };
}

/** Admit these exact obstacles in the central navigation revision before attaching the detail renderer. */
export function companionCityStreetDetailFootprints(): NavigationObstacle[] {
  const source = createCompanionCityStreetDetailsPlan(), crossingAt = createCityStreetCrossingLookup();
  const footprints = source.streets.flatMap(street => street.fixtures.flatMap(fixture => fixture.footprint ? [{ ...fixture.footprint }] : []));
  for (const street of source.streets) {
    const spans = cityStreetDetailSegments(street.road).flatMap(({ start, end }) => cityStreetConstructionSpans(street.road, start, end, crossingAt));
    for (const side of [-1, 1]) {
      let inner: [number, number][] = [], outer: [number, number][] = [], previousEnd = -Infinity;
      const flush = () => {
        if (!inner.length) return;
        // Up to three contiguous quadrilaterals become one exact eight-vertex boundary.
        const polygon = [...inner, ...outer.reverse()];
        footprints.push({ minX: Math.min(...polygon.map(point => point[0])), maxX: Math.max(...polygon.map(point => point[0])),
          minZ: Math.min(...polygon.map(point => point[1])), maxZ: Math.max(...polygon.map(point => point[1])), polygon });
        inner = []; outer = [];
      };
      for (const span of spans.filter(value => value.side === side)) {
        if (inner.length === 4 || Math.abs(span.start - previousEnd) > .001) flush();
        const polygon = span.footprint.polygon!;
        if (!inner.length) { inner.push([polygon[0][0], polygon[0][1]]); outer.push([polygon[3][0], polygon[3][1]]); }
        inner.push([polygon[1][0], polygon[1][1]]); outer.push([polygon[2][0], polygon[2][1]]);
        previousEnd = span.end;
      }
      flush();
    }
  }
  return footprints;
}
