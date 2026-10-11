#include "BaddieCompanionRecoveryState.h"
#include "BaddieCompanionWorldPlayerRead.h"
#include "BaddieCompanionStateJson.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace BaddieCompanion
{
    namespace WorldReadPrivate
    {
        const FRecordSchema& HealthSchema()
        {
            using K = EWorldValueKind;
            static constexpr FTextChoice Statuses[] = {TEXT("active"), TEXT("hospital"), TEXT("detained")};
            static constexpr FFieldRule Rules[] = {
                FFieldRule(TEXT("lifeId"), K::String).WithText({1}),
                FFieldRule(TEXT("revision"), K::Number).WithNumber(0, MaxSafeInteger, true),
                FFieldRule(TEXT("health"), K::Number), FFieldRule(TEXT("maxHealth"), K::Number),
                FFieldRule(TEXT("status"), K::String).WithChoices(Statuses),
                FFieldRule(TEXT("deathKey"), K::String, true, true),
                FFieldRule(TEXT("protectedUntil"), K::Number), FFieldRule(TEXT("recovery"), K::Object, true, true)
            };
            static constexpr FRecordSchema Schema(Rules);
            return Schema;
        }
        const FRecordSchema& RecoveryDestinationSchema()
        {
            using K = EWorldValueKind;
            static constexpr FFieldRule Rules[] = {
                FFieldRule(TEXT("placeId"), K::String), FFieldRule(TEXT("instanceId"), K::String),
                FFieldRule(TEXT("position"), K::Array).WithCount(3, 3), FFieldRule(TEXT("readyAt"), K::Number)
            };
            static constexpr FRecordSchema Schema(Rules);
            return Schema;
        }
        void FHealthRead::Cancel()
        {
            Health.Cancel(); Destination.Cancel(); Root = {}; Position = 0;
            Phase = Coordinate = 0; bNull = false; Status = WorldObservationPrivate::EStep::Invalid;
        }
        void FHealthRead::Begin(FValueRef InValue)
        {
            Cancel(); Root = InValue; Status = WorldObservationPrivate::EStep::Pending;
        }
        WorldObservationPrivate::EStep FHealthRead::Advance(const WorldObservationPrivate::FStorage& Storage)
        {
            using namespace WorldObservationPrivate;
            if (!Storage.bComplete || Storage.bRetiring) return Status = EStep::Invalid;
            if (Status != EStep::Pending) return Status;
            if (Phase == 0)
            {
                EWorldValueKind Kind;
                const auto Result = ReadKind(Storage, Root, Kind);
                if (Result != EStep::Complete) return Result == EStep::Pending ? Result : Status = Result;
                if (Kind == EWorldValueKind::Null) { bNull = true; return Status = EStep::Complete; }
                Health.Begin(Root, HealthSchema()); Phase = 1;
            }
            if (Phase == 1)
            {
                const auto Result = Health.Advance(Storage);
                if (Result != EStep::Complete) return Result == EStep::Pending ? Result : Status = Result;
                if (Health.Presence(7) == EWorldFieldPresence::Null) return Status = EStep::Complete;
                Destination.Begin(Health.Field(7), RecoveryDestinationSchema()); Phase = 2;
            }
            if (Phase == 2)
            {
                const auto Result = Destination.Advance(Storage);
                if (Result != EStep::Complete) return Result == EStep::Pending ? Result : Status = Result;
                Phase = 3;
            }
            if (Phase == 3)
            {
                if (!Storage.Budget->Spend(8)) return Storage.Budget->bExhausted ? EStep::ResourceUnavailable : EStep::Pending;
                const auto Ref = Destination.Field(2);
                const auto* Array = Storage.Get(Ref.Root, Ref.Node);
                if (!Array) return Status = EStep::Invalid;
                Position = Array->FirstChild; Phase = 4;
            }
            while (Coordinate < 3)
            {
                // Coordinate and sibling are one atomic step, so a tick yield
                // cannot skip a value or reread it without charging.
                if (!Storage.Budget->Spend(24)) return Storage.Budget->bExhausted ? EStep::ResourceUnavailable : EStep::Pending;
                const auto* Node = Storage.Get(Root.Root, Position);
                if (!Node || Node->Kind != EWorldValueKind::Number) return Status = EStep::Invalid;
                double N = 0;
                FMemory::Memcpy(&N, &Node->NumberBits, sizeof(N));
                if (!FiniteNumber(N)) return Status = EStep::Invalid;
                Position = Node->NextSibling; ++Coordinate;
            }
            return Status = Position == 0 ? EStep::Complete : EStep::Invalid;
        }
    }

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
            if (!WorldReadPrivate::ReadRecord(O, WorldReadPrivate::RecoveryDestinationSchema())
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
            if (!WorldReadPrivate::ReadRecord(O, WorldReadPrivate::HealthSchema())
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
        if (Kind == ERecoveryResponseKind::Subscription && Same(Json.TrimStartAndEnd(), TEXT("null")))
            return ReadWorldRecoveryValue(MakeShared<FJsonValueNull>(), Kind, Out, OutError);
        FObject O;
        if (!Parse(Json, O, ParseLimits)) return false;
        return ReadWorldRecoveryValue(MakeShared<FJsonValueObject>(O), Kind, Out, OutError);
    }

    bool ReadWorldRecoveryValue(const TSharedPtr<FJsonValue>& Value, ERecoveryResponseKind Kind,
        FRecoveryDecodedResponse& Out, FString& OutError)
    {
        using namespace RecoveryStatePrivate;
        Out = {}; OutError = TEXT("Recovery response is unsupported or malformed.");
        if (!Value.IsValid() || (Kind != ERecoveryResponseKind::Subscription
            && Kind != ERecoveryResponseKind::Mutation)) return false;
        FRecoveryDecodedResponse Next; Next.Kind = Kind;
        if (Kind == ERecoveryResponseKind::Subscription && Value->Type == EJson::Null)
        {
            Next.bValid = true; Out = MoveTemp(Next); OutError.Reset(); return true;
        }
        const auto O = Object(Value);
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
