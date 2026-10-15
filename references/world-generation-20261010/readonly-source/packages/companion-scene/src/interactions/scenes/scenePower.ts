import type { CompanionWorldPower } from "../../../../../packages/utils/src/companionWorldUtilities";

/** Register only instance-owned, explicitly mains-powered targets. Animated
 * lighting owners consume the adapter's factor instead of registering targets. */
export type ScenePowerBinding =
  | Readonly<{ kind: "light"; target: { intensity: number } }>
  | Readonly<{ kind: "emissive"; target: { emissiveIntensity: number } }>
  | Readonly<{ kind: "uniform"; target: { value: number } }>;

export type ScenePowerAdapter = {
  readonly factor: number;
  update: (power: CompanionWorldPower, sharedNowRealMs: number, reducedMotion?: boolean, visible?: boolean) => boolean;
  /** Pass the configured full-power value after a saved lighting choice changes. */
  setBaseline: (binding: ScenePowerBinding, value: number) => boolean;
  /** Restore configured baselines before disposing the owning scene resources. */
  dispose: () => void;
};

const transitionRealMs = 900;
const sampleRealMs = 50;
const maximumBindings = 128;
const validTime = (value: number) => Number.isSafeInteger(value) && value >= 0;
const validScalar = (value: number) => Number.isFinite(value) && value >= 0;

/** Absolute shared-time fade. Late joins and resumed views use the current
 * transition position, never a new local fade. Null retains the last factor. */
export function projectScenePowerFactor(power: CompanionWorldPower, sharedNowRealMs: number, reducedMotion = false): number | null {
  if (!power || (power.status !== "on" && power.status !== "outage")
    || typeof power.eventId !== "string" || power.eventId.length === 0
    || !validTime(sharedNowRealMs) || !validTime(power.startedAtRealMs)
    || !validTime(power.endsAtRealMs) || power.nextChangeAtRealMs !== power.endsAtRealMs
    || power.startedAtRealMs >= power.endsAtRealMs || sharedNowRealMs < power.startedAtRealMs
    || sharedNowRealMs >= power.endsAtRealMs) return null;
  const on = power.status === "on";
  if (reducedMotion || on && power.eventId.endsWith(":initial")) return on ? 1 : 0;
  const elapsed = Math.floor((sharedNowRealMs - power.startedAtRealMs) / sampleRealMs) * sampleRealMs;
  const progress = Math.min(1, elapsed / transitionRealMs);
  const fade = progress * progress * (3 - 2 * progress);
  return on ? fade : 1 - fade;
}

function read(binding: ScenePowerBinding) {
  switch (binding.kind) {
    case "light": return binding.target.intensity;
    case "emissive": return binding.target.emissiveIntensity;
    case "uniform": return binding.target.value;
  }
}

function write(binding: ScenePowerBinding, value: number): boolean {
  if (read(binding) === value) return false;
  switch (binding.kind) {
    case "light": binding.target.intensity = value; break;
    case "emissive": binding.target.emissiveIntensity = value; break;
    case "uniform": binding.target.value = value; break;
  }
  return true;
}

/** No traversal, timers, visibility edits, material cloning or resource disposal.
 * Mount after the room's saved lighting has configured these owned targets. */
export function createScenePowerAdapter(bindings: readonly ScenePowerBinding[] = []): ScenePowerAdapter {
  if (bindings.length > maximumBindings) throw new Error("Scene power exceeds its binding budget.");
  const targets = new Map<object, Set<ScenePowerBinding["kind"]>>();
  const channels = bindings.map(binding => {
    const kinds = targets.get(binding.target) ?? new Set<ScenePowerBinding["kind"]>();
    if (kinds.has(binding.kind)) throw new Error("Register each powered scalar once.");
    kinds.add(binding.kind); targets.set(binding.target, kinds);
    const baseline = read(binding);
    if (!validScalar(baseline)) throw new Error("Powered scalar baselines must be finite and nonnegative.");
    return { binding, baseline };
  });
  targets.clear();
  let factor = 1;
  let sampledAtRealMs = -1;
  let disposed = false;
  return {
    get factor() { return factor; },
    update(power, sharedNowRealMs, reducedMotion = false, visible = true) {
      if (disposed || !visible || sharedNowRealMs < sampledAtRealMs) return false;
      const next = projectScenePowerFactor(power, sharedNowRealMs, reducedMotion);
      if (next === null) return false;
      sampledAtRealMs = sharedNowRealMs;
      if (next === factor) return false;
      factor = next;
      for (const channel of channels) write(channel.binding, channel.baseline * factor);
      return true;
    },
    setBaseline(binding, value) {
      if (disposed) return false;
      if (!validScalar(value)) throw new Error("Powered scalar baselines must be finite and nonnegative.");
      const channel = channels.find(candidate => candidate.binding === binding);
      if (!channel) throw new Error("Update only a registered scene-power binding.");
      channel.baseline = value;
      return write(channel.binding, value * factor);
    },
    dispose() {
      if (disposed) return;
      disposed = true;
      for (const channel of channels) write(channel.binding, channel.baseline);
      factor = 1;
      channels.length = 0;
    },
  };
}
