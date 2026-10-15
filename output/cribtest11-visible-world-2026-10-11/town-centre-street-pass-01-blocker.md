October 11, 2026

# Town Centre street pass blocker

- Selected district remains `town-centre`.
- Intended target map was `/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Maps/L_town_centre_neighbourhood_20261011_streetpass01`.
- Source parent stayed `/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Maps/L_town_centre_neighbourhood_20261011`.

## What happened

- The live Unreal editor session was confirmed as PID `1784`, correct project, correct loaded map, no dirty packages.
- The first execution attempt failed before mutation because `author_slice.py`'s historical accepted generator hash no longer matched the current worktree hash from `native_generator.py`.
- After removing that unrelated guard, the second execution attempt crashed the editor during the map-switch stage.

## Crash evidence

- PID `1784` exited.
- MCP listener on `127.0.0.1:8000` disappeared.
- No new `streetpass01` map file or receipt was saved.
- Crash log:
  - `apps/companion-unreal/Saved/Crashes/UECC-Windows-A57636FB4E77BD65580D918497C617AA_0001/editor-20261011-081405.log`
- Minidump:
  - `apps/companion-unreal/Saved/Crashes/UECC-Windows-A57636FB4E77BD65580D918497C617AA_0001/UEMinidump.dmp`

## Root cause isolated from the log

- The crash is the same Unreal GC leak class already documented elsewhere in this repo for duplicate-map workflows.
- Fatal lines from the crash log:
  - `Old world ... L_town_centre_neighbourhood_20261011_streetpass01 not cleaned up by garbage collection while loading new map`
  - `World Memory Leaks: 2 leaks objects and packages`
- The prior implementation used `duplicate_asset(...)` followed by `load_level(...)` for a `UWorld` map asset.

## Source changes prepared after the crash

- `apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_street_pass.py` no longer uses the unsafe duplicate-map route.
- The script now:
  - snapshots the connected parent environment into plain data,
  - creates a fresh persistent level with `new_level(...)`,
  - reattaches the saved child levels from `town-centre-connected-neighbourhood-v1.json`,
  - preserves the additive street-dressing pass for Market Road, Library Lane, Civic Street and Garden Avenue,
  - keeps deterministic frontage variation and drain-cover placement.

## Resume requirement

- Do not auto-restart loops.
- Resume only once a fresh authenticated Unreal editor session is intentionally available again.
- First action on resume:
  - verify new PID, project, loaded map and clean packages,
  - then run the rebuilt `town_centre_street_pass.py` against that fresh editor session.

## Later recovery evidence, October 11, 2026

- Fresh editor PID `3940` reopened correctly and loaded `/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Maps/L_town_centre_neighbourhood_20261011_streetpass01`.
- Inspection showed the earlier rebuilt pass had only created the new parent and reattached the 11 connected child levels. The saved `streetpass01` map file stayed tiny at `8816` bytes and no receipt existed.
- The next resumed pass exposed two real script defects:
  - current canonical parcels use `street`, not `streetId`;
  - `road_rows()` emits both raw road rows and frontage-style transform rows, but `spawn_road_rows()` only handled the raw-road shape.
- After patching those defects, a live retry spawned roughly 1008 frontage actors and 4 road actors into the wrong loaded child level (`places/pool-courtyard`) because the parent persistent level cannot be made current through the inspected Python binding and the last attached child remained current for spawning.
- A direct actor-move recovery attempt then created `/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Maps/L_town_centre_neighbourhood_20261011_streetpass01_detail01`, but the editor became non-responsive before parent save, package cleanup or receipt write. The detail map on disk is only `6844` bytes and must be treated as incomplete.
- `town_centre_street_pass.py` is now further patched to create its own dedicated additive streaming detail child from the start (`...streetpass01_detail01`) so future spawns do not land in connected destination child levels.

## Safe next step after this note

- Terminate the hung PID `3940` once this evidence is preserved.
- Delete the incomplete local files:
  - `apps/companion-unreal/Content/BaddieGeneratedWorld/Connected/TownCentre_20261011/Maps/L_town_centre_neighbourhood_20261011_streetpass01.umap`
  - `apps/companion-unreal/Content/BaddieGeneratedWorld/Connected/TownCentre_20261011/Maps/L_town_centre_neighbourhood_20261011_streetpass01_detail01.umap`
- Relaunch the editor once through `Start-EditorBridge.ps1`, verify fresh PID/project/source parent/clean packages, then rerun the patched street pass cleanly from `/Game/.../L_town_centre_neighbourhood_20261011`.

## Resume command shape

- Bridge interpreter:
  - `C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\ThirdParty\Python3\Win64\python.exe`
- Bridge script:
  - `apps/companion-unreal/Scripts/python_bridge.py`
- Rebuilt authoring script:
  - `apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_street_pass.py`
- Required preconditions:
  - current loaded map must again be `/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Maps/L_town_centre_neighbourhood_20261011`
  - no dirty packages
  - no PIE/game world
