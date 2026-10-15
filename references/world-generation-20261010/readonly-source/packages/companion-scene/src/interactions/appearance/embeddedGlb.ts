import type { GLTFLoader, GLTFLoaderPlugin, GLTFParser } from "three/addons/loaders/GLTFLoader.js";

type BinaryChunk = { offset: number; length: number };
const invalid = () => new Error("Invalid 3D asset.");
// In the installed r180 loader, these extensions reach binary data through
// ordinary buffer views/accessors, except meshopt's explicit compressed range.
// Unfamiliar extensions keep an ordinary, body-local buffer instead.
const offsetSafeExtensions = new Set([
  "EXT_meshopt_compression", "EXT_mesh_gpu_instancing", "EXT_texture_webp", "EXT_texture_avif",
  "KHR_draco_mesh_compression", "KHR_mesh_quantization", "KHR_texture_basisu", "KHR_texture_transform",
  "KHR_lights_punctual", "KHR_materials_unlit", "KHR_materials_clearcoat", "KHR_materials_ior",
  "KHR_materials_transmission", "KHR_materials_volume", "KHR_materials_sheen", "KHR_materials_specular",
  "KHR_materials_emissive_strength", "KHR_materials_iridescence", "KHR_materials_anisotropy", "KHR_materials_dispersion",
]);

function readChunks(bytes: ArrayBuffer) {
  const view = new DataView(bytes);
  if (bytes.byteLength < 20 || view.getUint32(0, true) !== 0x46546c67 || view.getUint32(4, true) !== 2
    || view.getUint32(8, true) !== bytes.byteLength || view.getUint32(16, true) !== 0x4e4f534a) throw invalid();
  let json: string | undefined, binary: BinaryChunk | undefined;
  for (let cursor = 12; cursor < bytes.byteLength;) {
    if (bytes.byteLength - cursor < 8) throw invalid();
    const length = view.getUint32(cursor, true), type = view.getUint32(cursor + 4, true), offset = cursor + 8;
    if (length % 4 !== 0 || length > bytes.byteLength - offset) throw invalid();
    if (type === 0x4e4f534a) {
      if (cursor !== 12 || json !== undefined) throw invalid();
      json = new TextDecoder().decode(new Uint8Array(bytes, offset, length));
    } else if (type === 0x004e4942) {
      if (binary) throw invalid();
      binary = { offset, length };
    }
    // Unknown chunk types are ignored, as in GLTFLoader's GLB reader.
    cursor = offset + length;
  }
  if (json === undefined) throw invalid();
  return { json, binary };
}

function canUseContainer(parser: GLTFParser, binary: BinaryChunk) {
  const json = parser.json, buffer = json.buffers?.[0];
  if (!buffer || buffer.uri !== undefined || !Number.isSafeInteger(buffer.byteLength)
    || buffer.byteLength < 0 || buffer.byteLength > binary.length || binary.length - buffer.byteLength > 3) return false;
  for (const name of [...(json.extensionsUsed ?? []), ...(json.extensionsRequired ?? []), ...Object.keys(json.extensions ?? {})]) {
    if (!offsetSafeExtensions.has(name)) return false;
  }
  const inBuffer = (range: { byteOffset?: number; byteLength?: number }) => {
    const offset = range.byteOffset ?? 0, length = range.byteLength ?? 0;
    return Number.isSafeInteger(offset) && Number.isSafeInteger(length) && offset >= 0 && length >= 0
      && offset <= buffer.byteLength && length <= buffer.byteLength - offset;
  };
  for (const view of json.bufferViews ?? []) {
    if (!Number.isSafeInteger(view.buffer) || view.buffer < 0 || view.buffer >= json.buffers.length) return false;
    if (Object.keys(view.extensions ?? {}).some(name => name !== "EXT_meshopt_compression")) return false;
    if (view.buffer === 0 && !inBuffer(view)) return false;
    const compressed = view.extensions?.EXT_meshopt_compression;
    if (compressed && (!Number.isSafeInteger(compressed.buffer) || compressed.buffer < 0 || compressed.buffer >= json.buffers.length)) return false;
    if (compressed?.buffer === 0 && !inBuffer(compressed)) return false;
  }
  return true;
}

/** Parse the same GLB through Three's JSON entry, avoiding a second whole BIN
 * allocation where the installed loader's buffer-offset contracts are known. */
export async function parseEmbeddedGlb(loader: GLTFLoader, bytes: ArrayBuffer) {
  const { json, binary } = readChunks(bytes);
  const plugin = (parser: GLTFParser): GLTFLoaderPlugin => ({
    name: "BADDIE_glb_buffer",
    beforeRoot() {
      let body: ArrayBuffer | null = null;
      if (binary) {
        if (canUseContainer(parser, binary)) {
          // Validate every range before changing any offsets. Only parser-owned
          // metadata changes; verified package bytes remain immutable.
          for (const view of parser.json.bufferViews ?? []) {
            if (view.buffer === 0) view.byteOffset = (view.byteOffset ?? 0) + binary.offset;
            const compressed = view.extensions?.EXT_meshopt_compression;
            if (compressed?.buffer === 0) compressed.byteOffset = (compressed.byteOffset ?? 0) + binary.offset;
          }
          parser.json.buffers[0].byteLength = bytes.byteLength;
          body = bytes;
        } else body = bytes.slice(binary.offset, binary.offset + binary.length);
      }
      // r180 installs extensions and clears its dependency cache before this
      // hook. Scene/animation/texture dependencies are requested afterwards.
      parser.extensions.KHR_binary_glTF = {
        name: "KHR_binary_glTF", content: json, body,
        header: { magic: "glTF", version: 2, length: bytes.byteLength },
      };
      return null;
    },
  });
  loader.register(plugin);
  try { return await loader.parseAsync(json, ""); }
  finally { loader.unregister(plugin); }
}
