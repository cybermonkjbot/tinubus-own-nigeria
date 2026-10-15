import type { BufferGeometry } from "three";

export type GeometryLease = { geometry: BufferGeometry; release(): void };
type Entry = { key: string; geometry: BufferGeometry; users: number };
const variants = new WeakMap<object, Map<string, Entry>>();

/** Shapes are immutable. Use the source geometry or a stable procedural-family
 * owner, with every shape input in the key. Pose and materials stay per rig. */
export function acquireGeometryVariant(source: object, key: string, create: () => BufferGeometry): GeometryLease {
  let entries = variants.get(source);
  if (!entries) { entries = new Map(); variants.set(source, entries); }
  let entry = entries.get(key);
  if (!entry) {
    entry = { key, geometry: create(), users: 0 };
    entries.set(key, entry);
  }
  entry.users += 1;
  const owned = entry;
  const cache = entries;
  let released = false;
  return {
    geometry: owned.geometry,
    release() {
      if (released) return;
      released = true;
      if (--owned.users !== 0) return;
      // Share the retained recipe string as well as the geometry.
      cache.delete(owned.key);
      if (cache.size === 0) variants.delete(source);
      owned.geometry.dispose();
    },
  };
}
