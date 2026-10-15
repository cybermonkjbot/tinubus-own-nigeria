import * as THREE from "three";
import { sameCompanionStreetCrashScope, validCompanionStreetVehiclePose, type CompanionStreetCrashScope }
  from "../../../../../packages/utils/src/companionStreetCrashes";
import type { CompanionStreetCrashTrafficView } from "../../../../../packages/utils/src/companionStreetCrashTraffic";
import { VehicleController } from "../../physics/VehicleController";
import { TrafficAI, type TrafficFrame } from "../../ai/TrafficAI";
import { createTownSquareRoadSurfaces } from "../../world/townSquareRoadSurfaces";
import { vehicleDynamicsPose } from "../../../../../packages/utils/src/companionVehicleDynamics";
import { captureObjectTransform, type ObjectTransformSnapshot } from "../scenes/objectTransformSnapshot";

type Participant = { x: number; z: number; radius: number };
type Pose = { x: number; z: number; yaw: number; tx: number; tz: number; turnIn: number; curvature: number };
type Wheel = {
  original: ObjectTransformSnapshot;
  radius: number;
  front: boolean;
  rollAxis: THREE.Vector3;
  steerAxis: THREE.Vector3;
  angle: number;
  index: number;
  x: number;
  z: number;
  suspensionAxis: THREE.Vector3;
};
type Vehicle = {
  id: string;
  original: ObjectTransformSnapshot;
  originalPose: Pose;
  pose: Pose;
  height: number;
  start: number;
  station: number;
  nominalSpeed: number;
  speed: number;
  steering: number;
  wheelbase: number;
  rearAxle: number;
  wheels: Wheel[];
  active: boolean;
  readonly halfWidth: number;
  readonly halfLength: number;
  readonly bodyRadius: number;
  managed: boolean;
  held: boolean;
  externallyPosed: boolean;
  streetSequence: number;
  controller: VehicleController;
  ai: TrafficAI;
  readonly trafficFrame: TrafficFrame;
  readonly leadSnapshot: TrafficFrame["vehicle"];
  lateral: number;
  hornSequence: number;
  impactSequence: number;
  originalHornSequence: unknown;
  originalImpactSequence: unknown;
};
type MaterialBinding = {
  mesh: THREE.Mesh;
  original: THREE.Material | THREE.Material[];
  moving: THREE.Material | THREE.Material[];
  originalDepth: THREE.Material | undefined;
  originalDistance: THREE.Material | undefined;
  depth: THREE.Material | undefined;
  distance: THREE.Material | undefined;
};

const CENTER_X = -10;
// Turn inside the existing six-metre cross streets, not at the main road's ends.
const SOUTH_Z = -27.5;
const NORTH_Z = 19.5;
const TURN_RADIUS = 2.1;
const STRAIGHT = NORTH_Z - SOUTH_Z;
const ARC = Math.PI * TURN_RADIUS;
const LAP = 2 * (STRAIGHT + ARC);
const TAU = Math.PI * 2;
// Legacy art fallback only. Selected models must supply reviewed world extents.
const HALF_WIDTH = 1.25;
const HALF_LENGTH = 2.6;
const FOLLOW_GAP = .8;
const CLEARANCE = .2;
const BRAKE = 2.8;
const ACCELERATION = .9;
const TURN_SPEED = .8;
const MAX_SPEED = 4;
const MAX_VEHICLES = 16;
const MAX_PARTICIPANTS = 64;
const MAX_WATER_MAPS = 8;
const MAX_FRAME = .1;
const MAX_STEP = 1 / 60;
const SWEEP_STEP = .2;
const UP = new THREE.Vector3(0, 1, 0);
const wrap = (value: number, length = LAP) => ((value % length) + length) % length;
const finite = (value: unknown): value is number => typeof value === "number" && Number.isFinite(value);
const blankPose = (): Pose => ({ x: 0, z: 0, yaw: 0, tx: 0, tz: 1, turnIn: 0, curvature: 0 });

/** Project cached static geometry bounds into the root's world-metre axes.
 * Eight corners per mesh, once at mount. No vertex sampling, shared-geometry
 * mutation or per-frame measurements. Unsupported art needs explicit bounds. */
function measuredVehicleBounds(root: THREE.Object3D, origin: THREE.Vector3, yaw: number): { halfWidth: number; halfLength: number } | null {
  let halfWidth = 0, halfLength = 0, unsupported = false, meshes = 0;
  const corner = new THREE.Vector3(), c = Math.cos(yaw), s = Math.sin(yaw);
  root.traverse(child => {
    if (!(child instanceof THREE.Mesh)) return;
    if (++meshes > 512 || child instanceof THREE.SkinnedMesh || child instanceof THREE.InstancedMesh || child instanceof THREE.BatchedMesh
      || child.geometry.morphAttributes.position?.length || !child.geometry.boundingBox) { unsupported = true; return; }
    const box = child.geometry.boundingBox;
    for (let index = 0; index < 8; index++) {
      corner.set(index & 1 ? box.max.x : box.min.x, index & 2 ? box.max.y : box.min.y, index & 4 ? box.max.z : box.min.z)
        .applyMatrix4(child.matrixWorld).sub(origin);
      if (!corner.toArray().every(Number.isFinite)) { unsupported = true; continue; }
      halfWidth = Math.max(halfWidth, Math.abs(corner.x * c - corner.z * s));
      halfLength = Math.max(halfLength, Math.abs(corner.x * s + corner.z * c));
    }
  });
  return unsupported || !meshes ? null : { halfWidth, halfLength };
}

function sampleRoute(station: number, result: Pose): Pose {
  let s = wrap(station);
  result.curvature = 0;
  if (s < STRAIGHT) {
    result.x = CENTER_X - TURN_RADIUS;
    result.z = SOUTH_Z + s;
    result.yaw = 0;
    result.turnIn = STRAIGHT - s;
  } else if ((s -= STRAIGHT) < ARC) {
    const angle = s / TURN_RADIUS;
    result.x = CENTER_X - TURN_RADIUS * Math.cos(angle);
    result.z = NORTH_Z + TURN_RADIUS * Math.sin(angle);
    result.yaw = angle;
    result.turnIn = 0;
    result.curvature = 1 / TURN_RADIUS;
  } else if ((s -= ARC) < STRAIGHT) {
    result.x = CENTER_X + TURN_RADIUS;
    result.z = NORTH_Z - s;
    result.yaw = Math.PI;
    result.turnIn = STRAIGHT - s;
  } else {
    const angle = (s - STRAIGHT) / TURN_RADIUS;
    result.x = CENTER_X + TURN_RADIUS * Math.cos(angle);
    result.z = SOUTH_Z - TURN_RADIUS * Math.sin(angle);
    result.yaw = Math.PI + angle;
    result.turnIn = 0;
    result.curvature = 1 / TURN_RADIUS;
  }
  result.tx = Math.sin(result.yaw);
  result.tz = Math.cos(result.yaw);
  return result;
}

function nearestStation(x: number, z: number, pose: Pose, yaw?: number): number {
  const clampedZ = THREE.MathUtils.clamp(z, SOUTH_Z, NORTH_Z);
  let best = clampedZ - SOUTH_Z, distance = Infinity;
  for (let candidate = 0; candidate < 4; candidate++) {
    let station: number;
    if (candidate === 0) station = clampedZ - SOUTH_Z;
    else if (candidate === 1) station = STRAIGHT + ARC + NORTH_Z - clampedZ;
    else if (candidate === 2) {
      if (!(z >= NORTH_Z)) continue;
      station = STRAIGHT + TURN_RADIUS * Math.atan2(z - NORTH_Z, CENTER_X - x);
    } else {
      if (!(z <= SOUTH_Z)) continue;
      station = 2 * STRAIGHT + ARC + TURN_RADIUS * Math.atan2(SOUTH_Z - z, x - CENTER_X);
    }
    sampleRoute(station, pose);
    const heading = yaw === undefined ? 0 : Math.atan2(Math.sin(yaw - pose.yaw), Math.cos(yaw - pose.yaw));
    const squared = (x - pose.x) ** 2 + (z - pose.z) ** 2 + heading ** 2 * TURN_RADIUS ** 2;
    if (squared < distance) { best = station; distance = squared; }
  }
  return wrap(best);
}

function copyTrafficVehicle(vehicle: Vehicle, snapshot: TrafficFrame["vehicle"]): TrafficFrame["vehicle"] {
  snapshot.id = vehicle.id;
  snapshot.x = vehicle.pose.x;
  snapshot.z = vehicle.pose.z;
  snapshot.yaw = vehicle.pose.yaw;
  snapshot.speed = vehicle.speed;
  snapshot.halfWidth = vehicle.halfWidth;
  snapshot.halfLength = vehicle.halfLength;
  return snapshot;
}

function boxesOverlap(a: Pose, vehicle: Vehicle, b: Pose, other: Vehicle, padding: number): boolean {
  const dx = b.x - a.x, dz = b.z - a.z;
  if (dx * dx + dz * dz > (vehicle.bodyRadius + other.bodyRadius + Math.SQRT2 * padding) ** 2) return false;
  const cosine = Math.abs(a.tx * b.tx + a.tz * b.tz);
  const sine = Math.abs(a.tx * b.tz - a.tz * b.tx);
  const length = vehicle.halfLength + padding * .5, width = vehicle.halfWidth + padding * .5;
  const otherLength = other.halfLength + padding * .5, otherWidth = other.halfWidth + padding * .5;
  // Separating axes of both oriented rectangles, including the opposing lane at each turn.
  // Symmetric padding lets a leader move away instead of trapping it in its follower's margin.
  return Math.abs(dx * a.tx + dz * a.tz) <= length + otherLength * cosine + otherWidth * sine
    && Math.abs(dx * a.tz - dz * a.tx) <= width + otherLength * sine + otherWidth * cosine
    && Math.abs(dx * b.tx + dz * b.tz) <= otherLength + length * cosine + width * sine
    && Math.abs(dx * b.tz - dz * b.tx) <= otherWidth + length * sine + width * cosine;
}

function unmodifiedShader(material: THREE.Material): boolean {
  return material.onBeforeCompile === THREE.Material.prototype.onBeforeCompile
    && material.customProgramCacheKey === THREE.Material.prototype.customProgramCacheKey;
}

function inheritedRole(object: THREE.Object3D, model: THREE.Group): unknown {
  let owner: THREE.Object3D | null = object;
  while (owner) {
    if (owner.userData.lifeRole !== undefined) return owner.userData.lifeRole;
    if (owner === model) break;
    owner = owner.parent;
  }
  return undefined;
}

function onRoad(x: number, z: number): boolean {
  return x >= -14 && x <= -6 && z >= -46 && z <= 29
    || x >= -38 && x <= 38 && (z >= -32 && z <= -26 || z >= 18 && z <= 24);
}

function bodyFitsRoad(pose: Pose, halfWidth: number, halfLength: number): boolean {
  // Check corners and where each edge crosses the main-road sides. Corners alone
  // would miss an edge cutting across the inside corner of an intersection.
  let previousX = pose.x - pose.tx * halfLength - pose.tz * halfWidth;
  let previousZ = pose.z - pose.tz * halfLength + pose.tx * halfWidth;
  for (let corner = 0; corner < 4; corner++) {
    const length = corner < 2 ? halfLength : -halfLength;
    const width = corner === 0 || corner === 3 ? -halfWidth : halfWidth;
    const x = pose.x + pose.tx * length + pose.tz * width;
    const z = pose.z + pose.tz * length - pose.tx * width;
    if (!onRoad(x, z)) return false;
    for (let side = -14; side <= -6; side += 8) {
      if (x === previousX || (x - side) * (previousX - side) >= 0) continue;
      const fraction = (side - previousX) / (x - previousX);
      const crossingZ = previousZ + fraction * (z - previousZ);
      // Use the outside side of the boundary so the main road cannot hide a cut corner.
      if (!onRoad(side + (side === -14 ? -1e-6 : 1e-6), crossingZ)) return false;
    }
    previousX = x;
    previousZ = z;
  }
  return true;
}

// r180's standard, depth and distance shaders all expose this chunk. Geometry stays shared.
const WIND_VERTEX = `
  #include <begin_vertex>
  vec3 lifeWorld = (modelMatrix * vec4(transformed, 1.0)).xyz;
  float lifeWeight = smoothstep(1.2, 5.0, lifeWorld.y);
  vec3 lifeWind = lifeWeight * vec3(
    0.04 * sin(townSquareLifeTime * 1.2 + lifeWorld.x * 0.25 + lifeWorld.z * 0.21),
    0.0,
    0.025 * sin(townSquareLifeTime * 0.9 + lifeWorld.x * 0.17 - lifeWorld.z * 0.3)
  );
  // Convert world-horizontal motion to an orthogonal, possibly scaled mesh basis.
  transformed += vec3(
    dot(lifeWind, modelMatrix[0].xyz) / max(dot(modelMatrix[0].xyz, modelMatrix[0].xyz), 0.000001),
    dot(lifeWind, modelMatrix[1].xyz) / max(dot(modelMatrix[1].xyz, modelMatrix[1].xyz), 0.000001),
    dot(lifeWind, modelMatrix[2].xyz) / max(dot(modelMatrix[2].xyz, modelMatrix[2].xyz), 0.000001)
  );
`;

/**
 * Local review only. Route and participant coordinates are runtime Y-up world metres.
 * Call after attaching the model; keep its parent transforms fixed during animation.
 * Vehicle fronts are +Z. Vehicle parents may be plain Object3D nodes from Blender empties.
 * Foliage/water roles apply to descendant meshes. routeOffset adds metres to the nearest station.
 * Live activation may reposition authored vehicles outside the shorter intersection loop.
 * Enabled initially. update returns whether a redraw is needed, including restored state.
 * The caller owns scheduling, pause/visibility handling and dynamic shadow-map refresh.
 * reset and disable restore ordinary scenery, retaining scoped managed poses.
 * Dispose before releasing the model's GLTF lease. No geometry or borrowed texture is disposed.
 * Limits: fixed road rectangles, not arbitrary static-geometry
 * collision; participant snapshots have no velocity prediction. Turns have tight clearance.
 * Route following supplies driver input. Contact forces produce the proposed pose; the existing owner checks its sweep.
 * Wind leaves normals and culling bounds unchanged. Native rendering/performance is not covered.
 */
export function createTownSquareLife(model: THREE.Group, getParticipants: () => readonly Participant[]): {
  update(dtSeconds: number): boolean;
  setEnabled(enabled: boolean): void;
  reset(): void;
  dispose(): void;
  summary(): string;
  vehicleFootprints(): readonly { id: string; x: number; z: number; yaw: number; halfWidth: number; halfLength: number; speed: number; radius: number }[];
  /** Read-only accepted world projection. Bind the exact visit scope first.
   * Missing/invalid/reconnecting receipts freeze the fleet in place. */
  setStreetCrashes(view: CompanionStreetCrashTrafficView | null, scope: CompanionStreetCrashScope | null): boolean;
  streetIssues(): readonly string[];
} {
  const vehicles: Vehicle[] = [];
  const bindings: MaterialBinding[] = [];
  const ownedMaterials = new Set<THREE.Material>();
  const waterMaps = new Map<THREE.Texture, { texture: THREE.Texture; offset: THREE.Vector2; matrix: THREE.Matrix3 }>();
  const windMaterials = new Map<THREE.Material, THREE.Material>();
  const waterMaterials = new Map<THREE.Material, THREE.Material>();
  const time = { value: 0 };
  const participantPositions = new Float64Array(MAX_PARTICIPANTS * 3);
  let participantCount = 0;
  const position = new THREE.Vector3();
  const orientation = new THREE.Quaternion();
  const parentOrientation = new THREE.Quaternion();
  const wheelRotation = new THREE.Quaternion();
  const steeringRotation = new THREE.Quaternion();
  const restOrientation = new THREE.Quaternion();
  const chassisRotation = new THREE.Quaternion();
  const chassisEuler = new THREE.Euler(0, 0, 0, "YXZ");
  const ground = createTownSquareRoadSurfaces(model);
  const probe = blankPose();
  const predicted = blankPose();
  const stationProbe = blankPose();
  let enabled = true, disposed = false, dirty = false;
  let elapsedSeconds = 0;
  let candidateCount = 0, skippedVehicles = 0, foliageCount = 0, waterCount = 0, skippedAmbient = 0;
  let participantState = "pending";
  let streetScope: CompanionStreetCrashScope | null = null;
  let streetRevision = -1, streetObservedAt = 0;
  let streetFleetIdentity: string | null = null;
  let streetIssue: string | null = null;
  let fleetIssue: string | null = null;
  const compatible = (width: number, length: number) => finite(width) && finite(length)
    && width >= .3 && width <= 3 && length >= .5 && length <= 8;

  function configureWind<T extends THREE.Material>(material: T): T {
    material.onBeforeCompile = shader => {
      shader.uniforms.townSquareLifeTime = time;
      shader.vertexShader = `uniform float townSquareLifeTime;\n${shader.vertexShader}`
        .replace("#include <begin_vertex>", WIND_VERTEX);
    };
    material.customProgramCacheKey = () => "town-square-life-r180-wind-v1";
    ownedMaterials.add(material);
    return material;
  }

  function windMaterial(source: THREE.Material): THREE.Material {
    const cached = windMaterials.get(source);
    if (cached) return cached;
    const material = configureWind(source.clone());
    windMaterials.set(source, material);
    return material;
  }

  function waterMaterial(source: THREE.Material): THREE.Material {
    const cached = waterMaterials.get(source);
    if (cached) return cached;
    if (!(source instanceof THREE.MeshStandardMaterial) || !source.normalMap || !unmodifiedShader(source)) return source;
    const normal = source.normalMap;
    // Manual UV matrices may encode transforms that offset/repeat cannot reproduce.
    if (!normal.matrixAutoUpdate || normal.isRenderTargetTexture) return source;
    let entry = waterMaps.get(normal);
    if (!entry) {
      if (waterMaps.size >= MAX_WATER_MAPS) return source;
      entry = { texture: normal.clone(), offset: normal.offset.clone(), matrix: normal.matrix.clone() };
      waterMaps.set(normal, entry);
    }
    const material = source.clone();
    material.normalMap = entry.texture;
    ownedMaterials.add(material);
    waterMaterials.set(source, material);
    return material;
  }

  model.updateWorldMatrix(true, true);
  model.traverse(object => {
    if (object.userData.lifeRole === "vehicle") {
      object.getWorldPosition(position);
      // Includes the original parked car at source Y=-32, runtime Z=32.
      if (!finite(position.x) || !finite(position.y) || !finite(position.z)
        || position.x < -14 || position.x > -6 || position.z < -46 || position.z > 29
        || object.userData.parked === true || finite(object.userData.nominalSpeed) && object.userData.nominalSpeed <= 0) {
        skippedVehicles++;
        return;
      }
      candidateCount++;
      if (vehicles.length >= MAX_VEHICLES) return;
      const id = object.userData.vehicleId;
      // Never use traversal order, a generated name or a duplicate model ID as
      // the accepted fleet identity. Bad markers hold the entire fleet.
      if (typeof id !== "string" || !id || id.length > 240 || id.trim() !== id || vehicles.some(vehicle => vehicle.id === id)) {
        fleetIssue = "Traffic held: vehicle markers need unique stable vehicleId values."; return;
      }
      const selected = id.startsWith("blendkit-") || object.userData.runtimeAxisBakePending === true;
      const width = object.userData.halfWidth, length = object.userData.halfLength;
      if ((width !== undefined || length !== undefined || selected) && !compatible(width, length)
        || object.userData.runtimeAxisBakePending === true
        || selected && (object.userData.runtimeAxisBakePending !== false || object.userData.runtimeForward !== "+Z")) {
        fleetIssue = `Traffic held: ${id} needs reviewed runtime axes and per-vehicle dimensions.`; return;
      }
      const original = captureObjectTransform(object);
      const height = position.y;
      object.getWorldQuaternion(orientation);
      const vehicleOrientation = orientation.clone();
      const forward = new THREE.Vector3(0, 0, 1).applyQuaternion(orientation);
      const yaw = Math.atan2(forward.x, forward.z);
      const measured = measuredVehicleBounds(object, position.clone(), yaw);
      if (measured && (!compatible(measured.halfWidth, measured.halfLength)
        || compatible(width, length) && (width + .01 < measured.halfWidth || length + .01 < measured.halfLength))) {
        fleetIssue = `Traffic held: ${id} collision dimensions understate its mounted geometry.`; return;
      }
      const originalPose = { ...blankPose(), x: position.x, z: position.z, yaw, tx: Math.sin(yaw), tz: Math.cos(yaw) };
      const offset = finite(object.userData.routeOffset) ? object.userData.routeOffset : 0;
      const start = wrap(nearestStation(position.x, position.z, stationProbe) + offset);
      const wheels: Wheel[] = [];
      let frontZ = -Infinity, rearZ = Infinity;
      object.traverse(child => {
        if (child.userData.lifeRole !== "wheel") return;
        let ancestor = child.parent;
        while (ancestor && ancestor !== object && ancestor.userData.lifeRole !== "vehicle") ancestor = ancestor.parent;
        if (ancestor !== object) return;
        child.getWorldPosition(position);
        const dx = position.x - originalPose.x, dz = position.z - originalPose.z;
        const x = dx * Math.cos(yaw) - dz * Math.sin(yaw), z = dx * Math.sin(yaw) + dz * Math.cos(yaw);
        const front = child.userData.frontAxle === true;
        const side = x < 0 ? 0 : 1;
        if (front) frontZ = Math.max(frontZ, z);
        else rearZ = Math.min(rearZ, z);
        child.parent!.getWorldQuaternion(parentOrientation).invert().multiply(vehicleOrientation);
        const wheelParent = new THREE.Matrix4().copy(child.parent!.matrixWorld).invert();
        wheels.push({
          original: captureObjectTransform(child), front,
          radius: finite(child.userData.radius) && child.userData.radius > 0 ? THREE.MathUtils.clamp(child.userData.radius, .1, 1) : .32,
          rollAxis: new THREE.Vector3(1, 0, 0).applyQuaternion(parentOrientation),
          steerAxis: new THREE.Vector3(0, 1, 0).applyQuaternion(parentOrientation), angle: 0, index: (front ? 0 : 2) + side, x, z,
          suspensionAxis: new THREE.Vector3(0, 1, 0).applyMatrix4(wheelParent).sub(new THREE.Vector3().applyMatrix4(wheelParent)),
        });
      });
      if (wheels.length !== 4 || new Set(wheels.map(wheel => wheel.index)).size !== 4) {
        fleetIssue = `Traffic held: ${id} needs four independent wheel pivots with axle metadata.`; return;
      }
      wheels.sort((a, b) => a.index - b.index);
      const wheelbase = finite(frontZ - rearZ) ? THREE.MathUtils.clamp(frontZ - rearZ, 1, 4) : 2.7;
      const halfWidth = compatible(width, length) ? width : measured?.halfWidth ?? HALF_WIDTH;
      const halfLength = compatible(width, length) ? length : measured?.halfLength ?? HALF_LENGTH;
      const controller = new VehicleController({ danfo: /danfo/i.test(id), wheelbase, halfWidth, ground, groundCast: ground.cast(height),
        wheelRadius: wheels[0]?.radius,
        wheelLayout: wheels.map(wheel => ({ x: wheel.x, z: wheel.z, front: wheel.front, radius: wheel.radius })),
        impact: () => { const vehicle = vehicles.find(value => value.id === id);
          if (vehicle) object.userData.lagosImpactSequence = ++vehicle.impactSequence; } });
      controller.init();
      const ai = new TrafficAI(); ai.init();
      const nominalSpeed = THREE.MathUtils.clamp(finite(object.userData.nominalSpeed) ? object.userData.nominalSpeed : 2.5, .1, MAX_SPEED);
      const trafficPose = { id, x: originalPose.x, z: originalPose.z, yaw: originalPose.yaw, speed: 0, halfWidth, halfLength };
      const vehicle: Vehicle = {
        id, original, originalPose,
        pose: { ...originalPose }, height, start, station: start,
        nominalSpeed,
        speed: 0, steering: 0, wheelbase, rearAxle: Math.min(...controller.config.wheels.map(wheel => wheel.z)),
        wheels, active: false, halfWidth, halfLength, bodyRadius: Math.hypot(halfWidth, halfLength),
        managed: false, held: false, externallyPosed: false, streetSequence: -2,
        controller, ai, lateral: 0, hornSequence: 0, impactSequence: 0,
        trafficFrame: {
          vehicle: trafficPose, targetSpeed: 0, cruiseSpeed: nominalSpeed, lead: undefined, leadGap: Infinity,
          lateralGrip: controller.response.lateralGrip, canPass: false,
          clear: (lateral, forward) => passClear(vehicle, lateral, forward),
          horn: () => { vehicle.original.node.userData.lagosHornSequence = ++vehicle.hornSequence; },
        },
        leadSnapshot: { ...trafficPose },
        originalHornSequence: object.userData.lagosHornSequence, originalImpactSequence: object.userData.lagosImpactSequence,
      };
      vehicles.push(vehicle);
      return;
    }
    if (!(object instanceof THREE.Mesh)) return;
    const role = inheritedRole(object, model);
    if (role !== "foliage" && role !== "water") return;
    const materials = Array.isArray(object.material) ? object.material : [object.material];
    let moving: THREE.Material[];
    let depth = object.customDepthMaterial, distance = object.customDistanceMaterial;
    if (role === "foliage") {
      // No shader edits on custom, instanced or skinned assets with a different vertex contract.
      if (THREE.REVISION !== "180" || object instanceof THREE.InstancedMesh || object instanceof THREE.SkinnedMesh || object instanceof THREE.BatchedMesh
        || materials.some(material => !(material instanceof THREE.MeshStandardMaterial) || !unmodifiedShader(material))
        || depth && (!(depth instanceof THREE.MeshDepthMaterial) || !unmodifiedShader(depth))
        || distance && (!(distance instanceof THREE.MeshDistanceMaterial) || !unmodifiedShader(distance))) {
        skippedAmbient++;
        return;
      }
      moving = materials.map(windMaterial);
      // The shadow renderer copies map/alpha/displacement properties from each visible material.
      depth = depth ? windMaterial(depth) : configureWind(new THREE.MeshDepthMaterial({ depthPacking: THREE.RGBADepthPacking }));
      distance = distance ? windMaterial(distance) : configureWind(new THREE.MeshDistanceMaterial());
      foliageCount++;
    } else {
      moving = materials.map(waterMaterial);
      if (moving.every((material, index) => material === materials[index])) { skippedAmbient++; return; }
      waterCount++;
    }
    const binding: MaterialBinding = {
      mesh: object, original: object.material, moving: Array.isArray(object.material) ? moving : moving[0],
      originalDepth: object.customDepthMaterial, originalDistance: object.customDistanceMaterial, depth, distance,
    };
    bindings.push(binding);
    object.material = binding.moving;
    object.customDepthMaterial = depth;
    object.customDistanceMaterial = distance;
    dirty = true;
  });
  vehicles.sort((a, b) => a.id.localeCompare(b.id));

  function readParticipants(): boolean {
    participantCount = 0;
    try {
      const current = getParticipants();
      if (!Array.isArray(current) || current.length > MAX_PARTICIPANTS) {
        participantState = "traffic held: participant limit or invalid snapshot";
        return false;
      }
      let count = 0;
      for (const participant of current) {
        if (count >= MAX_PARTICIPANTS) {
          participantState = "traffic held: participant limit or invalid snapshot";
          return false;
        }
        if (!participant || !finite(participant.x) || !finite(participant.z) || !finite(participant.radius) || participant.radius < 0) {
          participantState = "traffic held: invalid participant position";
          return false;
        }
        const offset = count * 3;
        participantPositions[offset] = participant.x;
        participantPositions[offset + 1] = participant.z;
        participantPositions[offset + 2] = participant.radius;
        count++;
      }
      // Failed reads never expose a partial snapshot to traffic sweeps.
      participantCount = count;
      participantState = `${participantCount} participant footprints`;
      return true;
    } catch {
      participantState = "traffic held: participant snapshot unavailable";
      return false;
    }
  }

  function occupied(pose: Pose, vehicle: Vehicle, padding: number): boolean {
    if (!bodyFitsRoad(pose, vehicle.halfWidth, vehicle.halfLength)) return true;
    for (const other of vehicles) {
      if (other !== vehicle && boxesOverlap(pose, vehicle, other.pose, other, padding)) return true;
    }
    for (let index = 0; index < participantCount; index++) {
      const offset = index * 3;
      const dx = participantPositions[offset] - pose.x, dz = participantPositions[offset + 1] - pose.z;
      const extra = participantPositions[offset + 2] + padding;
      if (Math.abs(dx * pose.tx + dz * pose.tz) <= vehicle.halfLength + extra
        && Math.abs(dx * pose.tz - dz * pose.tx) <= vehicle.halfWidth + extra) return true;
    }
    return false;
  }

  function sampleVehicleRoute(station: number, lateral: number, result: Pose) {
    sampleRoute(station, result);
    result.x += result.tz * lateral; result.z -= result.tx * lateral;
    return result;
  }

  // Clearance starts at the actual chassis, including its unfinished turn and
  // lateral tracking error. A tangent-only box can strand a diagonal bus.
  function sampleVehicleSweep(vehicle: Vehicle, distance: number, lateral: number, result: Pose) {
    sampleVehicleRoute(vehicle.station, vehicle.lateral, stationProbe);
    const errorX = vehicle.pose.x - stationProbe.x, errorZ = vehicle.pose.z - stationProbe.z;
    const heading = Math.atan2(Math.sin(vehicle.pose.yaw - stationProbe.yaw), Math.cos(vehicle.pose.yaw - stationProbe.yaw));
    sampleVehicleRoute(vehicle.station + distance, lateral, result);
    const tracking = Math.max(0, 1 - distance / Math.max(2, vehicle.wheelbase));
    result.x += errorX * tracking; result.z += errorZ * tracking;
    result.yaw += heading * tracking;
    result.tx = Math.sin(result.yaw); result.tz = Math.cos(result.yaw);
    return result;
  }

  function passClear(vehicle: Vehicle, lateral: number, horizon: number): boolean {
    const samples = Math.ceil(horizon / SWEEP_STEP);
    // Check the lateral sweep before committing, then the forward passing corridor.
    for (let offset = 0; offset <= 8; offset++) {
      sampleVehicleSweep(vehicle, 0, THREE.MathUtils.lerp(vehicle.lateral, lateral, offset / 8), probe);
      if (occupied(probe, vehicle, CLEARANCE + .12)) return false;
    }
    for (let index = 0; index <= samples; index++) {
      sampleVehicleSweep(vehicle, index / Math.max(1, samples) * horizon, lateral, probe);
      if (occupied(probe, vehicle, CLEARANCE + .12)) return false;
      // Predict other spline owners at the time this car reaches each sample.
      // Opposing traffic cannot enter a currently empty passing corridor unnoticed.
      const time = index / Math.max(1, samples) * horizon / Math.max(5 / 3.6, vehicle.speed);
      for (const other of vehicles) {
        if (other === vehicle || !other.active || other.held || other.externallyPosed) continue;
        sampleVehicleSweep(other, other.speed * time, other.lateral, predicted);
        if (boxesOverlap(probe, vehicle, predicted, other,
          CLEARANCE + .12 + other.speed * SWEEP_STEP / Math.max(5 / 3.6, vehicle.speed))) return false;
      }
    }
    return true;
  }

  function clearDistance(vehicle: Vehicle, horizon: number): number {
    const samples = Math.ceil(horizon / SWEEP_STEP);
    const step = horizon / samples;
    const sweepPadding = SWEEP_STEP * .5 * (1 + vehicle.bodyRadius / TURN_RADIUS);
    for (let index = 0; index <= samples; index++) {
      sampleVehicleSweep(vehicle, index * step, vehicle.lateral, probe);
      if (occupied(probe, vehicle, CLEARANCE + sweepPadding)) return Math.max(0, (index - 1) * step);
    }
    return Infinity;
  }

  function applyVehicle(vehicle: Vehicle) {
    const node = vehicle.original.node;
    const response = vehicle.controller.response;
    const animate = enabled && !vehicle.held && !vehicle.externallyPosed && !streetIssue;
    position.set(vehicle.pose.x, vehicle.height + (animate ? response.lift : 0), vehicle.pose.z);
    orientation.setFromAxisAngle(UP, vehicle.pose.yaw);
    if (animate) {
      chassisEuler.set(response.pitch, response.brakingPull, response.roll, "YXZ");
      orientation.multiply(chassisRotation.setFromEuler(chassisEuler));
    }
    if (node.parent) {
      node.parent.worldToLocal(position);
      node.parent.getWorldQuaternion(parentOrientation).invert();
      orientation.premultiply(parentOrientation);
    }
    node.position.copy(position);
    node.quaternion.copy(orientation);
    node.matrixAutoUpdate = true;
    node.matrixWorldAutoUpdate = true;
    node.updateMatrix();
    for (const wheel of vehicle.wheels) {
      const child = wheel.original.node;
      const simulated = animate ? vehicle.controller.dynamics?.wheels[wheel.index] : undefined;
      // Y-up/+Z forward needs positive X roll: the bottom contact patch moves toward -Z.
      // Qsteer * Qroll * Qrest steers the axle while retaining the exported rest orientation.
      wheelRotation.setFromAxisAngle(wheel.rollAxis, wheel.angle);
      steeringRotation.setFromAxisAngle(wheel.steerAxis, simulated?.steering ?? (wheel.front ? vehicle.steering : 0));
      wheel.original.copyPositionTo(child.position);
      if (simulated) child.position.addScaledVector(wheel.suspensionAxis, simulated.compression - vehicle.controller.config.sag);
      child.quaternion.copy(steeringRotation).multiply(wheelRotation).multiply(wheel.original.copyQuaternionTo(restOrientation));
      child.matrixAutoUpdate = true;
      child.matrixWorldAutoUpdate = true;
      child.updateMatrix();
    }
  }

  function stepTraffic(dt: number) {
    for (const vehicle of vehicles) {
      if (vehicle.held || streetIssue || fleetIssue) { vehicle.speed = 0; vehicle.controller.hold(); continue; }
      if (vehicle.externallyPosed) continue;
      if (!vehicle.active) {
        sampleRoute(vehicle.start, probe);
        const sweepPadding = SWEEP_STEP * .5 * (1 + vehicle.bodyRadius / TURN_RADIUS);
        if (occupied(probe, vehicle, CLEARANCE + sweepPadding)) continue;
        Object.assign(vehicle.pose, probe);
        vehicle.active = true;
        dirty = true;
      }
      // Brake before the curvature changes, not after entering a tight turnaround.
      position.set(vehicle.pose.x, vehicle.height, vehicle.pose.z);
      vehicle.controller.setMotion(position, vehicle.pose.yaw,
        { speed: vehicle.speed, targetSpeed: vehicle.nominalSpeed, curvature: vehicle.pose.curvature, brake: BRAKE, acceleration: ACCELERATION });
      const brake = vehicle.controller.brakingDeceleration(BRAKE);
      let target = Math.min(vehicle.nominalSpeed, Math.sqrt(TURN_SPEED ** 2 + 2 * brake * Math.max(0, vehicle.pose.turnIn - .6)));
      let lead: Vehicle | undefined, leadGap = Infinity;
      const passId = vehicle.ai.passingVehicleId;
      for (const other of vehicles) {
        if (other === vehicle || !other.active) continue;
        // A crashed pose may be off its lap station. Its full box is handled by
        // clearDistance, not an invented route distance to the old station.
        if (other.held || other.externallyPosed) continue;
        const stationGap = wrap(other.station - vehicle.station);
        const rawGap = stationGap - vehicle.halfLength - other.halfLength;
        if (other.id === passId || !passId && stationGap < LAP / 2 && rawGap < leadGap) {
          lead = other; leadGap = other.id === passId && stationGap > LAP / 2
            ? stationGap - LAP - vehicle.halfLength - other.halfLength : rawGap;
        }
        if (Math.abs(other.lateral - vehicle.lateral) > other.halfWidth + vehicle.halfWidth + CLEARANCE) continue;
        const gap = rawGap - FOLLOW_GAP - vehicle.speed * .4;
        target = Math.min(target, Math.sqrt(other.speed ** 2 + 2 * brake * Math.max(0, gap)));
      }
      // Each AI consumes its own copied frame synchronously; later cars cannot change it.
      const frame = vehicle.trafficFrame;
      copyTrafficVehicle(vehicle, frame.vehicle);
      frame.targetSpeed = target;
      frame.cruiseSpeed = vehicle.nominalSpeed;
      frame.lead = lead ? copyTrafficVehicle(lead, vehicle.leadSnapshot) : undefined;
      frame.leadGap = leadGap;
      frame.lateralGrip = vehicle.controller.response.lateralGrip;
      sampleVehicleRoute(vehicle.station, vehicle.lateral, stationProbe);
      const headingError = Math.atan2(Math.sin(vehicle.pose.yaw - stationProbe.yaw), Math.cos(vehicle.pose.yaw - stationProbe.yaw));
      frame.canPass = vehicle.pose.curvature === 0 && vehicle.pose.turnIn > 16 && Math.abs(headingError) < .15;
      vehicle.ai.setFrame(frame);
      const decision = vehicle.ai.update(dt);
      if (vehicle.lateral !== decision.lateral) {
        vehicle.lateral = decision.lateral;
      }
      target = decision.speed;
      const horizon = vehicle.speed ** 2 / (2 * brake) + vehicle.speed * .5 + 1;
      const clear = clearDistance(vehicle, horizon);
      target = Math.min(target, Math.sqrt(2 * brake * Math.max(0, clear - .25 - vehicle.speed * .25)));
      const speed = vehicle.speed;
      // Pure pursuit requests steering. It never replaces chassis yaw or position with the route tangent.
      const lookAhead = Math.max(1.5, vehicle.wheelbase * .7 + speed * .35);
      sampleVehicleRoute(vehicle.station + lookAhead, vehicle.lateral, predicted);
      const rearAxle = vehicle.rearAxle;
      const dx = predicted.x + predicted.tx * rearAxle - vehicle.pose.x - vehicle.pose.tx * rearAxle;
      const dz = predicted.z + predicted.tz * rearAxle - vehicle.pose.z - vehicle.pose.tz * rearAxle;
      const localX = dx * Math.cos(vehicle.pose.yaw) - dz * Math.sin(vehicle.pose.yaw);
      const curvature = 2 * localX / Math.max(.25, dx * dx + dz * dz);
      position.set(vehicle.pose.x, vehicle.height, vehicle.pose.z);
      vehicle.controller.setMotion(position, vehicle.pose.yaw,
        { speed, targetSpeed: target, curvature, brake: BRAKE, acceleration: ACCELERATION });
      const previousDynamics = vehicle.controller.dynamics;
      const response = vehicle.controller.update(dt);
      const dynamics = vehicle.controller.dynamics;
      if (!dynamics) continue;
      const next = vehicleDynamicsPose(dynamics, vehicle.controller.config);
      const travelled = Math.hypot(next.x - vehicle.pose.x, next.z - vehicle.pose.z);
      const yawDelta = Math.atan2(Math.sin(next.yaw - vehicle.pose.yaw), Math.cos(next.yaw - vehicle.pose.yaw));
      const samples = Math.max(1, Math.ceil((travelled + Math.abs(yawDelta) * vehicle.bodyRadius) / .1));
      let blocked = travelled > clear;
      for (let index = 1; !blocked && index <= samples; index++) {
        const fraction = index / samples, yaw = vehicle.pose.yaw + yawDelta * fraction;
        probe.x = THREE.MathUtils.lerp(vehicle.pose.x, next.x, fraction);
        probe.z = THREE.MathUtils.lerp(vehicle.pose.z, next.z, fraction);
        probe.yaw = yaw;
        probe.tx = Math.sin(yaw);
        probe.tz = Math.cos(yaw);
        blocked = occupied(probe, vehicle, CLEARANCE + .05);
      }
      if (blocked) { vehicle.controller.hold(previousDynamics); vehicle.speed = 0; continue; }
      vehicle.speed = response.speed;
      vehicle.station = nearestStation(next.x, next.z, stationProbe, next.yaw);
      sampleVehicleRoute(vehicle.station, vehicle.lateral, predicted);
      vehicle.pose.x = next.x;
      vehicle.pose.z = next.z;
      vehicle.pose.yaw = next.yaw;
      vehicle.pose.tx = Math.sin(next.yaw);
      vehicle.pose.tz = Math.cos(next.yaw);
      vehicle.pose.curvature = predicted.curvature;
      vehicle.pose.turnIn = predicted.turnIn;
      vehicle.steering = dynamics.steering;
      for (const wheel of vehicle.wheels) wheel.angle = dynamics.wheels[wheel.index].angle;
      if (travelled > .00001 || Math.abs(yawDelta) > .00001 || dynamics.wheels.some(wheel => Math.abs(wheel.angularVelocity) > .001)) dirty = true;
      if (Math.abs(response.roll) + Math.abs(response.pitch) + Math.abs(response.lift) > .0001) dirty = true;
    }
  }

  function bindMaterials(moving: boolean) {
    for (const binding of bindings) {
      binding.mesh.material = moving ? binding.moving : binding.original;
      binding.mesh.customDepthMaterial = moving ? binding.depth : binding.originalDepth;
      binding.mesh.customDistanceMaterial = moving ? binding.distance : binding.originalDistance;
    }
  }

  function restoreState(releaseManaged = false) {
    for (const vehicle of vehicles) {
      vehicle.controller.reset(); vehicle.ai.reset(vehicle.managed && !releaseManaged ? vehicle.lateral : 0);
      if (vehicle.managed && !releaseManaged) {
        vehicle.speed = 0;
        // Pause/reset/reduced motion cannot teleport a retained world vehicle.
        applyVehicle(vehicle); continue;
      }
      vehicle.lateral = 0;
      vehicle.original.restore();
      Object.assign(vehicle.pose, vehicle.originalPose);
      vehicle.station = vehicle.start;
      vehicle.speed = 0;
      vehicle.steering = 0;
      vehicle.active = false;
      vehicle.managed = false; vehicle.held = false; vehicle.externallyPosed = false;
      vehicle.streetSequence = -2;
      for (const wheel of vehicle.wheels) { wheel.original.restore(); wheel.angle = 0; }
    }
    time.value = 0;
    elapsedSeconds = 0;
    for (const entry of waterMaps.values()) {
      entry.texture.offset.copy(entry.offset);
      entry.texture.matrix.copy(entry.matrix);
    }
    participantState = "pending";
    dirty = true;
  }

  function setStreetCrashes(view: CompanionStreetCrashTrafficView | null, nextScope: CompanionStreetCrashScope | null): boolean {
    if (disposed) return false;
    if (!nextScope) {
      if (!streetScope) return false;
      streetScope = null; streetRevision = -1; streetObservedAt = 0; streetIssue = null; streetFleetIdentity = null;
      restoreState(true); return true;
    }
    if (!streetScope || !sameCompanionStreetCrashScope(streetScope, nextScope)) {
      streetScope = Object.freeze({ ...nextScope }); streetRevision = -1; streetObservedAt = 0; streetFleetIdentity = null;
      // Retain present positions until the new scope's initial receipt arrives.
      for (const vehicle of vehicles) { vehicle.managed = true; vehicle.speed = 0; vehicle.streetSequence = -2; }
    }
    const reject = (issue: string) => { streetIssue = issue; for (const vehicle of vehicles) vehicle.speed = 0; return false; };
    if (!view) return reject("Traffic held: waiting for the accepted street receipt.");
    if (!Array.isArray(view.vehicles) || view.vehicles.length > MAX_VEHICLES) return reject("Traffic held: the accepted fleet exceeds the district limit.");
    const fleetIdentity = JSON.stringify(view.vehicles);
    if (!sameCompanionStreetCrashScope(view.scope, streetScope) || !Number.isSafeInteger(view.revision) || view.revision < streetRevision
      || view.revision === streetRevision && fleetIdentity !== streetFleetIdentity
      || !Number.isSafeInteger(view.observedAt) || view.observedAt < streetObservedAt
      || view.vehicles.length !== vehicles.length || new Set(view.vehicles.map(vehicle => vehicle.pose.id)).size !== view.vehicles.length
      || view.vehicles.some(vehicle => !validCompanionStreetVehiclePose(vehicle.pose)
        || !["ambient", "player", "resident", "responder"].includes(vehicle.owner) || typeof vehicle.parked !== "boolean"
        || vehicle.hold !== null && (!vehicle.hold || typeof vehicle.hold.contactId !== "string" || !vehicle.hold.contactId
          || vehicle.hold.contactId.length > 1600 || !Number.isSafeInteger(vehicle.hold.stoppedAt) || vehicle.hold.stoppedAt < nextScope.sharedStartedAt)
        || !Number.isSafeInteger(vehicle.sequence) || vehicle.sequence < -1 || !finite(vehicle.speed) || vehicle.speed < 0 || vehicle.speed > 35
        || !vehicles.some(known => known.id === vehicle.pose.id && vehicle.sequence >= known.streetSequence && Math.abs(known.halfWidth - vehicle.pose.halfWidth) <= 1e-6
          && Math.abs(known.halfLength - vehicle.pose.halfLength) <= 1e-6 && Math.abs(known.height - vehicle.pose.point[1]) <= .05)))
      return reject("Traffic held: street scope, fleet, dimensions or revision do not match the rendered district.");
    const changed = view.revision !== streetRevision || streetIssue !== null;
    streetIssue = null; streetRevision = view.revision; streetObservedAt = view.observedAt; streetFleetIdentity = fleetIdentity;
    if (!changed) return false;
    for (const vehicle of vehicles) {
      const accepted = view.vehicles.find(item => item.pose.id === vehicle.id)!;
      const held = !!accepted.hold || accepted.parked;
      // Responder/robbery receipts do not replay old ambient poses or wheel
      // state. Only a new movement sequence or changed hold owns a transform.
      if (vehicle.streetSequence === accepted.sequence && vehicle.held === held) continue;
      const pose = accepted.pose;
      const moved = Math.hypot(pose.point[0] - vehicle.pose.x, pose.point[2] - vehicle.pose.z);
      // Accepted externally owned motion supplies travelled distance once per
      // sequence. Holds never roll wheels, including a late-join crash receipt.
      if (!held && accepted.owner !== "ambient" && vehicle.streetSequence >= -1) {
        for (const wheel of vehicle.wheels) wheel.angle = wrap(wheel.angle + moved / wheel.radius, TAU);
      }
      Object.assign(vehicle.pose, { x: pose.point[0], z: pose.point[2], yaw: pose.yaw,
        tx: Math.sin(pose.yaw), tz: Math.cos(pose.yaw), turnIn: 0, curvature: 0 });
      vehicle.station = nearestStation(pose.point[0], pose.point[2], stationProbe); vehicle.active = true; vehicle.managed = true;
      vehicle.controller.reset(); vehicle.ai.reset(); vehicle.lateral = 0;
      vehicle.held = held; vehicle.streetSequence = accepted.sequence;
      vehicle.externallyPosed = accepted.owner !== "ambient";
      vehicle.speed = vehicle.held ? 0 : accepted.speed;
      // Wheel angle/steering stay at their existing contact state while held.
      applyVehicle(vehicle);
    }
    dirty = true; return true;
  }

  return {
    update(dtSeconds) {
      if (disposed) return false;
      const restored = dirty;
      dirty = false;
      if (!enabled || !finite(dtSeconds) || dtSeconds <= 0) return restored;
      // Discard long suspension gaps. No accumulated debt or catch-up after pause/visibility changes.
      if (dtSeconds > .25) {
        for (const vehicle of vehicles) { vehicle.speed = 0; vehicle.controller.hold(); }
        return restored;
      }
      const elapsed = Math.min(dtSeconds, MAX_FRAME);
      elapsedSeconds += elapsed;
      const steps = Math.ceil(elapsed / MAX_STEP);
      if (candidateCount <= MAX_VEHICLES && !fleetIssue && readParticipants()) {
        for (let index = 0; index < steps; index++) stepTraffic(elapsed / steps);
        if (dirty) for (const vehicle of vehicles) if (vehicle.active) applyVehicle(vehicle);
      } else {
        for (const vehicle of vehicles) vehicle.speed = 0;
      }
      // All frequencies have a whole number of cycles over this interval.
      time.value = (time.value + elapsed) % (Math.PI * 200);
      if (foliageCount > 0) dirty = true;
      for (const entry of waterMaps.values()) {
        entry.texture.offset.set(entry.offset.x + .006 * Math.sin(time.value * .16), entry.offset.y + .004 * Math.sin(time.value * .13));
        entry.texture.updateMatrix();
        dirty = true;
      }
      const changed = restored || dirty;
      dirty = false;
      return changed;
    },
    setEnabled(next) {
      if (disposed || next === enabled) return;
      enabled = next;
      restoreState();
      bindMaterials(enabled);
    },
    reset() {
      if (disposed) return;
      restoreState();
      bindMaterials(enabled);
    },
    dispose() {
      if (disposed) return;
      restoreState(true);
      for (const vehicle of vehicles) {
        vehicle.controller.dispose(); vehicle.ai.dispose();
        const data = vehicle.original.node.userData;
        if (vehicle.originalHornSequence === undefined) delete data.lagosHornSequence;
        else data.lagosHornSequence = vehicle.originalHornSequence;
        if (vehicle.originalImpactSequence === undefined) delete data.lagosImpactSequence;
        else data.lagosImpactSequence = vehicle.originalImpactSequence;
      }
      streetScope = null; streetIssue = null;
      bindMaterials(false);
      for (const material of ownedMaterials) material.dispose();
      for (const entry of waterMaps.values()) entry.texture.dispose();
      ownedMaterials.clear();
      waterMaps.clear();
      windMaterials.clear();
      waterMaterials.clear();
      bindings.length = 0;
      vehicles.length = 0;
      participantCount = 0;
      disposed = true;
      enabled = false;
      dirty = false;
    },
    summary() {
      if (disposed) return "Scene life disposed; authored still restored.";
      const moving = vehicles.filter(vehicle => vehicle.active && vehicle.speed > .01).length;
      const wheels = vehicles.reduce((count, vehicle) => count + vehicle.wheels.length, 0);
      const status = fleetIssue ?? streetIssue ?? (candidateCount > MAX_VEHICLES ? `traffic held: ${candidateCount}/${MAX_VEHICLES} vehicle limit` : participantState);
      const lines = [
        `Scene life ${enabled ? "on" : "off"} · sim ${elapsedSeconds.toFixed(1)} s · ${moving} moving / ${vehicles.length - moving} held`,
        `${wheels} wheels · ${foliageCount} foliage · ${waterCount} water · ${enabled ? status : "authored still"}`,
      ];
      // Read actual node world positions, so review measurements expose unapplied transforms.
      for (const vehicle of vehicles.slice(0, 2)) {
        vehicle.original.node.getWorldPosition(position);
        const wheel = vehicle.wheels.find(candidate => candidate.front) ?? vehicle.wheels[0];
        const roll = wheel ? `${THREE.MathUtils.radToDeg(wheel.angle).toFixed(0)}°` : "none";
        lines.push(`${vehicle.id}: ${vehicle.speed.toFixed(2)} m/s · x/z ${position.x.toFixed(2)}, ${position.z.toFixed(2)} m · roll ${roll} · steer ${THREE.MathUtils.radToDeg(vehicle.steering).toFixed(0)}°`);
      }
      if (skippedVehicles || skippedAmbient) lines.push(`Unchanged: ${skippedVehicles} parked/out-of-road vehicles, ${skippedAmbient} unsupported/unmapped ambient meshes`);
      return lines.join("\n");
    },
    vehicleFootprints() {
      if (disposed) return [];
      return vehicles.map(vehicle => {
        vehicle.original.node.getWorldPosition(position);
        return { id: vehicle.id, x: position.x, z: position.z, yaw: vehicle.pose.yaw, halfWidth: vehicle.halfWidth,
          halfLength: vehicle.halfLength, speed: vehicle.speed, radius: vehicle.bodyRadius };
      });
    },
    setStreetCrashes,
    streetIssues() { return [fleetIssue, streetIssue].filter((issue): issue is string => issue !== null); },
  };
}
