# Unreal care and task command assignment

October 10, 2026. Local request ownership only. Saved Home access remains read-only and does not authorize care commands.

## Owner and pins

Existing owner `01a12714-f810-7ac1-8171-bc6d7c8c20d6`, host local, has completed Food/Kitchen commands and both reviews. Its new exclusive scope is the LifeState.h/.cpp pair under `apps/companion-unreal/Source/BaddieCompanion/Public` and `Private`, plus new `docs/engineering/unreal-life-command-handoff-2026-10-10.md`. All Food/Kitchen source and previous handoffs stay read-only. Parent owns StateJson, tracker and acceptance; coordinator alone owns Windows writes.

Canonical reference and owned preimage pin is `410995b4dfa23a6c78d29e1b6728c4c56199f6ac`. The parent matched all sixteen references below to working bytes before dispatch and confirmed the new handoff absent. Recheck before editing. Use the unchanged complete StateJson pair from `b4bc1b7d7dbc9552ad1451062a404fc872a13224`. Missing shared operations require parent coordination, not a new parser or an unapproved helper edit.

## Implementation

Extend the existing FLifeReadState, retaining its complete decoder, selected-account/character identity, current-view revision ordering, once-per-observation null fallback and origin distinction. Read the previous Life handoff and every canonical branch in companion-life.ts before editing. The requested result is local command capture, dispatch/completion/failure receipts and pending-state ownership. It sends no requests.

### Care actions and retry

Translate start, cancel, reminders-on and reminders-off, including optional copied cooking recipe and batches 1 or 2. Capture character, room, station, expected life revision and one request ID. Validate the declared action and wire payload without inventing a station, geometry, availability, permission or simulated state. Reuse the backend request-ID length contract; generate IDs through UE's existing GUID facility or accept an explicitly validated ID without replacing it on retry.

Exactly one immutable care request may remain unresolved. Preserve canonical actInFlight independently of public busy because read failures and observer removal can clear public presentation during an attempt. Dispatch bookkeeping is explicit. A never-sent preparation is not an ambiguous server attempt. Recheck attempt, generation, selected account/character and disposal after notification and at dispatch. No notification or asynchronous gap may occur between a successful mark and actual sending by the future admitted adapter.

Manual retry reuses the original request ID and byte-identical captured payload, even after changing character or hiding the view within that account. Match canonical first-send versus replay generation rules. Account change, clear and disposal invalidate private requests and callbacks. Capture and retain original scope separately from current presentation.

A valid non-null response for the captured character resolves the request. Publish only for the captured current generation/character and an equal or newer displayed revision. An old-generation success can resolve the original request without populating a different character's view. Invalid/mismatched replies remain ambiguous. Network/service-unavailable, the exact save-error condition and unverifiable-care response preserve pending state; definite failure drops it according to the canonical care store, including a definite failed replay. The adapter supplies sanitized failure facts rather than raw exceptions or tokens.

Compose read and command errors correctly. A successful read preserves the existing error while a care request is pending; ordinary read recovery without pending state clears it. Keep invalid/read-failure life clearing and current-view revision semantics. Selection, reconnect, last-listener removal and completion must retain their distinct generation, public busy and retryable behavior.

### Activity reports

Translate arrived, blocked and interrupted reports for the current life activity ID. Capture the selected character and callback generation. Report does not start or complete a local movement route, clear furniture, settle needs or grant arrival authority. The eventual producer must supply an admitted actual activity observation.

Preserve the canonical nullable result, matching-character validation, current-generation/equal-or-newer revision publication and blocked diagnostic. A null result makes no synthetic life. Reports have no retained manual-care retry or fabricated request ID. Preserve their ability to coexist with other operations where the canonical client permits it, with opaque bounded receipt ownership and stale/duplicate callback rejection. If a transport retry is expected by the canonical wrapper, document it as future adapter behavior; do not add timers or send loops.

### Task commands

Cover every CompanionTaskCommand branch: enqueue with optional cooking, pause, resume, stop, remove, retry, move up/down and advance. Capture selected character, expected displayed revision, request ID and an immutable tagged payload. Do not compute task availability, advance the queue locally, simulate cooking or settle rewards.

Task preparation respects current busy/pending/life predicates. Capture and notification reentry cannot submit another account's or a disposed owner's request. Preserve generation-based response/error/busy settlement and non-null matching-character/equal-or-newer revision acceptance. Task actions do not use the care request's manual-retry slot. Do not convert the task type named retry into a transport retry. Request scheduling and any wrapper retry policy remain with the future authenticated adapter.

## Completion boundary

Preserve all read behavior except the explicitly required busy/retry/error composition. Use the existing decoder and UE JSON serialization. Do not duplicate generic StateJson operations or copy Food/Kitchen lifecycle merely because names resemble each other. Record native receipt bounds and any necessary compatibility distinctions in the handoff.

Implement only owned paths with apply_patch. Review the entire source diff, then commit explicit paths with hooks disabled. The owner writes its new handoff with source commit, consumed reference/preimage/output hashes, all command branches, adapter sequence and remaining gaps. Parent requires full independent Standards and Spec reviews before acceptance and guarded delivery. Existing Life read review does not accept new commands.

No tests, typechecks, lint, builds/compilation, codegen, generator execution, assets/import/conversion, launches, installs, remote writes, live calls, deployment or configuration changes. Preserve renderer/mobile source, access/readiness gates and false world-control interlock. No account credentials in native values, logs, receipts or documents. Native command permission, activity/station producers, authentic transport, mounting, imported content, compilation, in-game inspection and release remain incomplete.

## SHA-256 pins

```text
b87d63e40ab06932e0322268217f5d4b202083fcd475c9ae39e13e497d2ae90e  packages/api/src/companion-life.ts
33957fe910e8e00718cb2cba95189abc231c0a9822cf2acb6283ee6a2a2bc992  packages/api/src/convex-error.ts
b628e8415be3f064096b05e6878e1176f238635bd641f965582e68fa9e66ecf9  packages/utils/src/companionLife.ts
878b249b0ef1629d78e78c6fc2856205930030dd3ba3cb161c3225afe6ff1619  packages/utils/src/companionTasks.ts
b87c1927a96fb1806d63ca3fd3ada1bf9686659ba2d8ec2255d4737f2df61066  packages/utils/src/companionSkills.ts
80f615c7cd627fafe60588ea93f7c725bda581ae3944737e8127db848ee2c95a  packages/utils/src/companionActivityKinds.ts
67c4d9d38b6bffe7e50e141bd53cf9915c100635afdc012481d0e862cdc124e7  apps/backend/convex/companionLife.ts
70713e76678fb90cfa645e5beba8076a4620b2cd0f51e0d1a632fe7a5511084f  apps/backend/convex/lib/companionTasksModel.ts
394ff44ed11283e02b977666570f154a8e309555eb91b2027f750d4bb2511a83  apps/backend/convex/lib/companionSkillModel.ts
1c46ba2566d0f806974a0617e685e2020b100d39c3a005a94ff71cc2864ee6d9  apps/backend/convex/lib/companionModel.ts
d9102a5084da37c2f649e8ccdf87e8046f274f17f2e799bdeec20d2c89d711c7  docs/engineering/unreal-life-state-handoff-2026-10-10.md
3639a71fde0aa52b2f03bf0ed4fd992cf381cdf27b31600b3ae650282e1ea350  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionLifeState.h
45e1e8d8b61a9ef5d4a7a5a5728e42fa964421c648a2bb277121cf27a97a784e  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionLifeState.cpp
13ceb1b13bee4b0793ddb117e03559bfb8ea77f8bdb8dc81baa1ffa8e70bb618  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.h
df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.cpp
536bf39a4089acea43be36448ad3d02d3cd7f94a65015d3975f761941b802328  apps/companion-unreal/Source/BaddieCompanion/BaddieCompanion.Build.cs
```

