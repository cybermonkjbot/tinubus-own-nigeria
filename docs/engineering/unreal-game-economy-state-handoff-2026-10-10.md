# Unreal game economy read state

October 10, 2026. Source implementation and source/diff review only. Independent acceptance, Windows delivery, compilation, in-game verification and release remain separate.

Source checkpoint: `bd007a3debae7384c48f2d1f38aac89c3c8fcf7d`.

## Scope and pins

The assignment is the Economy reads section of `unreal-economy-pos-football-assignments-2026-10-10.md` at `b8cf46f060748e25e1531cd481e63dd7583a87c5`. The three destinations were absent before implementation. Only these paths belong to this slice:

- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionGameEconomyState.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionGameEconomyState.cpp`
- `docs/engineering/unreal-game-economy-state-handoff-2026-10-10.md`

The source checkpoint contains the two new source files. This handoff is checkpointed separately. Prior Jobs, MissionState, other accepted source/handoffs, shared helpers, backend, mobile, renderer inputs, gates and Config stay unchanged. The new code is unmounted; it triggers neither renderer packaging nor a mobile What's New change.

Canonical pin: `2a97459b581c9da4fe78171cfc2a12dd2afacd7c`. All six inspected working files matched that pin before implementation and during source review.

| Canonical source | SHA-256 |
| --- | --- |
| `packages/api/src/companion-game-economy.ts` | `d409aab0642ff3c9bad669737f8dc1014d293f1f38a97b37f4b1c63cf73ca4fa` |
| `packages/utils/src/companionGameEconomy.ts` | `85ff223aad8814d11b99eefe5c6df19b4ba19a3ee9d3235cf83aef6e522a9e62` |
| `packages/utils/src/companionCombatEquipment.ts` | `a3f2f01f9d50da5e7fbf271c2b94bee45104c89620684b497ac2f7ccdaf92325` |
| `apps/backend/convex/companionGameEconomy.ts` | `58ce9f0566e52536a64f1baa17413d83d433e4227f35e17da8b94b38265f8d09` |
| `apps/backend/convex/lib/companionGameEconomyModel.ts` | `5efe2c04a6d9d4f7cb62fd045e50a970f8d2f7be8b2f79a1fc8af9ff495c2754` |
| `apps/backend/convex/lib/companionWeaponVendorModel.ts` | `dcf81c67a07f3745980480926f5a7f04afa9ad9cbdffca0468621e77826a8717` |

The account store supplies selection, receive predicates, current-display ordering and failure behavior. The shared economy file supplies price/service/quote validity and current/historical policies. Equipment supplies the vendor's fixed price/stock rules. The backend query validators and actual producers establish the response shapes, policy normalization, account ownership and the vendor mismatch below. Command implementations were read to identify exclusions, not translated.

## Native dependencies and boundaries

The implementation uses existing Unreal strings, arrays, optionals, JSON objects and GUIDs. Its only project-specific include beyond its own header is private StateJson. It depends on `bc6e288621f97054c06368a9e7434835c7ce0ede`, byte-identical at the canonical pin. Both helper files matched this dependency during source review and were not edited.

| Dependency under `apps/companion-unreal/Source/BaddieCompanion` | SHA-256 |
| --- | --- |
| `Private/BaddieCompanionStateJson.h` | `fa20d7b05543229ec99caf20904cc68ab0a88b86f910efa777de6f791fc43040` |
| `Private/BaddieCompanionStateJson.cpp` | `df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce` |

Shared `Parse`, `Object`, `Field`, `String`, `Number`, `Boolean`, `Integer`, `Same`, `EnumValue` and `Nullable` own matching low-level reads. The local wrappers express economy key/amount bounds, domain choice tables, bounded object arrays and atomic publication. No scanner or enum loop is copied. All six decoder roots require objects, so shared `ParseNull` is not called. A null quote is not a valid endpoint response.

There is no HTTP client, credential, subscription transport, delegate, timer, command latch, request encoder, retry packet, wallet or simulation. Receipts bind read callbacks only. No local price, policy, stock, ownership flag or fulfilled quote grants account payment, conversion, withdrawal, equipment or gameplay authority.

## Typed data and policy acceptance

`FGameEconomyPrice` preserves the positive integer amount and discriminated unit/source. Real currency is exactly `NGN`, `real-currency-minor` and `main_balance`, with one of the seven canonical main-source keys. Game value is exactly `game-value-unit` and `banked_game_value`, retaining unit key and source key. Fixed literals are represented by the validated variant rather than editable duplicate strings. Amounts and units are never converted or combined.

The policy decoder requires an explicit price unit and `Current` or `Historical` read mode. Both reject enabled financial activation, approved funding and approved payment-source status. Invalid native mode/unit values fail. The accepted distinctions are:

| Field | Current | Historical provenance only |
| --- | --- | --- |
| Live financial activation | `disabled` | `disabled` |
| Conversion for game value | `pending_approval` | `active` |
| Conversion for real currency | `not_applicable` | `not_applicable` |
| Funding and payment-source status | Both `pending_approval` | Both `pending_approval` |
| Withdrawal status | `unavailable` | `available` |
| Exchange rate | Required null | Required object, exactly 100 credits per naira and 1 per kobo |

Historical mode retains the old fields and sets `Provenance` to `EGameEconomyPolicyRead::Historical`; it provides no conversion function or authorization. It implements the shared `legacyCompanionGameEconomyPolicy` compatibility rule in isolation. Services, catalogs, quotes and the state owner always use Current. They cannot opt into Historical. Backend `serviceView`, `companionGameEconomyQuoteToView` and saved request-result projection normalize old immutable policies to Current without rewriting their stored terms/fingerprints. The native read owner does not normalize or silently accept historical live responses.

Services retain offer/service/NPC/place keys, label, term revision, availability revision, price, availability, offer times, quote TTL and policy. Beneficiary must be exactly `self`. Validity follows the client predicate, including labels with surrounding whitespace if their trimmed content is nonempty. Backend provisioning has stricter label whitespace/control-character rules; those issuance rules are not substituted for the read predicate. No current-time availability or expiry is inferred.

Quotes retain ID, complete service terms, issue/expiry times, opaque terms fingerprint, status, revision and required-nullable confirmed/cancelled times. The status constraints are exact:

- `quoted`: revision 0, both times null.
- `approval_required` or `fulfilled`: revision 1, confirmed time present and cancelled time null.
- `cancelled`: cancellation time present; revision 1 without prior confirmation, otherwise revision 2.

Issue time is at least the offer start. Expiry is later than issue, no later than offer expiry, and within the service TTL. Confirmation lies from issue through strictly before expiry. Cancellation is at least confirmation when present, otherwise at least issue; it can occur after expiry. The native reader does not recompute or authenticate the fingerprint and does not require exact issuance TTL when the client permits a shorter duration.

Every decoder builds a fresh candidate. Failure clears its output and returns fixed text. Required-nullable fields reject omission, unknown enum values fail and unknown extra object fields are ignored. Catalog/vendor arrays preserve order and duplicates. Valid empty arrays remain distinct from malformed or null roots through the decoder disposition and read readiness/error state. An absent native `Vendor` means the snapshot has no vendor field, not an accepted empty result. An unset quote in the view can mean no selection, pending recovery or failed read; selected ID, `bQuoteReady`, error and disposition retain the canonical distinction. Root null never becomes an accepted quote.

## Vendor producer mismatch

The pinned `weaponVendor` query appends every `companionWeaponVendorRules.items` row: six weapons plus five firearm ammo packs, eleven rows in total. The pinned client's `connectVendor` rejects more than three rows. This native decoder preserves the three-row ceiling. The current full producer result therefore fails with `The weapon vendor couldn't load.` It is not truncated, filtered to the first three, or accepted under an expanded bound. Resolving that canonical mismatch requires a separate parent decision.

For an in-bound response, each row must match a pinned service key, exact price and maximum stock. Stock is a nonnegative safe integer; price must be `lagos-credit` from banked-game-value source `haul`. All eleven rule choices remain valid individually. The local constants validate received rows only; they do not seed stock, choose inventory or construct offers. Labels and `owned` are retained, not derived from equipment. In particular, the producer's ammo rows use its existing weapon-key comparison for `owned`; the decoder does not reinterpret that boolean as ammunition-purchase eligibility.

The backend validator also requires string labels, boolean `owned`/`ready`/`busy`, and null `error`. The JavaScript receive predicate does not explicitly revalidate all those fields. The typed native decoder enforces their endpoint types and adds an 8,192-character label bound. It preserves received ready/busy values rather than inferring readiness from the array. A local vendor read failure produces empty services, ready false, busy false and public error, leaving the main catalog/quote error unchanged.

## Selection, generations and failures

`FGameEconomyReadState` is account-private and single-game-thread. The external account host calls `SelectAccount`, uses `BeginObservation` for its first listener and `EndObservation` for its last, and owns actual subscriptions/notifications. Additional listeners reuse the active receipts. Copies returned by `GetView` and receipt getters do not alias mutable retained state. Account changes clear all selections/data and require observation to begin again. Same-account IDs are a no-op; session replacement or access revocation must call `Clear` and reselect, or dispose the old owner.

Catalog, quote and vendor use independent local slots and GUID generations. Capture returns current read parameters for the future transport, not encoded arguments or dispatch authority. Failed capture clears the output receipt. Retired, foreign or disposed receipts cannot change state. Ordinary callback failures do not retire their slot, so a later success can recover under the same receipt.

| Operation | Retired reads | Display effect |
| --- | --- | --- |
| Change catalog place/NPC | All three | Reset view and quote selection; retain private vendor scope for a fresh vendor read |
| Recover quote, including same ID | Quote only | Clear displayed quote and shared error; ready only when the selected ID is null |
| Change vendor scope, including equipment revision | Vendor only | Replace vendor with empty not-ready state; leave catalog/quote and shared error intact |
| Reload | All three | Retain services, quote and vendor; clear shared error, reset catalog readiness and selected-quote readiness |
| Last listener leaves | All three | Retain selections, readiness, data and errors |
| First listener returns | All three | Retain displayed state and capture fresh reads |
| Clear/account change/dispose | All three plus account generation | Clear private selections and data; dispose is terminal |

Unchanged catalog selection is a no-op. Invalid catalog keys preserve the selection and data but set `Choose an existing NPC service.` Invalid quote IDs preserve selection/data and set `Invalid quote ID.` A null recovery selection sets quoteReady true without opening a quote read. Invalid native vendor scope or time refuses capture/selection without replacing a valid prior read. Vendor scope equality compares the five typed fields, including equipment revision, with exact strings. It does not depend on JavaScript object property order.

Catalog callbacks require every service to match the captured place/NPC. A single mismatch rejects the whole catalog, clears services and sets ready/error; it is not filtered. A valid response replaces all services in callback order and clears the shared error, even after an earlier quote failure. No catalog revision floor is invented.

Quote callbacks require the captured selected ID. Invalid data, root null, ID mismatch or query failure clears the displayed quote, sets quoteReady true and records the shared error. A valid quote is retained only when the current displayed quote has the same ID and a strictly greater revision. Equal revisions replace. There is no persistent maximum revision after recovery or failure clears the display. A retained older callback changes neither readiness nor error, including after reload set quoteReady false. Catalog/quote successes share one error field as the canonical store does; vendor callbacks do not clear it.

`CaptureVendor` uses a caller-supplied Date.now-equivalent integer within 0 through 4,102,444,800,000, matching the backend clock guard. A fresh vendor slot records that time; recapturing the same slot preserves it. Reload, selection change or observation restart needs a fresh captured time. This is a frozen query argument, not clock advancement or permission to enter the vendor. The source owner must supply validated game-room, instance, life, world revision and equipment revision from coherent current account stores. The query returns no scope identity, so its receipt generation supplies callback isolation.

`ObserveFailure` accepts public text only, caps it to 8,192 characters and preserves an empty string if supplied. Larger messages use a fixed generic fallback. The future transport must sanitize raw exceptions before calling it and perform account-level revocation outside this read reducer. Native code does not add the Jobs store's unrelated access classifier or a new blocked-until-reload rule.

## Bounds and deferred integration

All JSON reads use shared preflight limits of 524,288 TCHARs, depth 16 and 16,384 separators. Future transport must bound bytes before allocating the FString. Keys follow `[A-Za-z0-9][A-Za-z0-9._:-]{0,159}` without trimming or case folding. Service labels are nonblank and at most 80 characters; fingerprints are nonempty and at most 4,000. Account/vendor identity strings have an additional native ceiling of 240; vendor identities must be nonempty. Scope revisions are nonnegative safe integers. These input limits do not establish account or visit ownership.

Amounts are integers from 1 through 1,000,000,000. Services cap at 32 and vendor rows at three. Service/availability revisions and offer times start at one. Offer duration is positive and at most 30 days; quote TTL is 1,000 through 900,000 milliseconds. All ordinary integer fields are JavaScript-safe, and quote revision is 0 through 2. Unlike provisioning's internal guard, the client read predicate permits the safe-integer maximum where its relational constraints still hold. No native rounding, currency formatting or expiry polling is added.

Missing integration includes admitted account transport, listener lifecycle/notifications, current NPC-service selection, coherent world/life/equipment scope production, read callback dispatch and native UI. The vendor producer/read-bound mismatch remains unresolved. Parent review and the existing coordinator own any later source delivery.

Excluded command interactions include quote creation/confirmation/cancellation, vendor purchases, command busy/error composition, `lastVendorScope`/pending-vendor retry ownership, response reconciliation against a mutation, and request IDs. Historical policy inspection does not provide a saved-policy fetcher. Quote availability display, reference-currency formatting, balances, conversions, cashout, grants, settlement, equipment changes, reach/path checks and gameplay admission remain outside this slice. No command latch is represented as a working owner.

## Source review and delivery status

Source and full owned-file diff inspection covered price branches and main sources; current/historical policy separation; nullable/absent times; all quote status/revision rules; atomic failure clearing; catalog scope rejection; vendor rule choices and the 11-versus-three mismatch; callback generation retirement; errors and later recovery; lower/equal quote revisions; same-ID recovery; retained state on reload/unsubscribe; vendor time capture; account replacement and disposal. No independent acceptance is claimed.

| Source under `apps/companion-unreal/Source/BaddieCompanion` | SHA-256 |
| --- | --- |
| `Public/BaddieCompanionGameEconomyState.h` | `df46a984167c22880f6b813420806a99e5f07be63d2fa30b523a04ebe4056ac0` |
| `Private/BaddieCompanionGameEconomyState.cpp` | `8a17a4d0ab29d05d919e0ad8a76a75c0a27ab27ba3a73a75ab083775487df1b9` |

No tests, typechecks, lint, builds, compilation, code generation, asset imports/conversion, installs, engine/server launches, remote operations or live configuration ran. Gates remain disabled and production renderer inputs are unchanged. Source implementation does not complete the economy inventory category or establish native gameplay acceptance.
