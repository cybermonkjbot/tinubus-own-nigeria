#include "BaddieCompanionKitchenState.h"
#include "BaddieCompanionStateJson.h"

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
            if (!V.IsValid() || V->Type != EJson::Array || V->AsArray().Num() > 4096) return false;
            for (const auto& Item : V->AsArray())
            {
                T Next;
                if (!Read(Item, Next)) return false;
                Out.Add(MoveTemp(Next));
            }
            return true;
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
        View.Error.Reset();
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
    void FKitchenReadState::Clear()
    {
        if (bDisposed) return;
        EndSubscription();
        Selection.Reset();
        View = FKitchenReadView{};
        Changed.Broadcast();
    }
    void FKitchenReadState::Dispose()
    {
        if (bDisposed) return;
        bDisposed = true;
        EndSubscription();
        Selection.Reset();
        Changed.Clear();
    }
}
