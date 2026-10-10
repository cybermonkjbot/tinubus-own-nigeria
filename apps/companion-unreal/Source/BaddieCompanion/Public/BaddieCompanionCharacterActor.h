#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaddieCompanionCharacterMaterials.h"
#include "BaddieCompanionCharacterActor.generated.h"

class ABaddieCompanionCharacterActor;
class UBaddieCompanionNativeApartment;

namespace BaddieCompanion
{
    struct FNativeCharacterNamedMaterial : FNativeCharacterMaterialSlot
    {
        // Actual component name in the accepted native class, never a saved document field.
        FName MeshComponent;
    };
    struct FNativeCharacterContent
    {
        FString SourceAssetKey, SourceSha256;
        // Trusted authored component templates, including imported mesh/skin/morph hierarchy.
        // Root origin/basis match the source runtime rig; room scale is applied by the actor.
        TSoftClassPtr<ABaddieCompanionCharacterActor> ActorClass;
        TArray<FNativeCharacterNamedMaterial> Materials;
    };
}

/** Private visual content only. Native ACharacter movement/possession remains with its future owner.
 * Authored subclasses supply the imported hierarchy, not gameplay or construction-script actions.
 */
UCLASS(Blueprintable, Transient)
class BADDIECOMPANION_API ABaddieCompanionCharacterActor : public AActor
{
    GENERATED_BODY()
public:
    ABaddieCompanionCharacterActor();
    static bool AcceptsContent(const BaddieCompanion::FHomePresentationPlan& Plan,
        const BaddieCompanion::FNativeCharacterContent& Content);
    void ClearPrivateCharacter();
protected:
    virtual void PostInitializeComponents() override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    friend class UBaddieCompanionNativeApartment;
    static bool HasInertDefaults(UClass* Class);
    bool Prepare(const BaddieCompanion::FHomePresentationPlan& Plan,
        const BaddieCompanion::FNativeCharacterContent& Content,
        const FTransform& LevelToApartment, bool bLevelAlreadyTransformed);
    bool PrepareBindings(const BaddieCompanion::FHomePresentationPlan& Plan,
        const BaddieCompanion::FNativeCharacterContent& Content,
        const FTransform& LevelToApartment, bool bLevelAlreadyTransformed);
    bool ReconcileLevelTransform();
    void SuppressPresentation();

    UPROPERTY(Transient)
    TObjectPtr<UBaddieCompanionCharacterMaterials> Materials;
    TOptional<FTransform> PendingApartmentPose;
    bool bPrepared = false;
    bool bPreparing = false;
    bool bDiscarded = false;
    bool bClearing = false;
};
