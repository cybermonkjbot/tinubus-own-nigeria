import type { CompanionAssetKey } from "./companionAssets";
import type { NavigationObstacle, RoomNavigation, ScenePoint } from "./companionNavigation";
import type { CompanionWaterfrontScenery } from "./companionWaterfront";

export const companionPlaceKinds = ["hospital", "mama-put", "content-house", "skyline", "fine-dining", "nightclub", "station", "soccer-pitch", "arena", "housing",
  "market", "supermarket", "bus-terminal", "fuel-station", "mechanic", "salon", "gym", "cinema", "library", "arts-centre", "campus", "office", "lounge", "pool-courtyard", "civic-interior",
  "water-terminal", "stilt-creek", "surf-island", "timber-atoll", "under-bridge", "race-circuit"] as const;
export type CompanionPlaceKind = typeof companionPlaceKinds[number];
export type CompanionPlaceServiceKind = "food" | "medic" | "driver" | "police" | "mission" | "dance" | "gather" | "capture" | "football" | "arena" | "home-door"
  | "pos" | "hawker" | "tout";
export type CompanionPlacePose = { point: ScenePoint; yaw: number };
export type CompanionPlaceFootprint = NavigationObstacle & { id: string; height: number; y?: number };
export type CompanionPlaceFixtureKind = "counter" | "table" | "bench" | "treatment-bed" | "stage" | "goal" | "cover" | "planter" | "canopy" | "cell-bars" | "skyline"
  | "market-stall" | "shelf" | "fuel-pump" | "tool-cabinet" | "tyre-rack" | "salon-chair" | "mirror" | "exercise-bike" | "cinema-seat" | "screen";
export type CompanionPlaceFixture = {
  id: string; kind: CompanionPlaceFixtureKind; point: ScenePoint; size: ScenePoint; yaw: number;
  material: "plaster" | "wood" | "stone" | "metal" | "linen" | "sage" | "rose";
  collision?: CompanionPlaceFootprint;
};
/** A measured subset of a published room. Transforms stay in runtime metres. */
export type CompanionPlaceAssetPart = {
  id: string; key: CompanionAssetKey; sourceBounds?: CompanionPlaceFootprint;
  npcId?: "mai-suya" | "akara-woman" | "street-tout-shirt" | "street-tout-cap";
  point: ScenePoint; yaw: number; sourceYaw?: number; collision?: CompanionPlaceFootprint;
};
export type CompanionPlaceService = {
  id: string; label: string; kind: CompanionPlaceServiceKind; approach: CompanionPlacePose;
  anchor: CompanionPlacePose; footprint: NavigationObstacle;
  npcId?: string; capacity: number; queue: readonly ScenePoint[];
  /** IDs only. Price, stock and entitlement are always supplied by their owner. */
  offers?: readonly string[]; roomSlotId?: string;
  access?: "policing-owner";
};
export type CompanionPlaceNpc = {
  id: string; role: "vendor" | "medic" | "driver" | "police" | "mission-contact" | "host" | "resident";
  label: string; home: CompanionPlacePose; route: readonly ScenePoint[]; refuge: ScenePoint;
  radius: number; serviceIds: readonly string[]; assetPartId?: string;
};
export type CompanionPlaceEntrance = {
  id: string; label: string; point: ScenePoint; outside: ScenePoint; width: number;
  role: "world" | "recovery" | "detention" | "home-room" | "match-team";
  access: "place-membership" | "recovery-owner" | "policing-owner" | "room-visit-grant" | "joined-match";
};
export type CompanionPlaceZone = {
  id: string; label: string; bounds: NavigationObstacle;
  access: "place-membership" | "policing-owner" | "joined-match";
  activity?: "dance" | "gather" | "capture" | "football" | "arena";
};
export type CompanionHomeRoomSlot = {
  id: string; label: string; approach: ScenePoint;
  /** Opens an existing saved room through its owner. Never copies or replaces it. */
  suggestedLayoutId: "pat-studio-v01" | "pink-bedroom-v02" | "modern-kitchen-v01" | "bathroom-v01";
};
export type CompanionPlaceLayout = {
  id: string; placeId: string; kind: CompanionPlaceKind; label: string;
  seed: string; generatorVersion: "lagos-places-v1"; assetVersion: string;
  navigation: RoomNavigation; fixtures: readonly CompanionPlaceFixture[]; assets: readonly CompanionPlaceAssetPart[];
  entrances: readonly CompanionPlaceEntrance[]; spawns: readonly CompanionPlacePose[];
  recoverySpawns: readonly CompanionPlacePose[]; detentionSpawns: readonly CompanionPlacePose[];
  services: readonly CompanionPlaceService[]; npcs: readonly CompanionPlaceNpc[];
  zones: readonly CompanionPlaceZone[]; homeRooms: readonly CompanionHomeRoomSlot[];
  cameras: { wide: { eye: ScenePoint; target: ScenePoint }; close: { eye: ScenePoint; target: ScenePoint } };
  theme: { background: string; floor: string; wall: string; accent: string; openAir: boolean };
  qualityGaps: readonly string[];
  waterfront?: CompanionWaterfrontScenery;
};
export type CompanionPlaceGeneration = { placeId: string; kind: CompanionPlaceKind; seed: string; generatorVersion?: "lagos-places-v1"; housingOption?: "room" | "compound" | "apartment" | "large-home" };
