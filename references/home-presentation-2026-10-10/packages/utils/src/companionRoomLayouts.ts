import type { PersonalRoom } from './companion';
import { legacyRoomLayout, type RoomLayout, type RoomObstacle } from './companionPlacement';
import { floorRoomKinds, tabletopRoomKinds } from './companionRoomItems';
import { companionHomeLayout } from './companionHomeDesign';

export type AuthoredRoomLayoutId = 'pat-studio-v01' | 'pink-bedroom-v01' | 'pink-bedroom-v02' | 'modern-kitchen-v01' | 'bathroom-v01';

// The source is in metres with Z up. Runtime axes are [x - 1.6891, z, 2.2606 - y].
// Bounds come from the supplied Studio and the versioned bedroom handoff.
const sourceObstacle = (id: string, x0: number, x1: number, y0: number, y1: number, height: number, y = 0): RoomObstacle => ({
  id, x: (x0 + x1) / 2 - 1.6891, z: 2.2606 - (y0 + y1) / 2, width: x1 - x0, depth: y1 - y0, height, y,
});
const sourceBounds = { minX: -1.6891, maxX: 1.6891, minZ: -2.2606, maxZ: 2.2606 };
const sourceStanding = [1.2, .6, 0, -.6, -1.2, -1.8].flatMap(z => [-.8, -.2, .4, 1, -1.3].map(x => ({ x, z, yaw: Math.PI })));
const sourceFloor = { id: 'floor' as const, name: 'Floor', x: 0, z: 0, y: 0, width: 3.3782, depth: 4.5212,
  kinds: floorRoomKinds, anchors: sourceStanding.map(({ x, z }) => ({ x, z })) };

export const patStudioLayout: RoomLayout = {
  id: 'pat-studio-v01', bounds: sourceBounds, characterRadius: .24, characterHeight: 1.4, objectScale: .45,
  standingCandidates: sourceStanding,
  walls: [
    { id: 'wall-front', name: 'Console wall', x: 0, z: 2.2556, yaw: Math.PI, width: 3.3, minY: .2, maxY: 2.7 },
  ],
  surfaces: [sourceFloor,
    // Paired with the customization-ready v06 asset. These are measured clear
    // volumes above real support tops, not the entire furniture bounding box.
    { id: 'console', name: 'Console', x: -.32335, z: 1.88635, y: .6, width: 1.8001, depth: .4185,
      kinds: tabletopRoomKinds, anchors: [{ x: -.1641, z: 1.8956 }], supports: ['console'],
      clearance: { x: -.1641, z: 1.8956, width: .41, depth: .41, height: .56 } },
    { id: 'shelf', name: 'Shelf', x: -1.4196, z: 1.9881, y: .8177, width: .455, depth: .455,
      kinds: tabletopRoomKinds, anchors: [{ x: -1.4191, z: 1.9876 }], supports: ['shelf'],
      clearance: { x: -1.4191, z: 1.9876, width: .41, depth: .41, height: .56 } },
  ],
  obstacles: [
    sourceObstacle('chair', 2.2003, 3.2187, .1324, 1.1943, .75),
    sourceObstacle('console', .4657, 2.2658, .165, .5835, .6),
    sourceObstacle('shelf', .023, .5326, .026, .5256, 1.775),
    sourceObstacle('workstation', 1.2633, 3.3684, 1.7087, 3.5597, .725),
    sourceObstacle('desk-chair', 1.6062, 2.3273, 2.3121, 3.0066, .9),
    sourceObstacle('utility-rack', 0, 1.2309, 2.362, 4.48, 1.968),
    sourceObstacle('utility-products', .0097, .3762, 2.106, 3.4696, 1.555),
    sourceObstacle('bamboo', 2.8585, 3.3513, .7452, 1.3084, 1.045),
    sourceObstacle('console-nessino', .6054, .9261, .1898, .5102, .223, .6),
    sourceObstacle('console-camera', 1.0377, 1.2438, .3165, .495, .25, .6),
    sourceObstacle('console-trailing-plant', 1.7976, 2.3108, .3604, .7457, .703),
    sourceObstacle('console-fado', 2.02, 2.271, .175, .469, .35, .6),
  ],
};

export const pinkBedroomLayout: RoomLayout = {
  id: 'pink-bedroom-v01', bounds: sourceBounds, characterRadius: .24, characterHeight: 1.4, objectScale: .45,
  standingCandidates: [-1.1, -.5, -1.7, .1, .7, 1.3].flatMap(z => [-.4, .2, -.95, .8, 1.35].map(x => ({ x, z, yaw: Math.PI }))),
  walls: patStudioLayout.walls,
  surfaces: [sourceFloor,
    { id: 'console', name: 'Dresser', x: 1.3009, z: -1.3994, y: .81, width: .50, depth: 1.18,
      kinds: tabletopRoomKinds, anchors: [{ x: 1.285, z: -1.65 }, { x: 1.285, z: -1.12 }], supports: ['dresser'] },
    { id: 'shelf', name: 'Shelf', x: -1.4191, z: -1.6094, y: 1.4225, width: .44, depth: .88,
      kinds: tabletopRoomKinds, anchors: [{ x: -1.42, z: -1.65 }], supports: ['shelf'] },
  ],
  obstacles: [
    { id: 'bed', x: -.165, z: 1.045, width: 1.63, depth: 2.15, height: 1.25 },
    { id: 'bedside-left', x: -1.25, z: 1.695, width: .46, depth: .49, height: .75 },
    { id: 'bedside-right', x: .94, z: 1.695, width: .48, depth: .49, height: .75 },
    { id: 'dresser', x: 1.285, z: -1.4, width: .55, depth: 1.2, height: .81 },
    { id: 'shelf', x: -1.42, z: -1.61, width: .44, depth: .88, height: 1.7 },
  ],
};

// The larger bedroom is a separate saved-room contract. Source axes convert
// to runtime [x - 2.4, z, 2.6 - y]; never reuse the v01 room's coordinates.
const bedroomV2Standing = [-.75, -1.3, -.25, .35, .95].flatMap(z =>
  [0, .6, -.6, 1.05, -1.05, -1.75].map(x => ({ x, z, yaw: Math.PI })));
export const pinkBedroomV2Layout: RoomLayout = {
  id: 'pink-bedroom-v02', bounds: { minX: -2.4, maxX: 2.4, minZ: -2.6, maxZ: 2.6 },
  characterRadius: .24, characterHeight: 1.4, objectScale: .45,
  standingCandidates: bedroomV2Standing,
  walls: [
    { id: 'wall-back', name: 'Workstation wall', x: 0, z: -2.595, yaw: 0, width: 4.7, minY: .2, maxY: 2.8,
      exclusions: [{ u: -1.74, y: 0, width: 1.12, height: 2.3 }] },
    { id: 'wall-left', name: 'Window wall', x: -2.395, z: 0, yaw: Math.PI / 2, width: 5.1, minY: .2, maxY: 2.8,
      exclusions: [{ u: -.025, y: .75, width: 2.3, height: 2.05 }] },
    { id: 'wall-right', name: 'Wardrobe wall', x: 2.395, z: 0, yaw: -Math.PI / 2, width: 5.1, minY: .2, maxY: 2.8 },
    // The curved headboard wall is not a flat mounting surface.
  ],
  floorPatches: [{ x: -.7, z: .25, width: 2.75, depth: 1.6, y: .014 }],
  surfaces: [
    { id: 'floor', name: 'Floor', x: 0, z: 0, y: 0, width: 4.8, depth: 5.2,
      kinds: floorRoomKinds, anchors: bedroomV2Standing.map(({ x, z }) => ({ x, z })) },
    { id: 'console', name: 'Console', x: -.53, z: -2.34, y: .8125, width: 1.33, depth: .49,
      kinds: tabletopRoomKinds, anchors: [{ x: -.53, z: -2.265 }], supports: ['media-console'],
      clearance: { x: -.53, z: -2.265, width: .42, depth: .42, height: .56 } },
    { id: 'shelf', name: 'Shelf', x: -2.145, z: -1.48, y: 1.2665, width: .48, depth: .90,
      kinds: tabletopRoomKinds, anchors: [{ x: -2.145, z: -1.48 }], supports: ['decor-shelf'],
      clearance: { x: -2.145, z: -1.48, width: .42, depth: .42, height: .56 } },
  ],
  // Measured from the v14 portable room. V15 adds only a wall-mounted canvas.
  // Sizes round outward to retain textile, foliage and curtain silhouettes.
  obstacles: [
    { id: 'bed', x: -.7, z: 1.2205, width: 1.9766, depth: 2.3352, height: 1.5351 },
    { id: 'bedside-left', x: -2.06, z: 1.91, y: .04, width: .4602, depth: .5202, height: .9126 },
    { id: 'bedside-right', x: .58, z: 1.91, y: .04, width: .4602, depth: .5202, height: .9126 },
    { id: 'wardrobe', x: 2.05, z: .55, y: .0299, width: .6962, depth: 3.4262, height: 2.8062 },
    { id: 'clothes-rack', x: 2.0457, z: -.48, y: .256, width: .6288, depth: 1.2802, height: 2.4081 },
    { id: 'workstation', x: 1.1, z: -2.261, width: 1.7502, depth: .6622, height: 1.3451 },
    { id: 'desk-chair', x: 1.1005, z: -1.52, width: .6292, depth: .6268, height: 1.0837 },
    { id: 'media-console', x: -.53, z: -2.327, y: .0399, width: 1.3302, depth: .5162, height: .7727 },
    { id: 'media-display', x: -.5, z: -2.545, y: .9049, width: 1.4302, depth: .0852, height: 1.2502 },
    { id: 'decor-shelf', x: -2.1475, z: -1.48, y: 1, width: .4852, depth: .9002, height: .2665 },
    { id: 'window-chair', x: -1.71, z: -.75, width: .7002, depth: .6102, height: 1.05 },
    { id: 'side-table', x: -1.62, z: -1.58, y: .009, width: .4402, depth: .4402, height: .5711 },
    { id: 'mirror', x: 2.36375, z: -1.44, y: .125, width: .0476, depth: .5102, height: 1.9501 },
    { id: 'curtain-near-bed', x: -2.2631, z: .7609, y: .0492, width: .0816, depth: .4304, height: 2.7991 },
    { id: 'curtain-near-seat', x: -2.2631, z: -.7091, y: .0492, width: .0816, depth: .4304, height: 2.7991 },
    { id: 'window-sill', x: -2.375, z: .025, y: .7675, width: .2202, depth: 2.1902, height: .0351 },
    { id: 'bedside-indoor-plant', x: 1.189, z: 1.856, width: .7866, depth: .6854, height: 2.0338 },
    { id: 'workstation-wall-painting', x: 1.1, z: -2.5675, y: 1.55, width: 1.21, depth: .065, height: .91 },
  ],
};

export function validRoomLayout(environmentId: unknown, layoutId: unknown): boolean {
  return layoutId === undefined || environmentId === 'studio' && ['pat-studio-v01', 'pink-bedroom-v01', 'pink-bedroom-v02', 'modern-kitchen-v01', 'bathroom-v01'].includes(String(layoutId));
}

/** Missing layout IDs retain the original saved-room coordinate contract. */
export function roomLayoutFor(room?: Pick<PersonalRoom, 'environmentId' | 'layoutId' | 'homeDesign'> | null): RoomLayout {
  const base = baseRoomLayoutFor(room);
  return room ? companionHomeLayout(room, base) : base;
}

function baseRoomLayoutFor(room?: Pick<PersonalRoom, 'environmentId' | 'layoutId'> | null): RoomLayout {
  if (room?.environmentId === 'terrace') return terracePlacementLayout;
  if (room?.environmentId !== 'studio') return legacyRoomLayout;
  if (room.layoutId === 'pat-studio-v01') return patStudioLayout;
  if (room.layoutId === 'pink-bedroom-v01') return pinkBedroomLayout;
  if (room.layoutId === 'pink-bedroom-v02') return pinkBedroomV2Layout;
  if (room.layoutId === 'modern-kitchen-v01') return kitchenLayout;
  if (room.layoutId === 'bathroom-v01') return bathroomLayout;
  return legacyRoomLayout;
}
// Terrace retains its saved horizontal supports, without imaginary indoor walls.
const terracePlacementLayout: RoomLayout = { ...legacyRoomLayout, walls: [] };

const homeStanding = [1.6, 1.1, -.9, -1.5].flatMap(z => [-1.6, -.8, 0, .8, 1.6].map(x => ({ x, z, yaw: Math.PI })));
const homeFloor = { id: 'floor' as const, name: 'Floor', x: 0, z: 0, y: 0, width: 5.2, depth: 5.2,
  kinds: floorRoomKinds, anchors: homeStanding.map(({ x, z }) => ({ x, z })) };
export const kitchenLayout: RoomLayout = {
  id: 'modern-kitchen-v01', bounds: { minX: -2.6, maxX: 2.6, minZ: -2.6, maxZ: 2.6 },
  characterRadius: .24, characterHeight: 1.4, objectScale: .45, standingCandidates: homeStanding,
  walls: [
    { id: 'wall-back', name: 'Counter wall', x: 0, z: -2.595, yaw: 0, width: 5.1, minY: .2, maxY: 2.8,
      exclusions: [{ u: -.4, y: 1.5, width: 3.3, height: 1.05 }, { u: -.25, y: .96, width: 3.85, height: .66 }] },
    { id: 'wall-left', name: 'Left wall', x: -2.595, z: 0, yaw: Math.PI / 2, width: 5.1, minY: .2, maxY: 2.8 },
  ],
  surfaces: [homeFloor, { id: 'console', name: 'Counter', x: -1.35, z: -2.18, y: .94, width: 1.2, depth: .55,
    kinds: tabletopRoomKinds, anchors: [{ x: -1.6, z: -2.18 }], supports: ['counter'] }],
  obstacles: [
    { id: 'island', x: 0, z: 0, width: 2.1, depth: 1.05, height: .95 },
    { id: 'counter', x: -.25, z: -2.2, width: 3.85, depth: .7, height: .96 },
    { id: 'fridge', x: 2.05, z: -2.1, width: .8, depth: .85, height: 2.1 },
    { id: 'island-seat', x: 1.4, z: .15, width: .5, depth: .5, height: .85 },
  ],
};
export const bathroomLayout: RoomLayout = {
  id: 'bathroom-v01', bounds: { minX: -2.6, maxX: 2.6, minZ: -2.6, maxZ: 2.6 },
  characterRadius: .24, characterHeight: 1.4, objectScale: .45, standingCandidates: homeStanding,
  walls: [
    { id: 'wall-back', name: 'Vanity wall', x: 0, z: -2.595, yaw: 0, width: 5.1, minY: .2, maxY: 2.8,
      exclusions: [{ u: .9, y: 1.15, width: 1.6, height: 1.2 }] },
    { id: 'wall-left', name: 'Bath wall', x: -2.595, z: 0, yaw: Math.PI / 2, width: 5.1, minY: .2, maxY: 2.8 },
  ],
  surfaces: [homeFloor, { id: 'console', name: 'Vanity', x: .9, z: -2.18, y: .87, width: 1.6, depth: .55,
    kinds: tabletopRoomKinds, anchors: [{ x: 1.5, z: -2.18 }], supports: ['vanity'] }],
  obstacles: [
    { id: 'bath', x: -1.05, z: .75, width: 1.05, depth: 2.05, height: .62 },
    { id: 'vanity', x: .9, z: -2.18, width: 1.7, depth: .65, height: .9 },
    { id: 'shower', x: -1.8, z: -1.7, width: 1.2, depth: 1.4, height: 2.2 },
  ],
};
