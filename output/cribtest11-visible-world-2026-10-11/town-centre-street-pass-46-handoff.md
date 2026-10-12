# Town Centre compound boundaries

We continued from a3e4fdbb48c1d14e76b3ace287a0fe2d4e272c74. Fresh authenticated inspection confirmed running Unreal PID 13584, BaddieCompanion in C:/dev/cribtest-game/apps/companion-unreal/, UE 5.8.3 and the expected clean Town Centre parent before authoring. The previous goal turn was progress: saved apartment shells, corrected overlap and residential entrance paths. This turn adds actual exterior geometry; the full-game goal remains active.

## Saved content

Three canonical apartment parcels gain supported perimeter boundaries and static open pedestrian gates: station-road-4, station-road-10 and college-lane-10. Their source widths, depths, positions, yaw, entrances, building shells and entrance-to-door paths remain unchanged. Canonical source authority remains lagos-city-v3.json SHA256 13a2699ee6ad513616ca25a41bbb05f165e9aeaf3e52b5d89e550ed8b523ad9a. Town Centre remains source X -556.625..573.92 and Z -992.69..28.855 metres; no district or coastline relocation occurs.

Walls sit inside the actual canonical parcel footprint, with supported coping, corner posts and taller entrance posts. Boundary heights vary between 125cm and 145cm before coping. A 280cm gate gap is fitted to the intersection of each retained source-entrance-to-door path and the parcel front. Thin barred gate leaves park perpendicular to the front wall, beside rather than across the path. They are static geometry, not working gates or access controls. Existing apartment doors stay closed and uninteractive.

The read-only camera-plan preparation first failed a canopy/front clearance assertion because the canonical parcel rotation was reversed. No imports or map mutations had occurred. Correcting yaw to degrees(source yaw)-180 matched the source geometry's rotation and axis mapping. Evidence remains in apps/companion-unreal/Saved/Bridge/editor-20261011-181531.log at 2026.10.12-06.48.22 UTC. The corrected planner checks source shell corners against parcel wall clearances before importing. No crash, restart or duplicate request occurred.

Six new native packages:

- /Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Compounds46/Meshes/SM_station_road_4_boundary
- /Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Compounds46/Meshes/SM_station_road_4_open_gate
- /Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Compounds46/Meshes/SM_station_road_10_boundary
- /Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Compounds46/Meshes/SM_station_road_10_open_gate
- /Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Compounds46/Meshes/SM_college_lane_10_boundary
- /Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Compounds46/Meshes/SM_college_lane_10_open_gate

Exact source parcels, road IDs, shell corner readback inputs, gate positions, six actor transforms, material paths, native package hashes, OBJ paths/hashes, unit actor scale and UV probes are in town-centre-street-pass-46-compounds.json. Immutable input directory: compounds46-import-source/. New geometry reuses the existing concrete and painted-steel materials without changing originals. Native centimetres, installed OBJ reflection, U=(Xcm+Zcm)/271,V=1-Ycm/271 and complex-as-simple static collision remain explicit. Source-fitted boundary geometry is not an imported approved complete compound-art asset.

Saved maps:

- /Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Maps/L_town_centre_neighbourhood_20261011_streetpass01
- /Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Maps/L_town_centre_neighbourhood_20261011_streetpass01_detail01

Both maps saved and safely reloaded after module/UObject and Python exception references were released. town-centre-street-pass-46-saved-readback.json checks all six new compound actors and all 24 retained entrance actors, exact level/transforms/unit scales/materials/mesh paths/collision profiles, current native hashes, no dirty packages and no game world. No engine operation remains running.

## Rendered evidence and remaining defects

All three before and three after views in previews/town-centre-compounds46-{before,after} were directly inspected. Camera height 165cm, poses, FOV78 and existing daylight match. Brighter Station4/10 images show the wall coping, supported posts, open gate and unobstructed path to the retained door. New front walls establish a distinct forecourt edge instead of leaving the apartment on undifferentiated ground. No obvious front wall/building intersection is visible. Rear and side wall contact require additional views; these approach images do not establish full-perimeter acceptance.

Walls remain visually plain and repetitive. College Lane10 is heavily shaded, so finish readability is incomplete. Gate hinge/latch/endcap details are unfinished. Narrow side yards are intentional source-fitted clearances, not accepted service routes. The existing path/ground material boundaries and nearby road/drain cuts remain visible. Apartment facade grammar, rooflines/rainwater routes, sparse neighbouring blocks, utilities, services, signs and accepted foliage coverage remain unfinished. Pass42 roof finish/outlet defects, Market floor parity, warm asphalt and Garden material break are unchanged. No interiors were expanded.

No destination-to-destination route is newly completed. Existing supermarket/salon/Dance Again Market Road, cinema/gym Cross Town Road, arts-centre/library Garden Avenue and civic courtyard/reception/lounge source relationships remain preserved. These three pass-by parcels gain compound entrance context, not new approved destinations. Roads, upstream drain transitions and runtime traversal remain incomplete.

world-inventory-reconciled-20261011-pass46.json adds compound evidence to these three parcels and uses current native readback. All seven districts, 806 parcels, 264 roads, 19 signatures, 40 destinations, 49 Home recipes, 12 waterfront destinations and five rural areas remain accounted for without a complete item or percentage claim. Historical zero-output records do not override newer evidence.

Next dependency-ready work remains Town Centre: compound side/rear inspection and gate fittings, richer forecourt finishes and street services, source-defined road/drain shoulders, remaining College Lane shops/townhouse and Arcade Crescent exterior parcels. Reconcile original module/index frontage before new shell placement. Do not use rejected foliage or import unfinished art for density.

This is static editor-authored content only. Matching approved native binaries, existing admitted gameplay entry/movement/control owners, character bindings and navigation integration remain the runtime dependency. No capsule traversal, working traffic/residents, interactive gates, account access or mission readiness is asserted. No gameplay owner or codec changed.

Only owned scripts, six assets, two maps, immutable inputs, receipts, six captures, inventory and handoff are checkpointed with hooks disabled. Unrelated .uproject, Config, native_generator.py, port-document changes, unowned assets and Blender workbench remain untouched. Native-only world authoring changes no consumed mobile/web renderer contract. No tests, lint, typechecks, compilation, full builds, HLOD, packaging, deployment, installs/downloads, production configuration changes or pushes ran.
