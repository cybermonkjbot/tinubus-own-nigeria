#include "BaddieCompanionGaitPresentation.h"

#include "BaddieCompanionMovementPresentation.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace BaddieCompanion
{
    namespace
    {
        constexpr double Pi = 3.14159265358979323846;

        double Clamp(double Value, double Min, double Max)
        {
            return std::max(Min, std::min(Max, Value));
        }

        double Damp(double From, double To, double Rate, double Dt)
        {
            const double Alpha = 1 - std::exp(-Rate * Dt);
            return (1 - Alpha) * From + Alpha * To;
        }

        bool Finite(const FVector& Value)
        {
            return std::isfinite(Value.X) && std::isfinite(Value.Y) && std::isfinite(Value.Z);
        }

        bool Rotation(const FQuat& Value)
        {
            if (!std::isfinite(Value.X) || !std::isfinite(Value.Y)
                || !std::isfinite(Value.Z) || !std::isfinite(Value.W)) return false;
            const double LengthSq = Value.X * Value.X + Value.Y * Value.Y
                + Value.Z * Value.Z + Value.W * Value.W;
            return std::abs(LengthSq - 1) < .000001;
        }

        double RotationDistance(const FQuat& A, const FQuat& B)
        {
            return 2 * std::acos(std::abs(Clamp(A.X * B.X + A.Y * B.Y
                + A.Z * B.Z + A.W * B.W, -1, 1)));
        }

        double FlatDistance(const FVector& A, const FVector& B)
        {
            return std::hypot(A.X - B.X, A.Z - B.Z);
        }

        bool ValidFrame(const FGaitFrameInput& Frame)
        {
            if (!std::isfinite(Frame.DeltaSeconds) || !std::isfinite(Frame.FloorYMetres)
                || !Finite(Frame.RootScale) || Frame.RootScale.Y <= 0
                || !std::isfinite(Frame.AcceptedSpeedMps) || !Finite(Frame.RootPositionMetres)
                || !Rotation(Frame.RootRotation)) return false;
            for (const FGaitFootSeed& Seed : Frame.Feet)
                if (!Finite(Seed.NeutralLocalMetres) || !Rotation(Seed.RestRotation)
                    || !std::isfinite(Seed.SoleHeightMetres) || Seed.SoleHeightMetres < 0) return false;
            return !Frame.Footwork.IsSet() || (Finite(Frame.Footwork.GetValue().LeftMetres)
                && Finite(Frame.Footwork.GetValue().RightMetres));
        }
    }

    double GaitEase(double Progress)
    {
        const double T = Clamp(Progress, 0, 1);
        return T * T * (3 - 2 * T);
    }

    EGaitPhase GaitStancePhase(double Progress)
    {
        return Progress < .08 ? EGaitPhase::HeelStrike : Progress < .28 ? EGaitPhase::Loading
            : Progress < .72 ? EGaitPhase::MidStance : EGaitPhase::HeelOff;
    }

    double GaitStancePitch(double Progress)
    {
        return -.21 * (1 - GaitEase(Progress / .24)) + .3 * GaitEase((Progress - .72) / .28);
    }

    double GaitSwingPitch(double Progress, double TakeoffPitch)
    {
        if (Progress < .16) return TakeoffPitch + (.38 - TakeoffPitch) * GaitEase(Progress / .16);
        if (Progress < .65) return .38 * (1 - GaitEase((Progress - .16) / .49));
        return -.21 * GaitEase((Progress - .65) / .35);
    }

    bool GaitStrideTiming(double SpeedMps, double Scale, FGaitStrideTiming& Out)
    {
        Out = {};
        if (!std::isfinite(SpeedMps) || !std::isfinite(Scale) || Scale <= 0) return false;
        const double Ratio = Clamp(SpeedMps / (1.7 * Scale), .16, 6);
        const double Duration = .32 / std::sqrt(Ratio);
        const double Travel = SpeedMps * Duration;
        const double Lead = Travel + std::min(.8 * Scale, Travel * .4);
        if (!std::isfinite(Lead)) return false;
        Out = {Duration, Lead};
        return true;
    }

    void FGaitPresentation::Reset()
    {
        *this = FGaitPresentation{};
    }

    bool FGaitPresentation::Update(const FGaitFrameInput& Frame, FGaitLayoutSupport LayoutSupport,
        FGaitContactFrame& Out)
    {
        Out = {};
        if (!ValidFrame(Frame)) { Reset(); return false; }
        const double Elapsed = std::max(0.0, Frame.DeltaSeconds);
        const double Dt = Elapsed > .5 ? 0 : Elapsed;
        const double Scale = Frame.RootScale.Y;
        const double ContinuousTravel = std::max({0.0, Frame.AcceptedSpeedMps, PreviousSpeed}) * Dt;
        if (bRootKnown && (Elapsed > .5
            || (Frame.RootPositionMetres - PreviousPosition).Size() > .8 * Scale + ContinuousTravel)) Reset();
        const bool bInitialize = !bRootKnown;
        FVector Forward = Frame.RootRotation.RotateVector(FVector(0, 0, 1));
        Forward.Y = 0;
        const double ForwardLength = Forward.Size();
        if (ForwardLength > 0) Forward /= ForwardLength;
        const double Yaw = std::atan2(Forward.X, Forward.Z);
        const double YawDelta = Yaw - PreviousYaw;
        const double YawRate = bRootKnown && Elapsed > 0
            ? Clamp(std::atan2(std::sin(YawDelta), std::cos(YawDelta)) / Elapsed, -5, 5) : 0;
        FVector Velocity = bRootKnown && Elapsed > 0
            ? (Frame.RootPositionMetres - PreviousPosition) / Elapsed : FVector::ZeroVector;
        Velocity.Y = 0;
        const double Speed = Velocity.Size();
        if (!Finite(Velocity) || !std::isfinite(Speed)) { Reset(); return false; }
        const bool bTravelling = Speed > .015 * Scale;
        const bool bLocomoting = Frame.bMoving || bTravelling;
        if (bLocomoting && !bWasMoving) { StartAge = 0; StopSteps = 0; }
        if (!bLocomoting && bWasMoving)
        {
            StopSteps = 2;
            for (FFoot& Foot : Feet)
                if (Foot.Contact.bStepping) { Foot.Step.bBraking = true; break; }
        }
        bWasMoving = bLocomoting;
        const double Acceleration = Elapsed > 0 ? Clamp((Speed - PreviousSpeed) / Elapsed, -4, 4) : 0;
        Bank = Damp(Bank, Clamp(-std::atan2(Speed * YawRate, 9.81), -.16, .16), 10, Dt);
        AccelerationLean = Damp(AccelerationLean, Clamp(Acceleration / 9.81, -.1, .1), 10, Dt);
        PreviousPosition = Frame.RootPositionMetres;
        PreviousYaw = Yaw;
        PreviousSpeed = Speed;
        bRootKnown = true;
        const double WalkThreshold = MovementPaceSpeed(EMovementPace::Walk) * 1.2;
        const double RunThreshold = MovementPaceSpeed(EMovementPace::Run) * .93;
        RunWeight = Damp(RunWeight, bTravelling
            ? GaitEase((Speed - WalkThreshold) / (RunThreshold - WalkThreshold)) : 0, 9, Dt);

        bool bSupportValid = true;
        const auto SupportHeight = [&](int32 Side, const FVector& At, const FQuat& Orientation)
        {
            if (!Finite(At) || !Rotation(Orientation))
            {
                bSupportValid = false;
                return Frame.FloorYMetres;
            }
            const double Height = LayoutSupport(Side, At, Orientation);
            if (!std::isfinite(Height)) { bSupportValid = false; return Frame.FloorYMetres; }
            return Clamp(Height, Frame.FloorYMetres - .35 * Scale, Frame.FloorYMetres + .35 * Scale);
        };
        for (int32 Side = 0; Side < 2; ++Side)
        {
            FFoot& Foot = Feet[Side];
            if (bInitialize)
            {
                Foot.Seed = Frame.Feet[Side];
                Foot.Planted = Frame.RootPositionMetres
                    + Frame.RootRotation.RotateVector(Foot.Seed.NeutralLocalMetres * Frame.RootScale);
                Foot.Planted.Y = Frame.FloorYMetres + Foot.Seed.SoleHeightMetres * Scale;
                Foot.Rotation = Frame.RootRotation * Foot.Seed.RestRotation;
            }
            Foot.DesiredRotation = Frame.RootRotation * Foot.Seed.RestRotation;
            Foot.EndNeutral = Frame.RootPositionMetres
                + Frame.RootRotation.RotateVector(Foot.Seed.NeutralLocalMetres * Frame.RootScale);
            Foot.EndNeutral.Y = SupportHeight(Side, Foot.EndNeutral, Foot.DesiredRotation)
                + Foot.Seed.SoleHeightMetres * Scale;
        }

        // Contact clocks consume slow frames in the same 60 Hz substeps as source.
        const int32 ContactSteps = std::max(1, static_cast<int32>(std::ceil(Dt * 60)));
        const double StepDt = Dt / ContactSteps;
        for (int32 Index = 0; Index < ContactSteps; ++Index)
        {
            const double Remaining = Dt * (ContactSteps - Index - 1) / ContactSteps;
            StartAge = bLocomoting ? std::min(.12, StartAge + StepDt) : 0;
            for (int32 Side = 0; Side < 2; ++Side)
            {
                FFoot& Foot = Feet[Side];
                Foot.Neutral = Foot.EndNeutral - Velocity * Remaining;
                Foot.Neutral.Y = SupportHeight(Side, Foot.Neutral, Foot.DesiredRotation)
                    + Foot.Seed.SoleHeightMetres * Scale;
                if (!Foot.Contact.bStepping)
                    Foot.Planted.Y = SupportHeight(Side, Foot.Planted, Foot.Rotation)
                        + Foot.Seed.SoleHeightMetres * Scale;
            }
            const bool bActive = Feet[0].Contact.bStepping || Feet[1].Contact.bStepping;
            if (!bActive && StepDt > 0 && (!bLocomoting || StartAge >= .09)
                && !(Frame.Footwork.IsSet() && !bLocomoting))
            {
                const double Threshold = (bLocomoting ? .14 : .04) * Scale;
                int32 Next = -1;
                for (int32 Side = 0; Side < 2; ++Side)
                {
                    const FFoot& Foot = Feet[Side];
                    if (!(FlatDistance(Foot.Planted, Foot.Neutral) > Threshold
                        || RotationDistance(Foot.Rotation, Foot.DesiredRotation) > .28)) continue;
                    if (Next < 0) Next = Side;
                    if (Side != PreviousStep) { Next = Side; break; }
                }
                if (Next < 0 && bTravelling && StartAge < .12) Next = 1 - PreviousStep;
                if (!bLocomoting && StopSteps == 0) Next = -1;
                if (Next >= 0)
                {
                    FGaitStrideTiming Timing;
                    if (!GaitStrideTiming(Speed, Scale, Timing)) { Reset(); return false; }
                    FFoot& Foot = Feet[Next];
                    Foot.Step = {Foot.Planted, Foot.Neutral, Foot.Rotation, 0,
                        bTravelling ? Timing.DurationSeconds : .24, Foot.Contact.PitchRadians, !bLocomoting};
                    if (bTravelling) Foot.Step.To += Velocity * (Timing.LeadMetres / Speed);
                    if (Foot.Step.bBraking) StopSteps = std::max(0, StopSteps - 1);
                    Foot.Contact.bStepping = true;
                    PreviousStep = Next;
                }
            }
            for (int32 Side = 0; Side < 2; ++Side)
            {
                FFoot& Foot = Feet[Side];
                FGaitFootContact& Contact = Foot.Contact;
                Contact.TargetMetres = Foot.Planted;
                Contact.Orientation = Foot.Rotation;
                Contact.SwingProgress = 0;
                if (Contact.bStepping)
                {
                    FStep& Step = Foot.Step;
                    const double PreviousPhase = Step.Age / Step.Duration;
                    FGaitStrideTiming Timing;
                    if (!GaitStrideTiming(Speed, Scale, Timing)) { Reset(); return false; }
                    Step.Duration = Damp(Step.Duration,
                        bTravelling && !Step.bBraking ? Timing.DurationSeconds : .24, 12, StepDt);
                    FFoot& Support = Feet[1 - Side];
                    bool bSupportLimited = false;
                    if (bTravelling && !Step.bBraking && !Support.Contact.bStepping)
                    {
                        const double Lead = FVector::DotProduct(Support.Planted - Support.Neutral, Velocity) / Speed;
                        const double SupportRemaining = std::max(StepDt, (Lead + .8 * Scale) / Speed);
                        const double Limit = SupportRemaining / std::max(.001, 1 - PreviousPhase);
                        bSupportLimited = Step.Duration > Limit;
                        Step.Duration = std::min(Step.Duration, Limit);
                    }
                    Step.Age = std::min(Step.Duration, PreviousPhase * Step.Duration + StepDt);
                    const double Phase = Step.Age / Step.Duration;
                    Contact.SwingProgress = Phase;
                    Contact.Phase = Phase < .12 ? EGaitPhase::ToeOff : EGaitPhase::Swing;
                    Contact.Lock = 1 - GaitEase(Phase / .12) + GaitEase((Phase - .82) / .18);
                    if (Phase < .72 || Step.bBraking || bSupportLimited)
                    {
                        FVector Landing = Foot.Neutral;
                        if (bTravelling && !Step.bBraking)
                            Landing += Velocity * (Step.Duration * (1 - Phase)
                                + std::min(Step.Duration * .4, .8 * Scale / Speed));
                        if (bSupportLimited) Step.To = Landing;
                        else Step.To += (Landing - Step.To) * ((1 - std::exp(-StepDt * 14))
                            * (1 - GaitEase((Phase - .5) / .5)));
                    }
                    Step.To.Y = SupportHeight(Side, Step.To, Foot.DesiredRotation)
                        + Foot.Seed.SoleHeightMetres * Scale;
                    const double Swing = Clamp((Phase - .12) / .88, 0, 1);
                    const double Blend = GaitEase(Swing);
                    Contact.TargetMetres = Step.From + (Step.To - Step.From) * Blend;
                    Contact.Orientation = FQuat::Slerp(Step.Rotation, Foot.DesiredRotation, Blend);
                    Contact.PitchRadians = GaitSwingPitch(Phase, Step.TakeoffPitch);
                    const double Clearance = std::min((.09 + RunWeight * .13) * Scale,
                        FlatDistance(Step.From, Step.To) * (.2 + RunWeight * .16));
                    const double Arc = std::sin(Swing * Pi);
                    Contact.TargetMetres.Y = std::max(Contact.TargetMetres.Y + Arc * Arc * Clearance,
                        SupportHeight(Side, Contact.TargetMetres, Contact.Orientation)
                            + Foot.Seed.SoleHeightMetres * Scale);
                    if (Phase == 1)
                    {
                        Foot.Planted = Contact.TargetMetres;
                        Foot.Rotation = Contact.Orientation;
                        Contact.bStepping = false;
                        Contact.SwingProgress = 0;
                        Contact.Phase = EGaitPhase::HeelStrike;
                        Foot.ContactAge = 0;
                        Foot.StanceDuration = Step.Duration;
                        Foot.bRolling = true;
                        Contact.Lock = 1;
                    }
                }
                else
                {
                    Foot.ContactAge += StepDt;
                    const double Progress = std::min(1.0, Foot.ContactAge / Foot.StanceDuration);
                    Contact.Phase = Foot.bRolling ? GaitStancePhase(Progress) : EGaitPhase::MidStance;
                    const double Pitch = Foot.bRolling
                        ? GaitStancePitch(bLocomoting ? Progress : std::min(.7, Progress)) : 0;
                    Contact.PitchRadians = Damp(Contact.PitchRadians, Pitch, 24, StepDt);
                    Contact.Lock = 1;
                }
                if (Frame.Footwork.IsSet() && !bLocomoting)
                {
                    const FGaitFootwork& Footwork = Frame.Footwork.GetValue();
                    const FVector& Offset = Side == 0 ? Footwork.LeftMetres : Footwork.RightMetres;
                    Contact.TargetMetres = Foot.Neutral + Forward * (Offset.Z * Scale);
                    Contact.TargetMetres.X += Forward.Z * Offset.X * Scale;
                    Contact.TargetMetres.Z -= Forward.X * Offset.X * Scale;
                    Contact.TargetMetres.Y = SupportHeight(Side, Contact.TargetMetres, Foot.DesiredRotation)
                        + (Foot.Seed.SoleHeightMetres + Offset.Y) * Scale;
                    Contact.Orientation = Foot.DesiredRotation;
                    Foot.Planted = Contact.TargetMetres;
                    Foot.Rotation = Contact.Orientation;
                    Contact.bStepping = false;
                    Contact.SwingProgress = 0;
                    Foot.bRolling = false;
                    Contact.PitchRadians = 0;
                    Contact.Lock = 1;
                    StopSteps = 2;
                }
            }
        }

        const bool bStepping = Feet[0].Contact.bStepping || Feet[1].Contact.bStepping;
        Weight = Damp(Weight, bLocomoting || bStepping ? 1 : 0, 9, Dt);
        double Rise = std::numeric_limits<double>::infinity();
        for (int32 Side = 0; Side < 2; ++Side)
        {
            const FFoot& Foot = Feet[Side];
            double FootRise = Foot.Planted.Y - Foot.Seed.SoleHeightMetres * Scale - Frame.FloorYMetres;
            if (Foot.Contact.bStepping)
            {
                const double Blend = GaitEase(std::min(1.0, Foot.Step.Age / Foot.Step.Duration));
                FootRise = (1 - Blend) * (Foot.Step.From.Y - Foot.Seed.SoleHeightMetres * Scale)
                    + Blend * SupportHeight(Side, Foot.Step.To, Foot.DesiredRotation) - Frame.FloorYMetres;
            }
            Rise = std::min(Rise, FootRise);
            if (!Finite(Foot.Contact.TargetMetres) || !Rotation(Foot.Contact.Orientation)
                || !std::isfinite(Foot.Contact.PitchRadians) || !std::isfinite(FootRise)
                || !std::isfinite(Foot.Contact.SwingProgress)) { Reset(); return false; }
        }
        if (!bSupportValid) { Reset(); return false; }
        Out.Feet[0] = Feet[0].Contact;
        Out.Feet[1] = Feet[1].Contact;
        Out.VelocityMps = Velocity;
        Out.DeltaSeconds = Dt;
        Out.SpeedMps = Speed;
        Out.Weight = Weight;
        Out.RunWeight = RunWeight;
        Out.StartAgeSeconds = StartAge;
        Out.BankRadians = Bank;
        Out.AccelerationLeanRadians = AccelerationLean;
        Out.LayoutRiseMetres = Rise;
        Out.PreviousStep = PreviousStep;
        Out.bLocomoting = bLocomoting;
        Out.bTravelling = bTravelling;
        Out.bContinueMotion = bLocomoting || bStepping || Weight > .02;
        return true;
    }
}
