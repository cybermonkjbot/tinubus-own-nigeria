# Unreal saved-Companion material reuse

October 10, 2026. Repository source review, Epic documentation and the parent's bounded installed-source inspection. The installed engine is UE 5.8.3, changelist 58210709; consulted Epic pages identify UE 5.8. This note verifies neither patch/device behavior nor native assets, graphs or rendered parity. [R6]

## Source colour contract

The participant rig restores captured per-material colours before applying six-digit hex overrides. These replace the material colour factor, not textures or the entire shaded result. [R1][T2]

| Saved field | Exact source material names |
| --- | --- |
| `bodyColor` | Peach cream fur; Cream guard hairs |
| `topColor` | Jacket fabric; Jacket ribbing |
| `trousersColor` | Charcoal twill; Twill seams |
| `footwearColor` | Ivory leather; Rose leather; Shoe sole |

Missing or invalid top colour selects fabric/ribbing `#e7dfcc/#c6bca5` for ivory, otherwise `#b97e74/#95685f`. Missing trousers/shoes restore authored factors. Missing body colour preserves authored bestie/bunny/bear values; cat/fox/panda resolve to `#a3a6ac/#cf8150/#eee9df`. Prefilling normalized body/top/trousers/shoes defaults `#efc18e/#b97e74/#393634/#e8d3b8` erases absence semantics. [R1][R3]

The face rig independently restores cloned head materials. Body tint excludes Ear velvet. Missing eye colour disables tint, preserving the authored iris. Missing nose colour restores Peach nose unless the base supplies cat `#b97e74` or fox/panda `#393634`. Default markings are cat stripes `#50525b`, fox muzzle `#f4dfbe`, panda mask `#393634`; otherwise none, with effective colour fallback `#795342`. Panda forces ear marking. Face colour setters do not repeat the group rig's hex validation. [R2][R3]

Generated wardrobe cloth uses top/look fallbacks, limb fur defaults to `#efc18e`, and pattern/trim colour to `#eee9df`. These differ from authored-slot restoration. [R7]

## Colour pipeline and iris

Both packages pin Three 0.180.0; the scene's installed package matches. Colour management defaults enabled, working space Linear-sRGB. Hex/CSS colours decode sRGB once; glTF base-colour factors are already linear, while base-colour textures are tagged sRGB. Shader numeric literals are linear, including blush `(0.66,0.22,0.19)` and freckles `(0.18,0.075,0.035)`. [R2][T1][T2]

For channel `c=byte/255`, r180 uses `c<0.04045 ? c*0.0773993808 : pow(c*0.9478672986+0.0521327014,2.4)`. UE's documented `FLinearColor::FromSRGBColor` is the appropriate conversion, not plain byte normalization or gamma 2.2. Keep captured linear defaults linear and preserve material opacity separately. Confirm Rec.709 working primaries and texture encoding/import settings; avoid double conversion. [T1][E1][E2][E3]

Amber iris modifies RGB after texture and vertex-colour multiplication, before lighting:

```text
L = dot(sourceDiffuseRGB, (0.2126, 0.7152, 0.0722))
RGB = lerp(sourceDiffuseRGB, linearEyeTint * clamp(L*2.8, 0.12, 1.5), enabled)
```

`enabled` is 1 only with eye colour, otherwise 0; alpha is unchanged. Preserve the 1.5 upper bound and explicit coefficients. Markings/cheeks require their head-local procedural masks, not just parameters. [R2][T2]

Character/group rendering uses LinearToneMapping, exposure 0.95 and default sRGB output; the community renderer uses 1.1. Equal parameters cannot establish equal rendered output. [R5][T1]

## Native integration requirements

These are integration requirements, not verified implementation. Apply on the game thread. Validate explicit owned mesh/slot pairs, unique bindings, expected parents and typed parameter identities before mutation; revalidate after asset replacement. Global names and layer parameters are distinct. A setter call or readback cannot prove graph wiring; static switches are compile-time, not runtime toggles. [E4][E5][E7]

Own a separate MID per bound instance/slot; preserve original assignments and defaults. `ClearParameterValues` clears all overrides, so rebuild the complete owned state or restore captured values. Cached parameter indices belong only to that MID; invalidate caches on reset/rebind. Confirm helper reuse and MID-parent behavior against installed source. [E5][E8]

Keep MIDs/originals in GC-tracked `UPROPERTY` references; raw pointers or Outer alone are not the retention strategy. Recommended teardown is idempotent, covers rebind/failure/EndPlay/destruction, restores only slots still holding this owner's MID, then releases references. Do not mutate shared assets. Parameter binding, imported hash-matched assets, native graph acceptance and rendered parity remain separate deliverables. [R4][R6][E6][E9]

## Parent implementation and installed-source check

`BaddieCompanionCharacterMaterials.h/.cpp` implements the Apply/clear operation on a registered, standalone-game-world actor component. It consumes the existing plan rather than recomputing appearance defaults. It stages transient MIDs before assigning slots, retains them through reflected references and rechecks all supplied bindings after assignment. Recursive Apply fails without mutation. Clear or lifecycle teardown during a native callback cancels Apply and restores owned assignments before it returns. Invalid replacement input also clears the prior appearance. The subsequent `CharacterActor` source owns this component; its apartment loading/spawning path remains behind the closed admission/render-readiness checks. See `unreal-native-avatar-ownership-2026-10-10.md`.

Cleanup retains the original override separately from `GetMaterial`'s resolved default. An originally empty override must restore to null through `SetMaterial`, otherwise the old mesh's default becomes an explicit override after mesh replacement. A changed mesh also gets null, and another owner's replacement is left untouched. The adapter reads `OverrideMaterials` only to capture/compare the native assignment; it never writes the array directly. Installed `MeshComponent.h` lines 28-31 permit reading that public field but warn against direct mutation. `MeshComponent.cpp` lines 63-98 show the supported setter and its pre-assignment removal callback. Those callbacks are why clearing cannot reset the owned array during an in-progress assignment.

The parent inspected the following files by read-only SSH. Paths are relative to `C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime`. Hashes cover entire files; references describe source signatures, not compilation results.

| File and inspected declarations | SHA-256 |
| --- | --- |
| `Engine/Public/Materials/MaterialInstanceDynamic.h`, lines 24, 109, 176, 187: scalar/vector setters, Create, ClearParameterValues | `b81a2e728a235acd268a0f3b5f18955e5202fb1d5c328b6bf44fc47825e7258a` |
| `Engine/Public/Materials/MaterialInterface.h`, lines 1213, 1217: boolean scalar/vector getters | `bea770f73328fb86c54d3ccd7199925d99dc4d5954d6424981af3b6dd15d66c5` |
| `Engine/Classes/Components/MeshComponent.h`, lines 28-31, 119-121: override storage, GetMaterial, SetMaterial | `77a229264c7fd057f8010fe77b29f9f1ff9ba0118a08c1261b6ce2562ce151e5` |
| `Engine/Private/Components/MeshComponent.cpp`, lines 63-98: setter and removal callback | `5c47d2fbe83e083476555c21e88ae07a6c6f3d062a716a6f76b124d91517f55e` |
| `Engine/Classes/Components/SkinnedMeshComponent.h`, line 1223: GetSkinnedAsset | `e5b118510740d3322436f3473030e0c6e2f72223010bcb0353acc82154794489` |
| `Engine/Classes/Engine/SkinnedAsset.h`, line 46: native base class | `4881dc765f02cc9e5a355a92efbf93d84f3311f921989499c461449b2d2d1ab3` |
| `Core/Public/Math/Color.h`, lines 630, 838-841: FromHex and sRGB-to-linear constructor | `ca03d670b8f69d9345d695b0f03afcc1b4e077745051fd3c915246e950172740` |
| `Core/Public/Misc/Char.h`, line 261: IsHexDigit | `b0a3b64758caddf5151a0396fdb042476bba4d9c9d1eaaeda94318b4b069a0e1` |

The getter's hashed parameter-info conversion and global-name behavior were already inspected in `cribtest11-native-furniture-api-2026-10-10.md`, Scalar parameter lookup. This adapter uses the same native contract. The colour constructor decodes each RGB byte through UE's sRGB table; the adapter then restores the authored vector alpha. Source review covered absent values, duplicates, wrong assets/owners/slots, missing parameters, rebind cancellation and cleanup. Tests, compilation, native import and material rendering were not run for this patch.

## Sources

Repository files inspected October 10, 2026, at HEAD `471a52aac`. Line references identify the inspected source, not native execution.

- R1. `packages/companion-scene/src/interactions/group-calls/rig.ts`, lines 112-131 and 172-194.
- R2. `packages/character-runtime/src/faceRig.ts`, lines 43-76 and 89-185.
- R3. `packages/utils/src/companionAppearance.ts`, lines 3-7, 25-28 and 39-67; `packages/utils/src/companion.ts`, lines 78-89.
- R4. `packages/companion-scene/src/interactions/appearance/gltfAssets.ts`, lines 27-74; `packages/character-runtime/src/appearanceRigLifetime.ts`, lines 6-19.
- R5. `packages/companion-scene/src/Character.tsx`, lines 140-150; `packages/companion-scene/src/interactions/group-calls/GroupCallScene.tsx`, lines 344-356; `packages/character-runtime/src/renderer.ts`, lines 46-50.
- R6. `docs/engineering/standalone-companion-unreal-assets.md`, Engine and project, Verified source files, Import and rendering requirements. This is the prior installation/import inventory, not a fresh native inspection.
- R7. `packages/character-runtime/src/wardrobeRig.ts`, lines 258-280.
- T1. `packages/companion-scene/package.json`, `package-lock.json` and installed `node_modules/three/package.json`; `packages/character-runtime/package.json`; installed Three `src/math/Color.js`, `src/math/ColorManagement.js`, `src/renderers/WebGLRenderer.js`. All Three source paths are under `packages/companion-scene/node_modules/three/`.
- T2. Same installed Three tree, `examples/jsm/loaders/GLTFLoader.js`, lines 3644-3659; `src/renderers/shaders/ShaderLib/meshphysical.glsl.js`, lines 163-187; `src/renderers/shaders/ShaderChunk/map_fragment.glsl.js` and `color_fragment.glsl.js`.

Epic primary documentation retrieved directly on October 10, 2026. Each rendered page identifies UE 5.8; none certifies installed 5.8.3 behavior.

- E1. FLinearColor, constructor and FromSRGBColor entries. `https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Core/FLinearColor`
- E2. Working Color Space, renderer defaults and source-versus-stored texture encoding. `https://dev.epicgames.com/documentation/en-us/unreal-engine/working-color-space-in-unreal-engine?application_version=5.8`
- E3. Texture Asset Editor, sRGB, legacy gamma and source colour settings. `https://dev.epicgames.com/documentation/en-us/unreal-engine/texture-asset-editor-in-unreal-engine?application_version=5.8`
- E4. Material Instances, parameterization, static parameters and runtime MIDs. `https://dev.epicgames.com/documentation/en-us/unreal-engine/instanced-materials-in-unreal-engine?application_version=5.8`
- E5. UMaterialInstanceDynamic, Create, ClearParameterValues, parameter setters, ByInfo and cached-index restrictions. `https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UMaterialInstanceDynamic`
- E6. Object Pointers, GC-tracked UPROPERTY references and weak-pointer validity. `https://dev.epicgames.com/documentation/en-us/unreal-engine/object-pointers-in-unreal-engine?application_version=5.8`
- E7. UMaterialInterface, parameter enumeration and boolean scalar/vector getters. `https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UMaterialInterface`
- E8. UPrimitiveComponent, CreateDynamicMaterialInstance and SetMaterial. `https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UPrimitiveComponent`
- E9. UActorComponent, EndPlay and OnComponentDestroyed. `https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UActorComponent`
