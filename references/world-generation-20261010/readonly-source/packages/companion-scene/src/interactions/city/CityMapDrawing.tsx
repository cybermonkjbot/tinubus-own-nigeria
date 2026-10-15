import { memo, useMemo, useRef, useState } from "react";
import type { CityWaypoint } from "./cityWaypoint";
import type { CompanionPosition } from "../../../../../packages/utils/src/companionCall";
import type { CompanionMovement } from "../../../../../packages/utils/src/companionMovement";
import { companionCityPlan as plan } from "../../../../../packages/utils/src/companionCity";
import { companionCityTheatre } from "../../../../../packages/utils/src/companionCityTheatre";
import { companionCityRefinery } from "../../../../../packages/utils/src/companionCityRefinery";
import type { TransitPlan } from "../../../../../packages/utils/src/companionCityTransit";
import { companionCityMapEdge, companionCityMapGeometry, companionCityMapGeometryIn, companionCityMapLandmarks,
  companionCityMapProject, type CompanionCityMapGeometry, type CompanionCityInterestingPlace, type CompanionFriendResidencePlacement } from "../../../../../packages/utils/src/companionCityMap";
import type { CompanionPlaceExpansionDestination } from "../../../../../packages/utils/src/companionPlacesExpansion";
import { cityMiniParkPoint } from "../../../../../packages/utils/src/companionCityMiniParks";
import { racingPoints, racingRules } from "../../../../../packages/utils/src/companionRacing";

const StaticGeometry = memo(function StaticGeometry({ geometry }: { geometry: CompanionCityMapGeometry }) {
  return <g>
    <polygon points={companionCityMapGeometry.water} fill="#95b7bd" />
    <polyline points={racingPoints.map(point => `${point[0] + racingRules.mapX},${point[2] + racingRules.mapZ}`).join(" ")}
      fill="none" stroke="#596260" strokeWidth={racingRules.halfWidth * 2} strokeLinejoin="round" />
    {geometry.neighbourhoods.map(surface => <polygon key={surface.id} points={surface.points} fill={surface.colour}
      stroke={surface.kind === "compound" || surface.kind === "forecourt" ? "#938c79" : undefined}
      strokeWidth=".7" />)}
    {geometry.beaches.map(beach => <polygon key={beach.id} points={beach.points} fill="#dfcda9" />)}
    {geometry.greens.map(space => <polygon key={space.id} points={space.points}
      fill={space.kind === "park" || space.kind === "garden" ? "#bdccac" : "#ded6c3"} />)}
    {geometry.lakes.map(lake => <polygon key={lake.id} points={lake.points} fill="#95b7bd" stroke="#688c94" strokeWidth=".7" />)}
    <g fill="none" stroke="#688c94" strokeWidth="1.2">
      {companionCityMapGeometry.banks.map((bank, index) => <polyline key={index} points={bank} vectorEffect="non-scaling-stroke" />)}
    </g>
    <g fill="none" strokeLinecap="round" strokeLinejoin="round">
      {geometry.roads.map(road => <polyline key={road.id} points={road.points} stroke="#eeeade"
        strokeWidth={Math.max(road.width + road.pavement * 2, 3)} />)}
      {geometry.roads.map(road => <polyline key={road.id} points={road.points}
        stroke={road.kind === "boardwalk" ? "#8c775d" : road.kind === "path" || road.kind === "sand-path" ? "#b49b76" : "#92918a"}
        strokeWidth={Math.max(road.width, 1.5)} />)}
    </g>
    <g>{geometry.parcels.map(parcel => {
      const fill = parcel.kind === "estate" || parcel.kind === "villa" ? "#b7a68d" : parcel.kind === "tower" ? "#849a9d" : parcel.district === "south-market" ? "#ba9b7e" : "#b2b1a5";
      const transform = `translate(${parcel.x} ${parcel.z}) rotate(${-parcel.yaw * 180 / Math.PI})`;
      if (parcel.style === "courtyard") {
        const w = parcel.width, d = parcel.depth;
        return <polygon key={parcel.id} transform={transform} fill={fill} points={[
          [-w / 2, -d / 2], [w / 2, -d / 2], [w / 2, d / 2], [w * .2, d / 2],
          [w * .2, -d * .18], [-w * .2, -d * .18], [-w * .2, d / 2], [-w / 2, d / 2],
        ].map(point => point.join(",")).join(" ")} />;
      }
      return <rect key={parcel.id} x={-parcel.width / 2} y={-parcel.depth / 2}
        width={parcel.width} height={parcel.depth} rx="2" transform={transform} fill={fill} />;
    })}</g>
    {geometry.buildings.map(building => <polygon key={building.id} points={building.points} fill="#c5bea8" stroke="#7c8173" strokeWidth="2" />)}
    {geometry.miniParks.map(park => <g key={park.id}>
      <polygon points={[[-11, -9], [11, -9], [11, 9], [-11, 9]].map(([x, z]) => {
        const point = cityMiniParkPoint(park, x, z); return `${point[0]},${point[2]}`;
      }).join(" ")} fill="#c5b58a" stroke="#817c65" strokeWidth="1" />
      {park.buses.map(bus => { const at = cityMiniParkPoint(park, bus.x, bus.z); return <rect key={bus.id}
        x={at[0] - 1} y={at[2] - 2.4} width="2" height="4.8" fill="#d7ad32"
        transform={`rotate(${-(park.yaw + bus.yaw) * 180 / Math.PI} ${at[0]} ${at[2]})`} />; })}
    </g>)}
  </g>;
});

export function HomeGlyph() {
  return <path d="M-6 0 0-5 6 0V6H2V2H-2V6H-6Z" fill="#6a5b43" />;
}

export function FriendHomeGlyph() {
  return <path d="M-5 1 0-4 5 1V5H2V2H-2V5H-5Z" fill="#a03b57" />;
}

export function CityMapDrawing({ at, home, movement, venues = [], friends = [], interestingPlaces = [], citywide = false, expanded = false, parkPaths = false, theatreDistrict = false, refineryDistrict = false, streetDensity = false, transitPlan = null, waypoint = null, onWaypoint }: {
  at: CompanionPosition; home: readonly number[] | null; movement: CompanionMovement | null; citywide?: boolean; expanded?: boolean;
  parkPaths?: boolean;
  theatreDistrict?: boolean;
  refineryDistrict?: boolean;
  streetDensity?: boolean;
  transitPlan?: TransitPlan | null;
  waypoint?: CityWaypoint | null;
  onWaypoint?(waypoint: CityWaypoint): void;
  venues?: readonly CompanionPlaceExpansionDestination[];
  friends?: readonly (CompanionFriendResidencePlacement & { number: number })[];
  interestingPlaces?: readonly (CompanionCityInterestingPlace & { number: number })[];
}) {
  const press = useRef<{ id: number; x: number; y: number } | null>(null);
  const [cursor, setCursor] = useState<{ x: number; y: number } | null>(null);
  const width = expanded ? 480 : 180, height = expanded ? 480 : 90;
  const bounds = refineryDistrict ? companionCityRefinery.bounds : theatreDistrict ? companionCityTheatre.bounds : plan.bounds;
  const span = citywide ? Math.max(bounds.maxX - bounds.minX, bounds.maxZ - bounds.minZ) + 240 : expanded ? 1000 : 640;
  const center = citywide ? { x: (bounds.minX + bounds.maxX) / 2, z: (bounds.minZ + bounds.maxZ) / 2 } : at;
  const scale = width / span, halfHeight = height / scale / 2;
  // Retain a buffered static crop across small moves. Only the transform and position marker move at 10 Hz.
  const cellX = Math.floor(center.x / 80), cellZ = Math.floor(center.z / 80);
  const geometry = useMemo(() => companionCityMapGeometryIn({ minX: cellX * 80 - span / 2 - 80, maxX: cellX * 80 + span / 2 + 160,
    minZ: cellZ * 80 - halfHeight - 80, maxZ: cellZ * 80 + halfHeight + 160 },parkPaths,theatreDistrict,refineryDistrict,streetDensity), [cellX, cellZ, span, halfHeight,parkPaths,theatreDistrict,refineryDistrict,streetDensity]);
  const project = (point: { x: number; z: number }) => companionCityMapProject(point, center, span, width, height);
  const player = project(at);
  const waypointPin = waypoint ? companionCityMapEdge(project(waypoint), width, height, 18) : null;
  const homePin = home ? companionCityMapEdge(project({ x: home[0], z: home[2] }), width, height, 24) : null;
  const route = useMemo(() => movement ? [movement.from, ...movement.path].map(point => `${point.x},${point.z}`).join(" ") : null, [movement]);
  const landmarks = companionCityMapLandmarks.map(place => ({ ...place, ...project(place) }))
    .filter(place => place.x > 14 && place.x < width - 14 && place.y > 14 && place.y < height - 14
      && Math.hypot(place.x - player.x, place.y - player.y) > 17
      && (!homePin || Math.hypot(place.x - homePin.x, place.y - homePin.y) > 23));
  const scaleMetres = citywide ? 500 : 200;
  const venueCandidates = venues.map((place, index) => ({ ...place, number: index + 1,
    ...project({ x: place.city.entrance[0], z: place.city.entrance[2] }) }))
    .filter(place => place.x > 14 && place.x < width - 14 && place.y > 14 && place.y < height - 14
      && Math.hypot(place.x - player.x, place.y - player.y) > 17
      && (!homePin || Math.hypot(place.x - homePin.x, place.y - homePin.y) > 23)
      && !landmarks.some(landmark => Math.hypot(place.x - landmark.x, place.y - landmark.y) < 20));
  const venuePins: typeof venueCandidates = [];
  for (const place of venueCandidates) {
    if (!venuePins.some(pin => Math.hypot(pin.x - place.x, pin.y - place.y) < (expanded ? 28 : 10))) venuePins.push(place);
  }

  const friendPins = friends.map(friend => ({ ...friend, ...project({ x: friend.x, z: friend.z }) }))
    .filter(friend => friend.x > 14 && friend.x < width - 14 && friend.y > 14 && friend.y < height - 14
      && Math.hypot(friend.x - player.x, friend.y - player.y) > 16
      && (!homePin || Math.hypot(friend.x - homePin.x, friend.y - homePin.y) > 22));

  const interestingCandidates = interestingPlaces.map(place => ({ ...place, ...project({ x: place.x, z: place.z }) }))
    .filter(place => place.x > 14 && place.x < width - 14 && place.y > 14 && place.y < height - 14
      && Math.hypot(place.x - player.x, place.y - player.y) > 16
      && (!homePin || Math.hypot(place.x - homePin.x, place.y - homePin.y) > 22)
      && !landmarks.some(landmark => Math.hypot(place.x - landmark.x, place.y - landmark.y) < 18)
      && !venuePins.some(venue => Math.hypot(place.x - venue.x, place.y - venue.y) < 18)
      && !friendPins.some(friend => Math.hypot(place.x - friend.x, place.y - friend.y) < 18));
  const interestingPins: typeof interestingCandidates = [];
  for (const place of interestingCandidates) {
    if (!interestingPins.some(pin => Math.hypot(pin.x - place.x, pin.y - place.y) < (expanded ? 28 : 10))) interestingPins.push(place);
  }
  const stopCandidates = (transitPlan?.stops ?? []).map(stop => ({ ...stop, ...project(stop.shelter) }))
    .filter(stop => stop.x > 10 && stop.x < width - 10 && stop.y > 10 && stop.y < height - 10
      && Math.hypot(stop.x - player.x, stop.y - player.y) > 14 && (!homePin || Math.hypot(stop.x - homePin.x, stop.y - homePin.y) > 20)
      && ![...landmarks,...venuePins,...interestingPins,...friendPins].some(pin=>Math.hypot(stop.x-pin.x,stop.y-pin.y)<(expanded?22:10)))
    .sort((a,b)=>Math.hypot(a.x-player.x,a.y-player.y)-Math.hypot(b.x-player.x,b.y-player.y)||a.id.localeCompare(b.id));
  const stopPins: typeof stopCandidates = [];
  for (const stop of stopCandidates) if (!stopPins.some(pin => Math.hypot(pin.x-stop.x,pin.y-stop.y) < (expanded ? 16 : 9))) stopPins.push(stop);

  const selectPoint = (x: number, y: number, hitRadius: number) => {
    if (!onWaypoint) return;
    const targets = [
      ...(home ? [{ id: "home", label: "Home", x: home[0], z: home[2] }] : []),
      ...landmarks.map(place => { const source = companionCityMapLandmarks.find(value => value.id === place.id)!;
        return { id: source.id, label: source.name, x: source.x, z: source.z }; }),
      ...venuePins.map(place => ({ id: place.id, label: place.label, x: place.city.entrance[0], z: place.city.entrance[2] })),
      ...friendPins.map(friend => ({ id: `friend:${friend.userId}`, label: friend.label, x: friend.entrance[0], z: friend.entrance[2] })),
      ...interestingPins.map(place => { const source = interestingPlaces.find(value => value.id === place.id)!;
        return { id: source.id, label: source.name, x: source.x, z: source.z }; }),
      ...stopPins.map(stop => ({ id: `stop:${stop.id}`, label: stop.label, x: stop.shelter.x, z: stop.shelter.z })),
    ];
    let selected: CityWaypoint | undefined, closest = hitRadius;
    for (const target of targets) {
      const pin = target.id === "home" && homePin ? homePin : project(target);
      const distance = Math.hypot(pin.x - x, pin.y - y);
      if (distance < closest) { selected = target; closest = distance; }
    }
    onWaypoint(selected ?? { id: "custom", label: "Waypoint",
      x: center.x + (x - width / 2) / scale, z: center.z + (y - height / 2) / scale });
  };

  return <svg viewBox={`0 0 ${width} ${height}`} className="city-map-drawing" aria-hidden={onWaypoint ? undefined : true}
    role={onWaypoint ? "group" : undefined} tabIndex={onWaypoint ? 0 : undefined} focusable={!!onWaypoint}
    aria-label={onWaypoint ? "Choose a waypoint. Use arrow keys to move the cursor, then Enter to set it." : undefined}
    onFocus={onWaypoint ? () => setCursor({ x: Math.max(18, Math.min(width - 18, player.x)), y: Math.max(18, Math.min(height - 18, player.y)) }) : undefined}
    onBlur={() => setCursor(null)}
    onPointerDown={onWaypoint ? event => {
      if (event.isPrimary && event.button === 0) press.current = { id: event.pointerId, x: event.clientX, y: event.clientY };
    } : undefined}
    onPointerMove={event => {
      if (press.current && Math.hypot(event.clientX - press.current.x, event.clientY - press.current.y) > 8) press.current = null;
    }}
    onPointerCancel={() => { press.current = null; }}
    onPointerLeave={() => { press.current = null; }}
    onPointerUp={onWaypoint ? event => {
      const down = press.current; press.current = null;
      if (!down || down.id !== event.pointerId || Math.hypot(event.clientX - down.x, event.clientY - down.y) > 8) return;
      const rect = event.currentTarget.getBoundingClientRect();
      if (!rect.width || !rect.height) return;
      selectPoint((event.clientX - rect.left) / rect.width * width, (event.clientY - rect.top) / rect.height * height, 22 * width / rect.width);
      setCursor(null);
    } : undefined}
    onKeyDown={onWaypoint ? event => {
      const current = cursor ?? { x: width / 2, y: height / 2 };
      if (event.key === "Enter" || event.key === " ") { event.preventDefault(); selectPoint(current.x, current.y, 14); }
      else if (["ArrowLeft", "ArrowRight", "ArrowUp", "ArrowDown"].includes(event.key)) {
        event.preventDefault();
        const step = event.shiftKey ? 2 : 12;
        setCursor({ x: Math.max(0, Math.min(width, current.x + (event.key === "ArrowRight" ? step : event.key === "ArrowLeft" ? -step : 0))),
          y: Math.max(0, Math.min(height, current.y + (event.key === "ArrowDown" ? step : event.key === "ArrowUp" ? -step : 0))) });
      }
    } : undefined}>
    <rect width={width} height={height} fill="#d7decf" />
    <g transform={`translate(${width / 2} ${height / 2}) scale(${scale}) translate(${-center.x} ${-center.z})`}>
      <StaticGeometry geometry={geometry} />
      {route ? <polyline points={route} fill="none" stroke="#2e627b" strokeWidth="2.5" vectorEffect="non-scaling-stroke" strokeLinecap="round" strokeLinejoin="round" /> : null}
    </g>
    {citywide ? <g className="city-map-district-labels" fill="#3b433c" stroke="#e3e7da" strokeWidth="3" paintOrder="stroke" textAnchor="middle">
      {plan.districts.map(district => { const point = project({ x: district.labelAt[0], z: district.labelAt[1] });
        const crowded = [...landmarks, ...venuePins].some(place => Math.abs(place.x - point.x) < 70 && Math.abs(place.y - point.y) < 18);
        return <text key={district.id} x={point.x} y={point.y + (crowded ? 27 : 0)}>{district.label}</text>;
      })}
      {theatreDistrict ? <text x={project({ x: -2200, z: 1660 }).x} y={project({ x: -2200, z: 1660 }).y} fontSize="11">{companionCityTheatre.district}</text> : null}
      {refineryDistrict ? <text x={project({x:companionCityRefinery.position[0],z:companionCityRefinery.position[2]}).x}
        y={project({x:companionCityRefinery.position[0],z:companionCityRefinery.position[2]}).y + 35} fontSize="11">{companionCityRefinery.district}</text> : null}
    </g> : null}
    {landmarks.map(place => <g key={place.id} transform={`translate(${place.x} ${place.y})`}>
      <circle r={expanded ? 10 : 5} fill="#faf6e9" stroke="#68765d" strokeWidth="1.5" />
      {expanded ? <text y="3.5" textAnchor="middle" fontSize="10" fill="#354531">{place.number || "T"}</text> : <circle r="1.5" fill="#68765d" />}
    </g>)}
    {venuePins.map(place => <g key={place.id} transform={`translate(${place.x} ${place.y})`}>
      <rect x={expanded ? -12 : -4} y={expanded ? -9 : -4} width={expanded ? 24 : 8} height={expanded ? 18 : 8}
        rx="4" fill="#faf6e9" stroke="#596a51" strokeWidth="1.5" />
      {expanded ? <text y="3.5" textAnchor="middle" fontSize="9" fill="#354531">V{place.number}</text> : null}
    </g>)}
    {interestingPins.map(place => <g key={place.id} transform={`translate(${place.x} ${place.y})`}>
      <polygon points={expanded ? "-10,-5 0,-11 10,-5 10,7 0,11 -10,7" : "-4,-2 0,-5 4,-2 4,3 0,5 -4,3"}
        fill="#faf6e9" stroke="#486858" strokeWidth="1.5" />
      {expanded ? <text y="3.5" textAnchor="middle" fontSize="9" fill="#2d483a" fontWeight="600">P{place.number}</text> : null}
    </g>)}
    {friendPins.map(friend => <g key={friend.userId} transform={`translate(${friend.x} ${friend.y})`}>
      <circle r={expanded ? 11 : 5.5} fill="#fff0f4" stroke="#a03b57" strokeWidth="1.5" />
      {expanded ? <text y="3.5" textAnchor="middle" fontSize="9" fill="#872944" fontWeight="700">F{friend.number}</text> : <circle r="1.5" fill="#a03b57" />}
    </g>)}
    {stopPins.map(stop => <g key={stop.id} transform={`translate(${stop.x} ${stop.y})`}>
      <rect x={expanded ? -10 : -4} y={expanded ? -7 : -4} width={expanded ? 20 : 8} height={expanded ? 14 : 8}
        rx="3" fill="#155584" stroke="#f7f6ed" strokeWidth="1.5" />
      {expanded ? <text y="3" textAnchor="middle" fontSize="7" fill="#fff">BRT</text> : null}
    </g>)}
    {homePin ? <g transform={`translate(${homePin.x} ${homePin.y})`}>
      <circle r="11" fill="#fff9eb" stroke="#79705f" strokeWidth="1.5" />
      <HomeGlyph />
      {homePin.offscreen ? <path d="M13-3 18 0 13 3" fill="none" stroke="#5e513e" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" transform={`rotate(${homePin.angle})`} /> : null}
    </g> : null}
    {waypointPin ? <g className="city-map-waypoint-pin" transform={`translate(${waypointPin.x} ${waypointPin.y})`}>
      <circle r={expanded ? 16 : 12} fill="#8551d0" fillOpacity=".18" />
      <path d="M0-10 8 0 0 10-8 0Z" fill="#8551d0" stroke="#fff" strokeWidth="2" strokeLinejoin="round" />
      {waypointPin.offscreen ? <path d="M11-3 16 0 11 3" fill="none" stroke="#503479" strokeWidth="2" strokeLinecap="round"
        transform={`rotate(${waypointPin.angle})`} /> : null}
    </g> : null}
    <g transform={`translate(${player.x} ${player.y}) rotate(${-at.yaw * 180 / Math.PI})`}>
      <path d="M-6 5 0 15 6 5Z" fill="#245b77" stroke="#fff" strokeWidth="1.5" strokeLinejoin="round" />
      <circle r="5" fill="#245b77" stroke="#fff" strokeWidth="2" />
    </g>
    {onWaypoint && cursor ? <g transform={`translate(${cursor.x} ${cursor.y})`} fill="none" stroke="#fff" strokeWidth="4">
      <path d="M-12 0H-4M4 0H12M0-12V-4M0 4V12" />
      <path d="M-12 0H-4M4 0H12M0-12V-4M0 4V12" stroke="#29262f" strokeWidth="2" />
    </g> : null}
    {expanded ? <g transform={`translate(16 ${height - 18})`} stroke="#303b35" strokeWidth="1.5" fill="#303b35">
      <path d={`M0-4V0H${scaleMetres * scale}V-4`} fill="none" />
      <text x={scaleMetres * scale / 2} y="-7" stroke="none" textAnchor="middle" fontSize="11">{scaleMetres} m</text>
    </g> : null}
  </svg>;
}
