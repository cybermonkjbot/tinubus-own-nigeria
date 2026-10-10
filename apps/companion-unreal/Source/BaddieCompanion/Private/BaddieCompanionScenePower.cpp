#include "BaddieCompanionScenePower.h"

namespace BaddieCompanion::ScenePower
{
    constexpr int64 MaxSafeInteger = 9007199254740991LL;
    bool Time(int64 Value) { return Value >= 0 && Value <= MaxSafeInteger; }
    bool Key(const FString& Value, int32 Maximum)
    {
        if (Value.IsEmpty() || Value.Len() > Maximum) return false;
        for (int32 I = 0; I < Value.Len(); ++I)
        {
            const TCHAR C = Value[I];
            const bool bAlphaNumeric = (C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z')
                || (C >= '0' && C <= '9');
            if (!bAlphaNumeric && (I == 0 || (C != '.' && C != '_' && C != ':' && C != '-'))) return false;
        }
        return true;
    }
}

bool BaddieCompanion::FScenePowerScope::Matches(const FScenePowerScope& Other) const
{
    return Version == Other.Version && Revision == Other.Revision
        && WorldKey.Equals(Other.WorldKey, ESearchCase::CaseSensitive)
        && ClockEpochKey.Equals(Other.ClockEpochKey, ESearchCase::CaseSensitive)
        && ZoneKey.Equals(Other.ZoneKey, ESearchCase::CaseSensitive)
        && GameRoomId.Equals(Other.GameRoomId, ESearchCase::CaseSensitive)
        && InstanceId.Equals(Other.InstanceId, ESearchCase::CaseSensitive)
        && RoomId.Equals(Other.RoomId, ESearchCase::CaseSensitive);
}

bool BaddieCompanion::FScenePowerScope::IsSupported() const
{
    return Version == 1 && ScenePower::Time(Revision)
        && ScenePower::Key(WorldKey, 80) && ScenePower::Key(ClockEpochKey, 160)
        && ScenePower::Key(ZoneKey, 160) && !GameRoomId.IsEmpty() && GameRoomId.Len() <= 160
        && !InstanceId.IsEmpty() && InstanceId.Len() <= 256 && !RoomId.IsEmpty() && RoomId.Len() <= 80;
}

bool BaddieCompanion::ProjectScenePowerFactor(const FScenePowerSample& Sample,
    int64 SharedNowRealMs, bool bReducedMotion, float& OutFactor)
{
    using namespace ScenePower;
    OutFactor = 0;
    if (Sample.Version != 1 || (Sample.Status != EScenePowerStatus::On && Sample.Status != EScenePowerStatus::Outage)
        || !Key(Sample.EventId, 100) || !Time(SharedNowRealMs) || !Time(Sample.StartedAtRealMs)
        || !Time(Sample.EndsAtRealMs) || !Time(Sample.ObservedAtRealMs) || !Time(Sample.SampledAtRealMs)
        || Sample.NextChangeAtRealMs != Sample.EndsAtRealMs || Sample.StartedAtRealMs >= Sample.EndsAtRealMs
        || Sample.SampledAtRealMs < Sample.ObservedAtRealMs || SharedNowRealMs < Sample.SampledAtRealMs
        || Sample.StartedAtRealMs > Sample.SampledAtRealMs || SharedNowRealMs >= Sample.EndsAtRealMs
        || SharedNowRealMs - Sample.ObservedAtRealMs > 60000) return false;
    const bool bOn = Sample.Status == EScenePowerStatus::On;
    if (bOn && Sample.EpisodeId.IsSet()) return false;
    if (!bOn && (!Sample.EpisodeId.IsSet() || !Key(Sample.EpisodeId.GetValue(), 100)
        || !Sample.EventId.Equals(Sample.EpisodeId.GetValue() + TEXT(":started"), ESearchCase::CaseSensitive))) return false;
    if (bReducedMotion || (bOn && Sample.EventId.EndsWith(TEXT(":initial"), ESearchCase::CaseSensitive)))
    { OutFactor = bOn ? 1.f : 0.f; return true; }
    // Canonical absolute-time 900 ms smoothstep, sampled in 50 ms steps. No local restart.
    const int64 Elapsed = ((SharedNowRealMs - Sample.StartedAtRealMs) / 50) * 50;
    const double Progress = FMath::Min(1.0, static_cast<double>(Elapsed) / 900.0);
    const float Fade = static_cast<float>(Progress * Progress * (3.0 - 2.0 * Progress));
    OutFactor = bOn ? Fade : 1.f - Fade;
    return true;
}
