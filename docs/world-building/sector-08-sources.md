# Sector 08 evidence register

October 11, 2026. Companion to the [Afariogun blueprint](sector-08-afariogun-olusoji-jubril.md). Source keys M01–M07 distinguish mapped records, source claims and bounded observations. Raw downloads remain temporary; versioned derivatives retain provenance.

## M01 — fresh OpenStreetMap geometry

Discovery endpoint: `https://api.openstreetmap.org/api/0.6/way/5153764/full.json`.

Discovery SHA256: `3524c0dc4685bd5faece72a9d836906c5325d41ffff71e4ebff778a7fc9281a4`.

Sector query: `https://api.openstreetmap.org/api/0.6/map.json?bbox=3.3365,6.5488,3.3430,6.5569`.

Raw map SHA256: `908b3f174fe36b08453a02214db8b3d4fb54e6c2821f2bc541ddea3cd2ad1f0a`.

Attribution: © OpenStreetMap contributors; ODbL 1.0. Licence reference: `https://www.openstreetmap.org/copyright`. Preserve applicable attribution/database obligations. Contributor account details are omitted; IDs, way/node versions, timestamps and tags remain in the derivatives.

[Roads](sector-08-roads.geojson) retain 19 complete selected ways, including Afariogun 5153764 v15, edited May 12, 2026, tagged tertiary/unpaved. Edits do not establish field inspection dates. The southern boundary way 134140052 v13 is primary/oneway=yes; do not conflate every carriageway bearing the same name or invent cross-median access. Names and classifications remain mapped records, not certified signage or traffic policy.

[Connections](sector-08-connections.json) retain 28 shared-node pairs at 26 distinct nodes, an 863.9 m spine, closed circuits of 714.2 and 635.5 m, and the 557.3 m alternate versus 78.2 m direct event segment. Haversine lengths use radius 6,371,008.8 m, sum unrounded segment lengths and round the final total to 0.1 m. This precision supports reproduction, not measured accuracy.

Only selected shared source nodes form candidate connections. Geometric crossings alone do not create road junctions. Route closure is not direction/access approval. The Oshodi Road overlap with S07 matches geometry, complete node order, node versions and way version; it needs one future runtime owner.

[Buildings](sector-08-buildings.geojson) retain 34 closed rings, all tagged building=yes. Selection: arithmetic mean of non-closing ring vertices within approximate 65 m of [3.3400597,6.5542291], scales 110590 m/degree longitude and 111200 m/degree latitude. It is not a cadastral buffer, full-ring containment or comprehensive block inventory. No heights, uses, household identities or entrances are established.

## M02 — council's historical repair account

Original public post: `https://www.instagram.com/p/C09W_TCIlbB/`, account `otunba_kehinde_almoroofoloyede`, **December 17, 2023**. The visible caption is credited to the media office of the Oshodi-Isolo local-government chairman. Browser time elements show December 17, 2023 at approximately 15:12 UTC; capture time of any media is not established.

The original caption was read, not just the search snippet. It identifies Afariogun Road within Afariogun ward as linking Oshodi Road and the Oshodi-Apapa Expressway, reports completed repairs to bad portions, and describes drivers avoiding those portions and causing queues, particularly in rain. This is a historical account by the commissioning office, not independent engineering acceptance or present condition evidence.

No exact repair polygon, width, method, lane count or current traffic volume is supplied. No post media was available in the inspected main-post image list; profile and related-post images were excluded. A fetched HTML response has SHA256 `5247acf4b90448568d78b2ce02d9aea8caa77fad87dad68643e6588b4652dd7c`; caption/date verification came from the rendered original page, not an assumption that this response is a full archive.

The current OSM unpaved tag and this older localized repair claim do not settle today's material. Both are retained with their limits. The fictional repair event does not reproduce the council, its workers or a specific job.

## M03 — fresh open photographic references

Commons geosearch: `https://commons.wikimedia.org/w/api.php?action=query&list=geosearch&gscoord=6.5534%7C3.3395&gsradius=600&gsnamespace=6&gslimit=100&format=json`.

Geosearch SHA256: `777d04ae7f48be0b83edca937bfb2aab75da62950741b54e73ea5e38a9edb7ae`.

Metadata request: `https://commons.wikimedia.org/w/api.php?action=query&pageids=35456655%7C63897481%7C63897485%7C63897482&prop=imageinfo&iiprop=url%7Cextmetadata&format=json`.

Metadata SHA256: `4695ca21627df240faea89067a3034b8646ce22df386247f14e48a7a6749a48e`.

All four originals were downloaded and directly inspected. Their exact URLs, hashes, source-claimed coordinates/dates, creators and licence references are in the [imagery ledger](sector-08-imagery.json).

| Commons file | Source-claimed date and position | Creator / source licence | Bounded visual evidence |
| --- | --- | --- | --- |
| 2011 Mushin Lagos Nigeria 6352734402.jpg | November 16, 2011, 08:15; [3.340166,6.551000] | zmtomako; CC BY-SA 2.0; Flickr origin | Roadway, uneven roadside margin, low-rise structures, pedestrians, motorcycles/minibuses, distant pedestrian bridge, light pole and utility supports |
| Bus conductor at work.jpg | November 6, 2017, 11:15:51; [3.341744,6.551589] | Tapiya; CC BY-SA 4.0 | Worn bus side panel, doorway and person at aperture; no verified bus make, route or current behaviour |
| Government Employees task with clearing gutters.jpg | November 6, 2017, 11:42:11; [3.341780,6.551590] | Tapiya; CC BY-SA 4.0 | People with tools/containers and reflective vests beside standing water, kerb and road divider |
| Protecting the Environment.jpg | November 6, 2017, 11:42:15; [3.341780,6.551590] | Tapiya; CC BY-SA 4.0 | Same cleaning scene; yellow minibus and large tanker visible behind |

The title/category “Mushin” and the 2011 geotag are not independently reconciled. Do not use a file title to redefine the sector, or infer a camera bearing by matching a distant bridge visually. The two cleaning images are one scene at essentially the same time and coordinate, not independent location corroboration. No exact camera registration or current infrastructure continuity was established.

Captions do not independently verify employment, a licence to work in traffic, water quality, a flood cause or the identity of an operator. Haze in a photograph does not establish Harmattan. These references support historic objects/material relationships, not current placement. Source licences are recorded; attribution/share-alike and depicted third-party rights need review before any reuse. No original image, likeness, logo or clothing mark is imported into the game.

## M04 — dated residential/workplace context, including disagreement

Paul Adunwoke, The Guardian Nigeria, **February 23, 2020, 5:06 AM WAT**:

`https://guardian.ng/sunday-magazine/afariogun-residents-in-oshodi-cry-out-over-bakery-fumes-noise/`

Original article read in the collaborative browser. Direct curl retrieval returned HTTP 403; no restriction was bypassed and no local HTML archive is claimed.

The reporter describes housing near Afariogun/Sadiku, including shared-room arrangements and darkened exteriors. Residents alleged smoke and mechanical-noise disturbance; the bakery operator disputed major aspects, describing changed equipment and operating practices. The blueprint preserves this disagreement. It uses only the broader design question of work and domestic space sharing a boundary, not allegations as established present facts.

No named operator, resident, health history, address or building is placed. No medical causation, current nuisance, precise room plan, equipment type or present operating schedule is inferred. No photograph or measured acoustic/emissions record was acquired from this article. The report cannot identify any of our 34 sampled footprints.

## M05 — coarse historical elevation context

Full query, eight coordinates and results: [terrain context](sector-08-terrain-context.json). Raw response SHA256 `9c869e86cd1b5c8e1ec25dc14be4c582e5189cad2320df3536e718f303871a8c`. Status OK, no null results; values 18, 22, 20, 17, 20, 18, 16 and 13 m.

Provider/product documentation carried from Sector 06 K05: `https://www.opentopodata.org/datasets/srtm/`, `https://www.opentopodata.org/api/`, and USGS SRTM 1 Arc-Second Global. The provider states SRTMv3, one arc-second/about 30 m spacing, WGS84 horizontal coordinates and EGM96 metre elevations. Radar mission February 11–22, 2000; fill-source dates may differ.

Nearest-neighbour was explicit. Returned locations repeat query coordinates, not independently documented cell centres. No underlying raster tile, local accuracy assessment, bare-earth separation or measured road/drain profile was obtained. Elevations remain excluded from collision, flood and road-grade calibration. Canonical registration and engine vertical datum are null.

## M06 — excluded same-name evidence

Fresh archive search: `https://akinwunmiambode.com/wp-json/wp/v2/posts?search=Afariogun&per_page=50`.

SHA256: `f11af1452780ae7084fd7c994061fa1d6a7cde6810622cb55a97507d7bf47785`.

Returned posts 13755 (October 22, 2016) and 14216 (November 4, 2016) concern works lists explicitly naming **Asa Afariogun Street, Ajao Estate**. Their list text was read; neither locates work on the selected Afariogun spine. They are excluded from this sector's surface/drainage claims. Search overviews that combine both names are not evidence of adjacency.

Other rental, directory, election-page and business-launch search results were not used to assign real establishments, residents, interiors or operations to source polygons. Web search returned no usable displayed results; collaborative-browser discovery and original-page reading plus public APIs supplied this pass's evidence.

## M07 — carried-forward lexical reference

Mimi Werna, British Council Voices, April 22, 2020; see Sector 04 H05: `https://www.britishcouncil.org/voices-magazine/nigerian-pidgin-words-phrases`.

Only basic meanings of “How far?”, “Abeg” and “No wahala” are carried forward. This pass adds dated work/residential accounts and historic activity photographs, not consultation, dialect validation, field audio or performance consent.

## Source-integrity review

Read-only review on October 11, 2026 matched all 53 retained features to the raw map: complete geometry, tags, node order, versions and timestamps. The 34-ring representative-point selection reproduced. All 28 shared-node pairs at 26 nodes and five route records reproduced, including both closed circuits and their rounded lengths. The S07 overlap matched its recorded geometry and version fields. Eight elevation samples matched raw results, query order and source nodes. Four image hashes, dates, coordinates and licence fields matched the saved originals/metadata. Raw map, terrain and Commons response hashes matched their records, and local Markdown links resolved. This establishes derivative consistency, not geographic survey accuracy, current access, native integration or playable acceptance.

## Readiness

| Layer | State |
| --- | --- |
| Horizontal topology | 19 complete roads, 28 pairs at 26 nodes, two candidate circuits |
| Footprints | 34 rings; envelopes, heights, use and entrances unresolved |
| Imagery | Four historic open originals inspected; present condition and camera registration unresolved |
| Terrain | Eight coarse samples; no road/drain survey |
| Culture/audio | Dated accounts and narrow lexical reference; local review and recordings pending |
| Manifest/event | Authored conditional specification |
| Assets/native/visual/playable | Not implemented or accepted by this package |
