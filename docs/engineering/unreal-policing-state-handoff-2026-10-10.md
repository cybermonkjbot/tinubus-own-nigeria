# Unreal policing read state

October 10, 2026. Source-only handoff for the existing policing client store.

## Scope and status

The initial bounded read/state implementation was checkpointed at `3a5396e16369c658646ae41ab245745ce9afb33c` in `BaddieCompanionPolicingState.h/.cpp`. It decodes the public encounter, preserves subscription/action ordering, captures local demand consent and prepares the four existing action shapes. It does not send requests or change an encounter locally after an action. Shared-reader integration is `1e49276bbd4b45308e30e6ed43fe72495997c138`. The full source passed independent Standards and Spec review with no findings, recorded in `unreal-native-state-review-2026-10-10.md`.

Only these three paths belong to this slice:

- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionPolicingState.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionPolicingState.cpp`
- `docs/engineering/unreal-policing-state-handoff-2026-10-10.md`

Source and independent review are complete. Windows transfer, compilation, native integration, in-game verification and release remain pending. No tests, compiler, code generation, build, editor, server, import, conversion, install, deployment or live configuration operation ran. Account/readiness gates remain unchanged and disabled. There is no playable-completion claim.

The module follows CombatState and RecoveryState's private decoded responses and GUID callback generations, and MissionState's caller-owned notifications. It reuses Unreal JSON, immutable shared reads, optional values and existing WorldPlayer field/point/enforcement types. CombatState supplies the complete six-weapon union and Lagos credit unit. There is no new simulator, networking implementation or native replication substitute.

## Canonical references

The references below were read from the shared checkout. SHA-256 pins identify the reviewed content, independent of concurrent commits.

| Reference | SHA-256 | Contract used |
| --- | --- | --- |
| `packages/api/src/companion-policing.ts` | `1383bd9678e5014495413b6a281381bbde91daa8afea60f1613d2a71a6141942` | Initial state, connect/disconnect/dispose, action gate order, callback generation and encounter checks, errors and busy completion. |
| `packages/utils/src/companionPolicing.ts` | `b1f6e06d26feb906cf5c5e1d5591311d868929f512a70e4bf4ed3c56781d3719` | Complete encounter/demand types, capture and exact confirmation comparison, amount cap. |
| `apps/backend/convex/lib/companionPolicingModel.ts` | `1bf485bea06bd3ebcb49f67a93e28dc5d3f6ad433711196b01996d37e9a06863` | Public validator, optional/null fields and point-array projection. |
| `apps/backend/convex/companionPolicing.ts` | `4a7d9c284ba8d1acc4e75b88634fbfc62f973504bdc0f7782edad7614001f59b` | Snapshot returns saved encounter/null. All three mutation endpoints return null. Demand generation, server expiry, settlement and release ownership. |
| `packages/utils/src/companionHaul.ts` | `2e729e6abeba8e28c7b7d328f985451162d8ba036174da86636adac0c9f02ab4` | Maximum 128 demand entry slices. |
| `packages/utils/src/companionCombatEquipment.ts` | `a3f2f01f9d50da5e7fbf271c2b94bee45104c89620684b497ac2f7ccdaf92325` | Bat, pistol, revolver, SMG, shotgun and rifle IDs. |

## Public-field audit

`DecodePolicingResponse` consumes the endpoint value, not a Convex response envelope. Subscription `null` is a loaded absence; initial/reconnecting absence remains unready. A missing body is invalid, never a loaded absence. Mutation values must be exactly JSON null.

| Wire field | Native field | Presence and value behavior |
| --- | --- | --- |
| `id`, `checkpointId`, `instanceId`, `officerId` | `Id`, `CheckpointId`, `InstanceId`, `OfficerId` | Required exact strings. No case folding, trimming or fabricated identifiers. |
| `phase` | `Phase` | Required searching/checking/cleared/arrested/detained/released enum. |
| `reason` | `Reason` | Required checkpoint/harassment/validated-assault/weapon-possession/observed-weapon/gunfire enum. |
| `createdAt`, `resolvesAt` | `CreatedAtMs`, `ResolvesAtMs` | Required finite doubles, unchanged server milliseconds. |
| `releaseAt` | `ReleaseAtMs` | Required finite number or null. Missing is rejected; zero remains a value. |
| `stationPlaceId`, `stationInstanceId` | Same PascalCase names | Required string or null. Missing is rejected; empty strings remain values. |
| `position` | `Position` | Required three-number array in source x/y/z order and metres. |
| `evidenceKey` | `EvidenceKey` | Required exact string. |
| `revision` | `Revision` | Required finite double. No revision filtering or rounding in reads. |
| `weaponId` | `WeaponId` | Missing/null/value retained separately; complete six-weapon enum. |
| `journeyId` | `JourneyId` | Missing/null/string retained separately. |
| `officerPosition` | `OfficerPosition` | Missing or three-number array. Explicit null is invalid. |
| `lastObservedAt` | `LastObservedAtMs` | Missing or finite double. Explicit null is invalid. |
| `agency` | `Agency` | Missing or police/lastma/efcc/ndlea. Missing stays missing, without a read-time default. Null is invalid. |
| `stopKind` | `StopKind` | Missing or checkpoint/roadside. Null is invalid. |
| `extortion` | `Extortion` | Missing/null/object retained separately. |
| `extortion.amount` | `Amount` | Required finite double, not a balance or predicted debit. |
| `extortion.unitKey` | `UnitKey` | Required lagos-credit enum. |
| `extortion.status` | `Status` | Required demanded/paid/refused enum. |
| `extortion.outingId` | `OutingId` | Required exact string. |
| `extortion.entries` | `Entries` | Required ordered array, including empty arrays, up to 128 items. No sorting, deduplication or sum repair. |
| `entries[].entryId`, `entries[].amount` | `EntryId`, `Amount` | Required exact string and finite double for every entry. |

Unknown object fields, wrong types, unsupported enum values, nonfinite numbers and malformed points fail decoding. Strings are retained within the overall 262,144-character response bound. The preparse scan limits nesting to 8 and structural separators to 2,048. Entry count is independently bounded before copying. These are defensive decoder limits, not new server validation or settlement rules. The future transport must bound raw response bytes before constructing the input string.

Decoding builds a fresh candidate and only publishes a `TSharedPtr<const FPoliceEncounter>` after every field succeeds. Failure resets the entire decoded output and returns fixed diagnostic text without payload values. State acceptance of an invalid subscription marks readiness false and retains the last valid encounter, matching subscription-failure behavior rather than converting malformed data into a null encounter.

## State and ordering

`OwnerId` is process-local scope supplied by the admitted caller, not a field added to the backend projection. A new account selection clears all data and retires callbacks. The caller must also clear on credential/access loss for the same account. The decoder cannot independently authenticate the account because this endpoint's value has no owner ID.

| Event | Result |
| --- | --- |
| First subscriber or reconnect | `BeginSubscription` creates a new GUID generation, clears encounter/error/readiness/busy and retires the old command. |
| Valid current subscription | Replaces encounter, including null, changed IDs and lower revisions. Sets ready and clears error, preserving busy. Arrival order is canonical; no native revision filter is added. |
| Current subscription failure | Keeps encounter and busy, sets ready false and stores the supplied display-safe error. |
| Last subscriber removal | `EndSubscription` retires all receipts, clears busy and retains encounter/readiness/error. A later begin clears them before reconnecting. |
| Command preparation | Requires an active subscription, non-busy state, readiness and an encounter. Pay additionally checks exact consent before the ordinary readiness/encounter return. Sets busy and clears error only when preparation succeeds. |
| Mutation null acknowledgement | Clears busy for the current pending command. Does not update encounter, readiness or an intervening error. Reports stale encounter if the displayed ID changed or became null. |
| Mutation failure | Sets error only if the current encounter ID exactly matches the captured ID. Always clears that pending command's busy state within the same generation. |
| Old generation, owner or completed command | Ignored without affecting the current state or a newer busy command. |
| Clear/dispose | Drops private data and retires callbacks. Dispose is terminal. |

Only one prepared command may be pending. Its private command GUID prevents duplicate completions from clearing a later request. Subscription receipts remain valid across encounter updates in the same generation. Mutation error matching uses only encounter ID, as the canonical store does; it does not add a revision, phase or instance constraint. A same-ID return after an intervening encounter remains eligible under that existing rule.

The caller owns subscription counts and UI notification delivery. Call begin only for the first subscriber or an actual reconnect, and end when the last subscriber leaves. `IsCurrentCommand` repeats the canonical generation check before dispatch and checks the active command receipt. It does not reprice or silently retarget a prepared command after a subscription update. Prepare and dispatch must share one synchronous caller step, with no awaited work between them.

For failed/cancelled dispatch, call `AcceptFailure` with the command receipt so busy can clear. Supply only an already extracted, display-safe error message. Empty messages remain empty, matching `Error.message`. Missing or over-1,024-character messages use fixed fallback text. The module never logs or retains transport bodies or credentials.

## Demand consent and command shapes

`CapturePoliceDemand` returns a confirmation only for checking encounters with a demanded Lagos-credit amount. Amount must be a safe integer from 1 through 100. Revision and resolvesAt must be nonnegative safe integers no greater than JavaScript's 9,007,199,254,740,991 limit. The capture does not require future resolvesAt, inspect entry sums or validate balances. These checks belong to the server, not this local receipt.

`MatchesPoliceDemand` compares exactly seven fields, with case-sensitive string equality and exact numeric equality: encounterId, instanceId, revision, resolvesAt, amount, unitKey and outingId. No tolerance or omitted-field fallback is used. Entries, evidence, agency, reason, station and positions are not extra confirmation conditions.

Pay while unready gives `Wait for the current encounter to load.` Pay with absent/mismatched consent while ready gives `This demand changed. Cancel and review the current encounter.` These errors do not start a command. Busy, unsubscribed or disposed calls remain silent, including pay calls, because those guards run first. Cooperate, release and refusal do not gain extra phase, deadline or demand gates absent from the canonical store.

| Prepared action | Canonical endpoint | Non-auth arguments |
| --- | --- | --- |
| Cooperate | `companionPolicing:cooperate` | `encounterId` |
| RequestRelease | `companionPolicing:requestRelease` | `encounterId` |
| PayExtortion | `companionPolicing:respondExtortion` | `encounterId`, response `pay` |
| RefuseExtortion | `companionPolicing:respondExtortion` | `encounterId`, response `refuse` |

The prepared command contains no amount, outing, entries, confirmation payload or credentials. Its function name records the existing account endpoint contract only. It does not make those endpoints available to a native grant. The future transport must preserve the canonical no-retry/no-query-cache-invalidation behavior and must not attach account credentials to Unreal.

No local clock expiry, release transition, offence finding, credit debit, haul mutation, journey resume or success simulation is implemented. The canonical backend owns those changes and publishes their resulting encounter or null. Consent is not native permission.

## Remaining integration

The parent owns integration and the existing coordinator owns Windows delivery. No other chat was messaged and no remote handoff was attempted.

The shared-helper dependency recorded at `e3ef0d373002bbdef335d3b3fc1d10b2c75bca9d` is now integrated in the policing source. The combat/recovery owner supplied private `BaddieCompanionStateJson.h/.cpp` at `ab535c29cedd4e893a62539f247a03488495b3e9`. This follow-up changes only the policing `.cpp` and this handoff, leaving the shared helper and public header unchanged.

Policing reuses `Parse`, `Same`, `Object`, `Field`, `Only`, `String`, `Number` and `Nullable`, with explicit limits of 262,144 characters, depth 8 and 2,048 separators. `Only` preserves the full lengths of UE 5.8 shared-string keys. The raw-null branch and outer length guard remain ahead of object parsing. Required nullable fields still decode into a fresh candidate; optional presence, enum sets, point shape and the 128-entry bound stay domain-owned. Typed lambdas select the shared two-argument numeric reader where it is passed as a callback. Demand safe-integer validation keeps its whole-number check around the shared finite, nonnegative safe-range predicate, without converting read values to integers.

Source/diff inspection and the complete independent review confirmed exact strings, finite-double reads, required/optional/null distinctions, unknown-field rejection, atomic failure clearing and state/command ordering. No missing helper behavior was found for this extraction. Delivery uses the reviewed matching shared helper at `691a60a98ac276d3c9a59793965b5b1bb47a69d1`; its later scalar-null addition does not change policing's existing raw-null branch or other reader operations.

An independently authorized producer must bind account/access scope, subscription lifetime, safe error extraction and response callbacks. Any future command route needs its own reviewed native consent and backend admission. The current read-only or entry/presence grants do not supply policing permission.

Native presentation still needs encounter UI, dedicated confirmation/cancellation, source-to-world coordinate mapping where required, and shared clock-based display of existing deadlines. Labels/reasons, patrol generation, enforcement planning, pursuit, custody movement and settlement were outside this bounded store translation and remain with their existing owners. Do not introduce local resolution when a displayed deadline elapses.

No production renderer, shared scene data, mobile or backend implementation changed, so this source slice requires neither renderer packaging nor a mobile What's New change. All validation beyond source/diff inspection remains deferred to Joshua's explicit authorization.
