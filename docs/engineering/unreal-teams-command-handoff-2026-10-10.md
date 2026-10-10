# Unreal Teams command handoff

October 11, 2026. Source implementation and self-review only. The filename retains the October 10 assignment date. Independent Standards/Spec acceptance, Windows delivery and runtime integration remain pending.

## Source, ownership and pins

Source commit `1f6c60f90d01cf0ea1eb1e8bb9a6cd7f65ef8bfc` changes exactly `BaddieCompanionTeamsState.h/.cpp`. This new handoff is the third owned path. Assignment `unreal-teams-command-assignment-2026-10-10.md` was introduced at `8a246b5bd9f1cc3c762f0d363fb22891035447d4`; canonical references and source preimages are pinned at `2b12d3338fcc77f17257d06414332a417bb702f0`.

All twelve working hashes matched before editing. Both owned source paths were clean and this handoff was absent. The ten unchanged inputs still matched at source checkpoint. POS, Resident, NpcDialogue, SocialState, earlier handoffs, tracker, StateJson, backend, mobile, renderer and configuration were left unchanged by this patch.

| Source output | SHA-256 |
| --- | --- |
| `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionTeamsState.h` | `392e158eaf269fe40713dd2827fe2918374bd153a342fb5d51c781cc3eb8b5e4` |
| `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionTeamsState.cpp` | `492957967e12fa36fc3f089b41b43d63911cd3c3b768d3c6120fd9aefa2505d5` |

Input and preimage SHA-256 values:

```text
3865698f96a385dfe2e5c2ebb311c72eb1b6d423451fac3e693ac285dc8b4a21  packages/api/src/companion-game-teams.ts
7b48fd0125415d7210e8042c87f4136fc05f9c3c8faa032750d4dd7190f32146  packages/utils/src/companionGameTeams.ts
564d96a85bc381760292a3e686240b85a3da9b7bd40df881acf04a095e71a308  packages/utils/src/companionCall.ts
8c375da18a2df74a76fb2b758bd2ec87c28be1734a82ccc06d9443ea854a3b2a  packages/utils/src/companionGames.ts
abaf86d039ce4a6947f75950003cc4d3b773259d8d4115c27a20a39740a83ccf  apps/backend/convex/companionGameTeams.ts
0c82dc0e21f9590926283ea5610f5699a9a838f64bece6dc9838ce0bf4d5f58e  apps/backend/convex/lib/companionGameTeamsModel.ts
9329c5c1e160fa7021256e8bf407884ad1cb58026f924e67a217d30ff0ee18fb  docs/engineering/unreal-social-teams-state-handoff-2026-10-10.md
0010ebd474920c3dcd2a1cae051b0b48750f3a217173e045b8ff1c82e7ca9da9  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionTeamsState.h
00e98e16fc184c36003a303a25813897baefca425a1e1c8fea43abf7940d76ac  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionTeamsState.cpp
13ceb1b13bee4b0793ddb117e03559bfb8ea77f8bdb8dc81baa1ffa8e70bb618  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.h
df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.cpp
536bf39a4089acea43be36448ad3d02d3cd7f94a65015d3975f761941b802328  apps/companion-unreal/Source/BaddieCompanion/BaddieCompanion.Build.cs
```

Use the complete unchanged StateJson pair from `b4bc1b7d7dbc9552ad1451062a404fc872a13224`, with the hashes above. New readers reuse its object, scalar, nullable, bounded-array and raw-null operations. The module already depends on Json; no Build.cs change is needed. The original combined social/teams handoff remains historical; this record supersedes only its Teams command/admission deferrals.

## Commands and response boundaries

`FTeamsState` remains the only Teams display/busy/error owner. Each accepted command stores a const packet with the command, initial call projection, any captured leaving call, operation and UE-serialized arguments. There is one active command per current identity. No second Teams store, call controller or retry queue was added.

| Command | Operation | Immutable non-auth arguments | Endpoint value | Final send result |
| --- | --- | --- | --- | --- |
| Create | `companionGameTeams:create` | `threadId`, `setting`, `capacity` | Complete admission | Admission after attachment, otherwise null |
| Invite | `companionGameTeams:invite` | `roomId`, `startedAt`, ordered `userIds` | `invitedUserIds`, `expiresAt` | Null |
| Join | `companionGameTeams:join` | `roomId`, `startedAt` | Complete admission | Admission after attachment, otherwise null |
| Decline | `companionGameTeams:decline` | `roomId`, `startedAt` | Raw null | Null |
| Leave | `companionGameTeams:leave` | `roomId`, `startedAt` | Raw null, then possible matching-call leave | Null |

Create preserves studio, bedroom, kitchen, terrace and town-square exactly. Surprise and invalid enum values are rejected. Capacity and command startedAt accept finite doubles without rounding or local eligibility limits. This preserves the current create endpoint's numeric input even though it enters the public city instead of applying the historical twelve-player capacity setting. The read owner still accepts public-city capacity 50 and retains separate selected outer-group and room-thread identities.

Invite requires a supplied array, allows empty, and caps it at twelve IDs. Order and duplicates survive encoding. It does not inspect membership or manufacture invitation effects. Its response decoder retains the returned array and expiry; an empty array is valid. The command flow validates this result and then returns null, as canonical send does. Returned invite arrays have an explicit native bound of twelve; this is not a server membership claim.

Admission decoding requires exactly roomId, threadId, callSessionId, startedAt, expiresAt and literal startMuted true. The nullable call ID must be present, and null is preserved. No requested room/group ID is substituted for a returned ID; public-city admission can name a different thread. Timestamps retain finite nonnegative safe-number values, including fractions, without a new expiry-order predicate. Decline, leave and cleanup accept only actual root null through the shared UE-compatible null parser.

All decoders publish atomically and clear outputs on failure. Existing native limits remain 262,144 input characters, depth eight, 4,096 structural separators, IDs up to 240 characters without controls or surrounding whitespace, and public text up to 8,192 characters. Missing fields, extras, unsupported types/literals and oversized data fail with fixed diagnostics. Typed commands reject missing required fields and fields belonging to another branch. These limits establish structure, not ownership or authority.

A malformed mutation response enters the local failure path and returns null. Only a complete decoded admission can supply cleanup identity. A partial or unusable entering response cannot be treated as proof that the mutation had no effect, and the reducer does not invent cleanup fields or automatically repeat the entry request. The future transport/server recovery remains responsible for that gap.

## Initial preflight and notification order

The caller supplies a fresh minimal `FTeamCallObservation` from the original account's controller. It contains status and required nullable room, with threadId, required nullable callSessionId and optional gameRoomId when a room exists. Default status or missing room is unavailable, never idle. Null call IDs remain distinct from strings. An explicit empty observed call ID is representable so post-join validation can reject it as empty; an absent or empty gameRoomId remains falsy for entering preflight.

`PrepareCommand` refuses disposed/unidentified/busy owners. Create must name the selected group. For other commands, roomId and startedAt must match the displayed room only when that room exists. Missing view/room does not add a refusal and does not grant permission. Entering commands require idle or joined with a truthy gameRoomId. Joined to some game is enough at this initial check; same-game identity is checked after admission. Explicit leave captures thread/call IDs only when initially joined to the requested game room.

`PreflightFailed` sets the canonical fixed choose-group, changed-game or keep-current-call error for caller notification. `Refused`, `InvalidInput` and `CallUnavailable` make no state change and create no request. Preparation success captures all inputs, acquires busy and clears error before the caller publishes the busy notification.

After that notification returns, call `ContinueCommandAfterNotification`. It checks the exact current request and identity, samples selectedAtStart then, and enters ReadyMutation. A selection change inside the busy notification therefore changes the error target without rewriting the original command. It does not rerun the earlier selected-group/room preflight. Clear, disposal or identity replacement during notification prevents dispatch. If the notification cannot complete, abandon the genuinely unsent request and finish it; do not fabricate a sent network failure.

## Caller-driven stages

All reducer methods are synchronous and invoke no external callbacks. The host drives the stages using the original command receipt. `MarkNextAction` returns Dispatched only when it publishes a fresh action receipt and const action. Perform that exact action immediately after marking, without a yield or notification. Mutation and cleanup adapters must use `retry:false` and `invalidateQueryCache:false` and bind credentials outside these packets to the original account lifetime.

Advanced means a local stage transition occurred without an outbound action; inspect `GetCommandStage` and continue. NeedCallObservation leaves the stage unchanged and issues no action. Supply a fresh projection when available, not a cached idle substitute or a polling loop. Rejected/default/foreign receipts do not advance work.

| Stage | Caller operation and next state |
| --- | --- |
| Prepared | Publish busy, then `ContinueCommandAfterNotification` to ReadyMutation |
| ReadyMutation | `MarkNextAction` publishes the immutable command and enters MutationSent |
| MutationSent | `AcceptMutationResult` validates the endpoint value; entering goes to AttachPresence or detached CleanupReady, invite/decline to Finally, leave to CheckLeavingCall |
| AttachPresence | `MarkNextAction` consumes fresh call state; same game goes to AttachmentReturned, idle marks a muted join, another call enters catch cleanup, retired identity exposes cleanup only |
| JoinSent | `AcceptCallCompletion` goes to CheckJoinedCall; failure enters catch cleanup |
| CheckJoinedCall | `MarkNextAction` validates fresh post-join call state, updates admission's actual call ID and goes to AttachmentReturned, or enters catch cleanup |
| AttachmentReturned | `CompleteAttachment` models the continuation after awaiting attachPresence; alive fixes the admission result and enters Finally, retired exposes CleanupReady and null result |
| CheckLeavingCall | `MarkNextAction` compares the fresh call to the captured leaving call; exact match marks leave, otherwise Finally without a call action |
| LeaveCallSent | `AcceptCallCompletion` goes to Finally; failure returns null with current-group catch error |
| CleanupReady | `MarkNextAction` publishes captured admission roomId/startedAt to the leave endpoint and enters CleanupSent |
| CleanupSent | Valid null goes to Finally; failure follows the bounded disposal/catch rules below |
| Finally | `FinishCommand` returns the fixed result, applies an eligible catch error, releases the original busy owner and seals Finished |
| Finished | No further stage or finalizer is accepted; release all caller-held receipts/actions |

Supply failures to `AcceptActionFailure` only for a marked stage. Use `AcceptMutationResult` only for mutation/cleanup and `AcceptCallCompletion` only for call actions. Separate shared stage identities reject duplicates even when two successive cleanup stages have the same enum value. A sent stage cannot be abandoned as unsent or completed with a default action receipt.

Preserve canonical await boundaries, including the separate continuation after attachPresence even when sameGame returns immediately. Do not add outcome notifications or waits between synchronous stages. Once Finally is reached, call `FinishCommand` immediately without an added yield/notification. Its error publication and busy release occur before the canonical final notification. Notify only when its bNotify output is true, then deliver the saved result to the original caller. Final-notification reentry can start new work; all old state writes have already ended.

## Attachment, leave and cleanup

SameGame requires joined status and exact gameRoomId, threadId and nullable callSessionId equality with the admission. A matching joined call with a null ID and null admission ID returns that exact admission without joining again. Otherwise the latest status must be idle. A joined different game passes initial preflight but fails attachment with the fixed keep-current-call error and admission cleanup.

The join action represents `calls.join(admission.threadId, admission.callSessionId ?? undefined, { audio: false })`. Null means omit the expected call ID, not an empty string. It does not start voice. After the join promise completes, a fresh observation must be joined, have a nonempty callSessionId, and match admission threadId and gameRoomId. A nonnull admitted call ID must also match exactly. Success preserves every admission field except replacing callSessionId with the observed actual ID. Failure uses `The team game couldn't open. Try again.` and enters cleanup. The selected crew thread never replaces the public city's returned thread.

Disposal or clear before admission arrives still allows that already-marked response to be decoded by its original receipt. A complete admission then exposes only captured cleanup, never a join. Retirement while awaiting a marked join still permits its completion and fresh-call validation, followed by the detached cleanup path. Retirement between sameGame/post-join success and CompleteAttachment also exposes cleanup and returns null. Missing post-join call observation is not fabricated as success; the caller must retain the draining receipt and provide the real projection.

Explicit leave checks gameRoomId plus both originally captured thread/call IDs again after the mutation. Null call IDs use exact nullable equality. With no original matching call, it goes straight to Finally. A changed or nonjoined call is retained. The action calls `calls.leave()` without arguments; its published identifiers describe the checked call, not an instruction to leave chat/group membership. As in the canonical store, a sent explicit leave can drain against its original matching controller after disposal. Never route this action to a replacement account's controller.

Cleanup contains only the exact admission roomId and startedAt. An attachment failure enters catch and exposes one cleanup action; its failure is swallowed without replacing the original error. Disposal before or after attachment exposes the canonical leave inside try. If that leave fails, catch exposes one further cleanup action. Failure of that final action is swallowed. Thus disposal can produce at most two cleanup attempts, while attachment failure produces one. Neither path rejoins, loops, invents rollback or leaves another call. Raw-null decode failure follows the corresponding cleanup failure branch.

## Lifetime, errors and legacy compatibility

The store, identity, command and outbound stage each have distinct shared receipt identities. The current store retains only its active command. Clear detaches it and erases displayed private state, but caller-held command/action receipts retain the original packet/admission until draining finishes. There is no global retired-request queue.

Keep the original FTeamsState object, command receipt, marked action receipt and admitted adapter/controller context alive until Finally finishes, including after Dispose. Only the original store instance accepts those receipts. A replacement identity in that object may own new work while old marked work drains, but old completion/error/finally cannot modify its snapshot, clear its busy owner or notify it. Unsent old work cannot be marked. Same textual identity is an existing no-op in SetIdentity; call Clear on same-account session replacement before installing the new lifetime.

`AbandonUnsentCommand` accepts only Prepared or ReadyMutation and moves to Finally. It performs no cleanup, restores no old error and claims no server outcome. Finish is still required to release current busy. After a stage has been marked, retain the receipts through its real completion/failure and any captured cleanup. Dropping the original store or the last detached receipt early loses the ability to drain; this source does not launch background recovery. Terminal Finish clears packet, admission, result, error, selection and account/request references from shared work. Retained stale copies still read Finished and cannot act. Discard caller-held const actions and returned admissions when their original consumer releases them.

Selection, reload and last-observer loss preserve the current command busy owner. Existing view/call-entry decoding, room/invitation expiry, member order, nullable fields, public-city capacity and outer-versus-room thread rules are unchanged. Successful snapshot observations clear command errors; read failures replace the public error category and clear command text without clearing busy. Reload clears error; selection resets the view/error while retaining busy. No command manufactures a Teams view or invitation.

Catch errors affect the view only when the original request is still live and selected group equals the selectedAtStart captured after busy notification. Read changes during an awaited cleanup can therefore be followed by the original catch error only when that comparison still holds. All non-entering successes and every failure return null. Successful entry returns the fixed admission only to its original caller, not a replacement identity.

FTeamsPublicFailure accepts an already-sanitized Error-instance message. Empty or oversized text uses `That change couldn't finish. Try again.` as the native bounded fallback. Fixed preflight, attachment and decoder diagnostics contain no server data. The reducer neither sanitizes raw exceptions nor retains/logs raw documents or credentials; the adapter must perform that sanitization.

Legacy BeginObservedCommand/FinishObservedCommand retains its previous one-step busy/error semantics and captures selection at legacy begin. It remains suitable only around externally admitted legacy work. New typed commands use the explicit after-notification continuation instead. Both APIs share the busy owner, cannot overlap, and their sealed token types cannot finish each other's work. Legacy success still synthesizes no team or admission; selection/reload/stop retain busy, and identity replacement rejects legacy finalizers.

## Backend gap and remaining integration

The current server-owned-city leave branch in `apps/backend/convex/companionGameTeams.ts` resolves the account's current guest by room/user and checks its session, but returns before the later startedAt comparison. It can release the current guest even when the captured admission refers to an earlier reservation. An immutable roomId/startedAt cleanup packet therefore does not establish replacement-reservation isolation. The backend was not changed, and this source must not be enabled as native authorization. The older combined handoff's claim of exact reservation release is superseded by this recorded gap.

Native transport/admission, response-envelope limits, sanitized-error mapping, the real current-call projection producer, original-controller lifetime binding, backend cleanup isolation, subscription/notification host, mounting and UI remain absent. Entry/presence v1, Home/apartment reads, crew membership, canJoin/canHost and decoded admissions grant no native world, combat, voice, editing or money permission. All account/readiness gates and the world-control interlock remain disabled.

## Review and completion limits

Self-review read the full canonical store, shared command/call types, endpoint/model declarations, original handoff, tracker and complete owned source/diff. It traced all five commands, all response shapes, same/different/null call cases, missing observations, selection changes during busy notification, changes during mutation/join/cleanup awaits, disposal before/after admission and attachment, finite cleanup failure paths, raw-null rejection, legacy overlap, duplicate/unmarked/stale completion and final-notification reentry. Scoped whitespace checks passed and source/input hashes were inspected. These are source findings, not executed tests or runtime evidence.

No tests, typechecks, lint, builds, compilation, codegen, imports, conversions, launches, installs, remote operations, live requests, call actions, configuration changes or deployment ran. This unmounted native slice changes no production renderer/mobile inputs and triggers no renderer packaging or What's New update. The parent owns full separate Standards and Spec reviews, integration and acceptance. The coordinator owns Windows delivery. Backend cleanup correctness, compilation, in-game behavior and release remain incomplete.
