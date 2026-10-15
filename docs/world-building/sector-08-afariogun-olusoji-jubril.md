# Sector 08 — Afariogun, Olusoji and Jubril

October 11, 2026. Research and conditional production blueprint. No assets, runtime geography or workers changed.

## 1. Character and evidence

Our next sector follows **Afariogun Street south from Oshodi Road**, then explores the Olusoji–Adedeji–Jubril circuit on its west side. The mapped spine reaches a one-way Apapa-Oworonshoki Expressway way at its southern endpoint. The difference is between through-movement and frontage life, not an invented transition to a glass skyline. [M01]

A council media-office post dated December 17, 2023 describes this Afariogun as a link between Oshodi Road and the Oshodi-Apapa Expressway, and reports repairs to bad portions that drivers had been avoiding. It is useful historical traffic context, not a current road-condition survey. **Asa Afariogun in Ajao Estate is a different research target**; its maintenance references are excluded. [M02, M06]

Evidence classes: **mapped** source geometry/tags; **reported** dated publications; **observed** bounded image details; **proposed** authored systems; **unresolved** fields that cannot yet support placement. Read the [evidence register](sector-08-sources.md) with this document. All numeric simulation and art settings below are proposed, not local measurements.

## 2. Spatial layout and adjacent interfaces

Fresh query rectangle: longitude 3.3365–3.3430 E, latitude 6.5488–6.5569 N. The [road dataset](sector-08-roads.geojson) preserves 19 complete source ways, including the Afariogun spine, connected side streets and the southern one-way boundary. Some complete ways extend outside the rectangle. This is selected research geometry, not comprehensive coverage, a streaming boundary or a new canonical district. [M01]

| Control | Longitude E | Latitude N | OSM node |
| --- | --- | --- | --- |
| Oshodi Road / Afariogun / Sodehinde Close | 3.3403826 | 6.5566126 | 35785462 |
| Afariogun / Adejumobi | 3.3401520 | 6.5553545 | 3745002397 |
| Afariogun / Olusoji | 3.3400634 | 6.5549323 | 3745002376 |
| Afariogun / Ishola Anigbjumo | 3.3400600 | 6.5547066 | 1079925442 |
| Afariogun / Jubril | 3.3400597 | 6.5542291 | 3745002384 |
| Afariogun / Adedeji | 3.3396791 | 6.5527672 | 3745002375 |
| Afariogun / Sadiku | 3.3393247 | 6.5522280 | 1079925455 |
| Afariogun / Akin-Lawanson | 3.3391722 | 6.5520004 | 35785428 |
| Afariogun / Eyinogun | 3.3381102 | 6.5502513 | 1079925565 |
| Afariogun / mapped expressway way | 3.3380918 | 6.5494411 | 1475221992 |

These are WGS84 source coordinates, not surveyed benchmarks. Preserve the source spelling “Ishola Anigbjumo Street”; do not silently replace it with a guessed name. [M01]

The graph contains 28 shared-node road pairs at 26 distinct nodes. Three ways meet at the northern Afariogun junction, so pair count is not junction count. Full node sequences and five exact route subchains are retained in [connections](sector-08-connections.json). [M01]

| Route | Mapped length | Purpose in this blueprint |
| --- | --- | --- |
| Afariogun north–south spine | 863.9 m | Oshodi interface to southern boundary |
| Afariogun → Adejumobi → Oshodi Road → Afariogun | 714.2 m | Northern circuit candidate |
| Olusoji → Adedeji → Jubril → Afariogun | 635.5 m | Local circuit candidate |
| Olusoji → Adedeji → Jubril, between Afariogun junctions | 557.3 m | Conditional north-to-south repair alternate |
| Direct Afariogun between those junctions | 78.2 m | Event corridor, not a measured worksite |

Lengths are Haversine sums rounded to 0.1 m for reproducibility, not survey precision. The alternate is much longer than the direct segment; it should not be sold to every NPC as the obvious choice. Geometry does not establish direction, public access, usable width, surface condition or safe turning. [M01]

Way 134140052 is mapped `primary`, `oneway=yes`, with name Apapa-Oworonshoki Expressway. Preserve source node direction and separate its admission policy from local streets. The source name does not prove that every part is a main carriageway, nor permit a turn across a median. Do not synthesize a ramp or join crossing lines without a shared source node and further geometric/access review. [M01]

Oshodi Road way 589001493 matches Sector 07 in geometry, node order, node versions and way version. It must have one future runtime owner. Southern imagery and roadways will be shared references when the next sector is researched; they are not duplicate traffic reservoirs. [M01]

## 3. Footprints, terrain and coordinate contract

The [building sample](sector-08-buildings.geojson) retains 34 closed source rings selected near Afariogun/Jubril using the arithmetic mean of non-closing vertices within an approximate 65 m radius. Every selected way has `building=yes`; none supplies an approved height, use, entrance, occupant or current roof outline. This is not a cadastral or full-polygon buffer. [M01]

A February 23, 2020 report discusses homes near Afariogun/Sadiku and describes shared-room housing arrangements and darkened exterior surfaces. Residents alleged smoke/noise disturbance; the bakery operator disputed key complaints. This is dated, contested street-associated context, not permission to identify any sampled polygon as that business or recreate residents, health claims, addresses or the dispute. [M04]

Eight fresh nearest-neighbour SRTM queries return 18, 22, 20, 17, 20, 18, 16 and 13 m. Their locations and source provenance are in [terrain context](sector-08-terrain-context.json). They are coarse historical raster context, not present-day bare-earth, road, drain, threshold or deck levels. The values do not justify a smooth downhill road, a local slope between samples or a flash-flood depth. [M05]

Required geometric evidence: current carriageway/footway widths, property thresholds, crossfalls, drain inverts and connectivity, actual culverts, turning radii and southern road interfaces. Do not infer a drainage connection to Sector 07's ditch merely because it lies west of this corridor.

```text
Evidence coordinates: WGS84 [longitude, latitude]; no altitude
Proposed research projection: EPSG:32631
Shared research origin: longitude 3.3510 E, latitude 6.5560 N
Research source metres: (NorthDelta, UpDelta, EastDelta)
Existing adapter: UE(X,Y,Z) = 100 * source(Z,X,Y)
Research UE centimetres: (EastDelta, NorthDelta, UpDelta)
Projected coordinates calculated: false
Engine vertical datum: null
Registration into existing canonical world: null
```

SRTM's EGM96 source datum does not establish the engine datum. No degrees enter the metre adapter. No existing world is recentered. Survey, projection execution and canonical registration remain explicit production gates.

## 4. Procedural traffic and go-slow behaviour

**Proposed:** use trip-purpose demand with a destination, vehicle envelope, permissible route and arrival window. The main conflict is drivers negotiating localized restrictions while side-street users preserve frontage access. Do not scale every street to permanent gridlock.

- Retain source way/subchain identity in lane candidates. Unknown width or mode permissions prevents final lane generation; it does not become a generic two-lane road.
- Reserve junction exit space before entry. A queue reaching the Oshodi junction suppresses new demand upstream; it cannot grow through other vehicles or block all crossing paths.
- Driver avoidance targets a physically represented surface defect or work enclosure. Steering uses vehicle swept volume, stopping distance and sightline; no lateral teleporting around invisible hazards.
- At low speed, negotiate priority through gap acceptance, slowing, eye-line/gesture cues and bounded horn use. Give drivers different patience and trip urgency, not a single aggressive personality.
- A proposed minibus class needs dated local vehicle/route references. The nearby historical images show yellow minibuses and a differently painted larger bus; neither identifies every vehicle as a Volkswagen or establishes present stops on Afariogun. [M03]
- Keep keke/okada admission disabled until current, route-specific permissions and imagery resolve where they belong. No green-keke default, unrestricted expressway access or motorcycle passage through pedestrian-only gaps.
- Hawker spawn eligibility requires locally confirmed trading activity, stopped traffic, a legal/admitted standing area, clear refuge and a return path. Proposed vehicle interaction threshold: below 0.5 m/s, with immediate withdrawal when restart intent is detected. Limit inventory and time away from refuge.
- Gala, plantain chips and cold packaged water remain requested stock candidates, not verified stock at this junction. Keep product placement and packaging rights unresolved instead of inventing a branded stall.

Near-field vehicles use body/wheel collision and steering constraints. Distant traffic may be aggregated only if trips and queue capacity reconcile on re-entry. The two candidate circuits are graph options, not currently authorized diversions.

## 5. Architecture, materials and LOD

Our visual hierarchy is proposed as: through-road surface and thresholds first, building envelopes second, frontage activity third, small material evidence last. Do not fill missing evidence with colonial facades, brutalist towers or luxury gates merely to meet a style list.

Historical Commons observations near the southern approach include a roadway and uneven roadside margin, low-rise forms, a pedestrian bridge in the distance, a tall light pole, and utility supports in the 2011 image. The 2017 close views show bus-panel wear, a door aperture, reflective work vests, containers/tools, standing water beside a kerb and a concrete road divider. These are image-specific observations, not measurements or current assignments to our 34 footprints. [M03]

| Material family | Proposed shader inputs | Placement gate |
| --- | --- | --- |
| Render/paint | Coating age, substrate, repair mask, orientation-specific runoff | Registered facade and actual substrate |
| Road repair | Base surface, cut edge, aggregate scale, compaction state, wetness | Current observed patch/material; historical repair claim is insufficient |
| Roofing/canopy | Sheet lap, fixings, underside, water path | Roof registration; no universal rust preset |
| Threshold/kerb | Measured steps, chipped edges, dirt in recesses | Walkable clear width and drain connection |
| Vehicle body | Paint, grime, localized panel wear and rubber seals | Referenced model and era; no copying identifiable markings |
| Equipment exhaust | Source position, run duration, wind, receiving surfaces | Verified equipment/facade; no arbitrary soot on homes |

The fresh Afariogun way retains `surface=unpaved` while the 2023 post reports repairs without identifying their exact extent or establishing today's surface. Keep that unresolved. Do not overwrite the tag with “asphalt everywhere,” or render the full spine as dirt just because a tag says unpaved. [M01, M02]

Proposed LOD bands: detail at frontage and road-edge interfaces within roughly 15 m, simplified openings/roof junctions through roughly 50 m, then silhouette-preserving massing based on screen coverage. Collision, occlusion-critical walls, entrance clearance and water barriers remain consistent across visual LOD. No performance or visual acceptance is claimed.

Campaign posters, MTN/Glo decals, utility logos, business names and shop interiors need dated positional evidence and asset rights. A Commons licence on a photograph does not clear every person or third-party mark pictured in it. The photographs remain research references; none is imported.

## 6. NPC logic, culture and audio

**Proposed behaviour tree:** `immediate danger → accessible route/refuge → committed work or journey → optional transaction → social activity/rest`. Store destination, deadline, mobility, carried load, stock, relationships, fatigue and pending interactions. A person can live here, work elsewhere, wait, socialize or rest; “hustle” is not nonstop shouting.

Trade uses reservation price, stock, timing and relationship memory. Negotiation ranges start at 1.5–3 m; passage requests at 2–4 m, reduced or disabled in cramped geometry. Market women are not a uniformly aggressive faction. Tech workers require no invented tech campus. Agbero dues collection is not placed without an evidenced motor park and locally reviewed context.

“How far?” as greeting, “Abeg” as a request and “No wahala” as reassurance/acknowledgement remain narrow lexical references from the earlier British Council source. New dialogue performance, code-switching, Yoruba/English/Pidgin balance and accent need fluent local review. No street-specific voice corpus or interview was obtained here. [M07]

Audio is spatially legible rather than uniformly loud. Proposed sources are tyre/body movement on Afariogun, directional speech near admitted gathering points, quieter side-street pockets and more continuous distant road energy toward the southern boundary. Do not call these mixes recorded local ambience.

Use obstruction for intervening walls, occlusion through doorways and distance attenuation per emitter. Doppler applies to moving horns/engines; a stopped bus horn does not sweep past the listener. Generator start, idle, load change and shutdown follow each verified unit's electrical state; a grid interruption need not start every generator simultaneously. Music requires an admitted source and a licensed/commissioned performance, including any Afrobeats or Fuji cues.

The 2020 account suggests a design concern—production and domestic rest can compete across a shared boundary—but does not supply acoustic levels, validated equipment, present operations or a universal community attitude. Keep residents' and operator's accounts distinct. Do not turn the allegations into a named pollution mechanic. [M04]

## 7. Weather, visibility and drainage

**Proposed, uncalibrated:** separate humidity, atmospheric aerosols, local dust, wet surfaces, rainfall and pooled water. Harmattan lighting uses distance-dependent contrast loss and sunlight scattering with tunable optical depth, not a constant orange grade or an assumption that every hazy photograph was taken during Harmattan.

Rain onset changes roof impacts, exposed-load protection, tyre spray, braking and shelter choices. Local water accumulates only where measured geometry, infiltration and drainage capacity support it. Store volume as inflow plus rainfall minus outflow/infiltration, with explicit boundary conditions. Do not use eight historical raster samples to connect drains or place a broad flood plane through thresholds.

NPC route cost considers water depth/velocity, obscured edges, mobility, load and refuge. Close a path when its measured conditions require it; do not trigger automatic district-wide flash floods for every storm. The cleaning photographs show standing water on one historic occasion, not rainfall timing, flood frequency, contamination or current drain function. [M03]

For worksite events, unsafe weather stops exposed work and prompts controlled clearance. A rain interruption may lengthen waiting but must not make agents enter water, climb a median or use private compounds as shortcuts.

## 8. Developer Asset Checklist

All production assets remain unbuilt. Research readiness is separate from placement approval, native integration, visual review and playable acceptance.

### 3D meshes and architecture

- [ ] S08-R01 — 19 source road ways, exact route subchains and shared Oshodi ownership reference.
- [ ] S08-F01 — 34 footprint rings; current envelopes, heights, entrances and public/private thresholds required.
- [ ] S08-J01 — Oshodi/Afariogun and side-street junctions from measured widths/turning envelopes.
- [ ] S08-J02 — southern one-way interface; no fabricated median crossing or ramp.
- [ ] S08-P01 — modular repair enclosure, cones, hand tools, aggregate containers and protective cover for a fictional crew.
- [ ] S08-V01 — referenced minibus/car/van classes; conditional keke/okada only after route approval.
- [ ] S08-D01 — surveyed drains and threshold connections; no channel generated from the historical photo alone.

### Material shaders and textures

- [ ] S08-M01 — maintained/worn render with individual repair history and receiving-surface masks.
- [ ] S08-M02 — road and patch candidates with scale-correct aggregate, dry/wet roughness and edge transitions.
- [ ] S08-M03 — roof/canopy details and runoff, assigned only after contemporary registration.
- [ ] S08-M04 — vehicle panel wear, door seals and rubber, without copying the photographed vehicle identity.
- [ ] S08-M05 — kerb/drain wetness with distinct standing water and flowing-water inputs.
- [ ] S08-X01 — photographic attribution/licence ledger, separate voice/music/branding/personality clearance.

### Dynamic systems and spawners

- [ ] S08-T01 — trip demand, exit-space reservation, defect avoidance and bounded queue propagation.
- [ ] S08-T02 — one-way boundary policy and whole-route diversion eligibility.
- [ ] S08-N01 — mobility/load-aware pedestrians, finite stock, optional bargaining and rest routines.
- [ ] S08-A01 — traffic/frontage audio zones, stateful generators and licensed music sources.
- [ ] S08-W01 — rainfall/aerosol/wetness separation and surveyed local water-storage model.
- [ ] S08-E01 — localized repair conflict, alternating passage when geometrically viable, safe cancellation and persistence.
- [ ] S08-Q01 — independent source, asset, native, visual and gameplay acceptance records.

## 9. Micro-narrative S08-E01 — one patch, two queues

**Fictional event inspired by historical repair context, not a recreation of the 2023 work.** Candidate corridor: the 78.2 m source subchain between Afariogun/Olusoji node 3745002376 and Afariogun/Jubril node 3745002384. Choose the actual enclosure only after verifying a work surface, pedestrian bypass, approach sightlines, side-street access and equipment refuge. Do not obstruct the intervening Ishola Anigbjumo junction. If those conditions cannot be met, withhold this placement.

A small crew protects a localized repair while two short queues approach. A crew member pauses a tool operation to admit one direction, then clears the corridor for the other. A driver with a time-sensitive trip may take the **557.3 m Olusoji–Adedeji–Jubril candidate alternate** if its entire route is approved; another chooses the short wait. The player can wait, follow an admitted pedestrian passage or accept optional directions. No task requires entering the enclosure or moving heavy equipment.

State sequence: `reserve enclosure/refuge/exits → establish stop points → tool operation → clear tool/person envelope → release one queue → clear corridor → release opposite queue → return to work or demobilize`. Proposed stop window 20–40 s, maximum interruption 120 s before safe reopening or cancellation. Exact vehicle release counts depend on measured storage, not a fixed invented lane capacity.

Admission requires a genuinely passable remaining lane. Otherwise no alternating-traffic version spawns. Emergency access, excessive queues, unsafe rain, blocked pedestrian passage or missing crew/equipment cancels the event with controlled clearance. Rain can require covering materials after tools are stopped; it is not an instant-flood trigger.

Persist crew identity, enclosure, tool state, queued trips, active reservation and completed work across streaming. No repeated-entry rewards, duplicated repair piles or resurrected queues. Acceptance requires finite recovery, conserved trips, clear pedestrian access and agreement between visible worksite, collision and route state. None has been implemented or played here.

## 10. Next adjacent work and readiness

Continue east via **Akin-Lawanson Street**, from Afariogun node **35785428** toward the southern transport frontage and the **Seinde Callisto** approach. Way **93156409** is the next discovery anchor; fresh topology and image checks must precede expansion.

This package supplies researched horizontal geometry and conditional systems. Contemporary camera registration, measured street/threshold/drain geometry, present traffic permissions, cultural consultation, rights clearance, canonical registration and all native/visual/playable work remain open. No blueprint is consumed by a renderer.
