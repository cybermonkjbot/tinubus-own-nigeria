# Sector 09 — Akin-Lawanson, Seinde Callisto and the pedestrian crossing

October 11, 2026. Conditional research/production blueprint. No assets, runtime map changes or worker actions.

## 1. Sector identity

We continue from Afariogun along **Akin-Lawanson Street** to the expressway frontage, then examine the mapped pedestrian bridge and the **Seinde Callisto Street** axis. The contrast is vertical and functional: local frontage, multiple parallel roadways, and pedestrian movement above traffic. This is not a new glass skyline or a fabricated transport terminal. [N01]

The bridge is a **candidate association with the reported Charity footbridge**, not an independently registered identification. A March 29, 2023 Guardian report describes reopening while reconstruction continued. Its photograph provides bounded material evidence; neither publication nor the OSM bridge tag proves current access, structural safety or completed works. [N02]

Evidence classes: **mapped** source geometry/tag; **reported** dated account; **observed** image-specific detail; **proposed** authored design; **unresolved** insufficient for placement. All simulation numbers below are proposed. The [source register](sector-09-sources.md) records the distinctions.

## 2. Streets, controls and topology

Fresh query rectangle: longitude 3.3388–3.3456 E, latitude 6.5503–6.5561 N. The [network](sector-09-network.geojson) retains 20 road ways and three pedestrian ways in full; complete ways extend beyond the rectangle. Selection is not complete local coverage or runtime ownership. [N01]

| Control | Longitude E | Latitude N | OSM node |
| --- | --- | --- | --- |
| Afariogun / Akin-Lawanson; S08 interface | 3.3391722 | 6.5520004 | 35785428 |
| Akin-Lawanson / mapped primary road | 3.3402922 | 6.5510982 | 177174622 |
| Primary road / north-side service approach | 3.3413832 | 6.5515980 | 1475182869 |
| Northern stair foot / service way | 3.3413592 | 6.5516474 | 5405812682 |
| Northern deck endpoint | 3.3415149 | 6.5517186 | 5405812681 |
| Southern deck endpoint | 3.3417332 | 6.5512600 | 5405812680 |
| Southern stair foot; unconnected to road graph | 3.3415799 | 6.5511891 | 5405812679 |
| Seinde Callisto / mapped primary road | 3.3418029 | 6.5517478 | 4395533032 |
| Southern Seinde footprint-sample centre | 3.3416220 | 6.5526154 | 1079849447 |
| Seinde / Church / Boladale / unnamed secondary | 3.3452941 | 6.5557881 | 1079849271 |

These are source map points, not survey benchmarks. The [connection record](sector-09-connections.json) distinguishes 24 road pairs, two pedestrian-continuity pairs and one pedestrian/road interface, across 22 unique shared nodes. Pair count is not junction count. [N01]

### Route records and their limits

| Record | Horizontal length | What it establishes |
| --- | --- | --- |
| Akin-Lawanson from Afariogun to primary-road node | 161.2 m | Source centreline only |
| Geometric trace from Afariogun to northern stair foot | 300.0 m | Connected source subchains, **not a drivable route or proven footpath** |
| Northern stairs → bridge → southern stairs | 94.0 m | Three-way pedestrian chain, not true 3D walking distance |
| Bridge way 560546954 alone | 56.4 m | Horizontal line length, not certified clear span |
| Seinde Callisto full spine | 655.9 m | Street source geometry, not approved traffic capacity |

The 300 m trace includes 132.7 m **against** the node direction of `oneway=yes` primary way 134140052. It is an adjacency diagnostic only. Do not offer it to vehicles; do not assume a parallel sidewalk exists. A valid vehicle connection and pedestrian frontage require their own verified routes. [N01]

The bridge line crosses four retained road ways in plan: 134140052, 17505891, 134147999 and 134140045. None shares a node with it. Their four calculated intersections remain **non-junctions**; road vehicles cannot turn onto the bridge or collide with a flattened deck. The bridge/steps have source `layer=1`, which expresses ordering rather than metres. [N01]

Northern stair foot 5405812682 shares a node with service way 1158125999. Southern foot 5405812679 carries `highway=crossing` but is not a node of nearby road 370658034. Its approximate 0.4 m point-to-line separation is derived in a local planar approximation, not a measured gap on the ground. Do not snap it into the graph automatically or infer a safe crossing from the tag. [N01]

Steps ways 560546953 and 560546955 also carry `traffic_calming=table`. Preserve that source tag for review; it does not authorize generating a raised road table under a stair. No step count, ramp, lift, landing dimensions, handrail clearance or accessible alternative is mapped in these selected ways. Missing evidence is not evidence that no accessible route exists. [N01]

### Ownership and boundaries

Akin-Lawanson 93156409, unnamed road 441859086 and primary way 134140052 match S08 geometry, node order, node versions and way versions. Assign one future runtime owner per source feature, with shared references for queues, collision and audio. Keep the four parallel carriageway records distinct until their real roles and transitions are verified. [N01]

The Guardian report associates the Charity crossing with NAFRC and a school. That does not authorize public traversal of the institution or supply its interior plan. This sector does not place military routines, security details, school rosters or private interiors. [N02]

## 3. Terrain, architecture and coordinate contract

The [southern Seinde sample](sector-09-buildings.geojson) contains 17 closed rings selected within an approximate 65 m representative-point radius. Sixteen have only `building=yes`; way 686906483 v1 also has religious-use/name tags, last edited April 30, 2019. Preserve its exact source spelling in evidence; do not create current signage, congregations, service times, denominational practices or interior assets from the tag. No sampled height or current use is verified. [N01]

The March 2016 works archive explicitly lists Seinde Callisto Street, Oshodi-Isolo, among maintenance/palliative works. It does not locate a repair, establish dimensions or prove a current material. Images elsewhere in that archive article concern other roads and are excluded. [N03]

Ten fresh SRTM queries yield 20, 21, 28, 28, 28, 28, 25, 29, 24 and 23 m. These are historical coarse raster context in EGM96, not ground/stair/deck benchmarks. Four equal values around the bridge cannot flatten its stairs, and the 28/25 m difference is not stair rise or clearance. [N05]

```text
Evidence: WGS84 [longitude,latitude]; no altitude
Proposed research projection: EPSG:32631
Shared research origin: longitude 3.3510 E, latitude 6.5560 N
Source metres: (NorthDelta, UpDelta, EastDelta)
Existing adapter: UE(X,Y,Z) = 100 * source(Z,X,Y)
Research UE centimetres: (EastDelta, NorthDelta, UpDelta)
Projected coordinates calculated: false
Engine vertical datum: null
Canonical-world registration: null
```

Required survey: true deck/soffit profiles, support positions, stair treads/risers, landings, railings, canopy envelope, road heights, sightlines, public approaches, drainage and building thresholds. Neither SRTM nor photo perspective substitutes for this. Retain canonical game geography; degrees never enter the metre adapter.

## 4. Traffic, walking and go-slow rules

**Proposed:** separate vehicle lane graphs, pedestrian surfaces and vertical links. A road shared node is not automatically a legal turn. A bridge line is not a 2D walkway painted across lanes. Store mode, direction, height interval, clearance, surface evidence date, access state and downstream capacity per link.

- Vehicles reserve downstream space before joining the frontage; queues do not spill through a stair-foot refuge or private entrance. Aggregate distant demand only if individual trips reconcile when streamed in.
- Turning uses measured swept paths, wheelbase, mirrors, braking and contact geometry. Opposing one-way roads do not merge because their names match.
- Danfo/minibus classes need model, livery and route evidence. Historic nearby images show yellow minibuses and a larger differently painted bus, not a Volkswagen-only fleet or present stops on Seinde Callisto. [N04]
- Keep keke and okada policy unknown until current route-specific evidence is obtained. No green-tricycle default, automatic motorway access or motorcycle routing onto steps.
- Hawker admission requires local evidence, a permitted standing area, finite stock and protected retreat. Proposed interaction threshold: stopped vehicles below 0.5 m/s with sufficient dwell time; suppress interactions on active carriageways, stairs, landings and bridge bottlenecks. Gala, plantain chips and cold water remain requested product candidates, not confirmed stock at this bridge.
- Pedestrians select routes by access, mobility, load and destination. A cart, wheelchair or mobility aid cannot silently receive a stair link. Lack of a verified alternative is reported to route planning; it never becomes a dash across the expressway.
- Stair links require measured geometry and suitable animation/contact constraints. Reserve landing storage before entry, maintain counterflow clearance and avoid stopping groups in the narrow section. Do not implement crowd pressure as a reward mechanic.

Proposed local walking/navigation states: `approach → verify link access → reserve landing capacity → traverse stairs → traverse deck → verify exit → clear landing`. Invalid exit or stale access state suppresses entry. The selected source graph is not yet a valid end-to-end public pedestrian route.

## 5. Material storytelling and LOD

The 2023 photograph visibly shows a green roof covering on a framed canopy, repeated vertical railing members, concrete-looking deck/support surfaces, and scaffolding with cones during works. Exact roofing chemistry, construction dimensions, support layout and contemporary condition remain unverified. The historical scaffold is not placed as a permanent landmark. [N02]

Freshly retrieved Commons photographs repeat the four S08 originals with matching bytes. They contribute historic roadside margins, a distant pedestrian bridge, vehicle-panel wear, reflective vests, containers/tools, kerbside standing water and a road divider. None is registered to a precise 2026 camera or one of the 17 footprints. [N04]

| Proposed material family | Shader/mesh contract | Placement evidence |
| --- | --- | --- |
| Bridge concrete | Separate substrate, repair patch, runoff and underside occlusion | Registered current surface and measured support geometry |
| Railing/frame | Profile-specific metal, coating wear and fasteners | Exact section, spacing and attachment; image spacing is not a measurement |
| Canopy | Independently authored top/underside, joins, translucency and wetness | Real material/thickness; green appearance alone is insufficient |
| Stairs/landings | True collision treads or equivalent verified walk surface; distinct nosing | Rise, tread, width, slip treatment, rails and accessible provision |
| Frontage/roof | Render, repairs, sheet or other roof candidates | Current building registration; no generalized decay preset |
| Road/kerb/drain | Surface-scale aggregate, repair edges and localized wetness | Surveyed curb/drain/road relationships |

Soot follows a verified exhaust and receiving surface, not arbitrary “grit.” Wires attach to observed supports. Campaign posters and MTN/Glo/business decals require exact evidence and rights; none is fabricated to decorate the bridge. Colonial/brutalist/glass building families remain outside this sector unless specifically evidenced.

Proposed visual detail bands: full approach/stair/railing silhouette at close range, simplified repeating members and facade detail beyond roughly 30–60 m, distant massing by screen coverage. Collision, guard boundaries, usable openings, vertical separation and water barriers do not disappear with LOD. A rail may simplify visually only if silhouette and safety-critical navigation remain consistent. No renderer/performance acceptance is claimed.

## 6. Ambient audio and NPC culture

**Proposed sound layers:** separate four roadway emitter paths below, footfalls and carried-object sounds on the elevated link, and frontage conversations on Seinde. Deck elevation changes source angle and shielding; it does not create a silent zone. Doppler follows moving vehicles; stationary horns do not sweep. Canopy rain impacts and footsteps need measured/verified material classes before final samples are selected.

Generators require verified positions and per-unit start/idle/load/shutdown states. Grid loss changes each powered source independently. Shop music requires a confirmed source and licensed or commissioned audio, including Afrobeats/Fuji references. No historic photo provides a music selection, generator location or field-recorded sound level.

NPC routines include commuting, errands, deliveries, optional trade, waiting, meeting friends and rest. Priorities are `safety/access → committed journey/work → consensual interaction → social activity/rest`. Store carried-object size, mobility, pace preference, fatigue and group membership. Do not make market women uniformly confrontational or infer tech/agbero factions from citywide stereotypes; dues collection needs an evidenced motor park.

Proposed speech radii: 1.5–3 m for a quiet exchange, 2–4 m for a passage request when sightline and crowding permit. “How far?”, “Abeg” and “No wahala” retain narrow lexical support from the earlier British Council reference; performance, code-switching and local use need fluent review. No new interviews, street-audio corpus or cultural approval were acquired. [N07]

The 2023 report documents concerns about crossing access among different users. Do not reenact named residents, actual students, institutional uniforms or hazardous road-crossing behaviour from the story. Treat access as a planning requirement, not a spectacle. [N02]

## 7. Weather and vertical navigation

**Proposed, uncalibrated:** keep aerosols, humidity, surface dust, rain, wetness and standing water as separate variables. Harmattan direction uses atmospheric contrast loss and scattered sunlight, not universal orange fog. Historical haze does not identify a season.

Rain strikes canopy, exposed stairs, deck edges and road surfaces differently. Roof discharge may affect an approach only where its real drainage path is measured. Never put the deck, ground and all roadways on one dynamic water plane. Solve admitted local storage/flow from rainfall and connected inflow minus infiltration/outflow; unknown drainage remains unknown.

Navigation responds to access, visibility, water depth/velocity and material traction. Stairs are not flooded merely because a nearby low point accumulates water. Lightning/high-wind/maintenance closures, if designed, need independently configured access logic and safe waiting locations; they cannot generate an unverified street-level crossing. No current bridge operational policy is asserted.

## 8. Developer Asset Checklist

All production items are unbuilt; source readiness, asset readiness, native integration, visual review and gameplay acceptance remain separate.

### 3D meshes and architecture

- [ ] S09-R01 — twenty road source ways with one-way direction and shared S08 ownership records.
- [ ] S09-P01 — northern stairs, deck and southern stairs from measured profiles; no fabricated ramp/lift.
- [ ] S09-P02 — verified public approach surfaces and southern connection; no automatic 0.4 m snap.
- [ ] S09-B01 — canopy/railing/deck/support kit, contemporary registration and survey pending.
- [ ] S09-F01 — seventeen source footprints with height/entrance/current-use review.
- [ ] S09-O01 — generic closed parcel, soft strap and carry poses for the fictional event; no brand or real business identity.
- [ ] S09-V01 — referenced vehicle variants and true collision envelopes.

### Material shaders and textures

- [ ] S09-M01 — concrete repair/runoff/underside masks tied to verified surfaces.
- [ ] S09-M02 — railing coating, fasteners and canopy top/underside candidates.
- [ ] S09-M03 — stair tread/nosing and wet traction cues matched to actual geometry.
- [ ] S09-M04 — frontage/roof/road/kerb materials, clean and weathered variants.
- [ ] S09-M05 — vertical-layer wetness and drainage effects without cross-level water leakage.
- [ ] S09-X01 — source attribution ledger and separate likeness, signage, voice and music clearances.

### Dynamic systems and spawners

- [ ] S09-G01 — typed road/pedestrian graphs; four plan intersections explicitly non-junctions.
- [ ] S09-G02 — one-way validation and unresolved south-approach gate.
- [ ] S09-N01 — load/mobility-aware stair admission and landing reservations.
- [ ] S09-T01 — downstream-storage traffic demand and local stopping eligibility.
- [ ] S09-A01 — height-aware roadway, footfall, canopy-rain and frontage audio.
- [ ] S09-W01 — surveyed vertical drainage and independent surface/access states.
- [ ] S09-E01 — bounded parcel-restaging event with passage preservation and cancellation.
- [ ] S09-Q01 — independent source, asset, native, visual and gameplay acceptance.

## 9. Micro-narrative S09-E01 — re-tie before the stairs

**Fictional; no claim about a real trader or incident.** Candidate decision anchor: northern approach node 5405812682. The event needs a surveyed, admitted refuge beside the approach, outside the stair entrance, service-road carriageway and private thresholds. A source node does not establish that refuge. If it is absent, do not place the event here.

A pedestrian notices a loose strap on a closed parcel before entering the stairs. They move to the reserved refuge and re-tie it; a companion waits beside them while through-pedestrians continue. A third acquaintance briefly approaches, then stands clear after a passage request. The player can continue, wait to speak, or offer consensual assistance. The event changes local avoidance and interaction, not the bridge's structural state.

Sequence: `validate refuge and continuing route → reserve refuge → leave through-path → re-tie/check load → optional exchange → release refuge → resume only if route remains valid`. Proposed duration 20–45 s, cap 60 s. No parcel opens into litter, no goods multiply, and nobody is pushed toward traffic. Cancel if crowd storage is exceeded, rain makes the refuge unsuitable, access closes or an emergency requires clearance.

This event never creates the missing southern connector or declares the full crossing usable. If no verified onward route exists, the character keeps a valid same-side destination rather than attempting the bridge. Mobility needs remain part of the route decision; a person or aid is not carried up stairs as a compulsory puzzle.

Persist parcel ownership, strap state, participants, reservation and route decision across streaming. Acceptance requires conserved objects, uninterrupted through-passage, bounded clearance and agreement between visible parcel/person geometry and collision. This has not been implemented or played.

## 10. Next sector and remaining work

Continue from northern Seinde node **1079849271** into the **Boladale–Church Street interface**, using Boladale way **93146072** as the next fresh discovery lead. Preserve the shared junction rather than inventing a new district gateway.

Current gaps: exact bridge identity/camera registration, current access and condition, measured vertical/approach geometry, road permissions, building envelopes, local cultural review, rights, canonical registration and all native/visual/playable production. The documentation package is not consumed by either renderer.
