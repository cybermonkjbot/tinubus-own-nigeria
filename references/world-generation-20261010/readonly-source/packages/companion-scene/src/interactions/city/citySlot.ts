import * as THREE from "three";
import { companionCityCameraObstacles, companionCityHasDetailCollision, companionCityHasRoadTreatments, companionCityHasParkPaths, companionCityHasTheatre, companionCityHasTransit, companionCityHasStreetDensity, companionCityTransitPlan, isCompanionCity } from "../../../../../packages/utils/src/companionCity";
import { companionCityResidenceEntrance, COMPANION_CITY_HOME_ENTER, COMPANION_CITY_HOME_EXIT, type CompanionCityResidence } from "../../../../../packages/utils/src/companionCityHousing";
import { companionCityPlaceDestinations, companionCityPlaceEnterService,
  type CompanionPlaceExpansionDestination } from "../../../../../packages/utils/src/companionPlacesExpansion";
import { companionWaterfrontPlaces, companionWaterfrontEnterService, type CompanionWaterfrontPlace } from "../../../../../packages/utils/src/companionWaterfront";
import { routeSegmentClear } from "../../../../../packages/utils/src/companionNavigation";
import type { CompanionWorldSnapshot } from "../../../../../packages/utils/src/companionWorld";
import { sampleCompanionWorldClock } from "../../../../../packages/utils/src/companionWorldClock";
import type { CompanionEnvironment, ScenePoint } from "../scenes/environments";
import type { LagosPlaceFocus } from "../lagos-places/scene";
import { createCityScene } from "./cityScene";
import { createCityAtmosphere } from "./cityAtmosphere";
import type { CompanionWorldUtilities } from "../../../../../packages/utils/src/companionWorldUtilities";
import { createScenePowerAdapter, type ScenePowerAdapter } from "../scenes/scenePower";
import type { CompanionAssetDelivery } from "../../../../../packages/utils/src/companionAssets";
import { publicCompanionAssets } from "../appearance/CompanionAssets";
import type { CompanionGunfireState } from "../../../../../packages/utils/src/companionGunfire";
import type { TransitRide } from "../../../../../packages/utils/src/companionCityTransit";
import { createCityTransit } from "./cityTransit";
import { createCityTraffic } from "./cityTraffic";
import { companionHomeDoor } from "../../../../../packages/utils/src/companionHomeDoor";

const residenceFields = { cityVersion: true, parcelId: true, unit: true } satisfies Record<keyof CompanionCityResidence, true>;
const residenceKeys = Object.keys(residenceFields) as (keyof CompanionCityResidence)[];
function sameResidence(a: CompanionCityResidence | null, b: CompanionCityResidence | null) {
  if (!a || !b) return a === b;
  for (const key of residenceKeys) if (a[key] !== b[key]) return false;
  return true;
}

/** Additive to the leased Town Square; never clones or mutates the cached source. */
export function createCitySlot(scene: THREE.Scene, renderer: THREE.WebGLRenderer, camera: THREE.PerspectiveCamera,
  lights: readonly THREE.Light[], delivery: () => CompanionAssetDelivery = () => publicCompanionAssets,
  changed: () => void = () => {}) {
  let body: ReturnType<typeof createCityScene> | undefined;
  let transit: ReturnType<typeof createCityTransit> | undefined;
  let traffic: ReturnType<typeof createCityTraffic> | undefined;
  let atmosphere: ReturnType<typeof createCityAtmosphere> | undefined;
  let power: ScenePowerAdapter | undefined;
  let marker: THREE.Mesh<THREE.TorusGeometry, THREE.MeshStandardMaterial> | undefined;
  let publicMarkerGeometry: THREE.TorusGeometry | undefined, publicMarkerMaterial: THREE.MeshStandardMaterial | undefined;
  const publicMarkers = new Map<THREE.Object3D, CompanionPlaceExpansionDestination>();
  const waterfrontMarkers = new Map<THREE.Object3D, CompanionWaterfrontPlace>();
  const homeDoorObjects = new Set<THREE.Object3D>();
  let homeDoorTarget: THREE.Mesh<THREE.PlaneGeometry, THREE.MeshBasicMaterial> | undefined;
  let parent: THREE.Group | undefined, scope = "", world: CompanionEnvironment | undefined;
  let snapshot: CompanionWorldSnapshot | undefined;
  let scopeInputs: { userId: string; instanceId: string; revision: string; residence: CompanionCityResidence | null; key: string } | undefined;
  let disposed = false;
  const hidden: THREE.Object3D[] = [];
  const clear = () => {
    traffic?.dispose(); traffic = undefined;
    transit?.dispose(); transit = undefined;
    power?.dispose(); power = undefined;
    body?.dispose(); atmosphere?.dispose(); body = undefined; atmosphere = undefined;
    marker?.removeFromParent(); marker?.geometry.dispose(); marker?.material.dispose(); marker = undefined;
    publicMarkers.forEach((_, object) => object.removeFromParent()); publicMarkers.clear();
    waterfrontMarkers.forEach((_, object) => object.removeFromParent()); waterfrontMarkers.clear();
    homeDoorObjects.clear();
    homeDoorTarget?.removeFromParent(); homeDoorTarget?.geometry.dispose(); homeDoorTarget?.material.dispose(); homeDoorTarget = undefined;
    publicMarkerGeometry?.dispose(); publicMarkerMaterial?.dispose(); publicMarkerGeometry = undefined; publicMarkerMaterial = undefined;
    hidden.forEach(object => { object.visible = true; }); hidden.length = 0;
    parent = undefined; world = undefined;
  };
  return {
    setBillboardAd(value?: import("../../../../../packages/utils/src/companionBillboardAds").CompanionBillboardAd | null) { body?.setBillboardAd(value); },
    billboardVisibility(width: number, height: number) { return body?.billboardVisibility(scene, camera, width, height) ?? null; },
    pickedBillboardAd(object: THREE.Object3D) { return body?.pickedBillboardAd(object) ?? null; },
    get billboardWakeAfter() { return body?.billboardWakeAfter ?? Infinity; },
    get active() { return !!body; },
    get world() { return world; },
    walkingBodies() { return [...(traffic?.walkingBodies() ?? []), ...(transit?.trafficObstacles() ?? [])]; },
    rider(ride: TransitRide, now: number, passengerId: string, visitRevision: number) {
      return transit?.rider(ride, now, passengerId, visitRevision) ?? null;
    },
    get transitMoving() { return !!transit?.moving || !!traffic?.moving; },
    get transitWakeAt() { return Math.min(transit?.nextWakeAt ?? Infinity, traffic?.nextWakeAt ?? Infinity); },
    prepare(next?: CompanionWorldSnapshot) {
      if (disposed) return;
      snapshot = next;
      let key = "";
      if (next?.instance && next.player?.instanceId === next.instance.id) {
        const userId = next.player.userId, instanceId = next.instance.id, revision = next.instance.navigation.revision;
        const residence = next.player.residence ?? null;
        if (!scopeInputs || scopeInputs.userId !== userId || scopeInputs.instanceId !== instanceId || scopeInputs.revision !== revision
          || !sameResidence(scopeInputs.residence, residence)) {
          // Copy values so updates to an existing snapshot still invalidate the scene.
          scopeInputs = { userId, instanceId, revision, residence: residence ? { ...residence } : null,
            key: `${userId}:${instanceId}:${revision}:${JSON.stringify(residence)}` };
        }
        key = scopeInputs.key;
      } else scopeInputs = undefined;
      if (key !== scope) { clear(); scope = key; }
      transit?.setSnapshot(next?.transit && next.instance && next.transit.instanceId === next.instance.id
        && next.player?.status === "present" ? next.transit : null);
    },
    sync(base: CompanionEnvironment | undefined, group: THREE.Group | undefined, pending: boolean) {
      if (disposed) return;
      const instance = snapshot?.instance;
      if (!base || !group || pending || !instance || snapshot?.player?.instanceId !== instance.id) return;
      if (parent === group) return;
      clear(); parent = group;
      if (isCompanionCity(instance.navigation)) {
        body = createCityScene(companionCityHasDetailCollision(instance.navigation), delivery, changed,
          companionCityHasRoadTreatments(instance.navigation), companionCityHasParkPaths(instance.navigation), companionCityHasTheatre(instance.navigation), companionCityHasTransit(instance.navigation), companionCityHasStreetDensity(instance.navigation)); group.add(body.group);
        power = createScenePowerAdapter(body.powerBindings);
        atmosphere = createCityAtmosphere(scene, renderer, camera, lights);
        const plan = instance.transitPlan ?? companionCityTransitPlan(instance.navigation);
        if (companionCityHasTransit(instance.navigation) && plan?.navigationRevision === instance.navigation.revision) {
          transit = createCityTransit(plan, delivery, changed, instance.navigation.floorY);
          group.add(transit.group);
          transit.setSnapshot(snapshot?.transit?.instanceId === instance.id && snapshot.player?.status === "present" ? snapshot.transit : null);
          traffic = createCityTraffic(plan, instance.navigation, instance.sharedStartedAt, delivery, changed);
          group.add(traffic.group);
        }
        // The retained district's small river patch would cover the continuous lagoon.
        group.traverse(object => {
          if (!(object instanceof THREE.Mesh) || object === body?.water.mesh || !object.visible) return;
          for (let owner:THREE.Object3D|null=object;owner;owner=owner.parent) if(owner===body?.group)return;
          let node: THREE.Object3D | null = object;
          while (node && node !== group && node.userData.lifeRole === undefined) node = node.parent;
          if (node?.userData.lifeRole === "water" || /^(river|water surface|lagoon water)/i.test(object.name)) {
            object.visible = false; hidden.push(object);
          }
        });
        world = { ...base, instanceId: instance.id, navigation: instance.navigation, cameraHeightMax: 24,
          cameraPolicy: "third-person", fov: 72,
          cameraObstacles: [...(base.cameraObstacles ?? []), ...companionCityCameraObstacles(instance.navigation)] };
      }
      const homeDoor = instance.placeId === "own-home" && instance.room ? companionHomeDoor(instance.room) : null;
      if (homeDoor?.authored) group.traverse(object => {
        const name = object.name.replace(/_/g, " ");
        if (/^(Closed green door leaf|Door frame |Door round knob|Door knob |Door hinge |Bedroom v08 \| entry (door|handle|threshold))/i.test(name))
          homeDoorObjects.add(object);
      });
      if (homeDoor?.authored) {
        // Delivered Studio geometry batches the door with other shell meshes.
        // Keep picking tied to the physical doorway, without changing its art.
        homeDoorTarget = new THREE.Mesh(new THREE.PlaneGeometry(.95, 2.2),
          new THREE.MeshBasicMaterial({ side: THREE.DoubleSide, transparent: true, opacity: 0, depthWrite: false, colorWrite: false }));
        homeDoorTarget.name = "Home doorway interaction";
        homeDoorTarget.position.set(homeDoor.doorway[0], homeDoor.doorway[1] + 1.1, homeDoor.doorway[2]);
        if (instance.room?.layoutId !== "pink-bedroom-v02") {
          homeDoorTarget.rotation.y = Math.PI / 2;
          homeDoorTarget.position.x += .06;
        } else homeDoorTarget.position.z += .06;
        group.add(homeDoorTarget); homeDoorObjects.add(homeDoorTarget);
      }
      const entrance = isCompanionCity(instance.navigation) ? companionCityResidenceEntrance(snapshot?.player?.residence)
        : instance.placeId === "own-home" && snapshot?.player?.residence && !homeDoor?.authored ? instance.exit : null;
      if (entrance) {
        marker = new THREE.Mesh(new THREE.TorusGeometry(.65, .045, 6, 32), new THREE.MeshStandardMaterial({ color: "#dbca9d", emissive: "#6c5d38", emissiveIntensity: .3 }));
        marker.name = "Home entrance"; marker.rotation.x = -Math.PI / 2; marker.position.set(entrance[0], .035, entrance[2]);
        group.add(marker);
      }
      const venues = companionCityPlaceDestinations(instance.navigation);
      const waterfront = isCompanionCity(instance.navigation) ? companionWaterfrontPlaces.filter(place => place.cityEntrance
        && routeSegmentClear(instance.navigation, place.cityEntrance, place.cityEntrance)) : [];
      if (venues.length || waterfront.length) {
        publicMarkerGeometry = new THREE.TorusGeometry(.65, .045, 6, 32);
        publicMarkerMaterial = new THREE.MeshStandardMaterial({ color: "#d4dbcf", emissive: "#596a51", emissiveIntensity: .2 });
        for (const place of venues) {
          const point = place.city.entrance;
          const object = new THREE.Mesh(publicMarkerGeometry, publicMarkerMaterial);
          object.name = `${place.label} entrance`; object.rotation.x = -Math.PI / 2;
          object.position.set(point[0], instance.navigation.floorY + .035, point[2]);
          group.add(object); publicMarkers.set(object, place);
        }
        for (const place of waterfront) {
          const point = place.cityEntrance!;
          const object = new THREE.Mesh(publicMarkerGeometry, publicMarkerMaterial);
          object.name = `${place.label} entrance`; object.rotation.x = -Math.PI / 2;
          object.position.set(point[0], instance.navigation.floorY + .035, point[2]);
          group.add(object); waterfrontMarkers.set(object, place);
        }
      }
    },
    focus(object: THREE.Object3D, instanceId: number | undefined, point: ScenePoint): LagosPlaceFocus | null {
      const instance = snapshot?.instance, residence = snapshot?.player?.residence;
      if (!instance || snapshot?.player?.instanceId !== instance.id || !parent) return null;
      let attached = false;
      for (let node: THREE.Object3D | null = object; node; node = node.parent) {
        if (!node.visible) return null;
        if (node === parent) attached = true;
      }
      if (!attached) return null;
      const transitFocus = snapshot?.transitAvailability?.boarding === "unavailable" ? null : transit?.focus(object, point);
      if (transitFocus) return transitFocus;
      const entering = isCompanionCity(instance.navigation);
      const parcelId = entering ? body?.parcel(object, instanceId) : undefined;
      const waterfront = object.parent === parent ? waterfrontMarkers.get(object) : undefined;
      if (waterfront?.cityEntrance) return { id: companionWaterfrontEnterService(waterfront.id), kind: "service",
        label: `Enter ${waterfront.label}`, point: waterfront.cityEntrance,
        reachable: Math.hypot(point[0] - waterfront.cityEntrance[0], point[2] - waterfront.cityEntrance[2]) <= 1.25 };
      const venue = object.parent === parent ? publicMarkers.get(object) : undefined;
      const place = venue ?? (parcelId ? companionCityPlaceDestinations(instance.navigation).find(value => value.city.parcelId === parcelId) : undefined);
      if (place) {
        const target = place.city.entrance;
        return { id: companionCityPlaceEnterService(place.id), kind: "service",
          label: `Enter ${place.label}`, point: target,
          reachable: Math.hypot(point[0] - target[0], point[2] - target[2]) <= 1.25 };
      }
      let pickedHomeDoor = false;
      for (let node: THREE.Object3D | null = object; node && node !== parent; node = node.parent)
        if (homeDoorObjects.has(node)) pickedHomeDoor = true;
      if (!residence || !pickedHomeDoor && object !== marker && parcelId !== residence.parcelId) return null;
      const target = entering ? companionCityResidenceEntrance(residence) : instance.exit;
      if (!target || !entering && instance.placeId !== "own-home") return null;
      return { id: entering ? COMPANION_CITY_HOME_ENTER : COMPANION_CITY_HOME_EXIT, kind: "service",
        label: entering ? "Enter Home" : "Go outside", point: target,
        reachable: Math.hypot(point[0] - target[0], point[2] - target[2]) <= 1.25 };
    },
    update(target: THREE.Vector3, sharedNow: number, reducedMotion: boolean, utilities: CompanionWorldUtilities | null = null,
      gunfire?: CompanionGunfireState) {
      if (!body || !atmosphere) return false;
      let hour = 10, seconds = 0, parkSeconds:number|null=null;
      const clock = snapshot?.residentWorld?.clock;
      const scope = utilities?.scope;
      if (clock && scope && scope.worldKey === clock.worldKey && scope.clockEpochKey === clock.epochKey
        && scope.zoneKey === `place:${snapshot?.instance?.placeId}`) power?.update(utilities!.power, utilities!.sampledAtRealMs, reducedMotion);
      if (clock && Number.isSafeInteger(sharedNow) && sharedNow >= clock.epochRealMs) {
        const time = sampleCompanionWorldClock(clock, sharedNow);
        hour = (time.minuteOfDay + time.minuteProgress) / 60;
        parkSeconds = (sharedNow - clock.epochRealMs) / 1000;
        if (!reducedMotion) seconds = (sharedNow - clock.epochRealMs) / 1000;
      }
      atmosphere.update(hour, target);
      body.water.update(seconds, atmosphere.direction, atmosphere.horizon, atmosphere.daylight);
      const geometryChanged = body.update(target, atmosphere.daylight,parkSeconds,reducedMotion, gunfire, snapshot?.instance?.navigation, sharedNow);
      // Mains changes affect emission only, not the sun's shadow geometry.
      const transitChanged = transit?.update(sharedNow, target, reducedMotion) ?? false;
      const trafficChanged = traffic?.update(sharedNow, target, reducedMotion, snapshot, transit?.trafficObstacles()) ?? false;
      return geometryChanged || transitChanged || trafficChanged;
    },
    dispose() {
      if (disposed) return;
      disposed = true; snapshot = undefined; scope = ""; scopeInputs = undefined;
      clear();
    },
  };
}
