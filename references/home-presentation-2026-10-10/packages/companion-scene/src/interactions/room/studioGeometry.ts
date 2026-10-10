import * as THREE from "three";
import { roomObstacles, roomSurfaces } from "./placement";
import type { RoomKind } from "./types";
import { registerRoomPowerBindings } from "../scenes/roomPowerBindings";
import { isPaidRoomItem } from "../../../../../packages/utils/src/companionRoomItems";
import { makeShopRoomObject } from "./shopGeometry";

function part(group: THREE.Group, geometry: THREE.BufferGeometry, color: number, x: number, y: number, z: number, roughness = .8) {
  const mesh = new THREE.Mesh(geometry, new THREE.MeshStandardMaterial({ color, roughness }));
  mesh.position.set(x, y, z); mesh.castShadow = true; mesh.receiveShadow = true; group.add(mesh); return mesh;
}
function fixPartTransforms(group: THREE.Group) {
  // These factories own fixed direct children. Keep their placement root and
  // world transforms automatic for furniture edits and venue placement.
  for (const child of group.children) if (child.matrixAutoUpdate) { child.updateMatrix(); child.matrixAutoUpdate = false; }
}
function slab(width: number, depth: number, thickness: number, radius = .08) {
  const shape = new THREE.Shape(); const x = -width / 2, y = -depth / 2;
  shape.moveTo(x + radius, y); shape.lineTo(x + width - radius, y); shape.quadraticCurveTo(x + width, y, x + width, y + radius);
  shape.lineTo(x + width, y + depth - radius); shape.quadraticCurveTo(x + width, y + depth, x + width - radius, y + depth);
  shape.lineTo(x + radius, y + depth); shape.quadraticCurveTo(x, y + depth, x, y + depth - radius);
  shape.lineTo(x, y + radius); shape.quadraticCurveTo(x, y, x + radius, y);
  const geometry = new THREE.ExtrudeGeometry(shape, { depth: thickness, bevelEnabled: false, curveSegments: 10 });
  geometry.rotateX(-Math.PI / 2); return geometry;
}
export function makeStudio() {
  const enclosure = new THREE.Group(), furniture = new THREE.Group();
  part(enclosure, new THREE.BoxGeometry(5.2, .15, 5.4), 0xc9b69b, 0, -.09, .2).material.userData.roomColorField = "floorColor";
  part(enclosure, new THREE.BoxGeometry(5.2, 3.7, .12), 0xe1d6c4, 0, 1.8, -2.05).material.userData.roomColorField = "wallColor";
  part(enclosure, new THREE.BoxGeometry(.12, 3.7, 4.6), 0xd5c8b4, -2.55, 1.8, .15).material.userData.roomColorField = "wallColor";
  part(enclosure, new THREE.BoxGeometry(5.15, .1, .05), 0xc2b398, 0, .05, -1.95);
  const floorJoint = new THREE.BoxGeometry(.009, .002, 5.25);
  for (let i = -5; i <= 5; i++) part(enclosure, floorJoint, 0xb9a88f, i * .46, -.008, .2);
  const arch = new THREE.Shape();
  arch.moveTo(-.58, 0); arch.lineTo(.58, 0); arch.lineTo(.58, 2.05); arch.absarc(0, 2.05, .58, 0, Math.PI, false); arch.lineTo(-.58, 0);
  const glass = new THREE.Mesh(new THREE.ShapeGeometry(arch, 40), new THREE.MeshBasicMaterial({ color: 0xc9d9d3 }));
  glass.position.set(.15, .35, -1.972); enclosure.add(glass);
  part(enclosure, new THREE.TorusGeometry(.6, .045, 16, 64, Math.PI), 0xf2e9d9, .15, 2.4, -1.93).castShadow = false;
  const windowSide = new THREE.BoxGeometry(.08, 2.05, .12);
  for (const x of [-.45, .75]) part(enclosure, windowSide, 0xf2e9d9, x, 1.375, -1.92);
  part(enclosure, new THREE.BoxGeometry(1.35, .08, .2), 0xf2e9d9, .15, .35, -1.91);
  part(enclosure, new THREE.BoxGeometry(.025, 2.5, .04), 0xf2e9d9, .15, 1.65, -1.92);
  part(enclosure, new THREE.BoxGeometry(1.14, .035, .04), 0xf2e9d9, .15, 1.5, -1.92);
  const print = part(enclosure, slab(.65, .95, .025, .025), 0xbe9588, 1.65, 1.95, -1.93); print.rotation.x = Math.PI / 2;
  const sunPatch = new THREE.Mesh(new THREE.PlaneGeometry(1.2, 3.2), new THREE.MeshBasicMaterial({ color: 0xffedce, transparent: true, opacity: .18, depthWrite: false }));
  sunPatch.rotation.set(-Math.PI / 2, 0, -.4); sunPatch.position.set(.6, .002, .25); enclosure.add(sunPatch);
  for (const [radius, y, color] of [[1.72, .004, 0xc3a69a], [1.66, .009, 0xcbb2a4], [1.61, .014, 0xd1baac]] as const) {
    const rug = part(enclosure, new THREE.CircleGeometry(radius, 96), color, .05, y, .55); rug.rotation.x = -Math.PI / 2; rug.scale.y = .76; rug.castShadow = false;
    rug.material.userData.roomColorField = "rugColor";
  }
  const table = roomSurfaces[1];
  part(furniture, slab(table.width, table.depth, .09), 0xb69370, table.x, table.y - .09, table.z);
  const tableLeg = new THREE.CylinderGeometry(.045, .06, .69, 20);
  for (const x of [-.68, .68]) for (const z of [-.3, .3]) part(furniture, tableLeg, 0x9e7955, table.x + x, .345, table.z + z);
  const shelf = roomSurfaces[2];
  part(furniture, slab(shelf.width, shelf.depth, .065, .04), 0xc9ac87, shelf.x, shelf.y - .065, shelf.z);
  const shelfBracket = new THREE.BoxGeometry(.055, .08, .72);
  for (const x of [-.65, .65]) part(furniture, shelfBracket, 0x917960, shelf.x + x, shelf.y - .13, -1.58);
  const chair = roomObstacles[0];
  const chairShell = new THREE.SphereGeometry(1, 40, 28);
  const base = part(furniture, chairShell, 0xd5c6ae, chair.x, .35, chair.z); base.scale.set(.48, .35, .57);
  const back = part(furniture, chairShell, 0xd8ccb8, chair.x, .71, chair.z - .42); back.scale.set(.47, .46, .17); back.rotation.x = -.12;
  const cushion = part(furniture, new THREE.SphereGeometry(1, 32, 24), 0xb98e83, chair.x + .03, .61, chair.z - .22); cushion.scale.set(.28, .24, .13); cushion.rotation.z = -.12;
  for (const mesh of [base, back, cushion]) mesh.material.userData.roomColorField = "upholsteryColor";
  fixPartTransforms(enclosure); fixPartTransforms(furniture);
  return { enclosure, furniture };
}
export function makeRoomObject(kind: RoomKind, lit = true) {
  if (isPaidRoomItem(kind)) return makeShopRoomObject(kind, lit);
  const group = new THREE.Group();
  const mesh = (geometry: THREE.BufferGeometry, color: number, x: number, y: number, z: number, roughness = .75) => part(group, geometry, color, x, y, z, roughness);
  if (kind === "lamp") {
    mesh(new THREE.CylinderGeometry(.23, .27, .08, 32), 0x85634d, 0, .04, 0);
    mesh(new THREE.CylinderGeometry(.045, .05, .56, 20), 0x8b694d, 0, .33, 0, .35);
    mesh(new THREE.CylinderGeometry(.18, .35, .32, 48, 1, true), 0xf2dfbb, 0, .75, 0).userData.lampShade = true;
    mesh(new THREE.SphereGeometry(.08, 20, 16), 0xffefc0, 0, .68, 0);
    const light = new THREE.PointLight(0xffd39b, 1.8, 2.1); light.position.y = .65; light.visible = lit; group.add(light);
    registerRoomPowerBindings(group, [{ kind: "light", target: light }]);
  } else if (kind === "stool") {
    mesh(new THREE.CylinderGeometry(.36, .34, .3, 48), 0xd7bcb1, 0, .44, 0);
    const leg = new THREE.CylinderGeometry(.055, .045, .32, 16);
    for (const [x, z] of [[-.2, -.2], [.2, -.2], [-.2, .2], [.2, .2]]) mesh(leg, 0x70543e, x, .16, z);
  } else {
    mesh(new THREE.CylinderGeometry(.25, .19, .36, 32), 0xb79073, 0, .18, 0);
    mesh(new THREE.CylinderGeometry(.225, .225, .018, 32), 0x514333, 0, .37, 0);
    mesh(new THREE.CylinderGeometry(.025, .035, .72, 12), 0x5a6546, 0, .71, 0);
    const leafGeometry = new THREE.SphereGeometry(1, 24, 16);
    for (let i = 0; i < 7; i++) {
      const angle = i * 2.4;
      const leaf = mesh(leafGeometry, i % 2 ? 0x81916b : 0x647d56, Math.sin(angle) * .18, .58 + i * .09, Math.cos(angle) * .18);
      leaf.scale.set(.16, .035, .26); leaf.rotation.set(.25, angle, i % 2 ? .25 : -.25);
    }
  }
  fixPartTransforms(group);
  return group;
}
