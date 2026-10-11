#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BaddieCompanionHomePresentation.h"
#include "BaddieCompanionCharacterMaterials.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UMeshComponent;

namespace BaddieCompanion
{
    enum class ENativeCharacterColorRecipe : uint8 { BaseColorFactor, IrisLuminanceTint };
    enum class ENativeCharacterMaterialIssue : uint8
    {
        None, OwnerUnavailable, SourceMismatch, InvalidBinding, UnsupportedRecipe,
        ParameterUnavailable, MaterialChanged, InstanceUnavailable
    };
    struct FNativeCharacterMaterialSlot
    {
        // Trusted imported content, not names or paths derived from a saved appearance.
        FString SourceMaterialName;
        TSoftObjectPtr<UObject> MeshAsset;
        TSoftObjectPtr<UMaterialInterface> AuthoredMaterial;
        FName MaterialSlot, ColorParameter, IrisEnabledParameter;
        // BaseColorFactor replaces the source material factor, not its texture.
        // IrisLuminanceTint requires the source luminance formula in the native material graph.
        ENativeCharacterColorRecipe Recipe = ENativeCharacterColorRecipe::BaseColorFactor;
    };
    struct FNativeCharacterMaterialBinding : FNativeCharacterMaterialSlot
    {
        TWeakObjectPtr<UMeshComponent> Component;
    };
    struct FNativeCharacterMaterialContent
    {
        FString SourceAssetKey, SourceSha256;
        // The imported manifest must cover every source occurrence. Apply checks supplied bindings,
        // but cannot discover source occurrences omitted by the content author.
        TArray<FNativeCharacterMaterialBinding> Bindings;
    };
}

USTRUCT()
struct FBaddieOwnedCharacterMaterial
{
    GENERATED_BODY()
    UPROPERTY(Transient)
    TWeakObjectPtr<UMeshComponent> Component;
    UPROPERTY(Transient)
    TWeakObjectPtr<UObject> MeshAsset;
    UPROPERTY(Transient)
    TObjectPtr<UMaterialInterface> PreviousOverride;
    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> Override;
    int32 Slot = INDEX_NONE;
    FName SlotName;
};

/** Game-thread, avatar-owned material overrides. No tick, load, pose edits or gameplay authority.
 * The admitted presentation owner supplies a current plan and complete imported bindings,
 * then clears on selection/access loss and before replacing the avatar's native meshes.
 * Recursive Apply fails without mutation. Clear during a mesh callback cancels the current Apply.
 */
UCLASS(NotBlueprintable, Transient)
class BADDIECOMPANION_API UBaddieCompanionCharacterMaterials : public UActorComponent
{
    GENERATED_BODY()
public:
    UBaddieCompanionCharacterMaterials();
    bool Apply(const BaddieCompanion::FHomePresentationPlan& Plan,
        const BaddieCompanion::FNativeCharacterMaterialContent& Content);
    bool Apply(const BaddieCompanion::FHomePresentationData& Plan,
        const BaddieCompanion::FNativeCharacterMaterialContent& Content);
    void ClearPrivateMaterials();
    // Game-thread lifetime guard, including restoration from EndPlay/OnUnregister.
    bool IsChangingMaterials() const { return bChangingMaterials; }
    // Zero means unavailable. Each successful Apply has a distinct identity; clear/unregister
    // invalidates it immediately, even when the component subsequently registers again.
    uint64 GetAppliedGeneration() const;
    BaddieCompanion::ENativeCharacterMaterialIssue GetIssue() const { return Issue; }
protected:
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void OnUnregister() override;
private:
    void RestoreOwnedMaterials();
    UPROPERTY(Transient)
    TArray<FBaddieOwnedCharacterMaterial> Owned;
    BaddieCompanion::ENativeCharacterMaterialIssue Issue = BaddieCompanion::ENativeCharacterMaterialIssue::None;
    uint64 ApplicationGeneration = 0;
    uint64 AppliedGeneration = 0;
    bool bEndingPlay = false;
    bool bChangingMaterials = false;
    bool bClearRequested = false;
};
