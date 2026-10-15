/** Authored character expression, not a claim about anyone's feelings. */
export const CHARACTER_ACTION_VALUES = {
  express: ["neutral", "attentive", "thoughtful", "delighted"],
  gesture: ["nod", "greeting", "cheer", "stretch", "groove", "sway"],
  look_at: ["user", "target"],
  present_target: ["target"],
} as const;
export type CharacterAction = keyof typeof CHARACTER_ACTION_VALUES;
export interface CharacterControlContext {
  characterId: string;
  visibleTargetIds: readonly string[];
  supportedActions: readonly CharacterAction[];
}
export interface CharacterCommand {
  requestId: string;
  sessionId: string;
  turnId: string;
  characterId: string;
  action: CharacterAction;
  value: string;
  intensity: number;
  durationMs: number;
  targetId?: string;
  issuedAt: number;
  expiresAt: number;
}
/** Local application receipt, never evidence that a remote participant rendered it. */
export interface CharacterExpressionReceipt {
  requestId: string;
  status: "applied" | "completed" | "cancelled" | "rejected";
}
export interface CharacterCue {
  source: "ai" | "user";
  type: "apply" | "cancel";
  command: CharacterCommand;
}
export const characterControlId = (value: unknown): value is string => typeof value === "string" && value.length > 0 && value.length <= 120 && /^[a-zA-Z0-9_.:-]+$/.test(value);

/** Copy only renderer-provided capabilities and visible IDs into model context. */
export function characterControlContext(value: CharacterControlContext | null | undefined): CharacterControlContext | null {
  if (!value || !characterControlId(value.characterId) || !Array.isArray(value.visibleTargetIds) || !Array.isArray(value.supportedActions)) return null;
  return {
    characterId: value.characterId,
    visibleTargetIds: [...new Set(value.visibleTargetIds.filter(characterControlId))].slice(0, 24),
    supportedActions: [...new Set(value.supportedActions.filter(action => Object.prototype.hasOwnProperty.call(CHARACTER_ACTION_VALUES, action)))],
  };
}

export function parseCharacterCommand(input: unknown, context: CharacterControlContext,
  identity: Pick<CharacterCommand, "requestId" | "sessionId" | "turnId" | "issuedAt">): CharacterCommand | null {
  if (!input || typeof input !== "object" || Array.isArray(input)) return null;
  const args = input as Record<string, unknown>;
  if (Object.keys(args).some(key => !["characterId", "action", "value", "intensity", "durationMs", "targetId"].includes(key))) return null;
  if (args.characterId !== context.characterId || typeof args.action !== "string" || !Object.prototype.hasOwnProperty.call(CHARACTER_ACTION_VALUES, args.action)) return null;
  const action = args.action as CharacterAction;
  if (!context.supportedActions.includes(action) || typeof args.value !== "string" || !(CHARACTER_ACTION_VALUES[action] as readonly string[]).includes(args.value)) return null;
  if (typeof args.intensity !== "number" || !Number.isFinite(args.intensity) || args.intensity < 0 || args.intensity > 1) return null;
  if (typeof args.durationMs !== "number" || !Number.isInteger(args.durationMs) || args.durationMs < 200 || args.durationMs > 3000) return null;
  const targeted = action === "present_target" || (action === "look_at" && args.value === "target");
  if (targeted ? !characterControlId(args.targetId) || !context.visibleTargetIds.includes(args.targetId) : args.targetId !== undefined) return null;
  return { ...identity, characterId: context.characterId, action, value: args.value, intensity: args.intensity,
    durationMs: args.durationMs, ...(targeted ? { targetId: args.targetId as string } : {}), expiresAt: identity.issuedAt + args.durationMs };
}
