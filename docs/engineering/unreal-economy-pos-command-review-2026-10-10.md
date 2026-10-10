# Economy and POS command source review

October 10, 2026. Both corrected source slices are accepted. Windows delivery remains separate.

## Fixed scopes

Both assignments are pinned at `1cf6cf5352efd1db42a02990ea3d5457c83f78a7`, with canonical/preimage base `a7ec09694e565c25e483bbcf9cab8ee58b63707f`. Each review covers the complete state header, implementation and owner command handoff. StateJson remains unchanged at `b4bc1b7d7dbc9552ad1451062a404fc872a13224`.

Economy initial full range ends at `c95fc3129e4e6ff265c5cbdf6e0e25acb4b1b0a1`, 459 additions and 11 removals. It includes implementation `618fe33f4e542d252727f968e8a7e52a7f5d939b` and preparation-notification correction `b00bb4c7782ede00a5ed7a2ad044172d2a88a28e`. Documentation correction `22c36c94e293246e6548ca9812f70835d4f6d5b4` leaves the C++ pair unchanged and brings the full three-path range to 465 additions and 11 removals.

POS initial full range ends at `ae37ea421b8299a0821d7b0110b69bc20842b655`, 435 additions and 17 removals. Its implementation is `89a7a56470cd93b46c7c88a05d66f41e73959d4c`.

Economy's accepted corrected full range ends at `c1ce58add0ae31dc26f9d4f49cc3344df1142012`, 513 additions and 11 removals. Correction source is `6e17f58d5403fcd0a721380f10f3f7b5d1be7333`. POS correction source is `10da2a91e3475a78231865cec05900d3d03f6a80`, handoff `aa28b097eb6d14899a7b607e397e129ae518e88c`. Its accepted corrected full range contains 481 additions and 17 removals. The original reviewers covered the complete corrected scopes.

## Economy Standards

Reviewer `01a127ef-cc0c-7021-806b-602e5b1e87b2` found two P3 documentation breaches at the initial endpoint: repeated quote-restart rules at handoff line 101 and a dense inspection inventory at line 116. The owner corrected both in `22c36c94e`. The same reviewer accepted the full corrected range with zero remaining or new documented breaches.

One optional Duplicated Code judgment remains deferred. GameEconomyState.cpp repeats preparation-error assignment with the notification flag near line 460. This is a maintenance suggestion, not a demonstrated defect. The same reviewer accepted the full corrected range ending at `c1ce58add` with zero remaining/new documented breaches and no new judgment. Completion ownership checks are centralized, the catch/finally instructions install the replacement before notification, both earlier documentation corrections remain intact, and source/dependency hashes match. Accepted; reviewer closed.

## Economy Spec

Reviewer `01a127ef-cb81-7070-90ff-9b4bfa26620e` found one P2 in the full initial range. A successful response can start completion before a caller-owned quote reconnect or vendor pre-final notification fails. BeginCompletion then rejects ObserveCommandFailure, and FinalizeCommand still returns the successful result.

The canonical store at the base pin runs reconnect and the vendor pre-final notification inside try, sets a relevant shared error in catch, returns null and releases busy in finally. The parent confirmed this source mismatch. The owner corrected the pair and handoff at source `6e17f58d5` and handoff `c1ce58add`. The same reviewer accepted the full corrected range with the P2 resolved and no new Spec defects.

Completion-phase failure now rotates the active completion identity and returns a replacement without a result. It preserves applied quotes and cleared vendor packets and sets sanitized error text only while the originating view/vendor visit remains relevant. Default, foreign, stale, duplicate and copied pre-failure success completions cannot finalize replacement or newer work. Clear invalidates the identities. The replacement finalizes after view changes and releases only its latch. All five commands, retry arguments versus attempted targets, generations, preparation notifications, quote ordering and restarts remain covered by the full review. Accepted; reviewer closed.

## POS Standards

Reviewer `01a127f1-4dd5-7c71-bbd0-dbd0750e702c` found zero hard breaches in the full initial range and one optional Duplicated Code judgment. PosState.cpp repeats current-attempt, phase and pending-packet checks near lines 461 and 483. The parent defers a shared-predicate refactor. Pinned output hashes and the required StateJson dependency match.

The same reviewer accepted the full corrected range ending at `aa28b097e` with zero remaining/new documented breaches and no new judgment. The explicit reconnect, acknowledgement and dispatch phases address the required ordering; all unsent phases remain abandonable. Source hashes, original assignment and StateJson match. The optional predicate refactor stays deferred. Accepted; reviewer closed.

## POS Spec

Reviewer `01a127f1-4d4f-7d12-9882-702f6876c2a8` found one P2 in the full initial range. A quote submission with truthy recovery clears recovery and invalidates the receipt slot during marking, but the new header/handoff require the external receipt restart after sending. The canonical account store calls connectReceipt before the mutation. Slot invalidation alone does not preserve the subscription and synchronous notification order.

The parent confirmed the mismatch. The owner completed a separate pre-dispatch restart stage at source `10da2a91e`, handoff `aa28b097e`, in turn `01a127f5-ea7f-7a63-a422-ebcf0840d97d`. The original reviewer accepted the full corrected range with the P2 resolved and no new findings.

Quote reconnect and synchronous notifications now precede acknowledgement and marking. Owner/packet/attempt and phase checks reject repeated entry, duplicate/non-restart acknowledgements and old-attempt completion. Recovery changes during callbacks survive without adding a second command-stage obligation. Final marking rechecks initial scope; retry/cancel retain their exceptions and account replacement invalidates old work. Every unsent phase remains abandonable. Initial work drops; uncertain retry work persists unless genuine receipt evidence resolves it. The full review also covers all four payloads, consent, immutable ownership, failures, receipt ordering, return values, notifications/finally and reconciliation. Accepted; reviewer closed.

## Acceptance boundary

Both slices passed their complete corrected Standards and Spec reviews. All four reviewers are closed. Deliver complete Economy and POS source pairs with matching complete StateJson, both assignments, both owner handoffs and this acceptance record. Coordinator alone owns the separately pinned guarded transfer. Original full reviews remain part of the record; correction-only review does not replace them.

These are local command owners, not authenticated transport, eligibility, payment, settlement scheduling, world admission or mounted gameplay. Economy's eleven-row vendor producer versus three-row reader mismatch remains unchanged. No tests, compilation, runtime inspection, content import, release or enablement is established.

Summary by slice: Economy Standards has zero remaining breaches and one deferred judgment; Spec has zero remaining findings. POS Standards has zero breaches and one deferred judgment; Spec has zero remaining findings.
