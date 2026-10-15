import * as THREE from "three";
import { mergeGeometries } from "three/addons/utils/BufferGeometryUtils.js";
import type { CompanionAssetDelivery, CompanionAssetKey } from "../../../../../packages/utils/src/companionAssets";
import { companionCityVehicleParts } from "../../../../../packages/utils/src/companionCityParking";
import { transitRoutePoint, type TransitPlan, type TransitRoute, type TransitPoint } from "../../../../../packages/utils/src/companionCityTransit";
import { navigationObstacleBounds, routeSegmentClear, type RoomNavigation, type ScenePoint } from "../../../../../packages/utils/src/companionNavigation";
import { townSquareBounds } from "../../../../../packages/utils/src/companionTownSquare";
import { sampleCompanionMovement } from "../../../../../packages/utils/src/companionMovement";
import type { CompanionWorldSnapshot } from "../../../../../packages/utils/src/companionWorld";
import { acquireCompanionGltf, instantiateGltf, type GltfLease } from "../appearance/gltfAssets";

export type CityTrafficObstacle = { x: number; z: number; yaw: number; halfWidth: number; halfLength: number };
type CarModel = { key: CompanionAssetKey; id: string; halfWidth: number; halfLength: number };
type Car = { id: string; route: TransitRoute; offset: number; speed: number; colour: number; model: CarModel };
type MountedCar = { car: Car; root: THREE.Group; fallback: THREE.Group; metres: number; wheelTravel: number; point: TransitPoint;
  instance?: ReturnType<typeof instantiateGltf>; lease?: GltfLease; failed: boolean; shadows: boolean;
  wheels: { node: THREE.Object3D; base: THREE.Quaternion; radius: number }[] };
const HALF_WIDTH = 1.1, HALF_LENGTH = 2.4;
const ENTER = 240, RELEASE = 330, MODEL_ENTER = 65, MODEL_RELEASE = 95;
const MAX_VISIBLE = 16, MAX_MODELS = 4, MAX_FLEET = 512;
// These are artwork variants, never player/resident vehicle identities. The
// envelope also encloses the procedural fallback while a download is pending.
const genericCar: CarModel = { key: "bestieVehicleCar", id: "car", halfWidth: HALF_WIDTH, halfLength: HALF_LENGTH };
const veloster: CarModel = { key: "bestieVehicleVeloster", id: "hyundai-veloster", halfWidth: HALF_WIDTH, halfLength: HALF_LENGTH };
const bronco: CarModel = { key: "bestieVehicleBronco", id: "ford-bronco", halfWidth: 1.27, halfLength: 2.67 };
const crownVictoria: CarModel = { key: "bestieVehicleCrownVictoria", id: "police-crown-victoria", halfWidth: 1.16, halfLength: 2.8 };
const narrowFleet = [veloster, genericCar, veloster] as const;
const wideFleet = [veloster, genericCar, bronco, veloster, genericCar, bronco, veloster, crownVictoria] as const;
const wrap = (value: number, total: number) => ((value % total) + total) % total;
const angle = (value: number) => Math.atan2(Math.sin(value), Math.cos(value));
function variation(id: string) {
  let hash = 2166136261;
  for (const char of id) hash = Math.imul(hash ^ char.charCodeAt(0), 16777619);
  hash ^= hash >>> 16; hash = Math.imul(hash, 0x85ebca6b);
  hash ^= hash >>> 13; hash = Math.imul(hash, 0xc2b2ae35); hash ^= hash >>> 16;
  return (hash >>> 0) / 4294967296;
}

/** Background scenery only. The city frame owns time; no ownership, fares or crash receipts are created. */
export function createCityTraffic(plan: TransitPlan, navigation: RoomNavigation, epochMs: number,
  delivery: () => CompanionAssetDelivery, changed: () => void) {
  const group = new THREE.Group(); group.name = "City background cars";
  const fleet: Car[] = [], mounted = new Map<string, MountedCar>();
  const geometries = new Set<THREE.BufferGeometry>(), materials = new Set<THREE.Material>();
  const templates = new Map<number, THREE.Group>();
  let provider: CompanionAssetDelivery | undefined, disposed = false, dirty = false, moving = false;
  let lastNow = NaN, lastFleetAt = -Infinity, reduced = false;
  const lastViewer = new THREE.Vector3(Infinity, 0, Infinity);
  const spin = new THREE.Quaternion(), axle = new THREE.Vector3(1, 0, 0);
  const wake = () => { if (!disposed) { dirty = true; changed(); } };

  // Admission covers the complete chassis with overlapping longitudinal discs.
  // A single chassis-length disc would reject safe lanes beside the pavement. Use
  // local obstacle subsets only for the fixed route sections, never per frame.
  // The retained Town Square keeps its existing traffic and crash owner.
  const reserved = { minX: townSquareBounds.minX - 8, maxX: townSquareBounds.maxX + 8,
    minZ: townSquareBounds.minZ - 8, maxZ: townSquareBounds.maxZ + 8 };
  const bounds = [...navigation.obstacles, reserved].map(obstacle => ({ obstacle, bounds: navigationObstacleBounds(obstacle) }));
  const routes = new Map<string, { route: TransitRoute; models: readonly CarModel[] }>();
  if (plan.navigationRevision === navigation.revision && Number.isSafeInteger(epochMs) && epochMs >= 0) {
    for (const route of plan.routes) {
      if (routes.has(route.roadId) || !route.loop || route.points.length < 2 || !Number.isFinite(route.metres) || route.metres <= 0) continue;
      const sections = route.points.slice(1).map((point, index) => {
        const previous = route.points[index], padding = 4;
        const section = { ...navigation, obstacles: bounds.filter(({ bounds: box }) =>
          box.maxX >= Math.min(previous.x, point.x) - padding && box.minX <= Math.max(previous.x, point.x) + padding
          && box.maxZ >= Math.min(previous.z, point.z) - padding && box.minZ <= Math.max(previous.z, point.z) + padding).map(value => value.obstacle) };
        return section;
      });
      const fits = (halfWidth: number, halfLength: number) => route.points.slice(1).every((point, index) => {
        const previous = route.points[index], section = sections[index];
        const heading = Math.atan2(point.x - previous.x, point.z - previous.z);
        // Three overlapping discs enclose every corner of this model's body.
        const radius = Math.hypot(halfWidth, halfLength / 3) + .15;
        const sliceOffset = halfLength * 2 / 3;
        const swept = { ...section, radius };
        const offset = (p: TransitPoint, yaw: number, metres: number): ScenePoint =>
          [p.x + Math.sin(yaw) * metres, navigation.floorY, p.z + Math.cos(yaw) * metres];
        if (![-sliceOffset, 0, sliceOffset].every(at => routeSegmentClear(swept, offset(previous, heading, at), offset(point, heading, at)))) return false;
        const next = route.points[(index + 2) % (route.points.length - 1)];
        const nextYaw = Math.atan2(next.x - point.x, next.z - point.z), turn = angle(nextYaw - heading);
        const steps = Math.max(1, Math.ceil(Math.abs(turn) / .05));
        // Enlarge by the maximum arc-to-chord distance between rotation samples.
        const turning = { ...swept, radius: radius + sliceOffset * (1 - Math.cos(Math.abs(turn) / steps / 2)) };
        for (let step = 1; step <= steps; step++) for (const at of [-sliceOffset, 0, sliceOffset]) {
          if (!routeSegmentClear(turning, offset(point, heading + turn * (step - 1) / steps, at),
            offset(point, heading + turn * step / steps, at))) return false;
        }
        return true;
      });
      if (fits(HALF_WIDTH, HALF_LENGTH)) routes.set(route.roadId,
        { route, models: fits(bronco.halfWidth, crownVictoria.halfLength) ? wideFleet : narrowFleet });
    }
    // Round-robin retains neighbourhood coverage if the global fleet cap is reached.
    for (let index = 0; index < 40 && fleet.length < MAX_FLEET; index++) for (const { route, models } of routes.values()) {
      const count = Math.min(40, Math.max(2, Math.ceil(route.metres / 180)));
      if (index >= count || fleet.length >= MAX_FLEET) continue;
      const id = `city-car:${route.roadId}:${index}`;
      fleet.push({ id, route, offset: route.metres * (index + variation(route.roadId)) / count,
        speed: 4.5 + variation(route.roadId) * 2, colour: Math.floor(variation(id) * 4),
        model: models[Math.floor(variation(`${id}:model`) * models.length)] });
    }
  }

  function fallbackTemplate(colour: number) {
    const cached = templates.get(colour); if (cached) return cached;
    const root = new THREE.Group(), batches = new Map<string, { material: THREE.Material; parts: THREE.BufferGeometry[] }>();
    const box = new THREE.BoxGeometry(1, 1, 1), wheel = new THREE.CylinderGeometry(.5, .5, 1, 12);
    wheel.rotateZ(Math.PI / 2);
    const cabin = new THREE.BufferGeometry();
    const vertices = [[-.5, -.5, -.5], [.5, -.5, -.5], [-.5, -.5, .5], [.5, -.5, .5],
      [-.42, .5, -.35], [.42, .5, -.35], [-.42, .5, .26], [.42, .5, .26]];
    cabin.setAttribute("position", new THREE.Float32BufferAttribute(
      [0, 1, 3, 0, 3, 2, 0, 4, 5, 0, 5, 1, 2, 3, 7, 2, 7, 6,
        0, 2, 6, 0, 6, 4, 1, 5, 7, 1, 7, 3, 4, 6, 7, 4, 7, 5].flatMap(index => vertices[index]), 3));
    cabin.computeVertexNormals();
    const matrix = new THREE.Matrix4(), quaternion = new THREE.Quaternion();
    try {
      for (const part of companionCityVehicleParts(`city-car-paint-${colour}`, "sedan")) {
        const key = `${part.material}:${part.colour}`;
        let batch = batches.get(key);
        if (!batch) {
          const material = new THREE.MeshStandardMaterial({ color: part.colour,
            metalness: part.material === "metal" ? .55 : part.material === "vehiclePaint" ? .32 : .1,
            roughness: part.material === "vehicleGlass" ? .18 : part.material === "vehiclePaint" ? .3 : .8 });
          materials.add(material); batch = { material, parts: [] }; batches.set(key, batch);
        }
        if (part.geometry === "vehicleWheel") {
          const geometry = wheel.clone(); geometries.add(geometry);
          const mesh = new THREE.Mesh(geometry, batch.material); mesh.position.set(part.x, part.y, part.z);
          mesh.scale.set(part.w, part.h, part.d); mesh.userData.wheelRadius = part.h / 2;
          mesh.receiveShadow = true; root.add(mesh); continue;
        }
        const geometry = (part.geometry === "vehicleCabin" ? cabin : box).clone();
        quaternion.setFromEuler(new THREE.Euler(...(part.rotation ?? [0, 0, 0])));
        matrix.compose(new THREE.Vector3(part.x, part.y, part.z), quaternion, new THREE.Vector3(part.w, part.h, part.d));
        geometry.applyMatrix4(matrix); batch.parts.push(geometry);
      }
      for (const batch of batches.values()) {
        if (!batch.parts.length) continue;
        const geometry = mergeGeometries(batch.parts); if (!geometry) throw new Error("City car geometry could not be assembled.");
        geometries.add(geometry); const mesh = new THREE.Mesh(geometry, batch.material);
        mesh.receiveShadow = true; root.add(mesh);
      }
      templates.set(colour, root); return root;
    } finally { box.dispose(); wheel.dispose(); cabin.dispose(); batches.forEach(batch => batch.parts.forEach(part => part.dispose())); }
  }
  function releaseModel(body: MountedCar) {
    body.instance?.dispose(); body.instance = undefined;
    body.lease?.release(); body.lease = undefined; body.wheels.length = 0;
    body.fallback.traverse(node => { if (typeof node.userData.wheelRadius === "number")
      body.wheels.push({ node, base: new THREE.Quaternion(), radius: node.userData.wheelRadius }); });
    body.fallback.visible = true;
  }
  function release(body: MountedCar) { releaseModel(body); body.root.removeFromParent(); body.root.clear(); }
  function requestModel(body: MountedCar) {
    const model = body.car.model;
    if (!provider || body.lease || body.failed || !provider.available(model.key)) return;
    const lease = acquireCompanionGltf(model.key, provider); body.lease = lease;
    lease.promise.then(asset => {
      if (disposed || body.lease !== lease || mounted.get(body.car.id) !== body) return;
      const instance = instantiateGltf(asset);
      try {
        const nodes: THREE.Object3D[] = []; instance.scene.traverse(node => nodes.push(node));
        const root = nodes.find(node => node.userData.modelId === model.id);
        if (!root || root.userData.front !== "+Z" || root.userData.units !== "metres" || root.userData.runtimeAxisBakePending !== false)
          throw new Error("City car coordinate metadata differs.");
        const envelope = new THREE.Box3().setFromObject(instance.scene);
        if (envelope.isEmpty() || ![...envelope.min.toArray(), ...envelope.max.toArray()].every(Number.isFinite)
          || envelope.min.x < -model.halfWidth || envelope.max.x > model.halfWidth
          || envelope.min.z < -model.halfLength || envelope.max.z > model.halfLength
          || envelope.min.y < -.02 || envelope.max.y > 3) throw new Error("City car exceeds its admitted envelope.");
        const wheels: MountedCar["wheels"] = [];
        for (const node of nodes) {
          if (node instanceof THREE.Mesh) {
            const materials = Array.isArray(node.material) ? node.material : [node.material];
            node.userData.cityCarTransparent = materials.every(material => material.transparent);
            node.castShadow = body.shadows && !node.userData.cityCarTransparent; node.receiveShadow = true;
          }
          const name = typeof node.userData.name === "string" ? node.userData.name : node.name;
          const radius = node.userData.wheelRadius;
          if ((node.userData.lifeRole === "wheel" || name.includes(":wheel:"))
            && node.userData.spinAxis === "X" && Number.isFinite(radius) && radius > 0 && radius < 1)
            wheels.push({ node, base: node.quaternion.clone(), radius });
        }
        if (wheels.length !== 4) throw new Error("City car needs four wheel pivots.");
        body.wheels = wheels; body.instance = instance; body.root.add(instance.scene); body.fallback.visible = false;
      } catch (error) { instance.dispose(); throw error; }
      wake();
    }).catch(() => { if (!disposed && body.lease === lease) { releaseModel(body); body.failed = true; wake(); } });
  }

  // Swept full-body separation. Pedestrians use a small square; transit keeps
  // its actual width/length instead of blocking both lanes with a bus-sized disc.
  function conflict(from: TransitPoint, to: TransitPoint, model: CarModel, other: CityTrafficObstacle) {
    const rotationMargin = Math.hypot(model.halfWidth, model.halfLength) * (1 - Math.cos(angle(to.yaw - from.yaw) / 2));
    for (const yaw of [from.yaw, to.yaw, other.yaw]) for (const direction of [yaw, yaw + Math.PI / 2]) {
      const x = Math.sin(direction), z = Math.cos(direction);
      const extent = (heading: number, width: number, length: number) =>
        Math.abs(x * Math.cos(heading) - z * Math.sin(heading)) * width
        + Math.abs(x * Math.sin(heading) + z * Math.cos(heading)) * length;
      const a = from.x * x + from.z * z, b = to.x * x + to.z * z, centre = other.x * x + other.z * z;
      const margin = Math.max(extent(from.yaw, model.halfWidth, model.halfLength), extent(to.yaw, model.halfWidth, model.halfLength))
        + rotationMargin + extent(other.yaw, other.halfWidth, other.halfLength) + .5;
      if (Math.max(a, b) < centre - margin || Math.min(a, b) > centre + margin) return false;
    }
    return true;
  }
  return {
    group,
    walkingBodies(): readonly CityTrafficObstacle[] {
      if (disposed || !group.visible) return [];
      return [...mounted.values()].filter(body => body.root.visible).map(body => ({ ...body.point,
        halfWidth: body.car.model.halfWidth, halfLength: body.car.model.halfLength }));
    },
    get moving() { return moving; },
    get nextWakeAt() { return !disposed && fleet.length && group.visible && !reduced ? lastFleetAt + 250 : Infinity; },
    update(now: number, viewer: THREE.Vector3, reducedMotion: boolean, snapshot: CompanionWorldSnapshot | undefined,
      transit: readonly CityTrafficObstacle[] = []) {
      if (disposed || !Number.isSafeInteger(now) || now < epochMs) return false;
      const active = snapshot?.player?.status === "present" && snapshot.player.instanceId === snapshot.instance?.id;
      if (group.visible !== active) { group.visible = active; dirty = true; }
      moving = false;
      if (!active) {
        mounted.forEach(release); mounted.clear(); lastNow = NaN; lastFleetAt = -Infinity;
        const result = dirty; dirty = false; return result;
      }
      const nextProvider = delivery();
      if (provider !== nextProvider) { mounted.forEach(body => { releaseModel(body); body.failed = false; }); provider = nextProvider; dirty = true; }
      if (reduced !== reducedMotion) {
        mounted.forEach(release); mounted.clear(); reduced = reducedMotion;
        lastFleetAt = -Infinity; lastNow = NaN; dirty = true;
      }
      const delta = Number.isFinite(lastNow) && now >= lastNow && now - lastNow <= 1000 ? Math.min(.25, (now - lastNow) / 1000) : 0;
      lastNow = now;
      const blockers: CityTrafficObstacle[] = [...transit, { x: viewer.x, z: viewer.z, yaw: 0, halfWidth: .8, halfLength: .8 }];
      for (const player of snapshot?.peers ?? []) {
        if (player.status !== "present" || player.instanceId !== snapshot?.instance?.id) continue;
        const point = player.driving?.pose ?? (player.movement ? sampleCompanionMovement(player.movement, now).position : player.position);
        blockers.push({ ...point, halfWidth: player.driving ? 1.4 : .8, halfLength: player.driving ? 2.8 : .8 });
      }
      const driving = snapshot?.player?.driving?.pose;
      if (driving) blockers.push({ ...driving, halfWidth: 1.4, halfLength: 2.8 });
      const candidates: { car: Car; point: TransitPoint; distance: number }[] = [];
      if (now < lastFleetAt || now - lastFleetAt >= 250 || lastViewer.distanceToSquared(viewer) > 64) {
        for (const car of fleet) {
          const retained = mounted.get(car.id);
          const metres = reduced ? car.offset : car.offset + (now - epochMs) / 1000 * car.speed;
          const point = retained?.point ?? transitRoutePoint(car.route, wrap(metres, car.route.metres));
          const distance = Math.hypot(point.x - viewer.x, point.z - viewer.z);
          if (distance < (retained ? RELEASE : ENTER)) candidates.push({ car, point, distance });
        }
        candidates.sort((a, b) => a.distance - (mounted.has(a.car.id) ? 40 : 0)
          - b.distance + (mounted.has(b.car.id) ? 40 : 0) || a.car.id.localeCompare(b.car.id));
        const selected = new Set(candidates.slice(0, MAX_VISIBLE).map(value => value.car.id));
        mounted.forEach((body, id) => { if (!selected.has(id)) { release(body); mounted.delete(id); dirty = true; } });
        for (const { car, point } of candidates.slice(0, MAX_VISIBLE)) if (!mounted.has(car.id)) {
          if (blockers.some(other => conflict(point, point, car.model, other))
            || [...mounted.values()].some(other => conflict(point, point, car.model,
              { ...other.point, halfWidth: other.car.model.halfWidth, halfLength: other.car.model.halfLength }))) continue;
          const root = new THREE.Group(), fallback = fallbackTemplate(car.colour).clone();
          root.name = car.id; root.add(fallback); group.add(root);
          const wheels: MountedCar["wheels"] = [];
          fallback.traverse(node => { if (typeof node.userData.wheelRadius === "number")
            wheels.push({ node, base: node.quaternion.clone(), radius: node.userData.wheelRadius }); });
          mounted.set(car.id, { car, root, fallback, point,
            metres: wrap(reduced ? car.offset : car.offset + (now - epochMs) / 1000 * car.speed, car.route.metres), wheelTravel: 0,
            failed: false, shadows: false, wheels }); dirty = true;
        }
        lastFleetAt = now; lastViewer.copy(viewer);
      }
      const bodies = [...mounted.values()].sort((a, b) =>
        Math.hypot(a.point.x - viewer.x, a.point.z - viewer.z) - (a.lease ? 15 : 0)
        - Math.hypot(b.point.x - viewer.x, b.point.z - viewer.z) + (b.lease ? 15 : 0) || a.car.id.localeCompare(b.car.id));
      let models = 0;
      for (const body of bodies) {
        const station = wrap(body.metres, body.car.route.metres);
        const found = body.car.route.lengths.findIndex(length => length > station + 1e-7);
        const segment = found > 0 ? found : body.car.route.lengths.length - 1;
        const end = body.car.route.points[segment], start = body.car.route.points[segment - 1];
        const heading = Math.atan2(end.x - start.x, end.z - start.z);
        const nextMetres = body.metres + (!reduced ? Math.min(delta * body.car.speed, Math.max(0, body.car.route.lengths[segment] - station)) : 0);
        const proposed = transitRoutePoint(body.car.route, wrap(nextMetres, body.car.route.metres));
        // Stop at the admitted vertex, then rotate there before the next leg.
        const turn = angle(heading - body.point.yaw);
        const safeTurn = reduced || delta === 0 || Math.abs(turn) <= 1.5 * delta;
        const next = safeTurn ? { ...proposed, yaw: heading }
          : { ...body.point, yaw: body.point.yaw + Math.sign(turn) * 1.5 * delta };
        const occupied = blockers.some(other => conflict(body.point, next, body.car.model, other))
          || bodies.some(other => other !== body && conflict(body.point, next, body.car.model,
            { ...other.point, halfWidth: other.car.model.halfWidth, halfLength: other.car.model.halfLength }));
        // Mounted cars stay solid and visible. Walking routes use this same body.
        const visible = body.root.visible;
        if (!reduced && delta > 0 && !occupied) {
          if (safeTurn) {
            body.wheelTravel = wrap(body.wheelTravel + nextMetres - body.metres, 1000);
            body.metres = wrap(nextMetres, body.car.route.metres);
          }
          body.point = next; moving = true;
        }
        body.root.position.set(body.point.x, navigation.floorY, body.point.z); body.root.rotation.y = body.point.yaw;
        const distance = Math.hypot(body.point.x - viewer.x, body.point.z - viewer.z);
        const shadows = distance < (body.shadows ? 100 : 75);
        if (shadows !== body.shadows) { body.shadows = shadows; body.root.traverse(node => {
          if (node instanceof THREE.Mesh) node.castShadow = shadows && !node.userData.cityCarTransparent;
        }); dirty = true; }
        const detail = visible && distance < (body.lease ? MODEL_RELEASE : MODEL_ENTER) && models < MAX_MODELS;
        if (detail) { models++; requestModel(body); }
        else if (body.lease) { releaseModel(body); body.failed = false; dirty = true; }
        for (const wheel of body.wheels) wheel.node.quaternion.copy(wheel.base).multiply(spin.setFromAxisAngle(axle, wrap(body.wheelTravel / wheel.radius, Math.PI * 2)));
      }
      const result = dirty || moving; dirty = false; return result;
    },
    dispose() {
      if (disposed) return; disposed = true; moving = false;
      mounted.forEach(release); mounted.clear(); fleet.length = 0; templates.clear();
      geometries.forEach(geometry => geometry.dispose()); materials.forEach(material => material.dispose());
      group.removeFromParent(); group.clear();
    },
  };
}
