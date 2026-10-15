import type { PersonalRoom } from "./companion";
import { characterControlId, parseCharacterCommand, type CharacterCommand, type CharacterCue } from "./companionExpression";

/** One bounded cue on the existing participant presence record. No audio or coordinates. */
export type CompanionCallExpression = CharacterCue & {
  eventId: string;
  sequence: number;
  roomRevision: number;
  characterRevision: number;
};

export const CALL_EXPRESSION_INTERVAL_MS = 750;

export function callExpressionTargetIds(room: PersonalRoom | null | undefined): string[] {
  return (room?.items ?? []).map(item => `room:${room!.id}:${item.kind}:${item.id}`).filter(characterControlId);
}

/** Shared limits, used before sending and independently by the authenticated server. */
export function validCallExpressionCommand(command: CharacterCommand, characterId: string,
  targetIds: readonly string[], now: number): boolean {
  if (!command || !characterControlId(command.requestId) || !characterControlId(command.sessionId)
    || !characterControlId(command.turnId) || !Number.isFinite(command.issuedAt)
    || !Number.isFinite(command.expiresAt) || command.issuedAt > now || command.expiresAt <= now) return false;
  const { action, value, intensity, durationMs, targetId } = command;
  const parsed = parseCharacterCommand({ characterId: command.characterId, action, value, intensity, durationMs,
    ...(targetId === undefined ? {} : { targetId }) }, {
    characterId, visibleTargetIds: targetIds, supportedActions: ["express", "gesture", "look_at", "present_target"],
  }, command);
  return !!parsed && parsed.expiresAt === command.expiresAt;
}

export function sameCharacterCommand(a: CharacterCommand, b: CharacterCommand): boolean {
  return a.requestId === b.requestId && a.sessionId === b.sessionId && a.turnId === b.turnId
    && a.characterId === b.characterId && a.action === b.action && a.value === b.value
    && a.intensity === b.intensity && a.durationMs === b.durationMs && a.targetId === b.targetId
    && a.issuedAt === b.issuedAt && a.expiresAt === b.expiresAt;
}
