import * as THREE from "three";
import { mergeGeometries } from "three/addons/utils/BufferGeometryUtils.js";
import { sampleTransitVehicle, sampleTransitPresentation, transitRules, transitSeatPoint,
  type TransitPlan, type TransitSnapshot, type TransitRide, type TransitSample, type TransitNpcActor, type CompanionTransitKind } from "../../../../../packages/utils/src/companionCityTransit";
import { admitTransitSnapshot } from "../../../../../packages/utils/src/companionTransitAuthority";
import type { LagosPlaceFocus } from "../lagos-places/scene";
import type { ScenePoint } from "../../../../../packages/utils/src/companionNavigation";
import type { CompanionAssetDelivery, CompanionAssetKey } from "../../../../../packages/utils/src/companionAssets";
import { publicCompanionAssets } from "../appearance/CompanionAssets";
import { acquireCompanionGltf, instantiateGltf, type GltfLease } from "../appearance/gltfAssets";
import { acquireGeometryVariant, type GeometryLease } from "../../../../../packages/character-runtime/src/geometryLease";

type TransitFallback = { root: THREE.Group; doors?: THREE.Group[]; wheels?: THREE.Mesh[] };
type HostedModel = { root: THREE.Group; fallback?: TransitFallback; key: CompanionAssetKey; modelId?: CompanionTransitKind;
  lease?: GltfLease; instance?: ReturnType<typeof instantiateGltf>; failed: boolean; near: boolean; poseDirty: boolean;
  wheels: { node: THREE.Object3D; base: THREE.Quaternion; radius: number }[];
  doors: { node: THREE.Object3D; base: THREE.Quaternion; direction: number }[] };
type VehicleBody = { root: THREE.Group;
  npcs: {root:THREE.Group; seated:THREE.Group; standing:THREE.Group; actorId?:string}[]; sample: TransitSample | null;
  wheelTravel?: number; doorsOpen?: boolean };
const vehicleAssets = { brt: "bestieVehicleBrt", danfo: "bestieVehicleDanfo", keke: "bestieVehicleKeke" } as const;
/** City frame owner supplies time. Hosted geometry uses the existing versioned GLB cache. */
export function createCityTransit(plan: TransitPlan, delivery: () => CompanionAssetDelivery = () => publicCompanionAssets,
  changed: () => void = () => {}, floorY = 0) {
  const group = new THREE.Group(); group.name = "City public transport";
  const materials = new Set<THREE.Material>(), geometries = new Set<THREE.BufferGeometry>();
  const batchLeases = new Map<THREE.Mesh, GeometryLease>();
  const stops = new Map<THREE.Group, TransitPlan["stops"][number]>(), vehicles = new Map<string, VehicleBody>();
  let snapshot: TransitSnapshot | null = null, disposed = false;
  const hosted = new Map<THREE.Group, HostedModel>();
  const fleet = new Map(plan.vehicles.map(vehicle=>[vehicle.id,vehicle]));
  let states = new Map<string,TransitSnapshot["states"][number]>();
  const nearby:TransitPlan["vehicles"]=[];
  let lastFleetAt=-Infinity, sampledAt=-Infinity;
  const fleetCandidates: number[] = [], fleetDistances = new Float64Array(plan.vehicles.length), visibleIds = new Set<string>();
  const compareCandidates = (a: number, b: number) => fleetDistances[a] - fleetDistances[b]
    || plan.vehicles[a].id.localeCompare(plan.vehicles[b].id);
  let stopViewX = NaN, stopViewZ = NaN;
  const fleetViewer=new THREE.Vector3(Infinity,0,Infinity);
  const samples=new Map<string,{sample:TransitSample|null;actors:TransitNpcActor[]|null}>();
  const sampleVehicle=(vehicle:TransitPlan["vehicles"][number],now:number,withActors=false)=>{
    if(sampledAt!==now){samples.clear();sampledAt=now;}
    let frame=samples.get(vehicle.id);
    if(!frame||withActors&&frame.actors===null){
      // A fleet-only sample can be upgraded after admission. Re-sample both
      // together so no lazy callback combines an old pose with newer inputs.
      frame=snapshot?(withActors?sampleTransitPresentation(plan,vehicle,snapshot.epochMs,now,states.get(vehicle.id))
        :{sample:sampleTransitVehicle(plan,vehicle,snapshot.epochMs,now,states.get(vehicle.id)),actors:null}):{sample:null,actors:null};
      samples.set(vehicle.id,frame);
    }
    return frame;
  };
  let provider: CompanionAssetDelivery | undefined, dirty = false, moving = false, nextWakeAt = Infinity;
  const spin = new THREE.Quaternion(), spinAxis = new THREE.Vector3(1,0,0), hingeAxis = new THREE.Vector3(0,1,0);
  const wake = () => { dirty = true; changed(); };
  const releaseModel = (entry: HostedModel) => {
    entry.instance?.dispose(); entry.instance = undefined;
    entry.lease?.release(); entry.lease = undefined;
    entry.wheels = []; entry.doors = []; entry.poseDirty = true;
    // A failed or replaced stop model may need its fallback on a stationary frame.
    if (!entry.modelId) stopViewX=stopViewZ=NaN;
  };
  function requestModel(entry: HostedModel) {
    if (disposed || !provider || !entry.near || entry.lease || entry.failed || !provider.available(entry.key)) return;
    const lease = acquireCompanionGltf(entry.key, provider); entry.lease = lease;
    lease.promise.then(asset => {
      if (disposed || entry.lease !== lease) return;
      const instance = instantiateGltf(asset);
      try {
        const nodes: THREE.Object3D[] = []; instance.scene.traverse(node => nodes.push(node));
        const root = entry.modelId ? nodes.find(node => node.userData.modelId === entry.modelId) : null;
        if (entry.modelId && (!root || root.userData.front !== "+Z" || root.userData.units !== "metres"))
          throw new Error("Transit vehicle coordinate metadata differs.");
        if (!entry.modelId && !nodes.some(node=>node.userData.assetVersion==="brt-stop-v01"
          && node.userData.runtimeForward==="+Z"&&node.userData.runtimeUp==="+Y"&&node.userData.units==="metres"))
          throw new Error("BRT stop coordinate metadata differs.");
        let meshes = 0;
        instance.scene.traverse(node => {
          if (node instanceof THREE.Mesh) { meshes++; node.castShadow = true; node.receiveShadow = true; }
          // GLTFLoader sanitizes names; original author names are retained in userData.name.
          const name = typeof node.userData.name === "string" ? node.userData.name : node.name;
          const radius = node.userData.wheelRadius;
          if (name.includes(":wheel:") && node.userData.spinAxis === "X" && typeof radius === "number" && radius > 0)
            entry.wheels.push({ node, base: node.quaternion.clone(), radius });
          const door = /^brt:door:\d+:(\d+)$/.exec(name);
          if (door && node.userData.hingeAxis === "Y")
            entry.doors.push({ node, base: node.quaternion.clone(), direction: Number(door[1]) % 2 ? 1 : -1 });
        });
        if (!meshes) throw new Error("Empty transit asset.");
        // Exports already bake Blender -Y forward into glTF +Z. Keep metres and the ground root.
        entry.instance = instance; entry.root.add(instance.scene); releaseFallback(entry); entry.poseDirty = true;
      } catch (error) { instance.dispose(); throw error; }
      wake();
    }).catch(() => {
      if (disposed || entry.lease !== lease) return;
      releaseModel(entry); entry.failed = true; wake();
    });
  }
  function addHosted(root: THREE.Group, key: CompanionAssetKey, modelId?: CompanionTransitKind) {
    const entry: HostedModel = { root, key, modelId, failed: false, near: false, poseDirty: true, wheels: [], doors: [] };
    hosted.set(root,entry); return entry;
  }
  const mat = (color: string, metalness = 0, roughness = .65) => {
    const material = new THREE.MeshStandardMaterial({ color, metalness, roughness }); materials.add(material); return material;
  };
  const blue = mat("#1255b5", .12,.4), navy = mat("#12305e"), ivory = mat("#ecefe9"), steel = mat("#88939c", .75,.3);
  const asphalt = mat("#575955"), yellow = mat("#d3ad41"), tyre = mat("#222629"), glazing = mat("#5a8697", .3,.24);
  const danfo = mat("#edbf27"), keke = mat("#3b926c"), seatMaterial = mat("#303f55"), skin = mat("#68432f");
  const lamps = mat("#f5e7bc");
  const boxGeometry = new THREE.BoxGeometry(1,1,1); geometries.add(boxGeometry);
  const wheelGeometry = new THREE.CylinderGeometry(1,1,1,16); wheelGeometry.rotateZ(Math.PI/2); geometries.add(wheelGeometry);
  const headGeometry = new THREE.SphereGeometry(.12,10,8); geometries.add(headGeometry);
  const mesh = (owner: THREE.Group, name: string, material: THREE.Material, position: readonly number[], size: readonly number[]) => {
    const object = new THREE.Mesh(boxGeometry,material); object.name=name; object.position.set(position[0],position[1],position[2]);
    object.scale.set(size[0],size[1],size[2]); owner.add(object); return object;
  };
  function batch(owner: THREE.Group) {
    const batches = new Map<THREE.Material, THREE.Mesh[]>();
    for (const child of [...owner.children]) if (child instanceof THREE.Mesh && child.geometry === boxGeometry) {
      child.updateMatrix();
      const material = child.material as THREE.Material;
      const parts = batches.get(material) ?? []; parts.push(child); batches.set(material,parts); owner.remove(child);
    }
    batches.forEach((parts,material) => {
      // Local matrices in source order fully describe this batch of immutable
      // boxes. Material selection and parent transforms stay per mesh.
      const key = parts.map(part => part.matrix.elements.map(value => Object.is(value, -0) ? "-0" : value).join(",")).join(";");
      const lease = acquireGeometryVariant(boxGeometry, key, () => {
        const copies: THREE.BufferGeometry[] = [];
        try {
          for (const part of parts) {
            const copy = boxGeometry.clone(); copies.push(copy); copy.applyMatrix4(part.matrix);
          }
          const geometry = mergeGeometries(copies);
          if (!geometry) throw new Error("Transit geometry could not be assembled.");
          return geometry;
        } finally { copies.forEach(copy => copy.dispose()); }
      });
      let object: THREE.Mesh | undefined;
      try {
        object = new THREE.Mesh(lease.geometry,material); object.castShadow=true; object.receiveShadow=true;
        // Geometry already contains the local box transforms. Doors, vehicle
        // roots and passenger pose groups retain their automatic transforms.
        object.matrixAutoUpdate=false;
        owner.add(object); batchLeases.set(object,lease);
      } catch (error) { object?.removeFromParent(); lease.release(); throw error; }
    });
  }
  function releaseRoot(root: THREE.Group) {
    root.traverse(object => {
      if (!(object instanceof THREE.Mesh)) return;
      const lease = batchLeases.get(object);
      if (lease) { batchLeases.delete(object); lease.release(); }
    });
    root.removeFromParent(); root.clear();
  }
  function releaseFallback(entry: HostedModel) {
    if (!entry.fallback) return;
    releaseRoot(entry.fallback.root); entry.fallback=undefined; entry.poseDirty=true;
  }
  function ensureFallback(entry: HostedModel) {
    if (entry.instance || entry.fallback) return;
    const fallback = entry.modelId ? vehicleFallback(entry.modelId) : stopFallback();
    try {
      entry.root.add(fallback.root); entry.fallback=fallback; entry.poseDirty=true; dirty=true;
    } catch (error) { releaseRoot(fallback.root); throw error; }
  }
  function releaseVehicle(body: Pick<VehicleBody, "root" | "npcs">) {
    const entry = hosted.get(body.root);
    if (entry) { releaseModel(entry); releaseFallback(entry); hosted.delete(body.root); }
    releaseRoot(body.root); body.npcs.forEach(npc => releaseRoot(npc.root));
  }
  function dispose() {
    if (disposed) return; disposed=true;
    hosted.forEach(entry=>{releaseModel(entry);releaseFallback(entry);}); hosted.clear(); group.removeFromParent(); group.clear();
    snapshot=null; vehicles.clear(); stops.clear(); samples.clear(); states.clear(); nearby.length=0;
    fleetCandidates.length=0; visibleIds.clear();
    batchLeases.forEach(lease=>lease.release()); batchLeases.clear();
    geometries.forEach(geometry=>geometry.dispose()); materials.forEach(material=>material.dispose());
  }
  try {
    for (const stop of plan.stops) {
      const container = new THREE.Group(); container.name=stop.id;
      container.position.set(stop.shelter.x,stop.approach[1],stop.shelter.z); container.rotation.y=stop.shelter.yaw;
      group.add(container); stops.set(container,stop); addHosted(container,"bestieBrtStop");
    }
  } catch (error) { dispose(); throw error; }
  function stopFallback(): TransitFallback {
    const root = new THREE.Group();
    try {
      // Blender source [x,y,z] becomes runtime [x,z,-y], including asymmetric fittings.
      mesh(root,"Flush boarding apron",asphalt,[0,-.045,0],[3.1,.09,13.2]);
      mesh(root,"Tactile edge",yellow,[-1.26,.008,0],[.31,.016,12]);
      for (const z of [-3.8,3.8]) for (const x of [-1,1]) {
        mesh(root,"Column",blue,[x,1.38,z],[.13,2.72,.13]);
        mesh(root,"Base plate",steel,[x,.018,z],[.27,.032,.27]);
      }
      mesh(root,"Blue canopy",blue,[0,2.88,0],[2.6,.16,10.4]);
      mesh(root,"White soffit",ivory,[0,2.787,0],[2.48,.026,10.25]);
      for(const z of [-5.12,5.12])mesh(root,"Fascia",navy,[0,2.89,z],[2.64,.27,.09]);
      for(const z of [2.5,.8,-.9,-2.6]) {
        mesh(root,"Back glazing",glazing,[1.05,1.45,z],[.018,2.15,1.56]);
        mesh(root,"Visibility strip",ivory,[1.025,1.2,z],[.007,.08,1.53]);
        for(const end of [-.8,.8])mesh(root,"Glazing stile",steel,[1.05,1.45,z+end],[.055,2.2,.045]);
        for(const y of [.35,2.55])mesh(root,"Glazing rail",steel,[1.05,y,z],[.055,.045,1.65]);
      }
      for(const z of [2.4,1.3,.2]) {
        mesh(root,"Waiting seat",blue,[.62,.47,z],[.52,.065,.96]);
        mesh(root,"Backrest",blue,[.91,.8,z],[.065,.55,.96]);
        for(const end of [-.32,.32])mesh(root,"Seat support",steel,[.66,.235,z+end],[.065,.43,.055]);
      }
      mesh(root,"Route case",navy,[.995,1.52,-2],[.085,1.27,.94]);
      mesh(root,"Route panel",ivory,[.944,1.52,-2],[.012,1.17,.84]);
      mesh(root,"Waste bin",navy,[.65,.48,-3],[.46,.85,.46]);
      mesh(root,"Bin lid",steel,[.65,.92,-3],[.48,.04,.48]);
      mesh(root,"Bin opening",navy,[.409,.78,-3],[.012,.12,.28]);
      mesh(root,"Boarding paint",ivory,[-1.35,.013,-4.7],[.25,.008,1.2]);
      batch(root); return {root};
    } catch (error) { releaseRoot(root); throw error; }
  }
  function vehicleFallback(kind: CompanionTransitKind): TransitFallback {
    const rules=transitRules.kinds[kind],root=new THREE.Group(),doors:THREE.Group[]=[],wheels:THREE.Mesh[]=[];
    try {
      const l=rules.length,w=rules.width,h=kind==="brt"?3.2:kind==="danfo"?2.1:1.75,paint=kind==="brt"?blue:kind==="danfo"?danfo:keke;
      // Hollow passenger cabin. Local +Z is forward; +X is the door side.
      mesh(root,"Cabin floor",navy,[0,rules.floor-.055,0],[w,.11,l]);
      mesh(root,"Roof",paint,[0,h,0],[w,.13,l]);
      mesh(root,"Left lower body",paint,[-w/2,.85,0],[.08,.65,l]);
      mesh(root,"Front mask",paint,[0,.78,l/2],[w,.6,.1]);
      mesh(root,"Rear mask",paint,[0,1.02,-l/2],[w,1.3,.09]);
      mesh(root,"Windscreen",glazing,[0,h-.67,l/2-.025],[w-.18,.95,.045]);
      if(kind==="brt")mesh(root,"White side band",ivory,[-w/2-.044,1.3,0],[.02,.28,l-.2]);
      const doorZ=kind==="brt"?[rules.doorZ,-.75]:[rules.doorZ];
      for(let z=-l/2+.2;z<l/2-.2;z+=kind==="brt"?1.2:.7){
        mesh(root,"Left pillar",paint,[-w/2,h-.72,z],[.09,1.35,.085]);
        if(!doorZ.some(door=>Math.abs(z-door)<.65)){
          mesh(root,"Right pillar",paint,[w/2,h-.72,z],[.09,1.35,.085]);
          mesh(root,"Right lower panel",paint,[w/2,.85,z],[.08,.65,kind==="brt"?1.15:.64]);
        }
      }
      // Window openings intentionally remain open in this procedural fallback so riders are visible.
      for(const z of doorZ)for(const leaf of [-1,1]){
        const door=new THREE.Group();door.position.set(w/2,0,z+leaf*.31);root.add(door);doors.push(door);
        mesh(door,"Door frame",navy,[0,h/2,0],[.075,h-.18,.6]);
        mesh(door,"Door glass",glazing,[.045,h*.67,0],[.015,.82,.49]);batch(door);
      }
      for(const z of [l*.29,-l*.28])for(const x of [-w/2,w/2]){
        const wheel=new THREE.Mesh(wheelGeometry,tyre);wheel.position.set(x,.4,z);wheel.scale.set(.21,.4,.4);root.add(wheel);wheels.push(wheel);
      }
      for(const x of [-w*.33,w*.33])mesh(root,"Headlamp",lamps,[x,.8,l/2+.06],[.2,.12,.03]);
      for(let seat=0;seat<rules.seats;seat++){
        const p=transitSeatPoint({kind,point:{x:0,z:0,yaw:0}},seat);
        mesh(root,"Passenger seat",seatMaterial,[p[0],rules.seatHeight-.0375,p[2]],[kind==="keke"?.3:.4,.075,.42]);
        mesh(root,"Seat back",seatMaterial,[p[0],rules.seatHeight+.25,p[2]-.2],[kind==="keke"?.3:.4,.5,.075]);
      }
      batch(root); return {root,doors,wheels};
    } catch (error) { releaseRoot(root); throw error; }
  }
  function vehicleBody(kind: CompanionTransitKind, id: string): VehicleBody {
    const rules=transitRules.kinds[kind],container=new THREE.Group(),npcs:VehicleBody["npcs"]=[];
    try {
      container.name=id;
      ensureFallback(addHosted(container,vehicleAssets[kind],kind));
      for(let index=0;index<rules.npcSeats;index++){
        const npc=new THREE.Group(),seated=new THREE.Group(),standing=new THREE.Group();npc.name=`${id}:service-passenger:${index}`;
        npcs.push({root:npc,seated,standing});
        npc.add(seated,standing);
        seated.position.y=rules.seatHeight-rules.floor-.35;
        mesh(seated,"Passenger torso",index%2?ivory:blue,[0,.67,0],[.29,.39,.22]);
        const head=new THREE.Mesh(headGeometry,skin);head.position.y=.99;head.updateMatrix();head.matrixAutoUpdate=false;seated.add(head);
        for(const x of [-.09,.09]){
          mesh(seated,"Thigh",navy,[x,.42,.12],[.12,.14,.37]);mesh(seated,"Shin",navy,[x,.2,.29],[.11,.36,.12]);
          mesh(standing,"Leg",navy,[x,.36,0],[.12,.72,.13]);
          mesh(standing,"Arm",index%2?ivory:blue,[x*2.1,.94,0],[.11,.42,.15]);
        }
        mesh(standing,"Passenger torso",index%2?ivory:blue,[0,1.02,0],[.29,.42,.22]);
        const uprightHead=new THREE.Mesh(headGeometry,skin);uprightHead.position.y=1.34;uprightHead.updateMatrix();uprightHead.matrixAutoUpdate=false;standing.add(uprightHead);
        batch(seated);batch(standing);group.add(npc);
      }
      group.add(container);
      return {root:container,npcs,sample:null};
    } catch (error) { releaseVehicle({root:container,npcs}); throw error; }
  }
  group.visible=false;
  return {
    group,
    trafficObstacles() {
      const obstacles: { x: number; z: number; yaw: number; halfWidth: number; halfLength: number }[] = [];
      for (const body of vehicles.values()) if (body.root.visible && body.sample) {
        const rules = transitRules.kinds[body.sample.kind];
        obstacles.push({ ...body.sample.point, halfWidth: rules.width / 2, halfLength: rules.length / 2 });
      }
      return obstacles;
    },
    get moving() { return moving; },
    get nextWakeAt() { return nextWakeAt; },
    setSnapshot(next: TransitSnapshot | null) {
      if(disposed)return;
      if(next===snapshot)return;
      if(next) {
        try { admitTransitSnapshot(plan,next); } catch { next = null; }
        if(next && snapshot && next.instanceId === snapshot.instanceId && next.revision < snapshot.revision) return;
      }
      snapshot=next;group.visible=!!next;dirty=true;lastFleetAt=-Infinity;samples.clear();
      stopViewX=stopViewZ=NaN;
      states=new Map(next?.states.map(state=>[state.vehicleId,state])??[]);
      if (!next) { nearby.length=0;visibleIds.clear();fleetCandidates.length=0;moving = false; nextWakeAt = Infinity; hosted.forEach(entry => { releaseModel(entry); releaseFallback(entry); entry.near = false; }); }
    },
    update(now:number,viewer:THREE.Vector3,reducedMotion:boolean) {
      if(disposed||!snapshot||!Number.isSafeInteger(now)||now<snapshot.epochMs)return false;
      const next = delivery();
      if (provider !== next) { hosted.forEach(entry => { releaseModel(entry); entry.failed=false; }); provider=next; dirty=true; stopViewX=stopViewZ=NaN; }
      // Stops are fixed. Model completion can wake a frame without changing
      // their distance admission; snapshot/provider replacement invalidates it.
      if (viewer.x !== stopViewX || viewer.z !== stopViewZ) {
        stops.forEach((stop,root)=>{
          const visible=Math.hypot(stop.shelter.x-viewer.x,stop.shelter.z-viewer.z)<460;
          if(root.visible!==visible)dirty=true;
          root.visible=visible;
          const entry=hosted.get(root)!;
          if(visible&&!entry.near)entry.failed=false;
          entry.near=visible;
          if(visible)ensureFallback(entry);
          else {
            if(entry.lease)releaseModel(entry);
            releaseFallback(entry);
          }
        });
        // Commit only after every visible fallback is ready. A failed assembly
        // must not make the stationary-view cache suppress its next attempt.
        stopViewX=viewer.x;stopViewZ=viewer.z;
      }
      moving=false;nextWakeAt=Infinity;
      // Distant fleet membership needs the existing 250 ms wake, not every character frame.
      if(now<lastFleetAt||now-lastFleetAt>=250||fleetViewer.distanceToSquared(viewer)>64){
        fleetCandidates.length=0;
        for (let index=0;index<plan.vehicles.length;index++) {
          const vehicle=plan.vehicles[index];
          const {sample}=sampleVehicle(vehicle,now);
          const distance=sample?Math.hypot(sample.point.x-viewer.x,sample.point.z-viewer.z):Infinity;
          if(distance<480){fleetDistances[index]=distance;fleetCandidates.push(index);}
        }
        // Refill in plan order so stable equal-distance/ID ties stay unchanged.
        fleetCandidates.sort(compareCandidates);
        nearby.length=0;visibleIds.clear();
        for(let index=0;index<Math.min(fleetCandidates.length,transitRules.visibleVehicles);index++){
          const vehicle=plan.vehicles[fleetCandidates[index]];nearby.push(vehicle);visibleIds.add(vehicle.id);
        }
        lastFleetAt=now;fleetViewer.copy(viewer);
        // Only membership selection can evict a body. Other frames still
        // sample and present the retained vehicles at the current shared time.
        vehicles.forEach((body,id)=>{
          if(visibleIds.has(id))return;
          releaseVehicle(body);
          vehicles.delete(id);
          dirty=true;
        });
      }
      for(const vehicle of nearby){
        const {sample,actors}=sampleVehicle(vehicle,now,true);
        let body=vehicles.get(vehicle.id);
        if(!body){body=vehicleBody(vehicle.kind,vehicle.id);vehicles.set(vehicle.id,body);dirty=true;}
        if(body.sample?.doorsOpen!==sample?.doorsOpen||body.sample?.stopId!==sample?.stopId)dirty=true;
        const visible=!!sample&&Math.hypot(sample.point.x-viewer.x,sample.point.z-viewer.z)<480;
        if(body.root.visible!==visible)dirty=true;
        body.sample=sample;body.root.visible=visible;
        if(!sample||!body.root.visible){body.npcs.forEach(npc=>{npc.root.visible=false;});continue;}
        body.root.position.set(sample.point.x,floorY,sample.point.z);body.root.rotation.y=sample.point.yaw;
        const entry=hosted.get(body.root)!;entry.near=true;ensureFallback(entry);
        const wheelTravel=reducedMotion?0:sample.metres;
        if(entry.poseDirty||body.wheelTravel!==wheelTravel){
          entry.wheels.forEach(({node,base,radius})=>node.quaternion.copy(base).multiply(spin.setFromAxisAngle(spinAxis,wheelTravel/radius)));
          entry.fallback?.wheels?.forEach(wheel=>{wheel.rotation.x=wheelTravel/.4;});
          body.wheelTravel=wheelTravel;
        }
        if(entry.poseDirty||body.doorsOpen!==sample.doorsOpen){
          entry.doors.forEach(({node,base,direction})=>node.quaternion.copy(base).multiply(spin.setFromAxisAngle(hingeAxis,sample.doorsOpen?direction*Math.PI*.46:0)));
          entry.fallback?.doors?.forEach((door,index)=>{door.rotation.y=sample.doorsOpen?(index%2?1:-1)*Math.PI*.46:0;});
          body.doorsOpen=sample.doorsOpen;
        }
        entry.poseDirty=false;
        const npcs=actors!;
        body.npcs.forEach((npc,index)=>{
          const passenger=npcs[index];
          if(npc.root.visible!==!!passenger||passenger&&npc.seated.visible!==passenger.seated)dirty=true;
          npc.root.visible=!!passenger;
          if(passenger){
            const dx=passenger.point[0]-npc.root.position.x,dz=passenger.point[2]-npc.root.position.z;
            if(!passenger.seated&&npc.actorId===passenger.id&&dx*dx+dz*dz>.0001)npc.root.rotation.y=Math.atan2(dx,dz);
            else if(passenger.seated||npc.actorId!==passenger.id)npc.root.rotation.y=passenger.yaw;
            npc.actorId=passenger.id;npc.root.position.set(passenger.point[0],floorY+passenger.point[1],passenger.point[2]);
            npc.seated.visible=passenger.seated;npc.standing.visible=!passenger.seated;}
        });
        moving ||= !sample.stopped||npcs.some(npc=>npc.phase!=="riding");
        if(sample.phaseEndsAt>now)nextWakeAt=Math.min(nextWakeAt,sample.phaseEndsAt);
      }
      // A later vehicle must still enter the view while the visible fleet is dwelling.
      if(plan.vehicles.length)nextWakeAt=Math.min(nextWakeAt,lastFleetAt+250);
      let pending=0;
      for(const entry of hosted.values())if(entry.lease&&!entry.instance)pending++;
      for(const entry of hosted.values()) { if(pending>=2)break; if(entry.near&&!entry.lease&&!entry.failed&&provider.available(entry.key)){requestModel(entry);pending++;} }
      const result=dirty||moving;dirty=false;return result;
    },
    focus(object:THREE.Object3D,player:ScenePoint):LagosPlaceFocus|null {
      if(!snapshot)return null;
      for(let node:THREE.Object3D|null=object;node&&node!==group;node=node.parent){
        if(!node.visible)return null;
        const stop=node instanceof THREE.Group?stops.get(node):undefined,body=[...vehicles.values()].find(value=>value.root===node);
        const sample=body?.sample??(stop?[...vehicles.values()].find(value=>value.sample?.stopId===stop.id)?.sample:null);
        if(sample?.doorsOpen&&sample.approach)return{id:`transit:board:${sample.vehicleId}`,kind:"service",label:`Board ${sample.kind==="brt"?"BRT":sample.kind}`,
          point:sample.approach,reachable:Math.hypot(player[0]-sample.approach[0],player[2]-sample.approach[2])<=transitRules.boardingReach};
      }
      return null;
    },
    rider(ride:TransitRide,now:number,passengerId:string,visitRevision:number){
      if(!snapshot||ride.visitRevision!==visitRevision)return null;
      const passenger=snapshot.passengers.find(value=>value.kind==="player"&&value.id===passengerId&&value.ride.id===ride.id
        && value.ride.vehicleId===ride.vehicleId&&value.ride.seat===ride.seat&&value.ride.visitRevision===visitRevision);
      if(!passenger)return null;
      // A due exit remains attached until the server commits the matching passenger exit.
      const vehicle=fleet.get(ride.vehicleId),sample=vehicle?sampleVehicle(vehicle,now).sample:null;
      if(!sample)return null;
      const p=transitSeatPoint(sample,ride.seat),point:ScenePoint=[p[0],floorY+p[1],p[2]];
      return {point,yaw:sample.point.yaw,seated:true,seat:{id:`${ride.vehicleId}:${ride.seat}`,
        position:[point[0],floorY+transitRules.kinds[sample.kind].seatHeight,point[2]] as ScenePoint,approach:point,yaw:sample.point.yaw}};
    },
    dispose,
  };
}
