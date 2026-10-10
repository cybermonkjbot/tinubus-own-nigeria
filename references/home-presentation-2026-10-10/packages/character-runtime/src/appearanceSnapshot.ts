import type { CompanionAppearance } from "../../utils/src/companion";

// Keep full-rig invalidation exhaustive when the appearance contract grows.
// Appearance values are scalars; nested state needs its own comparison rule.
const appearanceFields = {
  baseId: true, look: true,
  bodyColor: true, topColor: true, trousersColor: true, footwearColor: true,
  accessoryVisible: true, necklaceVisible: true, hipChainVisible: true,
  eyeColor: true, noseColor: true, markingColor: true,
  garmentStyle: true, fabricTexture: true, clothingPattern: true, patternColor: true,
  eyeShape: true, browStyle: true, noseShape: true, mouthShape: true,
  cheekStyle: true, markingStyle: true, eyewear: true,
} satisfies Record<keyof CompanionAppearance, true>;
const allFields = Object.keys(appearanceFields) as (keyof CompanionAppearance)[];

/** Per-rig scalar storage, independent of the caller's mutable appearance object. */
export function createAppearanceSnapshot(fields: readonly (keyof CompanionAppearance)[] = allFields) {
  const values: (string | boolean | undefined)[] = new Array(fields.length);
  let captured = false;
  return {
    // A failed application must retry even if the next input returns to the old values.
    begin(input: Partial<CompanionAppearance>, force = false) {
      if (captured && !force) {
        let unchanged = true;
        for (let i = 0; i < fields.length; i++) if (values[i] !== input[fields[i]]) { unchanged = false; break; }
        if (unchanged) return false;
      }
      captured = false;
      return true;
    },
    // Call only after applying the complete appearance, including forced refreshes.
    capture(input: Partial<CompanionAppearance>) {
      for (let i = 0; i < fields.length; i++) values[i] = input[fields[i]];
      captured = true;
    },
  };
}
