# First visible Theatre milestone — Windows, October 11, 2026

Saved actual native Theatre + canonical core garden/promenade. Four final 705×486 native MCP frames were opened locally; geometry is visible. This is a bare first area, not a finished Lagos world. The parent accepted the exposure diagnostic as visible geometry; final frame acceptance is pending.

## View files

- `theatre-south-promenade-wide.png` — SHA256 `4bb66064cd85912efbdd631898a37fa2f5f920b63f48f289fc7e5a0a7b469a4b`
- `theatre-south-promenade-middle.png` — `f9b69e2c99d9eea394602b4974c232a135a7fb9c5b40b578f2953cc7f1467cad`
- `theatre-south-arrival-close.png` — `09088ea5c6bbc1e32fa6a2eef4853344bc110fc1a7e44f6ac877464a9c79ca73`
- `theatre-elevated-overview.png` — `a62ad1c7c8770316649ef1dfdd25b436bc1c61209c9aa2fef9beef155e0873b1`

All files are under `C:\dev\cribtest-game\output\cribtest11-visible-world-2026-10-11\`. Each final image has a `.capture.json` containing exact camera/FOV readback. D3D11 Microsoft Basic Render Driver produced these frames; no NullRHI or substituted image. Wide/middle/entrance poses match `native-first-block.json`. Overview is an additional editor-only camera.

## Native outputs

Content root: `C:\dev\cribtest-game\apps\companion-unreal\Content\BaddieGeneratedWorld\`.

- `R_067bae887353d8b0d131ca62/Maps/L_city_national_theatre_fbff642fa0.umap`
- `R_0cbe78a195c4949e51337ec5/Maps/L_city_theatre_grounds_69a6242670.umap`
- `Presentation/Theatre_20261011/L_Theatre_Promenade_v1.umap` — SHA256 `a5043afa6462f7e3c1dde32fca805defc62beaf84386f1249df37db55e92828c`
- `Presentation/StreetProps_v1/SM_Danfo.uasset` — `5a2f413c249bdb85da48a3b4aa22624b95bbf787b711e976c9a06b36e2346faa`
- `Presentation/StreetProps_v1/SM_Kiosk.uasset` — `d1b69ba689e29a66fa6dcb220b5d531b7b6fdda83c476b17e5173ee2b2576dbf`

40 real native packages: 3 maps, 14 meshes, 23 materials. Of these, 22 are the immutable guarded-generator outputs; one is the separate presentation map; two meshes and 15 materials are editor-only derivative street props. `milestone-package-inventory.json` records every exact path/hash. No stop-dressing map or prop-integration frame was saved.

## Source and receipts

Canonical source pin: `e978763b74acfb0369b65b879af6a3a8c90b110b`. Accepted Windows correction: `5bc324adeaddbab1085d9d33db5a18359b492941`. Accepted six-file generator identity remains `44e0f0bde9ff18ef6cb72a09bc2e219469944a7a8ba120e30782ee7cbc2f4797`; helpers are in its `Presentation/` subdirectory.

- Pilot authorization: `58c97f305ced46b28096038ec23a947058b5f06787a9b32631614f3ca9fd670f`.
- Grounds authorization: `995c4f844185553a031aad32bbda2f7fb64122e93f1c5bb40af4ff7745c0af87`.
- Immutable Theatre batch: `dfd4db236dbf2d75dea4558b72680b17f92c5ac96cd596ab8fe6aa97f0c0ea9e`.
- Immutable grounds batch: `e9e9e3c5d6cdf0cc30dd8a9e9c0c09f63b7817199ac97c1339843b72d8d99fa0`.
- Presentation repair evidence `theatre-presentation-v3.json`: `4a92f36d27b7af1f9c703df48edc02cb109212315166a85257069bbc24b729f3`.
- Prop import evidence `street-import-v1.json`: `3c7cc2ebc0eff5b91f0fcdb18edc3bb6c0fb11eb212d12ac8a7e47656539b931`.

Read-only source audit matched all 250 pinned files and installed-source references. Post-crash file hashing found no immutable generator map/mesh/material mismatches. Config and project byte hashes still match initial inspection. Historical records remain intact; reconciliation records current human authoring authority and coordinator serialization review, never a separate Joshua manual digest-review claim.

## Rendering repair and limits

The first white PNG and subsequent white game-view PNG are retained as failed evidence. The camera and bounds were correct: Theatre meshes span approximately UE X125500..144500, Y−230600..−209400, Z8..4683 cm. Actor/component transforms are identity; city placement is already baked into vertices. Native import did not relocate or reflect the building elsewhere.

The 50,000-lux presentation sun combined with unsuitable default non-extended exposure settings produced whiteout. Explicit manual exposure, physical-camera exposure disabled and bias −12 restored visible geometry. Native game-view mode removes grid/axes; MCP `bShowUI:false` alone disables only selection/widgets. Canonical camera rotation uses named pitch/yaw/roll arguments after an installed positional-order mismatch. All repairs affect the presentation map only.

Overview shows roof/podium but also exposes the bare garden boundary and missing surrounding land. Entrance is dark; sign lettering, finish, vegetation, ring road, water/context and broader city are absent. No native navigation, light bake or gameplay acceptance is established.

Native props have verified scale 1 and Z=0 contact. Danfo measures 455.100×231.500×215.850 cm; kiosk 313.996×311.222×282.460 cm. FBX front imported as −X, requiring a 180° dressing yaw correction. Source frozen merged FBX lacks UVs; native palette-only materials are unpowered. Import reported degenerate tangent/nearly-zero binormal warnings; visual integration remains unverified. These are original derivative dressing, not registered runtime vehicle assets.

## Crash and recovery boundary

PID13096 exited after both props and 15 materials were saved. At `duplicate_asset(BASE, MAP)` followed by `load_level(MAP)`, Python retained the duplicated World wrapper. Unreal's GC check raised `EditorServer.cpp:2544: World Memory Leaks: 2 leaks objects and packages`; log explicitly identifies `FPyReferenceCollector`. This was not an import failure or a generic software-render-quality issue. See `editor-crash-13096.json`; the byte-matched executed source is retained under `executed-source/`.

No editor is currently running. No failed request was retried and no restart occurred. Recovery source now creates a fresh level and reinstantiates documented canonical actors, avoiding World duplication. It has not executed. Original saved maps, meshes and presentation remain unchanged.

Next authorization requested: one recovery launch using the existing `Baddie-Unreal-Editor-desktop` task, fresh PID/project/clean-state inspection, then saved-prop assembly in `/Game/BaddieGeneratedWorld/Presentation/Theatre_20261011/L_Theatre_Stop_Dressing_v1`, native save and engine captures. Bind the new PID explicitly; do not reuse PID13096 authorization or re-run canonical imports. Use the existing stop `brt-stop:theatre-ring-drive:1:340` berth/shelter references, not the promenade. A road/context handoff is still needed for more than the bare pilot surroundings.

No compilation, tests, lint, typechecks, packaging, source transfer, new services or gameplay activation performed. Normal native import, shader/mesh processing and save-time editor callbacks occurred. Account, movement, mission and utility gates remain unchanged. Entire-world generation is incomplete.

Write ownership: generated Content namespace; `Scripts/WorldGeneration/Presentation/`; generation `execution/` receipts/import inputs; this Windows visible-world output. Incoming `street-props/`, Mac recovery and selection files are read-only and excluded from this worker's checkpoint. Historical preparation outputs, existing Collections/Developers and unrelated work are untouched. Commit identity is recorded separately after the path-scoped checkpoint.
