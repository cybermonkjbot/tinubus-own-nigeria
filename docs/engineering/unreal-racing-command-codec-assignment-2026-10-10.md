# Racing command codec

October 10, 2026. Source-only packet and endpoint-response continuation. World command ownership remains a separate dependency.

## Ownership and pins

Existing chat `01a12714-f0c9-7153-8f81-94e2816fca6e`, host local, owns only RacingState.h/.cpp and new `docs/engineering/unreal-racing-command-codec-handoff-2026-10-10.md`. Extend the current racing module; retain the existing ReadRacingSnapshot API and every read/retention behavior. The original racing handoff, all prior Economy/Jobs/Mission/Play work and Football remain read-only. Parent owns assignments, shared StateJson, acceptance and tracker. Coordinator alone owns Windows writes.

Canonical references, dependencies and source preimages are pinned to `f2ce043fbe686cfbbf246e6ea0ab0f4e35368d50`. Parent matched all ten working files below. Recheck them and confirm the new handoff absent before editing. Read the complete current native pair and original handoff, RaceAction/validRaceAction/validRaceInput, the canonical sendRace/stopRace and related world callback ownership, and the backend command validators/returns. Keep complete shared StateJson at `b4bc1b7d7dbc9552ad1451062a404fc872a13224` unchanged.

## Five packet shapes

Provide typed, pure encoding through Unreal JSON. Each packet names `companionRacing:command` and serializes gameRoomId, instanceId, revision and the nested command below. Capture exact supplied values without credentials. Retain retry:false and invalidateQueryCache:false as packet/adapter policy. Successful serialization grants no dispatch, visit, fuel or vehicle authority.

| action | Nested fields besides action |
| --- | --- |
| join | vehicleId, vehicleRevision, transmission |
| start | raceId |
| leave | raceId |
| recover | raceId, sequence |
| input | raceId, sequence, input with throttle, brake, steering, handbrake, gear |

Use current FRaceInput and ERaceTransmission. Reject invalid native enum values. Preserve case, whitespace and JSON escaping; never normalize identifiers. Revision and vehicleRevision are nonnegative safe integers. Recover/input sequence is an integer in 1..1000000. Throttle/brake are finite in 0..1, steering finite in -1..1, handbrake boolean, gear an integer in 1..6. Reuse these numeric predicates between received and encoded input when practical without changing the existing decoder's accepted values.

Use the canonical action-key bound of 300 characters and the backend's stricter 240-character vehicleId ceiling for join. The backend also caps instanceId at 240. Require nonempty captured gameRoomId and instanceId; bound gameRoomId explicitly as native resource policy rather than pretending to validate a Convex ID. Document this structural policy. Do not add host, eligibility, saved-car ownership, ready, health or race-phase checks. Those require missing authoritative producers.

This encoder accepts a sequence already chosen by its future world owner. It must not increment, wrap, clamp, repair or acknowledge sequence. Stop is the existing input action with throttle 0, brake 1, steering 0, handbrake false and gear 1; it is not a new endpoint or command variant. Do not silently replace ordinary input with neutral input.

## Endpoint response

Add a pure bounded decoder for the exact command endpoint object, not a transport envelope. It requires racingJson as a string or explicit null and serverNow as a finite wire number. Preserve the current response validator's numeric breadth rather than adding positive/integer clock predicates. Missing fields, wrong types, unknown keys and malformed outer JSON fail atomically with fixed public-safe text and cleared output. Document the outer character/depth/separator bounds. They must permit the existing 32768-character racing document after JSON escaping.

Keep the endpoint envelope separate from ReadRacingSnapshot. The decoded racingJson remains raw string/null data suitable for the existing reader after real callback/visit acceptance. A syntactically malformed inner racing string, an oversized inner document, or one for another instance must not be mislabeled as an outer-envelope failure merely to simplify integration. The canonical owner first accepts the visit, observes serverNow, then calls readRacingSnapshot, which clears invalid inner racing data. Do not apply a clock, publish racing state or infer success of input/settlement in this pure decoder. The original reader remains the sole typed racing/older-tick implementation.

## Missing world owner

Record the remaining sendRace/stopRace lifecycle accurately in the new handoff. It captures connection/access generations, room and presentation identity including user, character, game room, instance, revision and recovery life. Current present-visit acceptance precedes response use. Commands serialize behind raceSending; only the latest input is queued, while non-input clears that queue. Input/recover sequence chooses the maximum of local next, caller request and current car next. Final queue release requires the same visit, captured scope and race ID. A stop can bypass not-ready locally but still requires the real visit/permission checks. Input success preserves the existing world error; non-input success clears actionError and composes ready/drops/residents errors. Access failures use failWorld. Account/session/access/life/visit loss retires state.

None of that owner, its queue, readiness, clock or error producers is implemented by this codec assignment. Do not create a second world store, permissive admission flags, fake receipts, automatic timers or a network client to stand in for them. Local vehicle physics, track generation, prediction, collision, lap/reward/fuel settlement and recovery placement remain outside this stage. Their later presentation should use native Unreal systems. Keep this codec unmounted until the proper native command consumer and permissions exist.

## Completion

Inspect all five encodings, neutral input, JSON escaping, empty/overlong identifiers, invalid enum/numeric bounds, unknown/missing response fields, outer null versus inner string null, malformed/oversized inner racing strings and atomic output clearing. Inspect the full owned-path diff and preservation of existing older-tick/read semantics. Checkpoint only the three owned paths with hooks disabled. Write source/preimage/dependency hashes, API contract, limits and inspection evidence in the new handoff. Parent will obtain independent Standards and Spec reviews of the complete three-path change.

No tests, typechecks, lint, builds/compilation, codegen, assets/imports/conversion, engine/server/Blender launches, installations, remote operations, deployment or live configuration. Preserve unrelated work and production renderer/mobile inputs. Account/readiness gates and world-control interlock stay disabled. Source acceptance, Windows delivery, compilation, in-game inspection and release remain separate.

## SHA-256 pins

```text
d985dbc931fec8d9c5e0b6a6e5a5aae5b6d2ffd568210e155c2ac9f0f0fa26b7  packages/api/src/companion-world.ts
0ead5eb2164aa4ab89d226e479d60460a37812bb8c8962fb334bcd9cbdea26b3  packages/utils/src/companionRacing.ts
652b3c3f20c6a1721dd4c65940c7144fb81530f61f20e038fe596e1c92515474  apps/backend/convex/companionRacing.ts
f9b6acc4cc38d5c0ee7e000745dfcfe1d2dff556d31768524fa75cf256baf65f  apps/backend/convex/lib/companionRacingModel.ts
45be4baf91578930669721d829a475e6202c19b03ba4996537640ebe754898ed  docs/engineering/unreal-racing-state-handoff-2026-10-10.md
493db64cf9ee02f713ae82d95464a16dd168e0aaa636c42bb1b7e9104922b4e5  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionRacingState.h
dd54bbc605c97f037e9cb36c6ea30713c5103bc806088c4a3700cece48bb5001  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionRacingState.cpp
13ceb1b13bee4b0793ddb117e03559bfb8ea77f8bdb8dc81baa1ffa8e70bb618  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.h
df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.cpp
536bf39a4089acea43be36448ad3d02d3cd7f94a65015d3975f761941b802328  apps/companion-unreal/Source/BaddieCompanion/BaddieCompanion.Build.cs
```
