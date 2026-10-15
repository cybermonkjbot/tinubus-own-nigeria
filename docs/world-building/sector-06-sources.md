# Sector 06 evidence register

Research date: October 11, 2026. Raw downloads are temporary; durable geometry retains source IDs, versions and provenance. Source labels K01–K05 refer to the companion blueprint. Reference imagery is not shipped as game art.

## K01 — fresh OpenStreetMap network and building sample

Endpoint: `https://api.openstreetmap.org/api/0.6/map.json?bbox=3.3280,6.5540,3.3360,6.5605`

Raw SHA256: `503b0837e6420f3a8727a624f288f0e70cb821926052918105646a210665a841`.

Derivatives: [roads](sector-06-roads.geojson), [footprints](sector-06-buildings.geojson), [connections](sector-06-connections.json). Attribution: © OpenStreetMap contributors, ODbL 1.0; licence reference `https://www.openstreetmap.org/copyright`. Retain applicable attribution/database obligations when reusing derivatives. Account metadata is omitted; tags, way/node identifiers, versions and timestamps remain.

Fourteen full ways are selected for the two circuits and their connections. Complete ways extend outside the query box. The network has 23 shared-node pairs at fourteen distinct nodes, including three same-road joins. It is not a complete local road inventory; no turning restriction, stop or access permission has been established. Preserve unnamed links 535191773 and 93156426 rather than assign names from nearby reports.

Haversine polyline distances use radius 6,371,008.8 m. Sum before rounding to 0.1 m: primary circuit 446.0 m, southern circuit 800.4 m, event alternate 294.4 m. Individual rounded legs can differ from a rounded total by 0.1 m. These are source-line calculations, not surveyed walking/driving distances.

Building selection uses an even-odd point-in-polygon calculation on the arithmetic mean of each ring's non-closing vertices. Boundary nodes: 4395533108 → 1073317441 → 1073319531 → 4395533109 → 4395533108. It follows primary-loop centreline geometry, not cadastral parcel boundaries. Whole-ring containment is not required. Forty selected closed ways have building=yes and version 1, without heights or uses. Thirty-nine carry timestamp 2024-04-23T16:52:12Z; one, way 1275841033, carries 2024-04-24T20:21:33Z. These are edit dates, not construction or image-capture dates.

The broader extract contains some building:levels tags outside this sample. They are not transferred to untagged buildings. Neither complete geometry nor a closed ring establishes comprehensive polygon validity or legal boundaries.

S05 overlap comparison matches geometry, node order, node versions and way versions for 496763689, 535191771, 535191773 and 441859117. One future runtime owner is required per feature. No `waterway` element was found in this extract; this does not establish absence of drains. No measured terrain, altitude or world registration is supplied.

Saved-file readback matched all 54 retained road/building features to raw coordinates, node order, node versions, way versions and tags. The 23 recorded connection pairs share their stated nodes, and both candidate circuits close in sequence. Nine terrain records match the queried OSM coordinates and raw elevation response; local document links resolve. This is source-integrity inspection, not survey, game testing or visual acceptance.

## K02 — Mukandasi video publication and inspected appearance

Lagos Reporters, Instagram:

`https://www.instagram.com/reel/DaAUzCfIRCc/`

Original post read in the collaborative browser. Explicit date attributes establish **June 25, 2026**: caption time 2026-06-25T10:12:50.000Z and post time 2026-06-25T10:13:10.000Z. Publication date is not independently verified capture date.

The caption places the report on Alhaji Mukandasi Street, Mafoluku, Oshodi-Isolo. Its statements about construction age, commissioning authority and collapse are publisher claims, not independently verified engineering facts. No claim about cause, responsibility, materials compliance or current officials is adopted.

Matching Facebook publication:

`https://www.facebook.com/lagosreportersnews/videos/residents-lament-as-mukandasi-street-constructed-less-than-2-years-ago-by-oshodi/1730788821605069/`

Facebook's inspected visible date was June 25 without a year. Its linked thumbnail was downloaded and directly viewed. SHA256: `557a0c750286ecde2aa19706fa68499abfd625e838e89483e2c65e62d9dbc4ed`. The corresponding Instagram video reports duration approximately 13.63 s; sampled paused frames around 7 s and 12 s were inspected in the browser. This is sampled visual inspection, not a complete audiovisual analysis or field recording.

Bounded observations: patterned block paving with uneven/missing portions and dusty/exposed areas; dark compact van and yellow/orange vehicles; painted frontages, a metal roof, railings, umbrellas and containers/bundles beside the street. Do not identify container contents, businesses, vehicle models, routes, residents or structural dimensions from these views. Publisher portraits, text panels and watermarks are editorial overlays, not environmental signage.

Coordinates, camera heading, precise street subchain, measured relief, capture date and reuse rights remain unresolved. The footage cannot overwrite every unpaved tag or supply exact height/collision data. It is a single publisher chain across two platforms, not two independent reports. No commenters' accusations are used.

## K03 — historical Baderin Link publication

Account: otunba_kehinde_almoroofoloyede, Instagram:

`https://www.instagram.com/reel/CVveDe2DRfE/`

Visible post date **November 1, 2021**, supported by post time 2021-11-01T18:11:40.000Z and caption time 2021-11-01T18:11:42.000Z. Its caption describes rehabilitation of Baderin Street and other streets in Mafoluku. The sampled street frame is overlaid Baderin Link / Mafoluku Oshodi by the publisher. This label does not establish equivalence to OSM Baderin way 535191771 or the anonymous return link.

A paused frame at 5 s was inspected, with media readyState 4 and seeking=false subsequently confirmed. It shows regular block paving, an open road-edge drainage channel, small frontage activity/umbrellas and a taller building at the far end. The video duration is approximately 117.73 s; it was not reviewed in full and no interview transcript/audio conclusion is claimed.

This is an interested-party publication, not an independent survey or completion certificate. Exact camera coordinates, capture date, drainage connection, building identity/height, present condition and reuse rights are unresolved. The 2021 view and K02's 2026 view are not a registered before-and-after pair. No photo/video file from K03 is bundled with the package.

## K04 — carried-forward language and adjacent context

The British Council lexical reference previously read in Sector 04 supports the basic usage of How far?, Abeg and No wahala, but not local pronunciation/performance. See [Sector 04 H05](sector-04-sources.md), Mimi Werna, April 22, 2020, `https://www.britishcouncil.org/voices-magazine/nigerian-pidgin-words-phrases`.

Sector 05's September 13, 2021 Nation maintenance article includes Baderin in its list; see [Sector 05 J02](sector-05-sources.md). It supplies historical context, not a located material/condition update. These carried-forward references are labelled as such; no new consultation or field audio is claimed.

## K05 — coarse elevation context, not a street survey

Provider dataset documentation: `https://www.opentopodata.org/datasets/srtm/`, downloaded SHA256 `91aa01652b1cd1209e29f2d46a7be481752dbeeb580d16de3e23269f76d3c104`.

Provider API documentation: `https://www.opentopodata.org/api/`, SHA256 `579891344ee1d20af998095295c6d3090a278a20272189181329da50d064bc7a`.

Both documents were read. The dataset page identifies the public service's SRTM version 3. API documentation defines WGS84 latitude/longitude input, dataset-native elevation units/datum, selectable nearest/bilinear/cubic interpolation and null/no-data behaviour. This query explicitly uses nearest interpolation and does not request path resampling. Query dates are not terrain acquisition dates.

The full nine-location query URL, response hash and exact results are retained in [terrain context](sector-06-terrain-context.json). Raw response SHA256: `74b9778a18d6c24f9a8adfd7bc6316a49e448611d3dd4efe5157baede8dab326`. Status was OK and all nine values were non-null. In blueprint control-table order, elevations are 25, 26, 27, 22, 24, 27, 29, 27 and 26 m. These are raster samples returned at query coordinates, not surveyed road-level benchmarks or explicitly reported cell-centre coordinates.

USGS EROS product reference, published July 30, 2018:

`https://www.usgs.gov/centers/eros/science/usgs-eros-archive-digital-elevation-shuttle-radar-topography-mission-srtm-1`

Read in the collaborative browser after a transient navigation timeout. Direct download returned 403 and is not used as page evidence. The product reference specifies WGS84 horizontal datum, EGM96 vertical datum, metres, one-arc-second spacing (approximately 30 m), and the radar mission dates February 11–22, 2000. It describes void filling using processing and other elevation sources. No source pixel's acquisition, fill status, local error or current bare-earth ground level was established here.

This is a third-party API delivery of a described public elevation product, not a locally validated original raster tile. Underlying tiles were not downloaded; no account registration or installation occurred. The sparse values are suitable only as a research lead. They must not drive collision, water depth, road grade, kerb/drain detail or foundations. Source EGM96 is recorded separately from the still-null engine vertical datum and canonical-world registration.

## Fresh open-repository searches and excluded matches

Commons query:

`https://commons.wikimedia.org/w/api.php?action=query&list=geosearch&gscoord=6.5583%7C3.3320&gsradius=600&gsnamespace=6&gslimit=100&format=json`

Returned an empty geosearch list. SHA256: `5b9a3e49850ea05b1a20ec8bf5d745292fb9384f6f1901c18eff7d1e7bc54e1e`. This is not proof that no open imagery exists, and the social-media references above are not assumed open-licensed.

Publisher archive APIs:

- `https://akinwunmiambode.com/wp-json/wp/v2/posts?search=Baderin&per_page=20` — SHA256 `2721527e3e9d92ff668786653e3d0e2d913e56135cdd9d51d96b6f1b156c244f`. The returned match concerns a person's longer name, not this street.
- `https://akinwunmiambode.com/wp-json/wp/v2/posts?search=Makinde&per_page=20` — SHA256 `2d0968986963109e62211a68e83a56df20b87be6019b5bf097c8b49c10391445`. Matches concern people and a road in the Lagos–Ogun boundary works context, not a verified Oshodi street project.

Web search returned no displayed usable results; collaborative browser discovery and direct API retrieval provided evidence. Google snippets about refuse complaints, business addresses and other roads are not used to populate the scene. Related-post text on social sites can appear in search results for unrelated posts; original caption and date were read before acceptance. AI-generated summaries and reader comments are excluded.

No current route-specific vehicle-policy source, municipal survey or local cultural consultation was obtained. The absence of local projection executables during this pass did not authorize installation; projected coordinates remain uncalculated rather than approximated into the game.

## Readiness

| Layer | Status |
| --- | --- |
| Map network | Versioned source geometry, two closed candidate circuits and specific event alternate retained |
| Building sample | Forty source rings; no approved heights, envelopes or uses |
| Appearance | Two dated-publication sources sampled; camera registration/capture dates/rights open |
| Surface/topography | Paving references and nine coarse SRTM samples retained; current road condition and measured sections absent |
| Culture/audio | Narrow language reference carried forward; consultation/recordings pending |
| Manifest/event | Conditional authored specification |
| Asset/native/visual/gameplay acceptance | Not performed by this package |
