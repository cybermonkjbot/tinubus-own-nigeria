# Sector 10 evidence register

October 11, 2026. Companion to the [Boladale–Church blueprint](sector-10-boladale-church.md). Raw downloads remain temporary; durable derivatives retain source IDs, versions, hashes and bounded observations. No photograph is imported as an asset.

## P01 — fresh OSM network and footprint sample

Discovery: `https://api.openstreetmap.org/api/0.6/way/93146072/full.json`.

Discovery SHA256: `8ffa810ff26ded5ffc8fb8e6e496336fdcbbc1d2860d8add9b23b50247827219`.

Area query: `https://api.openstreetmap.org/api/0.6/map.json?bbox=3.3440,6.5540,3.3505,6.5590`.

Area SHA256: `4026f2820aed6f705a2ff34bf6d1bfbb00bb4df660947a4bb5815a6247dd6c95`.

Attribution: © OpenStreetMap contributors; ODbL 1.0; reference `https://www.openstreetmap.org/copyright`. Contributor accounts are omitted; way/node IDs, versions, timestamps, node order, coordinates and tags are retained. Preserve applicable attribution/database obligations.

The [network](sector-10-network.geojson) contains sixteen complete, unclipped roads:

| Way / version | Source name | Important source tags |
| --- | --- | --- |
| 93146072 / 17 | Boladale Street | tertiary; unpaved |
| 1343514141 / 5 | Boladale Street | tertiary; unpaved; oneway=yes |
| 1547601494 / 3 | Church Street | secondary; paving_stones; maxspeed=40 |
| 496652063 / 7 | Church Street | secondary; asphalt; maxspeed=40 |
| 1547601493 / 2 | Oshodi Road | secondary; paving_stones; oneway=no |
| 5153761 / 23 | Oshodi Road | secondary; paving_stones; oneway=no |
| 441859099 / 8 | Unnamed | tertiary; unpaved |
| 441859097 / 6 | Unnamed | tertiary; unpaved; oneway=yes |
| 134140049 / 11 | Kunle Akinosi Street | tertiary; unpaved; maxspeed=40 |
| 441859090 / 4 | Adewale Crescent | residential; asphalt; maxspeed=30 |
| 496652587 / 2 | Sabiolegbe Street | residential; no surface tag |
| 496763688 / 4 | Ganikale Close | residential; unpaved; maxspeed=5 |
| 496652589 / 9 | Seinde Callisto Street | tertiary; paved |
| 1105198603 / 7 | Unnamed | secondary; paving_stones |
| 589001493 / 9 | Oshodi Road | secondary; asphalt |
| 1314547202 / 1 | Unnamed | residential |

Tags are source statements, not verified current carriageway condition, speed signage or legal access. October 3, 2026 edits on Church/Oshodi do not establish an October 3 ground inspection. Maxspeed values are retained raw, not used as approved simulation limits.

The [connections](sector-10-connections.json) contain 25 shared-node pairs at 13 unique nodes and two closed source-node circuits: 989.1 m eastern and 378.9 m inner. Explicit one-way links are traversed forward; legal turns, width, public pedestrian continuity and present operation are unverified. Lengths use Haversine radius 6,371,008.8 m, summing unrounded segments before 0.1 m rounding. Precision does not imply survey accuracy.

Ten overlap records across S01/S02/S03/S07/S08/S09 match geometry, node order, way/node versions, timestamps and way tags. They represent repeated feature/file occurrences, not ten unique roads. One future runtime owner is required for each feature.

The [building sample](sector-10-buildings.geojson) has 29 closed rings selected by arithmetic mean of non-closing vertices within approximate 65 m of [3.3466968,6.5557704]. Approximate scales: 110590 m/degree longitude, 111200 latitude. This is not full-ring containment, cadastral precision or a comprehensive building inventory. All carry building=yes. Twelve carry source=microsoft/BuildingFootprints: 1276039188, 1276039190, 1276039192, 1276039195, 1276039208, 1276039209, 1276039210, 1276039212, 1276039243, 1276039257, 1276039264 and 1276039271. Source tags do not verify current outlines or heights. No sampled business tags supply a present storefront identity. Ring closure is not comprehensive polygon validity.

## P02 — Boladale maintenance photographs, September 2024 claims

Original: `https://www.instagram.com/p/DAnZ8Kvtr0V/`, account titilola_cole_oso, displayed publication **October 2, 2024**; page times 2024-10-02T08:51:46Z and 08:51:45Z.

Caption identifies maintenance on Boladale Street in Mafoluku, Oshodi-Isolo LGA. Two main-post images were downloaded and directly inspected; related-post thumbnails were excluded.

| Image | Visible capture/location stamp | SHA256 |
| --- | --- | --- |
| First | September 6, 2024, 23:55:27; 6.555854 N, 3.346615 E | 5c772d11a2a4b08ce91eeb5ff6e24be51a74f59ad393260c408fb54e87d3087a |
| Second | September 7, 2024, 01:53:49; 6.555828 N, 3.346661 E | 251bfe34d16ea96312b8b45c70dc7ff54897e4199eacd5e3350269eb92098d38 |

Both stamps identify Boladale Street, Oshodi-Isolo, Lagos, Nigeria. Embedded text is a source claim, not independently authenticated GPS/capture metadata. Haversine separation from node 1475182789 is approximately 13.0 m and 7.5 m respectively; proximity does not establish camera bearing, facade identity or a surveyed match.

First view: night roadwork setting, rough/light exposed road material, parked vehicles, alternating dark/light facade panels, windows, overhead wires, poles/metal supports, gate/perimeter wall and bright point lights. Second: worker using a long-handled tool over dark aggregate/bituminous-looking material, reflective bands on blue workwear, roadside channel/kerb edge and related facade/wire/light context. No chemical material identification, channel depth or full-road completion follows.

Visible business lettering is not reproduced or assigned to a footprint. No open reuse licence was established. Signed image delivery addresses are not durable publication identifiers; the original post and hashes identify the evidence. These photos do not authorize current paving, a repair crew spawn or exact building reconstruction. [Imagery ledger](sector-10-imagery.json).

## P03 — fresh Commons search and image inspection

Search: `https://commons.wikimedia.org/w/api.php?action=query&list=geosearch&gscoord=6.5563%7C3.3468&gsradius=500&gsnamespace=6&gslimit=100&format=json`.

Search SHA256: `95d9253b1b1daa83210a2f665a8b6b6cc510bff98af0580183a8727fabe19a11`.

Metadata: `https://commons.wikimedia.org/w/api.php?action=query&pageids=88431486%7C128961602%7C88431487%7C63613778%7C63613779&prop=imageinfo&iiprop=url%7Cextmetadata&format=json`.

Metadata SHA256: `521ec59edb087384c9f97166e5a62513f670e40b2bfacc90a9d83a8ad95c1a48`.

Five results; four originals downloaded and directly inspected. Full page/original URLs, creator, licence, coordinates, dates, hashes and observations are in the [imagery ledger](sector-10-imagery.json).

- **88431486, Danfo bus struggles.jpg:** Funsho116, CC BY-SA 4.0, source date February 11, 2016, [3.348643,6.556468]. Close yellow minibuses with black stripes and worn panels; pedestrian foreground. No model/route/current fleet certainty.
- **88431487, A queue of danfo buses.jpg:** Funsho116, CC BY-SA 4.0, source date February 11, 2016 at 18:10:26, [3.349168,6.556448]. Yellow minibus row, pedestrians carrying large loads and stacked trays on head, low-rise/window/roof forms and poles/wires. Exact goods and a permanent/current terminal are not established.
- **63613778, Tapiya3.jpg:** Tapiya, CC BY-SA 4.0, source date August 3, 2017, [3.350206,6.555306]. Portrait dominates; limited background includes blue roadside barrier, pedestrians, vehicles and light poles. Marginal street context, not a likeness asset.
- **63613779, Tapiya1.jpg:** Tapiya, CC BY-SA 4.0, same source coordinates. DateTimeOriginal literally contains **Tapiya 2**, not a date. Capture date remains null. Do not copy the companion image's date or substitute upload date. Similar limited roadside context behind a portrait.
- **128961602, Traditional Selfie.jpg:** Jeremyida002, CC0, source date February 22, 2023. Metadata only; not downloaded/visually inspected. Description concerns attire, not street geometry; excluded from architectural and generalized cultural evidence. Metadata coordinates are rounded relative to geosearch output.

Dates/geotags are source claims, not verified capture time or camera registration. Source licences do not by themselves resolve third-party marks, likeness or voice rights. No image is a present-day street survey or game import.

## P04 — historical repair reports and misleading discovery dates

Realnews original, publisher byline, **August 15, 2017**, footer 9:00 GMT:

`https://realnewsmagazine.net/ariyoh-launches-operation-fix-oshodi-isolo/`

Original article read in the collaborative browser. It reports a council programme for road repairs, drain clearing and refuse removal, with repairs begun at Boladale junction and other named roads. Its soil-cement/crushed-stone description concerns Seinde Callisto around Charity, not a specified Boladale patch. The relative wording yesterday/Sunday conflicts with Tuesday publication; no exact work date is derived. No article photograph was inspected. This establishes a dated report, not present condition, engineering dimensions or a current council policy.

Facebook original resolved and its caption/date read:

`https://www.facebook.com/100044297780697/videos/lagos-state-nurtw-boss-commences-rehabilitation-of-boladale-street-iretioluwa-st/523879738533069/?locale=ms_MY&_rdr`

Displayed date **22 Mac 2020**, interpreted as **March 22, 2020**, not a current work programme. Discovery snippets suggested recent/2026 timing; original-page date takes precedence. Caption claims repair funding by Akinsanya and names Boladale, Iretioluwa and Suwebatu, with local-association/Adewale Crescent context. Those attribution claims were not independently corroborated. The video loaded but **was not reviewed**; no footage-derived observation is used. No current motor park, dues regime or worker placement follows.

Fresh archive query: `https://akinwunmiambode.com/wp-json/wp/v2/posts?search=Boladale&per_page=50` returned an empty list. SHA256: `4f53cda18c2baa0c0354bb5f9a3ecbe5ed12ab4d8e11ba873c2f11161202b945`. This query's empty result does not establish that no archive evidence exists.

## P05 — fresh coarse elevation samples

Full query and results: [terrain context](sector-10-terrain-context.json).

Raw SHA256: `3254ae80325f5d5ec28a2db187fa0f69639392c00116f00d1382f1040b17a080`.

Open Topo Data public srtm30m API, explicit nearest interpolation, status OK. Eight values in query order: 23,21,26,21,21,23,20,21 m. Product documentation carried from S06 K05: `https://www.opentopodata.org/datasets/srtm/`, `https://www.opentopodata.org/api/`, USGS SRTM 1 Arc-Second Global. Provider-stated SRTMv3, nominal one arc-second/about 30 m spacing, WGS84 horizontal coordinates and EGM96 metre elevations. Mission acquisition February 11–22, 2000; fill-source dates may differ.

Query positions are not documented cell centres or present ground benchmarks. No original raster tile, local accuracy assessment, ground/structure separation or municipal grade/drain survey was obtained. Samples are excluded from collision, road-grade and flood calibration. Engine vertical datum and canonical registration remain null.

## P06 — carried lexical reference; no new local consultation

Mimi Werna, British Council Voices, **April 22, 2020**, carried from S04 H05:

`https://www.britishcouncil.org/voices-magazine/nigerian-pidgin-words-phrases`.

Supports narrow meanings of How far / Abeg / No wahala. It does not establish Boladale-specific dialogue, accents, speech frequency, occupational identity or voice performance. No new interviews, consented sound recordings or cultural approval were obtained. Vehicle regulations and street-vendor eligibility remain unverified rather than inferred from historical photographs.

## Readiness and review scope

The package preserves mapped topology, two candidate circuits, historical material/transport observations and conditional simulation/asset/event design. It does not establish contemporary access, survey precision, approved asset rights, implemented systems or playable content. Web search returned no useful displayed results; original browser pages and public APIs supplied the evidence above.

Source-integrity review is limited to derivative consistency: coordinates, source node order/tags/versions, ring selection, shared pairs, route continuity/direction/lengths, overlap records, raster query matches, image hashes/metadata and local document links. It is not application testing, renderer validation or a site survey.

Read-only review on October 11, 2026 matched all 45 retained source features, the 29-ring selection and twelve footprint source tags; all 25 shared pairs at thirteen nodes; both circuit sequences, one-way flags and lengths; ten earlier-sector overlap records; eight terrain samples; and six inspected-image hashes. Commons raw dates, coordinates and licences matched, including the malformed date retained as unresolved. The two image-stamp proximity calculations reproduced. Sixty-five local links across the README and new prose resolved. A planar segment scan found no non-shared-node intersections among the sixteen selected road ways; this does not certify unselected crossings, vertical geometry or legal turns.
