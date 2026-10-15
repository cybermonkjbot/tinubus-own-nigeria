import { footballLimits, type FootballInput, type FootballPlayer, type FootballPoint, type FootballSeat, type FootballSide, type FootballState } from "./companionFootball";

type FootballNpcRole = "keeper" | "defender" | "playmaker" | "striker";
type FootballNpcStyle = { role: FootballNpcRole; lane: number; anticipation: number; shootingRange: number; passing: number; actionDelayMs: number };
const roles: readonly FootballNpcRole[] = ["keeper", "defender", "playmaker", "striker"];
const clamp = (n: number, low: number, high: number) => Math.max(low, Math.min(high, n));
const distance = (a: FootballPoint, b: FootballPoint) => Math.hypot(a.x - b.x, a.z - b.z);
const attackSign = (side: FootballSide) => side === "home" ? -1 : 1;
const compareIds = (a: FootballPlayer, b: FootballPlayer) => a.id < b.id ? -1 : a.id > b.id ? 1 : 0;
function seed(id: string): number {
  let value = 2166136261;
  for (let i = 0; i < id.length; i++) value = Math.imul(value ^ id.charCodeAt(i), 16777619);
  return value >>> 0;
}

/** Stable tendencies derived from actor identity, without another stored actor record. */
export function footballNpcStyle(player: Pick<FootballPlayer, "id" | "goalkeeper">): FootballNpcStyle {
  const value = seed(player.id), namedRole = player.id.split(":")[2];
  const role = player.goalkeeper ? "keeper" : roles.find(item => item === namedRole && item !== "keeper") ?? roles[1 + value % 3];
  return { role, lane: ((value >>> 4) % 3 - 1) * .48, anticipation: .18 + (value % 5) * .05,
    shootingRange: (role === "striker" ? 12 : role === "playmaker" ? 9 : 7) + value % 3,
    passing: role === "playmaker" ? 1.7 : role === "defender" ? 1.3 : .85,
    actionDelayMs: 520 + value % 5 * 80 };
}

/** Fill small sides to four actors, retaining every admitted human and the total cap. */
export function footballNpcRoster(matchId: string, seats: readonly FootballSeat[]): FootballSeat[] {
  const roster = seats.map(seat => ({ ...seat }));
  const counts = { home: roster.filter(p => p.side === "home").length, away: roster.filter(p => p.side === "away").length };
  const scope = seed(matchId).toString(36);
  while (roster.length < footballLimits.players && (counts.home < 4 || counts.away < 4)) {
    const side: FootballSide = counts.home < 4 && (counts.home <= counts.away || counts.away >= 4) ? "home" : "away";
    const role = roles.find(candidate => !roster.some(p => p.side === side
      && (candidate === "keeper" ? p.goalkeeper : p.bot && footballNpcStyle({ id: p.id, goalkeeper: !!p.goalkeeper }).role === candidate))) ?? "striker";
    let id = `bot:football:${role}:${side}:${scope}`;
    for (let suffix = 1; roster.some(p => p.id === id); suffix++) id = `bot:football:${role}:${side}:${scope}:${suffix}`;
    roster.push({ id, side, bot: true, goalkeeper: role === "keeper" }); counts[side]++;
  }
  return roster;
}

function boundedPoint(state: FootballState, point: FootballPoint): FootballPoint {
  const p = state.pitch, margin = footballLimits.playerRadius + .2;
  return { x: clamp(point.x, p.centre.x - p.halfWidth + margin, p.centre.x + p.halfWidth - margin),
    z: clamp(point.z, p.centre.z - p.halfLength + margin, p.centre.z + p.halfLength - margin) };
}
export function footballKeeperTarget(state: FootballState, player: FootballPlayer): FootballPoint {
  const p = state.pitch;
  return { x: clamp(state.ball.position.x, p.centre.x - p.goalHalfWidth + .3, p.centre.x + p.goalHalfWidth - .3),
    z: p.centre.z - attackSign(player.side) * (p.halfLength - 1.1) };
}
function laneClearance(from: FootballPoint, to: FootballPoint, opponents: readonly FootballPlayer[]): number {
  const dx = to.x - from.x, dz = to.z - from.z, squared = dx * dx + dz * dz;
  let clearance = 6;
  for (const opponent of opponents) {
    const t = clamp(((opponent.position.x - from.x) * dx + (opponent.position.z - from.z) * dz) / Math.max(.001, squared), 0, 1);
    clearance = Math.min(clearance, distance(opponent.position, { x: from.x + dx * t, z: from.z + dz * t }));
  }
  return clearance;
}
function intent(player: FootballPlayer, target: FootballPoint): FootballInput {
  return { sequence: player.lastSequence + 1, move: { x: 0, z: 0 }, aim: { x: 0, z: attackSign(player.side) },
    destination: target, action: "none", targetPlayerId: null, power: .7 };
}
function nearest(players: readonly FootballPlayer[], point: FootballPoint, styleBias = false): FootballPlayer | undefined {
  let best: FootballPlayer | undefined, bestDistance = Infinity;
  for (const player of players) {
    const score = distance(player.position, point) + (styleBias && player.bot && footballNpcStyle(player).role === "defender" ? .7 : 0);
    if (score < bestDistance || score === bestDistance && player.id < (best?.id ?? "")) { best = player; bestDistance = score; }
  }
  return best;
}

/** All choices observe the same pre-movement step. NPCs never send client commands. */
export function footballNpcInputs(state: FootballState, now: number): Map<string, FootballInput> {
  const inputs = new Map<string, FootballInput>();
  if (state.phase !== "playing") return inputs;
  const active = state.players.filter(player => player.connected).slice(0, footballLimits.players);
  const owner = active.find(player => player.id === state.ball.ownerId);
  for (const player of active) {
    if (!player.bot) continue;
    const style = footballNpcStyle(player), sign = attackSign(player.side), p = state.pitch;
    if (player.goalkeeper) { inputs.set(player.id, intent(player, footballKeeperTarget(state, player))); continue; }
    const mates = active.filter(mate => mate.side === player.side && mate.id !== player.id && !mate.goalkeeper);
    const opponents = active.filter(opponent => opponent.side !== player.side);
    const ownGoal = { x: p.centre.x, z: p.centre.z - sign * p.halfLength };
    const attackingGoal = { x: p.centre.x, z: p.centre.z + sign * p.halfLength };
    const ballTarget = boundedPoint(state, { x: state.ball.position.x + state.ball.velocity.x * style.anticipation,
      z: state.ball.position.z + state.ball.velocity.z * style.anticipation });
    let target: FootballPoint, pursuingBall = false;
    if (owner?.id === player.id) {
      // Dribble away from nearby defenders while keeping a route toward goal.
      const pressure = nearest(opponents, player.position);
      target = boundedPoint(state, { x: p.centre.x + style.lane * p.goalHalfWidth
        + (pressure && distance(pressure.position, player.position) < 3 ? (player.position.x >= pressure.position.x ? 1.5 : -1.5) : 0),
        z: attackingGoal.z - sign * 1.5 });
    } else if (owner?.side === player.side) {
      // Stay available instead of joining the carrier's collision radius.
      const forward = style.role === "striker" ? 5 : style.role === "defender" ? -5 : 1.5;
      const lane = style.lane || (seed(player.id) % 2 ? .48 : -.48);
      target = boundedPoint(state, { x: p.centre.x + lane * p.halfWidth,
        z: owner.position.z + sign * forward });
    } else {
      const outfield = active.filter(mate => mate.side === player.side && !mate.goalkeeper);
      const teamTarget = boundedPoint(state, { x: state.ball.position.x + state.ball.velocity.x * .28,
        z: state.ball.position.z + state.ball.velocity.z * .28 });
      // An idle human never prevents their NPC teammates from pursuing play.
      const chaser = nearest(outfield.filter(mate => mate.bot), teamTarget, true);
      if (chaser?.id === player.id) {
        target = owner ? boundedPoint(state, { x: owner.position.x + owner.velocity.x * style.anticipation,
          z: owner.position.z + owner.velocity.z * style.anticipation }) : ballTarget;
        pursuingBall = true;
      } else {
        const threats = opponents.filter(opponent => !opponent.goalkeeper && opponent.id !== owner?.id)
          .sort((a, b) => distance(a.position, ownGoal) - distance(b.position, ownGoal) || compareIds(a, b));
        const defenders = outfield.filter(mate => mate.id !== chaser?.id).sort(compareIds);
        const threat = owner ? threats[defenders.findIndex(mate => mate.id === player.id)] : undefined;
        target = threat ? { x: threat.position.x, z: threat.position.z - sign * 1.4 }
          : { x: p.centre.x + style.lane * p.halfWidth,
            z: p.centre.z + (state.ball.position.z - p.centre.z) * .4 - sign * p.halfLength * (style.role === "defender" ? .45 : .15) };
        target = boundedPoint(state, target);
      }
    }
    // Separate off-ball runs before the shared collision solver has to push actors apart.
    if (!pursuingBall && owner?.id !== player.id && active.some(other => other.id !== player.id && distance(other.position, target) < 1.2)) {
      target = boundedPoint(state, { x: target.x + (seed(player.id) % 2 ? 1.4 : -1.4), z: target.z });
    }
    const input = intent(player, target);
    if (owner?.id === player.id && now - player.actionAt >= style.actionDelayMs) {
      const pressure = nearest(opponents, player.position), pressured = !!pressure && distance(player.position, pressure.position) < 2.4;
      const range = distance(player.position, attackingGoal);
      const keeper = opponents.find(opponent => opponent.goalkeeper);
      const shotTarget = { x: p.centre.x + (keeper && keeper.position.x >= p.centre.x ? -1 : 1) * Math.max(.2, p.goalHalfWidth - .5), z: attackingGoal.z };
      const shotLane = laneClearance(player.position, shotTarget, opponents.filter(opponent => !opponent.goalkeeper));
      let passTo: FootballPlayer | undefined, passScore = -Infinity;
      for (const mate of mates) {
        const passDistance = distance(player.position, mate.position);
        if (passDistance < 2 || passDistance > 16) continue;
        const clearance = laneClearance(player.position, mate.position, opponents);
        if (clearance < .95) continue;
        const progress = (mate.position.z - player.position.z) * sign;
        const score = progress * .7 + Math.min(3, clearance) * style.passing - passDistance * .15 + (mate.bot ? 0 : .5);
        if (score > passScore || score === passScore && mate.id < (passTo?.id ?? "")) { passTo = mate; passScore = score; }
      }
      if (range <= style.shootingRange && shotLane > .9 && Math.abs(player.position.x - p.centre.x) < p.goalHalfWidth + 4
        && (style.role === "striker" || range < 7 || !passTo || passScore < 3)) {
        const d = distance(player.position, shotTarget);
        input.action = "shoot"; input.aim = { x: (shotTarget.x - player.position.x) / d, z: (shotTarget.z - player.position.z) / d };
        input.power = clamp(.45 + range / 30, .45, .9);
      } else if (passTo && (pressured || passScore > (style.role === "playmaker" ? 2 : 4))) {
        input.action = "pass"; input.targetPlayerId = passTo.id; input.power = clamp(distance(player.position, passTo.position) / 18, .25, 1);
      }
    } else if (owner && owner.side !== player.side && distance(player.position, owner.position) < 1.35 && now - player.actionAt >= 650) {
      input.action = "challenge";
    }
    inputs.set(player.id, input);
  }
  return inputs;
}

/** A held ball still needs a wakeup when an NPC can move, tackle or take a kick. */
export function footballNpcsNeedAdvance(state: FootballState, now: number): boolean {
  const inputs = footballNpcInputs(state, now);
  return state.players.some(player => {
    const input = inputs.get(player.id);
    return !!input && (input.action !== "none" || state.ball.ownerId === player.id
      || input.destination !== null && distance(player.position, input.destination) > (player.goalkeeper ? .08 : .1));
  });
}
