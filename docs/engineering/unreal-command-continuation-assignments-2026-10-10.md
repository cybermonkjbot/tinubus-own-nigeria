# Unreal command continuation assignments

October 10, 2026. Source-only continuation. These assignments extend accepted read owners; they do not enable commands in the live client.

## Pins and ownership

Canonical base is `12270e8fe452a81b528e356a70f937c8010f17b2`. The parent matched all 54 references, dependencies and owned preimages below to that commit before dispatch. Three new handoff paths and this assignment path were absent. Each owner must recheck its preimages before editing and record consumed hashes. Scope changes require a parent update.

| Existing chat, host local | Exclusive write scope |
| --- | --- |
| `01a12714-f0c9-7153-8f81-94e2816fca6e` | JobsState.h/.cpp and `docs/engineering/unreal-jobs-command-handoff-2026-10-10.md` |
| `01a12714-f810-7ac1-8171-bc6d7c8c20d6` | FoodState.h/.cpp, KitchenState.h/.cpp and `docs/engineering/unreal-food-kitchen-command-handoff-2026-10-10.md` |
| `01a126fe-ef0d-72f2-beb2-d0bb491b6df5` | `apps/backend/convex/companionRacing.ts`, `companionFootball.ts`, `companionCombatEquipment.ts`, the source-status comment only in `apps/mobile/src/features/you/whatsNewDropContent.ts`, and `docs/engineering/unreal-sports-equipment-isolation-handoff-2026-10-10.md` |

Native state paths are under `apps/companion-unreal/Source/BaddieCompanion/Public/` and `Private/`. The full backend paths all begin `apps/backend/convex/`. These explicit scopes supersede read-only ownership only for the listed paths. All other completed state, handoff, review, renderer and setup files remain read-only. Resident, Play and Football native source is outside these assignments. Parent owns StateJson, cross-module integration, tracker and acceptance. Coordinator alone owns Windows writes. None of these source owners may transfer or launch remote work.

Native dependencies use the complete StateJson pair at `b4bc1b7d7dbc9552ad1451062a404fc872a13224`, unchanged at this base. Reuse existing readers and UE JSON facilities. Report a missing shared operation before copying it or editing the helper. Keep the accepted read behavior intact; changes in an extended read view must compose command state, not relax receipt or scope checks.

## Courier command ownership

Read the Jobs reference group below and the existing jobs read handoff. Extend the existing JobsState owner rather than building another world, recovery, haul or job subscription owner. Preserve its current public read entry points and source-reconciliation semantics.

Translate all four canonical `send` intents, accept, collect, deliver and cancel, plus manual retry and explicit goal selection. An intent contains identifiers only. Capture the command's observed instance/revision/life/outing, immutable job/offer identity, original contact/service and one request ID from coherent current sources. Reuse the existing exact contact and can-work predicates. Apply canonical offer approval, current-job progression, ready/error and in-flight/goal-save checks. Do not accept caller-supplied replacement concurrency tokens or a permissive command-ready flag.

Represent dispatch as an explicit local intent/receipt boundary. It carries only the endpoint and non-secret arguments; it sends nothing and establishes no permission. The eventual authenticated adapter must separately admit the operation. Recheck current generation after any notification and immediately before a request is marked dispatched. A request that was never dispatched must not become an ambiguous server attempt. The existing read/mutation receipt paths should own result application, so there is one implementation of equal-revision ledger retention, job update ordering and contact feedback checks.

Retain exactly one unresolved immutable request. Retry reuses its original request ID, command and service ID. Preserve canonical uncertainty across view teardown, reload and temporary empty/error snapshots. Proven account, selected game room, life, death or outing changes clear it according to `sync`; temporary detention is not a new life. Last-observer removal clears public view but may retain the private unresolved identity. Account clear/disposal always drops private state.

Match `submit` ordering, including scope versus visit/contact checks, delayed results after a contact or job changes, query-owned equal-revision banking/forfeiture, and final feedback validity. A rejected retry must not prove that an earlier ambiguous attempt failed. Preserve the original was-uncertain distinction. Error classification belongs to the sanitized transport boundary using the pinned convex-error rules; the native owner receives an explicit failure category and bounded public text, never raw server bodies or credentials. Invalid response decoding cannot silently resolve an uncertain request as success.

Goal selection captures the current goal resident identity and selected boolean, excludes concurrent commands, and preserves its separate busy/error state. The goal subscription owns saved results; a mutation response does not replace that state. No automatic retries, settlement, reward calculation, account payment, networking or UI is included.

Missing world/layout/render producers remain explicit. Existing read observations are not new readiness or authorization. Record all translated command branches, pending/error transitions and remaining authenticated adapter requirements in the new handoff.

## Food and kitchen command ownership

Read the Food reference group and existing food/kitchen read handoff. Extend the current FoodState and KitchenState owners, preserving their decoder and read receipt behavior. Reuse typed scopes and existing quote/moving-offer decoding and retention rather than duplicating them.

Translate food quote, confirm, cancel, collect and eat intents, including moving-food confirm/cancel restrictions. Preserve the exact endpoint/payload differences: ordinary cancellation carries quote ID and request ID without a selected scope; moving commands carry the captured full scope and matching offer ID. Food quantity is the declared 1/3/6 union. Kitchen covers every declared command branch in the pinned utility, without inventing a cook/simulate command.

Both owners retain one immutable captured scope/action/request ID and expose explicit prepare/dispatch/completion/failure receipts to a future authenticated adapter. Preserve the canonical request-ID validator. Mark an attempt uncertain only at actual dispatch. In-flight notification reentry must not submit a replaced or disposed intent. Retrying uses the original payload even after selection changes or the view hides. This retention remains private to one account; account teardown clears it.

Translate selection, reload, last-observer removal, disposal and in-flight completion ordering from `companion-food.ts`. Preserve the different food and kitchen disposal presentation semantics while documenting account-clear requirements. A late successful food receipt may resolve its pending request and return a detached result without publishing it into a different selection/generation. Kitchen success only resolves the request; its live read owns the kitchen snapshot.

Keep quote revision retention and the existing moving-offer transition rules. Never write a returned order into the query-owned catalog. Compose pending command errors with read errors as the canonical store does. A successful read does not clear an uncertain command's error. Classify ambiguous versus definite failures from sanitized transport facts, retain the original request when required, and use bounded safe diagnostics. Invalid replies must not count as verified success.

These are local request lifecycle and argument encoders only. They do not add transport, credentials, purchase permission, timers, expiry/price simulation, cooking progression, UI, or a payment service. Read-only Home/apartment access and a wire field saying enabled/approved cannot authorize a request.

## Sports and equipment account isolation

Read the Backend reference group, `apps/backend/AGENTS.md`, the generated Convex guidelines, relevant schema sections and the Convex skill before editing. Repository opt-in validation and existing session authentication override generic skill suggestions to build, deploy or replace auth.

Use the existing `assertAccountWorldControl` helper unchanged to reject account-origin commands when the authenticated user's current world row has a native owner. Even the original approving phone session must not issue these commands for that row. A row with no native owner retains its canonical behavior. This adds no native command wrapper or permission.

Cover `companionRacing:command`, `companionCombatEquipment:setCarry`, and football `open`, `input`, `pulse` and `command`. Check before identical/replayed receipt returns, input writes, rate-limit side effects, driver release/renewal, simulation, settlement or equipment mutation. Preserve authentication and existing read-only access validation order where possible. Football release/leave/end must not bypass caller isolation merely because they waive feature checks. Do not place a blanket gate in shared actor, peer eligibility, projection or simulation helpers used by read queries/internal callbacks.

Keep snapshots, entry roster and other read queries unchanged. Preserve endpoint names, argument/return validators, legacy ordinary-phone behavior and trusted expiry/tick/eligibility/settlement bodies. Do not alter another player's ownership or solve mixed-player policy in this caller-only patch. Mixed native/phone peer effects remain an explicit blocker.

The three source modules and the new handoff are the only backend writes. No schema/helper/auth/interlock changes. Update only the existing mobile source-status comment to record this disabled source continuation, preserving public highlights and unrelated edits. Inspect both What's New guard scripts before running only the permitted guard against the canonical base. No tests, typecheck, lint, build, codegen or backend call is authorized.

Record each entry point, gate placement, replay/release coverage, unchanged internal/read behavior and remaining mixed-player/lifecycle/domain gaps. The accepted heartbeat-loss correction and its closed reviews remain intact. This assignment does not cover retained-presence cleanup, direct participant deletion, parent invalidation or other command domains.

## Completion and review

Read AGENTS.md and applicable skills. Implement the owned source, review the whole diff and checkpoint explicit paths with hooks disabled. Owners write their own new handoffs with source commit, canonical/dependency/preimage hashes, every branch and deferred gaps. Preserve unrelated changes and staging. Report missing dependencies without widening scope.

Parent acceptance requires independent Standards and Spec review of each complete nonempty pinned scope. A prior read-only review does not accept new commands. Do not launch redundant reviewers or redo already accepted historical reviews.

Source implementation, independent review, Windows delivery, imported content, compilation, in-game inspection and release are separate. Account access and world-control interlock remain disabled. No tests, typechecks, lint, compilation/builds, codegen, asset conversion/import, engine/server/simulator/Blender launches, installs, purchases, deployment or live configuration. The backend's inspected What's New guard is the only assigned executable verification. Native-only changes do not modify mobile behavior or renderer inputs.

## Canonical reference groups

All paths below resolve at the canonical base. They remain read-only unless included in the ownership table.

### Jobs

- `packages/api/src/companion-jobs.ts`
- `packages/api/src/convex-error.ts`
- `packages/utils/src/companionJobs.ts`
- `packages/utils/src/companionWorld.ts`
- `packages/utils/src/companionHaul.ts`
- `packages/utils/src/companionRecovery.ts`
- `apps/backend/convex/companionJobs.ts`
- `apps/backend/convex/lib/companionJobsModel.ts`
- `docs/engineering/unreal-jobs-state-handoff-2026-10-10.md`
- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionJobsState.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionJobsState.cpp`
- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionWorldPlayer.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionWorldPlayer.cpp`
- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionRecoveryState.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionRecoveryState.cpp`
- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionHaulState.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionHaulState.cpp`

### Food

- `packages/api/src/companion-food.ts`
- `packages/api/src/convex-error.ts`
- `packages/utils/src/companionFood.ts`
- `packages/utils/src/companionGameEconomy.ts`
- `packages/utils/src/companionMovingHawkers.ts`
- `apps/backend/convex/companionFood.ts`
- `apps/backend/convex/companionKitchen.ts`
- `apps/backend/convex/companionWorld.ts`
- `apps/backend/convex/lib/companionFoodModel.ts`
- `apps/backend/convex/lib/companionKitchenModel.ts`
- `docs/engineering/unreal-food-kitchen-state-handoff-2026-10-10.md`
- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionFoodState.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionFoodState.cpp`
- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionKitchenState.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionKitchenState.cpp`
- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionWorldPlayer.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionWorldPlayer.cpp`
- `apps/backend/convex/lib/companionFoodSchema.ts`
- `apps/backend/convex/lib/companionMovingHawkerFoodSchema.ts`
- `apps/backend/convex/lib/companionMovingHawkerFood.ts`
- `apps/backend/convex/lib/companionKitchenAccess.ts`
- `apps/backend/convex/lib/companionGameEconomyModel.ts`

### Backend

- `apps/backend/convex/companionRacing.ts`
- `apps/backend/convex/companionFootball.ts`
- `apps/backend/convex/companionCombatEquipment.ts`
- `apps/backend/convex/lib/companionWorldAccess.ts`
- `apps/backend/convex/lib/companionWorldAccessSchema.ts`
- `apps/backend/convex/lib/companionGamesModel.ts`
- `apps/backend/convex/lib/companionGameTeamsModel.ts`
- `apps/backend/convex/lib/companionFootballModel.ts`
- `apps/backend/convex/lib/companionRacingModel.ts`
- `apps/backend/convex/lib/companionCombatEquipmentModel.ts`
- `docs/engineering/unreal-world-command-isolation-handoff-2026-10-10.md`
- `docs/engineering/unreal-world-control-access-handoff-2026-10-10.md`
- `apps/mobile/src/features/you/whatsNewDropContent.ts`
- `apps/mobile/scripts/enforce-whats-new-release.mjs`
- `apps/mobile/scripts/whats-new-api-scope.mjs`

### Shared

- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.cpp`
- `apps/companion-unreal/Source/BaddieCompanion/BaddieCompanion.Build.cs`

## SHA-256 pins

```text
a0f2c51dd102cd953291f5979a12228d4a1a2225c4004b7db439815c41c80c47  packages/api/src/companion-jobs.ts
33957fe910e8e00718cb2cba95189abc231c0a9822cf2acb6283ee6a2a2bc992  packages/api/src/convex-error.ts
9fb5f4f6ba25202b133a0dccf0452e39c2a920a10f057233c7470b69341cc408  packages/utils/src/companionJobs.ts
05b2d4f854ff8e54a8233af45b87f159cd8e90efbd552cbe5a98029b7076e1ba  packages/utils/src/companionWorld.ts
2e729e6abeba8e28c7b7d328f985451162d8ba036174da86636adac0c9f02ab4  packages/utils/src/companionHaul.ts
e4ec7d77076df9713fa4177e4abc0b71968231d8f4538660665652c9a2882d79  packages/utils/src/companionRecovery.ts
dde5d33ec3a7edc048093db3c756182034db0c7cac468fc11a6b59faf2771608  apps/backend/convex/companionJobs.ts
dbef808f04752fabe16b5589c2da9b0bf1a9045f187e3f7232d9f72d8617d8b1  apps/backend/convex/lib/companionJobsModel.ts
c207d9022adaf05076c5dae8b36d8630c7ebb8f47b51073509c75cd8e6767e85  docs/engineering/unreal-jobs-state-handoff-2026-10-10.md
5f013bb5f4403f4e638aeacd0ca18a478b2da86576195286ad2e836964811180  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionJobsState.h
3037ca6a8787303b4f08e9bc27ca2153f3f1f1d1ddbada8b2ace7693910c3f1c  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionJobsState.cpp
90d230cf0233458c46324fd87d6f077c6f54bd7dd691030ba5a2a89e8607a17d  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionWorldPlayer.h
210d5cca522903cebb3aadcabf9206123cd067fead033bf3791b39c16833b416  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionWorldPlayer.cpp
2c874a564ef777217630435549a1f10d36ac7215d509e59c82b48122326d77c7  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionRecoveryState.h
e47db6550d6b173aaa8e24dda2ddb7cbb5f18893cd83375b7b2084911059562a  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionRecoveryState.cpp
3414c83fe57ca20724d0362efda073503e664c14b8724668c46888401a011869  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionHaulState.h
8768b4fcaaacc7db44e29e2085547a03955ebaa134f60a58a91f87af03525f57  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionHaulState.cpp
4a0f0c144fa7c219d78e1786c1d250b2c17682606f5907f56779f3402c9f5ac2  packages/api/src/companion-food.ts
2738d6ad02570b0e5fe086e9fa09f05734679658def1409aa045bdef4f5b7127  packages/utils/src/companionFood.ts
85ff223aad8814d11b99eefe5c6df19b4ba19a3ee9d3235cf83aef6e522a9e62  packages/utils/src/companionGameEconomy.ts
f929b0037e76c4d0c2dd1c5e39a4e6ad2285d6284185fc9dece370e13b0d3b16  packages/utils/src/companionMovingHawkers.ts
4eef31b095b7fc8b4a4f500e048979f3706eb1403b56bec2d01d4a5220f9a7c4  apps/backend/convex/companionFood.ts
ea0856ff060390a0912aabb7627d324d8c7ad2917c2bd84db45cbfbd68585a10  apps/backend/convex/companionKitchen.ts
9f8ef66bf02c445ab688ea7410bfb6410b5c7259bc39ac7d692d0665e2ab9798  apps/backend/convex/companionWorld.ts
02504385aad7d29422dd265e54ef0f74da68519590c2e43cc1c697f3cf0ace81  apps/backend/convex/lib/companionFoodModel.ts
254976690f73cb19383d840946c5e815df1202451657c5e5c5e8303c0ddceddd  apps/backend/convex/lib/companionKitchenModel.ts
a4bb4308ee2f1611467a5832c1b7d19fa2ce857aec57ee24d99f82dbb77b3765  docs/engineering/unreal-food-kitchen-state-handoff-2026-10-10.md
ed18e57d0dd2bf2cefd3d013c4947e8031465bba35b472d3aad680cda6efc11e  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionFoodState.h
70c25e9af33373661e03720b2311faf2d919b9a53bdeaf55c7c2b15d0ce99085  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionFoodState.cpp
dfe085173d4b372a72f7cb4cdf78effe274d5f7ee1a33998775a9099038d3b38  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionKitchenState.h
bf4e004bdb149d79d56fa957b1d22ad9728d8a0484486787a5d98857fa186f84  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionKitchenState.cpp
6133331747042a7a10386953b63008be5251a7390f93083421595d86d5e28bea  apps/backend/convex/lib/companionFoodSchema.ts
5935e9dc9a7ea31103afadb854518074f9f63817050cb0cdce97f2197c798554  apps/backend/convex/lib/companionMovingHawkerFoodSchema.ts
10da44ff157adbbc3251badb987978e08a001fc538b73b4a7e2e96036878bbdc  apps/backend/convex/lib/companionMovingHawkerFood.ts
c32a5e520891feb45cbea5b1c5f530094c922d51afaa0b8a765a3d584adbdfb0  apps/backend/convex/lib/companionKitchenAccess.ts
5efe2c04a6d9d4f7cb62fd045e50a970f8d2f7be8b2f79a1fc8af9ff495c2754  apps/backend/convex/lib/companionGameEconomyModel.ts
54f56d47ecccb6f93a28b1790f17eb079274933f6db4414251d3eaa9153d7228  apps/backend/convex/companionRacing.ts
9a073d2e7b447da4a66f974e97377f802fa738e05802ca13f28d5493c4a3d2e8  apps/backend/convex/companionFootball.ts
71a4773fcb8e60be503b1fe17232368d9466b44f19d60aa1b08f836b1cd4ff39  apps/backend/convex/companionCombatEquipment.ts
cd6fce899f8d9836ce2eadadc6d1d81301d9180853427710ca699fbab8b3be3e  apps/backend/convex/lib/companionWorldAccess.ts
7b2b162b0bdec5bb7470f2b49b4f2e7ac568930b597ccf00f83b44bd7d712217  apps/backend/convex/lib/companionWorldAccessSchema.ts
78bf931821fd47fd6506ffc8a112d29234ce725dc5025ddb32d8273f8f0084d2  apps/backend/convex/lib/companionGamesModel.ts
0c82dc0e21f9590926283ea5610f5699a9a838f64bece6dc9838ce0bf4d5f58e  apps/backend/convex/lib/companionGameTeamsModel.ts
1e78be4330d2d753170ef782cf9dc69f7b6f37b1d787c57feb6e5e76a381ff2f  apps/backend/convex/lib/companionFootballModel.ts
f9b6acc4cc38d5c0ee7e000745dfcfe1d2dff556d31768524fa75cf256baf65f  apps/backend/convex/lib/companionRacingModel.ts
55cdddbcb92a1983b98585c8feebfb831468e53e79e45d01370bc23898dc35b0  apps/backend/convex/lib/companionCombatEquipmentModel.ts
48ca459407c63aee70679b4d3d72f39bb0a575496577d5684eeb4d774b339b99  docs/engineering/unreal-world-command-isolation-handoff-2026-10-10.md
6fa5beaeafd0e24f81048737ac532191cc6a309a3476454800c546c468a76726  docs/engineering/unreal-world-control-access-handoff-2026-10-10.md
cb087bcc0c524216d85ac6540b14a36693194f2bcae113512de1ce42c7d870a4  apps/mobile/src/features/you/whatsNewDropContent.ts
7a05229218f6a4975b72b645ccdcacfe8101cdba4387e5f262ff32dc93fa7f69  apps/mobile/scripts/enforce-whats-new-release.mjs
5735c431ecc4a0c9f8e9f2df866711159fe0b976aa2a478c13695af0892bb5f5  apps/mobile/scripts/whats-new-api-scope.mjs
13ceb1b13bee4b0793ddb117e03559bfb8ea77f8bdb8dc81baa1ffa8e70bb618  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.h
df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.cpp
536bf39a4089acea43be36448ad3d02d3cd7f94a65015d3975f761941b802328  apps/companion-unreal/Source/BaddieCompanion/BaddieCompanion.Build.cs
```

