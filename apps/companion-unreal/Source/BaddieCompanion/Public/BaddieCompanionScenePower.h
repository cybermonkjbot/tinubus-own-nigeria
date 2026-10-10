#pragma once

#include "CoreMinimal.h"
#include "BaddieCompanionSnapshot.h"

class UBaddieCompanionNativeApartment;

namespace BaddieCompanion
{
    enum class EScenePowerStatus : uint8 { Unavailable, On, Outage };

    // Server-resolved household/visit identity, not a zone derived from a saved room ID.
    struct FScenePowerScope
    {
        int32 Version = 1;
        FString WorldKey, ClockEpochKey, ZoneKey;
        FString GameRoomId, InstanceId, RoomId;
        int64 Revision = 0;
        bool Matches(const FScenePowerScope& Other) const;
        bool IsSupported() const;
    };

    struct FScenePowerSample
    {
        int32 Version = 1;
        EScenePowerStatus Status = EScenePowerStatus::Unavailable;
        FString EventId;
        TOptional<FString> EpisodeId;
        int64 StartedAtRealMs = 0, EndsAtRealMs = 0, NextChangeAtRealMs = 0;
        // Keep the original observation age when the admitted owner projects a display sample.
        int64 ObservedAtRealMs = 0, SampledAtRealMs = 0;
    };

    // Presentation math only. Never samples future grid episodes or grants access.
    // False clears OutFactor to zero; unavailable is distinct from an accepted outage.
    BADDIECOMPANION_API bool ProjectScenePowerFactor(const FScenePowerSample& Sample,
        int64 SharedNowRealMs, bool bReducedMotion, float& OutFactor);

    // No public constructor, decoder, Blueprint setter or producer exists yet. The apartment
    // consumes this type; only a future admitted world owner may be given minting access.
    // Home/apartment read consent and ordinary content bindings cannot create one.
    class FAcceptedApartmentScenePower
    {
    private:
        friend class ::UBaddieCompanionNativeApartment;
        FAcceptedApartmentScenePower() = default;
        TWeakObjectPtr<UBaddieCompanionNativeApartment> Apartment;
        uint64 ContentGeneration = 0;
        uint64 PowerGeneration = 0;
        FSnapshotSelection Source;
        FScenePowerScope Scope;
        FScenePowerSample Sample;
        // Monotonic time corresponding to SampledAtRealMs, supplied once by the producer.
        double SampledAtSeconds = 0;
        bool bReducedMotion = false;
    };
}
