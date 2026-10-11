# Native Play room-utility observation assignment

October 11, 2026. Source implementation only.

## Ownership and dependency

Existing Play owner `01a12714-f0c9-7153-8f81-94e2816fca6e` owns this continuation. Its prior Play read/command source passed both reviews and reached Windows `8216add1e`. The new utility sampler at `043d3927f`, handoff `85f90afd1`, passed both full reviews. Acceptance and this assignment's canonical/preimage/dependency base are `6419fb26b9a805553f555ccd55b25b02b5ab4345`.

Own only:

- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionPlayState.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionPlayState.cpp`
- New `docs/engineering/unreal-play-utilities-handoff-2026-10-11.md`

Extend the existing FPlayState owner and replace its utility lifecycle placeholders. Reuse the accepted WorldUtilities types and sampler. Keep its pair, ResidentState, StateJson, Build.cs, ScenePower, backend/mobile/shared source, prior handoffs and tracker read-only. Parent matched all eighteen references/preimages and confirmed the new handoff absent. Parent owns integration/acceptance; coordinator alone owns Windows writes.

Read root AGENTS.md, the complete canonical `packages/api/src/companion-play.ts`, its receipt/scope/snapshot types, backend observeRoom response/access contract, utility observation/clock schema, accepted sampler and current Play owner before editing. This is the room-utility part of that account store, not a world/call observation owner.

## Required behavior

1. Add typed room-utility receipt and retained snapshot fields. Decode the exact backend receipt shape with serverNow, characterId, roomId, roomKey, roomRevision, required nullable invitationId, required nullable observation and retryAfterMs. Reuse FWorldUtilityObservation and FResidentClock. Apply bounded UE JSON parsing and existing StateJson facilities. Reject fractional/non-finite/unsafe integers before conversion. Preserve receipt roomKey exactly, requiring 1 through 131072 UTF-16 units. Other receipt fields must meet the captured target checks from the canonical store. Keep malformed data atomic and diagnostics free of private values.

2. Add the local equivalents of setRoomUtilitiesActive and observeRoomUtilities. Eligibility requires a non-disposed active owner, subscribers, selected solo scope without a truthy callSessionId, ready state without a truthy error, and a safe nonnegative roomRevision. This state is request eligibility, not native gameplay or Home permission. Keep existing optional/empty string semantics, not IsSet alone.

3. Capture one immutable non-auth mutation packet for `companionPlay:observeRoom`: characterId, roomId, roomRevision and only a truthy invitationId. Never include callSessionId or credentials. Use a distinct utility generation and request identity alongside the existing session/query generation. One-time marking, callback acceptance and cleanup must reject replaced, stale and duplicate receipts. Describe the external caller's immediate mark/send and abort obligations; do not claim an actual network abort without a transport.

4. Preserve monotonic millisecond timing using the existing UE platform clock, not wall time or a new timer. While a request is pending, elapsed 0 through 15000 ms inclusive suppresses another. A negative age or greater age retires that request, clears displayed utility and emits the canonical refresh error. Preserve the original notification boundary before reevaluating state and timing; use safe detached captures across callbacks. Set next attempt at begin plus 45000 ms and earliest re-entry at begin plus 5000 ms. A late request does not start a retry loop. Clearing sets the next attempt to the retained entry deadline. Failures do not shorten that deadline. The caller drives attempts; no tick, scheduler or server is launched.

5. Clear/retire utility work on Connect, query failure, active-state change, last unsubscribe, session reset and Dispose at the canonical boundaries. Last unsubscribe disables utility and retains prior observation; ordinary select/reconnect/query failure and active toggles retain prior observation too. Only an account/session reset or Dispose drops it and resets account-local deadlines. Keep existing read/command busy semantics, query generations and command receipts unchanged except their existing lifecycle clearing. Do not let utility completion modify play-command busy or command error.

6. On response, recheck session/query/utility/request identity and active state before reading time or applying data. Match the exact captured character, room, revision and invitation. Require positive safe-integer serverNow and retryAfterMs from 0 through 60000. Reject negative or over 15000 ms round trips. Once these receipt checks pass, advance the retry deadline to receivedAt plus max 45000/retryAfterMs before validating observation, as in the canonical store.

7. A non-null observation must have serverNowRealMs equal to receipt.serverNow. Reuse SampleWorldUtilities for full version/key/clock/arithmetic validation. Within the same scope, require unchanged clock fields and strictly increasing server time. Override the validation watermark with the retained previous observation's server time, then retain the original new observation, not the temporary watermark-adjusted copy. Changed scope permits a new epoch only through full validation. For accepted exact schema objects, compare all four scope fields and all six clock fields directly, avoiding TMap serialization order as identity. Document this structural equality for the typed backend contract; reordered/extra wire properties must not bypass the same-scope watermark.

8. A valid null observation clears displayed utility without overwriting previousObservation and clears utilityError. A valid observation retains the full receipt plus receivedAt. Preserve the original received time for later display projection; projected samples must not replace admitted observations or refresh age. Error completion accepts only bounded sanitized public text or the fixed refresh diagnostic and clears displayed utility only for the current request. Finally clears only that request, without retiring newer work created by a callback.

9. Preserve notification order and reentrancy. Subscription callbacks may change scope, active state, account/session, subscriptions or issue a new request. No references into optional/map/array storage survive a callback. Keep the existing owner contract that callbacks must not throw or destroy the owner during a call. Use existing separate completion/finalization patterns where needed to preserve the async account store's observable order.

10. Source-inspect active/solo/ready/error eligibility, empty optional strings, revision 0, all receipt field/null cases, UTF-16 length limit, clock changes and same-scope timestamp regression, valid null responses, equality at 15s/45s/5s, slow and reversed monotonic time, stale callbacks, duplicate completions, subscriber loss, reconnect and same-ID session replacement. Preserve all eighteen existing Play commands, full-wire older-round retention and midpoint serverOffset behavior.

## Limits and handoff

This is retained account-store source and explicit request/receipt staging. No live Convex client, HTTP adapter, credential, admitted native permission, actual abort/timer, automatic renewal, world/call observation, notice, settlement, UI or renderer binding is included. FAcceptedApartmentScenePower remains privately constructible with no producer. Never mint it from a receipt or sampler, edit saved lamp settings, unhide actors, change daylight or enable a gate.

Use apply_patch. Checkpoint only the three owned files with hooks disabled when the shared index is clear. Record source commit, exact input/output hashes, timing/identity/notification contracts, structural decoding/equality differences and source-inspected branches in the new handoff. Independent parent Standards and Spec reviews will cover the full three-path range before a separate Windows delivery. The pending Teams/WorldUtilities batch remains pinned to the acceptance base above and excludes this later Play continuation.

No tests, typechecks, lint, builds/compilation, codegen, asset operations, engine/server/Blender launches, installs, remote operations, deployment or configuration. No other chat messages. Production renderer inputs and mobile/shared behavior are unchanged; packaging and What's New are not triggered by these standalone-only files. Source, delivery, compilation, in-game inspection and release remain separate.

## Pins

SHA-256 values of committed and matching working bytes at the base above:

```text
03ae962bb47624b09c673e9dc9e35ab85a5e0b2e09448e3d70a6b9cf9cd3f8e9  packages/api/src/companion-play.ts
f002e398719f911596fd8c6308445711006be92e8d23fd827431b5784f984952  packages/utils/src/companionPlay.ts
1a27ac9f3cbfbbcc14b5e77cfdea3ba24dbd960362b3e3ddbc13a1f3d01d6c97  packages/utils/src/companionWorldUtilities.ts
75c8f91d25054af64a205e9cf65c3cb96c51891193946637f56aa4f3bd304dcb  packages/utils/src/companionWorldClock.ts
047ced36383ebaffca0d1be7def24209c9344c57ffc1772d8c52cd1d349a4727  apps/backend/convex/companionPlay.ts
9c54ce319e5b6e3198f0fc84034b4f2613a3455162ed9e1096d8eb6d8b71a1c3  apps/backend/convex/lib/companionWorldUtilityObservation.ts
3305230fd179ccab68f79005bf4db8312104e8547fffaa1567ff650dc1494821  apps/backend/convex/lib/companionResidentsModel.ts
6ba177bce942793406ed29cb927f3d10142a1963e5beb513ec81c377a52d9c63  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionPlayState.h
5095cf81315f6f250911008568ad44e48b6a8ab904bc1021d14a9b41e84d2924  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionPlayState.cpp
768e58819bfeed6830939bf54fe7c82a29d26cb280a18c3c53940050bdbe350d  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionWorldUtilities.h
08a9a4b97c4fb1be5639abbb8f3abd8e9256b6be709130b36079f0419dae5646  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionWorldUtilities.cpp
affaccaa74d19c3bd78aa8de5057b163e2cf41c3db1ede616f3abf1829a2275c  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionResidentState.h
13ceb1b13bee4b0793ddb117e03559bfb8ea77f8bdb8dc81baa1ffa8e70bb618  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.h
df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.cpp
44024a15478022a094164e57fa953da32ad19b685c3266371dcbd14c499327b1  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionScenePower.h
536bf39a4089acea43be36448ad3d02d3cd7f94a65015d3975f761941b802328  apps/companion-unreal/Source/BaddieCompanion/BaddieCompanion.Build.cs
330ed17de2284048c8540a227a13cf1da14ac1254e33ab192c5f413221e33bb5  docs/engineering/unreal-play-command-handoff-2026-10-11.md
eb32614a8e71223284171fd15872e5ff9edf50997fc15c21d0fbe71610131a12  docs/engineering/unreal-world-utilities-review-2026-10-11.md
```

