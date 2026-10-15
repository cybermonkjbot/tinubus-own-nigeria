import * as THREE from "three";
import { companionCityParks } from "../../../../../packages/utils/src/companionCity";
import type { CityParkRoute } from "../../../../../packages/utils/src/companionCityParks";
import type { CompanionAssetDelivery } from "../../../../../packages/utils/src/companionAssets";
import { acquireCompanionGltf, type GltfLease, type SceneAsset } from "../appearance/gltfAssets";
import { createLagosAssetPart, disposeLagosAssetPart } from "../lagos-places/assetParts";
import { createCompanionGunfireReactions, type CompanionGunfireState } from "../../../../../packages/utils/src/companionGunfire";
import type { RoomNavigation, ScenePoint } from "../../../../../packages/utils/src/companionNavigation";

type Walker = { id:string; route:CityParkRoute; headings:Float64Array; phase:number; speed:number; root?:THREE.Group;
  actor?:THREE.Object3D; mixer?:THREE.AnimationMixer; walk?:THREE.AnimationAction; idle?:THREE.AnimationAction; moving?:boolean;
  poseTime?:number; pausedAt?:number; pausedSeconds?:number;
  routeDistance?:number; ordinary?:{point:ScenePoint; yaw:number} };

/** Ambient park NPCs, never copies of persisted residents. One asset lease and no independent clock. */
export function createCityParkWalkers(delivery:()=>CompanionAssetDelivery, changed:()=>void) {
  const group=new THREE.Group();group.name="Park walking NPCs";
  // The shared city plan is fixed for this factory's lifetime. Both walkers
  // use the same prepared headings without changing the route's coordinates.
  const routes=companionCityParks().routes.map(route=>{
    const headings=new Float64Array(route.lengths.length);
    for(let index=0;index<headings.length;index++){
      const a=route.points[index],b=route.points[index+1];
      headings[index]=Math.atan2(b[0]-a[0],b[2]-a[2]);
    }
    return {route,headings};
  });
  const walkers:Walker[]=routes.flatMap(({route,headings})=>[0,1].map(i=>({
    id:`park-walker:${route.id}:${i}`,route,headings,phase:i*.5+.17,speed:i?1.08:1.2,
  })));
  let provider:CompanionAssetDelivery|undefined, lease:GltfLease|undefined, asset:SceneAsset|undefined;
  let disposed=false, failed=false, near=false, dirty=false, targetX=NaN, targetZ=NaN;
  let lastSeconds=0;
  let reactions:ReturnType<typeof createCompanionGunfireReactions>|undefined, navigationRevision="";
  const clearWalker=(w:Walker)=>{
    w.routeDistance=undefined;w.ordinary=undefined;
    if(!w.root)return;
    w.mixer?.stopAllAction();if(w.actor)w.mixer?.uncacheRoot(w.actor);disposeLagosAssetPart(w.root);
    w.root=undefined;w.actor=undefined;w.mixer=undefined;w.walk=undefined;w.idle=undefined;w.moving=undefined;w.poseTime=undefined;
  };
  const release=()=>{walkers.forEach(clearWalker);asset=undefined;lease?.release();lease=undefined;};
  const wake=()=>{dirty=true;changed();};
  const nearRoute=(x:number,z:number)=>{
    for(const {route} of routes)for(const point of route.points){
      const dx=point[0]-x,dz=point[2]-z;
      // Reject points outside the same radius on either axis before hypot.
      if(Math.abs(dx)<260&&Math.abs(dz)<260&&Math.hypot(dx,dz)<260)return true;
    }
    return false;
  };
  const ordinaryPose=(w:Walker,time:number,reducedMotion:boolean)=>{
    const routineTime=(w.pausedAt??time)-(w.pausedSeconds??0);
    const sampledDistance=(w.route.length*w.phase+(reducedMotion?0:routineTime)*w.speed)%w.route.length;
    if(w.ordinary&&Object.is(w.routeDistance,sampledDistance))return w.ordinary;
    let distance=sampledDistance,segment=0;
    while(segment<w.route.lengths.length-1&&distance>w.route.lengths[segment])distance-=w.route.lengths[segment++];
    const a=w.route.points[segment],b=w.route.points[segment+1],amount=distance/w.route.lengths[segment];
    // Reactions retain points as escape origins. Replace a changed snapshot;
    // never update its coordinates in place, even after its mesh leaves range.
    const pose={point:[a[0]+(b[0]-a[0])*amount,0,a[2]+(b[2]-a[2])*amount] as const,yaw:w.headings[segment]};
    w.routeDistance=sampledDistance;w.ordinary=pose;return pose;
  };
  const create=(w:Walker,index:number)=>{
    if(!asset)return;
    const npcId=index%2?"street-tout-shirt":"street-tout-cap";
    const part=createLagosAssetPart({id:w.id,key:"bestieTownSquareNpcs",npcId,point:[0,0,0],yaw:0,sourceYaw:-Math.PI/2},
      {scene:asset.scene,animations:asset.animations});
    try {
      const actor=part.actor!,mixer=new THREE.AnimationMixer(actor);
      w.root=part.group;w.actor=actor;w.mixer=mixer;
      w.walk=mixer.clipAction(part.clips.find(c=>c.name===`${npcId}_walk`)!);
      w.idle=mixer.clipAction(part.clips.find(c=>c.name===`${npcId}_idle`)!);
      // Keep animation node names, but discard inherited tout/vendor/service identity and voice metadata.
      part.group.traverse(node=>{
        node.userData={ambientParkWalkerId:w.id};
        if(node instanceof THREE.Mesh) {node.castShadow=false;if(node instanceof THREE.SkinnedMesh)node.frustumCulled=false;}
      });
      part.group.name="Park walker";part.group.userData.ambientParkWalkerId=w.id;group.add(part.group);
      part.group.userData.lifeRole="npc";part.group.userData.civilianVoiceId=w.id;part.group.userData.civilianVoiceVisible=true;
    } catch(error) {if(w.root)clearWalker(w);else disposeLagosAssetPart(part.group);throw error;}
  };
  return {group,
    update(target:THREE.Vector3,seconds:number|null,reducedMotion:boolean,gunfire?:CompanionGunfireState,navigation?:RoomNavigation,serverNow=0) {
      if(disposed||!Number.isFinite(target.x)||!Number.isFinite(target.z))return false;
      if(navigationRevision!==navigation?.revision){reactions?.clear();reactions=navigation?createCompanionGunfireReactions(navigation):undefined;navigationRevision=navigation?.revision??"";}
      const next=delivery();
      if(provider!==next){release();reactions?.clear();walkers.forEach(w=>{w.pausedAt=undefined;w.pausedSeconds=0;});provider=next;failed=false;dirty=true;}
      const active=target.x===targetX&&target.z===targetZ?near:nearRoute(target.x,target.z);
      targetX=target.x;targetZ=target.z;
      if(active&&!near)failed=false;near=active;
      if(lease&&(!active||!next.available("bestieTownSquareNpcs"))){release();dirty=true;}
      if(active&&!lease&&!failed&&next.available("bestieTownSquareNpcs")) {
        const current=acquireCompanionGltf("bestieTownSquareNpcs",next);lease=current;
        current.promise.then(value=>{if(disposed||lease!==current)return;asset=value;wake();})
          .catch(()=>{if(disposed||lease!==current)return;release();failed=true;wake();});
      }
      const valid=seconds!==null&&Number.isFinite(seconds)&&seconds>=0;
      if(valid)lastSeconds=seconds;
      let visibleCount=0;
      for(let index=0;index<walkers.length;index++) {
        const w=walkers[index],time=lastSeconds;
        // Reduced motion keeps a stable distributed pose and never runs walking clips.
        const ordinary=ordinaryPose(w,time,reducedMotion);
        const escape=reactions?.sample(w.id,ordinary,gunfire,serverNow);
        const point=escape?.point??ordinary.point;
        const x=point[0],z=point[2];
        const reacting=!!escape&&(escape.fleeing||Math.hypot(x-ordinary.point[0],z-ordinary.point[2])>.01);
        if(reacting)w.pausedAt??=time;
        else if(w.pausedAt!==undefined){w.pausedSeconds=(w.pausedSeconds??0)+Math.max(0,time-w.pausedAt);w.pausedAt=undefined;}
        const nearby=Math.hypot(x-target.x,z-target.z)<(w.root?170:145)&&visibleCount<12;
        if(!nearby){if(w.root){clearWalker(w);dirty=true;}continue;}
        visibleCount++;
        if(!w.root&&asset)try{create(w,index);dirty=true;}catch{failed=true;release();dirty=true;break;}
        if(!w.root)continue;
        const moving=valid&&!reducedMotion&&(reacting?!!escape?.moving:true);
        w.root.position.set(x,0,z);w.root.rotation.y=reacting?escape!.yaw:ordinary.yaw;
        w.root.userData.gunfireIncidentId=escape?.fleeing?escape.incidentId:undefined;
        w.root.userData.gunfireVoicePoint=point;
        const action=moving?w.walk!:w.idle!;
        const motionChanged=w.moving!==moving;
        if(motionChanged){w.walk?.stop();w.idle?.stop();action.reset().play();w.moving=moving;dirty=true;}
        const poseTime=moving?time*(reacting?(escape?.fleeing?2.8:1.4):w.speed/1.15)%action.getClip().duration:0;
        // Root placement and reactions still update. Re-evaluate bones only
        // when the chosen action or its absolute phase actually changes.
        if(motionChanged||w.poseTime!==poseTime){action.time=poseTime;w.mixer!.update(0);w.poseTime=poseTime;}
      }
      const result=dirty;dirty=false;return result;
    },dispose(){
      if(disposed)return;disposed=true;release();reactions?.clear();reactions=undefined;
      walkers.length=0;routes.length=0;provider=undefined;group.removeFromParent();group.clear();
    }};
}
