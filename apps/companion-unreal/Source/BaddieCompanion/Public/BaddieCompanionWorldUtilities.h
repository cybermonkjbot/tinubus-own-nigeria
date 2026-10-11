#pragma once

#include "BaddieCompanionResidentState.h"

namespace BaddieCompanion
{
    // Values describe server-supplied scope; neither construction nor sampling grants access.
    struct FWorldUtilityScope
    {
        int32 Version = 0;
        FString WorldKey, ClockEpochKey;
        // Opaque server-resolved zone, never derived from a room, address or call ID.
        FString ZoneKey;
    };

    struct FWorldUtilityObservation
    {
        FWorldUtilityScope Scope;
        FResidentClock Clock;
        int64 ServerNowRealMs = 0;
        // Admitted owner's server-time watermark; never reset to accept an older frame.
        int64 NotBeforeRealMs = 0;
    };

    struct FWorldUtilityWindow
    {
        FString EventId;
        TOptional<FString> EpisodeId;
        int64 StartedAtRealMs = 0, EndsAtRealMs = 0, NextChangeAtRealMs = 0;
    };

    enum class EWorldPowerStatus : uint8 { Unavailable, On, Outage };
    enum class EWorldNetworkStatus : uint8 { Unavailable, Good, Slow };
    struct FWorldPower : FWorldUtilityWindow
    {
        EWorldPowerStatus Status = EWorldPowerStatus::Unavailable;
    };
    struct FWorldSimulatedNetwork : FWorldUtilityWindow
    {
        EWorldNetworkStatus Status = EWorldNetworkStatus::Unavailable;
    };

    struct FWorldUtilities
    {
        int32 Version = 0;
        FWorldUtilityScope Scope;
        int64 SampledAtRealMs = 0, GameMinute = 0;
        FWorldPower Power;
        FWorldSimulatedNetwork Network;
        int64 NextChangeAtRealMs = 0;
    };

    // Pure version-one math. No device clock, retained observation, scheduling or admission.
    // Both calls clear Out on failure and return a fixed diagnostic in OutError.
    BADDIECOMPANION_API bool SampleWorldUtilities(const FWorldUtilityObservation& Observation,
        FWorldUtilities& Out, FString& OutError);

    // Display only, at most 60,000 ms after the original observation, inclusive.
    // Does not mutate that observation or make the projected sample a fresh receipt.
    BADDIECOMPANION_API bool ProjectWorldUtilities(const FWorldUtilityObservation& Observation,
        int64 DisplayNowRealMs, FWorldUtilities& Out, FString& OutError);
}
