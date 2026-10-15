import { COMPANION_CITY_VERSION, companionCityGeoPoint, companionCityParcel, isCompanionCity } from "./companionCity";
import type { RoomNavigation, ScenePoint } from "./companionNavigation";
import type { CompanionPlaceKind, CompanionPlaceService } from "./companionPlaces";
import { companionCourierCatalog } from "./companionCourierCatalog";
import { racingRules } from "./companionRacing";

export type CompanionPlaceExpansionDestination = Readonly<{
  id: string; label: string; kind: CompanionPlaceKind; seed: string;
  anchor: Readonly<{ latitude: number; longitude: number }>; placement: "proposed";
  city: Readonly<{ version: string; parcelId: string; entrance: ScenePoint }>;
}>;

function destination(id: string, label: string, kind: CompanionPlaceKind, parcelId: string, entranceOffset = 0): CompanionPlaceExpansionDestination {
  const parcel = companionCityParcel(parcelId);
  // Only existing non-residential plots. A venue never takes a saved Home unit.
  if (!parcel || parcel.kind !== "shop" || parcel.units !== 0) throw new Error(`Missing public venue plot ${parcelId}.`);
  const entrance: ScenePoint = [parcel.entrance[0] + Math.cos(parcel.yaw) * entranceOffset, parcel.entrance[1],
    parcel.entrance[2] - Math.sin(parcel.yaw) * entranceOffset];
  return Object.freeze({ id, label, kind, seed: `${id}-v1`, placement: "proposed",
    anchor: Object.freeze(companionCityGeoPoint(entrance[0], entrance[2])),
    city: Object.freeze({ version: COMPANION_CITY_VERSION, parcelId, entrance: Object.freeze(entrance) }) });
}

/** Additive registration batch for the world owner. Local city entrances are
 * authored placements, not surveyed businesses or provider-verified routes.
 * Nothing here admits a player, provisions a service or replaces a frozen instance.
 */
export const companionPlaceExpansionDestinations: readonly CompanionPlaceExpansionDestination[] = Object.freeze([
  Object.freeze({ id: racingRules.placeId, label: "Lagos Circuit", kind: "race-circuit", seed: racingRules.version, placement: "proposed",
    anchor: Object.freeze(companionCityGeoPoint(racingRules.mapX, racingRules.mapZ + 94)),
    city: Object.freeze({ version: COMPANION_CITY_VERSION, parcelId: "lagos-circuit-site-v1",
      entrance: Object.freeze([racingRules.mapX, 0, racingRules.mapZ + 94] as ScenePoint) }) }),
  destination("south-market", "South Market", "market", "unity-avenue-36"),
  destination("corner-supermarket", "Corner supermarket", "supermarket", "market-road-5"),
  destination("danfo-terminal", "Danfo terminal", "bus-terminal", "unity-avenue-39"),
  destination("unity-fuel-station", "Unity fuel station", "fuel-station", "unity-avenue-29"),
  destination("mechanic-yard", "Mechanic yard", "mechanic", "unity-avenue-32"),
  destination("salon", "Salon", "salon", "market-road-2"),
  destination("community-gym", "Community gym", "gym", "cross-town-road-11"),
  destination("cinema", "Cinema", "cinema", "cross-town-road-18"),
  destination("city-library", "City library", "library", "garden-avenue-13"),
  destination("arts-centre", "Arts centre", "arts-centre", "garden-avenue-11"),
  destination("college-courtyard", "College courtyard", "campus", "marina-drive-30"),
  destination("co-working-studio", "Co-working studio", "office", "marina-drive-10"),
  destination("dance-again", "Dance Again", "lounge", "market-road-6"),
  destination("pool-courtyard", "Pool courtyard", "pool-courtyard", "civic-street-2"),
  destination("civic-reception", "Civic reception", "civic-interior", "civic-street-2", -3.3),
  destination("civic-terrace-lounge", "Civic terrace lounge", "civic-interior", "civic-street-2", 3.3),
]);

export const COMPANION_CITY_PLACE_EXIT = "city:leave-place";
export const companionCityPlaceEnterService = (placeId: string) => `city:enter-place:${placeId}`;
export function companionCityPlaceEntry(serviceId: string) {
  return companionPlaceExpansionDestinations.find(place => companionCityPlaceEnterService(place.id) === serviceId) ?? null;
}
/** Both frozen v3 collision records share the same authored parcel entrances. */
export function companionCityPlaceDestinations(navigation?: RoomNavigation) {
  return isCompanionCity(navigation)
    ? companionPlaceExpansionDestinations.filter(place => place.city.version === COMPANION_CITY_VERSION) : [];
}

/** Only provisioned parcel contacts join the existing travel and carried-food controls. */
export function companionExpansionServiceSupported(placeId: string, service: CompanionPlaceService) {
  return !companionPlaceExpansionDestinations.some(place => place.id === placeId)
    || service.kind === "driver" || service.kind === "food" && !!service.offers?.includes("eat-carried-food")
    || service.kind === "mission" && companionCourierCatalog.some(job => [job.employer, job.destination].some(contact =>
      contact.placeId === placeId && contact.serviceId === service.id && contact.npcId === service.npcId));
}
