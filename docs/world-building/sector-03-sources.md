# Sector 03 evidence register

Retrieved/reviewed October 11, 2026. This register separates source assertions from observations, design and unanswered placement questions. Photos remain research references, not shipped textures.

## G01 — Fresh OpenStreetMap snapshot

Endpoint: `https://api.openstreetmap.org/api/0.6/map.json?bbox=3.3440,6.5593,3.3505,6.5660`

Raw SHA256: `552542bc9e15db1aad4dd403c77765421c97f1721f8b8f6a6cef09cf607f6da2`.

Durable derivatives: [road geometry](sector-03-roads.geojson) and [junction/route records](sector-03-junctions.json). Attribution: © OpenStreetMap contributors, ODbL 1.0. License reference: `https://www.openstreetmap.org/copyright`. Retain attribution and resolve applicable database obligations before distribution. Contributor account metadata is not retained in these derivatives.

Fifteen complete ways are retained, with node/way IDs, versions and timestamps. They include Igbehinadun, Oyetayo, Akinpelu, Adeoye, Okun Owa, Brown, Church and Adeyemi. Complete-way inclusion means some coordinates lie outside the query rectangle; this is not a clipped sector polygon.

Important source assertions:

- Igbehinadun way 1442626890 v2, timestamp July 31, 2026, has `oneway=yes`. Its ordered nodes lead north from 4395533094 to 35785494. The way timestamp does not establish when the one-way rule began or was field-confirmed.
- Oyetayo ways 93146069 v14 and 5153763 v12, both timestamped October 3, 2026, carry embankment, two-lane and unpaved tags. No measured elevation or cross-section accompanies those tags.
- The bounded snapshot inspection found no `ele` or waterway-tagged elements. It cannot establish that the area lacks drains or waterways.
- Source ways 5153762, 93146084 and 1442626893 match Sector 02 in coordinates and way versions. Shared node 4395533093 is `[3.3477254,6.5608319]`; the northern continuation starts there without an invented gap.
- Oyetayo 5153763 shares node 35785493 with Agege Motor Road way 746512955 and node 1477755408 with way 133805221 in the raw snapshot. These external road ways are not duplicated in S03's derivative; the connection alone does not approve a crossing or turn.

Route lengths use the haversine formula with radius 6,371,008.8 m and are rounded to 0.1 m for reproducibility, not survey precision. The 1,199.1 m loop is the sum of rounded leg lengths. Twenty-three shared-node pairs represent twelve unique nodes; seven pairs are same-road segment joins, sixteen are cross-street pairs. No sidewalk continuity or legal driving claim is derived from this graph alone.

## G02 — Nearby agency-labelled photograph: useful appearance, unresolved placement

File page: `https://commons.wikimedia.org/wiki/File:Lagos_state_environmental_and_special_offences_enforcement_agency.jpg`

Original repository path: `https://upload.wikimedia.org/wikipedia/commons/7/71/Lagos_state_environmental_and_special_offences_enforcement_agency.jpg`

Author Stephanie cheks, CC BY-SA 3.0. Commons metadata reports upload time November 2, 2022; the capture-date field is blank. Do not label it a November 2022 photograph. The inspected 1280 px thumbnail SHA256 is `51a7304b632237111309ec4943c44cb5555b751e677efde8e222a5719546d08e`.

The file's wikitext gives photographer location latitude 6.563003, longitude 3.346339, and a separate object location latitude 6.5643389, longitude 3.3472453. It also includes EXIF-derived heading/altitude. Those are not a validated footprint or ground survey. The different coordinates and unspecified capture date prevent an exact named-agency placement in our sector.

Direct image inspection supports a reference family: light rendered/block walls, red profiled pitched roofs, a yard-like space, trees, cables and parked vehicles. It does not establish agency operation, public entry rights, building heights, a current site plan or the identities/roles of people shown.

Wikitext endpoint: `https://commons.wikimedia.org/w/api.php?action=query&prop=revisions&pageids=124900511&rvprop=content&rvslots=main&format=json`

Response SHA256: `2729c51934edef31bba97d1d801cd37c3719fc4e50fcdecbdde134d09dffd0da`.

## G03 — Historical Danfo queue near the southern interface

File page: `https://commons.wikimedia.org/wiki/File:A_queue_of_danfo_buses.jpg`

Original repository path: `https://upload.wikimedia.org/wikipedia/commons/8/8c/A_queue_of_danfo_buses.jpg`

Commons metadata: February 11, 2016, Funsho116, CC BY-SA 4.0. Geosearch location: latitude 6.556448, longitude 3.349168, south of this new sector and near the earlier Oshodi Road study. Inspected 1280 px thumbnail SHA256: `681c799224447258cd32c524647e20a1b7280d867fd2bf47987be34970aa1f06`.

Directly observed: a line of yellow minibuses, nearby pedestrians, carried goods/trays, and mixed low/multistorey frontages. The image is a reference for relative crowd/vehicle scale and load-carrying poses. It does not establish a current bus route on Igbehinadun, a precise bus model, individual identities, current merchandise, measured traffic behavior or a licence to reproduce portraits in-game.

## G04 — Igbehinadun Phase 2 rehabilitation statement and collage

Public post: `https://www.facebook.com/groups/240657549992769/posts/1966645827393924/`

Read in the collaborative browser without logging in. Posted by Soloki Moses Gbenga to CONFERENCE 57 CPS; the body attributes the statement to the media office of the Oshodi-Isolo council chairman. This is a council-media-attributed repost, not an independently authenticated municipal works record. The body announces Phase 2 commencement and paving/construction materials delivered to the site; it does not certify completion.

Date handling: the visible post says **16 March** without a year. A Google search displayed March 16, 2026, but the post's readable DOM did not independently expose the year. Retain the year as unresolved until independently confirmed; do not claim a current works state or infer a completion date. The page explicitly labels its short generated title as AI-generated; the research used the readable statement body and image, not that generated title.

The attached six-panel collage was downloaded from the image URL exposed by the post and inspected. SHA256: `e750a66a9a85082f96af197d91432c16604d75b9fc4db216e4dafa8e8019b32b`. Stable provenance is the post above; the CDN URL is temporary. Rights for reuse are not established, so the image is not included in the repository or used as a game texture.

Observed reference details: strapped paver stacks, loose material piles, a tipper, pedestrians, balcony/frontage forms, awnings and cables. A visible sign reads **IGBEYINADUN STR**, differing from OSM's spelling. Exact camera positions, work limits, dates of individual panels and heights remain unverified. The collage is not used to place a named business, attribute a resident identity, or prove that the northern Igbehinadun segment is under construction.

## G05 — December 21, 2015 rehabilitation order

Primary archive page: `https://akinwunmiambode.com/ambode-moves-to-transform-ebute-meta-to-tourist-haven/`

Freshly fetched via publisher API: `https://akinwunmiambode.com/wp-json/wp/v2/posts?search=Igbehinadun&per_page=10`

API response SHA256: `154fccf85140c85b735e04bb81ebfb288626b5536a3b39ac8583d94dbe76e02a`.

The publisher record is dated December 21, 2015. Its Oshodi passage reports an order for the Public Works Corporation to rehabilitate Igbehinadun and Inasa, alongside a Brown Street works visit. This is an announced instruction, not proof of completion, pavement type, current condition or the precise extent of work. Unlocated images in that multi-location article are not assigned to Igbehinadun.

The same API search returns an April 2, 2018 article listing an Igbehinadun Road within an Alimosho road programme. That is a different geographic context and is excluded from this sector's completion evidence. Name matching alone is insufficient.

## Search coverage and rejected inferences

Fresh open-repository geosearch:

`https://commons.wikimedia.org/w/api.php?action=query&list=geosearch&gscoord=6.561841%7C3.3478084&gsradius=1000&gsnamespace=6&gslimit=100&format=json`

Response SHA256: `5c5932dfba4b3b6e0dac8f8d0621b6da6259f86e43081027b408936989b3ed44`.

Selected metadata endpoint:

`https://commons.wikimedia.org/w/api.php?action=query&prop=imageinfo&pageids=128877241%7C128885310%7C128862799%7C124900511%7C128885285%7C88431486%7C88431487&iiprop=url%7Cextmetadata&iiurlwidth=1200&format=json`

Response SHA256: `d91104031b7e0fe506ca75cd1a0bf09fc1d6c0390d409063b71d084890b54fe6`.

Many nearby results describe food or bead objects and share repeated coordinates. They do not establish a restaurant, market stall, cultural district, resident ethnicity or façade inventory. Metadata screening rejected them as street-placement sources; it did not prove their geotags erroneous. G02 and G03 were inspected visually; other candidate images are not silently treated as inspected.

Web search calls returned no usable displayed results; the collaborative browser and direct API retrievals provided the readable material. Google queries for Igbehinadun, Oshodi and rehabilitation were used for discovery, not to certify geography. Results referring to Ogba/Aguda, Shomolu/Pedro, Ijoko, Oworonshoki or Alimosho are not imported into this map.

## Placement/evidence status

| Requirement | Evidence now | Still needed |
| --- | --- | --- |
| Adjacent street topology | Fresh complete OSM ways, shared-node graph and comparison to S02 | Current access and mapped omissions review |
| Surface and works | Source tags, 2015 order, reported Phase 2 statement/collage | Geolocated current imagery, work limits, dated as-built condition |
| Buildings | Nearby/historically reported visual families | Registered footprints, dimensions, uses and permissions |
| Topography | Oyetayo embankment tags | Survey/DTM, vertical datum, drainage levels and outfalls |
| Transport | Mapped one-way segment, dated nearby Danfo image | Dated policy, operator routes, measured geometry/behavior |
| Culture and audio | Bounded visual observations; proposed non-stereotyped routines | Fluent local script review, sound references and recording permissions |
| Weather | Proposed systems only | Local meteorological/visibility calibration and drainage data |
| Production | Blueprint, checklist and source data | Asset construction, authorized integration, visual and gameplay acceptance |

Research gaps restrict individual placements and claims; they do not prevent the next connected research pass. No final 1:1 world, legal traffic network or cultural consultation is certified by this package.
