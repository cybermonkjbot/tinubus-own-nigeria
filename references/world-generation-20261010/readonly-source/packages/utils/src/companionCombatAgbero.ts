import type { CombatFighter } from "./companionCombat";

/** Only this authored body has a street-contact and representation contract. */
export const companionCombatAgberoBody = {
  npcId: "street-tout-shirt",
  contact: [-3.2, 0, -5],
  contactRadius: 3,
  sourceYaw: -Math.PI / 2,
} as const;

export function companionCombatAgberoSupported(fighter: Pick<CombatFighter, "kind" | "characterId" | "targetId">): boolean {
  return fighter.kind === "agbero" && fighter.characterId === companionCombatAgberoBody.npcId && !!fighter.targetId;
}
