import type { CompanionFurniture } from "../../../../../packages/utils/src/companion";
import { roomItemDefinitions } from "../../../../../packages/utils/src/companionRoomItems";
export type RoomKind = CompanionFurniture["kind"];
export type RoomSurfaceId = NonNullable<CompanionFurniture["surface"]>;
export type RoomItem = CompanionFurniture;
export type RoomPlacement = { item: RoomItem; valid: boolean; snapped: boolean; reason: string | null };
export type RoomEnvironment = "overlay" | "studio";
export type RoomCamera = { yaw: number; zoom: number };
export type RoomPlay = { id: string; token: number; kind: "water" | "light" | "bounce" };
export type RoomSceneState = {
  items: RoomItem[];
  selectedId: string | null;
  editing: boolean;
  reducedMotion: boolean;
  camera: RoomCamera;
  play: RoomPlay | null;
  environment?: RoomEnvironment;
  definition?: PersonalRoom;
};
export type RoomSceneBindings = {
  onSelect: (id: string | null) => void;
  onTransformStart: () => void;
  onTransform: (id: string, patch: Partial<Pick<RoomItem, "x" | "y" | "z" | "rotation" | "surface">>) => void;
  onTransformEnd: (cancelled: boolean) => void;
  onPlacementPreview?: (placement: RoomPlacement | null) => void;
  onCameraChange: (camera: RoomCamera) => void;
  onInteract: (id: string) => void;
  onPlayComplete: (token: number) => void;
};
export const roomCatalog: { kind: RoomKind; name: string }[] = [
  { kind: "lamp", name: "Table lamp" }, { kind: "stool", name: "Boucle stool" }, { kind: "plant", name: "Rubber plant" },
];
export const roomName = (kind: RoomKind) => roomItemDefinitions[kind].name;
export const initialRoom: RoomItem[] = [
  { id: "studio-lamp", kind: "lamp", x: .65, z: -.72, rotation: 0, lit: true, surface: "console" },
  { id: "studio-plant", kind: "plant", x: .75, z: 1.05, rotation: 0, surface: "floor" },
];
export const defaultRoomCamera: RoomCamera = { yaw: 0, zoom: 1 };
export { placementError } from "./placement";
import type { PersonalRoom } from "../../../../../packages/utils/src/companion";
