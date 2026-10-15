import type { CompanionAppearance, PersonalRoom } from "./companion";
import { companionEmotes, type CompanionEmoteId } from "./companionEmotes";
import type { CharacterCommand } from "./companionExpression";
import type { CompanionStation } from "./companionLife";
import type { CompanionKitchenScope, CompanionKitchenSnapshot } from "./companionFood";
import type { CompanionWorldUtilityObservation } from "./companionWorldUtilities";
import { navigableStandingPoints, roomNavigationFor, routeSegmentClear } from "./companionNavigation";

export const availablePlayKinds = ["mirror", "mystery", "style", "kitchen", "memories"] as const;
// Keep legacy indices and saved-round decoding stable. Room toys are retired.
export const playKinds = ["playground", ...availablePlayKinds] as const;
export type PlayKind = typeof playKinds[number];
export type PlayPlace = "studio" | "terrace" | "town-square";
export const playNames: Record<PlayKind, string> = { playground: "Room toys", mirror: "Mirror me", mystery: "Room mystery", style: "Runway", kitchen: "Kitchen session", memories: "Leave a surprise" };
export const playBriefs = ["Midnight rooftop host", "Airport outfit after missing the flight", "Opening night", "Sunday at the record shop", "One colour, head to toe", "Unexpected dinner guest"];
export const playColours = ["#e7adbb", "#ede4d5", "#77856b", "#343340", "#b48755", "#809bac"];
export const playIngredients = ["Rice", "Tomato", "Pepper", "Beans", "Plantain", "Onion"];
export const playSymbols = ["Moon", "Leaf", "Wave", "Star", "Sun", "Cloud"];
export const PLAY_MEMBER_LIMIT = 8;
export const PLAY_THROW_MS = 1400;
export type PlayScope = { characterId: string; roomId: string; callSessionId?: string; invitationId?: string; roomRevision?: number };
export type PlayLook = { look: "rose" | "ivory"; topColor: string; trousersColor: string; footwearColor: string };
export type PlayPlayer = { id: string; name: string; characterId: string; appearance: CompanionAppearance; progress: number; score: number; look: PlayLook | null; vote: string | null; lastActionId: string; lastActionAt: number;
  mirrorWatchingAt?: number };
export type PlayGame =
  | { kind: "playground"; toy: "ball" | "cushion" | "music" | "bubbles"; hits: number; throws: number; notes: number[]; flight: { from: number; to: number; power: number; at: number; hit: boolean } | null; lastTouch?: { target: number; at: number; playerId: string } }
  | { kind: "mirror"; sequence: CompanionEmoteId[]; showingAt: number; level: number; leader: string | null; lastMove: { playerId: string; emote: CompanionEmoteId; at: number } | null }
  | { kind: "mystery"; order: number[]; found: number[]; entered: number[]; unlocked: boolean }
  | { kind: "style"; brief: number; phase: "dress" | "reveal"; revealedAt: number | null }
  | { kind: "kitchen"; recipe: number[]; ingredients: number[]; chopped: boolean; heatAt: number | null; cooked: boolean; plated: boolean; served: number; rush: boolean }
  | { kind: "memories"; anchor: number; moves: CompanionEmoteId[]; recording: boolean };
export type PlayRound = { id: string; revision: number; seed: number; ownerId: string; startedAt: number; updatedAt: number; expiresAt: number; players: PlayPlayer[]; game: PlayGame };
export type PlayMemory = { id: string; author: string; anchor: number; moves: CompanionEmoteId[]; createdAt: number };
export type PlayRoomUtilityReceipt = {
  serverNow: number; characterId: string; roomId: string; roomKey: string; roomRevision: number; invitationId: string | null;
  observation: CompanionWorldUtilityObservation | null; retryAfterMs: number;
};
export type PlaySnapshot = { round: PlayRound | null; playerId: string; memories: PlayMemory[]; busy: boolean; error: string | null; ready: boolean; serverOffset: number;
  roomUtility?: { receipt: PlayRoomUtilityReceipt; receivedAt: number } | null; utilityError?: string | null;
};
export const emptyPlaySnapshot: PlaySnapshot = { round: null, playerId: "", memories: [], busy: false, error: null, ready: false, serverOffset: 0 };
/** Retired and packed rounds do not occupy the room's activity slot. */
export function activePlayRound(round: PlayRound | null | undefined): PlayRound | null {
  return !round?.players.length || round.game.kind === "playground" ? null : round;
}
export type PlayCommand =
  | { type: "start"; kind: PlayKind; rush?: boolean }
  | { type: "join" } | { type: "leave" } | { type: "replay" }
  | { type: "toy"; toy: "ball" | "cushion" | "music" | "bubbles" }
  | { type: "touch"; target: number; power?: number }
  | { type: "emote"; emote: CompanionEmoteId }
  | { type: "lead" } | { type: "perform" } | { type: "watch" }
  | { type: "look"; look: PlayLook } | { type: "vote"; playerId: string }
  | { type: "ingredient"; ingredient: number }
  | { type: "chop" } | { type: "heat" } | { type: "plate" } | { type: "serve" }
  | { type: "save-memory" } | { type: "remove-memory"; memoryId: string };
export interface CompanionPlayStore {
  getSnapshot(): PlaySnapshot;
  subscribe(listener: () => void): () => void;
  select(scope: PlayScope | null): void;
  /** Only the visible solo-room host enables renewal. Calls use their own heartbeat. */
  setRoomUtilitiesActive?(active: boolean): void;
  observeRoomUtilities?(): Promise<void>;
  send(command: PlayCommand): Promise<void>;
  dispose(): void;
}
export type PlayPresentation = { snapshot: PlaySnapshot; room?: PersonalRoom; environmentId: PlayPlace; enabled: boolean; canCare?: boolean; careKinds?: readonly CompanionStation["kind"][]; canSaveMemory?: boolean;
  /** Saved furniture actions without adding minigame props to the city Home. */
  furnitureOnly?: boolean;
  /** Local admitted kitchen read. Never inferred from the room or a minigame score. */
  kitchen?: { scope: CompanionKitchenScope; snapshot: CompanionKitchenSnapshot } | null;
};
export type PlayChoice = { label: string; command: PlayCommand; selected?: boolean; disabled?: boolean; colour?: string };

export function playNeedsConfirmation(command: PlayCommand, hasRound: boolean) {
  return command.type === "leave" || command.type === "replay" || command.type === "save-memory" || command.type === "remove-memory" || command.type === "start" && hasRound;
}
export function playConfirmation(command: PlayCommand, round?: PlayRound | null, playerId?: string): { title: string; action: string; description?: string } {
  if (command.type === "save-memory") return { title: "Leave this surprise?", action: "Leave here" };
  if (command.type === "remove-memory") return { title: "Remove this surprise?", action: "Remove" };
  if (command.type === "start") return { title: `Start ${playNames[command.kind]}?`, action: "Start" };
  if (command.type === "replay") return { title: "Play another round?", action: "Play again" };
  if (round && round.ownerId !== playerId) return { title: "Leave this activity?", action: "Leave activity" };
  return { title: "Pack away this activity?", action: "Pack away",
    description: round && round.players.length > 1 ? "This ends the activity for everyone in this room." : undefined };
}

const record = (v: unknown): v is Record<string, unknown> => !!v && typeof v === "object" && !Array.isArray(v);
const member = <T extends string>(v: unknown, values: readonly T[]): v is T => typeof v === "string" && values.includes(v as T);
const index = (v: unknown, limit = 6): v is number => Number.isInteger(v) && Number(v) >= 0 && Number(v) < limit;
export function readPlayCommand(value: unknown): PlayCommand | null {
  if (!record(value)) return null;
  switch (value.type) {
    case "start": return member(value.kind, availablePlayKinds) && (value.rush === undefined || typeof value.rush === "boolean") ? { type: "start", kind: value.kind, ...(value.rush === undefined ? {} : { rush: value.rush }) } : null;
    case "join": case "leave": case "replay": case "lead": case "perform": case "watch": case "chop": case "heat": case "plate": case "serve": case "save-memory": return { type: value.type };
    case "touch": return index(value.target) && (value.power === undefined || typeof value.power === "number" && Number.isFinite(value.power) && value.power >= 0 && value.power <= 1) ? { type: "touch", target: value.target, ...(value.power === undefined ? {} : { power: value.power }) } : null;
    case "emote": return member(value.emote, companionEmotes.map(e => e.id)) ? { type: "emote", emote: value.emote } : null;
    case "ingredient": return index(value.ingredient) ? { type: "ingredient", ingredient: value.ingredient } : null;
    case "vote": return typeof value.playerId === "string" && value.playerId.length <= 100 ? { type: "vote", playerId: value.playerId } : null;
    case "remove-memory": return typeof value.memoryId === "string" && value.memoryId.length <= 100 ? { type: "remove-memory", memoryId: value.memoryId } : null;
    case "look": {
      const look = value.look;
      return record(look) && member(look.look, ["rose", "ivory"]) && [look.topColor, look.trousersColor, look.footwearColor].every(c => typeof c === "string" && playColours.includes(c))
        ? { type: "look", look: { look: look.look, topColor: String(look.topColor), trousersColor: String(look.trousersColor), footwearColor: String(look.footwearColor) } } : null;
    }
    default: return null;
  }
}

function shuffle(seed: number) {
  const values = [0, 1, 2, 3, 4, 5];
  let n = seed >>> 0;
  for (let i = 5; i > 0; i--) { n = (Math.imul(n, 1664525) + 1013904223) >>> 0; const j = n % (i + 1); [values[i], values[j]] = [values[j], values[i]]; }
  return values;
}
export function newPlayGame(kind: PlayKind, seed: number, now: number, rush = false): PlayGame {
  const order = shuffle(seed);
  switch (kind) {
    case "playground": throw new Error("This activity is no longer available.");
    case "mirror": return { kind, sequence: order.slice(0, 2).map(i => companionEmotes[i].id), showingAt: now, level: 1, leader: null, lastMove: null };
    case "mystery": return { kind, order: order.slice(0, 3), found: [], entered: [], unlocked: false };
    case "style": return { kind, brief: seed % playBriefs.length, phase: "dress", revealedAt: null };
    case "kitchen": return { kind, recipe: order.slice(0, 3), ingredients: [], chopped: false, heatAt: null, cooked: false, plated: false, served: 0, rush };
    case "memories": return { kind, anchor: order[0], moves: [], recording: true };
  }
}
export function createPlayRound(id: string, player: PlayPlayer, kind: PlayKind, seed: number, now: number, rush = false): PlayRound {
  return { id, revision: 1, seed, ownerId: player.id, startedAt: now, updatedAt: now, expiresAt: now + 2 * 60 * 60_000, players: [player], game: newPlayGame(kind, seed, now, rush) };
}
export function defaultPlayLook(): PlayLook { return { look: "rose", topColor: playColours[0], trousersColor: playColours[3], footwearColor: playColours[1] }; }
export function playAppearance(round: PlayRound | null, playerId: string): Partial<CompanionAppearance> {
  const player = round?.players.find(p => p.id === playerId);
  return round?.game.kind === "style" && player ? { ...(player.look ?? defaultPlayLook()), garmentStyle: "bomber", fabricTexture: "cotton", clothingPattern: "plain" } : {};
}
export function memoryExpression(memory: PlayMemory, startedAt: number, characterId: string, now: number): CharacterCommand | null {
  const index = Math.floor((now - startedAt) / 3000), value = memory.moves[index];
  if (!value || index < 0) return null;
  const issuedAt = startedAt + index * 3000, requestId = `memory.${memory.id}.${issuedAt}`;
  return { requestId, characterId, sessionId: `memory.${memory.id}`, turnId: requestId, action: "gesture", value, intensity: 1, durationMs: 2800, issuedAt, expiresAt: issuedAt + 2800 };
}
export function mirrorPlayback(game: Extract<PlayGame, { kind: "mirror" }>, player: PlayPlayer | undefined, now: number) {
  // A later round always supersedes this player's optional replay.
  const startedAt = Math.max(game.showingAt, player?.mirrorWatchingAt ?? game.showingAt);
  const index = Math.floor((now - startedAt) / 3000), until = startedAt + game.sequence.length * 3000;
  return { startedAt, until, index, watching: !game.leader && now < until,
    move: !game.leader && index >= 0 && index < game.sequence.length ? game.sequence[index] : null };
}
export function mirrorShowingUntil(game: Extract<PlayGame, { kind: "mirror" }>, player?: PlayPlayer) {
  return Math.max(game.showingAt, player?.mirrorWatchingAt ?? game.showingAt) + game.sequence.length * 3000;
}
export function kitchenFinished(game: Extract<PlayGame, { kind: "kitchen" }>) { return game.served >= 3; }

/** Shared deterministic rules. Auth, membership and time come from the host, never an event payload. */
export function advancePlayRound(saved: PlayRound, player: PlayPlayer, command: PlayCommand, actionId: string, now: number): PlayRound {
  if (!activePlayRound(saved)) throw new Error("This activity is no longer available.");
  if (saved.expiresAt <= now) throw new Error("This activity ended. Start another round.");
  if (saved.players.some(p => p.id === player.id && p.lastActionId === actionId)) return saved;
  const round: PlayRound = { ...saved, players: saved.players.map(p => ({ ...p })), game: JSON.parse(JSON.stringify(saved.game)) as PlayGame };
  let own = round.players.find(p => p.id === player.id);
  if (command.type === "join") {
    if (!own) { if (round.players.length >= PLAY_MEMBER_LIMIT) throw new Error("This activity is full."); own = { ...player }; round.players.push(own); }
  } else if (command.type === "leave") {
    // Packing away belongs to the host. Guests leave without ending others' game.
    round.players = round.ownerId === player.id ? [] : round.players.filter(p => p.id !== player.id);
    if (round.game.kind === "mirror" && round.game.leader === player.id) round.game.leader = null;
  } else {
    if (!own) throw new Error("Join this activity first.");
    if (command.type === "start" || command.type === "replay") {
      if (round.ownerId !== player.id) throw new Error("Only the activity host can change the round.");
      round.seed = (round.seed + 7919) >>> 0;
      round.id = actionId;
      round.expiresAt = now + 2 * 60 * 60_000;
      round.game = newPlayGame(command.type === "start" ? command.kind : round.game.kind, round.seed, now, command.type === "start" ? command.rush : round.game.kind === "kitchen" && round.game.rush);
      round.startedAt = now;
      round.players.forEach(p => { p.progress = 0; p.score = 0; p.look = null; p.vote = null; });
    } else {
      const game = round.game;
      if (game.kind === "playground") {
        if (command.type === "toy") { game.toy = command.toy; game.flight = null; delete game.lastTouch; }
        else if (command.type === "touch") {
          if (game.toy === "music") game.notes = [...game.notes.slice(-15), command.target];
          else if (game.toy === "bubbles") { own.score++; game.hits++; }
          else {
            if (game.flight && now - game.flight.at < PLAY_THROW_MS) throw new Error("Wait for the throw to land.");
            const power = command.power ?? .5;
            const target = (round.seed + game.throws) % 5 + 1;
            const hit = command.target === target && Math.abs(power - .55) <= .16;
            game.flight = { from: 0, to: command.target, power, at: now, hit };
            game.throws++; if (hit) { game.hits++; own.score++; }
          }
          game.lastTouch = { target: command.target, at: now, playerId: player.id };
        } else throw new Error("Choose a toy or tap it in the room.");
      } else if (game.kind === "mirror") {
        if (command.type === "lead") {
          if (game.leader && game.leader !== player.id) throw new Error("Wait for the other player to finish choosing moves.");
          game.leader = player.id; game.sequence = []; game.showingAt = now;
          round.players.forEach(p => { p.progress = 0; });
        } else if (command.type === "perform" && game.leader === player.id) {
          if (!game.sequence.length) throw new Error("Add a move first.");
          game.leader = null; game.showingAt = now;
        } else if (command.type === "watch") {
          if (game.leader || !game.sequence.length) throw new Error("Wait until the moves are ready.");
          if (now < mirrorShowingUntil(game, own)) throw new Error("Watch the moves first.");
          if (own.progress >= game.sequence.length) throw new Error("Wait for the other players to finish.");
          // Rewatching changes no scores, progress, moves or other players' clocks.
          own.mirrorWatchingAt = now;
        } else if (command.type === "emote") {
          if (game.leader) {
            if (game.leader !== player.id) throw new Error("The other player is choosing moves.");
            if (game.sequence.length >= 8) throw new Error("Eight moves are ready. Choose Show moves.");
            game.sequence.push(command.emote);
          } else {
            if (now < mirrorShowingUntil(game, own)) throw new Error("Watch the moves first.");
            if (own.progress >= game.sequence.length) throw new Error("Waiting for the other players.");
            if (command.emote === game.sequence[own.progress]) { own.progress++; own.score++; }
            else own.progress = 0;
            if (round.players.every(p => p.progress >= game.sequence.length)) {
              game.level++; game.sequence = [...game.sequence.slice(-7), companionEmotes[(round.seed + game.level) % 6].id];
              game.showingAt = now + 3000; round.players.forEach(p => { p.progress = 0; });
            }
          }
          game.lastMove = { playerId: player.id, emote: command.emote, at: now };
        } else throw new Error("Choose a move.");
      } else if (game.kind === "mystery" && command.type === "touch") {
        if (game.unlocked) throw new Error("The box is open. Play another round.");
        if (command.target < 3) { if (!game.found.includes(command.target)) { game.found.push(command.target); own.score++; } }
        else {
          if (game.found.length < 3) throw new Error("Find the three clues first.");
          const value = command.target - 3;
          // Each lock key represents the symbol on one discovered clue. Their order is shuffled per round.
          const expected = [...game.order].sort((a, b) => a - b)[game.entered.length];
          if (game.order[value] === expected) game.entered.push(value); else game.entered = [];
          if (game.entered.length === 3) { game.unlocked = true; own.score += 3; }
        }
      } else if (game.kind === "style") {
        if (command.type === "look" && game.phase === "dress") own.look = command.look;
        else if (command.type === "perform" && game.phase === "dress") {
          own.progress = 1;
          if (!own.look) own.look = defaultPlayLook();
          if (round.players.every(p => p.progress === 1)) { game.phase = "reveal"; game.revealedAt = now; }
        } else if (command.type === "vote" && game.phase === "reveal") {
          if (command.playerId === player.id || !round.players.some(p => p.id === command.playerId)) throw new Error("Choose another look.");
          own.vote = command.playerId;
        } else throw new Error("That runway action is unavailable.");
      } else if (game.kind === "kitchen") {
        if (kitchenFinished(game)) throw new Error("All three plates are served. Start another session.");
        if (game.rush && now > round.startedAt + 180_000) throw new Error("Time is up. Try the rush again or choose relaxed cooking.");
        if (command.type === "ingredient" && !game.chopped) {
          if (game.ingredients.includes(command.ingredient)) game.ingredients = game.ingredients.filter(i => i !== command.ingredient);
          else if (game.ingredients.length < 3) game.ingredients.push(command.ingredient);
        } else if (command.type === "chop" && !game.chopped) {
          if (game.ingredients.length !== 3 || !game.recipe.every(i => game.ingredients.includes(i))) throw new Error("Check the ingredients against the order.");
          game.chopped = true; own.score++;
        } else if (command.type === "heat" && game.chopped && !game.cooked) {
          if (game.heatAt === null) game.heatAt = now;
          else {
            const elapsed = now - game.heatAt;
            if (elapsed < 4000) throw new Error("It needs a little longer on the heat.");
            if (elapsed > 12000) game.heatAt = now;
            else { game.cooked = true; own.score++; }
          }
        } else if (command.type === "plate" && game.cooked && !game.plated) { game.plated = true; own.score++; }
        else if (command.type === "serve" && game.plated) {
          game.served++; own.score++;
          if (!kitchenFinished(game)) { game.recipe = shuffle(round.seed + game.served).slice(0, 3); game.ingredients = []; game.chopped = false; game.heatAt = null; game.cooked = false; game.plated = false; }
        } else throw new Error("Finish the previous cooking step first.");
      } else if (game.kind === "memories") {
        if (!game.recording) throw new Error("Start another round to record new moves.");
        if (command.type === "touch") game.anchor = command.target;
        else if (command.type === "emote") { if (game.moves.length >= 8) throw new Error("Eight moves are ready to leave."); game.moves.push(command.emote); }
        else throw new Error("Choose a spot or add a move.");
      } else throw new Error("That action is unavailable for this activity.");
    }
  }
  if (command.type === "leave" && round.game.kind === "mirror" && !round.game.leader) {
    const game = round.game;
    if (!game.sequence.length) round.game = newPlayGame("mirror", round.seed, now);
    else if (round.players.length && round.players.every(p => p.progress >= game.sequence.length)) {
      game.level++;
      game.sequence = [...game.sequence.slice(-7), companionEmotes[(round.seed + game.level) % 6].id];
      game.showingAt = now; round.players.forEach(p => { p.progress = 0; });
    }
  }
  if (own) { own.lastActionId = actionId; own.lastActionAt = now; }
  round.revision++; round.updatedAt = now;
  return round;
}

export function validPlayMemory(value: unknown): value is PlayMemory {
  return record(value) && typeof value.id === "string" && value.id.length <= 100 && typeof value.author === "string" && value.author.length <= 100
    && index(value.anchor) && Array.isArray(value.moves) && value.moves.length > 0 && value.moves.length <= 8 && value.moves.every(m => member(m, companionEmotes.map(e => e.id)))
    && typeof value.createdAt === "number" && Number.isFinite(value.createdAt);
}

/** Stable clear-floor locations; games never mutate furniture or its collision map. */
export function playAnchors(room?: PersonalRoom, environmentId: PlayPlace = room?.environmentId ?? "studio") {
  const nav = roomNavigationFor(room, environmentId);
  const points = navigableStandingPoints(room, environmentId).map(p => [p.x, 0, p.z] as const);
  const clear: Array<readonly [number, number, number]> = [];
  for (const p of points) if (clear.every(q => Math.hypot(p[0] - q[0], p[2] - q[2]) > .38)) clear.push(p);
  if (clear.length < 6) for (let z = nav.bounds.maxZ - nav.radius; z >= nav.bounds.minZ + nav.radius && clear.length < 6; z -= .45) {
    for (let x = nav.bounds.minX + nav.radius; x <= nav.bounds.maxX - nav.radius && clear.length < 6; x += .45) {
      const p = [x, 0, z] as const;
      if (routeSegmentClear(nav, p, p) && clear.every(q => Math.hypot(x - q[0], z - q[2]) > .4)) clear.push(p);
    }
  }
  return clear.slice(0, 6);
}

export function playStatus(round: PlayRound, playerId: string, now: number): string {
  const game = round.game, own = round.players.find(p => p.id === playerId);
  if (game.kind === "playground") return "";
  if (!own) return `Join ${playNames[game.kind]}`;
  switch (game.kind) {
    case "mirror": return game.leader ? game.leader === playerId
      ? game.sequence.length >= 8 ? "Your moves are ready. Choose Show moves." : "Choose the moves you want to show."
      : `${round.players.find(p => p.id === game.leader)?.name ?? "The other player"} is choosing moves.`
      : now < mirrorShowingUntil(game, own) ? "Watch these moves."
      : own.progress >= game.sequence.length ? "Wait for the other players to finish."
      : own.progress ? `Repeat move ${own.progress + 1} of ${game.sequence.length}.` : "Your turn. Repeat the moves in the same order.";
    case "mystery": return game.unlocked ? "Mystery solved" : game.found.length < 3 ? `${game.found.length}/3 clues found` : `Unlock the box · ${game.entered.length}/3`;
    case "style": return game.phase === "dress" ? playBriefs[game.brief] : "Runway reveal";
    case "kitchen": return kitchenFinished(game) ? "Three plates served" : game.rush && now > round.startedAt + 180_000 ? "Time is up" : game.plated ? "Ready to serve" : game.cooked ? "Plate the meal" : game.heatAt !== null ? `On the heat · ${Math.max(0, Math.floor((now - game.heatAt) / 1000))}s` : game.chopped ? "Ready for the pan" : `Order ${game.served + 1} · ${game.recipe.map(i => playIngredients[i]).join(", ")}`;
    case "memories": return game.recording ? `${game.moves.length}/8 moves · Spot ${game.anchor + 1}` : "Surprise saved";
  }
}
export function playChoices(round: PlayRound, playerId: string, now: number): PlayChoice[] {
  const own = round.players.find(p => p.id === playerId), game = round.game;
  if (game.kind === "playground") return [];
  if (!own) return [{ label: "Join activity", command: { type: "join" } }];
  switch (game.kind) {
    case "mirror": return [
      ...companionEmotes.map(e => ({ label: e.label, command: { type: "emote", emote: e.id } as PlayCommand, disabled: game.leader ? game.leader !== playerId || game.sequence.length >= 8 : now < mirrorShowingUntil(game, own) || own.progress >= game.sequence.length })),
      { label: "Watch again", command: { type: "watch" }, disabled: !!game.leader || now < mirrorShowingUntil(game, own) || own.progress >= game.sequence.length },
      game.leader === playerId ? { label: "Show moves", command: { type: "perform" }, disabled: !game.sequence.length } : { label: "Choose moves", command: { type: "lead" }, disabled: !!game.leader },
    ];
    case "mystery": return game.unlocked ? [] : game.found.length < 3 ? [0, 1, 2].map(target => ({ label: game.found.includes(target) ? `Clue ${target + 1}: ${playSymbols[game.order[target]]}, ${game.order[target] + 1}` : `Find clue ${target + 1}`, command: { type: "touch", target }, disabled: game.found.includes(target) })) : game.order.map((symbol, i) => ({ label: `${playSymbols[symbol]} · ${symbol + 1}`, command: { type: "touch", target: i + 3 } }));
    case "style": {
      if (game.phase === "reveal") return round.players.filter(p => p.id !== playerId).map(p => ({ label: p.name, command: { type: "vote", playerId: p.id }, selected: own.vote === p.id }));
      const look = own.look ?? defaultPlayLook();
      return [
        ...playColours.map((colour, i) => ({ label: `Look ${i + 1}`, colour, command: { type: "look", look: { ...look, topColor: colour, trousersColor: playColours[(i + 3) % 6], footwearColor: playColours[(i + 1) % 6] } } as PlayCommand, selected: own.look?.topColor === colour })),
        { label: look.look === "rose" ? "Ivory jacket" : "Rose jacket", command: { type: "look", look: { ...look, look: look.look === "rose" ? "ivory" : "rose" } } },
        { label: own.progress ? "Ready" : "Reveal my look", command: { type: "perform" }, disabled: own.progress === 1 },
      ];
    }
    case "kitchen":
      if (kitchenFinished(game) || game.rush && now > round.startedAt + 180_000) return [];
      if (game.plated) return [{ label: "Serve", command: { type: "serve" } }];
      if (game.cooked) return [{ label: "Plate", command: { type: "plate" } }];
      if (game.chopped) return [{ label: game.heatAt === null ? "Start cooking" : now - game.heatAt > 12000 ? "Restart pan" : "Take off heat", command: { type: "heat" } }];
      return [...playIngredients.map((label, ingredient) => ({ label, command: { type: "ingredient", ingredient } as PlayCommand, selected: game.ingredients.includes(ingredient) })), { label: "Chop", command: { type: "chop" } }];
    case "memories": return game.recording ? [...companionEmotes.map(e => ({ label: e.label, command: { type: "emote", emote: e.id } as PlayCommand, disabled: game.moves.length >= 8 })), { label: "Leave here", command: { type: "save-memory" }, disabled: !game.moves.length }] : [];
  }
}
export function playExpression(round: PlayRound | null, playerId: string, characterId: string, now: number, serverOffset = 0): CharacterCommand | null {
  if (!round || round.expiresAt <= now) return null;
  const game = round.game;
  let emote: CompanionEmoteId | undefined, at = 0;
  if (game.kind === "mirror") {
    const playback = mirrorPlayback(game, round.players.find(player => player.id === playerId), now);
    if (playback.move) { emote = playback.move; at = playback.startedAt + playback.index * 3000; }
    else if (game.lastMove?.playerId === playerId && now - game.lastMove.at < 2800) { emote = game.lastMove.emote; at = game.lastMove.at; }
  } else if (game.kind === "style" && game.phase === "reveal" && game.revealedAt !== null) { emote = "cheer"; at = game.revealedAt; }
  else if (game.kind === "memories" && game.moves.length) { emote = game.moves[game.moves.length - 1]; at = round.updatedAt; }
  if (!emote || now < at || now - at >= 2800) return null;
  const requestId = `play.${round.id}.${at}.${emote}`;
  return { requestId, characterId, sessionId: `play.${round.id}`, turnId: requestId, action: "gesture", value: emote, intensity: 1, durationMs: 2800, issuedAt: at - serverOffset, expiresAt: at + 2800 - serverOffset };
}
