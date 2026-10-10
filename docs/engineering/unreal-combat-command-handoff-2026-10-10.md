# Unreal combat command and input handoff

Source checkpoint for the October 10, 2026 assignment. No transport or gameplay activation.

## Scope and pins

Source commit `fac5699a147bade400965390ec73ae4629c946f1` changes only `BaddieCompanionCombatState.h` and `BaddieCompanionCombatState.cpp` under `apps/companion-unreal/Source/BaddieCompanion/Public` and `Private`. This new handoff is the third owned path.

The assignment is `docs/engineering/unreal-combat-command-assignment-2026-10-10.md` at `443dd04e7be54e0efe39ef6e02ed48c85dbaaa74`. Canonical references and both source preimages are pinned at `964bb18fe0b5b97f481472d42c683bd5f408ec90`. The complete unchanged StateJson pair remains required from `b4bc1b7d7dbc9552ad1451062a404fc872a13224`. Recovery and all earlier source/handoffs remain frozen.

`FCombatReadState` now owns command packets, caller-driven input sequencing/rate accounting, one input flight and one pending neutral stop. The existing decoder and response application remain in this owner. No alternate movement, simulation, clock tick, sender loop, credential, permission or readiness input was added.

## Future adapter sequence

1. Establish native authority separately and bind each subscription generation to its original account/session/game/visit transport context. Select the externally established instance and player, then call `BeginSubscription`. Receipt getters expose exact instance, expected player and subscription generation for this binding; none is a credential. A changed session with the same selection requires `Clear`, not the same-value `Select` no-op.
2. Prepare a command with `PrepareJoin`, `PrepareLeave` or `PrepareRobberyAction`, or call `PrepareInput` for ordinary input. Supply a valid preparation wall time. Command capture saves selection, match/life, response scope and arguments before `StopInputs` and busy/error notification. Preparation rechecks ownership after that notification and returns empty if reentry retired it. Input preparation reserves the sole flight slot without notification. Neither path sends.
3. Drain captured cleanup after any command preparation, explicit stop, lifecycle operation or response callback. `GetPendingStop` exposes an opaque pending receipt for context lookup or unsent abandonment. When no flight remains, `PreparePendingStop` serializes that captured neutral input with a fresh prepared receipt. It does not consult the new selection or reconstruct a scope. Failure leaves the pending receipt available for abandonment. A pending receipt cannot dispatch or complete its later prepared request.
4. Finish adapter admission before calling `DispatchRequest(Receipt, DispatchedAtMs)`. On success, immediately send the returned immutable endpoint/arguments without yielding or notifying. Dispatch rechecks ownership and marks once. It captures the actual send wall time for midpoint timing and ordinary-input rate accounting. Add credentials only in the admitted adapter and retain `retry: false` and `invalidateQueryCache: false`. Copy the returned receipt for callbacks.
5. A no-flight stop must be dispatched or explicitly abandoned before command dispatch can succeed. A command does not wait for an already dispatched flight; that flight drains its pending stop on terminal completion. If cleanup is already prepared but unmarked, dispatch or abandon it first. A mark refusal leaves request state unchanged. Use `AbandonRequest` for matching unsent work that will not proceed; do not turn abandonment into a failed network response.
6. Decode replies with `DecodeCombatResponse`, then call `Accept` with the receipt wall time, or call `AcceptFailure`. Always check for pending cleanup after the method returns, including `StaleReceipt` or a false failure result. Those results describe display application, not flight retention. Stale and silent terminal outcomes still release their matching dispatched flight. A duplicate or unmarked receipt does not release anything.

All calls and callbacks belong to one owning thread. The adapter must keep the owner alive throughout notifications and every outstanding callback. After `Dispose`, `HasInputWork` remains true while a sent flight or captured cleanup needs settlement/draining. Once that work has settled or unsent cleanup has been abandoned, only external callback lifetime obligations remain. Dispose cannot start new gameplay input or commands.

Retain the old admitted transport context for cleanup, including across account selection changes. A captured packet is not authority to send. If that context no longer permits cleanup, abandon the unsent packet by its exact receipt. A caller-held request copy survives clearing and must never be replayed. The state enforces one dispatch mark, not how many times an adapter sends a retained string. After a successful mark, use the terminal callback path even if the adapter's send fails synchronously.

## Exact command packets

All arguments use UE JSON serialization. Strings retain case and whitespace; no additional native request-size ceiling was added. Existing decoder budgets continue to bound observed match, life and robbery data. Join's selected instance has the existing selection contract, not a new native character limit.

| Method | Endpoint | Non-credential arguments and eligibility |
| --- | --- | --- |
| `PrepareJoin` | `companionCombat:join` | Exact selected `instanceId`; `lifeId` only when observed. No `matchId`. Does not require a match or life. |
| `PrepareLeave` | `companionCombat:leave` | Selected `instanceId`, captured `matchId` and `lifeId`. Requires displayed match and life. |
| `PrepareRobberyAction` | `companionCombat:robberyAction` | Selected `instanceId`, captured `matchId`/`lifeId`, displayed player's robbery `encounterId`/`revision`, and exact `action` of `defend` or `flee`. Requires match, life and that robbery. |

All commands require a live selected owner, active subscription, no busy command and valid preparation/dispatch times. They share one busy owner. No fighter-status, robbery-phase, countdown, approach or arrival predicate was added. Request values never follow a later selection, life, match or robbery observation. Command dispatch checks current selection/generation/request, while response acceptance separately compares the captured and current scope generations.

## Input and stop ownership

`FCombatInputValue` uses an integer sequence, two named move axes, double-precision source radians and three booleans. Its wire shape is exactly `{ sequence, move: [right, forward], yaw, pitch, attack, block, reload }`. The input endpoint receives that value under `input`, alongside captured `instanceId`, `matchId` and `lifeId`. No local receipt ID is sent.

The typed shape excludes fractional sequence, malformed tuples and non-boolean flags. Remaining validation follows `validCombatInput`: sequence is nonnegative and strictly below 9,007,199,254,740,991; each finite move axis has magnitude at most 1; finite yaw has magnitude at most `2 * pi`; finite pitch has magnitude at most `pi / 2 - .05`. Values are neither normalized nor clamped.

Ordinary preparation requires an active subscription, selected instance, displayed match/life, phase `playing`, no command busy state and no existing flight or pending cleanup. It does not require a present or active fighter. Sequence is the maximum of supplied sequence, selected last sequence plus one, and observed player's last input sequence plus one. A missing fighter contributes zero. Values at or above 9,007,199,254,740,990 are refused to reserve the final valid neutral sequence.

The 200 ms interval is checked at preparation and again at dispatch using caller wall times. A 199 ms difference refuses; 200 ms permits when other predicates hold. A backward wall-clock difference also refuses. Dispatch records the actual send time and commits the allocated sequence and last input. Unsent ordinary abandonment therefore consumes neither the send interval nor a last-sent input. Neutral cleanup bypasses the ordinary rate/phase/busy predicates and does not advance the ordinary send clock.

| Stop case | Retained work |
| --- | --- |
| Dispatched non-neutral flight | Invalidates input response ownership. Captures one neutral stop at flight sequence plus one with the flight's original scope, yaw and pitch. Updates selected last sequence only if the flight still belongs to that exact player/instance/match/life. Clears last input. |
| Already neutral flight | Invalidates response ownership and clears last input without queuing another stop. |
| Repeated stop with pending cleanup | Retains the first captured stop. No gameplay coalescing or second queue. |
| No flight, active displayed fighter and prior non-neutral input | Captures a valid neutral input at the maximum of selected last sequence plus one and fighter last input sequence plus one, retaining prior yaw/pitch. No playing-phase check is added to this canonical branch. |
| No flight without those prerequisites, or exhausted neutral sequence | Invalidates input responses but creates no invalid or fabricated release. |
| Prepared but unmarked ordinary input | Discards that unsent reservation, then considers the previously sent input for the no-flight stop. It cannot send unsent gameplay after selection loss. |
| Prepared neutral cleanup | Remains drainable with its captured scope. Further stops cannot turn it into gameplay input. |

An input flight remains owned throughout response notification. A notification can request stop, change selection or dispose; duplicate completion is rejected while the original call is completing. The finally step then releases only that exact flight. Its pending stop becomes available to the adapter after return. No post-notification write changes the display, command state or a newer input flight.

Scope-changing accepted observations, command preparation, selection changes, subscription begin/end, Clear and Dispose call stop before losing old display scope. Match/life/selection changes reset selected last sequence, last input and send time. Subscription begin/end retire callback generations and busy ownership while retaining other display data and the canonical selected allocation history. Clear and Dispose reset private display/allocation data but retain a sent flight and its already captured neutral cleanup. Their late replies remain stale.

Preparation/encoding failures do not install partially built requests. Pending-stop encoding or invalid-time failure retains the pending identity for explicit abandonment. `AbandonRequest` clears only its matching unsent command, prepared input or pending stop; command abandonment notifies after clearing busy. It adds no error, ambiguity classification, retry slot or scheduling. Dispatched flights cannot be abandoned as unsent.

## Response preservation and legacy compatibility

The existing decoder, its helpers, field allowlists, nullable policies, privacy checks and resource limits are byte-identical to the canonical preimage. See `docs/engineering/unreal-combat-recovery-handoff-2026-10-10.md` for the complete projection and StateJson contract. That document's earlier statement that command/input ownership is absent is superseded only by this local source slice.

| Response case | Preserved behavior |
| --- | --- |
| Current subscription | Checks player and non-null state's selected instance. Same-match older revision is ignored before scope change. Equal revision, different match or null state can replace the display, set ready and clear error. Subscription server time is ignored. |
| Current command after scope changed | Accepts only if returned match/life agree with the current pair. Otherwise settles its own busy state without accepting the response or observing its time. |
| Accepted-scope mutation | Records `serverTime - (DispatchedAtMs / 2 + ReceivedAtMs / 2)` before same-match revision rejection. An older revision retains data but can update the midpoint offset. Legacy receipts continue using their captured `BeganAtMs`. |
| Input success or error | Requires captured subscription/input generation and match/life to remain current. An accepted response itself may change life or match. Ordinary failures use the existing fixed error. |
| Neutral failure | Stays silent. Its matching flight still settles, even after lifecycle loss. |
| Command failure | Publishes the fixed error only in the captured response scope, then settles its matching current busy owner. |
| Invalid response or identity mismatch | Uses the existing failure behavior. An owned dispatched input still releases its flight. |
| Default, foreign, retired, duplicate or unmarked-owned completion | Cannot alter current display or clear newer work. A sent stale input's first terminal callback only releases that exact retained flight and exposes existing cleanup. |

Receipt identity is opaque local bookkeeping. Owned completion reads the owner's dispatch-time receipt, so a caller retaining the preparation copy cannot substitute an earlier timestamp. A successful backend input response may contain unchanged state after sequence/rate refusal; acceptance is not an attack acknowledgement.

`CaptureCommand(double)` and `CaptureInput(double, bool)` remain externally admitted receipt-only APIs. They have no owned packet and require no dispatch mark. Command capture now shares the canonical stop boundary and post-notification identity check. Legacy input capture retains its previous scope-only response semantics and does not allocate sequence, reserve a flight or make duplicate completion one-shot. External legacy transport must continue owning those obligations; it must not represent an owned flight with a newly captured legacy receipt. `InvalidateInputReceipts` remains response invalidation only; use `StopInputs` to capture outbound cleanup.

## Source review and remaining work

Source review covered every command, join's omitted life, exact robbery capture/action, command reentry, unsent abandonment, no-flight/in-flight/repeated/already-neutral stop, selection and disposal during flight, pending-drain identity, stale/duplicate/unmarked completion, sequence exhaustion, 199/200 ms boundaries and existing null/lower/equal/cross-match/cross-life response branches. The complete two-file diff was inspected. `git diff --check` passed. All eleven assignment pins matched before editing; nine read-only files and both shared dependency blobs still matched afterward. Original decoder/helper bytes were compared directly to the canonical preimage.

The writing-for-agents skill separated the adapter sequence, behavioral reference and completion criteria. Unslop guided the status language. No tests, lint, typechecks, builds, compilation, code generation, asset conversion/import, launches, installations, remote calls/writes, deployments or configuration changes ran. Only owned paths were checkpointed, with hooks disabled. Unrelated work remains untouched. Renderer packaging and What's New are not triggered by this native-only slice.

Parent-owned independent full Standards and Spec reviews remain pending. The coordinator owns Windows delivery. Native transport/consent, world/control producers, first-person bindings, squad/hearing integration, content, compilation, in-game inspection and release remain incomplete. Existing account/readiness gates and the world-control interlock remain unchanged and disabled. Entry/presence and read-only Home grants cannot authorize combat. Character Movement remains the movement implementation; no combat simulation or replacement engine system was introduced.

## SHA-256 records

Owned preimages at `964bb18fe0b5b97f481472d42c683bd5f408ec90`:

```text
f64ac3d2e0a1efe7d555e889fc466ba3e621dd7cfcd8963667851303fd1d9621  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionCombatState.h
a1d30a72e112df92b9ca0977c97c91754a315f6d24bd0eaf2d0391d39a2382a1  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionCombatState.cpp
```

Outputs at `fac5699a147bade400965390ec73ae4629c946f1`:

```text
bbdebfa73a9d2e00579031711a640435f4d9b88dde4d81d2cee0fb3b60b54e4c  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionCombatState.h
57891ae3a067c42f40efe8de8148a5c0b23a5a469bea2c653c7861eea0dd0db2  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionCombatState.cpp
```

Unchanged canonical inputs/dependencies:

```text
3c8fe8b68481774c6a886c9fa1c964c8c307ac40acb5d48322f8cf9adbb5e320  packages/api/src/companion-combat.ts
56da7b73be7622d6d0d7954c6ada30ffc7cf10d04bb3ddbfbb29559b4839078d  packages/utils/src/companionCombat.ts
19e1ef63adeeb51fde2d5c798e33a260906b261ae791c3f3fe9de5ed84d1f1c2  apps/backend/convex/companionCombat.ts
68c4352f1d5730a35f199efe5c0e8cc839e5fb8781dad5c0a308b75c213ef238  apps/backend/convex/lib/companionCombatModel.ts
0378f1692ebb343ff8dc734e72edd08011469ef9061c58f25c2db33a006ca94a  docs/engineering/unreal-combat-recovery-handoff-2026-10-10.md
13ceb1b13bee4b0793ddb117e03559bfb8ea77f8bdb8dc81baa1ffa8e70bb618  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.h
df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.cpp
90d230cf0233458c46324fd87d6f077c6f54bd7dd691030ba5a2a89e8607a17d  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionWorldPlayer.h
536bf39a4089acea43be36448ad3d02d3cd7f94a65015d3975f761941b802328  apps/companion-unreal/Source/BaddieCompanion/BaddieCompanion.Build.cs
```
