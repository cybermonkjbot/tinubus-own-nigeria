# Unreal POS command handoff

October 10, 2026. Local source implementation and self-review only. Independent Standards/Spec acceptance, native integration, Windows delivery, compilation and in-game verification remain outside this handoff.

## Source and pins

Initial source commit `89a7a56470cd93b46c7c88a05d66f41e73959d4c` and correction commit `10da2a91e3475a78231865cec05900d3d03f6a80` change only the existing POS header and implementation. This record is the third owned path. It supersedes the command-deferral statements in `unreal-pos-state-handoff-2026-10-10.md`; that historical file remains unchanged.

The correction addresses the confirmed Spec P2 in the original handoff at `ae37ea421b8299a0821d7b0110b69bc20842b655`. The required quote receipt reconnect now precedes mutation dispatch, including synchronous read notifications. The prior post-send restart contract is superseded. The optional repeated-predicate refactoring remains deferred.

All three correction preimages were clean and matched their committed bytes before editing:

| Correction preimage | SHA-256 |
| --- | --- |
| `BaddieCompanionPosState.h` | `e6648db157bc2304f3ccddfc49d5f4513c54cd134538e3d2ed646d570fc24495` |
| `BaddieCompanionPosState.cpp` | `170ee770343834c5edfdc1c7e5f2c557defffb7ae42b45d3b0307a9542a835b2` |
| `unreal-pos-command-handoff-2026-10-10.md` | `38a2f1300876b2446112dee3b0f4c1b2e1ded77f83ca76f45a67670b8f1762eb` |

Assignment `docs/engineering/unreal-pos-command-assignment-2026-10-10.md` is pinned at `1cf6cf5352efd1db42a02990ea3d5457c83f78a7`. Canonical inputs and source preimages are pinned at `a7ec09694e565c25e483bbcf9cab8ee58b63707f`. All thirteen working hashes matched before editing. The eleven unchanged inputs still matched at source checkpoint.

| Source output | SHA-256 |
| --- | --- |
| `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionPosState.h` | `5ac05a861b4ab87dc2cabff85b3ad9a2fbc41fddf2ed28ccf8d3ed4ad34c304e` |
| `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionPosState.cpp` | `954f7261818e239a89de3113029f66d00fb57a94cba99671d5123bfb50d92222` |

Input and preimage SHA-256 values:

```text
f15de19b82aacdb0b46258e829e97631ce78bb7a64dd58670a0113eae397f805  packages/api/src/companion-pos.ts
33957fe910e8e00718cb2cba95189abc231c0a9822cf2acb6283ee6a2a2bc992  packages/api/src/convex-error.ts
d52ab1becf30ff0ef9cba4cd321d378c5175540fccfb4201b9d5f73a37832487  packages/utils/src/companionPos.ts
85ff223aad8814d11b99eefe5c6df19b4ba19a3ee9d3235cf83aef6e522a9e62  packages/utils/src/companionGameEconomy.ts
9056dd81d5249ea77a1e268ddb61edddc586b43bdfe0346dd0f493bba70d730a  apps/backend/convex/companionPos.ts
a6722acee2c94b0c6666f3e7546c47076da8ad2a714669dd1a5b861bdb0f6721  apps/backend/convex/lib/companionPosModel.ts
3a912b466b4e55824037b7f159904684dfe86ffa33fd0b26d51c23a76962c1b9  docs/engineering/unreal-pos-state-handoff-2026-10-10.md
11aaaa924d795803228191cc955b54bbdfff93ced7e43769d27e6388d855ef7c  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionPosState.h
a7b18c735b3ccaf930464e71628d0b92adfc942370b9e6356659e11126b7a9ab  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionPosState.cpp
13ceb1b13bee4b0793ddb117e03559bfb8ea77f8bdb8dc81baa1ffa8e70bb618  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.h
df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.cpp
536bf39a4089acea43be36448ad3d02d3cd7f94a65015d3975f761941b802328  apps/companion-unreal/Source/BaddieCompanion/BaddieCompanion.Build.cs
ed2712d780875753caebfe76e9c570ae6952cd5b4bac49ea3f69d0507711a6da  apps/backend/convex/lib/companionPosSettlementModel.ts
```

Use the complete unchanged StateJson pair from `b4bc1b7d7dbc9552ad1451062a404fc872a13224`, with the hashes above. The POS catalog now uses its `ReadBoundedArray` template with the existing amount reader, ordering and bounds. The older helper header named in the read-state handoff is insufficient. No shared helper or build configuration changed.

## Commands and immutable arguments

`FPosReadState` remains the sole receipt and command owner. It stores one immutable pending packet containing captured scope, command, original request ID, operation, serialized arguments and packet identity. A separate attempt identity and phase reserve one active submission. Uncertainty is internal state, never an argument supplied by a caller.

Scope means all five fields: `gameRoomId`, `instanceId`, `lifeId`, `revision`, `serviceId`.

| Command | Operation | Non-auth wire fields |
| --- | --- | --- |
| Quote | `companionPos:requestQuote` | Scope, `amount`, `requestId` |
| Confirm | `companionPos:confirm` | Scope, `quoteId`, `acceptsCarriedLoss: true`, `requestId` |
| Settle | `companionPos:settle` | Scope, `quoteId`, `requestId` |
| Cancel | `companionPos:cancel` | `quoteId`, `requestId` only |

The UE JSON serializer builds arguments once when preparing a new packet. Retry reuses the exact serialized bytes and original request ID; it creates only a new attempt identity. The const dispatch object publishes no credentials. A future admitted adapter supplies account authentication and must disable automatic retry and query-cache invalidation, matching the canonical mutation options.

Quote accepts any present finite double, including zero, negative and fractional values. It does not enforce the offered amount list, balances or integer eligibility. Confirm requires a present true acknowledgement; missing or false consent is rejected, never inferred from a receipt. Confirm, settle and cancel require a nonempty quote ID of at most 256 characters. Each branch rejects fields belonging to another branch. Invalid command kinds are rejected.

Request IDs match `^[A-Za-z0-9][A-Za-z0-9._:-]{15,127}$`. Scope validation retains the accepted read owner's bounds and exact case-sensitive equality. These are input checks, not admission. Backend terms, stock, source coverage, balance, utility, proximity and eligibility checks remain authoritative.

An initial non-cancel command captures selection and must still match it at dispatch marking. Cancel captures the displayed receipt's scope when its ID matches `quoteId`; otherwise it captures selection. Cancel retains that scope locally but sends no scope fields. Cancel marking does not require selection to remain equal. An uncertain retry also ignores later selection changes, including selection of null, and retains the original packet.

## Caller sequence

All methods run synchronously on one owning thread. They do not invoke listeners, start subscriptions or send requests. The caller owns notifications, external subscription retirement and transport admission.

1. Call `PrepareCommand` or `PrepareRetry` with the current owner. Preparation captures all inputs before any notification, reserves an attempt, sets busy, clears retryable and clears the error. It refuses a new command while any pending packet or active attempt exists. Read observation and catalog readiness are not prerequisites.
2. Notify listeners if required. Initial preparation and every pre-dispatch stage remain genuinely unsent and not uncertain. Retry preparation retains the previous packet's uncertainty. These explicit intermediate phases differ from canonical JS, which marks uncertainty before its busy notification. Catalog success and selection use the actual native uncertainty throughout these phases.
3. Call `BeginCommandPreDispatch` once. It rechecks owner, packet, attempt, prepared phase and the initial-scope rule. A quote with a truthy recovery ID clears that ID, retires its receipt slot, returns `bReceipt: true` and enters `AwaitingReceiptRestart`. Other commands and quotes without truthy recovery enter `ReadyToDispatch` directly. Empty recovery stays empty. This method never publishes dispatch arguments or marks uncertainty.
4. When `bReceipt` is true, retire the external prior receipt subscription and reconnect through `BeginReceiptRead` with the captured owner. Complete subscription setup and any synchronous read/error notifications before calling `CompleteCommandReceiptRestart` with the same command intent. A current unobserved owner completes this obligation without opening a subscription, matching canonical `connectReceipt`. Acknowledgement moves only this awaiting attempt to `ReadyToDispatch`. It is not proof of transport admission. A synchronous receipt error callback can complete normally and retain its read error; inability to complete reconnect setup follows the abandonment rule below.
5. After all reconnect callbacks, call `MarkCommandDispatched`. It rechecks exact owner, packet, attempt, `ReadyToDispatch` phase and initial-scope rules. On success it marks uncertainty and returns the immutable operation/arguments once. The admitted adapter must send those exact arguments immediately, without yielding, reconnecting or notifying between marking and sending. The dispatch object no longer contains a post-send restart obligation.
6. Feed current endpoint-value JSON to `AcceptCommandResult`, or a classified sanitized failure to `AcceptCommandFailure`. Success removes pending, applies existing receipt ordering, clears any truthy recovery ID, retires the affected receipt slot and always retires the catalog slot. Service receipt restart first, then catalog restart, with the captured owner and fresh authoritative time. Notify the outcome. Accepted failure and malformed response also require outcome notification. Busy remains latched through this stage.
7. Call `FinishCommand` with the same intent after outcome notification. For success it selects the current displayed receipt if its ID matches the returned receipt, otherwise the returned receipt. It then clears the attempt, releases busy, derives retryable from remaining uncertainty and reconciles retained subscription evidence. Notify finally, then deliver the saved result. A catch or malformed response returns null even if finally reconciliation discovers the outcome.

Read restart flags are obligations, not permission or subscriptions. The existing `Begin*Read` methods recheck owner, observation and selection. Use the owner captured for this operation, not a newly fetched replacement owner. If callbacks replace the account, old restarts and old finally must not touch the replacement. A stale success is rejected locally rather than publishing a retired account's returned receipt.

If reconnect setup cannot complete, do not acknowledge it or mark dispatch. Retire any incomplete external subscription and call `AbandonUnsentCommand` with the same intent. A current read intent can report a sanitized receipt-read failure through the existing read API; never call `AcceptCommandFailure` for a mutation that was not sent. Notify the resulting state after abandonment. An account replacement makes the old acknowledgement and abandonment no-ops, so they cannot release newer work.

Abandonment is valid in `Prepared`, `AwaitingReceiptRestart` and `ReadyToDispatch`, including after a failed final scope check. Initial abandonment removes the unsent packet and releases busy without inventing a network failure or restoring an earlier error or recovery ID. It does not roll back read data or erase recovery changes made by callbacks. Unsent retry abandonment retains earlier uncertainty, releases busy and can reconcile existing receipt evidence. Once marked, an attempt cannot be abandoned as unsent; it needs a result or classified failure followed by finally.

Duplicate pre-dispatch entry returns false with no restart flags and cannot clear recovery or retire a read again. Duplicate acknowledgement, acknowledgement for a non-restart path and acknowledgement from an earlier attempt are rejected. Marking before required acknowledgement is rejected. A callback may change selection, replace the account, abandon the attempt or start later work; the final mark always rechecks ownership and scope. Initial non-cancel scope mismatch refuses dispatch, while uncertain retry and cancel retain their existing exceptions. A new recovery selection made by a reconnect callback is preserved until the normal success path, not reset again at mark.

## Failures and response boundary

The adapter must classify the original failure before sanitizing its public text:

| Failure kind | Classification | Pending packet |
| --- | --- | --- |
| `NetworkUnavailable` | Canonical `isNetworkUnavailableError` or a faithful transport equivalent | Retain as uncertain |
| `GenericSaveFailure` | Error-instance message exactly `Unable to save changes right now. Please try again.` | Retain as uncertain |
| `Definite` | Neither ambiguity predicate matched | Remove |
| `Unclassified` or invalid enum | No accepted classification | Reject without changing the dispatched attempt |

Do not classify by the sanitized message. `FPosPublicFailure` accepts already-sanitized text from `toSafeConvexError` or its faithful equivalent. The reducer only bounds it to 4,096 characters. Empty or oversized text uses `The POS result is unknown. Retry the same action.` Raw exceptions, credentials and response documents are neither retained nor logged.

A current sent result must pass the existing bounded receipt decoder and be nonnull. Malformed JSON, an invalid receipt or root null cannot prove the mutation had no effect. They retain the original uncertain packet, set `The POS response is invalid. Retry the same action or reload the receipt.`, and move to the same outcome/finally stages. This is an explicit native response-boundary rule. It does not synthesize a receipt or reinterpret the failure as definite.

A valid command result has no additional command-ID or scope filter beyond the accepted receipt decoder and ordering. It may leave a newer displayed receipt in place. The successful return value remains distinct from that display. Success does not directly clear a read error raised during the request; canonical catalog/selection behavior still controls that error.

## Reconciliation and lifecycle

Reconciliation requires an actual uncertain packet, no active attempt and a displayed receipt. It runs after valid receipt observations, including observations whose incoming receipt ordering retained the prior display, and after finally or unsent abandonment releases busy.

| Pending command | Evidence in displayed receipt |
| --- | --- |
| Quote | Exact request ID, all five scope fields and amount |
| Confirm | Exact quote ID and status other than quoted |
| Settle or cancel | Exact quote ID and terminal status, neither quoted nor pending |

Known evidence clears pending, uncertainty, retryable and error. Unrelated evidence does not. Evidence arriving during preparation, dispatch or outcome notification waits until busy release. After a definite failure removes pending, receipt evidence cannot clear that failure through reconciliation.

- Default, foreign, retired-owner, wrong-attempt and duplicate intents cannot mark, complete, abandon or finish current work. An unmarked result/failure is rejected. Repeated marking and finally are rejected. Rejected operations only clear their output arguments, not owner state.
- `BeginOwner`, including same-ID replacement or invalid account input, and `Clear` retire reads, packet, attempt and saved returned result, and clear private values. `Clear` permits a fresh owner. `Dispose` is terminal. Old completion/finally calls cannot release a replacement attempt's busy latch.
- Changed selection retires only catalog ownership, resets catalog/readiness, preserves account receipt and pending packet, reports busy from the active attempt and retryable from actual uncertainty. It retains errors only while pending is uncertain. Unchanged selection remains a no-op. Invalid selection does not mutate state.
- Catalog success clears errors only without uncertain pending work. Catalog failure/malformed data clears catalog and sets readiness/error. Invalid captured time preserves catalog and sets the timestamp error. These read errors do not clear pending work.
- Receipt failure/malformed read retains display and pending, replacing only the public error. Stale reads and catalog-scope or by-ID mismatches are ignored. By-ID null remains a mismatch. Valid latest null retains an existing receipt or establishes authoritative null if none exists.
- Recovery changes retire only the receipt read. Unset and empty recovery remain distinct for read identity, while both select latest rather than by-ID. Reads and reconciliation do not themselves clear recovery. Only a truthy recovery value is reset by quote pre-dispatch entry or command success.
- Last-unsubscribe and reload retire read slots without clearing a packet or active attempt. The caller still owns external subscription cancellation and restart. A command can complete while unobserved; read begin then refuses to issue a subscription intent.

Receipt ordering is unchanged across quoted, pending, completed, cancelled, interrupted and expired. Null cannot erase an existing receipt. A different earlier receipt is retained unless explicitly recovering that ID; equal-time different IDs are accepted. The same ID cannot regress pending to quoted or change a terminal status, but the same terminal status may refresh fields. No new same-ID timestamp or scope-revision monotonicity rule was added.

No reducer method resumes across a caller notification. Subsequent stages recheck their sealed owner/packet/attempt identity. Success retains busy even after pending is cleared, preventing a new submission before finally.

## Remaining producers and verification

This source provides local command ownership only. Native admitted account transport, response-envelope handling, faithful failure classification/sanitization, authoritative time and world/service selection producers, caller notifications, subscription wiring and mounting remain absent. No UI or live call was added.

Settlement remains an explicit request intent. The existing world owner would schedule readyAt and expiry wakeups. There is no timer, retry pump, automatic settlement, stock/debit/ledger implementation, cash conversion or payment integration here. Presence, selection, catalog values and completed receipts grant no withdrawal, world-control or account-payment permission.

Self-review traced the complete source diff against the pinned command/store contracts, including owner replacement after notifications, scope changes before marking, selection-independent retry/cancel, result ordering, ambiguous/definite failures, malformed responses, read evidence during busy and duplicate finally. Scoped `git diff --check` passed and input/output hashes were inspected. These are source checks, not runtime evidence.

Correction review traced the added phases against canonical `companion-pos.ts` lines 75-81. It covered required and absent reconnects, synchronous receipt/error callbacks, changed scope and account during reconnect, duplicate entry/acknowledgement, stale acknowledgement after abandonment/retry, early marking, initial/retry abandonment after setup failure and retained read evidence during the new busy stages. StateJson and canonical store hashes still match their pins. Payload encoding, status ordering, completion/failure handling and finally return selection did not change. The corrected three-path range needs the same independent reviewers to recheck it.

No tests, typechecks, lint, builds, compilation, codegen, generators, imports/conversions, installs, launches, remote operations or in-game inspection ran. Production renderer/mobile inputs, account/readiness gates, world-control interlock, configuration and prior slice files were not changed by this patch. This unmounted native slice does not trigger renderer packaging or What's New changes.

The parent owns independent Standards and Spec review of the complete three-path range, integration and acceptance. The coordinator owns Windows delivery. This handoff does not claim delivery, runtime correctness or release completion. Unrelated working-tree changes remain outside these commits.
