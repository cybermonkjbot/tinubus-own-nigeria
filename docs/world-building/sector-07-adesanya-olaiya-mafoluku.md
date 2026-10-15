# Sector 07 — Adesanya, Olaiya and Mafoluku drainage crossings

October 11, 2026. Conditional research and production blueprint; no game assets or native content created.

## 1. Sector character

We continue from Sector 06's Amusa/Adesanya junction toward three mapped road crossings of the same ditch: Adesanya in the north, Olaiya in the middle and Mafoluku in the south. Rufai joins the eastern approaches, while unnamed links connect the western streets. This makes the sector about **crossing capacity, frontage access and choices between connected streets**. [L01]

Our proposed contrast is spatial and acoustic: local residential access on Olaiya, movement along the mapped secondary Mafoluku/Oshodi axis, and the separate drainage corridor below the bridges. The map's road classes suggest this design hierarchy but do not establish actual traffic volumes, bus routes or noise levels. There is no evidence here for a sudden transition to a modern glass skyline; it is not inserted as scenery.

Evidence classes: **mapped** = source geometry/tag; **observed/reported** = bounded imagery or publication; **proposed** = authored systems; **unresolved** = insufficient for final placement. All simulation settings below are proposed. Read the [source register](sector-07-sources.md) alongside this blueprint.

## 2. Street layout and topology

Study rectangle: longitude 3.3305–3.3405 E, latitude 6.5540–6.5595 N. The [network file](sector-07-network.geojson) retains nineteen complete road ways and ditch way 92451100. Complete source ways extend outside the query rectangle; this is neither clipped geometry nor a runtime boundary. Twenty-four road shared-node pairs occur at twenty-four unique nodes: six same-road joins and eighteen cross-street pairs. [L01]

| Control | Longitude E | Latitude N | OSM node |
| --- | --- | --- | --- |
| Amusa / Adesanya; S06 interface | 3.3318778 | 6.5566101 | 1073317008 |
| Adesanya / unnamed link toward Olaiya | 3.3349852 | 6.5577734 | 4395533010 |
| Adesanya bridge west end | 3.3383431 | 6.5585843 | 5624392796 |
| Adesanya bridge east end | 3.3384591 | 6.5585716 | 5624392798 |
| Adesanya / Daudu | 3.3399555 | 6.5582117 | 1079849451 |
| Daudu / Rufai | 3.3399403 | 6.5580871 | 5623442588 |
| Olaiya / unnamed link toward Mafoluku | 3.3358639 | 6.5574353 | 4395533012 |
| Olaiya bridge west end | 3.3379153 | 6.5573911 | 11876557994 |
| Olaiya bridge east end | 3.3380191 | 6.5573874 | 11876557995 |
| Olaiya / Rufai | 3.3397774 | 6.5572686 | 1079925522 |
| Rufai / Oshodi Road | 3.3396715 | 6.5566793 | 35785464 |
| Mafoluku bridge west end | 3.3377130 | 6.5566888 | 5624643863 |
| Mafoluku bridge east end / Oshodi source-way join | 3.3378710 | 6.5566944 | 5624392795 |

These are map records, not survey benchmarks. [L01]

### Three crossings, one drainage corridor

| Crossing | Road way/version | Approximate mapped road-line length | Derived 2D ditch intersection, lon/lat |
| --- | --- | --- | --- |
| Adesanya | 588965956 v2 | 12.9 m | 3.3384183, 6.5585761 |
| Olaiya | 1279347656 v1 | 11.5 m | 3.3379594, 6.5573895 |
| Mafoluku | 589001491 v6 | 17.5 m | 3.3378147, 6.5566924 |

Each road carries `bridge=yes`, `layer=1`. None shares a node with the ditch. The intersections are calculations between source line segments, **not road junctions or surveyed points**. The lengths are two-node road polylines, not certified clear spans, deck dimensions or widths. Source layer ordering cannot supply metres of clearance. [L01]

Ditch node order is not verified flow direction. Do not name the watercourse, assume tidal connectivity, allow canal navigation or draw pedestrian stairs down to it without evidence. The three crossings may share hydrological exposure; they are not three independent guarantees of access during a storm.

### Candidate circuits

The northern circuit is **Adesanya → short Daudu link → Rufai → Olaiya → unnamed way 441859080 → Adesanya**, approximately **1,256.3 m**. It crosses the ditch on Adesanya and Olaiya.

The southern circuit is **Olaiya → Rufai → Oshodi/Mafoluku → unnamed way 441859081 → Olaiya**, approximately **981.1 m**. It crosses on Olaiya and Mafoluku. Exact way subchains and ordered nodes appear in [connections](sector-07-connections.json); complete way lengths must not replace these subchains. [L01]

These are closed graph circuits, not approved travel routes. Never claim a safe diversion from geometry alone. Check the full alternate, both crossings, current access, turning clearance and downstream storage before presenting it. Neither circuit has been verified as a bypass of Sector 04's bridge.

### Shared world ownership

Fresh comparison matches S06's Amusa 92451057, Adesanya 92451090 and unnamed link 93156426. Daudu 8564505 and ditch 92451100 match S04. Geometry, node sequences, node versions and way versions agree. Assign one future runtime owner to each feature, with references from other sectors rather than duplicate bridge collision, water simulation, traffic queues or emitters. [L01]

## 3. Buildings, topography and engine registration

The [Olaiya-crossing sample](sector-07-buildings.geojson) retains 25 closed footprint ways within an approximate 65 m representative-point selection radius. Twenty-four have `building=yes`; way **1275302521 v2** has `building=construction`, last edited **May 3, 2024**. That tag does not establish construction activity in October 2026. No sampled footprint has a verified height or use. [L01]

Preserve source rings, then resolve roof versus wall outline, register contemporary imagery, measure height/entrances and establish public/private access. Do not extrude arbitrary floor counts, place a household because a frontage looks residential, or import a real address from a street-level advertisement into the nearest polygon.

A May 26, 2021 advertisement describes flats on Olaiya but provides no verified parcel registration in this package. A June 23, 2019 Adesanya report includes a demolition photograph. Neither approves a current building placement: the listing is a seller's assertion, and the damaged structure is historical rather than current scenery. [L02, L03]

Eight fresh SRTM nearest-neighbour samples range from 8 to 26 m in the source's EGM96 datum. Paired bridge-end samples are Adesanya 12/12 m, Olaiya 12/13 m and Mafoluku 8/8 m. These coarse raster values are **not deck elevations**. Equal endpoint samples do not prove a level deck; the Olaiya difference is not a measured slope. Regional raster context stays in a [separate record](sector-07-terrain-context.json), excluded from collision, grade and flood calibration. [L04]

Required measurements are bridge approaches/decks/soffits, parapets or other actual edge protection, channel sections, bank/threshold levels, drain connections, clear pedestrian widths and vehicle swept paths. The historic photograph supplies none of these crossing dimensions. Do not create support columns, culverts, railings or floodwalls merely because a bridge symbol exists.

Coordinate contract remains:

```text
Evidence: WGS84 [longitude, latitude]; road/footprint geometry has no altitude
Proposed research projection: EPSG:32631
Shared research origin: longitude 3.3510 E, latitude 6.5560 N
Research source metres: (NorthDelta, UpDelta, EastDelta)
Existing adapter: UE(X,Y,Z) = 100 * source(Z,X,Y)
Research UE centimetres: (EastDelta, NorthDelta, UpDelta)
Projected coordinates calculated: false
Engine vertical datum: null
Registration into canonical game world: null
```

Source EGM96 in the terrain record does not resolve engine datum. Do not feed degrees into the metre adapter or move existing world assets. Relevant projection packages were unavailable locally; no installation or handwritten approximate projection was substituted.

## 4. Traffic and pedestrian simulation

**Proposed systems:** separate the street graph from admission policy and geometric clearance. A road link carries way/subchain identity, eligible modes, direction, measured width, turning envelope, stopping permissions, dated policy evidence and queue capacity. Missing values remain unknown rather than permissive defaults.

- Spawn from a trip purpose and destination, with a valid downstream path. Suppress demand when storage is full instead of stacking overlapping vehicles.
- Reserve the crossing plus its exit space before entry. Hold queues on approved approaches, not across the bridge or an intersecting street. Do not invent alternating one-lane control unless the measured geometry or the fictional incident warrants it.
- Use vehicle-specific wheelbase, overhang, mirrors, steering limits and braking/contact behaviour. Vehicle body and collision must agree. A bridge's mapped length does not identify its vehicle clearance or load limit.
- Danfo remains a reference-dependent vehicle class: model, yellow livery details, operating routes and actual stops need evidence. Do not create a terminal or assume every minibus is a Volkswagen. Long-distance calls require actual destinations, not a random list of Lagos landmarks.
- Keke and okada need current route-specific admission. No green-tricycle default, unrestricted filtering or automatic pedestrian-gap use. Historical images elsewhere do not settle today's restrictions here.
- Hawkers require locally evidenced activity, finite stock, a legal/admitted standing area, clear refuge and a reserved return path. Proposed interaction trigger: effectively stopped vehicles below 0.5 m/s, with restart detection and adequate sightline. No hawker spawns over the ditch or crosses vehicle colliders.
- Gala, plantain chips and cold packaged water are requested product candidates, not established stock at these crossings. Packaging, sales location and branded artwork remain reference/rights work.

Pedestrian route cost accounts for load, mobility, shade, crowding and access—not just shortest distance. Bridge edges and private thresholds are hard constraints when established. A queue must leave an admitted pedestrian route available; if none exists, do not activate the staged interruption. No storm reroute sends people through an unmeasured channel or private yard.

## 5. Architecture, material layers and LOD

The 2019 Adesanya image visibly contains painted/rendered wall surfaces, window openings, exposed roof timbers, corrugated sheets and demolition debris. These are bounded historical material observations from an emergency scene, not a local prevalence survey or an asset template for the 25 Olaiya footprints. Do not reproduce the demolition, the pictured people or the real address as a playable disaster. [L02]

**Proposed asset families:** maintained and worn render, small frontage openings, roof junctions, bounded repair patches, bridge approach surfaces and accurately measured drainage interfaces. Keep material chronology object-specific: paint can be fresh beside an older wall; repairs need not match surrounding aggregate. A `construction` tag does not authorize scaffolding, workers or exposed rebar today.

| Layer | Authoring rule | Placement evidence needed |
| --- | --- | --- |
| Render/paint | Separate coating, substrate, exposure and runoff masks | Registered current facade; no global peeling-wall preset |
| Roofing | Sheet laps, fasteners, timber/metal supports and drainage at correct scale | Actual roof form and material per footprint |
| Street surface | Independent asphalt, paving and repair candidates | Current surface; do not materialize every OSM split as a visible seam |
| Bridge approaches | Distinct deck/approach joints only if observed | Surveyed geometry, edge protection and clearance |
| Channel/bank | Solid banks, openings and measured channel section | No automatic concrete lining, rubbish, sewage colour or oil film |
| Equipment/cables | Attach cables to verified supports; soot follows evidenced exhaust | No invented generator compound or random wire tangles |

Source asphalt/unpaved tags are retained, including Olaiya's unpaved bridge tag, but are not approved contemporary surface assignments. Colonial remnants, 1970s brutalism, luxury gates and glass facades are not transplanted here without specific references. Real signs, campaign posters, MTN/Glo graphics and business names require exact date/location/artwork clearance. An editorial portrait in a report is not a street decal.

Proposed detail bands: near-field approach/drain detail within about 15 m, simplified frontage/roof geometry through about 50 m, then silhouette-preserving massing with screen-size review. Collision, passage openings, bridge support topology and water barriers never switch with visual LOD. Use proxy collision only after it accurately represents measured solid surfaces. No performance or rendered acceptance is claimed.

## 6. NPC routines and ambient audio

**Proposed routines:** commuting, deliveries, household errands, trading, waiting and rest. Agents store destination, load dimensions, deadline, stock, social relationship, safe refuge and interaction consent. Priority is `immediate safety → accessible passage → committed work/journey → optional negotiation → social activity/rest`. Avoid a permanent queue of shouting vendors or a population that only reacts to the player.

Negotiation depends on stock, timing and familiarity, not gender. Do not make market women uniformly aggressive. Tech workers may pass through without a fabricated local cluster. Agbero/dues-collection roles require a verified motor-park context and locally reviewed behaviour; no park or dues node is placed by assumption.

Proposed interaction ranges are 1.5–3 m for trade and 2–4 m for passage negotiation, reduced or disabled when geometry is crowded. The carried-forward British Council guide supports the narrow meanings of “How far?”, “Abeg” and “No wahala”; greeting/request/acknowledgement contexts need fluent review and recorded performance consent. A lexical guide does not establish an Oshodi accent or universal language choice. [L05]

Audio follows emitters and state. Keep traffic above each crossing distinct from channel sound below. A dry or still channel is not a continuous rushing-water loop. Doppler follows relative motion; stationary horns remain stationary. Facades and corners occlude speech/high-frequency traffic differently from lower-frequency mechanical sound, subject to listening review.

Generators start only at verified equipment positions and follow each unit's power/load state. Shop music, including any Afrobeats/Fuji references, must be licensed or commissioned and positioned at admitted sources. Grid failure changes powered sources, not the entire city bed at once. Do not label designed ambience a field recording; no local audio dataset was acquired in this pass.

## 7. Rain, humidity and Harmattan

**Proposed weather system, not calibrated local climate:** maintain separate aerosol haze, humidity, local dust, surface wetness, standing water and channel flow. Harmattan art direction uses distance-dependent contrast loss and atmospheric sunlight scattering, not an unconditional orange filter or permanent street-level fog. Loose dry surfaces can raise dust; wet bridge decks do not emit a dust cloud by default.

Rain first affects exposure, roof/canopy impacts, tyre spray and shelter choices. Flooding requires measured storage and connected inflow/outflow. Never stretch one water plane through buildings and all three decks. Store rainfall/inflow minus infiltration/discharge/outflow per admitted basin/channel element; unknown boundaries cannot silently receive a default flood depth.

Each crossing gets an independent navigation state from measured local water/clearance conditions, while shared channel conditions can correlate failures. An unavailable Olaiya crossing does not prove Mafoluku is safe. Closure/reopening must consider depth, velocity, visibility, hidden edges and mobility needs. Keep notification and route availability synchronized with collision/water geometry.

## 8. Developer Asset Checklist

All listed assets/systems are unbuilt. Evidence readiness, placement approval, native integration, visual review and gameplay acceptance are separate records.

### 3D meshes and architecture

- [ ] S07-R01 — nineteen road source ways with exact circuit subchains and single-owner overlap records.
- [ ] S07-B01 — Adesanya crossing; deck/edge/support survey required, not inferred from its 12.9 m source line.
- [ ] S07-B02 — Olaiya crossing; measured access and approach geometry required.
- [ ] S07-B03 — Mafoluku crossing and Oshodi source-way transition; no invented physical road-name seam.
- [ ] S07-D01 — linked ditch corridor and bridge/channel interfaces; section, levels and flow boundaries pending.
- [ ] S07-F01 — twenty-five footprint rings; construction-tag recheck and current envelopes/entrances pending.
- [ ] S07-P01 — unbranded delivery van, folding hand trolley and reusable crates for the fictional event.
- [ ] S07-V01 — referenced Danfo/van and optional keke/okada variants with per-class clearance and admission.

### Material shaders and textures

- [ ] S07-M01 — render/paint/substrate/repair masks; maintain clean as well as weathered variants.
- [ ] S07-M02 — physically scaled road candidates and approach joints, assigned only after current references.
- [ ] S07-M03 — roofing, canopy, cable and equipment materials tied to real supports/exposure.
- [ ] S07-M04 — bank/channel/deck wetness with separate flow, sediment and drying inputs.
- [ ] S07-M05 — near/far surface transitions that preserve measured collision and water barriers.
- [ ] S07-X01 — signage, packaging, music, voice and photographic rights ledger; no third-party assets cleared here.

### Dynamic systems and spawners

- [ ] S07-G01 — typed road/ditch separation; three derived intersections explicitly excluded from road routing.
- [ ] S07-T01 — exit-space reservations, trip demand, finite stopping windows and full-route diversion admission.
- [ ] S07-N01 — load/mobility-aware NPCs, finite inventories and locally reviewed dialogue.
- [ ] S07-A01 — road-above/channel-below emitters, stateful generators and licensed localized music.
- [ ] S07-W01 — measured channel/basin model; correlated crossing hazards without blanket closures.
- [ ] S07-W02 — coarse raster/source-datum record kept separate from engine survey and deck levels.
- [ ] S07-E01 — bounded loading-slot conflict with safe clearance, cancellation and stream persistence.
- [ ] S07-Q01 — independent source, asset, native, visual and playable acceptance records.

## 9. Micro-narrative S07-E01 — two deliveries, one loading margin

**Fictional, not a claim about a real shop or incident.** Candidate decision anchor: Olaiya/Rufai node 1079925522, east of the mapped channel. Final stopping and handover polygons must be on verified public/admitted surfaces, clear of the junction and bridge approach. If sufficient space is absent, this event is not placed here.

A small delivery van arrives while a previous delivery is still being counted. The recipient asks the second driver to wait; the first porter folds a hand trolley and clears reusable crates from the reserved margin. A short queue forms only if the measured lane geometry warrants it. The player can wait, pass along the admitted pedestrian route, or help return an empty crate with consent. Nobody must step into the ditch, cross a closed bridge or abandon stock.

State sequence: `reserve loading/refuge/exit → first handover active → second arrival held upstream → reconcile count → clear trolley/crates → release first vehicle → admit second`. Proposed delay 30–60 s, with a 90 s cap. No van enters a bridge until its downstream space is free. Lack of pedestrian passage, emergency demand, excessive queue, unavailable recipient or unsafe weather prevents activation or triggers safe clearance.

If a navigation suggestion is offered, evaluate the entire selected circuit and both relevant crossing states. Do not call the other side of the ditch a shortcut without admission. Queue, crate ownership, recipient identity, event phase and reservations persist across streaming; repeated entry cannot duplicate goods or rewards. The source geometry establishes the decision node, not the loading bay or shop identity.

Acceptance requires observed load conservation, pedestrian passage, bounded queue recovery, safe cancellation, and agreement between visible stopping positions, collision and route state. An authored event or a closed source circuit alone does not establish gameplay.

## 10. Next adjacent sector

Continue southeast via the Oshodi Road connection to **Afariogun Street**, beginning at node **35785462**, using way **5153764** as a fresh discovery lead. This connects the sector to another north–south street axis without teleporting to a distant district. Re-fetch its topology and imagery before extending it.

Current gaps: surveyed crossing sections, camera registration, building envelopes, current surfaces and transport permissions, hydrology, cultural consultation, rights and native production. This package is not imported by the game and no worker was restarted or directed.
