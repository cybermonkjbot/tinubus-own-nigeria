# Cribtest11 native apartment integration

October 10, 2026. Source review, guarded delivery and native integration record.

## Reviewed source

We reviewed the connected-apartment change at `a377f2ecbcd548a4a0b9cf2e9e755309368378f2` against the prior Home checkpoint `67cf061108606cfba25a235d83561f4ff3c5a01d`. The specification review found no concrete defects in supplied-room identity, canonical `home-living` reuse, saved contents, extension-before-rotation order or exact-input caching. The existing one-room grant and response guard remain unchanged.

The standards review identified duplicated support-item eligibility in the bedroom and other layout mappings. We moved the unchanged item membership and ordering into the existing internal `SupportKinds` helper. The follow-up source review found no remaining issues in that four-file correction. Optional consolidation of room-limit constants and layout dispatch remains deferred.

Reviewed source checkpoint: `13edd22b375db1372f111bbd43cb4c9d0e5e9e4b`. The parent architecture/tracker checkpoint `dfe3a72b963583c0c3127acf54127479c2e0cbaf` is included in its ancestry. We did not edit the parent-owned MovementPresentation, MovementProtocol or WorldClock pairs.

## Windows delivery

The bridge confirmed the existing Windows chat `01a1251b-28dd-7bb2-b185-c78e4d385455` was not loaded, its previous Home turn `01a12545-2303-7cf3-b4bc-2e17fadd7d4f` was interrupted, and no chats were loaded before delivery. Fresh destination checks matched the prior receipt at Windows `b3d207cf88cd94bd7da39241535886a08d41ad37`, with a clean working tree.

Delivered Windows commit: `3162fdf7f0fc07bb69c4b460c074b350cc451dd0`.

The delivery contains all 48 Unreal project files and six engineering documents from the reviewed Mac checkpoint. A separate reference-only copy of `packages/utils/src/companionHomeDoor.ts` is under `references/apartment-native-2026-10-10`. All 55 resulting working files and committed Git blobs matched their SHA-256 values. All 22 earlier canonical references were preserved, with no additional or restored GaitPresentation source. Sixteen paths changed; the Windows tree was clean afterward.

We staged exact committed bytes, checked destination hashes again before atomic replacement, preserved preimages under `.git/handoffs`, and made a scoped commit with hooks disabled for that command. The local SSH observation timed out during the original operation. A read-only recovery found that the original process continued, committed the transfer and finished its complete readback. We recovered its receipt rather than repeating source writes.

Local receipt: `output/cribtest11-apartment-handoff-2026-10-10/handoff-manifest.json`. Supporting files in the same directory include the pinned file inventory, pre-transfer bridge status, remote audit and transfer recovery. The receipt records the remote `.git/handoffs` identity and the SHA-256 for every delivered file. Its recorded staged-diff SHA-256 is `3e6789f71ba9b68905bdd9fe904f8fc5ebc4a4782dae45f937a1b624e20bed39`.

## Native prerequisites

A fresh read-only inspection found `C:\Program Files\Epic Games\UE_5.8\Engine\Build\Build.version` reporting Unreal 5.8.3, changelist 58210709, compatible changelist 55116800 and branch `++UE5+Release-5.8`. The editor executable and the native level-instance, streaming, Character Movement, navigation, sky-light and directional-light headers exist. This supersedes the earlier bounded inventory that found no engine directory.

The launcher installation receipt is empty, the project's `EngineAssociation` remains blank, and no project `.uasset` or `.umap` Content was found. File/header presence is not a verified engine startup, asset import or compatible native implementation. `engine-inventory.json` records the inspected paths and metadata.

The next native slice retains one private apartment per account: saved rooms to hallway, hallway to the supplied canonical living room, and living room to an exterior landing. Missing `home-living` in the current narrow projection is not evidence that it does not exist. Complete inventory and exterior admission remain unverified.

Measured studio/bedroom doorways are reference inputs, not imported doorway bindings. Kitchen, bathroom and legacy standing-point fallbacks are not openings. Native common spaces, window geometry, saved content bindings, doorway clearance, daylight, exterior views and safe spawn still require authored content and inspection. The neutral inspection stage's pawn clamp and reset spawn remain separate from apartment traversal.

## Native dispatch

Native turn: `01a125c8-4d22-7b02-8d2c-4a26dc7ca458` in the existing Windows chat. We received its `inProgress` acknowledgement and kept the bridge connected until its terminal `completed` event. Exactly one implementation prompt was sent. The turn checkpointed seven files at Windows `e4491a6d78ed965dbf70aabc028d95714d50449e`, with a clean working tree and no push.

The first `thread/resume` acknowledgement and a subsequent full-turn read timed out before dispatch. A longer read then confirmed the chat was idle with no later turn. After an acknowledged resume, we sent the single implementation request. No service was restarted and no duplicate implementation request was sent.

The brief was scoped to the existing Windows chat, the standalone Unreal source and this record. It required native Unreal facilities, preservation of saved room transforms and existing lifecycle clearing, and explicit unavailable states for missing content or authority. It prohibited another mapper, renderer or pathfinder and any expansion of the live one-room grant.

## Native return and corrections

The return audit matched all 51 Unreal project/record files against both committed Git blobs and working files. We imported only the six native project changes onto the Mac. Newer parent Session, Snapshot, HomePresentation, MovementProtocol and engineering-document work was preserved. The original Windows record and complete source snapshot remain in `output/cribtest11-apartment-handoff-2026-10-10/native-return`; `native-source.json` and `native-source.diff` retain the readback evidence.

`UBaddieCompanionNativeApartment` is a transient default subobject of the existing controller. It uses the installed runtime `ULevelStreamingDynamic::LoadLevelInstance` API with rigid whole-level transforms, initially invisible streams and no reuse of old private instances. `ALevelInstance::SetWorldAsset` was inspected but is editor-only. Controller invalidation owns native clearing on access failure, exact snapshot replacement, re-pair, offline entry and teardown. Pending loads are hidden and requested for native unload without callbacks retaining old private plans.

Doorway checks preserve the measured source entrance and its quarter turn. Saved furniture and room-camera coordinates receive only the whole-room apartment transform. Native navigation queries use the existing Character Movement agent and Unreal NavMesh; camera candidates remain with the existing controller. Authored sky, sun, atmosphere, window, exposure and seating bindings are integration hooks, not verified daylight. No pawn is spawned or moved, no camera is adopted, and the neutral inspection clamp/spawn remains separate.

Both independent reviews found the same two dormant defects. We corrected the living/exterior connection to require its own named authored opening while preserving the canonical room/hall entrance; reused door/approach anchors and coincident doorway positions are rejected without inventing exit coordinates. Ordinary unavailable camera/navigation queries now preserve pending streams. Invalid authority, replacement snapshots and invalid loaded bindings still clear private content. Follow-up source review found no remaining concrete issues in these corrections.

Reviewed Mac native checkpoint: `3f0318fb84455f2427ce71c60e05cef6428a5b72`. Corrected Windows checkpoint: `ee6970422ba50e71b251d163442c7f38aa00dfc3`. The guarded correction delivery changed only the native component and controller implementations. All 58 delivery files plus the 22 earlier references matched committed and working hashes; the Windows tree was clean. Its receipt is `output/cribtest11-apartment-handoff-2026-10-10/native-corrections-manifest.json`.

Controller/panel ownership was released to the parent after the reviewed native checkpoint. We made no further edits to those files and started no further Windows implementation turn.

## Combined client delivery

We reviewed parent increments `b4864f6169947a6ffeaa0455a7a952d4a31c195d`, `be38f357f9709270ba36f1389f67255594ca0c79` and `581f0b66c4a887763a5922474547bad32750d0c1` at the last of those commits. The independent Standards and Spec reviews found no concrete issues. The source cross-check covered the existing world-movement response, separate apartment scope, case-sensitive room-ID pinning, session-only inventory provenance, Home/Enter defaults, confirmation cancellation, expiry and scope-preserving re-pair. Both native corrections remain intact.

Combined Mac checkpoint: `581f0b66c4a887763a5922474547bad32750d0c1`. Delivered Windows checkpoint: `87295378a1ce93db180a879b87909dc7de3d050f`, directly after the corrected native checkpoint `ee6970422ba50e71b251d163442c7f38aa00dfc3`.

The guarded delivery used exact committed bytes for all 50 standalone Unreal project files and seven engineering documents, including `standalone-companion-apartment-access.md`. Sixteen paths changed. Backend/mobile source, uncommitted parent changes and this record were excluded. The existing native record, doorway reference and all 22 earlier references were preserved. All 81 resulting delivery/reference files matched their working and committed SHA-256 values, with a clean Windows working tree.

Before staging and replacement, the helper checked the remote Git state, destination preimages and the completed native turn. It retained backups, made one scoped commit with hooks disabled, and verified the committed readback. The native turn remains `01a125c8-4d22-7b02-8d2c-4a26dc7ca458`; this delivery started zero implementation turns. The receipt is `output/cribtest11-apartment-handoff-2026-10-10/combined-client-manifest.json`.

The parent documents in that delivery retained their checkpoint-era pending-delivery wording. We supplied the completed delivery evidence without editing parent-owned documents. The separate record-only sync reached Windows `e0b4f77cc2ff9ec0fd539bd1972c63a0cda75074` from Mac `381e463f7d277b136767c1784263b032e8a0794b`; `output/cribtest11-apartment-handoff-2026-10-10/native-evidence-manifest.json` records its 81 matching working/committed hashes.

## Authored doorway continuation

We reviewed the exact parent increment `86315dba6dd6a95490c0d7586f9a4ed6e9f79d8f` against its parent. Independent Standards and Spec reviews found no concrete defects or new documented-standard violations. No corrective source change was needed. Parent-owned source and documents were not edited by this coordination task.

The change removes the unconditional measured-doorway requirement for unmeasured saved entrances. `PinkBedroomV2`, `PatStudioV1` and `PinkBedroomV1` entrances still require `SourceFloorYMetres` and reject `AuthoredLocalPose`. Kitchen, bathroom and legacy studio/terrace entrances, common-space openings and the separate living-room exterior exit require an explicit authored pose. These positions already include the imported basis and saved shell design in native level-local centimetres; preparation does not apply extension, quarter-turn or source conversion again. A standing-point fallback cannot supply an opening.

Preflight and loaded-anchor inspection share `ResolveLocalDoorway`. Review covered real unique actor names, finite and distinct door/approach poses, canonical measured positions, the separate living exit, and local pose agreement whether Unreal has applied the instance transform or not. Invalid content still clears through the existing owner. Ordinary unavailable navigation/camera queries retain their non-destructive behavior. Saved-render readiness, standalone admission and walk-through readiness remain false.

A fresh bridge read found no loaded chats and confirmed the same terminal native turn `01a125c8-4d22-7b02-8d2c-4a26dc7ca458`. The remote Git/preimage audit independently found Windows `e0b4f77cc2ff9ec0fd539bd1972c63a0cda75074`, a clean tree and all 81 prior hashes intact. The new importer-note destination was absent and untracked. We did not assume the earlier Windows head was still current.

Reviewed Mac source `86315dba6dd6a95490c0d7586f9a4ed6e9f79d8f` reached Windows `fe64f51d937aaee85aae5303b59543f89abe94c5`. The guarded source delivery included all 50 Unreal project files and eight engineering documents from the pinned commit, including the importer, assets, architecture, apartment/access, account-linking and full-port notes added or updated from `80954e0d5540de31b6ea9102626325e9dc420b6e`. Ten paths changed. All 82 resulting files/references matched their working and committed hashes; the Windows tree was clean. Prior references and the existing native record were preserved.

The following SHA-256 values matched the Mac committed bytes and both the Windows working files and committed Git blobs. Native source paths are under `apps/companion-unreal/Source/BaddieCompanion`; the importer note is under `docs/engineering`.

| File | Matching SHA-256 |
| --- | --- |
| `Private/BaddieCompanionNativeApartment.cpp` | `26683d602002962ea56a049073bc2d9beef95270e69610f17548ce791e06d7f9` |
| `Public/BaddieCompanionNativeApartment.h` | `354ef59754a4ed7b8a14b669332185e05d2b3df72cef753147fdf1669cd64016` |
| `apps/companion-unreal/README.md` | `c8c1ff049fc3db2b1323dc2866e2034c3d45fff87f9fb34dac621ec0dc36ccac` |
| `cribtest11-unreal-importer-2026-10-10.md` | `1f19e279f2db261146ae3c059afc55803eb20d056bcf01140e81d6d65bfdeaef` |

The receipt is `output/cribtest11-apartment-handoff-2026-10-10/doorway-source-manifest.json`. Separate bridge and remote-audit files share the `doorway-source` prefix. The helper retained preimage backups, rechecked the terminal native turn and Git state before replacement, committed only the scoped files with hooks disabled, and completed full hash readback. The separate record-only transfer records its result in `doorway-evidence-manifest.json`; it does not change the delivered source checkpoint.

The asset-preparation script in `packages/companion-scene` and its dependencies stayed on the Mac. We did not run it, convert or import art, invent native Content packages, or perform further importer research. The installed-importer findings in the transferred note remain the parent's source evidence, not observed import or runtime acceptance. No additional Windows implementation turn was started.

## Verification boundary

The native source slice, correction review, parent increment reviews and guarded source deliveries are complete. Native content binding, compilation, rendered inspection and the full game port are not complete. The saved-render-readiness and standalone-admission barriers remain closed; source bindings do not make an apartment walk-through ready. No tests, typechecks, lint, builds, code generation, engine launches, installs, asset conversion/imports, deployment, live configuration changes or pushes were performed in this slice. This native slice did not change production renderer inputs, backend contracts or mobile behavior; no renderer packaging or What's New update was triggered by it.
