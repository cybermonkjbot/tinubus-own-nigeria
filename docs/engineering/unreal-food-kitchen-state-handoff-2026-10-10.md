# Unreal food and kitchen read-state handoff

Date: October 10, 2026.

Source commit: `d9d67d360953aa42b69db558693521288fed614b`.

The typed food and kitchen read slice is implemented and source-reviewed. It is not integrated with a native subscription, UI, pawn or command owner. No tests, compilation, builds, launches or Windows delivery were performed. Account access, gameplay and financial gates remain unchanged and disabled.

## Owned changes

The source commit adds only these four files:

- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionFoodState.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionFoodState.cpp`
- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionKitchenState.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionKitchenState.cpp`

This handoff is the fifth owned path, `docs/engineering/unreal-food-kitchen-state-handoff-2026-10-10.md`, checkpointed separately so it can record the exact source commit. Unrelated concurrent changes were preserved. No existing public header, helper, tracker, mobile or backend file was edited.

## Dependency pin

Both decoders use `StateJsonPrivate` at `2dd54bfe57574b9e281ee4846fa10d51bb0ce9e2`. A scoped diff against that pin was empty before checkpointing, and the source commit contains that exact pair. Deliver a matching shared reader pair with these consumers. Kitchen also requires FoodState for the exact food-key parser and shared read-result enum. The existing WorldPlayer header supplies `TWorldPlayerField` and its presence enum.

During final handoff inspection, concurrent commit `691a60a98ac276d3c9a59793965b5b1bb47a69d1` added `ParseNull` to StateJson for social/team reads. Its diff leaves the functions used here unchanged. Food/kitchen do not depend on that addition and did not edit either helper. The hashes below distinguish the source commit's original dependency from the later working-tree pair; the parent owns the combined delivery pin.

There is no duplicated JSON scanner. The new files use the existing object parser, allocation preflight, exact string comparison, finite scalar readers, safe integer reader, time predicate and required-nullable reader. Local helpers decode only domain objects, arrays, enums and optional fields. No helper extension was needed.

## Decoded contracts

| Contract | Preserved data |
| --- | --- |
| Food scope | Game room, instance, life, world revision, service and optional moving-offer ID |
| Food catalog | Scope, eating, capacity, occupied, server time, ordered offers/orders/carried arrays and optional nullable moving offer |
| Catalog offer | Exact food key, label, price and stock |
| Food order | ID, nullable quote ID, food key, label, place, service, quantity, pickup/delivery, all six statuses, revision and all four timestamps |
| Price | Both real-currency-minor/NGN/main_balance and game-value-unit/banked_game_value branches; all seven main-balance keys; independent game unit and source keys |
| Quote | ID, complete service and policy, issue/expiry, terms fingerprint, all four statuses, revision and required nullable confirmation/cancellation times |
| Service and policy | Offer/service/NPC/place IDs, label, price, self beneficiary, revision, offer times, quote TTL, availability/revision, all activation/approval/withdrawal enums and required nullable exchange rate |
| Moving offer | All 27 public fields, including driving/vehicle/driver identity, room/instance/layout/life/visit, vendor/NPC/revisions, service/bottleneck, stock, amount/unit, timestamps/status/revision and window measurements |
| Food result | Required nullable quote and order, plus optional nullable moving offer; decoding only |
| Kitchen scope | Room, character and optional call session, room revision and invitation |
| Kitchen snapshot | Room, owner, canCook, care revision, required nullable cooking activity, ordered stock/batches/recipes and optional recipe skill fields |

The nine food keys are exact and case-sensitive. Moving offers narrow their key to `suya` or `akara`, quantity to 1/3/6, side to -1/1 and unit to `lagos-credit`, matching the backend offer validator. Recipe keys remain strings, not a replacement recipe allowlist.

Price branches encode their fixed currency/source literals in their types. The decoder populates only the selected branch. Policy fields remain observations even when the wire value says `enabled`, `active`, `approved` or `available`. Nothing here activates payment or authorizes an action.

Stock, capacity, occupied counts, order quantities, batch counts and recipe values are preserved rather than recalculated, clamped or used as admission rules. Guest batch `remaining` is already the server's served projection. The native reader does not infer the host's private stock, add recipes for guests, complete cooking, expire an offer or reconstruct omitted orders.

## Presence and parser boundary

- Required nullable fields reject omission. Null becomes an unset `TOptional`; values remain typed. This covers quote/order results, order quote IDs and terminal times, exchange rates and kitchen cooking.
- Raw catalog/result moving offers retain missing, null and value separately through `TWorldPlayerField`.
- Optional scope and recipe fields reject null. Missing remains unset, distinct from an empty string, zero or false. Kitchen's observed scope is missing until success and then holds the captured value; the canonical read callbacks never assign explicit null to it.
- Public decoder failure resets the entire output and returns fixed diagnostic text. Callers must check the boolean result. Read owners decode internally and publish only complete typed values through const snapshot pointers.
- Objects reject unknown keys and enforce all required fields. Arrays preserve wire order, including empty arrays. Strings are not trimmed, case-folded, replaced or interpreted as credentials.
- Revisions are nonnegative safe integers through 9,007,199,254,740,991. Timestamps and TTLs are finite, nonnegative and bounded by that same maximum, with fractional values preserved. Other numeric fields use the shared finite-number reader. These are native decoding bounds, not a claim that every broad `v.number()` value is meaningful server state.
- Each root is bounded to 2,097,152 characters, depth 32 and 131,072 structural separators. Individual arrays are bounded to 4,096 entries. These are allocation limits, not gameplay capacities. In particular, the kitchen decoder does not incorrectly cap returned batches at 24; the producer's recovery bound is `12 * 24 = 288`.

`DecodeFoodCatalog` and `DecodeFoodResult` use the shared utility contracts with optional moving offers. They do not enforce the moving endpoint's additional whole-response projection rules such as empty offers/orders and `eating: false`. Those values are copied from the response. The moving catalog's backend offer list currently is empty, although its validator permits string keys. Native populated offers use the shared utility's nine-key food union.

## Read lifecycle and callback ordering

These owners are single-threaded. They do not start or cancel network work. The transport must manage actual listener counts, unsubscribe functions and callback lifetime.

1. `Select` compares every typed scope field, including optional presence, with case-sensitive strings. Equal selection is a no-op. Changed selection retires the generation and clears presentation/readiness/error. Selection itself is not an access check.
2. `BeginSubscription` retires any previous generation, retains presentation, captures the selected scope and returns a receipt. It supplies no usable scope when disposed or unselected. Call it for the first observer or an explicit reload. A receipt's generation and captured scope are private and cannot be rewritten by a caller.
3. Food chooses the ordinary or moving endpoint by a present, nonempty moving-offer ID, matching JavaScript truthiness. Missing and present-empty still differ in scope equality. Map `Catalog` to `companionFood:catalog` and `MovingCatalog` to `companionWorld:movingFoodCatalog`. Kitchen uses `companionKitchen:snapshot`.
4. Every callback first checks subscription state, disposal, generation and the full captured selection. Stale receipts return without decoding, publishing, clearing an error or notifying listeners. Food then requires exact response scope equality. Kitchen requires only response room-ID equality because the response does not contain the other scope fields; the receipt still binds those fields.
5. A valid food callback replaces the whole catalog, applies the separate moving-offer retention rule, sets ready and clears the read error. A valid kitchen callback replaces the whole kitchen, records the captured scope, sets ready and clears the read error. Neither owner imposes a new monotonic catalog/care-revision filter.
6. `AcceptFailure` accepts only current receipts. It clears catalog/kitchen and sets ready plus a safe fallback error. Food retains its moving offer. Kitchen retains its last observed scope. Further callbacks from that subscription can recover normally.
7. `EndSubscription` retires callbacks while retaining presentation. Call it before invoking the external unsubscribe function on last-observer removal. After changing selection with observers still present, the adapter must begin the replacement subscription. Merely adding a delegate does not start observation.
8. `Clear` retires callbacks and clears selection and presentation. Food disposal also clears presentation. Kitchen disposal deliberately retains its displayed snapshot, matching the canonical store; account teardown must call `Clear` before `Dispose` and release any external snapshot references.

Notifications occur after a complete state update. Subscription methods themselves do not broadcast. Malformed values return `InvalidResponse`, and valid values with a different scope return `ScopeMismatch`, without changing presentation. The adapter may explicitly report a malformed current read through `AcceptFailure`; it must ignore a scope-mismatched callback rather than turning it into an error for the selected scope.

The moving endpoint's `serverNow` comes from the stored player's update time and may be zero or stale. It is preserved as supplied. There is no native clock-offset, freshness, purchase-window or expiry calculation.

## Moving-offer retention

The retained offer and the current catalog's raw offer are intentionally separate.

| Incoming observation | Retained result |
| --- | --- |
| Missing or null | Keep current, including initial missing |
| Value with no current offer | Take incoming |
| Different ID | Take incoming regardless of revision |
| Same ID, higher revision | Take incoming |
| Same ID, lower revision | Keep current |
| Same ID/revision, current offered | Take incoming, including either terminal state |
| Same ID/revision, same terminal status | Take incoming refreshed fields |
| Same ID/revision, terminal to offered or to a different terminal | Keep current |

This follows `newestMovingOffer` exactly. It does not prevent a higher-revision offered value from replacing a terminal value. Adding that restriction would change the canonical rule. A rejected retained-offer update does not reject the catalog callback or reorder its arrays.

## Deliberate command boundary

This is the read-only part of the stores, not a complete native replacement for `CompanionFoodStore` or `CompanionKitchenStore`.

There is no purchase/cook/serve/eat command API, request identity, busy flag, retryable flag, in-flight state, uncertain request retention, replay, settlement, transport, credentials, authority or UI/pawn binding. Quote and food-result decoders are available independently, but the read owner has no method that installs a command result, quote or order into catalog state. Canonical catalog orders remain subscription-owned. Kitchen command receipts are also outside this slice; they never contain a replacement kitchen snapshot.

Read errors use fixed safe fallback text. The canonical API additionally sanitizes backend/network exceptions and preserves an existing error while a command is uncertain. That transport/command behavior is not implemented here. Future integration must compose command errors separately and preserve the uncertain-command error across read success; it must not treat the read owner's cleared error as command resolution. Food quote revision retention and command-result moving-offer observations also belong to that future integration.

## Source review and remaining work

Manual review covered required/optional/null fields, all unions, array ordering, safe integer/time handling, atomic decoder failure, full captured scopes, stale generations, subscription errors, reload/unsubscribe retention, disposal and the moving-offer comparison. Scoped `git diff --cached --check` reported no whitespace errors. No executable behavior or Unreal compatibility is claimed from that source review.

Parent integration still needs a reviewed, authorized native read adapter with listener/lifetime management and safe endpoint-envelope handling, plus command ownership in a separately scoped change. The coordinator owns Windows delivery. Compile/test/runtime work requires explicit authorization. There was no renderer-affecting or mobile change, so this slice did not alter renderer registrations or What's New content.

## Source hashes

SHA-256 of the four files in the source commit:

```text
ed18e57d0dd2bf2cefd3d013c4947e8031465bba35b472d3aad680cda6efc11e  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionFoodState.h
70c25e9af33373661e03720b2311faf2d919b9a53bdeaf55c7c2b15d0ce99085  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionFoodState.cpp
dfe085173d4b372a72f7cb4cdf78effe274d5f7ee1a33998775a9099038d3b38  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionKitchenState.h
bf4e004bdb149d79d56fa957b1d22ad9728d8a0484486787a5d98857fa186f84  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionKitchenState.cpp
```

SHA-256 of the read-only native dependencies at the source commit:

```text
9e404342b870e7e74ef08bab90b27b3315070bf08211e8659a5ca86d622dc46b  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.h
49f67afef76fc9a5a2d0847d27f99f68d001d76490b3a734ba7fe69b49609b0b  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.cpp
90d230cf0233458c46324fd87d6f077c6f54bd7dd691030ba5a2a89e8607a17d  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionWorldPlayer.h
```

SHA-256 of the later StateJson pair from `691a60a98ac276d3c9a59793965b5b1bb47a69d1`, observed at final inspection:

```text
cf4d239e716530fa055423a2530f91b99dd36bb01b2120aba0427dd20cd4d141  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.h
df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.cpp
```

SHA-256 of the canonical TypeScript sources used to trace this slice. Hashes cover whole files; only the relevant contracts, projections and read-store behavior were translated.

```text
4a0f0c144fa7c219d78e1786c1d250b2c17682606f5907f56779f3402c9f5ac2  packages/api/src/companion-food.ts
2738d6ad02570b0e5fe086e9fa09f05734679658def1409aa045bdef4f5b7127  packages/utils/src/companionFood.ts
f929b0037e76c4d0c2dd1c5e39a4e6ad2285d6284185fc9dece370e13b0d3b16  packages/utils/src/companionMovingHawkers.ts
85ff223aad8814d11b99eefe5c6df19b4ba19a3ee9d3235cf83aef6e522a9e62  packages/utils/src/companionGameEconomy.ts
8bba23bf9d6e79d10d802fd82a02f320bd46c22f69cd0a930a714ff22bb51475  packages/utils/src/companion.ts
4eef31b095b7fc8b4a4f500e048979f3706eb1403b56bec2d01d4a5220f9a7c4  apps/backend/convex/companionFood.ts
ea0856ff060390a0912aabb7627d324d8c7ad2917c2bd84db45cbfbd68585a10  apps/backend/convex/companionKitchen.ts
9f8ef66bf02c445ab688ea7410bfb6410b5c7259bc39ac7d692d0665e2ab9798  apps/backend/convex/companionWorld.ts
6133331747042a7a10386953b63008be5251a7390f93083421595d86d5e28bea  apps/backend/convex/lib/companionFoodSchema.ts
02504385aad7d29422dd265e54ef0f74da68519590c2e43cc1c697f3cf0ace81  apps/backend/convex/lib/companionFoodModel.ts
5935e9dc9a7ea31103afadb854518074f9f63817050cb0cdce97f2197c798554  apps/backend/convex/lib/companionMovingHawkerFoodSchema.ts
10da44ff157adbbc3251badb987978e08a001fc538b73b4a7e2e96036878bbdc  apps/backend/convex/lib/companionMovingHawkerFood.ts
254976690f73cb19383d840946c5e815df1202451657c5e5c5e8303c0ddceddd  apps/backend/convex/lib/companionKitchenModel.ts
c32a5e520891feb45cbea5b1c5f530094c922d51afaa0b8a765a3d584adbdfb0  apps/backend/convex/lib/companionKitchenAccess.ts
5efe2c04a6d9d4f7cb62fd045e50a970f8d2f7be8b2f79a1fc8af9ff495c2754  apps/backend/convex/lib/companionGameEconomyModel.ts
```
