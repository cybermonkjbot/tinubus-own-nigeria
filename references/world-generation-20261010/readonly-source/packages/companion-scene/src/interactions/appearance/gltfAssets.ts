import * as THREE from "three";
import { GLTFLoader } from "three/addons/loaders/GLTFLoader.js";
import { clone as cloneSkinned } from "three/addons/utils/SkeletonUtils.js";
import { partitionStaticGeometry } from "../../../../../packages/character-runtime/src/spatialGeometry";
import { companionAssetEntries, type CompanionAssetDelivery, type CompanionAssetKey } from "../../../../../packages/utils/src/companionAssets";
import { cacheCompanionModelBytes, readCompanionAssetResponse } from "../../../../../packages/utils/src/companionAssetStreams";
import { reportResourceDownload } from "./ResourceDownloadState";
import { prepareAsset } from "./assetPreparation";
import { shareClonedSkeletons } from "../../../../../packages/character-runtime/src/skeletonSharing";
import { rendererTexturePreparation, type TexturePreparation } from "./gpuTextureDelivery";
import { createMeshoptPreparation } from "./meshoptPreparation";
import { createImagePreparation } from "./imagePreparation";
import { parseEmbeddedGlb } from "./embeddedGlb";
import { clearDisposedSceneGraph, createSceneAssetCache, disposeSceneAsset, retainSceneAsset, type SceneAsset } from "../../../../../packages/character-runtime/src/sceneAsset";

export type { SceneAsset };
export type GltfLease = { promise: Promise<SceneAsset>; release(): void };
type Entry = { users: number; controller: AbortController; promise: Promise<SceneAsset>; asset?: SceneAsset; releaseSpatial?: () => void };
const entries = new Map<string, Entry>();
// Decoded source graphs stay immutable after preparation. Cache only the
// classification, without keeping a released source or its resources alive.
const skinnedSources = createSceneAssetCache<boolean>();
const aborted = () => new DOMException("Asset request cancelled", "AbortError");

export const disposeGltf = disposeSceneAsset;

/** Clone an immutable decoded source. Pose, skin, weights and materials stay independent. */
export function instantiateGltf(asset: Pick<SceneAsset, "scene">) {
  let skinned = skinnedSources.get(asset.scene);
  if (skinned === undefined) {
    skinned = asset.scene.getObjectByProperty("isSkinnedMesh", true) !== undefined;
    skinnedSources.set(asset.scene, skinned);
  }
  // SkeletonUtils builds two node lookup maps and remaps bones. Static props,
  // rooms and articulated vehicles need only the same ordinary graph clone.
  const scene = skinned ? cloneSkinned(asset.scene) : asset.scene.clone();
  if (!(scene instanceof THREE.Group)) throw new Error("The cloned 3D asset has no scene group.");
  if (skinned) shareClonedSkeletons(scene);
  const materials = new Map<THREE.Material, THREE.Material>();
  const copyMaterial = (source: THREE.Material) => {
    let value = materials.get(source);
    if (!value) { value = source.clone(); materials.set(source, value); }
    return value;
  };
  scene.traverse(node => {
    if (!(node instanceof THREE.Mesh)) return;
    // Three's Mesh.copy already owns independent weights and material arrays.
    // Replace that array's entries rather than allocating another copy.
    if (Array.isArray(node.material)) {
      for (let i = 0; i < node.material.length; i++) node.material[i] = copyMaterial(node.material[i]);
    } else node.material = copyMaterial(node.material);
  });
  let disposed = false;
  return {
    scene,
    dispose() {
      if (disposed) return;
      disposed = true;
      const skeletons = new Set<THREE.Skeleton>();
      const instances = new Set<THREE.InstancedMesh>();
      scene.traverse(node => {
        if (node instanceof THREE.SkinnedMesh) skeletons.add(node.skeleton);
        if (node instanceof THREE.InstancedMesh) instances.add(node);
      });
      skeletons.forEach(value => value.dispose());
      instances.forEach(value => value.dispose());
      materials.forEach(value => value.dispose());
      materials.clear();
      scene.removeFromParent();
      // A retained disposed instance must not keep its mesh/bone tree alive.
      // Shared source geometry and textures still belong to the asset lease.
      clearDisposedSceneGraph(scene);
    },
  };
}

type Integrity = { sha256: string; bytes: number };
type AssetRead = { bytes: ArrayBuffer; verified: boolean };

async function verifyAssetBytes(bytes: ArrayBuffer, integrity: Integrity, signal: AbortSignal) {
  if (signal.aborted) throw aborted();
  if (bytes.byteLength !== integrity.bytes) throw new Error("The 3D download is incomplete.");
  if (!globalThis.crypto?.subtle) throw new Error("This renderer can't verify 3D downloads.");
  const digest = await globalThis.crypto.subtle.digest("SHA-256", bytes);
  if (signal.aborted) throw aborted();
  const hash = Array.from(new Uint8Array(digest), byte => byte.toString(16).padStart(2, "0")).join("");
  if (hash !== integrity.sha256) throw new Error("The 3D download couldn't be verified.");
}

// Verification evidence stays inside this read-to-parse path. External delivery
// callbacks still require their own renderer-side hash, even after host checks.
async function readAsset(uri: string, signal: AbortSignal, integrity?: Integrity,
  { background = false, model = false }: { background?: boolean; model?: boolean } = {}): Promise<AssetRead> {
  if (signal.aborted) throw aborted();
  if (uri.startsWith("file://")) {
    // Native delivery returns an immutable cache file. Read it directly in the
    // local WebView without copying the GLB through a base64 message bridge.
    const bytes = await new Promise<ArrayBuffer>((resolve, reject) => {
      const request = new XMLHttpRequest();
      let settled = false;
      const finish = (bytes?: ArrayBuffer, error?: Error) => {
        if (settled) return;
        settled = true;
        signal.removeEventListener("abort", cancel);
        request.onload = request.onerror = request.onabort = null;
        if (bytes) resolve(bytes);
        else reject(error ?? new Error("The cached 3D asset couldn't be read."));
      };
      const cancel = () => { finish(undefined, aborted()); request.abort(); };
      try {
        request.open("GET", uri, true);
        request.responseType = "arraybuffer";
        request.onload = () => {
          if ((request.status === 0 || request.status >= 200 && request.status < 300) && request.response instanceof ArrayBuffer) finish(request.response);
          else finish();
        };
        request.onerror = () => finish();
        request.onabort = () => finish(undefined, aborted());
        signal.addEventListener("abort", cancel, { once: true });
        if (signal.aborted) cancel();
        else request.send();
      } catch { finish(); }
    });
    return { bytes, verified: false };
  }
  // Persist only registered, immutable public packages, never account data.
  let cache: Cache | undefined;
  // Sandboxed web scenes can throw when the caches getter is read.
  try {
    if (integrity && globalThis.crypto?.subtle) cache = await globalThis.caches?.open("baddie-3d-resources-v1");
  } catch { /* Verified downloads remain available without persistent storage. */ }
  if (integrity && cache && globalThis.crypto?.subtle) {
    const saved = await cache?.match(uri).catch(() => undefined);
    if (saved) {
      try {
        const bytes = await (model ? readCompanionAssetResponse(saved, integrity.bytes, signal) : saved.arrayBuffer());
        await verifyAssetBytes(bytes, integrity, signal);
        return { bytes, verified: true };
      } catch {
        if (signal.aborted) throw aborted();
      }
      await cache?.delete(uri).catch(() => false);
    }
  }
  if (!background) reportResourceDownload(uri, { downloadedBytes: 0, totalBytes: integrity?.bytes ?? 0 });
  try {
    if (signal.aborted) throw aborted();
    const response = await fetch(uri, { signal, credentials: "omit" });
    if (!response.ok) throw new Error("The 3D asset couldn't download. Check your connection and retry.");
    const totalBytes = integrity?.bytes ?? (Number(response.headers.get("Content-Length")) || 0);
    let bytes: ArrayBuffer;
    if (model && integrity) {
      bytes = await readCompanionAssetResponse(response, integrity.bytes, signal, background ? undefined
        : downloadedBytes => reportResourceDownload(uri, { downloadedBytes, totalBytes }));
    } else if (response.body) {
      const reader = response.body.getReader();
      // Registered downloads have an exact size. Fill their final allocation
      // directly instead of retaining every chunk plus a second full-size copy.
      const destination = integrity ? new Uint8Array(integrity.bytes) : undefined;
      const chunks: Uint8Array[] = [];
      let downloadedBytes = 0;
      try {
        while (true) {
          const chunk = await reader.read();
          if (chunk.done) break;
          const offset = downloadedBytes;
          downloadedBytes += chunk.value.byteLength;
          if (integrity && downloadedBytes > integrity.bytes) throw new Error("Invalid 3D download size.");
          if (destination) destination.set(chunk.value, offset);
          else chunks.push(chunk.value);
          if (!background) reportResourceDownload(uri, { downloadedBytes, totalBytes });
        }
      } catch (error) { await reader.cancel().catch(() => {}); throw error; }
      finally { reader.releaseLock(); }
      if (integrity && downloadedBytes !== integrity.bytes) throw new Error("The 3D download is incomplete.");
      const result = destination ?? new Uint8Array(downloadedBytes);
      if (!destination) {
        let offset = 0;
        for (const chunk of chunks) { result.set(chunk, offset); offset += chunk.byteLength; }
      }
      bytes = result.buffer;
    } else bytes = await response.arrayBuffer();
    if (signal.aborted) throw aborted();
    if (integrity) await verifyAssetBytes(bytes, integrity, signal);
    if (integrity && cache) {
      await (model ? cacheCompanionModelBytes(cache, uri, bytes, signal, "model/gltf-binary")
        : cache.put(uri, new Response(bytes, { headers: { "Content-Type": "model/gltf-binary" } }))).catch(() => {});
      void navigator.storage?.persist?.().catch(() => false);
    }
    if (signal.aborted) throw aborted();
    return { bytes, verified: !!integrity };
  } finally { if (!background) reportResourceDownload(uri, null); }
}

export async function readAssetBytes(uri: string, signal: AbortSignal, integrity?: Integrity, background = false): Promise<ArrayBuffer> {
  return (await readAsset(uri, signal, integrity, { background })).bytes;
}

async function fetchGltf(uri: string, signal: AbortSignal, integrity?: { sha256: string; bytes: number }, read?: (signal: AbortSignal) => Promise<ArrayBuffer>, textures?: TexturePreparation) {
  const { bytes, verified } = read ? { bytes: await read(signal), verified: false } : await readAsset(uri, signal, integrity, { model: true });
  if (signal.aborted) throw aborted();
  if (integrity && !verified) await verifyAssetBytes(bytes, integrity, signal);
  // Delivery packages are self-contained. Never follow a GLB's unexpected
  // external image/buffer URLs outside the host's asset acquisition contract.
  const meshopt = createMeshoptPreparation(signal);
  const loader = new GLTFLoader();
  let images: ReturnType<typeof createImagePreparation> | undefined;
  let textureLease: ReturnType<TexturePreparation["acquire"]> | undefined;
  loader.register(parser => {
    // r180 invokes plugin factories before parser.parse and dependency loading.
    // Validate that parsed metadata without decoding/retaining a second JSON tree.
    if (signal.aborted) throw aborted();
    const json = parser.json;
    // r180 resolves the async meshopt adapter from parser options after plugins
    // are installed. Its legacy synchronous decoder methods are not required.
    parser.options.meshoptDecoder = meshopt.decoder;
    images = createImagePreparation(parser.loadImageSource, signal);
    parser.loadImageSource = images.loadImageSource;
    for (const items of [json.buffers, json.images]) for (const item of items ?? []) {
      if (item.uri !== undefined && (typeof item.uri !== "string" || !item.uri.startsWith("data:"))) throw new Error("The 3D asset is missing embedded files.");
    }
    if (json.extensionsUsed?.includes("KHR_texture_basisu") === true) {
      if (!textures) throw new Error("This asset needs a GPU-aware renderer.");
      textureLease = textures.acquire();
      // The Basis extension reads this option when it loads a texture, after
      // plugin setup. Keep decoder ownership limited to assets that need it.
      parser.options.ktx2Loader = textureLease.loader;
    }
    return {
      name: "BADDIE_embedded_images",
      beforeRoot() {
        if (signal.aborted) throw aborted();
        // Verified packages contain only embedded images. Load them through
        // img-src, not ImageBitmapLoader's fetch path outside connect-src.
        if (integrity) parser.textureLoader = new THREE.TextureLoader(parser.options.manager);
        return null;
      },
    };
  });
  let retained = false;
  try {
    const asset = await parseEmbeddedGlb(loader, bytes);
    try {
      images?.throwIfSourceFailed();
      if (signal.aborted) throw aborted();
      const result = retainSceneAsset(asset);
      retained = true;
      return result;
    } catch (error) { disposeGltf(asset); throw error; }
  } finally {
    try { await Promise.all([meshopt.dispose(), images?.dispose({ discardImages: !retained })]); }
    finally { textureLease?.release(); }
  }
}

/** One transfer/parse per version. Last-consumer release cancels unfinished work. */
export function acquireGltf(uri: string, integrity?: { sha256: string; bytes: number }, spatial = false,
  read?: (signal: AbortSignal) => Promise<ArrayBuffer>, textures?: TexturePreparation): GltfLease {
  const key = `${uri}|${integrity?.sha256 ?? ""}|${spatial}|${textures?.key ?? "uncompressed"}`;
  let entry = entries.get(key);
  if (!entry) {
    const controller = new AbortController();
    const created: Entry = { users: 0, controller,
      promise: prepareAsset(controller.signal, async () => {
        const asset = await fetchGltf(uri, controller.signal, integrity, read, textures);
        try {
          if (controller.signal.aborted || created.users === 0) throw aborted();
          if (spatial) {
            try { created.releaseSpatial = await partitionStaticGeometry(asset.scene, 32, controller.signal); }
            catch (error) {
              if (controller.signal.aborted) throw error;
              // A partition failure retains the verified, unchanged scenery.
            }
          }
          if (controller.signal.aborted || created.users === 0) throw aborted();
          created.asset = asset;
          return asset;
        } catch (error) {
          created.releaseSpatial?.(); created.releaseSpatial = undefined;
          disposeGltf(asset); throw error;
        }
      }) };
    created.promise = created.promise.catch(error => {
      if (entries.get(key) === created) entries.delete(key);
      throw error;
    });
    entry = created;
    entries.set(key, entry);
  }
  const owned = entry;
  owned.users += 1;
  let released = false;
  return {
    promise: owned.promise.then(asset => { if (released) throw aborted(); return asset; }),
    release() {
      if (released) return;
      released = true;
      owned.users -= 1;
      if (owned.users !== 0) return;
      if (entries.get(key) === owned) entries.delete(key);
      owned.controller.abort();
      owned.releaseSpatial?.(); owned.releaseSpatial = undefined;
      if (owned.asset) disposeGltf(owned.asset);
      owned.asset = undefined;
    },
  };
}

export function acquireCompanionGltf(key: CompanionAssetKey, delivery: CompanionAssetDelivery): GltfLease {
  const controller = new AbortController();
  let lease: GltfLease | undefined;
  const descriptor = companionAssetEntries[key];
  return {
    promise: delivery.acquire(key, controller.signal).then(uri => {
      if (controller.signal.aborted) throw aborted();
      lease = acquireGltf(uri, descriptor, key === "bestieTownSquare", delivery.read ? signal => delivery.read!(key, signal) : undefined,
        rendererTexturePreparation(delivery));
      return lease.promise;
    }),
    release() { controller.abort(); lease?.release(); lease = undefined; },
  };
}
