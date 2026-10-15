# Sector 07 evidence register

October 11, 2026. Source labels L01–L05 apply to the companion blueprint. Raw downloads remain temporary; durable derivatives retain provenance. No reference photograph is shipped as a game asset.

## L01 — fresh OSM roads, crossings and footprints

Endpoint: `https://api.openstreetmap.org/api/0.6/map.json?bbox=3.3305,6.5540,3.3405,6.5595`

Raw SHA256: `6daa5f81610f5bf9eb82b94411418c1260f0e2bd67b7881c7d30098df55b0754`.

Files: [network](sector-07-network.geojson), [buildings](sector-07-buildings.geojson), [connections](sector-07-connections.json). Attribution: © OpenStreetMap contributors; ODbL 1.0. Licence reference: `https://www.openstreetmap.org/copyright`. Preserve applicable attribution/database obligations in reuse. Account metadata is omitted; way/node IDs, versions, timestamps and tags remain.

Nineteen complete road ways and ditch 92451100 v9 are retained. Bridge ways are Adesanya 588965956 v2, Olaiya 1279347656 v1 and Mafoluku 589001491 v6, all bridge=yes/layer=1. Their line intersections with the ditch are calculated in longitude/latitude and stored separately from road connections. No bridge shares a node with the ditch. Ditch source ordering is not a hydraulic-flow measurement.

The road graph has 24 shared-node pairs at 24 distinct nodes, with six same-road joins. The Mafoluku/Oshodi naming transition is a source-way join between differently named roads, not proof of a physical sign or junction on the ground. Full ways can extend outside the query box; selection is not complete local coverage.

Haversine lengths use radius 6,371,008.8 m, summed before rounding to 0.1 m. Northern circuit: 1,256.3 m; southern circuit: 981.1 m. Bridge road polylines: 12.9, 11.5 and 17.5 m respectively. Rounding is for reproducibility, not accuracy. Exact route subchains are preserved. No road width, deck span, load limit, turning eligibility, surveyed access or flood clearance follows from these lines.

The building sample uses the arithmetic mean of non-closing ring vertices and an approximate planar radius of 65 m around longitude 3.3379594 / latitude 6.5573895, near the derived Olaiya crossing. Scale factors: 110590 m/degree longitude, 111200 m/degree latitude. This is not a cadastral buffer, full-polygon containment test or complete block inventory.

Twenty-five closed rings are retained: 24 building=yes and one building=construction. The latter is way 1275302521 v2, timestamp 2024-05-03T13:27:19Z. Edit dates do not establish construction age or ongoing work; no building height/use is verified. Ring closure does not prove comprehensive polygon validity.

Fresh overlap comparison matches geometry, node order, node versions and way versions for S06 ways 92451057, 92451090 and 93156426, and S04 ways 8564505 and 92451100. These remain shared references requiring one future runtime owner. No canonical-world registration is established.

## L02 — dated Adesanya report and historical material image

Kazeem Kasali, TVC News, **June 23, 2019**, displayed publication time 3:23 PM GMT+1:

`https://www.tvcnews.tv/lasema-pulls-down-partly-collapsed-building-in-lagos/`

HTML SHA256: `0b6ae98ffc5d512bbc0ce6b268a11a6cbbc3971c5d7761a0c0df03bd4f741efc`.

The original article reports an emergency response and demolition at 35 Adesanya Street, Mafoluku-Oshodi. That is the publisher's address statement, not a verified coordinate or current structure. Other search results vary in street number and casualty descriptions; no reconciliation or casualty claim is needed for this blueprint, and none is adopted.

The page initially exposes a generic lazy-load filler; it is not the reference image. The original image URL was read from the article image's data-src attribute:

`https://tvcnews.tv/wp-content/uploads/2019/06/WhatsApp-Image-2019-06-23-at-3.07.05-PM-1.jpeg`

Downloaded and directly inspected. SHA256: `a8665b4bd8df2cbca6263d8e755a882867fcb1ca1c174052a4b5f3b374975d04`.

Visible observations are rendered/painted walls, window openings, exposed roof timbers, corrugated sheet material and rubble during demolition. It is an emergency scene, not evidence of typical current street conditions. Camera position, precise capture time, source photographer, measured dimensions and reuse rights remain unresolved. The filename is not independent capture-date proof. No damaged building, people, machinery brand or real address is reproduced as a game asset/event.

## L03 — Olaiya street-associated housing advertisement

Nigeria Property Centre reference 957894, displayed as posted **May 26, 2021**:

`https://nigeriapropertycentre.com/for-sale/houses/block-of-flats/lagos/oshodi/mafoluku/957894-a-block-of-8-units-of-3-bedroom-flat-each-on-full-plot`

Original page read in the collaborative browser. It associates an advertised block of flats with Olaiya Street, Mafoluku, Oshodi, and currently states that the property is no longer on the market. The listing is supplied by an advertiser and the platform disclaims verification. No current transaction, price, ownership, floor count, unit count, dimensions or title claim is adopted as world truth.

No original property photographs were obtained from the inspected listing, and no parcel/camera coordinate was verified. This is limited street-associated housing context, not permission to label a sampled footprint. Generic area-guide content and unrelated similar-property images are excluded.

## L04 — fresh coarse elevation query

The complete query and eight results are preserved in [terrain context](sector-07-terrain-context.json). Provider: Open Topo Data public API, dataset srtm30m, interpolation explicitly nearest. Raw response SHA256: `ed8168cb216aa8f8d746b99a8e6f1874605d0b13f74173b5aa83a8e42e0d3e82`. Status OK; no null results.

Provider/product documentation was read in the preceding pass; see [Sector 06 K05](sector-06-sources.md): `https://www.opentopodata.org/datasets/srtm/`, `https://www.opentopodata.org/api/`, and the USGS SRTM 1 Arc-Second Global reference. Provider-stated version is 3, nominal spacing one arc-second (about 30 m), source horizontal datum WGS84, vertical datum EGM96, units metres. Radar mission dates are February 11–22, 2000; void-fill sources may differ.

Values in record order are 26, 12, 12, 12, 13, 8, 8 and 18 m. The first query is Amusa/Adesanya; the next six are paired endpoints of the three crossings; the last is Olaiya/Rufai. These are historical raster context, not present-day ground or deck benchmarks. The result locations repeat query coordinates, not documented cell-centre coordinates. Close bridge endpoints can share a cell or straddle cells; do not convert their differences into road slope or flood depth.

No underlying raster tile, local accuracy metadata, ground/structure separation, bridge survey or canonical registration was obtained. Raster EGM96 is separate from the null engine datum. Samples are explicitly excluded from collision, road-grade and flood-calibration use. Relevant local projection packages were absent; none was installed, and no approximate hand-coded projection was substituted.

## L05 — carried-forward language reference

See [Sector 04 H05](sector-04-sources.md): Mimi Werna, British Council Voices, April 22, 2020, `https://www.britishcouncil.org/voices-magazine/nigerian-pidgin-words-phrases`.

This supports narrow lexical meanings, not local performance, accent or universal language use. No new consultation, interview, audio licence or recording consent was obtained in Sector 07. Adjacent-sector imagery is not reassigned to this sector's footprints.

## Fresh searches, rejected matches and remaining coverage

Commons geosearch:

`https://commons.wikimedia.org/w/api.php?action=query&list=geosearch&gscoord=6.5573%7C3.3379&gsradius=600&gsnamespace=6&gslimit=100&format=json`

Returned an empty list. SHA256: `5b9a3e49850ea05b1a20ec8bf5d745292fb9384f6f1901c18eff7d1e7bc54e1e`. This does not prove that no open imagery exists. The TVC image is not presumed open-licensed.

Publisher archive searches:

- `https://akinwunmiambode.com/wp-json/wp/v2/posts?search=Olaiya&per_page=20` — SHA256 `39f35ff5ed1142aea1c78e9703ef26b62b33971b2b1a649e8c75515190c31378`; results concern people and Ajijolaiya Street in Epe, not the mapped Olaiya Street.
- `https://akinwunmiambode.com/wp-json/wp/v2/posts?search=Mafoluku&per_page=20` — SHA256 `48958f29a3793085ee444594eb8cf551726c022a22555adff546448e28908758`; returned roadwork references concern other named streets and supply no located crossing geometry here.

A search result appeared to concern Oshodi/Mafoluku asphalt work. Reading the original Facebook group post instead established a **September 20, 2025** notice concerning Abolaji Link, Kalejaiye and Beesam:

`https://www.facebook.com/groups/232242814944171/posts/1301924037976038/`

It is excluded from Sector 07's placement and present diversion rules. Do not carry a related-post snippet into the main document's claims.

A walking-video lead, `https://www.youtube.com/watch?v=Z5jMY8sTZ5E`, reached an automated-access challenge. No challenge was bypassed, video reviewed, date verified or route inferred. The title/description snippet is not street-image evidence.

Search hits concerning Eniola Adesanya in Beesam, Abraham Adesanya in Ajah, and Olaiya in other parts of Lagos are not merged with this sector. Brand/logistics address posts were not used to populate businesses. Web search produced no usable displayed results; collaborative-browser discovery, original-page inspection and direct public API retrieval supplied the evidence above.

No contemporary registered street-image coverage, survey, current route-specific transport permission or local cultural consultation was obtained. Those gaps prevent final 1:1 placement claims, but do not erase the useful horizontal topology and authored conditional systems.

## Source-integrity review

Read-only review on October 11, 2026 matched all 45 retained features to the downloaded OSM response: coordinates, complete node order, way/node versions, timestamps and tags. The building selection reproduced 25 closed rings. All 24 road shared-node pairs, six same-road joins, two closed circuit subchains and their rounded lengths reproduced; the three road/ditch intersections remained excluded from road routing. Five prior-sector overlaps matched their recorded geometry and version fields. All eight terrain samples matched the saved response, query order and OSM node coordinates; raw OSM and terrain hashes matched their registers. Local Markdown links resolved. This establishes derivative consistency, not survey accuracy, current access, rendering or gameplay acceptance.

## Readiness

| Layer | State |
| --- | --- |
| Road/water topology | Nineteen road ways, one ditch, three separated crossing leads and two candidate circuits |
| Footprints | Twenty-five source rings; construction-tag recheck and heights/uses open |
| Architecture imagery | Dated historical photograph inspected; current placement and rights unresolved |
| Terrain | Eight coarse raster samples; no measured street/bridge/channel profile |
| Culture/audio | Narrow lexical reference carried forward; consultation/recordings pending |
| Manifest/event | Authored conditional specification |
| Native/visual/playable acceptance | Not performed by this package |
