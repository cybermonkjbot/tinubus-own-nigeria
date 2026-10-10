#pragma once

#include "CoreMinimal.h"
#include "BaddieCompanionSession.h"

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
}
