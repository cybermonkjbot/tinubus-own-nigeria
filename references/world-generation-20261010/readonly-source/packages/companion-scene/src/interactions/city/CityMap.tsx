import { memo, useCallback, useMemo, useRef, useState } from "react";
import type { CompanionWorldSnapshot } from "../../../../../packages/utils/src/companionWorld";
import { isCompanionCity, companionCityHasParkPaths, companionCityHasTheatre, companionCityHasRefinery, companionCityHasStreetDensity, companionCityTransitPlan } from "../../../../../packages/utils/src/companionCity";
import { companionCityResidenceEntrance, companionCityResidenceLabel } from "../../../../../packages/utils/src/companionCityHousing";
import { companionCityInterestingPlaces, companionCityMapFacing, companionCityMapLandmarks, companionCityMapLocation,
  resolveCompanionFriendResidences, companionCityTheatreMapPlace, companionCityRefineryMapPlace, companionCityMiniParkMapPlaces } from "../../../../../packages/utils/src/companionCityMap";
import { companionCityPlaceDestinations } from "../../../../../packages/utils/src/companionPlacesExpansion";
import { MobileScroll } from "../../mobile";
import { CityMapOverlay } from "./CityMapOverlay";
import { CityMapDrawing, FriendHomeGlyph, HomeGlyph } from "./CityMapDrawing";
import { acceptedCityMapMovement, useCityMapPosition } from "./useCityMapPosition";
import { cityWaypointDistance, cityWaypointReached, type CityWaypoint } from "./cityWaypoint";
import "./city-map.css";

type CityMapProps = {
  world: CompanionWorldSnapshot;
  participants?: readonly { userId: string; displayName?: string }[];
  transitPosition?: { x: number; z: number; yaw: number };
  waypoint: CityWaypoint | null;
  onWaypoint(waypoint: CityWaypoint | null): void;
  onOpen(): void;
};

/** Local game geography and guidance. Pin selection never issues movement. */
export const CityMap = memo(function CityMap({ world, participants = [], transitPosition, waypoint, onWaypoint, onOpen }: CityMapProps) {
  const player = world.player;
  const instance = world.instance;
  if (!player || !instance || player.instanceId !== instance.id || !isCompanionCity(instance.navigation)) return null;
  return <CityMapHud key={`${player.userId}:${player.gameRoomId}:${player.instanceId}:${player.revision}`} world={world} participants={participants} transitPosition={transitPosition} waypoint={waypoint} onWaypoint={onWaypoint} onOpen={onOpen} />;
});

function CityMapHud({ world, participants = [], transitPosition, waypoint, onWaypoint, onOpen }: CityMapProps) {
  const [open, setOpen] = useState(false), [citywide, setCitywide] = useState(true);
  const hud = useRef<HTMLButtonElement>(null);
  const [hudNode, setHudNode] = useState<HTMLButtonElement | null>(null);
  const [sheetNode, setSheetNode] = useState<HTMLDivElement | null>(null);
  const bindHud = useCallback((node: HTMLButtonElement | null) => { hud.current = node; setHudNode(node); }, []);
  const walkingPosition = useCityMapPosition(world, transitPosition ? null : open ? sheetNode : hudNode);
  const at = transitPosition ?? walkingPosition;
  const player = world.player!;
  const home = companionCityResidenceEntrance(player.residence);
  const movement = player.transitRide ? null : acceptedCityMapMovement(world);
  const cellX = Math.round(at.x / 8), cellZ = Math.round(at.z / 8);
  const theatreDistrict = companionCityHasTheatre(world.instance!.navigation);
  const refineryDistrict = companionCityHasRefinery(world.instance!.navigation);
  const streetDensity = companionCityHasStreetDensity(world.instance!.navigation);
  const transitPlan = world.transit?.instanceId === world.instance!.id&&world.transit.navigationRevision===world.instance!.navigation.revision
    ? world.instance!.transitPlan ?? companionCityTransitPlan(world.instance!.navigation) : null;
  const location = useMemo(() => companionCityMapLocation(cellX * 8, cellZ * 8, theatreDistrict, refineryDistrict), [cellX, cellZ, theatreDistrict, refineryDistrict]);
  const homeDistance = home ? Math.round(Math.hypot(home[0] - at.x, home[2] - at.z)) : null;
  const homeLabel = home && player.residence ? companionCityResidenceLabel(player.residence) : null;
  const waypointDistance = waypoint ? Math.hypot(waypoint.x - at.x, waypoint.z - at.z) : null;
  const waypointDetail = waypointDistance === null ? "" : cityWaypointReached(waypointDistance) ? "At waypoint" : cityWaypointDistance(waypointDistance);
  const friends = useMemo(() => {
    return resolveCompanionFriendResidences({
      friends: world.friendResidences,
      peers: world.peers,
      participants,
      excludeUserId: player.userId,
    }).map((friend, index) => ({
      ...friend,
      number: index + 1,
      distance: Math.round(Math.hypot(friend.x - at.x, friend.z - at.z)),
    }));
  }, [world.friendResidences, world.peers, participants, player.userId, at.x, at.z]);
  const interestingPlaces = useMemo(() => {
    return [...companionCityInterestingPlaces, ...(theatreDistrict ? [companionCityTheatreMapPlace] : []), ...(refineryDistrict ? [companionCityRefineryMapPlace] : []), ...(streetDensity ? companionCityMiniParkMapPlaces() : [])].map(place => ({
      ...place,
      distance: Math.round(Math.hypot(place.x - at.x, place.z - at.z)),
    })).filter(place => citywide || Math.abs(place.x - at.x) < 600 && Math.abs(place.z - at.z) < 600);
  }, [citywide, at.x, at.z, theatreDistrict, refineryDistrict, streetDensity]);
  const venues = companionCityPlaceDestinations(world.instance!.navigation);
  const parkPaths = companionCityHasParkPaths(world.instance!.navigation);
  const closestFriend = friends.length > 0 ? friends.reduce((min, curr) => curr.distance < min.distance ? curr : min) : null;
  const friendStatusText = closestFriend && closestFriend.distance < 300 ? ` Nearby: ${closestFriend.label} (${closestFriend.distance} m).` : "";
  const positionLabel = `${location.street}${location.street === location.district ? "" : `, ${location.district}`}. Facing ${companionCityMapFacing(at.yaw)}.${homeDistance === null ? "" : ` Home ${homeDistance} metres away.`}${friendStatusText}${waypoint ? ` Waypoint: ${waypoint.label}, ${waypointDetail}.` : ""}`;
  const openMap = () => {
    if (open) return;
    // onOpen belongs to the intentional opening event only. The caller owns stopping movement.
    onOpen(); setCitywide(true); setOpen(true);
  };
  const closeMap = () => { setOpen(false); hud.current?.focus({ preventScroll: true }); };
  const north = <svg viewBox="0 0 26 30" className="city-map-north" aria-hidden="true" focusable="false">
    <path d="M9 10 13 3 17 10M13 4V15" fill="none" stroke="currentColor" strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round" />
    <text x="13" y="27" textAnchor="middle" fill="currentColor" fontSize="11" fontWeight="700">N</text>
  </svg>;
  const mapContents = <>
    {waypoint ? <div className="city-map-waypoint-status">
      <span><strong>{waypoint.label}</strong><small>{waypointDetail}</small></span>
      <button type="button" onClick={() => onWaypoint(null)} aria-label="Clear waypoint">Clear</button>
    </div> : null}
    <div className="city-map-bearing" aria-hidden="true">{north}</div>
    <div ref={setSheetNode} className="city-map-expanded" role="group"
      aria-label={`${citywide ? "Whole city" : "Nearby"} roads, walking paths, shoreline, landmarks, venues${transitPlan?.stops.length ? ", BRT stops" : ""} and friends' homes. North is up. ${positionLabel}`}>
      <CityMapDrawing at={at} home={home} movement={movement} venues={venues} friends={friends} interestingPlaces={interestingPlaces} citywide={citywide} expanded parkPaths={parkPaths} theatreDistrict={theatreDistrict} refineryDistrict={refineryDistrict} streetDensity={streetDensity} transitPlan={transitPlan} waypoint={waypoint} onWaypoint={onWaypoint} />
    </div>
    <p className="city-map-location">{location.street}{location.street === location.district ? "" : ` · ${location.district}`}</p>
    <div className="city-map-key">
      <span><svg viewBox="-12 -12 24 24" aria-hidden="true"><path d="M0-9 7 0 0 9-7 0Z" fill="#8551d0" /></svg>Waypoint</span>
      <span><svg viewBox="-12 -12 24 24" aria-hidden="true"><path d="M-5-3 0 7 5-3Z" fill="#245b77" /></svg>Position and facing</span>
      {homeLabel ? <span><svg viewBox="-12 -12 24 24" aria-hidden="true"><HomeGlyph /></svg>Home</span> : null}
      {friends.length > 0 ? <span><svg viewBox="-12 -12 24 24" aria-hidden="true"><circle r="8" fill="#fff0f4" stroke="#a03b57" strokeWidth="1.5" /><FriendHomeGlyph /></svg>Friends</span> : null}
      <span><svg viewBox="-12 -12 24 24" aria-hidden="true"><polygon points="-7,-3 0,-8 7,-3 7,5 0,8 -7,5" fill="#faf6e9" stroke="#486858" strokeWidth="1.5" /></svg>Interesting places</span>
      <span><svg viewBox="-12 -12 24 24" aria-hidden="true"><rect x="-6" y="-5" width="12" height="10" rx="3" fill="#faf6e9" stroke="#596a51" /></svg>Venues</span>
      <span><svg viewBox="-12 -12 24 24" aria-hidden="true"><path d="M-5 8V-7H6L3-2H-5" fill="none" stroke="#4d3d72" strokeWidth="2" /></svg>Mission pickups</span>
      {transitPlan?.stops.length ? <span><svg viewBox="-12 -12 24 24" aria-hidden="true"><rect x="-8" y="-6" width="16" height="12" rx="3" fill="#155584" /></svg>BRT stops</span> : null}
    </div>
    {homeLabel && home ? <ul className="city-map-landmarks"><li><button type="button" aria-label="Set waypoint to Home"
      aria-pressed={waypoint?.id === "home"} onClick={() => onWaypoint({ id: "home", label: "Home", x: home[0], z: home[2] })}>
      <span aria-hidden="true"><svg viewBox="-12 -12 24 24" width="24" height="24"><HomeGlyph /></svg></span>
      Home · {homeLabel} · {homeDistance} m
    </button></li></ul> : null}
    {friends.length > 0 ? <>
      <p className="city-map-section-title">Friends' homes</p>
      <ol className="city-map-landmarks city-map-friends">
        {friends.map(friend => <li key={friend.userId}><button type="button" aria-label={`Set waypoint to ${friend.label}`}
          aria-pressed={waypoint?.id === `friend:${friend.userId}`} onClick={() => onWaypoint({ id: `friend:${friend.userId}`, label: friend.label, x: friend.x, z: friend.z })}>
          <span className="city-map-friend-badge" aria-hidden="true">F{friend.number}</span>{friend.label} · {friend.address} · {friend.distance} m
        </button></li>)}
      </ol>
    </> : null}
    {transitPlan?.stops.length ? <>
      <p className="city-map-section-title">BRT stops</p>
      <ol className="city-map-landmarks">
        {[...transitPlan.stops].sort((a,b)=>Math.hypot(a.shelter.x-at.x,a.shelter.z-at.z)-Math.hypot(b.shelter.x-at.x,b.shelter.z-at.z))
          .slice(0,6).map(stop=><li key={stop.id}><button type="button" aria-label={`Set waypoint to ${stop.label}`}
            aria-pressed={waypoint?.id === `stop:${stop.id}`} onClick={() => onWaypoint({ id: `stop:${stop.id}`, label: stop.label, x: stop.shelter.x, z: stop.shelter.z })}>
            <span aria-hidden="true">BRT</span>{stop.label} · {Math.round(Math.hypot(stop.shelter.x-at.x,stop.shelter.z-at.z))} m
          </button></li>)}
      </ol>
    </> : null}
    <p className="city-map-section-title">Interesting places</p>
    <ol className="city-map-landmarks">
      {interestingPlaces.map(place => <li key={place.id}><button type="button" aria-label={`Set waypoint to ${place.name}`}
        aria-pressed={waypoint?.id === place.id} onClick={() => onWaypoint({ id: place.id, label: place.name, x: place.x, z: place.z })}>
        <span aria-hidden="true">P{place.number || "P"}</span>{place.name} · {place.distance} m
      </button></li>)}
    </ol>
  </>;

  return <>
    <button ref={bindHud} type="button" className="city-map-hud" aria-label={`Open city map. ${positionLabel}`}
      aria-haspopup="dialog" aria-expanded={open} onClick={openMap}
      onPointerDown={event => event.stopPropagation()} onPointerMove={event => event.stopPropagation()}
      onPointerUp={event => event.stopPropagation()} onKeyDown={event => event.stopPropagation()} onKeyUp={event => event.stopPropagation()}>
      <CityMapDrawing at={at} home={home} movement={movement} venues={venues} friends={friends} interestingPlaces={interestingPlaces} parkPaths={parkPaths} theatreDistrict={theatreDistrict} refineryDistrict={refineryDistrict} streetDensity={streetDensity} transitPlan={transitPlan} waypoint={waypoint} />
      <span className="city-map-hud-label" aria-hidden="true"><span>{waypoint ? <><span className="city-map-hud-destination">{waypoint.label}</span><small className="city-map-hud-distance">{waypointDetail}</small></> : location.street}</span>{north}</span>
    </button>
    <CityMapOverlay open={open} onOpenChange={next => { if (next) openMap(); else closeMap(); }} trigger={hud}>
      {native => <div className="city-map-sheet" onPointerDown={event => event.stopPropagation()}
        onPointerMove={event => event.stopPropagation()} onPointerUp={event => event.stopPropagation()}
        onKeyDown={event => { if (event.key !== "Escape" && event.key !== "Tab") event.stopPropagation(); }}
        onKeyUp={event => event.stopPropagation()}>
        <div className="city-map-tools">
          <button type="button" onClick={() => setCitywide(false)} aria-pressed={!citywide}>Recenter</button>
          <button type="button" onClick={() => setCitywide(true)} aria-pressed={citywide}>Whole city</button>
          <button type="button" onClick={closeMap} aria-label="Close city map">Close</button>
        </div>
        {native ? <div className="city-map-native-scroll">{mapContents}</div> : <MobileScroll className="city-map-scroll">{mapContents}</MobileScroll>}
      </div>}
    </CityMapOverlay>
  </>;
}
