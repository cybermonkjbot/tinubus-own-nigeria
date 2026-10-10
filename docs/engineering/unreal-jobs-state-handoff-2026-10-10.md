# Unreal courier jobs read state

October 10, 2026. Source implementation and source/diff review only. Native integration, compilation and in-game acceptance remain unverified.

Initial source checkpoint: `b6076e8c66dfc9e90e4e0d5107fdca974a7cf817`. Shared enum integration checkpoint: `0005d007a2edb413054a2e2b1007d45ac14944d5`.

## Changed paths and scope

- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionJobsState.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionJobsState.cpp`
- `docs/engineering/unreal-jobs-state-handoff-2026-10-10.md`

This is an account-private, caller-driven read reducer. It decodes jobs, offers, saved courier goals and externally obtained command responses. It owns local response receipts, not commands. It contains no credentials, network client, request encoding, retries, goal writes, timers, local settlement or simulation. It does not bind a controller, HUD, renderer or actor. Existing admission gates stay unchanged and disabled. MissionState and its handoff are unchanged.

The initial source checkpoint contains only the two JobsState source files. The enum integration changes only the private JobsState.cpp; the public header is unchanged. This record is checkpointed separately. Unrelated shared-checkout work is excluded. Production renderer packaging and What's New changes do not apply to these unmounted standalone files.

## Canonical sources

The following file hashes identify the inspected contracts. All matched the initial source checkpoint's repository versions. The enum integration changes no canonical contract.

| Source | SHA-256 |
| --- | --- |
| `packages/api/src/companion-jobs.ts` | `a0f2c51dd102cd953291f5979a12228d4a1a2225c4004b7db439815c41c80c47` |
| `packages/utils/src/companionJobs.ts` | `9fb5f4f6ba25202b133a0dccf0452e39c2a920a10f057233c7470b69341cc408` |
| `apps/backend/convex/companionJobs.ts` | `dde5d33ec3a7edc048093db3c756182034db0c7cac468fc11a6b59faf2771608` |
| `apps/backend/convex/companionWorld.ts` | `9f8ef66bf02c445ab688ea7410bfb6410b5c7259bc39ac7d692d0665e2ab9798` |
| `apps/backend/convex/lib/companionJobsModel.ts` | `dbef808f04752fabe16b5589c2da9b0bf1a9045f187e3f7232d9f72d8617d8b1` |
| `apps/backend/convex/lib/companionCourierProgress.ts` | `3898d5f51fbd1e9b653e0a7366622c6dd91ce491014c8e78ff720d539802eac9` |
| `apps/backend/convex/lib/companionHaulModel.ts` | `df75cdae2ae1928aa5d2c79d7d776af3a936b1474c35c6cae290f5f58d2e8a3f` |
| `packages/api/src/convex-error.ts` | `33957fe910e8e00718cb2cba95189abc231c0a9822cf2acb6283ee6a2a2bc992` |

The canonical store supplies lifecycle, context/contact selection, source-error precedence and response ordering. The shared jobs types supply complete public fields, enum cases and limits. The backend model supplies immutable terms and the courier-specific ledger projection. `companionWorld.ts:607-671` supplies query/command access and resident availability checks; the native reducer does not reproduce or bypass them. `companionCourierProgress.ts` supplies saved-goal selection, resident identity and the three-delivery target. `companionHaulModel.ts:88-104` supplies remaining versus original money amounts.

## Shared dependencies

The private StateJson dependency is now pinned to `bc6e288621f97054c06368a9e7434835c7ce0ede`, including `EnumValue` and `ParseNull`. Both working-tree helper files matched that commit during enum integration inspection. The new header adds only the shared enum reader; the .cpp and all existing reader operations are unchanged from `691a60a98ac276d3c9a59793965b5b1bb47a69d1`. That earlier nullable-root correction superseded `2dd54bfe57574b9e281ee4846fa10d51bb0ce9e2`. Neither helper was edited by this assignment. Independent review of the new helper integration remains pending.

The parent recorded installed UE 5.8.3 source evidence in `docs/engineering/unreal-native-state-review-2026-10-10.md`: `JsonReader.h:307-327` rejects scalar roots, and `JsonSerializer.cpp:5-28` accepts object/array roots only. The record identifies the engine files by SHA-256. This assignment inspected that record and the helper correction, not the remote engine. `DecodeJobsJob` uses shared `ParseNull` for a bare null response. The helper wraps the original input in an array for UE syntax validation and accepts exactly one lowercase null; nested nullable fields use shared `Nullable`.

JobsState reuses `FHaulReward`, `FHaulEntry`, `FHaulStoreSnapshot`, `FRecoveryReadView`, `FWorldPlayerView` and `EWorldFieldPresence`. The command decoder passes the received player subtree and server time to `DecodeWorldPlayerDocument`. It does not duplicate the player, recovery or full haul readers. The existing world decoder also uses MovementProtocol; all required native dependencies must travel with a later source delivery.

Native paths below are relative to `apps/companion-unreal/Source/BaddieCompanion`. Other than StateJson's explicit correction pin, these hashes identify dependencies as present at the initial JobsState source checkpoint.

| Dependency | SHA-256 |
| --- | --- |
| `Private/BaddieCompanionStateJson.h` | `fa20d7b05543229ec99caf20904cc68ab0a88b86f910efa777de6f791fc43040` |
| `Private/BaddieCompanionStateJson.cpp` | `df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce` |
| `Public/BaddieCompanionHaulState.h` | `3414c83fe57ca20724d0362efda073503e664c14b8724668c46888401a011869` |
| `Private/BaddieCompanionHaulState.cpp` | `8768b4fcaaacc7db44e29e2085547a03955ebaa134f60a58a91f87af03525f57` |
| `Public/BaddieCompanionRecoveryState.h` | `2c874a564ef777217630435549a1f10d36ac7215d509e59c82b48122326d77c7` |
| `Private/BaddieCompanionRecoveryState.cpp` | `e47db6550d6b173aaa8e24dda2ddb7cbb5f18893cd83375b7b2084911059562a` |
| `Public/BaddieCompanionWorldPlayer.h` | `90d230cf0233458c46324fd87d6f077c6f54bd7dd691030ba5a2a89e8607a17d` |
| `Private/BaddieCompanionWorldPlayer.cpp` | `210d5cca522903cebb3aadcabf9206123cd067fead033bf3791b39c16833b416` |
| `Public/BaddieCompanionMovementProtocol.h` | `d98eb366813cad032d1e2d241367578ca20d357a5169f6e244a68b1b388af2e6` |
| `Private/BaddieCompanionMovementProtocol.cpp` | `ddacf3e6f68bb27946f28c03c88f867a171db120e1b608a1234470c16f0c4825` |

## Data and presence

All four decoders accept endpoint values, not transport envelopes. They decode into fresh candidates, publish only on success, clear output on failure and return fixed public diagnostics. JobsState ignores unknown fields in its own objects; the delegated WorldPlayer decoder retains its existing player-field allowlist. Exact string comparisons are case-sensitive and accepted text is preserved without trimming or normalization.

Jobs retain IDs, full received terms, revision, status, nullable failure, acceptance/collection/end times and nullable ledger reward. Terms retain issuance key, world, labels, employer/destination contacts, parcel key/label, approval policy, approved value and acceptance window. `timeBasis` must be `server-ms`. The unavailable reward reason is exactly `approval_required`. `requiresTrustedCourier` distinguishes absence from true; explicit false or null fails. Parcel fields remain cargo, never an inventory item or reward. The acceptance window is not a delivery deadline.

The courier reward reader deliberately accepts only delivery-source, money/game-value-unit entries into the existing haul types. `companionJobView` guarantees this narrower branch before returning a job. The general haul entry reader is private, so JobsState neither calls that private function nor invents a haul snapshot to reach it. It retains reward ID, outing ID, source key, unit key, remaining amount, ledger status, grant time and optional original amount. Delivered jobs require a reward; other stages require null. Reward outing/unit/original amount must match the received job and approved terms. This is consistency checking, not independent reward approval or ownership proof.

Zero remaining reward is valid. An original amount may be absent but cannot be null; when present it must exceed the remaining amount. The backend emits it only after a positive transfer. No local balance, grant, transfer or completion source key is calculated.

Required nullable fields reject missing values. A successful root null job is distinct from invalid/missing input. The owner's `JobPresence` uses `EWorldFieldPresence`: `Missing` before any accepted value and after read failure, `Null` after an accepted no-job result when no prior job survives ordering, and `Value` for a retained job. Offers use `bOffersReady` to distinguish unread from a known empty list. Goals always require an object, while resident and completion time are required nullable fields. Goals retain selected state, resident identity/name, deliveries, target, completion time and `inCurrentWorld`, including a saved resident outside the current world. Completion time must be present exactly at three deliveries. Only the goal query updates the saved goal.

## Sources and lifecycle

The future account owner must supply coherent latest world, recovery and haul snapshots through `ObserveSources` before every query or command callback, matching canonical `sync()`. They must come from the selected account's validated owners. The reducer does not infer readiness from a payload or validate cross-store account ownership. A rejected source receipt must not be followed by accepting a response against older snapshots.

WorldPlayer does not contain rendered-instance receipts or the layout service list. `FJobsSources` therefore accepts explicit render attempt, rendered instance ID, current layout instance/place IDs and ordered service ID/NPC/access observations from the current world owner. This is a required future producer, not a completed renderer integration or permission claim. The source owner must bound these typed inputs before copying them. Render attempts must be nonnegative safe integers.

`Select` follows an already-admitted account/room. A changed or empty selection clears displayed data and retires source/read generations. Same IDs are a no-op. For replaced credentials or account/session stores with unchanged IDs, dispose the old reducer and create a new one. No credential enters JobsState.

The external listener owner calls `BeginObservation` for its first listener and `EndObservation` for its last, sharing `CaptureSources` between additional listeners. It owns actual subscriptions, stopping obsolete callbacks and notifications. `CaptureRead` opens or reuses a local job/offers/goal slot; it does not subscribe. Opaque GUID receipts reject other owners and retired scopes. All methods run on one game thread; `GetView` returns a value copy.

Job context requires a ready world/player in the selected room, ready recovery with health and no truthy error, and ready haul with a `lagos-v1` outing and no truthy error. An ordinary world error does not invalidate an otherwise ready job context. Goal context does require no truthy world error and depends only on selected account/room and player user ID. It is independent of life, outing and contact.

The domain key includes user, life, outing, health status and outing status. Changes clear job/contact/offers/result and rotate their receipts while retaining a still-valid goal read. The visit adds instance ID, world revision, player status and render attempt. A visit change clears selected service, feedback and command error. A contact change also clears offers and their error and rotates the offers receipt.

Contact selection requires a present living player, active living recovery, carried outing, matching current layout instance/place, matching rendered instance, and the first matching service with a nonempty NPC ID and non-policing-owner access. It does not establish NPC availability, proximity, route clearance, trust or backend admission. Received offers are filtered only by `lagos-v1` and exact employer contact, preserving server order and duplicates. No availability-window or trust result is computed locally.

`Reload` retains the latest job/goal and a still-valid contact, clears read readiness/errors/offers/feedback and retires read/domain receipts. Normal reload retains the source observation generation; revocation has already retired it, so the caller must capture a fresh source receipt after reload. Neither path retries a command. `EndObservation` clears displayed state and source/read receipts. `Clear` also drops selection. `Dispose` is terminal.

## Query and command-response ordering

Every accepted job query increments `JobUpdates`, including a retained older or null result. Jobs from another life, outing or world normalize to null before ordering. The table describes replacement of an existing job within one context.

| Incoming job | Query | Command response |
| --- | --- | --- |
| Null, including normalized scope mismatch | Retain previous job | Retain previous job |
| Same ID, lower revision | Retain previous job | Retain previous job |
| Same ID, equal revision | Accept ledger refresh | Retain previous job |
| Same ID, higher revision | Accept | Accept |
| Different ID, earlier acceptance time | Retain previous job | Retain previous job |
| Different ID, equal/later acceptance time | Accept | Accept only if no job query arrived since capture |

Progress equality compares job ID/revision/status/failure and reward ID/status/remaining money amount. It excludes immutable terms and original amount, matching the canonical store. Changed progress clears prior feedback. A failed job or a reward no longer carried also clears feedback, command error and in-flight response eligibility. Delivered with a carried reward is not treated as ended by this rule.

`CaptureMutation` records an externally admitted request's original account/room/user/life/outing/contact plus current visit, contact generation and query count. It owns no request ID, action, pending-command record or retry policy. The original contact must be supplied unchanged on a retry, even after contact selection changes. Capture refuses another in-flight receipt and requires the canonical present/alive/active/carried/not-world-or-recovery-busy conditions. That check does not authorize dispatch.

A response needs the current generation/scope/visit and a decoded returned player matching captured user, room, instance, revision and status. It never updates the world store. An existing truthy job-read error prevents replacement or feedback. A changed contact can still allow job reconciliation, but suppresses old feedback. Non-blocked feedback also requires matching latest job progress and a non-ended job. Blocked feedback may describe the latest retained or null job. Goal data never changes from a command response.

Trusted source null/identity/life/outing changes, death, hospitalization or resolved outings cancel response eligibility. Detention is not treated as death, though its context change retires the current response. Empty error snapshots alone do not prove an identity change. Busy remains tied to the exact in-flight receipt until it completes, even after selection changes or observation ends. An obsolete completion releases its own busy receipt without applying data; it cannot release a newer receipt. Disposal clears it terminally. The external owner remains responsible for preserving ambiguous request identity and any permitted retry.

## Errors and remaining caller responsibilities

Main error precedence is command, job, then offers, using optional presence as canonical null-coalescing does. Goal errors remain separate. Truthy error checks treat an empty string as false. An ordinary query failure retires that query, clears its data, marks its read ready and retains its public error. Job failure also clears feedback. Invalid decoded input follows the same failure path. A nonempty query error prevents reopening until explicit reload or the relevant scope changes; other reads remain independent.

The future transport must pass already-sanitized errors and apply `companion-jobs.ts`'s exact `accessFailure` classifier, including status, code and public-message rules. Source access checks use world, recovery, then haul order. `FJobsSources::AccessFailure` contains the first classified public message, not a raw exception. `FJobsPublicFailure` supplies the classification for query and command failures. This reducer only caps error text length. It does not sanitize raw payloads or classify access by itself.

Current access failure clears displayed data, retires source/read receipts and blocks reads until reload. A command access failure uses scope validity even if the visit changed; ordinary command errors require the same visit and contact. A definitively rejected failure uses sanitized server text. Ambiguous or malformed command responses use the canonical uncertain-result message. The transport must compute definitive rejection using the canonical network/details/status rules. Rejection of a retry does not establish that the earlier ambiguous attempt failed; that pending-command decision remains external.

The native view deliberately has no `retryable` or `goalBusy` field. There is no send/retry/setGoal implementation, goal-save response path, command argument encoder or subscription scheduler. Parent integration must supply those owners separately if authorized, plus account lifetime, bounded source production, error classification and main-thread callback dispatch. The private job/life/outing data must never be passed to peers or a scene renderer. Backend authority still owns issuance, acceptance, reach checks, trust, parcel stages and reward settlement.

## Bounds and review status

Ordinary JSON input is limited to 524,288 TCHARs, nesting depth 16 and 16,384 separators. Command envelopes allow 2,097,152 TCHARs, depth 32 and 131,072 separators; the delegated WorldPlayer decoder applies its own limits as well. The shared preflight runs before UE deserialization. Transport byte limits must apply before constructing the FString.

Offers are capped at 16. IDs are nonempty and at most 240 characters; game keys at most 160 with nonempty trimmed content; labels at most 100; issuance keys at most 64 using the canonical ASCII pattern. Accepted strings retain original whitespace. Resident identity is nonempty and at most 320; resident names and retained public errors are at most 8,192. Oversized errors use fixed fallback text, not truncation.

Integers are JavaScript-safe and nonnegative unless a positive minimum applies. Job revisions and command `serverNow` start at one; offer availability revisions may be zero. Approved/original amounts are 1 through 1,000,000,000; remaining amounts may be zero. Opens/closes are integers no greater than `MAX_SAFE_INTEGER - 1`, with a positive window of at most 30 days and different non-Home contacts. Job, reward and goal event timestamps preserve finite nonnegative values through `MAX_SAFE_INTEGER`, including fractions. Goal deliveries are 0 through 3 with target exactly 3. Limits reject input atomically and never truncate offers or infer success.

Initial source/diff inspection traced null/empty/missing values, optional true-only fields, zero remaining rewards, every enum, scoped filtering, equal/lower revision ordering, intervening queries, contact/visit changes, goal independence, access revocation, ordinary failures, reload, unsubscribe, stale completion and disposal. It also removed an unused helper, avoided an optional-value overload, rejected invalid read kinds and added a post-reconciliation contact guard. The parent subsequently reported zero actionable Spec findings in the full Jobs/NPC review through `bec5a28fdae1976ed1c7fb469f765b30c8458ea8`, plus one shared Standards duplication finding addressed below.

| JobsState source | SHA-256 |
| --- | --- |
| `Public/BaddieCompanionJobsState.h` | `5f013bb5f4403f4e638aeacd0ca18a478b2da86576195286ad2e836964811180` |
| `Private/BaddieCompanionJobsState.cpp` | `3037ca6a8787303b4f08e9bc27ca2153f3f1f1d1ddbada8b2ace7693910c3f1c` |

## Shared enum correction

The parent confirmed that JobsState and NpcDialogueState repeated the same schema-independent enum string/choice loop. JobsState now deletes its local `Enum` template and calls `StateJsonPrivate::EnumValue<T>` through its existing namespace import. The source correction is `0005d007a2edb413054a2e2b1007d45ac14944d5` and requires helper `bc6e288621f97054c06368a9e7434835c7ce0ede`.

Source/diff review covered all six changed call sites: reward approval, haul reward status, job status, nullable job failure, command outcome and nullable blocked reason. Their exact choice tables and destination values are unchanged. The shared reader uses the same `String` and case-sensitive `Same` operations, assigns only on a matching choice, and otherwise returns false without assigning. Required-nullable handling, parse budgets, surrounding validation, public headers, publication and lifecycle ordering are unchanged. The optional repeated read-slot dispatch judgment remains deferred.

This is source/diff review only. Independent acceptance of the enum integration remains pending. The existing Windows 22-path transfer remains pinned to `691a60a98ac276d3c9a59793965b5b1bb47a69d1`; this assignment did not change or replay it. No test, typecheck, lint, build, compilation, code generation, application process, remote transfer or engine/Blender operation ran for this correction.

No tests, lint, typechecks, builds, compilation, code generation, launches, imports, installs, asset conversion, network requests, live configuration changes or remote transfers ran for this assignment. No runtime acceptance or release is claimed. The parent owns acceptance and Windows coordination. Native transport/admission, source producers, controller/HUD binding and device/in-game verification remain incomplete.
