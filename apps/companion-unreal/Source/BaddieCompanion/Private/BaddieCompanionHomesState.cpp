#include "BaddieCompanionHomesState.h"
#include "BaddieCompanionStateJson.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

#include <cmath>

namespace BaddieCompanion
{
    namespace HomesStatePrivate
    {
        using StateJsonPrivate::FObject;
        using StateJsonPrivate::FValue;
        using StateJsonPrivate::Field;
        using StateJsonPrivate::Same;
        constexpr StateJsonPrivate::FParseLimits ListLimits{512 * 1024, 4, 1024};
        constexpr StateJsonPrivate::FParseLimits RoomLimits{256 * 1024, 6, 256};
        constexpr StateJsonPrivate::FParseLimits NullLimits{4096, 2, 8};
        bool Bounded(const FString& Value, int32 Maximum, bool bAllowEmpty = false)
        {
            if ((!bAllowEmpty && Value.IsEmpty()) || Value.Len() > Maximum) return false;
            int32 Units = 0;
            for (const TCHAR C : Value)
            {
                const uint32 Unit = sizeof(TCHAR) == 2 ? static_cast<uint16>(C) : static_cast<uint32>(C);
                Units += Unit > 0xFFFF ? 2 : 1;
                if (Units > Maximum) return false;
            }
            return true;
        }
        bool HasText(const FString& Value)
        {
            for (const TCHAR C : Value)
            {
                const uint32 Unit = sizeof(TCHAR) == 2 ? static_cast<uint16>(C) : static_cast<uint32>(C);
                if (!((Unit >= 0x9 && Unit <= 0xD) || Unit == 0x20 || Unit == 0xA0 || Unit == 0x1680
                    || (Unit >= 0x2000 && Unit <= 0x200A) || Unit == 0x2028 || Unit == 0x2029 || Unit == 0x202F
                    || Unit == 0x205F || Unit == 0x3000 || Unit == 0xFEFF)) return true;
            }
            return false;
        }
        bool Text(const FValue& Value, FString& Out, int32 Maximum, bool bAllowEmpty = false)
        {
            return StateJsonPrivate::String(Value, Out) && Bounded(Out, Maximum, bAllowEmpty);
        }
        bool Choice(const FValue& Value, std::initializer_list<TPair<const TCHAR*, int32>> Choices)
        {
            int32 Ignored = 0;
            return StateJsonPrivate::EnumValue(Value, Ignored, Choices);
        }
        bool Encode(const TSharedRef<FJsonObject>& Object, FString& Out)
        {
            return FJsonSerializer::Serialize(Object,
                TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Out));
        }
        bool Invitation(const FValue& Value, FHomeInvitation& Out)
        {
            const auto O = StateJsonPrivate::Object(Value);
            return StateJsonPrivate::Only(O, {TEXT("id"), TEXT("handle"), TEXT("roomId"), TEXT("roomName")})
                && Text(Field(O, TEXT("id")), Out.Id, 256)
                && Text(Field(O, TEXT("handle")), Out.Handle, 80, true)
                && Text(Field(O, TEXT("roomId")), Out.RoomId, 80)
                && Text(Field(O, TEXT("roomName")), Out.RoomName, 32, true);
        }
        bool Color(const FValue& Value)
        {
            if (!Value.IsValid()) return true;
            FString TextValue;
            if (!StateJsonPrivate::String(Value, TextValue) || TextValue.Len() != 7 || TextValue[0] != TEXT('#')) return false;
            for (int32 Index = 1; Index < 7; ++Index)
            {
                const TCHAR C = TextValue[Index];
                if (!((C >= TEXT('0') && C <= TEXT('9')) || (C >= TEXT('a') && C <= TEXT('f'))
                    || (C >= TEXT('A') && C <= TEXT('F')))) return false;
            }
            return true;
        }
        bool Item(const FValue& Value, FString& OutId)
        {
            const auto O = StateJsonPrivate::Object(Value);
            double X = 0, Z = 0, Rotation = 0;
            if (!StateJsonPrivate::Only(O, {TEXT("id"), TEXT("kind"), TEXT("x"), TEXT("z"), TEXT("rotation"),
                    TEXT("y"), TEXT("surface"), TEXT("lit")})
                || !Text(Field(O, TEXT("id")), OutId, 80) || !HasText(OutId)
                || !Choice(Field(O, TEXT("kind")), {
                    {TEXT("lamp"), 0}, {TEXT("stool"), 1}, {TEXT("plant"), 2},
                    {TEXT("botanical-painting"), 3}, {TEXT("abstract-painting"), 4}, {TEXT("landscape-painting"), 5},
                    {TEXT("wall-clock"), 6}, {TEXT("wall-mirror"), 7}, {TEXT("standing-fan"), 8}, {TEXT("tower-fan"), 9},
                    {TEXT("desk-fan"), 10}, {TEXT("floor-lamp"), 11}, {TEXT("desk-lamp"), 12}, {TEXT("armchair"), 13},
                    {TEXT("loveseat"), 14}, {TEXT("ottoman"), 15}, {TEXT("coffee-table"), 16}, {TEXT("side-table"), 17},
                    {TEXT("bookcase"), 18}, {TEXT("storage-basket"), 19}, {TEXT("book-stack"), 20}, {TEXT("candle"), 21}, {TEXT("succulent"), 22}})
                || !StateJsonPrivate::Number(Field(O, TEXT("x")), X) || std::abs(X) > 4
                || !StateJsonPrivate::Number(Field(O, TEXT("z")), Z) || std::abs(Z) > 4
                || !StateJsonPrivate::Number(Field(O, TEXT("rotation")), Rotation) || std::abs(Rotation) > 10000) return false;
            const auto Y = Field(O, TEXT("y"));
            double Height = 0;
            if (Y.IsValid() && (!StateJsonPrivate::Number(Y, Height) || Height < 0 || Height > 4)) return false;
            const auto Surface = Field(O, TEXT("surface"));
            if (Surface.IsValid() && !Choice(Surface, {{TEXT("floor"), 0}, {TEXT("console"), 1}, {TEXT("shelf"), 2},
                {TEXT("wall-back"), 3}, {TEXT("wall-left"), 4}, {TEXT("wall-right"), 5}, {TEXT("wall-front"), 6}})) return false;
            const auto Lit = Field(O, TEXT("lit"));
            bool bLit = false;
            return !Lit.IsValid() || StateJsonPrivate::Boolean(Lit, bLit);
        }
        bool Room(const FValue& Value, FRoomSelection& Out)
        {
            const auto O = StateJsonPrivate::Object(Value);
            if (!StateJsonPrivate::Only(O, {TEXT("id"), TEXT("name"), TEXT("environmentId"), TEXT("layoutId"),
                TEXT("homeDesign"), TEXT("palette"), TEXT("light"), TEXT("sharing"), TEXT("wallColor"), TEXT("floorColor"),
                TEXT("upholsteryColor"), TEXT("rugColor"), TEXT("items")})
                || !Text(Field(O, TEXT("id")), Out.Id, 80) || !HasText(Out.Id)
                || !Text(Field(O, TEXT("name")), Out.Name, 32) || !HasText(Out.Name)
                || !Text(Field(O, TEXT("environmentId")), Out.EnvironmentId, 16)
                || !Choice(Field(O, TEXT("environmentId")), {{TEXT("studio"), 0}, {TEXT("terrace"), 1}})
                || !Choice(Field(O, TEXT("palette")), {{TEXT("rose"), 0}, {TEXT("sage"), 1}, {TEXT("midnight"), 2}})
                || !Choice(Field(O, TEXT("light")), {{TEXT("daylight"), 0}, {TEXT("golden"), 1}, {TEXT("evening"), 2}})
                || !Choice(Field(O, TEXT("sharing")), {{TEXT("private"), 0}, {TEXT("host"), 1}, {TEXT("collaborative"), 2}})) return false;
            const auto Layout = Field(O, TEXT("layoutId"));
            if (Layout.IsValid() && (!Same(Out.EnvironmentId, TEXT("studio"))
                || !Text(Layout, Out.LayoutId, 32) || !Choice(Layout, {{TEXT("pat-studio-v01"), 0}, {TEXT("pink-bedroom-v01"), 1},
                    {TEXT("pink-bedroom-v02"), 2}, {TEXT("modern-kitchen-v01"), 3}, {TEXT("bathroom-v01"), 4}}))) return false;
            const auto DesignValue = Field(O, TEXT("homeDesign"));
            if (DesignValue.IsValid())
            {
                const auto Design = StateJsonPrivate::Object(DesignValue);
                int64 Version = 0, Turns = 0;
                double Extension = 0;
                if (!Layout.IsValid() || !Same(Out.EnvironmentId, TEXT("studio"))
                    || !StateJsonPrivate::Only(Design, {TEXT("version"), TEXT("turns"), TEXT("extension")})
                    || !StateJsonPrivate::Integer(Field(Design, TEXT("version")), Version) || Version != 1
                    || !StateJsonPrivate::Integer(Field(Design, TEXT("turns")), Turns) || Turns > 3
                    || !StateJsonPrivate::Number(Field(Design, TEXT("extension")), Extension)
                    || !(Extension == 0 || Extension == .6 || Extension == 1.2)
                    || (Extension != 0 && !Same(Out.LayoutId, TEXT("modern-kitchen-v01")) && !Same(Out.LayoutId, TEXT("bathroom-v01")))) return false;
            }
            if (!Color(Field(O, TEXT("wallColor"))) || !Color(Field(O, TEXT("floorColor")))
                || !Color(Field(O, TEXT("upholsteryColor"))) || !Color(Field(O, TEXT("rugColor")))) return false;
            TArray<FString> ItemIds;
            if (!StateJsonPrivate::ReadBoundedArray(Field(O, TEXT("items")), ItemIds, 8, Item)) return false;
            for (int32 Index = 0; Index < ItemIds.Num(); ++Index)
                for (int32 Prior = 0; Prior < Index; ++Prior) if (Same(ItemIds[Index], ItemIds[Prior])) return false;
            // Serialize the complete validated source object, not a default or a typed subset.
            // No coordinate migration, layout solver, entitlement filtering or mapping runs here.
            return Encode(O.ToSharedRef(), Out.RoomJson);
        }
    }

    bool DecodeHomeInvitations(const FString& Json, TSharedPtr<const FHomeInvitations>& Out)
    {
        using namespace HomesStatePrivate;
        const FString Source = Json.Len() <= ListLimits.Characters ? Json : FString();
        Out.Reset();
        FObject O;
        if (!StateJsonPrivate::Parse(Source, O, ListLimits)
            || !StateJsonPrivate::Only(O, {TEXT("received"), TEXT("sent")})) return false;
        const auto Next = MakeShared<FHomeInvitations>();
        if (!StateJsonPrivate::ReadBoundedArray(Field(O, TEXT("received")), Next->Received, 50, Invitation)
            || !StateJsonPrivate::ReadBoundedArray(Field(O, TEXT("sent")), Next->Sent, 50, Invitation)) return false;
        Out = Next; return true;
    }
    bool DecodeHomeVisit(const FString& Json, TSharedPtr<const FHomeVisit>& Out)
    {
        using namespace HomesStatePrivate;
        const FString Source = Json.Len() <= RoomLimits.Characters ? Json : FString();
        Out.Reset();
        FObject O;
        if (!StateJsonPrivate::Parse(Source, O, RoomLimits)
            || !StateJsonPrivate::Only(O, {TEXT("room"), TEXT("revision"), TEXT("ownerHandle")})) return false;
        const auto Next = MakeShared<FHomeVisit>();
        if (!Room(Field(O, TEXT("room")), Next->Room)
            || !StateJsonPrivate::Integer(Field(O, TEXT("revision")), Next->Revision)
            || !Text(Field(O, TEXT("ownerHandle")), Next->OwnerHandle, 80)) return false;
        Out = Next; return true;
    }
    const TCHAR* HomesErrorText(EHomesError Error)
    {
        switch (Error)
        {
        case EHomesError::ListFailed: return TEXT("Couldn't load Home invitations.");
        case EHomesError::VisitFailed: return TEXT("This Home visit is unavailable.");
        case EHomesError::InvitationFailed: return TEXT("Couldn't save the invitation.");
        case EHomesError::RevokeFailed: return TEXT("Couldn't remove the invitation.");
        default: return TEXT("The Home response is unavailable.");
        }
    }
    bool FHomesState::CurrentOwner(const FHomesOwnerReceipt& Owner) const
    {
        return !bDisposed && Owner.Generation.IsValid() && Owner.Generation == OwnerGeneration;
    }
    bool FHomesState::CurrentSubscription(const FHomesSubscription& Subscription) const
    {
        if (bDisposed || !OwnerGeneration.IsValid() || Subscription.Owner != OwnerGeneration || !Subscription.Generation.IsValid()) return false;
        return Subscription.Kind == EHomesReadKind::List ? Subscription.Generation == ListGeneration : Subscription.Generation == VisitGeneration;
    }
    bool FHomesState::CurrentRead(const FHomesIntent& Intent) const
    {
        if (bDisposed || !OwnerGeneration.IsValid() || Intent.Owner != OwnerGeneration || !Intent.Request.IsValid()
            || !Intent.Subscription.IsValid()) return false;
        if (Intent.Kind == EHomesOperation::Invitations) return Intent.Subscription == ListGeneration && Intent.Request == ListRead;
        if (Intent.Kind == EHomesOperation::Room) return Intent.Subscription == VisitGeneration && Intent.Request == VisitRead;
        return false;
    }
    bool FHomesState::CurrentCommand(const FHomesIntent& Intent) const
    {
        return !bDisposed && ActiveCommand.IsSet() && Intent.Owner == OwnerGeneration && Intent.Request.IsValid()
            && Intent.Request == ActiveCommand.GetValue().Request && Intent.Kind == ActiveCommand.GetValue().Kind;
    }
    void FHomesState::StopList()
    {
        ListGeneration.Invalidate(); ListRead.Invalidate(); View.Invitations.Reset(); View.bLoadingList = false; View.ListError.Reset();
    }
    void FHomesState::StopVisit()
    {
        VisitGeneration.Invalidate(); VisitRead.Invalidate(); SelectedInvitation.Reset();
        View.Visiting.Reset(); View.bLoadingVisit = false; View.VisitError.Reset();
    }
    void FHomesState::FinishCommand()
    {
        ActiveCommand.Reset(); View.bBusy = false;
    }
    FHomesOwnerReceipt FHomesState::BeginOwner(const FString& UserId)
    {
        const bool bValid = HomesStatePrivate::Bounded(UserId, 256);
        StopList(); StopVisit(); FinishCommand(); View = {}; OwnerGeneration.Invalidate(); bDisposed = false;
        if (!bValid) return {};
        OwnerGeneration = FGuid::NewGuid();
        FHomesOwnerReceipt Owner; Owner.Generation = OwnerGeneration; return Owner;
    }
    FHomesSubscription FHomesState::BeginList(const FHomesOwnerReceipt& Owner)
    {
        if (!CurrentOwner(Owner)) return {};
        StopList(); ListGeneration = FGuid::NewGuid(); View.bLoadingList = true;
        FHomesSubscription Result; Result.Owner = OwnerGeneration; Result.Generation = ListGeneration; return Result;
    }
    FHomesSubscription FHomesState::BeginVisit(const FHomesOwnerReceipt& Owner, const FString& InvitationId)
    {
        if (!CurrentOwner(Owner)) return {};
        const FString Next = HomesStatePrivate::Bounded(InvitationId, 256) ? InvitationId : FString();
        StopVisit();
        if (Next.IsEmpty()) return {};
        SelectedInvitation = Next; VisitGeneration = FGuid::NewGuid(); View.bLoadingVisit = true;
        FHomesSubscription Result; Result.Owner = OwnerGeneration; Result.Generation = VisitGeneration; Result.Kind = EHomesReadKind::Visit;
        return Result;
    }
    void FHomesState::LeaveVisit(const FHomesOwnerReceipt& Owner)
    {
        if (CurrentOwner(Owner)) StopVisit();
    }
    void FHomesState::Stop(const FHomesSubscription& Subscription)
    {
        if (!CurrentSubscription(Subscription)) return;
        if (Subscription.Kind == EHomesReadKind::List) StopList(); else StopVisit();
    }
    void FHomesState::StopObserving(const FHomesOwnerReceipt& Owner)
    {
        if (!CurrentOwner(Owner)) return;
        StopList(); StopVisit();
    }
    bool FHomesState::Read(const FHomesSubscription& Subscription, FHomesIntent& Out)
    {
        Out = {};
        if (!CurrentSubscription(Subscription)) return false;
        FHomesIntent Next;
        const auto Args = MakeShared<FJsonObject>();
        if (Subscription.Kind == EHomesReadKind::List)
        {
            Next.Kind = EHomesOperation::Invitations; Next.Operation = TEXT("companionHomes:invitations");
        }
        else
        {
            Next.Kind = EHomesOperation::Room; Next.Operation = TEXT("companionHomes:room");
            Args->SetStringField(TEXT("invitationId"), SelectedInvitation);
        }
        if (!HomesStatePrivate::Encode(Args, Next.ArgumentsJson)) return false;
        Next.Owner = OwnerGeneration; Next.Subscription = Subscription.Generation; Next.Request = FGuid::NewGuid();
        if (Next.Kind == EHomesOperation::Invitations) ListRead = Next.Request; else VisitRead = Next.Request;
        Out = MoveTemp(Next); return true;
    }
    EHomesCompletion FHomesState::AcceptRead(const FHomesIntent& Intent, const FString& Json)
    {
        if (!CurrentRead(Intent)) return EHomesCompletion::Stale;
        if (Intent.Kind == EHomesOperation::Invitations)
        {
            TSharedPtr<const FHomeInvitations> Next;
            const bool bValid = DecodeHomeInvitations(Json, Next);
            ListRead.Invalidate(); View.Invitations = MoveTemp(Next); View.bLoadingList = false;
            if (!bValid) View.ListError = EHomesError::InvalidResponse; else View.ListError.Reset();
            return bValid ? EHomesCompletion::Accepted : EHomesCompletion::InvalidResponse;
        }
        TSharedPtr<const FHomeVisit> Next;
        const bool bValid = DecodeHomeVisit(Json, Next);
        VisitRead.Invalidate(); View.Visiting = MoveTemp(Next); View.bLoadingVisit = false;
        if (!bValid) View.VisitError = EHomesError::InvalidResponse; else View.VisitError.Reset();
        return bValid ? EHomesCompletion::Accepted : EHomesCompletion::InvalidResponse;
    }
    EHomesCompletion FHomesState::AcceptReadFailure(const FHomesIntent& Intent)
    {
        if (!CurrentRead(Intent)) return EHomesCompletion::Stale;
        if (Intent.Kind == EHomesOperation::Invitations)
        {
            ListRead.Invalidate(); View.Invitations.Reset(); View.bLoadingList = false; View.ListError = EHomesError::ListFailed;
        }
        else
        {
            VisitRead.Invalidate(); View.Visiting.Reset(); View.bLoadingVisit = false; View.VisitError = EHomesError::VisitFailed;
        }
        return EHomesCompletion::Failed;
    }
    bool FHomesState::CaptureCommand(EHomesOperation Kind, const FString& First, const FString& Second, bool bEnabled, FHomesIntent& Out)
    {
        FHomesIntent Next; Next.Kind = Kind; Next.Owner = OwnerGeneration;
        const auto Args = MakeShared<FJsonObject>();
        if (Kind == EHomesOperation::SetInvitation)
        {
            Next.Operation = TEXT("companionHomes:setInvitation");
            Args->SetStringField(TEXT("handle"), First); Args->SetStringField(TEXT("roomId"), Second); Args->SetBoolField(TEXT("enabled"), bEnabled);
        }
        else if (Kind == EHomesOperation::RevokeInvitation)
        {
            Next.Operation = TEXT("companionHomes:revokeInvitation"); Args->SetStringField(TEXT("invitationId"), First);
        }
        else return false;
        if (!HomesStatePrivate::Encode(Args, Next.ArgumentsJson)) return false;
        Next.Request = FGuid::NewGuid(); ActiveCommand = Next; View.bBusy = true; View.CommandError.Reset();
        Out = MoveTemp(Next); return true;
    }
    bool FHomesState::SetInvitation(const FHomesOwnerReceipt& Owner, const FString& Handle, const FString& RoomId, bool bEnabled, FHomesIntent& Out)
    {
        const bool bHandleValid = HomesStatePrivate::Bounded(Handle, 80, true);
        const FString CapturedHandle = bHandleValid ? Handle : FString();
        const FString CapturedRoom = HomesStatePrivate::Bounded(RoomId, 80) ? RoomId : FString();
        Out = {};
        return CurrentOwner(Owner) && !ActiveCommand.IsSet() && bHandleValid && !CapturedRoom.IsEmpty()
            && CaptureCommand(EHomesOperation::SetInvitation, CapturedHandle, CapturedRoom, bEnabled, Out);
    }
    bool FHomesState::RevokeInvitation(const FHomesOwnerReceipt& Owner, const FString& InvitationId, FHomesIntent& Out)
    {
        const FString Captured = HomesStatePrivate::Bounded(InvitationId, 256) ? InvitationId : FString();
        Out = {};
        return CurrentOwner(Owner) && !ActiveCommand.IsSet() && !Captured.IsEmpty()
            && CaptureCommand(EHomesOperation::RevokeInvitation, Captured, FString(), false, Out);
    }
    EHomesCompletion FHomesState::AcceptNullResult(const FHomesIntent& Intent, const FString& Json)
    {
        if (!CurrentCommand(Intent)) return EHomesCompletion::Stale;
        StateJsonPrivate::FValue Value;
        const bool bValid = StateJsonPrivate::ParseNull(Json, Value, HomesStatePrivate::NullLimits);
        FinishCommand();
        if (!bValid) View.CommandError = EHomesError::InvalidResponse; else View.CommandError.Reset();
        return bValid ? EHomesCompletion::Accepted : EHomesCompletion::InvalidResponse;
    }
    EHomesCompletion FHomesState::AcceptCommandFailure(const FHomesIntent& Intent)
    {
        if (!CurrentCommand(Intent)) return EHomesCompletion::Stale;
        const auto Kind = Intent.Kind;
        FinishCommand(); View.CommandError = Kind == EHomesOperation::SetInvitation ? EHomesError::InvitationFailed : EHomesError::RevokeFailed;
        return EHomesCompletion::Failed;
    }
    bool FHomesState::Cancel(const FHomesIntent& Intent)
    {
        if (CurrentRead(Intent))
        {
            if (Intent.Kind == EHomesOperation::Invitations)
            {
                ListRead.Invalidate(); View.Invitations.Reset(); View.bLoadingList = false; View.ListError.Reset();
            }
            else
            {
                VisitRead.Invalidate(); View.Visiting.Reset(); View.bLoadingVisit = false; View.VisitError.Reset();
            }
            return true;
        }
        if (!CurrentCommand(Intent)) return false;
        FinishCommand(); View.CommandError.Reset(); return true;
    }
    void FHomesState::Dispose()
    {
        StopList(); StopVisit(); FinishCommand(); View = {}; OwnerGeneration.Invalidate(); bDisposed = true;
    }
}
