# Ebute Metta / Oyingbo: Freeman-Market pocket

Research and access date: October 11, 2026. One new conditional documentation pocket, not an engine district, completed asset set or surveyed reconstruction.

## Scope and evidence boundary

We select **Freeman Street at Market Street**, with **Willoughby Street, Griffith Street and Kadara Street** supplying connected side-street choices. Oyingbo Street is a larger approach outside the short walk. The pocket brings ordinary homes and small workrooms into the mosaic between the existing Yaba and old Lagos Island references; it is not a claim that those places are adjacent corners.

Read with the coordinator-owned [mosaic contract](README.md), the read-only [Yaba/Ikeja dossier](tech-and-mixed-centres.md) and [old Lagos Island dossier](old-lagos-island.md). No previous dossier, canonical ID, map, theatre, destination or placement is changed. No world envelope is imposed. The coordinator owns connector reconciliation and eventual placement.

- **F: source fact.** A source explicitly reports the statement; institutional history and service descriptions remain attributed, not independent field observations.
- **M: topology inference.** Fresh OSM ordered node references support street connections. A shared node is mapping evidence, not proof of a safe crossing, public entrance, legal vehicle movement or usable accessible pavement.
- **P: authored proposal.** Fictional premises, households, activities, dimensions, historical layering, routes and simulation. Unless separately marked F/M, all design below is P. Unnamed premises are fictional, not invented real businesses.
- **U: unknown.** Evidence is absent, stale, inaccessible or insufficient. OSM names/classifications/surface tags cannot establish current appearance, material condition, occupancy, demolition status or building dates.

Geographic pairs are **WGS84 [longitude east, latitude north] degrees**, without altitude. XML uses named `lat`/`lon` attributes; pairs below deliberately print lon first. API bbox order is **west longitude, south latitude, east longitude, north latitude**. Decimals retain source precision, not survey accuracy. Missing altitude is unknown, not zero. Geographic anchors are not engine positions. Existing metre-space remains **x east, y up, z south; UE(X,Y,Z) = 100 * source(z,x,y)**, applied once after coordinator registration. No origin, height datum, rotation or final position is assigned here.

## Fresh research and source register

Read-only HTTP requests returned OSM XML and institutional HTML. Selected ordered references, junction coordinates and text were inspected. No street-image pixels, private interiors, video or satellite imagery were inspected. No image capture date is established. OSM edit timestamps and image upload-directory dates are not photographic dates or architectural dates.

Every successfully fetched source below was accessed **October 11, 2026**. Only this Markdown is retained; selected evidence is transcribed below, not a raw XML/HTML archive. Live `/full` endpoints can change, so recorded versions/timestamps are important. Copyright OpenStreetMap contributors; OSM-derived data is available under ODbL, with attribution/licence reference S14. Institutional text/photos/logos have no open reuse permission established here. Evidence is not clearance to reproduce a building interior, photograph, logo, voice or personal record.

| Ref | Exact URL | Evidence actually read; source date | Limits |
| --- | --- | --- | --- |
| S1 | https://www.openstreetmap.org/api/0.6/map?bbox=3.378,6.470,3.392,6.485 | Successful discovery extract: named streets and full way node references; used to select the pocket. | Rectangle is neither a neighborhood boundary nor a world footprint. Selected ways extend beyond it. |
| S2 | https://www.openstreetmap.org/api/0.6/way/166893832/full | Freeman Street v7, 2024-07-02T03:41:03Z. | `highway=residential; source=Bing; surface=unpaved` are mapping tags, not current visual evidence. |
| S3 | https://www.openstreetmap.org/api/0.6/way/108050114/full | Market Street v11, 2026-05-11T03:16:08Z. | `residential; surface=unpaved`; name does not locate a market building or prove frontage use. |
| S4 | https://www.openstreetmap.org/api/0.6/way/108050115/full | Willoughby Street v11, 2026-02-03T18:11:07Z. | `residential; surface=unpaved`; no condition/width/house-date inference. |
| S5 | https://www.openstreetmap.org/api/0.6/way/108050110/full | Griffith Street v9, 2024-07-02T03:41:03Z. | `residential; source=Bing; surface=unpaved`; no visually inspected imagery. |
| S6 | https://www.openstreetmap.org/api/0.6/way/108050112/full | Kadara Street v8, 2024-06-12T12:26:06Z. | Same tag limits; public/access conditions unverified. |
| S7 | https://www.openstreetmap.org/api/0.6/way/534989419/full | Oyingbo Street v4, 2024-06-13T12:33:30Z; Freeman connection and northern Herbert Macaulay join. | `trunk; oneway=yes; ref=F266; surface=unpaved` does not establish legal pedestrian/vehicle routing today. |
| S8 | https://www.openstreetmap.org/api/0.6/way/215667104/full | Oyingbo Street v6, 2024-10-07T14:36:20Z; Griffith/Kadara and Iddo-side approach. | `foot=yes` on this segment is not a verified continuous footway or crossing. |
| S9a | https://api.openstreetmap.org/api/0.6/way/133805222/full | Herbert Macaulay Way v19, 2024-10-07T14:36:20Z. | Connects Oyingbo node 1278602338 to same-name join 1278601855. Different from nearby **Herbert Macaulay Street** way 166893830; never substitute the two. |
| S9b | https://api.openstreetmap.org/api/0.6/way/534993359/full | Herbert Macaulay Way v33, 2024-10-07T14:36:20Z; contains join 1278601855 and earlier Yaba Harvey anchor 2176500609. | `foot=yes; trunk; oneway=yes; surface=paved`; mapped chain is not an admitted accessible itinerary or vehicle route. |
| S10 | https://api.openstreetmap.org/api/0.6/way/112513501/full | Iddo Road v20, 2026-07-27T17:13:01Z; connects Market at 2270550585 and Oyingbo at 2176500527 in S1/S8. | `foot=yes; trunk; oneway=yes; surface=asphalt`; no actual widths, grades or present crossing treatment obtained. |
| S11a | https://api.openstreetmap.org/api/0.6/map?bbox=3.380,6.463,3.388,6.472 | Successful narrower Carter/Iddo discovery extract. | Bridge/road names and shared references, not deck heights, railings, sidewalk access or operating status. |
| S11b | https://api.openstreetmap.org/api/0.6/way/207402234/full | Carter Bridge v6, 2026-07-27T17:13:01Z; endpoint 1472319446 at [3.3846809,6.4675580], endpoint 1472319578 at [3.3798329,6.4690043]. | `bridge=yes; layer=1; primary; oneway=yes` are graph attributes, not a measured deck or a safe pedestrian route. |
| S11c | https://api.openstreetmap.org/api/0.6/way/87805666/full | Carter Bridge v7, 2026-07-27T17:13:01Z; endpoint 1020782226 at [3.3866429,6.4628289], endpoint 35783594 at [3.3855859,6.4656974]. | A selected bridge segment, not the entire bridge or proof of a complete route from the pocket. |
| S12a | https://api.openstreetmap.org/api/0.6/way/129535577/full | Idumagbo Avenue v6, 2024-09-04T03:28:53Z; old Island C1 node 1020782267 remains a way endpoint. | Source-supported endpoint offer, not direct Ebute Metta adjacency. |
| S12b | https://api.openstreetmap.org/api/0.6/way/264916646/full | Nnamdi Azikiwe Street v9, 2020-01-12T05:02:43Z; same C1 endpoint. | Selected southern segment does not close the route north to Carter Bridge. |
| S13a | https://www.lamata-ng.com/our-schedule/redline-train-schedule/ | Operator page lists Oyingbo and Yaba consecutively on the Red Line station sequence; page publication/update date not independently established. | Service relationship only. Do not import its timetable/counts, infer Marina through-service or equate Yaba station with University/Moore Road. No platform/entrance inspection. |
| S13b | https://www.lamata-ng.com/service/oyingbo-bus-terminal/ | Official page identifies Oyingbo Bus Terminal and labels its content construction pictures; undated page. | Does not establish present construction phase, completed access, routes or a station forecourt in the selected walk. Linked image upload directory is not a capture date. |
| S13c | https://nrc.gov.ng/ | NRC identifies its address as 1, Okeira Road, Railway Compound, Ebute-Metta, Lagos; identifies Legacy Railway Museum/Jaekel House in Ebute-Meta. Undated homepage sections read. | Institutional railway/heritage context, not a footprint or a public gate. NRC's attributed Jaekel House dating is not transferred to any selected house; no colonial structure date is admitted into this pocket. No museum visit or current appearance verified. |
| S14 | https://www.openstreetmap.org/copyright | OSM attribution and Open Database License statement read. | Credit OpenStreetMap contributors and identify ODbL. Database obligations differ from produced works; rights review needed before distributed derivatives. OSM does not license Bing imagery or institutional photos. |

A repeat of the larger S1 request and a broader request at `https://www.openstreetmap.org/api/0.6/map?bbox=3.378,6.457,3.390,6.477` did not return usable output during follow-up and were interrupted; no evidence depends on them. The initial S1 and selected `/full` calls succeeded. Search results were not sufficient to establish parcel-level workshop or household facts. S13 provides primary institutional context, not verified individual trades on Freeman. No invented demolition, reconstruction or eviction status is used.

## Retained anchors and connected street topology

Exact node URL pattern for E1-E8 is `https://www.openstreetmap.org/api/0.6/node/ID`, with ID replaced by the table's decimal ID; the eight requests succeeded on October 11, 2026. Exact node URLs are also retained below for unambiguous replay.

| Anchor | OSM node | [lon, lat] | Node version / edit UTC | Shared mapped ways |
| --- | --- | --- | --- | --- |
| E1 | 1783114075 | [3.3859825,6.4804964] | v5 / 2026-05-11T03:16:08Z | Freeman 166893832 / Market 108050114. |
| E2 | 1783114230 | [3.3863626,6.4796776] | v4 / 2018-05-14T16:59:59Z | Freeman / Willoughby 108050115. |
| E3 | 1783114104 | [3.3855794,6.4814099] | v2 / 2018-05-14T16:59:59Z | Freeman / Kadara 108050112. |
| E4 | 1240404281 | [3.3851436,6.4801220] | v4 / 2024-05-23T14:58:29Z | Market / Griffith 108050110. |
| E5 | 1240404279 | [3.3855043,6.4792538] | v3 / 2018-05-14T16:33:35Z | Griffith / Willoughby. |
| E6 | 1240404267 | [3.3846816,6.4810054] | v1 / 2011-04-10T22:45:51Z | Griffith / Kadara. |
| E7 | 1783114065 | [3.3850980,6.4823853] | v4 / 2018-05-14T16:59:59Z | Freeman / Oyingbo 534989419. |
| E8 | 1278602338 | [3.3856197,6.4835713] | v5 / 2018-05-14T16:59:58Z | Oyingbo 534989419 / Herbert Macaulay Way 133805222; Kano also appears in S1. |

- https://www.openstreetmap.org/api/0.6/node/1783114075
- https://www.openstreetmap.org/api/0.6/node/1783114230
- https://www.openstreetmap.org/api/0.6/node/1783114104
- https://www.openstreetmap.org/api/0.6/node/1240404281
- https://www.openstreetmap.org/api/0.6/node/1240404279
- https://www.openstreetmap.org/api/0.6/node/1240404267
- https://www.openstreetmap.org/api/0.6/node/1783114065
- https://www.openstreetmap.org/api/0.6/node/1278602338

**M:** The southern public-street candidate circuit is **E2 -> E1 -> E4 -> E5 -> E2**, on Freeman, Market, Griffith and Willoughby. The northern candidate circuit is **E1 -> E3 -> E6 -> E4 -> E1**, on Freeman, Kadara, Griffith and Market. Their shared Market leg is one reference segment, not two duplicated runtime streets. Public circulation is an authored admission decision; these circuits are not certified accessible or legally drivable routes.

Ordered subset sufficient to reproduce the short southern circuit: Freeman E2 `1783114230 -> 5617959407 -> 5617959408 -> 1783114075` E1; Market E4 `1240404281 -> 5617960234 -> 11928565606 -> 5617960233 -> 13816473533 -> 1783114075` E1; Griffith E5 `1240404279 -> 1240404281` E4; Willoughby E5 `1240404279 -> 5617903537 -> 5617903538 -> 1783114230` E2. Reverse ordered traversal is a pedestrian topology description, not authorization to reverse a vehicle one-way.

**M:** Market extends to Iddo Road at node **2270550585 [3.3834894,6.4798482]**. Griffith reaches Oyingbo at **1240404275 [3.3845622,6.4813520]**; Kadara reaches Oyingbo at **1240404277 [3.3842967,6.4808709]**. Coordinates/reference identities were read from successful full extracts. These longer approaches are outside the walk. Market Street's name is not evidence for the location, boundary or current condition of Oyingbo Market.

## 270 m authored human-scale walk

**P: all chainages, dimensions, facades and activity in this section.** Distance is walked design distance, not measured OSM length. The walk samples the circuit; it does not shrink either entire mapped circuit into 270 m. Maintain ordinary block depths, door spacing, people and vehicles at human scale. The last street continues beyond the authored sequence.

| Design chainage | Walking-height sequence | Proposed design values and choices |
| --- | --- | --- |
| 0-45 m | Begin at an E2-inspired Freeman/Willoughby corner. Pass a recessed family entrance and a public upholstery assessment counter. A resident brings a removable chair cushion, while another person waits in shade without being a customer. | Freeman carriageway 5.5 m, continuous firm pedestrian edge 2.0 m, queue recess 3.0 m deep. Gate leaf swings inside its property. Street mouth and counter canopy, not a heritage monument, orient the route. |
| 45-100 m | Continue Freeman toward the E1-inspired Market turn. One shared compound entrance separates stairs from a ground-floor workroom. A household member sits with schoolwork; a repairer discusses fabric samples at the public front. | Preserve 2.0 m clear circulation, 1.2 m public door clearance and an optional 1.8 by 1.8 m resting/turning recess. No required staircase or private shortcut. |
| 100-145 m | Turn onto Market-inspired frontage. A modest provisions/food counter faces the street; behind the serving room is a private domestic yard, not a passage to another road. People collect a meal, replenish household supplies or pass through. | Market carriageway 5.5 m, clear walking edge 2.0 m; food queue housed in a 3.0 m recess. A covered, flush driveway/drain crossing is authored, not attributed to real paving. |
| 145-180 m | Quieter residential interval approaching the E4-inspired Griffith turn: a maintained veranda beside a repaired frontage and an alterations front room. An office worker collects mended clothing; a resident tends plants. | Keep distinct property thresholds and setbacks; original facade bays 4-6 m wide, selected design values rather than cadastral frontages. Do not squeeze every door into a trading stall. |
| 180-220 m | Optional 40 m out-and-back into a fictional, permission-controlled alterations vestibule and its workroom waiting alcove; return to the same public street. A fitting screen protects a customer from street views. | Vestibule route 2.0 m clear, public room 4 by 5 m; proposed private courtyard 8 by 10 m visible only with invitation. No home access is needed for onward progress. If declined/closed, this optional visit is omitted and the public walk remains 230 m. |
| 220-270 m | Turn into the first 50 m of Griffith-inspired frontage. Domestic entrances and a small furniture-mending front room make a quieter finish, with the public street visibly continuing toward Willoughby. | Griffith carriageway 5.0 m with 2.0 m firm pedestrian edge. End at a public seating/collection recess, not a locked district gate. The full E4-to-E5 leg and return remain separately authored extensions, not falsely measured here. |

## Ordinary interiors, compounds and mixed history

**F, attributed:** S13c establishes an institutional railway address and a named railway museum in Ebute Metta. S13a establishes a present operator-described rail relationship with Yaba; S13b supplies an official Oyingbo terminal reference. Together they justify retaining transport/heritage context alongside present household life, not relocating the railway compound, museum or terminal onto Freeman. No verified specific workshop tenancy, resident census or historic house inventory was found for this pocket.

**P:** Public destinations are cushion/furniture repair assessment and collection, clothing alterations, food/provisions collection and an ordinary small workroom meeting. The upholstery room has a 4 by 6 m work area, sample shelf, enclosed cutting storage and a visitor boundary; the furniture repair room has a 4 by 5 m bench area and safe tool storage. A 3 by 4 m food-preparation room has washable surfaces and separate refuse storage. Dimensions are design values. Skilled repairs are substantive local purposes, not a row of technology cafes or generic poverty props.

**P:** A shared compound has separate household doors, a shaded common sitting area, washing space and a domestic cooking area behind the private boundary. Another property has a quieter family veranda and planted yard; a workroom occupies the public front of a third. An invitation can admit a courtyard visit, but the mapped street circuits remain outside homes. Resident papers, family photographs and address numbers are fictional/non-identifying. Access to religious or railway-controlled grounds is not implied.

**P:** Mixed history is authored through repair seams, replaced door/window assemblies, retained veranda proportions beside a newer simple infill frontage and changed room uses. These are fictional layers with **no calendar dates, demolition narrative, colonial-builder attribution or claims of surviving fabric**. Do not infer age from ornament, corrugated roofing, render, street names or maintenance. No property is declared vacant, condemned, demolished or redeveloped. If a future story needs a real structure's history, obtain building-specific evidence first.

**P:** Upkeep, shared facilities and working space vary between households; no real income class is assigned. People work, study, rest, visit friends and maintain homes, with genders and mobility needs independent of job or wealth. Morning emphasizes opening workrooms and household departures; midday has repairs, food collection and quieter domestic activity; evening keeps some food trade and occupied homes after other shutters close. These are authored schedules, not documented opening hours, transport demand or local language distributions.

## Developer Asset Checklist

Every item is **P**, an original/cleared asset requirement, not a created mesh or approved existing asset ID.

### Meshes and architecture

- [ ] Modular domestic/workshop facades with veranda, simple infill and repaired-opening variants; proposed 3.2 m floor-to-floor height and one-/two-upper-level options, without architectural dating.
- [ ] Recessed household entrances, inward gate swings, compound sitting/washing zones, invitation-controlled vestibule, stairs separated from public service, privacy screens and planted courtyard modules.
- [ ] Upholstery counter, removable cushions, fabric samples, cutting/storage bench; furniture-mending bench and tool storage; alterations machine, fitting screen, clothing shelf; food/provisions preparation and collection furniture.
- [ ] Firm pedestrian strips, covered drains, flush threshold/driveway crossings, seating alcoves, canopy supports outside circulation and public junction sightlines. Street geometry remains original proposed design, not a measured copy.
- [ ] Interior-facing privacy shutter and optional vestibule closure states, with no protrusion into street collision; neutral public collection counter that can remain reachable when the optional passage closes.

### Materials, shaders and textures

- [ ] Original painted render, restrained repaired-wall seams, door timber/metal options, corrugated canopy metal, textiles and washable preparation-room finishes; all selected design materials, not current observed surfaces.
- [ ] Original firm road/pedestrian surfaces. OSM `unpaved` does not select earth/gravel/sand or establish today's condition; any asphalt, mineral or paver option here stays P.
- [ ] Local wetness, canopy drip and shaded-drying responses; readable threshold contrast without reflective glare covering route cues. No flood depth, universal grime or age inferred from wear masks.
- [ ] Fictional repair labels, fabric samples, menus and course papers with no real contact details, operator marks or resident records. No photographic textures, copied colonial motifs or sacred objects taken from institutional sources.

### Dynamic systems and spawners

- [ ] Independent household, repair, alterations, food-service, visitor and passer-by routines; waiting zones remain outside the 2.0 m clear pedestrian route.
- [ ] Optional invitation/withdrawal, private gate and fitting-screen states; closed interiors do not invalidate the public path or count unfinished tasks as completed.
- [ ] Public counter collection, quiet domestic time, shade/rest choices and shutters that change the frontage without removing street connectivity.
- [ ] Low-speed fictional service/driveway yielding, separate pedestrian and vehicle route admission; never infer vehicle legality from a pedestrian reverse trace.
- [ ] Privacy-closure event states, stable accessible public alternatives and visible/non-audio route cues; no delivery accident, vehicle breakdown, flood, forced wait or purchase trigger.

## Audio, day/night and weather

**U:** No location audio recording, measured sound level, local power schedule, rainfall series, current street-light inspection or geolocated material photography was obtained. Primary railway context does not establish that trains are audible from a selected doorway.

**P:** Small-room sound layers: sewing-machine pulses, upholstery stapling moderated behind a workroom boundary, hand-tool taps, fabric handling, food preparation and conversation. Public junctions receive restrained distant road ambience; entering a permitted vestibule attenuates it according to authored walls and openings. An optional distant rail layer requires a coordinator-approved spatial context, not an automatic loop over every Ebute Metta street. Commission/clear sounds and reviewed performances; do not lift railway announcements, popular music, prayer or residents' voices. Provide text/visual alternatives and independent speech/traffic/tool volume control.

**P:** Daylight distinguishes shady verandas, open workrooms and brighter junction mouths. Evening retains readable exterior circulation, some food-service light and quieter domestic light behind curtains. Closed shutters are ordinary schedules, not automatic danger. No blanket outage or crime state is assigned to the neighborhood. Shielded public lighting and contrasting thresholds remain legible without color-only navigation.

**P:** Dry, rainy and dusty/hazy variants alter canopy sound, surface wetness and waiting positions without claiming a measured seasonal timetable. Sheltered queues move into property recesses, never the sole clear footway. Authored covered drains receive local runoff; no inferred elevation, storm capacity or real outfall is modeled. Keep a firm alternative outside puddles; no water wading, slippery-platform challenge or flooding incident. Real gradients, deck heights, thresholds and drainage continuity stay U.

## Fictional disruption: workshop privacy closure changes the optional route

**P:** At the alterations workroom, staff close the optional side vestibule and its view-through screen while a customer has a private fitting. The space was open by invitation earlier; it was never a public right-of-way. The preferred familiar doorway no longer reaches the waiting alcove, so a returning visitor must orient to the street-facing collection counter or continue outside. This is a privacy/access change, not a real incident, obstruction accident, religious ceremony or assumed local custom. No resident or real business is identified.

Before the closure takes effect, staff let any visitor leave; nobody is locked inside. A plain closed/private state and a visible street-front collection point communicate the change without exposing the customer. There is no route through the fitting room or courtyard, no camera challenge, no eavesdropping and no reward for ignoring consent.

- **Exit A:** Remain on Market's authored firm 2.0 m public route toward E1/Freeman and the northern Kadara branch.
- **Exit B:** Continue on Market toward E4/Griffith, then the longer mapped Willoughby return via E5/E2 if that public circuit has been admitted. Maintain the 2.0 m clear route and authored step-free crossings.
- **Immediate fallback:** Turn back along the same public edge or use the street-facing counter outside the private passage. Both exits remain independent of a railway, bridge, vehicle lane or private gate. If either public departure is obstructed by another authored state, do not activate this event.

Clearing condition: the fitting concludes and the owner chooses to restore invitation access; reopening is not presumed. Collection service continues at the street-front counter where appropriate. The player can leave, cancel or use another public destination without waiting, purchasing or completing a ceremony. The interior remains optional if closed indefinitely; no geographic circulation depends on it.

## Connector endpoint offers

Incoming/outgoing labels describe design offers, not legal travel directions. Every numeric clearance below is **P**, not a survey or compliance finding. Real public/private access, slopes and crossing safety remain **U**. Preserve inhabited frontages in all retained approaches; omitted neighborhoods are not empty buffer land. Endpoint offers do not amend the older dossiers or reserve final placements.

| Offer | Source and destination reference | Supported real relationship and omitted geography | Proposed handoff and continuity |
| --- | --- | --- | --- |
| E-local-in | E2 Freeman/Willoughby -> E1 Freeman/Market | M: same Freeman way, with ordered references retained above. Omitted frontages outside the selected walk remain unverified inhabited context, not vacant lots. | P: domestic-to-workshop public approach, walking and permission-reviewed local service vehicles separately admitted; 2.0 m clear pedestrian path and 1.8 m turning/rest recess. Quiet workroom audio increases at the counter. Covered-drain continuity is authored; real grade/outfall U. |
| E-local-return | E1 -> E4 -> E5 -> E2, Market/Griffith/Willoughby | M: shared-node southern circuit; longer than this partial walk, no accessible-length claim. | P: retain domestic fronts, public rest and workroom doors rather than replace the return with a private yard. 2.0 m firm pedestrian clearance; accessible crossings authored, not asserted existing. No deck/shoreline in this local join; height and drain grades U. |
| E-north-side | E1 -> E3 -> E6 -> E4, Freeman/Kadara/Griffith | M: shared-node northern circuit. Does not relocate a market or heritage building. | P: optional additional ordinary residential interval with 2.0 m firm route; walking/resting and later access-reviewed service traffic. Carry resident activity and attenuate workshop sound with distance. No private shortcut; local drainage only proposed. |
| E-to-Yaba street reference | E7 Freeman/Oyingbo -> E8 -> node 1278601855 [3.3836234,6.4879703] -> earlier **Y-D node 2176500609 [3.3774715,6.5111851]**, Harvey/Herbert Macaulay | M: S7/S9a/S9b give a shared-node chain along Oyingbo and Herbert Macaulay Way. Y-D was freshly reread at https://api.openstreetmap.org/api/0.6/node/2176500609, v3, 2018-05-04T11:52:54Z. Omitted portions are substantial streets, intersections, homes and services between the pockets; not direct adjacency. Earlier Y-D-to-Moore/University links remain that dossier's conditional references. | P: coordinator may compress the intervening **journey**, not the houses/courtyards or people within either slice. Arrival walking path 2.0 m clear; vehicle legality and continuous safe pedestrian crossings unresolved. Separate drain systems/grades unless later evidence supports continuity. Road-to-side-street audio transition follows authored barriers, not a sudden theme switch. No final connector distance or placement. |
| E-to-Yaba rail context | Oyingbo station name in S13a -> Yaba station name in S13a; neighborhood arrival offer E1 -> earlier Y-A node 2271072132 | F: operator station sequence supports rail relationship, **not** these doorstep endpoints or a station-to-University Road footpath. Platforms, entrance nodes, fares, current train performance and both last-mile routes remain U. | P: optional travel/chapter transition only if coordinator selects this mode and verifies accessible last-mile access. Authored arrival recess 6 by 10 m, 2.0 m pedestrian continuation; no forced stairs or assumed level boarding. No rail track, station replica or continuous drain created by this document. |
| E-to-old-Island endpoint offer | Market/Iddo node **2270550585 [3.3834894,6.4798482]** -> earlier Island **C1 node 1020782267 [3.3876053,6.4587384]**, Idumagbo/Nnamdi Azikiwe | M: pocket reaches Iddo Road; S11 provides Carter Bridge leads, and S12 freshly verifies C1. Node C1 fetched at https://api.openstreetmap.org/api/0.6/node/1020782267, v3, 2026-10-10T01:17:32Z. **A complete shared-node route from Iddo across Carter to C1 has not been established.** Intermediate approaches, bridge links and northern Nnamdi Azikiwe are missing from route verification. Omitted fabric includes working Iddo approaches, the water/bridge separation and Island streets, not empty land. | P: offer a shortened journey arriving at C1 only after coordinator reconciliation; do not weld the street grids into real adjacency. Walking arrival 2.0 m clear on this side, reconcile with older Island's 1.8 m design target without a sudden pinch point. No claimed bridge sidewalk, level deck, barrier height, gradient or legal vehicle movement. Separate audio/drainage environments; bridge deck/water datum and actual pedestrian access U. An abstract accessible travel transition is an option, not a certified real route. |

Carter Bridge is a researched lead distinct from Third Mainland and Lekki-Ikoyi references. This file assigns none of their endpoints to Freeman and does not substitute one bridge for another. No station name is a surveyed entrance. No current bridge closure, demolition or construction-completion claim is inferred from OSM tags or a dated terminal-photo page.

## Accessibility, privacy and cultural guardrails

- Keep continuous firm 2.0 m public pedestrian routes, 1.2 m clear service doors and 1.8 by 1.8 m resting/turning spaces as design targets. Flush thresholds or step-free alternatives are authored; ramps, if needed, target no steeper than 1:20 pending specialist review. These are not measurements of Freeman or certification under Nigerian standards.
- Show junction mouths, public counters and private entrances at walking height. Combine sightlines with readable/non-audio cues; tiny text, color, horn detection and crowd weaving cannot be the only navigation aid. Provide quiet resting choices.
- Queueing, canopy supports, furniture, gates and rain sheltering stay outside circulation. A mobility aid changes neither a person's job nor entitlement to ordinary routes. No compulsory stairs, curb jumping, vehicle-lane crossing or private hospitality.
- Compound access is consent-based and reversible. Workshops may have public counters and private work zones simultaneously. No rail compound, museum, sacred space or household interior is assumed publicly enterable.
- Different maintenance states are not a chronology or income census. Do not portray every home as colonial, every livelihood as informal hardship, or the whole pocket as a station/market. No demolition status, displacement narrative or real property history is invented.
- Language, food names, textiles, sacred practices, institutional signs and historically specific motifs require local/contextual review before production. No unverified street-name etymology, accent caricature or ceremony substitutes for everyday people. No copyrighted recordings or real resident likenesses are reused.

## Readiness and unresolved decisions

Ready for documentation review: one bounded pocket, fresh named/shared-node topology, a 270 m authored walk with a 230 m public-only option, ordinary homes/workrooms, categorized assets, privacy-based route disruption, day/night/weather proposals and endpoint offers toward the existing Yaba and old Island network.

Still U: contemporary geolocated photography/capture dates; actual facades and materials; current tenants and public access; safe crossings and traffic rules; street/door widths, heights, gradients, drainage and vertical datum; railway/terminal entrance and last-mile access; a complete Carter-to-C1 route; building-specific history; cultural/language/audio review and rights clearance. No surveyed current appearance, colonial-house age or demolition status is accepted. Primary institutional context does not resolve these gaps.

Coordinator must select connector modes/partners, resolve overlap and accessible-width transitions, retain omitted ordinary urban intervals where appropriate and assign any later composition. No current canonical content or older dossier is moved, relabeled or edited. This document is not consumed renderer data.

Only this assigned Markdown file is authored. No tests, builds, lint, typechecks, servers, scenes, runtime/native/renderer/mobile/config edits, agents, purchases, public messages or commits were performed. Source/text/diff review is documentation verification; geographic, rendered, playable and release acceptance are not claimed.
