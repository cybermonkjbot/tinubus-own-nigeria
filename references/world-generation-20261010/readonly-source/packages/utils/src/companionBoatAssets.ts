import type { CompanionWaterfrontProp, CompanionWaterfrontSurface } from "./companionWaterfront";

/** Retained v02 originals, portable PBR textures and metre-scale waterline roots. */
export const companionBoatAssetEntries = {
  bestieBoatDugoutCanoe: {
    path: "bestie/boats/v02/dugout-canoe.6795de9b885f645e.glb",
    md5: "dbdad78e3afb588acc94b7b3a083631e", sha256: "6795de9b885f645ea1514b26852ce7b455d6794ac89c7e3699ff40a8676dbe83",
    bytes: 9360408, download: "on-demand", published: true,
  },
  bestieBoatPlankedCanoe: {
    path: "bestie/boats/v02/planked-canoe.89d2e44f43542f90.glb",
    md5: "33a27a960ae75b0adf2a88dbb9acd40d", sha256: "89d2e44f43542f907ade67ac2d38979b67dba70be056366a3b11c603a1abe1be",
    bytes: 10647780, download: "on-demand", published: true,
  },
  bestieBoatFishingSkiff: {
    path: "bestie/boats/v02/fishing-skiff.1b7f1921e33c7905.glb",
    md5: "fb7646b790e8c442b87451f8da27470d", sha256: "1b7f1921e33c7905fd559ee5ff4977a6afedb1b47612adbe57d1b1e07631ab5f",
    bytes: 5131136, download: "on-demand", published: true,
  },
  bestieBoatInflatableTender: {
    path: "bestie/boats/v02/inflatable-tender.442e64adbdbda300.glb",
    md5: "ee08998517a73bfca04c6bd7f2737264", sha256: "442e64adbdbda300985791979ec651bec60fcf4b77bc3b9f27016b17f994dd61",
    bytes: 6711080, download: "on-demand", published: true,
  },
  bestieBoatOpenSpeedboat: {
    path: "bestie/boats/v02/open-speedboat.f1d7795508a02276.glb",
    md5: "b3b824019fb412c773de8373e291713e", sha256: "f1d7795508a02276a2aadb9bf4d13acc0c423e715a03ff013c966491d2e76005",
    bytes: 7453196, download: "on-demand", published: true,
  },
  bestieBoatPassengerSpeedboat: {
    path: "bestie/boats/v02/passenger-speedboat.210b720d9f9c8402.glb",
    md5: "d21d12aa0593c0ba5d36db1370770a55", sha256: "210b720d9f9c840296e0d57ffcb516276c0c9ac446bf71efbd5c63970bc55f06",
    bytes: 5672984, download: "on-demand", published: true,
  },
  bestieBoatCabinYacht: {
    path: "bestie/boats/v02/cabin-yacht.0bb420a1bc4c610c.glb",
    md5: "f6909dce2b37a0394a93ea1dbafa7bdf", sha256: "0bb420a1bc4c610c43e28d1d73129d39fc6f597bbc13cd72e34aec374cffa320",
    bytes: 10112984, download: "on-demand", published: true,
  },
  bestieBoatFlybridgeYacht: {
    path: "bestie/boats/v02/flybridge-yacht.a14cf1a599657986.glb",
    md5: "a0e6c829fd5bcd57e381e5c29586214d", sha256: "a14cf1a599657986d9c2bfc2ef91102760d67136c716387314c80d3f17d4357b",
    bytes: 10999352, download: "on-demand", published: true,
  },
  bestieBoatJetSki: {
    path: "bestie/boats/v02/jet-ski.d2fe316256b17e61.glb",
    md5: "06ad183af0ed5a6b76edfa693bf49f29", sha256: "d2fe316256b17e61bf2ab241506667d1dc8f0b00219d208ac0f0e0839a62bf30",
    bytes: 1744084, download: "on-demand", published: true,
  },
  bestieBoatTimberBarge: {
    path: "bestie/boats/v02/timber-barge.823ae6be044252a6.glb",
    md5: "f4aa5c6f139dec429fb37a9cac7b0716", sha256: "823ae6be044252a6cdd3c3e1c7b3579b1844a138f62af766df0dd2def821cac1",
    bytes: 10096124, download: "on-demand", published: true,
  },
  bestieBoatSandDredger: {
    path: "bestie/boats/v02/sand-dredger.a10372d917d9d6ec.glb",
    md5: "2c37f757f96fefdcb2d7aa2bc31eb437", sha256: "a10372d917d9d6ecc04b5a20e5ac4b056285a5727c1ae4f9234d31b0272e8ac7",
    bytes: 7494868, download: "on-demand", published: true,
  },
} as const;

export const companionBoatAssetKeys = {
  "dugout-canoe": "bestieBoatDugoutCanoe",
  "planked-canoe": "bestieBoatPlankedCanoe",
  "fishing-skiff": "bestieBoatFishingSkiff",
  "inflatable-tender": "bestieBoatInflatableTender",
  "open-speedboat": "bestieBoatOpenSpeedboat",
  "passenger-speedboat": "bestieBoatPassengerSpeedboat",
  "cabin-yacht": "bestieBoatCabinYacht",
  "flybridge-yacht": "bestieBoatFlybridgeYacht",
  "jet-ski": "bestieBoatJetSki",
  "timber-barge": "bestieBoatTimberBarge",
  "sand-dredger": "bestieBoatSandDredger",
} as const;
export type CompanionBoatModelId = keyof typeof companionBoatAssetKeys;
export type CompanionBoatAssetKey = typeof companionBoatAssetKeys[CompanionBoatModelId];

/** Exported Y-up bounds include the motors, rails, rigging and cargo. */
export const companionBoatBounds = {
  "dugout-canoe": { min: [-0.5172500014305115, -0.18000000715255737, -2.8227508068084717], max: [0.5172500014305115, 0.5614793300628662, 2.823087215423584] },
  "planked-canoe": { min: [-0.6477500200271606, -0.23999999463558197, -3.21348237991333], max: [0.6477500200271606, 0.6724572777748108, 3.21348237991333] },
  "fishing-skiff": { min: [-0.8688869476318359, -0.3199999928474426, -2.9114999771118164], max: [0.8979828357696533, 1.0679999589920044, 2.412318229675293] },
  "inflatable-tender": { min: [-0.8699432611465454, -0.2199999988079071, -1.8907999992370605], max: [0.8799828290939331, 0.9980000257492065, 1.8151600360870361] },
  "open-speedboat": { min: [-1.163583755493164, -0.5600000023841858, -4.230000019073486], max: [1.163583755493164, 1.834999918937683, 3.4168455600738525] },
  "passenger-speedboat": { min: [-1.2970564365386963, -0.6840000152587891, -5.8520002365112305], max: [1.2970564365386963, 2.455000162124634, 4.918889045715332] },
  "cabin-yacht": { min: [-2.0349831581115723, -1.100000023841858, -8.923999786376953], max: [2.0349831581115723, 3.369999885559082, 7.523486137390137] },
  "flybridge-yacht": { min: [-2.7867379188537598, -1.7249999046325684, -11.72499942779541], max: [2.7867379188537598, 6.360000133514404, 11.248108863830566] },
  "jet-ski": { min: [-0.5773407220840454, -0.3500000238418579, -1.6769999265670776], max: [0.5773407220840454, 0.8330000042915344, 1.6156702041625977] },
  "timber-barge": { min: [-2.955000162124634, -1.149999976158142, -9], max: [2.955000162124634, 4.050000190734863, 9] },
  "sand-dredger": { min: [-3.705000162124634, -2.0306715965270996, -11], max: [4.241931438446045, 4.050000190734863, 11] },
} as const;

export function companionBoatModelId(prop: Pick<CompanionWaterfrontProp, "boat" | "boatModel">): CompanionBoatModelId {
  if (prop.boatModel && Object.hasOwn(companionBoatAssetKeys, prop.boatModel)) return prop.boatModel;
  switch (prop.boat) {
    case "speedboat": return "passenger-speedboat";
    case "sand-barge": return "sand-dredger";
    case "yacht": return "cabin-yacht";
    case "jet-ski": return "jet-ski";
    default: return "dugout-canoe";
  }
}

export function companionBoatAssetKey(prop: Pick<CompanionWaterfrontProp, "boat" | "boatModel">): CompanionBoatAssetKey {
  return companionBoatAssetKeys[companionBoatModelId(prop)];
}

/** Old saved recipes retain their silhouettes when larger art would cover dry ground. */
export function companionBoatClearsDrySurfaces(prop: CompanionWaterfrontProp, surfaces: readonly CompanionWaterfrontSurface[]) {
  const bounds = companionBoatBounds[companionBoatModelId(prop)], yaw = prop.yaw + Math.PI;
  const c = Math.cos(yaw), s = Math.sin(yaw);
  const corners = [bounds.min[0], bounds.max[0]].flatMap(x => [bounds.min[2], bounds.max[2]].map(z =>
    [prop.point[0] + x * c + z * s, prop.point[2] - x * s + z * c]));
  // Covers the existing small roll as well as export tolerance.
  const padding = .12;
  const minX = Math.min(...corners.map(p => p[0])) - padding, maxX = Math.max(...corners.map(p => p[0])) + padding;
  const minZ = Math.min(...corners.map(p => p[1])) - padding, maxZ = Math.max(...corners.map(p => p[1])) + padding;
  return !surfaces.some(surface => minX < surface.maxX && maxX > surface.minX && minZ < surface.maxZ && maxZ > surface.minZ);
}
