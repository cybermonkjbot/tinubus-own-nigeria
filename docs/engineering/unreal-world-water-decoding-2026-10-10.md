# Unreal public water-control decoding

October 10, 2026. Source implementation only.

## Scope

The existing `DecodeWorldMovementResult` now decodes the public `player.waterControl` object into `FWorldWaterControl`. It retains scope identities, craft/domain identity, revision and sequence, source XYZ position, yaw, X/Z velocity, all server timestamps, input, nullable snag and feedback. Source units and time remain unchanged.

An omitted field remains `bHasWaterControlField == false` with no value. Explicit null has the flag true with no value. A decoded object has the flag true and a value. This preserves the wire distinction; none of these states establishes on-foot or boat control.

The decoder requires complete typed fields, finite scalar values, exact three-value point and two-value velocity arrays, safe nonnegative integer counters/timestamps, supported phase/feedback values and bounded input. Snag must be present as null or a complete object. Both active and landed states are retained. Water counters use the safe-integer bound, not the separate on-foot request sequence limit.

`sessionId` and `landingReceipt` are storage-only water fields. Their presence rejects the result before retaining the player JSON, including when their values are null. A failed decode clears the output and returns a fixed error without copying private payload values into diagnostics. Other player-domain envelopes retain their existing handling.

## Native reuse and remaining work

This extends the existing MovementProtocol module and uses its Unreal JSON and optional-value types. It adds no endpoint, input command, tick, simulated clock, physics loop, lease renewal, actor movement or asset loading.

The admitted gameplay owner still needs the equivalent of `admitCompanionWorldControls`: current scope/life/layout checks, occupancy conflict checks, receipt ordering, timestamp/lease and domain acceptance, and stale-response handling. Decoded water state must never be used as acceptance. Road driving and its optional dynamics remain opaque in the retained player document.

This is the existing CMS boat-control contract. It is not the new expedition's waist-deep wading, muddy-gully slowdown, lowered weapon pose or movement-noise implementation. Those remain in the rural expedition brief. Native craft rendering, input ownership and gameplay admission also remain unfinished.

## Source and verification

The parent read these existing sources for this implementation:

- `packages/utils/src/companionWaterTraversal.ts`, public control/scope/input types and source coordinate conventions.
- `apps/backend/convex/lib/companionWorldWaterSchema.ts`, complete public control and separate storage fields.
- `apps/backend/convex/lib/companionWorldWater.ts`, `worldWaterPublicControl` excludes session and terminal receipt.
- `packages/api/src/companion-world-street.ts`, control admission and freshness remain separate from decoding.
- `packages/utils/src/companionWorld.ts`, optional/null player control.
- The existing native MovementPresentation/Snapshot decoders, for Unreal JSON array and typed-value access.

Source review covers missing/null/object control, active/landed phase, numeric and tuple shape, missing/null/object snag, storage-only fields and failure-output clearing. No tests, compilation or rendered inspection ran. Account access and apartment render admission remain closed. The production mobile/web renderer is unchanged.

The Mac initially rejected the implementation write without changing either source file. The parent prepared the candidate in memory from Mac `78bd85c9ee102a6698853d6fe7af7669006e034a` for reviewed, preimage-guarded Windows delivery. After storage recovered, the parent saved the same source locally. Windows delivery and the local source checkpoint remain separate evidence.
