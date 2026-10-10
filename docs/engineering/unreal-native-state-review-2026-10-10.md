# Native gameplay state source review

October 10, 2026. Review covers `2ba875d2380a109d1c79f3db476690b6977f20e5...1ed6ec04cb77abada8ec39ed5c08cfb7cfac7fe3`, scoped to CombatState, RecoveryState and MissionState pairs and their two handoffs. It includes initial files captured in `95654a7fb`, mission completion `2ea422694` and combat/recovery completion `1ed6ec04c`. Concurrent changes are excluded. The pinned handoffs and canonical stores supplied the requirements; no issue-tracker document was available.

## Standards

Reviewer `01a12729-3705-7820-a0ce-bdb30396777d` found one documented duplication violation, counted once across both slices. CombatState.cpp:41-60, RecoveryState.cpp:32-51 and MissionState.cpp:38-58 repeat the JSON string/escape/depth/separator scan with different budgets. Combat and recovery also repeat object-key, string, number and nullable readers. This conflicts with AGENTS.md's instruction to avoid duplicated implementations. Share identical private readers while retaining each contract's budgets and acceptance policies.

Mission has two optional maintenance judgments. MissionState.cpp:192 expresses missing/null field policies as positional booleans. MissionState.h:45-47 holds separate X/Y/Z scalars for a point already grouped by the canonical contract. Named field policies and an existing point type could clarify these interfaces. Neither is a demonstrated behavior failure; both are deferred outside the correction.

No other documented violation was found. Recovery's unconditional subscription replacement and revision-filtered mutation acceptance match its canonical store.

## Spec

Reviewer `01a12729-377b-7c50-a9e1-bd7de4be300d` found no confirmed combat/recovery mismatch. Complete public fields, absent/null distinctions, immutable values, failed-output clearing, separate input receipt scope and canonical life/match transitions were accepted. Recovery subscriptions correctly accept lower revisions; mutation acceptance and failure handling retain the canonical cross-life rules.

Mission has one P3 finding. Its handoff promises that invalid caller time produces a typed error, but MissionState.cpp:379 clears the receipt and returns false for invalid `BeganAtMs` without setting `InvalidReceiptTime`. A selected idle owner given `-1` or NaN can retain `Error == None`, indistinguishable from ordinary capture refusal. Expose the typed error while retaining wire data, readiness and busy state.

No other mission mismatch was confirmed in public fields, nullability, malformed-result retention, request/generation ordering, busy/error transitions or reward-key casing.

## Correction ownership

The parent inspected the repeated readers and mission start method and confirmed the findings. Both initial reviewers are closed. Combat/recovery chat `01a12714-f810-7ac1-8171-bc6d7c8c20d6` owned the three State.cpp files, new private StateJson helpers and two handoffs for the correction below. Public interfaces, other state workers, backend, transport, controllers and runtime gates stayed outside its scope. The original mission chat stayed idle during that correction, avoiding overlapping edits.

The initial review held delivery for correction. Its original totals were one documented Standards duplication violation and two optional maintenance judgments, plus one mission Spec P3 error-reporting defect. The follow-up below resolves both actionable findings. Transport, content, admission and pawn/UI integration remain unfinished separately.

## Shared-reader and mission correction

Correction `ab535c29cedd4e893a62539f247a03488495b3e9`, compared with parent `7993021ad8ac154eac5e27ea0f8bb5de8cb26196`, changes the three private state consumers, adds private StateJson.h/.cpp and updates the two handoffs. Public headers remain unchanged. Review covered all seven paths at that fixed checkpoint.

### Standards

Reviewer `01a12735-7f18-7c83-8d84-1f2141adbcf5` accepted the correction with no new findings. Shared parsing and scalar/nullable readers retain explicit per-domain character, depth and separator budgets. Domain-specific field policy remains with each consumer. The original duplication violation is resolved; the two optional mission interface judgments remain deferred.

### Spec

Reviewer `01a12735-7f83-7b73-babf-56eec6f69869` accepted the correction with no new findings. An active selected idle mission owner now reports `InvalidReceiptTime` for an invalid mutation start, clears the output receipt and preserves wire data, readiness and busy state. Disposed, busy and unselected refusals remain no-ops. Parsing, null handling and response ordering remain unchanged.

Both reviewers are closed. This correction is accepted as source. No tests, compilation, engine operation or runtime acceptance ran. The later array-root extraction at `2dd54bfe57574b9e281ee4846fa10d51bb0ce9e2` changes the shared dependency and has its own review below; delivery must use one matching reviewed helper with every consumer.

## Policing, haul and social/teams integration

The complete thirteen-path review is pinned to `385ceacce8fa27c091a7ef89bdf3eb523704dad0...2dd54bfe57574b9e281ee4846fa10d51bb0ce9e2`. It includes the PolicingState, HaulState, SocialState and TeamsState pairs, private StateJson pair and three domain handoffs. Initial implementation and subsequent helper integration are both included, not only the last extraction diff.

Policing integration is `1e49276bbd4b45308e30e6ed43fe72495997c138`; haul integration is `6a5acbe7b5e5f7021d403d559ddc6ae279ade09d`; social/teams integration is `e37deb93fd35d897d0bfd12364f5660d2246f061`. Parent `2dd54bfe5` extracts `WithinParseLimits` for both object parsing and the existing social-array UE value parser. The preflight checks size and structure budgets, not JSON syntax or admission.

### Standards

Reviewer `01a12739-5d9d-7283-9168-90f0f7fcb2e0` found no documented breaches or optional findings in policing or haul. Both keep bounded shared reads, fresh nullable candidates, private receipts and domain-owned acceptance policy. Argument preparation creates no transport or settlement authority.

Social/teams/shared preflight has no documented breach and one optional naming judgment. `GetSelectedPlace()` and `SelectedPlace` hold a call-session ID, while encounters separately contain an actual place. An explicit call-session name would reduce ambiguity. This is not a demonstrated behavior failure and is deferred outside this dependency extraction. The parent extraction preserves the original scanner predicates, object serializer and social-array input. Social/teams limits remain 262,144 characters, depth 8 and 4,096 separators.

The Standards reviewer is closed. Totals are zero documented breaches and one optional naming judgment.

### Spec

Reviewer `01a12739-5e40-70c1-b09b-e5e0b3ccf51d` completed the full fixed scope and is closed. Canonical source hashes match the handoffs.

Policing has zero Spec findings. Public-field presence, exact demand confirmation, command guards, busy/error ordering and callback retirement match the contract.

Haul has zero Spec findings. Reward variants and provenance, null/empty distinctions, concurrent one-use receipts, cursor ownership, balance independence and generation/failure behavior match the contract.

Social/teams/shared preflight has zero confirmed Spec findings from that review, with one unresolved engine compatibility question. Selection, independent player watches, retained busy state, explicit expiry, joined-invitation retention, public-city fields and immutable reads match the contracts. The parent preflight extraction preserves the scanner, original input, serializer overloads and object consumers. The reviewer could not establish scalar-null acceptance from API signatures alone, so it withheld an unqualified pass for the nullable conversation-context and call-entry decoders.

## Installed-engine scalar-null finding and correction

The parent resolved the open question through bounded read-only SSH inspection on October 10. Installed Build.version identifies UE 5.8.3, changelist 58210709. Paths below are relative to `C:\Program Files\Epic Games\UE_5.8\Engine`; hashes identify the inspected bytes.

| Source | SHA-256 | Relevant implementation |
| --- | --- | --- |
| `Source/Runtime/Json/Public/Serialization/JsonReader.h` | `e31b913421aee4c06a8e64196e6cd538a3982f6553786fc8b93e1a9e2c20dd4c` | `ReadStart`, lines 307-327, rejects a first token other than an opening object or array. |
| `Source/Runtime/Json/Public/Serialization/JsonSerializer.h` | `1e9752513d3c0056f91b2a24646cedd9c3f5401a3ece7388feccba93b691bd5d` | Value and array overloads, lines 300-334, share stack decoding. Lines 539-549 create array state, 587-605 append nested null, and 609-614 require a valid container state. |
| `Source/Runtime/Json/Private/Serialization/JsonSerializer.cpp` | `d1b7669f204c67cf2856ef524ac2fbd24e1a96a1081a31c4bc1811897171ce0f` | Value policy, lines 5-28, accepts object/array roots only. Array output, lines 31-41, preserves the array. `ReadNull`, lines 87-90, creates `FJsonValueNull`. |

This confirms a source defect in the original SocialState and TeamsState raw-null branches, not a regression from shared preflight extraction. Their nullable endpoints returned a decoder failure for a legitimate null result. The other reviewed modules use separate raw-null handling and are not changed by this correction.

The parent adds private `ParseNull`, which checks the original budget and exact lowercase literal, encloses the unchanged input in a single array, and uses UE's existing array deserializer. It requires exactly one valid null value before assigning output. Only the two existing nullable branches call it; object and offers-array parsing remain unchanged. No domain field, public header, response ordering, engine implementation or gate changes.

Policing and haul are accepted as source at their full review pins. Original full-review totals are zero documented Standards breaches, one optional naming judgment and zero confirmed Spec findings; the parent's subsequent installed-source inspection confirmed the scalar-null defect and produced the correction above.

## Scalar-null correction acceptance

Both independent axes reviewed the six-path correction `d9d67d360953aa42b69db558693521288fed614b...691a60a98ac276d3c9a59793965b5b1bb47a69d1`. Concurrent food, jobs and dialogue changes were excluded.

### Standards

Reviewer `01a12739-5d9d-7283-9168-90f0f7fcb2e0` found no hard breaches or new optional judgments. The helper checks the original bounds before adding its two-character wrapper, reuses UE's array deserializer and assigns only one validated null value. Both nullable consumers retain fresh candidates and failure clearing. The operation stays private; domain policies, public headers and the other five state consumers remain unchanged. The original optional naming judgment remains deferred.

### Spec

Reviewer `01a12739-5e40-70c1-b09b-e5e0b3ccf51d` found zero findings and accepted the previously held social/teams source with the matching `691a60a98` helper. Nullable conversation context and call entry now publish explicit Null with fresh default payloads. The maximum wrapped input is 262,146 TCHARs. UE validates the unchanged surrounding whitespace; empty, quoted, other scalar, extra-value and malformed input remain rejected. Non-nullable consumers still reject null. Object/offers parsing, nested presence and state ordering are unchanged.

Both reviewers are closed. Totals for this correction are zero Standards findings and zero Spec findings. The complete policing, haul and social/teams source batch is accepted with StateJson from `691a60a98`, alongside the earlier accepted combat/recovery/mission source. Windows delivery, compilation and runtime acceptance remain separate. No tests, compilation, engine launches or executable validation ran.

## Food and kitchen read-state acceptance

The five-path review covers `12afce03a1b3c305bd2f56a974e3e9b6b93ed1db...5eb98f31957fab46277243b556d2fb71a4d91c1c`, containing the complete FoodState/KitchenState pairs and their handoff. Source is `d9d67d360953aa42b69db558693521288fed614b`. Both reviewers matched the canonical hashes and original dependencies. Concurrent helper, jobs and dialogue changes were excluded. The additive `691a60a98` helper leaves the operations used here unchanged; a later batch must pin the complete matching dependency.

### Standards

Reviewer `01a12753-7dc1-79c1-8b58-cf0db282728d` found no documented breach and one optional P3 duplication judgment. FoodState.cpp lines 32-42 and KitchenState.cpp lines 21-31 contain the same bounded-array traversal. A shared element-reader loop could reduce maintenance, but distinct nullability, retention and disposal policies must remain separate. This optional extraction is deferred; no blocking defect was found. Both owners retain bounded decoding, complete-candidate publication, private scope/generation receipts and stale-callback rejection. Notifications follow state updates with no subsequent owner writes.

### Spec

Reviewer `01a12753-7e53-73e2-b73d-6933be175683` found zero missing, partial, unrequested or incorrectly implemented scoped read requirements. Complete fields, unions and required/optional/null distinctions match the pinned projections. Food endpoint truthiness, scope equality, reload/end/failure retention and moving-offer ordering match the canonical store. Kitchen retains observed scope after read failure and presentation on disposal; account teardown must clear it first. Guest quantities and recipe availability remain server projections. Command ownership and uncertain-command error composition are explicitly deferred, not silently translated.

Both reviewers are closed. Totals are zero documented Standards breaches, one optional P3 judgment and zero Spec findings. Source is accepted for a later guarded handoff. No Windows delivery, native adapter/UI binding, executable validation, compilation or runtime acceptance is established by this review.

## Jobs and NPC dialogue review

The six-path review covers `12afce03a1b3c305bd2f56a974e3e9b6b93ed1db...bec5a28fdae1976ed1c7fb469f765b30c8458ea8`, including both complete source pairs and their handoffs. Both reviewers matched the source, canonical and dependency hashes. Concurrent work was excluded.

### Standards

Reviewer `01a12758-4d3a-71e3-9848-aaa2afd6a4fb` found one shared documented duplication breach. JobsState.cpp line 43 and NpcDialogueState.cpp line 62 each implement the same schema-independent enum reader: read a string, search case-sensitively and assign its matching choice. AGENTS.md requires avoiding duplicated implementations. Choice tables remain domain-owned; the common reader can belong to the existing private helper. The parent confirmed the duplicate.

Two optional judgments remain deferred. JobsState's `ReadGeneration` and `ReadActive` repeat dispatch over parallel slot fields. NpcDialogueState's `ReadTarget` repeats profile-key dispatch for NPC ID and authored name. Neither is a demonstrated behavior defect. No additional documented breach or permissive producer fallback was found.

### Spec

Reviewer `01a12758-4db5-78e2-b3fc-22964f447749` found zero actionable findings in either slice. Jobs preserves fields, reward projection, goal independence, lifecycle, query ordering and stale/in-flight responses within its read-only scope. NPC dialogue preserves targets/topics/replies, scope, local intents and completion receipts. Missing target/resident evidence fails closed. Command scheduling, retries, goal writes, target production and network integration remain explicitly deferred.

Original totals are one shared documented Standards breach, two optional judgments and zero Spec findings. Source delivery is held for the bounded enum-reader correction, not for a functional mismatch.

The parent added private `StateJsonPrivate::EnumValue` at `bc6e288621f97054c06368a9e7434835c7ce0ede`. It preserves the duplicate loops' string read, exact comparison, choice order and assignment-on-match behavior. Only StateJson.h changes; its existing operations and StateJson.cpp remain unchanged from accepted `691a60a98`. Jobs and NPC owners now own their respective private consumer/handoff corrections, leaving public headers and domain tables unchanged. LifeState's active owner was directed to use the same helper in its new files. Re-review follows a complete pinned integration. The existing Windows batch remains fixed to `691a60a98` and is unaffected.

Both consumer corrections are committed. Jobs source is `0005d007a2edb413054a2e2b1007d45ac14944d5`, with handoff `c911f7a1ff1d0e18ef85f353fe1ab14ef7754a1e`; NPC source/handoff is `a099824b34b7d9d8ffa4a010a8df1618474d0713`. Parent diff inspection found only the shared template, removal of the two duplicates, replacement of six Jobs and three NPC calls, and matching dependency records. The original public headers, tables, validation and lifecycle remain unchanged. The same two independent reviewers accepted the five-path correction `77b680f4f20fad320d76fd20d3165b59b39d2bc0...c911f7a1ff1d0e18ef85f353fe1ab14ef7754a1e`, excluding LifeState and parent documentation.

### Enum correction Standards acceptance

Reviewer `01a12758-4d3a-71e3-9848-aaa2afd6a4fb` confirmed the documented duplication breach is resolved with no new breach or smell finding. Domain choice tables remain local, exact reads and first-match assignment are unchanged, and handoff hashes match. The two original read-slot/profile organization judgments remain optional and deferred. There are zero outstanding documented breaches.

### Enum correction Spec acceptance

Reviewer `01a12758-4db5-78e2-b3fc-22964f447749` found zero actionable findings. Six Jobs and three NPC replacements preserve field destinations, choice/evaluation order, nullability, failure clearing, publication and lifecycle. The helper preserves non-string/unknown rejection without output assignment. StateJson.cpp, existing operations, public headers and previous consumers are unchanged. Both handoffs correctly require `bc6e28862`.

Both reviewers are closed. Jobs and NPC source are accepted with the matching helper. Original full Spec results remain zero findings per slice; correction results are zero new Standards findings and zero Spec findings. These consumers and the newer helper still require their own Windows handoff and runtime acceptance.

## First native-state Windows delivery

The accepted seven state pairs and StateJson from `691a60a98`, with six records from `b0ea171004b0a510c39c5f8779bc6938d538edbd`, reached Windows `535b7a190eaaef0568fc6f1708016957fad390b2`. The coordinator's exact 22-file receipt and independent readback preserve seven dependencies and 174 protected paths. At `2026-10-10T19:57:38.2302900Z`, a separate parent SSH read matched all 22 working-file hashes and committed blobs plus the seven dependencies, with stable Windows HEAD. Evidence is `output/cribtest11-native-state-2026-10-10/parent-readback.json`; the coordinator's adjacent record owns the protection audit. Jobs, dialogue, food/kitchen and the later EnumValue helper were not delivered. No new compilation, imported content, native integration, in-game acceptance or release is implied.
