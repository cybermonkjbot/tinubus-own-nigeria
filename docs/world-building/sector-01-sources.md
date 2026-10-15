# Sector 01 source register

Reviewed October 11, 2026. URLs identify provenance; source retrieval is not proof of current site conditions. Raw downloads remain temporary; the versioned geometric derivative is retained in the repository. No third-party photographs or PDF are shipped with this package.

## E01 — OpenStreetMap geometry

Endpoint: `https://api.openstreetmap.org/api/0.6/map.json?bbox=3.3485,6.5510,3.3550,6.5595`

Fresh retrieval SHA256: `5fe9254ffe1a256d47385642619442772f122bf2c537223cff7683d2206211f5`.

Derivative: [sector-01-geometry.geojson](sector-01-geometry.geojson), 26 complete ways and one station point. The file preserves way/node IDs, versions, timestamps, coordinates and relevant original tags, excluding contributor account metadata. Attribution: © OpenStreetMap contributors, ODbL 1.0. Retain attribution and evaluate the applicable database/share-alike requirements before distribution or mixing into another database. License reference: `https://www.openstreetmap.org/copyright`.

Selected ways include three terminal polygons, two platform polygons, road/rail alignments and footways. Complete Agege Motor Road ways extend beyond the query rectangle; neither the geometry file nor its feature count establishes fully mapped transport coverage. No elevation, width survey, current operating status or final access policy is supplied by this snapshot.

The bridge `layer` values encode relative separation, not metres. A station point is not a pedestrian entrance. Railway alignments and their tags are retained without assigning an operational service to each track. BRT ways 1157247978/1157247979 mentioned in earlier discovery were not captured in this rectangle and are not represented as verified geometry in this package.

Local comparison on October 11, 2026 found the Sector 01 and Sector 02 copies of way 5153761 identical in geometry, node sequence, node versions and way version. Shared interface node 1079849258 remains `[3.3497770, 6.5564935]`. This is a source-integrity observation, not a walkability check.

## E02 — Federal Ministry of Environment-hosted interchange ESIA

Source: `https://ead.gov.ng/wp-content/uploads/2020/08/Oshodi-Interchange.pdf`

Downloaded earlier on October 11, 2026 and re-read during this consolidation. SHA256: `83ab602969659824fd63d6834a35998d16302dc7710ba0c16a1259e4c72bf331`.

This is a planning/environmental document, not a contemporary as-built survey. Hosting path includes 2020/08; do not infer that every observation dates to August 2020.

| PDF page, 1-based | Evidence used | Limit |
| --- | --- | --- |
| 39 | Location at major transport routes and contextual project description | Broad coordinate bounds are inconsistent with the terminal area; reject as a georeferencing envelope |
| 41 | Labelled map includes Oshodi Road, Agege Motor Road and expressway | Historical map context, not current roadway condition |
| 43 | Three terminal sites and connecting infrastructure proposal | Coordinate headings swap apparent longitude/latitude; no blind numeric ingestion |
| 78 | Four-storey terminal proposal | Conflicts with OSM five-level tags; final count/height unresolved |
| 89 | Total-station topography/DTM described; investigation history | Actual DTM/spot-level dataset not obtained |
| 105–106 | Proposed standby power, drainage and boarding-area design | Generator sizes, equipment locations and gradients are not verified installations |
| 129–130 | Historical meteorological discussion referencing NIMET | Not a forecast or localized flood/visibility calibration |

The page 43 coordinate interpretation, retained only for cross-checking, is:

- Terminal 1: longitude 3°21.079′ E, latitude 6°33.292′ N.
- Terminal 2: longitude 3°21.061′ E, latitude 6°33.349′ N.
- Terminal 3: longitude 3°21.168′ E, latitude 6°33.392′ N.

These values are not surveyed footprint vertices, building centres or approved anchors. The document's electrical units/capacity statements and broad geographical values also require independent reconciliation; they must not drive equipment simulation or terrain heights.

## E03 — 2024 link bridge photograph

File page: `https://commons.wikimedia.org/wiki/File:Oshodi_Bus_Terminal_link_bridge.jpg`

Image: `https://upload.wikimedia.org/wikipedia/commons/3/38/Oshodi_Bus_Terminal_link_bridge.jpg`

Commons metadata reports photograph date May 25, 2024; author Atibrarian; CC BY-SA 4.0. Image SHA256: `09073c13ec2edf8d8d2c84455785793e16c541ebc2b6acfc3fd487fcc0a06dfb`.

Directly inspected: lattice support, cable-linked covered bridge, curved terminal envelope, exposed truss, barriers, traffic and station signage. Does not establish exact dimensions, all terminal elevations, sound, current opening/access or the identities of vehicles/occupants. Signs visible in a reference do not automatically authorize texture extraction.

## E04 — 2020 terminal photograph

File page: `https://commons.wikimedia.org/wiki/File:OSHODI_Bus_termina.jpg`

Image: `https://upload.wikimedia.org/wikipedia/commons/0/0a/OSHODI_Bus_termina.jpg`

Commons metadata reports photograph date February 29, 2020; author Oluwaseyi Ale; CC BY-SA 4.0. Image SHA256: `f1a96422d19409ff926085bbee078474e9b571e505e395d197d4f841e40c237e`.

Directly inspected: elevated pedestrian viewpoint with railings, curved envelope/truss structure, panelled/glazed surfaces, upper road deck, supports and activity below. It is useful for vertical relationships and appearance, not a complete walkability or accessibility audit.

Metadata for E03/E04 was freshly retrieved through Commons action=query, prop=imageinfo, iiprop=url|extmetadata. Response SHA256: `23be59bfd14dd5c68a343dbbfba4b33abc2f7c827cea7e5fdabac370e44c42c2`. Preserve author, license and alteration records for any authorized reuse; downloaded reference images are not game textures in this pass.

## E05 — LAMATA Red Line schedule

Source: `https://www.lamata-ng.com/our-schedule/red-line/`

Freshly retrieved October 11, 2026; HTML SHA256: `f1c38ddcb454d47bad8a8ef07a50ba79ae069c11d5ac5a8f0cd1ca3d6c7a2d83`.

The retrieved operator page lists Oshodi in the Monday–Friday Red Line timetable between Mushin and Ikeja in one direction and between Ikeja and Mushin in the reverse. This supports a published service relationship only. It is not live train telemetry, a Sunday operating guarantee, a route assigned to every mapped track, or evidence for terminal-to-platform access. Do not hardcode this timetable as permanently current game behavior.

## Claim conflicts and placement gates

| Claim | Decision |
| --- | --- |
| Four storeys versus five mapped levels | Keep both sources; approve neither height until as-built reconciliation |
| Numeric ESIA coordinate headings | Record the apparent swap; use OSM geometry for mapped topology, not misleading table labels |
| Surface tags equal current road materials | False; record tags and require current localized confirmation |
| A bridge layer supplies clearance | False; survey needed |
| Every requested vehicle/brand/phrase is locally verified | False; vehicle policy, exact merchandise and fluent script review remain pending |
| A current web timetable proves today's operations | False; published schedule only |
| An available photo is clearance for all artwork in it | Not established; separate rights review |
| This package creates native gameplay | False; documentation/research only |

The browser research tool produced no usable displayed citation results in this pass; direct API/HTTP retrievals and local image/PDF inspection supplied the evidence above. No claim of a complete current street survey, cultural consultation or production-ready placed world is made.
