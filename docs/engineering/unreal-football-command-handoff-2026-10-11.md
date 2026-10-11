# Unreal Football match command handoff

October 11, 2026. Seven match commands have local request ownership. The slice remains unmounted.

## Scope and pins

Original source commit `12463a3b31f13acd90ddecfedb1a19c73f65ff6e` changes only `BaddieCompanionFootballState.h` and `BaddieCompanionFootballState.cpp` under `apps/companion-unreal/Source/BaddieCompanion/Public` and `Private`. Correction commit `a0bd9dc4dae65f18c83d3af1f8eed7e700ddac87` changes only the `.cpp`, capturing completion identity before synchronous notifications. This handoff is the third owned file; its previous version is at `a6a2dd7e049dc71553962600bbbce791bcd5e035`.

The assignment is `docs/engineering/unreal-football-command-assignment-2026-10-11.md` at `f2ce043fbe686cfbbf246e6ea0ab0f4e35368d50`. Canonical references, dependencies and source preimages are pinned at `c9ee8d539835c87a87308b074759bc1a5896bee2`. Both unchanged StateJson files retain the array-reader dependency from `b4bc1b7d7dbc9552ad1451062a404fc872a13224`. All earlier scopes and the original Football read handoff remain read-only.

The patch extends `FFootballReadState`; it adds no second store, input owner, sender loop, timer, simulation or permission. Query and command success now use the same private `ApplyMatch` reducer. Existing raw decoders and entry-observer behavior are unchanged.

## Adapter sequence

1. Establish native Football consent and authenticated transport separately. Select the account, nonsecret session-incarnation marker and room through the existing read API. `ResetSession` always retires command state, including when all supplied IDs are unchanged. A same-value `Select` remains a no-op.
2. Build `FFootballCommand` and call `PrepareCommand`. It captures the room, match epoch, query generation, session generation and a local request ID before setting busy, clearing error and broadcasting. The future input owner must implement canonical `resetInput` and pump stopping at this boundary. This patch does not execute either effect.
3. Complete transport admission using the captured receipt scope and immutable JSON. Call `DispatchCommand`, then send the successful returned packet immediately without yielding or notifying. Use its endpoint and arguments with externally owned credentials, `retry: false` and `invalidateQueryCache: false`. A mark refusal makes no state change. A retained string or successful mark is not native authorization.
4. For unsent work that will not proceed, call `AbandonCommand` with its exact receipt. This clears only the matching pending request and busy latch, preserves current data/error and broadcasts. It creates no fictional network failure. Marked requests cannot be abandoned as unsent.
5. On transport success, call `BeginCommandSuccess` with the captured receipt and response JSON. It claims the terminal outcome once and decodes the complete snapshot. Malformed data publishes fixed public-safe error text and finishes through the ordinary finally path. A valid result is an immutable `FFootballCommandSuccess`, not an applied snapshot.
6. Before any pre-apply consumer effect, check `IsCommandSuccessCurrent`. For a current success whose `RequiresInputDeactivation()` is true, the future input owner must set input inactive before snapshot application. This covers leave and end, including replies that the ordering reducer will later ignore as older. Execute effects on the bound owner without an asynchronous gap. Then call `AcceptCommandSuccess` even if the stage has become stale, so its matching private terminal state can retire. The method rechecks ownership after any effect-induced reentry.
7. On an ordinary transport failure, call `AcceptCommandFailure` with `FFootballPublicReadFailure`. Only caller-sanitized public text belongs here. An absent message uses `Couldn't update the match. Try again.` A present empty message stays empty. Once success staging has claimed the outcome, a second success/failure callback is rejected.

Keep the owner alive through every synchronous notification, retained success stage and outstanding callback. Calls belong to one owning thread. Successful stages must reach `AcceptCommandSuccess` or a session reset; dropping a stage is not cancellation. Caller-held requests or stages cannot be replayed after completion. No request ID or session identity is serialized into endpoint arguments.

## Seven packet shapes and bounds

Every packet targets `companionFootball:command` with exactly `roomId`, `epoch` and `command`. Room and epoch are captured from the selected room and displayed match, not supplied by the command caller.

| `FFootballCommand.Type` | Nested `command` object |
| --- | --- |
| Ready | `{"type":"ready","ready":false}` or the corresponding true value |
| Side | `{"type":"side","side":"home"}` or `"away"` |
| Start | `{"type":"start"}` |
| Retry | `{"type":"retry"}` |
| Rejoin | `{"type":"rejoin"}` |
| Leave | `{"type":"leave"}` |
| End | `{"type":"end"}` |

Only the selected command member is serialized. Invalid command-type enum values reject; Side also rejects values outside Home/Away. Unused ready/side fields do not add fields to other command types. UE JSON serialization escapes captured strings; case and whitespace are preserved. Scope validation reuses the existing nonempty, at-most-4,096-code-unit account/session/room bounds. There is no additional packet-size ceiling.

The received match epoch remains a finite double in read state. Preparation reuses StateJson `Integer` with minimum `-9007199254740991`, accepting exactly the safe-integer range through positive `9007199254740991`. Fractional or out-of-range epochs reject without changing busy/error. Zero and negative safe integers are not forced positive; backend equality and all gameplay admission remain authoritative. No match state, pitch, player ID, host claim or simulation input enters the packet.

Preparation refuses disposed, busy or missing-match owners, invalid scope, invalid command enum or unrepresentable epoch. It does not require observers, ready=true, an empty error, foreground state, connected players, a phase or host status. A same-generation match update, including a changed epoch or null match, cannot retarget a prepared packet.

## Query-generation busy limitation

Command ownership distinguishes the session/request identity from the current query generation. `BeginMatchObservation`, reconnect/reload and `EndMatchObservation` replace the query generation but retain the command busy latch. This deliberately preserves canonical `send` finally behavior.

| Event | Command outcome |
| --- | --- |
| Preparation notification reloads or ends observation | Preparation returns no dispatchable request. Its unsent private receipt remains available through `GetPendingCommand`; busy stays true. |
| Query generation changes before dispatch | The old mark rejects. Explicit abandonment of that still-unsent owner can clear busy. |
| Query generation changes after dispatch or success staging | The old reply cannot apply data/error or run a current pre-apply effect. Its terminal call consumes only its private request; busy stays true. |
| Query generation changes inside success/error notification | The original finally path retires its request but does not clear busy or emit a final busy=false notification. |
| Changed selection, unconditional ResetSession, Clear or Dispose | Request/stage ownership retires and private display state, including busy, clears. No old callback can clear newer work. |

`GetPendingCommand` describes the current pending request, not necessarily the request from a failed outer preparation. Do not automatically abandon it after reentry; identify the intended unsent work before using that receipt. A marked stale completion can leave busy=true with no pending request. Subsequent reads retain that latch, and commands remain blocked until session/selection reset, Clear or Dispose. Reload alone does not repair it. This limitation is documented, not silently corrected by a retry or new readiness rule.

## Application and notification ordering

The command stages are Prepared, Dispatched, Completing and Applying. Entering Applying before broadcasting prevents copied success stages from applying again during notification. Each completion method copies the entering receipt before its checks and uses that local value through final cleanup. A listener replacing the caller's receipt or success object cannot redirect cleanup to newer work.

Current successful replies use the original ordering rules: lower epoch for the same match ID; then lower revision in the same match/epoch/player scope; then older advancement server time in that scope at equal revision. Otherwise the full snapshot replaces the view. Typed scope comparison, null/empty player equivalence, missing advancement clearing and all original finite-number/nullable policies remain unchanged.

Both accepted query and command replies preserve local `View.bBusy` and clear error. Wire busy/error remain decoded but never own the local latch. Older replies retain data/error and do not emit from the reducer. A command with an older reply still reaches its current-generation finally step and completes once.

| Current command outcome | Notifications after preparation |
| --- | --- |
| Accepted snapshot | Snapshot notification while busy=true, then matching finally clears busy and broadcasts again. |
| Older decoded snapshot | No apply notification; matching finally clears busy and broadcasts once. |
| Sanitized failure or malformed success | Retained-data error notification while busy=true, then matching finally clears busy and broadcasts again. |
| Query-stale terminal outcome | No display/error/final-busy notification. Private request retirement only. |

All post-notification cleanup is identity-checked. Clear, same-ID ResetSession, replacement commands and disposal during a notification cannot be overwritten by the old continuation. The final busy=false notification runs after private command retirement, allowing a listener to begin new work without a later old write. Existing Dispose behavior remains terminal and silent, clearing delegates without a notification. Entry observation does not share command generations or alter command busy.

## Source inspection and remaining work

The initial implementation received a two-file source walkthrough covering all seven packets, false ready and both sides, JSON-escaped strings, invalid type/side/epoch values, no-observer commands, snapshots arriving while busy, all older-response guards, malformed and sanitized failures, empty error text, unsent abandonment, duplicate dispatch/staging/application, query replacement at each notification boundary, same-ID reset and disposal. That walkthrough missed caller-alias replacement across notifications. The parent Spec review identified this P2 defect; the correction below addresses it.

The correction diff and every command continuation were inspected. These are source walkthroughs, not executed regression cases:

- In `AcceptCommandFailure`, command A publishes its error. A listener resets the same-ID session, restores a match, prepares or dispatches B, and overwrites the caller's receipt with B's receipt. Final cleanup now checks the local A receipt and leaves B's pending state, busy latch and error untouched.
- In malformed `BeginCommandSuccess`, the error notification permits the same replacement. Cleanup retains A's identity even if the listener also stages B's success. Neither the input JSON nor the caller's receipt is read after notification.
- In `AcceptCommandSuccess`, the snapshot enters `ApplyMatch` by value before notification. A listener can reset, prepare, dispatch and stage B, then assign B's success into the caller's success object. Cleanup still uses A's copied receipt. The old continuation neither applies B nor retires it.
- Preparation already captures the caller's command into a local request before broadcasting. Dispatch and success-currentness checks do not notify. Abandonment and final cleanup finish their writes before notifying and do not read arguments afterward. Failure text is copied into the view before its notification. Valid success staging does not notify.
- Without session replacement, recursive acceptance of A still rejects because its stage is Applying. Query-only replacement retires A without clearing the intentional stale busy latch. Changed selection, same-ID reset, Clear and Dispose invalidate A's ownership. Final busy=false listeners can begin B because no old state write follows that notification. No-observer command admission and all notification ordering remain unchanged.

Scoped `git diff --check` passed. The correction preimages matched the original source commit and previous handoff. All ten canonical assignment hashes were rechecked, and the eight read-only working references/dependencies still matched their pins. The complete StateJson pair also matched its dependency commit. The header remains unchanged by this correction.

The writing-for-agents skill separated the adapter sequence, lifecycle reference and completion limits. Unslop guided the wording. The diagnosing-bugs skill guided the alias audit; executable reproduction and regression steps were skipped under the assignment's source-only restriction. Only owned paths were checkpointed, with hooks disabled. No tests, typechecks, lint, builds, compilation, code generation, asset work, launches, installs, network mutations, remote operations, deployment or configuration changes ran. This native-only slice requires neither renderer packaging nor a What's New change.

Parent-owned Standards and Spec reviews of the full corrected three-path range from `c9ee8d539835c87a87308b074759bc1a5896bee2` remain pending. The original reviewers must review that complete range, not only the correction commit. The coordinator owns Windows delivery. Opening and locale-ordered captured roster/manual-retry identity remain following work. Input/reset/neutral-stop sequencing, active/foreground ownership, release coalescing, pump/pulse timing, server offset, NPC-aware fallback due calculation and teardown effects remain absent. In particular, this patch reports leave/end deactivation requirements but has no consumer that executes them. The slice must remain unmounted until that complete control lifecycle and native Football permissions exist.

Account/readiness gates and the world-control interlock are unchanged and disabled. Gameplay entry/presence and Home reads grant no Football permission. Source acceptance, Windows transfer, compilation, in-game inspection and release remain separate; none of the latter steps is claimed here.

## SHA-256 records

Source preimages at `c9ee8d539835c87a87308b074759bc1a5896bee2`:

```text
09143c0fd911dabd6c1362b95c0950fface589ad244ab457a4f043f986100a7a  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionFootballState.h
92a8d5cca6a4c45a86d6e8b78e4d867c8374249ebdee0ca0d31741e67382edc5  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionFootballState.cpp
```

Original source outputs at `12463a3b31f13acd90ddecfedb1a19c73f65ff6e`, also the correction preimages:

```text
09cfe09acdf1c51665cc039e0af3255dffbed8940214222ba8b15cef836d5062  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionFootballState.h
9656955c32512c43f8b4d227a788b2d6ccb7e4e967ee152f1d7fbf00f69b53de  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionFootballState.cpp
```

Corrected source outputs at `a0bd9dc4dae65f18c83d3af1f8eed7e700ddac87`:

```text
09cfe09acdf1c51665cc039e0af3255dffbed8940214222ba8b15cef836d5062  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionFootballState.h
0ec9919bb4633680562992ce542df67102f4dc17e11890600a2484e7a7e83bc2  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionFootballState.cpp
```

Handoff preimage at `a6a2dd7e049dc71553962600bbbce791bcd5e035`:

```text
96cbb915888c851aa596c290ba3214b1add82ec46529444a3256e986105228ab  docs/engineering/unreal-football-command-handoff-2026-10-11.md
```

The correction's final handoff commit and output hash accompany this document in the completion report, avoiding a self-referential hash.

Unchanged canonical references/dependencies:

```text
7790b96fdf23d66d84af08ac6c8033efe4145f40eaa6c6009f8c0dd99844343a  packages/api/src/companion-football.ts
979b865a71525b4602c3e486ad9b6175f14221d26da05953a407b58c0e4a033f  packages/utils/src/companionFootball.ts
58634a775a8d05cc3b22224096f5a55acdfc4bbea3d02b355c3f91e410db0930  apps/backend/convex/companionFootball.ts
1e78be4330d2d753170ef782cf9dc69f7b6f37b1d787c57feb6e5e76a381ff2f  apps/backend/convex/lib/companionFootballModel.ts
ec55f12339406b532d185c3e04ed6e82e23afb734cc6f480cb9cbeea5534f051  docs/engineering/unreal-football-state-handoff-2026-10-10.md
13ceb1b13bee4b0793ddb117e03559bfb8ea77f8bdb8dc81baa1ffa8e70bb618  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.h
df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.cpp
536bf39a4089acea43be36448ad3d02d3cd7f94a65015d3975f761941b802328  apps/companion-unreal/Source/BaddieCompanion/BaddieCompanion.Build.cs
```
