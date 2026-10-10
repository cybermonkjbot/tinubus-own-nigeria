# Unreal economy command assignment

October 10, 2026. Local request and response ownership only.

## Owner and pins

Existing owner `01a12714-f0c9-7153-8f81-94e2816fca6e`, host local, owns only GameEconomyState.h/.cpp under `apps/companion-unreal/Source/BaddieCompanion/Public` and `Private`, plus new `docs/engineering/unreal-economy-command-handoff-2026-10-10.md`. Use the full BaddieCompanion filename prefix. Its Mission, Jobs, Play and previous handoffs remain read-only. Mission is frozen for the active guarded Windows transfer. Parent owns StateJson, tracker, acceptance and integration. Coordinator alone owns Windows writes.

Canonical reference/preimage pin is `a7ec09694e565c25e483bbcf9cab8ee58b63707f`. All twelve hashes below match working bytes; the new handoff is absent. Recheck before editing. Reuse unchanged complete StateJson from `b4bc1b7d7dbc9552ad1451062a404fc872a13224`.

## Command ownership

Read the complete canonical account store and previous economy handoff. Extend the existing read owner, preserving its decoders, current/historical policy distinction, read generations, scope validation, quote ordering and error behavior. Keep the vendor's three-row ceiling and documented eleven-row producer mismatch unchanged.

Implement all five existing command operations through UE JSON serialization:

- requestQuote with the exact discriminated selector, offerId or serviceKey, and caller requestId.
- confirmQuote and cancelQuote with quoteId and requestId.
- requestWeaponQuote with the captured complete vendor scope, serviceKey and requestId.
- confirmWeaponQuote with captured complete vendor scope, quoteId and requestId.

Do not serialize credentials. Preserve exact strings and canonical key/request-ID predicates. Validate only contract inputs, never infer catalog availability, price, proximity, equipment readiness or permission. Record any additional native input bound separately. Vendor command validation differs from ordinary-service validation in the canonical store; trace that difference rather than silently adding catalog eligibility.

Add immutable preparation, one-time dispatch marking, unsent abandonment and completion/failure receipts. Capture before caller notifications and recheck owner/generation/request immediately before dispatch. The future admitted adapter must send returned immutable arguments immediately after marking, with no yield or notification. Default, foreign, stale, unmarked or duplicate receipts cannot settle newer work. Keep preparation/callback retirement distinct from canonical view relevance.

The canonical store shares one private inFlight latch across ordinary and vendor commands. Public busy tracks it, including catalog changes during a request. Ordinary requests have no retained retry packet; explicit retry uses the caller's retained ID. Vendor requests keep one pending packet, reusing its original arguments when operation and request ID match, including original equipment revision. Different operation or ID replaces that packet as the canonical store does. Trace repeated IDs with changed attempted payloads: captured wire arguments and the current attempt's result checks are not interchangeable. Never silently retarget the saved wire payload.

Use separate account, view, vendor-visit and read generations where the canonical store distinguishes them. A changed catalog or recoverQuote invalidates command display relevance. A changed nonnull vendor visit, comparing gameRoomId/instanceId/lifeId/revision but not equipmentRevision, clears pending and changes vendor-view relevance. Null vendor scope and equipment-only changes have different retention behavior. lastVendorScope survives null. Reload and last-unsubscribe retire reads without inventing command cancellation. Same-account session loss requires explicit Clear or Dispose. A stale-view completion still releases its own global inFlight latch, but not a newer request or another account.

Unsent abandonment must not invent an ambiguous response or discard an earlier unresolved vendor packet. Record the exact new-preparation versus saved-packet behavior and adapter sequence. No automatic retry, timer, purchase or balance update belongs here.

## Results and preserved reads

Reuse the existing quote decoder and receive/retention logic. Ordinary confirm accepts only approval_required, cancel only cancelled, with matching target ID. Ordinary request uses canonical quote validity without inventing selector/result equivalence absent from the store. Vendor confirm requires fulfilled and matching attempted quote ID; vendor request requires matching attempted service key.

A valid vendor response clears its matching pending packet before view-relevance checks. Failed or unreadable vendor responses retain that packet for explicit retry, including definite failures because this store has no Food/Kitchen-style definite-failure clearing branch. Ordinary failures retain no private request packet. Keep sanitized public error handling distinct from raw transport diagnostics.

Valid relevant responses select the returned quote and preserve newer same-ID displayed revisions. Vendor success retires/reopens the quote read even for the same ID; ordinary success does so only when the selected ID changes. Model these as caller-owned read restart obligations, not subscriptions or network calls. Preserve notification order and finally/busy clearing. Existing catalog/quote/vendor failure and success composition, lower/equal revisions, recovery/reload, subscription lifetime, account clearing and terminal disposal remain intact.

Live-financial activation, payment, withdrawal, conversion, stock/equipment fulfillment, reference-value formatting, balances and world admission remain outside this assignment. Local quote/receipt state grants no such permission.

## Completion

Use apply_patch and checkpoint only owned paths with hooks disabled. The owner writes the new handoff with source commit, input/preimage/output hashes, all five operations, pending/attempt distinctions, lifecycle/reentry cases and remaining adapters/producers. Full independent Standards and Spec reviews cover the complete three-path range.

No tests, typechecks, lint, builds/compilation, codegen, generators, asset conversion/import, launches, installs, remote writes, live calls, deployment or config changes. Preserve production renderer/mobile inputs, disabled account/readiness gates and world-control interlock. Native permission, authenticated transport, live producers, UI/mounting, content, compilation, in-game inspection and release remain incomplete.

## SHA-256 pins

```text
d409aab0642ff3c9bad669737f8dc1014d293f1f38a97b37f4b1c63cf73ca4fa  packages/api/src/companion-game-economy.ts
85ff223aad8814d11b99eefe5c6df19b4ba19a3ee9d3235cf83aef6e522a9e62  packages/utils/src/companionGameEconomy.ts
a3f2f01f9d50da5e7fbf271c2b94bee45104c89620684b497ac2f7ccdaf92325  packages/utils/src/companionCombatEquipment.ts
58ce9f0566e52536a64f1baa17413d83d433e4227f35e17da8b94b38265f8d09  apps/backend/convex/companionGameEconomy.ts
5efe2c04a6d9d4f7cb62fd045e50a970f8d2f7be8b2f79a1fc8af9ff495c2754  apps/backend/convex/lib/companionGameEconomyModel.ts
dcf81c67a07f3745980480926f5a7f04afa9ad9cbdffca0468621e77826a8717  apps/backend/convex/lib/companionWeaponVendorModel.ts
caa82737acc4a1467913697a2da629d481ff1be159a84668b96207a04ac21935  docs/engineering/unreal-game-economy-state-handoff-2026-10-10.md
df46a984167c22880f6b813420806a99e5f07be63d2fa30b523a04ebe4056ac0  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionGameEconomyState.h
8a17a4d0ab29d05d919e0ad8a76a75c0a27ab27ba3a73a75ab083775487df1b9  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionGameEconomyState.cpp
13ceb1b13bee4b0793ddb117e03559bfb8ea77f8bdb8dc81baa1ffa8e70bb618  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.h
df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.cpp
536bf39a4089acea43be36448ad3d02d3cd7f94a65015d3975f761941b802328  apps/companion-unreal/Source/BaddieCompanion/BaddieCompanion.Build.cs
```

