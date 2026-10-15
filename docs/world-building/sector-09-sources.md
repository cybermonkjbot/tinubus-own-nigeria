# Sector 09 evidence register

October 11, 2026. Companion to the [Akin-Lawanson–Seinde Callisto blueprint](sector-09-akin-lawanson-seinde-callisto.md). Raw evidence downloads remain temporary; durable derivatives retain IDs, versions and provenance. No photograph is imported as a game asset.

## N01 — fresh network and footprints

Endpoint: `https://api.openstreetmap.org/api/0.6/map.json?bbox=3.3388,6.5503,3.3456,6.5561`.

Raw SHA256: `f8720ecf285e5aeeef421ba32bc9e9221dda7987cc9f411ffb492e82fa24a39f`.

Attribution: © OpenStreetMap contributors; ODbL 1.0; licence reference `https://www.openstreetmap.org/copyright`. Contributor accounts are omitted, while way/node IDs, versions, timestamps and tags are retained. Preserve applicable attribution/database obligations.

[Network](sector-09-network.geojson): 20 roads and three pedestrian ways, complete and unclipped. [Connections](sector-09-connections.json): 24 road pairs, two pedestrian-continuity pairs, one mixed interface and 22 distinct shared nodes. Source way class/name does not certify current carriageway function, condition or legal turn permissions.

The three pedestrian ways 560546953/954/955 are version 1, timestamp February 11, 2018. Step ways retain layer=1 and traffic_calming=table; the middle footway retains bridge=yes/layer=1. Source tags do not supply stair dimensions, a raised-road-table location, clearance, ramp/lift or structural approval. Node tags highway=crossing at stair feet remain evidence, not automatic graph attachment.

Four calculated line intersections between bridge way 560546954 and parallel road ways have no shared nodes. Each is stored as a non-junction. Southern endpoint 5405812679 is not a node of nearby tertiary way 370658034. Approximate separation to its nearest selected segment is 0.4 m; calculation projects to a segment using 110590/111200 m-per-degree scale factors, then reports Haversine distance. This is not a surveyed gap or a connector. No snapping is performed.

The 300.0 m frontage trace is geometric only: 161.2 m Akin-Lawanson, 132.7 m reverse traversal of one-way primary 134140052, and 6.1 m service approach. It is explicitly not a vehicle route or verified footpath. The pedestrian chain is 94.0 m horizontally (18.9/56.4/18.7 m by leg), not its three-dimensional travel length. Seinde Callisto is 655.9 m. Haversine radius 6,371,008.8 m; totals sum unrounded segment lengths before rounding to 0.1 m. Decimal precision does not imply accuracy.

S08 overlaps 93156409, 441859086 and 134140052 match geometry, node order, node versions and way version. No new runtime ownership or canonical registration is established.

[Footprints](sector-09-buildings.geojson): 17 closed rings, selected by arithmetic mean of non-closing vertices within approximate 65 m of [3.3416220,6.5526154]. Scales 110590 m/degree longitude and 111200 latitude; not cadastral/full-ring containment or comprehensive inventory. Sixteen have only building=yes. Way 686906483 v1, April 30, 2019, also has religious-use/name tags. Those tags are not contemporary verification of the named institution, worship schedule, signage or building use. Ring closure is not comprehensive polygon validity, and no heights are established.

## N02 — Charity footbridge report and photograph

Victor Madu, The Guardian Nigeria, **March 29, 2023, 4:52 AM WAT**:

`https://guardian.ng/news/lagos-relief-as-fg-re-opens-charity-footbridge-to-pedestrians/`

The original article was read in the collaborative browser. It reports reopening after seven months of closure while also describing ongoing reconstruction and concerns about remaining work. “Reopened” and “complete/safe today” must not be conflated. It associates the bridge with the Oshodi-Apapa expressway, NAFRC and Command Day Secondary School. It provides no surveyed coordinate registration to OSM way 560546954; that association remains a candidate.

Original inspected image: `https://cdn.guardian.ng/wp-content/uploads/2023/03/Footbridge-1424x802.jpg`.

SHA256: `fa3c2b116ddc867be125e1583f41d8fde9613073627e2eedca8d01a3131b139e`.

The caption credits Victor Madu and describes reconstruction “yesterday,” relative to the March 29 article. No independent March 28 capture timestamp is established. Visible observations: green canopy covering and frame, vertical railing members, concrete-looking deck/support surfaces, scaffold poles and cones. The image does not establish material chemistry, measurable geometry, present repairs, load capacity or access. No open reuse licence was established. Record in [imagery](sector-09-imagery.json); no reconstruction scaffold, named person, institution interior or exact camera pose is placed.

## N03 — Seinde maintenance archive

Fresh search: `https://akinwunmiambode.com/wp-json/wp/v2/posts?search=Seinde&per_page=30`.

Raw SHA256: `c23c978d3b762811570348eb51c96a0762f453e4755e6b955a6647df63736e4d`.

Relevant original archive post, **March 11, 2016**, reporting March 7–11 works:

`https://akinwunmiambode.com/brief-report-on-the-activities-of-lagos-state-public-works-corporation-from-monday-march-7th-11th-friday-march-2016/`

Full post content was read from the public publisher API. It explicitly lists Seinde Callisto Street, Oshodi-Isolo, among maintenance/palliative works. No exact section, method, dimensions or current material follows. The post's photographs identify other roads (including Alimosho and Magodo), so none is reassigned to Seinde. Other search matches concern people with Seinde-containing names, not the street.

## N04 — fresh Commons search and historical reinspection

Search: `https://commons.wikimedia.org/w/api.php?action=query&list=geosearch&gscoord=6.5524%7C3.3421&gsradius=500&gsnamespace=6&gslimit=100&format=json`.

SHA256: `5369efe2cbda3aeac07808e19170edeb2b4e8f9204ceca1af609dd232d9ec933`.

Metadata: `https://commons.wikimedia.org/w/api.php?action=query&pageids=35456655%7C63897481%7C63897485%7C63897482&prop=imageinfo&iiprop=url%7Cextmetadata&format=json`.

Metadata SHA256: `4695ca21627df240faea89067a3034b8646ce22df386247f14e48a7a6749a48e`.

The four results are the same historic originals inspected for S08. All were freshly downloaded and visually reinspected; each image hash matches S08. Exact URLs, hashes, coordinates, source dates, creators and licences are retained in [imagery](sector-09-imagery.json).

- Page 35456655, 2011 Mushin Lagos Nigeria 6352734402.jpg: source date November 16, 2011, [3.340166,6.551000], zmtomako, CC BY-SA 2.0. Roadside margins, vehicles/people, low-rise forms, a distant pedestrian bridge and poles are visible. The Mushin title and location are not independently reconciled; no camera bearing/bridge identity inferred.
- Page 63897481, Bus conductor at work.jpg: source date November 6, 2017, [3.341744,6.551589], Tapiya, CC BY-SA 4.0. Bus-panel wear and doorway/person visible; no verified make, present route or operating pattern.
- Pages 63897482/63897485, Government Employees task with clearing gutters.jpg / Protecting the Environment.jpg: source date November 6, 2017, [3.341780,6.551590], Tapiya, CC BY-SA 4.0. Reflective vests, tools/containers, kerbside water and divider; a tanker and yellow minibus in one view. These are one scene four seconds apart by metadata, not independent geographical corroboration.

Captions do not independently establish employer, permission, water contamination, flood cause, climate season or present activity. Reuse rights, people and third-party markings remain separate from the recorded source licences. No image is a current survey or is imported into the game.

## N05 — coarse elevation context

Fresh full query and ten results are in [terrain context](sector-09-terrain-context.json). Raw SHA256: `0c04cc4bc4f5e19d79a473e798374bd85778c48c963ac709ab7645c3938ebeee`. Open Topo Data srtm30m, explicit nearest interpolation, status OK. Values: 20,21,28,28,28,28,25,29,24,23 m.

Product documentation carried from S06 K05: `https://www.opentopodata.org/datasets/srtm/`, `https://www.opentopodata.org/api/`, and USGS SRTM 1 Arc-Second Global. Provider-stated SRTMv3, one arc-second/about 30 m spacing, WGS84 coordinates, EGM96 metre elevations. Mission February 11–22, 2000; fill-source dates may differ.

Query positions are not documented cell centres or present-day bare-earth benchmarks. Equal bridge-end values and differences across stairs do not supply rise, deck profile or road clearance. No original raster tile, ground/structure separation or local accuracy assessment was obtained. Samples are excluded from collision, road grade, flood and bridge-clearance calibration; engine datum and registration remain null.

## N06 — later bridge lead rejected after original-page review

A search result from FIJ Nigeria mentioned a closed pedestrian bridge and surfaced Charity in surrounding search text. The linked original was read:

`https://fij.ng/article/lagosians-risk-their-lives-on-expressway-as-govt-abandons-dilapidated-pedestrian-bridge-for-years/`

Daniel Ojukwu, **February 28, 2026**. The article locates its subject near **Aswani Junction on Oshodi-Isolo expressway**, not this sector's Charity candidate. It is excluded from the current-status determination here. Its photographs are not imported or assigned to this bridge. Search snippets, repost dates and comments are not sufficient to establish identity.

No verified 2026 completion/access record for the selected crossing was obtained. Property, interior-shop and directory search leads near Seinde were not used to assign businesses, heights or signage to footprints. Web search supplied no usable displayed results; original browser pages and public APIs supplied the evidence above.

## N07 — language reference and consultation limit

Mimi Werna, British Council Voices, April 22, 2020, carried from S04 H05: `https://www.britishcouncil.org/voices-magazine/nigerian-pidgin-words-phrases`.

Only narrow lexical meanings for How far / Abeg / No wahala carry forward. New local consultation, dialect/performance review and sound recordings were not obtained. Dated accounts of bridge access do not establish universal local behaviour.

## Source-integrity review

Read-only review on October 11, 2026 matched all 40 retained features to raw coordinates, complete node order, tags, versions and timestamps, including node-level tags. All 27 typed shared-node pairs at 22 nodes and three route records reproduced, including the reverse one-way flag and rounded lengths. Four non-junction intersections matched source segments; the southern endpoint remained unconnected and its reported proximity distance reproduced. The 17-ring selection and three S08 overlaps matched. Ten terrain samples matched source nodes, query order and raw results. Five image hashes and the Commons date/coordinate/licence records matched saved evidence; local Markdown links resolved. This establishes derivative consistency, not current access, survey accuracy, native integration or playable acceptance.

## Readiness

| Layer | State |
| --- | --- |
| Network | 20 roads, three pedestrian ways; four non-junction crossings; southern connector unresolved |
| Route access | Reverse one-way frontage trace explicitly non-drivable; no verified end-to-end footpath |
| Footprints | 17 source rings; heights/current use/entrances unresolved |
| Imagery | Five originals inspected; historical and unregistered |
| Terrain | Ten coarse samples; no vertical survey |
| Culture/audio | Lexical reference and historic activity/access accounts; local review pending |
| Manifest/event | Authored conditional specification |
| Native/visual/playable | Not implemented or accepted by this package |
