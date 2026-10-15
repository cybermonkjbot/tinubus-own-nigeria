import { companionCityGeoPoint, companionCityPlan as plan, companionCityStreetDensity } from "./companionCity";
import { cityMiniParkPoint } from "./companionCityMiniParks";
import type { CompanionPosition } from "./companionCall";
import { companionPlaceExpansionDestinations } from "./companionPlacesExpansion";
import { companionWaterfrontPlaces } from "./companionWaterfront";
import { companionCityResidenceEntrance, companionCityResidenceLabel, type CompanionCityResidence } from "./companionCityHousing";
import type { CompanionWorldFriendResidence, CompanionWorldPlayer } from "./companionWorld";
import { companionCityParkLake, companionCityParkPaths, companionCityParkRemovedRoads } from "./companionCityParks";
import { companionCityTheatre, theatreEllipse } from "./companionCityTheatre";
import { companionCityRefinery } from "./companionCityRefinery";
import { companionCityNeighbourhoodSurfaces } from "./companionCityNeighbourhoods";

type Bounds = { minX: number; maxX: number; minZ: number; maxZ: number };
const points = (vertices: readonly (readonly number[])[]) => vertices.map(point => point.join(",")).join(" ");
const bounds = (vertices: readonly (readonly number[])[], margin = 0): Bounds => ({
  minX: Math.min(...vertices.map(point => point[0])) - margin,
  maxX: Math.max(...vertices.map(point => point[0])) + margin,
  minZ: Math.min(...vertices.map(point => point[1])) - margin,
  maxZ: Math.max(...vertices.map(point => point[1])) + margin,
});
const overlaps = (a: Bounds, b: Bounds) => a.minX <= b.maxX && a.maxX >= b.minX && a.minZ <= b.maxZ && a.maxZ >= b.minZ;

// Existing v03 asset footprints, source [x,y] converted to runtime [x,-y].
// art/rooms/town-square-v03/author_town_square.py owns these physical streets.
const retainedRoads = [
  { id: "retained-main-street", name: "Town Square", kind: "street", width: 8, pavement: 2.5, points: [[-10, -50], [-10, 34.5]] },
  ...[-29, 21].map(z => ({ id: `retained-cross-street:${z}`, name: "Town Square", kind: "street", width: 6, pavement: 1.6, points: [[-38, z], [38, z]] })),
];

/** Immutable drawing data from the same sampled plan as navigation and the city renderer. */
export const companionCityMapGeometry = {
  water: points(plan.lagoon.waterOutline),
  banks: [points(plan.lagoon.southBank), points(plan.lagoon.northBank)],
  beaches: plan.beaches.map(beach => ({ ...beach, points: points(beach.points), bounds: bounds(beach.points) })),
  greens: plan.greenSpaces.map(space => ({ ...space, points: points(space.points), bounds: bounds(space.points) })),
  // Retained connections are routing-only links, not physical new roads through Town Square.
  roads: [...plan.roads.filter(road => road.kind !== "retained"), ...retainedRoads].map(road => ({
    ...road, vertices: road.points, points: points(road.points), bounds: bounds(road.points, road.width / 2 + road.pavement),
  })),
  parcels: plan.parcels.map(parcel => {
    const c = Math.abs(Math.cos(parcel.yaw)), s = Math.abs(Math.sin(parcel.yaw));
    const halfX = (parcel.width * c + parcel.depth * s) / 2;
    const halfZ = (parcel.width * s + parcel.depth * c) / 2;
    return { ...parcel, bounds: { minX: parcel.x - halfX, maxX: parcel.x + halfX, minZ: parcel.z - halfZ, maxZ: parcel.z + halfZ } };
  }),
};

const parkMapPaths = companionCityParkPaths.map(road=>({...road,vertices:road.points,points:points(road.points),bounds:bounds(road.points,road.width/2)}));
const parkMapLakes = [{...companionCityParkLake,points:points(companionCityParkLake.points),bounds:bounds(companionCityParkLake.points)}];
const parkMapRoads = [...companionCityMapGeometry.roads.filter(r=>!companionCityParkRemovedRoads.has(r.id)),...parkMapPaths];
const theatreMapRoads = [...companionCityTheatre.roads, ...companionCityTheatre.paths].map(road =>
  ({ ...road, vertices: road.points, points: points(road.points), bounds: bounds(road.points, road.width / 2 + road.pavement) }));
const theatreMapLakes = companionCityTheatre.lakes.map(lake => ({ ...lake, points: points(lake.points), bounds: bounds(lake.points) }));
const theatreMapBuildings = [{ id: "national-theatre", points: points(theatreEllipse(...companionCityTheatre.building.podium as [number, number])),
  bounds: bounds(theatreEllipse(...companionCityTheatre.building.podium as [number, number])) }];
const theatreMapGreens = [{ id: "theatre-gardens", kind: "garden", points: points(companionCityTheatre.grounds), bounds: bounds(companionCityTheatre.grounds) }];
const refineryMapRoads = companionCityRefinery.roads.map(road => ({ ...road, vertices: road.points,
  points: points(road.points), bounds: bounds(road.points, road.width / 2 + road.pavement) }));
const refinerySite = companionCityRefinery.site;
const refineryMapBuildings = [{ id: companionCityRefinery.id, points: points([[refinerySite.minX,refinerySite.minZ],
  [refinerySite.maxX,refinerySite.minZ],[refinerySite.maxX,refinerySite.maxZ],[refinerySite.minX,refinerySite.maxZ]]),bounds:refinerySite }];
let neighbourhoodMapSurfaces: { id: string; kind: string; points: string; colour: string; bounds: Bounds }[] | undefined;
export function companionCityMapGeometryIn(view: Bounds, parkPaths = false, theatreDistrict = false, refineryDistrict = false, streetDensity = false) {
  const visible = <T extends { bounds: Bounds }>(shapes: readonly T[]) => shapes.filter(shape => overlaps(shape.bounds, view));
  neighbourhoodMapSurfaces ??= companionCityNeighbourhoodSurfaces().filter(surface => surface.kind !== "border").map(surface => ({
    id: surface.id, kind: surface.kind, points: points(surface.points), colour: surface.colour, bounds: bounds(surface.points),
  }));
  return {
    neighbourhoods: visible(neighbourhoodMapSurfaces),
    roads: visible([...(parkPaths ? parkMapRoads : companionCityMapGeometry.roads), ...(theatreDistrict ? theatreMapRoads : []),
      ...(refineryDistrict ? refineryMapRoads : [])]), beaches: visible(companionCityMapGeometry.beaches),
    greens: visible([...companionCityMapGeometry.greens, ...(theatreDistrict ? theatreMapGreens : [])]), parcels: visible(companionCityMapGeometry.parcels),
    lakes: visible([...(parkPaths ? parkMapLakes : []), ...(theatreDistrict ? theatreMapLakes : [])]),
    buildings: visible([...(theatreDistrict ? theatreMapBuildings : []),...(refineryDistrict ? refineryMapBuildings : [])]),
    miniParks: streetDensity ? companionCityStreetDensity().parks.filter(park => overlaps(bounds(
      [[-11, -9], [11, -9], [11, 9], [-11, 9]].map(([x, z]) => {
        const point = cityMiniParkPoint(park, x, z); return [point[0], point[2]];
      }),
    ), view)) : [],
  };
}
export type CompanionCityMapGeometry = ReturnType<typeof companionCityMapGeometryIn>;

function distanceToRoad(x: number, z: number, vertices: readonly (readonly number[])[]) {
  let nearest = Infinity;
  for (let index = 1; index < vertices.length; index++) {
    const a = vertices[index - 1], b = vertices[index], dx = b[0] - a[0], dz = b[1] - a[1];
    const lengthSquared = dx * dx + dz * dz;
    const t = lengthSquared ? Math.max(0, Math.min(1, ((x - a[0]) * dx + (z - a[1]) * dz) / lengthSquared)) : 0;
    nearest = Math.min(nearest, Math.hypot(x - a[0] - dx * t, z - a[1] - dz * t));
  }
  return nearest;
}

/** Human-readable named roads beat generated path IDs. No geocoding or navigation command. */
export function companionCityMapLocation(x: number, z: number, theatreDistrict = false, refineryDistrict = false) {
  if(refineryDistrict && x>1450 && z>1150 && z<1900){
    const road=companionCityRefinery.roads.reduce((a,b)=>distanceToRoad(x,z,a.points)<distanceToRoad(x,z,b.points)?a:b);
    return{street:road.name,district:companionCityRefinery.district};
  }
  if (theatreDistrict && x < -1510 && z > 880 && z < 1740) {
    const road = companionCityTheatre.roads.reduce((a, b) => distanceToRoad(x, z, a.points) < distanceToRoad(x, z, b.points) ? a : b);
    return { street: road.name, district: companionCityTheatre.district };
  }
  const district = plan.districts.reduce((closest, candidate) =>
    Math.hypot(x - candidate.labelAt[0], z - candidate.labelAt[1]) < Math.hypot(x - closest.labelAt[0], z - closest.labelAt[1]) ? candidate : closest);
  let street = "", distance = Infinity;
  for (const road of companionCityMapGeometry.roads) {
    if (road.kind === "path" || road.kind === "sand-path") continue;
    const next = distanceToRoad(x, z, road.vertices);
    if (next < distance) { distance = next; street = road.name; }
  }
  if (Math.abs(x) < 50 && Math.abs(z) < 60) street = "Town Square";
  else if (distance > 180) street = district.label;
  return { street: street || district.label, district: district.label };
}

export const companionCityMapLandmarks = [
  { id: "town-square", name: "Town Square", x: 0, z: 0, number: 0 },
  ...plan.pointsOfInterest.map((place, index) => ({ id: place.id, name: place.name, x: place.x, z: place.z, number: index + 1 })),
  ...companionWaterfrontPlaces.filter(place => place.level !== "abutment").map((place, index) => ({ id: place.id, name: place.label,
    x: place.mapPoint[0], z: place.mapPoint[1], number: plan.pointsOfInterest.length + index + 1 })),
];

/** +X is east and -Z is north, matching companionCityGeoPoint. Yaw zero faces +Z, south. */
export function companionCityMapFacing(yaw: number) {
  const headings = ["south", "southeast", "east", "northeast", "north", "northwest", "west", "southwest"];
  return headings[(Math.round(yaw / (Math.PI / 4)) + 8) % 8];
}

export function companionCityMapProject(position: Pick<CompanionPosition, "x" | "z">, center: Pick<CompanionPosition, "x" | "z">,
  span: number, width: number, height: number) {
  return { x: width / 2 + (position.x - center.x) * width / span, y: height / 2 + (position.z - center.z) * width / span };
}

/** Pin an offscreen Home to the inset edge on its true bearing, including diagonal bearings. */
export function companionCityMapEdge(point: { x: number; y: number }, width: number, height: number, inset = 15) {
  const dx = point.x - width / 2, dy = point.y - height / 2;
  const halfWidth = Math.max(1, width / 2 - inset), halfHeight = Math.max(1, height / 2 - inset);
  const amount = Math.max(1, Math.abs(dx) / halfWidth, Math.abs(dy) / halfHeight);
  return { x: width / 2 + dx / amount, y: height / 2 + dy / amount,
    offscreen: amount > 1, angle: Math.atan2(dy, dx) * 180 / Math.PI };
}

/** Invert companionCityGeoPoint: +X is east and -Z is north. */
export function companionCityPointFromGeo(geo: { latitude: number; longitude: number }) {
  return {
    x: (geo.longitude - plan.origin.longitude) * 111_320 * Math.cos(plan.origin.latitude * Math.PI / 180),
    z: (plan.origin.latitude - geo.latitude) * 111_320,
  };
}

export type CompanionCityInterestingPlace = {
  id: string;
  name: string;
  kind: "landmark" | "venue" | "dining" | "nightlife" | "activity" | "services" | "shopping" | "culture" | "entertainment" | "fitness" | "transport";
  x: number;
  z: number;
  anchor: { latitude: number; longitude: number };
  number?: number;
};

const classicGamePlaces: readonly { id: string; name: string; kind: CompanionCityInterestingPlace["kind"]; anchor: { latitude: number; longitude: number } }[] = [
  { id: "mama-put", name: "Mama put", kind: "dining", anchor: { latitude: 6.4524, longitude: 3.4101 } },
  { id: "nightclub", name: "Nightclub", kind: "nightlife", anchor: { latitude: 6.4488, longitude: 3.4074 } },
  { id: "content-house", name: "Content house", kind: "activity", anchor: { latitude: 6.4506, longitude: 3.4122 } },
  { id: "soccer-pitch", name: "Football pitch", kind: "fitness", anchor: { latitude: 6.4558, longitude: 3.4074 } },
  { id: "arena", name: "Arena", kind: "activity", anchor: { latitude: 6.4564, longitude: 3.4122 } },
  { id: "skyline", name: "Skyline restaurant", kind: "dining", anchor: { latitude: 6.4475, longitude: 3.4122 } },
  { id: "fine-dining", name: "Fine dining", kind: "dining", anchor: { latitude: 6.4468, longitude: 3.4101 } },
  { id: "hospital", name: "Hospital", kind: "services", anchor: { latitude: 6.4546, longitude: 3.4114 } },
  { id: "station", name: "Police station", kind: "services", anchor: { latitude: 6.4536, longitude: 3.4074 } },
];

/** Comprehensive interesting places across the companion world (landmarks, venues, activities, dining). */
export const companionCityInterestingPlaces: readonly CompanionCityInterestingPlace[] = [
  { id: "town-square", name: "Town Square", kind: "landmark", x: 0, z: 0, anchor: { latitude: 6.4506, longitude: 3.4074 }, number: 1 },
  ...plan.pointsOfInterest.map((poi, idx) => ({
    id: poi.id, name: poi.name, kind: "landmark" as const,
    x: poi.x, z: poi.z, anchor: companionCityGeoPoint(poi.x, poi.z),
    number: idx + 2,
  })),
  ...classicGamePlaces.map((place, idx) => {
    const coords = companionCityPointFromGeo(place.anchor);
    return {
      id: place.id, name: place.name, kind: place.kind,
      x: coords.x, z: coords.z, anchor: place.anchor,
      number: plan.pointsOfInterest.length + idx + 2,
    };
  }),
  ...companionPlaceExpansionDestinations.map((dest, idx) => {
    const kindMap: Record<string, CompanionCityInterestingPlace["kind"]> = {
      market: "shopping", supermarket: "shopping", "bus-terminal": "transport",
      "fuel-station": "services", mechanic: "services", salon: "services",
      gym: "fitness", cinema: "entertainment", library: "culture",
      "arts-centre": "culture", campus: "culture", office: "services", lounge: "nightlife",
    };
    return {
      id: dest.id, name: dest.label, kind: kindMap[dest.kind] ?? "venue",
      x: dest.city.entrance[0], z: dest.city.entrance[2], anchor: dest.anchor,
      number: plan.pointsOfInterest.length + classicGamePlaces.length + idx + 2,
    };
  }),
  ...companionWaterfrontPlaces.filter(place => place.level !== "abutment").map(place => ({ id: place.id, name: place.label,
    kind: place.kind === "water-terminal" ? "transport" as const : "venue" as const,
    x: place.mapPoint[0], z: place.mapPoint[1], anchor: place.anchor })),
];

export const companionCityTheatreMapPlace: CompanionCityInterestingPlace & { number: number } = {
  id: "national-theatre", name: companionCityTheatre.name, kind: "culture",
  x: companionCityTheatre.arrival[0], z: companionCityTheatre.arrival[1],
  anchor: companionCityGeoPoint(companionCityTheatre.arrival[0], companionCityTheatre.arrival[1]),
  number: Math.max(...companionCityInterestingPlaces.map(place => place.number ?? 0)) + 1,
};
export const companionCityRefineryMapPlace: CompanionCityInterestingPlace & { number: number } = {
  id: companionCityRefinery.id,name:companionCityRefinery.name,kind:"landmark",
  x:companionCityRefinery.arrival[0],z:companionCityRefinery.arrival[2],
  anchor:companionCityGeoPoint(companionCityRefinery.arrival[0],companionCityRefinery.arrival[2]),
  number:companionCityTheatreMapPlace.number+1,
};

export function companionCityMiniParkMapPlaces(): readonly (CompanionCityInterestingPlace & { number: number })[] {
  return companionCityStreetDensity().parks.map((park, index) => ({
    id: park.id, name: park.name, kind: "transport", x: park.x, z: park.z,
    anchor: companionCityGeoPoint(park.x, park.z), number: companionCityRefineryMapPlace.number + index + 1,
  }));
}

export type CompanionFriendResidencePlacement = {
  userId: string;
  name: string;
  label: string;
  address: string;
  residence: CompanionCityResidence;
  entrance: readonly [number, number, number];
  x: number;
  z: number;
  anchor: { latitude: number; longitude: number };
};

/** Resolve and deduplicate friend residences in the city. */
export function resolveCompanionFriendResidences({
  friends = [],
  peers = [],
  participants = [],
  excludeUserId,
}: {
  friends?: readonly CompanionWorldFriendResidence[];
  peers?: readonly CompanionWorldPlayer[];
  participants?: readonly { userId: string; displayName?: string }[];
  excludeUserId?: string;
}): CompanionFriendResidencePlacement[] {
  const nameMap = new Map<string, string>();
  for (const part of participants) {
    if (part.displayName) nameMap.set(part.userId, part.displayName);
  }
  const byUser = new Map<string, { name: string; residence: CompanionCityResidence }>();

  // Add recorded friend residences first
  for (const item of friends) {
    if (!item.userId || item.userId === excludeUserId) continue;
    const name = nameMap.get(item.userId) || item.name || "Friend";
    byUser.set(item.userId, { name, residence: item.residence });
  }

  // Add peers in call/room who have a residence
  for (const peer of peers) {
    if (!peer.userId || peer.userId === excludeUserId || !peer.residence) continue;
    if (!byUser.has(peer.userId)) {
      const name = nameMap.get(peer.userId) || peer.characterId || "Friend";
      byUser.set(peer.userId, { name, residence: peer.residence });
    }
  }

  const results: CompanionFriendResidencePlacement[] = [];
  for (const [userId, { name, residence }] of byUser.entries()) {
    const entrance = companionCityResidenceEntrance(residence);
    if (!entrance) continue;
    const address = companionCityResidenceLabel(residence);
    const label = `${name}'s Home`;
    const x = entrance[0], z = entrance[2];
    const anchor = companionCityGeoPoint(x, z);
    results.push({ userId, name, label, address, residence, entrance, x, z, anchor });
  }
  return results;
}
