# Sector 04 — Adeyemi Street, Owoseni approaches and the ditch crossing

October 11, 2026. Research and conditional production blueprint; no native content created.

## 1. Sector character

Our expansion follows Adeyemi west from the Oyetayo/Church junction in Sector 03, through its Owoseni and Arowojobe connections, to a short mapped bridge and the Ogunoloko/Raji Abayomi junction. A southern circuit through Owoseni, Orona, Adekunle and Mabayoje supplies an additional local route. [H01]

This sector's defining relationship is **street → threshold → open drainage → building frontage**, with a separate mapped ditch beneath the western road crossing. Historical Adeyemi photographs show small drain crossings beside occupied frontages; the map supplies the ditch/bridge alignment. These are two different evidence scales: we do not identify a photographed frontage drain as the mapped ditch. [H01, H02]

The research rectangle is longitude 3.3380–3.3455 E, latitude 6.5570–6.5640 N. Complete source features extend beyond it. Sector IDs and boundaries are research organization, not canonical game districts, administrative boundaries or generated walls.

Evidence vocabulary: **mapped** = source geometry/tag; **historical/reported** = dated document or image association; **proposed** = our authored design; **unresolved** = insufficient for final placement. All simulation numbers below are proposed unless explicitly described as calculations from mapped geometry. See [sources](sector-04-sources.md).

## 2. Spatial layout and ownership

| Control/decision point | Longitude E | Latitude N | OSM node |
| --- | --- | --- | --- |
| Adeyemi / Oyetayo / Church, S03 interface | 3.3449118 | 6.5612076 | 1079849444 |
| Adeyemi / Owoseni | 3.3442519 | 6.5610890 | 1079849290 |
| Adeyemi / Arowojobe | 3.3420212 | 6.5611092 | 35785466 |
| Adeyemi / Daudu | 3.3402634 | 6.5610060 | 35785465 |
| Bridge east end | 3.3388922 | 6.5611575 | 1073316752 |
| Bridge west end | 3.3387829 | 6.5611750 | 5624738944 |
| Adeyemi / Ogunoloko / Raji Abayomi | 3.3385073 | 6.5611962 | 1073319565 |
| Owoseni / Orona | 3.3442882 | 6.5604361 | 1079849327 |
| Owoseni / Mabayoje | 3.3443295 | 6.5597473 | 1079849216 |
| Orona / Adekunle | 3.3408958 | 6.5604307 | 1079849425 |
| Adekunle / Mabayoje | 3.3407911 | 6.5598073 | 1079849448 |

These coordinates are map records, not survey benchmarks. [H01]

The Adeyemi spine uses ways **8564504 → 589139290 → 589014132 → 589014131**, approximately **710.4 m** in total along mapped polylines. The bridge way is **589014132**, approximately **12.2 m** along its two-node line. That length is not a certified clear span, deck length or width. Surface tags alternate asphalt/unpaved/asphalt/unpaved; retain the conflict with historical roadwork evidence rather than turn each database split into a physical surfacing seam. [H01, H02, H03]

The southern candidate loop is **Owoseni → Mabayoje → Adekunle → Orona → Owoseni**, approximately **917.4 m**, starting/ending at node 1079849327. This is graph connectivity only. It does not bypass the western ditch, and it must not be offered as a cross-ditch detour. Pedestrian continuity and vehicle eligibility remain unverified. [H01]

Twelve selected road ways produce fourteen shared-node pairs at ten unique nodes. Three pairs are same-road joins; eleven are cross-street pairs. The entire Adeyemi way 8564504 overlaps Sector 03's evidence package. A future importer must retain one runtime road owner and compare node sequences/versions before linking sectors.

### Bridge/ditch topology

Ditch way **92451100** crosses the bridge line geometrically near **3.3388559 E, 6.5611633 N**. This is a derived 2D intersection between source segments, not a new surveyed point. The two ways share no node. The road carries `bridge=yes`, `layer=1`; it must remain above the drainage corridor rather than form an at-grade navigation junction. [H01]

The complete ditch line extends well beyond this study window. Its source node order is not a verified flow direction. Do not name the watercourse, infer a tidal connection, turn it into a canal boat route, or derive flood depth from its horizontal geometry.

### Building evidence sample

The companion building file retains **51 closed OSM footprints** around the western bridge. Selection uses an approximate 80 m radius around `[3.33884,6.56117]`, measured to each ring's arithmetic mean of non-closing vertices. This is a bounded research sample, not all buildings in the sector or a precise cadastral buffer. All selected ways have `building=yes`; none establishes height, floor count, use, tenancy or current condition. [H01]

Generation rule: `preserve ring → resolve whether footprint or roof outline → register current image → establish envelope/height → locate openings and access → classify use → place detail`. Do not scale a polygon to a convenient modular kit or infer shops from road frontage alone. Unnamed buildings stay unnamed. Do not distribute the two historical façade photos across fifty-one unrelated footprints.

## 3. Topography, collision and coordinate system

No surveyed deck level, soffit, bank crest, drain invert, channel width, kerb or threshold elevation is available. The bridge layer establishes relative topology, not metres. Regional terrain products cannot supply small drain crossings or deck clearances. [H01]

Required survey sections:

1. Adeyemi bridge: both approaches, deck profile, barriers, waterway section and lowest structural clearance.
2. Owoseni/Orona: pedestrian clear width, drainage coverage and threshold crossings.
3. Frontage study: two independently registered samples of the open-drain/crossing relationship visible historically in H02.
4. Downstream/upstream control sufficient to understand the ditch's direction and backwater behavior; neither is assumed from the map.

Collision groups are road/deck, barrier, pedestrian passage, private threshold, drain opening and channel boundary. Keep bridge supports and crossings as actual solid geometry when measured. A visually closed drain cannot retain an invisible open-hole collision, and a decorative cover cannot supply load-bearing access by appearance alone. Do not authorize street or water navigation beneath the bridge without evidence.

Research coordinates stay WGS84 `[longitude,latitude]`, no altitude. Proposed horizontal projection is EPSG:32631, shared origin latitude 6.5560 N / longitude 3.3510 E. Compute projected offsets using a reviewed CRS library; they have not been calculated here. Define research source metres `(NorthDelta,UpDelta,EastDelta)` and retain the documented canonical adapter `UE(X,Y,Z)=100*source(Z,X,Y)`. Research UE becomes `(EastDelta,NorthDelta,UpDelta)` centimetres. Vertical datum and registration into the existing canonical world remain null. No existing scene is moved.

## 4. Traffic and go-slow rules

**Proposed design:** preserve the long east–west trip along Adeyemi while local tasks enter and leave through Owoseni, Arowojobe and Daudu. A destination or loading reservation precedes a vehicle spawn. Capacity reduces only when a real occupied envelope, downstream queue or verified restriction requires it—not simply because a player enters Oshodi.

- Road links store source ID, direction, measured width, turning envelope, access-policy date, stopping permissions and downstream capacity. Unknown values do not default to permissive final traffic.
- Treat the bridge as one structural crossing with separate travel lanes/pedestrian space only after widths are known. A short mapped span does not imply a single-lane bridge.
- Danfo use model-specific dimensions, door/boarding sockets and verified route/stop evidence. Yellow minibus references from earlier sectors do not prove a service on every Adeyemi side street or a particular Volkswagen subtype.
- Keke/okada spawn permissions remain withheld pending current legal and local-route evidence. Do not assume green livery or give motorcycles a path through an uncovered drain.
- Car/van loading reserves frontage access. A full bay causes holding/diversion before entry, not automatic parking across a small crossing.
- Hawker approaches require a stopped vehicle below 1 m/s for 6 s, a reachable customer slot and a retreat route clear of drain openings. Goods, finite stock and transaction completion persist. Gala, chips and water need site/packaging references before placement; no merchandise is inferred from an unreadable photograph.
- Candidate traffic scenarios use demand at 0.3, 0.6 and 0.85 of later-established link capacity. Queue growth is measured against downstream space. These ratios are tuning inputs, not observed traffic counts.

The bridge event must preserve emergency passage and cannot create a false detour over the ditch. Vehicles unable to proceed wait or reschedule at an approved upstream holding point. Close an event before exceeding its permitted queue extent; do not turn the whole map into indefinite gridlock.

## 5. Architectural and material blueprint

Two photographs published with the August 12, 2021 Adeyemi report show open concrete drainage next to shop-sized frontages, small crossing slabs/thresholds, awnings, upper-floor windows and balconies, overhead wiring, a road roller and ground-edge damage. Exact camera positions, image capture dates and building identities remain unresolved. The report concerns a street inspection, not a current survey. [H02]

The visual priority is distinct, usable interfaces: a recessed door behind a crossing, a counter near but not occupying a passage, a shaded awning edge, an upper wall with individual openings, and a repair patch related to its substrate. These are proposed families requiring registration before placement, not proof that every selected footprint contains these features.

| Family | Technical rule | Evidence/approval gate |
| --- | --- | --- |
| Open frontage drain | Model wall thickness, void, cover edge and threshold separately; independent wet/dry stain masks | Historical form H02; current sections/locations pending |
| Bridge/deck | Preserve road silhouette and separation from ditch; no inferred supports or width | Horizontal crossing H01; engineering dimensions pending |
| Rendered/block wall | Patch boundaries tied to repairs; separate coating loss, roughness and runoff | H02 family reference; individual façades pending |
| Shop awning/shutter | Thickness and attachment; maintain entrance clearance | H02 appearance; identity/coordinates/rights pending |
| Roof and balcony | Roof envelope, fascia, railing and support as separate shapes | No random height or colonial/brutalist classification |
| Road surface | Asphalt/patch/paving candidates with independent material scale and condition | OSM tag variation is unresolved, not literal material zoning |
| Water/sediment | Depth-linked water appearance, bank contact, flow vectors and exposed sediment | No water level or contamination type established |
| Cables/generator traces | Attachment-based cables; soot only from an evidenced exhaust | Overhead wires visible historically; actual routes/generators pending |

No luxury estate, Island skyline, colonial building or modern glass tower is transplanted here. Observed repair and everyday occupation matter more than covering every object in rust. Political posters, brand decals and shop names require exact date/location/artwork clearance; a logo in a reference image is not automatically a reusable asset.

Proposed LOD distances for review: 15 m for drain/threshold detail, 50 m for simplified frontage, 150 m for consolidated exterior massing. Adjust by screen size and silhouette. Collision, navigation, water barriers and public/private boundaries never change with visual LOD. No frame-rate acceptance is claimed.

## 6. Ambient audio and social routines

**Proposed audio:** maintain an east–west traffic bed composed of real emitters. At the bridge, use separate positions for road traffic above and any evidenced water sound below. A drain has no audible flow when simulation storage/velocity says it is dry. Historical photographs do not establish a soundscape.

Horns derive Doppler from relative motion; a stopped bus uses no artificial passing pitch. Shop voices and stock handling are localized, with corners, walls and awnings shaping audibility. Generators start only at verified equipment locations and follow their own power/load state. Commission or license any Afrobeats/Fuji sources; do not label a looping music bed a field recording. Low-frequency music transmission and speech clarity need actual listening review.

NPC roles: commuters, drivers, traders, household errand-runners, porters and maintenance staff. Routines include work, rest, waiting, social ties and refusals. INEC's public directory associates its Oshodi-Isolo office with Adeyemi Street, but supplies no verified building placement in this package. Do not invent an office interior, polling station, schedule or political NPC faction. [H04]

Priority tree: `immediate safety → passable access → committed journey/work → consented interaction → optional activity/rest`. A trader negotiates based on stock, urgency and familiarity, not gender. A porter has a load envelope and destination, not an endlessly repeating animation. Park-dues roles and tech-worker clusters remain absent without local institutional evidence.

Language reference H05 supports the basic meanings of **Abeg** (please), **No wahala** (contextually agreement/no problem), and **How far?** as an informal greeting variant. Proposed usage is respectively a request for passage, acknowledgement, and greeting between familiar characters. These placements need fluent review; lexical evidence does not approve delivery, accent, tone, every speaker or a complete script. Avoid treating any phrase as compulsory speech for all residents.

Initial interaction distances: 1.5–3 m for trading, 2–4 m for access negotiation. Personal-space/carrying envelopes depend on actual body/load dimensions. Long-distance destination calls require researched routes and recording consent, not random famous destinations.

## 7. Weather and lighting

**Proposed weather responses, not calibrated local measurements.** Historical reporting identifies drainage problems at named points along Adeyemi, but those points have not been geolocated to the bridge or loop. Do not place reported damage at the nearest mapped ditch simply because both concern drainage. [H02]

Separate regional aerosol haze, humidity, local raised dust, surface wetness and standing water. A Harmattan candidate uses distance-dependent contrast loss and warmer direct light under aerosol scattering; no permanent orange colour grade. Rain uses exposed roof impacts, sheltered edges, gutter/cover discharge and tyre spray tied to water and speed. Humidity is not white ground fog.

The ditch simulation is a connected flow graph only after invert levels, section geometry and boundary conditions are established. For each basin use storage change = rainfall + inflow − infiltration − discharge − outflow. Bridge geometry does not itself prescribe hydraulic capacity. A backed-up ditch can only alter a road surface when the modeled level reaches its measured threshold; instant global flooding is forbidden.

Water meshes follow local basin/channel geometry, not one horizontal plane through homes and decks. Generate foam/turbulence only where flow warrants it; no automatic oil rainbow or sewage colour. Navigation uses reviewed depth/flow/visibility classes. Disabled or uncalibrated flooding must not quietly modify the verified collision layer.

## 8. Developer Asset Checklist

No asset below has been constructed by this package. Map evidence, reference readiness, placement approval and native delivery remain distinct.

### 3D meshes and architecture

- [ ] S04-R01 — four-part Adeyemi source chain; retain seamless database joins, measure actual material/width transitions.
- [ ] S04-R02 — Owoseni/Orona/Adekunle/Mabayoje route; verify crossings and public passage before admission.
- [ ] S04-B01 — bridge deck/edge/barrier kit; source alignment available, cross-section/supports pending.
- [ ] S04-D01 — ditch segment and bank/bridge interfaces; full mapped line retained, bed/flow/width pending.
- [ ] S04-F01 — 51 source building rings; each needs envelope, height, use and current-image registration.
- [ ] S04-F02 — drain-crossing threshold, shutter, awning and balcony families from H02; no cloned photo frontage.
- [ ] S04-P01 — cart, load, hand tools and temporary markers for the fictional event; geometry/rights/event staging pending.
- [ ] S04-V01 — vehicle-specific Danfo, car/van and optional keke/okada kits; sector route/policy admission pending.

### Material shaders and textures

- [ ] S04-M01 — independent render coating, exposed substrate, repair and runoff masks.
- [ ] S04-M02 — road candidates with physical texture scale; do not encode unverified source seams as reality.
- [ ] S04-M03 — concrete drain/cover, bank sediment and threshold wear with meaningful relief/collision separation.
- [ ] S04-M04 — rainfall exposure, water-depth, flow, wetness and drying inputs.
- [ ] S04-M05 — LOD/impostor materials preserving bridge and access silhouettes.
- [ ] S04-X01 — sign/brand/poster/photography rights ledger; no final branded textures in this pass.

### Dynamic systems and spawners

- [ ] S04-G01 — bridge/ditch separation and single-owner source features across S03/S04.
- [ ] S04-T01 — capacity-aware traffic, measured swept volumes and pre-reserved loading places.
- [ ] S04-N01 — finite work/travel/inventory states, local script review and context-sensitive interaction.
- [ ] S04-A01 — road-above/water-below sound positioning; licensed music and verified equipment state.
- [ ] S04-W01 — hydrologic boundaries, measured thresholds and reviewed navigation closures.
- [ ] S04-E01 — bounded handcart incident with reservation, abort and streaming persistence.
- [ ] S04-Q01 — separate source integrity, local visual, native integration and gameplay acceptance records.

## 9. Micro-narrative S04-E01 — a stalled handcart at the approach

**Fictional event; not a claim about a real accident or business.** Candidate anchor: eastern bridge approach node 1073316752. The final staging polygon must be on a verified public surface outside the ditch and preserve measured emergency/pedestrian clearance. If no such area exists, this event is not placed here.

A loaded handcart develops a wheel problem before entering the crossing. Its porter stops in the reserved margin and asks a colleague to help transfer a small part of the load to a second cart. Pedestrians negotiate passage; an eligible vehicle slows for the occupied envelope. The player can wait, help with a consented lightweight parcel, or turn back before committing to the crossing. No reward depends on hazardous lifting or stepping into the drain.

State sequence: `reserve margin and helper → cart fault → protect passage → transfer bounded load → move clear → release reservations`. Proposed disruption lasts 20–50 s, with a 75 s upper limit followed by a safe cancel/clear path. A failed reservation, unavailable helper, excess queue, emergency demand, poor visibility or unsafe water state prevents activation or cancels it.

The southern loop is not falsely advertised as a bridge bypass. An alternate western route can be offered only after that complete route is independently mapped and admitted; otherwise the choices remain a brief wait or rescheduled journey. Physical load, stock counts, NPC IDs, queue membership and any authorized compensation persist across streaming. Resetting the camera cannot reproduce goods or rewards.

Event acceptance requires an observed playable passage, safe cancellation, load conservation and agreement between visuals/collision/navigation. An event description or running timer alone does not pass it.

## 10. Next adjacent sector and open gates

Continue west/southwest along Ogunoloko Road from node 1073319565, using way 496763689 as the next discovery lead. Preserve its alternate source name separately; research the next route afresh before using it as a detour.

Remaining gates: registered contemporary imagery, parcel/road dimensions, vertical datum, drain boundaries, dated vehicle policy, local consultation/recordings, asset construction and authorized renderer/native integration. This pass establishes research data and design, not a complete 1:1 or playable sector.
