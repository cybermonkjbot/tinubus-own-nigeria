#include "BaddieCompanionMovementPresentation.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

#include <algorithm>
#include <cmath>

namespace BaddieCompanion
{
    namespace
    {
        constexpr double Pi = 3.14159265358979323846;
        constexpr double TurnSpeed = 4;
        constexpr int32 MaxPathPoints = 32;
        constexpr int64 MaxSafeInteger = 9007199254740991LL;

        bool ReadNumber(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key, double& Out)
        {
            const auto Value = Object.IsValid() ? Object->TryGetField(Key) : nullptr;
            return Value.IsValid() && Value->Type == EJson::Number
                && Value->TryGetNumber(Out) && std::isfinite(Out);
        }

        bool ReadInteger(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key,
            int64 Minimum, int64 Maximum, int64& Out)
        {
            double Number = 0;
            if (!ReadNumber(Object, Key, Number) || Number < Minimum || Number > Maximum
                || std::floor(Number) != Number) return false;
            Out = static_cast<int64>(Number);
            return true;
        }

        double NormalizeAngleRadians(double Value)
        {
            return std::atan2(std::sin(Value), std::cos(Value));
        }

        bool FinitePose(const FMovementPose& Pose)
        {
            return std::isfinite(Pose.X) && std::isfinite(Pose.Z)
                && std::isfinite(Pose.YawRadians);
        }

        bool ValidRoute(const FMovementRoute& Movement)
        {
            if (Movement.Version != 1 || Movement.Sequence < 1 || Movement.Sequence > 1000000000
                || Movement.RoomRevision < 0 || Movement.RoomRevision > MaxSafeInteger
                || !std::isfinite(Movement.StartedAtMs) || !FinitePose(Movement.From)
                || std::abs(Movement.From.YawRadians) > Pi || !std::isfinite(Movement.YawRadians)
                || std::abs(Movement.YawRadians) > Pi || Movement.Path.Num() > MaxPathPoints
                || (Movement.InitialSpeedMps.IsSet() && !std::isfinite(Movement.InitialSpeedMps.GetValue())))
                return false;
            switch (Movement.Pace)
            {
            case EMovementPace::Unpaced:
            case EMovementPace::Walk:
            case EMovementPace::Run:
            case EMovementPace::Fast: break;
            default: return false;
            }
            for (const FMovementPoint& Point : Movement.Path)
                if (!std::isfinite(Point.X) || !std::isfinite(Point.Z)) return false;
            return true;
        }

        struct FTravelProfile
        {
            double StartSpeedMps = 0;
            double RampDurationSeconds = 0;
            double RampDistanceMetres = 0;
            double PeakSpeedMps = 0;
            double CruiseDurationSeconds = 0;
            double AccelerationMps2 = 0;
            double DurationSeconds = 0;
        };

        FTravelProfile Travel(double Distance, double InitialSpeed, EMovementPace Pace)
        {
            const double Speed = MovementPaceSpeed(Pace);
            const double Acceleration = MovementPaceAcceleration(Pace);
            const double Start = std::max(0.0, std::min({Speed, InitialSpeed,
                std::sqrt(2 * Acceleration * Distance)}));
            const double Peak = std::min(Speed, std::sqrt(Acceleration * Distance + Start * Start / 2));
            const double Ramp = (Peak - Start) / Acceleration;
            const double RampDistance = (Start + Peak) * Ramp / 2;
            const double Cruise = Peak > 0
                ? std::max(0.0, Distance - RampDistance - Peak * Peak / (2 * Acceleration)) / Peak : 0;
            return {Start, Ramp, RampDistance, Peak, Cruise, Acceleration,
                Ramp + Cruise + Peak / Acceleration};
        }

        struct FMovementSegment
        {
            FMovementPose From;
            FMovementPose To;
            double Distance = 0;
            double Turn = 0;
            double TurnDuration = 0;
            FTravelProfile Profile;
        };

        struct FPreparedRoute
        {
            TArray<FMovementSegment> Segments;
            FMovementPose Last;
            double FinalTurn = 0;
            double FinalTurnDuration = 0;
            double EndsAtMs = 0;
        };

        bool PrepareRoute(const FMovementRoute& Movement, FPreparedRoute& Out)
        {
            if (!ValidRoute(Movement)) return false;
            Out.Segments.Reserve(Movement.Path.Num());
            FMovementPose Position = Movement.From;
            double InitialSpeed = Movement.InitialSpeedMps.IsSet() ? Movement.InitialSpeedMps.GetValue() : 0;
            double Seconds = 0;
            for (const FMovementPoint& Point : Movement.Path)
            {
                const double Dx = Point.X - Position.X;
                const double Dz = Point.Z - Position.Z;
                const double Distance = std::hypot(Dx, Dz);
                if (!std::isfinite(Distance)) return false;
                if (Distance < .000001) continue;
                const double Heading = std::atan2(Dx, Dz);
                const double Turn = NormalizeAngleRadians(Heading - Position.YawRadians);
                const double TurnDuration = std::abs(Turn) / TurnSpeed;
                const FTravelProfile Profile = Travel(Distance,
                    TurnDuration < .001 ? InitialSpeed : 0, Movement.Pace);
                if (!std::isfinite(Profile.DurationSeconds)) return false;
                const FMovementPose Next{Point.X, Point.Z, Heading};
                Out.Segments.Add(FMovementSegment{Position, Next, Distance, Turn, TurnDuration, Profile});
                Seconds += TurnDuration + Profile.DurationSeconds;
                InitialSpeed = 0;
                Position = Next;
            }
            Out.Last = Position;
            Out.FinalTurn = NormalizeAngleRadians(Movement.YawRadians - Position.YawRadians);
            Out.FinalTurnDuration = std::abs(Out.FinalTurn) / TurnSpeed;
            Out.EndsAtMs = Movement.StartedAtMs + (Seconds + Out.FinalTurnDuration) * 1000;
            return std::isfinite(Out.EndsAtMs);
        }

        bool SamplePrepared(const FMovementRoute& Movement, const FPreparedRoute& Route,
            double NowMs, FMovementSample& Out)
        {
            if (!std::isfinite(NowMs)) return false;
            double Seconds = std::max(0.0, (NowMs - Movement.StartedAtMs) / 1000);
            if (!std::isfinite(Seconds)) return false;
            for (const FMovementSegment& Segment : Route.Segments)
            {
                if (Seconds < Segment.TurnDuration)
                {
                    Out = {Segment.From, true, 0};
                    Out.Position.YawRadians = NormalizeAngleRadians(Segment.From.YawRadians
                        + Segment.Turn * Seconds / Segment.TurnDuration);
                    return true;
                }
                Seconds -= Segment.TurnDuration;
                const FTravelProfile& Profile = Segment.Profile;
                if (Seconds < Profile.DurationSeconds)
                {
                    const bool bRamping = Seconds < Profile.RampDurationSeconds;
                    const bool bCruising = Seconds < Profile.RampDurationSeconds + Profile.CruiseDurationSeconds;
                    const double Remaining = Profile.DurationSeconds - Seconds;
                    const double Speed = bRamping ? Profile.StartSpeedMps + Profile.AccelerationMps2 * Seconds
                        : bCruising ? Profile.PeakSpeedMps : Profile.AccelerationMps2 * Remaining;
                    const double Walked = bRamping
                        ? Profile.StartSpeedMps * Seconds + .5 * Profile.AccelerationMps2 * (Seconds * Seconds)
                        : bCruising ? Profile.RampDistanceMetres + Profile.PeakSpeedMps * (Seconds - Profile.RampDurationSeconds)
                        : Segment.Distance - .5 * Profile.AccelerationMps2 * (Remaining * Remaining);
                    const FMovementPose Position{
                        Segment.From.X + (Segment.To.X - Segment.From.X) * Walked / Segment.Distance,
                        Segment.From.Z + (Segment.To.Z - Segment.From.Z) * Walked / Segment.Distance,
                        Segment.To.YawRadians};
                    if (!FinitePose(Position) || !std::isfinite(Speed)) return false;
                    Out = {Position, true, Speed};
                    return true;
                }
                Seconds -= Profile.DurationSeconds;
            }
            Out = {Route.Last, Seconds < Route.FinalTurnDuration, 0};
            Out.Position.YawRadians = Out.bMoving
                ? NormalizeAngleRadians(Route.Last.YawRadians + Route.FinalTurn * Seconds / Route.FinalTurnDuration)
                : Movement.YawRadians;
            return true;
        }
    }

    bool DecodeMovementRoute(const TSharedPtr<FJsonObject>& Object, FMovementRoute& Out, FString& OutError)
    {
        Out = {};
        FMovementRoute Next;
        double Version = 0;
        OutError = TEXT("Movement route has an unsupported version.");
        if (!ReadNumber(Object, TEXT("version"), Version) || Version != 1) return false;
        OutError = TEXT("Movement route has an invalid sequence or room revision.");
        if (!ReadInteger(Object, TEXT("sequence"), 1, 1000000000, Next.Sequence)
            || !ReadInteger(Object, TEXT("roomRevision"), 0, MaxSafeInteger, Next.RoomRevision)) return false;

        OutError = TEXT("Movement route has invalid timing or positions.");
        const TSharedPtr<FJsonObject>* From = nullptr;
        if (!ReadNumber(Object, TEXT("startedAt"), Next.StartedAtMs)
            || !Object->TryGetObjectField(TEXT("from"), From) || !From || !From->IsValid()
            || !ReadNumber(*From, TEXT("x"), Next.From.X)
            || !ReadNumber(*From, TEXT("z"), Next.From.Z)
            || !ReadNumber(*From, TEXT("yaw"), Next.From.YawRadians)
            || !ReadNumber(Object, TEXT("yaw"), Next.YawRadians)) return false;

        if (Object->HasField(TEXT("initialSpeed")))
        {
            double Speed = 0;
            OutError = TEXT("Movement route has an invalid initial speed.");
            if (!ReadNumber(Object, TEXT("initialSpeed"), Speed)) return false;
            Next.InitialSpeedMps = Speed;
        }
        if (Object->HasField(TEXT("pace")))
        {
            const auto Value = Object->TryGetField(TEXT("pace"));
            FString Pace;
            OutError = TEXT("Movement route has an unsupported pace.");
            if (!Value.IsValid() || Value->Type != EJson::String || !Value->TryGetString(Pace)) return false;
            if (Pace.Equals(TEXT("walk"), ESearchCase::CaseSensitive)) Next.Pace = EMovementPace::Walk;
            else if (Pace.Equals(TEXT("run"), ESearchCase::CaseSensitive)) Next.Pace = EMovementPace::Run;
            else if (Pace.Equals(TEXT("fast"), ESearchCase::CaseSensitive)) Next.Pace = EMovementPace::Fast;
            else return false;
        }

        const TArray<TSharedPtr<FJsonValue>>* Path = nullptr;
        OutError = TEXT("Movement route has an invalid path.");
        if (!Object->TryGetArrayField(TEXT("path"), Path) || !Path || Path->Num() > MaxPathPoints) return false;
        Next.Path.Reserve(Path->Num());
        for (const auto& Value : *Path)
        {
            if (!Value.IsValid() || Value->Type != EJson::Object) return false;
            FMovementPoint Point;
            const auto PointObject = Value->AsObject();
            if (!ReadNumber(PointObject, TEXT("x"), Point.X)
                || !ReadNumber(PointObject, TEXT("z"), Point.Z)) return false;
            Next.Path.Add(Point);
        }
        // Reuse the sampler's semantic/numeric guards, including derived durations.
        // An empty path remains valid for a stop or turn in place.
        double EndsAtMs = 0;
        OutError = TEXT("Movement route cannot be sampled.");
        if (!MovementEndsAt(Next, EndsAtMs)) return false;
        Out = MoveTemp(Next);
        OutError.Reset();
        return true;
    }

    double MovementPaceSpeed(EMovementPace Pace)
    {
        return Pace == EMovementPace::Fast ? 5.6 : Pace == EMovementPace::Run ? 3.6
            : Pace == EMovementPace::Walk ? 1.4 : .65;
    }

    double MovementPaceAcceleration(EMovementPace Pace)
    {
        return Pace == EMovementPace::Fast ? 28.8 : Pace == EMovementPace::Unpaced ? 1.8 : 7.2;
    }

    bool SampleMovement(const FMovementRoute& Movement, double NowMs, FMovementSample& OutSample)
    {
        OutSample = {};
        FPreparedRoute Route;
        FMovementSample Sample;
        if (!PrepareRoute(Movement, Route) || !SamplePrepared(Movement, Route, NowMs, Sample)) return false;
        OutSample = Sample;
        return true;
    }

    bool MovementEndsAt(const FMovementRoute& Movement, double& OutEndsAtMs)
    {
        FPreparedRoute Route;
        const bool bValid = PrepareRoute(Movement, Route);
        OutEndsAtMs = bValid ? Route.EndsAtMs : 0;
        return bValid;
    }

    bool MovementWindow(const FMovementRoute& Movement, double FromAtMs, double ToAtMs,
        TArray<FMovementPose>& OutPositions)
    {
        OutPositions.Reset();
        FPreparedRoute Route;
        FMovementSample Start, End;
        if (!PrepareRoute(Movement, Route)
            || !SamplePrepared(Movement, Route, FromAtMs, Start)
            || !SamplePrepared(Movement, Route, ToAtMs, End)) return false;
        OutPositions.Reserve(Route.Segments.Num() + 2);
        OutPositions.Add(Start.Position);
        double At = Movement.StartedAtMs;
        for (const FMovementSegment& Segment : Route.Segments)
        {
            At += (Segment.TurnDuration + Segment.Profile.DurationSeconds) * 1000;
            if (At > FromAtMs && At < ToAtMs) OutPositions.Add(Segment.To);
            if (At >= ToAtMs) break;
        }
        OutPositions.Add(End.Position);
        return true;
    }

    bool MovementCollisionStopPosition(const FMovementRoute& Movement,
        const FMovementCollisionStop& Collision, double NowMs, FMovementPose& OutPosition)
    {
        if (Collision.Sequence != Movement.Sequence || !std::isfinite(NowMs)
            || !std::isfinite(Collision.ElapsedMs) || Collision.ElapsedMs < 0)
        {
            OutPosition = {};
            return false;
        }
        const double Elapsed = std::max(0.0, NowMs - Movement.StartedAtMs);
        if (!std::isfinite(Elapsed))
        {
            OutPosition = {};
            return false;
        }
        const double StoppedAt = Movement.StartedAtMs
            + std::max({0.0, Elapsed - 2000, std::min(Elapsed, Collision.ElapsedMs)});
        FMovementSample Sample;
        const bool bValid = SampleMovement(Movement, StoppedAt, Sample);
        OutPosition = bValid ? Sample.Position : FMovementPose{};
        return bValid;
    }
}
