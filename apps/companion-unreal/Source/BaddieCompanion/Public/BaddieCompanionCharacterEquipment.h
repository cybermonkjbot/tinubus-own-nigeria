#pragma once

#include "CoreMinimal.h"
#include "BaddieCompanionSnapshot.h"

class ABaddieCompanionCharacterActor;
class UBaddieCompanionNativeApartment;
class USceneComponent;
class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class FStreamableHandle;

namespace BaddieCompanion
{
    enum class ECharacterEquipmentIssue : uint8
    {
        None, ObservationUnknown, UnsupportedItem, UnsupportedBinding, AssetUnavailable,
        AttachmentUnavailable, GeneratedEyewearUnavailable, OwnerUnavailable, StaleObservation
    };
    struct FCharacterEquipmentObservation
    {
        // Unset is unknown. An explicitly present empty string means an observed empty slot.
        // These are the four object slots from companionShop, not combat equipment or outfits.
        TOptional<FString> Toy, Eyewear, Neckwear, Pin;
    };
    class FAcceptedCharacterEquipment
    {
    private:
        friend class ::UBaddieCompanionNativeApartment;
        FAcceptedCharacterEquipment() = default;
        // No producer exists. A future authorized observation owner must bind this receipt;
        // content setup and Home/apartment read grants cannot mint equipment observations.
        TWeakObjectPtr<UBaddieCompanionNativeApartment> Apartment;
        uint64 ContentGeneration = 0;
        uint64 ObservationSequence = 0; // Local acceptance order, not an invented server revision.
        FSnapshotSelection Source;
        FCharacterEquipmentObservation Observation;
        bool bRetryFailed = false;
    };
    struct FNativeEquipmentMaterial
    {
        int32 SourceMaterialIndex = INDEX_NONE;
        FName Slot;
        TSoftObjectPtr<UMaterialInterface> Material;
    };
    struct FNativeEquipmentMesh
    {
        // Source mesh-node ordinal in the hash-matched GLB; frames use createGlasses order 0..4.
        int32 SourceNodeIndex = INDEX_NONE;
        FName Component;
        TSoftObjectPtr<UStaticMesh> Mesh;
        TOptional<FTransform> LocalPose;
        // Complete imported slot mapping. Use the native asset's materials, never shared overrides.
        TArray<FNativeEquipmentMaterial> Materials;
    };
    struct FNativeEquipmentGroup
    {
        // A plain, identity scene root with direct static mesh children. Other import layouts
        // are unsupported. Item templates start under the character root, with null meshes.
        FName RootComponent, AttachmentComponent, AttachmentSocket;
        // Required when AttachmentComponent is a static/skinned mesh; absent for plain scene nodes.
        TSoftObjectPtr<UObject> AttachmentMeshAsset;
        TArray<FNativeEquipmentMesh> Meshes;
    };
    struct FNativeCharacterEquipmentItem
    {
        FString ItemId, SourceAssetKey, SourcePath, SourceSha256, SourceParent;
        FNativeEquipmentGroup Group;
    };
    struct FNativeGeneratedRoundEyewear
    {
        // Hash of canonical faceRig.ts. No generated geometry or fake native package is supplied.
        FString SourceRecipeSha256;
        FNativeEquipmentGroup Group; // Root is identity under the accepted Head attachment.
    };
    struct FNativeCharacterEquipmentContent
    {
        TArray<FNativeCharacterEquipmentItem> Items;
        TOptional<FNativeGeneratedRoundEyewear> GeneratedRoundFrames;
    };
    struct FCharacterEquipmentSlotStatus
    {
        FString RequestedId, AttachedId;
        bool bLoading = false;
        ECharacterEquipmentIssue Issue = ECharacterEquipmentIssue::ObservationUnknown;
    };
    struct FCharacterEquipmentStatus
    {
        bool bHasAcceptedObservation = false;
        TArray<FCharacterEquipmentSlotStatus> Slots; // toy, eyewear, neckwear, pin
        ECharacterEquipmentIssue GeneratedFrames = ECharacterEquipmentIssue::ObservationUnknown;
    };

    // Actor-owned state; only the actor may mutate it under its presentation/disposal guard.
    // Components own installed meshes. Pending streamable handles retain not-yet-installed assets.
    class FNativeCharacterEquipment
    {
        friend class ::ABaddieCompanionCharacterActor;
        bool Initialize(ABaddieCompanionCharacterActor& Actor, const FNativeCharacterEquipmentContent& Content,
            const TSet<FName>& ProtectedComponents, const TSet<FName>& FaceComponents, bool bWantsRoundFrames);
        ECharacterEquipmentIssue Observe(ABaddieCompanionCharacterActor& Actor,
            const FCharacterEquipmentObservation& Observation, bool bRetryFailed, TFunctionRef<bool()> IsCurrent);
        bool Poll(ABaddieCompanionCharacterActor& Actor, TFunctionRef<bool()> IsCurrent);
        void Cancel(); // Immediate cancellation only; retains borrowed setter storage until Clear.
        void Clear(ABaddieCompanionCharacterActor& Actor);
        FCharacterEquipmentStatus Status() const;
        bool InspectCurrent(const ABaddieCompanionCharacterActor& Actor) const;

        struct FMeshState
        {
            FNativeEquipmentMesh Binding;
            TWeakObjectPtr<UStaticMeshComponent> Component;
            bool bAssigned = false, bVisible = false;
        };
        struct FGroupState
        {
            FString ItemId;
            FNativeEquipmentGroup Binding;
            TWeakObjectPtr<USceneComponent> Root, Anchor;
            TArray<TWeakObjectPtr<USceneComponent>> AnchorChain;
            FTransform Target = FTransform::Identity;
            FTransform Pose = FTransform::Identity;
            TArray<FMeshState> Meshes;
            bool bAtAnchor = false, bGenerated = false;
        };
        struct FSlotState
        {
            TOptional<FString> Requested;
            int32 Attached = INDEX_NONE, Pending = INDEX_NONE;
            TSharedPtr<FStreamableHandle> Load;
            ECharacterEquipmentIssue Issue = ECharacterEquipmentIssue::ObservationUnknown;
        };
        bool BindGroup(ABaddieCompanionCharacterActor& Actor, FGroupState& Out,
            const TSet<FName>& ProtectedComponents, TSet<FName>& Claimed);
        bool Matches(const ABaddieCompanionCharacterActor& Actor, const FGroupState& Group,
            bool bCheckMaterials = true) const;
        bool Install(ABaddieCompanionCharacterActor& Actor, FGroupState& Group, TFunctionRef<bool()> IsCurrent);
        bool Retire(ABaddieCompanionCharacterActor& Actor, FGroupState& Group, TFunctionRef<bool()> IsCurrent);
        bool SyncFrames(ABaddieCompanionCharacterActor& Actor, TFunctionRef<bool()> IsCurrent);
        TArray<FGroupState> Groups;
        TOptional<FGroupState> Frames;
        FSlotState Slots[4];
        bool bInitialized = false, bCancelled = false, bObserved = false, bRoundFrames = false;
        ECharacterEquipmentIssue FramesIssue = ECharacterEquipmentIssue::ObservationUnknown;
    };
}
