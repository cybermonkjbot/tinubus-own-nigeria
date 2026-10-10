import * as THREE from "three";
import { GLTFLoader } from "three/addons/loaders/GLTFLoader.js";
import { MeshoptDecoder } from "three/addons/libs/meshopt_decoder.module.js";
import { clone } from "three/addons/utils/SkeletonUtils.js";
import type { GroupCallAppearance, GroupCallParticipant } from "./types";
import { createBaseRig } from "./baseRig";
import { createFaceRig } from "./faceRig";
import { createWardrobeRig } from "./wardrobeRig";
import { companionBodyColor } from "../../utils/src/companionAppearance";
import { shareClonedSkeletons } from "./skeletonSharing";
import { disposeSceneAsset, retainSceneAsset, type SceneAsset } from "./sceneAsset";
import { createAppearanceSnapshot } from "./appearanceSnapshot";

/** Visual input also used by the world renderer; no call or location state is required. */
export type GroupRigVisualState = Pick<GroupCallParticipant, "appearance" | "reaction" | "speechLevel">;

type AssetEntry = { users: number; promise: Promise<SceneAsset>; asset?: SceneAsset };
const sharedAssets = new Map<string, AssetEntry>();

/** One lease per renderer. Clone disposal never disposes pooled geometry or textures. */
export function acquireGroupRig(assetUrl = "/assets/bestie/3d/bestie.glb") {
  let sharedAsset = sharedAssets.get(assetUrl);
  if (!sharedAsset) {
    const entry: AssetEntry = {
      users: 0,
      promise: new GLTFLoader().setMeshoptDecoder(MeshoptDecoder).loadAsync(assetUrl).then(retainSceneAsset),
    };
    entry.promise = entry.promise.then(asset => {
      entry.asset = asset;
      if (entry.users === 0) {
        disposeSceneAsset(asset);
        if (sharedAssets.get(assetUrl) === entry) sharedAssets.delete(assetUrl);
      }
      return asset;
    }, error => {
      if (sharedAssets.get(assetUrl) === entry) sharedAssets.delete(assetUrl);
      throw error;
    });
    sharedAsset = entry;
    sharedAssets.set(assetUrl, entry);
  }
  const entry = sharedAsset;
  entry.users += 1;
  let released = false;
  return {
    promise: entry.promise,
    release() {
      if (released) return;
      released = true;
      entry.users -= 1;
      if (entry.users === 0 && entry.asset) {
        disposeSceneAsset(entry.asset);
        if (sharedAssets.get(assetUrl) === entry) sharedAssets.delete(assetUrl);
      }
    },
  };
}

export function createParticipantRig(asset: SceneAsset, consumedReactionId?: string) {
  const model = clone(asset.scene);
  shareClonedSkeletons(model);
  const baseRig = createBaseRig(model);
  const materials = new Map<THREE.Material, THREE.Material>();
  const originals = new Map<THREE.MeshStandardMaterial, THREE.Color>();
  model.traverse(node => {
    if (!(node instanceof THREE.Mesh)) return;
    // Whole-rig visibility includes animated limbs; cached skin bounds do not.
    if (node instanceof THREE.SkinnedMesh) node.frustumCulled = false;
    // Dense fur does not need a second shadow pass for each caller.
    node.castShadow = false;
    node.receiveShadow = false;
    const copyMaterial = (original: THREE.Material) => {
      let material = materials.get(original);
      if (!material) {
        material = original.clone();
        materials.set(original, material);
        if (material instanceof THREE.MeshStandardMaterial) originals.set(material, material.color.clone());
      }
      return material;
    };
    node.material = Array.isArray(node.material) ? node.material.map(copyMaterial) : copyMaterial(node.material);
  });
  const faceRig = createFaceRig(model);
  const wardrobe = createWardrobeRig(model);
  const mouth = model.getObjectByName("SpeechMouth");
  const mouthRestScale = mouth?.scale.y ?? 1;
  let mouthLevel = 0;
  const smile = model.getObjectByName("RestSmile");
  if (mouth) mouth.visible = false;
  if (smile) smile.visible = true;
  const response = new THREE.Group();
  const head = model.getObjectByName("Head");
  if (head?.parent) {
    response.position.copy(head.position);
    head.parent.add(response);
    const restOffset = new THREE.Group();
    restOffset.position.copy(head.position).negate();
    response.add(restOffset); restOffset.add(head);
  }
  const mixer = new THREE.AnimationMixer(model);
  const actions = new Map<string, THREE.AnimationAction>();
  // clipAction allocates track bindings even for clips that never play.
  for (const clip of asset.animations) {
    if (clip.name !== "Idle" && clip.name !== "Blink" && clip.name !== "Greeting") continue;
    actions.set(clip.name, mixer.clipAction(clip));
  }
  const greeting = actions.get("Greeting")?.setLoop(THREE.LoopOnce, 1);
  const blink = actions.get("Blink")?.setLoop(THREE.LoopOnce, 1);
  const appliedAppearance = createAppearanceSnapshot();
  let animatedBefore = false;
  let nextBlink = 2;
  let elapsed = 0;
  let reactionId = consumedReactionId;
  let reactionStart = 0;
  let reactionEnd = 0;
  let reactionKind: NonNullable<GroupCallParticipant["reaction"]>["kind"] = "nod";

  const applyAppearance = (appearance: GroupCallAppearance = {}) => {
    if (!appliedAppearance.begin(appearance)) return;
    baseRig.apply(appearance.baseId);
    originals.forEach((original, material) => {
      const name = material.name;
      const slot = ["Peach cream fur", "Cream guard hairs"].includes(name) ? "bodyColor"
        : ["Jacket fabric", "Jacket ribbing"].includes(name) ? "topColor"
        : ["Charcoal twill", "Twill seams"].includes(name) ? "trousersColor"
        : ["Ivory leather", "Rose leather", "Shoe sole"].includes(name) ? "footwearColor" : undefined;
      const color = slot === "bodyColor" ? companionBodyColor(appearance) : slot ? appearance[slot] : undefined;
      material.color.copy(original);
      if (color && /^#[0-9a-f]{6}$/i.test(color)) material.color.set(color);
      else if (name === "Jacket fabric") material.color.set(appearance.look === "ivory" ? "#e7dfcc" : "#b97e74");
      else if (name === "Jacket ribbing") material.color.set(appearance.look === "ivory" ? "#c6bca5" : "#95685f");
    });
    faceRig.apply(appearance);
    wardrobe.apply(appearance);
    appliedAppearance.capture(appearance);
  };

  return {
    model,
    get labelHeight() { return new THREE.Box3().setFromObject(model).max.y + .2; },
    update(participant: GroupRigVisualState, dt: number, animated: boolean, now: number) {
      applyAppearance(participant.appearance);
      elapsed += dt;
      if (animated !== animatedBefore) {
        mixer.stopAllAction();
        if (animated) actions.get("Idle")?.reset().play();
        animatedBefore = animated;
        reactionEnd = 0;
      }
      const reaction = participant.reaction;
      if (reaction && reaction.id !== reactionId) {
        reactionId = reaction.id;
        greeting?.stop();
        reactionEnd = 0;
        if (animated && Number.isFinite(reaction.expiresAt) && reaction.expiresAt > now) {
          reactionStart = now;
          reactionEnd = Math.min(reaction.expiresAt, now + 2000);
          reactionKind = reaction.kind;
          if (reaction.kind === "greeting") greeting?.reset().play();
        }
      }
      if (reactionEnd && (now >= reactionEnd || !reaction)) {
        greeting?.stop();
        reactionEnd = 0;
      }
      if (animated) {
        if (elapsed >= nextBlink) {
          blink?.reset().play();
          nextBlink = elapsed + 4;
        }
        mixer.update(dt);
      }
      const phase = reactionEnd && animated ? Math.min(1, (now - reactionStart) / 1150) : 1;
      const pulse = Math.sin(phase * Math.PI) ** 2;
      response.rotation.x = reactionKind === "nod" ? pulse * Math.sin(phase * Math.PI * 2) * .14 : 0;
      response.rotation.z = reactionKind === "delight" ? pulse * -.09 : 0;
      const level = animated && Number.isFinite(participant.speechLevel) ? THREE.MathUtils.clamp(participant.speechLevel ?? 0, 0, 1) : 0;
      mouthLevel = animated ? THREE.MathUtils.lerp(mouthLevel, level, 1 - Math.exp(-dt * 22)) : 0;
      if (mouth) { mouth.visible = mouthLevel > .025; mouth.scale.y = mouthRestScale * (.12 + mouthLevel * .88); }
      if (smile) smile.visible = mouthLevel <= .025;
    },
    dispose() {
      wardrobe.dispose(); faceRig.dispose(); baseRig.dispose();
      mixer.stopAllAction();
      mixer.uncacheRoot(model);
      model.traverse(node => { if (node instanceof THREE.SkinnedMesh) node.skeleton.dispose(); });
      materials.forEach(material => material.dispose());
      model.removeFromParent();
    },
  };
}
