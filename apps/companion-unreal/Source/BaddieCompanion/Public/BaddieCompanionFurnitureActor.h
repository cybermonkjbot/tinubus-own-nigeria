#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaddieCompanionHomePresentation.h"
#include "BaddieCompanionFurnitureActor.generated.h"

class UMaterialInstanceDynamic;
class UPointLightComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UBaddieCompanionNativeApartment;

namespace BaddieCompanion
{
    struct FNativeFurnitureAsset
    {
        EItemKind Kind = EItemKind::Lamp;
        // Trusted native content only. Mesh uses the source prop's bottom pivot and catalog scale,
        // converted to UE cm. Room scale, saved placement and apartment transform are not baked in.
        TSoftObjectPtr<UStaticMesh> Mesh;
        FString SourceGeometryRecipe;
        // Floor/desk lamp bulb only: actual native slot and scalar parameter, never guessed names.
        // The parameter represents source emissive intensity; its on value is 0.6.
        FName EmissiveMaterialSlot, EmissiveIntensityParameter;
    };
}

/** One saved prop, owned by the native apartment. No Blueprint, tick, replication or saved-state writes. */
UCLASS(NotBlueprintable, Transient)
class BADDIECOMPANION_API ABaddieCompanionFurnitureActor : public AActor
{
    GENERATED_BODY()
public:
    ABaddieCompanionFurnitureActor();
protected:
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    friend class UBaddieCompanionNativeApartment;
    static bool AcceptsBinding(const BaddieCompanion::FFurniturePlan& Item,
        const BaddieCompanion::FNativeFurnitureAsset& Asset);
    bool Configure(const BaddieCompanion::FRoomPlan& Room, const BaddieCompanion::FFurniturePlan& Item,
        const BaddieCompanion::FNativeFurnitureAsset& Asset, const FTransform& LevelToApartment,
        bool bLevelAlreadyTransformed);
    bool ReconcileLevelTransform();
    void ClearPrivateFurniture();

    UPROPERTY(Transient)
    TObjectPtr<UStaticMeshComponent> FurnitureMesh;
    UPROPERTY(Transient)
    TObjectPtr<UPointLightComponent> PracticalLight;
    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> BulbMaterial;
    // Exact saved identity stays private and is not used as an actor/object/package name.
    FString RoomId, ItemId;
    // Derived from saved local pose, not the actor's potentially stale cached world transform.
    TOptional<FTransform> PendingApartmentPose;
};
