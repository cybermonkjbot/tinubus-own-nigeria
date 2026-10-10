# Unreal POS command assignment

October 10, 2026. Account-private request ownership only.

## Owner and pins

Existing POS owner `01a126d5-064b-7703-a27e-31991c45a302`, host local, owns only BaddieCompanionPosState.h/.cpp under `apps/companion-unreal/Source/BaddieCompanion/Public` and `Private`, plus new `docs/engineering/unreal-pos-command-handoff-2026-10-10.md`. Resident, NPC dialogue, WorldPlayer and all previous handoffs stay read-only. Parent owns StateJson, integration, tracker and acceptance. Coordinator alone owns Windows writes.

Canonical reference/preimage pin is `a7ec09694e565c25e483bbcf9cab8ee58b63707f`. All thirteen hashes below match working bytes; the new handoff is absent. Recheck before editing. Use unchanged complete StateJson from `b4bc1b7d7dbc9552ad1451062a404fc872a13224`. This scope is disjoint from active GameEconomy commands.

## Commands and ownership

Read the complete canonical POS store, command union and previous read-state handoff. Extend the existing owner without another receipt store. Add all four commands using UE JSON serialization:

- quote maps to companionPos:requestQuote, with captured scope, amount and requestId.
- confirm maps to companionPos:confirm, with captured scope, quoteId, acceptsCarriedLoss and requestId.
- settle maps to companionPos:settle, with captured scope, quoteId and requestId.
- cancel maps to companionPos:cancel, with quoteId and requestId only. Scope remains local packet ownership, not a cancel wire argument.

For cancel, capture the displayed receipt's scope when its ID matches; otherwise use selection. Preserve the canonical request-ID predicate. Typed commands must preserve finite numeric inputs and consent presence exactly. acceptsCarriedLoss is an explicit true acknowledgement, never a default inferred from a receipt or button. Keep backend term, stock, balance, utility, proximity and admission checks authoritative; local input validation is not eligibility.

Model one immutable account-owned pending packet, one active submission, explicit retry and distinct uncertain state. The initial non-cancel attempt needs matching selected scope; uncertain retry preserves the original packet despite later selection. Cancel has the canonical exception. A new send refuses while any pending packet exists. Capture scope/command/request ID by value before caller notification. Retry never changes original fields or generates a replacement ID.

Provide prepare, one-time dispatch marking, unsent abandonment and completion/failure ownership. Recheck exact account/packet/attempt and relevant initial-scope rules after caller notification and immediately before dispatch. The future admitted adapter must send immutable returned arguments immediately after marking, without yield or notification. Reject default, foreign, stale, unmarked and duplicate completions without settling newer work. Distinguish genuinely unsent abandonment from uncertain sent work; abandoning an unsent retry must preserve the earlier uncertain packet.

Canonical submit marks the packet uncertain and publishes busy/retryable/error before its request. Model preparation versus dispatch truthfully and document any intermediate state. Unsent work is never a fictional network failure. A quote submission clears a truthy recovering ID and restarts the receipt read at canonical order. Existing caller-owned read slots provide restart intents/obligations; no subscription implementation is requested.

## Completion and read reconciliation

Preserve the exact canonical successful submission, catch and finally order. Successful current completion removes pending, applies existing receipt ordering, clears a truthy recovery selection, restarts catalog and relevant receipt reads, and returns the canonical accepted-or-returned result. A result that belongs to a retired account may not enter its replacement. Return values and retained display can differ when ordering keeps a newer receipt; do not collapse that distinction.

Use explicit sanitized transport-failure classification matching isNetworkUnavailableError or the canonical exact generic-save-failure case. Ambiguous failures keep the original packet and allow retry; definite failures remove it. A malformed response cannot establish that a sent request had no effect. Keep uncertainty for that native decode failure and report a fixed diagnostic. Never retain or print raw exceptions/documents. Document this native response-boundary case.

Integrate reconcileReceipt with actual pending state, not a permissive caller boolean. Reconcile only when pending is uncertain, no attempt is inFlight and a displayed receipt exists:

- quote is known only with matching requestId, scope and amount.
- confirm is known with matching quoteId and status other than quoted.
- cancel or settle is known with matching quoteId and a status other than quoted/pending.

A known outcome clears pending/retryable/error. An unrelated receipt does not. Run reconciliation after accepted receipt observations and after matching attempt finally/busy release, preserving the canonical possibility that subscription evidence arrived during a request.

Catalog success and changed selection retain errors only while the actual pending packet is uncertain. Selection retains the account receipt and pending packet, reports busy from inFlight, and changes only its read ownership. Last-unsubscribe/reload retire reads without erasing uncertain work. Recovering ID, null/empty distinctions, latest versus by-ID reads, all six status orderings and lower/equal timestamps remain as accepted. Clear/account replacement/disposal retire all requests and private data. No writes after caller notification may overwrite newer state.

## Completion and limits

Implement with apply_patch and checkpoint explicit owned paths with hooks disabled. The owner writes the new handoff with source commit, input/preimage/output hashes, all four command fields, caller notification/dispatch sequence, initial/retry/cancel differences, receipt reconciliation and every invalidation/error case. Parent requires full independent Standards and Spec review of the complete three-path range.

Settlement is a local request intent only. The existing world owner would schedule any readyAt/expiry wakeup; no scheduler, automatic settlement, stock/debit/ledger logic, cash conversion or payment integration is authorized here. Call presence, scope, catalog and completed receipts never grant withdrawal, world-control or account-payment permission. Native transport/admission, authoritative time/world/service producer, mounting, content, compilation, in-game inspection and release remain absent.

No tests, typechecks, lint, builds/compilation, codegen, generators, conversions/imports, launches, installs, remote writes, live calls, deployment or configuration changes. Preserve production renderer/mobile inputs and disabled account/readiness gates and world-control interlock.

## SHA-256 pins

```text
f15de19b82aacdb0b46258e829e97631ce78bb7a64dd58670a0113eae397f805  packages/api/src/companion-pos.ts
33957fe910e8e00718cb2cba95189abc231c0a9822cf2acb6283ee6a2a2bc992  packages/api/src/convex-error.ts
d52ab1becf30ff0ef9cba4cd321d378c5175540fccfb4201b9d5f73a37832487  packages/utils/src/companionPos.ts
85ff223aad8814d11b99eefe5c6df19b4ba19a3ee9d3235cf83aef6e522a9e62  packages/utils/src/companionGameEconomy.ts
9056dd81d5249ea77a1e268ddb61edddc586b43bdfe0346dd0f493bba70d730a  apps/backend/convex/companionPos.ts
a6722acee2c94b0c6666f3e7546c47076da8ad2a714669dd1a5b861bdb0f6721  apps/backend/convex/lib/companionPosModel.ts
3a912b466b4e55824037b7f159904684dfe86ffa33fd0b26d51c23a76962c1b9  docs/engineering/unreal-pos-state-handoff-2026-10-10.md
11aaaa924d795803228191cc955b54bbdfff93ced7e43769d27e6388d855ef7c  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionPosState.h
a7b18c735b3ccaf930464e71628d0b92adfc942370b9e6356659e11126b7a9ab  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionPosState.cpp
13ceb1b13bee4b0793ddb117e03559bfb8ea77f8bdb8dc81baa1ffa8e70bb618  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.h
df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.cpp
536bf39a4089acea43be36448ad3d02d3cd7f94a65015d3975f761941b802328  apps/companion-unreal/Source/BaddieCompanion/BaddieCompanion.Build.cs
ed2712d780875753caebfe76e9c570ae6952cd5b4bac49ea3f69d0507711a6da  apps/backend/convex/lib/companionPosSettlementModel.ts
```

