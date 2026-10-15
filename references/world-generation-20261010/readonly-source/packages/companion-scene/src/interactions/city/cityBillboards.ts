import * as THREE from "three";
import design from "../../../../../packages/assets/city/lagos-billboards-v1.json";
import { companionCityPlan } from "../../../../../packages/utils/src/companionCity";
import { companionBillboardAssetEntries } from "../../../../../packages/utils/src/companionBillboardAssets";
import type { CompanionAssetDelivery, CompanionAssetKey } from "../../../../../packages/utils/src/companionAssets";
import { publicCompanionAssets } from "../appearance/CompanionAssets";
import { createHostedArchitecture } from "./hostedArchitecture";
import { createCityBillboardAds } from "./cityBillboardAds";

/** Billboard hardware stays inside existing plots and camera-height reserves. */
export function createCityBillboards(mains: Readonly<{ value: number }>,
  delivery: () => CompanionAssetDelivery = () => publicCompanionAssets, changed: () => void = () => {}) {
  if (design.cityVersion !== companionCityPlan.version) throw new Error("Billboards require their authored city layout.");
  const placements = design.placements.map(spec => {
    const parcel = companionCityPlan.parcels.find(parcel => parcel.id === spec.parcelId);
    if (!parcel || parcel.x !== spec.position[0] || parcel.z !== spec.position[2] || parcel.yaw !== spec.yaw
      || parcel.width !== spec.footprint[0] || parcel.depth !== spec.footprint[1]
      || spec.maximumHeight !== parcel.floors * 3.3 + (parcel.kind === "tower" ? 6.6 : 3)
      || spec.screenBottom + spec.height + .12 > spec.maximumHeight
      || !Object.hasOwn(companionBillboardAssetEntries, spec.assetKey)) throw new Error("Billboard placement differs from its existing parcel.");
    return { ...spec, assetKey: spec.assetKey as CompanionAssetKey };
  });
  // No substitute ad or duplicate bundled model while its individual download
  // is unavailable. The underlying city building stays visible and usable.
  const ads = createCityBillboardAds(mains, changed);
  const hosted = createHostedArchitecture(placements, () => new THREE.Group(), mains, delivery, changed,
    { enter: 420, exit: 560, shadowEnter: 150, shadowExit: 205, retain: 900 }, daylight => .75 + .25 * (1 - daylight), ads);
  hosted.group.name = "Lagos electric billboards";
  return { ...hosted, setAd: ads.setAd, visibility: ads.visibility, pickedAd: ads.picked, get wakeAfter() { return ads.wakeAfter; },
    update(target: THREE.Vector3, daylight: number) { const dirty = hosted.update(target, daylight); ads.update(daylight); return dirty; },
    dispose() { hosted.dispose(); ads.dispose(); } };
}
