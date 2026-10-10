import { placementError, roomStandingPoints } from "./companionPlacement";
import { roomLayoutFor, validRoomLayout, type AuthoredRoomLayoutId } from "./companionRoomLayouts";
import { garmentStyles, fabricTextures, clothingPatterns, type GarmentStyle, type FabricTexture, type ClothingPattern, type PaidGarmentStyle } from "./companionWardrobe";
import { companionRoomColorFields, validRoomColor, type RoomColors } from "./companionRoomStyle";
import { isRoomItemKind, roomSurfaceIds, type RoomItemKind, type RoomSurfaceId, type PaidRoomItemId } from "./companionRoomItems";
import { companionHomeSeed, companionStarterRooms, type CompanionHomeProgressView } from "./companionHome";
import { validCompanionHomeDesign, type CompanionHomeDesign } from "./companionHomeDesign";
/** Serializable visual state shared by the account store, room host and renderers. */
export const characterBaseIds = ["bestie", "bunny", "bear", "cat", "fox", "panda"] as const;
export type CharacterBaseId = typeof characterBaseIds[number];
export const companionFaceChoices = {
  eyeShape: ["classic", "round", "almond", "sleepy"],
  browStyle: ["classic", "soft", "bold", "curious"],
  noseShape: ["classic", "button", "wide"],
  mouthShape: ["classic", "small", "wide"],
  cheekStyle: ["none", "blush", "freckles"],
  markingStyle: ["none", "muzzle", "mask", "stripes"],
  eyewear: ["none", "round"],
} as const;
export const companionColorFields = ["bodyColor", "topColor", "trousersColor", "footwearColor", "eyeColor", "noseColor", "markingColor", "patternColor"] as const;
export type CompanionAppearance = {
  baseId?: CharacterBaseId;
  look: "rose" | "ivory";
  bodyColor?: string;
  topColor?: string;
  trousersColor?: string;
  footwearColor?: string;
  accessoryVisible?: boolean;
  necklaceVisible?: boolean;
  hipChainVisible?: boolean;
  eyeColor?: string;
  noseColor?: string;
  markingColor?: string;
  garmentStyle?: GarmentStyle;
  fabricTexture?: FabricTexture;
  clothingPattern?: ClothingPattern;
  patternColor?: string;
} & { [K in keyof typeof companionFaceChoices]?: typeof companionFaceChoices[K][number] };
export type CompanionFurniture = {
  id: string;
  kind: RoomItemKind;
  x: number; z: number; rotation: number;
  surface?: RoomSurfaceId;
  /** Bottom edge of wall decor. Horizontal supports determine their own height. */
  y?: number;
  lit?: boolean;
};
export type CharacterIdentity = { id: string; name: string; appearance: CompanionAppearance };
export type PersonalRoom = RoomColors & {
  id: string;
  name: string;
  environmentId: "studio" | "terrace";
  /** Absent on legacy rooms. Geometry updates never migrate saved coordinates. */
  layoutId?: AuthoredRoomLayoutId;
  homeDesign?: CompanionHomeDesign;
  palette: "rose" | "sage" | "midnight";
  light: "daylight" | "golden" | "evening";
  sharing: "private" | "host" | "collaborative";
  items: CompanionFurniture[];
};
export type CompanionLibrary = {
  version: 1;
  characters: CharacterIdentity[];
  rooms: PersonalRoom[];
  activeCharacterId: string;
  activeRoomId: string;
  homeRoomId: string;
};
export type CompanionLibrarySnapshot = { library: CompanionLibrary | null; revision: number; busy: boolean; error: string | null; homeProgress?: CompanionHomeProgressView; ownedWardrobeIds?: readonly PaidGarmentStyle[]; ownedRoomItemIds?: readonly PaidRoomItemId[] };
export interface CompanionLibraryStore {
  scopeId: string;
  getSnapshot(): CompanionLibrarySnapshot;
  subscribe(listener: () => void): () => void;
  save(library: CompanionLibrary, expectedRevision: number, options?: { avatarPng: Blob; signal?: AbortSignal }): Promise<void>;
  reload(): void;
}
export const COMPANION_LIBRARY_LIMIT = 12;
export const characterBases: readonly { id: CharacterBaseId; name: string; color: string }[] = [
  { id: "bestie", name: "Bestie", color: "#efc18e" },
  { id: "bunny", name: "Bunny", color: "#f4dfbe" },
  { id: "bear", name: "Bear", color: "#b8855e" },
  { id: "cat", name: "Cat", color: "#a3a6ac" },
  { id: "fox", name: "Fox", color: "#cf8150" },
  { id: "panda", name: "Panda", color: "#eee9df" },
];
export function defaultCompanionLibrary(ownerId?: string): CompanionLibrary {
  return {
    version: 1,
    characters: [{ id: "original", name: "Bestie", appearance: { baseId: "bestie", look: "rose" } }],
    rooms: ownerId ? companionStarterRooms(companionHomeSeed(`home-v1:${ownerId}`))
      : [{ id: "home", name: "The Studio", environmentId: "studio", layoutId: "pat-studio-v01", palette: "rose", light: "daylight", sharing: "private", items: [] }],
    activeCharacterId: "original", activeRoomId: "home", homeRoomId: "home",
  };
}
const record = (value: unknown): value is Record<string, unknown> => !!value && typeof value === "object" && !Array.isArray(value);
const text = (value: unknown, max: number): value is string => typeof value === "string" && value.trim().length > 0 && value.length <= max;
const member = (value: unknown, values: readonly string[]) => typeof value === "string" && values.includes(value);
export function validCompanionAppearance(value: unknown): value is CompanionAppearance {
  if (!record(value) || !member(value.look, ["rose", "ivory"])) return false;
  if (value.baseId !== undefined && !member(value.baseId, characterBaseIds)) return false;
  if (value.garmentStyle !== undefined && !member(value.garmentStyle, garmentStyles) ||
      value.fabricTexture !== undefined && !member(value.fabricTexture, fabricTextures) ||
      value.clothingPattern !== undefined && !member(value.clothingPattern, clothingPatterns)) return false;
  for (const key of companionColorFields) {
    if (value[key] !== undefined && (typeof value[key] !== "string" || !/^#[0-9a-f]{6}$/i.test(value[key]))) return false;
  }
  for (const key of Object.keys(companionFaceChoices) as (keyof typeof companionFaceChoices)[]) {
    if (value[key] !== undefined && !member(value[key], companionFaceChoices[key])) return false;
  }
  return ["accessoryVisible", "necklaceVisible", "hipChainVisible"].every(key => value[key] === undefined || typeof value[key] === "boolean");
}
export function validPersonalRoom(value: unknown): value is PersonalRoom {
  if (!record(value) || !text(value.id, 80) || !text(value.name, 32) ||
    !member(value.environmentId, ["studio", "terrace"]) || !member(value.palette, ["rose", "sage", "midnight"]) ||
    !member(value.light, ["daylight", "golden", "evening"]) || !member(value.sharing, ["private", "host", "collaborative"]) ||
    !Array.isArray(value.items) || value.items.length > 8 || !validRoomLayout(value.environmentId, value.layoutId)
    || !validCompanionHomeDesign({ environmentId: value.environmentId, layoutId: value.layoutId, homeDesign: value.homeDesign })) return false;
  if (companionRoomColorFields.some(field => value[field] !== undefined && !validRoomColor(value[field]))) return false;
  const ids = new Set<string>();
  for (const item of value.items) {
    if (!record(item) || !text(item.id, 80) || ids.has(item.id) || !isRoomItemKind(item.kind) ||
      ![item.x, item.z, item.rotation].every(n => typeof n === "number" && Number.isFinite(n)) ||
      Math.abs(item.x as number) > 4 || Math.abs(item.z as number) > 4 || Math.abs(item.rotation as number) > 1e4 ||
      (item.surface !== undefined && !member(item.surface, roomSurfaceIds)) ||
      (item.y !== undefined && (typeof item.y !== "number" || !Number.isFinite(item.y) || item.y < 0 || item.y > 4)) ||
      (item.lit !== undefined && typeof item.lit !== "boolean")) return false;
    ids.add(item.id);
  }
  const layout = roomLayoutFor(value as PersonalRoom);
  const items = value.items as CompanionFurniture[];
  return placementError(items, layout) === null && (value.layoutId === undefined || roomStandingPoints(items, layout).length > 0);
}
export function validCompanionLibrary(value: unknown): value is CompanionLibrary {
  if (!record(value) || value.version !== 1 || !Array.isArray(value.characters) || !Array.isArray(value.rooms) ||
    !value.characters.length || value.characters.length > COMPANION_LIBRARY_LIMIT || !value.rooms.length || value.rooms.length > COMPANION_LIBRARY_LIMIT) return false;
  const characterIds = new Set<string>(), roomIds = new Set<string>();
  for (const character of value.characters) {
    if (!record(character) || !text(character.id, 80) || !text(character.name, 20) || !validCompanionAppearance(character.appearance) || characterIds.has(character.id)) return false;
    characterIds.add(character.id);
  }
  for (const room of value.rooms) {
    if (!validPersonalRoom(room) || roomIds.has(room.id)) return false;
    roomIds.add(room.id);
  }
  return typeof value.activeCharacterId === "string" && characterIds.has(value.activeCharacterId) &&
    typeof value.activeRoomId === "string" && roomIds.has(value.activeRoomId) && typeof value.homeRoomId === "string" && roomIds.has(value.homeRoomId);
}
