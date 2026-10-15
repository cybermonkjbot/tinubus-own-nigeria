import * as THREE from "three";
import {
  createCompanionCityStreetDetailsPlan, createCityStreetCrossingLookup, cityStreetConstructionSpans, cityStreetDetailSegments,
  cityStreetProfile, cityStreetSeed, sampleCityStreet,
  type CityStreetFixture, type CityStreetRoad, type CityStreetSample, type CityStreetWire,
} from "../../../../../packages/utils/src/companionCityStreetDetails";
import { companionCityDividedRoads, companionCityRoadSurface } from "../../../../../packages/utils/src/companionCityRoadTreatments";
import { LagosScatter } from "../../world/LagosScatter";
import { companionCityPlan, companionCityParcelPoint } from "../../../../../packages/utils/src/companionCity";
import { companionCityArchitectureParts, type CityArchitecturePart } from "../../../../../packages/utils/src/companionCityArchitectureDetails";
import { companionCityFootballStreetBanner } from "../../../../../packages/utils/src/companionCityFootballFrontages";
import { companionCityEntranceConnections, companionCityCableAnchor, createCityStreetActivityLookup,
  type CityEntranceConnection } from "../../../../../packages/utils/src/companionCitySpatialInfill";
import civicTerrace from "../../../../../packages/assets/venues/civic-terrace-v1.json";
import architectureDelivery from "../../../../../packages/assets/city/lagos-architecture-delivery-v1.json";
import { signatureBuildingParcelIds } from "./signatureBuildings";
import { companionCityResidentialLights, companionCityResidentialLightEmitter, companionCityStreetLightEmitters,
  type CityLightEmitter, type CityResidentialLight } from "../../../../../packages/utils/src/companionCityLighting";
import { createCompanionCityResidentialUtilities, createCityStreetWireClearance,
  type CityResidentialUtilityAnchor } from "../../../../../packages/utils/src/companionCityResidentialUtilities";
import { cityUtilityPoleParts, cityUtilityPolePoint, cityUtilityPolePorts } from "../../../../../packages/utils/src/companionCityUtilityPoles";

type Part = { x: number; y: number; z: number; w: number; h: number; d: number; yaw: number;
  pitch?: number; roll?: number; colour: string };
type Segment = { road: CityStreetRoad; start: number; end: number };
type Tile = { x: number; z: number; segments: Segment[]; fixtures: CityStreetFixture[]; residentialLights: CityResidentialLight[];
  utilityAnchors: CityResidentialUtilityAnchor[]; emitters: CityLightEmitter[]; wires: CityStreetWire[];
  entrances: CityEntranceConnection[]; root?: THREE.Group; geometries: THREE.BufferGeometry[]; signs: string[] };
type SurfaceChunk = { positions: Float32Array; colours: Float32Array; count: number };
type FlatSurface = { chunks: SurfaceChunk[]; count: number; colourSize: 3 | 4 };

const CELL_SIZE = 160;
const LOAD_RADIUS = 305;
const RELEASE_RADIUS = 430;
const SURFACE_CHUNK_VERTICES = 384;

/** Additive nearby street detail. The caller owns the render loop and shared daylight clock. */
export function createCityStreetDetails(detailCollision = false, mains = { value: 1 }, roadTreatments = false,
  additionalLights: readonly CityStreetFixture[] = [], additionalRoads: readonly CityStreetRoad[] = [],
  walkingRoads: readonly CityStreetRoad[] = companionCityPlan.roads) {
  const group = new THREE.Group();
  group.name = "Nearby city street construction and fixtures";
  const source = createCompanionCityStreetDetailsPlan();
  const crossingAt = createCityStreetCrossingLookup([...walkingRoads, ...additionalRoads]);
  const activityAt = createCityStreetActivityLookup([...walkingRoads, ...additionalRoads]);
  const extensionRoadIds = new Set(additionalRoads.map(road => road.id));
  const tiles = new Map<string, Tile>();
  const residentTiles = new Set<Tile>();
  const tileAt = (x: number, z: number) => {
    const cx = Math.floor(x / CELL_SIZE), cz = Math.floor(z / CELL_SIZE), key = `${cx}:${cz}`;
    let tile = tiles.get(key);
    if (!tile) {
      tile = { x: (cx + .5) * CELL_SIZE, z: (cz + .5) * CELL_SIZE, segments: [], fixtures: [], residentialLights: [],
        utilityAnchors: [], emitters: [], wires: [], entrances: [], geometries: [], signs: [] };
      tiles.set(key, tile);
    }
    return tile;
  };
  for (const street of source.streets) {
    // Split at authored vertices and short metric spans before assigning by centre.
    for (const { start, end } of cityStreetDetailSegments(street.road)) {
      const point = sampleCityStreet(street.road, (start + end) / 2);
      tileAt(point.x, point.z).segments.push({ road: street.road, start, end });
    }
    for (const fixture of street.fixtures) if (detailCollision || !fixture.footprint) tileAt(fixture.x, fixture.z).fixtures.push(fixture);
  }
  // Extension surfaces have their own owners; only flush construction enters this layer.
  // No external columns or wires are invented for a frozen navigation revision.
  for (const road of additionalRoads) {
    if (["retained", "path", "sand-path", "boardwalk"].includes(road.kind)) continue;
    for (const { start, end } of cityStreetDetailSegments(road)) {
      const point = sampleCityStreet(road, (start + end) / 2);
      tileAt(point.x, point.z).segments.push({ road, start, end });
    }
  }
  if(detailCollision)for (const wire of source.wires) tileAt((wire.from[0] + wire.to[0]) / 2, (wire.from[2] + wire.to[2]) / 2).wires.push(wire);
  const crossesBuilding = createCityStreetWireClearance(companionCityPlan.parcels);
  const residentialUtilities = createCompanionCityResidentialUtilities(companionCityPlan.parcels,
    source.streets.map(street => street.road), [...tiles.values()].flatMap(tile => tile.fixtures), crossesBuilding);
  for (const pole of residentialUtilities.fixtures) tileAt(pole.x, pole.z).fixtures.push(pole);
  for (const anchor of residentialUtilities.anchors) tileAt(anchor.x, anchor.z).utilityAnchors.push(anchor);
  for (const wire of residentialUtilities.wires)
    tileAt((wire.from[0] + wire.to[0]) / 2, (wire.from[2] + wire.to[2]) / 2).wires.push(wire);
  if (detailCollision) {
    const poles = source.streets.flatMap(street => street.fixtures.filter(f => f.kind === "utility"));
    const roadsByName = new Map(source.streets.map(street => [street.road.name, street.road.id]));
    for (const parcel of companionCityPlan.parcels) {
      if (parcel.id === civicTerrace.parcelId || parcel.id === architectureDelivery.exterior.parcelId
        || signatureBuildingParcelIds.has(parcel.id) || residentialUtilities.servedParcelIds.has(parcel.id)) continue;
      const anchor = companionCityCableAnchor(parcel);
      if (!anchor) continue;
      let wire: CityStreetWire | undefined, nearest = 36;
      const c = Math.cos(parcel.yaw), s = Math.sin(parcel.yaw);
      const front = (anchor[0] - parcel.x) * s + (anchor[2] - parcel.z) * c;
      for (const candidate of poles) {
        if (candidate.roadId !== roadsByName.get(parcel.street)) continue;
        const distance = Math.hypot(anchor[0] - candidate.x, anchor[2] - candidate.z);
        if (distance >= nearest || (candidate.x - parcel.x) * s + (candidate.z - parcel.z) * c < front + .2) continue;
        const drop: CityStreetWire = { id: `street:drop:${parcel.id}`, roadId: candidate.roadId,
          from: cityUtilityPolePorts(candidate).phases[3], to: anchor, strands: 1, diameter: .035,
          sag: Math.min(.65 + distance * .015, Math.min(anchor[1], candidate.height - .62) - 5.2) };
        if (anchor[1] < 5.8 || crossesBuilding(drop, [parcel.id])) continue;
        wire = drop; nearest = distance;
      }
      if (!wire) continue;
      // Keep traffic headroom and approach the target facade without crossing another plot.
      tileAt((wire.from[0] + wire.to[0]) / 2, (wire.from[2] + wire.to[2]) / 2).wires.push(wire);
    }
  }
  if (detailCollision) for (const light of additionalLights) tileAt(light.x, light.z).fixtures.push(light);
  for (const parcel of companionCityPlan.parcels) for (const light of companionCityResidentialLights(parcel))
    tileAt(light.x, light.z).residentialLights.push(light);
  // Only fixtures admitted to this frozen city's visible geometry can illuminate it.
  for (const tile of tiles.values()) {
    for (const fixture of tile.fixtures) tile.emitters.push(...companionCityStreetLightEmitters(fixture));
    for (const light of tile.residentialLights) tile.emitters.push(companionCityResidentialLightEmitter(light));
  }
  for (const entrance of companionCityEntranceConnections(companionCityPlan.parcels, walkingRoads)) {
    if ([-entrance.width / 2, 0, entrance.width / 2].some(offset =>
      crossingAt(entrance.road, entrance.distance + offset, entrance.side) === "junction")) continue;
    tileAt((entrance.gate[0] + entrance.edge[0]) / 2, (entrance.gate[1] + entrance.edge[1]) / 2).entrances.push(entrance);
  }

  const cube = new THREE.BoxGeometry(1, 1, 1);
  const cylinder = new THREE.CylinderGeometry(.5, .5, 1, 8);
  const taper = new THREE.CylinderGeometry(.35, .5, 1, 10);
  const cableLoop = new THREE.TorusGeometry(.5, .035, 4, 12);
  const signPlane = new THREE.PlaneGeometry(1, 1);
  const masonry = new THREE.MeshStandardMaterial({ color: "#ffffff", roughness: .93 });
  const steel = new THREE.MeshStandardMaterial({ color: "#ffffff", roughness: .66, metalness: .24 });
  const flatMaterial = new THREE.MeshStandardMaterial({ vertexColors: true, roughness: .93 });
  const siltMaterial = new THREE.MeshStandardMaterial({ vertexColors: true, roughness: .98, alphaTest: .26 });
  siltMaterial.onBeforeCompile = shader => {
    shader.vertexShader = `varying vec3 siltPoint;\n${shader.vertexShader}`
      .replace("#include <begin_vertex>", "#include <begin_vertex>\nsiltPoint=(modelMatrix*vec4(transformed,1.)).xyz;");
    shader.fragmentShader = `varying vec3 siltPoint;\n${shader.fragmentShader}`
      .replace("#include <color_fragment>", `#include <color_fragment>
        vec2 siltCells=siltPoint.xz*42.;
        float siltVisible=1.-smoothstep(.35,1.,max(fwidth(siltCells).x,fwidth(siltCells).y));
        float siltGrain=mix(.5,fract(sin(dot(floor(siltCells),vec2(127.1,311.7)))*43758.5453),siltVisible);
        diffuseColor.a=clamp(diffuseColor.a+(siltGrain-.5)*.38,0.,1.);
        diffuseColor.rgb*=.92+siltGrain*.15;`);
  };
  siltMaterial.customProgramCacheKey = () => "city-noisy-silt-v2";
  const drainMaterial = new THREE.MeshStandardMaterial({ vertexColors: true, roughness: .75 });
  const drainDaylight = { value: 1 };
  drainMaterial.onBeforeCompile = shader => {
    shader.uniforms.drainDaylight = drainDaylight;
    shader.vertexShader = `varying vec3 drainPoint;\n${shader.vertexShader}`
      .replace("#include <begin_vertex>", "#include <begin_vertex>\ndrainPoint=(modelMatrix*vec4(transformed,1.)).xyz;");
    shader.fragmentShader = `uniform float drainDaylight; varying vec3 drainPoint;\n${shader.fragmentShader}`
      .replace("#include <roughnessmap_fragment>", `#include <roughnessmap_fragment>
        float drainWater=1.-smoothstep(.18,.22,max(diffuseColor.r,max(diffuseColor.g,diffuseColor.b)));
        roughnessFactor=mix(roughnessFactor,.24,drainWater);`)
      .replace("#include <emissivemap_fragment>", `#include <emissivemap_fragment>
        float drainFresnel=pow(1.-abs(normalize(cameraPosition-drainPoint).y),5.);
        totalEmissiveRadiance+=vec3(.08,.12,.14)*drainWater*drainFresnel*drainDaylight;`);
  };
  drainMaterial.customProgramCacheKey = () => "city-damp-drain-v1";
  const glow = new THREE.MeshStandardMaterial({ color: "#e3dcc1", roughness: .45, emissive: "#efd9a5", emissiveIntensity: 0 });
  // Share one fixed pool across existing street lamps and residential fixtures.
  // Distant bulbs retain instanced emission; no per-fixture lights or shadow maps.
  const practicals = Array.from({ length: 8 }, () => {
    const light = new THREE.PointLight("#f2dbab", 0, 28, 2); light.castShadow = false;
    light.visible = false; group.add(light); return light;
  });
  const signMaterials = new Map<string, { material: THREE.MeshStandardMaterial; texture: THREE.CanvasTexture | null; users: number }>();
  const colour = new THREE.Color(), matrix = new THREE.Matrix4(), position = new THREE.Vector3();
  const scale = new THREE.Vector3(), rotation = new THREE.Quaternion(), up = new THREE.Vector3(0, 1, 0);
  const orientation = new THREE.Euler(0, 0, 0, "YXZ");
  let previousGlow = 0, disposed = false, lastX = Infinity, lastZ = Infinity;
  const nearbyPracticals: CityLightEmitter[] = [];
  const practicalDistances = new Float64Array(practicals.length);
  let lightX = Infinity, lightZ = Infinity;
  let scatterRoad: CityStreetRoad | undefined;
  let compoundParcel: typeof companionCityPlan.parcels[number] | undefined;
  let compoundInfill: readonly CityArchitecturePart[] = [];
  const inCompoundReserve = (x: number, z: number, radius: number) => {
    if (!compoundParcel) return false;
    const dx = x - compoundParcel.x, dz = z - compoundParcel.z;
    const localX = dx * Math.cos(compoundParcel.yaw) - dz * Math.sin(compoundParcel.yaw);
    const localZ = dx * Math.sin(compoundParcel.yaw) + dz * Math.cos(compoundParcel.yaw);
    return Math.abs(localX) + radius < compoundParcel.width / 2 + compoundParcel.garden - .2
      && Math.abs(localZ) + radius < compoundParcel.depth / 2 - .25
      && !compoundInfill.some(part => Math.abs(localX - part.x) < part.w / 2 + radius + .15
        && Math.abs(localZ - part.z) < part.d / 2 + radius + .15);
  };
  const scatter = new LagosScatter({ seed: cityStreetSeed("lagos-street-scatter-v1"), capacity: 8192,
    accept: placement => compoundParcel ? inCompoundReserve(placement.point.x, placement.point.z, placement.radius)
      : !!scatterRoad && placement.along > 12
        && [placement.along - placement.radius, placement.along, placement.along + placement.radius]
          .every(distance => !crossingAt(scatterRoad!, distance, Math.sign(placement.lateral))),
    allowSolid: placement => inCompoundReserve(placement.point.x, placement.point.z, .65) });
  scatter.init(); group.add(scatter.group);
  for (const street of source.streets) {
    if (street.family !== "market" && street.family !== "service") continue;
    scatterRoad = street.road;
    scatter.spawnScatterAlongPath(street.road.points.map(point => new THREE.Vector3(point[0], -.012, point[1])), street.road.width);
  }
  scatterRoad = undefined;
  for (const parcel of companionCityPlan.parcels) {
    if (parcel.district !== "south-market" || parcel.garden < 1.5) continue;
    compoundParcel = parcel;
    // Scatter stays inside the already blocked plot, beside an actual compound wall.
    // It neither narrows the public footway nor creates new collision geometry.
    const parts = companionCityArchitectureParts(parcel);
    if (!parts.some(part => part.role === "compound side wall")) continue;
    compoundInfill = parts.filter(part => part.role.startsWith("infill ") && part.h >= .12
      && part.y - part.h / 2 < 1.5 && !part.rotation);
    const inside = -parcel.width / 2 - parcel.garden + .75;
    const a = companionCityParcelPoint(parcel, inside, -parcel.depth / 2 + 1.4);
    const b = companionCityParcelPoint(parcel, inside, parcel.depth / 2 - 1.4);
    scatter.spawnScatterAlongPath([new THREE.Vector3(a[0], 0, a[2]), new THREE.Vector3(b[0], 0, b[2])], .8);
  }
  compoundParcel = undefined;
  compoundInfill = [];
  // All paths are appended synchronously. Upload flags and bounds need only one
  // final pass over the completed scatter, before this factory exposes its group.
  scatter.update(0);

  const batches = (root: THREE.Group, parts: Part[], material: THREE.MeshStandardMaterial, geometry: THREE.BufferGeometry, castShadow: boolean) => {
    if (!parts.length) return;
    const mesh = new THREE.InstancedMesh(geometry, material, parts.length);
    // All placement lives in instance buffers. Parent/world updates stay enabled.
    mesh.matrixAutoUpdate = false;
    parts.forEach((value, index) => {
      position.set(value.x, value.y, value.z);
      rotation.setFromEuler(orientation.set(value.pitch ?? 0, value.yaw, value.roll ?? 0, "YXZ"));
      matrix.compose(position, rotation, scale.set(value.w, value.h, value.d));
      mesh.setMatrixAt(index, matrix); mesh.setColorAt(index, colour.set(value.colour));
    });
    mesh.instanceMatrix.needsUpdate = true;
    if (mesh.instanceColor) mesh.instanceColor.needsUpdate = true;
    mesh.computeBoundingSphere(); mesh.receiveShadow = false;
    mesh.userData.cityStreetCastsShadow = false; mesh.castShadow = false;
    root.add(mesh);
  };
  // Surface helpers consume this scratch synchronously. Construction and winding
  // keep full number precision; only completed output values enter Float32 chunks.
  const surfacePoints = new Float64Array(12);
  const quadIndices = [0, 2, 1, 1, 2, 3] as const;
  const makeSurface = (colourSize: 3 | 4 = 3): FlatSurface => ({ chunks: [], count: 0, colourSize });
  const vertex = (surface: FlatSurface, x: number, y: number, z: number, tint: THREE.Color, alpha = 1) => {
    let chunk = surface.chunks[surface.chunks.length - 1];
    if (!chunk || chunk.count === SURFACE_CHUNK_VERTICES) {
      chunk = { positions: new Float32Array(SURFACE_CHUNK_VERTICES * 3),
        colours: new Float32Array(SURFACE_CHUNK_VERTICES * surface.colourSize), count: 0 };
      surface.chunks.push(chunk);
    }
    const positionOffset = chunk.count * 3, colourOffset = chunk.count * surface.colourSize;
    chunk.positions[positionOffset] = x; chunk.positions[positionOffset + 1] = y; chunk.positions[positionOffset + 2] = z;
    chunk.colours[colourOffset] = tint.r; chunk.colours[colourOffset + 1] = tint.g; chunk.colours[colourOffset + 2] = tint.b;
    if (surface.colourSize === 4) chunk.colours[colourOffset + 3] = alpha;
    chunk.count++; surface.count++;
  };
  const strip = (surface: FlatSurface, road: CityStreetRoad, start: number, end: number, inside: number, outside: number, y: number, tint: string, fadeInside = false) => {
    const a = sampleCityStreet(road, start), b = sampleCityStreet(road, end);
    surfacePoints[0] = a.x + a.nx * inside; surfacePoints[1] = a.z + a.nz * inside;
    surfacePoints[2] = a.x + a.nx * outside; surfacePoints[3] = a.z + a.nz * outside;
    surfacePoints[4] = b.x + b.nx * inside; surfacePoints[5] = b.z + b.nz * inside;
    surfacePoints[6] = b.x + b.nx * outside; surfacePoints[7] = b.z + b.nz * outside;
    colour.set(tint);
    // Horizontal triangles face upward on both sides of a road.
    for (let triangle = 0; triangle < quadIndices.length; triangle += 3) {
      const p = quadIndices[triangle] * 2, q = quadIndices[triangle + 1] * 2, r = quadIndices[triangle + 2] * 2;
      const reverse = (surfacePoints[q + 1] - surfacePoints[p + 1]) * (surfacePoints[r] - surfacePoints[p])
        - (surfacePoints[q] - surfacePoints[p]) * (surfacePoints[r + 1] - surfacePoints[p + 1]) < 0;
      for (let corner = 0; corner < 3; corner++) {
        const index = quadIndices[triangle + (reverse ? 2 - corner : corner)];
        vertex(surface, surfacePoints[index * 2], y, surfacePoints[index * 2 + 1], colour, fadeInside && index % 2 === 0 ? 0 : 1);
      }
    }
  };
  const plane = (surface: FlatSurface, point: CityStreetSample, along: number, across: number, length: number, width: number, y: number, tint: string) => {
    surfacePoints[0] = along - length / 2; surfacePoints[1] = across - width / 2;
    surfacePoints[2] = along + length / 2; surfacePoints[3] = across - width / 2;
    surfacePoints[4] = along - length / 2; surfacePoints[5] = across + width / 2;
    surfacePoints[6] = along + length / 2; surfacePoints[7] = across + width / 2;
    colour.set(tint);
    for (const index of quadIndices) {
      const x = surfacePoints[index * 2], z = surfacePoints[index * 2 + 1];
      vertex(surface, point.x + point.tx * x + point.nx * z, y, point.z + point.tz * x + point.nz * z, colour);
    }
  };
  const surfaceMesh = (tile: Tile, root: THREE.Group, surfaces: readonly FlatSurface[], material: THREE.MeshStandardMaterial) => {
    let count = 0;
    for (const surface of surfaces) count += surface.count;
    if (!count) return;
    const colourSize = surfaces[0].colourSize;
    const positions = new Float32Array(count * 3), colours = new Float32Array(count * colourSize);
    let offset = 0;
    // Copy each stream once in submission order. Kerbs never need a second
    // whole-surface staging array, and partial chunks leave no spare GPU capacity.
    for (const surface of surfaces) {
      for (const chunk of surface.chunks) {
        const full = chunk.count === SURFACE_CHUNK_VERTICES;
        positions.set(full ? chunk.positions : chunk.positions.subarray(0, chunk.count * 3), offset * 3);
        colours.set(full ? chunk.colours : chunk.colours.subarray(0, chunk.count * colourSize), offset * colourSize);
        offset += chunk.count;
      }
      surface.chunks.length = 0; surface.count = 0;
    }
    const geometry = new THREE.BufferGeometry();
    geometry.setAttribute("position", new THREE.BufferAttribute(positions, 3));
    geometry.setAttribute("color", new THREE.BufferAttribute(colours, colourSize));
    geometry.computeVertexNormals(); geometry.computeBoundingSphere();
    tile.geometries.push(geometry);
    const mesh = new THREE.Mesh(geometry, material); mesh.matrixAutoUpdate = false; mesh.receiveShadow = true; root.add(mesh);
  };

  const signMaterial = (name: string) => {
    const existing = signMaterials.get(name);
    if (existing) { existing.users++; return existing.material; }
    let map: THREE.CanvasTexture | null = null;
    if (typeof document !== "undefined") {
      const football = name === "FOOTBALL PITCH";
      const canvas = document.createElement("canvas"); canvas.width = football ? 224 : 512; canvas.height = football ? 384 : 128;
      const context = canvas.getContext("2d");
      if (context) {
        context.fillStyle = "#244b42"; context.fillRect(0, 0, canvas.width, canvas.height);
        context.strokeStyle = "#d8dfcf"; context.lineWidth = 3; context.strokeRect(9, 9, canvas.width - 18, canvas.height - 18);
        context.fillStyle = "#f0efdc"; context.textAlign = "center"; context.textBaseline = "middle";
        if (football) {
          // Original ball graphic, no club crest or copied branding.
          context.beginPath(); context.arc(112, 113, 52, 0, Math.PI * 2); context.fill();
          context.fillStyle = "#244b42"; context.beginPath();
          for (let i = 0; i < 5; i++) {
            const angle = -Math.PI / 2 + i * Math.PI * .4;
            const x = 112 + Math.cos(angle) * 23, y = 113 + Math.sin(angle) * 23;
            if (i === 0) context.moveTo(x, y); else context.lineTo(x, y);
          }
          context.closePath(); context.fill();
          context.fillStyle = "#f0efdc"; context.font = "bold 27px sans-serif";
          context.fillText("FOOTBALL", 112, 217, 188); context.fillText("PITCH", 112, 257, 188);
          context.font = "22px sans-serif"; context.fillText("NEARBY", 112, 319, 188);
        } else {
          context.font = `${name.length > 22 ? 28 : 34}px sans-serif`;
          context.fillText(name, 256, 64, 464);
        }
        map = new THREE.CanvasTexture(canvas); map.colorSpace = THREE.SRGBColorSpace;
        map.generateMipmaps = true;
      }
    }
    const material = new THREE.MeshStandardMaterial({ color: map ? "#ffffff" : "#244b42", map, roughness: .77 });
    signMaterials.set(name, { material, texture: map, users: 1 }); return material;
  };

  const addFixture = (fixture: CityStreetFixture, solid: Part[], poles: Part[], metal: Part[], bulbs: Part[],
    loops: Part[], cables: Part[], signs: Map<string, Part[]>) => {
    const at = (x: number, y: number, z: number, w: number, h: number, d: number, tint: string): Part => {
      const point = cityUtilityPolePoint(fixture, x, y, z);
      return { x: point[0], y: point[1], z: point[2], w, h, d, yaw: fixture.yaw,
        pitch: fixture.utility?.pitch, roll: fixture.utility?.roll, colour: tint };
    };
    if (fixture.kind !== "wall-sign") {
      const utility = fixture.kind === "utility", height = fixture.height;
      const finish = utility ? "#a5a18c" : fixture.style === "lantern" ? "#505e52" : "#7d8780";
      if (utility && fixture.utility) {
        for (const part of cityUtilityPoleParts(fixture)) {
          const batch = part.geometry === "pole" ? poles : part.geometry === "loop" ? loops : part.geometry === "cable" ? cables : solid;
          batch.push(part);
        }
      } else {
        poles.push(at(0, height / 2, 0, utility ? .26 : .17, height, utility ? .26 : .17, finish));
        solid.push(at(0, .18, 0, utility ? .38 : .3, .36, utility ? .38 : .3, "#aaa795"));
      }
      if (utility && !fixture.utility) {
        metal.push(at(0, height - .6, 0, 1.12, .09, .1, "#62685b"));
        for (const offset of [-.36, .36]) poles.push(at(offset, height - .48, 0, .1, .19, .1, "#d0cdb7"));
        metal.push(at(0, 1.8, -.15, .22, .42, .08, "#737b70"));
        // Small cable coils stay on the existing pole, above pedestrian headroom.
        for (let coil = 0; coil < 2 + fixture.variant % 2; coil++)
          loops.push(at((coil - 1) * .12, height - .9 - coil * .08, -.16,
            .34 + coil * .06, .49 + coil * .07, .14, "#292e2b"));
      }
      const lantern = fixture.style === "lantern" || fixture.kind === "compound-lamp";
      if (lantern) {
        solid.push(at(0, height + .23, 0, .42, .09, .42, "#515c50"));
        metal.push(at(0, height - .15, 0, .36, .055, .36, "#465247"));
        bulbs.push(at(0, height + .03, 0, .27, .29, .27, "#ece5ce"));
        for (const x of [-.15, .15]) for (const z of [-.15, .15]) metal.push(at(x, height + .03, z, .025, .31, .025, "#465247"));
      } else {
        const sides = fixture.style === "twin-arm" ? [-1, 1] : [-fixture.side];
        const lightY = utility ? height - (fixture.utility ? 2.6 : 1.1) : height;
        for (const side of sides) {
          const reach = fixture.style === "quay" ? 1.15 : utility ? fixture.utility ? .95 : .72 : 1.5;
          metal.push(at(0, lightY - .14, side * reach * .5, .065, .08, reach, "#6c756a"));
          solid.push(at(0, lightY - .18, side * reach, .34, .12, .62, "#65726a"));
          bulbs.push(at(0, lightY - .245, side * reach, .27, .015, .48, "#eee4c8"));
        }
      }
      if (fixture.kind !== "compound-lamp" && !fixture.utility) {
        // Bolted inspection cover. It fits on the column and never adds an obstacle.
        metal.push(at(0, .8, -.095, .085, .28, .018, "#737e70"));
      }
      if (fixture.kind === "lamp" && fixture.height >= 7 && companionCityFootballStreetBanner(fixture.x, fixture.z)) {
        // Existing admitted columns only, above pedestrian and vehicle height.
        const y = 5.55, z = -.16;
        metal.push(at(0, y + .68, z, .8, .04, .12, "#53695a"));
        metal.push(at(0, y - .68, z, .8, .04, .12, "#53695a"));
        const name = "FOOTBALL PITCH", plates = signs.get(name) ?? [];
        plates.push(at(0, y, z + .031, .73, 1.28, .002, "#ffffff"));
        plates.push({ ...at(0, y, z - .031, .73, 1.28, .002, "#ffffff"), yaw: fixture.yaw + Math.PI });
        signs.set(name, plates);
      }
    }
    if (fixture.streetName) {
      const y = fixture.kind === "wall-sign" ? fixture.height : fixture.utility ? 5.0 : 3.1;
      const z = fixture.kind === "wall-sign" ? 0 : -.11;
      solid.push(at(0, y, z, 2.3, .59, .045, "#53695a"));
      let plates = signs.get(fixture.streetName);
      if (!plates) { plates = []; signs.set(fixture.streetName, plates); }
      // Two readable faces on column signs; wall plates have one outward face.
      plates.push(at(0, y, z + .026, 2.23, .55, .002, "#ffffff"));
      if (fixture.kind !== "wall-sign") plates.push({ ...at(0, y, z - .026, 2.23, .55, .002, "#ffffff"), yaw: fixture.yaw + Math.PI });
    }
  };

  const buildTile = (tile: Tile) => {
    const root = new THREE.Group(); root.name = `Street detail ${tile.x},${tile.z}`; root.matrixAutoUpdate = false;
    const concrete: Part[] = [], metal: Part[] = [], poles: Part[] = [], bulbs: Part[] = [], loops: Part[] = [], cables: Part[] = [];
    const surface = makeSurface(), drains = makeSurface(), kerbs = makeSurface(), silt = makeSurface(4);
    const signs = new Map<string, Part[]>();
    for (const entrance of tile.entrances) {
      const [gx, gz] = entrance.gate, [ex, ez] = entrance.edge;
      const length = Math.hypot(gx - ex, gz - ez), tx = (gx - ex) / length, tz = (gz - ez) / length;
      const point: CityStreetSample = { x: (gx + ex) / 2, z: (gz + ez) / 2, tx, tz, nx: -tz, nz: tx, yaw: Math.atan2(-tz, tx) };
      // Concrete approaches align with actual gates. Public movement remains at its saved level.
      plane(surface, point, 0, 0, length, entrance.width, -.0035, entrance.timber ? "#ac9d81" : "#b7b2a0");
      for (const side of [-1, 1]) plane(surface, point, 0, side * (entrance.width / 2 - .07),
        length, .075, -.0025, "#8d8876");
      for (let d = 1.4; d < length - .4; d += 2.4)
        plane(surface, point, d - length / 2, 0, .021, entrance.width - .12, -.002, "#8d8d7b");
      if (entrance.timber) {
        const roadPoint = sampleCityStreet(entrance.road, entrance.distance);
        const across = entrance.side * (entrance.road.width / 2 + .28);
        for (let plank = -entrance.width / 2 + .11; plank < entrance.width / 2; plank += .23) {
          plane(surface, roadPoint, plank, across, .205, .7, -.0015, plank < 0 ? "#8d7858" : "#a08a65");
          for (const end of [-1, 1]) plane(surface, roadPoint, plank, across + end * .25, .018, .018, -.0005, "#565c4e");
        }
      }
    }
    for (const segment of tile.segments) {
      const { road, start, end } = segment;
      const profile = cityStreetProfile(road, sampleCityStreet(road, (start + end) / 2));
      const seed = cityStreetSeed(road.id);
      const streetLife = profile.family === "market" || profile.family === "service";
      if (streetLife) for (const side of [-1, 1]) {
        for (let d = Math.ceil(start / 7) * 7; d < end; d += 7) {
          if (d < 8 || crossingAt(road, d, side) === "junction") continue;
          const point = sampleCityStreet(road, d), pressure = activityAt(point.x, point.z);
          const seed = cityStreetSeed(`${road.id}:silt:${side}:${Math.floor(d)}`);
          const length = Math.min(5.5, end - d), width = .22 + pressure * .42;
          if (length < .5 || seed % 5 === 0) continue;
          // Irregular silt tongues blend over asphalt instead of a ruler-straight soil seam.
          for (let span = 0; span < 5; span++) {
            const a = d + length * span / 5, b = d + length * (span + 1) / 5;
            const variation = cityStreetSeed(`${seed}:${span}`) % 101 / 100;
            strip(silt, road, a, b, side * (road.width / 2 - width * (.35 + variation)),
              side * (road.width / 2 + .09), -.003, span % 2 ? "#9a7654" : "#ac865d", true);
          }
        }
      }
      if (extensionRoadIds.has(road.id) && road.width >= 14) {
        for (let d = Math.ceil(start / 16) * 16; d < end; d += 16) {
          if (d < 12 || crossingAt(road, d, 1) === "junction" || crossingAt(road, d, -1) === "junction") continue;
          const finish = Math.min(d + 5, end);
          plane(surface, sampleCityStreet(road, (d + finish) / 2), 0, 0, finish - d, .14, -.003, "#ded4ac");
        }
      }
      // Current flat-floor navigation admits columns, not raised kerbs. Keep these construction finishes flush.
      for (const span of cityStreetConstructionSpans(road, start, end, crossingAt)) {
        for (let d = span.start; d < span.end; d += 1.6) {
          const next = Math.min(d + 1.56, span.end), side = span.side;
          const tint = profile.paintedKerb ? Math.floor(d / 3.2) % 2 ? "#dad6c4" : "#353c35" : profile.kerb;
          const inside = side * (road.width / 2 + .5), outside = side * (road.width / 2 + .7);
          strip(kerbs, road, d, next, inside, outside, -.001, tint);
          if (profile.drain === "open") {
            // Flush drain-edge finish. The base floor remains intact.
            strip(kerbs, road, d, next, side * (road.width / 2 + .02), side * (road.width / 2 + .065), -.001, "#b6b49f");
          }
        }
      }
      for (const side of [-1, 1]) {
        // Narrow construction seams and covered channels remain at the existing locomotion level.
        strip(surface, road, start, end, side * (road.width / 2 + road.pavement - .12), side * (road.width / 2 + road.pavement - .09), -.009, "#8c8878");
        const drainInner = side * (road.width / 2 + .06), drainOuter = side * (road.width / 2 + .48);
        strip(drains, road, start, end, drainInner, drainOuter, -.009,
          profile.drain === "open" ? "#5e6253" : "#969989");
        for (let d = Math.ceil(start / 1.6) * 1.6; d < end; d += 1.6) {
          const next = Math.min(d + 1.52, end), point = sampleCityStreet(road, (d + next) / 2);
          const crossing = crossingAt(road, (d + next) / 2, side);
          const edge = side * (road.width / 2 + .53);
          if (crossing) {
            // Flush slabs span the drain at every existing entrance and connected path.
            strip(surface, road, d, next, side * road.width / 2, side * (road.width / 2 + .68), -.005,
              crossing === "driveway" ? "#b8b2a0" : profile.paving);
            continue;
          }
          const paint = profile.paintedKerb && Math.floor(d / 3.2) % 2 === 0 ? "#353c35"
            : profile.paintedKerb ? "#dad6c4" : profile.kerb;
          // Flush painted seam follows the shared construction runs.
          strip(surface, road, d, next, edge, edge + side * .18, -.004, paint);
          strip(surface, road, d, next, side * (road.width / 2 + .025), side * (road.width / 2 + .065), -.003, "#c0beaa");
          if (profile.drain === "covered") {
            plane(surface, point, 0, side * (road.width / 2 + .27), .025, .4, -.004, "#666e60");
          } else if (streetLife && next - d > .12 && cityStreetSeed(`${road.id}:slab:${side}:${Math.floor(d)}`) % 7 === 0) {
            // Intermittent covers are flush. They do not introduce a new walking level.
            plane(surface, point, 0, side * (road.width / 2 + .27), next - d - .08, .42, -.004, "#a49c87");
            plane(surface, point, .17, side * (road.width / 2 + .27), .025, .25, -.003, "#6c6d5b");
          }
          if (profile.drain === "open" && streetLife && cityStreetSeed(`${road.id}:wet:${side}:${Math.floor(d)}`) % 4 === 0) {
            plane(drains, point, 0, side * (road.width / 2 + .27), Math.max(.1, next-d-.13), .27, -.002, "#394c40");
            for (let leaf = 0; leaf < 2; leaf++) plane(surface, point, (leaf-.5)*.38,
              side * (road.width / 2 + .28), .11, .045, -.001, leaf ? "#877d55" : "#657547");
          }
        }
        for (let d = Math.ceil(start / profile.jointSpacing) * profile.jointSpacing; d < end; d += profile.jointSpacing) {
          const point = sampleCityStreet(road, d), crossing = crossingAt(road, d, side);
          if (crossing === "junction") continue;
          plane(surface, point, 0, side * (road.width / 2 + (road.pavement + .72) / 2), .024,
            Math.max(.4, road.pavement - .78), -.005, "#918e7e");
          // Offset paving courses show individual slabs on maintained footways.
          // These are flush surface joints, not raised walkable geometry.
          if (!streetLife && road.kind !== "bridge" && road.pavement > 1.8 && !crossing) {
            const next = Math.min(d + profile.jointSpacing, end);
            const middle = side * (road.width / 2 + .7 + (road.pavement - .7) / 2);
            strip(surface, road, d, next, middle, middle + side * .021, -.004, "#a1a08e");
            plane(surface, sampleCityStreet(road, (d + next) / 2), 0,
              side * (road.width / 2 + .7 + (road.pavement - .7) * .75), .018,
              (road.pavement - .7) * .5, -.004, "#a1a08e");
          }
        }
        // Access-cover frames and lifting slots add service detail without
        // occupying a route or changing its floor/collision revision.
        for (let d = Math.ceil((start - seed % 23) / 57) * 57 + seed % 23; d < end; d += 57) {
          if (d < Math.max(start, 12) || road.pavement < 1.6 || crossingAt(road, d, side)) continue;
          const point = sampleCityStreet(road, d), across = side * (road.width / 2 + road.pavement * .67);
          plane(surface, point, 0, across, .79, .63, -.003, "#687164");
          plane(surface, point, 0, across, .69, .53, -.002, "#a9ad9b");
          for (const lift of [-.22, .22]) plane(surface, point, lift, across, .035, .12, -.001, "#535f51");
        }
        // Small inset drain grates, rather than raised boxes on the walking route.
        for (let d = Math.ceil((start - seed % 19) / 31) * 31 + seed % 19; d < end; d += 31) {
          if (d < start || crossingAt(road, d, side)) continue;
          const point = sampleCityStreet(road, d), across = side * (road.width / 2 + .27);
          plane(drains, point, 0, across, .74, .43, -.003, "#555e51");
          for (let slot = -.28; slot <= .28; slot += .09) plane(surface, point, slot, across, .027, .36, -.002, "#a1a28f");
        }
      }
      if (road.kind === "bridge") {
        for (let d = Math.ceil(start / 44) * 44; d < end; d += 44) {
          plane(surface, sampleCityStreet(road, d), 0, 0, .16, road.width, -.005, "#374137");
          plane(surface, sampleCityStreet(road, d + .25), 0, 0, .045, road.width, -.004, "#a5aa98");
        }
      }
      if (profile.repairSpacing && !(roadTreatments && (companionCityRoadSurface(road.id) !== "asphalt" || companionCityDividedRoads.has(road.id)))) {
        const spacing = profile.repairSpacing, offset = seed % Math.floor(spacing * .65);
        for (let d = Math.ceil((start - offset) / spacing) * spacing + offset; d < end; d += spacing) {
          if (d < Math.max(12, start) || crossingAt(road, d, 1) === "junction" || crossingAt(road, d, -1) === "junction") continue;
          const point = sampleCityStreet(road, d), variant = cityStreetSeed(`${road.id}:repair:${Math.floor(d)}`);
          const across = (variant % 2 ? -1 : 1) * road.width * .27;
          const width = 1.1 + variant % 5 * .23, length = 2.4 + variant % 4 * .65;
          const tint = variant % 3 ? "#464e44" : "#777869";
          // Irregular repaired asphalt, not a navigable hole or a blanket of decay.
          surfacePoints[0] = -length / 2; surfacePoints[1] = -width * .36;
          surfacePoints[2] = -length * .28; surfacePoints[3] = -width * .5;
          surfacePoints[4] = length * .38; surfacePoints[5] = -width * .43;
          surfacePoints[6] = length / 2; surfacePoints[7] = -width * .08;
          surfacePoints[8] = length * .34; surfacePoints[9] = width * .45;
          surfacePoints[10] = -length * .41; surfacePoints[11] = width * .5;
          colour.set(tint);
          for (let i = 0; i < 6; i++) {
            const a = ((i + 1) % 6) * 2, b = i * 2;
            for (let corner = 0; corner < 3; corner++) {
              const x = corner === 0 ? 0 : surfacePoints[corner === 1 ? a : b];
              const z = corner === 0 ? 0 : surfacePoints[(corner === 1 ? a : b) + 1];
              vertex(surface, point.x + point.tx * x + point.nx * (across + z), -.004,
                point.z + point.tz * x + point.nz * (across + z), colour);
            }
          }
          for (const side of [-1, 1]) plane(surface, point, side * length * .27, across + side * width * .22, .35, .02, -.003, "#8a8976");
          if (streetLife && variant % 3 === 0) for (let chip = 0; chip < 9; chip++) {
            const grain = cityStreetSeed(`${road.id}:gravel:${Math.floor(d)}:${chip}`);
            plane(surface, point, (grain % 101 / 100 - .5) * length * .5,
              across + (Math.floor(grain / 101) % 101 / 100 - .5) * width * .45,
              .06 + grain % 3 * .025, .045, -.002, chip % 2 ? "#8d8a75" : "#b6ac92");
          }
        }
      }
    }
    for (const fixture of tile.fixtures) addFixture(fixture, concrete, poles, metal, bulbs, loops, cables, signs);
    for (const anchor of tile.utilityAnchors) {
      const at = (outward: number, y: number, w: number, h: number, d: number, tint: string): Part => ({
        x: anchor.x + Math.sin(anchor.yaw) * outward, y: anchor.y + y,
        z: anchor.z + Math.cos(anchor.yaw) * outward, w, h, d, yaw: anchor.yaw, colour: tint,
      });
      metal.push(at(-.15, 0, .18, .36, .065, "#62685b"));
      metal.push(at(-.07, -.09, .08, .06, .22, "#62685b"));
      poles.push(at(0, .07, .1, .23, .1, "#d0cdb7"));
    }
    for (const light of tile.residentialLights) {
      const c = Math.cos(light.yaw), s = Math.sin(light.yaw);
      const at = (x: number, y: number, z: number, w: number, h: number, d: number, tint: string): Part => ({
        x: light.x + x * c + z * s, y: light.y + y, z: light.z - x * s + z * c,
        w, h, d, yaw: light.yaw, colour: tint,
      });
      if (light.kind === "canopy") {
        metal.push(at(0, 0, 0, .46, .1, .34, "#48544f"));
        bulbs.push(at(0, -.059, 0, .36, .025, .25, "#fff0cc"));
      } else if (light.kind === "sconce") {
        metal.push(at(0, 0, 0, .22, .46, .09, "#46534c"));
        metal.push(at(0, .23, .06, .29, .065, .25, "#46534c"));
        bulbs.push(at(0, -.015, .072, .17, .32, .065, "#ffe3b1"));
      } else {
        metal.push(at(0, -.17, 0, .22, .06, .22, "#4c5a50"));
        metal.push(at(0, .17, 0, .23, .06, .23, "#4c5a50"));
        bulbs.push(at(0, 0, 0, .16, .28, .16, "#ffe3b1"));
        for (const side of [-1, 1]) metal.push(at(side * .09, 0, 0, .018, .28, .19, "#4c5a50"));
      }
    }
    batches(root, concrete, masonry, cube, true); batches(root, metal, steel, cube, true);
    batches(root, poles, masonry, taper, true); batches(root, bulbs, glow, cube, false);
    batches(root, loops, steel, cableLoop, false);
    batches(root, cables, steel, cylinder, false);
    for (const [name, plates] of signs) {
      tile.signs.push(name); batches(root, plates, signMaterial(name), signPlane, false);
    }
    if (tile.wires.length) {
      // Shared cylinder instances approximate hanging cables, without a separate animation loop.
      const from = new THREE.Vector3(), to = new THREE.Vector3(), direction = new THREE.Vector3();
      const mesh = new THREE.InstancedMesh(cylinder, steel, tile.wires.reduce((count, wire) => count + (wire.strands ?? 3) * 12, 0));
      mesh.matrixAutoUpdate = false;
      const point = (wire: CityStreetWire, strand: number, t: number, vector: THREE.Vector3) => vector.set(
        wire.from[0] + (wire.to[0] - wire.from[0]) * t,
        wire.from[1] + (wire.to[1] - wire.from[1]) * t + strand * .07 - 4 * wire.sag * (1 - strand * .12) * t * (1 - t),
        wire.from[2] + (wire.to[2] - wire.from[2]) * t);
      colour.set("#292e2b");
      let index = 0;
      for (const wire of tile.wires) for (let strand = 0; strand < (wire.strands ?? 3); strand++) for (let step = 0; step < 12; step++) {
        point(wire, strand, step / 12, from); point(wire, strand, (step + 1) / 12, to); direction.subVectors(to, from);
        const length = direction.length(); rotation.setFromUnitVectors(up, direction.normalize());
        position.copy(from).add(to).multiplyScalar(.5);
        matrix.compose(position, rotation, scale.set(wire.diameter ?? .023, length, wire.diameter ?? .023));
        mesh.setMatrixAt(index, matrix); mesh.setColorAt(index++, colour);
      }
      mesh.instanceMatrix.needsUpdate = true;
      if (mesh.instanceColor) mesh.instanceColor.needsUpdate = true;
      mesh.computeBoundingSphere(); mesh.name = "Overhead roadside utility wires"; root.add(mesh);
    }
    // Kerb paint and footway marks share one material and one submission per tile.
    surfaceMesh(tile, root, [drains], drainMaterial); surfaceMesh(tile, root, [surface, kerbs], flatMaterial);
    surfaceMesh(tile, root, [silt], siltMaterial);
    tile.root = root; group.add(root); residentTiles.add(tile);
  };
  const releaseTile = (tile: Tile) => {
    tile.root?.traverse(object => { if (object instanceof THREE.InstancedMesh) object.dispose(); });
    tile.root?.removeFromParent(); tile.root = undefined;
    residentTiles.delete(tile);
    for (const geometry of tile.geometries) geometry.dispose();
    tile.geometries.length = 0;
    for (const name of tile.signs) {
      const sign = signMaterials.get(name);
      if (sign && --sign.users === 0) {
        sign.material.dispose(); sign.texture?.dispose(); signMaterials.delete(name);
      }
    }
    tile.signs.length = 0;
  };

  return {
    group,
    update(target: THREE.Vector3, daylight: number): boolean {
      if (disposed || !Number.isFinite(target.x) || !Number.isFinite(target.z) || !Number.isFinite(daylight)) return false;
      let changed = scatter.setView(target);
      const moved = (target.x - lastX) ** 2 + (target.z - lastZ) ** 2 >= 8 ** 2;
      const level = Math.max(0, Math.min(1, daylight));
      scatter.setDaylight(level); drainDaylight.value = level;
      // Share the city facade's admitted mains factor. No independent outage owner.
      const power = Number.isFinite(mains.value) ? Math.max(0, Math.min(1, mains.value)) : 0;
      const emission = Math.round((1 - level) * .85 * power * 24) / 24;
      if (emission !== previousGlow) {
        glow.emissiveIntensity = emission; previousGlow = emission;
      }
      if (practicals.length) {
        if ((target.x - lightX) ** 2 + (target.z - lightZ) ** 2 >= 2 ** 2) {
          lightX = target.x; lightZ = target.z;
          nearbyPracticals.length = 0;
          const lowX = Math.floor((target.x - 65) / CELL_SIZE), highX = Math.floor((target.x + 65) / CELL_SIZE);
          const lowZ = Math.floor((target.z - 65) / CELL_SIZE), highZ = Math.floor((target.z + 65) / CELL_SIZE);
          for (let x = lowX; x <= highX; x++) for (let z = lowZ; z <= highZ; z++)
          for (const light of tiles.get(`${x}:${z}`)?.emitters ?? []) {
            const square = (light.x - target.x) ** 2 + (light.z - target.z) ** 2;
            if (!(square < 65 ** 2)) continue;
            let index = nearbyPracticals.length;
            // Strict insertion preserves the original stable sort's source-order ties.
            while (index > 0 && square < practicalDistances[index - 1]) index--;
            if (index === practicals.length) continue;
            for (let next = Math.min(nearbyPracticals.length, practicals.length - 1); next > index; next--) {
              nearbyPracticals[next] = nearbyPracticals[next - 1]; practicalDistances[next] = practicalDistances[next - 1];
            }
            nearbyPracticals[index] = light; practicalDistances[index] = square;
          }
        }
        practicals.forEach((light, index) => {
          const fixture = emission > 0 ? nearbyPracticals[index] : undefined;
          light.visible = !!fixture; light.intensity = fixture ? emission * fixture.intensity : 0;
          if (fixture) { light.position.set(fixture.x, fixture.y, fixture.z); light.distance = fixture.reach; }
        });
      }

      if (moved) {
        lastX = target.x; lastZ = target.z;
        for (const tile of residentTiles) {
          const square = (tile.x - target.x) ** 2 + (tile.z - target.z) ** 2;
          if (square > RELEASE_RADIUS ** 2) { releaseTile(tile); changed = true; }
        }
        // Only neighbouring keys can enter. Stationary frames do no city-wide tile scan.
        const lowX = Math.floor((target.x - LOAD_RADIUS) / CELL_SIZE), highX = Math.floor((target.x + LOAD_RADIUS) / CELL_SIZE);
        const lowZ = Math.floor((target.z - LOAD_RADIUS) / CELL_SIZE), highZ = Math.floor((target.z + LOAD_RADIUS) / CELL_SIZE);
        for (let x = lowX; x <= highX; x++) for (let z = lowZ; z <= highZ; z++) {
          const tile = tiles.get(`${x}:${z}`);
          if (!tile || tile.root) continue;
          const square = (tile.x - target.x) ** 2 + (tile.z - target.z) ** 2;
          if (square < LOAD_RADIUS ** 2) { buildTile(tile); changed = true; }
        }
      }
      return changed;
    },
    dispose(): void {
      if (disposed) return;
      disposed = true;
      scatter.dispose();
      for (const tile of residentTiles) releaseTile(tile);
      residentTiles.clear(); nearbyPracticals.length = 0;
      tiles.clear(); group.removeFromParent();
      for (const geometry of [cube, cylinder, taper, cableLoop, signPlane]) geometry.dispose();
      for (const material of [masonry, steel, flatMaterial, siltMaterial, drainMaterial, glow]) material.dispose();
      practicals.forEach(light => light.dispose());
      for (const sign of signMaterials.values()) { sign.material.dispose(); sign.texture?.dispose(); }
      signMaterials.clear();
    },
  };
}
