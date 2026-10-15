import { routeSegmentClear, type RoomNavigation, type ScenePoint } from "./companionNavigation";
import { companionStreetWeapon, type CompanionStreetWeaponId } from "./companionCombatEquipment";
import { advanceCompanionRobbery, companionRobberyActive, companionRobberyObjective, finishCompanionRobbery, type CompanionCombatRobbery } from "./companionCombatRobbery";
import type { CompanionGunfireState } from "./companionGunfire";

export type CombatPoint = ScenePoint;
export type CombatInput = { sequence: number; move: readonly [right: number, forward: number]; yaw: number; pitch: number; attack: boolean; block: boolean; reload: boolean };
export type CombatTuning = { health: number; stamina: number; speed: number; meleeDamage: number; meleeRange: number; meleeCooldownMs: number; bulletDamage: number; bulletRange: number; fireCooldownMs: number; magazine: number; reserveAmmo: number; reloadMs: number; npcSpeed: number; npcWindupMs: number; blockMultiplier: number; spawnProtectionMs: number; durationMs: number; target: number };
/** Default game tuning. Arena ammunition is separate from owned street equipment. */
export const proposedCombatTuning: CombatTuning = { health: 100, stamina: 100, speed: 4.4, meleeDamage: 18, meleeRange: 1.65, meleeCooldownMs: 500, bulletDamage: 25, bulletRange: 65, fireCooldownMs: 240, magazine: 12, reserveAmmo: 48, reloadMs: 1700, npcSpeed: 2.4, npcWindupMs: 850, blockMultiplier: .25, spawnProtectionMs: 2500, durationMs: 300000, target: 5 };
export const combatLimits = { players: 12, npcs: 3, inputIntervalMs: 200, inputExpiryMs: 450, maxCatchupMs: 600, stepMs: 40, events: 256 } as const;
export type CombatFighter = { id: string; kind: "player" | "agbero"; characterId: string; position: CombatPoint; yaw: number; pitch: number; health: number; maxHealth: number; stamina: number; ammo: number; reserveAmmo: number; reloadAt: number | null; status: "active" | "defeated" | "left"; joinedAt: number; lastInputSequence: number; lastAttackAt: number; protectedUntil: number; kills: number; streetWeaponId?: CompanionStreetWeaponId | null; targetId?: string; robbery?: CompanionCombatRobbery | null;
  skillLevels?: { fighting: number; selfDefence: number; fitness: number } };
export type CombatEvent = { id: number; kind: "attack" | "hit" | "defeat" | "reload" | "finish" | "left"; at: number; actorId: string; targetId: string | null; shotPoint?: CombatPoint; blocked?: boolean };
export type CombatState = { gunfire?: CompanionGunfireState; missionRunId?: string; id: string; instanceId: string; placeId: string; layoutId: string; callSessionId: string | null; mode: "melee" | "arena"; phase: "playing" | "finished" | "cancelled"; revision: number; simulatedAt: number; endsAt: number; navigation: RoomNavigation; spawns: CombatPoint[]; fighters: CombatFighter[]; objective: { target: number; winnerId: string | null }; events: CombatEvent[]; tuning: CombatTuning };
export type CombatTimedInput = { fighterId: string; input: CombatInput; receivedAt: number };
export type CombatSnapshot = { state: CombatState | null; playerId: string | null; ready: boolean; busy: boolean; error: string | null; serverOffset: number };
export interface CompanionCombatStore { getSnapshot(): CombatSnapshot; subscribe(listener: () => void): () => void; select(instanceId: string | null): void; send(input: CombatInput): Promise<void>; join(): Promise<void>; leave(): Promise<void>; robberyAction?(action: "defend" | "flee"): Promise<void>; stop(): void; dispose(): void }
export const emptyCombatSnapshot: CombatSnapshot = { state: null, playerId: null, ready: false, busy: false, error: null, serverOffset: 0 };

export function combatViewDirection(yaw: number, pitch: number): CombatPoint { return [Math.sin(yaw) * Math.cos(pitch), Math.sin(pitch), -Math.cos(yaw) * Math.cos(pitch)]; }
export function combatWeapon(state: CombatState, fighter: CombatFighter) {
  return state.mode === "melee" && fighter.kind === "player" ? companionStreetWeapon(fighter.streetWeaponId) : undefined;
}
export function combatUsesFirearm(state: CombatState, fighter: CombatFighter) { return state.mode === "arena" || combatWeapon(state, fighter)?.kind === "firearm"; }
export function combatMagazine(state: CombatState, fighter: CombatFighter) { return combatWeapon(state, fighter)?.magazine ?? state.tuning.magazine; }
export function validCombatInput(input: CombatInput): boolean {
  return Number.isSafeInteger(input.sequence) && input.sequence >= 0 && input.sequence < Number.MAX_SAFE_INTEGER && Array.isArray(input.move) && input.move.length === 2
    && input.move.every(v => Number.isFinite(v) && Math.abs(v) <= 1) && Number.isFinite(input.yaw) && Math.abs(input.yaw) <= Math.PI * 2
    && Number.isFinite(input.pitch) && Math.abs(input.pitch) <= Math.PI / 2 - .05 && [input.attack, input.block, input.reload].every(v => typeof v === "boolean");
}
export function idleCombatInput(sequence: number, yaw = 0, pitch = 0): CombatInput { return { sequence, move: [0, 0], yaw, pitch, attack: false, block: false, reload: false }; }
export function predictCombatPosition(state: CombatState, fighter: CombatFighter, input: CombatInput, deltaSeconds: number): CombatPoint {
  if (!validCombatInput(input) || fighter.status !== "active" || state.phase !== "playing") return fighter.position;
  const dt = Math.min(.2, Math.max(0, deltaSeconds)), length = Math.max(1, Math.hypot(...input.move));
  const right = input.move[0] / length, forward = input.move[1] / length;
  const speed = state.tuning.speed * (input.block ? .55 : 1);
  const dx = (Math.cos(input.yaw) * right + Math.sin(input.yaw) * forward) * speed * dt;
  const dz = (Math.sin(input.yaw) * right - Math.cos(input.yaw) * forward) * speed * dt;
  const start = fighter.position;
  const target: CombatPoint = [start[0] + dx, state.navigation.floorY, start[2] + dz];
  if (routeSegmentClear(state.navigation, start, target)) return target;
  const slideX: CombatPoint = [target[0], target[1], start[2]], slideZ: CombatPoint = [start[0], target[1], target[2]];
  if (routeSegmentClear(state.navigation, start, slideX)) return slideX;
  if (routeSegmentClear(state.navigation, start, slideZ)) return slideZ;
  return start;
}
export function combatObjective(state: CombatState, playerId: string): string {
  const player = state.fighters.find(f => f.id === playerId);
  if (player?.status === "defeated") return "Recover at the hospital";
  if (player?.robbery) return companionRobberyObjective(player.robbery);
  if (state.missionRunId) return state.phase === "playing" ? "Fight through the mission guards"
    : state.phase === "finished" && state.fighters.filter(f => f.kind === "agbero").every(f => f.status === "defeated") ? "Checkpoint complete" : "Checkpoint failed";
  if (state.phase === "cancelled") return "Encounter ended";
  if (state.phase === "finished") return state.objective.winnerId === playerId ? "Objective complete" : "Match ended";
  return state.mode === "melee" ? "Defeat the agbero or move away" : "Last player standing";
}
export function createCombatFighter(id: string, characterId: string, kind: CombatFighter["kind"], position: CombatPoint, now: number, tuning = proposedCombatTuning): CombatFighter {
  return { id, characterId, kind, position: [...position], yaw: 0, pitch: 0, health: tuning.health, maxHealth: tuning.health, stamina: tuning.stamina, ammo: tuning.magazine, reserveAmmo: tuning.reserveAmmo, reloadAt: null, status: "active", joinedAt: now, lastInputSequence: -1, lastAttackAt: 0, protectedUntil: now + tuning.spawnProtectionMs, kills: 0 };
}
function emit(state: CombatState, kind: CombatEvent["kind"], actorId: string, targetId: string | null, at: number, shotPoint?: CombatPoint) {
  state.events.push({ id: (state.events.at(-1)?.id ?? 0) + 1, kind, actorId, targetId, at, ...(shotPoint ? { shotPoint: [...shotPoint] as CombatPoint } : {}) });
  if (state.events.length > combatLimits.events) state.events.splice(0, state.events.length - combatLimits.events);
}
function clearSight(state: CombatState, a: CombatPoint, b: CombatPoint) {
  // Match collision and shooting use the exact resolved venue footprint.
  return routeSegmentClear({ ...state.navigation, radius: .001 }, [a[0], state.navigation.floorY, a[2]], [b[0], state.navigation.floorY, b[2]]);
}
function skillBonus(fighter: CombatFighter, skill: "fighting" | "selfDefence" | "fitness") {
  const level = fighter.skillLevels?.[skill] ?? 1;
  return Number.isSafeInteger(level) && level >= 1 && level <= 5 ? level - 1 : 0;
}
function strike(state: CombatState, attacker: CombatFighter, input: CombatInput, inputs: readonly CombatTimedInput[], now: number) {
  const gun = combatUsesFirearm(state, attacker), tuning = state.tuning, weapon = combatWeapon(state, attacker);
  if (attacker.robbery && (!companionRobberyActive(attacker.robbery) || attacker.robbery.phase === "demanding")) return;
  if (!input.attack || attacker.reloadAt !== null || now < attacker.protectedUntil || now - attacker.lastAttackAt < (weapon?.cooldownMs ?? (gun ? tuning.fireCooldownMs : tuning.meleeCooldownMs)) || (!gun && attacker.stamina < 12) || (gun && !attacker.ammo)) return;
  attacker.lastAttackAt = now;
  if (gun) attacker.ammo--; else attacker.stamina -= 12;
  emit(state, "attack", attacker.id, null, now, gun ? attacker.position : undefined);
  const direction = combatViewDirection(input.yaw, input.pitch);
  let victim: CombatFighter | undefined, nearest = Infinity;
  for (const target of state.fighters) {
    if (target.id === attacker.id || target.status !== "active" || now < target.protectedUntil || ((state.mode === "melee" || !!state.missionRunId) && target.kind === attacker.kind)
      || attacker.kind === "agbero" && attacker.targetId && target.id !== attacker.targetId
      || attacker.kind === "player" && attacker.robbery && target.id !== attacker.robbery.toutId) continue;
    const dx = target.position[0] - attacker.position[0], dz = target.position[2] - attacker.position[2];
    const distance = Math.hypot(dx, dz);
    if (distance > (weapon?.range ?? (gun ? tuning.bulletRange : tuning.meleeRange)) || distance >= nearest || !clearSight(state, attacker.position, target.position)) continue;
    const dy = -.25;
    const along = dx * direction[0] + dz * direction[2] + (gun ? dy * direction[1] : 0);
    if (along <= 0) continue;
    if (gun) {
      const cross = Math.hypot(dx - direction[0] * along, dz - direction[2] * along, dy - direction[1] * along);
      if (cross > (weapon?.aimRadius ?? .5)) continue;
    } else if (along / Math.max(.01, distance) < .5) continue;
    victim = target; nearest = distance;
  }
  if (!victim) return;
  const blocking = !gun && !combatUsesFirearm(state, victim) && inputs.some(i => i.fighterId === victim!.id && i.input.block && now - i.receivedAt <= combatLimits.inputExpiryMs);
  const guard = blocking && Math.cos(victim.yaw - Math.atan2(attacker.position[0] - victim.position[0], -(attacker.position[2] - victim.position[2]))) > .5;
  const rangeMultiplier = weapon ? 1 - weapon.damageFalloff * nearest / weapon.range : 1;
  const fightingMultiplier = gun ? 1 : 1 + skillBonus(attacker, "fighting") * .05;
  const guardMultiplier = guard ? tuning.blockMultiplier * (1 - skillBonus(victim, "selfDefence") * .05) : 1;
  victim.health = Math.max(0, victim.health - (weapon?.damage ?? (gun ? tuning.bulletDamage : tuning.meleeDamage)) * rangeMultiplier * fightingMultiplier * guardMultiplier);
  emit(state, "hit", attacker.id, victim.id, now);
  if (guard) state.events[state.events.length - 1].blocked = true;
  if (!victim.health) {
    victim.status = "defeated"; victim.reloadAt = null; attacker.kills++;
    emit(state, "defeat", attacker.id, victim.id, now);
  }
}
function settleRobberies(state: CombatState, now: number) {
  for (const player of state.fighters) {
    const robbery = player.robbery;
    if (player.kind !== "player" || !robbery) continue;
    const attacker = state.fighters.find(fighter => fighter.id === robbery.toutId);
    if (companionRobberyActive(robbery)) player.robbery = player.status === "defeated" ? finishCompanionRobbery(robbery, "defeated", now)
      : player.status === "left" ? robbery
      : !attacker || attacker.status === "left" ? finishCompanionRobbery(robbery, "interrupted", now)
      : attacker.status === "defeated" ? finishCompanionRobbery(robbery, "repelled", now)
      : advanceCompanionRobbery(robbery, player.position, attacker.position, now);
    if (!companionRobberyActive(player.robbery!)) {
      if (attacker?.status === "active") attacker.status = "left";
      if (player.status === "active") player.status = "left";
    } else if (player.status === "left" && attacker?.status === "active") attacker.status = "left";
  }
}
/** Single attack/reload edge is evaluated at receipt, not after a render frame. */
export function applyCombatAction(previous: CombatState, fighterId: string, input: CombatInput, inputRows: readonly CombatTimedInput[], now: number): CombatState {
  if (!validCombatInput(input) || previous.phase !== "playing" || !Number.isFinite(now) || now < previous.simulatedAt) return previous;
  const state: CombatState = { ...previous, fighters: previous.fighters.map(f => ({ ...f })), events: [...previous.events], objective: { ...previous.objective } };
  const fighter = state.fighters.find(f => f.id === fighterId);
  if (!fighter || fighter.status !== "active") return previous;
  fighter.yaw = input.yaw; fighter.pitch = input.pitch; fighter.lastInputSequence = input.sequence;
  if (combatUsesFirearm(state, fighter) && input.reload && fighter.reloadAt === null && fighter.ammo < combatMagazine(state, fighter) && fighter.reserveAmmo > 0) {
    fighter.reloadAt = now + (combatWeapon(state, fighter)?.reloadMs ?? state.tuning.reloadMs); emit(state, "reload", fighter.id, null, now);
  }
  strike(state, fighter, input, inputRows, now);
  settleRobberies(state, now);
  const winner = state.mode === "arena" && !state.missionRunId ? state.fighters.find(f => f.kind === "player" && f.kills >= state.objective.target) : undefined;
  if (winner || (state.mode === "melee" || !!state.missionRunId) && !state.fighters.some(f => f.kind === "agbero" && f.status === "active")) {
    state.phase = "finished"; state.objective.winnerId = winner?.id ?? fighter.id; emit(state, "finish", state.objective.winnerId, null, now);
  }
  if (state.phase === "playing" && !state.missionRunId && state.mode === "arena" && state.fighters.some(f => f.kind === "player" && f.status === "defeated") && state.fighters.filter(f => f.kind === "player" && f.status === "active").length === 1) {
    const survivor = state.fighters.find(f => f.kind === "player" && f.status === "active")!;
    state.phase = "finished"; state.objective.winnerId = survivor.id; emit(state, "finish", survivor.id, null, now);
  }
  if (state.missionRunId && state.phase === "playing" && state.fighters.some(f => f.kind === "player" && f.status !== "active")) state.phase = "cancelled";
  state.revision++;
  return state;
}
/** Server invocation only decides damage and defeat. Renderer predictions never settle consequences. */
export function advanceCombat(previous: CombatState, inputRows: readonly CombatTimedInput[], now: number): CombatState {
  if (!Number.isFinite(now) || now < previous.simulatedAt) throw new Error("Invalid combat time.");
  const state: CombatState = { ...previous, fighters: previous.fighters.map(f => ({ ...f, position: [...f.position] })), events: [...previous.events], objective: { ...previous.objective } };
  if (state.phase !== "playing") return state;
  const end = Math.min(now, state.endsAt), start = Math.max(state.simulatedAt, end - combatLimits.maxCatchupMs);
  for (let at = start; at < end;) {
    const next = Math.min(end, at + combatLimits.stepMs), dt = (next - at) / 1000; at = next;
    settleRobberies(state, at);
    const inputs: CombatTimedInput[] = inputRows.filter(row => validCombatInput(row.input) && at >= row.receivedAt && at - row.receivedAt <= combatLimits.inputExpiryMs);
    for (const fighter of state.fighters) {
      if (fighter.status !== "active") continue;
      const supplied = inputs.find(row => row.fighterId === fighter.id)?.input;
      let input = supplied ?? idleCombatInput(fighter.lastInputSequence + 1, fighter.yaw, fighter.pitch);
      if (fighter.kind === "agbero") {
        const target = state.fighters.filter(f => f.kind === "player" && f.status === "active" && (!fighter.targetId || fighter.targetId === f.id))
          .sort((a, b) => Math.hypot(a.position[0] - fighter.position[0], a.position[2] - fighter.position[2]) - Math.hypot(b.position[0] - fighter.position[0], b.position[2] - fighter.position[2]))[0];
        if (target) {
          if (target.robbery?.phase === "demanding") continue;
          const distance = Math.hypot(target.position[0] - fighter.position[0], target.position[2] - fighter.position[2]);
          input = { ...input, yaw: Math.atan2(target.position[0] - fighter.position[0], -(target.position[2] - fighter.position[2])), pitch: 0, move: distance > state.tuning.meleeRange * .8 && distance < 14 ? [0, state.tuning.npcSpeed / state.tuning.speed] : [0, 0], attack: distance <= state.tuning.meleeRange && at - fighter.lastAttackAt >= state.tuning.npcWindupMs };
          if (state.missionRunId) input = { ...input, pitch: Math.atan2(-.25, Math.max(.1, distance)),
            move: distance > 6 && distance < 30 ? [0, state.tuning.npcSpeed / state.tuning.speed] : [0, 0],
            attack: distance <= 30 && at - fighter.lastAttackAt >= state.tuning.npcWindupMs,
            reload: fighter.ammo === 0 && fighter.reserveAmmo > 0 };
        }
      }
      fighter.yaw = input.yaw; fighter.pitch = input.pitch;
      fighter.position = predictCombatPosition(state, fighter, input, dt);
      fighter.stamina = Math.min(state.tuning.stamina, fighter.stamina + dt * (10 + skillBonus(fighter, "fitness")));
      if (fighter.reloadAt !== null && fighter.reloadAt <= at) { const rounds = Math.min(combatMagazine(state, fighter) - fighter.ammo, fighter.reserveAmmo); fighter.ammo += rounds; fighter.reserveAmmo -= rounds; fighter.reloadAt = null; }
      if (combatUsesFirearm(state, fighter) && input.reload && fighter.reloadAt === null && fighter.ammo < combatMagazine(state, fighter) && fighter.reserveAmmo > 0) { fighter.reloadAt = at + (combatWeapon(state, fighter)?.reloadMs ?? state.tuning.reloadMs); emit(state, "reload", fighter.id, null, at); }
      strike(state, fighter, input, inputs, at);
    }
    settleRobberies(state, at);
    if (state.missionRunId && state.fighters.some(f => f.kind === "player" && f.status !== "active")) { state.phase = "cancelled"; break; }
    const winner = state.mode === "arena" && !state.missionRunId ? state.fighters.find(f => f.kind === "player" && f.kills >= state.objective.target) : undefined;
    if (winner || (state.mode === "melee" || !!state.missionRunId) && !state.fighters.some(f => f.kind === "agbero" && f.status === "active")) {
      state.phase = "finished"; state.objective.winnerId = winner?.id ?? state.fighters.find(f => f.kind === "player" && f.status === "active")?.id ?? null;
      emit(state, "finish", state.objective.winnerId ?? "", null, at); break;
    }
    if (!state.fighters.some(f => f.kind === "player" && f.status === "active")) { state.phase = "finished"; emit(state, "finish", "", null, at); break; }
    if (!state.missionRunId && state.mode === "arena" && state.fighters.some(f => f.kind === "player" && f.status === "defeated") && state.fighters.filter(f => f.kind === "player" && f.status === "active").length === 1) {
      const survivor = state.fighters.find(f => f.kind === "player" && f.status === "active")!;
      state.phase = "finished"; state.objective.winnerId = survivor.id; emit(state, "finish", survivor.id, null, at); break;
    }
  }
  if (state.missionRunId && state.fighters.some(f => f.kind === "player" && f.status !== "active")) state.phase = "cancelled";
  if (now >= state.endsAt && state.phase === "playing") { state.phase = "finished"; emit(state, "finish", "", null, now); }
  state.simulatedAt = now; state.revision++;
  return state;
}
