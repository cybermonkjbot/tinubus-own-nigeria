export const freeRoomItemIds = ["lamp", "stool", "plant"] as const;
export const paidRoomItemIds = [
  "botanical-painting", "abstract-painting", "landscape-painting", "wall-clock", "wall-mirror",
  "standing-fan", "tower-fan", "desk-fan", "floor-lamp", "desk-lamp",
  "armchair", "loveseat", "ottoman", "coffee-table", "side-table", "bookcase",
  "storage-basket", "book-stack", "candle", "succulent",
] as const;
export type PaidRoomItemId = typeof paidRoomItemIds[number];
export type RoomItemKind = typeof freeRoomItemIds[number] | PaidRoomItemId;
export const roomItemIds = [...freeRoomItemIds, ...paidRoomItemIds] as const;
export const horizontalRoomSurfaceIds = ["floor", "console", "shelf"] as const;
export const wallRoomSurfaceIds = ["wall-back", "wall-left", "wall-right", "wall-front"] as const;
export const roomSurfaceIds = [...horizontalRoomSurfaceIds, ...wallRoomSurfaceIds] as const;
export type HorizontalRoomSurfaceId = typeof horizontalRoomSurfaceIds[number];
export type WallRoomSurfaceId = typeof wallRoomSurfaceIds[number];
export type RoomSurfaceId = typeof roomSurfaceIds[number];
export type RoomItemDefinition = {
  name: string; category: string; description: string; color: string;
  placement: "floor" | "tabletop" | "wall";
  /** Geometry units, scaled by the saved room's objectScale. */
  radius: number; height: number; mass: number;
  wallWidth?: number; wallDepth?: number;
};

export const roomItemDefinitions: Record<RoomItemKind, RoomItemDefinition> = {
  lamp: { name: "Table lamp", category: "Lighting", description: "A cream shade on a walnut stand.", color: "#efe1c9", placement: "tabletop", radius: .29, height: .93, mass: .8 },
  stool: { name: "Boucle stool", category: "Seating", description: "A round upholstered seat with wooden legs.", color: "#e4cec4", placement: "floor", radius: .37, height: .6, mass: 2.1 },
  plant: { name: "Rubber plant", category: "Plants", description: "Broad green leaves in a terracotta pot.", color: "#d6dfcd", placement: "tabletop", radius: .28, height: 1.18, mass: 1.3 },
  "botanical-painting": { name: "Botanical painting", category: "Wall decor", description: "Sage leaves on an ivory canvas in a walnut frame.", color: "#dce3d1", placement: "wall", radius: .65, height: 1.7, wallWidth: 1.3, wallDepth: .12, mass: 1 },
  "abstract-painting": { name: "Abstract painting", category: "Wall decor", description: "Terracotta, sage and cream shapes in a walnut frame.", color: "#ead2c6", placement: "wall", radius: .9, height: 1.25, wallWidth: 1.8, wallDepth: .12, mass: 1 },
  "landscape-painting": { name: "Landscape painting", category: "Wall decor", description: "Rolling green hills beneath a warm sunset.", color: "#e1dfc9", placement: "wall", radius: .95, height: 1.15, wallWidth: 1.9, wallDepth: .12, mass: 1 },
  "wall-clock": { name: "Wall clock", category: "Wall decor", description: "A cream dial, walnut rim and dark hands.", color: "#e6ddce", placement: "wall", radius: .48, height: .96, wallWidth: .96, wallDepth: .14, mass: .7 },
  "wall-mirror": { name: "Oval wall mirror", category: "Wall decor", description: "An oval mirror with a slim brass frame.", color: "#dce3de", placement: "wall", radius: .52, height: 1.55, wallWidth: 1.04, wallDepth: .08, mass: 1.2 },
  "standing-fan": { name: "Standing fan", category: "Fans", description: "A sage pedestal fan with a round grille and weighted base.", color: "#d6dfd3", placement: "floor", radius: .55, height: 2.67, mass: 2 },
  "tower-fan": { name: "Tower fan", category: "Fans", description: "A tall ivory fan with dark vertical vents.", color: "#e8e4da", placement: "floor", radius: .34, height: 2.3, mass: 1.8 },
  "desk-fan": { name: "Desk fan", category: "Fans", description: "A small cream fan on a brass stand.", color: "#e9dfcf", placement: "tabletop", radius: .37, height: .92, mass: .8 },
  "floor-lamp": { name: "Floor lamp", category: "Lighting", description: "A linen shade on a tall brass stem.", color: "#eee0c6", placement: "floor", radius: .52, height: 3.05, mass: 2 },
  "desk-lamp": { name: "Desk lamp", category: "Lighting", description: "A sage shade and angled brass arm.", color: "#d8dfcf", placement: "tabletop", radius: .32, height: .95, mass: .8 },
  armchair: { name: "Lounge armchair", category: "Seating", description: "Cream upholstery with a deep seat and walnut feet.", color: "#e8dfd0", placement: "floor", radius: 1.13, height: 1.8, mass: 3 },
  loveseat: { name: "Blush loveseat", category: "Seating", description: "A compact two-seat sofa with rounded cushions.", color: "#e6cdca", placement: "floor", radius: 1.6, height: 1.7, mass: 4 },
  ottoman: { name: "Round ottoman", category: "Seating", description: "A sage upholstered footstool with a stitched top.", color: "#d6ddcd", placement: "floor", radius: .68, height: .78, mass: 2 },
  "coffee-table": { name: "Coffee table", category: "Tables", description: "A low walnut table with a rounded top.", color: "#dac4ad", placement: "floor", radius: 1.1, height: .82, mass: 3 },
  "side-table": { name: "Round side table", category: "Tables", description: "A walnut pedestal table with a circular top.", color: "#e1cdb2", placement: "floor", radius: .57, height: 1.05, mass: 2 },
  bookcase: { name: "Walnut bookcase", category: "Storage", description: "Four open shelves with a few cloth-bound books.", color: "#d8c5b1", placement: "floor", radius: .88, height: 2.7, mass: 4 },
  "storage-basket": { name: "Woven basket", category: "Storage", description: "An open woven basket with two handles.", color: "#e5d4b9", placement: "floor", radius: .5, height: .85, mass: 1 },
  "book-stack": { name: "Book stack", category: "Small decor", description: "Three cloth-bound books in cream, rose and sage.", color: "#e4d9c9", placement: "tabletop", radius: .34, height: .25, mass: .6 },
  candle: { name: "Pillar candle", category: "Small decor", description: "An ivory candle on a brass saucer.", color: "#eee4cd", placement: "tabletop", radius: .2, height: .47, mass: .5 },
  succulent: { name: "Succulent", category: "Plants", description: "A green rosette in a small cream planter.", color: "#dce2cf", placement: "tabletop", radius: .28, height: .48, mass: .6 },
};

export function isRoomItemKind(value: unknown): value is RoomItemKind {
  return typeof value === "string" && roomItemIds.some(id => id === value);
}
export function isPaidRoomItem(value: unknown): value is PaidRoomItemId {
  return typeof value === "string" && paidRoomItemIds.some(id => id === value);
}
export function isWallSurface(value: unknown): value is WallRoomSurfaceId {
  return typeof value === "string" && wallRoomSurfaceIds.some(id => id === value);
}
export function roomItemOwned(kind: RoomItemKind, owned: readonly string[] = []) {
  return !isPaidRoomItem(kind) || owned.includes(kind);
}
export const floorRoomKinds = roomItemIds.filter(kind => roomItemDefinitions[kind].placement !== "wall");
export const tabletopRoomKinds = roomItemIds.filter(kind => roomItemDefinitions[kind].placement === "tabletop");
