import { defineTable, makeFunctionReference } from "convex/server";
import { ConvexError, v } from "convex/values";
import type { MutationCtx, QueryCtx } from "../_generated/server";
import type { Doc, Id } from "../_generated/dataModel";
import { resolveCompanionWorldInstance, COMPANION_WORLD_ID, type CompanionWorldInstance } from "../../../../packages/utils/src/companionWorld";
import { companionHaulSavedLibrary, activeCompanionHaul, resolveValidatedCompanionHaul } from "./companionHaulModel";
import { setFootballPlaceEligibility } from "../companionFootball";
import { nightclubActivity } from "./companionNightclubModel";
import { createValidatedCompanionDeathDrop } from "./companionHaulDropsModel";
import type { ScenePoint } from "../../../../packages/utils/src/companionNavigation";
import { companionTrafficRequest, companionTrafficRoute, companionTrafficView } from "./companionTraffic";
import { streetWeaponId } from "./companionCombatEquipmentModel";
import { cityResidence, companionCityHousingTables } from "./companionCityHousingModel";
import { isCompanionCity } from "../../../../packages/utils/src/companionCity";
import { residentVehicleKind } from "./companionResidentVehiclesSchema";
import { interruptValidatedPlayerVehicleTrip } from "./companionPlayerVehicleTripInterruption";
import { companionRoadEndpoint, companionTravelEndpoint } from "./companionRoadsideSchema";
import { companionRoadsidePlaceId, resolveCompanionRoadsideInstance } from "../../../../packages/utils/src/companionRoadside";
import { settleWorldKitchenDeparture } from "./companionKitchenWorldLifecycle";
import { companionStreetCombatWorldFields } from "./companionStreetCombatSchema";
import { journeyEnforcementDecision } from "./companionPolicingModel";
import { worldTransitInstanceFields, worldTransitRequest, worldTransitRide, worldTransitTables } from "./companionWorldTransitSchema";
import { projectWorldTransitPlayer, releaseWorldTransitPassenger } from "./companionWorldTransit";
import { companionWorldJsonTables, readCompanionWorldInstanceJson, storeCompanionJson } from "./companionWorldJson";
import { worldTransitFareTables } from "./companionWorldTransitFareSchema";
import { playerDrivingState } from "./companionPlayerDrivingSchema";
import { projectWorldDrivingPlayer, releaseWorldDriving } from "./companionWorldDriving";
import { worldWaterControlState, worldWaterInstanceFields, worldWaterPlayerFields } from "./companionWorldWaterSchema";
import { projectWorldWaterPlayer, releaseWorldWaterControl, worldWaterPublicControl } from "./companionWorldWater";
import { worldStreetInstanceFields, worldStreetPlayerFields } from "./companionWorldStreetMechanicsSchema";
import { interruptWorldStreetMechanics } from "./companionWorldStreetMechanics";
import { interruptMovingHawkerFoodOuting } from "./companionMovingHawkerFood";
export { releaseWorldTransitPassenger } from "./companionWorldTransit";

const worldJourneyRoute = v.union(v.object({ id: v.string(), version: v.literal("lagos-corridors-v1"), placement: v.literal("proposed"),
  points: v.array(v.object({ latitude: v.number(), longitude: v.number() })), lengthMetres: v.number() }), companionTrafficRoute);
export const roadsideVisit = v.object({ endpoint: companionRoadEndpoint, sourceJourneyId: v.string(), stoppedAt: v.number(),
  exit: v.object({ placeId: v.string(), route: worldJourneyRoute }) });

import { worldPoint, worldMovement, worldGeoPoint } from "./companionWorldSchema";
export { worldPoint, worldMovement, worldGeoPoint };
export const worldJourney = v.object({ id: v.string(), requestId: v.string(), fromPlaceId: v.string(), toPlaceId: v.string(),
  route: worldJourneyRoute,
  vehicle: v.union(residentVehicleKind, v.literal("canoe"), v.literal("speedboat")), startedAt: v.number(), arrivesAt: v.number(),
  checkpoint: v.union(v.object({ progress: v.number(), startsAt: v.number(), endsAt: v.number(), resumedAt: v.optional(v.union(v.number(), v.null())), enforcement: v.optional(journeyEnforcementDecision) }), v.null()),
  traffic: v.optional(companionTrafficView),
  travelTiming: v.optional(v.object({ rate: v.union(v.literal(1), v.literal(4)), anchorAt: v.number(), anchorProgress: v.number(), durationMs: v.number() })),
  fromEndpoint: v.optional(companionTravelEndpoint), toEndpoint: v.optional(companionTravelEndpoint),
  travelMode: v.optional(v.union(v.literal("walk"), v.literal("vehicle"), v.literal("boat"))),
  status: v.union(v.literal("traveling"), v.literal("arrived"), v.literal("interrupted")) });
export const worldPlayerFields = { userId: v.id("users"), characterId: v.string(), gameRoomId: v.id("companionGameRooms"),
  placeId: v.string(), instanceId: v.string(), revision: v.number(), position: worldPoint,
  movement: v.union(worldMovement, v.null()), moveSequence: v.number(), status: v.union(v.literal("present"), v.literal("traveling"), v.literal("recovering")),
  health: v.number(), journey: v.union(worldJourney, v.null()), homeRoomId: v.string(), nightclub: v.optional(v.union(nightclubActivity, v.null())),
  residence: v.optional(v.union(cityResidence, v.null())), roadside: v.optional(v.union(roadsideVisit, v.null())),
  transitRide: v.optional(v.union(worldTransitRide, v.null())),
  driving: v.optional(v.union(playerDrivingState, v.null())) };
export const worldPlayerView = v.object({ ...worldPlayerFields,
  waterControl: v.optional(v.union(worldWaterControlState, v.null())),
  drawnWeaponId: v.optional(v.union(streetWeaponId, v.null())) });
export const companionWorldTables = {
  ...worldTransitTables,
  ...worldTransitFareTables,
  ...companionWorldJsonTables,
  ...companionCityHousingTables,
  companionWorldInstances: defineTable({ instanceId: v.string(), gameRoomId: v.id("companionGameRooms"), resolvedJson: v.string(), createdAt: v.number(),
    ...companionStreetCombatWorldFields, ...worldTransitInstanceFields, ...worldWaterInstanceFields,
    ...worldStreetInstanceFields, streetTrafficJson: v.optional(v.string()),
    streetRespondersJson: v.optional(v.string()), streetNpcSnapshotJson: v.optional(v.string()) })
    .index("by_instanceId", ["instanceId"]).index("by_gameRoomId", ["gameRoomId"]),
  companionWorldPlayers: defineTable({ ...worldPlayerFields, ...worldWaterPlayerFields, ...worldStreetPlayerFields,
    drivingWakeAt: v.optional(v.union(v.number(), v.null())),
    sessionId: v.id("userSessions"), lastMoveAt: v.number(), readyInstanceId: v.union(v.string(), v.null()),
    pendingHomeArrival: v.optional(v.object({ instanceId: v.string(), eventKey: v.string(), revision: v.number() })),
    venueReturn: v.optional(v.object({ venueInstanceId: v.string(), cityInstanceId: v.string(), cityVersion: v.string(),
      parcelId: v.string(), entrance: v.array(v.number()) })),
    travelRequest: v.optional(v.union(companionTrafficRequest, v.null())),
    transitRequests: v.optional(v.array(worldTransitRequest)),
    lastRequestId: v.string(), lastRequestFingerprint: v.string(), updatedAt: v.number() })
    .index("by_userId", ["userId"]).index("by_gameRoomId_and_placeId", ["gameRoomId", "placeId"]),
};
export async function companionWorldPlayer(ctx: QueryCtx | MutationCtx, userId: Id<"users">, now?: number): Promise<Doc<"companionWorldPlayers"> | null> {
  const row = await ctx.db.query("companionWorldPlayers").withIndex("by_userId", q => q.eq("userId", userId)).unique();
  // Queries pass an observation time. Mutation callers include combat/police.
  const driving = row?.driving ? await projectWorldDrivingPlayer(ctx, row) : row;
  const observedAt = now ?? ("scheduler" in ctx ? Date.now() : driving?.updatedAt ?? 0);
  const water = driving?.waterControl?.phase === "active" ? await projectWorldWaterPlayer(ctx, driving, observedAt) : driving;
  return water?.transitRide ? projectWorldTransitPlayer(ctx, water, observedAt) : water;
}
export function companionWorldPlayerView(row: Doc<"companionWorldPlayers">) {
  const { userId, characterId, gameRoomId, placeId, instanceId, revision, position, movement, moveSequence, status, health, journey, homeRoomId } = row;
  return { userId, characterId, gameRoomId, placeId, instanceId, revision, position, movement, moveSequence, status, health, journey, homeRoomId,
    residence: row.residence ?? null,
    roadside: row.roadside ?? null,
    transitRide: row.transitRide?.visitRevision === revision && status === "present" ? row.transitRide : null,
    driving: status === "present" && row.driving?.instanceId === instanceId ? row.driving : null,
    waterControl: status === "present" && row.waterControl?.phase === "active"
      && row.waterControl.scope.instanceId === instanceId ? worldWaterPublicControl(row.waterControl) : null,
    nightclub: placeId === "nightclub" && status === "present" && row.nightclub?.visitRevision === revision
      && (row.nightclub.kind !== "dance" || row.nightclub.round.instanceId === instanceId) ? row.nightclub : null };
}
export async function companionWorldInstance(ctx: QueryCtx | MutationCtx, player: Pick<Doc<"companionWorldPlayers">, "userId" | "gameRoomId" | "placeId" | "homeRoomId">
  & Partial<Pick<Doc<"companionWorldPlayers">, "roadside" | "journey">>, placeId = player.placeId): Promise<CompanionWorldInstance> {
  if (placeId.startsWith("roadside:")) {
    const endpoint = player.journey?.status === "traveling" && player.journey.toEndpoint?.kind === "road" && companionRoadsidePlaceId(player.journey.toEndpoint) === placeId
      ? player.journey.toEndpoint : player.roadside?.endpoint;
    if (!endpoint || companionRoadsidePlaceId(endpoint) !== placeId) throw new ConvexError("This roadside visit is unavailable.");
    return resolveCompanionRoadsideInstance(String(player.gameRoomId), endpoint);
  }
  if (placeId !== "own-home") {
    const frozen = await ctx.db.query("companionWorldInstances").withIndex("by_instanceId", q => q.eq("instanceId", `${player.gameRoomId}:${placeId}`)).unique();
    if (frozen) return readCompanionWorldInstanceJson(ctx, frozen.resolvedJson);
  }
  const library = await companionHaulSavedLibrary(ctx, player.userId);
  const home = library.rooms.find(room => room.id === (placeId === "own-home" && player.placeId === "own-home" ? player.homeRoomId : library.homeRoomId));
  return resolveCompanionWorldInstance(String(player.gameRoomId), placeId, home, String(player.userId));
}
/** Freeze a generated shared layout once. Saved homes remain the owner's live saved rooms. */
export async function ensureCompanionWorldInstance(ctx: MutationCtx, player: Parameters<typeof companionWorldInstance>[1], placeId = player.placeId): Promise<CompanionWorldInstance> {
  const instance = await companionWorldInstance(ctx, player, placeId);
  if (placeId !== "own-home" && !placeId.startsWith("roadside:") && !await ctx.db.query("companionWorldInstances").withIndex("by_instanceId", q => q.eq("instanceId", instance.id)).unique()) {
    instance.sharedStartedAt = Date.now();
    const fields = { instanceId: instance.id, gameRoomId: player.gameRoomId, createdAt: instance.sharedStartedAt };
    const resolvedJson = await storeCompanionJson(ctx, fields, "resolvedJson", JSON.stringify(instance));
    await ctx.db.insert("companionWorldInstances", { ...fields, resolvedJson });
    const game = await ctx.db.get(player.gameRoomId);
    if (game && !game.serverKey) await ctx.scheduler.runAt(Math.max(Date.now(), game.expiresAt) + 60_000,
      makeFunctionReference<"mutation", { gameRoomId: Id<"companionGameRooms"> }, null>("companionWorld:expireInstances"), { gameRoomId: game._id });
  }
  return instance;
}
/** Combat calls only after validating lethal damage, in that same mutation. */
export async function transferValidatedCompanionWorldDeath(ctx: MutationCtx, args: { userId: Id<"users">; gameRoomId: Id<"companionGameRooms">; instanceId: string; eventKey: string; position: ScenePoint }) {
  const player = await ctx.db.query("companionWorldPlayers").withIndex("by_userId", q => q.eq("userId", args.userId)).unique();
  if (!player || player.gameRoomId !== args.gameRoomId || player.instanceId !== args.instanceId
    || (player.status !== "present" && (player.status !== "traveling" || player.journey?.status !== "traveling")) || player.health <= 0) throw new ConvexError("This encounter no longer owns the player.");
  await interruptValidatedPlayerVehicleTrip(ctx, player, Date.now());
  await interruptWorldStreetMechanics(ctx, player, await companionWorldInstance(ctx, player), Date.now());
  await releaseWorldDriving(ctx, player, Date.now(), true);
  await releaseWorldWaterControl(ctx, player, Date.now(), "death");
  await releaseWorldTransitPassenger(ctx, player, Date.now());
  await settleWorldKitchenDeparture(ctx, player, Date.now());
  const outing = await activeCompanionHaul(ctx, player.userId);
  if (outing) {
    await interruptMovingHawkerFoodOuting(ctx, player.userId, outing._id);
    const outcome = await resolveValidatedCompanionHaul(ctx, { kind: "death", deathValidated: true, userId: player.userId, outingId: outing._id,
      eventKey: args.eventKey, worldId: COMPANION_WORLD_ID, worldRevision: player.revision + 1 });
    if (outcome.changed && outcome.outcome === "forfeited") await createValidatedCompanionDeathDrop(ctx, {
      userId: player.userId, outingId: outing._id, deathKey: args.eventKey, worldId: COMPANION_WORLD_ID,
      gameRoomId: player.gameRoomId, instanceId: player.instanceId, placeId: player.placeId, position: args.position,
    });
  }
  await setFootballPlaceEligibility(ctx, player.gameRoomId, player.userId, false);
  const hospital = await ensureCompanionWorldInstance(ctx, player, "hospital"), spawn = hospital.layout!.recoverySpawns[0]?.point ?? hospital.spawns[0];
  await ctx.db.patch(player._id, { placeId: "hospital", instanceId: hospital.id, position: { x: spawn[0], z: spawn[2], yaw: 0 },
    movement: null, journey: player.journey ? { ...player.journey, status: "interrupted" } : null, status: "recovering", health: 0,
    revision: player.revision + 1, readyInstanceId: null, nightclub: null, roadside: null, transitRide: null, driving: null,
    waterControl: null, roadWaterExit: null, transitRequests: undefined, pendingHomeArrival: undefined,
    venueReturn: undefined, updatedAt: Date.now() });
  return { instanceId: hospital.id, placeId: "hospital", position: spawn };
}
/** Only a validated medic/recovery transition may restore health. No medical charge. */
export async function recoverValidatedCompanionWorldPlayer(ctx: MutationCtx, userId: Id<"users">) {
  const player = await companionWorldPlayer(ctx, userId);
  if (!player || player.placeId !== "hospital" || player.status !== "recovering") throw new ConvexError("Recovery is unavailable.");
  await settleWorldKitchenDeparture(ctx, player, Date.now());
  await ctx.db.patch(player._id, { status: "present", health: 100, revision: player.revision + 1, updatedAt: Date.now() });
}
