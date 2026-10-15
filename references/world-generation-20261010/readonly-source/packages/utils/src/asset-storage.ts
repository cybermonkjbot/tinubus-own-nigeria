import { ACTIVE_OWNED_DOMAIN } from "./domain-config";

export type AssetVisibility = "public" | "private";
export type AssetStorageBackend = "cdn" | "convex_storage";
export type PublicAssetCategory =
  | "official_avatar"
  | "official_banner"
  | "room_hero_banner"
  | "explore_category_image"
  | "explore_lane_image"
  | "published_social_media"
  | "sticker"
  | "unknown";

export interface AssetStorageDecision {
  visibility: AssetVisibility;
  backend: AssetStorageBackend;
  category: PublicAssetCategory;
  shouldOptimize: boolean;
}

export const PUBLIC_ASSET_HOST = `assets.${ACTIVE_OWNED_DOMAIN}`;
export const PUBLIC_ASSET_BASE_URL = `https://${PUBLIC_ASSET_HOST}`;

const PUBLIC_CATEGORIES = new Set<PublicAssetCategory>([
  "official_avatar",
  "official_banner",
  "room_hero_banner",
  "explore_category_image",
  "explore_lane_image",
  "published_social_media",
  "sticker"
]);

export function resolvePublicAssetCategory(uploadKey?: string | null): PublicAssetCategory {
  const normalized = String(uploadKey ?? "").trim().toLowerCase();
  if (!normalized) return "unknown";
  if (normalized === "official-avatar") return "official_avatar";
  if (normalized === "official-banner") return "official_banner";
  if (normalized.startsWith("room-hero-banner-")) return "room_hero_banner";
  if (normalized.startsWith("explore-category-")) return "explore_category_image";
  if (normalized.startsWith("explore-lane-")) return "explore_lane_image";
  if (normalized.startsWith("social-publish-") || normalized.startsWith("post-publish-") || normalized.startsWith("official-social-")) {
    return "published_social_media";
  }
  if (normalized.startsWith("sticker-")) return "sticker";
  return "unknown";
}

export function decideAssetStorage(input: {
  uploadKey?: string | null;
  sourceType?: "cdn" | "convex_storage" | null;
  forcePrivate?: boolean;
}): AssetStorageDecision {
  if (input.forcePrivate) {
    return {
      visibility: "private",
      backend: "convex_storage",
      category: resolvePublicAssetCategory(input.uploadKey),
      shouldOptimize: true
    };
  }

  if (input.sourceType === "cdn") {
    return {
      visibility: "public",
      backend: "cdn",
      category: resolvePublicAssetCategory(input.uploadKey),
      shouldOptimize: false
    };
  }

  const category = resolvePublicAssetCategory(input.uploadKey);
  const isPublic = PUBLIC_CATEGORIES.has(category);

  return {
    visibility: isPublic ? "public" : "private",
    backend: isPublic ? "cdn" : "convex_storage",
    category,
    shouldOptimize: !isPublic
  };
}

export function isPublicAssetUrl(url?: string | null): boolean {
  const value = String(url ?? "").trim().toLowerCase();
  return value.startsWith(`${PUBLIC_ASSET_BASE_URL.toLowerCase()}/`) || value === PUBLIC_ASSET_BASE_URL.toLowerCase();
}
