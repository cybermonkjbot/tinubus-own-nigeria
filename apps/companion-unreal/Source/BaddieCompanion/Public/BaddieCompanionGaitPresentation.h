#pragma once

#include "CoreMinimal.h"

namespace BaddieCompanion
{
    enum class EGaitPhase : uint8 { HeelStrike, Loading, MidStance, HeelOff, ToeOff, Swing };

    struct FGaitStrideTiming
    {
        double DurationSeconds = 0;
        double LeadMetres = 0;
    };

    BADDIECOMPANION_API double GaitEase(double Progress);
    BADDIECOMPANION_API EGaitPhase GaitStancePhase(double Progress);
    BADDIECOMPANION_API double GaitStancePitch(double Progress);
    BADDIECOMPANION_API double GaitSwingPitch(double Progress, double TakeoffPitch);
    BADDIECOMPANION_API bool GaitStrideTiming(double SpeedMps, double Scale, FGaitStrideTiming& Out);

    // All vectors and quaternions stay in the source game's metres and Y-up axes.
    // Feet are ordered left, right. Seeds come from the unmodified authored pose.
    struct FGaitFootSeed
    {
        FVector NeutralLocalMetres = FVector::ZeroVector;
        FQuat RestRotation = FQuat::Identity;
        double SoleHeightMetres = 0;
    };

    struct FGaitFootwork
    {
        FVector LeftMetres = FVector::ZeroVector;
        FVector RightMetres = FVector::ZeroVector;
    };

    struct FGaitFrameInput
    {
        double DeltaSeconds = 0;
        double FloorYMetres = 0;
        FVector RootScale = FVector(1, 1, 1);
        FVector RootPositionMetres = FVector::ZeroVector;
        FQuat RootRotation = FQuat::Identity;
        bool bMoving = false;
        // Used only to distinguish continuous travel from a teleport. Foot cadence
        // uses measured root displacement, never a requested walking speed.
        double AcceptedSpeedMps = 0;
        FGaitFootSeed Feet[2];
        TOptional<FGaitFootwork> Footwork;
    };

    struct FGaitFootContact
    {
        FVector TargetMetres = FVector::ZeroVector;
        FQuat Orientation = FQuat::Identity;
        EGaitPhase Phase = EGaitPhase::MidStance;
        double PitchRadians = 0;
        double Lock = 1;
        bool bStepping = false;
        double SwingProgress = 0;
    };

    struct FGaitContactFrame
    {
        FGaitFootContact Feet[2];
        FVector VelocityMps = FVector::ZeroVector;
        double DeltaSeconds = 0;
        double SpeedMps = 0;
        double Weight = 0;
        double RunWeight = 0;
        double StartAgeSeconds = 0;
        double BankRadians = 0;
        double AccelerationLeanRadians = 0;
        double LayoutRiseMetres = 0;
        int32 PreviousStep = 1;
        bool bLocomoting = false;
        bool bTravelling = false;
        bool bContinueMotion = false;
    };

    /**
     * Contact-state port of groundedGait.ts, through advanceFeet and layout rise.
     * LayoutSupport must evaluate the source floorHeightAt contract for the named
     * foot's sole footprint, retaining FloorYMetres when no floor patch applies.
     * Output precedes rendered-ground probes, sole clearance, body offsets and IK.
     * It neither moves a root nor grants route authority. No live consumer is bound.
     */
    using FGaitLayoutSupport = TFunctionRef<double(int32, const FVector&, const FQuat&)>;

    class BADDIECOMPANION_API FGaitPresentation
    {
    public:
        // Call on room/rig/visibility changes or lost admission, even without a frame.
        void Reset();
        // Invalid input or support clears both state and output. Seeds are captured
        // when starting or resetting; supply the pre-gait pose on every frame.
        bool Update(const FGaitFrameInput& Frame, FGaitLayoutSupport LayoutSupport,
            FGaitContactFrame& Out);

    private:
        struct FStep
        {
            FVector From = FVector::ZeroVector;
            FVector To = FVector::ZeroVector;
            FQuat Rotation = FQuat::Identity;
            double Age = 0;
            double Duration = 0;
            double TakeoffPitch = 0;
            bool bBraking = false;
        };

        struct FFoot
        {
            FGaitFootSeed Seed;
            FVector Planted = FVector::ZeroVector;
            FQuat Rotation = FQuat::Identity;
            FVector EndNeutral = FVector::ZeroVector;
            FVector Neutral = FVector::ZeroVector;
            FQuat DesiredRotation = FQuat::Identity;
            FGaitFootContact Contact;
            double ContactAge = 1;
            double StanceDuration = .32;
            bool bRolling = false;
            FStep Step;
        };

        FFoot Feet[2];
        bool bRootKnown = false;
        bool bWasMoving = false;
        FVector PreviousPosition = FVector::ZeroVector;
        double PreviousYaw = 0;
        double PreviousSpeed = 0;
        double StartAge = 0;
        double Bank = 0;
        double AccelerationLean = 0;
        double Weight = 0;
        double RunWeight = 0;
        int32 PreviousStep = 1;
        int32 StopSteps = 0;
    };
}
