#pragma once

#include "BaddieCompanionFoodState.h"

namespace BaddieCompanion
{
    struct FKitchenScope
    {
        FString RoomId, CharacterId;
        TOptional<FString> CallSessionId, InvitationId;
        TOptional<int64> RoomRevision;
    };
    enum class EKitchenStorage : uint8 { Fridge, Pantry };
    struct FKitchenCooking
    {
        FString BatchId, ActivityId, Label;
        double StartedAtMs = 0, EndsAtMs = 0;
        bool bShared = false;
    };
    struct FKitchenStock
    {
        EFoodKey Key = EFoodKey::Suya;
        FString Label;
        double Quantity = 0;
        EKitchenStorage Storage = EKitchenStorage::Pantry;
    };
    struct FKitchenBatch
    {
        FString Id, Label;
        // Guest remaining is the server's served projection, not the owner's private stock.
        double Remaining = 0, Served = 0;
    };
    struct FKitchenRecipe
    {
        FString Key, Label;
        double Portions = 0, GameSeconds = 0;
        bool bAvailable = false;
        // Optional, non-nullable on the wire. Missing differs from zero or false.
        TOptional<double> CookingLevel, CookingExperience;
        TOptional<bool> Unlocked;
    };
    struct FKitchenSnapshot
    {
        FString RoomId;
        bool bOwner = false, bCanCook = false;
        int64 CareRevision = 0;
        TOptional<FKitchenCooking> Cooking;
        TArray<FKitchenStock> Stock;
        TArray<FKitchenBatch> Batches;
        TArray<FKitchenRecipe> Recipes;
    };
    BADDIECOMPANION_API bool DecodeKitchenScope(const FString& Json, FKitchenScope& Out, FString& OutError);
    BADDIECOMPANION_API bool DecodeKitchenSnapshot(const FString& Json, FKitchenSnapshot& Out, FString& OutError);

    class FKitchenReadReceipt
    {
    public:
        const TOptional<FKitchenScope>& GetScope() const { return Scope; }
    private:
        friend class FKitchenReadState;
        FGuid Generation;
        TOptional<FKitchenScope> Scope;
    };
    struct FKitchenReadView
    {
        TSharedPtr<const FKitchenSnapshot> Kitchen;
        // Missing until a successful read; failure retains the last observed scope.
        TOptional<FKitchenScope> Scope;
        bool bReady = false;
        // Read failures only, separate from any future uncertain cooking command.
        TOptional<FString> Error;
    };
    DECLARE_MULTICAST_DELEGATE(FKitchenReadChanged);
    class BADDIECOMPANION_API FKitchenReadState
    {
    public:
        FKitchenReadState() = default;
        FKitchenReadState(const FKitchenReadState&) = delete;
        FKitchenReadState& operator=(const FKitchenReadState&) = delete;
        const FKitchenReadView& GetView() const { return View; }
        const TOptional<FKitchenScope>& GetSelection() const { return Selection; }
        FKitchenReadChanged& OnChanged() { return Changed; }
        bool Select(const TOptional<FKitchenScope>& Scope);
        // Same explicit observation lifecycle as food, endpoint companionKitchen:snapshot.
        FKitchenReadReceipt BeginSubscription();
        void EndSubscription();
        EFoodReadResult Accept(const FKitchenReadReceipt& Receipt, const FString& Json);
        bool AcceptFailure(const FKitchenReadReceipt& Receipt);
        void Clear();
        // Canonical disposal retains presentation. Call Clear first for account teardown.
        void Dispose();
    private:
        bool Current(const FKitchenReadReceipt& Receipt) const;
        FKitchenReadView View;
        TOptional<FKitchenScope> Selection;
        FGuid Generation;
        bool bSubscribed = false, bDisposed = false;
        FKitchenReadChanged Changed;
    };
}
