# Unreal street detail pass

October 14, 2026. Native scenery pass saved and reopened; full port completion remains outstanding.

Joshua authorized the proposed existing-street completion pass and useful Unreal tooling: “Please lets do it, add any tools in unreal that make things easy”. Scope is native content authoring, import, saved PCG/instancing and inspection in the already-running Windows editor. No app build, tests, engine compilation, release, VM resize or production renderer changes.

## Protected baseline

- Project: `C:/dev/cribtest-game/apps/companion-unreal/BaddieCompanion.uproject`.
- Engine: `5.8.3-58210709+++UE5+Release-5.8`; editor PID 13584 at preflight.
- Existing map: `/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Maps/L_town_centre_neighbourhood_20261011_streetpass01`.
- Existing player-height comparison camera: position `[-58687.457698792343, -28530.809150025372, 165]`, pitch `-5`, yaw `-54.402858734130859`, FOV 78.
- Library Lane frontage parcels 9 and 10 are the bounded first pass. Existing parcel, entrance, road and saved Home identities stay unchanged. New decorative points do not activate services, utility power, pedestrians or vehicle mechanics.
- New work belongs under `/Game/BaddieGeneratedWorld/StreetFinish_20261014`. Existing maps, materials and meshes are not overwritten. Remote repository already has unrelated modified configuration/source and many untracked generated assets.

## Reused content and tools

Native PCG, PCG Primitives, Geometry Scripting, editor Python and material/mesh tools are already enabled. Secondary detailing kits V1–V4 exist under `output/cribtest11-visible-world-2026-10-11` on Windows but were not imported. Read their README, handoff, manifest and assembly rules before selecting content. Preserve three authored LODs, material slots, metric dimensions, source hashes and placement clearances.

The shared V1 importer used unavailable `StaticMeshEditorSubsystem.get_number_triangles`; the installed replacement is `StaticMesh.get_num_triangles`. Its original SHA-256 was `d3fc3fae35059a3c6f04b6623bfeb24791c0d6eb246ccd79a7656bb2190d33e6`. The original is retained locally under `output/unreal-street-detail-20261014/source/import_unreal.original.py`. The correction also selects the native FBX factory explicitly and supports bounded asset selection within an existing batch.

Initial shop-life import accepted nine mesh/LOD counts before rejecting `SM_CounterScale_A`: source LOD0 308 triangles, imported 244. The counter scale is excluded pending source repair; no failed mesh is silently accepted. Its original FBXs and immutable failure receipt remain available. Completion and rendered evidence will be appended after authoring.

## Delivered

- Saved parent `/Game/BaddieGeneratedWorld/StreetFinish_20261014/Maps/L_TownCentre_LibraryLane`, referencing original town-centre levels and the new `L_LibraryLane_Dressing` child. Original connected town-centre map files have no git diff.
- Two CPU PCG graphs with 70 persisted HISM instances: 23 everyday/fixture points and 47 planting/shade points. These include six street trees, five wall awnings, two benches, two low planted clusters, raised planting edges, chairs, stools, crates, coolers, bins and five unpowered wall-light housings.
- 29 accepted native meshes, each retaining three LODs: 15 shop-life, four household, five fixtures, and five reconstructed world-kit meshes. Imported assets not used by this particular composition remain available in its content folder.
- Native **Tools > Baddie World** menu: open the new map, three review cameras, select generators, regenerate dressing. Startup registration does not open or generate anything. No additional paid or experimental plugin was installed.
- The copied parent skylight uses the built-in daylight ambient cubemap at intensity 0.65. No original lighting actor package is overwritten.

## Corrections and evidence

The v06 world-kit manifest was present, but its `.blend` was absent from both checkouts and the checked asset directories. The existing Blender workbench PID 3224 was listening without answering; it was left untouched. A bounded, separate Blender 5.2.2 conversion process (PID 17212, now exited) reconstructed selected assets from retained authoring recipes. This is not represented as a byte-identical archived v06 conversion. The optional `.blend` write emitted a material loopback warning and remains unverified; native FBXs and packages were inspected separately.

The freestanding vendor canopy lost 64 triangles per LOD during import. Its newly created, unreferenced rejected native asset was removed; the original FBXs, recipe and failure receipt are retained. The rejected counter scale was handled the same way. Neither asset is placed in the street.

Live binding checks caught three important compatibility issues: positional `Rotator` arguments map differently from their presumed order; inherited editor properties require explicit property setters; FBX LOD import adds normalized material-name aliases. All 70 saved points were corrected to upright yaw-only rotations. Every LOD material alias on the five world-kit meshes was rebound to the intended source material. Native LOD triangle counts and centimetre bounds match the documented handedness conversion.

`SaveMap` writes a copy without changing the currently open world. The temporary addition to the original world's in-memory level list was discarded without saving that world. The new parent was explicitly loaded, its dressing level registered and both owned maps saved. Reopening it retained both generated graphs and all 70 instances (14,151 total loaded actors). This was an editor reload, not a test build or PIE session.

Actual captures in `output/unreal-street-detail-20261014/evidence` cover the original camera, a close frontage view and a reverse view. The layer-hidden comparison retains the same camera and underlying scene. Close inspection confirms upright props, tree/soil contact, wall-mounted awnings and clear central shop approaches. It does not establish every underside, gameplay collision or target-device performance.

## Remaining scope

The VM still renders with Microsoft Basic Render Driver, D3D11 SM5. Hardware-dependent features and the existing SM6/Nanite warning are not resolved by importing props. No hardware, VM settings or rendering backend was changed.

This is a bounded scenery pass, not a completed game port. NPCs, traffic, interactions, functional utilities, gameplay collision/navigation, fuller vegetation art and target-device performance remain separate work. The new map is accessible through the menu; project startup and production renderer registrations are unchanged. No tests, C++ compilation, app builds, release, R2 delivery or mobile renderer packaging were run.
