import type { GLTFParser } from "three/addons/loaders/GLTFLoader.js";
import { Texture } from "three";

type LoadImageSource = GLTFParser["loadImageSource"];
const closedError = () => new Error("The 3D image preparation is closed.");

/** r180 passes this dependency straight to Blob. Scope the byte view to that
 * image call; geometry still receives the parser's ordinary ArrayBuffer copy. */
function imageSourceParser(parser: GLTFParser, bufferViewIndex: number): GLTFParser {
  const view = parser.json.bufferViews?.[bufferViewIndex];
  // Compressed and unfamiliar buffer-view extensions need their loader path.
  if (!view || Object.keys(view.extensions ?? {}).length) return parser;
  const bufferIndex = view.buffer, offset = view.byteOffset || 0, length = view.byteLength || 0;
  if (!Number.isSafeInteger(bufferIndex) || bufferIndex < 0 || !Number.isSafeInteger(offset) || offset < 0
    || !Number.isSafeInteger(length) || length < 0) return parser;
  const sourceParser: GLTFParser = Object.create(parser);
  sourceParser.getDependency = (type, index) => {
    if (type !== "bufferView" || index !== bufferViewIndex) return parser.getDependency(type, index);
    return parser.getDependency("buffer", bufferIndex).then(buffer => {
      // Retain Three's slicing/error behavior for malformed or unfamiliar data.
      if (!(buffer instanceof ArrayBuffer) || offset > buffer.byteLength || length > buffer.byteLength - offset) {
        return parser.getDependency(type, index);
      }
      // Blob snapshots this bounded range. Never cache the view or expose it to
      // accessors, where it could keep the full encoded model alive in a scene.
      return new Uint8Array(buffer, offset, length);
    });
  };
  return sourceParser;
}

/** Queue new sources before r180 allocates image buffers, blobs and decoder
 * inputs. Duplicate sources retain r180's immediate promise/clone ordering. */
export function createImagePreparation(loadImageSource: LoadImageSource, signal: AbortSignal) {
  let closed = false;
  let tail = Promise.resolve();
  let sourceError: { reason: unknown } | undefined;
  let canReadImageViews: boolean | undefined;
  const sources = new Map<number, ReturnType<LoadImageSource>>();
  const bitmaps = new Set<ImageBitmap>();
  return {
    loadImageSource(this: GLTFParser, ...args: Parameters<LoadImageSource>): ReturnType<LoadImageSource> {
      if (closed || signal.aborted) return Promise.reject(closedError());
      const cached = sources.get(args[0]);
      // Attach duplicate clones now, before downstream material callbacks can
      // change the first texture's color space. Queueing this clone is too late.
      if (cached) return cached.then(texture => {
        if (closed || signal.aborted) throw closedError();
        return texture.clone();
      });
      const result = tail.then(() => {
        // Cancellation stops waiting images before any source buffers, blobs
        // or decoder work start. The admitted image still drains normally.
        if (closed || signal.aborted) throw closedError();
        try {
          const [sourceIndex, loader] = args;
          // Only buffer-view images have URLs and decoded images owned by this
          // parse. A data-URI/custom loader may return a shared cached image.
          const bufferViewIndex = this.json.images[sourceIndex]?.bufferView;
          if (bufferViewIndex === undefined) return loadImageSource.apply(this, args);
          // Plugins are installed after this preparation is created, before
          // any queued source starts. Only r180's meshopt plugin reads views;
          // leave other/custom view loaders in charge if they are introduced.
          canReadImageViews ??= Object.values(this.plugins).every(plugin => !plugin.loadBufferView || plugin.name === "EXT_meshopt_compression");
          const sourceParser = canReadImageViews ? imageSourceParser(this, bufferViewIndex) : this;
          let objectURL: string | undefined;
          let sourceBitmap: ImageBitmap | undefined;
          // Preserve loader flags, but call the original receiver so shared
          // KTX2 decoder state and manager behavior stay with their owner.
          const sourceLoader: typeof loader = Object.create(loader);
          sourceLoader.load = (url, onLoad, onProgress, onError) => {
            if (url.startsWith("blob:")) objectURL = url;
            return loader.load(url, value => {
              const bitmap = value instanceof Texture ? value.image : value;
              if (typeof ImageBitmap !== "undefined" && bitmap instanceof ImageBitmap) {
                sourceBitmap = bitmap;
                bitmaps.add(bitmap);
              }
              onLoad(value);
            }, onProgress, onError);
          };
          const releaseURL = () => {
            // r180 revokes on success only. Also release failed decodes and
            // synchronous loader failures after their source promise settles.
            if (objectURL) URL.revokeObjectURL(objectURL);
            objectURL = undefined;
          };
          return loadImageSource.call(sourceParser, sourceIndex, sourceLoader).then(texture => {
            releaseURL();
            return texture;
          }, error => {
            releaseURL();
            // An image may decode before its metadata fails. The parser can
            // tolerate that missing texture, but no scene owns its bitmap.
            if (sourceBitmap) { bitmaps.delete(sourceBitmap); sourceBitmap.close(); }
            throw error;
          });
        }
        catch (reason) {
          // r180's texture handler tolerates asynchronous image failures, but
          // synchronous source errors used to reject the whole parse. Preserve
          // that rejection when moving source setup into this promise chain.
          sourceError ??= { reason };
          closed = true;
          throw reason;
        }
      });
      sources.set(args[0], result);
      // The scheduling tail must not retain a completed texture or bitmap.
      tail = result.then(() => {}, () => {});
      return result;
    },
    throwIfSourceFailed() {
      if (sourceError) throw sourceError.reason;
    },
    async dispose({ discardImages = false }: { discardImages?: boolean } = {}) {
      closed = true;
      // Skip queued jobs after parse failure, but drain the active image before
      // releasing the model slot or its shared texture-transcoder lease.
      await tail;
      // No scene may retain these images on a failed parse. Successful parses
      // transfer ownership to the existing last-lease scene disposal instead.
      // close() also tolerates images already closed by rejected-asset cleanup.
      if (discardImages) for (const bitmap of bitmaps) bitmap.close();
      bitmaps.clear();
      sources.clear();
    },
  };
}
