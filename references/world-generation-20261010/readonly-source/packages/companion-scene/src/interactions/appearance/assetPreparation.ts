import { createCompanionAssetPreparationQueue } from "../../../../../packages/utils/src/companionAssetPreparation";

// The host's download queue and this document's decode queue have separate
// lifetimes. A decode slot includes the read, so waiting jobs hold no GLB bytes.
export const prepareAsset = createCompanionAssetPreparationQueue();
