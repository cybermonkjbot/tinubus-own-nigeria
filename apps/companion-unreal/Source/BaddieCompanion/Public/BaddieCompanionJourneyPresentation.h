#pragma once

#include "BaddieCompanionWorldPlayer.h"

namespace BaddieCompanion
{
    struct FWorldJourneyPresentationTiming
    {
        double Progress = 0, RemainingMs = 0;
        bool bAtCheckpoint = false, bArrivalDue = false;
    };
    struct FWorldJourneyPresentationSample
    {
        FWorldJourneyGeoPoint Point;
        FWorldJourneyPresentationTiming Timing;
    };

    // Pure saved-receipt presentation. ServerNowMs is caller-supplied real time.
    // Timing does not inspect geometry. ArrivalDue never dispatches or settles arrival.
    // Failure clears all output with fixed text, after finishing input reads.
    BADDIECOMPANION_API bool CalculateWorldJourneyTiming(const FWorldJourneyView& Journey, double ServerNowMs,
        FWorldJourneyPresentationTiming& Out, FString& OutError);
    // Geographic degrees and original retained length. No route repair or axis conversion.
    // Out may alias a point in Route; OutError may alias an input string.
    BADDIECOMPANION_API bool SampleWorldJourneyRoute(const FWorldJourneyRouteIdentity& Route, double Progress,
        FWorldJourneyGeoPoint& Out, FString& OutError);
    BADDIECOMPANION_API bool SampleWorldJourney(const FWorldJourneyView& Journey, double ServerNowMs,
        FWorldJourneyPresentationSample& Out, FString& OutError);
}
