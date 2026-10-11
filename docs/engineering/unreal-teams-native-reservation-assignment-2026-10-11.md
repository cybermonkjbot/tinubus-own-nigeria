# Native Teams reservation continuation

October 11, 2026. Source implementation only.

## Ownership and accepted dependency

Existing native Teams owner `01a126d5-064b-7703-a27e-31991c45a302` owns only the three paths below. Backend/API reservation cleanup at source `29d723cbec749a8f39d114c81c1cb0f470c32707` and handoff `f44138e74af01d70420899c26ebffe8b3f8eb7d1` passed both complete six-path reviews with zero findings. Acceptance and this assignment's canonical/preimage/dependency base are `7c5af9661efad0fb007bd9ad26d5f3990d0f8512`.

- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionTeamsState.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionTeamsState.cpp`
- `docs/engineering/unreal-teams-native-reservation-handoff-2026-10-11.md`

Parent matched all thirteen reference hashes and confirmed the new handoff absent. Both existing source files match their preimages. Read root AGENTS.md, the complete current canonical Teams store, shared types, backend cleanup handoff/review and original native command handoff. The original native commands have already passed review and reached Windows; its historical pending/gap statements do not supersede the tracker or new acceptance.

All backend/mobile/shared source, StateJson, Build.cs, Social/POS/Resident/NPC modules, prior handoffs, renderer inputs and tracker remain read-only. Parent owns acceptance and coordinator alone owns Windows writes.

## Required behavior

1. Extend the existing `FTeamAdmission` with an optional retained reservation generation. Preserve the original six admission fields and their structural/numeric/null behavior, public-city returned identity and literal muted start. Support the six-field legacy shape and the seven-field versioned shape. Unknown additional fields remain rejected.
2. For the optional generation, preserve canonical eligibility: a string of at most 128 UTF-16 code units with nonempty ECMAScript-trimmed content. Retain its exact original value. No UUID parsing, case folding, normalization, Identifier predicate or trimming of the retained string. Account for the full ECMAScript whitespace set rather than assuming native whitespace classification is identical. Missing legacy generation is valid and never fabricated. A present malformed generation may fail the existing atomic structural decoder boundary; document that deliberate stricter response rejection versus the account store's typed response. It must never produce a cleanup packet.
3. Replace only automatic CleanupReady actions with `companionGameTeams:releaseReservation`. Encode exactly the original admission roomId, startedAt and retained generation as non-auth JSON. Keep original account/controller lifetime binding external, retry false and cache invalidation false. Capture from the sealed original admission, never current selection/team/call or a newer entry.
4. A valid legacy admission without generation skips cleanup by advancing directly to Finally with no action receipt or outbound packet. It retains original result/error/finalization semantics. Never fall back to ordinary Leave, a timestamp-only identity or another entry request. A complete versioned admission retains its generation across the post-join callSessionId copy.
5. Preserve all three cleanup paths, retirement after admission, retirement after attachment and caught attachment failure. A failed disposal-path release may enter catch for the existing one final captured cleanup attempt. Catch cleanup failure stays swallowed. Distinct action receipts still reject duplicate completion, and detached work cannot clear a newer identity's busy/error or notify it.
6. Preserve explicit Leave's operation/arguments and exact matching-call recheck. Automatic cleanup must never call calls.leave, remove participants, leave a group, revoke a grant or enable voice. Preserve all five explicit command encoders, read/expiry behavior, retained null fields, legacy busy-owner APIs and notification/await boundaries.
7. Use a truthful cleanup action name or retain the current enum only with a precise compatibility comment. Inspect all source consumers before renaming. This is a local request/receipt continuation, not a transport or permission implementation.
8. Source inspection must trace versioned/legacy admission, malformed generation, 128/129 UTF-16 units, surrounding nonempty whitespace, all-whitespace including BOM/NBSP, unchanged six-field numeric behavior, same-city replacement generation, disposal at both boundaries, attachment failure, bounded cleanup failures, same-ID identity replacement and duplicate action completion. Keep broader backend ordinary-Leave, expiry/call and cross-player isolation gaps explicit.

## Handoff and limits

Use apply_patch; checkpoint only the three owned paths with hooks disabled when the shared index is clear. Record source commit, exact reference/output hashes, response and cleanup contracts, structural rejection difference and source-inspected branches in the new handoff. Parent will review the full three-path range independently on Standards and Spec axes before delivery.

This change adds no account credentials, native transport, current-call producer, world/mission/combat/voice/payment/editing authority, runtime mounting or UI. Keep all gates and world-control interlock disabled. Source acceptance is separate from Windows delivery, compilation, in-game inspection and release. Production renderer packaging and What's New are not triggered by these standalone-only paths.

No tests, typechecks, lint, builds/compilation, codegen, asset operations, engine/server/Blender launches, installs, remote operations, deployment or configuration. Do not message other chats.

## Pins

All values are SHA-256 of committed and matching working bytes at the base above.

```text
ac5215c3bc24d4d47bc1a75229d6d9b3b3103ea2c3b0b1de895cb0782850357e  apps/backend/convex/companionGameTeams.ts
6f1e333c0900610ec4fdce0bef33d37c78b519d5fe5335748f1d243529c44b26  apps/backend/convex/lib/companionGameTeamsModel.ts
58ae88bdd6714833c15a278b173a01172913a02f2b715f8eefd8578c6b6a1a3c  packages/api/src/companion-game-teams.ts
155b6a6cd926949b63a43db01c503de25b9366b442126b0fa4ec4ceeaf7cbe11  packages/utils/src/companionGameTeams.ts
564d96a85bc381760292a3e686240b85a3da9b7bd40df881acf04a095e71a308  packages/utils/src/companionCall.ts
392e158eaf269fe40713dd2827fe2918374bd153a342fb5d51c781cc3eb8b5e4  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionTeamsState.h
492957967e12fa36fc3f089b41b43d63911cd3c3b768d3c6120fd9aefa2505d5  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionTeamsState.cpp
13ceb1b13bee4b0793ddb117e03559bfb8ea77f8bdb8dc81baa1ffa8e70bb618  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.h
df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.cpp
536bf39a4089acea43be36448ad3d02d3cd7f94a65015d3975f761941b802328  apps/companion-unreal/Source/BaddieCompanion/BaddieCompanion.Build.cs
b0af4f4f982fcb16a9c3398835f5e8357c116c19021cac5bd35ffd10f2532424  docs/engineering/unreal-teams-command-handoff-2026-10-10.md
98ebf35fed934442d9fb39736719a0e2b60650954f1c9b7212948e292ffab7f6  docs/engineering/unreal-teams-reservation-cleanup-handoff-2026-10-11.md
7373fb063e4ad40e3aa99127c1691cca6b7b44a83affd83f294523596f4c943a  docs/engineering/unreal-teams-reservation-cleanup-review-2026-10-11.md
```
