import { companionStreetWeapons, type CompanionStreetWeaponId } from "./companionCombatEquipment";

/** Quotes retain service terms. Simulated vendor fulfillment is a separate atomic command. */
export const COMPANION_GAME_ECONOMY_MAX_AMOUNT = 1_000_000_000;
export const COMPANION_GAME_ECONOMY_MAX_SERVICES = 32;
export const COMPANION_GAME_ECONOMY_MAX_ACTIVE_QUOTES = 8;
export const COMPANION_GAME_ECONOMY_MAX_QUOTE_TTL_MS = 15 * 60_000;
export const COMPANION_GAME_ECONOMY_MAX_OFFER_LIFETIME_MS = 30 * 86_400_000;

export const COMPANION_CREDITS_PER_NAIRA = 100;
export const COMPANION_CREDITS_PER_KOBO = 1;

export function formatCreditsWithNaira(credits: number): string {
  const safeCredits = Math.max(0, Math.floor(credits));
  const naira = safeCredits / COMPANION_CREDITS_PER_NAIRA;
  return `${safeCredits.toLocaleString()} Lagos Credits (₦${naira.toLocaleString(undefined, { minimumFractionDigits: 2, maximumFractionDigits: 2 })})`;
}

export function creditsToKobo(credits: number): number {
  return Math.max(0, Math.floor(credits * COMPANION_CREDITS_PER_KOBO));
}

export function koboToCredits(kobo: number): number {
  return Math.max(0, Math.floor(kobo / COMPANION_CREDITS_PER_KOBO));
}

/** Existing main-earnings source names, not another balance account. */
export type CompanionGameEconomyMainSource =
  | "handyman" | "partner" | "declutter_customer" | "rewards_member"
  | "referral_partner" | "content_creator" | "influence_creator";
export type CompanionGameEconomyPrice =
  | { unit: "real-currency-minor"; currency: "NGN"; amount: number;
      source: { kind: "main_balance"; key: CompanionGameEconomyMainSource } }
  | { unit: "game-value-unit"; unitKey: string; amount: number;
      source: { kind: "banked_game_value"; key: string } };
export type CompanionGameEconomyPolicy = {
  liveFinancialActivation: "disabled" | "enabled";
  conversionStatus: "pending_approval" | "not_applicable" | "active";
  fundingStatus: "pending_approval" | "approved";
  paymentSourceStatus: "pending_approval" | "approved";
  withdrawalStatus: "unavailable" | "available";
  exchangeRate: { creditsPerNaira: number; creditsPerKobo: number } | null;
};
export function companionGameEconomyPolicy(price: CompanionGameEconomyPrice): CompanionGameEconomyPolicy {
  return {
    liveFinancialActivation: "disabled", conversionStatus: price.unit === "game-value-unit" ? "pending_approval" : "not_applicable",
    fundingStatus: "pending_approval", paymentSourceStatus: "pending_approval", withdrawalStatus: "unavailable",
    exchangeRate: null,
  };
}
export type CompanionGameEconomyService = {
  offerId: string; serviceKey: string; revision: number; npcKey: string; placeKey: string; label: string;
  price: CompanionGameEconomyPrice; beneficiary: "self"; offerStartsAt: number; offerExpiresAt: number; quoteTtlMs: number;
  availabilityRevision: number; available: boolean; policy: CompanionGameEconomyPolicy;
};
export type CompanionGameEconomyQuoteStatus = "quoted" | "approval_required" | "cancelled" | "fulfilled";
export type CompanionGameEconomyQuote = {
  id: string; service: CompanionGameEconomyService; issuedAt: number; expiresAt: number;
  termsFingerprint: string; status: CompanionGameEconomyQuoteStatus; revision: number;
  confirmedAt: number | null; cancelledAt: number | null;
};
export type CompanionGameEconomySelector = { offerId: string } | { serviceKey: string };
export type CompanionGameEconomyCatalog = { services: CompanionGameEconomyService[]; liveFinancialActivation: "disabled" };
export type CompanionGameEconomySnapshot = CompanionGameEconomyCatalog & {
  quote: CompanionGameEconomyQuote | null; ready: boolean; quoteReady: boolean; busy: boolean; error: string | null;
  vendor?: CompanionWeaponVendorSnapshot;
};
export type CompanionWeaponVendorScope = { gameRoomId: string; instanceId: string; lifeId: string; revision: number; equipmentRevision: number };
export type CompanionWeaponVendorSnapshot = { services: { serviceKey: string; label: string; price: CompanionGameEconomyPrice; stock: number; owned: boolean }[];
  ready: boolean; busy: boolean; error: string | null };
export type CompanionWeaponVendorItem = { serviceKey: string; label: string; amount: number; stock: number;
  weaponId: CompanionStreetWeaponId; kind: "weapon" | "ammo"; rounds: number };
export const companionWeaponVendorRules = { unitKey: "lagos-credit", placeKey: "mama-put", npcKey: "delivery-contact-npc", serviceId: "delivery-contact",
  items: companionStreetWeapons.flatMap<CompanionWeaponVendorItem>(weapon => [
    { serviceKey: weapon.serviceKey, label: weapon.label, amount: weapon.price, stock: weapon.stock, weaponId: weapon.id, kind: "weapon", rounds: 0 },
    ...(weapon.kind === "firearm" ? [{ serviceKey: `street-ammo:${weapon.id}`, label: `${weapon.ammoPack} ${weapon.id === "shotgun" ? "shotgun shells" : `${weapon.label.toLowerCase()} rounds`}`,
      amount: weapon.ammoPrice, stock: 100, weaponId: weapon.id, kind: "ammo" as const, rounds: weapon.ammoPack }] : []),
  ]),
  pistolAmmo: companionStreetWeapons[1].magazine, pistolReserve: companionStreetWeapons[1].reserveAmmo, ammoPack: companionStreetWeapons[1].ammoPack } as const;
export const emptyCompanionGameEconomySnapshot: CompanionGameEconomySnapshot = {
  services: [], quote: null, liveFinancialActivation: "disabled", ready: false, quoteReady: true, busy: false, error: null,
};
export interface CompanionGameEconomyStore {
  getSnapshot(): CompanionGameEconomySnapshot;
  subscribe(listener: () => void): () => void;
  select(placeKey: string, npcKey: string): void;
  recoverQuote(quoteId: string | null): void;
  reload(): void;
  requestQuote(selector: CompanionGameEconomySelector, requestId: string): Promise<CompanionGameEconomyQuote | null>;
  confirmQuote(quoteId: string, requestId: string): Promise<CompanionGameEconomyQuote | null>;
  cancelQuote(quoteId: string, requestId: string): Promise<CompanionGameEconomyQuote | null>;
  selectVendor(scope: CompanionWeaponVendorScope | null): void;
  requestVendorQuote(serviceKey: string, requestId: string): Promise<CompanionGameEconomyQuote | null>;
  confirmVendorQuote(quoteId: string, requestId: string): Promise<CompanionGameEconomyQuote | null>;
  dispose(): void;
}

export const validCompanionGameEconomyKey = (value: unknown): value is string =>
  typeof value === "string" && /^[A-Za-z0-9][A-Za-z0-9._:-]{0,159}$/.test(value);
export const validCompanionGameEconomyRequestId = (value: unknown): value is string =>
  typeof value === "string" && /^[A-Za-z0-9][A-Za-z0-9._:-]{15,127}$/.test(value);
const record = (value: unknown): value is Record<string, unknown> => !!value && typeof value === "object" && !Array.isArray(value);
const integer = (value: unknown, minimum = 0, maximum = Number.MAX_SAFE_INTEGER): value is number =>
  typeof value === "number" && Number.isSafeInteger(value) && value >= minimum && value <= maximum;
const mainSources: readonly string[] = ["handyman", "partner", "declutter_customer", "rewards_member", "referral_partner", "content_creator", "influence_creator"];
export function validCompanionGameEconomyPrice(value: unknown): value is CompanionGameEconomyPrice {
  if (!record(value) || !integer(value.amount, 1, COMPANION_GAME_ECONOMY_MAX_AMOUNT) || !record(value.source)) return false;
  return value.unit === "real-currency-minor" ? value.currency === "NGN" && value.source.kind === "main_balance"
    && typeof value.source.key === "string" && mainSources.includes(value.source.key)
    : value.unit === "game-value-unit" && validCompanionGameEconomyKey(value.unitKey)
      && value.source.kind === "banked_game_value" && validCompanionGameEconomyKey(value.source.key);
}
export function policiesMatch(a: CompanionGameEconomyPolicy, b: CompanionGameEconomyPolicy): boolean {
  if (!a || !b) return false;
  return a.liveFinancialActivation === b.liveFinancialActivation
    && a.conversionStatus === b.conversionStatus
    && a.fundingStatus === b.fundingStatus
    && a.paymentSourceStatus === b.paymentSourceStatus
    && a.withdrawalStatus === b.withdrawalStatus
    && (
      (a.exchangeRate === null && b.exchangeRate === null) ||
      (a.exchangeRate !== null && b.exchangeRate !== null
        && a.exchangeRate.creditsPerNaira === b.exchangeRate.creditsPerNaira
        && a.exchangeRate.creditsPerKobo === b.exchangeRate.creditsPerKobo)
    );
}

/** Read compatibility only. Old immutable offers advertised reference-value conversion.
 * Accept their exact disabled-finance shape for provenance, never as financial authority. */
export function legacyCompanionGameEconomyPolicy(price: CompanionGameEconomyPrice, policy: CompanionGameEconomyPolicy): boolean {
  return policiesMatch(policy, { liveFinancialActivation: "disabled",
    conversionStatus: price.unit === "game-value-unit" ? "active" : "not_applicable", fundingStatus: "pending_approval",
    paymentSourceStatus: "pending_approval", withdrawalStatus: "available",
    exchangeRate: { creditsPerNaira: COMPANION_CREDITS_PER_NAIRA, creditsPerKobo: COMPANION_CREDITS_PER_KOBO } });
}

export function validCompanionGameEconomyService(value: unknown): value is CompanionGameEconomyService {
  if (!record(value) || !validCompanionGameEconomyKey(value.offerId) || !validCompanionGameEconomyKey(value.serviceKey)
    || !validCompanionGameEconomyKey(value.npcKey) || !validCompanionGameEconomyKey(value.placeKey)
    || !integer(value.revision, 1) || !integer(value.availabilityRevision, 1)
    || typeof value.label !== "string" || !value.label.trim() || value.label.length > 80
    || !validCompanionGameEconomyPrice(value.price) || value.beneficiary !== "self"
    || !integer(value.offerStartsAt, 1) || !integer(value.offerExpiresAt, value.offerStartsAt + 1)
    || value.offerExpiresAt - value.offerStartsAt > COMPANION_GAME_ECONOMY_MAX_OFFER_LIFETIME_MS
    || !integer(value.quoteTtlMs, 1000, COMPANION_GAME_ECONOMY_MAX_QUOTE_TTL_MS)
    || typeof value.available !== "boolean" || !record(value.policy)) return false;
  const expected = companionGameEconomyPolicy(value.price);
  return policiesMatch(value.policy as CompanionGameEconomyPolicy, expected);
}
export function validCompanionGameEconomyQuote(value: unknown): value is CompanionGameEconomyQuote {
  if (!record(value) || !validCompanionGameEconomyKey(value.id) || !validCompanionGameEconomyService(value.service)
    || !integer(value.issuedAt, value.service.offerStartsAt) || !integer(value.expiresAt, 1) || value.expiresAt <= value.issuedAt
    || value.expiresAt > value.service.offerExpiresAt || value.expiresAt - value.issuedAt > value.service.quoteTtlMs
    || typeof value.termsFingerprint !== "string" || !value.termsFingerprint || value.termsFingerprint.length > 4000
    || !integer(value.revision, 0, 2)
    || value.confirmedAt !== null && (!integer(value.confirmedAt, value.issuedAt) || value.confirmedAt >= value.expiresAt)
    || value.cancelledAt !== null && !integer(value.cancelledAt, value.confirmedAt === null ? value.issuedAt : value.confirmedAt)) return false;
  if (value.status === "quoted") return value.revision === 0 && value.confirmedAt === null && value.cancelledAt === null;
  if (value.status === "approval_required") return value.revision === 1 && value.confirmedAt !== null && value.cancelledAt === null;
  if (value.status === "fulfilled") return value.revision === 1 && value.confirmedAt !== null && value.cancelledAt === null;
  return value.status === "cancelled" && value.revision === (value.confirmedAt === null ? 1 : 2) && value.cancelledAt !== null;
}

/** Evaluate on a render or user action. The store schedules no expiry polling. */
export function companionGameEconomyQuoteAvailability(quote: CompanionGameEconomyQuote, now: number,
  current?: CompanionGameEconomyService): CompanionGameEconomyQuoteStatus | "expired" | "offer_changed" {
  if (quote.status === "fulfilled") return "fulfilled";
  if (quote.status === "cancelled") return "cancelled";
  if (!Number.isFinite(now) || now >= quote.expiresAt) return "expired";
  if (current && (!current.available || current.offerId !== quote.service.offerId
    || current.availabilityRevision !== quote.service.availabilityRevision || current.revision !== quote.service.revision
    || companionGameEconomyPriceFingerprint(current.price) !== companionGameEconomyPriceFingerprint(quote.service.price))) return "offer_changed";
  return quote.status;
}

export const companionGameEconomyPriceFingerprint = (price: CompanionGameEconomyPrice): string => JSON.stringify(
  price.unit === "real-currency-minor" ? [price.unit, price.currency, price.amount, price.source.kind, price.source.key]
    : [price.unit, price.unitKey, price.amount, price.source.kind, price.source.key],
);
