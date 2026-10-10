# Native world-player decoder handoff

October 10, 2026. Source implementation and source review only. This decoder is unmounted and grants no gameplay authority.

## Owned files

- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionWorldPlayer.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionWorldPlayer.cpp`
- `docs/engineering/unreal-world-player-handoff-2026-10-10.md`

`DecodeWorldPlayerDocument` consumes `PlayerDocumentJson` and `ServerNowMs` from the same existing `FWorldMovementResult`. It reparses the bounded document and calls `DecodeWorldMovementResult` for core player, movement, driving and water fields. There is no duplicate driving/water parser or simulation. It publishes `TSharedPtr<const FWorldPlayerView>` containing typed data, with no raw JSON, account session or credential accessor.

`DecodeWorldCurrentInstanceIdentity` consumes the existing snapshot's `instanceJson` and that decoded player. It publishes const identity and typed navigation geometry, not an admitted or rendered instance. It distinguishes the occupied visit from a traveling player's destination. Neither function creates a transport, ticker, owner, actor, token or backend API.

The native GameplaySession checkpoint `8f6c3322a` remains a separate, unmounted entry/presence owner. Backend checkpoint `c0a68c95f` secures entry and presence, not world controls. Nothing in this patch changes that boundary or enables a gate.

## Journey-data continuation

The original three-file delivery remains pinned to parent `e978763b7` and source `6d61d437b72443aa7d8f325a73ebb915fb5a0abd`. This local continuation does not alter that immutable handoff or perform a Windows operation.

The existing `FWorldJourneyRouteIdentity` now retains `Points` and `SegmentDurationsMs` for both the journey and roadside exit route. Both arrays stay inside the const player result, in source order. The shared route reader still enforces the same point-count, coordinate, version/profile and duration bounds. It does not deduplicate points, snap endpoints, recompute length, normalize segment times or convert geographic degrees to native coordinates. The duplicate endpoint pair used for zero-length roadside exits is retained exactly.

`FWorldJourneyTraffic` and `FWorldJourneyEnforcement` replace the prior envelope-only markers. Their fields are data read from the saved journey. The decoder does not fetch traffic, plan or reroll an enforcement stop, infer an offence, sample travel, advance arrival or clear a checkpoint. That journey continuation left control observations, current-instance decoding, movement parsers and clock behavior unchanged.

## Current-instance geometry continuation

The same instance entry point now retains `Navigation`, `Spawns` and `Exit` in `FWorldCurrentInstanceIdentity`. `NavigationRevision` remains available for existing identity consumers and is copied from `Navigation.Revision` after decoding. All arrays own their data inside the const result. No JSON references escape, and a later geometry or identity failure discards the entire candidate.

Scene points retain source metres with +Y up and X/Z on the ground plane. Polygon vertices retain world-space X/Z pairs; rectangle yaw stays in source radians about the rectangle's centre. There is no axis conversion, rounding, angle wrapping, coordinate clamping, sorting or deduplication. Empty obstacles, spawns and graph arrays remain empty, never missing or filled with defaults.

The contract comes from `RoomNavigation`, `NavigationObstacle` and `ScenePoint` in `companionNavigation.ts`, graph limits and reference checks in `companionStreetRouting.ts`, and the instance producers in `companionWorld.ts` and `companionRoadside.ts`. Source review also covered saved Home doors, legacy studio/terrace navigation, city revision dispatch, place builders, waterfront layouts and `racingPlaceLayout`. Direct inspection of the frozen connected-transit recipe found 2,886 obstacles, 2,009 graph points and 2,275 edges; the street-density recipe has 3,109 obstacles, 2,024 points and 2,290 edges. No generator ran.

## Decoded contract and source review

The table records branch-by-branch source inspection, not executed tests. Typed optional fields carry `Missing`, `Null` or `Value` alongside their optional value. Only combinations allowed by each field's wire contract succeed. Omitted fields never acquire invented values.

| Input | Decoding and retained distinction |
| --- | --- |
| Player identity | Required user, character, game room, place, instance and Home room IDs. Comparisons are explicitly case-sensitive. Unknown top-level fields, including storage-only `sessionId` and `readyInstanceId`, reject the player. |
| Revision, sequence and movement | Reuse the existing safe-integer revision, 0 through 1,000,000,000 sequence, required null/object movement and route version-one checks. Route sequence and room revision must match the player. Zero sequence plus null movement remains valid. No comparison with an issued request or prior receipt occurs here. |
| Player status and health | Preserve present, traveling and recovering, plus finite health without clamping. Recovering and health at or below zero are separate observations. A traveling player requires a traveling journey. An interrupted journey can remain on a recovering player after death. |
| Journey | Required null/object. Decode ID, request ID, source/destination IDs, traveling/arrived/interrupted status and car/okada/keke/danfo/canoe/speedboat vehicle. All source times remain unchanged. |
| Journey route | Accept `lagos-corridors-v1` with `proposed`, or `mapbox-directions-v5` with `road-network`. Retain 2 through 4096 geographic points, identity, kind, point count and nonnegative length. Road durations must contain exactly one nonnegative finite time per segment and are retained unchanged. Corridor durations remain Missing with no array; missing/null/incorrectly sized road durations fail. Zero-length retained exits and duplicate points remain valid. |
| Road profile | Missing, `driving-traffic` or `walking`. Null and unknown values fail. The backend's `companionTrafficRoute` declares this field even though the shared base route type omits it. Corridor routes do not accept profile or segment-duration fields. |
| Travel endpoints | Optional, not nullable. Place endpoints retain ID and geographic point. Road endpoints also require paired null/null or value/value vehicle ID/revision. Road ID limit is 85 characters, place ID 128 and endpoint vehicle ID 160. Endpoint-derived place IDs must match the corresponding journey place IDs. |
| Travel mode | Missing, walk, vehicle or boat. No default is inferred from the vehicle field. |
| Journey checkpoint | Required null/object. Retain progress, startsAt and endsAt. `resumedAt` missing preserves legacy timing; explicit null remains a wait; a finite nonnegative value remains the supplied receipt time. No progress, arrival or release is advanced locally. |
| Travel timing | Optional, not nullable. Retain rate 1 or 4, anchor time/progress and positive duration. No acceleration or arrival calculation runs. |
| Journey traffic | Optional, not nullable. Require provider mapbox, fetchedAtRealMs, durationMs, coverage complete/partial/unavailable, required null/number typicalDurationMs and required null/low/moderate/heavy/severe congestion. Preserve missing traffic separately from unavailable coverage, and explicit null separately from zero duration or low congestion. All numeric fields use the existing nonnegative finite safe-number bounds without rounding. Unknown enums, missing required fields, scalar/array/object-type mismatches and explicit null traffic fail. |
| Checkpoint enforcement | Optional, not nullable. Require version 1, agency police/lastma/efcc/ndlea, stopKind checkpoint/roadside, district town-centre/palm-courts/garden-quarter/south-market/marina/skyline/lagoon-gardens, and boolean isNight. Preserve both true and false; reject missing/wrong types and unknown versions/enums. Missing enforcement stays missing and never invokes a legacy planner. |
| Roadside visit | Missing/null/object. Retain the road endpoint, sourceJourneyId, stoppedAt, exit place and the complete checked exit route, including its original geometry and road durations. A previous roadside record may remain during subsequent travel; destination selection uses the journey's endpoint instead. |
| Transit ride | Missing/null/object. Retain ID, vehicle, safe nonnegative seat, boardedAt and visitRevision. Present-player status and matching visit revision are required. Seat capacity and vehicle availability require the transit owner. |
| Transit exit | Required null/object. A supplied exit retains at, finite X/Z/yaw and required null/string stopId. An exit request does not remove occupancy or imply alighting. |
| Nightclub lounge | Missing/null/object at player level. Lounge retains activity ID, visit revision, service ID, joinedAt and optional non-null seatId. Seat occupancy and an existing server-owned route remain visible together. |
| Nightclub dance | Decode round ID/instance/service, startsAt, beat/window duration, required hits and up to 24 lanes. Decode up to 24 received input records without sorting, merging or scoring. Required stoppedAt preserves null versus supplied time. Difficulty remains missing/easy/standard/hard; skillCharacterId and skillsSettled remain optional and non-null. |
| Activity consistency | Supplied activities require the nightclub place, present status and current visit revision. Dance round ID must equal activity ID, and its instance must equal the player's instance. Beat/window/lane/hit guards follow the existing dance bounds. `bDanceActivity` means a retained dance record, not that the round is currently playing. |
| Residence and weapon | Residence preserves missing/null/object, city version, parcel ID and positive integer unit. Weapon preserves missing/null/bat/pistol. Omitted weapons are not retained from an older player by this decoder. |
| Driving and water | Existing parsers retain missing/null/object and all supported control fields. Active water is distinct from landed water. The original storage-only water-field rejection remains in force. No controls are erased to manufacture on-foot state. |
| Conflicting or different-owner transport | Publish conflict and identity-mismatch observations while retaining the controls. Compare player user/character/game/instance, water place/revision and driver/occupant life consistency. Current life is not in this public player view. No mismatch is accepted for gameplay merely because decoding succeeded. |
| Current instance | Decode ID, place, seed, generator version, layout ID, navigation revision, sharedStartedAt and optional roadside identity. Layout and room are required null/object envelopes; optional transitPlan is object-only. These envelopes do not establish complete schemas or supported content. |
| Navigation | Required object with revision, bounds, obstacles, positive finite radius and finite floorY. Optional gridSize is positive, finite and non-null. Missing gridSize stays missing; no 0.12 default is installed. The race-circuit producer supplies 3, so the walking planner's 0.12 through 1 range is not a decoding limit. |
| Bounds and obstacles | Require finite minX/maxX/minZ/maxZ with min no greater than max. Degenerate boxes remain representable. Optional yaw must be finite and non-null. Optional polygon must be non-null with 3 through 8 exact finite X/Z pairs inside its enclosing box; a polygon with nonzero yaw fails. Zero yaw and omitted yaw stay distinct. The same reader handles navigation bounds and obstacles. |
| Street graph | Missing or object, never null. Require points and edges arrays, at most 2,048 exact finite XYZ triples and 8,192 exact integer index pairs. Every edge endpoint must index this graph's points. Empty points with nonempty edges fail; empty graphs, self-edges, repeated points and duplicate edges remain representable. Source order is retained without adjacency construction. |
| Spawns and exit | Required spawns array of exact finite XYZ triples and required exit triple. Missing, null and malformed tuples fail. Empty spawns remain empty. Exit is read independently, never replaced by the first spawn. Y is retained without snapping to floorY. |
| Current visit versus destination | Non-traveling instance IDs must match the player. Traveling instances match journey.toPlaceId and remain labeled TravelDestination. Non-Home IDs match gameRoomId plus placeId. Own Home includes the player user ID and supplied room ID; a current Home also matches homeRoomId. No library-selected destination Home is synthesized. |
| Roadside instance | Match the selected journey/visit endpoint exactly, including coordinates and nullable vehicle pair. Require roadside-v1 generator/layout/navigation, endpoint seed, zero sharedStartedAt and null room/layout. Decode supplied geometry with the same structural checks as other instances; do not substitute the roadside recipe or claim that its geometry matches that recipe. |
| Transport layout | For a current visit, expose mismatched driving navigation revision or water layout ID/revision. This does not establish domain, content, vehicle or life admission. Destination instances do not validate source transport. |
| Failure | Both public entry points reset the caller's output before parsing and publish only after all scoped reads succeed. Every failure returns fixed text, never payload values. A failed decode cannot leave an earlier view in that output variable. |

Both parsers cap input at 2,097,152 TCHAR code units, nesting at 32, and commas/colons outside strings at 131,072 before Unreal creates the JSON tree. The scanner accounts for quotes and escapes; Unreal still validates JSON syntax and the root object. These are parser limits, not a transport receive-memory cap. Ordinary IDs are capped at 240 characters; layout/navigation identities allow 160,000 because saved-room navigation revisions include serialized furniture. New numeric reads require finite values. Counters are safe nonnegative integers; journey/activity times retain fractional source values within the nonnegative safe-number range.

Array storage is allocated only after each existing count bound passes. Each of the two retained routes contains at most 4096 points and, for a road route, 4095 durations. A failed later point, duration, profile, traffic or enforcement read discards the unpublished candidate, including its arrays, and leaves the caller's output reset. No references into the temporary JSON tree survive publication.

Geometry array ceilings are 16,384 obstacles and 43,691 spawns before typed-array allocation, derived from the existing separator budget rather than a new content quota. Four-field boxes consume at least eight separators per entry including their array delimiter; XYZ triples consume three. The surrounding document also consumes that budget, so these are loose upper bounds. Polygon and graph arrays have the smaller limits above. The backend can hydrate chunked JSON without an aggregate content ceiling, so a valid backend document may still exceed this native decoder's limits. Rejection does not authorize truncation or fallback geometry.

Structural decoding does not check obstacle containment within navigation bounds, polygon convexity or area, point clearance, floor proximity, radius clearance, graph connectivity, reachable exits or the walking planner's grid range and 10,000-cell limit. Out-of-bounds scenery obstacles are legitimate in existing recipes. These checks do not establish collision-safe navigation, content identity or movement authority.

## Controller obligations

There is intentionally no `CanWalk`, accepted-route or admitted-control result. The public player document cannot report every movement owner. `companionWorld:move` also consults racing, missions, combat, football, police, detention, readiness and server-side rate limits. An empty set of observations, null movement, null transport, stopped dance or landed water does not grant walking permission.

A future admitted owner must do the following before using these views:

1. Authenticate the existing operation through a separately authorized world-control contract. Entry/presence and read-only Home grants are insufficient. Bind callback generation, selected account/character, current life, game, visit and access lifetime before receiving or publishing state.
2. Compare revisions and sequences with the previous receipt and issued request. A lower returned sequence can be a soft decline or an unchanged other-owner stop. Do not mark the request successful, replay prediction or clear another control owner from that response. A revision, life or visit change retires pending movement and rendering receipts.
3. Apply the existing street-control admission rules for current instance, life, navigation, vehicle/domain, occupancy, timing and previous receipts. Preserve the previous accepted driving/water control and position for older same-control receipts, as `admitCompanionWorldControls` does. Do not confuse driving revision with player visit revision.
4. Reject conflicts and identity/layout mismatches for control use. Enforce water time ordering, expiry and domain acceptance; driving mechanical/configuration bounds; transit capacity and occupancy; and nightclub activity timing. Preserve lounge seat routes and received dance inputs according to the existing owner rules.
5. Update `FWorldPresentationClock` only after scope and response acceptance. Clear clock and private views on access loss, disposal, changed life/visit or a new session in the same game room. Shared pointers copied by a future owner must also be released; resetting one decoder output cannot revoke its other references.
6. Resolve and validate instance content separately. A travel destination must not become the occupied world before server arrival. Require the existing rendered/readiness acknowledgement before movement. Do not invent a Home, roadside endpoint, spawn or fallback navigation.

The existing `MovementProtocol`, `MovementPresentation` and `WorldClock` pairs were dependencies only and remain unchanged by this patch. This implementation reuses the native module's established UE JSON, shared-pointer, array and optional-value patterns. It adds no engine-version-specific gameplay API or module dependency.

## Exact remaining decoding and integration work

This is a player/journey-data and current-instance identity/geometry slice, not a complete world snapshot adapter.

- Journey geometry, traffic and saved checkpoint enforcement are now typed and published. Native presentation still needs an admitted owner and any separately authorized sampler integration. This decoder does not validate geometric route length, endpoint proximity, positive timing totals, traffic freshness, police encounter state or arrival eligibility. Individual zero durations and zero-length routes remain representable; consumers must not assume they support every sampling or settlement operation.
- Current-instance navigation, spawns and exit are now typed. The full layout remains separate, including its nested navigation consistency, asset/fixture transforms and footprint metadata, entrance/spawn poses, recovery/detention spawns, services, NPCs, zones, home-room slots, cameras, theme, quality gaps and waterfront scenery. Layout/asset/generator identities are not approved for rendering.
- Saved room content beyond the Home ID remains separate, including colors, name, environment, layout ID, home design, palette, light, sharing and furniture. Optional floor-patch metadata is not part of `RoomNavigation` and is not retained here, including the extra field on legacy town-square navigation. Room-layout floor patches affect placement, gait and seating height; finite floorY alone does not establish those presentation behaviors. Extra obstacle footprint fields such as id, height and y are also outside this geometry view.
- Hydrated transitPlan remains an object envelope. Its version, navigationRevision, stops, routes, vehicles and internal references need a separate decoder. No consistency with nested layout navigation, room contents or transitPlan is claimed by the new geometry reader.
- Full snapshot fields outside player/instance identity still need their own adapters, including racingJson, transitJson, transitLifeId, fare receipts, checkpointStatus, street traffic, driving catalog, road/faction/hawker/water domains, resident world and peers. The public player has no top-level current life ID, server readiness or complete other-owner discriminator.
- The const data views need an admitted native owner and transport, stale-response handling, session clearing, previous-receipt reconciliation, clock integration, readiness and pawn/camera binding. None is mounted here.

## Evidence and delivery status

Independent review of journey continuation `f5f7578ea99847071eced69a32ca1795ea6bda6b` against `88dc517f403d2b53f7fa131fcd69abc92c97ed52` found no actionable finding in journey/roadside geometry, durations, traffic or enforcement. This is source-review evidence, not compilation or in-game acceptance.

Initial source `6d61d437b72443aa7d8f325a73ebb915fb5a0abd`, pinned by `e978763b74acfb0369b65b879af6a3a8c90b110b`, reached Windows `cdc7dcc03596365cdfbdc8d5a0c62d3f258f66ee`. The coordinator readback at `2026-10-10T17:55:57.779Z` matched all three working/committed files, nine dependencies and 114 protected paths, preserving both Config edits. The earlier transfer receipt is dated `2026-10-10T17:54:06.441Z`. Evidence is `output/cribtest11-world-player-2026-10-10/{plan,receipt,readback}-e978763b7.json`. The parent compared receipt hashes with pinned Mac blobs but did not perform an independent live Windows read. Later journey continuation `f5f7578ea99847071eced69a32ca1795ea6bda6b` remains undelivered at this handoff. These records do not establish delivery of the current-instance geometry continuation.

Independent source review of `6d61d437b72443aa7d8f325a73ebb915fb5a0abd` against parent `6a911f1d21f19d5d6ff8d422ca2090e2e0eb2ede` found no actionable defect in the scoped identity, ordering, nullability, recovery/activity, control-observation, parser-clearing or destination/current-instance behavior. The reviewer used commit-pinned canonical sources and dependencies. Geometry publication and downstream admission were unfinished at that checkpoint. That independent review does not cover this later journey-data continuation. No executable validation ran.

Initial source review traced the canonical `worldPlayerView` validator and `companionWorldPlayerView` projection, snapshot destination selection, move declines and other-owner stops, death-to-hospital interruption, saved Home doors, roadside arrival/exit, and the client accept/control-admission paths. The backend profile alternative and source-versus-destination distinction are included above. Current-instance envelopes and owner acceptance remain explicitly unfinished.

Continuation source review traced `companionJourney.ts`, `companionRoadside.ts`, `companionPolicing.ts`, the district union, backend traffic/policing/world validators, `fetchCompanionTrafficEstimate`, `completeTravel`, vehicle departure validation and checkpoint resume/observation. Review covered both route kinds and both route locations, array order and bounds, zero/fractional durations, duplicate zero-length exit points, all traffic and enforcement enum alternatives, optional versus required nullable fields, and failure clearing after partial array allocation. No actionable defect was found by source/diff inspection. Its subsequent independent review is recorded above; executable validation remains pending.

Geometry source/diff review covered required versus optional fields, null rejection, finite numbers, tuple lengths, inverted and degenerate boxes, polygon bounds and yaw, array ceilings, empty arrays, graph index bounds, immutable publication and failures after partial allocation. Review caught and removed a draft gridSize cap that would have rejected the existing race-circuit value of 3. Existing visit/destination, Home, roadside and transport-layout comparisons remain in place.

Independent review of geometry commit `7ee692aee84109c2ff8a46b15db0daa1a75145e6` against `904e92b7ebab4010307350e138f51ebf1cf7a222` is complete. Standards found zero hard violations and one optional Duplicated Code judgment for the repeated bounded-array read pattern in scene-point and obstacle arrays. That small consolidation is deferred; it is not a behavior defect. Spec found no actionable finding after tracing every new schema branch, the two frozen city recipes, graph and polygon counts, race grid size, immutable publication, failure clearing and occupied-visit/destination distinction. This accepts the scoped geometry reader, not clearance, reachability, art, nested layout schemas, compilation or in-game behavior. Neither review ran executable validation or remote operations.

The source contracts distinguish wire data from admission. The traffic validator uses numeric fields, while current departure producers round aggregate durations and enforce fresh positive bounded times at admission. Segment durations can remain fractional. The decoder therefore preserves fractional nonnegative values and does not apply a new departure's freshness or 24-hour checks to a retained journey. Coverage and congestion remain independently decoded fields; neither is synthesized from the other. Likewise, the server can use legacy enforcement planning when no saved decision exists, but this decoder retains absence and never runs that planner. There is no unresolved field-shape uncertainty in the scoped contracts.

No tests, typechecks, lint, builds, compilation, code generation, asset import/conversion, installs, engine/server starts, live calls, deployments or configuration changes ran. The production renderer, asset registry and bridge contracts are unchanged, so no renderer packaging was triggered. No Windows transfer was performed. Coordinator `01a12172-6903-7ff1-8244-8d386ff688a7` retains sole handoff ownership. Content integration, compilation, runtime lifecycle checks, rendered/gameplay acceptance and release remain pending separately from this source checkpoint.

SHA-256 hashes at the initial source review:

```text
4c3b841e63c2c40d543a6590e1cc635b8e48f1c6e8fe028297be757b9ce9a67a  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionWorldPlayer.h
e41c22d685191189f07bfd6d050f24bedc113b9b642bef1916288c47311dcb1d  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionWorldPlayer.cpp
05b2d4f854ff8e54a8233af45b87f159cd8e90efbd552cbe5a98029b7076e1ba  packages/utils/src/companionWorld.ts
6c989a2b70ba19a412df7112f3fa35970cf56aeacaddc3c9b88ec3f61df71838  packages/utils/src/companionJourney.ts
e57c1742b37c9b71a1e01e2338f9aca049696b64667417007652330ab9950521  packages/utils/src/companionRoadside.ts
00400e99da7cd084184bff5e1dc06b2a1d1e360923f4c1046dd2ea98ded56ec0  packages/utils/src/companionCityTransit.ts
d5443f87b78d65091057b44bc141738acff53fcf2d7cf6a2446974c4655a1c8f  packages/utils/src/companionNightclub.ts
4a29decf12e26f5d3acf091c141e50f6e537dfb7a0460d808cb0963e8c3dc551  packages/utils/src/companionNavigation.ts
0ec5c8a213ee436ccb99bf21c16d506be347bc76c5a55eda9aca5106d8ab9541  packages/utils/src/companionCityHousing.ts
d985dbc931fec8d9c5e0b6a6e5a5aae5b6d2ffd568210e155c2ac9f0f0fa26b7  packages/api/src/companion-world.ts
f6e9b9aa43178ab9c7d699a3c6aacff779990ce7c43b3480ab83d2c00c2a1246  packages/api/src/companion-world-street.ts
46777a5156888c6ebb1544365fdd517f1bebe9129d5d52252d02880e17af3def  apps/backend/convex/companionWorld.ts
008f54db546c7c644feb22d99903cce881c717a480b02b0db2dbc80199271005  apps/backend/convex/lib/companionWorldModel.ts
35dacad3820aaa457f5eb938284b2840692a7f70c5b0a221ae4befd3e8053c13  apps/backend/convex/lib/companionWorldTransitSchema.ts
1cfe5da07ea3df94b4e3ecdfb4b4ba5527cd0869a2102aac5f084e958925d524  apps/backend/convex/lib/companionRoadsideSchema.ts
0320c7393b6bd2ad85c9632a772264091f4cf8948477f07a183151d2587b92fc  apps/backend/convex/lib/companionTraffic.ts
b0ca0b62562c5f28d8d90d21567d6eb379864fa4453f581f5b14918307e6dcc4  apps/backend/convex/lib/companionNightclubModel.ts
3725118c3f18b0c273834a9fbccb015a92f9085adb13b29f962569070431bbb5  apps/backend/convex/lib/companionCityHousingModel.ts
eb0d6f4956917bf6cf6a136844408a882489d410a6830aec54e4ebe30c091a57  apps/backend/convex/lib/companionResidentVehiclesSchema.ts
d98eb366813cad032d1e2d241367578ca20d357a5169f6e244a68b1b388af2e6  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionMovementProtocol.h
ddacf3e6f68bb27946f28c03c88f867a171db120e1b608a1234470c16f0c4825  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionMovementProtocol.cpp
5fe18e8da08a35eaf00c75705ef277d55d8f1da46058f34a76d2f51b2355351d  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionMovementPresentation.h
424e219466c8a80259e508c0962a4f27fcb139939718149f22d3313461b6d85f  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionMovementPresentation.cpp
50fa1c634dd9eb6926ef0a836e82c0aa683fe452ed5e5480fbba5e0b8c22d5a4  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionWorldClock.h
cf0dfd35ed1d5cf95af887efb20352ba7319fda7ab0e20224d5f1fc845ef391a  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionWorldClock.cpp
cdb38c15f7d562972c73b422ca86993b93b069e2ae51bd62d20945b51b477005  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionSnapshot.cpp
```

SHA-256 hashes at the journey-data continuation review:

```text
39371d9ad12bc17ac16e132397b748d88cd956d2ede709118378ce3d1bc72140  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionWorldPlayer.h
72db223de0987369f7327ceda2573c5fa3a318497d439e930af42ae8b002669d  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionWorldPlayer.cpp
6c989a2b70ba19a412df7112f3fa35970cf56aeacaddc3c9b88ec3f61df71838  packages/utils/src/companionJourney.ts
e57c1742b37c9b71a1e01e2338f9aca049696b64667417007652330ab9950521  packages/utils/src/companionRoadside.ts
b1f6e06d26feb906cf5c5e1d5591311d868929f512a70e4bf4ed3c56781d3719  packages/utils/src/companionPolicing.ts
f18398357a2686cc1c44db09941d72fc804869d853f67d63e0c88c12db1f0805  packages/utils/src/companionPolicingDistricts.ts
0320c7393b6bd2ad85c9632a772264091f4cf8948477f07a183151d2587b92fc  apps/backend/convex/lib/companionTraffic.ts
1bf485bea06bd3ebcb49f67a93e28dc5d3f6ad433711196b01996d37e9a06863  apps/backend/convex/lib/companionPolicingModel.ts
008f54db546c7c644feb22d99903cce881c717a480b02b0db2dbc80199271005  apps/backend/convex/lib/companionWorldModel.ts
46777a5156888c6ebb1544365fdd517f1bebe9129d5d52252d02880e17af3def  apps/backend/convex/companionWorld.ts
f386fac6dcffcde55d1b8a88c3e9bed778a4be03ed2773dcd79e2ea6d0e6a61b  apps/backend/convex/companionPolicing.ts
4769cf025981f2bada8baf908a71de84a08943b2eb5fa45dad8bd6b09cb9c90f  apps/backend/convex/lib/companionPlayerVehiclesModel.ts
```

SHA-256 hashes at the current-instance geometry source review:

```text
90d230cf0233458c46324fd87d6f077c6f54bd7dd691030ba5a2a89e8607a17d  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionWorldPlayer.h
210d5cca522903cebb3aadcabf9206123cd067fead033bf3791b39c16833b416  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionWorldPlayer.cpp
4a29decf12e26f5d3acf091c141e50f6e537dfb7a0460d808cb0963e8c3dc551  packages/utils/src/companionNavigation.ts
279ec8785bc12ac9e455098d64fd0b8938a7d4a05da85d2ad0660394dd26f48a  packages/utils/src/companionStreetRouting.ts
05b2d4f854ff8e54a8233af45b87f159cd8e90efbd552cbe5a98029b7076e1ba  packages/utils/src/companionWorld.ts
e57c1742b37c9b71a1e01e2338f9aca049696b64667417007652330ab9950521  packages/utils/src/companionRoadside.ts
9915bbaf9fe79bb9d5acaace0baff37141b8a41a0e1bf3830ec64b7fcd5189c7  packages/utils/src/companionHomeDoor.ts
b27fe215e09903b61c9173db2ceb2d36e7d346de1d3f341aea1f169f23e14665  packages/utils/src/companionCity.ts
ef0483319d09e5262742d0767b6f77854dcc1d3791aff9e5e5f1eb917cb6c52c  packages/utils/src/companionCityData.ts
1e32ba830f60b492ac85eeec063e127ea3bb5ef197ff9c50dbea5d23297092a1  packages/utils/src/companionTownSquare.ts
7c65c2f30a11a6f8fff893a1bf29073d1c52219e3503b9d65455da59a3d01370  packages/utils/src/companionPlaces.ts
29c0bdbd4bb21a72801b99e56d23517d895f5307ede5fb52d5d1d8624e431541  packages/utils/src/companionPlacesLayouts.ts
801063ea4cf4fc1e3d66ea0af8d0d70b52e3d0225e9015ee9749986d693e952c  packages/utils/src/companionWaterfrontLayouts.ts
0ead5eb2164aa4ab89d226e479d60460a37812bb8c8962fb334bcd9cbdea26b3  packages/utils/src/companionRacing.ts
21be37b211cd5c5e02044a62c9c498704567da712c3df3f0023e621e88c031df  packages/assets/city/lagos-connected-transit-v1.json
77ffb38a43dc50c22421cb36dbb88e6634557aa809ddc5338c22d7f24ab607d4  packages/assets/city/lagos-street-density-navigation-v1.json
7f694b7c557aa1417ca7a6ffed902246f6f8689ec04b97fcfdc5acc18526516b  apps/backend/convex/lib/companionWorldJson.ts
```
