# Unreal life command handoff

October 10, 2026.

Source commit: `773b036e50b89c87b10c3509715e44e11eb08d42`.

`FLifeReadState` now owns local care, activity-report and task command requests. The existing Life decoder and read owner remain in place. No transport, native command permission, simulation, station producer or UI integration was added. Gates and the false world-control interlock are unchanged.

## Scope and dependencies

Assignment: `docs/engineering/unreal-life-command-assignment-2026-10-10.md` at `86ef1f7797e42cb8303620529e5b72b3e567c441`. Canonical references and owned preimages: `410995b4dfa23a6c78d29e1b6728c4c56199f6ac`.

The source commit changes only:

- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionLifeState.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionLifeState.cpp`

This new handoff is the third owned path, committed separately. All sixteen assigned pins matched both canonical Git bytes and working SHA-256 values before editing; the handoff path was absent. `packages/api/src/convex.ts` was additionally inspected for the mutation wrapper policy and matched the same canonical base.

The complete StateJson pair remains at `b4bc1b7d7dbc9552ad1451062a404fc872a13224`. Life uses its existing bounded parser, null parser, exact readers and enum decoder, plus UE JSON serialization for arguments. No shared operation was missing and no shared helper was edited. Life's request-ID validation is its own backend length contract, not Food's stricter grammar. Accepted Food/Kitchen source and all prior handoffs stayed read-only. Unrelated changes and staging were preserved; commits use explicit paths with hooks disabled.

## Adapter sequence

The owner is single-threaded. A future authenticated adapter must provide non-secret account identity, selected character and admitted observations, and marshal callbacks to that thread. Neither a receipt nor a saved Home read grants permission to invoke these account endpoints.

1. Select account and character, then use the existing observation lifecycle. Commands capture selection and displayed life; callers cannot supply a replacement character or expected revision. Request IDs for care and tasks are explicit arguments, validated once and preserved. The adapter may generate them with UE GUID facilities before preparation.
2. Call `PrepareCare`, `PrepareCareRetry`, `PrepareReport` or `PrepareTask`. Each successful call returns an opaque `FLifeCommandReceipt` with endpoint, original request ID where applicable, non-secret scope and immutable JSON arguments. `IsValid` means populated, not currently authorized. Failed preparations clear their output.
3. Care and task preparation update public busy/error state and notify listeners. Care then rechecks attempt identity, account and disposal, plus generation/character for a first send. Task rechecks its receipt and generation/scope. Reentrant selection, clear or disposal cannot revive a retired preparation. Reports do not notify or modify busy during preparation.
4. Admit the captured operation through the future authenticated boundary. At actual send, call `DispatchCommand`. A true return marks dispatch without notification; send the captured arguments immediately, with no asynchronous or callback gap. A false return means do not send. This method itself sends nothing. Care retries may retain an old character/generation within the same account; first care sends and auxiliary commands require their captured current generation and scope.
5. If preparation is abandoned before any send, call `AbandonPreparedCommand`. Never use it after dispatch. A first care preparation is dropped without ambiguity. Abandoning a prepared care retry retains the earlier uncertain request. Task abandonment settles busy only in its captured generation; report abandonment changes no presentation.
6. Deliver one final endpoint value to `CompleteCommand`, or bounded sanitized failure facts to `FailCommand`, with that receipt. Completion/failure requires a current dispatched receipt. Default, retired, duplicate, cross-owner and old-account receipts cannot publish or settle new work. Receipts retire before completion notifications, and methods perform no later writes over reentrant changes.
7. On session replacement or access loss, call `Clear`, retire external callbacks/receipts and release private snapshots. A changed account ID clears command state automatically. Same-account session replacement still requires explicit clear. `Dispose` retires all receipts, pending care, selection, presentation and delegates. Reusing the same IDs cannot validate an old receipt.

The adapter owns authentication, observer counts, unsubscribe functions, response-envelope extraction and object lifetime. All encoded payloads omit account identity, tokens and device credentials. The endpoint values do not echo a command request ID, so response-to-receipt correlation must remain intact in the adapter.

## Care actions

All four actions use `companionLife:act` with `characterId`, `roomId`, `stationId`, `expectedRevision`, `requestId`, `action` and optional copied `cooking`.

| Action | Encoded literal |
| --- | --- |
| Start | `start` |
| Cancel | `cancel` |
| Enable reminders | `reminders-on` |
| Disable reminders | `reminders-off` |

`expectedRevision` is copied from the displayed Life, and character/account from selection. An immutable private care request retains the entire intent, original scope/revision/request ID and once-encoded JSON. A new action is rejected while public busy, private `actInFlight`, pending care or missing life prevents it. No station lookup, route decision, cooking availability, timer or optimistic life update occurs.

There is exactly one pending care request and at most one active care attempt. `actInFlight` is independent of public busy: invalid reads, read failures and last-observer removal may clear busy without releasing the private attempt. A never-dispatched request does not become uncertain. After dispatch, ambiguous failure retains it; only explicit `PrepareCareRetry` retries it. Retry uses a fresh local attempt GUID but byte-identical JSON and the same original request ID. It works after character changes or hiding via an empty character within the same account, without requiring current life or public busy to be false.

A first send captures its current generation and cannot dispatch if notification reentry or a later pre-send change retires that generation. A replay captures the generation at retry preparation but deliberately bypasses the first-send generation/character check. Account change, clear or disposal always wins. Neither first send nor retry can overlap another care attempt.

Completion requires a valid non-null Life for the request's original character. It resolves pending care even if publication is no longer allowed. Publication requires the attempt's captured current generation, account/character and a revision at least as large as the currently displayed Life. Older or old-generation success returns `ResolvedWithoutPublication`; no different character's view is populated. A successful care response does not clear an error received during flight. Care's final settlement clears busy and sets retryable from remaining pending care, even after a same-account character change.

Null, malformed or wrong-character care responses return `InvalidResponse` and retain pending care as ambiguous, using `Care response could not be verified. Retry the previous action.` only in the captured current view. They do not clear life or install a default.

## Activity reports

`PrepareReport` accepts `arrived`, `blocked` and `interrupted` for the exact currently displayed activity ID. It captures account, character, callback generation and status. `companionLife:report` receives only `characterId`, `activityId` and `status`. Its internal GUID is receipt identity, not a fabricated wire request ID; `GetRequestId` is empty.

Reports can coexist with care, pending ambiguous care and tasks. They do not set busy, occupy the care retry slot or compute an arrival. The actual producer must establish an admitted activity observation; a string ID and the read Life alone do not establish arrival authority.

Report completion accepts null or a valid Life matching the captured character. Null consumes the receipt without synthesizing Life, clearing error, changing flags or notifying. Non-null publication requires the current captured generation/scope and an equal or higher displayed revision. It does not require the activity to remain current when the response arrives, matching the canonical report callback.

An accepted non-null blocked report sets `The character couldn't reach this activity. Move the blocking furniture and try again.` Other accepted report statuses clear error. Lower revisions change nothing. Invalid/mismatched responses use the canonical care-data error; report failure updates only the captured current view and never creates manual care retry state.

## Task commands

Tasks use `companionLife:task` with captured `characterId`, displayed `expectedRevision`, original `requestId` and an immutable nested `command`. Only the selected branch's fields are serialized.

| Type | Fields beyond `type` |
| --- | --- |
| enqueue | `roomId`, `stationId`, optional `cooking` |
| pause | None |
| resume | None |
| stop | None |
| remove | `taskId` |
| retry | `taskId` |
| move | `taskId`, `direction: up/down` |
| advance | `roomId` |

Preparation requires non-busy presentation, no pending care and a displayed Life. It captures revision before notification and cannot submit a retired account/generation after reentry. It does not inspect task availability, reorder a local queue, settle rewards or start cooking. The task type `retry` is a queue command, not a transport retry or the care retry operation.

There is intentionally no private task in-flight exclusion beyond the canonical public busy/pending predicates. A read failure can clear busy while an earlier task remains in flight; after life recovers, another task may become possible. Task receipts support that overlap. Each current-generation task completion/failure clears busy, including when its returned revision is older than the current view, matching the canonical generation-based `finally`. It does not write care retry state. Success preserves errors received after preparation; failure writes its operation error. Old-generation task results/errors/busy settlement cannot change the new view.

Task success requires non-null, valid, matching-character Life and publishes only at equal or higher current-view revision. Invalid responses use `Task data couldn't load. Reopen your space to try again.` and consume the task receipt without entering the care retry slot.

## Read and lifecycle composition

The full Life decoder, read receipt checks, field bounds, current-view revision comparison, captured once-per-observation null default and origin distinction remain intact. Command publication goes through the same displayed Life revision comparison and marks a non-null returned value as `SavedValue`; only an authoritative null read can publish `DefaultForNull`.

| Event | Public view | Private command state |
| --- | --- | --- |
| Initial/clear/dispose | Life unavailable, busy false, error unset, retryable absent | Empty |
| Same-account character change/hide | Life/origin/error reset; busy from `actInFlight`; retryable from pending care | Care retained; auxiliary receipts retired |
| Account change | Reset as a changed selection, with busy/retryable false | All old-account commands retired |
| Begin/reconnect | Retains Life, origin, busy, error and retryable; captures a new null default | Care retained; auxiliary receipts retired with old generation |
| Last-observer removal | Retains Life/origin/error/retryable; busy false, no notification | Care and `actInFlight` retained; auxiliary receipts retired |
| Invalid read/read failure | Life/origin cleared, busy false, retryable absent, read error set | Pending care and private `actInFlight` unchanged |
| Successful read with pending care | Accepted current Life/origin; current error preserved | Pending care unchanged |
| Successful read without pending care | Accepted current Life/origin; error cleared | No command settlement |

Read success spreads the current flags; it does not reconstruct a missing retryable field after read failure. Selection explicitly assigns retryable, preparation assigns false for care, and care completion assigns whether pending care remains. Revision comparison is against the current displayed Life, never a permanent maximum, so recovery after life-clearing read failure can accept a lower revision or a revision-zero null default.

Reconnection must not be implemented by first calling public `EndSubscription`, which represents last-observer removal and clears busy. `BeginSubscription` retires its own previous generation while retaining busy. The adapter separately stops/replaces the actual subscription. A reconnect during a task can retain busy while retiring that task's callback generation; the old task must not clear that retained/new-generation busy. This is the canonical generation behavior, not a new task lock.

## Validation and failure boundary

Care/task request IDs preserve the backend's length-only contract of 8 to 100 characters, with no trimming, ASCII grammar or generated replacement. Command character IDs are nonempty and at most 80 characters; observed read IDs may still use the accepted 100-character read limit. Expected revisions are nonnegative safe integers strictly below `MAX_SAFE_INTEGER`, copied from Life rather than caller concurrency tokens.

Care room/station strings allow empty values and are at most 80 characters, including cancellation/reminder payloads. Start cooking recipe strings are capped at 80; the server owns recipe existence and station-kind checks. Non-start care preserves optional recipe text without imposing the start-only recipe restriction. Every present cooking object copies recipe and batches, with batches exactly 1 or 2. Task enqueue requires nonempty room/station/recipe strings at most 80; advance requires a nonempty room ID at most 80; task IDs are nonempty and at most 100. Reports require an activity ID of 8 to 100 characters. Invalid enum/literal values fail preparation. These are wire-input checks, not gameplay admission.

Native auxiliary ownership is bounded to 64 combined prepared/dispatched report and task receipts, separate from the one care request. Capacity rejection leaves state unchanged. Completion, failure or prepared abandonment frees a slot; generation retirement clears all auxiliary slots because their callbacks cannot publish afterward. This resource bound and explicit pre-dispatch checks are native adapter distinctions from the promise-based canonical client. Retiring a receipt does not cancel or undo a dispatched server operation.

The future transport boundary supplies an explicit definite/ambiguous category plus already-sanitized public text. For care, reproduce the pinned `isNetworkUnavailableError` predicate and the exact save/unverifiable-care message cases. Network/service-unavailable, `Unable to save changes right now. Please try again.` and unverifiable-care responses retain pending care. Definite failure drops it, including a definite failed replay. Error publication still requires the attempt's captured current generation/character. Read success does not prove command success.

Command public text is bounded to 500 characters; missing, overlong or disallowed control-character text uses the operation fallback. Present-empty text stays empty. Fallbacks are `Couldn't save this activity. Try again.`, `Couldn't update this activity. Try again.` and `Couldn't update the tasks. Try again.` Read failure text keeps its accepted existing contract. The adapter must supply sanitized public diagnostics, never raw exceptions, server envelopes, tokens or credentials. Length bounding is not sanitization.

The canonical care invocation uses `retry: false`. Report/task omit that option, so the pinned `convexMutation` wrapper can retry according to `shouldRetryConvexInvocation`, then invalidates query cache on success by default and sanitizes terminal errors. Those wrapper retries and cache effects belong to a future admitted adapter, not this owner. It should deliver one final completion/failure per marked logical invocation with the same captured arguments throughout. No timer, send loop or automatic retry was added here.

## Review and remaining work

Manual source review covered all four care actions, first-send/replay generation rules, immutable payloads, unsent abandonment, private/public busy separation, all report statuses and nullable results, every task branch and move direction, overlapping auxiliary requests, account invalidation, notification reentry, stale/duplicate receipts, current-view revisions, origin/null handling, error composition and disposal. Scoped `git diff --check` found no whitespace errors. Read decoder behavior was not changed.

No tests, typechecks, lint, compilation, builds, codegen, imports/conversion, engine/server launches, installs, remote writes, live calls, configuration, deployment or release ran. No renderer/mobile inputs changed, so packaging and What's New were not triggered. This is source inspection, not executable or Unreal compatibility evidence.

Parent independent Standards and Spec reviews remain required for the complete pinned command scope. Prior Life read acceptance does not accept this continuation. The coordinator owns transfer; no Windows delivery was performed here. Native command permission, authenticated transport, admitted activity/station producers, mounting, imported content, compilation, in-game inspection and release remain incomplete.

## SHA-256 record

Outputs at source commit `773b036e50b89c87b10c3509715e44e11eb08d42`:

```text
117cabff74a9ba2a568385e5a0e8ddc2d55a48ca06b268f2bc12d8cca98e94d4  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionLifeState.h
bd9a26eec8cf5c0a880f4f25e8a8f0e742592ad0ecc47c21c13e63d51cbaa14f  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionLifeState.cpp
```

Owned preimages at canonical base:

```text
3639a71fde0aa52b2f03bf0ed4fd992cf381cdf27b31600b3ae650282e1ea350  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionLifeState.h
45e1e8d8b61a9ef5d4a7a5a5728e42fa964421c648a2bb277121cf27a97a784e  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionLifeState.cpp
```

Read-only references and dependencies at canonical base. Hashes cover full files; inspection used the relevant contracts, store behavior, producer boundaries and wrapper policy.

```text
b87d63e40ab06932e0322268217f5d4b202083fcd475c9ae39e13e497d2ae90e  packages/api/src/companion-life.ts
33957fe910e8e00718cb2cba95189abc231c0a9822cf2acb6283ee6a2a2bc992  packages/api/src/convex-error.ts
b628e8415be3f064096b05e6878e1176f238635bd641f965582e68fa9e66ecf9  packages/utils/src/companionLife.ts
878b249b0ef1629d78e78c6fc2856205930030dd3ba3cb161c3225afe6ff1619  packages/utils/src/companionTasks.ts
b87c1927a96fb1806d63ca3fd3ada1bf9686659ba2d8ec2255d4737f2df61066  packages/utils/src/companionSkills.ts
80f615c7cd627fafe60588ea93f7c725bda581ae3944737e8127db848ee2c95a  packages/utils/src/companionActivityKinds.ts
67c4d9d38b6bffe7e50e141bd53cf9915c100635afdc012481d0e862cdc124e7  apps/backend/convex/companionLife.ts
70713e76678fb90cfa645e5beba8076a4620b2cd0f51e0d1a632fe7a5511084f  apps/backend/convex/lib/companionTasksModel.ts
394ff44ed11283e02b977666570f154a8e309555eb91b2027f750d4bb2511a83  apps/backend/convex/lib/companionSkillModel.ts
1c46ba2566d0f806974a0617e685e2020b100d39c3a005a94ff71cc2864ee6d9  apps/backend/convex/lib/companionModel.ts
d9102a5084da37c2f649e8ccdf87e8046f274f17f2e799bdeec20d2c89d711c7  docs/engineering/unreal-life-state-handoff-2026-10-10.md
13ceb1b13bee4b0793ddb117e03559bfb8ea77f8bdb8dc81baa1ffa8e70bb618  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.h
df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.cpp
536bf39a4089acea43be36448ad3d02d3cd7f94a65015d3975f761941b802328  apps/companion-unreal/Source/BaddieCompanion/BaddieCompanion.Build.cs
df868d2b3d624db3b74173e68f5cfdc0d96667ec559bfa111b7d44f45837a7e8  packages/api/src/convex.ts
```
