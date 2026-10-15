import type * as THREE from "three";

// Position, quaternion, scale, local matrix and world matrix. Keep JS precision.
const stride = 42;

class PackedObjectTransform {
  private readonly auto: boolean;
  private readonly worldAuto: boolean;

  constructor(readonly node: THREE.Object3D, private readonly values: Float64Array, private readonly offset: number) {
    node.position.toArray(values, offset);
    node.quaternion.toArray(values, offset + 3);
    node.scale.toArray(values, offset + 7);
    node.matrix.toArray(values, offset + 10);
    node.matrixWorld.toArray(values, offset + 26);
    this.auto = node.matrixAutoUpdate;
    this.worldAuto = node.matrixWorldAutoUpdate;
  }

  copyPositionTo(target: THREE.Vector3) { return target.fromArray(this.values, this.offset); }
  copyQuaternionTo(target: THREE.Quaternion) { return target.fromArray(this.values, this.offset + 3); }

  restore() {
    const { node, values, offset } = this;
    this.copyPositionTo(node.position);
    this.copyQuaternionTo(node.quaternion);
    node.scale.fromArray(values, offset + 7);
    node.matrix.fromArray(values, offset + 10);
    node.matrixWorld.fromArray(values, offset + 26);
    node.matrixAutoUpdate = this.auto;
    node.matrixWorldAutoUpdate = this.worldAuto;
    node.matrixWorldNeedsUpdate = true;
  }
}

export type ObjectTransformSnapshot = PackedObjectTransform;

/** Capture the existing matrices as-is; the caller owns their update order. */
export function captureObjectTransform(node: THREE.Object3D): ObjectTransformSnapshot {
  return new PackedObjectTransform(node, new Float64Array(stride), 0);
}

/** Records share one numeric buffer without retaining the temporary node list. */
export function captureObjectTransforms(root: THREE.Object3D): ObjectTransformSnapshot[] {
  const nodes: THREE.Object3D[] = [];
  root.traverse(node => nodes.push(node));
  const values = new Float64Array(nodes.length * stride);
  return nodes.map((node, index) => new PackedObjectTransform(node, values, index * stride));
}

export type LocalTransformSnapshot = { restore(): void; dispose(): void };

/** Save local pose only. Matrix updates remain with the caller. */
export function captureLocalTransforms(root: THREE.Object3D, includeVisibility = false): LocalTransformSnapshot {
  const nodes: THREE.Object3D[] = [];
  root.traverse(node => nodes.push(node));
  const localStride = 10;
  let values: Float64Array | undefined = new Float64Array(nodes.length * localStride);
  let visibility = includeVisibility ? new Uint8Array(nodes.length) : undefined;
  for (let index = 0; index < nodes.length; index += 1) {
    const node = nodes[index], offset = index * localStride;
    node.position.toArray(values, offset);
    node.quaternion.toArray(values, offset + 3);
    node.scale.toArray(values, offset + 7);
    if (visibility) visibility[index] = node.visible ? 1 : 0;
  }
  return {
    restore() {
      if (!values) return;
      for (let index = 0; index < nodes.length; index += 1) {
        const node = nodes[index], offset = index * localStride;
        node.position.fromArray(values, offset);
        node.quaternion.fromArray(values, offset + 3);
        node.scale.fromArray(values, offset + 7);
        if (visibility) node.visible = visibility[index] !== 0;
      }
    },
    dispose() { nodes.length = 0; values = undefined; visibility = undefined; },
  };
}
