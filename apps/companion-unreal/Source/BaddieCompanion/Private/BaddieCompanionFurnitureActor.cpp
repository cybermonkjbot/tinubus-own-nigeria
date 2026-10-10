#include "BaddieCompanionFurnitureActor.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Level.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Templates/UnrealTemplate.h"

namespace BaddieCompanion::FurnitureActor
{
    bool HasEmissiveBulb(EItemKind Kind)
    {
        return Kind == EItemKind::FloorLamp || Kind == EItemKind::DeskLamp;
    }
}

ABaddieCompanionFurnitureActor::ABaddieCompanionFurnitureActor()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = false;
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    FurnitureMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FurnitureMesh"));
    SetRootComponent(FurnitureMesh);
    FurnitureMesh->SetMobility(EComponentMobility::Movable);
    FurnitureMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FurnitureMesh->SetSimulatePhysics(false);
    FurnitureMesh->SetVisibility(false);
    PracticalLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PracticalLight"));
    PracticalLight->SetupAttachment(FurnitureMesh);
    PracticalLight->SetMobility(EComponentMobility::Movable);
    PracticalLight->SetVisibility(false);
    PracticalLight->SetIntensity(0);
    PracticalLight->SetCastShadows(false);
}

bool ABaddieCompanionFurnitureActor::AcceptsBinding(const BaddieCompanion::FFurniturePlan& Item,
    const BaddieCompanion::FNativeFurnitureAsset& Asset)
{
    if (!Item.Kind.Effective.IsSet() || Item.Kind.Effective.GetValue() != Asset.Kind || Asset.Mesh.IsNull()
        || Item.SourceGeometryRecipe.IsEmpty()
        || !Item.SourceGeometryRecipe.Equals(Asset.SourceGeometryRecipe, ESearchCase::CaseSensitive)) return false;
    const bool bEmissive = BaddieCompanion::FurnitureActor::HasEmissiveBulb(Asset.Kind);
    return bEmissive ? !Asset.EmissiveMaterialSlot.IsNone() && !Asset.EmissiveIntensityParameter.IsNone()
        : Asset.EmissiveMaterialSlot.IsNone() && Asset.EmissiveIntensityParameter.IsNone();
}

bool ABaddieCompanionFurnitureActor::Configure(const BaddieCompanion::FRoomPlan& Room,
    const BaddieCompanion::FFurniturePlan& Item, const BaddieCompanion::FNativeFurnitureAsset& Asset,
    const FTransform& LevelToApartment, bool bLevelAlreadyTransformed)
{
    check(IsInGameThread());
    // Each saved-content generation spawns a new actor. Never revive a discarded instance.
    if (bConfigured || bDiscarded || bChangingPresentation || IsActorBeingDestroyed()) return false;
    bool bSuccess = false;
    {
        TGuardValue<bool> Changing(bChangingPresentation, true);
        bSuccess = ConfigureBindings(Room, Item, Asset, LevelToApartment, bLevelAlreadyTransformed);
    }
    if (!bSuccess || bDiscarded || IsActorBeingDestroyed()) { ClearPrivateFurniture(); return false; }
    bConfigured = true;
    return true;
}

bool ABaddieCompanionFurnitureActor::ConfigureBindings(const BaddieCompanion::FRoomPlan& Room,
    const BaddieCompanion::FFurniturePlan& Item, const BaddieCompanion::FNativeFurnitureAsset& Asset,
    const FTransform& LevelToApartment, bool bLevelAlreadyTransformed)
{
    using namespace BaddieCompanion;
    if (!AcceptsBinding(Item, Asset) || Room.Id.IsEmpty() || Item.Id.IsEmpty()
        || !Room.bSourceMapped || !Room.Shell.IsSet() || !Item.PlacementPose.IsSet()
        || !Item.Dimensions.IsSet() || !Item.Volume.IsSet() || !IsValid(Asset.Mesh.Get())) return false;
    const double Scale = Room.Shell->ObjectScale;
    const auto& Placement = Item.PlacementPose.GetValue();
    if (!FMath::IsFinite(Scale) || Scale <= 0 || Placement.UnrealCentimetres.ContainsNaN()
        || !FMath::IsFinite(Placement.UnrealYawDegrees) || LevelToApartment.ContainsNaN()
        || !LevelToApartment.GetRotation().IsNormalized()
        || !LevelToApartment.GetScale3D().Equals(FVector::OneVector, 1.e-6)
        || !LevelToApartment.GetRotation().RotateVector(FVector::UpVector).Equals(FVector::UpVector, 1.e-6)) return false;

    const FTransform LocalTransform(FRotator(0, Placement.UnrealYawDegrees, 0), Placement.UnrealCentimetres, FVector(Scale));
    const FTransform ApartmentTransform = LocalTransform * LevelToApartment;
    // Saved furniture is already in designed room coordinates. Do not turn it with the shell again.
    const FTransform& Transform = bLevelAlreadyTransformed ? ApartmentTransform : LocalTransform;
    if (LocalTransform.ContainsNaN() || ApartmentTransform.ContainsNaN()
        || !FurnitureMesh->SetStaticMesh(Asset.Mesh.Get()) || bDiscarded) return false;
    if (!SetActorTransform(Transform, false, nullptr, ETeleportType::TeleportPhysics) || bDiscarded)
    { ClearPrivateFurniture(); return false; }

    if (FurnitureActor::HasEmissiveBulb(Asset.Kind))
    {
        int32 MatchingSlots = 0;
        for (const FName SlotName : FurnitureMesh->GetMaterialSlotNames())
            if (SlotName == Asset.EmissiveMaterialSlot) ++MatchingSlots;
        if (MatchingSlots != 1) { ClearPrivateFurniture(); return false; }
        const int32 Slot = FurnitureMesh->GetMaterialIndex(Asset.EmissiveMaterialSlot);
        auto* Material = Slot == INDEX_NONE ? nullptr : FurnitureMesh->GetMaterial(Slot);
        float Current = 0;
        if (!Material || !Material->GetScalarParameterValue(FMaterialParameterInfo(Asset.EmissiveIntensityParameter), Current)
            || !FMath::IsFinite(Current))
        { ClearPrivateFurniture(); return false; }
        BulbMaterial = FurnitureMesh->CreateDynamicMaterialInstance(Slot);
        if (!BulbMaterial || bDiscarded) { ClearPrivateFurniture(); return false; }
        EmissiveParameter = Asset.EmissiveIntensityParameter;
        SavedEmissiveIntensity = Item.bEffectiveLit ? .6f : 0.f;
        BulbMaterial->SetScalarParameterValue(EmissiveParameter, 0.f);
        if (bDiscarded) return false;
    }

    if (Asset.Kind == EItemKind::Lamp || FurnitureActor::HasEmissiveBulb(Asset.Kind))
    {
        const bool bTableLamp = Asset.Kind == EItemKind::Lamp;
        const double SourceHeight = bTableLamp ? .65 : Asset.Kind == EItemKind::FloorLamp ? 2.75 : .65;
        PracticalLight->SetRelativeLocation(SourceToUnreal(FVector(0, SourceHeight, 0)));
        PracticalLight->SetIntensityUnits(ELightUnits::Candelas);
        PracticalLight->SetUseInverseSquaredFalloff(true);
        SavedLightIntensity = Item.bEffectiveLit ? (bTableLamp ? 1.8f : 1.1f) : 0.f;
        PracticalLight->SetIntensity(0.f);
        PracticalLight->SetLightColor(FLinearColor(FColor(255, 211, 155)));
        // Three's distance is world-space and does not inherit the furniture group's scale.
        PracticalLight->SetAttenuationRadius(bTableLamp ? 210.f : 240.f);
    }
    if (bDiscarded || IsActorBeingDestroyed()) return false;
    RoomId = Room.Id;
    ItemId = Item.Id;
    if (!bLevelAlreadyTransformed) PendingApartmentPose = ApartmentTransform;
    // Prepared content remains hidden/non-colliding. Native admission/render acceptance still owns activation.
    return true;
}

bool ABaddieCompanionFurnitureActor::ReconcileLevelTransform()
{
    if (!bConfigured || bDiscarded || bChangingPresentation) return false;
    TGuardValue<bool> Changing(bChangingPresentation, true);
    if (!PendingApartmentPose.IsSet()) return true;
    auto* Level = GetLevel();
    if (!IsValid(Level) || !IsValid(FurnitureMesh) || !IsValid(PracticalLight)) return false;
    if (!Level->bAlreadyMovedActors) return true;
    // UE moves relative properties directly when showing a level. SpawnActor may already have
    // registered this actor, so refresh its cache before using the normal world-transform setter.
    FurnitureMesh->UpdateComponentToWorld(EUpdateTransformFlags::None, ETeleportType::TeleportPhysics);
    if (bDiscarded || !PendingApartmentPose.IsSet()) return false;
    const FTransform Expected = PendingApartmentPose.GetValue();
    if (!SetActorTransform(Expected, false, nullptr, ETeleportType::TeleportPhysics) || bDiscarded) return false;
    UpdateComponentTransforms();
    if (bDiscarded || !GetActorTransform().Equals(Expected, 1.e-4)) return false;
    PendingApartmentPose.Reset();
    return true;
}

bool ABaddieCompanionFurnitureActor::ApplyScenePower(float Factor)
{
    check(IsInGameThread());
    if (!bConfigured || bDiscarded || bChangingPresentation || IsActorBeingDestroyed() || !FMath::IsFinite(Factor)
        || Factor < 0 || Factor > 1 || !IsValid(PracticalLight) || !IsValid(FurnitureMesh))
    { SuppressPower(); return false; }
    TGuardValue<bool> Changing(bChangingPresentation, true);
    const uint64 Generation = PowerGeneration;
    // Utility acceptance never activates the actor, mesh, level or collision.
    const bool bVisible = !IsHidden() && FurnitureMesh->IsVisible() && !PendingApartmentPose.IsSet();
    const float Output = bVisible ? Factor : 0.f;
    if (LastAppliedPower.IsSet() && LastAppliedPower.GetValue() == Output) return true;
    if (BulbMaterial) BulbMaterial->SetScalarParameterValue(EmissiveParameter, SavedEmissiveIntensity * Output);
    if (bDiscarded || Generation != PowerGeneration) { SuppressPower(); return false; }
    PracticalLight->SetIntensity(SavedLightIntensity * Output);
    if (bDiscarded || Generation != PowerGeneration) { SuppressPower(); return false; }
    PracticalLight->SetVisibility(SavedLightIntensity * Output > 0.f);
    if (bDiscarded || Generation != PowerGeneration) { SuppressPower(); return false; }
    LastAppliedPower = Output;
    return true;
}

void ABaddieCompanionFurnitureActor::SuppressPower()
{
    if (PowerGeneration == MAX_uint64) bDiscarded = true;
    ++PowerGeneration;
    LastAppliedPower.Reset();
    if (bSuppressingPower) return;
    TGuardValue<bool> Suppressing(bSuppressingPower, true);
    TGuardValue<bool> Changing(bChangingPresentation, true);
    if (PracticalLight)
    {
        PracticalLight->SetVisibility(false);
        PracticalLight->SetIntensity(0);
    }
    if (BulbMaterial && !EmissiveParameter.IsNone())
        BulbMaterial->SetScalarParameterValue(EmissiveParameter, 0.f);
}

void ABaddieCompanionFurnitureActor::ClearPrivateFurniture()
{
    bDiscarded = true; bConfigured = false;
    RoomId.Reset(); ItemId.Reset(); PendingApartmentPose.Reset();
    SavedLightIntensity = 0; SavedEmissiveIntensity = 0;
    if (bClearing) return;
    TGuardValue<bool> Clearing(bClearing, true);
    SuppressPower();
    {
        TGuardValue<bool> Changing(bChangingPresentation, true);
        SetActorHiddenInGame(true);
        SetActorEnableCollision(false);
        SetActorTickEnabled(false);
    }
    // SetMaterial/transform callbacks can invalidate the owner. Keep mesh disposal and actor
    // destruction outside the active mutation, as with the apartment's CharacterActor.
    if (bChangingPresentation) { QueueDeferredDisposal(); return; }
    TGuardValue<bool> Changing(bChangingPresentation, true);
    if (PracticalLight)
    {
        PracticalLight->SetVisibility(false);
        PracticalLight->SetIntensity(0);
        PracticalLight->Deactivate();
    }
    if (FurnitureMesh)
    {
        FurnitureMesh->SetVisibility(false);
        FurnitureMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        FurnitureMesh->SetSimulatePhysics(false);
        FurnitureMesh->EmptyOverrideMaterials();
        FurnitureMesh->SetStaticMesh(nullptr);
    }
    BulbMaterial = nullptr;
    EmissiveParameter = NAME_None;
}

void ABaddieCompanionFurnitureActor::QueueDeferredDisposal()
{
    bDisposalBlocked = true;
    if (!DeferredDisposal.IsValid())
        DeferredDisposal = FTSTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateUObject(this, &ABaddieCompanionFurnitureActor::FinishDeferredDisposal));
}

bool ABaddieCompanionFurnitureActor::DestroyPrivateFurniture()
{
    check(IsInGameThread());
    bDestroyRequested = true;
    ClearPrivateFurniture();
    if (bChangingPresentation || bClearing) QueueDeferredDisposal();
    if (DeferredDisposal.IsValid()) return true;
    return IsActorBeingDestroyed() || Destroy(false, false);
}

bool ABaddieCompanionFurnitureActor::FinishDeferredDisposal(float)
{
    check(IsInGameThread());
    bDisposalBlocked = false;
    ClearPrivateFurniture();
    if (bDisposalBlocked) return true;
    DeferredDisposal.Reset();
    if (bDestroyRequested && !IsActorBeingDestroyed())
    {
        const bool bDestroyed = Destroy(false, false);
        ensureMsgf(bDestroyed, TEXT("Discarded furniture actor could not be destroyed."));
    }
    return false;
}

void ABaddieCompanionFurnitureActor::EndPlay(const EEndPlayReason::Type Reason)
{
    ClearPrivateFurniture();
    Super::EndPlay(Reason);
}
