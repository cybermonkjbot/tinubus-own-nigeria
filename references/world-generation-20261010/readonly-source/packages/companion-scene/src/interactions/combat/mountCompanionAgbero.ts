import * as THREE from "three";
import type { CombatFighter } from "../../../../../packages/utils/src/companionCombat";
import { companionCombatAgberoBody, companionCombatAgberoSupported } from "../../../../../packages/utils/src/companionCombatAgbero";
import { createLagosAssetPart, disposeLagosAssetPart } from "../lagos-places/assetParts";
import type { EnvironmentAsset } from "../scenes/environments";
import type { CombatRigMount } from "./mountCompanionCombat";

/** World-owned, instance-bound suppression of the authored body. The controller
 * must preserve suppression through reset/disable and restore it on release. */
export type CompanionAgberoBodyLease = {
  npcId: typeof companionCombatAgberoBody.npcId;
  instanceId: string;
  fighterId: string;
  /** True only while this lease owns the unique, hidden authored body. */
  current(): boolean;
  release(): void;
};

/** Reuses the host's decoded NPC asset. No loader, animation loop or NPC state. */
export function mountCompanionAgbero(fighter: CombatFighter, parent: THREE.Group, options: {
  asset: EnvironmentAsset;
  lease: CompanionAgberoBodyLease;
  instanceId: string;
  presentation(): { animated: boolean };
}): CombatRigMount {
  const lease = options.lease;
  let part: ReturnType<typeof createLagosAssetPart> | undefined;
  let mixer: THREE.AnimationMixer | undefined, action: THREE.AnimationAction | undefined;
  let disposed = false;
  const current = () => !disposed && lease.current() && lease.instanceId === options.instanceId
    && lease.fighterId === fighter.id && lease.npcId === companionCombatAgberoBody.npcId;
  const ready = () => current() && !!part?.actor && part.actor.parent === part.group && part.actor.visible
    && part.group.parent === parent && part.group.visible;
  const dispose = () => {
    if (disposed) return;
    disposed = true;
    try {
      if (part) {
        mixer?.stopAllAction();
        if (part.actor) mixer?.uncacheRoot(part.actor);
        disposeLagosAssetPart(part.group);
      }
    } finally { lease.release(); }
  };
  try {
    if (!companionCombatAgberoSupported(fighter) || fighter.status === "left" || !current())
      throw new Error("The street encounter body is unavailable.");
    part = createLagosAssetPart({ id: fighter.id, key: "bestieTownSquareNpcs", npcId: lease.npcId,
      point: fighter.position, yaw: fighter.yaw, sourceYaw: companionCombatAgberoBody.sourceYaw }, options.asset);
    const actor = part.actor!;
    // The clone inherits source suppression. Show only its root; preserve
    // hidden descendants and leave the leased authored source untouched.
    actor.visible = true;
    mixer = new THREE.AnimationMixer(actor);
    const clips = new Map(part.clips.map(clip => [clip.name, clip]));
    const select = (name: "idle" | "walk" | "gesture", animated: boolean, dt: number) => {
      const next = mixer!.clipAction(clips.get(`${lease.npcId}_${name}`)!);
      if (next !== action) { action?.stop(); next.reset().play(); action = next; }
      if (!animated) next.time = 0;
      mixer!.update(animated ? Math.max(0, Math.min(.1, dt)) : 0);
    };
    select("idle", false, 0);
    const bounds = new THREE.Box3().setFromObject(part.group);
    if (bounds.isEmpty() || ![...bounds.min.toArray(), ...bounds.max.toArray()].every(Number.isFinite))
      throw new Error("The street encounter character has no usable body.");
    part.group.userData.combatFighterId = fighter.id;
    part.group.traverse(node => { if (node instanceof THREE.SkinnedMesh) node.frustumCulled = false; });
    parent.add(part.group);
    return {
      ready,
      update(pose, dt) {
        if (disposed) return;
        if (!ready() || pose.fighter.id !== fighter.id || !companionCombatAgberoSupported(pose.fighter))
          throw new Error("Reload the current street encounter character.");
        part!.group.position.fromArray(pose.position); part!.group.rotation.y = pose.yaw;
        part!.group.visible = pose.fighter.status !== "left";
        // Defeated bodies stay visible and stop gesturing. Their outcome is
        // authoritative; authored assets do not supply a death animation.
        const animated = options.presentation().animated && pose.fighter.status === "active";
        select(animated && pose.attackProgress > 0 ? "gesture" : animated && pose.speed > .1 ? "walk" : "idle", animated, dt);
      },
      dispose,
    };
  } catch (error) { dispose(); throw error; }
}
