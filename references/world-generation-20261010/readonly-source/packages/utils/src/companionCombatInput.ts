import { combatLimits, combatUsesFirearm, combatMagazine, combatWeapon, idleCombatInput, type CombatInput, type CombatSnapshot, type CompanionCombatStore } from "./companionCombat";

export type CombatInputFeedback = { id: number; action: "attack" | "reload"; kind: "received" | "queued" | "denied"; message: string };

export type CombatControlSnapshot = {
  active: boolean;
  blocked: boolean;
  eligible: boolean;
  input: CombatInput;
  error: string | null;
  feedback: CombatInputFeedback | null;
};
export type CompanionCombatInput = ReturnType<typeof createCompanionCombatInput>;
type InputStore = Pick<CompanionCombatStore, "getSnapshot" | "send" | "stop">;

const finite = (value: number) => Number.isFinite(value) ? value : 0;
const yawLimit = (value: number) => Math.atan2(Math.sin(value), Math.cos(value));
const pitchLimit = (value: number) => Math.max(-Math.PI / 2 + .05, Math.min(Math.PI / 2 - .05, finite(value)));

/** Host-frame driven. No timers, subscriptions to the backend, or local damage decisions. */
export function createCompanionCombatInput(store: InputStore, options: { sensitivity?: number; onError?(error: string): void } = {}) {
  const listeners = new Set<() => void>();
  const moves = new Map<string, readonly [number, number]>();
  const attacks = new Set<string>(), blocks = new Set<string>();
  let snapshot = store.getSnapshot();
  let active = false, blocked = false, disposed = false, eligible = false;
  let yaw = 0, pitch = 0, sequence = 0, scope = "", generation = 0;
  let attackEdge = false, attackExpiresAt = 0, reload = false, reloadExpiresAt = 0, dirty = false, inFlight = false;
  let awaitingAck: { sequence: number; sentAt: number; reload: boolean } | null = null;
  let feedback: CombatInputFeedback | null = null, feedbackId = 0, feedbackUntil = 0, heldDenial = "";
  let lastSend = -Infinity, error: string | null = null;
  let sensitivity = Math.max(.0002, Math.min(.02, finite(options.sensitivity ?? .0025)));
  let current: CombatControlSnapshot = { active, blocked, eligible, input: idleCombatInput(sequence), error, feedback };

  function acknowledge(action: CombatInputFeedback["action"], kind: CombatInputFeedback["kind"], message: string) {
    feedback = { id: ++feedbackId, action, kind, message };
    feedbackUntil = Date.now() + (kind === "received" ? 300 : 1400);
  }
  function attackAvailability() {
    const state = snapshot.state, fighter = state?.fighters.find(item => item.id === snapshot.playerId);
    if (!state || !fighter) return { wait: Infinity, reason: "Combat is unavailable." };
    const now = Date.now() + snapshot.serverOffset;
    if (fighter.robbery?.resolvedAt) return { wait: Infinity, reason: "Encounter ended." };
    // The host turns the first attack during a demand into the Defend choice.
    // Do not block that existing encounter transition with strike-only gates.
    if (fighter.robbery?.phase === "demanding") return { wait: 0, reason: "" };
    if (reload || fighter.reloadAt !== null) return { wait: fighter.reloadAt !== null && fighter.reloadAt > now ? fighter.reloadAt - now : Infinity, reason: "Reloading." };
    if (now < fighter.protectedUntil) return { wait: fighter.protectedUntil - now, reason: "Spawn protection is active." };
    const firearm = combatUsesFirearm(state, fighter);
    if (firearm && !fighter.ammo) return { wait: Infinity, reason: fighter.reserveAmmo ? "Magazine empty. Reload." : "Out of ammo." };
    if (!firearm && fighter.stamina < 12) return { wait: Infinity, reason: "Low stamina. Wait to recover." };
    const cooldown = combatWeapon(state, fighter)?.cooldownMs ?? (firearm ? state.tuning.fireCooldownMs : state.tuning.meleeCooldownMs);
    return { wait: Math.max(0, fighter.lastAttackAt + cooldown - now), reason: "Recovering. Attack again shortly." };
  }

  function input(): CombatInput {
    let right = 0, forward = 0;
    moves.forEach(value => { right += value[0]; forward += value[1]; });
    const length = Math.max(1, Math.hypot(right, forward));
    return { sequence, move: [right / length, forward / length], yaw, pitch,
      attack: attacks.size > 0 || attackEdge, block: blocks.size > 0, reload };
  }
  function publish() {
    const next = input(), previous = current.input;
    if (current.active === active && current.blocked === blocked && current.eligible === eligible && current.error === error && current.feedback === feedback
      && previous.sequence === next.sequence && previous.yaw === next.yaw && previous.pitch === next.pitch
      && previous.attack === next.attack && previous.block === next.block && previous.reload === next.reload
      && previous.move[0] === next.move[0] && previous.move[1] === next.move[1]) return;
    current = { active, blocked, eligible, input: next, error, feedback };
    listeners.forEach(listener => listener());
  }
  function release() {
    const needsStop = active || moves.size > 0 || attacks.size > 0 || blocks.size > 0 || attackEdge || reload;
    generation++;
    active = false; moves.clear(); attacks.clear(); blocks.clear(); attackEdge = false; reload = false; dirty = false;
    attackExpiresAt = 0; reloadExpiresAt = 0; feedback = null; heldDenial = "";
    awaitingAck = null;
    if (needsStop) store.stop();
    publish();
  }
  function sync(value: CombatSnapshot) {
    if (disposed) return;
    snapshot = value;
    const state = value.state, fighter = state?.fighters.find(item => item.id === value.playerId);
    const nextScope = JSON.stringify([state?.id, state?.instanceId, state?.layoutId, state?.navigation.revision, value.playerId]);
    eligible = value.ready && !value.busy && !value.error && state?.phase === "playing"
      && fighter?.kind === "player" && fighter.status === "active";
    if (scope !== nextScope) {
      scope = nextScope; release();
      yaw = yawLimit(finite(fighter?.yaw ?? 0)); pitch = pitchLimit(fighter?.pitch ?? 0);
      sequence = Math.max(0, (fighter?.lastInputSequence ?? -1) + 1); lastSend = -Infinity;
    } else if (fighter) sequence = Math.max(sequence, fighter.lastInputSequence + 1);
    if (awaitingAck && fighter && fighter.lastInputSequence >= awaitingAck.sequence) awaitingAck = null;
    if (!eligible && active) release();
    publish();
  }
  function canInput() { return active && eligible && !blocked && !disposed; }
  function frame(now: number) {
    if (!canInput() || !Number.isFinite(now)) return;
    const clock = Date.now();
    if (feedback && clock >= feedbackUntil) { feedback = null; publish(); }
    // Expiry runs even while transport is busy. A released tap cannot replay
    // after a slow request, backgrounding, a scope change or control release.
    if (attackEdge && clock > attackExpiresAt) {
      attackEdge = false;
      if (!attacks.size) acknowledge("attack", "denied", "Attack expired. Press again.");
      publish();
    }
    if (reload && clock > reloadExpiresAt) { reload = false; acknowledge("reload", "denied", "Reload expired. Press again."); publish(); }
    const availability = attackAvailability();
    if (attackEdge && availability.wait > 180) {
      attackEdge = false; acknowledge("attack", "denied", availability.reason); heldDenial = availability.reason; publish();
    }
    if (attacks.size && !Number.isFinite(availability.wait) && availability.reason !== heldDenial) {
      heldDenial = availability.reason; acknowledge("attack", "denied", availability.reason); publish();
    }
    // The embedded renderer's send promise only posts to the host. Wait for
    // the authoritative sequence too, so a busy host cannot silently drop the next tap.
    if (awaitingAck) {
      if (clock - awaitingAck.sentAt > 3000) {
        error = "Combat connection is delayed. Take control to retry.";
        release(); options.onError?.(error);
      }
      return;
    }
    if (inFlight) return;
    const value = input();
    value.attack = value.attack && availability.wait === 0;
    if (!(dirty || value.move.some(Boolean) || value.attack || value.block || value.reload)) return;
    if (now >= lastSend && now - lastSend < combatLimits.inputIntervalMs) return;
    if (sequence >= Number.MAX_SAFE_INTEGER - 1) { error = "Combat controls need to reconnect."; release(); return; }
    const stamp = generation;
    const command = { ...value, sequence: sequence++ };
    // Every one-shot edge needs a neutral follow-up; held reload must not restart later.
    lastSend = now; dirty = command.attack && attacks.size === 0 || command.reload;
    if (command.attack) {
      attackEdge = false; heldDenial = "";
      if (feedback?.action === "attack" && feedback.kind === "queued") acknowledge("attack", "received", "Attack");
    }
    reload = false; inFlight = true;
    awaitingAck = { sequence: command.sequence, sentAt: clock, reload: command.reload };
    publish();
    // Only one command may be in flight. The owner must drop queued input when stop() runs.
    void Promise.resolve().then(() => {
      if (disposed || stamp !== generation) return;
      return store.send(command);
    }).catch((reason: unknown) => {
      if (disposed || stamp !== generation) return;
      error = reason instanceof Error ? reason.message : "Combat controls could not connect. Try again.";
      release(); options.onError?.(error);
    }).finally(() => { inFlight = false; });
  }

  sync(snapshot);
  return {
    subscribe(listener: () => void) { listeners.add(listener); return () => { listeners.delete(listener); }; },
    getSnapshot: () => current,
    getInput: input,
    sync,
    start() {
      if (disposed) return false;
      sync(store.getSnapshot());
      if (!eligible || blocked) return false;
      // An explicit new start never replays keys, touches or a pending shot.
      release(); active = true; error = null; dirty = true; publish(); return true;
    },
    setBlocked(value: boolean) {
      if (disposed || blocked === value) return;
      blocked = value; release();
    },
    move(source: string, right: number, forward: number) {
      if (!canInput()) return;
      const x = finite(right), z = finite(forward), length = Math.max(1, Math.hypot(x, z));
      const previous = moves.get(source);
      if ((!previous && !x && !z) || previous && previous[0] === x / length && previous[1] === z / length) return;
      if (x || z) moves.set(source, [x / length, z / length]); else moves.delete(source);
      dirty = true; publish();
    },
    hold(source: string, action: "attack" | "block", held: boolean) {
      if (!canInput()) return;
      const set = action === "attack" ? attacks : blocks;
      if (set.has(source) === held) return;
      if (held) {
        if (action === "attack") {
          const availability = attackAvailability();
          heldDenial = "";
          if (attackEdge) acknowledge("attack", "queued", "Attack already queued.");
          else if (availability.wait <= 180) {
            attackEdge = true;
            // One tap may wait through the final 180ms of recovery and one
            // send interval. It never becomes an unbounded action queue.
            attackExpiresAt = Date.now() + availability.wait + combatLimits.inputIntervalMs + 50;
            acknowledge("attack", availability.wait > 0 || inFlight || awaitingAck ? "queued" : "received",
              availability.wait > 0 || inFlight || awaitingAck ? "Attack queued" : "Attack");
          } else { heldDenial = availability.reason; acknowledge("attack", "denied", availability.reason); }
        }
        set.add(source);
      }
      else set.delete(source);
      dirty = true; publish();
    },
    releaseSource(source: string) {
      if (disposed) return;
      const changed = moves.delete(source) || false;
      const attackChanged = attacks.delete(source), blockChanged = blocks.delete(source);
      if (changed || attackChanged || blockChanged) { dirty = true; publish(); }
    },
    aim(deltaX: number, deltaY: number) {
      if (!canInput() || !Number.isFinite(deltaX) || !Number.isFinite(deltaY)) return;
      // Positive pointer x turns right; positive y looks down.
      yaw = yawLimit(yaw + Math.max(-2000, Math.min(2000, deltaX)) * sensitivity);
      pitch = pitchLimit(pitch - Math.max(-2000, Math.min(2000, deltaY)) * sensitivity);
      dirty = true; publish();
    },
    reload() {
      if (!canInput() || !snapshot.state) return;
      const fighter = snapshot.state.fighters.find(item => item.id === snapshot.playerId);
      const reason = !fighter || !combatUsesFirearm(snapshot.state, fighter) ? "No firearm equipped."
        : reload || awaitingAck?.reload || fighter.reloadAt !== null ? "Already reloading."
        : fighter.ammo >= combatMagazine(snapshot.state, fighter) ? "Magazine full."
        : fighter.reserveAmmo <= 0 ? "No reserve ammo." : null;
      if (reason) { acknowledge("reload", "denied", reason); publish(); return; }
      attackEdge = false;
      reload = true; reloadExpiresAt = Date.now() + combatLimits.inputExpiryMs; dirty = true;
      acknowledge("reload", "received", "Reload requested"); publish();
    },
    setSensitivity(value: number) { if (Number.isFinite(value)) sensitivity = Math.max(.0002, Math.min(.02, value)); },
    frame,
    release,
    dispose() { if (disposed) return; release(); disposed = true; listeners.clear(); },
  };
}
