import type { CompanionAppearance } from "../../utils/src/companion";
export type { CharacterBaseId } from "../../utils/src/companion";
export type GroupCallAppearance = Partial<CompanionAppearance>;
export type GroupCallParticipant = { appearance?: GroupCallAppearance; speechLevel?: number; reaction?: { id: string; kind: "nod" | "delight" | "greeting"; expiresAt: number } | null };
