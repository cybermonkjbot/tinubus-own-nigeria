import design from "../../assets/city/lagos-billboards-v1.json";

/** Two fixed boards, in separate districts. The other three remain scenery. */
export const COMPANION_AD_BILLBOARD_IDS = ["bestieBillboardTownCentre", "bestieBillboardSkyline"] as const;
export type CompanionAdBillboardId = typeof COMPANION_AD_BILLBOARD_IDS[number];
export const COMPANION_BILLBOARD_IMAGE_LIMIT = 80 * 1024;
export type CompanionBillboardAd = Readonly<{
  boardId: CompanionAdBillboardId;
  ticketId: string;
  advertiser: string;
  title: string;
  imageDataUri: string;
  expiresAt: number;
}>;
export type CompanionBillboardTarget = Readonly<{ boardId: CompanionAdBillboardId; ticketId: string }>;
export type CompanionBillboardVisibility = CompanionBillboardTarget & Readonly<{ visible: boolean }>;

export function isCompanionAdBillboardId(value: unknown): value is CompanionAdBillboardId {
  return COMPANION_AD_BILLBOARD_IDS.some(id => id === value);
}
export function validCompanionBillboardAd(value: unknown): value is CompanionBillboardAd {
  if (!value || typeof value !== "object") return false;
  const ad = value as Record<string, unknown>;
  return isCompanionAdBillboardId(ad.boardId) && typeof ad.ticketId === "string" && ad.ticketId.length > 0 && ad.ticketId.length <= 240
    && typeof ad.advertiser === "string" && ad.advertiser.length > 0 && ad.advertiser.length <= 80
    && typeof ad.title === "string" && ad.title.length > 0 && ad.title.length <= 160
    && typeof ad.expiresAt === "number" && Number.isSafeInteger(ad.expiresAt) && ad.expiresAt > 0
    && typeof ad.imageDataUri === "string" && ad.imageDataUri.length <= COMPANION_BILLBOARD_IMAGE_LIMIT
    && /^data:image\/jpeg;base64,\/9j\/[A-Za-z0-9+/]+={0,2}$/.test(ad.imageDataUri);
}
export function nearbyCompanionAdBillboard(position?: { x: number; z: number }): CompanionAdBillboardId | null {
  if (!position || !Number.isFinite(position.x) || !Number.isFinite(position.z)) return null;
  let nearest: CompanionAdBillboardId | null = null, distance = 420;
  for (const board of design.placements) {
    if (!isCompanionAdBillboardId(board.id)) continue;
    const next = Math.hypot(position.x - board.focusPosition[0], position.z - board.focusPosition[2]);
    if (next < distance) { nearest = board.id; distance = next; }
  }
  return nearest;
}
