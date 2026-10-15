import * as THREE from "three";
import { companionCombatAgberoBody } from "../../../../../packages/utils/src/companionCombatAgbero";
import type { ScenePoint } from "../../../../../packages/utils/src/companionNavigation";
import { companionTownSquareAgberoSourcePoint } from "../../../../../packages/utils/src/companionTownSquareResidentBodies";
import { townSquareObstacles } from "../../../../../packages/utils/src/companionTownSquare";
import { createCompanionGunfireReactions, type CompanionGunfireState } from "../../../../../packages/utils/src/companionGunfire";
import type { RoomNavigation } from "../../../../../packages/utils/src/companionNavigation";
import { npcEscapeMotion } from "../lagos-places/npcEscapeMotion";
import { createNpcMotionClips } from "../lagos-places/npcWalk";
import { companionCityMedianCrossings, companionPedestrianRouteNavigation, type CompanionPedestrianRouteSample } from "../../../../../packages/utils/src/companionStreetPedestrians";
import { captureObjectTransforms, type ObjectTransformSnapshot } from "../scenes/objectTransformSnapshot";

type NpcId = "mai-suya" | "akara-woman" | "street-tout-cap" | "street-tout-shirt" | "street-tout-vest";
type Motion = "work" | "idle" | "gesture" | "walk";
type Phase = "idle" | "approach" | "gesture" | "return";
type Participant = { x: number; z: number; radius: number };
type NpcPose = Readonly<{ point: ScenePoint; yaw: number; moving: boolean }>;
type Definition = { id: NpcId; kind: "vendor" } | {
  id: NpcId;
  kind: "tout";
  delay: number;
  cooldown: number;
  gestureSeconds: number;
  destination?: readonly [number, number];
  speed: number;
  lookAt?: readonly [number, number];
};
type Playback = {
  action: THREE.AnimationAction;
  weight: number;
  from: number;
  target: number;
  playing: boolean;
};
type Npc = {
  definition: Definition;
  root: THREE.Object3D;
  rootVisible: boolean;
  originals: ObjectTransformSnapshot[];
  home: THREE.Vector3;
  homeOrientation: THREE.Quaternion;
  homeYaw: number;
  position: THREE.Vector3;
  yaw: number;
  travelSeconds: number;
  phase: Phase;
  phaseSeconds: number;
  idleSeconds: number;
  motion: Motion | undefined;
  blendSeconds: number;
  blendPending: boolean;
  actions: Map<Motion, Playback>;
  issues: string[];
  body?: THREE.Object3D;
  bodyOriginal?: ObjectTransformSnapshot;
  bodyVisible?: boolean;
  readonly escapeReserves: RoomNavigation["obstacles"];
  vendorBodies?: readonly { node: THREE.Object3D; visible: boolean }[];
  residentControlled: boolean;
  residentVisible: boolean;
  residentAt?: number;
  residentReducedMotion: boolean;
  residentPose?: NpcPose;
  localPoseSnapshot?: NpcPose;
  floorPoseSnapshot?: NpcPose;
  escape?: { point: ScenePoint; yaw: number; moving: boolean; fleeing: boolean; incidentId?: string };
  escapeBones?: readonly ObjectTransformSnapshot[];
  escapeHome?: NpcPose;
};

const DEFINITIONS: readonly Definition[] = [
  { id: "mai-suya", kind: "vendor" },
  { id: "akara-woman", kind: "vendor" },
  {
    id: "street-tout-cap", kind: "tout", delay: 8, cooldown: 35,
    gestureSeconds: 4, destination: [-8, -19.2], speed: 1.1, lookAt: [-8, -16.2],
  },
  {
    id: "street-tout-shirt", kind: "tout", delay: 16, cooldown: 33,
    gestureSeconds: 2.5, speed: 0,
  },
  {
    id: "street-tout-vest", kind: "tout", delay: 24, cooldown: 37,
    gestureSeconds: 5, destination: [-4.4, 15.25], speed: .8, lookAt: [-3.96, 16.5],
  },
];
const UP = new THREE.Vector3(0, 1, 0);
const MAX_FRAME = .1;
const SUSPEND_GAP = .25;
const BLEND_SECONDS = .3;
const TURN_SPEED = 3.5;
const RADIUS = .35;

/**
 * Owns only local NPC motion and its mixer. Pass the separate, uncentred NPC GLB
 * scene, with its authored Y-up transforms and globally named, rig-local clips.
 * Call update before sampling participants for traffic. Skipping updates pauses.
 * Disabling restores authored poses. Reset or re-enable restarts on the next
 * enabled update; neither starts a render loop.
 * No timers, player controls, scene resources or caller-owned pause state are owned.
 */
export function createTownSquareNpcs(model: THREE.Group, animations: THREE.AnimationClip[], canStartRoadEpisode: () => boolean = () => true): {
  update(dt: number): boolean;
  setEnabled(enabled: boolean): void;
  reset(): void;
  dispose(): void;
  participants(reserveCrossing?: boolean): readonly Participant[];
  summary(): string;
  /** Body-only controls retain stand equipment and authored root transforms. */
  setResidentVendor(id: "mai-suya" | "akara-woman", state: { visible: boolean; sharedSeconds: number; reducedMotion: boolean } | null): boolean;
  vendorBody(id: "mai-suya" | "akara-woman"): THREE.Object3D | null;
  setResidentAgbero(state: { visible: boolean; sharedSeconds: number; reducedMotion: boolean;
    pose: { point: ScenePoint; yaw: number; moving: boolean } | null } | null): boolean;
  agberoBody(): THREE.Object3D | null;
  /** Exclusive suppression only. The combat owner mounts its own body. */
  leaseAgbero(): { current(): boolean; release(): void } | null;
  issues(): readonly string[];
  setGunfire(state: CompanionGunfireState | null, navigation: RoomNavigation | null, now: number): void;
  reacting(id: NpcId): boolean;
  /** Accepted samples use these same bodies; gunfire and combat remain higher priority. */
  setStreetPedestrians(samples: readonly CompanionPedestrianRouteSample[], navigation: RoomNavigation | null, now: number, instanceId: string | null): void;
  streetOwned(id: NpcId): boolean;
} {
  const mixer = new THREE.AnimationMixer(model);
  const npcs: Npc[] = [];
  const npcGroups = new Map<string, readonly Npc[]>();
  const noNpcs: readonly Npc[] = [];
  const missingParents: NpcId[] = [];
  const roots = new Map<NpcId, THREE.Object3D[]>();
  const worldPosition = new THREE.Vector3();
  const localPosition = new THREE.Vector3();
  const orientation = new THREE.Quaternion();
  const parentOrientation = new THREE.Quaternion();
  const restOrientation = new THREE.Quaternion();
  let enabled = true, disposed = false, dirty = false;
  let mixerPoseDirty = true;
  let elapsedSeconds = 0;
  let gunfire: CompanionGunfireState | null = null, gunfireNavigation: RoomNavigation | null = null, gunfireNow = 0;
  let reactions: ReturnType<typeof createCompanionGunfireReactions> | undefined;
  const streetSamples = new Map<string, CompanionPedestrianRouteSample>();
  const streetReturns = new Map<string, CompanionPedestrianRouteSample>();
  const streetEscapeNavigations = new Map<string, RoomNavigation>();
  const streetEscapeMedianIds = new Map<string, string | null>();
  const streetEscapeReactions = new Map<string, ReturnType<typeof createCompanionGunfireReactions>>();
  let streetNow = 0, streetInstanceId: string | null = null, streetNavigation: RoomNavigation | null = null;
  let agberoGeneration = 0;
  let agberoLease: { npc: Npc; rig: THREE.Object3D; parent: THREE.Object3D; rootParent: THREE.Object3D;
    generation: number; visible: boolean; release(): void } | null = null;

  model.updateWorldMatrix(true, true);
  model.traverse(node => {
    if (node.userData.lifeRole !== "npc") return;
    const definition = DEFINITIONS.find(candidate => candidate.id === node.userData.npcId);
    if (!definition) return;
    const matches = roots.get(definition.id) ?? [];
    matches.push(node);
    roots.set(definition.id, matches);
  });

  for (const definition of DEFINITIONS) {
    const matches = roots.get(definition.id) ?? [];
    const group: Npc[] = [];
    npcGroups.set(definition.id, group);
    if (!matches.length) missingParents.push(definition.id);
    const reserveId = definition.kind === "vendor" ? definition.id === "akara-woman" ? "akara-stand" : "mai-suya-stand"
      : definition.id === "street-tout-cap" ? "cap-npc-sweep" : definition.id === "street-tout-vest" ? "vest-npc-sweep" : "shirt-npc";
    const escapeReserves = townSquareObstacles.filter(box => box.id === reserveId);
    for (const root of matches) {
      const originals = captureObjectTransforms(root);
      const home = root.getWorldPosition(new THREE.Vector3());
      const homeOrientation = root.getWorldQuaternion(new THREE.Quaternion());
      const front = new THREE.Vector3(0, 0, 1).applyQuaternion(homeOrientation);
      const homeYaw = Math.atan2(front.x, front.z);
      const travelSeconds = definition.kind === "tout" && definition.destination
        ? Math.hypot(definition.destination[0] - home.x, definition.destination[1] - home.z) / definition.speed : 0;
      const npc: Npc = {
        definition, root, rootVisible: root.visible, originals, home, homeOrientation, homeYaw, position: home.clone(), yaw: homeYaw,
        travelSeconds, phase: "idle", phaseSeconds: 0, idleSeconds: definition.kind === "tout" ? definition.delay : 0,
        motion: undefined, blendSeconds: 0, blendPending: false, actions: new Map(), issues: [], escapeReserves,
        residentControlled: false, residentVisible: false, residentReducedMotion: false,
        localPoseSnapshot: undefined, floorPoseSnapshot: undefined,
      };
      if (matches.length !== 1) npc.issues.push("duplicate NPC parents");
      if (![home.x, home.y, home.z, homeYaw, travelSeconds].every(Number.isFinite)) npc.issues.push("invalid authored transform");
      if (definition.kind === "vendor") {
        const rigName = THREE.PropertyBinding.sanitizeNodeName(`${definition.id} rig`);
        const bodies = root.children.filter(node => node.name === rigName);
        npc.vendorBodies = bodies.map(node => ({ node, visible: node.visible }));
        if (bodies.length !== 1) npc.issues.push("missing or ambiguous vendor body");
        else { npc.body = bodies[0]; npc.bodyVisible = bodies[0].visible; }
      }
      if (definition.id === companionCombatAgberoBody.npcId) {
        const bodies = root.children.filter(node => node.name === THREE.PropertyBinding.sanitizeNodeName(`${definition.id} rig`));
        if (bodies.length !== 1 || root.children.length !== 1) npc.issues.push("missing or ambiguous agbero body");
        else { npc.body = bodies[0]; npc.bodyVisible = bodies[0].visible; }
      }
      if (npc.body) npc.bodyOriginal = originals.find(original => original.node === npc.body);
      // At the authored home this is 10.46 s. Include frame-boundary rounding;
      // never run an unexpectedly longer road block from a different layout.
      if (definition.id === "street-tout-cap" && definition.kind === "tout"
        && travelSeconds * 2 + definition.gestureSeconds + MAX_FRAME * 3 >= 12) {
        npc.issues.push("road route exceeds the 12-second limit");
      }
      const motions: readonly Motion[] = definition.kind === "vendor" ? ["work"]
        : definition.destination || definition.id === companionCombatAgberoBody.npcId ? ["idle", "gesture", "walk"] : ["idle", "gesture"];
      const clips = new Map<Motion, THREE.AnimationClip>();
      const everydayMotions = definition.kind === "tout" && !npc.issues.length ? createNpcMotionClips(root,animations,definition.id) : [];
      for (const motion of motions) {
        const name = `${definition.id}_${motion}`;
        const candidates = animations.filter(clip => clip.name === name);
        const clip = everydayMotions.find(clip => clip.name === name) ?? candidates[0];
        if (!clip) npc.issues.push(`missing ${name}`);
        else if (candidates.length !== 1) npc.issues.push(`duplicate ${name}`);
        else if (!Number.isFinite(clip.duration) || clip.duration <= 0 || !clip.tracks.length) npc.issues.push(`empty or invalid ${name}`);
        else clips.set(motion, clip);
      }
      // An incomplete NPC stays at its authored pose; no procedural substitute.
      if (!npc.issues.length) {
        for (const [motion, clip] of clips) {
          const action = mixer.clipAction(clip, model, THREE.NormalAnimationBlendMode);
          action.setLoop(THREE.LoopRepeat, Infinity).setEffectiveWeight(0);
          npc.actions.set(motion, { action, weight: 0, from: 0, target: 0, playing: false });
        }
        const escape = npcEscapeMotion(model, animations, definition.id);
        if (escape && !npc.actions.has("walk")) {
          const action = mixer.clipAction(escape, model, THREE.NormalAnimationBlendMode);
          action.setLoop(THREE.LoopRepeat, Infinity).setEffectiveWeight(0);
          npc.actions.set("walk", { action, weight: 0, from: 0, target: 0, playing: false });
        }
        npc.escapeBones = npc.originals.filter(original => original.node instanceof THREE.Bone);
      }
      npcs.push(npc);
      group.push(npc);
    }
  }
  roots.clear();

  // Mounted membership is fixed. Retain duplicates so admission still fails closed.
  function matchingNpcs(id: string): readonly Npc[] {
    return npcGroups.get(id) ?? noNpcs;
  }

  // Reactions may retain these points. Replace changed snapshots instead of mutating them.
  function localNpcPose(npc: Npc): NpcPose {
    const saved = npc.localPoseSnapshot, { x, y, z } = npc.position;
    if (saved && Object.is(saved.point[0], x) && Object.is(saved.point[1], y)
      && Object.is(saved.point[2], z) && Object.is(saved.yaw, npc.yaw)) return saved;
    return npc.localPoseSnapshot = { point: [x, y, z], yaw: npc.yaw, moving: false };
  }

  function projectedEscapePose(npc: Npc, pose: NpcPose, floorY: number | undefined): NpcPose {
    if (floorY === undefined) return pose;
    const saved = npc.floorPoseSnapshot, x = pose.point[0], z = pose.point[2];
    if (saved && Object.is(saved.point[0], x) && Object.is(saved.point[1], floorY)
      && Object.is(saved.point[2], z) && Object.is(saved.yaw, pose.yaw) && saved.moving === pose.moving) return saved;
    return npc.floorPoseSnapshot = { point: [x, floorY, z], yaw: pose.yaw, moving: pose.moving };
  }

  function stopPlayback(playback: Playback) {
    if (!playback.playing) return;
    playback.action.stop();
    playback.playing = false;
    mixerPoseDirty = true;
  }

  function samplePlayback(playback: Playback, time: number) {
    const action = playback.action;
    if (playback.playing && action.enabled && action.paused && Object.is(action.time, time)
      && action.weight === 1 && action.getEffectiveWeight() === 1) return;
    mixerPoseDirty = true;
    action.enabled = true; action.paused = true;
    if (!playback.playing) action.play();
    action.time = time;
    action.setEffectiveWeight(1);
    playback.playing = true;
  }

  function selectMotion(npc: Npc, motion: Motion) {
    if (npc.motion === motion) return;
    mixerPoseDirty = true;
    npc.blendPending = true;
    const first = npc.motion === undefined;
    const smallGesture = npc.definition.id === "street-tout-shirt" && motion === "gesture";
    for (const [name, playback] of npc.actions) {
      playback.from = playback.weight;
      playback.target = smallGesture ? name === "idle" ? .7 : name === "gesture" ? .3 : 0 : name === motion ? 1 : 0;
      if (playback.target > 0 && !playback.playing) {
        playback.action.reset().setEffectiveTimeScale(1).setEffectiveWeight(playback.weight).play();
        playback.playing = true;
      }
    }
    npc.motion = motion;
    npc.blendSeconds = first ? BLEND_SECONDS : 0;
  }

  function blend(npc: Npc, dt: number) {
    if (!npc.blendPending) return;
    npc.blendSeconds = Math.min(BLEND_SECONDS, npc.blendSeconds + dt);
    const amount = THREE.MathUtils.smoothstep(npc.blendSeconds, 0, BLEND_SECONDS);
    // Both actions run during a transition. Their weights sum to one, including
    // the shirt NPC's smaller gesture layered with its continuing idle clip.
    for (const playback of npc.actions.values()) {
      if (!playback.playing) continue;
      playback.weight = THREE.MathUtils.lerp(playback.from, playback.target, amount);
      playback.action.setEffectiveWeight(playback.weight);
      if (npc.blendSeconds === BLEND_SECONDS && playback.target === 0) {
        stopPlayback(playback);
      }
    }
    npc.blendPending = npc.blendSeconds < BLEND_SECONDS;
  }

  function enterPhase(npc: Npc, phase: Phase) {
    npc.phase = phase;
    npc.phaseSeconds = 0;
    if (phase === "idle" && npc.definition.kind === "tout") npc.idleSeconds = npc.definition.cooldown;
    selectMotion(npc, phase === "approach" || phase === "return" ? "walk" : phase);
  }

  function applyPose(npc: Npc) {
    const node = npc.root;
    localPosition.copy(npc.position);
    // Add only a world-Y heading delta to the exported orientation. Replacing
    // the quaternion with a bare yaw would discard authored axis corrections.
    orientation.setFromAxisAngle(UP, npc.yaw - npc.homeYaw).multiply(npc.homeOrientation);
    if (node.parent) {
      node.parent.worldToLocal(localPosition);
      node.parent.getWorldQuaternion(parentOrientation).invert();
      orientation.premultiply(parentOrientation);
    }
    node.position.copy(localPosition);
    node.quaternion.copy(orientation);
    node.matrixAutoUpdate = true;
    node.matrixWorldAutoUpdate = true;
    node.updateMatrix();
  }

  function advance(npc: Npc, dt: number) {
    const definition = npc.definition;
    if (definition.kind === "vendor") {
      selectMotion(npc, "work");
      return;
    }
    if (!npc.motion) selectMotion(npc, "idle");
    npc.phaseSeconds += dt;
    let targetYaw = npc.homeYaw;
    const destination = definition.destination;
    if (npc.phase === "idle") {
      if (npc.phaseSeconds >= npc.idleSeconds) {
        if (definition.id === "street-tout-cap" && !canStartRoadEpisode()) {
          npc.phaseSeconds = 0; npc.idleSeconds = 1;
        } else enterPhase(npc, destination ? "approach" : "gesture");
      }
    } else if ((npc.phase === "approach" || npc.phase === "return") && destination) {
      const returning = npc.phase === "return";
      const progress = npc.travelSeconds > 0 ? Math.min(1, npc.phaseSeconds / npc.travelSeconds) : 1;
      const outward = returning ? 1 - progress : progress;
      npc.position.set(
        THREE.MathUtils.lerp(npc.home.x, destination[0], outward), npc.home.y,
        THREE.MathUtils.lerp(npc.home.z, destination[1], outward),
      );
      const direction = returning ? -1 : 1;
      targetYaw = Math.atan2((destination[0] - npc.home.x) * direction, (destination[1] - npc.home.z) * direction);
      if (progress === 1) enterPhase(npc, returning ? "idle" : "gesture");
    } else if (npc.phase === "gesture") {
      if (definition.lookAt) targetYaw = Math.atan2(definition.lookAt[0] - npc.position.x, definition.lookAt[1] - npc.position.z);
      if (npc.phaseSeconds >= definition.gestureSeconds) enterPhase(npc, destination ? "return" : "idle");
    }
    const turn = Math.atan2(Math.sin(targetYaw - npc.yaw), Math.cos(targetYaw - npc.yaw));
    npc.yaw += THREE.MathUtils.clamp(turn, -TURN_SPEED * dt, TURN_SPEED * dt);
    if (destination) applyPose(npc);
  }

  function restoreNpc(npc: Npc, release = false) {
    mixerPoseDirty = true;
    npc.localPoseSnapshot = npc.floorPoseSnapshot = undefined;
    delete npc.root.userData.gunfireIncidentId;
    delete npc.root.userData.gunfireVoicePoint;
    npc.position.copy(npc.home);
    npc.yaw = npc.homeYaw;
    npc.phase = "idle";
    npc.phaseSeconds = 0;
    npc.idleSeconds = npc.definition.kind === "tout" ? npc.definition.delay : 0;
    npc.motion = undefined;
    npc.blendSeconds = 0;
    npc.blendPending = false;
    for (const playback of npc.actions.values()) {
      playback.action.stop();
      playback.weight = playback.from = playback.target = 0;
      playback.playing = false;
      playback.action.paused = false;
      playback.action.setEffectiveWeight(0);
    }
    for (const original of npc.originals) original.restore();
    if (release) npc.actions.clear();
    for (const body of npc.vendorBodies ?? []) body.node.visible = release || !npc.residentControlled
      ? body.visible : npc.residentVisible;
    if (npc.definition.id === companionCombatAgberoBody.npcId) {
      if (!release && npc.residentControlled && npc.residentPose) {
        npc.position.fromArray(npc.residentPose.point); npc.yaw = npc.residentPose.yaw; applyPose(npc);
      }
      npc.root.visible = agberoLease?.npc === npc ? false
        : npc.rootVisible && (release || !npc.residentControlled || npc.residentVisible);
    }
  }

  function currentAgbero(): boolean {
    const claim = agberoLease;
    if (!claim) return false;
    const root = claim.npc.root;
    // Full duplicate admission belongs to acquisition. Repeated render
    // readiness checks use captured identities and structural invalidation.
    if (disposed || claim.generation !== agberoGeneration || model.parent !== claim.parent || root.parent !== claim.rootParent
      || root.userData.lifeRole !== "npc" || root.userData.npcId !== companionCombatAgberoBody.npcId
      || root.children.length !== 1 || root.children[0] !== claim.rig || claim.rig.parent !== root
      || !claim.rig.visible || claim.npc.root.visible) {
      claim.release(); return false;
    }
    return true;
  }

  function restoreState(release = false) {
    if (release) agberoLease?.release();
    mixer.stopAllAction();
    if (release) mixer.uncacheRoot(model);
    mixer.time = 0;
    elapsedSeconds = 0;
    reactions?.clear();
    for (const reaction of streetEscapeReactions.values()) reaction.clear();
    for (const npc of npcs) {
      // Restore after uncaching as well, including bindings never played.
      restoreNpc(npc, release);
      npc.escape = undefined; npc.escapeHome = undefined;
    }
    model.updateWorldMatrix(true, true);
    dirty = npcs.length > 0;
  }
  function clearStreetState() {
    for (const id of streetSamples.keys()) {
      const npc = matchingNpcs(id)[0];
      if (npc && agberoLease?.npc !== npc) { restoreNpc(npc); npc.escape = undefined; npc.escapeHome = undefined; }
    }
    streetSamples.clear(); streetReturns.clear(); streetEscapeNavigations.clear(); streetEscapeMedianIds.clear();
    for (const reaction of streetEscapeReactions.values()) reaction.clear();
    streetEscapeReactions.clear(); streetNow = 0; streetInstanceId = null; streetNavigation = null; dirty = true;
  }

  return {
    update(dt) {
      if (disposed) return false;
      currentAgbero();
      const restored = dirty;
      dirty = false;
      const advanceScenery = enabled && Number.isFinite(dt) && dt > 0 && dt <= SUSPEND_GAP;
      // No accumulator: discard suspension gaps and the excess of a long frame.
      const step = advanceScenery ? Math.min(dt, MAX_FRAME) : 0;
      let active = false;
      for (const npc of npcs) {
        npc.root.userData.civilianVoiceId = npc.definition.id;
        npc.root.userData.civilianVoiceVisible = agberoLease?.npc !== npc && !npc.issues.length
          && (!npc.residentControlled || npc.residentVisible) && (npc.body?.visible ?? npc.root.visible);
        delete npc.root.userData.gunfireIncidentId;
        delete npc.root.userData.gunfireVoicePoint;
        if (agberoLease?.npc === npc) continue;
        if (npc.issues.length) continue;
        const street = streetSamples.get(npc.definition.id);
        const ordinary = street ? { point: street.point, yaw: street.yaw, moving: street.moving } : npc.residentPose ?? npc.escapeHome
          ?? localNpcPose(npc);
        // Gunfire keeps its existing mover. Median-aware navigation only removes this actor's accepted island reserve.
        const escapeOwner = street ? streetEscapeReactions.get(npc.definition.id) ?? reactions : reactions;
        const escapeFloor = (street ? streetEscapeNavigations.get(npc.definition.id) : gunfireNavigation)?.floorY;
        const escapePose = projectedEscapePose(npc, ordinary, escapeFloor);
        const escape = (!npc.residentControlled || npc.residentVisible) && escapeOwner
          ? escapeOwner.sample(npc.definition.id, escapePose, gunfire, gunfireNow, npc.escapeReserves) : undefined;
        const displaced = !!escape && Math.hypot(escape.point[0] - ordinary.point[0], escape.point[2] - ordinary.point[2]) > .01;
        if (escape && (escape.fleeing || displaced)) {
          npc.escapeHome ??= ordinary;
          mixerPoseDirty = true;
          for (const original of npc.escapeBones ?? []) original.restore();
          npc.escape = escape;
          npc.root.userData.gunfireIncidentId = escape.fleeing ? escape.incidentId : undefined;
          npc.root.userData.gunfireVoicePoint = escape.point;
          const motion = escape.moving ? "walk" : "idle";
          const playback = npc.actions.get(motion) ?? npc.actions.get("walk");
          for (const other of npc.actions.values()) if (other !== playback) stopPlayback(other);
          if (playback) {
            samplePlayback(playback, npc.residentReducedMotion || !enabled || !escape.moving ? 0
              : gunfireNow / 1000 * (escape.fleeing ? 2.8 : 1.4) % playback.action.getClip().duration);
          }
          if (npc.definition.kind === "vendor" && npc.body) {
            const original = npc.bodyOriginal!;
            const world = worldPosition.fromArray(escape.point); npc.root.worldToLocal(world);
            original.copyPositionTo(npc.body.position).add(world);
            npc.body.quaternion.setFromAxisAngle(UP, escape.yaw - npc.homeYaw).multiply(original.copyQuaternionTo(restOrientation));
          } else { npc.position.fromArray(escape.point); npc.yaw = escape.yaw; applyPose(npc); }
          npc.motion = undefined; active = true; continue;
        }
        if (npc.escape) {
          mixerPoseDirty = true;
          if (!npc.residentControlled && npc.escapeHome) {
            npc.position.fromArray(npc.escapeHome.point); npc.yaw = npc.escapeHome.yaw;
            if (npc.definition.kind === "tout") applyPose(npc);
          }
          for (const playback of npc.actions.values()) {
            stopPlayback(playback); playback.action.paused = false;
            playback.weight = playback.from = playback.target = 0;
          }
          const body = npc.definition.kind === "vendor" ? npc.bodyOriginal : undefined;
          if (body) body.restore();
          for (const original of npc.escapeBones ?? []) original.restore();
          npc.escape = undefined; npc.escapeHome = undefined; npc.motion = undefined;
        }
        if (street && (!npc.residentControlled || npc.residentVisible)) {
          mixerPoseDirty = true;
          for (const original of npc.escapeBones ?? []) original.restore();
          const motion = street.moving && enabled && !npc.residentReducedMotion ? "walk" : "idle";
          const playback = npc.actions.get(motion) ?? npc.actions.get("walk");
          for (const other of npc.actions.values()) if (other !== playback) stopPlayback(other);
          if (playback) {
            samplePlayback(playback, motion === "walk" ? streetNow / 1000 * (street.kind === "median" ? 3 : street.kind === "horn" ? 2.8 : 1.2)
              % playback.action.getClip().duration : 0);
          }
          if (npc.definition.kind === "vendor" && npc.body) {
            const original = npc.bodyOriginal!;
            const world = worldPosition.fromArray(street.point); npc.root.worldToLocal(world);
            original.copyPositionTo(npc.body.position).add(world);
            npc.body.quaternion.setFromAxisAngle(UP, street.yaw - npc.homeYaw).multiply(original.copyQuaternionTo(restOrientation));
          } else { npc.position.fromArray(street.point); npc.yaw = street.yaw; applyPose(npc); }
          npc.motion = undefined; active = true; continue;
        }
        if (npc.residentControlled) {
          if (!npc.residentVisible) {
            for (const playback of npc.actions.values()) stopPlayback(playback);
            npc.motion = undefined; continue;
          }
          if (npc.residentPose) {
            npc.position.fromArray(npc.residentPose.point); npc.yaw = npc.residentPose.yaw; applyPose(npc);
          }
          const motion = npc.definition.kind === "vendor" ? "work"
            : npc.residentPose?.moving && !npc.residentReducedMotion ? "walk" : "idle";
          selectMotion(npc, motion);
          for (const [name, other] of npc.actions) if (name !== motion) {
            stopPlayback(other); other.weight = other.from = other.target = 0;
          }
          const playback = npc.actions.get(motion);
          if (playback) {
            samplePlayback(playback, npc.residentReducedMotion ? 0 : (npc.residentAt ?? 0) % playback.action.getClip().duration);
            playback.weight = playback.from = playback.target = 1;
          }
          active = true; continue;
        }
        if (!advanceScenery) continue;
        advance(npc, step);
        blend(npc, step);
        active = true;
      }
      if (active) {
        // Zero-time updates still evaluate every active track. Keep the existing
        // frame/timeline cadence, but reuse a pose until an action or restore changes it.
        if (step > 0 || mixerPoseDirty) { mixer.update(step); mixerPoseDirty = false; }
        elapsedSeconds += step;
      }
      return active || restored;
    },
    setEnabled(next) {
      if (disposed || next === enabled) return;
      enabled = next;
      restoreState();
    },
    setGunfire(state, navigation, now) {
      if (disposed) return;
      if (gunfireNavigation?.revision !== navigation?.revision) { reactions?.clear(); reactions = navigation ? createCompanionGunfireReactions(navigation) : undefined; }
      gunfireNavigation = navigation; gunfire = state; gunfireNow = now;
    },
    reacting(id) { return matchingNpcs(id).some(npc => !!npc.escape); },
    streetOwned(id) { return streetSamples.has(id); },
    setStreetPedestrians(samples, navigation, now, instanceId) {
      if (disposed) return;
      if (!navigation || !instanceId) { clearStreetState(); return; }
      const validId = (id: string) => typeof id === "string" && id.length > 0 && id.length <= 240 && id.trim() === id;
      const nextScope = streetInstanceId !== instanceId || streetNavigation?.revision !== navigation.revision;
      if (!validId(instanceId) || !Number.isSafeInteger(now) || now < 0 || !nextScope && now < streetNow || !Array.isArray(samples) || samples.length > 24
        || samples.some(s => !s || !validId(s.routeId) || !validId(s.actorId) || !validId(s.npcId)
          || !Array.isArray(s.point) || s.point.length !== 3 || !s.point.every(Number.isFinite) || !Number.isFinite(s.yaw)
          || !Array.isArray(s.returnPoint) || s.returnPoint.length !== 3 || !s.returnPoint.every(Number.isFinite)
          || Math.abs(s.yaw) > Math.PI || !["horn", "median", "approach", "hawker", "return"].includes(s.kind)
          || s.released !== undefined && (s.released !== true || s.moving
            || s.routeStartedAt === undefined || s.routeRevision === undefined
            || Math.hypot(...s.point.map((p, i) => p - s.returnPoint[i])) > .1)
          || s.routeStartedAt !== undefined && (!Number.isSafeInteger(s.routeStartedAt) || s.routeStartedAt < 0 || s.routeStartedAt > s.at)
          || s.routeRevision !== undefined && (!Number.isSafeInteger(s.routeRevision) || s.routeRevision < 0)
          || !Number.isFinite(s.radius) || s.radius < .2 || s.radius > .8 || typeof s.moving !== "boolean"
          || s.layoutRevision !== navigation.revision || s.instanceId !== instanceId || !Number.isSafeInteger(s.at) || s.at < 0 || s.at > now || now - s.at > 250
          || s.point[0] < navigation.bounds.minX + s.radius || s.point[0] > navigation.bounds.maxX - s.radius
          || s.point[2] < navigation.bounds.minZ + s.radius || s.point[2] > navigation.bounds.maxZ - s.radius
          || Math.abs(s.point[1] - navigation.floorY) > .001 && Math.abs(s.point[1] - navigation.floorY - .134) > .001)
        || new Set(samples.map(s => s.npcId)).size !== samples.length || new Set(samples.map(s => s.actorId)).size !== samples.length
        || !nextScope && samples.some(sample => {
          const previous = streetSamples.get(sample.npcId) ?? streetReturns.get(sample.npcId);
          return previous && (previous.actorId !== sample.actorId || previous.at > sample.at
            || previous.routeStartedAt !== undefined && (sample.routeStartedAt === undefined || sample.routeStartedAt < previous.routeStartedAt
              || sample.routeStartedAt === previous.routeStartedAt && previous.routeRevision !== undefined
                && (sample.routeRevision === undefined || sample.routeRevision < previous.routeRevision)));
        })) return;
      const medianNavigations = new Map<string, RoomNavigation>();
      for (const sample of samples) {
        if (sample.medianId === null) {
          if (Math.abs(sample.point[1] - navigation.floorY) > .001) return;
          continue;
        }
        const median = companionCityMedianCrossings(navigation).find(c => c.median.id === sample.medianId)?.median;
        const nav = median && companionPedestrianRouteNavigation(navigation, median);
        if (!nav) return;
        if (Math.abs(sample.point[1] - navigation.floorY) > .001) {
          const island = median!.island!, x = (island.minX + island.maxX) / 2, z = (island.minZ + island.maxZ) / 2;
          const c = Math.cos(island.yaw ?? 0), s = Math.sin(island.yaw ?? 0);
          if (Math.abs((sample.point[0] - x) * c - (sample.point[2] - z) * s) >= (island.maxX - island.minX) / 2
            || Math.abs((sample.point[0] - x) * s + (sample.point[2] - z) * c) >= (island.maxZ - island.minZ) / 2) return;
        }
        medianNavigations.set(sample.npcId, nav);
      }
      if (nextScope) clearStreetState();
      for (const id of streetSamples.keys()) if (!samples.some(s => s.npcId === id)) {
        const npc = matchingNpcs(id)[0];
        const previous = streetSamples.get(id)!;
        const home = previous.returnPoint;
        const ordinary = npc?.residentPose?.point ?? (npc ? [npc.home.x, npc.home.y, npc.home.z] : null);
        // A missing route cannot return a displaced actor through walls or traffic.
        if (npc && home && Math.hypot(previous.point[0] - home[0], previous.point[2] - home[2]) <= .1
          && Math.abs(previous.point[1] - home[1]) <= .01
          && ordinary && Math.hypot(...home.map((p, i) => p - ordinary[i])) <= .1
          && !npc.escape && agberoLease?.npc !== npc) {
          restoreNpc(npc); streetSamples.delete(id); streetEscapeNavigations.delete(id); streetEscapeMedianIds.delete(id);
          streetEscapeReactions.get(id)?.clear(); streetEscapeReactions.delete(id);
        } else streetSamples.set(id, { ...previous, moving: false, at: now });
      }
      streetNow = now; streetInstanceId = instanceId; streetNavigation = navigation;
      for (const sample of samples) {
        const matches = matchingNpcs(sample.npcId);
        if (matches.length !== 1 || matches[0].issues.length || agberoLease?.npc === matches[0]) continue;
        if (sample.released) {
          const previous = streetSamples.get(sample.npcId), npc = matches[0];
          streetReturns.set(sample.npcId, sample);
          // Release only at the ordinary owner's real restored pose, never a logical stand point.
          const ordinary = npc.residentPose?.point ?? [npc.home.x, npc.home.y, npc.home.z];
          if (previous && previous.actorId === sample.actorId && !npc.escape
            && Math.hypot(...sample.returnPoint.map((p, i) => p - ordinary[i])) <= .1) {
            restoreNpc(npc); streetSamples.delete(sample.npcId);
            streetEscapeNavigations.delete(sample.npcId); streetEscapeMedianIds.delete(sample.npcId);
            streetEscapeReactions.get(sample.npcId)?.clear(); streetEscapeReactions.delete(sample.npcId);
          } else if (previous) streetSamples.set(sample.npcId, { ...sample, released: undefined, moving: false });
          continue;
        }
        streetReturns.delete(sample.npcId);
        const nav = medianNavigations.get(sample.npcId) ?? navigation;
        if (!matches[0].escape && (streetEscapeNavigations.get(sample.npcId)?.revision !== nav.revision
          || streetEscapeMedianIds.get(sample.npcId) !== sample.medianId || !streetEscapeReactions.has(sample.npcId))) {
          streetEscapeReactions.get(sample.npcId)?.clear();
          streetEscapeReactions.set(sample.npcId, createCompanionGunfireReactions(nav));
          streetEscapeNavigations.set(sample.npcId, nav);
          streetEscapeMedianIds.set(sample.npcId, sample.medianId);
        }
        streetSamples.set(sample.npcId, { ...sample, point: [sample.point[0], sample.point[1], sample.point[2]],
          returnPoint: [sample.returnPoint[0], sample.returnPoint[1], sample.returnPoint[2]] });
      }
      dirty = true;
    },
    reset() {
      if (!disposed) { clearStreetState(); restoreState(); }
    },
    dispose() {
      if (disposed) return;
      reactions?.clear();
      clearStreetState();
      restoreState(true);
      for (const npc of npcs) {
        delete npc.root.userData.civilianVoiceId;
        delete npc.root.userData.civilianVoiceVisible;
        delete npc.root.userData.gunfireIncidentId;
        delete npc.root.userData.gunfireVoicePoint;
      }
      npcs.length = 0;
      npcGroups.clear();
      gunfire = null; gunfireNavigation = null; reactions = undefined;
      disposed = true;
      enabled = false;
      dirty = false;
    },
    participants(reserveCrossing = true) {
      if (disposed) return [];
      currentAgbero();
      // Still-visible, disabled or clip-missing NPCs retain their footprints.
      // The combat owner supplies the leased actor's accepted moving position.
      const footprints: Participant[] = [];
      for (const npc of npcs) {
        if (agberoLease?.npc === npc || npc.residentControlled && !npc.residentVisible) continue;
        const street = streetSamples.get(npc.definition.id);
        if (npc.escape) worldPosition.fromArray(npc.escape.point); else if (street) worldPosition.fromArray(street.point);
        else if (npc.definition.kind === "vendor" && npc.body) npc.body.getWorldPosition(worldPosition); else npc.root.getWorldPosition(worldPosition);
        footprints.push({ x: worldPosition.x, z: worldPosition.z, radius: street?.radius ?? RADIUS });
      }
      // Reserve the lane point while crossing, so approaching traffic brakes
      // before the walking actor enters its swept body area.
      const crossing = matchingNpcs("street-tout-cap").find(npc => npc.phase === "approach");
      if (reserveCrossing && crossing?.definition.kind === "tout" && crossing.definition.destination && !streetSamples.has(crossing.definition.id)) {
        footprints.push({ x: crossing.definition.destination[0], z: crossing.definition.destination[1], radius: RADIUS });
      }
      return footprints;
    },
    setResidentVendor(id, state) {
      if (disposed) return false;
      const matches = matchingNpcs(id);
      if (matches.length !== 1 || !matches[0].body || matches[0].issues.length) {
        let changed = false;
        // Fail closed even when duplicate parents prevent one canonical body.
        for (const npc of matches) {
          npc.residentControlled = !!state; npc.residentVisible = false;
          for (const body of npc.vendorBodies ?? []) {
            const visible = !state && body.visible;
            changed = body.node.visible !== visible || changed; body.node.visible = visible;
          }
        }
        dirty = dirty || changed;
        return changed;
      }
      const npc = matches[0], visible = state ? state.visible && Number.isFinite(state.sharedSeconds) && state.sharedSeconds >= 0 : npc.bodyVisible ?? true;
      const changed = npc.residentControlled !== !!state || npc.body!.visible !== visible;
      npc.residentControlled = !!state; npc.residentVisible = visible;
      npc.residentAt = state?.sharedSeconds; npc.residentReducedMotion = state?.reducedMotion ?? false;
      npc.body!.visible = visible;
      if (changed) {
        for (const playback of npc.actions.values()) { stopPlayback(playback); playback.action.paused = false; }
        npc.motion = undefined; dirty = true;
      }
      return changed;
    },
    vendorBody(id) {
      if (disposed) return null;
      const matches = matchingNpcs(id);
      return matches.length === 1 && !matches[0].issues.length ? matches[0].body ?? null : null;
    },
    setResidentAgbero(state) {
      if (disposed) return false;
      const matches = matchingNpcs(companionCombatAgberoBody.npcId);
      let changed = false;
      for (const npc of matches) {
        const pose = state?.pose;
        const valid = !!state && matches.length === 1 && !npc.issues.length && !!npc.body && !!pose
          && pose.point.length === 3 && pose.point.every(Number.isFinite) && Number.isFinite(pose.yaw)
          && Math.abs(pose.yaw) <= Math.PI && Number.isFinite(state.sharedSeconds) && state.sharedSeconds >= 0;
        const visible = state ? valid && state.visible && npc.rootVisible && !!npc.bodyVisible : npc.rootVisible;
        const nextPose = valid && pose ? pose : undefined;
        const poseChanged = npc.residentPose?.yaw !== nextPose?.yaw || npc.residentPose?.moving !== nextPose?.moving
          || npc.residentPose?.point.some((value, index) => value !== nextPose?.point[index]) === true;
        const controlChanged = npc.residentControlled !== !!state || npc.residentVisible !== visible;
        const updated = controlChanged || poseChanged;
        const savedPose = npc.residentPose;
        // Escape origins can retain the previous pose. Reuse only exact values;
        // copy changed points so later caller mutations cannot rewrite that origin.
        const reusePose = nextPose && savedPose && Object.is(savedPose.yaw, nextPose.yaw) && savedPose.moving === nextPose.moving
          && Object.is(savedPose.point[0], nextPose.point[0]) && Object.is(savedPose.point[1], nextPose.point[1])
          && Object.is(savedPose.point[2], nextPose.point[2]);
        npc.residentPose = nextPose ? reusePose ? savedPose : { point: [nextPose.point[0], nextPose.point[1], nextPose.point[2]],
          yaw: nextPose.yaw, moving: nextPose.moving } : undefined;
        npc.residentControlled = !!state; npc.residentVisible = visible;
        npc.residentAt = state?.sharedSeconds; npc.residentReducedMotion = state?.reducedMotion ?? false;
        if (agberoLease?.npc !== npc) {
          if (controlChanged) restoreNpc(npc);
          else if (nextPose && poseChanged) { npc.position.fromArray(nextPose.point); npc.yaw = nextPose.yaw; applyPose(npc); }
        }
        npc.root.visible = agberoLease?.npc === npc ? false : visible;
        changed = updated || changed;
      }
      dirty = dirty || changed;
      return changed;
    },
    agberoBody() {
      if (disposed) return null;
      const matches = matchingNpcs(companionCombatAgberoBody.npcId);
      return matches.length === 1 && !matches[0].issues.length ? matches[0].body ?? null : null;
    },
    leaseAgbero() {
      if (disposed || !model.parent || currentAgbero()) return null;
      const matches = matchingNpcs(companionCombatAgberoBody.npcId);
      if (matches.length !== 1 || matches[0].issues.length || !matches[0].body) return null;
      const npc = matches[0], body = npc.body;
      if (!body) return null;
      const rootsNow: THREE.Object3D[] = [];
      model.traverse(node => {
        if (node.userData.lifeRole === "npc" && node.userData.npcId === companionCombatAgberoBody.npcId) rootsNow.push(node);
      });
      const bodies = npc.root.children.filter(node => node.name === THREE.PropertyBinding.sanitizeNodeName(`${companionCombatAgberoBody.npcId} rig`));
      if (rootsNow.length !== 1 || rootsNow[0] !== npc.root || bodies.length !== 1 || bodies[0] !== body || !body.visible) return null;
      for (let node: THREE.Object3D | null = body; node; node = node.parent) {
        if (!node.visible) return null;
        if (node === model) break;
      }
      if (Math.hypot(...npc.home.toArray().map((value, index) => value - companionTownSquareAgberoSourcePoint[index])) > .01
        || Math.abs(npc.homeYaw - companionCombatAgberoBody.sourceYaw) > .01) return null;
      // This tout's root contains only its rig. Hide that root, so combat can
      // show its independent cloned root without changing hidden descendants.
      if (npc.root.children.length !== 1) return null;
      const rootParent = npc.root.parent;
      if (!rootParent) return null;
      const structuralNodes: THREE.Object3D[] = [];
      for (let node: THREE.Object3D | null = npc.root; node; node = node.parent) {
        structuralNodes.push(node);
        if (node === model) break;
      }
      const invalidate = () => claim.release();
      const claim = { npc, rig: body, parent: model.parent, rootParent, generation: ++agberoGeneration,
        visible: npc.root.visible, release() {
          if (agberoLease !== claim) return;
          for (const node of structuralNodes) {
            node.removeEventListener("childadded", invalidate);
            node.removeEventListener("childremoved", invalidate);
            node.removeEventListener("removed", invalidate);
          }
          agberoLease = null;
          agberoGeneration++;
          restoreNpc(npc);
          npc.root.visible = claim.visible && (!npc.residentControlled || npc.residentVisible);
          dirty = true;
        } };
      agberoLease = claim;
      for (const node of structuralNodes) {
        node.addEventListener("childadded", invalidate);
        node.addEventListener("childremoved", invalidate);
        node.addEventListener("removed", invalidate);
      }
      restoreNpc(npc);
      dirty = true;
      return { current: () => agberoLease === claim && currentAgbero(), release: claim.release };
    },
    issues: () => [...missingParents.map(id => `Missing ${id} NPC root.`), ...npcs.flatMap(npc => npc.issues.map(issue => `${npc.definition.id}: ${issue}`))],
    summary() {
      if (disposed) return "NPCs disposed; authored poses restored.";
      const lines = [`NPCs ${enabled ? "on" : "off"}; ${elapsedSeconds.toFixed(1)} s; ${npcs.length}/5 parents`];
      for (const id of missingParents) lines.push(`${id}: held; missing NPC parent`);
      for (const npc of npcs) {
        npc.root.getWorldPosition(worldPosition);
        const state = agberoLease?.npc === npc ? "combat body leased"
          : npc.issues.length ? `held; ${npc.issues.join(", ")}`
          : !enabled || !npc.motion ? "authored pose"
          : npc.motion === "work" ? "work loop" : npc.phase;
        lines.push(`${npc.definition.id}: ${state}; x/z ${worldPosition.x.toFixed(2)}, ${worldPosition.z.toFixed(2)} m`);
      }
      return lines.join("\n");
    },
  };
}
