# Unreal Home and Companion asset prerequisites

October 10, 2026. Read-only source inspection for the next rendering step.

## Engine and project

A fresh bounded SSH inspection of Cribtest11 found Epic Launcher and its redistributable directories under `C:\Program Files\Epic Games`. The launcher's installation receipt contained no installations. No engine appeared in the other inspected standard `C:` and `D:` locations. This is not an exhaustive disk search or an engine installation. The Windows coordinator also found no imported project `Content` directory.

Choose and verify the installed engine before selecting an importer or claiming compatibility. Engine installation, editor launch, import, compilation and rendered inspection remain outside the current source-only authority. Source mapping can proceed while those operations are deferred.

## Verified source files

Both inspected files matched the current SHA-256 and byte count in `packages/utils/src/companionAssets.ts`. The Companion was fetched from its registered public asset URL into memory only. The Home file was read locally. Neither was imported or changed.

| Asset key | Exact inspected source | SHA-256 | Bytes |
| --- | --- | --- | ---: |
| `bestieCharacterV27` | `bestie/characters/bestie-v27a-lossless.552e83aae0f42244.glb` on the registered public asset host | `552e83aae0f422440c913993f8ef25e2a7495406ad9bf86238e87445487806f2` | 15,887,680 |
| `bestiePinkBedroomV2` | `packages/companion-scene/art/rooms/lossless-repack-2026-10-08/pink-bedroom-lossless.7765d65d8419ffa8.glb` | `7765d65d8419ffa86f9136f8fdb22640994af03dfa901b1332443f73463ae2e3` | 4,829,976 |

The older local Companion file `packages/companion-scene/art/blender/bestie-v27a.001dd3046e90c9ad.glb` has a different hash and byte count. Do not silently label it as the current registered asset. Preserve provenance if an approved import conversion uses an earlier authoring source.

## Import and rendering requirements

The GLB JSON headers provide these facts:

- Both current assets require `EXT_meshopt_compression`, `EXT_texture_webp` and `KHR_mesh_quantization`. Verify support in the selected Unreal import path. A `.glb` extension alone does not establish compatibility. No engine importer was invoked in this inspection.
- The Companion has 145 nodes, 65 meshes, 19 materials, 15 skin bindings and 49 distinct joint indices. Fifteen mesh primitives carry 132 morph-target slots in total. Preserve the bindings and morph names rather than flattening the character into a static mesh.
- The Companion contains `Idle`, `Greeting`, `Blink`, `FingerCurl` and `FingerSpread` clips. It has no named walk or run clip. Existing gameplay adds grounded gait, expressions, care/resting, cooking and weapon-holding poses in source. Importing these five clips does not translate that behavior.
- The bedroom has 349 scene roots and meshes, 46 materials, no skins and no animation clips. Preserve each authored transform and the material/geometry identities needed by room customization. Do not merge all geometry into an uneditable object and call saved-room parity complete.
- Neither inspected asset refers to external image or buffer files. The Home uses additional clearcoat, emissive-strength, sheen, specular and transmission material extensions; the Companion uses clearcoat and sheen. Their rendered material parity remains unverified.

For character pose requirements, start with `packages/companion-scene/src/interactions/group-calls/rig.ts` and its `scenes/groundedGait`, `scenes/rigActivity`, `scenes/seatedPose`, `appearance/expressionRig`, `group-calls/cookingPose` and `combat/weaponHolding` dependencies. Use these as behavior references. Follow the Unreal-first implementation section of `standalone-companion-unreal-port.md` to meet those requirements with native animation, warping, IK and suitable Epic sample content. Do not port the Three.js pose machinery line for line. The current runtime plays only its selected Idle/Blink/Greeting clips; keep that distinction from the asset's full clip inventory.

The saved Home/appearance mapping record is `cribtest11-home-presentation-2026-10-10.md`. Use its presentation-space convention when binding imported assets. Importer axis/unit conversion and authored room normalization must be established together so translation, scale or room rotation is not applied twice.

After authorized import, record actual Unreal packages, skeleton and morph bindings, material slots, source hashes and the accepted import conversion. A mapping plan stays separate from installed assets and from in-game verification. No production renderer, asset registry, binary art or shared scene data changed during this inspection.
