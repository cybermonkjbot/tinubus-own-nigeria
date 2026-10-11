#include "BaddieCompanionWorldPresentation.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/World.h"

UBaddieCompanionWorldPresentation::UBaddieCompanionWorldPresentation()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UBaddieCompanionWorldPresentation::Observe(const BaddieCompanion::FWorldObservation& Observation)
{
    using namespace BaddieCompanion;
    if (!Observation.IsCurrent()) { Clear(); return; }
    if (Current.Generation() == Observation.Generation()) return;
    Clear();
    if (bClearing || ContentGeneration == MAX_uint64) return;
    Current = Observation;
    TSharedPtr<const FSnapshotSelection> Selection;
    if (!Current.CopySelection(Selection)) { Issue = EWorldContentIssue::SavedSelectionUnavailable; return; }
    // Only the world capture's approved selection enters this cache. The separate
    // Home read session and complete-apartment provenance are not consulted.
    auto Plan = MakeShared<FHomePresentationPlan>();
    TArray<FHomeMappingIssue> Issues;
    if (!BuildHomePresentationPlan(*Selection, *Plan, Issues))
    { Issue = EWorldContentIssue::SavedSelectionUnavailable; return; }
    SavedHome = MoveTemp(Plan);
    Issue = EWorldContentIssue::MissingImportedContent;
}

bool UBaddieCompanionWorldPresentation::Prepare(const BaddieCompanion::FNativeWorldContent& Content)
{
    using namespace BaddieCompanion;
    if (bClearing || !Current.IsCurrent() || !SavedHome
        || !SamePresentationInput(SavedHome->Source, Content.Source)) return false;
    TSharedPtr<const FWorldCurrentInstanceIdentity> Instance;
    if (!Current.CopyInstance(Instance)) return false;
    // A loaded map/declaration is not evidence of imported coverage, openings or
    // animation. There is no accepted ordinary-world content producer yet.
    Issue = EWorldContentIssue::MissingImportedContent;
    return false;
}

bool UBaddieCompanionWorldPresentation::PrepareHiddenHome(const BaddieCompanion::FNativeWorldContent& Content)
{
    using namespace BaddieCompanion;
    const auto Plan = SavedHome;
    TSharedPtr<const FWorldPlayerView> Player;
    TSharedPtr<const FWorldCurrentInstanceIdentity> Instance;
    if (!Plan || !Current.CopyPlayer(Player) || !Current.CopyInstance(Instance)
        || Instance->Relation != EWorldInstanceRelation::CurrentVisit
        || !Player->InstanceId.Equals(Instance->Id, ESearchCase::CaseSensitive)
        || !Player->PlaceId.Equals(TEXT("own-home"), ESearchCase::CaseSensitive)
        || !Plan->Home.Shell.IsSet() || !ABaddieCompanionCharacterActor::AcceptsContent(*Plan, Content.Character)) return false;
    // This method is kept private until Prepare has real native content evidence.
    // It uses the existing visual owners; it never grants rendering or possession.
    auto* World = GetWorld();
    auto* Class = Content.Character.ActorClass.Get();
    if (!World || !ABaddieCompanionCharacterActor::HasInertDefaults(Class)) return false;
    const uint64 Generation = ContentGeneration;
    FActorSpawnParameters Params; Params.Owner = GetOwner(); Params.bNoFail = false;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Spawned = World->SpawnActor<ABaddieCompanionCharacterActor>(Class, FTransform::Identity, Params);
    if (!IsValid(Spawned)) return false;
    if (Generation != ContentGeneration || !Current.IsCurrent()
        || Spawned->GetWorld() != World || Spawned->GetOwner() != GetOwner())
    { Spawned->DestroyPrivateCharacter(); return false; }
    Character = Spawned;
    const FTransform Pose(FRotator(0, SourceYawToUnreal(Player->Position.YawRadians), 0),
        SourceToUnreal(FVector(Player->Position.X, Instance->Navigation.FloorY, Player->Position.Z)), FVector::OneVector);
    if (!Spawned->PrepareWorld(*Plan, Content.Character, Pose)
        || Generation != ContentGeneration || !Current.IsCurrent()) return false;
    for (const auto& Item : Plan->Home.Items)
    {
        if (!Item.Kind.Effective.IsSet()) return false;
        const auto* Asset = Content.Furniture.FindByPredicate([&Item](const FNativeFurnitureAsset& Binding)
            { return Binding.Kind == Item.Kind.Effective.GetValue(); });
        if (!Asset || !ABaddieCompanionFurnitureActor::AcceptsBinding(Item, *Asset)) return false;
        auto* Prop = World->SpawnActor<ABaddieCompanionFurnitureActor>(FTransform::Identity, Params);
        if (!IsValid(Prop)) return false;
        if (Generation != ContentGeneration || !Current.IsCurrent()
            || Prop->GetWorld() != World || Prop->GetOwner() != GetOwner())
        { Prop->DestroyPrivateFurniture(); return false; }
        Furniture.Add(Prop);
        if (!Prop->Configure(Plan->Home, Item, *Asset, FTransform::Identity, true)
            || Generation != ContentGeneration || !Current.IsCurrent()) return false;
    }
    return true;
}

void UBaddieCompanionWorldPresentation::Clear()
{
    Current = {};
    if (bClearing) return;
    TGuardValue<bool> Guard(bClearing, true);
    if (ContentGeneration != MAX_uint64) ++ContentGeneration;
    auto OldCharacter = Character; Character = nullptr;
    auto OldFurniture = MoveTemp(Furniture);
    auto OldStream = Stream; Stream = nullptr;
    SavedHome.Reset();
    Issue = BaddieCompanion::EWorldContentIssue::ObservationUnavailable;
    if (IsValid(OldCharacter)) OldCharacter->DestroyPrivateCharacter();
    for (auto* Actor : OldFurniture) if (IsValid(Actor)) Actor->DestroyPrivateFurniture();
    if (IsValid(OldStream))
    {
        OldStream->SetShouldBeVisible(false); OldStream->SetShouldBeLoaded(false);
        OldStream->SetIsRequestingUnloadAndRemoval(true);
    }
}

void UBaddieCompanionWorldPresentation::EndPlay(const EEndPlayReason::Type Reason)
{ Clear(); Super::EndPlay(Reason); }
void UBaddieCompanionWorldPresentation::OnUnregister()
{ Clear(); Super::OnUnregister(); }
