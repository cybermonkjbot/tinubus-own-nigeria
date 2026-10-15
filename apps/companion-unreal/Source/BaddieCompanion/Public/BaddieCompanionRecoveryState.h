#pragma once

#include "BaddieCompanionWorldPlayer.h"
#include "Misc/Guid.h"

class FJsonValue;

namespace BaddieCompanion
{
    class FWorldSession;
    namespace WorldObservationPrivate { class FPublicationPreparation; }
    struct FWorldRecoveryReceipt
    {
    private:
        friend class FRecoveryReadState;
        FGuid Generation;
        uint64 Lifetime = 0;
    };
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
        friend bool ReadWorldRecoveryValue(const TSharedPtr<FJsonValue>&, ERecoveryResponseKind,
            FRecoveryDecodedResponse&, FString&);
        bool bValid = false;
        bool bHasHealth = false;
        ERecoveryResponseKind Kind = ERecoveryResponseKind::Subscription;
        TSharedPtr<const FCompanionHealthView> Health;
        double ServerTimeMs = 0;
    };
    class FRecoveryReceipt
    {
    public:
        // Empty for the legacy externally admitted receipt-only API.
        const FString& GetArgumentsJson() const { return ArgumentsJson; }
    private:
        friend class FRecoveryReadState;
        FGuid Generation, MutationId;
        ERecoveryResponseKind Kind = ERecoveryResponseKind::Subscription;
        FString OwnerId, LifeId, ArgumentsJson;
        double BeganAtMs = 0;
        bool bOwnedPreparation = false;
    };
    class FRecoveryDispatch
    {
    public:
        const TCHAR* GetEndpoint() const { return TEXT("companionRecovery:recover"); }
        const FString& GetArgumentsJson() const { return Receipt.GetArgumentsJson(); }
        const FRecoveryReceipt& GetReceipt() const { return Receipt; }
    private:
        friend class FRecoveryReadState;
        FRecoveryReceipt Receipt;
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
    // Local request ownership only. No timer, medic eligibility, economy, transport,
    // credentials, recovery permission or world-player mutation.
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
        // Captures the currently displayed life; ServiceId is exact, nonempty and <=100.
        // Returned arguments may be inspected for separate adapter admission, not sent yet.
        TOptional<FRecoveryReceipt> PrepareRecovery(const FString& ServiceId, double BeganAtMs);
        // Marks one owned request. On success, send returned arguments immediately without
        // a callback/asynchronous gap. This method sends nothing and does not notify.
        TOptional<FRecoveryDispatch> DispatchRecovery(const FRecoveryReceipt& Receipt);
        bool AbandonPreparedRecovery(const FRecoveryReceipt& Receipt);
        ERecoveryReadResult Accept(const FRecoveryReceipt& Receipt, const FRecoveryDecodedResponse& Response,
            double ReceivedAtMs);
        bool AcceptFailure(const FRecoveryReceipt& Receipt);
        void Clear();
        void Dispose();
    private:
        friend class FWorldSession;
        friend class WorldObservationPrivate::FPublicationPreparation;
        bool BeginWorldSubscription(uint64 Lifetime, FWorldRecoveryReceipt& Out);
        bool StageWorldSubscription(const FWorldRecoveryReceipt& Receipt, FRecoveryReadView& Next) const;
        bool CurrentWorldSubscription(const FWorldRecoveryReceipt& Receipt) const;
        void CommitWorldSubscription(const FWorldRecoveryReceipt& Receipt, FRecoveryReadView&& Next, FRecoveryReadView& Retired);
        void ClearWorldSubscription(FRecoveryReadView& Retired, bool bEndLifetime = true);
        void NotifyWorldSubscription(const FWorldRecoveryReceipt& Receipt);
        bool Current(const FRecoveryReceipt& Receipt) const;
        bool CanComplete(const FRecoveryReceipt& Receipt) const;
        TOptional<FRecoveryReceipt> CaptureRecoveryInternal(double BeganAtMs, const TOptional<FString>& ServiceId);
        void RetireRecovery();
        FRecoveryReadView View;
        FGuid Generation, ActiveMutation;
        TOptional<FRecoveryReceipt> PreparedRecovery;
        bool bRecoveryDispatched = false;
        bool bSubscribed = false, bDisposed = false;
        uint64 WorldLifetime = 0;
        FRecoveryReadChanged Changed;
    };
}
