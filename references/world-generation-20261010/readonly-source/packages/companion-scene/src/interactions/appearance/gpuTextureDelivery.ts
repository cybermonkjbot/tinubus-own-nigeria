import { LoadingManager, type WebGLRenderer } from "three";
import { KTX2Loader } from "three/addons/loaders/KTX2Loader.js";
import { gunzipSync } from "three/addons/libs/fflate.module.js";
import transcoderSource from "three/addons/libs/basis/basis_transcoder.js?raw";
import transcoderGzip, { byteLength as transcoderByteLength } from "baddie:basis-transcoder-gzip";
import type { CompanionAssetDelivery } from "../../../../../packages/utils/src/companionAssets";

type Decoder = { loader: KTX2Loader; owners: number; jobs: number; releaseBinary(): void };
export type TexturePreparation = { key: string; acquire(): { loader: KTX2Loader; release(): void } };
const decoders = new Map<string, Decoder>();
const contexts = new WeakMap<CompanionAssetDelivery, TexturePreparation>();

function releaseDecoder(key: string, decoder: Decoder) {
  if (decoder.owners || decoder.jobs) return;
  if (decoders.get(key) === decoder) decoders.delete(key);
  // r180's active-loader counter increments only in init().
  try { if (decoder.loader.transcoderPending) decoder.loader.dispose(); }
  finally { decoder.releaseBinary(); }
}

/** One worker per GPU format profile, allocated only when a KTX2 asset is read. */
export function createRendererAssetDelivery(renderer: WebGLRenderer) {
  const manager = new LoadingManager();
  let binaryUrl: string | undefined;
  const releaseBinary = () => {
    if (binaryUrl) URL.revokeObjectURL(binaryUrl);
    binaryUrl = undefined;
  };
  // FileLoader has consumed both decoder files before manager completion.
  // Keep no second binary copy in blob storage while the worker is in use.
  manager.onLoad = releaseBinary;
  manager.setURLModifier(url => {
    if (url === "baddie-basis/basis_transcoder.js") return `data:text/javascript;charset=utf-8,${encodeURIComponent(transcoderSource)}`;
    if (url === "baddie-basis/basis_transcoder.wasm") {
      if (!binaryUrl) {
        // KTX2Loader requests this only on first transcode. Ordinary room and
        // character assets allocate no decoder bytes or temporary blob URL.
        const encoded = atob(transcoderGzip);
        const compressed = new Uint8Array(encoded.length);
        for (let i = 0; i < encoded.length; i++) compressed[i] = encoded.charCodeAt(i);
        const binary = new Uint8Array(transcoderByteLength);
        gunzipSync(compressed, { out: binary });
        binaryUrl = URL.createObjectURL(new Blob([binary], { type: "application/wasm" }));
      }
      return binaryUrl;
    }
    return url;
  });
  const candidate = new KTX2Loader(manager).setTranscoderPath("baddie-basis/").setWorkerLimit(1).detectSupport(renderer);
  const key = JSON.stringify(candidate.workerConfig);
  const decoder = decoders.get(key) ?? { loader: candidate, owners: 0, jobs: 0, releaseBinary };
  decoders.set(key, decoder);
  decoder.owners += 1;
  let closed = false;
  const deliveries = new WeakMap<CompanionAssetDelivery, CompanionAssetDelivery>();
  const preparation: TexturePreparation = {
    key,
    acquire() {
      if (decoder.owners === 0) throw new Error("The 3D renderer is closed.");
      decoder.jobs += 1;
      let released = false;
      return { loader: decoder.loader, release() {
        if (released) return;
        released = true; decoder.jobs -= 1; releaseDecoder(key, decoder);
      } };
    },
  };
  return {
    resolve(source?: CompanionAssetDelivery): CompanionAssetDelivery | undefined {
      if (!source || closed) return undefined;
      let delivery = deliveries.get(source);
      if (!delivery) {
        delivery = {
          get allowBundledCharacterFallback() { return source.allowBundledCharacterFallback; },
          available: asset => source.available(asset),
          acquire: (asset, signal, progress) => source.acquire(asset, signal, progress),
          ...(source.read ? { read: (asset: Parameters<CompanionAssetDelivery["acquire"]>[0], signal: AbortSignal) => source.read!(asset, signal) } : {}),
        };
        contexts.set(delivery, preparation); deliveries.set(source, delivery);
      }
      return delivery;
    },
    dispose() {
      if (closed) return;
      closed = true; decoder.owners -= 1; releaseDecoder(key, decoder);
    },
  };
}

export function rendererTexturePreparation(delivery: CompanionAssetDelivery) { return contexts.get(delivery); }
