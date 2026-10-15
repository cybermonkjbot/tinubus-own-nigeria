# Yaba and Ikeja: tech and mixed centres

Research/access date: October 11, 2026. Documentation and authored-world proposals only. Coordinator-owned composition; no approved placements, assets, implementation or geographic reconstruction.

## Scope and evidence rules

We select two bounded slices: **University Road at Moore Road, Onike/Akoka approach in the Yaba assignment**, and **Toyin Street between the western Emina Crescent mouth and Amore Street, Ikeja**. These are street pockets, not descriptions of all Yaba or Ikeja. The Yaba choice is a student-facing university approach rather than a claim that every frontage is occupied by students or technology companies. The Ikeja choice places commerce beside residential streets rather than treating the whole area as Computer Village.

Read alongside the [coordinator contract](README.md). The existing [world-building register](../README.md) and [readiness audit](../readiness-audit.md) remain unchanged. Their Oshodi sectors, documentation IDs and canonical game content are not renamed or relocated. No earlier 6 by 4.5 km envelope is assumed.

Evidence labels used throughout:

- **F: source-confirmed fact** means the named source explicitly states it. Institutional and operator statements are attributed; they are not an independent inspection of current conditions.
- **M: map-supported inference** means OSM topology/tags support the interpretation. Shared node identity supports a mapped junction, not surveyed access, a usable crossing, current traffic permission or sidewalk condition.
- **P: authored game proposal** means our fictional composition, dimensions, materials, interiors, activity or simulation. P does not describe an observed real property. All unnamed businesses and households below are fictional production metadata, not invented real business names.
- **U: unresolved** means no adequate evidence was obtained. Coordinate precision does not imply accuracy. No architectural dates, elevations, real widths or current building occupancy are inferred.

Source coordinates below are **WGS84 longitude, latitude**, in decimal degrees, with no altitude. OSM XML names its attributes `lat` and `lon`; the ledger deliberately prints **lon, lat**. OSM API `bbox` order is **west longitude, south latitude, east longitude, north latitude**. Source coordinates are research anchors, not game positions. Existing metre-space convention stays **x east, y up, z south; UE(X,Y,Z) = 100 * source(z,x,y)**, applied once by the eventual integration owner. No geographic registration, height datum, origin, final rotation or engine position is assigned here.

## Research method and source register

Fresh read-only OSM API requests retrieved XML node/way extracts and selected full ways. Names, way versions, tag values, node references and shared-node coordinates were inspected directly. Institutional/operator HTML was fetched and its text read. This pass did not inspect street photographs, satellite pixels, video or private interiors. No imagery capture dates are therefore claimed; upload paths, OSM edit timestamps and page publication dates are not imagery dates.

All fetched sources in this register were accessed **October 11, 2026**; L2 is an unfetched licence link discovered on L1 and is explicitly distinguished below. IDs and versions below retain selected evidence in this Markdown file; full raw XML/HTML was not saved because this assignment permits only this output file. Live endpoints can change. The recorded versions identify this reading, not a complete immutable archive. OSM-derived names, coordinates and topology: copyright OpenStreetMap contributors, available under the Open Database License (ODbL); exact rights reference L1.

| Ref | Exact source URL | Date/coverage and permitted claim | Limits and rights |
| --- | --- | --- | --- |
| Y-M1 | https://www.openstreetmap.org/api/0.6/map?bbox=3.377,6.507,3.387,6.516 | Fresh Yaba/Onike discovery extract; roads and shared nodes used below. Selected full ways can extend outside the request rectangle. | OSM contributor data, not field survey; building uses, widths and access absent/unverified. ODbL; see L1. |
| Y-M2 | https://www.openstreetmap.org/api/0.6/way/207402243/full | University Road v17, edited September 8, 2026; full node order read. | `primary`, `surface=paved`, `oneway=no` are tags, not a current traffic/surface inspection. |
| Y-M3 | https://www.openstreetmap.org/api/0.6/way/217801562/full | Moore Road v6, edited June 14, 2024; joins University Road and Harvey Road in the mapped graph. | `unclassified`, `surface=unpaved`; material/maintenance status in October 2026 unknown. |
| Y-M4 | https://www.openstreetmap.org/api/0.6/way/217801566/full | Unnamed residential way v2, edited June 14, 2024; Moore-to-Herbert Macaulay side connection. | No street name supplied; never invent one. `surface=unpaved` and mapped connection do not establish public passage. |
| Y-M5 | https://www.openstreetmap.org/api/0.6/way/334647370/full | Tobi Lawal Rd v7, edited March 10, 2025; University/Moore junction shares node 2271072132. | `unclassified`, `surface=unpaved`; no width/access evidence. |
| Y-M6 | https://www.openstreetmap.org/api/0.6/map?bbox=3.377,6.507,3.387,6.516 | Harvey Road way 114912315 v6, June 14, 2024; Herbert Macaulay Way 534993359 v33, October 7, 2024, retrieved in Y-M1. | Harvey has `source=Bing`, but no image acquisition date; source tag is not inspected imagery. Herbert Macaulay has `foot=yes`, `oneway=yes`, `surface=paved`; no crossing guarantee. |
| Y-F1 | https://unilag.edu.ng/invitation-for-pre-qualification-rehabilitation-of-student-hostel-for-college-of-medicine/ | Published February 9, 2026. UNILAG identifies its main campus as Akoka, Lagos. | The works described are at the College of Medicine, Idi-Araba, not this slice. Do not relocate that project to Moore Road. Text read; photographs not inspected. No open media licence established. |
| Y-F2 | https://unilag.edu.ng/unilag-breaks-ground-for-new-1200-bed-space-student-hostel/ | Published January 19, 2026, reporting a January 17, 2026 groundbreaking. UNILAG discusses expensive accommodation outside its gate and student housing needs. | Context for student routines/economic variation, not proof of any selected parcel's tenancy or finished construction. Project completion not verified. Report credits Bayo Salau and photographs Ayo Oloyede; photos not inspected or reused. |
| Y-F3 | https://unilag.edu.ng/unilag-postgraduates-gain-first-hand-insights-into-nigerias-broadcast-history-from-veteran-broadcaster/ | Published September 2, 2026; reports an August 31, 2026 mass-communication session at UNILAG. | Supports study beyond a coding stereotype; no claim that this activity happened on Moore Road. Text only; no image or dialogue reuse rights established. |
| I-M1 | https://www.openstreetmap.org/api/0.6/map?bbox=3.347,6.590,3.357,6.604 | Fresh Toyin/Emina/Amore extract; the larger request was used after initial discovery. | Roads/tags, not tenancy or measured clearances; ODbL. |
| I-M2 | https://www.openstreetmap.org/api/0.6/map?bbox=3.349,6.595,3.355,6.603 | Initial Ikeja discovery extract; Toyin ways and Amore identified. | Bounding box is a research query, not a proposed world envelope. |
| I-M3 | https://www.openstreetmap.org/api/0.6/way/136415222/full | Emina Crescent v7, November 10, 2022; full way confirms two Toyin junctions. | `residential`, `oneway=no`, `surface=asphalt`, `smoothness=intermediate`; tag age/current condition unresolved. |
| I-M4 | https://www.openstreetmap.org/api/0.6/way/136415215/full | Amore Street v5, February 23, 2022; full node order read. | `residential`, `oneway=no`; surface not tagged. No compound occupancy or access established. |
| I-M5 | https://www.openstreetmap.org/api/0.6/map?bbox=3.347,6.590,3.357,6.604 | Toyin Street ways 136415219 v13 and 136415218 v11, both June 26, 2026; node 13968439187 connects mapped Allen Avenue/Opebi/Toyin ways. | Both Toyin ways have `tertiary`, `lanes=2`, `oneway=yes`, `surface=asphalt`, `smoothness=excellent`. These are mapped split directional ways; do not add their lane counts to claim a surveyed four-lane cross-section. |
| I-F1 | https://all.accor.com/hotel/7577/index.en.shtml | Operator page for ibis Lagos Ikeja lists 75 Toyin Street; GPS printed as latitude 6.596099, longitude 3.351254; describes a residential/commercial area with surrounding shops/restaurants and hotel restaurant, bars, gym and pool. | Page has no single verified publication date; accessed October 11, 2026. Operator coordinate converted below to lon, lat. Address/offer is not a cadastral footprint or facade inspection. No photo/logo/private-interior reuse permission established. Ignore promotional travel-time estimates for navigation. |
| L1 | https://www.openstreetmap.org/copyright | OSM's copyright/licensing page read on access date. | Attribute OpenStreetMap contributors and identify ODbL. OSM data rights do not license underlying Bing imagery or institutional/operator photos. |
| L2 | https://opendatacommons.org/licenses/odbl/1-0/ | Exact licence target linked from L1; not separately fetched in this pass. | Distribution of a derived database and a produced visual work have different obligations; obtain a rights review before asset/map distribution. Do not treat this dossier as clearance for proprietary imagery. |

Research gap: https://studentaccommod8.com/ failed certificate trust validation in this environment on October 11, 2026. Validation was not bypassed and no claims rely on that site. UNILAG's Moore Road search returned no results; a University Road search led to Y-F3, whose article does not establish that street address. Neither search is evidence for a specific student residence here. OSM POI discovery also contained inconsistent city/address tags; those uncorroborated businesses and worship names are excluded.

## Slice 1: University Road / Moore Road student-commercial pocket

### Source anchors and connected streets

**F:** UNILAG's own Y-F1 places its main campus at Akoka; Y-F2 supplies student-housing context and Y-F3 supports a broader academic life. **M:** Y-M1 maps Onike as suburb node 14178015854 at **3.3824862, 6.5125737**. That is a label node, not an administrative boundary. Our choice is the northern Moore Road junction pocket in the Yaba research assignment, not a surveyed definition of Yaba.

University Road is the main frontage. Moore Road and Tobi Lawal Rd meet it at the same mapped node. Farther along Moore, an unnamed side street reaches Herbert Macaulay Way; Moore also reaches Harvey Road. These are separate connections, not all one 240 m intersection cluster.

| Anchor | Coordinate (lon, lat) | Recorded node version / edit time UTC | Mapped relationship |
| --- | --- | --- | --- |
| Y-A: node 2271072132 | 3.3774551, 6.5152498 | v4 / 2024-06-14T11:27:06Z | Shared by University 207402243, Moore 217801562 and Tobi Lawal 334647370. |
| Y-B: node 2271072146 | 3.3779231, 6.5141172 | v4 / 2025-03-10T04:26:45Z | Moore 217801562 / unnamed side street 217801566. |
| Y-C: node 2271072117 | 3.3788712, 6.5117847 | v5 / 2025-03-10T04:26:45Z | Moore 217801562 / Harvey 114912315; outside the short authored walk. |
| Y-D: node 2176500609 | 3.3774715, 6.5111851 | v3 / 2018-05-04T11:52:54Z | Harvey 114912315 / Herbert Macaulay 534993359 / Harvey continuation 585484496. |
| Y-E: node 2271072446 | 3.3765029, 6.5135349 | Version not separately retained | Unnamed way 217801566 / Herbert Macaulay 534993359, established by shared references in Y-M1. |

Exact individually fetched node URLs for Y-A through Y-D:

- https://www.openstreetmap.org/api/0.6/node/2271072132
- https://www.openstreetmap.org/api/0.6/node/2271072146
- https://www.openstreetmap.org/api/0.6/node/2271072117
- https://www.openstreetmap.org/api/0.6/node/2176500609

Retained node order for Moore v6: `2271072132,5594909288,5594909287,2271072146,5594909286,5594909285,5594909284,5594909283,5594909282,2271072117`. Unnamed way v2: `2271072446,2271071950,2271072146`. This supports the connected branch graph without inventing a named alley or a campus gate location. There is no verified campus entrance at Y-A in this pass.

### Walking-height sequence: 240 m authored design

**P for this entire sequence and all dimensions.** Chainage is walked design distance, not OSM measurement; it does not assign scene coordinates. Keep bodies, vehicles, setbacks and ordinary room sizes at human scale. Preserve the quieter gap between shops rather than replacing the block with a single technology landmark.

| Design chainage | Street/space and visible action | Navigation and proposed dimensions |
| --- | --- | --- |
| 0-50 m | University Road approach: a stationery/print counter, an open food-service hatch and a recessed household gate. Someone collects bound course notes; another visitor orders lunch; a resident returns with groceries. | 7.0 m carriageway design value; 2.0 m continuous clear pedestrian strip on the walk side; shop queue recessed into a 3.0 m forecourt. A street-name cue and the visible Moore turn, not a giant campus logo, orient the walk. |
| 50-90 m | Turn at a junction inspired by Y-A into Moore. An unbranded small repair counter handles a cracked phone and a desk lamp, not a street of coding cafes. | 5.5 m side-street carriageway design value; 2.0 m clear strip. Carry the pedestrian surface continuously across an authored low-speed driveway crossing. Junction geometry is fictional, not a claim about existing kerbs. |
| 90-165 m | Moore residential interval: two differently maintained compounds, one shared-rental entry and one quieter family frontage. A person studies on a shaded veranda while someone hangs washed clothing inside the yard. | Proposed 2.5 m gate recess; 4.0 m service setback at one property; 1.2 m accessible shop door. Shade, gate rhythm and a bench alcove distinguish this stretch. No requirement to enter a home. |
| 165-185 m | Moore-side seam, inspired by Y-B: a laundry collection shelf and visible unnamed side-street mouth. The print customer meets a classmate; a resident receives a repaired appliance. | Reserve 2.0 m clear movement past any waiting customer. The unnamed street is labeled by direction/destination in fictional navigation, never given a false real name. |
| 185-240 m | Enter the first 55 m of an authored version of the unnamed branch; a publicly reachable tailor/alterations front room faces the street. A private yard is visible through a gate, not used as a through-route. | Side-street carriageway 4.5 m design value; clear pedestrian edge 2.0 m; optional 8 by 10 m private yard visible from threshold. Walk ends at a public forecourt; the street continues beyond it rather than terminating in a landmark wall. |

The design devotes 135 m to Moore after the initial 50 m approach. This is a proposed sequence, not a calculation that the whole Moore Road or the Harvey return fits inside 240 m. Tobi Lawal provides a separate northern branch off Y-A; Harvey and Herbert Macaulay remain contextual exits beyond the short walk.

### Ordinary destinations, interiors and socioeconomic variation

**P:** Four useful public destinations: print/binding counter, food hatch with a washable preparation room, repair desk with parts storage, and alterations front room with fitting screen. Print collection, lunch pickup, appliance collection and a clothing repair give navigation concrete local purposes. No real branding is needed.

**P:** Behind the public fronts, provide a 4 by 6 m print room, a 3 by 5 m repair room and a 3 by 4 m food preparation room. These are design values, not surveyed floor plans. Upper living space is optional private scenery; every necessary public interaction stays at ground level. Keep working equipment behind the counter and a clear visitor route rather than filling every surface with laptop props.

**P:** One shared-rental compound has individual room doors, a common shaded sitting area, a laundry zone and bins separated from food preparation. A neighboring family compound has a more enclosed veranda, planted pots and a cleaner repainted frontage; a third tenant uses a small street-facing workspace. Do not infer real incomes from roofs, facade color, residents' appearance or tenancy. Mixed resources are authored through different repair states, service choices and shared/private space, not a poor-to-rich color gradient.

**P:** Morning: workers and students leave, a household gets breakfast, print orders are prepared. Midday: some residents remain home, repair work continues, deliveries wait off the clear path. Evening: food collection and conversation outlast some shutters; residential doors stay private. Students study communications, design, accounting or other subjects as well as computing. No specific enrollment, timetable, uniform, language mixture or daily population is claimed.

### Developer Asset Checklist

All items are **P**, original assets required; no existing asset ID is reassigned.

**Meshes and architecture**

- [ ] Modular ground-floor fronts and one-/two-upper-level facade variants, with proposed floor-to-floor height 3.2 m; varying gate recesses and verandas; no brutalist attribution or construction date.
- [ ] Compound walls, optional view-through gates, private-room door groups, yard canopy, washing rack, planted pots and shaded seating; private/public thresholds readable without real resident identifiers.
- [ ] Print counter, binder, paper stacks, neutral sample sheets, shelves; repair workbench, parts trays and desk-lamp repair props; food hatch/prep surfaces; tailoring machine, fabric storage and fitting screen.
- [ ] Continuous pavement, covered-drain modules, driveway crossings, seating recesses, public forecourt and removable event barricades; collision for gate leaves and retractable shutter without protrusion into accessible circulation.

**Materials, shaders and textures**

- [ ] Original painted-render variations, repair patches, metal gates/shutters, corrugated canopy sheets, smooth washable food-room surfaces, paper/fabric and vegetation. These are selected materials, not source-observed architectural claims.
- [ ] Paved University-inspired surface and authored compacted/mineral side-street option; OSM's `unpaved` tag cannot select sand/gravel/earth subtype. Keep the playable pedestrian strip firm and continuous regardless of background option.
- [ ] Controlled wetness/roughness and splash masks; localized wear at thresholds rather than universal grime. Original fictional signs and course sheets; no university marks, personal data or copied photographs.

**Dynamic systems and spawners**

- [ ] Distinct resident, student, visitor, repair worker, food-service and delivery routines; varied ages/genders and mobility aids without mapping identity to work or income.
- [ ] Counter service, order collection, shutter schedules and optional seated conversations; queuing zones outside the 2.0 m clear strip; no mandatory tutorial or forced acknowledgements.
- [ ] Private-gate permission state, yard-only household routines, low-speed delivery yielding, independent incident exits and a weather shelter choice that never consumes the sole accessible path.

### Audio, light, day/night and weather

**U:** No location recording, measured sound level, power reliability, climate series or street-light survey was obtained. **P:** Use commissioned/consented sounds: binding machine clicks, paper movement, food preparation, repair tools, conversation with reviewed performances and restrained distant traffic. Lower the main-road bed inside Moore; let gates and walls occlude sources rather than applying one citywide crowd loop. Keep traffic, machinery and speech independently adjustable, with non-audio navigation equivalents.

**P:** At night, lit service hatches, occupied verandas and shielded exterior lamps produce different public/private light pools; reduced trade is not an automatic danger state. Any optional power-loss moment affects a particular fictional property, not every street or a claimed Lagos schedule. Backup power, if used, is outdoors and away from occupied interiors; no emissions or electrical-hazard gameplay is needed.

**P:** Author dry, rainy and dusty/hazy variants without claiming a calendar forecast. Rain on canopy sheets differs from rain on paving; water flows to authored covered drains, never from assumed elevation data. Local puddles stay outside an alternative firm route; sheltered queueing is relocated into forecourts. Real drain dimensions, gradients and flooding frequency are **U**, so this is not calibrated flood simulation.

### Fictional disruption: binding-paper delivery blocks a gate-side crossing

**P:** A handcart's strapped paper bundles shift while being moved from a small delivery vehicle into the print premises near the Moore turn. Staff stop the transfer and cordon the forecourt/driveway crossover; nobody is injured. Do not identify a real trader or imply an observed incident.

The blocked crossing is local, not the sole entrance to Yaba. Before activation, the authored traffic marshal stops the nearby low-speed vehicle approach and opens a firm 2.0 m bypass around the forecourt. Exit A remains backward along University Road; exit B continues down Moore outside the loading recess; exit C takes the Tobi Lawal branch at the junction if its public route has been admitted. Farther along Moore, the unnamed side branch is an additional exit only after access verification; a private yard is never necessary. Give visible directional cues before the obstruction, optional spoken equivalents and the ability to turn back without a task penalty.

Clearing condition: staff secure the bundles and park the vehicle fully inside the fictional loading recess; remove the barrier only after people have cleared the crossover. Player help is optional. No timed platforming, curb hopping, forced purchase or waiting ritual. Event authoring must preserve A and B even if C is not approved.

### Connector handoff

All clearances are **P design values**; observed clearances/grades are **U**. Incoming/outgoing describes the proposed walk, not one-way pedestrian law.

| Brief | Source-side / destination-side reference | Supported relationship and omitted geography | Proposed join, modes and continuity |
| --- | --- | --- | --- |
| Y-in: main approach | University way 207402243 western endpoint node 5602610868 (**3.3762413, 6.5147414**) / Y-A node 2271072132 | M: same named way and retained ordered references. Intervening frontages are not verified or empty land. | P: retain a readable main-road approach and 2.0 m clear pedestrian continuation; public gameplay street proposed, real access unresolved. Do not compress its facades to manufacture a gate. Main-road traffic and shop audio taper into the side street; drain/outfall transition requires later design, no vertical datum. |
| Y-out: residential continuation | Y-B node 2271072146 on Moore / Y-C 2271072117 at Harvey, then Y-D 2176500609 at Herbert Macaulay | M: Moore reaches Harvey, Harvey reaches Herbert Macaulay through shared nodes. This larger real graph is not the 240 m walk and does not establish a usable pedestrian crossing. | P: preserve additional residential frontage, service gates and quiet intervals. Walking and low-speed local vehicles subject to later access/traffic review; separate pedestrian crossing decisions at Herbert Macaulay. Minimum 2.0 m authored clear walk; no promised bridge/deck/elevation. Keep local drainage connected only in authored design and increase traffic bed toward the main road. |
| Y-side: unnamed return | Y-B / Y-E 2271072446 on Herbert Macaulay | M: way 217801566 connects these nodes. No verified name, permission, width or current paving. | P: optional public side-street continuation only after access review; do not make a private gate the mandatory return. 2.0 m clear walk design value; no invented real destination sign. Omitted frontages must remain an inhabited street interval if reconstructed. |
| Y-to-I composite offer | Y-A / I-A node 1496385144, western Toyin/Emina mouth | **No direct adjacency supported.** Two distinct mapped city pockets; intervening Lagos streets, junctions, homes and commercial districts are omitted, not empty land. No real itinerary researched. | P: optional shortened journey with a legible chapter/transport transition, not a newly named real road. Walking/vehicle arrival modes and counterpart placement belong to coordinator. Proposed arrival forecourt 6 by 10 m, 2.0 m pedestrian path; no airport/campus shuttle claim. No literal continuous drain across the fiction seam; audio/traffic fade between separately authored environments. Height relationship and full vehicle routing unresolved. |

## Slice 2: Toyin Street / Emina Crescent / Amore Street pocket

### Source anchors and connected streets

**F:** I-F1 places ibis Lagos Ikeja at **75 Toyin Street**, describes surrounding residential/commercial uses and names hotel services. Its printed GPS becomes **3.351254, 6.596099 (lon, lat)**. This is a research reference near the western Emina mouth, not permission to duplicate the hotel, its grounds or private interiors.

**M:** Toyin is the main street. Emina Crescent way 136415222 has both endpoints on Toyin way 136415219, forming a mapped side-street return rather than a dead end. Amore Street attaches separately farther west. The Allen/Opebi junction is a contextual outgoing anchor beyond this small pocket, not a second landmark biome squeezed into the walk.

| Anchor | Coordinate (lon, lat) | Recorded node version / edit time UTC | Mapped relationship |
| --- | --- | --- | --- |
| I-A: node 1496385144 | 3.3509830, 6.5960945 | v2 / 2018-04-17T17:47:23Z | Western Emina mouth on Toyin 136415219. |
| I-B: node 1496385179 | 3.3540903, 6.5957668 | v4 / 2026-05-05T01:37:22Z | Other Emina mouth on the same Toyin way. |
| I-C: node 1496385241 | 3.3498698, 6.5962577 | v3 / 2019-09-20T08:46:25Z | Amore 136415215 / Toyin 136415219. |
| I-D: node 13968439187 | 3.3550703, 6.5958717 | v2 / 2026-06-26T15:39:03Z | Toyin 136415219/136415218, Allen 134404795/136338240 and Opebi 136341474/136341477 share the node in I-M1. |
| I-E: node 1496385075 | 3.3495621, 6.6003030 | Version not separately retained | Far endpoint of Amore in I-M4; beyond the 260 m walk. No outgoing connection claimed here. |

Exact individually fetched node URLs for I-A through I-D:

- https://www.openstreetmap.org/api/0.6/node/1496385144
- https://www.openstreetmap.org/api/0.6/node/1496385179
- https://www.openstreetmap.org/api/0.6/node/1496385241
- https://www.openstreetmap.org/api/0.6/node/13968439187

Emina v7 retained order: `1496385144,1496385233,5575849435,1496385170,1496385118,1496385236,1496385090,1496385251,5575849434,1496385126,10173044684,1496385229,1496385204,1496385139,1496385160,1496385088,5575849433,1496385211,1496385081,1496385179`. It is a larger mapped return; its entirety is not compressed into 260 m or assigned a surveyed walking length.

Amore v5 retained order: `1496385241,9529313232,8925989251,1496385120,6811473376,1496385231,8925989249,1496385272,1496385191,1496385150,1496385209,1496385134,1496385114,1496385060,1496385164,1496385075`. Road tags alone do not establish that any specific residence is publicly traversable.

### Walking-height sequence: 260 m authored design

**P for this entire sequence and all dimensions.** Use the western Toyin/Emina mouth and Amore attachment as topology references, not a tracing of hotel plots. No real institution is relocated into the authored pocket.

| Design chainage | Street/space and visible action | Navigation and proposed dimensions |
| --- | --- | --- |
| 0-45 m | Begin at a western Emina-inspired side-street mouth on Toyin. A small unbranded lodging/service frontage receives a guest's bag while a resident passes without entering. The side street remains visible as a real navigation choice. | Main carriageway 7.0 m design value, not derived from OSM lane counts; 2.2 m clear pedestrian strip. Arrival bay and bags remain inside a 4.0 m setback. No reproduced ibis facade/sign or assumed hotel entrance location. |
| 45-85 m | Toyin errands: pharmacy-style general health-supplies counter, food pickup and an electrical/appliance repair counter. A technician takes in a fan; an office worker collects a parcel; a resident buys household supplies. | Queue recesses 3.0 m deep; publicly usable ground-floor doors 1.2 m clear design value; deliveries separate from walking. No medical advice, prescription gameplay or invented real shop identity. |
| 85-125 m | Quieter gate frontage before the Amore turn: a planted family compound and a building with an upper rented flat and ground-floor office. Service activity slows without disappearing. | 2.2 m clear Toyin path, covered drainage crossing at gate, seating in a 2 by 4 m recess. Wayfinding uses a street-mouth sightline and distinctive authored gate proportions, not wealth-coded colored zones. |
| 125-175 m | Turn into Amore-inspired residential street. A resident waits at a recessed gate for a visitor; a laundry worker sorts returned garments behind a public counter. | Side carriageway 5.5 m design value, 2.0 m clear walking edge. Keep a view back to Toyin and continue the street beyond the next gate; not an enclosed themed hotel courtyard. |
| 175-210 m | Small mixed front room: laundry collection, accounting/office work at a separate desk, domestic sounds behind a private threshold. A resident takes a repaired fan home. | Public front room 4 by 5 m design value; private room not a navigation target. A visibly separate service gate avoids opening every compound to visitors. |
| 210-260 m | Optional 50 m out-and-back within a public repair/laundry forecourt and covered waiting passage. A private planted courtyard is visible but not entered. End again at the Amore public edge, with the onward street unobstructed. | Forecourt 10 by 12 m and covered passage 2.0 m clear design values. Turning/meeting space 1.8 by 1.8 m; no required stairs. Optional interior visit does not replace the through street. |

### Ordinary destinations, interiors and socioeconomic variation

**P:** Useful destinations are appliance collection, parcel collection, laundry pickup, a food counter and a small office meeting room. An authored lodging reception is optional, not the whole street. A phone can be an everyday tool for a technician, office worker or resident; technology is not exclusively programming or a neon coworking cafe.

**P:** Provide a compact ground-floor office with ordinary filing/storage, chairs and a shared desk; a repair room with fan parts and a safe disassembly bench; a laundry sorting room with shelves, a covered drying yard and separate utility access. A domestic sitting room and veranda can be seen from a consented fictional home visit but are never compulsory route links. Hotel pool/gym services reported by I-F1 are reference context only, not mandatory copied assets.

**P:** Include a well-maintained gated compound beside a rented-flat building and a street-facing service business. Show different maintenance budgets, shared facilities and levels of privacy without assigning real household incomes. Some office workers walk, some take a vehicle; a technician's work is skilled rather than background poverty scenery. Residents are not extras that disappear when the businesses close.

**P:** Morning has laundry intake, school/work departures and supply deliveries; afternoon has repair collection, office visitors and shaded residential activity; evening has food pickup, subdued gate conversations and occupied homes behind darker shutters. Times and density are design schedules, not operator opening hours or local population measurements. No claim about current bus, motorcycle or tricycle permission is made.

### Developer Asset Checklist

All items are **P**, original assets required.

**Meshes and architecture**

- [ ] Mixed small commercial fronts, upper-flat facades, recessed compound gates, veranda and planted yard variants; proposed floor-to-floor height 3.2 m without style/date attribution.
- [ ] Ground-floor office/reception, parcel shelving, household-supplies counter, repair bench and fan parts, laundry desk/racks; no unlicensed operator logos, copied interiors or authentic-looking invented addresses.
- [ ] Arrival/loading bay, sheltered waiting forecourt, retractable shutters, service gate and covered-drain crossings; movable event cones and delivery dolly kept separate from pedestrian collision.
- [ ] Domestic seating, plants, curtains, refuse storage away from food activity, utility enclosure and optional outdoor power unit; not every property receives the same clutter kit.

**Materials, shaders and textures**

- [ ] Original painted plaster/render, gates/railings, textured paving, canopy metal, glazed office front and domestic fabric; selected design palette, not image-confirmed local materials.
- [ ] Asphalt-inspired Toyin/Emina roads informed only at tag level by I-M3/I-M5; Amore surface remains unknown, so any chosen paving is explicitly authored.
- [ ] Wet asphalt and paving response, canopy drip edges and contained puddle masks; preserve contrast at crossings and avoid reflective glare hiding the clear path.
- [ ] Original service signs, neutral parcel labels and paperwork with no real names, phone numbers, personal records, copied logos or medical advertising claims.

**Dynamic systems and spawners**

- [ ] Independent resident, office visitor, laundry, repair, delivery and lodging routines; public counter queues and arrival/loading positions outside accessible paths.
- [ ] Gate invitation/closure, optional public meeting or service collection, shutter states and household activity that continues after trade quiets.
- [ ] Low-speed driveway yielding, reversible delivery incident, two unblocked pedestrian exits, optional Emina return after access review and independent rain shelter behavior.

### Audio, light, day/night and weather

**U:** No onsite recording, facade lighting inspection, rainfall time series or flood-depth evidence. **P:** Use separate beds for Toyin traffic, quieter Emina/Amore conversation, fan repair, laundry handling, shutters, cutlery and gated domestic space. Do not sample hotel promotional videos, popular music or worship recordings. Commission context-reviewed voices and generic licensed machinery; conversational language and accent choices require local review, not random phrase decoration.

**P:** Daytime navigation reads by street mouths, recessed gates and awnings. Evening keeps some food/arrival light on the main frontage and quieter warm domestic light behind gates; do not add a universal club soundtrack. Any business closure leaves a readable public path. A light failure at one fictional office is optional and unrelated to crime or a claimed outage pattern.

**P:** Dry/rain/haze variants use authored exposure and surface response, not an asserted seasonal timetable. Rain changes canopy sound, umbrella use and waiting positions; shuttered shops do not create an impassable sidewalk. Covered drain crossings remain firm and legible. No real gradient, flood hazard or drain capacity has been sourced; do not make Amore universally flooded or automatically affluent because it has gates.

### Fictional disruption: appliance delivery temporarily spans a service driveway

**P:** A replacement washing machine is unloaded into the fictional laundry near the Toyin/Amore corner. A loose dolly wheel makes staff halt in the service driveway while securing the load. The temporary obstruction occupies the loading crossover, not both street exits, and is not attributed to a real company or resident.

Exit A continues on the maintained Toyin-side pedestrian strip with an authored 2.2 m clear bypass. Exit B remains down Amore with a 2.0 m clear pedestrian edge; a visitor can reverse to the previous forecourt. Exit C uses the mapped Emina side-street return via I-A and I-B only after public access and safe Toyin crossing are verified/admitted; do not shorten it to the event's immediate footprint. No through-home route, uncontrolled vehicle-lane crossing, jumping or purchase is required. Staff keep delivery movement stopped until pedestrians clear. If the design cannot preserve A and B, the incident must not spawn.

Clearing condition: the dolly is stabilized, the appliance reaches the receiving bay and the vehicle leaves or parks fully clear. Optional assistance is not a gate on progress; the fictional staff can complete the transfer themselves. Visible cues and text-equivalent route information replace reliance on shouting or horn sounds.

### Connector handoff

| Brief | Source-side / destination-side reference | Supported relationship and omitted geography | Proposed join, modes and continuity |
| --- | --- | --- | --- |
| I-in: side-street arrival | I-B node 1496385179 / I-A node 1496385144 through Emina 136415222 | M: one continuous residential way returns to Toyin at both ends. Its intervening homes/frontages are not surveyed, not blank space, and not compressed into the 260 m walk. | P: optional longer residential circuit with public gameplay access subject to real access review; 2.0 m pedestrian clear width, low-speed local vehicles only after traffic review. No private gate as required cut-through. Quiet audio along the bend returns to Toyin's traffic bed; grade/drain outfall unknown. |
| I-out: Amore continuation | I-C node 1496385241 / I-E node 1496385075 through Amore 136415215 | M: same mapped street extends beyond the selected frontages. No onward junction or through-route beyond I-E verified here. | P: keep additional homes, yards and service intervals if expanded; 2.0 m clear path, optional local service access. Public/private boundaries remain explicit. No asserted elevation, bridge or continuous real drain; avoid false destination signage at an unresolved endpoint. |
| I-out-main: Allen/Opebi approach | I-A / I-D node 13968439187 through Toyin 136415219 | M: Toyin reaches the mapped Allen/Opebi junction; split ways and one-way tags recorded, not surveyed manoeuvres. Intervening urban fronts omitted from the short sequence. | P: maintain a main-street continuation rather than a blank connector corridor. Walking and vehicle routing reviewed separately; 2.2 m clear walking design value, crossing permission unresolved. Traffic/audio increase toward the junction; actual lanes, carriageway division, deck/kerb heights and drainage require later evidence. |
| I-to-Y composite return | I-A / Y-A node 2271072132 | **No direct real adjacency or itinerary established.** This is the reverse endpoint offer for Y-to-I, not a second invented real road. All intervening inhabited city fabric is omitted explicitly. | P: shared optional journey/arrival transition controlled by coordinator. Do not connect the two drain networks or imply a continuous 260/240 m street. 6 by 10 m arrival forecourt and 2.0 m pedestrian continuation proposed; vehicle path, height relationship and placement unassigned. |

## Human-scale navigation, accessibility and cultural guardrails

These are **P authoring requirements**, not claims of existing accessibility or legal compliance. Keep the two slices legible through street-mouth sightlines, gates, public forecourts, awnings and quieter residential intervals. Do not put all destinations behind one prestige building or shrink bodies, vehicles, room depths or the distance between ordinary doors to meet a composition envelope.

- Maintain a continuous firm 2.0 m pedestrian route in the authored side streets; Toyin's proposed route is 2.2 m. Provide 1.8 by 1.8 m turning/rest spaces near public services and 1.2 m clear public doors. Numbers are design targets, not real measurements or certified standards.
- Use flush public thresholds or authored step-free alternatives; target ramps no steeper than 1:20 where included, pending specialist review. Upper private rooms need not be navigation objectives. Unknown real gradients remain unknown; do not generate steps from geographic coordinates.
- Keep loading, queues, canopy posts, gate swings, drainage openings and parked vehicles outside the clear route. If a detailed background includes a damaged edge, supply a firm alternative without making disability a difficulty mode.
- Make public/private access visible and preserve cancellation/turnback. Compound courtyards are households' spaces, not free city shortcuts. Optional invitations may admit a fictional visit; no forced entry, snooping at personal records or compulsory home access.
- Offer non-color and non-audio navigation equivalents, readable contrast, adjustable competing sound layers and resting points. Do not require horn detection, rapid crowd weaving, curb jumping or flood-water traversal.
- Portray people of varied genders, ages, resources and abilities as residents and skilled participants. Do not make Yaba synonymous with coding or Ikeja synonymous with gadgets, nightlife or a single income class. Do not use wear, skin tone, clothing or accent as a proxy for danger, honesty or wealth.
- Dialogue, food names, fabric associations, religious routines, signs and performances need context-specific local evidence/review before production. This pass commissions no speakers, contacts no residents and asserts no locally measured language distribution. Use unnamed services and original generic props until that review is authorized.
- No real operator is associated with fictional accidents, misconduct or hazards. Document references to ibis/UNILAG support context; they do not imply endorsement or rights to marks, private grounds, photos or architecture reproduction.

## Evidence gaps and coordinator decisions

The core road anchors and selected shared-node topology are retained; everyday content is a detailed authored proposal, not a street survey. Outstanding **U** items: contemporary photographs with verified capture/location metadata; facade/roof material observation; business/tenancy verification; permissions and gate states; pedestrian crossing feasibility; real widths/heights/kerbs/gradients; vertical datum and drain outfalls; current traffic and transport permissions; cultural/language review; consented audio and media/branding clearance. OSM edit dates and operator GPS cannot close these gaps.

Before any implementation, coordinator must reconcile the Y-to-I endpoint offer with all other dossiers, select the actual incoming/outgoing partners and journey modes, approve access-dependent alternates and assign composition/placement. No cross-dossier endpoint outside these two slices is invented here. If a continuous surveyed route or legally cleared real-property reconstruction is required, these unresolved items block that acceptance; they do not block delivery of this fictional research-backed blueprint.

No runtime map, renderer, mobile file, configuration, canonical placement or asset registry was edited. No build, test, lint, typecheck, server start, purchase, publication, agent launch, renderer packaging or git commit is part of this dossier. Review is source/text/diff only; playable, rendered and geographic acceptance remain unperformed.
