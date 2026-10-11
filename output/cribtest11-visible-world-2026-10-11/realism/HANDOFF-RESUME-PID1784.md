# Saved Nigerian street repair

October 11, 2026. Joshua's direct yes released the recovery hold. One existing desktop-task launch succeeded: PID1784, correct BaddieCompanion project, Unreal5.8.3, D3D11 Microsoft Basic Render Driver softwareSM5. Editor remains running on the saved repaired street, junction camera, no dirty packages. Earlier stop/crash records remain historical, unchanged.

## Actual output

New native output: **47 packages:3maps/44assets**,3,718,357bytes. This includes32v3meshes,11v3material derivatives,1dry-road material,2street maps and1streaming parent. Full realism inventory now136packages/19maps/117assets, including historical setup/rejected attempts and11empty district children; not whole-city coverage. All89historical native files remain byte-identical. Exact paths/hashes: `resume-output-inventory-pid1784.json` SHA256 `3fdaee9a1b28943f282d386accf07715331e74640a528f23faaf8998f130b4d7`.

Saved paths relative to `C:/dev/cribtest-game/apps/companion-unreal/Content/BaddieGeneratedWorld/Realism/YabaSlice_20261011/`:

- `DenseStreet_v2/Maps/L_Yaba_DenseStreet_v2.umap`: prepared bounded repair, unsupported doubled uppers removed, dry derivative asphalt, pedestrian/drain span restored. Immutable receipt `yaba-dense-street-v2.json`, SHA256 `dca21fd04e339d31f29a1aa2f89d4ce69b99ac9cceee7344751698efedb208c5`.
- `DenseStreet_v2Reviewed/Maps/L_Yaba_DenseStreet_v2Reviewed.umap`: same120m local street/20m side branch/21runs using reviewed v3 inset masonry, unchanged openings, variant windows/AC and shop stock.3651source placements:208boundedISMcomponents/3545instances plus106separate mesh actors;2earlier static props remain blockout dressing.1710stock placements. Complete source-ID/component/index/transform/material journal in `yaba-dense-street-v2-reviewed.json`, SHA256 `bd981004b5179ed75edbba9e2c46cb5e6d2d0bf5f13f1d068682e40e57117667`. Independent save/reload readback `yaba-dense-v2-reviewed-post-reload.json` confirms zero transform/material mismatches. Counts are not measured performance.
- `Frontage_v3/Meshes/SM_*.uasset`:32actualFBXimports; `Frontage_v3/Materials/M_*.uasset`:11new materials/usage derivatives. Native asset receipt SHA256 `b12b6d8f1dea57912217cb5fae0ca69988753bfced3e8115b0f6afb54feda5e8`. Per-module bounds/UV/slot/hash records retained.
- `StreamedAuthoring_v2/Maps/L_Lagos_AuthoringPersistent_v2.umap`: explicit native streamed-level alternative, separately saved and reloaded.11relationships verified. Receipt SHA256 `a06f7fc1478199434983cbf3c9833e4dd87b58bc817ca3762da80435112d2d74`. Children remain empty; no WorldPartition, HLODproxies, player streaming or destination admission. `disable_distance_streaming` was true in authoring but false after reload: installed LevelStreaming.h:316 declares it transient. Do not treat that flag as saved runtime policy.

## Actual image inspection

Eight native705x486PNGframes, real CaptureViewport pixels after advancing native frames. Baseline v2 and v3-equipped v2 use exact prior walking-height cameras/FOV. All eight opened locally. Exact hashes/camera readback in matching `.capture.json` files and inventory.

- `yaba-dense-v2-reviewed-forward.png`, SHA `10c29c2e4e0705eabd7a96479beeefb9b3c6019931d4f4816ca6fc4c3ac3b0ef`.
- `yaba-dense-v2-reviewed-reverse.png`, SHA `b3c72e440b913823b1b22238e02744c28085e096e4e79a2b84bdc9856973283e`.
- `yaba-dense-v2-reviewed-junction.png`, SHA `e452b652cde5b2a281b067dca8765756197d73c6432083f18d5e95c2582a3b6e`.
- `yaba-dense-v2-reviewed-stock-threshold.png`: supported shelf/counter stock visible; generic unlabelled cans/packages and dark interior remain.
- `yaba-dense-v2-reviewed-side-street.png`: supported20mroad/footpaths and adjoining buildings visible, but open black boundary exposes unfinished extent. **Not a finished connected street.**

Repairs visibly improve floating-storey attachment, dry road response, stock and panel contacts. Uniform pavement, repeating structural shell, blank shop signs, thin rail/shutter/glass sampling and unfinished branch termination remain. `resume-defects-pid1784.json` links observations to images. No final realism or coordinator visual acceptance asserted; no larger district expansion performed.

## Source/material contracts

Rechecked250pinnedread-only sources, zero mismatches; pin `e978763b74acfb0369b65b879af6a3a8c90b110b`. Accepted canonical generator identity unchanged: `44e0f0bde9ff18ef6cb72a09bc2e219469944a7a8ba120e30782ee7cbc2f4797`. Canonical theatre, travel, account placements/gates unchanged. Current human authoring authorization plus coordinator serialization review is recorded truthfully; no fabricated separate digest-review quote.

V3bindings `6bca8305d23b8641a4b1b9d379ca7162d78f36f6f6ed76809b73d1fbde2c6244`; placement `49e399471e8d349d1fe747db80e45002fd6d116c1da54bfd1800b50da8f287e5`;32FBXs match workerreview. Actual asymmetric panel bounds verify metre(x,y,z) ->UEcm(-100y,-100x,100z), yaw negated.25cmthickness/3.5cmsetback retained. SurfaceUVpredivided, native1x. Existing scanned surfaces reused, sRGBcolour/data roughness and normals; timber same-sourceDXnormal unchanged, no doublegreenflip. Roof scan normal intentionally omitted over modeled corrugations. V3originals/Blenderworkbench untouched.

Completed vehicle/earth-road exports and actual Blender views inspected read-only. Repaired danfo/sedan remain simplified geometry/material art, not imported here. Earth kit lacks licensedsoil/sand/gravel scans and still has weak joins; no relabelled asphalt/concrete. Current road handoff preserves fiveweights in twoLINEARFBXcolourlayers plus topologyJSON, not the earlier source-only UV1-3contract. Adapt that contract explicitly before future native import. Rejected trees remain unplaced.

## Recovery and limitations

Safe helper released World/Level/Actor/Package wrappers before each switch, strings-only dirty inspection and exception cleanup; no World duplication. OldPID5916crash log/context preserved. V3readback accessor mismatch was repaired without scene replay; `executed-source/frontage_v3_native-pid1784-authoring.py` reproduces the immutable executed source SHA `4130fea1f6e7c71606b8bd43fe82d9828f8add051e811a577ba9bb5d152f6d7e`.

Prepared streaming code's Python `make_level_current(Level)` failed because installed callable acceptsLevelStreaming. Finished that inspected partial via explicit `save_map(World,parent)` in `finish_streamed_parent.py`; no reattach/replay. Separate load/readback succeeded; previous exception wrappers cleared before switching back to street.

`renderer-readback-pid1784.json` records actual preview values: GI0/reflections2/VSM0/AA4. Nanite/Lumen flags do not demonstrate active hardware features. Existing5Nanitederivatives and owned targetprofile remain inactive; v3fallbacks preserved. GPU timings/memory/traversal and playable acceptance pending.

Existing editor automatically ran its save-time asset validators and load-time MAPCHECKDEP. No independent tests/lint/typechecks/compilation/builds/commandlets, installs/downloads, config edits, gameplay activation or pushes invoked. `.uproject` and both Config hashes match pre-resume dirty state.

Checkpoint only generated-world `Realism/`, generation-script `Realism/`, visible-output `realism/`, with inspected/disabled hooks. Commit recorded separately in `.git/operations/world-generation-20261010/worker/realism-resume-checkpoint-pid1784.json` and chat. Next: coordinator inspect supplied frames and select bounded contact/context/material refinement before expansion. Licensed soil and reviewed detailed foliage remain missing inputs; this is saved local art, not full Lagos or a GTA-like playable game yet.
