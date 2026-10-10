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
        bool bReady = false;
        TOptional<FString> Error;
        // No busy/retryable flags. Pending commands and their error composition are deferred.
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

    // Single owning thread, caller-driven reads. No requests, listeners, timers, commands,
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
        // Replaces the account lifetime, even for the same ID. Invalidates every old read.
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
    private:
        bool CurrentOwner(const FPosOwnerReceipt& Owner) const;
        bool Current(const FPosCatalogReadIntent& Intent) const;
        bool Current(const FPosReceiptReadIntent& Intent) const;
        void CloseCatalog();
        void CloseReceipt();
        EPosReadResult AcceptReceipt(TSharedPtr<const FPosReceipt> Receipt);
        FGuid OwnerGeneration, CatalogGeneration, ReceiptGeneration;
        TOptional<FPosScope> Selected;
        TOptional<FString> RecoveringId;
        FPosReadView View;
        bool bObserved = false, bDisposed = false;
    };
}
