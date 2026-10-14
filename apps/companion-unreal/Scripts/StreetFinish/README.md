# Library Lane native street tools

The Windows editor has a saved, additive street-dressing pass. Open **Tools > Baddie World > Open Library Lane**. The menu provides six review cameras, direct editing of the active graphs, generator selection, regeneration and temporary hide/show comparisons. Regeneration does not save automatically. Startup only registers the menu; it never opens a map or generates content.

- Parent: `/Game/BaddieGeneratedWorld/StreetFinish_20261014/Maps/L_TownCentre_LibraryLane`
- Additive levels: `L_LibraryLane_Dressing` and `L_LibraryLane_StreetEdges` under that root's `Maps` folder.
- Active graphs: `PCG_LibraryLane_Everyday`, `PCG_LibraryLane_PlantingAndShadeFull`, `PCG_LibraryLane_StreetEdges` and `PCG_LibraryLane_SurfaceSeparation` under its `PCG` folder. The original `PCG_LibraryLane_PlantingAndShade` remains unchanged as a fallback.
- Output: **189 persisted instances** across four CPU PCG generators: 23 everyday/fixture, 47 planting/shade, 117 street-edge and two render-surface instances.
- Vegetation/seating total: 16 trees, 32 shrub clusters and four benches. The continuation adds ten trees, 30 shrub clusters, 15 edged beds and two benches. Two proposed roadside beds were omitted because their corners crossed uneven existing ground.

Edit the points and mesh spawners in the saved native graphs, then use Generate on the PCG component or the menu. The authored point lists keep the existing central shop approaches and road corridor clear. No new collision, navigation, service activation, NPC or traffic authority is added. This is scenery, not completed gameplay.

## Source tools

`build_street.py` creates the parent-map copy, additive level and everyday graph. It refuses to overwrite completed graphs or existing maps. `add_canopy_pass.py` adds the planting/shade graph. These are initial-authoring scripts, not a replacement for accepted native graph edits. Run only against the intended, already-open Windows editor.

`export_fuller_foliage.py` uses the retained recipes to add connected leafy shoots to the existing tree/shrub structure without enlarging individual leaves or duplicating trunks. Run it with the same source/destination arguments as `export_worldkit.py`, in an isolated Blender worker. The bundled read-only `probe_scene.py` records that worker's identity before authoring. Native-only `Foliage2` imports retain three LODs; the shared mobile renderer source and accepted original meshes are unchanged.

`use_fuller_foliage.py` clones the accepted planting graph and swaps only its tree/shrub references, retaining all 47 point transforms. `add_street_edges.py` creates a new additive layer using the actual road-support and entrance transforms, ground traces and clearance exclusions. Both refuse to overwrite completed work. `inspect_street.py` reads actual generated instances, transforms, LODs, material bindings and collision/navigation flags; it never generates or saves a map.

`separate_road_surface.py` addresses the inspected coplanar overlap at the park approach. It reproduces the exact segment 5/6 asphalt footprint and material 1 cm above the original render surfaces, with collision and shadow casting disabled. This removes the pale junction patch without rewriting the original roads, park path or traversal data. It is a reversible native render repair, not a source-layout migration.

`export_worldkit.py` converts an archived source or reconstructs only six selected assets from retained recipes in a dedicated background Blender process. Reconstruction is explicitly distinguished from the absent archived v06 `.blend`. It writes FBXs with three LODs, an inspection manifest and a Blender source copy; it never renders or modifies the source recipes. The October 14 Blender library write emitted a material loopback warning; that optional `.blend` copy has not been certified portable. Native FBXs and imported packages are the inspected deliverables.

`import_worldkit.py` imports or resumes only hash-matching owned native assets. It validates centimetre bounds and each LOD's triangle count and binds the additional material-name aliases introduced by Unreal's LOD importer. Default acceptance excludes the freestanding vendor canopy: its imports lost 64 triangles per LOD. The accepted corrugated wall awning supplies this pass's shade. The original canopy recipe and FBXs remain for repair.

The staged secondary-detailing importer also needed the installed `StaticMesh.get_num_triangles` API, an explicit FBX factory and bounded asset selection. Its original and corrected copies are retained in `output/unreal-street-detail-20261014/source` on the Mac. The VM's existing V1 importer is corrected in place.

Use named `Rotator(pitch=..., yaw=..., roll=...)` arguments with this engine's Python bindings. Positional arguments use the underlying `MakeRotator` order. Use the active viewport configuration key for viewport camera methods. `save_map` writes a copy without switching the open world; explicitly load the new parent before authoring its child-level references.

## Verified scope

October 14, 2026: UE `5.8.3-58210709`, editor PID 13584. The first pass saved 70 instances; the continuation now reopens with all 189 instances and matching native graph-point counts. Graph execution reported no node errors. Actual images cover the original street camera, close shopfronts, reverse direction, continuation and side-return garden. A same-camera all-dressing-hidden capture isolates the changes, including the road repair. Hide/show was exercised without dirtying maps, and the street-edge graph shortcut opened the intended native PCG asset. Original town-centre maps and the original planting graph remain unchanged in git.

The new parent uses the built-in daylight ambient cubemap on its copied skylight. Original lighting assets and maps are not overwritten. The VM still exposes Microsoft Basic Render Driver with D3D11 SM5. No engine restart, VM reconfiguration, new preview server, test suite, C++ build, app packaging or release was performed. PIE, navigation/collision integration, target-device performance and the full game port remain unverified.

Full receipts and images: `output/unreal-street-detail-20261014`; continuation state is in `extension-final-state.json`, accepted imagery includes `evidence/extension-final.png`. Initial-authoring receipts retain their original pending flags; the later reopened-state receipt and engineering record document acceptance. Native packages and the Tools menu are in the Windows checkout; these Mac source files alone do not install the generated world into a mobile renderer. Surrounding parcels still need broader environment work; this is not a finished town or game port.
