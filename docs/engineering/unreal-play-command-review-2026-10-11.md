# Unreal Play command review

October 11, 2026. The complete source change passed independent Standards and Spec review. Windows handoff remains a separate pending step.

## Pinned scope

- Canonical/preimage/dependency base: `cfa15652c52e510aac532a8cc4e13ce633d8415e`.
- Assignment: `unreal-play-command-assignment-2026-10-11.md` at `1de31c66058254648f8e4d7dab055dbf443fe98b`.
- Source: `4ac907671ab6d0bee65feb5185a4c21b0eb212d1`.
- Handoff/end: `eb82036be4fdc7a92a6e832e9593df765b43ca6b`.
- Completed owner turn: `01a12853-1347-7381-953b-5e96e03712a6` in `01a12714-f0c9-7153-8f81-94e2816fca6e`, host local.

Both reviews cover the full three-path range, 747 additions. Parent matched all eleven pinned references and all three working output hashes before dispatch.

```sh
git diff cfa15652c52e510aac532a8cc4e13ce633d8415e...eb82036be4fdc7a92a6e832e9593df765b43ca6b -- apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionPlayState.h apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionPlayState.cpp docs/engineering/unreal-play-command-handoff-2026-10-11.md
```

Output SHA-256:

```text
6ba177bce942793406ed29cb927f3d10142a1963e5beb513ec81c377a52d9c63  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionPlayState.h
5095cf81315f6f250911008568ad44e48b6a8ab904bc1021d14a9b41e84d2924  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionPlayState.cpp
330ed17de2284048c8540a227a13cf1da14ac1254e33ab192c5f413221e33bb5  docs/engineering/unreal-play-command-handoff-2026-10-11.md
```

## Standards

Reviewer `01a12860-ef01-7c11-9338-b1bc25aec1c8` completed the full range with zero documented-standard breaches and one low-priority heuristic judgment. The reviewer applied root AGENTS.md, the Unreal-first/shared-reader rules and the complete twelve-item smell baseline.

Possible Duplicated Code appears in PlayState.cpp at pinned lines 570, 587 and 653: preparation failure, abandonment and finalization each remove the owned request, clear busy and emit. The abandonment hunk is `Requests.Remove(Receipt.Id); Snapshot.bBusy = false; Emit(); return true;`. A later bookkeeping change would need coordinated edits. This is not an observed behavior defect or a documented-standard breach.

Parent inspected all three locations and defers extraction. Their identity/error preconditions remain explicit, and source acceptance requires no behavioral correction. The reviewer is closed.

## Spec

Reviewer `01a12860-efa9-7580-8060-6948a7bf3efd` completed the full range with zero findings: no missing/partial requirement, scope creep or incorrect implementation identified.

Coverage includes all eighteen commands and their numeric/enum/palette/optional-value bounds; complete null/different/equal/newer wire replacement and older-wire retention; unchanged saved-appearance decoding; exact scope/session/query ownership; no-observer eligibility; expected-round capture after notification; once-only dispatch/completion; overlapping commands after query failure; caller-alias and reentry protection; midpoint offset and failure/finalization order. Utility clear/disable boundaries are documented without inventing observations or permission.

Evidence at the pinned end includes command encoding at PlayState.cpp line 295, lifecycle/preparation at line 420, read reducers at line 495, receipts at line 576 and completion at line 606. The utility deferral begins at handoff line 118. Parent accepts this source axis. The reviewer is closed.

## Limits and continuation

Both original reviewers are closed. Do not repeat this accepted review or silently include later changes in its acceptance.

Play's retained reads and eighteen commands are source-complete and accepted for a guarded source handoff with the matching complete StateJson pair. Native permission, transport, utility observation, UI/render bindings, compilation, in-game inspection and release remain unfinished. Keep all gates disabled.

Summary: Standards has zero hard breaches and one deferred low-priority judgment. Spec has zero findings.
