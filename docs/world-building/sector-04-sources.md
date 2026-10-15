# Sector 04 evidence register

Research date: October 11, 2026. Raw downloads remain temporary; durable derivatives retain source provenance. No third-party photos are shipped with these documents.

## H01 — OpenStreetMap roads, ditch and footprints

Endpoint: `https://api.openstreetmap.org/api/0.6/map.json?bbox=3.3380,6.5570,3.3455,6.5640`

Raw SHA256: `429102ec4471034b65544547f97c4a30e1c98c329e953d134f1858e3d07f84a0`.

Files: [network](sector-04-network.geojson), [building sample](sector-04-buildings.geojson), [connections](sector-04-connections.json). Attribution: © OpenStreetMap contributors, ODbL 1.0; licence reference `https://www.openstreetmap.org/copyright`. Preserve applicable attribution/database obligations when reusing the derivatives. User/account metadata is omitted; source tags, node/way IDs, versions and timestamps remain.

Twelve complete road ways and ditch way 92451100 are retained. Adeyemi's four-way chain is 8564504 v12, 589139290 v6, 589014132 v3 and 589014131 v5. Bridge 589014132 has bridge=yes/layer=1; ditch 92451100 v9 has waterway=ditch. Their 2D segment intersection is calculated in geographic coordinates and recorded as a derived lead, with no shared node and no known clearance. Never add it to the road junction graph.

The map data supplies no surveyed road widths, water levels, bank crests or hydraulic capacity. Haversine polyline lengths use radius 6,371,008.8 m, rounded to 0.1 m for reproducibility, not precision. The 710.4 m spine and 917.4 m candidate loop are not verified travel distances.

The building derivative contains 51 closed ways, selected by an approximately 80 m planar-distance cutoff around longitude 3.33884 / latitude 6.56117. The algorithm uses each ring's non-closing-vertex arithmetic mean and factors 110590 m/degree longitude, 111200 m/degree latitude. It is a sampling rule, not a cadastral or survey buffer. All sampled building ways are version 1, timestamped April 23, 2024, with building=yes and no height/use evidence. An edit timestamp is not a construction date or proof of the imagery acquisition date.

A bounded source inspection found no intersections between the sampled footprint boundaries and the mapped ditch segments. That is a geometric observation, not evidence of lawful setbacks, bank clearance, lack of encroachment, polygon validity in every respect or present-day conditions. Do not extrude the footprints into approved structures without further evidence.

The raw snapshot also contains named or addressed structures outside the selected sample, including a building:levels=0 tag. Those do not supply valid height defaults; no named resident/business or religious site is assigned to a sampled footprint.

Local readback on October 11, 2026 matched all 64 retained features to their source coordinates and way versions. Shared Adeyemi way 8564504 matches Sector 03 in geometry, node order, node versions and way version. The four-leg candidate loop closes by shared node ID; the ditch is excluded from every road-junction pair. These are source-integrity observations, not game tests, survey or access acceptance.

## H02 — Adeyemi inspection report and historical photographs

Report: `https://www.dconnectnews.com.ng/2021/08/adeyemi-road-construction-oshodi-isolo.html`

Publisher HTML datePublished: **August 12, 2021**, with timestamp 2021-08-12T11:07:00+01:00. The article's live page header shows the current date; it is not the publication date. HTML SHA256: `39d0337d48e22c5f417b6ae285e97cbf0c717fa0180cca5fb3ae909a6fe3368f`.

The report reproduces an inspection account and quoted commitments concerning drainage at Furniture Bus Stop, Sunmola Abayomi Street and the Oja-Oke axis. These source labels are not geolocated to our crossing. It describes intended complementary drainage work, not a completion certificate, current damage report or flood survey.

Two attached images were inspected directly:

- Publisher image identifier `IMG_ORG_1628762427226.jpeg`; downloaded image SHA256 `d79b851015d7cdce16b67809d084089a16e7c2dba8206890911df14ce7f0f922`. Visible open concrete drain, frontage crossings, a small shop/awning and people standing by the road edge.
- Publisher image identifier `IMG_ORG_1628762441353.jpeg`; SHA256 `1db98ba39ca2b6ed8c3f7f8a42db0f32930268c4b9d1b55067c2c6896f6e9880`. Visible road roller, damaged/unfinished edge and mixed frontages with upper-floor openings.

Image capture date, camera coordinates, rights and identities are not independently established. The date is the associated publication date. Photographs support bounded appearance observations only. Neither is identified as the western mapped bridge. Visible logos/signs are not cleared texture assets.

Related contemporaneous public post:

`https://www.facebook.com/groups/809242119541461/posts/1229963227469346/`

Read without login; visible date **August 12, 2021**, posted by Obababa Gcfr in a chairman campaign group. Its body carries substantially the same inspection account. This is corroboration of publication/context, not independent engineering verification or proof of current officeholders. Meta labels the short generated title as AI-generated; only the original body/date were used. No claim is based on comments or reactions.

## H03 — Historical public-works activity record

Publisher API: `https://akinwunmiambode.com/wp-json/wp/v2/posts?search=Adeyemi&per_page=20`

Response SHA256: `604299a3f125a17688e7f802c6ed0446aab80453a5c8de6c9392fc3679ca07bd`.

Relevant publisher page:

`https://akinwunmiambode.com/brief-report-on-the-activities-of-lagos-state-public-works-corporation-from-monday-october-31st-friday-november-4th-2016/`

Dated November 4, 2016. The maintenance/palliative-work list includes Adeyemi/Salawu/Oremeji in Oshodi. It establishes a historical reported maintenance context, not a precise work polygon, material, completion state or current asset condition. Images in the article label other roads and are not misassigned to Adeyemi.

The response also contains a November 10, 2016 entry for the week November 7–11 mentioning Salawu/Adeyemi. Its title extends beyond the publication timestamp; no daily completion chronology is inferred. Search hits for people named Adeyemi or the Adeyemi Bero Auditorium are unrelated and excluded.

## H04 — INEC public street-address record

Source: `https://cvr.inecnigeria.org/locator/viewCentre/1547`

Read in the collaborative browser October 11, 2026. The locator identifies the Oshodi-Isolo LGA office and lists Adeyemi Street as its address. This is a public institutional street association, not a verified building coordinate, entrance, opening schedule, active electoral service or licence to recreate interiors. No user registration, appointment or account action was taken.

Direct HTTP download returned 403; no raw-file hash is claimed. Browser-readable text, rather than a search snippet, supplied the limited address evidence. The directions page was not successfully retrieved and supplied no coordinate authority.

## H05 — Nigerian Pidgin lexical context

Mimi Werna, British Council Voices, **April 22, 2020**, “Nigerian Pidgin – 20 useful words and phrases.”

`https://www.britishcouncil.org/voices-magazine/nigerian-pidgin-words-phrases`

Read in the collaborative browser. The author presents “How far/body?” among informal greeting variants, explains “Abeg” as a polite request marker, and describes “No wahala” as context-dependent agreement or absence of a problem. The article also explains go-slow as congestion. We use only those narrow lexical points, not its broader sociolinguistic generalizations as universal facts.

This is a Nigerian author's general language guide, not an Oshodi speaker sample or consultation. Final character use, register, pronunciation, timing and recording consent remain subject to fluent local review. No long example dialogue, photograph or audio was reused. Direct HTTP returned 403; browser readback succeeded, so no downloaded-page hash is asserted.

## Open-repository image search and excluded results

Fresh Commons geosearch:

`https://commons.wikimedia.org/w/api.php?action=query&list=geosearch&gscoord=6.5611092%7C3.3420212&gsradius=500&gsnamespace=6&gslimit=100&format=json`

SHA256: `b00eb3e575aabeb4db09c50620641e5aa7f65f51d654f1fa33c01e2518ff998b`.

The returned list contained 19 media candidates near the radius edge. Metadata for six candidates was inspected through:

`https://commons.wikimedia.org/w/api.php?action=query&prop=imageinfo&pageids=128856060%7C128891207%7C128891079%7C129106069%7C128886200%7C131373713&iiprop=url%7Cextmetadata&format=json`

SHA256: `8c8881343fbacb597033c6575a42789f2953dd033746c53fc03242bfb8cc46ed`.

Descriptions concern clothing, people in traditional dress or a dance, not surveyed streets. Their geotags do not establish building use, population identity or a permanent cultural event at our bridge. They were not used for street placement. This search did not obtain open-licensed, contemporary, geolocated frontage coverage.

Web search tools returned no usable displayed evidence. Collaborative browser discovery and direct API/HTTP retrieval supplied the materials above. A guessed Nation article path returned 404 and was not used. Search results about Abiola Adeyemi Street in Igando-Ikotun, Kamoru Adeyemi Street elsewhere, or individuals with the same name are excluded. Current surface, hydrology and traffic policy remain unresolved.

## Readiness

| Deliverable | State |
| --- | --- |
| Connected road/ditch topology | Source geometry retained; no measured access/vertical clearance |
| Building footprint sample | 51 source rings retained; no approved envelopes or uses |
| Architectural reference | Two dated-publication images reviewed; coordinates/capture dates/rights unresolved |
| Cultural language reference | Basic lexical meanings supported; local performance review outstanding |
| Terrain/weather | Horizontal drainage lead only; calibration/survey missing |
| Asset manifest and event | Authored conditional specification |
| Native implementation / visual gameplay acceptance | Not performed by this package |
