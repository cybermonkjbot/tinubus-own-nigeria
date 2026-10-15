import * as THREE from "three";
import { clone as cloneSkeleton } from "three/addons/utils/SkeletonUtils.js";
import { shareClonedSkeletons } from "../../../../../packages/character-runtime/src/skeletonSharing";
import { clearDisposedSceneGraph, createSceneAssetCache } from "../../../../../packages/character-runtime/src/sceneAsset";
import type { CompanionPlaceAssetPart } from "../../../../../packages/utils/src/companionPlaces";
import type { EnvironmentAsset } from "../scenes/environments";
import { createNpcMotionClips } from "./npcWalk";

type PreparedNpc = { source: THREE.Object3D; clips: THREE.AnimationClip[]; orientation: THREE.Quaternion; scale: THREE.Vector3; walkPrepared?: boolean };
type SourceParts = {
  animations: THREE.AnimationClip[];
  npcs: Map<string, THREE.Object3D[]>;
  preparedNpcs: Map<string, PreparedNpc>;
  staticMeshes: THREE.Mesh[];
  staticBounds?: { source: THREE.Mesh; box: THREE.Box3 }[];
};
// Callers pass the immutable decoded source, never a placed or animated clone.
// Last-lease disposal drops mesh/clip references even if the source root is retained.
const sourceParts = createSceneAssetCache<SourceParts>();
const variantMaterials = new WeakMap<THREE.Group, Set<THREE.Material>>();

function varyStreetActor(group: THREE.Group, actor: THREE.Object3D, clips: THREE.AnimationClip[], id: string) {
  let seed = 2166136261;
  for (const character of id) seed = Math.imul(seed ^ character.charCodeAt(0),16777619) >>> 0;
  const profiles = [[.94,1.02,.98],[1.04,.98,1.02],[1,1,.94],[.98,1.04,1.04],[1.05,.96,.99],[.97,1.01,1.03]] as const;
  const shape = profiles[seed % profiles.length];
  let head: THREE.Bone | undefined;
  const owned = new Set<THREE.Material>(); variantMaterials.set(group,owned);
  actor.traverse(node => {
    if (node instanceof THREE.Bone && (node.userData.name === "head" || /^head(?:_\d+)?$/.test(node.name))) head = node;
    if (!(node instanceof THREE.SkinnedMesh) || !/body$/.test(node.name)) return;
    const tint = seed % 3 === 0 ? [.88,.84,.81] : seed % 3 === 1 ? [1,1,1] : [1.10,1.04,.98];
    const source = Array.isArray(node.material) ? node.material : [node.material];
    const surfaces = source.map(material => {
      const copy = material.clone(); owned.add(copy);
      if (copy instanceof THREE.MeshStandardMaterial) copy.color.multiply(new THREE.Color().setRGB(tint[0],tint[1],tint[2]));
      return copy;
    });
    node.material = Array.isArray(node.material) ? surfaces : surfaces[0];
  });
  group.scale.set(.96+(seed%7)*.012,.96+(seed%9)*.01,.97+(seed%5)*.014);
  group.userData.npcPoseOffset = seed % 36;
  if (!head) return clips;
  const name = `${head.name}.scale`, scale = head.scale.clone().multiply(new THREE.Vector3(...shape));
  head.scale.copy(scale);
  // A scale track keeps facial proportions through authored gesture clips too.
  return clips.map(clip => new THREE.AnimationClip(clip.name,clip.duration,[...clip.tracks.filter(track => track.name !== name),
    new THREE.VectorKeyframeTrack(name,[0,clip.duration],[...scale.toArray(),...scale.toArray()])],clip.blendMode));
}

function prepareSource(asset: EnvironmentAsset): SourceParts {
  const previous = sourceParts.get(asset.scene);
  if (previous && previous.animations === asset.animations) return previous;
  const prepared: SourceParts = { animations: asset.animations, npcs: new Map(), preparedNpcs: new Map(), staticMeshes: [] };
  asset.scene.updateWorldMatrix(true, true);
  asset.scene.traverse(node => {
    const npcId: unknown = node.userData.npcId;
    if (node.userData.lifeRole === "npc" && typeof npcId === "string") {
      const matches = prepared.npcs.get(npcId) ?? [];
      matches.push(node); prepared.npcs.set(npcId, matches);
    }
    if (node instanceof THREE.Mesh && !(node instanceof THREE.SkinnedMesh)) prepared.staticMeshes.push(node);
  });
  sourceParts.set(asset.scene, prepared);
  return prepared;
}

function prepareNpc(parts: SourceParts, npcId: string): PreparedNpc {
  const previous = parts.preparedNpcs.get(npcId);
  if (previous) return previous;
  const matches = parts.npcs.get(npcId);
  if (matches?.length !== 1) throw new Error(`The ${npcId} asset is unavailable or ambiguous.`);
  const source = matches[0];
  const clips = parts.animations.filter(clip => clip.name.startsWith(`${npcId}_`));
  const required = npcId === "akara-woman" || npcId === "mai-suya" ? ["work"] : ["idle", "walk", "gesture"];
  const names = new Set<string>();
  if (clips.some(clip => {
    if (names.has(clip.name) || !Number.isFinite(clip.duration) || clip.duration <= 0 || !clip.tracks.length) return true;
    names.add(clip.name); return false;
  }) || required.some(motion => !names.has(`${npcId}_${motion}`))) {
    throw new Error(`The ${npcId} asset requires unique, valid motion clips.`);
  }
  const nodes = new Set<THREE.Object3D>(); source.traverse(node => nodes.add(node));
  for (const node of nodes) {
    if (!node.matrixWorld.elements.every(Number.isFinite)) throw new Error(`The ${npcId} asset has invalid rig transforms.`);
    if (node instanceof THREE.SkinnedMesh && node.skeleton.bones.some(bone => !nodes.has(bone))) {
      throw new Error(`The ${npcId} asset needs a complete independent skeleton.`);
    }
  }
  const orientation = new THREE.Quaternion(), scale = new THREE.Vector3();
  source.matrixWorld.decompose(new THREE.Vector3(), orientation, scale);
  const prepared = { source, clips, orientation, scale };
  parts.preparedNpcs.set(npcId, prepared);
  return prepared;
}

/** Dispose independent skeleton/instance buffers, never cache geometry/materials. */
export function disposeLagosAssetPart(group: THREE.Group): void {
  variantMaterials.get(group)?.forEach(material => material.dispose()); variantMaterials.delete(group);
  const skeletons = new Set<THREE.Skeleton>();
  const instances = new Set<THREE.InstancedMesh>();
  group.traverse(node => {
    if (node instanceof THREE.SkinnedMesh) skeletons.add(node.skeleton);
    if (node instanceof THREE.InstancedMesh) instances.add(node);
  });
  skeletons.forEach(skeleton => skeleton.dispose());
  instances.forEach(instance => instance.dispose());
  group.removeFromParent(); clearDisposedSceneGraph(group);
}

/** Pass an immutable decoded source with unchanged transforms and clips.
 * Source preparation and geometry remain owned by the existing resource cache.
 * Returned transforms and skeletons are instance-owned. No network requests. */
export function createLagosAssetPart(definition: CompanionPlaceAssetPart, asset: EnvironmentAsset): {
  group: THREE.Group; actor?: THREE.Object3D; clips: THREE.AnimationClip[];
} {
  if (definition.point.length !== 3 || !definition.point.every(Number.isFinite) || !Number.isFinite(definition.yaw)
    || definition.sourceYaw !== undefined && !Number.isFinite(definition.sourceYaw)) throw new Error(`The ${definition.id} asset has invalid placement.`);
  const group = new THREE.Group(); group.name = definition.id;
  const parts = prepareSource(asset);
  if (definition.npcId) {
    const prepared = prepareNpc(parts, definition.npcId);
    const { source, orientation, scale } = prepared;
    const actor = cloneSkeleton(source); group.add(actor);
    try {
      shareClonedSkeletons(actor);
      // Preserve authored scale and floor height, then replace only scene origin
      // and heading. Skin and equipment keep their relative source transforms.
      actor.position.set(0, 0, 0); actor.quaternion.copy(orientation); actor.scale.copy(scale);
      const heading = definition.sourceYaw ?? 0;
      actor.quaternion.premultiply(new THREE.Quaternion().setFromAxisAngle(new THREE.Vector3(0, 1, 0), -heading));
      actor.updateMatrix(); actor.matrixAutoUpdate = true;
      if (!prepared.walkPrepared) {
        const motions = createNpcMotionClips(actor, prepared.clips, definition.npcId);
        prepared.clips = prepared.clips.map(clip => motions.find(motion => motion.name === clip.name) ?? clip);
        prepared.walkPrepared = true;
      }
      group.position.fromArray(definition.point); group.rotation.y = definition.yaw;
      const clips = definition.npcId.startsWith("street-tout") ? varyStreetActor(group,actor,prepared.clips,definition.id) : prepared.clips.slice();
      return { group, actor, clips };
    } catch (error) { disposeLagosAssetPart(group); throw error; }
  }
  const bounds = definition.sourceBounds;
  if (!bounds) throw new Error(`The ${definition.id} asset has no measured selection.`);
  const selection = new THREE.Box3(new THREE.Vector3(bounds.minX, bounds.y ?? 0, bounds.minZ),
    new THREE.Vector3(bounds.maxX, (bounds.y ?? 0) + bounds.height, bounds.maxZ));
  const selected: THREE.Mesh[] = [];
  // Precise vertex bounds are evaluated once, and only if static parts are used.
  parts.staticBounds ??= parts.staticMeshes.map(source => ({ source, box: new THREE.Box3().setFromObject(source, true) }));
  // Select complete static meshes by measured location, never a scene-wide
  // material/name substring. Exclude floor/wall meshes crossing the selection.
  for (const { source, box } of parts.staticBounds) {
    const padding = .015;
    if (box.min.x >= selection.min.x - padding && box.max.x <= selection.max.x + padding &&
      box.min.z >= selection.min.z - padding && box.max.z <= selection.max.z + padding &&
      box.min.y >= selection.min.y - padding && box.max.y <= selection.max.y + padding) selected.push(source);
  }
  if (!selected.length) throw new Error(`No measured geometry is available for ${definition.id}.`);
  const shift = new THREE.Matrix4().makeTranslation(-(bounds.minX + bounds.maxX) / 2, -(bounds.y ?? 0), -(bounds.minZ + bounds.maxZ) / 2);
  for (const source of selected) {
    const mesh = new THREE.Mesh(source.geometry, source.material);
    mesh.name = source.name;
    mesh.matrix.copy(shift).multiply(source.matrixWorld);
    mesh.matrixAutoUpdate = false; mesh.castShadow = true; mesh.receiveShadow = true;
    mesh.userData = { ...source.userData, lagosAssetPart: definition.id };
    group.add(mesh);
  }
  group.position.fromArray(definition.point); group.rotation.y = definition.yaw;
  return { group, clips: [] };
}
