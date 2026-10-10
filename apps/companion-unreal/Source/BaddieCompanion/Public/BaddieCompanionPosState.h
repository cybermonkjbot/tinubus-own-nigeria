#pragma once

#include "CoreMinimal.h"

namespace BaddieCompanion
{
    struct FPosScope
    {
        FString GameRoomId, InstanceId, LifeId, ServiceId;
        int64 Revision = 0;
    };
    enum class EPosStatus : uint8 { Quoted, Pending, Completed, Cancelled, Interrupted, Expired };
    enum class EPosSourceKind : uint8 { BankedGameValue };
    enum class EPosUnit : uint8 { LagosCredit };
    enum class EPosSourceCoverage : uint8 { Complete, Bounded };
    enum class EPosCashConversion : uint8 { Disabled };
    enum class EPosDeposit : uint8 { OwnHomeOnly };
    struct FPosSource
    {
        EPosSourceKind Kind = EPosSourceKind::BankedGameValue;
        EPosUnit Key = EPosUnit::LagosCredit;
    };
    struct FPosReceipt
    {
        FString Id, RequestId, OutingId;
        FPosScope Scope;
        int64 OutingRevision = 0;
        FPosSource Source;
        EPosUnit UnitKey = EPosUnit::LagosCredit;
        double Amount = 0, Fee = 0, Total = 0;
        EPosStatus Status = EPosStatus::Quoted;
        double IssuedAtMs = 0, ExpiresAtMs = 0;
        // All six keys are REQUIRED on the wire. Unset means explicit null, not omission.
        TOptional<double> ConfirmedAtMs, ReadyAtMs, SettledAtMs;
        TOptional<FString> UtilityEventId, Reason, GrantedEntryId;
    };
    struct FPosCatalog
    {
        FPosScope Scope;
        FString OutingId;
        int64 OutingRevision = 0;
        double AvailableCash = 0, Fee = 0, EligibleBankedCredits = 0;
        TArray<double> Amounts;
        EPosSourceCoverage BankedSourceCoverage = EPosSourceCoverage::Complete;
        EPosCashConversion CashConversion = EPosCashConversion::Disabled;
        EPosDeposit Deposit = EPosDeposit::OwnHomeOnly;
        int64 ServerNowMs = 0;
    };
    BADDIECOMPANION_API bool SamePosScope(const FPosScope& A, const FPosScope& B);
    // Decode endpoint values, not Convex envelopes. Atomic const publication, bounded
    // input and fixed diagnostics. Receipt success with null Out means authoritative null.
    // These functions validate data, not account ownership, admission or dispatch rights.
    BADDIECOMPANION_API bool DecodePosCatalog(const FString& Json,
        TSharedPtr<const FPosCatalog>& Out, FString& OutError);
    BADDIECOMPANION_API bool DecodePosReceipt(const FString& Json,
        TSharedPtr<const FPosReceipt>& Out, FString& OutError);

    enum class EPosReceiptPresence : uint8 { Missing, Null, Value };
    struct FPosReadView
    {
        TSharedPtr<const FPosCatalog> Catalog;
        TSharedPtr<const FPosReceipt> Receipt;
        EPosReceiptPresence ReceiptPresence = EPosReceiptPresence::Missing;
        // Canonical ready belongs to the catalog, never to receipt recovery.
        bool bReady = false, bBusy = false, bRetryable = false;
        TOptional<FString> Error;
    };
    class FPosOwnerReceipt
    {
        friend class FPosReadState;
        FGuid Generation;
    };
    class FPosCatalogReadIntent
    {
    public:
        // Capture marker only. The caller still needs an admitted transport and current owner.
        bool IsValid() const { return ReadGeneration.IsValid(); }
        const FPosScope& GetScope() const { return Scope; }
        int64 GetNowMs() const { return NowMs; }
    private:
        friend class FPosReadState;
        FGuid OwnerGeneration, ReadGeneration;
        FPosScope Scope;
        int64 NowMs = 0;
    };
    enum class EPosReceiptReadKind : uint8 { Latest, ById };
    class FPosReceiptReadIntent
    {
    public:
        bool IsValid() const { return ReadGeneration.IsValid(); }
        BADDIECOMPANION_API EPosReceiptReadKind GetKind() const;
        // ById only. Latest has no quoteId argument and no selected-service scope.
        FString GetQuoteId() const { return RecoveringId.IsSet() ? RecoveringId.GetValue() : FString(); }
    private:
        friend class FPosReadState;
        FGuid OwnerGeneration, ReadGeneration;
        TOptional<FString> RecoveringId;
    };
    struct FPosPublicFailure
    {
        // Already sanitized by toSafeConvexError or a faithful transport equivalent.
        // Never pass raw exceptions, credentials, request bodies or arbitrary server text.
        // The reducer bounds this text but does not perform transport error sanitization.
        FString Message;
    };
    enum class EPosReadResult : uint8 { Accepted, Retained, StaleRead, IdentityMismatch, InvalidResponse };

    enum class EPosCommandKind : uint8 { Invalid, Quote, Confirm, Settle, Cancel };
    struct FPosCommand
    {
        EPosCommandKind Kind = EPosCommandKind::Invalid;
        // Quote only, any finite number. The backend owns offered amounts and affordability.
        TOptional<double> Amount;
        // Confirm/settle/cancel only.
        TOptional<FString> QuoteId;
        // Confirm only. Explicit presence and true are required; never inferred or defaulted.
        TOptional<bool> AcceptsCarriedLoss;
    };
    class FPosCommandIntent
    {
    public:
        // Capture marker, not dispatch/admission. Default and copied stale intents cannot complete.
        bool IsValid() const { return AttemptId.IsValid(); }
    private:
        friend class FPosReadState;
        FGuid OwnerGeneration, PacketId, AttemptId;
    };
    struct FPosReadRestarts
    {
        // Obligations for the current owner, not requests or permission. Use its existing
        // Begin*Read methods, which recheck owner/observation/selection and capture fresh time.
        bool bCatalog = false, bReceipt = false;
    };
    struct FPosCommandDispatch
    {
        // Immutable const publication from MarkCommandDispatched. Non-auth arguments only.
        // Adapter supplies authorized account credentials; no automatic retries/cache invalidation.
        FString Operation, ArgumentsJson;
        FPosReadRestarts RestartReads;
    };
    enum class EPosCommandFailureKind : uint8
    {
        Unclassified, NetworkUnavailable, GenericSaveFailure, Definite
    };
    struct FPosCommandFailure
    {
        // Classify BEFORE sanitization using isNetworkUnavailableError, or the exact
        // Error-instance message "Unable to save changes right now. Please try again.".
        // Definite means neither predicate matched. Unclassified is rejected, not guessed.
        EPosCommandFailureKind Kind = EPosCommandFailureKind::Unclassified;
        FPosPublicFailure PublicError;
    };
    enum class EPosCommandResult : uint8 { Accepted, InvalidResponse, StaleAttempt };

    // Single owning thread, caller-driven reads and command ownership. No network, listeners, timers,
    // balance mutations, world/service resolution, utility progression or native UI.
    class BADDIECOMPANION_API FPosReadState
    {
    public:
        FPosReadState() = default;
        FPosReadState(const FPosReadState&) = delete;
        FPosReadState& operator=(const FPosReadState&) = delete;
        FPosReadState(FPosReadState&&) = delete;
        FPosReadState& operator=(FPosReadState&&) = delete;
        FPosReadView GetView() const { return View; }
        bool HasPendingCommand() const { return Pending.IsValid(); }
        bool HasUncertainCommand() const { return Pending.IsValid() && bPendingUncertain; }
        // Replaces the account lifetime, even for the same ID. Invalidates all reads/attempts.
        FPosOwnerReceipt BeginOwner(const FString& AccountId);
        void Clear();
        void Dispose();
        // Models first/last observer. Teardown retains values but retires both read slots.
        void SetObserved(const FPosOwnerReceipt& Owner, bool bObserved);
        // Only an external current-world/service producer may supply the selected scope.
        // Invalid scope leaves selection unchanged. Null clears catalog, not account receipt.
        bool Select(const FPosOwnerReceipt& Owner, const TOptional<FPosScope>& Scope);
        // Empty ID selects latest, matching canonical truthiness. Same ID is a no-op.
        bool Recover(const FPosOwnerReceipt& Owner, const FString& QuoteId);
        void Reload(const FPosOwnerReceipt& Owner);
        // Each begin retires only its own slot. Caller stops old subscriptions and opens
        // new ones after select/recover/reload/observer changes. No automatic dispatch.
        // Catalog time is captured once as floor(ServerNowMs), a positive safe integer.
        bool BeginCatalogRead(const FPosOwnerReceipt& Owner, double ServerNowMs, FPosCatalogReadIntent& Out);
        bool BeginReceiptRead(const FPosOwnerReceipt& Owner, FPosReceiptReadIntent& Out);
        // Intents can deliver multiple subscription observations until invalidated.
        EPosReadResult ObserveCatalog(const FPosCatalogReadIntent& Intent, const FString& Json);
        EPosReadResult ObserveReceipt(const FPosReceiptReadIntent& Intent, const FString& Json);
        bool ObserveCatalogFailure(const FPosCatalogReadIntent& Intent, const FPosPublicFailure& Failure);
        bool ObserveReceiptFailure(const FPosReceiptReadIntent& Intent, const FPosPublicFailure& Failure);

        // Prepare captures an immutable packet and reserves the busy latch before caller notification.
        // Initial preparation is UNSENT, not uncertain. Retry retains the original uncertain packet.
        // No observation/catalog readiness is required here; transport admission is still external.
        bool PrepareCommand(const FPosOwnerReceipt& Owner, const FPosCommand& Command,
            const FString& RequestId, FPosCommandIntent& Out);
        bool PrepareRetry(const FPosOwnerReceipt& Owner, FPosCommandIntent& Out);
        // After notification, recheck and mark once. The admitted adapter must send these exact
        // arguments immediately, without yield/notification, then service RestartReads.
        // A failed mark leaves prepared work for AbandonUnsentCommand; it does not fake a failure.
        bool MarkCommandDispatched(const FPosCommandIntent& Intent, TSharedPtr<const FPosCommandDispatch>& Out);
        bool AbandonUnsentCommand(const FPosCommandIntent& Intent);
        // Only marked current attempts can settle. Both outcome methods retain busy until finally.
        // On success, service receipt restart first, then catalog restart, and notify before
        // FinishCommand. On failure, notify before FinishCommand. If notification replaces
        // the owner, the old finally is a no-op.
        EPosCommandResult AcceptCommandResult(const FPosCommandIntent& Intent, const FString& Json, FPosReadRestarts& Out);
        bool AcceptCommandFailure(const FPosCommandIntent& Intent, const FPosCommandFailure& Failure);
        // Evaluate canonical accepted-or-returned receipt after outcome notification, then release
        // busy and reconcile subscription evidence. Null Out is the catch result. Notify again
        // after this returns. Rejects unmarked, duplicate and retired-account finally calls.
        bool FinishCommand(const FPosCommandIntent& Intent, TSharedPtr<const FPosReceipt>& Out);
    private:
        bool CurrentOwner(const FPosOwnerReceipt& Owner) const;
        bool Current(const FPosCatalogReadIntent& Intent) const;
        bool Current(const FPosReceiptReadIntent& Intent) const;
        void CloseCatalog();
        void CloseReceipt();
        EPosReadResult AcceptReceipt(TSharedPtr<const FPosReceipt> Receipt);
        struct FPendingPacket
        {
            FGuid Id;
            FPosScope Scope;
            FPosCommand Command;
            FString RequestId, Operation, ArgumentsJson;
        };
        enum class EAttemptPhase : uint8 { None, Prepared, Dispatched, AwaitingFinally };
        bool CurrentAttempt(const FPosCommandIntent& Intent) const;
        bool InitialScopeMatches() const;
        void PrepareAttempt(FPosCommandIntent& Out);
        void ClearAttempt();
        void ReconcileReceipt();
        void RecordCommandFailure(bool bUncertain, FString Message);
        FGuid OwnerGeneration, CatalogGeneration, ReceiptGeneration;
        TOptional<FPosScope> Selected;
        TOptional<FString> RecoveringId;
        FPosReadView View;
        TSharedPtr<const FPendingPacket> Pending;
        bool bPendingUncertain = false;
        FGuid ActiveAttempt, ActivePacket;
        EAttemptPhase AttemptPhase = EAttemptPhase::None;
        TSharedPtr<const FPosReceipt> ReturnedReceipt;
        bool bObserved = false, bDisposed = false;
    };
}
