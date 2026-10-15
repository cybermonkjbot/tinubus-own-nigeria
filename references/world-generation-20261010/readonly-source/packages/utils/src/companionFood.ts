import type { CompanionGameEconomyQuote, CompanionGameEconomyPrice } from "./companionGameEconomy";
import type { CompanionNeeds } from "./companionLife";
import type { CompanionMovingHawkerOffer } from "./companionMovingHawkers";
import { companionCookingProgress, type CompanionSkills } from "./companionSkills";

export const COMPANION_FOOD_CAPACITY = 6;
export const companionFoodMenu = Object.freeze([
  { offerKey: "suya", nutritionKey: "suya-beef-skewer", label: "Suya", amount: 12, stock: 48 },
  { offerKey: "akara", nutritionKey: "akara-agege-bread", label: "Akara and Agege bread", amount: 8, stock: 64 },
  { offerKey: "water", nutritionKey: "chilled-pure-water", label: "Sachet water", amount: 2, stock: 96 },
  { offerKey: "ready-meal", nutritionKey: null, label: "Rice meal", amount: 10, stock: 64 },
  { offerKey: "energy-drink", nutritionKey: null, label: "Energy drink", amount: 4, stock: 96 },
  { offerKey: "rice", nutritionKey: null, label: "Rice", amount: 2, stock: 200 },
  { offerKey: "beans", nutritionKey: null, label: "Beans", amount: 2, stock: 200 },
  { offerKey: "tomato", nutritionKey: null, label: "Tomato", amount: 1, stock: 200 },
  { offerKey: "oil", nutritionKey: null, label: "Cooking oil", amount: 1, stock: 200 },
] as const);
export type CompanionFoodKey = typeof companionFoodMenu[number]["offerKey"];
export const companionFoodCounters = Object.freeze([
  { placeId: "mama-put", serviceId: "mama-vendor", npcId: "mama-vendor-npc", offers: ["suya", "akara"] },
  { placeId: "town-square", serviceId: "mai-suya-food", npcId: "mai-suya", offers: ["suya"] },
  { placeId: "town-square", serviceId: "akara-woman-food", npcId: "akara-woman", offers: ["akara"] },
  { placeId: "skyline", serviceId: "restaurant-host", npcId: "restaurant-host-npc", offers: ["water", "ready-meal"] },
  { placeId: "nightclub", serviceId: "club-water", npcId: "club-water-npc", offers: ["water"] },
] as const);
export const companionFoodItem = (key: string) => companionFoodMenu.find(item => item.offerKey === key) ?? null;
export const companionFoodCounter = (placeId: string, serviceId: string) =>
  companionFoodCounters.find(counter => counter.placeId === placeId && counter.serviceId === serviceId) ?? null;
export const companionFoodServiceKey = (placeId: string, serviceId: string, key: CompanionFoodKey) =>
  `food-v1:${placeId}:${serviceId}:${key}`;
export const companionFoodPrice = (amount: number): CompanionGameEconomyPrice => ({
  unit: "game-value-unit", unitKey: "lagos-credit", amount, source: { kind: "banked_game_value", key: "haul" },
});
export function companionFoodNutrition(needs: CompanionNeeds, key: CompanionFoodKey): CompanionNeeds {
  if (key === "ready-meal") return { ...needs, hunger: Math.max(0, needs.hunger - 55),
    energy: Math.min(100, needs.energy + 12), enjoyment: Math.min(100, needs.enjoyment + 10) };
  if (key === "energy-drink") return { ...needs, energy: Math.min(100, needs.energy + 20) };
  const nutrition = key === "suya" ? { hunger: 35, energy: 15, enjoyment: 20 }
    : key === "akara" ? { hunger: 50, energy: 25, enjoyment: 15 }
    : key === "water" ? { hunger: 5, energy: 15, enjoyment: 8 } : null;
  if (!nutrition) throw new Error("This item must be cooked first.");
  return { ...needs, hunger: Math.max(0, needs.hunger - nutrition.hunger), energy: Math.min(100, needs.energy + nutrition.energy),
    enjoyment: Math.min(100, needs.enjoyment + nutrition.enjoyment) };
}
export const companionFoodEdible = (key: CompanionFoodKey) => !["rice", "beans", "tomato", "oil"].includes(key);
export const companionFoodStorage = (key: CompanionFoodKey) => ["ready-meal", "energy-drink", "tomato", "water"].includes(key) ? "fridge" as const : "pantry" as const;
export const COMPANION_HOME_FOOD_CAPACITY = 200;
export const companionFoodRecipes = Object.freeze([
  { key: "rice-and-tomato", label: "Tomato rice", portions: 3, gameSeconds: 140,
    cookingLevel: 1, cookingExperience: 10,
    ingredients: [{ key: "rice", quantity: 2 }, { key: "tomato", quantity: 1 }, { key: "oil", quantity: 1 }] },
  { key: "beans-pot", label: "Beans", portions: 3, gameSeconds: 160,
    cookingLevel: 1, cookingExperience: 10,
    ingredients: [{ key: "beans", quantity: 2 }, { key: "tomato", quantity: 1 }, { key: "oil", quantity: 1 }] },
  { key: "rice-and-beans", label: "Rice and beans", portions: 3, gameSeconds: 180,
    cookingLevel: 2, cookingExperience: 15,
    ingredients: [{ key: "rice", quantity: 1 }, { key: "beans", quantity: 1 }, { key: "tomato", quantity: 1 }, { key: "oil", quantity: 1 }] },
  { key: "jollof-rice", label: "Jollof rice", portions: 3, gameSeconds: 200,
    cookingLevel: 3, cookingExperience: 20,
    ingredients: [{ key: "rice", quantity: 2 }, { key: "tomato", quantity: 2 }, { key: "oil", quantity: 1 }] },
] as const);
export const companionFoodRecipe = (key: string) => companionFoodRecipes.find(recipe => recipe.key === key) ?? null;
export const companionFoodRecipeUnlocked = (recipe: { cookingLevel: number }, skills?: CompanionSkills) =>
  companionCookingProgress(skills).level >= recipe.cookingLevel;
export type CompanionKitchenScope = { roomId: string; characterId: string; callSessionId?: string; roomRevision?: number; invitationId?: string };
export type CompanionKitchenSnapshot = { roomId: string; owner: boolean;
  canCook: boolean; careRevision: number;
  cooking: { batchId: string; activityId: string; label: string; startedAt: number; endsAt: number; shared: boolean } | null;
  stock: { key: CompanionFoodKey; label: string; quantity: number; storage: "fridge" | "pantry" }[];
  batches: { id: string; label: string; remaining: number; served: number }[];
  recipes: { key: string; label: string; portions: number; gameSeconds: number; available: boolean;
    cookingLevel?: number; cookingExperience?: number; unlocked?: boolean }[];
};
export type CompanionKitchenCommand = { type: "serve"; batchId: string; portions: number }
  | { type: "cook"; recipeKey: string; batches: 1 | 2; expectedRevision: number }
  | { type: "stop-cooking"; batchId: string; activityId: string; expectedRevision: number }
  | { type: "plate-meal"; portions: number } | { type: "drink"; key: CompanionFoodKey }
  | { type: "eat-stock"; key: CompanionFoodKey } | { type: "eat"; batchId: string };
export type CompanionKitchenStoreSnapshot = { kitchen: CompanionKitchenSnapshot | null; scope?: CompanionKitchenScope | null;
  ready: boolean; busy: boolean; retryable: boolean; error: string | null };
export interface CompanionKitchenStore {
  getSnapshot(): CompanionKitchenStoreSnapshot;
  subscribe(listener: () => void): () => void;
  select(scope: CompanionKitchenScope | null): void;
  send(command: CompanionKitchenCommand, requestId: string): Promise<void>;
  retry(): Promise<void>;
  reload(): void;
  dispose(): void;
}

export type CompanionFoodScope = {
  gameRoomId: string; instanceId: string; lifeId: string; revision: number; serviceId: string;
  movingOfferId?: string;
};
export type CompanionFoodOrder = {
  id: string; quoteId: string | null; offerKey: CompanionFoodKey; label: string; placeId: string; serviceId: string;
  quantity: number; mode: "pickup" | "delivery"; status: "ready" | "carried" | "secured" | "stocked" | "consumed" | "lost"; revision: number; purchasedAt: number; readyAt: number;
  collectedAt: number | null; consumedAt: number | null;
};
export type CompanionFoodCatalog = {
  scope: CompanionFoodScope; eating: boolean; capacity: number; occupied: number; serverNow: number;
  offers: { offerKey: CompanionFoodKey; label: string; price: CompanionGameEconomyPrice; stock: number }[];
  orders: CompanionFoodOrder[]; carried: CompanionFoodOrder[];
  movingOffer?: CompanionMovingHawkerOffer | null;
};
export type CompanionFoodAction =
  | { type: "quote"; offerKey: CompanionFoodKey; quantity: 1 | 3 | 6 }
  | { type: "confirm" | "cancel"; quoteId: string }
  | { type: "collect" | "eat"; orderId: string };
export type CompanionFoodResult = { quote: CompanionGameEconomyQuote | null; order: CompanionFoodOrder | null;
  movingOffer?: CompanionMovingHawkerOffer | null };
export type CompanionFoodSnapshot = {
  catalog: CompanionFoodCatalog | null; quote: CompanionGameEconomyQuote | null; result: CompanionFoodResult | null;
  ready: boolean; busy: boolean; retryable: boolean; error: string | null;
  movingOffer?: CompanionMovingHawkerOffer | null;
};
export const emptyCompanionFoodSnapshot: CompanionFoodSnapshot = {
  catalog: null, quote: null, result: null, ready: false, busy: false, retryable: false, error: null,
};
export interface CompanionFoodStore {
  getSnapshot(): CompanionFoodSnapshot;
  subscribe(listener: () => void): () => void;
  select(scope: CompanionFoodScope | null): void;
  send(action: CompanionFoodAction, requestId: string): Promise<CompanionFoodResult | null>;
  retry(): Promise<CompanionFoodResult | null>;
  reload(): void;
  dispose(): void;
}
