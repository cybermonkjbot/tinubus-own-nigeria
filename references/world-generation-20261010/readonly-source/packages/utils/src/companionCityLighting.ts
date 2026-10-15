import type { CompanionCityParcel } from "./companionCity";
import type { CityStreetFixture } from "./companionCityStreetDetails";
import { cityUtilityPolePoint } from "./companionCityUtilityPoles";
import { companionCityArchitectureEntrance } from "./companionCityArchitectureDetails";
import { companionCityParkingTreatment } from "./companionCityParking";
import { companionCityFrontageBoundaryParts, companionCityFrontageTreatment, companionCityResidentialFrontageSpaces } from "./companionCityFrontages";
import signatures from "../../assets/city/lagos-architecture-delivery-v1.json";
import civicTerrace from "../../assets/venues/civic-terrace-v1.json";

export type CityLightEmitter = { x: number; y: number; z: number; intensity: number; reach: number };
export type CityResidentialLight = CityLightEmitter & {
  id: string; parcelId: string; yaw: number; kind: "canopy" | "sconce" | "wall-cap";
};
const authored = new Set([civicTerrace.parcelId, signatures.exterior.parcelId, ...signatures.buildings.map(p => p.parcelId)]);

/** Match the actual lantern, utility arm, quay arm or paired street-lamp heads. */
export function companionCityStreetLightEmitters(fixture: CityStreetFixture): CityLightEmitter[] {
  if (fixture.kind === "wall-sign") return [];
  if (fixture.kind === "utility" && fixture.utility) {
    const point = cityUtilityPolePoint(fixture, 0, fixture.height - 2.9, -fixture.side * .95);
    return [{ x: point[0], y: point[1], z: point[2], intensity: 160, reach: 28 }];
  }
  if (fixture.style === "lantern" || fixture.kind === "compound-lamp")
    return [{ x: fixture.x, y: fixture.height + .03, z: fixture.z,
      intensity: fixture.kind === "compound-lamp" ? 42 : 90, reach: fixture.kind === "compound-lamp" ? 16 : 24 }];
  const utility = fixture.kind === "utility", reach = fixture.style === "quay" ? 1.15 : utility ? .72 : 1.5;
  return (fixture.style === "twin-arm" ? [-1, 1] : [-fixture.side]).map(side => ({
    x: fixture.x + side * reach * Math.sin(fixture.yaw),
    y: fixture.height - (utility ? 1.1 : 0) - .3,
    z: fixture.z + side * reach * Math.cos(fixture.yaw), intensity: 160, reach: 28,
  }));
}

/** Fixtures mount on existing canopies, courtyard pilasters and boundary masonry. No new ground obstacles. */
export function companionCityResidentialLights(p: CompanionCityParcel): CityResidentialLight[] {
  if (!p.units || authored.has(p.id)) return [];
  const result: CityResidentialLight[] = [], c = Math.cos(p.yaw), s = Math.sin(p.yaw);
  const add = (id: string, kind: CityResidentialLight["kind"], x: number, y: number, z: number,
    intensity: number, reach: number, yaw = 0) => result.push({
    id: `${p.id}:light:${id}`, parcelId: p.id, kind, x: p.x + x * c + z * s, y,
    z: p.z - x * s + z * c, yaw: p.yaw + yaw, intensity, reach,
  });
  const entry = companionCityArchitectureEntrance(p).x;
  const front = p.depth / 2 - (companionCityParkingTreatment(p)?.forecourtDepth ?? 0);
  for (const side of [-1, 1]) {
    // The canopy underside is at 2.92 m. The housing retains more than 2.7 m headroom.
    add(`entrance-${side}`, "canopy", entry + side * 1.12, 2.87, front - .47, 48, 15);
    if (p.style === "courtyard")
      add(`court-${side}`, "sconce", side * (p.width * .2 + .035), 2.65, -p.depth * .06,
        52, 17, -side * Math.PI / 2);
  }
  const boundary = companionCityFrontageBoundaryParts(p) ?? [];
  for (const [index, pier] of boundary.filter(part => part.role === "frontage gate pier").entries())
    add(`gate-${index}`, "wall-cap", pier.x, pier.y + pier.h / 2 + .2, pier.z, 28, 12);

  const treatment = companionCityFrontageTreatment(p);
  if (treatment?.boundary === "low-wall" || treatment?.boundary === "masonry") {
    for (const space of companionCityResidentialFrontageSpaces(p)) {
      if (space.kind === "garden" || Math.abs(space.x) <= p.width / 2) continue;
      const wall = boundary.find(part => part.role === "frontage boundary wall"
        && Math.abs(part.x) > p.width / 2 && Math.sign(part.x) === space.side && part.rotation?.[1] === Math.PI / 2);
      if (wall) add(`${space.kind}-${space.side}`, "wall-cap", wall.x,
        wall.y + wall.h / 2 + .09 + .2, space.z, 22, 10);
    }
  }
  return result;
}

/** Put the practical outside its opaque housing, on the emitting face. */
export function companionCityResidentialLightEmitter(light: CityResidentialLight): CityLightEmitter {
  const outward = light.kind === "sconce" ? .16 : 0;
  return { x: light.x + Math.sin(light.yaw) * outward,
    y: light.y - (light.kind === "canopy" ? .11 : 0),
    z: light.z + Math.cos(light.yaw) * outward, intensity: light.intensity, reach: light.reach };
}
