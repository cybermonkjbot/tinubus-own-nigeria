import { companionAssetEntries } from "./companionAssets";
import { routeSegmentClear, type ScenePoint } from "./companionNavigation";
import type { CompanionPlaceAssetPart, CompanionPlaceNpc, CompanionPlaceService } from "./companionPlaces";
import { sampleCompanionResidentInPlace, type CompanionResidentPlaceBinding, type CompanionResidentWorldView } from "./companionResidentSimulation";
import type { CompanionWorldInstance } from "./companionWorld";
import { sampleCompanionWorldClock } from "./companionWorldClock";
import { companionResidentKeys } from "./companionResidents";
import { townSquareNavigation, townSquareStanding } from "./companionTownSquare";
import { companionCityNavigation, COMPANION_CITY_VERSION, isCompanionCity } from "./companionCity";

type VendorId = "mai-suya" | "akara-woman";
const point = (x: number, z: number): ScenePoint => Object.freeze([x, 0, z] as const);
const pose = (value: ScenePoint) => Object.freeze({ point: value, yaw: -Math.PI / 2 });
function vendor(npcId: VendorId, label: string, body: ScenePoint, work: ScenePoint,
  approach: ScenePoint, offers: readonly string[], bounds: CompanionPlaceService["footprint"]) {
  const service: CompanionPlaceService = Object.freeze({ id: `${npcId}-food`, label, kind: "food", npcId,
    anchor: pose(body), approach: pose(approach), footprint: Object.freeze(bounds), capacity: 1,
    queue: Object.freeze([]), offers: Object.freeze([...offers]) });
  const asset: CompanionPlaceAssetPart & { npcId: VendorId; key: "bestieTownSquareNpcs"; sourceYaw: number } = Object.freeze({ id: `${npcId}-authored`, key: "bestieTownSquareNpcs",
    npcId, point: body, yaw: -Math.PI / 2, sourceYaw: -Math.PI / 2 });
  const npc: CompanionPlaceNpc = Object.freeze({ id: npcId, role: "vendor", label, home: pose(work),
    route: Object.freeze([work]), refuge: work, radius: .3, serviceIds: Object.freeze([service.id]), assetPartId: asset.id });
  return Object.freeze({ service, npc, asset });
}

/** Authored v07 roots and stand footprints. Logical work points are beside the
 * equipment, not the rig origin inside collision. No district is regenerated. */
export const companionTownSquareVendors = Object.freeze([
  vendor("mai-suya", "Suya", point(-4.12, -23.55), point(-2.8, -23.55), point(-2.8, -22.15), ["suya"],
    { minX: -5.06, maxX: -3.5, minZ: -25, maxZ: -22.6 }),
  vendor("akara-woman", "Akara", point(-3.96, 16.5), point(-2.8, 16.5), point(-2.8, 17.95), ["akara"],
    { minX: -5.06, maxX: -3.5, minZ: 15.1, maxZ: 17.65 }),
]);
export const companionTownSquareVendor = (npcId: string) => companionTownSquareVendors.find(value => value.npc.id === npcId) ?? null;
export const companionTownSquareService = (serviceId: string) => companionTownSquareVendors.find(value => value.service.id === serviceId)?.service ?? null;

/** Bisi may retain a deliberately configured street post. The source roster's
 * separate seller leaves her existing Mama put job and possessions unchanged. */
export function companionTownSquareVendorAllowsResident(npcId: string, actorKey: string): boolean {
  return npcId === "mai-suya" ? actorKey === "mai-suya"
    : npcId === "akara-woman" && (actorKey === "akara-woman" || actorKey === "street-akara-vendor");
}

/** Full finite JSON comparison. Cycle/depth checks protect malformed input, not storage capacity. */
function sameFrozenValue(a: unknown, b: unknown): boolean {
  const activeA = new Set<object>(), activeB = new Set<object>();
  function same(left: unknown, right: unknown, depth: number): boolean {
    if (depth > 32) return false;
    if (left === null || typeof left !== "object") {
      if (left !== right || !["string", "boolean", "number"].includes(typeof left) && left !== null
        || typeof left === "number" && !Number.isFinite(left)) return false;
      return true;
    }
    if (!right || typeof right !== "object" || activeA.has(left) || activeB.has(right)) return false;
    const array = Array.isArray(left);
    if (array !== Array.isArray(right)) return false;
    if (!array && [left, right].some(value => ![Object.prototype, null].includes(Object.getPrototypeOf(value)))) return false;
    activeA.add(left); activeB.add(right);
    let equal: boolean;
    if (array && Array.isArray(right)) {
      equal = left.length === right.length;
      for (let index = 0; equal && index < left.length; index++) equal = same(left[index], right[index], depth + 1);
    } else {
      const l = left as Record<string, unknown>, r = right as Record<string, unknown>;
      const keys = Object.keys(l).filter(key => l[key] !== undefined).sort();
      const other = Object.keys(r).filter(key => r[key] !== undefined).sort();
      equal = keys.length === other.length;
      for (let index = 0; equal && index < keys.length; index++) {
        equal = keys[index] === other[index] && same(l[keys[index]], r[keys[index]], depth + 1);
      }
    }
    activeA.delete(left); activeB.delete(right);
    return equal;
  }
  try { return same(a, b, 0); } catch { return false; }
}
const validKey = (value: unknown): value is string => typeof value === "string" && value.length > 0 && value.length <= 240 && value.trim() === value;
const finitePoint = (value: unknown): value is ScenePoint => Array.isArray(value) && value.length === 3 && value.every(Number.isFinite);
export function companionTownSquareInstanceIsCompatible(instance: CompanionWorldInstance): boolean {
  const city = isCompanionCity(instance?.navigation);
  const navigation = city ? companionCityNavigation(instance.navigation.revision) : townSquareNavigation;
  const generation = city ? COMPANION_CITY_VERSION : "town-square-v06";
  if (!instance || instance.placeId !== "town-square" || instance.layout !== null || instance.room !== null
    || ![instance.id, instance.layoutId, instance.seed, instance.generatorVersion].every(validKey)
    || instance.seed !== generation || instance.generatorVersion !== generation
    || instance.layoutId !== navigation.revision || instance.navigation?.revision !== navigation.revision
    || !Number.isSafeInteger(instance.sharedStartedAt) || instance.sharedStartedAt < 0
    || !finitePoint(instance.exit) || !Array.isArray(instance.spawns) || instance.spawns.length > 64
    || !instance.spawns.length || !instance.spawns.every(finitePoint)) return false;
  const spawns = townSquareStanding.map(at => [at.x, townSquareNavigation.floorY, at.z]);
  return companionAssetEntries.bestieTownSquare.published && companionAssetEntries.bestieTownSquareNpcs.published
    && sameFrozenValue(instance.navigation, navigation) && sameFrozenValue(instance.spawns, spawns)
    && sameFrozenValue(instance.exit, spawns[0])
    && companionTownSquareVendors.every(value => routeSegmentClear(instance.navigation, value.npc.home.point, value.service.approach.point));
}

/** Companion to the generated-layout resolver. Compare the complete frozen
 * authored instance except its enclosing ID and shared start. */
export function resolveCompanionTownSquareResidentBinding(resident: CompanionWorldInstance,
  game: CompanionWorldInstance): CompanionResidentPlaceBinding | null {
  if (!companionTownSquareInstanceIsCompatible(resident) || !companionTownSquareInstanceIsCompatible(game)) return null;
  // Both versions independently validate the retained vendor geometry. The city
  // adds surrounding streets without relocating the existing resident work points.
  if (resident.navigation.revision !== game.navigation.revision) return Object.freeze({
    placeId: "town-square", layoutId: game.layoutId, residentInstanceId: resident.id, gameInstanceId: game.id });
  const { id: residentInstanceId, sharedStartedAt: residentStart, ...a } = resident;
  const { id: gameInstanceId, sharedStartedAt: gameStart, ...b } = game;
  void residentStart; void gameStart;
  return sameFrozenValue(a, b) ? Object.freeze({ placeId: "town-square", layoutId: game.layoutId,
    residentInstanceId, gameInstanceId }) : null;
}

export type CompanionTownSquareResident = Readonly<{
  residentId: string; revision: number; gameMinute: number; npcId: "mai-suya" | "akara-woman";
  serviceId: string; visible: boolean; available: boolean;
}>;
/** Read-only work presentation. Neither visible arrival nor a clip opens a
 * service before the resident owner commits settled work at the bound stand. */
export function sampleCompanionTownSquareResidents(world: CompanionResidentWorldView, instance: CompanionWorldInstance,
  sharedNow: number): { residents: readonly CompanionTownSquareResident[]; error: string | null } {
  const invalid = (error: string) => ({ residents: Object.freeze([]) as readonly CompanionTownSquareResident[], error });
  if (!companionTownSquareInstanceIsCompatible(instance) || !world || !world.clock || world.worldKey !== world.clock.worldKey
    || !Array.isArray(world.residents) || world.residents.length > companionResidentKeys.length
    || !Array.isArray(world.placeBindings) || world.placeBindings.length > companionResidentKeys.length * 2)
    return invalid("Refresh the Town Square resident snapshot and layout.");
  let gameMinute: number;
  try { gameMinute = sampleCompanionWorldClock(world.clock, sharedNow).gameMinute; }
  catch { return invalid("Use a current Town Square resident clock."); }
  const places = new Set<string>(), residentInstances = new Set<string>(), gameInstances = new Set<string>();
  for (const binding of world.placeBindings) {
    if (!binding || ![binding.placeId, binding.residentInstanceId, binding.gameInstanceId, binding.layoutId].every(validKey)
      || places.has(binding.placeId) || residentInstances.has(binding.residentInstanceId) || gameInstances.has(binding.gameInstanceId))
      return invalid("Use unique Town Square resident place bindings.");
    places.add(binding.placeId); residentInstances.add(binding.residentInstanceId); gameInstances.add(binding.gameInstanceId);
  }
  const bindings = world.placeBindings.filter(binding => binding.gameInstanceId === instance.id || binding.placeId === "town-square");
  if (bindings.length !== 1 || bindings[0].gameInstanceId !== instance.id || bindings[0].placeId !== "town-square"
    || bindings[0].layoutId !== instance.layoutId || !validKey(bindings[0].residentInstanceId)) return invalid("Refresh the Town Square resident place binding.");
  const ids = new Set<string>(), claims = new Set<string>(), residents: CompanionTownSquareResident[] = [];
  for (const resident of world.residents) {
    const identity = resident?.identity;
    if (!identity || !validKey(identity.id) || ids.has(identity.id) || identity.worldKey !== world.worldKey
      || !companionResidentKeys.includes(identity.actorKey)
      || identity.id !== `npc:${world.worldKey.length}:${world.worldKey}:${identity.actorKey}` || identity.actorKind !== "npc" || identity.rosterVersion !== 1
      || resident.clockEpochKey !== world.clock.epochKey || !Number.isSafeInteger(resident.revision) || resident.revision < 0
      || !Number.isSafeInteger(resident.updatedAtGameMinute) || resident.updatedAtGameMinute < 0 || resident.updatedAtGameMinute > gameMinute)
      return invalid("Use unique, current Town Square resident identities.");
    ids.add(identity.id);
    if (resident.serviceActor?.placeId !== "town-square") continue;
    const vendor = companionTownSquareVendor(resident.serviceActor.npcId), art = resident.renderAsset;
    if (!vendor || claims.has(vendor.npc.id) || !companionTownSquareVendorAllowsResident(vendor.npc.id, identity.actorKey) || identity.role !== vendor.npc.role
      || !identity.appearance || identity.appearance.assetNpcId !== vendor.asset.npcId || !art
      || art.key !== vendor.asset.key || art.npcId !== vendor.asset.npcId || (art.sourceYaw ?? 0) !== vendor.asset.sourceYaw
      || !(art.key in companionAssetEntries && companionAssetEntries[art.key as keyof typeof companionAssetEntries].published)) return invalid("Bind each Town Square vendor to its own published resident art.");
    claims.add(vendor.npc.id);
    const sampled = sampleCompanionResidentInPlace(resident, bindings[0], sharedNow);
    const visible = !!sampled?.working && !sampled.moving && resident.journey === null && resident.localMove === null
      && resident.activity.kind === "at-place"
      && Math.hypot(...sampled.pose.point.map((value, index) => value - vendor.npc.home.point[index])) <= .01;
    const available = visible && resident.activity.kind === "at-place" && resident.activity.interactionId === null;
    residents.push(Object.freeze({ residentId: identity.id, revision: resident.revision, gameMinute: resident.updatedAtGameMinute,
      npcId: vendor.asset.npcId, serviceId: vendor.service.id, visible, available }));
  }
  return { residents: Object.freeze(residents), error: null };
}
