import * as THREE from "three";
import type { PersonalRoom } from "../../../../../packages/utils/src/companion";
import { companionEnvironments, disposeEnvironment, type CompanionEnvironment, type ScenePoint } from "./environments";
import { makeRoomObject, makeStudio } from "../room/studioGeometry";
import { itemSurfaceHeight, itemVolume, roomStandingPoints } from "../room/placement";
import { roomLayoutFor } from "../../../../../packages/utils/src/companionRoomLayouts";
import { authoredEnvironment, configureAuthoredMaterials } from "./authoredEnvironment";
import { routeSegmentClear } from "./navigation";
import { roomNavigationFor } from "../../../../../packages/utils/src/companionNavigation";
import { makeTerrace } from "./terraceGeometry";
import { observeRoomExpressionTargets } from "../../roomExpressionTargets";
import type { CharacterExpressionTarget } from "../appearance/expressionRig";
import { applyCustomRoomColors } from "./roomMaterials";
import { clearRoomPowerBindings, registerOwnedAuthoredRoomPower, registerRoomPowerBindings } from "./roomPowerBindings";
import { configureHomeDesign, homeDesignEnvironment } from "./homeDesign";
import { installRoomTelevision } from "./roomTelevision";

export function personalEnvironment(room: PersonalRoom, includeItems = true, onTargets?: (targets: readonly CharacterExpressionTarget[]) => void): CompanionEnvironment {
  const base = homeDesignEnvironment(authoredEnvironment({ ...room, homeDesign: undefined })
    ?? companionEnvironments.find(value => value.id === room.environmentId) ?? companionEnvironments[0], room);
  const layout = roomLayoutFor(room);
  const tint = { rose: "#e4ccbb", sage: "#becbb7", midnight: "#636681" }[room.palette];
  const navigation = roomNavigationFor(room);
  // Spawn and Walk over use the same swept-body clearance as tap-to-walk.
  const clear = (point: ScenePoint) => routeSegmentClear(navigation, point, point);
  const standingPoints: ScenePoint[] = roomStandingPoints(room.items, layout).map(point => [point.x, 0, point.z] as const).filter(clear);
  const preferredSpawn: ScenePoint = base.spawn ?? [0, 0, .6];
  let spawn: ScenePoint | undefined = clear(preferredSpawn) ? preferredSpawn : standingPoints[0];
  const margin = navigation.radius + .05;
  if (!spawn) for (let z = navigation.bounds.maxZ - margin; z >= navigation.bounds.minZ + margin && !spawn; z -= .15) for (let x = navigation.bounds.minX + margin; x <= navigation.bounds.maxX - margin; x += .15) {
    const candidate: ScenePoint = [x, 0, z];
    if (clear(candidate)) { spawn = candidate; break; }
  }
  function addItems(group: THREE.Group) {
    const targets: { id: string; object: THREE.Object3D }[] = [];
    if (includeItems) for (const item of room.items) {
      const object = makeRoomObject(item.kind, item.lit !== false);
      object.userData.companionFurnitureId = item.id;
      object.scale.setScalar(layout.objectScale);
      object.position.set(item.x, itemSurfaceHeight(item, layout), item.z); object.rotation.y = item.rotation;
      group.add(object);
      targets.push({ id: `room:${room.id}:${item.kind}:${item.id}`, object });
    }
    return targets;
  }
  return {
    ...base,
    id: `${room.id}:${JSON.stringify({ ...room, items: includeItems ? room.items : [] })}`,
    instanceId: `${room.id}:${room.environmentId}:${layout.id}`,
    layout,
    cameraObstacles: [...(base.cameraObstacles ?? layout.obstacles), ...(includeItems ? room.items : []).map(item => ({ id: item.id, ...itemVolume(item, layout) }))],
    name: room.name,
    background: room.environmentId === "terrace" ? { daylight: "#e3ddd0", golden: "#e9c9ac", evening: "#363e56" }[room.light] : tint,
    lighting: room.light,
    walkingPoints: standingPoints,
    spawn,
    close: spawn ? {
      eye: [base.close.eye[0] + spawn[0] - preferredSpawn[0], base.close.eye[1], base.close.eye[2] + spawn[2] - preferredSpawn[2]],
      target: [base.close.target[0] + spawn[0] - preferredSpawn[0], base.close.target[1], base.close.target[2] + spawn[2] - preferredSpawn[2]],
    } : base.close,
    navigation,
    authored: base.authored ? { ...base.authored, fixedSourceTransforms: true, configure(group) {
      configureAuthoredMaterials(group, room);
      const releaseDesign = configureHomeDesign(group, room);
      let releasePowerBindings: (() => void) | undefined;
      let releaseTelevision: (() => void) | undefined;
      const items = new THREE.Group();
      try {
        releasePowerBindings = registerOwnedAuthoredRoomPower(group);
        releaseTelevision = installRoomTelevision(group, room);
        if (room.environmentId === "terrace" && room.light !== "daylight") {
          const practical = new THREE.PointLight("#ffe2b9", room.light === "evening" ? 1.1 : .25, 5, 2);
          practical.position.set(0, 2.4, -2.25); items.add(practical);
          registerRoomPowerBindings(items, [{ kind: "light", target: practical }]);
        }
        const targets = addItems(items); group.add(items);
        if (onTargets) observeRoomExpressionTargets(group, targets, onTargets);
      }
      catch (error) { releaseTelevision?.(); releaseDesign(); releasePowerBindings?.(); clearRoomPowerBindings(items); items.removeFromParent(); disposeEnvironment(items); throw error; }
      return () => { releaseTelevision?.(); releaseDesign(); releasePowerBindings?.(); clearRoomPowerBindings(items); items.removeFromParent(); disposeEnvironment(items); };
    } } : undefined,
    create() {
      const group = new THREE.Group();
      if (room.environmentId === "terrace") group.add(makeTerrace(room.palette, room.light));
      else { const studio = makeStudio(); group.add(studio.enclosure, studio.furniture); }
      const tinted = new Set<THREE.Material>();
      group.traverse(node => {
        if (room.environmentId === "terrace") return;
        if (!(node instanceof THREE.Mesh)) return;
        for (const material of Array.isArray(node.material) ? node.material : [node.material]) {
          if (material instanceof THREE.MeshStandardMaterial && !tinted.has(material)) { material.color.lerp(new THREE.Color(tint), room.palette === "midnight" ? .5 : .2); tinted.add(material); }
        }
      });
      applyCustomRoomColors(group, room);
      const targets = addItems(group);
      if (onTargets) observeRoomExpressionTargets(group, targets, onTargets);
      return group;
    },
  };
}
