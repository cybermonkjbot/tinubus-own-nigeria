#include "BaddieCompanionHaulState.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

#include <cmath>
#include <initializer_list>

namespace BaddieCompanion
{
    namespace HaulStatePrivate
    {
        constexpr int64 MaxInteger = 9007199254740991LL;
        constexpr int64 MaxAmount = 1000000000;
        constexpr int32 MaxEntries = 128, MaxQuantity = 10000, MaxPendingReads = 64;
        constexpr int32 MaxDocumentCharacters = 512 * 1024, MaxCursorCharacters = 16384;
        using FValue = TSharedPtr<FJsonValue>;
        using FObject = TSharedPtr<FJsonObject>;

        bool Same(const FString& A, const FString& B) { return A.Equals(B, ESearchCase::CaseSensitive); }
        bool Parse(const FString& Json, FObject& Out)
        {
            if (Json.IsEmpty() || Json.Len() > MaxDocumentCharacters) return false;
            bool bString = false, bEscape = false;
            int32 Depth = 0, Separators = 0;
            for (const TCHAR C : Json)
            {
                if (bString)
                {
                    if (bEscape) bEscape = false;
                    else if (C == TEXT('\\')) bEscape = true;
                    else if (C == TEXT('"')) bString = false;
                    continue;
                }
                if (C == TEXT('"')) bString = true;
                else if (C == TEXT('{') || C == TEXT('[')) { if (++Depth > 12) return false; }
                else if (C == TEXT('}') || C == TEXT(']')) { if (--Depth < 0) return false; }
                else if (C == TEXT(',') || C == TEXT(':')) { if (++Separators > 16384) return false; }
            }
            if (bString || Depth != 0) return false;
            return FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Out) && Out.IsValid();
        }
        FObject Object(const FValue& Value)
        {
            return Value.IsValid() && Value->Type == EJson::Object ? Value->AsObject() : nullptr;
        }
        FValue Field(const FObject& O, const TCHAR* Name) { return O.IsValid() ? O->TryGetField(Name) : nullptr; }
        bool Text(const FValue& Value, FString& Out, int32 Maximum)
        {
            return Value.IsValid() && Value->Type == EJson::String && Value->TryGetString(Out) && Out.Len() <= Maximum;
        }
        bool Id(const FValue& Value, FString& Out) { return Text(Value, Out, 240) && !Out.IsEmpty(); }
        bool Key(const FValue& Value, FString& Out)
        {
            // Canonical keys may contain whitespace. Validate but never trim or normalize.
            return Text(Value, Out, 160) && !Out.TrimStartAndEnd().IsEmpty();
        }
        bool Cursor(const FValue& Value, FString& Out) { return Text(Value, Out, MaxCursorCharacters); }
        bool Integer(const FValue& Value, int64& Out, int64 Max = MaxInteger, int64 Min = 0)
        {
            double N = 0;
            if (!Value.IsValid() || Value->Type != EJson::Number || !Value->TryGetNumber(N)
                || !std::isfinite(N) || N < Min || N > Max || std::floor(N) != N) return false;
            Out = static_cast<int64>(N);
            return true;
        }
        bool Time(const FValue& Value, double& Out)
        {
            return Value.IsValid() && Value->Type == EJson::Number && Value->TryGetNumber(Out)
                && std::isfinite(Out) && Out >= 0 && Out <= MaxInteger;
        }
        bool Boolean(const FValue& Value, bool& Out)
        {
            return Value.IsValid() && Value->Type == EJson::Boolean && Value->TryGetBool(Out);
        }
        template <typename T> bool Enum(const FValue& Value, T& Out,
            std::initializer_list<TPair<const TCHAR*, T>> Choices)
        {
            FString Name;
            if (!Text(Value, Name, 64)) return false;
            for (const auto& Choice : Choices)
                if (Same(Name, Choice.Key)) { Out = Choice.Value; return true; }
            return false;
        }
        bool Status(const FValue& Value, EHaulStatus& Out)
        {
            return Enum<EHaulStatus>(Value, Out, {{TEXT("carried"), EHaulStatus::Carried},
                {TEXT("banked"), EHaulStatus::Banked}, {TEXT("forfeited"), EHaulStatus::Forfeited}});
        }
        template <typename T, typename F> bool OptionalNullable(const FValue& Value, THaulField<T>& Out, F Read)
        {
            if (!Value.IsValid()) return true;
            if (Value->Type == EJson::Null) { Out.Presence = EHaulPresence::Null; return true; }
            if (!Read(Value, Out.Value)) return false;
            Out.Presence = EHaulPresence::Value;
            return true;
        }
        bool Source(const FValue& Value, FHaulSource& Out)
        {
            const auto O = Object(Value);
            return Enum<EHaulSourceKind>(Field(O, TEXT("kind")), Out.Kind, {
                {TEXT("world-loot"), EHaulSourceKind::WorldLoot}, {TEXT("job"), EHaulSourceKind::Job},
                {TEXT("delivery"), EHaulSourceKind::Delivery}, {TEXT("mission"), EHaulSourceKind::Mission},
                {TEXT("activity"), EHaulSourceKind::Activity}, {TEXT("pos-withdrawal"), EHaulSourceKind::PosWithdrawal},
                {TEXT("street-trade"), EHaulSourceKind::StreetTrade}})
                && Key(Field(O, TEXT("key")), Out.Key);
        }
        bool Reward(const FValue& Value, FHaulReward& Out)
        {
            const auto O = Object(Value);
            if (!Enum<EHaulRewardKind>(Field(O, TEXT("kind")), Out.Kind,
                {{TEXT("item"), EHaulRewardKind::Item}, {TEXT("money"), EHaulRewardKind::Money}})) return false;
            if (Out.Kind == EHaulRewardKind::Item)
                return Key(Field(O, TEXT("itemKey")), Out.ItemKey)
                    && Integer(Field(O, TEXT("quantity")), Out.Quantity, MaxQuantity, 1);
            if (!Integer(Field(O, TEXT("amount")), Out.Amount, MaxAmount)
                || !Enum<EHaulMoneyUnit>(Field(O, TEXT("unit")), Out.Unit,
                    {{TEXT("game-value-unit"), EHaulMoneyUnit::GameValueUnit},
                     {TEXT("real-currency-minor"), EHaulMoneyUnit::RealCurrencyMinor}})) return false;
            if (Out.Unit == EHaulMoneyUnit::GameValueUnit) return Key(Field(O, TEXT("unitKey")), Out.UnitKey);
            return Text(Field(O, TEXT("currency")), Out.Currency, 160)
                && Text(Field(O, TEXT("fundingReference")), Out.FundingReference, 8192);
        }
        bool Entry(const FValue& Value, FHaulEntry& Out)
        {
            const auto O = Object(Value);
            if (!Id(Field(O, TEXT("id")), Out.Id) || !Id(Field(O, TEXT("outingId")), Out.OutingId)
                || !Source(Field(O, TEXT("source")), Out.Source) || !Reward(Field(O, TEXT("reward")), Out.Reward)
                || !Status(Field(O, TEXT("status")), Out.Status) || !Time(Field(O, TEXT("grantedAt")), Out.GrantedAtMs)) return false;
            const auto Original = Field(O, TEXT("originalMoneyAmount"));
            if (Original.IsValid())
            {
                int64 Amount = 0;
                if (!Integer(Original, Amount, MaxAmount, 1)) return false;
                Out.OriginalMoneyAmount = Amount;
            }
            return true;
        }
        bool Entries(const FValue& Value, TArray<FHaulEntry>& Out, int32 Maximum)
        {
            if (!Value.IsValid() || Value->Type != EJson::Array || Value->AsArray().Num() > Maximum) return false;
            Out.Reserve(Value->AsArray().Num());
            for (const auto& Item : Value->AsArray())
            {
                FHaulEntry Next;
                if (!Entry(Item, Next)) return false;
                Out.Add(MoveTemp(Next));
            }
            return true;
        }
        bool Outing(const FValue& Value, FHaulOuting& Out)
        {
            const auto O = Object(Value);
            if (!Id(Field(O, TEXT("id")), Out.Id) || !Key(Field(O, TEXT("characterId")), Out.CharacterId)
                || !Key(Field(O, TEXT("worldId")), Out.WorldId) || !Key(Field(O, TEXT("instanceId")), Out.InstanceId)
                || !Status(Field(O, TEXT("status")), Out.Status) || !Integer(Field(O, TEXT("revision")), Out.Revision)
                || !Integer(Field(O, TEXT("entryCount")), Out.EntryCount, MaxEntries)
                || !Time(Field(O, TEXT("departedAt")), Out.DepartedAtMs)) return false;
            const auto Resolved = Field(O, TEXT("resolvedAt"));
            if (!Resolved.IsValid()) return false;
            if (Resolved->Type != EJson::Null)
            {
                double ResolvedAt = 0;
                if (!Time(Resolved, ResolvedAt)) return false;
                Out.ResolvedAtMs = ResolvedAt;
            }
            return true;
        }
        bool Literal(const FObject& O, const TCHAR* Name, const FString& Expected)
        {
            FString Actual;
            return Text(Field(O, Name), Actual, 32) && Same(Actual, Expected);
        }
    }

    bool DecodeHaulSnapshot(const FString& Json, FHaulSnapshot& Out, FString& OutError)
    {
        using namespace HaulStatePrivate;
        FObject O;
        FHaulSnapshot Next;
        bool bValid = Parse(Json, O);
        const auto OutingValue = Field(O, TEXT("outing"));
        bValid = bValid && OutingValue.IsValid() && Entries(Field(O, TEXT("entries")), Next.Entries, MaxEntries)
            && Literal(O, TEXT("cashConversion"), Next.CashConversion)
            && Literal(O, TEXT("withdrawal"), Next.Withdrawal)
            && Literal(O, TEXT("bankedTransfer"), Next.BankedTransfer);
        if (bValid && OutingValue->Type != EJson::Null)
        {
            FHaulOuting Value;
            bValid = Outing(OutingValue, Value);
            if (bValid) Next.Outing = MoveTemp(Value);
        }
        if (bValid)
        {
            // These are the public snapshot endpoint's consistency checks, not settlement.
            bValid = Next.Outing.IsSet() ? Next.Outing->EntryCount == Next.Entries.Num() : Next.Entries.IsEmpty();
            for (const auto& E : Next.Entries)
                if (!Next.Outing.IsSet() || !Same(E.OutingId, Next.Outing->Id) || E.Status != Next.Outing->Status) bValid = false;
        }
        if (!bValid) { Out = {}; OutError = TEXT("The haul snapshot is unavailable."); return false; }
        Out = MoveTemp(Next);
        OutError.Reset();
        return true;
    }

    bool DecodeHaulPage(const FString& Json, FHaulPage& Out, FString& OutError)
    {
        using namespace HaulStatePrivate;
        FObject O;
        FHaulPage Next;
        bool bValid = Parse(Json, O);
        bValid = bValid && Entries(Field(O, TEXT("page")), Next.Page, 50)
            && Cursor(Field(O, TEXT("continueCursor")), Next.ContinueCursor)
            && Boolean(Field(O, TEXT("isDone")), Next.bIsDone)
            && OptionalNullable(Field(O, TEXT("splitCursor")), Next.SplitCursor, Cursor)
            && OptionalNullable(Field(O, TEXT("pageStatus")), Next.PageStatus,
                [](const FValue& V, EHaulPageStatus& S) { return Enum<EHaulPageStatus>(V, S,
                    {{TEXT("SplitRecommended"), EHaulPageStatus::SplitRecommended},
                     {TEXT("SplitRequired"), EHaulPageStatus::SplitRequired}}); });
        for (const auto& E : Next.Page) if (E.Status != EHaulStatus::Banked) bValid = false;
        if (!bValid) { Out = {}; OutError = TEXT("The banked haul page is unavailable."); return false; }
        Out = MoveTemp(Next);
        OutError.Reset();
        return true;
    }

    bool DecodeHaulBalance(const FString& Json, FHaulBalance& Out, FString& OutError)
    {
        using namespace HaulStatePrivate;
        FObject O;
        FHaulBalance Next;
        Next.bNullResponse = Json.Len() <= MaxDocumentCharacters && Same(Json.TrimStartAndEnd(), TEXT("null"));
        const bool bValid = Next.bNullResponse || (Parse(Json, O)
            && OptionalNullable(Field(O, TEXT("bankedCredits")), Next.BankedCredits,
                [](const FValue& V, int64& N) { return Integer(V, N); }));
        if (!bValid) { Out = {}; OutError = TEXT("The banked game-credit balance is unavailable."); return false; }
        Out = MoveTemp(Next);
        OutError.Reset();
        return true;
    }

    bool HaulMoneyGroups(const TArray<FHaulEntry>& Entries, TArray<FHaulMoneyGroup>& Out, FString& OutError)
    {
        using namespace HaulStatePrivate;
        TArray<FHaulMoneyGroup> Next;
        bool bValid = true;
        for (const auto& Entry : Entries)
        {
            const auto& Reward = Entry.Reward;
            if (Reward.Kind != EHaulRewardKind::Money || Entry.Status == EHaulStatus::Forfeited) continue;
            if (Reward.Amount < 0 || Reward.Amount > MaxInteger) { bValid = false; break; }
            if (Reward.Amount == 0) continue;
            const FString& UnitKey = Reward.Unit == EHaulMoneyUnit::GameValueUnit ? Reward.UnitKey : Reward.Currency;
            auto* Group = Next.FindByPredicate([&](const FHaulMoneyGroup& G)
            { return G.Status == Entry.Status && G.Unit == Reward.Unit && Same(G.UnitKey, UnitKey); });
            if (!Group)
            {
                FHaulMoneyGroup G;
                G.Unit = Reward.Unit; G.UnitKey = UnitKey; G.Status = Entry.Status;
                Group = &Next.Add_GetRef(MoveTemp(G));
            }
            if (Reward.Amount > MaxInteger - Group->Amount) { bValid = false; break; }
            Group->Amount += Reward.Amount;
            Group->SourceKeys.Add(Entry.Source.Key);
            if (Reward.Unit == EHaulMoneyUnit::RealCurrencyMinor) Group->FundingReferences.Add(Reward.FundingReference);
        }
        if (!bValid) { Out.Reset(); OutError = TEXT("The haul amount needs review."); return false; }
        Out = MoveTemp(Next);
        OutError.Reset();
        return true;
    }

    void FHaulReadState::ResetGeneration()
    {
        Generation = FGuid::NewGuid();
        PendingBanked.Reset();
        PendingBalance.Reset();
        State = {};
    }
    bool FHaulReadState::SelectAccount(const FString& InAccountId)
    {
        if (bDisposed || InAccountId.Len() > 240 || (!InAccountId.IsEmpty() && InAccountId.TrimStartAndEnd().IsEmpty())) return false;
        if (HaulStatePrivate::Same(AccountId, InAccountId)) return true;
        AccountId = InAccountId;
        bObserving = false;
        ResetGeneration();
        return true;
    }
    FHaulObservationReceipt FHaulReadState::Scope() const
    {
        FHaulObservationReceipt Receipt;
        Receipt.Generation = Generation;
        Receipt.AccountId = AccountId;
        return Receipt;
    }
    bool FHaulReadState::Current(const FHaulObservationReceipt& Receipt) const
    {
        return !bDisposed && !AccountId.IsEmpty() && Generation == Receipt.Generation
            && HaulStatePrivate::Same(AccountId, Receipt.AccountId);
    }
    bool FHaulReadState::Current(const FHaulBankedReceipt& Receipt) const
    {
        return Current(Receipt.Scope) && PendingBanked.Contains(Receipt.RequestId);
    }
    bool FHaulReadState::Current(const FHaulBalanceReceipt& Receipt) const
    {
        return Current(Receipt.Scope) && PendingBalance.Contains(Receipt.RequestId);
    }
    bool FHaulReadState::BeginObservation(FHaulObservationReceipt& Out)
    {
        Out = {};
        if (bDisposed || AccountId.IsEmpty()) return false;
        if (!bObserving) { ResetGeneration(); bObserving = true; }
        Out = Scope();
        return true;
    }
    bool FHaulReadState::CaptureObservation(FHaulObservationReceipt& Out) const
    {
        Out = {};
        if (bDisposed || !bObserving || AccountId.IsEmpty()) return false;
        Out = Scope();
        return true;
    }
    void FHaulReadState::EndObservation()
    {
        if (bDisposed) return;
        bObserving = false;
        ResetGeneration();
    }
    void FHaulReadState::Reload() { if (!bDisposed) ResetGeneration(); }
    void FHaulReadState::Clear()
    {
        if (bDisposed) return;
        EndObservation();
        AccountId.Reset();
    }
    void FHaulReadState::Dispose() { if (!bDisposed) { Clear(); bDisposed = true; } }

    EHaulReadResult FHaulReadState::Observe(const FHaulObservationReceipt& Receipt, const FString& Json)
    {
        if (!bObserving || !Current(Receipt)) return EHaulReadResult::StaleReceipt;
        FHaulSnapshot Next;
        FString Error;
        if (!DecodeHaulSnapshot(Json, Next, Error))
        {
            ObserveFailure(Receipt, Error);
            return EHaulReadResult::InvalidResponse;
        }
        State.Wire = MoveTemp(Next);
        State.bReady = true;
        State.Error.Reset();
        return EHaulReadResult::Accepted;
    }
    bool FHaulReadState::ObserveFailure(const FHaulObservationReceipt& Receipt, const FString& PublicMessage)
    {
        if (!bObserving || !Current(Receipt)) return false;
        const FString Error = PublicMessage.Len() <= 8192 ? PublicMessage : TEXT("The haul snapshot is unavailable.");
        State = {};
        State.bReady = true;
        State.Error = Error;
        return true;
    }
    bool FHaulReadState::CaptureBanked(FHaulBankedReceipt& Out, const TOptional<FHaulCursor>& Cursor, int32 Limit)
    {
        Out = {};
        if (bDisposed || AccountId.IsEmpty() || Limit < 1 || Limit > 50
            || PendingBanked.Num() + PendingBalance.Num() >= HaulStatePrivate::MaxPendingReads
            || (Cursor.IsSet() && !Current(Cursor->Scope))) return false;
        Out.Scope = Scope();
        Out.RequestId = FGuid::NewGuid();
        Out.Limit = Limit;
        if (Cursor.IsSet()) Out.Cursor = Cursor->Value;
        PendingBanked.Add(Out.RequestId);
        return true;
    }
    bool FHaulReadState::EncodeBankedArguments(const FHaulBankedReceipt& Receipt, FString& OutJson) const
    {
        OutJson.Reset();
        if (!Current(Receipt)) return false;
        const auto Options = MakeShared<FJsonObject>();
        if (Receipt.Cursor.IsSet()) Options->SetStringField(TEXT("cursor"), Receipt.Cursor.GetValue());
        else Options->SetField(TEXT("cursor"), MakeShared<FJsonValueNull>());
        Options->SetNumberField(TEXT("numItems"), Receipt.Limit);
        const auto Args = MakeShared<FJsonObject>();
        Args->SetObjectField(TEXT("paginationOpts"), Options);
        const bool bWritten = FJsonSerializer::Serialize(Args, TJsonWriterFactory<>::Create(&OutJson));
        if (!bWritten) OutJson.Reset();
        return bWritten;
    }
    EHaulReadResult FHaulReadState::AcceptBanked(const FHaulBankedReceipt& Receipt, const FString& Json, FHaulBankedRead& Out)
    {
        if (!Current(Receipt)) { Out = {}; return EHaulReadResult::StaleReceipt; }
        PendingBanked.Remove(Receipt.RequestId);
        FHaulPage Page;
        FString Error;
        if (!DecodeHaulPage(Json, Page, Error)) { Out = {}; return EHaulReadResult::InvalidResponse; }
        FHaulBankedRead Next;
        Next.ContinueCursor.Scope = Receipt.Scope;
        Next.ContinueCursor.Value = Page.ContinueCursor;
        if (Page.SplitCursor.HasValue())
        {
            FHaulCursor Cursor;
            Cursor.Scope = Receipt.Scope;
            Cursor.Value = Page.SplitCursor.Value;
            Next.SplitCursor = MoveTemp(Cursor);
        }
        Next.Wire = MoveTemp(Page);
        Out = MoveTemp(Next);
        return EHaulReadResult::Accepted;
    }
    bool FHaulReadState::FailBanked(const FHaulBankedReceipt& Receipt)
    {
        if (!Current(Receipt)) return false;
        PendingBanked.Remove(Receipt.RequestId);
        return true;
    }
    bool FHaulReadState::CaptureBalance(FHaulBalanceReceipt& Out)
    {
        Out = {};
        if (bDisposed || AccountId.IsEmpty()
            || PendingBanked.Num() + PendingBalance.Num() >= HaulStatePrivate::MaxPendingReads) return false;
        Out.Scope = Scope();
        Out.RequestId = FGuid::NewGuid();
        PendingBalance.Add(Out.RequestId);
        return true;
    }
    EHaulReadResult FHaulReadState::AcceptBalance(const FHaulBalanceReceipt& Receipt, const FString& Json, FHaulBalance& Out)
    {
        Out = {};
        if (!Current(Receipt)) return EHaulReadResult::StaleReceipt;
        PendingBalance.Remove(Receipt.RequestId);
        FString Error;
        return DecodeHaulBalance(Json, Out, Error) ? EHaulReadResult::Accepted : EHaulReadResult::InvalidResponse;
    }
    bool FHaulReadState::FailBalance(const FHaulBalanceReceipt& Receipt)
    {
        if (!Current(Receipt)) return false;
        PendingBalance.Remove(Receipt.RequestId);
        return true;
    }
}
