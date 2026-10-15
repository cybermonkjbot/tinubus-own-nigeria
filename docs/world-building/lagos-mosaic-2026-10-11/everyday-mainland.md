# Everyday mainland: Surulere and Mushin

Research access date: October 11, 2026. Documentation and authored composite-world proposals only. Assignment-local labels `EM-S` and `EM-M` are not canonical district IDs.

## Scope and evidence contract

We select two small reference slices: **Olufemi Street between Olukole Street and Ajao Street, Surulere**, and **Palm Avenue between Olorunsogo Street and the unnamed residential way 441859323, Mushin**. Their side streets remain part of the experience, not scenery behind a landmark. Ogunlana Drive and Sokunbi Street provide documented approach references, not invitations to model whole districts. No Oshodi terminal, interchange or market-approach planning is duplicated.

The sequences below are fictional arrangements informed by these slices. We compress intervening journeys, not people, doorways, compounds or vehicles. They are not surveyed walking routes, verified current streetscapes or claims that Surulere and Mushin directly adjoin. There is no assumed 6 by 4.5 km envelope.

Evidence labels apply throughout:

- **F: source-confirmed record.** A retrieved source actually contains the stated tag, coordinate, address or description. An OSM tag confirms a database record, not contemporary ground truth. A directory confirms a listing, not current trading or exact placement.
- **I: map-supported inference.** Shared-node topology, relative orientation and likely route choices derived from the retrieved map. Access, safe passage and present condition remain unverified.
- **P: authored proposal.** Fictional frontages, interiors, people, activity schedules, sounds, event logic and dimensions. All dimensions in design sections are design values, never measured street widths or elevations.
- **G: gap.** Unresolved evidence or rights clearance that prevents a stronger claim.

Geographic coordinates below are WGS84 **(longitude, latitude)**, in degrees, with no altitude. OSM XML uses named `lon` and `lat` attributes; its map API bbox is **west longitude, south latitude, east longitude, north latitude**. Printed decimals reproduce records and do not imply survey accuracy. Source metre-space remains **x east, y up, z south**; `UE(X,Y,Z) = 100 * source(z,x,y)` applies once after coordinator-approved registration. No geographic origin, elevation datum, final engine positions, theatre relocation or canonical relabeling is assigned here.

Read repository guidance: `AGENTS.md`, `CONTEXT.md`, `docs/world-building/README.md`, and the coordinator-owned `docs/world-building/lagos-mosaic-2026-10-11/README.md`. Existing sector documents and all canonical content remain untouched.

## Source register and limits

Research retrievals and failed attempts occurred on **October 11, 2026**. Feature records were read either directly at the listed extract URL or inside the explicitly identified containing map extract; record links for the latter are reproducibility references, not claims of separate page inspection. Live API versions are identified below; a later retrieval may change. Only this Markdown file is delivered: no raw XML, imagery, extracted geometry or external assets are saved. Reproducible URLs plus versions and selected nodes are included, but a complete frozen extract is an outstanding archival task.

| Ref | Exact source URL | Retrieval and evidence | Date and use limits |
| --- | --- | --- | --- |
| O-S | https://api.openstreetmap.org/api/0.6/map?bbox=3.349,6.505,3.353,6.5075 | Retrieved XML: Olufemi and intersecting streets, tagged worship/parking features and building footprints | Retrieval date is not imagery date; ODbL data |
| O-M | https://api.openstreetmap.org/api/0.6/map?bbox=3.3535,6.5345,3.356,6.5365 | Retrieved XML: Palm Avenue, Olorunsogo, unnamed branches, building footprints and a clinic node | Same limits; bbox is a research window, not a district boundary |
| O-L | https://www.openstreetmap.org/copyright | Retrieved OSM attribution/license statement | OSM data under ODbL; no photo, music, private-interior or trademark permission follows |
| L-S | https://www.finelib.com/cities/lagos/areas-and-suburbs/surulere | Retrieved local directory; lists food, home repair, computing, beauty, printing and other service categories | No listing observation/publication date supplied; secondary local source, all-rights-reserved footer |
| L-SC | https://www.finelib.com/cities/lagos/areas-and-suburbs/surulere/computer-services | Retrieved directory descriptions of computer sales/repair in Surulere | Addresses are elsewhere in Surulere, not proof of a repair shop on selected Olufemi block; no publication date supplied |
| L-M | https://www.finelib.com/cities/lagos/areas-and-suburbs/mushin | Retrieved directory; includes a legal-services listing at 13 Oliyide Street, Palm Avenue, and furniture/carpentry repair at 99 Itire Road | Listing spelling retained; neither address is geolocated into this slice; undated secondary source |
| L-MH | https://www.finelib.com/cities/lagos/areas-and-suburbs/mushin/home-and-living | Retrieved directory: Beji Trading Store at 2A Amu Street off Olorunsogo, described as furniture-accessories retail; Dambee Interiors on Buhari Street off Olorunsogo, described as furniture retail | Supports an Olorunsogo-area furniture-related activity reference, not a street-by-street workshop survey; publication date unknown |
| L-MF | https://www.finelib.com/cities/lagos/areas-and-suburbs/mushin/food-services | Retrieved directory: grocery, catering and bakery listings in Mushin | Supports varied district livelihoods only; no placement on Palm/Olorunsogo inferred; undated |

Primary geographic evidence is the OSM API. Local directories supplement it without establishing architectural appearance or ethnographic observations. No photographic original, street video, aerial frame or private interior was visually inspected. Imagery capture dates are **unknown**. Some Surulere ways carry `source=BING`; this identifies contributor provenance, not an inspected image or capture date. No business logo, photograph, directory prose, recording or private address is proposed as a reusable game asset.

OSM-derived data requires OpenStreetMap contributor attribution and an ODbL notice; derivative-database obligations must be evaluated before production distribution. A fictional scene is not automatically a license-free substitute for copied map data. Keep the evidence layer traceable and have the coordinator resolve downstream attribution. Website availability and directory listing do not grant reproduction rights.

### Failed access and discovery-only leads

These attempts establish gaps, not facts to place in the world:

- Overpass GET `https://overpass-api.de/api/interpreter?data=%5Bout%3Ajson%5D%5Btimeout%3A25%5D%3Bway%5Bhighway%5D%5Bname~%22Ogunlana%7COlufemi%7CPalm%20Avenue%7CFafolu%7COlateju%22%5D%286.48%2C3.32%2C6.55%2C3.37%29%3Bout%20body%3B` returned HTTP 406. Direct OSM API retrieval succeeded instead.
- `https://lagosstate.gov.ng` returned a JavaScript shell with the government website title; no slice-specific article was retrievable from that response. It supplies no ground-condition evidence here.
- `https://goodnewsworld.org/` and `https://goodnewsworld.org/contact/` failed DNS resolution. The OSM-linked church website was not independently confirmed.
- `https://rhbooks.com.ng/contact-us/` returned 403. No bookshop placement is claimed.
- `https://ng.africabz.com/lagos/d-arena-supermarket-355754`, `https://www.findglocal.com/NG/Surulere/123602854331435/Numerique-Limited`, and `https://nigeria-streets.openalfa.com/streets/olufemi-street-surulere-lagos` returned 403. Search results mentioning them are discovery leads only, not current business confirmation.
- `https://propertypro.ng/property/2-bedroom-flat-apartment-for-rent-palm-avenue-mushin-lagos-0QEZR` and `https://jiji.ng/ha/mushin-challenge/commercial-property-for-rent/500sq-m-self-compound-warehouse-with-office-at-palm-av-mushin-for-leas-bsvx4HCoTyEaOWQ4IWzqrRBK.html` returned 403. Search snippets mentioned Palm Avenue housing and warehouse accommodation; prices, dimensions, conditions and listing dates are not accepted evidence here.
- `https://html.duckduckgo.com/html/?q=Palm+Avenue+Mushin+street` supplied discovery results, and `https://html.duckduckgo.com/html/?q=Olufemi%20Street%20Surulere%20businesses` supplied address leads. Later searches hit a human-verification challenge; it was not bypassed. Search snippets do not establish inspected imagery or present operation.

## EM-S: Olufemi / Olukole / Ajao, Surulere

### Source anchors and street graph

**F, O-S:** Olufemi Street is tagged `highway=unclassified`, `surface=paved`. Olukole, Ajao and Simisola are tagged residential and paved. The selected junctions share actual node IDs, not merely crossing lines on a picture.

| Feature | OSM version and last edit UTC | Selected record / exact extract URL |
| --- | --- | --- |
| Olufemi Street, way 92439856 | v10; 2024-06-04T15:35:10Z | https://api.openstreetmap.org/api/0.6/way/92439856/full |
| Olukole Street, way 138058050 | v6; 2025-03-06T09:16:15Z | https://api.openstreetmap.org/api/0.6/way/138058050/full |
| Ajao Street, way 92439848 | v8; 2025-03-07T08:56:09Z | https://api.openstreetmap.org/api/0.6/way/92439848/full |
| Simisola Street, way 205394916 | v4; 2025-03-06T09:16:15Z | https://api.openstreetmap.org/api/0.6/way/205394916/full |
| Adegoke Street Link, way 205394913 | v3; 2025-03-06T09:16:15Z, from O-S XML | Joins Olufemi at node 2153655665; residential, paved. Record URL: https://api.openstreetmap.org/api/0.6/way/205394913 |
| Ogunlana Drive, way 583323034 | v5; 2024-06-04T15:35:10Z, from O-S XML | Joins Olufemi at node 5576802509; tagged secondary, four lanes, one-way, paved. Record URL: https://api.openstreetmap.org/api/0.6/way/583323034 |
| Short unnamed linking way 583328795 | v3; 2025-03-07T07:20:47Z, from O-S XML | Node order 5576882410 -> 5576802509; unclassified, unpaved tag. https://api.openstreetmap.org/api/0.6/way/583328795 |
| Other Ogunlana segment, way 12339482 | v14; 2024-06-04T15:35:10Z | Contains 5576882410, not Olufemi's 5576802509. https://api.openstreetmap.org/api/0.6/way/12339482/full |
| Building sample, way 1283960556 | v1; 2024-05-17T11:24:21Z | `building=yes` only; no dwelling type, storeys or date. https://api.openstreetmap.org/api/0.6/way/1283960556/full |
| Good News Baptist Church, way 790773375 | v2; 2024-05-17T11:26:19Z | OSM records worship, Baptist, Christian and address 47;49 Olufemi Street. Outside the selected junction-to-junction core; contextual anchor only. https://api.openstreetmap.org/api/0.6/way/790773375/full |

Node anchors, all **(longitude, latitude)**:

| Node | Coordinate | Role and exact source |
| --- | --- | --- |
| 5576802509 | (3.3521531, 6.5058046) | Olufemi / Ogunlana segment 583323034; from Olufemi full extract |
| 1513671881 | (3.3515589, 6.5060125) | Olufemi / Olukole; node v5, 2024-05-18T05:10:26Z. https://api.openstreetmap.org/api/0.6/node/1513671881 |
| 1073171129 | (3.3507402, 6.5063262) | Olufemi / Ajao; node v5, 2025-03-06T08:18:09Z. https://api.openstreetmap.org/api/0.6/node/1073171129 |
| 1073171220 | (3.3498564, 6.5066554) | Olufemi / Simisola; from Olufemi full extract |

**I:** In the inspected node order, Olufemi runs from Ogunlana through Olukole, Ajao, Adegoke Street Link and Simisola. Olukole extends broadly southward; Ajao and Simisola extend broadly northward. These give distinct branch choices. The mapped Ogunlana segments and their short link must not be collapsed into a fictitious single safe crossing. Four-lane and one-way tags are unverified traffic records, not measured clearance or legal advice. No sidewalk, curb, median or crossing provision is confirmed.

**G:** The building sample lies near the research window's northern edge, not a verified dwelling on the selected frontage. We cannot infer a compound plan from its outline. The church tag does not establish public through-access, opening hours or a present congregation. OSM parking way 790773379 is tagged `access=customers`; it is not a public bypass.

### Authored walking-height sequence: 240 m

**P, all distances and structures below:** We reinterpret the Olukole-to-Ajao frontage with an Olukole residential excursion and a fictional public return lane. Chainage measures authored travel distance, not OSM segment length. No invented real street name is attached to the return lane.

| Chainage | Walking sequence and ordinary destination |
| --- | --- |
| 0-35 m | Enter Olufemi-inspired frontage at a small covered food counter. A seated customer and a person collecting a packed meal share the forecourt; the cooking area is behind the counter, not in the movement strip. A clearly shaped canopy marks the junction rather than a giant district sign. |
| 35-75 m | Turn into Olukole-inspired residential frontage. Pass a repaired gate and a quieter planted setback, then an occupied rental compound beside a maintained detached-home frontage. Both have normal maintenance and individual belongings; neither is a visual shorthand for virtue or disorder. |
| 75-105 m | Reach an unnamed fictional alterations shop in a shallow street-facing room. Its customer area connects to a side yard only by invitation. An upstairs landing and an enclosed household door remain non-public. A resident carries laundry to the yard while another works at a table. |
| 105-140 m | Follow a fictional public return lane behind the commercial frontage, with service doors, a covered drain crossing and a quiet wall interval. It does not pass through someone's living room or the real church parking. |
| 140-200 m | Rejoin Olufemi-inspired street. Pass a small grocery, an independently authored phone/computer repair counter and a professional-services upstairs entrance. Repair trade is a district-informed proposal from L-SC, not a verified Olufemi tenant. |
| 200-240 m | Reach Ajao-inspired junction. A bench-sized rest recess, visible return lane and food-canopy silhouette make three route choices legible. Continue on Ajao, back along Olufemi or through the public lane; an optional compound visit remains separate. |

**P dimensions:** Main frontage corridor 12 m overall, with 6 m carriageway, two 2 m clear pedestrian strips and two 1 m frontage/service buffers. Residential branch carriageway 4.5 m, with a 1.8 m continuous clear pedestrian route. Public return lane 2.4 m clear. Shop bays 3-4.5 m wide, 4-6 m deep; residential compound courtyard 9 by 12 m; detached-home setback 4 m; fictional ground-floor rooms approximately 3.5 by 4 m. One- and two-storey modules use 3 m floor-to-floor spacing. These are usability/art-direction values, not measurements or claims about local architectural dates.

### Interiors, compounds and everyday activity

**P:** Keep the food counter, grocery, repair counter and alterations shop as four distinct destinations. Give each a small usable interior, stock/storage separation, a customer position and an accessible exit. The repair room has a work mat, parts trays and collection shelf; the alterations room has a sewing table, folded fabric and a fitting enclosure. Upstairs services also have a ground-floor consultation/handoff room so no public activity requires stairs. No real trade name or logo is reproduced.

**P:** The rental compound has separate front doors around a shared yard, washing space and a shaded seat. The detached-home frontage is quieter but has repairs and deliveries too. A household in each setting can have a desk, ordinary kitchen storage and charging devices. Visible finishes vary within both, rather than making every Surulere household comfortable and every Mushin household deprived. Occupancy, tenure and income are invented production metadata, not demographic findings.

**P:** Morning includes opening shutters, household departures and breakfast collection; midday includes repairs, grocery restocking and an occupied but less busy yard; evening includes returning residents, food pickup and a closed shop beside an open one. People may linger, wait, work or decline interaction. Staff and household roles are not assigned by gender. Door states and invitation states make private rooms legible without mandatory explanatory dialogue.

### Developer Asset Checklist

All entries are **P**; no assets are produced or registered in this assignment.

| Category | Required content and behavior |
| --- | --- |
| Meshes / architecture | Distinct food counter, grocery shelving, repair bench, sewing room and upstairs entrance; one- and two-storey facade modules; rental-yard doors; detached-home gate/setback; exterior landing; canopy; drain covers; refuse containers; quiet wall and planted recess; accessible rest seat. Keep roofs and upper rooms from implying an architectural era. |
| Materials / textures / shaders | Authored rendered masonry, clean and repainted wall variants, matte steel gates, sheet-metal canopy, floor tile, concrete yard, paving repair patches, fabric and timber furniture; wetness/roughness variants and window emission masks. Vary repair and care independently of income. No asserted real facade palette; no photographs copied from search results. |
| Systems / spawners | Shop open/closed states; short customer queues with clear-route exclusion; resident/visitor invitation gates; delivery handoff; laundry/repair tasks; seated conversation; yield-to-pedestrian vehicle behavior; public-route reservation; event blocker and restoration; separate indoor/outdoor audio zones. Counts and timing remain design tuning, not crowd observations. |

### Audio, lighting, weather and accessibility

**P:** Use localized shutter movement, food-preparation utensils, low conversation, repair-tool taps and sewing-machine sound. Ogunlana-inspired arrival may carry distant traffic that falls behind walls on the residential branch. Do not use one loud market bed everywhere. Commission original dialogue with locally reviewed Nigerian English, Yoruba and Pidgin where appropriate; neither language nor accent is inferred for a particular resident from the map. No unlicensed song, prayer or field recording.

**P:** Morning shadows reveal recesses; midday canopies shade shop entrances; evening combines occupied-window light, a repair-counter lamp and closed shutters. Provide readable navigation without forced power-failure darkness. Rain sends authored roof runoff into drain assets, keeps covered crossings usable and changes footsteps by surface. A dry/dustier seasonal variant is optional, not a sourced local weather calendar. No real rainfall intensity, drain capacity, flood frequency or elevation is supplied.

**P:** Preserve a 1.8 m clear accessible route to each public destination and 1.5 m turning/rest clearances. Fictional accessible ramps target no steeper than 1:20; these are game-design targets, not certification of local compliance. Cover mandatory-route drains, keep furniture out of the route and provide non-color-only junction cues. Avoid compulsory curb jumps, crowded squeeze passages or navigation dependent on hearing. Private compound refusal never blocks progress.

### Fictional disruption: gate-panel delivery at the alterations frontage

**P:** A small delivery vehicle arrives with a replacement gate panel. The crew temporarily occupies the near-side Olukole-inspired branch frontage at authored chainage 70-105 m while maneuvering the panel into a service recess. Reserve public lane entries on both sides of that frontage; its invitation-only yard is separate. This is not a reported Lagos incident, enforcement encounter or depiction of a real business.

- Exit A: continue via the protected opposite-side branch route, using a fictional traffic-held crossing; neither crossing permission nor crossing infrastructure is claimed as real.
- Exit B: use the always-public return lane to rejoin Olufemi beyond the delivery.
- Exit C: withdraw toward the food-counter junction and take the open Olukole-side continuation. Ajao-side continuation remains reachable from the far end.
- Optional invited-yard route is a fourth choice only when already accessible; it is never required and never silently converts a home into public land.
- Clear when the panel is placed inside its recess, crew and vehicle leave the reserved route, and the barrier is removed. A bounded design fallback clears a stalled delivery without requiring player labor or payment. Event activation must leave two accessible public routes open from either approach; otherwise it does not spawn.

### Connector handoff

| Connector | Source and destination references / supported relationship | Authored join, movement and continuity |
| --- | --- | --- |
| Incoming local approach | Ogunlana 583323034 to Olufemi 92439856 at 5576802509: shared-node connection confirmed. Other Ogunlana segment 12339482 connects through short way 583328795. | P: retain a readable junction and 1.8 m pedestrian continuation; car access conditional on future traffic/access verification. Do not fabricate an accessible crossing between mapped segments. Buffer traffic sound into quieter frontage; drain continuity and road levels unknown. No district-scale journey omitted in this immediate junction brief. |
| Outgoing local branches | Olufemi to Olukole at 1513671881 and Ajao at 1073171129: shared nodes confirmed; Simisola at 1073171220 is a further documented branch. | P: preserve separate residential continuations, service access and public walking route. Proposed branch mouth clear pedestrian width 1.8 m; no private through-route. Beyond the slice, leave geometry unauthored rather than implying dead ends. Drain junctions, grades and traffic direction unknown. |
| Optional outgoing composite handoff | Source-side Ajao node 1073171129; destination-side Palm/Olorunsogo node 4395916411 in EM-M. No direct adjacency or verified route between these nodes established. | P: an unplaced, 60-90 m transition with ordinary residential frontage and two turns; 6 m authored local carriageway and 1.8 m clear walking strips. Omitted geography is occupied mainland street fabric, junctions, homes and services, not vacant land; exact intervening route intentionally unresearched. Keep pedestrian and optional local-vehicle modes, public access and distinct service yards. Reset drainage to a separate fictional catchment; fade traffic rather than joining a fictional single drain. Destination accepted only through coordinator reconciliation. |

## EM-M: Palm Avenue / Olorunsogo, Mushin

### Source anchors and street graph

**F, O-M:** Palm Avenue is tagged tertiary and unpaved on way 11102731; Olorunsogo is residential and unpaved on way 441897768. Unnamed side ways have residential tags. These tags do not prove today's mud, potholes, road quality or vehicle rights. The neighborhood reference is working/residential, not an assertion that all Mushin is industrial or poor.

| Feature | OSM version and last edit UTC | Exact extract URL / retained evidence |
| --- | --- | --- |
| Palm Avenue, way 11102731 | v21; 2026-05-07T06:50:18Z | https://api.openstreetmap.org/api/0.6/way/11102731/full |
| Olorunsogo Street, way 441897768 | v7; 2024-03-23T16:52:15Z | https://api.openstreetmap.org/api/0.6/way/441897768/full |
| Unnamed residential side street, way 441859323 | v5; 2026-04-29T05:30:18Z | https://api.openstreetmap.org/api/0.6/way/441859323/full |
| Unnamed cross street, way 441859324 | v4; 2026-04-29T05:30:18Z | https://api.openstreetmap.org/api/0.6/way/441859324/full |
| Sokunbi Street, way 11102602 | v10; 2025-08-12T15:53:37Z | https://api.openstreetmap.org/api/0.6/way/11102602/full; tertiary, unpaved tag |
| Building sample, way 618828917 | v1; 2018-08-22T14:36:24Z | https://api.openstreetmap.org/api/0.6/way/618828917/full; `building=yes` only. Edit date is not construction date |
| Fafolu Clinic and Maternity Home, node 11926018063 | v1; 2024-05-22T15:49:31Z | https://api.openstreetmap.org/api/0.6/node/11926018063; tagged hospital, private operator, (3.3553952, 6.5346151). Context anchor only; current service, entrance and routing unverified |

| Junction node | Coordinate (longitude, latitude) | Topological role and source |
| --- | --- | --- |
| 4395916411 | (3.3541254, 6.5359808) | Palm / Olorunsogo; node v5, 2026-04-29T05:30:18Z. https://api.openstreetmap.org/api/0.6/node/4395916411 |
| 4395534666 | (3.3547057, 6.5356184) | Palm / unnamed way 441859323; node v4, 2026-04-29T05:30:18Z. https://api.openstreetmap.org/api/0.6/node/4395534666 |
| 4395534667 | (3.3540374, 6.5343773) | Unnamed ways 441859323 / 441859324; retrieved in both full extracts |
| 4395534671 | (3.3534092, 6.5347493) | Unnamed way 441859324 / Olorunsogo; retrieved in both full extracts |
| 4395916410 | (3.3556036, 6.5383070) | Olorunsogo / Sokunbi, outside the narrow core; retrieved in both full extracts |

**I:** A shared-node circuit exists: Palm 4395916411 -> 4395534666, unnamed way 441859323 -> 4395534667, unnamed way 441859324 -> 4395534671, Olorunsogo -> 4395916411. The latter three legs are not contained in a 150-300 m real-world loop; no compressed authored loop length below is claimed as its geographic length. The unnamed way also continues toward Sokunbi at 4395534662. It must not be named Amu, Buhari or Fafolu merely because a directory or nearby clinic mentions those names.

**F, L-MH:** The local directory lists furniture accessories off Olorunsogo on Amu Street and furniture retail off Olorunsogo on Buhari Street. **I:** This supplies a more specific activity reference than a generic market theme. It does not locate those listed firms on the selected Palm block or verify their current operation. Their names stay in research metadata, not game signage. **G:** No locally confirmed workshop interior, named tenant, roof material, building height, street width or land-use inventory is available for the core.

### Authored walking-height sequence: 270 m

**P:** We retain Palm as the active frontage and Olorunsogo plus two unnamed branches as a navigable residential circuit. We shorten the reference circuit as fiction, keep normal-sized compounds and rooms, and make the quieter rear leg substantial. Chainage below is authored walking distance only.

| Chainage | Walking sequence and ordinary destination |
| --- | --- |
| 0-45 m | Enter the Palm/Olorunsogo-inspired junction. One unnamed furniture-fittings counter displays handles and hinges; an adjacent food counter has a shaded eating recess. A person collects a small parcel while a resident walks past without trading. No terminal forecourt or compulsory market crowd. |
| 45-90 m | Continue along Palm-inspired frontage beside an upholstery/repair room and its loading recess. Finished chairs, work in progress and an orderly domestic entrance share the block. Trade activity is authored from the Olorunsogo-area reference, not a replica of a listed business. |
| 90-130 m | Turn into the unnamed-side-street-inspired branch. Pass a rental compound with a shared yard, then a recently maintained apartment entrance with its own gate and modest planting. An open food shop and a closed services office punctuate the housing. |
| 130-175 m | Follow the unnamed-cross-street-inspired rear leg. A cobbler-sized repair counter, laundry yard and quiet household wall create destinations and pauses. An invited courtyard visit offers a seat and conversation but no required shortcut. |
| 175-225 m | Return via Olorunsogo-inspired frontage. Pass a small professional-services room above a grocery and a workshop service yard set back from the walking line. A worker sweeps only their work area; residents are not spawned as generic sanitation scenery. |
| 225-270 m | Rejoin Palm at the food-counter recess. The fittings counter, different gate silhouettes and open branch sightlines establish direction. The circuit stays usable in both directions and offers an independent outward continuation. |

**P dimensions:** Palm-inspired corridor 11 m overall: 5 m carriageway, two 2 m pedestrian strips, two 1 m service/frontage buffers. Olorunsogo-inspired branch carriageway 4.5 m with a 1.8 m clear pedestrian route. Unnamed public rear connection 2.4 m clear; compound yard 10 by 12 m; workshop 5 by 7 m; loading recess 3 by 6 m; grocery/services bays 3.5 by 5 m. One- to three-storey authored modules use 3 m floor-to-floor spacing. No numbers describe real geometry, household wealth or building age.

### Interiors, courtyards and specific local activity

**P:** The fittings counter stores hinges, pulls and furniture fasteners in labeled compartments; the upholstery/repair room has fabric, foam pieces, a hand-tool bench and a separate finished-item pickup position. Commission local trade review before culturally specific tool brands, methods or dialogue are finalized. Do not portray every storefront as noisy fabrication: include a grocery, professional office and quiet dwellings. The upstairs office also provides ground-floor consultations/handoffs; public participation never requires stairs. A furniture delivery goes to a household as well as a commercial customer.

**P:** The compound offers separate household doors, a shared wash yard, cooking-storage niches and a covered sitting edge. The maintained apartment entrance shares the same street and has a repair pending too. A less expensive room can be cared for, furnished and technologically connected; a more expensive dwelling can be crowded or weathered. No simulated income distribution is asserted. A courtyard is inhabited space, not loot storage or a default escape corridor.

**P:** Morning includes household departures, a food-counter opening and a small supply delivery; midday alternates work bursts with customer discussion and domestic quiet; evening includes parcel collection, returning residents and selectively closed shutters. Roles vary across gender, age and mobility. Avoid constant hammering, incessant bargaining or presenting children as a workforce. The clinic record is not turned into an emergency mission or a reproduced private interior.

### Developer Asset Checklist

| Category | Required content and behavior; all P |
| --- | --- |
| Meshes / architecture | Fittings counter with compartmented stock; upholstery workroom and pickup shelf; separate domestic entrances; rental-yard room modules; maintained apartment gate; grocery; services stair entrance; small repair counter; shaded food recess; loading pocket; protected drain covers; laundry yard; two distinct unnamed branch mouths. No giant anonymous factory or Oshodi terminal kit. |
| Materials / textures / shaders | Authored render/repaint variations; timber furniture grain; plain metal fittings; fabric/foam surfaces; matte gates; yard concrete; compacted-surface and patched-paving alternatives pending ground evidence; selective wetness/roughness and occupied-window emission variants. `surface=unpaved` does not justify blanket mud. No sourced architectural era, universal rust treatment or mandatory distress palette. |
| Systems / spawners | Furniture assembly/repair poses reviewed for safe practice; stock sorting; finite loading reservation; household arrivals; invited-yard occupancy; grocery and office schedules; pedestrian yield; short queues outside clear routes; event exclusion volumes; rain-cover behavior; quiet-period and indoor-audio states. No actual firm identity, customer-data reproduction or permanent crowd congestion. |

### Audio, lighting, weather and accessibility

**P:** Spatialize a short hand-tool tap, fabric movement, fittings drawer, rolling cart and conversations at the relevant shop only. Place quieter room/yard beds behind gates and walls. No undifferentiated industrial roar; no association between a language and a trade or income tier. Use commissioned and reviewed voice performances and original incidental music, if any.

**P:** Day lighting reveals workshop depth under shade while keeping the opposite residential frontage occupied. At night, leave some domestic windows and food frontage lit while other shops close; navigation remains legible. Rain produces distinct canopy drips and covered-yard responses. Material wetness need not disable all routes. Drain geometry, outfall, levels and rainfall records are unknown; any puddle outline, runoff direction or dry-season dust state is a simulation choice, not a flood-risk map.

**P:** Maintain 1.8 m accessible walking continuity and 1.5 m turning clearances at destinations and corners; cover mandatory drain crossings, keep stock and loading out of the protected strip and avoid mandatory stairs. Ramp target 1:20 or gentler where needed, without claiming regulatory compliance. Rear-leg navigation has visible landmarks and textual/shape cues in addition to sound. Any event alternative must serve wheelchair users and pedestrians carrying loads, not only agile avatars.

### Fictional disruption: finished-sofa loading crosses a branch mouth

**P:** At chainage 80-110 m, an unnamed workshop moves a finished sofa on a trolley into its recessed delivery space. A brief protected exclusion zone interrupts the direct entry into the unnamed branch. This is fictional ordinary logistics, not a current incident or claim about a real trader.

- Exit A: remain on the protected Palm walking line and continue outward; the event never occupies the full corridor.
- Exit B: enter Olorunsogo from the starting junction and take the rear circuit in reverse, reaching the residential branch beyond the loading zone.
- Exit C: withdraw along Palm to the incoming connector; its walking route remains independent of the branch blocker.
- Optional yard access remains invitation-only. No climbing over the sofa, payment, threat, policing or unsafe road sprint is required.
- Clear once the trolley and sofa are wholly inside the loading recess and barriers are withdrawn. If the delivery state stalls, a bounded design fallback returns goods to the workshop and releases the route. Activate only while both public circuit directions and an outward connector remain reachable. Keep the real clinic's unresolved entrance out of event placement.

### Connector handoff

| Connector | Source and destination references / supported relationship | Authored join, movement and continuity |
| --- | --- | --- |
| Incoming optional composite | EM-S Ajao junction 1073171129 to EM-M Palm/Olorunsogo 4395916411; this is the reciprocal endpoint of EM-S's proposed outgoing join. No verified direct street connection. | P: coordinator-owned 60-90 m fictional neighborhood transition, not a second independently positioned corridor. Omit unresearched occupied mainland fabric explicitly, never empty land. Preserve 1.8 m public walking continuation, optional 6 m local carriageway and service access; traffic mode subject to future checks. Keep separate fictional catchments and acoustic fade. No elevations, bridge or shoreline assertions. |
| Outgoing local residential circuit | Palm 4395534666 -> unnamed 441859323 -> 4395534667 -> unnamed 441859324 -> 4395534671 -> Olorunsogo: shared nodes confirmed. | P: keep a continuous public pedestrian circuit with 2.4 m authored rear passage clearance and 1.8 m branch clear routes. Vehicle suitability, surface condition and access restrictions unknown. Real circuit distance is not replaced by the 270 m authored sequence in the evidence layer. Preserve gates, service pockets and separate roof runoff; unknown drain junctions need later review. |
| Outgoing extended street reference | Olorunsogo 441897768 -> Sokunbi 11102602 at node 4395916410; outside the core. Unnamed way 441859323 also shares Sokunbi node 4395534662. | P: offer an unplaced walking continuation with 1.8 m clear route and quiet residential frontage between busier pockets. Any compression omits inhabited side streets and services; no terminal, express-road shortcut or named destination is assigned. Local vehicles conditional on access review; no assumption that either node is a safe major-road crossing. Grades, drainage outfall and future endpoint remain G. |

## Cultural and navigation guardrails

- Present these as two authored slices, not representative statistics for whole local government areas. Distinct class conditions, quiet homes, skilled work, leisure and professional services appear within both slices.
- Keep real names only where the map/listing actually supplies them. Unnamed OSM streets stay unnamed in evidence; fictional businesses have category labels and fictional status in production metadata, without patronizing on-screen disclaimers.
- Do not derive ethnicity, religion, criminality, accent, migration status or behavior from a street name. A mapped church or clinic is not permission to recreate private spaces, congregations, patients or staff.
- Local review is required for dialogue, food specificity, trade methods, personal styling and culturally specific props. No observation claim is made from unviewed photographs or broad area guides.
- Navigation landmarks should include shade structures, corner geometry, door/gate silhouettes, sightlines and rest recesses, not only famous monuments. Keep ordinary homes and quiet stretches between activities.
- Preserve public/private distinction in routing, permission and signage. Make rejection, closing hours and event bypasses ordinary route states; do not turn an occupied compound into a mandatory public shortcut.
- Incident logic must never trap a whole slice, depend solely on sound/color, punish slower movement or force dangerous crossing. NPC placement respects accessible routes and service access.

## Handoff status and unresolved blockers

The dossier provides two concrete source slices, node-supported side-street networks, 240 m and 270 m authored walking sequences, ordinary interiors, categorized asset briefs, audio/weather/day-night choices, two bounded multi-exit incidents and reciprocal connector metadata. These are research/design deliverables, not accepted geography or implemented content.

Outstanding evidence: present street-level imagery and capture dates; exact frontage uses and access; community/trade review; current businesses; surfaces, drains, crossing provision, storeys, widths and heights; architectural dates; any survey/height datum; exact inter-slice journey; and a complete archived API extract. Local primary cultural/architectural sources were not obtained. Directory corroboration is useful but insufficient for those claims. Source licensing/attribution and recording/branding permissions need production review.

Coordinator decisions: accept or adjust slice boundaries, reconcile overlap, approve fictional compression, assign all final placements and connector destinations, and preserve canonical IDs/content. None of these gaps prevents documentation delivery; they do prevent claims of surveyed fidelity, culturally verified final assets or implementation readiness.

Only this file is authored. No build, test, lint, typecheck, server start, engine/runtime/renderer/mobile/config edit, packaging, purchase, public message, agent launch or git commit was performed. No in-game or rendered verification is claimed.
