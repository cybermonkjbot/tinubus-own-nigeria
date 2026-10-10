# Unreal social command assignment

October 10, 2026. Local request/result ownership only.

## Owner and pins

Existing owner `01a12728-2e34-7ea3-9615-22f614dd0d9b`, host local, owns only SocialState.h/.cpp under `apps/companion-unreal/Source/BaddieCompanion/Public` and `Private`, plus new `docs/engineering/unreal-social-command-handoff-2026-10-10.md`. TeamsState and the prior combined handoff are read-only. Parent owns StateJson, assignment, tracker and acceptance. Coordinator alone owns Windows transfer.

Canonical references and owned preimages are pinned at `149ec7f1929da0f9e80d58dc3c2cce8c9eeb6a6a`. Parent matched all ten working hashes below and the new handoff was absent. Recheck before edits. Reuse complete unchanged StateJson from `b4bc1b7d7dbc9552ad1451062a404fc872a13224`.

## Request and result contracts

Read complete `packages/api/src/companion-game-social.ts`, the shared social contracts, backend offer/respond/transferCredits definitions and the social sections of the original handoff. Keep the existing FSocialState and every accepted offers/player/reputation/context decoder and lifecycle. Add the missing three command contracts using UE JSON, not a new networking or state framework.

| Endpoint | Non-credential arguments | Response |
| --- | --- | --- |
| companionGameSocial:offer | userId, callSessionId, roomRevision | encounterId, exact offered/qualified/declined status, required nullable threadId |
| companionGameSocial:respond | encounterId, explicit accept boolean, preserving false | Same encounter response |
| companionGameSocial:transferCredits | recipientUserId, credits, optional contextPlace and note | success boolean, credits, kobo, nairaLabel, recipientHandle, recipientName, threadId, messageId |

Capture exact strings and optional omission versus supplied empty text. Do not inject null for omitted options or trim/normalize their content. Preserve actual numeric response values and false. Validate input scalar structure against the backend contract and record explicit native limits. Room revision is a safe integer, credits a positive safe integer. Local IDs are opaque strings, not client proofs of server record identity. Keep the shared-reader bounds and atomic failed-output clearing; any new response budget is explicit.

The caller receives the complete typed endpoint result or a fixed failure category. The command result is not an offers snapshot, messaging acceptance, permission, credit ledger or proof of gameplay admission. No result may directly update offers, player access, reputation, wallet totals, team state, unread counts or private Inbox content. Do not calculate money labels, kobo, balances, rewards or recipient metadata locally.

## Invocation and completion ownership

The canonical methods have no global busy latch and no requirement for selected offers, active offers listeners or a displayed player. Preserve independent concurrent requests. A command for call A must not become a command for selected call B. An account/session identity is required locally, but is not authority; the future admitted adapter still owns credentials and exact endpoint permission.

Use immutable request packets and independently owned, one-shot receipts. Preparation, one-time dispatch, unsent abandonment and terminal completion must be explicit. Clear output before refusal. Reject default/foreign/copied duplicate/unmarked-owned completions. Recheck ownership at dispatch; adapters send immediately after marking without yielding or notifying. Unsent abandonment has no ambiguous-write meaning. Sent commands cannot be abandoned as unsent, retried automatically or turned into new calls.

Preserve the canonical distinction between view lifetime and an already-sent Promise. Selecting/reloading/stopping offers or player watches does not cancel or retarget a sent social command. Dispose rejects new calls, but the canonical already-started Promise can still return its result to its original caller. Keep any accepted completion bound to that original account/session and request. It must never repopulate a disposed owner or publish into a replacement identity. Document how the original caller retains only the bounded receipt needed to settle, and how terminal completion/discard releases it. Do not require the current offers selection or epoch to accept an independently captured result.

Keep command errors separate from offers/player errors. The existing social methods propagate failures and do not set the displayed offers error, busy or ready flags. This owner has no delegates; retain caller-driven notifications. Use fixed local diagnostics without storing raw server messages, tokens or payloads. Do not hide an invalid response behind a success-shaped default.

Both offer/respond use retry:false and invalidateQueryCache:false. transferCredits uses retry:false and invalidateQueryCache:true. Record these exact future adapter obligations per packet. The current source slice executes neither setting. There is no canonical idempotency key or retry slot for these methods; do not invent one. If native pending storage needs a cap, make refusal explicit and document the bound without evicting existing requests.

## Completion and exclusions

Implement with apply_patch, inspect source/diff and checkpoint exact owned paths with hooks disabled. Write the new handoff with immutable source commit, canonical/preimage/output hashes, complete endpoint/response matrix, disposal and return-only result ownership, concurrent-request behavior, adapter sequence and remaining work. Review all three commands, absent/empty options, false accept/success, malformed results, concurrent out-of-order completions, same/different selection, identity change, disposal before/after dispatch, duplicate completion and unsent abandonment through source inspection. Parent requires full independent Standards and Spec review.

This is source translation, not authorization to transfer credits, greet anyone, create messages or call live endpoints. Team commands/call attachment, Inbox routing/awareness, host producers, native endpoint permission, financial consent UI, trusted transport and backend isolation remain separate work. A Home read grant, call membership, local identity, encounter result or gameplay entry/presence never grants any such action.

No tests, lint, typechecks, builds/compilation, codegen, generation/import/conversion, engine/server launches, installs, remote writes, live calls, deployment or configuration changes. Keep account/readiness gates and world-control interlock disabled. Preserve renderer/mobile inputs and unrelated work. Source acceptance, Windows delivery, content, compilation, in-game inspection and release remain separate.

## SHA-256 pins

```text
fed80430466cc42f622aeac0bfa7d06a173ed3952b21296baf6f42ea5f50d293  packages/api/src/companion-game-social.ts
c1833ae412a501aceec19c45cf0c90b9c9e18fda4f61ed14d1000d2611b3892a  packages/utils/src/companionGameSocial.ts
e01e2d899ac2359023f585de7155b913dc80c1cf58d00aac6afc32ff0aec3711  apps/backend/convex/companionGameSocial.ts
1dee03f6d9d5dfa5bc92ffa0e18205a3c7ac4ccd0e95e27ccf4ef601dd3b694f  apps/backend/convex/lib/companionGameSocialModel.ts
9329c5c1e160fa7021256e8bf407884ad1cb58026f924e67a217d30ff0ee18fb  docs/engineering/unreal-social-teams-state-handoff-2026-10-10.md
9c22d20d1a920b9875ec1e6b89195c5abc5df16fdd02f30932e3ac68d6a1dd13  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionSocialState.h
f67c0d647b1b54744a7f60f13bc99f716d4d43304ac247f5c14bd1842e03403a  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionSocialState.cpp
13ceb1b13bee4b0793ddb117e03559bfb8ea77f8bdb8dc81baa1ffa8e70bb618  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.h
df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.cpp
536bf39a4089acea43be36448ad3d02d3cd7f94a65015d3975f761941b802328  apps/companion-unreal/Source/BaddieCompanion/BaddieCompanion.Build.cs
```
