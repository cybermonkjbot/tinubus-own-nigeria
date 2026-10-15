import * as THREE from "three";
import { companionNpcLimits, companionNpcObservationIsDecorative, createCompanionNpcDecorativePlan, createCompanionNpcRuntime, immutableCompanionNpcSnapshot, sampleCompanionNpcDecoration, sampleCompanionNpcs, type CompanionNpcObservation, type CompanionNpcRuntime, type CompanionNpcSample } from "../../../../../packages/utils/src/companionNpcs";
import type { CompanionAssetDelivery, CompanionAssetKey } from "../../../../../packages/utils/src/companionAssets";
import type { CompanionPlaceLayout, CompanionPlaceService } from "../../../../../packages/utils/src/companionPlaces";
import { inspectCompanionPlaceLayout } from "../../../../../packages/utils/src/companionPlacesLayouts";
import { companionPlaceExpansionDestinations, companionExpansionServiceSupported,
  COMPANION_CITY_PLACE_EXIT } from "../../../../../packages/utils/src/companionPlacesExpansion";
import { planRoomRoute, type ScenePoint } from "../../../../../packages/utils/src/companionNavigation";
import { companionWaterfrontPlace, COMPANION_WATERFRONT_EXIT } from "../../../../../packages/utils/src/companionWaterfront";
import { sampleCompanionResidentInPlace, type CompanionResidentPlaceBinding, type CompanionResidentView,
  type CompanionResidentWorldView } from "../../../../../packages/utils/src/companionResidentSimulation";
import { companionResidentKeys } from "../../../../../packages/utils/src/companionResidents";
import { companionMissionVenueStaff } from "../../../../../packages/utils/src/companionMissionContacts";
import { sampleCompanionWorldClock, type CompanionWorldClock } from "../../../../../packages/utils/src/companionWorldClock";
import type { CompanionWorldUtilities, CompanionWorldUtilityScope } from "../../../../../packages/utils/src/companionWorldUtilities";
import type { CompanionEnvironment, EnvironmentAsset, EnvironmentParticipant } from "../scenes/environments";
import { createScenePowerAdapter, projectScenePowerFactor } from "../scenes/scenePower";
import { createLagosAssetPart, disposeLagosAssetPart } from "./assetParts";
import { createLagosPlaceGeometry, disposeLagosPlaceGeometry, updateNightclubLighting } from "./geometry";
import { createNightclubCrowd } from "./nightclubCrowd";

type Actor = { group: THREE.Group; mixer: THREE.AnimationMixer; clips: Map<string, THREE.AnimationClip>; action?: THREE.AnimationAction;
  poseTime?: number; sample?: CompanionNpcSample; fixedWork: boolean };
type ResidentActor = { actor: Actor; key: CompanionAssetKey; npcId: string; sourceYaw: number | undefined };
const utilityKeyPattern = /^[A-Za-z0-9][A-Za-z0-9._:-]*$/;
const validUtilityKey = (value: unknown): value is string => typeof value === "string"
  && value.length > 0 && value.length <= 160 && utilityKeyPattern.test(value);
export type LagosPlaceFocus = { id: string; kind: "service"; label: string; point: ScenePoint; reachable: boolean };
export type LagosPlaceScene = {
  environment: CompanionEnvironment; group: THREE.Group;
  /** Caller supplies shared observations and owns render scheduling. No timers. */
  update(dt: number, sharedNow: number, observation: CompanionNpcObservation | null, visible: boolean, viewer?: ScenePoint, reducedMotion?: boolean,
    participants?: readonly EnvironmentParticipant[]): boolean;
  /** Reactive late join/reconnect requires the complete owning snapshot. */
  resumeNpcs(snapshot: CompanionNpcRuntime): void;
  /** Explicit owner transition after reactive state ends. Does not clear consequences. */
  resumeDecoration(sharedNow: number): void;
  /** Account-owned world snapshot. Residents never become human participants. */
  setResidents(world: CompanionResidentWorldView | null): void;
  /** Already-admitted utility projection for this displayed instance. Null
   * suspends projection without restoring mains or clearing owner freshness. */
  setUtilities(utilities: CompanionWorldUtilities | null, displayedInstanceId: string): void;
  /** Pick only an owned, visible resident body, never a userData claim. */
  resident(object: THREE.Object3D): string | null;
  focus(object: THREE.Object3D, player: ScenePoint): LagosPlaceFocus | null;
  /** Frozen registry entries; object references remain owned by this scene. */
  targets(): readonly { service: CompanionPlaceService; object: THREE.Object3D }[];
  /** Detached, deeply frozen observation of local prediction, not authority. */
  npcSnapshot(): CompanionNpcRuntime;
  issues(): readonly string[];
  dispose(): void;
};

/** Assets are already acquired through Baddie's existing on-demand cache.
 * Optional cosmetic scenery uses the caller's existing delivery provider.
 * No database calls, call ownership, account stores or render loop.
 * Use this disposer, not disposeEnvironment, for cache-owned asset meshes. */
export function createLagosPlaceScene(layout: CompanionPlaceLayout, assets: ReadonlyMap<CompanionAssetKey, EnvironmentAsset>,
  instance: { id: string; sharedStartedAt: number; npcSnapshot?: CompanionNpcRuntime },
  delivery?: CompanionAssetDelivery, changed: () => void = () => {}): LagosPlaceScene {
  const layoutIssues = inspectCompanionPlaceLayout(layout);
  if (layoutIssues.length) throw new Error(`Place layout cannot be admitted: ${layoutIssues.join("; ")}`);
  // Validate clocks, paths and supplied snapshots before allocating scene resources.
  const decorativePlan = createCompanionNpcDecorativePlan(layout, instance.id, instance.sharedStartedAt);
  let runtime = instance.npcSnapshot ? createCompanionNpcRuntime(layout, instance.id, instance.sharedStartedAt, instance.npcSnapshot)
    : sampleCompanionNpcDecoration(decorativePlan, instance.sharedStartedAt);
  const group = new THREE.Group(); group.name = instance.id;
  const geometry = createLagosPlaceGeometry(layout, assets.get("bestieDanceAgainInterior"), delivery, changed); group.add(geometry);
  const actors = new Map<string, Actor>(), issueSet = new Set<string>(), targets: { service: CompanionPlaceService; object: THREE.Object3D }[] = [];
  const focusBindings = new Map<THREE.Object3D, CompanionPlaceService | null>();
  const targetServices = new Map<string, CompanionPlaceService>();
  const copyPoint = (point: ScenePoint): ScenePoint => Object.freeze([point[0], point[1], point[2]] as const);
  decorativePlan.npcs.forEach(npc => { if (npc.issue) issueSet.add(npc.issue); });
  const assetGroups: THREE.Group[] = [];
  let nightclubCrowd: ReturnType<typeof createNightclubCrowd> | undefined;
  let reactive = !!instance.npcSnapshot;
  let disposed = false;
  const power = createScenePowerAdapter();
  let utilities: CompanionWorldUtilities | null = null, utilityScope: CompanionWorldUtilityScope | null = null;
  let utilitySampledAt = -1, utilityIssue: string | null = null;
  let lastObservationIssue: string | null = null;
  let residentWorld: CompanionResidentWorldView | null = null;
  let residentWorldKey: string | null = null;
  let residentClock: CompanionWorldClock | null = null;
  let residentBinding: CompanionResidentPlaceBinding | null = null;
  let residentDirty = false;
  let residentSnapshotIssue: string | null = null;
  const residentRevisions = new Map<string, { revision: number; gameMinute: number }>();
  const controlledNpcs = new Set<string>(), workingNpcs = new Set<string>();
  const residentActors = new Map<string, ResidentActor>();
  const residentBodies = new Map<THREE.Object3D, string>();
  const residentArtIssues = new Map<string, string>();
  const residentPresentationIssues = new Map<string, string>();
  const isDutyMedic = (npcId: string) => layout.kind === "hospital"
    && layout.services.some(service => service.npcId === npcId && service.kind === "medic");
  const requiresResident = companionPlaceExpansionDestinations.some(place => place.id === layout.placeId);
  const venueStaff = new Set(layout.npcs.filter(npc => companionMissionVenueStaff(layout, npc.id)).map(npc => npc.id));
  const serviceAvailable = (service: CompanionPlaceService) => companionExpansionServiceSupported(layout.placeId, service)
    && (!service.npcId || isDutyMedic(service.npcId)
      || !residentSnapshotIssue && (requiresResident ? controlledNpcs.has(service.npcId)
        ? !!residentBinding && workingNpcs.has(service.npcId) : venueStaff.has(service.npcId)
        : !controlledNpcs.has(service.npcId) || workingNpcs.has(service.npcId)));
  function releaseResidentActors() {
    residentActors.forEach(({ actor }) => {
      actor.mixer.stopAllAction(); actor.mixer.uncacheRoot(actor.mixer.getRoot()); disposeLagosAssetPart(actor.group);
    });
    residentActors.clear(); residentArtIssues.clear();
    residentPresentationIssues.clear();
  }
  try {
    for (const definition of layout.assets) {
      const asset = assets.get(definition.key);
      if (!asset) { issueSet.add(`Missing ${definition.key} for ${definition.id}. Keep its service unavailable until loading succeeds.`); continue; }
      try {
        const part = createLagosAssetPart(definition, asset); group.add(part.group); assetGroups.push(part.group);
        const npc = layout.npcs.find(npc => npc.assetPartId === definition.id);
        if (part.actor && npc) {
          const actor: Actor = { group: part.group, mixer: new THREE.AnimationMixer(part.actor), clips: new Map(), fixedWork: definition.npcId === "akara-woman" || definition.npcId === "mai-suya" };
          for (const clip of part.clips) actor.clips.set(clip.name.slice((definition.npcId?.length ?? 0) + 1), clip);
          actors.set(npc.id, actor);
        }
      } catch (error) { issueSet.add(error instanceof Error ? error.message : `Unable to assemble ${definition.id}.`); }
    }
    for (const service of layout.services) {
      const npc = service.npcId ? actors.get(service.npcId) : undefined;
      // Saved-room doors and meal seats have real scene targets. NPC services
      // are not turned into invisible click regions when their rig is absent.
      if (service.npcId && !npc && layout.npcs.find(person => person.id === service.npcId)?.role !== "resident") continue;
      const target = new THREE.Group(); target.name = `${service.id}:target`; target.userData.lagosServiceId = service.id;
      target.position.fromArray(service.anchor.point);
      if (service.kind === "home-door") {
        const material = new THREE.MeshStandardMaterial({ color: "#a8876a", roughness: .8 });
        const door = new THREE.Mesh(new THREE.BoxGeometry(1.1, 2.1, .06), material); door.position.set(0, 1.05, -.65);
        door.userData.lagosServiceId = service.id; target.add(door);
      }
      // A small plate is an object to act on, not an unexplained floating HUD.
      if (service.kind === "food" && service.offers?.includes("eat-carried-food")) {
        const plate = new THREE.Mesh(new THREE.CylinderGeometry(.17, .16, .025, 24), new THREE.MeshStandardMaterial({ color: "#e8e2d7", roughness: .4 }));
        const table = layout.fixtures.find(fixture => fixture.kind === "table" && Math.hypot(fixture.point[0] - service.anchor.point[0], fixture.point[2] - service.anchor.point[2]) < 1.6);
        const island = layout.assets.find(asset => asset.id.startsWith("shared-table-") && Math.abs(asset.point[2] - service.anchor.point[2]) < .5);
        if (table) plate.position.set(table.point[0] - service.anchor.point[0], table.size[1] + .014, table.point[2] - service.anchor.point[2]);
        else if (island) plate.position.set(island.point[0] - service.anchor.point[0], .965, island.point[2] - service.anchor.point[2]);
        else { plate.geometry.dispose(); (plate.material as THREE.Material).dispose(); issueSet.add(`Missing meal support for ${service.id}.`); continue; }
        plate.userData.lagosServiceId = service.id; target.add(plate);
      }
      const definition: CompanionPlaceService = Object.freeze({ ...service,
        approach: Object.freeze({ ...service.approach, point: copyPoint(service.approach.point) }),
        anchor: Object.freeze({ ...service.anchor, point: copyPoint(service.anchor.point) }),
        footprint: Object.freeze({ ...service.footprint }), queue: Object.freeze(service.queue.map(copyPoint)),
        offers: service.offers ? Object.freeze([...service.offers]) : undefined });
      group.add(target); targets.push(Object.freeze({ service: definition, object: npc?.group ?? target }));
      targetServices.set(service.id, definition); focusBindings.set(target, definition);
      // One body cannot identify two services. Explicit fixture targets still
      // work; do not silently replace an earlier NPC binding with the last one.
      if (npc) focusBindings.set(npc.group, focusBindings.has(npc.group) ? null : definition);
    }
    // Capture authored fixture bindings once. Later tags on foreign objects
    // cannot create a service target, even if their ID matches this layout.
    geometry.traverse(node => {
      const service = targetServices.get(node.userData.lagosServiceId);
      if (service) focusBindings.set(node, service);
    });
    if (layout.kind === "nightclub") {
      nightclubCrowd = createNightclubCrowd(layout, assets); group.add(nightclubCrowd.group);
    }
  } catch (error) {
    nightclubCrowd?.dispose();
    power.dispose();
    actors.forEach(actor => { actor.mixer.stopAllAction(); actor.mixer.uncacheRoot(actor.mixer.getRoot()); });
    assetGroups.forEach(disposeLagosAssetPart);
    disposeLagosPlaceGeometry(group); group.clear(); throw error;
  }
  const trayMaterial = new THREE.MeshStandardMaterial({ color: "#dfd6c7", roughness: .45 });
  const tray = new THREE.Mesh(new THREE.CylinderGeometry(.2, .19, .035, 24), trayMaterial); tray.visible = false; group.add(tray);
  const waterfrontPlace = companionWaterfrontPlace(layout.placeId);
  const streetExit = requiresResident || waterfrontPlace?.cityEntrance ? layout.entrances.find(entrance => entrance.role === "world")?.point : undefined;
  const exitMarker = streetExit ? new THREE.Mesh(new THREE.TorusGeometry(.65, .045, 6, 32),
    new THREE.MeshStandardMaterial({ color: "#d4dbcf", emissive: "#596a51", emissiveIntensity: .2 })) : null;
  if (exitMarker && streetExit) {
    exitMarker.name = "Street exit"; exitMarker.rotation.x = -Math.PI / 2;
    exitMarker.position.set(streetExit[0], layout.navigation.floorY + .035, streetExit[2]); group.add(exitMarker);
  }
  let targetRegistry = Object.freeze([...targets]);
  function suspendActors() {
    if (nightclubCrowd) nightclubCrowd.group.visible = false;
    residentBodies.clear();
    tray.visible = false;
    actors.forEach(actor => { actor.mixer.stopAllAction(); actor.action = undefined; actor.sample = undefined; });
    workingNpcs.clear();
    controlledNpcs.forEach(id => { const actor = actors.get(id); if (actor) actor.group.visible = false; });
    residentActors.forEach(({ actor }) => { actor.group.visible = false; actor.mixer.stopAllAction(); actor.action = undefined; });
  }
  function playMotion(actor: Actor, motion: string | undefined, sharedNow: number, reducedMotion = false) {
    const clip = motion ? actor.clips.get(motion) : undefined;
    if (!clip) { actor.mixer.stopAllAction(); actor.action = undefined; return; }
    const action = actor.action?.getClip() === clip ? actor.action : actor.mixer.clipAction(clip);
    const motionChanged = actor.action !== action;
    if (motionChanged) {
      actor.mixer.stopAllAction();
      action.reset().setLoop(THREE.LoopRepeat, Infinity).play(); actor.action = action;
    }
    // Absolute clip phase avoids different poses after joining, hiding or culling.
    const poseTime = reducedMotion ? 0 : ((sharedNow - instance.sharedStartedAt) / 1000 + (actor.fixedWork ? 0 : actor.group.userData.npcPoseOffset ?? 0)) % clip.duration;
    // This mixer alone owns the pose. Stops clear action, so re-entry always
    // evaluates even if the requested phase matches the last visible frame.
    if (motionChanged || actor.poseTime !== poseTime) {
      action.paused = false;
      actor.mixer.setTime(poseTime); actor.poseTime = poseTime;
    }
    action.paused = reducedMotion;
  }
  function animate(actor: Actor, sample: CompanionNpcSample, sharedNow: number, reducedMotion: boolean) {
    const motion = actor.fixedWork ? sample.behaviour === "flee" ? undefined : "work" : sample.moving ? "walk" : sample.cue || sample.behaviour === "attend" ? "gesture" : "idle";
    if (!actor.fixedWork) { actor.group.position.fromArray(sample.pose.point); actor.group.rotation.y = sample.pose.yaw; }
    playMotion(actor, motion, sharedNow, reducedMotion);
  }
  function residentActor(resident: CompanionResidentView, point: ScenePoint, yaw: number): Actor | null {
    const mapping = resident.serviceActor;
    if (mapping?.placeId === layout.placeId && !isDutyMedic(mapping.npcId)) return actors.get(mapping.npcId) ?? null;
    const definition = resident.renderAsset;
    if (!definition || residentArtIssues.has(resident.identity.id)) return null;
    // Preserve JSON's previous equality for absent/non-finite numeric headings.
    const sourceYaw = Number.isFinite(definition.sourceYaw) ? definition.sourceYaw : undefined;
    const existing = residentActors.get(resident.identity.id);
    if (existing && existing.key === definition.key && existing.npcId === definition.npcId && existing.sourceYaw === sourceYaw) return existing.actor;
    if (existing) {
      existing.actor.mixer.stopAllAction(); existing.actor.mixer.uncacheRoot(existing.actor.mixer.getRoot());
      disposeLagosAssetPart(existing.actor.group); residentActors.delete(resident.identity.id);
    }
    const asset = assets.get(definition.key);
    if (!asset) { residentArtIssues.set(resident.identity.id, `Resident art is not loaded for ${resident.identity.name}.`); return null; }
    try {
      const part = createLagosAssetPart({ ...definition, id: resident.identity.id, point, yaw }, asset);
      if (!part.actor) { disposeLagosAssetPart(part.group); return null; }
      const actor: Actor = { group: part.group, mixer: new THREE.AnimationMixer(part.actor),
        clips: new Map(part.clips.map(clip => [clip.name.slice(definition.npcId.length + 1), clip])),
        fixedWork: definition.npcId === "mai-suya" || definition.npcId === "akara-woman" };
      group.add(part.group); residentActors.set(resident.identity.id, { actor, key: definition.key, npcId: definition.npcId, sourceYaw }); return actor;
    } catch (error) {
      residentArtIssues.set(resident.identity.id, error instanceof Error ? error.message : "Resident art is unavailable."); return null;
    }
  }
  function updateResidents(sharedNow: number, viewer: ScenePoint | undefined, reducedMotion: boolean) {
    let changed = residentDirty;
    if (!changed) for (const id of controlledNpcs) {
      if (actors.get(id)?.group.visible) { changed = true; break; }
    }
    if (!changed) for (const { actor } of residentActors.values()) {
      if (actor.group.visible) { changed = true; break; }
    }
    residentDirty = false;
    residentBodies.clear();
    residentPresentationIssues.clear();
    workingNpcs.clear();
    controlledNpcs.forEach(id => { const actor = actors.get(id); if (actor) actor.group.visible = false; });
    residentActors.forEach(({ actor }) => { actor.group.visible = false; });
    if (!residentWorld || !residentBinding) return changed;
    let gameMinute: number;
    try { gameMinute = sampleCompanionWorldClock(residentWorld.clock, sharedNow).gameMinute; }
    catch { residentSnapshotIssue = "Use a current resident world clock."; return changed; }
    residentSnapshotIssue = null;
    let moving = false;
    for (const resident of residentWorld.residents) {
      const art = resident.renderAsset;
      if (resident.identity.worldKey !== residentWorld.worldKey || resident.clockEpochKey !== residentWorld.clock.epochKey
        || resident.updatedAtGameMinute > gameMinute) {
        residentPresentationIssues.set(resident.identity.id, `Resident state is ahead of the shared clock for ${resident.identity.id}.`); continue;
      }
      const revision = residentRevisions.get(resident.identity.id);
      if (revision) { revision.revision = resident.revision; revision.gameMinute = resident.updatedAtGameMinute; }
      else residentRevisions.set(resident.identity.id, { revision: resident.revision, gameMinute: resident.updatedAtGameMinute });
      const sample = sampleCompanionResidentInPlace(resident, residentBinding, sharedNow);
      if (!sample) continue;
      if (!art || !resident.identity.appearance
        || resident.identity.appearance.assetNpcId !== null && resident.identity.appearance.assetNpcId !== art.npcId) {
        residentPresentationIssues.set(resident.identity.id, `Resident state or art is unavailable for ${resident.identity.id}.`); continue;
      }
      const mapping = resident.serviceActor?.placeId === layout.placeId ? resident.serviceActor : null;
      // Duty staff remain separate. Old configurations cannot duplicate them.
      if (mapping && isDutyMedic(mapping.npcId)) continue;
      const npc = mapping ? layout.npcs.find(value => value.id === mapping.npcId) : undefined;
      if (mapping) {
        const part = npc ? layout.assets.find(value => value.id === npc.assetPartId) : undefined;
        if (!npc || npc.role !== resident.identity.role || !part || part.key !== art.key || part.npcId !== art.npcId
          || (part.sourceYaw ?? 0) !== (art.sourceYaw ?? 0)) {
          residentPresentationIssues.set(resident.identity.id, `Resident art does not match the bound venue actor ${mapping.npcId}.`); continue;
        }
      } else if (!layout.assets.some(value => value.key === art.key)) {
        residentPresentationIssues.set(resident.identity.id, `Resident art is not loaded in this venue for ${resident.identity.id}.`); continue;
      }
      const atWork = sample.working && !!npc
        && Math.hypot(sample.pose.point[0] - npc.home.point[0], sample.pose.point[1] - npc.home.point[1],
          sample.pose.point[2] - npc.home.point[2]) <= .01;
      // The published cooks have work clips only. Never slide a seated rig
      // through the street or substitute a different person's walking body.
      const fixedWork = art.npcId === "akara-woman" || art.npcId === "mai-suya";
      if (fixedWork && !atWork) continue;
      const actor = residentActor(resident, sample.pose.point, sample.pose.yaw);
      if (!actor) continue;
      if (atWork && mapping && resident.localMove === null && resident.journey === null
        && resident.activity.kind === "at-place" && resident.activity.interactionId === null) workingNpcs.add(mapping.npcId);
      const nearby = !viewer || Math.hypot(sample.pose.point[0] - viewer[0], sample.pose.point[2] - viewer[2]) <= 18;
      actor.group.visible = nearby;
      if (!nearby) { actor.mixer.stopAllAction(); actor.action = undefined; continue; }
      if (!actor.fixedWork) { actor.group.position.fromArray(sample.pose.point); actor.group.rotation.y = sample.pose.yaw; }
      actor.group.userData.companionResidentId = resident.identity.id;
      residentBodies.set(actor.group, resident.identity.id);
      playMotion(actor, actor.fixedWork ? "work" : sample.moving ? "walk" : "idle", sharedNow, reducedMotion);
      moving ||= sample.moving || !!actor.action;
    }
    residentActors.forEach(({ actor }) => {
      if (!actor.group.visible) { actor.mixer.stopAllAction(); actor.action = undefined; }
    });
    return moving || changed;
  }
  const environment: CompanionEnvironment = {
    id: `lagos:${layout.kind}`, instanceId: instance.id, name: layout.label, background: layout.theme.background,
    characterScale: .38, fov: layout.theme.openAir ? 72 : 76, cameraPolicy: "third-person",
    wide: layout.cameras.wide, close: layout.cameras.close, spawn: layout.spawns[0].point, spawnYaw: layout.spawns[0].yaw,
    walkingPoints: layout.spawns.map(spawn => spawn.point), navigation: layout.navigation,
    cameraObstacles: layout.navigation.obstacles.filter(box => !layout.waterfront || "height" in box).map((box, i) => ({ id: `venue-${i}`, x: (box.minX + box.maxX) / 2, z: (box.minZ + box.maxZ) / 2,
      width: box.maxX - box.minX, depth: box.maxZ - box.minZ, height: "height" in box ? Number(box.height) : 2.5 })),
    create: () => { if (disposed) throw new Error("Place scene was disposed."); return group; },
  };
  return {
    environment, group,
    update(dt, sharedNow, observation, visible, viewer, reducedMotion = false, participants = []) {
      if (disposed) return false;
      if (!Number.isSafeInteger(sharedNow) || sharedNow < runtime.simulatedAt) {
        lastObservationIssue = "Use a current shared clock for this place."; suspendActors(); return false;
      }
      if (utilities && (residentWorldKey !== null && utilities.scope.worldKey !== residentWorldKey
        || residentClock !== null && utilities.scope.clockEpochKey !== residentClock.epochKey)) {
        utilities = null; utilityIssue = "Refresh this place's admitted utility projection.";
      }
      // The admitted utility sample advances between resident snapshots. Use its
      // own clock so a lighting transition does not wait for the next NPC update.
      const powerChanged = utilities
        ? power.update(utilities.power, utilities.sampledAtRealMs, reducedMotion, visible) : false;
      // The lighting owner remains the sole writer of animated mains channels.
      const clockTime = residentClock ? sampleCompanionWorldClock(residentClock, sharedNow) : null;
      const hour = clockTime ? (clockTime.minuteOfDay + clockTime.minuteProgress) / 60 : 10;
      const daylight = Math.max(0, Math.min(1, Math.sin((hour - 6) / 12 * Math.PI)));
      const lightingChanged = updateNightclubLighting(geometry, (sharedNow - instance.sharedStartedAt) / 1000,
        reducedMotion, visible, power.factor, daylight) || powerChanged;
      if (!visible || !Number.isFinite(dt) || dt <= 0 || dt > .25) { suspendActors(); return lightingChanged; }
      if (!observation) { suspendActors(); lastObservationIssue = "Provide the authorized place observation before updating NPCs."; return lightingChanged; }
      if (observation.instanceId !== instance.id || observation.layoutRevision !== layout.navigation.revision ||
        !Number.isSafeInteger(observation.observedAt) || !Number.isSafeInteger(observation.expiresAt) || observation.observedAt > sharedNow || observation.expiresAt <= sharedNow ||
        observation.expiresAt - observation.observedAt > 5000) {
        lastObservationIssue = "Use a current observation and shared clock for this place."; suspendActors(); return lightingChanged;
      }
      const decorative = companionNpcObservationIsDecorative(observation);
      if (!reactive && !decorative) { lastObservationIssue = "Resume reactive NPCs from the owning snapshot before applying observations."; suspendActors(); return lightingChanged; }
      // Decorative phase covers all bounded NPCs without replay; distance only
      // culls animation. Reactive prediction also keeps one common actor set.
      const sampled = reactive ? sampleCompanionNpcs(layout, runtime, observation, sharedNow)
        : { runtime: sharedNow - runtime.simulatedAt >= companionNpcLimits.stepMs ? sampleCompanionNpcDecoration(decorativePlan, sharedNow) : runtime,
          caughtUp: true, error: null };
      runtime = sampled.runtime; lastObservationIssue = sampled.error;
      if (sampled.error) { suspendActors(); return lightingChanged; }
      const crowdChanged = nightclubCrowd?.update((sharedNow - instance.sharedStartedAt) / 1000,
        true, reducedMotion, participants, viewer) ?? false;
      let moving = false;
      for (const sample of runtime.npcs) {
        const actor = actors.get(sample.id);
        if (!actor || controlledNpcs.has(sample.id)) continue;
        if (residentSnapshotIssue && !isDutyMedic(sample.id)) { actor.group.visible = false; continue; }
        const nearby = !viewer || Math.hypot(sample.pose.point[0] - viewer[0], sample.pose.point[2] - viewer[2]) <= 18;
        actor.group.visible = nearby;
        if (!nearby) { actor.mixer.stopAllAction(); actor.action = undefined; continue; }
        // Apply only nearby visible rig work; world geometry remains intact.
        animate(actor, sample, sharedNow, reducedMotion); actor.sample = sample; moving ||= sample.moving || !!actor.action;
      }
      moving = updateResidents(sharedNow, viewer, reducedMotion) || moving;
      // A server-acknowledged food receipt drives the physical handover. It
      // never grants stock, charges funds, eats food or banks carried goods.
      const cue = observation.cues.find(cue => cue.kind === "handover-food" && cue.startsAt <= sharedNow && sharedNow < cue.expiresAt &&
        runtime.npcs.some(npc => npc.id === cue.npcId && npc.cue === cue.kind && npc.serviceId === cue.serviceId && !npc.issue));
      const service = cue ? layout.services.find(service => service.id === cue.serviceId && service.npcId === cue.npcId && serviceAvailable(service)) : undefined;
      tray.visible = !!service;
      if (cue && service) {
        const amount = Math.min(1, Math.max(0, (sharedNow - cue.startsAt) / 650));
        tray.position.set(service.anchor.point[0] + (service.approach.point[0] - service.anchor.point[0]) * amount, .95,
          service.anchor.point[2] + (service.approach.point[2] - service.anchor.point[2]) * amount);
        moving = true;
      }
      return lightingChanged || crowdChanged || moving || reactive && !sampled.caughtUp;
    },
    setUtilities(next, displayedInstanceId) {
      if (disposed) return;
      const reject = (message: string) => { utilities = null; utilityIssue = message; };
      if (displayedInstanceId !== instance.id) { reject("Use utilities from this displayed place instance."); return; }
      if (!next) { utilities = null; return; }
      const scope = next.scope;
      if (next.version !== 1 || !scope || scope.version !== 1
        || !validUtilityKey(scope.worldKey) || !validUtilityKey(scope.clockEpochKey) || !validUtilityKey(scope.zoneKey)
        || !Number.isSafeInteger(next.sampledAtRealMs) || next.sampledAtRealMs < instance.sharedStartedAt
        || next.sampledAtRealMs < utilitySampledAt
        || projectScenePowerFactor(next.power, next.sampledAtRealMs) === null
        || residentWorldKey !== null && scope.worldKey !== residentWorldKey
        || residentClock !== null && scope.clockEpochKey !== residentClock.epochKey) {
        reject("Refresh this place's admitted utility projection."); return;
      }
      if (utilityScope !== null && (scope.version !== utilityScope.version || scope.worldKey !== utilityScope.worldKey
        || scope.clockEpochKey !== utilityScope.clockEpochKey || scope.zoneKey !== utilityScope.zoneKey)) {
        reject("Reload this place after its utility scope changes."); return;
      }
      utilityScope ??= { version: scope.version, worldKey: scope.worldKey, clockEpochKey: scope.clockEpochKey, zoneKey: scope.zoneKey };
      utilitySampledAt = next.sampledAtRealMs;
      // The shared projector already freezes these records. Bridged or mutable
      // input still receives the original defensive copies after admission.
      utilities = Object.isFrozen(next) && Object.isFrozen(scope) && Object.isFrozen(next.power) ? next
        : Object.freeze({ ...next, scope: Object.freeze({ ...scope }), power: Object.freeze({ ...next.power }) });
      utilityIssue = null;
    },
    resumeNpcs(snapshot) {
      if (disposed || snapshot.instanceId !== instance.id || snapshot.layoutRevision !== layout.navigation.revision ||
        !Number.isSafeInteger(snapshot.simulatedAt) || snapshot.simulatedAt < runtime.simulatedAt) throw new Error("Use a fresh NPC snapshot from this place owner.");
      runtime = createCompanionNpcRuntime(layout, instance.id, instance.sharedStartedAt, snapshot); reactive = true; lastObservationIssue = null; suspendActors();
    },
    resumeDecoration(sharedNow) {
      if (disposed || sharedNow < runtime.simulatedAt) throw new Error("Use a current shared clock for decorative resume.");
      runtime = sampleCompanionNpcDecoration(decorativePlan, sharedNow); reactive = false; lastObservationIssue = null; suspendActors();
    },
    setResidents(world) {
      if (disposed || world === residentWorld) return;
      residentDirty = true;
      const reject = (reason: string) => {
        residentSnapshotIssue = reason; residentWorld = null; residentBinding = null; suspendActors();
      };
      if (world) {
        if (!world.clock || !Array.isArray(world.residents) || world.residents.length > companionResidentKeys.length || world.worldKey !== world.clock.worldKey
          || !Array.isArray(world.placeBindings) || world.placeBindings.length > companionResidentKeys.length * 2) { reject("Refresh the complete resident world snapshot."); return; }
        try { sampleCompanionWorldClock(world.clock, world.clock.epochRealMs); }
        catch { reject("Use a supported resident world clock."); return; }
        // Epoch migration needs explicit owner re-admission, not an implicit reset.
        if (world.worldKey === residentWorldKey && residentClock
          && (world.clock.epochKey !== residentClock.epochKey || world.clock.epochRealMs !== residentClock.epochRealMs
            || world.clock.epochGameMinute !== residentClock.epochGameMinute || world.clock.realMsPerGameMinute !== residentClock.realMsPerGameMinute)) {
          reject("Reload the place after a resident clock epoch changes."); return;
        }
        const ids = new Set<string>(), claims = new Set<string>();
        for (const resident of world.residents) {
          const identity = resident?.identity;
          const previous = identity && world.worldKey === residentWorldKey ? residentRevisions.get(identity.id) : undefined;
          if (!identity || identity.actorKind !== "npc" || identity.rosterVersion !== 1 || !companionResidentKeys.includes(identity.actorKey)
            || identity.id !== `npc:${world.worldKey.length}:${world.worldKey}:${identity.actorKey}` || ids.has(identity.id)
            || identity.worldKey !== world.worldKey || resident.clockEpochKey !== world.clock.epochKey
            || !Number.isSafeInteger(resident.revision) || resident.revision < 0
            || !Number.isSafeInteger(resident.updatedAtGameMinute) || resident.updatedAtGameMinute < 0
            || previous && (resident.revision < previous.revision || resident.updatedAtGameMinute < previous.gameMinute)) {
            reject("Use a fresh, unique resident snapshot for this world."); return;
          }
          ids.add(identity.id);
          const mapping = resident.serviceActor;
          if (mapping?.placeId === layout.placeId && !isDutyMedic(mapping.npcId)) {
            if (claims.has(mapping.npcId)) { reject("Each venue actor needs one resident binding."); return; }
            claims.add(mapping.npcId);
          }
        }
        const places = new Set<string>(), gameInstances = new Set<string>(), residentInstances = new Set<string>();
        for (const binding of world.placeBindings) {
          if (!binding || ![binding.placeId, binding.gameInstanceId, binding.residentInstanceId, binding.layoutId].every(value => typeof value === "string" && value.length > 0 && value.length <= 240)
            || places.has(binding.placeId) || gameInstances.has(binding.gameInstanceId) || residentInstances.has(binding.residentInstanceId)
            || (binding.gameInstanceId === instance.id || binding.placeId === layout.placeId)
              && (binding.gameInstanceId !== instance.id || binding.placeId !== layout.placeId || binding.layoutId !== layout.id)) {
            reject("Refresh the unique resident place bindings."); return;
          }
          places.add(binding.placeId); gameInstances.add(binding.gameInstanceId); residentInstances.add(binding.residentInstanceId);
        }
      }
      const changedWorld = world && residentWorldKey !== null && world.worldKey !== residentWorldKey;
      if (changedWorld) {
        releaseResidentActors();
        controlledNpcs.forEach(id => { const actor = actors.get(id); if (actor) delete actor.group.userData.companionResidentId; });
        controlledNpcs.clear();
        residentRevisions.clear();
      }
      residentWorld = world; residentBinding = null; workingNpcs.clear(); residentSnapshotIssue = null;
      if (!world) { suspendActors(); return; }
      residentWorldKey = world.worldKey;
      residentClock = { ...world.clock };
      const bindings = world.placeBindings?.filter(value => value.gameInstanceId === instance.id && value.placeId === layout.placeId && value.layoutId === layout.id) ?? [];
      if (bindings.length === 1) residentBinding = bindings[0];
      else if (world.residents.some(resident => resident.serviceActor?.placeId === layout.placeId)) residentSnapshotIssue = "Refresh the resident binding for this venue.";
      const seen = new Set<string>();
      for (const resident of world.residents) {
        const mapping = resident.serviceActor;
        if (mapping?.placeId === layout.placeId && !isDutyMedic(mapping.npcId)) {
          controlledNpcs.add(mapping.npcId);
        }
        seen.add(resident.identity.id);
      }
      for (const [id, { actor }] of residentActors) {
        if (seen.has(id)) continue;
        actor.mixer.stopAllAction(); actor.mixer.uncacheRoot(actor.mixer.getRoot()); disposeLagosAssetPart(actor.group); residentActors.delete(id); residentArtIssues.delete(id);
      }
      suspendActors();
    },
    resident(object) {
      if (disposed || !residentWorld || !residentBinding) return null;
      let found: string | undefined;
      let attached = false;
      for (let node: THREE.Object3D | null = object; node; node = node.parent) {
        if (!node.visible) return null;
        if (node === group) attached = true;
        if (!attached && !found) found = residentBodies.get(node);
      }
      return attached ? found ?? null : null;
    },
    focus(object, player) {
      if (disposed) return null;
      if (exitMarker && object === exitMarker && exitMarker.parent === group && streetExit) {
        for (let node: THREE.Object3D | null = object; node; node = node.parent) if (!node.visible) return null;
        return { id: waterfrontPlace ? COMPANION_WATERFRONT_EXIT : COMPANION_CITY_PLACE_EXIT, kind: "service", label: "Go outside", point: copyPoint(streetExit),
          reachable: Math.hypot(player[0] - streetExit[0], player[2] - streetExit[2]) <= 1.25 };
      }
      let service: CompanionPlaceService | null | undefined;
      let bound = false, attached = false;
      // Verify the whole ancestry, including any hidden parent above the scene.
      // A detached/stale target must not retain an actionable service identity.
      for (let node: THREE.Object3D | null = object; node; node = node.parent) {
        if (!node.visible) return null;
        if (node === group) attached = true;
        if (!attached && !bound && focusBindings.has(node)) { service = focusBindings.get(node); bound = true; }
      }
      if (!attached) return null;
      if (!service || !serviceAvailable(service) || layout.kind === "nightclub" && service.kind === "food") return null;
      return { id: service.id, kind: "service", label: service.label, point: service.approach.point,
        reachable: !!planRoomRoute(layout.navigation, player, service.approach.point) };
    },
    targets: () => Object.freeze(targetRegistry.filter(target => serviceAvailable(target.service))),
    npcSnapshot: () => immutableCompanionNpcSnapshot(runtime),
    issues: () => [...issueSet, ...runtime.issues, ...residentArtIssues.values(), ...residentPresentationIssues.values(), ...(lastObservationIssue ? [lastObservationIssue] : []), ...(residentSnapshotIssue ? [residentSnapshotIssue] : []), ...(utilityIssue ? [utilityIssue] : [])],
    dispose() {
      if (disposed) return; disposed = true;
      nightclubCrowd?.dispose(); nightclubCrowd = undefined;
      power.dispose(); utilities = null; utilityScope = null; utilityIssue = null;
      releaseResidentActors(); residentBodies.clear(); controlledNpcs.clear(); workingNpcs.clear(); residentRevisions.clear(); residentWorld = null; residentWorldKey = null; residentClock = null; residentBinding = null;
      actors.forEach(actor => { actor.mixer.stopAllAction(); actor.mixer.uncacheRoot(actor.mixer.getRoot()); }); actors.clear();
      assetGroups.forEach(disposeLagosAssetPart); assetGroups.length = 0;
      // Target meshes are local. Remove cache-owned geometry before cleanup.
      disposeLagosPlaceGeometry(group); group.clear(); group.removeFromParent(); targets.length = 0; targetRegistry = Object.freeze([]);
      focusBindings.clear(); targetServices.clear(); issueSet.clear();
    },
  };
}
