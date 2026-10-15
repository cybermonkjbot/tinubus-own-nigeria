import { useSyncExternalStore } from "react";

const active = new Map<string, { downloadedBytes: number; totalBytes: number }>();
const listeners = new Set<() => void>();
let snapshot: { downloadedBytes: number; totalBytes: number } | null = null;

export function reportResourceDownload(uri: string, progress: { downloadedBytes: number; totalBytes: number } | null) {
  if (progress) active.set(uri, progress);
  else active.delete(uri);
  snapshot = active.size ? [...active.values()].reduce((sum, value) => ({
    downloadedBytes: sum.downloadedBytes + value.downloadedBytes,
    totalBytes: sum.totalBytes + value.totalBytes,
  }), { downloadedBytes: 0, totalBytes: 0 }) : null;
  listeners.forEach(listener => listener());
}

export const subscribeResourceDownloadProgress = (listener: () => void) => { listeners.add(listener); return () => { listeners.delete(listener); }; };
export const getResourceDownloadProgress = () => snapshot;

export function useResourceDownloadProgress() {
  return useSyncExternalStore(subscribeResourceDownloadProgress, getResourceDownloadProgress, () => null);
}

export function ResourceDownloadState() {
  const progress = useResourceDownloadProgress();
  if (!progress) return null;
  return <div className="resource-download-state" role="status" style={{ position: "absolute", inset: 0, zIndex: 10, background: "#ffffff", color: "#292a2f",
    display: "flex", alignItems: "center", justifyContent: "center", padding: 24, boxSizing: "border-box" }}>
    <div style={{ width: "100%", maxWidth: 320 }}>
    <style>{`[data-resource-downloading="true"] > :not(.resource-download-state) { visibility: hidden; pointer-events: none; }
      [data-resource-downloading="true"] > .resource-download-state { visibility: visible; }`}</style>
    <div style={{ fontSize: 18, fontWeight: 600, marginBottom: 12 }}>Downloading resources</div>
    <progress aria-label="Downloading 3D resources" value={progress.totalBytes ? progress.downloadedBytes : undefined}
      max={progress.totalBytes || 1} style={{ width: "100%", accentColor: "#292a2f" }} />
    <div style={{ marginTop: 8 }}>{(progress.downloadedBytes / 1_000_000).toFixed(1)} MB{progress.totalBytes ? ` / ${(progress.totalBytes / 1_000_000).toFixed(1)} MB` : ""}</div>
    </div>
  </div>;
}
