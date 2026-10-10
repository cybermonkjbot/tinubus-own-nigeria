#include "BaddieCompanionKitchenState.h"
#include "BaddieCompanionStateJson.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include <cmath>

namespace BaddieCompanion
{
    namespace KitchenPrivate
    {
        using namespace StateJsonPrivate;
        // Resource bounds permit recovered batches beyond the normal 24-batch creation limit.
        constexpr FParseLimits Limits { 2097152, 32, 131072 };
        bool Time(const FValue& V, double& Out) { return Number(V, Out) && ValidTime(Out); }
        bool Text(const FObject& O, const TCHAR* K, FString& Out) { return String(Field(O, K), Out); }
        template <typename T, typename F> bool Optional(const FValue& V, TOptional<T>& Out, F Read)
        {
            if (!V.IsValid()) return true;
            T Next;
            if (!Read(V, Next)) return false;
            Out = MoveTemp(Next);
            return true;
        }
        template <typename T, typename F> bool Array(const FValue& V, TArray<T>& Out, F Read)
        {
            return ReadBoundedArray(V, Out, 4096, Read);
        }
        bool Revision(const FValue& V, int64& Out) { return Integer(V, Out); }
        bool Scalar(const FValue& V, double& Out) { return Number(V, Out); }
        bool Scope(const FObject& O, FKitchenScope& Out)
        {
            return Only(O, { TEXT("roomId"), TEXT("characterId"), TEXT("callSessionId"), TEXT("roomRevision"), TEXT("invitationId") })
                && Text(O, TEXT("roomId"), Out.RoomId) && Text(O, TEXT("characterId"), Out.CharacterId)
                && Optional(Field(O, TEXT("callSessionId")), Out.CallSessionId, String)
                && Optional(Field(O, TEXT("roomRevision")), Out.RoomRevision, Revision)
                && Optional(Field(O, TEXT("invitationId")), Out.InvitationId, String);
        }
        bool Cooking(const FValue& V, FKitchenCooking& Out)
        {
            const auto O = Object(V);
            return Only(O, { TEXT("batchId"), TEXT("activityId"), TEXT("label"), TEXT("startedAt"), TEXT("endsAt"), TEXT("shared") })
                && Text(O, TEXT("batchId"), Out.BatchId) && Text(O, TEXT("activityId"), Out.ActivityId)
                && Text(O, TEXT("label"), Out.Label) && Time(Field(O, TEXT("startedAt")), Out.StartedAtMs)
                && Time(Field(O, TEXT("endsAt")), Out.EndsAtMs) && Boolean(Field(O, TEXT("shared")), Out.bShared);
        }
        bool Stock(const FValue& V, FKitchenStock& Out)
        {
            const auto O = Object(V);
            FString Key, Storage;
            if (!Only(O, { TEXT("key"), TEXT("label"), TEXT("quantity"), TEXT("storage") })
                || !Text(O, TEXT("key"), Key) || !ParseFoodKey(Key, Out.Key) || !Text(O, TEXT("label"), Out.Label)
                || !Number(O, TEXT("quantity"), Out.Quantity) || !Text(O, TEXT("storage"), Storage)) return false;
            if (Same(Storage, TEXT("fridge"))) Out.Storage = EKitchenStorage::Fridge;
            else if (Same(Storage, TEXT("pantry"))) Out.Storage = EKitchenStorage::Pantry;
            else return false;
            return true;
        }
        bool Batch(const FValue& V, FKitchenBatch& Out)
        {
            const auto O = Object(V);
            return Only(O, { TEXT("id"), TEXT("label"), TEXT("remaining"), TEXT("served") })
                && Text(O, TEXT("id"), Out.Id) && Text(O, TEXT("label"), Out.Label)
                && Number(O, TEXT("remaining"), Out.Remaining) && Number(O, TEXT("served"), Out.Served);
        }
        bool Recipe(const FValue& V, FKitchenRecipe& Out)
        {
            const auto O = Object(V);
            return Only(O, { TEXT("key"), TEXT("label"), TEXT("portions"), TEXT("gameSeconds"), TEXT("available"),
                    TEXT("cookingLevel"), TEXT("cookingExperience"), TEXT("unlocked") })
                && Text(O, TEXT("key"), Out.Key) && Text(O, TEXT("label"), Out.Label)
                && Number(O, TEXT("portions"), Out.Portions) && Number(O, TEXT("gameSeconds"), Out.GameSeconds)
                && Boolean(Field(O, TEXT("available")), Out.bAvailable)
                && Optional(Field(O, TEXT("cookingLevel")), Out.CookingLevel, Scalar)
                && Optional(Field(O, TEXT("cookingExperience")), Out.CookingExperience, Scalar)
                && Optional(Field(O, TEXT("unlocked")), Out.Unlocked, Boolean);
        }
        bool Snapshot(const FObject& O, FKitchenSnapshot& Out)
        {
            return Only(O, { TEXT("roomId"), TEXT("owner"), TEXT("canCook"), TEXT("careRevision"), TEXT("cooking"),
                    TEXT("stock"), TEXT("batches"), TEXT("recipes") })
                && Text(O, TEXT("roomId"), Out.RoomId) && Boolean(Field(O, TEXT("owner")), Out.bOwner)
                && Boolean(Field(O, TEXT("canCook")), Out.bCanCook) && Integer(Field(O, TEXT("careRevision")), Out.CareRevision)
                && Nullable(Field(O, TEXT("cooking")), Out.Cooking, Cooking)
                && Array(Field(O, TEXT("stock")), Out.Stock, Stock) && Array(Field(O, TEXT("batches")), Out.Batches, Batch)
                && Array(Field(O, TEXT("recipes")), Out.Recipes, Recipe);
        }
        template <typename T, typename F> bool Decode(const FString& Json, T& Out, FString& Error, F Read)
        {
            Out = T{};
            Error = TEXT("Invalid kitchen read response.");
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
        bool SameScope(const FKitchenScope& A, const FKitchenScope& B)
        {
            return Same(A.RoomId, B.RoomId) && Same(A.CharacterId, B.CharacterId)
                && SameOptionalText(A.CallSessionId, B.CallSessionId) && SameOptionalText(A.InvitationId, B.InvitationId)
                && A.RoomRevision.IsSet() == B.RoomRevision.IsSet()
                && (!A.RoomRevision.IsSet() || A.RoomRevision.GetValue() == B.RoomRevision.GetValue());
        }
        bool SameSelection(const TOptional<FKitchenScope>& A, const TOptional<FKitchenScope>& B)
        {
            return A.IsSet() == B.IsSet() && (!A.IsSet() || SameScope(A.GetValue(), B.GetValue()));
        }
        bool EncodeCommand(const FKitchenScope& Scope, const FKitchenCommand& Command, const FString& RequestId, FString& Json)
        {
            auto Action = MakeShared<FJsonObject>();
            switch (Command.Type)
            {
            case EKitchenCommandType::Serve:
                if (!std::isfinite(Command.Portions)) return false;
                Action->SetStringField(TEXT("type"), TEXT("serve"));
                Action->SetStringField(TEXT("batchId"), Command.BatchId);
                Action->SetNumberField(TEXT("portions"), Command.Portions);
                break;
            case EKitchenCommandType::Cook:
                if (!std::isfinite(Command.ExpectedRevision) || (Command.Batches != EKitchenBatchCount::One
                    && Command.Batches != EKitchenBatchCount::Two)) return false;
                Action->SetStringField(TEXT("type"), TEXT("cook"));
                Action->SetStringField(TEXT("recipeKey"), Command.RecipeKey);
                Action->SetNumberField(TEXT("batches"), static_cast<uint8>(Command.Batches));
                Action->SetNumberField(TEXT("expectedRevision"), Command.ExpectedRevision);
                break;
            case EKitchenCommandType::StopCooking:
                if (!std::isfinite(Command.ExpectedRevision)) return false;
                Action->SetStringField(TEXT("type"), TEXT("stop-cooking"));
                Action->SetStringField(TEXT("batchId"), Command.BatchId);
                Action->SetStringField(TEXT("activityId"), Command.ActivityId);
                Action->SetNumberField(TEXT("expectedRevision"), Command.ExpectedRevision);
                break;
            case EKitchenCommandType::PlateMeal:
                if (!std::isfinite(Command.Portions)) return false;
                Action->SetStringField(TEXT("type"), TEXT("plate-meal"));
                Action->SetNumberField(TEXT("portions"), Command.Portions);
                break;
            case EKitchenCommandType::Drink:
            case EKitchenCommandType::EatStock:
            {
                const TCHAR* Key = FoodKeyWire(Command.Key);
                if (!Key) return false;
                Action->SetStringField(TEXT("type"), Command.Type == EKitchenCommandType::Drink ? TEXT("drink") : TEXT("eat-stock"));
                Action->SetStringField(TEXT("key"), Key);
                break;
            }
            case EKitchenCommandType::Eat:
                Action->SetStringField(TEXT("type"), TEXT("eat"));
                Action->SetStringField(TEXT("batchId"), Command.BatchId);
                break;
            default: return false;
            }
            auto Args = MakeShared<FJsonObject>();
            Args->SetStringField(TEXT("roomId"), Scope.RoomId);
            Args->SetStringField(TEXT("characterId"), Scope.CharacterId);
            if (Scope.CallSessionId.IsSet()) Args->SetStringField(TEXT("callSessionId"), Scope.CallSessionId.GetValue());
            if (Scope.InvitationId.IsSet()) Args->SetStringField(TEXT("invitationId"), Scope.InvitationId.GetValue());
            if (Scope.RoomRevision.IsSet()) Args->SetNumberField(TEXT("roomRevision"), static_cast<double>(Scope.RoomRevision.GetValue()));
            Args->SetObjectField(TEXT("command"), Action);
            Args->SetStringField(TEXT("requestId"), RequestId);
            return FJsonSerializer::Serialize(Args, TJsonWriterFactory<>::Create(&Json));
        }
        bool CommandResult(const FString& Json, const FString& RequestId)
        {
            FObject O;
            FString Id, Outcome;
            return Parse(Json, O, Limits) && Only(O, { TEXT("requestId"), TEXT("outcome") })
                && Text(O, TEXT("requestId"), Id) && Same(Id, RequestId) && Text(O, TEXT("outcome"), Outcome)
                && (Same(Outcome, TEXT("applied")) || Same(Outcome, TEXT("replayed")));
        }
    }

    bool DecodeKitchenScope(const FString& Json, FKitchenScope& Out, FString& OutError)
    {
        return KitchenPrivate::Decode(Json, Out, OutError, KitchenPrivate::Scope);
    }
    bool DecodeKitchenSnapshot(const FString& Json, FKitchenSnapshot& Out, FString& OutError)
    {
        return KitchenPrivate::Decode(Json, Out, OutError, KitchenPrivate::Snapshot);
    }
    bool FKitchenReadState::Select(const TOptional<FKitchenScope>& Scope)
    {
        if (bDisposed || (Scope.IsSet() && Scope->RoomRevision.IsSet()
            && (Scope->RoomRevision.GetValue() < 0 || Scope->RoomRevision.GetValue() > StateJsonPrivate::MaxInteger))) return false;
        if (KitchenPrivate::SameSelection(Selection, Scope)) return true;
        EndSubscription();
        Selection = Scope;
        View = FKitchenReadView{};
        View.bBusy = Attempt.IsValid();
        View.bRetryable = Pending.IsValid() && bUncertain;
        Changed.Broadcast();
        return true;
    }
    FKitchenReadReceipt FKitchenReadState::BeginSubscription()
    {
        EndSubscription();
        FKitchenReadReceipt Receipt;
        if (bDisposed || !Selection.IsSet()) return Receipt;
        bSubscribed = true;
        Receipt.Generation = Generation;
        Receipt.Scope = Selection;
        return Receipt;
    }
    void FKitchenReadState::EndSubscription()
    {
        Generation = FGuid::NewGuid();
        bSubscribed = false;
    }
    bool FKitchenReadState::Current(const FKitchenReadReceipt& Receipt) const
    {
        return !bDisposed && bSubscribed && Receipt.Generation.IsValid() && Receipt.Generation == Generation
            && Selection.IsSet() && Receipt.Scope.IsSet() && KitchenPrivate::SameSelection(Selection, Receipt.Scope);
    }
    EFoodReadResult FKitchenReadState::Accept(const FKitchenReadReceipt& Receipt, const FString& Json)
    {
        if (!Current(Receipt)) return EFoodReadResult::StaleReceipt;
        FKitchenSnapshot Next;
        FString Error;
        if (!DecodeKitchenSnapshot(Json, Next, Error)) return EFoodReadResult::InvalidResponse;
        if (!StateJsonPrivate::Same(Next.RoomId, Receipt.Scope->RoomId)) return EFoodReadResult::ScopeMismatch;
        View.Kitchen = MakeShared<FKitchenSnapshot>(MoveTemp(Next));
        View.Scope = Receipt.Scope;
        View.bReady = true;
        if (!Pending.IsValid() || !bUncertain) View.Error.Reset();
        Changed.Broadcast();
        return EFoodReadResult::Accepted;
    }
    bool FKitchenReadState::AcceptFailure(const FKitchenReadReceipt& Receipt)
    {
        if (!Current(Receipt)) return false;
        View.Kitchen.Reset();
        View.bReady = true;
        View.Error = TEXT("Kitchen details couldn't load. Reload to retry.");
        Changed.Broadcast();
        return true;
    }
    bool FKitchenReadState::PrepareCommand(const FKitchenCommand& Command, const FString& RequestId, FFoodKitchenCommandReceipt& Out)
    {
        Out = FFoodKitchenCommandReceipt{};
        if (bDisposed || Attempt.IsValid() || Pending.IsValid() || !Selection.IsSet()
            || !ValidFoodKitchenRequestId(RequestId)) return false;
        auto Request = MakeShared<FRequest>();
        Request->Scope = Selection.GetValue();
        Request->Command = Command;
        Request->RequestId = RequestId;
        if (!KitchenPrivate::EncodeCommand(Request->Scope, Command, RequestId, Request->ArgumentsJson)) return false;
        Pending = Request;
        bUncertain = false;
        return PrepareAttempt(Out);
    }
    bool FKitchenReadState::PrepareRetry(FFoodKitchenCommandReceipt& Out)
    {
        Out = FFoodKitchenCommandReceipt{};
        return Pending.IsValid() && bUncertain && PrepareAttempt(Out);
    }
    bool FKitchenReadState::PrepareAttempt(FFoodKitchenCommandReceipt& Out)
    {
        if (bDisposed || Attempt.IsValid() || !Pending.IsValid()
            || (!bUncertain && (!Selection.IsSet() || !KitchenPrivate::SameScope(Selection.GetValue(), Pending->Scope)))) return false;
        FFoodKitchenCommandReceipt Receipt;
        Receipt.Attempt = Attempt = FGuid::NewGuid();
        Receipt.Endpoint = TEXT("companionKitchen:command");
        Receipt.ArgumentsJson = Pending->ArgumentsJson;
        Receipt.RequestId = Pending->RequestId;
        bReplay = bUncertain;
        bDispatched = false;
        View.bBusy = true;
        View.bRetryable = false;
        View.Error.Reset();
        Changed.Broadcast();
        if (!CurrentAttempt(Receipt)) return false;
        if (!bReplay && (!Selection.IsSet() || !KitchenPrivate::SameScope(Selection.GetValue(), Pending->Scope)))
        {
            AbandonPreparedCommand(Receipt);
            return false;
        }
        Out = MoveTemp(Receipt);
        return true;
    }
    bool FKitchenReadState::CurrentAttempt(const FFoodKitchenCommandReceipt& Receipt) const
    {
        return !bDisposed && Pending.IsValid() && Receipt.Attempt.IsValid() && Receipt.Attempt == Attempt;
    }
    bool FKitchenReadState::DispatchCommand(const FFoodKitchenCommandReceipt& Receipt)
    {
        if (!CurrentAttempt(Receipt) || bDispatched) return false;
        if (!bReplay && (!Selection.IsSet() || !KitchenPrivate::SameScope(Selection.GetValue(), Pending->Scope)))
        {
            AbandonPreparedCommand(Receipt);
            return false;
        }
        bDispatched = true;
        bUncertain = true;
        return true;
    }
    bool FKitchenReadState::AbandonPreparedCommand(const FFoodKitchenCommandReceipt& Receipt)
    {
        if (!CurrentAttempt(Receipt) || bDispatched) return false;
        if (!bUncertain) Pending.Reset();
        FinishAttempt();
        return true;
    }
    void FKitchenReadState::FinishAttempt()
    {
        Attempt.Invalidate();
        bDispatched = bReplay = false;
        if (!Pending.IsValid()) bUncertain = false;
        View.bBusy = false;
        View.bRetryable = Pending.IsValid() && bUncertain;
        Changed.Broadcast();
        // Reentrant listeners own any subsequent state changes.
    }
    EFoodKitchenCompletion FKitchenReadState::CompleteCommand(const FFoodKitchenCommandReceipt& Receipt, const FString& Json)
    {
        if (!CurrentAttempt(Receipt) || !bDispatched) return EFoodKitchenCompletion::StaleReceipt;
        if (!KitchenPrivate::CommandResult(Json, Pending->RequestId))
        {
            FailCommand(Receipt, FFoodKitchenCommandFailure{});
            return EFoodKitchenCompletion::InvalidResponse;
        }
        Pending.Reset();
        // The live read owns Kitchen, Scope, readiness and any error received during flight.
        FinishAttempt();
        return EFoodKitchenCompletion::Applied;
    }
    bool FKitchenReadState::FailCommand(const FFoodKitchenCommandReceipt& Receipt, const FFoodKitchenCommandFailure& Failure)
    {
        if (!CurrentAttempt(Receipt) || !bDispatched
            || (Failure.Category != EFoodKitchenFailureCategory::Definite && Failure.Category != EFoodKitchenFailureCategory::Ambiguous)) return false;
        const bool bSelected = Selection.IsSet() && KitchenPrivate::SameScope(Selection.GetValue(), Pending->Scope);
        if (Failure.Category == EFoodKitchenFailureCategory::Definite) Pending.Reset();
        if (bSelected) View.Error = Failure.PublicMessage(TEXT("Something went wrong. Please try again."));
        FinishAttempt();
        return true;
    }
    void FKitchenReadState::Clear()
    {
        if (bDisposed) return;
        EndSubscription();
        Selection.Reset();
        Pending.Reset();
        Attempt.Invalidate();
        bUncertain = bDispatched = bReplay = false;
        View = FKitchenReadView{};
        Changed.Broadcast();
    }
    void FKitchenReadState::Dispose()
    {
        if (bDisposed) return;
        bDisposed = true;
        EndSubscription();
        Selection.Reset();
        Pending.Reset();
        Attempt.Invalidate();
        bUncertain = bDispatched = bReplay = false;
        Changed.Clear();
    }
}
