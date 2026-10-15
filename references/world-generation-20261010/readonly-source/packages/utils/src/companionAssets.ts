import { PUBLIC_ASSET_BASE_URL } from "./asset-storage";
import { companionArchitectureAssetEntries } from "./companionArchitectureAssets";
import { companionBillboardAssetEntries } from "./companionBillboardAssets";
import { companionRoadAssetEntries } from "./companionRoadAssets";
import { companionFrontageAssetEntries } from "./companionFrontageAssets";
import { companionNpcVoiceAssetEntries } from "./companionNpcVoices";
import { companionBoatAssetEntries } from "./companionBoatAssets";
import { companionVehicleAssetEntries } from "./companionVehicleAssets";
import { companionDistrictAssetEntries } from "./companionDistrictAssets";
import { companionPickupAssetEntries } from "./companionPickupAssets";

/** Public, immutable files only. Account libraries and room edits never enter this cache. */
export const companionAssetEntries = {
  ...companionBoatAssetEntries,
  ...companionVehicleAssetEntries,
  ...companionDistrictAssetEntries,
  ...companionPickupAssetEntries,
  ...companionArchitectureAssetEntries,
  ...companionBillboardAssetEntries,
  ...companionRoadAssetEntries,
  ...companionFrontageAssetEntries,
  ...companionNpcVoiceAssetEntries,
  bestieShopRibbonBall: {
    path: "bestie/shop/v1/ribbon-ball.a7dd51d14ac9e881.glb",
    md5: "d7ae5de034b278cada511b1d81ec16f2",
    sha256: "a7dd51d14ac9e881fae48d60d672dc122d2a73a7b8c59bc84b22db9574866491",
    bytes: 905092, download: "on-demand", published: true,
  },
  bestieShopStackingRings: {
    path: "bestie/shop/v1/stacking-rings.987b3dd9942b84df.glb",
    md5: "1205d89162603caa40932a5398a97f4e",
    sha256: "987b3dd9942b84dfa9deb654afc81e2b3658a8b62a72b4e9293fbd7211fd11a7",
    bytes: 1029184, download: "on-demand", published: true,
  },
  bestieShopCuddleBear: {
    path: "bestie/shop/v1/cuddle-bear.8f50d7d2d9b956b7.glb",
    md5: "0236a7d0eef50a243ae77384065be247",
    sha256: "8f50d7d2d9b956b7e603e6f0915fd77965a610168ce08715cdd93fc1ee2ee4ee",
    bytes: 1424140, download: "on-demand", published: true,
  },
  bestieShopRoundGlasses: {
    path: "bestie/shop/v1/round-glasses.79ab93ea78694986.glb",
    md5: "afc5c78074b5193c5265b0eb26ae0717",
    sha256: "79ab93ea786949864445c53f77d554d592d64b6f35e973a0ef530c26130ac1b0",
    bytes: 117168, download: "on-demand", published: true,
  },
  bestieShopLinenScarf: {
    path: "bestie/shop/v1/linen-scarf.e9da1d1e5a09e2de.glb",
    md5: "5e5223583906d0a49d3a22cfe7d14631",
    sha256: "e9da1d1e5a09e2de0ea1aac78d81ba68a38fbfe0201ea1af1e739dcaa16231d4",
    bytes: 964744, download: "on-demand", published: true,
  },
  bestieShopDaisyPin: {
    path: "bestie/shop/v1/daisy-pin.317a48a4002c62ec.glb",
    md5: "48c51a2754d0b2a0bf7f93173f1db791",
    sha256: "317a48a4002c62ec493a816c431f07791b0dfc746de1cf22cb8750aea3b133d7",
    bytes: 161712, download: "on-demand", published: true,
  },
  bestieWeaponPistol: {
    path: "bestie/weapons/v2/pistol-v02.f6872d79273affdd.glb",
    md5: "70f7f69bdf7c43480c247b4aafffd657",
    sha256: "f6872d79273affdd0afee086fd65932395980a4bfb945c7b4a2e08581d11dd8a",
    bytes: 1928796, download: "on-demand", published: true,
  },
  bestieWeaponRevolver: {
    path: "bestie/weapons/v2/revolver-v02.54d87744506d088e.glb",
    md5: "d2c22efa52bbeda86c8c99559a828d70",
    sha256: "54d87744506d088ea425dcf58c6791cc369457312561d0a9d918ab5f38e51d7a",
    bytes: 1644488, download: "on-demand", published: true,
  },
  bestieWeaponSmg: {
    path: "bestie/weapons/v2/smg-v02.7ae96b1dbf720a04.glb",
    md5: "432ab4bfe1ef6ed55d90061e73937167",
    sha256: "7ae96b1dbf720a04062fc722ea1cd2b2da82cb7a5c3173f6edc40e99e14edfdd",
    bytes: 1723668, download: "on-demand", published: true,
  },
  bestieWeaponShotgun: {
    path: "bestie/weapons/v2/shotgun-v02.7c2612b5aac21c75.glb",
    md5: "54029e0808ed85d4944156fb9de98cd1",
    sha256: "7c2612b5aac21c75f03de5e8ad4d81cbba9ee31bb4eedd7e4d7492767f51b5a8",
    bytes: 2038264, download: "on-demand", published: true,
  },
  bestieWeaponRifle: {
    path: "bestie/weapons/v2/rifle-v02.f47e914d4ff85d4f.glb",
    md5: "29f04b4a45abe4e403194d7ab32c70ef",
    sha256: "f47e914d4ff85d4fcd98a8940b1f8e215ebe1a8423726d43852f3cb1a213d0c0",
    bytes: 2454692, download: "on-demand", published: true,
  },
  bestieTerrace: {
    path: "bestie/rooms/terrace-v03-lossless.2765c88f90bd5e68.glb",
    md5: "efae016b35b7b500d9db8be600329afa",
    sha256: "2765c88f90bd5e689aa4f618d148be141543346569266b25c19c74cfe18920be",
    bytes: 3419968, download: "on-demand", published: true,
  },
  bestieTownSquare: {
    path: "bestie/rooms/town-square-v07-lossless.4b93f185219bc88e.glb",
    md5: "f21d353aa51ad318ebca297a146da971",
    sha256: "4b93f185219bc88e1379a1c45a3d4429abba62baa57093446c0c09164efb3dd0",
    bytes: 114933604, download: "on-demand", published: true,
  },
  bestieTownSquareNpcs: {
    path: "bestie/rooms/town-square-v07-npcs-lossless.b815b4ddad587e84.glb",
    md5: "d23ee73fd8e69e71debdf5feb51e89f9",
    sha256: "b815b4ddad587e84bc9279ed2fd12f3f43a4f734c7e6c4a9b3451245e19b5ec1",
    bytes: 32012796, download: "on-demand", published: true,
  },
  bestieKitchen: {
    path: "bestie/rooms/modern-kitchen-v01.3d2f160c5a001827.glb",
    md5: "7d0e22d8411df8819b783a58774d7c99",
    sha256: "3d2f160c5a0018270a7762e38e029fe7e1eee20d98d4a51dd4805f5edac53308", bytes: 1377752,
    download: "on-demand", published: true,
  },
  bestieBathroom: {
    path: "bestie/rooms/bathroom-v01.1715c195b8d09dad.glb",
    md5: "4a5326e836ac16c566d33acf5f00b37a",
    sha256: "1715c195b8d09dadb980e1919500f1d84ed779bf016c94e7e8130dff77ac423b", bytes: 795984,
    download: "on-demand", published: true,
  },
  bestieCharacterV27: {
    path: "bestie/characters/bestie-v27a-lossless.552e83aae0f42244.glb",
    md5: "787c634f30d73f440ba42a1fff875c46",
    sha256: "552e83aae0f422440c913993f8ef25e2a7495406ad9bf86238e87445487806f2",
    bytes: 15887680,
    download: "on-demand",
    published: true,
  },
  bestiePatStudio: {
    path: "bestie/rooms/studio-v07-lossless.cf20f87f5f2eac92.glb",
    md5: "f5d25bf6b3a2d6dec64f3119c4ae593a",
    sha256: "cf20f87f5f2eac92a306e121dca39917444455699ab8da80a4f458c5fb5d769c",
    bytes: 19580572,
    download: "on-demand",
    published: true,
  },
  bestiePinkBedroom: {
    path: "bestie/rooms/pink-bedroom-v03.cef0a0cc8ec684f4.glb",
    md5: "c3d24ac57a91b67ccb0002c1831382df",
    sha256: "cef0a0cc8ec684f47eb3186de71e837e0b02e48b938a82dd01b62fa108e187d9",
    bytes: 1242604,
    download: "on-demand",
    published: false,
  },
  bestiePinkBedroomV2: {
    path: "bestie/rooms/pink-bedroom-v15-lossless.7765d65d8419ffa8.glb",
    md5: "6f32b1978494f231715c6628de4b67d2",
    sha256: "7765d65d8419ffa86f9136f8fdb22640994af03dfa901b1332443f73463ae2e3",
    bytes: 4829976,
    download: "on-demand",
    published: true,
  },
} as const;

export type CompanionAssetKey = keyof typeof companionAssetEntries;

/** Keep native cache names aligned with each registered file's actual format. */
export function companionAssetCacheFilename(key: CompanionAssetKey): string {
  const entry = companionAssetEntries[key];
  return `${key}-${entry.md5}.${entry.path.split(".").at(-1)}`;
}

export const companionShopAssetKeys = {
  "ribbon-ball": "bestieShopRibbonBall",
  "stacking-rings": "bestieShopStackingRings",
  "cuddle-bear": "bestieShopCuddleBear",
  "round-glasses": "bestieShopRoundGlasses",
  "linen-scarf": "bestieShopLinenScarf",
  "daisy-pin": "bestieShopDaisyPin",
} as const satisfies Record<string, CompanionAssetKey>;

export type CompanionShopAssetId = keyof typeof companionShopAssetKeys;

export const companionShopPreviewPaths = {
  "ribbon-ball": "bestie/previews/shop-v1-ribbon-ball.c982b1312bb8a2ed.webp",
  "stacking-rings": "bestie/previews/shop-v1-stacking-rings.c2fa5f24537514f1.webp",
  "cuddle-bear": "bestie/previews/shop-v1-cuddle-bear.fd504d58770b590c.webp",
  "round-glasses": "bestie/previews/shop-v1-round-glasses.a3055567d87e728f.webp",
  "linen-scarf": "bestie/previews/shop-v1-linen-scarf.68f995961364bcfe.webp",
  "daisy-pin": "bestie/previews/shop-v1-daisy-pin.05a8a8b295644462.webp",
} as const satisfies Record<CompanionShopAssetId, string>;

/** Verified public R2 host. Each entry still gates publication independently. */
export const companionAssetBaseUrl = PUBLIC_ASSET_BASE_URL;

/** Reviewed Rose room stills, separate from the geometry-only native asset bridge. */
export const companionRoomPreviewPaths = {
  "pat-studio-v01": "bestie/previews/pat-studio-v07.dfcb74b75e7b7efa.webp",
  "pink-bedroom-v02": "bestie/previews/pink-bedroom-v15.8ce13f96cc49a9f7.webp",
} as const;

export function companionAssetUrl(key: CompanionAssetKey): string | null {
  const entry = companionAssetEntries[key];
  return entry.published ? `${companionAssetBaseUrl}/${entry.path}` : null;
}

/** The host resolves files through its existing non-critical asset store. */
export interface CompanionAssetDelivery {
  /** Native packages contain renderer code, not the web preview's legacy GLB. */
  allowBundledCharacterFallback?: boolean;
  available(key: CompanionAssetKey): boolean;
  acquire(key: CompanionAssetKey, signal: AbortSignal,
    onProgress?: (progress: { downloadedBytes: number; totalBytes: number }) => void): Promise<string>;
  /** Isolated web renderers read registered bytes through their app's cache. */
  read?(key: CompanionAssetKey, signal: AbortSignal): Promise<ArrayBuffer>;
}
