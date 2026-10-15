import * as THREE from "three";
import { roomItemDefinitions, type PaidRoomItemId } from "../../../../../packages/utils/src/companionRoomItems";
import { registerRoomPowerBindings } from "../scenes/roomPowerBindings";

/** Small original meshes. No new remote model or image download is needed. */
export function makeShopRoomObject(kind: PaidRoomItemId, lit: boolean) {
  const group = new THREE.Group();
  group.name = roomItemDefinitions[kind].name;
  const cream = 0xeee4d0, wood = 0x8b654b, brass = 0xb59661, sage = 0x8b9c80, dark = 0x41453f;
  const mesh = (geometry: THREE.BufferGeometry, color: number, x: number, y: number, z: number, metalness = 0) => {
    const part = new THREE.Mesh(geometry, new THREE.MeshStandardMaterial({ color, roughness: metalness ? .24 : .75, metalness }));
    part.position.set(x, y, z); part.castShadow = true; part.receiveShadow = true; group.add(part); return part;
  };
  const box = (w: number, h: number, d: number, color: number, x: number, y: number, z: number) => mesh(new THREE.BoxGeometry(w, h, d), color, x, y, z);
  const cylinder = (r: number, h: number, color: number, x: number, y: number, z: number, bottom = r) => mesh(new THREE.CylinderGeometry(r, bottom, h, 32), color, x, y, z);
  const oval = (w: number, h: number, d: number, color: number, x: number, y: number, z: number) => {
    const part = mesh(new THREE.SphereGeometry(1, 24, 16), color, x, y, z); part.scale.set(w / 2, h / 2, d / 2); return part;
  };
  const rod = (from: THREE.Vector3, to: THREE.Vector3, radius: number, color: number) => {
    const part = cylinder(radius, from.distanceTo(to), color, 0, 0, 0);
    part.position.copy(from).add(to).multiplyScalar(.5);
    part.quaternion.setFromUnitVectors(new THREE.Vector3(0, 1, 0), to.clone().sub(from).normalize());
    return part;
  };
  const glow = (y: number, z = 0) => {
    const bulb = mesh(new THREE.SphereGeometry(.07, 16, 12), cream, 0, y, z);
    bulb.material.emissive.set(0xffd39b); bulb.material.emissiveIntensity = lit ? .6 : 0;
    const light = new THREE.PointLight(0xffd39b, 1.1, 2.4); light.position.set(0, y, z); light.visible = lit; group.add(light);
    registerRoomPowerBindings(group, [{ kind: "light", target: light }, { kind: "emissive", target: bulb.material }]);
  };
  if (kind.endsWith("-painting")) {
    const definition = roomItemDefinitions[kind], w = definition.wallWidth!, h = definition.height;
    box(w, h, .09, wood, 0, h / 2, .045);
    box(w - .12, h - .12, .014, cream, 0, h / 2, .083);
    if (kind === "botanical-painting") {
      box(.018, h * .66, .008, dark, 0, h * .5, .094);
      for (let i = 0; i < 6; i++) {
        const side = i % 2 ? 1 : -1;
        const leaf = oval(w * .33, h * .1, .006, i % 2 ? sage : 0x6b805e, side * w * .14, h * (.25 + i * .095), .095);
        leaf.rotation.z = side * .45;
      }
    } else if (kind === "abstract-painting") {
      oval(w * .45, h * .67, .008, 0xbc8069, -w * .16, h * .53, .095);
      box(w * .28, h * .5, .008, sage, w * .22, h * .4, .098);
      oval(w * .23, h * .29, .008, 0xd8b58b, w * .2, h * .72, .1);
    } else {
      box(w - .15, h * .35, .008, 0xd9b18b, 0, h * .735, .094);
      oval(w * .6, h * .4, .008, 0xa2ae8c, -w * .15, h * .4, .1);
      oval(w * .55, h * .27, .008, 0x788867, w * .18, h * .26, .105);
      oval(h * .19, h * .19, .008, cream, w * .22, h * .72, .1);
    }
  } else if (kind === "wall-clock") {
    const rim = cylinder(.48, .1, wood, 0, .48, .05); rim.rotation.x = Math.PI / 2;
    const face = cylinder(.425, .012, cream, 0, .48, .101); face.rotation.x = Math.PI / 2;
    for (let i = 0; i < 12; i++) {
      const angle = i * Math.PI / 6;
      const tick = box(.018, .045, .005, dark, Math.sin(angle) * .35, .48 + Math.cos(angle) * .35, .11); tick.rotation.z = -angle;
    }
    rod(new THREE.Vector3(0, .48, .115), new THREE.Vector3(-.17, .59, .115), .016, dark);
    rod(new THREE.Vector3(0, .48, .12), new THREE.Vector3(.12, .73, .12), .012, dark);
  } else if (kind === "wall-mirror") {
    const rim = mesh(new THREE.TorusGeometry(.47, .04, 12, 48), brass, 0, .775, .04, .8); rim.scale.y = 1.5;
    const glass = mesh(new THREE.CircleGeometry(.445, 48), 0xb4c4c5, 0, .775, .045, .92); glass.scale.y = 1.5;
  } else if (kind === "standing-fan" || kind === "desk-fan") {
    const small = kind === "desk-fan", radius = small ? .33 : .52, centre = small ? .57 : 2.12;
    const casing = small ? cream : sage;
    cylinder(small ? .26 : .42, .1, casing, 0, .05, 0);
    cylinder(small ? .045 : .055, centre - .12, brass, 0, centre / 2, -.06);
    const motor = cylinder(radius * .3, .22, casing, 0, centre, -.09); motor.rotation.x = Math.PI / 2;
    for (let i = 0; i < 3; i++) {
      const angle = i * Math.PI * 2 / 3;
      const blade = oval(radius * .48, radius * 1.06, .035, casing, Math.sin(angle) * radius * .34, centre + Math.cos(angle) * radius * .34, .055);
      blade.rotation.z = -angle + .3;
    }
    for (const fraction of [.3, .6, 1]) mesh(new THREE.TorusGeometry(radius * fraction, .012, 8, 40), dark, 0, centre, .1, .5);
    for (let i = 0; i < 12; i++) {
      const angle = i * Math.PI / 6;
      rod(new THREE.Vector3(0, centre, .11), new THREE.Vector3(Math.sin(angle) * radius, centre + Math.cos(angle) * radius, .11), .009, dark);
    }
    const cap = cylinder(radius * .13, .04, brass, 0, centre, .13); cap.rotation.x = Math.PI / 2;
  } else if (kind === "tower-fan") {
    cylinder(.33, .1, dark, 0, .05, 0);
    cylinder(.22, 2.16, cream, 0, 1.18, 0);
    for (let i = 0; i < 7; i++) box(.016, 1.72, .03, dark, (i - 3) * .04, 1.18, .204);
    cylinder(.06, .015, dark, 0, 2.268, 0);
  } else if (kind === "floor-lamp" || kind === "desk-lamp") {
    const floor = kind === "floor-lamp", height = floor ? 3.05 : .95;
    cylinder(floor ? .37 : .22, .08, brass, 0, .04, 0);
    rod(new THREE.Vector3(0, .08, 0), new THREE.Vector3(0, height - .32, floor ? 0 : -.12), .035, brass);
    mesh(new THREE.CylinderGeometry(floor ? .27 : .16, floor ? .5 : .28, floor ? .58 : .3, 32, 1, true), floor ? cream : sage, 0, height - (floor ? .29 : .15), 0);
    glow(height - .3);
  } else if (kind === "armchair" || kind === "loveseat") {
    const w = kind === "armchair" ? 1.55 : 2.75, color = kind === "armchair" ? cream : 0xcfa8a1;
    for (const x of [-w / 2 + .15, w / 2 - .15]) for (const z of [-.48, .48]) cylinder(.055, .25, wood, x, .125, z);
    box(w, .36, 1.28, color, 0, .43, 0);
    oval(w, 1.26, .35, color, 0, 1.1, -.51);
    for (const x of [-w / 2 + .12, w / 2 - .12]) oval(.3, .83, 1.4, color, x, .83, 0);
    const cushions = kind === "armchair" ? 1 : 2;
    for (let i = 0; i < cushions; i++) oval((w - .55) / cushions, .24, 1.04, color, (i - (cushions - 1) / 2) * (w - .55) / cushions, .72, .08);
  } else if (kind === "ottoman") {
    cylinder(.62, .62, sage, 0, .36, 0);
    oval(1.34, .25, 1.34, 0x9aa98d, 0, .655, 0);
  } else if (kind === "coffee-table" || kind === "side-table") {
    const small = kind === "side-table", height = small ? 1.05 : .82;
    const top = cylinder(small ? .56 : 1.05, .12, wood, 0, height - .06, 0); if (!small) top.scale.z = .65;
    if (small) { cylinder(.11, .9, wood, 0, .45, 0); cylinder(.36, .08, wood, 0, .04, 0); }
    else for (const x of [-.64, .64]) for (const z of [-.38, .38]) cylinder(.055, .7, wood, x, .35, z);
  } else if (kind === "bookcase") {
    for (const x of [-.69, .69]) box(.1, 2.7, .72, wood, x, 1.35, 0);
    box(1.4, 2.7, .055, wood, 0, 1.35, -.34);
    for (const y of [.08, .73, 1.38, 2.03, 2.65]) box(1.38, .08, .72, wood, 0, y, 0);
    for (let shelf = 0; shelf < 4; shelf++) for (let i = 0; i < 4; i++) box(.11, .34 + i % 2 * .09, .38, [sage, cream, 0xbc8d7d][i % 3], -.44 + i * .14, .3 + shelf * .65, -.06);
  } else if (kind === "storage-basket") {
    cylinder(.46, .63, 0xb89568, 0, .34, 0, .36);
    cylinder(.42, .012, 0x776046, 0, .661, 0);
    for (let i = 0; i < 8; i++) {
      const band = mesh(new THREE.TorusGeometry(.365 + i * .012, .013, 6, 32), 0xd8bc8b, 0, .08 + i * .078, 0); band.rotation.x = Math.PI / 2;
    }
    for (const x of [-.36, .36]) mesh(new THREE.TorusGeometry(.1, .022, 8, 20), 0xc5a775, x, .72, 0);
  } else if (kind === "book-stack") {
    for (let i = 0; i < 3; i++) {
      box(.5, .07, .34, [sage, 0xbc8d7d, cream][i], (i % 2) * .02, .045 + i * .08, 0);
      box(.46, .04, .345, 0xf1eadb, (i % 2) * .02, .045 + i * .08, .005);
    }
  } else if (kind === "candle") {
    cylinder(.2, .025, brass, 0, .015, 0);
    cylinder(.13, .4, cream, 0, .23, 0);
    cylinder(.008, .035, dark, 0, .447, 0);
  } else if (kind === "succulent") {
    cylinder(.21, .22, cream, 0, .11, 0, .16);
    cylinder(.19, .012, 0x514333, 0, .222, 0);
    for (let i = 0; i < 10; i++) {
      const angle = i * 2.4;
      const leaf = oval(.14, .13, .29, i % 2 ? sage : 0x6f8560, Math.sin(angle) * .1, .28 + i * .009, Math.cos(angle) * .1);
      leaf.rotation.set(.3, angle, 0);
    }
  }
  return group;
}
