# Football opening and input lifecycle

October 11, 2026. Source-only continuation of the accepted Football read and seven-command owner.

## Ownership and baseline

Existing chat `01a12714-f810-7ac1-8171-bc6d7c8c20d6`, host local, owns only:

- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionFootballState.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionFootballState.cpp`
- New `docs/engineering/unreal-football-lifecycle-handoff-2026-10-11.md`

All source preimages, canonical references and dependencies below use `cfa15652c52e510aac532a8cc4e13ce633d8415e`. Parent matched every working file. Confirm them and the new handoff's absence before editing. Extend FFootballReadState, retaining the accepted command completion-alias correction. Original read/command handoffs, StateJson, Build.cs, prior domains and production inputs are read-only. Parent owns this assignment, tracker, shared integration and acceptance; coordinator alone owns Windows writes.

Read the complete account store, shared Football types, validFootballInput, footballNextAdvanceAt and its NPC dependency closure, endpoint validators and current native source/handoffs. Translate Baddie's scheduling and receipt rules. Keep movement, collision, animation, AI navigation and clocks on native UE facilities when runtime integration is assigned. This work adds no local match simulation, physics, NPC movement or rendering.

## Opening and manual retry

Implement the existing open flow as immutable preparation, explicit once-only dispatch and terminal handling. Capture instanceId, revision, epoch, requestId, mode and a detached player/side roster. Sort the copied roster stably using UE's culture-aware text comparison. Keep exact identifier bytes and equal-comparison input order. The canonical code uses default-locale localeCompare, not lexicographic or case-insensitive ordering. Use the engine API, not a custom collation implementation. Record the selected culture/comparison behavior and leave cross-runtime locale acceptance explicit.

Epic's 5.8 FTextComparison reference identifies Internationalization/TextComparison.h and CompareTo. Its engine implementation is in ICUText.cpp or LegacyText.cpp. Official reference consulted October 11: https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Core/FTextComparison . This does not establish equivalent collation data across JS and UE versions.

The retry signature consists of instanceId, revision, epoch, mode and the sorted player/side roster. It excludes requestId. An identical retry retains the earlier captured request, including its requestId. A changed signature replaces it. Preserve the request on an ambiguous failure and clear it on successful open or session destruction. There is no automatic retry.

Opening refuses disposed or busy state but does not require an existing match, observers, foreground or host flags. Respect the backend's structural revision/epoch, requestId, mode and roster bounds without locally claiming visit, membership or pitch admission. Packet endpoint is companionFootball:open with captured roomId plus the selection. Credentials remain external; retry and query-cache invalidation are false.

Before notification, reset input, set busy and clear error, and stop pumping. Preserve the difference from send: successful open clears pendingEntry while the owner lives even when the query generation changed, but applies only for its captured generation; finally clears busy while alive. Ordinary open failure returns a sanitized outcome to the caller rather than setting send's match error. Preserve complete receipt identity through notification reentry and same-ID session reset. Document any required fail-closed dispatch boundary instead of retargeting a packet after scope replacement.

## Input, focus and neutral stop

Implement typed FootballInput with optional cancelActions, canonical validation and one owner for sequence, latest movement, dirty state, the maximum-two-action queue, action consumption, movement renewal and neutral cancellation. Use the pinned footballLimits; do not invent a second set of timing constants.

The playing predicate is exactly ready, no error, not busy, playing/restart phase and a connected current player. It is scheduling eligibility, not native permission. Normal sending also requires active input, no input/stop in flight and the minimum send interval. Consume a queued action only when the previous action sequence has been observed. A movement renewal preserves canonical action/target semantics. Ack sequence advancement requires the captured query generation, match scope and epoch.

Neutral cancel packets have a newer sequence and may overtake an in-flight action. Their branch precedes the ordinary in-flight/rate guard. Preserve neutralPending, sentMovement and actionSequence behavior across reset, input, focus loss, stop, accepted phase/scope changes and teardown. Validate complete packets without clamping coordinates, changing target IDs or inventing accepted input. Retain the distinction between an absent cancelActions and false.

setActive and setForeground remain separate. Foreground loss deactivates input, flushes required neutral state and requests driver release. Foreground return may request a pulse. stop resets input without inventing a match command. Integrate reset/pump effects into accepted command begin, failure, success and finally. Execute leave/end input deactivation before applying even an older successful command response. Existing command packet, ordering and intentional stale-query busy-latch behavior remain intact.

## Pump, pulse and teardown

Represent scheduling through an explicit caller-driven pump/deadline contract using UE time units and facilities. Do not start a timer, engine, network client or background process in this assignment. The future adapter must have one pump, not parallel custom timer loops.

Preserve syncPump's observer/playing/foreground/active/neutral predicate. Nonrelease pulses require foreground, playing and the tick interval. Use advancement.nextAt when advancement is present, including its authoritative null. Only absent advancement uses footballNextAdvanceAt(match, [], match.simulatedAt). Translate the complete due predicate, including NPC wakeups, overlaps, ball movement/claims, restart and match deadline. Reuse native containers/math. Implement only the read-only NPC calculation needed by this predicate; no autonomous NPC commands or physics step.

Preserve connected nonbot peer order, current player ordinal, ownerUntil delay and owner exemption. Query/command receipts do not establish serverOffset. A current pulse reply updates offset from advancement.serverNow against receive time before apply. Failed nonrelease pulses use the existing disconnect behavior; release failures do not set that error. One in-flight pulse coalesces a pending release, including a successful ownership reply arriving after foreground loss.

Last match observer loss and Dispose prepare any canonical final neutral/release effects while the captured session is still valid, then retire callbacks and stop pumping. If dispatch must be staged, expose exact immutable one-use effects and required ordering rather than sending after authority is gone. No expired session or replaced receipt may acquire fresh permission. Capture arguments before synchronous notifications; old cleanup must not retire newer requests. Preserve independent entry observers.

## Acceptance and handoff

Inspect every opening/input/pulse branch and notification boundary. Include reordered roster/manual retry, equal-collation IDs, changed signatures, invalid selections, scope/phase replacement, action queue saturation/consumption, neutral overtaking, stale acks, absent/null advancement, NPC wakeups, peer staggering, pending release, command integration, observer loss and disposal. Describe any canonical limitation separately from structural safety guards.

Checkpoint only the three owned paths with hooks disabled. The handoff must give full source/preimage/dependency pins, packet shapes, caller sequence, scheduling/clock units, manual retry identity, complete implemented cases and remaining integration. Parent will review the complete three-path range independently on Standards and Spec.

Account/readiness gates and world-control interlock stay disabled. Home and entry/presence grants never grant Football control. No tests, lint, typechecks, compilation/builds, codegen, asset work, launches, installs, remote writes, deployment or live configuration. Preserve unrelated work, saved identity and production renderer/mobile inputs. Source implementation, independent review, Windows delivery, compilation, in-game inspection and release remain separate.

## SHA-256 pins

```text
7790b96fdf23d66d84af08ac6c8033efe4145f40eaa6c6009f8c0dd99844343a  packages/api/src/companion-football.ts
979b865a71525b4602c3e486ad9b6175f14221d26da05953a407b58c0e4a033f  packages/utils/src/companionFootball.ts
5c4852ab1f1547f380ec6f475e049b65186a9d57443c1a97a3c9251c203fdde4  packages/utils/src/companionFootballEngine.ts
9b930929488f251b983c3854269c70ea764d7ffe285ca12698a70785e09892aa  packages/utils/src/companionFootballNpcs.ts
58634a775a8d05cc3b22224096f5a55acdfc4bbea3d02b355c3f91e410db0930  apps/backend/convex/companionFootball.ts
1e78be4330d2d753170ef782cf9dc69f7b6f37b1d787c57feb6e5e76a381ff2f  apps/backend/convex/lib/companionFootballModel.ts
09cfe09acdf1c51665cc039e0af3255dffbed8940214222ba8b15cef836d5062  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionFootballState.h
0ec9919bb4633680562992ce542df67102f4dc17e11890600a2484e7a7e83bc2  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionFootballState.cpp
13ceb1b13bee4b0793ddb117e03559bfb8ea77f8bdb8dc81baa1ffa8e70bb618  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.h
df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.cpp
536bf39a4089acea43be36448ad3d02d3cd7f94a65015d3975f761941b802328  apps/companion-unreal/Source/BaddieCompanion/BaddieCompanion.Build.cs
ec55f12339406b532d185c3e04ed6e82e23afb734cc6f480cb9cbeea5534f051  docs/engineering/unreal-football-state-handoff-2026-10-10.md
f60b41e8513be6359ee63828d724da296b3176c6f4ca60ca4b67b8b35a2b3ad6  docs/engineering/unreal-football-command-handoff-2026-10-11.md
```
