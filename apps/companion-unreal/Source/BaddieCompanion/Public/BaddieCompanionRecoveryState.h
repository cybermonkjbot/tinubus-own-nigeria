#pragma once

#include "BaddieCompanionWorldPlayer.h"
#include "Misc/Guid.h"

namespace BaddieCompanion
{
    DECLARE_MULTICAST_DELEGATE(FRecoveryReadChanged);
    enum class ECompanionHealthStatus : uint8 { Active, Hospital, Detained };
    struct FCompanionRecoveryDestination
    {
        FString PlaceId, InstanceId;
        FWorldScenePoint Position;
        double ReadyAtMs = 0;
    };
    struct FCompanionHealthView
    {
        FString LifeId;
        int64 Revision = 0;
        double Health = 0, MaxHealth = 0, ProtectedUntilMs = 0;
        ECompanionHealthStatus Status = ECompanionHealthStatus::Active;
        TOptional<FString> DeathKey;
        TOptional<FCompanionRecoveryDestination> Recovery;
    };
    enum class ERecoveryResponseKind : uint8 { Subscription, Mutation };
    class FRecoveryDecodedResponse;
    // Subscription value is health/null; mutation value is { health, serverTime }.
    BADDIECOMPANION_API bool DecodeRecoveryResponse(const FString& Json, ERecoveryResponseKind Kind,
        FRecoveryDecodedResponse& Out, FString& OutError);
    class BADDIECOMPANION_API FRecoveryDecodedResponse
    {
    public:
        bool IsValid() const { return bValid && bHasHealth == Health.IsValid(); }
        ERecoveryResponseKind GetKind() const { return Kind; }
        const TSharedPtr<const FCompanionHealthView>& GetHealth() const { return Health; }
        double GetServerTimeMs() const { return ServerTimeMs; }
    private:
        friend bool DecodeRecoveryResponse(const FString&, ERecoveryResponseKind, FRecoveryDecodedResponse&, FString&);
        bool bValid = false;
        bool bHasHealth = false;
        ERecoveryResponseKind Kind = ERecoveryResponseKind::Subscription;
        TSharedPtr<const FCompanionHealthView> Health;
        double ServerTimeMs = 0;
    };
    class FRecoveryReceipt
    {
        friend class FRecoveryReadState;
        FGuid Generation, MutationId;
        ERecoveryResponseKind Kind = ERecoveryResponseKind::Subscription;
        FString LifeId;
        double BeganAtMs = 0;
    };
    struct FRecoveryReadView
    {
        FString OwnerId;
        EWorldFieldPresence HealthPresence = EWorldFieldPresence::Missing;
        TSharedPtr<const FCompanionHealthView> Health;
        bool bReady = false, bBusy = false;
        TOptional<FString> Error;
        double ServerOffsetMs = 0;
    };
    enum class ERecoveryReadResult : uint8 { Accepted, OlderRevision, StaleReceipt, InvalidResponse };
    // No timer, medic eligibility, economy, commands, credentials or world-player mutation.
    class BADDIECOMPANION_API FRecoveryReadState
    {
    public:
        FRecoveryReadState() = default;
        FRecoveryReadState(const FRecoveryReadState&) = delete;
        FRecoveryReadState& operator=(const FRecoveryReadState&) = delete;
        const FRecoveryReadView& GetView() const { return View; }
        FRecoveryReadChanged& OnChanged() { return Changed; }
        void SelectOwner(const FString& OwnerId);
        FRecoveryReceipt BeginSubscription();
        void EndSubscription();
        // Receipt capture only, after separate external admission. Never permission to recover.
        TOptional<FRecoveryReceipt> CaptureRecovery(double BeganAtMs);
        ERecoveryReadResult Accept(const FRecoveryReceipt& Receipt, const FRecoveryDecodedResponse& Response,
            double ReceivedAtMs);
        bool AcceptFailure(const FRecoveryReceipt& Receipt);
        void Clear();
        void Dispose();
    private:
        bool Current(const FRecoveryReceipt& Receipt) const;
        FRecoveryReadView View;
        FGuid Generation, ActiveMutation;
        bool bSubscribed = false, bDisposed = false;
        FRecoveryReadChanged Changed;
    };
}
