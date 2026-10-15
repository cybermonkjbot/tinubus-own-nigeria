import * as THREE from "three";

/** Replace the old crouched march on the three reusable street rigs. Work and
 * gesture clips remain authored. Generate once per decoded source, on a clone. */
export function createNpcMotionClips(actor: THREE.Object3D, clips: readonly THREE.AnimationClip[], npcId: string) {
  const idle = clips.find(clip => clip.name === `${npcId}_idle`);
  const original = clips.find(clip => clip.name === `${npcId}_walk`);
  if (!idle || !original) return [];
  const bones: THREE.Bone[] = [];
  actor.traverse(node => { if (node instanceof THREE.Bone) bones.push(node); });
  const named = (name: string) => bones.find(bone => bone.userData.name === name
    || bone.name.replace(/_\d+$/, "") === THREE.PropertyBinding.sanitizeNodeName(name));
  const root = named("root"), spine = named("spine02");
  const legs = (["L", "R"] as const).map(side => ({ upper: named(`upperleg.${side}`), lower: named(`lowerleg.${side}`), end: named(`foot.${side}`) }));
  const arms = (["L", "R"] as const).map(side => ({ upper: named(`upperarm.${side}`), lower: named(`lowerarm.${side}`), end: named(`wrist.${side}`) }));
  if (!root || !spine || [...legs, ...arms].some(limb => !limb.upper || !limb.lower || !limb.end)) return [];
  const saved = bones.map(bone => ({ bone, position: bone.position.clone(), quaternion: bone.quaternion.clone(), scale: bone.scale.clone() }));
  const mixer = new THREE.AnimationMixer(actor);
  try {
    mixer.clipAction(idle).play(); mixer.update(0); actor.updateWorldMatrix(true, true);
    const base = bones.map(bone => ({ bone, position: bone.position.clone(), quaternion: bone.quaternion.clone(), scale: bone.scale.clone() }));
    const inverse = actor.matrixWorld.clone().invert(), actorRotation = actor.getWorldQuaternion(new THREE.Quaternion());
    const localPoint = (bone: THREE.Bone) => bone.getWorldPosition(new THREE.Vector3()).applyMatrix4(inverse);
    const endPoints = [...legs, ...arms].map(limb => localPoint(limb.end!));
    const endRotations = [...legs, ...arms].map(limb => actorRotation.clone().invert().multiply(limb.end!.getWorldQuaternion(new THREE.Quaternion())));
    const hip = localPoint(root), stature = Math.max(1.4, localPoint(named("head") ?? spine).y + .18);
    const supports = legs.map(limb => ({ hip: localPoint(limb.upper!),
      length: localPoint(limb.upper!).distanceTo(localPoint(limb.lower!)) + localPoint(limb.lower!).distanceTo(localPoint(limb.end!)) }));
    const size = stature / 1.72;
    const head = named("head");
    const moved = new Set<THREE.Bone>([root, spine, ...(head ? [head] : [])]);
    [...legs, ...arms].forEach(limb => { moved.add(limb.upper!); moved.add(limb.lower!); moved.add(limb.end!); });
    const tracks = [...moved].map(bone => ({ bone, rotations: [] as number[], positions: [] as number[] }));
    const times: number[] = [];
    const a = new THREE.Vector3(), b = new THREE.Vector3(), c = new THREE.Vector3();
    const direction = new THREE.Vector3(), bend = new THREE.Vector3(), joint = new THREE.Vector3();
    const from = new THREE.Vector3(), to = new THREE.Vector3(), quaternion = new THREE.Quaternion(), parentRotation = new THREE.Quaternion();
    const orient = (bone: THREE.Bone, child: THREE.Bone, target: THREE.Vector3) => {
      bone.getWorldPosition(a); child.getWorldPosition(b);
      from.copy(b).sub(a).normalize(); to.copy(target).sub(a).normalize();
      quaternion.setFromUnitVectors(from, to).multiply(bone.getWorldQuaternion(new THREE.Quaternion()));
      bone.parent!.getWorldQuaternion(parentRotation).invert(); bone.quaternion.copy(parentRotation.multiply(quaternion));
      bone.updateWorldMatrix(false, true);
    };
    const solve = (limb: typeof legs[number], target: THREE.Vector3, pole: THREE.Vector3) => {
      const upper = limb.upper!, lower = limb.lower!, end = limb.end!;
      upper.getWorldPosition(a); lower.getWorldPosition(b); end.getWorldPosition(c);
      const start = a.clone(), u = a.distanceTo(b), l = b.distanceTo(c);
      direction.copy(target).sub(start); const distance = Math.max(.001, Math.min(direction.length(), u + l - .0005)); direction.normalize();
      const along = (u * u - l * l + distance * distance) / (2 * distance);
      bend.copy(pole).sub(start).addScaledVector(direction, -pole.clone().sub(start).dot(direction)).normalize();
      joint.copy(start).addScaledVector(direction, along).addScaledVector(bend, Math.sqrt(Math.max(0, u * u - along * along)));
      const endpoint = start.clone().addScaledVector(direction, distance);
      orient(upper, lower, joint); orient(lower, end, endpoint);
    };
    const setWorldRotation = (bone: THREE.Bone, rotation: THREE.Quaternion) => {
      bone.parent!.getWorldQuaternion(parentRotation).invert(); bone.quaternion.copy(parentRotation.multiply(rotation)); bone.updateWorldMatrix(false, true);
    };
    for (let frame = 0; frame <= 48; frame++) {
      const phase = frame / 48, angle = phase * Math.PI * 2;
      for (const pose of base) { pose.bone.position.copy(pose.position); pose.bone.quaternion.copy(pose.quaternion); pose.bone.scale.copy(pose.scale); }
      actor.updateWorldMatrix(true, true);
      const pelvis = hip.clone();
      pelvis.x += .009 * Math.sin(angle) * size;
      const feet = legs.map((_, side) => {
        const step = (phase + side * .5) % 1;
        const foot = endPoints[side].clone();
        foot.x *= .58;
        if (step < .6) foot.z += (.30 - step * 1.0) * size;
        else {
          const progress = (step - .6) / .4, eased = progress * progress * (3 - 2 * progress);
          foot.z += (-.30 + .60 * eased) * size;
          foot.y += .034 * Math.sin(Math.PI * progress) ** 2 * size;
        }
        return foot;
      });
      // Keep each planted ankle reachable instead of lifting it when the leg
      // reaches full extension. The pelvis rises again over the support foot.
      const drop = Math.max(.004 * size, ...feet.map((foot, side) => {
        const support = supports[side], dx = support.hip.x + pelvis.x - hip.x - foot.x, dz = support.hip.z - foot.z;
        const vertical = Math.sqrt(Math.max(.01, (support.length - .006 * size) ** 2 - dx * dx - dz * dz));
        return support.hip.y - foot.y - vertical;
      }));
      pelvis.y -= drop;
      root.position.copy(root.parent!.worldToLocal(actor.localToWorld(pelvis))); root.updateWorldMatrix(false, true);
      for (let side = 0; side < 2; side++) {
        const sign = side ? -1 : 1, foot = feet[side];
        solve(legs[side], actor.localToWorld(foot), actor.localToWorld(new THREE.Vector3(endPoints[side].x * .58, hip.y * .55, .8 * size)));
        setWorldRotation(legs[side].end!, actorRotation.clone().multiply(endRotations[side]));
        const hand = endPoints[side + 2].clone();
        hand.z += sign * .16 * Math.cos(angle - .15) * size;
        hand.y += (.012 + .015 * Math.sin(angle + side * Math.PI)) * size;
        hand.x += sign * .006 * Math.sin(angle) * size;
        solve(arms[side], actor.localToWorld(hand), actor.localToWorld(new THREE.Vector3(sign * .42 * size, stature * .62, -.18 * size)));
        const wrist = endRotations[side + 2].clone();
        wrist.premultiply(new THREE.Quaternion().setFromAxisAngle(new THREE.Vector3(1, 0, 0), sign * .11 * Math.cos(angle - .4)));
        setWorldRotation(arms[side].end!, actorRotation.clone().multiply(wrist));
      }
      for (const track of tracks) { track.rotations.push(...track.bone.quaternion.toArray()); track.positions.push(...track.bone.position.toArray()); }
      times.push(phase);
    }
    const clip = (name: string, duration: number) => new THREE.AnimationClip(name, duration, tracks.flatMap(track => [
      new THREE.QuaternionKeyframeTrack(`${track.bone.name}.quaternion`, times, track.rotations),
      new THREE.VectorKeyframeTrack(`${track.bone.name}.position`, times, track.positions),
    ]));
    const walk = clip(original.name, 1);
    tracks.forEach(track => { track.rotations.length = 0; track.positions.length = 0; }); times.length = 0;
    // Unarmed idle repertoire. Existing work/gesture clips keep task ownership;
    // the duty-rifle overlay replaces only the arms on armed officers.
    const duration = 36, smooth = (value: number) => { const t = THREE.MathUtils.clamp(value,0,1); return t*t*(3-2*t); };
    for (let frame = 0; frame <= 144; frame++) {
      const seconds = frame / 4, angle = seconds / duration * Math.PI * 2;
      const section = Math.floor(seconds / 6) % 6, within = seconds % 6;
      const weight = smooth(within / 1.2) * smooth((6-within) / 1.2);
      for (const pose of base) { pose.bone.position.copy(pose.position); pose.bone.quaternion.copy(pose.quaternion); pose.bone.scale.copy(pose.scale); }
      actor.updateWorldMatrix(true,true);
      const pelvis = hip.clone(); pelvis.x += Math.sin(angle*2)*.013*size; pelvis.y -= .009*size;
      root.position.copy(root.parent!.worldToLocal(actor.localToWorld(pelvis))); root.updateWorldMatrix(false,true);
      for (let side=0;side<2;side++) {
        const sign = side ? -1 : 1, foot = endPoints[side].clone(); foot.x *= .76;
        solve(legs[side],actor.localToWorld(foot),actor.localToWorld(new THREE.Vector3(foot.x,hip.y*.55,.8*size)));
        setWorldRotation(legs[side].end!,actorRotation.clone().multiply(endRotations[side]));
        const resting = endPoints[side+2].clone(); resting.z += Math.sin(angle*3+side*.3)*.008*size;
        const target = resting.clone(), fingerDirection = new THREE.Vector3(0,-1,0);
        if (section === 2) { target.set(sign*.20*size,stature*.57,.015*size); fingerDirection.set(-sign*.3,-1,.1); }
        else if (section === 4) { target.set(-sign*.14*size,stature*(side ? .66 : .69),.23*size); fingerDirection.set(-sign,0,.05); }
        else if (section === 5 && side === 1) { target.set(-.25*size,stature*.65,.31*size); fingerDirection.set(0,.25,1); }
        resting.lerp(target,weight);
        solve(arms[side],actor.localToWorld(resting),actor.localToWorld(new THREE.Vector3(sign*(section===2 ? .48 : .38)*size,stature*.59,-.05*size)));
        const wrist = endRotations[side+2].clone();
        const posed = wrist.clone().premultiply(new THREE.Quaternion().setFromUnitVectors(new THREE.Vector3(0,-1,0),fingerDirection.normalize()));
        setWorldRotation(arms[side].end!,actorRotation.clone().multiply(wrist.slerp(posed,weight)));
      }
      if (head) {
        const rotation = head.getWorldQuaternion(new THREE.Quaternion());
        const turn = Math.sin(angle*3)*.10 + (section===1?Math.sin(within/6*Math.PI*2)*.24*weight:0);
        rotation.premultiply(new THREE.Quaternion().setFromAxisAngle(new THREE.Vector3(0,1,0).applyQuaternion(actorRotation),turn));
        setWorldRotation(head,rotation);
      }
      for (const track of tracks) { track.rotations.push(...track.bone.quaternion.toArray()); track.positions.push(...track.bone.position.toArray()); }
      times.push(seconds);
    }
    return [walk,clip(idle.name,duration)];
  } finally {
    mixer.stopAllAction(); mixer.uncacheRoot(actor);
    for (const pose of saved) { pose.bone.position.copy(pose.position); pose.bone.quaternion.copy(pose.quaternion); pose.bone.scale.copy(pose.scale); }
    actor.updateWorldMatrix(true, true);
  }
}
