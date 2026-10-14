# V5 environment integration

October 14, 2026. User authorized integration into the existing environments.

Unreal 5.8.3 editor PID 13584 was inspected on
`/Game/BaddieGeneratedWorld/StreetFinish_20261014/Maps/L_TownCentre_LibraryLane`.
The parent already references Library Lane dressing, street edges and the
town-centre EyeLevel layer. GameDefaultMap remains `/Engine/Maps/Entry`;
changing a default map alone would not establish gameplay admission or a release.

## Completed

Imported `SM_WallTap_A` and `SM_HoseCoil_A` into
`/Game/BaddieGeneratedWorld/StreetFinish_20261014/ConnectionsV5/Meshes`.
The staged importer accepted all three LOD triangle counts and saved only the
new meshes and material. It reused the two hash-verified Everyday atlas textures.
No existing material was saved or overwritten.

Receipt:
`output/cribtest11-visible-world-2026-10-11/secondary-detailing-v5/receipts/water-connections-412695e4eec74913ad55a4534268f477.json`.

Added an explicit PID-bound placement script:
`apps/companion-unreal/Scripts/StreetFinish/integrate_connections_v5.py`.
Its intended pass is four wall taps on existing Library Lane side jambs and a
ground-supported hose beside the existing bucket. It uses native PCG/HISM,
short cull distances, authored LODs, no collision/navigation, and no utility
power or interaction authority. Native reflected-Y bounds were inspected to
calculate the tap's wall-contact transform. No new world instance was created.

## Concurrent-writer blocker

Initial inspection found no dirty maps and 16 unrelated dirty materials. Later,
before placement, the shared parent and `L_TownCentre_Biomes` became dirty.
The script rejected installation before changing the world. Those changes
belong to another writer; they were not saved, discarded or reloaded.

At that later read, the loaded world also contained 18 EyeLevel generators with
2,090 instances, in addition to the earlier four generators with 189 instances.
That live count is not proof of saved Biomes work or game/release use.

## Resume

Coordinate exclusive native-world writing and let the current owner resolve
the dirty maps first. Reinspect PID, project, map, entrance clearances, existing
instances and current source before running `install(expected_pid)`. If native
generation completes, run `finish(expected_pid)` to save only the owned layer
and its parent reference. Do not rerun install against an existing layer.
Review actual wall contact and hose support from close and reverse views before
acceptance. Then connect the accepted saved parent to the intended game startup
path without changing the existing game mode or account/movement authority.

V5 placement, startup integration, saved-world reinspection and visual acceptance
remain incomplete. No tests, builds, PIE, renderer packaging or release ran.
