import type { CompanionCityParcel } from "./companionCity";
import { companionCityArchitectureEntrance, companionCityArchitectureParts, type CityArchitecturePart } from "./companionCityArchitectureDetails";
import { companionCityFrontageProps } from "./companionCityFrontages";
import { cityStreetSeed, type CityStreetFixture, type CityStreetRoad, type CityStreetWire } from "./companionCityStreetDetails";
import { cityUtilityPolePorts, cityUtilityPoleProfile, cityUtilityPoleSpan } from "./companionCityUtilityPoles";
import signatures from "../../assets/city/lagos-architecture-delivery-v1.json";
import civicTerrace from "../../assets/venues/civic-terrace-v1.json";

export type CityResidentialUtilityAnchor = { parcelId: string; x: number; y: number; z: number; yaw: number };
type WireBounds = { x: number; z: number; c: number; s: number; halfWidth: number; halfDepth: number; height: number };
const authored = new Set([civicTerrace.parcelId, signatures.exterior.parcelId, ...signatures.buildings.map(p => p.parcelId)]);
const shellRoles = new Set(["left courtyard wing", "right courtyard wing", "rear courtyard wing", "tower podium",
  "villa main", "villa terrace wing", "apartment main", "apartment return", "street building"]);

function heightAt(wire: CityStreetWire, t: number) {
  return wire.from[1] + (wire.to[1] - wire.from[1]) * t - 4 * wire.sag * t * (1 - t);
}

/** Clip against the oriented footprint, then check the cable's lowest height inside it. */
function crossesBounds(wire: CityStreetWire, other: WireBounds): boolean {
  const fromX = wire.from[0] - other.x, fromZ = wire.from[2] - other.z;
  const toX = wire.to[0] - other.x, toZ = wire.to[2] - other.z;
  const ax = fromX * other.c - fromZ * other.s, az = fromX * other.s + fromZ * other.c;
  const bx = toX * other.c - toZ * other.s, bz = toX * other.s + toZ * other.c;
  let enter = 0, exit = 1;
  for (let axis = 0; axis < 2; axis++) {
    const a = axis === 0 ? ax : az, b = axis === 0 ? bx : bz;
    const half = axis === 0 ? other.halfWidth : other.halfDepth, delta = b - a;
    if (Math.abs(delta) < 1e-8) { if (Math.abs(a) > half) return false; continue; }
    const low = (-half - a) / delta, high = (half - a) / delta;
    enter = Math.max(enter, Math.min(low, high)); exit = Math.min(exit, Math.max(low, high));
    if (enter > exit) return false;
  }
  const lowest = wire.sag > 0 ? Math.max(enter, Math.min(exit, .5 - (wire.to[1] - wire.from[1]) / (8 * wire.sag))) : enter;
  return Math.min(heightAt(wire, enter), heightAt(wire, exit), heightAt(wire, lowest)) < other.height;
}

/** Prepared once during scene construction, shared by old and residential service drops. */
export function createCityStreetWireClearance(parcels: readonly CompanionCityParcel[]) {
  const buildings = parcels.map(p => ({ id: p.id, x: p.x, z: p.z, c: Math.cos(p.yaw), s: Math.sin(p.yaw),
    halfWidth: p.width / 2 + p.garden + .2, halfDepth: p.depth / 2 + .2, height: p.floors * 3.3 + 4 }));
  return (wire: CityStreetWire, endpointIds: readonly string[] = []) =>
    buildings.some(other => !endpointIds.includes(other.id) && crossesBounds(wire, other));
}

function overlapsPart(x: number, z: number, part: CityArchitecturePart) {
  const yaw = part.rotation?.[1] ?? 0, c = Math.abs(Math.cos(yaw)), s = Math.abs(Math.sin(yaw));
  return Math.abs(x - part.x) < (part.w * c + part.d * s) / 2 + .28
    && Math.abs(z - part.z) < (part.d * c + part.w * s) / 2 + .28;
}

/** Plot-edge poles and short facade feeds use existing plot reserves, never new navigation obstacles. */
export function createCompanionCityResidentialUtilities(parcels: readonly CompanionCityParcel[],
  roads: readonly CityStreetRoad[], existingFixtures: readonly CityStreetFixture[],
  crossesBuilding = createCityStreetWireClearance(parcels)) {
  const fixtures: CityStreetFixture[] = [], wires: CityStreetWire[] = [], anchors: CityResidentialUtilityAnchor[] = [];
  const servedParcelIds = new Set<string>(), roadsByName = new Map(roads.map(road => [road.name, road.id]));
  const poles = existingFixtures.filter(fixture => fixture.kind === "utility");
  const masses = new Map<string, WireBounds[]>();
  const clear = (wire: CityStreetWire, ids: string[]) => !crossesBuilding(wire, ids)
    && ids.every(id => !(masses.get(id) ?? []).some(box => crossesBounds(wire, box)));
  for (const p of parcels) {
    const roadId = roadsByName.get(p.street);
    if (!roadId || !p.units || p.garden < 3 || authored.has(p.id)) continue;
    const c = Math.cos(p.yaw), s = Math.sin(p.yaw), parts = companionCityArchitectureParts(p);
    const shells = parts.filter(part => shellRoles.has(part.role));
    const world = (x: number, z: number) => ({ x: p.x + x * c + z * s, z: p.z - x * s + z * c });
    masses.set(p.id, shells.map(part => ({ ...world(part.x, part.z), c, s,
      halfWidth: part.w / 2 + .04, halfDepth: part.d / 2 + .04, height: part.y + part.h / 2 + .18 })));
    const props = companionCityFrontageProps(p).filter(prop => prop.kind !== "lawn" && prop.kind !== "paving");
    const entry = companionCityArchitectureEntrance(p).x, firstSide = cityStreetSeed(`${p.id}:utility`) % 2 ? -1 : 1;
    for (const side of [firstSide, -firstSide]) {
      const x = side * (p.width / 2 + p.garden - .75), z = p.depth / 2 - .65;
      if (Math.abs(x - entry) < 3 || parts.some(part =>
        (part.h > .09 && part.y - part.h / 2 < 11 || part.role === "parking paved bay") && overlapsPart(x, z, part))) continue;
      if (props.some(prop => Math.hypot(x - prop.x, z - prop.z) < .34 + .95 * Math.max(prop.scale[0], prop.scale[2]))) continue;
      const position = world(x, z);
      if (existingFixtures.some(f => f.kind !== "wall-sign" && Math.hypot(f.x - position.x, f.z - position.z) < 1.15)) continue;
      const fixture: CityStreetFixture = { id: `residential:utility:${p.id}`, roadId, distance: 0, ...position,
        yaw: p.yaw, kind: "utility", style: "short-arm", side: 1, height: 10.4 + p.variant % 3 * .12,
        variant: p.variant, reservedParcelId: p.id,
        utility: cityUtilityPoleProfile(cityStreetSeed(`${p.id}:pole`), p.district === "palm-courts" || p.district === "south-market"
          ? "dense" : ["skyline", "lagoon-gardens", "garden-quarter"].includes(p.district) ? "tidy" : "mixed") };
      // Try existing nearby poles first. Reuse is allowed only if the facade feed remains clear.
      const candidates = poles.filter(pole => pole.roadId === roadId && Math.hypot(pole.x - position.x, pole.z - position.z) < 18)
        .sort((a, b) => Math.hypot(a.x - position.x, a.z - position.z) - Math.hypot(b.x - position.x, b.z - position.z));
      candidates.push(fixture);
      let connected = false;
      for (const pole of candidates) {
        for (const shell of shells.filter(part => part.y + part.h / 2 >= 6.6)
          .sort((a, b) => side * (b.x + side * b.w / 2 - a.x - side * a.w / 2))) {
          const localX = shell.x + side * (shell.w / 2 + .18);
          const localZ = Math.max(shell.z - shell.d / 2 + .7, Math.min(z - 1.65, shell.z + shell.d / 2 - .7));
          const anchor: CityResidentialUtilityAnchor = { parcelId: p.id, ...world(localX, localZ), y: 6.18, yaw: p.yaw + side * Math.PI / 2 };
          const distance = Math.hypot(pole.x - anchor.x, pole.z - anchor.z);
          if (distance > 32) continue;
          const drop: CityStreetWire = { id: `residential:drop:${p.id}`, roadId,
            from: cityUtilityPolePorts(pole).phases[3], to: [anchor.x, anchor.y, anchor.z],
            sag: Math.min(.55, distance * .035), strands: 1, diameter: .035 };
          const ids = [p.id, ...(pole.reservedParcelId ? [pole.reservedParcelId] : [])];
          if (!clear(drop, ids)) continue;
          if (pole === fixture) { fixtures.push(fixture); poles.push(fixture); }
          wires.push(drop); anchors.push(anchor); servedParcelIds.add(p.id); connected = true; break;
        }
        if (connected) break;
      }
      if (connected) break;
    }
  }
  // Link neighbouring new poles only along nearly straight, same-facing frontages.
  // Other plots retain conservative full-reserve exclusion; endpoint shells are checked separately.
  const linkedTargets = new Set<string>();
  for (const a of fixtures) {
    const c = Math.cos(a.yaw), s = Math.sin(a.yaw);
    const neighbours = fixtures.filter(b => {
      const dx = b.x - a.x, dz = b.z - a.z, distance = Math.hypot(dx, dz);
      return b.roadId === a.roadId && !linkedTargets.has(b.id) && Math.cos(a.yaw - b.yaw) > .97
        && distance >= 12 && distance <= 68 && dx * c - dz * s > 0 && Math.abs(dx * s + dz * c) < 5;
    }).sort((b, d) => Math.hypot(b.x - a.x, b.z - a.z) - Math.hypot(d.x - a.x, d.z - a.z));
    for (const b of neighbours) {
      const span = cityUtilityPoleSpan(a, b, `residential:span:${a.id}:${b.id}`);
      if (!span.every(wire => clear(wire, [a.reservedParcelId!, b.reservedParcelId!]))) continue;
      wires.push(...span); linkedTargets.add(b.id); break;
    }
  }
  return { fixtures, wires, anchors, servedParcelIds };
}
