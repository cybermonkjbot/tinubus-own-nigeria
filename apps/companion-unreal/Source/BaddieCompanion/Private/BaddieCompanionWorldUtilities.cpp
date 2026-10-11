#include "BaddieCompanionWorldUtilities.h"

namespace BaddieCompanion
{
    namespace WorldUtilitiesPrivate
    {
        constexpr int64 MaxInteger = 9007199254740991LL;
        constexpr int64 MaximumDisplayAgeRealMs = 60000;
        constexpr int32 Version = 1;
        constexpr const TCHAR* UtilityRangeError = TEXT("Utility time exceeds the supported server-millisecond range.");

        struct FRules
        {
            const TCHAR* Axis;
            int64 Cycle, Minimum, Maximum, GoodMargin;
        };
        constexpr FRules PowerRules{TEXT("power"), 180, 8, 16, 30};
        constexpr FRules NetworkRules{TEXT("network"), 90, 2, 5, 20};

        bool Fail(FString& OutError, const TCHAR* Message)
        {
            OutError = Message;
            return false;
        }

        bool IntegerTime(int64 Value)
        {
            return Value >= 0 && Value <= MaxInteger;
        }

        bool Add(int64 A, int64 B, int64& Out)
        {
            if (!IntegerTime(A) || !IntegerTime(B) || A > MaxInteger - B) return false;
            Out = A + B;
            return true;
        }

        bool Multiply(int64 A, int64 B, int64& Out)
        {
            if (!IntegerTime(A) || !IntegerTime(B) || (B != 0 && A > MaxInteger / B)) return false;
            Out = A * B;
            return true;
        }

        bool AsciiAlphanumeric(TCHAR Value)
        {
            return (Value >= TEXT('A') && Value <= TEXT('Z'))
                || (Value >= TEXT('a') && Value <= TEXT('z'))
                || (Value >= TEXT('0') && Value <= TEXT('9'));
        }

        bool ValidKey(const FString& Value, int32 Maximum)
        {
            if (Value.IsEmpty() || Value.Len() > Maximum || !AsciiAlphanumeric(Value[0])) return false;
            for (int32 Index = 1; Index < Value.Len(); ++Index)
            {
                const TCHAR Character = Value[Index];
                if (!AsciiAlphanumeric(Character) && Character != TEXT('.') && Character != TEXT('_')
                    && Character != TEXT(':') && Character != TEXT('-')) return false;
            }
            return true;
        }

        bool SampleGameMinute(const FResidentClock& Clock, int64 Now, int64& Out, FString& OutError)
        {
            if (Clock.Version != Version || !ValidKey(Clock.WorldKey, 80) || !ValidKey(Clock.EpochKey, 160)
                || !IntegerTime(Clock.EpochRealMs) || !IntegerTime(Clock.EpochGameMinute)
                || !IntegerTime(Clock.RealMsPerGameMinute) || Clock.RealMsPerGameMinute == 0
                || !IntegerTime(Now) || Now < Clock.EpochRealMs)
                return Fail(OutError, TEXT("Provide a supported shared world epoch, positive time scale and a timestamp at or after that epoch."));
            const int64 Elapsed = Now - Clock.EpochRealMs;
            const int64 Remainder = Elapsed % Clock.RealMsPerGameMinute;
            const int64 ElapsedMinutes = (Elapsed - Remainder) / Clock.RealMsPerGameMinute;
            if (!Add(Clock.EpochGameMinute, ElapsedMinutes, Out))
                return Fail(OutError, TEXT("Shared game time exceeds the safe integer range."));
            return true;
        }

        bool ValidateObservation(const FWorldUtilityObservation& Observation, int64& GameMinute, FString& OutError)
        {
            const auto& Scope = Observation.Scope;
            const auto& Clock = Observation.Clock;
            if (Scope.Version != Version || !ValidKey(Scope.ZoneKey, 160)
                || !Scope.WorldKey.Equals(Clock.WorldKey, ESearchCase::CaseSensitive)
                || !Scope.ClockEpochKey.Equals(Clock.EpochKey, ESearchCase::CaseSensitive)
                || !IntegerTime(Observation.NotBeforeRealMs) || Observation.NotBeforeRealMs < Clock.EpochRealMs
                || Observation.ServerNowRealMs < Observation.NotBeforeRealMs)
                return Fail(OutError, TEXT("Provide the admitted utility zone, current epoch and non-stale server observation."));
            return SampleGameMinute(Clock, Observation.ServerNowRealMs, GameMinute, OutError);
        }

        // Only validated ASCII keys and JSON assembled from them reach these helpers.
        // Their TCHAR values therefore equal the canonical UTF-16 code units.
        uint32 Hash(const FString& Text, uint32 Seed = 2166136261u)
        {
            uint32 Value = Seed;
            for (int32 Index = 0; Index < Text.Len(); ++Index)
                Value = static_cast<uint32>(static_cast<uint64>(Value ^ static_cast<uint32>(Text[Index])) * 16777619u);
            return Value;
        }

        // Restricted JSON string encoding: the internal seed adds quotes/backslashes,
        // but admitted keys cannot contain controls or non-ASCII characters.
        FString QuoteSeed(const FString& Text)
        {
            FString Result = TEXT("\"");
            for (int32 Index = 0; Index < Text.Len(); ++Index)
            {
                const TCHAR Character = Text[Index];
                if (Character == TEXT('"') || Character == TEXT('\\')) Result.AppendChar(TEXT('\\'));
                Result.AppendChar(Character);
            }
            Result.AppendChar(TEXT('"'));
            return Result;
        }

        FString ScopeSeed(const FWorldUtilityScope& Scope)
        {
            return TEXT("[1,") + QuoteSeed(Scope.WorldKey) + TEXT(",") + QuoteSeed(Scope.ClockEpochKey)
                + TEXT(",") + QuoteSeed(Scope.ZoneKey) + TEXT("]");
        }

        FString ScopeLabel(const FString& Seed)
        {
            return FString::Printf(TEXT("%08x%08x"), Hash(Seed), Hash(Seed, 2246822519u));
        }

        struct FEpisode
        {
            FString Id;
            int64 StartsAtRealMs = 0, EndsAtRealMs = 0;
        };

        bool Episode(const FString& Seed, const FString& Label, const FResidentClock& Clock,
            const FRules& Rules, int64 Block, FEpisode& Out)
        {
            if (!IntegerTime(Block)) return false;
            // Decimal integer formatting avoids locale/grouping and floating exponent forms.
            const FString BlockText = FString::Printf(TEXT("%lld"), static_cast<long long>(Block));
            const FString Prefix = TEXT("[") + QuoteSeed(Seed) + TEXT(",\"") + Rules.Axis + TEXT("\",") + BlockText;
            const int64 Duration = Rules.Minimum
                + Hash(Prefix + TEXT(",\"duration\"]")) % (Rules.Maximum - Rules.Minimum + 1);
            const int64 PossibleOffsets = Rules.Cycle - Duration - Rules.GoodMargin * 2 + 1;
            const int64 Offset = Rules.GoodMargin + Hash(Prefix + TEXT(",\"offset\"]")) % PossibleOffsets;
            int64 BlockMinute = 0, StartMinute = 0, StartDelta = 0, DurationMs = 0;
            if (!Multiply(Block, Rules.Cycle, BlockMinute) || !Add(BlockMinute, Offset, StartMinute)
                || !Multiply(StartMinute, Clock.RealMsPerGameMinute, StartDelta)
                || !Add(Clock.EpochRealMs, StartDelta, Out.StartsAtRealMs)
                || !Multiply(Duration, Clock.RealMsPerGameMinute, DurationMs)
                || !Add(Out.StartsAtRealMs, DurationMs, Out.EndsAtRealMs)) return false;
            Out.Id = TEXT("utilities-v1:") + Label + TEXT(":") + Rules.Axis + TEXT(":") + BlockText;
            return true;
        }

        bool AxisWindow(const FString& Seed, const FString& Label, const FResidentClock& Clock,
            const FRules& Rules, int64 Now, FWorldUtilityWindow& Out, bool& Disrupted, FString& OutError)
        {
            int64 CycleMs = 0;
            if (!Multiply(Rules.Cycle, Clock.RealMsPerGameMinute, CycleMs)) return Fail(OutError, UtilityRangeError);
            if (CycleMs == 0) return Fail(OutError, TEXT("Utility cycles require a positive stored game-time scale."));
            const int64 Block = (Now - Clock.EpochRealMs) / CycleMs;
            FEpisode Current;
            if (!Episode(Seed, Label, Clock, Rules, Block, Current)) return Fail(OutError, UtilityRangeError);
            Disrupted = Now >= Current.StartsAtRealMs && Now < Current.EndsAtRealMs;
            if (Disrupted)
            {
                Out.EventId = Current.Id + TEXT(":started");
                Out.EpisodeId = Current.Id;
                Out.StartedAtRealMs = Current.StartsAtRealMs;
                Out.EndsAtRealMs = Out.NextChangeAtRealMs = Current.EndsAtRealMs;
                return true;
            }

            FEpisode Previous, Next;
            const bool HasPrevious = Now >= Current.EndsAtRealMs || Block > 0;
            if (Now >= Current.EndsAtRealMs) Previous = Current;
            else if (Block > 0 && !Episode(Seed, Label, Clock, Rules, Block - 1, Previous))
                return Fail(OutError, UtilityRangeError);
            if (Now < Current.StartsAtRealMs) Next = Current;
            else
            {
                int64 NextBlock = 0;
                if (!Add(Block, 1, NextBlock) || !Episode(Seed, Label, Clock, Rules, NextBlock, Next))
                    return Fail(OutError, UtilityRangeError);
            }

            // Good intervals retain the previous restoration identity across cycle boundaries.
            Out.EventId = HasPrevious ? Previous.Id + TEXT(":restored")
                : TEXT("utilities-v1:") + Label + TEXT(":") + Rules.Axis + TEXT(":initial");
            Out.EpisodeId.Reset();
            Out.StartedAtRealMs = HasPrevious ? Previous.EndsAtRealMs : Clock.EpochRealMs;
            Out.EndsAtRealMs = Out.NextChangeAtRealMs = Next.StartsAtRealMs;
            return true;
        }
    }

    bool SampleWorldUtilities(const FWorldUtilityObservation& Observation, FWorldUtilities& Out, FString& OutError)
    {
        Out = FWorldUtilities{};
        OutError.Reset();
        FWorldUtilities Candidate;
        if (!WorldUtilitiesPrivate::ValidateObservation(Observation, Candidate.GameMinute, OutError)) return false;
        const FString Seed = WorldUtilitiesPrivate::ScopeSeed(Observation.Scope);
        const FString Label = WorldUtilitiesPrivate::ScopeLabel(Seed);
        bool PowerOff = false, NetworkSlow = false;
        if (!WorldUtilitiesPrivate::AxisWindow(Seed, Label, Observation.Clock, WorldUtilitiesPrivate::PowerRules,
                Observation.ServerNowRealMs, Candidate.Power, PowerOff, OutError)
            || !WorldUtilitiesPrivate::AxisWindow(Seed, Label, Observation.Clock, WorldUtilitiesPrivate::NetworkRules,
                Observation.ServerNowRealMs, Candidate.Network, NetworkSlow, OutError)) return false;
        Candidate.Version = WorldUtilitiesPrivate::Version;
        Candidate.Scope = Observation.Scope;
        Candidate.SampledAtRealMs = Observation.ServerNowRealMs;
        Candidate.Power.Status = PowerOff ? EWorldPowerStatus::Outage : EWorldPowerStatus::On;
        Candidate.Network.Status = NetworkSlow ? EWorldNetworkStatus::Slow : EWorldNetworkStatus::Good;
        Candidate.NextChangeAtRealMs = FMath::Min(Candidate.Power.NextChangeAtRealMs, Candidate.Network.NextChangeAtRealMs);
        Out = MoveTemp(Candidate);
        return true;
    }

    bool ProjectWorldUtilities(const FWorldUtilityObservation& Observation, int64 DisplayNowRealMs,
        FWorldUtilities& Out, FString& OutError)
    {
        Out = FWorldUtilities{};
        OutError.Reset();
        int64 OriginalGameMinute = 0;
        if (!WorldUtilitiesPrivate::ValidateObservation(Observation, OriginalGameMinute, OutError)) return false;
        if (!WorldUtilitiesPrivate::IntegerTime(DisplayNowRealMs) || DisplayNowRealMs < Observation.ServerNowRealMs
            || DisplayNowRealMs - Observation.ServerNowRealMs > WorldUtilitiesPrivate::MaximumDisplayAgeRealMs)
            return WorldUtilitiesPrivate::Fail(OutError, TEXT("Refresh the admitted server observation before projecting utility conditions."));
        FWorldUtilityObservation Projected = Observation;
        Projected.ServerNowRealMs = DisplayNowRealMs;
        Projected.NotBeforeRealMs = Observation.ServerNowRealMs;
        return SampleWorldUtilities(Projected, Out, OutError);
    }
}
