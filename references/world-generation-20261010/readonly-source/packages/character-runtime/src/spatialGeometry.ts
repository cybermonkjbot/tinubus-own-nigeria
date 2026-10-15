import * as THREE from "three";

type ChunkOutput = { geometry: THREE.BufferGeometry; indices: Uint16Array | Uint32Array; bounds: THREE.Box3 };
type MaterialBucket = { id: number; count: number; cursor: number; output?: ChunkOutput };

/** Await before exposing the decoded source to renderers. Cancellation restores
 * its original graph. Vertices and material groups remain unchanged. */
export async function partitionStaticGeometry(root: THREE.Object3D, cellSize = 32, signal?: AbortSignal) {
  const checkCancelled = () => { if (signal?.aborted) throw signal.reason ?? new DOMException("Scenery preparation cancelled", "AbortError"); };
  checkCancelled();
  let deadline = performance.now() + 4;
  const yieldTurn = async () => {
    await new Promise<void>((resolve, reject) => {
      const finish = () => {
        signal?.removeEventListener("abort", cancel);
        if (signal?.aborted) reject(signal.reason ?? new DOMException("Scenery preparation cancelled", "AbortError"));
        else resolve();
      };
      const cancel = () => { clearTimeout(timer); finish(); };
      const timer = setTimeout(finish, 0);
      signal?.addEventListener("abort", cancel, { once: true });
      if (signal?.aborted) cancel();
    });
    checkCancelled(); deadline = performance.now() + 4;
  };
  const shouldYield = () => { checkCancelled(); return performance.now() >= deadline; };
  const meshes: THREE.Mesh[] = [];
  root.updateMatrixWorld(true);
  root.traverse(node => {
    if (node instanceof THREE.Mesh && !(node instanceof THREE.SkinnedMesh) && !(node instanceof THREE.InstancedMesh) && !(node instanceof THREE.BatchedMesh)
      && node.children.length === 0 && !Object.keys(node.geometry.morphAttributes).length
      && !Object.values(node.geometry.attributes).some(attribute => attribute instanceof THREE.InstancedBufferAttribute
        || attribute instanceof THREE.InterleavedBufferAttribute && attribute.data instanceof THREE.InstancedInterleavedBuffer)
      && !(Array.isArray(node.material) ? node.material : [node.material]).some(material => material.transparent)) meshes.push(node);
  });
  const owned: THREE.BufferGeometry[] = [];
  const replaced: { source: THREE.Mesh; group: THREE.Group; visible: boolean }[] = [];
  let released = false;
  const release = () => {
    if (released) return;
    released = true;
    replaced.forEach(({ source, group, visible }) => {
      group.removeFromParent(); source.visible = visible; group.clear();
    });
    replaced.length = 0;
    owned.forEach(geometry => geometry.dispose());
    // Disposal releases GPU resources, not the JS graphs or index buffers.
    // A retained release callback must not keep those detached cells alive.
    owned.length = 0;
  };
  const point = new THREE.Vector3();
  if (!Number.isFinite(cellSize) || cellSize <= 0) throw new Error("Invalid scenery cell size.");
  try {
    for (const source of meshes) {
      if (shouldYield()) await yieldTurn();
      const geometry = source.geometry, position = geometry.getAttribute("position"), index = geometry.index;
      // Tiny material primitives are cheaper as one draw than many spatial draws.
      if (!position || (index?.count ?? position.count) < 9000 || !source.parent) continue;
      const bounds = new THREE.Box3().setFromObject(source), size = bounds.getSize(new THREE.Vector3());
      if (Math.max(size.x, size.z) < cellSize * 2) continue;
      const columns = new Map<number, Map<number, Map<number, MaterialBucket>>>();
      // Numeric lookups avoid constructing a cell-key string for every triangle.
      // Keep first-seen cell order separately; column order would reorder draws.
      const chunks: Map<number, MaterialBucket>[] = [];
      const buckets: MaterialBucket[] = [];
      const sourceRanges = geometry.groups.length ? geometry.groups : [{ start: 0, count: index?.count ?? position.count, materialIndex: 0 }];
      const ranges = sourceRanges.map(range => ({
        start: Math.max(range.start, geometry.drawRange.start),
        end: Math.min(range.start + range.count, geometry.drawRange.start + geometry.drawRange.count, index?.count ?? position.count),
        material: range.materialIndex ?? 0,
      }));
      if (ranges.some(range => !Number.isSafeInteger(range.start) || !Number.isSafeInteger(range.end))) {
        throw new Error("Invalid scenery draw range.");
      }
      const triangleCount = ranges.reduce((total, range) => total + Math.max(0, Math.floor((range.end - range.start) / 3)), 0);
      // One compact bucket ID per triangle replaces copied index triples in
      // growing JS arrays. The second pass reads the original index buffer.
      const destinations = new Uint32Array(triangleCount);
      let triangle = 0;
      for (const range of ranges) {
        let previousX = NaN, previousZ = NaN, bucket: MaterialBucket | undefined;
        for (let offset = range.start; offset + 2 < range.end; offset += 3) {
          if ((triangle & 1023) === 0 && shouldYield()) await yieldTurn();
          const a = index ? index.getX(offset) : offset, b = index ? index.getX(offset + 1) : offset + 1, c = index ? index.getX(offset + 2) : offset + 2;
          point.set((position.getX(a) + position.getX(b) + position.getX(c)) / 3,
            (position.getY(a) + position.getY(b) + position.getY(c)) / 3,
            (position.getZ(a) + position.getZ(b) + position.getZ(c)) / 3).applyMatrix4(source.matrixWorld);
          const x = Math.floor(point.x / cellSize), z = Math.floor(point.z / cellSize);
          // A range has one material. Consecutive triangles in the same cell
          // can use its bucket directly, including across a preparation yield.
          if (!bucket || x !== previousX || z !== previousZ) {
            let column = columns.get(x);
            if (!column) { column = new Map(); columns.set(x, column); }
            let materials = column.get(z);
            if (!materials) { materials = new Map(); column.set(z, materials); chunks.push(materials); }
            bucket = materials.get(range.material);
            if (!bucket) {
              bucket = { id: buckets.length, count: 0, cursor: 0 };
              buckets.push(bucket); materials.set(range.material, bucket);
            }
            previousX = x; previousZ = z;
          }
          destinations[triangle++] = bucket.id; bucket.count += 3;
        }
      }
      if (chunks.length < 2) continue;
      const outputs: ChunkOutput[] = [];
      for (const materials of chunks) {
        if (shouldYield()) await yieldTurn();
        const chunk = new THREE.BufferGeometry(); owned.push(chunk);
        let count = 0; for (const bucket of materials.values()) count += bucket.count;
        const indices = position.count > 65535 ? new Uint32Array(count) : new Uint16Array(count);
        const output: ChunkOutput = { geometry: chunk, indices, bounds: new THREE.Box3() }; outputs.push(output);
        let cursor = 0;
        for (const [material, bucket] of materials) {
          chunk.addGroup(cursor, bucket.count, material);
          bucket.cursor = cursor; bucket.output = output; cursor += bucket.count;
        }
        // Cells own only their triangle indices and bounds. Sharing immutable
        // attributes avoids copying/uploading the whole district's vertices a
        // second time, and preserves packed, interleaved and half-float data.
        for (const [name, attribute] of Object.entries(geometry.attributes)) {
          chunk.setAttribute(name, attribute);
        }
        chunk.setIndex(new THREE.BufferAttribute(indices, 1));
      }
      triangle = 0;
      for (const range of ranges) {
        for (let offset = range.start; offset + 2 < range.end; offset += 3) {
          if ((triangle & 1023) === 0 && shouldYield()) await yieldTurn();
          const bucket = buckets[destinations[triangle++]];
          // Every counted bucket receives its final output before this pass.
          const { indices, bounds } = bucket.output!;
          const a = index ? index.getX(offset) : offset, b = index ? index.getX(offset + 1) : offset + 1, c = index ? index.getX(offset + 2) : offset + 2;
          indices[bucket.cursor++] = a; indices[bucket.cursor++] = b; indices[bucket.cursor++] = c;
          bounds.expandByPoint(point.fromBufferAttribute(position, a));
          bounds.expandByPoint(point.fromBufferAttribute(position, b));
          bounds.expandByPoint(point.fromBufferAttribute(position, c));
        }
      }
      const group = new THREE.Group(); group.name = `${source.name}_spatial`; group.matrix.copy(source.matrix); group.matrixAutoUpdate = false;
      group.visible = source.visible; group.userData = { ...source.userData };
      group.layers.mask = source.layers.mask;
      for (const { geometry: chunk, bounds } of outputs) {
        if (shouldYield()) await yieldTurn();
        // computeBounding* would include the shared source's unused vertices.
        chunk.boundingBox = bounds;
        chunk.boundingSphere = bounds.getBoundingSphere(new THREE.Sphere());
        const mesh = new THREE.Mesh(chunk, source.material);
        mesh.castShadow = source.castShadow; mesh.receiveShadow = source.receiveShadow;
        mesh.layers.mask = source.layers.mask; mesh.renderOrder = source.renderOrder; mesh.userData = { ...source.userData };
        group.add(mesh);
      }
      source.parent.add(group); replaced.push({ source, group, visible: source.visible }); source.visible = false;
    }
  } catch (error) {
    release();
    throw error;
  } finally {
    meshes.length = 0;
  }
  return release;
}
