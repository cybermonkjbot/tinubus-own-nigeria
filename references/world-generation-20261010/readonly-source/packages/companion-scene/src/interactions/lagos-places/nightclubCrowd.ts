import * as THREE from "three";
import { clone as cloneSkeleton } from "three/addons/utils/SkeletonUtils.js";
import { mergeGeometries } from "three/addons/utils/BufferGeometryUtils.js";
import { shareClonedSkeletons } from "../../../../../packages/character-runtime/src/skeletonSharing";
import type { CompanionAssetKey } from "../../../../../packages/utils/src/companionAssets";
import type { CompanionPlaceFixture, CompanionPlaceLayout } from "../../../../../packages/utils/src/companionPlaces";
import { nightclubLimits, nightclubSeats } from "../../../../../packages/utils/src/companionNightclub";
import { planRoomRoute, routeSegmentClear, type ScenePoint } from "../../../../../packages/utils/src/companionNavigation";
import type { EnvironmentAsset, EnvironmentParticipant } from "../scenes/environments";
import { disposeLagosAssetPart } from "./assetParts";

type Role = "dancer" | "choreography" | "guest" | "dj" | "bottle-girl" | "hypeman";
type JointPose = Readonly<{ position: THREE.Vector3; quaternion: THREE.Quaternion; scale: THREE.Vector3 }>;
type Joint = JointPose & { node: THREE.Bone };
type PreparedCivilian = { rest: readonly JointPose[]; wrappers: Map<THREE.BufferGeometry, THREE.BufferGeometry> };
type PropKind = "bottle" | "microphone" | "headphones" | "glass";
type PropPart = { geometry: THREE.BufferGeometry; material: THREE.Material };
type Route = { points: readonly ScenePoint[]; lengths: number[]; total: number };
type Limb = { upper: THREE.Bone; lower: THREE.Bone; tip: THREE.Bone; chained: boolean };
type ActorSide = { sign: number; arm?: Limb; leg?: Limb };
type CrowdActor = {
  group: THREE.Group; joints: Joint[]; hips?: THREE.Bone; sides: readonly ActorSide[]; role: Role; phase: number;
  home: THREE.Vector3; yaw: number; height: number; route?: Route; seatId?: string;
  prop?: THREE.Group; hand?: THREE.Bone;
};
const limits = { actors: 30, guestsPerSofa: 2, routePoints: 512 } as const;
const clothes = ["#24202e", "#eee1cd", "#4d254e", "#183b3b", "#944b58", "#bba16b"];
// Linear albedo gains retain the shared skin map's facial and hand detail.
const complexions = [
  [.55, .48, .44], [.74, .64, .58], [.95, .82, .75],
  [1.15, 1, .88], [1.38, 1.2, 1.06], [1.68, 1.48, 1.32],
  [2.05, 1.9, 1.73],
] as const;

/** Visual nightlife only. No synthetic players, service receipts or account writes. */
export function createNightclubCrowd(layout: CompanionPlaceLayout, assets: ReadonlyMap<CompanionAssetKey, EnvironmentAsset>) {
  const group = new THREE.Group(); group.name = `${layout.id}:club-crowd`;
  group.visible = false;
  const actors: CrowdActor[] = [], materials = new Set<THREE.Material>(), geometries = new Set<THREE.BufferGeometry>();
  const sources = new Map<string, THREE.Object3D>();
  const civilians = new Map<THREE.Object3D, PreparedCivilian>(), propParts = new Map<PropKind, readonly PropPart[]>();
  const asset = assets.get("bestieTownSquareNpcs");
  asset?.scene.updateWorldMatrix(true, true);
  asset?.scene.traverse(node => { if (node.userData.lifeRole === "npc" && typeof node.userData.npcId === "string") sources.set(node.userData.npcId, node); });
  const local = new THREE.Vector3(), head = new THREE.Vector3(), end = new THREE.Vector3(), joint = new THREE.Vector3();
  const direction = new THREE.Vector3(), pole = new THREE.Vector3(), axis = new THREE.Vector3(0, 1, 0), target = new THREE.Vector3();
  const currentRotation = new THREE.Quaternion(), parentRotation = new THREE.Quaternion(), turn = new THREE.Quaternion();
  const decomposedPosition = new THREE.Vector3(), decomposedScale = new THREE.Vector3(), inverseWorld = new THREE.Matrix4();
  let sampledAt = -1, wasReduced = false, wasVisible = false, disposed = false;
  const mat = (color: string, metalness = 0, roughness = .6) => {
    const value = new THREE.MeshStandardMaterial({ color, metalness, roughness }); materials.add(value); return value;
  };
  const gold = mat("#c8a25f", .8, .22), green = mat("#244e3b", .28, .16), black = mat("#17171f", .2, .38);
  const glass = mat("#a8c9cd", .15, .18), ivory = mat("#f1e4ca");
  function prepareCivilian(source: THREE.Object3D): PreparedCivilian {
    const previous = civilians.get(source); if (previous) return previous;
    // Sources stay immutable throughout this synchronous crowd construction.
    const originalBones: THREE.Bone[] = [], bindWorld = new Map<THREE.Bone, THREE.Matrix4>();
    source.traverse(node => { if (node instanceof THREE.Bone) originalBones.push(node); });
    source.traverse(node => { if (node instanceof THREE.SkinnedMesh) node.skeleton.bones.forEach((bone, index) => {
      if (!bindWorld.has(bone)) bindWorld.set(bone, node.skeleton.boneInverses[index].clone().invert());
    }); });
    const transform = new THREE.Matrix4(), parentBind = new THREE.Matrix4(), rest: JointPose[] = [];
    for (const original of originalBones) {
      const bind = bindWorld.get(original);
      if (!bind) throw new Error("Club crowd requires a complete civilian skeleton.");
      const parent = original.parent instanceof THREE.Bone ? bindWorld.get(original.parent) : original.parent?.matrixWorld;
      if (!parent) throw new Error("Club crowd requires a bound skeleton parent.");
      const pose = { position: new THREE.Vector3(), quaternion: new THREE.Quaternion(), scale: new THREE.Vector3() };
      parentBind.copy(parent);
      transform.copy(parentBind).invert().multiply(bind).decompose(pose.position, pose.quaternion, pose.scale);
      rest.push(pose);
    }
    const prepared: PreparedCivilian = { rest, wrappers: new Map() };
    civilians.set(source, prepared); return prepared;
  }
  function prepareProp(kind: PropKind): readonly PropPart[] {
    const previous = propParts.get(kind); if (previous) return previous;
    const batches = new Map<THREE.Material, THREE.BufferGeometry[]>();
    function add(geometry: THREE.BufferGeometry, material: THREE.Material, x: number, y: number, z: number) {
      geometry.translate(x, y, z); const batch = batches.get(material) ?? []; batch.push(geometry); batches.set(material, batch);
    }
    const cylinder = (material: THREE.Material, top: number, bottom: number, height: number, x: number, y: number, z: number) =>
      add(new THREE.CylinderGeometry(top, bottom, height, 12), material, x, y, z);
    try {
      if (kind === "bottle") {
        cylinder(green, .052, .067, .27, 0, -.075, 0);
        cylinder(green, .024, .052, .065, 0, .0925, 0);
        cylinder(gold, .024, .024, .11, 0, .18, 0);
        cylinder(ivory, .068, .068, .08, 0, -.065, 0);
        cylinder(gold, .026, .026, .028, 0, .249, 0);
      } else if (kind === "microphone") {
        cylinder(black, .025, .029, .19, 0, -.035, 0);
        add(new THREE.SphereGeometry(.044, 12, 8), glass, 0, .1, 0);
        cylinder(gold, .026, .026, .014, 0, .035, 0);
      } else if (kind === "glass") {
        cylinder(glass, .045, .035, .13, 0, .035, 0);
        cylinder(gold, .009, .009, .09, 0, -.075, 0);
      } else {
        const band = new THREE.TorusGeometry(.12, .015, 6, 20, Math.PI);
        add(band, black, 0, 0, 0);
        for (const side of [-1, 1]) add(new THREE.BoxGeometry(.047, .09, .06), gold, side * .105, -.05, 0);
      }
      const parts: PropPart[] = [];
      for (const [material, batch] of batches) {
        const geometry = mergeGeometries(batch, false);
        if (!geometry) throw new Error(`Cannot assemble ${kind}.`);
        geometries.add(geometry); parts.push({ geometry, material });
      }
      propParts.set(kind, parts); return parts;
    } finally {
      for (const batch of batches.values()) for (const geometry of batch) geometry.dispose();
    }
  }
  function prop(kind: PropKind) {
    const result = new THREE.Group(); result.name = `club-${kind}`;
    for (const { geometry, material } of prepareProp(kind)) {
      const mesh = new THREE.Mesh(geometry, material); mesh.raycast = () => {}; result.add(mesh);
    }
    return result;
  }
  function prepareLimb(bones: ReadonlyMap<string, THREE.Bone>, side: ".L" | ".R", part: "arm" | "leg"): Limb | undefined {
    const upper = bones.get(`${part === "arm" ? "upperarm" : "upperleg"}${side}`);
    const lower = bones.get(`${part === "arm" ? "lowerarm" : "lowerleg"}${side}`);
    const tip = bones.get(`${part === "arm" ? "wrist" : "foot"}${side}`);
    if (!upper || !lower || !tip) return;
    const below = (node: THREE.Bone, ancestor: THREE.Bone) => {
      for (let parent = node.parent; parent; parent = parent.parent) if (parent === ancestor) return true;
      return false;
    };
    // This factory owns the hierarchy after construction. Intervening bones
    // are supported; unusual disconnected limbs retain individual refreshes.
    return { upper, lower, tip, chained: below(lower, upper) && below(tip, lower) };
  }
  function addActor(role: Role, point: ScenePoint, yaw: number, female = false, seatId?: string, route?: Route) {
    if (actors.length >= limits.actors) return;
    const source = sources.get(female ? "akara-woman" : actors.length % 3 ? "street-tout-shirt" : "street-tout-cap");
    if (!source) return;
    const model = cloneSkeleton(source), root = new THREE.Group(); root.add(model);
    model.traverse(node => {
      delete node.userData.lifeRole; delete node.userData.npcId;
      delete node.userData.companionResidentId;
    });
    try {
      // Recover bind-local bones, not the vendor's baked seated/work pose.
      const prepared = prepareCivilian(source), clonedBones: THREE.Bone[] = [];
      model.traverse(node => { if (node instanceof THREE.Bone) clonedBones.push(node); });
      const bones = new Map<string, THREE.Bone>();
      for (let i = 0; i < prepared.rest.length; i++) {
        const pose = prepared.rest[i], node = clonedBones[i];
        if (!node) throw new Error("Club crowd requires a complete civilian skeleton.");
        node.position.copy(pose.position); node.quaternion.copy(pose.quaternion); node.scale.copy(pose.scale);
        bones.set(typeof node.userData.name === "string" ? node.userData.name : node.name, node);
      }
      const skin = female ? "akara-woman" : source.userData.npcId as string;
      const complexionIndex = (actors.length * 5 + 2) % complexions.length;
      const complexion = complexions[complexionIndex];
      model.position.set(0, 0, 0); model.rotation.set(0, 0, 0); model.scale.setScalar(1);
      // Keep the female body and fitted wrapper, remove cooking equipment.
      const remove: THREE.Object3D[] = [], wrappers: THREE.SkinnedMesh[] = [];
      model.traverse(node => {
        if (!(node instanceof THREE.Mesh)) return;
        const name = typeof node.userData.name === "string" ? node.userData.name : node.name;
        const retain = node instanceof THREE.SkinnedMesh && (name.startsWith(`${skin} `) || female && name === "Akara seated wrapper");
        if (!retain) { remove.push(node); return; }
        node.castShadow = false; node.receiveShadow = true; node.raycast = () => {};
        const skinBody = name === `${skin} body`;
        if (skinBody || name.endsWith(" shirt") || name === "Akara seated wrapper" || name.endsWith(" wrap")) {
          const originals = Array.isArray(node.material) ? node.material : [node.material];
          const owned = originals.map(value => {
            const copy = value.clone(); materials.add(copy);
            if (copy instanceof THREE.MeshStandardMaterial) {
              if (skinBody) {
                copy.color.multiply(new THREE.Color().setRGB(complexion[0], complexion[1], complexion[2], THREE.LinearSRGBColorSpace));
              } else {
                copy.map = null;
                copy.color.set(role === "bottle-girl" ? "#1b1824" : clothes[actors.length % clothes.length]);
                copy.roughness = .55;
              }
            }
            return copy;
          });
          node.material = Array.isArray(node.material) ? owned : owned[0];
        }
        // The published seated wrapper is rigidly weighted to the hips. Anchor
        // it there in bind space so standing staff do not wear a floating skirt.
        if (female && name === "Akara seated wrapper") {
          wrappers.push(node);
        }
      });
      const hips = bones.get("root");
      if (hips) for (const node of wrappers) {
        let wrapper = prepared.wrappers.get(node.geometry);
        if (!wrapper) {
          wrapper = node.geometry.clone(); geometries.add(wrapper);
          wrapper.applyMatrix4(source.matrixWorld.clone().invert()); prepared.wrappers.set(node.geometry, wrapper);
        }
        const fitted = new THREE.Mesh(wrapper, node.material); fitted.name = "club-fitted-wrapper";
        fitted.raycast = () => {}; fitted.receiveShadow = true;
        model.updateWorldMatrix(true, true); hips.attach(fitted); remove.push(node);
      }
      remove.forEach(node => node.removeFromParent());
      shareClonedSkeletons(model);
      const sides: ActorSide[] = ([".L", ".R"] as const).map(side => ({
        sign: side === ".L" ? 1 : -1, arm: prepareLimb(bones, side, "arm"), leg: prepareLimb(bones, side, "leg"),
      }));
      const controlled = new Set<THREE.Bone>();
      if (hips) controlled.add(hips);
      for (const side of sides) for (const chain of [side.arm, side.leg]) {
        if (chain) { controlled.add(chain.upper); controlled.add(chain.lower); controlled.add(chain.tip); }
      }
      // Every bone receives its bind pose above. Only hips and solved limbs
      // change afterward; this decorative crowd has no other animation writer.
      const joints: Joint[] = [];
      for (let i = 0; i < prepared.rest.length; i++) {
        const node = clonedBones[i];
        if (node && controlled.has(node)) joints.push({ node, ...prepared.rest[i] });
      }
      root.name = `club-${role}-${actors.length}`; root.userData.nightclubRole = role;
      root.userData.nightclubComplexion = complexionIndex;
      root.position.fromArray(point); root.rotation.y = yaw;
      const actor: CrowdActor = { group: root, joints, hips, sides, role, phase: actors.length * 2.399963,
        home: new THREE.Vector3().fromArray(point), yaw, height: female ? 1.64 : skin === "street-tout-shirt" ? 1.87 : 1.72, seatId, route };
      if (role === "bottle-girl" || role === "hypeman" || role === "guest") {
        actor.prop = prop(role === "bottle-girl" ? "bottle" : role === "hypeman" ? "microphone" : "glass");
        actor.hand = bones.get("wrist.R"); root.add(actor.prop);
      }
      if (role === "dj") {
        const headphones = prop("headphones"); headphones.position.set(0, actor.height - .085, 0); root.add(headphones);
      }
      group.add(root); actors.push(actor);
    } catch (error) { disposeLagosAssetPart(root); throw error; }
  }
  function point(fixture: CompanionPlaceFixture, x: number, z: number, y = layout.navigation.floorY): ScenePoint {
    return [fixture.point[0] + x * Math.cos(fixture.yaw) + z * Math.sin(fixture.yaw), y,
      fixture.point[2] - x * Math.sin(fixture.yaw) + z * Math.cos(fixture.yaw)];
  }
  const clear = (value: ScenePoint) => routeSegmentClear(layout.navigation, value, value)
    && layout.services.every(service => Math.hypot(value[0] - service.anchor.point[0], value[2] - service.anchor.point[2]) > .9
      && service.queue.every(queue => Math.hypot(value[0] - queue[0], value[2] - queue[2]) > .8));
  function route(waypoints: readonly ScenePoint[]): Route | undefined {
    const points: ScenePoint[] = [];
    for (let i = 0; i < waypoints.length; i++) {
      const path = planRoomRoute(layout.navigation, waypoints[i], waypoints[(i + 1) % waypoints.length]);
      if (!path || !path.length || points.length + path.length > limits.routePoints) return;
      if (!points.length) points.push(waypoints[i]);
      points.push(...path);
    }
    const lengths: number[] = []; let total = 0;
    for (let i = 1; i < points.length; i++) { total += Math.hypot(points[i][0] - points[i - 1][0], points[i][2] - points[i - 1][2]); lengths.push(total); }
    return total > 0 ? { points, lengths, total } : undefined;
  }
  try {
    if (layout.kind === "nightclub" && asset) {
      const floor = layout.fixtures.find(fixture => fixture.id === "dance-floor");
      if (floor) {
        for (const [i, offset] of [[-3.2, -3.2], [3.2, -2.7], [-3.3, -1.2], [-1.9, -.7],
          [1.8, -.9], [3.2, .2], [-3.1, .7], [3.3, 2.5]].entries()) {
          const home = point(floor, offset[0], offset[1]);
          if (clear(home)) addActor("dancer", home, i % 2 ? -.65 : .65, i % 3 === 1);
        }
        for (const [i, offset] of [[-1.2, -3.3], [.4, -3.3], [-1.2, -1.9], [.4, -1.9]].entries()) {
          const home = point(floor, offset[0], offset[1]);
          if (clear(home)) addActor("choreography", home, floor.yaw, i % 2 === 0);
        }
      }
      const booth = layout.fixtures.find(fixture => fixture.id === "dj-booth");
      if (booth) addActor("dj", point(booth, 0, -.78), booth.yaw);
      const stage = layout.fixtures.find(fixture => fixture.id === "dj-stage");
      if (stage) addActor("hypeman", point(stage, stage.size[0] * .32, 0), stage.yaw);
      const seats = nightclubSeats(layout);
      for (const sofa of layout.fixtures.filter(fixture => fixture.id.startsWith("club-lounge"))) {
        const cushions = seats.filter(seat => seat.id.startsWith(`${sofa.id}:`));
        // Leave most seats available to players. Occupied cushions hide their guest.
        for (const seat of cushions.filter((_, index) => index === 0 || index === cushions.length - 1).slice(0, limits.guestsPerSofa)) {
          addActor("guest", [seat.position[0], layout.navigation.floorY, seat.position[2]], seat.yaw, actors.length % 2 === 0, seat.id);
        }
      }
      const tables = layout.fixtures.filter(fixture => fixture.id.startsWith("club-table"));
      for (const table of tables) for (const sign of [-1, 1]) {
        const home = point(table, .2, sign * 1.1);
        if (clear(home)) addActor("dancer", home, Math.atan2(table.point[0] - home[0], table.point[2] - home[2]), sign < 0);
      }
      const bar = layout.fixtures.find(fixture => fixture.id === "club-bar");
      if (bar && tables.length) {
        const start = point(bar, -Math.sign(bar.point[0]) * (bar.size[0] / 2 + .85), .5);
        const stops = tables.map(table => point(table, 0, table.point[2] < 0 ? 1.6 : -1.6));
        const path = route([start, ...stops]);
        if (path) for (let i = 0; i < 3; i++) addActor("bottle-girl", start, 0, true, undefined, path);
      }
    }
  } catch (error) {
    actors.forEach(actor => disposeLagosAssetPart(actor.group)); geometries.forEach(geometry => geometry.dispose()); materials.forEach(material => material.dispose());
    group.clear(); throw error;
  } finally {
    // No actors are added after construction. Release lookup tables; meshes
    // and read-only rest values keep their own lifetimes.
    civilians.clear(); propParts.clear(); sources.clear();
  }

  function aim(node: THREE.Bone | undefined, target: THREE.Vector3) {
    if (!node?.parent) return;
    // Refresh this chain once. Three's world getters would repeat that walk.
    node.updateWorldMatrix(true, false);
    node.matrixWorld.decompose(head, currentRotation, decomposedScale);
    direction.subVectors(target, head).normalize(); local.copy(axis).applyQuaternion(currentRotation);
    turn.setFromUnitVectors(local, direction).multiply(currentRotation);
    node.parent.matrixWorld.decompose(decomposedPosition, parentRotation, decomposedScale);
    node.quaternion.copy(parentRotation.invert().multiply(turn));
    // The next joint solve refreshes its own ancestors. Publish all remaining
    // descendants once in updateActor, after the last limb is solved.
  }
  function limb(actor: CrowdActor, chain: Limb | undefined, part: "arm" | "leg", x: number, y: number, z: number, px: number, py: number, pz: number) {
    if (!chain) return;
    const { upper, lower, tip } = chain;
    if (chain.chained) {
      // Refreshing the tip also refreshes both verified ancestors.
      tip.updateWorldMatrix(true, false);
      head.setFromMatrixPosition(upper.matrixWorld); joint.setFromMatrixPosition(lower.matrixWorld); end.setFromMatrixPosition(tip.matrixWorld);
    } else {
      upper.getWorldPosition(head); lower.getWorldPosition(joint); tip.getWorldPosition(end);
    }
    const a = head.distanceTo(joint), b = joint.distanceTo(end);
    end.set(x, y, z).applyMatrix4(actor.group.matrixWorld); pole.set(px, py, pz).applyMatrix4(actor.group.matrixWorld);
    direction.subVectors(end, head); const distance = Math.max(.001, Math.min(direction.length(), a + b - .001)); direction.normalize();
    const reach = (a * a - b * b + distance * distance) / (2 * distance);
    const bend = Math.sqrt(Math.max(0, a * a - reach * reach));
    pole.sub(head); pole.addScaledVector(direction, -pole.dot(direction)).normalize();
    joint.copy(head).addScaledVector(direction, reach).addScaledVector(pole, bend);
    end.copy(head).addScaledVector(direction, distance);
    aim(upper, joint); aim(lower, end);
    // Refresh the just-rotated chain before choosing the hand/foot direction.
    tip.getWorldPosition(target);
    if (part === "leg") {
      local.set(0, 0, 1).transformDirection(actor.group.matrixWorld);
      target.add(local); aim(tip, target);
    } else {
      local.set(0, 1, 0).transformDirection(actor.group.matrixWorld);
      target.add(local); aim(tip, target);
    }
  }
  function actorVisible(actor: CrowdActor, participants: readonly EnvironmentParticipant[], viewer?: ScenePoint) {
    const position = actor.group.position;
    const nearby = !viewer || Math.hypot(position.x - viewer[0], position.z - viewer[2]) <= 22;
    const occupied = participants.some(person => Math.hypot(position.x - person.x, position.z - person.z) < (actor.seatId ? .75 : person.radius + .42));
    return nearby && !occupied;
  }
  function updateActor(actor: CrowdActor, seconds: number, reducedMotion: boolean,
    participants: readonly EnvironmentParticipant[], viewer?: ScenePoint) {
    const beat = seconds / (nightclubLimits.beatMs / 1000);
    const phrase = beat % 80;
    const choreography = actor.role === "choreography" && !reducedMotion && phrase >= 48;
    const weight = choreography ? THREE.MathUtils.smoothstep(Math.min(phrase - 48, 80 - phrase), 0, 2) : 0;
    const routine = choreography ? Math.floor((phrase - 48) / 8) % 4 : -1;
    actor.group.position.copy(actor.home); actor.group.rotation.y = actor.yaw;
    if (choreography) {
      const sideStep = Math.sin(beat * Math.PI / 2) * .17 * weight;
      actor.group.position.x += Math.cos(actor.yaw) * sideStep;
      actor.group.position.z -= Math.sin(actor.yaw) * sideStep;
      actor.group.rotation.y += routine === 2 ? Math.sin(beat * Math.PI / 4) * .45 * weight : 0;
    }
    if (actor.route) {
      const walkingSeconds = reducedMotion ? actor.phase * 4 : seconds + actor.phase * 4;
      const distance = (walkingSeconds * .85) % actor.route.total;
      const index = actor.route.lengths.findIndex(length => length >= distance);
      const previous = index > 0 ? actor.route.lengths[index - 1] : 0;
      const from = actor.route.points[index], to = actor.route.points[index + 1];
      const amount = (distance - previous) / Math.max(.001, actor.route.lengths[index] - previous);
      actor.group.position.set(from[0] + (to[0] - from[0]) * amount, layout.navigation.floorY, from[2] + (to[2] - from[2]) * amount);
      actor.group.rotation.y = Math.atan2(to[0] - from[0], to[2] - from[2]);
    }
    // Keep route/choreography positions current, but solve limbs only for an
    // admitted body. Re-entry restores its current absolute pose before drawing.
    if (!actorVisible(actor, participants, viewer)) return false;
    const groove = reducedMotion ? 0 : THREE.MathUtils.lerp(Math.sin(beat * Math.PI + actor.phase), Math.sin(beat * Math.PI), weight);
    const bounce = reducedMotion ? 0 : .025 * (1 - THREE.MathUtils.lerp(Math.cos(beat * Math.PI * 2 + actor.phase), Math.cos(beat * Math.PI * 2), weight));
    for (const value of actor.joints) {
      value.node.position.copy(value.position); value.node.quaternion.copy(value.quaternion); value.node.scale.copy(value.scale);
    }
    actor.group.updateWorldMatrix(true, true);
    const hips = actor.hips;
    if (hips?.parent) {
      local.setFromMatrixPosition(hips.matrixWorld);
      if (actor.role === "guest") local.y = layout.navigation.floorY + .62;
      else local.y += bounce - (actor.role === "dancer" || actor.role === "choreography" || actor.role === "hypeman" ? .045 : 0);
      if (actor.role === "dancer" || actor.role === "choreography") local.x += groove * .035;
      local.applyMatrix4(inverseWorld.copy(hips.parent.matrixWorld).invert());
      hips.position.copy(local);
    }
    const h = actor.height;
    for (const side of actor.sides) {
      const sign = side.sign;
      const stride = actor.route && !reducedMotion ? Math.sin(seconds * Math.PI * 2 / 1.2 + actor.phase + (sign < 0 ? Math.PI : 0)) : 0;
      limb(actor, side.leg, "leg", sign * .13, .09 + Math.max(0, stride) * .07,
        actor.role === "guest" ? .43 : stride * .22, sign * .18, .5, 1);
      if (actor.role === "guest") limb(actor, side.arm, "arm", sign * .22, sign < 0 ? 1.03 : .81, .26, sign * .48, .9, .12);
      else if (actor.role === "dj") limb(actor, side.arm, "arm", sign * (.34 + groove * .035), 1.29, .51, sign * .52, 1.1, .22);
      else if (actor.role === "bottle-girl") limb(actor, side.arm, "arm", sign * .26, sign < 0 ? h * .82 : h * .44, sign < 0 ? .20 : -stride * .16, sign * .46, 1.12, .08);
      else if (actor.role === "hypeman") limb(actor, side.arm, "arm", sign < 0 ? -.13 : .35 + groove * .06,
        sign < 0 ? h * .90 : h * .98 + groove * .12, sign < 0 ? .20 : .05, sign * .5, h * .7, .15);
      else limb(actor, side.arm, "arm", sign * (.3 + groove * .065),
        h * THREE.MathUtils.lerp(.61 + (actor.phase % 3 < 1 ? .2 : 0), routine === 1 ? .9 : routine === 3 ? .73 : .62, weight) + sign * groove * .10,
        THREE.MathUtils.lerp(.2, routine === 3 ? .42 : .2, weight), sign * .55, h * .65, .05);
    }
    // Hip/limb solves leave unrelated descendants pending. Refresh the complete
    // actor before hand-prop reads and before the caller can draw it.
    actor.group.updateWorldMatrix(true, true);
    if (actor.prop && actor.hand) {
      local.setFromMatrixPosition(actor.hand.matrixWorld).applyMatrix4(inverseWorld.copy(actor.group.matrixWorld).invert());
      actor.prop.position.copy(local);
      actor.prop.rotation.set(actor.role === "hypeman" ? .7 : -.08, 0, 0);
    }
    return true;
  }
  return {
    group,
    update(elapsedSeconds: number, visible: boolean, reducedMotion: boolean, participants: readonly EnvironmentParticipant[] = [], viewer?: ScenePoint) {
      if (disposed || !Number.isFinite(elapsedSeconds) || elapsedSeconds < 0) return false;
      const visibilityChanged = group.visible !== visible;
      group.visible = visible;
      if (!visible) { wasVisible = false; return false; }
      const sample = reducedMotion ? 0 : Math.floor(elapsedSeconds * 20);
      let changed = visibilityChanged || !wasVisible || wasReduced !== reducedMotion || sampledAt !== sample;
      const reposed = changed; wasVisible = true; wasReduced = reducedMotion; sampledAt = sample;
      for (const actor of actors) {
        const atHome = actor.home;
        if (!actor.route && viewer && Math.hypot(atHome.x - viewer[0], atHome.z - viewer[2]) > 22) {
          changed ||= actor.group.visible; actor.group.visible = false; continue;
        }
        const show = reposed || !actor.group.visible
          ? updateActor(actor, sample / 20, reducedMotion, participants, viewer)
          : actorVisible(actor, participants, viewer);
        changed ||= actor.group.visible !== show; actor.group.visible = show;
      }
      return changed;
    },
    dispose() {
      if (disposed) return; disposed = true;
      actors.forEach(actor => disposeLagosAssetPart(actor.group)); actors.length = 0;
      geometries.forEach(geometry => geometry.dispose()); materials.forEach(material => material.dispose());
      geometries.clear(); materials.clear(); group.clear(); group.removeFromParent();
    },
  };
}
