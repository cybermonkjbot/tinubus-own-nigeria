# Policing command packets and dispatch ownership

October 11, 2026. Source implementation and source review only.

## Checkpoint and scope

Source commit: `df9511ded09e51f2fe35a433c3b1d2716ac42e9d`.

Assignment: `docs/engineering/unreal-policing-command-assignment-2026-10-11.md` at `2a2110c55e1071d9d4a48fe136685ea51bca23f9`. Canonical references, dependencies and preimages use `7900099f44c8e08fbe447fe750cc74a171867f90`. All fourteen assigned SHA-256 values matched before editing, and this handoff did not exist. The twelve read-only references/dependencies still matched after source implementation.

Only these paths changed:

- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionPolicingState.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionPolicingState.cpp`
- `docs/engineering/unreal-policing-command-handoff-2026-10-11.md`

The existing read/consent/state owner now serializes all four action packets and tracks explicit once-only dispatch ownership. No second store, request queue or network client was added. The original policing handoff, StateJson, CombatState, WorldPlayer, backend, mobile, Config and every other worker's scope were left unchanged. The source commit contains only the state pair; this handoff is a separate exact-path checkpoint.

Source and diff inspection are complete. Independent Standards/Spec review, Windows delivery, compilation, native integration and in-game acceptance remain pending. No tests, lint, typechecks, compilation, builds, code generation, engine/server/Blender launches, installation, conversion, remote actions, live requests, deployment or configuration changes ran. Account/readiness gates and the world-control interlock remain disabled. This does not establish a playable policing feature.

## Packets

`PrepareSerializedCommand` creates an `FPolicingSerializedCommand` with a captured endpoint string, UE-serialized argument string and the existing prepared action/receipt. All getters are read-only. No mutable JSON object escapes. Packet copies retain the same request identity, so copying never permits a second mark or completion.

The argument examples below use `encounter-1` as a placeholder and omit insignificant serializer whitespace. Object member order is not a protocol requirement.

| Action | Endpoint | JSON arguments |
| --- | --- | --- |
| Cooperate | `companionPolicing:cooperate` | `{"encounterId":"encounter-1"}` |
| RequestRelease | `companionPolicing:requestRelease` | `{"encounterId":"encounter-1"}` |
| PayExtortion | `companionPolicing:respondExtortion` | `{"encounterId":"encounter-1","response":"pay"}` |
| RefuseExtortion | `companionPolicing:respondExtortion` | `{"encounterId":"encounter-1","response":"refuse"}` |

Both packet policies are fixed false: `ShouldRetry()` and `ShouldInvalidateQueryCache()`. They represent the canonical account store's adapter policy. They do not implement transport behavior or authorize a native endpoint.

The existing prepared command supplies the endpoint mapping, response distinction and exact encounter ID. Preparation copies the strings before returning or invoking any external notification. `FJsonObject::SetStringField` and `FJsonSerializer` own escaping of quotes, backslashes and control characters. No manual JSON interpolation, trimming, case folding, identifier creation or new eligibility predicate is added. Empty identifiers accepted by the existing public read contract remain representable.

Only encounterId and the applicable pay/refuse response enter the argument object. Confirmation amount, outing, entry slices, revision, price, device/account credentials and request metadata stay out of the packet. Private GUIDs remain local bookkeeping and are not serialized.

The read bound remains 262,144 characters, depth 8 and 2,048 structural separators. The new serialized argument ceiling is `6 * 262144 + 128`, or 1,572,992 characters. It allows up to six characters of JSON escaping per source character plus the fixed object fields without narrowing the existing decoded-ID range. Serialization is naturally bounded by the immutable encounter decoded under the existing read ceiling; the output ceiling is checked before publication. This is a native structural bound, not a server eligibility or payment rule. Serialization failure publishes no packet, releases the captured busy owner and sets the existing fixed action-error fallback.

## Shared preparation and legacy compatibility

Both preparation APIs call one `PrepareCommandInternal`. Its canonical early order remains:

1. Disposed, no subscribers or busy calls return silently.
2. Unsupported action values retain the existing silent refusal.
3. Pay checks readiness and the complete seven-field confirmation before the ordinary ready/encounter return.
4. All actions require a ready encounter. Non-pay actions gain no phase, deadline, balance or demand-status predicate.
5. Capture generation and encounter ID, allocate the command GUID, set busy and clear error.

The fixed pay errors are unchanged. Unready pay gives `Wait for the current encounter to load.` Missing or changed consent while ready gives `This demand changed. Cancel and review the current encounter.` The existing capture/match functions are unchanged, including safe-integer checks, the 1-through-100 amount range and exact comparison of encounterId, instanceId, revision, resolvesAt, amount, unitKey and outingId.

Legacy `PrepareCommand`, `IsCurrentCommand`, `Accept` and `AcceptFailure` remain compatible for legacy prepared commands and subscription receipts. Legacy commands need no dispatch mark. The serialized path marks its private receipt with `bRequiresDispatchMark`; the state separately records that ownership mode. Legacy completion APIs reject those receipts both before and after dispatch, preventing receipt-only completion from bypassing the serialized path's mark rule. A legacy request also cannot be passed as a serialized packet.

Only one request of either mode owns busy. Completion or abandonment retires its GUID and clears both ownership flags. A stale legacy completion cannot clear a newer serialized request, and the reverse is also rejected. Subscription receipts retain their original generation semantics independently of either command mode.

## Dispatch, abandonment and completion

`MarkCommandDispatched` requires a structurally valid serialized packet, its private mutation kind and owned-mode flag, the active command GUID and the current subscription generation. It rejects defaults, foreign owners, retired requests and duplicate marks without changing state. It sets one local dispatch flag and sends nothing.

After the caller publishes the busy notification, selection/access replacement, last-observer removal, reconnect or disposal prevents the old packet from being marked. A same-generation subscription update does not cancel or retarget the packet, even when the current encounter changes ID, becomes null, revises the demand or becomes unready after a subscription failure. This matches the canonical dispatch recheck of generation/disposal rather than refreshed encounter identity or consent.

`AbandonUnsentCommand` succeeds only for the current unmarked serialized request. It releases that request's busy ownership without changing encounter, readiness or error. It does not simulate a response or an action. After marking, abandonment is rejected; the adapter must settle any actual or synchronous dispatch failure through the marked failure API. Repeated abandonment cannot affect later work.

`CompleteSerializedCommand` and `FailSerializedCommand` require the active marked serialized request. Default, foreign, unmarked, retired and duplicate completions leave state alone. A successfully decoded subscription response is the wrong completion kind and returns `WrongResponseKind` without releasing busy. It must not be treated as the action's reply.

Actual action success requires a valid mutation-kind decoded response whose root is JSON null. `DecodePolicingResponse` now delegates its existing raw-null branch to unchanged `StateJsonPrivate::ParseNull`, which runs UE syntax validation. JSON strings, objects, arrays, multiple roots and malformed input cannot act as a null acknowledgement. Normal JSON whitespace is accepted. The previous trim-only shortcut no longer establishes validity by itself. Valid legacy null/encounter payloads, budgets, output clearing and field policies remain unchanged.

A malformed action response produces `InvalidResponse` and settles the marked request through the existing failure path. A valid wrong-kind response remains pending for its correct callback. The shared completion implementation retains canonical behavior:

- Null acknowledgement never changes encounter or readiness and never clears an intervening read error.
- A changed or null current encounter returns `StaleEncounter` on acknowledgement but still releases the original busy owner within the same generation.
- Failure publishes an error only when the current encounter ID exactly matches the captured ID. Revision, phase and instance do not add new error guards.
- A same-ID return after intervening encounter changes remains eligible for the canonical ID-only error comparison.
- Generation loss/disposal prevents completion from reviving data or clearing newer work.

Failure messages follow the existing display-safe input contract and 1,024-character ceiling. Missing or overlong messages use the existing fixed fallback. No transport body, credential or log data is retained.

## Notification contract and reentry

Notifications remain external and synchronous. State methods do not call delegates, dispatch requests, schedule timers or retry anything. Keep each prepared packet in a request-local value through notification and asynchronous completion; do not overwrite it by reusing its output slot for a different preparation.

| Result | State and required caller action |
| --- | --- |
| Preparation `Prepared` | Busy is true and error is clear. Publish the normal busy notification, then mark that exact packet. |
| Preparation `Ignored` | State is unchanged and output is invalid. No notification or dispatch. |
| Preparation `DemandRejected` | Fixed consent/readiness error is set; busy is unchanged. Publish the normal error notification, including when the text is unchanged. |
| Preparation `SerializationFailed` | Output is invalid, busy is released and the fixed action error is set. Publish the error notification; do not dispatch. |
| Mark true | Send immediately through a separately admitted adapter, without yielding or notifying between mark and send. No notification for the mark itself. |
| Mark false | Do not send. Do not run a broad cleanup that could clear a newer request. |
| Abandon true | Busy is released without a response. Publish the normal notification after return. |
| Abandon false | No state change or notification. |
| Completion `Accepted`, `StaleEncounter`, `InvalidResponse` | The owned request has settled and busy is released. Publish the normal completion notification after return. |
| Completion `StaleReceipt`, `WrongResponseKind` | No state change, release or notification. |
| Failure true | The marked request has settled; any applicable same-encounter error is set. Publish the normal completion notification after return. |
| Failure false | No state change or notification. |

Preparation notification may synchronously abandon the request or replace the selected owner/subscription, then prepare another request. The outer caller's mark checks the original GUID/generation and fails without touching the newer request. A recursive preparation while busy simply returns `Ignored`. If notification fails before marking and the request is still current and unsent, explicitly abandon that packet rather than fabricating success.

The caller must not yield or notify between successful mark and actual send. If the admitted send fails synchronously, call `FailSerializedCommand` for that marked packet. After a completion or abandonment notification starts, the state owner has already retired the old request. No deferred old-request cleanup follows that notification, so a reentrant new request keeps its own busy state. Preserve that ordering in the future adapter.

## Source-review findings

The complete owned source diff was inspected against `7900099f44c8e08fbe447fe750cc74a171867f90`. Source reasoning covered the following cases; no runtime or test execution is implied.

| Review case | Source result |
| --- | --- |
| Four actions and escaped strings | Exactly the canonical endpoint/argument shapes; UE owns string escaping. Non-pay responses are omitted rather than null. |
| Pay before readiness/encounter return | Shared preparation preserves the fixed errors and silent busy/disposed/unsubscribed behavior. |
| Preparation notification reentry | Captured GUID and generation prevent old marking, abandonment or completion from touching replacements. |
| Same/different/null encounter during notification | Packet keeps its captured ID; marking has no refreshed encounter/consent/readiness gate. |
| Root-null acknowledgements | Shared UE null parser validates syntax; mutation decoder never accepts an encounter object as success. |
| Unsent abandonment | Clears only the active unmarked request; no error or encounter mutation. |
| Marked duplicate/copy | Shared request identity admits one mark and one completion across packet copies. |
| Wrong-kind/default/foreign/old generation | Invalid ownership is rejected before response effects; valid subscription completion is separately rejected. |
| Intervening subscription failure | Success preserves readiness/error; action failure retains the canonical current-ID guard. |
| Legacy/new overlap | Shared busy owner and private mode flag prevent legacy APIs from settling serialized requests. |
| Clearing/disposal | All command modes and dispatch flags retire with the generation; dispose is terminal. |

The source review tightened the original trim-only null check with the existing shared null parser. No remaining source defect was identified in this review. Independent review remains required and is owned by the parent. The packet ceiling and notification contract are explicit native adapter constraints; they are not backend permissions.

## Hashes and immutable references

Preimage and reference hashes at `7900099f44c8e08fbe447fe750cc74a171867f90`:

```text
1383bd9678e5014495413b6a281381bbde91daa8afea60f1613d2a71a6141942  packages/api/src/companion-policing.ts
b1f6e06d26feb906cf5c5e1d5591311d868929f512a70e4bf4ed3c56781d3719  packages/utils/src/companionPolicing.ts
2e729e6abeba8e28c7b7d328f985451162d8ba036174da86636adac0c9f02ab4  packages/utils/src/companionHaul.ts
a3f2f01f9d50da5e7fbf271c2b94bee45104c89620684b497ac2f7ccdaf92325  packages/utils/src/companionCombatEquipment.ts
4a7d9c284ba8d1acc4e75b88634fbfc62f973504bdc0f7782edad7614001f59b  apps/backend/convex/companionPolicing.ts
1bf485bea06bd3ebcb49f67a93e28dc5d3f6ad433711196b01996d37e9a06863  apps/backend/convex/lib/companionPolicingModel.ts
b992b581d0fa60852f0819bfab2071376be4d99af88b65fea203c51311f0f7bb  docs/engineering/unreal-policing-state-handoff-2026-10-10.md
51134f54334a05335374576dbf9b04f6a467a128a6aa1166e177eb309ad09748  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionPolicingState.h
fc740172318d0b42766804e55956a2d2371088ef341d03e52385ec4123922a80  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionPolicingState.cpp
bbdebfa73a9d2e00579031711a640435f4d9b88dde4d81d2cee0fb3b60b54e4c  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionCombatState.h
90d230cf0233458c46324fd87d6f077c6f54bd7dd691030ba5a2a89e8607a17d  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionWorldPlayer.h
13ceb1b13bee4b0793ddb117e03559bfb8ea77f8bdb8dc81baa1ffa8e70bb618  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.h
df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.cpp
536bf39a4089acea43be36448ad3d02d3cd7f94a65015d3975f761941b802328  apps/companion-unreal/Source/BaddieCompanion/BaddieCompanion.Build.cs
```

Output source hashes at `df9511ded09e51f2fe35a433c3b1d2716ac42e9d`:

```text
3cd99b30b5bc67f22833ec69c1ccc9661a1813a8159e56353faca63b91888977  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionPolicingState.h
a7012f98216ddf05a4a10db1e610320c874ac013de34f8bafd9832388fdc85f6  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionPolicingState.cpp
```

Complete unchanged StateJson from `b4bc1b7d7dbc9552ad1451062a404fc872a13224` is required, including `ParseNull`; do not deliver this source against an older helper without it. The existing CombatState and WorldPlayer headers remain read-only type dependencies, not new command owners. Source and handoff commits must be reviewed as one three-path range before delivery.

## Remaining integration

Native transport/consent, subscription and notification producers remain absent. Any future adapter must enforce independent admission, preserve the packet's false retry/cache policies and follow the synchronous mark/send boundary. No account credential may be supplied to Unreal through these packets. The caller must clear on access loss even if the displayed account ID remains the same.

Backend enforcement, detention release and credit settlement are unchanged. No local countdown expiry, offence finding, payment, debt, balance change, journey resume or simulated success was added. Home reads, entry/presence, decoded encounters and demand confirmations grant no policing, movement, payment, combat or editing permission.

The parent collects this checkpoint and owns full source acceptance. The coordinator alone owns any later Windows transfer. No chat was messaged and no remote handoff was attempted. Compilation, gameplay integration, in-game inspection and release remain separate, unverified stages.
