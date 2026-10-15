#include "BaddieCompanionJourneyPresentation.h"
#include "BaddieCompanionStateJson.h"
#include <cmath>

namespace BaddieCompanion
{
    namespace
    {
        using StateJsonPrivate::ValidTime;
        constexpr const TCHAR* Error = TEXT("The saved journey couldn't be sampled.");
        bool Finite(double N) { return std::isfinite(N); }
        bool Fraction(double N) { return Finite(N) && N >= 0 && N <= 1; }
        double Clamp(double N) { return FMath::Clamp(N, 0.0, 1.0); }
        template<typename T> bool Presence(const TWorldPlayerField<T>& Field, bool Missing, bool Null)
        {
            switch (Field.Presence)
            {
            case EWorldFieldPresence::Missing: return Missing && !Field.Value.IsSet();
            case EWorldFieldPresence::Null: return Null && !Field.Value.IsSet();
            case EWorldFieldPresence::Value: return Field.Value.IsSet();
            default: return false;
            }
        }
        bool Point(const FWorldJourneyGeoPoint& P)
        { return Finite(P.Latitude) && Finite(P.Longitude) && std::abs(P.Latitude) <= 90 && std::abs(P.Longitude) <= 180; }
        bool RouteId(const FString& Id)
        {
            if (Id.IsEmpty() || FChar::IsWhitespace(Id[0]) || FChar::IsWhitespace(Id[Id.Len() - 1])) return false;
            int32 Units = 0;
            for (const TCHAR C : Id)
            {
                const uint32 Value = static_cast<uint32>(C);
                Units += sizeof(TCHAR) > 2 && Value > 0xffff ? 2 : 1;
                if (Units > 240 || Value < 0x20) return false;
            }
            return true;
        }
        bool RouteValid(const FWorldJourneyRouteIdentity& R)
        {
            if (!RouteId(R.Id) || R.Points.Num() < 2 || R.Points.Num() > 4096
                || R.PointCount != R.Points.Num() || !Finite(R.LengthMetres) || R.LengthMetres < 0
                || !Presence(R.Profile, true, false) || !Presence(R.SegmentDurationsMs, true, false)) return false;
            for (const auto& P : R.Points) if (!Point(P)) return false;
            if (R.Kind == EWorldJourneyRouteKind::Corridor)
                return R.Profile.Presence == EWorldFieldPresence::Missing && R.SegmentDurationsMs.Presence == EWorldFieldPresence::Missing;
            if (R.Kind != EWorldJourneyRouteKind::RoadNetwork || R.SegmentDurationsMs.Presence != EWorldFieldPresence::Value
                || R.SegmentDurationsMs.Value->Num() != R.Points.Num() - 1) return false;
            if (R.Profile.Value.IsSet() && *R.Profile.Value != EWorldJourneyProfile::DrivingTraffic
                && *R.Profile.Value != EWorldJourneyProfile::Walking) return false;
            for (const double D : *R.SegmentDurationsMs.Value) if (!ValidTime(D)) return false;
            return true;
        }
        bool TimingValid(const FWorldJourneyView& J, double Now)
        {
            if (!ValidTime(Now) || !ValidTime(J.StartedAtMs) || !ValidTime(J.ArrivesAtMs) || J.ArrivesAtMs < J.StartedAtMs
                || (J.Status != EWorldJourneyStatus::Traveling && J.Status != EWorldJourneyStatus::Arrived && J.Status != EWorldJourneyStatus::Interrupted)
                || !Presence(J.Checkpoint, false, true) || !Presence(J.TravelTiming, true, false)) return false;
            if (J.Checkpoint.Value.IsSet())
            {
                const auto& P = *J.Checkpoint.Value;
                if (!Fraction(P.Progress) || !ValidTime(P.StartsAtMs) || !ValidTime(P.EndsAtMs) || P.EndsAtMs < P.StartsAtMs
                    || !Presence(P.ResumedAtMs, true, true) || (P.ResumedAtMs.Value.IsSet() && !ValidTime(*P.ResumedAtMs.Value))) return false;
            }
            if (J.TravelTiming.Value.IsSet())
            {
                const auto& T = *J.TravelTiming.Value;
                if ((T.Rate != 1 && T.Rate != 4) || !ValidTime(T.AnchorAtMs) || !Fraction(T.AnchorProgress)
                    || !ValidTime(T.DurationMs) || T.DurationMs <= 0) return false;
            }
            return true;
        }
        bool Timing(const FWorldJourneyView& J, double Now, FWorldJourneyPresentationTiming& Out)
        {
            if (!TimingValid(J, Now)) return false;
            const auto* P = J.Checkpoint.Value.IsSet() ? &*J.Checkpoint.Value : nullptr;
            const bool Waiting = P && P->ResumedAtMs.Presence == EWorldFieldPresence::Null;
            const double PauseEnd = P ? P->ResumedAtMs.Value.IsSet() ? FMath::Max(P->StartsAtMs, *P->ResumedAtMs.Value) : P->EndsAtMs : 0;
            Out.bAtCheckpoint = P && Now >= P->StartsAtMs && (Waiting || Now < PauseEnd);
            if (J.TravelTiming.Value.IsSet())
            {
                const auto& T = *J.TravelTiming.Value;
                const double ElapsedPause = P ? FMath::Max(0.0, FMath::Min(Now, Waiting ? Now : PauseEnd) - FMath::Max(T.AnchorAtMs, P->StartsAtMs)) : 0;
                const double Elapsed = FMath::Max(0.0, Now - T.AnchorAtMs - ElapsedPause);
                const double Progress = J.Status == EWorldJourneyStatus::Arrived ? 1 : T.AnchorProgress + Elapsed * T.Rate / T.DurationMs;
                if (!Finite(ElapsedPause) || !Finite(Elapsed) || !Finite(Progress)) return false;
                Out.Progress = Clamp(Progress);
                const double RemainingPause = P ? FMath::Max(0.0, (Waiting ? P->EndsAtMs : PauseEnd) - FMath::Max(Now, P->StartsAtMs)) : 0;
                Out.RemainingMs = FMath::Max(0.0, (1 - Out.Progress) * T.DurationMs / T.Rate) + RemainingPause;
                Out.bArrivalDue = J.Status == EWorldJourneyStatus::Traveling && !Waiting && Out.Progress >= 1;
            }
            else
            {
                const double PauseMs = P ? P->EndsAtMs - P->StartsAtMs : 0;
                const double Duration = FMath::Max(1.0, J.ArrivesAtMs - J.StartedAtMs - PauseMs);
                const double ElapsedPause = P ? FMath::Max(0.0, Waiting ? Now - P->StartsAtMs
                    : FMath::Min(PauseEnd - P->StartsAtMs, Now - P->StartsAtMs)) : 0;
                const double ArrivesAt = P && !Waiting ? J.ArrivesAtMs + PauseEnd - P->EndsAtMs : J.ArrivesAtMs;
                const double ElapsedTravel = Now - J.StartedAtMs - ElapsedPause;
                const double Progress = J.Status == EWorldJourneyStatus::Arrived ? 1 : ElapsedTravel / Duration;
                if (!Finite(Duration) || !Finite(ElapsedPause) || !Finite(ArrivesAt) || !Finite(ElapsedTravel) || !Finite(Progress)) return false;
                Out.Progress = Clamp(Progress);
                Out.RemainingMs = P && Waiting ? FMath::Max(0.0, Duration - ElapsedTravel)
                    + FMath::Max(0.0, P->EndsAtMs - FMath::Max(Now, P->StartsAtMs)) : FMath::Max(0.0, ArrivesAt - Now);
                Out.bArrivalDue = J.Status == EWorldJourneyStatus::Traveling && !Waiting && Now >= ArrivesAt;
            }
            return Fraction(Out.Progress) && Finite(Out.RemainingMs) && Out.RemainingMs >= 0;
        }
        bool Distance(const FWorldJourneyGeoPoint& A, const FWorldJourneyGeoPoint& B, double& Out)
        {
            constexpr double Radians = 3.14159265358979323846 / 180;
            const double Lat = (B.Latitude - A.Latitude) * Radians, Lng = (B.Longitude - A.Longitude) * Radians;
            const double SinLat = std::sin(Lat / 2), SinLng = std::sin(Lng / 2);
            const double H = SinLat * SinLat + std::cos(A.Latitude * Radians) * std::cos(B.Latitude * Radians) * SinLng * SinLng;
            Out = 6371000 * 2 * std::atan2(std::sqrt(H), std::sqrt(FMath::Max(0.0, 1 - H)));
            return Finite(H) && Finite(Out) && Out >= 0;
        }
        bool Interpolate(const FWorldJourneyGeoPoint& A, const FWorldJourneyGeoPoint& B, double Ratio, FWorldJourneyGeoPoint& Out)
        {
            if (!Finite(Ratio)) return false;
            Out.Latitude = A.Latitude + (B.Latitude - A.Latitude) * Ratio;
            Out.Longitude = A.Longitude + (B.Longitude - A.Longitude) * Ratio;
            return Point(Out);
        }
        bool Segment(const FWorldJourneyGeoPoint& A, const FWorldJourneyGeoPoint& B,
            double Remaining, double Weight, FWorldJourneyGeoPoint& Out)
        {
            const double Ratio = Weight > 0 ? Remaining / Weight : 1;
            return Finite(Ratio) && Interpolate(A, B, FMath::Min(1.0, Ratio), Out);
        }
        bool Geographic(const FWorldJourneyRouteIdentity& R, double Progress, FWorldJourneyGeoPoint& Out)
        {
            double Remaining = Clamp(Progress) * R.LengthMetres;
            if (!Finite(Remaining)) return false;
            for (int32 I = 1; I < R.Points.Num(); ++I)
            {
                double Length = 0;
                if (!Distance(R.Points[I - 1], R.Points[I], Length)) return false;
                if (Remaining <= Length || I == R.Points.Num() - 1)
                    return Segment(R.Points[I - 1], R.Points[I], Remaining, Length, Out);
                Remaining -= Length;
                if (!Finite(Remaining)) return false;
            }
            return false;
        }
        bool JourneyPoint(const FWorldJourneyRouteIdentity& R, double Progress, FWorldJourneyGeoPoint& Out)
        {
            if (R.Kind != EWorldJourneyRouteKind::RoadNetwork) return Geographic(R, Progress, Out);
            // These shortcuts precede the total-duration fallback, including all-zero routes.
            if (Progress <= 0) { Out = R.Points[0]; return true; }
            if (Progress >= 1) { Out = R.Points.Last(); return true; }
            const auto& Durations = *R.SegmentDurationsMs.Value;
            double Total = 0;
            for (const double D : Durations) { Total += D; if (!Finite(Total)) return false; }
            if (Total <= 0) return Geographic(R, Progress, Out);
            double Remaining = Progress * Total;
            if (!Finite(Remaining)) return false;
            for (int32 I = 0; I < Durations.Num(); ++I)
            {
                if (Remaining <= Durations[I] || I == Durations.Num() - 1)
                    return Segment(R.Points[I], R.Points[I + 1], Remaining, Durations[I], Out);
                Remaining -= Durations[I];
                if (!Finite(Remaining)) return false;
            }
            return false;
        }
        template<typename T> bool Finish(bool Success, const T& Candidate, T& Out, FString& OutError)
        {
            Out = Success ? Candidate : T{};
            OutError = Success ? FString() : FString(Error);
            return Success;
        }
    }
    bool CalculateWorldJourneyTiming(const FWorldJourneyView& Journey, double ServerNowMs,
        FWorldJourneyPresentationTiming& Out, FString& OutError)
    {
        FWorldJourneyPresentationTiming Candidate;
        const bool Success = Timing(Journey, ServerNowMs, Candidate);
        return Finish(Success, Candidate, Out, OutError);
    }
    bool SampleWorldJourneyRoute(const FWorldJourneyRouteIdentity& Route, double Progress,
        FWorldJourneyGeoPoint& Out, FString& OutError)
    {
        FWorldJourneyGeoPoint Candidate;
        const bool Success = Finite(Progress) && RouteValid(Route) && Geographic(Route, Progress, Candidate);
        return Finish(Success, Candidate, Out, OutError);
    }
    bool SampleWorldJourney(const FWorldJourneyView& Journey, double ServerNowMs,
        FWorldJourneyPresentationSample& Out, FString& OutError)
    {
        FWorldJourneyPresentationSample Candidate;
        const bool Success = Timing(Journey, ServerNowMs, Candidate.Timing) && RouteValid(Journey.Route)
            && JourneyPoint(Journey.Route, Candidate.Timing.Progress, Candidate.Point);
        return Finish(Success, Candidate, Out, OutError);
    }
}
