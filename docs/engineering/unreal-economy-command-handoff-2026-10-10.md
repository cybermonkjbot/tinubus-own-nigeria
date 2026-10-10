# Unreal economy command handoff

October 10, 2026. Local source implementation and source/diff inspection are complete. Independent acceptance, Windows delivery, compilation, native integration and release remain separate.

Final source checkpoint: `6e17f58d5403fcd0a721380f10f3f7b5d1be7333`. The implementation is in `618fe33f4e542d252727f968e8a7e52a7f5d939b`, followed by the preparation-notification correction in `b00bb4c7782ede00a5ed7a2ad044172d2a88a28e` and completion-phase failure correction in the final checkpoint. Integrate all three source commits, or the complete source pair at the final checkpoint.

## Scope and dependencies

Assignment: `docs/engineering/unreal-economy-command-assignment-2026-10-10.md` at `1cf6cf5352efd1db42a02990ea3d5457c83f78a7`. Its SHA-256 is `5b908d410345c4a9bbc2f747b62b11f4982bca629afbb5f7626adfc2e2e4eb5a`.

Canonical/preimage pin: `a7ec09694e565c25e483bbcf9cab8ee58b63707f`. The twelve assignment hashes matched working bytes before editing. The owned source pair had no changes against that pin, and this handoff was absent.

The completion-phase correction starts from `22c36c94e293246e6548ca9812f70835d4f6d5b4`. All three owned files matched that commit before editing. This correction retains its consolidated quote rules and lifecycle-grouped review coverage.

Only these paths belong to this continuation:

- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionGameEconomyState.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionGameEconomyState.cpp`
- `docs/engineering/unreal-economy-command-handoff-2026-10-10.md`

Each source commit contains only the source pair. This handoff is committed separately. All checkpoints use explicit owned paths and disabled hooks. Unrelated work and staging remain outside the checkpoints.

Use the complete, unchanged StateJson pair from `b4bc1b7d7dbc9552ad1451062a404fc872a13224`. Its header has additions since the prior economy read handoff. That earlier handoff's older helper header pin is not this continuation's dependency. The working pair has an empty diff against the required commit. Existing module JSON dependencies suffice; `BaddieCompanion.Build.cs` is unchanged.

The account store, shared economy/equipment definitions, backend command handlers/models and prior economy read handoff were read. Backend files are references only. Existing decoders, Current/Historical policy separation and the vendor three-row ceiling remain unchanged.

## Adapter sequence

The owner stays account-private and single-game-thread. It has no transport, delegates or subscriptions. A future admitted adapter must perform the following sequence:

1. Select the account and obtain any vendor scope from coherent account/world/life/equipment producers. Observation is needed for read capture, not command preparation. Same-account session replacement or loss requires `Clear` followed by reselection, or disposal of the old owner.
2. Call one of the five `Prepare*` methods before notifying listeners. Success captures immutable JSON, the current attempt's target and generations, sets top-level busy and clears the shared error. Keep the receipt in that invocation's own storage. On rejection, emit only if its `ShouldNotifyPreparationFailure()` flag is true, even when the error text equals the previous error. The rejected receipt has no command identity and no command was reserved.
3. Emit the prepared busy snapshot. Reentrant commands are refused while this request owns the global latch. If the adapter will not send, call `AbandonPreparedCommand` before dispatch marking and emit the released state when it succeeds.
4. Recheck account admission outside this owner. Call `MarkCommandDispatched` immediately before sending. A successful mark returns the endpoint and immutable arguments, which must be sent immediately with no yield or notification. Add account credentials only in the admitted transport; the owner never encodes them. A failed mark returns an empty dispatch and authorizes no send. Do not rebuild arguments from current selection or auto-retry the request.
5. Route one callback to `ObserveCommand` with bounded response JSON, or `ObserveCommandFailure` with already-sanitized public text. A true return consumes the callback and supplies a completion, even for a malformed or stale-view result. Busy remains held. A false return rejects the receipt, clears the output and changes no owner state. Keep a previously accepted completion separate from duplicate callbacks' output storage.
6. In the adapter's try phase, if `ShouldRestartQuoteRead()` is true, reconcile the quote subscription using the owner's current `CaptureQuote` receipt. The old quote read is already retired. Stop obsolete transport reads and open the current selected quote only if capture succeeds. If reentry changed selection or observation, reconcile that state rather than the command's old result ID. The account host must stop old-account read/notification work if reentry cleared or replaced the account.
7. Still in try, if `ShouldNotifyBeforeFinish()` is true and the account host still owns this invocation, emit while busy is held.
8. In catch, sanitize a reconnect or pre-final notification failure and call `ObserveCommandCompletionFailure` with the current completion and a separate output variable. On acceptance, replace the current completion with that output before notifying. Emit the error only if the replacement's `ShouldNotifyBeforeFinish()` is true and the account host still owns the invocation. On rejection, keep the existing completion for finally. An already-failed completion cannot transition again; an error-notification failure proceeds to finally without recursive error notification.
9. In finally, call `FinalizeCommand` exactly once with the current completion, including the replacement from catch. Emit the final snapshot only on success. Account clear/disposal invalidates finalization instead of touching another account's state. Final notification happens after release and outside the completion-failure transition.
10. Return the detached optional quote produced by successful finalization. An uninterrupted success returns the raw command result, not a later read value or the newer quote retained in the display. View changes alone do not rewrite it; a completion-phase failure discards it. Do not add a post-final-notification result recheck. Stale-view responses and failures produce an unset result.

The caller retains request IDs for explicit retry. No timer, queue, automatic retry or HTTP dispatch is included here.

## Commands and validation

Every endpoint has prefix `companionGameEconomy:`. JSON uses Unreal objects and the condensed writer, never string interpolation. Vendor scope includes `gameRoomId`, `instanceId`, `lifeId`, `revision` and `equipmentRevision`. All commands include the caller's exact `requestId`.

| Preparation | Endpoint suffix | Payload beyond request ID | Additional result condition after quote decoding |
| --- | --- | --- | --- |
| `PrepareRequestQuote` | `requestQuote` | `selector` with exactly one `offerId` or `serviceKey` | None; no selector/result equivalence is imposed |
| `PrepareConfirmQuote` | `confirmQuote` | `quoteId` | Attempted ID and `approval_required` |
| `PrepareCancelQuote` | `cancelQuote` | `quoteId` | Attempted ID and `cancelled` |
| `PrepareRequestWeaponQuote` | `requestWeaponQuote` | Complete saved scope and `serviceKey` | Service key equals the current attempt's service key |
| `PrepareConfirmWeaponQuote` | `confirmWeaponQuote` | Complete saved scope and `quoteId` | Attempted ID and `fulfilled` |

Ordinary keys match `[A-Za-z0-9][A-Za-z0-9._:-]{0,159}`. Request IDs match `[A-Za-z0-9][A-Za-z0-9._:-]{15,127}`. No trimming or case folding occurs. The typed selector permits one key and rejects an invalid native discriminator. Ordinary invalid request IDs set `Invalid service request ID.` Invalid selectors set `Choose an existing service offer.` Invalid confirm/cancel targets set `Choose an existing quote.`

All preparation requires a selected account, a live owner and no active request. Ordinary commands need neither listeners nor catalog, quote or vendor readiness. Vendor commands additionally require a selected vendor scope; absent scope or invalid request ID refuses silently. Vendor service/quote payloads deliberately skip ordinary key, nonempty, catalog membership, stock and eligibility checks, matching the canonical account store.

The additional native vendor payload bound is 8,192 TCHARs per current attempted service key or quote ID, including a reused packet's new attempt. Empty strings, whitespace and other non-key strings within this bound remain eligible for serialization. An oversized payload or serialization failure sets `The service request couldn't be prepared.` and leaves active/pending ownership unchanged. Existing vendor identities remain nonempty strings of at most 240 TCHARs; revisions remain nonnegative JavaScript-safe integers. These bounds establish no visit or purchase permission.

Command responses reuse `DecodeGameEconomyQuote`, which requires Current policy. Root null, missing fields, invalid policy/status/revision/times and operation-specific mismatches fail. The fixed ordinary response error is `The service response couldn't be read. Retry with the same request ID.` The vendor error is `The purchase response couldn't be read. Retry the same request.`

Transport failure text is public and sanitized before entering the owner. Existing `PublicError` preserves messages through 8,192 TCHARs, including empty strings, and replaces longer text with the generic service error. No raw transport diagnostics or new access-error classifier enters the view. Existing parser limits remain 524,288 TCHARs, depth 16 and 16,384 separators; transport must cap bytes before allocating response strings.

## Pending vendor packet and current attempt

One private vendor packet stores its GUID identity, operation, request ID and serialized arguments. One separate active request owns the global busy latch. Its receipt captures the current attempted payload independently of the saved packet. Its dispatch holds its own immutable argument copy so clearing the pending packet cannot retarget an active command.

The same operation and exact request ID reuse the saved JSON, including its original equipment revision and original service/quote target. A different operation or ID replaces the packet at successful preparation. Ordinary commands neither create nor clear this vendor packet, and retain no ordinary retry packet after release.

For example, a failed vendor request for service A at equipment revision 4 leaves its packet. A retry with the same operation/ID, service B and equipment revision 5 still sends A/4, but validates the response against B. A returned A quote fails that attempt's validation and retains the packet. An explicit retry attempting A can accept a returned A quote and clear it. Vendor confirmation has the same split for attempted quote IDs. No code silently substitutes the saved target into the new attempt's validation.

Transport failures and malformed responses retain the matching pending vendor packet, including definite and ambiguous failures. A valid operation-matching vendor result clears its packet by GUID before view-relevance checks. If a visit change already cleared that packet, the callback does not recreate it or clear another packet. A later reconnect or notification failure also leaves the cleared packet absent.

Unsent abandonment releases only the exact unmarked active request. It sets no response error. If preparation reused an unresolved packet, abandonment preserves those original bytes and identity. If preparation created a packet, abandonment retains that new packet, even though it was never sent. If a different operation/ID replaced an earlier packet during preparation, abandonment does not restore the older one. Failed preparation never replaces a packet. Marked requests cannot be abandoned; they need a callback and finalization, or account teardown.

## Generations and reentry

Account identity, command-view identity, vendor-visit identity and the three read slots are separate. Dispatch/callback ownership checks the exact active request and account generation plus phase. It does not require active observation or current display generations. Thus a prepared request may still dispatch its original bytes after a catalog, recovery or vendor selection change. Those changes control response display relevance instead. External admission remains the adapter's responsibility.

| Event | Read retirement | Command and pending behavior |
| --- | --- | --- |
| Changed valid catalog | All reads | Rotate view; reset display/quote selection; preserve global busy, vendor scope, last scope and pending packet |
| Same valid catalog | None | No change |
| Valid `RecoverQuote`, even same ID or null | Quote | Rotate view; clear displayed quote/error; preserve busy and pending |
| Equipment-only vendor change | Vendor | Reset vendor display with nested busy copied from the latch; retain pending and vendor-view generation |
| Vendor set to null | Vendor | Preserve last nonnull scope, pending and vendor-view generation; new vendor preparation is refused |
| Nonnull vendor matching last visit after null | Vendor | Preserve pending and vendor-view generation; update last scope |
| Different nonnull visit | Vendor | Clear pending and rotate vendor-view generation; preserve active request and its captured dispatch |
| Reload or last observer leaves | All reads | Retain command lifetime/relevance, busy, pending and selections |
| Observation starts again | All reads on reopening | Capture fresh reads without cancelling a command |
| Clear, account replacement or dispose | All reads and account/view/vendor identity | Drop active dispatch, pending and last scope; clear display; disposal is terminal |

Vendor selection equality includes all five fields with exact string comparison. Visit equality omits only equipment revision. The first nonnull scope records the last scope without rotating a prior visit. Invalid selection leaves prior state intact, apart from the existing ordinary public validation errors.

The active request remains busy through catalog resets, recovery, observation teardown and both completion phases. A stale-view callback still completes its exact request. It cannot mutate display/error, but finalization releases that request's latch and the caller emits. A callback from an old account or abandoned/completed request cannot settle newer work. Default, foreign, unmarked and duplicate receipts fail. Duplicate observation cannot consume the first completion, and duplicate finalization cannot release a later request.

Completions also carry an owner-tracked generation and the originating command receipt. `ObserveCommandCompletionFailure` accepts only the current successful completion and rotates its generation. Every copied pre-failure completion becomes stale, so none can restore its successful result or finalize the latch. The replacement has no result and cannot fail a second time. Default, foreign, stale, already-failed and finalized completions refuse the transition without changing owner state. Account clear invalidates both the command and completion identities.

Nested vendor busy is not an alias of top-level command busy. Selection copies the current latch into the empty vendor snapshot. Vendor reads supply their own busy value, and vendor read failures set it false. Command preparation/finalization change only top-level busy, preserving the canonical nested behavior.

## Quote application and notifications

Read and command paths share `ReceiveQuote`. A strictly lower revision for the currently displayed same ID retains the display, readiness and error unchanged. Equal revision replaces. An accepted quote sets quoteReady and clears the shared error. There is no permanent revision floor after read failure or recovery clears the display.

A relevant valid command selects the returned quote ID without checking its service against the current catalog's place/NPC. Neither success path retires catalog or vendor reads. Until a completion-phase failure, the completion retains the raw response even when display retention keeps a newer quote.

| Completion | Before finalization, while busy | Finalization |
| --- | --- | --- |
| Relevant ordinary success | Apply/retain quote; reconnect quote only if ID changed; no command-specific pre-final emit | Release busy, emit, return raw result |
| Relevant vendor success | Clear matching packet; apply/retain quote; reconnect quote even for the same ID and a lower saved response; emit | Release busy, emit, return raw result |
| Relevant transport failure or unreadable response | Set shared error and emit; retain vendor packet | Release busy, emit, return no result |
| Reconnect or success-notification failure | Replace completion; set shared error and emit only if the original view/vendor visit remains relevant; keep applied/read-delivered quote and cleared packet state | Release exact busy latch, emit, return no result even if the error became stale |
| Stale-view callback | No display/error or pre-final emit; valid vendor response may clear matching packet | Release exact busy latch, emit, return no result |

Read callbacks can run during the caller-owned reconnect or notification phase. Their own receipts govern acceptance; the completion-failure transition and finalization do not roll back their quote/readiness. If a pre-final emit returns normally, recovery or catalog change during that emit preserves the captured raw result. If it fails, the replacement completion returns no result regardless of later selection changes. Account clear instead invalidates completion. A listener may start another command during the final emit after busy is released; the older completion is already spent.

The completion-failure transition checks the original command's current view relevance, and its vendor-visit relevance for vendor commands, when setting the shared error. Null/equipment-only vendor changes retain relevance; a different nonnull visit suppresses the vendor error. Ordinary failures ignore vendor-visit changes. Reload and observation teardown do not cancel this transition. Error text must already be sanitized and uses the existing `PublicError` bound. The transition changes neither quote selection nor read generations and never retries reconnect.

Existing read composition remains intact. Catalog failure clears services and sets ready/error; quote failure clears quote and sets quoteReady/error; later valid catalog/quote responses can clear the shared error. Vendor failures use their separate nested error and do not clear the shared one. Lower quote revisions after reload retain readiness/error. Same-account selection remains a no-op. The caller owns subscription teardown and notification for these existing methods.

## Source review and remaining work

Manual source/diff inspection covered these lifecycle stages:

- Preparation and dispatch covered all five payload branches, request predicates, exact strings, immutable dispatch, marking and unsent abandonment.
- Retry and selection changes covered packet replacement/reuse, repeated IDs with changed attempted payloads, account/view/read isolation and null/equipment/new-visit transitions.
- Completion covered all five result branches, duplicate callbacks, shared busy, completion phases, lower/equal quote retention and raw-result return during reentry.
- Completion-phase correction covered ordinary reconnect and vendor notification failures, copied success receipts, duplicate transitions, stale views/vendor visits, account clear, unchanged applied quotes/cleared packets and exact-latch finalization with an unset result.

The initial handoff review exposed missing notification intent for failed preparation, corrected in `b00bb4c7782ede00a5ed7a2ad044172d2a88a28e`. Full Spec review then identified that caller-owned reconnect/notification failure could still finalize as success. At the canonical account-store pin, those operations run inside try and their failure goes through catch before finally releases busy. The final source checkpoint adds that missing transition. Acceptance of the corrected range remains with the parent's existing Standards and Spec reviewers. Parent also owns integration and any Windows dispatch through the coordinator.

The pinned vendor producer still returns eleven rows while the client/native reader accepts at most three. This continuation preserves that mismatch, all eleven per-item price/stock rules and the existing error for an oversized result. It does not truncate or expand the reader.

Missing work includes native account permission, authenticated transport, adapter notification/finalization wiring, live producers, native UI/mounting and content. Local quote/receipt state grants no financial activation, payment, withdrawal, conversion, stock/equipment fulfillment, balance update or world admission. The account/readiness gates and world-control interlock remain disabled and unchanged.

Mission, Jobs, Play, prior handoffs, StateJson, tracker/helper files, backend/mobile and production renderer inputs were not edited. The unmounted native owner does not trigger companion renderer packaging or mobile What's New work. No tests, lint, typechecks, builds, compilation, codegen, generators, asset conversion/import, launches, installs, remote operations, live calls, deployment or configuration changes ran. Compilation, in-game inspection and release remain incomplete.

## SHA-256 evidence

Canonical inputs at `a7ec09694e565c25e483bbcf9cab8ee58b63707f`:

```text
d409aab0642ff3c9bad669737f8dc1014d293f1f38a97b37f4b1c63cf73ca4fa  packages/api/src/companion-game-economy.ts
85ff223aad8814d11b99eefe5c6df19b4ba19a3ee9d3235cf83aef6e522a9e62  packages/utils/src/companionGameEconomy.ts
a3f2f01f9d50da5e7fbf271c2b94bee45104c89620684b497ac2f7ccdaf92325  packages/utils/src/companionCombatEquipment.ts
58ce9f0566e52536a64f1baa17413d83d433e4227f35e17da8b94b38265f8d09  apps/backend/convex/companionGameEconomy.ts
5efe2c04a6d9d4f7cb62fd045e50a970f8d2f7be8b2f79a1fc8af9ff495c2754  apps/backend/convex/lib/companionGameEconomyModel.ts
dcf81c67a07f3745980480926f5a7f04afa9ad9cbdffca0468621e77826a8717  apps/backend/convex/lib/companionWeaponVendorModel.ts
caa82737acc4a1467913697a2da629d481ff1be159a84668b96207a04ac21935  docs/engineering/unreal-game-economy-state-handoff-2026-10-10.md
```

Source preimages under `apps/companion-unreal/Source/BaddieCompanion`:

```text
df46a984167c22880f6b813420806a99e5f07be63d2fa30b523a04ebe4056ac0  Public/BaddieCompanionGameEconomyState.h
8a17a4d0ab29d05d919e0ad8a76a75c0a27ab27ba3a73a75ab083775487df1b9  Private/BaddieCompanionGameEconomyState.cpp
```

Unchanged required dependencies under the same root:

```text
13ceb1b13bee4b0793ddb117e03559bfb8ea77f8bdb8dc81baa1ffa8e70bb618  Private/BaddieCompanionStateJson.h
df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce  Private/BaddieCompanionStateJson.cpp
536bf39a4089acea43be36448ad3d02d3cd7f94a65015d3975f761941b802328  BaddieCompanion.Build.cs
```

Completion-phase correction preimages at `22c36c94e293246e6548ca9812f70835d4f6d5b4`:

```text
1fe9e864847de3e009417b07c7f8421a0dd489e7104789b3e1b99cbd826118f5  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionGameEconomyState.h
bdc9f9c696e3973e6cb76d3e1aca2330be728c57c42bea00290ba26c82162986  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionGameEconomyState.cpp
efcda6bf1c65867624bcd0733df60ee9f6e530fe76f840572589a11ebf946446  docs/engineering/unreal-economy-command-handoff-2026-10-10.md
```

Final source outputs at `6e17f58d5403fcd0a721380f10f3f7b5d1be7333` under `apps/companion-unreal/Source/BaddieCompanion`:

```text
48a523d01521a4764db698c72e9c1113f4a724b870cf1a4e96d8c45460419172  Public/BaddieCompanionGameEconomyState.h
acf3b18c89891e181890340704dbf27daaa32ec4a542f3b07cbf3ddbae0fe193  Private/BaddieCompanionGameEconomyState.cpp
```
