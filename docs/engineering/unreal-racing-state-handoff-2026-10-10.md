# Unreal racing snapshot handoff

October 10, 2026. Bounded decoding and received-state retention only. Source commit: `7bce7763ee33e2b58f3b55c0f2a06cc96516a92a`. Independent acceptance, Windows delivery, compilation, in-game verification and release remain separate.

Owned paths, relative to the repository root:

- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionRacingState.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionRacingState.cpp`
- `docs/engineering/unreal-racing-state-handoff-2026-10-10.md`

All destinations were absent before implementation. All prior source and handoffs remain read-only. The parent owns acceptance and the tracker; the coordinator owns Windows transfers. No native race query, world owner or renderer is mounted by this patch.

## Canonical dependencies

All six files matched canonical pin `dbac2133fa25f111c6ed0745ba632e551c2a63c5` before implementation and again before the source checkpoint.

| File | SHA-256 |
| --- | --- |
| `packages/utils/src/companionRacing.ts` | `0ead5eb2164aa4ab89d226e479d60460a37812bb8c8962fb334bcd9cbdea26b3` |
| `packages/utils/src/companionPlayerDriving.ts` | `2e268765a20956c92c09f7677af35238407629225396456a594046166a9f31f6` |
| `packages/api/src/companion-world.ts` | `d985dbc931fec8d9c5e0b6a6e5a5aae5b6d2ffd568210e155c2ac9f0f0fa26b7` |
| `apps/backend/convex/companionRacing.ts` | `54f56d47ecccb6f93a28b1790f17eb079274933f6db4414251d3eaa9153d7228` |
| `apps/backend/convex/lib/companionRacingModel.ts` | `f9b6acc4cc38d5c0ee7e000745dfcfe1d2dff556d31768524fa75cf256baf65f` |
| `apps/backend/convex/companionWorld.ts` | `9f8ef66bf02c445ab688ea7410bfb6410b5c7259bc39ac7d692d0665e2ab9798` |

The shared StateJson dependency is pinned to `bc6e288621f97054c06368a9e7434835c7ce0ede` and unchanged at the canonical pin. Paths in the next two tables are relative to `apps/companion-unreal/Source/BaddieCompanion/`.

| Shared dependency file | SHA-256 |
| --- | --- |
| `Private/BaddieCompanionStateJson.h` | `fa20d7b05543229ec99caf20904cc68ab0a88b86f910efa777de6f791fc43040` |
| `Private/BaddieCompanionStateJson.cpp` | `df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce` |

Source outputs are pinned to `7bce7763ee33e2b58f3b55c0f2a06cc96516a92a`.

| Source-output file | SHA-256 |
| --- | --- |
| `Public/BaddieCompanionRacingState.h` | `493db64cf9ee02f713ae82d95464a16dd168e0aaa636c42bb1b7e9104922b4e5` |
| `Private/BaddieCompanionRacingState.cpp` | `dd54bbc605c97f037e9cb36c6ea30713c5103bc806088c4a3700cece48bb5001` |

The decoder uses UE JSON and shared `Parse`, `ParseNull`, `Only`, `Object`, `Field`, `String`, finite `Number`, `Integer`, `Boolean`, `Nullable`, `Same` and `EnumValue`. Signed integers reuse `Integer` with the explicit negative safe-integer minimum. No shared helper change or duplicate JSON scanner/enum loop is needed.

## Decoder contract

`ReadRacingSnapshot` consumes the `racingJson` field from an already-bounded world or command envelope as a UE JSON value. It also takes the accepted player's expected instance and an optional prior validated snapshot. The prior value is a caller-owned result from this decoder, not an admission receipt. Inputs are passed by value so the prior shared pointer may also be the output variable without losing retention data.

| Outcome | Condition | Output |
| --- | --- | --- |
| `Absent` | Field pointer absent | Null, no diagnostic |
| `NullField` | Outer field is JSON null | Null, no diagnostic |
| `NullDocument` | String contains syntax-valid JSON null, with an expected instance | Null, no diagnostic |
| `InvalidField` | Present outer field is neither string nor null | Null, fixed diagnostic |
| `InvalidInstance` | Expected instance is empty, after field type/length checks | Null, fixed diagnostic |
| `InvalidDocument` | Oversized, malformed, structurally invalid or out-of-bounds document | Null, fixed diagnostic |
| `InstanceMismatch` | Valid incoming race belongs to another instance | Null, fixed diagnostic |
| `Accepted` | Valid incoming snapshot without older-tick retention | Complete new snapshot |
| `RetainedOlderTick` | Valid incoming race is older than the prior race in the expected instance | Complete prior snapshot |

Every null output corresponds to the canonical cleared racing value. Diagnostics are fixed public text and contain no payload, IDs or credentials. The decoder does not publish errors into world state. A valid snapshot with `race: null` is still a nonnull snapshot, preserving its selected car and eligibility.

The canonical input limit is 32,768 characters inclusive. Native parsing additionally limits depth to 16 and separators to 2,048 before UE deserialization. The typed tree fits those limits. Cars are limited to eight, retain source order and require case-sensitive unique user IDs. Empty car arrays are valid. Unknown keys and wrong primitive types are rejected atomically through a fresh candidate.

## Complete fields and receive predicates

| Object/fields | Typed read behavior |
| --- | --- |
| Snapshot `race`, `car`, `eligibility` | Race and selected car are required nullable objects. Eligibility is exactly `eligible`, `no-car`, `car-away`, `car-busy`, `no-fuel` or `damaged`. |
| Selected car `vehicleId`, `revision`, `fuelUnits`, `condition`, `here` | Vehicle ID string at most 240 characters; all three numbers finite; `here` boolean. No extra nonnegative or integer rules. |
| Race `id`, `instanceId`, `hostId`, `phase` | ID string at most 300 characters; instance exactly matches the nonempty expected instance; host is a string. Phase is `lobby`, `countdown`, `racing`, `finished` or `cancelled`. |
| Race `startsAt`, `endsAt`, `updatedAt`, `cars` | Start/end required nullable signed safe integers; updated time a signed safe integer. Ordered car array as bounded above. |
| Car `userId`, `name`, `vehicleId`, `transmission` | Strings, with name at most 60 characters. Transmission is `automatic` or `manual`. |
| Car `x`, `z`, `yaw`, `speed` | Finite numbers; x in `[-155,155]`, z in `[-95,95]`, speed in `[0,36]`. Yaw is not normalized. |
| Car `gear`, `sequence`, `inputAt`, `simulatedAt`, `distance`, `progress`, `lapStartedAt` | Required finite numbers. No command sequence ceiling, positivity rule, integer conversion or inferred progress. |
| Car `nextCheckpoint`, `laps` | Safe integers in `0..7` and `0..3` respectively. Stored as bounded native integers. |
| Car `recoveredAt` | Optional signed safe integer. Missing remains absent; null is invalid. |
| Car `lastLapMs`, `bestLapMs`, `finishedAt` | Required nullable finite numbers, not restricted to integers or nonnegative values. |
| Car `status` | Exactly `ready`, `racing`, `finished` or `withdrawn`. |
| Received `input` | Required throttle/brake in `[0,1]`, steering in `[-1,1]`, boolean handbrake and safe-integer gear in `1..6`, matching `validRaceInput`. |

Signed safe integers cover `-9007199254740991..9007199254740991`. No extra time relationships, host membership, phase/status consistency, selected-car eligibility recomputation or vehicle-ownership checks are invented. Strings retain case and whitespace; the reader's string predicates allow empty strings except the required expected instance. Uncapped identity strings remain bounded by the complete input length.

Coordinates, distance and progress remain in metres, speed in metres per second, yaw in radians and times in milliseconds. No Unreal-axis or centimetre conversion occurs. `CompanionDrivingInput` supplies the three received pedal/steering fields; no driving authority or vehicle mechanics are translated.

The native contract combines the canonical receive predicates with complete typed server data. JavaScript's reader uses truthiness for `race` and `car`, so some missing or false-valued fields can evade its checks. Native decoding requires the declared nullable objects. JavaScript also omits the car's top-level `gear` from its finite-number list; native decoding requires the declared numeric field and finiteness, without applying the input gear's `1..6` rule to it. Unknown fields are rejected rather than silently retained. These are structural boundary differences, not new racing or command rules.

## Exact older-tick retention

The incoming snapshot is completely decoded, bounded and instance-checked before examining prior tick ordering. Malformed input, duplicate users, invalid selected-car data or invalid eligibility clears output even when an older-looking race could otherwise retain previous data.

Retention requires an incoming race, a prior race whose instance equals the expected instance, and prior `updatedAt` strictly greater than incoming `updatedAt`. Race IDs are not compared. The entire prior snapshot is returned, including its selected car and eligibility, without merging incoming fields. Equal or newer ticks replace it. A valid incoming null race replaces it too. A prior race in another instance, or a prior snapshot without a race, cannot trigger retention. There is no permanent high-water mark or separate car/eligibility revision ordering.

## Existing caller ownership

The relevant paths are in pinned `packages/api/src/companion-world.ts`.

World reception checks disposed state, the store connection generation and query generation. It rejects older current-player revision/move-sequence values, resolves the supplied instance and calls `accept(value.player, value.peers, undefined, false)` before reading `racingJson` with the resulting `state.player?.instanceId` and retained `state.racing`.

Inside `accept`, existing world/control admission determines the accepted player. Its actual same-visit predicate requires matching resolved instance ID, layout ID, room and roadside data, plus matching player user, character, game room, instance, place, status and journey ID. Status must not be traveling. Prior racing survives that acceptance only for this same visit with a present player. World revision is not itself part of this same-visit predicate, although changed revision separately resets rendered state. These checks remain with the world owner; this decoder accepts no `ready` or `sameVisit` switches.

Race commands capture the store generation, access generation, room and `presentationKey`. That key includes user, character, game room, instance, world revision and current recovery life ID. A reply is consumed only while those generations/room still match and the current player is present with the same presentation key. Only then does the command owner observe `serverNow` and pass the returned `racingJson` to the reader. Command sequencing, serialized sending, queued latest input, stop behavior and error composition remain outside this decoder. An input command can return before the server tick consumes its input; no local sequence acknowledgement or motion is inferred from the returned snapshot.

The account store owns clearing. `failWorld` clears racing and increments access generation; a changed recovery life clears racing; `disconnect` resets world state and advances the connection generation. Room selection, reconnect/reload, last-listener teardown and disposal use that clearing lifecycle. The store closes over one authentication context, so account/session replacement must dispose or replace the owning store and discard its prior racing snapshot, even with identical instance IDs. Ordinary race-command errors retain current data unless they trigger `failWorld`; malformed decoded replies clear racing. The native integration owner must preserve these differences.

## Producers and limits of this slice

Backend `readWorldRacing` returns null unless the player is present at `race-circuit`. It reads the instance's saved race, resolves the account's single owned car and produces selected-car data and eligibility. That function, not the native decoder, owns the saved-car selection and eligibility policy. `readCompanionWorldForIdentity` serializes it into optional `racingJson`; early no-player results omit the field. The race command returns its serialized projection and a separate `serverNow`. A serialized null and an omitted field therefore both occur in the existing world contract.

An authenticated native world/command envelope consumer, accepted current-player/visit owner, account/life clearing integration and presentation consumer are still missing from this slice. Backend admission, race seats, fuel reservation/settlement and movement interlocks remain authoritative. Instance equality, `here`, `eligible`, race status and host ID confer no movement, command, fuel, ownership or reward authority.

There is no local simulation, prediction, collision, track generation, recovery placement, ranking execution, input queue, stop/recover pump, command, retry, clock, award, persistence or rendering implementation. Later native vehicle/movement presentation must reuse Unreal facilities under separate authority. Production renderer inputs and all gates are unchanged; no mobile behavior changed, so no renderer packaging or What's New edit is triggered.

## Source inspection

Source review covered every field and enum, all numeric/string/array bounds, required nullability, optional recovery time, duplicate-user rejection and output clearing. Retention walkthroughs included different race IDs in the same instance, equal ticks, prior wrong instance, incoming null race, malformed older snapshots, changed selected-car eligibility and prior/output aliasing. The complete owned source diff was reviewed before checkpointing; this handoff's diff was reviewed separately. These were source inspections, not executed tests.

No tests, typechecks, lint, builds, compilation, code generation, imports, assets, launches, installs, remote actions or live configuration ran. Exact-path commits used disabled hooks and preserved unrelated staging/edits. Compiled compatibility and runtime behavior are unverified. Independent Standards and Spec acceptance must precede the coordinator's later Windows delivery; source completion alone does not complete racing gameplay.
