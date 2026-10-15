# Sector 03 — Igbehinadun–Oyetayo and the Akinpelu return

October 11, 2026. Conditional research/design package; not a generated level.

## 1. Direction

We continue north from Sector 02 along Igbehinadun Street to Oyetayo Street, then provide a return through Akinpelu, Adeoye and Okun Owa to Brown Street. The result is a roughly 1.20 km mapped loop, not a teleport to a distant district. Our design contrast is between the interchange's transport scale and street-level negotiations over movement, frontage access and repair work. [G01]

Use **mapped**, **historical/reported**, **proposed** and **unresolved** as separate evidence states. All behavioral values, asset budgets and event timings here are proposed. Sources are in [the evidence register](sector-03-sources.md); complete road geometry and shared-node records accompany this document.

The research rectangle is longitude 3.3440–3.3505 E, latitude 6.5593–6.5660 N. It is neither a neighbourhood boundary nor a runtime ownership boundary. Complete ways extend outside it. We retain the street-based sector name because this pass has not established an administrative or locally preferred neighbourhood boundary. [G01]

## 2. Exact spatial structure

| Decision point | Longitude E | Latitude N | OSM node |
| --- | --- | --- | --- |
| Brown / Igbehinadun | 3.3475022 | 6.5595975 | 1079849289 |
| Sector 02 northern Igbehinadun seam | 3.3477254 | 6.5608319 | 4395533093 |
| Igbehinadun source-segment join | 3.3478084 | 6.5618410 | 4395533094 |
| Igbehinadun / Oyetayo | 3.3478130 | 6.5635174 | 35785494 |
| Oyetayo / Akinpelu | 3.3471947 | 6.5632685 | 1079849206 |
| Akinpelu / Okun Owa | 3.3472588 | 6.5624117 | 1079849260 |
| Akinpelu / Adeoye | 3.3470010 | 6.5606501 | 4395533085 |
| Adeoye / Okun Owa | 3.3458748 | 6.5607079 | 4395533083 |
| Okun Owa / Brown | 3.3458385 | 6.5600661 | 1079849293 |
| Oyetayo / Church / Adeyemi | 3.3449118 | 6.5612076 | 1079849444 |

These are source map coordinates, not survey control monuments. [G01]

### Primary candidate route

Starting at Brown/Igbehinadun: follow the northern part of way 5153762, then 1442626891 and 1442626890 to Oyetayo. Turn west for the short section of way 93146069 to Akinpelu. Follow 93146079 and 1442626892 south, Adeoye 441859107 west, Okun Owa 1442626895 south, and Brown 93146084 east to the start. The recorded polyline length is approximately **1,199.1 m** using spherical segment distances, not a surveyed walking distance. [G01]

Northern Igbehinadun way 1442626890 is tagged `oneway=yes` in its node order from 4395533094 toward 35785494. The candidate loop respects that mapped direction. Reversing the same loop is not a valid inferred driving route. Missing one-way tags elsewhere do not independently establish current legal access. The seam at 4395533094 is a database split, not a verified junction or physical barrier. [G01]

Okun Owa way 93146061 curves between Akinpelu and Adeoye, giving a second mapped interior circuit. Oyetayo curves west/south to Church/Adeyemi; do not straighten it into a rectangular grid. Its eastern segment 5153763 reaches mapped Agege Motor Road nodes, but safe crossings, turn permissions and rail/road interfaces require separate confirmation. [G01]

The fifteen selected ways have **23 shared-node pairs at 12 unique nodes**: seven same-road segment joins and sixteen cross-street pairs. These counts are not twenty-three distinct junctions. Three source ways shared with Sector 02 retain identical coordinates and way versions in this retrieval. [G01]

### Placement grammar

`source road → verified cross-section → surveyed access/thresholds → evidenced footprint/envelope → frontage use → operational state → movable detail`

No source way is widened to make a vehicle fit. No alley is invented between Akinpelu and Igbehinadun to shorten a mission. No whole block becomes a market merely because a nearby photo shows trading. Shared roads have one future runtime owner; sector files are evidence copies.

## 3. Topography and coordinate blueprint

Oyetayo's two selected ways carry `embankment=two_sided` or `embankment=yes`. That is a useful terrain investigation lead, not evidence of a particular height, ditch, flood barrier or ground slope. This snapshot has no `ele` values or tagged waterways found by the bounded inspection. Missing mapping is not evidence of absent drainage. [G01]

Survey priorities are both sides of Oyetayo at the Akinpelu and Igbehinadun connections; the Igbehinadun northbound cross-section; and Brown/Okun Owa thresholds. Capture crown, edge, verge, drain invert, outfall direction, threshold and any retaining interface in one declared vertical datum. Do not take the EXIF altitude attached to the agency photograph as ground height. [G02]

The research frame remains the shared WGS84/UTM 31N proposal, EPSG:32631, anchored at latitude 6.5560 N, longitude 3.3510 E. A reviewed CRS library must calculate projected Easting/Northing. Research source metres are `(NorthDelta, UpDelta, EastDelta)`; canonical conversion remains `UE(X,Y,Z) = 100 * source(Z,X,Y)`. Research UE is therefore `(EastDelta, NorthDelta, UpDelta)` centimetres. No approved rotation/translation into the existing game or vertical datum is supplied. Never recenter our canonical world to this origin.

Collision uses measured road and threshold sections, not a displaced colour texture. Retaining edges, steps and open drains need semantic volumes; roof corrugations, cables and small finish defects normally do not. Keep walk, vehicle and restricted-entry surfaces separate. A future preview with estimated widths/heights stays in an approximation layer, not the verified map.

## 4. Traffic and go-slow dynamics

**Proposed:** traffic pressure enters through verified portals on Brown and Oyetayo. A local trip reserves a legal destination/loading place before dispatch. Downstream space, turning conflict and temporary access work cause queues; entering a cell does not automatically spawn congestion.

Northern Igbehinadun's mapped direction constrains path planning. A blocked vehicle cannot solve the event by reversing down the entire one-way segment. A reroute is selected at the last eligible upstream decision point, or the vehicle waits with emergency access preserved. Oyetayo's `lanes=2` is recorded evidence, not enough information to build two measured lanes. [G01]

- **Danfo:** G03 gives a dated nearby visual reference for yellow minibuses and pedestrian interaction, not a current route on Akinpelu. Vehicle subtype, livery, stop location and service must be verified before sector spawning. Use actual wheelbase and turning envelope; no bus enters a link only because its centreline connects.
- **Keke/okada:** retain the vehicle-controller specification but withhold routes until current restrictions and local observations agree. Do not assume green tricycles or universal motorcycle permeability.
- **Cars, vans and works vehicles:** reserve a loading or staging polygon. If none is supported by geometry and access, cancel that job rather than stop in an arbitrary entrance.
- **Hawkers:** proposed stopped-customer interaction needs speed below 1 m/s for 6 s, a reserved approach slot and a retreat route. Finite stock and one offer per encounter prevent endless repeats. No pedestrian is spawned inside a vehicle swept path.
- **Goods:** the historical photograph supports a carried stack of trays as a visual reference; it does not establish the identity of every item. Gala, plantain chips and cold water remain candidate products, requiring reference/rights and placement checks.
- **Queue propagation:** candidate arrival demand starts at 25%, 60% and 85% of a later measured link capacity for calm/busy/event scenarios. These are simulation load ratios, not local traffic counts. Discharge follows downstream space, not a timed teleport.

Agent ID, cargo, payment status, stop reservation and queue order survive sector streaming. The visualization may simplify distant agents; the authoritative simulation must not duplicate inventory or invent access permissions.

## 5. Architecture, materials and repair history

G04's reviewed roadwork collage shows strapped stacks of rectangular paving units, loose aggregate/sand piles, a tipper vehicle, low and multistorey frontages, balconies, shutters/awnings and overhead wires. Its road sign spells the name **IGBEYINADUN STR**, whereas OSM uses **Igbehinadun Street**. Record the spelling difference; do not silently rename the source geometry or assert an exact sign coordinate. The collage's viewpoint positions are unresolved. [G01, G04]

G02 shows light rendered/block walls, red profiled pitched roofing, trees, cables, vehicles and an enclosed-looking yard. The photographer location and object location differ, and the capture date is unspecified. It is a nearby material/massing reference, not authority to place a named agency on a guessed parcel or make its compound traversable.

Our distinctive environmental story is maintenance over time, not uniform decay. The 2015 official-personal archive records an order to rehabilitate Igbehinadun and Inasa. The readable council-media-attributed post reports Phase 2 commencement and paving supplies. Neither proves today's final surface or that work occupies our northern segment. [G04, G05]

| Asset/material | Proposed implementation | Placement condition |
| --- | --- | --- |
| Road paving | Individual edge/silhouette geometry near interaction; instanced surface elsewhere; separate joints, bedding, dust and wetness | Final pavement material and limits still unverified |
| Paver stacks | Strapped rectangular stacks with load variants; stable collision and actual ground contact | Reference family in G04; no copied stack coordinate |
| Sand/aggregate | Stockpile shape, granular normals and damp/dry variants; limited windborne grains | Only at approved fictional/event staging or verified works site |
| Render/block | Painted/unpainted repair boundaries, restrained pore detail and water paths | Match each located façade; no universal exposed block |
| Pitched metal roof | Separate sheet, ridge, fastener and edge modules; under-roof depth | G02/G04 family references; exact roof footprint pending |
| Balcony/shutter/awning | Actual support thickness, openings and operable collision | Viewpoint registration and public/private threshold pending |
| Utility runs | Pole/cable relations with plausible sag; no random tangled bundles | Actual route and attachment evidence needed |
| Generator soot | Deposition from an identified exhaust; independent power/audio state | No generator has been located by this package |

British-colonial façades, brutalist blocks, modern glass towers, estate gates and brand/campaign decals are not inserted to satisfy a citywide checklist. This slice's supported references concern local frontages, works and yard-like structures, not a complete architectural survey.

Proposed LOD plan: keep thresholds, silhouette edges and large supports at all navigable distances; replace repeated roof corrugation and paving joints with normals at distance. Candidate review distances are 10, 35 and 100 m; acceptance is based on screen size and silhouette continuity. Collision/access stays unchanged across visual LODs. No performance result is claimed.

## 6. Spatial audio and NPC routines

**Proposed, not a field recording survey.** Crossfade transport intensity by actual distance/obstruction from the Brown and Oyetayo approaches. A quieter interior conversation should be possible without switching the whole city off. Place individual engines, tools, footsteps and voices; use walls and corners for occlusion, not sector rectangles.

Roadworks provide distinct source events: pavers set down, hand-tool contact, wheelbarrow/trolley motion and a truck backing under controlled clearance. Use recordings with permission, varied takes and event cooldowns. Do not turn one impact into a looping industrial soundtrack. Moving horns use relative velocity/Doppler; stationary horns do not.

Generator sound is conditional on a verified generator: start attempt → engine catch → load settling → running → shutdown. A power interruption affects only connected equipment. Shop music uses licensed/commissioned Afrobeats/Fuji only at supported premises, with stronger high-frequency attenuation through walls; it is not evidence that every shop plays the same genre.

NPC priority: `hazard → protected passage → agreed work/journey → conversation/transaction → rest`. Proposed roles include commuters, traders, household errand-runners, drivers and a maintenance crew. Gender and clothing do not select aggression or occupation. Named people in sources are not reproduced as NPCs without permission.

- Traders reserve stock/space and negotiate through offers, counteroffers and opt-out; impatience depends on time/workload, not gender.
- Maintenance workers coordinate a defined task, hold a safe work perimeter, and release passage when the task ends. They do not endlessly excavate for atmosphere.
- Commuters choose wait versus verified detour from remaining travel time, luggage and access needs.
- Park-dues collection is not added to this street without evidence of the actual institution/location. No automatic agbero gang or tech-worker faction is seeded.
- “How far?”, “Abeg” and “No wahala” remain script candidates pending fluent contextual/recording review. Route calls require verified services; dialect and accent must not be randomized for comedy.

Initial transaction radius is 1.5–3 m; a work-access negotiation uses 2–4 m. Personal-space and cart envelopes derive from body/load width. These are proposed tuning values. Quiet intervals, refusal and rest are normal states, not idle-animation failures.

## 7. Weather and lighting

**Proposed seasonal rendering; no street-level weather calibration obtained.** Keep aerosol haze, humidity, local dust, surface wetness and standing water as separate variables. Harmattan candidates reduce distant contrast and warm direct light through aerosol scattering, without a permanent orange filter. Dust rises only from active, evidenced loose surfaces or work piles, not every pavement.

Rain begins with exposed-surface darkening, then roof impacts, edge drips and wheel spray. Sheltered render stays drier; a roof only creates runoff at a modeled edge. Paver joints retain moisture differently from cladding and road patches. On clearing, drying depends on exposure, wind and material, not a synchronized dissolve.

The Oyetayo embankment tags motivate a survey question: how does water cross or leave each side? They do not answer it. Never create a retaining dam, culvert or flooded lower street from the tag alone. Hydrologic storage uses rainfall/inflow minus drainage/infiltration/outflow, with per-basin surfaces. No universal water plane or assumed instant flash flood.

Update navigation only from reviewed water depth/flow classes and actual geometry. An event must have a confirmed accessible alternative or remain disabled. Missing topography prevents calibrated flooding, not preparation of the system interfaces.

## 8. Developer Asset Checklist

Each unchecked item is unimplemented. Reference availability, placement approval and native delivery are separate states.

### 3D meshes and architecture

- [ ] S03-ROAD-01 — Igbehinadun three-segment chain; source geometry ready, width/height/access pending.
- [ ] S03-ROAD-02 — Oyetayo curved alignment and eastern outlet; embankment cross-sections and crossings pending.
- [ ] S03-ROAD-03 — Akinpelu/Adeoye/Okun Owa return; measured turning/footway envelope pending.
- [ ] S03-FRONT-01 — small/multistorey frontage variants with balconies, shutters and awnings; family reference in G04, parcels/heights pending.
- [ ] S03-ROOF-01 — profiled pitched-roof kit; reference in G02/G04, exact placements pending.
- [ ] S03-WORK-01 — strapped paver stacks, hand tools, trolley and controlled tipper envelope; reference family in G04, event staging pending.
- [ ] S03-VEH-01 — model-specific Danfo reference set and door/passenger sockets; nearby historical reference only.
- [ ] S03-EDGE-01 — thresholds, kerbs, drainage and any retaining edges; survey required.

### Material shaders and textures

- [ ] S03-MAT-01 — paving/patch variants with physical scale; retain surface conflict until resolved.
- [ ] S03-MAT-02 — render, block, profiled roofing and shutter materials; exposure-led wear.
- [ ] S03-MAT-03 — loose aggregate, straps and stack contact dirt; no floating piles.
- [ ] S03-MAT-04 — separate wetness, joint retention, runoff, puddle and drying inputs.
- [ ] S03-DECAL-01 — street spelling/date/rights ledger; no final sign/brand/campaign decal admitted.
- [ ] S03-LOD-01 — silhouette-preserving material/mesh transitions; native performance unmeasured.

### Dynamic systems and spawners

- [ ] S03-GRAPH-01 — mapped direction and shared-node ownership; one-way-aware rerouting.
- [ ] S03-QUEUE-01 — downstream capacity, reserved loading and protected passage.
- [ ] S03-WORK-02 — bounded maintenance-window state machine and cancellation.
- [ ] S03-NPC-01 — finite work/stock/travel goals, consent/refusal and reviewed dialogue.
- [ ] S03-AUDIO-01 — source-positioned work/traffic mix with rights records and occlusion.
- [ ] S03-WEATHER-01 — exposure/flow/terrain interfaces; no calibrated flood yet.
- [ ] S03-PERSIST-01 — durable agent IDs, reservations and event outcomes across S02/S03.

## 9. Micro-narrative S03-E01 — the paving window

**Fictional scenario inspired by reported rehabilitation, not a claim of an active works site.** Candidate staging is the Igbehinadun approach south of Oyetayo node 35785494. Final work polygon, frontage permissions and pedestrian bypass require survey/confirmation; the node alone does not approve them.

A maintenance crew needs a short clear interval to move strapped paving stock into an approved work bay. One trolley crosses the eligible approach; a northbound queue forms while a worker checks the passage. A commuter asks whether waiting is quicker than taking the Akinpelu return. The player may wait, choose the verified detour, or assist only with a safe, consensual lightweight task. No unauthorized truck operation or hazardous lifting is offered.

State sequence: `reserve work bay and bypass → warn upstream → hold new entry → clear crossing → move stock → reopen → release reservations`. Proposed active hold is 30–60 s, maximum 90 s. The crew abandons the move safely if emergency access, poor visibility, crowding or a conflicting vehicle prevents clearance. No vehicle reverses against the mapped one-way rule as an automatic escape.

Before a closure, the route planner can offer Brown → Okun Owa → Adeoye → Akinpelu → Oyetayo only if that specific vehicle/pedestrian class has independently verified access. A player already beyond the decision point gets a confirmed bypass or a brief wait, never an invented alley. Mobility needs alter route eligibility, not merely walking speed.

Persist event ID, stock count, trolley location, queue membership and reservations. Stream-out cannot replenish materials, duplicate rewards or reopen a blocked route visually while its collision remains closed. Completion is passage restored and reservations released, not a fixed timer alone.

## 10. Next work and acceptance

The next adjacent research corridor is west from the Oyetayo/Church/Adeyemi node along Adeyemi Street, beginning with way 8564504 and its Owoseni connection. This is a discovery lead, not an approved next-sector design.

S03 has mapped geometry, historical/reported references and a conditional blueprint. Exact parcel registration, contemporary imagery, road permissions, cultural review, vertical datum, asset production, native integration and gameplay acceptance remain open. No completed map or playable maintenance/flood system is claimed.
