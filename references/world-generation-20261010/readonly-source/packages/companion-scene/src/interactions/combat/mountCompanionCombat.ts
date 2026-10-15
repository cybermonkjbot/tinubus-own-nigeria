import * as THREE from "three";
import { combatLimits, combatUsesFirearm, combatWeapon, combatViewDirection, predictCombatPosition, type CombatEvent, type CombatFighter,
  type CombatPoint, type CombatSnapshot, type CombatState, type CompanionCombatStore } from "../../../../../packages/utils/src/companionCombat";
import { createCompanionCombatInput, type CompanionCombatInput } from "../../../../../packages/utils/src/companionCombatInput";
import { routeSegmentClear } from "../../../../../packages/utils/src/companionNavigation";
import { createCombatBrowserInput } from "./combatBrowserInput";
import { companionCombatAgberoSupported } from "../../../../../packages/utils/src/companionCombatAgbero";

export type CombatRigPose = {
  fighter: CombatFighter;
  position: CombatPoint;
  yaw: number;
  pitch: number;
  speed: number;
  firstPerson: boolean;
  blocking: boolean;
  attackProgress: number;
  weaponCycleProgress?: number;
  hitProgress: number;
  reloadProgress: number;
};
export type CombatRigMount = {
  /** Optional for existing player bindings. Required for authored-body leases. */
  ready?(): boolean;
  /** Update the existing animal/NPC rig and its mixer, including hidden first-person head/body parts. */
  update(pose: CombatRigPose, deltaSeconds: number): void;
  /** Release only this borrowed rig's lease, restoring hidden parts. Never dispose a shared source asset. */
  dispose(): void;
};
export type CombatPresentation = {
  phase: "inactive" | "pending" | "ready" | "failed";
  instanceId: string | null;
  matchId: string | null;
  error: string | null;
};
export type CombatMountOptions = {
  store: CompanionCombatStore;
  /** Focus/input boundary containing the canvas and combat HUD. Leave absent for a native input bridge. */
  element?: HTMLElement;
  /** Venue local metre coordinates. This parent must not have nonuniform scale. */
  parent?: THREE.Object3D;
  input?: CompanionCombatInput;
  eyeHeight?: number;
  interpolationMs?: number;
  reducedMotion?: boolean;
  monotonicNow?(): number;
  epochNow?(): number;
  mountFighter(fighter: CombatFighter, context: { scene: THREE.Scene; parent: THREE.Group; local: boolean }): CombatRigMount;
  onEvent?(event: CombatEvent, state: CombatState): void;
  onCameraOwnershipChange?(owned: boolean): void;
  onPointerLockUnavailable?(): void;
  /** The host must revoke world readiness on failure before allowing damage. */
  onPresentationChange?(presentation: CombatPresentation): void;
  onError?(error: string): void;
};
export type CompanionCombatMount = ReturnType<typeof mountCompanionCombat>;
type Sample = { state: CombatState; at: number };
type Rig = { binding: CombatRigMount; position: CombatPoint; attackAt: number; hitAt: number };
type Scope = { matchId: string; instanceId: string; layoutId: string; navigationRevision: string; playerId: string | null };
function findFighter(fighters: readonly CombatFighter[], id: string | null): CombatFighter | undefined {
  for (let index = 0, length = fighters.length; index < length; index++) {
    const fighter = fighters[index];
    if (fighter.id === id) return fighter;
  }
}
const lerpYaw = (a: number, b: number, alpha: number) => a + Math.atan2(Math.sin(b - a), Math.cos(b - a)) * alpha;
const pulse = (at: number, now: number, duration: number) => Math.max(0, Math.min(1, 1 - (now - at) / duration));

/** The existing renderer supplies every frame. This module never starts RAF, a worker or a backend loop. */
export function mountCompanionCombat(scene: THREE.Scene, camera: THREE.Camera, options: CombatMountOptions) {
  const clock = options.monotonicNow ?? (() => performance.now());
  const epoch = options.epochNow ?? (() => Date.now());
  const input = options.input ?? createCompanionCombatInput(options.store, { onError: options.onError });
  const browser = options.element ? createCombatBrowserInput(options.element, input, {
    onPointerLockUnavailable: options.onPointerLockUnavailable,
  }) : null;
  const root = new THREE.Group(); root.name = "Companion combat rig mounts"; root.visible = false;
  (options.parent ?? scene).add(root);
  const rigs = new Map<string, Rig>();
  const eye = new THREE.Vector3(), look = new THREE.Vector3(), direction = new THREE.Vector3(), rotation = new THREE.Quaternion();
  const scratchLocalEye = new THREE.Vector3();
  const savedCameraPosition = new THREE.Vector3(), savedCameraRotation = new THREE.Quaternion();
  let savedCameraFov: number | undefined;
  const eyeHeight = Math.max(.5, Math.min(2.2, options.eyeHeight ?? 1.1));
  const delay = Math.max(0, Math.min(250, options.interpolationMs ?? combatLimits.inputIntervalMs));
  let snapshot: CombatSnapshot = options.store.getSnapshot();
  let scope: Scope | null = null, failedScope: Scope | null = null;
  let eventId = -1, cameraOwned = false, disposed = false;
  const samples: Sample[] = [];
  let predicted: CombatPoint | null = null;
  let presentation: CombatPresentation = { phase: "inactive", instanceId: null, matchId: null, error: null };

  function publishPresentation(phase: CombatPresentation["phase"], error: string | null = null) {
    const instanceId = phase === "inactive" ? null : snapshot.state?.instanceId ?? null;
    const matchId = phase === "inactive" ? null : snapshot.state?.id ?? null;
    if (presentation.phase === phase && presentation.instanceId === instanceId && presentation.matchId === matchId && presentation.error === error) return;
    presentation = { phase, instanceId, matchId, error };
    try { options.onPresentationChange?.(presentation); } catch { /* Display callbacks never own rig cleanup. */ }
  }

  function cameraOwnership(owned: boolean) {
    if (cameraOwned === owned) return;
    cameraOwned = owned;
    if (owned) {
      savedCameraPosition.copy(camera.position); savedCameraRotation.copy(camera.quaternion);
      if (camera instanceof THREE.PerspectiveCamera) {
        savedCameraFov = camera.fov;
        camera.fov = 78; camera.updateProjectionMatrix();
      }
    } else {
      camera.position.copy(savedCameraPosition); camera.quaternion.copy(savedCameraRotation);
      if (camera instanceof THREE.PerspectiveCamera && savedCameraFov !== undefined) {
        camera.fov = savedCameraFov; camera.updateProjectionMatrix(); savedCameraFov = undefined;
      }
      camera.updateMatrixWorld();
    }
    options.onCameraOwnershipChange?.(owned);
  }
  function clear() {
    input.release();
    rigs.forEach(rig => { try { rig.binding.dispose(); } catch { /* Release every remaining lease. */ } }); rigs.clear();
    samples.length = 0; predicted = null; root.visible = false; cameraOwnership(false);
  }
  function fail(reason: unknown) {
    failedScope = scope; clear();
    const message = reason instanceof Error ? reason.message : "Combat characters could not load.";
    publishPresentation("failed", message); options.onError?.(message);
  }
  function update(value: CombatSnapshot) {
    if (disposed) return;
    const previous = samples.at(-1)?.state ?? snapshot.state;
    if (previous && value.ready && value.state?.id === previous.id && value.state.instanceId === previous.instanceId
      && value.playerId === snapshot.playerId && value.state.revision < previous.revision) return;
    snapshot = value; input.sync(value);
    const state = value.state;
    if (!state || !value.ready) { clear(); scope = null; failedScope = null; eventId = -1; publishPresentation("inactive"); return; }
    if (state.phase === "cancelled") { clear(); publishPresentation("inactive"); return; }
    // The host can submit the same encounter every frame. Retain scalar values,
    // not mutable state identity, so in-place scope changes still reset leases.
    const nextScope = scope && scope.matchId === state.id && scope.instanceId === state.instanceId
      && scope.layoutId === state.layoutId && scope.navigationRevision === state.navigation.revision && scope.playerId === value.playerId
      ? scope : { matchId: state.id, instanceId: state.instanceId, layoutId: state.layoutId,
        navigationRevision: state.navigation.revision, playerId: value.playerId };
    const changed = nextScope !== scope;
    if (changed) { clear(); scope = nextScope; failedScope = null; eventId = state.events.at(-1)?.id ?? -1; publishPresentation("pending"); }
    if (failedScope === nextScope) { input.release(); return; }
    const latest = samples.at(-1), at = clock();
    if (latest && state.revision <= latest.state.revision) return;
    if (latest && (at - latest.at > 1000 || state.phase !== latest.state.phase)) samples.length = 0;
    samples.push({ state, at }); if (samples.length > 4) samples.shift();
    const fighter = findFighter(state.fighters, value.playerId);
    // Rebase prediction to server position. Collision-safe smoothing below absorbs small corrections.
    predicted = fighter?.position ?? null;
    const visible = state.fighters.filter(item => item.status !== "left");
    const ids = new Set(visible.map(item => item.id));
    try {
      if (visible.length > combatLimits.players + combatLimits.npcs || ids.size !== visible.length)
        throw new Error("The combat character list is unavailable.");
      const attackers = visible.filter(item => item.kind === "agbero");
      if (state.mode === "melee" && (attackers.length > 1 || attackers.some(item => !companionCombatAgberoSupported(item))))
        throw new Error("The street encounter needs one supported authored character.");
      rigs.forEach((rig, id) => { if (!ids.has(id)) { rig.binding.dispose(); rigs.delete(id); } });
      for (const item of visible) if (!rigs.has(item.id)) {
        publishPresentation("pending");
        rigs.set(item.id, { binding: options.mountFighter(item, { scene, parent: root, local: item.id === value.playerId }),
          position: item.position, attackAt: -Infinity, hitAt: -Infinity });
      }
    } catch (reason: unknown) {
      fail(reason); return;
    }
    if (!changed) for (const event of state.events) {
      if (event.id <= eventId) continue;
      eventId = event.id;
      // Reconnect snapshots must not replay old gunfire or damage animations.
      if (epoch() + value.serverOffset - event.at > 1000) continue;
      if (event.kind === "attack") { const rig = rigs.get(event.actorId); if (rig) rig.attackAt = at; }
      if (event.kind === "hit" && event.targetId) { const rig = rigs.get(event.targetId); if (rig) rig.hitAt = at; }
      options.onEvent?.(event, state);
    }
    root.visible = true;
  }
  function frame(now: number, deltaSeconds: number) {
    if (disposed || !Number.isFinite(now) || !Number.isFinite(deltaSeconds)) return;
    const latest = samples.at(-1), state = latest?.state;
    if (!state || !root.visible || failedScope === scope) { cameraOwnership(false); return; }
    const dt = Math.max(0, Math.min(.1, deltaSeconds));
    const controls = input.getSnapshot(), command = input.getInput();
    const local = findFighter(state.fighters, snapshot.playerId);
    const ownsCamera = !!local && combatUsesFirearm(state, local) && controls.active && !controls.blocked
      && !!local && local.status === "active" && state.phase === "playing";
    cameraOwnership(ownsCamera);
    const fresh = now - latest.at < 600;
    if (local && predicted && controls.active && fresh) predicted = predictCombatPosition(state, { ...local, position: predicted }, command, dt);
    const time = options.reducedMotion ? latest.at : now - delay;
    let before = samples[0], after = latest;
    for (const sample of samples) { if (sample.at <= time) before = sample; if (sample.at >= time) { after = sample; break; } }
    const alpha = before.at === after.at ? 1 : THREE.MathUtils.clamp((time - before.at) / (after.at - before.at), 0, 1);
    try {
      for (const fighter of state.fighters) {
        const rig = rigs.get(fighter.id); if (!rig) continue;
        if (rig.binding.ready && !rig.binding.ready()) throw new Error("Reload the current combat characters.");
        const isLocal = fighter.id === snapshot.playerId;
        // Read current arrays after ready(), which may call back into the host.
        const a = findFighter(before.state.fighters, fighter.id) ?? fighter;
        const b = before.state.fighters === after.state.fighters ? a : findFighter(after.state.fighters, fighter.id) ?? fighter;
        const target = isLocal ? predicted : null;
        const x = target ? target[0] : a.position[0] + (b.position[0] - a.position[0]) * alpha;
        const y = target ? target[1] : a.position[1] + (b.position[1] - a.position[1]) * alpha;
        const z = target ? target[2] : a.position[2] + (b.position[2] - a.position[2]) * alpha;
        const smoothing = isLocal && fresh && !options.reducedMotion ? 1 - Math.exp(-25 * dt) : 1;
        // Only the final position escapes to a binding; keep it independently owned.
        let point: CombatPoint = [rig.position[0] + (x - rig.position[0]) * smoothing,
          rig.position[1] + (y - rig.position[1]) * smoothing, rig.position[2] + (z - rig.position[2]) * smoothing];
        if (!routeSegmentClear(state.navigation, rig.position, point) || fighter.status !== "active") point = fighter.position;
        const speed = fighter.status === "active" && dt > 0 ? Math.min(state.tuning.speed * 1.5, Math.hypot(point[0] - rig.position[0], point[2] - rig.position[2]) / dt) : 0;
        rig.position = point;
        const yaw = isLocal && controls.active ? command.yaw : lerpYaw(a.yaw, b.yaw, alpha);
        const pitch = isLocal && controls.active ? command.pitch : a.pitch + (b.pitch - a.pitch) * alpha;
        const serverNow = epoch() + snapshot.serverOffset;
        const reloadProgress = fighter.reloadAt === null ? 0 : Math.max(0, Math.min(1, 1 - (fighter.reloadAt - serverNow) / (combatWeapon(state, fighter)?.reloadMs ?? state.tuning.reloadMs)));
        rig.binding.update({ fighter, position: point, yaw, pitch, speed, firstPerson: isLocal && ownsCamera,
          blocking: isLocal && controls.active && command.block && !combatUsesFirearm(state, fighter),
          attackProgress: options.reducedMotion ? 0 : pulse(rig.attackAt, now, combatUsesFirearm(state, fighter) ? 180 : 350),
          weaponCycleProgress: options.reducedMotion ? 0 : pulse(rig.attackAt, now, fighter.streetWeaponId === "shotgun" ? 650 : 180),
          hitProgress: options.reducedMotion ? 0 : pulse(rig.hitAt, now, 260), reloadProgress }, dt);
        if (rig.binding.ready && !rig.binding.ready()) throw new Error("Reload the current combat characters.");
        if (isLocal && ownsCamera) {
          eye.set(point[0], point[1] + eyeHeight, point[2]); root.updateWorldMatrix(true, false); root.localToWorld(eye);
          if (camera.parent) { camera.parent.updateWorldMatrix(true, false); camera.position.copy(camera.parent.worldToLocal(scratchLocalEye.copy(eye))); }
          else camera.position.copy(eye);
          direction.fromArray(combatViewDirection(yaw, pitch)); root.getWorldQuaternion(rotation); direction.applyQuaternion(rotation);
          look.copy(eye).add(direction); camera.lookAt(look); camera.updateMatrixWorld();
        }
      }
    } catch (reason: unknown) { fail(reason); return; }
    publishPresentation("ready");
    input.frame(now);
  }
  const unsubscribeInput = input.subscribe(() => { if (!input.getSnapshot().active) cameraOwnership(false); });
  update(snapshot);
  return {
    input,
    root,
    ownsCamera: () => cameraOwned,
    getPresentation: () => presentation,
    update,
    frame,
    start(pointerLock = true) {
      if (presentation.phase !== "ready") return false;
      return browser ? browser.start(pointerLock) : input.start();
    },
    /** Host invokes after repairing its authored-body lease or asset admission. */
    retry() {
      if (disposed || presentation.phase !== "failed") return;
      clear(); scope = null; failedScope = null; update(snapshot);
    },
    setBlocked(value: boolean) { if (browser) browser.setBlocked(value); else input.setBlocked(value); },
    release() { if (browser) browser.release(); else input.release(); },
    dispose() {
      if (disposed) return;
      disposed = true; browser?.dispose(); input.release(); unsubscribeInput();
      if (!options.input) input.dispose(); clear(); root.removeFromParent(); publishPresentation("inactive");
    },
  };
}
