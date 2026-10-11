# Unreal Football lifecycle review

October 11, 2026. The complete corrected range passed both independent source reviews. Windows delivery is separate and pending.

## Pinned scope

- Base: `cfa15652c52e510aac532a8cc4e13ce633d8415e`.
- Assignment: `unreal-football-lifecycle-assignment-2026-10-11.md` at `1de31c66058254648f8e4d7dab055dbf443fe98b`.
- Source: `e766d0eee5508526f88708d7272409266cfe8839`.
- Original handoff: `a9d0c144e87c826d4a902485d3e9875578cd951e`.
- Correction source: `0b828eace1512ef92c379b66768e28832efc9831`.
- Corrected handoff/end: `8e2715438a6c4b2ea2c1c0cfbcb9e9c41934fd04`.
- Completed correction turn: `01a12869-7723-7e93-8eb3-15f9196080ce`.
- Completed owner turn: `01a12853-1167-7d53-b8e4-2623acc5c7c6` in `01a12714-f810-7ac1-8171-bc6d7c8c20d6`, host local.

Parent matched all thirteen canonical/preimage/dependency references and all three working outputs. The original range had 954 additions and 23 removals. The full corrected range has 978 additions and 23 removals. Parent matched all three corrected working output hashes.

```sh
git diff cfa15652c52e510aac532a8cc4e13ce633d8415e...8e2715438a6c4b2ea2c1c0cfbcb9e9c41934fd04 -- apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionFootballState.h apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionFootballState.cpp docs/engineering/unreal-football-lifecycle-handoff-2026-10-11.md
```

Output SHA-256:

```text
692bd9266380e4690ac92a949e4c7cb23e0c40272e699d33ca36a22e805a911e  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionFootballState.h
13b40d63a9fb245aa42826275b4b67511ed1518e5368fff23b2024158dab5475  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionFootballState.cpp
77272300e815d0a3110e0e2964e1730123729cd761d673fcf555902f6246f180  docs/engineering/unreal-football-lifecycle-handoff-2026-10-11.md
```

## Standards

Reviewer `01a12863-545d-7ea0-9719-d790889cbbaa` completed the full range with zero documented-standard breaches and three optional P3 judgments. It is closed. The judgments are possible Duplicated Code in the scope/phase resets at pinned lines 595/603 and input-envelope construction at 953/965, plus possible Repeated Switches in the const/non-const effect-slot mappings at 849/860. Header and handoff findings were zero.

Parent inspected all three locations. These are maintenance suggestions rather than behavior defects. Extraction remains deferred; retain the different sequence/state transitions and const contracts. Corrected-range Standards reviewer `01a1286b-f5fd-7ce1-861f-78ae7ad28086` independently reviewed the complete original range through the corrected end. It found zero hard breaches and the same three optional judgments, with no new Standards issue in Playing. Parent accepts the corrected Standards axis. Both Standards reviewers are closed.

## Spec

Reviewer `01a12863-54ea-75b1-a6d7-a2ee65fe7be2` completed the original full range with one P2 finding. Playing at pinned FootballState.cpp line 812 finds the first matching ID and then checks connection, while the canonical store at line 23 finds any matching connected player. The decoder still accepts duplicate IDs. A disconnected row followed by a connected row therefore wrongly disables native input, neutral flushing and pumping. Assignment line 33 requires the exact connected-current-player predicate. No other actionable requirement gap was reported.

Parent reproduced the predicate difference by source inspection and assigned the original owner a correction limited to Playing eligibility and its handoff. The correction preserves CurrentPlayer semantics for its three other consumers and preserves decoder acceptance. The same Spec reviewer re-reviewed the complete corrected three-path range, confirmed the P2 resolved and found zero remaining or new findings. It traced opening/retry, input/neutral sequencing, focus/pump, NPC fallback, offset, release/teardown and notification/alias ownership. Parent accepts the corrected Spec axis. The reviewer is closed.

## Continuation

All original and corrected-range reviewers are closed. Do not repeat these accepted source reviews. The corrected source is eligible for a separately pinned guarded Windows delivery with complete matching StateJson and dependencies.

The source implements opening/manual retry, input/neutral sequencing, focus, pulse scheduling and final-effect staging. Transport/admission, actual listener/timer binding, locale compatibility, compilation, in-game inspection and release remain unfinished. This acceptance covers this lifecycle source continuation only. Gates remain disabled.

Summary: corrected Standards has zero hard breaches and three deferred P3 judgments. Corrected Spec has zero remaining or new findings.
