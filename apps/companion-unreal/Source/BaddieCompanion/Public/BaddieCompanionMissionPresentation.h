#pragma once

#include "CoreMinimal.h"

namespace BaddieCompanion
{
    // Native, process-local presentation contract. No credentials or actor/hidden-position fields.
    struct FMissionIdentity
    {
        FString AccountId;
        FString CompanionId;
        FString EncounterId;
        uint64 Generation = 0;

        bool IsValid() const
        {
            return Generation != 0 && !AccountId.IsEmpty() && !CompanionId.IsEmpty() && !EncounterId.IsEmpty();
        }
        bool operator==(const FMissionIdentity& Other) const
        {
            return Generation == Other.Generation && AccountId.Equals(Other.AccountId, ESearchCase::CaseSensitive)
                && CompanionId.Equals(Other.CompanionId, ESearchCase::CaseSensitive)
                && EncounterId.Equals(Other.EncounterId, ESearchCase::CaseSensitive);
        }
    };

    struct FMissionPresentationToken
    {
        FMissionIdentity Identity;
        uint64 Presentation = 0;
        bool operator==(const FMissionPresentationToken& Other) const
        {
            return Presentation == Other.Presentation && Identity == Other.Identity;
        }
    };

    enum class EMissionAction : uint8 { ClearBrush, Untie };
    enum class EMissionActionState : uint8 { Hidden, Available, Pending, Failed };
    enum class EMissionFireSource : uint8 { Manual, HipAssistance };

    struct FMissionSelectedAction
    {
        FString TargetId;
        uint64 Revision = 0;
        EMissionAction Action = EMissionAction::ClearBrush;
        // Supplied by the admitted mission owner after selection/range/equipment checks.
        bool bAcceptedReachable = false;
        // Local monotonic expiry, not server time. The owner must refresh or invalidate it.
        double ValidUntilSeconds = 0;
        bool SameTarget(const FMissionSelectedAction& Other) const
        {
            return TargetId.Equals(Other.TargetId, ESearchCase::CaseSensitive)
                && Revision == Other.Revision && Action == Other.Action;
        }
    };

    struct FMissionActionRequest
    {
        FMissionPresentationToken Token;
        FMissionSelectedAction Selection;
        uint64 RequestId = 0;
    };

    struct FMissionCombatPolicy
    {
        double ValidUntilSeconds = 0;
        bool bManualFireAvailable = false;
        bool bADSAvailable = false;
        bool bADS = false;
        bool bWeaponRaised = false;
        bool bFiringStance = false;
        FString WeaponId;
        // All supplied by combat's current visible crosshair selection, never by hearing.
        FString TargetId;
        bool bVisible = false;
        bool bEligibleHostile = false;
        bool bUnderCrosshair = false;
        bool bWithinWeaponCloseRange = false;
        bool bWeaponAllowsAssistance = false;
    };

    struct FMissionHudView
    {
        FMissionPresentationToken Token;
        EMissionActionState ActionState = EMissionActionState::Hidden;
        EMissionAction Action = EMissionAction::ClearBrush;
        FString TargetId;
        uint64 TargetRevision = 0;
        bool bCanAct = false;
        bool bCanFire = false;
        bool bCanADS = false;
        bool bModal = false;
        bool bHipAssistance = false;
    };

    // Unbound by default. The parent gameplay owner alone supplies this admission callback.
    DECLARE_DELEGATE_RetVal_OneParam(bool, FMissionAdmission, FMissionIdentity&);
    DECLARE_DELEGATE_TwoParams(FMissionVectorInput, const FMissionPresentationToken&, FVector2D);
    DECLARE_DELEGATE_RetVal_TwoParams(bool, FMissionADSInput, const FMissionPresentationToken&, bool);
    DECLARE_DELEGATE_RetVal_ThreeParams(bool, FMissionFireInput, const FMissionPresentationToken&, bool, EMissionFireSource);
    DECLARE_DELEGATE_RetVal_OneParam(bool, FMissionActionInput, const FMissionActionRequest&);
    DECLARE_DELEGATE_OneParam(FMissionActionCancel, const FMissionActionRequest&);
}
