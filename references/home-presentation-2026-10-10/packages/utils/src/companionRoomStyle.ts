import type { PersonalRoom } from "./companion";
import { companionRoomName } from "./companionRoomNames";

export const companionRoomColorFields = ["wallColor", "floorColor", "upholsteryColor", "rugColor"] as const;
export type RoomColorField = typeof companionRoomColorFields[number];
export type RoomColors = Partial<Record<RoomColorField, string>>;

export const roomColorSwatches = [
  ["Ivory", "#f3eee4"], ["Sand", "#d6c2a6"], ["Rose", "#c58c91"],
  ["Terracotta", "#b87561"], ["Sage", "#a4b29a"], ["Forest", "#506651"],
  ["Sky", "#a8becd"], ["Lilac", "#b5a6c8"], ["Walnut", "#795844"], ["Ink", "#44485e"],
] as const;

export function validRoomColor(value: unknown): value is string {
  return typeof value === "string" && /^#[0-9a-f]{6}$/i.test(value);
}

export function validRoomColors(value: RoomColors): boolean {
  return companionRoomColorFields.every(field => value[field] === undefined || validRoomColor(value[field]));
}

/** An absent override retains the room's authored finish and selected palette. */
export function withRoomColor(room: PersonalRoom, field: RoomColorField, color: string): PersonalRoom {
  const next = { ...room };
  if (color === "") delete next[field];
  else next[field] = color.toLowerCase();
  return next;
}

/** Only expose finishes supported by the registered room's material contract. */
export function roomFinishOptions(room: Pick<PersonalRoom, "environmentId" | "layoutId">): readonly { field: RoomColorField; label: string }[] {
  if (room.environmentId === "terrace") return [
    { field: "floorColor", label: "Paving" }, { field: "upholsteryColor", label: "Upholstery" }, { field: "rugColor", label: "Cushions" },
  ];
  if (room.layoutId === "modern-kitchen-v01") return [{ field: "wallColor", label: "Cabinetry" }];
  if (room.layoutId === "bathroom-v01") return [{ field: "upholsteryColor", label: "Towels" }];
  return [
    { field: "wallColor", label: "Walls" }, { field: "floorColor", label: "Floor tint" },
    { field: "upholsteryColor", label: "Upholstery" }, { field: "rugColor", label: room.layoutId === "pink-bedroom-v01" ? "Bedding" : "Rug" },
  ];
}

export function withRoomLayout(room: PersonalRoom, environmentId: PersonalRoom["environmentId"], layoutId?: PersonalRoom["layoutId"]): PersonalRoom {
  const next = { ...room, environmentId, layoutId };
  if (room.homeDesign && (environmentId !== room.environmentId || layoutId !== room.layoutId)) {
    delete next.homeDesign;
    next.items = [];
  }
  if (!room.name.trim() || room.name === companionRoomName(room)) next.name = companionRoomName(next);
  const supported = new Set(roomFinishOptions(next).map(option => option.field));
  for (const field of companionRoomColorFields) if (!supported.has(field)) delete next[field];
  return next;
}

/** A preset is a complete style; individual overrides otherwise hide its effect. */
export function withRoomPalette(room: PersonalRoom, palette: PersonalRoom["palette"]): PersonalRoom {
  const next = { ...room, palette };
  for (const field of companionRoomColorFields) delete next[field];
  return next;
}
