# Unreal Teams command review

October 11, 2026. Source acceptance only.

## Fixed scope

The full three-path review is `2b12d3338fcc77f17257d06414332a417bb702f0...7900099f44c8e08fbe447fe750cc74a171867f90`. Source is `1f6c60f90d01cf0ea1eb1e8bb9a6cd7f65ef8bfc`; handoff is `7900099f44c8e08fbe447fe750cc74a171867f90`. It contains 689 additions and three removals in TeamsState.h/.cpp and `unreal-teams-command-handoff-2026-10-10.md`. Parent resolved the references and matched all three current working hashes to the reviewed head.

The spec is `unreal-teams-command-assignment-2026-10-10.md` at `8a246b5bd9f1cc3c762f0d363fb22891035447d4`, with canonical references pinned to the base above. The owner completed turn `01a1280f-ebf3-78d1-a1c9-f8b7bd26fd30`. Reviews cover both commits and the complete source, not a selected command or documentation increment.

The source extends the existing owner with five immutable command packets, complete admission/result decoding, notification continuation, muted call attachment, matching-call leave, bounded cleanup and original-owner finalization. It uses complete unchanged StateJson from `b4bc1b7d7dbc9552ad1451062a404fc872a13224`.

## Standards

Independent reviewer `01a1281d-19f9-7973-bd05-23f6405f79ed` found zero documented breaches and one optional naming judgment. It reviewed AGENTS.md, CONTEXT.md, assignment ownership/reuse rules, the complete range and pinned canonical references.

At TeamsState.h line 160, the action receipt has `TSharedPtr<uint8> Stage;`. TeamsState.cpp line 488 compares `Token.Work->Action != Token.Stage`, then uses `const auto Stage = Token.Work->Stage;`. The first name means action identity and the second means lifecycle enum. Naming the identity fields ActionIdentity could make the distinction easier to read.

This is a low-priority Mysterious Name judgment, not a documented breach or defect in shared-identity ownership. It is deferred without changing the accepted source. The reviewer is closed.

## Spec

Independent reviewer `01a1281d-1a78-7e72-b83d-1915817ddbf9` returned: "Spec: 0 findings across the full pinned three-path range." The reviewer is closed.

The assigned review covered the full canonical command and response behavior, after-notification selectedAtStart, exact stage receipts, same/null/different call cases, original controller lifetime, disposal before/after admission and attachment, bounded cleanup, explicit leave, legacy overlap and finalization reentry. No correction is requested for the accepted range.

## Acceptance and limits

Both axes accept these pins. Standards has zero documented breaches and one deferred naming judgment. Spec has zero findings. Both reviewers are closed; do not restart their accepted review.

Windows delivery remains pending and must wait for the existing Economy/POS transfer to finish. A later batch must include complete Teams and StateJson pairs, matching dependencies, this record, assignment and handoff, with independently verified destination preimages. The coordinator alone transfers to Windows.

The backend server-owned-city leave branch can release the current guest without checking startedAt first. Captured room/start arguments do not prove replacement-reservation isolation, and the source review does not approve that backend behavior. Missing native transport, call observation/controller producers and permissions remain explicit. Account/readiness gates and world-control interlock stay disabled.

No tests, lint, typechecks, compilation, engine/content work, live calls, configuration or release ran. Source acceptance, Windows delivery, imported content, compilation, in-game inspection and release remain separate.

## Parent-matched hashes

```text
392e158eaf269fe40713dd2827fe2918374bd153a342fb5d51c781cc3eb8b5e4  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionTeamsState.h
492957967e12fa36fc3f089b41b43d63911cd3c3b768d3c6120fd9aefa2505d5  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionTeamsState.cpp
b0af4f4f982fcb16a9c3398835f5e8357c116c19021cac5bd35ffd10f2532424  docs/engineering/unreal-teams-command-handoff-2026-10-10.md
```

