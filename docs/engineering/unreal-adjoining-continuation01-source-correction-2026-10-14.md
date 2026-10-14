# Adjoining Continuation Source Correction

October 14, 2026. Cribtest11, `C:/dev/cribtest-game`. Correction base `a3de996c921f83b881b7075bfad19cebdb0ffc7a`; prior reviewed base `72efbc091213a8c3a126915a42047e4ba1d64fb5`. The correction checkpoint is the commit introducing this document. Source only, existing owner/chat, no native execution or new baseline. Parent re-review remains required.

## Owned Changes

- `Scripts/StreetFinish/add_adjoining.py`: adopt the existing five default constants and substitutions in native_kit, imported_rows, install, generate and audit. Defaults preserve the original adjoining helper. No other helper refactor.
- `Scripts/StreetFinish/continue_adjoining01.py`: verify shared function-global namespace before configuring the adapter; fail-closed ground preflight and exclusive attempt marker; versioned future protected-state capture/comparison; precise historical audit fields; explicit transitive provenance list.
- `Scripts/StreetFinish/capture_continuation01.py`: compare protected captured fields before/after each future image and report the actual evidence schema/coverage.
- Historical handoff: narrow protected-state claims and queue the next world task only after correction acceptance. This correction document supplies dependency closure and limits.

No `.umap`, `.uasset`, material, source OBJ or original output receipt was changed. Dirty `.uproject`, Config, ground helper, native generator and port document remain excluded. The staging list contains only the five paths above (two documents and three Python files).

## Committed Dependency Closure

Paths below are relative to `apps/companion-unreal/Scripts`. Unchanged dependencies are pinned by the base commit and Git blob IDs below, not merely local SHA256 evidence. The amended entrypoints and this document are pinned by the correction checkpoint tree. There is no uncommitted behavior required by the continuation's selected function paths.

| Path | Git Blob |
| --- | --- |
| StreetFinish/add_adjoining.py | bec2b544d8a5eed50c57c807c0944d0ab8ec14a6 |
| StreetFinish/add_adjoining_ground.py | 515122990df82ab2f69ecd99804e37cf9e223c0e |
| StreetFinish/build_street.py | 7bcf0b4406c3aebcecf0e7fd0751fad27138f67d |
| StreetFinish/add_biomes.py | 3fcae333a349aa63adbe8089a5049cd751295006 |
| StreetFinish/add_forecourts.py | 06f4927e0245b5508c9d912da0b586f45c49e8a3 |
| WorldGeneration/Connected/town_centre_native_grammar65.py | eca58e3fc431922d657876a9e1f771afb55e9a13 |
| WorldGeneration/Connected/town_centre_fabric_pass49.py | baf4e25c4863d0566c2c546ec2855e69cfdb5f02 |
| WorldGeneration/Connected/town_centre_blocks_pass41.py | 909958a7b43d66efddb4c6c7e8528480e04f41f6 |
| WorldGeneration/Connected/town_centre_roofs_pass42.py | b4ea7352d8b9afcf38c388255e4f0ff9141da80d |
| WorldGeneration/Connected/town_centre_crossings_pass37.py | 955fdbefece5c04751640278697d147932dc3f55 |
| WorldGeneration/Connected/town_centre_street_pass.py | 9ce76f81316dc9216f686c379c3759ec6417f0e0 |
| WorldGeneration/Connected/capture_town_centre_views.py | e685e0c90f5ecde752c45526d3dede8df25fad84 |
| WorldGeneration/Realism/author_slice.py | 1a3c4ae6264b1a1154fe46e0e17919ca0ae96e25 |
| WorldGeneration/Realism/batch_frontage.py | 699ed7beb6a86e1e12499ae520b180757138e061 |
| WorldGeneration/Realism/assemble_frontage.py | e6f11b469a4b6d5172db64c326f7ca9002f55105 |
| WorldGeneration/Realism/frontage_import.py | 5c4b08361430a060fe60d5c12c02832bb852dd5f |
| WorldGeneration/Realism/slice-config.json | e212430c83adf83b77ccfc05c6af40700d159c0e |
| WorldGeneration/source-lock.json | eecbf1f5eb62eadb19fe593f5cdd0a88ebb17716 |
| WorldGeneration/__init__.py | 54195a37577e09b442c222c1c01b573d1db9629d |
| WorldGeneration/bridge_request.py | eb901253afbea1c306ed80393e772b52f9589b2c |
| WorldGeneration/native_generator.py | f5b4196a7c5012ffba34c91d965ad5425ca12951 |
| WorldGeneration/canonical.py | 5c37042d1d9a1fc9a6bb87f75746af4624e1c9f5 |
| WorldGeneration/geometry.py | 95921ff348936db2fa1d0dd70001921d7dfbf1c9 |
| WorldGeneration/recipes.py | af25b24bea6c7c9e85a40a94f1527d884671931d |
| mcp_client.py | 0d92485fbb0acabfe09c47e3c77ba4ff56bc5a1e |
| python_bridge.py | 50c028b4c9088fafcedadb30247836a959832627 |

Import/function survey: continuation runpy-loads adjoining and ground; adjoining loads street/biomes/forecourts. Its native kit loads grammar65 -> fabric49 -> blocks41/roofs42 -> crossings37 -> street pass. Street pass imports author_slice and batch_frontage -> assemble_frontage -> frontage_import, plus canonical. Author_slice imports native_generator.generator_identity; native_generator imports canonical/geometry/recipes and reads all root WorldGeneration Python hashes on import (bridge_request included). Capture loads capture_town_centre_views -> mcp_client and uses python_bridge -> installed Epic remote_execution. Its configured empty receipt camera list selects the direct viewport branch, not alternate street-pass camera preparation. The correction adds runpy-loading continuation for pre/post capture comparison.

The ground continuation calls only unchanged `area`, `rectangle`, `transform`, `box_poly`, `cut`, `inside` from the ground helper. Dirty parameterization of its prepare/install/audit functions is unused. The native-generator dirty Rotator hunk belongs to NativeAuthor and is unused here. Its import-time source identity will differ between this dirty workspace and a clean checkout; that value is not used to grant or validate continuation authoring. This closure does not authorize invoking NativeAuthor or make its dirty behavior reproducible. Connected helpers are tracked and clean; the original provenance's broad Connected glob was overinclusive and omitted transitive imports, now listed explicitly for future provenance.

Exact historical working hashes match the original provenance: adjoining `d433ff4d14d748ad4a0246de89948e6db2d8de7403dd5e64354b245852081c03`; ground `eced618ba17d4b5e024a11650c4e987151bf2729e0c2255d3fad1054379bced5`. Git normalizes line endings, so working-byte SHA256 is not a Git blob identity. Only adjoining's necessary parameterization is adopted.

External prerequisites remain separate from Python source closure: installed Unreal/Python plugin and authenticated loopback MCP; frozen reference files under `.git/operations/world-generation-20261010/readonly-source` (source commit `e978763b74acfb0369b65b879af6a3a8c90b110b`); city SHA256 `13a2699ee6ad513616ca25a41bbb05f165e9aeaf3e52b5d89e550ed8b523ad9a`; coverage signature inventory; accepted frontage-v3 native and frontage-v2 ISM manifests; adjoining/forecourt surveys and ground receipts; forecourt converted manifest/native modules. These are retained data/native dependencies, not newly generated or certified by this checkpoint. Python standard library and installed `unreal` are external interpreter dependencies. Realism and Connected use namespace packages (no missing required initializer).

## Safety And Evidence Limits

Ground checks any existing completion/attempt receipt without parsing or replacing it. Any source directory remnant, candidate owned/labeled ground actor, matching target mesh reference, loaded package, registry asset or package sidecar blocks the operation. All candidate packages and polygon geometry are prepared before the first import; the exclusive attempt marker precedes imports/resaves/spawns. Recovery is intentionally refusal, not guessed reuse. An invalid/partial receipt or failed attempt requires separate explicit recovery review; never remove it to make a rerun pass. Sole-writer operation remains required; this is not a cross-process editor lock.

Future `protected-state-v2.json` captures actor identity/transforms/tags, scene attachment/relative/world transforms, primitive BodyInstance/collision/navigation/render flags, supported shape dimensions and static-mesh body setup, every local/world ISM transform, and spline point transforms/tangents/types. Any unreadable field or unsupported primitive class raises before a mutation rather than silently weakening coverage. No mutation phase falls back to the historical baseline. A fresh survey refuses existing output/owned content; it cannot manufacture an original pre-state after completion. This fixed completed layer/output is intentionally non-rerunnable.

Historical read-only comparisons keep the original algorithm and name their limited coverage. Immutable `protected-state.json` SHA256 `d86f3a66099f9850a4ea2436bbd8d21c9f2b72199e87ff34b955d34cf55b1c94`; immutable `provenance.json` SHA256 `35e1bfd976be16fcf9a9822a4c80a2e727aa5392f2672fc2067d0bb632742778`. Earlier broad booleans in original receipts are not rewritten: interpret them only as the historical captured fields comparing unchanged. Omitted pre-state is unavailable. No richer current or pre-state baseline was captured in this source-only pass.

Source/diff review only. Installed engine headers were read for reflected transform/body support fields; new capture APIs have not been executed. Unsupported primitives or unavailable reflected properties may require an explicit read-only follow-up before a future survey succeeds. Mesh pixel/geometry equality, runtime per-shape physics overrides, material internals, non-scene gameplay component state, actual gameplay, performance and release acceptance are outside this captured-field guard. Existing PID10404 is a historical binding, not fresh live editor proof. No editor session or native generator was used for this correction.

Hooks inspected: only sample hooks, no configured core.hooksPath. Owned checkpoint uses core.hooksPath=NUL, maintenance.auto=false and gc.auto=0. No tests, lint, typechecks, build/compile, HLOD, PIE, restart, config change, install, packaging, export, release, push, worker, new chat, automation or goal.

Next-world queue only after parent accepts this range: fresh source/occupancy survey Unity Avenue11, fitted corridor ground/curb/drain contacts toward13/15/17, separate additive layer. Saved/rendered historical native work remains separate from committed source closure and from playable Lagos/terrain acceptance.
