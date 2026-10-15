/** Cache reads have no fetch signal, so cancel their reader explicitly. */
export async function readCompanionAssetResponse(response: Response, expectedBytes: number, signal: AbortSignal,
  onProgress?: (downloadedBytes: number) => void): Promise<ArrayBuffer> {
  signal.throwIfAborted();
  if (!response.body) {
    const bytes = await response.arrayBuffer();
    signal.throwIfAborted();
    return bytes;
  }
  const reader = response.body.getReader();
  let cancellation: Promise<void> | undefined;
  const cancel = () => { cancellation ??= reader.cancel(signal.reason).catch(() => {}); };
  signal.addEventListener("abort", cancel, { once: true });
  try {
    signal.throwIfAborted();
    let result: Uint8Array<ArrayBuffer> | undefined;
    let offset = 0;
    while (true) {
      const chunk = await reader.read();
      signal.throwIfAborted();
      if (chunk.done) break;
      if (offset + chunk.value.byteLength > expectedBytes) throw new Error("Invalid asset download size.");
      if (chunk.value.byteLength) {
        // Waiting/empty reads allocate nothing. Allocation failure still cleans up.
        result ??= new Uint8Array(expectedBytes);
        result.set(chunk.value, offset);
      }
      offset += chunk.value.byteLength; onProgress?.(offset);
    }
    if (offset !== expectedBytes) throw new Error("The asset download is incomplete.");
    return result?.buffer ?? new ArrayBuffer(0);
  } catch (error) { cancel(); throw error; }
  finally {
    signal.removeEventListener("abort", cancel);
    // Keep the model-preparation slot until the underlying read has stopped.
    if (cancellation) await cancellation;
    reader.releaseLock();
  }
}

/** Keep the verified buffer owned by the caller until storage finishes or rejects. */
export async function cacheCompanionModelBytes(cache: Cache, uri: string, bytes: ArrayBuffer, signal: AbortSignal,
  contentType = "application/octet-stream") {
  signal.throwIfAborted();
  const headers = { "Content-Type": contentType };
  if (typeof ReadableStream === "undefined") {
    await cache.put(uri, new Response(bytes, { headers }));
    return;
  }
  let source: ArrayBuffer | null = bytes, offset = 0;
  let writer: ReadableStreamDefaultController<Uint8Array> | null = null;
  const stop = () => {
    source = null; writer?.error(signal.reason ?? new DOMException("Asset download cancelled.", "AbortError")); writer = null;
  };
  const body = new ReadableStream<Uint8Array>({
    start(controller) { writer = controller; },
    pull(controller) {
      if (!source) { controller.close(); writer = null; return; }
      const end = Math.min(offset + 256 * 1024, source.byteLength);
      // Independent chunks also bound copies made by Cache/Response cloning.
      // A subarray could cause a structured clone of the entire model buffer.
      const chunk = new Uint8Array(source.slice(offset, end));
      offset = end;
      controller.enqueue(chunk);
      if (offset === source.byteLength) { source = null; controller.close(); writer = null; }
    },
    cancel() { source = null; writer = null; },
  }, { highWaterMark: 0 });
  signal.addEventListener("abort", stop, { once: true });
  try {
    if (signal.aborted) stop();
    await cache.put(uri, new Response(body, { headers }));
  } finally {
    signal.removeEventListener("abort", stop);
    // A rejected write must not retain the model or pull from transferred bytes.
    stop();
  }
}
