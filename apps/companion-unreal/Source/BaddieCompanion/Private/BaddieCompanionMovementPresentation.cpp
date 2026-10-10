#include "BaddieCompanionMovementPresentation.h"

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

        double Angle(double Value)
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
            double Start = 0;
            double Ramp = 0;
            double RampDistance = 0;
            double Peak = 0;
            double Cruise = 0;
            double Acceleration = 0;
            double Duration = 0;
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
                const double Turn = Angle(Heading - Position.YawRadians);
                const double TurnDuration = std::abs(Turn) / TurnSpeed;
                const FTravelProfile Profile = Travel(Distance,
                    TurnDuration < .001 ? InitialSpeed : 0, Movement.Pace);
                if (!std::isfinite(Profile.Duration)) return false;
                const FMovementPose Next{Point.X, Point.Z, Heading};
                Out.Segments.Add(FMovementSegment{Position, Next, Distance, Turn, TurnDuration, Profile});
                Seconds += TurnDuration + Profile.Duration;
                InitialSpeed = 0;
                Position = Next;
            }
            Out.Last = Position;
            Out.FinalTurn = Angle(Movement.YawRadians - Position.YawRadians);
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
                    Out.Position.YawRadians = Angle(Segment.From.YawRadians
                        + Segment.Turn * Seconds / Segment.TurnDuration);
                    return true;
                }
                Seconds -= Segment.TurnDuration;
                const FTravelProfile& Profile = Segment.Profile;
                if (Seconds < Profile.Duration)
                {
                    const bool bRamping = Seconds < Profile.Ramp;
                    const bool bCruising = Seconds < Profile.Ramp + Profile.Cruise;
                    const double Remaining = Profile.Duration - Seconds;
                    const double Speed = bRamping ? Profile.Start + Profile.Acceleration * Seconds
                        : bCruising ? Profile.Peak : Profile.Acceleration * Remaining;
                    const double Walked = bRamping
                        ? Profile.Start * Seconds + .5 * Profile.Acceleration * (Seconds * Seconds)
                        : bCruising ? Profile.RampDistance + Profile.Peak * (Seconds - Profile.Ramp)
                        : Segment.Distance - .5 * Profile.Acceleration * (Remaining * Remaining);
                    const FMovementPose Position{
                        Segment.From.X + (Segment.To.X - Segment.From.X) * Walked / Segment.Distance,
                        Segment.From.Z + (Segment.To.Z - Segment.From.Z) * Walked / Segment.Distance,
                        Segment.To.YawRadians};
                    if (!FinitePose(Position) || !std::isfinite(Speed)) return false;
                    Out = {Position, true, Speed};
                    return true;
                }
                Seconds -= Profile.Duration;
            }
            Out = {Route.Last, Seconds < Route.FinalTurnDuration, 0};
            Out.Position.YawRadians = Out.bMoving
                ? Angle(Route.Last.YawRadians + Route.FinalTurn * Seconds / Route.FinalTurnDuration)
                : Movement.YawRadians;
            return true;
        }
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
            At += (Segment.TurnDuration + Segment.Profile.Duration) * 1000;
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
