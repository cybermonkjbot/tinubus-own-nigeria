#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Templates/PimplPtr.h"
#include "BaddieCompanionWorldObservation.h"
#include "BaddieCompanionGameplaySession.generated.h"

class FJsonObject;
class APlayerController;
struct FBaddieCompanionGameplaySessionData;

namespace BaddieCompanion
{
    enum class EGameplayScope : uint8 { EntryOnly, World };
    enum class EGameplayConnectionState : uint8
    {
        Disabled, SignedOut, RequestingCode, AwaitingApproval, Authorized,
        Entering, Joined, Leaving, EntryLost, Unavailable, Expired
    };

    // Status only. Joined means call presence, never world/mission/render readiness.
    struct FGameplayConnectionView
    {
        EGameplayConnectionState State = EGameplayConnectionState::Disabled;
        FString DeviceName, DisplayCode, ApprovalLink, Error;
        double GrantOrPairingRemainingSeconds = 0;
        double PresenceRemainingSeconds = 0;
        double RetryAfterSeconds = 0;
        bool bStale = false;
        EGameplayScope Scope = EGameplayScope::EntryOnly;
        uint64 WorldGeneration = 0;
        EWorldReadStatus WorldStatus = EWorldReadStatus::Unavailable;
    };

    enum class EGameplayRequest : uint8 { None, Begin, Exchange, Enter, Heartbeat, Leave };
}

DECLARE_MULTICAST_DELEGATE(FBaddieCompanionGameplayConnectionChanged);

/** Separate in-memory gameplay capability owner. No account tokens or gameplay controls. */
UCLASS()
class BADDIECOMPANION_API UBaddieCompanionGameplaySession : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UBaddieCompanionGameplaySession();
    virtual ~UBaddieCompanionGameplaySession() override;
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // Explicit trusted configuration only. Initialize reads no account-linking flag.
    // This source adds no caller and defaults to disabled.
    void Configure(const FString& ConvexOrigin, bool bEnabled);
    void BeginPairing(const FString& DeviceName);
    void BeginWorldPairing(const FString& DeviceName);
    void Enter(const FString& CrewThreadId = FString());
    void Leave();
    void Disconnect();
    BaddieCompanion::FGameplayConnectionView GetConnectionView() const;
    BaddieCompanion::FWorldSessionView GetWorldView() const;
    void AttachWorldConsumer(APlayerController* Consumer);
    void DetachWorldConsumer(APlayerController* Consumer);
    void ReloadWorld(APlayerController* Consumer);
    bool RequestWorldWalk(APlayerController* Consumer, const BaddieCompanion::FMovementPoint& Target,
        BaddieCompanion::EMovementPace Pace);
    void StopWorldWalk(APlayerController* Consumer);
    void WorldContentLost(APlayerController* Consumer, uint64 ObservationGeneration);
    FBaddieCompanionGameplayConnectionChanged OnConnectionChanged;

private:
    TPimplPtr<FBaddieCompanionGameplaySessionData> Data;
    FTSTicker::FDelegateHandle Ticker;
    FDelegateHandle DeactivateHandle, ReactivateHandle, BackgroundHandle, ForegroundHandle, SlateActivationHandle;

    bool TickSession(float DeltaSeconds);
    void TickWorld();
    bool IsForeground() const;
    void AttachSlateLifecycle();
    void ApplicationDeactivated();
    void ApplicationReactivated();
    void ApplicationBackgrounded();
    void ApplicationForegrounded();
    void SlateActivationChanged(bool bActive);
    void FocusLost();
    void RetireRequest();
    void ClearPrivateState();
    void Stop(BaddieCompanion::EGameplayConnectionState State, const TCHAR* Message);
    bool CheckLifetimes();
    void BeginLeave(const TCHAR* Message);
    void Send(BaddieCompanion::EGameplayRequest Operation);
    void AcceptResult(BaddieCompanion::EGameplayRequest Operation,
        const TSharedPtr<FJsonObject>& Result, double ElapsedSeconds);
    void TransportFailed(BaddieCompanion::EGameplayRequest Operation);
    void Schedule(BaddieCompanion::EGameplayRequest Operation, double DelaySeconds, const TCHAR* Message);
};
