# Town Centre shoulder refinement

October 12, 2026. We continued 485653edefa3fca9b5a720a57e23ee3e35cf4ea6. The previous goal turn made saved road-entrance progress. Fresh process/authenticated discovery confirmed Unreal PID 13584, BaddieCompanion in C:/dev/cribtest-game/apps/companion-unreal/, UE 5.8.3. Authoring guarded the expected map, project and clean packages. No restart or other worker was used. The full-game goal remains active and incomplete.

## Saved correction

Replaced only the three pass47 shoulder actors at station-road-4, station-road-10 and college-lane-10 with finer source-fitted meshes. The original footprint polygons, bilinear height profile, source entrances, transforms and material bindings stay unchanged. Fifteen-centimetre clipped cells replace the coarse triangle fans; exterior sides retain support down to -12cm. All nine central ramp/deck actors remain unchanged. Original native shoulder packages and failure previews are preserved rather than overwritten.

New exact packages:

- /Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Shoulders48/Meshes/SM_station_road_4_shoulders
- /Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Shoulders48/Meshes/SM_station_road_10_shoulders
- /Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Shoulders48/Meshes/SM_college_lane_10_shoulders

town-centre-street-pass-48-refined-shoulders.json records source identity, three removed and three new actor records, exact transforms/materials, unchanged profile references, grid step, triangle counts, native hashes, immutable OBJ paths/hashes and UV probes. Input directory: shoulders48-import-source/. Native centimetres, installed OBJ axis reflection, U=Xcm/271,V=1-Ycm/271, unit actor scales and complex-as-simple static collision are retained. Existing concrete material source is unchanged.

Saved maps:

- /Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Maps/L_town_centre_neighbourhood_20261011_streetpass01
- /Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Maps/L_town_centre_neighbourhood_20261011_streetpass01_detail01

Both maps saved and safely reloaded after transient module/UObject and Python exception references were released. town-centre-street-pass-48-saved-readback.json verifies three replacements, absence of the three superseded actors, nine retained ramp actors, exact levels/transforms/scale/material/mesh/collision bindings, current native hashes, no dirty packages and no game world.

## Actual image review

All three images in previews/town-centre-shoulders48-after were directly inspected against retained pass47 approach views. Camera height 165cm, pose, pitch -5, FOV78 and existing daylight match. Station Road4 and Station Road10 show substantially reduced coarse shoulder facets while keeping the central ramp, compounds and buildings unchanged. College Lane10 stays heavily shaded, so full finish acceptance remains incomplete. Thin seams, plain concrete, dark kerb/drain-end gaps, road ribbon seams and brown asphalt remain. No close-up end-contact, drainage structure or runtime traversal acceptance is established.

The first capture attempt stopped before any image because the generic helper routed receipt camera IDs to its legacy fixed list and raised StopIteration. This was capture preparation, not an editor crash or failed map save. Evidence remains in editor-20261011-181531.log at 2026.10.12-07.00.29 UTC. The owned capture wrapper uses current saved readback plus explicit matched camera poses through the existing direct-pose route. All subsequent captures completed; no requests remain live.

Town Centre remains the selected district, with source X -556.625..573.92 and Z -992.69..28.855 metres. Canonical plan SHA256 remains 13a2699ee6ad513616ca25a41bbb05f165e9aeaf3e52b5d89e550ed8b523ad9a. No district, road, parcel, destination or coastline placement changed. This is correction of existing entrance support, not a new neighbourhood or destination-to-destination route.

world-inventory-reconciled-20261011-pass48.json adds refinement evidence for these three parcels and current readback while preserving older evidence. All seven districts, 806 parcels, 264 roads, 19 signatures, 40 destinations, 49 Home recipes, twelve waterfront destinations and five rural areas remain accounted for. No complete status or completion percentage is claimed.

Next dependency-ready work stays Town Centre: close-up drain/end contacts, compound side/rear inspection and gate fittings, varied adjoining College Lane/Arcade Crescent exterior fabric and street services before interiors. Apartment repetition/roof/rainwater, sparse surrounding blocks, plain boundaries, missing utilities/signs/accepted foliage and prior Market/Garden/pass42 defects remain. No new reference delivery is required for the next independent exterior correction; rejected art remains excluded.

This is editor-authored static world geometry only. Matching approved native binaries, existing admitted entry/movement/control owners, character bindings and navigation integration remain the runtime dependency. No gameplay owner, codec, wallet/save, credential or authority/readiness gate changed. No working traffic/residents, interactive doors/gates, account access or playable route is asserted.

Checkpoint only owned scripts, three assets, two maps, inputs, receipts, three captures, inventory and this handoff with hooks disabled. Preserve unrelated .uproject, Config, native_generator.py, port-document changes, unowned assets, old maps/captures and Blender workbench. Native-only authoring changes no consumed mobile/web renderer contract. No tests, lint, typechecks, compilation, full builds, HLOD, app packaging, install/download, deployment, production configuration changes or pushes ran.
