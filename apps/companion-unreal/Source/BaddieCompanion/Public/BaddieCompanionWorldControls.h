#pragma once

#include "BaddieCompanionWorldPlayer.h"

namespace BaddieCompanion
{
    // Selected read-side transport fields only. The decoded player keeps its original
    // wire fields and observations; this view neither changes them nor grants controls.
    struct FWorldTransportRead
    {
        FString UserId, CharacterId, GameRoomId, InstanceId;
        int64 VisitRevision = 0;
        FMovementPose Position;
        // Missing and null wire controls both become absent at this read stage,
        // matching admitCompanionWorldControls. No on-foot permission follows.
        TOptional<FWorldDrivingControl> Driving;
        TOptional<FWorldWaterControl> WaterControl;
    };

    // Pure translation of the street store's transport read checks and older-receipt
    // retention. Inputs come from the existing bounded player/instance decoders.
    // Previous must be a result from this function in the same live session. The owner
    // clears it on grant, account, life or visit loss, including same-room re-entry.
    // CurrentLifeId comes from separately accepted recovery state. Empty permits the
    // canonical peer-read path; it does not establish a known life or control authority.
    // A missing player succeeds with null Out. Failures clear Out and use fixed text.
    // Shared inputs are passed by value so Previous may also be the caller's Out.
    // No expiry extension, clock update, prediction, vehicle simulation or request occurs.
    BADDIECOMPANION_API bool ReconcileWorldTransportRead(
        TSharedPtr<const FWorldPlayerView> Player,
        TSharedPtr<const FWorldCurrentInstanceIdentity> Instance,
        TSharedPtr<const FWorldTransportRead> Previous,
        FString CurrentLifeId,
        TSharedPtr<const FWorldTransportRead>& Out, FString& OutError);
}
