import type { CompanionAppearance, PersonalRoom } from "./companion";
import type { CharacterCommand, CharacterExpressionReceipt } from "./companionExpression";
import type { CompanionCallExpression } from "./companionCallExpression";
import type { CompanionMovement, CompanionPace } from "./companionMovement";
import type { CompanionEmoteId } from "./companionEmotes";

export type CompanionPosition = { x: number; z: number; yaw: number };
export type CompanionReaction = "nod" | "delight" | "greeting";
/** Public pose only. Kitchen stock, recipes, needs and purchase receipts stay private. */
export type CompanionCallCooking = {
  activityId: string; characterId: string; characterRevision: number;
  callSessionId: string; roomId: string; roomRevision: number;
  expressionSessionId: string; movementSequence: number;
  stationId: string; startedAt: number; endsAt: number;
};
export type CompanionCallRoom = {
  callSessionId: string | null; threadId: string; name: string;
  environmentId: "studio" | "terrace" | "town-square"; canChangeEnvironment: boolean; expiresAt: number | null;
  destinations?: Array<{ participantId: string; roomId: string; name: string }>;
  gameRoomId?: string;
  /** Server-advertised, explicitly enrolled shared-room transport. */
  edgeTransportAvailable?: boolean;
  /** Public room hearing is positional; private conversation audio is not. */
  voiceMode?: "proximity" | "call";
  room: PersonalRoom | null; roomRevision: number; canEditRoom: boolean; isHost: boolean;
  /** Current saved kitchen identity, excluding edited or temporary call copies. */
  savedKitchen?: boolean;
  participants: Array<{
    userId: string; displayName: string; handle: string; agoraUid: number;
    position: CompanionPosition; expiresAt: number;
    movementSequence?: number;
    movement?: CompanionMovement | null;
    reaction: { kind: CompanionReaction; expiresAt: number } | null;
    characterId: string | null; characterName: string | null; appearance: CompanionAppearance | null;
    characterRevision?: number;
    /** Opaque presence identity, not an authentication token. Changes on rejoin. */
    expressionSessionId?: string;
    /** Current server-granted room transport generation, not an authentication token. */
    edgeEpoch?: string;
    expressionSequence?: number;
    expression?: CompanionCallExpression | null;
    cooking?: CompanionCallCooking | null;
  }>;
};
export type CompanionCallAudio = {
  channelId: string; status: "idle" | "connecting" | "connected" | "ended" | "error";
  reconnecting: boolean; isMuted: boolean; isSpeakerEnabled: boolean; errorMessage: string | null;
  participants: Array<{ uid: number; userId: string | null; isLocal: boolean; isMuted: boolean | null; isSpeaking: boolean; audioLevel: number | null }>;
};
export interface CompanionCallTransport {
  capabilities: { speakerRouting: boolean };
  getSnapshot(): CompanionCallAudio;
  subscribe(listener: (snapshot: CompanionCallAudio) => void): () => void;
  updateParticipants(value: readonly { uid: number; userId: string }[]): void;
  join(value: { appId: string; token: string; uid: number; startMuted?: boolean }): Promise<unknown>;
  setRemoteAudioLevels(value: readonly { uid: number; level: number }[]): void;
  leave(): Promise<void>;
  renewToken(value: string): Promise<void>;
  setMuted(value: boolean): Promise<void>;
  setSpeakerEnabled(value: boolean): void;
  dispose(): void;
}
export type CompanionCallState = {
  worldUtilities?: import("./companionWorld").CompanionWorldUtilityReceipt | null;
  utilityDisplay?: { observation: import("./companionWorldUtilities").CompanionWorldUtilityObservation;
    receivedAt: number; callSessionId: string; roomRevision: number } | null;
  /** False for an explicitly joined space without an RTC connection. */
  audioEnabled?: boolean;
  voiceError?: string | null;
  groups: Array<{ threadId: string; communityId?: string; name: string; memberCount: number }> | null;
  room: CompanionCallRoom | null; audio: CompanionCallAudio | null;
  status: "idle" | "joining" | "joined" | "leaving";
  localUid: number | null; error: string | null; groupsError: string | null;
  speakerRouting: boolean;
  /** Local draft cue. It is not published until the own rig reports a rendered frame. */
  localExpression?: CharacterCommand | null;
  /** Predicted or acknowledged own route, independent of membership heartbeats. */
  localMovement?: CompanionMovement | null;
};
export interface CompanionCallController {
  getSnapshot(): CompanionCallState;
  subscribe(listener: () => void): () => void;
  /** Group selection owns this read; local call-state observers do not. */
  observeGroups(): () => void;
  join(threadId: string, expectedCallSessionId?: string, options?: { audio?: boolean; startMuted?: boolean }): Promise<void>;
  leave(): Promise<void>;
  /** Optional game voice never owns player presence or gameplay. Starts muted. */
  setVoiceEnabled(value: boolean): Promise<void>;
  setMuted(value: boolean): Promise<void>;
  setSpeakerEnabled(value: boolean): void;
  stageRoom(id: string, revision: number, ownerParticipantId?: string): Promise<void>;
  travelToTownSquare(revision: number): Promise<void>;
  saveRoom(room: PersonalRoom, revision: number): Promise<void>;
  move(position: CompanionPosition, pace?: CompanionPace): Promise<void>;
  stopMoving(): Promise<void>;
  react(kind: CompanionReaction | CompanionEmoteId): Promise<void>;
  reportCharacterExpression(command: CharacterCommand, receipt: CharacterExpressionReceipt): void;
  cancelCharacterExpression(): void;
  reloadGroups(): void;
  dismissError(): void;
  dispose(): Promise<void>;
}
