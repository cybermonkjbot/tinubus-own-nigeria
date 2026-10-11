# Football match commands

October 11, 2026. A source-only continuation of the existing Football read owner.

## Ownership and fixed references

Existing chat `01a12714-f810-7ac1-8171-bc6d7c8c20d6`, host local, owns only these paths for this assignment:

- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionFootballState.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionFootballState.cpp`
- New `docs/engineering/unreal-football-command-handoff-2026-10-11.md`

Canonical references, dependencies and both source preimages are pinned to `c9ee8d539835c87a87308b074759bc1a5896bee2`. The parent matched all ten working files below. Recheck these exact bytes and confirm the new handoff absent before editing. Extend FFootballReadState rather than adding another Football store. Original read handoff, shared StateJson and all previously completed scopes, including Combat, Recovery, Life, Food/Kitchen and Racing, are read-only. Parent owns shared integration, this assignment, review acceptance and the tracker. Coordinator alone owns Windows writes.

Read the complete canonical account store and shared Football types, command endpoint and response validators, and current native pair and original handoff. Retain the accepted array-reader dependency at `b4bc1b7d7dbc9552ad1451062a404fc872a13224` without editing either helper file. Source paths in the hash table are repository-relative.

## Seven commands

Translate the existing send operation into typed command preparation, immutable JSON packets, explicit dispatch and completion. Every packet targets `companionFootball:command`, carries captured roomId and epoch plus the nested command object, and retains retry:false and invalidateQueryCache:false. Use Unreal JSON serialization and current bounded readers. No credentials belong in this state owner or packet.

| Command type | Nested fields besides type |
| --- | --- |
| ready | ready, boolean |
| side | side, home or away |
| start | none |
| retry | none |
| rejoin | none |
| leave | none |
| end | none |

Preserve case-sensitive identifiers and exact captured strings. Reject invalid native enum values and unrepresentable command numbers without inventing gameplay eligibility. The backend requires a safe-integer epoch equal to its authoritative match. Do not round a received number, force it positive or infer admission from it. Reuse native text bounds and document any added structural boundary. The packet never carries match state, pitch, player identity, host claims, simulation inputs or new endpoint fields.

## Lifecycle and retained reads

The canonical early return is disposed, busy or missing match. It does not require match observers, ready=true, no error, foreground, a connected player, a particular phase or host=true. Do not turn any read field into native authorization. Transport and separate domain consent remain missing.

Capture the current query generation, session incarnation, room and match epoch before publishing busy. Set busy and clear error. Keep one command owner and explicit once-only dispatch marking. The external adapter must send immediately after marking without another yield or notification. Default, foreign, retired and duplicate receipts cannot dispatch or complete. An unmarked request can be abandoned explicitly, releasing only its own busy latch; a marked request cannot be silently abandoned or automatically retried. A successful begin or mark is local bookkeeping only.

Preserve synchronous notification behavior through the existing Changed delegate. Recheck request/session identity after every notification before changing state or exposing a dispatchable packet. A listener may reload, end observation, reset the same-ID session, clear or dispose. A same-generation match update must not retarget the immutable packet. Reject callback application after query-generation replacement. A session reset always clears private command state, even with identical IDs.

A current successful reply decodes the complete FootballSnapshot and uses the same older-epoch, older-revision and older-advancement ordering as an observed reply. Factor this existing apply behavior privately so query and command callbacks cannot disagree. Preserve local command busy while accepting either reply; wire busy/error never own it. Accepted data clears error. An older decoded response does not replace the view but still proceeds through the command's current-generation finally behavior. A current ordinary failure retains match data and publishes caller-sanitized text, with the canonical fallback `Couldn't update the match. Try again.` A present empty error remains empty. Malformed success uses fixed public-safe text and retained data, not partial application or raw JSON in the error.

Canonical send clears busy only when its captured query generation still matches and the owner is alive. Query reconnect or last-listener loss changes that generation but does not clear the existing busy latch. Preserve and document this limitation rather than silently repairing the canonical lifecycle during translation. Select with a changed scope, unconditional ResetSession, Clear and Dispose retire the request and clear private state. A stale completion never clears a newer owner's busy/error. Completion is single-use even when the decoded reply is older than the current view. Preserve canonical notification ordering and protect post-notification cleanup from reentry.

Successful leave/end requests also deactivate input in the canonical store before applying their response. Input focus, resetInput and pump ownership do not exist in the native reader yet. Record these required consumer effects explicitly in the result/handoff if needed; do not pretend to execute them, fabricate an input owner or broaden this assignment to one. This source slice must remain unmounted until the complete control lifecycle and native permissions exist.

## Completion and next stages

Inspect all seven serialized packets, strings needing JSON escaping, invalid enum/epoch values, retained snapshot updates during busy, older replies, sanitized failures, notification reentry, unsent abandonment, duplicate dispatch/completion, no-observer commands, changed query generation, same-ID session replacement and disposal. Checkpoint only the three owned paths with hooks disabled. Write the new handoff with immutable source/preimage/dependency hashes, complete packet shapes, public API/notification order, source-inspection evidence and remaining integration. Parent will obtain independent Standards and Spec review of the complete three-path change.

Match opening and its locale-ordered captured roster/manual-retry identity remain a following source assignment. Input/neutral-stop sequencing, active/foreground ownership, release coalescing, pulse timing/server offset, NPC-aware fallback due calculation and teardown effects also remain unfinished. Do not substitute guessed due times or permissive readiness inputs. No automatic timers, new network client, local simulation, room/call creation, physics replacement or UE engine machinery is needed here. Reuse native systems when their integration is assigned. This bounded command stage does not remove those requirements from the full port.

Account/readiness gates and world-control interlock stay disabled. Gameplay entry/presence, Home reads, a decoded snapshot and a local receipt grant no football, movement, combat, mission, call, payment or editing permission. No tests, typechecks, lint, builds/compilation, codegen, asset conversion/import, engine/server/Blender launches, installs, remote writes, deployment or live configuration. Preserve production renderer/mobile inputs and unrelated changes. Source acceptance, Windows delivery, compilation, in-game inspection and release remain separate.

## SHA-256 pins

```text
7790b96fdf23d66d84af08ac6c8033efe4145f40eaa6c6009f8c0dd99844343a  packages/api/src/companion-football.ts
979b865a71525b4602c3e486ad9b6175f14221d26da05953a407b58c0e4a033f  packages/utils/src/companionFootball.ts
58634a775a8d05cc3b22224096f5a55acdfc4bbea3d02b355c3f91e410db0930  apps/backend/convex/companionFootball.ts
1e78be4330d2d753170ef782cf9dc69f7b6f37b1d787c57feb6e5e76a381ff2f  apps/backend/convex/lib/companionFootballModel.ts
ec55f12339406b532d185c3e04ed6e82e23afb734cc6f480cb9cbeea5534f051  docs/engineering/unreal-football-state-handoff-2026-10-10.md
09143c0fd911dabd6c1362b95c0950fface589ad244ab457a4f043f986100a7a  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionFootballState.h
92a8d5cca6a4c45a86d6e8b78e4d867c8374249ebdee0ca0d31741e67382edc5  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionFootballState.cpp
13ceb1b13bee4b0793ddb117e03559bfb8ea77f8bdb8dc81baa1ffa8e70bb618  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.h
df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.cpp
536bf39a4089acea43be36448ad3d02d3cd7f94a65015d3975f761941b802328  apps/companion-unreal/Source/BaddieCompanion/BaddieCompanion.Build.cs
```
