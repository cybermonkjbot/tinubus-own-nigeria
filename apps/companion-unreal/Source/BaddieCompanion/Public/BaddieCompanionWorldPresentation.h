#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BaddieCompanionWorldObservation.h"
#include "BaddieCompanionHomePresentation.h"
#include "BaddieCompanionCharacterActor.h"
#include "BaddieCompanionFurnitureActor.h"
#include "BaddieCompanionWorldPresentation.generated.h"

class ABaddieCompanionSlicePlayerController;
class ULevelStreamingDynamic;

namespace BaddieCompanion
{
    enum class EWorldContentIssue : uint8
    { ObservationUnavailable, SavedSelectionUnavailable, MissingImportedContent, UnverifiedOpenings, UnverifiedAnimation };
    struct FNativeWorldContent
    {
        // Trusted authored package binding. Saved IDs are never used as package names.
        FSnapshotSelection Source;
        TSoftObjectPtr<UWorld> Level;
        FNativeCharacterContent Character;
        TArray<FNativeFurnitureAsset> Furniture;
    };
}

UCLASS(NotBlueprintable, Transient)
class BADDIECOMPANION_API UBaddieCompanionWorldPresentation : public UActorComponent
{
    GENERATED_BODY()
public:
    UBaddieCompanionWorldPresentation();
    void Observe(const BaddieCompanion::FWorldObservation& Observation);
    void Clear();
    BaddieCompanion::EWorldContentIssue GetIssue() const { return Issue; }
    bool IsReady() const { return false; }
protected:
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void OnUnregister() override;
private:
    friend class ABaddieCompanionSlicePlayerController;
    bool Prepare(const BaddieCompanion::FNativeWorldContent& Content);
    bool PrepareHiddenHome(const BaddieCompanion::FNativeWorldContent& Content);
    BaddieCompanion::FWorldObservation Current;
    TSharedPtr<const BaddieCompanion::FHomePresentationPlan> SavedHome;
    BaddieCompanion::EWorldContentIssue Issue = BaddieCompanion::EWorldContentIssue::ObservationUnavailable;
    uint64 ContentGeneration = 0;
    bool bClearing = false;
    UPROPERTY(Transient)
    TObjectPtr<ABaddieCompanionCharacterActor> Character;
    UPROPERTY(Transient)
    TArray<TObjectPtr<ABaddieCompanionFurnitureActor>> Furniture;
    UPROPERTY(Transient)
    TObjectPtr<ULevelStreamingDynamic> Stream;
};
