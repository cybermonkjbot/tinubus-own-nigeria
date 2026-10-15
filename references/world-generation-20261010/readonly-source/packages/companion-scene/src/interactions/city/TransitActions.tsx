import { sampleTransitVehicle, transitRoadside, transitRules, type TransitPlan, type TransitSnapshot, type TransitRide } from "../../../../../packages/utils/src/companionCityTransit";
import type { RoomNavigation } from "../../../../../packages/utils/src/companionNavigation";
import type { TransitCommand } from "../../../../../packages/utils/src/companionTransitAuthority";
import { companionCityTransitRoads } from "../../../../../packages/utils/src/companionCity";
import type { CompanionWorldSnapshot } from "../../../../../packages/utils/src/companionWorld";
import { companionTransitFares } from "../../../../../packages/utils/src/companionTransitFares";
import "./transit.css";

/** Existing frame owner supplies time/position. Commands stay with the host's service flow. */
export function TransitActions({ plan, snapshot, navigation, position, ride, now, busy, available, availability, error, onCommand }: {
  plan: TransitPlan; snapshot: TransitSnapshot | null; navigation: RoomNavigation; position: { x:number; z:number };
  ride: TransitRide | null; now:number; busy:boolean; available:boolean; error?:string|null;
  availability: CompanionWorldSnapshot["transitAvailability"];
  onCommand(command:TransitCommand):void;
}) {
  if(!available||!snapshot||snapshot.navigationRevision!==navigation.revision||snapshot.navigationRevision!==plan.navigationRevision)return null;
  const aboard=!!ride;
  const atBrtStop=plan.stops.some(stop=>Math.hypot(position.x-stop.shelter.x,position.z-stop.shelter.z)<9
    ||Math.hypot(position.x-stop.approach[0],position.z-stop.approach[2])<=transitRules.boardingReach);
  const unavailable=availability?.boarding==="unavailable"?availability:null;
  const faresAvailable=availability?.boarding==="available"&&availability.farePolicy===companionTransitFares.version;
  const nearest=new Map<TransitPlan["vehicles"][number]["kind"],{sample:NonNullable<ReturnType<typeof sampleTransitVehicle>>;distance:number}>();
  if(!aboard&&faresAvailable){
    const states=new Map(snapshot.states.map(state=>[state.vehicleId,state]));
    for(const vehicle of plan.vehicles){
      const sample=sampleTransitVehicle(plan,vehicle,snapshot.epochMs,now,states.get(vehicle.id));
      if(sample?.doorsOpen&&sample.approach){
        const distance=Math.hypot(position.x-sample.approach[0],position.z-sample.approach[2]);
        if(distance<=transitRules.boardingReach&&distance<(nearest.get(sample.kind)?.distance??Infinity))nearest.set(sample.kind,{sample,distance});
      }
    }
  }
  const boardable=[...nearest.values()].sort((a,b)=>a.distance-b.distance).map(value=>value.sample);
  const roadside=!aboard?transitRoadside(navigation,position,"danfo",companionCityTransitRoads):null;
  const hailKinds=(["danfo","keke"] as const).filter(kind=>!nearest.has(kind)&&plan.vehicles.some(vehicle=>vehicle.kind===kind&&plan.routes.some(route=>route.id===vehicle.routeId&&route.roadId===roadside?.road.id)));
  const roadsideAvailable=!!roadside&&roadside.distance<=transitRules.boardingReach&&hailKinds.length>0;
  if(!aboard&&!boardable.length&&!(!faresAvailable&&atBrtStop)&&!roadsideAvailable)return null;
  return <section className="city-transit-actions" aria-label="Transport" onPointerDown={event=>event.stopPropagation()}
    onPointerMove={event=>event.stopPropagation()} onPointerUp={event=>event.stopPropagation()} onKeyDown={event=>event.stopPropagation()} onKeyUp={event=>event.stopPropagation()}>
    {aboard&&ride?<button type="button" disabled={busy||!!ride.exit} onClick={()=>onCommand({type:"alight"})}>
      {ride.exit?"Stop requested":plan.vehicles.find(vehicle=>vehicle.id===ride.vehicleId)?.kind==="brt"?"Get off at next stop":"Get off"}
    </button>:!faresAvailable?<p role="status">{unavailable?.message??"Transit fares are loading."}</p>:<>
      {boardable.map(sample=><button key={sample.vehicleId} type="button" disabled={busy} onClick={()=>onCommand({type:"board",vehicleId:sample.vehicleId})}>
        Board {sample.kind==="brt"?"BRT":sample.kind} · {companionTransitFares.amounts[sample.kind]} credits
      </button>)}
      {roadsideAvailable&&hailKinds.map(kind=><button key={kind} type="button" disabled={busy} onClick={()=>onCommand({type:"hail",kind})}>Hail {kind}</button>)}
    </>}
    {error&&error!==unavailable?.message?<p role="alert">{error}</p>:null}
  </section>;
}
