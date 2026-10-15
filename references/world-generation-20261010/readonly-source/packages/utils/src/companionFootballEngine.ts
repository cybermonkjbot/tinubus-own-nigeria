import { footballLimits, type FootballBall, type FootballInput, type FootballPitch, type FootballPlayer, type FootballPoint, type FootballSeat, type FootballSide, type FootballState, type FootballTimedInput } from "./companionFootball";
import { footballNpcInputs, footballNpcRoster, footballNpcsNeedAdvance } from "./companionFootballNpcs";

const clamp = (n: number, low: number, high: number) => Math.max(low, Math.min(high, n));
const length = (p: FootballPoint) => Math.hypot(p.x, p.z);
const distance = (a: FootballPoint, b: FootballPoint) => Math.hypot(a.x - b.x, a.z - b.z);
const direction = (from: FootballPoint, to: FootballPoint) => unit({ x: to.x - from.x, z: to.z - from.z });
const other = (side: FootballSide): FootballSide => side === "home" ? "away" : "home";
const zero = (): FootballPoint => ({ x: 0, z: 0 });
function unit(p: FootballPoint): FootballPoint { const n = length(p); return n > .001 ? { x: p.x / n, z: p.z / n } : zero(); }
function copy(state: FootballState): FootballState {
  return { ...state, pitch: { ...state.pitch, centre: { ...state.pitch.centre } }, objective: { ...state.objective }, score: { ...state.score },
    players: state.players.map(p => ({ ...p, position: { ...p.position }, velocity: { ...p.velocity } })),
    ball: { ...state.ball, position: { ...state.ball.position }, velocity: { ...state.ball.velocity } }, lastEvent: state.lastEvent ? { ...state.lastEvent } : null };
}
export function validFootballPitch(p: FootballPitch): boolean {
  return !!p.id && p.id.length <= 160 && !!p.instanceId && p.instanceId.length <= 160
    && [p.floorY, p.centre.x, p.centre.z, p.halfWidth, p.halfLength, p.goalHalfWidth, p.goalHeight].every(Number.isFinite)
    && Number.isSafeInteger(p.revision) && p.revision >= 0 && p.halfWidth >= 6 && p.halfWidth <= 40
    && p.halfLength >= 10 && p.halfLength <= 65 && p.goalHalfWidth >= 1 && p.goalHalfWidth <= p.halfWidth / 2
    && p.goalHeight >= 1 && p.goalHeight <= 4;
}
export function validFootballInput(input: FootballInput, pitch: FootballPitch): boolean {
  const points = [input.move, input.aim, ...(input.destination ? [input.destination] : [])];
  return Number.isSafeInteger(input.sequence) && input.sequence >= 1 && input.sequence <= Number.MAX_SAFE_INTEGER
    && points.every(p => Number.isFinite(p.x) && Number.isFinite(p.z)) && length(input.move) <= 1.01 && length(input.aim) <= 1.01
    && Number.isFinite(input.power) && input.power >= .1 && input.power <= 1
    && (input.cancelActions === undefined || typeof input.cancelActions === "boolean")
    && (!input.cancelActions || input.action === "none" && !input.destination && length(input.move) === 0 && input.targetPlayerId === null)
    && ["none", "pass", "shoot", "challenge"].includes(input.action) && (input.targetPlayerId === null || input.targetPlayerId.length <= 160)
    && (!input.destination || Math.abs(input.destination.x - pitch.centre.x) <= pitch.halfWidth && Math.abs(input.destination.z - pitch.centre.z) <= pitch.halfLength);
}
function centreBall(pitch: FootballPitch): FootballBall {
  return { position: { ...pitch.centre }, velocity: zero(), height: footballLimits.ballRadius, verticalSpeed: 0,
    ownerId: null, lastTouchId: null, lastPassId: null, claimAfter: 0 };
}
function spawn(pitch: FootballPitch, side: FootballSide, ordinal: number, goalkeeper: boolean): FootballPoint {
  const sign = side === "home" ? 1 : -1;
  return { x: pitch.centre.x + (goalkeeper || ordinal === 0 ? 0 : ordinal % 2 ? -1 : 1) * Math.min(pitch.halfWidth * .6, 2 + ordinal * 1.2),
    z: pitch.centre.z + sign * (goalkeeper ? pitch.halfLength - 1.1 : ordinal === 0 ? 1.2 : pitch.halfLength * .4) };
}
export function createFootballMatch(id: string, pitch: FootballPitch, seats: readonly FootballSeat[], mode: FootballState["mode"], now: number, epoch = 1): FootballState {
  if (!validFootballPitch(pitch) || !Number.isFinite(now) || !id || seats.length < 1 || seats.length > footballLimits.players
    || new Set(seats.map(s => s.id)).size !== seats.length || seats.some(s => !s.id || s.id.length > 160 || !["home", "away"].includes(s.side))) throw new Error("Invalid football roster or pitch.");
  if (mode === "solo") {
    if (seats.length !== 1 || seats[0].side !== "home" || seats[0].bot || seats[0].goalkeeper) throw new Error("Solo football needs one outfield player.");
  } else if (mode === "coop") {
    if (seats.length > footballLimits.players - 4 || seats.some(s => s.side !== "home" || s.bot || s.goalkeeper))
      throw new Error("Co-op football needs up to eight admitted players on Home.");
  } else if (!seats.some(s => s.side === "home" && !s.bot) || !seats.some(s => s.side === "away" && !s.bot)) throw new Error("Both teams need a player.");
  const roster = footballNpcRoster(id, seats);
  const counts = { home: 0, away: 0 };
  return { id, epoch, revision: 1, pitch: { ...pitch, centre: { ...pitch.centre } }, mode, phase: "lobby",
    players: roster.map(s => ({ id: s.id, side: s.side, bot: !!s.bot, goalkeeper: !!s.goalkeeper,
      position: spawn(pitch, s.side, counts[s.side]++, !!s.goalkeeper), velocity: zero(), yaw: s.side === "home" ? Math.PI : 0,
      ready: !!s.bot, connected: true, lastSequence: 0, actionAt: -1000, goals: 0, assists: 0 })),
    ball: centreBall(pitch), score: { home: 0, away: 0 }, winner: null, startedAt: null, endsAt: null, simulatedAt: now,
    restartAt: null, restartSide: "home", objective: { goalsToWin: mode === "solo" ? 3 : 5, durationMs: mode === "solo" ? 120000 : 180000 }, lastEvent: null };
}
function event(state: FootballState, kind: NonNullable<FootballState["lastEvent"]>["kind"], side: FootballSide | null, playerId: string | null) {
  state.lastEvent = { id: (state.lastEvent?.id ?? 0) + 1, kind, side, playerId };
}
function kickoff(state: FootballState, now: number) {
  state.phase = "playing"; state.restartAt = null; state.ball = centreBall(state.pitch);
  const counts = { home: 0, away: 0 };
  for (const p of state.players) {
    p.position = spawn(state.pitch, p.side, counts[p.side]++, p.goalkeeper); p.velocity = zero(); p.yaw = p.side === "home" ? Math.PI : 0;
  }
  const player = state.players.find(p => p.connected && !p.bot && p.side === state.restartSide && !p.goalkeeper)
    ?? state.players.find(p => p.connected && p.side === state.restartSide && !p.goalkeeper);
  if (player) { player.position = { x: state.pitch.centre.x, z: state.pitch.centre.z + (player.side === "home" ? .65 : -.65) }; state.ball.ownerId = player.id; state.ball.lastTouchId = player.id; }
  state.ball.claimAfter = now; event(state, "kickoff", state.restartSide, player?.id ?? null);
}
export function startFootballMatch(source: FootballState, now: number): FootballState {
  if (source.phase !== "lobby") return source;
  if (source.mode === "teams" && ["home", "away"].some(side => !source.players.some(p => !p.bot && p.connected && p.side === side))) throw new Error("Both teams need a player.");
  if (source.players.some(p => !p.bot && (!p.ready || !p.connected))) throw new Error("Wait for everyone to be ready.");
  const state = copy(source); state.startedAt = now; state.endsAt = now + state.objective.durationMs; state.simulatedAt = now;
  kickoff(state, now); state.revision++; return state;
}
export function retryFootballMatch(source: FootballState, now: number): FootballState {
  if (!["finished", "cancelled"].includes(source.phase)) throw new Error("Finish the current match before retrying.");
  return createFootballMatch(source.id, source.pitch, source.players.filter(p => !p.bot).map(p => ({ id: p.id, side: p.side, goalkeeper: p.goalkeeper })), source.mode, now, source.epoch + 1);
}
function finish(state: FootballState) {
  state.phase = "finished"; state.restartAt = null; state.ball.ownerId = null; state.ball.velocity = zero();
  state.ball.verticalSpeed = 0; state.players.forEach(p => { p.velocity = zero(); });
  state.winner = state.mode === "solo" ? state.score.home >= state.objective.goalsToWin ? "home" : "away"
    : state.score.home === state.score.away ? "draw" : state.score.home > state.score.away ? "home" : "away";
  event(state, "finished", state.winner === "draw" ? null : state.winner, null);
}
export function cancelFootballMatch(source: FootballState): FootballState {
  if (["finished", "cancelled"].includes(source.phase)) return source;
  const state = copy(source); state.phase = "cancelled"; state.restartAt = null; state.ball.ownerId = null; state.ball.velocity = zero();
  state.players.forEach(p => { p.velocity = zero(); }); event(state, "cancelled", null, null); state.revision++; return state;
}
function keepInside(state: FootballState, p: FootballPlayer) {
  const { centre: c, halfWidth: w, halfLength: l } = state.pitch, r = footballLimits.playerRadius;
  p.position.x = clamp(p.position.x, c.x - w + r, c.x + w - r); p.position.z = clamp(p.position.z, c.z - l + r, c.z + l - r);
}
function movePlayer(state: FootballState, p: FootballPlayer, input: FootballInput | null, dt: number) {
  if (!p.connected) { p.velocity = zero(); return; }
  let move = input?.move ?? zero();
  const speed = p.goalkeeper && p.bot ? 3.6 : footballLimits.speed;
  if (input?.destination && length(move) < .01) {
    const d = distance(p.position, input.destination);
    move = d > (p.bot && p.goalkeeper ? .08 : .1) ? { x: (input.destination.x - p.position.x) / Math.max(d, speed * dt), z: (input.destination.z - p.position.z) / Math.max(d, speed * dt) } : zero();
  }
  const n = Math.max(1, length(move));
  p.velocity = { x: move.x / n * speed, z: move.z / n * speed }; p.position.x += p.velocity.x * dt; p.position.z += p.velocity.z * dt;
  if (length(p.velocity) > .05) p.yaw = Math.atan2(p.velocity.x, p.velocity.z);
  keepInside(state, p);
}
function separatePlayers(state: FootballState) {
  for (let i = 0; i < state.players.length; i++) for (let j = i + 1; j < state.players.length; j++) {
    const a = state.players[i], b = state.players[j]; if (!a.connected || !b.connected) continue;
    const d = distance(a.position, b.position), radius = footballLimits.playerRadius * 2;
    if (d >= radius) continue;
    const n = d > .001 ? direction(b.position, a.position) : { x: 1, z: 0 }, push = (radius - d) / 2;
    a.position.x += n.x * push; a.position.z += n.z * push; b.position.x -= n.x * push; b.position.z -= n.z * push;
    keepInside(state, a); keepInside(state, b);
  }
}
function kick(state: FootballState, p: FootballPlayer, input: FootballInput, now: number) {
  if (now - p.actionAt < 350 || state.ball.ownerId !== p.id || !["pass", "shoot"].includes(input.action)) return;
  let aim = length(input.aim) > .05 ? unit(input.aim) : { x: Math.sin(p.yaw), z: Math.cos(p.yaw) };
  const mate = input.action === "pass" ? state.players.find(other => other.id === input.targetPlayerId && other.id !== p.id && other.side === p.side && other.connected) : undefined;
  if (mate) aim = direction(p.position, mate.position);
  const speed = input.action === "shoot" ? 11 + input.power * 9 : 7 + input.power * 5;
  state.ball.position = { x: p.position.x + aim.x * .7, z: p.position.z + aim.z * .7 };
  state.ball.velocity = { x: aim.x * speed, z: aim.z * speed }; state.ball.height = footballLimits.ballRadius;
  state.ball.verticalSpeed = input.action === "shoot" ? 1.2 + input.power * 2.4 : 0;
  state.ball.ownerId = null; state.ball.lastTouchId = p.id; state.ball.lastPassId = input.action === "pass" ? p.id : state.ball.lastPassId;
  state.ball.claimAfter = now + 180; p.actionAt = now;
}
function claimBall(state: FootballState, now: number, inputs: Map<string, FootballInput>) {
  const ball = state.ball;
  if (ball.ownerId) {
    const owner = state.players.find(p => p.id === ball.ownerId && p.connected);
    if (!owner) ball.ownerId = null;
    else {
      const challenger = state.players.find(p => p.connected && p.side !== owner.side && inputs.get(p.id)?.action === "challenge"
        && (inputs.get(p.id)?.sequence ?? 0) > p.lastSequence && now - p.actionAt >= 650 && distance(p.position, owner.position) < 1.1);
      if (challenger) { ball.ownerId = challenger.id; ball.lastTouchId = challenger.id; ball.lastPassId = null; challenger.actionAt = now; }
      return;
    }
  }
  if (now < ball.claimAfter || ball.height > 1.05) return;
  const closest = state.players.filter(p => p.connected && distance(p.position, ball.position) < (p.goalkeeper ? 1.05 : .7)
    && (p.goalkeeper || length(ball.velocity) < 14)).sort((a, b) => distance(a.position, ball.position) - distance(b.position, ball.position))[0];
  if (!closest) return;
  const prior = state.players.find(p => p.id === ball.lastTouchId);
  if (prior?.side !== closest.side) ball.lastPassId = null;
  ball.ownerId = closest.id; ball.lastTouchId = closest.id; ball.velocity = zero(); ball.height = footballLimits.ballRadius; ball.verticalSpeed = 0;
  if (closest.goalkeeper) { ball.ownerId = null; ball.velocity = { x: (ball.position.x - state.pitch.centre.x) * .5, z: closest.side === "home" ? -9 : 9 }; ball.claimAfter = now + 350; event(state, "save", closest.side, closest.id); }
}
function scoreGoal(state: FootballState, side: FootballSide, now: number) {
  state.score[side]++;
  const scorer = state.players.find(p => p.id === state.ball.lastTouchId);
  if (scorer?.side === side) scorer.goals++;
  const assist = state.players.find(p => p.id === state.ball.lastPassId && p.id !== scorer?.id && p.side === side);
  if (assist) assist.assists++;
  event(state, "goal", side, scorer?.side === side ? scorer.id : null);
  state.ball.ownerId = null; state.ball.velocity = zero(); state.ball.verticalSpeed = 0;
  state.players.forEach(p => { p.velocity = zero(); });
  if (state.score[side] >= state.objective.goalsToWin) finish(state);
  else { state.phase = "restart"; state.restartAt = now + 1400; state.restartSide = other(side); if (state.mode === "solo") state.restartSide = "home"; }
}
function advanceBall(state: FootballState, dt: number, now: number) {
  const ball = state.ball, old = { ...ball.position }, oldHeight = ball.height;
  const owner = state.players.find(p => p.id === ball.ownerId && p.connected);
  if (owner) {
    ball.position = { x: owner.position.x + Math.sin(owner.yaw) * .65, z: owner.position.z + Math.cos(owner.yaw) * .65 };
    ball.velocity = { ...owner.velocity }; ball.height = footballLimits.ballRadius; ball.verticalSpeed = 0;
  } else {
    ball.position.x += ball.velocity.x * dt; ball.position.z += ball.velocity.z * dt;
    ball.height += ball.verticalSpeed * dt; ball.verticalSpeed -= 9.81 * dt;
    if (ball.height <= footballLimits.ballRadius) { ball.height = footballLimits.ballRadius; ball.verticalSpeed = Math.abs(ball.verticalSpeed) > 1 ? -ball.verticalSpeed * .22 : 0; }
    const drag = Math.exp(-(ball.height <= footballLimits.ballRadius ? .62 : .12) * dt); ball.velocity.x *= drag; ball.velocity.z *= drag;
    if (length(ball.velocity) < .08) ball.velocity = zero();
  }
  const p = state.pitch, r = footballLimits.ballRadius;
  // Local posts and crossbar use the venue's clear mouth dimensions. Reflect a
  // swept contact before goal detection so a fast ball cannot skip the frame.
  for (const sign of [-1, 1]) {
    const line = p.centre.z + sign * p.halfLength, dz = ball.position.z - old.z;
    if (Math.abs(dz) < .0001 || (old.z - line) * (ball.position.z - line) > 0) continue;
    const t = clamp((line - old.z) / dz, 0, 1), x = old.x + (ball.position.x - old.x) * t, y = oldHeight + (ball.height - oldHeight) * t;
    const post = Math.abs(Math.abs(x - p.centre.x) - (p.goalHalfWidth + .065)) < r + .065 && y <= p.goalHeight + .13;
    const crossbar = Math.abs(x - p.centre.x) <= p.goalHalfWidth + .13 && Math.abs(y - (p.goalHeight + .065)) < r + .065;
    if (!post && !crossbar) continue;
    ball.position = { x, z: line - Math.sign(dz) * (r + .07) }; ball.velocity.z = -ball.velocity.z * .55;
    ball.velocity.x *= .8; ball.ownerId = null; ball.claimAfter = now + 100;
    if (crossbar) ball.verticalSpeed = -Math.abs(ball.verticalSpeed) * .55;
    return;
  }
  for (const side of ["home", "away"] as const) {
    const sign = side === "home" ? -1 : 1, line = p.centre.z + sign * (p.halfLength + r);
    if ((old.z - line) * sign <= 0 && (ball.position.z - line) * sign > 0) {
      const t = (line - old.z) / (ball.position.z - old.z), x = old.x + (ball.position.x - old.x) * t, y = oldHeight + (ball.height - oldHeight) * t;
      if (Math.abs(x - p.centre.x) <= p.goalHalfWidth - r && y <= p.goalHeight - r) { scoreGoal(state, side, now); return; }
    }
  }
  if (Math.abs(ball.position.x - p.centre.x) > p.halfWidth + r || Math.abs(ball.position.z - p.centre.z) > p.halfLength + r) {
    const toucher = state.players.find(p => p.id === ball.lastTouchId);
    state.restartSide = state.mode === "solo" ? "home" : toucher ? other(toucher.side) : "home";
    state.phase = "restart"; state.restartAt = now + 800; ball.velocity = zero(); ball.ownerId = null; event(state, "out", state.restartSide, null);
  }
}
/** The next server event. A settled pitch needs no simulation heartbeat. */
export function footballNextAdvanceAt(state: FootballState, commands: readonly FootballTimedInput[], now: number): number | null {
  if (!["playing", "restart"].includes(state.phase)) return null;
  const deadline = state.endsAt ?? Infinity;
  if (state.phase === "restart") return Math.min(deadline, state.restartAt ?? deadline);
  const moving = length(state.ball.velocity) > 0 || state.ball.height > footballLimits.ballRadius || state.ball.verticalSpeed !== 0
    || state.players.some(p => p.connected && length(p.velocity) > 0)
    || footballNpcsNeedAdvance(state, now)
    || commands.some(c => now - c.receivedAt <= footballLimits.inputExpiryMs && state.players.some(p => p.id === c.playerId && p.connected
      && (length(c.input.move) > 0 || c.input.destination !== null || c.input.action !== "none" && c.input.sequence > p.lastSequence)))
    || state.players.some((p, i) => p.connected && state.players.slice(i + 1).some(q => q.connected && distance(p.position, q.position) < footballLimits.playerRadius * 2 - .001))
    || state.ball.ownerId === null && state.ball.height <= 1.05 && state.players.some(p => p.connected && distance(p.position, state.ball.position) < (p.goalkeeper ? 1.05 : .7));
  return moving ? Math.min(deadline, now + footballLimits.tickMs) : Number.isFinite(deadline) ? deadline : null;
}

/** Server elapsed time only. At most 25 small steps per call, never a client result. */
export function advanceFootballMatch(source: FootballState, commands: readonly FootballTimedInput[], now: number): FootballState {
  if (!Number.isFinite(now) || now <= source.simulatedAt || !["playing", "restart"].includes(source.phase)) return source;
  const state = copy(source), byId = new Map(commands.slice(0, footballLimits.players).map(c => [c.playerId, c]));
  const start = Math.max(source.simulatedAt, now - footballLimits.catchupMs);
  let clock = start;
  while (clock < now && ["playing", "restart"].includes(state.phase)) {
    const dtMs = Math.min(footballLimits.stepMs, now - clock); clock += dtMs;
    if (state.endsAt !== null && clock >= state.endsAt) { finish(state); break; }
    if (state.phase === "restart") { if (clock >= (state.restartAt ?? Infinity)) kickoff(state, clock); else continue; }
    const inputs = footballNpcInputs(state, clock);
    for (const p of state.players) {
      if (p.bot) continue;
      const latest = byId.get(p.id), command = latest && latest.receivedAt <= clock ? latest : latest?.previous;
      const live = command && command.receivedAt <= clock && clock - command.receivedAt <= footballLimits.inputExpiryMs;
      const accepted = live && validFootballInput(command.input, state.pitch) ? command.input : null;
      const input = accepted && clock - (latest?.actionReceivedAt ?? command!.receivedAt) > footballLimits.inputExpiryMs ? { ...accepted, action: "none" as const } : accepted;
      if (input) {
        // A packet older than its accepted sequence cannot undo a newer action.
        if (input.sequence >= p.lastSequence) inputs.set(p.id, input);
      }
    }
    for (const p of state.players) movePlayer(state, p, inputs.get(p.id) ?? null, dtMs / 1000);
    separatePlayers(state); claimBall(state, clock, inputs);
    for (const p of state.players) {
      const input = inputs.get(p.id);
      if (input && input.sequence > p.lastSequence) { kick(state, p, input, clock); p.lastSequence = input.sequence; }
    }
    advanceBall(state, dtMs / 1000, clock);
  }
  // Inactive clients cannot fast-forward through all missed physics or mint goals.
  if (state.endsAt !== null && now >= state.endsAt && ["playing", "restart"].includes(state.phase)) finish(state);
  state.simulatedAt = now; state.revision++; return state;
}

export function setFootballReady(source: FootballState, playerId: string, ready: boolean): FootballState {
  if (source.phase !== "lobby") throw new Error("Readiness changes before kick-off.");
  const state = copy(source), p = state.players.find(p => p.id === playerId && !p.bot);
  if (!p || !p.connected) throw new Error("Rejoin the pitch first.");
  p.ready = ready; state.revision++; return state;
}
export function setFootballSide(source: FootballState, playerId: string, side: FootballSide): FootballState {
  if (source.mode !== "teams" || source.phase !== "lobby") throw new Error("Choose sides before kick-off.");
  const player = source.players.find(p => p.id === playerId && !p.bot);
  if (!player?.connected) throw new Error("Rejoin the pitch first.");
  if (player.side === side) return source;
  const state = copy(source), counts = { home: 0, away: 0 };
  const humans = state.players.filter(p => !p.bot);
  humans.find(p => p.id === playerId)!.side = side;
  const priorBots = new Map(state.players.filter(p => p.bot).map(p => [p.id, p]));
  state.players = footballNpcRoster(state.id, humans).map(seat => humans.find(p => p.id === seat.id) ?? priorBots.get(seat.id)
    ?? { id: seat.id, side: seat.side, bot: true, goalkeeper: !!seat.goalkeeper, position: zero(), velocity: zero(), yaw: 0,
      ready: true, connected: true, lastSequence: 0, actionAt: -1000, goals: 0, assists: 0 });
  for (const p of state.players) {
    p.ready = p.bot; p.position = spawn(state.pitch, p.side, counts[p.side]++, p.goalkeeper); p.velocity = zero(); p.yaw = p.side === "home" ? Math.PI : 0;
  }
  state.revision++; return state;
}
export function setFootballConnected(source: FootballState, playerId: string, connected: boolean): FootballState {
  const state = copy(source), p = state.players.find(p => p.id === playerId && !p.bot);
  if (!p || p.connected === connected) return source;
  p.connected = connected; p.velocity = zero(); if (!connected) p.ready = false;
  if (state.ball.ownerId === playerId && !connected) { state.ball.ownerId = null; state.ball.velocity = zero(); }
  state.revision++;
  if (!state.players.some(p => !p.bot && p.connected)) return cancelFootballMatch(state);
  if (state.mode === "teams" && ["playing", "restart"].includes(state.phase) && !state.players.some(p => !p.bot && p.connected && p.side === "home")) return cancelFootballMatch(state);
  if (state.mode === "teams" && ["playing", "restart"].includes(state.phase) && !state.players.some(p => !p.bot && p.connected && p.side === "away")) return cancelFootballMatch(state);
  return state;
}
