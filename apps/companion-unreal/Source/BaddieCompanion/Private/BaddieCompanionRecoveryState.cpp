#include "BaddieCompanionRecoveryState.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include <cmath>
#include <initializer_list>

namespace BaddieCompanion
{
    namespace RecoveryStatePrivate
    {
        using FObject = TSharedPtr<FJsonObject>;
        using FValue = TSharedPtr<FJsonValue>;
        constexpr double MaxInteger = 9007199254740991.0;
        bool Same(const FString& A, const FString& B) { return A.Equals(B, ESearchCase::CaseSensitive); }
        bool Time(double N) { return std::isfinite(N) && N >= 0 && N <= MaxInteger; }
        FObject Object(const FValue& V) { return V.IsValid() && V->Type == EJson::Object ? V->AsObject() : nullptr; }
        FValue Field(const FObject& O, const TCHAR* Key) { return O.IsValid() ? O->TryGetField(Key) : nullptr; }
        bool Only(const FObject& O, std::initializer_list<const TCHAR*> Keys)
        {
            if (!O.IsValid()) return false;
            for (const auto& Pair : O->Values)
            {
                bool bKnown = false;
                for (const auto Key : Keys) if (Same(Pair.Key, Key)) { bKnown = true; break; }
                if (!bKnown) return false;
            }
            return true;
        }
        bool Parse(const FString& Json, FObject& Out)
        {
            if (Json.IsEmpty() || Json.Len() > 32768) return false;
            bool bString = false, bEscape = false;
            int32 Depth = 0, Separators = 0;
            for (TCHAR C : Json)
            {
                if (bString)
                {
                    if (bEscape) bEscape = false;
                    else if (C == TEXT('\\')) bEscape = true;
                    else if (C == TEXT('"')) bString = false;
                    continue;
                }
                if (C == TEXT('"')) bString = true;
                else if (C == TEXT('{') || C == TEXT('[')) { if (++Depth > 8) return false; }
                else if (C == TEXT('}') || C == TEXT(']')) { if (--Depth < 0) return false; }
                else if (C == TEXT(',') || C == TEXT(':')) { if (++Separators > 128) return false; }
            }
            return !bString && Depth == 0 && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Out) && Out.IsValid();
        }
        bool String(const FValue& V, FString& Out)
        {
            return V.IsValid() && V->Type == EJson::String && V->TryGetString(Out);
        }
        bool Number(const FValue& V, double& Out)
        {
            return V.IsValid() && V->Type == EJson::Number && V->TryGetNumber(Out) && std::isfinite(Out);
        }
        bool Number(const FObject& O, const TCHAR* Key, double& Out) { return Number(Field(O, Key), Out); }
        template <typename T, typename F> bool Nullable(const FValue& V, TOptional<T>& Out, F Read)
        {
            if (!V.IsValid()) return false;
            if (V->Type == EJson::Null) return true;
            T Next;
            if (!Read(V, Next)) return false;
            Out = MoveTemp(Next); return true;
        }
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
            double Revision = 0;
            FString Status;
            if (!Only(O, {TEXT("lifeId"), TEXT("revision"), TEXT("health"), TEXT("maxHealth"), TEXT("status"),
                    TEXT("deathKey"), TEXT("protectedUntil"), TEXT("recovery")})
                || !String(Field(O, TEXT("lifeId")), Out.LifeId) || Out.LifeId.IsEmpty()
                || !Number(O, TEXT("revision"), Revision) || !Time(Revision) || std::floor(Revision) != Revision
                || !Number(O, TEXT("health"), Out.Health) || !Number(O, TEXT("maxHealth"), Out.MaxHealth)
                || !String(Field(O, TEXT("status")), Status) || !Number(O, TEXT("protectedUntil"), Out.ProtectedUntilMs)
                || !Nullable(Field(O, TEXT("deathKey")), Out.DeathKey, String)
                || !Nullable(Field(O, TEXT("recovery")), Out.Recovery, Destination)) return false;
            Out.Revision = static_cast<int64>(Revision);
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
        if (Json.IsEmpty() || Json.Len() > 32768
            || (Kind != ERecoveryResponseKind::Subscription && Kind != ERecoveryResponseKind::Mutation)) return false;
        FRecoveryDecodedResponse Next; Next.Kind = Kind;
        if (Kind == ERecoveryResponseKind::Subscription && Same(Json.TrimStartAndEnd(), TEXT("null")))
        {
            Next.bValid = true; Out = MoveTemp(Next); OutError.Reset(); return true;
        }
        FObject O;
        if (!Parse(Json, O)) return false;
        FObject HealthObject = O;
        if (Kind == ERecoveryResponseKind::Mutation)
        {
            if (!Only(O, {TEXT("health"), TEXT("serverTime")}) || !Number(O, TEXT("serverTime"), Next.ServerTimeMs)
                || !Time(Next.ServerTimeMs)) return false;
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
        View = {}; View.OwnerId = NextOwner; Generation = FGuid::NewGuid(); ActiveMutation.Invalidate();
        bSubscribed = false; Changed.Broadcast();
    }
    FRecoveryReceipt FRecoveryReadState::BeginSubscription()
    {
        if (bDisposed || View.OwnerId.IsEmpty()) return {};
        Generation = FGuid::NewGuid(); ActiveMutation.Invalidate(); bSubscribed = true; View.bBusy = false;
        FRecoveryReceipt Receipt; Receipt.Generation = Generation;
        Changed.Broadcast(); return Receipt;
    }
    void FRecoveryReadState::EndSubscription()
    {
        if (bDisposed) return;
        Generation = FGuid::NewGuid(); ActiveMutation.Invalidate(); bSubscribed = false; View.bBusy = false;
        Changed.Broadcast();
    }
    TOptional<FRecoveryReceipt> FRecoveryReadState::CaptureRecovery(double BeganAtMs)
    {
        if (bDisposed || !bSubscribed || View.bBusy || !View.Health.IsValid() || !RecoveryStatePrivate::Time(BeganAtMs)) return {};
        FRecoveryReceipt Receipt;
        Receipt.Generation = Generation; Receipt.Kind = ERecoveryResponseKind::Mutation;
        ActiveMutation = FGuid::NewGuid(); Receipt.MutationId = ActiveMutation;
        Receipt.LifeId = View.Health->LifeId; Receipt.BeganAtMs = BeganAtMs;
        View.bBusy = true; View.Error.Reset(); Changed.Broadcast(); return Receipt;
    }
    bool FRecoveryReadState::Current(const FRecoveryReceipt& Receipt) const
    {
        return !bDisposed && bSubscribed && Receipt.Generation.IsValid() && Receipt.Generation == Generation
            && (Receipt.Kind != ERecoveryResponseKind::Mutation || (ActiveMutation.IsValid() && Receipt.MutationId == ActiveMutation));
    }
    ERecoveryReadResult FRecoveryReadState::Accept(const FRecoveryReceipt& Receipt, const FRecoveryDecodedResponse& Response, double ReceivedAtMs)
    {
        using namespace RecoveryStatePrivate;
        if (!Current(Receipt)) return ERecoveryReadResult::StaleReceipt;
        if (!Response.IsValid() || Response.GetKind() != Receipt.Kind
            || (Receipt.Kind == ERecoveryResponseKind::Mutation && !Time(ReceivedAtMs)))
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
        ActiveMutation.Invalidate(); View.bBusy = false;
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
        if (!Current(Receipt)) return false;
        if (Receipt.Kind == ERecoveryResponseKind::Subscription)
        {
            View.bReady = true; View.Error = TEXT("Recovery could not connect. Try again.");
        }
        else
        {
            if (View.Health.IsValid() && RecoveryStatePrivate::Same(View.Health->LifeId, Receipt.LifeId))
                View.Error = TEXT("Recovery could not finish. Try again.");
            ActiveMutation.Invalidate(); View.bBusy = false;
        }
        Changed.Broadcast(); return true;
    }
    void FRecoveryReadState::Clear()
    {
        if (bDisposed) return;
        View = {}; Generation = FGuid::NewGuid(); ActiveMutation.Invalidate(); bSubscribed = false;
        Changed.Broadcast();
    }
    void FRecoveryReadState::Dispose()
    {
        if (bDisposed) return;
        bDisposed = true; bSubscribed = false; View = {}; Generation.Invalidate(); ActiveMutation.Invalidate();
        Changed.Broadcast(); Changed.Clear();
    }
}
