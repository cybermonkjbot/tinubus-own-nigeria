import type { PersonalRoom } from "./companion";
import { companionStations } from "./companionLife";
import { kitchenLayout } from "./companionRoomLayouts";
import type { ScenePoint } from "./companionNavigation";
import { companionHomeBounds, companionHomePoint } from "./companionHomeDesign";

export type CompanionKitchenObject = Readonly<{
  id: "kitchen.fridge" | "kitchen.cook" | "kitchen.meal";
  kind: "fridge" | "cook" | "meal";
  label: string;
  stationId: "fridge" | "island" | "island-seat";
  approach: ScenePoint;
  /** Room-local selection on the existing batched mesh, never new collision. */
  bounds: Readonly<{ minX: number; maxX: number; minY: number; maxY: number; minZ: number; maxZ: number }>;
}>;
const at = (x: number, y: number, z: number): ScenePoint => Object.freeze([x, y, z] as const);
const fridge = kitchenLayout.obstacles.find(value => value.id === "fridge")!;
const stations = companionStations({ id: "kitchen-anchors", layoutId: "modern-kitchen-v01", environmentId: "studio" });
const cook = stations.find(value => value.id === "island")!;
const eat = stations.find(value => value.id === "island-seat")!;
const targets: readonly CompanionKitchenObject[] = Object.freeze([
  Object.freeze({ id: "kitchen.fridge", kind: "fridge", label: "Fridge", stationId: "fridge",
    approach: at(fridge.x, 0, fridge.z + fridge.depth / 2 + kitchenLayout.characterRadius + .15),
    bounds: Object.freeze({ minX: fridge.x - fridge.width / 2 - .04, maxX: fridge.x + fridge.width / 2 + .04,
      minY: .05, maxY: fridge.height + .04, minZ: fridge.z - fridge.depth / 2 - .04, maxZ: fridge.z + fridge.depth / 2 + .05 }) }),
  Object.freeze({ id: "kitchen.cook", kind: "cook", label: "Cook", stationId: "island",
    approach: at(...cook.approach),
    bounds: Object.freeze({ minX: -1.05, maxX: 1.05, minY: .86, maxY: 1.35, minZ: -.525, maxZ: .525 }) }),
  Object.freeze({ id: "kitchen.meal", kind: "meal", label: "Food", stationId: "island-seat",
    approach: at(...eat.approach),
    // The existing care plate sits at [position.x - .5, .975, position.z].
    bounds: Object.freeze({ minX: .7, maxX: 1.05, minY: .895, maxY: 1.12, minZ: -.05, maxZ: .35 }) }),
]);
const empty: readonly CompanionKitchenObject[] = Object.freeze([]);

/** No stock, shopping permissions or meal quantities are inferred from a room. */
export function companionKitchenObjectTargets(room: Pick<PersonalRoom, "environmentId" | "layoutId" | "homeDesign">): readonly CompanionKitchenObject[] {
  if (room.environmentId !== "studio" || room.layoutId !== "modern-kitchen-v01") return empty;
  return room.homeDesign ? targets.map(target => ({ ...target, approach: companionHomePoint(room, target.approach),
    bounds: companionHomeBounds(room, target.bounds) })) : targets;
}
