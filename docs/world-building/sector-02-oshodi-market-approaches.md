# Sector 02 — Brown Street–Oshodi Road market approaches

October 11, 2026. Detailed conditional blueprint; no native level has been generated.

## 1. Direction and evidence boundary

We extend the first sector westward into the mapped street network bounded by Oshodi Road, MacCarthy Lane, Brown Street and Igbehinadun Street. Banjoko and Araromi supply cross-routes, while Oredeyin creates an additional return to Oshodi Road. The geometry comes from S01 in [the source register](sector-02-sources.md), not a randomly generated market grid.

The design contrast is an interchange approach resolving into small-scale frontage decisions: choosing a passage, waiting for a handcart, negotiating a purchase, finding shelter and returning to transport. These are proposed activities, not claims that particular current shops perform them.

Evidence labels used here:

- **Mapped:** OSM geometry/tag or shared-node connection; not a field survey.
- **Historical:** dated photograph or project record; not current placement authority.
- **Proposed:** authored simulation, art or performance choice.
- **Unresolved:** cannot enter the verified placement layer yet.

Starting boundary: longitude 3.3448–3.3510 E, latitude 6.5560–6.5609 N. It is a research rectangle, not an administrative boundary or enclosing wall. Overlap with Sector 01 is intentional; its street-level treatment is refined here. The map must retain shared geometry rather than create overlapping copies.

## 2. Exact mapped layout

The main candidate loop is approximately 1,025 m along the mapped polylines:

**Oshodi Road → MacCarthy Lane → Brown Street → Oredeyin Street → Oshodi Road.**

This proves graph connectivity, not that the entire loop has accessible pavements, safe crossings or legal vehicle access.

| Control point | Latitude N | Longitude E | OSM node |
| --- | --- | --- | --- |
| Oshodi Road / MacCarthy Lane, eastern interface | 6.5564935 | 3.3497770 | 1079849258 |
| MacCarthy / Araromi | 6.5573958 | 3.3497846 | 1079849303 |
| MacCarthy / Banjoko | 6.5579721 | 3.3500444 | 4395533076 |
| MacCarthy / Brown | 6.5585907 | 3.3502665 | 1079849239 |
| Brown / Oredeyin | 6.5593025 | 3.3482359 | 1079849221 |
| Brown / Igbehinadun | 6.5595975 | 3.3475022 | 1079849289 |
| Oredeyin / Oshodi Road | 6.5565058 | 3.3477201 | 1079849462 |
| Igbehinadun / Oshodi Road | 6.5564911 | 3.3470173 | 35785463 |

Retain the oblique Brown Street alignment and non-orthogonal cross-streets. Banjoko connects Igbehinadun, Oredeyin and MacCarthy through shared nodes; Araromi supplies a separate connection through the same three named streets. Their crossings are recorded individually in the junction file.

The fifteen shared-node pairs comprise thirteen cross-street connections and two same-road segment joins. They are not fifteen independently surveyed junctions.

Oshodi Road is split into multiple source ways; do not create a seam or duplicate intersection at node 4395533064. Brown Street continues west across way boundary node 1079849293; that database split must not become a road end.

### Proposed spatial beats

1. **Eastern arrival:** cross the Sector 01 interface at node 1079849258. Carry existing pedestrians, queued vehicles and audio state across it without respawning.
2. **First route choice:** Araromi affords a cross-route before the longer loop. The player chooses based on visible conditions and destination.
3. **Intermediate activity:** Banjoko connects the interior, enabling a delivery detour rather than forcing all traffic to Brown Street.
4. **Northern frontage:** Brown/Oredeyin provides a distinct decision point and the candidate event anchor.
5. **Return:** Oredeyin reconnects to Oshodi Road; Igbehinadun supplies an alternate larger circuit.
6. **Future expansion:** continue north along verified Igbehinadun nodes, retaining the real connecting corridor. Do not teleport to Apapa or Surulere.

No building, shopfront, food vendor or generator receives a final coordinate from this beat list.

## 3. Topography, surveying and coordinate contract

No reliable street-level elevation, kerb height, drainage invert, flood depth or absolute altitude is established. Road surface tags are recorded, but Brown Street's unpaved tag conflicts with the historical rehabilitation evidence in S02. The current surface remains unresolved.

The research GeoJSON uses two-element `[longitude, latitude]` coordinates. Missing height is absent, not zero. The full downloaded snapshot supplies source versions; geometry remains unsmoothed.

Proposed survey adapter, not a replacement for canonical game coordinates:

1. Project WGS84 coordinates to EPSG:32631 (UTM 31N).
2. Choose one origin shared with Sector 01: geographic 6.5560 N, 3.3510 E, with its projected Easting/Northing calculated by a CRS library.
3. Preserve vertical datum and units explicitly; withhold final Z until the dataset's datum is resolved.
4. For an isolated research frame, define source metres as `(NorthDelta, UpDelta, EastDelta)`.
5. Apply the existing documented adapter `UE = 100 * source(Z,X,Y)`, giving research UE `(EastDelta, NorthDelta, UpDelta) * 100`.
6. Establish a separate, reviewed registration from that research frame into the existing game. Its translation/rotation are currently null. Do not rotate or relocate existing canonical assets.

A local map origin is not an assertion that the canonical city's X or Z already means north or east. OSM coordinates must never be fed directly to a metre-based adapter.

### Collision and terrain requirements

- Preserve every surveyed road crossfall, threshold, drain crossing and public opening as semantic geometry.
- Use simple collision for road/deck/step volumes. Decorative wires, basket weave and paint chips do not need full triangle collision.
- Navigation areas distinguish road, pedestrian passage, threshold, private entrance and uncovered drain.
- Keep protected rail/expressway access outside any generated shortcut.
- Treat a 30 m surface model as regional context only (S06); no centimetre kerbs interpolated from it.
- Any preview with inferred widths/heights belongs in a visibly labelled approximation layer and cannot pass 1:1 acceptance.

## 4. Traffic and delivery dynamics

**Proposed simulation rules. No measured counts or actual operator timetable is claimed.**

Traffic pressure originates at the connected Oshodi Road approaches and propagates through eligible links. Loading, pedestrian crossing and turning capacity determine queues. A road does not become busy solely because the player enters its cell.

Each link stores vehicle-class eligibility, scenario date, available width, turning clearance, occupied loading space and downstream capacity. Unresolved legal/access fields remain closed to autonomous final-placement traffic; they are not implicitly permissive.

- **Danfo:** use on independently verified services, not every narrow street. Validate full swept turning paths. Boarding occurs at approved stops only; no invented terminal at Brown/Oredeyin.
- **Keke/okada:** keep modular controllers but withhold this sector's spawn routes until dated rules and local observations are reconciled. Colour and model require local vehicle references.
- **Cars/vans:** delivery goals reserve a legal loading location before departure. Failed reservations cause holding/diversion, not stopping across a junction by default.
- **Handcarts:** historical S02 imagery supports a reference candidate; current placements still need confirmation. Small carts use their own swept volumes, turning radius and load-dependent pace.
- **Hawkers:** use occupied, reachable customer positions, finite stock and a retreat path. No spawning between moving vehicles.

Initial design tuning: near-queue approaches require a vehicle below 1 m/s for 6 seconds and a clear retreat. These values are gameplay parameters, not observed Lagos norms. Horn impulses reflect individual traffic events; horn cooldowns prevent an endless identical loop.

Preserve unique vehicle and NPC IDs across cells. An unloading vehicle's reservation, remaining cargo and queue position survive streaming. A client-side visual event cannot mint inventory, money or authoritative traffic permissions.

## 5. Architectural grammar and materials

Historical S02 shows mixed close-set frontages, raised edges, repair activity and varied openings. Use it to reject an identical repeated shell, not to claim today's façade inventory. S03 supplies historical produce-display references, not a present-day market parcel.

Proposed grammar:

`verified footprint → evidenced envelope → structure → openings → access → frontage use → maintained/repair state → loose props`

Reject any generation that changes a footprint to fit a kit, chooses a floor count randomly, extends stock into a protected passage, or creates unsupported upper storeys. Variation belongs in approved finish/prop slots, with deterministic seeds keyed by a surveyed feature ID.

### Material families

| Family | Proposed implementation | Evidence condition |
| --- | --- | --- |
| Road/paving | Separate finish, joints, abrasion, patches, dust and wetness | Current surface unresolved; no default dirt road |
| Raised edges and thresholds | Contact dirt and runoff masks; geometry for actual level changes | Measure before collision approval |
| Painted render | Local repair boundaries and weather exposure | Match a geolocated façade; no universal peeling paint |
| Corrugated roofing | Separate sheet, ridge, fastener and patch variation | Verify roof type and exact occupied extent |
| Steel shutters/grilles | Wear at contact/locking surfaces | No invented branding or inaccessible opening |
| Timber counters/crates | Abraded working edges; grain at physical scale | Historical prop family, placement pending |
| Baskets/metal bowls | Readable silhouettes, inexpensive weave/normal detail | Historical S03 reference only |
| Drain water/sediment | Water depth and sediment separate from base surface | Drain geometry and flow direction unresolved |
| Utility staining | Exhaust/rain traces only downstream of actual emitters | Generator/cable location must be evidenced |

Dust, damage, coating loss, surface wetness and standing water are independent parameters. No whole-street glossy coat, permanent smoke overlay, or red-laterite substitution for an unverified surface.

Brand signs and campaign posters stay absent until exact artwork, date, location and reuse clearance are known. Do not fill blank signboards with invented businesses. British colonial buildings, brutalist monuments and modern towers are not inserted here to satisfy a citywide aesthetic checklist.

## 6. Audio and social routines

**Proposed behavior and mix; not a sound survey or claim about current residents.**

Create localized voices, footsteps, trolley wheels, stock handling and vehicle sources. Elevation, walls, corners and open storefronts affect audibility. A quiet conversation beside a counter must remain possible amid a busy street.

- Generator audio only exists at an approved generator asset with an actual power-state transition; stagger start attempts instead of switching every block simultaneously.
- Music comes from licensed/commissioned tracks assigned to verified suitable businesses. No generic citywide Afrobeats/Fuji soundtrack masquerading as a field recording.
- Destination calls require a researched route and current service; no automatic famous-place name generator.
- Bargaining is turn-taking with stock, price, familiarity and time constraints. Personality varies independently of gender.
- Pedestrians pause for a passage, assist a delivery, continue errands, shelter or rest. Busy does not mean constantly hostile.
- Traders and porters have finite inventories/work tasks. They do not reset after the player looks away.
- Park-dues roles are withheld without an exact, supported institution/location. Do not attach agbero collection to every junction or make it a gang mechanic by default.
- Do not seed a stereotyped “tech-bro faction” here merely because a separate Yaba sector was discussed.

Initial interaction distances: 1.5–3 m for a transaction; 2–4 m for a delivery negotiation; avoidance envelopes based on body/cart width. These are tunable design choices.

The user's requested “How far?”, “Abeg” and “No wahala” remain candidate script tokens. No newly composed Pidgin bark, accent, gendered voice assignment or destination call is approved until fluent local review establishes intent, pronunciation and context.

Behavior priority:

`immediate safety → access/shelter → current work obligation → interaction → incidental activity/rest`

## 7. Seasonal weather and light

**Proposed responses, with no calibrated storm/flood claim.** Do not import numerical climate or elevation values from generic Lagos studies as Brown Street measurements.

Dry/hazy condition: distance-dependent contrast loss, spatially consistent wind, dust retained in sheltered crevices, and local raised dust only from active sources. Humidity is not a blanket orange filter. Harmattan interpretation needs a dated local meteorological/reference set before final seasonal calibration.

Rain condition: roof impacts, awning drips, gutters and road spray use different effects. Wetness propagates with exposure; standing water occurs only where the resolved surface and drainage allow it. On clearing, exposed and covered areas dry at different rates.

Keep a local water balance: rainfall plus inflow minus discharge, infiltration and outflow. A material-only puddle cannot silently become a deep navigation obstacle. High-flow/depth areas close according to a reviewed traversal policy; a known dry detour remains available or the event does not activate.

Lighting follows geographic time and sun orientation in the research frame. Final shadow validation must account for the canonical registration. Reuse engine rendering facilities; this blueprint installs or enables no renderer feature.

## 8. Developer Asset Checklist

All rows are briefs, not completed assets. “Historical” never authorizes exact current placement.

### 3D meshes and architecture

| ID | Required asset/deliverable | Evidence and acceptance condition |
| --- | --- | --- |
| GEO-02-01 | Ten named road-way chains with source IDs | S01 retained; width/Z unresolved |
| GEO-02-02 | Thirteen cross-street connections and two segment joins | No duplicated split-way seams or invented turns |
| GEO-02-03 | Surveyed kerb, threshold and drain-crossing modules | Current cross-sections and accessible route evidence |
| ARC-02-01 | Per-building footprint/envelope records | No approved building catalogue exists yet |
| ARC-02-02 | Approved storefront/opening families | Geolocated frontage references; preserve door access |
| ARC-02-03 | Roofing, shutters, shade and grille modules | Per-site reference and overhead/egress clearances |
| PROP-02-01 | Counters, baskets, bowls and handling props | S03 historical design reference; current placement pending |
| PROP-02-02 | Small handcart, wheel pivots and loaded/unloaded states | S02 historical reference; dimensions/current use pending |
| PROP-02-03 | Tarpaulin, clips and support attachments | New evidence required; no unsupported cloth |
| VEH-02-01 | Eligible delivery van/car variants | Turning envelope, approved curb space and vehicle reference |
| INF-02-01 | Generator/fence/exhaust ensemble | Exact installation source; no inferred hidden generator |
| INF-02-02 | Supported pole/cable network | Verified connections; no random wire webs |

### Material shaders and textures

| ID | Required family | Acceptance condition |
| --- | --- | --- |
| MAT-02-01 | Dry/wet road and joint system | Resolve Brown Street surface conflict first |
| MAT-02-02 | Concrete repair/contact/runoff masks | Reference-controlled, no random structural damage |
| MAT-02-03 | Painted render and shutter finish | Correct scale and evidence-linked palette |
| MAT-02-04 | Corrugated sheet/fastener wear | Preserve material identity and roof direction |
| MAT-02-05 | Timber, basket and metal-container surfaces | Physical scale; low-cost distant representation |
| MAT-02-06 | Dust/film/puddle/sediment controls | Separate states; no duplicate wetness reflections |
| MAT-02-07 | Signage/decal registry | Exact wording/date/location plus rights clearance |

### Dynamic systems and spawners

| ID | Required system | Acceptance condition |
| --- | --- | --- |
| SYS-02-01 | Street interface and actor-state transfer | Single owner per shared node; no duplicated actors |
| SYS-02-02 | Delivery reservations and spillback | Eligible curb/turning path; no blocked emergency exit |
| SYS-02-03 | Trader/porter/commuter routines | Finite stock, rest and persistent intentions |
| SYS-02-04 | Safe near-queue trading interaction | Clearance/retreat checks; no clipping or compulsory collision |
| SYS-02-05 | Shelter selection and weather response | Capacity-aware; no mass teleporting |
| SYS-02-06 | Power-linked localized audio | One visible emitter/state, not global arbitrary loops |
| SYS-02-07 | Local runoff and traversal closure | Surveyed drainage, hysteresis and viable detour |
| SYS-02-08 | Micro-event owner and recovery | Persistent phases, reservations, cancellation and save/load |
| SYS-02-09 | Evidence-admission review | Mapped/history/design/unknown remain separate |

### Performance budgets — initial targets, not results

Use bounded native instancing only for genuinely repeated meshes. Keep façade semantic IDs when grouping. Near actors retain gameplay state; distant crowds may simplify presentation without settling economic events or changing authority.

Texture budgets, active agent caps and streaming distances remain profiling inputs. Collision must not change with visual LOD. Preserve basket/counter silhouette near the player but avoid fully modelled weave at distance. Neither the existing software-rendered editor nor this document establishes mobile or GPU performance.

## 9. Micro-narrative: the covered delivery at Brown/Oredeyin

Fictional event `S02-E01`, anchored for design at mapped junction node 1079849221 (6.5593025 N, 3.3482359 E). No real named trader, business or road defect is implicated.

### Preconditions

A current frontage/loading position and sheltered destination have been evidenced; a clear passage exists; the connected Banjoko detour is verified for the affected actor class. Without these, the event remains a brief, not a placed spawner.

### Sequence

1. A porter approaches a receiving frontage with a finite handcart delivery.
2. Rain begins while the receiving space is briefly occupied by another transaction.
3. The receiver clears a covered patch; the porter waits in a permitted recess, leaving a passage.
4. One unloading movement temporarily narrows an eligible local route. Pedestrians negotiate the gap; queued vehicles hold or select a valid alternative.
5. The player can wait, take the Banjoko/MacCarthy alternative, or assist through an authorized interaction.
6. The delivery completes; reservations release and the queue clears progressively. Stock remains with its legitimate owner.
7. If shelter, clearance or the detour is lost, cancel safely and relocate only to a known reachable position. Never teleport actors into an exit.

Possible outcome is inconvenience and cooperation, not automatic conflict. No rain-induced “flash flood” is asserted without drainage evidence. Umbrella vendors are not spawned merely to fill a dramatic trope.

Persist event ID, phase, participants, inventory ownership, reservation IDs, weather trigger and cancellation reason. No economic reward or stock transfer becomes authoritative through the scenery layer.

## 10. Acceptance and next boundary

| Stage | Current state | Required proof |
| --- | --- | --- |
| Geographic topology | Ten ways/fifteen pairs recorded | Source versions retained; current road-access review still needed |
| Historical references | Two images visually inspected | Dates/context retained; contemporary frontage coverage missing |
| Topography | Unresolved | Vertical datum, spot levels, drains and thresholds |
| Cultural/operational detail | Conditional design | Local voice, business, trading and transport verification |
| Asset production | Not performed | Named meshes/materials and attributable evidence |
| Native integration | Not performed | Correct canonical registration, saved level/content ownership |
| Visual acceptance | Not performed | Matched geolocated views at player height |
| Gameplay acceptance | Not performed | Traversal, interaction, streaming, failure recovery and persistence |
| Performance/release | Not performed | Actual target profiling and separately authorized packaging |

Next adjacent research lead: Igbehinadun north of node 4395533093 at 6.5608319 N, 3.3477254 E. Freshly resolve the onward way and nearby land uses rather than naming a new neighbourhood from memory. Retain Sector 01 reconciliation and citywide scope; this blueprint does not finish either the world or the game.
