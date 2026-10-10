#pragma once

#include "BaddieCompanionWorldPlayer.h"
#include "Misc/Guid.h"

namespace BaddieCompanion
{
    enum class EFoodKey : uint8 { Suya, Akara, Water, ReadyMeal, EnergyDrink, Rice, Beans, Tomato, Oil };
    BADDIECOMPANION_API bool ParseFoodKey(const FString& Wire, EFoodKey& Out);
    BADDIECOMPANION_API const TCHAR* FoodKeyWire(EFoodKey Key);
    enum class EFoodOrderMode : uint8 { Pickup, Delivery };
    enum class EFoodOrderStatus : uint8 { Ready, Carried, Secured, Stocked, Consumed, Lost };
    enum class EFoodMovingStatus : uint8 { Offered, Fulfilled, Cancelled };
    enum class EFoodCreditUnit : uint8 { LagosCredit };
    enum class EFoodPriceUnit : uint8 { RealCurrencyMinor, GameValueUnit };
    enum class EFoodMainBalanceKey : uint8
    {
        Handyman, Partner, DeclutterCustomer, RewardsMember, ReferralPartner, ContentCreator, InfluenceCreator
    };
    struct FFoodMainBalancePrice
    {
        // Literal currency NGN and source kind main_balance.
        EFoodMainBalanceKey SourceKey = EFoodMainBalanceKey::Handyman;
    };
    struct FFoodBankedValuePrice
    {
        // Literal source kind banked_game_value. The unit and source keys are independent.
        FString UnitKey, SourceKey;
    };
    struct FFoodEconomyPrice
    {
        EFoodPriceUnit Unit = EFoodPriceUnit::GameValueUnit;
        double Amount = 0;
        // Decoded values populate exactly the branch selected by Unit.
        TOptional<FFoodMainBalancePrice> MainBalance;
        TOptional<FFoodBankedValuePrice> BankedValue;
    };
    enum class EFoodFinancialActivation : uint8 { Disabled, Enabled };
    enum class EFoodConversionStatus : uint8 { PendingApproval, NotApplicable, Active };
    enum class EFoodApprovalStatus : uint8 { PendingApproval, Approved };
    enum class EFoodWithdrawalStatus : uint8 { Unavailable, Available };
    enum class EFoodBeneficiary : uint8 { Self };
    enum class EFoodQuoteStatus : uint8 { Quoted, ApprovalRequired, Cancelled, Fulfilled };
    struct FFoodExchangeRate { double CreditsPerNaira = 0, CreditsPerKobo = 0; };
    struct FFoodEconomyPolicy
    {
        EFoodFinancialActivation LiveFinancialActivation = EFoodFinancialActivation::Disabled;
        EFoodConversionStatus ConversionStatus = EFoodConversionStatus::PendingApproval;
        EFoodApprovalStatus FundingStatus = EFoodApprovalStatus::PendingApproval;
        EFoodApprovalStatus PaymentSourceStatus = EFoodApprovalStatus::PendingApproval;
        EFoodWithdrawalStatus WithdrawalStatus = EFoodWithdrawalStatus::Unavailable;
        TOptional<FFoodExchangeRate> ExchangeRate;
    };
    struct FFoodEconomyService
    {
        FString OfferId, ServiceKey, NpcKey, PlaceKey, Label;
        int64 Revision = 0, AvailabilityRevision = 0;
        FFoodEconomyPrice Price;
        EFoodBeneficiary Beneficiary = EFoodBeneficiary::Self;
        double OfferStartsAtMs = 0, OfferExpiresAtMs = 0, QuoteTtlMs = 0;
        bool bAvailable = false;
        FFoodEconomyPolicy Policy;
    };
    struct FFoodEconomyQuote
    {
        FString Id, TermsFingerprint;
        FFoodEconomyService Service;
        double IssuedAtMs = 0, ExpiresAtMs = 0;
        EFoodQuoteStatus Status = EFoodQuoteStatus::Quoted;
        int64 Revision = 0;
        TOptional<double> ConfirmedAtMs, CancelledAtMs;
    };
    struct FFoodScope
    {
        FString GameRoomId, InstanceId, LifeId, ServiceId;
        int64 Revision = 0;
        // Missing differs from present-empty. Null is not part of this wire contract.
        TOptional<FString> MovingOfferId;
    };
    struct FFoodOrder
    {
        FString Id, Label, PlaceId, ServiceId;
        TOptional<FString> QuoteId;
        EFoodKey OfferKey = EFoodKey::Suya;
        double Quantity = 0;
        EFoodOrderMode Mode = EFoodOrderMode::Pickup;
        EFoodOrderStatus Status = EFoodOrderStatus::Ready;
        int64 Revision = 0;
        double PurchasedAtMs = 0, ReadyAtMs = 0;
        TOptional<double> CollectedAtMs, ConsumedAtMs;
    };
    struct FFoodOffer
    {
        EFoodKey OfferKey = EFoodKey::Suya;
        FString Label;
        FFoodEconomyPrice Price;
        double Stock = 0;
    };
    struct FFoodMovingOffer
    {
        FString Id, DrivingId, VehicleId, DriverId, GameRoomId, InstanceId, LayoutRevision;
        FString LifeId, VisitId, VendorId, NpcId, ServiceId, BottleneckId, StockKey;
        int64 VendorRevision = 0, StockRevision = 0, Revision = 0;
        // The moving endpoint narrows this to suya or akara.
        EFoodKey OfferKey = EFoodKey::Suya;
        int32 Quantity = 1;
        double Amount = 0;
        EFoodCreditUnit UnitKey = EFoodCreditUnit::LagosCredit;
        double IssuedAtMs = 0, ExpiresAtMs = 0;
        EFoodMovingStatus Status = EFoodMovingStatus::Offered;
        // Source window measurements. No native unit conversion or local reach/expiry decision.
        int32 WindowSide = 1;
        double WindowHalfWidth = 0, WindowOffset = 0;
    };
    struct FFoodCatalog
    {
        FFoodScope Scope;
        bool bEating = false;
        double Capacity = 0, Occupied = 0, ServerNowMs = 0;
        TArray<FFoodOffer> Offers;
        TArray<FFoodOrder> Orders, Carried;
        TWorldPlayerField<FFoodMovingOffer> MovingOffer;
    };
    struct FFoodResult
    {
        TOptional<FFoodEconomyQuote> Quote;
        TOptional<FFoodOrder> Order;
        TWorldPlayerField<FFoodMovingOffer> MovingOffer;
    };

    // Endpoint values, not envelopes. Failure clears Out and supplies fixed diagnostic text.
    // Standalone decoding does not publish state.
    BADDIECOMPANION_API bool DecodeFoodScope(const FString& Json, FFoodScope& Out, FString& OutError);
    BADDIECOMPANION_API bool DecodeFoodCatalog(const FString& Json, FFoodCatalog& Out, FString& OutError);
    BADDIECOMPANION_API bool DecodeFoodResult(const FString& Json, FFoodResult& Out, FString& OutError);
    BADDIECOMPANION_API bool DecodeFoodQuote(const FString& Json, FFoodEconomyQuote& Out, FString& OutError);

    enum class EFoodActionType : uint8 { Quote, Confirm, Cancel, Collect, Eat };
    enum class EFoodQuantity : uint8 { One = 1, Three = 3, Six = 6 };
    struct FFoodAction
    {
        EFoodActionType Type = EFoodActionType::Quote;
        EFoodKey OfferKey = EFoodKey::Suya;
        EFoodQuantity Quantity = EFoodQuantity::One;
        FString QuoteId, OrderId;
        // Only the selected branch's fields are encoded; no caller-supplied scope.
    };
    // Shared by these two account-bound owners. No transport or authorization evidence.
    BADDIECOMPANION_API bool ValidFoodKitchenRequestId(const FString& RequestId);
    enum class EFoodKitchenFailureCategory : uint8 { Definite, Ambiguous };
    struct BADDIECOMPANION_API FFoodKitchenCommandFailure
    {
        EFoodKitchenFailureCategory Category = EFoodKitchenFailureCategory::Ambiguous;
        // Already sanitized by the authenticated boundary. Never pass raw exception text.
        FString SafePublicMessage;
        FString PublicMessage(const TCHAR* Fallback) const;
    };
    class FFoodKitchenCommandReceipt
    {
    public:
        bool IsValid() const { return Attempt.IsValid(); }
        const FString& GetEndpoint() const { return Endpoint; }
        const FString& GetArgumentsJson() const { return ArgumentsJson; }
        const FString& GetRequestId() const { return RequestId; }
    private:
        friend class FFoodReadState;
        friend class FKitchenReadState;
        FGuid Attempt;
        FString Endpoint, ArgumentsJson, RequestId;
    };
    enum class EFoodKitchenCompletion : uint8 { Applied, Detached, StaleReceipt, InvalidResponse };

    enum class EFoodReadResult : uint8 { Accepted, StaleReceipt, InvalidResponse, ScopeMismatch };
    enum class EFoodReadEndpoint : uint8 { Catalog, MovingCatalog };
    class BADDIECOMPANION_API FFoodReadReceipt
    {
    public:
        const TOptional<FFoodScope>& GetScope() const { return Scope; }
        EFoodReadEndpoint GetEndpoint() const;
    private:
        friend class FFoodReadState;
        FGuid Generation;
        TOptional<FFoodScope> Scope;
    };
    struct FFoodReadView
    {
        TSharedPtr<const FFoodCatalog> Catalog;
        // Canonical initial value is missing. Incoming null/missing never clears retention.
        TWorldPlayerField<FFoodMovingOffer> MovingOffer;
        TOptional<FFoodEconomyQuote> Quote;
        TSharedPtr<const FFoodResult> Result;
        bool bReady = false, bBusy = false, bRetryable = false;
        // Read success preserves this while a dispatched request remains unresolved.
        TOptional<FString> Error;
    };
    DECLARE_MULTICAST_DELEGATE(FFoodReadChanged);
    // Single-threaded, one account per owner lifetime. No transport or purchase authority.
    class BADDIECOMPANION_API FFoodReadState
    {
    public:
        FFoodReadState() = default;
        FFoodReadState(const FFoodReadState&) = delete;
        FFoodReadState& operator=(const FFoodReadState&) = delete;
        const FFoodReadView& GetView() const { return View; }
        const TOptional<FFoodScope>& GetSelection() const { return Selection; }
        FFoodReadChanged& OnChanged() { return Changed; }
        bool Select(const TOptional<FFoodScope>& Scope);
        // Transport calls this on first observation or reload, and End on last unsubscribe.
        // Begin retires old callbacks but retains displayed state. It does not subscribe.
        FFoodReadReceipt BeginSubscription();
        void EndSubscription();
        EFoodReadResult Accept(const FFoodReadReceipt& Receipt, const FString& Json);
        // Fixed safe fallback; transport error sanitization is outside this owner.
        bool AcceptFailure(const FFoodReadReceipt& Receipt);
        bool PrepareCommand(const FFoodAction& Action, const FString& RequestId, FFoodKitchenCommandReceipt& Out);
        bool PrepareRetry(FFoodKitchenCommandReceipt& Out);
        // After adapter admission, call immediately at actual send with no intervening callbacks.
        // False means do not send. Successful dispatch does not notify.
        bool DispatchCommand(const FFoodKitchenCommandReceipt& Receipt);
        // Only for a prepared attempt that has never been sent. Retains a prior uncertain request.
        bool AbandonPreparedCommand(const FFoodKitchenCommandReceipt& Receipt);
        EFoodKitchenCompletion CompleteCommand(const FFoodKitchenCommandReceipt& Receipt,
            const FString& Json, TSharedPtr<const FFoodResult>& OutResult);
        bool FailCommand(const FFoodKitchenCommandReceipt& Receipt, const FFoodKitchenCommandFailure& Failure);
        // Account teardown invalidates read and command receipts, even with identical next scopes.
        void Clear();
        void Dispose();
    private:
        bool Current(const FFoodReadReceipt& Receipt) const;
        struct FRequest
        {
            FFoodScope Scope;
            FFoodAction Action;
            FString RequestId, Endpoint, ArgumentsJson;
        };
        bool PrepareAttempt(FFoodKitchenCommandReceipt& Out);
        bool CurrentAttempt(const FFoodKitchenCommandReceipt& Receipt) const;
        void FinishAttempt();
        TSharedPtr<const FRequest> Pending;
        FGuid Attempt, AttemptReadGeneration;
        bool bUncertain = false, bDispatched = false, bReplay = false;
        FFoodReadView View;
        TOptional<FFoodScope> Selection;
        FGuid Generation;
        bool bSubscribed = false, bDisposed = false;
        FFoodReadChanged Changed;
    };
}
