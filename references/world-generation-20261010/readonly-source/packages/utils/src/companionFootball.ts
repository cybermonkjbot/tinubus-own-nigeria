/** Local metres. The venue owns these resolved coordinates, never a client. */
export type FootballPoint = { x: number; z: number };
export type FootballSide = "home" | "away";
export type FootballPitch = {
  id: string; instanceId: string; revision: number; floorY: number;
  centre: FootballPoint; halfWidth: number; halfLength: number;
  goalHalfWidth: number; goalHeight: number;
};
export type FootballAction = "none" | "pass" | "shoot" | "challenge";
export type FootballInput = {
  sequence: number; move: FootballPoint; aim: FootballPoint;
  destination: FootballPoint | null; action: FootballAction;
  targetPlayerId: string | null; power: number;
  /** Focus loss cancels actions that the shared driver has not consumed yet. */
  cancelActions?: boolean;
};
export type FootballPlayer = {
  id: string; side: FootballSide; bot: boolean; goalkeeper: boolean;
  position: FootballPoint; velocity: FootballPoint; yaw: number;
  ready: boolean; connected: boolean; lastSequence: number; actionAt: number;
  goals: number; assists: number;
};
export type FootballBall = {
  position: FootballPoint; velocity: FootballPoint; height: number; verticalSpeed: number;
  ownerId: string | null; lastTouchId: string | null; lastPassId: string | null;
  claimAfter: number;
};
export type FootballState = {
  id: string; epoch: number; revision: number; pitch: FootballPitch;
  mode: "solo" | "teams" | "coop"; phase: "lobby" | "playing" | "restart" | "finished" | "cancelled";
  players: FootballPlayer[]; ball: FootballBall;
  score: { home: number; away: number }; winner: FootballSide | "draw" | null;
  startedAt: number | null; endsAt: number | null; simulatedAt: number;
  restartAt: number | null; restartSide: FootballSide;
  objective: { goalsToWin: number; durationMs: number };
  lastEvent: { id: number; kind: "kickoff" | "goal" | "out" | "save" | "finished" | "cancelled"; side: FootballSide | null; playerId: string | null } | null;
};
export type FootballSeat = { id: string; side: FootballSide; bot?: boolean; goalkeeper?: boolean };
export type FootballTimedInput = { playerId: string; input: FootballInput; receivedAt: number; actionReceivedAt?: number; previous?: { input: FootballInput; receivedAt: number } };
export type FootballCommand = { type: "ready"; ready: boolean } | { type: "side"; side: FootballSide } | { type: "start" | "retry" | "rejoin" | "leave" | "end" };
export type FootballAdvancement = { ownerId: string | null; ownerUntil: number; nextAt: number | null; serverNow: number };
export type FootballSnapshot = {
  playerId: string | null; host: boolean; callSessionId: string | null;
  match: FootballState | null; ready: boolean; busy: boolean; error: string | null;
  advancement?: FootballAdvancement;
};
export type FootballEntryRoster = {
  playerId: string; instanceId: string; revision: number; epoch: number;
  phase: FootballState["phase"] | null; serverNow: number;
  players: Array<{ id: string; name: string; side: FootballSide | null }>;
};
export type FootballEntrySelection = {
  instanceId: string; revision: number; epoch: number; requestId: string;
  mode: FootballState["mode"]; players: Array<{ id: string; side: FootballSide }>;
};
export interface CompanionFootballStore {
  getSnapshot(): FootballSnapshot;
  subscribe(listener: () => void): () => void;
  input(input: Omit<FootballInput, "sequence">): void;
  /** Foreground gameplay only. It never controls or ends the account call. */
  setActive(active: boolean): void;
  /** Visible foreground match, including chat overlays. Separate from input focus. */
  setForeground(foreground: boolean): void;
  stop(): void;
  send(command: FootballCommand): Promise<void>;
  subscribeEntry(listener: (roster: FootballEntryRoster) => void, onError: (error: unknown) => void): () => void;
  open(selection: FootballEntrySelection): Promise<void>;
  reload(): void;
  dispose(): void;
}
export const emptyFootballSnapshot: FootballSnapshot = { playerId: null, host: false, callSessionId: null, match: null, ready: false, busy: false, error: null };
export const footballLimits = { players: 12, tickMs: 200, inputMs: 200, inputRenewMs: 400, driverLeaseMs: 1200, inputExpiryMs: 650, stepMs: 40, catchupMs: 1000, playerRadius: .42, ballRadius: .16, speed: 5.2 } as const;
export const idleFootballInput: Omit<FootballInput, "sequence"> = { move: { x: 0, z: 0 }, aim: { x: 0, z: -1 }, destination: null, action: "none", targetPlayerId: null, power: .7 };
export function footballObjective(state: FootballState): string {
  if (state.phase === "cancelled") return "Match ended";
  if (state.phase === "finished") return state.winner === "draw" ? "Draw" : state.mode === "solo" ? state.winner === "home" ? "Challenge complete" : "Time's up" : state.winner === "home" ? "Home wins" : "Away wins";
  return state.mode === "solo" ? `Score ${state.objective.goalsToWin} goals` : `First to ${state.objective.goalsToWin}`;
}
