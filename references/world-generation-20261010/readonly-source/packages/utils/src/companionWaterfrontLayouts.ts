import { companionAssetEntries } from "./companionAssets";
import { companionBoatAssetEntries, companionBoatAssetKey, companionBoatModelId, companionBoatBounds, type CompanionBoatModelId } from "./companionBoatAssets";
import type { NavigationObstacle, ScenePoint } from "./companionNavigation";
import type { CompanionPlaceAssetPart, CompanionPlaceFixture, CompanionPlaceGeneration, CompanionPlaceLayout,
  CompanionPlaceNpc, CompanionPlaceService } from "./companionPlaces";
import { COMPANION_WATERFRONT_VERSION, companionWaterfrontPlace, type CompanionBoatKind,
  companionWaterfrontLevelService, type CompanionWaterfrontProp, type CompanionWaterfrontPropKind, type CompanionWaterfrontSurface } from "./companionWaterfront";

/** One stored recipe owns geometry, collision, staff and boarding points. Never shuffle an occupied place. */
export function generateWaterfrontLayout(input: CompanionPlaceGeneration): CompanionPlaceLayout {
  const place = companionWaterfrontPlace(input.placeId);
  if (!place || place.kind !== input.kind) throw new Error("Choose a registered waterfront place.");
  const bridge = place.kind === "under-bridge", creek = place.kind === "stilt-creek", surf = place.kind === "surf-island";
  const width = bridge ? 60 : surf ? 90 : creek ? 76 : 84, depth = bridge ? 48 : surf ? 72 : 64;
  const bounds = { minX: -width / 2, maxX: width / 2, minZ: -depth / 2, maxZ: depth / 2 };
  const props: CompanionWaterfrontProp[] = [], surfaces: CompanionWaterfrontSurface[] = [];
  const fixtures: CompanionPlaceFixture[] = [], obstacles: NavigationObstacle[] = [];
  const services: CompanionPlaceService[] = [], npcs: CompanionPlaceNpc[] = [], assets: CompanionPlaceAssetPart[] = [];
  const point = (x: number, z: number, y = 0): ScenePoint => [x, y, z];
  const block = (id: string, x: number, z: number, w: number, d: number, h: number, yaw = 0) => {
    const collision = { id, minX: x - w / 2, maxX: x + w / 2, minZ: z - d / 2, maxZ: z + d / 2, height: h, yaw };
    fixtures.push({ id, kind: "cover", point: point(x, z), size: [w, h, d], yaw, material: "wood", collision });
    obstacles.push(collision);
  };
  const prop = (kind: CompanionWaterfrontPropKind, id: string, x: number, z: number, w: number, h: number, d: number,
    collides = true, yaw = 0, y = 0, variant = props.length % 5) => {
    const value: CompanionWaterfrontProp = { id, kind, point: point(x, z, y), size: [w, h, d], yaw, variant };
    props.push(value);
    if (collides) block(`${id}:footprint`, x, z, w, d, h, yaw);
    return value;
  };
  const surface = (id: string, kind: CompanionWaterfrontSurface["kind"], minX: number, maxX: number, minZ: number, maxZ: number) =>
    surfaces.push({ id, kind, minX, maxX, minZ, maxZ });
  const waterBlock = (minX: number, maxX: number, minZ: number, maxZ: number) => obstacles.push({ minX, maxX, minZ, maxZ });
  const boat = (kind: CompanionBoatKind, id: string, x: number, z: number, yaw = 0, model?: CompanionBoatModelId) => {
    const value = prop("boat", id, x, z, 1, 1, 1, false, yaw, -.18);
    value.boat = kind; value.boatModel = model;
    value.boatModel = companionBoatModelId(value);
    const bounds = companionBoatBounds[companionBoatModelId(value)];
    value.size = [bounds.max[0] - bounds.min[0], bounds.max[1] - bounds.min[1], bounds.max[2] - bounds.min[2]];
  };
  const service = (id: string, label: string, x: number, z: number, nx: number, nz: number,
    kind: CompanionPlaceService["kind"] = "driver") => {
    const npcId = `${id}-npc`, home = { point: point(nx, nz), yaw: Math.atan2(x - nx, z - nz) };
    services.push({ id, label, kind, approach: { point: point(x, z), yaw: home.yaw + Math.PI > Math.PI ? home.yaw - Math.PI : home.yaw + Math.PI },
      anchor: home, footprint: { minX: nx - .35, maxX: nx + .35, minZ: nz - .35, maxZ: nz + .35 },
      npcId, capacity: 1, queue: [point(x, z), point(x, z + 1), point(x, z + 2)], offers: kind === "gather" ? ["conversation-circle"] : undefined });
    const assetId = `${npcId}-art`;
    npcs.push({ id: npcId, role: kind === "driver" ? "driver" : "host", label, home, route: [home.point], refuge: point(0, depth / 2 - 5),
      radius: .3, serviceIds: [id], assetPartId: assetId });
    assets.push({ id: assetId, key: "bestieTownSquareNpcs", npcId: "street-tout-shirt", point: home.point, yaw: home.yaw, sourceYaw: -Math.PI / 2 });
  };
  const sign = (text: string, x: number, z: number, yaw = 0) => {
    prop("sign", `sign-${props.length}`, x, z, 3.6, 2.4, .18, false, yaw).text = text;
  };
  const entry: ScenePoint = point(0, depth / 2 - 2);
  const passage = (target: string, label: string, x: number, z: number, stairId: string) => {
    const id = companionWaterfrontLevelService(target), pose = { point: point(x, z), yaw: Math.PI };
    services.push({ id, label, kind: "home-door", approach: pose, anchor: pose,
      footprint: { minX: x - .35, maxX: x + .35, minZ: z - .35, maxZ: z + .35 }, capacity: 1, queue: [pose.point] });
    const stair = props.find(prop => prop.id === stairId);
    if (stair) stair.serviceId = id;
  };

  if (surf) {
    surface("sandbar", "sand", -40, 40, -28, 32);
    waterBlock(-45, -40, -36, 36); waterBlock(40, 45, -36, 36);
    waterBlock(-40, 40, -36, -28); waterBlock(-40, 40, 32, 36);
    // Arrival is on the quiet lagoon side, never in the surf.
    surface("landing", "wood", -2, 2, 24, 35);
    obstacles.splice(obstacles.length - 1, 1);
    waterBlock(-40, -2, 32, 36); waterBlock(2, 40, 32, 36);
    prop("jetty", "island-landing", 0, 29, 4, .25, 12, false);
    boat("speedboat", "island-transfer", -9, 33.6, Math.PI / 2);
    for (const [i, x] of [-30, -21, 20, 29].entries()) {
      prop("palm", `coconut-${i}`, x, -12 + i * 8, 1.1, 9 + i % 2, 1.1);
      prop("hut", `fishing-hut-${i}`, x, 18, 5.8, 3.3, 4.2);
      prop("net-rack", `nets-${i}`, x + 4, 23, 1.5, 1.7, .7);
    }
    prop("beach-club", "whitewashed-club", -23, -8, 19, 5.2, 12);
    for (const x of [10, 15, 20, 25]) prop("lounger", `sunbed-${x}`, x, -17, 1.4, 2.3, 3);
    prop("quad-bike", "sand-quad-a", 22, 5, 1.5, 1.25, 2.3);
    prop("quad-bike", "sand-quad-b", 27, 8, 1.5, 1.25, 2.3, true, -.7);
    for (let i = 0; i < 12; i++) prop("tetrapod", `sea-defence-${i}`, -34 + i * 6.1, -34, 3.2, 2.5, 3.2, false, i * .6, -.8);
    prop("rock-groyne", "surf-groyne", 37, -29, 4, 2.8, 20, false);
    for (let i = 0; i < 3; i++) prop("container-ship", `anchored-ship-${i}`, -150 + i * 155, -430 - i * 60, 25, 22, 110, false, 1.1 + i * .4, -.4);
    sign(place.label, 7, 25);
    service("boat-driver", "Board a speedboat", -1, 26, -2.7, 26);
  } else if (creek) {
    // Black-water alleys remain open to canoes. The dry walking network is a connected timber cross.
    surface("central-boardwalk", "wood", -2.2, 2.2, -30, 32);
    surface("cross-boardwalk", "wood", -35, 35, -2.2, 2.2);
    for (const x of [-1, 1]) for (const z of [-1, 1])
      waterBlock(x < 0 ? -38 : 2.2, x < 0 ? -2.2 : 38, z < 0 ? -32 : 2.2, z < 0 ? -2.2 : 32);
    for (const [i, x] of [-27, -16, 16, 27].entries()) for (const z of [-10, 10]) {
      prop("stilt-house", `stilt-house-${i}-${z}`, x, z, 8.4, 6.6, 7.5, false, 0, -.35, i);
      prop("laundry", `washing-${i}-${z}`, x, z + 4, 5, 2.9, .25, false);
      boat("canoe", `dugout-${i}-${z}`, x + 4.8, z, z < 0 ? 0 : Math.PI, i % 2 ? "planked-canoe" : "dugout-canoe");
    }
    for (const x of [-31, 31]) for (const z of [-24, -15, 15, 24]) prop("mangrove", `roots-${x}-${z}`, x, z, 6, 6.3, 6, false);
    for (let i = 0; i < 10; i++) prop(i % 2 ? "oil-sheen" : "hyacinth", `creek-float-${i}`, -31 + i * 6.8, i % 2 ? -23 : 22, 3.4, .08, 2.3, false);
    prop("fish-rack", "smoked-fish", -8, 0, 3, 1.7, 1.3);
    prop("net-rack", "net-mending", 9, 0, 3, 2, 1.4);
    service("canoe-driver", "Paddle to Okobaba", 0, 24, 1.3, 24);
    boat("canoe", "fishing-skiff", -7, 24, 0, "fishing-skiff");
    sign("Makoko canoe landing", 4, 27);
  } else if (place.kind === "timber-atoll") {
    surface("sawdust-atoll", "mud", -34, 34, -23, 30);
    surface("timber-landing", "wood", -2, 2, 26, 32);
    waterBlock(-42, -34, -32, 32); waterBlock(34, 42, -32, 32); waterBlock(-34, 34, -32, -23);
    waterBlock(-34, -2, 30, 32); waterBlock(2, 34, 30, 32);
    prop("jetty", "timber-jetty", 0, 28, 4, .3, 9, false);
    prop("sawmill", "working-sawmill", -21, -9, 16, 5, 13);
    prop("generator", "mill-generator", -28, 8, 3.2, 2.2, 2.2);
    prop("wood-pile", "offcut-pyramid", 21, -10, 12, 4.4, 10);
    for (let i = 0; i < 5; i++) prop("log-raft", `floating-log-raft-${i}`, -29 + i * 15, -28 - i % 2 * 5, 10, 1, 7, false, .08 * i, -.3);
    for (const x of [-19, 18]) prop("wood-pile", `bound-timber-${x}`, x, 12, 9, 2.3, 5);
    boat("sand-barge", "dredging-barge", 40, 0);
    boat("sand-barge", "timber-carrier", -40, 0, 0, "timber-barge");
    boat("canoe", "creek-transfer", -7, 31, Math.PI / 2);
    boat("speedboat", "lagoon-transfer", 9, 33, Math.PI / 2);
    service("boat-driver", "Choose a boat crossing", 0, 24, -1.4, 24);
    sign("Okobaba timber landing", 6, 23);
  } else if (bridge) {
    const upper = place.level === "abutment", pier = place.level === "pier";
    if (pier) {
      surface("pier-foundation", "concrete", -28, 28, -20, 15);
      surface("pier-waterside-walk", "wood", -2.5, 2.5, 15, 24);
      waterBlock(-30, -28, -24, 24); waterBlock(28, 30, -24, 24);
      waterBlock(-28, 28, -24, -20);
      waterBlock(-28, -2.5, 15, 24); waterBlock(2.5, 28, 15, 24);
    } else surface("bridge-floor", upper ? "wood" : "concrete", -30, 30, -24, 24);
    prop("bridge-deck", "concrete-flyover", 0, 0, 48, 2, 64, false, 0, upper ? 4.3 : 7);
    for (const x of [-23, 23]) for (const z of [-16, 14]) prop("bridge-pier", `pier-${x}-${z}`, x, z, 2.4, upper ? 5 : 8, 3.4);
    if (upper) {
      for (const [i, x] of [-17, -9, 9, 17].entries()) {
        prop("sleeping-platform", `sleeping-boards-${i}`, x, -10, 5.5, 1, 2.5);
        prop("cart", `stored-vendor-cart-${i}`, x, 5, 3, 1.8, 1.8);
      }
      prop("laundry", "girders-laundry", 0, -16, 24, 2.5, .3, false);
      prop("stair", "abutment-stair", -14, 18, 6, 4.5, 4, true);
      passage("obalende-underbridge", "Go down to the market", -14, 21.2, "abutment-stair");
    } else if (pier) {
      prop("stilt-house", "pier-braced-shack", -17, -8, 11, 6.8, 9);
      prop("fish-rack", "pier-fish-smoking", 12, -5, 5, 2, 2.5);
      prop("net-rack", "pier-net-mending", 12, 7, 6, 2.8, 2);
      for (const x of [-16, -7, 8, 17]) boat("canoe", `tied-canoe-${x}`, x, -30);
      prop("jetty", "pier-boarding", 0, 18, 5, .3, 11, false);
      service("boat-driver", "Board a boat", 0, 16, -1.4, 16);
    } else {
      for (const [i, z] of [-12, -4, 4].entries()) {
        prop(i === 1 ? "phone-stall" : "market-stall", `pillar-market-${i}`, -14, z, 7, 3.2, 4.4);
        sign(i === 1 ? "Phone accessories" : i === 0 ? "Market" : "Repairs", -14, z + 2.4);
      }
      prop("food-grill", "suya-and-akara", 13, -12, 6, 2.7, 4.5);
      prop("mechanic-bay", "vulcanizer-workshop", 14, 2, 11, 3, 9);
      prop("tyre-stack", "spare-tyres", 23, 2, 2.8, 2.5, 3.5);
      prop("danfo", "terminal-danfo", 14, 16, 3, 3, 8);
      prop("prayer-space", "prayer-corner", -14, -20, 11, 2.6, 4);
      prop("plaza", "bridge-plaza", -36, -4, 13, 9, 26, false);
      prop("stair", "market-stair", -23, 17, 4, 5.7, 5, true);
      if (place.id === "obalende-underbridge") passage("obalende-abutment", "Go up to the abutment", -23, 21.2, "market-stair");
      prop("sleeping-platform", "raised-sleeping-boards", 11, -18, 7, 1.7, 3, false, 0, 4);
      prop("laundry", "abutment-washing", 0, -19, 17, 4.8, .3, false);
      prop("drain", "open-drain", -27, 0, 1, .25, 44, true);
      service("terminal-driver", "Choose transport", 0, 16, -1.4, 16);
    }
    sign(place.label, 6, 20);
  } else {
    surface("terminal-concourse", "concrete", -42, 42, -12, 32);
    surface("boarding-pier", "wood", -2.5, 2.5, -29, -10);
    waterBlock(-42, -2.5, -32, -12); waterBlock(2.5, 42, -32, -12); waterBlock(-2.5, 2.5, -32, -29);
    prop("jetty", "boarding-jetty", 0, -19, 5, .35, 20, false);
    for (const [i, x] of [-27, -15, 15, 27].entries()) {
      prop("market-stall", `jetty-kiosk-${i}`, x, 9, 7, 3.8, 5);
      prop("bollard", `mooring-${i}`, x, -11, .7, .7, .7);
      prop("palm", `terminal-palm-${i}`, x, 23, 1.1, 8.8, 1.1);
    }
    boat("speedboat", "ferry-a", -8, -23); boat("speedboat", "ferry-b", 8, -23);
    boat("sand-barge", "sand-dredger", -36, -50, Math.PI / 2);
    if (place.id === "ikoyi-marina") {
      boat("yacht", "marina-yacht", 23, -26);
      boat("yacht", "marina-flybridge", 36, -36, 0, "flybridge-yacht");
      boat("speedboat", "marina-runabout", -21, -23, 0, "open-speedboat");
      boat("canoe", "marina-tender", 15, -32, Math.PI / 2, "inflatable-tender");
      boat("jet-ski", "marina-jet-ski", 17, -20);
    }
    service("boat-driver", "Choose a boat crossing", 0, -7, -1.4, -7);
    service("terminal-driver", "Choose road transport", 0, 24, -1.4, 24);
    sign(place.label, 7, 24); sign("Boat departures", 7, -9);
    for (let i = 0; i < 12; i++) prop(i % 3 ? "hyacinth" : "debris", `lagoon-float-${i}`, -35 + i * 6, -35 - i % 3 * 4, 2.8, .1, 2.2, false);
  }
  const id = `${input.placeId}:lagos-places-v1:${COMPANION_WATERFRONT_VERSION}:${encodeURIComponent(input.seed)}`;
  const boatKeys = [...new Set(props.filter(p => p.kind === "boat").map(companionBoatAssetKey))].sort();
  const assetVersion = `${COMPANION_WATERFRONT_VERSION}|bestieTownSquareNpcs:${companionAssetEntries.bestieTownSquareNpcs.sha256}`
    + boatKeys.map(key => `|${key}:${companionBoatAssetEntries[key].sha256}`).join("");
  const spawnZ = depth / 2 - (surf ? 5 : 4);
  return { id, placeId: place.id, kind: place.kind, label: place.label, seed: input.seed, generatorVersion: "lagos-places-v1", assetVersion,
    navigation: { revision: `${id}:${assetVersion}`, bounds, obstacles, radius: .24, floorY: 0, gridSize: .95 }, fixtures, assets,
    entrances: [{ id: "world-entry", label: place.boatOnly ? "Boat landing" : "Street", point: surf ? point(0, 30) : entry,
      outside: point(0, depth / 2 + 1), width: 4, role: "world", access: "place-membership" }],
    spawns: [-.6, .6].map(x => ({ point: point(x, spawnZ), yaw: Math.PI })), recoverySpawns: [], detentionSpawns: [], services, npcs,
    zones: [], homeRooms: [], cameras: { wide: { eye: point(3, spawnZ + 2, 2.6), target: point(0, spawnZ - 7, 1.1) },
      close: { eye: point(1.7, spawnZ + 1, 1.8), target: point(0, spawnZ - 3, .9) } },
    theme: { background: bridge ? "#a6adb0" : "#b6ccd5", floor: surf ? "#d4bd8a" : creek ? "#382f23" : "#a29c89",
      wall: "#b9b7aa", accent: "#6b8584", openAir: true },
    waterfront: { version: COMPANION_WATERFRONT_VERSION, zone: place.waterZone, level: place.level ?? "ground", props, surfaces },
    qualityGaps: ["Native/web gameplay review remains pending. Boats other than the registered crossing craft are scenery.",
      "Rolling logs, free boat steering, surf collision, vehicle ownership and new trade transactions are not supplied by the scenery recipe."],
  };
}
