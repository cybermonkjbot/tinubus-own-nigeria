# Unreal read-only haul state

October 10, 2026. Source implementation and source review only. No native caller, network request, Windows delivery, compilation or in-game verification is established by this record.

## Owned files

- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionHaulState.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionHaulState.cpp`
- This handoff record.

The module reuses UE JSON, strings, arrays, optional values, GUIDs and sets, following the existing MissionState and RecoveryState adapters. Matching JSON readers now come from the private StateJson helper. It contains no credentials, HTTP client, cache, subscription transport, delegate, ticker, actor, simulation or command authority. Account/readiness gates and all other modules remain untouched. Production renderer packaging and What's New changes do not apply to these unmounted standalone files.

## Canonical references

SHA-256 values from the files inspected for this implementation:

| Source | SHA-256 |
| --- | --- |
| `packages/api/src/companion-haul.ts` | `b58210375e0936d5184e9c01908f189ad17427d51cfbcca38b9c3de47388b665` |
| `packages/utils/src/companionHaul.ts` | `2e729e6abeba8e28c7b7d328f985451162d8ba036174da86636adac0c9f02ab4` |
| `apps/backend/convex/companionHaul.ts` | `7b0f35c14fafacea9edee972f0138814c8a2fed2daf43c13324b73edb11e8e1a` |
| `apps/backend/convex/lib/companionHaulModel.ts` | `df75cdae2ae1928aa5d2c79d7d776af3a936b1474c35c6cae290f5f58d2e8a3f` |
| `apps/backend/convex/lib/companionGameValueDebit.ts` | `00a227c1923acb0e58cd9fe33031b474f6dcbd842873d78ae3c28dbf03d5cbc5` |
| Installed `node_modules/convex/src/server/pagination.ts`, Convex 1.43.0 | `9cf7cea5512b885d3ef024dfd1be1e25dc0ccdfb3fb5764454362ddae326e995` |

The public backend queries authenticate the account with `requireNonTVSessionByToken`. They are not native-grant endpoints. Nothing here admits a standalone client to them or transfers an account credential to UE.

## Data translation

`DecodeHaulSnapshot` retains the outing ID, character/world/instance IDs, status, revision, count and departure/resolution times. It retains entries in server order with entry and outing IDs, source kind/key, reward variant, status, grant time and optional original money amount. Item key and quantity remain separate from money. A remaining money amount of zero is valid and does not discard the receipt or its original amount.

`outing` and `resolvedAt` require a field but accept null. Missing required fields fail decoding. `originalMoneyAmount` may be absent, but null fails. The three public restrictions remain exactly `cashConversion: disabled`, `withdrawal: unavailable` and `bankedTransfer: disabled`; an absent or changed restriction fails decoding. Snapshot entries must match their outing, status and entry count. A null outing requires an empty entries array. Carried, banked and forfeited snapshots are all supported; the server can return the latest resolved outing when none is active.

Game-value-unit rewards retain their exact unit key. Real-currency-minor rewards retain their currency and funding reference without conversion or funding claims. The backend's grant helper currently rejects new real-currency rewards. The decoder accepting that public reward variant does not bypass the backend rule.

The shared source union includes `street-trade`; the inspected backend source validator does not. The native type and decoder preserve the shared union. This patch does not change the backend validator, produce a street-trade reward or claim that endpoint can currently return one.

`DecodeHaulPage` preserves entry order, `continueCursor`, `isDone` and the installed backend validator's optional `splitCursor`/`pageStatus`. Missing, null and concrete split metadata remain distinct. Empty pages, empty cursor strings, terminal pages and split statuses are data, not local success or completion decisions. No sorting, deduplication, automatic fetching or page accumulation occurs.

`DecodeHaulBalance` preserves a nonnegative safe-integer `bankedCredits`. `GetCreditsOrZero` mirrors `result?.bankedCredits ?? 0`, retaining whether the response was null or the field was missing/null. Malformed data fails instead of becoming an accepted zero. A cleared output is not an accepted balance; callers must inspect the disposition.

The balance is the server's available `lagos-credit` allocation sum, bounded there to 1,000 allocation records. It is not reconstructed from haul rewards. `HaulMoneyGroups` separately translates the shared display helper: group by status, unit kind and case-sensitive unit/currency key in first-seen order; skip items, forfeited entries and zero amounts; retain source-key and funding-reference order and duplicates; reject negative or unsafe amounts/sums. It uses remaining reward amounts, never original grants. It does not combine game units, currencies or statuses and does not calculate an account-money balance.

## Lifecycle and ordering

`SelectAccount` uses exact account-ID equality. A changed or empty account clears the snapshot and every pending read/cursor scope. The same ID is a no-op. On replacement credentials, access loss or a new session for the same ID, the external owner must call `Clear` and reselect, or destroy the state. This replaces the canonical store's immutable captured auth object without retaining credentials in this module.

The external listener owner calls `BeginObservation` for its first listener. Additional listeners share `CaptureObservation` or the same receipt. Starting observation retires reads issued before that start, matching `connect` incrementing the canonical generation. It must call `EndObservation` when its last listener leaves and stop the actual subscription itself.

Every valid current snapshot replaces the previous snapshot in callback order, including lower or equal outing revisions. There is no invented timestamp/revision filter. Success sets ready and clears error. A current subscription failure clears outing/entries, restores the restrictions, sets ready and retains the bounded public error message. Invalid snapshot decoding takes that same clearing path with a fixed diagnostic. A subsequent success can recover in the same observation generation. Subscription errors do not retire independent banked/balance reads because the canonical store does not advance its generation on error.

`Reload` clears to the initial not-ready/no-error snapshot, retires all old receipts/cursors and preserves whether observation is active. The caller stops/reconnects the subscription and captures the new receipt before accepting callbacks. `EndObservation` clears and retires all reads. `Clear` additionally drops account selection. `Dispose` is terminal and clears everything. Old or foreign receipts cannot mutate current state. `GetSnapshot` returns a value copy with no mutable JSON or retained-state aliases.

## Read receipts and pagination

`CaptureBanked` defaults to a null start cursor and 25 items. Sizes outside 1 through 50 fail locally. Its opaque continuation/split cursors are issued only after a current page is accepted and carry the exact account and generation. Cursors from another state, account or retired generation fail. A cursor's contents are never normalized, parsed as authority or persisted by this module.

`EncodeBankedArguments` uses UE JSON serialization and emits only `paginationOpts.cursor` and `paginationOpts.numItems`. The future external transport supplies its separately admitted request context. The canonical operation is `companionHaul:banked` with cache TTL zero. Balance uses `companionHaul:balance` with canonical cache TTL 2,000 milliseconds; snapshot uses `companionHaul:snapshot`. No cache or automatic polling is implemented here.

Banked and balance requests each receive a one-use typed receipt. At most 64 combined requests can be pending. Each completion consumes its own receipt, even on invalid data. Concurrent responses in the same generation can finish in either order; issuing a newer request does not invalidate an older one. There is no global latest-query-wins slot and neither query updates the subscription snapshot.

`AcceptBanked` clears output on invalid/stale responses and returns an explicit disposition. A valid empty page is accepted and distinguishable from the canonical obsolete/disposed null result. `AcceptBalance` likewise clears rejected output; obsolete/disposed reads correspond to the canonical zero fallback, not an observed zero balance. Failed capture must not dispatch. `FailBanked` and `FailBalance` consume only current receipts without touching the snapshot. The external query owner retains its rejection/error behavior, including obsolete promise rejection, and must not interpret a failed query as an empty accepted page or zero balance.

## Bounds and source review

Decoding accepts at most 524,288 TCHARs, nesting depth 12 and 16,384 separators before entering UE's JSON parser. Snapshot entry count is at most 128, page count at most 50, item quantity 1 through 10,000 and reward amount 0 through 1,000,000,000. Original grants are positive and bounded to that amount. Integer fields must be JavaScript-safe. Public timestamps retain finite nonnegative values, including fractions, without rounding. IDs are at most 240 characters, canonical game keys 160, currency 160, funding references 8,192 and opaque cursors 16,384. Text is preserved, including key whitespace accepted by the canonical key guard. Public error retention is bounded to 8,192 characters, with a fixed fallback for larger messages.

These are native input limits, not new server permissions. The installed pagination contract allows reactive result counts to vary from `numItems`; decoding therefore does not require count equality with the requested size. This adapter's independent page ceiling remains 50. It does not support unbounded reactive ranges or silently truncate oversized responses. Transport byte limits must apply before constructing the input FString.

Source/diff inspection covered all reward and source variants, absent/null distinctions, zero remaining rewards, mixed units/statuses, ordered provenance, empty/latest-resolved snapshots, split metadata, malformed and oversized input, error recovery, lower-revision replacement, duplicate query callbacks, concurrent completion order, account changes, same-account reset requirements, reload, unsubscribe and disposal. No tests, typechecks, lint, build, compilation, editor/server launches, imports or live operations ran. This is not runtime evidence or independent review.

Source SHA-256 values for the handoff:

| File | SHA-256 |
| --- | --- |
| `BaddieCompanionHaulState.h` | `3414c83fe57ca20724d0362efda073503e664c14b8724668c46888401a011869` |
| `BaddieCompanionHaulState.cpp` | `8768b4fcaaacc7db44e29e2085547a03955ebaa134f60a58a91f87af03525f57` |

## Shared JSON integration

The original haul source checkpoint is `33eace2e227e81a328ff5cc9c98c20771f15af79`. The missing-helper note at `95c47ca1b6b2f9997dfe7698a09eee40dcffb9b4` is superseded by this integration. The implementation now depends on `Private/BaddieCompanionStateJson.h/.cpp` from `ab535c29cedd4e893a62539f247a03488495b3e9`, owned separately by the combat/recovery chat. Both inspected helper files match that commit; neither was edited here.

The haul parser delegates to `StateJsonPrivate::Parse` with explicit limits of 524,288 characters, depth 12 and 16,384 separators. Object, field, exact string comparison and boolean readers use the helper directly. Bounded text delegates to its string reader. The haul integer wrapper passes the unchanged minimum to the shared safe-integer reader, checks the domain maximum in a temporary and assigns only on success. Time delegates to finite-number reading and `ValidTime`, preserving fractional nonnegative times. Required nullable outing and resolution-time fields use the shared `Nullable` reader with fresh candidates.

The raw-null balance branch is unchanged. Haul-specific ID/key/cursor/text bounds, enum choices, amount maxima, array limits and three-way optional/null/value fields stay in this module. Shared `Only` is not used, so accepted unknown fields remain accepted. No JSON object-key iteration or shared-string conversion was added. Snapshot publication, clearing, account/generation scope, request receipts and response ordering are unchanged. No missing helper operation blocks this integration; the retained wrappers express policies that the helper deliberately leaves to each domain.

Dependency SHA-256 values:

| File | SHA-256 |
| --- | --- |
| `BaddieCompanionStateJson.h` | `90e2436d03e7368501098725653ebad9999e7ca7a6948cb0b61b38cdd577fec4` |
| `BaddieCompanionStateJson.cpp` | `64450793edd25860cc85ed97e316aea275ccf9d8d1e1d059e0e3a2a8427cd7d3` |

Source/diff inspection compared the replaced readers and every changed call site with the original haul implementation. This covers exact parse limits, failure assignment, minimum/maximum numbers, null candidates, whitespace and unchanged raw-null handling. Independent full Standards and Spec reviews accepted integration `6a5acbe7b5e5f7021d403d559ddc6ae279ade09d` with zero findings. `unreal-native-state-review-2026-10-10.md` records the fixed scope and acceptance. Delivery uses the matching reviewed helper at `691a60a98ac276d3c9a59793965b5b1bb47a69d1`; its new scalar-null operation does not change haul's existing readers or raw-null balance branch. No tests, compilation, process starts or Windows handoff ran.

## Remaining integration

The parent owns integration and the existing coordinator owns Windows delivery. Any later haul source delivery must include or verify the matching shared StateJson dependency. A separately admitted, bounded and privacy-safe transport must bind account/session lifetime, listener notifications, query cancellation and response receipts. No current account or gameplay grant is expanded by this module. No HUD, inventory screen, belongings consumer or balance consumer is bound. Native compilation, device/in-game acceptance and release remain unverified.

Rewards, settlement, Home-arrival/death assertions, drop/pickup, robbery, purchases, transfers and command replay remain outside this read module. Nothing here changes saved state or substitutes Unreal replication for backend authority.
