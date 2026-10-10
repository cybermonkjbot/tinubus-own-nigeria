#pragma once

#include "BaddieCompanionMovementPresentation.h"

namespace BaddieCompanion
{
    // Source metres/radians. The owner supplies the current scope and sequence.
    struct FMovementRequest
    {
        int64 Sequence = 0;
        int64 RoomRevision = 0;
        FMovementPose Destination;
        bool bStop = false;
        EMovementPace Pace = EMovementPace::Unpaced;
        // Only companionWorld:move accepts these fields, not chatCallSpaces:move.
        TOptional<TArray<FMovementPoint>> Path;
        TOptional<FMovementCollisionStop> Collision;
    };

    struct FCallMovementResult
    {
        bool bOk = false;
        // A declined first request can report sequence zero and no route.
        int64 Sequence = 0;
        int64 RoomRevision = 0;
        FMovementPose Position;
        TOptional<FMovementRoute> Movement;
        double RetryAfterMs = 0;
        TOptional<FString> Reason;
    };

    enum class EWorldPlayerStatus : uint8 { Present, Traveling, Recovering };

    // Receipt values retain the shared source's +Y-up metres, radians and server time.
    // These are data only, not Unreal transforms or another vehicle simulation.
    struct FWorldVehicleVector
    {
        double X = 0, Y = 0, Z = 0;
    };
    struct FWorldDrivingPose
    {
        FWorldVehicleVector Position;
        double YawRadians = 0;
    };
    struct FWorldDrivingOccupant
    {
        // This type represents the wire's only supported role/seat: driver, zero.
        FString UserId, CharacterId, LifeId;
    };
    struct FWorldDrivingContact
    {
        FString Id, BodyId;
        int64 AtMs = 0;
        double Speed = 0;
        FWorldDrivingPose Pose;
    };
    struct FWorldVehicleWheel
    {
        double AngularVelocity = 0, Angle = 0, Steering = 0, Compression = 0;
        double NormalLoad = 0, SlipRatio = 0, SlipAngle = 0;
        double LongitudinalForce = 0, LateralForce = 0;
        bool bGrounded = false;
    };
    struct FWorldVehicleDynamics
    {
        // Only wire version 1 is decoded. No missing mechanical state is synthesized.
        int32 Version = 1;
        FWorldVehicleVector Position, Velocity, AngularVelocity, OrientationXYZ;
        double OrientationW = 0;
        double Steering = 0, EngineOmega = 0, ShiftRemaining = 0;
        int64 Gear = 0;
        TArray<FWorldVehicleWheel> Wheels;
    };
    enum class EWorldDrivingMode : uint8 { Driving, Stopped, PulledOver, Suspended };
    enum class EWorldDrivingSurface : uint8 { CleanAsphalt, LateriteMud, Pothole, GutterWood };
    struct FWorldDrivingControl
    {
        FString Id, VehicleId, GameRoomId, InstanceId, NavigationRevision, RouteId;
        int64 VehicleRevision = 0, Revision = 0, Sequence = 0;
        FWorldDrivingOccupant Driver;
        TArray<FWorldDrivingOccupant> Occupants;
        FWorldDrivingPose Pose;
        double Speed = 0, Throttle = 0, Brake = 0, Steering = 0;
        double DrivenMetres = 0, FuelMetres = 0;
        int64 AcceptedAtMs = 0, InputExpiresAtMs = 0;
        EWorldDrivingMode Mode = EWorldDrivingMode::Stopped;
        EWorldDrivingSurface Surface = EWorldDrivingSurface::CleanAsphalt;
        TOptional<FWorldDrivingContact> Contact;
        TOptional<FWorldVehicleDynamics> Dynamics;
    };

    enum class EWorldWaterPhase : uint8 { Active, Landed };
    enum class EWorldWaterFeedback : uint8 { Clear, Hyacinth, Snagged, Blocked };
    struct FWorldWaterScope
    {
        FString GameRoomId, InstanceId, PlaceId, LayoutId, LayoutRevision;
        FString UserId, CharacterId, LifeId;
        int64 Revision = 0;
    };
    struct FWorldWaterSnag
    {
        FString ZoneId;
        double ExposureMs = 0;
        double ClearMs = 0;
        bool bHeld = false;
    };
    struct FWorldWaterControl
    {
        FString Id, CraftId, DomainId;
        FWorldWaterScope Scope;
        int64 CraftRevision = 0;
        int64 Sequence = 0;
        EWorldWaterPhase Phase = EWorldWaterPhase::Active;
        // Shared source coordinates: metres, +Y up, X/Z water plane; no UE conversion.
        double X = 0, Y = 0, Z = 0, YawRadians = 0;
        FMovementPoint Velocity;
        int64 SimulatedAtMs = 0, StartedAtMs = 0, ExpiresAtMs = 0, EndsAtMs = 0, LastInputAtMs = 0;
        double Throttle = 0, Steer = 0;
        bool bBrake = false;
        TOptional<FWorldWaterSnag> Snag;
        EWorldWaterFeedback Feedback = EWorldWaterFeedback::Clear;
    };

    struct FWorldMovementResult
    {
        // Scope identifiers are private process-memory data, not proof of an admitted visit.
        FString UserId, CharacterId, GameRoomId, PlaceId, InstanceId, HomeRoomId;
        int64 Revision = 0;
        int64 MoveSequence = 0;
        int64 ServerNowMs = 0;
        FMovementPose Position;
        TOptional<FMovementRoute> Movement;
        EWorldPlayerStatus Status = EWorldPlayerStatus::Present;
        double Health = 0;
        // Missing: false/unset. Explicit null: true/unset. Object: true/set.
        // None of these states alone establishes who owns movement.
        bool bHasDrivingField = false;
        TOptional<FWorldDrivingControl> Driving;
        bool bHasWaterControlField = false;
        TOptional<FWorldWaterControl> WaterControl;
        // Retain the full player document, including omitted versus null control state.
        // Driving/public water controls are decoded; journey/transit/activity remain opaque.
        // No control state is admitted here, and private water session/landing fields are rejected.
        // No logging, Blueprint, SaveGame or persistent storage of this document.
        FString PlayerDocumentJson;
    };

    // Encode only movement arguments with UE JSON. No credentials, lease IDs,
    // game-room identity or network request are created here. Call encoding rejects
    // world-only fields rather than silently dropping a proposed path or receipt.
    // The admitted transport must add its own scope and enforce lifecycle rules.
    BADDIECOMPANION_API bool EncodeCallMovementRequest(
        const FMovementRequest& Request, TSharedPtr<FJsonObject>& Out, FString& OutError);
    BADDIECOMPANION_API bool EncodeWorldMovementRequest(
        const FMovementRequest& Request, TSharedPtr<FJsonObject>& Out, FString& OutError);

    // Input must already be size-bounded and decoded by the transport. Preserves
    // server timestamps and declines; success means a decoded result, not bOk,
    // admission or acceptance in the current scope. Clears Out on failure.
    BADDIECOMPANION_API bool DecodeCallMovementResult(
        const TSharedPtr<FJsonObject>& Object, FCallMovementResult& Out, FString& OutError);

    // Input must already be size-bounded and decoded by the transport.
    // World movement returns player/serverNow, not ok/retryAfterMs. Decode movement fields
    // and driving/public water-control fields, retaining the complete player document. Other
    // nested player schemas need their own adapters. Scope/life/layout, occupancy, freshness,
    // lease, vehicle configuration and domain acceptance belong to the admitted gameplay owner.
    // An older moveSequence can be a soft decline, and a stop may return a vehicle-owned
    // player unchanged. Neither a decoded result nor null movement grants on-foot control.
    // The owner must enforce generation, visit/instance, revision and controller ownership
    // before accepting the result or updating the world presentation clock. Clears Out on failure.
    BADDIECOMPANION_API bool DecodeWorldMovementResult(
        const TSharedPtr<FJsonObject>& Object, FWorldMovementResult& Out, FString& OutError);
}
