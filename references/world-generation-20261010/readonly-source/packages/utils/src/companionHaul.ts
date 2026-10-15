/** Account-owned game rewards. This is not a wallet or a cash conversion. */
export type CompanionHaulStatus = "carried" | "banked" | "forfeited";
export type CompanionHaulSource = {
  kind: "world-loot" | "job" | "delivery" | "mission" | "activity" | "pos-withdrawal" | "street-trade";
  /** Server-issued completion or loot claim, scoped to its beneficiary. */
  key: string;
};
export type CompanionHaulReward =
  | { kind: "item"; itemKey: string; quantity: number }
  | { kind: "money"; amount: number; unit: "game-value-unit"; unitKey: string }
  | { kind: "money"; amount: number; unit: "real-currency-minor"; currency: string; fundingReference: string };
export type CompanionHaulEntry = {
  id: string; outingId: string; source: CompanionHaulSource; reward: CompanionHaulReward;
  status: CompanionHaulStatus; grantedAt: number;
  /** Original grant remains available for receipts; reward is the value still held. */
  originalMoneyAmount?: number;
};
export type CompanionHaulOuting = {
  id: string; characterId: string; worldId: string; instanceId: string;
  status: CompanionHaulStatus; revision: number; entryCount: number;
  departedAt: number; resolvedAt: number | null;
};
export type CompanionHaulSnapshot = {
  outing: CompanionHaulOuting | null; entries: CompanionHaulEntry[];
  cashConversion: "disabled"; withdrawal: "unavailable"; bankedTransfer: "disabled";
};
export type CompanionHaulPage = { page: CompanionHaulEntry[]; continueCursor: string; isDone: boolean };
export type CompanionHaulStoreSnapshot = CompanionHaulSnapshot & { ready: boolean; error: string | null };
export const emptyCompanionHaulSnapshot: CompanionHaulStoreSnapshot = {
  outing: null, entries: [], cashConversion: "disabled", withdrawal: "unavailable", bankedTransfer: "disabled",
  ready: false, error: null,
};
export const COMPANION_HAUL_MAX_ENTRIES = 128;
export const COMPANION_HAUL_MAX_QUANTITY = 10_000;
export const COMPANION_HAUL_MAX_AMOUNT = 1_000_000_000;

/** Never sum currencies, funded minor units and game units together. */
export function companionHaulMoneyGroups(entries: readonly CompanionHaulEntry[]) {
  const groups = new Map<string, {
    unit: "game-value-unit" | "real-currency-minor"; unitKey: string; status: CompanionHaulStatus;
    amount: number; sourceKeys: string[]; fundingReferences: string[];
  }>();
  for (const entry of entries) {
    if (entry.reward.kind !== "money" || entry.status === "forfeited") continue;
    const reward = entry.reward;
    const unit = reward.unit === "game-value-unit" ? reward.unitKey : reward.currency;
    const key = JSON.stringify([entry.status, reward.unit, unit]);
    const group = groups.get(key);
    if (!Number.isSafeInteger(reward.amount) || reward.amount < 0) throw new Error("The haul amount needs review.");
    if (reward.amount === 0) continue;
    const amount = (group?.amount ?? 0) + reward.amount;
    if (!Number.isSafeInteger(amount)) throw new Error("The haul amount needs review.");
    groups.set(key, { unit: reward.unit, unitKey: unit, status: entry.status, amount,
      sourceKeys: [...(group?.sourceKeys ?? []), entry.source.key],
      fundingReferences: [...(group?.fundingReferences ?? []), ...(reward.unit === "real-currency-minor" ? [reward.fundingReference] : [])],
    });
  }
  return [...groups.values()];
}
