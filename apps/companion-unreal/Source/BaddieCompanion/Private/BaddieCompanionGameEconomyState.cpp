#include "BaddieCompanionGameEconomyState.h"
#include "BaddieCompanionStateJson.h"

namespace BaddieCompanion
{
    namespace GameEconomyStatePrivate
    {
        using namespace StateJsonPrivate;
        constexpr FParseLimits Limits{512 * 1024, 16, 16384};
        constexpr int64 MaxAmount = 1000000000, MaxOfferLifetimeMs = 2592000000LL;
        constexpr int64 MaxQuoteTtlMs = 900000, MaxVendorNowMs = 4102444800000LL;
        constexpr const TCHAR* CatalogError = TEXT("Service offers couldn't load. Reopen the service to try again.");
        constexpr const TCHAR* QuoteError = TEXT("The quote couldn't load. Request a new quote.");
        constexpr const TCHAR* VendorError = TEXT("The weapon vendor couldn't load.");

        bool AsciiAlphanumeric(TCHAR C)
        {
            return (C >= 'A' && C <= 'Z') || (C >= 'a' && C <= 'z') || (C >= '0' && C <= '9');
        }
        bool ValidKey(const FString& S)
        {
            if (S.IsEmpty() || S.Len() > 160 || !AsciiAlphanumeric(S[0])) return false;
            for (const TCHAR C : S) if (!AsciiAlphanumeric(C) && C != '.' && C != '_' && C != ':' && C != '-') return false;
            return true;
        }
        bool Key(const FValue& V, FString& Out) { return String(V, Out) && ValidKey(Out); }
        bool Text(const FValue& V, FString& Out, int32 Maximum)
        {
            return String(V, Out) && Out.Len() <= Maximum;
        }
        bool Literal(const FValue& V, const TCHAR* Expected)
        {
            FString S;
            return String(V, S) && Same(S, Expected);
        }
        bool ReadInteger(const FValue& V, int64& Out) { return Integer(V, Out); }
        bool Price(const FObject& O, FGameEconomyPrice& Out)
        {
            const auto Source = Object(Field(O, TEXT("source")));
            if (!Integer(Field(O, TEXT("amount")), Out.Amount, 1) || Out.Amount > MaxAmount
                || !EnumValue<EGameEconomyPriceUnit>(Field(O, TEXT("unit")), Out.Unit,
                    {{TEXT("real-currency-minor"), EGameEconomyPriceUnit::RealCurrencyMinor},
                     {TEXT("game-value-unit"), EGameEconomyPriceUnit::GameValueUnit}})) return false;
            if (Out.Unit == EGameEconomyPriceUnit::GameValueUnit)
                return Key(Field(O, TEXT("unitKey")), Out.UnitKey)
                    && Literal(Field(Source, TEXT("kind")), TEXT("banked_game_value"))
                    && Key(Field(Source, TEXT("key")), Out.BankedSourceKey);
            EGameEconomyMainSource Main;
            if (!Literal(Field(O, TEXT("currency")), TEXT("NGN"))
                || !Literal(Field(Source, TEXT("kind")), TEXT("main_balance"))
                || !EnumValue<EGameEconomyMainSource>(Field(Source, TEXT("key")), Main,
                    {{TEXT("handyman"), EGameEconomyMainSource::Handyman}, {TEXT("partner"), EGameEconomyMainSource::Partner},
                     {TEXT("declutter_customer"), EGameEconomyMainSource::DeclutterCustomer}, {TEXT("rewards_member"), EGameEconomyMainSource::RewardsMember},
                     {TEXT("referral_partner"), EGameEconomyMainSource::ReferralPartner}, {TEXT("content_creator"), EGameEconomyMainSource::ContentCreator},
                     {TEXT("influence_creator"), EGameEconomyMainSource::InfluenceCreator}})) return false;
            Out.MainSource = Main;
            return true;
        }
        bool ExchangeRate(const FValue& V, FGameEconomyExchangeRate& Out)
        {
            const auto O = Object(V);
            return Number(Field(O, TEXT("creditsPerNaira")), Out.CreditsPerNaira)
                && Number(Field(O, TEXT("creditsPerKobo")), Out.CreditsPerKobo);
        }
        bool Policy(const FObject& O, EGameEconomyPriceUnit Unit, EGameEconomyPolicyRead Read, FGameEconomyPolicy& Out)
        {
            if ((Unit != EGameEconomyPriceUnit::RealCurrencyMinor && Unit != EGameEconomyPriceUnit::GameValueUnit)
                || (Read != EGameEconomyPolicyRead::Current && Read != EGameEconomyPolicyRead::Historical)) return false;
            if (!EnumValue<EGameEconomyActivation>(Field(O, TEXT("liveFinancialActivation")), Out.LiveFinancialActivation,
                    {{TEXT("disabled"), EGameEconomyActivation::Disabled}, {TEXT("enabled"), EGameEconomyActivation::Enabled}})
                || !EnumValue<EGameEconomyConversion>(Field(O, TEXT("conversionStatus")), Out.ConversionStatus,
                    {{TEXT("pending_approval"), EGameEconomyConversion::PendingApproval}, {TEXT("not_applicable"), EGameEconomyConversion::NotApplicable},
                     {TEXT("active"), EGameEconomyConversion::Active}})
                || !EnumValue<EGameEconomyApproval>(Field(O, TEXT("fundingStatus")), Out.FundingStatus,
                    {{TEXT("pending_approval"), EGameEconomyApproval::PendingApproval}, {TEXT("approved"), EGameEconomyApproval::Approved}})
                || !EnumValue<EGameEconomyApproval>(Field(O, TEXT("paymentSourceStatus")), Out.PaymentSourceStatus,
                    {{TEXT("pending_approval"), EGameEconomyApproval::PendingApproval}, {TEXT("approved"), EGameEconomyApproval::Approved}})
                || !EnumValue<EGameEconomyWithdrawal>(Field(O, TEXT("withdrawalStatus")), Out.WithdrawalStatus,
                    {{TEXT("unavailable"), EGameEconomyWithdrawal::Unavailable}, {TEXT("available"), EGameEconomyWithdrawal::Available}})
                || !Nullable(Field(O, TEXT("exchangeRate")), Out.ExchangeRate, ExchangeRate)) return false;
            const bool bHistorical = Read == EGameEconomyPolicyRead::Historical;
            const auto ExpectedConversion = Unit == EGameEconomyPriceUnit::RealCurrencyMinor ? EGameEconomyConversion::NotApplicable
                : bHistorical ? EGameEconomyConversion::Active : EGameEconomyConversion::PendingApproval;
            Out.Provenance = Read;
            return Out.LiveFinancialActivation == EGameEconomyActivation::Disabled
                && Out.ConversionStatus == ExpectedConversion && Out.FundingStatus == EGameEconomyApproval::PendingApproval
                && Out.PaymentSourceStatus == EGameEconomyApproval::PendingApproval
                && Out.WithdrawalStatus == (bHistorical ? EGameEconomyWithdrawal::Available : EGameEconomyWithdrawal::Unavailable)
                && (bHistorical ? Out.ExchangeRate.IsSet() && Out.ExchangeRate->CreditsPerNaira == 100 && Out.ExchangeRate->CreditsPerKobo == 1
                    : !Out.ExchangeRate.IsSet());
        }
        bool Service(const FObject& O, FGameEconomyService& Out)
        {
            return Key(Field(O, TEXT("offerId")), Out.OfferId) && Key(Field(O, TEXT("serviceKey")), Out.ServiceKey)
                && Key(Field(O, TEXT("npcKey")), Out.NpcKey) && Key(Field(O, TEXT("placeKey")), Out.PlaceKey)
                && Integer(Field(O, TEXT("revision")), Out.Revision, 1)
                && Integer(Field(O, TEXT("availabilityRevision")), Out.AvailabilityRevision, 1)
                && Text(Field(O, TEXT("label")), Out.Label, 80) && !Out.Label.TrimStartAndEnd().IsEmpty()
                && Price(Object(Field(O, TEXT("price"))), Out.Price) && Literal(Field(O, TEXT("beneficiary")), TEXT("self"))
                && Integer(Field(O, TEXT("offerStartsAt")), Out.OfferStartsAtMs, 1)
                && Integer(Field(O, TEXT("offerExpiresAt")), Out.OfferExpiresAtMs, 1) && Out.OfferExpiresAtMs > Out.OfferStartsAtMs
                && Out.OfferExpiresAtMs - Out.OfferStartsAtMs <= MaxOfferLifetimeMs
                && Integer(Field(O, TEXT("quoteTtlMs")), Out.QuoteTtlMs, 1000) && Out.QuoteTtlMs <= MaxQuoteTtlMs
                && Boolean(Field(O, TEXT("available")), Out.bAvailable)
                && Policy(Object(Field(O, TEXT("policy"))), Out.Price.Unit, EGameEconomyPolicyRead::Current, Out.Policy);
        }
        bool Quote(const FObject& O, FGameEconomyQuote& Out)
        {
            if (!Key(Field(O, TEXT("id")), Out.Id) || !Service(Object(Field(O, TEXT("service"))), Out.Service)
                || !Integer(Field(O, TEXT("issuedAt")), Out.IssuedAtMs, Out.Service.OfferStartsAtMs)
                || !Integer(Field(O, TEXT("expiresAt")), Out.ExpiresAtMs, 1) || Out.ExpiresAtMs <= Out.IssuedAtMs
                || Out.ExpiresAtMs > Out.Service.OfferExpiresAtMs || Out.ExpiresAtMs - Out.IssuedAtMs > Out.Service.QuoteTtlMs
                || !Text(Field(O, TEXT("termsFingerprint")), Out.TermsFingerprint, 4000) || Out.TermsFingerprint.IsEmpty()
                || !Integer(Field(O, TEXT("revision")), Out.Revision) || Out.Revision > 2
                || !Nullable(Field(O, TEXT("confirmedAt")), Out.ConfirmedAtMs, ReadInteger)
                || !Nullable(Field(O, TEXT("cancelledAt")), Out.CancelledAtMs, ReadInteger)
                || !EnumValue<EGameEconomyQuoteStatus>(Field(O, TEXT("status")), Out.Status,
                    {{TEXT("quoted"), EGameEconomyQuoteStatus::Quoted}, {TEXT("approval_required"), EGameEconomyQuoteStatus::ApprovalRequired},
                     {TEXT("cancelled"), EGameEconomyQuoteStatus::Cancelled}, {TEXT("fulfilled"), EGameEconomyQuoteStatus::Fulfilled}})) return false;
            if (Out.ConfirmedAtMs.IsSet() && (*Out.ConfirmedAtMs < Out.IssuedAtMs || *Out.ConfirmedAtMs >= Out.ExpiresAtMs)) return false;
            if (Out.CancelledAtMs.IsSet() && *Out.CancelledAtMs < (Out.ConfirmedAtMs.IsSet() ? *Out.ConfirmedAtMs : Out.IssuedAtMs)) return false;
            if (Out.Status == EGameEconomyQuoteStatus::Quoted)
                return Out.Revision == 0 && !Out.ConfirmedAtMs.IsSet() && !Out.CancelledAtMs.IsSet();
            if (Out.Status == EGameEconomyQuoteStatus::Cancelled)
                return Out.Revision == (Out.ConfirmedAtMs.IsSet() ? 2 : 1) && Out.CancelledAtMs.IsSet();
            return Out.Revision == 1 && Out.ConfirmedAtMs.IsSet() && !Out.CancelledAtMs.IsSet();
        }
        template <typename T, typename F> bool Array(const FValue& V, TArray<T>& Out, int32 Maximum, F Read)
        {
            if (!V.IsValid() || V->Type != EJson::Array || V->AsArray().Num() > Maximum) return false;
            for (const auto& Item : V->AsArray())
            {
                T Next;
                if (!Read(Object(Item), Next)) return false;
                Out.Add(MoveTemp(Next));
            }
            return true;
        }
        bool Catalog(const FObject& O, FGameEconomyCatalog& Out)
        {
            return Literal(Field(O, TEXT("liveFinancialActivation")), TEXT("disabled"))
                && Array(Field(O, TEXT("services")), Out.Services, 32, Service);
        }
        struct FVendorRule { const TCHAR* Key; int64 Amount, Stock; };
        // Read validation only, matching companionWeaponVendorRules.items at the canonical pin.
        // The client still caps each vendor response at THREE rows despite these eleven rules.
        constexpr FVendorRule VendorRules[] = {
            {TEXT("street-weapon:bat"), 120, 24}, {TEXT("street-weapon:pistol"), 600, 12}, {TEXT("street-ammo:pistol"), 60, 100},
            {TEXT("street-weapon:revolver"), 750, 10}, {TEXT("street-ammo:revolver"), 60, 100},
            {TEXT("street-weapon:smg"), 1100, 8}, {TEXT("street-ammo:smg"), 120, 100},
            {TEXT("street-weapon:shotgun"), 900, 8}, {TEXT("street-ammo:shotgun"), 75, 100},
            {TEXT("street-weapon:rifle"), 1500, 6}, {TEXT("street-ammo:rifle"), 120, 100}
        };
        bool VendorItem(const FObject& O, FGameEconomyVendorItem& Out)
        {
            if (!Key(Field(O, TEXT("serviceKey")), Out.ServiceKey) || !Text(Field(O, TEXT("label")), Out.Label, 8192)
                || !Price(Object(Field(O, TEXT("price"))), Out.Price) || !Integer(Field(O, TEXT("stock")), Out.Stock)
                || !Boolean(Field(O, TEXT("owned")), Out.bOwned) || Out.Price.Unit != EGameEconomyPriceUnit::GameValueUnit
                || !Same(Out.Price.UnitKey, TEXT("lagos-credit")) || !Same(Out.Price.BankedSourceKey, TEXT("haul"))) return false;
            for (const auto& Rule : VendorRules)
                if (Same(Out.ServiceKey, Rule.Key) && Out.Price.Amount == Rule.Amount && Out.Stock <= Rule.Stock) return true;
            return false;
        }
        bool Vendor(const FObject& O, FGameEconomyVendor& Out)
        {
            const auto Error = Field(O, TEXT("error"));
            return Array(Field(O, TEXT("services")), Out.Services, 3, VendorItem)
                && Boolean(Field(O, TEXT("ready")), Out.bReady) && Boolean(Field(O, TEXT("busy")), Out.bBusy)
                && Error.IsValid() && Error->Type == EJson::Null;
        }
        template <typename T, typename F> bool Decode(const FString& Json, T& Out, FString& OutError, const TCHAR* Error, F Read)
        {
            FObject O;
            T Next;
            if (!Parse(Json, O, Limits) || !Read(O, Next)) { Out = {}; OutError = Error; return false; }
            Out = MoveTemp(Next); OutError.Reset(); return true;
        }
        bool SameScope(const FGameEconomyVendorScope& A, const FGameEconomyVendorScope& B)
        {
            return Same(A.GameRoomId, B.GameRoomId) && Same(A.InstanceId, B.InstanceId) && Same(A.LifeId, B.LifeId)
                && A.Revision == B.Revision && A.EquipmentRevision == B.EquipmentRevision;
        }
        bool ValidScope(const FGameEconomyVendorScope& S)
        {
            return !S.GameRoomId.IsEmpty() && S.GameRoomId.Len() <= 240 && !S.InstanceId.IsEmpty() && S.InstanceId.Len() <= 240
                && !S.LifeId.IsEmpty() && S.LifeId.Len() <= 240 && S.Revision >= 0 && S.Revision <= MaxInteger
                && S.EquipmentRevision >= 0 && S.EquipmentRevision <= MaxInteger;
        }
        FString PublicError(const FString& Message)
        {
            return Message.Len() <= 8192 ? Message : FString(TEXT("The service request couldn't finish. Try again."));
        }
    }

    bool DecodeGameEconomyPrice(const FString& Json, FGameEconomyPrice& Out, FString& OutError)
    {
        return GameEconomyStatePrivate::Decode(Json, Out, OutError, TEXT("The service price couldn't be read."), GameEconomyStatePrivate::Price);
    }
    bool DecodeGameEconomyPolicy(const FString& Json, EGameEconomyPriceUnit Unit, EGameEconomyPolicyRead Read,
        FGameEconomyPolicy& Out, FString& OutError)
    {
        using namespace GameEconomyStatePrivate;
        return Decode(Json, Out, OutError, TEXT("The service policy couldn't be read."),
            [Unit, Read](const FObject& O, FGameEconomyPolicy& P) { return Policy(O, Unit, Read, P); });
    }
    bool DecodeGameEconomyService(const FString& Json, FGameEconomyService& Out, FString& OutError)
    {
        return GameEconomyStatePrivate::Decode(Json, Out, OutError, GameEconomyStatePrivate::CatalogError, GameEconomyStatePrivate::Service);
    }
    bool DecodeGameEconomyQuote(const FString& Json, FGameEconomyQuote& Out, FString& OutError)
    {
        return GameEconomyStatePrivate::Decode(Json, Out, OutError, GameEconomyStatePrivate::QuoteError, GameEconomyStatePrivate::Quote);
    }
    bool DecodeGameEconomyCatalog(const FString& Json, FGameEconomyCatalog& Out, FString& OutError)
    {
        return GameEconomyStatePrivate::Decode(Json, Out, OutError, GameEconomyStatePrivate::CatalogError, GameEconomyStatePrivate::Catalog);
    }
    bool DecodeGameEconomyVendor(const FString& Json, FGameEconomyVendor& Out, FString& OutError)
    {
        return GameEconomyStatePrivate::Decode(Json, Out, OutError, GameEconomyStatePrivate::VendorError, GameEconomyStatePrivate::Vendor);
    }

    bool FGameEconomyReadState::Enabled() const { return !bDisposed && bObserving && !AccountId.IsEmpty(); }
    FGameEconomyReadState::FReadSlot& FGameEconomyReadState::Slot(EGameEconomyReadKind Kind)
    {
        return ReadSlots[static_cast<uint8>(Kind)];
    }
    const FGameEconomyReadState::FReadSlot& FGameEconomyReadState::Slot(EGameEconomyReadKind Kind) const
    {
        return ReadSlots[static_cast<uint8>(Kind)];
    }
    void FGameEconomyReadState::RetireReads()
    {
        for (auto& Read : ReadSlots) Read.Retire();
        VendorNowMs.Reset();
    }
    bool FGameEconomyReadState::SelectAccount(const FString& Id)
    {
        if (bDisposed || Id.Len() > 240) return false;
        if (StateJsonPrivate::Same(AccountId, Id)) return true;
        Clear(); AccountId = Id; return true;
    }
    bool FGameEconomyReadState::BeginObservation()
    {
        if (bDisposed || AccountId.IsEmpty()) return false;
        if (!bObserving) { bObserving = true; RetireReads(); }
        return true;
    }
    void FGameEconomyReadState::EndObservation()
    {
        if (bDisposed) return;
        bObserving = false; RetireReads();
    }
    bool FGameEconomyReadState::SelectCatalog(const FString& Place, const FString& Npc)
    {
        using namespace GameEconomyStatePrivate;
        if (bDisposed || AccountId.IsEmpty()) return false;
        if (Same(PlaceKey, Place) && Same(NpcKey, Npc)) return true;
        if (!ValidKey(Place) || !ValidKey(Npc)) { View.Error = TEXT("Choose an existing NPC service."); return false; }
        PlaceKey = Place; NpcKey = Npc; QuoteId.Reset(); View = {}; RetireReads(); return true;
    }
    bool FGameEconomyReadState::RecoverQuote(const TOptional<FString>& Id)
    {
        if (bDisposed || AccountId.IsEmpty()) return false;
        if (Id.IsSet() && !GameEconomyStatePrivate::ValidKey(*Id)) { View.Error = TEXT("Invalid quote ID."); return false; }
        QuoteId = Id; View.Quote.Reset(); View.bQuoteReady = !Id.IsSet(); View.Error.Reset();
        Slot(EGameEconomyReadKind::Quote).Retire(); return true;
    }
    bool FGameEconomyReadState::SelectVendor(const TOptional<FGameEconomyVendorScope>& Scope)
    {
        using namespace GameEconomyStatePrivate;
        if (bDisposed || AccountId.IsEmpty() || (Scope.IsSet() && !ValidScope(*Scope))) return false;
        if (Scope.IsSet() == VendorScope.IsSet() && (!Scope.IsSet() || SameScope(*Scope, *VendorScope))) return true;
        VendorScope = Scope; View.Vendor = FGameEconomyVendor{};
        Slot(EGameEconomyReadKind::Vendor).Retire(); VendorNowMs.Reset(); return true;
    }
    void FGameEconomyReadState::Reload()
    {
        if (bDisposed) return;
        View.bReady = false; View.bQuoteReady = !QuoteId.IsSet(); View.Error.Reset(); RetireReads();
    }
    void FGameEconomyReadState::Clear()
    {
        if (bDisposed) return;
        bObserving = false; AccountGeneration = FGuid::NewGuid(); RetireReads();
        AccountId.Reset(); PlaceKey.Reset(); NpcKey.Reset(); QuoteId.Reset(); VendorScope.Reset(); View = {};
    }
    void FGameEconomyReadState::Dispose()
    {
        if (bDisposed) return;
        Clear(); bDisposed = true;
    }
    bool FGameEconomyReadState::Capture(EGameEconomyReadKind Kind, FGameEconomyReadReceipt& Out)
    {
        Out = {};
        if (!Enabled()) return false;
        switch (Kind)
        {
        case EGameEconomyReadKind::Catalog: if (PlaceKey.IsEmpty() || NpcKey.IsEmpty()) return false; break;
        case EGameEconomyReadKind::Quote: if (!QuoteId.IsSet()) return false; break;
        case EGameEconomyReadKind::Vendor: if (!VendorScope.IsSet() || !VendorNowMs.IsSet()) return false; break;
        default: return false;
        }
        Slot(Kind).bActive = true;
        Out.AccountGeneration = AccountGeneration; Out.ReadGeneration = Slot(Kind).Generation; Out.Kind = Kind;
        if (Kind == EGameEconomyReadKind::Catalog) { Out.PlaceKey = PlaceKey; Out.NpcKey = NpcKey; }
        else if (Kind == EGameEconomyReadKind::Quote) Out.QuoteId = QuoteId;
        else { Out.VendorScope = VendorScope; Out.VendorNowMs = VendorNowMs; }
        return true;
    }
    bool FGameEconomyReadState::CaptureCatalog(FGameEconomyReadReceipt& Out) { return Capture(EGameEconomyReadKind::Catalog, Out); }
    bool FGameEconomyReadState::CaptureQuote(FGameEconomyReadReceipt& Out) { return Capture(EGameEconomyReadKind::Quote, Out); }
    bool FGameEconomyReadState::CaptureVendor(int64 NowMs, FGameEconomyReadReceipt& Out)
    {
        Out = {};
        if (!Enabled() || !VendorScope.IsSet() || NowMs < 0 || NowMs > GameEconomyStatePrivate::MaxVendorNowMs) return false;
        if (!Slot(EGameEconomyReadKind::Vendor).bActive) VendorNowMs = NowMs;
        return Capture(EGameEconomyReadKind::Vendor, Out);
    }
    bool FGameEconomyReadState::Current(const FGameEconomyReadReceipt& R) const
    {
        if (!Enabled() || R.AccountGeneration != AccountGeneration) return false;
        switch (R.Kind)
        {
        case EGameEconomyReadKind::Catalog:
        case EGameEconomyReadKind::Quote:
        case EGameEconomyReadKind::Vendor: return Slot(R.Kind).bActive && Slot(R.Kind).Generation == R.ReadGeneration;
        default: return false;
        }
    }
    void FGameEconomyReadState::Fail(EGameEconomyReadKind Kind, const FString& Message)
    {
        const FString Error = GameEconomyStatePrivate::PublicError(Message);
        if (Kind == EGameEconomyReadKind::Vendor)
        {
            FGameEconomyVendor Vendor;
            Vendor.Error = Error; View.Vendor = MoveTemp(Vendor);
        }
        else
        {
            if (Kind == EGameEconomyReadKind::Catalog) { View.Services.Reset(); View.bReady = true; }
            else { View.Quote.Reset(); View.bQuoteReady = true; }
            View.Error = Error;
        }
    }
    EGameEconomyReadResult FGameEconomyReadState::Observe(const FGameEconomyReadReceipt& R, const FString& Json)
    {
        using namespace GameEconomyStatePrivate;
        if (!Current(R)) return EGameEconomyReadResult::StaleReceipt;
        FString Error;
        if (R.Kind == EGameEconomyReadKind::Catalog)
        {
            FGameEconomyCatalog CatalogValue;
            bool bValid = DecodeGameEconomyCatalog(Json, CatalogValue, Error);
            if (bValid) for (const auto& S : CatalogValue.Services)
                if (!Same(S.PlaceKey, R.PlaceKey) || !Same(S.NpcKey, R.NpcKey)) { bValid = false; Error = CatalogError; break; }
            if (!bValid) { Fail(R.Kind, Error); return EGameEconomyReadResult::InvalidResponse; }
            View.Services = MoveTemp(CatalogValue.Services); View.bReady = true; View.Error.Reset();
        }
        else if (R.Kind == EGameEconomyReadKind::Quote)
        {
            FGameEconomyQuote QuoteValue;
            if (!DecodeGameEconomyQuote(Json, QuoteValue, Error) || !R.QuoteId.IsSet() || !Same(QuoteValue.Id, *R.QuoteId))
            { Fail(R.Kind, QuoteError); return EGameEconomyReadResult::InvalidResponse; }
            // Compare only the CURRENT displayed quote. A failure/recovery clears this floor.
            if (View.Quote.IsSet() && Same(View.Quote->Id, QuoteValue.Id) && View.Quote->Revision > QuoteValue.Revision)
                return EGameEconomyReadResult::Retained;
            View.Quote = MoveTemp(QuoteValue); View.bQuoteReady = true; View.Error.Reset();
        }
        else
        {
            FGameEconomyVendor VendorValue;
            if (!DecodeGameEconomyVendor(Json, VendorValue, Error)) { Fail(R.Kind, Error); return EGameEconomyReadResult::InvalidResponse; }
            View.Vendor = MoveTemp(VendorValue);
        }
        return EGameEconomyReadResult::Accepted;
    }
    bool FGameEconomyReadState::ObserveFailure(const FGameEconomyReadReceipt& R, const FString& PublicMessage)
    {
        if (!Current(R)) return false;
        // Ordinary failures retain the subscription generation. A later success can recover.
        Fail(R.Kind, PublicMessage); return true;
    }
}
