# Native adjoining buildings and ground

October 14, 2026. Cribtest11, UE 5.8.3-58210709, editor PID 10404 after the requested VM restart. This is additive native environment work, not a finished town or completed game port.

## Saved result

Parent: `/Game/BaddieGeneratedWorld/StreetFinish_20261014/Maps/L_TownCentre_LibraryLane`.

New level: `L_TownCentre_Adjoining`. Existing original maps, completed forecourts, park biomes and street dressing remain referenced and unchanged. The selected plots had native footprint/entrance supports but no tagged newer exteriors or retained coarse frontage actors. Landmark parcels were excluded.

| Parcel | Source floors | Saved exterior instances |
| --- | ---: | ---: |
| Unity Avenue 17 | 8 | 463 |
| Unity Avenue 19 | 2 | 94 |
| Unity Avenue 20 | 7 | 413 |
| Unity Avenue 21 | 9 | 403 |
| Unity Avenue 22 | 9 | 484 |
| Theatre Street 2 | 2 | 75 |
| Theatre Street 4 | 2 | 110 |
| Theatre Street 5 | 2 | 102 |
| Theatre Street 6 | 6 | 307 |
| Total | | 2,451 |

One shared CPU facade grammar consumes 988 editable, variant-tagged native spline runs. Nine separate anchor graphs place fitted structural pieces, closed residential portals, source-door links, roofs, gutters and downpipes. The earlier accepted modular frontage assets are reused; 37 new parcel-fitted meshes preserve native centimetre bounds, UV conventions and exact imported triangle counts. These fitted meshes retain the existing single-LOD import contract; no new Nanite or performance claim is made.

Sixteen non-colliding ground surfaces cover approximately 13,542 m²: nine fitted garden/apron surfaces and seven connected verge sections on Unity Avenue and Theatre Street. Original road shoulders, entrance strips, other parcel footprints and the new building envelopes are excluded geometrically. This is flat render-surface treatment, not a Landscape, heightfield, slope repair or terrain streaming implementation.

The ground graph adds 441 instances: 266 grass clumps, 106 groundcover clusters, 23 shrubs, 18 fern pots and 28 planted-bed surrounds. These are deterministic authored PCG points, not a new biome-density system. The two existing native park biomes remain the rule-based biome layers. One candidate planting point with no horizontal support was rejected and recorded; no plant was forced onto that unsupported point.

## Persistence and controls

The saved parent was reopened successfully. Readback verified all 2,451 exterior instances, exact mesh/material grouping, rotations/scales, 988 spline lengths/endpoints, all 16 ground surfaces and all 441 planting/detail transforms. Maximum source-position error was approximately 0.000558 cm. Every protected `TC_` actor transform remained unchanged. All 5,017 earlier scenery instances remain, and the park boundaries still contain 12 and 14 points. Dirty map and content-package lists were empty after reopening.

All new generated components and ground actors have collision and navigation contribution disabled. Closed doors and rendered street detail do not establish interiors or playable routes.

`Tools > Baddie World > Adjoining buildings` exposes walking-height views, building/spline selection, shared facade-grammar editing, separate per-building anchor editing, regeneration and temporary hide/show. Opening Theatre Street 2's anchors resolved to its actual native graph. Hide/show restored visibility without dirtying the map. Regeneration does not save automatically. The shared facade action is labeled explicitly because changing its grammar affects all nine buildings.

## Evidence and source

`output/unreal-adjoining-20261014/evidence` retains nine initial before/after pedestrian pairs at FOV 78 and three full-height roof/ground views. Its `matched` folder holds three settled comparison pairs at a common viewport size; their before views hide only this pass's 26 actors, then restore all of them for the after views. The comparison sheet uses those unchanged screenshots. The initial Unity Avenue 19 after capture contained a transient missing distant facade; recapture and native readback confirmed it was intact. The earlier pass-65 and pass-68 exteriors retain all 861 and 1,150 instances respectively. Wider views show stronger coverage along the selected streets, while also exposing the remaining sparse surroundings, repetitive facades, dark shadow-facing frontage and broad tiled ground beyond the new surfaces.

`survey.json` records the exact source parcels, accepted module rows, native support masks and protected fingerprint. `imported.json`, `installed.json`, `ground-prepared.json` and `ground-installed.json` retain authoring receipts. `reopened-state.json` and `ground-reopened-state.json` record persisted native state. Initial pending-generation flags in installation receipts are historical, not final status.

The parcel reference remains pinned to commit `e978763b74acfb0369b65b879af6a3a8c90b110b` and city JSON SHA-256 `13a2699ee6ad513616ca25a41bbb05f165e9aeaf3e52b5d89e550ed8b523ad9a`. The bounded native authoring uses the existing accepted facade/roof/import helpers and checks native supports against these source records. It does not run the guarded whole-city generator or resolve its outstanding manifest/dispatch discrepancy.

`add_adjoining.py` separates survey, fitted import, installation, generation, audit and save. It refuses occupied plots and completed-layer replacement. `add_adjoining_ground.py` clips convex ground pieces, imports source-fitted surfaces and verifies persisted planting. The initial ground installation stopped at the unsupported planting point before generating vegetation; its guarded recovery reused only the exact sixteen already-created ground actors. No source actor was removed.

The Windows-only accepted helper dependencies remain under `Scripts/WorldGeneration/Connected`. Their source revisions and fitted OBJ files are retained in the VM checkout. New source files and evidence are also retained on the Mac. No shared mobile renderer, registry or asset contract changed, so mobile renderer packaging was not triggered.

No tests, lint, typechecks, C++ compilation, HLOD generation, PIE, app packaging, release or push were run. No third-party plugin was purchased, installed or integrated. Broader coverage, material/lighting quality, continuous terrain and authorized traversal/performance work remain outstanding.
