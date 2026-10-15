import { Object3D, Skeleton, SkinnedMesh } from "three";

/** Share identical bone palettes within one clone, never between participants. */
export function shareClonedSkeletons(root: Object3D) {
  const palettes = new Map<string, Skeleton[]>();
  const replacements = new Map<Skeleton, Skeleton>();
  root.traverse(node => {
    if (!(node instanceof SkinnedMesh)) return;
    const skeleton = node.skeleton;
    const previous = replacements.get(skeleton);
    if (previous) { node.skeleton = previous; return; }
    if (skeleton.bones.length !== skeleton.boneInverses.length) return;
    const key = skeleton.bones.map(bone => bone.uuid).join(":");
    const candidates = palettes.get(key) ?? [];
    const shared = candidates.find(candidate => candidate.boneInverses.every((inverse, index) => inverse.equals(skeleton.boneInverses[index])));
    if (shared === skeleton) return;
    if (shared) {
      // Bind matrices remain per mesh. Identical joint order and inverse-bind
      // matrices make only the skeleton's repeated palette update redundant.
      replacements.set(skeleton, shared);
      node.skeleton = shared;
    } else { candidates.push(skeleton); palettes.set(key, candidates); }
  });
  for (const skeleton of replacements.keys()) skeleton.dispose();
}
