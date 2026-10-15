import { createContext, useContext } from "react";
import { companionAssetUrl, type CompanionAssetDelivery } from "../../../../../packages/utils/src/companionAssets";

// The authenticated host injects its existing cache. No uploads, credentials,
// alternate CDN or startup warm-up belong to the character/room renderer.
export const CompanionAssetsContext = createContext<CompanionAssetDelivery | undefined>(undefined);
export const useCompanionAssets = () => useContext(CompanionAssetsContext);

/** URL resolution is lazy; the existing GLB loader owns transfer and integrity. */
export const publicCompanionAssets: CompanionAssetDelivery = {
  available: key => companionAssetUrl(key) !== null,
  async acquire(key, signal) {
    signal.throwIfAborted();
    const url = companionAssetUrl(key);
    if (!url) throw new Error("This room isn't available yet.");
    return url;
  },
};
