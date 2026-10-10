# Unreal Football read-state handoff

October 10, 2026. Source implementation and source/diff inspection only. Independent Standards and Spec acceptance, Windows delivery, compilation, in-game verification and release remain separate.

Source commit: `8fe2ae49cf07604d6ec666dd8ff40bd0631ee046`.

Owned files, relative to the repository root:

- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionFootballState.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionFootballState.cpp`
- `docs/engineering/unreal-football-state-handoff-2026-10-10.md`

The source commit changes only the header and implementation. This handoff is a separate documentation checkpoint. Earlier accepted slices, shared JSON helpers, production renderer inputs and admission/readiness gates are unchanged. The parent owns acceptance and the port tracker; the coordinator owns Windows delivery. This slice completes no gameplay inventory category.

## Canonical dependencies

Canonical pin: `2a97459b581c9da4fe78171cfc2a12dd2afacd7c`. Working bytes matched this pin during source review.

| File | SHA-256 |
| --- | --- |
| `packages/api/src/companion-football.ts` | `7790b96fdf23d66d84af08ac6c8033efe4145f40eaa6c6009f8c0dd99844343a` |
| `packages/utils/src/companionFootball.ts` | `979b865a71525b4602c3e486ad9b6175f14221d26da05953a407b58c0e4a033f` |
| `packages/utils/src/companionFootballEngine.ts` | `5c4852ab1f1547f380ec6f475e049b65186a9d57443c1a97a3c9251c203fdde4` |
| `apps/backend/convex/companionFootball.ts` | `9a073d2e7b447da4a66f974e97377f802fa738e05802ca13f28d5493c4a3d2e8` |
| `apps/backend/convex/lib/companionFootballModel.ts` | `1e78be4330d2d753170ef782cf9dc69f7b6f37b1d787c57feb6e5e76a381ff2f` |
| `apps/backend/convex/lib/chatCallSpaceModel.ts` | `2324f76ed76e8cff95ecb835ff1850474d573dd9c79b889e4955fdd04a1ed4fe` |

The additional `chatCallSpaceModel.ts` reference supplies `SPACE_MEMBER_LIMIT = 50`, which bounds the entry query's participant reads. It does not change the match's 12-player limit or transfer call ownership.

Shared dependency pin: `bc6e288621f97054c06368a9e7434835c7ce0ede`, byte-identical at the canonical pin. Paths below are relative to `apps/companion-unreal/Source/BaddieCompanion/`.

| File | SHA-256 |
| --- | --- |
| `Private/BaddieCompanionStateJson.h` | `fa20d7b05543229ec99caf20904cc68ab0a88b86f910efa777de6f791fc43040` |
| `Private/BaddieCompanionStateJson.cpp` | `df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce` |

Football source outputs at source commit `8fe2ae49cf07604d6ec666dd8ff40bd0631ee046`. Paths use the same source directory above.

| File | SHA-256 |
| --- | --- |
| `Public/BaddieCompanionFootballState.h` | `09143c0fd911dabd6c1362b95c0950fface589ad244ab457a4f043f986100a7a` |
| `Private/BaddieCompanionFootballState.cpp` | `c4fb599b240ab86b0311bc8836165452304f2a8e2d0993ecfbb45e52b9a3303b` |

The implementation reuses shared `Parse`, `Only`, `Field`, `Object`, `String`, finite `Number`, `Boolean`, `Nullable`, `Same` and `EnumValue`. Both endpoint roots require objects, so neither uses `ParseNull`. No shared helper addition is needed.

## Typed reads and bounds

`DecodeFootballSnapshot` decodes the value of `companionFootball:snapshot`. `DecodeFootballEntryRoster` decodes the value of `companionFootball:entryRoster`. Neither accepts an envelope or a nullable root. Both build a fresh candidate, clear output on failure and publish only after all fields decode. They reject missing required fields, unknown object keys, wrong primitive types, unknown case-sensitive enum strings and nonfinite numbers.

The header retains every field in the pinned read types:

- Pitch identity, instance, revision, centre, floor and goal geometry.
- Player identity, side, bot/goalkeeper flags, position, velocity, yaw, readiness, connection, last sequence, action time, goals and assists.
- Ball position, velocity, height, vertical speed, owner, last touch, last pass and claim time.
- Match identity, epoch, revision, mode, phase, players, ball, score, winner, start/end/simulation/restart times, restart side, objective and last event.
- Snapshot player, host, call session, match, ready, busy, error and advancement.
- Advancement owner, lease expiry, nullable next time and server time.
- Entry player, instance, world revision, previous match epoch, nullable phase, server time and ordered participant identity/name/nullable side.

Required nullable fields use `TOptional`. Absence fails; explicit null decodes to unset. `advancement` alone is optional on the wire. Its absence remains unset; explicit null is invalid. A null match does not discard other snapshot fields. A null entry phase with a positive previous epoch is valid. Empty arrays and empty strings remain distinguishable from missing or null values.

Native resource limits are 262,144 input characters, depth 24, 8,192 separators and 4,096 characters per decoded string. Match arrays allow up to 12 players; entry arrays allow up to 50 participants. Arrays retain supplied order and are rejected rather than truncated when over capacity. Entry names are not trimmed or re-sorted locally; the backend already supplies its host-first ordering and names capped at 80 characters.

All response numbers remain finite doubles, including revisions, counters and milliseconds. This follows the broad response `v.number()` validators without rounding or imposing command-input integer constraints. In particular, `actionAt = -1000` is preserved. No local nonnegative-time rule, deadline relationship, score arithmetic, winner reconstruction, player-reference membership rule or mode-specific admission rule is added.

`validFootballPitch` and `createFootballMatch` establish stricter creation rules, including pitch sizes, safe revision, nonempty IDs and roster composition. The existing client has no general receive-side `validFootballState`. This reader therefore applies the response structure and stated native resource bounds, not creation rules. Successful decoding does not establish playable geometry or authorize a command. Coordinates and dimensions remain in source metres; velocity, yaw and time values retain their source units.

## Caller integration

`FFootballReadState` is single-threaded and caller-driven. It contains no transport. The integration owner supplies authenticated query origin, dispatches callbacks on the owner's thread and owns actual query cancellation.

1. Select an account, nonsecret session-incarnation marker and room with `Select`. Changing any component clears the displayed snapshot and invalidates every match and entry receipt. Call `ResetSession` for unconditional replacement, including identical IDs. Reusing the same account ID alone must not preserve callbacks from a replaced session.
2. On the first match listener or explicit reconnect, stop the old transport subscription and call `BeginMatchObservation`. Start `companionFootball:snapshot` only if it succeeds. Use the receipt's room and captured `now` with externally owned authentication. Capture `now` once per connection, never on each delivered value. Its native integer range is the backend's inclusive `0..4102444800000`. An invalid begin retires the old match receipt and retains the displayed data/error.
3. Route a snapshot through `AcceptMatch` with the receipt for that query. Ordinary failures use `AcceptMatchFailure` with public-safe text. Binding `OnChanged` does not count listeners or open a query.
4. For reload, stop the prior match query and call `ReloadMatch`. It clears read error immediately and emits a change while retaining the match, ready state and advancement. It issues a replacement receipt only when match observation was active and the new captured time is valid. With no match observers, it clears error without opening a query.
5. On the last match listener, stop that transport query and call `EndMatchObservation`. This invalidates match callbacks but retains the displayed snapshot and error. Entry observers stay active.
6. Each entry listener gets a separate `BeginEntryObservation` receipt and its own `companionFootball:entryRoster` query with its own captured time. Pass decoded `AcceptEntry` output only to that observer. Stop its transport and call `EndEntryObservation` when it unsubscribes. Starting or ending another observer has no effect on it.
7. Stop all owned transports on account/session/room replacement, `Clear` or `Dispose`. `Clear` resets selection and presentation and emits a change. `Dispose` permanently retires all receipts, clears private state and delegate bindings, and does not emit. The owner cannot stop an external subscription itself.

Receipt generations are opaque GUIDs. Default, wrong-channel, replaced, ended and foreign-owner receipts cannot publish values. Disposed owners cannot be reactivated. No receipt contains credentials or grants server access.

## Match apply ordering

The comparisons run against the current displayed snapshot, in the same order as canonical `apply`:

1. When both matches share an ID, a lower incoming epoch is ignored even if the incoming player changes.
2. Within the same match/epoch/player scope, a lower revision is ignored.
3. Within that same scope and equal revisions, an older advancement `serverNow` is ignored only when both snapshots have advancement. Revision equality also holds when both matches are null.
4. Otherwise the full snapshot replaces the current one. A missing incoming advancement clears a previously present value. A higher revision may carry a lower server time. A new epoch or changed player scope may carry a lower revision, subject to the older-epoch guard.

Scope comparison uses typed match ID/epoch/player components instead of concatenating colon-delimited strings. It preserves the client's null-player/empty-player equivalence for ordering while retaining the original nullable field. It intentionally avoids accidental delimiter collisions between distinct IDs. `bHasApplied` distinguishes the client's initial empty scope from a previously accepted null-match snapshot. Phase changes do not reject reads.

Ignored responses do not clear error or emit. Accepted responses clear read error. The raw decoder preserves wire `busy` and `error`, but the owner does not adopt them. Canonical `apply` preserves command-owned busy and clears error; this read-only owner has no command latch, so its `bBusy` remains false.

Ordinary query failures retain all displayed match fields, readiness and advancement. Malformed responses use the same retained-state behavior with fixed public error text, an explicit native boundary policy. A valid later response can recover according to the same ordering rules. Public failure text is caller-sanitized; a present empty message stays empty, and an absent match message uses the canonical disconnect fallback. Raw exceptions and envelopes are not accepted as error messages.

## Independent entry observers

Entry reads have no shared displayed roster, revision guard, epoch guard or server-time guard. Each current observer receives every structurally valid value, even when counters decrease. The implementation stores only active receipt IDs. `AcceptEntry` returns a fresh typed result; the caller owns any retained roster presentation. An entry error goes only to its observer and never changes the match view or another observer.

The canonical entry API forwards an unknown error to `onError`. This native boundary instead forwards caller-sanitized optional text, with a fixed fallback when absent. Malformed entry values return `InvalidResponse`; stale callbacks clear output/error and return `StaleReceipt`. Neither publishes a partial roster. Callers must branch on the result before forwarding output.

## Deferred integration

Authenticated account/session transport, active-listener counting, query dispatch/cancellation and error sanitization still need a consumer. The backend remains the authority for admitted players, host status, call/session membership, pitch/world readiness, expiry, health, combat and contact restrictions. Returned host, call ID, player flags and advancement ownership are read data, not local authority.

Canonical `apply` is also used by command, pulse and open responses. Those paths are absent here. Future integration must compose command busy/error state and reset input on match/epoch/player or phase changes. It must also restore sequence acknowledgement, pending-open replay identity, stop/release behavior and observation teardown as one lifecycle. A read-only receipt is not permission to route arbitrary mutation results through the query owner.

Only canonical pulse replies update the client's server-clock offset. This slice preserves supplied advancement without calculating `nextAt`, advancing time, deciding driver eligibility or scheduling anything. No engine loop, NPC roster/input generation, ball physics, movement, AI, awards, match opening, commands, input/stop/pulse pump, retries, timers, call joins, rendering or world registration is implemented. Later movement/physics/navigation integration should use Unreal facilities under a separate assignment.

## Inspection record and completion limits

Source review covered every typed field against the pinned type and backend validators, nullable/absent handling, finite-number behavior, resource bounds, all three ordered match guards, retained-state failure/reload behavior, independent entry generations, same-ID session replacement and disposal. The exact two-file staged diff was inspected before the source commit. Canonical and shared dependency diffs were empty; hashes above identify the reviewed bytes.

Boundary cases traced in source included a lower epoch with a changed player, lower revision with a changed player, higher revision with older server time, equal revisions with older server time, both-null matches, missing advancement after retained advancement, initial negative action time, expired entry phase with retained epoch, match unsubscribe while entry observation continues and one entry unsubscribe while another continues. These were source walkthroughs, not executed tests.

No tests, typechecks, lint, compilation, builds, code generation, imports, conversion, engine/server launches, installs, remote writes or live configuration changes ran. Hooks were disabled for the exact-path checkpoint. Unrelated staged and unstaged work was preserved. No mobile behavior or production renderer input changed, so this patch has no What's New or renderer-packaging update. Unreal compile/runtime compatibility and rendered behavior remain unverified. Independent acceptance, content delivery and release are pending their assigned owners.
