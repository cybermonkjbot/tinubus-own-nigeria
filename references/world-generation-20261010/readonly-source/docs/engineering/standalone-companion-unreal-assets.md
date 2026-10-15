# Unreal Home and Companion asset prerequisites

October 10, 2026. Source inspection and offline import-preparation implementation. No conversion or Unreal import has run.

## Engine and project

The installed files at `C:\Program Files\Epic Games\UE_5.8` identify Unreal 5.8.3, changelist 58210709, branch `++UE5+Release-5.8`. The native editor setup has since compiled its pinned Windows checkpoint and opened the editor, superseding the earlier file-only toolchain status. `cribtest11-python-mcp-2026-10-10.md` records that separately authorized operation, its build scope and warnings. The project's `EngineAssociation` is blank and the launcher supplies the engine path. No imported project `.uasset` or `.umap` Content was found. A parent read-only native level query returned `/Temp/Untitled_1`, an unsaved engine template.

The installed Interchange glTF translator rejects required `EXT_meshopt_compression` and `EXT_texture_webp` extensions unless an earlier reader error stops it first. It accepts `KHR_mesh_quantization` and implements native quantized accessor reads. Its texture path expects core texture sources and PNG/JPEG bytes. Reuse that importer after preparing the two unsupported storage formats. The primary-source paths, line references and hashes are in `cribtest11-unreal-importer-2026-10-10.md`. This is source evidence, not a successful import or a verified engine/toolchain startup.

Asset conversion, engine installation, new editor launches, import and compilation remain outside this source implementation turn's authority. Read-only queries against the existing editor are permitted. This work enabled no engine plugins or native material packages; the separate editor setup does not validate our imported assets or queued source.

## Verified source files

Both inspected files matched the current SHA-256 and byte count in `packages/utils/src/companionAssets.ts`. The Companion was fetched from its registered public asset URL into memory only. The Home file was read locally. Neither was imported or changed.

| Asset key | Exact inspected source | SHA-256 | Bytes |
| --- | --- | --- | ---: |
| `bestieCharacterV27` | `bestie/characters/bestie-v27a-lossless.552e83aae0f42244.glb` on the registered public asset host | `552e83aae0f422440c913993f8ef25e2a7495406ad9bf86238e87445487806f2` | 15,887,680 |
| `bestiePinkBedroomV2` | `packages/companion-scene/art/rooms/lossless-repack-2026-10-08/pink-bedroom-lossless.7765d65d8419ffa8.glb` | `7765d65d8419ffa86f9136f8fdb22640994af03dfa901b1332443f73463ae2e3` | 4,829,976 |

The older local Companion file `packages/companion-scene/art/blender/bestie-v27a.001dd3046e90c9ad.glb` has a different hash and byte count. Do not silently label it as the current registered asset. Preserve provenance if an approved import conversion uses an earlier authoring source.

## Import and rendering requirements

The GLB JSON headers provide these facts:

- Both current assets require `EXT_meshopt_compression`, `EXT_texture_webp` and `KHR_mesh_quantization`. The installed importer supports the last of these, but not the first two. Removing extension names without decoding their data cannot make these files importable. No engine importer was invoked.
- The Companion has 145 nodes, 65 meshes, 19 materials, 15 skin bindings and 49 distinct joint indices. Fifteen mesh primitives carry 132 morph-target slots in total. Preserve the bindings and morph names rather than flattening the character into a static mesh.
- The Companion contains `Idle`, `Greeting`, `Blink`, `FingerCurl` and `FingerSpread` clips. It has no named walk or run clip. Existing gameplay adds grounded gait, expressions, care/resting, cooking and weapon-holding poses in source. Importing these five clips does not translate that behavior.
- The bedroom has 349 scene roots and meshes, 46 materials, no skins and no animation clips. Preserve each authored transform and the material/geometry identities needed by room customization. Do not merge all geometry into an uneditable object and call saved-room parity complete.
- Neither inspected asset refers to external image or buffer files. The Home uses additional clearcoat, emissive-strength, sheen, specular and transmission material extensions; the Companion uses clearcoat and sheen. Their rendered material parity remains unverified.

For character pose requirements, start with `packages/companion-scene/src/interactions/group-calls/rig.ts` and its `scenes/groundedGait`, `scenes/rigActivity`, `scenes/seatedPose`, `appearance/expressionRig`, `group-calls/cookingPose` and `combat/weaponHolding` dependencies. Use these as behavior references. Follow the Unreal-first implementation section of `standalone-companion-unreal-port.md` to meet those requirements with native animation, warping, IK and suitable Epic sample content. Do not port the Three.js pose machinery line for line. The current runtime plays only its selected Idle/Blink/Greeting clips; keep that distinction from the asset's full clip inventory.

The saved Home/appearance mapping record is `cribtest11-home-presentation-2026-10-10.md`. Use its presentation-space convention when binding imported assets. Importer axis/unit conversion and authored room normalization must be established together so translation, scale or room rotation is not applied twice.

After authorized import, record actual Unreal packages, skeleton and morph bindings, material slots, source hashes and the accepted import conversion. A mapping plan stays separate from installed assets and from in-game verification. No production renderer, asset registry, binary art or shared scene data changed during this inspection.

## Source-only import preparation

`packages/companion-scene/scripts/repack-glb-losslessly.mjs` now has an explicit `--prepare-unreal-import` mode. It reuses the existing GLB reader, storage checks, encoder and immutable output writer, plus installed `meshoptimizer` 0.22.0 and `sharp` 0.34.5. Default repacking and its runtime compression options retain their existing behavior. Import preparation rejects those compression options instead of producing another compressed runtime asset.

The mode decodes meshopt buffer views with `MeshoptDecoder.decodeGltfBuffer`, including declared filters. It converts embedded WebP images to PNG and moves each texture's WebP source to its core source field. PNG/JPEG images pass through. It retains `KHR_mesh_quantization`, material extensions, scene/node/mesh/accessor indices, names, extras, morph targets, skins, animation, sampler settings and transforms. It applies no axis/unit conversion, simplification, welding, material merging, resizing or texture recompression to a lossy format.

Before writing, the implementation compares every candidate stream with its source's decoded bytes or converted PNG bytes. It checks PNG dimensions and decoded RGBA values, including RGB beneath alpha, against the current WebP. This preserves the current decoded image, not an unavailable earlier authoring image. A metadata comparison allows only storage offsets/buffers, the two removed format extensions and the corresponding image encoding changes. Unknown extensions, external resources, ambiguous WebP fallbacks, shared image/accessor streams, virtual-buffer metadata, profiled/oriented/animated WebP and unsupported formats fail explicitly. Limits are 16,777,216 pixels per converted image and 512 MiB of decoded/output streams.

The output is a separate content-addressed `.glb` and JSON provenance receipt. The receipt records source/candidate hashes, tool versions, the script hash, unchanged scene inventory, per-stream hashes and texture comparisons. It states that Unreal import, in-game inspection, registration and publication have not run. Writes never overwrite a different existing candidate. The source hash is checked before and after preparation.

The bedroom command below is documented for a separately authorised conversion pass. It has not been executed:

```sh
node packages/companion-scene/scripts/repack-glb-losslessly.mjs \
  --prepare-unreal-import \
  --input /Users/joshua/Documents/ChatGPT/baddie.ng/packages/companion-scene/art/rooms/lossless-repack-2026-10-08/pink-bedroom-lossless.7765d65d8419ffa8.glb \
  --source-sha256 7765d65d8419ffa86f9136f8fdb22640994af03dfa901b1332443f73463ae2e3 \
  --out-dir /Users/joshua/Documents/ChatGPT/baddie.ng/output/unreal-import/bedroom-v02 \
  --name pink-bedroom-unreal-import
```

Source review covered preservation, extension placement, storage bounds, shared streams, failure paths, immutable outputs and unchanged default-mode statements. Only source/diff inspection ran. The converter and its pixel comparisons have not run; actual Unreal compatibility remains unverified. This preparation tool stays in the Mac source tree and was not part of the combined Windows client delivery. It changes no production renderer input and requires no renderer packaging or mobile release-note change.

## Next native content pass

During the authorised content pass, first prepare one hash-matched bedroom candidate and inspect its receipt. Then use Unreal's existing Interchange scene import to retain editable room objects and their hierarchy. Do not collapse the furnished room into one static object. Record source node/material indices against real imported object names and packages, because the native importer sanitizes names and handles units/axes. Reconcile those results with the existing Home presentation-space mapping before placing complete room level instances around the hallway.

Keep authored fixed fixtures separate from saved item instances. `FNativeApartmentContent::FurnitureAssets` now accepts real native static meshes keyed by the existing item kind and exact `studioGeometry.makeRoomObject:<kind>` recipe. Each mesh must preserve the source prop's bottom pivot, materials and catalog scale with the documented basis/unit conversion. Do not bake room object scale, saved item placement, the shell's saved turn or apartment placement into the mesh. The native actor applies room scale and the support-derived saved pose separately. This binding is source-only; no native furniture packages exist yet.

Floor/desk lamps also require one unique bulb material slot and a global scalar parameter that actually drives source emission, with on intensity `0.6`. The actor makes an instance-owned material override. The original lamp has no emissive override. Existing lamp light heights, warm colour, candela intensities and world-space cutoffs are source requirements, not a claim of matched UE exposure or attenuation appearance. Other props have no new light or animation behavior. The installed runtime signatures and streaming caveat are in `cribtest11-native-furniture-api-2026-10-10.md`; geometry, materials and lit/unlit appearance still need rendered acceptance.

The character requires the exact current source hash above, native skeletal/morph/animation payloads and a separate rig/material acceptance pass. The older locally available character is not a substitute. Native payload branches exist, but their presence does not establish fidelity for our character. Do not invent asset paths or mark `IsRenderReady()` true before imported content and its bindings exist. Author the hallway, separate living-room exterior opening, daylight windows and outside approach using the existing native apartment owner. Compilation, movement admission, player-height daylight/clearance inspection and release remain separate work.

## Saved-character material bindings

`UBaddieCompanionCharacterMaterials` consumes the existing material requirements and trusted native content bindings, using per-slot `UMaterialInstanceDynamic` objects. `ABaddieCompanionCharacterActor` now owns it and the apartment owner prepares that hidden visual actor through the existing gated lifecycle. Read `unreal-native-avatar-ownership-2026-10-10.md` before authoring its native class and `unreal-character-material-reuse-2026-10-10.md` before creating material graphs or the binding manifest. They record the lifetime requirements, source names, absence/default rules, linear colour handling and exact iris formula.

The manifest must identify every imported occurrence of each mapped source material, its native component/mesh, authored material, unique slot and global vector parameter. Iris bindings additionally need a scalar parameter, initially zero, that selects the source luminance-tint formula after texture/vertex colour and before lighting. Base-colour bindings replace only the factor. The adapter preserves the vector's authored alpha; the graph must also retain source opacity and texture encoding. Parameter lookup alone does not verify any graph wiring.

Apply checks the declared source key/hash, supplied requirements, native ownership, loaded assets, slots and parameter types before installing overrides. It neither verifies conversion provenance nor discovers missing mesh occurrences, so the content manifest and visual acceptance must establish those separately. Clear cancels reentrant application, restores only slots still carrying this component's MID and releases private parameter values. The apartment owner now cancels loads and clears/destroys its character on invalidation, replacement, unregister or teardown. No imported class/manifest, character graph, wardrobe/face geometry or animation integration exists yet. `IsRenderReady()` remains unchanged and false.

The trusted actor class must contain the complete accepted skeletal/static hierarchy as native component templates, with no child actors, gameplay scripts, automatic input, replication or active preparation behavior. Its root is identity before placement. Preserve the source runtime rig's normalized origin, basis and internal transforms in the authored hierarchy; apply Home's `CharacterScale` only at the actor root. The source checks actor defaults and the completed instance's named material components, but those checks do not audit Blueprint scripts or establish rig/morph coverage. Preparation retains the source preferred Home pose, not a verified collision-clear spawn. No method activates the actor or grants movement.

## Saved-character accessory bindings

`FNativeCharacterContent::Accessories` must enumerate every native mesh occurrence under source `AccessoryNecklace` and `AccessoryChain`, including hidden descendants. Each entry identifies the actual component and loaded mesh and explicitly supplies `VisibleWhenEnabled`. Derive that mask from the retained source hierarchy with the controlled accessory root treated as enabled. The native class starts each bound mesh with that visible flag and `bHiddenInGame` false, while the whole actor remains hidden. This preserves an intentionally hidden descendant without allowing an initially disabled group root to veto a saved selection.

The actor checks supplied identities, unique component bindings, both accessory groups and the declared baseline. It cannot discover omitted source occurrences or prove a declared mask. Import acceptance still owns that coverage and geometry correspondence. Mixed accessory/body mesh components are not suitable bindings; preserve independently controllable native occurrences. Read `unreal-accessory-visibility-reuse-2026-10-10.md` for the exact source precedence, Unreal setter behavior and unresolved eyewear dependency. No accessory packages or accepted manifest exist yet.
