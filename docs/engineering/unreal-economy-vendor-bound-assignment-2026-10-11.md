# Economy vendor catalog bound correction

October 11, 2026. Source-only integration correction.

## Ownership and base

Existing owner `01a12714-f0c9-7153-8f81-94e2816fca6e`, host local, owns exactly four paths:

- `packages/api/src/companion-game-economy.ts`, the vendor service-array length predicate only.
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionGameEconomyState.cpp`, the matching vendor array ceiling and its stale comment only.
- `apps/mobile/src/features/you/whatsNewDropContent.ts`, a Companion source-status comment only.
- New `docs/engineering/unreal-economy-vendor-bound-handoff-2026-10-11.md`.

Canonical/preimage/dependency base is `432c4368040fac84c1ac6bbbb95eed0b53921000`. The parent matched all sixteen committed references and fifteen exact working references. What's New has the separately pinned unrelated banner-save comment; preserve it and exclude it from the commit. The handoff is absent. Read root/mobile AGENTS.md and the existing economy read/command handoffs before editing. The public header, shared StateJson, Build.cs, backend, shared rules, prior handoffs/reviews, Play and all other native domains remain read-only.

## Required correction

The current backend weaponVendor query emits every row in companionWeaponVendorRules.items. The unchanged equipment catalog produces six weapons and five firearm ammo packs, eleven rows. The canonical API and native decoder both cap services at three. This new parent assignment explicitly resolves that documented integration blocker. Earlier assignments' preserve-the-mismatch requirement remains historical, not the rule for this correction.

1. Replace the API's literal three-row ceiling with the length of the existing companionWeaponVendorRules.items collection. Keep every other predicate and callback statement byte-identical.
2. Replace the native decoder's literal three-row ceiling with a count derived from its existing VendorRules array. Use an existing language/engine array-count facility; add no helper, dependency, registry or copied catalog. Remove or correct the stale THREE-row comment. Preserve all eleven rule entries and every other decoder, read-owner and command-owner byte.
3. Preserve empty and partial arrays, ordering and duplicate handling. Accept up to the existing catalog count, not arbitrary arrays, and reject count-plus-one before per-item handling. Keep individual service-key, exact price, stock, unit/source and native endpoint-shape constraints unchanged. Do not filter, truncate, deduplicate or seed anything.
4. Preserve read generations, stale-callback rejection, account clearing, error/readiness/busy behavior, quote ordering, the global command latch, retry packets and completion reentry. This changes no purchase path, financial policy, balance, fulfillment, eligibility or permission.
5. Add a concise Companion source-status comment describing the corrected bound as source-only, unreleased and unverified in-game. Keep the five public highlights and metadata unchanged. Native access and interlock remain disabled.
6. Write the new handoff with exact input/output pins, source/end commits, before/after scope, branch inspection and remaining integration/validation. Preserve historical handoffs. The current producer is the reason for the correction, not proof of a working live vendor.

Source inspection must account for zero, three, four, eleven and twelve rows; malformed or unknown rows; wrong price/stock/unit/source; preserved ordering/duplicates and the unchanged failure/recovery path. Inspect these conditions without executing tests or application code. Confirm all non-owned source regions and read-only references are unchanged.

The shared API serves the existing mobile Companion service. Its only requested behavior difference is accepting the existing complete vendor catalog. No UI layout, renderer code, consumed assets, map data, registry or bridge contract changes; renderer packaging is not triggered. Existing native/web rendered behavior remains unverified unless inspectable in an already-running preview.

## Commit and completion

Use apply_patch and hooks-disabled commits with explicit owned paths/hunks. Check the shared index before staging; do not reset or absorb another owner's staged work. Stage only your What's New hunk, retaining the banner-save working diff. Parent owns tracker and separate Standards/Spec acceptance. Return the exact source and handoff/end commits and hashes in this chat. Do not message other chats or deliver to Windows. Coordinator alone handles later guarded transfer after acceptance.

Read the full metadata guard and direct import before running only `WHATS_NEW_BASE_REF=432c4368040fac84c1ac6bbbb95eed0b53921000 node apps/mobile/scripts/enforce-whats-new-release.mjs`. No tests, lint, typechecks, builds/compilation, codegen, asset operations, new servers/engine launches, installs, remote operations, deployment or live configuration. Full source review, Windows delivery, runtime validation and release remain separate.

## Pinned references

Committed SHA-256 at the base:

```text
d409aab0642ff3c9bad669737f8dc1014d293f1f38a97b37f4b1c63cf73ca4fa  packages/api/src/companion-game-economy.ts
85ff223aad8814d11b99eefe5c6df19b4ba19a3ee9d3235cf83aef6e522a9e62  packages/utils/src/companionGameEconomy.ts
a3f2f01f9d50da5e7fbf271c2b94bee45104c89620684b497ac2f7ccdaf92325  packages/utils/src/companionCombatEquipment.ts
58ce9f0566e52536a64f1baa17413d83d433e4227f35e17da8b94b38265f8d09  apps/backend/convex/companionGameEconomy.ts
dcf81c67a07f3745980480926f5a7f04afa9ad9cbdffca0468621e77826a8717  apps/backend/convex/lib/companionWeaponVendorModel.ts
48a523d01521a4764db698c72e9c1113f4a724b870cf1a4e96d8c45460419172  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionGameEconomyState.h
acf3b18c89891e181890340704dbf27daaa32ec4a542f3b07cbf3ddbae0fe193  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionGameEconomyState.cpp
13ceb1b13bee4b0793ddb117e03559bfb8ea77f8bdb8dc81baa1ffa8e70bb618  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.h
df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.cpp
536bf39a4089acea43be36448ad3d02d3cd7f94a65015d3975f761941b802328  apps/companion-unreal/Source/BaddieCompanion/BaddieCompanion.Build.cs
a63b4dd4605bd2238d6606615a830c43e677c3f927eda36fb3f3b3577a226d6f  apps/mobile/src/features/you/whatsNewDropContent.ts
caa82737acc4a1467913697a2da629d481ff1be159a84668b96207a04ac21935  docs/engineering/unreal-game-economy-state-handoff-2026-10-10.md
8672fae586bf03867f25f869bb1ea97ae5b4f4e3ff34fe74ace358befb05f478  docs/engineering/unreal-economy-command-handoff-2026-10-10.md
e2e108e8b70b8635c224e1951d7c918829356746bb5df3347364a8260632c37f  docs/engineering/unreal-economy-pos-command-review-2026-10-10.md
7a05229218f6a4975b72b645ccdcacfe8101cdba4387e5f262ff32dc93fa7f69  apps/mobile/scripts/enforce-whats-new-release.mjs
5735c431ecc4a0c9f8e9f2df866711159fe0b976aa2a478c13695af0892bb5f5  apps/mobile/scripts/whats-new-api-scope.mjs
```

What's New working preimage is `07a3823b8bb7624c082f29ee0a9c9419d112ad464f493fd1fefbefbb781b52c1`. Its only pre-existing difference is the banner-save comment. Recheck before editing and preserve any additional unrelated changes.

