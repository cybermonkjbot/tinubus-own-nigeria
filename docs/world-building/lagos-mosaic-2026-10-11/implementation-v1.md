# First mosaic implementation

October 11, 2026. Source implemented; native execution deferred because the existing world generator owns the active editor.

## Implemented source

- `apps/companion-unreal/Scripts/WorldGeneration/Realism/mosaic_layout.py`: pure, deterministic source-bound placement planning with unique IDs, finite coordinates, unscaled complete imported building assemblies and separate native placement identities.
- `apps/companion-unreal/Scripts/WorldGeneration/Realism/mosaic-yaba-v1.json`: pinned existing asset/placement receipts and Yaba research document; 29 frontage/courtyard building assignments, 120 m main street, 50 m side-road surface, 12 by 12 m three-sided yard and four walking-height camera configurations. All dimensions are authored values, not surveyed measurements.
- `apps/companion-unreal/Scripts/WorldGeneration/Realism/mosaic_native.py`: guarded native author/save/readback and camera preparation through the existing PID-bound Python bridge. It reuses the existing bounded ISM placement pipeline and material contract; it does not change source meshes or materials.

The intended new map is `/Game/BaddieGeneratedWorld/Realism/YabaSlice_20261011/Mosaic_v1/Maps/L_Yaba_MosaicStreet_v1`. This is a target path, not a saved or verified map.

The first implementation uses the earlier imported Yaba frontage as an asset library, not as a claim to reconstruct University/Moore Road. Existing imported coordinates are already native centimetres. Geographic source coordinates remain separate and are not converted again or assigned to this local art layout.

Commercial bays have different proposed uses and stock filtering; residential groups close inherited commercial openings with whole existing shutters. Purpose-specific shop fitout, usable household doors/interiors, courtyard furniture, vehicle ramp transitions, NPCs, traffic, audio and events remain pending. There is no new runtime admission or dynamic simulation implementation in this patch.

## Active editor ownership

The existing Python bridge identified Unreal 5.8.3, project `C:/dev/cribtest-game/apps/companion-unreal`, PID 1784. Read-only MCP calls returned different civic-generation maps on successive inspections. The existing `output/cribtest11-world-generation/execution/worker.lock` records PID 1784 and owner `cribtest11-world-generation-20261010`. Native Python discovery also intermittently returned no editor; a successful identity response does not establish exclusive ownership.

No map-load, level creation, actor/material mutation or editor restart was performed. The new authoring entrypoint rejects the existing generator lease, changed PID/project/world, active PIE and dirty packages. It also uses its own exclusive lease, retains that lease on failure and refuses replay when an immutable receipt exists. Do not delete either lease to force execution. A future native pass requires the existing writer to relinquish ownership and a fresh identity/current-world inspection.

## Inspection and next native operation

Source review inspected inherited placement transforms, module bounds, material contracts, source receipt hashes, courtyard geometry relationships and namespace ownership. No test, typecheck, lint, compilation, build, script execution or native rendered inspection was performed for the new implementation.

After a legitimate editor handoff, invoke `mosaic_native.author(inspected_pid, inspected_world_package)` through the existing authenticated Python bridge. It creates only the fresh target map, saves it and records `output/cribtest11-visible-world-2026-10-11/realism/mosaic-yaba-native-v1.json` with source-to-component mappings and file hashes. A partial failure requires explicit owner review; do not replay over its saved output.

Then inspect the saved native map with `mosaic_native.readback`, prepare the four configured views and capture the already-running native viewport using the existing capture pipeline. Review main-forward, main-reverse, side-depth and yard-entry, including ground contact, source seams, yard opening, residential frontage and road/drain interfaces. Component inspection does not establish visual quality, traversability or performance.

This patch is isolated additive Unreal authoring source, not production companion renderer code, consumed shared scene/map data or a mobile change. Renderer packaging and What's New updates are not triggered. No game preview, native map use, production release or delivery to a remote renderer is claimed.
