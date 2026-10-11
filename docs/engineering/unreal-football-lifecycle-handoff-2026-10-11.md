# Unreal Football opening and input lifecycle

October 11, 2026. Source-only extension of the existing `FFootballReadState`. Original source commit `e766d0eee5508526f88708d7272409266cfe8839` adds captured opening, input/neutral ownership, focus and foreground handling, pulse scheduling and final-effect dispatch. Correction `0b828eace1512ef92c379b66768e28832efc9831` fixes connected-player eligibility for duplicate IDs. The slice remains unmounted. Full corrected-range review, compilation and runtime acceptance are pending.

## Scope and pins

Assignment `docs/engineering/unreal-football-lifecycle-assignment-2026-10-11.md` is pinned at `1de31c66058254648f8e4d7dab055dbf443fe98b`. Canonical references, source preimages and dependencies use `cfa15652c52e510aac532a8cc4e13ce633d8415e`.

Only these paths belong to this change:

- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionFootballState.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionFootballState.cpp`
- This new handoff.

All 13 pinned working files matched before editing, and this handoff was absent. The 11 read-only references and dependencies matched afterward. The existing decoder region was compared byte-for-byte with the base and remains unchanged. StateJson, Build.cs, previous handoffs, other domains, backend/mobile and configuration were not edited.

The accepted seven-command implementation and its completion-alias correction remain in the owner. The new lifecycle performs the previously deferred input resets, pump synchronization and leave/end deactivation. It adds no second world store, transport, local simulation, physics, NPC movement or rendering.

## Adapter contract

Calls and callbacks belong to one owning thread. Keep the owner alive through every notification and final-effect callback. Credentials, domain permission, transport admission and query cancellation remain external.

1. Select the account, nonsecret session-incarnation marker and room through the existing API. A changed selection or unconditional `ResetSession`, including identical IDs, retires all lifecycle state. A same-value `Select` remains a no-op.
2. Maintain actual match-listener counts externally. Call `BeginMatchObservation` on first observation or reconnect. `OnChanged` bindings alone do not count as observers. Call `EndMatchObservation` only on last-listener loss, not as the first half of a reconnect. Independent entry receipts retain their existing behavior.
3. After each owner call and completion, reconcile `GetPumpInterval()` with one native timer. Unset means stop. Set means a 200 ms `FTimespan`: on the stopped-to-running transition, arm the first tick one interval later, then keep that cadence without restarting it on each call. Each tick calls `Pump` once. No timer or process starts in this source.
4. Drain `GetPendingEffects()` after each owner call, including after notifications return. It returns copies, with neutral cancellation first. Inspect the packet's captured scope, establish current external admission, call `DispatchEffect` with its receipt, and send only the successfully returned packet immediately without yielding or notifying. All mutations require `retry: false` and `invalidateQueryCache: false`. Receipts are local ownership, not native or server authorization.
5. A failed dispatch does not retarget or mutate the packet. Explicitly abandon only identified unsent work with `AbandonEffect`. Marked packets require their terminal callback or session destruction. Never abandon whichever packet happens to be pending after reentry without checking identity.
6. Route open replies to `AcceptOpenSuccess` or `AcceptOpenFailure`, input/neutral acknowledgements to `AcceptInputSuccess`, and pulse/release replies to `AcceptPulseSuccess`. Input/pulse failures use `AcceptEffectFailure`. Supply caller-sanitized public failure text, not raw exceptions or envelopes.
7. Keep the existing `PrepareCommand`, `DispatchCommand`, `BeginCommandSuccess` and `AcceptCommandSuccess` sequence. `AcceptCommandSuccess` now deactivates owned input for current leave/end success before applying the snapshot. The retained `RequiresInputDeactivation` hint does not require a second call to `SetActive(false)`.
8. Bind `OnFinalEffects` before using the owner with transport. During last-observer loss or `Dispose`, it receives immutable neutral/release candidates while the session and query are still valid. Dispatch and send the supplied packets synchronously through `DispatchEffect`, in supplied order. All other state mutation and terminal callbacks are rejected in this window. Return before external authority is destroyed; do not queue final sends for later. The owner then retires control callbacks and stops the pump.

Clock parameters are `FDateTime` UTC samples, defaulting to `UtcNow()`. They become Unix milliseconds through the native date API. Send intervals and renewal intervals use client wall time, matching `Date.now()`. Source coordinates remain metres; there is no Unreal-axis or centimetre conversion. The adapter must not pass monotonic elapsed seconds as wall time.

## Opening and manual retry

`PrepareOpen` copies the selection and roster before notification. It uses `TArray::StableSort` with `FTextComparison::CompareTo` and `ETextComparisonLevel::Default`. Comparison uses the engine's active culture configuration, with no hardcoded language, case folding, byte normalization or lexical fallback. Equal comparisons retain input order. The selected comparison policy is engine-default culture-aware comparison; no actual runtime culture name was observed this turn.

The local importer record identifies installed Unreal 5.8.3. Epic's [5.8 FTextComparison reference](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Core/FTextComparison) confirms `Internationalization/TextComparison.h` and the `CompareTo` signature. Its [comparison-level reference](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Core/ETextComparisonLevel__Type) includes `Default`. These references were consulted October 11, 2026. The installed ICU/Legacy implementation was not inspected this turn. Before mounting, record the engine's active culture/locale and the JavaScript runtime's default locale, then accept reordered and equal-collation examples across those runtime versions. Matching API names do not establish equivalent collation data.

The retry signature contains instanceId, revision, epoch, mode and the ordered id/side roster. It excludes requestId. Exact signature equality reuses the earlier complete captured selection, including its requestId. Changing any signature component replaces it. A new caller requestId, even an invalid one, does not override an identical retained request; validation applies to the effective captured request. An ambiguous transport failure or malformed response retains the request. A successfully decoded open reply clears it while the captured session still owns the operation, even if its query generation changed. Session destruction also clears it. There is no automatic retry.

Structural validation rejects invalid mode/side enums, empty or over-160-character instance/player IDs, duplicate player IDs, unsafe or negative revision/epoch, request IDs outside 8 through 64 ASCII letters/digits/underscore/hyphen, and rosters outside 1 through 12 players. Solo requires one Home seat, co-op permits up to eight Home seats, and teams require both sides. These bounds come from the endpoint and its shared creation validators. The owner does not infer that the selected account is a roster member, host, admitted visitor or ready at the pitch; the server still checks those conditions.

Opening refuses disposed, finalizing, busy or invalid-scope owners. It does not require an existing match, match observers, foreground, ready or host flags. Before notifying, it captures the packet, resets input, sets busy, clears error and stops the pump.

Open and command completion intentionally differ:

| Event | Open | Seven commands |
| --- | --- | --- |
| Query changes after dispatch | Successful decoding clears the retained entry, but the snapshot does not apply. Finally clears busy and synchronizes the pump. | Terminal state retires without applying data/error or clearing busy. |
| Ordinary failure | Returns sanitized `FFootballOpenOutcome.Error`, retains the entry, and clears busy. It does not publish a match error. | Publishes retained-data match error before its current-generation finally notification. |
| Current successful snapshot | Applies while busy, then clears busy and notifies. | Same notification order. Leave/end deactivates input before even an older reply is considered. |
| Session replaced inside notification | Old local receipt cannot clear the new entry, request or busy latch. | The accepted completion-alias protection remains intact. |

`FFootballOpenOutcome.Result` describes snapshot handling. `Error` distinguishes an ordinary or malformed failure; a present empty public message remains empty. Unset failure text uses `Couldn't open football. Try again.` Malformed open data uses the existing fixed decoder error without publishing it to the match. A stale-query successful open has `StaleReceipt` and no error.

## Packet shapes

Every packet contains captured roomId. Authentication and device values remain external.

| Endpoint | Additional fields |
| --- | --- |
| `companionFootball:open` | `instanceId`, `revision`, `epoch`, retained `requestId`, `mode`, `players: [{id, side}]` |
| `companionFootball:input` | `epoch`, `input: {sequence, move:{x,z}, aim:{x,z}, destination:{x,z}\|null, action, targetPlayerId:string\|null, power, cancelActions?}` |
| `companionFootball:pulse` | `epoch`, `release:boolean` |
| `companionFootball:command` | Existing seven-command shape, unchanged. |

`cancelActions` preserves absent versus explicit false. The owner-generated neutral packet has zero movement, aim `{x:0,z:-1}`, null destination/target, action `none`, power `.7`, `cancelActions:true` and a newer sequence. No input packet includes match state, simulation results, credentials or ownership claims.

## Input and focus

The playing predicate is ready, empty/absent error, not busy, a playing/restart match and any player row whose exact ID matches playerId and whose connected flag is true. A disconnected earlier duplicate does not prevent a later connected row from qualifying. An empty error string is falsey here, matching the canonical store. This predicate schedules work; it grants no control permission.

`Input` copies the complete sample and ignores its caller sequence. It validates using the next owned sequence, then stores latest movement with action `none` while retaining aim, target, power and cancelActions. Changed latest input sets dirty. A non-none action joins the detached queue only while it has fewer than two items; saturation still updates latest movement. Normal flushing requires active input, no input/neutral request in flight or prepared, and the 200 ms minimum send interval. It takes the first action only after the current player's observed lastSequence reaches actionSequence. Otherwise dirty movement or a 400 ms movement renewal can send latest. Renewals preserve the canonical latest target/aim values; server-side inheritance of a pending action remains server-owned.

Every generated packet receives complete validation. Sequence must be an integer from 1 through 9007199254740991; points must be finite; move/aim length must not exceed 1.01; power must be from .1 through 1; target length must not exceed 160; destination must be within the displayed pitch. True cancellation requires action none, no destination or target, and zero movement. Coordinates are never clamped. Epochs use the accepted command safe-integer boundary, including negative/zero values; opening's epoch is separately nonnegative.

Reset empties latest movement and the action queue, clears dirty and preserves neutralPending when movement or an unconsumed action was sent/reserved. It does not reset sequence. The neutral branch precedes ordinary rate and in-flight guards, so a higher-sequence cancel can overtake an action. Preparing it clears neutralPending, sentMovement and actionSequence. The neutral and ordinary channels have distinct receipts. Older completions cannot retire a newer neutral request.

Acknowledgements require exactly finite sequence, revision and epoch fields. They advance the local sequence only for the captured query generation, typed match/player scope and matching epoch. They do not count as action consumption; the observed player lastSequence does that. Received numbers stay broad finite doubles, while the next outbound packet must pass safe-integer validation.

`SetActive` and `SetForeground` are separate. Active changes reset input and attempt a flush. Foreground loss deactivates input, resets, attempts neutral flush and requests release; foreground return may request a nonrelease pulse. `Stop` resets and flushes without inventing a match command. These methods synchronize the pump, but do not emit a match-data notification.

Accepted scope replacement resets sequence to the new current player's lastSequence, clears actionSequence and send timing, and drops old neutral state. Phase replacement resets input and neutral movement state without inventing a new epoch. The original epoch/revision/advancement ordering runs first, so ignored snapshots do not reset input. Command/open begin resets input and stops pumping; command failure retains active state, while ordinary read/input/nonrelease-pulse failure deactivates input and uses disconnect behavior. Matching command/open finally synchronizes the pump after clearing busy.

## Pulse scheduling and NPC fallback

The pump runs only with match observation, playing eligibility and at least one of foreground, active input or neutralPending. Its tick attempts input first and pulse second. Immediate focus/input calls can prepare input without observers, as in the canonical store; a match listener is only a pump prerequisite.

Nonrelease pulses require foreground, playing and at least 200 ms since the preceding pulse preparation. Present advancement owns nextAt, including authoritative null. Only absent advancement invokes the read-only equivalent of `footballNextAdvanceAt(match, [], match.simulatedAt)`.

The fallback includes restart and match deadlines; ball velocity, height and vertical speed; connected-player velocity; overlapping connected players; a nearby claimable-height unowned ball; and NPC wakeups. It intentionally has no local timed-input rows because the canonical call supplies an empty array. It does not add a claimAfter delay that the canonical due predicate lacks. Settled playing state returns its finite deadline or no due time. Restart without either deadline remains infinitely delayed.

NPC wakeups preserve identity-derived role, lane and anticipation; keeper targets; carrier wakeups; teammate support; bot-only chaser selection with defender bias; ordered defensive threats and assignments; bounded target calculation; off-ball separation; challenge distance/cooldown; and keeper/outfield arrival thresholds. They use the first 12 connected players in source order. Duplicate IDs retain the canonical map's last calculated target/action for final lookup, rather than imposing a new read validator. The identity seed uses UTF-16 units and unsigned 32-bit arithmetic. NPC ties use native case-sensitive lexical comparison, distinct from open-roster collation. A bot carrier always requires advancement, so its detailed shot/pass choice cannot change this predicate and is not implemented. No generated NPC intent reaches a transport or modifies the snapshot.

Pulse staggering preserves connected nonbot player order. The current player's index is its ordinal, defaulting to zero if absent. Current advancement owners use nextAt directly. Other peers use `max(nextAt, ownerUntil or 0) + ordinal * 200 ms`. The decision compares that due time with client Unix milliseconds plus serverOffset. Only a current pulse/release reply with advancement updates the offset, from serverNow minus its captured receive time, before snapshot ordering/application. Query, open and command replies never establish offset.

Only one pulse/release occupies the pulse channel. A release requested while it is occupied sets releasePending. After the matching completion retires that packet, one release can be prepared using the current owner state. A successful ownership reply received after foreground loss also sets releasePending after checking post-notification ownership. Release failures and malformed release responses do not set the disconnect error. Nonrelease failures do.

## Teardown and staging limits

Last-observer loss and `Dispose` deactivate active/foreground state, reset input, prepare eligible neutral/release work and invoke the synchronous final-effect window before invalidation. Final candidates are one-use receipts and contain exact captured JSON. The window freezes other state mutation, including same-ID reset, query replacement and recursive disposal. After it returns, unsent final candidates are discarded, control callbacks retire and pumping stops. Last-observer loss preserves the displayed snapshot and independent entry observers. `Dispose` clears everything and delegate bindings without a match-data notification.

If no final dispatcher is bound or external permission has already expired, no final network send occurs. A release waiting behind an in-flight pulse cannot be manufactured as a second simultaneous pulse. Teardown retires that deferred release; a late ownership reply cannot acquire a new receipt after invalidation. Foreground loss without teardown retains normal coalescing. Runtime integration must establish the final-dispatch boundary before relinquishing authority; this source does not guarantee a best-effort network request succeeds.

Preparation reserves sequence, queue consumption and rate time before transport marking, matching the canonical pre-await work. The adapter must drain promptly. Unsent ordinary packets are discarded on reset, and unsent neutral packets are discarded on accepted scope/phase replacement. Explicit unsent abandonment cannot synthesize a transport failure. Abandoning ordinary input resets transient input; abandoning current neutral input restores neutralPending. Reserved sequences are not reused within an unchanged session/match scope. A conservative neutral may therefore follow abandoned reserved movement/action. These staging safeguards have no direct asynchronous-transport counterpart in the JavaScript store and do not authorize automatic retries.

Opening dispatch fails closed if notification reentry replaces the query, even though the canonical immediate mutation could already have started. Its unsent receipt remains identifiable for abandonment, which releases busy while retaining manual-retry selection. A session replacement exposes none of that old work. The original command query-generation busy limitation remains unchanged: a stale command terminal callback retires its request but can leave busy=true until explicit session/selection reset, Clear or Dispose.

`ResetSession` and `Clear` destroy old-session state without sending. Where final effects are still permitted, the adapter must use the existing final-effect boundary before replacing external session authority. No expired-session callback can produce fresh work.

## Inspection and remaining integration

The original source walkthrough covered the following branches and notification boundaries, but missed the disconnected-first duplicate-ID case corrected below. These were not executed tests:

- Reordered roster retry; stable equal-collation ties; changed revision, epoch, instance, mode or sides; replaced requestId; duplicate/invalid roster and request bounds; open without match or observers; ambiguous/malformed failure; query-stale success; busy final notification starting newer work.
- Queue saturation, observed action consumption, retained target/aim during renewals, cancelActions absent/false/true, invalid destination/power/sequence, neutral overtaking, stale/malformed acknowledgements, scope/phase replacement, empty error text and active/foreground separation.
- Advancement absent versus null nextAt, carrier/keeper/chaser/defender/challenge wakeups, overlap and ball-claim thresholds, restart/deadline handling, ordered peer staggering, owner exemption, current-pulse offset, release coalescing and release failure.
- Command begin/failure/finally, older leave/end success, completion-alias reassignment during notification, same-ID reset, independent entry observers, final-dispatch reentry, last-observer loss and disposal.

### Connected-player correction

The parent Spec review of the range ending at `a9d0c144e87c826d4a902485d3e9875578cd951e` found one P2 discrepancy. `Playing` found the first matching ID through `CurrentPlayer`, then checked that row's connected flag. The canonical store searches for ID and connected together. Since the decoder accepts duplicate IDs, a disconnected first match followed by a connected match incorrectly disabled native input, neutral flushing and pumping.

Correction `0b828eace1512ef92c379b66768e28832efc9831` changes only `Playing` to search for any exact matching connected row. Source inspection traced disconnected-first/connected-later and the reverse order, all matching rows disconnected, a connected nonmatching row, missing playerId and a present empty playerId. All existing ready/error/busy/match/phase guards remain intact. Input, flush, dispatch, pulse and pump consumers retain their existing additional guards.

`CurrentPlayer` and all three remaining calls are unchanged. Sequence initialization on scope replacement, observed actionSequence clearing and queued-action consumption still use the first matching player's lastSequence, regardless of connection, as the canonical store does. The decoder, header and every source byte outside `Playing` match the correction preimage. Optional duplication cleanup was not included.

The parent reported zero hard Standards breaches and three optional duplication judgments in the original review. Parent-owned re-review must cover the complete original three-path range through this corrected handoff, not only the correction commit. All transport, timer, final-dispatch, locale, compilation, Windows and runtime limits below remain unchanged. The diagnosing-bugs skill guided this source-only correction audit; its executable reproduction and regression steps were skipped under the assignment's explicit restrictions. No remote operations ran during the correction.

The full source diff was inspected and scoped `git diff --check` passed. Only owned paths were checkpointed, with hooks disabled. The writing-for-agents skill separated the adapter sequence, lifecycle rules and acceptance gaps; unslop guided the handoff wording.

No tests, lint, typechecks, builds, compilation, code generation, asset work, launches, installs, remote status/engine reads, remote writes, deployment or live configuration ran. Research was limited to official Epic API documentation. No production renderer or mobile input changed; neither renderer packaging nor What's New is required.

Parent-owned independent Standards and Spec reviews must cover the complete three-path range from `cfa15652c52e510aac532a8cc4e13ce633d8415e` through the final handoff commit. Native transport/admission, final-effect adapter, actual listener and timer ownership, input binding, culture/locale compatibility, compilation and in-game inspection remain unverified. Movement, collisions, animation, AI navigation and match simulation remain with their separately assigned native/runtime owners. Every account/readiness gate and world-control interlock remains unchanged and disabled.

The parent reports the prior command/Racing batch completed at Windows `e3b840164ee65231b1ae083cae0a9c649bf14cb5`. It was not replayed or independently read this turn. This lifecycle source has not been delivered to Windows. Source implementation, independent acceptance, Windows delivery, compilation, in-game verification and release are separate stages.

## SHA-256 records

Source preimages at `cfa15652c52e510aac532a8cc4e13ce633d8415e`:

```text
09cfe09acdf1c51665cc039e0af3255dffbed8940214222ba8b15cef836d5062  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionFootballState.h
0ec9919bb4633680562992ce542df67102f4dc17e11890600a2484e7a7e83bc2  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionFootballState.cpp
```

Original source outputs at `e766d0eee5508526f88708d7272409266cfe8839`, also the correction preimages:

```text
692bd9266380e4690ac92a949e4c7cb23e0c40272e699d33ca36a22e805a911e  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionFootballState.h
4712be5abee5dd36265a5eb06277afdd71fbc795a913f35f10a0e9dacac295a8  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionFootballState.cpp
```

Corrected source outputs at `0b828eace1512ef92c379b66768e28832efc9831`:

```text
692bd9266380e4690ac92a949e4c7cb23e0c40272e699d33ca36a22e805a911e  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionFootballState.h
13b40d63a9fb245aa42826275b4b67511ed1518e5368fff23b2024158dab5475  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionFootballState.cpp
```

Handoff correction preimage at `a9d0c144e87c826d4a902485d3e9875578cd951e`:

```text
6779fbfee073d710bd60d8b9a6f0726cecd1c7a4e7165b6f9e9f8a825c6007d0  docs/engineering/unreal-football-lifecycle-handoff-2026-10-11.md
```

Unchanged references and dependencies at the same canonical base:

```text
7790b96fdf23d66d84af08ac6c8033efe4145f40eaa6c6009f8c0dd99844343a  packages/api/src/companion-football.ts
979b865a71525b4602c3e486ad9b6175f14221d26da05953a407b58c0e4a033f  packages/utils/src/companionFootball.ts
5c4852ab1f1547f380ec6f475e049b65186a9d57443c1a97a3c9251c203fdde4  packages/utils/src/companionFootballEngine.ts
9b930929488f251b983c3854269c70ea764d7ffe285ca12698a70785e09892aa  packages/utils/src/companionFootballNpcs.ts
58634a775a8d05cc3b22224096f5a55acdfc4bbea3d02b355c3f91e410db0930  apps/backend/convex/companionFootball.ts
1e78be4330d2d753170ef782cf9dc69f7b6f37b1d787c57feb6e5e76a381ff2f  apps/backend/convex/lib/companionFootballModel.ts
13ceb1b13bee4b0793ddb117e03559bfb8ea77f8bdb8dc81baa1ffa8e70bb618  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.h
df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.cpp
536bf39a4089acea43be36448ad3d02d3cd7f94a65015d3975f761941b802328  apps/companion-unreal/Source/BaddieCompanion/BaddieCompanion.Build.cs
ec55f12339406b532d185c3e04ed6e82e23afb734cc6f480cb9cbeea5534f051  docs/engineering/unreal-football-state-handoff-2026-10-10.md
f60b41e8513be6359ee63828d724da296b3176c6f4ca60ca4b67b8b35a2b3ad6  docs/engineering/unreal-football-command-handoff-2026-10-11.md
```

The original handoff had no preimage. The correction's final handoff commit and SHA-256 accompany the completion report rather than appearing as a self-referential hash.
