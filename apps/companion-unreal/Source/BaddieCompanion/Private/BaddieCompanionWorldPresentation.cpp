#include "BaddieCompanionWorldPresentation.h"
#include "BaddieCompanionWorldPublication.h"
#include "BaddieCompanionSlicePlayerController.h"
#include "BaddieCompanionGameplaySession.h"
#include "BaddieCompanionWorldCommandLifecycle.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NavigationData.h"

UBaddieCompanionWorldPresentation::UBaddieCompanionWorldPresentation()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UBaddieCompanionWorldPresentation::Observe(const BaddieCompanion::FWorldObservation& Observation)
{
    using namespace BaddieCompanion;
    if (!Observation.IsCurrent()) { Clear(); return; }
    if (bClearing || Current.Token() == Observation.Token() || Pending.Token() == Observation.Token()) return;
    Pending = Observation;
}

void UBaddieCompanionWorldPresentation::Reconcile()
{
    using namespace BaddieCompanion;
    if (bClearing) return;
    CheckNativeContent();
    if (bClearing) return;
    if (Pending.Token().IsSet())
    {
        const auto Next = Pending;
        const uint64 BeforeClear = ContentGeneration;
        Clear();
        if (bClearing || !Next.IsCurrent() || BeforeClear == MAX_uint64
            || ContentGeneration != BeforeClear + 1 || ContentGeneration == MAX_uint64) return;
        Current = Next;
    }
    if (!Current.IsCurrent()) { Clear(); return; }
    const auto Result = Current.ReadHomeProjection(SavedHome);
    if (Result == EWorldReadResult::Pending) return;
    if (Result == EWorldReadResult::Unavailable)
    { Issue = EWorldContentIssue::SavedSelectionUnavailable; return; }
    bool bMapped = false;
    const auto Mapped = SavedHome.Outcome(bMapped);
    if (Mapped == EWorldReadResult::Pending) return;
    Issue = Mapped == EWorldReadResult::Ready && bMapped ? EWorldContentIssue::MissingImportedContent
        : EWorldContentIssue::SavedSelectionUnavailable;
}

bool UBaddieCompanionWorldPresentation::Prepare(const BaddieCompanion::FNativeWorldContent& Content)
{
    using namespace BaddieCompanion;
    Reconcile();
    if (bClearing || !Current.IsCurrent() || !SavedHome.IsCurrent()
        || !Content.Source.IsCurrent() || Current.Token() != Content.Source.Token()) return false;
    FWorldInstanceRead Instance;
    FWorldInstanceHeader Header;
    if (Current.ReadInstance(Instance) != EWorldReadResult::Ready || Instance.Header(Header) != EWorldReadResult::Ready) return false;
    // A loaded map/declaration is not evidence of imported coverage, openings or
    // animation. There is no accepted ordinary-world content producer yet.
    Issue = EWorldContentIssue::MissingImportedContent;
    return false;
}

bool UBaddieCompanionWorldPresentation::PrepareHiddenHome(const BaddieCompanion::FNativeWorldContent& Content)
{
    using namespace BaddieCompanion;
    WorldObservationPrivate::FHomeBorrow Borrow;
    FWorldPlayerRead PlayerRead;
    FWorldInstanceRead InstanceRead;
    FWorldPlayerHeader Player;
    FWorldInstanceHeader Instance;
    if (Current.BorrowHome(Borrow) != EWorldReadResult::Ready
        || Current.ReadPlayer(PlayerRead) != EWorldReadResult::Ready || PlayerRead.Header(Player) != EWorldReadResult::Ready
        || Current.ReadInstance(InstanceRead) != EWorldReadResult::Ready || InstanceRead.Header(Instance) != EWorldReadResult::Ready
        || Instance.Relation != EWorldInstanceRelation::CurrentVisit || !Instance.bPlayerInstanceMatches || !Instance.bOwnHome
        || !Content.Source.IsCurrent() || Content.Source.Token() != Current.Token()) return false;
    const auto* Plan = Borrow.Data;
    const auto Token = Current.Token();
    // Includes binding scans, actor/component allocations, furniture growth,
    // setters and failure-path native teardown. No native work is qualified.
    constexpr uint64 QualifiedNativePreparationWork = 0;
    constexpr uint64 QualifiedNativePreparationBytes = 0;
    if (!QualifiedNativePreparationWork || !QualifiedNativePreparationBytes
        || QualifiedNativePreparationWork > WorldObservationPrivate::TickWorkLimit
        || !Borrow.Storage || !Borrow.Storage->Publication) return false;
    if (!Borrow.Storage->Budget->Spend(QualifiedNativePreparationWork, false)
        || !Borrow.Storage->Budget->Reserve(QualifiedNativePreparationBytes)) return false;
    Borrow.Storage->Publication->NativeCharge += QualifiedNativePreparationBytes;
    if (!Plan
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
    if (Generation != ContentGeneration || Current.Token() != Token || !Current.IsCurrent()
        || Spawned->GetWorld() != World || Spawned->GetOwner() != GetOwner())
    { Spawned->DestroyPrivateCharacter(); return false; }
    Character = Spawned;
    const FTransform Pose(FRotator(0, SourceYawToUnreal(Player.Position.YawRadians), 0),
        SourceToUnreal(FVector(Player.Position.X, Instance.FloorY, Player.Position.Z)), FVector::OneVector);
    if (!Spawned->PrepareWorld(*Plan, Content.Character, Pose)
        || Generation != ContentGeneration || Current.Token() != Token || !Current.IsCurrent()) return false;
    for (const auto& Item : Plan->Home.Items)
    {
        if (Generation != ContentGeneration || Current.Token() != Token || !Current.IsCurrent()) return false;
        if (!Item.Kind.Effective.IsSet()) return false;
        const auto* Asset = Content.Furniture.FindByPredicate([&Item](const FNativeFurnitureAsset& Binding)
            { return Binding.Kind == Item.Kind.Effective.GetValue(); });
        if (!Asset || !ABaddieCompanionFurnitureActor::AcceptsBinding(Item, *Asset)) return false;
        auto* Prop = World->SpawnActor<ABaddieCompanionFurnitureActor>(FTransform::Identity, Params);
        if (!IsValid(Prop)) return false;
        if (Generation != ContentGeneration || Current.Token() != Token || !Current.IsCurrent()
            || Prop->GetWorld() != World || Prop->GetOwner() != GetOwner())
        { Prop->DestroyPrivateFurniture(); return false; }
        Furniture.Add(Prop);
        if (!Prop->Configure(Plan->Home, Item, *Asset, FTransform::Identity, true)
            || Generation != ContentGeneration || Current.Token() != Token || !Current.IsCurrent()) return false;
    }
    if (Generation != ContentGeneration || Current.Token() != Token || !Current.IsCurrent()) return false;
    NativeCharacter = Content.Character; NativeFurniture = Content.Furniture;
    NativeWorld = World; NativeMaterialGeneration = Spawned->PreparedMaterialGeneration;
    // Only completed private native preparation can supply this loss token.
    // Missing imports and IsReady false never create one or invalidate reads.
    NativeToken = Token;
    return true;
}

void UBaddieCompanionWorldPresentation::CheckNativeContent()
{
    if (!NativeToken.IsSet() || bClearing) return;
    if (!Current.IsCurrent() || NativeToken != Current.Token()) { NativeToken = {}; return; }
    if (Current.AdmitNativeContentObservation() != BaddieCompanion::EWorldReadResult::Ready) return;
    bool bLost = !IsValid(Character) || Character->IsActorBeingDestroyed();
    for (const auto& Actor : Furniture)
        bLost |= !IsValid(Actor.Get()) || Actor->IsActorBeingDestroyed();
    if (!bLost) return;
    const auto Token = NativeToken;
    NativeToken = {};
    if (auto* Controller = Cast<ABaddieCompanionSlicePlayerController>(GetOwner()))
        Controller->WorldNativeContentLost(Token);
}
bool UBaddieCompanionWorldPresentation::InspectNative(BaddieCompanion::FWorldNativeEvidence& Out)
{
    using namespace BaddieCompanion;
    Out = {};
    constexpr bool bTrustedCoverageQualified = false;
    // There is no trusted imported rig/navigation/openings/activation producer.
    // Loaded actors and mapped descriptors alone cannot create trusted evidence.
    // Admission precedes every native getter and topology/material allocation.
    if (!bTrustedCoverageQualified || bClearing || !BindingGeneration || !Current.IsCurrent()
        || NativeToken != Current.Token() || !SavedHome.IsCurrent()
        || Current.AdmitNativeContentObservation() != EWorldReadResult::Ready) return false;
    const auto Token = Current.Token();
    const uint64 Generation = ContentGeneration, Binding = BindingGeneration;
    WorldObservationPrivate::FHomeBorrow Borrow;
    if (Current.BorrowHome(Borrow) != EWorldReadResult::Ready || !Borrow.Data || !Borrow.Storage
        || !IsValid(Character) || !NativeWorld.IsValid() || NativeWorld.Get() != GetWorld()
        || !Character->InspectCurrent(*Borrow.Data, NativeCharacter, NativeWorld.Get(), GetOwner(), NativeMaterialGeneration)
        || Furniture.Num() != Borrow.Data->Home.Items.Num()) return false;
    const auto& Room = Borrow.Data->Home;
    if (!Room.bSourceMapped || !Room.Shell.IsSet()) return false;
    for (int32 I = 0; I < Furniture.Num(); ++I)
    {
        auto* Actor = Furniture[I].Get();
        const auto& Item = Room.Items[I];
        const auto* Asset = NativeFurniture.FindByPredicate([&](const auto& Value)
            { return Item.Kind.Effective.IsSet() && Value.Kind == Item.Kind.Effective.GetValue(); });
        if (!IsValid(Actor) || Actor->IsActorBeingDestroyed() || Actor->GetOwner() != GetOwner()
            || Actor->GetWorld() != NativeWorld.Get() || !Actor->bConfigured || Actor->bDiscarded
            || Actor->bChangingPresentation || Actor->bClearing || Actor->bSuppressingPower
            || Actor->bDestroyRequested || Actor->bDisposalBlocked || Actor->PendingApartmentPose.IsSet()
            || !Asset || !Actor->AcceptsBinding(Item, *Asset) || !Item.PlacementPose.IsSet()
            || !Actor->RoomId.Equals(Room.Id, ESearchCase::CaseSensitive)
            || !Actor->ItemId.Equals(Item.Id, ESearchCase::CaseSensitive)
            || !IsValid(Actor->FurnitureMesh) || !IsValid(Actor->PracticalLight)
            || !Actor->FurnitureMesh->IsRegistered() || !Actor->PracticalLight->IsRegistered()
            || Actor->FurnitureMesh->GetOwner() != Actor || Actor->PracticalLight->GetOwner() != Actor
            || Actor->FurnitureMesh->GetWorld() != NativeWorld.Get() || Actor->PracticalLight->GetWorld() != NativeWorld.Get()
            || !Asset->Mesh.IsValid() || Actor->FurnitureMesh->GetStaticMesh() != Asset->Mesh.Get()
            || Actor->PracticalLight->GetAttachParent() != Actor->FurnitureMesh.Get()) return false;
        const auto& Placement = *Item.PlacementPose;
        const FTransform Pose(FRotator(0, Placement.UnrealYawDegrees, 0), Placement.UnrealCentimetres,
            FVector(Room.Shell->ObjectScale));
        if (Pose.ContainsNaN() || !Actor->GetActorTransform().Equals(Pose, 1.e-4)) return false;
        const bool bEmissive = Asset->Kind == EItemKind::FloorLamp || Asset->Kind == EItemKind::DeskLamp;
        if (bEmissive)
        {
            const auto Names = Actor->FurnitureMesh->GetMaterialSlotNames();
            int32 Count = 0;
            for (FName Name : Names) if (Name == Asset->EmissiveMaterialSlot) ++Count;
            const int32 Slot = Actor->FurnitureMesh->GetMaterialIndex(Asset->EmissiveMaterialSlot);
            float Value = 0;
            const float Power = Actor->LastAppliedPower.Get(0.f);
            if (Count != 1 || Slot == INDEX_NONE || !IsValid(Actor->BulbMaterial)
                || Actor->FurnitureMesh->GetMaterial(Slot) != Actor->BulbMaterial.Get()
                || Actor->EmissiveParameter != Asset->EmissiveIntensityParameter
                || Actor->SavedEmissiveIntensity != (Item.bEffectiveLit ? .6f : 0.f)
                || !Actor->BulbMaterial->GetScalarParameterValue(FMaterialParameterInfo(Actor->EmissiveParameter), Value)
                || !FMath::IsFinite(Value) || Value != Actor->SavedEmissiveIntensity * Power) return false;
        }
        else if (Actor->BulbMaterial || !Actor->FurnitureMesh->OverrideMaterials.IsEmpty()) return false;
        if (Generation != ContentGeneration || Binding != BindingGeneration || NativeToken != Token
            || !Current.IsCurrent() || Current.Token() != Token) return false;
    }
    if (bClearing || Generation != ContentGeneration || Binding != BindingGeneration || NativeToken != Token
        || !Current.IsCurrent() || Current.Token() != Token) return false;
    Out.Token = Current.Token(); Out.Content = ContentGeneration; Out.Binding = BindingGeneration;
    Out.bTrusted = true; return true;
}
bool UBaddieCompanionWorldPresentation::CurrentCycle() const
{
    const auto* Controller = Cast<ABaddieCompanionSlicePlayerController>(GetOwner());
    const auto* Session = Controller ? Controller->GameplaySession.Get() : nullptr;
    return Session && Current.IsCurrent() && NativeToken == Current.Token()
        && Session->WorldCycleCurrent(const_cast<ABaddieCompanionSlicePlayerController*>(Controller), Current.Token());
}
bool UBaddieCompanionWorldPresentation::AdoptCompatible(const BaddieCompanion::FWorldObservation& Observation)
{
    using namespace BaddieCompanion;
    auto* Controller = Cast<ABaddieCompanionSlicePlayerController>(GetOwner());
    auto* Session = Controller ? Controller->GameplaySession.Get() : nullptr;
    if (bClearing || !Session || !Observation.IsCurrent()
        || !Session->WorldTransferCurrent(Controller, Current.Token(), Observation.Token())) return false;
    Current = Observation; Pending = {}; NativeToken = Observation.Token(); SavedHome = {};
    if (Current.ReadHomeProjection(SavedHome) != EWorldReadResult::Ready) return false;
    FWorldNativeEvidence Evidence;
    if (!InspectNative(Evidence)) return false;
    Session->ObserveWorldNative(Controller, Evidence);
    return !bClearing && Current.IsCurrent() && Current.Token() == Observation.Token();
}

void UBaddieCompanionWorldPresentation::Clear()
{
    Current = {}; Pending = {}; SavedHome = {}; NativeToken = {}; BindingGeneration = 0;
    NativeCharacter = {}; NativeFurniture.Reset(); NativeWorld.Reset(); NativeMaterialGeneration = 0;
    StreetNavigation.Reset(); StreetNavigationGeneration = 0;
    if (ContentGeneration != MAX_uint64) ++ContentGeneration;
    if (bClearing) return;
    TGuardValue<bool> Guard(bClearing, true);
    auto OldCharacter = Character; Character = nullptr;
    auto OldFurniture = MoveTemp(Furniture);
    auto OldStream = Stream; Stream = nullptr;
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
{
    const auto Token = NativeToken;
    NativeToken = {};
    if (Token.IsSet())
        if (auto* Controller = Cast<ABaddieCompanionSlicePlayerController>(GetOwner()))
            Controller->WorldNativeContentLost(Token);
    Clear(); Super::EndPlay(Reason);
}
void UBaddieCompanionWorldPresentation::OnUnregister()
{
    const auto Token = NativeToken;
    NativeToken = {};
    if (Token.IsSet())
        if (auto* Controller = Cast<ABaddieCompanionSlicePlayerController>(GetOwner()))
            Controller->WorldNativeContentLost(Token);
    Clear(); Super::OnUnregister();
}
