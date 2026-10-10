#include "BaddieCompanionCharacterMaterials.h"

#include "Components/MeshComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkinnedAsset.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Templates/UnrealTemplate.h"

namespace BaddieCompanion::CharacterMaterials
{
    UObject* MeshAsset(UMeshComponent* Component)
    {
        if (auto* Static = Cast<UStaticMeshComponent>(Component)) return Static->GetStaticMesh();
        if (auto* Skinned = Cast<USkinnedMeshComponent>(Component)) return Skinned->GetSkinnedAsset();
        return nullptr;
    }
    UMaterialInterface* SlotOverride(const UMeshComponent* Component, int32 Slot)
    {
        // Read only. Changes must go through SetMaterial for UE's GC/render lifecycle.
        return Component->OverrideMaterials.IsValidIndex(Slot) ? Component->OverrideMaterials[Slot].Get() : nullptr;
    }
    bool FiniteColor(const FLinearColor& Color)
    {
        return FMath::IsFinite(Color.R) && FMath::IsFinite(Color.G)
            && FMath::IsFinite(Color.B) && FMath::IsFinite(Color.A);
    }
    bool ReadColor(const FString& Hex, FLinearColor& Out)
    {
        if (Hex.Len() != 7 || Hex[0] != TEXT('#')) return false;
        for (int32 I = 1; I < Hex.Len(); ++I)
            if (!FChar::IsHexDigit(Hex[I])) return false;
        // Saved CSS hex is sRGB. FLinearColor(FColor) converts it to linear RGB once.
        Out = FLinearColor(FColor::FromHex(Hex));
        return FiniteColor(Out);
    }
    bool ValidRequirement(const FCharacterMaterialRequirement& Requirement)
    {
        if (Requirement.SourceMaterialName.IsEmpty()) return false;
        const bool bIris = Requirement.SourceMaterialName.Equals(TEXT("Amber iris"), ESearchCase::CaseSensitive);
        switch (Requirement.Rule)
        {
        case EColorRule::PreserveAuthored: return !Requirement.TargetHex.IsSet();
        case EColorRule::IrisLuminanceTint: return bIris && Requirement.TargetHex.IsSet();
        case EColorRule::ExplicitOverride:
        case EColorRule::AutomaticBase:
        case EColorRule::LookDefault: return !bIris && Requirement.TargetHex.IsSet();
        default: return false;
        }
    }
}

UBaddieCompanionCharacterMaterials::UBaddieCompanionCharacterMaterials()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(false);
}

void UBaddieCompanionCharacterMaterials::ClearPrivateMaterials()
{
    check(IsInGameThread());
    bClearRequested = true;
    // SetMaterial can call lifecycle hooks. Keep the current traversal valid and cancel Apply
    // immediately after that native call returns, before another slot can be changed.
    if (bChangingMaterials) return;
    TGuardValue<bool> Changing(bChangingMaterials, true);
    RestoreOwnedMaterials();
    Issue = BaddieCompanion::ENativeCharacterMaterialIssue::None;
}

void UBaddieCompanionCharacterMaterials::RestoreOwnedMaterials()
{
    for (auto& Entry : Owned)
    {
        auto* Component = Entry.Component.Get();
        if (IsValid(Component) && IsValid(Entry.Override.Get())
            && BaddieCompanion::CharacterMaterials::SlotOverride(Component, Entry.Slot) == Entry.Override.Get())
        {
            const auto Slots = Component->GetMaterialSlotNames();
            const bool bSameBinding = Entry.MeshAsset.IsValid()
                && BaddieCompanion::CharacterMaterials::MeshAsset(Component) == Entry.MeshAsset.Get()
                && Slots.IsValidIndex(Entry.Slot) && Slots[Entry.Slot] == Entry.SlotName;
            // Null restores the original default lookup, not an override of the old mesh's default.
            // A replaced mesh gets its own default. Another owner's override is left untouched.
            Component->SetMaterial(Entry.Slot, bSameBinding ? Entry.PreviousOverride.Get() : nullptr);
        }
        if (IsValid(Entry.Override.Get())) Entry.Override->ClearParameterValues();
    }
    Owned.Reset();
}

bool UBaddieCompanionCharacterMaterials::Apply(const BaddieCompanion::FHomePresentationPlan& Plan,
    const BaddieCompanion::FNativeCharacterMaterialContent& Content)
{
    using namespace BaddieCompanion;
    using namespace BaddieCompanion::CharacterMaterials;
    check(IsInGameThread());
    if (bChangingMaterials) return false;
    TGuardValue<bool> Changing(bChangingMaterials, true);
    bClearRequested = false;
    RestoreOwnedMaterials();
    Issue = ENativeCharacterMaterialIssue::None;
    const auto Fail = [this](ENativeCharacterMaterialIssue Reason)
    {
        RestoreOwnedMaterials();
        Issue = Reason;
        return false;
    };
    auto* Owner = GetOwner();
    auto* World = GetWorld();
    if (bClearRequested || bEndingPlay || !IsRegistered() || !IsValid(Owner)
        || Owner->IsActorBeingDestroyed() || !World || !World->IsGameWorld()
        || World->GetNetMode() != NM_Standalone)
        return Fail(ENativeCharacterMaterialIssue::OwnerUnavailable);
    if (!Plan.Appearance.bCompleteKnownSelections || Plan.CharacterMaterials.IsEmpty()
        || Plan.CharacterMaterials.Num() > 64 || Content.SourceAssetKey.IsEmpty()
        || Content.SourceSha256.Len() != 64
        || !Content.SourceAssetKey.Equals(Plan.CharacterAsset.Key, ESearchCase::CaseSensitive)
        || !Content.SourceSha256.Equals(Plan.CharacterAsset.Sha256, ESearchCase::CaseSensitive))
        return Fail(ENativeCharacterMaterialIssue::SourceMismatch);
    if (Content.Bindings.IsEmpty() || Content.Bindings.Num() > 256)
        return Fail(ENativeCharacterMaterialIssue::InvalidBinding);

    TArray<FString> SourceNames;
    for (const auto& Requirement : Plan.CharacterMaterials)
    {
        if (!ValidRequirement(Requirement)
            || SourceNames.ContainsByPredicate([&](const FString& Name)
                { return Name.Equals(Requirement.SourceMaterialName, ESearchCase::CaseSensitive); }))
            return Fail(ENativeCharacterMaterialIssue::UnsupportedRecipe);
        SourceNames.Add(Requirement.SourceMaterialName);
        if (!Content.Bindings.ContainsByPredicate([&](const auto& Binding)
            { return Binding.SourceMaterialName.Equals(Requirement.SourceMaterialName, ESearchCase::CaseSensitive); }))
            return Fail(ENativeCharacterMaterialIssue::InvalidBinding);
    }

    struct FResolved
    {
        const FNativeCharacterMaterialBinding* Binding = nullptr;
        TWeakObjectPtr<UMeshComponent> Component;
        TWeakObjectPtr<UObject> Mesh;
        TWeakObjectPtr<UMaterialInterface> Original;
        TWeakObjectPtr<UMaterialInterface> PreviousOverride;
        FName SlotName;
        int32 Slot = INDEX_NONE;
        int32 OwnedIndex = INDEX_NONE;
        TOptional<FLinearColor> Color;
    };
    TArray<FResolved> Resolved;
    for (const auto& Binding : Content.Bindings)
    {
        const auto* Requirement = Plan.CharacterMaterials.FindByPredicate([&](const auto& Value)
            { return Value.SourceMaterialName.Equals(Binding.SourceMaterialName, ESearchCase::CaseSensitive); });
        auto* Component = Binding.Component.Get();
        auto* Mesh = IsValid(Component) ? MeshAsset(Component) : nullptr;
        if (!Requirement || !IsValid(Component) || !Component->IsRegistered() || Component->GetOwner() != Owner
            || Component->GetWorld() != World || !IsValid(Mesh) || Binding.MeshAsset.Get() != Mesh
            || !IsValid(Binding.AuthoredMaterial.Get()) || Binding.MaterialSlot.IsNone()
            || Binding.ColorParameter.IsNone()) return Fail(ENativeCharacterMaterialIssue::InvalidBinding);
        int32 MatchingSlots = 0;
        for (const FName Slot : Component->GetMaterialSlotNames())
            if (Slot == Binding.MaterialSlot) ++MatchingSlots;
        const int32 Slot = Component->GetMaterialIndex(Binding.MaterialSlot);
        if (MatchingSlots != 1 || Slot < 0 || Slot >= Component->GetNumMaterials()
            || Resolved.ContainsByPredicate([&](const auto& Value)
                { return Value.Component.Get() == Component && Value.Slot == Slot; }))
            return Fail(ENativeCharacterMaterialIssue::InvalidBinding);
        auto* Original = Component->GetMaterial(Slot);
        if (Original != Binding.AuthoredMaterial.Get() || Original->IsA<UMaterialInstanceDynamic>())
            return Fail(ENativeCharacterMaterialIssue::MaterialChanged);
        const bool bIris = Binding.SourceMaterialName.Equals(TEXT("Amber iris"), ESearchCase::CaseSensitive);
        if (Binding.Recipe != (bIris ? ENativeCharacterColorRecipe::IrisLuminanceTint
            : ENativeCharacterColorRecipe::BaseColorFactor)
            || (bIris && Binding.IrisEnabledParameter.IsNone())
            || (!bIris && !Binding.IrisEnabledParameter.IsNone()))
            return Fail(ENativeCharacterMaterialIssue::UnsupportedRecipe);
        FLinearColor OriginalColor;
        if (!Original->GetVectorParameterValue(FMaterialParameterInfo(Binding.ColorParameter), OriginalColor)
            || !FiniteColor(OriginalColor)) return Fail(ENativeCharacterMaterialIssue::ParameterUnavailable);
        if (bIris)
        {
            float Enabled = 0;
            if (!Original->GetScalarParameterValue(FMaterialParameterInfo(Binding.IrisEnabledParameter), Enabled)
                || !FMath::IsFinite(Enabled) || Enabled != 0)
                return Fail(ENativeCharacterMaterialIssue::ParameterUnavailable);
        }
        FResolved Next;
        Next.Binding = &Binding; Next.Component = Component; Next.Mesh = Mesh;
        Next.Original = Original; Next.Slot = Slot; Next.SlotName = Binding.MaterialSlot;
        Next.PreviousOverride = SlotOverride(Component, Slot);
        if (Requirement->TargetHex.IsSet())
        {
            FLinearColor Color;
            if (!ReadColor(Requirement->TargetHex.GetValue(), Color))
                return Fail(ENativeCharacterMaterialIssue::UnsupportedRecipe);
            // The source changes RGB only; keep any authored alpha in the vector parameter.
            Color.A = OriginalColor.A;
            Next.Color = Color;
        }
        Resolved.Add(Next);
    }

    const auto BindingIsCurrent = [&](const FResolved& Value, UMaterialInterface* Expected)
    {
        auto* Component = Value.Component.Get();
        return !bClearRequested && !bEndingPlay && IsRegistered() && IsValid(Owner)
            && !Owner->IsActorBeingDestroyed() && IsValid(Component) && Component->IsRegistered()
            && Component->GetOwner() == Owner && Component->GetWorld() == World
            && Value.Mesh.IsValid() && MeshAsset(Component) == Value.Mesh.Get()
            && Component->GetMaterialIndex(Value.SlotName) == Value.Slot
            && IsValid(Expected) && Component->GetMaterial(Value.Slot) == Expected;
    };
    // Prepare all instances before replacing any slot. Missing parameters cannot leave a partial look.
    for (auto& Value : Resolved)
    {
        if (!BindingIsCurrent(Value, Value.Original.Get()))
            return Fail(ENativeCharacterMaterialIssue::MaterialChanged);
        if (!Value.Color.IsSet()) continue;
        auto* Material = UMaterialInstanceDynamic::Create(Value.Original.Get(), this);
        if (!IsValid(Material)) return Fail(ENativeCharacterMaterialIssue::InstanceUnavailable);
        Material->SetFlags(RF_Transient);
        Value.OwnedIndex = Owned.Num();
        auto& Entry = Owned.AddDefaulted_GetRef();
        Entry.Component = Value.Component; Entry.MeshAsset = Value.Mesh;
        Entry.PreviousOverride = Value.PreviousOverride.Get(); Entry.Override = Material;
        Entry.Slot = Value.Slot; Entry.SlotName = Value.SlotName;
        Material->SetVectorParameterValue(Value.Binding->ColorParameter, Value.Color.GetValue());
        if (Value.Binding->Recipe == ENativeCharacterColorRecipe::IrisLuminanceTint)
            Material->SetScalarParameterValue(Value.Binding->IrisEnabledParameter, 1.f);
    }
    for (const auto& Value : Resolved)
    {
        if (!BindingIsCurrent(Value, Value.Original.Get())
            || SlotOverride(Value.Component.Get(), Value.Slot) != Value.PreviousOverride.Get())
            return Fail(ENativeCharacterMaterialIssue::MaterialChanged);
        if (Value.OwnedIndex == INDEX_NONE) continue;
        auto* Material = Owned[Value.OwnedIndex].Override.Get();
        Value.Component->SetMaterial(Value.Slot, Material);
        if (!BindingIsCurrent(Value, Material))
            return Fail(ENativeCharacterMaterialIssue::MaterialChanged);
    }
    // A later SetMaterial callback can also invalidate an earlier or unchanged binding.
    for (const auto& Value : Resolved)
    {
        auto* Expected = Value.OwnedIndex == INDEX_NONE ? Value.Original.Get()
            : Owned[Value.OwnedIndex].Override.Get();
        if (!BindingIsCurrent(Value, Expected)) return Fail(ENativeCharacterMaterialIssue::MaterialChanged);
    }
    return true; // Material parameters only, never whole-character or gameplay readiness.
}

void UBaddieCompanionCharacterMaterials::EndPlay(const EEndPlayReason::Type Reason)
{
    bEndingPlay = true;
    ClearPrivateMaterials();
    Super::EndPlay(Reason);
}

void UBaddieCompanionCharacterMaterials::OnUnregister()
{
    ClearPrivateMaterials();
    Super::OnUnregister();
}
