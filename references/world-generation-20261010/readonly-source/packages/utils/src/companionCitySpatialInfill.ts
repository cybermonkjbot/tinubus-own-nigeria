import type { CompanionCityParcel } from "./companionCity";
import type { CityArchitecturePart, CityArchitectureMaterial, CityArchitectureGeometry } from "./companionCityArchitectureDetails";
import { cityStreetSeed, sampleCityStreet, type CityStreetRoad } from "./companionCityStreetDetails";
import plan from "../../assets/city/lagos-city-v3.json";
import { companionCityParkingTreatment } from "./companionCityParking";
import { companionCityResidentialFrontageSpaces } from "./companionCityFrontages";

/** Authored junctions determine trade density. Walking-path crossings do not become markets. */
export function createCityStreetActivityLookup(roads: readonly CityStreetRoad[]) {
  const streets = roads.filter(r => !["retained", "path", "sand-path", "boardwalk", "bridge"].includes(r.kind));
  const segments = streets.flatMap(road => road.points.slice(1).map((b, i) => ({ road, a: road.points[i], b })));
  const junctions: { x: number; z: number; weight: number }[] = [];
  const add = (x: number, z: number, weight: number) => {
    const near = junctions.find(p => Math.hypot(p.x - x, p.z - z) < 12);
    if (near) near.weight = Math.max(near.weight, weight);
    else junctions.push({ x, z, weight });
  };
  for (let i = 0; i < segments.length; i++) for (let j = i + 1; j < segments.length; j++) {
    const a = segments[i], b = segments[j];
    if (a.road.id === b.road.id) continue;
    const weight = Math.min(1, .4 + Math.max(a.road.width, b.road.width) / 32);
    const ax = a.b[0] - a.a[0], az = a.b[1] - a.a[1], bx = b.b[0] - b.a[0], bz = b.b[1] - b.a[1];
    const determinant = ax * bz - az * bx;
    if (Math.abs(determinant) > 1e-8) {
      const dx = b.a[0] - a.a[0], dz = b.a[1] - a.a[1];
      const t = (dx * bz - dz * bx) / determinant, u = (dx * az - dz * ax) / determinant;
      if (t >= 0 && t <= 1 && u >= 0 && u <= 1) add(a.a[0] + ax * t, a.a[1] + az * t, weight);
    }
    // A T-junction may end at the carriageway edge rather than at its centre line.
    for (const [first, second] of [[a, b], [b, a]]) {
      const dx = second.b[0] - second.a[0], dz = second.b[1] - second.a[1], square = dx * dx + dz * dz;
      if (square < 1e-8) continue;
      for (const endpoint of [first.road.points[0], first.road.points.at(-1)!]) {
        if (endpoint !== first.a && endpoint !== first.b) continue;
        const t = Math.max(0, Math.min(1, ((endpoint[0] - second.a[0]) * dx + (endpoint[1] - second.a[1]) * dz) / square));
        const x = second.a[0] + dx * t, z = second.a[1] + dz * t;
        if (Math.hypot(endpoint[0] - x, endpoint[1] - z) <= second.road.width / 2 + 2) add(x, z, weight);
      }
    }
  }
  return (x: number, z: number) => junctions.reduce((pressure, point) =>
    Math.max(pressure, point.weight * Math.max(0, 1 - Math.hypot(point.x - x, point.z - z) / 85)), 0);
}
let tradeActivity: ReturnType<typeof createCityStreetActivityLookup> | undefined;
export function companionCityTradePressure(p: CompanionCityParcel) {
  tradeActivity ??= createCityStreetActivityLookup(plan.roads);
  return tradeActivity(p.entrance[0], p.entrance[2]);
}

/** Service drops terminate on actual low-rise masonry, outside the public walking reserve. */
export function companionCityCableAnchor(p: CompanionCityParcel): [number, number, number] | null {
  if (p.floors < 2 || p.kind === "tower" || p.style === "waterfront" || ["terrace-villa", "courtyard-villa"].includes(p.style)
    || !["south-market", "palm-courts"].includes(p.district)) return null;
  const x = -p.width * .42, z = p.depth / 2 - 1.13 - (companionCityParkingTreatment(p)?.forecourtDepth ?? 0);
  const c = Math.cos(p.yaw), s = Math.sin(p.yaw);
  return [p.x + x * c + z * s, Math.min(8, p.floors * 3.3 - .7) + .3, p.z - x * s + z * c];
}

export type CityEntranceConnection = {
  parcelId: string; road: CityStreetRoad; distance: number; side: number; width: number;
  gate: readonly [number, number]; edge: readonly [number, number]; timber: boolean;
};

/** Project actual gates onto their own street, never a fixed world-facing direction. */
export function companionCityEntranceConnections(parcels: readonly CompanionCityParcel[], roads: readonly CityStreetRoad[]): CityEntranceConnection[] {
  const byName = new Map(roads.filter(r => !["retained", "path", "sand-path", "boardwalk", "bridge"].includes(r.kind)).map(r => [r.name, r]));
  const result: CityEntranceConnection[] = [];
  for (const p of parcels) {
    const road = byName.get(p.street);
    if (!road) continue;
    const c = Math.cos(p.yaw), s = Math.sin(p.yaw);
    const entry = (p.entrance[0] - p.x) * c - (p.entrance[2] - p.z) * s;
    const front = p.depth / 2 - .14;
    const gate: [number, number] = [p.x + entry * c + front * s, p.z - entry * s + front * c];
    let travelled = 0, nearest = Infinity, distance = 0;
    for (let i = 1; i < road.points.length; i++) {
      const a = road.points[i - 1], b = road.points[i], dx = b[0] - a[0], dz = b[1] - a[1];
      const length = Math.hypot(dx, dz);
      if (length < .001) continue;
      const t = Math.max(0, Math.min(1, ((gate[0] - a[0]) * dx + (gate[1] - a[1]) * dz) / (length * length)));
      const gap = Math.hypot(gate[0] - a[0] - dx * t, gate[1] - a[1] - dz * t);
      if (gap < nearest) { nearest = gap; distance = travelled + length * t; }
      travelled += length;
    }
    const point = sampleCityStreet(road, distance), nx = -point.tz, nz = point.tx;
    const side = Math.sign((gate[0] - point.x) * nx + (gate[1] - point.z) * nz);
    const edge: [number, number] = [point.x + point.nx * side * (road.width / 2 + .02), point.z + point.nz * side * (road.width / 2 + .02)];
    const gap = Math.hypot(edge[0] - gate[0], edge[1] - gate[1]);
    // The gate faces the street and must not connect across another carriageway.
    if (!side || nearest < road.width / 2 + .7 || gap > 42 || gap < .25
      || ((edge[0] - gate[0]) * s + (edge[1] - gate[1]) * c) / gap < .75) continue;
    const width = p.garden > 0 ? 4.5 : 2.6;
    // Do not connect through another plot, even if its road has the same name.
    const obstructed = parcels.some(other => {
      if (other.id === p.id || Math.hypot(other.x - gate[0], other.z - gate[1]) > gap + other.width + other.depth + other.garden * 2) return false;
      const oc = Math.cos(other.yaw), os = Math.sin(other.yaw);
      const local = (point: readonly number[]) => {
        const dx = point[0] - other.x, dz = point[1] - other.z;
        return [dx * oc - dz * os, dx * os + dz * oc];
      };
      const a = local(gate), b = local(edge);
      const half = [other.width / 2 + other.garden + width / 2 + .15, other.depth / 2 + width / 2];
      let enter = 0, exit = 1;
      for (let axis = 0; axis < 2; axis++) {
        const delta = b[axis] - a[axis];
        if (Math.abs(delta) < 1e-8) { if (Math.abs(a[axis]) > half[axis]) return false; continue; }
        const low = (-half[axis] - a[axis]) / delta, high = (half[axis] - a[axis]) / delta;
        enter = Math.max(enter, Math.min(low, high)); exit = Math.min(exit, Math.max(low, high));
        if (enter > exit) return false;
      }
      return true;
    });
    if (obstructed) continue;
    result.push({ parcelId: p.id, road, distance, side, gate, edge, width,
      timber: p.garden === 0 && p.style === "market-shophouse" && cityStreetSeed(`${p.id}:bridge`) % 3 !== 0 });
  }
  return result;
}

/** Fit daily-life details to occupied plots. No new walking obstacles or random state. */
export function companionCitySpatialInfill(p: CompanionCityParcel, existing: readonly CityArchitecturePart[]): CityArchitecturePart[] {
  const result: CityArchitecturePart[] = [];
  const outer = p.width / 2 + p.garden, halfDepth = p.depth / 2;
  const entry = (p.entrance[0] - p.x) * Math.cos(p.yaw) - (p.entrance[2] - p.z) * Math.sin(p.yaw);
  const maintained = p.kind === "tower" || p.style === "waterfront" || ["terrace-villa", "courtyard-villa"].includes(p.style)
    || ["skyline", "lagoon-gardens", "garden-quarter"].includes(p.district);
  const pick = (salt: string) => cityStreetSeed(`${p.id}:infill:${salt}`) / 4294967296;
  const pressure = maintained ? 0 : companionCityTradePressure(p);
  const add = (role: string, material: CityArchitectureMaterial, x: number, y: number, z: number,
    w: number, h: number, d: number, colour: string, geometry: CityArchitectureGeometry = "box", rotation?: readonly [number, number, number]) => {
    result.push({ role: `infill ${role}`, material, x, y, z, w, h, d, colour, geometry, shadow: h > .08, ...(rotation ? { rotation } : {}) });
  };
  const masses = existing.filter(part => ["left courtyard wing", "right courtyard wing", "rear courtyard wing", "tower podium",
    "villa main", "villa terrace wing", "apartment main", "apartment return", "street building"].includes(part.role) && part.y - part.h / 2 < .05);
  for (const mass of masses) {
    const base = maintained ? "#b5b5a7" : p.district === "south-market" ? "#91836e" : "#a09e8a";
    for (const side of [-1, 1]) {
      add("foundation reveal", "stone", mass.x + side * (mass.w / 2 + .025), .19, mass.z, .06, .38, mass.d, base);
      add("ground contact", "groundSeam", mass.x + side * (mass.w / 2 + .12), -.004, mass.z, .19, .018, mass.d,
        maintained ? "#75796b" : "#646d55");
    }
    // Front bands stop at the saved entrance, including the courtyard's open axis.
    const low = mass.x - mass.w / 2, high = mass.x + mass.w / 2;
    for (const [start, end] of [[low, Math.min(high, entry - 1.7)], [Math.max(low, entry + 1.7), high]]) {
      if (end - start < .2) continue;
      add("foundation reveal", "stone", (start + end) / 2, .19, mass.z + mass.d / 2 + .025,
        end - start, .38, .055, base);
      add("ground contact", "groundSeam", (start + end) / 2, -.004, mass.z + mass.d / 2 + .12,
        end - start, .018, .19, maintained ? "#75796b" : "#675e49");
    }
  }
  const walls = existing.filter(part => /^compound (front|side|rear) wall$/.test(part.role)
    || part.role === "frontage boundary wall");
  for (const wall of walls) {
    // Fit weathering to the wall itself rather than a decal crossing open ground.
    add("wall damp foot", "stone", wall.x, .085, wall.z, wall.w + .012, .17, wall.d + .012,
      maintained ? "#a7a99a" : "#868675", "box", wall.rotation);
    if (!maintained && wall.role === "compound front wall" && wall.w > 4.5 && pick(`wall-graphics-${wall.x}`) < .25 + pressure * .3) {
      const z = wall.z + wall.d / 2 + .008, x = wall.x - wall.w * .2;
      const messages = ["COMMUNITY\nMUSIC NIGHT", "REMEMBERING\nOUR NEIGHBOURS", "PRAYER\nGATHERING"];
      for (let layer = 0; layer < 3; layer++) {
        const width = .6 + layer * .06;
        add("layered wall poster", "canvas", x + layer * .19, .82 + layer * .12, z + layer * .004,
          width, .72 - layer * .03, .002, "#ffffff");
        const poster = result[result.length - 1];
        poster.text = messages[(cityStreetSeed(p.id) + layer) % messages.length];
        poster.print = "poster"; poster.printVariant = layer; poster.shadow = false;
      }
      if (wall.w > 7) {
        add("painted local service", "wood", wall.x + wall.w * .2, .89, z + .006, Math.min(2, wall.w * .3), .38, .002, "#ffffff");
        const notice = result[result.length - 1]; notice.text = "REWIRE / AC GAS\nPLUMBER AVAILABLE";
        notice.print = "paint"; notice.shadow = false;
      }
    }
    if (maintained || wall.h < 1.25 || pick("wall-security") > .32) continue;
    const alongZ = wall.d > wall.w, length = alongZ ? wall.d : wall.w;
    const yaw = (wall.rotation?.[1] ?? 0) + (alongZ ? Math.PI / 2 : 0);
    const count = Math.floor((length - .6) / 1.05);
    for (let i = 0; i < count; i++) {
      const offset = (i - (count - 1) / 2) * 1.05;
      const x = wall.x + offset * Math.cos(yaw), z = wall.z - offset * Math.sin(yaw);
      const cap = wall.role === "compound front wall" ? .11 : 0;
      add("wall security coil", "metal", x, wall.y + wall.h / 2 + cap + .16, z, 1.08, .32, .2, "#70766b", "securityCoil", [0, yaw, 0]);
    }
  }
  type FixtureBounds = { x: number; z: number; w: number; d: number; y?: number; h?: number; rotation?: readonly [number, number, number] };
  const fixtures: FixtureBounds[] = [];
  const available = (x: number, z: number, w: number, d: number, height: number) => {
    if (Math.abs(x) + w / 2 > outer - .3 || Math.abs(z) + d / 2 > halfDepth - .35
      || Math.abs(x - entry) < w / 2 + 2.7) return false;
    const blocked: readonly FixtureBounds[] = [...existing, ...fixtures];
    return !blocked.some(part => {
      // A flush parking bay still reserves its full area, including space beside the car.
      const parkingBay = "role" in part && part.role === "parking paved bay";
      if (!parkingBay && part.h !== undefined && ((part.y ?? 0) - part.h / 2 >= height || part.h < .12)) return false;
      const yaw = part.rotation?.[1] ?? 0;
      const dx = (x - part.x) * Math.cos(yaw) - (z - part.z) * Math.sin(yaw);
      const dz = (x - part.x) * Math.sin(yaw) + (z - part.z) * Math.cos(yaw);
      const rx = (w * Math.abs(Math.cos(yaw)) + d * Math.abs(Math.sin(yaw))) / 2;
      const rz = (w * Math.abs(Math.sin(yaw)) + d * Math.abs(Math.cos(yaw))) / 2;
      return Math.abs(dx) < rx + part.w / 2 + .12 && Math.abs(dz) < rz + part.d / 2 + .12;
    });
  };
  const reserve = (x: number, z: number, w: number, d: number, height = 1.5) => {
    if (!available(x, z, w, d, height)) return false;
    fixtures.push({ x, z, w, d }); return true;
  };
  for (const space of companionCityResidentialFrontageSpaces(p)) {
    const { x, z, w, d, side, kind } = space;
    // Reserve the whole garden or court before adding incidental storage below.
    // This includes the kit tree's canopy, not only its trunk at ground level.
    if (!reserve(x, z, w, d, 4.7)) continue;
    const colour = maintained ? "#c2c3b2" : "#b8ae96";
    const edge = (ex: number, ez: number, ew: number, ed: number) =>
      add(`residential ${kind} edging`, "stone", ex, .06, ez, ew, .12, ed, colour);
    edge(x + side * (w / 2 - .06), z, .12, d);
    edge(x, z - d / 2 + .06, w - .24, .12);
    if (kind === "garden") {
      edge(x - side * (w / 2 - .06), z, .12, d);
      edge(x, z + d / 2 - .06, w - .24, .12);
    } else if (kind === "service") {
      add("residential service screen", "stone", x, .46, z - d / 2 + .08, w - .24, .92, .16, colour);
      add("residential service screen", "stone", x + side * (w / 2 - .08), .46, z - d * .15,
        .16, .92, d * .7, colour);
    }
    // Seating and service corners stay open towards the building and the front of the plot.
  }
  if (maintained) return result;
  // Side setbacks have space for residents' storage, while shop recesses remain commercial.
  if (p.garden >= 2.2 && ["south-market", "palm-courts"].includes(p.district)) {
    const side = pick("yard-side") < .5 ? -1 : 1;
    const x = side * (p.width / 2 + p.garden / 2), z = -halfDepth + 2.1;
    if (reserve(x, z, 1.05, 1.45)) {
      add("yard soil", "recess", x, .026, z, 1.08, .06, 1.5, "#8d7957");
      if (pick("yard-use") < .55) {
        for (let level = 0; level < 3; level++) for (let row = 0; row < 2; row++) {
          const bx = x + (row - .5) * .42;
          add("stored block", "stone", bx, .13 + level * .19, z, .38, .18, .66, "#a2a18e");
          for (const end of [-1, 1]) add("block cavity", "recess", bx, .224 + level * .19, z + end * .17, .2, .008, .16, "#555d51");
        }
      } else {
        for (let level = 0; level < 3; level++) add("stored tyre", "rubber", x, .16 + level * .2, z,
          .72, .19, .72, "#30372e", "tyre");
      }
      // Small drainage swells stay entirely in the already blocked side yard.
      for (const offset of [-.48, .48]) add("yard swell", "recess", x + offset, .07, z + .45, .12, .14, .55,
        "#747c52", "gable");
    }
    const ladderZ = halfDepth - 4.1;
    if (reserve(x, ladderZ, .6, .45, 2.64)) {
      for (const rail of [-1, 1]) add("stored ladder rail", "wood", x + rail * .24, 1.32, ladderZ, .065, 2.64, .065, "#85714e");
      for (let y = .3; y < 2.6; y += .32) add("stored ladder rung", "wood", x, y, ladderZ, .48, .05, .07, "#8f7c57");
    }
    const benchZ = halfDepth - 7.1;
    if (pressure > .3 && reserve(x, benchZ, 1.55, .55)) {
      add("pako bench seat", "wood", x, .45, benchZ, 1.48, .085, .42, "#887451");
      for (const side of [-1, 1]) {
        add("pako bench leg", "wood", x + side * .55, .21, benchZ, .12, .42, .35, "#786548");
        add("bench brace", "wood", x, .22, benchZ + side * .15, 1.16, .06, .055, "#786548");
      }
    }
  }
  if (p.style === "market-shophouse" || p.style === "corner-shop") {
    const face = Math.max(...masses.map(m => m.z + m.d / 2));
    for (const side of [-1, 1]) {
      const x = side * p.width * .3, z = face + .68;
      const layers = 2 + Math.floor(pressure * 6);
      if (pick(`shop-stock-${side}`) > .3 + pressure * .65 || !reserve(x, z, .75, .56, layers * .31)) continue;
      for (let level = 0; level < layers; level++) {
        const y = .135 + level * .31, colour = level % 2 ? "#657949" : "#9b6048";
        add("drink crate base", "recess", x, y - .1, z, .66, .07, .46, colour);
        for (const edge of [-1, 1]) {
          add("drink crate side", "recess", x + edge * .31, y, z, .035, .27, .46, colour);
          for (const lift of [-.1, .1]) add("drink crate rail", "recess", x, y + lift, z + edge * .22, .65, .04, .035, colour);
        }
        for (let bottle = 0; bottle < 3; bottle++) add("bottle stock", "glass", x - .19 + bottle * .19, y + .03, z,
          .09, .22, .09, "#547454", "pipe");
      }
    }
    const x = -p.width * .3, z = face + .72;
    if (pressure > .25 && reserve(x, z, 1.95, .76)) {
      add("fruit barrow tray", "metal", x, .73, z, 1.13, .13, .58, "#658064");
      for (const side of [-1, 1]) {
        add("barrow rail", "wood", x + .2, .91, z + side * .3, 1.05, .19, .045, "#94754c");
        add("barrow handle", "metal", x - .64, .65, z + side * .27, .61, .055, .055, "#5a6858");
        add("barrow stand", "metal", x - .25, .33, z + side * .2, .045, .66, .045, "#5a6858");
      }
      add("barrow wheel", "rubber", x + .47, .29, z, .13, .56, .56, "#30372e", "vehicleWheel", [0, Math.PI / 2, 0]);
      add("barrow axle", "metal", x + .47, .29, z, .06, .06, .55, "#798273");
      for (const side of [-1, 1]) add("barrow wheel fork", "metal", x + .47, .485, z + side * .11,
        .045, .39, .04, "#5a6858");
      for (let fruit = 0; fruit < 8; fruit++) add("barrow fruit", "recess", x - .34 + fruit % 4 * .2, .88,
        z - .14 + Math.floor(fruit / 4) * .26, .19, .18, .19, fruit % 3 ? "#b99142" : "#709050", "ball");
    }
  }
  const bay = existing.find(part => part.role === "parking forecourt ground");
  if (p.style === "workshop" && bay) {
    const x = p.width * .34, z = halfDepth - 1;
    if (reserve(x, z, 1.05, .8)) {
      add("compressor tank", "metal", x, .37, z, .38, .82, .38, "#a84e39", "pipe", [0, 0, Math.PI / 2]);
      add("compressor motor", "metal", x, .69, z, .36, .28, .28, "#636c5f");
      for (const side of [-1, 1]) add("compressor wheel", "rubber", x + side * .29, .12, z, .23, .23, .13, "#30372e", "ball");
      add("compressor handle", "metal", x - .39, .52, z, .04, .58, .04, "#575e52");
    }
    const tyreX = -p.width * .32, tyreZ = halfDepth - .8;
    if (pressure > .2 && reserve(tyreX, tyreZ, .82, .85)) {
      for (let layer = 0; layer < 3; layer++) add("vulcanizer tyre", "rubber", tyreX, .095 + layer * .19, tyreZ,
        .72, .19, .72, "#30372e", "tyre");
      add("tyre price board", "wood", tyreX, .79, tyreZ + .37, .52, .28, .025, "#313c32");
      result[result.length - 1].text = "TYRE REPAIRS";
      result[result.length - 1].print = "paint";
    }
  }
  const roof = existing.find(part => part.role === "pitched roof");
  if (roof && ["workshop", "warehouse", "market-shophouse"].includes(p.style) && pick("roof-ballast") < .4) {
    for (const side of [-1, 1]) {
      const x = side * roof.w * .32, y = roof.y + roof.h / 2;
      const rise = roof.h / roof.d * 2, pitch = Math.atan(rise);
      if (side < 0) {
        const offset = roof.d * .13;
        add("roof tyre ballast", "rubber", x, y - rise * offset + .075 / Math.cos(pitch), roof.z + offset,
          .64, .14, .64, "#3b4035", "tyre", [pitch, 0, 0]);
      }
      else {
        const offset = roof.d * .1;
        add("roof block ballast", "stone", x, y - rise * offset + .08 / Math.cos(pitch), roof.z - offset,
          .42, .16, .25, "#a09f8b", "box", [-pitch, 0, 0]);
      }
    }
  }
  // Selected workshop repairs use a tied scaffold inside the existing forecourt.
  if (p.style === "workshop" && bay && pick("repair-scaffold") < .22) {
    const face = Math.max(...masses.map(m => m.z + m.d / 2));
    const x = p.width * .26, z = face + 1.95;
    if (reserve(x, z, 2.1, .95, 4.86)) {
      for (const end of [-1, 1]) for (const depth of [-1, 1]) {
        const px = x + end * .86, pz = z + depth * .36;
        add("bamboo scaffold upright", "wood", px, 2.43, pz, .085, 4.86, .085, "#b3a476", "pipe");
        for (const y of [1.9, 3.65]) add("scaffold rope lashing", "wood", px, y, pz, .12, .11, .12, "#887853", "pipe");
      }
      for (const depth of [-1, 1]) for (const y of [1.9, 3.65])
        add("bamboo scaffold ledger", "wood", x, y, z + depth * .36, 1.85, .075, .075, "#b3a476");
      for (const end of [-1, 1]) add("scaffold wall tie", "metal", x + end * .86, 1.9,
        (face + z - .36) / 2, .035, .035, z - .36 - face, "#747f6c");
      add("scaffold work board", "wood", x, 1.93, z, 1.88, .09, .7, "#998259");
      const length = Math.hypot(1.72, 3.4);
      add("scaffold brace", "wood", x, 1.92, z + .36, .075, length, .075, "#a79667", "pipe", [0, 0, -Math.atan2(1.72, 3.4)]);
    }
  }
  const cable = companionCityCableAnchor(p);
  if (cable) {
    const x = -p.width * .42, z = halfDepth - 1.21 - (companionCityParkingTreatment(p)?.forecourtDepth ?? 0);
    add("service cutout box", "metal", x, cable[1] - .3, z, .34, .42, .14, "#7e8877");
    add("service insulator", "stone", x, cable[1], z + .08, .11, .18, .11, "#b7b69f", "pipe");
  }
  if (p.style === "market-shophouse" && p.floors > 1 && p.width > 12 && pick("trade-banner") < .24) {
    const front = Math.max(...masses.map(m => m.z + m.d / 2));
    const width = Math.min(5, p.width * .35), y = 4.75, z = front + .2;
    for (const side of [-1, 1]) add("banner tie", "metal", side * (width / 2 + .15), y + .28, z,
      .35, .024, .024, "#5d6758");
    add("trade banner", "canvas", 0, y, z, width, .61, .025, "#809567");
    result[result.length - 1].text = "RECHARGE & DATA";
    result[result.length - 1].print = "banner"; result[result.length - 1].shadow = false;
  }
  return result;
}
