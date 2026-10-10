# Unreal food and kitchen command handoff

October 10, 2026.

Source commit: `9e47f5488dc7e91d18c1253472d3167e58377ee2`.

Food and Kitchen now own local command preparation, dispatch bookkeeping, completion, failure and manual retry. Their accepted read owners remain the same classes. This is source-only implementation, with no authenticated adapter or live commands. Account access and world-control interlock remain disabled.

## Scope and pins

Assignment: `docs/engineering/unreal-command-continuation-assignments-2026-10-10.md` at `18dcf98aaaa76e3f45f2703e6b38fd0cd1ef532a`. Canonical base and owned preimages: `12270e8fe452a81b528e356a70f937c8010f17b2`.

The source commit changes exactly these files:

- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionFoodState.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionFoodState.cpp`
- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionKitchenState.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionKitchenState.cpp`

This new handoff is the fifth owned path. The original read handoff, other state owners, StateJson, tracker, backend, mobile, renderer and Windows files were not edited. Unrelated changes and staging were preserved. Commits use explicit owned paths with hooks disabled.

All 25 files in the assignment's Food and Shared groups matched both the assignment's SHA-256 pins and canonical Git bytes before editing. This included the four owned preimages. The new handoff path was absent.

StateJson dependency: the complete pair at `b4bc1b7d7dbc9552ad1451062a404fc872a13224`, unchanged at the canonical base. Existing enum and array wrappers now call `EnumValue` and `ReadBoundedArray`, retaining the 4,096-entry bound. The Food key table supplies both parsing and encoding. Quote decoding and moving-offer retention each have one implementation.

The missing shared request-ID operation was reported before implementation. `ValidFoodKitchenRequestId` lives in the owned Food pair and serves both owners; StateJson remains read-only. UE JSON facilities encode the arguments. There is no new generic JSON parser or command framework.

## Adapter sequence

These owners are single-threaded and belong to one account for their entire lifetime. The future adapter must bind each owner to fixed authenticated account context and marshal callbacks to that thread. A selected scope, read observation, `canCook`, policy approval or receipt is not permission to send.

1. Select the observed typed scope. `PrepareCommand` accepts a tagged action and a request ID, copies the selected scope and action, and encodes the request once. It rejects disposal, an active attempt, an unresolved request, no selection, invalid request IDs and invalid action tags or literal unions. Only the active action branch is serialized. It does not infer stock, contact, cooking availability, money or permission.
2. Preparation allocates a fresh private attempt GUID, sets busy, clears error and retryable, and clears Food's last result. It broadcasts, then rechecks the same attempt and, for a first send, scope equality. Reentrant `Clear`, `Dispose` or replacement cannot dispatch the retired attempt or have its busy state overwritten by the returning call.
3. Inspect the returned receipt's endpoint, request ID and non-secret JSON arguments, then perform authenticated admission separately. `IsValid` means the receipt was populated, not that it remains current or authorized. A failed preparation clears its output receipt.
4. At the actual send boundary, call `DispatchCommand` immediately before sending the unchanged captured arguments. It rechecks attempt identity, disposal, phase and first-send scope equality. A true return marks the request uncertain and sends no notification; a false return forbids sending. Do not place an asynchronous wait, account switch or callback between successful dispatch bookkeeping and the actual send. This method itself performs no transport.
5. If admission stops a prepared attempt before sending, call `AbandonPreparedCommand`. A first request is dropped without becoming uncertain. An abandoned retry retains the previously uncertain request. Never use abandonment for a dispatched attempt.
6. Deliver one endpoint value to `CompleteCommand`, or sanitized failure facts to `FailCommand`, with the same receipt. Both require a current dispatched attempt. Old, duplicate, cross-owner, cleared or disposed receipts do not mutate state. After settling an attempt, its GUID is retired before notification; reentrant work owns any subsequent state changes.
7. After an ambiguous failure, only explicit `PrepareRetry` can prepare the retained request. It uses a new attempt GUID but the original request ID and byte-identical JSON, even when selection changed or is hidden. It never merges new scope tokens or action fields. A current attempt excludes further commands and retries.

The adapter owns actual observer counts and transport cancellation. `BeginSubscription` continues to retire old read callbacks and retain presentation, for first observation or reload. `EndSubscription` retires read callbacks without discarding presentation or private pending commands. After a changed selection with observers, the adapter starts the replacement subscription. Food captures the read generation at each attempt's preparation; a reload during preparation is allowed, but makes that attempt's later result detached, matching the canonical store.

Account teardown must call `Clear` before `Dispose`, retire external callbacks, discard adapter-held receipts and snapshots, and create a new owner for a new account. `Clear` invalidates both read and command receipts even if the next scope has identical IDs. `Dispose` drops private pending state in both owners. Food also clears presentation; Kitchen deliberately retains its last presentation, including its flags, matching the canonical disposal behavior. Late callbacks cannot publish after disposal.

## Exact arguments

Every payload below includes the captured `requestId`. Auth fields are absent and belong only to the future authenticated adapter. Food scope contains `gameRoomId`, `instanceId`, `lifeId`, `revision`, `serviceId`, and `movingOfferId` only when present. Missing and present-empty remain different. A nonempty moving ID chooses moving behavior, as in the canonical client.

| Food intent | Endpoint | Other arguments |
| --- | --- | --- |
| quote | `companionFood:requestQuote` | Full captured scope, `offerKey`, `quantity` |
| confirm | `companionFood:confirmQuote` | Full captured scope, `quoteId` |
| cancel | `companionFood:cancelQuote` | `quoteId` only, no scope fields |
| collect | `companionFood:useOrder` | Full captured scope, `orderId`, `type: collect` |
| eat | `companionFood:useOrder` | Full captured scope, `orderId`, `type: eat` |
| moving confirm/cancel | `companionWorld:movingFoodCommand` | Full captured scope, matching `offerId`, `type: confirm/cancel` |

Food quantity is exactly 1, 3 or 6. Moving selection permits only confirm/cancel and requires the action's quote ID to equal the selected moving-offer ID, case-sensitively. There is no moving quote, collect or eat branch here.

For non-cancel ordinary commands, a present-empty `movingOfferId` is preserved in the full scope, as the canonical client spreads it. The ordinary backend scope validator does not declare this optional field. The adapter must source coherent selections; this implementation does not silently normalize a captured request to hide that existing client/backend edge.

Every Kitchen command uses `companionKitchen:command` with the full captured Kitchen scope, `requestId` and a nested `command` object. Optional call-session, room-revision and invitation fields retain presence, including empty strings and zero.

| Kitchen type | Command fields beyond `type` |
| --- | --- |
| serve | `batchId`, `portions` |
| cook | `recipeKey`, `batches`, `expectedRevision` |
| stop-cooking | `batchId`, `activityId`, `expectedRevision` |
| plate-meal | `portions` |
| drink | `key` |
| eat-stock | `key` |
| eat | `batchId` |

`cook` is a declared canonical command, not a new simulation operation. Batches are exactly 1 or 2. Food keys use the existing nine-key union. Recipe, batch, activity, quote and order IDs remain strings. Kitchen portions and expected revisions preserve finite numbers as declared by the client contract; backend integer/range, recipe, hunger, inventory, ownership and shared-kitchen rules remain backend admission. Selected scope revisions keep the accepted nonnegative safe-integer bounds. No local timers, nutrition, prices, expiry or cooking progression were added.

The shared request-ID validator preserves `^[A-Za-z0-9][A-Za-z0-9._:-]{15,127}$`: 16 through 128 ASCII characters with an alphanumeric first character. It does not trim, case-fold, generate or replace IDs.

## Completion and presentation

Food uses the accepted `DecodeFoodResult`, including its required nullable quote/order and existing quote decoder. Completion additionally enforces endpoint shape: moving results require a present nullable moving offer and a null quote; ordinary results omit the moving-offer field. Standalone decoding and accepted read decoding remain unchanged.

A valid Food result resolves pending state. It publishes quote, moving offer and result only when selection equals the captured scope and the read generation still equals the attempt's captured generation. Otherwise `CompleteCommand` returns `Detached` with the decoded result but does not publish it into the new view. Both paths settle busy/retryable. Food response values have no request-ID field, so the adapter must preserve response-to-receipt correlation.

Quote retention accepts a first quote, a different quote ID or the same ID with an equal/higher revision. Null and lower revisions retain the current quote. Moving-offer retention uses the same function for query and command observations: missing/null retains; a different ID or higher revision replaces; equal revision replaces only when the current status is offered or both statuses match. Terminal-to-other-status regression at equal revision is retained, while higher revision may replace it.

The catalog remains entirely query-owned. No command result inserts or updates catalog orders, carried food or offers.

Kitchen completion strictly decodes `{requestId, outcome}` with no unknown keys, a matching request ID and outcome `applied` or `replayed`. It resolves pending state and busy/retryable only. It does not replace Kitchen, Scope, readiness or a read error received while the request was in flight. Its live query owns the snapshot.

Malformed Food or Kitchen replies return `InvalidResponse`, finish the attempt as ambiguous and retain the original request for manual retry. They never count as verified success. The existing bounded JSON parser applies. No raw body or decoder diagnostic is published as an error.

## Error and pending transitions

`FFoodKitchenCommandFailure` accepts an explicit definite/ambiguous category and already-sanitized public text. The future boundary must classify with the pinned `isNetworkUnavailableError` rules, plus the canonical exact save-error message, before sanitization loses those facts. Network/service-unavailable cases and `Unable to save changes right now. Please try again.` are ambiguous; permission, session and other definite failures are not made ambiguous merely by their presentation text. Native state does not parse exceptions, raw server bodies, credentials or status strings.

Public messages are bounded to 500 characters. Empty, overlong or disallowed control-character text uses the fixed fallback. Food uses `Food action couldn't finish. Retry the same request.` and Kitchen uses `Something went wrong. Please try again.` Bounding is not a replacement for boundary sanitization.

| Event | Pending request | Presentation |
| --- | --- | --- |
| Prepared first send | Captured, not uncertain | Busy; error/result cleared as applicable; not retryable |
| Dispatched | Same immutable request becomes uncertain | Busy; no dispatch notification |
| Ambiguous failure | Retained | Busy false, retryable true; error only if original scope is still selected |
| Definite failure | Dropped, including a failed replay | Busy false, retryable false; error only if original scope is still selected |
| Invalid reply | Retained as ambiguous | Same failure/fallback handling; no success publication |
| Changed selection or hide | Retained | Reset selected presentation; preserve current busy and pending uncertainty as retryable |
| Successful read during uncertainty | Retained | Publish read data but preserve the current error |
| Successful read without uncertainty | Unchanged | Publish read data and clear error |
| Read failure | Unchanged | Accepted read fallback, ready true, query data cleared; Food quote/result/moving retention and Kitchen observed scope remain |
| Clear | Dropped; receipts retired | Empty presentation |

Food and Kitchen intentionally drop an uncertain request after a definite retry failure, following their canonical `submit`; they do not adopt Jobs' different was-uncertain rule. Read callbacks do not resolve pending state. Selection changes reset error, so a later read during uncertainty preserves that reset value rather than restoring an old error.

## Review and remaining work

Source inspection covered all five Food intents, moving restrictions, all seven Kitchen branches, payload omission/presence, literal unions, request-ID grammar, prepare notification reentry, final dispatch checks, abandoned first sends and retries, stale attempts, detached Food completion, quote/moving retention, query ownership, read-error composition and account disposal. Scoped `git diff --check` found no whitespace errors.

No tests, typechecks, lint, compilation, builds, codegen, asset import/conversion, launches, live calls, remote writes, deployment or release were performed. No mobile/runtime-renderer inputs changed, so What's New and renderer packaging were not triggered.

Independent Standards and Spec review is still the parent's acceptance step for this nonempty command scope. Prior read acceptance does not accept these commands. Windows delivery belongs to the coordinator and has not been performed here. Compilation, in-game verification and release remain separate and unverified.

The future integration still needs authenticated command admission, native endpoint/world-control coverage for these domains, observer and object-lifetime management, safe response-envelope handling and sanitized failure classification. Existing account endpoints are contract references, not native authorization. World/layout/pawn producers and Home access must supply coherent observed scopes without treating reads or wire approval flags as permission. This patch does not change any gate or solve those integration gaps.

## SHA-256 record

Source outputs at `9e47f5488dc7e91d18c1253472d3167e58377ee2`:

```text
fa232b7d2ab3f93fda11494c99d2c1a827fd570fb00b1cebd0c6c9fadee11ec6  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionFoodState.h
01a8475fea19c9c2f9abca4edfaffb3dfd03ff34a5a61f886c7ec327df7e7bf2  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionFoodState.cpp
d7ab9d3e38507a229aeba8a465bdf4bdb3bfd8f0d72a79f1cbd3cea5e4c68b41  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionKitchenState.h
916e65e230f60cc0d85775167f4d3bdde2b621827cbc0a1a0c6efd54f86e23cc  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionKitchenState.cpp
```

Owned preimages at canonical base:

```text
ed18e57d0dd2bf2cefd3d013c4947e8031465bba35b472d3aad680cda6efc11e  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionFoodState.h
70c25e9af33373661e03720b2311faf2d919b9a53bdeaf55c7c2b15d0ce99085  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionFoodState.cpp
dfe085173d4b372a72f7cb4cdf78effe274d5f7ee1a33998775a9099038d3b38  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionKitchenState.h
bf4e004bdb149d79d56fa957b1d22ad9728d8a0484486787a5d98857fa186f84  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionKitchenState.cpp
```

Read-only canonical references and dependencies. Hashes cover full files; inspection and translation used the relevant contracts, store behavior and producer/admission paths:

```text
4a0f0c144fa7c219d78e1786c1d250b2c17682606f5907f56779f3402c9f5ac2  packages/api/src/companion-food.ts
33957fe910e8e00718cb2cba95189abc231c0a9822cf2acb6283ee6a2a2bc992  packages/api/src/convex-error.ts
2738d6ad02570b0e5fe086e9fa09f05734679658def1409aa045bdef4f5b7127  packages/utils/src/companionFood.ts
85ff223aad8814d11b99eefe5c6df19b4ba19a3ee9d3235cf83aef6e522a9e62  packages/utils/src/companionGameEconomy.ts
f929b0037e76c4d0c2dd1c5e39a4e6ad2285d6284185fc9dece370e13b0d3b16  packages/utils/src/companionMovingHawkers.ts
4eef31b095b7fc8b4a4f500e048979f3706eb1403b56bec2d01d4a5220f9a7c4  apps/backend/convex/companionFood.ts
ea0856ff060390a0912aabb7627d324d8c7ad2917c2bd84db45cbfbd68585a10  apps/backend/convex/companionKitchen.ts
9f8ef66bf02c445ab688ea7410bfb6410b5c7259bc39ac7d692d0665e2ab9798  apps/backend/convex/companionWorld.ts
02504385aad7d29422dd265e54ef0f74da68519590c2e43cc1c697f3cf0ace81  apps/backend/convex/lib/companionFoodModel.ts
254976690f73cb19383d840946c5e815df1202451657c5e5c5e8303c0ddceddd  apps/backend/convex/lib/companionKitchenModel.ts
a4bb4308ee2f1611467a5832c1b7d19fa2ce857aec57ee24d99f82dbb77b3765  docs/engineering/unreal-food-kitchen-state-handoff-2026-10-10.md
90d230cf0233458c46324fd87d6f077c6f54bd7dd691030ba5a2a89e8607a17d  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionWorldPlayer.h
210d5cca522903cebb3aadcabf9206123cd067fead033bf3791b39c16833b416  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionWorldPlayer.cpp
6133331747042a7a10386953b63008be5251a7390f93083421595d86d5e28bea  apps/backend/convex/lib/companionFoodSchema.ts
5935e9dc9a7ea31103afadb854518074f9f63817050cb0cdce97f2197c798554  apps/backend/convex/lib/companionMovingHawkerFoodSchema.ts
10da44ff157adbbc3251badb987978e08a001fc538b73b4a7e2e96036878bbdc  apps/backend/convex/lib/companionMovingHawkerFood.ts
c32a5e520891feb45cbea5b1c5f530094c922d51afaa0b8a765a3d584adbdfb0  apps/backend/convex/lib/companionKitchenAccess.ts
5efe2c04a6d9d4f7cb62fd045e50a970f8d2f7be8b2f79a1fc8af9ff495c2754  apps/backend/convex/lib/companionGameEconomyModel.ts
13ceb1b13bee4b0793ddb117e03559bfb8ea77f8bdb8dc81baa1ffa8e70bb618  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.h
df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.cpp
536bf39a4089acea43be36448ad3d02d3cd7f94a65015d3975f761941b802328  apps/companion-unreal/Source/BaddieCompanion/BaddieCompanion.Build.cs
```
