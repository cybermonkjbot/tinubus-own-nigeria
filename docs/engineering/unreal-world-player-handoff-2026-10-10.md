# Native world-player decoder handoff

October 10, 2026. Source implementation and source review only. This decoder is unmounted and grants no gameplay authority.

## Owned files

- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionWorldPlayer.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionWorldPlayer.cpp`
- `docs/engineering/unreal-world-player-handoff-2026-10-10.md`

`DecodeWorldPlayerDocument` consumes `PlayerDocumentJson` and `ServerNowMs` from the same existing `FWorldMovementResult`. It reparses the bounded document and calls `DecodeWorldMovementResult` for core player, movement, driving and water fields. There is no duplicate driving/water parser or simulation. It publishes `TSharedPtr<const FWorldPlayerView>` containing typed data, with no raw JSON, account session or credential accessor.

`DecodeWorldCurrentInstanceIdentity` consumes the existing snapshot's `instanceJson` and that decoded player. It publishes a const identity projection, not a navigable or rendered instance. It distinguishes the occupied visit from a traveling player's destination. Neither function creates a transport, ticker, owner, actor, token or backend API.

The native GameplaySession checkpoint `8f6c3322a` remains a separate, unmounted entry/presence owner. Backend checkpoint `c0a68c95f` secures entry and presence, not world controls. Nothing in this patch changes that boundary or enables a gate.

## Decoded contract and source review

The table records branch-by-branch source inspection, not executed tests. Typed optional fields carry `Missing`, `Null` or `Value` alongside their optional value. Only combinations allowed by each field's wire contract succeed. Omitted fields never acquire invented values.

| Input | Decoding and retained distinction |
| --- | --- |
| Player identity | Required user, character, game room, place, instance and Home room IDs. Comparisons are explicitly case-sensitive. Unknown top-level fields, including storage-only `sessionId` and `readyInstanceId`, reject the player. |
| Revision, sequence and movement | Reuse the existing safe-integer revision, 0 through 1,000,000,000 sequence, required null/object movement and route version-one checks. Route sequence and room revision must match the player. Zero sequence plus null movement remains valid. No comparison with an issued request or prior receipt occurs here. |
| Player status and health | Preserve present, traveling and recovering, plus finite health without clamping. Recovering and health at or below zero are separate observations. A traveling player requires a traveling journey. An interrupted journey can remain on a recovering player after death. |
| Journey | Required null/object. Decode ID, request ID, source/destination IDs, traveling/arrived/interrupted status and car/okada/keke/danfo/canoe/speedboat vehicle. All source times remain unchanged. |
| Journey route | Accept `lagos-corridors-v1` with `proposed`, or `mapbox-directions-v5` with `road-network`. Validate 2 through 4096 geographic points and nonnegative length. Road durations must contain exactly one nonnegative finite time per segment. Publish only route identity, kind, point count and length, not a sampleable route. Zero-length retained exits remain valid. |
| Road profile | Missing, `driving-traffic` or `walking`. Null and unknown values fail. The backend's `companionTrafficRoute` declares this field even though the shared base route type omits it. Corridor routes do not accept profile or segment-duration fields. |
| Travel endpoints | Optional, not nullable. Place endpoints retain ID and geographic point. Road endpoints also require paired null/null or value/value vehicle ID/revision. Road ID limit is 85 characters, place ID 128 and endpoint vehicle ID 160. Endpoint-derived place IDs must match the corresponding journey place IDs. |
| Travel mode | Missing, walk, vehicle or boat. No default is inferred from the vehicle field. |
| Journey checkpoint | Required null/object. Retain progress, startsAt and endsAt. `resumedAt` missing preserves legacy timing; explicit null remains a wait; a finite nonnegative value remains the supplied receipt time. No progress, arrival or release is advanced locally. |
| Travel timing | Optional, not nullable. Retain rate 1 or 4, anchor time/progress and positive duration. No acceleration or arrival calculation runs. |
| Traffic and checkpoint enforcement | Optional object envelopes only. Null/scalar/array forms fail. Their contents are deliberately not typed, interpreted or exposed by this slice. Exact remaining fields are listed below. |
| Roadside visit | Missing/null/object. Retain the road endpoint, sourceJourneyId, stoppedAt, exit place and the validated exit-route identity. A previous roadside record may remain during subsequent travel; destination selection uses the journey's endpoint instead. |
| Transit ride | Missing/null/object. Retain ID, vehicle, safe nonnegative seat, boardedAt and visitRevision. Present-player status and matching visit revision are required. Seat capacity and vehicle availability require the transit owner. |
| Transit exit | Required null/object. A supplied exit retains at, finite X/Z/yaw and required null/string stopId. An exit request does not remove occupancy or imply alighting. |
| Nightclub lounge | Missing/null/object at player level. Lounge retains activity ID, visit revision, service ID, joinedAt and optional non-null seatId. Seat occupancy and an existing server-owned route remain visible together. |
| Nightclub dance | Decode round ID/instance/service, startsAt, beat/window duration, required hits and up to 24 lanes. Decode up to 24 received input records without sorting, merging or scoring. Required stoppedAt preserves null versus supplied time. Difficulty remains missing/easy/standard/hard; skillCharacterId and skillsSettled remain optional and non-null. |
| Activity consistency | Supplied activities require the nightclub place, present status and current visit revision. Dance round ID must equal activity ID, and its instance must equal the player's instance. Beat/window/lane/hit guards follow the existing dance bounds. `bDanceActivity` means a retained dance record, not that the round is currently playing. |
| Residence and weapon | Residence preserves missing/null/object, city version, parcel ID and positive integer unit. Weapon preserves missing/null/bat/pistol. Omitted weapons are not retained from an older player by this decoder. |
| Driving and water | Existing parsers retain missing/null/object and all supported control fields. Active water is distinct from landed water. The original storage-only water-field rejection remains in force. No controls are erased to manufacture on-foot state. |
| Conflicting or different-owner transport | Publish conflict and identity-mismatch observations while retaining the controls. Compare player user/character/game/instance, water place/revision and driver/occupant life consistency. Current life is not in this public player view. No mismatch is accepted for gameplay merely because decoding succeeded. |
| Current instance | Decode ID, place, seed, generator version, layout ID, navigation revision, sharedStartedAt and optional roadside identity. Layout and room are required null/object envelopes; optional transitPlan is object-only. These envelopes do not establish complete schemas or supported content. |
| Current visit versus destination | Non-traveling instance IDs must match the player. Traveling instances match journey.toPlaceId and remain labeled TravelDestination. Non-Home IDs match gameRoomId plus placeId. Own Home includes the player user ID and supplied room ID; a current Home also matches homeRoomId. No library-selected destination Home is synthesized. |
| Roadside instance | Match the selected journey/visit endpoint exactly, including coordinates and nullable vehicle pair. Require roadside-v1 generator/layout/navigation, endpoint seed, zero sharedStartedAt and null room/layout. Geometry is still unvalidated. |
| Transport layout | For a current visit, expose mismatched driving navigation revision or water layout ID/revision. This does not establish domain, content, vehicle or life admission. Destination instances do not validate source transport. |
| Failure | Both public entry points reset the caller's output before parsing and publish only after all scoped reads succeed. Every failure returns fixed text, never payload values. A failed decode cannot leave an earlier view in that output variable. |

Both parsers cap input at 2,097,152 TCHAR code units, nesting at 32, and commas/colons outside strings at 131,072 before Unreal creates the JSON tree. The scanner accounts for quotes and escapes; Unreal still validates JSON syntax and the root object. These are parser limits, not a transport receive-memory cap. Ordinary IDs are capped at 240 characters; layout/navigation identities allow 160,000 because saved-room navigation revisions include serialized furniture. New numeric reads require finite values. Counters are safe nonnegative integers; journey/activity times retain fractional source values within the nonnegative safe-number range.

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

This is the smallest useful player/control-identity slice, not a complete world snapshot adapter.

- Journey geometry `route.points` and `route.segmentDurationsMs`, including the roadside exit equivalents, is validated but not published. A future journey-presentation adapter must retain it before sampling. This slice does not validate geometric route length, endpoint proximity or road timing totals.
- `journey.traffic` remains envelope-only. Its remaining fields are provider, fetchedAtRealMs, durationMs, typicalDurationMs, coverage and congestion.
- `journey.checkpoint.enforcement` remains envelope-only. Its remaining fields are version, agency, stopKind, district and isNight. No enforcement version or decision is accepted by this view.
- Current-instance geometry remains outside this decoder: navigation bounds, obstacles and their yaw/polygons, radius, floorY, gridSize, streetGraph points/edges, spawns and exit. The full layout, saved room, and hydrated transitPlan contents also remain separate. GeneratorVersion is retained as an opaque identity, never approved for rendering.
- Full snapshot fields outside player/instance identity still need their own adapters, including racingJson, transitJson, transitLifeId, fare receipts, checkpointStatus, street traffic, driving catalog, road/faction/hawker/water domains, resident world and peers. The public player has no top-level current life ID, server readiness or complete other-owner discriminator.
- The const data views need an admitted native owner and transport, stale-response handling, session clearing, previous-receipt reconciliation, clock integration, readiness and pawn/camera binding. None is mounted here.

## Evidence and delivery status

Independent source review of `6d61d437b72443aa7d8f325a73ebb915fb5a0abd` against parent `6a911f1d21f19d5d6ff8d422ca2090e2e0eb2ede` found no actionable defect in the scoped identity, ordering, nullability, recovery/activity, control-observation, parser-clearing or destination/current-instance behavior. The reviewer used commit-pinned canonical sources and dependencies. Geometry publication and downstream admission remain unfinished as listed above. No executable validation ran.

Source review traced the canonical `worldPlayerView` validator and `companionWorldPlayerView` projection, snapshot destination selection, move declines and other-owner stops, death-to-hospital interruption, saved Home doors, roadside arrival/exit, and the client accept/control-admission paths. The backend profile alternative and source-versus-destination distinction are included above. No additional actionable source defect remains in the implemented projection after this review. Envelope-only domains and owner acceptance are explicitly unfinished, not validated by omission.

No tests, typechecks, lint, builds, compilation, code generation, asset import/conversion, installs, engine/server starts, live calls, deployments or configuration changes ran. The production renderer, asset registry and bridge contracts are unchanged, so no renderer packaging was triggered. No Windows transfer was performed. Coordinator `01a12172-6903-7ff1-8244-8d386ff688a7` retains sole handoff ownership. Content integration, compilation, runtime lifecycle checks, rendered/gameplay acceptance and release remain pending separately from this source checkpoint.

SHA-256 hashes at source review:

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
