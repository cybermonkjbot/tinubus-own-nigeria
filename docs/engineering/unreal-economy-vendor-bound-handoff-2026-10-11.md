# Economy vendor catalog bound handoff

October 11, 2026. Source correction and source/diff inspection are complete. Independent acceptance, Windows delivery, compilation, in-game verification and release remain separate.

## Scope and checkpoints

Assignment: `docs/engineering/unreal-economy-vendor-bound-assignment-2026-10-11.md` at `47f162d4753bf0092f177088c35dd6e171bf00a9`, SHA-256 `b06fc34ea207b8d93dcc20b31d5a5886536439cf16b0f14fb8339be1f5c423a3`.

Canonical/preimage/dependency base: `432c4368040fac84c1ac6bbbb95eed0b53921000`.

Source commit: `953c16ae78ea12c27869cfa93155a892cc64fa52`. It contains only the API ceiling, native ceiling and stale-comment removal, and the Companion source-status comment. The handoff/end checkpoint is the separate commit adding this document; its exact commit ID and document hash accompany the completion reply.

The backend `weaponVendor` query emits every shared catalog row. Six weapons and five firearm ammo packs produce eleven rows. The prior API and native ceiling was three. The API now derives its ceiling from `companionWeaponVendorRules.items.length`; native uses `UE_ARRAY_COUNT(VendorRules)` with its existing eleven-entry array and includes. No helper, dependency or copied catalog was added.

All other API predicates and callback statements, native executable bytes and rule entries remain unchanged. What's New adds only a source-only, unreleased, unverified-in-game comment. Its five public highlights, metadata and existing comments remain unchanged. The unrelated banner-save working hunk was preserved and excluded from the commit.

## Branch inspection

The following conclusions come from source inspection, not executed tests or application code. Acceptance below requires the existing row predicates and, for native, endpoint-shape constraints to pass.

| Services length | Before | After |
| --- | --- | --- |
| 0 | Accepted | Accepted; empty remains distinct from malformed |
| 3 | Accepted | Accepted |
| 4 | Rejected by count | Accepted |
| 11 | Rejected by count | Accepted, including the complete current catalog |
| 12 | Rejected by count | Rejected before per-item validation |

Neither reader requires every catalog item to appear. Partial arrays, original order and duplicate valid rows remain accepted within the ceiling. There is no filtering, truncation, deduplication, sorting or stock seeding.

Unknown or invalid service keys still fail the existing predicates. Known rows still require the exact catalog price, nonnegative safe-integer stock no greater than that rule's stock, `game-value-unit`, `lagos-credit`, and `banked_game_value` source `haul`. Wrong prices, negative/fractional/unsafe/excess stock and wrong unit/source values fail those checks. Native retains its price parser, label bound, `owned`/`ready`/`busy` booleans and required-null `error` constraints.

Malformed roots and non-array services retain their existing rejection paths. Native rejects malformed rows or nested fields and clears decoder output atomically. The API retains its existing direct nested accesses, which can throw for malformed row, price or source objects. This patch adds no malformed-input hardening; arrays of four through eleven rows now reach those same predicates instead of stopping at the old count check.

The API's predicate-failure branch still publishes empty services, ready false, busy false and `The weapon vendor couldn't load.` Native decode failure still uses that error and its read owner resets the vendor value. Query failures retain their existing public-error paths. Ordinary read failure does not retire the current generation, so a later valid response can replace the failed vendor state. Vendor results leave the shared catalog/quote error unchanged.

Source comparison confirms that read generations, stale-callback rejection, account clearing, quote revision ordering, readiness/busy handling, the global command latch, exact vendor retry packets and completion reentry are byte-identical outside the owned edits. Purchase commands, financial policy, balance, fulfillment, eligibility and permissions are unchanged.

## Verification and remaining integration

The sixteen committed reference hashes matched the assignment. The thirteen read-only references remain byte-identical. Reversing only the owned edits in memory reproduces the pinned API/native preimages and the separately pinned What's New working preimage. This also confirms the untouched five highlights and preserved banner-save hunk. The assignment itself remains byte-identical to its pin.

After reading the complete metadata guard and its direct import, the authorized command ran with exit 0:

```sh
WHATS_NEW_BASE_REF=432c4368040fac84c1ac6bbbb95eed0b53921000 node apps/mobile/scripts/enforce-whats-new-release.mjs
```

Output: `What's New release guard passed.` This is metadata evidence, not runtime acceptance. The shared index was empty before staging. The source checkpoint used explicit owned files and only the new What's New hunk, with hooks disabled. The remaining What's New working diff is the original banner-save comment.

No tests, lint, typechecks, builds, compilation, codegen, asset operations, renderer packaging, new servers or engines, installs, remote operations, deployment or live configuration ran. Renderer code, consumed assets, scene/map data, registries and bridge contracts did not change, so packaging was not triggered. Native access and the world-control interlock remain disabled.

The shared API change affects the existing mobile Companion vendor service. No layout or interaction implementation changed. Native/web rendered behavior and in-game acceptance remain unverified. The producer's eleven-row output explains this correction; it does not establish a working live vendor.

Parent owns the separate Standards/Spec reviews and acceptance. Coordinator alone owns later guarded Windows transfer. Native account admission, authenticated transport, producer/adapter wiring, mounted UI, authorized runtime validation and release remain separate work. Play and all other native domains, prior handoffs/reviews and the tracker were left untouched. No other chat was messaged and no work was delegated.

## Input SHA-256 pins

Committed references at the canonical base:

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

What's New working preimage, including the unrelated banner-save comment: `07a3823b8bb7624c082f29ee0a9c9419d112ad464f493fd1fefbefbb781b52c1`. This handoff was absent before the correction.

## Output SHA-256 pins

Committed outputs at the source checkpoint:

```text
90312d3b3bf41f5b749986420674e2f7c9835db68c3e0be84846999c52ff2e6c  packages/api/src/companion-game-economy.ts
ffde9cfc284f736d4875e65b7334061942e7467e6cc88bf40b76574ec3d5e2f9  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionGameEconomyState.cpp
5e19f6caaa76da4545bb1647d5149b41c0e7388fa0c3349ed901e523041a337a  apps/mobile/src/features/you/whatsNewDropContent.ts
```

What's New working output, retaining the unstaged banner-save comment: `2bfda6e3cb3eeb25098493dd0e5cab87d68328cd9bc9e35291abf487267009d5`.
