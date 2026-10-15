# Sector 01 — Oshodi Transport Interchange

October 11, 2026. Research and conditional production blueprint; no level or asset implementation.

## 1. Sector identity and evidence

Our starting contrast is the interchange's large steel-and-cladding structures above a layered transport corridor, transitioning west along Oshodi Road into Sector 02's smaller street network. We retain the actual interchange rather than importing an Island skyline or inventing a colonial landmark. The reviewed 2020 and 2024 photographs establish a dated architectural reference, not the condition of every structure today. [E01–E04]

Evidence keys refer to [the source register](sector-01-sources.md). **Mapped** means an OSM record, **historical** means a dated observation or planning document, **proposed** means our authored simulation choice, and **unresolved** means placement authority is missing. All numeric behavior and rendering settings below are proposed, not measured Lagos norms.

The refined research window is west 3.3485, south 6.5510, east 3.3550, north 6.5595, in WGS84 degrees. It narrows the initial conversational study window around the interchange; it is not a municipal boundary. Complete ways in the geometry file extend beyond the query rectangle and do not enlarge sector ownership. [E01]

## 2. Spatial blueprint

| Component | OSM record | Treatment |
| --- | --- | --- |
| Terminal 1 | way 693552184 v8 | Separate mapped footprint; final heights unresolved |
| Terminal 2 | way 693552185 v5 | Separate mapped footprint; do not merge with Terminal 1 |
| Terminal 3 | way 693552186 v3 | Separate mapped footprint across the corridor |
| Oshodi station | node 12260658336 v1 | Longitude 3.3514068, latitude 6.5572533; point is not an entrance |
| Agege Motor Road | ways 133805221 v62 / 746512955 v44 | Separate one-way carriageways; preserve source node order |
| Apapa-Oworonshoki Expressway | ways 17505890 v7 / 134140054 v4 | Bridge segments tagged layer 1, not at-grade intersections |
| Oshodi Road | way 5153761 v23 | Shared with Sector 02; tagged two-way, paving stones |
| BRT Lane | way 1157247977 v1 | Tagged access=no, psv=designated; no generic car shortcut |
| Rail platforms | ways 1157247965 / 1157247966, both v2 | Separate platform polygons; access not inferred from polygon presence |
| Mapped rail alignments | ways 88675618 / 919669868 / 919669869 | Preserve each alignment; do not merge by proximity |
| Elevated footways | ways 560546961 / 560546965, layer 1; 936728640 / 936728641, layer 3 | Layer is ordinal topology, not metres or verified deck height |

The record names and tags above come from E01. Rail tags distinguish 1067 mm and 1435 mm alignments; these are mapping assertions, not surveyed clearances. The transport graph still lacks a complete, field-verified set of entrances, ramps, transfers and access restrictions. No line crossing creates a connection without a shared node and compatible vertical/access semantics.

### Sector 02 handoff

Use shared OSM node **1079849258**, longitude **3.3497770**, latitude **6.5564935**, at Oshodi Road/MacCarthy Lane. The full Oshodi Road way 5153761 is retained in both research packages for comparison, but must have one owner in any future runtime. Compare geometry, node IDs and versions before import. Sector 02 supplies the westward loop; this package supplies the approach eastward toward the transport corridor. Do not infer a walkable route from that connection into any terminal interior. [E01]

Proposed ownership is by stable source-feature ID, not by bounding-box clipping. The future streamer transfers pedestrian identity, vehicle queue position, work goals and audio emitter state at a cell edge. It must not duplicate a road, reset a transaction, or teleport an agent onto a different deck.

### Topography and collision

The ESIA says a total-station survey and DTM informed design, but this package does not contain the DTM. Its planning drainage gradients and regional climate figures are not as-built kerb levels. Final road crossfall, bridge clearance, steps, drain inverts and absolute heights remain unresolved. The ESIA's four-storey proposal conflicts with OSM building:levels=5; neither is converted to a guessed height. [E01, E02]

Proposed geometry rules:

- Keep road, rail, platform, elevated footway and terminal surfaces separately classified. Never flatten them into one navigation plane.
- Road collision follows measured cross-sections; bridge collision includes deck thickness, soffit and barriers. Supports must have actual contact rather than floating through a deck.
- Stairs, ramps and lifts require measured dimensions and confirmed access. An unverified lift is not an accessibility route.
- Cable/truss silhouette matters visually; player collision belongs to meaningful solids and guardrails, not every thin cable.
- Missing elevations stay missing. A zero-altitude preview cannot pass geographic acceptance.
- Terrain resolution is driven by its source accuracy. No regional DEM may synthesize centimetre-scale drain geometry.

### Engine coordinates

Retain geographic coordinates as `[longitude, latitude]`. Proposed isolated research projection is WGS84 / UTM 31N, EPSG:32631, with common origin 6.5560 N, 3.3510 E. Compute Easting/Northing with a reviewed CRS library before use. No projected coordinates have been calculated in this package.

Define research source metres as `(NorthDelta, UpDelta, EastDelta)`, then use the existing canonical adapter `UE(X,Y,Z) = 100 * source(Z,X,Y)`. The research frame consequently becomes centimetres `(EastDelta, NorthDelta, UpDelta)`. The separate registration into our existing game remains null. Vertical datum also remains null. Do not move canonical assets or directly feed degrees into the metre adapter.

## 3. Traffic and go-slow system

**Proposed system:** downstream capacity, boarding dwell, merge contention and route demand produce queues. Congestion is a consequence of obligations, not a decorative ring of immobilized buses around the player.

Each lane needs `{sourceWay, direction, layer, widthEvidence, permittedClasses, policyDate, downstreamCapacity, stopReservations}`. Unknown permissions deny final traffic admission. Agege Motor Road directions follow the mapped one-way ways; expressway decks do not exchange traffic with ground streets without a separately mapped and verified connector. The one captured BRT lane is not a complete bus circulation graph. [E01]

- **Danfo controller:** commissioned vehicle references determine model, dimensions, yellow paint/livery and wear. Do not label every yellow minibus a Volkswagen without evidence. Route choice, passenger destination and stop reservation precede dispatch. Doors/boarding synchronize with a stopped state. Use an individual horn cooldown, not a repeating sector horn loop.
- **Keke and okada controllers:** prepare swept-volume, turning and gap-selection logic, but leave sector spawners disabled until current legal restrictions and local operating evidence are reconciled. Green tricycles are not the default simply because the brief requested them.
- **Formal buses:** preserve authorized bus-only circulation and boarding areas; do not share a stop merely because a Danfo is nearby. The operator schedule currently lists Oshodi on the Red Line, but supplies neither a bus timetable nor proof of an accessible station transfer. [E05]
- **Queue tuning:** initial candidate headway 1.2–2.4 s, acceleration cap 1.0–2.0 m/s², and stopped-gap target 1.0–2.5 m. These require later tuning by vehicle geometry and footage; they are not observational estimates.
- **Hawker logic:** seek an eligible stopped customer only below 1 m/s for 6 s, with a free retreat corridor. Reserve one approach slot, offer once, update finite stock, retreat when the gap closes. Never spawn on an expressway deck, rail track or inside a moving bus. These are safety-oriented design constraints, not assertions about observed practice.
- **Merchandise:** water, plantain chips and Gala are requested candidates, not verified inventories at a particular stop. Packaging, brand permission and site placement remain pending. No invented shop or price is published as real.

Vehicles use their actual wheelbase and swept envelope; pedestrians use capsule avoidance plus personal-space reservations. Physical collision remains the last safeguard, not the routine crowd controller. Cell streaming keeps authoritative IDs, queue order and transactions intact.

## 4. Architectural grammar and materials

The two reviewed photographs show large curved terminal envelopes, exposed steel trusses, tall lattice supports, cable-linked elevated bridges, panelled/clad surfaces, glazing, road barriers and multiple transport levels. Reproduce those relationships before distressing surfaces. The photographs do not provide a complete dimensioned façade survey. [E03, E04]

Proposed build order: source footprint → independently evidenced envelope → structural supports/deck relationships → openings/access → materials → service details → temporary objects. A generic rectangular shell with a photographic sign does not satisfy terminal fidelity.

| Material family | Proposed shader/geometry rule | Evidence gate |
| --- | --- | --- |
| Steel truss, lattice tower, cables | Separate member scale, bolts, contact dirt and coating; preserve thin silhouettes in LOD | Dated shape reference available; dimensions pending |
| Terminal cladding | Physical panel layout, restrained roughness variation, independent dirt mask | Panel appearance visible; precise module size pending |
| Glazing | Separate glass from opaque infill; preserve mullions and view-dependent reflections | Visible façade reference, not interior survey |
| Concrete decks/pillars | Geometry for edges and joints; runoff below actual water paths | Exact support plan, clearances and decay pending |
| Asphalt/paving | Independent wear, repairs, deposited dirt and wetness | OSM tags recorded; current local condition unverified |
| Rail/platform surfaces | Distinct track and pedestrian materials; no inferred at-grade crossing | Geometry mapped, access and dimensions pending |
| Generators and soot | Source-attached exhaust deposits, shielded low-frequency emitter | ESIA proposes standby systems; installed equipment/location not proven |
| Posters, billboards, cables | Explicit dated placement and artwork provenance | Withheld until geolocated/cleared; no procedural political posters |

Do not mix colonial, brutalist and glass architecture into every cell. This sector's verified contrast is its transport structure against neighbouring street fabric. Unverified luxury estates, skyline towers and hanging cable bundles remain absent.

Proposed asset budget: separate hero terminal envelopes from repeatable members; use instances for repeated validated modules. Candidate LOD evaluation distances are 25, 80 and 200 m, adjusted by screen size and silhouette rather than accepted as performance results. Keep deck collision/navigation stable across visual LOD changes. No frame-rate claim is established.

## 5. Spatial audio blueprint

**Proposed mix; no field sound survey is supplied.** Vehicle emitters move with the vehicle and derive Doppler from relative velocity. Use elevation-aware obstruction: an overhead horn is not colocated with a pedestrian under the flyover. Transition between exposed road, under-deck space, covered footbridge and concourse using measured geometry when available.

- Near voices: positional, conversational, individually rate-limited. A destination call requires an evidenced operating route and local script review.
- Bus engines/horns: separate idle, acceleration, braking and horn events; vary recorded takes, not arbitrary pitch caricatures.
- Footbridge: footfalls and trolley joints follow deck contact. Reverb zones follow the enclosure, not an entire bounding rectangle.
- Generators: start attempt → ignition → load settle → running → shutdown, only at an evidenced source. A grid event schedules independently delayed changes; it does not start an imaginary generator behind every fence.
- Shop music: commissioned/licensed Afrobeats or Fuji candidates, assigned only to supported premises. Occlusion attenuates high frequencies more strongly than low bass; do not use commercial recordings without clearance.
- Rain: exposed roofs, sheltered roofs, gutter discharge, road spray and footsteps are distinct buses. Sheltered dialogue remains readable.

Initial mix caps: 24 nearby speech emitters and 16 detailed vehicle emitters, with distant aggregation and priority for player interaction. These are proposed budgets, not measured capacity. Subtitle wording and direction cues need their own review.

## 6. NPC routines and cultural review

**Proposed roles, not claims about named residents:** commuters, drivers, conductors, traders, porters, maintenance workers and visitors. Work, household obligations, rest and social ties coexist. Occupation and personality are independent of gender, clothing, ethnicity and wealth.

Priority tree: `immediate hazard → access/shelter → reserved journey/work → agreed interaction → incidental conversation/rest`. A commuter holds a destination and trip budget; a trader holds stock and reservation state; a driver holds route and boarding obligations. Agents can decline, wait, renegotiate or leave without becoming hostile by default.

Initial interaction envelopes: 1.5–3 m for a transaction, 2–4 m for a delivery negotiation, 3–6 m for a directional call. These are design values. Park-dues collection is withheld until the exact institution, authorization, location and practice are evidenced; “agbero” is not a universal criminal faction. Do not seed a Yaba-style tech-worker cluster into a terminal merely to fill a faction quota.

“How far?”, “Abeg” and “No wahala” remain user-requested script candidates. Their exact conversational use, pronunciation, code-switching, speaker consent and recording require fluent local review. The package makes no claim that an unreviewed generated line is authentic Pidgin. Negotiation uses stock, urgency and relationship state, not a gendered aggression setting.

## 7. Seasonal weather and volumetric lighting

The ESIA discusses a wet/dry regime and humidity using historical meteorological references. Those values are not a current forecast or proof that this particular pavement floods. Its drainage discussion includes relatively drained conditions during investigation; instant universal flooding would contradict the need for local evidence. [E02]

**Proposed art direction:** humid conditions use reduced distant contrast and controlled aerial scattering, not permanent white fog. A Harmattan candidate uses a separate aerosol variable, muted distant colours and warmer direct light; local dust sources remain distinct from regional haze. Exact seasonal visibility and colour calibration await dated local references.

Rain state sequence: dry → first wetting → sustained rain → drainage response → recession → drying. Exposure masks shelter surfaces under bridges. Wind drives droplets consistently; roof runoff appears at actual edges; tyre spray needs speed and surface water. Avoid an orange post-process over all weather states.

Standing water follows `storage change = rainfall + inflow − discharge − infiltration − outflow`. Basin surfaces may be dynamic water meshes, but cannot be one citywide horizontal plane. Rain alone does not select a water height. Drain capacities, ground levels and blockage probability stay unresolved until surveyed.

Navigation changes are event-driven by reviewed depth/flow/visibility classes; thresholds are not invented as human safety limits. The weather system must preserve a confirmed alternate route or decline to activate its obstructive event. Wetness and puddle shader changes alone do not establish a playable flood system.

## 8. Developer Asset Checklist

All items remain unimplemented in this package. “Reference-ready” does not mean mesh-ready or approved for placement.

### 3D meshes and architecture

- [ ] Three individually traced terminal footprints/envelopes — mapped polygons and dated silhouette references ready; heights unresolved.
- [ ] Steel truss, lattice tower and cable bridge kit — dated visual references ready; measurements/connection details pending.
- [ ] Separate expressway decks, supports and barriers — mapped deck alignments; as-built cross-sections pending.
- [ ] Footbridge/ramp/stair kit — mapped elevated ways; exact access and accessibility pending.
- [ ] Distinct rail alignments and two platform meshes — mapped source geometry; station entry and platform details pending.
- [ ] Oshodi Road seam module — shared source way retained; widths/crossfall and safe walking corridor pending.
- [ ] Bus/van/car references, wheels, doors and passenger sockets — model-specific dimensions and rights pending.
- [ ] Trader carry/display props, goods and utility equipment — local placement and reference verification pending.

### Material shaders and textures

- [ ] Steel coating, cladding, glazing and concrete material families with physical scale controls.
- [ ] Road finish variants; no current pavement claim from source tags alone.
- [ ] Separate dust, runoff, wetness, standing-water and drying masks.
- [ ] Contact dirt at structural joints, not indiscriminate rust everywhere.
- [ ] Decal registry with source date, exact location, artist/owner, rights and approved texture use.
- [ ] Reference ledger retaining attribution for CC BY-SA images; no baked-in reuse until rights workflow is resolved.

### Dynamic systems and spawners

- [ ] Layer-aware graph and source-feature ownership across S01/S02.
- [ ] Route/access-policy admission with dated evidence; unknown permissions closed.
- [ ] Finite boarding, stock, journey and loading reservations.
- [ ] Congestion response and stopped-customer interaction with retreat checks.
- [ ] Licensed localized speech/music; per-source horn and generator states.
- [ ] Shelter/exposure masks, basin water balance and navigation change events.
- [ ] Streaming persistence, event cancellation and prevention of duplicated inventory/payment.
- [ ] Separate visual review and gameplay acceptance; neither is completed by this checklist.

## 9. Micro-narrative event S01-E01 — the covered transfer

**Fictional authored event, not a report of a real incident.** Candidate anchor is the shared Oshodi Road/MacCarthy node 1079849258. Final staging offsets require a verified public loading/shelter area nearby; no real shop or terminal entrance is invented.

An approaching shower interrupts a small covered delivery. A porter pauses while a loose cover is secured. Two other pedestrians yield, narrowing a confirmed passage temporarily. The player can wait, help hold the cover, or follow the already verified Sector 02 detour. The disruption comes from occupied space and people cooperating, not compulsory violence or manufactured poverty.

Activation requires an unoccupied approved staging polygon, reserved porter/cart/NPCs, enough public width for the scripted state, and a confirmed alternate route. No drain flood or broken Danfo is invented to force the event. If evidence or reservations are absent, the event is disabled.

State machine: `reserve → rain cue → secure load → optional interaction → release passage → delivery resumes`. Proposed pause 20–45 s; maximum occupation 60 s before a safe reset. Help changes a fictional relationship state, not a real-world claim. Any compensation must use existing authoritative economy rules, never a client-spawned balance.

Cancel on emergency access, player disconnect, lost route, excess crowding or conflicting use. Persist event ID, cargo count, reservations and completed interaction; release them exactly once. Crossing the sector edge cannot reset the goods or reward.

## 10. Acceptance and next sector

Research geometry can advance independently of missing placement evidence. Final 1:1 acceptance requires surveyed registration/vertical datum, as-built heights and access, current vehicle policy, geolocated contemporary imagery, local cultural review and rights records. These are open gates, not completed checks.

Next expansion remains the physically connected northern Igbehinadun continuation from Sector 02, starting with a fresh check of node 4395533093 and candidate way 1442626891. We do not jump to Apapa or Surulere and call it adjacency.
