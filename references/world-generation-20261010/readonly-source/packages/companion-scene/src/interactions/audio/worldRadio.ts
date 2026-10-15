import catalog from "./radioStations.json";

export const worldRadioStations = catalog.stations;
export type WorldRadioState = { stationId: string | null; status: "off" | "connecting" | "playing" | "paused" | "error"; error: string | null };
export type WorldMusicOutput = { context: AudioContext; input: GainNode };

/** One user-selected live stream. No prefetch, metadata polling, proxy or automatic retry. */
export function createWorldRadio(output: () => WorldMusicOutput | null, resume: () => Promise<boolean>) {
  let state: WorldRadioState = { stationId: null, status: "off", error: null };
  const listeners = new Set<() => void>();
  let media: HTMLAudioElement | null = null, source: MediaElementAudioSourceNode | null = null;
  let timeout: ReturnType<typeof setTimeout> | null = null, generation = 0, disposed = false;
  const publish = (next: WorldRadioState) => { state = next; for (const listener of listeners) listener(); };
  const clearTimeoutIfNeeded = () => { if (timeout !== null) clearTimeout(timeout); timeout = null; };
  function release() {
    clearTimeoutIfNeeded();
    if (media) {
      media.onplaying = media.onwaiting = media.onstalled = media.onerror = media.onended = null;
      media.pause(); media.removeAttribute("src"); media.load(); media = null;
    }
    source?.disconnect(); source = null;
  }
  function pause() {
    generation++; release();
    if (state.status === "playing" || state.status === "connecting") publish({ ...state, status: "paused", error: null });
  }
  const visibility = () => { if (document.hidden) pause(); };
  document.addEventListener("visibilitychange", visibility);
  window.addEventListener("pagehide", pause);
  return {
    subscribe(listener: () => void) { listeners.add(listener); return () => { listeners.delete(listener); }; },
    getSnapshot: () => state,
    async play(stationId: string) {
      const station = worldRadioStations.find(value => value.id === stationId);
      if (disposed || document.hidden || !station) return;
      // resume creates the shared context synchronously inside this click's activation.
      const unlocking = resume();
      const token = ++generation;
      release();
      publish({ stationId, status: "connecting", error: null });
      const current = () => !disposed && generation === token && !document.hidden;
      const fail = (message: string) => {
        if (!current()) return;
        generation++; release(); publish({ stationId, status: "error", error: message });
      };
      try {
        const destination = output();
        if (!destination) { await unlocking; throw new Error("Game audio is unavailable."); }
        const audio = new Audio();
        audio.crossOrigin = "anonymous"; audio.preload = "none";
        audio.src = station.stream;
        media = audio; source = destination.context.createMediaElementSource(audio); source.connect(destination.input);
        const buffering = () => {
          if (!current()) return;
          if (state.status !== "connecting") publish({ stationId, status: "connecting", error: null });
          if (timeout === null) timeout = setTimeout(() => fail("The station did not connect. Retry or choose another station."), 12_000);
        };
        audio.onplaying = () => {
          if (!current()) return;
          clearTimeoutIfNeeded(); publish({ stationId, status: "playing", error: null });
        };
        audio.onwaiting = audio.onstalled = buffering;
        audio.onerror = () => fail("This station is unavailable. Retry or choose another station.");
        audio.onended = () => fail("The broadcast ended. Retry or choose another station.");
        buffering();
        const [running] = await Promise.all([unlocking, audio.play()]);
        if (!current()) return;
        if (!running) fail("Tap Play again to enable game audio.");
      } catch {
        void unlocking.catch(() => {});
        fail("Radio could not start. Check your connection and tap Play again.");
      }
    },
    pause,
    stop() { generation++; release(); publish({ stationId: null, status: "off", error: null }); },
    dispose() {
      if (disposed) return;
      disposed = true; generation++; release(); listeners.clear();
      document.removeEventListener("visibilitychange", visibility); window.removeEventListener("pagehide", pause);
    },
  };
}
