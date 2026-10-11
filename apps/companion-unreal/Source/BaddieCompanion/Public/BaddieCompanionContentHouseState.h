#pragma once

#include "CoreMinimal.h"

namespace BaddieCompanion
{
    struct FContentHouseParticipant
    {
        FString UserId, Name;
        bool bIsYou = false, bAllowed = false;
    };
    struct FContentHousePermission
    {
        FString PermissionKey;
        int64 ExpiresAtMs = 0;
        bool bAllowed = false;
        TArray<FContentHouseParticipant> Participants;
    };
    // Endpoint value only. Atomic bounded publication; failure clears Out. No authority is inferred.
    BADDIECOMPANION_API bool DecodeContentHousePermission(const FString& Json,
        TSharedPtr<const FContentHousePermission>& Out);

    enum class EContentHousePurpose : uint8 { Capture, SaveDraft, Publish };
    enum class EContentHouseOperation : uint8 { Snapshot, SetPermission, Revoke, AuthorizePhoto };
    enum class EContentHouseError : uint8 { InvalidResponse, InvalidClock, ReadFailed, ChangeFailed, PermissionChanged };
    BADDIECOMPANION_API const TCHAR* ContentHouseErrorText(EContentHouseError Error);
    struct FContentHouseView
    {
        // Retains the server fields, including individual consent after display expiry.
        TSharedPtr<const FContentHousePermission> Permission;
        // Descriptive display state only. Neither this nor bAllowed enables capture or publication.
        bool bEffectiveAllowed = false, bBusy = false;
        TOptional<EContentHouseError> ReadError, CommandError;
    };
    class FContentHouseOwnerReceipt
    {
        friend class FContentHouseState;
        FGuid Generation;
    };
    class FContentHouseSubscription
    {
        friend class FContentHouseState;
        FGuid Owner, Visit, Generation;
    };
    class FContentHouseIntent
    {
    public:
        // A captured local description, not permission/currentness or proof of dispatch.
        bool IsValid() const { return Request.IsValid(); }
        EContentHouseOperation GetKind() const { return Kind; }
        const FString& GetOperation() const { return Operation; }
        const FString& GetArgumentsJson() const { return ArgumentsJson; }
    private:
        friend class FContentHouseState;
        FGuid Owner, Visit, Subscription, Observation, Request;
        EContentHouseOperation Kind = EContentHouseOperation::Snapshot;
        EContentHousePurpose Purpose = EContentHousePurpose::Capture;
        FString Operation, ArgumentsJson, PermissionKey;
    };
    enum class EContentHouseCompletion : uint8 { Accepted, Stale, InvalidResponse, PermissionChanged, Failed };

    // Single-thread caller-driven state. No transport, subscriptions, timers, credentials,
    // readiness input, native-grant admission, pixels, publication or gate activation.
    // Account endpoints require the original account/session/device adapter externally;
    // current native gameplay grants cannot dispatch these descriptions.
    class BADDIECOMPANION_API FContentHouseState
    {
    public:
        FContentHouseState() = default;
        FContentHouseState(const FContentHouseState&) = delete;
        FContentHouseState& operator=(const FContentHouseState&) = delete;
        const FContentHouseView& GetView() const { return View; }
        // Every call replaces the account/session lifetime, including the same user ID.
        FContentHouseOwnerReceipt BeginOwner(const FString& UserId);
        // Always retires the old visit, including replacement with the same instance/revision.
        bool BeginVisit(const FContentHouseOwnerReceipt& Owner, const FString& InstanceId, int64 VisitRevision);
        void EndVisit(const FContentHouseOwnerReceipt& Owner);
        FContentHouseSubscription BeginSubscription(const FContentHouseOwnerReceipt& Owner);
        // Call before unsubscribe, on last-observer removal, or on feature disablement.
        // Clears visit-bound reads/work, but retains an original-account revoke intent.
        void StopSubscription(const FContentHouseSubscription& Subscription);

        // Capture before the read, or at ordered subscription ingress before asynchronous
        // processing. Only the latest captured read can complete; arrival order is not a clock.
        // Every accepted observation replaces the prior one and cancels its authorization check.
        bool Snapshot(const FContentHouseSubscription& Subscription, double DisplayNowMs, FContentHouseIntent& Out);
        EContentHouseCompletion AcceptSnapshot(const FContentHouseIntent& Intent, const FString& Json, double DisplayNowMs);
        EContentHouseCompletion AcceptReadFailure(const FContentHouseIntent& Intent);
        // Caller supplies display time while observed and at authorization boundaries.
        // Expiry is sticky for this observation. A local clock is not server permission.
        bool ObserveClock(const FContentHouseSubscription& Subscription, double DisplayNowMs);

        // At most one mutation intent. SetPermission requires an active visit subscription.
        // No optimistic consent, transport retries or query-cache invalidation. Null success
        // discards local permission and outstanding reads, requiring a subsequent snapshot.
        bool SetPermission(const FContentHouseOwnerReceipt& Owner, bool bAllowCaptureAndShare, FContentHouseIntent& Out);
        // Requires only the original account lifetime, even after travel or subscription loss.
        bool Revoke(const FContentHouseOwnerReceipt& Owner, FContentHouseIntent& Out);
        bool AuthorizePhoto(const FContentHouseOwnerReceipt& Owner, const FString& PermissionKey,
            EContentHousePurpose Purpose, double DisplayNowMs, FContentHouseIntent& Out);
        EContentHouseCompletion AcceptNullResult(const FContentHouseIntent& Intent, const FString& Json);
        // Accepted consumes only this request for this exact purpose and immediate caller stage.
        // It stores no reusable grant. Recheck separately before pixels and after pixels,
        // then for save_draft/publish and every existing remote upload/commit stage.
        EContentHouseCompletion AcceptAuthorization(const FContentHouseIntent& Intent,
            EContentHousePurpose ExpectedPurpose, const FString& Json, double DisplayNowMs);
        EContentHouseCompletion AcceptCommandFailure(const FContentHouseIntent& Intent);
        // Only the exact pending receipt can cancel. Cancelling a read also clears permission
        // and its bound authorization. Caller owns actual abort/unsubscribe.
        bool Cancel(const FContentHouseIntent& Intent);
        void Dispose();

    private:
        bool CurrentOwner(const FContentHouseOwnerReceipt& Owner) const;
        bool CurrentSubscription(const FContentHouseSubscription& Subscription) const;
        bool CurrentRead(const FContentHouseIntent& Intent) const;
        bool CurrentCommand(const FContentHouseIntent& Intent) const;
        bool ApplyClock(double DisplayNowMs);
        bool CaptureCommand(EContentHouseOperation Kind, EContentHousePurpose Purpose,
            bool bAllowCaptureAndShare, FContentHouseIntent& Out);
        void FinishCommand();
        void ClearPermission();
        void StopReads();

        FString OwnerId, InstanceId;
        int64 VisitRevision = 0;
        FGuid OwnerGeneration, VisitGeneration, SubscriptionGeneration, ObservationGeneration, ActiveRead;
        TOptional<FContentHouseIntent> ActiveCommand;
        FContentHouseView View;
        bool bExpired = false, bDisposed = false;
    };
}
