# Unreal driving-control decoding

October 10, 2026. Source implementation only.

The existing `DecodeWorldMovementResult` now decodes `player.driving` into `FWorldDrivingControl`. Its presence flag and optional value distinguish omitted, explicit null and a supplied control, matching the water field's existing convention. Failure clears the entire output and returns a fixed diagnostic. The decoder creates no command, movement, predicted state or authority.

The typed receipt retains the supplied driver and occupant identities independently. It requires the current contract's one occupant, literal driver role and seat zero, but does not substitute the driver's identity for a different supplied occupant. The admitted owner must still check both against the current player and life. Source XYZ positions, yaw, signed speed, input, distance/fuel, revisions, sequence and server times are retained without coordinate conversion, clamping or clock advancement. Driving counters use safe integers, not the separate on-foot sequence limit. Modes and surfaces retain all currently declared variants.

`contact` is required and nullable. A supplied contact retains its ID, body ID, time, speed and full pose. `dynamics` is optional but not nullable. Omission remains omission; neither the decoder nor the retained document receives invented chassis or wheel values. A supplied version-one object retains position, quaternion, velocity, angular velocity, steering, engine speed, gear, shift timing and all wheel fields in source order.

Current player driving is keke-only. `companionDrivingDynamicsConfig` defines its three wheel slots, so this decoder rejects other counts instead of truncating or reordering them. It does not normalize orientation, derive pose from the centre of mass, infer grounded state, integrate tires or construct a Chaos vehicle. Those would be presentation or mechanics work, not wire decoding. Native vehicle integration must reuse Unreal physics while retaining the existing server's authority.

## Remaining acceptance

The existing client and authoritative driving owners still define player/life/visit identity, navigation and route matching, vehicle revision/configuration, speed and mechanical limits, fuel policy, conflicting occupancy, receipt ordering and lifecycle suspension. Those checks require the current instance, vehicle and previous receipt, which this decoder does not possess. No returned control or null field grants on-foot movement, driving, a seat or an outcome. The water decoder's private-field rejection remains unchanged.

No runtime source calls `DecodeWorldMovementResult` yet. This continuation prepares the existing transport interface for that integration. Native vehicle assets, control input, possession, camera, movement acceptance and rendered inspection remain unfinished. It does not implement bush wading, squad commands or combat.

## Source review

The parent compared the implementation with `packages/utils/src/companionPlayerDriving.ts`, `packages/utils/src/companionPlayerDrivingMotion.ts`, `packages/utils/src/companionVehicleDynamics.ts`, `apps/backend/convex/lib/companionPlayerDrivingSchema.ts` and `packages/api/src/companion-world-street.ts`. The header owns the full native field list; these sources own the game rules. The existing solver and admission logic remain authoritative; this decoder only checks the wire contract.

Review covers omitted/null/object driving, required nullable contact, omitted versus null dynamics, literal seat/role, complete finite vector/quaternion/wheel fields, safe integer precision, enum spelling and failure-output clearing. The existing Unreal JSON helpers perform typed reads. Tests, compilation and runtime checks were not run. No new module dependency, production mobile/web renderer change, account enablement or engine operation is part of this patch. Windows delivery remains a separate handoff.
