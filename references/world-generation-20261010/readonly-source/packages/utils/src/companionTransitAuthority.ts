import { routeSegmentClear, type RoomNavigation } from "./companionNavigation";
import { companionCityTransitRoads } from "./companionCityTransitNetwork";
import { COMPANION_CITY_TRANSIT_VERSION, sampleTransitVehicle, transitNpcPassengers, transitRoadside,
  transitRoutePoint, transitRules, type TransitPassenger, type TransitPlan, type TransitRide, type TransitSample,
  type TransitSnapshot, type TransitVehicle, type TransitVehicleState } from "./companionCityTransit";

export type TransitCommand = { type: "board"; vehicleId: string } | { type: "hail"; kind: "danfo" | "keke" }
  | { type: "alight" };
export type TransitAdmission = {
  passengerId: string; instanceId: string; visitRevision: number; now: number; position: { x: number; z: number };
  /** Supplied by the existing authenticated world owner, never read from a client command. */
  allowed: boolean;
  /** Existing payment owner verifies this receipt for this passenger, vehicle and visit. */
  paidBoarding: { vehicleId: string; receiptId: string } | null;
};
export type TransitTransition = { snapshot: TransitSnapshot; ride: TransitRide | null;
  exit: { x: number; z: number; yaw: number } | null; notice: string | null };

export function initialTransitSnapshot(plan: TransitPlan, instanceId: string, epochMs: number): TransitSnapshot {
  if (!instanceId || instanceId.length > 240 || !Number.isSafeInteger(epochMs) || epochMs < 0) throw new Error("Invalid transit instance.");
  return { version: COMPANION_CITY_TRANSIT_VERSION, instanceId, navigationRevision: plan.navigationRevision,
    epochMs, revision: 0, states: [], passengers: [] };
}
export function admitTransitSnapshot(plan: TransitPlan, snapshot: TransitSnapshot) {
  if (snapshot.version !== plan.version || snapshot.navigationRevision !== plan.navigationRevision
    || !Number.isSafeInteger(snapshot.epochMs) || snapshot.epochMs < 0 || !Number.isSafeInteger(snapshot.revision) || snapshot.revision < 0
    || snapshot.states.length > transitRules.maxVehicles || snapshot.passengers.length > transitRules.maxPassengers) throw new Error("Refresh the shared transit state.");
  const identities = new Set<string>(), seats = new Set<string>();
  for (const passenger of snapshot.passengers) {
    const vehicle = plan.vehicles.find(value => value.id === passenger.ride.vehicleId);
    const seatKey = `${passenger.ride.vehicleId}:${passenger.ride.seat}`;
    if (!vehicle || !passenger.id || passenger.id.length > 240 || identities.has(passenger.id) || seats.has(seatKey)
      || !Number.isSafeInteger(passenger.ride.seat) || passenger.ride.seat < 0
      || passenger.ride.seat >= transitRules.kinds[vehicle.kind].seats
      || !Number.isSafeInteger(passenger.ride.boardedAt) || passenger.ride.boardedAt < snapshot.epochMs
      || !Number.isSafeInteger(passenger.ride.visitRevision) || passenger.ride.visitRevision < 0
      || passenger.kind === "player" && passenger.ride.seat >= transitRules.kinds[vehicle.kind].seats-transitRules.kinds[vehicle.kind].npcSeats
      || passenger.ride.exit && (!Number.isSafeInteger(passenger.ride.exit.at) || passenger.ride.exit.at < passenger.ride.boardedAt
        || !Object.values(passenger.ride.exit.point).every(Number.isFinite))) throw new Error("Passenger state needs review.");
    identities.add(passenger.id); seats.add(seatKey);
  }
  const vehicleIds = new Set<string>();
  for (const state of snapshot.states) {
    if (vehicleIds.has(state.vehicleId) || !plan.vehicles.some(vehicle => vehicle.id === state.vehicleId)
      || !Number.isSafeInteger(state.delayMs) || state.delayMs < 0 || state.delayMs > 4_102_444_800_000
      || state.pauses.length > transitRules.maxPauses) throw new Error("Vehicle state needs review.");
    vehicleIds.add(state.vehicleId);
    let end = snapshot.epochMs;
    for (const pause of state.pauses) {
      if (!Number.isSafeInteger(pause.startsAt) || !Number.isSafeInteger(pause.endsAt) || pause.startsAt < end
        || pause.endsAt - pause.startsAt !== transitRules.roadsideDwellMs
        || plan.vehicles.find(vehicle=>vehicle.id===state.vehicleId)?.kind === "brt"
        || pause.routeMetres<0 || pause.routeMetres>plan.routes.find(route=>route.id===plan.vehicles.find(vehicle=>vehicle.id===state.vehicleId)?.routeId)!.metres
        || ![pause.point.x, pause.point.z, pause.point.yaw, pause.routeMetres, ...pause.approach].every(Number.isFinite)) throw new Error("Roadside stops need review.");
      end = pause.endsAt;
    }
  }
}

/** Called by the world owner on an existing command/wakeup, not a per-frame worker. */
export function settleTransit(snapshot: TransitSnapshot, now: number): TransitSnapshot {
  const passengers = snapshot.passengers.filter(passenger => !passenger.ride.exit || now < passenger.ride.exit.at);
  let changed = passengers.length !== snapshot.passengers.length;
  const states = snapshot.states.map(state => {
    const completed = state.pauses.filter(pause => pause.endsAt <= now);
    if (!completed.length) return state;
    changed = true;
    return { ...state, delayMs: state.delayMs + completed.reduce((total, pause) => total + pause.endsAt - pause.startsAt, 0),
      pauses: state.pauses.filter(pause => pause.endsAt > now) };
  });
  return changed ? { ...snapshot, revision: snapshot.revision + 1, passengers, states } : snapshot;
}
export function projectTransitRide(ride: TransitRide | null | undefined, instanceRevision: number, now: number) {
  return ride?.visitRevision === instanceRevision && (!ride.exit || now < ride.exit.at) ? ride : null;
}
/** Apply these exits to matching player rows before releasing due seats in the same world transaction. */
export function dueTransitExits(snapshot:TransitSnapshot,now:number){
  return snapshot.passengers.flatMap(passenger=>passenger.kind==="player"&&passenger.ride.exit&&now>=passenger.ride.exit.at
    ?[{passengerId:passenger.id,rideId:passenger.ride.id,visitRevision:passenger.ride.visitRevision,point:passenger.ride.exit.point}]:[]);
}
function nextRouteArrival(plan: TransitPlan, vehicle: TransitVehicle, snapshot: TransitSnapshot, metres: number, now: number): number | null {
  const state = snapshot.states.find(value => value.vehicleId === vehicle.id);
  const route = plan.routes.find(value => value.id === vehicle.routeId)!;
  let at = now;
  // At most one loop, bounded by the number of dwells and pending roadside holds.
  for (let step = 0; step < route.stops.length * 2 + transitRules.maxPauses * 2 + 8; step++) {
    const sample = sampleTransitVehicle(plan, vehicle, snapshot.epochMs, at, state);
    if (!sample) return null;
    if (!sample.stopped && sample.metres <= metres) {
      const arrival = at + (metres - sample.metres) / sample.speed * 1000;
      if (arrival <= sample.phaseEndsAt) return Math.ceil(arrival);
    }
    if (sample.stopped && Math.abs(sample.metres - metres) < .1) return Math.ceil(at);
    at = Math.ceil(sample.phaseEndsAt) + 1;
    if (at - now > 900_000) return null;
  }
  return null;
}
function nearestRouteMetres(plan: TransitPlan, vehicle: TransitVehicle, point: { x: number; z: number }) {
  const route = plan.routes.find(value => value.id === vehicle.routeId)!;
  let best = { metres: 0, distance: Infinity };
  for (let i = 1; i < route.points.length; i++) {
    const a = route.points[i - 1], b = route.points[i], dx = b.x - a.x, dz = b.z - a.z, squared = dx * dx + dz * dz;
    const t = Math.max(0, Math.min(1, ((point.x - a.x) * dx + (point.z - a.z) * dz) / Math.max(.001, squared)));
    const d = Math.hypot(a.x + dx * t - point.x, a.z + dz * t - point.z);
    if (d < best.distance) best = { metres: route.lengths[i - 1] + t * (route.lengths[i] - route.lengths[i - 1]), distance: d };
  }
  return best;
}
function heldState(snapshot: TransitSnapshot, vehicle: TransitVehicle, pause: TransitVehicleState["pauses"][number], now: number) {
  if (vehicle.kind === "brt") throw new Error("BRTs stop only at BRT stops.");
  const current = snapshot.states.find(value => value.vehicleId === vehicle.id) ?? { vehicleId: vehicle.id, delayMs: 0, pauses: [] };
  if (current.pauses.length >= transitRules.maxPauses) throw new Error("This vehicle is finishing its booked stops. Choose another.");
  if (current.pauses.some(value => pause.startsAt < value.endsAt && pause.endsAt > value.startsAt)) throw new Error("This vehicle already has a roadside stop then.");
  if (current.pauses.some(value => value.endsAt > now)) throw new Error("This vehicle is finishing another pickup. Try after it departs.");
  return { ...snapshot, states: [...snapshot.states.filter(value => value.vehicleId !== vehicle.id),
    { ...current, pauses: [...current.pauses, pause].sort((a, b) => a.startsAt - b.startsAt) }] };
}
export function transitSampleForRide(plan: TransitPlan, snapshot: TransitSnapshot, ride: TransitRide, now: number): TransitSample | null {
  const vehicle = plan.vehicles.find(value => value.id === ride.vehicleId);
  return vehicle ? sampleTransitVehicle(plan, vehicle, snapshot.epochMs, now, snapshot.states.find(state => state.vehicleId === vehicle.id)) : null;
}

/** Pure transaction core. The caller persists the vehicle and player changes in ONE authenticated mutation. */
export function applyTransitCommand(plan: TransitPlan, navigation: RoomNavigation, retained: TransitSnapshot,
  admission: TransitAdmission, command: TransitCommand): TransitTransition {
  admitTransitSnapshot(plan, retained);
  if (!admission.allowed || admission.instanceId !== retained.instanceId || navigation.revision !== plan.navigationRevision
    || !admission.passengerId || admission.passengerId.length > 240 || !Number.isSafeInteger(admission.visitRevision)
    || !Number.isSafeInteger(admission.now) || admission.now < retained.epochMs
    || ![admission.position.x, admission.position.z].every(Number.isFinite)) throw new Error("Transport is unavailable for this visit.");
  const now = admission.now, snapshot = settleTransit(retained, now);
  const passenger = snapshot.passengers.find(value => value.id === admission.passengerId && value.kind === "player");
  const ride = passenger?.ride ?? null;
  if (ride && ride.visitRevision !== admission.visitRevision) throw new Error("Leave the previous ride before boarding again.");
  if (command.type === "hail") {
    if (ride) throw new Error("Leave the current vehicle before hailing another.");
    const roadside = transitRoadside(navigation, admission.position, command.kind, companionCityTransitRoads);
    if (!roadside || roadside.distance > transitRules.boardingReach) throw new Error("Walk to the road edge to hail a ride.");
    if(plan.stops.some(stop=>Math.hypot(roadside.approach[0]-stop.shelter.x,roadside.approach[2]-stop.shelter.z)<7))
      throw new Error("BRT stops are for BRTs. Hail from the road edge beyond the shelter.");
    const candidates = plan.vehicles.filter(vehicle => vehicle.kind === command.kind
      && plan.routes.find(route => route.id === vehicle.routeId)?.roadId === roadside.road.id);
    const pauseId = `hail:${admission.passengerId}:${admission.visitRevision}`;
    if (snapshot.states.some(state => state.pauses.some(pause => pause.id === pauseId && pause.endsAt > now)))
      return { snapshot, ride: null, exit: null, notice: "Your pickup is already booked." };
    const arrivals = candidates.filter(vehicle => !snapshot.states.find(state => state.vehicleId === vehicle.id)?.pauses.some(pause => pause.endsAt > now)).map(vehicle => {
      const nearest = nearestRouteMetres(plan, vehicle, roadside.point);
      return { vehicle, nearest, at: nextRouteArrival(plan, vehicle, snapshot, nearest.metres, now) };
    }).filter(value => value.at !== null && value.nearest.distance < 2).sort((a, b) => a.at! - b.at!);
    const selected = arrivals[0];
    if (!selected) throw new Error("No available vehicle serves this road.");
    const route = plan.routes.find(value => value.id === selected.vehicle.routeId)!;
    const next = heldState(snapshot, selected.vehicle, { id: pauseId, startsAt: selected.at!, endsAt: selected.at! + transitRules.roadsideDwellMs,
      point: transitRoutePoint(route, selected.nearest.metres), approach: roadside.approach, routeMetres: selected.nearest.metres }, now);
    return { snapshot: { ...next, revision: snapshot.revision + 1 }, ride: null, exit: null, notice: "Pickup booked." };
  }
  if (command.type === "board") {
    if (ride?.vehicleId === command.vehicleId) return { snapshot, ride, exit: null, notice: null };
    if (ride) throw new Error("Leave the current vehicle first.");
    const vehicle = plan.vehicles.find(value => value.id === command.vehicleId);
    const sample = vehicle && sampleTransitVehicle(plan, vehicle, snapshot.epochMs, now, snapshot.states.find(state => state.vehicleId === vehicle.id));
    if (!vehicle || !sample?.stopped || !sample.doorsOpen || !sample.approach || vehicle.kind === "brt" && !sample.stopId)
      throw new Error("Wait for the vehicle to stop and open its doors.");
    if (Math.hypot(admission.position.x - sample.approach[0], admission.position.z - sample.approach[2]) > transitRules.boardingReach
      || !routeSegmentClear(navigation, [admission.position.x, navigation.floorY, admission.position.z], sample.approach))
      throw new Error("Walk to the boarding point first.");
    if (!admission.paidBoarding?.receiptId || admission.paidBoarding.receiptId.length>240 || admission.paidBoarding.vehicleId !== vehicle.id) throw new Error("Confirm this ride's fare before boarding.");
    const rules = transitRules.kinds[vehicle.kind];
    const occupied = new Set(snapshot.passengers.filter(value => value.ride.vehicleId === vehicle.id).map(value => value.ride.seat));
    const seat = Array.from({ length: rules.seats - rules.npcSeats }, (_, index) => index).find(index => !occupied.has(index));
    if (seat === undefined || snapshot.passengers.length >= transitRules.maxPassengers) throw new Error("This vehicle is full.");
    const nextRide: TransitRide = { id: `ride:${admission.paidBoarding.receiptId}`, vehicleId: vehicle.id, seat,
      boardedAt: now, visitRevision: admission.visitRevision, exit: null };
    const nextPassenger: TransitPassenger = { id: admission.passengerId, kind: "player", ride: nextRide };
    return { snapshot: { ...snapshot, revision: snapshot.revision + 1, passengers: [...snapshot.passengers, nextPassenger] },
      ride: nextRide, exit: null, notice: null };
  }
  if (!ride) return { snapshot, ride: null, exit: null, notice: null };
  if (ride.exit) return { snapshot, ride, exit: null, notice: "Your stop is already requested." };
  const vehicle = plan.vehicles.find(value => value.id === ride.vehicleId)!;
  const sample = transitSampleForRide(plan, snapshot, ride, now);
  if (!sample) throw new Error("Refresh this ride.");
  let next = snapshot;
  let exit: NonNullable<TransitRide["exit"]>;
  if (vehicle.kind === "brt") {
    const route = plan.routes.find(value => value.id === vehicle.routeId)!;
    const candidates = route.stops.map(stop => ({ stop, at: nextRouteArrival(plan, vehicle, snapshot, stop.metres, now) }))
      .filter(value => value.at !== null && value.stop.stopId).sort((a, b) => a.at! - b.at!);
    const selected = candidates[0], stop = plan.stops.find(value => value.id === selected?.stop.stopId);
    if (!selected || !stop) throw new Error("This bus has no available next stop.");
    exit = { at: selected.at!, point: { x: stop.approach[0], z: stop.approach[2], yaw: stop.shelter.yaw }, stopId: stop.id };
  } else {
    const roadside = transitRoadside(navigation, sample.point, vehicle.kind, companionCityTransitRoads);
    if (!roadside || roadside.road.id !== plan.routes.find(value => value.id === vehicle.routeId)?.roadId)
      throw new Error("Wait until the vehicle reaches an accessible road edge.");
    if(plan.stops.some(stop=>Math.hypot(roadside.approach[0]-stop.shelter.x,roadside.approach[2]-stop.shelter.z)<7))
      throw new Error("Wait until the vehicle has passed the BRT stop.");
    const nearest = nearestRouteMetres(plan, vehicle, roadside.point);
    if (nearest.distance > 2) throw new Error("Wait until the vehicle reaches an accessible road edge.");
    // Stop at the current along-road coordinate. No ejection into moving traffic or a remote venue.
    const at = now;
    if (!sample.stopped) next = heldState(snapshot, vehicle, { id: `alight:${ride.id}`, startsAt: at,
      endsAt: at + transitRules.roadsideDwellMs, point: sample.point, approach: roadside.approach, routeMetres: sample.metres }, now);
    exit = { at, point: { x: roadside.approach[0], z: roadside.approach[2], yaw: roadside.point.yaw }, stopId: null };
  }
  const exiting = { ...ride, exit };
  next = { ...next, revision: next.revision + 1,
    passengers: next.passengers.map(value => value.id === passenger!.id ? { ...value, ride: exiting } : value) };
  return { snapshot: settleTransit(next, now), ride: now >= exit.at ? null : exiting,
    exit: now >= exit.at ? exit.point : null, notice: now >= exit.at ? null : "Next stop requested." };
}

/** Scene-facing NPC occupancy is deterministic and cannot grant money, items or resident outcomes. */
export function projectTransitPassengers(plan: TransitPlan, snapshot: TransitSnapshot, now: number) {
  return [...settleTransit(snapshot, now).passengers, ...transitNpcPassengers(plan, plan.vehicles, snapshot.epochMs, now, snapshot.states)]
    .slice(0, transitRules.maxPassengers);
}
/** World departure, death, account switch and lease expiry release the seat in their existing transaction. */
export function releaseTransitPassenger(snapshot: TransitSnapshot, passengerId: string) {
  return { ...snapshot, revision: snapshot.revision + 1, passengers: snapshot.passengers.filter(value => value.id !== passengerId) };
}
