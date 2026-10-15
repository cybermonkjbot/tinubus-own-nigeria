import * as THREE from "three";
import { companionCityPlan as plan, companionCityBankZ } from "../../../../../packages/utils/src/companionCity";
import { companionCityLandscapeTreatment } from "../../../../../packages/utils/src/companionCityLandscapeDetails";
import { companionCityParkLake } from "../../../../../packages/utils/src/companionCityParks";
import { companionCityTheatre } from "../../../../../packages/utils/src/companionCityTheatre";
import { cityStreetSeed } from "../../../../../packages/utils/src/companionCityStreetDetails";
import { companionCityNeighbourhoodSurfaces } from "../../../../../packages/utils/src/companionCityNeighbourhoods";
import { applyCityStreetFinish } from "./cityStreetFinish";

type PolygonPatch = {
  points: readonly (readonly number[])[];
  indices: Uint32Array;
  height: number;
  distances?: number[][];
  colour?: THREE.Color;
  uvYaw?: number;
};
type PolygonBatch = { patches: PolygonPatch[]; vertices: number };

/** Polygon coast, dry sand and a darker wash line share the authoritative bank samples. */
export function createCityTerrain(parkPaths = false, theatreDistrict = false) {
  const group=new THREE.Group(); group.name="Coast, beaches and garden ground";
  const geometries: THREE.BufferGeometry[]=[], materials: THREE.Material[]=[];
  const ground=new THREE.MeshStandardMaterial({color:"#818768",roughness:.98});
  const sand=new THREE.MeshStandardMaterial({color:"#d7c398",roughness:.97});
  const gardens=new THREE.MeshStandardMaterial({vertexColors:true,roughness:.98});
  const courts=new THREE.MeshStandardMaterial({vertexColors:true,roughness:.9});
  applyCityStreetFinish(courts, "paving");
  const bankMaterial=new THREE.MeshStandardMaterial({vertexColors:true,roughness:.96});
  materials.push(ground,sand,gardens,courts,bankMaterial);
  // World-scale grain avoids stretched UVs across the kilometre-wide shore.
  for(const material of [ground,sand,gardens]) {
    const beach=material===sand;
    material.onBeforeCompile=shader=>{
      shader.vertexShader="varying vec3 beachPoint;\n"+(beach?"attribute vec2 coastDistance; varying vec2 beachDistance;\n":"")+shader.vertexShader;
      shader.vertexShader=shader.vertexShader.replace("#include <begin_vertex>",
        "#include <begin_vertex>\nbeachPoint=(modelMatrix*vec4(position,1.)).xyz;"+(beach?"beachDistance=coastDistance;":""));
      shader.fragmentShader="varying vec3 beachPoint;\n"+(beach?"varying vec2 beachDistance;\n":"")+`
        float groundHash(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);}
        float groundNoise(vec2 p){
          vec2 i=floor(p),f=fract(p);f=f*f*(3.-2.*f);
          return mix(mix(groundHash(i),groundHash(i+vec2(1.,0.)),f.x),
            mix(groundHash(i+vec2(0.,1.)),groundHash(i+vec2(1.,1.)),f.x),f.y);
        }\n`+shader.fragmentShader;
      shader.fragmentShader=shader.fragmentShader.replace("#include <color_fragment>",`
        #include <color_fragment>
        vec2 grainCells=beachPoint.xz*16.;
        float grainVisible=1.-smoothstep(.35,1.,max(fwidth(grainCells).x,fwidth(grainCells).y));
        float grain=mix(.5,groundNoise(grainCells),grainVisible);
        float variation=groundNoise(beachPoint.xz*.13);
        diffuseColor.rgb*=.90+variation*.14+grain*.055;
        ${beach?`
          float damp=1.-smoothstep(.8,5.5,beachDistance.x+(groundNoise(beachPoint.xz*.35)-.5)*.7);
          float inland=1.-smoothstep(0.,4.2,beachDistance.y);
          // Linear shader colours match Three's material colour inputs.
          diffuseColor.rgb=mix(diffuseColor.rgb,vec3(.396,.319,.178)*(.94+variation*.1),damp*.9);
          diffuseColor.rgb=mix(diffuseColor.rgb,vec3(.220,.242,.138),inland*.78);
          float ripples=sin(beachPoint.z*5.+beachPoint.x*.8+variation*2.);
          diffuseColor.rgb*=1.+ripples*.014*(1.-damp);`:""}`);
      if(beach)shader.fragmentShader=shader.fragmentShader.replace("#include <roughnessmap_fragment>",
        "#include <roughnessmap_fragment>\nroughnessFactor=mix(roughnessFactor,.48,damp);");
    };
    material.customProgramCacheKey=()=>beach?"city-bank-sand-v3":"city-ground-grain-v3";
  }
  const batches=new Map<THREE.Material,PolygonBatch>(),tint=new THREE.Color();
  const polygon=(_name:string,points:readonly (readonly number[])[],height:number,material:THREE.Material,distances?:number[][],colour?:string,
    holes: readonly (readonly (readonly number[])[])[] = [], uvYaw = 0)=>{
    const contour=points.map(p=>new THREE.Vector2(p[0],-p[1]));
    const allPoints=[...points,...holes.flat()];
    let batch=batches.get(material);
    if(!batch){batch={patches:[],vertices:0};batches.set(material,batch);}
    if(colour)tint.set(colour);
    const faces=THREE.ShapeUtils.triangulateShape(contour,holes.map(h=>h.map(p=>new THREE.Vector2(p[0],-p[1]))));
    const indices=new Uint32Array(faces.length*3);
    let offset=0;
    for(const face of faces){
      const a=allPoints[face[0]],b=allPoints[face[1]],c=allPoints[face[2]];
      const reverse=(b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])>0;
      indices[offset++]=face[0];indices[offset++]=reverse?face[2]:face[1];indices[offset++]=reverse?face[1]:face[2];
    }
    // Retain source points and compact triangle order, not expanded vertex arrays.
    batch.patches.push({points:allPoints,indices,height,distances,colour:colour?tint.clone():undefined,uvYaw});
    batch.vertices+=indices.length;
  };
  polygon("South bank land",[...plan.lagoon.southBank,[10000,-10000],[-10000,-10000]],-.04,ground,undefined,undefined,
    parkPaths ? [companionCityParkLake.points] : []);
  polygon("North bank land",[...plan.lagoon.northBank,[10000,10000],[-10000,10000]],-.04,ground,undefined,undefined,
    theatreDistrict ? companionCityTheatre.lakes.map(lake => lake.points) : []);
  if (theatreDistrict) polygon("Theatre Gardens", companionCityTheatre.grounds, -.021, gardens, undefined, "#73895b");
  for(const beach of plan.beaches) {
    // Each cross-shore quad remains valid where the retained square narrows the sand to zero.
    const inland=beach.points.slice(beach.bank.length).reverse();
    for(let i=1;i<beach.bank.length;i++) {
      const a=beach.bank[i-1],b=beach.bank[i];
      const wa=Math.abs(inland[i-1][1]-a[1]),wb=Math.abs(inland[i][1]-b[1]);
      polygon(beach.name,[a,b,inland[i],inland[i-1]],-.03,sand,[[0,wa],[0,wb],[wb,0],[wa,0]]);
    }
  }
  // Cover the former flat green fill, staying below paths and the playable floor.
  for(const space of plan.greenSpaces)
    polygon(space.name,space.points,-.021,gardens,undefined,companionCityLandscapeTreatment(space.id).ground,
      parkPaths && space.id === companionCityParkLake.spaceId ? [companionCityParkLake.points] : []);
  // Shared with the map. All finishes stay below the walking floor and existing streets.
  const surfaceHeights = { compound: -.031, forecourt: -.030, garden: -.028, path: -.026, border: -.025 };
  for (const surface of companionCityNeighbourhoodSurfaces())
    polygon(surface.id, surface.points, surfaceHeights[surface.kind], surface.kind === "garden" ? gardens : courts,
      undefined, surface.colour, [], surface.yaw);
  // Sculpt only the water-side bank. Street, beach-path and bridge landings retain their levels.
  const bankCuts = [...new Set([...plan.lagoon.southBank.map(p => p[0]), ...plan.lagoon.northBank.map(p => p[0]),
    ...plan.bridges.flatMap(b => [b.x - b.width / 2 - 4, b.x + b.width / 2 + 4]), -80, 80])].sort((a,b) => a-b);
  const spanCounts = new Float64Array(bankCuts.length);
  let spansPerSide = 0;
  for (let cut=1;cut<bankCuts.length;cut++) {
    const low=bankCuts[cut-1],high=bankCuts[cut],middle=(low+high)/2;
    if (high<plan.bounds.minX || low>plan.bounds.maxX || Math.abs(middle)<80
      || plan.bridges.some(b => Math.abs(middle-b.x)<b.width/2+4)) continue;
    spanCounts[cut] = Math.ceil((high - low) / 6); spansPerSide += spanCounts[cut];
  }
  // One byte records absence or the existing rock width variant. Count output
  // before allocating it without retaining per-span coordinates or rehashing rocks.
  const rocks = new Uint8Array(spansPerSide * 2);
  let triangleCount = spansPerSide * 12, rockIndex = 0;
  for (const side of [-1,1]) for (let cut=1;cut<bankCuts.length;cut++) {
    for (let span=0;span<spanCounts[cut];span++) {
      const seed = cityStreetSeed(`bank-rock:${side}:${cut}:${span}`);
      if (seed % 4 === 0) { rocks[rockIndex] = seed % 5 + 1; triangleCount += 4; }
      rockIndex++;
    }
  }
  const bankPositions = new Float32Array(triangleCount * 9), bankColours = new Float32Array(triangleCount * 9);
  // Two four-point shelves, four rock corners and a rock tip, all at JS precision.
  const points = new Float64Array(39);
  const bankTints = ["#8c8163", "#716950", "#5e6251", "#989a85", "#7c7f70"].map(colour => new THREE.Color(colour));
  let bankOffset = 0;
  const setBankPoint = (point: number, x: number, y: number, z: number) => {
    const offset = point * 3; points[offset] = x; points[offset + 1] = y; points[offset + 2] = z;
  };
  const writeBankPoint = (point: number, colour: THREE.Color) => {
    const offset = point * 3;
    bankPositions[bankOffset] = points[offset]; bankColours[bankOffset++] = colour.r;
    bankPositions[bankOffset] = points[offset + 1]; bankColours[bankOffset++] = colour.g;
    bankPositions[bankOffset] = points[offset + 2]; bankColours[bankOffset++] = colour.b;
  };
  const bankTriangle = (a: number, b: number, c: number, colour: THREE.Color) => {
    const ai = a * 3, bi = b * 3, ci = c * 3;
    const reverse = (points[bi] - points[ai]) * (points[ci + 2] - points[ai + 2])
      - (points[bi + 2] - points[ai + 2]) * (points[ci] - points[ai]) > 0;
    writeBankPoint(a, colour); writeBankPoint(reverse ? c : b, colour); writeBankPoint(reverse ? b : c, colour);
  };
  const shelf = (point: number, x: number, z: number, side: number) => {
    const variation = cityStreetSeed(`bank:${side}:${x.toFixed(3)}`) % 101 / 100;
    setBankPoint(point, x, -.028, z);
    setBankPoint(point + 1, x, -.08 - variation * .025, z + side * .4);
    setBankPoint(point + 2, x, -.75 - variation * .2, z + side * 1.35);
    setBankPoint(point + 3, x, -1.65 - variation * .25, z + side * 3.2);
  };
  rockIndex = 0;
  for (const side of [-1,1]) {
    const bank = side === 1 ? plan.lagoon.southBank : plan.lagoon.northBank;
    for (let cut=1;cut<bankCuts.length;cut++) {
      const low=bankCuts[cut-1],high=bankCuts[cut],count=spanCounts[cut];
      for (let span=0;span<count;span++) {
        const a=low+(high-low)*span/count,b=low+(high-low)*(span+1)/count;
        const az=companionCityBankZ(bank,a),bz=companionCityBankZ(bank,b);
        // Join at the authored shore, then fall below the existing water level.
        shelf(0, a, az, side); shelf(4, b, bz, side);
        for (let row=0;row<3;row++) {
          bankTriangle(row, 4 + row, row + 1, bankTints[row]);
          bankTriangle(row + 1, 4 + row, 5 + row, bankTints[row]);
        }
        const rock = rocks[rockIndex++];
        if (!rock) continue;
        const x=(a+b)/2,z=(az+bz)/2+side*.48,w=.4+(rock-1)*.12;
        setBankPoint(8, x - w, -.16, z - .28); setBankPoint(9, x + w, -.16, z - .24);
        setBankPoint(10, x + w * .6, -.2, z + .37); setBankPoint(11, x - w * .7, -.18, z + .29);
        setBankPoint(12, x + w * .17, -.025, z);
        for(let face=0;face<4;face++)bankTriangle(8 + face, 8 + (face + 1) % 4, 12, bankTints[face % 2 ? 4 : 3]);
      }
    }
  }
  // Placement lives in the vertices; only parent/world transforms need updating.
  if (bankPositions.length) {
    const geometry=new THREE.BufferGeometry();
    geometry.setAttribute("position",new THREE.BufferAttribute(bankPositions,3));
    geometry.setAttribute("color",new THREE.BufferAttribute(bankColours,3));
    geometry.computeVertexNormals(); geometry.computeBoundingSphere();
    const mesh=new THREE.Mesh(geometry,bankMaterial);mesh.name="Sloping lagoon mud banks and stone seams";
    mesh.matrixAutoUpdate=false;mesh.receiveShadow=true;geometries.push(geometry);group.add(mesh);
  }
  for(const [material,batch] of batches) {
    const positions=new Float32Array(batch.vertices*3);
    const coast=material===sand?new Float32Array(batch.vertices*2):undefined;
    const colours=material===gardens||material===courts?new Float32Array(batch.vertices*3):undefined;
    const uvs=material===courts?new Float32Array(batch.vertices*2):undefined;
    let positionOffset=0,coastOffset=0,colourOffset=0,uvOffset=0;
    for(const patch of batch.patches)for(const i of patch.indices){
      const point=patch.points[i];
      positions[positionOffset++]=point[0];positions[positionOffset++]=patch.height;positions[positionOffset++]=point[1];
      if(uvs){
        const c=Math.cos(patch.uvYaw??0),s=Math.sin(patch.uvYaw??0);
        uvs[uvOffset++]=point[0]*c-point[1]*s;uvs[uvOffset++]=point[0]*s+point[1]*c;
      }
      if(coast&&patch.distances){
        const distance=patch.distances[i];coast[coastOffset++]=distance[0];coast[coastOffset++]=distance[1];
      }
      if(colours&&patch.colour){
        colours[colourOffset++]=patch.colour.r;colours[colourOffset++]=patch.colour.g;colours[colourOffset++]=patch.colour.b;
      }
    }
    batch.patches.length=0;
    const geometry=new THREE.BufferGeometry();
    geometry.setAttribute("position",new THREE.BufferAttribute(positions,3));
    if(coast)geometry.setAttribute("coastDistance",new THREE.BufferAttribute(coast,2));
    if(colours)geometry.setAttribute("color",new THREE.BufferAttribute(colours,3));
    if(uvs)geometry.setAttribute("uv",new THREE.BufferAttribute(uvs,2));
    geometry.computeVertexNormals();geometry.computeBoundingSphere();
    const mesh=new THREE.Mesh(geometry,material);mesh.matrixAutoUpdate=false;mesh.receiveShadow=true;
    geometries.push(geometry);group.add(mesh);
  }
  batches.clear();
  let disposed=false;
  return {group,update(_target:THREE.Vector3,_daylight:number){return false;},dispose(){
    if(disposed)return;disposed=true;group.removeFromParent();group.clear();
    geometries.forEach(g=>g.dispose());materials.forEach(m=>m.dispose());
    geometries.length=0;materials.length=0;
  }};
}
