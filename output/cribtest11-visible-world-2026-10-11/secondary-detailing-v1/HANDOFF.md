# Secondary detailing import preparation

October 11, 2026. Owned directory: `secondary-detailing-v1` only.

## Prepared

17 assets, 51 FBXs, an editable Blender library, shared 2048px color/ORM
atlases and a 1024px packed weather mask. Three batches: litter (5 assets),
infrastructure (6) and notices/signs (6). Triangle counts are in the manifest;
the tire is the largest LOD0 at 1,152 triangles, reducing to 384 and 144.
Simple boards retain their already small mesh counts across LODs.

The staged importer prepares opaque/masked material graphs, optional close
plastic and four weather-decal graphs. It records exact source hashes, imported
LOD counts and material slots, saves only its created assets and writes a
receipt. It is not executed here and may require installed-engine API adjustment;
unsupported required properties stop the batch instead of silently degrading it.

## Reviewed

Original typography atlas inspected directly: fitted church lettering, inset
torn masks and ink-only warnings. Individual algae mask inspected directly;
multiscale noise replaced the initial periodic pattern. Saved Blender library
read back: one owned scene, 51 objects, two materials and three packed textures.
Export hashes match the manifest. Existing workbench file and scene remain intact.
FBX context inspection caught the workbench selection in the initial exports.
All 51 files were re-exported with explicit native window scene binding and
their intended object IDs inspected in the actual binary payloads.

## Not accepted or integrated

No Unreal import, rendered mesh inspection, in-game placement, native collision
review, LOD transition review or frame-time measurement. This is an initial
authored kit, not verified photoreal art. Niagara systems, full shared-wall
vertex-paint integration and actual endpoint-fitted wire splines remain owner
integration work. Wrapper art is original sausage-roll packaging, not approved
Gala branding. Terrain, layout, buildings, account state and world authority are
unchanged. No production renderer, mobile, release or external upload work.

Use `README.md` for separate batch calls. Do not import every LOD FBX as a
separate content asset: the importer attaches LOD1/2 to its LOD0 mesh. No automatic
world spawn or new editor/server process is included.
