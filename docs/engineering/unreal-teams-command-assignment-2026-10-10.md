# Unreal teams command assignment

October 10, 2026. Source-only command and caller-driven call-action ownership.

## Owner and pins

Existing owner `01a126d5-064b-7703-a27e-31991c45a302`, host local, owns only TeamsState.h/.cpp under `apps/companion-unreal/Source/BaddieCompanion/Public` and `Private`, plus new `docs/engineering/unreal-teams-command-handoff-2026-10-10.md`. Its POS and other prior scopes are frozen. The original social/teams owner now writes only SocialState and its new social-command handoff; Teams ownership transfers exclusively to this assignment. Leave that owner's active SocialState and the earlier combined handoff unchanged. Parent owns StateJson, assignment, tracker and acceptance; coordinator alone owns Windows writes.

Canonical references and owned preimages are pinned at `2b12d3338fcc77f17257d06414332a417bb702f0`. Parent matched all twelve working hashes below; the new handoff was absent. Recheck before edits. Use complete unchanged StateJson from `b4bc1b7d7dbc9552ad1451062a404fc872a13224`.

## Extend the existing owner

Read the full `packages/api/src/companion-game-teams.ts`, shared teams/call types, relevant backend endpoint/response declarations and the original handoff. Extend FTeamsState and its existing busy/error observation ownership rather than introducing another teams store. Retain complete view/call-entry decoding, public-city capacity 50 and separate outer/room thread IDs, expiry and all accepted read behavior.

Use UE JSON and existing strings/optionals/shared receipt identities. The scope translates Baddie's service orchestration, not an engine call client, voice system, network stack, timer or gameplay controller. A minimal explicit projection of the call fields actually consumed by the canonical store is sufficient; its real native producer remains missing. Missing call observation must not default to idle or permission.

## Commands and endpoint responses

| Command | Immutable non-credential arguments | Endpoint response and canonical send result |
| --- | --- | --- |
| create | threadId, setting, capacity | Complete admission, then attachPresence sequence; return admission only after that sequence |
| invite | roomId, startedAt, ordered userIds | invitedUserIds and expiresAt; canonical send returns null |
| join | roomId, startedAt | Complete admission, then attachPresence sequence |
| decline | roomId, startedAt | null; send returns null |
| leave | roomId, startedAt | null, then possible matching-call leave; send returns null |

Endpoints are under companionGameTeams. Preserve all five supported client settings and source values without substitution. The current create endpoint validates a numeric capacity and setting but enters the public city rather than applying the historical capacity limit. Do not enforce 12 as a new create or read rule. Invite accepts at most twelve supplied IDs; preserve order and duplicates, rather than inventing deduplication or membership checks.

Add complete admission decoding for roomId, threadId, required nullable callSessionId, startedAt, expiresAt and literal startMuted:true. Keep both null and non-null call IDs. Validate all actual endpoint response shapes atomically, including invite's empty returned array and raw null via the accepted shared helper. Do not invent invitation effects or replace server fields with local defaults. IDs and size limits must be explicit native structural boundaries, not authority.

## Canonical preflight and stages

Implement caller-driven stages for the complete send/attachPresence/catch/finally flow, without making requests or invoking a call controller.

- Refuse new work after disposal or while the command busy owner exists. create must target the selected group. For other commands, compare roomId/startedAt with the displayed room only when that room exists. Missing view is not an invented refusal or permission.
- Capture the initial current-call observation. Entering commands allow idle, or joined with a gameRoomId as canonical. Other call states fail with a fixed error. Do not infer that a joined game is the requested game at this early stage.
- For explicit leave, capture threadId/callSessionId only when the initial call is joined and its gameRoomId matches the requested room.
- Acquire command busy and clear its error, then let the existing external owner publish the canonical notification. selectedAtStart is sampled after that notification in the TypeScript store. Preserve this ordering with an explicit continuation stage. Recheck exact request and identity after synchronous notification. A disposed/cleared prepared request must not become dispatchable.
- Capture immutable packets before exposure and mark each outbound stage exactly once. Default/foreign/stale/duplicate/unmarked completion and unsent abandonment must not settle newer work. Preserve compatible legacy BeginObservedCommand/FinishObservedCommand behavior while keeping owned stages distinct. Future adapters must send or perform the explicit call action immediately after a successful mark, without a yield/notification gap.
- create/join mutation success captures its exact admission. If disposed before attachment, expose only the canonical cleanup leave intent for that admission, return null and never join.
- sameGame checks joined status plus matching gameRoomId, threadId and callSessionId, preserving required-null equality. When already the same game, return the admission without a new call join. Otherwise the latest call must be idle before exposing a join intent.
- Join intent is calls.join(admission.threadId, admission.callSessionId or omitted, {audio:false}). It never starts voice. After its completion, require a fresh call observation: joined, nonempty callSessionId, matching thread and gameRoomId, and matching admission callSessionId when that was non-null. Return admission with the observed actual callSessionId. Preserve public-city thread differences.
- Disposal after attachment success still exposes the captured admission cleanup and returns null. An attachment failure after an admission also exposes cleanup of that admission. Cleanup failure is swallowed by the canonical catch; it must not cause a new join, infinite retry or invented rollback.
- Explicit leave mutation success may expose calls.leave only if the new current-call observation still matches gameRoomId and both captured thread/call IDs. Never leave another call or the underlying group/chat. Hiding a view or disposing ordinary reads does not leave a call.
- Errors affect the current view only when not disposed and selected group equals selectedAtStart. All successful non-entering commands and all failures return null. finally releases exactly the original busy owner and lets the external owner notify only if alive. Selection/reload/last-observer loss retains that busy owner as canonical, while identity replacement cannot let old finalizers clear newer work.

Keep enough original-lifetime state to settle already-sent stages and expose captured cleanup after disposal, without reviving displayed private state or applying to a replacement identity. Explicitly document required receipt/owner lifetime, terminal release/discard and reentry handling. No global network client, automatic retry, polling, background queue or real call ownership is assigned. All mutations retain retry:false and invalidateQueryCache:false.

## Backend and permission limits

The canonical cleanup request contains only the returned roomId and startedAt. The current server-owned-city leave branch resolves the account's current guest and does not compare startedAt before releasing it. A captured request is therefore not proof that the server can isolate a replaced reservation. Record this backend gap; do not claim exact reservation safety, silently change the endpoint or enable a native caller. Backend lifecycle/mixed-player isolation remains separate source work.

Keep all account/readiness gates and world-control interlock disabled. Entry/presence v1, Home/apartment reads, crew membership, display canJoin/canHost or a decoded admission grant no native command, world, combat, voice or editing permission. The future adapter needs separately accepted native authority and a real call observation producer. No account credentials belong in packets or diagnostics.

## Completion

Use apply_patch, inspect full source/diff and commit exact owned paths with hooks disabled. Write the new handoff with immutable source/canonical/preimage/output hashes, all five commands, complete stage and response coverage, notification ordering, legacy compatibility, native limits and the unresolved backend cleanup gap. Review same/different/nullable calls, current-call changes during awaits, disposal before/after admission and attachment, cleanup failures, view selection changes, duplicate/stale completion and finally reentry by source inspection. Parent requires full independent Standards and Spec reviews before delivery.

No tests, lint, typechecks, builds/compilation, codegen, generators/imports/conversion, launches, installs, remote writes/live calls, deployment or configuration. Preserve renderer/mobile inputs and unrelated work. Native transport/admission, call producer, backend cleanup correctness, mounting, content, compilation, in-game inspection and release remain incomplete.

## SHA-256 pins

```text
3865698f96a385dfe2e5c2ebb311c72eb1b6d423451fac3e693ac285dc8b4a21  packages/api/src/companion-game-teams.ts
7b48fd0125415d7210e8042c87f4136fc05f9c3c8faa032750d4dd7190f32146  packages/utils/src/companionGameTeams.ts
564d96a85bc381760292a3e686240b85a3da9b7bd40df881acf04a095e71a308  packages/utils/src/companionCall.ts
8c375da18a2df74a76fb2b758bd2ec87c28be1734a82ccc06d9443ea854a3b2a  packages/utils/src/companionGames.ts
abaf86d039ce4a6947f75950003cc4d3b773259d8d4115c27a20a39740a83ccf  apps/backend/convex/companionGameTeams.ts
0c82dc0e21f9590926283ea5610f5699a9a838f64bece6dc9838ce0bf4d5f58e  apps/backend/convex/lib/companionGameTeamsModel.ts
9329c5c1e160fa7021256e8bf407884ad1cb58026f924e67a217d30ff0ee18fb  docs/engineering/unreal-social-teams-state-handoff-2026-10-10.md
0010ebd474920c3dcd2a1cae051b0b48750f3a217173e045b8ff1c82e7ca9da9  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionTeamsState.h
00e98e16fc184c36003a303a25813897baefca425a1e1c8fea43abf7940d76ac  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionTeamsState.cpp
13ceb1b13bee4b0793ddb117e03559bfb8ea77f8bdb8dc81baa1ffa8e70bb618  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.h
df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.cpp
536bf39a4089acea43be36448ad3d02d3cd7f94a65015d3975f761941b802328  apps/companion-unreal/Source/BaddieCompanion/BaddieCompanion.Build.cs
```
