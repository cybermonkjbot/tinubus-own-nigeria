# Native street detail and park biomes

October 14, 2026. Initially authored in Windows editor PID 13584, UE 5.8.3-58210709. The project is now reopened in PID 10904 with the saved layers retained; see the recovery and forecourt continuation below. Native-only work; no shared mobile scene or production renderer changes.

## Delivered scope

Parent: `/Game/BaddieGeneratedWorld/StreetFinish_20261014/Maps/L_TownCentre_LibraryLane`.

The parent now references `L_TownCentre_EyeLevel` and `L_TownCentre_Biomes`, alongside the accepted earlier Library Lane layers. The original connected-town-centre maps and canonical road, plot and entrance transforms are retained.

Eye-level dressing contributes 1,867 instances through 18 separate CPU PCG graphs. Small foliage is placed beside existing frontage openings, in supported troughs/pots, at wall brackets and in earlier soil beds. Seven native-only plant meshes have three verified LODs. The isolated Blender production workflow produced and reopened the retained source file; it did not change the live Blender scene or shared renderer assets. The 20 street/bridge comparisons and seven closer frontage comparisons show useful near detail but also confirm that the town remains sparse. Counts are not visual acceptance of every street.

Park biomes use the installed experimental PCG Biome Core plugin, not a new third-party installation. Two native closed biome splines follow conservative polygons derived from the existing canonical park-loop support segments. Existing path, approach and plot-support boxes define native exclusions with additional clearance. Two definitions share a native plant palette and separate canopy, understory and groundcover generator data assets. The three cloned template graphs use seeded surface sampling, mesh variation, world projection and cull distances. They do not contain fixed authored point arrays.

| Area | Trees/palms | Shrubs | Grass clumps | Low plants | Total |
| --- | ---: | ---: | ---: | ---: | ---: |
| College Green | 19 | 106 | 355 | 169 | 649 |
| Neighbourhood Park 18 | 41 | 198 | 687 | 339 | 1,265 |

The saved parent was reopened successfully with 1,914 biome instances and 2,056 earlier/eye-level instances. Native receipts record source fingerprints, actual component materials, world transforms, generated state and collision/navigation flags. The biome audit found no roots on the protected support footprints, no missing/WorldGrid materials, no collision/navigation additions, and no changed protected actor transforms. Measured lowest-vertex ground error was below 0.001 cm on the existing flat support. This does not establish collision integration or target-device performance.

## Editor controls and implementation details

`Tools > Baddie World > Open detailed town centre` opens the saved parent. `Street-by-street detail` provides per-street views and graph access. `Park biomes` provides boundary selection, plant palette, density graphs, visibility and sequential local-data/core generation. The one-click College Green regeneration completed in the running editor before the final save. It does not implicitly save the map. The planting shortcut resolves the active graph asset rather than relying on the retained legacy actor label.

The installed templates required three corrections during authoring:

1. Biome Core's box component has a 512/512/128 scale, and exclusion actors default to scale 5. These are normalized before applying centimetre bounds; otherwise exclusions cover most of the park.
2. World-ray queries use object channels, not visibility-trace channels. The existing support is WorldStatic. Explicit vertical ray bounds avoid dependence on the shallow spline actor's physical bounds.
3. Root height filtering runs after mesh translation. The allowed range includes the measured groundcover stem-origin correction but still excludes paved support at Z=0. The local biome must generate before the parent core consumes its data.

Debug bypass and local-preview spawning are disabled. Diagnostic pins were confined to a separate `PCG_LocalPark` copy; neither live biome uses that graph. Its unreferenced diagnostic asset remains on the VM because the editor declined deletion; it is excluded from the delivered content checkpoint. Original plugin graphs were not modified. Owned scenery material usage changes were saved explicitly, without changing project/RHI settings.

Primary source files: `Scripts/StreetFinish/add_biomes.py`, `add_eye_level.py`, `export_eye_level_kit.py`, `build_street.py`, and `Content/Python/baddie_street_tools.py` under `apps/companion-unreal`.

Evidence: `output/unreal-biomes-20261014/reopened-state.json`, `output/unreal-eye-level-20261014/reopened-state.json`, and each folder's `evidence` directory. Initial installation receipts retain their historical pending state; reopened-state receipts establish persistence.

## Session status after persistence verification

The saved parent reopened at 10:34:39 on October 14, 2026, and readback at 10:35:35 confirmed all 3,970 scenery instances with no dirty maps. The editor connection subsequently disappeared; a process check found no running `UnrealEditor`, and Python discovery found no editor nodes. SSH remained available. No editor/VM restart or graphics-setting change was performed.

The retained engine log, `Saved/Bridge/editor-20261011-181531.log`, ends at the 10:36:12 `CaptureViewport` dispatch. This does not establish why the process stopped. The two available crash contexts for PID 13584 are `Ensure` reports, not a confirmed fatal termination report: `_0000` records a realtime-override warning on October 12; `_0001` records the failed unload of the unreferenced `PCG_LocalPark` diagnostic package at 10:34:21 on October 14. The editor continued through the successful reopen/readback after that ensure. The diagnostic package remains untouched and excluded from the native checkpoint. No root cause is claimed and no reproduction or repair was attempted.

Two final menu-source corrections were transferred after the editor became unavailable: the planting graph shortcut resolves by asset name, and biome hide/show uses temporary editor visibility. They were subsequently hot-reloaded and exercised after the authorized recovery below. Earlier menu registration and College Green's one-click sequential regeneration had been verified before the previous save.

## Authorized recovery and forecourt continuation

Joshua approved reopening and continuing building coverage and ground treatment. An initial noninteractive launch (PID 2168) failed with `DXGI_ERROR_NOT_CURRENTLY_AVAILABLE` during swap-chain creation; there was no signed-in Windows desktop. Connecting the existing Cribtest11 device in Windows App established the interactive desktop. The existing `Baddie-Unreal-Editor-desktop` scheduled task then reopened UE 5.8.3 in PID 10904. No VM restart, graphics/RHI change, installation or build was needed. This diagnosed startup failure is separate from the still-unconfirmed earlier PID 13584 termination.

The retained map had all 3,970 previous scenery instances and no dirty maps. College Green hide/show worked, and the planting shortcut opened `PCG_LibraryLane_PlantingAndShadeFull` as confirmed by the open-assets readback.

The ground survey revealed a persistence defect missed by the earlier instance-count audit: both editable biome splines had reverted to the template's four-point 50-metre square, although their generated plants and core bounds remained intact. Programmatically authored spline points lacked `spline_has_been_edited`. The repair first matched the recorded 12/14-point outlines against the current canonical park-loop supports and refused any possible hand-edited boundary. It restored the points and set the editor flag. Repeated saves/reopens now retain 12 College Green points and 14 Neighbourhood Park 18 points, with all 1,914 biome instances. No plugin template was changed. A saved instance count alone does not establish generator-input persistence.

The completed bounded continuation is recorded in `unreal-forecourts-ground-2026-10-14.md`: five additive compounds, fitted ground inside both park loops, new native editor controls, and matched viewport evidence. It does not add new buildings or finish the wider town.

## Remaining work and external tools

These additions do not resolve the whole-map complaint. Grass-toned surfaces now cover the two park interiors; outside them the broad ground remains tiled and flat. The five selected shells now have compounds and forecourts, but streets still lack continuous adjoining building fabric and terrain depth. Further structural work must use the accepted parcel/ground records and preserve source reconciliation guards, not cover missing building plots with vegetation or replace the city layout with an unrelated generated world.

No Calysto, WorldGen, Gaea, World Creator, Nwiro or VIO installation, purchase, account creation or project upload was performed. Existing research distinguishes terrain creation, asset generation, biome distribution and streaming; they are not interchangeable. Calysto's documented DX12 setup is separate from this editor's D3D11 software-rendering configuration. Runtime infinite terrain would be a substantial architecture change. The exact VIO 3D / V3 World product link is still needed to verify its claims.

No tests, lint, typechecks, C++ compilation, HLOD builds, PIE, mobile packaging, release or push were run. Native mesh import and editor generation were part of the requested authoring. Gameplay, target-device rendering/performance and full-town visual acceptance remain incomplete.
