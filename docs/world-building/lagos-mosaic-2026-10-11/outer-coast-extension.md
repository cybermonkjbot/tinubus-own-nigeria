# Outer-coast extension: an Okun-Ajah residential pocket

Research and authored design, accessed **October 11, 2026**. One additional pocket beyond the Fola Osibo reference; documentation only and pending coordinator review.

## Selection and evidence boundary

We select **the small residential loop north of Okun-Ajah Road between OSM junction nodes 1060342115 and 1060342148**, including its northern branch at node 5427154357. A separate service spur south of the main road supplies a shoreward research reference, not an approved beach entrance. This is not a resort estate, a reconstruction of identified households, or a claim that the whole community has the same income or built form.

The connected reference consists of main road way 295026390, unnamed residential ways 563090753 and 563090754, northern continuation 563304293, and shoreward service way 317161891. Preserve unnamed roads as unnamed; descriptive labels such as "west arm" are document labels, not invented Lagos street names.

Evidence classes:

- **F - source fact:** explicitly reported by the named source. An OSM fact means the database records it, not that today's ground condition is verified.
- **T - topology inference:** inferred from inspected shared nodes or geographic geometry, not proof of access, occupancy, drainage or appearance.
- **P - authored proposal:** fictional destinations, people, routines, event, measurements and simulation. All numeric gameplay dimensions and timings below are P design values.
- **U - unknown:** missing, unverified, dated, conflicting or insufficient evidence.

**Access decision:** no current public pedestrian route from this pocket to an open beach was established. We do not extend a mapped road across the unmapped gap, route through a resort, invent an accessible boardwalk, or promise beach entry. The authored walk stays within the residential street network. Shoreward travel remains a conditional research lead, not a playable public connector.

## Repository context and preservation

Read root `AGENTS.md`, coordinator-owned `README.md`, and the complete previous `island-and-coast.md`. Also read relevant assembly-plan passages concerning repeated activities/events and the M08 coastal-access limitation. Those documents remain read-only. The coordinator's first-round numbering, assembly plan and pending second-round assignments are not changed here.

No final engine positions, height datum, map envelope or new canonical IDs are assigned. Existing theatre, districts, maps, destinations and runtime content remain unchanged. Geographic evidence is **WGS84 `(longitude, latitude)` in degrees, no altitude**; XML uses separate lat/lon attributes. API bbox order is `(west longitude, south latitude, east longitude, north latitude)`. Coordinate decimals are storage precision, not survey accuracy. Eventual source metres remain `x east, y up, z south`; `UE(X,Y,Z) = 100 * source(z,x,y)` once, after coordinator registration. Do not treat degrees or an OSM layer tag as metre-space coordinates or elevations.

## Fresh source register

Every URL below was accessed October 11, 2026. URLs identify evidence, not production assets or access permissions. Text and XML were read; **no photographic pixels, street panoramas, aerial images or video frames were visually inspected**.

| ID | Exact URL | Date/version and supported evidence | Limits |
| --- | --- | --- | --- |
| O1 | https://api.openstreetmap.org/api/0.6/map?bbox=3.596,6.4255,3.598,6.4283 | Fresh compact XML extract: main road, west/east residential arms, north branch and service spur. Selected versions below. | Whole intersecting ways/nodes can extend beyond bbox. Not a parcel survey or a clipped land mask. |
| O2 | https://api.openstreetmap.org/api/0.6/map?bbox=3.590,6.419,3.602,6.432 | Fresh neighborhood context: Okun-Ajah Community Road, named western streets, multiple unnamed residential branches and coastline way. | Reference discovery window, not an authored district boundary. No current surface/tenure inference. |
| O3 | https://api.openstreetmap.org/api/0.6/way/563090753/full | West residential arm v1, February 21, 2018, 18:00:55 UTC. | Way edit date is not node edit date, photograph date or construction date. |
| O4 | https://api.openstreetmap.org/api/0.6/way/563090754/full | East/curving residential arm v1, February 21, 2018, 18:00:55 UTC. | Shared-node evidence only; current pedestrian/public status unverified. |
| O5 | https://api.openstreetmap.org/api/0.6/way/563304293/full | Northern branch v2, May 11, 2023, 02:05:55 UTC. | Continued network, not an established shore route. |
| O6 | https://api.openstreetmap.org/api/0.6/way/311463959/full | Okun-Ajah Community Road v18, September 10, 2026, 13:21:51 UTC. | Database records `surface=dirt`; no current appearance inferred. |
| O7 | https://api.openstreetmap.org/api/0.6/way/639915070/full | Western Okun-Ajah Road v6, September 10, 2026, 13:21:51 UTC; joins way 295026390 at node 3171132168. | Similar naming does not justify merging this into the separate Lagos-Calabar highway. |
| O8 | https://api.openstreetmap.org/api/0.6/way/13762125/full | Coastline v47, May 2, 2026, 14:01:57 UTC. Local node edits include April 16, 2024. | Coastline line is not high/low tide, usable beach extent, legal boundary, bathymetry or evidence of current access. |
| O9 | https://api.openstreetmap.org/api/0.6/map?bbox=3.593,6.419,3.602,6.435 | Supplemental context shows separately mapped Lagos-Calabar Coastal Highway north of the selected loop. | Map geometry does not certify road opening, crossing permissions or construction completion. Do not place the highway between this loop and the shore simply because its name includes "coastal". |
| S1 | https://lagosschoolsonline.com/single-school.php?id=706 | Undated school record: OKUN-AJA NURSERY & PRIMARY SCHOOL, Okun Ajah; school account situates it along Okun Mopo Road near Adekanbi Compound. | Portal is operated by Global Education Media and lists government partners; treat as a local school record, not an independently current inspection. Founding date is not a building date. Enrollment, staff names and approximate site dimensions are not used as present facts. |
| S2 | https://lagosschoolsonline.com/single-school.php?id=108666 | Undated record: OKUN-AJAH COMMUNITY SENIOR SECONDARY SCHOOL; address 5 Baale Olukosi Street, Okun Ajah. | No geolocated school entrance established in our pocket; do not name an unnamed OSM branch after this address. Listed plans/support requests do not establish completed works or current roof condition. |
| S3 | https://westsidebeach.com/mopo-okun-ajah-road-improvement/ | Resort operator's September 9, 2026 update reports road work toward its 34 Mopo Akindele Road address and states its photographs were recorded September 3, 2026. It describes the corridor as serving communities, homes, businesses and leisure destinations. | Primary for the operator's own account, not independent street survey. Only text was inspected. Claimed image date is recorded, but no visual material claim follows. No public beach entitlement, mapped parcel alignment, road ownership or project completion established. |
| S4 | https://www.mecoyprojects.com/project-details.html?id=19 | Direct response returned a loading shell, not usable project detail. | Not adopted as substantive evidence of fishing, buildings or a shore-access route. |
| L1 | https://www.openstreetmap.org/copyright | OSM copyright/license text freshly read. | OSM database is ODbL; contributor attribution and license identification required. Separate image/brand/access rights remain unresolved. |

**Reproducibility:** O1 response text re-encoded as UTF-8 has SHA-256 `7e27f106f8f5536af58f6bff846b6c64c873ee69617d62368a4d952a2a87ab95`. This is a text digest, not an assertion of original transport-byte identity. XML was inspected in memory; no raw files were saved under the exclusive-output restriction. Current endpoints are mutable. Selected way/node versions and source dates below identify the reviewed objects but do not substitute for a complete future licensed snapshot.

**Rights:** OpenStreetMap contributor data licensing does not license Bing imagery named in mapping tags. No Bing capture dates were obtained. S1/S2 footer identifies copyright; S3 reserves rights. No unrestricted photo, music, logo, household-plan or voice rights were established. Use original assets later or obtain permission, and resolve applicable ODbL derivative-database/produced-work obligations before distribution. No resort signage, school branding, identifiable children or private compound imagery is authorized here.

## Source anchors and connected geography

### Selected street graph

**F O1-O7:** the road names, classifications, versions and member-node sequences below are present in the inspected database. **T:** their shared nodes support a compact residential circuit with a separate northern branch and a southward service spur.

| Role | Object/version; UTC way edit date | Relevant membership |
| --- | --- | --- |
| Main frontage | Okun-Ajah Road, way 295026390 v56; June 1, 2026, 16:23:11 | Recorded `secondary`, `source=bing`, `surface=unpaved`. Local west-to-east selected anchors include 1060342115, 3234013273 and 1060342148. These tags do not establish the present road section or appearance. |
| West residential arm | way 563090753 v1; February 21, 2018, 18:00:55 | Ordered nodes 5427154357 -> 5427154356 -> 1060342115. |
| Curving east arm | way 563090754 v1; February 21, 2018, 18:00:55 | 5427154357 -> 5427154367 -> 5427154366 -> 5427154365 -> 5427154364 -> 5427154363 -> 5427154362 -> 5427154361 -> 5427154360 -> 5427154359 -> 5427154358 -> 1060342148. |
| Northern continuation | way 563304293 v2; May 11, 2023, 02:05:55 | 5427154357 -> 5427216970 -> 5427216815. Not a dead-end courtyard by assumption; its wider continuations are outside the showcase walk. |
| Shoreward service spur | way 317161891 v3; May 11, 2023, 02:05:55 | 3234013273 -> 10885554619 -> 3234013274. Source class is `service`; no established public right of passage. A further service branch, way 1171457560 v1, shares node 10885554619 in O2; no shore outlet proved. |
| Incoming regional road context | way 311463959 v18, Okun-Ajah Community Road; September 10, 2026, 13:21:51 | Terminates at node 3171132168 shared with Okun-Ajah Road ways 639915070 and 295026390. |

We use the candidate circuit **main road -> west arm -> curving east arm -> main road**. This is **T**, not a confirmed drivable loop or a measured accessible route. Missing `access`, `oneway`, gates or footway tags do not prove unrestricted use. The main road is not renamed "Mopo Akindele", "Baale Olukosi" or "Coaster Road" on the strength of another source's address.

### Geographic anchors, longitude first

| Anchor | WGS84 `(longitude, latitude)` | Node version and UTC edit date |
| --- | --- | --- |
| Main/west-arm node 1060342115 | `(3.5965525, 6.4265625)` | v4, June 25, 2024, 12:30:30 |
| Main/east-arm node 1060342148 | `(3.5977897, 6.4265884)` | v5, June 25, 2024, 12:30:30 |
| Residential fork node 5427154357 | `(3.5966251, 6.4274555)` | v1, February 21, 2018, 18:00:53 |
| Northern continuation node 5427216815 | `(3.5967774, 6.4281147)` | v2, May 11, 2023, 02:05:55 |
| Main/service node 3234013273 | `(3.5969297, 6.4265598)` | v4, June 25, 2024, 12:30:30 |
| Service-fork node 10885554619 | `(3.5969509, 6.4262228)` | v1, May 11, 2023, 02:05:55 |
| Service-spur end node 3234013274 | `(3.5969715, 6.4258612)` | v2, February 21, 2018, 18:00:57 |
| Community/main road node 3171132168 | `(3.5927476, 6.4265656)` | v4, June 25, 2024, 12:30:30 |
| Coastline reference node 3246603759 | `(3.5962026, 6.4243556)` | v2, April 16, 2024, 13:17:33 |
| Next coastline node east, 3246603760 | `(3.6022513, 6.4243930)` | v2, April 16, 2024, 13:17:33 |

**T shore relationship:** the inspected service spur ends north of the mapped coastline. It neither shares a coastline node nor supplies an inspected connecting pedestrian way to it. The gap contains unverified intervening ground/property; it is not empty land to bridge with a guessed path. The coastline reference is a boundary line, not a destination entrance.

**T highway separation:** O9 places selected highway segment nodes north of the pocket: way 1479655587 v2 includes node 13057330734 at `(3.5954038, 6.4327241)`; opposite segment way 1479655588 v2 includes node 14125780492 at `(3.5963005, 6.4329860)`. Both ways were edited August 26, 2026, 18:58:16 UTC. These are context anchors only. No highway interchange, public crossing, deck height or walking connector is assigned.

### Local activity evidence, with limits

**F S1/S2:** local school records support education as a community activity and provide specific address leads. Neither record locates a school in the selected loop. Do not move a real school into the walk or copy a founding date onto a proposed building. No pupils' identities, staffing or enrollment counts are used.

**F S3:** the operator reports continuing work on its approach route as of September 2026 and identifies residential/business uses along the corridor. That is evidence against treating the periphery as only resorts, not a census of this block or proof of the currently visible road surface. The exact alignment of its named route against our OSM streets is **U**.

**U:** no current facade sequence, household/tenure survey, measured road section, legal beach-access record, tree inventory, licensed interior reference, present utilities, traffic counts or onsite sound recording. Fishing, boat yards and fish-smoking practices were not established for this exact pocket; they are not added merely because it is coastal. Current maintenance and appearance cannot be inferred from `residential`, `unpaved`, `dirt`, `building=yes` or an edit timestamp.

## Authored 280 m walk

**P throughout this section.** We follow the west-arm/curve/east-arm logic, keeping domestic lots full-sized. The cumulative values below are authored walking distances, not a survey or measurement of OSM segments. The sequence ends on the east return arm; the remaining main-road junction lies beyond the showcase endpoint. We do not force the complete circuit, school or shoreline into 280 m.

| Cumulative distance | Walking-height sequence and ordinary destination |
| --- | --- |
| 0-35 m | Start just inside the west-arm mouth off Okun-Ajah Road. A plain neighborhood cooked-food counter opens into a recessed forecourt; a resident collects a meal while another continues home. Main-road sound remains behind the turn. A separate pedestrian strip avoids the cooking/work area. |
| 35-85 m | Pass two domestic frontages: a freshly repainted modest flat compound and a larger home with a repaired wall patch. One open workroom has an upholstery cutting table and stacked foam offcuts; fabric and tools stay inside the setback. This is an unnamed fictional business, not an identified real operator. |
| 85-120 m | Reach the residential fork. The northern branch remains visible as continued neighborhood street, not an empty horizon or a mandatory school path. A shaded public rest pocket and differently shaped gates make the east turn readable. |
| 120-185 m | Follow the curving arm east past a shared courtyard compound and a separate single-household yard. Laundry, bicycle storage, a covered washing corner and a small home workspace are visible only through invited/open entrances; private rooms remain occluded. An adult sits reading while a worker checks an order list. |
| 185-230 m | A cabinet maker's collection counter sits beside a small household provisions shelf, with a quiet gap between them. Furniture sample pieces are displayed in the forecourt, not on every walking surface. Tool use alternates with measuring, assembly and conversation. |
| 230-280 m | Turn along the east return toward the main road. Pass a plain household gate, a shared sitting ledge and a flexible neighborhood meeting room. End at a public rest pocket on the return arm, with the road continuation legible. Shoreward travel is not offered here as a beach destination. |

### Proposed spatial values

**P:** residential carriageway 5.5 m with a continuous 2.0 m firm, step-free pedestrian strip; opposite strip 1.8 m where authored. Main frontage section proposal 7.0 m carriageway with a 2.0 m walking continuation, not a measured Lagos width. Minimum clear pedestrian headroom 2.4 m; conditional vehicle overhead clearance 4.5 m if a future canopy crosses a permitted vehicle route. Corner rest/turning pockets 3 x 4 m; shop fronts 7-12 m; household fronts 9-18 m; upholstery room 5 x 7 m; cabinetry room 6 x 8 m; meeting room 7 x 9 m; shared courtyard 11 x 13 m; washing alcove 2.5 x 3 m. Proposed shells are one or two storeys, with no real height/date claim.

These values include room to pass a seated person, remain outside cooking/tool zones and turn without entering a private yard. Use proposed 1.5 x 1.5 m turning spaces or larger at usable destinations. Threshold treatment and gradients must be designed to a later chosen accessibility standard; this is not certification of legal compliance or the real street. No pavement, ramp, bridge or elevation is inferred from the map.

### Homes, courtyards and work

**P:** domestic interiors have cupboards, sitting/dining furniture, study/work surfaces and varied cooking/washing arrangements. A shared compound offers separate household doors and a communal washing corner, not a single undifferentiated "poor household". Another modest home has well-maintained fittings; a roomier house has an unfinished maintenance task. Vary upkeep, tenancy proposals and fit-out independently of footprint size, gender, occupation or clothing.

**P:** the upholstery room supports an order collection or fabric-selection interaction using original samples; the cabinet counter supports collecting a small commissioned household item. The food counter is an ordinary meal destination, with menu/food specifics withheld until local review. The meeting room allows an optional appointment or neighborhood gathering; it is not an invented real institution. A home visit requires an invitation, and cancelling the interaction leaves a clear exit. No private yard becomes an emergency through-route.

**P routines:** mornings mix household departures, food preparation and measuring/cutting work; midday includes quiet fitting/assembly and people at home; evening moves to meal collection, sitting and finishing small orders. A separate school-journey population may travel along a future verified community route, but no real dismissal time, uniform, pupil identity or school entrance is invented. Show workers' rest and family/social life as well as work. People of varied genders, ages and abilities use each setting without encoding wealth or hostility from appearance.

## Developer Asset Checklist

All items are **P future asset requirements**, not produced meshes, systems or accepted reference matches.

| Category | Checklist |
| --- | --- |
| Meshes / architecture: street and access | Main/side-street junction modules; independent footway/road surfaces; corner rest pockets; covered flush drain crossings; low-speed protected crossing modules; readable curving walls; distinct pedestrian gate leaves; screened utility enclosure. Keep source-shore gap unconnected. |
| Meshes / architecture: households | One/two-storey plain compound and modest-flat variants; separate household entrances; privacy walls; shared courtyard; washing alcove; drying racks; kitchen/storage modules; small home workspace; chairs, tables and a bicycle. Avoid resort pools and identical prestige gates as the default. |
| Meshes / architecture: work and activity | Unbranded meal counter; indoor preparation/safe-storage modules; upholstery cutting table and enclosed tool cabinet; original fabric samples/foam offcuts; cabinetry bench and collection shelf; compact meeting room; movable furniture display plinths and rest seats. No real school or resort replica. |
| Materials / textures | Original painted render in maintained, patched and partially renewed states; original firm pedestrian paving option; road finish alternatives held open pending imagery; painted metal/timber furniture; plain textiles; plastics and glazed household surfaces. Do not derive present sand, rust, roof type or surface color from tags. |
| Shaders / environmental states | Independent cleaning, contact-wear, repair and wetness masks per lot; restrained puddle/reflection treatment; privacy glazing/occlusion; fabric movement options; localized dust only where authored. No universal poverty/grime or coastal salt-damage overlay. |
| Dynamic systems / spawners: routine | Destination-based pedestrians; work/rest cycles; meal and furniture collection; outdoor sitting and optional home invitations; bounded indoor/outdoor queues; pedestrian-strip exclusion volumes; household permissions; independently scheduled lights/shutters. No crowd fill applied to the whole neighborhood. |
| Dynamic systems / spawners: event and navigation | Movable showcase footprint; optional event participation; protected bypass assignment; accessible alternate routing; retraction/clearance state; captionable route cues; event attendance caps; no spawn on the alternate or both connector mouths. |
| Audio assets | Original commissioned local speech after review; measuring/cutting/assembly foley; food utensils; gate latches; low domestic talk; layered road ambience. No sampled resort promotional music, generic shouted market bed or identifiable children's recording. |

## Audio, day-night and weather

**P spatial audio:** main-road movement remains audible near both mouths, then attenuates behind frontage walls. Use short measuring/cutting bursts, drawer/wood contact, cloth handling and utensils at the respective workrooms; tool sounds do not play continuously. Courtyard conversation is low and occluded from private rooms. A broader road sound may occupy northern context only after future spatial review; it is not proof of a highway acoustic level.

**U coast acoustics:** no onsite recording establishes audible surf, birds or breeze in this pocket. An optional distant sea bed must remain a reviewed design choice, not an authenticity claim. Do not use sea sound as a navigation promise to an unavailable beach. Spoken language, accent and idiom require local performers/review; no invented cultural catchphrases.

**P lighting:** mornings expose workroom entries and shaded yards; midday emphasizes canopy/wall shade; dusk alternates open food/collection counters, occupied domestic windows and unlit closed rooms. The route uses visible gates, corners and redundant shape/text cues rather than color or hearing alone. Night navigation does not require a claim that real streetlights exist or a blanket unsafe-street portrayal.

**P rain and seasonal behavior:** roof/forecourt runoff goes to separately designed gutters and storage/catch points, with firm covered crossings. Wet weather moves sample furniture indoors and residents toward shelter without blocking the pedestrian strip. Drier/hazier presets vary distant contrast and shaded sitting choices. No real rainfall quantity, tide, groundwater, erosion rate, drain capacity, water depth or seasonal calendar was obtained. Do not create a flood incident or make ordinary rain a crisis. Terrain gradients, runoff/outfall continuity and shore elevation remain U; do not infer sea-level ground from coastal proximity.

## Fictional event: neighborhood furniture showcase

**P:** two unnamed local workrooms jointly hold a short, voluntary display of small chairs, stools and household storage pieces in a recessed forecourt on the curving residential arm. Neighbors arrive and compare sample pieces; the familiar collection counter temporarily moves inside the meeting room. Increased stopping changes the preferred path without a vehicle failure, delivery obstruction, flood, access dispute or compulsory celebration.

The event is invented, not a documented Okun-Ajah tradition. No named organizer, real performance, song or culturally specific ceremony is reproduced. Display footprint **P 4 x 7 m**, entirely in the authored setback; a bounded waiting pocket may occupy one optional frontage spur. Tool work pauses during the gathering, so the event has social presence rather than an amplified noise blanket.

### Multiple exits and credible choices

- **P A, short through-route:** use the continuous opposite 2.0 m residential strip via an authored protected, low-speed crossing. It remains firm, step-free and unoccupied by seats, queues or equipment. No requirement to negotiate entry or listen for directions.
- **T/P B, public circuit candidate:** continue down the east arm to node 1060342148, along the main-road continuation and back through the west mouth at node 1060342115. The underlying circuit is T; public permission, crossings and accessible surfaces are P requirements pending real verification. It is longer than the showcase walk and not reduced to a guessed alley shortcut.
- **P C, decline and retreat:** turn back along the west arm to the incoming connector, or remain at the fork rest pocket. The northern branch remains an optional continuation only if later access review supports it. No private courtyard, resort, school or shore spur is needed.

**P clearing condition:** organizers move sample pieces and seats back into the workrooms, release the waiting pocket and return the collection counter to its usual location. Proposed active session 10-15 minutes of authored world time, ending when the packing tasks finish; players can leave immediately. An optional furniture interaction can be declined. If packing stalls, the protected route remains clear and the temporary collection location remains available, rather than locking a destination or district. No event attendee spawns across both circuit exits.

## Connector briefs for the coordinator

| Port | Source/destination references and supported relationship | Movement, access and proposed join |
| --- | --- | --- |
| Incoming composite extension from Fola Osibo | Source: earlier Fola/Grace Anjos reference node 1513601085, ways 138040470/138040483. Destination: Okun-Ajah west-arm mouth node 1060342115 on ways 295026390/563090753. **No direct real adjacency or complete itinerary established.** Omitted geography includes eastern Lekki and intervening developed residential/access corridors; it is not an empty coastal strip. | **P:** one montage/travel transition or full-scale transition street selected by the coordinator; 2.0 m step-free pedestrian continuation and conditional local vehicle envelope, proposed 7.0 m roadway/4.5 m overhead clearance if needed. No expressway walking, named-road invention, new sea gap or assumed estate passage. Reduce main-frontage density/sound through the transition. Keep catchments independent until heights/outfalls are reviewed. Earlier Fola port is an offer, not silently amended. |
| Outgoing local main-road continuation | Source: east-arm way 563090754. Destination: Okun-Ajah Road way 295026390 at shared node 1060342148. **T mapped local connection.** The main road beyond the selected mouths is outside the walk, not removed urban life. | **P:** continuous 2.0 m walking strip and protected return crossing; locally permitted vehicles only after access/direction review. Proposed 7.0 m road section and conditional 4.5 m overhead clearance; no mapped elevation claim. Continue traffic sound and local gutters only under later slope/outfall design. No required private entry. |
| Optional northern neighborhood branch | Source: residential fork node 5427154357. Destination: north branch way 563304293 toward node 5427216815. **T direct connection.** Beyond that branch, additional residential/service geometry exists but is outside this compact walk. | **P conditional:** preserve a legible continuation without assigning a real school or shore destination. Proposed 2.0 m step-free walking strip and 5.5 m local roadway; gates/public access U. Carry domestic/road sound gradually; do not connect drains to an invented highway culvert. No deck or shoreline interface selected. |
| Shoreward evidence port, deferred rather than playable | Source: main-road node 3234013273. Destination: service way 317161891 ending at node 3234013274; coastline way 13762125 is a separate reference, not its connected endpoint. **T spur exists; U final shore passage/public access.** Intervening property/ground and coastal edge remain unverified. | **No approved pedestrian, vehicle or beach join.** Do not add clearance/boardwalk dimensions that imply a real passage. Future field/access evidence must establish permission, firm-route choice, shoreline/tide geometry, height/datum and safe return before any route can be authored. Resort admission does not satisfy this requirement. Drain/sea and acoustic continuity unassigned. |

No Third Mainland or Lekki-Ikoyi bridge endpoint relationship is inferred from this peripheral pocket. The mapped Lagos-Calabar highway and local Okun-Ajah Road remain distinct references; no interchange is invented.

## Accessibility, privacy and cultural constraints

**P:** retain full-size homes, ordinary side-street depth and quiet gaps. Public destinations must have a firm step-free route, rest/turning space and a clear exit even during the event. Soft sand, drain edges, private entrances, stairs, live highway shoulders and hearing-only cues are not compulsory alternatives. Accessibility targets are authored requirements, not claims about present infrastructure.

**P:** screen household rooms and work hazards; visitors may see an open courtyard only under that owner's invitation state. School records support education, not recording/recreating identifiable minors or opening a school campus as a shortcut. Do not disclose fictional patient, household or pupil records as scenery.

**P:** portray competent residents and workers with leisure, choice and varied maintenance. Do not classify compounds as a slum/luxury binary, use coastal fishing stereotypes without evidence, or assign ethnicity/religion from names. Uniforms, language, food specifics, fabric motifs and cultural artwork require Lagos-based review and original/cleared assets. No resort branding or real school identity is needed for the fictional destinations.

## Unresolved evidence and delivery

| Gap | Consequence and next evidence needed |
| --- | --- |
| Current public beach access | Shore port remains deferred; obtain geolocated entrance/path, access authority and contemporary return-route evidence. A tourism listing or ocean-facing address is insufficient. |
| Present visual appearance | No imagery acceptance: obtain dated, geolocated, permission-compatible street-height references for this exact loop. S3's September 3, 2026 image date is text-only provenance, not inspected evidence. |
| Widths, elevations, shoreline and drainage | All section/room dimensions are P; no survey datum, current shore profile, tide, water depth or drain outlet. Do not turn OSM precision into accuracy. |
| Current public access and accessible circuit | Verify gates, walking surfaces, main-road crossings and northern continuation; missing OSM restrictions are not permission. Protected authored alternatives remain a design prerequisite. |
| Current households, income, livelihoods and businesses | Workrooms and residential mix are fictional. School/operator sources supply community context but not a census or exact parcel uses. No real cabinetry/upholstery operator invented. |
| Road-name reconciliation and completion | Okun Mopo/Mopo Akindele/Coaster/Baale Olukosi address leads are not reconciled with every OSM way. September road-work report does not certify completion or today's surface. |
| Rights and archival data | No production media or interior rights cleared; response hash is not an offline extract. Later archival capture and ODbL/asset-rights review required. |
| Cross-dossier composition | Coordinator owns port reconciliation, overlap, omitted travel and placement. This extension neither modifies the previous dossier nor registers a destination. |

**Status:** one researched, conditional outer-coastal residential pocket with a 280 m authored walk, everyday work/home destinations and a distinct social-timing event. Geographic access, contemporary visual fidelity and cultural acceptance remain incomplete where stated. Only `outer-coast-extension.md` is created; no commits, tests/builds/lint/typechecks, server starts, scene generation, runtime/native/renderer/mobile/config edits, further agents, public messages or purchases.
