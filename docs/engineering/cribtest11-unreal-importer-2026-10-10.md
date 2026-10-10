# Cribtest11 Unreal glTF importer inspection

Inspected installed Epic source on October 10, 2026. `Build.version` identifies Unreal 5.8.3, changelist 58210709, branch `++UE5+Release-5.8` [V:1-10]. References below identify exact installed paths, one-based line ranges and full-file SHA-256 hashes. The parent independently read the translator's acceptance list and required-extension rejection block and matched its full-file hash.

## Conclusion

The installed Interchange glTF importer does not support `EXT_meshopt_compression` or `EXT_texture_webp`. Its translator rejects either name in `extensionsRequired`, provided parsing reaches that check without an earlier reader error. `KHR_mesh_quantization` is accepted and has native accessor handling. These are source findings, not observed import results [T:57-83,566-615; R:1140-1165; D:678-756].

| Extension | Installed implementation | Consequence for import preparation |
| --- | --- | --- |
| `EXT_meshopt_compression` | Absent from the translator's acceptance list. Buffer and buffer-view extension handlers only check for unsupported names [T:57-83; E:498-508]. | Supply ordinary decoded buffer views or a complete core-format fallback. Removing the declaration does not decompress bytes [R:184-277; H:37-40,122-124]. |
| `EXT_texture_webp` | Absent from the acceptance list. Texture setup reads the core `source`, not an extension source. Image recognition is limited to PNG/JPEG [T:57-83; R:824-850; C:128-155]. | Supply real PNG/JPEG image bytes and core texture sources, not renamed WebP or declaration-only edits. |
| `KHR_mesh_quantization` | Accepted; `extensionsRequired` enables quantized mesh and morph accessors. Bulk reads handle normalized and non-normalized integer components [T:81; R:373-393,418-433,1140-1165; D:68-119,153-223,302-351,1279-1286]. | No blanket dequantization requirement follows from this inspection. Exact accessor, skin and morph fidelity remains unverified. |

## Complete acceptance declaration

`ImporterSupportedExtensions`, used by `UInterchangeGLTFTranslator::Translate`, contains these 19 names in source order [T:57-83; A:452-504]:

```text
KHR_lights_punctual
KHR_lights
EXT_lights_ies
KHR_materials_variants
KHR_materials_unlit
KHR_materials_ior
KHR_materials_clearcoat
KHR_materials_transmission
KHR_materials_sheen
KHR_materials_specular
KHR_materials_pbrSpecularGlossiness
KHR_materials_emissive_strength
KHR_materials_iridescence
KHR_materials_volume
MSFT_packing_occlusionRoughnessMetallic
MSFT_packing_normalRoughnessMetallic
KHR_texture_transform
KHR_mesh_quantization
KHR_draco_mesh_compression
```

Do not substitute either other declaration for this list. `EExtension` has 22 values plus `Count`, including names the translator does not accept [AH:29-54]. The separate nine-entry `FAsset::SupportedExtensions` contains `KHR_materials_pbrSpecularGlossiness`, `KHR_materials_unlit`, `KHR_lights_punctual`, `KHR_materials_clearcoat`, `KHR_materials_transmission`, `KHR_materials_sheen`, `KHR_materials_variants`, `KHR_materials_specular` and `KHR_materials_ior`. That smaller set is not the set used by this translator's required-extension check [A:124-132; T:566-575].

Preserving material extensions during preparation is not sufficient for acceptance. Every retained required name must pass the translator list. For example, `KHR_materials_anisotropy` appears in the enum but not that list and would still trigger rejection if required [AH:41; A:474-477; T:57-83,566-615].

## Required extensions and downstream behavior

1. `FFileReader` copies every `extensionsRequired` string into the asset and separately enables `bMeshQuantized` for `KHR_mesh_quantization`. It does not reject unknown required names there. It then constructs buffers, views, accessors and meshes; it records `extensionsUsed` later [R:1140-1165,1193-1212].
2. `SetupObjects` stops subsequent setup when its batch records an error. The translator collects unsupported required names, reports a reader error first if present, otherwise emits `UnsupportedRequiredExtensions` and returns `false` when the unsupported list is nonempty [R:1214-1247; T:566-615]. Thus the first diagnostic for a particular compressed file is not established by the required-extension list alone.
3. `CheckExtensions` records unsupported object-level extensions without implementing them. Its severity is normally `Warning`, or `Display` during Unreal automation testing. Merely listing a name in `extensionsUsed` is not the required-extension rejection condition [E:770-784; L:9-18; R:1201-1209; T:566-615].

For meshopt, `SetupBuffer` reads core data URIs, external files or raw GLB BIN storage, checking declared sizes and missing data. `SetupBufferView` uses the core buffer, offset, length and stride. `FBufferView::DataAt` resolves a raw buffer pointer. Neither extension handler decodes compressed storage or supplies missing fallback data [R:184-277; H:37-40,122-124; E:498-508]. No occurrences of either unsupported extension name were found in the bounded 13 `.cpp` files directly under the GLTF parser and glTF translator directories. The explicit declarations and handlers, not that negative search alone, establish the gap.

For WebP, an extension-only texture source does not create a texture: the core source index is invalid and `SetupTexture` records a warning. With a valid core source, its extension handler still does no substitution [R:824-850; E:764-767]. `.webp` and `image/webp` resolve to `Unknown` [C:128-155]. The reader's data-URI branch records an error for an unknown format. Embedded buffer-view images retain that format; external images use the filename classification [R:730-807]. The translator records an error and skips each texture whose format is `Unknown`, before creating its texture node [T:638-666]. Later ImageWrapper or external texture-translator delegation does not bypass that gate [T:992-1034].

Quantization support has limits. Validation includes separate ordinary and morph-target component rules [D:678-756]. Bulk vector reads handle non-normalized integers, but the per-value `AcquireVec3` route only accepts F32 or normalized values; its caller otherwise reaches an `ensure` and zero return [D:302-351,499-517,1170-1193,1279-1286]. This distinction is not evidence of a defect in any Baddie asset without establishing which accessor route it uses.

## Native import path and limits

Reuse the installed Interchange path. The import module registers `UInterchangeGLTFTranslator`; it advertises scene translation for `.gltf`/`.glb`, with mesh, material and animation asset types, then calls `GLTF::FFileReader` [M:91-111; T:465-515]. Native payload branches already cover static meshes, skeletal meshes, morph targets, transform curves and morph-target curves [T:1047-1080,1359-1395]. Import preparation should remove the two demonstrated format dependencies while retaining the saved asset structure for this path, not introduce a new runtime GLB renderer.

Installed source does not prove that the project has enabled the relevant modules, loaded required material packages or completed an import. The translator itself warns about missing material packages [T:476-489]. Native import also sanitizes names and performs coordinate/unit handling, so saved IDs, names and transforms need explicit correspondence checks [A:19-33; T:517-519,551-561,617]. No editor, tests, builds, imports, conversions, installations or deployment ran. No native asset fidelity, playable scene or release is verified here.

## Primary-source evidence

All paths are on Cribtest11. Hashes cover whole files, not just the cited excerpts.

- V: `C:\Program Files\Epic Games\UE_5.8\Engine\Build\Build.version`
  SHA-256 `eab58750e84719489f3ee0d42d05a533ceb357eb3ee511777e9408d27a85f43c`
- T: `C:\Program Files\Epic Games\UE_5.8\Engine\Plugins\Interchange\Runtime\Source\Import\Private\Gltf\InterchangeGltfTranslator.cpp`
  SHA-256 `d42895e6f1d9ac4a6bc74a660950fa2486cf7ee549126f03e8821c3a78637eda`
- M: `C:\Program Files\Epic Games\UE_5.8\Engine\Plugins\Interchange\Runtime\Source\Import\Private\InterchangeImportModule.cpp`
  SHA-256 `8ed892aac2c5fd2b845fecee7ccfb9c60b04093bbd306d50156f5b2fba65c69c`
- R: `C:\Program Files\Epic Games\UE_5.8\Engine\Plugins\Interchange\Runtime\Source\Parsers\GLTFCore\Private\GLTF\GLTFReader.cpp`
  SHA-256 `2def056171f3090078f093043ac2ea251b25a88aa0e5fd583c116dd75813fb87`
- E: `C:\Program Files\Epic Games\UE_5.8\Engine\Plugins\Interchange\Runtime\Source\Parsers\GLTFCore\Private\GLTF\ExtensionsHandler.cpp`
  SHA-256 `5e6877e9207995974de098a84a89c8e6c7b2642ba5eeae54e90d94e45cdecc01`
- A: `C:\Program Files\Epic Games\UE_5.8\Engine\Plugins\Interchange\Runtime\Source\Parsers\GLTFCore\Private\GLTF\GLTFAsset.cpp`
  SHA-256 `9cb255e5188a4de88152d059c857278c5b320898d71fd5ddf3a65c9e20e899c4`
- AH: `C:\Program Files\Epic Games\UE_5.8\Engine\Plugins\Interchange\Runtime\Source\Parsers\GLTFCore\Public\GLTFAsset.h`
  SHA-256 `d05764c91b0b307278814a5d5df7ecad3d8a178ec6ce429f825dd7c2e2a3b88d`
- C: `C:\Program Files\Epic Games\UE_5.8\Engine\Plugins\Interchange\Runtime\Source\Parsers\GLTFCore\Private\GLTF\ConversionUtilities.h`
  SHA-256 `f0cd81c39477b1878ecb1e1364762143b85ba5cb547d8812e5fd4e84b95a51b5`
- H: `C:\Program Files\Epic Games\UE_5.8\Engine\Plugins\Interchange\Runtime\Source\Parsers\GLTFCore\Public\GLTFAccessor.h`
  SHA-256 `d9227a66a68016543f9c825a5e4c1d10cdfcf8c9aa045ead957103ebc16a032e`
- D: `C:\Program Files\Epic Games\UE_5.8\Engine\Plugins\Interchange\Runtime\Source\Parsers\GLTFCore\Private\GLTF\GLTFAccessor.cpp`
  SHA-256 `ca51b4635e4d6fe3cbe1b97f788349e6eade25a9c1bd3a2e0a5cdaa0994774ea`
- L: `C:\Program Files\Epic Games\UE_5.8\Engine\Plugins\Interchange\Runtime\Source\Parsers\GLTFCore\Private\GLTFLogger.cpp`
  SHA-256 `c919bfe2d30d6cb261e7da62ce45b977980f76c500bf5d5584ca18bf48539912`
