# Native Teams reservation continuation handoff

October 11, 2026. Source implementation only.

## Pins and status

- Assignment `docs/engineering/unreal-teams-native-reservation-assignment-2026-10-11.md` at `b1e5cbad9d848912413d2b042c9b63ce3e1ba9b6`.
- Canonical, preimage and accepted dependency base `7c5af9661efad0fb007bd9ad26d5f3990d0f8512`.
- Native source commit `cea2a34a0725284fa410b00e26c33fe86eab01aa`, containing only the Teams header and implementation below.
- Accepted backend/API source `29d723cbec749a8f39d114c81c1cb0f470c32707`, handoff `f44138e74af01d70420899c26ebffe8b3f8eb7d1`. Both complete six-path reviews passed with zero findings. This continuation does not repeat that review.
- Original native commands at `1f6c60f90d01cf0ea1eb1e8bb9a6cd7f65ef8bfc`, handoff `7900099f44c8e08fbe447fe750cc74a171867f90`, have already passed review and reached Windows per the assignment. Their historical pending statements do not describe this continuation's status.
- Shared StateJson dependency `b4bc1b7d7dbc9552ad1451062a404fc872a13224` remains unchanged.

Implementation and source inspection are complete. Parent-owned independent Standards and Spec reviews must cover the full three-path change before acceptance. Windows delivery, compilation, in-game inspection and release of this continuation remain separate and were not performed.

## Admission contract

`FTeamAdmission` adds `TOptional<FString> ReservationGeneration`. The decoder accepts exactly the original six required fields, or those six plus `reservationGeneration`. Required-field reads and exact object field counts still reject missing required fields and unknown additions. `roomId` and `threadId` remain the returned public-city identities, not values reconstructed from the selected crew. `callSessionId` remains required and nullable. `startMuted` must remain literal `true`.

The original numeric decoder is unchanged. `startedAt` and `expiresAt` accept finite nonnegative numbers up to `9_007_199_254_740_991`, including fractions. This continuation does not tighten them to the release endpoint's safe-integer timestamp range. A captured time outside that endpoint's contract can therefore produce a rejected release, handled by the existing cleanup-failure path.

A missing generation is valid legacy data and remains unset. A present generation must be a string, at most 128 UTF-16 code units, with some content outside the ECMAScript trim whitespace set. The reader counts each UTF-16 surrogate half once, and each supplementary scalar twice on a wider-TCHAR platform. The retained value is the decoded string without trimming, normalization, UUID parsing, case changes or the existing Identifier predicate.

The explicit whitespace set is `U+0009` through `U+000D`, `U+0020`, `U+00A0`, `U+1680`, `U+2000` through `U+200A`, `U+2028`, `U+2029`, `U+202F`, `U+205F`, `U+3000` and `U+FEFF`. A nonempty string consisting only of these characters is rejected. `U+0085`, `U+180E` and `U+200B` are content, not trim whitespace. The generation predicate adds no general control-character prohibition.

Present malformed generation rejects the whole admission at the existing atomic decoder boundary. This is deliberately stricter than the account store, which trusts its typed admission for attachment and only skips cleanup when generation metadata is malformed. Native decode failure clears its output and `AcceptMutationResult` never assigns `Work.Admission`; catch proceeds to `Finally` without a cleanup packet. Null, number, boolean, array, object, empty, whitespace-only and overlong generations all fail this boundary.

## Cleanup contract and caller obligations

At `CleanupReady`, a versioned admission emits `ETeamsActionKind::ReleaseReservation` with operation `companionGameTeams:releaseReservation`. Its non-auth JSON contains exactly `roomId`, `startedAt` and `reservationGeneration` from `Work.Admission`. The room and time are the original admission values, and generation is retained unchanged. No current selection, displayed team, fresh call or replacement entry contributes cleanup identity.

The former `CleanupLeave` enum name had no source consumers outside the owned pair. The rename preserves its enum position. Explicit command encoding remains unchanged, including `companionGameTeams:leave` with only `roomId` and `startedAt`.

A legacy admission advances from `CleanupReady` directly to `Finally` and returns `Advanced`. `MarkNextAction` resets both output arguments before that branch, and the early return precedes action-token creation. It emits no action receipt or packet. Existing result, failure and finalization state is retained. There is no ordinary-Leave, timestamp-only or re-entry fallback.

Successful muted-call attachment still copies the entire admission and changes only `CallSessionId`. Generation therefore survives the post-join copy. Automatic cleanup never requests `calls.leave`, call-participant removal, group departure, grant revocation or voice enablement. Explicit Leave alone retains the exact captured room/thread/nullable-call recheck before `calls.leave`.

The continuation remains a local request/receipt model. The original account adapter must retain its original credentials and call controller externally, use `retry: false` and `invalidateQueryCache: false`, and return the release endpoint's `null` value to `AcceptMutationResult`. The native action contains no authentication fields and provides no transport or permission implementation.

Caller boundaries remain unchanged:

1. Prepare captures command and initial call, sets busy, and returns for caller notification. `ContinueCommandAfterNotification` then captures `SelectedAtStart`.
2. Supply fresh original-controller observations at `AttachPresence`, `CheckJoinedCall` and `CheckLeavingCall`. Dispatch the exact action immediately after `Dispatched`, without a yield or notification between marking and sending.
3. Complete the separate attachment await boundary with `CompleteAttachment`, even after same-game attachment. Once the stage is `Finally`, call `FinishCommand` without another yield or notification.
4. Keep the state object and sent receipts alive through finalization, including after clear/disposal. Deliver any result only to the original caller and notify only when `FinishCommand` returns `bNotify: true`.

## Source-inspected branches

These conclusions come from source and diff inspection, not executed tests or runtime checks.

| Branch | Result |
| --- | --- |
| Six-field legacy admission | All original required-field, nullable, numeric and muted-start checks remain. Generation stays unset; cleanup advances without a receipt. |
| Seven-field versioned admission | Valid generation is retained exactly and enables captured reservation release. Unknown additional keys still fail the object-count and required-field checks. |
| Present malformed generation | Atomic decode clears output; command admission is never captured; no cleanup is constructed. |
| 128 versus 129 UTF-16 units | The counter accepts 128 with non-whitespace content and rejects as soon as it exceeds 128. Sixty-four supplementary characters consume 128 units; adding one BMP character consumes 129. |
| Surrounding whitespace | Whitespace counts toward the 128-unit limit. Nonempty content permits surrounding whitespace, which remains in the release argument unchanged. |
| All-whitespace, including BOM/NBSP | The explicit set includes BOM and NBSP and all listed ECMAScript whitespace/line terminators. Empty and all-whitespace strings fail `bHasContent`. |
| Same-city replacement entry | Detached work retains its original generation even if room ID and start time remain identical. A newer work receipt cannot replace its admission. The accepted backend's exact session/generation checks decide whether the old release has effects. |
| Retirement after admission response | `AcceptMutationResult` captures a valid response but enters `DisposedCleanup` when the original work is no longer live. Retirement before the next attachment action is also rechecked in `AttachPresence`. Neither path starts a new join. |
| Retirement after attachment | `CompleteAttachment` rechecks liveness at the separate await boundary and enters cleanup instead of returning the admission. The whole-admission copy preserves generation after a successful call join. |
| Attachment error or call mismatch | Existing failure completion or call validation enters catch. It clears the result and cleans only the captured admission, preserving the other call. |
| Disposal cleanup fails | `CleanupFailed` enters catch for one final captured release. A new action token is minted for that attempt. The same fields are used again. |
| Catch cleanup fails | Failure is swallowed and the stage becomes `Finally`. No loop, third attempt or ordinary-Leave fallback is introduced. Invalid cleanup response JSON uses this same failure path. |
| Duplicate action completion | Completion clears `Work.Action`. `CurrentAction` requires the exact retained token and a sent stage, so repeats and earlier-attempt tokens are rejected, including during a later cleanup attempt. |
| Same-ID identity replacement | Clear detaches old work and resets its displayed owner. Reinstalling the same identity string creates a different identity token. Old receipts retain store ownership and can drain cleanup, but fail `LiveWork` and cannot clear the replacement busy/error or notify it. |
| Selection change | Cleanup continues to use the captured admission. A caught error only reaches the live view when the selection still equals `SelectedAtStart`; it never overwrites another selected group's error. |
| Explicit Leave | Original operation and arguments remain unchanged. The post-mutation call check still matches game room, thread and nullable call session before emitting `LeaveMatchingCall`. |
| Successful attachment and legacy busy API | Result/finalization semantics, all five explicit encoders, read/expiry behavior, retained null fields and the separate observed-command busy-owner API remain unchanged. |

## Reference and output hashes

All thirteen assignment hashes matched committed and working preimages before editing. The eleven read-only reference files still matched after the source checkpoint. The handoff path was absent before work. The two source output hashes below match the source commit and working files.

```text
ac5215c3bc24d4d47bc1a75229d6d9b3b3103ea2c3b0b1de895cb0782850357e  apps/backend/convex/companionGameTeams.ts
6f1e333c0900610ec4fdce0bef33d37c78b519d5fe5335748f1d243529c44b26  apps/backend/convex/lib/companionGameTeamsModel.ts
58ae88bdd6714833c15a278b173a01172913a02f2b715f8eefd8578c6b6a1a3c  packages/api/src/companion-game-teams.ts
155b6a6cd926949b63a43db01c503de25b9366b442126b0fa4ec4ceeaf7cbe11  packages/utils/src/companionGameTeams.ts
564d96a85bc381760292a3e686240b85a3da9b7bd40df881acf04a095e71a308  packages/utils/src/companionCall.ts
392e158eaf269fe40713dd2827fe2918374bd153a342fb5d51c781cc3eb8b5e4  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionTeamsState.h
492957967e12fa36fc3f089b41b43d63911cd3c3b768d3c6120fd9aefa2505d5  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionTeamsState.cpp
13ceb1b13bee4b0793ddb117e03559bfb8ea77f8bdb8dc81baa1ffa8e70bb618  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.h
df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.cpp
536bf39a4089acea43be36448ad3d02d3cd7f94a65015d3975f761941b802328  apps/companion-unreal/Source/BaddieCompanion/BaddieCompanion.Build.cs
b0af4f4f982fcb16a9c3398835f5e8357c116c19021cac5bd35ffd10f2532424  docs/engineering/unreal-teams-command-handoff-2026-10-10.md
98ebf35fed934442d9fb39736719a0e2b60650954f1c9b7212948e292ffab7f6  docs/engineering/unreal-teams-reservation-cleanup-handoff-2026-10-11.md
7373fb063e4ad40e3aa99127c1691cca6b7b44a83affd83f294523596f4c943a  docs/engineering/unreal-teams-reservation-cleanup-review-2026-10-11.md
```

Source outputs:

```text
8ab87259890f857fcaf04809faa152db6c5968817846fda9bcdb33bcae2477ee  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionTeamsState.h
a170427959cfd421bf094a5031ac6faa6c16de0ecd78b4b88896ab2d439a0bea  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionTeamsState.cpp
```

## Limits and verification

The accepted account endpoint protects matching reservation release with session/generation checks and the native-owner guard before transit/guest effects. That does not authorize native transport. Ordinary public-city Leave still lacks these new generation/native-owner protections. General expiry, call cleanup and broader cross-player/shared-instance release effects remain unresolved as recorded in the backend cleanup handoff and review. Abandoned legacy admissions and rejected cleanup remain with those existing lifecycle owners.

No account credentials, current-call producer, native transport, world/mission/combat/voice/payment/editing authority, runtime mounting or UI were added. Gates and the world-control interlock remain disabled. Backend, mobile, shared source, StateJson, Build.cs, other native modules, historical handoffs and the tracker were not edited. These standalone native changes do not trigger renderer packaging or What's New changes.

Verification was limited to source/consumer inspection, complete scoped diff review, SHA-256 comparison and scoped `git diff --check`. The source checkpoint used hooks disabled and only the owned pair after confirming the shared index was clear. This handoff is the third owned path and is checkpointed separately. Unrelated concurrent work was preserved.

No tests, typechecks, lint, builds, compilation, codegen, asset operations, engine/server/Blender launches, installs, live backend calls, remote operations, deployment or configuration changes were performed. No other chat was messaged. The writing-for-agents skill shaped the contract and acceptance sections; unslop guided the prose edit.
