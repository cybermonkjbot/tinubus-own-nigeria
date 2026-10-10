#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "GameFramework/Actor.h"
#include "BaddieCompanionCharacterMaterials.h"
#include "BaddieCompanionCharacterAppearance.h"
#include "BaddieCompanionCharacterEquipment.h"
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
    struct FNativeCharacterAccessoryMesh
    {
        // Exact source node: AccessoryNecklace or AccessoryChain. Not equipment/eyewear.
        FString SourceNode;
        FName MeshComponent;
        TSoftObjectPtr<UObject> MeshAsset;
        // Accepted source occurrence visibility with the accessory root enabled. Include
        // hidden descendants explicitly; UE component visibility is not Three group visibility.
        TOptional<bool> VisibleWhenEnabled;
    };
    struct FNativeCharacterContent
    {
        FString SourceAssetKey, SourceSha256;
        // Trusted authored component templates, including imported mesh/skin/morph hierarchy.
        // Root origin/basis match the source runtime rig; room scale is applied by the actor.
        TSoftClassPtr<ABaddieCompanionCharacterActor> ActorClass;
        TArray<FNativeCharacterNamedMaterial> Materials;
        // Every imported mesh occurrence under both source accessory roots, including hidden ones.
        TArray<FNativeCharacterAccessoryMesh> Accessories;
        // Eight source face transform roots and their complete accepted drawable subtrees.
        TArray<FNativeCharacterFaceNode> FaceNodes;
        FNativeCharacterEquipmentContent Equipment;
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
    bool HasCurrentMaterials() const;
    BaddieCompanion::ECharacterEquipmentIssue ObserveEquipment(
        const BaddieCompanion::FCharacterEquipmentObservation& Observation, bool bRetryFailed);
    bool PollEquipment();
    BaddieCompanion::FCharacterEquipmentStatus EquipmentStatus() const;
    bool PrepareAccessoryVisibility(const BaddieCompanion::FAppearancePlan& Appearance,
        const TArray<BaddieCompanion::FNativeCharacterAccessoryMesh>& Bindings);
    void SuppressPresentation();
    bool DestroyPrivateCharacter();
    void QueueDeferredDisposal();
    bool FinishDeferredDisposal(float DeltaSeconds);

    UPROPERTY(Transient)
    TObjectPtr<UBaddieCompanionCharacterMaterials> Materials;
    BaddieCompanion::FNativeCharacterFaceAppearance FaceAppearance;
    BaddieCompanion::FNativeCharacterEquipment Equipment;
    TOptional<FTransform> PendingApartmentPose;
    uint64 PreparedMaterialGeneration = 0;
    FTSTicker::FDelegateHandle DeferredDisposal;
    bool bPrepared = false;
    bool bChangingPresentation = false;
    bool bDiscarded = false;
    bool bClearing = false;
    bool bDestroyRequested = false;
    bool bDisposalBlocked = false;
};
