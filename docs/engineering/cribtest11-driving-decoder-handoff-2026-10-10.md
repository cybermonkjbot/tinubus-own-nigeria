# Cribtest11 driving-decoder handoff

October 10, 2026, 17:23 BST. Reviewed source delivery only.

## Source and review

We delivered exactly five paths from Mac checkpoint
`222855be1b30a6bd5a9e8f7ff41428b16f36bfc9`. Every payload came from that
commit's Git objects and matched the supplied SHA-256 before transfer.
The previous water delivery was not replayed.

The existing Standards and Spec reviewers found no actionable issues against
`66c5ec1598e8e93b35f56d9dfc9e194fe9fc4b4c`. They checked the canonical driving,
motion, dynamics, backend schema and world-street contracts. Review covered
complete field mapping, omitted/null driving, required nullable contact,
optional non-null dynamics, ordered three-wheel state, the literal driver seat,
independent identities, source units/time, numeric bounds and failure clearing.
Water parsing and private-field rejection remain unchanged.

This source decodes receipts. It does not admit control, advance a clock, apply
input, move an actor, simulate vehicles or add vehicle assets. Scope, life,
occupancy, freshness and physical acceptance remain with gameplay owners.

## Verified delivery

Windows moved from `9334934b78140a3bcfe720b2da77de4f3323976c` to
`01949ae314b931a00df96fe570f4a22f0a756f18` at
`2026-10-10T16:23:38.542Z`. The four existing target preimages matched Mac
`66c5ec159`; the new driving note was absent. The guarded transfer saved recovery
copies and committed only the five approved paths with hooks disabled.

A separate SSH readback at `2026-10-10T16:23:58.616Z` matched all five working
files and committed Git objects. All 92 protected paths matched before and after.
Their manifest SHA-256 remains
`3d78238992057414020c6a0ba85120ab6927aad9fb9b3406505e9b8b4a5819d2`.

| Path | SHA-256 |
| --- | --- |
| `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionMovementProtocol.h` | `d98eb366813cad032d1e2d241367578ca20d357a5169f6e244a68b1b388af2e6` |
| `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionMovementProtocol.cpp` | `ddacf3e6f68bb27946f28c03c88f867a171db120e1b608a1234470c16f0c4825` |
| `docs/engineering/standalone-companion-unreal-port.md` | `fd89397b8ba678c88eb5351de84f92099c2c5e48fe735d3549d0b05bc6006dde` |
| `docs/engineering/unreal-world-water-decoding-2026-10-10.md` | `70b941def2fcfa803b776e06562bbdbb9b56e935e2e3de6b4d88b0d86b0b696b` |
| `docs/engineering/unreal-world-driving-decoding-2026-10-10.md` | `f769edef6188f94ca90b41aabff12ab0a85ece1f654300a3c5ad1fa730c751a9` |

The two pre-existing Windows Config edits remain byte-identical and are the only
dirty paths. Editor PID `11084` and Codex PIDs `2660` and `5176` retained their
identities. Other Unreal source, setup, scripts, binaries and account-disablement
configuration were preserved. No compiler or commandlet appeared in the snapshots.

## Recovery and limits

Windows evidence is under
`C:\dev\cribtest-game\.git\operations\driving-decoder-20261010-222855be1`:
`before.json`, `manifest.json`, numbered `.before` backups and `.payload` files,
`receipt.json` and `independent-readback.json`.

Mac evidence is under
`/Users/joshua/Documents/ChatGPT/baddie.ng/output/cribtest11-driving-decoder-2026-10-10`:
`plan-222855be1.json`, `receipt-222855be1.json` and `readback-222855be1.json`.
Both local receipt mirrors were saved and read back successfully. This separate
result document was not added to the Windows payload. The pinned source notes'
pending-delivery statements describe their pre-transfer checkpoint; this record
establishes the subsequent delivery.

No tests, compilation, lint, typechecks, code generation, engine calls, launches,
restarts, imports, conversions, installs, live settings, account enablement or
deployment ran. Compilation acceptance remains at Windows
`206a331daf693eff0965cf2a4c539e54393b0c3b`; later avatar, water and driving source
is uncompiled. Gameplay and rendered acceptance remain unverified. The full game
port is not complete.
