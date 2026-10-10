# Room activity and resident read assignments

October 10, 2026. Source-only continuation of the full Unreal port.

## Pins and ownership

Canonical base is `98de7532f3f983d4cf794d6282b0b9e2f33c1ee4`. The parent matched all 23 references and dependencies listed below to that commit and confirmed all six output paths absent before dispatch. Owners must record the exact hashes they consume in their handoffs. Existing completed scopes remain read-only.

| Existing chat, host local | Exclusive output paths |
| --- | --- |
| `01a12714-f0c9-7153-8f81-94e2816fca6e` | `Public/BaddieCompanionPlayState.h`, `Private/BaddieCompanionPlayState.cpp`, `docs/engineering/unreal-play-state-handoff-2026-10-10.md` |
| `01a126d5-064b-7703-a27e-31991c45a302` | `Public/BaddieCompanionResidentState.h`, `Private/BaddieCompanionResidentState.cpp`, `docs/engineering/unreal-resident-state-handoff-2026-10-10.md` |

Public and Private paths are under `apps/companion-unreal/Source/BaddieCompanion/`. Parent owns shared dependencies, integration, acceptance and the port tracker. Coordinator alone owns Windows delivery. Neither owner edits the other's files, racing source, earlier state pairs, backend/mobile, configuration or production renderer inputs.

Both owners reuse private StateJson.h/.cpp at `bc6e288621f97054c06368a9e7434835c7ce0ede`, unchanged at the canonical base. Their SHA-256 values are `fa20d7b05543229ec99caf20904cc68ab0a88b86f910efa777de6f791fc43040` and `df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce`. Shared operations belong there. Report missing operations to the parent before duplicating or editing a helper.

## Room activity read

Read these canonical files at the base:

- `packages/api/src/companion-play.ts`
- `packages/utils/src/companionPlay.ts`
- `packages/utils/src/companionEmotes.ts`
- `packages/utils/src/companion.ts`
- `apps/backend/convex/companionPlay.ts`
- `apps/backend/convex/lib/companionPlayModel.ts`
- `apps/backend/convex/lib/companionModel.ts`

Also inspect the existing HomePresentation.h/.cpp at the base before deciding how to retain saved appearance. Those files are read-only. The existing appearance mapper owns render defaults, recipes and missing-content handling.

Implement bounded endpoint-value decoding for the `Wire` projection of round, playerId and memories. Cover every field of PlayRound, PlayPlayer, PlayLook, PlayMemory and all six saved PlayGame branches. Preserve optional mirrorWatchingAt and lastTouch, required nullable values, array order and source units. Retired playground data may decode for saved compatibility but must never become an available activity. Preserve `activePlayRound` semantics as a separate read projection.

Saved appearance must survive unchanged, including absent optional fields. Reuse an existing suitable mapping entry point if available. Otherwise retain the complete bounded appearance object as serialized JSON with explicitly limited validation, rather than duplicating appearance rules or generating defaults. That retained object is not validated render readiness. Record the remaining typed-appearance integration requirement.

Implement pure received-wire reconciliation corresponding to the store's `accept`. A new round ID, no prior round, equal/newer revision or incoming null round replaces the entire wire. An older revision for the same round ID retains the entire prior wire, including playerId and memories. Validate the entire incoming wire before retention. Account, room, call and invitation clearing remain with the future authenticated owner, not a permissive ready/same-scope boolean.

Use backend read guarantees and existing predicates, not command-only bounds or invented gameplay consistency. Distinguish native resource limits from canonical validation. Preserve the eight-member and twelve-memory producer limits, and document any additional bounded JSON/array limits against every union branch. Return atomic immutable results and fixed diagnostics without IDs, names or payload values. Distinguish null round from invalid or absent envelope data.

This slice does not send commands, advance rounds, evaluate scores, start timers, apply looks, perform emotes, save memories or mount native UI. `observeRoom` is an authenticated mutation with separate generations, timeout and retry ownership. Do not disguise it as a read or supply room power from Play state. Record command reply serverTime and pending-error composition as excluded caller responsibilities. Read-only Home/apartment consent cannot invoke either mutation.

## Resident population read

Read these canonical files at the base:

- `packages/api/src/companion-world.ts`, especially connectResidents and its clearing lifecycle
- `packages/utils/src/companionResidents.ts`
- `packages/utils/src/companionResidentSimulation.ts`
- `packages/utils/src/companionWorldClock.ts`
- `packages/utils/src/companionJourney.ts`
- `packages/utils/src/companionMovement.ts`
- `packages/utils/src/companionVehicles.ts`
- `packages/utils/src/companionPlaces.ts`
- `apps/backend/convex/companionWorld.ts`, especially companionWorld:residents
- `apps/backend/convex/lib/companionResidentsModel.ts`, especially readCompanionResidentWorld and residentView

Inspect and reuse MovementPresentation.h/.cpp at the base for nested movement decoding. Keep source distances, coordinate axes and real/game time distinctions unchanged. Do not implement another movement sampler, route generator, clock or physics loop. If a required reusable decode operation is inaccessible, report the dependency to the parent and continue independent field work without publishing a false complete result.

Implement a bounded typed decoder for the serialized nullable CompanionResidentWorldView endpoint. Cover clock, nextAdvanceAtRealMs, each resident's identity, revision, epoch, needs, health, worth, activity, journey, localMove, serviceActor and optional renderAsset, plus optional placeBindings. Preserve missing versus null exactly. Retain roster identity and source strings; never replace absent art with another resident's asset. Decode received worth rather than recomputing balances or inventing free values for null valuations.

The client currently checks worldKey against clock.worldKey, roster count and nullable nonnegative safe-integer nextAdvanceAtRealMs. Preserve these checks and the declared complete types. Document additional native structural/resource checks separately from source gameplay rules. Bound data before UE deserialization, clear output atomically on failure, preserve order and use fixed diagnostics. Read-only output must not expose mutable parsed JSON.

The endpoint is game-room scoped, not a per-instance population query. Do not require all resident activities or bindings to equal the viewer's current instance. A clock epoch or world change invalidates the caller's dependent street/faction observations; a read decoder does not carry out that owner lifecycle. Document subscription/account generation, failure clearing and the one-population wakeup owner. Do not add per-resident timers, wake mutations, synthetic epochs, local routine advancement or a revision high-water mark absent from this subscription.

Structural decode is not the resident render/interaction producer. Frozen-layout compatibility, explicit service-actor replacement, target resolution, model bindings, live world acceptance and native navigation/animation remain separate. No decoded ID, binding or clock grants access to a player's Home, a resident's housing, voice, movement, combat, missions, rewards or edits. Record all missing producers instead of introducing ready flags.

## Completion

Read AGENTS.md and applicable skills before editing. Review the complete owned diff and checkpoint only the three assigned paths with hooks disabled, preserving unrelated work and staging. Handoffs must identify canonical/dependency/source pins and hashes, every translated branch, retention or clearing ownership, structural differences, remaining producers and deferred verification. Parent independent Standards and Spec acceptance precede delivery.

No tests, typechecks, lint, compilation/builds, codegen, asset conversion/import, engine/server/Blender launches, installs, remote writes, deployment or live configuration. Keep account access and world-control interlock disabled. Source implementation, review, Windows delivery, imported content, compilation, in-game inspection and release remain separate. These state-only changes do not modify production renderer inputs or mobile behavior.
