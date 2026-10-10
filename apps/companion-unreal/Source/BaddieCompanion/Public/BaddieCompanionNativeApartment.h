#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BaddieCompanionHomePresentation.h"
#include "BaddieCompanionNativeApartment.generated.h"

class ABaddieCompanionSlicePlayerController;
class ACharacter;
class ULevelStreamingDynamic;
class UWorld;

namespace BaddieCompanion
{
    enum class ENativeApartmentGap : uint8
    {
        MissingContent, IncompleteInventory, AdmissionUnavailable, SavedPresentationUnavailable,
        InvalidBinding, DoorwayNotAuthored, LoadFailed, AnchorUnavailable,
        NavigationUnavailable, DaylightUnverified, RenderAcceptanceOutstanding
    };
    struct FNativeApartmentStatus
    {
        TArray<ENativeApartmentGap> Gaps;
        int32 PendingLevels = 0;
        int32 BoundDoorways = 0;
        // Source bindings do not establish player-height acceptance or gameplay authority.
        bool IsWalkThroughReady() const { return false; }
    };
    struct FNativeDoorwayBinding
    {
        int32 ConnectionIndex = INDEX_NONE;
        bool bFromEndpoint = true;
        // Real actor names in the supplied native level; no invented sockets or tags.
        FName DoorActor, ApproachActor;
        // Native navigation must supply this height; the doorway source does not define floorY.
        TOptional<double> SourceFloorYMetres;
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
        // Native room packages must already contain this exact saved presentation/design.
        // The whole-level transform never applies the saved shell turn to decor a second time.
        FSnapshotSelection Source;
        TArray<FNativeApartmentLevelBinding> Levels;
    };
    struct FNativeDoorwayPose
    {
        FVector DoorCm = FVector::ZeroVector, PreferredApproachCm = FVector::ZeroVector;
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
private:
    friend class ABaddieCompanionSlicePlayerController;
    bool PrepareContent(const BaddieCompanion::FHomePresentationPlan& Plan,
        const BaddieCompanion::FNativeApartmentContent& Content);
    void PollNativeLevels();
    bool ValidateLoadedAnchors(int32 Index);
    bool CopyNavigationApproach(ACharacter& Character, int32 ConnectionIndex,
        bool bFromEndpoint, FVector& Out);
    bool CopyRoomCamera(const FString& RoomId, BaddieCompanion::FNativeRoomCamera& Out);
    bool RefreshAuthoredDaylight();
    void AddGap(BaddieCompanion::ENativeApartmentGap Gap);

    TOptional<BaddieCompanion::FSnapshotSelection> Source;
    TOptional<BaddieCompanion::FHomePresentationPlan> Presentation;
    TArray<BaddieCompanion::FNativeApartmentLevelBinding> Bindings;
    // GC owns pending stream objects; reset requests hide/unload before dropping references.
    UPROPERTY(Transient)
    TArray<TObjectPtr<ULevelStreamingDynamic>> Streams;
    TArray<bool> AnchorsChecked;
    bool bDaylightHookAttempted = false;
    BaddieCompanion::FNativeApartmentStatus Status;
};
