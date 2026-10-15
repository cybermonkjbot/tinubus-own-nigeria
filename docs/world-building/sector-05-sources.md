# Sector 05 evidence register

Research date: October 11, 2026. Source identifiers J01–J04 apply to the companion blueprint. Raw research downloads are temporary; versioned geometry and provenance are durable. No reference photograph is shipped as an asset.

## J01 — fresh OpenStreetMap extract

Endpoint: `https://api.openstreetmap.org/api/0.6/map.json?bbox=3.3315,6.5555,3.3390,6.5620`

Raw SHA256: `8c186dc65284c63eff2a73b636f9c45fe67fc211f13489d3d022cebcc59e30b1`.

Derivatives: [roads](sector-05-roads.geojson), [footprints](sector-05-buildings.geojson), [connections](sector-05-connections.json). Attribution: © OpenStreetMap contributors; ODbL 1.0. Licence reference: `https://www.openstreetmap.org/copyright`. Preserve applicable attribution and database obligations on reuse. Account metadata is omitted; source tags, node/way IDs, versions and timestamps remain.

Sixteen complete road ways are retained, including all mapped branches directly connected to the Ogunoloko spine, the northern loop and Ariyo Close. Complete ways can extend outside the query box. This is a selected network, not complete street coverage. The 23 shared-node pairs occur at 13 distinct nodes; none is a ditch/road crossing. All pairs join distinct road ways; unnamed source ways stay unnamed. No access permission is established.

Haversine lengths use radius 6,371,008.8 m, rounded to 0.1 m for reproducibility, not survey precision. The spine is 644.6 m and the four-leg candidate circuit is 740.5 m. The connection file stores exact subchains so database endpoints do not accidentally lengthen the loop.

The 64 building sample rings are selected by approximate planar distance from each ring's non-closing-vertex arithmetic mean to longitude 3.3356446 / latitude 6.5601349. Cutoff: 65 m; factors: 110590 m/degree longitude and 111200 m/degree latitude. This is not a cadastral buffer, polygon centroid claim or complete parcel inventory. All selected ways have building=yes, version 1, April 23, 2024 edit timestamps, and no height/use evidence. Closed rings alone do not establish comprehensive polygon validity.

Source comparison found three S04 overlaps matching geometry, node order, node versions and way versions: 496763689, 589014131 and 535191784. These observations establish source continuity, not world registration or gameplay. No projected coordinates, road cross-sections or vertical datum have been calculated.

Saved-file readback matched all 80 retained road/building features to raw source coordinates, node order, way versions and tags; local document links resolved. This is a bounded source-integrity review, not a build, game test, survey or visual acceptance.

## J02 — historical road maintenance report

Okwy Iroegbu-Chikezie, The Nation, **September 13, 2021**:

`https://thenationonlineng.net/lagos-roads-to-remain-motorable-despite-rains-says-lspwc-boss/`

Downloaded HTML SHA256: `c9ef5afeaa085655b3c238339ddc5b9553b3892843edd22d826a871e3be8a0fd`.

The article reports an interview with the then LSPWC general manager and includes Ogunoloko, Baderin and Adeyemi in a maintenance list. It discusses wet-season maintenance methods generally; it does not assign a specific material, road length, drainage section or current condition to our selected ways. Treat reported maintenance as historical context, not a current engineering certificate. Its live October 11, 2026 masthead is not the article date. No claim about the current officeholder is made.

## J03 — publisher-captioned flood photograph

The Nation Nigeria public post, read without account action:

`https://www.facebook.com/thenationnigeria/posts/photos-physically-challenged-man-navigates-flooded-ogunoloko-canal-in-mafoluku-o/122273002310255308/`

Visible date: **June 28**, with no year in the inspected post label/attributes. Search relative-age labels are not used to resolve the year. Publication year and original capture date remain unknown.

The body identifies a rain-related access incident at the Ogunoloko canal area in Mafoluku-Oshodi and credits Isaac Jimoh Ayodele. The displayed image was downloaded from the post's image element and inspected directly. SHA256: `2d477534744cc043fbb735ae336dbfec364d6bdb9e11a2a31ceb095aeb7ca1ad`.

Bounded visual observations: yellow tricycles with dark canopies, pedestrians and a wheelchair user in water, small frontages/poles/wiring in the background, and water spilling across an irregular edge on the image's right. The photograph does not provide calibrated depth, velocity, structural dimensions, camera coordinates, an as-built street plan or current traffic permissions. Its location is publisher-captioned, not independently registered to an OSM bridge. Do not identify the pictured people or copy them into characters. Ignore reader comments as evidence. Rights are unresolved; neither the image nor its logos are shipped as textures.

The source's report and the image are one publication chain, not independent corroboration. The media file is appearance evidence, not a surveyed hydraulic observation or proof of present conditions.

## J04 — carried-forward adjacent architecture and language references

See [Sector 04 sources H02 and H05](sector-04-sources.md). The August 12, 2021 Adeyemi report's two inspected photographs support adjacent historical frontage/drain observations, not exact Ogunoloko facade placement. They were not newly geolocated in this pass.

Mimi Werna's British Council Voices article, April 22, 2020, was read in the preceding research pass: `https://www.britishcouncil.org/voices-magazine/nigerian-pidgin-words-phrases`. It supports only the narrow lexical uses referenced in the blueprint. Oshodi-specific speech performance, social context and recording consent remain unresolved. No audio or extended dialogue is reused.

## Fresh searches and exclusions

Commons geosearch endpoint:

`https://commons.wikimedia.org/w/api.php?action=query&list=geosearch&gscoord=6.5595%7C3.3355&gsradius=700&gsnamespace=6&gslimit=100&format=json`

SHA256: `5b9a3e49850ea05b1a20ec8bf5d745292fb9384f6f1901c18eff7d1e7bc54e1e`. Returned geosearch list is empty. This query's failure does not prove that no open imagery exists.

Publisher archive APIs were queried for Ogunoloko, Ogunlaja and Larinde:

`https://akinwunmiambode.com/wp-json/wp/v2/posts?search=Ogunoloko&per_page=20`

Ogunoloko and Larinde returned empty arrays, SHA256 `4f53cda18c2baa0c0354bb5f9a3ecbe5ed12ab4d8e11ba873c2f11161202b945`. The Ogunlaja response, SHA256 `72e189e7bc48b6fe767464201370aa6c7a8ef1caf18eeb556b5aac8baa9c4842`, concerns personal names and unrelated places; none supplies street evidence here.

Web search returned no usable displayed results. Collaborative browser Google discovery followed by original-page reading and direct API/download access supplied the evidence above. Google AI Overviews are excluded. Search snippets about schools, ward centres, businesses, land disputes, violence and proposed 2024 roadwork are discovery only; they do not authorize buildings, activities or events. No current transport-law or surveyed municipal terrain source was obtained.

## Readiness

| Layer | State |
| --- | --- |
| Horizontal network | Versioned source geometry and closed candidate loop retained |
| Footprints | 64 source rings; envelopes/heights/uses not approved |
| Appearance | One publisher-captioned flood image inspected; exact position/date/rights unresolved |
| Surface and topography | Historical maintenance report; current surface and survey unresolved |
| Culture | Narrow lexical reference; local consultation and performance pending |
| Asset checklist/event | Conditional authored specification |
| Native content and gameplay | Not created or accepted by this package |
