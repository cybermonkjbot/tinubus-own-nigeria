# Unreal resident population read handoff

October 10, 2026. Decoder source and self-review only. Independent acceptance, Windows delivery, compilation, content, native integration, in-game inspection and release remain separate and unfinished.

## Source and pins

Source commit: `4bf112760afd5ed106be685afc510997d0d8d24e`. It adds exactly the two files below. This handoff is the third assigned path. All paths are relative to the repository root.

| Source file | SHA-256 |
| --- | --- |
| `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionResidentState.h` | `affaccaa74d19c3bd78aa8de5057b163e2cf41c3db1ede616f3abf1829a2275c` |
| `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionResidentState.cpp` | `db9c57eee24382c40da67ff3cbfa9763f92f147c430c3c7da2da5a29880bca02` |

Assignment commit: `8e624a9ed5480441e5647247fa3c422a417e125b`, `docs/engineering/unreal-play-resident-read-assignments-2026-10-10.md`. Canonical base: `98de7532f3f983d4cf794d6282b0b9e2f33c1ee4`.

The implementation traces these pinned contracts, endpoint projections and client acceptance/lifecycle branches. It does not translate the referenced simulation or mutation implementations.

| Canonical file | SHA-256 |
| --- | --- |
| `packages/api/src/companion-world.ts` | `d985dbc931fec8d9c5e0b6a6e5a5aae5b6d2ffd568210e155c2ac9f0f0fa26b7` |
| `packages/utils/src/companionResidents.ts` | `bd3a8ee3cda5e1af3f7b87d7004b3204e2b060756564acfe2302e62f1a6885ba` |
| `packages/utils/src/companionResidentSimulation.ts` | `1bf871eb744c0ac6e3864ec374ef1c151e04168ae4afcf8b56ece8abf4d1c2e7` |
| `packages/utils/src/companionWorldClock.ts` | `75c8f91d25054af64a205e9cf65c3cb96c51891193946637f56aa4f3bd304dcb` |
| `packages/utils/src/companionJourney.ts` | `6c989a2b70ba19a412df7112f3fa35970cf56aeacaddc3c9b88ec3f61df71838` |
| `packages/utils/src/companionMovement.ts` | `0e4a96725023f7d1d6c33e18c7774dca9c7e560cfe293b827d486e05eac85809` |
| `packages/utils/src/companionVehicles.ts` | `464192ed6375a3c47c292109aedd109b579f18dd19abaaa16f61803def2168f2` |
| `packages/utils/src/companionPlaces.ts` | `7c65c2f30a11a6f8fff893a1bf29073d1c52219e3503b9d65455da59a3d01370` |
| `apps/backend/convex/companionWorld.ts` | `9f8ef66bf02c445ab688ea7410bfb6410b5c7259bc39ac7d692d0665e2ab9798` |
| `apps/backend/convex/lib/companionResidentsModel.ts` | `3305230fd179ccab68f79005bf4db8312104e8547fffaa1567ff650dc1494821` |

Two additional references at the same base resolve nested field definitions. `companionResidentVehiclesSchema.ts` supplies the exact transport/model/asset validators. The `CompanionNeeds` declaration in `companionLife.ts` supplies the four need names.

| Additional reference | SHA-256 |
| --- | --- |
| `apps/backend/convex/lib/companionResidentVehiclesSchema.ts` | `eb0d6f4956917bf6cf6a136844408a882489d410a6830aec54e4ebe30c091a57` |
| `packages/utils/src/companionLife.ts` | `b628e8415be3f064096b05e6878e1176f238635bd641f965582e68fa9e66ecf9` |

## Shared dependencies

The existing public `DecodeMovementRoute` is accessible and handles departure, arrival and local movement. No new movement sampler or shared decode operation was needed. StateJson supplies bounded UE JSON parsing, exact strings, scalar reads, nullable reads and enum matching. The dependency files were inspected; their working hashes match the consumed pins. All remain unchanged.

| Dependency under `apps/companion-unreal/Source/BaddieCompanion/` | Pin | SHA-256 |
| --- | --- | --- |
| `Public/BaddieCompanionMovementPresentation.h` | Canonical base | `5fe18e8da08a35eaf00c75705ef277d55d8f1da46058f34a76d2f51b2355351d` |
| `Private/BaddieCompanionMovementPresentation.cpp` | Canonical base | `424e219466c8a80259e508c0962a4f27fcb139939718149f22d3313461b6d85f` |
| `Private/BaddieCompanionStateJson.h` | `bc6e288621f97054c06368a9e7434835c7ce0ede`, unchanged at base | `fa20d7b05543229ec99caf20904cc68ab0a88b86f910efa777de6f791fc43040` |
| `Private/BaddieCompanionStateJson.cpp` | `bc6e288621f97054c06368a9e7434835c7ce0ede`, unchanged at base | `df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce` |

WorldPlayer's broader player-journey declarations were inspected for reuse. The resident corridor reader covers only the resident endpoint's corridor schema and adds no player-journey dependency or sampling. No dependency on the concurrent PlayState implementation exists.

The comparison references also match the canonical base: `Public/BaddieCompanionWorldPlayer.h`, SHA-256 `90d230cf0233458c46324fd87d6f077c6f54bd7dd691030ba5a2a89e8607a17d`, and `Private/BaddieCompanionWorldPlayer.cpp`, SHA-256 `210d5cca522903cebb3aadcabf9206123cd067fead033bf3791b39c16833b416`. Neither is a runtime dependency of this pair.

## Endpoint and translated fields

`companionWorld:residents` returns a serialized population string or null after its existing game access check. `DecodeResidentWorld` accepts the inner population JSON, or literal JSON `null` when the transport has received authoritative endpoint null. It does not unwrap a Convex envelope or a JSON-quoted endpoint string. The future transport must reject a missing envelope rather than manufacture a null result.

The decoder returns an immutable `TSharedPtr<const FResidentWorldView>`. Success with an empty pointer means known null. Failure clears output and returns only `Resident population data is invalid.` It bounds and copies input before clearing outputs, including input/error aliasing and input referring to a previous result. Parsed JSON objects never escape.

| Area | Preserved fields and branches |
| --- | --- |
| Population | `worldKey`, full clock, required-nullable `nextAdvanceAtRealMs`, ordered residents and optional ordered place bindings |
| Clock | Version 1, world/epoch keys, real epoch milliseconds, whole epoch game minute and the stored real-ms-per-game-minute scale. Older positive scales remain readable; no forced 15,000 default |
| Identity | NPC kind, exact ID/world/name, all eleven roster keys, roster version 1, all ten occupations, seven roles, two ordered traits from six choices, appearance seed and required-nullable asset NPC ID |
| Resident | Revision, clock epoch, updated game minute, both health fields, hunger/energy/cleanliness/enjoyment, worth, activity, journey, local move, service actor and optional render asset |
| Worth | Game-value unit, exact unit key, known banked/carried values, debt, required-nullable signed net worth and carried value, ordered unvalued asset IDs |
| At-place activity | Anchor, rest/work/eat/socialize/recover action, required-nullable interaction ID and since-game-minute |
| Traveling activity | Journey ID, source/destination places, walk/car/okada/keke/danfo mode, required-nullable vehicle ID and since-game-minute |
| Journey | ID, source/destination anchors, mode, vehicle ID, optional transport, server-ms basis, departure/arrival movements, corridor geometry, route start/end and arrival real times, rest/work/socialize arrival action |
| Transport | Vehicle and owner IDs, required-nullable rider ID, full model and required-nullable asset. Model retains catalogue/model IDs, catalogue revision, four vehicle kinds, tank capacity and metres per fuel unit |
| Local move | Existing movement route, destination anchor and rest/work/socialize action |
| Service actor/art | Required-nullable service actor containing place/NPC IDs; render asset's key, four supported asset NPC IDs and optional source yaw. No substitute art or model binding |
| Place binding | Place ID, resident instance ID, game instance ID and layout ID, without filtering to a viewer instance |

All required-nullable fields reject omission and wrong types. In addition to the fields above, identity `appearance.assetNpcId`, resident `journey`, `localMove` and `serviceActor` are required nullable. Transport asset `sourceYaw` is required when its asset exists; render asset `sourceYaw` is optional and rejects null.

`renderAsset` explicitly distinguishes missing, null and value. `placeBindings` distinguishes missing from an empty array and rejects null. Journey `transport` distinguishes missing from an object and rejects null; it is not fabricated for an older vehicle journey. The existing movement type retains omitted `initialSpeed` and represents omitted `pace` as `Unpaced`, distinct from walk/run/fast. Present null movement optionals fail through the shared decoder.

The endpoint projects worth, not raw holdings or saved home/work/social configuration. Those storage fields are not missing from the decoder's contract. Received worth is never recalculated. Null valuation totals stay unknown, negative net worth stays negative, and carried value does not become banked value. Names, IDs, asset keys and unit keys are retained without trimming, profile regeneration or substitution.

Place points remain X/Y/Z metres with Y up; movement remains X/Z metres and radians. Geographic corridor points remain latitude/longitude degrees. Journey movement and route times remain real server milliseconds, separate from game-minute fields. No unit conversion, clock sampling, route-length repair or local progression occurs.

## Validation and resource boundaries

The three explicit `connectResidents` checks are preserved: population world key equals clock world key, residents is an array of at most eleven entries, and next-advance time is either null or a nonnegative safe integer. The native decoder also validates the complete declared structure instead of relying on a TypeScript cast. Empty text is rejected, unlike the client's loose falsy-string fallback; the backend emits a serialized object or null, never an empty string.

Additional structural checks are separate from gameplay admission:

- Expected object keys, field types, enum spellings and supported versions must match. Movement objects retain the existing decoder's unknown-key policy. Health and needs need only be finite; dead/recovering residents are not discarded, and values are not clamped to display ranges.
- Revisions, appearance seed, game minutes and clock numbers are nonnegative safe integers; clock scale is positive. Worth uses safe integers, allowing signed net worth. Journey route start/end/arrival timestamps are finite nonnegative values through the safe-number maximum, without rounding.
- Anchors require exactly three finite coordinates and a finite yaw. Render/vehicle asset yaws are finite and unchanged. Corridor coordinates stay within geographic latitude/longitude ranges, and length is finite/nonnegative. The decoder does not require a positive length, two points, matched arrival endpoints or chronological/identity agreement across activity and journey. Those checks belong to the future resident projection/admission owner.
- Vehicle catalogue revision is positive. Tank capacity and metres per fuel unit are positive integers up to the canonical vehicle ceiling of 1,000,000. These are model storage limits, not fuel balances or invented vehicle defaults.
- Nested movement uses the existing public decoder unchanged, including version, sequence 1 through 1,000,000,000, safe room revision, finite coordinates/timing, bounded yaw, finite optional speed, pace choices, at most 32 path points and its derived-duration numeric guard. It accepts an empty stop/turn path. This slice does not add the resident render sampler's 90-second, speed, epoch, identity or cross-route checks.

Native allocation bounds are 1,048,576 input characters, depth 16 and 40,000 structural separators before UE deserialization. Keys have a nonempty 256-character bound; names have a 4,096-character bound. These bounds do not implement provisioning-key grammar or asset-registry validation.

Arrays retain their source order. Personality requires two entries. Unvalued assets allow at most 32, matching the holdings asset ceiling. Corridor geometry allows up to 256 points, matching the broader in-place projection bound; the pinned journey producer makes two to four points. Bindings allow up to 22, two per member of the eleven-key declared roster. The current producer iterates only the eight persistent residents and can emit at most sixteen distinct public routine-place bindings. The decoder preserves the client's eleven-member allowance rather than silently narrowing it to eight. Array emptiness, duplicate IDs and consistency across resident records do not establish render eligibility.

## Clearing and wakeup ownership still required

This is a stateless decoder, not the missing authenticated population owner. The future owner must translate the following existing client behavior without adding a revision high-water mark:

- `connectResidents` subscribes by the current player's game room, not current instance. A changed room stops the prior subscription, increments its generation, clears population/resident error and resets the population wake attempt. An instance change within the same game does not replace that subscription.
- Callbacks must match account generation, population subscription generation and room scope, and must be rejected after disposal. A valid response replaces the population wholesale, including null. The canonical subscription has no per-resident revision-retention rule.
- Ordinary read/parse failure clears population, records the sanitized resident error and reschedules the shared wakeup. Access failure calls the broader `failWorld`, which clears population and dependent world data and invalidates access/subscription generations. Successful reads clear the resident error using the existing action/ready/drop/resident error precedence, without overwriting an unrelated error.
- A nonnull accepted population whose world or epoch conflicts with current street/faction observations clears accepted street traffic, street scope/traffic, road, factions and hawkers. Null does not itself run those nonnull epoch-comparison branches. This decoder neither compares against prior observations nor clears those other owners.
- Disconnect, last-observer teardown, reload, account/session replacement, game selection change and disposal must retire the subscription and clear private population state as part of the enclosing world lifecycle. No local population is initialized on reconnection.

The current client has one population deadline in its existing world wakeup, gated by rendered-world observation. Its attempt key combines room scope and `nextAdvanceAtRealMs`, not a resident ID. A changed deadline resets that attempt. The due callback checks the enclosing generation, access generation and current deadline before the authoritative `advanceResidents` mutation. The reply also checks the captured population subscription generation and room scope before updating observed server time. An early returned server time can release the attempt for the same deadline. Ordinary wake failure records an error; access failure clears the world. None of this scheduling, mutation, clock observation or retry ownership is implemented here. No per-resident timers or synthetic epochs exist.

## Missing producers and verification

No shared-code blocker remains for this decoder. The admitted native population transport/envelope/error adapter, account/room subscription owner, world/epoch acceptance, shared wakeup integration and dependent street/faction invalidation remain missing. Structural validity cannot supply those producers.

Resident rendering still needs frozen-layout compatibility, explicit replacement of the existing service actor without duplication, current resident target resolution, verified art/model bindings, native navigation/animation and live world acceptance. The existing NPC dialogue target producer remains absent. A decoded binding or clock grants no Home/housing access, voice participation, movement, combat, mission, reward, edit or command authority.

Self-review covered every declared field and union, optional/null handling, nested movement reuse, numeric/resource limits, immutable publication, failure clearing and input/output alias safety. The complete owned source diff and whitespace checks were inspected. Dependency hashes matched. No tests, typechecks, lint, builds, compilation, codegen, conversion, imports, engine/server launches, installs, remote operations or live changes ran.

Earlier state pairs and handoffs, backend/mobile source, shared helpers, configuration, gates and production renderer inputs remain unchanged by this assignment. This unmounted decoder triggers neither renderer packaging nor What's New content. Parent independent Standards/Spec acceptance precedes the coordinator's Windows delivery. Source completion does not complete resident gameplay or establish rendered acceptance.
