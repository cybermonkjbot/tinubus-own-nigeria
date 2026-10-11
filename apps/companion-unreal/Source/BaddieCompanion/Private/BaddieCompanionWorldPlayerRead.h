#pragma once

#include "BaddieCompanionWorldPlayer.h"
#include "BaddieCompanionRecoveryState.h"
#include "BaddieCompanionRacingState.h"

class FJsonObject;
class FJsonValue;

namespace BaddieCompanion
{
    // Semantic entry points shared with the legacy text decoders. The segmented
    // owner must qualify and charge a projection before allocating these objects.
    // These functions do not establish permission, timing or resource admission.
    bool ReadWorldPlayerObject(const TSharedPtr<FJsonObject>& Player, int64 ServerNowMs,
        TSharedPtr<const FWorldPlayerView>& Out, FString& OutError);
    bool ReadWorldInstanceObject(const TSharedPtr<FJsonObject>& Instance, const FWorldPlayerView& Player,
        TSharedPtr<const FWorldCurrentInstanceIdentity>& Out, FString& OutError);
    bool ReadWorldRecoveryValue(const TSharedPtr<FJsonValue>& Value, ERecoveryResponseKind Kind,
        FRecoveryDecodedResponse& Out, FString& OutError);
    ERacingReadResult ReadWorldRacingValue(EWorldFieldPresence Presence, TSharedPtr<FJsonValue> Value,
        FString ExpectedInstanceId, TSharedPtr<const FRacingSnapshot> Previous,
        TSharedPtr<const FRacingSnapshot>& Out, FString& OutError);
}
