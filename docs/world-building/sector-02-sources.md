# Sector 02 source register

Retrieved/reviewed October 11, 2026. Read alongside the blueprint. A source date is not a statement that the source depicts today's conditions.

## S01 — OpenStreetMap road/node snapshot

Primary mapping source, directly downloaded from the OSM API:

```text
https://api.openstreetmap.org/api/0.6/map.json?bbox=3.3448,6.5560,3.3510,6.5609
```

Full-response SHA-256: `f91b0352aa05e491a24ea1547dda95fee5867926ab88afb87941d61f31b91514`.

The small full response is retained temporarily at `/tmp/oshodi-sector02.LzlUam/osm-map.json`; that temporary path is not durable archival storage. The durable derivative `sector-02-roads.geojson` retains all selected way geometries, tags, node IDs/versions/timestamps and way versions/timestamps. No OSM contributor account identifiers are included.

Ten ways selected, without clipping their node sequences: 5153761, 1547601493, 93146055, 93146084, 1442626893, 5153762, 93146058, 93146083, 441859101 and 441859104. The snapshot contains fifteen pairwise shared-node connections among these ways. The derived loop is approximately 1,025 metres, using spherical great-circle distances between mapped nodes. This is neither a surveyed distance nor a proven pedestrian route.

Claim scope: mapped names, geometry, topology and recorded tags. Does not establish today's paving, kerb widths, legal speeds, trading permissions, footway accessibility, elevations or road conditions. Brown Street's `surface=unpaved` is retained but not accepted as current ground truth. A way's edit timestamp does not date every tag it contains.

Attribution: OpenStreetMap contributors. Licence: ODbL 1.0. Preserve attribution and assess applicable database-distribution obligations when this derivative enters a shipping data pipeline.

## S02 — Brown Street rehabilitation photographic record

Primary project-era communication, February 1, 2016:

```text
https://akinwunmiambode.com/rehabilitation-of-brown-street-oshodi-in-progress/
https://akinwunmiambode.com/wp-content/uploads/2016/02/PIX-0447.jpg
```

The page says work was nearing completion; it is not a completion certificate. The inspected photograph shows a light, textured road surface, raised edge walks, close-set mixed frontages and several transport/work activities. The photographic surface and rehabilitation record conflict with treating the current OSM unpaved tag as sufficient evidence for a dirt road.

Claim scope: historical Brown Street appearance. Camera position/orientation and correspondence to a particular present-day segment are not established. No construction material chemistry, exact widths, floor counts, current business identities or current pedestrian rights are inferred. The photograph has no established reusable asset licence in this review: reference only, not texture redistribution.

Correction to the conversational progress update: the direct primary page establishes rehabilitation in progress, not that a completed upgrade was independently verified.

## S03 — Historical Oshodi produce-market reference

Public photographic repository and metadata API:

```text
https://commons.wikimedia.org/wiki/File:Oshodi_market_lagos.jpg
https://commons.wikimedia.org/w/api.php?action=query&prop=imageinfo&titles=File%3AOshodi_market_lagos.jpg&iiprop=url%7Cextmetadata&format=json
https://upload.wikimedia.org/wikipedia/commons/d/d5/Oshodi_market_lagos.jpg
```

The description dates the scene April 15, 2004; the Commons upload date is December 28, 2007. The inspected photo visibly contains tomatoes/peppers, woven baskets, shallow metal containers, wooden selling surfaces and stock handling. These support a historical prop-reference family only.

No exact location within this sector is supplied. Do not place a produce market on Brown Street solely from this image, copy identifiable people into NPCs, or describe 2004 inventory as verified 2026 merchandise. The repository labels it public domain with a US Department of State source, but also flags incomplete machine-readable source/author metadata. Preserve provenance and resolve that record before embedding it in a distributable reference library.

## S04 — Oshodi Interchange environmental assessment

Government-hosted project report, used as a historical/context source, not a contemporary topographic survey:

```text
https://ead.gov.ng/wp-content/uploads/2020/08/Oshodi-Interchange.pdf
```

Prior direct PDF inspection is retained in the thread; this sector uses its site-network context rather than importing its numerical environmental claims. Its coordinate tables have axis-label inconsistencies. The assessment's regional environmental sections are not street-by-street levels for Brown Street, and no listed flood depth or slope is adopted here. New extraction/georeferencing would be required before an engineering use.

## S05 — City-plan discovery lead

Official LASBCA catalogue:

```text
https://lasbca.lagosstate.gov.ng/model-city-plans/
```

A relevant planning-document discovery route, not a parsed, georeferenced plan in this package. No setback, zoning, topographic or development-control figure is derived from it. Obtain the applicable Oshodi-Isolo sheet, edition, scale, coordinate system and reuse terms before admitting geometry.

## S06 — Regional elevation limitation

Primary dataset publisher/distributor documentation:

```text
https://registry.opendata.aws/copernicus-dem/
https://dataspace.copernicus.eu/explore-data/data-collections/copernicus-contributing-missions/collections-description/COP-DEM
```

GLO-30 is a surface model at roughly 30-metre posting, not kerb/drain survey data. It may guide broad regional relief after datum checks; it does not resolve channels, building thresholds or bridge clearance. No elevation tile or vertical correction is applied in this package.

## S07 — Existing project coordinate and authority contract

Local repository documents inspected this turn:

- `docs/engineering/unreal-native-world-tooling-reuse-research-2026-10-11.md`, especially the documented adapter at line 29.
- `output/cribtest11-visible-world-2026-10-11/native-first-block.md`, coordinate rule at line 25.
- Repository root `AGENTS.md`, read on both Mac and Cribtest11.

These establish the existing source-metres axis mapping and documentation-only operation boundaries. They do not establish a WGS84 registration for the game's fictionalized canonical city.

## Discovery results deliberately not promoted to evidence

Searches also surfaced market-closure reports, old market photographs, and generic area directories. A named market's closure or a directory listing must not be assigned to every shop in this polygon. No current closure, operator roster, denomination, ethnic composition, specific commercial vehicle permission or shop opening schedule is established here.

Commons metadata for `Oshodi 15.jpg` and `Oshodi 19.jpg` dates those photographs January 26, 2019 and names Emmanuel Odekunle as author with CC BY-SA 4.0. Their files were not visually inspected in this pass, and the metadata does not locate them on the selected streets. They are not placement evidence.

## Claim register

| Claim | Status | Next evidence needed |
| --- | --- | --- |
| Ten mapped ways and fifteen shared-node pairs | Verified against downloaded S01 | Re-fetch versions if authoring much later |
| Main loop roughly 1.03 km | Derived from S01; approximate | Pedestrian/vehicle path and elevation survey |
| Brown Street currently unpaved | Unresolved; not accepted | Dated geolocated contemporary imagery/survey |
| Raised road-edge walks visible historically | Historical S02 observation | Current segment matching and measured access |
| Produce props belong at a specific Brown Street stall | Not established | Exact contemporary frontage record |
| Particular road floods to a particular depth | Not established | Surface and invert survey plus calibrated rainfall/runoff |
| Keke/okada are permitted on these links | Not established | Applicable dated official route/class rules |
| Current sign, business name, opening hours or dues | Not established | Local operator/municipal evidence and clearance |
| Region-to-game spatial registration | Not established | Multi-control-point alignment reviewed against canonical geography |
| Unreal assets, render, gameplay or performance | Not performed | Separately authorized implementation/inspection |
