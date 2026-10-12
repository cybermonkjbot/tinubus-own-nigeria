# Town Centre exterior handoff: passes 38-40

We continued the existing active full-game goal from checkpoint 0d330f5fabb8ec842e22a756679c8306b31e7e49. The previously pending pass39 bridge request finished successfully before any additional editor request. PID 13584 still identifies BaddieCompanion in Unreal 5.8.3-58210709+++UE5+Release-5.8. No editor restart or worker was used.

## Saved exterior content

Town Centre remains the selected district: 138 canonical parcels, source X -556.625..573.92 and Z -992.69..28.855 metres. Plan SHA256 remains 13a2699ee6ad513616ca25a41bbb05f165e9aeaf3e52b5d89e550ed8b523ad9a. Native mapping remains (X,Y,Z)=100*(source Z,source X,source Y). No approved coastline, road, parcel, destination or travel relationship was changed.

Pass38 adds four fitted pavement-side apron meshes around the retained 1.8m crossing lanes at market-road-4, market-road-10, market-road-11 and market-road-12. The 22 polygon pieces reconstruct the omitted source pavement slots rather than covering the whole entrance with a generic slab. Native centimetres, unit actor scale, measured UV0, existing concrete material and complex-as-simple static triangle collision are recorded in town-centre-street-pass-38-fitted-aprons.json. Its saved readback verifies four actors and 792 vertex instances. Pavement-band side voids are reduced, not eliminated across the drain/kerb band.

Pass39 adds accepted kerb, drain, drain-cover and paving modules along source-defined station-road, theatre-street, college-lane, arcade-crescent and old-town-lane segments within the district bounds. It preserves junction/entrance conflict exclusions, source FBX hashes, pivots, unit scale, material paths and UVs. Its receipt records 15,247 module instances, 519 superseded owned legacy roadside actors removed and 1,337 conflict omissions. These counts describe authoring output, not completed roads or gameplay.

Pass40 adds 3,797 hidden paving subbases for the new roadside batch. Top +3cm and bottom -7cm support the unchanged accepted paving at +15cm. Primitive subbase UVs are not accepted visible surface artwork. This does not excavate drains or fill junction and entrance omissions.

Exact saved map packages:

- /Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Maps/L_town_centre_neighbourhood_20261011_streetpass01
- /Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Maps/L_town_centre_neighbourhood_20261011_streetpass01_detail01

New asset namespace: /Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Aprons38/Meshes/SM_market_road_{4,10,11,12}_apron. Exact individual asset packages, immutable OBJ inputs and hashes are in the pass38 receipt. No accepted art source bytes were changed.

Both maps were saved and safely reloaded after transient Python Unreal objects, modules and exception references were released. town-centre-street-pass-40-saved-readback.json verifies all eleven roadside corridor batches, their native source transforms, materials and collision, retained on-demand PCG, 15,047 total subbases, current map hashes, clean packages and no active game world. Historical pass38 map hashes refer to that earlier saved revision; pass40 supplies current map hashes. No duplicate authoring request remains live.

## Rendered inspection and limits

All five matched before corridor images and seven after images were directly inspected. Cameras are at 165cm with fixed source segment tangents and FOV; capture records preserve native poses, frame warmup and image hashes. Sets: previews/town-centre-streetpass01-corridors39-before and previews/town-centre-streetpass01-corridors39-after. The after set includes Market Road parcel4 and junction regression views. All six earlier apron after-images were inspected during pass38; this continuation also inspected parcel10, parcel11 and parcel12. Apron preview set: previews/town-centre-streetpass01-aprons38-after.

The new corridors show more defined paving and kerb edges, replacing flat legacy strips. Fitted apron sides visibly reduce entrance cuts while keeping the central crossing lanes. Native persistence is verified, but visual completion is rejected: long stretches remain nearly empty concrete ground, with repetitive distant buildings and unfinished end/corner pieces. Market junction remains an open field, and dark tapered gaps remain beside the drain/kerb bands of the apron approaches. No rendered image establishes a complete connected neighbourhood.

No new destination-to-destination traversal is accepted. Existing canonical destinations and entrance supports remain: supermarket market-road-5, salon market-road-2, gym cross-town-road-11, cinema cross-town-road-18, library garden-avenue-13, arts-centre garden-avenue-11, Dance Again market-road-6, and the courtyard/reception/lounge group at civic-street-2. Road detailing does not establish their finished exteriors, interiors or physical end-to-end routes. Interiors were deliberately deferred.

This is editor-authored static world content only. No active game world, capsule traversal, navigation acceptance, working traffic or resident simulation was established. Matching approved native binaries and the existing admitted movement/gameplay owners remain the runtime dependency; no compilation was requested or run. No codec, wallet, saved-state owner, credential handling or readiness gate changed.

## Coverage and next work

world-inventory-reconciled-20261011-pass40.json now prefers current pass40 native evidence, includes the five pass39 corridors and attaches partial apron evidence to four parcels. It retains individual accounting for seven districts, 806 parcels, 264 roads, 19 signatures, 40 destinations, 49 Home recipes, 12 waterfront destinations and five rural areas. No item is marked complete, and historical zero-output records cannot erase later saved evidence.

The next dependency-ready work remains Town Centre, not another district: fitted entrance drain-band shoulders and source junction corners, then exact-floor ordinary pass-by blocks with compound edges and varied silhouettes along the existing corridors. Accepted merged frontage stacks do not satisfy arbitrary canonical floor counts; market-road-10 and market-road-12 especially require correction. Blank backs, sparse services/signage/foliage, warm brown asphalt and the Garden material break remain visual/content defects. Source boundary/coastline constraints still apply to southern ring, bridge, quay and beach work. Do not shorten routes or invent terrain approval.

We preserved unrelated .uproject, Config, native_generator.py, port-document changes, unowned generated assets, old maps/captures and the Blender workbench. Only owned maps, apron assets, scripts, receipts, inputs, captures and inventory are included in this checkpoint. Native-only authoring does not change mobile/web consumed renderer contracts, so renderer packaging and What's New are not triggered. No tests, lint, typechecks, C++ compilation, project generation, full builds, HLOD, app packaging, installs, downloads, deployment or pushes ran. The full-game goal remains active and incomplete.
