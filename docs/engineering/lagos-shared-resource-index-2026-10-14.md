# Shared Lagos resource index

October 14, 2026. Handoff for concurrent chats in `C:/dev/cribtest-game`. Read existing receipts before generating duplicates. Paths below are available workspace resources, not an assertion that every asset is imported, accepted or shipped.

## New Tree And Drain Assets

- `output/unreal-street-detail-20261014/tree-drain-kit/6376944f38f4dbca`: six genuinely different tree meshes with three LODs each; six open-drain/pavement modules; 24 OBJ files, MTL, hash/bounds/triangle manifest, full geometry JSON and editable `TreeDrainKit_Source.blend`.
- `apps/companion-unreal/Scripts/StreetFinish/tree_drain_geometry.py`, `author_tree_drain_kit.py`, `tree_drain_blender_source.py`, `import_tree_drain_kit.py`: original procedural source, immutable authoring and guarded explicit native import. Not yet imported or placed. Geometry digest `6376944f38f4dbca47ee95f9fba13d167c49a1657c5b3d55c691154490dc249c`.
- `docs/engineering/unreal-tree-drain-kit-2026-10-14.md`: dimensions, LOD counts and integration contract. Open channels require an actual cutout/replacement of the underlying support slab; otherwise the surface fills the void. Do not use convex collision that seals the trough.
- `docs/world-building/lagos-mosaic-2026-10-11/tree-drain-reference-2026-10-14.md`: University of Lagos/GBIF checklist, inspected public photographs, source rights and evidence gaps. Morphotypes are not verified species at an address.

## Existing Street And Detail Libraries

- `output/unreal-street-detail-20261014/worldkit`: retained recipes, six-asset OBJ/FBX conversion context, native-import receipts and rejected canopy evidence. Inspect manifests; do not reimport rejected assets as accepted.
- `output/unreal-street-detail-20261014/foliage2`: fuller broadleaf/shrub variants and native-import receipts. Fuller foliage is not a substitute for different tree forms.
- `output/unreal-street-detail-20261014/evidence`, `final-state.json`, `planting-receipt.json`, `street-edges-receipt.json`: existing captures and placement/readback records. Historical acceptance has a bounded camera/scope, not citywide or gameplay acceptance.
- `apps/companion-unreal/Scripts/StreetFinish/README.md`: saved Library Lane parent/layers, PCG tools and explicit authoring/import scripts. Preserve accepted point edits rather than replaying initial-authoring scripts.
- `apps/companion-unreal/Content/Python/baddie_street_tools.py`: existing native Tools menu. Startup registers controls; it must not automatically load a map or regenerate content.
- `output/cribtest11-visible-world-2026-10-11/secondary-detailing-v1` through `secondary-detailing-v5`: staged detail kits with manifests, LODs and import/assembly guidance. Read each README/HANDOFF. V5 supplies 20 connection/repair accessories and 60 FBXs; its wrapper depends on v1 importer and v2 textures/manifest. Copying a later folder alone is not a complete bundle.
- `output/cribtest11-visible-world-2026-10-11/realism`: pinned frontage/base receipts, mesh imports, historical captures, defect and crash handoffs. `apps/companion-unreal/Scripts/WorldGeneration/Realism` contains the associated isolated authoring helpers.
- Other visible-world folders include street props, verified material inputs, roofs, crossings, apron, ground/support, residential/compound, oldtown, Library Lane and road-join import sources. Use their matching receipt/manifest rather than treating source OBJ existence as native acceptance.

## Research And World Authoring

- `docs/world-building/lagos-mosaic-2026-10-11`: mainland, tech/mixed centres, island/coast, lagoon, port/work streets, old Island, Agege, outer coast and Obalende/Ikoyi dossiers. `connector-evidence-ledger.md`, `imagery-evidence-review.md` and `assembly-plan-v1.md` distinguish mapped geography, actual inspected images and composite design.
- `ebute-metta-extension.md` and `ebute-metta-detail-v1.md`: five mapped streets, two candidate circuits and individual fictional premises. Names/connectivity are map-derived; tenants/interiors are design.
- `output/lagos-mosaic-authoring/mosaic-ebute-metta-98a5b42c88a75e047ebea7dfe66d889ea4401ed87defd5bea27c6e3b7bf9520b.json`: current detail packet, 18 premises, 54 room/door targets, 95 props and 18 audio-emitter specifications; runtime unregistered. Source is `mosaic_ebute_layout.py`, `mosaic_ebute_detail.py` and `mosaic_ebute_export.py`.
- Do not place the older Ebute packet ending `1dbf62be33b2f139806099900780d34e1546417e252f9292842e4ce543b08086`: it has the wrong ground/vertical transform. Correct native geography is X south, Y east, Z up in centimetres. Source metre-space uses x east, y up, z south; conversion is applied once.
- `apps/companion-unreal/Scripts/WorldGeneration/Connected` and corresponding visible-world receipts contain existing connected-town source, terrain/support, crossings and street geometry. Preserve canonical IDs and original maps.

## Tools And Coordination

Read-only inspection this turn found UE 5.8.3 editor PID 13584 on `/Game/BaddieGeneratedWorld/StreetFinish_20261014/Maps/L_TownCentre_LibraryLane`, no PIE or dirty maps, but seven dirty material packages in Everyday, Fixtures, WorldKit and Foliage2. This is a snapshot, not a durable lock. Do not save/discard another chat's material edits. Reinspect current state and coordinate one writer before any mutation. This newer StreetFinish session is distinct from the historical Realism crash/recovery hold.

Existing PID-bound transport: `apps/companion-unreal/Scripts/python_bridge.py`. Unreal embedded Python: `C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/ThirdParty/Python3/Win64/python.exe`. Installed Blender: `C:/dev/tools/baddie-blender/blender-5.2.2-windows-x64/blender.exe`; an existing interactive Blender was PID 3224. Asset-library authoring used an isolated background worker, not that live session.

Use the T3 collaborative preview tools for browser inspection. Blender production guidance is at `C:/Users/cribadmin/.codex/skills/blender-production/SKILL.md`. Fresh research may use read-only OSM/GBIF/Commons endpoints; web search returned blank in this session. Dates, metadata and inspected pixels must remain distinct.

No tests, lint, typechecks, app builds/exports or releases without explicit authorization. Required production companion packaging is authorized only when production renderer/shared consumed data changes. This native-only source kit did not trigger packaging or mobile What's New. Respect dirty user files, preserve immutable outputs and separate source generation, import, placement, rendered review, gameplay admission and release status.
