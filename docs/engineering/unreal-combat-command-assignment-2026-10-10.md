# Unreal combat command and input assignment

October 10, 2026. Source-only request ownership.

## Owner and pins

Existing owner `01a12714-f810-7ac1-8171-bc6d7c8c20d6`, host local, owns only CombatState.h/.cpp under `apps/companion-unreal/Source/BaddieCompanion/Public` and `Private`, plus new `docs/engineering/unreal-combat-command-handoff-2026-10-10.md`. Recovery and all earlier source/handoffs stay read-only. Parent owns StateJson, assignments, tracker and acceptance. Coordinator alone owns Windows writes.

Canonical references and owned preimages are pinned at `964bb18fe0b5b97f481472d42c683bd5f408ec90`. Parent matched all eleven working hashes below; the new handoff was absent. Recheck before editing. Use the complete unchanged StateJson pair from `b4bc1b7d7dbc9552ad1451062a404fc872a13224`.

## Existing owner to extend

Read the full `packages/api/src/companion-combat.ts`, its input types/validators in `packages/utils/src/companionCombat.ts`, relevant backend endpoint contracts and the original combat/recovery handoff. Keep FCombatReadState, its complete decoder, identity checks and accepted read/receipt behavior. CaptureCommand and CaptureInput currently provide only receipt bookkeeping. Extend this owner rather than introducing another combat store.

Use UE JSON serialization and existing value/receipt facilities. This work translates request ownership, not combat simulation or movement. Character Movement remains the movement implementation. No replacement physics, navigation, damage, reload, AI, timer, ticking sender or renderer is needed.

## Commands

Prepare immutable requests for all three canonical commands:

| Endpoint | Non-credential arguments |
| --- | --- |
| companionCombat:join | selected instanceId and optional captured lifeId, omitted when absent |
| companionCombat:leave | selected instanceId, captured matchId and lifeId |
| companionCombat:robberyAction | selected instanceId, captured matchId/lifeId, the displayed player's robbery encounterId/revision and exact defend/flee action |

Keep client eligibility from command(): disposed/unselected/busy refusals, leave requiring state/life, and robbery requiring state/life plus the current player's robbery and action. Existing native active-subscription/identity/time guards remain. Do not add fighter-status, robbery-phase or arrival predicates that the canonical client does not have. Preserve exact source strings and document any necessary native size ceiling.

Capture selection, subscription/scope identity and command values before the canonical stop boundary and notification. Keep join/leave/robbery's same command busy owner. Command preparation must request the existing-input stop behavior below, rather than merely invalidate response receipts. Preserve the distinct captured command scope and current response scope.

Provide explicit one-time dispatch and unsent abandonment. Recheck the exact captured owner/request after synchronous notification and before returning a dispatchable packet. Default, stale, foreign, duplicate or unmarked-owned completion must not settle newer work. Abandonment releases only its unsent owner; sent requests are not unsent. Keep compatible receipt-only APIs distinct from newly owned requests. Future adapters dispatch immediately after a successful mark, without yielding or notifying in between. Credentials and retry:false/invalidateQueryCache:false stay with the future admitted adapter.

Preserve command response rules, including a scope-changed success only when returned match/life agree with the current pair; same-match older revision retention; server midpoint observation before revision rejection; original-scope error relevance; and current-generation finally clearing only its own busy state. Notifications must not let a stale continuation overwrite reentrant newer selection or work.

## Inputs and stop

Implement explicit caller-driven input ownership from send/submit/stop, with no HTTP client or automatic loop.

- Retain exact input fields: sequence, two move axes, yaw, pitch, attack, block and reload. Use validCombatInput's safe-integer and finite bounds without normalization or clamping. Preserve source radians and axis order.
- Ordinary input refuses while disposed, another input is in flight, command busy, unselected, missing state/life, invalid input or phase other than playing. Do not invent an active-fighter requirement for ordinary send.
- Use caller-supplied wall time for the 200ms send interval. Allocate sequence as max of supplied sequence, last sent sequence plus one and observed player's lastInputSequence plus one. Reserve the final safe neutral-release sequence exactly as the canonical less-than MAX_SAFE_INTEGER-1 condition does. Never wrap or silently clamp.
- Keep one input flight, captured immutable scope/input, last-send/input/sequence state and at most one pending neutral stop. Ordinary send is dropped while occupied, not coalesced into a new gameplay queue. Track explicit dispatch/settlement so an unmarked or duplicated native callback cannot release someone else's flight.
- stop invalidates input response ownership. With a flight, queue its original scope and a neutral input at flight.sequence+1 only when non-neutral and no stop is already queued. Retain yaw/pitch. Update the selected scope's sequence only when the flight belongs to it; clear lastInput as canonical.
- Without a flight, stop requires the displayed active fighter, selected instance/state/life, and a prior non-neutral input. Build a valid neutral input using max of lastSequence+1 and fighter.lastInputSequence+1, preserving the prior yaw/pitch. A neutral input has zero movement and all three booleans false.
- A flight's terminal completion releases exactly that flight and exposes the captured pending stop next, including after selection changes, last-observer loss, Clear or Dispose. Never retarget it to a new account, instance, match or life. The future sender must retain its original externally admitted transport context; a retained packet is not authority to send.
- Scope-changing observations, commands and lifecycle exits must capture stop before losing the old state. Reset selected input allocation at the canonical match/life/selection boundaries. Old read responses remain stale even while the old neutral packet can be drained.
- Neutral failures stay silent. Ordinary input success/error requires the captured subscription/input/match/life scope; a successful response itself may change life. Unchanged backend state can mean rate/duplicate refusal, not an acknowledged attack.
- Explain how disposal clears private display state yet permits only the already captured cleanup packet to drain, how its terminal result releases local resources, and who must keep the owner alive. No new gameplay input or command may start after disposal.

Use explicit stages/receipts if needed to preserve try/finally and notification reentry. Preparation failures, dispatch abandonment and completion must not strand command busy, an input flight or a neutral-stop packet. Do not recreate scheduling with permissive readiness inputs.

## Completion and exclusions

Checkpoint explicit owned paths with hooks disabled and write the owner handoff with source commit, input/preimage/output hashes, full command/input/stop coverage, adapter sequence, legacy compatibility and remaining integration. Review no-flight/in-flight/already-neutral/repeated-stop, selection and disposal during flight, same/different life/match, stale/duplicate callbacks, sequence exhaustion, 199/200ms boundary, notification reentry and all accepted decoder/read branches through source inspection only. Parent requires independent full Standards and Spec reviews.

No tests, lint, typechecks, builds/compilation, codegen, generator execution, asset conversion/import, launches, installation, remote writes/calls, deployment or live configuration. Preserve production renderer/mobile inputs. Account/readiness gates and world-control interlock remain disabled. Gameplay v1 entry/presence and Home read grants never authorize combat. Native transport/consent, world/control producers, first-person bindings, squad/hearing integration, content, compilation, in-game inspection and release remain incomplete.

## SHA-256 pins

```text
3c8fe8b68481774c6a886c9fa1c964c8c307ac40acb5d48322f8cf9adbb5e320  packages/api/src/companion-combat.ts
56da7b73be7622d6d0d7954c6ada30ffc7cf10d04bb3ddbfbb29559b4839078d  packages/utils/src/companionCombat.ts
19e1ef63adeeb51fde2d5c798e33a260906b261ae791c3f3fe9de5ed84d1f1c2  apps/backend/convex/companionCombat.ts
68c4352f1d5730a35f199efe5c0e8cc839e5fb8781dad5c0a308b75c213ef238  apps/backend/convex/lib/companionCombatModel.ts
0378f1692ebb343ff8dc734e72edd08011469ef9061c58f25c2db33a006ca94a  docs/engineering/unreal-combat-recovery-handoff-2026-10-10.md
f64ac3d2e0a1efe7d555e889fc466ba3e621dd7cfcd8963667851303fd1d9621  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionCombatState.h
a1d30a72e112df92b9ca0977c97c91754a315f6d24bd0eaf2d0391d39a2382a1  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionCombatState.cpp
13ceb1b13bee4b0793ddb117e03559bfb8ea77f8bdb8dc81baa1ffa8e70bb618  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.h
df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.cpp
90d230cf0233458c46324fd87d6f077c6f54bd7dd691030ba5a2a89e8607a17d  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionWorldPlayer.h
536bf39a4089acea43be36448ad3d02d3cd7f94a65015d3975f761941b802328  apps/companion-unreal/Source/BaddieCompanion/BaddieCompanion.Build.cs
```
