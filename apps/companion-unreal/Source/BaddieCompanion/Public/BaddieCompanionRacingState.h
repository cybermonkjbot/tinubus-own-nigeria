#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonValue.h"

namespace BaddieCompanion
{
    enum class ERaceTransmission : uint8 { Automatic, Manual };
    enum class ERacePhase : uint8 { Lobby, Countdown, Racing, Finished, Cancelled };
    enum class ERaceCarStatus : uint8 { Ready, Racing, Finished, Withdrawn };
    enum class ERacingEligibility : uint8 { Eligible, NoCar, CarAway, CarBusy, NoFuel, Damaged };

    // Received or caller-supplied input. These fields provide no control grant.
    struct FRaceInput
    {
        double Throttle = 0, Brake = 0, Steering = 0;
        bool bHandbrake = false;
        int32 Gear = 1;
    };
    struct FRaceCar
    {
        FString UserId, Name, VehicleId;
        ERaceTransmission Transmission = ERaceTransmission::Automatic;
        // Source metres, radians and metres/second. No Unreal coordinate conversion.
        double X = 0, Z = 0, Yaw = 0, Speed = 0;
        // The car's gear and sequence are finite wire numbers. Only Input.Gear has
        // the canonical receive-side integer range 1..6; command sequence rules differ.
        double Gear = 0, Sequence = 0;
        FRaceInput Input;
        double InputAtMs = 0, SimulatedAtMs = 0;
        // Missing is allowed; a present null is invalid. Signed safe-integer milliseconds.
        TOptional<int64> RecoveredAtMs;
        double Distance = 0, Progress = 0;
        int32 NextCheckpoint = 0, Laps = 0;
        double LapStartedAtMs = 0;
        // Required nullable finite milliseconds, without new positivity constraints.
        TOptional<double> LastLapMs, BestLapMs, FinishedAtMs;
        ERaceCarStatus Status = ERaceCarStatus::Ready;
    };
    struct FRaceState
    {
        FString Id, InstanceId, HostId;
        ERacePhase Phase = ERacePhase::Lobby;
        // Required nullable signed safe-integer milliseconds. UpdatedAt has the same
        // numeric range, with no local clock, epoch conversion or monotonic-time inference.
        TOptional<int64> StartsAtMs, EndsAtMs;
        int64 UpdatedAtMs = 0;
        TArray<FRaceCar> Cars;
    };
    struct FRacingSelectedCar
    {
        FString VehicleId;
        double Revision = 0, FuelUnits = 0, Condition = 0;
        bool bHere = false;
    };
    struct FRacingSnapshot
    {
        // Both fields are required nullable on the wire. Neither may be omitted.
        TOptional<FRaceState> Race;
        TOptional<FRacingSelectedCar> Car;
        ERacingEligibility Eligibility = ERacingEligibility::NoCar;
    };

    enum class ERacingReadResult : uint8
    {
        Accepted, RetainedOlderTick,
        Absent, NullField, NullDocument,
        InvalidField, InvalidInstance, InvalidDocument, InstanceMismatch
    };

    // Pure readRacingSnapshot translation. RacingJson is the racingJson field from an
    // already-bounded world/command envelope, NOT the snapshot object itself. An invalid
    // shared pointer means absent; JSON null and a string containing null are separate
    // outcomes. Other non-string fields are invalid. Strings are limited to 32,768 chars.
    //
    // ExpectedInstanceId is the caller's accepted current player's nonempty instance.
    // Previous must be a validated result from this function, retained only by the actual
    // account/visit owner. Clear it on session/access/life/visit loss before calling. No
    // instance string, selected-car eligibility or successful decode grants authority.
    //
    // Incoming data is fully validated BEFORE retaining an older tick. Prior race instance
    // and updatedAt alone govern retention; race IDs need not match. Retention returns the
    // entire previous snapshot. Equal ticks and a null incoming race replace it.
    //
    // Accepted/RetainedOlderTick produce a nonnull Out. All other outcomes clear Out,
    // matching the canonical null racing value. Invalid outcomes return fixed public text;
    // absent/null outcomes have no error. Shared inputs and instance are passed by value
    // so Previous may alias Out. The caller owns generations, clocks and publication.
    BADDIECOMPANION_API ERacingReadResult ReadRacingSnapshot(
        TSharedPtr<FJsonValue> RacingJson, FString ExpectedInstanceId,
        TSharedPtr<const FRacingSnapshot> Previous,
        TSharedPtr<const FRacingSnapshot>& Out, FString& OutError);

    struct FRacingCommandScope
    {
        // Structural bounds only, not Convex ID validation or accepted-visit identity.
        FString GameRoomId, InstanceId;
        int64 Revision = 0;
    };
    struct FRacingJoinCommand
    {
        FString VehicleId;
        int64 VehicleRevision = 0;
        ERaceTransmission Transmission = ERaceTransmission::Automatic;
    };
    struct FRacingStartCommand { FString RaceId; };
    struct FRacingLeaveCommand { FString RaceId; };
    struct FRacingRecoverCommand
    {
        FString RaceId;
        int64 Sequence = 0;
    };
    struct FRacingInputCommand
    {
        FString RaceId;
        int64 Sequence = 0;
        FRaceInput Input;
    };
    struct FRacingCommandPacket
    {
        FString Endpoint, ArgumentsJson;
        // Adapter policy, not wire fields or dispatch permission.
        bool bRetry = false, bInvalidateQueryCache = false;
    };
    // Pure codecs. Strings retain case/whitespace. No credentials, admission or sequence allocation.
    // Failure clears the packet and returns fixed public text. Success clears OutError.
    BADDIECOMPANION_API bool EncodeRacingCommand(const FRacingCommandScope& Scope, const FRacingJoinCommand& Command,
        FRacingCommandPacket& Out, FString& OutError);
    BADDIECOMPANION_API bool EncodeRacingCommand(const FRacingCommandScope& Scope, const FRacingStartCommand& Command,
        FRacingCommandPacket& Out, FString& OutError);
    BADDIECOMPANION_API bool EncodeRacingCommand(const FRacingCommandScope& Scope, const FRacingLeaveCommand& Command,
        FRacingCommandPacket& Out, FString& OutError);
    BADDIECOMPANION_API bool EncodeRacingCommand(const FRacingCommandScope& Scope, const FRacingRecoverCommand& Command,
        FRacingCommandPacket& Out, FString& OutError);
    BADDIECOMPANION_API bool EncodeRacingCommand(const FRacingCommandScope& Scope, const FRacingInputCommand& Command,
        FRacingCommandPacket& Out, FString& OutError);

    struct FRacingCommandResponse
    {
        // On success, exactly a raw string or explicit JSON null. Never a parsed snapshot.
        // Pass to ReadRacingSnapshot only after the real world owner accepts the callback.
        TSharedPtr<FJsonValue> RacingJson;
        double ServerNowMs = 0;
    };
    // Endpoint object only, not an HTTP/Convex transport envelope. Requires exactly racingJson/serverNow.
    // Outer limits: 262,144 characters, depth 4, 16 separators. Inner content is not validated here.
    // serverNow accepts any finite wire number; this function never applies a clock or publishes state.
    // Failure clears Out and returns fixed public text. Success clears OutError.
    BADDIECOMPANION_API bool DecodeRacingCommandResponse(const FString& Json, FRacingCommandResponse& Out, FString& OutError);
}
