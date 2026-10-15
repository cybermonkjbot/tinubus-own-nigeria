import { COMPANION_CITY_VERSION, companionCityParcel } from "./companionCity";
import type { ScenePoint } from "./companionNavigation";

/** A virtual address is independent of editable saved rooms and real-world finances. */
export type CompanionCityResidence = { cityVersion: string; parcelId: string; unit: number };
export const COMPANION_CITY_HOME_ENTER = "city:enter-home";
export const COMPANION_CITY_HOME_EXIT = "city:leave-home";
export function companionCityResidenceEntrance(residence?: CompanionCityResidence | null): ScenePoint | null {
  if (!residence || residence.cityVersion !== COMPANION_CITY_VERSION || !Number.isInteger(residence.unit)) return null;
  const parcel = companionCityParcel(residence.parcelId);
  if (!parcel || residence.unit < 1 || residence.unit > parcel.units) return null;
  return [parcel.entrance[0], parcel.entrance[1], parcel.entrance[2]];
}
export function companionCityResidenceLabel(residence: CompanionCityResidence) {
  const parcel = companionCityParcel(residence.parcelId);
  if (!parcel || !companionCityResidenceEntrance(residence)) return "Home";
  return `${parcel.name}, ${residence.unit}`;
}
