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

The delivered parent documents retain their checkpoint-era pending-delivery wording. This record supplies the completed delivery evidence without changing parent-owned documents. Its separate committed-byte sync is recorded in `output/cribtest11-apartment-handoff-2026-10-10/native-evidence-manifest.json` after that operation completes.

## Verification boundary

The native source slice, correction review, parent increment review and combined source delivery are complete. Native content binding, compilation, rendered inspection and the full game port are not complete. The saved-render-readiness and standalone-admission barriers remain closed; source bindings do not make an apartment walk-through ready. No tests, typechecks, lint, builds, code generation, engine launches, installs, asset imports, deployment, live configuration changes or pushes were performed in this slice. This native slice did not change production renderer inputs, backend contracts or mobile behavior; no renderer packaging or What's New update was triggered by it.
