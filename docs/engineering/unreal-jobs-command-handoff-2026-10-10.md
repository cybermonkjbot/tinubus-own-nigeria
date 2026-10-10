# Unreal courier command ownership

October 10, 2026. Source implementation and source/diff review only. Commands remain unmounted. Account access and the world-control interlock remain disabled.

Source checkpoint: `ca06a0789fc13eda1216d4fec41e604d56085aef`.

## Scope and dependencies

This checkpoint extends the existing `FJobsReadState` in exactly two files:

- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionJobsState.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionJobsState.cpp`

This new handoff is committed separately. The assignment is `docs/engineering/unreal-command-continuation-assignments-2026-10-10.md` at `18dcf98aaaa76e3f45f2703e6b38fd0cd1ef532a`. Canonical sources and owned preimages are pinned to `12270e8fe452a81b528e356a70f937c8010f17b2`. The assignment and owned HEAD preimages still matched those pins before checkpointing.

The complete StateJson pair at `b4bc1b7d7dbc9552ad1451062a404fc872a13224` is required. JobsState reuses its exact comparisons, readers, parse bounds and null parser, plus UE JSON serialization. No shared helper, other state pair, old handoff, configuration, backend, mobile or tracker file was edited. These standalone native files change no production renderer input or mobile behavior, so renderer packaging and What's New changes do not apply.

## Future adapter sequence

The owner runs on one game thread. Before implementing its adapter, read the sources/lifecycle and decoder sections of `docs/engineering/unreal-jobs-state-handoff-2026-10-10.md`. That record remains authoritative for unchanged read behavior. Its statements that commands, retry and goal-save ownership are external describe the earlier checkpoint and are superseded here.

1. Supply coherent current world, recovery and haul observations using a current source receipt. Rejecting a source receipt must never lead to handling a response against older observations. The host owns first/last-listener observation and actual subscriptions.
2. Call `PrepareCommand` with action and identifier only, or `PrepareRetry` for the retained command. Success claims the attempt, clears command error/feedback and sets busy. Notify listeners outside the reducer.
3. Refresh coherent sources after notification. The authenticated adapter must separately admit the operation. At its immediate dispatch boundary, call `MarkCommandDispatched` with the returned receipt. Dispatch only when it returns true, using the returned endpoint and argument JSON unchanged. No notification or callback may intervene between marking and dispatch. Neither this receipt nor a ready snapshot establishes permission.
4. If preparation is abandoned before dispatch, call `CancelPreparedCommand`. A failed current mark already releases that attempt. Notify the resulting state. Cancellation cannot erase an earlier uncertain request, and a never-dispatched preparation remains non-uncertain. Once marked, use the response/failure path rather than cancellation.
5. For a response, refresh sources and call the three-argument `ObserveMutation`. For a failure, refresh sources and call `ObserveMutationFailure` with sanitized public text and explicit failure categories. Matching completion releases busy even when scope is stale. Notify the resulting state unless disposed.
6. After the final notification and any resulting source updates, call `TakeCommandResult` on the completion ticket. It clears that ticket and returns a detached result only if visit, contact, job error, job progress and ended-state checks still allow feedback. Copying a ticket is not a second submission or an authorization mechanism.

`FJobsDispatch` exposes copies of only the endpoint and non-secret arguments. Payloads still contain account-private life/outing/job data and belong only in the authenticated account adapter, never peers or renderer state. The reducer contains no transport, credentials, delegates, scheduler or automatic retries. The adapter must retain request callbacks until they finish or explicitly dispose the account owner.

Goal selection uses `PrepareGoalSelection`, external busy notification, refreshed sources, and `MarkGoalDispatched` in the same order. `CancelPreparedGoal` ends an undispatched preparation. Complete a dispatched goal through `ObserveGoalMutation` or `ObserveGoalMutationFailure`, then notify. It has no detached command-result ticket and no automatic retry.

## Intent branches and captured arguments

Every new command requires an enabled observation, unblocked owner, no active command or goal save, and no unresolved pending command. A pending command produces `Retry the pending courier action first.` The current context must satisfy the existing can-work predicate, job readiness and no truthy job error. Otherwise preparation reports `Wait for your current world, life and haul to load.`

Can-work remains present/living player, active/living recovery, carried outing, and neither world nor recovery busy. Contact resolution remains the existing exact current-layout/rendered-instance/service predicate. It does not establish NPC availability, reach, route clearance, trust or backend permission.

| Intent | Additional preparation requirements | Captured action fields |
| --- | --- | --- |
| Accept | First matching offer, ready/error-free offers, available, exact employer contact, approved reward, no accepted/carrying job | `type: accept`, `offerId` |
| Collect | Matching current accepted job and exact employer contact | `type: collect`, current `jobId` and `jobRevision` |
| Deliver | Matching current carrying job and exact destination contact | `type: deliver`, current `jobId` and `jobRevision` |
| Cancel | Matching current accepted/carrying job; no contact required | `type: cancel`, current `jobId` and `jobRevision` |

Accept failures retain the canonical messages for unavailable offers, unavailable rewards and an existing active job. Other stage/ID failures report `This courier job has changed.` Collect/deliver contact failure reports `Visit this job's current contact.` No local stage, reward, availability window or trusted-courier progress is calculated.

The endpoint is `companionJobs:command`. Arguments contain the selected `gameRoomId`, `command`, and original `serviceId` except for cancel, which omits service entirely. Each command captures current `observed.instanceId`, `worldRevision`, `lifeId`, `outingId` and one request ID. Original contact remains captured even for cancel because feedback still uses contact equality. Intent callers cannot supply replacement concurrency tokens.

The owner serializes arguments once and retains that string. Manual retry keeps it byte-for-byte, including action, tokens, service and request ID. A fresh attempt receipt captures the retry's current generation, instance/revision/status/render attempt, contact generation and job-query count. It retains the original contact for feedback. Retry requires matching account/user/room/life/outing and can-work, but does not revalidate offer availability, current stage, contact or job-read readiness/error.

## Pending identity, failure and teardown

Exactly one unresolved command is retained. Preparation starts it with uncertainty false. Only a successful `MarkCommandDispatched` sets uncertainty true. Each prepared attempt also captures whether uncertainty was already true before that attempt.

| Event | Pending identity | Current feedback/error |
| --- | --- | --- |
| Cancelled or stale preparation before dispatch | Retain unless an independent scope-ending event cleared it; preserve prior uncertainty | No fabricated transport failure |
| Ambiguous failure or malformed response | Retain original uncertain request | Canonical retry message only when feedback is current |
| Definitive rejection of first dispatched attempt | Clear, provided current scope/visit still match | Sanitized rejection text only at current contact |
| Definitive rejection after an earlier ambiguous attempt | Retain original uncertain request | Sanitized rejection text only at current contact |
| Valid matching response | Resolve even if contact changed, job error exists or a newer job is retained | Existing ordering and feedback checks decide publication |
| Stale dispatched completion | Do not resolve a retained request or affect a newer attempt | Release only its matching busy attempt |
| Current-scope access failure | Clear and revoke | Clear view/read receipts and retain bounded access error |

The ambiguous/malformed message is `The courier result wasn't received. Retry the same action.` Returned-player mismatch is stale, not verified success, and leaves the uncertain request unresolved.

Access failure uses scope validity even when the visit changed. Ordinary rejection requires the captured visit; error presentation additionally requires the original contact and contact generation. `FJobsPublicFailure` does not inspect raw bodies. The future adapter must use the pinned `toSafeConvexError`, access classifier and network/details/status rules. Definitive rejection excludes classified network ambiguity and requires an error-details object containing `data`, or a 4xx status other than 408. A retry's rejection cannot establish failure of its earlier ambiguous attempt. Public text remains capped at 8,192 characters with fixed fallback text.

Reload, last-observer teardown and temporary unready/error snapshots retain private pending identity. Detention changes the read context and retires the attempt's generation, but does not itself prove a new life. Pending clears when a ready/no-error source proves one of these changes:

- World has no player, a different user/room or nonpositive health.
- Recovery has no health, a different life, hospitalization or nonpositive health.
- Haul has no outing, a different outing or a non-carried outing.

A valid reconstructed context with a different account/user/room/life/outing scope also clears pending. So does an accepted job update reporting failure or a reward no longer carried.

Selecting a different account or room clears pending. Same identifiers remain a no-op. Account replacement, `Clear` and `Dispose` release owned command/goal attempts and drop their private state. Replaced credentials or account stores with the same IDs still require disposing the old owner. Room change and last-observer removal retire view receipts but preserve the active-attempt exclusion until its matching callback or prepared cancellation. A cleared goal view can therefore show goal busy false while the private stale goal attempt still excludes new work, matching canonical teardown presentation.

The legacy `CaptureMutation` and two-argument `ObserveMutation` remain available. They share result application with owned commands and retain their earlier externally owned receipt lifecycle. They cannot overlap an owned pending command or goal save. Legacy capture creates no encoded payload and cannot use the new mark boundary; the host remains responsible for that external request. Disposal clears legacy receipts as before.

## Response ordering and saved goals

The existing decoder and `ReceiveJob` remain the single application path. Every response still needs a matching returned player user/room/instance/revision/status. It never publishes a world-player update. A truthy job error suppresses job application and feedback without preventing resolution of an otherwise valid pending request.

Equal-revision command results retain the query-owned job, including changed remaining reward amounts and banking/forfeiture. Lower revisions and null results cannot replace an existing job. Different-job results respect acceptance time and intervening job-query counts. Non-blocked feedback requires matching current progress and a non-ended job; blocked feedback may refer to the retained or null job. The post-notification ticket check also suppresses a result if a listener changes contact, room, generation, job error or ledger progress.

`bRetryable` composes retained pending identity, no active command, matching scope and can-work. Main error precedence remains command, job, offers by optional presence. Empty error strings remain false for truthy guards. Goal error stays separate.

Goal selection requires no command/pending/goal attempt, a ready goal without a truthy goal error, its resident, and a current player in the selected room. Selecting true also requires `inCurrentWorld`; selecting false permits deselecting the saved resident outside the current world. It captures resident `actorIdentity`, the explicit selected boolean and room for `companionJobs:selectGoal`.

Goal completion validity uses owner generation and account/room/goal-user key. It does not use goal-query generation or replace the captured actor when a query changes resident. Life/context teardown or reload can retire the owner generation. A successful response must decode as JSON null and never replaces saved goal state or clears a query error. Invalid response sets a fixed goal-save error. Current failure revokes access or stores bounded public goal error. Every matching dispatched completion releases its own goal-save exclusion, including stale completion. The goal query alone publishes selected state and progress.

## Native differences and review status

Request IDs use `courier-` plus a hyphenated GUID instead of the JavaScript timestamp/random-prefix/counter generator. They satisfy the canonical ASCII pattern and 100-character maximum and remain unchanged across retries. Each native intent must match a validated offer/job ID. Invalid enum values are rejected. Command capture also rejects world/job revisions at `MAX_SAFE_INTEGER`, following the backend command limit of one less, and rejects unsupported instance/life keys above 160 characters or with empty trimmed content. Source strings are preserved, not normalized. These capture checks do not change read decoding.

JSON serialization failure leaves no dispatched attempt. Goal null parsing uses the shared parser with a 64-character, depth-two, four-separator preflight. Oversized or malformed replies fail rather than resolving as success. Existing command-response and read parse limits remain unchanged.

Source/diff review covered every intent and rejection branch, first/repeated preparation, dispatch reentry after reload/selection/disposal, cancelled uncertain retry setup, scope versus visit/contact failures, malformed and mismatched responses, query-owned equal-revision reward changes, newer job retention, delayed contact feedback, final-notification invalidation, transient source loss, detention, proven life/outing changes, and stale completion after account replacement. Goal review covered selection/deselection, resident changes, query errors, null/malformed responses and stale busy release. No remaining issue was identified by this source review; it is not independent acceptance or executable validation.

Parent-owned independent Standards and Spec review of this nonempty pinned scope remains required. The authenticated native adapter, coherent bounded world/layout/render producers, listener notifications, account lifetime wiring and controller/HUD binding remain unimplemented. Read observations and locally encoded endpoints provide no new readiness or access. Backend authority retains NPC reach/trust, command admission, parcel transitions and reward settlement.

No tests, typechecks, lint, builds, compilation, codegen, packaging, launches, imports, asset conversion, installs, live calls, networking, configuration, remote transfers or release ran for this continuation. Windows delivery belongs to the coordinator. Compilation, in-game verification and release remain separate and unverified.

## Consumed SHA-256 records

All consumed references below matched the assignment pins. The extra `companionWorld.ts` read confirmed the actual command response and goal-null return at the re-exported endpoints. The old courier-progress file was only searched/hashed during initial inspection; no implementation was reused from it.

| Reference | SHA-256 |
| --- | --- |
| `packages/api/src/companion-jobs.ts` | `a0f2c51dd102cd953291f5979a12228d4a1a2225c4004b7db439815c41c80c47` |
| `packages/api/src/convex-error.ts` | `33957fe910e8e00718cb2cba95189abc231c0a9822cf2acb6283ee6a2a2bc992` |
| `packages/utils/src/companionJobs.ts` | `9fb5f4f6ba25202b133a0dccf0452e39c2a920a10f057233c7470b69341cc408` |
| `packages/utils/src/companionWorld.ts` | `05b2d4f854ff8e54a8233af45b87f159cd8e90efbd552cbe5a98029b7076e1ba` |
| `packages/utils/src/companionHaul.ts` | `2e729e6abeba8e28c7b7d328f985451162d8ba036174da86636adac0c9f02ab4` |
| `packages/utils/src/companionRecovery.ts` | `e4ec7d77076df9713fa4177e4abc0b71968231d8f4538660665652c9a2882d79` |
| `apps/backend/convex/companionJobs.ts` | `dde5d33ec3a7edc048093db3c756182034db0c7cac468fc11a6b59faf2771608` |
| `apps/backend/convex/lib/companionJobsModel.ts` | `dbef808f04752fabe16b5589c2da9b0bf1a9045f187e3f7232d9f72d8617d8b1` |
| `apps/backend/convex/companionWorld.ts` | `9f8ef66bf02c445ab688ea7410bfb6410b5c7259bc39ac7d692d0665e2ab9798` |
| `docs/engineering/unreal-jobs-state-handoff-2026-10-10.md` | `c207d9022adaf05076c5dae8b36d8630c7ebb8f47b51073509c75cd8e6767e85` |
| `Public/BaddieCompanionWorldPlayer.h` | `90d230cf0233458c46324fd87d6f077c6f54bd7dd691030ba5a2a89e8607a17d` |
| `Private/BaddieCompanionWorldPlayer.cpp` | `210d5cca522903cebb3aadcabf9206123cd067fead033bf3791b39c16833b416` |
| `Public/BaddieCompanionRecoveryState.h` | `2c874a564ef777217630435549a1f10d36ac7215d509e59c82b48122326d77c7` |
| `Private/BaddieCompanionRecoveryState.cpp` | `e47db6550d6b173aaa8e24dda2ddb7cbb5f18893cd83375b7b2084911059562a` |
| `Public/BaddieCompanionHaulState.h` | `3414c83fe57ca20724d0362efda073503e664c14b8724668c46888401a011869` |
| `Private/BaddieCompanionHaulState.cpp` | `8768b4fcaaacc7db44e29e2085547a03955ebaa134f60a58a91f87af03525f57` |
| `Private/BaddieCompanionStateJson.h` | `13ceb1b13bee4b0793ddb117e03559bfb8ea77f8bdb8dc81baa1ffa8e70bb618` |
| `Private/BaddieCompanionStateJson.cpp` | `df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce` |
| `BaddieCompanion.Build.cs` | `536bf39a4089acea43be36448ad3d02d3cd7f94a65015d3975f761941b802328` |

Native shorthand paths above resolve under `apps/companion-unreal/Source/BaddieCompanion/`.

| Owned file | Canonical preimage SHA-256 | Source checkpoint SHA-256 |
| --- | --- | --- |
| `Public/BaddieCompanionJobsState.h` | `5f013bb5f4403f4e638aeacd0ca18a478b2da86576195286ad2e836964811180` | `ee23fc8e4b11e810eb9401c135ff4eeff7040690001488af1ce4e4f0d487a24a` |
| `Private/BaddieCompanionJobsState.cpp` | `3037ca6a8787303b4f08e9bc27ca2153f3f1f1d1ddbada8b2ace7693910c3f1c` | `2a3eeb1bda50a079a81eab4b4161d9df41ba6739783b12fcfbf31518f5dfc274` |
