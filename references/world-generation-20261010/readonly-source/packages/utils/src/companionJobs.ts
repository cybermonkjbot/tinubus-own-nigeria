import type { CompanionHaulEntry, CompanionHaulReward } from "./companionHaul";

/** Storage/transaction limits, not approved prices or game-time tuning. */
export const companionJobsLimits = {
  offersPerContact: 16,
  offerWindowMs: 30 * 24 * 60 * 60 * 1000,
  offerKeyLength: 64,
  labelLength: 100,
  requestKeyLength: 100,
} as const;

export type CompanionJobContact = { placeId: string; npcId: string; serviceId: string };
export const companionTrustedCourierTarget = 3 as const;
export type CompanionCourierGoal = {
  selected: boolean;
  resident: { actorIdentity: string; name: string } | null;
  deliveries: number;
  target: typeof companionTrustedCourierTarget;
  completedAt: number | null;
  inCurrentWorld: boolean;
};
export type CompanionJobGameValue = Extract<CompanionHaulReward, { unit: "game-value-unit" }>;
export type CompanionJobReward =
  | { status: "unavailable"; reason: "approval_required" }
  | { status: "approved"; approvalKey: string; value: CompanionJobGameValue };
export type CompanionJobTerms = {
  /** Trusted issuance identity. Never derive a new key from a visit, retry or departure. */
  offerKey: string;
  worldId: string;
  label: string;
  employer: CompanionJobContact;
  destination: CompanionJobContact;
  /** Job cargo only, not an inventory item or a saleable/drop-eligible reward. */
  parcel: { key: string; label: string };
  reward: CompanionJobReward;
  timeBasis: "server-ms";
  /** Acceptance window only. Accepted parcels have no time-based delivery deadline. */
  opensAt: number;
  closesAt: number;
  requiresTrustedCourier?: true;
};
export type CompanionJobStatus = "accepted" | "carrying" | "delivered" | "cancelled" | "failed";
export type CompanionJobFailure = "death" | "outing-ended" | "life-ended";

/** Account-private concurrency tokens. Never publish life IDs to peers or NPC profiles. */
export type CompanionJobObservation = {
  instanceId: string;
  worldRevision: number;
  outingId: string;
  lifeId: string;
};
export type CompanionJobCommand = {
  requestId: string;
  observed: CompanionJobObservation;
} & (
  | { type: "accept"; offerId: string }
  | { type: "collect" | "deliver" | "cancel"; jobId: string; jobRevision: number }
);
export type CompanionJobOfferView = {
  id: string;
  terms: CompanionJobTerms;
  available: boolean;
  availabilityRevision: number;
};
export type CompanionJobView = {
  id: string;
  offerId: string;
  terms: CompanionJobTerms;
  outingId: string;
  lifeId: string;
  revision: number;
  status: CompanionJobStatus;
  failure: CompanionJobFailure | null;
  acceptedAt: number;
  collectedAt: number | null;
  endedAt: number | null;
  /** Read from the haul ledger each time, including later banking or forfeiture. */
  reward: CompanionHaulEntry | null;
};
export type CompanionJobBlockedReason =
  | "stale_context" | "not_found" | "wrong_world" | "wrong_contact"
  | "offer_unavailable" | "reward_unavailable" | "active_job" | "already_attempted"
  | "out_of_order" | "job_closed" | "outing_closed" | "haul_full" | "source_used" | "trust_required";
export type CompanionJobResult = {
  outcome: "applied" | "replayed" | "blocked";
  reason: CompanionJobBlockedReason | null;
  job: CompanionJobView | null;
};

/** Host intents contain identifiers only. The account store captures every concurrency token. */
export type CompanionJobsAction =
  | { type: "accept"; offerId: string }
  | { type: "collect" | "deliver" | "cancel"; jobId: string };
/** Account-private. Never pass this snapshot to scene renderers or peers. */
export type CompanionJobsSnapshot = {
  goal: CompanionCourierGoal | null;
  goalReady: boolean;
  goalBusy: boolean;
  goalError: string | null;
  job: CompanionJobView | null;
  contact: CompanionJobContact | null;
  offers: CompanionJobOfferView[];
  ready: boolean;
  offersReady: boolean;
  busy: boolean;
  retryable: boolean;
  result: Pick<CompanionJobResult, "outcome" | "reason"> | null;
  error: string | null;
};
export const emptyCompanionJobsSnapshot: CompanionJobsSnapshot = {
  goal: null, goalReady: false, goalBusy: false, goalError: null,
  job: null, contact: null, offers: [], ready: false, offersReady: false,
  busy: false, retryable: false, result: null, error: null,
};
export interface CompanionJobsStore {
  getSnapshot(): CompanionJobsSnapshot;
  subscribe(listener: () => void): () => void;
  /** Follow the existing host's admitted room selection. Does not select, enter or leave the world. */
  select(gameRoomId: string | null): void;
  /** Resolve this ID against the current world's NPC services. Travel clears the selection. */
  selectContact(serviceId: string | null): void;
  send(action: CompanionJobsAction): Promise<CompanionJobResult | null>;
  setGoal(selected: boolean): Promise<void>;
  /** Repeat the retained command and service ID unchanged, including its original request ID. */
  retry(): Promise<CompanionJobResult | null>;
  /** Refresh reads only. Never retries a command or discards an ambiguous request. */
  reload(): void;
  /** The account host must dispose this store before replacing its account/session stores. */
  dispose(): void;
}

export function companionJobActive(status: CompanionJobStatus): boolean {
  return status === "accepted" || status === "carrying";
}

/** Travel and reconnect never change the parcel stage. */
export function companionJobNextStatus(
  status: CompanionJobStatus,
  action: "collect" | "deliver" | "cancel",
): CompanionJobStatus | null {
  if (action === "cancel") return companionJobActive(status) ? "cancelled" : null;
  if (action === "collect" && status === "accepted") return "carrying";
  if (action === "deliver" && status === "carrying") return "delivered";
  return null;
}

/** One beneficiary claim per trusted issuance, across all visits, outings and lives. */
export function companionJobCompletionSourceKey(offerKey: string, beneficiaryId: string): string {
  if (!/^[a-zA-Z0-9_.:-]+$/.test(offerKey) || offerKey.length > companionJobsLimits.offerKeyLength
    || !beneficiaryId || beneficiaryId.length > 64) throw new Error("Invalid courier completion identity.");
  const key = JSON.stringify(["courier-v1", offerKey, beneficiaryId]);
  if (key.length > 160) throw new Error("The courier completion identity is too long.");
  return key;
}
