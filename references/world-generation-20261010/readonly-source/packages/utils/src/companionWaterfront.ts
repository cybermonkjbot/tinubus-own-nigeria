import plan from "../../assets/city/lagos-city-v3.json";
import { companionRouteLength, type CompanionCorridorRoute, type CompanionGeoPoint } from "./companionJourney";
import type { ScenePoint } from "./companionNavigation";

export const COMPANION_WATERFRONT_VERSION = "lagos-waterfront-v2";
export const companionWaterfrontKinds = ["water-terminal", "stilt-creek", "surf-island", "timber-atoll", "under-bridge"] as const;
export type CompanionWaterfrontKind = typeof companionWaterfrontKinds[number];
export type CompanionWaterZone = "ocean" | "lagoon" | "creek";
export type CompanionBoatKind = "canoe" | "speedboat" | "sand-barge" | "yacht" | "jet-ski";
export type CompanionWaterfrontPropKind = "boat" | "hut" | "stilt-house" | "palm" | "mangrove" | "beach-club" | "lounger"
  | "quad-bike" | "log-raft" | "sawmill" | "wood-pile" | "generator" | "jetty" | "bollard" | "fish-rack" | "net-rack"
  | "bridge-deck" | "bridge-pier" | "market-stall" | "phone-stall" | "food-grill" | "mechanic-bay" | "tyre-stack"
  | "danfo" | "cart" | "sleeping-platform" | "laundry" | "stair" | "plaza" | "tetrapod" | "rock-groyne"
  | "container-ship" | "hyacinth" | "debris" | "oil-sheen" | "reeds" | "sign" | "drain" | "prayer-space";
export type CompanionWaterfrontProp = { id: string; kind: CompanionWaterfrontPropKind; point: ScenePoint; yaw: number;
  size: ScenePoint; variant: number; boat?: CompanionBoatKind;
  boatModel?: import("./companionBoatAssets").CompanionBoatModelId; text?: string; serviceId?: string };
export type CompanionWaterfrontSurface = { id: string; kind: "sand" | "mud" | "concrete" | "wood";
  minX: number; maxX: number; minZ: number; maxZ: number };
export type CompanionWaterfrontScenery = { version: "lagos-waterfront-v1" | typeof COMPANION_WATERFRONT_VERSION; zone: CompanionWaterZone;
  level: "ground" | "abutment" | "pier"; props: readonly CompanionWaterfrontProp[]; surfaces: readonly CompanionWaterfrontSurface[] };
export const companionBoatModels: Record<CompanionBoatKind, { label: string; length: number; beam: number; seats: number; speed: number; draft: number }> = {
  canoe: { label: "Paddle canoe", length: 5.6, beam: .95, seats: 2, speed: 2.2, draft: .18 },
  speedboat: { label: "Speedboat", length: 9.8, beam: 2.6, seats: 20, speed: 12, draft: .65 },
  "sand-barge": { label: "Sand barge", length: 22, beam: 7, seats: 0, speed: 1.8, draft: 1.3 },
  yacht: { label: "Yacht", length: 15, beam: 4.1, seats: 12, speed: 10, draft: 1.1 },
  "jet-ski": { label: "Jet ski", length: 3.2, beam: 1.15, seats: 2, speed: 15, draft: .35 },
};
const bank = (points: readonly (readonly number[])[], x: number) => {
  for (let i = 1; i < points.length; i++) if (x <= points[i][0]) {
    const a = points[i - 1], b = points[i];
    return a[1] + (b[1] - a[1]) * Math.max(0, Math.min(1, (x - a[0]) / (b[0] - a[0])));
  }
  return points[points.length - 1][1];
};
const geo = (x: number, z: number): CompanionGeoPoint => ({ latitude: plan.origin.latitude - z / 111_320,
  longitude: plan.origin.longitude + x / (111_320 * Math.cos(plan.origin.latitude * Math.PI / 180)) });
export type CompanionWaterfrontPlace = { id: string; label: string; kind: CompanionWaterfrontKind; seed: string;
  placement: "proposed"; anchor: CompanionGeoPoint; waterZone: CompanionWaterZone; boatOnly: boolean;
  mapPoint: readonly [number, number]; cityEntrance?: ScenePoint; level?: "ground" | "abutment" | "pier" };
function place(id: string, label: string, kind: CompanionWaterfrontKind, x: number, z: number,
  waterZone: CompanionWaterZone, boatOnly = false, cityEntrance?: ScenePoint,
  level?: CompanionWaterfrontPlace["level"]): CompanionWaterfrontPlace {
  return { id, label, kind, seed: `${id}:lagos-waterfront-v1`, placement: "proposed",
    anchor: geo(x, z), mapPoint: [x, z], waterZone, boatOnly, ...(cityEntrance ? { cityEntrance } : {}), ...(level ? { level } : {}) };
}
// Authored game placements. Keep real-world reference names separate from surveyed-location claims.
const south = (x: number): ScenePoint => [x, 0, bank(plan.lagoon.southBank, x) - 14];
const north = (x: number): ScenePoint => [x, 0, bank(plan.lagoon.northBank, x) + 16];
export const companionWaterfrontPlaces: readonly CompanionWaterfrontPlace[] = [
  place("cms-jetty", "CMS jetty", "water-terminal", -780, south(-780)[2], "lagoon", false, south(-780)),
  place("five-cowries-terminal", "Five Cowries terminal", "water-terminal", 680, south(680)[2], "lagoon", false, south(680)),
  place("ikorodu-jetty", "Ikorodu jetty", "water-terminal", 1200, north(1200)[2], "lagoon", false, north(1200)),
  place("ikoyi-marina", "Ikoyi marina", "water-terminal", 900, north(900)[2], "lagoon", false, north(900)),
  place("makoko-creeks", "Makoko creeks", "stilt-creek", -1530, 560, "creek", true),
  place("tarkwa-bay", "Tarkwa Bay", "surf-island", -1220, 420, "ocean", true),
  place("ilashe", "Ilashe", "surf-island", 1450, 400, "ocean", true),
  place("okobaba-timber", "Okobaba timber atoll", "timber-atoll", -900, 490, "lagoon", true),
  place("obalende-underbridge", "Obalende under-bridge", "under-bridge", -480, 28, "lagoon", false, [-480, 0, 28], "ground"),
  place("ojuelegba-underbridge", "Ojuelegba under-bridge", "under-bridge", 480, 28, "lagoon", false, [480, 0, 28], "ground"),
  place("carter-pier", "Carter Bridge pier", "under-bridge", -480, 270, "lagoon", true, undefined, "pier"),
  place("obalende-abutment", "Obalende upper abutment", "under-bridge", -480, 28, "lagoon", false, undefined, "abutment"),
];
export const companionWaterfrontPlace = (id: string) => companionWaterfrontPlaces.find(p => p.id === id);
export const isCompanionWaterfrontKind = (kind: string): kind is CompanionWaterfrontKind => companionWaterfrontKinds.some(value => value === kind);
export const companionWaterfrontEnterService = (id: string) => `waterfront:enter:${id}`;
export const COMPANION_WATERFRONT_EXIT = "waterfront:leave";
export const companionWaterfrontLevelService = (id: string) => `waterfront:level:${id}`;
export const companionWaterfrontLevelTarget = (fromId: string, serviceId: string) => {
  const toId = fromId === "obalende-underbridge" ? "obalende-abutment"
    : fromId === "obalende-abutment" ? "obalende-underbridge" : null;
  return toId && serviceId === companionWaterfrontLevelService(toId) ? companionWaterfrontPlace(toId) : null;
};
export const companionWaterfrontEntry = (serviceId: string) => companionWaterfrontPlaces.find(p => p.cityEntrance
  && companionWaterfrontEnterService(p.id) === serviceId);

/** Only registered water contacts have crossing routes. Road vehicles cannot substitute for boats. */
export function companionWaterfrontRoute(fromId: string, toId: string): { route: CompanionCorridorRoute; boat: "canoe" | "speedboat"; durationMs: number } | null {
  const from = companionWaterfrontPlace(fromId), to = companionWaterfrontPlace(toId);
  if (!from || !to || from.id === to.id || from.kind === "under-bridge" && from.level !== "pier"
    || to.kind === "under-bridge" && to.level !== "pier") return null;
  const creek = from.waterZone === "creek" || to.waterZone === "creek";
  // Transfer at the timber landing before entering the shallow canoe alleys.
  if (creek && from.id !== "okobaba-timber" && to.id !== "okobaba-timber") return null;
  const boat = creek ? "canoe" : "speedboat";
  const a = from.mapPoint, b = to.mapPoint;
  const middle = (a[0] + b[0]) / 2;
  const lane = (bank(plan.lagoon.southBank, middle) + bank(plan.lagoon.northBank, middle)) / 2;
  const points = [from.anchor, geo(a[0], lane), geo(b[0], lane), to.anchor];
  const lengthMetres = companionRouteLength(points);
  return { boat, route: { id: `${fromId}:${toId}`, version: "lagos-corridors-v1", placement: "proposed", points, lengthMetres },
    durationMs: Math.max(8_000, Math.ceil(lengthMetres / companionBoatModels[boat].speed * 1000)) };
}

export function companionWaterfrontDestinations(fromId: string) {
  const from = companionWaterfrontPlace(fromId);
  return companionWaterfrontPlaces.filter(to => to.level !== "abutment" && to.id !== fromId && (from?.boatOnly
    ? !!companionWaterfrontRoute(fromId, to.id) : !to.boatOnly || !!companionWaterfrontRoute(fromId, to.id)));
}
