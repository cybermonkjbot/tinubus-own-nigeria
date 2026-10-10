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

    enum class EWorldPlayerStatus : uint8 { Present, Traveling, Recovering };
    struct FWorldMovementResult
    {
        // Scope identifiers are private process-memory data, not proof of an admitted visit.
        FString UserId, CharacterId, GameRoomId, PlaceId, InstanceId, HomeRoomId;
        int64 Revision = 0;
        int64 MoveSequence = 0;
        int64 ServerNowMs = 0;
        FMovementPose Position;
        TOptional<FMovementRoute> Movement;
        EWorldPlayerStatus Status = EWorldPlayerStatus::Present;
        double Health = 0;
        // Retain the full player document, including omitted versus null control state.
        // Nested journey/vehicle/water/activity state is not decoded or admitted by this helper.
        // No logging, Blueprint, SaveGame or persistent storage of this document.
        FString PlayerDocumentJson;
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
    BADDIECOMPANION_API bool DecodeCallMovementResult(
        const TSharedPtr<FJsonObject>& Object, FCallMovementResult& Out, FString& OutError);

    // Input must already be size-bounded and decoded by the transport.
    // World movement returns player/serverNow, not ok/retryAfterMs. Decode movement fields
    // and retain the complete player document; nested player schemas need their own adapters.
    // An older moveSequence can be a soft decline, and a stop may return a vehicle-owned
    // player unchanged. Neither a decoded result nor null movement grants on-foot control.
    // The owner must enforce generation, visit/instance, revision and controller ownership
    // before accepting the result or updating the world presentation clock. Clears Out on failure.
    BADDIECOMPANION_API bool DecodeWorldMovementResult(
        const TSharedPtr<FJsonObject>& Object, FWorldMovementResult& Out, FString& OutError);
}
