import type { CompanionCityParcel } from "./companionCity";
import type { CityArchitecturePart } from "./companionCityArchitectureDetails";
import { companionCityParkingParts } from "./companionCityParking";
import signatures from "../../assets/city/lagos-architecture-delivery-v1.json";

export type CityFrontageBoundary = "open" | "low-wall" | "railings" | "slats" | "masonry";
export type CityFrontageProp = {
  kind: "lawn" | "paving" | "shrub" | "flowers" | "croton" | "agave" | "tree" | "planter" | "bench" | "bin" | "hose";
  x: number; z: number; yaw: number; scale: readonly [number, number, number];
};
const signatureIds = new Set(signatures.buildings.map(b => b.parcelId));

function variation(id: string, salt: string) {
  let hash = 2166136261;
  for (const char of `${id}:${salt}`) hash = Math.imul(hash ^ char.charCodeAt(0), 16777619);
  hash ^= hash >>> 16; hash = Math.imul(hash, 0x85ebca6b);
  hash ^= hash >>> 13; hash = Math.imul(hash, 0xc2b2ae35); hash ^= hash >>> 16;
  return (hash >>> 0) / 4294967296;
}

function sharedResidentialPlot(p: CompanionCityParcel) {
  return p.units > 1 && (p.kind === "apartment" || p.kind === "tower" || p.style === "courtyard" || p.style === "waterfront");
}

/** Signature exteriors keep their authored grounds. Shared residential gardens are not district-gated. */
export function companionCityFrontageTreatment(p: CompanionCityParcel) {
  const residential = sharedResidentialPlot(p);
  if (p.garden < 3 || signatureIds.has(p.id)
    || !residential && !["garden-quarter", "lagoon-gardens", "skyline", "marina"].includes(p.district)) return null;
  const n = variation(p.id, "frontage-boundary");
  const boundary: CityFrontageBoundary = residential
    ? n < .12 ? "open" : n < .68 ? "low-wall" : n < .9 ? "railings" : "masonry"
    : n < .25 ? "open" : n < .47 ? "low-wall" : n < .68 ? "railings" : n < .85 ? "slats" : "masonry";
  return { boundary, landscaped: residential || variation(p.id, "frontage-landscape") < .9 };
}

type ResidentialFrontageSpace = {
  kind: "seating" | "garden" | "service";
  x: number; z: number; w: number; d: number; side: number;
};

/** Shared by kit placement and masonry edging so both describe the same bounded spaces. */
export function companionCityResidentialFrontageSpaces(p: CompanionCityParcel): ResidentialFrontageSpace[] {
  if (!sharedResidentialPlot(p) || !companionCityFrontageTreatment(p)?.landscaped) return [];
  const spaces: ResidentialFrontageSpace[] = [], halfDepth = p.depth / 2;
  const bays = companionCityParkingParts(p).filter(part => part.role === "parking paved bay");
  const seatingSide = bays.length ? -Math.sign(bays[0].x) : variation(p.id, "seating-side") < .5 ? -1 : 1;
  const add = (kind: ResidentialFrontageSpace["kind"], lowX: number, highX: number, back: number, front: number, side: number) => {
    if (highX - lowX < 1.7 || front - back < 1.8) return;
    spaces.push({ kind, x: (lowX + highX) / 2, z: (back + front) / 2, w: highX - lowX, d: front - back, side });
  };
  for (const side of [-1, 1]) {
    const x = side * (p.width / 2 + p.garden / 2), width = Math.min(p.garden - 1.1, 4.8);
    const sideBays = bays.filter(bay => Math.sign(bay.x) === side).sort((a, b) => a.z - b.z);
    // Allocate only clear intervals, including every car in a multi-bay compound.
    let back = -halfDepth + 1;
    const intervals: { back: number; front: number }[] = [];
    for (const bay of sideBays) {
      const front = bay.z - bay.d / 2 - .8;
      if (front > back) intervals.push({ back, front });
      back = Math.max(back, bay.z + bay.d / 2 + .8);
    }
    if (back < halfDepth - 1.05) intervals.push({ back, front: halfDepth - 1.05 });
    const low = x - width / 2, high = x + width / 2;
    const largest = intervals.reduce<{ back: number; front: number } | undefined>((best, value) =>
      !best || value.front - value.back > best.front - best.back ? value : best, undefined);
    for (const interval of intervals) {
      const { back, front } = interval;
      if (interval !== largest || front - back < 3.6) add("garden", low, high, back, front, side);
      else if (side === seatingSide) {
        const seatBack = Math.max(back, front - 4.8);
        add("garden", low, high, back, seatBack - .6, side);
        add("seating", low, high, seatBack, front, side);
      } else {
        add("service", low, high, back, back + 3, side);
        add("garden", low, high, back + 3.6, front, side);
      }
    }
  }
  if (p.style === "courtyard") {
    const entry = (p.entrance[0] - p.x) * Math.cos(p.yaw) - (p.entrance[2] - p.z) * Math.sin(p.yaw);
    // The U-shaped shell occupies the outer 30% on each side and the rear 32%.
    // Keep 5.4 m around the saved doorway axis and clearance from all three wings.
    const inner = p.width * .2 - .55, back = -p.depth * .18 + 1, front = halfDepth - 3;
    for (const side of [-1, 1]) {
      const low = side < 0 ? -inner : Math.max(-inner, entry + 2.75);
      const high = side < 0 ? Math.min(inner, entry - 2.75) : inner;
      add("garden", low, high, back, front - 5.4, side);
      add("seating", low, high, front - 4.8, front, side);
    }
  }
  return spaces;
}

function residentialFrontageProps(spaces: readonly ResidentialFrontageSpace[]): CityFrontageProp[] {
  const values: CityFrontageProp[] = [], occupied: { x: number; z: number; rx: number; rz: number }[] = [];
  const put = (space: ResidentialFrontageSpace, kind: CityFrontageProp["kind"], x: number, z: number,
    width: number, depth: number, yaw = 0, scale: readonly [number, number, number] = [1, 1, 1]) => {
    const c = Math.abs(Math.cos(yaw)), s = Math.abs(Math.sin(yaw));
    const rx = (width * c + depth * s) / 2, rz = (width * s + depth * c) / 2;
    const ground = kind === "lawn" || kind === "paving";
    if (Math.abs(x - space.x) + rx > space.w / 2 + 1e-7 || Math.abs(z - space.z) + rz > space.d / 2 + 1e-7) return;
    if (!ground && occupied.some(other => Math.abs(x - other.x) < rx + other.rx + .18
      && Math.abs(z - other.z) < rz + other.rz + .18)) return;
    values.push({ kind, x, z, yaw, scale });
    if (!ground) occupied.push({ x, z, rx, rz });
  };
  for (const space of spaces) {
    const { x, z, w, d, side } = space;
    put(space, space.kind === "garden" ? "lawn" : "paving", x, z, w, d, 0, [w, 1, d]);
    if (space.kind === "seating") {
      put(space, "bench", x + side * (w / 2 - .54), z - .6, 1.75, .72, -side * Math.PI / 2);
      if (w >= 3.5) put(space, "bench", x - side * (w / 2 - .54), z - .6, 1.75, .72, side * Math.PI / 2);
      put(space, "planter", x, z + d / 2 - .85, 1.5, 1.2);
    } else if (space.kind === "service") {
      for (const offset of [-.4, .4]) put(space, "bin", x + offset, z - .65, .58, .68);
      put(space, "hose", x, z + .65, .65, .65);
    } else {
      const treeScale = Math.min(1, (w - .3) / 1.8, (d - .3) / 1.8);
      put(space, "tree", x, z - d / 2 + 1.1, 1.8 * treeScale, 1.8 * treeScale, 0, [treeScale, treeScale, treeScale]);
      let index = 0;
      for (let at = z - d / 2 + 3.3; at < z + d / 2 - .85; at += 2.2) {
        const kind = (["shrub", "flowers", "croton", "agave"] as const)[index++ % 4];
        put(space, kind, x, at, 1.55, 1.55, side < 0 ? Math.PI : 0);
      }
    }
  }
  return values;
}

/** Boundary replaces the old uniform wall. Open plots have no fence, pillars or gate leaves. */
export function companionCityFrontageBoundaryParts(p: CompanionCityParcel): CityArchitecturePart[] | null {
  const treatment = companionCityFrontageTreatment(p);
  if (!treatment) return null;
  if (treatment.boundary === "open") return [];
  const result: CityArchitecturePart[] = [], outer = p.width / 2 + p.garden;
  const dx = p.entrance[0] - p.x, dz = p.entrance[2] - p.z;
  const entry = dx * Math.cos(p.yaw) - dz * Math.sin(p.yaw), front = p.depth / 2 - .18;
  const style = treatment.boundary, wallHeight = style === "low-wall" ? .58 : style === "masonry" ? 1.72 : .42;
  const tint = p.variant % 2 ? "#d4cebd" : "#bbbfb0", steel = "#44534c", wood = "#8c7457";
  const add = (role: string, material: CityArchitecturePart["material"], x: number, y: number, z: number,
    w: number, h: number, d: number, colour: string, yaw = 0) => result.push({
    role: `frontage ${role}`, material, x, y, z, w, h, d, colour,
    geometry: "box", shadow: true, ...(yaw ? { rotation: [0, yaw, 0] as const } : {}),
  });
  const run = (cx: number, cz: number, length: number, yaw: number) => {
    if (length <= .3) return;
    const at = (x: number, y: number, w: number, h: number, d: number, material: CityArchitecturePart["material"], colour: string, role: string) =>
      add(role, material, cx + x * Math.cos(yaw), y, cz - x * Math.sin(yaw), w, h, d, colour, yaw);
    at(0, wallHeight / 2, length, wallHeight, .22, "stone", tint, "boundary wall");
    at(0, wallHeight + .045, length, .09, .25, "stone", "#ded7c5", "wall coping");
    if (style === "railings" || style === "slats") {
      const top = style === "slats" ? 1.62 : 1.35;
      for (const y of [wallHeight + .13, top - .06]) at(0, y, length, .055, .07, "metal", steel, "fence rail");
      for (let x = -length / 2 + .13; x < length / 2 - .05; x += style === "slats" ? .22 : .18)
        at(x, (wallHeight + top) / 2, style === "slats" ? .145 : .027, top - wallHeight, .045,
          style === "slats" ? "wood" : "metal", style === "slats" ? wood : steel, "fence upright");
    }
  };
  run((-outer + entry - 2.4) / 2, front, entry - 2.4 + outer, 0);
  run((entry + 2.4 + outer) / 2, front, outer - entry - 2.4, 0);
  for (const side of [-1, 1]) {
    run(side * (outer - .015), 0, p.depth - .6, Math.PI / 2);
    add("gate pier", "stone", entry + side * 2.28, .93, front - .1, .3, 1.86, .34, tint);
    // Folded leaves stay beside the unchanged 4.5 m Home entrance.
    add("folded gate leaf", "metal", entry + side * 2.4, .8, front - 1.18, .055, 1.5, 1.9, steel);
  }
  run(0, -p.depth / 2 + .18, outer * 2 - .3, 0);
  return result;
}

/** Planting and daily-life props remain inside the existing oriented plot, not public walking routes. */
export function companionCityFrontageProps(p: CompanionCityParcel): CityFrontageProp[] {
  const treatment = companionCityFrontageTreatment(p);
  if (!treatment?.landscaped) return [];
  const residentialSpaces = companionCityResidentialFrontageSpaces(p);
  if (residentialSpaces.length) return residentialFrontageProps(residentialSpaces);
  const values: CityFrontageProp[] = [], parking = companionCityParkingParts(p);
  const bays = parking.filter(v => v.role === "parking paved bay");
  const halfDepth = p.depth / 2, strip = Math.min(p.garden - .65, 3.5);
  const add = (kind: CityFrontageProp["kind"], x: number, z: number, width: number, depth: number,
    yaw = 0, scale: readonly [number, number, number] = [1, 1, 1]) => {
    const c = Math.abs(Math.cos(yaw)), s = Math.abs(Math.sin(yaw));
    const rx = (width * c + depth * s) / 2, rz = (width * s + depth * c) / 2;
    if (Math.abs(x) - rx < p.width / 2 + .12 || Math.abs(x) + rx > p.width / 2 + p.garden - .2
      || Math.abs(z) + rz > halfDepth - .55) return;
    if (bays.some(v => Math.abs(x - v.x) < rx + v.w / 2 + .35 && Math.abs(z - v.z) < rz + v.d / 2 + .4)) return;
    if (values.some(v => !["lawn", "paving"].includes(v.kind)
      && Math.hypot(x - v.x, z - v.z) < (kind === "tree" ? 2.2 : 1.55))) return;
    values.push({ kind, x, z, yaw, scale });
  };
  for (const side of [-1, 1]) {
    const x = side * (p.width / 2 + p.garden / 2), back = -halfDepth + 3;
    const car = bays.find(v => Math.sign(v.x) === side), end = car ? car.z - car.d / 2 - .7 : halfDepth - 2;
    const length = end - back;
    if (length > 3) add("lawn", x, (back + end) / 2, strip, length, 0, [strip, 1, length]);
    add("tree", x, -halfDepth + 2.1, 1.8, 1.8);
    const step = p.garden >= 6 ? 3.2 : 3.7;
    let index = 0;
    for (let z = back + 1; z < end - 1; z += step) {
      const n = variation(p.id, `plant-${side}-${index++}`);
      const kind: CityFrontageProp["kind"] = n < .2 ? "croton" : n < .37 ? "agave" : n < .64 ? "shrub" : "flowers";
      add(kind, x + side * (variation(p.id, `plant-jitter-${index}`) - .5) * .24,
        z + (variation(p.id, `plant-depth-${side}-${index}`) - .5) * .65, 1.55, 1.55,
        variation(p.id, `plant-turn-${index}`) * Math.PI * 2);
    }
    // A planted tree replaces one of the old spherical crowns, with a clear forecourt below it.
    const frontZ = halfDepth - 2;
    if (!car) {
      add("planter", x, frontZ, 1.5, 1.2);
      if (variation(p.id, `frontage-seat-${side}`) < .36) add("bench", x, frontZ - 3.2, 1.75, .72, Math.PI / 2);
    }
    if (p.garden >= 6 && variation(p.id, `frontage-bin-${side}`) < .38)
      add("bin", side * (p.width / 2 + .7), halfDepth - 7.5, .58, .68);
    if (p.kind !== "tower" && variation(p.id, `frontage-hose-${side}`) < .3)
      add("hose", side * (p.width / 2 + .62), -halfDepth + 6, .65, .65);
  }
  return values;
}
