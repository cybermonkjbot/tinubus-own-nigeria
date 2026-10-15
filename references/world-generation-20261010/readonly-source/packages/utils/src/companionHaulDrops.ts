import type { ScenePoint } from "./companionNavigation";

/** One death's unbanked haul. Claiming it never banks or converts its value. */
export type CompanionHaulDrop = {
  id: string;
  instanceId: string;
  position: ScenePoint;
  entryCount: number;
  hasMoney: boolean;
  hasItems: boolean;
  createdAt: number;
};

/** Initial interaction tuning, independent of real-money or reward policy. */
export const COMPANION_HAUL_PICKUP_REACH = 1.75;
export const COMPANION_HAUL_VISIBLE_DROPS = 32;
