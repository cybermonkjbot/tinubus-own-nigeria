# Island and coast: two Lagos neighborhood slices

Research and authored-world dossier. Access date for every external source below: **October 11, 2026**. Documentation only; coordinator review pending.

## Scope and evidence rules

We select two compact references, not whole districts:

1. **Victoria Island: Adeola Odeku frontage / Eletu Ogabi Street / Amichi Close**, with Saka Tinubu Street as the southern public-street continuation.
2. **Lekki Phase 1: Fola Osibo Street between Theophilus Oji Street and Kafayat Abdulrasaq Street**, with Osaro Isokpan Street joining those side streets and Grace Anjos Drive as an eastern continuation reference.

The second is a street within the coastal-peninsula assignment, not a beach or a claim that the Atlantic touches this block. We do not make Lekki an actual detached island, attach an invented beach access, or substitute a resort for ordinary neighborhood life.

Evidence notation throughout:

- **F - source-confirmed:** the cited source explicitly reports the statement. An OSM tag confirms what the database records, not that it has been surveyed or remains correct on the ground.
- **M - map-supported inference:** derived from inspected node memberships or geometry; not an observation of current access, occupation, materials or culture.
- **P - authored proposal:** fictional places, people, routines, dimensions and simulation choices. These are design values, not surveyed Lagos conditions.
- **G - gap:** evidence absent, contradictory, inaccessible or too old to support the proposed claim.

All distances, clearances, room sizes, heights and durations in proposal sections are **P design values**. No real-world road width, elevation, architectural date, parcel tenancy or income distribution is asserted. Real business names appear only as research anchors; proposed playable businesses are unnamed and fictional in production metadata.

### Repository and composition boundary

Read: root `AGENTS.md`, `CONTEXT.md`, `docs/world-building/README.md`, coordinator-owned `docs/world-building/lagos-mosaic-2026-10-11/README.md`, and relevant preservation/coordinate passages of `output/cribtest11-visible-world-2026-10-11/parent-geography/lagos-geographic-authoring-v1.json`.

The earlier sectors remain their existing documentation; canonical district IDs, theatre, buildings, maps and destinations remain untouched. This document neither moves nor relabels current content. The parent reference's estimated envelopes and optional fictional slots are not approved placements. We do not assume the earlier 6 x 4.5 km envelope.

Source coordinates below are **WGS84 geographic `(longitude, latitude)` in degrees**, without altitude. OSM XML supplies separate `lat` and `lon` attributes; we deliberately reorder them in the ledger. OSM map API bbox order is `(west longitude, south latitude, east longitude, north latitude)`. Node decimals are database precision, not measured accuracy. Metre-space convention remains `x east, y up, z south`; `UE(X,Y,Z) = 100 * source(z,x,y)`, applied once after coordinator registration. No source degree pair is a metre position. Height datum, source-to-world transform and final engine positions are withheld.

## Source register

Exact URLs are retained for reproducibility. Web search was used for discovery; substantive street topology was freshly retrieved through the OSM API, and local primary pages were read for the claims below. No photographs, video frames, satellite images or private interiors were visually inspected in this pass.

| ID | Exact source URL | Source date / version | Supported use and limits |
| --- | --- | --- | --- |
| V1 | https://api.openstreetmap.org/api/0.6/map?bbox=3.421,6.427,3.428,6.433 | Live response accessed October 11, 2026; individual versions below | Adeola Odeku, Eletu Ogabi, Saka Tinubu, Amichi and mapped commercial anchors. API includes whole intersecting ways and their nodes beyond bbox; not a clipped land mask. |
| V2 | https://api.openstreetmap.org/api/0.6/map?bbox=3.419,6.427,3.423,6.430 | Live response, October 11, 2026 | Supplemental western frontage/Amichi context. Completes nodes outside V1's western boundary. |
| V3 | https://api.openstreetmap.org/api/0.6/way/107554130/full | Eletu Ogabi way v10, May 8, 2023, 05:35:13 UTC, as recorded in V1 | Exact member-node topology; replay current endpoint may return a later edit. |
| V4 | https://api.openstreetmap.org/api/0.6/way/146604674/full | Amichi Close way v7, March 6, 2025, 04:08:34 UTC | Branch geometry and endpoint membership; no public-access survey. |
| V5 | https://uniondiagnostic.com.ng/contact-us/ | Undated contact page, read October 11, 2026 | Union Diagnostic lists 5 Eletu Ogabi Street, off Adeola Odeku, Victoria Island. Supports a diagnostic-service address, not building form, parcel coordinates or access to treatment rooms. |
| V6 | https://www.sparnigeria.com/ | Direct retrieval returned HTTP 403 on October 11, 2026 | Not substantive primary confirmation. SPAR at 47 Adeola Odeku is only an OSM retail anchor in this dossier. |
| L1 | https://api.openstreetmap.org/api/0.6/map?bbox=3.465,6.440,3.477,6.449 | Live response accessed October 11, 2026 | Fola Osibo and connected side streets, bridge tag and nearby channel. Full-way returns may include remote features; only selected local features are used. |
| L2 | https://api.openstreetmap.org/api/0.6/way/138040470/full | Fola Osibo way v10, February 5, 2018, 20:21:03 UTC, as recorded in L1 | Main-street node sequence and `surface=paving_stones`; old tag, not current texture evidence. |
| L3 | https://api.openstreetmap.org/api/0.6/way/146603442/full | Fola Osibo Water Channel way v6, June 19, 2018, 19:34:51 UTC | Explicit `waterway=drain`; long diagonal alignment, not a drain running along every Fola frontage. |
| L4 | https://medplusnig.com/sellers/46 | Undated current store page, read October 11, 2026 | Medplus lists 12 Fola Osibo Road, Lekki Phase 1. Published hours: Mon-Fri 08:00-21:00, Sat 09:00-21:00, Sun 10:00-20:00. Confirms pharmacy frontage activity and scheduled evening opening, not its precise position within the selected two junctions. |
| L5 | https://www.ntda.lg.gov.ng/newsdetails.php?hash=6416338d51fcb327457a3bb52a54db30&nid=53 | NTDA article, June 26, 2020, 16:07 as displayed; timezone unstated | Primary historical report of rehabilitation commencing at Hakeem Dickson and Fola Osibo junctions, failed road portions, traffic and drainage/flooding concerns. Not proof of completion, today's condition or the same defect in this western block. |
| L6 | https://www.315serviceslimited.com/ | Site reachable October 11, 2026, but read response did not expose its address | OSM node 11998537311 independently tags a travel agency at 2A Olarenwaju Ninalowo Crescent. Website reachability does not confirm that address or present operation. Use only as a mapped nearby service lead. |
| X1 | https://www.openstreetmap.org/copyright | License page read October 11, 2026 | OSM data: ODbL; credit OpenStreetMap contributors and retain license notices. Data derivatives and produced works need the applicable ODbL treatment, not an assumption that a game asset erases provenance. |

**Licensing and imagery:** OSM data licensing does not license Bing imagery referenced by some source tags. `source=bing` is a mapping provenance clue, not a capture date or texture-reuse grant. No acquisition dates for underlying imagery were obtained. Primary business and NTDA pages had no verified open photo/audio/brand license in this review; use their facts as references, not their logos, screenshots, photos or text as assets. OSM's database grant likewise does not authorize private-interior reproduction or identifiable people. No asset purchase or download pack was made.

**Retrieval gaps:** initial Overpass requests failed (overpass-api.de HTTP 406; overpass.kumi.systems HTTP 500); direct OSM map/way API requests succeeded. No raw extract was saved outside this exclusive Markdown file. The feature/version ledger preserves the used selections, not a complete archival XML snapshot. Current full endpoints are mutable; historical object review can use OSM's `/api/0.6/way/ID/VERSION` and corresponding versioned nodes. Some discovery sites were inaccessible or returned only application shells; none is treated as confirmed evidence merely because it appeared in search.

## Slice 1: Victoria Island service street behind the frontage

### Anchors and exact local topology

**F [V1-V5]:** OSM names Adeola Odeku Street, Eletu Ogabi, Saka Tinubu Street and Amichi Close. Union Diagnostic's own contact page explicitly places its Eletu Ogabi address off Adeola Odeku. This is a stronger activity anchor than a general description of Victoria Island as an affluent district.

| Feature | OSM ID and version | Edit date UTC | Relevant tag or node relationship |
| --- | --- | --- | --- |
| Adeola Odeku carriageway | way 5150268 v35 | August 10, 2026, 09:30:00 | `secondary`, `lanes=2`, `oneway=yes`, `surface=paving_stones`; contains node 1235946207. |
| Other Adeola Odeku carriageway | way 134684101 v29 | August 10, 2026, 09:30:00 | Same named/tagged road; contains node 2339926116. Do not collapse two mapped carriageways into one surveyed width. |
| Eletu Ogabi | way 107554130 v10 | May 8, 2023, 05:35:13 | Nodes in order: 1235946207, 2339926116, 10877125287, 1598718011, 1235946208. No width/surface/one-way claim inferred from missing tags. |
| Amichi Close | way 146604674 v7 | March 6, 2025, 04:08:34 | Ends at shared node 1598718011; western arm then bends north. No second external street connection recorded in the inspected way. |
| Saka Tinubu | way 5196508 v12 | March 20, 2026, 11:23:08 | `tertiary`; shares node 1235946208 with Eletu Ogabi. |
| Retail research lead | way 594763846 v5 | May 28, 2024, 11:28:08 | `shop=supermarket`, SPAR, address 47 Adeola Odeku. No entry node, footprint height or current operation verified. |

| Junction anchor | `(longitude, latitude)` | Node version / edit date UTC |
| --- | --- | --- |
| Eletu / Adeola northern carriageway, node 1235946207 | `(3.4213475, 6.4295050)` | v5, June 7, 2018, 18:22:41 |
| Eletu / other Adeola carriageway, node 2339926116 | `(3.4213232, 6.4294311)` | v3, May 4, 2018, 12:05:51 |
| Eletu / Amichi, node 1598718011 | `(3.4213223, 6.4278125)` | v5, June 7, 2018, 18:22:41 |
| Eletu / Saka Tinubu, node 1235946208 | `(3.4213218, 6.4270402)` | v5, March 20, 2026, 11:17:38 |

**M:** shared membership supports an Adeola-to-Saka street connection through Eletu and a westward Amichi branch. It does not prove a legal crossing of both Adeola carriageways, an accessible sidewalk, an open compound gate or a walkable Amichi outlet. Treat Amichi as a destination/return branch, not a through-route. For walking-height navigation, the long side street and its turn provide more useful identity than a skyline silhouette.

**G:** no contemporary photographed frontage sequence, occupancy survey, actual socioeconomic profile, measured drains, tree inventory, utility positions, frontage boundaries or interior plans. OSM's `residential` highway tag is not evidence that each parcel is residential. The diagnostic address is not permission to reconstruct patient areas.

### Authored 260 m walk and street section

**P throughout this subsection.** The sequence follows the reference's turn-and-branch logic without claiming exact reconstruction. Distances are walked path design values, not distances measured between OSM nodes. Preserve full-sized lots and ordinary intervening frontage; if later reference registration does not fit, adjust the route or slice boundary rather than shrink buildings.

| Cumulative walking distance | Authored encounter and navigation |
| --- | --- |
| 0-30 m | Approach the Eletu mouth along Adeola frontage. An unnamed grocery entrance, a waiting bench in its setback and a delivery bay sit beside a closed office frontage. Keep a continuous pedestrian strip past the bay. The crossing between carriageways is a separately authored controlled crossing, not a claimed real crossing. |
| 30-95 m | Turn into Eletu: a small diagnostic reception on one side, a copy/print counter and an appliance-maintenance room on the other. A courier hands over a sealed parcel; an office worker waits for collection. Waiting and delivery stay inside setbacks. |
| 95-160 m | Quieter interval: gated apartment compound, shared stair landing, a caretaker's chair and a small planted yard. Another frontage has a modest household flat above an unnamed service office. This fictional tenure mix is not an identified real building or resident income classification. |
| 160-215 m | A side-yard laundry collection counter and an air-conditioning repair yard lead toward the Amichi turn. Use one visible corner, a wall-color change and an open workshop shutter for orientation. The Saka continuation stays visible as a public exit beyond this walk. |
| 215-260 m | Turn into the first Amichi arm to a fictional invited courtyard: covered washing area, bicycle storage and two sitting groups separated from a workbench. Public pavement ends at the gate; entry is optional and permission-based. Retrace to Eletu to continue publicly. |

**P dimensions:** main side-street carriageway 7.0 m; clear pedestrian strip 2.0 m per side where designed, with 2.4 m waiting pockets; Adeola carriageway proposal 6.5 m each, with a 2.5 m refuge only if a controlled crossing is authored; these do not certify OSM lane widths. Amichi local carriageway 5.5 m and one continuous 1.8 m step-free strip. Frontages 8-18 m; shallow service rooms 4 x 6 m; reception 6 x 8 m; courtyard 10 x 12 m; delivery bay 3 x 8 m; pedestrian headroom 2.4 m. Proposed buildings are one to four storeys, not source heights or architectural dates. No permanent obstacle narrows the clear strip below 1.8 m.

### Ordinary destinations, yards and livelihoods

**P:** make the diagnostic reception usable for a fictional appointment/check-in, with a private staff door rather than explorable treatment rooms. Add a print counter for collection, repair workshop for a booked small-appliance job, laundry collection shelf, grocery pickup and an invitation-only household visit. Ordinary kitchens include a sink, refrigerator, preparation shelf, reusable water containers and dining chairs; choose final appliance and food details after local reference review. Residential rooms contain storage, study/work tables and everyday laundry, not only luxury furniture or deprivation props.

**P:** morning workers open shutters while residents leave or remain home; midday deliveries share the setback with visitors; evening household cooking and sitting overlap with a late collection. Include residents, service proprietors, technicians, reception staff, visitors, cleaners and delivery workers of varied genders and ages. Staff and residents both have social agency and leisure. Do not use clothing, occupation, skin tone or vehicle ownership to assign wealth or criminality. The mix of modest rooms, shared facilities, fitted offices and more spacious compounds is our fictional socioeconomic composition, not a survey result.

### Developer Asset Checklist

Everything below is **P**, to be authored later; no assets are created by this dossier.

| Category | Required assets and behavior |
| --- | --- |
| Meshes / architecture | Modular recessed storefronts; plain office and apartment shells; gate with independent pedestrian leaf; compound wall and shaded stair; reception with privacy partition; print and repair counters; service-yard workbench; laundry shelves; benches; handcart; AC outdoor-unit enclosure; screened utility yard; tree planters; covered drain crossing modules. No copied real facade or business logo. |
| Materials / textures / shaders | Original asphalt/paver alternatives, painted render in maintained and repaired variants, concrete patch edges, powder-coated and repainted metal, glazing with privacy/frost options, woven shade and plain textile variants. `paving_stones` is map-tag evidence only for Adeola, not a specification for Eletu. Wetness, tire marks and repair seams are authored material states; avoid universal grime. |
| Systems / spawners | Pedestrian destinations separate from work queues; appointment waiting with privacy; loading-bay reservation; timed shutter/gate state; owner permission for courtyard; maintenance technician task; accessible-route occupancy exclusion; vehicle direction taken from supported road rules only after integration review; density variation that never fills every setback with crowds. |

### Audio, light and weather

**P:** place traffic beds along the main frontage, then attenuate them behind walls and the Eletu turn. Localize printer feed, tools, trolley wheels, latch clicks and low conversation to their rooms; a screened generator or AC hum is optional authored utility sound, not a confirmed equipment inventory. Keep household audio inside its room boundary. No copyrighted music or unreviewed accent imitation. Distinguish directions with visible motion as well as sound.

**P:** morning angled light at gates; midday shade under small canopies; evening warm residential windows beside cooler office lighting; dark closed rooms remain dark without making all side streets unsafe. Rain gathers in designed gutters and shallow service-yard catch points; covered flush crossings keep the walking route continuous. Create drier/hazier and wetter seasonal presets only as simulation alternatives, without asserting rainfall totals, dates, tide levels or neighborhood flood depths. Real slope, drain direction/outfall and surface elevation are **G**; do not flatten the reference to a claimed sea-level plane.

### Fictional disruption: repair delivery at the Eletu mouth

**P:** a replacement refrigeration cabinet shifts on a handcart at a fictional service frontage just inside the Eletu mouth. The cabinet and a parked delivery vehicle temporarily obstruct one frontage route. This is not a reported incident at Union Diagnostic or SPAR.

- Exit A: continue along the authored Adeola clear strip to the composite outgoing port; no need to cross live traffic.
- Exit B: use the protected opposite Eletu strip via an authored low-speed crossing, then continue south to Saka Tinubu. Reserve a 1.8 m clear route with hard exclusion from props and queues.
- Exit C: retreat to the previous Adeola approach. Amichi provides an optional waiting destination but is never mislabeled as a through-exit.

**P clearing condition:** workers stabilize and remove the cabinet, move the handcart into the delivery bay and release the reserved vehicle space. A 90-180 second design window varies with actual NPC completion; do not impose a ceremonial wait. No lifting action is mandatory for the player, no household entry is required, and the event cannot close the southern Eletu connector. Persistent failure leaves the protected route available and a retryable worker task, not a district-wide lock.

### Incoming / outgoing connector briefs

| Connector | Evidence relationship and omitted geography | Authored handoff |
| --- | --- | --- |
| VI incoming local feeder | Source side: Saka Tinubu way 5196508. Destination: Eletu way 107554130 at shared node 1235946208. **M real local connection** from V1; nothing omitted at this junction. Approach farther along Saka is outside this authored walk, not empty land. | **P:** pedestrians and locally permitted vehicles; continuous 2.0 m walking strip, proposed 7.0 m carriageway, 4.5 m vehicle headroom if a canopy is ever added. No courtyard required. Verify actual public access/directions later. Carry traffic sound through the corner; connect designed gutters only after slope/outfall review. No shore or deck interface selected; all heights unknown. |
| VI outgoing composite port | Source reference: Adeola/Eletu nodes 1235946207 and 2339926116. Destination reference: Fola/Kafayat node 1513601093. **No direct real adjacency established.** Intervening Victoria Island streets, Oniru/western Lekki approaches and Phase 1 access streets are omitted from this montage; no road-by-road itinerary claimed. | **P reciprocal composite join with Lekki incoming:** coordinator may insert a full-scale transition street or travel transition between these references. Proposed 8.0 m vehicle envelope, 2.0 m pedestrian continuation, 4.5 m overhead clearance where needed. Do not invent a named real connecting road, bridge or sea gap. Public-route intent; gates/private yards excluded. Traffic/audio changes progressively, not abruptly to surf; separate drainage catchments until height evidence exists. |

## Slice 2: Lekki Phase 1 pharmacy frontage and side-street circuit

### Anchors and exact local topology

**F [L1-L5]:** OSM records Fola Osibo Street and named residential side streets. Medplus's primary store page uses the variant **Fola Osibo Road** for its address. Retain this street/road wording difference in research metadata; do not interpret it as two separate roads. The pharmacy's stated evening hours establish a local service rhythm, not all shops' hours. NTDA's 2020 report supports historical road/drainage maintenance concerns elsewhere along the Fola junction context; it does not prove that this chosen block currently floods.

| Feature | OSM ID and version | Edit date UTC | Relevant tag or membership |
| --- | --- | --- | --- |
| Fola Osibo main street | way 138040470 v10 | February 5, 2018, 20:21:03 | `tertiary`, `surface=paving_stones`; ordered nodes 1598668829, 1513601100, 1513601093, 1513601085, 1513601086, 2981944792. |
| Theophilus Oji | way 138040497 v6 | October 28, 2017, 01:46:12 | Shared Fola node 1513601100; via 2981943415 and 1513601032 to 1513600977. |
| Kafayat Abdulrasaq | way 138040502 v6 | October 28, 2017, 01:46:12 | Shared Fola node 1513601093; via 1513601066 and 1513601026 to 1513600978. Preserve map spelling pending sign/photo review. |
| Osaro Isokpan | way 152415208 v3 | October 21, 2023, 10:29:53 | Nodes 1513601032, 1513601028, 1513601026 join the two side streets south of Fola. |
| Nearby residential crescent | way 138040476 v3 | July 27, 2014, 00:01:15 | Olarenwaju Ninalowo Crescent shares node 1513601066 with Kafayat. Naming and travel-agency occupancy need contemporary confirmation. |
| Eastern branch | way 138040483 v6 | August 2, 2022, 10:21:05 | Grace Anjos Drive connects to Fola at node 1513601085. |
| Fola bridge reference, outside selected west block | way 294603096 v3 | February 5, 2018, 20:21:04 | Nodes 2981944792, 2981943382; `bridge=yes`, `layer=1`; east end joins Emma Abimbola Cole way 334645555 v6, edited August 18, 2025. No deck height/clearance surveyed. |
| Channel context, outside selected west block | way 146603442 v6 | June 19, 2018, 19:34:51 | `name=Fola Osibo Water Channel`, `waterway=drain`; channel geometry has no shared node with the bridge deck. Do not snap water to road level or represent crossing lines as a street junction. |

| Junction anchor | `(longitude, latitude)` | Node version / edit date UTC |
| --- | --- | --- |
| Fola / Theophilus, node 1513601100 | `(3.4650059, 6.4426434)` | v7, June 16, 2018, 22:05:53 |
| Fola / Kafayat, node 1513601093 | `(3.4667547, 6.4426434)` | v11, April 1, 2024, 15:09:46 |
| Theophilus / Osaro, node 1513601032 | `(3.4650058, 6.4409989)` | v3, December 2, 2017, 20:27:12 |
| Kafayat / Osaro, node 1513601026 | `(3.4667660, 6.4409957)` | v5, April 1, 2024, 15:09:46 |
| Fola / Grace Anjos, node 1513601085 | `(3.4681226, 6.4426433)` | v12, June 16, 2018, 22:05:53 |
| Fola / Kayode Animashaun, node 1513601086 | `(3.4690386, 6.4426433)` | v8, June 16, 2018, 22:05:53 |
| Fola bridge west/east nodes 2981944792 / 2981943382 | `(3.4712405, 6.4426433)` / `(3.4714758, 6.4426399)` | Both v2, January 23, 2018, 21:13:25 |

**M:** node membership supports the circuit Fola -> Theophilus -> Osaro -> Kafayat -> Fola, with a crescent branch off Kafayat. This is a candidate pedestrian circuit, not a confirmed accessible public route or approved driving itinerary. Missing `access` or `oneway` tags do not establish unrestricted access. The bridge/channel reference is farther east and must not be squeezed into this short western walk to manufacture a waterfront.

**G:** pharmacy address is not geolocated to an exact parcel within the two junctions; no dated image establishes current buildings, traffic, pavers, gate positions, sidewalk continuity or income/tenure mix. Mapped travel agency node 11998537311 at `(3.4669599, 6.4415936)` is a nearby service lead, not proof of current operation. No verified market or beach is within the selected block; do not add one under a real name.

### Authored 260 m walk and connected side streets

**P throughout this subsection.** Start at the Kafayat/Fola reference and walk toward Theophilus, then turn south. A full circuit remains a separate longer optional route; it is not presented as a 260 m surveyed loop.

| Cumulative walking distance | Authored encounter and navigation |
| --- | --- |
| 0-45 m | Kafayat corner: unnamed pharmacy entry, a recessed collection window and a shade bench; a commuter collects a prescription while another resident continues without shopping. Reproduce no Medplus branding or patient data. |
| 45-105 m | Fola frontage: everyday provisions shop, laundry collection and a small clothing-alteration room sharing a forecourt. Clothes rails and waiting customers remain inside the lot. Put a modest upstairs flat beside a more spacious gated compound rather than repeating identical upscale blocks. |
| 105-155 m | Break in retail: a household gate, planted setback, shaded sitting place and service vehicle entrance. A caretaker receives maintenance equipment; residents talk or work on a laptop at an optional invitation-only verandah. No claim that this is a mapped real household. |
| 155-195 m | Approach Theophilus: an unnamed repair room with bicycle and appliance parts, a stopped delivery handcart and a visible turn into the side street. Keep the Fola through-path legible beyond the turn. |
| 195-260 m | Turn south onto Theophilus to a quieter pair of compounds: shared drying courtyard in one, small professional office reception in the other. Osaro is the continued public reference farther along, not silently relocated into this 65 m arm. Courtyards are optional destinations, never essential shortcuts. |

**P dimensions:** Fola carriageway 8.0 m, 2.0 m clear pedestrian strip each side, 3.0 m setback pockets for service activity; side streets 6.0 m carriageway and at least one continuous 1.8 m pedestrian strip; Osaro alternate has a 2.0 m clear walking route. Frontages 9-20 m, shop room 5 x 7 m, alteration room 4 x 5 m, shared courtyard 12 x 14 m, verandah 2.4 m deep, shaded rest pocket 3 x 4 m, proposed headroom 2.4 m. One- to three-storey shells vary in maintenance and fit-out. These are proposed dimensions/heights, not OSM-derived measurements or age claims.

**P:** retain meaningful side-street depth in the future composition. At each junction, the next corner or a repeatable wall/gate feature should reveal continuation. A view down Osaro connects households and services, not an empty back-of-map alley. Do not require the entire circuit to fit inside the 150-300 m showcase walk.

### Ordinary interiors, activity and socioeconomic texture

**P:** usable destinations are a pharmacy collection counter, alteration pickup, provisions shop, laundry counter, repair booking, professional-office reception and invited home visit. Provide stock shelving, ordinary baskets, measuring/cutting table, iron rest, sewing-machine work area, tool cupboard, indoor fan, domestic washing corner and separate utility storage. None is evidence of a real business interior. Medical stock is fictional and interaction stays within a professional counter boundary; no diagnosis game or disclosure of another person's records.

**P:** layer proprietors, employees, residents, visitors, maintenance workers and couriers across the same block. Shared household facilities and small workrooms coexist with better-equipped offices and larger compounds; these are design choices for heterogeneous everyday life. Do not make every resident wealthy or every service worker poor, and do not equate weathering with poverty. Retain clean modest interiors and repaired high-investment frontages as well as the reverse. All genders participate in technical, commercial, domestic and social tasks.

**F [L4] / P:** use the primary pharmacy's published weekday/weekend hours only as a reference for an authored early-evening service counter. Other shop routines are invented: morning opening and household departures, midday alterations/repair collections, evening food preparation and conversation, then quieter residential sound after shutters close. Do not claim universal 24-hour nightlife or a particular religious/ethnic routine for this block.

### Developer Asset Checklist

| Category | Required assets and behavior, all P |
| --- | --- |
| Meshes / architecture | Recessed pharmacy shell with private dispensary boundary; provisions shelving; alteration table/machine; laundry counter; modest flat and larger compound variants; verandah; gated shared courtyard; service entrance with separated pedestrian leaf; simple office reception; repair rack; drying rack; planter; bicycle; covered gutter crossing. Keep eastern bridge/channel modules a deferred context set, not mandatory assets inside this slice. |
| Materials / textures / shaders | Original interlocking-paver option based on the recorded Fola tag, with alternate renewed/patch textures until images resolve current surface; plain render; ceramic shop-floor option; painted metal; timber furniture; privacy glazing; corrugated utility-cover option; fabric shade and household textile variants. No texture copied from NTDA or business photos. Per-lot wetness/maintenance variation rather than a uniform luxury or decay pass. |
| Systems / spawners | Separate pedestrian/service vehicle priorities; delivery and pickup reservations; alteration/laundry collection tasks; courtyard invitation state; household sit/work/cook routines; shutters with independent schedules; rain shelter destinations outside the clear strip; bounded waiting groups; accessible-route exclusions; repair event and public alternate-route state. Avoid unsourced motorcycle route assumptions. |

### Audio, day-night and weather

**P:** Fola traffic provides a mid-distance bed; the side-street turn lowers it rather than eliminating it. Sewing-machine bursts, repair tools, delivery trolleys and domestic cooking/sitting sounds occur at their locations and hours. Spoken lines require Lagos-based language review and consented recording; English, Nigerian Pidgin and Yoruba may be candidate review languages, not a measured distribution on this block. Never use a stock shouted market loop to turn all frontage into a market.

**P:** dusk leaves selected service interiors open while residential windows and gate lights differ by compound. Orient with readable entrances, wall breaks and lit corners; do not require hearing a vendor call or seeing a color alone. Wet paving reflects local lights with restrained roughness changes, not a mirror over every surface.

**F [L5] / P:** historical maintenance/drainage concerns justify studying a rain-response design, not asserting present flooding. Our simulation sends roof/forecourt runoff to separately designed local gutters; shallow puddles may occupy setback pockets, not the sole accessible route. Event rain changes shelter and delivery behavior. No numeric real rainfall, tidal backwater, drain capacity, water depth or street elevation is available. The eastern channel's eventual outfall and bridge deck remain unresolved; coastal context does not justify audible surf, sand coverage or salt damage throughout this inland block.

**P seasonal presets:** a wetter variant increases canopy use and localized runoff; a drier/hazier variant changes distant contrast and shifts sitting groups toward shade. Neither preset asserts an observed seasonal calendar, wind direction or local climate measurement. Do not recolor every lot or close the side-street circuit merely to signal a season.

### Fictional disruption: canopy brace failure after rain

**P:** rain loosens a removable canopy brace over an unnamed laundry frontage between the Fola junctions. Staff lower and fold the canopy; the folding work zone temporarily diverts pedestrians from one strip. This is not an incident at Medplus or an assertion of unsafe real construction.

- Exit A: use the opposite Fola strip through authored low-speed, step-free crossings at the work-zone ends; protect 1.8 m clear width from customers and props.
- Exit B: at the eastern junction turn into Kafayat, continue to Osaro, then return via Theophilus. The underlying circuit is **M** map-supported; its playable step-free/public status is a **P** requirement pending real access review. No private crescent or courtyard is mandatory.
- Exit C: return to the Kafayat incoming port or continue west from Theophilus on the authored Fola continuation. The event must not close both ports.

**P clearing condition:** staff lower the canopy safely, remove its brace and clear the folded fabric into the forecourt. A technician then confirms the passage clear; replacement installation can occur later without closing the route. Proposed 120-240 second active-work window; the player can continue immediately on an alternate. Keep shelter and visual route cues available without requiring sound or a timed agility task. If the canopy task stalls, the alternate path remains available.

### Incoming / outgoing connector briefs

| Connector | Evidence relationship and omitted geography | Authored handoff |
| --- | --- | --- |
| Lekki incoming composite port | Source reference: Adeola/Eletu nodes 1235946207 and 2339926116. Destination: Fola/Kafayat node 1513601093. Same proposed composite join as VI outgoing, **not a second real link**. The real street-to-street itinerary is unverified; intervening developed neighborhoods and access streets are deliberately omitted, not represented as vacant land. | **P:** reciprocal 8.0 m road-envelope / 2.0 m walking continuation intent, with vehicle access conditional on coordinator routing. Public path, no estate/private gate traversal; proposed 4.5 m clearance where relevant. Gradient, drainage and datum unresolved. Crossfade frontage traffic and service audio through a transition, with no invented water crossing. |
| Lekki outgoing local eastern feeder | Source: Fola way 138040470 after Kafayat node 1513601093. Destination: Grace Anjos way 138040483 at shared node 1513601085. **M real mapped branch**; the intervening Fola frontage east of the selected walk is outside scope and must be retained or explicitly compressed as a journey. | **P:** extend pedestrian continuation at 2.0 m and reserve 8.0 m road envelope until actual section review; courtyard access remains private/optional. Proposed 4.5 m overhead vehicle clearance if needed. Carry traffic and gutter design through without inventing outfall elevations. This port is land-street based, not a beach exit. |

**Further eastern context, not an approved connector:** Fola continues in the mapped sequence to bridge way 294603096 and Emma Abimbola Cole way 334645555 at node 2981943382. No bridge deck elevation, railing condition, pedestrian clearance, drain water level or shoreline profile was verified. The channel and bridge are separate layers. Keep them outside this compact slice unless the coordinator allocates a separate full-scale transition. Neither this bridge nor the composite port is the Lekki-Ikoyi Link Bridge; no Third Mainland Bridge connection is asserted.

## Shared navigation, accessibility and cultural guardrails

All requirements here are **P** for future authoring, not claims that the real streets already comply.

- Preserve human-scale building, vehicle and courtyard dimensions; compress travel between slices, not household rooms or side-street life. Maintain quiet intervals and optional domestic destinations alongside service frontage.
- Keep a continuous step-free public route to each public destination and each connector. Proposed minimum clear width 1.8 m, turning/rest space 1.5 x 1.5 m or larger; use flush covered drain crossings and threshold ramps designed to a later chosen accessibility standard. These are gameplay targets, not legal-compliance certification.
- Provide low-stimulation alternatives, visible destinations, rest shade, redundant shape/text cues and controllable audio intensity. No required traversal through water, drains, live carriageways without an authored protected crossing, or invitation-only rooms.
- Make gates distinguish public shops, staff-only areas and private homes. Separate NPC draft/queue space from pedestrian circulation; cancellation of an interaction cannot trap a person inside a compound.
- Treat households and businesses as ordinary places with privacy, competent workers and social life. No ethnic/religious assignment from a name; no poverty spectacle, automatic hostility at gates, forced tipping, criminalized livelihoods or gendered labor stereotypes.
- Use original unbranded fictional signs in production metadata; Lagos-based reviewers must approve idiom, names, culturally specific food/dress/artwork and recorded speech before production. Do not insert invented Yoruba or Pidgin dialogue as authentic local evidence.
- Do not convert NTDA's historical official maintenance narrative into blame assigned to residents, or turn all rainy conditions into crisis. People can work, relax and travel in varied conditions without every scene becoming a hazard.

## Evidence gaps and coordinator handoff

| Unresolved item | Consequence / required next evidence |
| --- | --- |
| Contemporary street imagery and capture dates | No visual material/architecture acceptance. Obtain geolocated, dated and permission-compatible street-height references for both exact slices; image captions alone are insufficient. |
| Public/private access, sidewalk continuity and real crossing controls | OSM topology is conditional. Verify Amichi gates, the Lekki side-street circuit and carriageway crossings before any factual route or accessibility claim. |
| Parcel frontage, buildings, business locations and socioeconomic mixture | Primary addresses do not locate exact shop bays or certify surrounding tenure. Our mixed compounds and interiors remain fictional; local observation/interviews with consent are needed for more precise claims. |
| Heights, widths, slopes, drainage, bridge/channel levels | All dimensional sections are P. No survey/DEM datum or bathymetry acquired; do not derive elevations from road tags, decimals, `layer` or the parent reference plane. |
| Photographic, audio, logo and interior permissions | No production texture, photograph, music, branded signage or voice rights cleared. ODbL provenance does not clear these assets. |
| External itinerary and montage placement | No verified real direct Adeola-to-Fola link; coordinator owns omitted journey, overlap reconciliation and final placement. Reciprocal composite briefs describe one fictional join, not actual adjacency. |
| Archival reproducibility | Selected IDs, versions, edit timestamps and node coordinates retained; full extract not archived under this single-file restriction. Later capture may differ. |

**Delivery:** research/design is useful but conditional; geographic, cultural and visual acceptance remains incomplete where noted. Only this dossier is authored. No engine/native/web/mobile/config changes, renderer packaging, server starts, builds, tests, lint, typechecks, purchases, public messages, further agents or git commit are part of this pass.
