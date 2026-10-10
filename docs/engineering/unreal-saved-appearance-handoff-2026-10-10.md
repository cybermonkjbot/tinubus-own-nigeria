# Native saved face selections

October 10, 2026. Source implementation and review only. No imported content, compilation, rendered acceptance or Windows delivery in this slice.

## Implemented

`CharacterActor` now applies the existing Home plan's eye, brow, nose and mouth choices through native scene-component transforms. This is the first missing appearance behavior implemented here. `faceRig.ts` changes these authored node poses, not morph weights. The adapter leaves skeletal assets, morph targets, blink and speech channels untouched.

| Source selection | Canonical local change |
| --- | --- |
| Eyes | Round width/height `0.97/1.16`; almond `1.08/0.83`, Z rotation `-0.08/+0.08` radians for L/R; sleepy height `0.72`; classic preserves pose. |
| Brows | Soft height `0.7`; bold `1.7`; curious raises only R by `0.025` source metres and adds `0.15` Z radians; classic preserves pose. |
| Nose | Button width/height `0.78/1.3`; wide width `1.4`; classic preserves pose. |
| Mouth | Small width `0.72`; wide `1.35`; classic preserves pose. Applies to Smile, Mouth opening and Lower mouth warmth together. |

The adapter consumes `Effective` choices from the existing mapper. It does not normalize saved documents again or erase presence. Cat/fox/panda defaults and explicit overrides therefore follow the existing `companionAppearance.ts` rules. Unknown or missing effective choices fail acceptance. The overall face/base/wardrobe issue markers remain unported because those recipes have unfinished parts.

Production renderer code, material application, necklace/hip-chain selection, snapshots, grants and readiness gates are unchanged. The actor remains hidden, inactive and noncolliding. No gameplay authority is inferred from Home access.

## Trusted binding contract

`FNativeCharacterContent::FaceNodes` requires all eight source identifiers: `EyeL`, `EyeR`, `Groomed eyebrow L`, `Groomed eyebrow R`, `Nose`, `Smile`, `Mouth opening`, `Lower mouth warmth`. These are the unsanitized identifiers in the source recipe. Native component names are supplied separately; the adapter does not guess imported names.

Each binding supplies the actual native component and parent names, an explicit accepted `AuthoredLocalPose`, and every mesh component/asset below that root. Acceptance still uses the character's existing source key/hash check. The current required source remains `bestieCharacterV27`, SHA-256 `552e83aae0f422440c913993f8ef25e2a7495406ad9bf86238e87445487806f2`. No native package paths or import manifest were invented.

The authored hierarchy must retain local source axes as UE `[z,x,y]` and source metres as centimetres. Root/parent coordinates must use that same conversion, with the authored XYZ Euler convention and no baked face choice. Width changes UE Y, height changes UE Z, and source Z rotation becomes a local UE X quaternion delta. The code right-multiplies that delta onto the authored rotation. It does not substitute UE yaw or scale the position delta by Home's character scale a second time.

Runtime validation checks the instantiated components, actual loaded static/skinned mesh identities, parent identity, local pose, registration, world/owner and complete supplied native subtree coverage. Duplicate names, overlapping roots, omitted/extra meshes, changed descendants and invalid poses fail. Bindings are limited to 256 total scene components. Components must be movable, use positive local scales and inherit transforms without sockets or absolute-transform flags. Bone-only controls, merged face regions, importer axis rebasing and mirrored/nonpositive component scales are unsupported by this binding contract. Do not relabel them as supported; retain suitable independently transformable imported nodes or supply a separately reviewed native mapping.

Loaded identity checks cannot establish conversion provenance, source occurrence coverage or skin deformation fidelity. Content acceptance must verify those and confirm that each controlled subtree moves with the corresponding source node. A shared native skeleton or external pose driver does not automatically satisfy that requirement.

## Lifetime and source review

Preparation stages every root and descendant before changing any pose, then retains weak component/mesh identities and authored transforms. It validates the complete staged state before and after each native setter, after accessory application and during level-transform reconciliation. Material application generation must remain current throughout preparation. Reentry, access loss, material clear/unregistration or replacement still invalidate the enclosing actor preparation.

The existing actor presentation guard spans the native setter stack. Callback-time clearing defers pose restoration and mesh disposal through the existing core ticker. Pose storage cannot be reset while `SetRelativeTransform` borrows its target. Clearing restores only still-matching roots whose components, assets, attachments and poses remain owned by this application. It leaves replacement or externally modified state untouched, drops the weak records, then follows existing one-shot actor disposal. No setter writes shared mesh assets or morph arrays.

Source/diff review covered all four eye choices, all four brow choices, all three nose/mouth choices, base-derived defaults, unknown choices, duplicate/overlapping declarations, missing or replaced meshes, mutated descendants, partial application, callback-time clear, material generation changes and teardown. A duplicate native-name case found during review was tightened before handoff. This is source reasoning, not executed regression coverage.

## Apartment load integration

Mac source commit `a61cfb9e18d441c0dd95cc7627ed3010ee2f725b` completes the load-list integration for face checkpoint `69ff3e17dfb8f2507623e769a2a0ab117d27cf76`. `NativeApartment::PrepareContent` now adds every `Content.Character.FaceNodes[*].Meshes[*].MeshAsset` to the existing `CharacterPaths` through `AddUnique`, before the existing `RequestAsyncLoad`. Class, material, accessory, furniture and level loading retain their existing paths and ownership. No new loader, synchronous load or completion callback was added.

Source review traced declaration acceptance, loaded mesh identity, cancellation, generation changes, preparation failure and handle release. `HasLoadCompleted` is not treated as successful mesh binding: `CharacterAppearance` still compares each instantiated mesh against its loaded declared asset. Missing or mismatched content fails preparation and the apartment clears private content. `ClearPrivateContent` increments the content generation, cancels/resets the load handle and discards the character. Spawn/prepare generation checks and the actor's deferred cleanup remain unchanged. Successful polling releases the handle only after hidden character preparation and reconciliation, when the native components retain their mesh assets.

This bounded follow-up changes only `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionNativeApartment.cpp` and this note. No further source correction was needed for the load integration. Account access, apartment admission and render readiness remain closed. Source/diff inspection only; no compilation, tests, imports, runtime inspection or Windows transfer ran.

## Parent integration review

The parent independently read the face adapter, enclosing actor preparation/clear paths and apartment load/spawn/poll paths at `69ff3e17dfb8f2507623e769a2a0ab117d27cf76` plus `a61cfb9e18d441c0dd95cc7627ed3010ee2f725b`. It compared the numeric choices and transform order against `packages/character-runtime/src/faceRig.ts`. No additional source defect was found in the scoped face/load integration. The review traced staged partial application, identity checks before restoration, deferred callback-time disposal, generation rejection and release of the load handle after mesh ownership transfers to the prepared actor.

This source is ready for a six-file guarded handoff through the existing coordinator: the two CharacterActor files, two CharacterAppearance files, NativeApartment.cpp and this note. It does not depend on the pending gameplay-entry backend or its native caller. The separate gameplay security findings do not authorize or activate this character. Windows delivery, compilation and visual acceptance are still unverified.

## Remaining work

- During the separately authorized content pass, populate the eight bindings from the accepted current character import. Verify local axes/units, XYZ rotations, mesh occurrence membership and retained skeletal deformation. Existing empty content declarations intentionally fail acceptance.
- Wardrobe geometry/fabric/patterns, base silhouettes, head-local markings/cheeks and native animation remain unfinished. No replacement garment, geometry generator, morph name, bone or animation loop was introduced.
- Eyewear still requires current attached-equipment state. `faceRig.ts` suppresses generated round frames only while `ShopItem:round-glasses` is actually attached. `items3d.ts` keeps the previous attachment during replacement loading/failure. Home supplies neither that attachment state nor permission to fetch a full account session. This patch does not treat absent equipment as empty.
- The coordinator remains the sole Windows handoff owner. Compilation, asset conversion/import, in-game appearance checks, activation and release remain outstanding. No tests, builds, engine launches, packaging, deployment or live configuration changes ran.

## Original face checkpoint files

- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionCharacterActor.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionCharacterActor.cpp`
- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionCharacterAppearance.h`, new binding and private state contract.
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionCharacterAppearance.cpp`, new integrated pose adapter.
- This handoff note.

## Primary-source evidence

Repository sources inspected read-only include `packages/character-runtime/src/faceRig.ts`, `baseRig.ts`, `wardrobeRig.ts`, `rig.ts`, `packages/utils/src/companionAppearance.ts`, `companionWardrobe.ts`, `packages/companion-scene/src/interactions/group-calls/rig.ts` and `interactions/shop/items3d.ts`. The source/default and equipment behavior above comes from these files, not a new appearance schema.

| Canonical file | SHA-256 at review |
| --- | --- |
| `packages/character-runtime/src/faceRig.ts` | `fc3dcb55e2e1d92cf9a0ea7c995c6b0b8a2a001366c6d59b51fa7ca2770e3eaa` |
| `packages/utils/src/companionAppearance.ts` | `d456526d45f97ed7400ebde27d329d18c03b802503f3823118faeeecabd64903` |
| `packages/companion-scene/src/interactions/shop/items3d.ts` | `466884e09286e24ce8f33cb9f59c905420739d7a9670832790f4c92fa24ec48d` |

Bounded read-only SSH inspected the installed source under `C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime`. `Launch/Resources/Version.h:58-60` confirms 5.8.3. These checks establish declarations and implementation paths, not compilation.

| Native source | Inspected API/behavior | SHA-256 |
| --- | --- | --- |
| `Engine/Classes/Components/SceneComponent.h` | Lines 461-465, 705, 725, 1574-1625 and 1666: transform setter/getter, socket/descendant access, absolute flags and mobility getter. | `a9e1cecc43547fe2dc10bae4cfdcb49d3951e89717c16c8c3e1bd0831030b1d5` |
| `Engine/Private/Components/SceneComponent.cpp` | Lines 1723-1758, 1865-1898 and 1907-1916: relative rotation/location moves, scale update/overlaps and two-phase transform setter. The actor guard must cover both phases and their callbacks. | `58c6337430d41c3fb9acd8060221a8b46696e0f372c2cc590bc2b865ed89b757` |
| `Core/Public/Math/Quat.h` | Lines 29-34 and 912-924: right-first composition, local delta order and radians/axis constructor. | `5a48b5539fa788ae3eada306f48df3adfef242e0befea35432b46d5b0214cddc` |

Installed Three 0.180.0 source `src/math/Euler.js:446` specifies XYZ default order. `src/math/Quaternion.js:330-335` supplies its XYZ quaternion formula. Incrementing only Euler Z after restoring the authored quaternion is therefore represented by the right-multiplied source-Z delta used here. This source equivalence still needs native rendered acceptance with the approved import.
