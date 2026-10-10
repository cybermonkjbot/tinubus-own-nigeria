# Native accessory visibility reuse

October 10, 2026. Read-only SSH inspection of the installed Epic source at `C:\Program Files\Epic Games\UE_5.8\Engine\Source` on `20.109.124.208`. `Runtime/Launch/Resources/Version.h:58-67` confirms UE 5.8.3. Findings concern source behavior, not an executed native scene.

## Safe reuse choice

Reuse public `USceneComponent::SetVisibility(desired, false)` on explicitly accepted, owned `UMeshComponent` instances. Keep authored local visibility separately and leave each component's `bHiddenInGame` untouched. Retain `SetActorHiddenInGame(true)` as the existing whole-character suppression gate. Accessory selection must not open that gate. This is an implementation recommendation derived from the engine behavior below and the character's existing hidden preparation lifecycle. [E1][E2][E3][R1]

For a simple group, capture each mesh's original `GetVisibleFlag()` before any override, then apply `groupEnabled && originalMeshFlag`. On release, restore captured flags only for valid, still-owned bindings, never replacement components or another owner's state. For a hierarchy, include retained local visibility along the authored path below the controlled group and every applicable selection gate. Selection replaces the controlled root's local flag, so an initially disabled root must not permanently veto selection. If that root is itself a mesh, apply its selected state too. Never recapture an already-masked result as the authored baseline. Bindings must establish group membership, ownership and complete drawable coverage; reject ambiguous overlaps or explicitly combine their gates. These are requirements for preserving group semantics, not evidence that such bindings or geometry already exist. [E1][E2]

Do not use a non-rendering parent alone as the group switch. The base visibility/render predicates do not consult ordinary attachment-parent visibility. `ShouldRender` has a separate `UChildActorComponent` parent check, but the current character rejects child actors. Do not use recursive `SetVisibility(true, true)` to restore a group or substitute recursive `SetHiddenInGame` for it. [E2][R1]

## Installed behavior

- `SetVisibility(value, true)` traverses the complete attachment subtree, including through non-mesh components, and assigns the same local visible flag to every descendant. It traverses even when the root already has that value. It retains no previous child state. With `false`, the public overload selects the private `DirtyOnly` policy: changed roots dirty descendant render states without rewriting descendant flags. The enum overload is private, not an application API. [E1][E2]
- `SetHiddenInGame(value, ...)` has the same traversal policy but changes the independent `bHiddenInGame` flag. `true` hides. Its header comment at line 931 says the opposite; the implementation and property declaration establish the actual meaning. `SetVisibility(true)` does not clear this flag. [E1][E2]
- `GetVisibleFlag()` returns literal `bVisible`. Base `IsVisible()` additionally requires `!bHiddenInGame` and a visible cached level collection, when present. It is virtual and does not check the owner's hidden flag. `ShouldRender()` adds actor hiding and world/editor conditions. Neither `IsVisible()` nor `ShouldRender()` is an authored-state snapshot or proof of pixels on screen. [E1][E2]
- `AActor::SetActorHiddenInGame` changes actor `bHidden`, then calls `UpdateComponentVisibility`. That invokes registered components' virtual `OnActorVisibilityChanged`; its base implementation dirties render state. It does not overwrite their local visibility or hidden-in-game flags. Custom overrides still require acceptance. [E3][E4]

## Callback and cleanup requirements

Component setters assign the flag before invoking virtual `OnVisibilityChanged` or `OnHiddenInGameChanged`. Their base implementations call `MarkRenderStateDirty`, which can synchronously broadcast `MarkRenderStateDirtyEvent` before the setter returns. Actor hiding also reaches this callback path. Dirty-only descendant traversal still has callback exposure. The setter traversal uses raw component pointers with null checks, not lifetime-safe snapshots. [E2][E3][E4]

Reuse the character's game-thread prepare/discard/deferred-disposal pattern. Stage baseline and ownership records before setters, reject recursive apply, and treat callback-time clear as cancellation. Defer destruction, detachment, rebind and record disposal until the outer setter stack unwinds. Revalidate weak handles, ownership and cancellation after each call before continuing or reporting success. Post-call checks cannot make destructive mutation inside engine traversal safe. The existing material guard is a reuse pattern, not an already-implemented accessory guard. [E2][E4][R1]

## Canonical source and implemented scope

The parent inspected `packages/utils/src/companionAppearance.ts`, `normalizeAppearance`, and `packages/character-runtime/src/faceRig.ts`, `createFaceRig`, on October 10. Each necklace/hip-chain override takes precedence over legacy `accessoryVisible`; an omitted legacy flag defaults to true. Explicit true still wins over legacy false. The existing C++ appearance mapper already resolves those rules and retains saved presence. The character consumes its effective values without changing normalization or saved data.

`FNativeCharacterContent::Accessories` now declares the exact source node, native mesh/component and explicit `VisibleWhenEnabled` mask for each drawable occurrence. Both source roots are required; unknown roots and overlapping component bindings fail. Preparation validates loaded mesh identity, owner/world, registration and the native enabled baseline before changing flags. Each native component starts with the declared visible mask and `bHiddenInGame` false. This separates retained source descendant visibility from the accessory selection and global actor suppression. Mixed body/accessory components cannot represent independently switched source groups and must not be accepted as bindings.

The actor stages all supplied occurrences, applies `selected && mask` through `SetVisibility(value, false)` and rechecks bindings after setters and after the full pass. A mismatch or callback-time clear fails preparation. Each check also requires the material application generation captured before the accessory pass. Clearing, unregistering/re-registering or replacing that material application invalidates preparation even if the meshes still match. `unreal-native-avatar-ownership-2026-10-10.md` owns the generation contract. The existing apartment asset handle now includes accessory meshes. Content acceptance must still establish actual source membership, complete occurrence coverage and mask provenance. No imported bindings exist yet. This one-shot actor is discarded on clear; it releases its meshes instead of restoring flags for reuse.

Eyewear remains outside this implementation. `faceRig.ts:140-142` shows generated round frames only when the appearance requests them and no `ShopItem:round-glasses` node is attached. `packages/companion-scene/src/interactions/shop/items3d.ts`, `load`, retains the previous attached item during loading or failure, and names the replacement only when its loaded instance attaches. Unequip releases the previous attachment. Thus a requested equipment ID, ownership record or appearance field alone cannot determine generated-frame visibility. The native snapshot supplies no equipment state. The appearance-only attachment requirement is now documented as intent, not final eyewear visibility; no equipment state or native frame geometry was invented.

These source hashes identify the inspected canonical files:

- `faceRig.ts`: `fc3dcb55e2e1d92cf9a0ea7c995c6b0b8a2a001366c6d59b51fa7ca2770e3eaa`.
- `companionAppearance.ts`: `d456526d45f97ed7400ebde27d329d18c03b802503f3823118faeeecabd64903`.
- `items3d.ts`: `466884e09286e24ce8f33cb9f59c905420739d7a9670832790f4c92fa24ec48d`.

The handoff review also found that the previous actor guard missed material restoration from unregistration. The corrected lifetime now defers unsafe mesh disposal and actor destruction across material/presentation mutation, including accessory setters, and finishes through a weak native core ticker. Read `unreal-native-avatar-ownership-2026-10-10.md` for that correction and ticker evidence. Source/diff review covered omitted and explicit choices, missing/duplicate bindings, hidden descendants, replacement and callback-time clearing. No tests, compilation, asset import or rendered inspection ran. Admission and render readiness remain closed; the production renderer and shared contracts are unchanged.

## Evidence and limits

Engine paths below are relative to the inspected source root.

- E1. `Runtime/Engine/Classes/Components/SceneComponent.h:181-226, 851-935, 1641-1659`; `Runtime/Engine/Classes/Components/MeshComponent.h:17-24`.
- E2. `Runtime/Engine/Private/Components/SceneComponent.cpp:3479-3519, 3555-3681`.
- E3. `Runtime/Engine/Private/Actor.cpp:5313-5320, 6352-6368, 6939-6943`.
- E4. `Runtime/Engine/Classes/Components/ActorComponent.h:1209-1210`; `Runtime/Engine/Private/Components/ActorComponent.cpp:2695-2709`.
- R1. Repository `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionCharacterActor.h:18-24, 46-62`; `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionCharacterActor.cpp:12-23, 53-73, 88-122, 135-154, 176-203`.

The research sidecar wrote only the initial native-engine findings. The parent added the canonical source and implementation section. This note establishes no equipment mapping, imported geometry, accepted native hierarchy, rendered parity or release readiness.
