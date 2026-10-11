# Native Play room-utility observations

October 11, 2026. Source checkpoint `462d1a63abfbe93c0a93195701b27224f4cd51cb` extends the existing Play owner with retained solo-room utility observation and explicit mutation staging. Verification is source/diff inspection only. Independent review, Windows delivery, compilation, in-game inspection and release remain separate.

Assignment: `unreal-play-utilities-assignment-2026-10-11.md` at `5d8790a6fb484f283915c1d5a2140911747b8cd0`. Canonical/preimage/dependency base: `6419fb26b9a805553f555ccd55b25b02b5ab4345`.

Comment-only corrective source checkpoint: `ab56d2384e70290fd6ab20b19666a97b6132f5f7`. The reconnect comment now distinguishes retained Play wire, busy, readiness, command error and offset from cleared `RoomUtility` and `UtilityError`, with no emission. Executable code and the header are unchanged. This addresses the parent-reported Standards documentation finding; final two-axis acceptance remains with the parent.

Correction preimages were source `462d1a63abfbe93c0a93195701b27224f4cd51cb` and handoff `13ee0ab93607dbe4a3640b1488af776b7b727f85`. Their working SHA-256 values matched `74ef9f19209356e7a60436f067f6c150ce0635f070a8d88ca0d7a656661b5d31` for PlayState.cpp and `8bcf178c37b73206ca4c9672813cb85c419bb2523bfbd2c4702c5307189ab18c` for this handoff. The complete correction diff was inspected and the unchanged header hash confirmed before checkpointing.

## Scope and pins

Only `Public/BaddieCompanionPlayState.h` and `Private/BaddieCompanionPlayState.cpp` under `apps/companion-unreal/Source/BaddieCompanion`, plus this new handoff, belong to this continuation. All eighteen working input hashes matched the assignment before editing. The source preimages had no diff from the base, the assignment matched its pin, and this handoff was absent. The shared index was clear before the source checkpoint. Commits use explicit owned paths with hooks disabled.

Output SHA-256 at corrective source checkpoint `ab56d2384e70290fd6ab20b19666a97b6132f5f7`:

```text
2846dae90a3fa2a427d44984cf5d2f2b90410d06c76343000a4b5beec5006a16  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionPlayState.h
ad8953bc94c202bb78463afb0964c2459be37fbadb5c8210fd2fed05f9c0ee51  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionPlayState.cpp
```

Input/preimage SHA-256 at the canonical base:

```text
03ae962bb47624b09c673e9dc9e35ab85a5e0b2e09448e3d70a6b9cf9cd3f8e9  packages/api/src/companion-play.ts
f002e398719f911596fd8c6308445711006be92e8d23fd827431b5784f984952  packages/utils/src/companionPlay.ts
1a27ac9f3cbfbbcc14b5e77cfdea3ba24dbd960362b3e3ddbc13a1f3d01d6c97  packages/utils/src/companionWorldUtilities.ts
75c8f91d25054af64a205e9cf65c3cb96c51891193946637f56aa4f3bd304dcb  packages/utils/src/companionWorldClock.ts
047ced36383ebaffca0d1be7def24209c9344c57ffc1772d8c52cd1d349a4727  apps/backend/convex/companionPlay.ts
9c54ce319e5b6e3198f0fc84034b4f2613a3455162ed9e1096d8eb6d8b71a1c3  apps/backend/convex/lib/companionWorldUtilityObservation.ts
3305230fd179ccab68f79005bf4db8312104e8547fffaa1567ff650dc1494821  apps/backend/convex/lib/companionResidentsModel.ts
6ba177bce942793406ed29cb927f3d10142a1963e5beb513ec81c377a52d9c63  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionPlayState.h
5095cf81315f6f250911008568ad44e48b6a8ab904bc1021d14a9b41e84d2924  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionPlayState.cpp
768e58819bfeed6830939bf54fe7c82a29d26cb280a18c3c53940050bdbe350d  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionWorldUtilities.h
08a9a4b97c4fb1be5639abbb8f3abd8e9256b6be709130b36079f0419dae5646  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionWorldUtilities.cpp
affaccaa74d19c3bd78aa8de5057b163e2cf41c3db1ede616f3abf1829a2275c  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionResidentState.h
13ceb1b13bee4b0793ddb117e03559bfb8ea77f8bdb8dc81baa1ffa8e70bb618  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.h
df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.cpp
44024a15478022a094164e57fa953da32ad19b685c3266371dcbd14c499327b1  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionScenePower.h
536bf39a4089acea43be36448ad3d02d3cd7f94a65015d3975f761941b802328  apps/companion-unreal/Source/BaddieCompanion/BaddieCompanion.Build.cs
330ed17de2284048c8540a227a13cf1da14ac1254e33ab192c5f413221e33bb5  docs/engineering/unreal-play-command-handoff-2026-10-11.md
eb32614a8e71223284171fd15872e5ff9edf50997fc15c21d0fbe71610131a12  docs/engineering/unreal-world-utilities-review-2026-10-11.md
```

The accepted WorldUtilities pair, `FResidentClock`, StateJson and ScenePower remain read-only dependencies. Prior command/read behavior is documented in `unreal-play-command-handoff-2026-10-11.md`. Its utility placeholders are superseded by this continuation; the prior handoff itself is unchanged. This handoff's own hash follows in the completion report rather than being embedded recursively.

## Retained types and request API

`FPlaySnapshot` adds optional `RoomUtility` and `UtilityError`. A populated `FPlayRoomUtilityState` retains the full `FPlayRoomUtilityReceipt` and original monotonic `ReceivedAtMs`. The receipt carries server time, character/room/key/revision, nullable invitation, nullable `FWorldUtilityObservation` and retry delay. Its observation reuses `FResidentClock`. Snapshot getters return detached copies; public data construction grants no authority and cannot submit a receipt to the owner.

Utility request identity contains private session, query-generation, utility-generation and request GUIDs. Completion identity adds a separate once-only GUID. The utility slot is independent of the existing command request map. It does not write command `bBusy`, `Error`, `Wire`, readiness or `ServerOffset`.

| New public method | Contract |
| --- | --- |
| `SetRoomUtilitiesActive` | A changed value clears utility work and emits. Same value or disposed owner is a no-op. This records caller intent, not permission. |
| `BeginRoomUtilityObservation` | Checks retained eligibility and pacing, handles expiry notification, then prepares one immutable request. Ordinary begin does not emit or clear the existing display/error. |
| `MarkRoomUtilityDispatched` | Consumes a prepared request once and returns its packet. Requires immediate external send without a yield/callback. |
| `IsCurrentRoomUtilityRequest` | Checks active state and every private identity. External transport uses false to retire/abort its pending handle. |
| `AbandonPreparedRoomUtilityRequest` | Removes an unsent prepared request only. Preserves display, errors and deadlines; no notification or automatic retry. |
| `ObserveRoomUtility` | Consumes one dispatched response, validates it and emits success or fixed failure before returning a completion. Stale/duplicate responses return no completion and change nothing. |
| `ObserveRoomUtilityFailure` | Consumes one dispatched error, clears the utility display and emits bounded sanitized public text or the fixed fallback. |
| `ObserveRoomUtilityCompletionFailure` | Converts a current successful completion to failure, replaces its completion GUID and emits. Prior completion copies cannot finalize it. Keeps previous observation and deadlines. |
| `FinalizeRoomUtility` | Clears only the matching completing request. No notification or data/deadline changes. Cannot clear a newer request. |

`FPlayUtilityDispatch` exposes `GetEndpoint`, `GetArgumentsJson`, `GetReceipt`, `ShouldRetry` and `ShouldInvalidateQueryCache`. Both option getters return false. `FPlayUtilityCompletion::WasAccepted` includes valid null observations; it describes response handling, not current scope or native admission.

The future admitted transport must use a separate one-request mutation client, outside the shared gameplay mutation queue. Capture its own receipt, mark/send immediately, then pass one success or sanitized failure to the owner and finalize the returned completion after notification. If caller-side work fails after accepted response handling, replace the success completion through `ObserveRoomUtilityCompletionFailure` before finalizing. Stop/abort an outstanding handle when its receipt is no longer current. These APIs perform no HTTP request, actual abort or timer. A normal completion also becomes non-current after finalization.

## Eligibility, packet and timing

Eligibility requires a non-disposed active owner, at least one subscriber, selected scope, ready snapshot, no truthy command error, no truthy call-session ID, and present nonnegative safe room revision. Empty call-session/error strings are false, so they do not block. Revision zero is valid. Utility error and command busy are not additional guards. Character, room, invitation, host and Home permissions remain backend/admitted-transport responsibilities.

The packet targets the mutation `companionPlay:observeRoom`. It contains only `characterId`, `roomId`, `roomRevision` and a truthy `invitationId`. Missing/empty invitation omits the field and captures a null expected invitation. It never contains call-session ID, auth, permission fields or an act action ID. UE JSON owns escaping and number encoding. The existing 8,192-TCHAR target-string and outgoing Play JSON resource limits apply; serialization failure emits utility error and finalizes that unsent request without shortening its deadlines.

All attempt/receive times use `FPlatformTime::Seconds() * 1000`, retaining fractional milliseconds. Wall-clock command timing is unchanged. Nonfinite/negative begin time or nonfinite computed deadline fails closed. No periodic work is scheduled.

| Boundary | Behavior |
| --- | --- |
| Request age 0 through 15,000 ms, inclusive | Another attempt is suppressed, including while its completion notification runs. |
| Negative age or age greater than 15,000 ms | Retire the old request, clear displayed utility, set `Room conditions couldn't refresh.` and emit before checking the retry deadline. |
| New request at time T | Next attempt becomes T + 45,000 ms; retained earliest re-entry becomes T + 5,000 ms. |
| Valid receipt header/identity/RTT at R | Next attempt becomes R + max 45,000/retryAfterMs, before observation validation. |
| Ordinary clear | Next attempt becomes the retained earliest re-entry. No new five-second interval is started by the clear itself. |
| Failure or finalization | Does not shorten or reset either deadline. |

After an expiry notification, the method rechecks session/query/utility generations, eligibility and whether a callback already reserved a request. It uses the original captured begin time against the current retry deadline. This prevents the suspended attempt from replacing reentrant work or dispatching in a changed scope. At exactly the applicable 45-second retry or 5-second re-entry deadline an otherwise eligible attempt proceeds. Expiring a request at 15,001 ms still waits for its retry deadline. A completed response can extend that deadline beyond 45 seconds from the original begin.

## Decode, identity and observation continuity

The utility response uses bounded UE parsing with a separate ceiling of 1,048,576 TCHARs, depth 8 and 128 separators. This admits a maximally escaped room key within the required limit without widening existing Play reads. The future transport must also bound response bytes before constructing an FString.

The exact eight-field envelope requires `serverNow`, `characterId`, `roomId`, `roomKey`, `roomRevision`, `invitationId`, `observation` and `retryAfterMs`. Invitation and observation must be explicitly null or correctly typed; omission is invalid. Other receipt strings use the existing 8,192-TCHAR ceiling and must match the captured target. `roomKey` is preserved verbatim, is not derived locally or compared to a scene token, and must contain 1 through 131,072 UTF-16 units. `StringCast<UTF16CHAR>` counts with the explicit FString length, including embedded zero units, without replacing the stored string.

StateJson integer readers reject fractional, nonfinite and unsafe numbers before integer conversion. Header validation requires serverNow greater than zero, nonnegative room revision, and retryAfterMs from zero through 60,000. Character, room, revision and nullable invitation must match the exact captured target. Returned empty invitation therefore fails when the request omitted it. RTT must be nonnegative and at most 15,000 ms, inclusive. Identity/stage checks happen before reading receive time. Header/identity/RTT failures leave the existing retry deadline; a successful header advances it before observation shape, clock or sampler checks. Partial receipt data never reaches the snapshot.

Observation objects have exactly four fields, scopes four and clocks six. Versions must be 1. Unknown properties at every level reject; property order does not matter. Required timestamps/clock numbers are safe integers. Epoch times/game minute, scale and server time are nonnegative at decode; the reused sampler enforces positive scale, key syntax/length, epoch/watermark order and arithmetic bounds.

For non-null observation, `serverNowRealMs` must equal receipt serverNow. Same scope means exact equality of version, worldKey, clockEpochKey and zoneKey. Within that scope all six clock fields must be unchanged and server time must strictly increase. Comparison uses typed fields, not JSON/TMap serialization order. Reordered or extra fields cannot bypass continuity. A changed scope can establish a new epoch only through the full sampler validation. Only the last previous observation is retained, as in the canonical store; there is no per-scope history map.

Validation copies the new observation and, for the same scope, substitutes the previous observation's server time as `NotBeforeRealMs`. `SampleWorldUtilities` validates this temporary value; its sample is not retained. On success, the original new observation goes into both continuity state and the full displayed receipt. Original watermark decoding admits signed safe integers because the canonical same-scope path replaces that field during validation. Consequently an original negative or future watermark can survive a same-scope receipt even though direct later projection of that original value can fail. It is never silently repaired. Fractional/nonfinite/unsafe original watermarks still reject before conversion, a stricter native structural requirement.

A valid null observation clears displayed utility and utility error without replacing previous observation. Valid non-null data retains the original receive time. Later display projection must use the retained original observation and receive age with the accepted projection helper; it must not replace this receipt, refresh its age, clear continuity or mint scene power. No projection consumer is added here.

Malformed success and sampler/continuity failure emit the fixed refresh diagnostic. Error entry points require already sanitized public text; missing or over-8,192-TCHAR text uses the same fallback, while present empty text stays empty. A length cap alone is not a sanitizer for raw response bodies or exception text. Both success and failure emit before finalization. Finalization is silent and request-specific.

## Clear boundaries and reentrancy

`ClearUtilities` increments the utility generation, retires the request, resets next attempt to the retained entry deadline, and clears displayed utility/error. It preserves active intent and previous observation.

| Caller boundary | Additional behavior |
| --- | --- |
| Connect, including first subscription, changed selection and reconnect | Uses the ordinary clear, even if no subscription can open. Existing query/command generation behavior remains unchanged. |
| Current query failure | Uses the ordinary clear, then applies the existing empty ready/error snapshot. Utility activity and previous observation survive. |
| Active-state change | Records the new active value, clears and emits. |
| Last unsubscribe | Disables utilities, clears and preserves previous observation. Existing busy clearing and no-notification behavior remain. |
| Session reset or Dispose | Disables, clears, drops previous observation and resets both account-local deadlines to zero. Existing scope/listener/command clearing remains. |

Every callback-bearing public method captures caller values by value. No reference into optional/request/listener storage survives a notification. A success listener may clear utilities and start B; A returns a detached completion whose finalization then rejects without touching B. A recursive duplicate response sees a completing stage and cannot apply data twice. A failed completion conversion rotates its identity before notifying. The accepted owner contract still permits reentrancy but requires callbacks not to throw or destroy the owner during a call.

## Source inspection and remaining work

The complete two-file diff was inspected against the full account store, shared receipt/scope/snapshot types, backend access/observeRoom contract, utility/clock schema and accepted sampler. No request, sampler or timing test was executed. Manual branch traces covered:

- Active/subscriber/solo/ready/error eligibility, absent versus empty optional strings, revision zero and rejected fractional/negative/unsafe revisions. Command busy and utility error do not gate utility attempts.
- Every required envelope field, wrong types, missing versus null invitation/observation, target mismatch, UTF-16 limit including escaped/non-BMP input, version/key/clock failures and atomic publication.
- Exactly 15,000 ms pending/RTT acceptance, greater and negative ages, exactly 45,000/5,000 ms deadlines, 60,000 ms server retry delay, retained deadlines on header failure and extended deadlines on observation failure.
- Same-scope clock changes, equal/older server times, reordered/extra properties, changed scope/epoch, preserved original watermark, valid null receipt and continuity across select/reconnect/toggles/query failure/last unsubscribe.
- Expiry notification starting reentrant work, stale and duplicate receipts/completions, success/failure callbacks replacing caller variables, subscriber loss, same-ID session reset and disposal. Old finalization cannot remove newer work.
- Existing 18 command alternatives, complete-wire older-round retention and midpoint serverOffset code are unchanged. Utility clear hooks are the only additions inside their shared lifecycle methods. Utility response methods touch only utility state and never command busy/error.

Parent full-range Standards and Spec reviews must cover this complete three-path continuation before separate Windows delivery. The pending accepted Teams/WorldUtilities batch stays pinned to `6419fb26b9a805553f555ccd55b25b02b5ab4345` and excludes these later Play changes. No other chat was messaged.

Source implementation and author source/diff inspection are complete. Independent acceptance and Windows delivery are pending. Compilation, tests, in-game inspection and release were not performed. Authenticated transport, credential/permission integration, actual abort handling, caller-driven renewal wiring, UI and renderer consumers remain unimplemented. World/call observation, notices and settlement are outside this owner.

`FAcceptedApartmentScenePower` remains privately constructible with no producer. No receipt or sample changes saved lamps, actor visibility, daylight or native admission. StateJson, sampler, Resident/ScenePower, Build.cs, backend/mobile/shared source, old handoffs and tracker were not edited. Existing gates and the world-control interlock remain disabled.

No tests, typechecks, lint, builds/compilation, codegen, asset operations, engine/server/Blender launches, installs, remote operations, deployment or configuration ran. Production renderer inputs and mobile/shared behavior are unchanged; packaging and What's New do not apply. Writing-for-agents kept lifecycle contracts beside their APIs and separated source evidence from remaining integration; unslop guided the copy pass.
