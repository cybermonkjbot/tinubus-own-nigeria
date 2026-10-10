import * as THREE from "three";
import type { CharacterBaseId } from "./types";
import { acquireGeometryVariant, type GeometryLease } from "./geometryLease";
import { withAppearanceRigLifetime } from "./appearanceRigLifetime";

/** Reversible silhouette changes. Never mutate the pooled GLB geometry. */
export function createBaseRig(model: THREE.Object3D) {
  const ears = [-1, 1].map(side => ({ side, node: model.getObjectByName(THREE.PropertyBinding.sanitizeNodeName(`Ear ${side}`)) })).filter(value => !!value.node)
    .map(({ side, node }) => ({ side, node: node!, position: node!.position.clone(), scale: node!.scale.clone(), rotation: node!.quaternion.clone() }));
  const head = model.getObjectByName("Head");
  const headScale = head?.scale.clone();
  const originalGeometry = new Map<THREE.Mesh, THREE.BufferGeometry>();
  const headParts: { node: THREE.Mesh; position: THREE.Vector3; scale: THREE.Vector3; lease?: GeometryLease; variant?: string }[] = [];
  // The tuft is part of the head/fur meshes. Compress only its upper vertices,
  // preserving the face, normals below the crown, eye rigs and mouth animation.
  head?.children.forEach(node => {
    if (!(node instanceof THREE.Mesh) || !/^(Peach_cream_fur|Cream_guard_hairs|Groomed_crown_and_cheek_fibers)(?:_\d+)?$/.test(node.name)) return;
    originalGeometry.set(node, node.geometry);
    headParts.push({ node, position: node.position.clone(), scale: node.scale.clone() });
  });
  const selectHead = (part: typeof headParts[number], variant: "original" | "rounded" | "short") => {
    if (part.variant === variant) return;
    const { node, position, scale } = part;
    const original = originalGeometry.get(node)!;
    const key = [variant, !/(hairs|fibers)/.test(node.name), ...position.toArray(), ...scale.toArray()].join(":");
    const next = variant === "original" ? undefined : acquireGeometryVariant(original, key, () => {
      const geometry = original.clone(), positions = geometry.getAttribute("position");
      for (let i = 0; i < positions.count; i++) {
        const y = positions.getY(i) * scale.y + position.y;
        if (variant === "rounded" && y > .82) {
          const x = positions.getX(i) * scale.x + position.x;
          const z = positions.getZ(i) * scale.z + position.z;
          const crown = .5 + Math.sqrt(Math.max(0, .49 ** 2 - x ** 2 - (z * .9) ** 2));
          positions.setY(i, (Math.min(y, crown) - position.y) / scale.y);
        } else if (variant === "short" && y > .70) {
          positions.setY(i, (.70 + (y - .70) * .58 - position.y) / scale.y);
        }
      }
      positions.needsUpdate = true;
      if (!/(hairs|fibers)/.test(node.name)) geometry.computeVertexNormals();
      geometry.computeBoundingBox(); geometry.computeBoundingSphere();
      return geometry;
    });
    node.geometry = next?.geometry ?? original;
    part.lease?.release(); part.lease = next; part.variant = variant;
  };
  const earGeometry: { node: THREE.Mesh; original: THREE.BufferGeometry; pointed?: GeometryLease }[] = [];
  ears.forEach(({ node }) => node.traverse(child => {
    if (child instanceof THREE.Mesh) earGeometry.push({ node: child, original: child.geometry });
  }));
  const pointedEarFor = (ear: typeof earGeometry[number]) => {
    ear.pointed ??= acquireGeometryVariant(ear.original, `pointed:${!/fibers/.test(ear.node.name)}`, () => {
      const pointed = ear.original.clone(), positions = pointed.getAttribute("position");
      let minY = Infinity, maxY = -Infinity;
      for (let i = 0; i < positions.count; i++) { minY = Math.min(minY, positions.getY(i)); maxY = Math.max(maxY, positions.getY(i)); }
      for (let i = 0; i < positions.count; i++) {
        const height = (positions.getY(i) - minY) / Math.max(.0001, maxY - minY);
        positions.setX(i, positions.getX(i) * (1 - .78 * Math.max(0, (height - .35) / .65)));
      }
      positions.needsUpdate = true;
      if (!/fibers/.test(ear.node.name)) pointed.computeVertexNormals();
      pointed.computeBoundingBox(); pointed.computeBoundingSphere();
      return pointed;
    });
    return ear.pointed.geometry;
  };
  let previous: CharacterBaseId | undefined;
  return withAppearanceRigLifetime<CharacterBaseId>({
    apply(base: CharacterBaseId = "bestie") {
      if (base === previous) return;
      previous = base;
      headParts.forEach(part => selectHead(part, base === "bestie" ? "original" : base === "bear" || base === "bunny" ? "rounded" : "short"));
      const round = base === "bear" || base === "panda";
      if (head && headScale) head.scale.copy(headScale).multiply(new THREE.Vector3(round ? 1.12 : base === "fox" ? .94 : 1, base === "bunny" ? .96 : 1, round ? 1.08 : 1));
      earGeometry.forEach(ear => {
        if (base === "cat" || base === "fox") ear.node.geometry = pointedEarFor(ear);
        else { ear.node.geometry = ear.original; ear.pointed?.release(); ear.pointed = undefined; }
      });
      ears.forEach(({ side, node, position, scale, rotation }) => {
        node.position.copy(position); node.scale.copy(scale); node.quaternion.copy(rotation);
        if (base === "bunny") { node.scale.multiply(new THREE.Vector3(.8, 2.55, .88)); node.position.set(side * .32, 1.02, position.z); node.rotation.z = -side * .1; }
        if (round) { node.scale.multiplyScalar(1.25); node.position.set(side * .46, .83, position.z); }
        if (base === "cat" || base === "fox") {
          node.scale.multiply(new THREE.Vector3(base === "fox" ? 1.2 : 1, base === "fox" ? 1.9 : 1.65, 1));
          node.position.set(side * .38, .91, position.z); node.rotation.z = -side * .15;
        }
      });
    },
    dispose() {
      originalGeometry.forEach((geometry, node) => { node.geometry = geometry; });
      headParts.forEach(part => { part.lease?.release(); part.lease = undefined; });
      earGeometry.forEach(ear => { ear.node.geometry = ear.original; ear.pointed?.release(); ear.pointed = undefined; });
    },
  });
}
