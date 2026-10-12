# Town Centre adjoining fabric and opening repairs

We continued from `28c6ac78953ac2fa15ddcb2b4bbb849ff602db66` on verified live editor PID `13584`, project `C:/dev/cribtest-game/apps/companion-unreal/`, Unreal `5.8.3-58210709+++UE5+Release-5.8`. The preceding pending pass30 request completed successfully; it was not rerun. This remains partial editor-authored Town Centre, not a completed neighbourhood or playable city.

## Saved exterior work

Town Centre retains canonical source bounds X -556.625..573.92, Z -992.69..28.855 metres, 138 parcels and the fictional metric plan SHA256 `13a2699ee6ad513616ca25a41bbb05f165e9aeaf3e52b5d89e550ed8b523ad9a`. No districts, coastlines, source roads, parcels or destination positions moved.

Pass29 reconciles 131 pedestrian-overlay decisions within 52 bounds-clipped route IDs. Source paths now use existing concrete rather than asphalt. Invalid asphalt overlays for zero-width retained connections, boardwalk and sand-path routes were removed; underlying support remains. Native boardwalk decking/edge modules and licensed native-ready sand/beach finishes remain explicit missing inputs. This is not complete route or district acceptance.

Pass30 replaces identity-matched legacy frontage on `market-road-4`, `market-road-10`, `market-road-11`, `market-road-12`. Entrance-aligned front/rear wings and one courtyard return reuse accepted frontage assets with unchanged scale, UVs, material identities and source FBX hashes. A side service passage remains open. All four retain their Market Road relationship. Source floor counts are respectively 2, 1, 3, 10; merged module construction does not satisfy those counts. In particular the one-storey shop and ten-storey apartment are not accepted as completed buildings. No new destination connection is claimed.

Pass31 adds 108 accepted native window assemblies to unfilled intermediate ResidentialBay apertures across pass23, pass25 and pass30 fabric. It removes 468 lower RoofSheetPanel placements trapped underneath upper stacks, preserving the top sheets and source asset bytes. Original removal identities and new attachment transforms are recorded in `town-centre-street-pass-31-intermediate-openings.json`. This repair does not remove merged parapets or turn stacked two-storey structures into correct single-floor shells.

Saved packages:

- `/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Maps/L_town_centre_neighbourhood_20261011_streetpass01`
- `/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Maps/L_town_centre_neighbourhood_20261011_streetpass01_detail01`

Both batches survived safe reload with transient and exception references released. `town-centre-street-pass-31-saved-readback.json` binds current map hashes, all 108 window transforms/mesh identities, final roof component counts and 13 actual loaded levels. No dirty map/content packages or active game world was present. Existing destination levels remain loaded but end-to-end physical traversal is not verified. Static danfo remains static, not traffic.

## Rendered evidence

We inspected every PNG in `previews/town-centre-streetpass01-market30-daylight` (seven matched/source views) and `previews/town-centre-streetpass01-openings31-daylight` (the same seven plus four parcel approaches). Each native capture retains exact camera pose, native frame warmup, image hash and receipt hash. New approaches are 18 metres outward from saved entrances at pedestrian height, not repositioned buildings. Existing regression cameras did not move.

Matched Market frontage A/B and Civic open A visibly replace the dark intermediate hole with window assemblies. Source-road view remains readable between buildings. The four direct parcel approaches expose repetitive silhouettes, shaded unstocked open bays and flat pavement patches. The taller fronts exceed the fixed approach frame; these images cannot establish their complete roofline or canonical floor parity. No interiors were accepted. No full street-life, gameplay, collision traversal or navigation acceptance was established.

The first pass31 preflight incorrectly interpreted legacy `instanceIndex` metadata on static actor records as native ISM identity. It failed before mutation, with clean maps. `town-centre-street-pass-31-preflight-failure.json` preserves the defect; lookup now distinguishes `actorLabel` ISM records from static `label` records. Recovery required no editor restart.

## Remaining work and ownership

Town Centre stays the next dependency-ready area: complete source-centred pavement/drain/corner transitions and adjoining parcel fabric before interior expansion. Replace the stacked merged structures with exact floor-count shells; vary setbacks, silhouettes, approved finishes and rooflines while preserving all source entrances. The Market junction is still an open ground expanse, not a finished junction. Uniform concrete land, exposed surface seams, warm brown asphalt, sparse reachable blocks, missing signage/services and empty shop stock remain defects.

The full seven districts, 806 parcels, 264 roads, 19 signatures, 40 destinations, 49 Home recipes, waterfront and five rural areas remain open. Historical `current-coverage.json` zero-output entries are superseded only by newer item-specific native evidence; no full item-by-item reconciliation or completion percentage is claimed. The three October 11 Mac backlog documents are absent in this checkout and did not stop authoring. Current canonical sources and Windows handoffs suffice for the next exterior work.

Gameplay remains with existing owners. No active game world was available; reviewed source/current binaries and approved runtime admission remain dependencies for genuine play inspection. No account/movement/readiness gates, wallet/save systems or gameplay codecs changed. No tests, lint, typechecks, builds, compilation, HLOD, packaging, deployment, installs, downloads, public services, production configuration, pushes or Blender workbench changes ran. Native-only maps and tooling do not change consumed mobile/web renderer assets. Unrelated project, Config, native generator and port-document edits remain preserved and excluded from this checkpoint.
