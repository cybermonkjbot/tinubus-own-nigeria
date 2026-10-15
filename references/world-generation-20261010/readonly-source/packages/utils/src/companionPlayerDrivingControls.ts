import { companionPlayerDrivingRules as rules, neutralCompanionDrivingInput,
  type CompanionDrivingCommand, type CompanionDrivingInput, type CompanionPlayerDrivingState,
  type CompanionDrivingAuthority, type CompanionDrivingHandling, type CompanionDrivingPose } from "./companionPlayerDriving";
import { advanceCompanionPlayerDriving, validCompanionDrivingInput } from "./companionPlayerDrivingMotion";
import type { CompanionPlayerVehicle } from "./companionPlayerVehicles";

export type CompanionDrivingControlSnapshot = Readonly<{
  accepted: CompanionPlayerDrivingState | null; input: CompanionDrivingInput;
  active: boolean; busy: boolean; retryable: boolean; error: string | null;
}>;
export type CompanionDrivingAcknowledgement = Readonly<{
  requestId: string; state: CompanionPlayerDrivingState;
}> | Readonly<{ requestId: string; rejected: string }>;

/** Input/prediction only. The existing world store owns transport, membership and accepted state. */
export function createCompanionDrivingControls(options: {
  send(command: CompanionDrivingCommand): Promise<CompanionDrivingAcknowledgement>;
  changed?(): void;
  requestId?(): string;
}) {
  const sources = new Map<string, CompanionDrivingInput>();
  const pulseSources = new Set<string>();
  const listeners = new Set<() => void>();
  let accepted: CompanionPlayerDrivingState | null = null, worldRevision = 0;
  let active = false, disposed = false, busy = false, retryable = false, error: string | null = null;
  let input = neutralCompanionDrivingInput, pending: CompanionDrivingCommand | null = null;
  let action: CompanionDrivingCommand["action"] | null = null, lastPump = -Infinity, generation = 0, awaitingSequence = 0;
  let lastHornAt = -Infinity;
  let cached: CompanionDrivingControlSnapshot | null = null;
  const changed = () => { cached = null; listeners.forEach(listener => listener()); options.changed?.(); };
  function merge() {
    let throttle = 0, brake = 0, steering = 0;
    sources.forEach(value => { throttle = Math.max(throttle, value.throttle); brake = Math.max(brake, value.brake); steering += value.steering; });
    input = active && sources.size ? Object.freeze({ throttle, brake, steering: Math.max(-1, Math.min(1, steering)) }) : neutralCompanionDrivingInput;
    changed();
  }
  function stop() {
    if (disposed) return;
    sources.clear(); pulseSources.clear(); input = neutralCompanionDrivingInput; action = accepted ? "stop" : null; changed();
  }
  function accept(state: CompanionPlayerDrivingState | null, revision: number) {
    if (disposed || !Number.isSafeInteger(revision) || revision < 0) return;
    const replacement = !state || !accepted || state.id !== accepted.id || state.driver.lifeId !== accepted.driver.lifeId
      || state.driver.userId !== accepted.driver.userId || state.instanceId !== accepted.instanceId;
    if (!replacement && state && accepted && (state.revision < accepted.revision || state.sequence < accepted.sequence
      || state.acceptedAt < accepted.acceptedAt || revision < worldRevision)) return;
    if (replacement) { generation++; pending = null; busy = retryable = false; error = null; action = null;
      awaitingSequence = 0; lastPump = lastHornAt = -Infinity; sources.clear(); pulseSources.clear(); input = neutralCompanionDrivingInput; }
    accepted = state; worldRevision = revision;
    if (!state || state.mode === "suspended") { active = false; sources.clear(); pulseSources.clear(); input = neutralCompanionDrivingInput; }
    changed();
  }
  async function dispatch(command: CompanionDrivingCommand) {
    if (disposed || busy) return;
    pending = command; busy = true; retryable = false; error = null;
    const stamp = generation; changed();
    try {
      const result = await options.send(command);
      if (disposed || stamp !== generation || pending !== command) return;
      if (result.requestId !== command.requestId) throw new Error("The driving result is unresolved. Retry the same command.");
      if ("rejected" in result) {
        error = result.rejected; pending = null; stop();
        if (command.action === "stop") { active = false; action = null; }
      }
      else {
        // World subscription applies current state. A replayed receipt cannot replace a later visit.
        if (!accepted || result.state.id !== command.drivingId || result.state.driver.lifeId !== command.lifeId
          || result.state.driver.userId !== accepted.driver.userId || result.state.vehicleId !== command.vehicleId
          || result.state.sequence !== command.sequence || result.state.revision !== command.drivingRevision + 1
          || result.state.vehicleRevision !== command.vehicleRevision + 1 || result.state.instanceId !== command.instanceId
          || result.state.gameRoomId !== command.gameRoomId)
          throw new Error("Refresh the current driving state.");
        awaitingSequence = command.sequence; pending = null;
      }
    } catch (reason) {
      if (disposed || stamp !== generation || pending !== command) return;
      retryable = true; error = reason instanceof Error ? reason.message : "Driving couldn't update. Retry this command.";
      sources.clear(); pulseSources.clear(); input = neutralCompanionDrivingInput; action = accepted ? "stop" : null;
    } finally { if (!disposed && stamp === generation) { busy = false; changed(); } }
  }
  function pump(now: number, sampleIdle = false) {
    if (disposed || !accepted || busy || retryable || accepted.sequence < awaitingSequence
      || !Number.isFinite(now) || action !== "stop" && now - lastPump < rules.stepMs) return;
    if (!active && !action) return;
    const nextAction = action ?? "input";
    if (!sampleIdle && nextAction === "input" && input.throttle === 0 && input.steering === 0 && accepted.speed === 0
      && accepted.input.throttle === 0 && accepted.input.steering === 0) return;
    const command: CompanionDrivingCommand = Object.freeze({ drivingId: accepted.id, vehicleId: accepted.vehicleId,
      vehicleRevision: accepted.vehicleRevision, drivingRevision: accepted.revision, gameRoomId: accepted.gameRoomId,
      instanceId: accepted.instanceId, lifeId: accepted.driver.lifeId, worldRevision, sequence: accepted.sequence + 1,
      requestId: options.requestId?.() ?? `drive-${Date.now().toString(36)}-${Math.random().toString(36).slice(2)}`,
      action: nextAction, input: Object.freeze({ ...input }) });
    action = null; lastPump = now;
    if (nextAction === "input" && pulseSources.size) {
      pulseSources.forEach(source => sources.delete(source)); pulseSources.clear(); merge();
    }
    void dispatch(command);
  }
  function hold(source: string, value: CompanionDrivingInput) {
    if (disposed || !active || retryable || !validCompanionDrivingInput(value) || !source || source.length > 80) return false;
    if (!sources.has(source) && sources.size >= 12) return false;
    sources.set(source, Object.freeze({ ...value })); merge(); return true;
  }
  return {
    getSnapshot(): CompanionDrivingControlSnapshot { return cached ??= Object.freeze({ accepted, input, active, busy, retryable, error }); },
    subscribe(listener: () => void) { listeners.add(listener); return () => listeners.delete(listener); },
    accept,
    setActive(value: boolean) { if (disposed || active === value) return; active = value && !!accepted && accepted.mode !== "suspended"; stop(); },
    hold,
    /** A screen-reader activation queues one input step, including while a prior command is in flight. */
    pulse(source: string, value: CompanionDrivingInput) { if (hold(source, value)) pulseSources.add(source); },
    release(source: string) { pulseSources.delete(source); if (sources.delete(source)) merge(); },
    stop,
    action(value: "horn" | "pull-over" | "exit" | "stop") {
      if (disposed || !accepted || retryable && value !== "stop" || busy && value !== "stop" || value === "horn" && !active) return;
      if (value === "horn") {
        const now = Date.now();
        if (now < lastHornAt || now - lastHornAt < 900) return;
        lastHornAt = now;
      }
      if (value !== "horn") stop(); action = value; changed();
    },
    pump,
    retry() { if (pending && retryable && !busy) return dispatch(pending); return Promise.resolve(); },
    predict(vehicle: CompanionPlayerVehicle, authority: CompanionDrivingAuthority, handling?: CompanionDrivingHandling): CompanionDrivingPose | null {
      if (!accepted || !active || retryable || authority.now < accepted.acceptedAt
        || authority.now - accepted.acceptedAt > rules.maxPredictionMs) return accepted?.pose ?? null;
      try {
        const command: CompanionDrivingCommand = { drivingId: accepted.id, vehicleId: vehicle.vehicleId, vehicleRevision: vehicle.revision,
          drivingRevision: accepted.revision, gameRoomId: accepted.gameRoomId, instanceId: accepted.instanceId,
          lifeId: accepted.driver.lifeId, worldRevision: authority.worldRevision, sequence: accepted.sequence + 1,
          requestId: "local-prediction", action: "input", input };
        return advanceCompanionPlayerDriving(vehicle, { ...accepted, input, inputExpiresAt: accepted.acceptedAt + rules.inputTtlMs },
          command, authority, handling, true).state.pose;
      } catch { return accepted.pose; }
    },
    dispose() { if (disposed) return; stop(); disposed = true; generation++; active = false; pending = null; sources.clear(); listeners.clear(); },
  };
}
export type CompanionDrivingControls = ReturnType<typeof createCompanionDrivingControls>;
