# Tree diversity and open-drain street kit

October 14, 2026. We authored original mesh source assets in response to repeated tree silhouettes and missing exposed roadside gutters. This is an isolated native asset kit, not a change to shared mobile scene data.

## Assets Delivered

Six different tree models have separately authored trunk/branch/leaf geometry and three explicit LODs each:

| Model | Shape | Near / middle / far triangles |
| --- | --- | --- |
| Tiered broadleaf | Separated horizontal branch tiers and broad leaves | 57,708 / 30,598 / 16,022 |
| Rounded dense broadleaf | Rising branches and compact crown | 65,916 / 34,942 / 18,286 |
| Wide fine leaf | Wide spread, exposed branching and small leaves | 55,656 / 29,512 / 15,456 |
| Upright small broadleaf | Narrower crown and rising branches | 49,500 / 26,254 / 13,758 |
| Leaning feather palm | Curved trunk with long pinnate fronds | 5,108 / 3,336 / 2,484 |
| Upright feather palm | More vertical trunk and shorter distinct crown | 4,532 / 2,960 / 2,204 |

Leaves attach to modeled shoots/twigs or frond midribs. These are generic morphotypes, not botanical replicas or claims about trees on a particular street. Current material inputs are authored colors/roughness with opaque two-sided foliage; texture, wind and botanical acceptance remain pending. Triangle counts are generated source counts, not a performance result; dense near crowns should not become default high-density street planting.

Six drain/pavement meshes provide an open trough, localized sediment trough, supported entrance bridge, partially covered channel, raised tiled sidewalk and removable lifting-loop cover. The trough has a real bottom and sidewalls with no top face across its opening. The catalog includes maintained and localized-deposit variants without assigning waste or flooding to every street.

Original source is `apps/companion-unreal/Scripts/StreetFinish/tree_drain_geometry.py`. `author_tree_drain_kit.py` writes content-addressed OBJ/MTL files and geometry/manifest JSON. `tree_drain_blender_source.py` creates an editable source library in an isolated factory-startup Blender process. `import_tree_drain_kit.py` is the explicit, PID-bound native importer; it is not run automatically and does not change maps.

## Files

Folder: `output/unreal-street-detail-20261014/tree-drain-kit/6376944f38f4dbca`.

Geometry SHA-256: `6376944f38f4dbca47ee95f9fba13d167c49a1657c5b3d55c691154490dc249c`.

The folder contains 24 OBJ meshes, one MTL, full geometry JSON, per-file hash/bounds/triangle manifest and `TreeDrainKit_Source.blend`. The Blender library places assets on a display grid in metres; object-local geometry retains the tree-ground or drain-rim pivot. Native OBJ source is centimetres and precompensates the existing installed importer's Y reflection and UV-V reversal. Native axis/material/LOD readback remains pending.

Research and source rights are recorded in `docs/world-building/lagos-mosaic-2026-10-11/tree-drain-reference-2026-10-14.md`. A University of Lagos checklist and inspected photographer-uploaded references support a varied candidate palette and open-channel families. They do not establish measured narrow gutter sections at the authored street. We copied no photograph, texture or third-party model into the kit.

## Street Integration Contract

Dimensions are game-design values, not a civil survey. Module X runs along the channel, Y crosses it and Z is up. A 2 m trough has 90 cm outside width, 66 cm clear opening, 12 cm walls and a bed 65 cm below the rim. An entrance bridge spans both wall tops with a 15 cm thick deck. Its length along the street is 2 m, so the crossing has a broad continuous surface rather than a stepping stone.

Example local assembly uses channel rim Z=0, bridge top Z=15 cm, and the raised sidewalk's object origin Z=10 cm, making its 5 cm tile tops meet the deck at Z=15 cm. Place the 2 m-wide sidewalk beside the channel with its Y=0 edge at the channel's Y=45 cm outside edge. Road and doorway approaches need separately fitted level changes; the kit does not claim to contain their ramps or driveway load-bearing design.

- Do not overlay a negative-depth trough on an uncut support slab. The existing slab must be replaced or scoped to exclude the channel volume; a flat ground plane would otherwise fill the opening.
- Keep the drain outside the clear walking corridor. Use bearing bridge sections at admitted entrances; no route should require jumping the trough. Open portions are not walkable surfaces.
- Use static triangle collision for channel surfaces, never a combined convex hull that seals the void. The importer requests triangle collision for structural modules; street collision/navigation admission still needs actual inspection.
- Fit bends, junctions and outlets separately. This initial kit has straight modules, not mitered corners, outfalls, sloped large channels or a hydraulic simulation. Do not scatter disconnected modules as if they form functioning drainage.
- Tree variants need soil/root space and actual canopy-bound checks against walls, wires and roadway. Large shade crowns belong in sufficiently roomy setbacks or yards; no automatic dense curb planting is supplied.
- Preserve hand-edited native PCG points. Selecting six different meshes replaces scale-only variation, but the kit does not silently rewrite existing graphs or plants.

## Verification And Blocker

The original mesh generator completed, generated manifest counts/bounds were inspected, and Blender 5.2.2 created the editable source library. No render, bake, test suite, lint, typecheck, app build or packaging ran. Visual shape, smooth shading, foliage density, native import and gameplay collision remain unverified.

Read-only inspection found Unreal 5.8.3 PID 13584 on the saved Library Lane parent, no PIE and no dirty maps, but seven unrelated dirty material packages in Everyday, Fixtures, WorldKit and Foliage2. We did not save or mutate them, import new assets or modify the street. The importer rejects dirty packages and existing immutable asset paths; native import and visible street placement remain incomplete.
