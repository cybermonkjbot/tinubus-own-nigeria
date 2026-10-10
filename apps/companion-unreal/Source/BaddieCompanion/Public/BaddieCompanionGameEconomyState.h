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
        bool bReady = false, bQuoteReady = true;
        TOptional<FString> Error;
        // No command busy/latch or retry state. Vendor busy is received read data only.
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

    // Account-private single-game-thread read owner, using UE value types and GUIDs.
    // No subscriptions, networking, commands, timers, money/equipment writes or admission.
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
        FString AccountId, PlaceKey, NpcKey;
        TOptional<FString> QuoteId;
        TOptional<FGameEconomyVendorScope> VendorScope;
        TOptional<int64> VendorNowMs;
        FGameEconomyReadView View;
        FGuid AccountGeneration = FGuid::NewGuid();
        FReadSlot ReadSlots[3];
        bool bObserving = false, bDisposed = false;
    };
}
