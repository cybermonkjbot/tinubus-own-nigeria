#pragma once

#include "CoreMinimal.h"

namespace BaddieCompanion
{
    enum class EMovementPace : uint8 { Unpaced, Walk, Run, Fast };

    // Retain the shared game's metres, X/Z plane and yaw radians until rendering.
    struct FMovementPoint
    {
        double X = 0;
        double Z = 0;
    };

    struct FMovementPose
    {
        double X = 0;
        double Z = 0;
        double YawRadians = 0;
    };

    struct FMovementRoute
    {
        int32 Version = 1;
        int64 Sequence = 0;
        int64 RoomRevision = 0;
        // The receiving owner must put StartedAtMs and sampling times on one clock.
        double StartedAtMs = 0;
        FMovementPose From;
        TOptional<double> InitialSpeedMps;
        EMovementPace Pace = EMovementPace::Unpaced;
        TArray<FMovementPoint> Path;
        double YawRadians = 0;
    };

    struct FMovementSample
    {
        FMovementPose Position;
        bool bMoving = false;
        double SpeedMps = 0;
    };

    struct FMovementCollisionStop
    {
        int64 Sequence = 0;
        double ElapsedMs = 0;
    };

    BADDIECOMPANION_API double MovementPaceSpeed(EMovementPace Pace);
    BADDIECOMPANION_API double MovementPaceAcceleration(EMovementPace Pace);

    /**
     * Presentation port of packages/utils/src/companionMovement.ts. These functions
     * sample an existing route; they do not plan it, authorize it or move an actor.
     * The owner must first establish admission, room revision and route authority.
     * No current read-only Companion grant supplies or authorizes gameplay routes.
     * Keep this shared server-timing contract separate from Unreal Character
     * Movement, collision and animation. It is not a replacement movement system.
     * Invalid numeric/route input returns false and clears the output.
     */
    BADDIECOMPANION_API bool SampleMovement(
        const FMovementRoute& Movement, double NowMs, FMovementSample& OutSample);

    BADDIECOMPANION_API bool MovementEndsAt(
        const FMovementRoute& Movement, double& OutEndsAtMs);

    // Includes corners crossed between frames, not just the two sampled endpoints.
    BADDIECOMPANION_API bool MovementWindow(
        const FMovementRoute& Movement, double FromAtMs, double ToAtMs,
        TArray<FMovementPose>& OutPositions);

    // Presentation of the shared two-second receipt bound, not a server acceptance.
    BADDIECOMPANION_API bool MovementCollisionStopPosition(
        const FMovementRoute& Movement, const FMovementCollisionStop& Collision,
        double NowMs, FMovementPose& OutPosition);
}
