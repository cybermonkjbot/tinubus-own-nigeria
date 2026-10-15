import * as THREE from "three";
import type { CompanionWorldInstance } from "../../../../../packages/utils/src/companionWorld";
import type { CompanionAssetDelivery, CompanionAssetKey } from "../../../../../packages/utils/src/companionAssets";
import type { CompanionResidentWorldView } from "../../../../../packages/utils/src/companionResidentSimulation";
import { acquireCompanionGltf, type GltfLease } from "../appearance/gltfAssets";
import { createLagosPlaceScene, type LagosPlaceScene } from "./scene";
import type { EnvironmentAsset, EnvironmentParticipant } from "../scenes/environments";
import type { CompanionWorldUtilities } from "../../../../../packages/utils/src/companionWorldUtilities";
import type { CompanionNpcObservation } from "../../../../../packages/utils/src/companionNpcs";
import { createUtilityScopeKey } from "../scenes/utilityScopeKey";

const noObservationItems: readonly never[] = Object.freeze([]);

/** Leases only destination assets. Generated fixtures and cached assets have separate disposers. */
export function createWorldEnvironmentSlot(parent: THREE.Object3D, changed: (state: { loading: boolean; error: string | null }) => void) {
  let current: LagosPlaceScene | undefined, requested: CompanionWorldInstance | undefined, source: CompanionAssetDelivery | undefined;
  let leases: GltfLease[] = [], generation = 0, closed = false, error: string | null = null;
  let pendingLeases: GltfLease[] = [];
  let utilityScope = "", utilities: CompanionWorldUtilities | null = null, loadError: string | null = null;
  const utilityKey = createUtilityScopeKey();
  let loading = false;
  // Borrowed only during the scene's synchronous update. No authority is added.
  const observation: CompanionNpcObservation = { instanceId: "", layoutRevision: "", observedAt: 0, expiresAt: 0,
    danger: noObservationItems, traffic: noObservationItems, tasks: noObservationItems, cues: noObservationItems, queues: noObservationItems };
  const viewerPoint: [number, number, number] = [0, 0, 0];
  return {
    request(instance: CompanionWorldInstance | undefined, delivery: CompanionAssetDelivery, retry = false) {
      if (closed || !retry && requested?.id === instance?.id && requested?.sharedStartedAt === instance?.sharedStartedAt && source === delivery) return;
      pendingLeases.forEach(lease => lease.release()); pendingLeases = [];
      requested = instance; source = delivery; const token = ++generation;
      if (!instance?.layout) { current?.dispose(); current = undefined; leases.forEach(lease => lease.release()); leases = []; error = loadError = null; loading = false; return; }
      loading = true;
      const keys: CompanionAssetKey[] = [...new Set(instance.layout.assets.map(asset => asset.key))];
      if (instance.layout.kind === "lounge" && instance.layout.placeId === "dance-again" && !keys.includes("bestieDanceAgainInterior")) keys.push("bestieDanceAgainInterior");
      changed({ loading: true, error: null });
      loadError = null;
      const nextLeases = keys.map(key => acquireCompanionGltf(key, delivery));
      pendingLeases = nextLeases;
      Promise.all(nextLeases.map(lease => lease.promise)).then(assets => {
        if (closed || token !== generation) { nextLeases.forEach(lease => lease.release()); return; }
        const decoded = new Map(keys.map((key, index) => [key, { scene: assets[index].scene, animations: assets[index].animations } as EnvironmentAsset]));
        const next = createLagosPlaceScene(instance.layout!, decoded, { id: instance.id, sharedStartedAt: instance.sharedStartedAt }, delivery,
          () => { if (!closed && token === generation) changed({ loading, error }); });
        next.setUtilities(utilities, instance.id);
        const issues = next.issues();
        if (issues.length) { next.dispose(); throw new Error(issues[0]); }
        parent.add(next.group); current?.dispose(); leases.forEach(lease => lease.release()); leases = nextLeases; pendingLeases = []; current = next; error = null;
        loading = false;
        changed({ loading: false, error: null });
      }).catch(reason => {
        nextLeases.forEach(lease => lease.release());
        // Failed preparation may include fulfilled assets. Drop only this
        // request's pending list, never a replacement request or the live scene.
        if (pendingLeases === nextLeases) pendingLeases = [];
        if (!closed && token === generation) { loading = false; loadError = error = reason instanceof Error ? reason.message : "The place couldn't download."; changed({ loading: false, error }); }
      });
    },
    retry() { if (requested && source) this.request(requested, source, true); },
    get world() { return current?.environment; }, get group() { return current?.group; }, get scene() { return current; },
    get pending() { return !!requested?.layout && current?.environment.instanceId !== requested.id || loading || !!error; },
    setUtilities(next: CompanionWorldUtilities | null) {
      if (closed) return;
      const scope = next ? utilityKey.read(next.scope) : utilityScope;
      if (utilityScope && scope !== utilityScope && requested && source) {
        current?.dispose(); current = undefined;
        leases.forEach(lease => lease.release()); leases = [];
        utilities = next; utilityScope = scope;
        this.request(requested, source, true);
      } else { utilities = next; utilityScope = scope; }
    },
    update(dt: number, participants: readonly (EnvironmentParticipant & { isLocal?: boolean })[], enabled: boolean, now: number, reducedMotion = false,
      residents: CompanionResidentWorldView | null = null) {
      if (!current || !requested?.layout) return false;
      current.setResidents(residents);
      current.setUtilities(utilities, requested.id);
      const layout = requested.layout;
      const viewer = participants.find(participant => participant.isLocal && Number.isFinite(participant.x) && Number.isFinite(participant.z));
      const sharedNow = Math.floor(now);
      observation.instanceId = requested.id; observation.layoutRevision = layout.navigation.revision;
      observation.observedAt = sharedNow - 2000; observation.expiresAt = sharedNow + 1000;
      if (viewer) { viewerPoint[0] = viewer.x; viewerPoint[1] = layout.navigation.floorY; viewerPoint[2] = viewer.z; }
      // Decorative routines only. No authoritative danger, queues or service receipt is invented.
      const updated = current.update(dt, sharedNow, observation, enabled,
        viewer ? viewerPoint : undefined, reducedMotion, participants);
      const issue = loadError ?? current.issues()[0] ?? null;
      if (issue !== error) { error = issue; changed({ loading, error }); }
      return updated;
    },
    dispose() {
      if (closed) return;
      closed = true; generation++; current?.dispose(); current = undefined;
      leases.forEach(lease => lease.release()); pendingLeases.forEach(lease => lease.release()); leases = []; pendingLeases = [];
      requested = undefined; source = undefined; utilities = null; utilityScope = ""; utilityKey.clear();
      observation.instanceId = ""; observation.layoutRevision = "";
    },
  };
}
