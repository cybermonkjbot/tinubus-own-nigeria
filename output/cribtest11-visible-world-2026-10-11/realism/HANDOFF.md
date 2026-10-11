# Windows native art checkpoint

October 11, 2026. Editor **stopped** after PID5916 crashed at10:25:20UTC. No restart or engine mutation occurred after the stop instruction. The checkpoint contains prior saved native output plus unexecuted repair source, not a finished Lagos world or playable acceptance.

## Saved and inspected

Under `C:/dev/cribtest-game/apps/companion-unreal/Content/BaddieGeneratedWorld/Realism/`: **89 native packages:16maps/73assets**,202,890,999bytes. This includes setup, preserved rejected material attempts,11empty district child levels and an uncertified streamed parent. `saved-output-inventory.json` lists exact paths and file hashes. The earlier40packages/3maps remain unchanged.

- `YabaSlice_20261011/Frontage_v2/Maps/L_Frontage_ImportCheck_v1.umap`:18detailed imported meshes,83placements,11.7m assembly. Receipt `frontage-v2-assembly-v1.json`, SHA256 `9e91de818fcb843fdfeb4cebf3922268dbf7e0f1870150cb8179a9dcce291940`.
- `YabaSlice_20261011/Frontage_v2/Maps/L_Frontage_ISM_v1.umap`:12ISMcomponents/75instances plus8separate mesh actors;83-source-ID mapping/readback. Receipt SHA256 `3b15c88e9e3e44ffc268180a18f1b7f285fa1674bd840d75709b7e93e48f324b`. Coordinator confirmed zero location/scale/yaw differences and no apparent batching loss at705x486. No frame-time result.
- `YabaSlice_20261011/DenseStreet_v1/Maps/L_Yaba_DenseStreet_v1.umap`:120m local art slice,20m side street;2033module placements,118ISMcomponents/1950instances,83separate modules and2earlier static props. Receipt SHA256 `0f6e153f51c28e86ab6da6b989e91408d970538bc186e2b4f704125f3818ae6b`. Readback and actual forward/reverse/junction images exist. **Visual review rejected** floating upper duplicates, repetition, empty shops, glossy road and dark unsupported junction span. `defects.json` retains matching images and evidence.

Seven actual native PNGs are retained in this evidence directory. Key views: `frontage-v2-native-walk-wide.png`, `frontage-v2-ism-walk-wide.png`, `yaba-dense-v1-forward.png`, `yaba-dense-v1-reverse.png`, `yaba-dense-v1-junction.png`. PNG/camera/receipt hashes are in the inventory. Local forward inspection confirms the visible defects; no new frame was rendered after the crash.

Five separate opaque Nanite derivatives have actual enabled-flag readback, with full software fallbacks retained. They are not active in the D3D11 Microsoft Basic Render Driver preview. `render-target-profile.json` is an inactive owned target artifact, not shipping renderer settings. GPU frame times, memory/traversal profiling and final Lumen/Nanite output remain unmeasured.

## Crash and save defect

Failure: `LevelEditorSubsystem.load_level(DenseStreet_v1)` while the Python bridge's local `dirty` list still held the old streamed-parent **UPackage** wrapper. Log GC chain: `GCObjectReferencer -> FPyReferenceCollector::AddReferencedObjects -> UPackage`. Installed Python plugin source visits every live wrapper; releasing only World references is insufficient.

Log: `C:/dev/cribtest-game/apps/companion-unreal/Saved/Bridge/editor-20261011-024041.log`, SHA256 `6ce60705008c8cafe333ef5241425c1b77fc61e6d1a697bd415b545f48a38e27`. Fatal: `EditorServer.cpp:1951 World Memory Leaks:1objects/packages`.

Crash context: `apps/companion-unreal/Saved/Crashes/UECC-Windows-B9305A994205C2CB81A5F9978B4E13E9_0000/CrashContext.runtime-xml`, SHA256 `065eec3b3cd23684b1dc822c45f092e0b0a96b1035f414355eaefa37f2a3e115`. `crash-5916-investigation.json` and log excerpt retain exact evidence.

`CreateNewStreamingLevel` makes the child current; `SaveCurrentLevel` saves that child, not the persistent parent. Thus11childmaps exist, but saved parent relationships are **not certified**. Later attempts resaved OptionalDressing, changing its file hash from `3edb41f8...` to `9bba2e57...`; the original receipt stays immutable and the actual final bytes are bound in the inventory. Parent hash remained `ba54613d...`. These empty native levels do not establish district geometry, World Partition or playable streaming.

## Prepared, not executed

Owned source: `apps/companion-unreal/Scripts/WorldGeneration/Realism/`.

- `repair_dense_v2.py`:separate bounded map; remove all unsupported upper duplicates, preserve original two-storey construction/openings/roof without scaling; dry-road derivative `max(scannedR,.88)`,specular.18,attenuated normalized scan normal; restore actual imported sidewalk/channel/cover modules across the missing span. Same cameras, lights and80m-start restrained atmosphere asv1. Detailed vehicle ramps/culvert continuation still need contact inspection; panel/pier overlap, stock and facade variation await reviewed source revisions. Correct roughness map/noncolourR linkage was read back; root cause of the water-like preview is not proven.
- `safe_level_change.py`:strings-only dirty inspection, standard last-exception cleanup, scoped switch. The bridge caller must retain **no** Package/World/Level/Actor wrappers or active exception frames containing them. Merely calling garbage collection cannot free live references. This correction has not been exercised in an editor.
- `streamed_authoring.py`:new `StreamedAuthoring_v2` parent can attach the saved children, select the exact persistent-level identity and explicitly save/reload/read back. It preservesv1maps/receipts and binds the historical OptionalDressing resave. No corrected parent has been created.
- `unpaved_lane.py` and `unpaved-road-contract.json`:hash-bound import/material/placement preparation for frozen road recipe `063f9f7f9a7eb1b55cc6a0fd8c285ba8481a16600b16a4a51305cf874205f197`; handoff `2dd655b9edb829782adbd55d70ea4914c417b66fd0556f96ffd92801540ec641`. No recipe execution or native road assets. Recipe has real crown/ruts/shoulders and a closed underside, not asphalt recolour. Explicit five-weight export request:UV1earth/sand,UV2damp/gravel,UV3asphalt/candidateWet; UV0metres tiles once per verified scan coverage. Frontage's pre-scaled SurfaceUV remains1x. Requires completed reviewed road FBXs/full attribute manifests and licensed earth/sand/gravel textures. Asphalt/concrete cannot be relabelled soil. New bounded side-lane placements require supported joins, drain/threshold review and already-converted UE transforms. No water geometry, weather or traction implementation.

**DenseStreet_v2, corrected streaming parent and unpaved lane:zero native assets/maps/captures.** Rejected tree remains unplaced; incomplete vehicles/frontage-v3 are not imported.

## Source authority and coverage

Pinned canonical source `e978763b74acfb0369b65b879af6a3a8c90b110b`:all250references rehashed without mismatch; accepted generator identity remains `44e0f0bde9ff18ef6cb72a09bc2e219469944a7a8ba120e30782ee7cbc2f4797`.18frontage FBXs still match their snapshot. The current human authoring request plus coordinator serialization reconciliation is recorded truthfully; no separate Joshua manual digest quote is invented. Historical preparation/hold records remain unchanged, not presented as current.

Canonical output remains limited to `city/national-theatre` and `city/theatre-grounds`. The135-job inventory is preserved; city entirety, places/interiors,49reusable Home recipes, waterfront and rural expedition remain incomplete. The local Yaba art slice is not a canonical district or geographic remap. Factual geographic topology and approved fictional separate-island composition stay separate; no saved coordinates/travel endpoints move. Account access, movement admission, mission readiness and utility authority remain closed.

## Next direct authorization

Authorize **one** existing inspected desktop-task recovery (`Baddie-Unreal-Editor-desktop` / existing `Start-EditorBridge.ps1`), followed by fresh project/PID/RHI inspection through the authenticated Python/MCP bridge. Then authorize the bounded new DenseStreet_v2 author/save/readback and identical forward/reverse/junction captures, plus new streamed-parent explicit save/reload/readback. Old PID-bound receipts cannot authorize the fresh PID. Unpaved execution separately waits for complete reviewed road exports/soil materials and its exact new import receipt.

No compilation, tests, lint/typechecks, HLOD builds, full builds, deployment, downloads/installs, config/runtime activation or pushes were performed. Owned checkpoint paths only: generated-world `Realism/`, generation-script `Realism/`, visible-output `realism/`. Hooks are inspected and disabled command-locally. Commit identity is recorded separately under `.git/operations/world-generation-20261010/worker/realism-checkpoint-20261011.json` and in the chat handoff.
