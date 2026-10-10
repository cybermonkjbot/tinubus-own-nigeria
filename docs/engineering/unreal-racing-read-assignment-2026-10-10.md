# Racing snapshot read assignment

October 10, 2026. Parent-owned source assignment. Racing simulation, input and native vehicle integration remain separate.

## Ownership and pins

Canonical pin: `dbac2133fa25f111c6ed0745ba632e551c2a63c5`. The parent matched the six canonical files below and both shared StateJson files to this pin before dispatch. The three destination paths were absent from the pin and working tree.

Existing owner chat `01a12714-f810-7ac1-8171-bc6d7c8c20d6`, host `local`, owns only:

- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionRacingState.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionRacingState.cpp`
- `docs/engineering/unreal-racing-state-handoff-2026-10-10.md`

All earlier Football, Life, Food/Kitchen, Combat/Recovery source and handoffs remain read-only. Their accepted native delivery is independent of this work. The parent owns shared helpers, integration, acceptance and the tracker. The coordinator alone owns Windows transfers.

Read these canonical files at the pin:

- `packages/utils/src/companionRacing.ts`
- `packages/utils/src/companionPlayerDriving.ts`
- `packages/api/src/companion-world.ts`
- `apps/backend/convex/companionRacing.ts`
- `apps/backend/convex/lib/companionRacingModel.ts`
- `apps/backend/convex/companionWorld.ts`

Reuse `Private/BaddieCompanionStateJson.h/.cpp` from `bc6e288621f97054c06368a9e7434835c7ce0ede`, unchanged at the canonical pin. Report any missing shared operation to the parent instead of editing or duplicating it.

## Required source behavior

Translate the complete typed `RacingSnapshot`, `RaceState`, `RaceCar`, selected-car eligibility and received input fields. Preserve source units, all phases/statuses, nullable lap/finish/start/end times and optional `recoveredAt`. Keep array order and unique user IDs. Reuse the canonical read predicates and documented native resource limits; distinguish typed endpoint guarantees from loose JavaScript truthiness and from command or simulation rules.

Implement bounded decoding and received-state retention corresponding to `readRacingSnapshot`, with an explicit expected instance and optional prior validated value. Preserve the 32,768-character input limit and eight-car limit. Invalid, absent and null inputs must remain distinguishable as decoding outcomes while making the canonical cleared racing value available to the future caller. Failure clears outputs atomically and returns fixed, non-secret diagnostics.

The older-tick rule compares the prior race's instance and `updatedAt`, not race ID or a global revision. It runs after the incoming snapshot is validated. Do not return old state before checking malformed input. Preserve complete prior snapshot retention in that branch, including its selected-car data and eligibility. Equal ticks replace; an accepted null race does not retain an old race through this rule.

Trace both `companionWorld:snapshot` reception and the racing command reply in the existing account store to document integration. World reception first accepts the current player/visit, then decodes `racingJson`. Existing world acceptance retains racing only for its actual same-visit and present-player conditions. Command callbacks have their own account/access/presentation generations. This decoder does not implement or replace those owners. Do not introduce permissive `ready`/`sameVisit` booleans, an invented standalone race query, a command latch or an unearned admission receipt.

The handoff must identify which caller owns clearing on account/session/visit change and which producers are still missing. Instance identity and read validity alone never authorize movement, race commands, vehicle ownership, fuel changes or rewards.

## Exclusions and delivery

Do not port `advanceRaceCar`, prediction, collision, track generation, recovery placement, ranking execution, input queues, stop/recover pumps, commands, retries, server clocks, awards or persistence. Later movement/vehicle presentation must reuse Unreal facilities; this assignment is a data contract, not replacement physics. No native owner, UI or renderer is mounted here.

Read AGENTS.md and applicable skills. Review source and the complete owned-path diff, then checkpoint only those paths with hooks disabled. Record source and dependency commits/hashes, all translated fields and retention rules, canonical differences, missing consumers and unverified behavior in the handoff. Preserve unrelated staging and edits.

No tests, typechecks, lint, compilation, builds, codegen, asset import/conversion, engine/server/Blender launches, installs, remote writes or live configuration are authorized. Gates and production renderer inputs remain unchanged. Parent independent Standards and Spec review precede any later Windows delivery. Source completion, delivery, compiled compatibility, in-game acceptance and release remain separate.
