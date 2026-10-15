import type { MeshoptDecoder } from "three/addons/libs/meshopt_decoder.module.js";
import decoderSource from "three/addons/libs/meshopt_decoder.module.js?raw";
import { createMeshoptDecoder } from "./meshoptDecoderFactory.js";

type DecodeArguments = Parameters<typeof MeshoptDecoder.decodeGltfBufferAsync>;
const closedError = () => new Error("The 3D mesh decoder is closed.");
const exportDeclaration = /\bexport\s*\{\s*MeshoptDecoder\s*\};?\s*$/;
// Keep the pinned decoder and its licence intact inside a classic blob worker.
// No remote script, module worker or additional worker pool is required.
const workerHandler = `
MeshoptDecoder.ready?.catch(() => {});
self.onmessage = async event => {
  const { count, size, source, mode, filter } = event.data;
  try {
    if (!MeshoptDecoder.supported) throw new Error("Mesh decoding is unavailable.");
    await MeshoptDecoder.ready;
  } catch {
    self.postMessage({ unavailable: true });
    return;
  }
  try {
    const result = await MeshoptDecoder.decodeGltfBufferAsync(count, size, source, mode, filter);
    self.postMessage({ result }, [result.buffer]);
  } catch (error) {
    self.postMessage({ error: error instanceof Error ? error.message : "The 3D mesh couldn't be decoded." });
  }
};`;

/** One parse owns the worker and its WASM heap. Uncompressed assets create none. */
export function createMeshoptPreparation(signal: AbortSignal) {
  let worker: Worker | undefined;
  let fallbackDecoder: typeof MeshoptDecoder | undefined;
  let pending = 0;
  let idleTimer: ReturnType<typeof setTimeout> | undefined;
  let yieldTimer: ReturnType<typeof setTimeout> | undefined;
  let resumeFallback: (() => void) | undefined;
  let fallbackDeadline = 0;
  let workerUnavailable = false, closed = false;
  // The parse owns both decoder paths. Packaging verifies that this lazy
  // factory preserves the pinned decoder body and licence.
  const decodeFallback = async (...args: DecodeArguments) => {
    // A promise-only decode queue can occupy one long main-thread task. Yield
    // before starting fallback work and between four-millisecond batches.
    // A single WASM decode is indivisible and may exceed that scheduling target.
    if (performance.now() >= fallbackDeadline) {
      await new Promise<void>(resolve => {
        const resume = () => { yieldTimer = undefined; resumeFallback = undefined; resolve(); };
        resumeFallback = resume;
        yieldTimer = setTimeout(resume, 0);
      });
      fallbackDeadline = performance.now() + 4;
    }
    if (closed) throw closedError();
    const decoder = fallbackDecoder ??= createMeshoptDecoder();
    await decoder.ready;
    if (closed) throw closedError();
    return decoder.decodeGltfBufferAsync(...args);
  };
  let rejectActive: ((error: Error) => void) | undefined;
  let tail = Promise.resolve();
  const clearCallbacks = () => {
    if (worker) worker.onmessage = worker.onerror = worker.onmessageerror = null;
  };
  const stopWorker = () => {
    clearCallbacks(); worker?.terminate(); worker = undefined;
  };
  const clearIdleTimer = () => { clearTimeout(idleTimer); idleTimer = undefined; };
  const finishJob = () => {
    pending -= 1;
    if (pending || closed || !worker && !fallbackDecoder) return;
    // Images may continue parsing after the last mesh. Keep scratch state for
    // short dependency gaps, then release it without waiting for the whole GLB.
    idleTimer = setTimeout(() => {
      idleTimer = undefined;
      stopWorker(); fallbackDecoder = undefined; fallbackDeadline = 0;
    }, 1000);
  };
  const getWorker = () => {
    if (worker) return worker;
    if (typeof Worker === "undefined" || !exportDeclaration.test(decoderSource)) throw new Error("Mesh workers are unavailable.");
    const blob = new Blob([decoderSource.replace(exportDeclaration, ""), workerHandler], { type: "text/javascript" });
    const url = URL.createObjectURL(blob);
    try { worker = new Worker(url); }
    finally { URL.revokeObjectURL(url); }
    return worker;
  };
  const decode = (...args: DecodeArguments): Promise<Uint8Array> => {
    if (workerUnavailable) return decodeFallback(...args);
    return new Promise((resolve, reject) => {
      let settled = false;
      const finish = (result?: Uint8Array | Promise<Uint8Array>, error?: Error) => {
        if (settled) return;
        settled = true; clearCallbacks(); rejectActive = undefined;
        if (error) reject(error); else resolve(result!);
      };
      const fallback = () => {
        if (settled) return;
        workerUnavailable = true; stopWorker();
        try { finish(decodeFallback(...args)); }
        catch (error) { finish(undefined, error instanceof Error ? error : new Error("The 3D mesh couldn't be decoded.")); }
      };
      rejectActive = error => finish(undefined, error);
      try {
        const target = getWorker();
        target.onmessage = event => {
          const message = event.data;
          if (message?.unavailable) { fallback(); return; }
          if (typeof message?.error === "string") { finish(undefined, new Error(message.error)); return; }
          const result = message?.result;
          if (!(result instanceof Uint8Array) || result.byteLength !== args[0] * args[1]) { fallback(); return; }
          finish(result);
        };
        target.onerror = event => { event.preventDefault(); fallback(); };
        target.onmessageerror = fallback;
        // Transfer only this compressed range. Never detach the parser's shared
        // GLB body, and do not copy queued ranges until the worker can take them.
        const [count, size, source, mode, filter] = args;
        const input = new Uint8Array(source);
        target.postMessage({ count, size, source: input, mode, filter }, [input.buffer]);
      } catch { fallback(); }
    });
  };
  const close = () => {
    if (closed) return;
    closed = true;
    signal.removeEventListener("abort", close);
    clearIdleTimer(); clearTimeout(yieldTimer); resumeFallback?.();
    rejectActive?.(closedError()); stopWorker();
  };
  // This signal belongs to the shared source, not an individual scene. Only
  // its last lease cancels queued ranges and the parse-owned worker.
  signal.addEventListener("abort", close, { once: true });
  if (signal.aborted) close();
  return {
    decoder: {
      // r180's async meshopt path reads only supported and this decode method.
      // Match the pinned decoder's capability check without initializing WASM.
      supported: typeof WebAssembly === "object",
      decodeGltfBufferAsync(...args: DecodeArguments) {
        if (closed) return Promise.reject(closedError());
        clearIdleTimer(); pending += 1;
        const result = tail.then(() => {
          if (closed) throw closedError();
          return decode(...args);
        });
        // Keep no decoded buffer in the scheduling tail. A malformed buffer
        // rejects its caller without leaving the remaining queue unsettled.
        tail = result.then(finishJob, finishJob);
        return result;
      },
    },
    async dispose() {
      close();
      // A main-thread fallback cannot be interrupted. Hold the preparation slot
      // until it settles; later queued decodes observe closed before allocating.
      await tail;
      fallbackDecoder = undefined;
    },
  };
}
