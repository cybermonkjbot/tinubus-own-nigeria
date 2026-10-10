# Unreal life read-state handoff

Date: October 10, 2026.

Source commit: `6fd4af7afea28944880aa1949f5ed15f4cc5ea49`.

The typed life read slice is implemented and manually source-reviewed. It is not a complete LifeStore, an admitted native read connection or a care simulator. No tests, compilation, builds, engine operations or Windows delivery ran. Gates remain disabled.

## Owned paths and pins

The source commit adds only:

- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionLifeState.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionLifeState.cpp`

This handoff is the third owned path, `docs/engineering/unreal-life-state-handoff-2026-10-10.md`, committed separately to record the immutable source commit. Existing food/kitchen source and its accepted handoff remain unchanged, confirmed by a scoped diff against `5eb98f31957fab46277243b556d2fb71a4d91c1c`. Other state owners, shared helpers, controllers, backend/mobile/renderer code and the parent tracker were not edited.

Canonical TypeScript references are pinned to `b0ea171004b0a510c39c5f8779bc6938d538edbd`. Working bytes matched that pin before and after implementation. The eight assigned files were traced, along with the actual endpoint life validator in `apps/backend/convex/lib/companionModel.ts`, which also matches that pin.

The final StateJson dependency is `bc6e288621f97054c06368a9e7434835c7ce0ede`, following the parent's explicit dependency update. It adds `EnumValue` to the accepted `691a60a98ac276d3c9a59793965b5b1bb47a69d1` header. The `.cpp`, `ParseNull` and prior operations are unchanged. LifeState uses the shared `EnumValue` for activity kinds, activity phases and task statuses; domain choices and validation stay local. The final helper diff against `bc6e2886` is empty. Do not deliver this consumer with a header missing `EnumValue` or a `.cpp` missing `ParseNull`. The earlier Windows batch remains separately pinned by the parent.

No parser, runtime or schema-independent enum traversal was added. LifeState uses native strings, arrays, optionals, shared pointers and delegates, plus the existing private StateJson reader. Its small domain time predicate intentionally uses the shared finite-number reader rather than `ValidTime`, because the life validator does not impose a safe-integer maximum on timestamps.

## Typed contract and validator boundaries

The decoder reads the direct `companionLife:snapshot` result. It applies the backend's object/union shapes and the client-side `validCompanionLife`, `validCompanionTaskQueue` and `validCompanionSkills` bounds.

| Contract | Implemented boundary |
| --- | --- |
| Life | Character ID length 1 to 100, required needs, finite nonnegative updated time, safe integer revision 0 to 9,007,199,254,740,991, required nullable activity, integer meals 0 to 3 and required boolean reminders |
| Needs | Required hunger, energy, cleanliness and enjoyment, each finite and within 0 to 100; fractions preserved |
| Activity | ID, room ID and station ID lengths 1 to 100; exact kind; finite start/end, start at least zero, end strictly after start, duration at most 120,000 ms |
| Optional activity fields | Phase is approaching or active; requested time is finite, nonnegative and no later than start; room key length 0 to 5,000, including present-empty |
| Task queue | Optional non-null object; required task array with at most eight entries, boolean paused and required nullable reason |
| Task | ID length 1 to 100, room/station lengths 1 to 80, exact activity kind and waiting/active/blocked status, required nullable activity ID and reason |
| Queue identity/order | Exact case-sensitive unique IDs; preserve input order; active task only at index zero with a nonempty activity ID; other statuses require null activity ID |
| Reasons | Null or nonempty text of at most 240 characters, for both queue and task |
| Task cooking | Optional non-null object, permitted only for cook tasks; recipe key length 1 to 80 and batches exactly 1 or 2; cooking is not required merely because kind is cook |
| Skills | Optional non-null object; cooking required when present; seven other skills independently optional; every present progress requires a safe integer experience within 0 to 300 |

All 11 activity kinds are retained: `sleep`, `sit`, `play`, `cook`, `eat`, `bathe`, `work`, `watch-tv`, `read`, `stretch`, `wash`.

All eight skills are retained: `cooking`, `fighting`, `selfDefence`, `charisma`, `fitness`, `hustle`, `dance`, `flirting`. A received skills object with only cooking remains different from a new all-zero skills object. No level, practice, reward or duration calculation is inferred from experience.

Strings are not trimmed or case-folded. Whitespace-only strings within the canonical length bounds are retained. Unknown object keys and non-string enum values are rejected, matching the endpoint validators. The utility validators alone ignore extra keys, and the task utility uses `String(status)` in one membership check; neither permits widening the backend's exact returned union.

Missing optional activity fields, cooking details, taskQueue, skills and individual non-cooking skill progress remain missing. Explicit null is invalid for those optional fields. Required nullable fields reject omission. Native unset `TOptional` represents their successfully decoded null, not an omitted required field.

No additional consistency rules were added between a queued active task and `life.activity`, between an activity and `updatedAt`, or between queue pause/reason and status. The canonical validators do not impose them. The decoder neither repairs nor simulates these relationships.

Times remain doubles. Fractional and finite nonnegative values above `MAX_SAFE_INTEGER` remain valid where the life validator permits them. Revisions and experience still use the shared safe-integer reader. The duration comparison uses the supplied finite start/end values; it does not clamp or replace an expired activity.

Allocation preflight limits each input to 262,144 characters, nesting depth 32 and 8,192 structural separators. These are parser resource bounds, not new gameplay rules. JSON arrays/strings/primitives other than the literal null cannot substitute for a life object. Empty, missing, malformed or envelope-shaped input fails atomically, clears decoder output and returns the canonical fixed care-data error.

## Captured default for authoritative null

`BeginSubscription` receives the connect time explicitly. It rejects non-finite or negative time, an empty account identity, or a character ID outside length 1 to 100. It retires the previous observation even on failure, returns an invalid receipt and preserves presentation. Selection itself stores exact caller identifiers and is not admission.

A successful begin captures one immutable default for that observation, equivalent to `newCompanionLife(characterId, connectedAt)`:

| Field | Captured value |
| --- | --- |
| Character | Selected character ID |
| Needs | Hunger 20, energy 85, cleanliness 90, enjoyment 80 |
| Updated time | Validated caller-supplied connect time |
| Revision and meals | Zero |
| Activity | Null |
| Reminders | False |
| Task queue | Missing |
| Skills | All eight present, each experience zero, matching `newCompanionSkills` |

The default is not published at connect. `DecodeLifeResponse` uses the shared `ParseNull` to distinguish a syntactically valid null from missing/malformed data. Only a current, admitted query callback carrying that decoded null may use the captured default. A valid received object never receives default fields. Repeated null callbacks in the same observation reuse the same captured timestamp and values. Reconnecting captures a new default, but does not publish it before a reply.

The wire contract is `life | null`. Unlike JavaScript's generic nullish operator, missing/undefined input is not an authoritative null in this native boundary. The caller must establish query provenance; parsing the text `null` does not establish account permission.

`ELifeReadOrigin::DefaultForNull` distinguishes the local default from `SavedValue`. Both may be displayed, but the default is not a saved record, a new life write, a grant, a reminder opt-in or permission to start an activity. `Unavailable` accompanies a cleared life.

## Selection, receipts and revision ordering

`FLifeReadState` is single-threaded and caller-driven. Account identity is a non-secret partition key representing the canonical store's fixed account owner. Changing account is equivalent to retiring that account's old store; changing character follows the canonical selection reset.

1. `Select` compares account and character case-sensitively. Equal selection changes nothing. Changed selection retires callbacks, clears life/origin/error and notifies listeners. Empty selection suspends observation. It never begins a query.
2. `BeginSubscription` retires prior callbacks, captures scope/default and returns an opaque receipt. Existing presentation/error is retained. The adapter must stop its old subscription and start the replacement for `companionLife:snapshot`; adding a delegate does not perform either operation.
3. Every callback first checks disposal, current subscription, generation and full account/character scope. A stale callback is ignored before parsing. A default receipt, receipt from another owner, or receipt from before a reconnect cannot publish data.
4. A malformed current callback clears life, sets origin unavailable, publishes the fixed care-data error and returns `InvalidResponse`. This matches the canonical invalid-data path; it does not retain the old life or install the fresh default.
5. A valid object for another character is ignored. A lower revision than the currently displayed life is ignored. Neither case clears errors or notifies listeners. Equal and higher revisions replace the entire life and publish once; updated time does not supply a separate monotonic gate.
6. A decoded null selects the captured default, then follows the same character/revision comparison. Its revision zero cannot replace a displayed revision above zero. It can replace revision zero, including a revision-zero saved value.
7. `AcceptFailure` for a current receipt clears life and origin, installs the supplied public error text and notifies listeners. The transport owns error safety; `FLifePublicReadFailure` is not a sanitizer. The receipt remains current so a later valid callback can recover. Empty public messages remain empty rather than being silently rewritten.
8. `EndSubscription` invalidates callbacks and retains life/origin/error. The adapter calls it on last-observer removal before invoking its unsubscribe callback. `Clear` invalidates and clears selection/presentation. `Dispose` does the same and clears delegates; subsequent reads cannot publish.

The revision comparison uses only the current displayed life, defaulting to zero when none exists. It is not a lifetime maximum. After an error clears a revision-10 life, a later valid revision zero or authoritative null can recover. Last-observer removal and reconnect retain the current life, so they do not reset that comparison point. This distinction follows `snapshot.life?.revision ?? 0` in the pinned store.

The adapter must clear on session replacement or access loss, even when the same account ID is reused. It must also release retained view/receipt references during private-state teardown. Neither the scope strings nor the receipt grant access. Native live-read admission, callback lifetime and an authoritative account/character source are still absent.

## Command and simulation boundary

The canonical store keeps its displayed error on successful subscription callbacks while any `pendingAct` exists. This read owner has no pending-act state and clears its read error on accepted success. A future composed LifeStore must preserve the currently displayed error while a pending act exists, including a prior read error, and must not interpret read success as command completion or resolution of an ambiguous request. Commands must also participate in comparison against the composed current life when integrated.

No `act`, `report`, task dispatch, mutation receipt/retry ownership, busy/retryable flags, reminder scheduling, care drift, activity completion, task settlement, meal conversion, rewards or skill awards were implemented. Saved needs, activities, meals, reminders and experience stay as received. There is no station generation, navigation/arrival inference, interaction, IK, UI or pawn binding.

The backend snapshot reads the account-owned saved life or null after authentication and library membership checks; it does not project care in that query. Native subscription admission and a credential-safe producer for that result remain separate work. Existing Home or entry consent is not permission to call the account endpoint. StateJson decoding cannot supply that missing producer.

## Source review and remaining integration

Manual review covered every declared field, all activity/skill/status cases, missing/null boundaries, text/numeric limits, queue uniqueness/order/active rules, atomic failure, same-character revision ordering, captured defaults, account changes, reconnects, malformed reads and error recovery. Scoped diff whitespace inspection passed. This is source evidence only; no executable validation or Unreal compatibility result is claimed.

The new code requires the matching LifeState pair and StateJson at `bc6e2886`. The parent owns independent review and integration. The coordinator owns any Windows transfer. A separately authorized adapter, command owner and runtime/compile/test pass remain outstanding. No mobile or production renderer changes were made, so there is no renderer packaging or What's New update in this slice.

## Immutable hashes

SHA-256 of the source pair at `6fd4af7afea28944880aa1949f5ed15f4cc5ea49`:

```text
3639a71fde0aa52b2f03bf0ed4fd992cf381cdf27b31600b3ae650282e1ea350  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionLifeState.h
45e1e8d8b61a9ef5d4a7a5a5728e42fa964421c648a2bb277121cf27a97a784e  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionLifeState.cpp
```

SHA-256 of the unchanged native helper dependency at `bc6e288621f97054c06368a9e7434835c7ce0ede`:

```text
fa20d7b05543229ec99caf20904cc68ab0a88b86f910efa777de6f791fc43040  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.h
df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.cpp
```

SHA-256 of canonical files at `b0ea171004b0a510c39c5f8779bc6938d538edbd`. Hashes cover whole files; only the read contracts and behavior described above were translated.

```text
b87d63e40ab06932e0322268217f5d4b202083fcd475c9ae39e13e497d2ae90e  packages/api/src/companion-life.ts
b628e8415be3f064096b05e6878e1176f238635bd641f965582e68fa9e66ecf9  packages/utils/src/companionLife.ts
878b249b0ef1629d78e78c6fc2856205930030dd3ba3cb161c3225afe6ff1619  packages/utils/src/companionTasks.ts
b87c1927a96fb1806d63ca3fd3ada1bf9686659ba2d8ec2255d4737f2df61066  packages/utils/src/companionSkills.ts
80f615c7cd627fafe60588ea93f7c725bda581ae3944737e8127db848ee2c95a  packages/utils/src/companionActivityKinds.ts
67c4d9d38b6bffe7e50e141bd53cf9915c100635afdc012481d0e862cdc124e7  apps/backend/convex/companionLife.ts
70713e76678fb90cfa645e5beba8076a4620b2cd0f51e0d1a632fe7a5511084f  apps/backend/convex/lib/companionTasksModel.ts
394ff44ed11283e02b977666570f154a8e309555eb91b2027f750d4bb2511a83  apps/backend/convex/lib/companionSkillModel.ts
1c46ba2566d0f806974a0617e685e2020b100d39c3a005a94ff71cc2864ee6d9  apps/backend/convex/lib/companionModel.ts
```
