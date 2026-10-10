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
    if (!bNecklace || !bHipChain) return false;
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

bool ABaddieCompanionCharacterActor::Prepare(const BaddieCompanion::FHomePresentationPlan& Plan,
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

bool ABaddieCompanionCharacterActor::PrepareBindings(const BaddieCompanion::FHomePresentationPlan& Plan,
    const BaddieCompanion::FNativeCharacterContent& Content,
    const FTransform& LevelToApartment, bool bLevelAlreadyTransformed)
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
    const auto& Spawn = Shell.PreferredSpawn;
    if (!FMath::IsFinite(Shell.CharacterScale) || Shell.CharacterScale <= 0
        || Spawn.UnrealCentimetres.ContainsNaN() || !FMath::IsFinite(Spawn.UnrealYawDegrees)) return false;
    const FTransform LocalPose(FRotator(0, Spawn.UnrealYawDegrees, 0),
        Spawn.UnrealCentimetres, FVector(Shell.CharacterScale));
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
    if (!Materials->Apply(Plan, Resolved) || bDiscarded || IsActorBeingDestroyed()) return false;
    if (!PrepareAccessoryVisibility(Plan.Appearance, Content.Accessories)) return false;
    if (!bLevelAlreadyTransformed) PendingApartmentPose = ApartmentPose;
    return true; // Hidden material/accessory preparation, not complete appearance or movement readiness.
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
        return !bDiscarded && !IsActorBeingDestroyed() && IsHidden() && IsValid(Mesh)
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
    return true;
}

bool ABaddieCompanionCharacterActor::ReconcileLevelTransform()
{
    if (bChangingPresentation || !bPrepared || bDiscarded || IsActorBeingDestroyed()) return false;
    TGuardValue<bool> Changing(bChangingPresentation, true);
    if (!PendingApartmentPose.IsSet()) return true;
    auto* Level = GetLevel();
    auto* Root = GetRootComponent();
    if (!IsValid(Level) || !IsValid(Root)) return false;
    if (!Level->bAlreadyMovedActors) return true;
    const FTransform Expected = PendingApartmentPose.GetValue();
    Root->UpdateComponentToWorld(EUpdateTransformFlags::None, ETeleportType::TeleportPhysics);
    if (bDiscarded || IsActorBeingDestroyed()
        || !SetActorTransform(Expected, false, nullptr, ETeleportType::TeleportPhysics)
        || bDiscarded || IsActorBeingDestroyed()) return false;
    UpdateComponentTransforms();
    if (bDiscarded || IsActorBeingDestroyed() || !GetActorTransform().Equals(Expected, 1.e-4)) return false;
    PendingApartmentPose.Reset();
    return true;
}

void ABaddieCompanionCharacterActor::ClearPrivateCharacter()
{
    check(IsInGameThread());
    bDiscarded = true; bPrepared = false;
    PendingApartmentPose.Reset();
    if (bClearing) { QueueDeferredDisposal(); return; }
    TGuardValue<bool> Clearing(bClearing, true);
    SuppressPresentation();
    if (IsValid(Materials.Get())) Materials->ClearPrivateMaterials();
    // Restoration also runs outside Prepare, including component unregistration. Neither meshes
    // nor their actor may be disposed from inside an active native setter or suppression callback.
    if (bChangingPresentation || (IsValid(Materials.Get()) && Materials->IsChangingMaterials()))
    { QueueDeferredDisposal(); return; }
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
