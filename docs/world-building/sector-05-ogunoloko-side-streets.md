# Sector 05 — Ogunoloko Road and the Olatunbosun–Arewa–Temidire loop

October 11, 2026. Research and conditional production blueprint. No game assets or native content created.

## 1. Sector character and evidence

We continue southwest from Sector 04's western Adeyemi junction along Ogunoloko Road, preserving its mapped alternate name, Ogunlaja Lane. A northern loop through Olatunbosun, Arewa and Temidire supplies a smaller-scale contrast to the through road. The contrast is a design opportunity afforded by the mapped network, not a measured claim about traffic or neighbourhood demographics. [J01]

The scene concept is a working street with competing journeys: deliveries must reach a destination, residents need passage, and a queue cannot occupy space indefinitely. Small decisions about stopping, shelter and carrying loads create the energy. We do not manufacture a skyline transition, transplant colonial buildings, or assume that every frontage is a shop.

Evidence classes throughout: **mapped** = source tag/geometry; **reported/observed** = bounded publication or image evidence; **proposed** = authored design; **unresolved** = not approved for final placement. Numerical simulation settings are proposed. See the [evidence register](sector-05-sources.md).

Research rectangle: longitude 3.3315–3.3390 E, latitude 6.5555–6.5620 N. Complete selected ways extend beyond it; this is neither a clipped map nor a runtime sector boundary. The package retains 16 road ways, 23 shared-node pairs at 13 nodes, and 64 sampled building rings. [J01]

## 2. Exact horizontal network

| Junction/control | Longitude E | Latitude N | OSM node |
| --- | --- | --- | --- |
| Adeyemi / Ogunoloko / Raji Abayomi / unnamed connection | 3.3385073 | 6.5611962 | 1073319565 |
| Ogunoloko / Wahab Larinde | 3.3374771 | 6.5608953 | 1073318067 |
| Ogunoloko / Olatunbosun | 3.3368793 | 6.5606530 | 1073319363 |
| Ogunoloko / unnamed way 92451165 | 3.3362508 | 6.5603753 | 1073316181 |
| Ogunoloko / Temidire | 3.3356446 | 6.5601349 | 1073317387 |
| Ogunoloko / Dosunmu | 3.3350552 | 6.5598959 | 4395533004 |
| Ogunoloko / unnamed way 441859078 | 3.3347854 | 6.5597856 | 4395533006 |
| Ogunoloko / unnamed way 92451167 | 3.3342183 | 6.5595714 | 1073317343 |
| Ogunoloko / Baderin and two unnamed branches | 3.3330860 | 6.5590862 | 4395533108 |
| Olatunbosun / Arewa | 3.3360668 | 6.5624568 | 1073316485 |
| Arewa / Temidire | 3.3348366 | 6.5620097 | 1073318353 |
| Arewa / Ariyo Close | 3.3354325 | 6.5622361 | 1073316516 |

Coordinates above are map records, not survey benchmarks. All junction memberships derive from shared node IDs. An unnamed branch stays unnamed; street names cannot be extended across a junction by guesswork. [J01]

Ogunoloko way **496763689 v12** is approximately **644.6 m** along its mapped line. Its ten-node sequence is retained intact in [road geometry](sector-05-roads.geojson). Its `secondary` class does not establish two lanes, a bus route, width or current access. `surface=unpaved` is a source assertion, not verified present-day condition. A September 13, 2021 report lists work on Ogunoloko and Baderin, without defining the treated geometry or today's condition. [J01, J02]

The candidate circuit is **Olatunbosun → Arewa → Temidire → Ogunoloko → Olatunbosun**, approximately **740.5 m**. Leg lengths are 220.2, 145.4, 226.8 and 148.1 m. The Arewa and Ogunoloko legs use subchains rather than their entire ways; exact nodes appear in [connections](sector-05-connections.json). This is a closed source graph, not an admitted pedestrian or vehicle route. It is not a bypass of Sector 04's bridge. [J01]

Preserve the three matching S04/S05 ways: Ogunoloko 496763689, western Adeyemi 589014131 and Raji Abayomi 535191784. Each needs one future runtime owner, not duplicate collision, traffic queues or sound emitters. Documentation overlap adds resolution rather than claiming new land everywhere.

## 3. Topography, architecture and engine coordinates

The 64-ring sample is centred on the Ogunoloko/Temidire node using an approximate 65 m selection radius. Every retained way has `building=yes`, version 1 and an April 23, 2024 edit timestamp. None establishes height, age, use, resident identity or whether the outline follows roof or wall. The date is an edit date, not a construction date. [J01]

Envelope workflow: preserve source ring → register contemporary imagery → resolve roof/wall outline → measure height and setbacks → establish public/private access → author openings and exterior detail. Do not distribute one photographed facade across unrelated rings. Keep source plan geometry separate from any temporary massing assumption.

No municipal survey, drain invert, kerb level, camber, flood surface or legal right-of-way has been obtained. The reported Ogunoloko flood photograph has no registered camera position here; it must not be assigned automatically to the nearest mapped ditch or to the Temidire junction. Keep S04's road/ditch grade separation intact. [J01, J03]

Survey priorities are the four loop corners, each proposed loading/shelter location, accessible frontage crossings, and drainage connections to the already-mapped eastern corridor. Capture crossfall, entrance thresholds, covers/openings and turning clearance. A coarse terrain raster cannot certify these small features.

Coordinate contract:

```text
evidence geometry = WGS84 [longitude, latitude], no altitude
proposed research projection = EPSG:32631
shared research origin = longitude 3.3510 E, latitude 6.5560 N
research source metres = (NorthDelta, UpDelta, EastDelta)
existing adapter = UE(X,Y,Z) = 100 * source(Z,X,Y)
research UE centimetres = (EastDelta, NorthDelta, UpDelta)
projected coordinates calculated = false
vertical datum = null
registration into canonical game world = null
```

Resolve projection and registration before any engine placement. No existing map is moved. Visual LOD never changes collision, road connectivity or public/private boundaries. Proposed review distances: 15 m for threshold/detail geometry, 50 m for simplified frontage and 150 m for exterior massing, adjusted by screen size. Keep junction silhouettes and access openings stable. These are authoring targets, not measured performance.

## 4. Traffic and go-slow blueprint

**Proposed systems.** Model Ogunoloko as the through-road spine and the loop as local access, subject to measured widths and dated permissions. Every trip owns a destination before spawning. Each directed link stores source way/subchain, allowed classes, clear width, stop eligibility, queue capacity and next-link reservation. Unknown access does not default to unrestricted routing.

- A vehicle enters a junction only when its swept envelope and exit storage are available. Queue propagation travels upstream; spawning pauses before capacity is exceeded. No vehicle disappears merely to make a jam look resolved.
- Permit a loading task only at a registered usable margin with a deadline and an exit path. Traffic slows because of occupied geometry, sightline or downstream storage, not a scripted congestion aura.
- Use measured wheelbase, steering limit, body width, mirrors and load clearance per vehicle. Separate steering/navigation from wheel contact and suspension. Collision and visual dimensions must agree at tight turns.
- Danfo remains an asset class requiring exact vehicle-model, livery, operating-route and stop references before local admission. Do not make every minibus a Volkswagen or assume a terminal on Ogunoloko.
- J03 visibly contains yellow tricycles with dark canopies. That is a bounded appearance reference, not authorization for present-day keke operation on every link. Do not use a green default. Okada admission, filtering and any restrictions remain pending current, route-specific evidence.
- Hawker agents require a verified trading location, a refuge and finite inventory. A candidate interaction begins only with vehicles effectively stopped: proposed speed ceiling 0.5 m/s, a reserved walking corridor and a return route. Abort when traffic restarts or refuge fails. Do not let bodies clip through vehicles or stock respawn at camera boundaries.
- Gala, plantain chips and packaged water are requested product candidates, not confirmed sellers or brands at this junction. Product form, packaging, placement and rights need their own references. Unverified branded stock remains out of the placed scene.

The northern loop can become an alternate route only when its complete access and capacity are admitted. Penalize through-traffic diversion where it would overwhelm local access. No forced shortcut through Ariyo Close, private compounds or unmapped gaps.

## 5. Material and architectural storytelling

**Proposed material families; placement conditional.** Adjacent Adeyemi images previously reviewed in S04 show concrete frontage drains/crossings, awnings, mixed upper-floor openings and overhead wires. They remain adjacent historical references, not verified Ogunoloko elevations. J03 adds a bounded view of a flooded road, small commercial frontages, poles/wiring and tricycle surfaces. Neither source establishes a complete architectural typology here. [J03, J04]

| Material/mesh family | Authoring rule | Evidence limit |
| --- | --- | --- |
| Rendered masonry | Separate coating, substrate, repair patches and runoff; retain maintained examples | Wall construction and exact facades need registration |
| Road repairs | Independently control aggregate, patch edge, roughness and water retention | J02 does not locate materials along the road |
| Corrugated roof/canopy | Model overlaps, fasteners and wind/sag response only after roof evidence | Do not assign rusted metal to every ring |
| Threshold/drain interface | Model measured openings, covers and walking surfaces with matching collision | Adjacent reference does not locate a local drain |
| Vehicle paint/canopy | Distinct paint, metal, rubber and fabric wear; panel-dependent roughness | J03 supports visible colour/form only |
| Rainwater | Depth/flow-driven refraction and ripples; sediment an independent input | No photo-derived numerical depth or contamination claim |
| Cables and equipment | Attach to registered poles/buildings; exhaust soot follows actual equipment | No random wire tangles or assumed generator yard |

Use restrained, causally placed wear: runoff below an outlet, abrasion at contact, patching where repaired. No blanket sepia/grime mask. Colonial remnants, brutalist structures, glass towers and gated estates require specific local evidence; none is inserted merely to satisfy a citywide mood board. MTN/Glo signage, campaign posters and real business names stay outside final assets until location/date/artwork are supported and reuse is cleared.

## 6. Spatial audio and NPC routines

**Proposed sound field:** moving engine/horn sources along the actual road; lower through-traffic audibility inside the northern loop through distance and obstruction, not an arbitrary zone-volume cut. Horn Doppler follows relative velocity; stationary queues do not sound like passing traffic. A corner occludes direct sound but does not mute the entire street.

At approved shop/equipment locations, use distinct speech, stock handling, shutter movement and mechanical sources. Generator state is `off → start attempt → running/load → shutdown`, tied to actual equipment and local power state; not every grid failure starts every unit simultaneously. License or commission Afrobeats/Fuji cues, then audition low-frequency transmission and interior/exterior occlusion. No music or crowd track is described as a field recording without one. Water sound follows simulated flow, not a permanently roaring drain.

Behaviour priority: `immediate safety → accessible route → committed work/journey → consented interaction → rest/social activity`. Give residents, traders, drivers, porters and visitors destinations, time budgets, stock and personal relationships. Negotiation responds to inventory, urgency and familiarity; aggression is not a trait assigned to market women as a class. Some people decline an interaction or continue their journey.

Proposed trade radius is 1.5–3 m, load/passage negotiation 2–4 m; crowded geometry can reduce or prevent activation. Park-dues collectors/agberos require a verified motor-park context and locally reviewed institutional behaviour; no dues node is invented here. Tech workers can have individual journeys without inventing a technology cluster or a costume-coded faction.

The previously read British Council guide supports the narrow lexical meanings of “How far?”, “Abeg” and “No wahala”. Proposed contexts are a familiar greeting, request and acknowledgement respectively. Fluent local review must approve register, pronunciation and performance. Do not assign all speakers one accent or force a phrase into every exchange. Route calls require evidenced destinations and recordings with consent. [J04]

## 7. Rain, Harmattan and navigation

**Proposed, uncalibrated weather design.** Separate aerosol concentration, humidity, local dust, wetness, standing water and flow. The Harmattan candidate reduces distant contrast and warms direct light according to the atmosphere; avoid an unconditional orange overlay or dense ground fog. Dust particles appear where transport can raise them, not inside sealed rooms or over wet surfaces.

Rain first changes exposure, roof impacts, vehicle spray and shelter decisions. Wetness accumulates on exposed surfaces and dries at different rates under cover. Flood storage changes through rainfall/inflow minus infiltration/discharge/outflow. Unknown drains and levels stay unresolved: no instantaneous district-wide water plane is enabled.

J03 shows turbulent runoff and hazardous access in a reported incident, but supplies neither surveyed flow nor depth. Do not infer a universal rainfall-to-flood threshold. Local water meshes must respect measured basins, thresholds and obstructions. Navigation classifications consider depth, flow, hidden edges, visibility and mobility needs; no wheelchair user is routed through water simply because a source photograph depicts it. Represent access barriers without copying the pictured person or making their hardship a spectacle.

## 8. Developer Asset Checklist

All items below remain unbuilt. Evidence readiness, placement approval, native integration and acceptance are separate fields.

### 3D meshes and architecture

- [ ] S05-R01 — Ogunoloko ten-node source chain; measured width/section and stopping locations pending.
- [ ] S05-R02 — four-leg loop, preserving Arewa/Ogunoloko subchains and Ariyo branch; access review pending.
- [ ] S05-F01 — 64 retained footprint rings; resolve height, envelope, roof outline, openings and use per feature.
- [ ] S05-F02 — frontage threshold, cover/open drain, awning and shutter families; registered samples required.
- [ ] S05-V01 — vehicle-specific Danfo and optional keke/okada kits; dimensions, references and route admission required.
- [ ] S05-P01 — handcart, delivery parcels, reusable weather cover and tie-down poses for the event; no real brand copied.
- [ ] S05-P02 — poles, cable attachments and optional generator enclosure; verify equipment and exact anchors.

### Material shaders and textures

- [ ] S05-M01 — coating/substrate/repair/runoff stack with separate physical texture scales.
- [ ] S05-M02 — road and patch materials; current surface verification before assignment.
- [ ] S05-M03 — canopy fabric, painted metal, rubber and fastener materials with component-specific wear.
- [ ] S05-M04 — exposure/wetness/drying plus separate water depth, flow and sediment inputs.
- [ ] S05-M05 — haze/light-scattering and rain-impact variants, visually reviewed across daylight conditions.
- [ ] S05-X01 — signs, packaging, music, dialogue and imagery provenance/rights ledger; no cleared third-party assets supplied.

### Dynamic systems and spawners

- [ ] S05-G01 — single ownership of three S04 overlap ways; shared traffic/audio/reservation state.
- [ ] S05-T01 — destination-driven trips, downstream storage, swept-volume junction reservations and bounded loading.
- [ ] S05-N01 — finite-inventory trading, refuge-aware pedestrians, accessible routes and reviewed language performance.
- [ ] S05-A01 — velocity-driven horns, obstruction-aware road/shop sound, equipment state and licensed music.
- [ ] S05-W01 — measured basins/outlets and mobility-aware closures; rain does not automatically authorize flooding.
- [ ] S05-E01 — delivery-cover event with safe staging, bounded queue, cancellation and persistence.
- [ ] S05-Q01 — source-integrity, local-reference, asset, native, visual and gameplay readiness records.

## 9. Micro-narrative S05-E01 — securing a delivery before the rain

**Fictional event, not a reconstruction of J03.** Candidate anchor: Ogunoloko/Temidire node 1073317387. Final staging must be an evidenced public loading margin outside junction swept paths, drains and required pedestrian clearance. If no such margin exists, do not place the event here.

A porter approaching with covered parcels notices a loose tie as a shower begins. They pause in the reserved margin, ask a colleague to hold the cover, and secure the load. A waiting recipient renegotiates handover timing. Pedestrians pass single file only where the measured route permits; an eligible delivery vehicle waits at its reserved stop rather than blocking the junction. The player can wait, help hold the cover with consent, or take the admitted northern loop. No flood crossing is offered as the exciting option.

State sequence: `reserve staging/refuge/helper → approaching shower → pause and secure cover → resume handover → clear margin → release`. Proposed active duration 25–45 s, with a 60 s cap and safe cancellation. Emergency demand, unsafe water, unavailable helper, loss of clearance or full downstream storage prevents activation or triggers clearance. Do not spawn opportunistic sellers unless the local activity and inventory have been admitted.

Save parcel ownership, NPC IDs, event phase, deadline and reservations across streaming. Cover animation cannot create or destroy stock. Re-entering the sector cannot reset rewards. Acceptance requires observable pedestrian access, bounded queue recovery, correct cancellation and consistency between visible geometry and navigation—not merely an event timer.

## 10. Next adjacent sector and readiness

Next discovery corridor: **Baderin → Alhaji Mukandasi → Makinde**, leaving Ogunoloko at node 4395533108 through way 535191771. Re-fetch the network before declaring its loop/access; nearby Makinde and Mafoluku links are discovery leads, not pre-approved roads.

This pass completes a conditional research package. Current frontage registration, road/survey dimensions, dated vehicle permissions, hydrology, local cultural consultation and rights remain open. No renderer consumes these files; asset creation, native integration and playable acceptance have not occurred.
