#pragma once

#include "CoreMinimal.h"
#include "BaddieCompanionSession.h"
#include "BaddieCompanionGameplaySession.h"

namespace BaddieCompanion
{
    inline bool IsValidConnectionDeviceName(const FString& DeviceName)
    {
        const FString Name = DeviceName.TrimStartAndEnd();
        if (Name.IsEmpty() || Name.Len() > 80) return false;
        for (const TCHAR Ch : Name) if (Ch < 32 || Ch == 127) return false;
        return true;
    }

    inline bool IsConnectionPending(EConnectionState State)
    {
        return State == EConnectionState::RequestingCode
            || State == EConnectionState::AwaitingApproval
            || State == EConnectionState::LoadingHome;
    }

    inline bool IsGameplayPairing(EGameplayConnectionState State)
    {
        return State == EGameplayConnectionState::RequestingCode || State == EGameplayConnectionState::AwaitingApproval;
    }

    inline bool CanPairGameplay(const FGameplayConnectionView& View)
    {
        return View.RetryAfterSeconds <= 0 && (View.State == EGameplayConnectionState::SignedOut
            || View.State == EGameplayConnectionState::Unavailable || View.State == EGameplayConnectionState::Expired);
    }

    inline bool CanEnterGameplay(const FGameplayConnectionView& View)
    {
        return View.Scope == EGameplayScope::EntryOnly
            && View.State == EGameplayConnectionState::Authorized && View.GrantOrPairingRemainingSeconds > 0
            && View.RetryAfterSeconds <= 0 && !View.bStale;
    }

    inline bool CanDisconnectGameplay(const FGameplayConnectionView& View)
    {
        return View.State == EGameplayConnectionState::Authorized || View.State == EGameplayConnectionState::Entering
            || View.State == EGameplayConnectionState::Joined || View.State == EGameplayConnectionState::Leaving
            || View.State == EGameplayConnectionState::EntryLost;
    }

    inline bool SameGameplayControls(const FGameplayConnectionView& A, const FGameplayConnectionView& B)
    {
        return A.State == B.State && A.Scope == B.Scope && A.WorldGeneration == B.WorldGeneration
            && A.WorldStatus == B.WorldStatus && A.DeviceName == B.DeviceName && A.DisplayCode == B.DisplayCode
            && A.ApprovalLink == B.ApprovalLink && A.Error == B.Error && A.bStale == B.bStale
            && (A.GrantOrPairingRemainingSeconds > 0) == (B.GrantOrPairingRemainingSeconds > 0)
            && (A.PresenceRemainingSeconds > 0) == (B.PresenceRemainingSeconds > 0)
            && (A.RetryAfterSeconds > 0) == (B.RetryAfterSeconds > 0);
    }
}
