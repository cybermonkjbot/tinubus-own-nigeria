# Unreal mission command assignment

October 10, 2026. Local command and response ownership only.

## Owner and pins

Existing owner `01a12714-f0c9-7153-8f81-94e2816fca6e`, host local, owns only the MissionState.h/.cpp pair under `apps/companion-unreal/Source/BaddieCompanion/Public` and `Private`, plus new `docs/engineering/unreal-mission-command-handoff-2026-10-10.md`. Its completed Jobs, Economy and Play scopes and previous handoffs stay read-only. Parent owns StateJson, tracker and acceptance. Coordinator alone owns Windows writes.

Canonical reference and owned preimage pin is `773b036e50b89c87b10c3509715e44e11eb08d42`. Parent matched all ten paths below to working bytes and confirmed the new handoff absent before dispatch. Recheck before editing. Use the unchanged complete StateJson pair from `b4bc1b7d7dbc9552ad1451062a404fc872a13224`; request parent coordination for missing shared operations.

## Implementation

Read the previous mission handoff, complete canonical mission store and command union, then extend FMissionState. Preserve its complete decoder, selected-room generation, accepted-update counter, revision retention, exact IDs, mutation ordering and midpoint clock semantics. Use UE JSON serialization for an immutable captured request, without account credentials or a networking client.

Cover all eleven command tags:

- resume, with no additional fields.
- start, with missionId, withCrew and optional copied pickup instanceId/revision.
- join, with runId.
- set-ready, with runId, readinessRevision and ready.
- transfer-lead, with runId, revision and userId.
- choose, with runId, revision and choiceId.
- begin, advance, skip, retry and leave, each with runId/revision.

Capture only fields belonging to the selected tag. Validate required IDs, finite safe revisions, booleans and optional pickup presence without inventing catalog, equipment, target, readiness or permission. Preserve unknown bounded mission/choice IDs. The command named retry is a mission action, not a transport retry.

Provide explicit preparation, dispatch and completion/failure ownership. Reuse the existing mutation receipt and application logic where possible. The captured room, command and callback generation are immutable. Prevent stale, duplicate or foreign-owner receipts from changing state. Recheck ownership after any caller notification and immediately before sending. The future adapter must not yield or notify between successful dispatch marking and its actual request. A never-sent preparation needs local abandonment rather than a fictional ambiguous network failure.

Match canonical single busy request, successful-read error clearing, last-unsubscribe, reload, changed/same selection, identity loss and disposal behavior. No Jobs-style unresolved-request slot or manual transport retry belongs here. The canonical mission mutation disables retries and query-cache invalidation. Preserve eligible mutation midpoint clock updates even when older same-run wire is retained. Invalid start time must retain the accepted correction's diagnostic and non-mutating behavior.

## Once-per-observation resume

Translate canonical connect's resume branch as an explicit caller-consumed local intent, not an automatic network call. A valid current subscription observation containing a run may request resume once per observation generation when not busy. This includes an older same-run observation retained by accept. A response with no run, an invalid/stale observation or mutation completion must not invent the request.

Preserve the order around canonical accept notification and the subsequent busy/resumed check. The consumer must be able to process the actual observation result, notify, then recheck current generation, selection, disposal and busy before consuming the intent. Do not derive the observed-run fact only from the retained snapshot. A notification that starts another mutation must prevent that resume attempt without consuming eligibility for a later observation. Reconnect/reload resets resume eligibility; identity loss clears it. Record any native receipt/caller obligations precisely in the handoff.

## Acceptance and limits

Source review must trace every command, optional pickup absence, all eleven tags, once-only resume, stale/retained/null observations, notification reentry, busy refusal, abandoned preparations, duplicate callbacks, reload, unsubscribe and identity change. Preserve the complete existing mutation acceptance and clock behavior. Parent requires separate full Standards and Spec reviews of all three owned paths.

Implement with apply_patch and commit explicit owned paths with hooks disabled. The owner writes its new handoff with source commit, input/preimage/output hashes, complete adapter sequence and remaining gaps. Previous handoffs remain historical, not silently rewritten.

No tests, typechecks, lint, builds/compilation, codegen, generators, imports/conversion, engine/server launches, installs, remote writes, live calls, deployment or configuration changes. Keep the account/readiness gates and world-control interlock disabled. Mission/crew/readiness observations, call presence and local paths grant no movement, mission, combat, squad, voice, payment or Home-edit permission. Do not add the separate rural campaign's unimplemented commands. Transport, live producers, native UI, mounting, content, compilation, gameplay inspection and release remain incomplete.

## SHA-256 pins

```text
f56f25b592c50b491c24be0985c52f8f883ce6310d76447f1ea0342957783a30  packages/api/src/companion-missions.ts
5801b3926f513dde0fb3aaa12744667451c90dc74bff61033bebaded54366d54  packages/utils/src/companionMissions.ts
1fbb544f87bf00a3581575a61e2f4e49b8ec02724be041caa5b3fdb92e026e8a  apps/backend/convex/companionMissions.ts
4cd55f75d9fd37c32c7db641ea284d77388d5ed5f28bfdb1d421aaa707f8a99b  apps/backend/convex/lib/companionMissionsModel.ts
37bd4daa95b5aef8848185b5fcb6e2ce617ace669650a73c1f12001a6869171e  docs/engineering/unreal-mission-state-handoff-2026-10-10.md
7793727ff1caffb8c32e446400b2c7484182ce34338027b7ba2b0feaa6e0600e  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionMissionState.h
b4ff56b2fef568574eedf26fa10fbf5ba3f81a354db8592f89810f2610b98ebf  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionMissionState.cpp
13ceb1b13bee4b0793ddb117e03559bfb8ea77f8bdb8dc81baa1ffa8e70bb618  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.h
df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.cpp
536bf39a4089acea43be36448ad3d02d3cd7f94a65015d3975f761941b802328  apps/companion-unreal/Source/BaddieCompanion/BaddieCompanion.Build.cs
```

