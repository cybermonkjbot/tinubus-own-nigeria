import * as THREE from "three";
import { companionWaterfrontPlaces } from "../../../../../packages/utils/src/companionWaterfront";
import { generateWaterfrontLayout } from "../../../../../packages/utils/src/companionWaterfrontLayouts";
import { createWaterfrontGeometry, releaseWaterfrontGeometry, updateWaterfrontGeometry } from "../lagos-places/waterfrontGeometry";
import type { CompanionAssetDelivery } from "../../../../../packages/utils/src/companionAssets";
import { companionCityBankZ, companionCityPlan } from "../../../../../packages/utils/src/companionCity";

type Shoreline = { group: THREE.Group; materials: THREE.Material[]; textures: THREE.Texture[] };

/** Water-side dressing only. Occupied city navigation, road decks and saved plots stay unchanged. */
export function createCityWaterfront(delivery: () => CompanionAssetDelivery, changed: () => void) {
  const group = new THREE.Group(); group.name = "Lagos waterfront craft";
  const terminals = companionWaterfrontPlaces.filter(place => place.kind === "water-terminal" && place.cityEntrance);
  const loaded = new Map<string, Shoreline>();
  let disposed = false;
  let provider: CompanionAssetDelivery | undefined;
  const fitsWater = (bounds: THREE.Box3) => {
    const { southBank, northBank } = companionCityPlan.lagoon;
    const xs = [bounds.min.x, bounds.max.x, ...southBank.map(point => point[0]), ...northBank.map(point => point[0])]
      .filter(x => x >= bounds.min.x && x <= bounds.max.x);
    return xs.every(x => bounds.min.z > companionCityBankZ(southBank, x) + .2
      && bounds.max.z < companionCityBankZ(northBank, x) - .2)
      && companionCityPlan.bridges.every(bridge => bounds.max.x < bridge.x - bridge.width / 2 - 3
        || bounds.min.x > bridge.x + bridge.width / 2 + 3);
  };
  const release = (entry: Shoreline) => {
    // Restore generated fallbacks and detach leased models before collecting.
    releaseWaterfrontGeometry(entry.group);
    const geometries = new Set<THREE.BufferGeometry>();
    entry.group.traverse(node => {
      if (node instanceof THREE.InstancedMesh) node.dispose();
      if (node instanceof THREE.Mesh) geometries.add(node.geometry);
    });
    geometries.forEach(geometry => geometry.dispose());
    entry.materials.forEach(material => material.dispose()); entry.textures.forEach(texture => texture.dispose());
    entry.group.removeFromParent(); entry.group.clear();
  };
  const clear = () => {
    loaded.forEach(release); loaded.clear();
  };
  return {
    group,
    update(viewer: THREE.Vector3, daylight: number, seconds: number, reducedMotion: boolean) {
      if (disposed) return false;
      let updated = false;
      const next = delivery();
      if (provider !== next) { clear(); provider = next; updated = true; }
      for (const place of terminals) {
        const point = place.cityEntrance!;
        const nearby = Math.hypot(viewer.x - point[0], viewer.z - point[2]) < 240;
        let entry = loaded.get(place.id);
        if (!entry && nearby) {
          const layout = generateWaterfrontLayout({ placeId: place.id, kind: place.kind, seed: place.seed });
          const root = new THREE.Group(); root.name = `${place.label} shoreline`;
          root.position.fromArray(point);
          // The first two terminals approach from the south bank. Local -Z faces water.
          root.rotation.y = place.id === "cms-jetty" || place.id === "five-cowries-terminal" ? Math.PI : 0;
          entry = { group: root, materials: [], textures: [] };
          try {
            createWaterfrontGeometry({ ...layout, waterfront: { ...layout.waterfront!, surfaces: [],
              props: layout.waterfront!.props.filter(prop => ["boat", "hyacinth", "debris"].includes(prop.kind)) } },
            root, entry.materials, entry.textures, false, next, changed, fitsWater);
          } catch (error) {
            release(entry);
            throw error;
          }
          group.add(root); loaded.set(place.id, entry); updated = true;
        }
        if (entry) {
          if (!nearby) {
            release(entry); loaded.delete(place.id); updated = true;
          } else updated = updateWaterfrontGeometry(entry.group, seconds, reducedMotion, true, 1, daylight) || updated;
        }
      }
      return updated;
    },
    dispose() {
      if (disposed) return; disposed = true;
      clear(); group.removeFromParent(); group.clear();
    },
  };
}
