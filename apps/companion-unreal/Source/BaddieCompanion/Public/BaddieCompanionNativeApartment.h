#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BaddieCompanionCharacterActor.h"
#include "BaddieCompanionFurnitureActor.h"
#include "BaddieCompanionHomePresentation.h"
#include "BaddieCompanionNativeApartment.generated.h"

class ABaddieCompanionSlicePlayerController;
class ACharacter;
class FStreamableHandle;
class ULevelStreamingDynamic;
class UWorld;

namespace BaddieCompanion
{
    enum class ENativeApartmentGap : uint8
    {
        MissingContent, IncompleteInventory, AdmissionUnavailable, SavedPresentationUnavailable,
        InvalidBinding, DoorwayNotAuthored, LoadFailed, AnchorUnavailable,
        NavigationUnavailable, DaylightUnverified, RenderAcceptanceOutstanding, FurnitureUnavailable,
        CharacterUnavailable
    };
    struct FNativeApartmentStatus
    {
        TArray<ENativeApartmentGap> Gaps;
        int32 PendingLevels = 0;
        int32 BoundDoorways = 0;
        int32 PreparedFurniture = 0;
        bool bFurnitureAssetsPending = false;
        bool bCharacterAssetsPending = false;
        bool bCharacterPrepared = false;
        // Source bindings do not establish player-height acceptance or gameplay authority.
        bool IsWalkThroughReady() const { return false; }
    };
    struct FNativeDoorwayPose
    {
        FVector DoorCm = FVector::ZeroVector, PreferredApproachCm = FVector::ZeroVector;
    };
    struct FNativeDoorwayBinding
    {
        int32 ConnectionIndex = INDEX_NONE;
        bool bFromEndpoint = true;
        // Real actor names in the supplied native level; no invented sockets or tags.
        FName DoorActor, ApproachActor;
        // Measured bedroom/studio entrance only. Native navigation supplies its floor height.
        TOptional<double> SourceFloorYMetres;
        // All other openings require measured native-level-local anchor positions, in UE cm.
        // Already includes imported basis and the saved shell design, never the apartment transform.
        // Trusted authored content only; not a standing-point fallback or a measured-entrance override.
        TOptional<FNativeDoorwayPose> AuthoredLocalPose;
    };
    struct FNativeApartmentLevelBinding
    {
        FApartmentEndpoint Endpoint;
        // Trusted content setup only, never a snapshot asset key, link or inferred /Game path.
        TSoftObjectPtr<UWorld> Level;
        FTransform InstanceToApartment = FTransform::Identity;
        // Empty-front-strip extension and shell turn must match Source before whole-level assembly.
        // No room is stretched or regenerated here; unsupported native variants fail preparation.
        TArray<FNativeDoorwayBinding> Doors;
        // Source actor identifiers for authored common-space components/geometry.
        FName WindowActor, SkyActor, SunActor, AtmosphereActor, ExposureActor, SeatingViewActor;
    };
    struct FNativeApartmentContent
    {
        // Native packages contain the authored shell/fixed fixtures for this saved design.
        // Saved item instances are created separately; do not bake them into these packages.
        // The whole-level transform never applies the saved shell turn to decor a second time.
        FSnapshotSelection Source;
        TArray<FNativeApartmentLevelBinding> Levels;
        TArray<FNativeFurnitureAsset> FurnitureAssets;
        FNativeCharacterContent Character;
    };
    struct FNativeRoomCamera
    {
        // Keep the canonical room-space values separate from the assembled apartment-space view.
        FCameraReference SourceRoom;
        FVector EyeApartmentCm = FVector::ZeroVector, TargetApartmentCm = FVector::ZeroVector;
    };
    // Only the three measured canonical layouts have doors. No standing-point fallback.
    BADDIECOMPANION_API bool CopyCanonicalNativeDoorway(const FRoomPlan& Room,
        double SourceFloorYMetres, const FTransform& InstanceToApartment, FNativeDoorwayPose& Out);
}

/** Controller-owned native level lifetime. No ticker, credential, pawn, movement or camera owner. */
UCLASS(NotBlueprintable, Transient)
class BADDIECOMPANION_API UBaddieCompanionNativeApartment : public UActorComponent
{
    GENERATED_BODY()
public:
    UBaddieCompanionNativeApartment();
    void ClearPrivateContent();
    bool HasPrivateContent() const { return Source.IsSet() || !Streams.IsEmpty(); }
    void ObserveValidPresentation(const BaddieCompanion::FHomePresentationPlan& Plan);
    const BaddieCompanion::FNativeApartmentStatus& GetStatus() const { return Status; }
protected:
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void OnUnregister() override;
private:
    friend class ABaddieCompanionSlicePlayerController;
    bool PrepareContent(const BaddieCompanion::FHomePresentationPlan& Plan,
        const BaddieCompanion::FNativeApartmentContent& Content);
    void PollNativeLevels();
    bool PrepareRoomFurniture(int32 Index);
    bool PrepareHomeCharacter(int32 Index);
    bool ValidateLoadedAnchors(int32 Index);
    bool CopyNavigationApproach(ACharacter& Character, int32 ConnectionIndex,
        bool bFromEndpoint, FVector& Out);
    bool CopyRoomCamera(const FString& RoomId, BaddieCompanion::FNativeRoomCamera& Out);
    bool RefreshAuthoredDaylight();
    void AddGap(BaddieCompanion::ENativeApartmentGap Gap);

    TOptional<BaddieCompanion::FSnapshotSelection> Source;
    TOptional<BaddieCompanion::FHomePresentationPlan> Presentation;
    TArray<BaddieCompanion::FNativeApartmentLevelBinding> Bindings;
    TArray<BaddieCompanion::FNativeFurnitureAsset> FurnitureAssets;
    TSharedPtr<FStreamableHandle> FurnitureAssetLoad;
    TOptional<BaddieCompanion::FNativeCharacterContent> CharacterContent;
    TSharedPtr<FStreamableHandle> CharacterAssetLoad;
    UPROPERTY(Transient)
    TObjectPtr<ABaddieCompanionCharacterActor> PreparedCharacter;
    // GC owns pending stream objects; reset requests hide/unload before dropping references.
    UPROPERTY(Transient)
    TArray<TObjectPtr<ULevelStreamingDynamic>> Streams;
    UPROPERTY(Transient)
    TArray<TObjectPtr<ABaddieCompanionFurnitureActor>> Furniture;
    TArray<bool> AnchorsChecked;
    TArray<bool> FurniturePrepared;
    bool bDaylightHookAttempted = false;
    bool bClearing = false;
    bool bEndingPlay = false;
    uint64 ContentGeneration = 0;
    BaddieCompanion::FNativeApartmentStatus Status;
};
