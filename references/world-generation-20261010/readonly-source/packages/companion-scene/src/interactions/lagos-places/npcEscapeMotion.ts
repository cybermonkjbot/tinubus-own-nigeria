import * as THREE from "three";

/** Reuse the published human gait on the cooks' existing skeletons.
 * Keep limb lengths and scales. Lift the seated cook using a leg-scaled root track. */
export function npcEscapeMotion(model: THREE.Object3D, clips: readonly THREE.AnimationClip[], npcId: string): THREE.AnimationClip | null {
  const own = clips.find(clip => clip.name === `${npcId}_walk`);
  if (own) return own;
  const source = clips.find(clip => clip.name === "street-tout-cap_walk");
  const root = model.getObjectByName(THREE.PropertyBinding.sanitizeNodeName(`${npcId} rig`));
  const sourceRoot = model.getObjectByName(THREE.PropertyBinding.sanitizeNodeName("street-tout-cap rig"));
  if (!source || !root || !sourceRoot) return null;
  const bones = new Map<string, THREE.Bone>();
  const sourceBones = new Map<string, THREE.Bone>();
  const canonical = (name: string) => name.replace(/_\d+$/, "");
  root.traverse(node => { if (node instanceof THREE.Bone) bones.set(canonical(node.name), node); });
  sourceRoot.traverse(node => { if (node instanceof THREE.Bone) sourceBones.set(canonical(node.name), node); });
  const legLength = (skeleton: Map<string, THREE.Bone>) => ["lowerleg.L", "foot.L"]
    .reduce((sum, name) => sum + (skeleton.get(THREE.PropertyBinding.sanitizeNodeName(name))?.position.length() ?? 0), 0);
  const sourceLength = legLength(sourceBones), targetLength = legLength(bones);
  if (sourceLength <= 0 || targetLength <= 0) return null;
  const tracks: THREE.KeyframeTrack[] = [];
  for (const track of source.tracks) {
    const binding = THREE.PropertyBinding.parseTrackName(track.name);
    const name = canonical(binding.nodeName ?? ""), bone = bones.get(name);
    if (!bone) continue;
    if (binding.propertyName === "quaternion") {
      const copied = track.clone(); copied.name = `${bone.name}.quaternion`; tracks.push(copied);
    } else if (name === "root" && binding.propertyName === "position") {
      const copied = track.clone(); copied.name = `${bone.name}.position`;
      for (let index = 0; index < copied.values.length; index++) copied.values[index] *= targetLength / sourceLength;
      tracks.push(copied);
    }
  }
  return tracks.length ? new THREE.AnimationClip(`${npcId}_walk`, source.duration, tracks) : null;
}
