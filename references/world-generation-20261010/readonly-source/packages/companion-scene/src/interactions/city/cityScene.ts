import * as THREE from "three";
import { createCircuitGeometry } from "../racing/circuitGeometry";
import { racingRules } from "../../../../../packages/utils/src/companionRacing";
import { companionCityPlan as plan, companionCityParcelPoint, companionCityBankZ, companionCityStreetDensity, type CompanionCityParcel } from "../../../../../packages/utils/src/companionCity";
import { createCityMiniParks } from "./cityMiniParks";
import { createCityWater } from "./cityWater";
import { createCityStreets } from "./cityStreets";
import { createCityTerrain } from "./cityTerrain";
import { createCityArchitectureDetails } from "./cityArchitectureDetails";
import { createCityLandscapeDetails } from "./cityLandscapeDetails";
import { createCityStreetDetails } from "./cityStreetDetails";
import { createCivicTerrace } from "./civicTerrace";
import { createDanceAgainExterior } from "./danceAgainExterior";
import { createSignatureBuildings, signatureBuildingParcelIds } from "./signatureBuildings";
import { createCityBillboards } from "./cityBillboards";
import { createCityRoadTreatments } from "./cityRoadTreatments";
import { createCityParkDetails } from "./cityParkDetails";
import { createCityParkWalkers } from "./cityParkWalkers";
import { createCityFrontages } from "./cityFrontages";
import { createCityWaterfront } from "./cityWaterfront";
import { createCityTheatre } from "./cityTheatre";
import { createCityRefinery } from "./cityRefinery";
import { companionCityTheatre } from "../../../../../packages/utils/src/companionCityTheatre";
import { companionCityRefinery } from "../../../../../packages/utils/src/companionCityRefinery";
import { companionCityParkWalkingRoads } from "../../../../../packages/utils/src/companionCityParks";
import type { ScenePowerBinding } from "../scenes/scenePower";
import type { CompanionAssetDelivery } from "../../../../../packages/utils/src/companionAssets";
import { publicCompanionAssets } from "../appearance/CompanionAssets";
import type { CompanionGunfireState } from "../../../../../packages/utils/src/companionGunfire";
import type { RoomNavigation } from "../../../../../packages/utils/src/companionNavigation";

type Box = { x: number; y: number; z: number; w: number; h: number; d: number; yaw?: number; colour?: string; parcel?: string };
type Tile = { key: string; parcels: CompanionCityParcel[]; shells: THREE.Group };
const palettes: Record<string, string[]> = {
  "town-centre": ["#d9cdb7","#e8dfce","#b5b6ad","#c5aa94","#cbc8ba","#eee5d8"],
  "palm-courts": ["#d4c28f","#dfd2ae","#a6b69b","#cfad97","#ddd6bf","#b8bdac"],
  "garden-quarter": ["#e5e1d1","#bbc5ac","#cfc0ac","#e0d1b6","#c2cbbb","#d5bfa9"],
  "south-market": ["#b98762","#c6ab80","#d4c198","#ad9d85","#c5b29b","#cba480"],
  marina: ["#c3c4b5","#cfd6cf","#aab9b7","#cebaa6","#e2ddd0","#a9b9bf"],
  skyline: ["#b8cacf","#d5d9d0","#9aaeb2","#dedbd0","#b4c0b8","#b9b9ad"],
  "lagoon-gardens": ["#efe9db","#d9d2be","#e9e2d1","#cfcec0","#e0d8c8","#c6d0c5"],
};

export function createCityScene(detailCollision = false, delivery: () => CompanionAssetDelivery = () => publicCompanionAssets,
  changed: () => void = () => {}, roadTreatments = false, parkPaths = false, theatreDistrict = false, connectedDistricts = false, streetDensity = false) {
  const group = new THREE.Group(); group.name = "Lagos district city";
  const circuit = createCircuitGeometry(); circuit.group.position.set(racingRules.mapX, 0, racingRules.mapZ); group.add(circuit.group);
  const civicTerrace = createCivicTerrace(); group.add(civicTerrace.group);
  const cube = new THREE.BoxGeometry(1,1,1);
  const gable = new THREE.BufferGeometry();
  const roofPoints = [[-.5,-.5,-.5],[.5,-.5,-.5],[-.5,-.5,.5],[.5,-.5,.5],[-.5,.5,0],[.5,.5,0]];
  gable.setAttribute("position",new THREE.Float32BufferAttribute([0,4,5,0,5,1,2,3,5,2,5,4,0,2,4,1,5,3].flatMap(i=>roofPoints[i]),3));
  gable.computeVertexNormals();
  const night = { value: 0 };
  const mains = { value: 1 };
  const density = streetDensity ? companionCityStreetDensity() : undefined;
  const miniParks = density ? createCityMiniParks(density.parks, delivery, changed) : undefined;
  if (miniParks) group.add(miniParks.group);
  const danceAgain = createDanceAgainExterior(mains, delivery, changed); group.add(danceAgain.group);
  const signatureBuildings = createSignatureBuildings(mains, delivery, changed); group.add(signatureBuildings.group);
  const billboards = createCityBillboards(mains, delivery, changed); group.add(billboards.group);
  const planting = roadTreatments ? createCityRoadTreatments(mains, delivery, changed) : undefined;
  if (planting) group.add(planting.group);
  const frontages = createCityFrontages(delivery, changed); group.add(frontages.group);
  const waterfront = createCityWaterfront(delivery, changed); group.add(waterfront.group);
  const theatre = theatreDistrict ? createCityTheatre(delivery, changed, connectedDistricts) : undefined;
  if (theatre) group.add(theatre.group);
  const refinery = connectedDistricts ? createCityRefinery(delivery, changed) : undefined;
  if (refinery) group.add(refinery.group);
  const parkDetails=parkPaths?createCityParkDetails():undefined;
  const parkWalkers=parkPaths?createCityParkWalkers(delivery,changed):undefined;
  if(parkDetails)group.add(parkDetails.group);
  if(parkWalkers)group.add(parkWalkers.group);
  const powerBindings: readonly ScenePowerBinding[] = [{ kind: "uniform", target: mains }];
  const facade = new THREE.MeshStandardMaterial({ color:"#ffffff",roughness:.82 });
  facade.onBeforeCompile = shader => {
    shader.uniforms.cityNight = night;
    shader.uniforms.cityMains = mains;
    shader.vertexShader = `varying vec3 cityPoint; varying vec3 cityNormal;\n${shader.vertexShader}`
      .replace("#include <begin_vertex>",`#include <begin_vertex>
        vec4 cityVertex = vec4(position,1.);
        vec3 cityScale = vec3(1.);
        #ifdef USE_INSTANCING
          cityVertex = instanceMatrix * cityVertex;
          cityScale = vec3(length(instanceMatrix[0].xyz),length(instanceMatrix[1].xyz),length(instanceMatrix[2].xyz));
        #endif
        cityPoint = vec3(position.x*cityScale.x,(modelMatrix*cityVertex).y,position.z*cityScale.z);
        cityNormal = normal;`);
    shader.fragmentShader = `uniform float cityNight; uniform float cityMains; varying vec3 cityPoint; varying vec3 cityNormal;\n${shader.fragmentShader}`
      .replace("#include <color_fragment>",`#include <color_fragment>
        float facadeAxis = abs(cityNormal.x)>.5 ? cityPoint.z : cityPoint.x;
        vec2 bay = fract(vec2(facadeAxis/4.2,cityPoint.y/3.3));
        float cityWindow=step(.17,bay.x)*step(bay.x,.8)*step(.28,bay.y)*step(bay.y,.81)
          *(1.-step(.5,abs(cityNormal.y)))*step(1.4,cityPoint.y);
        float occupied=step(.39,fract(sin(dot(floor(vec2(facadeAxis/4.2,cityPoint.y/3.3)),vec2(12.9898,78.233)))*43758.5453));
        diffuseColor.rgb=mix(diffuseColor.rgb,vec3(.12,.20,.23),cityWindow);`)
      .replace("#include <roughnessmap_fragment>","#include <roughnessmap_fragment>\nroughnessFactor=mix(roughnessFactor,.24,cityWindow);")
      .replace("#include <emissivemap_fragment>","#include <emissivemap_fragment>\ntotalEmissiveRadiance+=vec3(.9,.55,.22)*cityWindow*occupied*cityNight*cityMains*.45;");
  };
  facade.customProgramCacheKey=()=>"city-oriented-facade-v3";
  const stone=new THREE.MeshStandardMaterial({color:"#ffffff",roughness:.9});
  const metal=new THREE.MeshStandardMaterial({color:"#ffffff",roughness:.48,metalness:.35});
  const matrix=new THREE.Matrix4(),scale=new THREE.Vector3(),position=new THREE.Vector3();
  const rotation=new THREE.Quaternion(),up=new THREE.Vector3(0,1,0);
  const colours=new Map<string,THREE.Color>();
  const batch=(parent:THREE.Group,values:Box[],material:THREE.Material,geometry:THREE.BufferGeometry=cube,shadow=false)=>{
    if(!values.length)return;
    const mesh=new THREE.InstancedMesh(geometry,material,values.length);
    try {
      // Placement lives in the instance buffer; the batch's local matrix stays identity.
      mesh.matrixAutoUpdate=false;
      const parcelIds:(string|undefined)[]=new Array(values.length);
      mesh.userData.cityParcelIds=parcelIds;
      values.forEach((b,i)=>{
        rotation.setFromAxisAngle(up,b.yaw??0);
        matrix.compose(position.set(b.x,b.y,b.z),rotation,scale.set(b.w,b.h,b.d));
        const tint=b.colour??"#ffffff";
        let colour=colours.get(tint);
        if(!colour){colour=new THREE.Color(tint);colours.set(tint,colour);}
        // setColorAt copies the converted channels into this mesh's own buffer.
        mesh.setMatrixAt(i,matrix);mesh.setColorAt(i,colour);parcelIds[i]=b.parcel;
      });
      mesh.instanceMatrix.needsUpdate=true;
      if(mesh.instanceColor)mesh.instanceColor.needsUpdate=true;
      mesh.computeBoundingSphere();mesh.castShadow=shadow;mesh.receiveShadow=true;parent.add(mesh);
    } catch(error) {mesh.removeFromParent();mesh.dispose();throw error;}
  };
  const part=(p:CompanionCityParcel,x:number,y:number,z:number,w:number,h:number,d:number,tint?:string):Box=>{
    const at=companionCityParcelPoint(p,x,z);
    return{x:at[0],y,z:at[2],w,h,d,yaw:p.yaw,colour:tint,parcel:p.id};
  };
  const tiles=new Map<string,Tile>();
  for(const p of plan.parcels){
    const x=Math.floor(p.x/plan.cellSize),z=Math.floor(p.z/plan.cellSize),key=x+":"+z;
    if(!tiles.has(key))tiles.set(key,{key,parcels:[],shells:new THREE.Group()});
    tiles.get(key)!.parcels.push(p);
  }
  // Tile boundaries are only for culling. They never determine a road or plot.
  for(const tile of tiles.values()){
    const shells:Box[]=[],roofs:Box[]=[],pitches:Box[]=[];
    for(const p of tile.parcels){
      if(p.id===civicTerrace.parcelId||p.id===danceAgain.parcelId||signatureBuildingParcelIds.has(p.id))continue;
      const h=p.floors*3.3,w=p.width,d=p.depth,tint=palettes[p.district][p.variant];
      const mass=(x:number,z:number,mw:number,md:number,mh:number,base=0)=>{
        shells.push(part(p,x,base+mh/2,z,mw,mh,md,tint));
        roofs.push(part(p,x,base+mh+.16,z,mw+.4,.32,md+.4,p.district==="lagoon-gardens"?"#b5ac97":"#828c84"));
      };
      if(p.style==="stepped-tower"){
        mass(0,0,w,d,9.9);mass((p.variant%2?1:-1)*w*.08,-d*.06,w*.68,d*.72,h-9.9,9.9);
        mass(w*.05,-d*.09,w*.4,d*.43,6.6,h);
      }else if(p.style==="courtyard"){
        mass(-w*.35,0,w*.3,d,h);mass(w*.35,0,w*.3,d,h);
        mass(0,-d*.34,w*.4,d*.32,Math.max(6.6,h-3.3));
      }else if(p.style==="terrace-villa"||p.style==="courtyard-villa"){
        mass(-w*.18,0,w*.64,d,h);mass(w*.32,-d*.2,w*.36,d*.6,h-3.3);
        roofs.push(part(p,w*.32,h-3.1,-d*.2,w*.36,.18,d*.6,"#d7c7a7"));
      }else if(p.style==="urban-apartment"||p.style==="waterfront"){
        mass(-w*.08,0,w*.84,d,h);mass(w*.4,-d*.12,w*.2,d*.76,Math.max(6.6,h-3.3));
      }else{
        mass(0,0,w,d,h);
        if(["townhouse","garden-home","warehouse","workshop"].includes(p.style))
          pitches.push(part(p,0,h+1.65,0,w+1,3,d+1,p.style==="warehouse"?"#697b7c":p.variant%2?"#8b5e45":"#777669"));
      }
    }
    tile.shells.name=`Distant city buildings ${tile.key}`;tile.shells.matrixAutoUpdate=false;group.add(tile.shells);
    batch(tile.shells,shells,facade,cube,true);batch(tile.shells,roofs,stone,cube,true);batch(tile.shells,pitches,metal,gable,true);
    tile.parcels.length=0;
  }
  const terrain=createCityTerrain(parkPaths,theatreDistrict);group.add(terrain.group);
  const streets=createCityStreets(roadTreatments,parkPaths);group.add(streets.group);
  const extensionRoads = [...(theatreDistrict ? [...companionCityTheatre.roads, ...companionCityTheatre.paths] : []),
    ...(connectedDistricts ? companionCityRefinery.roads : [])];
  const streetDetails=createCityStreetDetails(detailCollision,mains,roadTreatments,density?.lights,extensionRoads,
    parkPaths ? companionCityParkWalkingRoads : plan.roads);group.add(streetDetails.group);
  const bridges:Box[]=[];
  for(const b of plan.bridges)for(const side of [-1,1]){
    const south=companionCityBankZ(plan.lagoon.southBank,b.x),north=companionCityBankZ(plan.lagoon.northBank,b.x);
    bridges.push({x:b.x+side*(b.width/2+3.8),y:.62,z:(south+north)/2,w:.22,h:1.2,d:north-south,colour:"#a9a99e"});
    for(let z=b.minZ+45;z<b.maxZ-20;z+=65)bridges.push({x:b.x+side*9,y:-3,z,w:2,h:6,d:3,colour:"#777f78"});
  }
  batch(group,bridges,stone,cube,true);
  colours.clear();
  const water=createCityWater(parkPaths,theatreDistrict);group.add(water.group);

  const architecture=createCityArchitectureDetails(mains);group.add(architecture.group);
  const nearCells=architecture.group.userData.cityArchitectureNearCellKeys as ReadonlySet<string>;

  const landscape=createCityLandscapeDetails(detailCollision,parkPaths);
  group.add(landscape.group);
  let disposed=false;
  return {group,water,powerBindings,
    setBillboardAd: billboards.setAd, billboardVisibility: billboards.visibility,
    pickedBillboardAd: billboards.pickedAd,
    get billboardWakeAfter() { return billboards.wakeAfter; },
    update(target:THREE.Vector3,daylight:number,seconds:number|null=null,reducedMotion=false,
      gunfire?: CompanionGunfireState, navigation?: RoomNavigation, serverNow = 0){
      if(disposed)return false;
      night.value=1-THREE.MathUtils.clamp(daylight,0,1);
      const architectureChanged=architecture.update(target,daylight);
      if(architectureChanged)for(const tile of tiles.values())tile.shells.visible=!nearCells.has(tile.key);
      const landscapeChanged=landscape.update(target,daylight);
      const streetsChanged=streetDetails.update(target,daylight);
      const terraceChanged=civicTerrace.update(target,daylight);
      const loungeChanged=danceAgain.update(target,daylight);
      const signatureChanged=signatureBuildings.update(target,daylight);
      const billboardsChanged=billboards.update(target,daylight);
      const plantingChanged=planting?.update(target,daylight)??false;
      const frontagesChanged=frontages.update(target,daylight);
      const waterfrontChanged=waterfront.update(target,daylight,seconds??0,reducedMotion);
      const parkChanged=parkDetails?.update(target,daylight)??false;
      const walkersChanged=parkWalkers?.update(target,seconds,reducedMotion,gunfire,navigation,serverNow)??false;
      const theatreChanged=theatre?.update(target,daylight)??false;
      const refineryChanged=refinery?.update(target,daylight)??false;
      const miniParksChanged=miniParks?.update(target,daylight)??false;
      return architectureChanged||landscapeChanged||streetsChanged||terraceChanged||loungeChanged||signatureChanged||billboardsChanged||plantingChanged||frontagesChanged||waterfrontChanged||parkChanged||walkersChanged||theatreChanged||refineryChanged||miniParksChanged;
    },
    parcel(object:THREE.Object3D,instanceId?:number){
      const hosted = signatureBuildings.parcel(object) ?? danceAgain.parcel(object) ?? billboards.parcel(object);
      if (hosted) return hosted;
      const ids=object.userData.cityParcelIds as (string|undefined)[]|undefined;
      return instanceId===undefined?undefined:ids?.[instanceId];
    },
    dispose(){
      if(disposed)return;disposed=true;
      architecture.dispose();landscape.dispose();streetDetails.dispose();civicTerrace.dispose();danceAgain.dispose();signatureBuildings.dispose();billboards.dispose();planting?.dispose();
      parkWalkers?.dispose();parkDetails?.dispose();
      frontages.dispose();
      waterfront.dispose();
      circuit.dispose();
      theatre?.dispose();
      refinery?.dispose();
      miniParks?.dispose();
      group.traverse(o=>{if(o instanceof THREE.InstancedMesh)o.dispose();});
      group.removeFromParent();water.dispose();streets.dispose();terrain.dispose();
      [cube,gable].forEach(g=>g.dispose());
      [facade,stone,metal].forEach(m=>m.dispose());
      // Disposed meshes still own their CPU instance buffers. Release the
      // remaining shell/bridge graph and tile references as well as the GPU data.
      for(const tile of tiles.values())tile.shells.clear();
      tiles.clear();group.clear();
    },
  };
}
