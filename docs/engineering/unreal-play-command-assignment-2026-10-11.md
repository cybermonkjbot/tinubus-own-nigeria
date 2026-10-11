# Room activity read and command lifecycle

October 11, 2026. Source-only continuation of the accepted Play wire decoder.

## Ownership and baseline

Existing chat `01a12714-f0c9-7153-8f81-94e2816fca6e`, host local, owns only:

- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionPlayState.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionPlayState.cpp`
- New `docs/engineering/unreal-play-command-handoff-2026-10-11.md`

All canonical references, dependencies and source preimages below use `cfa15652c52e510aac532a8cc4e13ce633d8415e`. Parent matched every working file. Confirm the pins and new handoff's absence before editing. Keep DecodePlayWire and ReconcilePlayWire behavior and the accepted shared StateJson reader. Add one retained Play owner in this module, not a second world or room store.

StateJson, Build.cs, the original read handoff, Racing, Economy and every earlier domain remain read-only. Parent owns this assignment, tracker, shared integration and review acceptance. Coordinator alone owns Windows writes. Read the full canonical account store, shared Play types/readPlayCommand, endpoint/response validators and existing native pair and read handoff.

## Retained reads and scope

Model account/session incarnation plus PlayScope: characterId, roomId and separately present optional callSessionId, invitationId and roomRevision. Preserve exact strings and optional presence. Account/session reset clears private data even when public IDs repeat. Same-value scope selection is a no-op; a changed selection clears the view, replaces query generation and emits.

Implement explicit subscription generation/once-only request ownership for companionPlay:snapshot, first observer, last observer, reconnect, sanitized failure and disposal. No caller-supplied ready/same-scope flag substitutes for actual retained ownership. Raw decoding stays pure; accepted null-round data is still a received wire.

An accepted null/different/equal/newer round replaces the complete wire, sets ready and clears error. An older same-ID round retains all previous fields and readiness/error, but still emits. Subscription error resets the snapshot to empty, ready=true and error. Last observer loss clears busy and retires query callbacks but retains displayed wire. Dispose clears everything and listeners. Scope and round identity are never permissions.

Utilities remain an explicit unimplemented dependency. Record all utility-clear/disable boundaries from connect, select, last unsubscribe, query failure and Dispose so a later owner can integrate them. Do not add utility-ready booleans, generate observations, call observeRoom or supply power to NativeApartment. observeRoom is a separate authenticated mutation with generation, timeout/abort, timing and continuity obligations, not a query or ordinary act command.

## All accepted commands

Translate all 18 readPlayCommand variants. Retired toy and start-playground reject. Reuse the six existing emotes and saved look type.

| Command | Fields |
| --- | --- |
| start | kind from mirror/mystery/style/kitchen/memories; optional boolean rush |
| join, leave, replay, lead, perform, watch, chop, heat, plate, serve, save-memory | type only |
| touch | integer target 0..5; optional finite power 0..1 |
| emote | one of six known emotes |
| look | rose/ivory plus top/trousers/footwear colors from the exact six-color palette |
| vote | playerId, at most 100 code units, empty allowed |
| ingredient | integer 0..5 |
| remove-memory | memoryId, at most 100 code units, empty allowed |

Use native typed command alternatives and UE JSON serialization. Reject invalid enum/numeric values atomically. Invalid/retired actions set the canonical unavailable error and emit without dispatch. A displayed playground round blocks every action except start and remove-memory. Do not revive it as an available game or locally evaluate game scores, care, ownership, membership or memory-write permission.

The initial send guards are disposed, missing scope, busy or not ready. Existing error, round presence, host membership and observer presence are not extra guards. Capture query generation and began wall-clock time, publish busy/error state, then prepare the arguments from the current scope/round after that synchronous notification. The canonical store reads expectedRoundId and expectedRevision after the notification, so a same-generation update must influence the packet. If scope/session replacement retires the operation, fail closed rather than dispatching under a different account.

Every immutable packet targets companionPlay:act with scope fields, command, expectedRoundId as string/null, expectedRevision as the current revision or zero, and an opaque unique actionId. Use native GUID/time facilities for a bounded action ID, preserving its value through this request. Backend permits 8..100 characters and a nonnegative safe expectedRevision; no rounding or invented accepted revision. All packets retain retry:false and invalidateQueryCache:false. No credentials or native permission fields are serialized.

Use explicit once-only dispatch and terminal receipts; unsent work may be explicitly abandoned, marked requests cannot be silently retried. A read failure can clear busy while a request is in flight and permit another command in the same query generation. Trace this case rather than adding a global pending-request guard that silently changes canonical behavior. Distinguish displayed busy state from each request's dispatch/cleanup identity. Old completion must not retire a newer request.

## Completion and time

Decode the full command wire plus finite serverTime atomically with current bounds. For current success, update serverOffset = serverTime - (began + receivedWallTime) / 2 before the ordinary accept reducer. Even a valid older round follows that offset/emit/finally order. Preserve snapshot data on command failure. Ordinary failure changes sanitized error without its own notification; current-generation finally clears busy and emits. Missing public failure text uses "That move couldn't finish. Try again."; present empty text stays empty. Malformed success uses fixed public-safe diagnostics.

Capture receipt identity and caller values before any notification. Recheck session/generation after callbacks and protect terminal ownership from recursion, alias replacement, same-ID reset, select, reconnect and disposal. Stale completions cannot alter a new scope. Trace accepted/retained wire, callback during busy publication, failure before finally and callbacks overlapping a query failure. Document all structurally stricter native checks and canonical limitations.

## Acceptance and handoff

Review the complete native diff against the canonical store and all command variants, including JSON escaping, optional values, invalid enums, palette restrictions, empty permitted IDs, noninteger read revisions, no-observer commands, null/older/equal round acceptance, midpoint time and overlapping requests. Leave raw wire validation, detached saved appearance, array bounds and prior generic-helper correction unchanged.

Checkpoint only the three owned paths with hooks disabled. Write immutable source/preimage/dependency hashes, packet shapes, complete public API and notification order, source-inspection evidence and remaining integration in the new handoff. Parent will obtain full-range Standards and Spec reviews.

Room utility observation, native UI/confirmations, anchors, emote/appearance playback, transport/admission and rendering remain following work. Kitchen puzzle actions never stand in for groceries, meals or Life care. Use UE-native UI, navigation and animation when those consumers are assigned, not copied web renderer machinery.

Keep all account/readiness gates and the world-control interlock disabled. Home/apartment reads and gameplay entry/presence grant no room act, call, voice, payment or editing permission. No tests, lint, typechecks, builds/compilation, codegen, asset work, launches, installs, remote writes, deployment or live configuration. Preserve unrelated work and renderer/mobile inputs. Report source, independent review, delivery, compilation, in-game inspection and release separately.

## SHA-256 pins

```text
03ae962bb47624b09c673e9dc9e35ab85a5e0b2e09448e3d70a6b9cf9cd3f8e9  packages/api/src/companion-play.ts
f002e398719f911596fd8c6308445711006be92e8d23fd827431b5784f984952  packages/utils/src/companionPlay.ts
32a184e58c20ea6022f66b01d88e831a14511a8a59c90e6cbc949b7f1b633a04  packages/utils/src/companionEmotes.ts
047ced36383ebaffca0d1be7def24209c9344c57ffc1772d8c52cd1d349a4727  apps/backend/convex/companionPlay.ts
af006f9d58574836e5dce6136ce54a71d0023b76dfa771a9682d515e83eb7ecb  apps/backend/convex/lib/companionPlayModel.ts
b9eb9bec353858e9175eab5deb3da1535c56b6f7796ded617225ad8cf7c1f3da  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionPlayState.h
75add5bd91b5b15619aa8afceb60ec3fb4d48d3e00122cf8e080177e9ff86346  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionPlayState.cpp
13ceb1b13bee4b0793ddb117e03559bfb8ea77f8bdb8dc81baa1ffa8e70bb618  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.h
df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.cpp
536bf39a4089acea43be36448ad3d02d3cd7f94a65015d3975f761941b802328  apps/companion-unreal/Source/BaddieCompanion/BaddieCompanion.Build.cs
e10d0eed4d6efd74edb565c129b0470a10a195db790b6420a6d77534bf4935e4  docs/engineering/unreal-play-state-handoff-2026-10-10.md
```
