# Town Centre exact-floor exterior shells

We continued from 175ebe28882021a3d213d3a1aed5d5d8fffa7a55. Current inspection confirmed PID 13584, BaddieCompanion, Unreal 5.8.3 and the clean expected Town Centre parent. The previous goal turn made saved progress. No worker, editor launch, restart or prohibited validation command ran.

## New native content

Six non-destination shop parcels now have full-depth exterior shells: station-road-3 (3 floors), station-road-7 (1), station-road-8 (3), college-lane-6 (1), college-lane-7 (1), college-lane-17 (2). Before authoring, native tags showed only their existing source footprint and entrance support. Nothing was removed. Canonical centres, entrance positions, inward orientation and travel relationships remain unchanged. Width and depth use whole 3.9m bays inside source parcel dimensions; the resulting setback is recorded rather than moving the entrance.

We replaced the merged-pack authoring approach for these new parcels with source-fitted concrete frames, continuous slabs, perimeter posts and floor beams at 3m intervals. Individual accepted v3 wall bays, shutters, window variants, balconies and selected AC assemblies retain source hashes, pivots, UVs, material bindings and unit scales. Side and rear walls have windows rather than blank backs. All ground-floor front shutters remain closed pass-by shops. No new business identity, interior destination or activity simulation was invented.

Six new native frame packages use /Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Blocks41/Meshes/SM_{station_road_3,station_road_7,station_road_8,college_lane_6,college_lane_7,college_lane_17}_frame. Exact packages, source IDs, immutable OBJ hashes and 462 actor transforms are in town-centre-street-pass-41-parcel-frontage.json. Frame UV0 uses U=(Xcm+Zcm)/271, V=1-Ycm/271, verified against the installed OBJ importer. This avoids collapsed vertical UVs but remains measured provisional frame artwork, not final architectural surfacing. The import helper's new option defaults off, preserving older crossing/apron geometry. Static frame triangle collision and BlockAll component profiles are authoring inputs, not approved walking.

Saved maps:

- /Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Maps/L_town_centre_neighbourhood_20261011_streetpass01
- /Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Maps/L_town_centre_neighbourhood_20261011_streetpass01_detail01

Both maps were saved and safely reloaded after all transient authoring, module and exception references were released. town-centre-street-pass-41-saved-readback.json verifies 462 actor transforms, materials, unit scales, six exact roof/floor heights, 7,164 frame vertex-instance UVs, retained entrance support, native file hashes, clean packages and no active game world. No request remains live.

## Rendered evidence

We directly inspected both matched before road views and all fourteen after images: six frontages, six pedestrian-height corner views and the two matched road views. Sets: previews/town-centre-blocks41-before and previews/town-centre-blocks41-after. Native camera poses, FOV, frame warmup and hashes are recorded beside each PNG. Close-front cameras crop the wider facades; corner views supply their full silhouettes and side-wall context.

Station Road's matched view now shows nearby one- and three-storey building mass on both sides. College Lane's source-centre view is unchanged because the new parcels are elsewhere along that road; we did not relocate source buildings for a screenshot. Corner images confirm full-depth shells and grounded wall bases. They do not show a complete neighbourhood. Long gaps between parcels, uniform ground, all-closed shutters, plain roofs and missing local services still prevent Lagos-density visual acceptance. No new destination or end-to-end route is marked complete.

Roof finish and rainwater drainage, entrance/forecourt boundaries, shop signage and everyday services remain unfinished. The older Market Road floor-count defects remain: this batch solves parity only for these six new parcels. Existing warm brown asphalt, Garden material discontinuity, unfinished junction/entrance shoulders, covered unexcavated drains and boundary/shore dependencies persist. Interiors remain deferred.

world-inventory-reconciled-20261011-pass41.json retains all seven districts, 806 parcels, 264 roads, 19 signatures, 40 destinations, 49 Home recipes, 12 waterfront destinations and five rural areas. It attaches exact-floor partial shell evidence to the six parcels and uses current saved map evidence. No complete item or whole-city acceptance is inferred.

Next dependency-ready ownership remains Town Centre: finish these roof/rainwater/forecourt treatments, add neighbouring source parcels with varied silhouettes, and repair source-defined junction/entrance transitions before interiors. Current runtime still requires matching approved native binaries and the existing admitted movement/gameplay owners. No active game world, working traffic, residents, navigation or capsule traversal was verified.

Unrelated .uproject, Config, native_generator.py, port-document work, unowned assets, captures and Blender workbench remain preserved. Only owned maps, frame assets, scripts, source inputs, receipts, captures and inventory are checkpointed with hooks disabled. Native-only content did not alter mobile/web renderer contracts or registries, so renderer packaging and What's New are not triggered. The full-game goal remains active and incomplete.
