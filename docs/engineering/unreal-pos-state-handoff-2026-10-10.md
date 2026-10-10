# Unreal POS read-state handoff

October 10, 2026. Source implementation and self-review only. Independent acceptance, Windows delivery, compilation, native integration and in-game verification remain pending.

## Source and dependencies

Source commit: `56a3d2ee2fcda4cc3bd18f832c9efa738dad08f9`.

The source commit contains exactly these two new files. Paths are relative to the repository root.

| File | SHA-256 |
| --- | --- |
| `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionPosState.h` | `11aaaa924d795803228191cc955b54bbdfff93ced7e43769d27e6388d855ef7c` |
| `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionPosState.cpp` | `a7b18c735b3ccaf930464e71628d0b92adfc942370b9e6356659e11126b7a9ab` |

All five canonical files were read at `2a97459b581c9da4fe78171cfc2a12dd2afacd7c`. The assignment is recorded in `unreal-economy-pos-football-assignments-2026-10-10.md` at `b8cf46f06`.

| Canonical file | SHA-256 | Contract traced |
| --- | --- | --- |
| `packages/api/src/companion-pos.ts` | `f15de19b82aacdb0b46258e829e97631ce78bb7a64dd58670a0113eae397f805` | Selection, recovery, independent reads, receipt acceptance and read errors |
| `packages/utils/src/companionPos.ts` | `d52ab1becf30ff0ef9cba4cd321d378c5175540fccfb4201b9d5f73a37832487` | Types, six statuses, noncash unit, terms and excluded progress/command contracts |
| `apps/backend/convex/companionPos.ts` | `9056dd81d5249ea77a1e268ddb61edddc586b43bdfe0346dd0f493bba70d730a` | Catalog, receipt and latestReceipt endpoints; required time and account ownership |
| `apps/backend/convex/lib/companionPosModel.ts` | `a6722acee2c94b0c6666f3e7546c47076da8ad2a714669dd1a5b861bdb0f6721` | Wire validators, scope limits, receipt terms, coverage and server-only access/ledger rules |
| `apps/backend/convex/lib/companionPosSettlementModel.ts` | `ed2712d780875753caebfe76e9c570ae6952cd5b4bac49ea3f69d0507711a6da` | Release preserves nullable confirmation/progress fields; settlement remains server-only |

The pair reuses `BaddieCompanionStateJson.h/.cpp`, dependency commit `bc6e288621f97054c06368a9e7434835c7ce0ede`, byte-identical at the canonical pin. Both files were inspected and their working hashes matched. No shared helper changed and no missing generic operation was found.

| Dependency under `apps/companion-unreal/Source/BaddieCompanion/Private/` | SHA-256 |
| --- | --- |
| `BaddieCompanionStateJson.h` | `fa20d7b05543229ec99caf20904cc68ab0a88b86f910efa777de6f791fc43040` |
| `BaddieCompanionStateJson.cpp` | `df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce` |

There is no dependency on the concurrent economy or football slices. Prior WorldPlayer, NPC dialogue and Jobs source was left unchanged.

## Typed values and presence

`DecodePosCatalog` accepts the catalog endpoint's object value. `DecodePosReceipt` accepts a receipt object or an authoritative root null, not a Convex response envelope. Both return immutable shared values, clear output on failure and use fixed parse diagnostics. They copy bounded input before clearing outputs, including when the input aliases the error output or a prior value's string. All JSON parsing, resource preflight, required-null handling and enum matching reuse StateJson.

Scopes preserve `gameRoomId`, `instanceId`, `lifeId`, `revision` and `serviceId`. Matching is case-sensitive across all five fields. IDs remain opaque strings, not native database ownership proofs. Scope lengths and revision limits follow `posAccess`: nonempty instance up to 160 characters, life/service up to 100, and revision from zero through `Number.MAX_SAFE_INTEGER - 1`. Outing revisions use the same integer bound.

Receipts retain all six statuses: quoted, pending, completed, cancelled, interrupted and expired. `confirmedAt`, `readyAt`, `settledAt`, `utilityEventId`, `reason` and `grantedEntryId` must each be present. Explicit null becomes an unset `TOptional`; omission or a wrong type rejects the whole response. Release can terminate an unconfirmed quote, so no extra status-specific nullability rules are imposed. Receipt amounts of 20, 50 or 100, fee 1, matching total and 60,000 ms quote lifetime follow the checks that both read endpoints run through `ownedPosQuote`.

Catalog values retain amount order, fee, cash stock, eligible banked credits, outing identity, scope and captured `serverNow`. An empty amounts array is representable and is distinct from a missing or malformed array. The pinned producer returns three amounts. Catalog policy requires exactly `cashConversion: disabled` and `deposit: own-home-only`. Receipt source requires `banked_game_value` and `lagos-credit`, including the receipt's unit key. `complete` and `bounded` source coverage remain distinct; bounded coverage is not represented as a complete balance. No money, units or timestamps are converted.

`FPosReadView::ReceiptPresence` distinguishes an unread value from an accepted null and a retained receipt. A receipt read never sets catalog readiness. Malformed input and read errors are not interpreted as empty data or zero balances.

Native allocation bounds are separate from gameplay terms: 65,536 input characters, depth 8, 1,024 structural separators, IDs up to 256 characters, text up to 4,096 and at most 128 catalog amounts. Exact expected keys and enum spellings are required. Numeric values must be finite and nonnegative within the safe-number range; revisions and catalog time must be safe integers. Receipt times retain source milliseconds without locally advancing them. Unknown keys, unsupported literals and out-of-bound data fail atomically.

## Caller-driven lifecycle

`FPosReadState` runs on one owning thread. It does not subscribe, dispatch, stop network operations or create credentials. Its sealed owner/read receipts only identify local lifetimes.

- `BeginOwner` replaces the account lifetime and clears selection, recovery ID, catalog, receipt, presence, readiness and error. It also clears private state for same-ID session replacement. Invalid account input clears the previous lifetime without creating a new one. `Clear` permits a later fresh owner; `Dispose` is terminal.
- `SetObserved` represents the first/last observer transition. Both read slots retire, but existing values remain. The caller owns actual subscription start/stop and observer counting.
- `Select` compares the entire scope. An unchanged scope is a no-op. A changed scope, including selection of null, retires only the catalog slot and resets catalog, readiness and read error. The account receipt, its presence, recovery ID and receipt slot survive. Invalid scope input returns false without changing selection.
- `Recover` retires only the receipt slot when the ID changes. A repeated ID is a no-op. A nonempty ID selects the by-ID endpoint. An empty ID selects latest, matching the canonical captured-ID truthiness rule. Unset and empty recovery values remain distinct for generation matching. A successful read does not clear the recovery ID.
- `Reload` retires both slots without clearing their values. Each `BeginCatalogRead` or `BeginReceiptRead` retires only its own prior slot. Read intents accept repeated subscription observations until that slot or account lifetime changes.

The caller must begin the affected reads after observation, selection, recovery or reload changes and retire the corresponding external subscriptions. Catalog begin requires observation and a selected scope. It captures `floor(ServerNowMs)` once, accepts only positive safe-integer milliseconds and exposes the exact non-auth scope/time arguments. Invalid time issues no intent, sets readiness and `Refresh the game timestamp.`, and retains any preceding catalog, as canonical `connectCatalog` does. No local time fallback exists.

Receipt begin requires observation but no selected service. Latest has no service or quote-ID argument. By-ID exposes only its captured quote ID. Account changes, stopped observation, reload and replacement of the relevant read reject old callbacks before decoding or error publication. A service selection change does not invalidate the independent account receipt read. The account/transport producer must bind callbacks to the correct account; the receipt JSON itself has no account-ID field.

Catalog observations require exact captured-scope equality. The returned server time is validated but is not used as a new revision-order filter or clock update. By-ID observations require the exact captured ID; a null or another ID is ignored. Latest observations use canonical receipt ordering without checking against the selected service.

## Receipt ordering and read errors

The native `AcceptReceipt` retains the existing receipt in exactly these canonical cases after structural decoding:

1. The new observation is null.
2. A different receipt has an earlier `issuedAt`, unless it is the explicitly recovering ID.
3. The same receipt would regress from pending to quoted.
4. The current receipt is terminal and the same receipt arrives with any different status.

Equal-time different IDs are accepted. A same-ID observation with the same terminal status can update other fields. A same-ID observation is not rejected for an older issue time or scope revision. Recovery's older-ID exception does not bypass the same-ID status rules. There is no global revision, outing-revision or timestamp monotonicity rule, and no receipt-to-pending-command reconciliation.

| Observation | Catalog/readiness | Receipt | Read error |
| --- | --- | --- | --- |
| Valid matching catalog | Replace catalog; ready | Retain | Clear |
| Catalog failure or malformed catalog | Clear catalog; ready | Retain | Replace |
| Invalid captured catalog time | Retain catalog; ready | Retain | Set timestamp error |
| Valid latest or matching by-ID receipt | Retain | Apply receipt ordering | Retain |
| Receipt failure or malformed receipt | Retain | Retain | Replace |
| Stale read, catalog scope mismatch or by-ID mismatch/null | Retain | Retain | Retain |
| Changed selection | Clear catalog; not ready | Retain | Clear |
| Account replacement, clear or disposal | Clear catalog; not ready | Clear to unread | Clear |

`FPosPublicFailure` accepts only already-sanitized public text from `toSafeConvexError` or a faithful future transport equivalent. This reducer does not sanitize raw exceptions or classify transport failures. Empty/oversized public messages use fixed local fallback text. No raw response document is retained or logged.

The single read error models the canonical store with no pending command. The canonical pending-uncertainty branches that preserve errors after catalog success or selection, plus mutation/read error composition, remain deferred. There is no permissive uncertainty flag, busy/retryable placeholder or command latch claiming to implement them.

## Missing integration and verification limits

The native admitted account transport, bounded response/envelope adapter, safe-error adapter, aligned server-time producer and current world/service selection producer are absent from this slice. A selected scope does not establish service proximity, NPC availability, life/visit eligibility, outing eligibility or consent. The server retains those checks. No POS owner or UI is mounted.

Quote creation, confirmation, cancellation, settlement, command request-ID ownership, retry packets, ambiguous command ownership, ledger allocation, stock mutation, utility progress, expiry/settlement scheduling, carry grants and pending-mutation reconciliation remain unimplemented. There is no payment, conversion or withdrawal integration. A completed receipt records server state and grants no native command or account-payment authority.

Self-review compared all field readers, presence branches, scope/time checks, lifecycle invalidations, recovery matching, error branches and receipt-order predicates against the five pinned files. Source diffs and whitespace checks were inspected. Helper hashes matched the dependency. No tests, typechecks, lint, compilation, builds, codegen, installs, conversions, engine launches, remote operations or runtime inspection ran. Independent Standards/Spec acceptance remains with the parent; Windows delivery remains with the coordinator.

Only the POS source pair and this record were added. No gate, production renderer input, mobile behavior, configuration or prior slice changed. Renderer packaging and What's New changes are not triggered by this unmounted native read slice. It does not complete the economy/gameplay inventory category or establish a release.
