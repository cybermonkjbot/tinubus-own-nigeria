import plan from "../../assets/city/lagos-city-v3.json";
import type { CompanionCityParcel } from "./companionCity";

// These existing non-residential shops neighbour the mapped football destination.
// They do not reserve a field or grant street-level entry into the match instance.
const shops: Record<string, { name: string; kit: string }> = {
  "theatre-street-2": { name: "PITCHSIDE SPORTS", kit: "#39795f" },
  "theatre-street-4": { name: "BOOTS & KITS", kit: "#c4a04d" },
};

export function companionCityFootballFrontage(parcel: CompanionCityParcel) {
  return parcel.units === 0 && parcel.style === "corner-shop" ? shops[parcel.id] : undefined;
}

const frontages = plan.parcels.filter(parcel => companionCityFootballFrontage(parcel));

/** Existing lamp columns only. No new street obstacle or destination is introduced. */
export function companionCityFootballStreetBanner(x: number, z: number) {
  return frontages.some(parcel => Math.hypot(x - parcel.x, z - parcel.z) < 110);
}
