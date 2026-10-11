# Football and Racing command source review

October 11, 2026, Europe/London. Source acceptance and Windows delivery remain separate.

## Fixed scopes

### Football

Complete corrected three-path range `c9ee8d539835c87a87308b074759bc1a5896bee2...1d191fdc01f6b8ac7916a3cd1025abc812acffa7`, 352 additions and 11 removals. The initial range ended at `a6a2dd7e049dc71553962600bbbce791bcd5e035`. Correction source is `a0bd9dc4dae65f18c83d3af1f8eed7e700ddac87`, handoff `1d191fdc01f6b8ac7916a3cd1025abc812acffa7`, completed in owner turn `01a12841-3c6a-76b1-8684-e0efb008ef68`. Specification: `docs/engineering/unreal-football-command-assignment-2026-10-11.md`. Scoped commits: `12463a3b31f13acd90ddecfedb1a19c73f65ff6e Add native Football match command ownership`; `a6a2dd7e049dc71553962600bbbce791bcd5e035 Document Football match command ownership handoff`.

The scope is the existing FootballState.h/.cpp pair and `docs/engineering/unreal-football-command-handoff-2026-10-11.md`. Parent inspected the full source diff and handoff, matched all three working files to the endpoint commit, and matched all ten assignment preimages/references. All eight read-only canonical/dependency files remain unchanged. Neither review is limited to an incremental extraction.

### Racing

Complete three-path range `f2ce043fbe686cfbbf246e6ea0ab0f4e35368d50...d4ea778966413698918b3781974fbe52f41357cd`. Specification: `docs/engineering/unreal-racing-command-codec-assignment-2026-10-10.md`. Scoped commits: `e55e6fe4d51b9a01a7c633b9eeda2eec77b83ba0 Add pure racing command packets and endpoint response codec`; `d4ea778966413698918b3781974fbe52f41357cd Document racing command codec contract and missing world owner`.

The scope is the existing RacingState.h/.cpp pair and `docs/engineering/unreal-racing-command-codec-handoff-2026-10-10.md`. Parent inspected the full source diff and handoff, matched all three working files to the endpoint commit, and matched all ten assignment preimages/references. All eight read-only canonical/dependency files remain unchanged. Neither review is limited to an incremental extraction.

## Football Standards

Reviewer `01a1283e-436f-7d70-b1fe-796fe3999526` found one P2 documented-standard breach and zero optional judgments. Completion methods read caller-owned receipt/success references again after synchronous notifications. A listener can replace those values with a newly dispatched request, letting the old cleanup retire the new command and clear its busy latch. This violates the assignment's replacement-command isolation rule and overstates the protection described in the original handoff.

The original reviewer accepted the full corrected range with zero remaining or new documented-standard breaches and zero optional judgments. All three completion paths copy the entering receipt before notification; the reducer already receives its snapshot by value. The handoff now acknowledges the missed alias case and accurately describes the correction. This axis accepts the corrected range. The reviewer is closed.

## Football Spec

Reviewer `01a1283e-42d0-7633-a336-0088f391d06d` independently found one P2 incorrect implementation. `AcceptCommandFailure`, the malformed-success path in `BeginCommandSuccess`, and `AcceptCommandSuccess` cleanup must retain the entering identity across notification. The assignment requires that stale completion never clear a newer owner's busy/error. No additional missing requirement or scope creep was found.

The same owner completed the bounded correction. The original reviewer accepted the full corrected range and all four scoped commits with zero remaining or new findings. Packet construction, admission boundaries, ordering, retained data, notification reentry, abandonment, duplicate rejection and session retirement remain within the specification. The canonical stale-generation busy limitation is preserved. This axis accepts the corrected range. The reviewer is closed.

## Racing Standards

Reviewer `01a1283e-44c0-73f3-80cf-e2c1b53de829` found zero documented-standard breaches and zero heuristic findings. The complete three-path change follows AGENTS.md, the assignment, Unreal-first guidance and neighboring native conventions. Unreal JSON and shared input predicates are reused without introducing world authority. This axis accepts the pinned range. The reviewer is closed.

## Racing Spec

Reviewer `01a1283e-45a4-71e2-88b4-05c952716ea1` found zero missing/partial requirements, zero scope creep and zero incorrectly implemented requirements. All five packet shapes, exact-value bounds, outer/inner response separation, atomic failure clearing and retained reader behavior match the assignment. The handoff's missing-world-owner description and hashes match source. This axis accepts the pinned range. The reviewer is closed.

## Acceptance boundary

Both slices are accepted as source, with zero outstanding findings on either axis. All four reviewers are closed. Parent matched the corrected Football outputs and unchanged Racing outputs to their complete review endpoints. The combined Windows handoff can proceed through the coordinator only after payload, dependency and preimage pins are recorded. None of these reviews ran tests, compilation or execution-based validation.

The parent preflight at `2026-10-10T23:56:29.1044682Z` matched all twelve planned payload/dependency preimages and committed blobs, including five absent documents, with stable Windows HEAD `d187169ac86f879e1cb13ab544bb1b81f9406539`. Evidence is `output/cribtest11-football-racing-commands-2026-10-11/parent-preflight.json`. This read is not a transfer or full live protection audit. The coordinator must recheck immediately before any later write.

A later batch must include complete Football/Racing and matching StateJson pairs, both command assignments/handoffs, this acceptance record and Build.cs as a read-only dependency. The helpers remain unchanged from `b4bc1b7d7dbc9552ad1451062a404fc872a13224`. The coordinator alone writes Windows. All previous completed deliveries remain intact and must not be replayed.

Football opening/input/pulse and Racing world command ownership remain unfinished. Local packets, snapshots and receipts grant no gameplay permission. Account/readiness gates and the world-control interlock remain disabled. Imported content, compilation, native gameplay inspection and release remain unverified.

## Parent-matched Football correction outputs

```text
09cfe09acdf1c51665cc039e0af3255dffbed8940214222ba8b15cef836d5062  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionFootballState.h
0ec9919bb4633680562992ce542df67102f4dc17e11890600a2484e7a7e83bc2  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionFootballState.cpp
f60b41e8513be6359ee63828d724da296b3176c6f4ca60ca4b67b8b35a2b3ad6  docs/engineering/unreal-football-command-handoff-2026-10-11.md
```

## Parent-matched original outputs

These hashes record the initial reviewed source, not any later correction.

```text
09cfe09acdf1c51665cc039e0af3255dffbed8940214222ba8b15cef836d5062  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionFootballState.h
9656955c32512c43f8b4d227a788b2d6ccb7e4e967ee152f1d7fbf00f69b53de  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionFootballState.cpp
96cbb915888c851aa596c290ba3214b1add82ec46529444a3256e986105228ab  docs/engineering/unreal-football-command-handoff-2026-10-11.md
b833429fc84d23f0c8de3300500e9d5fbe195e67b94732e477d6cb3543bad1b8  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionRacingState.h
378d62c713365178835ee67c86fcbc631ec72d02735b2dcf6150c8689c4e1d4b  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionRacingState.cpp
572309cc15fffe11b42fe624b1210734446df9617e17ffd1483868c0eed8ed74  docs/engineering/unreal-racing-command-codec-handoff-2026-10-10.md
```

