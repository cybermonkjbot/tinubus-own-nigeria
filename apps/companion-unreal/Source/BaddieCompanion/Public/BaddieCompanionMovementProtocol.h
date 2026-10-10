#pragma once

#include "BaddieCompanionMovementPresentation.h"

namespace BaddieCompanion
{
    // Source metres/radians. The owner supplies the current scope and sequence.
    struct FMovementRequest
    {
        int64 Sequence = 0;
        int64 RoomRevision = 0;
        FMovementPose Destination;
        bool bStop = false;
        EMovementPace Pace = EMovementPace::Unpaced;
        // Only companionWorld:move accepts these fields, not chatCallSpaces:move.
        TOptional<TArray<FMovementPoint>> Path;
        TOptional<FMovementCollisionStop> Collision;
    };

    struct FCallMovementResult
    {
        bool bOk = false;
        // A declined first request can report sequence zero and no route.
        int64 Sequence = 0;
        int64 RoomRevision = 0;
        FMovementPose Position;
        TOptional<FMovementRoute> Movement;
        double RetryAfterMs = 0;
        TOptional<FString> Reason;
    };

    // Encode only movement arguments with UE JSON. No credentials, lease IDs,
    // game-room identity or network request are created here. Call encoding rejects
    // world-only fields rather than silently dropping a proposed path or receipt.
    // The admitted transport must add its own scope and enforce lifecycle rules.
    BADDIECOMPANION_API bool EncodeCallMovementRequest(
        const FMovementRequest& Request, TSharedPtr<FJsonObject>& Out, FString& OutError);
    BADDIECOMPANION_API bool EncodeWorldMovementRequest(
        const FMovementRequest& Request, TSharedPtr<FJsonObject>& Out, FString& OutError);

    // Input must already be size-bounded and decoded by the transport. Preserves
    // server timestamps and declines; success means a decoded result, not bOk,
    // admission or acceptance in the current scope. Clears Out on failure.
    // World movement returns a full player/serverNow response, not this contract.
    BADDIECOMPANION_API bool DecodeCallMovementResult(
        const TSharedPtr<FJsonObject>& Object, FCallMovementResult& Out, FString& OutError);
}
