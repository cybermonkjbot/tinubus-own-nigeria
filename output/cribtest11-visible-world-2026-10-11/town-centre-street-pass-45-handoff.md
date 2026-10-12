# Town Centre apartment and entrance handoff

We continued checkpoint ceba2890ffa2ecdfa3f7c68a4365af2f9dd318cc in the existing chat. Authenticated inspection confirmed PID 13584, BaddieCompanion at C:/dev/cribtest-game/apps/companion-unreal/, UE 5.8.3-58210709+++UE5+Release-5.8. The running editor remained in use; no worker, restart or build was requested. Recorded work is October 11, 2026 local / October 12 UTC. The full-game goal remains active and incomplete.

## New exterior content

Pass43 added eight source-centred exact-floor apartment shells: station-road-1 (8 floors), station-road-4 (9), station-road-9 (9), station-road-10 (6), station-road-12 (7), college-lane-8 (7), college-lane-10 (8), college-lane-12 (9). Accepted individual wall, window, balcony and AC modules retain original native hashes, pivots, UVs, material paths and unit scales. Eight new supported structural frames retain the source floor heights. Exact 3,095 actor identities, transforms, source hashes and OBJ inputs are in town-centre-street-pass-43-parcel-frontage.json and blocks43-import-source/.

Initial pass43 captures exposed old merged frontage overlapping the new apartments. A tag-only absence check was insufficient because original streetpass actors lack parcel tags. Pass44 reconstructed original canonical frontage rows and removed 498 superseded actors only after matching their source/module/index, owner, level, mesh, position and yaw. All removals are preserved in town-centre-street-pass-44-superseded-frontage.json. Initial overlapping images remain failure evidence. Future shell generation now rejects unreconciled original frontage labels, not only parcel tags.

Pass45 replaced the eight provisional shutters with closed residential steel double-door portals, framed leaf edges, pull handles, concrete surrounds and projecting canopies. Eight 1.8m-wide fitted paths connect the unchanged saved entrance points to their door centres. These are exterior support paths, not new public destination routes or enterable lobbies. New geometry reuses existing concrete/painted-steel materials without changing the original material assets. Ten new native mesh packages and immutable OBJ inputs preserve native cm, installed OBJ axis reflection, measured UV mapping and complex-as-simple static collision. Unit actor scale and BlockAll remain authoring inputs, not movement acceptance.

## Saved packages and inspection

Exact saved maps:

- /Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Maps/L_town_centre_neighbourhood_20261011_streetpass01
- /Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Maps/L_town_centre_neighbourhood_20261011_streetpass01_detail01

New shell packages: /Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Blocks43/Meshes/SM_{station_road_1,station_road_4,station_road_9,station_road_10,station_road_12,college_lane_8,college_lane_10,college_lane_12}_frame.

New entrance packages: /Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Entrances45/Meshes/SM_residential_{surround,door}, and SM_{station_road_1,station_road_4,station_road_9,station_road_10,station_road_12,college_lane_8,college_lane_10,college_lane_12}_threshold. Individual package paths, source geometry hashes, UV probes and native hashes are enumerated in town-centre-street-pass-45-residential-entrances.json.

Both maps saved and safely reloaded after transient module/World/actor/component references and Python exception references were released. Pass43/44 readbacks preserve earlier map revisions. Current town-centre-street-pass-45-saved-readback.json verifies 3,087 retained shell actors, 24 new entrance actors, eight removed provisional shutters, continued absence of the 498 superseded original actors, exact transforms/materials/unit scales/collision profiles, current native hashes and clean packages. No game world exists. All engine requests completed.

Directly inspected preview sets:

- previews/town-centre-blocks43-before: two source-centre street views.
- previews/town-centre-blocks43-after: ten initial views, including preserved overlap failure evidence inspected in the prior continuation.
- previews/town-centre-blocks44-corrected: ten matched corrected views, all inspected; removed original frontage no longer visibly overlaps the new shells.
- previews/town-centre-entrances45-before and previews/town-centre-entrances45-after: eight matched views each, all inspected at 165cm camera height, identical pose/FOV and existing daylight.

Doors, paired handles, canopies and fitted paths are visible in the brighter Station4/10/12 images. Other orientations are heavily shaded; dark finish/readability remains a defect, not a reason to alter matched lighting. No obvious new facade intersection is visible. Paths sit close to the retained ground and remain plain, with visible material boundaries. Wide block cameras crop some upper roofs; complete roof inspection is not established.

## Coverage and remaining work

Town Centre remains the selected district: source X -556.625..573.92 and Z -992.69..28.855 metres, 138 parcels. Authority remains lagos-city-v3.json SHA256 13a2699ee6ad513616ca25a41bbb05f165e9aeaf3e52b5d89e550ed8b523ad9a. UE axes remain 100*(source Z, source X, source Y). Station Road and College Lane gain ordinary pass-by residential fabric without relocating roads, parcels or entrances. No district/coastline composition decision is invented.

Existing destination relationships are preserved: supermarket market-road-5, salon market-road-2, gym cross-town-road-11, cinema cross-town-road-18, library garden-avenue-13, arts-centre garden-avenue-11, Dance Again market-road-6 and civic-street-2 courtyard/reception/lounge. This batch completes no destination-to-destination route. Upstream entrance/drain crossings, coherent junction corners and surrounding ground context remain unfinished.

world-inventory-reconciled-20261011-pass45.json accounts for all seven districts, 806 parcels, 264 roads, 19 signatures, 40 destinations, 49 Home recipes, 12 waterfront destinations and five rural areas. It adds entrance evidence to the eight parcels and retains newer native evidence instead of historical zero-output conclusions. No inventory item is accepted complete.

Remaining defects: repeating facade grammar and grey finishes; sparse adjoining blocks and open ground; unfinished apartment rooflines/rainwater routes; pass42 shop roof finish rejection and unfinished outlet fittings; missing compounds/gates, forecourt differentiation, services, signage and accepted foliage coverage; dark entrance finishes; plain fitted approach paths; incomplete road/drain shoulders and junctions; Market floor parity, warm asphalt and Garden material break. Closed doors have no action or lobby. Interiors remain deliberately deferred.

Next dependency-ready area stays Town Centre: compound boundaries and more varied exterior finishes around these source-linked apartments, remaining College Lane shops/townhouse and Arcade Crescent parcels, with original frontage reconciliation before mutation. Source-defined junction/entrance shoulders take priority over interior expansion. The three optional October11 team-backlog documents are absent locally; they did not prevent this work. No new reference download is required for the next source-defined exterior batch. Approved replacements and local native-ready manifests are required before using rejected foliage/other unfinished art.

This is editor-authored static content only. Matching approved native binaries plus existing admitted gameplay entry, movement/control owners, character bindings and navigation integration remain the runtime dependency. No capsule traversal, playable destination routes, working traffic, residents, missions or live account access was verified. Gameplay source, codecs, wallets/saves, credentials and readiness gates were not modified.

We preserve unrelated .uproject, Config, native_generator.py and port-document changes, unowned generated assets, prior maps/captures and Blender workbench. Checkpoint only owned tooling, two maps, Blocks43/Entrances45 assets, inputs, receipts, previews, inventories and this handoff with hooks disabled. Native-only authoring does not change consumed mobile/web renderer contracts, so packaging and What's New are not triggered. No tests, lint, typechecks, C++ compilation, project generation, full builds, HLOD commands, app packaging, installation/download, deployment, configuration change or push ran.
