# Cribtest11 Home integration

October 10, 2026. Source-transfer and remote-run record.

## Previous run

The authenticated TLS bridge returned no loaded threads before this integration. The Windows worktree was clean at `d041ba7`, `Implement standalone Unreal offline Home slice foundation`, after `dead6fe`, the initial transferred-source checkpoint. We read and backed up the complete scoped source before changing it. The earlier helper used ephemeral threads; no stored final-turn status was available. The commit and files establish an offline source implementation, not a working engine build.

The Mac checkout now retains the Windows HomeStage, SliceCharacter, SliceGameMode, SliceHUD, SlicePlayerController and SliceSession files and both configuration files. Existing Mac account-linking changes remain intact. Unrelated Mac work is outside this integration.

## Verified transfer

We used the existing bridge at `20.109.124.208:8766`, with its token and certificate verification for `cribtest11.local`. Every destination was checked against its inspected previous hash before replacement, and every resulting file was read back and SHA-256 verified. No guest-agent command, new service, credential transfer or live configuration change was used.

Paths below are relative to `C:\dev\cribtest-game` and the Mac repository. The subsystem and account contract match Mac commit `e21584041`; Build.cs, README and the main handoff merge both sides. The handoff includes the Lagos direction and preserved Windows inventory.

| Transferred file | SHA-256 at dispatch |
| --- | --- |
| `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionSession.h` | `58a9bfc19d3792841c8503aa53f7951f6b0af0b330138b1666ccf5367800a069` |
| `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionSession.cpp` | `4ffaee2384f14a93c73e7a01d954010d863f131b45ff81e72645dd80da4ad240` |
| `docs/engineering/standalone-companion-account-linking.md` | `7f902c19ba6675bfefa2699e11a28a516e1967a33e08d250f6c7f8d4ce7e7957` |
| `apps/companion-unreal/Source/BaddieCompanion/BaddieCompanion.Build.cs` | `df09defaf3ecb5fe25fbc6e127af69a84bdc1b0483531a3bb462d4bc934c6a46` |
| `apps/companion-unreal/README.md` | `4651f27e827601448581387ed309e1e9b34f7f1e6ed97cab827191f7add03b89` |
| `docs/engineering/standalone-companion-unreal.md` | `21f962fcc57a2db51dfe7e4955223759b7622a9a4fb679b2189b2c45e0cb258f` |

## Integration turn

- Remote thread: `01a1251b-28dd-7bb2-b185-c78e4d385455`.
- Remote turn: `01a1251b-2bee-70a0-84f3-801040520b8c`.
- Model: `gpt-6.1-sol`, high reasoning.
- Workspace: `C:\dev\cribtest-game`.
- Status at dispatch: `inProgress`. This thread is persistent, unlike the old helper's ephemeral runs.
- Terminal status: `completed`, confirmed by `thread/read` with full turns. The thread returned `idle` after completion.

The turn implements game-side connection controls and integrates existing gameplay presentation with `UBaddieCompanionSession`. It must clear private presentation when `CopySnapshot` fails, keep explicit offline inspection separate, preserve saved IDs and leave unsupported art unavailable. It may edit only the scoped standalone source and handoffs. It must retain disabled flags and omit live origins, backend changes, installs, tests, builds, code generation, engine launches and deployment.

Read this exact thread with `thread/read` and `includeTurns: true` to inspect the actual turn status. Completion requires a final `turn/completed` result or stored terminal status, source/diff review, scoped file readback and a checkpoint. A running process or an idle coordination chat is not a completion signal.

## Result

Windows gameplay commit: `66f861607f1ed24ddf14a1e9b6b4df1e3245aacb`. The agent left the three previously transferred dependency files untouched and untracked. After confirming the turn had completed and those bytes still matched the Mac checkpoint, we committed them separately at `1eda26019935a9cf34698ca1686a0eced694bf36`. The Windows worktree was clean before the return transfer. Neither commit was pushed.

We returned only the standalone source and its two existing handoffs. Every local destination was checked against its pre-run hash before copying; all 33 scoped files then matched the Windows readback byte-for-byte. Unrelated Mac work was preserved. The completion record and its pointer in the main handoff are the subsequent documentation-only closeout.

The source milestone includes:

- Controller ownership of the existing account session, with public connection state, approval-link copying, comparison code and the available lifetime/retry countdowns.
- A keyboard/pointer Slate panel with dedicated disconnect/re-pair modals, cancellation, focus restoration, scrolling and wrapping.
- Clearing of retained Companion/Home documents, IDs, names and modal/input state on invalid access, replacement pairing and teardown. Notifications, presentation ticks and actions check the session's validity.
- Explicit offline-only selection, primitive stage and local movement gates. The live path does not create a replacement Companion or pretend the primitive stage is the saved Home.
- The Lagos requirement synced to Windows without changing saved layouts, production renderer inputs or shared assets.

Source/diff review covered session clearing, delegate and viewport lifetime, offline isolation, input/focus behavior and the unchanged imported subsystem/reader hashes. Diff whitespace checks passed. Tests, compilation, builds, typechecks, lint, code generation, engine launches, new installs, deployment and rendered verification did not run. Account linking remains disabled with no committed live origin.

Saved appearance/Home asset mapping, authoritative gameplay admission and runtime acceptance remain outstanding. The public session interface exposes the begin retry delay only; exchange/snapshot retry deadlines remain private, so the UI shows their public error without inventing a countdown. Those are transport-owner and engine-verification follow-ups, not completed gameplay claims.
