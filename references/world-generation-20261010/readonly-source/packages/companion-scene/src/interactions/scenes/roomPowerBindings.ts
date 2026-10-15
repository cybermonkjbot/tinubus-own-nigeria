import * as THREE from "three";
import type { ScenePowerBinding } from "./scenePower";

const maximumBindings = 128;
const bindingsByOwner = new WeakMap<THREE.Object3D, readonly ScenePowerBinding[]>();
const sameScalar = (a: ScenePowerBinding, b: ScenePowerBinding) => a.kind === b.kind && a.target === b.target;

/** Call where a fresh mains-powered target is created. Baselines are captured
 * later by createScenePowerAdapter, after saved room settings have been applied. */
export function registerRoomPowerBindings(owner: THREE.Object3D, bindings: readonly ScenePowerBinding[]): () => void {
  if (bindings.length > maximumBindings) throw new Error("Room power exceeds its binding budget.");
  const existing = bindingsByOwner.get(owner) ?? [];
  const added: ScenePowerBinding[] = [];
  for (const binding of bindings) {
    if (existing.some(value => sameScalar(value, binding)) || added.some(value => sameScalar(value, binding))) continue;
    if (existing.length + added.length >= maximumBindings) throw new Error("Room power exceeds its binding budget.");
    added.push(binding);
  }
  if (added.length) bindingsByOwner.set(owner, [...existing, ...added]);
  return () => {
    const current = bindingsByOwner.get(owner);
    if (!current || !added.length) return;
    const remaining = current.filter(binding => !added.includes(binding));
    if (remaining.length) bindingsByOwner.set(owner, remaining);
    else bindingsByOwner.delete(owner);
    added.length = 0;
  };
}

/** Mount-time collection of explicit registrations only. It never discovers
 * lights by type/name or selects arbitrary glowing materials. */
export function roomPowerBindings(root: THREE.Object3D): readonly ScenePowerBinding[] {
  const bindings: ScenePowerBinding[] = [];
  root.traverse(owner => {
    for (const binding of bindingsByOwner.get(owner) ?? []) {
      if (bindings.some(value => sameScalar(value, binding))) continue;
      if (bindings.length >= maximumBindings) throw new Error("Room power exceeds its binding budget.");
      bindings.push(binding);
    }
  });
  return bindings;
}

/** Caller must supply an instantiated, material-cloned room after its saved
 * lighting configuration, never a decoded/cache-owned GLTF or venue asset part.
 * Only the existing practical-emission metadata admits an authored material. */
export function registerOwnedAuthoredRoomPower(root: THREE.Group): () => void {
  const seen = new Set<THREE.Material>();
  const bindings: ScenePowerBinding[] = [];
  root.traverse(node => {
    if (!(node instanceof THREE.Mesh)) return;
    for (const material of Array.isArray(node.material) ? node.material : [node.material]) {
      if (!(material instanceof THREE.MeshStandardMaterial) || seen.has(material)) continue;
      seen.add(material);
      const emission = material.userData.baddie_practical_emission;
      const levels = material.userData.baddie_practical_emission_levels;
      const practical = typeof emission === "number" && Number.isFinite(emission) && emission >= 0
        || Array.isArray(levels) && levels.length === 3
          && levels.every(value => typeof value === "number" && Number.isFinite(value) && value >= 0);
      if (!practical) continue;
      if (bindings.length >= maximumBindings) throw new Error("Room power exceeds its binding budget.");
      bindings.push({ kind: "emissive", target: material });
    }
  });
  return registerRoomPowerBindings(root, bindings);
}

/** Call after the adapter restores baselines and before scene-resource disposal. */
export function clearRoomPowerBindings(root: THREE.Object3D): void {
  root.traverse(owner => { bindingsByOwner.delete(owner); });
}
