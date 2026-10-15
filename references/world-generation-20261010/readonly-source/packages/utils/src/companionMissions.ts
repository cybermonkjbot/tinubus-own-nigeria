import type { ScenePoint } from "./companionNavigation";
import { companionMissionExpansion } from "./companionMissionExpansion";

export type MissionShot = { durationMs: number; speaker: string; text: string;
  eye: ScenePoint; target: ScenePoint; fov: number };
export type MissionStep = { kind: "scene"; id: string; shots: readonly MissionShot[] }
  | { kind: "reach"; id: string; placeId: string; label: string }
  | { kind: "fight"; id: string; placeId: "arena"; label: string; enemies: number }
  | { kind: "inspect"; id: string; placeId: string; label: string; hint: string; actionLabel?: string;
      target: { kind: "service" | "fixture"; id: string }; finding: string }
  | { kind: "choice"; id: string; placeId: string; label: string; speaker: string;
      choices: readonly { id: string; label: string; reply: string; nextStepId: string }[] }
  | { kind: "challenge"; id: string; placeId: "soccer-pitch" | "nightclub"; label: string;
      activity: "football" | "dance"; goals?: number; assists?: number; hits?: number; combo?: number; difficulty?: "easy" | "standard" | "hard" };
export const missionCategories = ["story", "neighbourhood", "investigation", "sport", "nightlife", "creator"] as const;
export type MissionCategory = typeof missionCategories[number];
export type CompanionMission = { id: string; title: string; summary: string; prerequisite: string | null;
  category?: MissionCategory; contact?: string; credits?: number; xp?: number; repeatHours?: number; steps: readonly MissionStep[] };
const shot = (speaker: string, text: string, side: number, durationMs = 4500): MissionShot => ({
  speaker, text, durationMs, eye: [side * 3.2, 1.7, 4.5], target: [0, 1.1, 0], fov: 42,
});

/** Authored story, shared by the server, native host and scene renderer. No generated outcomes. */
export const companionMissions: readonly CompanionMission[] = [
  { id: "missing-reel", title: "The missing reel", summary: "Beat the two arena guards, then return Tomi's stolen footage to the creator contact at the content house.", prerequisite: null, steps: [
    { kind: "scene", id: "call-from-tomi", shots: [
      shot("Tomi", "They took the reel before we could publish. The only copy is at the arena.", -1),
      shot("Tomi", "Get it back. Whoever hired them is in that footage.", 1),
    ] },
    { kind: "reach", id: "arena-door", placeId: "arena", label: "Reach the arena entrance" },
    { kind: "scene", id: "no-free-entry", shots: [
      shot("Arena guard", "Tomi sent you? Then you already know this isn't a friendly visit.", 1),
      shot("Tomi", "Stay together. They're between you and the reel.", -1),
    ] },
    { kind: "fight", id: "recover-the-reel", placeId: "arena", label: "Fight through the guards and recover the reel", enemies: 2 },
    { kind: "scene", id: "a-name-on-film", shots: [
      shot("Tomi", "That's it. The reel is intact. Bring it to the content house.", -1),
      shot("Tomi", "I recognised the man in the car. He's supposed to be on our side.", 1),
    ] },
    { kind: "inspect", id: "deliver-the-reel", placeId: "content-house", label: "Hand the reel to the creator contact", actionLabel: "Hand over reel",
      hint: "Approach the creator work contact inside the content house.", target: { kind: "service", id: "creator-contact" },
      finding: "The creator contact accepts the recovered reel for Tomi. The footage is intact." },
    { kind: "scene", id: "the-reveal", shots: [
      shot("Tomi", "Dele paid for the theft. And this isn't the first crew he's silenced.", -1),
      shot("Tomi", "We publish tomorrow. Tonight, we find his runner.", 1),
    ] },
  ] },
  { id: "last-witness", title: "The last witness", summary: "Ask at Mama put for the runner's lead, beat three arena guards and hand the payment records to the creator contact.", prerequisite: "missing-reel", steps: [
    { kind: "scene", id: "runner-location", shots: [
      shot("Tomi", "The runner agreed to meet at Mama put. He's carrying the payment records.", -1),
      shot("Tomi", "Dele knows he's talking. Don't leave him behind.", 1),
    ] },
    { kind: "inspect", id: "meet-the-runner", placeId: "mama-put", label: "Ask the food vendor for the runner's message", actionLabel: "Ask about the runner",
      hint: "Approach the Mama put vendor at the food counter.", target: { kind: "service", id: "mama-vendor" },
      finding: "The vendor passes on the runner's message. The payment records are in an arena locker." },
    { kind: "scene", id: "an-unsafe-meeting", shots: [
      shot("Runner", "They saw me leave. The records are in an arena locker. That's where they'll wait.", 1),
      shot("Tomi", "Clear a way in. We need those records before Dele does.", -1),
    ] },
    { kind: "reach", id: "return-to-arena", placeId: "arena", label: "Reach the arena entrance" },
    { kind: "fight", id: "secure-the-records", placeId: "arena", label: "Defeat the crew guarding the payment records", enemies: 3 },
    { kind: "inspect", id: "bring-evidence-home", placeId: "content-house", label: "Hand the payment records to the creator contact", actionLabel: "Hand over records",
      hint: "Approach the creator work contact inside the content house.", target: { kind: "service", id: "creator-contact" },
      finding: "The creator contact accepts the records for Tomi. The payments match the recovered footage." },
    { kind: "scene", id: "ready-to-publish", shots: [
      shot("Tomi", "The reel, the payments, his name. We finally have enough.", -1),
      shot("Runner", "Then let the whole city see it.", 1),
    ] },
  ] },
  ...companionMissionExpansion,
];
export const companionMission = (id: string) => companionMissions.find(mission => mission.id === id) ?? null;
export function missionReward(mission: CompanionMission) {
  return { credits: mission.credits ?? 100, xp: mission.xp ?? 40 };
}
export function missionCareer(completed: readonly string[]) {
  const missions = companionMissions.filter(mission => completed.includes(mission.id));
  const xp = missions.reduce((total, mission) => total + missionReward(mission).xp, 0);
  const ranks = [{ name: "New arrival", xp: 0 }, { name: "Known around town", xp: 120 },
    { name: "Trusted regular", xp: 350 }, { name: "City insider", xp: 750 }, { name: "Lagos mainstay", xp: 1400 }];
  const rank = [...ranks].reverse().find(value => xp >= value.xp)!;
  return { xp, rank: rank.name, nextRank: ranks.find(value => value.xp > xp) ?? null,
    contacts: [...new Set(missions.flatMap(mission => mission.contact ? [mission.contact] : []))] };
}
export const missionSceneDuration = (step: Extract<MissionStep, { kind: "scene" }>) => step.shots.reduce((ms, frame) => ms + frame.durationMs, 0);
export type MissionRunView = { id: string; missionId: string; leaderId: string; crewThreadId: string | null; revision: number;
  stepIndex: number; phase: "lobby" | "scene" | "objective" | "combat" | "failed" | "completed"; sceneStartedAt: number | null;
  failure: string | null; participants: Array<{ userId: string; name: string }>;
  canLead?: boolean; completionPending?: boolean; finding?: string | null;
  readiness?: { revision: number; selfReady: boolean; readyUserIds: string[]; allReady: boolean };
  findings?: Array<{ stepId: string; text: string }>;
  challengeContributors?: string[];
  objectiveStartedAt?: number;
  target?: { placeId: string; point: ScenePoint; label: string } | null;
  reward?: { credits: number; status: "carried" | "banked" | "forfeited" } | null };
export type MissionCutscene = { runId: string; revision: number; stepId: string; startedAt: number; shots: readonly MissionShot[] };
/** Crew consent is renewed before the opening scene and each fight attempt. Solo play is unchanged. */
export function missionReadinessRequired(run: {
  crewThreadId?: string | null; missionId: string; phase: MissionRunView["phase"]; stepIndex: number; completionPending?: boolean;
}) {
  return !!run.crewThreadId && (run.phase === "lobby" || !run.completionPending
    && (run.phase === "objective" || run.phase === "failed") && companionMission(run.missionId)?.steps[run.stepIndex]?.kind === "fight");
}
export function missionCutscene(run: MissionRunView | null): MissionCutscene | null {
  const step = run ? companionMission(run.missionId)?.steps[run.stepIndex] : null;
  return run?.phase === "scene" && run.sceneStartedAt !== null && step?.kind === "scene"
    ? { runId: run.id, revision: run.revision, stepId: step.id, startedAt: run.sceneStartedAt, shots: step.shots } : null;
}
export function sampleMissionCutscene(scene: MissionCutscene, now: number) {
  let elapsed = Math.max(0, now - scene.startedAt);
  for (const frame of scene.shots) {
    if (elapsed < frame.durationMs) return frame;
    elapsed -= frame.durationMs;
  }
  // Keep the last shot until the authoritative phase changes, including network delay.
  return scene.shots[scene.shots.length - 1] ?? null;
}
export type CompanionMissionsSnapshot = { run: MissionRunView | null; joinable: MissionRunView | null;
  completed: string[]; rewardReadyAt?: Record<string, number>; canUseCrew: boolean; ready: boolean; busy: boolean; error: string | null; serverOffset: number };
export const emptyCompanionMissions: CompanionMissionsSnapshot = { run: null, joinable: null, completed: [], rewardReadyAt: {}, canUseCrew: false, ready: false, busy: false, error: null, serverOffset: 0 };
export type MissionCommand = { type: "resume" } | { type: "start"; missionId: string; withCrew: boolean;
    pickup?: { instanceId: string; revision: number } }
  | { type: "join"; runId: string }
  | { type: "set-ready"; runId: string; readinessRevision: number; ready: boolean }
  | { type: "transfer-lead"; runId: string; revision: number; userId: string }
  | { type: "choose"; runId: string; revision: number; choiceId: string }
  | { type: "begin" | "advance" | "skip" | "retry" | "leave"; runId: string; revision: number };
export interface CompanionMissionsStore {
  getSnapshot(): CompanionMissionsSnapshot; subscribe(listener: () => void): () => void;
  select(gameRoomId: string | null): void; send(command: MissionCommand): Promise<void>; reload(): void; dispose(): void;
}
