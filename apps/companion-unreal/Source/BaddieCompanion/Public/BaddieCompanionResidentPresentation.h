#pragma once

#include "BaddieCompanionResidentState.h"

namespace BaddieCompanion
{
    struct FResidentPlaceSample
    {
        FResidentPose Pose;
        bool bMoving = false;
        bool bWorking = false;
    };

    // Pure projection of one typed resident into a resolved place binding. Retains
    // source metres, Y up, yaw radians and fractional server milliseconds.
    // The caller admits world/epoch and resolves the frozen-layout binding first.
    // No map pose, membership settlement, target readiness or authority is produced.
    // False clears Out and sets a fixed diagnostic; true clears OutError.
    // Reads finish before output/error writes, including aliased input fields.
    BADDIECOMPANION_API bool SampleResidentInPlace(const FResidentView& Resident,
        const FResidentPlaceBinding& Binding, double ServerNowMs,
        FResidentPlaceSample& Out, FString& OutError);
}
