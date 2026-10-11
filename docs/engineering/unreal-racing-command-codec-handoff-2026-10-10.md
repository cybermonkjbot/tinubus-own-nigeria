# Unreal racing command codec handoff

October 11, 2026. Source-only continuation of the October 10 assignment. Source commit: `e55e6fe4d51b9a01a7c633b9eeda2eec77b83ba0`.

## Scope and pins

Assignment: `docs/engineering/unreal-racing-command-codec-assignment-2026-10-10.md` at `0493042c78a2bd799499cfa867d49eb46d0686b0`. Assignment SHA-256: `b49217ba03ee0e3e38420371a97f9a2c1dd3f019a3f6e7c0a5164dd18ed23bf4`.

Canonical references, source preimages and dependencies are pinned to `f2ce043fbe686cfbbf246e6ea0ab0f4e35368d50`. All ten assignment hashes matched working files before editing. The new handoff was absent. Only these paths belong to this continuation:

- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionRacingState.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionRacingState.cpp`
- `docs/engineering/unreal-racing-command-codec-handoff-2026-10-10.md`

The source checkpoint contains only the native pair; this handoff is committed separately. Use the complete unchanged StateJson pair at `b4bc1b7d7dbc9552ad1451062a404fc872a13224`. Its working bytes have an empty diff against that commit. The original racing handoff's older backend/helper pins are historical, not this continuation's dependencies. Existing module JSON dependencies remain unchanged.

This patch adds pure codecs to the existing racing module. It does not create or mount a world state store. The original `ReadRacingSnapshot` API remains in place.

## Future consumer sequence

Keep the codec unmounted until the real native world command owner and permissions exist. That consumer must:

1. Capture the admitted account, present visit and command scope under the canonical lifecycle described below. Choose the command's sequence in that owner before encoding.
2. Call the matching `EncodeRacingCommand` overload. A true return supplies `FRacingCommandPacket` with `Endpoint`, `ArgumentsJson`, `bRetry = false` and `bInvalidateQueryCache = false`. A false return clears the packet and provides fixed public text. Encoding supplies no dispatch permission or receipt.
3. Perform the owner's pre-dispatch checks and send the captured packet through its authenticated transport. Credentials and the two adapter-policy flags are not JSON argument fields.
4. Unwrap transport data separately and call `DecodeRacingCommandResponse` on the endpoint object. Treat a false return as endpoint decoding failure. Do not pass HTTP status wrappers or Convex transport envelopes as that object.
5. Before using response data, accept the callback against the captured current visit. Only then observe `ServerNowMs`, and pass `RacingJson`, the accepted player's instance and the prior validated snapshot to `ReadRacingSnapshot`. Publish the reader's result through the world owner, including its cleared output for invalid inner data.

Neither codec performs the admission, dispatch, clock observation or publication steps.

## Typed packets

All five overloads accept `FRacingCommandScope`, with exact `GameRoomId`, `InstanceId` and `Revision`. All produce endpoint `companionRacing:command` and argument fields `gameRoomId`, `instanceId`, `revision` and `command`.

| Typed command | Nested action | Remaining nested fields |
| --- | --- | --- |
| `FRacingJoinCommand` | `join` | `vehicleId`, `vehicleRevision`, `transmission` |
| `FRacingStartCommand` | `start` | `raceId` |
| `FRacingLeaveCommand` | `leave` | `raceId` |
| `FRacingRecoverCommand` | `recover` | `raceId`, `sequence` |
| `FRacingInputCommand` | `input` | `raceId`, `sequence`, `input` containing `throttle`, `brake`, `steering`, `handbrake`, `gear` |

Separate command types prevent unrelated fields or an arbitrary action string from entering a packet. Join reuses `ERaceTransmission`, accepting only `Automatic` and `Manual`, encoded as `automatic` and `manual`. Invalid native enum values fail. Input reuses `FRaceInput`.

| Input | Accepted values and source of restriction |
| --- | --- |
| Race ID | Nonempty, at most 300 TCHARs, matching the canonical action-key bound |
| Join vehicle ID | Nonempty, at most 240 TCHARs; backend ceiling is stricter than the action predicate's 300 |
| Instance ID | Nonempty, at most 240 TCHARs; nonempty captured identity is native structural policy, ceiling is the backend's |
| Game room ID | Nonempty, at most 240 TCHARs; native resource policy, not validation of a Convex ID |
| World and vehicle revisions | Native integers from 0 through 9,007,199,254,740,991 |
| Recover/input sequence | Native integer from 1 through 1,000,000 |
| Throttle and brake | Finite numbers in 0..1 |
| Steering | Finite number in -1..1 |
| Handbrake | Native boolean |
| Input gear | Native integer in 1..6 |

Identifiers retain case, whitespace and control characters within the length limits. There is no trim, key regex or inferred ownership. Unreal JSON handles quoting and escaping. Numeric inputs are rejected outside their bounds, never clamped. Typed integer fields cannot represent fractional revisions, sequence or gear; upstream producers must avoid lossy conversion before calling the codec.

The encoder copies only supplied values. It does not increment, wrap, repair or acknowledge sequence. It does not consult saved-car ownership, health, fuel, eligibility, host status, readiness or race phase. Packet policy values are plain data for the future adapter, not admission flags.

A stop uses `FRacingInputCommand` with throttle 0, brake 1, steering 0, handbrake false and gear 1. It uses the same input overload and endpoint; there is no stop command variant. `FRaceInput`'s existing defaults have brake 0, so a default input is not a stop. Ordinary input is never silently replaced with neutral input.

Each encoder clears `Out` before validation and publishes a fresh packet only after successful serialization. Every failure returns `The race command couldn't be prepared.` Success clears `OutError`. No diagnostic includes identifiers, credentials or payload content.

## Endpoint response boundary

`DecodeRacingCommandResponse` accepts exactly the object fields `racingJson` and `serverNow`. Both are required. Unknown keys, missing fields, wrong primitive types, a non-object root or malformed outer JSON fail atomically with `The race response couldn't be read.` Failure resets `RacingJson` to an absent pointer and `ServerNowMs` to zero; the boolean return distinguishes failure from a valid zero-time response.

On success, `FRacingCommandResponse.RacingJson` is a shared UE JSON value holding either a raw string or explicit JSON null. `ServerNowMs` is a finite wire number. Negative, zero, fractional and finite numbers outside the safe-integer clock range remain accepted; no positive, integer or epoch restriction is added. The decoder does not apply that value to any clock.

Outer parsing uses shared StateJson limits of 262,144 TCHARs, depth 4 and 16 separators. Separators and nesting inside a quoted string do not count. A 32,768-character racing document can require 196,608 characters when every character uses a six-character JSON escape. The outer budget leaves room for the two field names, a normal finite number representation and formatting. Transport must still cap response bytes before allocating an FString.

There is no inner 32,768-character check in the endpoint decoder. A longer inner string can pass when the full outer response fits its resource budget. Malformed inner syntax and wrong-instance data also pass this boundary unchanged. An outer response exceeding its own budget fails independently of the inner document's meaning.

| Wire case | Endpoint outcome | Existing snapshot reader after visit acceptance |
| --- | --- | --- |
| Outer root `null` | Failure | Not called with a decoded response |
| `racingJson: null` | Success with explicit null value | `NullField`, cleared snapshot |
| `racingJson: "null"` | Success with raw string | `NullDocument` with a nonempty expected instance |
| Malformed or oversized inner string within the outer budget | Success with raw string | `InvalidDocument`, cleared snapshot |
| Valid inner snapshot for another race instance | Success with raw string | `InstanceMismatch`, cleared snapshot |
| Missing, numeric, object or array `racingJson` | Failure | Not called with a decoded response |

This separation preserves the canonical ordering of visit acceptance, server-time observation and inner parsing. Inner decode failure is not an endpoint-envelope failure, input acknowledgement or settlement result.

## Preserved snapshot reader

The body and signature of `ReadRacingSnapshot` are unchanged. The only receive-predicate change extracts pedal/steering/gear bounds into private `ValidInput` for reuse by input encoding. Received input still requires all five typed fields, rejects unknown fields, and reads gear through the existing safe-integer 1..6 predicate. Finiteness and inclusive numeric endpoints remain the same.

Incoming snapshot data is still fully validated and instance-checked before older-tick retention. A strictly older tick returns the complete prior snapshot for the expected instance, even if race IDs differ. Equal/newer ticks and an incoming null race replace it. Malformed inner data clears it rather than retaining an older-looking snapshot. Prior/output aliasing, null/absent distinctions, car bounds, duplicate-user rejection, optional recovery time and every other existing read rule remain unchanged. Full prior read semantics are recorded in `docs/engineering/unreal-racing-state-handoff-2026-10-10.md`, which was read and left untouched.

## Missing canonical world owner

The pinned `packages/api/src/companion-world.ts` remains the reference for `sendRace`, `stopRace` and callback ownership. Its lifecycle is not implemented by these codecs.

- Capture and admission use connection generation, access generation, room and `presentationKey`. The key includes user, character, game room, instance, world revision and current recovery life ID. `sameVisit` requires matching generations/room and a currently present player with that presentation key, both before send and before response use.
- Serialization waits behind `raceSending`. Input arriving during a send replaces the one queued latest input; non-input clears that queue. Input/recover sequence is the maximum of local next, caller-supplied sequence and current car sequence plus one. The codec consumes that final choice without changing it.
- Response handling first accepts the visit and observes `serverNow`. Input success preserves the existing world error. Non-input success clears `actionError` and composes the remaining error as `readyError ?? dropsError ?? residentsError`. Both publish the existing reader's result, including null for invalid inner data.
- Failure handling ignores stale visits. Current failures are sanitized; access failures go through `failWorld`, while other failures set `actionError` and the world error before rethrowing. Finalization clears `raceSending` only for its own pending promise. It releases queued input only for the same visit, captured scope and queued race ID matching the current race.
- Stop requires a current race and the current user's car in `racing` status, clears queued input and sends the neutral input with car sequence plus one through `sendRace`. The local readiness bypass recognizes throttle 0/brake 1; it does not bypass present-visit acceptance, access generations or backend permissions.

World query callbacks separately use connection/query generations, current-player revision/move ordering, instance resolution and control admission. Their accepted same-visit state requires matching resolved instance/layout/room/roadside context and player user/character/game room/instance/place/status/journey identity. Racing is retained only for a present same-visit player. `failWorld` retires access and clears racing; recovery-life changes clear racing; disconnect advances the connection generation and resets world state. Room/reconnect/reload, last-observer teardown and disposal use that lifecycle. Account/session replacement must discard the old account owner and prior racing state.

Missing integration includes that native world owner, coherent current-player/recovery/saved-car producers, authenticated transport, admission and callback retirement, input queue/stop handling, clock/error composition, and the native command/presentation consumer. No substitute store, fake receipt, permissive readiness switch or automatic timer is provided.

Backend race seats, vehicle/fuel ownership and settlement remain authoritative. Local vehicle physics, track generation, prediction, collision, laps/rewards and recovery placement are outside this stage. Later presentation should use native Unreal systems under the proper world owner.

## Inspection and status

Source/diff inspection covered these boundaries:

- Packet construction covered all five nested shapes, both transmissions, invalid enum values, exact strings/JSON escaping and the neutral input tuple. Empty/overlong IDs, safe-integer revision endpoints, sequence endpoints and invalid/nonfinite input ranges were traced through their failure paths.
- Endpoint decoding covered required fields, unknown keys, wrong types, outer null, explicit inner null, string null, malformed/oversized inner strings, wrong-instance data and atomic output clearing. The outer budget was checked against six-character escaping of the full existing inner budget.
- Receive compatibility covered shared input-predicate equivalence, the untouched `ReadRacingSnapshot` body and its older/equal-tick behavior. No queue, admission, clock, simulation or publication logic entered either codec.

These are manual inspections, not executed tests. No tests, typechecks, lint, builds, compilation, codegen, assets/imports/conversion, launches, installs, remote operations or live configuration ran. Checkpoints use explicit owned paths with hooks disabled. Prior Economy/Jobs/Mission/Play work, Football, the original Racing handoff, StateJson, tracker and unrelated staging/edits remain outside this change.

Production renderer/mobile inputs, disabled account/readiness gates and the world-control interlock are unchanged. This unmounted native codec triggers neither renderer packaging nor mobile What's New changes. Parent owns independent Standards/Spec review and acceptance; the coordinator alone owns Windows delivery. Compilation, in-game inspection and release remain incomplete.

## SHA-256 evidence

Canonical inputs at `f2ce043fbe686cfbbf246e6ea0ab0f4e35368d50`:

```text
d985dbc931fec8d9c5e0b6a6e5a5aae5b6d2ffd568210e155c2ac9f0f0fa26b7  packages/api/src/companion-world.ts
0ead5eb2164aa4ab89d226e479d60460a37812bb8c8962fb334bcd9cbdea26b3  packages/utils/src/companionRacing.ts
652b3c3f20c6a1721dd4c65940c7144fb81530f61f20e038fe596e1c92515474  apps/backend/convex/companionRacing.ts
f9b6acc4cc38d5c0ee7e000745dfcfe1d2dff556d31768524fa75cf256baf65f  apps/backend/convex/lib/companionRacingModel.ts
45be4baf91578930669721d829a475e6202c19b03ba4996537640ebe754898ed  docs/engineering/unreal-racing-state-handoff-2026-10-10.md
```

Source preimages and unchanged dependencies under `apps/companion-unreal/Source/BaddieCompanion`:

```text
493db64cf9ee02f713ae82d95464a16dd168e0aaa636c42bb1b7e9104922b4e5  Public/BaddieCompanionRacingState.h
dd54bbc605c97f037e9cb36c6ea30713c5103bc806088c4a3700cece48bb5001  Private/BaddieCompanionRacingState.cpp
13ceb1b13bee4b0793ddb117e03559bfb8ea77f8bdb8dc81baa1ffa8e70bb618  Private/BaddieCompanionStateJson.h
df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce  Private/BaddieCompanionStateJson.cpp
536bf39a4089acea43be36448ad3d02d3cd7f94a65015d3975f761941b802328  BaddieCompanion.Build.cs
```

Source outputs at `e55e6fe4d51b9a01a7c633b9eeda2eec77b83ba0` under the same root:

```text
b833429fc84d23f0c8de3300500e9d5fbe195e67b94732e477d6cb3543bad1b8  Public/BaddieCompanionRacingState.h
378d62c713365178835ee67c86fcbc631ec72d02735b2dcf6150c8689c4e1d4b  Private/BaddieCompanionRacingState.cpp
```
