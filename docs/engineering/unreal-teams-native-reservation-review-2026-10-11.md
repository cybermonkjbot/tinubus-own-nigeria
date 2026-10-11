# Native Teams reservation review

October 11, 2026. Both full-range source reviews passed. Windows delivery remains pending.

## Fixed range and reconciliation

Assignment `unreal-teams-native-reservation-assignment-2026-10-11.md` is pinned at `b1e5cbad9d848912413d2b042c9b63ce3e1ba9b6`. Canonical/preimage/dependency base is `7c5af9661efad0fb007bd9ad26d5f3990d0f8512`. Owner turn `01a1286f-6b68-7ff3-823e-2132e826cd4c` completed at source `cea2a34a0725284fa410b00e26c33fe86eab01aa` and handoff `40d6b51333a0802d8428be410abd9d4a002061d3`.

Review the full three-path range, 147 additions and five removals:

```text
git diff 7c5af9661efad0fb007bd9ad26d5f3990d0f8512...40d6b51333a0802d8428be410abd9d4a002061d3 -- apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionTeamsState.h apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionTeamsState.cpp docs/engineering/unreal-teams-native-reservation-handoff-2026-10-11.md
```

Parent matched all thirteen original committed reference/preimage pins, eleven unchanged working references and all three committed/working output hashes. The handoff was absent at the base. Outputs:

```text
8ab87259890f857fcaf04809faa152db6c5968817846fda9bcdb33bcae2477ee  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionTeamsState.h
a170427959cfd421bf094a5031ac6faa6c16de0ecd78b4b88896ab2d439a0bea  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionTeamsState.cpp
137f817c91306b35c526ff7b9d9ad72d36a7c9e975890d8f1e6dc49e5589801f  docs/engineering/unreal-teams-native-reservation-handoff-2026-10-11.md
```

## Standards

Reviewer `01a12877-a3e0-79e0-8998-01ff2804e35c` completed the full three-path range with zero documented-standard breaches and zero actionable smell judgments. It checked root AGENTS.md, all twelve heuristics, the pinned assignment, canonical contracts and unchanged lifecycle consumers. The existing decoder, receipt and cleanup machinery is reused without a duplicate implementation or speculative abstraction. The reviewer is closed. Standards acceptance is complete; Spec remains separate.

## Spec

Reviewer `01a12877-a44f-7911-8df9-bcc1fe39ae38` completed the full three-path range with zero findings. It checked legacy/versioned decoding, unchanged numeric/null behavior, exact generation retention, UTF-16 bounds and ECMAScript whitespace, stricter malformed-response rejection, captured release arguments and legacy no-action finalization. It traced retirement and attachment errors, bounded cleanup failures, replacement/duplicate receipt handling, explicit Leave and matching-call checks, notification boundaries and external caller obligations. No missing scoped requirement, scope creep or incorrectly implemented behavior was found. The reviewer is closed.

## Acceptance limits

Both full-range axes are accepted and both reviewers are closed. Do not repeat these accepted reviews. The accepted backend/API cleanup at `7c5af9661` is a dependency, not a repeated review scope. Present malformed generation deliberately rejects native admission atomically, unlike the account store's typed response. Legacy cleanup emits no action; versioned cleanup retains exact admission identity. Explicit Leave and broader expiry/call/cross-player isolation remain outside this correction.

No native transport, call producer, permission, runtime mounting or UI was added. Gates remain disabled. Production renderer inputs are untouched. Source acceptance, Windows delivery, compilation, in-game inspection and release are separate. No tests, compilation, engine/content operations or deployment ran.
