# Unreal recovery request handoff

October 10, 2026. Local request ownership, source-only verification.

## Scope and pins

Source commit `b8c7b8891953d17f56905cc914df66ebb639363c` changes only `BaddieCompanionRecoveryState.h` and `BaddieCompanionRecoveryState.cpp` under `apps/companion-unreal/Source/BaddieCompanion/Public` and `Private`. This handoff is the only additional owned file.

The assignment is `docs/engineering/unreal-recovery-command-assignment-2026-10-10.md` at `46ba40508da8f809ce90097f4e98fe7baa33674f`. Canonical references and source preimages are pinned at `6fa1fa38d58738a95098e61d5b627a6c17f3c601`. The complete unchanged StateJson pair comes from `b4bc1b7d7dbc9552ad1451062a404fc872a13224` and remains required for transfer.

`FRecoveryReadState` now owns immutable preparation, a one-time dispatch mark and unsent abandonment. It reuses the existing capture and response paths. It installs no transport or native permission and changes no backend admission, recovery simulation, world state, renderer, mobile input or gate.

## Future adapter sequence

1. Establish native account/session authority separately. Bind callbacks to the selected owner and current subscription generation on one owning thread. Keep the state owner alive through each synchronous call and notification. Access loss or a session change requires `Clear` or `Dispose`; an unchanged `SelectOwner` is intentionally a no-op.
2. Call `PrepareRecovery(ServiceId, BeganAtMs)` with the actual start wall time. It requires an active subscription, observed non-null health, an idle owner and a valid timestamp. It captures the exact service ID, displayed life ID, owner, generation and start time before publishing busy state and clearing the old error. Notification reentry that retires the request makes preparation return empty without changing any newer request.
3. Use the returned receipt's const `GetArgumentsJson()` only for inspection and separate admission. It contains exactly `serviceId` and `lifeId`. Complete adapter checks before dispatch. If an unsent request will not proceed, call `AbandonPreparedRecovery` with that receipt. Only the matching current, unmarked request is retired; abandonment clears busy and owned payload without adding an error or ambiguous outcome.
4. Call `DispatchRecovery` with the captured receipt. It rechecks owner, generation and request identity, rejects an already marked request, and returns a copied `FRecoveryDispatch` without notifying. On success, immediately send its `GetArgumentsJson()` to `GetEndpoint()`, which is `companionRecovery:recover`. Do not yield or notify between marking and sending. Attach credentials only in the future admitted transport. Use `retry: false` and `invalidateQueryCache: false`.
5. Retain `GetReceipt()` by value for the callback. Decode the mutation response with the existing decoder, then call `Accept` with the actual receipt wall time, or `AcceptFailure` for failure. Before the dispatch mark, both owned completion paths reject without settling busy. After the mark, unsent abandonment rejects. The first current terminal outcome retires the request, so repeated callbacks cannot settle a newer request.

Only a successful optional dispatch result permits this adapter sequence to continue. A default dispatch object is not evidence of admission. A caller-held payload or dispatch copy survives owner clearing, so it must never be saved for later sending or replayed. The state can enforce a one-time mark, not prevent an external adapter from sending a retained string twice. There is no manual retry slot, idempotency field or send loop.

## Payload and compatibility

UE JSON serialization writes exactly two string fields. `ServiceId` must be nonempty and at most 100 `FString` code units. Case and whitespace are preserved, including whitespace-only strings within the bound. The life ID comes from displayed health, never a caller argument. Later health/null subscription updates cannot rewrite this captured payload. No medic, hospital, countdown or health-status admission check was added.

There is no additional native request-size ceiling. The unchanged health decoder's 32,768-code-unit document limit still bounds the source of the captured life ID. Existing strict decoding, nullable fields and numeric rules remain as recorded in `docs/engineering/unreal-combat-recovery-handoff-2026-10-10.md`, including its shared JSON correction section.

`CaptureRecovery(double)` remains externally admitted receipt-only bookkeeping. Its receipt has empty argument JSON, needs no new dispatch mark and completes through the existing methods. It cannot use owned dispatch or abandonment. Both paths share capture predicates, timestamps, busy/error publication and response application. The shared post-notification identity check now also withholds a legacy receipt retired during notification.

Receipt fields are private. The selected owner is compared exactly in addition to generation and active request GUID. These GUIDs are local callback identity, not request arguments, credentials or server idempotency keys. Default, foreign, stale and already completed mutation receipts reject without clearing current work. A copied current receipt remains the same request and can complete it once. Subscription receipts intentionally remain usable for repeated current-generation updates.

## Preserved response and lifecycle behavior

| Case | Result |
| --- | --- |
| Current subscription health/null | Replaces displayed health unconditionally, including lower revision or different life. Sets ready and clears error. Does not retarget or retire an owned request. |
| Current mutation with no displayed health | Accepts decoded non-null health and records the midpoint offset. Settles busy. |
| Mutation with equal/newer revision | Accepts even across life IDs. Recovery can create a new life. Records `serverTime - (BeganAtMs / 2 + ReceivedAtMs / 2)` and settles busy. |
| Mutation with older revision | Retains displayed health and clock, settles busy and returns `OlderRevision`. |
| Successful mutation after a subscription error | Does not independently clear that error or set ready. |
| Mutation failure with captured life still displayed | Publishes the existing fixed recovery error and settles matching busy state. |
| Mutation failure after null/different life | Suppresses the recovery error, retains other view data and settles matching busy state. |
| Unmarked owned success, malformed response or failure | Rejects before response application. Keeps the prepared request and busy state. |
| Same owner | No-op. |
| Changed owner | Clears the view and private request state, selects the exact new owner and retires the subscription generation. |
| Subscription begin/end | Retires the old generation and any owned payload/dispatch state, clears busy, and retains health, readiness, errors and clock. |
| Clear | Drops selection, view, subscription and private request state. |
| Dispose | Permanently clears state, publishes the terminal notification and clears delegates. |

State changes precede notifications. Preparation only reads identity after its notification; abandonment and completion do not write state afterward. A listener that clears, switches owner, reconnects or prepares newer work cannot have that work overwritten when the older call returns. StateJson and the full recovery decoder are unchanged.

## Verification and remaining work

Source review traced immutable capture, service bounds, owner/generation reentry, dispatch marking, unsent abandonment, unmarked and duplicate completion, lower/equal/newer and cross-life revisions, null reads, fixed errors and identity clearing. `git diff --check` passed for the source pair. All eleven assignment hashes matched their canonical Git bytes before editing. The nine read-only references/dependencies still matched afterward, including both StateJson files at their dependency commit. Source output hashes are below.

No tests, lint, typechecks, compilation, builds, generation, launch, installation, live call, remote write, deployment or configuration change ran. This native-only change requires neither renderer packaging nor a What's New update. Unrelated working-tree changes remain outside this checkpoint.

Parent-owned independent Standards and Spec reviews remain pending. The coordinator owns Windows transfer. Native permission, authenticated transport, medic/world producers, mounting, content integration, compilation, in-game inspection and release remain incomplete. Account/readiness gates and the world-control interlock were not changed; this patch does not enable them. Saved Home access, presence and displayed health cannot authorize recovery.

## SHA-256 records

Source preimages at the canonical base:

```text
2c874a564ef777217630435549a1f10d36ac7215d509e59c82b48122326d77c7  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionRecoveryState.h
e47db6550d6b173aaa8e24dda2ddb7cbb5f18893cd83375b7b2084911059562a  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionRecoveryState.cpp
```

Source outputs at `b8c7b8891953d17f56905cc914df66ebb639363c`:

```text
238201a44e0d1cb3f41025bd73f34c9be65f6d9188e1e9a3804b8344c250dca5  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionRecoveryState.h
56dccbfee2431cab7a36d60aaf1904f38b8b247845e527cf83fd1aab028048b9  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionRecoveryState.cpp
```

Unchanged reference/dependency inputs at the canonical base:

```text
bbbf79d23adc51bd8d7ef693a4ff7c8b63caf190db4affeaa631f6b7d1d2fd52  packages/api/src/companion-recovery.ts
e4ec7d77076df9713fa4177e4abc0b71968231d8f4538660665652c9a2882d79  packages/utils/src/companionRecovery.ts
79db41dc4657010ef31df042c220996db258230eb21e744b1fc7a2b405e8b971  apps/backend/convex/companionRecovery.ts
df76e2a3b8db23c5ebb3cec9f8d8d760016ced75963e9ea81f2a65452f9dc2c0  apps/backend/convex/lib/companionRecoveryModel.ts
0378f1692ebb343ff8dc734e72edd08011469ef9061c58f25c2db33a006ca94a  docs/engineering/unreal-combat-recovery-handoff-2026-10-10.md
13ceb1b13bee4b0793ddb117e03559bfb8ea77f8bdb8dc81baa1ffa8e70bb618  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.h
df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.cpp
90d230cf0233458c46324fd87d6f077c6f54bd7dd691030ba5a2a89e8607a17d  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionWorldPlayer.h
536bf39a4089acea43be36448ad3d02d3cd7f94a65015d3975f761941b802328  apps/companion-unreal/Source/BaddieCompanion/BaddieCompanion.Build.cs
```
