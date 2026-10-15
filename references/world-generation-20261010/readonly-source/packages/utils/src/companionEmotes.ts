import type { CharacterCommand } from "./companionExpression";

export const companionEmotes = [
  { id: "greeting", label: "Wave", durationMs: 2200 },
  { id: "nod", label: "Nod", durationMs: 1400 },
  { id: "cheer", label: "Cheer", durationMs: 2600 },
  { id: "stretch", label: "Stretch", durationMs: 3000 },
  { id: "groove", label: "Groove", durationMs: 2800 },
  { id: "sway", label: "Sway", durationMs: 3000 },
] as const;
export type CompanionEmoteId = typeof companionEmotes[number]["id"];
export const isCompanionEmote = (value: unknown): value is CompanionEmoteId => companionEmotes.some(emote => emote.id === value);
export function companionEmoteCommand(characterId: string, emoteId: CompanionEmoteId, now = Date.now()): CharacterCommand {
  const emote = companionEmotes.find(value => value.id === emoteId)!;
  const requestId = `emote.${now}.${Math.random().toString(36).slice(2)}`;
  return { characterId, requestId, sessionId: "local-emote", turnId: requestId, action: "gesture", value: emoteId,
    intensity: 1, durationMs: emote.durationMs, issuedAt: now, expiresAt: now + emote.durationMs };
}
