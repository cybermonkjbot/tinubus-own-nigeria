import * as THREE from "three";
import { companionCityRefinery as recipe } from "../../../../../packages/utils/src/companionCityRefinery";
import type { CompanionAssetDelivery } from "../../../../../packages/utils/src/companionAssets";
import { createCityDistrictAsset } from "./cityDistrictAsset";
import { applyCityStreetFinish } from "./cityStreetFinish";

/** Industrial Harbour is separate from existing residential parcels and authored refinery geometry. */
export function createCityRefinery(delivery: () => CompanionAssetDelivery, changed: () => void) {
  const group = new THREE.Group(); group.name = "Industrial Harbour and refinery";
  group.matrixAutoUpdate = false;
  const materials = [new THREE.MeshStandardMaterial({color:"#626c5f",roughness:.96}),
    new THREE.MeshStandardMaterial({color:"#c5c3b3",roughness:.9}),
    new THREE.MeshStandardMaterial({color:"#424947",roughness:.92})];
  applyCityStreetFinish(materials[1], "paving");
  applyCityStreetFinish(materials[2], "asphalt");
  const geometry: THREE.BufferGeometry[] = [];
  const road = (points: readonly (readonly number[])[], width: number, height: number, material: THREE.Material) => {
    const positions = new Float32Array(points.length * 6), uvs = new Float32Array(points.length * 4);
    const indexCount = Math.max(0, points.length - 1) * 6;
    const indices = points.length * 2 - 1 >= 65535 && indexCount > 0
      ? new Uint32Array(indexCount) : new Uint16Array(indexCount);
    const closed = points.length > 0 && points[0][0] === points.at(-1)![0] && points[0][1] === points.at(-1)![1];
    let distance = 0;
    points.forEach((p,i)=>{
      if(i)distance+=Math.hypot(p[0]-points[i-1][0],p[1]-points[i-1][1]);
      const a=points[i===0&&closed?points.length-2:Math.max(0,i-1)],b=points[i===points.length-1&&closed?1:Math.min(points.length-1,i+1)],length=Math.hypot(b[0]-a[0],b[1]-a[1])||1;
      const x=-(b[1]-a[1])/length*width/2,z=(b[0]-a[0])/length*width/2;
      const offset=i*6,uvOffset=i*4;
      positions[offset]=p[0]+x;positions[offset+1]=height;positions[offset+2]=p[1]+z;
      positions[offset+3]=p[0]-x;positions[offset+4]=height;positions[offset+5]=p[1]-z;
      uvs[uvOffset]=width/2;uvs[uvOffset+1]=distance;uvs[uvOffset+2]=-width/2;uvs[uvOffset+3]=distance;
      if(i){const k=(i-1)*2,index=(i-1)*6;
        indices[index]=k;indices[index+1]=k+2;indices[index+2]=k+1;
        indices[index+3]=k+1;indices[index+4]=k+2;indices[index+5]=k+3;}
    });
    const shape=new THREE.BufferGeometry();shape.setAttribute("position",new THREE.BufferAttribute(positions,3));
    shape.setAttribute("uv",new THREE.BufferAttribute(uvs,2));
    shape.setIndex(new THREE.BufferAttribute(indices,1));shape.computeVertexNormals();shape.computeBoundingSphere();geometry.push(shape);
    const mesh=new THREE.Mesh(shape,material);mesh.matrixAutoUpdate=false;mesh.receiveShadow=true;group.add(mesh);
  };
  for(const value of recipe.roads){road(value.points,value.width+value.pavement*2,-.019,materials[1]);road(value.points,value.width,-.008,materials[2]);}
  const shape=new THREE.PlaneGeometry(740,650);shape.rotateX(-Math.PI/2);geometry.push(shape);
  const ground=new THREE.Mesh(shape,materials[0]);ground.position.set(2320,-.03,1550);ground.receiveShadow=true;group.add(ground);
  ground.updateMatrix();ground.matrixAutoUpdate=false;
  const asset=createCityDistrictAsset("bestieRefinery",recipe.position,delivery,changed,undefined,1000);group.add(asset.group);
  let disposed=false;
  return{group,update:asset.update,dispose(){if(disposed)return;disposed=true;asset.dispose();group.removeFromParent();
    group.clear();geometry.forEach(value=>value.dispose());geometry.length=0;materials.forEach(value=>value.dispose());materials.length=0;}};
}
