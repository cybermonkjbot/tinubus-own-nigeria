# Town Centre entrance crossing handoff

We continued from eee32cb74d9305ccf16581a9579730c50df1a38a. Live inspection confirmed editor PID 13584, the BaddieCompanion project, Unreal 5.8.3 and the expected clean Town Centre map. The previous turn made concrete saved roadside/PCG progress. This turn adds native exterior crossing geometry; Town Centre and the overall game remain incomplete.

## Saved content

Four source-defined Market Road entrance routes now have 1.8m supported crossing lanes: market-road-4, market-road-10, market-road-11 and market-road-12. Each starts at the canonical carriageway edge, ramps from Z=0cm to the existing pavement top at +15cm, crosses the raised pavement band, then descends to the retained entrance support at -1cm. Source entrance positions and road relationships remain unchanged. No destination or district was relocated. Town Centre retains its 138 parcels and source bounds X -556.625..573.92, Z -992.69..28.855 metres, using plan SHA256 13a2699ee6ad513616ca25a41bbb05f165e9aeaf3e52b5d89e550ed8b523ad9a.

Fifteen native meshes and static actors implement ramp, crossing-deck, entrance-ramp and remaining entrance-link sections. These are fitted geometric surfaces using the existing native concrete material, not downloaded replacement artwork. Immutable OBJ inputs preserve native centimetres, inverse installed FBX Y reflection, reversed native winding and measured UV0. Native UV0 reads U=Xcm/271 and V=1-Ycm/271 because the installed OBJ importer flips V. No accepted paving, kerb, drain, material or texture asset bytes were modified. Actor scale remains one. Existing entrance support actors are retained.

The initial import stopped before actor placement on a UV assertion; actual readback also found zero automatically generated simple hulls for the shallow wedge. town-centre-street-pass-37-import-recovery.md preserves the failure and recovery. We reused only the inspected owned in-memory mesh, verified immutable input and native UVs, and used complex-as-simple static triangle collision. This is collision input for static surfaces, not dynamic physics or approved walking. No repeated import, crash, restart or map discard occurred. The saved metadata description predates the corrected script's UV/collision wording; authoritative native readback and receipt supply the actual contract.

Saved packages:

- /Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Maps/L_town_centre_neighbourhood_20261011_streetpass01
- /Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Maps/L_town_centre_neighbourhood_20261011_streetpass01_detail01
- /Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Crossings37/Meshes/SM_market_road_{4,10,11,12}_{section}

The exact fifteen mesh paths, source IDs, source hashes, native bounds, actor transforms, section elevations and retained support identities are recorded in town-centre-street-pass-37-entrance-crossings.json. Both maps safely reloaded after transient authoring and exception references were released. town-centre-street-pass-37-saved-readback.json verifies all fifteen actor transforms, material paths, unit scales, static triangle collision flags, 540 vertex-instance UVs, exact centreline section continuity, retained entrance support and saved native file hashes. Packages are clean; no active game world exists.

## Rendered inspection

We captured and inspected all four before views in previews/town-centre-streetpass01-crossings37-before and all six after views in previews/town-centre-streetpass01-crossings37-after. Matched source camera poses, 165cm height, FOV, native frame warmup and hashes are preserved. The four parcel approaches visibly replace flat unfinished cut centres with raised supported crossing surfaces. Market Road source-centre and Market junction show no introduced placement change. No interiors were changed or accepted; visible vehicles remain static.

This does not complete the full entrance apron. Exclusion cuts can be wider than the new 1.8m lanes, leaving low side areas and abrupt transitions to adjacent paving. Side joins, fitted shoulders and junction mitering remain next work. Centreline mesh continuity alone is not capsule traversal, navigation or accessibility acceptance. Open pavement/entrance side patches in the inspected images remain explicit defects.

Existing defects persist: sparse reachable blocks, uniform oversized concrete ground, warm brown asphalt, Garden Avenue material discontinuity, repetitive merged silhouettes and blank backs, incorrect canonical floor parity especially market-road-10 and market-road-12, dark unstocked shops, missing business signage/services/foliage/activity props, unfinished Market junction and unexcavated covered drains. The full neighbourhood is not recognisably Lagos at the required density yet.

## Inventory and next work

world-inventory-reconciled-20261011-pass37.json retains individual accounting for seven districts, 806 parcels, 264 roads, 19 signatures, 40 destinations, 49 Home recipes, 12 waterfront destinations and five rural areas. Four parcel entries now link their saved partial crossing evidence; no item is newly marked complete. Historical inventories and newer earlier receipts are preserved. A district journal is not individual building acceptance, and failed sibling revisions never erase saved output.

Next dependency-ready ownership remains Town Centre: fit these entrance side aprons and remaining source junction transitions, then author varied exact-floor ordinary blocks, prioritising Library Lane/Cross Town/Garden context. Missing exact-floor shell art, native finished corner/entrance details, approved signage/service assets and source beach/boardwalk finishes remain specific dependencies. Existing independent street work does not wait for the absent three Mac backlog documents. Do not move to another district while this one remains incomplete.

Genuine gameplay remains unverified. Current runtime depends on matching approved native binaries and the existing admitted movement, resident, transport and gameplay owners. No account/movement/readiness gate, codec, wallet or save system changed. The active full-game goal remains open.

We preserved unrelated .uproject, Config, native_generator.py, port-document changes, old maps/captures, unowned assets and the Blender workbench. Native-only independent maps/tooling do not change consumed mobile/web renderer data or registry contracts, so packaging and What's New are not triggered. No tests, lint, typechecks, C++ compilation, full builds, HLOD, app packaging, installs/downloads, service exposure, deployment, pushes or production configuration changes ran. Ordinary mesh import, collision processing, map/asset saves and native editor captures ran; automatic save-time internal validation remains an Unreal side effect, not a separate validation command.
