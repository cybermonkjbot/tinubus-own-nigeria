import { planRoomRoute, routeSegmentClear, type ScenePoint } from "./companionNavigation";
import type { CompanionPlaceLayout, CompanionPlaceService } from "./companionPlaces";

export const companionPlaceInteractionLimits = { arrivalMs: 10_000, arrivalRadius: .45, routePoints: 32, receipts: 64, choices: 24, beats: 64 } as const;
export type CompanionPlaceCommand =
  | { type: "food-browse" }
  | { type: "food-order"; offerId: string; quoteId: string; confirmationId: string }
  | { type: "food-collect"; orderId: string }
  | { type: "food-eat"; itemId: string }
  | { type: "driver-board"; boardingId: string }
  | { type: "medic-state" }
  | { type: "medic-recover"; recoveryId: string }
  | { type: "police-state" }
  | { type: "police-release"; detentionId: string; optionId: string }
  | { type: "mission-browse" }
  | { type: "mission-accept"; missionId: string }
  | { type: "mission-deliver"; missionId: string; itemId: string }
  | { type: "dance-start" }
  | { type: "dance-hit"; roundId: string; beatIndex: number; lane: number }
  | { type: "dance-finish"; roundId: string }
  | { type: "gather-join"; gatheringId: string }
  | { type: "gather-ready"; gatheringId: string }
  | { type: "capture"; gatheringId: string; consentId: string; format: "photo" | "clip" }
  | { type: "football-enter"; mode: "solo" | "teams"; teamId: string | null }
  | { type: "arena-enter"; matchId: string; rulesRevision: string; consentId: string }
  | { type: "home-open"; roomSlotId: string; visitGrantId: string }
  | { type: "pos-browse" }
  | { type: "pos-withdraw"; amount: number; confirmationId: string }
  | { type: "pos-deposit"; amount: number; confirmationId: string }
  | { type: "hawker-browse" }
  | { type: "hawker-buy"; offerId: string; quoteId: string; confirmationId: string }
  | { type: "tout-state" }
  | { type: "tout-settle"; settlementId: string; optionId: "pay" | "charm" | "flee" };
export type CompanionPlaceOwner = "food" | "travel" | "recovery" | "policing" | "missions" | "activities" | "content" | "football" | "combat" | "homes" | "pos" | "hawker" | "tout";
export type CompanionPlaceMembership = {
  id: string; userId: string; characterId: string; placeId: string; instanceId: string;
  layoutRevision: string; expiresAt: number; phase: "travelling" | "arrived" | "detained" | "recovering";
};
/** A current, owner-supplied view. IDs are references to existing state, not new stores. */
export type CompanionPlaceServiceState = {
  serviceId: string; revision: number; expiresAt: number; available: boolean; reason: string | null;
  membershipId: string; userId: string; instanceId: string; layoutRevision: string;
  food?: {
    offers: readonly { id: string; label: string; available: boolean; quoteId: string | null; quoteExpiresAt: number }[];
    orders: readonly { id: string; label: string; ready: boolean }[];
    carried: readonly { id: string; label: string; edible: boolean }[];
  };
  boarding?: { id: string; label: string; available: boolean };
  recovery?: { id: string; eligible: boolean };
  detention?: { id: string; options: readonly { id: string; label: string; eligible: boolean; paymentRequired: boolean }[] };
  missions?: readonly { id: string; offerId: string; label: string; status: "offered" | "active"; deliveryItemIds: readonly string[] }[];
  dance?: { round: CompanionDanceRound | null; inputs: readonly CompanionDanceInput[] };
  gathering?: { id: string; offerId: string; joined: boolean; ready: boolean; canCapture: boolean; consentId: string | null; formats: readonly ("photo" | "clip")[] };
  football?: { solo: boolean; teamIds: readonly string[] };
  arena?: { matchId: string; rulesRevision: string; eligible: boolean };
  home?: { roomSlotId: string; visitGrantId: string; expiresAt: number };
  pos?: { availableCash: number; fee: number; bankedBalance: number };
  hawker?: { offers: readonly { id: string; label: string; available: boolean; price: number; quoteId: string }[] };
  tout?: { encounterId: string; amountDemanded: number; options: readonly { id: "pay" | "charm" | "flee"; label: string; eligible: boolean }[] };
};
export type CompanionPlaceInteractionContext = {
  layout: CompanionPlaceLayout; sharedNow: number;
  /** From the authenticated account owner, never the renderer's claimed principal. */
  principal: { userId: string; characterId: string } | null;
  membership: CompanionPlaceMembership | null; position: ScenePoint;
  inputActive: boolean; services: readonly CompanionPlaceServiceState[];
};
export type CompanionPlaceArrival = {
  actionId: string; membershipId: string; userId: string; characterId: string;
  instanceId: string; layoutRevision: string; movementSequence: number;
  position: ScenePoint; arrivedAt: number; expiresAt: number;
};
export type CompanionPlaceIntent = {
  actionId: string; owner: CompanionPlaceOwner; serviceId: string; npcId: string | null;
  membership: CompanionPlaceMembership; serviceRevision: number; command: CompanionPlaceCommand;
  movementSequence: number; approach: ScenePoint; requestedAt: number;
};
export type CompanionPlaceAcknowledgement = {
  actionId: string; membershipId: string; userId: string; instanceId: string; layoutRevision: string;
  status: "acknowledged" | "pending" | "rejected"; reason?: string;
};
export type CompanionPlaceInteractionResult = {
  intent: CompanionPlaceIntent; route: readonly ScenePoint[];
  status: "walking" | "sending" | "retryable" | "acknowledged" | "rejected" | "cancelled";
  error: string | null;
};
export type CompanionPlaceChoice = {
  label: string; command: CompanionPlaceCommand; disabled?: string;
  confirmation?: "payment" | "arena-entry" | "capture";
};

const id = (value: unknown): value is string => typeof value === "string" && value.length > 0 && value.length <= 240;
const pointValid = (point: ScenePoint) => point.length === 3 && point.every(Number.isFinite);
const distance = (a: ScenePoint, b: ScenePoint) => Math.hypot(a[0] - b[0], a[2] - b[2]);
const record = (value: unknown): value is Record<string, unknown> => !!value && typeof value === "object" && !Array.isArray(value);
function ownerFor(command: CompanionPlaceCommand): CompanionPlaceOwner {
  if (command.type.startsWith("food-")) return "food";
  if (command.type.startsWith("mission-")) return "missions";
  if (command.type.startsWith("police-")) return "policing";
  if (command.type.startsWith("medic-")) return "recovery";
  if (command.type.startsWith("pos-")) return "pos";
  if (command.type.startsWith("hawker-")) return "hawker";
  if (command.type.startsWith("tout-")) return "tout";
  if (command.type.startsWith("dance-") || command.type.startsWith("gather-")) return "activities";
  switch (command.type) {
    case "driver-board": return "travel";
    case "capture": return "content";
    case "football-enter": return "football";
    case "arena-enter": return "combat";
    default: return "homes";
  }
}

/** Parse bounded requests only. Owning modules validate terms and apply effects. */
export function readCompanionPlaceCommand(value: unknown): CompanionPlaceCommand | null {
  if (!record(value)) return null;
  switch (value.type) {
    case "food-browse": case "medic-state": case "police-state": case "mission-browse": case "dance-start": return { type: value.type };
    case "food-order": return id(value.offerId) && id(value.quoteId) && id(value.confirmationId) ? { type: value.type, offerId: value.offerId, quoteId: value.quoteId, confirmationId: value.confirmationId } : null;
    case "food-collect": return id(value.orderId) ? { type: value.type, orderId: value.orderId } : null;
    case "food-eat": return id(value.itemId) ? { type: value.type, itemId: value.itemId } : null;
    case "driver-board": return id(value.boardingId) ? { type: value.type, boardingId: value.boardingId } : null;
    case "medic-recover": return id(value.recoveryId) ? { type: value.type, recoveryId: value.recoveryId } : null;
    case "police-release": return id(value.detentionId) && id(value.optionId) ? { type: value.type, detentionId: value.detentionId, optionId: value.optionId } : null;
    case "mission-accept": return id(value.missionId) ? { type: value.type, missionId: value.missionId } : null;
    case "mission-deliver": return id(value.missionId) && id(value.itemId) ? { type: value.type, missionId: value.missionId, itemId: value.itemId } : null;
    case "dance-hit": return id(value.roundId) && Number.isSafeInteger(value.beatIndex) && (value.beatIndex as number) >= 0 && (value.beatIndex as number) < companionPlaceInteractionLimits.beats
      && Number.isSafeInteger(value.lane) && (value.lane as number) >= 0 && (value.lane as number) < 4 ? { type: value.type, roundId: value.roundId, beatIndex: value.beatIndex as number, lane: value.lane as number } : null;
    case "dance-finish": return id(value.roundId) ? { type: value.type, roundId: value.roundId } : null;
    case "gather-join": case "gather-ready": return id(value.gatheringId) ? { type: value.type, gatheringId: value.gatheringId } : null;
    case "capture": return id(value.gatheringId) && id(value.consentId) && (value.format === "photo" || value.format === "clip") ? { type: value.type, gatheringId: value.gatheringId, consentId: value.consentId, format: value.format } : null;
    case "football-enter": return (value.mode === "solo" && value.teamId === null || value.mode === "teams" && id(value.teamId)) ? { type: value.type, mode: value.mode, teamId: value.teamId as string | null } : null;
    case "arena-enter": return id(value.matchId) && id(value.rulesRevision) && id(value.consentId) ? { type: value.type, matchId: value.matchId, rulesRevision: value.rulesRevision, consentId: value.consentId } : null;
    case "home-open": return id(value.roomSlotId) && id(value.visitGrantId) ? { type: value.type, roomSlotId: value.roomSlotId, visitGrantId: value.visitGrantId } : null;
    case "pos-browse": case "hawker-browse": case "tout-state": return { type: value.type };
    case "pos-withdraw": case "pos-deposit": return Number.isSafeInteger(value.amount) && (value.amount as number) > 0 && id(value.confirmationId) ? { type: value.type, amount: value.amount as number, confirmationId: value.confirmationId } : null;
    case "hawker-buy": return id(value.offerId) && id(value.quoteId) && id(value.confirmationId) ? { type: value.type, offerId: value.offerId, quoteId: value.quoteId, confirmationId: value.confirmationId } : null;
    case "tout-settle": return id(value.settlementId) && (value.optionId === "pay" || value.optionId === "charm" || value.optionId === "flee") ? { type: value.type, settlementId: value.settlementId, optionId: value.optionId } : null;
    default: return null;
  }
}

function contextError(context: CompanionPlaceInteractionContext): string | null {
  const { principal, membership, layout, sharedNow } = context;
  if (!principal || !id(principal.userId) || !id(principal.characterId)) return "Sign in before using this place.";
  if (!Number.isSafeInteger(sharedNow) || sharedNow < 0) return "Refresh the shared place clock.";
  if (!membership || !id(membership.id) || !id(membership.instanceId) || membership.userId !== principal.userId || membership.characterId !== principal.characterId) return "Refresh authorized place membership.";
  if (!Number.isFinite(membership.expiresAt) || membership.expiresAt <= sharedNow) return "Place access expired. Rejoin the place.";
  if (membership.placeId !== layout.placeId || membership.layoutRevision !== layout.navigation.revision) return "Place layout changed. Reload before acting.";
  if (!["arrived", "detained", "recovering"].includes(membership.phase)) return "Arrive at the destination first.";
  if (!context.inputActive) return "Close the overlay and return focus to the place.";
  if (!pointValid(context.position) || !routeSegmentClear(layout.navigation, context.position, context.position)) return "Wait for an authorized, collision-safe position.";
  return null;
}
function stateFor(context: CompanionPlaceInteractionContext, service: CompanionPlaceService) {
  const matches = context.services.filter(state => state.serviceId === service.id);
  return matches.length === 1 ? matches[0] : undefined;
}
function serviceError(context: CompanionPlaceInteractionContext, service: CompanionPlaceService, state?: CompanionPlaceServiceState): string | null {
  if (!state || !Number.isSafeInteger(state.revision) || state.revision < 0 || !Number.isFinite(state.expiresAt) || state.expiresAt <= context.sharedNow) return "Refresh this service's current availability.";
  const membership = context.membership;
  if (!membership || state.membershipId !== membership.id || state.userId !== membership.userId || state.instanceId !== membership.instanceId || state.layoutRevision !== membership.layoutRevision) return "Refresh service options for the current authorized member.";
  if (!state.available) return state.reason || "This service is unavailable. Choose another activity.";
  if (service.npcId && !context.layout.npcs.some(npc => npc.id === service.npcId)) return "Service staff are missing from the layout.";
  return null;
}
const eatingAnchor = (service: CompanionPlaceService) => service.kind === "food" && !!service.offers?.includes("eat-carried-food");
function commandError(context: CompanionPlaceInteractionContext, service: CompanionPlaceService, state: CompanionPlaceServiceState, command: CompanionPlaceCommand): string | null {
  const wrongService = "Choose the matching person or activity.";
  const now = context.sharedNow;
  if (service.access === "policing-owner" && context.membership?.phase !== "detained") return "This custody service requires authorized detention membership.";
  if (context.membership?.phase === "detained" && command.type !== "police-state" && command.type !== "police-release") return "Complete release with the policing owner before other activities.";
  if (context.membership?.phase === "recovering" && command.type !== "medic-state" && command.type !== "medic-recover") return "Complete recovery with the recovery owner before other activities.";
  if (command.type.startsWith("dance-") && !service.offers?.includes("rhythm-circle")
    || command.type.startsWith("gather-") && (!state.gathering || !service.offers?.includes(state.gathering.offerId))
    || command.type === "capture" && !service.offers?.includes(command.format)
    || command.type === "football-enter" && !service.offers?.includes(command.mode === "solo" ? "solo-practice" : "team-match")
    || command.type === "arena-enter" && !service.offers?.includes("join-match")
    || command.type === "police-release" && !service.offers?.includes("release-options")) return "This activity is not offered at this service.";
  switch (command.type) {
    case "food-browse": return service.kind === "food" && !eatingAnchor(service) ? null : wrongService;
    case "food-order": {
      if (service.kind !== "food" || eatingAnchor(service)) return wrongService;
      const offer = state.food?.offers.find(item => item.id === command.offerId);
      return offer?.available && service.offers?.includes(command.offerId) && offer.quoteId === command.quoteId && Number.isFinite(offer.quoteExpiresAt) && offer.quoteExpiresAt > now ? null : "Refresh the available meal and confirm its current quote.";
    }
    case "food-collect": return service.kind === "food" && !eatingAnchor(service) && state.food?.orders.some(order => order.id === command.orderId && order.ready) ? null : "Wait for this order to be ready at its food counter.";
    case "food-eat": return eatingAnchor(service) && state.food?.carried.some(item => item.id === command.itemId && item.edible) ? null : "Bring collected food to an eating place.";
    case "driver-board": return service.kind === "driver" && state.boarding?.available && state.boarding.id === command.boardingId ? null : "Choose an available ride and confirm any fare with its owner.";
    case "medic-state": return service.kind === "medic" ? null : wrongService;
    case "medic-recover": return service.kind === "medic" && state.recovery?.id === command.recoveryId && state.recovery.eligible ? null : "Ask the medic for current recovery options.";
    case "police-state": return service.kind === "police" ? null : wrongService;
    case "police-release": return service.kind === "police" && state.detention?.id === command.detentionId
      && state.detention.options.some(option => option.id === command.optionId && option.eligible && !option.paymentRequired) ? null : "Choose an eligible non-payment release option. Paid bail is unavailable.";
    case "mission-browse": return service.kind === "mission" ? null : wrongService;
    case "mission-accept": return service.kind === "mission" && state.missions?.some(mission => mission.id === command.missionId && service.offers?.includes(mission.offerId) && mission.status === "offered") ? null : "Choose a currently offered mission.";
    case "mission-deliver": return service.kind === "mission" && state.missions?.some(mission => mission.id === command.missionId && service.offers?.includes(mission.offerId) && mission.status === "active" && mission.deliveryItemIds.includes(command.itemId)) ? null : "Bring the mission's required carried item to this contact.";
    case "dance-start": return service.kind === "dance" && !!state.dance && (!state.dance.round || evaluateCompanionDance(state.dance.round, state.dance.inputs, context.principal!.userId, now).phase !== "playing") ? null : "Finish the current rhythm objective first.";
    case "dance-hit": {
      const round = state.dance?.round;
      return service.kind === "dance" && round?.id === command.roundId && round.instanceId === context.membership!.instanceId && round.serviceId === service.id
        && evaluateCompanionDance(round, state.dance!.inputs, context.principal!.userId, now).phase === "playing" && command.beatIndex < round.lanes.length ? null : "Join the active rhythm objective before tapping a beat.";
    }
    case "dance-finish": {
      const round = state.dance?.round;
      return service.kind === "dance" && round?.id === command.roundId && round.instanceId === context.membership!.instanceId && round.serviceId === service.id
        && evaluateCompanionDance(round, state.dance!.inputs, context.principal!.userId, now).phase === "success" ? null : "Complete the rhythm objective before submitting it.";
    }
    case "gather-join": return service.kind === "gather" && state.gathering?.id === command.gatheringId && !state.gathering.joined ? null : "Choose an available gathering.";
    case "gather-ready": return service.kind === "gather" && state.gathering?.id === command.gatheringId && state.gathering.joined && !state.gathering.ready ? null : "Join this gathering before taking a place.";
    case "capture": return service.kind === "capture" && state.gathering?.id === command.gatheringId && state.gathering.canCapture
      && state.gathering.consentId === command.consentId && state.gathering.formats.includes(command.format) ? null : "Choose a capture format and obtain current participant consent.";
    case "football-enter": return service.kind === "football" && (command.mode === "solo" ? state.football?.solo : !!command.teamId && state.football?.teamIds.includes(command.teamId)) ? null : "Choose solo practice or an authorized chat team.";
    case "arena-enter": return service.kind === "arena" && state.arena?.eligible && state.arena.matchId === command.matchId && state.arena.rulesRevision === command.rulesRevision ? null : "Review the current arena rules and explicitly join an eligible match.";
    case "home-open": return service.kind === "home-door" && service.roomSlotId === command.roomSlotId && state.home?.roomSlotId === command.roomSlotId
      && state.home.visitGrantId === command.visitGrantId && state.home.expiresAt > now ? null : "Request current access to this saved room.";
    case "pos-browse": return service.kind === "pos" ? null : wrongService;
    case "pos-withdraw": case "pos-deposit": return service.kind === "pos" ? "POS transfers are unavailable." : wrongService;
    case "hawker-browse": return service.kind === "hawker" ? null : wrongService;
    case "hawker-buy": return service.kind === "hawker" ? "Purchases are unavailable until current payment terms are supplied." : wrongService;
    case "tout-state": return service.kind === "tout" ? null : wrongService;
    case "tout-settle": return command.optionId === "pay" ? "Paid settlement is unavailable."
      : service.kind === "tout" && state.tout?.encounterId === command.settlementId && state.tout.options.some(opt => opt.id === command.optionId && opt.eligible) ? null : "Choose an available settlement option.";
  }
}

/** Contextual choices use owner facts. Confirmation IDs are filled by the existing dedicated sheet. */
export function companionPlaceChoices(context: CompanionPlaceInteractionContext, serviceId: string): CompanionPlaceChoice[] {
  const service = context.layout.services.find(item => item.id === serviceId);
  if (!service) return [];
  const state = stateFor(context, service), error = contextError(context) || serviceError(context, service, state);
  if (!state) return [];
  const choices: CompanionPlaceChoice[] = [];
  switch (service.kind) {
    case "food":
      if (eatingAnchor(service)) {
        for (const item of state.food?.carried.slice(0, companionPlaceInteractionLimits.choices) ?? []) choices.push({ label: `Eat ${item.label}`, command: { type: "food-eat", itemId: item.id }, disabled: item.edible ? undefined : "This item cannot be eaten." });
      } else {
        // Ready collections must remain reachable when the menu fills the cap.
        for (const order of state.food?.orders.filter(order => order.ready).slice(0, companionPlaceInteractionLimits.choices) ?? []) choices.push({ label: `Collect ${order.label}`, command: { type: "food-collect", orderId: order.id } });
        choices.push({ label: "Browse food", command: { type: "food-browse" } });
        for (const offer of state.food?.offers.filter(offer => service.offers?.includes(offer.id)).slice(0, companionPlaceInteractionLimits.choices) ?? []) choices.push({ label: `Order ${offer.label}`, command: { type: "food-order", offerId: offer.id, quoteId: offer.quoteId ?? "", confirmationId: "" }, confirmation: "payment", disabled: offer.available && offer.quoteId && offer.quoteExpiresAt > context.sharedNow ? undefined : "Refresh availability and price." });
        for (const order of state.food?.orders.filter(order => !order.ready).slice(0, companionPlaceInteractionLimits.choices) ?? []) choices.push({ label: `Collect ${order.label}`, command: { type: "food-collect", orderId: order.id }, disabled: "Order is being prepared." });
      }
      break;
    case "driver": if (state.boarding) choices.push({ label: state.boarding.label, command: { type: "driver-board", boardingId: state.boarding.id }, disabled: state.boarding.available ? undefined : "Ride unavailable." }); break;
    case "medic": choices.push({ label: "Check recovery", command: { type: "medic-state" } }); if (state.recovery?.eligible) choices.push({ label: "Begin recovery", command: { type: "medic-recover", recoveryId: state.recovery.id } }); break;
    case "police":
      choices.push({ label: "Check status", command: { type: "police-state" } });
      for (const option of state.detention?.options.slice(0, companionPlaceInteractionLimits.choices) ?? []) if (!option.paymentRequired) choices.push({ label: option.label, command: { type: "police-release", detentionId: state.detention!.id, optionId: option.id }, disabled: option.eligible ? undefined : "Release conditions are not met." });
      break;
    case "mission":
      choices.push({ label: "Available work", command: { type: "mission-browse" } });
      for (const mission of state.missions?.slice(0, companionPlaceInteractionLimits.choices) ?? []) {
        if (mission.status === "offered") choices.push({ label: mission.label, command: { type: "mission-accept", missionId: mission.id } });
        else for (const itemId of mission.deliveryItemIds.slice(0, 4)) choices.push({ label: `Deliver for ${mission.label}`, command: { type: "mission-deliver", missionId: mission.id, itemId } });
      }
      break;
    case "dance": {
      const round = state.dance?.round, progress = round && state.dance ? evaluateCompanionDance(round, state.dance.inputs, context.principal?.userId ?? "", context.sharedNow) : null;
      if (!round || progress?.phase === "failed" || progress?.phase === "invalid") choices.push({ label: "Start rhythm", command: { type: "dance-start" } });
      else if (progress?.phase === "success") choices.push({ label: "Finish rhythm", command: { type: "dance-finish", roundId: round.id } });
      // During play, the renderer submits focused rhythm input, not another idle button row.
      break;
    }
    case "gather": if (state.gathering) choices.push(state.gathering.joined ? { label: "Take my place", command: { type: "gather-ready", gatheringId: state.gathering.id }, disabled: state.gathering.ready ? "Already ready." : undefined } : { label: "Join gathering", command: { type: "gather-join", gatheringId: state.gathering.id } }); break;
    case "capture": if (state.gathering) for (const format of [...new Set(state.gathering.formats.slice(0, 2))].filter(format => (format === "photo" || format === "clip") && service.offers?.includes(format))) choices.push({ label: format === "photo" ? "Take a photo" : "Record a clip", command: { type: "capture", gatheringId: state.gathering.id, consentId: state.gathering.consentId ?? "", format }, confirmation: "capture", disabled: state.gathering.canCapture ? undefined : "Gather participants and obtain consent." }); break;
    case "football": if (state.football?.solo) choices.push({ label: "Solo practice", command: { type: "football-enter", mode: "solo", teamId: null } }); for (const teamId of state.football?.teamIds.slice(0, companionPlaceInteractionLimits.choices) ?? []) choices.push({ label: "Play with team", command: { type: "football-enter", mode: "teams", teamId } }); break;
    case "arena": if (state.arena) choices.push({ label: "Join arena", command: { type: "arena-enter", matchId: state.arena.matchId, rulesRevision: state.arena.rulesRevision, consentId: "" }, confirmation: "arena-entry", disabled: state.arena.eligible ? undefined : "Match unavailable." }); break;
    case "home-door": if (state.home) choices.push({ label: "Enter room", command: { type: "home-open", roomSlotId: state.home.roomSlotId, visitGrantId: state.home.visitGrantId } }); break;
    case "pos":
      choices.push({ label: "Check POS balance", command: { type: "pos-browse" } });
      break;
    case "hawker":
      choices.push({ label: "Check items", command: { type: "hawker-browse" } });
      break;
    case "tout":
      choices.push({ label: "Street check", command: { type: "tout-state" } });
      if (state.tout) {
        for (const opt of state.tout.options.slice(0, companionPlaceInteractionLimits.choices)) {
          if (opt.id !== "pay") choices.push({ label: opt.label, command: { type: "tout-settle", settlementId: state.tout.encounterId, optionId: opt.id }, disabled: opt.eligible ? undefined : "Option unavailable." });
        }
      }
      break;
  }
  return choices.slice(0, companionPlaceInteractionLimits.choices).map(choice => ({ ...choice,
    disabled: error ?? choice.disabled ?? commandError(context, service, state, choice.command) ?? undefined }));
}

export function prepareCompanionPlaceInteraction(context: CompanionPlaceInteractionContext, serviceId: string, command: CompanionPlaceCommand, actionId: string, movementSequence: number): CompanionPlaceInteractionResult {
  const parsed = readCompanionPlaceCommand(command), service = context.layout.services.find(item => item.id === serviceId);
  const state = service && stateFor(context, service);
  const error = contextError(context) || (!id(actionId) || !Number.isSafeInteger(movementSequence) || movementSequence < 1 ? "Provide a stable action ID and movement sequence." : null)
    || (!parsed || !service ? "Choose a supported action in this layout." : null) || (service ? serviceError(context, service, state) : null)
    || (service && state && parsed ? commandError(context, service, state, parsed) : null);
  if (error || !parsed || !service || !state || !context.membership) throw new Error(error || "Refresh this place before acting.");
  const route = planRoomRoute(context.layout.navigation, context.position, service.approach.point);
  if (!route || route.length > companionPlaceInteractionLimits.routePoints) throw new Error("No safe route to this service. Choose another approach.");
  return { intent: { actionId, owner: ownerFor(parsed), serviceId, npcId: service.npcId ?? null, membership: { ...context.membership }, serviceRevision: state.revision,
    command: parsed, movementSequence, approach: [...service.approach.point] as ScenePoint, requestedAt: context.sharedNow },
    route: route.map(point => [...point] as ScenePoint), status: "walking", error: null };
}

/** Arrival must be supplied by the authenticated movement owner, not a canvas receipt. */
export function validateCompanionPlaceArrival(context: CompanionPlaceInteractionContext, intent: CompanionPlaceIntent, arrival: CompanionPlaceArrival): string | null {
  const error = contextError(context);
  if (error) return error;
  const membership = context.membership!, now = context.sharedNow;
  if (membership.id !== intent.membership.id || membership.instanceId !== intent.membership.instanceId || membership.userId !== intent.membership.userId || membership.characterId !== intent.membership.characterId) return "Membership changed. Choose the action again.";
  const service = context.layout.services.find(item => item.id === intent.serviceId), state = service && stateFor(context, service);
  if (!service || !pointValid(intent.approach) || Math.hypot(...service.approach.point.map((value, index) => value - intent.approach[index])) > .001
    || (service.npcId ?? null) !== intent.npcId) return "Service moved. Choose the action again.";
  const unavailable = serviceError(context, service, state);
  if (unavailable) return unavailable;
  if (state!.revision !== intent.serviceRevision) return "Service changed. Refresh its options before acting.";
  if (arrival.actionId !== intent.actionId || arrival.membershipId !== membership.id || arrival.userId !== membership.userId || arrival.characterId !== membership.characterId
    || arrival.instanceId !== membership.instanceId || arrival.layoutRevision !== membership.layoutRevision || intent.membership.layoutRevision !== membership.layoutRevision
    || arrival.movementSequence !== intent.movementSequence) return "Wait for the matching authorized movement arrival.";
  if (![arrival.arrivedAt, arrival.expiresAt].every(Number.isFinite) || arrival.arrivedAt < intent.requestedAt || arrival.arrivedAt > now || now - arrival.arrivedAt > companionPlaceInteractionLimits.arrivalMs
    || arrival.expiresAt <= now || arrival.expiresAt > membership.expiresAt) return "Arrival expired. Approach this service again.";
  if (!pointValid(arrival.position) || !routeSegmentClear(context.layout.navigation, arrival.position, service.approach.point)
    || distance(arrival.position, service.approach.point) > companionPlaceInteractionLimits.arrivalRadius || distance(context.position, arrival.position) > .1) return "Complete the walk and stay at this service.";
  return commandError(context, service, state!, intent.command);
}

export interface CompanionPlaceCommandOwner {
  /** Existing owner resolves this ID before rechecking eligibility or applying effects. */
  lookup(intent: CompanionPlaceIntent): Promise<CompanionPlaceAcknowledgement | null>;
  /** Deduplicate actionId atomically, revalidate auth/arrival/quotes/consent and apply owned state only. */
  dispatch(intent: CompanionPlaceIntent, arrival: CompanionPlaceArrival): Promise<CompanionPlaceAcknowledgement>;
}
export type CompanionPlaceInteractionOwners = {
  context(): CompanionPlaceInteractionContext;
  arrival(intent: CompanionPlaceIntent): Promise<CompanionPlaceArrival | null>;
  commands: Partial<Record<CompanionPlaceOwner, CompanionPlaceCommandOwner>>;
};

/** Local command tracking only. No timers, retries, purchases or commands run without an explicit call. */
export function createCompanionPlaceInteractions(owners: CompanionPlaceInteractionOwners) {
  const results = new Map<string, CompanionPlaceInteractionResult>();
  const sending = new Set<string>();
  let generation = 0, disposed = false;
  const copy = (result: CompanionPlaceInteractionResult): CompanionPlaceInteractionResult => ({ ...result, intent: { ...result.intent, membership: { ...result.intent.membership }, command: { ...result.intent.command }, approach: [...result.intent.approach] as ScenePoint }, route: result.route.map(point => [...point] as ScenePoint) });
  const put = (result: CompanionPlaceInteractionResult) => { results.set(result.intent.actionId, result); return copy(result); };
  const receipt = (result: CompanionPlaceInteractionResult, ack: CompanionPlaceAcknowledgement) => {
    const intent = result.intent;
    if (!ack || ack.actionId !== intent.actionId || ack.membershipId !== intent.membership.id || ack.userId !== intent.membership.userId || ack.instanceId !== intent.membership.instanceId
      || ack.layoutRevision !== intent.membership.layoutRevision || !["acknowledged", "pending", "rejected"].includes(ack.status)) throw new Error("Owner acknowledgement did not match this action. Retry with the same action ID.");
    return put({ ...result, status: ack.status === "acknowledged" ? "acknowledged" : ack.status === "pending" ? "retryable" : "rejected",
      error: ack.status === "acknowledged" ? null : ack.reason || (ack.status === "pending" ? "The owner has not acknowledged completion. Retry this action." : "Action rejected. Refresh the service options.") });
  };
  async function send(actionId: string, retry: boolean): Promise<CompanionPlaceInteractionResult> {
    if (disposed) throw new Error("Interaction controller is closed. Use the current account's controller.");
    const initial = results.get(actionId);
    if (!initial) throw new Error("Choose a service action first.");
    if (sending.has(actionId) || initial.status === "acknowledged" || initial.status === "rejected" || initial.status === "cancelled") return copy(initial);
    const owner = owners.commands[initial.intent.owner];
    if (!owner) return put({ ...initial, status: "retryable", error: `Connect the existing ${initial.intent.owner} owner before using this action.` });
    sending.add(actionId); put({ ...initial, status: "sending", error: null });
    const startedGeneration = generation;
    const currentRequest = () => {
      const principal = owners.context().principal;
      return generation === startedGeneration && results.has(actionId) && principal?.userId === initial.intent.membership.userId
        && principal?.characterId === initial.intent.membership.characterId;
    };
    const stale = (): CompanionPlaceInteractionResult => ({ ...copy(initial), status: "retryable", error: "Account or interaction changed. Resolve this action with its original owner." });
    try {
      const principal = owners.context().principal;
      if (!principal || principal.userId !== initial.intent.membership.userId || principal.characterId !== initial.intent.membership.characterId) throw new Error("Account or character changed. Return to the original account to resolve this action.");
      if (retry || initial.status === "retryable") {
        const acknowledged = await owner.lookup(copy(initial).intent);
        if (!currentRequest()) return stale();
        if (acknowledged) return receipt(initial, acknowledged);
      }
      const arrival = await owners.arrival(copy(initial).intent);
      if (!currentRequest()) return stale();
      if (!arrival) throw new Error("Complete the matching authorized walk before continuing.");
      const current = owners.context(), error = validateCompanionPlaceArrival(current, initial.intent, arrival);
      if (error) throw new Error(error);
      // Eligibility after a lost response belongs to the owner, which resolves the stable ID first.
      const acknowledgement = await owner.dispatch(copy(initial).intent, { ...arrival, position: [...arrival.position] as ScenePoint });
      if (!currentRequest()) return stale();
      return receipt(initial, acknowledgement);
    } catch (error) {
      if (!currentRequest()) return stale();
      return put({ ...initial, status: "retryable", error: error instanceof Error ? error.message : "Action not acknowledged. Retry with the same action ID." });
    } finally { sending.delete(actionId); }
  }
  return {
    prepare(serviceId: string, command: CompanionPlaceCommand, actionId: string, movementSequence: number) {
      if (disposed) throw new Error("Interaction controller is closed. Use the current account's controller.");
      const existing = results.get(actionId);
      if (existing) {
        const principal = owners.context().principal;
        if (existing.intent.serviceId !== serviceId || JSON.stringify(existing.intent.command) !== JSON.stringify(readCompanionPlaceCommand(command)) || existing.intent.movementSequence !== movementSequence
          || existing.intent.membership.userId !== principal?.userId || existing.intent.membership.characterId !== principal?.characterId) throw new Error("Action ID already belongs to another request. Use a new action ID.");
        return copy(existing);
      }
      if (results.size >= companionPlaceInteractionLimits.receipts) throw new Error("Resolve outstanding actions, then clear the local interaction history.");
      return put(prepareCompanionPlaceInteraction(owners.context(), serviceId, command, actionId, movementSequence));
    },
    arrived: (actionId: string) => send(actionId, false),
    retry: (actionId: string) => send(actionId, true),
    get(actionId: string) { const result = results.get(actionId); return result ? copy(result) : null; },
    cancel(actionId: string) {
      const result = results.get(actionId);
      if (!result) return null;
      if (sending.has(actionId) || result.status === "retryable") throw new Error("Resolve the owner's result before discarding this action.");
      return result.status === "walking" ? put({ ...result, status: "cancelled", error: null }) : copy(result);
    },
    clear() {
      if (sending.size || [...results.values()].some(result => result.status === "retryable")) throw new Error("Resolve unacknowledged actions before clearing interaction history.");
      results.clear();
      generation++;
    },
    /** Account owner unmounts this tracker; unresolved IDs must remain in its existing command receipts. */
    dispose() { disposed = true; generation++; results.clear(); },
  };
}

export type CompanionDanceRound = {
  id: string; instanceId: string; serviceId: string; startsAt: number; beatMs: number;
  windowMs: number; lanes: readonly number[]; requiredHits: number;
};
/** receivedAt is assigned by the activity owner. Client tap timestamps never determine scores. */
export type CompanionDanceInput = { userId: string; beatIndex: number; lane: number; receivedAt: number };
export function evaluateCompanionDance(round: CompanionDanceRound, inputs: readonly CompanionDanceInput[], userId: string, sharedNow: number) {
  const valid = id(round.id) && id(round.instanceId) && id(round.serviceId) && Number.isFinite(sharedNow) && Number.isFinite(round.startsAt)
    && Number.isFinite(round.beatMs) && round.beatMs >= 250 && round.beatMs <= 2000 && Number.isFinite(round.windowMs) && round.windowMs >= 30 && round.windowMs < round.beatMs / 2
    && round.lanes.length > 0 && round.lanes.length <= companionPlaceInteractionLimits.beats && round.lanes.every(lane => Number.isSafeInteger(lane) && lane >= 0 && lane < 4)
    && Number.isSafeInteger(round.requiredHits) && round.requiredHits >= 1 && round.requiredHits <= round.lanes.length;
  if (!valid) return { phase: "invalid" as const, hits: 0, misses: 0, remaining: 0, nextBeat: null as number | null, endsAt: 0, objective: "Refresh the rhythm objective." };
  const endsAt = round.startsAt + (round.lanes.length - 1) * round.beatMs + round.windowMs;
  const attempts = new Map<number, CompanionDanceInput>();
  for (const input of inputs.slice(0, companionPlaceInteractionLimits.beats * 24)) {
    if (input.userId !== userId || !Number.isSafeInteger(input.beatIndex) || input.beatIndex < 0 || input.beatIndex >= round.lanes.length
      || !Number.isFinite(input.receivedAt) || input.receivedAt > sharedNow || input.receivedAt < round.startsAt - round.windowMs) continue;
    const old = attempts.get(input.beatIndex);
    if (!old || input.receivedAt < old.receivedAt || input.receivedAt === old.receivedAt && input.lane < old.lane) attempts.set(input.beatIndex, input);
  }
  let hits = 0, misses = 0, nextBeat: number | null = null;
  for (let index = 0; index < round.lanes.length; index++) {
    const at = round.startsAt + index * round.beatMs, input = attempts.get(index);
    const hit = !!input && input.lane === round.lanes[index] && Math.abs(input.receivedAt - at) <= round.windowMs;
    if (hit) hits++;
    else if (at + round.windowMs < sharedNow) misses++;
    else if (nextBeat === null) nextBeat = index;
  }
  const phase = sharedNow <= endsAt ? "playing" as const : hits >= round.requiredHits ? "success" as const : "failed" as const;
  return { phase, hits, misses, remaining: round.lanes.length - hits - misses, nextBeat, endsAt, objective: `Hit ${round.requiredHits} of ${round.lanes.length} beats` };
}

export type CompanionGatheringObjective = {
  id: string; instanceId: string; serviceId: string; layoutRevision: string;
  zoneId: string; requiredParticipants: number; expiresAt: number;
};
export type CompanionGatheringSeat = {
  membership: CompanionPlaceMembership; gatheringId: string; joined: boolean; ready: boolean;
  position: ScenePoint; observedAt: number; expiresAt: number; captureConsentId: string | null; captureConsentExpiresAt: number;
};
/** Display/objective calculation only. Existing activity/content owners publish completion or capture. */
export function evaluateCompanionGathering(layout: CompanionPlaceLayout, objective: CompanionGatheringObjective, seats: readonly CompanionGatheringSeat[], sharedNow: number) {
  const service = layout.services.find(item => item.id === objective.serviceId && (item.kind === "gather" || item.kind === "capture"));
  const zone = layout.zones.find(item => item.id === objective.zoneId && (item.activity === "gather" || item.activity === "capture"));
  if (!service || !zone || objective.layoutRevision !== layout.navigation.revision || !id(objective.id) || !id(objective.instanceId) || !Number.isFinite(sharedNow)
    || !Number.isFinite(objective.expiresAt) || objective.expiresAt <= sharedNow || !Number.isSafeInteger(objective.requiredParticipants) || objective.requiredParticipants < 1 || objective.requiredParticipants > service.capacity) return { ready: false, canCapture: false, participants: [] as string[], remaining: 0, error: "Refresh the gathering objective and zone." };
  const participants = new Map<string, CompanionGatheringSeat>();
  for (const seat of seats.slice(0, companionPlaceInteractionLimits.choices)) {
    const membership = seat.membership, point = seat.position;
    if (!id(membership.userId) || !id(membership.id) || membership.instanceId !== objective.instanceId || membership.placeId !== layout.placeId || membership.layoutRevision !== objective.layoutRevision
      || !Number.isFinite(membership.expiresAt) || membership.expiresAt <= sharedNow || membership.phase !== "arrived" || seat.gatheringId !== objective.id || !seat.joined || !seat.ready
      || !Number.isFinite(seat.observedAt) || !Number.isFinite(seat.expiresAt) || seat.observedAt > sharedNow || sharedNow - seat.observedAt > companionPlaceInteractionLimits.arrivalMs || seat.expiresAt <= sharedNow
      || !pointValid(point) || !routeSegmentClear(layout.navigation, point, point) || point[0] < zone.bounds.minX || point[0] > zone.bounds.maxX || point[2] < zone.bounds.minZ || point[2] > zone.bounds.maxZ) continue;
    // Duplicate seats cannot increase a group's completion count or replace missing consent.
    if (participants.has(membership.userId)) return { ready: false, canCapture: false, participants: [] as string[], remaining: objective.requiredParticipants, error: "Refresh duplicate gathering seats." };
    participants.set(membership.userId, seat);
  }
  const ready = participants.size >= objective.requiredParticipants && participants.size <= service.capacity;
  return { ready, canCapture: ready && [...participants.values()].every(seat => id(seat.captureConsentId) && Number.isFinite(seat.captureConsentExpiresAt) && seat.captureConsentExpiresAt > sharedNow),
    participants: [...participants.keys()].sort(), remaining: Math.max(0, objective.requiredParticipants - participants.size), error: null };
}
