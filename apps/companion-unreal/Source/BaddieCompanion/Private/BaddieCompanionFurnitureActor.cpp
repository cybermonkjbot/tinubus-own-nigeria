#include "BaddieCompanionFurnitureActor.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Level.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

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
    ClearPrivateFurniture();
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
        || !FurnitureMesh->SetStaticMesh(Asset.Mesh.Get())) return false;
    if (!SetActorTransform(Transform, false, nullptr, ETeleportType::TeleportPhysics))
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
        if (!BulbMaterial) { ClearPrivateFurniture(); return false; }
        BulbMaterial->SetScalarParameterValue(Asset.EmissiveIntensityParameter, Item.bEffectiveLit ? .6f : 0.f);
    }

    if (Asset.Kind == EItemKind::Lamp || FurnitureActor::HasEmissiveBulb(Asset.Kind))
    {
        const bool bTableLamp = Asset.Kind == EItemKind::Lamp;
        const double SourceHeight = bTableLamp ? .65 : Asset.Kind == EItemKind::FloorLamp ? 2.75 : .65;
        PracticalLight->SetRelativeLocation(SourceToUnreal(FVector(0, SourceHeight, 0)));
        PracticalLight->SetIntensityUnits(ELightUnits::Candelas);
        PracticalLight->SetUseInverseSquaredFalloff(true);
        PracticalLight->SetIntensity(Item.bEffectiveLit ? (bTableLamp ? 1.8f : 1.1f) : 0.f);
        PracticalLight->SetLightColor(FLinearColor(FColor(255, 211, 155)));
        // Three's distance is world-space and does not inherit the furniture group's scale.
        PracticalLight->SetAttenuationRadius(bTableLamp ? 210.f : 240.f);
    }
    RoomId = Room.Id;
    ItemId = Item.Id;
    if (!bLevelAlreadyTransformed) PendingApartmentPose = ApartmentTransform;
    // Prepared content remains hidden/non-colliding. Native admission/render acceptance still owns activation.
    return true;
}

bool ABaddieCompanionFurnitureActor::ReconcileLevelTransform()
{
    if (!PendingApartmentPose.IsSet()) return true;
    auto* Level = GetLevel();
    if (!IsValid(Level) || !IsValid(FurnitureMesh) || !IsValid(PracticalLight)) return false;
    if (!Level->bAlreadyMovedActors) return true;
    // UE moves relative properties directly when showing a level. SpawnActor may already have
    // registered this actor, so refresh its cache before using the normal world-transform setter.
    FurnitureMesh->UpdateComponentToWorld(EUpdateTransformFlags::None, ETeleportType::TeleportPhysics);
    if (!SetActorTransform(PendingApartmentPose.GetValue(), false, nullptr, ETeleportType::TeleportPhysics)) return false;
    UpdateComponentTransforms();
    if (!GetActorTransform().Equals(PendingApartmentPose.GetValue(), 1.e-4)) return false;
    PendingApartmentPose.Reset();
    return true;
}

void ABaddieCompanionFurnitureActor::ClearPrivateFurniture()
{
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    SetActorTickEnabled(false);
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
    RoomId.Reset(); ItemId.Reset();
    PendingApartmentPose.Reset();
}

void ABaddieCompanionFurnitureActor::EndPlay(const EEndPlayReason::Type Reason)
{
    ClearPrivateFurniture();
    Super::EndPlay(Reason);
}
