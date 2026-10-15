import { companionFoodCounter, companionFoodItem, companionFoodServiceKey, type CompanionFoodKey } from "./companionFood";
import { routeSegmentClear, type NavigationObstacle, type RoomNavigation, type ScenePoint } from "./companionNavigation";
import { companionPlayerDrivingRules, type CompanionPlayerDrivingState } from "./companionPlayerDriving";
import { validCompanionStreetVehiclePose, type CompanionStreetVehiclePose } from "./companionStreetCrashes";
import { admitCompanionPedestrianRoute, companionPedestrianPointRefuge, type CompanionStreetPedestrianActor,
  type CompanionPedestrianRoute } from "./companionStreetPedestrians";

export const companionMovingHawkerRules = Object.freeze({ maxSpeed: 15 / 3.6, reach: 1.6, offerMs: 8000,
  acceptedAgeMs: 250, maxOffers: 8, bottleneckGap: 8, windowOffsetLimit: 1 });
export type CompanionMovingHawkerWindow = Readonly<{ side: -1 | 1; halfWidth: number; longitudinalOffset: number; point: ScenePoint }>;
export type CompanionHawkerBottleneck = Readonly<{ id: string; instanceId: string; layoutRevision: string; at: number;
  bounds: NavigationObstacle; blockedBy: CompanionStreetVehiclePose; blockedSpeed: number }>;
export type CompanionMovingHawkerStock = Readonly<{ serviceKey: string; revision: number; remaining: number }>;
export type CompanionMovingHawkerOffer = Readonly<{ id: string; drivingId: string; vehicleId: string; driverId: string;
  gameRoomId: string; instanceId: string; layoutRevision: string; lifeId: string; visitId: string;
  vendorId: string; npcId: string; vendorRevision: number; serviceId: string; bottleneckId: string; offerKey: CompanionFoodKey;
  quantity: 1 | 3 | 6; stockKey: string; stockRevision: number; amount: number; unitKey: "lagos-credit";
  issuedAt: number; expiresAt: number; status: "offered" | "fulfilled" | "cancelled"; revision: number;
  windowSide: -1 | 1; windowHalfWidth: number; windowOffset: number }>;
export type CompanionMovingHawkerContext = Readonly<{ driving: CompanionPlayerDrivingState; visitId: string;
  navigation: RoomNavigation; vendor: CompanionStreetPedestrianActor; window: CompanionMovingHawkerWindow;
  bottleneck: CompanionHawkerBottleneck; stock: CompanionMovingHawkerStock; now: number;
  /** Finite current stock counter, resident identity and driving scope were resolved by their owners. */
  inputHeldBySheet: boolean; serviceAvailable: boolean;
  /** World observes the exact persisted neutral chassis during confirmation, without accepting new movement. */
  stationaryObservation?: Readonly<{ drivingId: string; drivingRevision: number; vehicleRevision: number;
    sequence: number; at: number }> }>;
const keyValid = (key: string) => typeof key === "string" && key.length > 0 && key.length <= 240 && key.trim() === key;
const integer = (n: number) => Number.isSafeInteger(n) && n >= 0;
const pointValid = (p: ScenePoint) => Array.isArray(p) && p.length === 3 && p.every(Number.isFinite);
const dist = (a: ScenePoint, b: ScenePoint) => Math.hypot(a[0] - b[0], a[2] - b[2]);
export function companionMovingHawkerWindow(driving: CompanionPlayerDrivingState, side: -1 | 1, halfWidth: number,
  longitudinalOffset = 0): CompanionMovingHawkerWindow {
  const p = driving.pose;
  return Object.freeze({ side, halfWidth, longitudinalOffset, point: Object.freeze([
    p.x + Math.cos(p.yaw) * side * halfWidth + Math.sin(p.yaw) * longitudinalOffset, p.y,
    p.z - Math.sin(p.yaw) * side * halfWidth + Math.cos(p.yaw) * longitudinalOffset]) as ScenePoint });
}
function contextEligible(context: CompanionMovingHawkerContext, requireReach = true, confirmHeld = false) {
  const { driving: d, vendor: v, window: w, bottleneck: b, stock, now, navigation } = context;
  const observation = context.stationaryObservation;
  const observedHeld = confirmHeld && context.inputHeldBySheet && !!observation
    && observation.drivingId === d.id && observation.drivingRevision === d.revision
    && observation.vehicleRevision === d.vehicleRevision && observation.sequence === d.sequence
    && integer(observation.at) && observation.at <= now && now - observation.at <= companionMovingHawkerRules.acceptedAgeMs
    && observation.at >= d.acceptedAt && d.speed === 0 && ["stopped", "pulled-over", "suspended"].includes(d.mode)
    && d.input.throttle === 0 && d.input.brake === 1 && d.input.steering === 0;
  if (!integer(now) || ![d.id, d.vehicleId, d.gameRoomId, d.instanceId, d.driver.userId, d.driver.lifeId, context.visitId].every(keyValid)
    || ![d.pose.x, d.pose.y, d.pose.z, d.pose.yaw, d.speed].every(Number.isFinite) || d.speed < 0 || d.speed >= companionMovingHawkerRules.maxSpeed
    || !integer(d.acceptedAt) || d.acceptedAt > now || !observedHeld && now - d.acceptedAt > companionMovingHawkerRules.acceptedAgeMs
    || !["driving", "stopped"].includes(d.mode) && !observedHeld
    || d.navigationRevision !== navigation.revision || !context.serviceAvailable || !v.available || !["routine", "hawker"].includes(v.motionOwner)
    || !keyValid(v.id) || !keyValid(v.npcId) || !integer(v.revision) || !integer(v.acceptedAt) || v.acceptedAt > now
    || now - v.acceptedAt > 250 || !pointValid(v.point) || !Number.isFinite(v.radius) || v.radius < .2 || v.radius > .8
    || ![-1, 1].includes(w.side) || !Number.isFinite(w.halfWidth) || w.halfWidth < .3 || w.halfWidth > 3
    || w.halfWidth !== companionPlayerDrivingRules.halfWidth || d.routeId !== companionPlayerDrivingRules.routeId
    || !Number.isFinite(w.longitudinalOffset) || Math.abs(w.longitudinalOffset) > companionMovingHawkerRules.windowOffsetLimit
    || !pointValid(w.point) || dist(w.point, companionMovingHawkerWindow(d, w.side, w.halfWidth, w.longitudinalOffset).point) > .01
    || Math.abs(w.point[1] - d.pose.y) > .01 || Math.abs(v.point[1] - d.pose.y) > .3
    || requireReach && (dist(v.point, w.point) > companionMovingHawkerRules.reach
      || !routeSegmentClear({ ...navigation, radius: .15 }, v.point, w.point))
    || !keyValid(b.id) || b.instanceId !== d.instanceId || b.layoutRevision !== navigation.revision
    || !integer(b.at) || b.at > now || now - b.at > 250 || !validCompanionStreetVehiclePose(b.blockedBy)
    || b.blockedBy.id === d.vehicleId || !Number.isFinite(b.blockedSpeed) || b.blockedSpeed < 0 || b.blockedSpeed >= 15 / 3.6
    || ![b.bounds.minX, b.bounds.maxX, b.bounds.minZ, b.bounds.maxZ].every(Number.isFinite)
    || b.bounds.minX >= b.bounds.maxX || b.bounds.minZ >= b.bounds.maxZ || b.bounds.yaw || b.bounds.polygon
    || d.pose.x < b.bounds.minX || d.pose.x > b.bounds.maxX || d.pose.z < b.bounds.minZ || d.pose.z > b.bounds.maxZ
    || !keyValid(stock.serviceKey) || !integer(stock.revision) || !integer(stock.remaining)) return false;
  const dx = b.blockedBy.point[0] - d.pose.x, dz = b.blockedBy.point[2] - d.pose.z;
  const forward = dx * Math.sin(d.pose.yaw) + dz * Math.cos(d.pose.yaw), lateral = Math.abs(dx * Math.cos(d.pose.yaw) - dz * Math.sin(d.pose.yaw));
  const vendorX = v.point[0] - d.pose.x, vendorZ = v.point[2] - d.pose.z;
  const vendorLateral = Math.abs(vendorX * Math.cos(d.pose.yaw) - vendorZ * Math.sin(d.pose.yaw));
  const vendorForward = Math.abs(vendorX * Math.sin(d.pose.yaw) + vendorZ * Math.cos(d.pose.yaw));
  if (requireReach && vendorLateral <= companionPlayerDrivingRules.halfWidth + v.radius + .1
    && vendorForward <= companionPlayerDrivingRules.halfLength + v.radius + .1) return false;
  const relative = b.blockedBy.yaw - d.pose.yaw, c = Math.abs(Math.cos(relative)), s = Math.abs(Math.sin(relative));
  const leadingHalfLength = c * b.blockedBy.halfLength + s * b.blockedBy.halfWidth;
  const leadingHalfWidth = c * b.blockedBy.halfWidth + s * b.blockedBy.halfLength;
  return forward >= leadingHalfLength + companionPlayerDrivingRules.halfLength
    && forward - leadingHalfLength - companionPlayerDrivingRules.halfLength <= companionMovingHawkerRules.bottleneckGap
    && lateral <= leadingHalfWidth + w.halfWidth && Math.abs(b.blockedBy.point[1] - d.pose.y) <= .3;
}
/** Route the current vendor to a point outside the complete chassis before issuing a reachable offer. */
export function admitCompanionMovingHawkerApproach(context: CompanionMovingHawkerContext,
  route: Omit<Parameters<typeof admitCompanionPedestrianRoute>[0], "navigation" | "actor" | "kind" | "refuge" | "now">): CompanionPedestrianRoute | null {
  if (!contextEligible(context, false) || context.inputHeldBySheet || context.stock.remaining < 1
    || route.scope.instanceId !== context.driving.instanceId || route.scope.layoutRevision !== context.navigation.revision) return null;
  const item = companionFoodItem(context.vendor.npcId === "mai-suya" ? "suya" : context.vendor.npcId === "akara-woman" ? "akara" : "");
  if (!item || context.stock.remaining > item.stock
    || context.stock.serviceKey !== companionFoodServiceKey("town-square", `${context.vendor.npcId}-food`, item.offerKey)) return null;
  const { window, vendor, driving } = context, offset = vendor.radius + .2;
  const point: ScenePoint = [window.point[0] + Math.cos(driving.pose.yaw) * window.side * offset, context.navigation.floorY,
    window.point[2] - Math.sin(driving.pose.yaw) * window.side * offset];
  return admitCompanionPedestrianRoute({ ...route, navigation: context.navigation, actor: vendor, kind: "hawker",
    refuge: companionPedestrianPointRefuge(`${driving.id}:window:${window.side}`, point, vendor.radius), now: context.now });
}
/** The caller supplies accepted body/stock observations. This function never spawns a vendor or provisions stock. */
export function admitCompanionMovingHawkerOffer(context: CompanionMovingHawkerContext,
  request: { id: string; serviceId: string; offerKey: CompanionFoodKey; quantity: 1 | 3 | 6 },
  offers: readonly CompanionMovingHawkerOffer[]): CompanionMovingHawkerOffer | null {
  const item = companionFoodItem(request.offerKey), counter = companionFoodCounter("town-square", request.serviceId);
  if (!contextEligible(context) || !keyValid(request.id) || !item || !["suya", "akara"].includes(request.offerKey)
    || !counter || counter.npcId !== context.vendor.npcId || !counter.offers.some(key => key === request.offerKey)
    || ![1, 3, 6].includes(request.quantity) || context.stock.serviceKey !== companionFoodServiceKey("town-square", request.serviceId, request.offerKey)
    || context.stock.remaining < request.quantity || context.stock.remaining > item.stock || offers.length >= companionMovingHawkerRules.maxOffers
    || offers.some(offer => offer.id === request.id || offer.status === "offered" && offer.expiresAt > context.now
      && (offer.vendorId === context.vendor.id || offer.vehicleId === context.driving.vehicleId))) return null;
  const d = context.driving;
  return Object.freeze({ id: request.id, drivingId: d.id, vehicleId: d.vehicleId, driverId: d.driver.userId, gameRoomId: d.gameRoomId,
    instanceId: d.instanceId, layoutRevision: d.navigationRevision, lifeId: d.driver.lifeId, visitId: context.visitId,
    vendorId: context.vendor.id, npcId: context.vendor.npcId, vendorRevision: context.vendor.revision, serviceId: request.serviceId,
    bottleneckId: context.bottleneck.id, offerKey: request.offerKey, quantity: request.quantity, stockKey: context.stock.serviceKey,
    stockRevision: context.stock.revision, amount: item.amount * request.quantity, unitKey: "lagos-credit",
    issuedAt: context.now, expiresAt: context.now + companionMovingHawkerRules.offerMs, status: "offered", revision: 0,
    windowSide: context.window.side, windowHalfWidth: context.window.halfWidth, windowOffset: context.window.longitudinalOffset });
}
export function movingHawkerOfferStillEligible(offer: CompanionMovingHawkerOffer, context: CompanionMovingHawkerContext): boolean {
  const d = context.driving, counter = companionFoodCounter("town-square", offer.serviceId), item = companionFoodItem(offer.offerKey);
  return !!item && !!counter && ["suya", "akara"].includes(offer.offerKey) && counter.npcId === offer.npcId && counter.offers.some(k => k === offer.offerKey)
    && contextEligible(context, true, true) && offer.status === "offered" && offer.issuedAt <= context.now && context.now < offer.expiresAt
    && offer.expiresAt - offer.issuedAt === companionMovingHawkerRules.offerMs && offer.drivingId === d.id && offer.vehicleId === d.vehicleId
    && offer.driverId === d.driver.userId && offer.gameRoomId === d.gameRoomId && offer.instanceId === d.instanceId
    && offer.lifeId === d.driver.lifeId && offer.layoutRevision === d.navigationRevision && offer.visitId === context.visitId
    && offer.vendorId === context.vendor.id && offer.npcId === context.vendor.npcId && offer.vendorRevision === context.vendor.revision
    && offer.bottleneckId === context.bottleneck.id && offer.stockKey === context.stock.serviceKey && offer.stockRevision === context.stock.revision
    && context.stock.remaining >= offer.quantity && [1, 3, 6].includes(offer.quantity) && offer.amount === item.amount * offer.quantity
    && offer.unitKey === "lagos-credit" && offer.windowSide === context.window.side && offer.windowHalfWidth === context.window.halfWidth
    && offer.windowOffset === context.window.longitudinalOffset;
}
