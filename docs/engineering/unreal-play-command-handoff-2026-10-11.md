# Unreal room activity reads and commands

October 11, 2026. Source checkpoint `4ac907671ab6d0bee65feb5185a4c21b0eb212d1` implements the retained Play owner and all 18 accepted commands. Verification here is source and diff inspection only.

Assignment: `unreal-play-command-assignment-2026-10-11.md` at `1de31c66058254648f8e4d7dab055dbf443fe98b`. Canonical inputs, dependencies and source preimages use `cfa15652c52e510aac532a8cc4e13ce633d8415e`. Every pinned working input matched before editing. The new handoff did not exist.

The source commit changes only `Public/BaddieCompanionPlayState.h` and `Private/BaddieCompanionPlayState.cpp` under `apps/companion-unreal/Source/BaddieCompanion`. This handoff is the third owned path. StateJson, Build.cs, the original Play handoff, prior domains, backend/mobile, renderer inputs and configuration remain unchanged by this work. Checkpoints use explicit owned paths with hooks disabled.

## Immutable pins

Source SHA-256 at `4ac907671ab6d0bee65feb5185a4c21b0eb212d1`:

| Path under the native module | SHA-256 |
| --- | --- |
| `Public/BaddieCompanionPlayState.h` | `6ba177bce942793406ed29cb927f3d10142a1963e5beb513ec81c377a52d9c63` |
| `Private/BaddieCompanionPlayState.cpp` | `5095cf81315f6f250911008568ad44e48b6a8ab904bc1021d14a9b41e84d2924` |

Preimages and unchanged dependencies at the canonical base:

| Repository path | SHA-256 |
| --- | --- |
| `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionPlayState.h` | `b9eb9bec353858e9175eab5deb3da1535c56b6f7796ded617225ad8cf7c1f3da` |
| `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionPlayState.cpp` | `75add5bd91b5b15619aa8afceb60ec3fb4d48d3e00122cf8e080177e9ff86346` |
| `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.h` | `13ceb1b13bee4b0793ddb117e03559bfb8ea77f8bdb8dc81baa1ffa8e70bb618` |
| `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.cpp` | `df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce` |
| `apps/companion-unreal/Source/BaddieCompanion/BaddieCompanion.Build.cs` | `536bf39a4089acea43be36448ad3d02d3cd7f94a65015d3975f761941b802328` |
| `packages/api/src/companion-play.ts` | `03ae962bb47624b09c673e9dc9e35ab85a5e0b2e09448e3d70a6b9cf9cd3f8e9` |
| `packages/utils/src/companionPlay.ts` | `f002e398719f911596fd8c6308445711006be92e8d23fd827431b5784f984952` |
| `packages/utils/src/companionEmotes.ts` | `32a184e58c20ea6022f66b01d88e831a14511a8a59c90e6cbc949b7f1b633a04` |
| `apps/backend/convex/companionPlay.ts` | `047ced36383ebaffca0d1be7def24209c9344c57ffc1772d8c52cd1d349a4727` |
| `apps/backend/convex/lib/companionPlayModel.ts` | `af006f9d58574836e5dce6136ce54a71d0023b76dfa771a9682d515e83eb7ecb` |
| `docs/engineering/unreal-play-state-handoff-2026-10-10.md` | `e10d0eed4d6efd74edb565c129b0470a10a195db790b6420a6d77534bf4935e4` |

The StateJson pair remains the accepted `b4bc1b7d7dbc9552ad1451062a404fc872a13224` dependency. No generic reader was copied or changed. The existing `DecodePlayWire`, `ReconcilePlayWire`, `FPlayWire` getters and `GetActiveRound` bodies are unchanged. Their original handoff remains authoritative for saved fields, opaque appearance preservation and receive bounds.

## Retained owner and public API

`FPlayState` is noncopyable and nonmovable. It belongs to one account/session incarnation on the game thread. Construction creates private session/query GUIDs. `ResetSession` must run on every account/session replacement, even when public IDs repeat. No caller-supplied readiness or same-scope flag can admit a receipt.

`FPlayScope` stores exact character/room strings and separately present optional call, invitation and room-revision fields. `FPlaySnapshot` holds optional `Wire`, `bBusy`, `bReady`, optional `Error` and `ServerOffset`. An unset wire projects to the canonical empty round, empty player ID and empty memories. A received null-round wire remains set. A query failure can therefore have no wire and still be ready. All getters, arguments and receipts are detached values.

| Public operation | State and notification contract |
| --- | --- |
| `GetSnapshot`, `GetScope` | Detached current values; neither emits. |
| `Subscribe` | Registers a nonempty `TFunction<void()>`, returns an opaque observer receipt and opens a connection generation if none is reserved. No immediate notification. |
| `Unsubscribe` | Consumes that registration once. Last removal retires query/command callbacks and clears busy, retaining scope, wire, readiness, error and offset. No notification. |
| `Select` | Same-value scope is a no-op. A changed scope clears the snapshot, replaces the connection generation and emits. |
| `Reconnect` | Replaces the query generation and retires its command receipts. Retains the displayed snapshot, including busy, and does not emit. |
| `ResetSession` | Clears scope, snapshot, listeners and all receipts; creates a fresh incarnation. No notification. Disposed instances stay disposed. |
| `Dispose` | Performs the session clear and permanently rejects further work. |
| `TakeQuerySubscription` | Returns an immutable snapshot packet once per available generation. Requires a retained scope and observer. Serialization failure consumes the attempt and follows query failure; reconnect is needed for another attempt. |
| `IsCurrentQuery` | Checks the private session/generation and claimed subscription. The transport must stop subscriptions when this becomes false. |
| `ObserveQuery` | Current complete input reconciles and emits. Malformed input follows query failure. Returns wire disposition; `Invalid` also covers a stale receipt, which changes nothing and does not emit. |
| `ObserveQueryFailure` | Current failure replaces the snapshot with empty state, ready true and public error, then emits. Keeps the generation and outstanding command identities. |
| `BeginCommand` | Initial guard, validation, busy notification, then post-notification packet preparation. Returns an opaque receipt only if still current and preparation succeeds. |
| `MarkCommandDispatched` | Consumes a prepared receipt once and returns its immutable packet. No notification. Send immediately, without an intervening yield or callback. |
| `AbandonPreparedCommand` | Removes only an unsent prepared request, clears displayed busy and emits. Dispatched/completing/stale requests cannot be abandoned. |
| `ObserveCommand` | Consumes one dispatched callback, decodes the whole response plus time, updates offset, reconciles and emits on success. Returns a completion receipt. Invalid success sets fixed error without a pre-final notification. |
| `ObserveCommandFailure` | Consumes one dispatched callback and sets public error without notification. Returns a failure completion. |
| `ObserveCommandCompletionFailure` | Converts a current successful completion to failure, rotates its private completion ID and sets public error without notification. Applied wire and offset survive. Every old completion copy is invalid. |
| `FinalizeCommand` | Consumes the current completion once, removes only its request, clears displayed busy and emits. Stale or duplicate completions do nothing. |

`FPlayQueryDispatch` exposes `GetEndpoint`, `GetArgumentsJson` and `GetReceipt`. `FPlayCommandDispatch` also exposes `GetActionId`, `ShouldRetry` and `ShouldInvalidateQueryCache`; the last two always return false. `FPlayCommandCompletion::GetDisposition` reports accepted, retained or consumed failure. Observer/query/command receipts have no writable identity fields or authority flags.

The owner calls listeners synchronously. It visits current registrations in insertion order, skips removed registrations and includes newly added ones. It keeps each active callable alive across removal. Each native subscription has its own token; unlike the JavaScript Set, registering the same callable twice does not deduplicate it. Callbacks may reenter, but must not throw or destroy the owner during a method call. An emission stops if its session/query generation changes. There are no C++ exception handlers or Build.cs changes.

The admitted integration must own subscription creation/cancellation and call finalization after response handling. The owner does not schedule those actions. After a callback, a returned receipt or disposition records the consumed event, not proof that the current view still belongs to it. Later owner methods recheck the private identity. Explicit reconnect preserves canonical connect's busy value; it is not a command-cancellation UI or an automatic busy recovery mechanism.

## Packets and commands

The snapshot endpoint is `companionPlay:snapshot`. Its arguments contain `characterId`, `roomId`, and only the present `callSessionId`, `invitationId` and `roomRevision`. Empty optional strings and zero revisions stay present. There are no credentials or permission fields in either packet. Authenticated transport and admission remain separate work.

The act endpoint is `companionPlay:act`. Its arguments add `command`, required `expectedRoundId` as string or null, required `expectedRevision` as number, and `actionId`. Transport options are `retry: false` and `invalidateQueryCache: false`, not serialized argument fields. The action ID uses `play-`, captured Unix wall-clock milliseconds and a 32-digit native GUID. It remains within the backend's 8..100-character bound and is preserved for the entire request. A marked request is never silently retried or converted back to prepared.

| Accepted typed command | Encoded command fields and validation |
| --- | --- |
| `FPlayStartCommand` | `type: start`, kind `mirror`, `mystery`, `style`, `kitchen` or `memories`, and optional boolean `rush`. Present false is preserved. |
| `FPlayJoinCommand`, `FPlayLeaveCommand`, `FPlayReplayCommand`, `FPlayLeadCommand`, `FPlayPerformCommand`, `FPlayWatchCommand`, `FPlayChopCommand`, `FPlayHeatCommand`, `FPlayPlateCommand`, `FPlayServeCommand`, `FPlaySaveMemoryCommand` | Type only, respectively `join`, `leave`, `replay`, `lead`, `perform`, `watch`, `chop`, `heat`, `plate`, `serve`, `save-memory`. |
| `FPlayTouchCommand` | `type: touch`, integer `target` 0..5, optional finite `power` 0..1. Omitted, zero and one remain distinct. |
| `FPlayEmoteCommand` | `type: emote`, one of `greeting`, `nod`, `cheer`, `stretch`, `groove`, `sway`. |
| `FPlayLookCommand` | `type: look`, nested saved `FPlayLook` with `look` rose/ivory and all three exact palette colors. |
| `FPlayVoteCommand` | `type: vote`, `playerId` at most 100 TCHAR code units, including empty. |
| `FPlayIngredientCommand` | `type: ingredient`, integer `ingredient` 0..5. |
| `FPlayRemoveMemoryCommand` | `type: remove-memory`, `memoryId` at most 100 TCHAR code units, including empty. |

The exact case-sensitive palette is `#e7adbb`, `#ede4d5`, `#77856b`, `#343340`, `#b48755`, `#809bac`. Saved read colors remain unrestricted bounded strings. Invalid enum casts, nonfinite/fractional indices, out-of-range power and invalid palette values reject atomically. `std::monostate` represents malformed/unknown input. `FPlayRetiredToyCommand` and start kind `Playground` exist only to reject retired input; neither serializes. Typed inputs cannot contain unknown fields or wrong JSON value types.

`BeginCommand` initially guards only disposed, missing scope, busy or not ready. It does not require observers, an error-free view, a round, membership or host status. Invalid/retired input sets `This activity is no longer available.` and emits without dispatch. A displayed playground round blocks every command except valid start and remove-memory.

The owner copies caller input, captures generation and began time, reserves a per-request record, then sets busy true, clears error and emits. After notification it reacquires that record and reads the current scope/round. A same-generation query update changes the outgoing expected round/revision. Query failure during notification produces null/zero expectations. Session/scope/query replacement retires the record and prevents dispatch. The retired-round check is deliberately before notification, as in the canonical store.

`expectedRevision` must be a nonnegative safe integer no greater than 9007199254740991. Read revisions remain finite doubles, including negative, fractional and larger finite values. An unencodable post-notification revision causes local command failure/finalization without rounding or replacing the saved wire. Scope strings have an additional 8,192-TCHAR resource ceiling, and present scope revision must be finite for JSON. These are structural checks, not local character, room or invitation admission. Negative/fractional finite scope revisions remain representable; utility observation rules do not apply to act scope.

UE JSON serialization owns escaping and numeric/string types. Complete outgoing JSON uses the existing Play preflight ceilings. Scope comparison ignores object property order, compares strings exactly and preserves optional presence. Signed zero compares equal. Nonfinite scope values are never encoded; repeated NaN scope values compare equal locally. The canonical JSON-stringified comparison has no useful typed nonfinite scope contract.

## Response order and source traces

Current success first validates the full wire with `ReconcilePlayWire`, then reads finite `serverTime` through a second bounded parse. Neither parse publishes partial state. Receive time is captured from native UTC milliseconds on entry. Offset is `serverTime - (began + receivedWallTime) / 2`; a nonfinite computed result rejects. No monotonic clock, timestamp positivity check, RTT limit or minimum sample policy is substituted for the canonical wall-clock calculation.

Accepted null/different/equal/newer round data replaces the complete wire, sets ready and clears error. Older same-ID data retains the whole prior wire, readiness and error. Both valid outcomes update offset first, emit, and leave finalization to the caller. Finalization clears busy and emits again. Query acceptance does not change offset or busy.

Command failures retain the snapshot data and offset, set error and do not emit until finalization. Missing or oversized public text uses `That move couldn't finish. Try again.`; present empty text remains empty. Malformed success uses that same fixed diagnostic. Query failures use `Room activity couldn't load.` when public text is absent/oversized, reset offset and busy with the empty snapshot, set ready true and emit. Public-text methods accept only already sanitized text from the future admitted transport. The local cap is not a sanitizer for arbitrary exception messages or response bodies.

The complete two-file diff was read against the canonical store, shared command reader and endpoint validators. These are manual source traces, not executed tests:

- All 18 alternatives map to the canonical fields. Optional false/zero values survive; empty vote/memory IDs are allowed; retired actions and invalid numeric/enum/palette input have no packet. Every caller string goes through UE serialization.
- A null-round subscription is received state and permits commands. Equal revision replaces player ID/memories as well as the round. A strictly older same-ID wire retains all of them. Malformed older data still fails complete validation.
- A busy listener that delivers a newer same-generation round changes the packet's expected fields. A listener that selects another scope, resets the same public account IDs, reconnects or disposes prevents the old request from dispatching. Caller variable replacement cannot rewrite the by-value command, receipt or response already being processed.
- A dispatched request A survives a query failure in the same generation. That failure sets busy false and permits B. A's success follows offset, accept emission, then final emission; A's finalization removes only A and may clear busy while B remains outstanding. B can still dispatch or complete. A duplicate callback/finalization cannot consume B.
- A query failure during A's busy notification can start B reentrantly. No map pointer survives that notification; A reacquires its own record before preparation. A serialization failure removes only A.
- Command failure followed by a query update before finalization preserves the intervening update. Finalization changes only busy and request ownership. A caller-side completion failure rotates its completion identity, so old success copies cannot finalize it.
- Success listeners may unsubscribe, select, reset or dispose. The emitted result is already applied, but the old completion cannot alter replacement state afterward. Recursive duplicate response calls see a completing stage and are rejected.
- After last unsubscribe, retained ready data still permits a no-observer command. First resubscription replaces the query generation and retires that command's callbacks. No observer-presence or global pending-request guard was introduced.
- A fractional/unsafe read revision is retained as read data and refused only at post-notification packet preparation. Missing/nonfinite server time rejects the command response without publishing its wire or offset.

The original read parser, saved appearance serialization, limits of eight players/twelve memories, game-array bounds, required-nullable fields and shared `ReadBoundedArray` calls are untouched. The source diff contains additions only.

## Utility observation remains unimplemented

There is no utility-ready flag, observation generation, room-utility power value, `observeRoom` call or NativeApartment integration in this patch. The following canonical boundaries are recorded for the future utility owner; code comments identify the matching lifecycle locations.

| Boundary | Canonical utility work still required |
| --- | --- |
| Connect, including first observer and changed selection | Increment utility generation, set `nextUtilityAt = nextUtilityEntryAt`, detach/abort the outstanding request and clear `roomUtility`/`utilityError`. Keep activity and previous observation unchanged. |
| Last unsubscribe | Disable utility activity and perform that clear/abort. Preserve previous observation and entry pacing. |
| Current query failure | Perform clear/abort, then replace the Play snapshot with empty ready/error state. Keep utility activity and previous observation. |
| Dispose or native account/session replacement | Disable, clear/abort and discard previous observation along with private account state. |
| Explicit utility-active change | The future `setRoomUtilitiesActive` equivalent must clear/abort and emit. No such public method is implemented here. |

`companionPlay:observeRoom` is a separate authenticated mutation using a one-request client, not a snapshot query or an act variant. Its current guards require active visible observers, scope, ready/error-free state, no truthy call-session ID and nonnegative safe room revision. It uses performance time, a 15-second timeout/abort boundary, 45-second retry pacing and a retained 5-second entry floor. Receipt validation must match character/room/revision/invitation, bound retry delay, validate server/receive timing and room key, and validate observation sample/clock continuity against the previous observation. Plain utility clearing does not discard that previous observation. None of these obligations is satisfied by this command owner.

## Remaining integration and status

The parent owns full-range Standards and Spec reviews, acceptance and shared integration. The coordinator alone owns Windows delivery. Review the complete three-path range from the canonical base through the source and handoff checkpoints; prior read acceptance is not command acceptance.

| Stage | Status for this continuation |
| --- | --- |
| Source implementation | Complete in the source checkpoint above. |
| Author source/diff inspection | Complete; traces and bounds recorded here. |
| Independent Standards/Spec reviews | Pending parent acceptance. |
| Windows delivery | Not performed by this continuation. |
| Compilation and executed tests | Not run. |
| In-game inspection | Not run. |
| Release | Not performed. |

Authenticated transport/admission, query teardown wiring, native UI and dedicated confirmations, anchors/navigation, player bindings, emote/appearance playback, utility observation and rendering remain following work. Kitchen puzzle commands confer no groceries, meal or Life care authority. Home/apartment reads and gameplay entry/presence confer no act, call, voice, payment or editing permission. All existing account/admission gates and the world-control interlock remain unchanged and disabled.

No tests, lint, typechecks, builds/compilation, codegen, asset work, launches, installs, remote operations, deployment or live configuration ran. No mobile behavior or production renderer inputs changed, so this slice requires neither renderer packaging nor a What's New edit.
