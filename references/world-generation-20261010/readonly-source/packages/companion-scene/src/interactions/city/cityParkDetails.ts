import * as THREE from "three";
import { companionCityParkBridges, companionCityParkLake } from "../../../../../packages/utils/src/companionCityParks";

/** Flush decks match floorY zero; only the side rails enter player/camera collision. */
export function createCityParkDetails() {
  const group=new THREE.Group();group.name="Botanic lake footbridges and banks";
  group.matrixAutoUpdate=false;
  const geometry=new THREE.BoxGeometry(1,1,1);
  const timber=new THREE.MeshStandardMaterial({color:"#8b795b",roughness:.87});
  const metal=new THREE.MeshStandardMaterial({color:"#566254",roughness:.67,metalness:.25});
  const stone=new THREE.MeshStandardMaterial({color:"#8a8b71",roughness:.98});
  const batches=new Map<THREE.Material,{x:number;y:number;z:number;w:number;h:number;d:number;yaw:number}[]>();
  const add=(x:number,y:number,z:number,w:number,h:number,d:number,material:THREE.Material,yaw=0)=>{
    const values=batches.get(material)??[];values.push({x,y,z,w,h,d,yaw});batches.set(material,values);
  };
  for(const b of companionCityParkBridges) {
    // No ramps or steps on the flat navigation floor. Deck surface meets the approach path.
    add((b.x1+b.x2)/2,-.06,b.z,b.x2-b.x1,.12,b.width,timber);
    for(let x=b.x1+.2;x<b.x2-.1;x+=.65) add(x,.001,b.z,.018,.002,b.width-.22,metal);
    for(const side of [-1,1]) {
      const z=b.z+side*(b.width/2-.06);
      add((b.x1+b.x2)/2,1.06,z,b.x2-b.x1,.08,.12,timber);
      add((b.x1+b.x2)/2,.49,z,b.x2-b.x1,.055,.09,metal);
      for(let x=b.x1+.06;x<=b.x2-.06;x+=2.4) {
        add(x,.55,z,.12,1.1,.12,metal);
        add(x,-.18,z,.15,.36,.15,metal);
      }
    }
  }
  // The retaining edge meets dry ground at -.021 and the recessed water at -.24.
  const points=companionCityParkLake.points;
  for(let i=0;i<points.length;i++) {
    const a=points[i],b=points[(i+1)%points.length],length=Math.hypot(b[0]-a[0],b[1]-a[1]);
    add((a[0]+b[0])/2,-.15,(a[1]+b[1])/2,length,.258,.1,stone,-Math.atan2(b[1]-a[1],b[0]-a[0]));
  }
  const matrix=new THREE.Matrix4(),position=new THREE.Vector3(),scale=new THREE.Vector3();
  const rotation=new THREE.Quaternion(),up=new THREE.Vector3(0,1,0);
  for(const [material,values] of batches) {
    const mesh=new THREE.InstancedMesh(geometry,material,values.length);
    mesh.matrixAutoUpdate=false;
    values.forEach((v,i)=>{
      rotation.setFromAxisAngle(up,v.yaw);matrix.compose(position.set(v.x,v.y,v.z),rotation,scale.set(v.w,v.h,v.d));
      mesh.setMatrixAt(i,matrix);
    });
    mesh.instanceMatrix.needsUpdate=true;mesh.computeBoundingSphere();mesh.receiveShadow=true;group.add(mesh);
  }
  batches.clear();
  let disposed=false,shadows=false;group.visible=false;
  let viewX=NaN,viewZ=NaN,distance=Infinity;
  return {group,update(target:THREE.Vector3,daylight:number){
    if(disposed||!Number.isFinite(target.x)||!Number.isFinite(target.z))return false;
    if(target.x!==viewX||target.z!==viewZ){viewX=target.x;viewZ=target.z;distance=Math.hypot(viewX-870,viewZ+555);}
    const visible=distance<(group.visible?360:280);
    const cast=visible&&daylight>.15&&distance<(shadows?180:140);
    if(visible===group.visible&&cast===shadows)return false;
    group.visible=visible;shadows=cast;
    group.traverse(node=>{if(node instanceof THREE.InstancedMesh)node.castShadow=cast&&node.material!==stone;});return true;
  },dispose(){if(disposed)return;disposed=true;group.traverse(node=>{if(node instanceof THREE.InstancedMesh)node.dispose();});group.removeFromParent();group.clear();
    geometry.dispose();timber.dispose();metal.dispose();stone.dispose();}};
}
