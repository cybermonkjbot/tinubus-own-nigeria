# Native content-house permission state

October 11, 2026. Source implementation only.

## Ownership and purpose

Existing owner `01a126d5-064b-7703-a27e-31991c45a302` completed its Teams reservation turn `01a1286f-6b68-7ff3-823e-2132e826cd4c`. That work passed both reviews and reached Windows. All prior scopes remain frozen. This assignment adds the local permission state required before native content-house capture can be connected.

The source/reference base is `99328431b3f34a1cdf20794402f165969090f403`. Parent matched all sixteen references below and confirmed these three new paths absent. Only these paths are writable:

- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionContentHouseState.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionContentHouseState.cpp`
- `docs/engineering/unreal-content-house-native-handoff-2026-10-11.md`

Read AGENTS.md, CONTEXT.md and the complete pinned content-house backend, API, shared contract and five consumers. Reuse the existing private StateJson helper and UE Json serializer. The NPC dialogue pair is a reference for opaque local receipts, not a dependency to add or a domain to edit. Keep the public interface small and hide parsing and receipt bookkeeping in this module.

Parent owns tracker, acceptance and future integration. The coordinator alone owns Windows writes. Backend/mobile/shared source, StateJson, Build.cs, prior handoffs, all world/session/controller/presentation files and renderer inputs stay read-only. Native entry/static and backend session-retirement implementations run concurrently in their separate assignments.

## Required implementation

### Permission reads and lifetime

Implement an atomic, bounded decoder for the existing permission snapshot. Retain permissionKey, expiresAt, allowed and the full ordered participant list with exact userId, name, isYou and allowed values. Preserve strings without trimming or case conversion. The canonical roster is at most twelve humans and includes the requesting user. Reject malformed types, invalid finite/time values, duplicate identities, an absent or ambiguous own participant, and an allowed snapshot whose roster does not all allow capture. State the native structural rejection rules and parse limits in the handoff. Never turn malformed or missing data into an empty authorized roster.

Keep account lifetime separate from visit/subscription lifetime. Replacement of the same account identifier still retires old local receipts. Each read result belongs to the original account lifetime, instanceId, visitRevision and subscription generation. Before installing a view, match its sole isYou participant to the current owner userId. A stopped/replaced subscription cannot publish late values or errors into another visit. An active query failure clears current permission, matching the existing Tools consumer. Cancellation, last-observer removal and disposal release private state and invalidate old results.

Use explicit caller-supplied display time as the existing account adapter does. Expiry forces effective allowed=false without changing the roster's individual consent values. After a particular observed value expires, a backward clock sample must not revive it. A fresh accepted observation may replace it. Do not claim a display clock proves server permission. Local read state remains descriptive and never establishes capture, publication or gameplay authority.

### Explicit intents and completions

Implement local intents and opaque completion receipts for the existing operations. Encode only their exact non-auth arguments through UE Json:

| Operation | Arguments owned here | Return value |
| --- | --- | --- |
| snapshot | instanceId, visitRevision, now | Permission snapshot |
| setPermission | instanceId, visitRevision, allowCaptureAndShare | null |
| revoke | Empty object | null |
| authorizePhoto | instanceId, visitRevision, permissionKey, purpose | permissionKey, expiresAt |

The snapshot display time follows the backend's nonnegative safe-integer limit of 9,000,000,000,000. The visit revision is a nonnegative safe integer. Purposes are exactly capture, save_draft and publish. No command implies a retry, query-cache invalidation, mutation dispatch or new endpoint. Expose operation names/arguments as local request descriptions only. Account credentials and device credentials never enter this module or its JSON.

setPermission is an explicit consent-change intent. revoke belongs to the original granting account/session lifetime and must remain representable after travel, loss of the visit subscription or feature disablement. It must not revoke a replacement account/session lifetime. Neither null success nor a pending mutation sets allowed=true. Preserve the distinction between changing consent and receiving a subsequent authoritative snapshot. Repeated, stale, mismatched and cancelled completions cannot clear a newer busy/error state or install a result for another operation. Keep finite in-flight bookkeeping and describe any deliberate native concurrency restriction.

An authorizePhoto completion must match its captured key, purpose, account/visit lifetime and request identity. Require an unexpired receipt and a still-current permission observation. A changed roster/key, expired read, stopped subscription, travel, cancellation or owner replacement invalidates it. Raw response errors are not retained; use fixed sanitized errors. Missing/invalid completion data never counts as permission. Keep local receipt acceptance separate from any actual native dispatch or capture capability.

The future capture consumer must perform separate checks before pixels and after pixels, then again for save_draft or publish and at each existing remote commit stage. A successful check for one request or purpose is not a reusable grant for another stage. Preserve this requirement in the interface comments and handoff. This assignment implements neither a capture pipeline nor a publication path.

## Authority and Unreal reuse

The current backend endpoints require the account session and device binding. Version-one native gameplay grants do not authorize them. There is no native permission transport, authenticated producer or capture consumer in this assignment. Do not create a permissive readiness input, bypass the account owner, widen a grant, or treat loaded content, call presence, a photo button or a local path as permission.

The current web receipt source is scene-canvas. Do not label Unreal pixels as canvas output or change that shared contract. Native screenshots/render-target readback should reuse UE facilities in a later scoped capture implementation after inspecting the installed implementation. This assignment owns no pixels, media files, audio, screenshot APIs, clips, uploads, composer UI or assets. Keep all gates false. Existing social publication remains in Baddie's account-owned composer.

## Source acceptance and handoff

Inspect every branch for malformed snapshot/receipt, empty and twelve/thirteen-person roster, duplicate/missing-own identities, expiry/backward time, same-ID account replacement, visit replacement, out-of-order read completion, stop/error/dispose, consent pending/success/failure, explicit revoke after travel, mismatched authorization key/purpose/receipt and expiry during an in-flight check. Inspect the complete two-file source diff and its correspondence to all pinned consumers. This is source review, not executable validation.

Use apply_patch. Commit only the two owned source files with hooks disabled after checking the shared index is empty. Then record source commit, exact input/output hashes, implementation decisions, source-inspected cases and remaining integrations in the separate owned handoff commit. Preserve concurrent work and never stage unrelated changes. Parent will reconcile all three paths, then obtain independent full-range Standards and Spec reviews before any Windows handoff.

No tests or fixtures, typechecks, lint, builds, compilation, codegen, business-code execution, live endpoint calls, engine/server launches, installs, asset operations, remote writes, deployment or configuration. Do not message other chats. Standalone-only source does not trigger production renderer packaging or What's New changes. Source implementation, independent acceptance, transfer, content, compilation, in-game verification and release remain separate.

## Pinned references

SHA-256 of the committed and matching working bytes at the source base:

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
