#include "BaddieCompanionRecoveryState.h"
#include "BaddieCompanionStateJson.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace BaddieCompanion
{
    namespace RecoveryStatePrivate
    {
        using StateJsonPrivate::FObject;
        using StateJsonPrivate::FValue;
        using StateJsonPrivate::Same;
        using StateJsonPrivate::Object;
        using StateJsonPrivate::Field;
        using StateJsonPrivate::Only;
        using StateJsonPrivate::Parse;
        using StateJsonPrivate::String;
        using StateJsonPrivate::Number;
        using StateJsonPrivate::Integer;
        using StateJsonPrivate::Nullable;
        using StateJsonPrivate::ValidTime;
        constexpr StateJsonPrivate::FParseLimits ParseLimits{32768, 8, 128};
        bool Destination(const FValue& V, FCompanionRecoveryDestination& Out)
        {
            const auto O = Object(V);
            if (!Only(O, {TEXT("placeId"), TEXT("instanceId"), TEXT("position"), TEXT("readyAt")})
                || !String(Field(O, TEXT("placeId")), Out.PlaceId) || !String(Field(O, TEXT("instanceId")), Out.InstanceId)
                || !Number(O, TEXT("readyAt"), Out.ReadyAtMs)) return false;
            const auto P = Field(O, TEXT("position"));
            return P.IsValid() && P->Type == EJson::Array && P->AsArray().Num() == 3
                && Number(P->AsArray()[0], Out.Position.X) && Number(P->AsArray()[1], Out.Position.Y)
                && Number(P->AsArray()[2], Out.Position.Z);
        }
        bool Health(const FObject& O, FCompanionHealthView& Out)
        {
            FString Status;
            if (!Only(O, {TEXT("lifeId"), TEXT("revision"), TEXT("health"), TEXT("maxHealth"), TEXT("status"),
                    TEXT("deathKey"), TEXT("protectedUntil"), TEXT("recovery")})
                || !String(Field(O, TEXT("lifeId")), Out.LifeId) || Out.LifeId.IsEmpty()
                || !Integer(Field(O, TEXT("revision")), Out.Revision)
                || !Number(O, TEXT("health"), Out.Health) || !Number(O, TEXT("maxHealth"), Out.MaxHealth)
                || !String(Field(O, TEXT("status")), Status) || !Number(O, TEXT("protectedUntil"), Out.ProtectedUntilMs)
                || !Nullable(Field(O, TEXT("deathKey")), Out.DeathKey, String)
                || !Nullable(Field(O, TEXT("recovery")), Out.Recovery, Destination)) return false;
            if (Same(Status, TEXT("active"))) Out.Status = ECompanionHealthStatus::Active;
            else if (Same(Status, TEXT("hospital"))) Out.Status = ECompanionHealthStatus::Hospital;
            else if (Same(Status, TEXT("detained"))) Out.Status = ECompanionHealthStatus::Detained;
            else return false;
            return true;
        }
    }

    bool DecodeRecoveryResponse(const FString& Json, ERecoveryResponseKind Kind, FRecoveryDecodedResponse& Out, FString& OutError)
    {
        using namespace RecoveryStatePrivate;
        Out = {}; OutError = TEXT("Recovery response is unsupported or malformed.");
        if (Json.IsEmpty() || Json.Len() > ParseLimits.Characters
            || (Kind != ERecoveryResponseKind::Subscription && Kind != ERecoveryResponseKind::Mutation)) return false;
        FRecoveryDecodedResponse Next; Next.Kind = Kind;
        if (Kind == ERecoveryResponseKind::Subscription && Same(Json.TrimStartAndEnd(), TEXT("null")))
        {
            Next.bValid = true; Out = MoveTemp(Next); OutError.Reset(); return true;
        }
        FObject O;
        if (!Parse(Json, O, ParseLimits)) return false;
        FObject HealthObject = O;
        if (Kind == ERecoveryResponseKind::Mutation)
        {
            if (!Only(O, {TEXT("health"), TEXT("serverTime")}) || !Number(O, TEXT("serverTime"), Next.ServerTimeMs)
                || !ValidTime(Next.ServerTimeMs)) return false;
            HealthObject = Object(Field(O, TEXT("health")));
        }
        FCompanionHealthView Candidate;
        if (!Health(HealthObject, Candidate)) return false;
        Next.Health = MakeShared<FCompanionHealthView>(MoveTemp(Candidate));
        Next.bHasHealth = true;
        Next.bValid = true; Out = MoveTemp(Next); OutError.Reset(); return true;
    }

    void FRecoveryReadState::SelectOwner(const FString& OwnerId)
    {
        if (bDisposed || RecoveryStatePrivate::Same(View.OwnerId, OwnerId)) return;
        const FString NextOwner = OwnerId;
        View = {}; View.OwnerId = NextOwner; Generation = FGuid::NewGuid(); RetireRecovery();
        bSubscribed = false; Changed.Broadcast();
    }
    FRecoveryReceipt FRecoveryReadState::BeginSubscription()
    {
        if (bDisposed || View.OwnerId.IsEmpty()) return {};
        Generation = FGuid::NewGuid(); RetireRecovery(); bSubscribed = true;
        FRecoveryReceipt Receipt; Receipt.Generation = Generation; Receipt.OwnerId = View.OwnerId;
        Changed.Broadcast(); return Receipt;
    }
    void FRecoveryReadState::EndSubscription()
    {
        if (bDisposed) return;
        Generation = FGuid::NewGuid(); RetireRecovery(); bSubscribed = false;
        Changed.Broadcast();
    }
    TOptional<FRecoveryReceipt> FRecoveryReadState::CaptureRecovery(double BeganAtMs)
    {
        return CaptureRecoveryInternal(BeganAtMs, {});
    }
    TOptional<FRecoveryReceipt> FRecoveryReadState::PrepareRecovery(const FString& ServiceId, double BeganAtMs)
    {
        if (ServiceId.IsEmpty() || ServiceId.Len() > 100) return {};
        return CaptureRecoveryInternal(BeganAtMs, TOptional<FString>(ServiceId));
    }
    TOptional<FRecoveryReceipt> FRecoveryReadState::CaptureRecoveryInternal(double BeganAtMs, const TOptional<FString>& ServiceId)
    {
        if (bDisposed || !bSubscribed || View.bBusy || !View.Health.IsValid() || !RecoveryStatePrivate::ValidTime(BeganAtMs)) return {};
        FRecoveryReceipt Receipt;
        Receipt.Generation = Generation; Receipt.Kind = ERecoveryResponseKind::Mutation;
        Receipt.MutationId = FGuid::NewGuid(); Receipt.OwnerId = View.OwnerId;
        Receipt.LifeId = View.Health->LifeId; Receipt.BeganAtMs = BeganAtMs;
        Receipt.bOwnedPreparation = ServiceId.IsSet();
        if (Receipt.bOwnedPreparation)
        {
            const auto Args = MakeShared<FJsonObject>();
            Args->SetStringField(TEXT("serviceId"), ServiceId.GetValue());
            Args->SetStringField(TEXT("lifeId"), Receipt.LifeId);
            if (!FJsonSerializer::Serialize(Args, TJsonWriterFactory<>::Create(&Receipt.ArgumentsJson))) return {};
        }
        ActiveMutation = Receipt.MutationId;
        PreparedRecovery.Reset(); bRecoveryDispatched = false;
        if (Receipt.bOwnedPreparation) PreparedRecovery = Receipt;
        View.bBusy = true; View.Error.Reset(); Changed.Broadcast();
        // Reentry may retire this generation and even prepare another owner's request.
        if (!Current(Receipt)) return {};
        return Receipt;
    }
    TOptional<FRecoveryDispatch> FRecoveryReadState::DispatchRecovery(const FRecoveryReceipt& Receipt)
    {
        if (!Receipt.bOwnedPreparation || !Current(Receipt) || bRecoveryDispatched) return {};
        FRecoveryDispatch Dispatch;
        Dispatch.Receipt = PreparedRecovery.GetValue();
        bRecoveryDispatched = true;
        return Dispatch;
    }
    bool FRecoveryReadState::AbandonPreparedRecovery(const FRecoveryReceipt& Receipt)
    {
        if (!Receipt.bOwnedPreparation || !Current(Receipt) || bRecoveryDispatched) return false;
        RetireRecovery();
        Changed.Broadcast();
        return true;
    }
    void FRecoveryReadState::RetireRecovery()
    {
        ActiveMutation.Invalidate(); PreparedRecovery.Reset(); bRecoveryDispatched = false; View.bBusy = false;
    }
    bool FRecoveryReadState::Current(const FRecoveryReceipt& Receipt) const
    {
        return !bDisposed && bSubscribed && Receipt.Generation.IsValid() && Receipt.Generation == Generation
            && RecoveryStatePrivate::Same(Receipt.OwnerId, View.OwnerId)
            && (Receipt.Kind != ERecoveryResponseKind::Mutation || (ActiveMutation.IsValid() && Receipt.MutationId == ActiveMutation
                && Receipt.bOwnedPreparation == PreparedRecovery.IsSet()
                && (!Receipt.bOwnedPreparation || PreparedRecovery->MutationId == Receipt.MutationId)));
    }
    bool FRecoveryReadState::CanComplete(const FRecoveryReceipt& Receipt) const
    {
        return Current(Receipt) && (!Receipt.bOwnedPreparation || bRecoveryDispatched);
    }
    ERecoveryReadResult FRecoveryReadState::Accept(const FRecoveryReceipt& Receipt, const FRecoveryDecodedResponse& Response, double ReceivedAtMs)
    {
        using namespace RecoveryStatePrivate;
        if (!CanComplete(Receipt)) return ERecoveryReadResult::StaleReceipt;
        if (!Response.IsValid() || Response.GetKind() != Receipt.Kind
            || (Receipt.Kind == ERecoveryResponseKind::Mutation && !ValidTime(ReceivedAtMs)))
        {
            AcceptFailure(Receipt); return ERecoveryReadResult::InvalidResponse;
        }
        const auto& Health = Response.GetHealth();
        if (Receipt.Kind == ERecoveryResponseKind::Subscription)
        {
            // Canonical subscriptions replace unconditionally, even across lives or revisions.
            View.Health = Health; View.HealthPresence = Health.IsValid() ? EWorldFieldPresence::Value : EWorldFieldPresence::Null;
            View.bReady = true; View.Error.Reset(); Changed.Broadcast(); return ERecoveryReadResult::Accepted;
        }
        RetireRecovery();
        if (View.Health.IsValid() && Health->Revision < View.Health->Revision)
        {
            Changed.Broadcast(); return ERecoveryReadResult::OlderRevision;
        }
        // Recovery may create a new life. The canonical mutation guard compares revision, not life ID.
        View.Health = Health; View.HealthPresence = EWorldFieldPresence::Value;
        View.ServerOffsetMs = Response.GetServerTimeMs() - (Receipt.BeganAtMs / 2 + ReceivedAtMs / 2);
        // A successful mutation does not clear a subscription error received during its flight.
        Changed.Broadcast(); return ERecoveryReadResult::Accepted;
    }
    bool FRecoveryReadState::AcceptFailure(const FRecoveryReceipt& Receipt)
    {
        if (!CanComplete(Receipt)) return false;
        if (Receipt.Kind == ERecoveryResponseKind::Subscription)
        {
            View.bReady = true; View.Error = TEXT("Recovery could not connect. Try again.");
        }
        else
        {
            if (View.Health.IsValid() && RecoveryStatePrivate::Same(View.Health->LifeId, Receipt.LifeId))
                View.Error = TEXT("Recovery could not finish. Try again.");
            RetireRecovery();
        }
        Changed.Broadcast(); return true;
    }
    void FRecoveryReadState::Clear()
    {
        if (bDisposed) return;
        View = {}; Generation = FGuid::NewGuid(); RetireRecovery(); bSubscribed = false;
        Changed.Broadcast();
    }
    void FRecoveryReadState::Dispose()
    {
        if (bDisposed) return;
        bDisposed = true; bSubscribed = false; View = {}; Generation.Invalidate(); RetireRecovery();
        Changed.Broadcast(); Changed.Clear();
    }
}
