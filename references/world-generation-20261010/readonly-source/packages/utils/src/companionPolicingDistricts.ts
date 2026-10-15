import city from "../../assets/city/lagos-city-v3.json";
import { companionWorldPlace } from "./companionWorld";
import type { CompanionGeoPoint } from "./companionJourney";

export type CompanionEnforcementDistrict = "town-centre" | "palm-courts" | "garden-quarter" | "south-market" | "marina" | "skyline" | "lagoon-gardens";
type DistrictProfile = Readonly<{ policeDay: number; policeNight: number; trafficDay: number; ndlea: number; nightlifeEfcc: number }>;
/** Chosen game percentages, not measured street activity. Keep quiet districts quiet. */
export const companionEnforcementDistrictProfiles: Readonly<Record<CompanionEnforcementDistrict, DistrictProfile>> = {
  "town-centre": { policeDay: 5, policeNight: 9, trafficDay: 3, ndlea: 1, nightlifeEfcc: 1 },
  "palm-courts": { policeDay: 3, policeNight: 5, trafficDay: 1, ndlea: 0, nightlifeEfcc: 1 },
  "garden-quarter": { policeDay: 2, policeNight: 4, trafficDay: 1, ndlea: 0, nightlifeEfcc: 1 },
  "south-market": { policeDay: 7, policeNight: 11, trafficDay: 5, ndlea: 2, nightlifeEfcc: 1 },
  marina: { policeDay: 4, policeNight: 7, trafficDay: 3, ndlea: 1, nightlifeEfcc: 2 },
  skyline: { policeDay: 3, policeNight: 6, trafficDay: 2, ndlea: 0, nightlifeEfcc: 2 },
  "lagoon-gardens": { policeDay: 2, policeNight: 4, trafficDay: 1, ndlea: 0, nightlifeEfcc: 1 },
};
export function isCompanionEnforcementDistrict(id: unknown): id is CompanionEnforcementDistrict {
  return typeof id === "string" && Object.prototype.hasOwnProperty.call(companionEnforcementDistrictProfiles, id);
}
/** Same nearest district anchor as the city map, without its drawing dependencies. */
export function companionEnforcementDistrictAt(x: number, z: number): CompanionEnforcementDistrict {
  if (!Number.isFinite(x) || !Number.isFinite(z)) return "town-centre";
  const district = city.districts.reduce((nearest, candidate) =>
    Math.hypot(x - candidate.labelAt[0], z - candidate.labelAt[1])
      < Math.hypot(x - nearest.labelAt[0], z - nearest.labelAt[1]) ? candidate : nearest);
  return isCompanionEnforcementDistrict(district.id) ? district.id : "town-centre";
}
export function companionEnforcementDistrictForPoint(point: CompanionGeoPoint) {
  return companionEnforcementDistrictAt(
    (point.longitude - city.origin.longitude) * 111_320 * Math.cos(city.origin.latitude * Math.PI / 180),
    (city.origin.latitude - point.latitude) * 111_320);
}
export function companionEnforcementPlaceDistrict(placeId: string): CompanionEnforcementDistrict | null {
  // Home parcels belong to their saved residence, never a generic Home anchor.
  if (placeId === "own-home" || placeId.startsWith("roadside:")) return null;
  const place = companionWorldPlace(placeId);
  if (!place) return null;
  return place.city ? companionEnforcementDistrictAt(place.city.entrance[0], place.city.entrance[2])
    : companionEnforcementDistrictForPoint(place.anchor);
}
export const companionEnforcementNightlifeRoute = (fromPlaceId: string, toPlaceId: string) =>
  [fromPlaceId, toPlaceId].some(id => ["nightclub", "dance-again", "civic-terrace-lounge"].includes(id));
