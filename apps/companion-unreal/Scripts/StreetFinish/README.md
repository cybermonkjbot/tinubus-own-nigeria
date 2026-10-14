# Library Lane native street tools

The Windows editor has a saved, additive street-dressing pass. Open **Tools > Baddie World > Open Library Lane**. The same menu provides the reviewed street, frontage and reverse cameras, selection of both generators, and regeneration. Regeneration does not save automatically. Startup only registers the menu; it never opens a map or generates content.

- Parent: `/Game/BaddieGeneratedWorld/StreetFinish_20261014/Maps/L_TownCentre_LibraryLane`
- Additive level: `/Game/BaddieGeneratedWorld/StreetFinish_20261014/Maps/L_LibraryLane_Dressing`
- Graphs: `PCG_LibraryLane_Everyday` and `PCG_LibraryLane_PlantingAndShade` under the same root's `PCG` folder.
- Output: 23 everyday/fixture instances and 47 planting/shade instances, using CPU PCG and HISM components. Both persist across map reload.

Edit the points and mesh spawners in the saved native graphs, then use Generate on the PCG component or the menu. The authored point lists keep the existing central shop approaches and road corridor clear. No new collision, navigation, service activation, NPC or traffic authority is added. This is scenery, not completed gameplay.

## Source tools

`build_street.py` creates the parent-map copy, additive level and everyday graph. It refuses to overwrite completed graphs or existing maps. `add_canopy_pass.py` adds the planting/shade graph. These are initial-authoring scripts, not a replacement for accepted native graph edits. Run only against the intended, already-open Windows editor.

`export_worldkit.py` converts an archived source or reconstructs only six selected assets from retained recipes in a dedicated background Blender process. Reconstruction is explicitly distinguished from the absent archived v06 `.blend`. It writes FBXs with three LODs, an inspection manifest and a Blender source copy; it never renders or modifies the source recipes. The October 14 Blender library write emitted a material loopback warning; that optional `.blend` copy has not been certified portable. Native FBXs and imported packages are the inspected deliverables.

`import_worldkit.py` imports or resumes only hash-matching owned native assets. It validates centimetre bounds and each LOD's triangle count and binds the additional material-name aliases introduced by Unreal's LOD importer. Default acceptance excludes the freestanding vendor canopy: its imports lost 64 triangles per LOD. The accepted corrugated wall awning supplies this pass's shade. The original canopy recipe and FBXs remain for repair.

The staged secondary-detailing importer also needed the installed `StaticMesh.get_num_triangles` API, an explicit FBX factory and bounded asset selection. Its original and corrected copies are retained in `output/unreal-street-detail-20261014/source` on the Mac. The VM's existing V1 importer is corrected in place.

Use named `Rotator(pitch=..., yaw=..., roll=...)` arguments with this engine's Python bindings. Positional arguments use the underlying `MakeRotator` order. Use the active viewport configuration key for viewport camera methods. `save_map` writes a copy without switching the open world; explicitly load the new parent before authoring its child-level references.

## Verified scope

October 14, 2026: UE `5.8.3-58210709`, editor PID 13584. Both graphs executed without reported node errors. The saved parent was reopened and all 70 upright instances remained. Actual editor images cover the original street camera, close shopfronts and a reverse view. A same-camera layer-hidden capture isolates the scenery changes. Original town-centre map files remain unchanged in git.

The new parent uses the built-in daylight ambient cubemap on its copied skylight. Original lighting assets and maps are not overwritten. The VM still exposes Microsoft Basic Render Driver with D3D11 SM5. No engine restart, VM reconfiguration, new preview server, test suite, C++ build, app packaging or release was performed. PIE, navigation/collision integration, target-device performance and the full game port remain unverified.

Full receipts and images: `output/unreal-street-detail-20261014`. Native packages and the Tools menu are in the Windows checkout; these Mac source files alone do not install the generated world into a mobile renderer.
