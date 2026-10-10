# Standalone Unreal game translation

October 10, 2026. Full-port tracker, not a completion report.

## Goal and current position

We are translating the existing Companion game into the standalone Unreal client. The account-linking and read-only Home selection slice is an early dependency, not the game. Joshua corrected the narrower completion claim on October 10. Keep this port open after individual slices finish.

The reviewed baseline is Mac `a3eb8c0f0` and Windows `84cb1bc4`. All 35 scoped files matched at that checkpoint. That proves source transfer only. The client has connection controls, retained selection documents and an explicitly offline primitive inspection stage. It does not yet render the saved Companion or Home, provide admitted live movement or implement the city game.

Use `standalone-companion-unreal.md` for architecture, the Lagos direction and remote access rules. Use `standalone-companion-account-linking.md` for the grant contract. Historical source completion evidence is in `cribtest11-home-integration-2026-10-10.md`.

## Unreal-first implementation

Joshua's October 10 requirement is to reuse as much existing Unreal implementation as possible. Translate the game's behavior and saved data, not the web renderer's internal machinery. Before adding a custom gameplay or rendering system, inspect the corresponding Unreal implementation and relevant Epic sample for the selected engine version. Configure or extend that implementation. Record the concrete missing behavior before writing a replacement.

The following are implementation choices, not claims that these features are configured in this project. Epic's consulted documentation currently identifies itself as 5.8. No engine version is associated with this project yet; verify installed-engine and sample compatibility before adopting version-specific APIs or assets.

| Need | Reuse first | Baddie-specific work to retain |
| --- | --- | --- |
| Character movement and collision | Existing `ACharacter` and `UCharacterMovementComponent`. They already own the offline inspection pawn. Use supported movement/root-motion extension points rather than a second physics loop. [1] | Admission, accepted route/revision, receipt timing and correction policy. Default Unreal network replication is not an adapter for the existing backend. |
| Locomotion animation | Animation Blueprints, native pose warping and IK Rig. Evaluate Epic's Game Animation Sample and retarget compatible animation to the Companion. [2][3][4] | Approved character rig/morphs, actual displacement inputs, stance/contact requirements and appearance identity. Do not recreate the web gait state machine or IK solver by default. |
| AI navigation | Unreal Navigation System, NavMesh and path following. [5] | Permitted destinations, room revisions and server acceptance. A local path never grants gameplay clearance. |
| Assets and materials | Unreal asset references, async loading and material/mesh facilities. [6] | Stable saved asset IDs, approved hashes, recipes, transforms and explicit unsupported cases. Do not create a parallel GLB scene runtime. |
| Camera and connection UI | The existing SpringArm, CameraComponent and Slate controls in this client | Game framing, input ownership, consent and private-state clearing. Keep the native implementations already in use. |

Retain Character Movement for the current pawn; a newer movement system is not automatically a better fit. Epic still labels Mover experimental in its current documentation. [7] Do not swap frameworks merely to use something newer, nor enable a collection of unused plugins.

The approved character has no walk/run clips, as recorded in `standalone-companion-unreal-assets.md`. Unreal warping and retargeting need compatible animation and rig assets; they do not create missing animation content. The next locomotion work is to identify reusable Epic/sample assets, verify the selected version and permitted use, then bind an Animation Blueprint and native IK during the authorized engine/content pass. Preserve Baddie's contact and turning requirements in acceptance, without requiring a line-for-line port of the Three.js pose code. This instruction does not authorize installations, asset import, editor launch, compilation or live configuration.

Official references consulted October 10, 2026:

```text
[1] https://dev.epicgames.com/documentation/unreal-engine/understanding-networked-movement-in-the-character-movement-component-for-unreal-engine?lang=en-US
[2] https://dev.epicgames.com/documentation/unreal-engine/pose-warping-in-unreal-engine?lang=en-US
[3] https://dev.epicgames.com/documentation/unreal-engine/ik-rig-animation-retargeting-in-unreal-engine?lang=en-US
[4] https://dev.epicgames.com/documentation/unreal-engine/game-animation-sample-project-in-unreal-engine?lang=en-US
[5] https://dev.epicgames.com/documentation/unreal-engine/navigation-system-in-unreal-engine?lang=en-US
[6] https://dev.epicgames.com/documentation/unreal-engine/asynchronous-asset-loading-in-unreal-engine?lang=en-US
[7] https://dev.epicgames.com/documentation/unreal-engine/mover-in-unreal-engine?lang=en-US
```

## Port inventory

This inventory follows the existing `apps/mobile/src/services/companionGameService.ts`, its imported stores and the scene sources. It identifies work to translate; it is not a fresh runtime audit of the existing game. Paths are relative to the repository root. Expand each area into its actual commands, presentation states and failures before implementing it. A category cannot be declared finished from one representative action.

| Area | Existing source to trace | Current Unreal source position |
| --- | --- | --- |
| Account linking and saved selection | `apps/backend/convex/companionGameAccess.ts`, `packages/utils/src/companion.ts` | Initial source integrated and reviewed. Mac retry-view follow-up awaits transfer. Disabled; live connection unverified. |
| Companion body, appearance, wardrobe and animation | `packages/character-runtime/src/`, `packages/utils/src/companionWardrobe.ts`, `packages/companion-scene/src/Character.tsx` | Typed saved-appearance and material/attachment requirements checkpointed and independently source-reviewed on Mac. Windows handoff pending. No imported rig, native appearance rendering or animation integration. |
| Home geometry, furniture and room placement | `packages/utils/src/companionHomeDesign.ts`, `companionRoomLayouts.ts`, `companionPlacement.ts`, `companionRoomItems.ts` | Room/furniture plans and live-controller cache checkpointed and independently source-reviewed on Mac. Measured shell for pink-bedroom-v02 only; other layouts remain explicit. Windows handoff pending. No saved-layout or furniture rendering. |
| Game admission, movement and lifecycle | `packages/utils/src/companionMovement.ts`, `packages/api/src/companion-games.ts`, `companion-world.ts`, `apps/backend/convex/companionGames.ts`, `companionWorld.ts` | Route sampling/decoding, call/world argument encoding, call-result decoding and the world presentation clock implemented on Mac. No live gameplay transport. Response acceptance, admission, clock-owner wiring and native movement integration remain open. Current pawn movement is offline inspection only. |
| Room edits and Home visits | `apps/backend/convex/companions.ts`, `companionHomes.ts`, `packages/api/src/companion-homes.ts` | Not translated. Guest access must remain separate from owner editing. |
| Care, skills, tasks and object interaction | `apps/backend/convex/companionLife.ts`, `lib/companionTasksModel.ts`, `lib/companionSkillModel.ts`, `packages/companion-scene/src/native-scene/furnitureInteraction.ts` | Not translated. |
| Lagos world, places and residents | `packages/utils/src/companionCity.ts`, `companionPlacesLayouts.ts`, `packages/companion-scene/src/interactions/lagos-places/`, `apps/backend/convex/companionResidents.ts` | Art direction transferred. No translated Lagos scene or resident presentation. |
| Vehicles, transit and water traversal | `packages/utils/src/companionCityTransit.ts`, `companionWaterTraversal.ts`, `packages/companion-scene/src/interactions/driving/`, `apps/backend/convex/companionPlayerVehicles.ts`, `companionWorld.ts` | Not translated. |
| Missions, jobs, dialogue, food and cooking | `packages/api/src/companion-missions.ts`, `companion-jobs.ts`, `companion-npc-dialogue.ts`, `companion-food.ts` | Not translated. |
| Combat, equipment, policing and recovery | `packages/api/src/companion-combat.ts`, `companion-combat-equipment.ts`, `companion-policing.ts`, `companion-recovery.ts` | Not translated. |
| Social play, crews, shared rooms and spatial voice | `packages/api/src/companion-game-social.ts`, `companion-game-teams.ts`, `packages/utils/src/companionCall.ts`, `companionProximityAudio.ts` | Not translated. Existing permission and call ownership rules remain authoritative. |
| Game inventory, haul and economy | `packages/api/src/companion-haul.ts`, `companion-game-economy.ts`, `companion-pos.ts` | Not translated. The current read-only grant does not authorize these writes or account payments. |
| Room games, football and racing | `apps/backend/convex/companionPlay.ts`, `companionFootball.ts`, `companionRacing.ts` | Not translated. |
| Content capture and sharing permissions | `apps/backend/convex/companionContentHouse.ts`, `packages/companion-scene/src/native-scene/contentHouseCapture.ts` | Not translated. Social publication remains in the existing Baddie flow unless separately scoped. |
| Asset delivery, engine compatibility and release | `packages/utils/src/companionAssets.ts`, `packages/companion-scene/art/`, `apps/companion-unreal/` | No verified Unreal asset import, compiled client, rendered parity or release. |

## Current implementation slice

Owner: coordination chat `01a12172-6903-7ff1-8244-8d386ff688a7`, last verified title `Explore Unity for web games`. The parent chat owns this tracker and the main Unreal handoff while the coordinator owns gameplay changes.

The current slice translates saved Home and Companion presentation into typed C++ plans with stable IDs, layout versions, asset keys, furniture transforms, explicit coordinate conversion and unsupported results. The existing live controller consumes the plan through its valid snapshot and clears the cache when access fails. A resolver or asset key is not imported art and must not be reported as rendered parity.

Mac checkpoint `15de0f335` contains the seven Home mapping files, controller/panel integration and README update. `65830460c` records recovery and source review in `cribtest11-home-presentation-2026-10-10.md`. The parent independently reviewed that fixed range against `22bd1b9e6`; Windows handoff remains pending. The range leaves the parent session, movement, protocol and world-clock pairs and configuration unchanged. The existing automatic/manual retry expression is also unchanged.

After the initial Windows turn was confirmed interrupted, the coordinator resolved the uncertain resume outcome with a fresh bridge read: no loaded chat or later implementation turn existed. It recovered the five partial source files without overwriting local destinations, then completed the mapping on Mac. The Home record contains exact recovery hashes and all 22 canonical reference hashes. Confirm current remote state before the eventual handoff; a bridge timeout or an old start event does not establish completion or continued execution. Keep canonical source references read-only and preserve destination changes.

The parent's read-only SSH preflight after the local checkpoint found the same five untracked Windows files. All five SHA-256 values matched the Home record's recovered preimages. This confirms preserved partial source, not a completed handoff. The coordinator must recheck destination hashes immediately before writing the reviewed Home files, matching session/panel dependencies, parent movement/protocol/clock files and current parent documents.

Before binding Home or Companion art, read `standalone-companion-unreal-assets.md`. It records the exact inspected asset hashes, required GLB extensions, skeletal/morph structure, missing locomotion clips and the current engine-installation gap. Native animation integration and material/rig conversion remain after the data mapping; asset filenames do not establish readiness.

The parent checkpointed the initial `BaddieCompanionMovementPresentation.h/.cpp` sampler at Mac `a5f8542b09cd55fd2973384ef9e91ee1c6785f4e`. This ports sampling, route end time, intervening corners and collision-stop presentation from `packages/utils/src/companionMovement.ts`, SHA-256 `0e4a96725023f7d1d6c33e18c7774dca9c7e560cfe293b827d486e05eac85809`. Guarded Windows transfer remains pending. Keep the sampler only as the shared server-timing contract when integrating native movement.

`DecodeMovementRoute` now reads the existing `CompanionMovement` object through Unreal's JSON types. It retains optional initial speed, omitted pace, zero-point stop/turn routes, source coordinates and unchanged server timestamps. It rejects invalid field types, unsafe revisions/sequences, unsupported versions/paces and paths longer than the shared 32-point limit, then applies the sampler's numeric guards. Failure clears the route and returns fixed diagnostic text. The future transport must bound and decode its response first; success here establishes structure, not an accepted movement response or authority. Live transport, instance/revision/sequence acceptance, clock-owner wiring, native pawn integration and animation remain unimplemented. Source review covered all route fields, optional-field semantics, empty paths and failure clearing; compilation and runtime verification remain deferred.

`BaddieCompanionMovementProtocol.h/.cpp` adds movement-only argument encoding and call-result decoding through UE JSON. It makes no requests and creates no credentials or admission state. Source coordinates and server times stay unchanged. An admitted transport must supply its own authenticated scope.

| Existing operation | Encoded movement arguments | Response boundary |
| --- | --- | --- |
| `chatCallSpaces:move` | `sequence`, `roomRevision`, `destination`, optional `stop` and `pace`. Reject proposed paths and collision receipts because this endpoint does not accept them. | Decode `ok`, `sequence`, `roomRevision`, `position`, nullable `movement`, `retryAfterMs` and optional `reason`. |
| `companionWorld:move` | `sequence`, `revision`, `position`, required boolean `stop`, optional `pace`, `path` and `collision`. | Full `player` plus `serverNow`. This is not a call movement result and its decoding remains pending. |

The call decoder preserves declines, a zero sequence with no route, and retry delays above the call owner's short-retry window. It does not retry or convert a declined result into an accepted move. A present route must match the result's sequence and room revision. Invalid data clears output and returns fixed diagnostic text. The current read-only grant cannot use either endpoint.

Source review traced the exact argument names, optional fields, zero-sequence declines, nullable routes and owner-side lifecycle behavior in `packages/api/src/companion-calls.ts`, `companion-call-movement.ts`, `companion-world.ts`, `apps/backend/convex/chatCallSpaces.ts`, `companionWorld.ts` and `lib/chatCallSpaceModel.ts`. The two backend source hashes at review were `c17068988731ee795487c1ac9c6489986cb4dc0df17abde0cc3d7ba7e2de0291` and `46777a5156888c6ebb1544365fdd517f1bebe9129d5d52252d02880e17af3def`, respectively. Protocol helpers now have a distinct private namespace so their names do not collide with the sampler when source files share a compilation unit. Request scheduling, late-response rejection, world-player decoding, clock-owner wiring and native actor integration are still separate work. No tests, compilation or runtime verification ran. Windows transfer of this parent-owned pair is pending.

`BaddieCompanionWorldClock.h/.cpp` ports `observeTime`, the time receipt in `displaySnapshot`, and room-selection reset behavior from `packages/api/src/companion-world.ts`. It uses `FPlatformTime::Seconds`, already used by the native session, without creating a ticker or timer. It accepts positive safe-integer server times for the selected game room, ignores older/equal receipts without extending their lifetime, and advances display time by whole monotonic milliseconds. At exactly 60,000 milliseconds the receipt has zero remaining time; older samples are unavailable. Clearing or changing rooms drops the receipt, and a mismatched current-player room cannot read it. Numeric overflow also makes a sample unavailable.

`SampleRoute` connects this clock to the existing route sampler without rewriting server timestamps. It clears output when the clock or route is invalid. It must not consume locally predicted routes, and it does not establish instance identity, revision, sequence or movement ownership. The future admitted world owner must reject late callbacks and clear the clock on access loss, disposal or a new session in the same room. No world owner calls this helper yet. Source review covered receipt ordering, scope changes, expiry bounds, invalid values and output clearing against source SHA-256 `d985dbc931fec8d9c5e0b6a6e5a5aae5b6d2ffd568210e155c2ac9f0f0fa26b7`. Windows transfer and runtime verification remain pending.

The parent removed the unused `BaddieCompanionGaitPresentation.h/.cpp` implementation after Joshua's Unreal-reuse instruction. Its historical checkpoint is `86497f4dfa7ff1c7ecc6a55cb5172a9a8e923107`; do not transfer or revive it as the default locomotion path. The movement-profile naming improvements in that checkpoint are retained. The coordinator must preserve the parent-owned movement pair and current documentation during source return.

The parent has also implemented the documented automatic-retry view gap in `BaddieCompanionSession.h/.cpp` and only the retry-text hunk of `SBaddieCompanionConnectionPanel.cpp`. The existing UE HTTP/ticker owner supplies the deadline; manual begin throttling keeps its original meaning. Source review covered scheduling, expiry and private-state clearing. The account-linking document owns the public-field contract. Preserve these parent changes during Home return, then transfer the reconciled panel with its matching session pair. Windows parity and rendered countdown verification remain pending.

### Standards

The independent movement review found no documented-standard breaches and two low-priority smell judgments. The angle helper and travel-profile fields now state their meaning and units. The small separate pace dispatch functions remain close to the canonical shared contract; the suggested consolidation was a maintenance judgment, not a behavior defect.

### Spec

The independent movement review found no actionable mismatch with the five scoped canonical sampling/timing functions. Gait contact review was superseded by the Unreal-first decision, not accepted as completed animation. No compiler, tests or runtime verification ran. Summary: Standards had zero hard breaches and two smell judgments, with naming addressed; Spec had zero findings in the sampler slice.

### Home integration review

The two independent source reviews cover `22bd1b9e6...65830460c`, scoped to `apps/companion-unreal` and the Home source record. The parent separately confirmed preservation of all eight session/movement/protocol/clock source files, configuration and the retry-text expression.

Standards found no documented-standard breaches and two P3 heuristic judgments. `BaddieCompanionHomePresentation.cpp` repeats room-selection comparisons between cache matching and same-ID consistency checks. `BaddieCompanionBedroomMapping.cpp` passes position, dimension and anchor scalars in different orders across measured-layout helpers. Shared equality and named vector/box arguments are maintenance suggestions, not observed behavior defects; they are deferred to avoid changing the reviewed geometry during handoff.

Spec found no actionable missing scoped requirement, scope creep or incorrect implementation. It traced saved IDs, optional appearance and accessory semantics, canonical asset references, bedroom v02, coordinate conversion, quarter turns, support dimensions, rug height, wall-bottom placement and access-loss/cache clearing. This result covers typed mapping and its controller consumers, not imported art or the full game. No tests, compilation or rendered verification ran. Summary: Standards has zero hard breaches and two low-priority judgments; Spec has zero findings.

## Completion and continuation

For each area, record source implementation, source review, Mac/Windows handoff, imported assets, compilation, in-game verification and release separately. Include evidence and unresolved cases. A file count, completed remote turn or clean worktree cannot establish functional or visual parity.

After a slice completes, reconcile and checkpoint it, update its inventory entry, then select the next dependency-ready item. Keep the overall goal and scheduled follow-through active while authorized source work remains. Do not replace implementation with repeated unchanged status checks.

The game is complete only when the translated feature inventory is accounted for and the required runtime and visual acceptance has actually passed. Any omitted existing behavior needs Joshua's agreement; no silent scope cuts. Until then, report concrete delivered work without a completion percentage.

Current authority covers source implementation, source review and scoped handoffs. Keep existing server rules authoritative and the account-linking gate disabled. A read-only grant never becomes permission to edit saved state. Tests, builds, compilation, code generation, engine launches, tool installation, deployment and live configuration changes require explicit authorization. When those are the only remaining steps, record the exact operation requiring authority and pause scheduled work rather than marking the game done. Preserve the existing mobile/web renderer and unrelated work throughout.
