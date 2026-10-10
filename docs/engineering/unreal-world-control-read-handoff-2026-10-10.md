# Native transport receipt reconciliation

October 10, 2026. Source implementation only. No transport, pawn or gameplay permission is enabled.

## Scope

The parent owns `Public/BaddieCompanionWorldControls.h`, `Private/BaddieCompanionWorldControls.cpp` under `apps/companion-unreal/Source/BaddieCompanion`, and this handoff. The WorldPlayer decoder, movement protocol, world clock, backend access implementation and character equipment remain separate owners.

`ReconcileWorldTransportRead` translates `admitCompanionWorldControls`, `drivingValid` and `waterValid` from `packages/api/src/companion-world-street.ts`. It consumes the existing bounded, decoded player and current-instance views. It returns const selected driving/water fields and position, with the user, character, game, instance and visit revision needed to compare the next receipt. It neither mutates the original player nor copies its raw control-observation flags into the selected result.

The native implementation reuses the existing UE shared-pointer and optional-value types and existing decoded controls. It creates no parser, vehicle physics loop, request, actor, ticker, plugin or module dependency.

## Read behavior

| Input or condition | Result |
| --- | --- |
| Missing player | Success with null output. |
| Missing instance, non-present player, health at or below zero, or different instance | Publish the supplied position and scope with both transport controls absent. Original player data is untouched. This does not authorize walking. |
| Missing or null driving/water field | Both normalize to absent at this read stage, matching the canonical store. The decoder still retains the original wire distinction. |
| Driving | Match game, instance, navigation, user, character and optional known life. Require exactly the decoded driver in seat zero, finite pose, safe nonnegative counters/times, finite nonnegative distance/fuel, valid input/mode/surface and speed from -7 through 7 metres per second. The read check does not substitute the stricter command/physics admission rules. |
| Water | Match full player/visit/layout scope and optional known life. Retain active versus landed. Check finite point, velocity, yaw and input; supported phase/feedback; safe nonnegative times/counters; startedAt and lastInputAt no later than simulatedAt; expiresAt no later than endsAt. The scope revision preserves the stricter water-scope integer bound. |
| Conflicting occupancy | Reject driving together with active water, or either active transport together with movement, transit occupancy or a traveling journey. Landed water alone does not count as active occupancy. |
| Older driving receipt | Within the same user, character, game, instance and visit revision, retain prior control and player position when driving ID and driver life match and any driving revision, sequence or acceptedAt regresses. A higher field does not cancel another field's regression. |
| Older water receipt | Within that same visit, retain prior control and position when control ID and every water-scope field match and any sequence, craft revision or simulatedAt regresses. |
| Different identity, visit, control or life | Do not retain the previous control. A missing new control never resurrects a prior one. |
| Equal or newer fields | Retain the supplied current control and position. This function does not advance time or interpolate a pose. |
| Invalid input or selected control | Failure with cleared output and fixed diagnostic text. No payload value is logged or returned in an error. |

The current life comes from separately accepted recovery state. An empty life string supports the canonical peer-read behavior; a control's own life value never proves the player's current life. The future local-player owner must supply its accepted current life before using transport presentation.

The prior result is passed as a shared pointer by value. It stays alive if the caller uses the same variable for prior input and output. Caller-owned output is reset before validation; partial work never publishes a result.

## Retained-control consistency

The canonical function validates incoming controls before independently retaining older driving and water receipts. This native adapter also validates the selected controls and occupancy after retention. This is a deliberate additional rejection, not a new control rule or fallback.

For example, a newer incoming driving control can coexist with a landed water record before retention, but retaining an older active water receipt would produce conflicting occupancy. Likewise, the same driving ID with regressed timing could restore a receipt for an older navigation revision. The second check rejects these combined results instead of publishing them. It does not change the production store, drop just one conflicting control, invent a new revision or claim walking permission.

## Integration still required

The future world owner must authenticate a separately consented world operation, reject stale grant/request callbacks and player revisions/sequences, validate life and exact visit ownership, and clear retained state on account/grant/life/visit loss. This helper cannot authenticate its inputs or revoke copies retained by another consumer. It performs no wall-clock expiry check, lease renewal or server-time observation.

Consume the selected controls and position together; do not combine a retained control with the incoming position or use the original decoder's control-observation flags as permissions. Domain/fleet/catalogue, mechanical configuration, contact, fuel capacity, transit capacity, route clearance, rendering readiness and full world/mission/combat ownership remain separate checks. A valid read is not command admission and cannot be used as a `CanWalk` or `CanDrive` signal.

No caller is wired yet. Backend world-control consent and ownership are being implemented separately. Entry/presence consent version one and Home/apartment grants remain insufficient. The gameplay configuration and readiness gates remain disabled.

## Source evidence

Source review traced null/missing normalization, suppressed-read ordering, all driving/water conditions, inclusive and strict integer limits, speed/input boundaries, landed-water coexistence, full scope comparison, each independent regression condition and output reset. It also traced the extra post-retention conflict/navigation check. No executable validation ran. Independent source review is pending.

Canonical source SHA-256 values:

```text
f6e9b9aa43178ab9c7d699a3c6aacff779990ce7c43b3480ab83d2c00c2a1246  packages/api/src/companion-world-street.ts
2e268765a20956c92c09f7677af35238407629225396456a594046166a9f31f6  packages/utils/src/companionPlayerDriving.ts
483ba984b92abcc58e8c396b7e881633c6e62983964b70dbfed2cffc8b0edd2f  packages/utils/src/companionPlayerDrivingMotion.ts
af9c8bcdeb150b7d7be576f4913f5e874e15359872764416c8d0f88aafdb7a3b  packages/utils/src/companionWaterTraversal.ts
```

Decoder dependencies at source review:

```text
90d230cf0233458c46324fd87d6f077c6f54bd7dd691030ba5a2a89e8607a17d  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionWorldPlayer.h
d98eb366813cad032d1e2d241367578ca20d357a5169f6e244a68b1b388af2e6  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionMovementProtocol.h
```

No tests, typechecks, lint, builds, compilation, code generation, engine launches, asset import/conversion, network calls, configuration or deployment ran for this slice. Production renderer inputs and mobile behavior are unchanged, so renderer packaging and a What's New update are not triggered. Source review, Windows delivery, compilation, in-game verification and release remain separate. The existing coordinator alone owns any later pinned Windows handoff.
