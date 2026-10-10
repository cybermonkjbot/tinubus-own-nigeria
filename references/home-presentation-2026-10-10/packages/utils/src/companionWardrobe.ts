import type { CompanionAppearance } from "./companion";

export const garmentStyles = ["bomber", "t-shirt", "shirt", "a-line-dress", "pleated-dress", "wrap-dress", "evening-gown"] as const;
export type GarmentStyle = typeof garmentStyles[number];
export const fabricTextures = ["cotton", "linen", "knit", "denim", "satin", "velvet"] as const;
export type FabricTexture = typeof fabricTextures[number];
export const clothingPatterns = ["plain", "stripes", "gingham", "dots"] as const;
export type ClothingPattern = typeof clothingPatterns[number];
export const paidGarmentStyles = ["pleated-dress", "wrap-dress", "evening-gown"] as const;
export type PaidGarmentStyle = typeof paidGarmentStyles[number];

export const wardrobeStyles: readonly { id: GarmentStyle; name: string; category: "Separates" | "Dresses"; paid: boolean; texture: FabricTexture }[] = [
  { id: "bomber", name: "Bomber", category: "Separates", paid: false, texture: "cotton" },
  { id: "t-shirt", name: "T-shirt", category: "Separates", paid: false, texture: "cotton" },
  { id: "shirt", name: "Collared shirt", category: "Separates", paid: false, texture: "linen" },
  { id: "a-line-dress", name: "A-line dress", category: "Dresses", paid: false, texture: "cotton" },
  { id: "pleated-dress", name: "Pleated dress", category: "Dresses", paid: true, texture: "satin" },
  { id: "wrap-dress", name: "Wrap dress", category: "Dresses", paid: true, texture: "linen" },
  { id: "evening-gown", name: "Evening gown", category: "Dresses", paid: true, texture: "velvet" },
];
export const fabricChoices = fabricTextures.map(id => ({ id, name: id[0].toUpperCase() + id.slice(1) }));
export const patternChoices = clothingPatterns.map(id => ({ id, name: id[0].toUpperCase() + id.slice(1) }));
export const isDress = (style?: GarmentStyle) => style !== undefined && (style.endsWith("-dress") || style === "evening-gown");
export const isPaidGarment = (id: string | undefined): id is PaidGarmentStyle => paidGarmentStyles.some(style => style === id);
export const wardrobeLocked = (appearance: Pick<CompanionAppearance, "garmentStyle">, owned: readonly string[] = []) =>
  isPaidGarment(appearance.garmentStyle) && !owned.includes(appearance.garmentStyle);

export function garmentValue(value: CompanionAppearance, style: GarmentStyle): CompanionAppearance {
  return { ...value, garmentStyle: style, fabricTexture: wardrobeStyles.find(item => item.id === style)!.texture };
}

/** Shared functional silhouettes for native and web wardrobe pickers. */
export const garmentPaths: Record<GarmentStyle, readonly string[]> = {
  bomber: ["M11 5 5 9 3 23 8 24 10 15 10 27 22 27 22 15 24 24 29 23 27 9 21 5", "M11 5 16 10 21 5M16 10V27M11 23H14M18 23H21"],
  "t-shirt": ["M11 5 4 9 2 16 8 18 10 14 10 27 22 27 22 14 24 18 30 16 28 9 21 5", "M11 5C12 11 20 11 21 5"],
  shirt: ["M11 5 5 9 3 25 8 26 10 15 10 27 22 27 22 15 24 26 29 25 27 9 21 5", "M11 5 16 9 13 13 10 8M21 5 16 9 19 13 22 8M16 10V27"],
  "a-line-dress": ["M12 4Q16 8 20 4L22 10 20 15 27 28Q16 31 5 28L12 15 10 10Z", "M12 15H20"],
  "pleated-dress": ["M12 4Q16 8 20 4L22 10 20 15 27 28Q16 31 5 28L12 15 10 10Z", "M12 15H20M13 17 10 29M16 17V30M19 17 22 29"],
  "wrap-dress": ["M12 4 16 9 20 4 22 10 20 15 27 28Q16 31 5 28L12 15 10 10Z", "M20 5 12 15 20 16 11 29M20 15 24 13 25 17 20 15 23 21"],
  "evening-gown": ["M12 3Q16 8 20 3L21 10 19 16Q20 23 27 29Q16 32 5 29Q12 23 13 16L11 10Z", "M13 16H19"],
};
