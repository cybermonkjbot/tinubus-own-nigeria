# Cribtest11 native avatar handoff review

October 10, 2026, 16:32 BST. Held before transfer.

Reviewed Mac checkpoint: `bafdbfb9eda59096d1c983ef0f4f6808f5fd6567`, the exact
11-file native avatar/apartment continuation. The previous queue was not replayed.
No files from this checkpoint were delivered. Windows remains at
`801fe8f773c2da70783d25185aea2fcf116e29f3`.

## Standards

**P2: defer disposal during material restoration, not only preparation.**

In the pinned
[CharacterActor.cpp](/Users/joshua/Documents/ChatGPT/baddie.ng/apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionCharacterActor.cpp:187),
`ClearPrivateCharacter` calls `Materials->ClearPrivateMaterials` at line 184,
then tests only `bPreparing` at line 187 before `EmptyOverrideMaterials` at
line 193. The apartment's destruction guard at
[NativeApartment.cpp:160](/Users/joshua/Documents/ChatGPT/baddie.ng/apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionNativeApartment.cpp:160)
also tests only `bPreparing`. Line references here identify the pinned revision,
not later working-tree edits.

The uncovered callback sequence is:

1. Preparation has completed, so the actor's `bPreparing` is false.
2. `CharacterMaterials::OnUnregister` calls `ClearPrivateMaterials`, which sets
   `bChangingMaterials` while `RestoreOwnedMaterials` invokes `SetMaterial`.
3. A native removal/lifecycle callback clears the apartment. That invokes the
   actor's `ClearPrivateCharacter` while the material setter is still active.
4. The nested `ClearPrivateMaterials` marks `bClearRequested` and returns because
   `bChangingMaterials` is true. It does not tell the actor that restoration is
   still in progress.
5. The actor sees `bPreparing == false` and empties the mesh override arrays.
   Apartment cleanup can also destroy it before the original setter returns.

Neither actor nor apartment `bClearing` is set when material-driven restoration
begins. Those guards prevent deeper recursive clearing, not this first entry.
Changing the content generation does not defer native array mutation either.

Material mutation lifetime must cover restoration and unregistration as well as
`Apply`. Mesh release and actor destruction must wait until the outer material
operation returns, with an owner/completion path that then finishes disposal and
checks destruction. Adding a busy check without arranging that later completion
would leave cleanup unfinished after the apartment releases its actor pointer.

## Spec

The spec review initially reported no findings. A targeted review of the
sequence above accepted the same defect and corrected that conclusion.

The pinned `unreal-native-avatar-ownership-2026-10-10.md` requires material
restoration before mesh teardown and explicitly prohibits emptying native
override arrays during a setter callback. The trusted-content restriction
excludes gameplay/construction actions; it does not exclude native lifecycle or
material-removal callbacks. Actor-first teardown may avoid this sequence, but
the source does not require every material unregistration to originate there.

Standards: one P2 finding. Spec: the same P2 finding. One distinct defect, no
runtime reproduction claimed. The admission/readiness gates remain closed.

## Destination evidence

Read-only SSH snapshots at `2026-10-10T15:26:18.740Z` and
`2026-10-10T15:32:17.108Z` matched all 11 target paths and 17 protected paths.
Every target preimage matched its expected pinned parent; divergence did not
cause the hold. The review finding did.

The snapshots also matched the previous delivery's protected files. Editor PID
`11084`, existing Codex PIDs `2660` and `5176`, binaries, setup scripts, V7 targets,
session compatibility changes and index were unchanged. The existing uncommitted
`Config/DefaultEngine.ini` and `Config/DefaultInput.ini` modifications remain
byte-identical. No compiler/commandlet process appeared in these snapshots.

Local `plan.json` contains each target's exact expected/payload SHA-256 and source
commit. `unchanged.json` records the final observed destination hashes and state.
Both are under
`/Users/joshua/Documents/ChatGPT/baddie.ng/output/cribtest11-native-avatar-2026-10-10`.
There is no delivery receipt or new Windows commit for this continuation.

## Handoff boundary

The parent acknowledged the hold and owns the correction. Do not transfer until
a corrected pinned handoff is reviewed. New accessory-visibility work or other
dirty source is not part of this payload. No parent-owned source was edited.

Only source review, whitespace/diff inspection and read-only destination
inspection ran. No builds, tests, typechecks, lint, codegen, imports, installs,
engine calls/launches/restarts, deployments, pushes or live configuration changes
ran. The production mobile/web renderer and account access were untouched.
Compilation acceptance remains at Windows
`206a331daf693eff0965cf2a4c539e54393b0c3b`; this review establishes neither gameplay
nor rendered acceptance for the continuation.

## Replacement review and source correction

The record above describes the held `bafdbfb9e` payload. Both reviewers subsequently
accepted the deferred-disposal correction in Mac `ab56e47236fdde3efb12e84710f6b4baf783029d`,
but found another P2. Accessory visibility callbacks could clear or unregister and
re-register `CharacterMaterials` after `Apply` succeeded. The actor checked mesh
visibility but could still report preparation success without its applied materials.
The replacement remained undelivered.

The parent correction gives each successful material application a nonzero,
nonreused generation. Clear, unregistration and replacement application invalidate
the previous generation. Character preparation and transform reconciliation require
the captured generation to remain current across native callbacks. The existing
deferred-disposal correction remains intact. See
`unreal-native-avatar-ownership-2026-10-10.md` for the lifecycle contract.

This correction adds `Private/BaddieCompanionCharacterMaterials.cpp` to the
replacement's explicit source scope. A new pinned checkpoint and review must
precede delivery. The interrupted replacement `plan.json` is not delivery evidence;
regenerate its manifest from the pinned checkpoint and verified destination
preimages. Preserve this historical hold and record the delivery result separately.
