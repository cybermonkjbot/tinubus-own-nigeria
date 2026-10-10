#include "BaddieCompanionWorldClock.h"

#include "HAL/PlatformTime.h"

#include <cmath>

namespace BaddieCompanion
{
    namespace
    {
        constexpr double WorldClockMaxReceiptAgeMs = 60000;
        constexpr double WorldClockMaxSafeInteger = 9007199254740991;
    }

    void FWorldPresentationClock::SelectGameRoom(const FString& GameRoomId)
    {
        if (SelectedGameRoomId.Equals(GameRoomId, ESearchCase::CaseSensitive)) return;
        Clear();
        SelectedGameRoomId = GameRoomId;
    }

    void FWorldPresentationClock::Clear()
    {
        Receipt.Reset();
        SelectedGameRoomId.Reset();
    }

    bool FWorldPresentationClock::Observe(const FString& ResponseGameRoomId, double ServerNowMs)
    {
        if (SelectedGameRoomId.IsEmpty()
            || !SelectedGameRoomId.Equals(ResponseGameRoomId, ESearchCase::CaseSensitive)
            || !std::isfinite(ServerNowMs) || ServerNowMs <= 0 || ServerNowMs > WorldClockMaxSafeInteger
            || std::floor(ServerNowMs) != ServerNowMs
            || (Receipt.IsSet() && ServerNowMs <= Receipt.GetValue().ServerNowMs)) return false;
        const double ReceivedAtSeconds = FPlatformTime::Seconds();
        if (!std::isfinite(ReceivedAtSeconds)) return false;
        Receipt = FReceipt{ServerNowMs, ReceivedAtSeconds};
        return true;
    }

    bool FWorldPresentationClock::CopyTime(const FString& PlayerGameRoomId,
        FWorldPresentationTime& Out) const
    {
        Out = {};
        if (!Receipt.IsSet() || SelectedGameRoomId.IsEmpty()
            || !SelectedGameRoomId.Equals(PlayerGameRoomId, ESearchCase::CaseSensitive)) return false;
        const auto& Current = Receipt.GetValue();
        const double ElapsedMs = (FPlatformTime::Seconds() - Current.ReceivedAtSeconds) * 1000;
        if (!std::isfinite(ElapsedMs)) return false;
        const double AgeMs = std::floor(FMath::Max(0.0, ElapsedMs));
        if (AgeMs > WorldClockMaxReceiptAgeMs) return false;
        const double ServerNowMs = Current.ServerNowMs + AgeMs;
        if (!std::isfinite(ServerNowMs) || ServerNowMs > WorldClockMaxSafeInteger) return false;
        Out = {ServerNowMs, static_cast<int32>(WorldClockMaxReceiptAgeMs - AgeMs)};
        return true;
    }

    bool FWorldPresentationClock::SampleRoute(const FString& PlayerGameRoomId,
        const FMovementRoute& Movement, FMovementSample& Out) const
    {
        Out = {};
        FWorldPresentationTime Time;
        return CopyTime(PlayerGameRoomId, Time)
            && SampleMovement(Movement, Time.ServerNowMs, Out);
    }
}
