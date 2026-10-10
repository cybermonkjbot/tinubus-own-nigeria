# Native NPC dialogue state handoff

October 10, 2026. Unmounted source implementation and author source review only. No gameplay, account access or request authority is enabled.

Original source commit: `0752d23210879ed2e7e6338f58447bea5e7103e5`. The later shared-enum correction changes only the private implementation and this handoff, as recorded below.

## Owned paths and dependencies

- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionNpcDialogueState.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionNpcDialogueState.cpp`
- `docs/engineering/unreal-npc-dialogue-state-handoff-2026-10-10.md`

The implementation reuses Unreal strings, arrays, optionals, GUIDs, JSON and const shared pointers. It consumes the existing `FWorldPlayerView`, `FWorldCurrentInstanceIdentity` and `FRecoveryReadView`, and calls the existing `SampleMovement` function. It adds no physics, navigation, actor, network or clock owner.

The original shared private JSON dependency was `BaddieCompanionStateJson.h/.cpp` at fixed pin `2dd54bfe57574b9e281ee4846fa10d51bb0ce9e2`. This slice uses its object parser, case-sensitive equality, field access, unknown-key check, typed scalar reads, nullable read and time predicate. Accepted pin `691a60a98ac276d3c9a59793965b5b1bb47a69d1` adds `ParseNull` without changing those operations. The shared-enum correction now also requires the header at `bc6e288621f97054c06368a9e7434835c7ce0ede` for `StateJsonPrivate::EnumValue<T>`. Its implementation file and existing reader operations are unchanged from `691a60a98`. This slice does not call `ParseNull`, modify the helper or duplicate its JSON scanner.

## Canonical sources and field contract

`packages/api/src/companion-npc-dialogue.ts` defines the local lifecycle. `packages/utils/src/companionNpcDialogue.ts` defines the targets, topic order and reply data. The exact action response is `npcDialogueReply` in `apps/backend/convex/lib/companionNpcDialogueModel.ts`; `respond`, `contextFor`, `claim` and `finish` in `apps/backend/convex/companionNpcDialogue.ts` define its current producer and identity checks. This is the public action value, not a Convex response envelope, internal claim, stored legacy row or conversation history.

| Data | Retained contract |
| --- | --- |
| Target | Typed key, name, actorIdentity, serviceId, npcId, placeId, instanceId and revision. Only `mama-vendor` and `delivery-contact` at `mama-put` are supported. Target instance and revision must match the selected scope. |
| Authored identity | Exact `authored:<gameRoomId>:<npcId>`. Vendor uses `mama-vendor-npc` and `Food vendor`; delivery uses `delivery-contact-npc` and `Delivery contact`. Resolved authored evidence must have no resident ID and must come from verified absence of a resident mapping. |
| Resident identity | Exact `<residentWorldKey>:<residentId>`, with the supplied resident name. The target observation must include the resident ID and the same world/epoch scope as the player observation. No identity is generated from a decorative label. |
| Vendor topics, in order | `greeting` / Hello, `menu` / Menu, `work` / Delivery work, `directions` / Directions, `prices` / Prices. |
| Delivery topics, in order | `greeting` / Hello, `work` / Delivery work, `directions` / Directions, `compliment` / Compliment. Unknown keys produce no topics; unknown topic values have no wire ID. |
| Required reply fields | requestId, instanceId, revision, lifeId, npcKey, reply, text, actorIdentity, speakerName, source and confidence. Missing fields, wrong types, null required strings and unknown fields fail. Legacy rows with missing actorIdentity or speakerName are not accepted as current replies. |
| Reply alternatives | `greet`, `welcome-back`, `menu`, `work`, `directions`, `price-boundary`, `de-escalate`, `clarify`, `compliment`. Menu and price-boundary belong only to the vendor; compliment belongs only to the delivery contact. Both greeting alternatives remain representable because the native client does not own encounter history. |
| Source and confidence | `scripted` requires explicit null confidence. `laya` requires a finite numeric confidence from 0.75 through 1, matching the current finish producer. Missing confidence fails; null is never converted to zero. No confidence or source is inferred. |
| Reply identity | Request IDs match `[A-Za-z0-9._:-]{1,100}`. Instance ID is nonempty and at most 160 UTF-16 units; life ID at most 100; actor identity at most 320. Revision is a positive safe integer. |
| Display text | Speaker name is nonblank and at most 160 UTF-16 units. Reply text is nonblank and at most 1,024. Preserve supplied text and name without trimming, rewriting or normalization. The current longest variable text comes from at most eight menu references of at most 80 characters, which fits that native bound. |

`DecodeNpcDialogueReply` first takes a bounded copy of the input, so clearing an earlier output cannot invalidate aliased input text. It then resets the output, uses the shared parser and publishes a const shared reply only after all fields pass. Failures return fixed diagnostic text without payload values. No JSON object survives publication. The state decodes only after confirming that the receipt still belongs to the active request.

The document limit is 16,384 TCHAR code units, depth 4 and 64 commas/colons outside strings. These are native allocation limits, not server quotas or a receive-buffer implementation. The field limits and source/confidence pairing validate current producer output more strictly than the bare backend `v.string()` and nullable-number schema. A future longer or changed response must be reviewed; the decoder never truncates it or manufactures a reply.

## Scope and local eligibility

`FNpcDialogueScope` retains user, character, game room, instance, visit revision, moveSequence, renderAttempt, recovery life ID, resident world key and resident clock epoch key. String comparisons are case-sensitive. Resident world and epoch omission collapse to null as in the canonical `?? null` scope entries; the native observation requires both identifiers or neither. Present world/epoch IDs are bounded at 80/160 UTF-16 units. User, character and game room IDs are bounded at 240; moveSequence retains the existing 0 through 1,000,000,000 bound. Render attempts are nonnegative safe integers.

`ReadScope` requires an observed account lifetime, ready/error-free world and recovery observations, matching player/recovery owner IDs, a present player with positive finite health, and active recovery health with positive finite health. Recovery health presence must be Value. The instance must be the current visit, with matching player instance and place. Travel destinations cannot supply conversation scope.

The caller supplies fresh aligned server time. If the player has movement, the existing native sampler must succeed and report not moving; otherwise finite player X/Z is required. The state does not move the player, advance a route or interpret null movement as gameplay permission. It does not add new driving, water or activity admission rules.

Target evidence must match the full scope and exact sample time. Opening also requires the rendered instance marker and non-busy world/recovery observations. Retaining a selection, invalidating it and accepting a reply use the canonical non-rendered scope. Temporary loss of only the rendered marker, or a busy flag, can therefore retain an open conversation. Sending still requires the rendered marker and idle world/recovery state. A changed renderAttempt closes the selection even if the instance ID is unchanged.

## Request lifecycle

The class runs on one caller-owned thread. It creates no subscriptions or delegates. The future owner supplies fresh observations for every operation and completion, calls `Invalidate` on relevant world/recovery updates, and refreshes its own view after state operations.

| Operation | State and ordering |
| --- | --- |
| BeginOwner | Replaces the account lifetime even for the same user ID. Clears selection and pending state, creates a private owner generation and requires observing to be enabled again. An invalid owner clears the prior lifetime and returns an unusable receipt. |
| SetObserved | Models whether the canonical store has listeners. Setting false closes the selection. It does not release a still-pending request. Stale owner receipts cannot change the current state. |
| Open | Requires current owner, observed/eligible scope, rendering, idle dependencies and resolved target evidence. An active request blocks reopening. Success creates a new conversation generation and clears prior reply/error. Failure leaves the current view unchanged. Unimplemented resolution returns UnimplementedTarget once the other prerequisites pass. |
| Invalidate | Closes when scope or actor identity changes, movement is active/invalid, health or readiness is lost, or target re-resolution is unavailable/unimplemented. It never opens a conversation or captures a request. |
| SendMessage | Revalidates the selection, rejects an active request, then checks loading and input. Requires nonblank text no longer than 400 UTF-16 units, including whitespace. Blank detection follows ECMAScript trim whitespace; original text stays unchanged in the arguments. Oversized input is not copied into an intent. |
| SendTopic | Accepts only a topic from the selected target's ordered list and emits an empty message with a present topic. Text sends omit topic. There is no combined text/topic path. |
| Captured intent | Contains exact non-auth action arguments: gameRoomId, serviceId, actorIdentity, instanceId, revision, lifeId, requestId, message and optional topic. IDs use `npc-` plus a fresh UE GUID rather than the web store's prefix/serial construction, within the same request-ID contract. Scope fields not accepted by the action remain local to the receipt. Capture sets busy and clears prior reply/error. |
| Close | Clears target, selected scope, reply, error and visible busy state, and retires the conversation generation. The active request latch remains until its matching completion. Closing is not network cancellation. |
| AcceptReply | A receipt must match the owner generation and active dispatch before it can release that latch. Conversation generation, full scope and current target are then rechecked. Current replies are bounded-decoded and must match requestId, instanceId, revision, lifeId, npcKey and actorIdentity. Only complete success replaces the target name and publishes reply text. |
| Invalid reply | Clears the matching busy state and records ConversationChanged for malformed data or identity mismatch. No partial text/name publishes. A later explicit send can create a fresh request. |
| AcceptFailure | Uses the same receipt/scope checks and releases the matching busy state. It accepts a fixed typed error, never an arbitrary exception string. A valid failure retains the target. |
| Late or duplicate receipt | Cannot release another request or publish into a new account, visit, life, actor or conversation. The matching old completion after Close releases its own latch but remains stale for publication. A changed selection is cleared, not silently retargeted. |
| Dispose | Permanently clears the account lifetime and pending state. Later operations cannot reactivate it. |

Send output clears on failure. The caller must retain a separate copy of each unsettled intent until its result or failure is delivered, including cancellation or failed dispatch. `IsValid()` only says the intent was captured; it does not report currentness or grant dispatch permission. Capturing an intent does not send anything. A future transport must recheck current scope at actual dispatch, disable retries as the canonical store does, and return exactly one completion to release the latch. This module creates no local cooldown, lease, timeout, fingerprint, replay cache or scheduler.

Loading/input errors preserve a previously displayed reply, matching the source store's pre-send behavior. An actual capture clears that reply. The error enum retains the known NPC-domain messages and common sanitized auth/network/rate-limit messages through `NpcDialogueErrorText`. Unknown transport failures must map to ReplyUnavailable. Arbitrary safe-error passthrough from `toSafeConvexError` is not translated here; transport error normalization and any further approved mappings remain caller work. No raw error body or credentials enter the state.

## Explicit producer and authority gaps

`FNpcDialogueTargetObservation` defaults to Unimplemented. This is not a native implementation of `companionNpcDialogueTarget`. WorldPlayer still lacks complete layout services/NPCs/assets and resident population data. A supplied target or resolution enum cannot prove those facts or authorize gameplay. No current producer sets a resolved observation.

Before supplying ResolvedAuthored or ResolvedResident, the future producer must implement the exact canonical guards:

1. Select exactly one service by ID and exactly one NPC by its service binding. Require the two-service profile allowlist, matching NPC ID, food/vendor or mission/mission-contact kinds/roles, reciprocal service membership and no policing-owner access.
2. Use the accepted movement sample or player position. Require finite X/Z, distance at most 0.85 metres from the service approach, and the existing `routeSegmentClear` check using navigation floorY. This state does not implement distance or collision clearance, and an identity match cannot replace either check.
3. Inspect resident mappings before choosing an authored identity. If a mapped employee fails resident selection, return Unavailable. Never use authored identity because resident decoding, binding, sampling, availability or art validation is missing.
4. For mapped employees, preserve `selectCompanionServiceResidentProfile` and its `admitWorld`/`selectProfile` guards. These include consistent layout/generator/navigation and game/place identities, non-housing public places, valid shared time and world epoch, bounded unique resident/place bindings and claims, resident world/epoch consistency, supported render-asset identity and source yaw, appearance compatibility, sampled visibility/distance, correct NPC/art binding, settled work pose within 0.01 metres, and no local move, journey or interaction. Keep the source worth/unit validation too; do not silently remove it because dialogue displays only the name.

The server remains authoritative for session/account moderation and game access, player readiness, visit/life, combat/dance/police/football restrictions, target reach and resident availability. `companionNpcDialogueIdentity` checks bound world, work-slot employee, generated identity consistency and speaker role. Its identity fallback applies only to the canonical missing-binding/employee branches; it is not a native permission fallback. The server repeats context checks at claim and finish, including moveSequence, actor identity, request fingerprint, session and life consistency.

Encounter history, courier memory, menu references, job state, charisma/flirting skills, authored reply selection, LAYA admission, confidence fallback, leases, rate limits, skill awards and persistence stay on the backend. The native module neither invents knowledge nor reproduces those decisions. Topic selection requests text only; it does not buy food, accept work, grant trust or perform any other game action.

Remaining integration is the full target-observation producer, accepted world/recovery observation wiring, server-time alignment, lifecycle update delivery, separately authorized authenticated dialogue transport, safe error mapping and native UI. No controller, HUD, actor, voice or account/readiness gate is bound. The module retains only one reply, not history; the caller must release copied intents/views and any reply shared pointers when private scope ends. Clearing this state cannot revoke copies held elsewhere.

## Source review and delivery status

The original author source/diff review covered both targets, all topics and reply enums, required versus nullable fields, confidence/source pairs, case-sensitive identity checks, message bounds and whitespace, parser failure clearing and alias safety. Lifecycle inspection covered unobserved accounts, same-user new lifetimes, current visit versus destination, movement/health loss, renderAttempt and resident epoch changes, temporary rendering loss, busy dependencies, close during a request, failed dispatch, invalid/late/duplicate replies and failure receipts. That review left the source pair at `0752d23210879ed2e7e6338f58447bea5e7103e5`; the subsequent duplication finding and correction follow below.

This is source inspection, not executed regression coverage or independent acceptance. No tests, typechecks, lint, builds, compilation, code generation, engine/server/Blender operations, imports, installs, network/AI calls, deployment or configuration changes ran. WorldPlayer, RecoveryState, Movement, StateJson, renderer/mobile/backend files, previous handoffs and parent trackers were not edited by this assignment. Renderer packaging was not triggered.

Integration re-review, Windows delivery, compilation, rendered/gameplay verification and release remain separate and pending for the shared-enum correction. The parent owns acceptance and integration; the existing coordinator owns Windows transfers.

## Shared-enum correction

The parent reported the completed six-path Jobs/NPC review at `12afce03a...bec5a28fd`. Spec found zero actionable findings. Standards found the duplicated schema-independent enum string/choice loop in both private implementations. The parent added `StateJsonPrivate::EnumValue<T>` at `bc6e288621f97054c06368a9e7434835c7ce0ede` to address that finding.

This bounded correction deletes the local `Enum` template, explicitly imports `EnumValue` and uses it at the existing `npcKey`, `reply` and `source` call sites. Source/diff inspection compared each call with its prior table and the pinned helper:

- `npcKey` retains both choices and writes the same `Next->NpcKey` field.
- `reply` retains all nine choices, their order and the same `Next->Reply` field.
- `source` retains both choices and writes the same `Next->Source` field.

The shared helper uses the same exact string read and case-sensitive comparison. It returns false for non-strings and unmatched values without assigning the output, and assigns only on a match. Surrounding validation, short-circuit ordering, confidence checks, publication, the public header and every lifecycle method are unchanged. The optional profile-descriptor/repeated-target-dispatch judgment is not addressed.

Only `Private/BaddieCompanionNpcDialogueState.cpp` and this document change. The helper itself is read-only. Source/diff review found no additional scoped defect; no executable validation or independent acceptance of this integration is claimed. The current Windows 22-path transfer remains pinned to `691a60a98`; this correction does not change, replay or establish delivery of that transfer.

SHA-256 at the correction's source review:

```text
ba3bf7961ed8312855baf8b64b7d348e4168d7e24ee79edb792b35e25059c0bd  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionNpcDialogueState.cpp
fa20d7b05543229ec99caf20904cc68ab0a88b86f910efa777de6f791fc43040  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.h
df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.cpp
```

## Exact hashes

SHA-256 of the source pair at `0752d23210879ed2e7e6338f58447bea5e7103e5`:

```text
b03f1f8a26d5d12d3ac707596d2ce057ce704bad9149ff4903de46fa9634694e  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionNpcDialogueState.h
c7c2fa8f8819b48c760746da7f440fc2292528a0333399674d2fe218b15eda46  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionNpcDialogueState.cpp
```

Canonical source hashes inspected for this handoff:

```text
8e848709f390b728a6b740c095df39c0d0d8ffa51f222c97e38d3b4b67500578  packages/api/src/companion-npc-dialogue.ts
c88a36ea04d51c6ae06507e103adfdc1027e5d4677b879c72d3b75231294148b  packages/utils/src/companionNpcDialogue.ts
005a2b8a985b26ee4c12397cdb983cd07a0ef921b546fa1c9fd1db2f256a9afd  packages/utils/src/companionResidentProfile.ts
4a29decf12e26f5d3acf091c141e50f6e537dfb7a0460d808cb0963e8c3dc551  packages/utils/src/companionNavigation.ts
0e4a96725023f7d1d6c33e18c7774dca9c7e560cfe293b827d486e05eac85809  packages/utils/src/companionMovement.ts
75c8f91d25054af64a205e9cf65c3cb96c51891193946637f56aa4f3bd304dcb  packages/utils/src/companionWorldClock.ts
33957fe910e8e00718cb2cba95189abc231c0a9822cf2acb6283ee6a2a2bc992  packages/api/src/convex-error.ts
e16c7bebecb5f1f35992587175d99163cdaa749f79b53ee34d57fb95767c2030  apps/backend/convex/companionNpcDialogue.ts
cb99cefb52651345d9c05a0a86f0e05a2abbdd14b1854bf75bb65d886320b676  apps/backend/convex/lib/companionNpcDialogueModel.ts
8b47af8b5e067200ba7a1f2727b5fd2153127d2b6704a8842af0dd121682fe14  apps/backend/convex/lib/companionNpcDialogueIdentity.ts
3305230fd179ccab68f79005bf4db8312104e8547fffaa1567ff650dc1494821  apps/backend/convex/lib/companionResidentsModel.ts
```

Shared JSON hashes at the original dependency pin `2dd54bfe57574b9e281ee4846fa10d51bb0ce9e2`:

```text
9e404342b870e7e74ef08bab90b27b3315070bf08211e8659a5ca86d622dc46b  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.h
49f67afef76fc9a5a2d0847d27f99f68d001d76490b3a734ba7fe69b49609b0b  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.cpp
```

Historical read-only native dependency hashes at the original review. StateJson matches accepted pin `691a60a98ac276d3c9a59793965b5b1bb47a69d1`; the correction's newer header hash appears above:

```text
cf4d239e716530fa055423a2530f91b99dd36bb01b2120aba0427dd20cd4d141  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.h
df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.cpp
90d230cf0233458c46324fd87d6f077c6f54bd7dd691030ba5a2a89e8607a17d  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionWorldPlayer.h
210d5cca522903cebb3aadcabf9206123cd067fead033bf3791b39c16833b416  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionWorldPlayer.cpp
2c874a564ef777217630435549a1f10d36ac7215d509e59c82b48122326d77c7  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionRecoveryState.h
e47db6550d6b173aaa8e24dda2ddb7cbb5f18893cd83375b7b2084911059562a  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionRecoveryState.cpp
5fe18e8da08a35eaf00c75705ef277d55d8f1da46058f34a76d2f51b2355351d  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionMovementPresentation.h
424e219466c8a80259e508c0962a4f27fcb139939718149f22d3313461b6d85f  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionMovementPresentation.cpp
```
