import * as THREE from "three";
import type { PersonalRoom } from "../../../../../packages/utils/src/companion";
import { validRoomColor, type RoomColorField } from "../../../../../packages/utils/src/companionRoomStyle";

const authoredFields: Record<string, RoomColorField> = { wall: "wallColor", upholstery: "upholsteryColor", rug: "rugColor" };
const terraceFields: Record<string, RoomColorField> = { stone: "floorColor", upholstery: "upholsteryColor", accent: "rugColor" };

export function roomMaterialField(material: THREE.Material, room: PersonalRoom): RoomColorField | undefined {
  const tagged = material.userData.roomColorField;
  if (tagged === "wallColor" || tagged === "floorColor" || tagged === "upholsteryColor" || tagged === "rugColor") return tagged;
  if (room.environmentId === "terrace") return terraceFields[material.userData.paletteSlot];
  const authored = authoredFields[material.userData.baddie_palette_slot];
  if (authored) return authored;
  // These floor materials predate palette metadata. Match the registered asset,
  // not a broad name fragment that also catches marble tables or wall panels.
  if (room.layoutId === "pat-studio-v01" && material.name === "Studio v06 | floor baked finish"
    || room.layoutId === "pink-bedroom-v01" && material.name === "Floor | actual video grain and sealed finish"
    || room.layoutId === "pink-bedroom-v02" && material.name === "Bedroom v09 marble-floor") return "floorColor";
  return undefined;
}

export function customRoomMaterialColor(material: THREE.Material, room: PersonalRoom): string | undefined {
  const field = roomMaterialField(material, room);
  const value = field ? room[field] : undefined;
  return validRoomColor(value) ? value : undefined;
}

/** Called once on instance-owned materials. Shared textures remain untouched. */
export function applyCustomRoomColors(group: THREE.Group, room: PersonalRoom) {
  const visited = new Set<THREE.Material>();
  group.traverse(node => {
    if (!(node instanceof THREE.Mesh)) return;
    for (const material of Array.isArray(node.material) ? node.material : [node.material]) {
      if (!(material instanceof THREE.MeshStandardMaterial) || visited.has(material)) continue;
      visited.add(material);
      const color = customRoomMaterialColor(material, room);
      if (!color) continue;
      const target = new THREE.Color(color);
      const recorded = material.userData.baddie_palette_base_linear;
      if (material.map && Array.isArray(recorded) && recorded.length === 3
        && recorded.every(value => typeof value === "number" && Number.isFinite(value) && value > 0)) {
        material.color.multiply(new THREE.Color().setRGB(target.r / recorded[0], target.g / recorded[1], target.b / recorded[2]));
      } else if (material.map) {
        // Untagged floors keep their photographed grain or marble veining.
        material.color.multiply(target);
      } else material.color.copy(target);
    }
  });
}
