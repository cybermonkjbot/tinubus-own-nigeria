type PendingPreparation = { start(): void; cancel(): void };

/** One allocation-heavy job at a time. Waiting jobs must not allocate asset bytes. */
export function createCompanionAssetPreparationQueue() {
  const pending: PendingPreparation[] = [];
  let preparing = false;
  let scheduled = false;
  const cancelled = (signal: AbortSignal) => signal.reason ?? new DOMException("Asset request cancelled", "AbortError");

  function schedule() {
    if (preparing || scheduled || pending.length === 0) return;
    scheduled = true;
    // Let transfers, input and a displayed frame finish before the next job.
    setTimeout(() => {
      scheduled = false;
      if (!preparing) pending.shift()?.start();
    }, 0);
  }

  return function prepare<T>(signal: AbortSignal, work: () => Promise<T>): Promise<T> {
    return new Promise((resolve, reject) => {
      if (signal.aborted) { reject(cancelled(signal)); return; }
      let started = false;
      const job: PendingPreparation = {
        start() {
          if (signal.aborted) { job.cancel(); schedule(); return; }
          started = true;
          preparing = true;
          signal.removeEventListener("abort", job.cancel);
          // Cancellation cannot free the slot while an active decoder or read
          // still owns its temporary buffers. The job receives the same signal.
          Promise.resolve().then(work).then(resolve, reject).finally(() => {
            preparing = false;
            schedule();
          });
        },
        cancel() {
          if (started) return;
          const index = pending.indexOf(job);
          if (index !== -1) pending.splice(index, 1);
          signal.removeEventListener("abort", job.cancel);
          reject(cancelled(signal));
        },
      };
      signal.addEventListener("abort", job.cancel, { once: true });
      pending.push(job);
      schedule();
    });
  };
}
