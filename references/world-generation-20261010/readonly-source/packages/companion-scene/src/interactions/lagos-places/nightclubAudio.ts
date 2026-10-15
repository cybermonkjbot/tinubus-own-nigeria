import { nightclubLimits } from "../../../../../packages/utils/src/companionNightclub";
import type { WorldMusicOutput } from "../audio/worldRadio";

/** Original local synth, not streamed music. Runs only after an explicit listen action. */
export function createNightclubAudio(output: () => WorldMusicOutput | null, resume: () => Promise<boolean>) {
  let context: AudioContext | null = null, master: GainNode | null = null, disposed = false, generation = 0;
  let placeId = "", startedAt = 0, lastStep = -1, enabled = false;
  const active = new Set<OscillatorNode>();
  const silence = () => {
    active.forEach(node => { try { node.stop(); } catch { /* A completed voice is already silent. */ } });
    active.clear(); lastStep = -1;
  };
  function tone(frequency: number, at: number, duration: number, gain: number, kind: OscillatorType, endFrequency?: number) {
    if (!context || !master || active.size >= 24) return;
    const node = context.createOscillator(), envelope = context.createGain();
    node.type = kind; node.frequency.setValueAtTime(frequency, at);
    if (endFrequency) node.frequency.exponentialRampToValueAtTime(endFrequency, at + duration);
    envelope.gain.setValueAtTime(.0001, at);
    envelope.gain.exponentialRampToValueAtTime(gain, at + .012);
    envelope.gain.exponentialRampToValueAtTime(.0001, at + duration);
    node.connect(envelope); envelope.connect(master); active.add(node);
    node.onended = () => { active.delete(node); node.disconnect(); envelope.disconnect(); };
    node.start(at); node.stop(at + duration + .02);
  }
  return {
    async listen(instanceId: string, sharedStartedAt: number) {
      if (disposed) throw new Error("The nightclub sound is unavailable.");
      const token = ++generation;
      silence(); placeId = instanceId; startedAt = sharedStartedAt;
      try {
        const running = await resume(), destination = output();
        if (disposed || token !== generation) return false;
        if (!running || !destination) throw new Error("Enable game sound, then tap Music again.");
        if (context !== destination.context) {
          master?.disconnect(); context = destination.context; master = context.createGain(); master.gain.value = 1;
          master.connect(destination.input);
        }
        enabled = true;
        return true;
      } catch (error) { if (token === generation) enabled = false; throw error; }
    },
    get enabled() { return enabled && !disposed && context?.state === "running"; },
    frame(instanceId: string | null, sharedNow: number, visible: boolean) {
      if (disposed || !enabled || !context || context.state !== "running" || !output() || !visible || instanceId !== placeId) {
        if (active.size) silence();
        return;
      }
      const stepMs = nightclubLimits.beatMs / 2;
      // Schedule at most the next two subdivisions. Hidden time is never replayed.
      const next = Math.ceil((sharedNow - startedAt) / stepMs);
      if (lastStep < next - 2) lastStep = next - 1;
      for (let step = Math.max(next, lastStep + 1); step <= next + 1; step++) {
        const at = context.currentTime + Math.max(.01, (startedAt + step * stepMs - sharedNow) / 1000);
        if (step % 2 === 0) {
          tone(110, at, .16, .55, "sine", 42);
          const bass = [55, 55, 65.41, 73.42][Math.floor(step / 8) % 4];
          tone(bass, at + .025, .22, .13, "triangle");
        }
        if (step % 4 === 2) tone(230, at, .07, .07, "triangle", 140);
        tone(1700, at, .026, .025, "square", 1100);
        if (step % 4 === 1) tone([220, 261.63, 293.66, 329.63][Math.floor(step / 4) % 4], at, .16, .065, "sine");
        lastStep = step;
      }
    },
    stop() { generation++; enabled = false; silence(); },
    dispose() {
      if (disposed) return;
      disposed = true; generation++; enabled = false; silence(); master?.disconnect(); master = null;
      context = null;
    },
  };
}
