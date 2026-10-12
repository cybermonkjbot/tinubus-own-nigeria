# Town Centre roadside network and native PCG handoff

We continued from c5b4acfce6d53b649e71faf1a00ca526f856bc2f on live editor PID 13584, the expected BaddieCompanion project and Unreal 5.8.3-58210709+++UE5+Release-5.8. D3D11 software-rendered editor views remain separate from genuine gameplay inspection. The overall game goal stays active. Town Centre is not complete.

## Saved exterior coverage

Town Centre keeps canonical bounds X -556.625..573.92, Z -992.69..28.855 metres and 138 parcels. The fictional metric plan SHA256 remains 13a2699ee6ad513616ca25a41bbb05f165e9aeaf3e52b5d89e550ed8b523ad9a. We moved no districts, destinations, source roads, entrances or coastlines.

Pass32, inherited pending work, saved 20,081 source-centred kerb, drain, drain-cover and pavement instances on Market Road, Civic Street and Unity Avenue. It removed 4,814 identity-checked owned legacy roadside actors and preserved 1,539 intersection/entrance exclusions. Pass33 added 6,378 hidden subbase instances to support paving undersides without modifying accepted paving geometry, scale, pivots or UVs.

Pass34 now contains a real native PCG graph: Create Points in world coordinates feeds Static Mesh Spawner with 1,141 exact source-centred Market Road kerb placements. Source-ID seeds and source-to-native instance indices are recorded. The generated mesh, material, transforms and BlockAll collision were checked before removing the old owned batch. Generation is on demand; automatic editor regeneration is disabled. This is fixed-point source placement, not spline sampling, building grammar, runtime PCG or city completion. The interrupted constructor and post-save receipt failures are preserved in town-centre-street-pass-34-recovery.md. No crash/relaunch loop or geometry rerun was used.

Pass35 adds 16,543 source-centred roadside modules to bounds-clipped Garden Avenue, Cross Town Road and Library Lane, removes 4,728 matching legacy actors and preserves 1,443 intersection/entrance exclusions. Pass36 supplies 4,872 hidden subbases. All six corridors total 36,624 roadside instances, including the PCG kerbs, with 11,250 subbases. Counts describe evidence, not acceptance.

Saved native packages:

- /Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Maps/L_town_centre_neighbourhood_20261011_streetpass01
- /Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Maps/L_town_centre_neighbourhood_20261011_streetpass01_detail01
- /Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/PCG/PCG_MarketRoadKerbs34

Both map batches safely reloaded after authoring globals containing Unreal wrappers and standard exception references were released. town-centre-street-pass-36-saved-readback.json verifies current saved map/graph hashes, all current roadside transforms and material identities, all support transforms, absent removed actors, zero maximum roadside location error, no duplicate PCG batch and clean packages. No active game world was present.

## Rendered inspection and connections

We inspected eleven matched PCG/support views in previews/town-centre-streetpass01-pcg34-daylight and nine current corridor views in previews/town-centre-streetpass01-corridors36-daylight. Cameras remain at 165cm with exact native pose, FOV, frame warmup, capture and image hashes. The earlier eleven pass32 views remain preserved; that batch was inspected before this continuation. Current Library Lane views show continuous readable kerbs/pavements beside the source carriageway. Matched Market/Civic views retain earlier building and road placements. New parcel views expose missing surrounding context instead of hiding it.

The six source corridors retain existing connections through the native road/ground support network; intersection exclusions still require fitted geometry. Library Lane arcade remains at library-lane-3. Garden Avenue still serves the source positions of City library (garden-avenue-13) and Arts centre (garden-avenue-11). Cross Town Road retains Cinema (cross-town-road-18) and Community gym (cross-town-road-11). Existing Market Road destinations and civic destinations are unchanged. This pass improves roadside geometry, not destination interiors; no new complete destination connection or end-to-end physical traversal is accepted. The library and cinema approach renders expose insufficient native building/context coverage.

The apparent pass32 underside gap is no longer visible in the inspected current approaches. Fitted accessible entrance ramps, mitered corners, segment-end remainders and road junction finishes are still missing. Covered drains are not excavated through inland support. Market junction still reads as an open concrete field. Garden Avenue source-centre exposes a brown-to-grey carriageway material discontinuity and insufficient surrounding context; this view is not accepted as a completed approach. Sparse blocks, enormous uniform ground areas, warm brown asphalt, repeated merged silhouettes, blank side/back walls, missing business signage/services and dark unstocked shops remain. Existing floor-parity defects remain especially on market-road-10 and market-road-12. Static danfo is not working traffic. No interiors were changed or accepted.

## Inventory and next work

world-inventory-reconciled-20261011-pass36-final.json accounts individually for seven districts, 806 parcels, 264 roads, 19 signatures, 40 destinations, 49 Home recipes, 12 waterfront destinations and five rural areas. Historical inventories remain unchanged. Saved item-specific journals supersede old zeros; a failed Town Centre sibling revision does not erase the saved one. Current file evidence establishes partial native geometry only, not live completeness or traversal. The preceding pass35/pass36 inventory revisions remain as correction history; the -final document fixes own-home wildcard readiness to explicit native-shell/admitted-owner dependencies.

One district and 138 supported parcels are partial; six districts and 668 parcels are source-ready to author. All 264 road ribbon records remain partial, not finished routes. Three signatures have saved fallback geometry and sixteen are source-ready, not accepted registered full artwork. Four destinations have partial saved content; 36 require named native inputs. All Home recipes, waterfront destinations and rural areas retain explicit shell/layout, art or metric-terrain dependencies. No category is accepted as complete native content.

Next dependency-ready work remains Town Centre exterior completion: fitted junctions and entrance transitions, then exact-floor ordinary parcel shells and varied adjoining block fabric. Prioritise Library Lane/Cross Town/Garden context now that roadside geometry exists. Do not move to another district while this neighbourhood remains sparse and incomplete. Native PCG can later consume source splines/exclusions and parcel grammar, but must not randomise canonical layout or use rejected art. Missing exact-floor shells and native entrance/corner modules remain specific art dependencies.

The three October 11 Mac backlog documents remain absent and do not block existing road authoring. Existing canonical sources and handoffs remain sufficient for independent exterior work. Gameplay requires matching approved binaries and existing admitted movement/resident/transport owners; editor navigation and BlockAll configuration establish none of those. No readiness, access, wallets, saves or gameplay codecs changed.

We preserved unrelated project, Config, native_generator.py, port-document changes, existing maps/captures, unowned generated assets and the Blender workbench. This native-only pass changes no consumed mobile/web renderer code, shared maps or registered renderer assets, so renderer packaging and What's New are not triggered. No tests, lint, typechecks, C++ compilation, full build, HLOD build, packaging, installs, asset downloads, deployment, pushes or production configuration changes ran. Unreal save-time internal asset validation logged automatically; no separate validation command was invoked.
