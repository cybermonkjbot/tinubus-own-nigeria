# Unreal room activity read state

October 10, 2026. Source implementation and source/diff review only. Independent acceptance, Windows delivery, compilation, in-game inspection and release remain separate.

Original source checkpoint: `70bb96cb5faede5dc98eec4b9f1a1356f929f0e5`. Original handoff checkpoint: `102e265c5c759055d4464b358b8dfb1df4622f6b`.

Current integrated source/dependency pin: parent correction `b4bc1b7d7dbc9552ad1451062a404fc872a13224`, against `62c1e91854f194828ef096c56d776c6b1020352f`. This follow-up changes only this handoff; the parent owns the source correction and its independent acceptance.

## Assignment and source pins

The room activity section of `unreal-play-resident-read-assignments-2026-10-10.md` at `8e624a9ed5480441e5647247fa3c422a417e125b` owns exactly these new files:

- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionPlayState.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionPlayState.cpp`
- `docs/engineering/unreal-play-state-handoff-2026-10-10.md`

Canonical base: `98de7532f3f983d4cf794d6282b0b9e2f33c1ee4`. All seven canonical files and both HomePresentation files were read and matched that base during original implementation. Earlier source/handoffs, resident source, shared helpers, backend/mobile, configuration and renderer inputs were not edited by the original Play slice. The later parent-owned shared-helper correction is recorded below.

| Canonical source | SHA-256 |
| --- | --- |
| `packages/api/src/companion-play.ts` | `03ae962bb47624b09c673e9dc9e35ab85a5e0b2e09448e3d70a6b9cf9cd3f8e9` |
| `packages/utils/src/companionPlay.ts` | `f002e398719f911596fd8c6308445711006be92e8d23fd827431b5784f984952` |
| `packages/utils/src/companionEmotes.ts` | `32a184e58c20ea6022f66b01d88e831a14511a8a59c90e6cbc949b7f1b633a04` |
| `packages/utils/src/companion.ts` | `8bba23bf9d6e79d10d802fd82a02f320bd46c22f69cd0a930a714ff22bb51475` |
| `apps/backend/convex/companionPlay.ts` | `047ced36383ebaffca0d1be7def24209c9344c57ffc1772d8c52cd1d349a4727` |
| `apps/backend/convex/lib/companionPlayModel.ts` | `af006f9d58574836e5dce6136ce54a71d0023b76dfa771a9682d515e83eb7ecb` |
| `apps/backend/convex/lib/companionModel.ts` | `1c46ba2566d0f806974a0617e685e2020b100d39c3a005a94ff71cc2864ee6d9` |

The API store supplies whole-wire revision ordering and the lifecycle boundaries. Shared Play types and backend validators supply all saved fields and unions. `validPlayMemory` supplies the saved-memory predicate. The backend producer supplies the eight-member and twelve-memory limits and performs authenticated room, call and invitation filtering. Native decoding does not repeat those access decisions.

## Dependencies and appearance preservation

The original implementation consumed private StateJson.h/.cpp at `bc6e288621f97054c06368a9e7434835c7ce0ede`, byte-identical at the canonical base. The original StateJson.h SHA-256 was `fa20d7b05543229ec99caf20904cc68ab0a88b86f910efa777de6f791fc43040`. Parent correction `b4bc1b7d7dbc9552ad1451062a404fc872a13224` adds shared `ReadBoundedArray` to that header and updates PlayState.cpp and FootballState.cpp to use it. StateJson.cpp and the public headers are unchanged.

The current implementation reuses `Parse`, `WithinParseLimits`, `Object`, `Field`, `String`, `Number`, `Boolean`, `Integer`, `Same`, `EnumValue`, `Nullable` and `ReadBoundedArray`. Local code owns domain choice tables, array limits, element readers and atomic publication. Shared `ReadBoundedArray` owns generic array traversal; the former local `Rows` helper is removed. It appends in source order to a fresh caller-owned candidate and can leave partial candidate data on failure. The enclosing decoder still publishes only a complete wire. There is no copied scanner or enum matcher. `ParseNull` is unnecessary because the envelope must be an object; only its required `round` field may be null.

Current dependency hashes at the integrated pin follow. The HomePresentation hashes retain the original inspection evidence; the parent correction does not modify those files.

| File under `apps/companion-unreal/Source/BaddieCompanion` | SHA-256 |
| --- | --- |
| `Private/BaddieCompanionStateJson.h` | `13ceb1b13bee4b0793ddb117e03559bfb8ea77f8bdb8dc81baa1ffa8e70bb618` |
| `Private/BaddieCompanionStateJson.cpp` | `df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce` |
| `Public/BaddieCompanionHomePresentation.h` | `6df35f32c3751bd5a3c60f28ca9552e0de2a475321b945d0613dfbf5bafee8f6` |
| `Private/BaddieCompanionHomePresentation.cpp` | `d109ee38077f6d0745bfc1cc388ec96743607430c09441d7011a6f95355b6c3f` |

HomePresentation's public entry takes a complete `FSnapshotSelection`, including room data, and produces render plans. It has no public standalone saved-appearance decoder suitable for a Play player. This slice does not manufacture a room selection or duplicate the appearance mapper.

`FPlayPlayer::AppearanceJson` retains the entire appearance object through UE JSON serialization, including all supplied fields, unknown fields and absent optional fields. Strings and booleans remain source values. Serialization may change whitespace, escaping and object-key order; the retained value is not a byte-for-byte original substring. No defaults, colour normalization, wardrobe overrides or render recipes are applied.

Appearance validation is deliberately limited to an object and resource bounds. It does not validate even the required `look` member or the optional appearance field types/enums. An empty object or an unknown/null optional appearance value can therefore be retained, but cannot be treated as a valid typed appearance or render readiness. This is the assignment's opaque-preservation fallback, not a second appearance schema. Parent-owned standalone typed-appearance integration remains necessary before rendering. No missing shared StateJson operation blocks this read slice.

## Complete wire and saved branch coverage

`DecodePlayWire` consumes the `Wire` projection of `companionPlay:snapshot`: required nullable `round`, required string `playerId`, and required `memories` array. The result contains either a complete immutable `FPlayWire` or no wire and the fixed diagnostic `Room activity couldn't load.` No diagnostic contains an ID, name or payload value.

`FPlayWire` has a private default constructor and no field setters. Its getters return detached value copies, including appearance strings. No mutable parsed JSON escapes. `Invalid`, `Accepted` and `Retained` dispositions distinguish failed decoding, whole-wire replacement and revision retention. A valid null-round envelope still returns a set wire. Missing fields, root null, wrong root types and malformed JSON return no wire.

Every round retains `id`, `revision`, `seed`, `ownerId`, `startedAt`, `updatedAt`, `expiresAt`, ordered `players` and the saved game. Every player retains `id`, `name`, `characterId`, `appearance`, `progress`, `score`, required-nullable `look` and `vote`, `lastActionId`, `lastActionAt`, and optional `mirrorWatchingAt`. A look retains its `rose`/`ivory` selection and three colour strings. Endpoint colour strings are not restricted to the command palette.

The game is a `std::variant`; its active type preserves the source discriminator without permitting multiple simultaneous branches.

| Saved kind | Retained fields | Additional native array ceiling |
| --- | --- | --- |
| `playground` | `toy`, `hits`, `throws`, ordered `notes`, required-nullable `flight` with `from`, `to`, `power`, `at`, `hit`; optional `lastTouch` with `target`, `at`, `playerId` | 256 notes |
| `mirror` | Ordered `sequence`, `showingAt`, `level`, required-nullable `leader`, required-nullable `lastMove` with `playerId`, `emote`, `at` | 256 sequence entries |
| `mystery` | Ordered `order`, `found`, `entered`, and `unlocked` | 256 per array |
| `style` | `brief`, `dress`/`reveal` phase, required-nullable `revealedAt` | No arrays |
| `kitchen` | Ordered `recipe`, `ingredients`, `chopped`, required-nullable `heatAt`, `cooked`, `plated`, `served`, `rush` | 256 per array |
| `memories` | `anchor`, ordered `moves`, `recording` | 256 moves |

All four saved toy names and all six emotes are decoded exactly. Unknown union selections fail. `mirrorWatchingAt` and `lastTouch` preserve absence and reject explicit null. All required-nullable fields reject omission and preserve null separately from their valid value, including empty nullable strings. Other unknown object fields are ignored by this projection; appearance is the exception and retains its complete object.

Each `PlayMemory` retains `id`, `author`, `anchor`, ordered `moves` and `createdAt`. Its reader follows `validPlayMemory`: ID/author strings at most 100 characters, including empty strings; integer anchor 0 through 5; one through eight known emotes; finite creation time. The API store does not call this predicate in its subscription callback, and the backend validator alone is looser. Applying the existing saved-memory predicate here is explicit native receive validation, not a claim that the client already performs it.

`GetActiveRound` implements only `activePlayRound`. It returns no round when the saved round is null, its players are empty, or its kind is retired `playground`. Otherwise it returns a detached complete round. It does not check expiry, join eligibility, ownership or room admission. Saved playground data remains inspectable through `GetRound`, but this slice never lists it as an available activity or supplies a start command. The backend's `eligibleRound` normally removes retired/packed rounds before returning a snapshot; native saved compatibility does not undo that filtering.

## Bounds and structural differences

The shared preflight bounds the complete input before UE deserialization to 524,288 TCHARs, depth 16 and 16,384 separators outside strings. The future transport still must cap response bytes before allocating the FString. After the globally bounded parse, each serialized appearance object is additionally limited to 32,768 TCHARs, depth 8 and 1,024 separators. The latter is a retained-object limit, not an independent pre-deserialization byte limit.

The decoder preserves the eight-player and twelve-memory producer limits. Every other known string has an additional native maximum of 8,192 TCHARs, with no trimming, case folding, nonempty check or copied command ID bound. Appearance strings use the whole-object limits instead. All known game arrays use the 256-entry ceiling listed above. Empty game arrays are valid; the saved-memory predicate alone requires nonempty moves. Array order and duplicates are preserved, including duplicate players or memory IDs.

Except for the memory predicate's anchor index, endpoint numbers remain finite doubles. Negative, fractional or large finite values are not silently rounded or clamped. There are no new safe-integer, positivity, score, index, timestamp ordering, two-hour lifetime, seed-width, flight-power or inter-field gameplay constraints. Source times remain milliseconds. The backend's `v.number()` shapes and saved types do not establish command-only rules for these read values. Nonfinite JSON numbers fail the shared number reader. Resource bounds and complete structural checking are native receive requirements, not claims of new canonical gameplay validity.

No ownership consistency is invented between playerId, players, ownerId, votes, leader, lastMove or lastTouch. The authenticated backend filters eligible players, may change ownerId and may clear a disallowed mirror leader. The decoder preserves the received result without recomputing those decisions.

## Whole-wire reconciliation and caller ownership

`ReconcilePlayWire` takes an optional prior decoded wire and serialized incoming data. It always runs the full decoder first, including memories and appearance bounds. Malformed older data cannot use revision retention to bypass validation. An invalid response returns no wire; it does not mutate the caller's prior value or substitute a null-round success.

| Valid incoming response | Result |
| --- | --- |
| No prior wire or no prior round | Replace the entire wire |
| Different round ID, regardless of revision | Replace the entire wire |
| Same round ID, equal or newer revision | Replace the entire wire |
| Null round | Replace the entire wire, including playerId and memories |
| Same round ID, strictly older revision | Retain the entire previous wire, including playerId and memories |

There is no per-field merge, historical revision cache or monotonic status rule. An old same-ID callback cannot update just memories or playerId. Equal revisions can replace both. After a null-round response, a later round is compared against no prior round. The function is pure and retains no account state internally.

The future authenticated owner must establish the account, character, room, call and invitation generation before passing a callback to this function. A matching round ID or a supplied previous wire proves no scope authority. No permissive ready/same-scope argument replaces these responsibilities.

The canonical store's surrounding lifecycle remains unimplemented:

- Scope selection changes clear the entire snapshot, reconnect and retire the prior subscription generation. Account/session replacement must likewise discard the old account-owned store and wire.
- Accepted wire replacement sets ready true and clears the shared error. Revision retention leaves readiness and pending error unchanged, but still emits. Native disposition allows the future owner to preserve that distinction.
- Subscription failure clears utilities and replaces the snapshot with empty state, ready true and the received error. Native invalid decoding is not a network failure classifier; the caller owns that error/clearing path and public-error sanitization.
- Last unsubscribe retires callbacks, disables utility activity, clears utility state and busy, but retains the displayed wire. Reconnection retires the previous generation and clears utilities. Disposal clears scope, snapshot, listeners and previous utility observation.

Command replies include `serverTime`; the store updates `serverOffset` from the request midpoint before calling `accept`. That time validation/offset calculation, pending busy/error composition, action IDs, command generations and reply ownership are excluded. Extra envelope fields do not become command receipts merely because the read projection ignores them.

`companionPlay:observeRoom` is an authenticated mutation. It can ensure resident-world setup and observe room conditions. It requires separate utility generations, request identity, timeout/abort, retry timing and observation validation. This slice implements none of it and supplies no power state. Read-only Home/apartment consent cannot invoke either `observeRoom` or `act`.

## Remaining producers and verification

Remaining native ownership includes authenticated subscription dispatch and lifecycle, coherent room/call/invitation selection, typed appearance mapping, player/model bindings, room anchors, native UI and interactions, round commands, emote playback, look application, memory persistence, server-time handling and room utility observation. There are no timers, score evaluation, simulation, mutations, command encoders, retries, mounting or authority-enabling flags here. Kitchen puzzle state is not food, purchased ingredients or care authority.

Current source hashes at `b4bc1b7d7dbc9552ad1451062a404fc872a13224` follow. The public header is unchanged from the original implementation. Original PlayState.cpp SHA-256 was `2f245134528bfc50ee792ddbb8325b70e13ce91984b67d34389b8e9106ab3293`.

| Source file under `apps/companion-unreal/Source/BaddieCompanion` | SHA-256 |
| --- | --- |
| `Public/BaddieCompanionPlayState.h` | `b9eb9bec353858e9175eab5deb3da1535c56b6f7796ded617225ad8cf7c1f3da` |
| `Private/BaddieCompanionPlayState.cpp` | `75add5bd91b5b15619aa8afceb60ec3fb4d48d3e00122cf8e080177e9ff86346` |

Source review covered every field/branch, required-nullable and optional presence, empty arrays, array ceilings, exact enum matching, memory predicate bounds, appearance preservation, whole-wire replacement/retention, validation before retention and detached value ownership. The complete owned source diff was reviewed before checkpointing with hooks disabled. Unrelated changes/staging remain outside this patch.

For this documentation follow-up, the complete three-path correction diff was inspected against `62c1e91854f194828ef096c56d776c6b1020352f`. It removes Play's `Rows` and Football's `Players` generic helpers, adds shared `ReadBoundedArray`, and replaces their eleven Play and two Football call sites. Each call retains its prior limit and element reader. No domain rules, appearance handling, revision reconciliation or publication behavior changed. The three working source files matched `b4bc1b7d7dbc9552ad1451062a404fc872a13224`; current PlayState.cpp and StateJson.h hashes above were read directly. No source was edited during this follow-up.

The parent reported zero findings in the original full Spec review. Standards identified the copied generic array reader and its `Rows` name, which prompted this parent integration. Independent reviews of the correction and later delivery remain pending. This bounded diff inspection does not replace that acceptance or establish runtime verification.

No tests, typechecks, lint, builds, compilation, codegen, imports/conversion, installs, engine/server launches, remote operations or live configuration ran. Parent independent Standards and Spec acceptance precede coordinator-owned Windows delivery. Account access and the world-control interlock remain disabled. These unmounted state files do not change mobile behavior or production renderer inputs, so renderer packaging and What's New updates do not apply.
