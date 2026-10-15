import type { CombatPoint } from "./companionCombat";

export type CompanionCombatRobberyPhase = "demanding" | "fleeing" | "defending" | "surrendered" | "escaped" | "repelled" | "defeated" | "interrupted" | "expired";
export type CompanionCombatRobbery = {
  id: string;
  toutId: string;
  phase: CompanionCombatRobberyPhase;
  unitKey: "lagos-credit";
  amountCap: number;
  createdAt: number;
  respondsAt: number;
  expiresAt: number;
  origin: CombatPoint;
  escapeSince: number | null;
  resolvedAt: number | null;
  surrenderedAmount: number;
  revision: number;
  purpose?: "recovery";
};

export const companionRobberyTuning = {
  demandMs: 12_000,
  encounterMs: 60_000,
  maximumStake: 200,
  stakePercent: 15,
  escapeFromOrigin: 8,
  escapeFromAttacker: 5,
  escapeHoldMs: 1_000,
  maxCarriedEntries: 128,
} as const;

export function companionRobberyActive(robbery: CompanionCombatRobbery) {
  return robbery.phase === "demanding" || robbery.phase === "fleeing" || robbery.phase === "defending";
}

export function companionRobberyStake(carriedCredits: number, isNight = false) {
  if (!Number.isSafeInteger(carriedCredits) || carriedCredits < 0) throw new Error("Invalid carried Lagos credits.");
  const maxStake = isNight ? 250 : companionRobberyTuning.maximumStake;
  const stakePercent = isNight ? 20 : companionRobberyTuning.stakePercent;
  return Math.min(maxStake, Math.floor(carriedCredits / 100 * stakePercent));
}

export function finishCompanionRobbery(robbery: CompanionCombatRobbery,
  phase: Exclude<CompanionCombatRobberyPhase, "demanding" | "fleeing" | "defending">, now: number): CompanionCombatRobbery {
  if (!Number.isFinite(now) || now < robbery.createdAt) throw new Error("Invalid encounter time.");
  return companionRobberyActive(robbery) ? { ...robbery, phase, escapeSince: null, resolvedAt: now, revision: robbery.revision + 1 } : robbery;
}

/** Only accepted authoritative positions can resolve escape. Selecting Run never does. */
export function advanceCompanionRobbery(robbery: CompanionCombatRobbery, player: CombatPoint,
  attacker: CombatPoint, now: number): CompanionCombatRobbery {
  if (!companionRobberyActive(robbery)) return robbery;
  if (!Number.isFinite(now) || now < robbery.createdAt || ![player, attacker].every(point => point.length === 3 && point.every(Number.isFinite)))
    throw new Error("Invalid encounter observation.");
  if (now >= robbery.expiresAt) return finishCompanionRobbery(robbery, "expired", now);
  const clear = Math.hypot(player[0] - robbery.origin[0], player[2] - robbery.origin[2]) >= companionRobberyTuning.escapeFromOrigin
    && Math.hypot(player[0] - attacker[0], player[2] - attacker[2]) >= companionRobberyTuning.escapeFromAttacker;
  const escapeSince = clear ? robbery.escapeSince ?? now : null;
  if (escapeSince !== null && now - escapeSince >= companionRobberyTuning.escapeHoldMs) return finishCompanionRobbery(robbery, "escaped", now);
  const phase = robbery.phase === "demanding" && now >= robbery.respondsAt ? "defending" : robbery.phase;
  return phase !== robbery.phase || escapeSince !== robbery.escapeSince
    ? { ...robbery, phase, escapeSince, revision: robbery.revision + 1 } : robbery;
}

export function companionRobberyObjective(robbery: CompanionCombatRobbery) {
  if (robbery.purpose === "recovery" && companionRobberyActive(robbery)) return "Defeat the agbero to recover stolen credits";
  switch (robbery.phase) {
    case "demanding": return "Agbero demand";
    case "fleeing": return "Get clear of the agbero";
    case "defending": return "Defend or get clear";
    case "surrendered": return robbery.surrenderedAmount ? `Surrendered ${robbery.surrenderedAmount} Lagos credits` : "Released without payment";
    case "escaped": return "Escaped";
    case "repelled": return "Agbero defeated";
    case "defeated": return "Recover at the hospital";
    case "interrupted": return "Encounter interrupted";
    case "expired": return "Agbero left";
  }
}
