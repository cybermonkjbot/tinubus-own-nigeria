import type { CompanionGameEconomyQuote } from "./companionGameEconomy";
import type { CompanionTransitKind } from "./companionCityTransit";

/** City-seat fares decided October 7, 2026. Operator fuel is included. */
export const companionTransitFares = Object.freeze({
  version: "lagos-city-seat-fares-v1" as const,
  unitKey: "lagos-credit" as const,
  sourceKey: "haul" as const,
  amounts: Object.freeze({ brt: 5, danfo: 10, keke: 12 }),
  quoteTtlMs: 30_000,
  fuel: "operator-included" as const,
});

export type CompanionTransitFare = {
  quote: CompanionGameEconomyQuote;
  vehicleId: string;
  kind: CompanionTransitKind;
  instanceId: string;
  visitRevision: number;
  lifeId: string;
  boardingEndsAt: number;
};

/** Server quotes must match the current fare and its banked-credit source. */
export function currentCompanionTransitFare(fare: CompanionTransitFare): boolean {
  const price = fare.quote.service.price;
  return price.unit === "game-value-unit" && price.unitKey === companionTransitFares.unitKey
    && price.amount === companionTransitFares.amounts[fare.kind]
    && price.source.kind === "banked_game_value" && price.source.key === companionTransitFares.sourceKey;
}
