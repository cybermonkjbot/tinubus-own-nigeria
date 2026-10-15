import { companionAssetEntries } from "./companionAssets";
import { planRoomRoute, routeSegmentClear, type NavigationObstacle, type ScenePoint } from "./companionNavigation";
import { patStudioLayout } from "./companionRoomLayouts";
import type { CompanionPlaceAssetPart, CompanionPlaceFixtureKind, CompanionPlaceFootprint, CompanionPlaceGeneration, CompanionPlaceKind, CompanionPlaceLayout, CompanionPlaceNpc, CompanionPlacePose, CompanionPlaceService, CompanionPlaceServiceKind } from "./companionPlaces";
import { generateCompanionPlaceExpansionLayout } from "./companionPlacesExpansionLayouts";
import loungeDesign from "../../assets/venues/dance-again-v1.json";
import { generatePoolCourtyardLayout } from "./companionPlacesPoolCourtyard";
import { generateCivicInteriorLayout } from "./companionPlacesCivicInteriors";
import { generateWaterfrontLayout } from "./companionWaterfrontLayouts";
import { racingPlaceLayout } from "./companionRacing";

export const COMPANION_PLACE_GENERATOR_VERSION = "lagos-places-v1" as const;
export const companionPlaceNames: Record<CompanionPlaceKind, string> = {
  "race-circuit": "Lagos Circuit",
  hospital: "Hospital", "mama-put": "Mama put", "content-house": "Content house", skyline: "Skyline restaurant",
  "fine-dining": "Fine dining", nightclub: "Nightclub", station: "Police station", "soccer-pitch": "Football pitch", arena: "Arena", housing: "Home",
  market: "South Market", supermarket: "Corner supermarket", "bus-terminal": "Danfo terminal", "fuel-station": "Unity fuel station",
  mechanic: "Mechanic yard", salon: "Salon", gym: "Community gym", cinema: "Cinema", library: "City library",
  "arts-centre": "Arts centre", campus: "College courtyard", office: "Co-working studio", lounge: "Dance Again", "pool-courtyard": "Pool courtyard",
  "civic-interior": "Civic Street interiors",
  "water-terminal": "Water terminal", "stilt-creek": "Makoko creeks", "surf-island": "Surf island",
  "timber-atoll": "Timber atoll", "under-bridge": "Under-bridge",
};
const p = (x: number, z: number, y = 0): ScenePoint => [x, y, z];
const pose = (x: number, z: number, yaw = Math.PI): CompanionPlacePose => ({ point: p(x, z), yaw });
const footprint = (id: string, x: number, z: number, width: number, depth: number, height = 1): CompanionPlaceFootprint =>
  ({ id, minX: x - width / 2, maxX: x + width / 2, minZ: z - depth / 2, maxZ: z + depth / 2, height });
function seedHash(seed: string) {
  let hash = 2166136261;
  for (let i = 0; i < seed.length; i++) hash = Math.imul(hash ^ seed.charCodeAt(i), 16777619);
  return hash >>> 0;
}
type Builder = ReturnType<typeof createBuilder>;
function createBuilder(input: CompanionPlaceGeneration, width: number, depth: number, openAir = false) {
  const layout: CompanionPlaceLayout = {
    id: `${input.placeId}:${COMPANION_PLACE_GENERATOR_VERSION}:${input.kind}:${input.housingOption ?? "default"}:${encodeURIComponent(input.seed)}`,
    placeId: input.placeId, kind: input.kind, label: companionPlaceNames[input.kind], seed: input.seed,
    generatorVersion: COMPANION_PLACE_GENERATOR_VERSION,
    assetVersion: "",
    navigation: { revision: "", radius: .24, floorY: 0, gridSize: Math.max(.24, Math.max(width, depth) / 76),
      bounds: { minX: -width / 2, maxX: width / 2, minZ: -depth / 2, maxZ: depth / 2 }, obstacles: [] },
    fixtures: [], assets: [], entrances: [], spawns: [], recoverySpawns: [], detentionSpawns: [], services: [], npcs: [], zones: [], homeRooms: [],
    cameras: { wide: { eye: p(1.4, depth / 2 - 1, 2.4), target: p(0, -depth / 4, 1) },
      close: { eye: p(1.5, depth / 2 - 1.2, 1.8), target: p(0, depth / 2 - 3, .9) } },
    theme: { background: openAir ? "#b9ccdc" : "#e6e3dc", floor: "#d5ccbd", wall: "#ece7dd", accent: "#88967c", openAir },
    qualityGaps: ["New layouts have not been inspected in native or desktop rendering.", "Existing room parts retain their published finish; new architectural fixtures need matched player-height visual review."],
  };
  const fixtures: CompanionPlaceLayout["fixtures"][number][] = [], assets: CompanionPlaceAssetPart[] = [], services: CompanionPlaceService[] = [], npcs: CompanionPlaceNpc[] = [];
  const entrances: CompanionPlaceLayout["entrances"][number][] = [], spawns: CompanionPlacePose[] = [];
  const zones: CompanionPlaceLayout["zones"][number][] = [];
  const add = (kind: CompanionPlaceFixtureKind, id: string, x: number, z: number, w: number, d: number, h: number,
    material: CompanionPlaceLayout["fixtures"][number]["material"] = "wood", collides = true, y = 0) => {
    fixtures.push({ id, kind, point: p(x, z, y), size: [w, h, d], yaw: 0, material,
      collision: collides ? footprint(id, x, z, w, d, h) : undefined });
  };
  // The primary entrance is always an open three-metre passage. No invisible
  // wall across it, and no random furnishing in the arrival corridor.
  const wallHeight = openAir ? 1.15 : 3.4;
  add("cover", "wall-back", 0, -depth / 2 + .1, width, .2, wallHeight, "plaster");
  for (const x of [-width / 2 + .1, width / 2 - .1]) add("cover", x < 0 ? "wall-left" : "wall-right", x, 0, .2, depth, wallHeight, "plaster");
  for (const sign of [-1, 1]) add("cover", sign < 0 ? "wall-front-left" : "wall-front-right", sign * (width / 4 + .75), depth / 2 - .1, width / 2 - 1.5, .2, wallHeight, "plaster");
  entrances.push({ id: "world-entry", label: "Street", point: p(0, depth / 2 - .65), outside: p(0, depth / 2 + 1), width: 3, role: "world", access: "place-membership" });
  spawns.push(pose(-.55, depth / 2 - 2), pose(.55, depth / 2 - 2), pose(-.55, depth / 2 - 3.1), pose(.55, depth / 2 - 3.1));
  function service(kind: CompanionPlaceServiceKind, id: string, label: string, x: number, z: number, nx: number, nz: number,
    role: CompanionPlaceNpc["role"], offers?: readonly string[], capacity = 1) {
    const npcId = `${id}-npc`;
    const entry: CompanionPlaceService = { id, kind, label, approach: pose(x, z), anchor: pose(nx, nz, Math.atan2(x - nx, z - nz)),
      footprint: footprint(`${id}-service`, nx, nz, .7, .7), npcId, capacity,
      queue: [p(x, z), p(x, z + .9), p(x, z + 1.8)], offers };
    services.push(entry);
    // Seats are object interactions, not invisible resident NPCs.
    if (role === "resident") delete entry.npcId;
    else npcs.push({ id: npcId, role, label, home: entry.anchor, route: [entry.anchor.point], refuge: p(0, depth / 2 - 4.2), radius: .3, serviceIds: [id] });
    return entry;
  }
  function part(key: CompanionPlaceAssetPart["key"], id: string, source: CompanionPlaceFootprint, x: number, z: number, yaw = 0) {
    const width = source.maxX - source.minX, depth = source.maxZ - source.minZ;
    // Quarter-turns only. These conservative footprints match extracted parts
    // without stretching room furniture to fill a different building.
    const turned = Math.abs(Math.sin(yaw)) > .5;
    assets.push({ key, id, sourceBounds: source, point: p(x, z), yaw,
      collision: footprint(id, x, z, turned ? depth : width, turned ? width : depth, source.height) });
  }
  function finish() {
    for (const npc of npcs) {
      if (npc.assetPartId || npc.role === "resident") continue;
      const id = `${npc.id}-art`;
      npc.assetPartId = id;
      assets.push({ id, key: "bestieTownSquareNpcs", npcId: "street-tout-shirt", point: npc.home.point, yaw: npc.home.yaw, sourceYaw: -Math.PI / 2 });
    }
    layout.fixtures = fixtures; layout.assets = assets; layout.services = services; layout.npcs = npcs; layout.entrances = entrances; layout.spawns = spawns; layout.zones = zones;
    if (npcs.some(npc => npc.role !== "resident")) layout.qualityGaps = [...layout.qualityGaps,
      "Staff reuse an existing fictional civilian rig without new clothing. Dedicated medic, police, driver and host art remains unapproved and unreviewed."];
    // Seed variation changes the resolved arrangement and every corresponding
    // collision, entrance, service, spawn and NPC target together.
    if (seedHash(input.seed) % 2) mirrorLayout(layout);
    const keys = [...new Set(assets.map(asset => asset.key))].sort();
    layout.assetVersion = keys.map(key => `${key}:${companionAssetEntries[key].sha256}`).join("|") || "geometry-v1";
    layout.navigation = { ...layout.navigation, revision: `${layout.id}:${layout.assetVersion}`,
      obstacles: [...layout.fixtures.flatMap(f => f.collision ? [f.collision] : []), ...layout.assets.flatMap(a => a.collision ? [a.collision] : [])] };
    const issues = inspectCompanionPlaceLayout(layout);
    if (issues.length) throw new Error(`Invalid ${input.kind} layout: ${issues.join("; ")}`);
    return layout;
  }
  return { layout, add, service, part, fixtures, assets, services, npcs, entrances, spawns, zones, finish };
}
function roomPart(layout: typeof patStudioLayout, id: string): CompanionPlaceFootprint {
  const obstacle = layout.obstacles.find(item => item.id === id);
  if (!obstacle) throw new Error(`Missing authored room part ${id}`);
  return { ...footprint(id, obstacle.x, obstacle.z, obstacle.width, obstacle.depth, obstacle.height), y: obstacle.y ?? 0 };
}
function streetDriver(b: Builder, z: number) {
  b.service("driver", "driver", "Board a ride", -2.4, z, -3.4, z, "driver");
}
function hospital(input: CompanionPlaceGeneration) {
  const b = createBuilder(input, 18, 22);
  b.layout.theme.accent = "#90a9a6";
  b.add("counter", "reception-desk", -4.6, 2, 3.4, .9, .95, "stone");
  b.service("medic", "reception", "Recovery desk", -4.6, 3.3, -4.6, 1.1, "medic");
  for (const [i, z] of [-6, -1, 4].entries()) {
    b.add("treatment-bed", `recovery-bed-${i}`, 6, z, 1.2, 2.3, .7, "linen");
    b.add("counter", `bedside-${i}`, 7.25, z - .65, .5, .5, .72, "stone");
  }
  b.service("medic", "medic", "Get treatment", 4.1, -3.4, 6, -3.4, "medic");
  b.layout.recoverySpawns = [pose(4.1, 1.3), pose(4.1, 6.3), pose(3.1, -5.5)];
  b.entrances.push({ id: "hospital-recovery", label: "Recovery ward", point: p(4.1, 1.3), outside: p(4.1, 1.3), width: 1.4, role: "recovery", access: "recovery-owner" });
  for (const z of [-5, -.5]) b.part("bestiePatStudio", `waiting-chair-${z}`, roomPart(patStudioLayout, "chair"), -6.4, z);
  b.add("bench", "waiting-bench", -4.4, -6.3, 2.6, .7, .5);
  b.zones.push({ id: "recovery-ward", label: "Recovery ward", bounds: footprint("ward", 5, -.5, 6, 16), access: "place-membership" });
  streetDriver(b, 8.5);
  b.npcs.find(npc => npc.id === "medic-npc")!.route = [p(6, -3.4), p(4.1, -3.4), p(4.1, -7.9), p(4.1, 6.3), p(4.1, -3.4), p(6, -3.4)];
  return b.finish();
}
function mamaPut(input: CompanionPlaceGeneration) {
  const b = createBuilder(input, 18, 20, true);
  b.layout.theme.floor = "#b9ad97"; b.layout.theme.wall = "#d6c5ad";
  b.add("counter", "food-counter", -4.7, -4.3, 5.6, 1.1, .92, "stone");
  b.add("canopy", "kitchen-canopy", -4.7, -4.7, 6.4, 3, 3.2, "metal", false);
  for (const x of [-7.85, -1.55]) for (const z of [-6.15, -3.25]) b.add("cover", `canopy-post-${x}-${z}`, x, z, .1, .1, 3.2, "metal");
  b.service("food", "mama-vendor", "Food counter", -4.7, -2.8, -4.7, -5.4, "vendor", ["jollof-rice", "beans", "akara", "suya"]);
  // Published photographed food and separate NPC rig retain the source scale.
  b.assets.push({ id: "mama-vendor-art", key: "bestieTownSquareNpcs", npcId: "akara-woman", point: p(-4.7, -6.8), yaw: 0, sourceYaw: -Math.PI / 2,
    collision: footprint("mama-vendor-art", -4.7, -6.8, 2.4, 2.4, 1.8) });
  b.npcs.find(npc => npc.id === "mama-vendor-npc")!.assetPartId = "mama-vendor-art";
  // Staff stand outside the equipment collision. The authored vendor remains
  // seated at its own asset origin, independently of ambient navigation.
  b.npcs.find(npc => npc.id === "mama-vendor-npc")!.home = pose(-6.5, -6.8, 0);
  b.npcs.find(npc => npc.id === "mama-vendor-npc")!.route = [p(-6.5, -6.8)];
  for (const [i, z] of [-5.6, -.4, 4.5].entries()) {
    b.part("bestieKitchen", `shared-table-${i}`, footprint("island", 0, 0, 2.1, 1.05, .95), 4.4, z);
    for (const sign of [-1, 1]) b.add("bench", `meal-bench-${i}-${sign}`, 4.4, z + sign * 1.05, 2.5, .5, .48);
    b.service("food", `eat-${i}`, "Eat", 2.6, z, 3.05, z, "resident", ["eat-carried-food"], 4);
  }
  b.add("planter", "mama-tree", -7.4, 2.4, 1.1, 1.1, 1.9, "sage");
  b.service("mission", "delivery-contact", "Deliver an order", -4.3, 3.4, -5.5, 3.4, "mission-contact", ["food-delivery"]);
  streetDriver(b, 7.5);
  b.layout.qualityGaps = [...b.layout.qualityGaps, "Jollof and beans have menu contracts but no dedicated authored meal art. The existing akara rig represents the food vendor; new staff uniforms are not authored."];
  return b.finish();
}
function contentHouse(input: CompanionPlaceGeneration) {
  const b = createBuilder(input, 26, 24);
  b.layout.theme.accent = "#c19288";
  // Separate larger floor plan, published Studio furniture at its own scale.
  // Never scale or replace the saved Studio enclosure.
  for (const [i, z] of [-7.2, -.5].entries()) {
    b.part("bestiePatStudio", `studio-console-${i}`, roomPart(patStudioLayout, "console"), -8.6, z);
    b.part("bestiePatStudio", `studio-chair-${i}`, roomPart(patStudioLayout, "chair"), -6.4, z + 1.5);
  }
  b.part("bestiePatStudio", "creator-workstation", roomPart(patStudioLayout, "workstation"), 8.6, -7.1);
  b.part("bestiePatStudio", "creator-chair", roomPart(patStudioLayout, "chair"), 8.6, -4.1, Math.PI);
  b.add("stage", "studio-stage", 3.8, -.8, 5.2, 3.8, .02, "wood", false);
  b.add("planter", "studio-plant-west", -10.8, 4, 1, 1, 2, "sage");
  b.add("planter", "studio-plant-east", 10.8, 4, 1, 1, 2, "sage");
  b.service("gather", "host", "Join the gathering", -3, 3.1, -4.4, 3.1, "host", ["group-pose", "conversation-circle"], 12);
  b.service("capture", "camera", "Make content", 3.8, 3.1, 6.2, 3.1, "host", ["photo", "clip"], 8);
  b.service("mission", "creator-contact", "Creator work", 6.3, -5.3, 7.3, -5.3, "mission-contact", ["creator-brief"]);
  b.zones.push({ id: "gathering", label: "Gathering", bounds: footprint("gathering", -3.1, -1.1, 5.4, 4.5), access: "place-membership", activity: "gather" },
    { id: "capture-stage", label: "Studio stage", bounds: footprint("stage", 3.8, -.8, 5.2, 3.8), access: "place-membership", activity: "capture" });
  b.spawns.splice(0, b.spawns.length, ...[-2.8, -1.4, 0, 1.4, 2.8].flatMap(x => [pose(x, 9), pose(x, 7.8)]));
  streetDriver(b, 9.5);
  b.layout.qualityGaps = [...b.layout.qualityGaps, "Larger Studio architecture is newly assembled. Capture, consent and publishing use the content-house owner, not automatic recording."];
  return b.finish();
}
function restaurant(input: CompanionPlaceGeneration, elevated: boolean) {
  const b = createBuilder(input, elevated ? 22 : 20, 24, elevated);
  b.layout.theme = elevated ? { background: "#c0cedb", floor: "#c9c0b2", wall: "#ded5c6", accent: "#829484", openAir: true }
    : { background: "#ded7cd", floor: "#d8d0c6", wall: "#efe8df", accent: "#b58981", openAir: false };
  b.add("counter", "restaurant-service", -6.6, 4.8, 3, .9, .95, "stone");
  b.service("food", "restaurant-host", "Choose a meal", -6.6, 6.1, -6.6, 3.8, "vendor", ["restaurant-meal", "water"]);
  for (const x of [-5, 5]) for (const z of [-6.5, -1.5]) {
    const id = `dining-${x}-${z}`;
    b.add("table", id, x, z, 1.9, 1.05, .76, "stone");
    b.part("bestiePatStudio", `${id}-chair`, roomPart(patStudioLayout, "chair"), x + 1.65, z, -Math.PI / 2);
    b.part("bestiePatStudio", `${id}-chair-left`, roomPart(patStudioLayout, "chair"), x - 1.65, z, Math.PI / 2);
    b.service("food", `${id}-meal`, "Eat", x, z + 1.1, x, z, "resident", ["eat-carried-food"], 2);
  }
  if (elevated) {
    b.add("skyline", "lagoon-view", 0, -13.8, 23, .5, 13, "plaster", false);
    b.layout.qualityGaps = [...b.layout.qualityGaps, "Skyline silhouettes are virtual scenery, not a surveyed Lagos view. Geographic placement and the elevated exterior need approval."];
  } else b.add("planter", "dining-plant", 7.8, 6.1, 1, 1, 2, "sage");
  streetDriver(b, 9.5);
  return b.finish();
}
function lounge(input: CompanionPlaceGeneration) {
  const b = createBuilder(input, loungeDesign.size[0], loungeDesign.size[2]);
  b.layout.theme = { ...loungeDesign.theme };
  const cameraPoint = (value: readonly number[]): ScenePoint => [value[0], value[1], value[2]];
  b.layout.cameras = {
    wide: { eye: cameraPoint(loungeDesign.cameras.wide.eye), target: cameraPoint(loungeDesign.cameras.wide.target) },
    close: { eye: cameraPoint(loungeDesign.cameras.close.eye), target: cameraPoint(loungeDesign.cameras.close.target) },
  };
  for (const side of [-1, 1]) for (const [index, z] of loungeDesign.boothRows.entries()) {
    const [w, h, d] = loungeDesign.boothSize;
    b.add("bench", `lounge-booth-${side}-${index}`, side * loungeDesign.boothX, z, d, w, h, "rose");
    const booth = b.fixtures[b.fixtures.length - 1];
    booth.size = [w, h, d]; booth.yaw = side < 0 ? Math.PI / 2 : -Math.PI / 2;
    const [tw, th, td] = loungeDesign.tableSize;
    b.add("table", `lounge-table-${side}-${index}`, side * loungeDesign.tableX, z, tw, td, th, "wood");
    const [cw, ch, cd] = loungeDesign.chairSize;
    b.add("bench", `lounge-chair-${side}-${index}`, side * loungeDesign.chairX, z, cd, cw, ch, "metal");
    const chair = b.fixtures[b.fixtures.length - 1];
    chair.size = [cw, ch, cd]; chair.yaw = side < 0 ? -Math.PI / 2 : Math.PI / 2;
    // Wall decor and ledge plants stay above the booths, outside the aisle.
    b.add("cover", `lounge-gallery-${side}-${index}`, side * 4.28, z, .12, 2.5, 1.25, "wood", false, 1.55);
    const gallery = b.fixtures[b.fixtures.length - 1];
    gallery.size = [2.5, 1.25, .12]; gallery.yaw = side < 0 ? Math.PI / 2 : -Math.PI / 2;
    b.add("planter", `lounge-foliage-${side}-${index}`, side * 4.02, z + 1.2, .38, .38, .95, "sage", false, 1.15);
  }
  for (const [id, item] of [["lounge-bar", loungeDesign.bar], ["lounge-backbar", loungeDesign.backbar]] as const) {
    b.add("counter", id, item.point[0], item.point[2], item.size[0], item.size[2], item.size[1], "wood");
  }
  b.add("cover", "lounge-neon", loungeDesign.neon.point[0], loungeDesign.neon.point[2],
    loungeDesign.neon.size[0], loungeDesign.neon.size[2], loungeDesign.neon.size[1], "metal", false, loungeDesign.neon.point[1]);
  b.add("canopy", "lounge-ceiling", 0, 0, 8.6, 15.6, .12, "metal", false, 3.22);
  b.zones.push({ id: "lounge-aisle", label: "Lounge", bounds: footprint("lounge-aisle", 0, -.2, 2, 12.4), access: "place-membership" });
  // No paid counter, dance game or invented staff service is attached to this room.
  return b.finish();
}
function nightclub(input: CompanionPlaceGeneration) {
  const b = createBuilder(input, 24, 26);
  b.layout.theme = { background: "#171923", floor: "#302d35", wall: "#393640", accent: "#b39aa9", openAir: false };
  // The DJ footprint is closed equipment, not a climbable platform. All
  // player circulation stays at floorY, including the nearly flush dance floor.
  b.add("stage", "dj-stage", 0, -10, 6.8, 2.4, .035, "wood");
  b.add("counter", "dj-booth", 0, -10, 3.4, .88, 1.24, "metal");
  b.add("stage", "dance-floor", 0, -1, 9.2, 9.2, .035, "wood", false);
  for (const sign of [-1, 1]) {
    b.add("cover", `club-speaker-${sign}`, sign * 5.1, -9.6, .95, 1.25, 2.35, "metal");
    b.add("bench", `club-lounge-${sign}`, sign * 9.1, -1.5, 1.25, 5.8, 1.02, "rose");
    // Local sofa fronts face inward. Quarter-turn collision bounds were added
    // above in world metres; mirroring reflects both the position and yaw.
    const sofa = b.fixtures[b.fixtures.length - 1];
    sofa.size = [5.8, 1.02, 1.25]; sofa.yaw = sign < 0 ? Math.PI / 2 : -Math.PI / 2;
    b.add("table", `club-table-${sign}`, sign * 7, -1.6, 1.15, 1.15, .5, "stone");
    for (const [i, z] of [-8, -3, 2, 7].entries()) {
      b.add("cover", `club-side-panel-${sign}-${i}`, sign * 11.64, z, .18, 3.4, 2.7, "wood", true, .25);
      const panel = b.fixtures[b.fixtures.length - 1];
      panel.size = [3.4, 2.7, .18]; panel.yaw = sign < 0 ? Math.PI / 2 : -Math.PI / 2;
    }
  }
  for (const [i, x] of [-8.7, -5.8, -2.9, 0, 2.9, 5.8, 8.7].entries())
    b.add("cover", `club-back-panel-${i}`, x, -12.55, 2.52, .22, 2.8, "wood", true, .22);
  b.add("canopy", "club-ceiling-frame", 0, -2.6, 11.3, 15.3, .14, "metal", false, 3.16);
  b.add("bench", "club-lounge-gather", -7.7, 6.5, 5, 1.2, 1.02, "rose");
  b.fixtures[b.fixtures.length - 1].yaw = Math.PI;
  b.add("table", "club-table-gather", -7.7, 4.25, 1.25, 1.15, .48, "stone");
  b.add("counter", "club-bar", 7.9, 6.5, 6, 1.15, 1.12, "stone");
  b.add("counter", "club-backbar", 7.9, 9.55, 6, .72, 2.65, "wood");
  b.fixtures[b.fixtures.length - 1].yaw = Math.PI;
  for (const [i, x] of [9.3, 10.35].entries())
    b.add("bench", `club-bar-stool-${i}`, x, 4.7, .48, .48, .78, "rose");
  const dance = b.service("dance", "dance-host", "Join the dance", 0, 2.2, 2, 2.4, "host", ["rhythm-circle"], 12);
  dance.queue = [p(0, 2.2), p(-1, 2.2), p(-2, 2.2)];
  const gather = b.service("gather", "club-host", "Meet the group", -6, 5.15, -5.4, 4.8, "host", ["conversation-circle"], 8);
  gather.queue = [p(-6, 5.15), p(-5.4, 3.9), p(-5.4, 2.9)];
  const water = b.service("food", "club-water", "Food and water", 6.1, 4.8, 7.4, 7.85, "vendor", ["water", "club-meal"]);
  water.queue = [p(6.1, 4.8), p(7.1, 4.8), p(8.1, 4.8)];
  b.zones.push(
    { id: "dance-floor", label: "Dance floor", bounds: footprint("dance-floor", 0, -1, 9.2, 9.2), access: "place-membership", activity: "dance" },
    { id: "club-lounge", label: "Lounge", bounds: footprint("club-lounge", -7.6, 4.5, 5.6, 4.8), access: "place-membership", activity: "gather" },
  );
  b.layout.cameras = { wide: { eye: p(1.4, 10.5, 2.65), target: p(0, -4, 1.15) }, close: { eye: p(1.6, 4.8, 1.7), target: p(0, -.8, .9) } };
  streetDriver(b, 10.5);
  b.layout.qualityGaps = [...b.layout.qualityGaps, "DJ and staff lack dedicated authored rigs. There is no licensed music stream; rhythm uses the owning game's shared clock."];
  return b.finish();
}
function station(input: CompanionPlaceGeneration) {
  const b = createBuilder(input, 22, 24);
  b.layout.theme.accent = "#7b8792";
  b.add("counter", "station-desk", -6, 2.2, 4, 1, 1, "stone");
  b.service("police", "desk-officer", "Station desk", -6, 3.7, -6, 1.1, "police", ["encounter-status", "release-options"]);
  for (const [i, z] of [-6.6, -.6].entries()) {
    b.add("cell-bars", `cell-front-${i}`, 3.7, z, .18, 4.5, 2.5, "metal");
    b.add("cover", `cell-back-${i}`, 9, z, .18, 4.5, 3.2, "plaster");
    for (const sign of [-1, 1]) b.add("cover", `cell-end-${i}-${sign}`, 6.3, z + sign * 2.25, 5.4, .18, 3.2, "plaster");
    b.add("bench", `cell-bench-${i}`, 7.9, z, .65, 2.4, .5);
    b.layout.detentionSpawns = [...b.layout.detentionSpawns, pose(5.2, z), pose(6.1, z)];
    b.entrances.push({ id: `cell-${i}`, label: "Detention", point: p(5.2, z), outside: p(2.4, z), width: 1.1, role: "detention", access: "policing-owner" });
    b.zones.push({ id: `detention-${i}`, label: "Detention", bounds: footprint(`detention-${i}`, 6.3, z, 5.1, 4.3), access: "policing-owner" });
    const custody = b.service("police", `cell-service-${i}`, "Custody officer", 5.2, z, 2.4, z, "police", ["release-options", "encounter-status"]);
    custody.access = "policing-owner";
    custody.queue = [p(5.2, z), p(6.1, z)];
  }
  b.service("police", "custody-officer", "Custody officer", 2.4, 3.1, 3.6, 3.1, "police", ["release-options", "regroup"]);
  b.add("bench", "station-waiting", -6.4, -5.5, 3, .7, .5);
  streetDriver(b, 9.5);
  b.npcs.find(npc => npc.id === "custody-officer-npc")!.route = [p(3.6, 3.1), p(2.4, 3.1), p(2.4, -8), p(2.4, 3.1), p(3.6, 3.1)];
  b.layout.qualityGaps = [...b.layout.qualityGaps, "Cells are collision-closed. Only the policing owner can authorize detention admission or release. Charges, paid bail and escape rules are not defined."];
  return b.finish();
}
function pitch(input: CompanionPlaceGeneration) {
  const b = createBuilder(input, 40, 58, true);
  b.layout.theme.floor = "#6c8e61";
  for (const sign of [-1, 1]) {
    b.add("goal", `goal-${sign}`, 0, sign * 22, 7.3, 1.8, 2.5, "metal", false);
    for (const x of [-3.65, 3.65]) b.add("cover", `goal-post-${sign}-${x}`, x, sign * 22, .13, .13, 2.5, "metal");
  }
  for (const sign of [-1, 1]) b.add("bench", `team-bench-${sign}`, sign * 16, 0, 1, 8, .55);
  b.zones.push({ id: "football-field", label: "Football field", bounds: footprint("field", 0, 0, 28, 44), access: "place-membership", activity: "football" });
  b.service("football", "football-contact", "Play football", 0, 24.6, 2, 24.6, "host", ["solo-practice", "team-match"], 12);
  b.spawns.splice(0, b.spawns.length, ...[-6, -3, 0, 3, 6].flatMap(x => [pose(x, 26), pose(x, -25)]));
  b.entrances.push({ id: "team-entry", label: "Team entrance", point: p(0, -25), outside: p(0, -25), width: 2, role: "match-team", access: "joined-match" });
  b.layout.cameras = { wide: { eye: p(12, 27, 5), target: p(0, 0, .8) }, close: { eye: p(2, 26, 2.1), target: p(0, 23, 1) } };
  b.layout.qualityGaps = [...b.layout.qualityGaps, "Pitch markings and goals are supplied here; football owns ball collision, goal rules and team positions."];
  return b.finish();
}
function arena(input: CompanionPlaceGeneration) {
  const b = createBuilder(input, 58, 58, true);
  b.layout.theme.floor = "#a39987";
  // Staggered lanes and broken cover leave a clear perimeter and central path.
  for (const x of [-16, -7, 7, 16]) for (const z of [-13, 0, 13]) {
    b.add("cover", `arena-cover-${x}-${z}`, x, z + (x > 0 ? 2 : -2), 3.2, 4.4, Math.abs(x) > 10 ? 2.2 : 1.1, "stone");
  }
  for (const sign of [-1, 1]) {
    b.add("cover", `arena-divider-${sign}`, sign * 12, -23, 12, .7, 2.4, "plaster");
    b.entrances.push({ id: `arena-team-${sign}`, label: "Team staging", point: p(sign * 23, -24), outside: p(sign * 23, -24), width: 2.4, role: "match-team", access: "joined-match" });
  }
  b.zones.push({ id: "arena-match", label: "Arena", bounds: footprint("arena", 0, -1, 53, 50), access: "joined-match", activity: "arena" });
  b.service("arena", "arena-contact", "Join an arena match", 0, 25, 2, 25, "host", ["join-match"], 16);
  b.spawns.splice(0, b.spawns.length, ...[-1, 1].flatMap(sign => [-25, -23, -21].map(z => pose(sign * 23, z, sign * -Math.PI / 2))));
  b.layout.cameras = { wide: { eye: p(0, 27, 4), target: p(0, 0, 1) }, close: { eye: p(2, 27, 1.8), target: p(0, 24, 1) } };
  b.layout.qualityGaps = [...b.layout.qualityGaps, "Combat owns opt-in match admission, protected team spawns, aiming, equipment and death. This layout alone never enables public PvP."];
  return b.finish();
}
function housing(input: CompanionPlaceGeneration) {
  const count = { room: 1, compound: 2, apartment: 4, "large-home": 4 }[input.housingOption ?? "apartment"];
  const b = createBuilder(input, count === 1 ? 10 : 18, count === 1 ? 12 : 18, input.housingOption === "compound");
  const slots = [
    { id: "living", label: "Studio", suggestedLayoutId: "pat-studio-v01" as const, approach: p(-5, -3) },
    { id: "bedroom", label: "Bedroom", suggestedLayoutId: "pink-bedroom-v02" as const, approach: p(5, -3) },
    { id: "kitchen", label: "Kitchen", suggestedLayoutId: "modern-kitchen-v01" as const, approach: p(-5, 3) },
    { id: "bathroom", label: "Bathroom", suggestedLayoutId: "bathroom-v01" as const, approach: p(5, 3) },
  ].slice(0, count);
  if (count === 1) slots[0].approach = p(-2.8, -2);
  b.layout.homeRooms = slots;
  for (const slot of slots) {
    b.entrances.push({ id: `room-entry-${slot.id}`, label: slot.label, point: slot.approach, outside: slot.approach, width: 1.4, role: "home-room", access: "room-visit-grant" });
    b.services.push({ id: `room-${slot.id}`, label: slot.label, kind: "home-door", approach: { point: slot.approach, yaw: 0 }, anchor: { point: slot.approach, yaw: 0 },
      footprint: footprint(`door-${slot.id}`, slot.approach[0], slot.approach[2], 1.4, .2), capacity: 1, queue: [slot.approach], roomSlotId: slot.id });
  }
  if (input.housingOption === "large-home") {
    b.part("bestiePatStudio", "home-lounge-chair", roomPart(patStudioLayout, "chair"), -5, .3);
    b.add("planter", "home-garden", 5, .3, 1.2, 1.2, 2, "sage");
  }
  b.layout.qualityGaps = [...b.layout.qualityGaps, "Housing options are floor-plan choices, not approved wealth tiers, prices or districts. Bind each door to a real saved room and its visit grant. Own-home banking remains the world and haul owners' verified arrival transaction."];
  return b.finish();
}
function mirrorLayout(layout: CompanionPlaceLayout) {
  const point = (v: ScenePoint): ScenePoint => [-v[0], v[1], v[2]];
  const bounds = <T extends NavigationObstacle>(v: T): T => ({ ...v, minX: -v.maxX, maxX: -v.minX });
  const reflectPose = (v: CompanionPlacePose): CompanionPlacePose => ({ point: point(v.point), yaw: -v.yaw });
  // Mirror the arrangement, not individual mesh geometry or photographed food.
  layout.fixtures = layout.fixtures.map(v => ({ ...v, point: point(v.point), yaw: -v.yaw, collision: v.collision ? bounds(v.collision) : undefined }));
  layout.assets = layout.assets.map(v => ({ ...v, point: point(v.point), yaw: -v.yaw, collision: v.collision ? bounds(v.collision) : undefined }));
  layout.entrances = layout.entrances.map(v => ({ ...v, point: point(v.point), outside: point(v.outside) }));
  layout.spawns = layout.spawns.map(reflectPose); layout.recoverySpawns = layout.recoverySpawns.map(reflectPose); layout.detentionSpawns = layout.detentionSpawns.map(reflectPose);
  layout.services = layout.services.map(v => ({ ...v, approach: reflectPose(v.approach), anchor: reflectPose(v.anchor), footprint: bounds(v.footprint), queue: v.queue.map(point) }));
  layout.npcs = layout.npcs.map(v => ({ ...v, home: reflectPose(v.home), route: v.route.map(point), refuge: point(v.refuge) }));
  layout.homeRooms = layout.homeRooms.map(v => ({ ...v, approach: point(v.approach) }));
  layout.zones = layout.zones.map(v => ({ ...v, bounds: bounds(v.bounds) }));
  layout.cameras = { wide: { eye: point(layout.cameras.wide.eye), target: point(layout.cameras.wide.target) }, close: { eye: point(layout.cameras.close.eye), target: point(layout.cameras.close.target) } };
}
/** Only the world owner generates and stores this resolved layout per instance.
 * Clients consume that stored result; never regenerate an occupied place. */
export function generateCompanionPlaceLayout(input: CompanionPlaceGeneration): CompanionPlaceLayout {
  if (typeof input.placeId !== "string" || !input.placeId || input.placeId.length > 160 || typeof input.seed !== "string" || !input.seed || input.seed.length > 160 ||
    input.generatorVersion !== undefined && input.generatorVersion !== COMPANION_PLACE_GENERATOR_VERSION) throw new Error("Invalid place generation identity or version.");
  switch (input.kind) {
    case "race-circuit": return racingPlaceLayout(input.seed);
    case "water-terminal": case "stilt-creek": case "surf-island": case "timber-atoll": case "under-bridge": {
      const layout = generateWaterfrontLayout(input);
      const issues = inspectCompanionPlaceLayout(layout);
      if (issues.length) throw new Error(`Invalid waterfront layout: ${issues.join("; ")}`);
      return layout;
    }
    case "hospital": return hospital(input);
    case "mama-put": return mamaPut(input);
    case "content-house": return contentHouse(input);
    case "skyline": return restaurant(input, true);
    case "fine-dining": return restaurant(input, false);
    case "nightclub": return nightclub(input);
    case "lounge": return lounge(input);
    case "pool-courtyard": {
      const layout = generatePoolCourtyardLayout(input);
      const issues = inspectCompanionPlaceLayout(layout);
      if (issues.length) throw new Error(`Invalid pool courtyard layout: ${issues.join("; ")}`);
      return layout;
    }
    case "civic-interior": {
      const layout = generateCivicInteriorLayout(input);
      const issues = inspectCompanionPlaceLayout(layout);
      if (issues.length) throw new Error(`Invalid Civic Street interior: ${issues.join("; ")}`);
      return layout;
    }
    case "station": return station(input);
    case "soccer-pitch": return pitch(input);
    case "arena": return arena(input);
    case "housing": return housing(input);
    case "market": case "supermarket": case "bus-terminal": case "fuel-station": case "mechanic": case "salon":
    case "gym": case "cinema": case "library": case "arts-centre": case "campus": case "office":
      return generateCompanionPlaceExpansionLayout(input, createBuilder);
    default: throw new Error("Unknown place kind.");
  }
}
/** Static layout admission checks, not a substitute for rendered inspection. */
export function inspectCompanionPlaceLayout(layout: CompanionPlaceLayout): string[] {
  const issues: string[] = [], nav = layout.navigation;
  const clear = (point: ScenePoint) => routeSegmentClear(nav, point, point);
  const finitePoint = (point: ScenePoint) => point.length === 3 && point.every(Number.isFinite);
  const finitePose = (value: CompanionPlacePose) => finitePoint(value.point) && Number.isFinite(value.yaw);
  const finiteBounds = (value: NavigationObstacle) => [value.minX, value.maxX, value.minZ, value.maxZ].every(Number.isFinite)
    && value.minX < value.maxX && value.minZ < value.maxZ;
  const finiteFootprint = (value: CompanionPlaceFootprint) => finiteBounds(value) && Number.isFinite(value.height) && value.height > 0
    && (value.y === undefined || Number.isFinite(value.y));
  const spawn = layout.spawns[0]?.point;
  if (!spawn) return ["No public spawn"];
  const ids = new Set<string>();
  for (const value of [...layout.fixtures, ...layout.assets, ...layout.services, ...layout.entrances, ...layout.npcs]) {
    if (ids.has(value.id)) issues.push(`Duplicate target ${value.id}`);
    ids.add(value.id);
  }
  for (const fixture of layout.fixtures) {
    if (!finitePoint(fixture.point) || !Number.isFinite(fixture.yaw) || !finitePoint(fixture.size) || fixture.size.some(size => size <= 0)
      || fixture.collision && !finiteFootprint(fixture.collision)) issues.push(`Invalid fixture transform ${fixture.id}`);
  }
  const assets = new Map(layout.assets.map(asset => [asset.id, asset]));
  const npcs = new Map(layout.npcs.map(npc => [npc.id, npc]));
  const services = new Map(layout.services.map(service => [service.id, service]));
  const actorAssets = new Set<string>();
  for (const asset of layout.assets) {
    if (!finitePoint(asset.point) || !Number.isFinite(asset.yaw) || asset.sourceYaw !== undefined && !Number.isFinite(asset.sourceYaw)
      || asset.collision && !finiteFootprint(asset.collision) || asset.sourceBounds && !finiteFootprint(asset.sourceBounds)
      || !asset.npcId && !asset.sourceBounds) issues.push(`Invalid asset selection or transform ${asset.id}`);
  }
  for (const entry of [...layout.spawns, ...layout.recoverySpawns, ...layout.detentionSpawns]) if (!finitePose(entry) || !clear(entry.point)) issues.push("Blocked spawn");
  for (const entry of layout.entrances) {
    if (!clear(entry.point)) issues.push(`Blocked entrance ${entry.id}`);
    if (entry.access !== "policing-owner" && !planRoomRoute(nav, spawn, entry.point)) issues.push(`Unreachable entrance ${entry.id}`);
  }
  for (const service of layout.services) {
    if (!finitePose(service.approach) || !finitePose(service.anchor) || !finiteBounds(service.footprint)
      || !Number.isSafeInteger(service.capacity) || service.capacity <= 0) issues.push(`Invalid service target ${service.id}`);
    if (service.npcId && !npcs.get(service.npcId)?.serviceIds.includes(service.id)) issues.push(`Missing NPC binding ${service.id}`);
    if (!clear(service.approach.point) || service.access !== "policing-owner" && !planRoomRoute(nav, spawn, service.approach.point)) issues.push(`Unreachable service ${service.id}`);
    for (const point of service.queue) if (!clear(point) || !planRoomRoute(nav, service.approach.point, point)) issues.push(`Unreachable queue ${service.id}`);
  }
  for (const npc of layout.npcs) {
    if (!finitePose(npc.home) || !Number.isFinite(npc.radius) || npc.radius <= 0) issues.push(`Invalid NPC pose ${npc.id}`);
    if (npc.assetPartId) {
      if (!assets.get(npc.assetPartId)?.npcId || actorAssets.has(npc.assetPartId)) issues.push(`Missing or shared NPC asset ${npc.id}`);
      actorAssets.add(npc.assetPartId);
    } else if (npc.role !== "resident") issues.push(`Missing NPC asset ${npc.id}`);
    if (new Set(npc.serviceIds).size !== npc.serviceIds.length || npc.serviceIds.some(id => services.get(id)?.npcId !== npc.id)) issues.push(`Invalid NPC services ${npc.id}`);
    if (!clear(npc.home.point) || !clear(npc.refuge)) issues.push(`Blocked NPC ${npc.id}`);
    for (let i = 0; i < npc.route.length; i++) if (!clear(npc.route[i]) || i > 0 && !planRoomRoute(nav, npc.route[i - 1], npc.route[i])) issues.push(`Blocked routine ${npc.id}`);
  }
  for (const asset of layout.assets) if (asset.npcId && !actorAssets.has(asset.id)) issues.push(`Unbound NPC asset ${asset.id}`);
  for (const point of layout.recoverySpawns) {
    if (!planRoomRoute(nav, point.point, spawn)) issues.push("Recovery cannot reach exit");
    if (!layout.services.some(service => service.kind === "medic" && planRoomRoute(nav, point.point, service.approach.point))) issues.push("Recovery cannot reach a medic");
  }
  for (const point of layout.detentionSpawns) {
    if (!layout.services.some(service => service.kind === "police" && service.access === "policing-owner" && planRoomRoute(nav, point.point, service.approach.point))) issues.push("Detention cannot reach a custody service");
    if (planRoomRoute(nav, point.point, spawn)) issues.push("Detention has an unauthorized public exit");
  }
  return issues;
}
