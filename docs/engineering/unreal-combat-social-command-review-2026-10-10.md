# Unreal combat and social command review

October 10, 2026. Both owners completed their assigned source turns and passed both full independent reviews. Acceptance is source-only, not Windows delivery or gameplay verification.

## Fixed review scopes

| Slice | Canonical base | Complete reviewed head | Source commit |
| --- | --- | --- | --- |
| Combat | `964bb18fe0b5b97f481472d42c683bd5f408ec90` | `059e5099d94d4a4d877a4557e33318e21e0eac0e` | `fac5699a147bade400965390ec73ae4629c946f1` |
| Social | `149ec7f1929da0f9e80d58dc3c2cce8c9eeb6a6a` | `350e8f0e3301819012be771db8c7b8c212b309c5` | `95f4dc52a524ea0bbb6cac201008dec33c89c91a` |

Each review covers the full existing state pair and its new owner handoff, not a representative method or incremental patch. Combat has 468 additions and 14 removals across three paths. Social has 511 additions and no removals across three paths. Parent resolved both ranges, confirmed nonempty three-path diffs and matched all six current working hashes to the pinned heads.

Combat's completed owner turn is `01a12805-3c14-7b33-bc72-a6b60b4fee43`, owner `01a12714-f810-7ac1-8171-bc6d7c8c20d6`. Its spec is `unreal-combat-command-assignment-2026-10-10.md` at `443dd04e7be54e0efe39ef6e02ed48c85dbaaa74`. The source adds join/leave/robbery packets, input sequencing/rate/one-flight state and captured neutral-stop draining.

Social's completed owner turn is `01a12808-04bf-7d03-8c17-f659d75d1e2a`, owner `01a12728-2e34-7ea3-9615-22f614dd0d9b`. Its spec is `unreal-social-command-assignment-2026-10-10.md` at `7b42b022eff5994a6520585e415d3a41457c2cd1`. Source adds offer/respond/transferCredits packets, complete results and independent original-caller completion lifetimes.

Both depend on unchanged complete StateJson from `b4bc1b7d7dbc9552ad1451062a404fc872a13224`. Neither implementation edits shared source or any production renderer/mobile input. The separate Teams command assignment remains in progress and outside these reviews.

## Standards

| Slice | Independent reviewer | Status |
| --- | --- | --- |
| Combat | `01a12813-8296-7ce2-aac6-0d8e9601e6f8` | Accepted; zero documented breaches and two deferred judgments |
| Social | `01a12813-83c6-7fb1-927f-6edb61977467` | Accepted; zero documented breaches and one deferred judgment |

Review uses repository instructions and the code-review skill's separately labeled smell judgments. No compiler, test or lint run is authorized.

Combat Standards found no ownership or reuse breach. The change extends the existing owner, retains its decoder, uses UE serialization and preserves StateJson. Two optional judgments remain: duplicated completion acquisition/dispatch-time receipt selection/completing/finalization at CombatState.cpp lines 576-582 and 626-632; and private command identity inferred from an endpoint string and nonempty action at lines 375-395. A shared completion helper and private command enum could reduce maintenance risk. The existing wrappers constrain the combinations correctly; neither is a demonstrated defect. Both are deferred.

Social Standards found no documented breach against AGENTS.md, CONTEXT.md or the assignment's ownership/reuse requirements. Existing decoder reuse, exact owner/identity/stage dispatch checks, caller-owned completion and unchanged StateJson were confirmed. One low-priority Duplicated Code judgment remains in the three preparation methods: repeated Encode failure handling followed by PrepareCommandPacket at SocialState.cpp lines 537, 550 and 571. This is optional orchestration cleanup, not a second domain implementation or observed defect. It is deferred to keep the accepted behavior unchanged.

## Spec

| Slice | Independent reviewer | Status |
| --- | --- | --- |
| Combat | `01a12813-834b-7c82-8df1-1834506d3c00` | Accepted; zero actionable findings |
| Social | `01a12813-843f-7d83-81ba-c3c4d8aa89e7` | Accepted; zero actionable findings |

Review covers each complete assignment and its canonical callers, including packet values, response rules, copied/stale receipts, notification ordering, disposal, original-scope cleanup and finally ownership. Missing native transport, permission, call/world producers and content remain explicit exclusions, not accepted runtime behavior.

Combat Spec found no missing/partial requirement, scope creep or incorrect implementation in the full range. It checked exact command arguments and eligibility, captured scope, post-notification checks, input bounds, sequence exhaustion, 200ms accounting, neutral-stop capture, one-time dispatch and unsent abandonment. It also traced default/copied/stale/foreign receipts, duplicate completion, synchronous reentry, midpoint-before-revision ordering, scope-sensitive errors and lifecycle cleanup. Decoder/read contracts remain preserved. Consolidated native notifications are an existing documented difference, not new behavior. Accepted input responses do not acknowledge an attack or establish gameplay success.

Social Spec found no missing scoped requirement, scope creep or incorrect implementation in the full range. All three command encoders preserve explicit arguments, false and omitted/empty options. Result decoders retain every endpoint field, nullable thread, false success and returned numbers within documented bounds. Independent one-shot receipts reject default, foreign, unmarked and duplicate completions while allowing out-of-order original-caller settlement after view/identity changes or disposal. Completion retires only its own lifetime and never publishes into read state. Existing reads remain unchanged. The adapter contract retains immediate dispatch and endpoint-specific retry/cache policy. Accepted decoding does not establish financial success, messaging permission or game admission.

## Integration and delivery hold

All four reviewers are closed. Both full ranges are accepted on both axes. Standards totals are zero documented breaches and three deferred maintenance judgments; Spec totals are zero actionable findings. No correction or repeated review is required for these accepted pins.

Social is accepted at source `95f4dc52a524ea0bbb6cac201008dec33c89c91a`, handoff `350e8f0e3301819012be771db8c7b8c212b309c5`. Both Social reviewers are closed. Combat is accepted at source `fac5699a147bade400965390ec73ae4629c946f1`, handoff `059e5099d94d4a4d877a4557e33318e21e0eac0e`. No new native delivery is queued while the existing Economy/POS handoff awaits coordinator reconciliation. A later accepted batch must include complete state pairs, matching StateJson, all consumed dependencies, immutable source pins and verified destination preimages. Coordinator alone owns Windows writes.

Account/readiness gates and world-control interlock remain disabled. Local packets and decoded results provide no authority for combat, social writes, credit transfers, world entry or voice. Source implementation, source review, Windows delivery, imported content, compilation, in-game inspection and release remain separate.

## Parent-matched source hashes

### Combat

```text
bbdebfa73a9d2e00579031711a640435f4d9b88dde4d81d2cee0fb3b60b54e4c  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionCombatState.h
57891ae3a067c42f40efe8de8148a5c0b23a5a469bea2c653c7861eea0dd0db2  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionCombatState.cpp
126f65efeca41a56847424f6c11db9204a398837e9c4a3a3940d4bba64814f24  docs/engineering/unreal-combat-command-handoff-2026-10-10.md
```

### Social

```text
4e9d712d00e1df4c6480fb9954dc2e3f63d7dd9cabbc1c9d1852c484c6fd89e9  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionSocialState.h
a5bf55fc69204eff538a7cc49506212ca6a3cf7e0e7648bf3701a30b03e5acd6  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionSocialState.cpp
f84643cfba4795dfd13054c86846b98bd029bfcf74df35b27bf8c08766079831  docs/engineering/unreal-social-command-handoff-2026-10-10.md
```
