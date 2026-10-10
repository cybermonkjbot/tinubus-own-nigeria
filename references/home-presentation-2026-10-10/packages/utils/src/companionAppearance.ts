import { characterBases, companionFaceChoices, type CharacterBaseId, type CompanionAppearance } from "./companion";

export const characterFaceDefaults: Record<CharacterBaseId, Partial<CompanionAppearance>> = {
  bestie: {}, bunny: {}, bear: {},
  cat: { eyeShape: "almond", noseShape: "button", noseColor: "#b97e74", markingStyle: "stripes", markingColor: "#50525b" },
  fox: { eyeShape: "almond", browStyle: "curious", noseColor: "#393634", markingStyle: "muzzle", markingColor: "#f4dfbe" },
  panda: { eyeShape: "round", noseShape: "wide", noseColor: "#393634", markingStyle: "mask", markingColor: "#393634" },
};
export const faceOptions = [
  { field: "eyeShape", label: "Eye shape", choices: [["classic", "Classic"], ["round", "Round"], ["almond", "Almond"], ["sleepy", "Sleepy"]] },
  { field: "browStyle", label: "Brows", choices: [["classic", "Classic"], ["soft", "Soft"], ["bold", "Bold"], ["curious", "Curious"]] },
  { field: "noseShape", label: "Nose", choices: [["classic", "Classic"], ["button", "Button"], ["wide", "Wide"]] },
  { field: "mouthShape", label: "Mouth", choices: [["classic", "Classic"], ["small", "Small smile"], ["wide", "Wide smile"]] },
  { field: "cheekStyle", label: "Cheeks", choices: [["none", "Plain"], ["blush", "Blush"], ["freckles", "Freckles"]] },
  { field: "markingStyle", label: "Markings", choices: [["none", "Plain"], ["muzzle", "Muzzle"], ["mask", "Eye patches"], ["stripes", "Stripes"]] },
] as const;
export const faceColorOptions = {
  Eyes: { field: "eyeColor", colors: [["Amber", "#b77d32"], ["Hazel", "#82763f"], ["Brown", "#624334"], ["Blue", "#6895ba"], ["Green", "#6c987b"], ["Violet", "#9880b5"]] },
  Nose: { field: "noseColor", colors: [["Peach", "#b9664d"], ["Rose", "#b97e74"], ["Cocoa", "#795342"], ["Charcoal", "#393634"]] },
  Markings: { field: "markingColor", colors: [["Cream", "#f4dfbe"], ["White", "#eee9df"], ["Caramel", "#b8855e"], ["Cocoa", "#795342"], ["Slate", "#50525b"], ["Charcoal", "#393634"]] },
} as const;
export function characterBaseValue(value: CompanionAppearance, baseId: CharacterBaseId): CompanionAppearance {
  return { ...value, baseId, bodyColor: characterBases.find(base => base.id === baseId)!.color };
}
/** Keep legacy authored colours when no override was saved. */
export function companionBodyColor(value: Pick<CompanionAppearance, "baseId" | "bodyColor">): string | undefined {
  return value.bodyColor ?? (value.baseId && ["cat", "fox", "panda"].includes(value.baseId)
    ? characterBases.find(base => base.id === value.baseId)?.color : undefined);
}
export function resetFace(value: CompanionAppearance): CompanionAppearance {
  const next = { ...value };
  for (const key of Object.keys(companionFaceChoices) as (keyof typeof companionFaceChoices)[]) {
    if (key !== "eyewear") delete next[key];
  }
  delete next.eyeColor; delete next.noseColor; delete next.markingColor;
  return next;
}

export const appearanceDefaults: CompanionAppearance = {
  look: "rose", bodyColor: "#efc18e", topColor: "#b97e74", trousersColor: "#393634", footwearColor: "#e8d3b8", accessoryVisible: true,
};
export function normalizeAppearance(value: CompanionAppearance): CompanionAppearance {
  return {
    ...characterFaceDefaults[value.baseId ?? "bestie"],
    ...value,
    baseId: value.baseId ?? "bestie", look: value.look,
    bodyColor: companionBodyColor(value) ?? appearanceDefaults.bodyColor,
    topColor: value.topColor ?? (value.look === "ivory" ? "#e7dfcc" : "#b97e74"),
    trousersColor: value.trousersColor ?? appearanceDefaults.trousersColor,
    footwearColor: value.footwearColor ?? appearanceDefaults.footwearColor,
    accessoryVisible: value.accessoryVisible !== false,
    necklaceVisible: value.necklaceVisible ?? value.accessoryVisible !== false,
    hipChainVisible: value.hipChainVisible ?? value.accessoryVisible !== false,
    eyeShape: value.eyeShape ?? characterFaceDefaults[value.baseId ?? "bestie"].eyeShape ?? "classic",
    browStyle: value.browStyle ?? characterFaceDefaults[value.baseId ?? "bestie"].browStyle ?? "classic",
    noseShape: value.noseShape ?? characterFaceDefaults[value.baseId ?? "bestie"].noseShape ?? "classic",
    mouthShape: value.mouthShape ?? "classic", cheekStyle: value.cheekStyle ?? "none",
    markingStyle: value.markingStyle ?? characterFaceDefaults[value.baseId ?? "bestie"].markingStyle ?? "none",
    eyewear: value.eyewear ?? "none",
    garmentStyle: value.garmentStyle ?? "bomber",
    fabricTexture: value.fabricTexture ?? "cotton",
    clothingPattern: value.clothingPattern ?? "plain",
    patternColor: value.patternColor ?? "#eee9df",
    eyeColor: value.eyeColor,
    noseColor: value.noseColor ?? characterFaceDefaults[value.baseId ?? "bestie"].noseColor,
    markingColor: value.markingColor ?? characterFaceDefaults[value.baseId ?? "bestie"].markingColor,
  };
}
export const colorOptions = {
  Body: { field: "bodyColor", colors: [["Peach", "#efc18e"], ["Vanilla", "#f4dfbe"], ["Caramel", "#b8855e"], ["Cocoa", "#795342"], ["Silver", "#a3a6ac"], ["Rust", "#cf8150"], ["Snow", "#eee9df"], ["Midnight", "#50525b"]] },
  Tops: { field: "topColor", colors: [["Rose", "#b97e74"], ["Ivory", "#e7dfcc"], ["Sage", "#8c9c83"], ["Chocolate", "#705349"], ["Lilac", "#a89bbd"], ["Sky", "#8caec2"], ["Cherry", "#9f4856"], ["Ink", "#393b4a"]] },
  Bottoms: { field: "trousersColor", colors: [["Charcoal", "#393634"], ["Sand", "#baaa8c"], ["Olive", "#646b52"], ["Navy", "#384657"], ["Denim", "#6a859f"], ["Cream", "#ddd4bf"], ["Plum", "#78586e"], ["Coffee", "#806553"]] },
  Shoes: { field: "footwearColor", colors: [["Cream", "#e8d3b8"], ["Rose", "#c39186"], ["White", "#eeeeea"], ["Coffee", "#806553"], ["Black", "#353638"], ["Sage", "#8c9c83"], ["Lilac", "#a89bbd"], ["Blue", "#6a859f"]] },
} as const;
export const outfitPresets = [
  { id: "rose", label: "Rose bomber", topColor: "#b97e74" },
  { id: "ivory", label: "Ivory bomber", topColor: "#e7dfcc" },
] as const;
export function outfitValue(value: CompanionAppearance, id: "rose" | "ivory"): CompanionAppearance {
  return { ...value, garmentStyle: "bomber", look: id, topColor: outfitPresets.find(item => item.id === id)!.topColor,
    trousersColor: appearanceDefaults.trousersColor, footwearColor: appearanceDefaults.footwearColor };
}
export function isOutfitSelected(value: CompanionAppearance, id: "rose" | "ivory") {
  const outfit = outfitValue(value, id);
  return (value.garmentStyle ?? "bomber") === "bomber" && value.look === id && value.topColor === outfit.topColor && value.trousersColor === outfit.trousersColor && value.footwearColor === outfit.footwearColor;
}
