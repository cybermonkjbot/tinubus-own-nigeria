#include "BaddieCompanionContentHouseState.h"
#include "BaddieCompanionStateJson.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

#include <cmath>

namespace BaddieCompanion
{
    namespace ContentHouseStatePrivate
    {
        using StateJsonPrivate::FObject;
        using StateJsonPrivate::FValue;
        using StateJsonPrivate::Field;
        using StateJsonPrivate::Same;
        constexpr StateJsonPrivate::FParseLimits SnapshotLimits{128 * 1024, 4, 128};
        constexpr StateJsonPrivate::FParseLimits ReceiptLimits{4096, 2, 8};
        bool Bounded(const FString& Value, int32 Maximum, bool bAllowEmpty = false)
        {
            return (bAllowEmpty || !Value.IsEmpty()) && Value.Len() <= Maximum;
        }
        bool Text(const FObject& O, const TCHAR* Name, FString& Out, int32 Maximum, bool bAllowEmpty = false)
        {
            return StateJsonPrivate::String(Field(O, Name), Out) && Bounded(Out, Maximum, bAllowEmpty);
        }
        bool DisplayTime(double Value)
        {
            return std::isfinite(Value) && Value >= 0 && Value <= 9000000000000.0 && std::floor(Value) == Value;
        }
        bool Participant(const FValue& Value, FContentHouseParticipant& Out)
        {
            const auto O = StateJsonPrivate::Object(Value);
            return StateJsonPrivate::Only(O, {TEXT("userId"), TEXT("name"), TEXT("isYou"), TEXT("allowed")})
                && Text(O, TEXT("userId"), Out.UserId, 256) && Text(O, TEXT("name"), Out.Name, 8192, true)
                && StateJsonPrivate::Boolean(Field(O, TEXT("isYou")), Out.bIsYou)
                && StateJsonPrivate::Boolean(Field(O, TEXT("allowed")), Out.bAllowed);
        }
        bool Authorization(const FString& Json, FString& Key, int64& ExpiresAt)
        {
            FObject O;
            return StateJsonPrivate::Parse(Json, O, ReceiptLimits)
                && StateJsonPrivate::Only(O, {TEXT("permissionKey"), TEXT("expiresAt")})
                && Text(O, TEXT("permissionKey"), Key, 256)
                && StateJsonPrivate::Integer(Field(O, TEXT("expiresAt")), ExpiresAt);
        }
        const TCHAR* PurposeName(EContentHousePurpose Purpose)
        {
            switch (Purpose)
            {
            case EContentHousePurpose::Capture: return TEXT("capture");
            case EContentHousePurpose::SaveDraft: return TEXT("save_draft");
            case EContentHousePurpose::Publish: return TEXT("publish");
            default: return nullptr;
            }
        }
        bool Encode(const TSharedRef<FJsonObject>& Args, FString& Out)
        {
            return FJsonSerializer::Serialize(Args,
                TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Out));
        }
    }

    bool DecodeContentHousePermission(const FString& Json, TSharedPtr<const FContentHousePermission>& Out)
    {
        using namespace ContentHouseStatePrivate;
        // Preserve input if it aliases a field owned by the previous output.
        const FString Source = Json.Len() <= SnapshotLimits.Characters ? Json : FString();
        Out.Reset();
        FObject O;
        if (!StateJsonPrivate::Parse(Source, O, SnapshotLimits)
            || !StateJsonPrivate::Only(O, {TEXT("permissionKey"), TEXT("expiresAt"), TEXT("allowed"), TEXT("participants")})) return false;
        const auto Next = MakeShared<FContentHousePermission>();
        if (!Text(O, TEXT("permissionKey"), Next->PermissionKey, 256)
            || !StateJsonPrivate::Integer(Field(O, TEXT("expiresAt")), Next->ExpiresAtMs)
            || !StateJsonPrivate::Boolean(Field(O, TEXT("allowed")), Next->bAllowed)
            || !StateJsonPrivate::ReadBoundedArray(Field(O, TEXT("participants")), Next->Participants, 12, Participant)) return false;
        int32 OwnCount = 0;
        for (int32 Index = 0; Index < Next->Participants.Num(); ++Index)
        {
            const auto& Peer = Next->Participants[Index];
            if (Peer.bIsYou) ++OwnCount;
            if (Next->bAllowed && !Peer.bAllowed) return false;
            for (int32 Prior = 0; Prior < Index; ++Prior)
                if (Same(Peer.UserId, Next->Participants[Prior].UserId)) return false;
        }
        if (OwnCount != 1) return false;
        Out = Next;
        return true;
    }

    const TCHAR* ContentHouseErrorText(EContentHouseError Error)
    {
        switch (Error)
        {
        case EContentHouseError::InvalidClock: return TEXT("Photo permission time is unavailable.");
        case EContentHouseError::ReadFailed: return TEXT("Couldn't load photo permission. Reopen this visit to try again.");
        case EContentHouseError::ChangeFailed: return TEXT("Couldn't update photo permission. Try again.");
        case EContentHouseError::PermissionChanged: return TEXT("Photo permission changed. Take a new photo.");
        default: return TEXT("Photo permission response is unavailable.");
        }
    }
    bool FContentHouseState::CurrentOwner(const FContentHouseOwnerReceipt& Owner) const
    {
        return !bDisposed && Owner.Generation.IsValid() && Owner.Generation == OwnerGeneration;
    }
    bool FContentHouseState::CurrentSubscription(const FContentHouseSubscription& Subscription) const
    {
        return !bDisposed && OwnerGeneration.IsValid() && VisitGeneration.IsValid() && SubscriptionGeneration.IsValid()
            && Subscription.Owner == OwnerGeneration && Subscription.Visit == VisitGeneration
            && Subscription.Generation == SubscriptionGeneration;
    }
    bool FContentHouseState::CurrentRead(const FContentHouseIntent& Intent) const
    {
        return !bDisposed && Intent.Kind == EContentHouseOperation::Snapshot && ActiveRead.IsValid()
            && Intent.Request == ActiveRead && Intent.Owner == OwnerGeneration && Intent.Visit == VisitGeneration
            && SubscriptionGeneration.IsValid() && Intent.Subscription == SubscriptionGeneration;
    }
    bool FContentHouseState::CurrentCommand(const FContentHouseIntent& Intent) const
    {
        if (bDisposed || !ActiveCommand.IsSet() || !Intent.Request.IsValid() || Intent.Owner != OwnerGeneration
            || Intent.Request != ActiveCommand.GetValue().Request || Intent.Kind != ActiveCommand.GetValue().Kind) return false;
        return Intent.Kind == EContentHouseOperation::Revoke
            || (VisitGeneration.IsValid() && Intent.Visit == VisitGeneration
                && SubscriptionGeneration.IsValid() && Intent.Subscription == SubscriptionGeneration);
    }
    void FContentHouseState::FinishCommand()
    {
        ActiveCommand.Reset(); View.bBusy = false;
    }
    void FContentHouseState::ClearPermission()
    {
        View.Permission.Reset(); View.bEffectiveAllowed = false; bExpired = false; ObservationGeneration.Invalidate();
        if (ActiveCommand.IsSet() && ActiveCommand.GetValue().Kind == EContentHouseOperation::AuthorizePhoto)
        {
            FinishCommand(); View.CommandError = EContentHouseError::PermissionChanged;
        }
    }
    void FContentHouseState::StopReads()
    {
        SubscriptionGeneration.Invalidate(); ActiveRead.Invalidate(); ClearPermission(); View.ReadError.Reset();
        if (!ActiveCommand.IsSet() || ActiveCommand.GetValue().Kind != EContentHouseOperation::Revoke)
        {
            FinishCommand(); View.CommandError.Reset();
        }
    }
    FContentHouseOwnerReceipt FContentHouseState::BeginOwner(const FString& UserId)
    {
        if (bDisposed) return {};
        const FString Next = ContentHouseStatePrivate::Bounded(UserId, 256) ? UserId : FString();
        StopReads(); FinishCommand(); View = {};
        OwnerGeneration.Invalidate(); VisitGeneration.Invalidate(); OwnerId.Reset(); InstanceId.Reset(); VisitRevision = 0;
        if (Next.IsEmpty()) return {};
        OwnerId = Next; OwnerGeneration = FGuid::NewGuid();
        FContentHouseOwnerReceipt Receipt; Receipt.Generation = OwnerGeneration; return Receipt;
    }
    bool FContentHouseState::BeginVisit(const FContentHouseOwnerReceipt& Owner, const FString& NextInstance, int64 Revision)
    {
        if (!CurrentOwner(Owner)) return false;
        const FString Next = ContentHouseStatePrivate::Bounded(NextInstance, 256) ? NextInstance : FString();
        EndVisit(Owner);
        if (Next.IsEmpty() || Revision < 0 || Revision > StateJsonPrivate::MaxInteger) return false;
        InstanceId = Next; VisitRevision = Revision; VisitGeneration = FGuid::NewGuid(); return true;
    }
    void FContentHouseState::EndVisit(const FContentHouseOwnerReceipt& Owner)
    {
        if (!CurrentOwner(Owner)) return;
        StopReads(); VisitGeneration.Invalidate(); InstanceId.Reset(); VisitRevision = 0;
    }
    FContentHouseSubscription FContentHouseState::BeginSubscription(const FContentHouseOwnerReceipt& Owner)
    {
        if (!CurrentOwner(Owner) || !VisitGeneration.IsValid()) return {};
        StopReads(); SubscriptionGeneration = FGuid::NewGuid();
        FContentHouseSubscription Result;
        Result.Owner = OwnerGeneration; Result.Visit = VisitGeneration; Result.Generation = SubscriptionGeneration;
        return Result;
    }
    void FContentHouseState::StopSubscription(const FContentHouseSubscription& Subscription)
    {
        if (CurrentSubscription(Subscription)) StopReads();
    }
    bool FContentHouseState::ApplyClock(double DisplayNowMs)
    {
        if (!ContentHouseStatePrivate::DisplayTime(DisplayNowMs))
        {
            ClearPermission(); View.ReadError = EContentHouseError::InvalidClock; return false;
        }
        if (View.Permission.IsValid())
        {
            bExpired = bExpired || View.Permission->ExpiresAtMs <= DisplayNowMs;
            View.bEffectiveAllowed = View.Permission->bAllowed && !bExpired;
            if (bExpired && ActiveCommand.IsSet() && ActiveCommand.GetValue().Kind == EContentHouseOperation::AuthorizePhoto)
            {
                FinishCommand(); View.CommandError = EContentHouseError::PermissionChanged;
            }
        }
        return true;
    }
    bool FContentHouseState::ObserveClock(const FContentHouseSubscription& Subscription, double DisplayNowMs)
    {
        return CurrentSubscription(Subscription) && ApplyClock(DisplayNowMs);
    }
    bool FContentHouseState::Snapshot(const FContentHouseSubscription& Subscription, double DisplayNowMs, FContentHouseIntent& Out)
    {
        using namespace ContentHouseStatePrivate;
        Out = {};
        if (!CurrentSubscription(Subscription)) return false;
        if (!ApplyClock(DisplayNowMs)) { ActiveRead.Invalidate(); return false; }
        const auto Args = MakeShared<FJsonObject>();
        Args->SetStringField(TEXT("instanceId"), InstanceId);
        Args->SetNumberField(TEXT("visitRevision"), static_cast<double>(VisitRevision));
        Args->SetNumberField(TEXT("now"), DisplayNowMs);
        FContentHouseIntent Next;
        if (!Encode(Args, Next.ArgumentsJson)) return false;
        Next.Operation = TEXT("companionContentHouse:snapshot");
        Next.Owner = OwnerGeneration; Next.Visit = VisitGeneration; Next.Subscription = SubscriptionGeneration;
        Next.Request = FGuid::NewGuid(); ActiveRead = Next.Request; Out = MoveTemp(Next); return true;
    }
    EContentHouseCompletion FContentHouseState::AcceptSnapshot(const FContentHouseIntent& Intent, const FString& Json, double DisplayNowMs)
    {
        if (!CurrentRead(Intent)) return EContentHouseCompletion::Stale;
        TSharedPtr<const FContentHousePermission> Next;
        const bool bDecoded = DecodeContentHousePermission(Json, Next);
        ActiveRead.Invalidate();
        ClearPermission();
        if (!bDecoded)
        {
            View.ReadError = EContentHouseError::InvalidResponse; return EContentHouseCompletion::InvalidResponse;
        }
        const auto* Own = Next->Participants.FindByPredicate([](const FContentHouseParticipant& Peer) { return Peer.bIsYou; });
        if (!Own || !StateJsonPrivate::Same(Own->UserId, OwnerId))
        {
            View.ReadError = EContentHouseError::InvalidResponse; return EContentHouseCompletion::InvalidResponse;
        }
        if (!ApplyClock(DisplayNowMs)) return EContentHouseCompletion::InvalidResponse;
        View.Permission = Next; ObservationGeneration = FGuid::NewGuid(); View.ReadError.Reset();
        ApplyClock(DisplayNowMs);
        return EContentHouseCompletion::Accepted;
    }
    EContentHouseCompletion FContentHouseState::AcceptReadFailure(const FContentHouseIntent& Intent)
    {
        if (!CurrentRead(Intent)) return EContentHouseCompletion::Stale;
        ActiveRead.Invalidate(); ClearPermission(); View.ReadError = EContentHouseError::ReadFailed;
        return EContentHouseCompletion::Failed;
    }
    bool FContentHouseState::CaptureCommand(EContentHouseOperation Kind, EContentHousePurpose Purpose,
        bool bAllowCaptureAndShare, FContentHouseIntent& Out)
    {
        using namespace ContentHouseStatePrivate;
        const auto Args = MakeShared<FJsonObject>();
        FContentHouseIntent Next;
        Next.Kind = Kind; Next.Purpose = Purpose; Next.Owner = OwnerGeneration;
        if (Kind == EContentHouseOperation::Revoke) Next.Operation = TEXT("companionContentHouse:revoke");
        else
        {
            Next.Visit = VisitGeneration; Next.Subscription = SubscriptionGeneration;
            Args->SetStringField(TEXT("instanceId"), InstanceId);
            Args->SetNumberField(TEXT("visitRevision"), static_cast<double>(VisitRevision));
            if (Kind == EContentHouseOperation::SetPermission)
            {
                Next.Operation = TEXT("companionContentHouse:setPermission");
                Args->SetBoolField(TEXT("allowCaptureAndShare"), bAllowCaptureAndShare);
            }
            else if (Kind == EContentHouseOperation::AuthorizePhoto && View.Permission.IsValid() && PurposeName(Purpose))
            {
                Next.Operation = TEXT("companionContentHouse:authorizePhoto");
                Next.PermissionKey = View.Permission->PermissionKey; Next.Observation = ObservationGeneration;
                Args->SetStringField(TEXT("permissionKey"), Next.PermissionKey);
                Args->SetStringField(TEXT("purpose"), PurposeName(Purpose));
            }
            else return false;
        }
        if (!Encode(Args, Next.ArgumentsJson)) return false;
        Next.Request = FGuid::NewGuid(); ActiveCommand = Next; View.bBusy = true; View.CommandError.Reset();
        Out = MoveTemp(Next); return true;
    }
    bool FContentHouseState::SetPermission(const FContentHouseOwnerReceipt& Owner, bool bAllowCaptureAndShare, FContentHouseIntent& Out)
    {
        Out = {};
        return CurrentOwner(Owner) && VisitGeneration.IsValid() && SubscriptionGeneration.IsValid() && !ActiveCommand.IsSet()
            && CaptureCommand(EContentHouseOperation::SetPermission, EContentHousePurpose::Capture, bAllowCaptureAndShare, Out);
    }
    bool FContentHouseState::Revoke(const FContentHouseOwnerReceipt& Owner, FContentHouseIntent& Out)
    {
        Out = {};
        return CurrentOwner(Owner) && !ActiveCommand.IsSet()
            && CaptureCommand(EContentHouseOperation::Revoke, EContentHousePurpose::Capture, false, Out);
    }
    bool FContentHouseState::AuthorizePhoto(const FContentHouseOwnerReceipt& Owner, const FString& PermissionKey,
        EContentHousePurpose Purpose, double DisplayNowMs, FContentHouseIntent& Out)
    {
        const FString Key = ContentHouseStatePrivate::Bounded(PermissionKey, 256) ? PermissionKey : FString();
        Out = {};
        if (!CurrentOwner(Owner) || !VisitGeneration.IsValid() || !SubscriptionGeneration.IsValid() || ActiveCommand.IsSet()) return false;
        if (!ApplyClock(DisplayNowMs) || !ContentHouseStatePrivate::PurposeName(Purpose) || !View.bEffectiveAllowed
            || !View.Permission.IsValid() || !ObservationGeneration.IsValid() || !StateJsonPrivate::Same(Key, View.Permission->PermissionKey)) return false;
        return CaptureCommand(EContentHouseOperation::AuthorizePhoto, Purpose, false, Out);
    }
    EContentHouseCompletion FContentHouseState::AcceptNullResult(const FContentHouseIntent& Intent, const FString& Json)
    {
        if ((Intent.Kind != EContentHouseOperation::SetPermission && Intent.Kind != EContentHouseOperation::Revoke)
            || !CurrentCommand(Intent)) return EContentHouseCompletion::Stale;
        StateJsonPrivate::FValue Value;
        const bool bValid = StateJsonPrivate::ParseNull(Json, Value, ContentHouseStatePrivate::ReceiptLimits);
        FinishCommand();
        if (!bValid)
        {
            View.CommandError = EContentHouseError::InvalidResponse; return EContentHouseCompletion::InvalidResponse;
        }
        // A consent mutation result is not a permission snapshot. Require a subsequent read.
        ActiveRead.Invalidate(); ClearPermission(); View.CommandError.Reset();
        return EContentHouseCompletion::Accepted;
    }
    EContentHouseCompletion FContentHouseState::AcceptAuthorization(const FContentHouseIntent& Intent,
        EContentHousePurpose ExpectedPurpose, const FString& Json, double DisplayNowMs)
    {
        using namespace ContentHouseStatePrivate;
        if (Intent.Kind != EContentHouseOperation::AuthorizePhoto || Intent.Purpose != ExpectedPurpose
            || !PurposeName(ExpectedPurpose) || !CurrentCommand(Intent)) return EContentHouseCompletion::Stale;
        FString Key; int64 ExpiresAt = 0;
        const bool bValid = Authorization(Json, Key, ExpiresAt);
        const bool bClockValid = ApplyClock(DisplayNowMs);
        const bool bCurrent = CurrentCommand(Intent) && ObservationGeneration.IsValid()
            && Intent.Observation == ObservationGeneration && View.Permission.IsValid() && View.bEffectiveAllowed
            && Same(Intent.PermissionKey, View.Permission->PermissionKey);
        FinishCommand();
        if (!bValid)
        {
            View.CommandError = EContentHouseError::InvalidResponse; return EContentHouseCompletion::InvalidResponse;
        }
        if (!bClockValid || !bCurrent || ExpiresAt <= DisplayNowMs || !Same(Key, Intent.PermissionKey))
        {
            View.CommandError = EContentHouseError::PermissionChanged; return EContentHouseCompletion::PermissionChanged;
        }
        View.CommandError.Reset();
        return EContentHouseCompletion::Accepted;
    }
    EContentHouseCompletion FContentHouseState::AcceptCommandFailure(const FContentHouseIntent& Intent)
    {
        if (!CurrentCommand(Intent)) return EContentHouseCompletion::Stale;
        const bool bAuthorization = Intent.Kind == EContentHouseOperation::AuthorizePhoto;
        FinishCommand();
        View.CommandError = bAuthorization ? EContentHouseError::PermissionChanged : EContentHouseError::ChangeFailed;
        return EContentHouseCompletion::Failed;
    }
    bool FContentHouseState::Cancel(const FContentHouseIntent& Intent)
    {
        if (CurrentRead(Intent))
        {
            ActiveRead.Invalidate(); ClearPermission(); View.ReadError.Reset(); return true;
        }
        if (!CurrentCommand(Intent)) return false;
        FinishCommand(); View.CommandError.Reset(); return true;
    }
    void FContentHouseState::Dispose()
    {
        if (bDisposed) return;
        StopReads(); FinishCommand(); View = {}; OwnerId.Reset(); InstanceId.Reset(); VisitRevision = 0;
        OwnerGeneration.Invalidate(); VisitGeneration.Invalidate(); bDisposed = true;
    }
}
