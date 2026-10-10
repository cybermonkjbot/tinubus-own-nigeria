#include "BaddieCompanionFoodState.h"
#include "BaddieCompanionStateJson.h"

namespace BaddieCompanion
{
    namespace FoodPrivate
    {
        using namespace StateJsonPrivate;
        // Allocation limits, not gameplay capacity or inventory admission rules.
        constexpr FParseLimits Limits { 2097152, 32, 131072 };
        template <typename T> bool Enum(const FValue& V, T& Out,
            std::initializer_list<TPair<const TCHAR*, T>> Choices)
        {
            FString Wire;
            if (!String(V, Wire)) return false;
            for (const auto& Choice : Choices)
                if (Same(Wire, Choice.Key)) { Out = Choice.Value; return true; }
            return false;
        }
        bool Literal(const FValue& V, const TCHAR* Expected)
        {
            FString Wire;
            return String(V, Wire) && Same(Wire, Expected);
        }
        bool Time(const FValue& V, double& Out) { return Number(V, Out) && ValidTime(Out); }
        bool Text(const FObject& O, const TCHAR* K, FString& Out) { return String(Field(O, K), Out); }
        bool Key(const FValue& V, EFoodKey& Out)
        {
            FString Wire;
            return String(V, Wire) && ParseFoodKey(Wire, Out);
        }
        template <typename T, typename F> bool Array(const FValue& V, TArray<T>& Out, F Read)
        {
            if (!V.IsValid() || V->Type != EJson::Array || V->AsArray().Num() > 4096) return false;
            for (const auto& Item : V->AsArray())
            {
                T Next;
                if (!Read(Item, Next)) return false;
                Out.Add(MoveTemp(Next));
            }
            return true;
        }
        bool Scope(const FObject& O, FFoodScope& Out)
        {
            if (!Only(O, { TEXT("gameRoomId"), TEXT("instanceId"), TEXT("lifeId"), TEXT("revision"),
                    TEXT("serviceId"), TEXT("movingOfferId") })
                || !Text(O, TEXT("gameRoomId"), Out.GameRoomId) || !Text(O, TEXT("instanceId"), Out.InstanceId)
                || !Text(O, TEXT("lifeId"), Out.LifeId) || !Text(O, TEXT("serviceId"), Out.ServiceId)
                || !Integer(Field(O, TEXT("revision")), Out.Revision)) return false;
            const auto Moving = Field(O, TEXT("movingOfferId"));
            if (Moving.IsValid())
            {
                FString Id;
                if (!String(Moving, Id)) return false;
                Out.MovingOfferId = MoveTemp(Id);
            }
            return true;
        }
        bool Price(const FValue& V, FFoodEconomyPrice& Out)
        {
            const auto O = Object(V);
            if (!Enum(Field(O, TEXT("unit")), Out.Unit, {
                    { TEXT("real-currency-minor"), EFoodPriceUnit::RealCurrencyMinor },
                    { TEXT("game-value-unit"), EFoodPriceUnit::GameValueUnit } })
                || !Number(O, TEXT("amount"), Out.Amount)) return false;
            const auto Source = Object(Field(O, TEXT("source")));
            if (!Only(Source, { TEXT("kind"), TEXT("key") })) return false;
            if (Out.Unit == EFoodPriceUnit::RealCurrencyMinor)
            {
                FFoodMainBalancePrice Branch;
                if (!Only(O, { TEXT("unit"), TEXT("currency"), TEXT("amount"), TEXT("source") })
                    || !Literal(Field(O, TEXT("currency")), TEXT("NGN"))
                    || !Literal(Field(Source, TEXT("kind")), TEXT("main_balance"))
                    || !Enum(Field(Source, TEXT("key")), Branch.SourceKey, {
                        { TEXT("handyman"), EFoodMainBalanceKey::Handyman },
                        { TEXT("partner"), EFoodMainBalanceKey::Partner },
                        { TEXT("declutter_customer"), EFoodMainBalanceKey::DeclutterCustomer },
                        { TEXT("rewards_member"), EFoodMainBalanceKey::RewardsMember },
                        { TEXT("referral_partner"), EFoodMainBalanceKey::ReferralPartner },
                        { TEXT("content_creator"), EFoodMainBalanceKey::ContentCreator },
                        { TEXT("influence_creator"), EFoodMainBalanceKey::InfluenceCreator } })) return false;
                Out.MainBalance = MoveTemp(Branch);
            }
            else
            {
                FFoodBankedValuePrice Branch;
                if (!Only(O, { TEXT("unit"), TEXT("unitKey"), TEXT("amount"), TEXT("source") })
                    || !Text(O, TEXT("unitKey"), Branch.UnitKey)
                    || !Literal(Field(Source, TEXT("kind")), TEXT("banked_game_value"))
                    || !Text(Source, TEXT("key"), Branch.SourceKey)) return false;
                Out.BankedValue = MoveTemp(Branch);
            }
            return true;
        }
        bool Exchange(const FValue& V, FFoodExchangeRate& Out)
        {
            const auto O = Object(V);
            return Only(O, { TEXT("creditsPerNaira"), TEXT("creditsPerKobo") })
                && Number(O, TEXT("creditsPerNaira"), Out.CreditsPerNaira)
                && Number(O, TEXT("creditsPerKobo"), Out.CreditsPerKobo);
        }
        bool Approval(const FValue& V, EFoodApprovalStatus& Out)
        {
            return Enum(V, Out, { { TEXT("pending_approval"), EFoodApprovalStatus::PendingApproval },
                { TEXT("approved"), EFoodApprovalStatus::Approved } });
        }
        bool Policy(const FValue& V, FFoodEconomyPolicy& Out)
        {
            const auto O = Object(V);
            return Only(O, { TEXT("liveFinancialActivation"), TEXT("conversionStatus"), TEXT("fundingStatus"),
                    TEXT("paymentSourceStatus"), TEXT("withdrawalStatus"), TEXT("exchangeRate") })
                && Enum(Field(O, TEXT("liveFinancialActivation")), Out.LiveFinancialActivation, {
                    { TEXT("disabled"), EFoodFinancialActivation::Disabled }, { TEXT("enabled"), EFoodFinancialActivation::Enabled } })
                && Enum(Field(O, TEXT("conversionStatus")), Out.ConversionStatus, {
                    { TEXT("pending_approval"), EFoodConversionStatus::PendingApproval },
                    { TEXT("not_applicable"), EFoodConversionStatus::NotApplicable }, { TEXT("active"), EFoodConversionStatus::Active } })
                && Approval(Field(O, TEXT("fundingStatus")), Out.FundingStatus)
                && Approval(Field(O, TEXT("paymentSourceStatus")), Out.PaymentSourceStatus)
                && Enum(Field(O, TEXT("withdrawalStatus")), Out.WithdrawalStatus, {
                    { TEXT("unavailable"), EFoodWithdrawalStatus::Unavailable }, { TEXT("available"), EFoodWithdrawalStatus::Available } })
                && Nullable(Field(O, TEXT("exchangeRate")), Out.ExchangeRate, Exchange);
        }
        bool Service(const FValue& V, FFoodEconomyService& Out)
        {
            const auto O = Object(V);
            return Only(O, { TEXT("offerId"), TEXT("serviceKey"), TEXT("revision"), TEXT("npcKey"), TEXT("placeKey"),
                    TEXT("label"), TEXT("price"), TEXT("beneficiary"), TEXT("offerStartsAt"), TEXT("offerExpiresAt"),
                    TEXT("quoteTtlMs"), TEXT("availabilityRevision"), TEXT("available"), TEXT("policy") })
                && Text(O, TEXT("offerId"), Out.OfferId) && Text(O, TEXT("serviceKey"), Out.ServiceKey)
                && Integer(Field(O, TEXT("revision")), Out.Revision) && Text(O, TEXT("npcKey"), Out.NpcKey)
                && Text(O, TEXT("placeKey"), Out.PlaceKey) && Text(O, TEXT("label"), Out.Label)
                && Price(Field(O, TEXT("price")), Out.Price) && Literal(Field(O, TEXT("beneficiary")), TEXT("self"))
                && Time(Field(O, TEXT("offerStartsAt")), Out.OfferStartsAtMs)
                && Time(Field(O, TEXT("offerExpiresAt")), Out.OfferExpiresAtMs)
                && Time(Field(O, TEXT("quoteTtlMs")), Out.QuoteTtlMs)
                && Integer(Field(O, TEXT("availabilityRevision")), Out.AvailabilityRevision)
                && Boolean(Field(O, TEXT("available")), Out.bAvailable) && Policy(Field(O, TEXT("policy")), Out.Policy);
        }
        bool Quote(const FValue& V, FFoodEconomyQuote& Out)
        {
            const auto O = Object(V);
            return Only(O, { TEXT("id"), TEXT("service"), TEXT("issuedAt"), TEXT("expiresAt"), TEXT("termsFingerprint"),
                    TEXT("status"), TEXT("revision"), TEXT("confirmedAt"), TEXT("cancelledAt") })
                && Text(O, TEXT("id"), Out.Id) && Service(Field(O, TEXT("service")), Out.Service)
                && Time(Field(O, TEXT("issuedAt")), Out.IssuedAtMs) && Time(Field(O, TEXT("expiresAt")), Out.ExpiresAtMs)
                && Text(O, TEXT("termsFingerprint"), Out.TermsFingerprint)
                && Enum(Field(O, TEXT("status")), Out.Status, {
                    { TEXT("quoted"), EFoodQuoteStatus::Quoted }, { TEXT("approval_required"), EFoodQuoteStatus::ApprovalRequired },
                    { TEXT("cancelled"), EFoodQuoteStatus::Cancelled }, { TEXT("fulfilled"), EFoodQuoteStatus::Fulfilled } })
                && Integer(Field(O, TEXT("revision")), Out.Revision)
                && Nullable(Field(O, TEXT("confirmedAt")), Out.ConfirmedAtMs, Time)
                && Nullable(Field(O, TEXT("cancelledAt")), Out.CancelledAtMs, Time);
        }
        bool Order(const FValue& V, FFoodOrder& Out)
        {
            const auto O = Object(V);
            return Only(O, { TEXT("id"), TEXT("quoteId"), TEXT("offerKey"), TEXT("label"), TEXT("placeId"), TEXT("serviceId"),
                    TEXT("quantity"), TEXT("mode"), TEXT("status"), TEXT("revision"), TEXT("purchasedAt"), TEXT("readyAt"),
                    TEXT("collectedAt"), TEXT("consumedAt") })
                && Text(O, TEXT("id"), Out.Id) && Nullable(Field(O, TEXT("quoteId")), Out.QuoteId, String)
                && Key(Field(O, TEXT("offerKey")), Out.OfferKey) && Text(O, TEXT("label"), Out.Label)
                && Text(O, TEXT("placeId"), Out.PlaceId) && Text(O, TEXT("serviceId"), Out.ServiceId)
                && Number(O, TEXT("quantity"), Out.Quantity)
                && Enum(Field(O, TEXT("mode")), Out.Mode, {
                    { TEXT("pickup"), EFoodOrderMode::Pickup }, { TEXT("delivery"), EFoodOrderMode::Delivery } })
                && Enum(Field(O, TEXT("status")), Out.Status, {
                    { TEXT("ready"), EFoodOrderStatus::Ready }, { TEXT("carried"), EFoodOrderStatus::Carried },
                    { TEXT("secured"), EFoodOrderStatus::Secured }, { TEXT("stocked"), EFoodOrderStatus::Stocked },
                    { TEXT("consumed"), EFoodOrderStatus::Consumed }, { TEXT("lost"), EFoodOrderStatus::Lost } })
                && Integer(Field(O, TEXT("revision")), Out.Revision)
                && Time(Field(O, TEXT("purchasedAt")), Out.PurchasedAtMs) && Time(Field(O, TEXT("readyAt")), Out.ReadyAtMs)
                && Nullable(Field(O, TEXT("collectedAt")), Out.CollectedAtMs, Time)
                && Nullable(Field(O, TEXT("consumedAt")), Out.ConsumedAtMs, Time);
        }
        bool Offer(const FValue& V, FFoodOffer& Out)
        {
            const auto O = Object(V);
            return Only(O, { TEXT("offerKey"), TEXT("label"), TEXT("price"), TEXT("stock") })
                && Key(Field(O, TEXT("offerKey")), Out.OfferKey) && Text(O, TEXT("label"), Out.Label)
                && Price(Field(O, TEXT("price")), Out.Price) && Number(O, TEXT("stock"), Out.Stock);
        }
        bool Moving(const FValue& V, FFoodMovingOffer& Out)
        {
            const auto O = Object(V);
            double Quantity = 0, Side = 0;
            if (!Only(O, { TEXT("id"), TEXT("drivingId"), TEXT("vehicleId"), TEXT("driverId"), TEXT("gameRoomId"),
                    TEXT("instanceId"), TEXT("layoutRevision"), TEXT("lifeId"), TEXT("visitId"), TEXT("vendorId"), TEXT("npcId"),
                    TEXT("vendorRevision"), TEXT("serviceId"), TEXT("bottleneckId"), TEXT("offerKey"), TEXT("quantity"),
                    TEXT("stockKey"), TEXT("stockRevision"), TEXT("amount"), TEXT("unitKey"), TEXT("issuedAt"), TEXT("expiresAt"),
                    TEXT("status"), TEXT("revision"), TEXT("windowSide"), TEXT("windowHalfWidth"), TEXT("windowOffset") })
                || !Text(O, TEXT("id"), Out.Id) || !Text(O, TEXT("drivingId"), Out.DrivingId)
                || !Text(O, TEXT("vehicleId"), Out.VehicleId) || !Text(O, TEXT("driverId"), Out.DriverId)
                || !Text(O, TEXT("gameRoomId"), Out.GameRoomId) || !Text(O, TEXT("instanceId"), Out.InstanceId)
                || !Text(O, TEXT("layoutRevision"), Out.LayoutRevision) || !Text(O, TEXT("lifeId"), Out.LifeId)
                || !Text(O, TEXT("visitId"), Out.VisitId) || !Text(O, TEXT("vendorId"), Out.VendorId)
                || !Text(O, TEXT("npcId"), Out.NpcId) || !Integer(Field(O, TEXT("vendorRevision")), Out.VendorRevision)
                || !Text(O, TEXT("serviceId"), Out.ServiceId) || !Text(O, TEXT("bottleneckId"), Out.BottleneckId)
                || !Key(Field(O, TEXT("offerKey")), Out.OfferKey)
                || (Out.OfferKey != EFoodKey::Suya && Out.OfferKey != EFoodKey::Akara)
                || !Number(O, TEXT("quantity"), Quantity) || (Quantity != 1 && Quantity != 3 && Quantity != 6)
                || !Text(O, TEXT("stockKey"), Out.StockKey) || !Integer(Field(O, TEXT("stockRevision")), Out.StockRevision)
                || !Number(O, TEXT("amount"), Out.Amount) || !Literal(Field(O, TEXT("unitKey")), TEXT("lagos-credit"))
                || !Time(Field(O, TEXT("issuedAt")), Out.IssuedAtMs) || !Time(Field(O, TEXT("expiresAt")), Out.ExpiresAtMs)
                || !Enum(Field(O, TEXT("status")), Out.Status, {
                    { TEXT("offered"), EFoodMovingStatus::Offered }, { TEXT("fulfilled"), EFoodMovingStatus::Fulfilled },
                    { TEXT("cancelled"), EFoodMovingStatus::Cancelled } })
                || !Integer(Field(O, TEXT("revision")), Out.Revision)
                || !Number(O, TEXT("windowSide"), Side) || (Side != -1 && Side != 1)
                || !Number(O, TEXT("windowHalfWidth"), Out.WindowHalfWidth)
                || !Number(O, TEXT("windowOffset"), Out.WindowOffset)) return false;
            Out.Quantity = static_cast<int32>(Quantity);
            Out.WindowSide = static_cast<int32>(Side);
            return true;
        }
        bool OptionalMoving(const FValue& V, TWorldPlayerField<FFoodMovingOffer>& Out)
        {
            if (!V.IsValid()) return true;
            if (V->Type == EJson::Null) { Out.Presence = EWorldFieldPresence::Null; return true; }
            FFoodMovingOffer Next;
            if (!Moving(V, Next)) return false;
            Out.Presence = EWorldFieldPresence::Value;
            Out.Value = MoveTemp(Next);
            return true;
        }
        bool Catalog(const FObject& O, FFoodCatalog& Out)
        {
            return Only(O, { TEXT("scope"), TEXT("eating"), TEXT("capacity"), TEXT("occupied"), TEXT("serverNow"),
                    TEXT("offers"), TEXT("orders"), TEXT("carried"), TEXT("movingOffer") })
                && Scope(Object(Field(O, TEXT("scope"))), Out.Scope) && Boolean(Field(O, TEXT("eating")), Out.bEating)
                && Number(O, TEXT("capacity"), Out.Capacity) && Number(O, TEXT("occupied"), Out.Occupied)
                && Time(Field(O, TEXT("serverNow")), Out.ServerNowMs) && Array(Field(O, TEXT("offers")), Out.Offers, Offer)
                && Array(Field(O, TEXT("orders")), Out.Orders, Order) && Array(Field(O, TEXT("carried")), Out.Carried, Order)
                && OptionalMoving(Field(O, TEXT("movingOffer")), Out.MovingOffer);
        }
        bool Result(const FObject& O, FFoodResult& Out)
        {
            return Only(O, { TEXT("quote"), TEXT("order"), TEXT("movingOffer") })
                && Nullable(Field(O, TEXT("quote")), Out.Quote, Quote)
                && Nullable(Field(O, TEXT("order")), Out.Order, Order)
                && OptionalMoving(Field(O, TEXT("movingOffer")), Out.MovingOffer);
        }
        template <typename T, typename F> bool Decode(const FString& Json, T& Out, FString& Error, F Read)
        {
            Out = T{};
            Error = TEXT("Invalid food read response.");
            FObject O;
            T Next;
            if (!Parse(Json, O, Limits) || !Read(O, Next)) return false;
            Out = MoveTemp(Next);
            Error.Reset();
            return true;
        }
        bool SameOptionalText(const TOptional<FString>& A, const TOptional<FString>& B)
        {
            return A.IsSet() == B.IsSet() && (!A.IsSet() || Same(A.GetValue(), B.GetValue()));
        }
        bool SameScope(const FFoodScope& A, const FFoodScope& B)
        {
            return Same(A.GameRoomId, B.GameRoomId) && Same(A.InstanceId, B.InstanceId) && Same(A.LifeId, B.LifeId)
                && A.Revision == B.Revision && Same(A.ServiceId, B.ServiceId) && SameOptionalText(A.MovingOfferId, B.MovingOfferId);
        }
        bool SameSelection(const TOptional<FFoodScope>& A, const TOptional<FFoodScope>& B)
        {
            return A.IsSet() == B.IsSet() && (!A.IsSet() || SameScope(A.GetValue(), B.GetValue()));
        }
    }

    bool ParseFoodKey(const FString& Wire, EFoodKey& Out)
    {
        const TPair<const TCHAR*, EFoodKey> Keys[] = {
            { TEXT("suya"), EFoodKey::Suya }, { TEXT("akara"), EFoodKey::Akara }, { TEXT("water"), EFoodKey::Water },
            { TEXT("ready-meal"), EFoodKey::ReadyMeal }, { TEXT("energy-drink"), EFoodKey::EnergyDrink },
            { TEXT("rice"), EFoodKey::Rice }, { TEXT("beans"), EFoodKey::Beans }, { TEXT("tomato"), EFoodKey::Tomato },
            { TEXT("oil"), EFoodKey::Oil } };
        Out = EFoodKey::Suya;
        for (const auto& Key : Keys)
            if (StateJsonPrivate::Same(Wire, Key.Key)) { Out = Key.Value; return true; }
        return false;
    }
    bool DecodeFoodScope(const FString& Json, FFoodScope& Out, FString& OutError)
    {
        return FoodPrivate::Decode(Json, Out, OutError, FoodPrivate::Scope);
    }
    bool DecodeFoodCatalog(const FString& Json, FFoodCatalog& Out, FString& OutError)
    {
        return FoodPrivate::Decode(Json, Out, OutError, FoodPrivate::Catalog);
    }
    bool DecodeFoodResult(const FString& Json, FFoodResult& Out, FString& OutError)
    {
        return FoodPrivate::Decode(Json, Out, OutError, FoodPrivate::Result);
    }
    bool DecodeFoodQuote(const FString& Json, FFoodEconomyQuote& Out, FString& OutError)
    {
        return FoodPrivate::Decode(Json, Out, OutError, [](const StateJsonPrivate::FObject& O, FFoodEconomyQuote& Next)
        {
            return FoodPrivate::Quote(MakeShared<FJsonValueObject>(O), Next);
        });
    }
    EFoodReadEndpoint FFoodReadReceipt::GetEndpoint() const
    {
        return Scope.IsSet() && Scope->MovingOfferId.IsSet() && !Scope->MovingOfferId->IsEmpty()
            ? EFoodReadEndpoint::MovingCatalog : EFoodReadEndpoint::Catalog;
    }
    bool FFoodReadState::Select(const TOptional<FFoodScope>& Scope)
    {
        if (bDisposed || (Scope.IsSet() && (Scope->Revision < 0 || Scope->Revision > StateJsonPrivate::MaxInteger))) return false;
        if (FoodPrivate::SameSelection(Selection, Scope)) return true;
        EndSubscription();
        Selection = Scope;
        View = FFoodReadView{};
        Changed.Broadcast();
        return true;
    }
    FFoodReadReceipt FFoodReadState::BeginSubscription()
    {
        EndSubscription();
        FFoodReadReceipt Receipt;
        if (bDisposed || !Selection.IsSet()) return Receipt;
        bSubscribed = true;
        Receipt.Generation = Generation;
        Receipt.Scope = Selection;
        return Receipt;
    }
    void FFoodReadState::EndSubscription()
    {
        Generation = FGuid::NewGuid();
        bSubscribed = false;
    }
    bool FFoodReadState::Current(const FFoodReadReceipt& Receipt) const
    {
        return !bDisposed && bSubscribed && Receipt.Generation.IsValid() && Receipt.Generation == Generation
            && Selection.IsSet() && Receipt.Scope.IsSet() && FoodPrivate::SameSelection(Selection, Receipt.Scope);
    }
    EFoodReadResult FFoodReadState::Accept(const FFoodReadReceipt& Receipt, const FString& Json)
    {
        if (!Current(Receipt)) return EFoodReadResult::StaleReceipt;
        FFoodCatalog Next;
        FString Error;
        if (!DecodeFoodCatalog(Json, Next, Error)) return EFoodReadResult::InvalidResponse;
        if (!FoodPrivate::SameScope(Next.Scope, Receipt.Scope.GetValue())) return EFoodReadResult::ScopeMismatch;
        if (Next.MovingOffer.Value.IsSet())
        {
            const auto& Incoming = Next.MovingOffer.Value.GetValue();
            const auto& Previous = View.MovingOffer.Value;
            if (!Previous.IsSet() || !StateJsonPrivate::Same(Previous->Id, Incoming.Id)
                || Incoming.Revision > Previous->Revision
                || (Incoming.Revision == Previous->Revision
                    && (Previous->Status == EFoodMovingStatus::Offered || Incoming.Status == Previous->Status)))
                View.MovingOffer = Next.MovingOffer;
        }
        View.Catalog = MakeShared<FFoodCatalog>(MoveTemp(Next));
        View.bReady = true;
        View.Error.Reset();
        Changed.Broadcast();
        return EFoodReadResult::Accepted;
    }
    bool FFoodReadState::AcceptFailure(const FFoodReadReceipt& Receipt)
    {
        if (!Current(Receipt)) return false;
        View.Catalog.Reset();
        View.bReady = true;
        View.Error = TEXT("Food details couldn't load. Reload to retry.");
        Changed.Broadcast();
        return true;
    }
    void FFoodReadState::Clear()
    {
        if (bDisposed) return;
        EndSubscription();
        Selection.Reset();
        View = FFoodReadView{};
        Changed.Broadcast();
    }
    void FFoodReadState::Dispose()
    {
        if (bDisposed) return;
        bDisposed = true;
        EndSubscription();
        Selection.Reset();
        View = FFoodReadView{};
        Changed.Clear();
    }
}
