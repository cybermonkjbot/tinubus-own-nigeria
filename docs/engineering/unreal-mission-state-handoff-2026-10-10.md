# Native mission snapshot and response state

October 10, 2026. Source implementation and source review only. No caller, mission admission, playable result or Windows delivery is included.

## Owned delivery

- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionMissionState.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionMissionState.cpp`
- This handoff.

All three paths were absent from disk, the index and tracked source before implementation. The port tracker and existing handoffs contained no competing assignment for these paths. The parent assigned this slice exclusively. Existing MissionPresentation, controller, session, controls, squad, compass, combat/recovery, backend, mobile, Config, Build.cs and tracker files remain outside the original patch. The later shared-reader correction is recorded below.

A concurrent workspace checkpoint, `95654a7fb1d1283d92e03b78fb9cf39078e609e3`, captured this turn's new source pair before its final commit. Inspection confirmed that those blobs match the implementation before the final case-sensitive reward-map correction. The final owned-path checkpoint adds that correction and this handoff. It does not rewrite the concurrent commit or include its other files. Transfer the three final pinned paths, not the unrelated contents of that earlier checkpoint.

## Implementation

`DecodeMissionSnapshot` uses Unreal's JSON reader, serializer and typed values. It accepts the endpoint's result object, not a Convex transport envelope. The decoder does not retain the raw JSON tree, unrecognized fields, credentials or private database records. Failure clears the decoder output and returns fixed text. The state owner decodes into a temporary value, so failure retains its last accepted snapshot.

`FMissionState` is a non-copyable game-thread object with no networking, ticking, actor references, delegates or command encoding. An external caller can capture a selection-generation token and feed subscription results. `BeginObservedMutation` captures the selected room, generation, update count, request serial and supplied start time for bookkeeping only. It neither sends a request nor says that one is permitted. Exact current-request matching also rejects duplicate completions. Tokens retain a small local identity allocation so a destroyed owner cannot be confused with a new owner at the same memory address.

There is no include or binding to `BaddieCompanionMissionPresentation.h`. `canLead`, `canUseCrew`, lobby entry, mission phase and crew readiness remain observations, not native command permission. No automatic resume, account credentials, native grant scope, activation or gate change was added.

## Public projection parity

The field mapping follows `MissionRunView` in `packages/utils/src/companionMissions.ts:86`, `missionView` and `missionRunView` in `apps/backend/convex/lib/companionMissionsModel.ts:17` and `:54`, and the result/view in `apps/backend/convex/companionMissions.ts:29` and `:50`.

| Canonical fields | Native representation and handling |
| --- | --- |
| `run`, `joinable` | Required nullable `FMissionRunState` values. Both decode the complete public projection, without assuming joinable state grants access. |
| `completed`, `rewardReadyAt`, `canUseCrew`, `serverTime` | Exact case-sensitive IDs, per-mission timestamps, boolean and server milliseconds. Missing/null `rewardReadyAt` becomes an empty map, matching `wire.rewardReadyAt ?? {}`. Other required fields may not be omitted. |
| `id`, `missionId`, `leaderId`, `crewThreadId` | Exact IDs. Crew ID is required and nullable. Mission IDs are not renamed, filtered through a replacement catalog or converted to case-insensitive `FName`. |
| `revision`, `stepIndex`, `phase`, `sceneStartedAt`, `failure` | Nonnegative safe-integer revision/index, the six canonical phases, required nullable scene time and failure text. A terminal index beyond the final authored step remains representable. |
| `participants` | Public user ID/name pairs in received order. No reconstruction of filtered people or assumption that the leader must appear in the visible roster. |
| `canLead`, `completionPending` | Optional booleans; absence stays distinct from false. A failed run with completion pending does not become a replayed objective or a completed reward. |
| `readiness` | Optional revision, self-ready flag, ready user IDs and all-ready flag. No local readiness computation or inferred consent. |
| `finding`, `findings` | Nullable optional current text and optional ordered step ID/text history. No recreation of private choice records. |
| `challengeContributors`, `objectiveStartedAt` | Optional received IDs and time. Contributors need not be in the current visible roster; historical evidence is not discarded locally. No local score adjudication. |
| `target` | Optional nullable place ID, three finite source-coordinate values and label. No UE coordinate conversion, actor lookup, tracking or hidden position synthesis. |
| `reward` | Optional nullable credits and carried/banked/forfeited status. No reward grant, local claim, bank operation or inferred payout from elapsed time. |

`TMissionField` distinguishes missing, null and value. Optional non-nullable fields reject explicit null. Required nullable fields reject omission. Before the first accepted result, run/joinable are missing rather than a confirmed server null. `bReady` alone is not proof of valid data because a subscription error also sets it. Consumers must inspect error and field presence.

The decoder bounds input before allocating the JSON tree to 262,144 TCHARs, 16 nested containers and 16,384 structural separators. It limits IDs to 240 characters, public text to 8,192 characters, participants and ready IDs to four, completed/reward-map/finding entries to 128, and contributors to 512. These are native acceptance ceilings, not new backend limits. The backend currently reads fewer than 128 history records, at most four seats, findings bounded by authored step count and activity evidence bounded by step count times four. The current definitions fit these ceilings; future expansions must review them. Exceeding a bound rejects the entire result without truncation.

Numbers must have JSON numeric type and be finite. Revisions and indices must be nonnegative integers no larger than 9,007,199,254,740,991. Times and credits retain fractional values within that nonnegative range. Target coordinates retain any finite value in the public triple. Strings are not coerced from numbers or booleans. IDs reject edge whitespace and control characters. Unknown bounded mission, place and step IDs remain data; unsupported phase/reward enum values reject the result.

## Local response parity trace

These are source traces against `packages/api/src/companion-missions.ts`, not executed tests.

| Canonical owner | Native behavior |
| --- | --- |
| `accept`, lines 13-18 | An older revision of the same non-null run retains the whole prior snapshot, including joinable, history, clock and error. Equal revisions may update participant readiness or reward data. Null or a different run can replace the prior run. Joinable does not get an invented independent revision rule. |
| Successful observation | Increments accepted-update count, replaces the complete wire value, marks ready, clears error and observes `serverTime - receivedAt`. Busy is unchanged while a mutation remains outstanding. |
| Mutation start, lines 32-35 | No selected room, disposed state or an outstanding request prevents capture. Successful capture sets busy and clears error without changing ready or story state. Capture does not establish authority. |
| Mutation ordering, lines 39-44 | Accept only if no result has been accepted since request start, or both optional run IDs compare equal and incoming revision is at least current revision. Two null runs compare equal with revision -1, matching JavaScript optional-ID semantics. |
| Mutation clock, lines 42-43 | Eligible responses use `serverTime - (beganAt + receivedAt) / 2`. With no intervening accepted update, an older same-run response still updates this midpoint offset even though `accept` retained the wire. This preserves the canonical expression rather than silently changing its behavior. |
| Subscription failure, lines 28-29 | Marks ready and records error while retaining wire, offset and any outstanding busy request. |
| Mutation failure/finally, lines 45-50 | Records error without changing ready or wire, then clears only the matching request's busy state. A stale or duplicate completion cannot clear a newer busy request. |
| `select`, line 59 | Changed room, including selection of no room, clears displayed state and retires callbacks. Same room is a no-op. Invalid native room input is rejected without changing the current selection. |
| `reload`, line 60 | Retains wire/ready/offset, clears busy/error and retires callbacks. The caller owns resubscription. |
| Last unsubscribe, line 57 | `StopObserving` retires callbacks and clears busy while retaining data/error. The caller owns listener counts and transport cancellation. |
| Disposal, line 61 | Terminal invalidation rejects every callback. Native `Dispose` additionally clears retained presentation data; `Clear` supplies the same clearing for identity/access loss without terminal disposal. |

Concrete ordering cases inspected:

- Run A revision 5, request starts, subscription accepts A revision 6, mutation returns A revision 5. Retain revision 6 and its clock; finish that request's busy state.
- Same sequence, mutation returns A revision 6 or 7. Accept it and use the request midpoint clock. Equal revision is deliberately not rejected.
- Request starts in A, subscription changes to B or null, response returns A. Retain the subscription result. If both current and returned runs are null, the canonical equal-optional-ID branch allows the response.
- No intervening accepted update, mutation returns older A. Retain the wire and error, update only the eligible midpoint clock, clear busy.
- Select another room, reload, stop observing, clear or dispose during a request. Its old generation cannot change data, error, ready, clock or a subsequent request's busy state.
- Malformed nested data after a valid result. Retain all prior wire fields and report an error; no partial projection or invented empty mission escapes. A later valid result clears the error.

Caller times are finite Unix-epoch milliseconds, supplied where the canonical owner calls `Date.now`. This class does not sample a clock, extrapolate mission completion, impose WorldClock's expiry rules or sort results by server time. Local wall-clock reversal is not a new ordering rule. Invalid caller time produces a typed error. Counters fail closed rather than wrap.

The deliberate transport-free differences are explicit. There is no event-emission layer or listener registration, no automatic resume from line 26, and no raw transport exception text. Native errors use `EMissionStateError`; future UI/transport integration must supply approved actionable public diagnostics without exposing credentials or private exception bodies. The endpoint's public run failure/finding text is preserved separately. Generation-local update counters reset only after retiring all old receipts, preserving the canonical comparison.

## Existing story and future consumers

This patch does not copy or replace the authored catalog. `missing-reel`, `last-witness` and all expansion IDs remain unchanged in canonical source. Their received run IDs, indices, phases, findings, timestamps, completion history and reward state survive decoding. Scenes, reach, arena fights, inspections, choices and football/dance challenges remain the existing objective kinds. Authored shots, choice definitions, rank/XP display and cutscene sampling still need native content/presentation integration. No local timer advances a phase or credits a reward.

The rural-expedition document proposes additional missions and objectives that are not in the current contract. None is registered here. The mission state may later supply story/checkpoint display to the existing mobile-first panel after separate admitted caller integration. It cannot map an inspection target into Clear brush or Untie, because the current mission command union does not implement those actions. Action admission still requires the real run, revision, target, range, equipment and current request owner.

The existing compass accepts admitted coarse hearing sectors only. Do not feed mission target coordinates, participant IDs or presence into it. Squad Move/Hold still needs accepted encounter/unit ownership and eye-level visible-ground resolution. Mission readiness and crew membership do not grant that control. Streamed actor loading/unloading must not change this logical run state. Combat, recovery, rescue, terrain and reward decisions stay with their canonical owners. No jobs, dialogue, food or rural-campaign expansion belongs to this slice.

## Engine evidence

The port and session handoffs identify the installed Windows engine as UE 5.8.3. This turn made no Windows calls. It reused the existing module's Core/Json dependency and the established native decoder pattern, with no module or engine configuration change. Epic's 5.8 API documentation was consulted on October 10 for `FJsonValue`, `FJsonObject`, `TJsonReaderFactory`, `TJsonSerializer`, its JSON-object policy and shared-string keys. In 5.8, `FJsonSerializer` aliases `TJsonSerializer<FJsonSerializerPolicy_JsonObject>`; the policy's map result is `TSharedPtr<FJsonObject>`. The reward-map reader copies JSON shared-string keys by explicit length before validation, avoiding an assumed implicit conversion to `FString`. Its retained map uses Unreal's `FLocKeyMapFuncs<double>` for case-sensitive equality and hashing, so IDs differing only in case cannot merge through default FString key comparison.

Primary references:

```text
https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Json
https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Json/FJsonValue
https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Json/FJsonObject
https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Json/FJsonObjectSharedStringStorage
https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Json/TJsonReaderFactory
https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Json/TJsonSerializer/Deserialize
https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Json/FJsonSerializerPolicy_JsonObject
https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Core/TSharedString
https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Core/FLocKeyMapFuncs
```

The direct old `FJsonSerializer` documentation route was unavailable; the 5.8 module alias and template/policy pages supplied the relevant declarations. Documentation and existing usage are not installed-header or compiler acceptance for these new files.

## Shared reader and invalid start-time correction

The parent assigned one editor to the three State `.cpp` files after independent review identified duplicated JSON preflight/scalar readers and a P3 mismatch between mutation-start behavior and the promised `InvalidReceiptTime` diagnostic. This correction also adds private `BaddieCompanionStateJson.h/.cpp` and updates the combat/recovery handoff. No public header, signature, field-presence representation or canonical response-ordering policy changed.

The shared helper owns bounded object parsing, object/field lookup, exact strings, finite numeric and typed boolean reads, safe integer/time predicates, case-sensitive comparison, opt-in unknown-key rejection and the existing combat/recovery required-nullable helper. Its exact contract is recorded in `unreal-combat-recovery-handoff-2026-10-10.md`, under Shared JSON and mission error correction. Both shared files are now required dependencies for source delivery.

Mission supplies its unchanged limits of 262,144 TCHARs, depth 16 and 16,384 separators. Its text/ID constraints, optional/nullable policies, array limits, enum checks, ignored unknown fields, case-sensitive reward map and public projection remain in the mission implementation. It does not use combat/recovery's unknown-key allowlists or nullable template. The shared code stays in a named private namespace with one non-template implementation, preserving unity-build isolation.

`BeginObservedMutation` now clears the output receipt first, then checks ordinary disposed/busy/unselected refusal before validating time. For an active selected idle owner, NaN, infinity, negative or above-safe-range BeganAtMs sets `InvalidReceiptTime` and returns false. It preserves wire, readiness, busy, offset, pending-request ownership, update count and request serial. No observation token or mutation receipt is minted. Valid fractional, zero and maximum-safe-range timestamps keep their prior behavior. Ordinary refusal leaves state and its existing error unchanged. Successful capture still clears error and sets busy, and completion ordering is untouched.

Source/diff review covered populated and empty snapshots, prior errors, invalid start times, ordinary refusals, cleared output receipts and valid capture. The shared preflight/scalar extraction was compared against all three previous implementations, including recovery's separate raw-null branch. Public headers were left untouched. Positional field-policy booleans and XYZ grouping remain deferred as requested. No tests, lint, typechecks, builds, codegen, process starts or remote delivery ran. Parent correction review and transfer remain pending.

## Canonical source pins

| Source | SHA-256 at review |
| --- | --- |
| `packages/api/src/companion-missions.ts` | `f56f25b592c50b491c24be0985c52f8f883ce6310d76447f1ea0342957783a30` |
| `packages/utils/src/companionMissions.ts` | `5801b3926f513dde0fb3aaa12744667451c90dc74bff61033bebaded54366d54` |
| `packages/utils/src/companionMissionExpansion.ts` | `b8363e7e6d81254e6f6bca63d1989b2acc72b0f4b5ac33107b8fb864efbacfd8` |
| `apps/backend/convex/companionMissions.ts` | `1fbb544f87bf00a3581575a61e2f4e49b8ec02724be041caa5b3fdb92e026e8a` |
| `apps/backend/convex/lib/companionMissionsModel.ts` | `4cd55f75d9fd37c32c7db641ea284d77388d5ed5f28bfdb1d421aaa707f8a99b` |
| `packages/utils/src/companionMissionTargets.ts` | `407f7bb6fdb6c6ede94f30c1b7614e5487cac56c54f3ed832462b34c28bab2dc` |

## Remaining integration and acceptance

The parent owns independent review and integration; the coordinator owns Windows delivery. The future caller must validate its own account/Companion/room/session identity and actual mission authorization, bound and unwrap the transport response, capture metadata before dispatch, serialize calls on the game thread and cancel observation ownership on access or identity loss. Call `Clear` for a new identity even if the room ID is unchanged. Capture new observation tokens after selection/reload and pass the original receipt to the matching completion. Dispose before destroying the owner and never invoke a destroyed object. Any resume must be a separately admitted explicit request.

Native catalog/content mapping, actionable error presentation, cutscene display, command transport, listener integration, MissionPresentation admission, combat/hearing/squad ownership, Windows transfer, compilation and runtime acceptance remain unfinished. No tests, typechecks, lint, build, compilation, codegen, engine/server launch, install, import/conversion, deployment, release or live configuration ran. No production renderer input or mobile behavior changed, so renderer packaging and What's New changes are not triggered. Source review does not establish visual or playable parity.
