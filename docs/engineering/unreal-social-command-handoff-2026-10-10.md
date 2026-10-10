# Unreal social command ownership

Source checkpoint `95f4dc52a524ea0bbb6cac201008dec33c89c91a`, implementing the October 10 assignment at `7b42b022eff5994a6520585e415d3a41457c2cd1`. Canonical/preimage base is `149ec7f1929da0f9e80d58dc3c2cce8c9eeb6a6a`.

We added command packets, complete typed results and independent return lifetimes to `Public/BaddieCompanionSocialState.h` and `Private/BaddieCompanionSocialState.cpp`. This new handoff is the only other owned path. Existing read declarations, decoder bodies and state methods are unchanged. TeamsState, the earlier combined handoff, StateJson, backend/mobile/renderer, transport, controllers and Config stay outside this patch.

Source and diff inspection are complete for this slice. Full independent Standards and Spec reviews remain pending. No live command, credit transfer, messaging action, native permission or gate enablement occurred. Source acceptance, Windows delivery, compilation, in-game inspection and release remain separate.

## Endpoint contracts

| Endpoint | Exact non-credential arguments | Complete result | Adapter policy |
| --- | --- | --- | --- |
| `companionGameSocial:offer` | `userId`, `callSessionId`, `roomRevision` | `encounterId`, status exactly `offered`/`qualified`/`declined`, required nullable `threadId` | `retry:false`, `invalidateQueryCache:false` |
| `companionGameSocial:respond` | `encounterId`, explicit boolean `accept` | Same encounter result | `retry:false`, `invalidateQueryCache:false` |
| `companionGameSocial:transferCredits` | `recipientUserId`, `credits`, optional `contextPlace`, optional `note` | `success`, `credits`, `kobo`, `nairaLabel`, `recipientHandle`, `recipientName`, `threadId`, `messageId` | `retry:false`, `invalidateQueryCache:true` |

The packet policy getters record these obligations; this module executes neither retries nor cache invalidation. There is no idempotency key, retry slot or automatic second request. A later explicit preparation is a distinct command, not recovery of an ambiguous write.

`FSocialOfferCommand`, `FSocialRespondCommand` and `FSocialTransferCreditsCommand` contain typed inputs. Supplied strings pass unchanged to the UE JSON writer. `accept:false` remains a present false field. Each `TOptional<FString>` keeps omission separate from a supplied empty string, whitespace or other text. Omitted options produce no JSON member, never null. The backend currently does not use note in its effects, but the packet still preserves that canonical input.

Input IDs use the existing opaque-string guard: nonempty, at most 240 TCHARs, no control characters or surrounding whitespace. Rejected IDs are not trimmed or repaired. This validates local shape, not record existence or permission. `roomRevision` accepts finite signed safe integers, from -9,007,199,254,740,991 through 9,007,199,254,740,991, matching the backend's safe-integer scalar predicate without claiming a revision is current. Credits must be finite positive safe integers. Inputs remain doubles until validated, so fractional values are rejected rather than truncated by an integer parameter type. Each supplied optional text is capped at 8,192 TCHARs; no normalization or content filtering occurs. The serialized non-credential argument object is capped at 131,072 TCHARs. Inputs are bounded before encoding, including text that expands through JSON escaping.

`FSocialCommandPacket` holds private shared const data. Its getters return copies, so changing an input object or getter result cannot retarget a prepared call. Endpoint names derive only from the private command kind. The packet also carries the original non-secret identity scope, separate from the argument object. It contains no credential, native endpoint grant or read-state snapshot.

Both new result decoders reuse the accepted social decoder and shared StateJson. The response limits remain 262,144 TCHARs, eight container levels and 4,096 separators outside strings. Public text remains capped at 8,192 TCHARs and result IDs use the existing 240-character guard. Each result object requires its exact fields; missing/unknown fields, incorrect types and unsupported statuses fail. A missing encounter thread does not become null. All failed pure decodes clear the output and return the existing fixed social diagnostic.

Transfer result numbers use the shared finite-number reader, preserving returned values rather than enforcing input rules on a server result. Fractional, zero, signed and finite values beyond the safe-integer range are not rounded, clamped or recomputed. The native code does not derive kobo, nairaLabel, balances, recipient metadata, messaging status or credit movement. A valid `success:false` result remains a typed transfer result with false. A malformed result has no result optional set and an `InvalidResponse` outcome; it is never represented as a valid default transfer. Encounter defaults use `Unavailable`, not `Offered`.

## Independent lifetimes

Preparation requires the current `FSocialState` identity and rejects disposal, but has no dependency on selected offers, active listeners, a displayed peer, readiness or a global busy latch. Every successful preparation creates its own `FSocialCommandRequest`. Multiple offer, response and transfer commands can coexist and complete in any order. No preparation or completion changes offers, player access, reputation, readiness, read errors, team state, balances or Inbox content.

Each request captures separate process-local owner and identity markers. A private shared lifetime holds the stage, command kind, non-secret scope and immutable prepared packet. `GetPreparedPacket` allows inspection only while prepared. It does not mark dispatch or authorize sending. Copies of the request share one lifetime, not independent attempts.

`MarkCommandDispatched` rechecks the issuing state owner, current identity marker, not-disposed state, prepared stage and packet presence. Another state cannot dispatch it, even with the same scope string. Clearing/replacing identity or disposing before dispatch makes marking fail. Returning to an identical scope string after `Clear` still creates a different identity marker. Changes to offers selection, reload, listener lifetime or player watches do not invalidate command dispatch.

Successful marking returns the exact immutable packet and a paired `FSocialCommandReceipt`, changes the shared stage once to Dispatched and removes the packet from the lifetime. It makes no request and emits no notification. Repeating marking through any copied handle fails and clears both outputs. The future admitted adapter must send the returned packet immediately after marking without a yield, callback or notification gap.

The original caller retains its request/receipt pair to settle. Both handles reference the same bounded lifetime allocation. After marking, that allocation retains only the original owner/identity markers, scope of at most 240 TCHARs, command kind and stage. It has no state-owner pointer, credentials, argument JSON, response JSON, delegates, timer or copied read state. The adapter must release its outgoing packet and any inspection copies when they are no longer needed; immutable packet copies remain caller-owned and cannot be revoked by a result.

`Complete` and `CompleteFailure` operate on the original request handle with its matching dispatched receipt. They do not consult current offers or current account state. This preserves the canonical already-started Promise after a view closes, selection changes, identity changes, disposal or destruction of the `FSocialState` object. Their only result is returned to the original caller. The output retains a private `FSocialCommandOrigin`, exact original scope and command kind. It never publishes into any state owner. `MatchesCommandOrigin` is an optional presentation-lifetime check that is false for default, foreign, disposed or replaced identities, including a reused scope string. It is not a permission check and must not gate settlement of an already-sent original request.

Terminal success, invalid response and request failure all consume the shared latch exactly once. Default receipts, receipts from another request, unmarked prepared requests and copied duplicate completions are rejected with a cleared unavailable output. `Complete` builds its candidate before publication, so a JSON input aliasing the caller's previous output remains readable until decoding ends. No raw server exception or response payload is retained. Request failure returns a fixed `RequestFailed` category; it does not assert that the backend made no change.

`Complete` returning true means a terminal outcome was delivered, not that its operation succeeded. The caller must inspect `Outcome.Error`, the appropriate result optional and the returned endpoint fields. This matters both for malformed responses and for a structurally valid transfer with `success:false`.

`AbandonUnsent` succeeds only in Prepared, including when the original state has since ended. It cannot reclassify a sent command as unsent. `DiscardCompletion` requires the matching dispatched receipt and records a separate Discarded terminal stage. Discarding expresses loss of interest, not cancellation, rollback or proof of no write. Every terminal path removes packet and identity metadata from the shared lifetime. Copied handles retain only the terminal stage until released; previously returned outcomes remain owned by their original caller. Dropping all handles also releases the allocation without contacting the backend.

There is no owner-side pending registry or pending-count cap, and no existing request is evicted. Each caller owns its bounded independent request, matching the canonical concurrent-Promise model. All state and request-lifetime operations run on the game thread. Transport callbacks must marshal there before settling; shared stages are not a cross-thread synchronization mechanism.

## Future adapter sequence

1. Establish a non-secret session-lifetime identity in the read owner. Separately retain the actual authenticated transport scope and endpoint permission. Neither the local identity nor a Home/gameplay-entry grant grants a social write.
2. Prepare the exact explicit command. Handle refusal through its fixed local error. Inspect the packet if needed for separate consent/admission; do not send an inspection copy.
3. After external admission, mark that request once and immediately send the returned packet with the matching credentials and packet policy. Do not yield or notify between marking and sending. No adapter or endpoint capability is implemented by this patch.
4. Retain only the original request/receipt handles for completion and release outgoing argument copies after transport no longer needs them. Validate the transport envelope and byte bounds outside this module, then pass its endpoint value to `Complete`, or call `CompleteFailure` on failure. Do not retry a sent command automatically.
5. Deliver the typed result or fixed failure to its original caller. Do not redirect an old result to whichever identity or panel is now current. Use the origin check only before any separately implemented current-view routing; no command result directly updates a wallet, profile, offers list, membership or unread count.
6. If an unsent command is withdrawn, abandon it. If a sent caller explicitly drops completion, discard it without claiming cancellation. Release terminal request, receipt and outcome objects when the original caller finishes.

Team commands/call attachment, Inbox routing and awareness, financial consent UI, native endpoint permission, trusted transport, backend isolation and live integration remain separate assignments. We added no automatic greeting, presence, call join, credit action or local simulation. Account/readiness gates and world-control interlock remain unchanged and disabled.

## Source inspection

| Case inspected | Source outcome |
| --- | --- |
| Three commands and complete results | Exact endpoint fields and cache policies; no credential members or local result calculations. |
| False accept; missing versus empty text; false success | Present boolean encoding, optional IsSet checks and preserved typed false result. |
| Invalid input, overlong strings, unsafe/fractional numbers | Explicit preparation refusal, cleared request output, fixed error; no request allocated or sent. |
| Malformed/partial/wrong-kind result | Atomic output clearing; dispatched request settles once with InvalidResponse and no typed result. |
| Multiple requests completing out of order | Each request/receipt pair has its own latch; no busy flag, response ordering merge or shared result slot. |
| Same/different offers selection, reload, stop or peer-watch release | No command retargeting or cancellation; arguments and command identity remain captured. |
| Identity change or disposal before dispatch | Marking refuses; original caller can abandon the unsent request. |
| Identity change, disposal or owner destruction after dispatch | Original caller still settles; no owner mutation; origin does not match a replacement identity. |
| Default, foreign, unmarked or duplicate completion | Rejected with unavailable cleared output; no other request is consumed. |
| Abandon after send, discard before send, repeated terminal operations | Refused by distinct stage checks; no retry, cancellation or ambiguous-write relabeling. |

The source diff contains additions only in SocialState: command declarations, one owner marker, writer includes and command implementations. Existing read methods and decoder bodies are unchanged. Shared StateJson remains byte-identical to `b4bc1b7d7dbc9552ad1451062a404fc872a13224`. The prior read acceptance and Windows delivery are not acceptance of these new commands.

No tests, typechecks, lint, builds, compilation, codegen, imports, launches, installs, remote operations, deployment or configuration changes ran for this slice. We preserved concurrent work and committed only the owned source paths with hooks disabled. This handoff records inspection, not executable validation or playable completion.

## Hash evidence

All ten assignment hashes matched before editing, and this handoff path was absent. Paths below are repository-relative.

| Canonical or preimage path | SHA-256 at base 149ec7f1929da0f9e80d58dc3c2cce8c9eeb6a6a |
| --- | --- |
| `packages/api/src/companion-game-social.ts` | `fed80430466cc42f622aeac0bfa7d06a173ed3952b21296baf6f42ea5f50d293` |
| `packages/utils/src/companionGameSocial.ts` | `c1833ae412a501aceec19c45cf0c90b9c9e18fda4f61ed14d1000d2611b3892a` |
| `apps/backend/convex/companionGameSocial.ts` | `e01e2d899ac2359023f585de7155b913dc80c1cf58d00aac6afc32ff0aec3711` |
| `apps/backend/convex/lib/companionGameSocialModel.ts` | `1dee03f6d9d5dfa5bc92ffa0e18205a3c7ac4ccd0e95e27ccf4ef601dd3b694f` |
| `docs/engineering/unreal-social-teams-state-handoff-2026-10-10.md` | `9329c5c1e160fa7021256e8bf407884ad1cb58026f924e67a217d30ff0ee18fb` |
| `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionSocialState.h` | `9c22d20d1a920b9875ec1e6b89195c5abc5df16fdd02f30932e3ac68d6a1dd13` |
| `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionSocialState.cpp` | `f67c0d647b1b54744a7f60f13bc99f716d4d43304ac247f5c14bd1842e03403a` |
| `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.h` | `13ceb1b13bee4b0793ddb117e03559bfb8ea77f8bdb8dc81baa1ffa8e70bb618` |
| `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.cpp` | `df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce` |
| `apps/companion-unreal/Source/BaddieCompanion/BaddieCompanion.Build.cs` | `536bf39a4089acea43be36448ad3d02d3cd7f94a65015d3975f761941b802328` |

Output source hashes at `95f4dc52a524ea0bbb6cac201008dec33c89c91a`:

| Path | SHA-256 |
| --- | --- |
| `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionSocialState.h` | `4e9d712d00e1df4c6480fb9954dc2e3f63d7dd9cabbc1c9d1852c484c6fd89e9` |
| `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionSocialState.cpp` | `a5bf55fc69204eff538a7cc49506212ca6a3cf7e0e7648bf3701a30b03e5acd6` |
