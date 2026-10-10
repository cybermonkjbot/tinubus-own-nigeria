# Unreal recovery request assignment

October 10, 2026. Local command capture only.

## Owner and pins

Existing owner `01a12714-f810-7ac1-8171-bc6d7c8c20d6`, host local, has completed Life commands and both reviews. Its new exclusive scope is RecoveryState.h/.cpp under `apps/companion-unreal/Source/BaddieCompanion/Public` and `Private`, plus new `docs/engineering/unreal-recovery-command-handoff-2026-10-10.md`. Life files are frozen for the active Windows delivery. Combat, Food, Kitchen, Football, Racing and earlier handoffs remain read-only. Parent owns StateJson, tracker and acceptance; coordinator alone owns Windows writes.

Canonical references and owned preimages are pinned at `6fa1fa38d58738a95098e61d5b627a6c17f3c601`. All eleven hashes below match working bytes; the new handoff was absent. Recheck before editing. Reuse unchanged complete StateJson from `b4bc1b7d7dbc9552ad1451062a404fc872a13224`.

## Missing behavior

The accepted FRecoveryReadState already decodes health/null and recovery results, captures externally admitted recovery receipts, owns busy/generation state and applies canonical response ordering. Keep it. The missing part is immutable serviceId/lifeId request preparation and explicit dispatch/abandonment, not another recovery store.

Read complete `packages/api/src/companion-recovery.ts` and the Recovery sections of the previous combat/recovery handoff. Add a prepared request for `companionRecovery:recover` with exactly serviceId and the currently displayed health's lifeId, using UE JSON serialization. Keep credentials with the future adapter. Preserve the original exact strings. Match the backend serviceId rule, nonempty and at most 100 characters. Any additional native size ceiling must be explicit in the handoff and must not become invented medic eligibility.

Capture life ID, owner, callback generation and start time before notification. A caller cannot substitute a life or rewrite the payload after preparation. New preparation must respect existing active-subscription, observed-health, non-busy and valid-time predicates. Reuse accepted capture/application logic rather than copy it. Keep the existing receipt-only API compatible; distinguish an owned prepared request from legacy external bookkeeping.

Preparation may notify through the existing delegate. Recheck exact owner/generation/request after notification and at dispatch so selection, clear, reconnect, last-observer removal or disposal cannot expose a usable stale request. Mark dispatch once without notification, returning immutable endpoint/arguments for immediate sending. The future admitted adapter must not yield or notify between that successful mark and sending. A prepared request abandoned before dispatch clears only its matching busy/payload state and never creates a fictional ambiguous response. Sent requests cannot be abandoned as unsent. Reject default, copied duplicate, foreign, stale and unmarked-owned completion receipts without settling newer work.

Keep the canonical transport options `retry: false` and `invalidateQueryCache: false` as future adapter obligations. No manual retry slot, generated idempotency ID, timer or send loop is required.

## Response preservation

Preserve the full accepted decoder and all read/response behavior:

- Subscription health/null replaces the displayed value unconditionally within the current generation, including lower revisions and different lives. It clears error and sets ready.
- Mutation success may create a new life. Do not require returned lifeId to equal captured lifeId. Accept only when no current health exists or returned revision is equal/newer, including cross-life comparisons. Older mutation revisions retain health and clock.
- Accepted mutation observes the existing midpoint clock. Success does not independently set ready or clear an intervening read error.
- Mutation failure publishes its existing fixed error only while displayed health still has captured lifeId. Null/different current health suppresses that error, while the matching current busy request still settles.
- Preserve same/changed owner, subscription begin/end, Clear and terminal Dispose behavior and coherent post-update notifications. No writes after notification may overwrite reentrant newer state.

Local prepared ownership is not permission. No treatment, death/new-life settlement, hospital/station countdown admission, relocation, medic approach, payment, world control or simulation is implemented here. Call presence, health data and saved Home access cannot authorize recovery.

## Completion

Implement only owned files with apply_patch and checkpoint explicit paths with hooks disabled. The owner writes the new handoff with exact source commit, input/preimage/output hashes, compatibility behavior, adapter sequence and remaining gaps. Source review covers immutable capture, service bounds, notification reentry, mark/abandon, stale/duplicate completion, all existing null/lower/cross-life response cases and identity clearing. Parent requires full separate Standards and Spec reviews.

No tests, lint, typechecks, compilation/builds, codegen, generators, imports/conversion, launches, installs, remote writes, live calls, deployment or configuration changes. Preserve renderer/mobile inputs and disabled account/readiness gates and world-control interlock. Native permission, authentic transport, medic/world producer, mounting, content, compilation, in-game inspection and release remain incomplete.

## SHA-256 pins

```text
bbbf79d23adc51bd8d7ef693a4ff7c8b63caf190db4affeaa631f6b7d1d2fd52  packages/api/src/companion-recovery.ts
e4ec7d77076df9713fa4177e4abc0b71968231d8f4538660665652c9a2882d79  packages/utils/src/companionRecovery.ts
79db41dc4657010ef31df042c220996db258230eb21e744b1fc7a2b405e8b971  apps/backend/convex/companionRecovery.ts
df76e2a3b8db23c5ebb3cec9f8d8d760016ced75963e9ea81f2a65452f9dc2c0  apps/backend/convex/lib/companionRecoveryModel.ts
0378f1692ebb343ff8dc734e72edd08011469ef9061c58f25c2db33a006ca94a  docs/engineering/unreal-combat-recovery-handoff-2026-10-10.md
2c874a564ef777217630435549a1f10d36ac7215d509e59c82b48122326d77c7  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionRecoveryState.h
e47db6550d6b173aaa8e24dda2ddb7cbb5f18893cd83375b7b2084911059562a  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionRecoveryState.cpp
13ceb1b13bee4b0793ddb117e03559bfb8ea77f8bdb8dc81baa1ffa8e70bb618  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.h
df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.cpp
90d230cf0233458c46324fd87d6f077c6f54bd7dd691030ba5a2a89e8607a17d  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionWorldPlayer.h
536bf39a4089acea43be36448ad3d02d3cd7f94a65015d3975f761941b802328  apps/companion-unreal/Source/BaddieCompanion/BaddieCompanion.Build.cs
```

