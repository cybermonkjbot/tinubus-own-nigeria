# Windows handoff

Copy both Python and contract files to the same new source directory. Parent prepares a separate owned asset-output base, writes `.owner-yaba-kit` containing `yaba-frontage-replacement-v1`, and assigns the environment marker. No existing Blender scene is opened or modified. A run directory must not already exist.

```powershell
$base = 'C:\dev\baddie-assets\YabaRealismKit'
New-Item -ItemType Directory -Force $base | Out-Null
Set-Content -Encoding ascii "$base\.owner-yaba-kit" 'yaba-frontage-replacement-v1'
$env:CRIBTEST_BLENDER_OWNER_JOB = 'yaba-frontage-replacement-v1'
& 'C:\dev\tools\baddie-blender\blender-5.2.2-windows-x64\blender.exe' `
  --background --factory-startup --threads 2 --python-exit-code 1 `
  --python 'C:\dev\cribtest-game\output\cribtest11-visible-world-2026-10-11\realism-kit\author_yaba_frontage.py' `
  -- --repo-root 'C:\dev\cribtest-game' --output-base $base --run-name frontage-v1 `
  --owned-job yaba-frontage-replacement-v1 --render --samples 24
```

`--render` produces three 960x640 Cycles CPU stills with two threads; omit it for source/export only. Samples 48/96 change preview convergence, not mesh detail. Create `STOP` in the run directory to stop between stills. No performance acceptance is inferred from this CPU preview.

`YabaFrontage.blend` contains an assembled scene and a separate editable module-library scene. `fbx/` retains evaluated thickness, UV0 and material slots, one FBX per reusable module. Preserve imported module object offsets; use each module as one prefab/blueprint or combine its meshes during import, then apply `placement.json` instances. Material binding must follow `material-bindings.json`, not FBX shader guesses. No 100x geometry scale is baked; check importer unit conversion once. Convert tangent normal green once for Unreal's DirectX convention. UV0 repeats at the recorded physical metre period; unique lightmap UVs and collision remain separate work.

Default execution uses explicit constants when Windows originals are absent and reports missing scan channels. Existing Mac originals were inspected and their hashes match repository CC0 provenance. Transfer is not part of this source task. Optional `--material-inputs` accepts the schema in `kit-contract.json`; absolute available Windows paths are allowed when explicitly bound, relative paths stay beneath that input manifest. Maps require provenance/license and exact SHA256. `coverage_m: [width,height]` sets physical coverage; separate `roughness` maps may replace ORM. Use `normal_gl` or `normal_dx`, never both. DirectX green converts for Blender only; the transferred bitmap remains unchanged. Missing files remain unresolved; mismatched hashes fail. The script has no noise textures, network calls, downloads or texture generation. It copies only approved existing bitmaps into its fresh output and packs them in the blend.

The frontage uses local origin [0,0,0]. Dimensions derive from the existing detailed compound recipe, not a dated Yaba photographic survey. Source quality, Yaba identity, imported material response and native frame-time remain unverified. The contract lists texture and production gaps; this handoff is an executable replacement source, not a final-quality claim.
