# Unreal mission command ownership

October 10, 2026. Source-only continuation. Source checkpoint `4c2970f5041925684a63a2c1f7cc936a3df7f08f` implements all eleven mission command tags and explicit subscription-resume consumption. No native transport or live enablement is included.

## Scope and pins

The source checkpoint changes only:

- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionMissionState.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionMissionState.cpp`

This new handoff is checkpointed separately. The assignment is `docs/engineering/unreal-mission-command-assignment-2026-10-10.md` at `187c40dc98cfabb9b12b8aea8589e54fdb0be81d`. Canonical/preimage pin is `773b036e50b89c87b10c3509715e44e11eb08d42`. All ten input hashes matched before editing; the assignment was unchanged and the new handoff path was absent.

The complete StateJson pair from `b4bc1b7d7dbc9552ad1451062a404fc872a13224` remains required and unchanged. The source uses its existing types, limits and time predicates plus UE JSON serialization. Jobs, Economy, Play, prior handoffs, backend, mobile, tracker and configuration remain outside this patch. These unmounted native files change no production renderer inputs or mobile behavior, so renderer packaging and What's New updates do not apply.

## Adapter sequence

Before integrating the owner, read `docs/engineering/unreal-mission-state-handoff-2026-10-10.md` for its complete decoder, presence rules, source bounds and response-ordering contract. Its earlier statements that command encoding and resume ownership are external are superseded by this continuation. The existing read/response behavior remains intact.

The host owns listener counts and account lifetime. It selects an already-admitted room, captures an observation token for the subscription, and calls `StopObserving` on last unsubscribe. Every reconnect must retire the previous observation generation through `Reload` or the appropriate selection/teardown method before capturing a new token. `CaptureObservation` alone does not reconnect or reset resume eligibility. Identity/access loss requires `Clear`, including a replacement account in the same room. Dispose before destroying the owner.

For an explicit mission command:

1. Construct `FMissionCommand` from the selected tag's required values and call `PrepareCommand` with the current wall-clock start time. It captures room, JSON arguments, request serial, generation and accepted-update count. Success sets busy and clears error. It does not require snapshot readiness, infer permission or dispatch anything.
2. Notify listeners outside the reducer. The future authenticated adapter must separately admit the operation. Immediately before dispatch, call `MarkCommandDispatched` with the original receipt. It rechecks owner identity, generation, selected room, exact request serial, busy state and whether this request was already marked.
3. Send only if marking succeeds, using the returned endpoint and argument JSON unchanged. No notification, callback, yield or queued delay may intervene between a successful mark and actual dispatch. Use the canonical transport options `retry: false` and `invalidateQueryCache: false`.
4. If an owned preparation will not be sent, call `AbandonPreparedCommand` with its receipt, then notify if it changed state. Abandonment clears that preparation's busy state and captured payload without a fictional network failure. After dispatch, abandonment is rejected.
5. Complete a dispatched request through the existing `FinishMutation` or `FinishMutationFailure`, using the original receipt and a fresh received-at wall-clock time for responses. Notify the resulting state unless disposed. A stale callback is ignored and cannot clear a newer request. The reducer has no delegates or internal notification reentry.

The endpoint is `companionMissions:send`; arguments contain only `gameRoomId` and `command`. Credentials remain with the account adapter. `FMissionDispatch` returns a copy of the captured argument string, so editing the original command, pickup or selection cannot rewrite the prepared request. Default or failed dispatch outputs must never be sent.

`BeginObservedMutation` remains available for existing externally admitted requests and shares the same single-busy exclusion and completion path. It has no owned encoded command and cannot use the new mark/abandon methods. Owned preparations reject both completion methods until marked dispatched. Legacy bookkeeping remains the host's responsibility.

## Command fields and validation

The native command is a typed tag plus fields. Only fields belonging to that tag are validated and encoded; unrelated caller fields never enter or remain in the captured request.

| Tag | Fields inside `command`, in addition to `type` |
| --- | --- |
| `resume` | None |
| `start` | `missionId`, `withCrew`; optional `pickup` with `instanceId` and `revision` |
| `join` | `runId` |
| `set-ready` | `runId`, `readinessRevision`, `ready` |
| `transfer-lead` | `runId`, `revision`, `userId` |
| `choose` | `runId`, `revision`, `choiceId` |
| `begin` | `runId`, `revision` |
| `advance` | `runId`, `revision` |
| `skip` | `runId`, `revision` |
| `retry` | `runId`, `revision` |
| `leave` | `runId`, `revision` |

IDs reuse the existing exact identifier predicate: nonempty, at most 240 characters, no edge whitespace or control characters. Unknown bounded mission and choice IDs are preserved without catalog lookup or normalization. Invalid command enum values fail.

Required revisions use `TOptional<int64>` so absence differs from zero. Integer typing excludes fractional/NaN/infinite values; the encoder also requires a value from zero through `MAX_SAFE_INTEGER`. Required booleans use `TOptional<bool>` so false is valid and absence fails. A future untyped binding must validate input types before constructing these values rather than coercing strings/numbers or narrowing invalid numbers.

An absent pickup is omitted, not encoded as null or synthesized from current world state. A present pickup requires a valid instance ID and a present safe revision. The canonical union allows absence even though the current backend can reject a start without pickup. The native owner also leaves the backend's narrower revision limits and all catalog, roster, consent, proximity, equipment, objective and authorization decisions to that backend. It does not compare caller revisions with the displayed snapshot or invent replacement tokens.

Invalid command fields or serialization failure set the new `InvalidCommand` category, append-only in the existing error enum. They mint no receipt and preserve story data, readiness, clock, request counters and busy state. Ordinary disposed/busy/unselected refusal leaves the existing error unchanged. Invalid start time takes precedence over command encoding and retains `InvalidReceiptTime`, with no request or payload captured. `BeginObservedMutation` itself is unchanged.

The action named `retry` is a mission checkpoint action. There is no transport retry method, idempotency/request ID in the arguments, unresolved-request slot or ambiguity retention policy. The captured payload exists only for the current busy request and is released on completion, failure, abandonment or callback retirement.

## Explicit once-per-observation-generation resume

Use the four-argument `Observe` overload for a subscription callback and retain its `FMissionResumeObservation` output for that callback's immediate post-notification check. The receipt records that this actual, valid wire contained a run. It does not read run presence back from the retained snapshot.

For `Applied`, notify the accepted snapshot first. For `Retained`, the canonical `accept` made no state notification, but the actual older same-run observation may still request resume. Invalid observations update their existing error state and produce no resume receipt; ignored observations change nothing. A no-run response also produces no receipt. Mutation completions and subscription failures cannot create one.

Immediately after handling the observation and its notification, call `PrepareObservedResume` with that observation receipt and the current start time. It rechecks selected room, owner identity, generation, disposal, busy and the generation's resume allowance. Success prepares the exact `{ type: resume }` command and consumes the allowance. Follow the normal busy-notification, admission and dispatch-mark sequence above. The method never sends anything automatically.

| Sequence | Result |
| --- | --- |
| First valid run observation, idle after notification | Prepare resume once for this generation |
| Older same-run wire retained by `Accept` | Its actual run presence remains eligible |
| Null, invalid or stale observation | No resume receipt from that observation |
| Notification starts another mutation which remains busy | Refuse this observation's resume check; retain allowance for a later observation |
| Copied observation receipt is checked again | Shared consumption marker rejects reuse after success or busy/already-resumed refusal |
| Notification reloads, changes room, stops observation, clears identity or disposes | Old observation receipt fails current-owner/generation checks |
| A later same-generation run observation after successful resume preparation | No second observation-triggered resume |
| Reconnect/reload or identity retirement | Old receipts retire and resume allowance resets |

Each observation receipt's consumption marker is shared across copies. A foreign owner cannot consume it. Busy refusal consumes only that callback's check, preventing callers from queuing it until a different request finishes. A later actual observation can try again. An invalid preparation time leaves both the observation check and generation allowance unconsumed while setting the existing time diagnostic. The host must handle that local error, not schedule a resume loop.

The allowance is consumed on successful preparation, as canonical connect sets `resumed` when it starts `send`. Abandoning that preparation or receiving a failure does not rearm it in the same generation. Explicit `PrepareCommand` with `resume` is separate and does not consume the subscription allowance, matching an explicit canonical `send`. No polling or retry scheduler is added.

A nested same-generation observation during notification does not by itself invalidate the outer callback's observed-run fact. Each callback retains its own provenance and rechecks busy/allowance afterwards. Any identity/generation change invalidates both. The host must process receipts in callback order immediately after notification; they are not a deferred queue.

## Response and lifecycle preservation

The decoder, `Accept`, `SameRun`, revision comparison, accepted-update counter and timestamp formula are unchanged. A successful accepted read replaces the complete wire, clears error, marks ready and observes `serverTime - receivedAt`. An older same-run read retains the complete previous state and does not increment accepted updates or clear error. Busy is unaffected by reads.

Mutation results remain eligible only with no intervening accepted update, or matching optional run IDs and an incoming revision at least as new as the retained run. Two null/absent optional run IDs still compare equal. Eligible results use `serverTime - (beganAt + receivedAt) / 2`, including the existing case where an older same-run wire is retained by `Accept` but no accepted update intervened. Ineligible results cannot replace wire or clock. Invalid response/time sets its existing typed diagnostic, retains wire/clock/readiness and releases only the matching request. Transport failure sets `MutationFailed` without retaining raw error text.

Changed selection clears displayed state and retires callbacks; same selection remains a no-op. Reload retains wire/readiness/clock while clearing busy/error and resetting observation resume eligibility. Last unsubscribe retires callbacks and clears busy while retaining wire/error. `Clear` drops selection and account-private state. `Dispose` remains terminal and clears presentation data as the accepted native owner already did. Counter exhaustion remains fail-closed.

## Source review and remaining work

Source/diff inspection covered all eleven tags, optional pickup absence/presence, required false/zero values, invalid IDs/enums/revisions, exact field isolation, copied command immutability, invalid start-time preservation, prepared abandonment, unmarked/duplicate/stale/foreign completions, post-notification room/reload/disposal changes, once-only resume, retained/null/invalid observations, busy reentry and fresh-generation reset. The original mutation-order and midpoint-clock branches were compared directly with the pinned source. No remaining issue was identified in this source review; independent acceptance remains pending.

Parent owns separate full Standards and Spec review of these three paths and cross-module integration. Coordinator owns Windows delivery. The authenticated adapter, bounded live producers, listener/notification wiring, actionable error presentation, native UI, mounting, authored content, compilation, gameplay inspection and release remain incomplete. Account/readiness gates and world-control interlock remain disabled. Mission/crew/readiness observations and local receipts grant no movement, mission, combat, squad, voice, payment or Home-edit permission. No rural-campaign commands were added.

No tests, typechecks, lint, builds, compilation, codegen, generators, imports, asset conversion, process launches, installs, live calls, remote writes, deployment or configuration changes ran for this continuation. Only the owned source pair and this new handoff were checkpointed, with hooks disabled.

## SHA-256 records

| Consumed reference | SHA-256 |
| --- | --- |
| `packages/api/src/companion-missions.ts` | `f56f25b592c50b491c24be0985c52f8f883ce6310d76447f1ea0342957783a30` |
| `packages/utils/src/companionMissions.ts` | `5801b3926f513dde0fb3aaa12744667451c90dc74bff61033bebaded54366d54` |
| `apps/backend/convex/companionMissions.ts` | `1fbb544f87bf00a3581575a61e2f4e49b8ec02724be041caa5b3fdb92e026e8a` |
| `apps/backend/convex/lib/companionMissionsModel.ts` | `4cd55f75d9fd37c32c7db641ea284d77388d5ed5f28bfdb1d421aaa707f8a99b` |
| `docs/engineering/unreal-mission-state-handoff-2026-10-10.md` | `37bd4daa95b5aef8848185b5fcb6e2ce617ace669650a73c1f12001a6869171e` |
| `Private/BaddieCompanionStateJson.h` | `13ceb1b13bee4b0793ddb117e03559bfb8ea77f8bdb8dc81baa1ffa8e70bb618` |
| `Private/BaddieCompanionStateJson.cpp` | `df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce` |
| `BaddieCompanion.Build.cs` | `536bf39a4089acea43be36448ad3d02d3cd7f94a65015d3975f761941b802328` |

Native shorthand paths resolve under `apps/companion-unreal/Source/BaddieCompanion/`.

| Owned file | Canonical preimage SHA-256 | Source checkpoint SHA-256 |
| --- | --- | --- |
| `Public/BaddieCompanionMissionState.h` | `7793727ff1caffb8c32e446400b2c7484182ce34338027b7ba2b0feaa6e0600e` | `a787752ca56fe63b868922078e82846bbbad3fd28dd9a685bc05cd686cb8a4ce` |
| `Private/BaddieCompanionMissionState.cpp` | `b4ff56b2fef568574eedf26fa10fbf5ba3f81a354db8592f89810f2610b98ebf` | `c1245886809655ae46e5a4b3d1f43590b65338b4f6a309ba7705a35359e0d5f2` |
