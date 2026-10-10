# Unreal command continuation review

October 10, 2026. Independent source acceptance only. Native transport, Windows delivery, compilation, in-game inspection and release remain separate.

The spec is `unreal-command-continuation-assignments-2026-10-10.md` at `18dcf98aaaa76e3f45f2703e6b38fd0cd1ef532a`. All full reviews start at canonical base `12270e8fe452a81b528e356a70f937c8010f17b2`. Each scope includes its complete source and owner-written handoff, not only the last incremental commit. Reviewers performed source inspection without executing project code or validation.

## Sports and equipment caller isolation

Source is `c115663abdd4caf8abbca5b17940d4fff83eb355`; handoff is `c5379d4053f7295d485cc5abbf489e715461a2dd`. The full five-path diff ends at the handoff commit and contains 111 additions. Paths are backend `companionRacing.ts`, `companionFootball.ts`, `companionCombatEquipment.ts`, the source-status comment in `apps/mobile/src/features/you/whatsNewDropContent.ts`, and `unreal-sports-equipment-isolation-handoff-2026-10-10.md`.

### Standards

Reviewer `01a127bb-6386-72d1-ab8c-787a1a42e25a` found zero documented breaches and zero actionable heuristic smells. Six guards reuse the existing helper and authenticated identity. Endpoint-specific repeated calls do not warrant an abstraction. Shared read/trusted simulation bodies and public highlights remain unchanged. All 26 committed-file hashes in the handoff match. Accepted; reviewer closed.

### Spec

Reviewer `01a127bb-6417-7062-9290-97e3589f11d0` found zero findings. Racing command, Equipment setCarry and Football open/input/pulse/command check caller ownership before their replay, release, mutation and simulation paths. Football's feature-check exceptions do not waive isolation. Authentication remains first; validators, ordinary-phone branches, queries and trusted helpers remain unchanged. Source/preimage/output pins match. Accepted; reviewer closed.

This patch is caller-only isolation. Peer effects, retained-presence cleanup, parent/session invalidation and other domains remain incomplete. No backend delivery, deployment or activation is queued. The owner's reported What's New guard pass is recorded in its handoff; reviewers did not rerun it or reconstruct transient worktree state.

## Food and kitchen commands

Source is `9e47f5488dc7e91d18c1253472d3167e58377ee2`; handoff is `1c2f95a63c9292724b92d6179ac0d62d19254997`. The full five-path diff ends at that handoff and contains 678 additions and 47 removals. Paths are FoodState.h/.cpp, KitchenState.h/.cpp and `unreal-food-kitchen-command-handoff-2026-10-10.md`. Native paths are under `apps/companion-unreal/Source/BaddieCompanion/Public` and `Private`.

### Standards

Reviewer `01a127bf-4813-7761-9651-1cea4508d8d6` found zero documented breaches and one low-priority Duplicated Code judgment. Attempt bookkeeping repeats between FoodState.cpp:511 and KitchenState.cpp:309 at the review end commit. Sharing those mechanics could reduce future paired edits. The parent defers that maintenance change because Food generation handling, detached completion and disposal semantics differ; no behavioral defect was found. Shared enum/array/request-ID/key helpers are reused, and StateJson remains unchanged. Accepted with that explicit judgment.

### Spec

Reviewer `01a127bf-4888-73f2-bfe0-cf0afc1bfc1d` found zero findings. All five Food actions and seven Kitchen branches, including canonical cook, preserve endpoint/payload differences, immutable retries, dispatch uncertainty, reentry, owner/attempt/scope/generation checks, detached Food results, query-owned snapshots and error retention. The four output and 25 reference/preimage/dependency hashes match. Accepted.

Both Food/Kitchen reviewers are closed. Definite retry failures drop these pending requests according to their canonical stores, unlike Jobs. Present-empty movingOfferId preserves the existing client/backend mismatch; it is not new purchase authority. Both owners remain local intent/receipt state without authenticated adapters, UI, payment permission or simulation. Delivery must carry both complete state pairs and matching StateJson from `b4bc1b7d7dbc9552ad1451062a404fc872a13224`.

## Courier commands

Source is `ca06a0789fc13eda1216d4fec41e604d56085aef`; handoff is `3344244693645acb7869f374e54ddc41e3713c33`. The full three-path diff ends at that handoff and contains 488 additions and 24 removals. Paths are JobsState.h/.cpp and `unreal-jobs-command-handoff-2026-10-10.md`.

### Standards

Reviewer `01a127c3-8612-7602-ae6f-0d50f7ac22a0` found zero documented breaches and zero actionable heuristic judgments. Scope reconciliation, mutation capture and result application reuse the existing owner. StateJson's null parser remains shared. The retained read-state name and compatibility overload follow the preservation requirement; command and goal lifecycles have different retention semantics. All 19 dependency hashes and both preimage/output pairs match. Accepted.

### Spec

Reviewer `01a127c3-8696-7b02-a887-2af10bb29aaf` found zero missing requirements, scope creep or incorrect implementations. All four commands preserve approval, progression, contact and concurrency checks. Retry retains immutable arguments and earlier uncertainty after a rejected replay. Dispatch, completion, ledger ordering, malformed responses, final feedback, scope-ending changes and query-owned goal selection match the assignment. Both StateJson files match `b4bc1b7d7dbc9552ad1451062a404fc872a13224`. Accepted.

Both courier reviewers are closed. Jobs, Food and Kitchen source is ready for one separately dispatched guarded Windows batch with matching StateJson. Transport, admission, source producers and mounting remain incomplete.

## Delivery boundary

Keep access/readiness gates and the world-control interlock disabled. Read data, local intents and saved selection do not grant command authority. Parent owns integration and acceptance; the existing coordinator alone owns a separately dispatched guarded Windows handoff. Prior read-state deliveries remain complete and must not be replayed.
