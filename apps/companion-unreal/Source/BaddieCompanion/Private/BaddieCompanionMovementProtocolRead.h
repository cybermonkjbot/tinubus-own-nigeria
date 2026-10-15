#pragma once

#include "BaddieCompanionMovementProtocol.h"

namespace BaddieCompanion
{
    namespace MovementProtocolPrivate
    {
        // Shared wire readers, implemented in MovementProtocol.cpp. Callers provide
        // size-bounded UE JSON and a fresh candidate. Failure may leave partial data;
        // the enclosing endpoint decoder owns atomic publication and failure clearing.
        // Source metres/radians and optional mechanical state remain unchanged.
        // These reads do not establish driving ownership, admission or simulation.
        bool ReadDrivingPose(const TSharedPtr<FJsonObject>& Object, FWorldDrivingPose& Out);
        bool ReadDrivingControl(const TSharedPtr<FJsonObject>& Object, FWorldDrivingControl& Out);
    }
}
