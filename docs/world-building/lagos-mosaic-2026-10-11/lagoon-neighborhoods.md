# Lagoon neighborhoods: two street-scale Lagos composite slices

Research and authoring date: October 11, 2026. Exclusive dossier: lagoon neighborhoods. Documentation only; no map registration, engine placement, asset production or runtime change.

## Reading contract

We select two concrete reference slices, not two citywide biomes:

1. **Makoko Road / connected footpath loop / authored shore-water threshold**, within the Makoko-Iwaya reference area. Ramotu Street supplies a mapped landward continuation. The playable water threshold is fictional, not a claim that the mapped loop ends at a surveyed jetty.
2. **Ipakodo terminal approach / residential branches / ferry boundary**, around the western F270 approach to Beach Road and the mapped Ikorodu Ferry Terminal. This contrasts a neighborhood served by formal ferry infrastructure with a small-scale canoe threshold; it does not equate Ipakodo with universal affluence or Makoko with universal deprivation.

Evidence labels used throughout:

- **F: source-confirmed fact** means the cited text or OSM response actually states it. A historical fact remains historical; a witness statement remains attributed, not a measured environmental result.
- **M: map-supported inference** means a relationship inferred from retrieved geometry/shared nodes. M does not establish current physical passage, street material, access rights or building use.
- **P: authored game proposal** means fictional compression, dimensions, activity placement, interiors, incident logic or atmosphere. P is never surveyed geography.

All proposed distances, widths, clearances, gradients, room sizes, timings and activity counts below are **design values**. Actual elevations, waterfront/deck levels, water depths, road widths and architectural dates are unknown. No final engine positions are assigned. Geographic points remain WGS84 `[longitude, latitude]`, in decimal degrees, without altitude. OSM XML supplies separately named `lat` and `lon` attributes; those are explicitly reordered in our tables. API bounding boxes use `[west, south, east, north]`.

The coordinator owns composition, overlaps and registration. Preserve source metres `(x east, y up, z south)` and `UE(X,Y,Z) = 100 * source(z,x,y)`, applied once. Do not import the older sector documents' north/east research axes, assign a new origin, reuse the additive reference's offset as a placement, or assume the earlier 6 x 4.5 km envelope. Existing canonical content, IDs, theatre, districts and destinations remain unchanged and must not be relabeled as either slice.

## Research record and sources

Every external source in this ledger was accessed **October 11, 2026**. Fresh research used read-only HTTP retrieval and OSM XML parsing in memory. No external extracts, photographs or PDFs were written to the repository; this Markdown is the only authored output. Map data: copyright OpenStreetMap contributors, available under ODbL; see https://www.openstreetmap.org/copyright and L01 for licensing.

### Text and local/primary sources

| ID | Exact source URL | Date / inspected evidence | Supports and limits |
| --- | --- | --- | --- |
| T01 | https://homef.org/2017/07/26/fishnet-dialogue-at-makoko/ | Published July 26, 2017; participant organization's report of July 24, 2017 dialogue; text inspected | HOMEF distinguishes Makoko on water and on land; reports fisher/trader testimony and purchases of fish elsewhere when local catches decline. Its spellings include `AkoAgbon` and `Appolo`; do not silently treat these as surveyed boundary labels. Statements about pollution/species are testimony, not sampled water chemistry or a present fisheries census. |
| T02 | https://homef.org/2024/03/14/culture-of-survival-at-makoko/ | Published March 14, 2024; report of February 28, 2024 event; text inspected | HOMEF/CEE-HOPE event at a Makoko resource centre, community stories, fish trade, poetry and performance. Residents describe seasonal water, raised structures, electricity/fuel difficulties and school journeys affected by flooding. No surveyed address, route widths, exact shore edge or permission to reproduce poems, recordings or participants. |
| T03 | https://homef.org/2026/01/20/makoko-eviction-is-not-development/ | Published January 20, 2026; organization describes its visit to affected residents; text inspected | Explicitly describes homes on land and stilts and recent demolition/displacement. Reports resumed operations December 23, 2025 and damage observed by its visit. Claims/counts are attributed to HOMEF, not independently verified parcel inventory. No October 2026 surviving-building map. |
| T04 | https://www.theguardian.com/cities/2016/feb/23/makoko-lagos-danger-ingenuity-floating-slum | Published February 23, 2016; modified February 3, 2020; Tolu Ogunlesi's local reporting; text and captions inspected, not image pixels | Describes a small canoe boarding threshold, wooden waterfront structures, verandas, fish smoking, trade, boat/home building, multiple languages and land/water communities. Useful historical everyday-life evidence, not current route certification. Its historic floating-school passage must not be represented as standing today. Avoid adopting its sensational headline language. |
| T05 | https://www.nleworks.com/case/makoko-floating-school/ | Undated current project page; preview event explicitly March 2, 2013; text and outbound link labels inspected | Primary designer acknowledges original floating school's collapse and links subsequent iterations. Project heading `2011` is not an architectural date for surrounding houses. Page's scaling-up status is not evidence of a replacement built in Makoko. Photograph credits name NLE, Iwan Baan and George Osodi; no reuse permission established. |
| T06 | https://www.theguardian.com/cities/2016/jun/10/makoko-floating-school-collapse-lagos-nigeria-slum-water | Published June 10, 2016; modified October 19, 2022; Cynthia Okoroafor reporting a visit two days after collapse; text/captions inspected | Reports collapse at 10:30 on the preceding Tuesday, consistent with June 7, 2016, and students relocated beforehand. Conflicting maintenance explanations are not resolved here. The 2016 remaining platform is not evidence that ruins/platform survive in 2026. |
| T07 | https://lagferry.gov.ng/our-terminals/ | Operator page, publication/update date not supplied; text inspected | Names **Ipakodo Terminal, Ikorodu**, and route relationships to Five Cowries/Falomo-Ikoyi and Ebute Ero/Marina-CMS. Supports real transport relationships, not present departures or precise boarding gates. A separate Mile 2 location line appears inconsistent; not used for geography. |
| T08 | https://lagferry.gov.ng/safety-and-regulation/ | Operator page, publication/update date not supplied; text inspected | Published guidance includes lifejackets, manifests, capacity limits and no night/poor-visibility travel. This is operator guidance, not a legal opinion or confirmation of service on October 11. Use it to avoid proposing an all-night real LAGFERRY route. |
| T09 | https://lagferry.gov.ng/ | Undated operator home page; text inspected | Lists Ikorodu connections including CMS, Falomo/VI and Liverpool/Apapa. Route availability/schedules remain unverified. No need to import its marketing statistics or branding. |
| T10 | https://www.nleworks.com/publication/makoko-research-heinrich-boll-stiftung/ | Title identifies 2012 research; page published February 7, 2013; text inspected | Historical design research and floating-building proposals. Its broad canoe-only/water-housing language must not override T01/T03's land/water distinction; population and future-city projections are not adopted. Download links were discovered, not read. |
| T11 | https://www.thisdaylive.com/2026/01/16/rights-groups-condemn-demolition-of-makoko-call-for-immediate-halt/ | URL dated January 16, 2026; retrieved page displays relative age rather than exact article date; Michael Olugbode; text inspected | Local report of coalition allegations that demolition began January 5, 2026. T03 describes earlier resumption on December 23, 2025. These dates may refer to different phases; no resolution invented. Confirms the need for a current-condition audit, not exact damage at our selected nodes. |
| L01 | https://www.openstreetmap.org/copyright | Current licensing text inspected | OSM database is ODbL; credit OpenStreetMap contributors, identify license and assess derivative-database obligations before distributing extracted geometry. OSM documentation's license is separate. Open data does not license imagery used by mappers, real logos or private property access. |

### OSM acquisition ledger

Successful map requests:

- **O00-M:** https://www.openstreetmap.org/api/0.6/map?bbox=3.388,6.492,3.394,6.498
- **O00-M2:** https://www.openstreetmap.org/api/0.6/map?bbox=3.388,6.497,3.392,6.500
- **O00-I:** https://www.openstreetmap.org/api/0.6/map?bbox=3.486,6.601,3.492,6.607

Individual full-way responses below supplied the reference-node chains, named coordinates and metadata used here. These are **feature edit timestamps, not imagery dates or dates of construction**. A way can extend outside the requested map bounding box; map response inclusion alone does not delimit our slice. Tables intentionally retain some older residential geometry rather than pretend an October 2026 fetch makes it contemporary observation.

| ID | Exact full-way URL / OSM way | Retrieved version and edit time UTC | Tags used |
| --- | --- | --- | --- |
| O01 | https://www.openstreetmap.org/api/0.6/way/217793801/full | v13; 2026-05-07T06:18:21Z | `name=Makoko Road`, `highway=residential`, `surface=unpaved`, `source=Bing` |
| O02 | https://www.openstreetmap.org/api/0.6/way/217793822/full | v10; 2026-05-07T06:53:30Z | `name=Ramotu Street`, `highway=residential`, `surface=unpaved` |
| O03 | https://www.openstreetmap.org/api/0.6/way/746004745/full | v3; 2024-06-06T00:05:34Z | unnamed `highway=path`, `surface=unpaved` |
| O04 | https://www.openstreetmap.org/api/0.6/way/745650052/full | v5; 2024-06-06T00:05:34Z | unnamed `highway=path`, `surface=unpaved` |
| O05 | https://www.openstreetmap.org/api/0.6/way/745487971/full | v4; 2024-06-06T00:05:34Z | unnamed `highway=path`, `surface=unpaved` |
| O06 | https://www.openstreetmap.org/api/0.6/way/407972992/full | v22; 2024-06-13T15:21:57Z | unnamed `waterway=canal`, `boat=yes`; a discovery anchor, not pedestrian geometry |
| O07 | https://www.openstreetmap.org/api/0.6/way/538323260/full | v10; 2025-04-07T10:54:32Z | `name=Beach Road`, `highway=primary`, `ref=F270`, `oneway=no` |
| O08 | https://www.openstreetmap.org/api/0.6/way/144135082/full | v12; 2018-05-02T06:01:30Z | unnamed `highway=primary`, `ref=F270`, `oneway=no`; western approach, not proven named Beach Road |
| O09 | https://www.openstreetmap.org/api/0.6/way/448344877/full | v2; 2018-05-02T06:50:47Z | unnamed `highway=residential`; landward branch |
| O10 | https://www.openstreetmap.org/api/0.6/way/448344876/full | v3; 2022-07-22T08:50:33Z | unnamed `highway=residential`; terminal-side branch |
| O11 | https://www.openstreetmap.org/api/0.6/way/538323252/full | v2; 2018-05-02T04:06:13Z | F270 `highway=primary`, `junction=roundabout`, `oneway=yes` |
| O12 | https://www.openstreetmap.org/api/0.6/way/1080483434/full | v1; 2022-07-22T08:50:33Z | unnamed `highway=service`; terminal approach |
| O13 | https://www.openstreetmap.org/api/0.6/way/538323256/full | v4; 2022-07-22T08:50:33Z | unnamed `highway=service`; service loop |
| O14 | https://www.openstreetmap.org/api/0.6/way/659370442/full | v3; 2025-05-26T15:14:29Z | `name=Ikorodu Ferry Terminal`, `amenity=ferry_terminal`, `public_transport=station`, `ferry=yes` |
| O15 | https://www.openstreetmap.org/api/0.6/way/1389859755/full | v1; 2025-05-26T15:14:29Z | unnamed `highway=footway` near terminal; no proven service-road connection |

Additional fresh O00-I discovery: unnamed residential ways `158052990` and `283053946` meet O08/O07 at nodes `1577180250` and `2869727141` respectively. Cross street `283053948` joins those residential ways at nodes `2869727147` and `2869727144`. Full responses were inspected at https://www.openstreetmap.org/api/0.6/way/158052990/full (v10, 2018-04-10T12:46:32Z), https://www.openstreetmap.org/api/0.6/way/283053946/full (v5, 2025-11-17T17:57:17Z), and https://www.openstreetmap.org/api/0.6/way/283053948/full (v1, 2014-05-20T05:49:19Z). This is an **outside-slice continuation lead**, not a third neighborhood or a compressed street inserted between terminal frontage buildings. No invented names are assigned to these streets.

### Geographic source anchors

Each pin below is an OSM node obtained through its parent full-way response. Exact node URLs provide reinspection targets; separate node-version responses were not fetched. Precision is OSM precision, not surveyed accuracy. None is a proposed engine location.

| Slice / role | Exact OSM node URL | WGS84 `[longitude, latitude]` | Parent/source |
| --- | --- | --- | --- |
| Makoko landward reference | https://www.openstreetmap.org/node/1300349922 | `[3.3893627, 6.4976739]` | O01 |
| Makoko loop mouth A | https://www.openstreetmap.org/node/6981647402 | `[3.3895175, 6.4978911]` | O01/O03 |
| Makoko path join A | https://www.openstreetmap.org/node/6979331163 | `[3.3898106, 6.4978371]` | O03/O04 |
| Makoko path join B | https://www.openstreetmap.org/node/9546057442 | `[3.3895587, 6.4979424]` | O04/O05 |
| Makoko loop mouth B | https://www.openstreetmap.org/node/2270990831 | `[3.3894675, 6.4980570]` | O01/O05 |
| Makoko Road / Ramotu join | https://www.openstreetmap.org/node/6977529471 | `[3.3895732, 6.4992089]` | O01/O02; beyond compact loop |
| Makoko canal discovery pin | https://www.openstreetmap.org/node/6981755494 | `[3.3914869, 6.4984914]` | O06; not a surveyed boarding point |
| Ipakodo main/landward branch | https://www.openstreetmap.org/node/1577179182 | `[3.4873391, 6.6026453]` | O08/O09 |
| Ipakodo main/terminal-side branch | https://www.openstreetmap.org/node/1577180581 | `[3.4879947, 6.6026444]` | O08/O10 |
| Ipakodo service entry | https://www.openstreetmap.org/node/5210744479 | `[3.4861089, 6.6031086]` | O11/O12 |
| Ipakodo service loop join | https://www.openstreetmap.org/node/9903652937 | `[3.4862790, 6.6021268]` | O12/O13 |
| Ipakodo terminal boundary sample | https://www.openstreetmap.org/node/127831615 | `[3.4866471, 6.6014758]` | O14; not gate/deck/centroid |
| Ipakodo footway sample | https://www.openstreetmap.org/node/12864935853 | `[3.4865266, 6.6017325]` | O15 |
| F270 / named Beach Road continuation | https://www.openstreetmap.org/node/1577180250 | `[3.4899630, 6.6032805]` | O07/O08; outside selected frontage |

### Imagery, rights and evidence gaps

- **No photographic pixels, street-view panorama, aerial image or video frames were visually inspected in this pass.** Text/captions are not visual review. T04's photo captions credit Andrew Esiebo/The Guardian and drone footage credits Adekanmi Ojuri/Oxford Brookes; capture dates are unspecified. T06 describes a June 9, 2016 visit but that does not date every embedded photo. T05's photo credits do not establish capture dates. O01's `source=Bing` does not identify acquisition date, license or current pavement condition.
- Exact discovered but **unread** primary PDF target: https://gallery.mailchimp.com/75e591db8b22aa2c51b6c7278/files/161130_Makoko_Floating_School_FAQs_and_appendices.02.01.pdf . Filename suggests a date; contents/date were not verified. No claims here rely on it. Historical download https://www.ng.boell.org/downloads/Makoko-Solution.pdf was linked by T10 but not inspected.
- Candidate HBS page https://ng.boell.org/en/2014/03/03/makoko-iwaya-waterfront-regeneration-plan returned 404; search endpoints also failed. Do not cite the guessed page as read evidence. Overpass request to `https://overpass-api.de/api/interpreter` returned 406; direct OSM map/full-way requests supplied usable evidence. An initial larger OSM bbox exceeded its node limit; reduced requests succeeded. Search-engine retrieval was challenge-blocked or irrelevant, so findings rely on directly read publishers, not search snippets. Local Nigerian Guardian demolition page https://guardian.ng/news/nigeria/metro/makoko-residents-protest-demolitions-at-lagos-assembly/ returned 403 and is not relied upon.
- T01-T11 are not open asset licenses. Create original meshes/textures/recordings later or obtain explicit permission; do not turn article photos into textures, reproduce poems/music/dialogue, copy NGO/operator logos or recreate identifiable private interiors. Historical architectural drawings are not production assets merely because a download link exists.
- Current surviving structures, public shore access, community boundaries, household arrangements, roof types, exact drain routes, water quality, vegetation/species, boat operating rights, street-name pronunciation and accessibility need further dated/local review. No parcel, survey, DEM, bathymetry or measured acoustic sample acquired.
- Open-repository review read root `AGENTS.md`, `CONTEXT.md`, `docs/world-building/README.md`, coordinator `README.md`, and `output/cribtest11-visible-world-2026-10-11/parent-geography/lagos-geographic-authoring-v1.json`. That last document is estimated additive reference, not authority to place these slices. No installed library/API integration was needed.

## Slice A: Makoko Road footpath loop and shore-water threshold

### Bounded reference and connected streets

**F:** O01 is named Makoko Road and O02 Ramotu Street. O03-O05 are unnamed paths, not roads with missing guessed names. T01 and T03 explicitly distinguish land and water homes. T02 supports ordinary schooling, a resource centre and fish trade within Makoko but does not locate them at our pins.

**M:** O01 meets O03 at node `6981647402`; O03 meets O04 at `6979331163`; O04 meets O05 at `9546057442`; O05 returns to O01 at `2270990831`. These shared nodes support a compact mapped loop. O01 continues to O02 at `6977529471`, outside that loop. The landward continuation and two independent path mouths prevent the reference from becoming one picturesque canal cul-de-sac. O03 also branches into way `746004746` at `6981647401` in O00-M2; its ultimate access/destination was not audited and is not a required escape route.

**M limitation:** O06 provides a nearby canal reference farther east, but none of O03-O05 shares a retrieved node with O06. This research does **not** establish a direct loop-to-canal landing, boardwalk or navigable channel to open lagoon. Footpath `surface=unpaved` does not identify decking or foundations. No real community sub-boundary is attached to the selected loop.

**P:** Keep the ordinary land street, two side-path mouths and inhabited inner frontage. Attach a separately authored public shore spur to the loop, with a small boarding platform and a second dry public return. Documentation handles `A-main`, `A-loop-a`, `A-loop-b`, `A-shore` and `A-return` are internal labels, not real street names or canonical IDs. Do not build an endless continuous promenade through real waterfront homes. The composite deliberately folds the **unverified intervening yards/paths between loop and canal**; it does not claim those spaces are empty or publicly traversable.

### Authored walking sequence: 230 m

The following is **P, entirely design distance**, not a traced OSM itinerary. Reference nodes orient the source study only. Preserve building/person/vehicle scale; only the relation between land loop and water threshold is compressed.

| Cumulative design distance | Proposed walk/frontage | Proposed dimensions and usable destination |
| --- | --- | --- |
| 0-45 m | `A-main`: enter on land; pass a closed residential gate, open repair counter and shaded pause before first side-path mouth | Main shared surface 5.5 m; separate 1.8 m clear pedestrian continuation. Fictional repair room 3 x 4 m, door clear opening 1.0 m. Keep counter customers out of passing route. |
| 45-80 m | `A-loop-a`: turn off main street; household threshold, laundry yard glimpse, then net/boat-repair workspace | Public passage 2.2 m; repair yard 6 x 7 m, open work strip separate from circulation. No automatic right to enter the adjoining home. |
| 80-110 m | Inner loop: two room fronts, quiet sitting recess and small unnamed food/fish counter; remain on dry public path | Clear path 2.0 m with 1.8 m passing/turning pockets; fictional counter room 3 x 3 m. A second, better-maintained rendered wall beside a timber workshop varies means and maintenance without estimating incomes. |
| 110-155 m | `A-shore`: authored spur opens to water at a supervised boarding nook; repair skiff remains stationary beside working canoe | Spur 2.4 m; landing 5 x 7 m; protected circulation 1.8 m; optional canoe berth envelope 2 x 7 m. Water/deck height and berth depth unset. Accessible dry overlook reaches the same water view. |
| 155-195 m | `A-return`: take independent public dry edge back toward loop mouth B; hear home conversation behind screens, see a short supply delivery | Return 2.2 m with 1.8 m unobstructed minimum. One optional invited courtyard 7 x 8 m, not part of this through-route. |
| 195-230 m | `A-loop-b` to main street: regain a familiar repair-counter roof silhouette and leave landward; water remains audible behind, not visible everywhere | Mouth 3.0 m, main continuation 1.8 m clear pedestrian width. Branch to Ramotu-inspired continuation is a coordinator connector, not additional walking length. |

Accessible route design target: step-free dry circulation; ramps where needed target no steeper than 1:20, crossfall target no more than 1:50, and level turning space 1.8 x 1.8 m. These are gameplay/accessibility **design targets**, not Nigerian-code certification or a claim about existing Makoko pathways. Height/gradient cannot be finalized without authored-level decisions. Do not flatten the entire area to sea level to satisfy them.

### Ordinary rooms, courtyards and local activity

**F/historical basis:** T04 describes wooden waterfront homes and verandas, fish smoking/trading and boat/home-building work. T01/T02 describe supply constraints: some traders buy fish from elsewhere, and flood/transport conditions affect education. Neither establishes our exact room plan or a constant productive fishing channel beside every home.

**P production brief:** Use original, unnamed businesses and fictional households. A land household has a shared sitting/cooking yard, screened living doorway, washing line and small repair shelf. A different household has a tidier rendered frontage and a quiet upper-room glimpse; another has a timber workshop beside a modest room. Material/building counts are art proposals, not local socioeconomic survey. A waterside dwelling may have a shaded veranda, living room and secure storage, but no identifiable family's plan is reproduced.

Usable local destinations: buy an ordinary meal at the unnamed counter; deliver a wrapped repair part to the workshop; converse in the publicly open seating recess; meet a fictional reading group in a small community room; view the water from dry ground; optionally board an operated canoe when permitted. Fish smoking occurs in a supervised work yard away from the clear passage; bought fish arrives in covered containers rather than all fish spawning as a nearshore catch. This reflects reported supply constraints without assigning measured pollution to our fictional berth. The community room is **not** CEE-HOPE's real premises or an unverified school replacement.

Morning proposals: deliveries, net preparation, school-age characters traveling with carers on safe paths. Midday: repair, domestic work, reading, lunch, rest and ordinary discussion. Evening: packing counters, maintaining boats, conversations and a smaller public flow. No fixed assignment of fishing, trade or domestic tasks by gender. Childhood is ordinary life, not a stunt/chase mechanic or tourist spectacle. Public invitation, open hours and household privacy are explicit destinations states; private courtyards are never mandatory shortcuts.

### Developer Asset Checklist

All checklist entries are **P asset needs**, not installed assets or source-confirmed inventory. Labels reflect categories for later development, not authorization to implement now.

| Category | Required authored assets | Reference/guardrail |
| --- | --- | --- |
| Meshes / architecture | Modular land room-fronts, timber workshop bay, alternate rendered-wall frontage, screened living doorway, veranda, household gate, public corner recess, repair counter, community room | T04 supports timber waterfront structures; exact land-wall forms and room layouts are authored. Mix repair/maintenance states; no universal decay kit. |
| Meshes / architecture | Separate courtyard shells, work benches, laundry frames, seating, covered containers, net racks, hand tools and safe fish-smoking enclosure | Ordinary destinations remain larger than their prop clusters; objects cannot close both path mouths. No source-derived private floor plans. |
| Meshes / water edge | Canoe and repair-skiff variants, original landing, mooring fittings, edge protection, removable work panels and dry overlook | Canoe basis T04; platform geometry is P. No MFS I school, A-frame stand-in, surviving-ruin claim or imported Venice/Bruges replacement. |
| Materials / textures / shaders | Original weathered and maintained timber, painted render, metal fixings, cloth screens, shaded floor, selected roof alternatives and limited work soot | Roof type, color distribution and real material condition unverified. Author original textures; do not sample article photography. No blanket rust/grime overlay. |
| Materials / textures / shaders | Separate dry-path and wet-edge surfaces; water reflection/ripple, localized splash and stains, smoke confined to work activity | OSM unpaved tag is dated map evidence, not current soil/deck identification. Water hue/clarity is P, not water-quality measurement; no tropical turquoise preset or universal sewage effect. |
| Systems / spawners | Route-aware households, repair clients, traders, carers/students, reading-group visitors, canoe operator; time/state-controlled deliveries | Spawn to actual tasks, not repeated poverty vignettes. Hard cap occupancy at the boarding area; retain free dry return. |
| Systems / spawners | Public/private doorway state, invited-room access, covered-fish delivery, repair progression, accessible route occupancy and local event gating | Closed/private interiors stay destinations only by permission; staff gate water activity during unsafe authored conditions. |
| Systems / spawners | Local rain shelter, runoff masks, smoke/audio occlusion, protected water edge and optional boarding state | Shore heights, bathymetry, real flood calibration and collision remain future authoring work. No implicit new engine subsystem requested. |

### Audio, day/night and weather

**F:** T04/T01 support boat work and trade; T02 records seasonal water/flooding testimony. No acoustic levels, tide series, rainfall intensity or local hourly schedule measured.

**P:** Main street has intermittent light vehicle/passenger conversation; turning into the loop attenuates engines behind walls. Repair sounds come from a single visible bench; canoe/paddle and mooring contact localize at the landing. Fish-yard work and household talk occupy different small zones. Caption navigation-relevant sounds; never require hearing alone to find the return. Record original speech with local performers later; T04's multilingual description does not license invented Egun/Yoruba dialogue or a random mix of accents. No continuous generic drums, amplified worship or commercial songs as an ambient identity shortcut.

Morning practicals give way to shaded daylight; evening has varied household/work lights, not universal blackout. Night keeps safe dry route cues and quieter activity, with no guaranteed canoe transport. Rain changes roof/puddle sounds, brings laundry and supplies under shelter and reduces exposed workshop activity. Keep drain crossings readable and protected. Seasonal-water change is a separately authored state: raise/close only the optional berth connection with a dry overlook alternative; do not use unmeasured rise values or flood every home. No simulated monsoon, ocean surf or invented tidal amplitude.

### Fictional navigation-disrupting event: landing panel replacement

**P:** During a routine repair, the operator moves a canoe to a side berth and replaces one landing approach panel. A marked work area blocks **only `A-shore` beyond its dry junction**, not the main street, both loop mouths or `A-return`. This is fictional maintenance, not a replay of collapse, eviction or a real reported accident.

Multiple independent exits remain available: return through `A-loop-a` to main; continue through `A-return` and `A-loop-b` to main; pause at the dry overlook then leave via either public dry branch. A waterborne character already at the side berth can disembark onto the authored dry return with operator assistance. No jumping a gap, swimming, private yard trespass or payment is required to escape. Design work-zone length 8 m, visible before the spur commitment; uninterrupted bypass width 1.8 m. The only unavailable destination is boarding from that repaired panel.

Clearing condition: operator completes actual panel replacement and removes work tools; navigation obstruction clears after the work state completes and the berth circulation is clear. No mandatory player task or ceremonial wait. On event interruption, stay on safe dry circulation and keep the blocked optional segment closed until its recovery state is valid. Prevent incident/spawn placement if two independent land exits cannot remain usable. Timings are authored, not estimates of local labor.

### Connector briefs

| Connector | Source-side / destination-side reference | Real relationship and omitted geography | Proposed join and operational conditions |
| --- | --- | --- | --- |
| A-in: land approach | O01 at node `1300349922` -> O01 at node `6981647402` | **M:** same mapped road, compact local continuation. No claim about exact frontage or crossings between points; preserve rather than replace those ordinary spaces. | **P:** street into first loop mouth; walking and low-speed delivery, public fictional circulation only. 1.8 m pedestrian clearance, 5.5 m shared surface. Match traffic intermittency and visible drainage across join; levels/drain invert unknown. No highway ramp from Third Mainland. |
| A-out: landward continuation | O01 loop mouth B `2270990831` -> O01/O02 junction `6977529471` | **M:** same Makoko Road reaches Ramotu Street. Intervening inhabited street is not empty land. No surveyed external Yaba/Iwaya route supplied. | **P:** coordinator may compress only this onward journey, retaining a residential interval and street-scale turn. Walk/light delivery; 1.8 m clear foot continuation; no public access inferred through homes. Match dry runoff path and reduce shore audio progressively. Endpoint beyond Ramotu stays unassigned. |
| A-water: optional composite handoff | O06 canal pin `6981755494` as source-area reference -> O14 terminal boundary node `127831615` as destination-area reference | **F/M:** both are Lagos lagoon-context references; **no retrieved shared water-route relation or verified Makoko-Ipakodo service**. O06 is not a real departure gate. Open water, working waterfronts and intervening neighborhoods are omitted, not declared empty. | **P only:** optional fictional operated transfer between an original Makoko-inspired landing and an original Ipakodo-inspired berth, not a named LAGFERRY service or direct street adjacency. Berth envelope 2 x 7 m, approach 1.8 m; draught, water clearance and deck heights unknown. Keep dry exit at both ends. Gate water movement for weather/visibility; water/audio continuity requires later review. Coordinator may decline this connector. |

Third Mainland is a context reference, not a walking bridge spur from this loop. No bridge abutment, deck level or driving junction is established here; do not insert one through the homes.

## Slice B: Ipakodo ferry approach and inhabited side streets

### Bounded reference and connected streets

**F:** T07 names Ipakodo Terminal, Ikorodu. O14 names Ikorodu Ferry Terminal. Treat their correspondence as **M**, a supported locality match, not proof that every gate or mapped polygon belongs to one unrestricted operator compound. O07 names Beach Road; O08, its connected western F270 approach, carries no retrieved street name. Do not rename O08 `TOS Benson Road`, `Oba Sekumade Road` or Beach Road without further evidence.

**M:** O08 connects to O07 at `1577180250`. Within the selected terminal-approach reference, side street O09 meets O08 at `1577179182` and O10 meets it at `1577180581`. O09 extends landward; O10 extends toward the terminal side. Their existence creates two different residential fronts, not a terminal plaza with decorative homes. Do not invent a connecting cross street between O09/O10: the retrieved chains do not establish one.

**M:** O12 attaches to roundabout O11 at `5210744479`, then joins service loop O13 at `9903652937`. O08 reaches the roundabout through the mapped F270 approaches; do not interpret the `oneway=yes` roundabout as bidirectional driving. O15 is a short mapped footway near O14, but its chain `12864935853 -> 12864935852 -> 12814051343` does **not** share a retrieved O12/O13 node. Terminal-boundary O14 does not prove a public promenade, free through-passage, a current gate or accessible ferry boarding.

**P:** Center this slice on a **terminal approach neighborhood**, with the two inhabited branch mouths, an ordinary main-road frontage interval and a public pedestrian continuation to an original ferry-boundary forecourt. The named Beach Road segment remains an eastward connector reference outside the short walk. Fold the longer road/service approach into the composite connector, not into undersized buildings. Internal handles `B-main`, `B-landward`, `B-terminal-side`, `B-public-link`, `B-forecourt` are production labels, not fabricated local names.

### Authored walking sequence: 260 m

This is **P design distance**, not the measured walk between the OSM nodes. In particular, `B-public-link` is an original connection; no real public link between O10 and terminal grounds was established.

| Cumulative design distance | Proposed walk/frontage | Proposed dimensions and usable destination |
| --- | --- | --- |
| 0-55 m | `B-main`: busier approach, unnamed meal counter, small parts shop, setback compound gate and visible first residential branch | Shared road 7.0 m, pedestrian clear path 2.0 m, protected crossing/refuge 2.0 m; no real traffic counts or road-width claim. Counter 3 x 4 m; parts room 4 x 4 m. |
| 55-100 m | `B-landward`: visit quieter compound frontage, clothing repair room and shaded sitting yard, then return by the same public branch | Branch shared surface 4.5 m with 1.8 m clear walking zone; invited courtyard 8 x 9 m, workshop 3 x 4 m. Out-and-return movement is included in the 45 m design distance; no hidden cross street. |
| 100-145 m | `B-main` to second branch: pass a more maintained compound, a modest rental-room frontage and delivery recess | Pedestrian zone 2.0 m; delivery bay 3 x 6 m outside it. Proposed mixed maintenance/privacy conditions, not a household-income estimate. |
| 145-205 m | `B-terminal-side` / `B-public-link`: repair bench and household supply delivery give way to a fictional public pedestrian forecourt connection | Branch 4.5 m shared surface, public link 2.4 m, weather shelter 4 x 6 m. No uninvited passage through compound grounds; no loading-yard shortcut. |
| 205-260 m | `B-forecourt`: arrival/departure seating, information/ticket destination, protected waterfront view; separate onward land exit returns to main approach | Forecourt 12 x 14 m, queue lane 1.8 m, public walking bypass 2.0 m; optional boarding vestibule 5 x 6 m. Terminal internal access controlled; dry seating/view available without boarding. Deck/water elevations unset. |

**P human-scale navigation:** First branch reads as quieter inhabited street; second branch retains homes before the transport threshold. A fictional repair-bench canopy, compound gate and forecourt shelter supply distinct silhouettes at standing and seated eye height. Avoid identical facade kits, a single straight monumental corridor or a wealthy promenade replacing service life. Courtyard entrances remain optional and permitted. No invented building heights or historical styles asserted.

Step-free public circulation target 1.8 m minimum, turning pockets 1.8 x 1.8 m, ramps target 1:20 maximum slope, crossfall target 1:50 maximum. These are **design values**, not real terminal accessibility certification. Preserve seats with backs/arms, shaded rests and a dry alternative to boarding. Never require a stair, ticket purchase, ferry availability or road sprint to finish the walk.

### Ordinary rooms, courtyards and local activity

**F/M:** T07-T09 support transport activity; O09/O10 establish mapped residential streets, not businesses or specific building materials. Everyday interiors below are authored, with less directly inspected local cultural/material evidence than Slice A. This asymmetry must remain visible in production metadata.

**P:** One compound has an inhabited sitting/washing yard, screened living rooms and a small sewing repair counter with permission-based access. Another has a painted, better-maintained wall/gate, a shaded shared landing and quiet residents. A rental-room frontage has a meal kiosk and covered household supplies; a repair yard services ordinary equipment, not an invented named marine business. Include both people arriving by public transport and people living/workshopping here. Commuter demand does not mean every resident is a commuter; formal terminal presence does not establish universal services or wealth.

Usable local destinations: collect a repaired garment, buy an ordinary meal, deliver household goods, sit with a resident by invitation, check a fictional ferry's boarding state, meet an arriving friend, or leave on land. A passenger completes boarding/manifest/lifejacket interactions only when actually traveling; do not add tutorial ceremony to sitting or walking past. No real operator ticket price or promise of current departure is specified.

Morning proposal: varied arrivals, shop opening and modest delivery flow. Midday: domestic errands and work continue between ferry pulses; some rooms stay closed. Afternoon: repairs collected, supplies unloaded outside pedestrian space, arriving passengers meet people already waiting. Night: households and a limited meal counter remain active while passenger-water operations close. The forecourt does not become an unrestricted cargo depot or continuous outdoor party.

### Developer Asset Checklist

All entries are **P**, including unverified architectural/material choices.

| Category | Required authored assets | Reference/guardrail |
| --- | --- | --- |
| Meshes / architecture | Two distinct residential branch sets, rental-room frontage, more maintained compound, optional courtyards, screened room doors and domestic service corner | O09/O10 support street structure only. Specific walls, roof forms and layouts need later local visual review; no architectural dates assigned. |
| Meshes / architecture | Meal counter, parts shelf, sewing/repair room, equipment work bay, delivery recess and public rest shelter | Unnamed fictional premises; no invented real business names/logos. Retain workshop/yard depth and quiet gaps. |
| Meshes / transport | Original terminal-boundary facade, controlled boarding opening, seats, manifest/ticket desk, original information sign, lifejacket racks and distinct public bypass | T07/T08 activity basis. Not a replica of current operator facilities. No real boarding gate claimed from polygon nodes. |
| Meshes / transport | Generic original passenger vessel, mooring/edge protection, weather shelter, service gate, pedestrian crossing/refuge and drains with guarded crossings | Vessel and deck scale require later authoring; no transport licensing, real fleet dimensions or deck height asserted. |
| Materials / textures / shaders | Original render/paint alternatives, maintained and repaired metal, cloth, floor finishes, selected roof alternatives, signage/visibility treatments | Material palette is P pending imagery; no universal luxury glass or universal corroded shed palette. |
| Materials / textures / shaders | Authored road/footway blend, separate wet footprints, localized runoff, berth water and sheltered dry zones | No OSM surface tag was used to assert O08 pavement. No invented flood depth, drainage capacity or tidal datum. |
| Systems / spawners | Residents, workshop customers, deliveries, meal-counter patrons, passenger arrival/departure pulses, staff and people meeting passengers | Residential and commuter tasks coexist; avoid crowd density inferred from marketing totals. All roles gender-inclusive. |
| Systems / spawners | Controlled boarding, weather/visibility closure, manifest/lifejacket state, public bypass and optional courtyard invitations | T08 is operator guidance, not guaranteed current service. Dry public exit survives closure; don't strand non-traveling people behind boarding gates. |
| Systems / spawners | Local delivery obstruction, accessible route reservation, seating occupancy, rain shelter, traffic/audio attenuation and recovery state | Keep crowd/vehicle queues out of both exits and the crossing refuge. No implementation performed. |

### Audio, day/night and weather

**F:** Operator sources support passenger/ferry operations; no audio recording, noise level or frequency measured. T08 states no night/poor-visibility travel; no departure timetable verified.

**P:** Main approach layers intermittent vehicles, conversation and opening shop shutters. Landward branch reduces engines through gates/walls; household talk remains localized. Terminal-side branch adds occasional tool work before a distinct arrival pulse at the forecourt. Vessel engines start/stop at a visible berth, not continuous roaring across both branches. Public address uses original, intelligible fictional departure/status lines with captions, not copied LAGFERRY announcements. No commercial music, religious sound or ethnic performance becomes a neighborhood-wide loop.

Daylight emphasizes shade under canopies and unobstructed sightlines to crossings. Evening domestic practicals differ from terminal work lights. Close passenger-water service at night in the proposed state; do not present that fictional timetable as operator information. Poor visibility stops boarding and preserves the meeting/rest/land destinations. Rain shelters queues without pushing them onto the through-path; puddles are localized, drains have continuous fictional outfalls only after coordinator height authoring. Lagoons do not receive Atlantic surf by default. No storm/tide/calibrated seasonal-level numbers supplied.

### Fictional navigation-disrupting event: delivery vehicle at the branch mouth

**P:** A supplier stops to unload household and workshop goods at `B-terminal-side`. The stopped vehicle and supervised unloading temporarily close the short **vehicle** entry and one direct pedestrian turn, while residents/transport users continue through marked public routes. This is ordinary fictional logistics, not a security raid or disaster.

Multiple exits: continue along `B-main` to the landward connector; use the independent `B-forecourt` land exit to the main/service approach; use the protected 2.0 m walking strip around the loading recess to reach the forecourt. No real O09/O10 cross-link is claimed, and no private compound opens automatically. People inside the second branch retain an authored public pedestrian outlet even while vehicle entry is closed. Keep the crossing refuge and forecourt bypass empty.

Design unloading recess 3 x 6 m; obstruction zone 9 m; bypass 2.0 m. Spawn the event only when all pedestrian alternatives are clear, prohibit simultaneous berth/land-exit blockage, and yield to seated/mobility-aid users. Clearing condition: actual goods moved into the receiving bay, doors secured, then vehicle leaves and barriers removed. Player intervention is optional; no payment or mandatory task to reopen the district. Interrupted unload resumes safely or remains in the recess without blocking escape.

### Connector briefs

| Connector | Source-side / destination-side reference | Real relationship and omitted geography | Proposed join and operational conditions |
| --- | --- | --- | --- |
| B-in: main approach | O07/O08 node `1577180250` -> O08/O09 node `1577179182` | **M:** mapped F270 continuation from named Beach Road into unnamed western approach. O10's mouth lies along this road too. Omitted frontage and junction approaches are inhabited/used spaces, not vacant buffer. | **P:** shorten onward journey outside this slice, preserve both branch mouths and ordinary frontage. Walking/local traffic/delivery; 2.0 m pedestrian clear zone, 7.0 m shared road design value. Respect mapped one-way topology where roundabout is retained; provide protected walking continuity. Drainage, kerbs and heights unknown; main-road audio fades into branches. |
| B-out: terminal public boundary | O12/O13 node `9903652937` -> O15 sample `12864935853` / O14 boundary | **M:** service road and near-terminal footway exist, but no retrieved shared node proves continuous public access. Gate, ramp, deck and internal rights unknown. Omitted service/terminal space must not be represented as unrestricted empty ground. | **P:** original forecourt/public link with two independent dry exits and controlled boarding destination. Walk/mobility aids, staff-only service vehicles separated; 2.0 m bypass and 1.8 m queue lane. Drain continuity and deck/water transitions remain unset. Traffic attenuates before passenger speech/berth engines. No invented real entrance. |
| B-water-out: sourced route relationship | T07's **Ipakodo Terminal, Ikorodu** / O14 locality -> T07's **Marina Terminal, CMS**, via its listed Ebute Ero relationship | **F:** operator lists this relationship. Destination OSM feature ID/berth/route polyline not retrieved; no street adjacency or schedule asserted. Intervening lagoon travel, docks, waterfronts and urban districts are omitted; Ebute Ero and Marina are not one gate. | **P:** coordinator may select a fictional time-compressed passenger transfer to the Island dossier's eventual approved berth reference. Walk/board/boat, supervised restricted boarding, public dry exit on both shores. No final destination position or deck clearance. Avoid night/poor-visibility service (T08); synchronize water state/audio and retain land alternative. Source naming mismatch in T07/T09 requires route confirmation before representing a real service itinerary. |
| B-water-in: optional Makoko-inspired arrival | O06 source-area canal -> O14 locality / original fictional secondary berth | No verified real Makoko-Ipakodo passenger route; see A-water. This is **P only**, not an incoming LAGFERRY connection. | Keep a distinct small-craft berth separate from the passenger queue; 1.8 m clear landing circulation design target. Staff permission, water/deck/drain unknowns and independent dry exit required. Coordinator can omit it without disabling either slice. |

## Shared cultural, navigation and composition guardrails

- Makoko is not wholly on water, and water-based work is not its residents' only activity. Keep inhabited land streets, domestic yards, ordinary rooms and non-water destinations visible. Do not place the collapsed original floating school as standing, substitute a later overseas iteration, or claim the 2016 debris survives today.
- January 2026 demolition/displacement reports are material current-condition evidence. Do not infer exact affected parcels at our nodes, portray ongoing homes as universally destroyed, or use real evictions as an entertainment obstruction. This is a fictional composite, not an October 2026 documentary reconstruction.
- Specific multilingual/cultural references have dated support only for Makoko. Commission pronunciation/dialogue/performance review with consenting local collaborators later. Do not borrow Makoko identity cues for Ipakodo, treat neighborhoods as one ethnicity, or assign labor/wealth by gender or accent.
- Use consent/privacy for likenesses, household stories, identifiable businesses/interiors and cultural performances. No tourists-with-cameras reward loop, exoticizing labels, poverty scoring or residents used as environmental decoration. Original signage may use sourced street names, but not fabricated real enterprises.
- Maintain orientation at seated and standing eye heights through distinct junction silhouettes, readable signs and captioned directional cues. Avoid depending on hue, hearing, jumping or narrow planks. Edge protection and predictable supported footing are P safety goals, not claims that real infrastructure has them.
- Never make a private yard, worship room, staffed boarding gate or paid boat the sole district exit. Events must leave multiple independent public dry exits; weather may suspend water activity without invalidating the inhabited street. Closures preserve rest/meeting destinations and explicit return paths.
- Preserve mixed maintenance, work and quiet intervals within each slice. Ipakodo's formal terminal is not a socioeconomic census; Makoko's housing conditions do not erase skill, culture, ordinary leisure or varied households. Don't combine all missing-service claims into a universal deprivation preset.
- No elevated highway or bridge is attached because water is nearby. Makoko's contextual Third Mainland reference does not make this loop a bridge endpoint. Lekki-Ikoyi is not the same crossing or a connector supplied by this dossier.
- Journeys between reference slices may be fictionalized; rooms, people, vehicles, passage widths, compounds and frontage depth stay human-scale. Keep omitted geography explicit in internal connector metadata. Fictional compression labeling belongs in this production dossier, not unsolicited tutorial text in product scenes.

## Acceptance and unresolved handoff

Completed: two bounded reference slices; fresh direct OSM node/way research with edit versions, IDs and coordinate order; primary/local textual evidence; separate fact/inference/proposal labels; 230 m and 260 m authored walks; ordinary interiors/courtyards/activity; categorized asset checklists; atmosphere/weather; one multi-exit event per slice; typed connector briefs and canonical-preservation contract.

Unresolved before geographic/visual acceptance: dated visual inspection, surviving Makoko structures after the 2025/2026 demolition phases, loop-to-water route/shore status, Ipakodo gate and footway continuity/public access, exact material/roof references, pronunciation/cultural review, permissions, measured heights/drainage/water conditions and accessible boarding. None prevents this conditional design dossier, but they prevent claiming surveyed reconstruction or production-ready geography.

Coordinator decisions: reconcile mainland/Island overlaps; decide whether to use the explicitly fictional Makoko-Ipakodo water connector; select Island receiving feature for the sourced Ipakodo route relationship; retain or compress onward land intervals; author heights/public routes; own all composition and placements. No fixed world envelope imposed.

Source/document inspection only. No builds, tests, lint, typechecks, server starts, further agents, engine/runtime/renderer/mobile/config edits, purchases, uploads, public messages or git commit. Packaging and What's New are not triggered by this documentation-only file. In-game/visual verification and implementation are not performed or claimed.
