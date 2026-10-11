#include "BaddieCompanionCharacterActor.h"

#include "Components/ChildActorComponent.h"
#include "Components/MeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Level.h"
#include "Engine/SkinnedAsset.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Templates/UnrealTemplate.h"

namespace BaddieCompanion::CharacterAccessories
{
    TOptional<bool> SelectedVisibility(const FAppearancePlan& Appearance, const FString& SourceNode)
    {
        if (SourceNode.Equals(TEXT("AccessoryNecklace"), ESearchCase::CaseSensitive))
            return Appearance.EffectiveNecklaceVisible;
        if (SourceNode.Equals(TEXT("AccessoryChain"), ESearchCase::CaseSensitive))
            return Appearance.EffectiveHipChainVisible;
        return {}; // Eyewear also needs the current attached-equipment state, absent from this plan.
    }
    UObject* MeshAsset(UMeshComponent* Component)
    {
        if (auto* Static = Cast<UStaticMeshComponent>(Component)) return Static->GetStaticMesh();
        if (auto* Skinned = Cast<USkinnedMeshComponent>(Component)) return Skinned->GetSkinnedAsset();
        return nullptr;
    }
}

ABaddieCompanionCharacterActor::ABaddieCompanionCharacterActor()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = false;
    SetReplicateMovement(false);
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    AutoReceiveInput = EAutoReceiveInput::Disabled;
    auto* Root = CreateDefaultSubobject<USceneComponent>(TEXT("CharacterRoot"));
    Root->SetMobility(EComponentMobility::Movable);
    SetRootComponent(Root);
    Materials = CreateDefaultSubobject<UBaddieCompanionCharacterMaterials>(TEXT("SavedMaterials"));
}

bool ABaddieCompanionCharacterActor::AcceptsContent(const BaddieCompanion::FHomePresentationPlan& Plan,
    const BaddieCompanion::FNativeCharacterContent& Content)
{
    return AcceptsContent(static_cast<const BaddieCompanion::FHomePresentationData&>(Plan), Content);
}

bool ABaddieCompanionCharacterActor::AcceptsContent(const BaddieCompanion::FHomePresentationData& Plan,
    const BaddieCompanion::FNativeCharacterContent& Content)
{
    if (!Plan.Appearance.bCompleteKnownSelections || Content.ActorClass.IsNull()
        || Content.SourceAssetKey.IsEmpty() || Content.SourceSha256.Len() != 64
        || !Content.SourceAssetKey.Equals(Plan.CharacterAsset.Key, ESearchCase::CaseSensitive)
        || !Content.SourceSha256.Equals(Plan.CharacterAsset.Sha256, ESearchCase::CaseSensitive)
        || Content.Materials.IsEmpty() || Content.Materials.Num() > 256
        || Content.Accessories.Num() < 2 || Content.Accessories.Num() > 128) return false;
    for (const auto& Binding : Content.Materials)
        if (Binding.MeshComponent.IsNone() || Binding.MeshAsset.IsNull() || Binding.AuthoredMaterial.IsNull()
            || Binding.SourceMaterialName.IsEmpty() || Binding.MaterialSlot.IsNone()
            || Binding.ColorParameter.IsNone()) return false;
    TArray<FName> AccessoryMeshes;
    bool bNecklace = false, bHipChain = false;
    for (const auto& Binding : Content.Accessories)
    {
        if (Binding.MeshComponent.IsNone() || Binding.MeshAsset.IsNull() || !Binding.VisibleWhenEnabled.IsSet()
            || !BaddieCompanion::CharacterAccessories::SelectedVisibility(Plan.Appearance, Binding.SourceNode).IsSet()
            || AccessoryMeshes.Contains(Binding.MeshComponent)) return false;
        AccessoryMeshes.Add(Binding.MeshComponent);
        bNecklace |= Binding.SourceNode.Equals(TEXT("AccessoryNecklace"), ESearchCase::CaseSensitive);
        bHipChain |= Binding.SourceNode.Equals(TEXT("AccessoryChain"), ESearchCase::CaseSensitive);
    }
    if (!bNecklace || !bHipChain
        || !BaddieCompanion::FNativeCharacterFaceAppearance::Accepts(Plan.Appearance, Content.FaceNodes)) return false;
    return true; // Declaration checks only; imported provenance/coverage requires content acceptance.
}

bool ABaddieCompanionCharacterActor::HasInertDefaults(UClass* Class)
{
    if (!IsValid(Class) || !Class->IsChildOf(StaticClass())
        || Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists)) return false;
    const auto* Defaults = Class->GetDefaultObject<ABaddieCompanionCharacterActor>();
    // CDO checks do not audit Blueprint-added components or scripts. Content acceptance owns those.
    return IsValid(Defaults) && Defaults->IsHidden() && !Defaults->GetActorEnableCollision()
        && !Defaults->GetIsReplicated() && !Defaults->PrimaryActorTick.bCanEverTick
        && Defaults->AutoReceiveInput == EAutoReceiveInput::Disabled && Defaults->GetRootComponent()
        && Defaults->GetRootComponent()->GetRelativeTransform().Equals(FTransform::Identity, 1.e-6);
}

void ABaddieCompanionCharacterActor::SuppressPresentation()
{
    TGuardValue<bool> Changing(bChangingPresentation, true);
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    SetActorTickEnabled(false);
    TInlineComponentArray<UActorComponent*> Components(this);
    for (auto* Component : Components)
    {
        if (!IsValid(Component)) continue;
        Component->SetComponentTickEnabled(false);
        Component->Deactivate();
        if (!IsValid(Component)) continue;
        // Actor hiding preserves authored per-component visibility, including hidden accessories.
        if (auto* Primitive = Cast<UPrimitiveComponent>(Component))
        {
            Primitive->SetSimulatePhysics(false);
            Primitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Primitive->SetCanEverAffectNavigation(false);
        }
    }
}

void ABaddieCompanionCharacterActor::PostInitializeComponents()
{
    Super::PostInitializeComponents();
    SuppressPresentation();
}

void ABaddieCompanionCharacterActor::BeginPlay()
{
    SuppressPresentation();
    Super::BeginPlay();
    SuppressPresentation();
}

bool ABaddieCompanionCharacterActor::Prepare(const BaddieCompanion::FHomePresentationData& Plan,
    const BaddieCompanion::FNativeCharacterContent& Content,
    const FTransform& LevelToApartment, bool bLevelAlreadyTransformed)
{
    check(IsInGameThread());
    if (bChangingPresentation || bPrepared || bDiscarded) return false;
    bool bSuccess = false;
    {
        TGuardValue<bool> Changing(bChangingPresentation, true);
        bSuccess = PrepareBindings(Plan, Content, LevelToApartment, bLevelAlreadyTransformed);
    }
    // Mesh disposal must run after any native SetMaterial callback stack has unwound.
    if (!bSuccess || bDiscarded || IsActorBeingDestroyed()) { ClearPrivateCharacter(); return false; }
    bPrepared = true;
    return true;
}

bool ABaddieCompanionCharacterActor::PrepareBindings(const BaddieCompanion::FHomePresentationData& Plan,
    const BaddieCompanion::FNativeCharacterContent& Content,
    const FTransform& LevelToApartment, bool bLevelAlreadyTransformed, const FTransform* WorldPose)
{
    SuppressPresentation();
    if (bDiscarded || IsActorBeingDestroyed() || !GetWorld() || !GetWorld()->IsGameWorld()
        || GetWorld()->GetNetMode() != NM_Standalone || GetIsReplicated()
        || AutoReceiveInput != EAutoReceiveInput::Disabled
        || !AcceptsContent(Plan, Content) || Content.ActorClass.Get() != GetClass()
        || !Plan.Home.bSourceMapped || !Plan.Home.Shell.IsSet() || !IsValid(Materials.Get())
        || !GetRootComponent() || !GetActorTransform().Equals(FTransform::Identity, 1.e-6)
        || LevelToApartment.ContainsNaN() || !LevelToApartment.GetRotation().IsNormalized()
        || !LevelToApartment.GetScale3D().Equals(FVector::OneVector, 1.e-6)
        || !LevelToApartment.GetRotation().RotateVector(FVector::UpVector).Equals(FVector::UpVector, 1.e-6))
        return false;

    TInlineComponentArray<UChildActorComponent*> ChildActors(this);
    if (!ChildActors.IsEmpty()) return false; // This owner's material slots belong to this actor only.

    const auto& Shell = Plan.Home.Shell.GetValue();
    if (!FMath::IsFinite(Shell.CharacterScale) || Shell.CharacterScale <= 0) return false;
    FTransform LocalPose;
    if (WorldPose)
    {
        if (WorldPose->ContainsNaN() || !WorldPose->GetRotation().IsNormalized()
            || !WorldPose->GetScale3D().Equals(FVector::OneVector, 1.e-6)) return false;
        LocalPose = *WorldPose;
        LocalPose.SetScale3D(FVector(Shell.CharacterScale));
    }
    else
    {
        const auto& Spawn = Shell.PreferredSpawn;
        if (Spawn.UnrealCentimetres.ContainsNaN() || !FMath::IsFinite(Spawn.UnrealYawDegrees)) return false;
        LocalPose = FTransform(FRotator(0, Spawn.UnrealYawDegrees, 0),
            Spawn.UnrealCentimetres, FVector(Shell.CharacterScale));
    }
    const FTransform ApartmentPose = LocalPose * LevelToApartment;
    if (ApartmentPose.ContainsNaN()
        || !SetActorTransform(bLevelAlreadyTransformed ? ApartmentPose : LocalPose, false,
            nullptr, ETeleportType::TeleportPhysics) || bDiscarded || IsActorBeingDestroyed()) return false;

    BaddieCompanion::FNativeCharacterMaterialContent Resolved;
    Resolved.SourceAssetKey = Content.SourceAssetKey; Resolved.SourceSha256 = Content.SourceSha256;
    TInlineComponentArray<UMeshComponent*> Meshes(this);
    for (const auto& Named : Content.Materials)
    {
        UMeshComponent* Found = nullptr;
        for (auto* Mesh : Meshes)
            if (IsValid(Mesh) && Mesh->GetOwner() == this && Mesh->GetFName() == Named.MeshComponent)
            {
                if (Found) return false;
                Found = Mesh;
            }
        if (!Found) return false;
        auto& Binding = Resolved.Bindings.AddDefaulted_GetRef();
        static_cast<BaddieCompanion::FNativeCharacterMaterialSlot&>(Binding) = Named;
        Binding.Component = Found;
    }
    if (!IsValid(Materials.Get()) || !Materials->Apply(Plan, Resolved)
        || bDiscarded || IsActorBeingDestroyed() || !IsValid(Materials.Get())) return false;
    PreparedMaterialGeneration = Materials->GetAppliedGeneration();
    const auto IsCurrent = [this]()
    {
        return !bDiscarded && !IsActorBeingDestroyed() && IsHidden() && HasCurrentMaterials();
    };
    if (!FaceAppearance.Apply(*this, Plan.Appearance, Content.FaceNodes, IsCurrent)
        || !PrepareAccessoryVisibility(Plan.Appearance, Content.Accessories)
        || !IsCurrent() || !FaceAppearance.Matches(*this)) return false;
    TSet<FName> FaceComponents;
    FaceAppearance.AppendControlledComponentNames(FaceComponents);
    TSet<FName> ProtectedComponents = FaceComponents;
    for (const auto& Material : Content.Materials) ProtectedComponents.Add(Material.MeshComponent);
    for (const auto& Accessory : Content.Accessories) ProtectedComponents.Add(Accessory.MeshComponent);
    if (!Equipment.Initialize(*this, Content.Equipment, ProtectedComponents, FaceComponents,
        Plan.Appearance.Eyewear.Effective.GetValue() == BaddieCompanion::EEyewear::Round)) return false;
    if (!bLevelAlreadyTransformed) PendingApartmentPose = ApartmentPose;
    return true; // Hidden face/material/accessory preparation, not complete appearance or movement readiness.
}

bool ABaddieCompanionCharacterActor::PrepareWorld(const BaddieCompanion::FHomePresentationData& Plan,
    const BaddieCompanion::FNativeCharacterContent& Content, const FTransform& WorldPose)
{
    check(IsInGameThread());
    if (bChangingPresentation || bPrepared || bDiscarded) return false;
    bool bSuccess = false;
    {
        TGuardValue<bool> Changing(bChangingPresentation, true);
        bSuccess = PrepareBindings(Plan, Content, FTransform::Identity, true, &WorldPose);
    }
    if (!bSuccess || bDiscarded || IsActorBeingDestroyed()) { ClearPrivateCharacter(); return false; }
    bPrepared = true;
    return true;
}

bool ABaddieCompanionCharacterActor::PrepareAccessoryVisibility(const BaddieCompanion::FAppearancePlan& Appearance,
    const TArray<BaddieCompanion::FNativeCharacterAccessoryMesh>& Bindings)
{
    using namespace BaddieCompanion::CharacterAccessories;
    // Prepare owns the presentation guard. Stage every supplied occurrence before changing flags.
    if (!bChangingPresentation || bDiscarded || IsActorBeingDestroyed()) return false;
    struct FResolved
    {
        TWeakObjectPtr<UMeshComponent> Component;
        TWeakObjectPtr<UObject> Asset;
        FName Name;
        bool bBaseline = false;
        bool bTarget = false;
    };
    TArray<FResolved> Resolved;
    TInlineComponentArray<UMeshComponent*> Meshes(this);
    for (const auto& Binding : Bindings)
    {
        const auto Selected = SelectedVisibility(Appearance, Binding.SourceNode);
        if (!Selected.IsSet() || !Binding.VisibleWhenEnabled.IsSet()) return false;
        UMeshComponent* Found = nullptr;
        for (auto* Mesh : Meshes)
            if (IsValid(Mesh) && Mesh->GetOwner() == this && Mesh->GetFName() == Binding.MeshComponent)
            {
                if (Found) return false;
                Found = Mesh;
            }
        auto* Asset = Found ? MeshAsset(Found) : nullptr;
        if (!Found || !Found->IsRegistered() || Found->GetWorld() != GetWorld() || Found->bHiddenInGame
            || !IsValid(Asset) || Binding.MeshAsset.Get() != Asset
            || Found->GetVisibleFlag() != Binding.VisibleWhenEnabled.GetValue()
            || Resolved.ContainsByPredicate([&](const FResolved& Value) { return Value.Component.Get() == Found; }))
            return false;
        FResolved Next;
        Next.Component = Found; Next.Asset = Asset; Next.Name = Binding.MeshComponent;
        Next.bBaseline = Binding.VisibleWhenEnabled.GetValue();
        Next.bTarget = Selected.GetValue() && Next.bBaseline;
        Resolved.Add(Next);
    }
    const auto Matches = [&](const FResolved& Value, bool bExpectedVisible)
    {
        const auto* Mesh = Value.Component.Get();
        return !bDiscarded && !IsActorBeingDestroyed() && HasCurrentMaterials() && IsHidden() && IsValid(Mesh)
            && Mesh->IsRegistered() && Mesh->GetOwner() == this && Mesh->GetWorld() == GetWorld()
            && Mesh->GetFName() == Value.Name && Value.Asset.IsValid()
            && MeshAsset(Value.Component.Get()) == Value.Asset.Get() && !Mesh->bHiddenInGame
            && Mesh->GetVisibleFlag() == bExpectedVisible;
    };
    for (const auto& Value : Resolved)
    {
        if (!Matches(Value, Value.bBaseline)) return false;
        Value.Component->SetVisibility(Value.bTarget, false);
        if (!Matches(Value, Value.bTarget)) return false;
    }
    // A later render-state callback may have changed an earlier occurrence.
    for (const auto& Value : Resolved)
        if (!Matches(Value, Value.bTarget)) return false;
    for (int32 I = 0; I < Resolved.Num(); ++I)
    {
        const auto& Value = Resolved[I];
        auto& Saved = PreparedAccessories.AddDefaulted_GetRef();
        Saved.Component = Value.Component; Saved.Asset = Value.Asset; Saved.Name = Value.Name;
        Saved.Parent = Value.Component->GetAttachParent(); Saved.Socket = Value.Component->GetAttachSocketName();
        Saved.LocalPose = Value.Component->GetRelativeTransform(); Saved.Source = Bindings[I].SourceNode;
        Saved.bBaseline = Value.bBaseline;
    }
    return true;
}

bool ABaddieCompanionCharacterActor::HasCurrentMaterials() const
{
    return PreparedMaterialGeneration != 0 && IsValid(Materials.Get())
        && Materials->GetAppliedGeneration() == PreparedMaterialGeneration;
}

bool ABaddieCompanionCharacterActor::InspectCurrent(const BaddieCompanion::FHomePresentationData& Plan,
    const BaddieCompanion::FNativeCharacterContent& Content, const UWorld* World,
    const AActor* Owner, uint64 MaterialGeneration) const
{
    constexpr uint64 QualifiedCharacterInspectionWork = 0;
    if (!QualifiedCharacterInspectionWork) return false;
    const auto Current = [&]()
    {
        return IsValid(this) && GetWorld() == World && GetOwner() == Owner && IsValid(Owner)
            && World && World->IsGameWorld() && World->GetNetMode() == NM_Standalone
            && bPrepared && !bChangingPresentation && !bDiscarded && !bClearing && !bDestroyRequested
            && !bDisposalBlocked && !IsActorBeingDestroyed() && !PendingApartmentPose.IsSet()
            && PreparedMaterialGeneration == MaterialGeneration && HasCurrentMaterials();
    };
    if (!Current() || !AcceptsContent(Plan, Content) || Content.ActorClass.Get() != GetClass()
        || !FaceAppearance.InspectCurrent(*this, Plan.Appearance, Content.FaceNodes)
        || !Plan.Appearance.Eyewear.Effective.IsSet()
        || !Equipment.InspectCurrent(*this, Content.Equipment,
            Plan.Appearance.Eyewear.Effective.GetValue() == BaddieCompanion::EEyewear::Round)) return false;
    BaddieCompanion::FNativeCharacterMaterialContent Resolved;
    Resolved.SourceAssetKey = Content.SourceAssetKey; Resolved.SourceSha256 = Content.SourceSha256;
    TInlineComponentArray<UMeshComponent*> Meshes(this);
    const auto FindMesh = [&](FName Name) -> UMeshComponent*
    {
        UMeshComponent* Found = nullptr;
        for (auto* Mesh : Meshes)
            if (IsValid(Mesh) && Mesh->GetFName() == Name && Mesh->GetOwner() == this)
            { if (Found) return nullptr; Found = Mesh; }
        return Found;
    };
    for (const auto& Named : Content.Materials)
    {
        auto* Mesh = FindMesh(Named.MeshComponent);
        if (!Mesh) return false;
        auto& Binding = Resolved.Bindings.AddDefaulted_GetRef();
        static_cast<BaddieCompanion::FNativeCharacterMaterialSlot&>(Binding) = Named;
        Binding.Component = Mesh;
    }
    if (!Materials->InspectCurrent(Plan, Resolved, MaterialGeneration)
        || PreparedAccessories.Num() != Content.Accessories.Num() || !Plan.Home.Shell.IsSet()
        || !GetActorScale3D().Equals(FVector(Plan.Home.Shell->CharacterScale), 1.e-6)) return false;
    for (const auto& Binding : Content.Accessories)
    {
        const auto Visible = BaddieCompanion::CharacterAccessories::SelectedVisibility(Plan.Appearance, Binding.SourceNode);
        auto* Mesh = FindMesh(Binding.MeshComponent);
        const auto* Saved = PreparedAccessories.FindByPredicate([&](const auto& Value)
            { return Value.Name == Binding.MeshComponent; });
        if (!Visible.IsSet() || !Binding.VisibleWhenEnabled.IsSet() || !IsValid(Mesh) || !Mesh->IsRegistered()
            || !Saved || Saved->Component.Get() != Mesh || !Saved->Asset.IsValid() || !Saved->Parent.IsValid()
            || !Saved->Source.Equals(Binding.SourceNode, ESearchCase::CaseSensitive)
            || Saved->bBaseline != Binding.VisibleWhenEnabled.GetValue()
            || Saved->Asset.Get() != Binding.MeshAsset.Get() || Mesh->GetAttachParent() != Saved->Parent.Get()
            || Mesh->GetAttachSocketName() != Saved->Socket || Saved->Parent->GetOwner() != this
            || !Saved->Parent->IsRegistered() || Saved->Parent->GetWorld() != World
            || !Mesh->GetRelativeTransform().Equals(Saved->LocalPose, 1.e-6)
            || Mesh->GetMobility() != EComponentMobility::Movable || Mesh->IsUsingAbsoluteLocation()
            || Mesh->IsUsingAbsoluteRotation() || Mesh->IsUsingAbsoluteScale()
            || Mesh->GetWorld() != World || Mesh->bHiddenInGame || !Binding.MeshAsset.IsValid()
            || BaddieCompanion::CharacterAccessories::MeshAsset(Mesh) != Binding.MeshAsset.Get()
            || Mesh->GetVisibleFlag() != (Visible.GetValue() && Binding.VisibleWhenEnabled.GetValue())) return false;
    }
    return Current() && FaceAppearance.InspectCurrent(*this, Plan.Appearance, Content.FaceNodes) && Current();
}

BaddieCompanion::ECharacterEquipmentIssue ABaddieCompanionCharacterActor::ObserveEquipment(
    const BaddieCompanion::FCharacterEquipmentObservation& Observation, bool bRetryFailed)
{
    using BaddieCompanion::ECharacterEquipmentIssue;
    check(IsInGameThread());
    if (bChangingPresentation || !bPrepared || bDiscarded) return ECharacterEquipmentIssue::OwnerUnavailable;
    ECharacterEquipmentIssue Result;
    {
        TGuardValue<bool> Changing(bChangingPresentation, true);
        const auto Current = [this]()
        {
            return !bDiscarded && !IsActorBeingDestroyed() && IsHidden() && HasCurrentMaterials()
                && FaceAppearance.Matches(*this);
        };
        Result = Equipment.Observe(*this, Observation, bRetryFailed, Current);
        if (!Current()) Result = ECharacterEquipmentIssue::OwnerUnavailable;
    }
    if (Result == ECharacterEquipmentIssue::OwnerUnavailable) ClearPrivateCharacter();
    return Result;
}

bool ABaddieCompanionCharacterActor::PollEquipment()
{
    check(IsInGameThread());
    if (bChangingPresentation || !bPrepared || bDiscarded) return false;
    bool bCurrent;
    {
        TGuardValue<bool> Changing(bChangingPresentation, true);
        const auto Current = [this]()
        {
            return !bDiscarded && !IsActorBeingDestroyed() && IsHidden() && HasCurrentMaterials()
                && FaceAppearance.Matches(*this);
        };
        bCurrent = Equipment.Poll(*this, Current) && Current();
    }
    if (!bCurrent) ClearPrivateCharacter();
    return bCurrent;
}

BaddieCompanion::FCharacterEquipmentStatus ABaddieCompanionCharacterActor::EquipmentStatus() const
{
    // A native callback must not observe a half-committed attachment/visibility transition.
    return !bChangingPresentation && bPrepared && !bDiscarded ? Equipment.Status()
        : BaddieCompanion::FCharacterEquipmentStatus();
}

bool ABaddieCompanionCharacterActor::ReconcileLevelTransform()
{
    if (bChangingPresentation || !bPrepared || bDiscarded || IsActorBeingDestroyed()
        || !HasCurrentMaterials() || !FaceAppearance.Matches(*this)) return false;
    TGuardValue<bool> Changing(bChangingPresentation, true);
    if (!PendingApartmentPose.IsSet()) return true;
    auto* Level = GetLevel();
    auto* Root = GetRootComponent();
    if (!IsValid(Level) || !IsValid(Root)) return false;
    if (!Level->bAlreadyMovedActors) return true;
    const FTransform Expected = PendingApartmentPose.GetValue();
    Root->UpdateComponentToWorld(EUpdateTransformFlags::None, ETeleportType::TeleportPhysics);
    if (bDiscarded || IsActorBeingDestroyed() || !HasCurrentMaterials()
        || !SetActorTransform(Expected, false, nullptr, ETeleportType::TeleportPhysics)
        || bDiscarded || IsActorBeingDestroyed() || !HasCurrentMaterials()) return false;
    UpdateComponentTransforms();
    if (bDiscarded || IsActorBeingDestroyed() || !HasCurrentMaterials()
        || !FaceAppearance.Matches(*this) || !GetActorTransform().Equals(Expected, 1.e-4)) return false;
    PendingApartmentPose.Reset();
    return true;
}

void ABaddieCompanionCharacterActor::ClearPrivateCharacter()
{
    check(IsInGameThread());
    bDiscarded = true; bPrepared = false; PreparedMaterialGeneration = 0;
    PreparedAccessories.Reset();
    Equipment.Cancel();
    PendingApartmentPose.Reset();
    if (bClearing) { QueueDeferredDisposal(); return; }
    TGuardValue<bool> Clearing(bClearing, true);
    SuppressPresentation();
    if (IsValid(Materials.Get())) Materials->ClearPrivateMaterials();
    // Restoration also runs outside Prepare, including component unregistration. Neither meshes
    // nor their actor may be disposed from inside an active native setter or suppression callback.
    if (bChangingPresentation || (IsValid(Materials.Get()) && Materials->IsChangingMaterials()))
    { QueueDeferredDisposal(); return; }
    {
        TGuardValue<bool> Changing(bChangingPresentation, true);
        Equipment.Clear(*this);
        FaceAppearance.Clear(*this);
    }
    TInlineComponentArray<UMeshComponent*> Meshes(this);
    for (auto* Mesh : Meshes)
    {
        if (!IsValid(Mesh)) continue;
        const TWeakObjectPtr<UMeshComponent> Current(Mesh);
        Mesh->EmptyOverrideMaterials();
        if (!Current.IsValid()) continue;
        if (auto* Static = Cast<UStaticMeshComponent>(Current.Get())) Static->SetStaticMesh(nullptr);
        else if (auto* Skinned = Cast<USkinnedMeshComponent>(Current.Get())) Skinned->SetSkinnedAssetAndUpdate(nullptr, false);
    }
}

void ABaddieCompanionCharacterActor::QueueDeferredDisposal()
{
    bDisposalBlocked = true;
    if (!DeferredDisposal.IsValid())
        DeferredDisposal = FTSTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateUObject(this, &ABaddieCompanionCharacterActor::FinishDeferredDisposal));
}

bool ABaddieCompanionCharacterActor::DestroyPrivateCharacter()
{
    check(IsInGameThread());
    bDestroyRequested = true;
    ClearPrivateCharacter();
    // The weak core ticker holds no snapshot and is independent of the world's timers.
    // The pending result means disposal was requested, not that destruction has completed.
    if (DeferredDisposal.IsValid()) return true;
    return IsActorBeingDestroyed() || Destroy(false, false);
}

bool ABaddieCompanionCharacterActor::FinishDeferredDisposal(float)
{
    check(IsInGameThread());
    bDisposalBlocked = false;
    ClearPrivateCharacter();
    // Reschedule the same entry if blocked. Adding a new zero-delay ticker from a ticker can
    // execute it again in the same core tick, so never implement this retry by re-enqueueing.
    if (bDisposalBlocked) return true;
    DeferredDisposal.Reset();
    if (bDestroyRequested && !IsActorBeingDestroyed())
    {
        const bool bDestroyed = Destroy(false, false);
        ensureMsgf(bDestroyed, TEXT("Discarded Companion actor could not be destroyed."));
    }
    return false;
}

void ABaddieCompanionCharacterActor::EndPlay(const EEndPlayReason::Type Reason)
{
    ClearPrivateCharacter();
    Super::EndPlay(Reason);
}
