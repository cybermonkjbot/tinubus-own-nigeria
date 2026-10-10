#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BaddieCompanionSnapshot.h"
#include "BaddieCompanionSession.generated.h"

class FJsonObject;
struct FBaddieCompanionSessionData;

namespace BaddieCompanion
{
    enum class EConnectionScope : uint8 { Home, Apartment };

    enum class EConnectionState : uint8
    {
        Disabled, SignedOut, RequestingCode, AwaitingApproval, LoadingHome, Ready, Unavailable, Expired
    };

    // Presentation may copy this view. It never contains an account token or device proof.
    struct FConnectionView
    {
        EConnectionState State = EConnectionState::Disabled;
        EConnectionScope Scope = EConnectionScope::Home;
        FString DeviceName;
        FString DisplayCode;
        FString ApprovalLink;
        FString Error;
        double RemainingSeconds = 0;
        // Manual begin throttling only. Existing Connect gating retains this meaning.
        double RetryAfterSeconds = 0;
        // Existing exchange/snapshot error backoff, not routine polling. Zero while
        // requesting, after clearing, or when expiry occurs before another attempt.
        double AutomaticRetryAfterSeconds = 0;
        bool bStale = false;
    };
}

DECLARE_MULTICAST_DELEGATE(FBaddieCompanionConnectionChanged);

/** C++-only owner of short-lived, read-only grants. No SaveGame or Blueprint credentials. */
UCLASS()
class BADDIECOMPANION_API UBaddieCompanionSession : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UBaddieCompanionSession();
    virtual ~UBaddieCompanionSession() override;
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // Trusted application configuration only. Both client and backend default to disabled.
    void Configure(const FString& ConvexOrigin, bool bEnabled);
    void BeginPairing(const FString& DeviceName,
        BaddieCompanion::EConnectionScope Scope = BaddieCompanion::EConnectionScope::Home);
    void Disconnect();
    void RefreshSnapshot();
    BaddieCompanion::FConnectionView GetConnectionView() const;
    // Clears Out on failure. Callers must discard their previous private presentation then.
    bool CopySnapshot(BaddieCompanion::FSnapshotSelection& Out) const;
    FBaddieCompanionConnectionChanged OnConnectionChanged;

private:
    TUniquePtr<FBaddieCompanionSessionData> Data;
    FTSTicker::FDelegateHandle Ticker;

    bool TickSession(float DeltaSeconds);
    void ClearPrivateState();
    void Stop(BaddieCompanion::EConnectionState State, const TCHAR* Message);
    void BestEffortDisconnect();
    void Send(const FString& Operation, const TSharedRef<FJsonObject>& Args);
    void AcceptResult(const FString& Operation, const TSharedPtr<FJsonObject>& Result, double ElapsedSeconds);
    void TransportFailed(const FString& Operation);
};
