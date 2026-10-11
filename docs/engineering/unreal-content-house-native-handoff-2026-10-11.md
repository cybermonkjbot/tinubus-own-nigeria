# Native content-house permission handoff

October 11, 2026. Source implementation only.

## Pins and status

- Assignment `docs/engineering/unreal-content-house-native-assignment-2026-10-11.md` at `f597db16c2739709234679cb10f10e294c905d50`, SHA-256 `9d962e4315e456dfa7ccd208e21694796a49afbc12115cb37debac507f18b4b2`.
- Source/reference base `99328431b3f34a1cdf20794402f165969090f403`.
- Source commit `17113e884437636b019912f11616cd8a12867df4`. It adds only `Public/BaddieCompanionContentHouseState.h` and `Private/BaddieCompanionContentHouseState.cpp` under `apps/companion-unreal/Source/BaddieCompanion`.
- This new handoff is the third owned path and is committed separately. All three paths were absent before implementation. Prior Teams scopes remain frozen.

Implementation and source inspection are complete. Parent still owns full-range Standards and Spec reviews and acceptance. The coordinator owns any future Windows delivery. This change has not been compiled, connected to an authenticated producer, exercised in-game or released.

## Decoder and display state

`DecodeContentHousePermission` reads the existing endpoint value through unchanged StateJson and UE Json. It publishes an immutable complete permission only after every check passes and clears its output on failure. It bounds a copied input before replacing an aliased prior output.

The snapshot has exactly the required `permissionKey`, `expiresAt`, `allowed` and `participants` fields. Each participant has exactly `userId`, `name`, `isYou` and `allowed`. Missing fields, unknown fields, wrong JSON types, a non-object root and invalid numbers reject. Field checking applies to the parsed UE object; this module adds no independent raw-JSON parser.

Native structural limits are explicit:

| Value | Limit |
| --- | --- |
| Snapshot JSON | 131,072 native TCHAR units, depth 4, at most 128 structural commas/colons outside strings |
| Authorization/null response JSON | 4,096 native TCHAR units, depth 2, at most 8 structural commas/colons outside strings |
| Permission key, user ID, owner ID, instance ID | Nonempty string, at most 256 native TCHAR units |
| Participant name | String, at most 8,192 native TCHAR units; empty names remain representable |
| Roster | One through twelve participants, in original order; exactly one `isYou: true` |
| Snapshot/authorization expiry | Finite nonnegative safe integer, at most `9_007_199_254_740_991` |
| Visit revision | Nonnegative safe integer, at most `9_007_199_254_740_991` |
| Caller-supplied display time and snapshot `now` | Finite nonnegative safe integer, at most `9_000_000_000_000`, matching the snapshot endpoint |

These bounds and integral expiry rejection are deliberate native structural checks over the shared typed response. Strings remain exactly decoded, including case and surrounding whitespace. No trimming, case conversion, key hashing, display-name fallback or roster sorting occurs. Permission keys remain opaque strings rather than client-recomputed roster hashes.

Duplicate user IDs reject using case-sensitive equality. An empty roster, missing own participant or multiple own participants rejects. A snapshot with aggregate `allowed: true` rejects if any participant has `allowed: false`. An aggregate false snapshot with all individual values true stays false. After decoding, installation separately requires the sole own participant's user ID to match the current account owner.

`FContentHouseView::Permission` retains all server fields and individual consent values. `bEffectiveAllowed` is the separate display result. Expiry at or before the supplied time sets it false and latches expiry for that observed value. A later backward clock cannot revive that value. Only a fresh accepted snapshot replaces the latch. Invalid display time clears permission and sets a fixed error. None of these display checks proves server permission or enables capture.

## Lifetimes, reads and cancellation

`BeginOwner` creates a new local account/session lifetime on every invocation, including the same user ID. It clears all old owner, visit, subscription, observation, read and command state. The future account adapter must call it for session/device-owner replacement even if the user ID is unchanged. Credentials are never passed to this module.

`BeginVisit` retires the prior visit before installing the new instance ID and revision, including replacement with the same values. Invalid next scope leaves the old visit retired. `EndVisit` clears visit-bound state without ending the account lifetime. Subscription replacement creates a separate generation. The future observer owner calls `StopSubscription` before unsubscribing, on last-observer removal or feature disablement. It clears permission, read receipts, visit-bound commands and their errors. The retained account and optional visit scope permit later subscription setup; they do not preserve permission.

Each snapshot intent captures account, visit and subscription generations plus a unique local request identity. Its immutable JSON captures instance ID, visit revision and display time. There is only one current read receipt. A newer `Snapshot` intent supersedes the previous receipt, so an older result or error cannot overwrite newer state. The future adapter captures the intent before starting a read, or at ordered subscription ingress before asynchronous processing. This module does not discover transport ordering or create subscriptions.

An active read failure clears the current permission, matching the Tools consumer. It also invalidates any authorization bound to that observation. A stale failure does nothing. `Cancel` only accepts an exact pending receipt; cancelling a read also clears permission and its bound authorization. Stopping/replacing a subscription invalidates late results even if instance and revision are unchanged. Disposal is terminal and releases all private state.

Cancellation is local receipt invalidation, not proof that a sent backend operation was undone. The future adapter owns actual cancellation and unsubscribe. No callback, delegate, timer, abort controller or transport is created here.

## Intent and completion contracts

All operation strings use the existing `companionContentHouse:` prefix. UE Json constructs only these non-auth fields:

| Operation | Non-auth arguments | Completion |
| --- | --- | --- |
| `snapshot` | `instanceId`, `visitRevision`, `now` | Exact permission snapshot |
| `setPermission` | `instanceId`, `visitRevision`, `allowCaptureAndShare` | JSON `null` |
| `revoke` | Empty object | JSON `null` |
| `authorizePhoto` | `instanceId`, `visitRevision`, `permissionKey`, `purpose` | Exactly `permissionKey`, `expiresAt` |

Purposes are exactly `capture`, `save_draft` and `publish`. Intents expose read-only operation and JSON getters; their account, visit, subscription, observation, purpose and request identities are private. `IsValid` means only that an intent was captured. It is not currentness, dispatch permission or an authenticated receipt.

The state retains at most one mutation intent alongside the latest read receipt. There is no pending queue or retry loop. Set-permission and authorization require an active visit subscription. Revocation only requires the original account lifetime and an empty mutation slot, so it remains representable after travel, subscription loss or feature disablement. An already captured revoke survives those transitions. Account/session replacement or disposal invalidates it. To replace pending work within a visit, the caller explicitly cancels its exact receipt; no cancellation sends another operation.

Pending mutations never alter aggregate or individual consent. A successful `setPermission` or `revoke` null result releases only its own busy slot, discards cached permission and outstanding reads, and requires a subsequent authoritative snapshot. This is a conservative native behavior; the current UI ordinarily waits for subscription updates without clearing its cached view on mutation success. Malformed null results and active failures release their own slot with fixed errors, never manufacture permission, and do not store raw error text.

Authorization captures the current permission key and observation identity. Its completion must have the exact pending request and account/visit/subscription lifetime, the caller's expected purpose, a matching response key, an unexpired response and a still-current, effectively allowed observation. It samples display time again before acceptance. Every accepted snapshot replaces the observation and invalidates a pending authorization, even if its fields are unchanged. This deliberate restriction also covers a changed roster with an unchanged key.

`AcceptAuthorization` consumes the pending request and stores no authorization result or reusable grant. Wrong-purpose and wrong-completion-method calls return stale without clearing the pending operation. Old, duplicate and cancelled receipts cannot clear a newer busy/error state. Malformed data and changed/expired permission never return accepted. Failures use only fixed enum-backed messages.

## Correspondence to the pinned consumers

| Consumer | Preserved requirement |
| --- | --- |
| `CompanionContentHousePermission.tsx` | Explicit allow and revoke remain separate intents. Busy work does not change the roster's consent or close any native UI. Confirmation UI remains outside this module. |
| `CompanionContentHouseTools.tsx` | Subscription errors clear permission. Display expiry and current key gate descriptive state. New observations invalidate prior checks; no creation capability or commit-check readiness input is added. |
| `CompanionContentHouseComposer.tsx` | Save-draft and publish are separate purposes. Cancellation invalidates pending local checks. Composer, upload and commit ownership stay with the account host. |
| `companionContentHouseCapture.ts` | A future capture consumer needs separate checks before pixels and after pixels. Cancellation/changed permission must prevent the result from progressing. This module creates neither pixels nor media ownership. |
| `native-scene/contentHouseCapture.ts` | The current receipt remains `source: "scene-canvas"`. Nothing here labels Unreal output as canvas output or changes the renderer contract. |

## Source-inspected cases

These are source-review conclusions, not executed tests, fixtures or runtime acceptance.

| Case | Source result |
| --- | --- |
| Missing/malformed snapshot, wrong primitive or unknown fields | Bounded parse and required typed fields reject atomically; an active result clears cached permission and reports a fixed invalid-response error. |
| Empty, twelve-person and thirteen-person roster | Empty fails the own-participant count. Twelve can pass with unique IDs and one matching owner. Thirteen fails the bounded-array reader. |
| Duplicate IDs, missing or ambiguous own participant | Reject before publication; an otherwise valid sole own participant belonging to another account fails installation. |
| Aggregate allow with a denied participant | Rejects rather than constructing an authorized subset. Roster order and every participant remain intact on success. |
| Negative, fractional, nonfinite or oversized time/revision | Rejects through display-time, integer or scope bounds; no numeric rounding or clock-derived authority is added. |
| Expiry then backward time | The same observation stays effectively denied; individual consent and server fields remain unchanged. A fresh accepted observation may replace it. |
| Same-ID owner replacement | A new owner generation retires all prior receipts, including revoke. Old results/errors cannot reach the replacement owner's view or busy slot. |
| Visit/subscription replacement and late reads | New local generations retire earlier reads, even when textual scope values match. |
| Out-of-order read completion | Only the latest captured read identity can publish. Old success, failure or cancel cannot clear a newer read's result. |
| Stop, active read error, read cancellation and disposal | Clear permission and invalidate its authorization. Of pending commands, stop retains only original-account revoke; disposal retains nothing and cannot restart. |
| Consent pending, success and failure | Pending leaves consent descriptive. Null success clears local permission and awaits a new snapshot. Failure releases only its current slot with a fixed error. |
| Explicit revoke after travel or feature disablement | Can be captured without a visit/subscription and emits `{}`. A pending revoke can complete after travel on the same account lifetime. Its success invalidates that account's current cached permission. |
| Wrong key, purpose, operation or receipt | Wrong response key fails authorization. Wrong purpose/method and old receipts are stale without disturbing newer work. The response has no wire purpose/request ID; the opaque original intent provides their local binding. |
| Read or response expiry during an in-flight authorization | Completion samples time, enforces the sticky read expiry and response deadline, consumes the current check and fails without granting permission. An earlier clock observation can already have invalidated it. |
| Changed roster/key or a fresh identical observation during authorization | Replacing the observation retires the check. Its later completion cannot clear another operation's busy/error state. |
| Repeated accepted completion | The original request is no longer active, so replay returns stale. No authorization is stored for reuse at another stage. |

## Reference and output hashes

Parent pinned the committed inputs at the base above. All sixteen working reference hashes and the assignment hash matched before implementation. Source and handoff paths were new. The reference files, Build.cs, StateJson, historical scopes, renderer inputs and concurrent owners' work were not edited by this assignment.

```text
bba0c912518dbb8c579145ede3a0ef95320ed918422e6f717d2ca96016615e1a  AGENTS.md
4d8455c0c76e08d7243670e6c97f277e083dd13daf8c9ac59b30d55d1250912d  CONTEXT.md
2661504780df0842860c656400bbcf86972e21d633c43dd6af4ccaa74d0abd93  packages/api/src/companion-content-house.ts
4fa8952f46767242aea6a82627b491369940605e55b3b3bf511cc534d7292cb9  packages/utils/src/companionContentHouse.ts
3b90d88ac080a2d2050b6e5ccef78c406deaa7a08bfd9b974a3c39141d8f4b8e  apps/backend/convex/companionContentHouse.ts
71fd91f0b8ff5013f1ad271b89e3e1d4bf61a9529ed1b8c5b35199d1f2995786  apps/backend/convex/lib/companionContentHouseModel.ts
d7d24eca012e0a0bcbf87d0c98c45584fd006e480315d32b7c49617d4983787e  apps/mobile/src/features/companion/CompanionContentHousePermission.tsx
78fea19ffb28fe5d4b5e8c513103bf92aecb1ba177130f8417823024f93e445b  apps/mobile/src/features/companion/CompanionContentHouseTools.tsx
510445b073705e191950c324c44d483f21b16489536f63b15a7115be6bc64231  apps/mobile/src/features/companion/CompanionContentHouseComposer.tsx
cb4febe83b4f7136b48a759d9e21f144b0ab272cb76ce8298bfca9adee2ae5be  apps/mobile/src/features/companion/companionContentHouseCapture.ts
ee48f5966e0471f4899b540522f800025c45475b0f685db1e0c75fd0745bb363  packages/companion-scene/src/native-scene/contentHouseCapture.ts
13ceb1b13bee4b0793ddb117e03559bfb8ea77f8bdb8dc81baa1ffa8e70bb618  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.h
df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.cpp
b03f1f8a26d5d12d3ac707596d2ce057ce704bad9149ff4903de46fa9634694e  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionNpcDialogueState.h
ba3bf7961ed8312855baf8b64b7d348e4168d7e24ee79edb792b35e25059c0bd  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionNpcDialogueState.cpp
753a82dfa30a4faad358cb02e92acfe70251e4505cba1402c8e98fb3706e3ecb  apps/companion-unreal/Source/BaddieCompanion/BaddieCompanion.Build.cs
```

Source output SHA-256:

```text
785554493a6558828db827c8e338b3db281e66514f53908f3815402274fa533c  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionContentHouseState.h
b52330c17310d442a7e0eaa0f9a8a032fd290b47a1c8d1d474680ba4312304a7  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionContentHouseState.cpp
```

## Remaining integrations and verification limits

The existing backend requires account-session and device binding. Version-one native gameplay grants do not authorize these endpoints. No native permission transport or authenticated observation producer exists in this patch. A future authorized integration must bind every description and completion to the original account/session/device resources, deliver ordered read receipts, supply fresh display time, invalidate local lifetimes before owner/travel/subscription transitions and handle actual cancellation. It must never attach replacement-account credentials to an old description.

No native capture consumer, screenshot/render-target readback, media file, upload, composer UI or publication path is implemented. Future pixel capture must inspect and reuse installed UE facilities under its own scope and resolve the shared receipt-source contract. It must check permission separately before pixels, after pixels, for each save-draft/publish request and at every existing remote commit stage. An accepted check is only the result of that one request and purpose, not a reusable grant. Social publication remains in Baddie's account-owned composer.

Gates and world-control interlocks remain disabled. No readiness bypass, gameplay authority or grant expansion was added. Standalone source does not trigger production renderer packaging or What's New changes. Content availability, compilation, in-game verification and release remain separate from source acceptance and transfer.

Verification consisted of complete pinned-reference reads, reference/output SHA-256 inspection, complete new-file source diff inspection and the branch traces above. Both checkpoints use explicit owned paths and disabled hooks after checking that the shared index is clear. The writing-for-agents skill shaped the contract, lifecycle and remaining-integration sections; unslop guided the prose edit.

No tests, fixtures, executable validation, typechecks, lint, builds, compilation, codegen, live/business-code execution, remote writes, engine/server launches, asset operations, installs, deployment or configuration changes were performed. No other chat was messaged. Concurrent world-entry, backend-retirement and unrelated working changes were preserved.
