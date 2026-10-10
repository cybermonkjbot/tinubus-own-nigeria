#include "BaddieCompanionPosState.h"

#include "BaddieCompanionStateJson.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include <cmath>

namespace BaddieCompanion
{
    namespace PosStatePrivate
    {
        using namespace StateJsonPrivate;
        // Native allocation limits, not transaction limits. Canonical catalog has three amounts.
        constexpr FParseLimits Limits{65536, 8, 1024};
        constexpr int32 MaxIdLength = 256, MaxTextLength = 4096, MaxAmounts = 128;
        const TCHAR* InvalidCatalog = TEXT("POS catalog data is invalid.");
        const TCHAR* InvalidReceipt = TEXT("POS receipt data is invalid.");

        bool Id(const FValue& Value, FString& Out)
        {
            return String(Value, Out) && !Out.IsEmpty() && Out.Len() <= MaxIdLength;
        }
        bool Text(const FValue& Value, FString& Out)
        {
            return String(Value, Out) && Out.Len() <= MaxTextLength;
        }
        bool Time(const FValue& Value, double& Out)
        {
            return Number(Value, Out) && ValidTime(Out);
        }
        bool Revision(const FValue& Value, int64& Out)
        {
            return Integer(Value, Out) && Out < MaxInteger;
        }
        bool ValidScope(const FPosScope& Scope)
        {
            return !Scope.GameRoomId.IsEmpty() && Scope.GameRoomId.Len() <= MaxIdLength
                && !Scope.InstanceId.IsEmpty() && Scope.InstanceId.Len() <= 160
                && !Scope.LifeId.IsEmpty() && Scope.LifeId.Len() <= 100
                && !Scope.ServiceId.IsEmpty() && Scope.ServiceId.Len() <= 100
                && Scope.Revision >= 0 && Scope.Revision < MaxInteger;
        }
        bool ReadScope(const FValue& Value, FPosScope& Out)
        {
            const FObject O = Object(Value);
            return Only(O, {TEXT("gameRoomId"), TEXT("instanceId"), TEXT("lifeId"), TEXT("revision"), TEXT("serviceId")})
                && Id(Field(O, TEXT("gameRoomId")), Out.GameRoomId)
                && Id(Field(O, TEXT("instanceId")), Out.InstanceId)
                && Id(Field(O, TEXT("lifeId")), Out.LifeId)
                && Id(Field(O, TEXT("serviceId")), Out.ServiceId)
                && Revision(Field(O, TEXT("revision")), Out.Revision) && ValidScope(Out);
        }
        bool ValueAmount(const FValue& Value, double& Out)
        {
            return Number(Value, Out) && Out >= 0 && Out <= MaxInteger;
        }
        bool ReadCatalog(const FObject& O, FPosCatalog& Out)
        {
            if (!Only(O, {TEXT("scope"), TEXT("outingId"), TEXT("outingRevision"), TEXT("availableCash"), TEXT("fee"),
                TEXT("amounts"), TEXT("eligibleBankedCredits"), TEXT("bankedSourceCoverage"), TEXT("cashConversion"),
                TEXT("deposit"), TEXT("serverNow")})
                || !ReadScope(Field(O, TEXT("scope")), Out.Scope)
                || !Id(Field(O, TEXT("outingId")), Out.OutingId)
                || !Revision(Field(O, TEXT("outingRevision")), Out.OutingRevision)
                || !ValueAmount(Field(O, TEXT("availableCash")), Out.AvailableCash)
                || !ValueAmount(Field(O, TEXT("fee")), Out.Fee)
                || !ValueAmount(Field(O, TEXT("eligibleBankedCredits")), Out.EligibleBankedCredits)
                || !Integer(Field(O, TEXT("serverNow")), Out.ServerNowMs, 1)
                || !EnumValue<EPosSourceCoverage>(Field(O, TEXT("bankedSourceCoverage")), Out.BankedSourceCoverage,
                    {{TEXT("complete"), EPosSourceCoverage::Complete}, {TEXT("bounded"), EPosSourceCoverage::Bounded}})
                || !EnumValue<EPosCashConversion>(Field(O, TEXT("cashConversion")), Out.CashConversion,
                    {{TEXT("disabled"), EPosCashConversion::Disabled}})
                || !EnumValue<EPosDeposit>(Field(O, TEXT("deposit")), Out.Deposit,
                    {{TEXT("own-home-only"), EPosDeposit::OwnHomeOnly}})) return false;
            return ReadBoundedArray(Field(O, TEXT("amounts")), Out.Amounts, MaxAmounts, ValueAmount);
        }
        bool ReadReceipt(const FObject& O, FPosReceipt& Out)
        {
            const FObject Source = Object(Field(O, TEXT("source")));
            if (!Only(O, {TEXT("id"), TEXT("requestId"), TEXT("scope"), TEXT("outingId"), TEXT("outingRevision"),
                TEXT("source"), TEXT("unitKey"), TEXT("amount"), TEXT("fee"), TEXT("total"), TEXT("status"),
                TEXT("issuedAt"), TEXT("expiresAt"), TEXT("confirmedAt"), TEXT("readyAt"), TEXT("settledAt"),
                TEXT("utilityEventId"), TEXT("reason"), TEXT("grantedEntryId")})
                || !Id(Field(O, TEXT("id")), Out.Id) || !Text(Field(O, TEXT("requestId")), Out.RequestId)
                || !ReadScope(Field(O, TEXT("scope")), Out.Scope) || !Id(Field(O, TEXT("outingId")), Out.OutingId)
                || !Revision(Field(O, TEXT("outingRevision")), Out.OutingRevision)
                || !Only(Source, {TEXT("kind"), TEXT("key")})
                || !EnumValue<EPosSourceKind>(Field(Source, TEXT("kind")), Out.Source.Kind,
                    {{TEXT("banked_game_value"), EPosSourceKind::BankedGameValue}})
                || !EnumValue<EPosUnit>(Field(Source, TEXT("key")), Out.Source.Key, {{TEXT("lagos-credit"), EPosUnit::LagosCredit}})
                || !EnumValue<EPosUnit>(Field(O, TEXT("unitKey")), Out.UnitKey, {{TEXT("lagos-credit"), EPosUnit::LagosCredit}})
                || !ValueAmount(Field(O, TEXT("amount")), Out.Amount) || !ValueAmount(Field(O, TEXT("fee")), Out.Fee)
                || !ValueAmount(Field(O, TEXT("total")), Out.Total)
                || !EnumValue<EPosStatus>(Field(O, TEXT("status")), Out.Status,
                    {{TEXT("quoted"), EPosStatus::Quoted}, {TEXT("pending"), EPosStatus::Pending},
                     {TEXT("completed"), EPosStatus::Completed}, {TEXT("cancelled"), EPosStatus::Cancelled},
                     {TEXT("interrupted"), EPosStatus::Interrupted}, {TEXT("expired"), EPosStatus::Expired}})
                || !Time(Field(O, TEXT("issuedAt")), Out.IssuedAtMs) || !Time(Field(O, TEXT("expiresAt")), Out.ExpiresAtMs)
                || !Nullable(Field(O, TEXT("confirmedAt")), Out.ConfirmedAtMs, Time)
                || !Nullable(Field(O, TEXT("readyAt")), Out.ReadyAtMs, Time)
                || !Nullable(Field(O, TEXT("settledAt")), Out.SettledAtMs, Time)
                || !Nullable(Field(O, TEXT("utilityEventId")), Out.UtilityEventId, Text)
                || !Nullable(Field(O, TEXT("reason")), Out.Reason, Text)
                || !Nullable(Field(O, TEXT("grantedEntryId")), Out.GrantedEntryId, Id)) return false;
            // Both read endpoints pass ownedPosQuote's current immutable-terms checks.
            // Do not invent status-specific null rules: release can terminate an unconfirmed quote.
            return (Out.Amount == 20 || Out.Amount == 50 || Out.Amount == 100)
                && Out.Fee == 1 && Out.Total == Out.Amount + Out.Fee && Out.ExpiresAtMs - Out.IssuedAtMs == 60000;
        }
        bool SameId(const TOptional<FString>& A, const TOptional<FString>& B)
        {
            return A.IsSet() == B.IsSet() && (!A.IsSet() || Same(A.GetValue(), B.GetValue()));
        }
        bool OpenStatus(EPosStatus Status)
        {
            return Status == EPosStatus::Quoted || Status == EPosStatus::Pending;
        }
        FString FailureText(const FPosPublicFailure& Failure, const TCHAR* Fallback)
        {
            return !Failure.Message.IsEmpty() && Failure.Message.Len() <= MaxTextLength ? Failure.Message : FString(Fallback);
        }
        bool RequestId(const FString& Value)
        {
            if (Value.Len() < 16 || Value.Len() > 128) return false;
            for (int32 Index = 0; Index < Value.Len(); ++Index)
            {
                const TCHAR C = Value[Index];
                const bool bAlphanumeric = (C >= TEXT('A') && C <= TEXT('Z'))
                    || (C >= TEXT('a') && C <= TEXT('z')) || (C >= TEXT('0') && C <= TEXT('9'));
                if (!bAlphanumeric && (Index == 0 || (C != TEXT('.') && C != TEXT('_') && C != TEXT(':') && C != TEXT('-')))) return false;
            }
            return true;
        }
        bool ValidCommand(const FPosCommand& Command)
        {
            if (Command.Kind == EPosCommandKind::Quote)
                return Command.Amount.IsSet() && std::isfinite(Command.Amount.GetValue())
                    && !Command.QuoteId.IsSet() && !Command.AcceptsCarriedLoss.IsSet();
            if (!Command.QuoteId.IsSet() || Command.QuoteId.GetValue().IsEmpty()
                || Command.QuoteId.GetValue().Len() > MaxIdLength || Command.Amount.IsSet()) return false;
            if (Command.Kind == EPosCommandKind::Confirm)
                return Command.AcceptsCarriedLoss.IsSet() && Command.AcceptsCarriedLoss.GetValue();
            return (Command.Kind == EPosCommandKind::Settle || Command.Kind == EPosCommandKind::Cancel)
                && !Command.AcceptsCarriedLoss.IsSet();
        }
        bool EncodeCommand(const FPosScope& Scope, const FPosCommand& Command, const FString& Id,
            FString& Operation, FString& Json)
        {
            if (!ValidScope(Scope) || !ValidCommand(Command) || !RequestId(Id)) return false;
            const auto Arguments = MakeShared<FJsonObject>();
            if (Command.Kind != EPosCommandKind::Cancel)
            {
                Arguments->SetStringField(TEXT("gameRoomId"), Scope.GameRoomId);
                Arguments->SetStringField(TEXT("instanceId"), Scope.InstanceId);
                Arguments->SetStringField(TEXT("lifeId"), Scope.LifeId);
                Arguments->SetNumberField(TEXT("revision"), static_cast<double>(Scope.Revision));
                Arguments->SetStringField(TEXT("serviceId"), Scope.ServiceId);
            }
            switch (Command.Kind)
            {
            case EPosCommandKind::Quote:
                Operation = TEXT("companionPos:requestQuote");
                Arguments->SetNumberField(TEXT("amount"), Command.Amount.GetValue());
                break;
            case EPosCommandKind::Confirm:
                Operation = TEXT("companionPos:confirm");
                Arguments->SetStringField(TEXT("quoteId"), Command.QuoteId.GetValue());
                Arguments->SetBoolField(TEXT("acceptsCarriedLoss"), Command.AcceptsCarriedLoss.GetValue());
                break;
            case EPosCommandKind::Settle:
                Operation = TEXT("companionPos:settle");
                Arguments->SetStringField(TEXT("quoteId"), Command.QuoteId.GetValue());
                break;
            case EPosCommandKind::Cancel:
                Operation = TEXT("companionPos:cancel");
                Arguments->SetStringField(TEXT("quoteId"), Command.QuoteId.GetValue());
                break;
            default: return false;
            }
            Arguments->SetStringField(TEXT("requestId"), Id);
            const auto Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Json);
            return FJsonSerializer::Serialize(Arguments.ToSharedRef(), Writer);
        }
    }

    bool SamePosScope(const FPosScope& A, const FPosScope& B)
    {
        using StateJsonPrivate::Same;
        return Same(A.GameRoomId, B.GameRoomId) && Same(A.InstanceId, B.InstanceId)
            && Same(A.LifeId, B.LifeId) && A.Revision == B.Revision && Same(A.ServiceId, B.ServiceId);
    }
    bool DecodePosCatalog(const FString& Json, TSharedPtr<const FPosCatalog>& Out, FString& OutError)
    {
        using namespace PosStatePrivate;
        // Copy bounded input before clearing outputs, including Json aliasing OutError or an old value.
        const FString Input = Json.Len() <= Limits.Characters ? Json : FString();
        Out.Reset(); OutError = InvalidCatalog;
        FObject Object;
        FPosCatalog Next;
        if (!Parse(Input, Object, Limits) || !ReadCatalog(Object, Next)) return false;
        Out = MakeShared<FPosCatalog>(MoveTemp(Next)); OutError.Reset();
        return true;
    }
    bool DecodePosReceipt(const FString& Json, TSharedPtr<const FPosReceipt>& Out, FString& OutError)
    {
        using namespace PosStatePrivate;
        const FString Input = Json.Len() <= Limits.Characters ? Json : FString();
        Out.Reset(); OutError = InvalidReceipt;
        FValue Null;
        if (ParseNull(Input, Null, Limits)) { OutError.Reset(); return true; }
        FObject Object;
        FPosReceipt Next;
        if (!Parse(Input, Object, Limits) || !ReadReceipt(Object, Next)) return false;
        Out = MakeShared<FPosReceipt>(MoveTemp(Next)); OutError.Reset();
        return true;
    }
    EPosReceiptReadKind FPosReceiptReadIntent::GetKind() const
    {
        return RecoveringId.IsSet() && !RecoveringId.GetValue().IsEmpty() ? EPosReceiptReadKind::ById : EPosReceiptReadKind::Latest;
    }
    FPosOwnerReceipt FPosReadState::BeginOwner(const FString& AccountId)
    {
        const bool bValidAccount = !AccountId.IsEmpty() && AccountId.Len() <= PosStatePrivate::MaxIdLength;
        Clear();
        FPosOwnerReceipt Out;
        if (!bDisposed && bValidAccount) { OwnerGeneration = FGuid::NewGuid(); Out.Generation = OwnerGeneration; }
        return Out;
    }
    void FPosReadState::CloseCatalog() { CatalogGeneration.Invalidate(); }
    void FPosReadState::CloseReceipt() { ReceiptGeneration.Invalidate(); }
    void FPosReadState::Clear()
    {
        CloseCatalog(); CloseReceipt(); OwnerGeneration.Invalidate();
        Pending.Reset(); bPendingUncertain = false; ClearAttempt();
        Selected.Reset(); RecoveringId.Reset(); View = FPosReadView(); bObserved = false;
    }
    void FPosReadState::Dispose() { Clear(); bDisposed = true; }
    bool FPosReadState::CurrentOwner(const FPosOwnerReceipt& Owner) const
    {
        return !bDisposed && OwnerGeneration.IsValid() && Owner.Generation == OwnerGeneration;
    }
    void FPosReadState::SetObserved(const FPosOwnerReceipt& Owner, bool bNextObserved)
    {
        if (!CurrentOwner(Owner) || bObserved == bNextObserved) return;
        CloseCatalog(); CloseReceipt(); bObserved = bNextObserved;
    }
    bool FPosReadState::Select(const FPosOwnerReceipt& Owner, const TOptional<FPosScope>& Scope)
    {
        if (!CurrentOwner(Owner) || (Scope.IsSet() && !PosStatePrivate::ValidScope(Scope.GetValue()))) return false;
        if (Scope.IsSet() == Selected.IsSet() && (!Scope.IsSet() || SamePosScope(Scope.GetValue(), Selected.GetValue()))) return true;
        Selected = Scope;
        CloseCatalog();
        View.Catalog.Reset(); View.bReady = false;
        View.bBusy = ActiveAttempt.IsValid(); View.bRetryable = HasUncertainCommand();
        if (!HasUncertainCommand()) View.Error.Reset();
        // Receipt, its presence, recovery ID and receipt subscription survive service/visit changes.
        return true;
    }
    bool FPosReadState::Recover(const FPosOwnerReceipt& Owner, const FString& QuoteId)
    {
        if (!CurrentOwner(Owner) || QuoteId.Len() > PosStatePrivate::MaxIdLength) return false;
        if (RecoveringId.IsSet() && StateJsonPrivate::Same(RecoveringId.GetValue(), QuoteId)) return true;
        RecoveringId = QuoteId; CloseReceipt();
        return true;
    }
    void FPosReadState::Reload(const FPosOwnerReceipt& Owner)
    {
        if (CurrentOwner(Owner)) { CloseCatalog(); CloseReceipt(); }
    }
    bool FPosReadState::BeginCatalogRead(const FPosOwnerReceipt& Owner, double ServerNowMs, FPosCatalogReadIntent& Out)
    {
        Out = FPosCatalogReadIntent();
        if (!CurrentOwner(Owner)) return false;
        CloseCatalog();
        if (!bObserved || !Selected.IsSet()) return false;
        const double Now = std::floor(ServerNowMs);
        if (!std::isfinite(Now) || Now < 1 || Now > StateJsonPrivate::MaxInteger)
        {
            // Canonical invalid time marks catalog ready but retains the preceding catalog.
            View.bReady = true; View.Error = TEXT("Refresh the game timestamp.");
            return false;
        }
        CatalogGeneration = FGuid::NewGuid();
        Out.OwnerGeneration = OwnerGeneration; Out.ReadGeneration = CatalogGeneration;
        Out.Scope = Selected.GetValue(); Out.NowMs = static_cast<int64>(Now);
        return true;
    }
    bool FPosReadState::BeginReceiptRead(const FPosOwnerReceipt& Owner, FPosReceiptReadIntent& Out)
    {
        Out = FPosReceiptReadIntent();
        if (!CurrentOwner(Owner)) return false;
        CloseReceipt();
        if (!bObserved) return false;
        ReceiptGeneration = FGuid::NewGuid();
        Out.OwnerGeneration = OwnerGeneration; Out.ReadGeneration = ReceiptGeneration; Out.RecoveringId = RecoveringId;
        return true;
    }
    bool FPosReadState::Current(const FPosCatalogReadIntent& Intent) const
    {
        return !bDisposed && bObserved && OwnerGeneration.IsValid() && Intent.OwnerGeneration == OwnerGeneration
            && CatalogGeneration.IsValid() && Intent.ReadGeneration == CatalogGeneration
            && Selected.IsSet() && SamePosScope(Intent.Scope, Selected.GetValue());
    }
    bool FPosReadState::Current(const FPosReceiptReadIntent& Intent) const
    {
        return !bDisposed && bObserved && OwnerGeneration.IsValid() && Intent.OwnerGeneration == OwnerGeneration
            && ReceiptGeneration.IsValid() && Intent.ReadGeneration == ReceiptGeneration
            && PosStatePrivate::SameId(Intent.RecoveringId, RecoveringId);
    }
    EPosReadResult FPosReadState::ObserveCatalog(const FPosCatalogReadIntent& Intent, const FString& Json)
    {
        if (!Current(Intent)) return EPosReadResult::StaleRead;
        TSharedPtr<const FPosCatalog> Next;
        FString Error;
        if (!DecodePosCatalog(Json, Next, Error))
        {
            View.Catalog.Reset(); View.bReady = true; View.Error = MoveTemp(Error);
            return EPosReadResult::InvalidResponse;
        }
        if (!SamePosScope(Next->Scope, Intent.Scope)) return EPosReadResult::IdentityMismatch;
        View.Catalog = MoveTemp(Next); View.bReady = true;
        if (!HasUncertainCommand()) View.Error.Reset();
        return EPosReadResult::Accepted;
    }
    EPosReadResult FPosReadState::AcceptReceipt(TSharedPtr<const FPosReceipt> Receipt)
    {
        using namespace PosStatePrivate;
        const auto& CurrentReceipt = View.Receipt;
        if (CurrentReceipt.IsValid())
        {
            if (!Receipt.IsValid()) return EPosReadResult::Retained;
            const bool bSameReceipt = Same(Receipt->Id, CurrentReceipt->Id);
            const bool bRecoveringThis = RecoveringId.IsSet() && Same(RecoveringId.GetValue(), Receipt->Id);
            if (!bSameReceipt && Receipt->IssuedAtMs < CurrentReceipt->IssuedAtMs && !bRecoveringThis)
                return EPosReadResult::Retained;
            if (bSameReceipt && ((CurrentReceipt->Status == EPosStatus::Pending && Receipt->Status == EPosStatus::Quoted)
                || (!OpenStatus(CurrentReceipt->Status) && Receipt->Status != CurrentReceipt->Status)))
                return EPosReadResult::Retained;
        }
        View.Receipt = MoveTemp(Receipt);
        View.ReceiptPresence = View.Receipt.IsValid() ? EPosReceiptPresence::Value : EPosReceiptPresence::Null;
        return EPosReadResult::Accepted;
    }
    EPosReadResult FPosReadState::ObserveReceipt(const FPosReceiptReadIntent& Intent, const FString& Json)
    {
        if (!Current(Intent)) return EPosReadResult::StaleRead;
        TSharedPtr<const FPosReceipt> Next;
        FString Error;
        if (!DecodePosReceipt(Json, Next, Error))
        {
            View.Error = MoveTemp(Error);
            return EPosReadResult::InvalidResponse;
        }
        // By-ID null is ignored, just like capturedId && receipt?.id !== capturedId.
        if (Intent.GetKind() == EPosReceiptReadKind::ById
            && (!Next.IsValid() || !StateJsonPrivate::Same(Next->Id, Intent.RecoveringId.GetValue())))
            return EPosReadResult::IdentityMismatch;
        // Account-owned receipt is deliberately not filtered by selected catalog scope.
        // A read alone does not end recovery or clear errors. Only known pending evidence does.
        const EPosReadResult Result = AcceptReceipt(MoveTemp(Next));
        ReconcileReceipt();
        return Result;
    }
    bool FPosReadState::ObserveCatalogFailure(const FPosCatalogReadIntent& Intent, const FPosPublicFailure& Failure)
    {
        if (!Current(Intent)) return false;
        const FString Message = PosStatePrivate::FailureText(Failure, TEXT("Unable to load POS catalog."));
        View.Catalog.Reset(); View.bReady = true; View.Error = Message;
        return true;
    }
    bool FPosReadState::ObserveReceiptFailure(const FPosReceiptReadIntent& Intent, const FPosPublicFailure& Failure)
    {
        if (!Current(Intent)) return false;
        View.Error = PosStatePrivate::FailureText(Failure, TEXT("Unable to load POS receipt."));
        return true;
    }

    bool FPosReadState::CurrentAttempt(const FPosCommandIntent& Intent) const
    {
        return !bDisposed && OwnerGeneration.IsValid() && Intent.OwnerGeneration == OwnerGeneration
            && ActiveAttempt.IsValid() && Intent.AttemptId == ActiveAttempt
            && ActivePacket.IsValid() && Intent.PacketId == ActivePacket;
    }
    bool FPosReadState::InitialScopeMatches() const
    {
        return Pending.IsValid() && (bPendingUncertain || Pending->Command.Kind == EPosCommandKind::Cancel
            || (Selected.IsSet() && SamePosScope(Pending->Scope, Selected.GetValue())));
    }
    void FPosReadState::PrepareAttempt(FPosCommandIntent& Out)
    {
        ActiveAttempt = FGuid::NewGuid(); ActivePacket = Pending->Id;
        AttemptPhase = EAttemptPhase::Prepared; ReturnedReceipt.Reset();
        View.bBusy = true; View.bRetryable = false; View.Error.Reset();
        Out.OwnerGeneration = OwnerGeneration; Out.PacketId = ActivePacket; Out.AttemptId = ActiveAttempt;
    }
    void FPosReadState::ClearAttempt()
    {
        ActiveAttempt.Invalidate(); ActivePacket.Invalidate();
        AttemptPhase = EAttemptPhase::None; ReturnedReceipt.Reset();
    }
    bool FPosReadState::PrepareCommand(const FPosOwnerReceipt& Owner, const FPosCommand& Command,
        const FString& RequestId, FPosCommandIntent& Out)
    {
        Out = FPosCommandIntent();
        if (!CurrentOwner(Owner) || ActiveAttempt.IsValid() || Pending.IsValid()
            || !PosStatePrivate::ValidCommand(Command) || !PosStatePrivate::RequestId(RequestId)) return false;
        TOptional<FPosScope> Scope = Selected;
        if (Command.Kind == EPosCommandKind::Cancel && View.Receipt.IsValid()
            && StateJsonPrivate::Same(View.Receipt->Id, Command.QuoteId.GetValue())) Scope = View.Receipt->Scope;
        if (!Scope.IsSet()) return false;
        FPendingPacket Next;
        Next.Id = FGuid::NewGuid(); Next.Scope = Scope.GetValue(); Next.Command = Command; Next.RequestId = RequestId;
        if (!PosStatePrivate::EncodeCommand(Next.Scope, Next.Command, Next.RequestId, Next.Operation, Next.ArgumentsJson)) return false;
        Pending = MakeShared<FPendingPacket>(MoveTemp(Next)); bPendingUncertain = false;
        PrepareAttempt(Out);
        return true;
    }
    bool FPosReadState::PrepareRetry(const FPosOwnerReceipt& Owner, FPosCommandIntent& Out)
    {
        Out = FPosCommandIntent();
        if (!CurrentOwner(Owner) || ActiveAttempt.IsValid() || !HasUncertainCommand()) return false;
        PrepareAttempt(Out);
        return true;
    }
    bool FPosReadState::BeginCommandPreDispatch(const FPosCommandIntent& Intent, FPosReadRestarts& Out)
    {
        Out = FPosReadRestarts();
        if (!CurrentAttempt(Intent) || AttemptPhase != EAttemptPhase::Prepared || !Pending.IsValid()
            || Pending->Id != Intent.PacketId || !InitialScopeMatches()) return false;
        AttemptPhase = EAttemptPhase::ReadyToDispatch;
        // Canonical receipt reconnect precedes the request, including synchronous callbacks.
        // Empty recovery stays empty. This stage does not claim that a request was sent.
        if (Pending->Command.Kind == EPosCommandKind::Quote && RecoveringId.IsSet() && !RecoveringId.GetValue().IsEmpty())
        {
            RecoveringId.Reset(); CloseReceipt(); Out.bReceipt = true;
            AttemptPhase = EAttemptPhase::AwaitingReceiptRestart;
        }
        return true;
    }
    bool FPosReadState::CompleteCommandReceiptRestart(const FPosCommandIntent& Intent)
    {
        if (!CurrentAttempt(Intent) || AttemptPhase != EAttemptPhase::AwaitingReceiptRestart
            || !Pending.IsValid() || Pending->Id != Intent.PacketId) return false;
        AttemptPhase = EAttemptPhase::ReadyToDispatch;
        return true;
    }
    bool FPosReadState::MarkCommandDispatched(const FPosCommandIntent& Intent, TSharedPtr<const FPosCommandDispatch>& Out)
    {
        Out.Reset();
        if (!CurrentAttempt(Intent) || AttemptPhase != EAttemptPhase::ReadyToDispatch || !Pending.IsValid()
            || Pending->Id != Intent.PacketId || !InitialScopeMatches()) return false;
        FPosCommandDispatch Next;
        Next.Operation = Pending->Operation; Next.ArgumentsJson = Pending->ArgumentsJson;
        bPendingUncertain = true; AttemptPhase = EAttemptPhase::Dispatched;
        Out = MakeShared<FPosCommandDispatch>(MoveTemp(Next));
        return true;
    }
    bool FPosReadState::AbandonUnsentCommand(const FPosCommandIntent& Intent)
    {
        if (!CurrentAttempt(Intent) || (AttemptPhase != EAttemptPhase::Prepared
            && AttemptPhase != EAttemptPhase::AwaitingReceiptRestart && AttemptPhase != EAttemptPhase::ReadyToDispatch)
            || !Pending.IsValid() || Pending->Id != Intent.PacketId) return false;
        if (!bPendingUncertain) Pending.Reset();
        ClearAttempt(); View.bBusy = false; View.bRetryable = HasUncertainCommand();
        // An unsent retry keeps its earlier uncertainty unless retained read evidence resolves it.
        ReconcileReceipt();
        return true;
    }
    void FPosReadState::RecordCommandFailure(bool bUncertain, FString Message)
    {
        if (!bUncertain) { Pending.Reset(); bPendingUncertain = false; }
        ReturnedReceipt.Reset(); AttemptPhase = EAttemptPhase::AwaitingFinally;
        View.bRetryable = bUncertain; View.Error = MoveTemp(Message);
    }
    EPosCommandResult FPosReadState::AcceptCommandResult(const FPosCommandIntent& Intent, const FString& Json, FPosReadRestarts& Out)
    {
        Out = FPosReadRestarts();
        if (!CurrentAttempt(Intent) || AttemptPhase != EAttemptPhase::Dispatched
            || !Pending.IsValid() || Pending->Id != Intent.PacketId) return EPosCommandResult::StaleAttempt;
        TSharedPtr<const FPosReceipt> Next;
        FString Error;
        if (!DecodePosReceipt(Json, Next, Error) || !Next.IsValid())
        {
            // A sent request can have taken effect despite an unusable reply, including null.
            RecordCommandFailure(true, TEXT("The POS response is invalid. Retry the same action or reload the receipt."));
            return EPosCommandResult::InvalidResponse;
        }
        Pending.Reset(); bPendingUncertain = false;
        AcceptReceipt(Next);
        if (RecoveringId.IsSet() && !RecoveringId.GetValue().IsEmpty())
        {
            RecoveringId.Reset(); CloseReceipt(); Out.bReceipt = true;
        }
        CloseCatalog(); Out.bCatalog = true;
        ReturnedReceipt = MoveTemp(Next); AttemptPhase = EAttemptPhase::AwaitingFinally;
        return EPosCommandResult::Accepted;
    }
    bool FPosReadState::AcceptCommandFailure(const FPosCommandIntent& Intent, const FPosCommandFailure& Failure)
    {
        if (!CurrentAttempt(Intent) || AttemptPhase != EAttemptPhase::Dispatched
            || !Pending.IsValid() || Pending->Id != Intent.PacketId) return false;
        bool bUncertain;
        switch (Failure.Kind)
        {
        case EPosCommandFailureKind::NetworkUnavailable:
        case EPosCommandFailureKind::GenericSaveFailure: bUncertain = true; break;
        case EPosCommandFailureKind::Definite: bUncertain = false; break;
        default: return false;
        }
        RecordCommandFailure(bUncertain, PosStatePrivate::FailureText(Failure.PublicError,
            TEXT("The POS result is unknown. Retry the same action.")));
        return true;
    }
    bool FPosReadState::FinishCommand(const FPosCommandIntent& Intent, TSharedPtr<const FPosReceipt>& Out)
    {
        Out.Reset();
        if (!CurrentAttempt(Intent) || AttemptPhase != EAttemptPhase::AwaitingFinally) return false;
        // JS evaluates the successful return after its outcome emit, before entering finally.
        if (ReturnedReceipt.IsValid()) Out = View.Receipt.IsValid()
            && StateJsonPrivate::Same(View.Receipt->Id, ReturnedReceipt->Id) ? View.Receipt : ReturnedReceipt;
        ClearAttempt(); View.bBusy = false; View.bRetryable = HasUncertainCommand();
        ReconcileReceipt();
        return true;
    }
    void FPosReadState::ReconcileReceipt()
    {
        if (!HasUncertainCommand() || ActiveAttempt.IsValid() || !View.Receipt.IsValid()) return;
        const FPosCommand& Command = Pending->Command;
        const FPosReceipt& Receipt = *View.Receipt;
        const bool bKnown = Command.Kind == EPosCommandKind::Quote
            ? StateJsonPrivate::Same(Receipt.RequestId, Pending->RequestId) && SamePosScope(Receipt.Scope, Pending->Scope)
                && Receipt.Amount == Command.Amount.GetValue()
            : StateJsonPrivate::Same(Receipt.Id, Command.QuoteId.GetValue()) && (Command.Kind == EPosCommandKind::Confirm
                ? Receipt.Status != EPosStatus::Quoted : !PosStatePrivate::OpenStatus(Receipt.Status));
        if (!bKnown) return;
        Pending.Reset(); bPendingUncertain = false; View.bRetryable = false; View.Error.Reset();
    }
}
