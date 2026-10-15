#pragma once

#include "BaddieCompanionMovementPresentation.h"

namespace BaddieCompanion
{
    struct FWorldPresentationTime
    {
        double ServerNowMs = 0;
        int32 RemainingMs = 0;
    };

    // Ports the world store's display-time receipt, using Unreal's monotonic clock.
    // This is not a grant, network synchronizer, timer or admission owner. Call only
    // on the game thread, after the owner has rejected stale session callbacks.
    class BADDIECOMPANION_API FWorldPresentationClock
    {
    public:
        // A changed room clears its receipt. Clear explicitly on access loss,
        // disposal or rejoining the same room under a new session generation.
        void SelectGameRoom(const FString& GameRoomId);
        void Clear();

        // A stale/equal server time never refreshes the receipt's 60-second life.
        // Returns false without changing the current receipt for rejected input.
        bool Observe(const FString& ResponseGameRoomId, double ServerNowMs);

        // The session retains this anchor on the original capture-producing ticket.
        // Manifest/page completion and result lookup must not supply a new anchor.
        // ServerNowMs remains the original receipt value, without elapsed time added.
        bool ObserveCaptured(const FString& ResponseGameRoomId, double ServerNowMs,
            double TicketReceivedAtSeconds);

        // Requires the current player's room to match. Failure clears Out.
        bool CopyTime(const FString& PlayerGameRoomId, FWorldPresentationTime& Out) const;

        // Sample unchanged server-time routes. The caller must separately establish
        // player/instance identity, revision, sequence and movement ownership.
        // Predicted local-clock routes must not be passed to this function.
        bool SampleRoute(const FString& PlayerGameRoomId,
            const FMovementRoute& Movement, FMovementSample& Out) const;

    private:
        struct FReceipt
        {
            double ServerNowMs = 0;
            double ReceivedAtSeconds = 0;
        };
        FString SelectedGameRoomId;
        TOptional<FReceipt> Receipt;
    };
}
