# Unreal economy, POS and football read assignments

October 10, 2026. Parent-owned assignment record. These are read-state implementations, not complete gameplay stores or permission to make requests.

## Pins and ownership

Canonical source pin: `2a97459b581c9da4fe78171cfc2a12dd2afacd7c`. Before dispatch, the parent matched the working bytes of all sixteen canonical files below and both shared helper files to this pin. All six new native source paths were absent. Each owner checks its three destination paths before editing and preserves unexpected existing work.

All chats run on host `local`. Source paths below are relative to `apps/companion-unreal/Source/BaddieCompanion/`.

| Owner chat | Exclusive source paths | Exclusive handoff under `docs/engineering/` |
| --- | --- | --- |
| `01a12714-f0c9-7153-8f81-94e2816fca6e` | `Public/BaddieCompanionGameEconomyState.h`, `Private/BaddieCompanionGameEconomyState.cpp` | `unreal-game-economy-state-handoff-2026-10-10.md` |
| `01a126d5-064b-7703-a27e-31991c45a302` | `Public/BaddieCompanionPosState.h`, `Private/BaddieCompanionPosState.cpp` | `unreal-pos-state-handoff-2026-10-10.md` |
| `01a12714-f810-7ac1-8171-bc6d7c8c20d6` | `Public/BaddieCompanionFootballState.h`, `Private/BaddieCompanionFootballState.cpp` | `unreal-football-state-handoff-2026-10-10.md` |

The preceding Jobs, NPC dialogue and LifeState assignments are accepted. Their owners' earlier source and handoffs are now read-only. The parent owns StateJson, this record, acceptance and the port tracker. The coordinator alone owns Windows delivery. New source requires independent Standards and Spec review before delivery.

Reuse `Private/BaddieCompanionStateJson.h/.cpp` from `bc6e288621f97054c06368a9e7434835c7ce0ede`, byte-identical at the canonical pin. Header SHA-256 is `fa20d7b05543229ec99caf20904cc68ab0a88b86f910efa777de6f791fc43040`; implementation SHA-256 is `df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce`. Use its UE JSON parsing, exact string comparisons, bounded reads, `ParseNull` and `EnumValue`. Owners leave the helper unchanged and identify any missing shared operation for parent integration. Keep domain rules inside the relevant owner, without depending on another in-progress slice.

## Economy reads

Trace these six pinned files:

- `packages/api/src/companion-game-economy.ts`
- `packages/utils/src/companionGameEconomy.ts`
- `packages/utils/src/companionCombatEquipment.ts`
- `apps/backend/convex/companionGameEconomy.ts`
- `apps/backend/convex/lib/companionGameEconomyModel.ts`
- `apps/backend/convex/lib/companionWeaponVendorModel.ts`

Implement typed price, policy, service, quote, catalog and weapon-vendor read results, plus caller-driven account-owned read state for catalog selection, quote recovery, vendor scope selection, reload, observation teardown and disposal. Preserve independent read generations, quote ID matching, current-display revision ordering, errors and readiness. Trace both the endpoint validators and actual client receive predicates. Keep unknown, null, empty and malformed results distinct.

Preserve exact disabled-finance policy acceptance and current versus historical policy distinctions. A quoted price, historical exchange-rate field, banked value or fulfilled receipt never grants payment, conversion, withdrawal, equipment or gameplay authority. Keep amounts and units unchanged. Record any mismatch between current vendor production and the client's read bound without silently broadening the contract or changing canonical source.

Commands, quote requests/confirmation/cancellation, ambiguous retries, local wallets, conversion, cashout, settlement and grants are excluded. Do not port the command latch as an invented working command owner. The future service/world/equipment producers and transport remain explicit dependencies.

## POS reads

Trace these five pinned files:

- `packages/api/src/companion-pos.ts`
- `packages/utils/src/companionPos.ts`
- `apps/backend/convex/companionPos.ts`
- `apps/backend/convex/lib/companionPosModel.ts`
- `apps/backend/convex/lib/companionPosSettlementModel.ts`

Implement typed scopes, catalogs and required-nullable receipt fields for all six statuses, and caller-driven catalog/latest-receipt/by-ID read ownership. Preserve exact scope matching, valid captured server time for catalog intents, independent read generations, recovering-ID behavior and canonical receipt ordering. Older unrelated receipts, null after retained receipt, pending-to-quoted regression and incompatible terminal transitions must follow the existing `acceptReceipt` rules rather than a newly invented global revision rule.

Within one account, selecting a different service retains the account receipt as the canonical store does. An account change clears all private state and receipts. Read-error behavior differs between catalog and receipt reads; preserve it. A read-only reducer must identify pending-command uncertainty/error composition as deferred, not simulate it through permissive flags.

Keep `lagos-credit`, `cashConversion: disabled`, bounded versus complete source coverage and `deposit: own-home-only` intact. No balance changes, quote/confirm/cancel/settle commands, retry packets, scheduling, utility progression, receipt reconciliation against a pending mutation, payment or withdrawal integration. Source/model reads do not authorize backend changes.

## Football reads

Trace these five pinned files:

- `packages/api/src/companion-football.ts`
- `packages/utils/src/companionFootball.ts`
- `packages/utils/src/companionFootballEngine.ts`
- `apps/backend/convex/companionFootball.ts`
- `apps/backend/convex/lib/companionFootballModel.ts`

Implement typed match snapshots and entry rosters, preserving pitch, all players/ball fields, modes, phases, score, objective, nullable event/times and optional advancement. Use the engine file only to understand canonical validity and read semantics; do not translate its simulation loop. Preserve native JSON resource bounds separately from gameplay rules and reject malformed results atomically.

Add caller-driven account/room observation state with source-accurate apply ordering. An older epoch for the same match is ignored. Within the same match/epoch/player scope, retain newer revision and advancement server time according to the canonical comparisons. Keep entry-roster observers independently invalidatable, preserve nullable/absent fields and match data on ordinary query failures where the source does so. Same-ID account/session replacement must retire old receipts. Reads alone must not synthesize server-clock advancement, input activation, host privileges or call ownership.

Coordinates stay in source metres. No local ball physics, AI, match simulation, score or skill awards, input/stop/pulse pump, match opening, commands, retries, timers, call joins, UI, rendering or world registration. The complete input/lifecycle integration remains a later slice. Reuse UE movement/physics/navigation during that later integration rather than recreating engine mechanisms now.

## Delivery criteria

Each owner reads `AGENTS.md`, the port tracker and applicable skills, then implements only its three paths. Additional canonical references may be read when necessary; record their pins and why they are needed. Do not broaden write ownership to resolve a dependency.

Review source and the exact diff. Checkpoint explicit owned paths with hooks disabled, preserving unrelated staged and unstaged work. The handoff records source commits and hashes, canonical dependencies, complete translated read behavior, intentionally deferred commands/consumers, missing producers and review limits. No tests, typechecks, lint, compilation, builds, codegen, imports, conversion, launches, installs, remote writes or live configuration are authorized. Keep admission/readiness gates disabled and production renderer inputs unchanged.

Return the source and handoff commits with exact changed paths. Independent acceptance, Windows delivery, compilation, content, in-game inspection and release stay separate. Source files alone do not complete any inventory category.
