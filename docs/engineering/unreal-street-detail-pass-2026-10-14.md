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

## Continuation: planting, street edges and junction surface

Joshua requested continued implementation: “keep going, theres so much more to do”. The same UE 5.8.3 editor, PID 13584, was reconfirmed before native work. Initial open-map state retained the first 70 instances and no dirty maps. Existing unrelated source/config changes and four already-dirty material packages were left alone.

The Blender production workflow shaped a targeted foliage correction: retain the accepted trunk/root scale and add connected leaf-bearing shoots, rather than overlap whole trees or inflate leaf size. An isolated Blender 5.2.2 worker (PID 5816) reconstructed the retained recipes and exited after exporting two variants. Native imports match source bounds and all three LOD counts: tree 154,288 / 77,050 / 30,857 triangles; shrub 86,296 / 43,148 / 17,258. They use separate `Foliage2` materials and packages. The optional Blender library write again emitted the loopback warning; its portability is not accepted. FBXs and native packages are the inspected assets.

`PCG_LibraryLane_PlantingAndShadeFull` is a clone of the accepted planting graph. Only its tree/shrub references changed; all 47 authored transforms and the original graph are retained. The new `L_LibraryLane_StreetEdges` layer contains 117 additional instances: ten trees, 30 shrub clusters, 15 raised soil/perimeter beds and two benches. Roadside placement follows existing segment 6/7 support transforms, with 220 cm centre-to-asphalt-edge separation, a 140 cm total bed width and preserved inward pedestrian space. Canonical entrance corridors and existing tree positions are excluded. Two candidate beds at segment 7 fraction 0.13 were omitted after corner traces found a 15 cm height change; no ground was flattened to force them in. Side-return gardens and seating were inspected directly in the editor.

The large pale patch visible in the first street camera was diagnosed independently of foliage: the park path/approach and road segment 5/6 asphalt occupy Z=0 at the same positions. Native multi-hit trace evidence is retained in `surface-diagnostics.json`. The new two-point `PCG_LibraryLane_SurfaceSeparation` graph reproduces exactly those asphalt meshes, XY transforms and material, adding 1 cm of render separation. Its instances have no collision, navigation effect or shadow casting. The same-camera capture confirms the patch is gone. Original road and park-path packages are unchanged; this is an additive visual repair, not a redesign of their routes.

The parent was saved and reopened, retaining **189 actual HISM instances**: Everyday 23, PlantingAndShadeFull 47, StreetEdges 117 and SurfaceSeparation 2. Each count matches its saved graph's authored points. Total greenery/seating is now 16 trees, 32 shrub clusters and four benches. `extension-final-state.json` records the reopened world, each actual transform, active material bindings, LODs and collision/navigation state. Original connected-town-centre maps and the original planting graph have no git diff.

The native Tools menu now opens each active graph, provides six review cameras and can hide/show all added scenery for comparison. The visibility controls were exercised with no dirty maps; the graph shortcut opened the exact StreetEdges asset. The graph editor was closed afterward, leaving the dressed street available. Regeneration remains explicit and does not save automatically.

Evidence: `extension-before.png`, `extension-after.png` (before road repair), `extension-final.png`, `extension-hidden.png`, `extension-close.png`, `extension-reverse.png`, `extension-continuation.png` and `extension-side-garden.png`. The added garden/foliage received direct multi-view inspection, not a synthetic mock-up. Static views do not establish animated foliage stability, LOD transition quality, collisions, navigation, target-device performance or complete gameplay. Bare surrounding parcels and other pre-existing street-edge artifacts still need further work. No tests, app/C++ builds, renderer packaging, release, VM hardware changes or additional plugins were performed.
