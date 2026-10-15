/** Metres, Y up. Preserved v03 district with v05 road damage and v06 vendor clearances. */
export const townSquareBounds = {"minX":-38,"maxX":38,"minZ":-33,"maxZ":39};
const districtObstacles = [
  {"id":"West cream compound","minX":-30,"maxX":-16.85,"minZ":22.72,"maxZ":37.28},
  {"id":"West bungalow","minX":-30,"maxX":-16.85,"minZ":4.719999999999999,"maxZ":19.28},
  {"id":"West apartments","minX":-30,"maxX":-16.85,"minZ":-13.280000000000001,"maxZ":1.2800000000000011},
  {"id":"West corner house","minX":-30,"maxX":-16.85,"minZ":-32.28,"maxZ":-17.72},
  {"id":"Distant west compound","minX":-30,"maxX":-16.85,"minZ":-49.28,"maxZ":-34.72},
  {"id":"East residence","minX":16.95,"maxX":29,"minZ":22.16,"maxZ":37.84},
  {"id":"East family house","minX":16.95,"maxX":29,"minZ":-31.84,"maxZ":-16.16},
  {"id":"East apartments","minX":16.95,"maxX":29,"minZ":-49.84,"maxZ":-34.16},
  {"id":"Provision store","minX":19.5,"maxX":30.5,"minZ":6.3,"maxZ":13.7},
  {"id":"Suya kitchen","minX":19.5,"maxX":30.5,"minZ":-6.2,"maxZ":2.2},
  {"id":"Records","minX":19.5,"maxX":30.5,"minZ":-16.7,"maxZ":-9.3},
  {"id":"tree bed","minX":-17.45,"maxX":-14.55,"minZ":10.55,"maxZ":13.45},
  {"id":"tree bed","minX":-17.45,"maxX":-14.55,"minZ":-7.45,"maxZ":-4.55},
  {"id":"tree bed","minX":-17.45,"maxX":-14.55,"minZ":-25.45,"maxZ":-22.55},
  {"id":"bench","minX":-17.3,"maxX":-14.7,"minZ":4.5,"maxZ":5.5},
  {"id":"bench","minX":-17.3,"maxX":-14.7,"minZ":-6.5,"maxZ":-5.5},
  {"id":"tree bed","minX":14.55,"maxX":17.45,"minZ":10.55,"maxZ":13.45},
  {"id":"tree bed","minX":14.55,"maxX":17.45,"minZ":-7.45,"maxZ":-4.55},
  {"id":"tree bed","minX":14.55,"maxX":17.45,"minZ":-25.45,"maxZ":-22.55},
  {"id":"bench","minX":14.7,"maxX":17.3,"minZ":4.5,"maxZ":5.5},
  {"id":"bench","minX":14.7,"maxX":17.3,"minZ":-6.5,"maxZ":-5.5},
  {"id":"roadside tree","minX":-16.55,"maxX":-16.05,"minZ":31.75,"maxZ":32.25},
  {"id":"roadside tree","minX":-16.55,"maxX":-16.05,"minZ":1.75,"maxZ":2.25},
  {"id":"roadside tree","minX":-16.55,"maxX":-16.05,"minZ":-16.25,"maxZ":-15.75},
  {"id":"roadside tree","minX":-16.55,"maxX":-16.05,"minZ":-34.25,"maxZ":-33.75},
  {"id":"roadside tree","minX":-3.25,"maxX":-2.75,"minZ":31.75,"maxZ":32.25},
  {"id":"roadside tree","minX":-3.25,"maxX":-2.75,"minZ":1.75,"maxZ":2.25},
  {"id":"roadside tree","minX":-3.25,"maxX":-2.75,"minZ":-16.25,"maxZ":-15.75},
  {"id":"roadside tree","minX":-3.25,"maxX":-2.75,"minZ":-34.25,"maxZ":-33.75},
  {"id":"tree bed","minX":-34.45,"maxX":-31.55,"minZ":34.55,"maxZ":37.45},
  {"id":"tree bed","minX":-23.45,"maxX":-20.55,"minZ":34.55,"maxZ":37.45},
  {"id":"tree bed","minX":20.55,"maxX":23.45,"minZ":34.55,"maxZ":37.45},
  {"id":"tree bed","minX":31.55,"maxX":34.45,"minZ":34.55,"maxZ":37.45},
  {"id":"danfo","minX":-14.7,"maxX":-9.5,"minZ":7.4,"maxZ":12.6},
  {"id":"danfo","minX":6.4,"maxX":11.6,"minZ":18.4,"maxZ":23.6},
  {"id":"danfo","minX":-10.5,"maxX":-5.300000000000001,"minZ":-25.6,"maxZ":-20.4},
  {"id":"taxi","minX":-8.85,"maxX":-6.75,"minZ":24.85,"maxZ":29.15},
  {"id":"parked car","minX":-13.25,"maxX":-11.149999999999999,"minZ":-7.15,"maxZ":-2.85},
  {"id":"parked car","minX":-8.85,"maxX":-6.75,"minZ":-39.15,"maxZ":-34.85},
  {"id":"parked car","minX":-13.25,"maxX":-11.149999999999999,"minZ":29.85,"maxZ":34.15},
  {"id":"motorcycle","minX":-15.58,"maxX":-14.819999999999999,"minZ":16.04,"maxZ":17.96},
  {"id":"motorcycle","minX":18.62,"maxX":19.38,"minZ":-4.96,"maxZ":-3.04},
  {"id":"utility pole","minX":-16.7,"maxX":-16.3,"minZ":31.8,"maxZ":32.2},
  {"id":"utility pole","minX":-16.7,"maxX":-16.3,"minZ":15.8,"maxZ":16.2},
  {"id":"utility pole","minX":-16.7,"maxX":-16.3,"minZ":-1.2,"maxZ":-0.8},
  {"id":"utility pole","minX":-16.7,"maxX":-16.3,"minZ":-18.2,"maxZ":-17.8},
  {"id":"utility pole","minX":-16.7,"maxX":-16.3,"minZ":-35.2,"maxZ":-34.8},
  {"id":"utility pole","minX":-16.7,"maxX":-16.3,"minZ":-50.2,"maxZ":-49.8},
  {"id":"utility pole","minX":-3.6,"maxX":-3.1999999999999997,"minZ":31.8,"maxZ":32.2},
  {"id":"utility pole","minX":-3.6,"maxX":-3.1999999999999997,"minZ":15.8,"maxZ":16.2},
  {"id":"utility pole","minX":-3.6,"maxX":-3.1999999999999997,"minZ":-1.2,"maxZ":-0.8},
  {"id":"utility pole","minX":-3.6,"maxX":-3.1999999999999997,"minZ":-18.2,"maxZ":-17.8},
  {"id":"utility pole","minX":-3.6,"maxX":-3.1999999999999997,"minZ":-35.2,"maxZ":-34.8},
  {"id":"utility pole","minX":-3.6,"maxX":-3.1999999999999997,"minZ":-50.2,"maxZ":-49.8},
  {"id":"market","minX":10.3,"maxX":13.7,"minZ":10.7,"maxZ":13.3},
  {"id":"market","minX":17.3,"maxX":20.7,"minZ":-19.3,"maxZ":-16.7},
  {"id":"market","minX":-19.7,"maxX":-16.3,"minZ":1.7,"maxZ":4.3},
  {"id":"street figure","minX":-15.28,"maxX":-14.72,"minZ":-13.22,"maxZ":-12.78},
  {"id":"street figure","minX":-15.28,"maxX":-14.72,"minZ":4.78,"maxZ":5.22},
  {"id":"street figure","minX":-4.98,"maxX":-4.42,"minZ":8.78,"maxZ":9.22},
  {"id":"street figure","minX":-4.98,"maxX":-4.42,"minZ":-14.22,"maxZ":-13.78},
  {"id":"street figure","minX":9.72,"maxX":10.28,"minZ":11.78,"maxZ":12.22},
  {"id":"street figure","minX":18.72,"maxX":19.28,"minZ":-19.22,"maxZ":-18.78},
  {"id":"Stage canopy column","minX":-4.789999961853027,"maxX":-4.609999656677246,"minZ":-8.489999771118164,"maxZ":-8.309999465942383},
  {"id":"Stage canopy column.001","minX":-4.789999961853027,"maxX":-4.609999656677246,"minZ":-13.690000534057617,"maxZ":-13.510000228881836},
  {"id":"Stage canopy column.002","minX":4.609999656677246,"maxX":4.789999961853027,"minZ":-8.489999771118164,"maxZ":-8.309999465942383},
  {"id":"Stage canopy column.003","minX":4.609999656677246,"maxX":4.789999961853027,"minZ":-13.690000534057617,"maxZ":-13.510000228881836},
  {"id":"Stage back wall","minX":-5,"maxX":5,"minZ":-14,"maxZ":-13.799999237060547},
  {"id":"Stage speaker cabinet","minX":-4.210000038146973,"maxX":-3.5900001525878906,"minZ":-13.675000190734863,"maxZ":-13.22499942779541},
  {"id":"Stage speaker cabinet.001","minX":3.5900001525878906,"maxX":4.210000038146973,"minZ":-13.675000190734863,"maxZ":-13.22499942779541},
  {"id":"Streetlight","minX":14.9350004196167,"maxX":15.0649995803833,"minZ":11.9350004196167,"maxZ":12.0649995803833},
  {"id":"Streetlight.001","minX":14.9350004196167,"maxX":15.0649995803833,"minZ":-0.0650000050663948,"maxZ":0.0650000050663948},
  {"id":"Streetlight.002","minX":14.9350004196167,"maxX":15.0649995803833,"minZ":-12.0649995803833,"maxZ":-11.9350004196167},
  {"id":"Promenade bench","minX":-32,"maxX":-30,"minZ":35.724998474121094,"maxZ":36.275001525878906},
  {"id":"Promenade bench leg","minX":-31.760000228881836,"maxX":-31.64000129699707,"minZ":35.775001525878906,"maxZ":36.224998474121094},
  {"id":"Promenade bench leg.001","minX":-30.35999870300293,"maxX":-30.239999771118164,"minZ":35.775001525878906,"maxZ":36.224998474121094},
  {"id":"Promenade light","minX":-33.05500030517578,"maxX":-32.94499969482422,"minZ":34.744998931884766,"maxZ":34.85499954223633},
  {"id":"Promenade bench.001","minX":-21,"maxX":-19,"minZ":35.724998474121094,"maxZ":36.275001525878906},
  {"id":"Promenade bench leg.002","minX":-20.760000228881836,"maxX":-20.64000129699707,"minZ":35.775001525878906,"maxZ":36.224998474121094},
  {"id":"Promenade bench leg.003","minX":-19.35999870300293,"maxX":-19.239999771118164,"minZ":35.775001525878906,"maxZ":36.224998474121094},
  {"id":"Promenade light.001","minX":-22.05500030517578,"maxX":-21.94499969482422,"minZ":34.744998931884766,"maxZ":34.85499954223633},
  {"id":"Promenade bench.002","minX":23,"maxX":25,"minZ":35.724998474121094,"maxZ":36.275001525878906},
  {"id":"Promenade bench leg.004","minX":23.239999771118164,"maxX":23.35999870300293,"minZ":35.775001525878906,"maxZ":36.224998474121094},
  {"id":"Promenade bench leg.005","minX":24.64000129699707,"maxX":24.760000228881836,"minZ":35.775001525878906,"maxZ":36.224998474121094},
  {"id":"Promenade light.002","minX":21.94499969482422,"maxX":22.05500030517578,"minZ":34.744998931884766,"maxZ":34.85499954223633},
  {"id":"Promenade bench.003","minX":34,"maxX":36,"minZ":35.724998474121094,"maxZ":36.275001525878906},
  {"id":"Promenade bench leg.006","minX":34.23999786376953,"maxX":34.36000061035156,"minZ":35.775001525878906,"maxZ":36.224998474121094},
  {"id":"Promenade bench leg.007","minX":35.63999938964844,"maxX":35.76000213623047,"minZ":35.775001525878906,"maxZ":36.224998474121094},
  {"id":"Promenade light.003","minX":32.94499969482422,"maxX":33.05500030517578,"minZ":34.744998931884766,"maxZ":34.85499954223633},
];
// Roadwear source coordinates [x,y] map to runtime [x,-y]. Crossing slabs remain open.
const drainObstacles = [
  [-14.83, -14.25, 24.12, 28.8], [-14.83, -14.25, 30, 34.3],
  [-14.83, -14.25, -4.5, 10.45], [-14.83, -14.25, 13.55, 17.95],
  [-5.75, -5.09, -7.7, -3.2], [-5.75, -5.09, -2, 8.65], [-5.75, -5.09, 9.85, 17.95],
].map(([minX, maxX, minZ, maxZ], index) => ({ id: `open-drain-${index}`, minX, maxX, minZ, maxZ }));
const potholeObstacles = [
  [-10.5, 15, .70, 1.02], [-8.1, 10.6, .58, .82], [-12.55, 18.9, .46, .70],
  [-8.4, 4.2, .41, .62], [-10.6, -2.6, .69, .91],
].map(([x, z, rx, rz], index) => ({ id: `pothole-${index}`,
  minX: x - rx * 1.2, maxX: x + rx * 1.2, minZ: z - rz * 1.2, maxZ: z + rz * 1.2 }));
// Reserve stand equipment and the complete local NPC sweep, not only their home poses.
const vendorObstacles = [
  { id: "mai-suya-stand", minX: -5.06, maxX: -3.5, minZ: -25, maxZ: -22.6 },
  { id: "akara-stand", minX: -5.06, maxX: -3.5, minZ: 15.1, maxZ: 17.65 },
  { id: "cap-npc-sweep", minX: -8.35, maxX: -4.1, minZ: -19.55, maxZ: -18.85 },
  { id: "shirt-npc", minX: -4.65, maxX: -3.95, minZ: -5.35, maxZ: -4.65 },
  { id: "vest-npc-sweep", minX: -4.75, maxX: -4.05, minZ: 13.45, maxZ: 15.6 },
];
export const townSquareObstacles = [
  // Keep authored vehicle footprints for reduced motion and disabled scenery.
  // Live traffic collision needs a shared contract before these can be removed.
  ...districtObstacles,
  ...drainObstacles, ...potholeObstacles, ...vendorObstacles,
];
export const townSquareFloorPatches = [
  // West sidewalk curb elevation (+0.14m)
  { x: -15.5, z: 5, width: 2.2, depth: 64, y: 0.14 },
  // East sidewalk curb elevation (+0.14m)
  { x: 15.5, z: 5, width: 2.2, depth: 64, y: 0.14 },
  // Stage platform elevation (+0.45m)
  { x: 0, z: -11, width: 9.6, depth: 5.5, y: 0.45 },
  // Roadside open drains / gutters (-0.22m depression)
  ...[
    [-14.54, 26.46, .58, 4.68], [-14.54, 32.15, .58, 4.3],
    [-14.54, 2.97, .58, 14.95], [-14.54, 15.75, .58, 4.4],
    [-5.42, -5.45, .66, 4.5], [-5.42, 3.32, .66, 10.65], [-5.42, 13.9, .66, 8.1],
  ].map(([x, z, width, depth]) => ({ x, z, width, depth, y: -0.22 })),
  // Pothole depressions (-0.10m depression)
  ...[
    [-10.5, 15, 1.4, 2.04], [-8.1, 10.6, 1.16, 1.64], [-12.55, 18.9, .92, 1.4],
    [-8.4, 4.2, .82, 1.24], [-10.6, -2.6, 1.38, 1.82],
  ].map(([x, z, width, depth]) => ({ x, z, width, depth, y: -0.10 })),
];

export const townSquareNavigation = {
  revision: "town-square-v08", bounds: townSquareBounds, obstacles: townSquareObstacles,
  radius: .3, floorY: 0, gridSize: .8, floorPatches: townSquareFloorPatches,
};
export const townSquareStanding = [4, 2, 0, -2, 6].flatMap(z => [0, 2, 4, 6, 8, 10, 12, -2, 1, 3].map(x => ({ x, z, yaw: Math.PI })));
