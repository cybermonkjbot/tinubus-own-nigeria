#pragma once

#include "CoreMinimal.h"
#include "Misc/Guid.h"

namespace BaddieCompanion
{
    enum class EGameEconomyPriceUnit : uint8 { RealCurrencyMinor, GameValueUnit };
    enum class EGameEconomyMainSource : uint8
    {
        Handyman, Partner, DeclutterCustomer, RewardsMember, ReferralPartner, ContentCreator, InfluenceCreator
    };
    struct FGameEconomyPrice
    {
        EGameEconomyPriceUnit Unit = EGameEconomyPriceUnit::GameValueUnit;
        int64 Amount = 0;
        // Real currency is exactly NGN/main_balance; game value is banked_game_value.
        TOptional<EGameEconomyMainSource> MainSource;
        FString UnitKey, BankedSourceKey;
    };
    enum class EGameEconomyActivation : uint8 { Disabled, Enabled };
    enum class EGameEconomyConversion : uint8 { PendingApproval, NotApplicable, Active };
    enum class EGameEconomyApproval : uint8 { PendingApproval, Approved };
    enum class EGameEconomyWithdrawal : uint8 { Unavailable, Available };
    enum class EGameEconomyPolicyRead : uint8 { Current, Historical };
    struct FGameEconomyExchangeRate
    {
        double CreditsPerNaira = 0, CreditsPerKobo = 0;
    };
    struct FGameEconomyPolicy
    {
        EGameEconomyActivation LiveFinancialActivation = EGameEconomyActivation::Disabled;
        EGameEconomyConversion ConversionStatus = EGameEconomyConversion::PendingApproval;
        EGameEconomyApproval FundingStatus = EGameEconomyApproval::PendingApproval;
        EGameEconomyApproval PaymentSourceStatus = EGameEconomyApproval::PendingApproval;
        EGameEconomyWithdrawal WithdrawalStatus = EGameEconomyWithdrawal::Unavailable;
        TOptional<FGameEconomyExchangeRate> ExchangeRate;
        EGameEconomyPolicyRead Provenance = EGameEconomyPolicyRead::Current;
    };
    struct FGameEconomyService
    {
        FString OfferId, ServiceKey, NpcKey, PlaceKey, Label;
        int64 Revision = 0, AvailabilityRevision = 0;
        FGameEconomyPrice Price;
        // Wire beneficiary is exactly self. No other beneficiary can decode.
        int64 OfferStartsAtMs = 0, OfferExpiresAtMs = 0, QuoteTtlMs = 0;
        bool bAvailable = false;
        FGameEconomyPolicy Policy;
    };
    enum class EGameEconomyQuoteStatus : uint8 { Quoted, ApprovalRequired, Cancelled, Fulfilled };
    struct FGameEconomyQuote
    {
        FString Id, TermsFingerprint;
        FGameEconomyService Service;
        int64 IssuedAtMs = 0, ExpiresAtMs = 0, Revision = 0;
        EGameEconomyQuoteStatus Status = EGameEconomyQuoteStatus::Quoted;
        TOptional<int64> ConfirmedAtMs, CancelledAtMs;
    };
    struct FGameEconomyCatalog
    {
        TArray<FGameEconomyService> Services;
        EGameEconomyActivation LiveFinancialActivation = EGameEconomyActivation::Disabled;
    };
    struct FGameEconomyVendorScope
    {
        FString GameRoomId, InstanceId, LifeId;
        int64 Revision = 0, EquipmentRevision = 0;
    };
    struct FGameEconomyVendorItem
    {
        FString ServiceKey, Label;
        FGameEconomyPrice Price;
        int64 Stock = 0;
        bool bOwned = false;
    };
    struct FGameEconomyVendor
    {
        TArray<FGameEconomyVendorItem> Services;
        bool bReady = false, bBusy = false;
        // Endpoint requires null. Only caller-reported read failures populate this text.
        TOptional<FString> Error;
    };

    // Endpoint values only, with atomic failure clearing and fixed public diagnostics.
    // All endpoint roots are objects; null, missing and malformed input fail.
    BADDIECOMPANION_API bool DecodeGameEconomyPrice(const FString& Json, FGameEconomyPrice& Out, FString& OutError);
    // Historical reads accept only the exact old disabled-finance policy for provenance.
    // Service/catalog/quote readers ALWAYS require Current, never Historical.
    BADDIECOMPANION_API bool DecodeGameEconomyPolicy(const FString& Json, EGameEconomyPriceUnit Unit,
        EGameEconomyPolicyRead Read, FGameEconomyPolicy& Out, FString& OutError);
    BADDIECOMPANION_API bool DecodeGameEconomyService(const FString& Json, FGameEconomyService& Out, FString& OutError);
    BADDIECOMPANION_API bool DecodeGameEconomyQuote(const FString& Json, FGameEconomyQuote& Out, FString& OutError);
    BADDIECOMPANION_API bool DecodeGameEconomyCatalog(const FString& Json, FGameEconomyCatalog& Out, FString& OutError);
    BADDIECOMPANION_API bool DecodeGameEconomyVendor(const FString& Json, FGameEconomyVendor& Out, FString& OutError);

    struct FGameEconomyReadView
    {
        TArray<FGameEconomyService> Services;
        TOptional<FGameEconomyQuote> Quote;
        // Unset vendor means absent, not a known empty vendor result.
        TOptional<FGameEconomyVendor> Vendor;
        EGameEconomyActivation LiveFinancialActivation = EGameEconomyActivation::Disabled;
        bool bReady = false, bQuoteReady = true, bBusy = false;
        TOptional<FString> Error;
    };
    enum class EGameEconomyReadKind : uint8 { Catalog, Quote, Vendor };
    enum class EGameEconomyReadResult : uint8 { Accepted, Retained, StaleReceipt, InvalidResponse };
    class FGameEconomyReadReceipt
    {
        friend class FGameEconomyReadState;
    public:
        EGameEconomyReadKind GetKind() const { return Kind; }
        FString GetPlaceKey() const { return PlaceKey; }
        FString GetNpcKey() const { return NpcKey; }
        TOptional<FString> GetQuoteId() const { return QuoteId; }
        TOptional<FGameEconomyVendorScope> GetVendorScope() const { return VendorScope; }
        TOptional<int64> GetVendorNowMs() const { return VendorNowMs; }
    private:
        FGuid AccountGeneration, ReadGeneration;
        EGameEconomyReadKind Kind = EGameEconomyReadKind::Catalog;
        FString PlaceKey, NpcKey;
        TOptional<FString> QuoteId;
        TOptional<FGameEconomyVendorScope> VendorScope;
        TOptional<int64> VendorNowMs;
    };

    enum class EGameEconomySelectorKind : uint8 { OfferId, ServiceKey };
    struct FGameEconomySelector
    {
        EGameEconomySelectorKind Kind = EGameEconomySelectorKind::OfferId;
        FString Key;
    };
    enum class EGameEconomyCommand : uint8
    {
        RequestQuote, ConfirmQuote, CancelQuote, RequestWeaponQuote, ConfirmWeaponQuote
    };
    class FGameEconomyCommandDispatch
    {
        friend class FGameEconomyReadState;
    public:
        FString GetEndpoint() const { return Endpoint; }
        FString GetArgumentsJson() const { return ArgumentsJson; }
    private:
        FString Endpoint, ArgumentsJson;
    };
    class FGameEconomyCommandReceipt
    {
        friend class FGameEconomyReadState;
    private:
        FGuid Id, AccountGeneration, ViewGeneration, VendorViewGeneration, PendingVendorId;
        EGameEconomyCommand Operation = EGameEconomyCommand::RequestQuote;
        // The current attempt's target can differ from the saved vendor wire payload.
        FString AttemptedValue;
    };
    class FGameEconomyCommandCompletion
    {
        friend class FGameEconomyReadState;
    public:
        bool ShouldRestartQuoteRead() const { return bRestartQuoteRead; }
        bool ShouldNotifyBeforeFinish() const { return bNotifyBeforeFinish; }
    private:
        FGuid Id, AccountGeneration;
        TOptional<FGameEconomyQuote> Result;
        bool bRestartQuoteRead = false, bNotifyBeforeFinish = false;
    };

    // Account-private single-game-thread read/command owner, using UE value types and GUIDs.
    // No subscriptions, networking, timers, money/equipment writes or admission.
    class BADDIECOMPANION_API FGameEconomyReadState
    {
    public:
        FGameEconomyReadState() = default;
        FGameEconomyReadState(const FGameEconomyReadState&) = delete;
        FGameEconomyReadState& operator=(const FGameEconomyReadState&) = delete;
        FGameEconomyReadState(FGameEconomyReadState&&) = delete;
        FGameEconomyReadState& operator=(FGameEconomyReadState&&) = delete;
        FGameEconomyReadView GetView() const { return View; }
        bool SelectAccount(const FString& AccountId);
        bool BeginObservation();
        // Last listener retires callbacks but retains selected IDs and displayed data.
        void EndObservation();
        bool SelectCatalog(const FString& PlaceKey, const FString& NpcKey);
        bool RecoverQuote(const TOptional<FString>& QuoteId);
        // Caller supplies validated world/life/equipment identity; not service admission.
        bool SelectVendor(const TOptional<FGameEconomyVendorScope>& Scope);
        void Reload();
        void Clear();
        void Dispose();
        // Open/reuse local read receipts; the caller owns actual subscriptions and teardown.
        bool CaptureCatalog(FGameEconomyReadReceipt& Out);
        bool CaptureQuote(FGameEconomyReadReceipt& Out);
        // Fresh subscription uses the caller's captured Date.now-equivalent milliseconds.
        // Reusing its receipt preserves that first time; this never advances a game clock.
        bool CaptureVendor(int64 NowMs, FGameEconomyReadReceipt& Out);
        EGameEconomyReadResult Observe(const FGameEconomyReadReceipt& Receipt, const FString& Json);
        // Public error text only. Length is capped here; sanitization belongs to the caller.
        bool ObserveFailure(const FGameEconomyReadReceipt& Receipt, const FString& PublicMessage);
        // Prepare before notifying listeners. Success reserves the shared busy latch.
        bool PrepareRequestQuote(const FGameEconomySelector& Selector, const FString& RequestId, FGameEconomyCommandReceipt& Out);
        bool PrepareConfirmQuote(const FString& Id, const FString& RequestId, FGameEconomyCommandReceipt& Out);
        bool PrepareCancelQuote(const FString& Id, const FString& RequestId, FGameEconomyCommandReceipt& Out);
        bool PrepareRequestWeaponQuote(const FString& ServiceKey, const FString& RequestId, FGameEconomyCommandReceipt& Out);
        bool PrepareConfirmWeaponQuote(const FString& Id, const FString& RequestId, FGameEconomyCommandReceipt& Out);
        // Recheck immediately before sending returned immutable arguments, with no yield or notification.
        // View/read changes affect response relevance, not dispatch ownership. Credentials stay outside.
        bool MarkCommandDispatched(const FGameEconomyCommandReceipt& Receipt, FGameEconomyCommandDispatch& Out);
        // Unsent only. Retains the vendor packet, including one reused from an unresolved attempt.
        bool AbandonPreparedCommand(const FGameEconomyCommandReceipt& Receipt);
        // True consumes one callback and leaves busy held. False rejects receipt without state changes.
        bool ObserveCommand(const FGameEconomyCommandReceipt& Receipt, const FString& Json, FGameEconomyCommandCompletion& Out);
        bool ObserveCommandFailure(const FGameEconomyCommandReceipt& Receipt, const FString& PublicMessage, FGameEconomyCommandCompletion& Out);
        // Reconcile the selected quote read and notify as flagged, then finalize once and emit.
        // Out is the detached raw response, not a newer retained display. Account clear invalidates it.
        bool FinalizeCommand(const FGameEconomyCommandCompletion& Completion, TOptional<FGameEconomyQuote>& Out);
    private:
        struct FReadSlot
        {
            FGuid Generation = FGuid::NewGuid();
            bool bActive = false;
            void Retire() { Generation = FGuid::NewGuid(); bActive = false; }
        };
        bool Enabled() const;
        bool Current(const FGameEconomyReadReceipt& Receipt) const;
        bool Capture(EGameEconomyReadKind Kind, FGameEconomyReadReceipt& Out);
        FReadSlot& Slot(EGameEconomyReadKind Kind);
        const FReadSlot& Slot(EGameEconomyReadKind Kind) const;
        void RetireReads();
        void Fail(EGameEconomyReadKind Kind, const FString& PublicMessage);
        bool ReceiveQuote(const FGameEconomyQuote& Quote);
        bool PrepareCommand(EGameEconomyCommand Operation, const FString& RequestId, const FString& Value,
            EGameEconomySelectorKind SelectorKind, FGameEconomyCommandReceipt& Out);
        bool CurrentCommand(const FGameEconomyCommandReceipt& Receipt) const;
        bool CommandRelevant(const FGameEconomyCommandReceipt& Receipt) const;
        bool BeginCompletion(const FGameEconomyCommandReceipt& Receipt, FGameEconomyCommandCompletion& Out);
        void ReleaseCommand();
        struct FPendingVendor
        {
            FGuid Id;
            EGameEconomyCommand Operation = EGameEconomyCommand::RequestWeaponQuote;
            FString RequestId, ArgumentsJson;
        };
        FString AccountId, PlaceKey, NpcKey;
        TOptional<FString> QuoteId;
        TOptional<FGameEconomyVendorScope> VendorScope;
        TOptional<int64> VendorNowMs;
        FGameEconomyReadView View;
        FGuid AccountGeneration = FGuid::NewGuid();
        FGuid ViewGeneration = FGuid::NewGuid(), VendorViewGeneration = FGuid::NewGuid(), ActiveRequest;
        TOptional<FGameEconomyVendorScope> LastVendorScope;
        TOptional<FPendingVendor> PendingVendor;
        // Independent of PendingVendor so a changed visit cannot retarget a prepared dispatch.
        FGameEconomyCommandDispatch ActiveDispatch;
        bool bCommandDispatched = false, bCommandCompleting = false;
        FReadSlot ReadSlots[3];
        bool bObserving = false, bDisposed = false;
    };
}
