#include "BaddieCompanionNativeApartment.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/Level.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/StaticMesh.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/PlatformTime.h"
#include "NavigationSystem.h"
#include "NavigationData.h"
#include "NavigationSystemTypes.h"
#include "Templates/UnrealTemplate.h"

#include <cmath>

namespace BaddieCompanion::NativeApartment
{
    bool SameEndpoint(const FApartmentEndpoint& A, const FApartmentEndpoint& B)
    {
        return A.Space == B.Space && A.RoomId.Equals(B.RoomId, ESearchCase::CaseSensitive);
    }
    bool IsAuthoredLivingExit(const FApartmentPlan& Plan, const FNativeApartmentLevelBinding& Binding,
        const FNativeDoorwayBinding& Door)
    {
        if (Binding.Endpoint.Space != EApartmentSpace::LivingRoom
            || !Plan.Connections.IsValidIndex(Door.ConnectionIndex)) return false;
        const auto& Connection = Plan.Connections[Door.ConnectionIndex];
        const auto& Other = Door.bFromEndpoint ? Connection.To : Connection.From;
        return Other.Space == EApartmentSpace::ExteriorLanding;
    }
    bool RigidInstance(const FTransform& Transform)
    {
        // No scaling, mirroring, pitch/roll or deformation of saved room geometry.
        return !Transform.ContainsNaN() && Transform.GetRotation().IsNormalized()
            && Transform.GetScale3D().Equals(FVector::OneVector, 1.e-6)
            && Transform.GetRotation().RotateVector(FVector::UpVector).Equals(FVector::UpVector, 1.e-6);
    }
    bool HasMeasuredEntrance(const FRoomPlan& Room)
    {
        if (!Room.Environment.Effective.IsSet() || Room.Environment.Effective.GetValue() != EEnvironment::Studio
            || !Room.Layout.Effective.IsSet()) return false;
        switch (Room.Layout.Effective.GetValue())
        {
        case ELayout::PinkBedroomV2:
        case ELayout::PatStudioV1:
        case ELayout::PinkBedroomV1: return true;
        default: return false;
        }
    }
    bool ResolveLocalDoorway(const FHomePresentationPlan& Plan, const FNativeApartmentLevelBinding& Binding,
        const FNativeDoorwayBinding& Door, FNativeDoorwayPose& Out)
    {
        Out = {};
        const FRoomPlan* Room = nullptr;
        if (!Binding.Endpoint.RoomId.IsEmpty())
        {
            Room = Plan.Rooms.FindByPredicate([&](const auto& R)
                { return R.Id.Equals(Binding.Endpoint.RoomId, ESearchCase::CaseSensitive); });
            if (!Room || !Room->bSourceMapped || !Room->Shell.IsSet()) return false;
        }
        if (Room && HasMeasuredEntrance(*Room) && !IsAuthoredLivingExit(Plan.Apartment, Binding, Door))
        {
            // Bad measured input cannot fall through to an authored override.
            if (Door.AuthoredLocalPose.IsSet() || !Door.SourceFloorYMetres.IsSet()) return false;
            return CopyCanonicalNativeDoorway(*Room, Door.SourceFloorYMetres.GetValue(), FTransform::Identity, Out);
        }
        // Kitchen/bathroom/legacy entrances, common spaces and the separate living-room exit.
        // Nothing here invents a doorway or uses the source navigation standing-point fallback.
        if (Door.SourceFloorYMetres.IsSet() || !Door.AuthoredLocalPose.IsSet()) return false;
        const auto& Authored = Door.AuthoredLocalPose.GetValue();
        if (Authored.DoorCm.ContainsNaN() || Authored.PreferredApproachCm.ContainsNaN()
            || Authored.DoorCm.Equals(Authored.PreferredApproachCm, .1)) return false;
        Out = Authored;
        return true;
    }
    AActor* ActorNamed(ULevel* Level, FName Name)
    {
        if (!Level || Name.IsNone()) return nullptr;
        AActor* Found = nullptr;
        for (AActor* Actor : Level->Actors)
            if (IsValid(Actor) && Actor->GetFName() == Name)
            {
                if (Found) return nullptr;
                Found = Actor;
            }
        return Found;
    }
    template<typename T> T* ComponentNamed(ULevel* Level, FName ActorName)
    {
        auto* Actor = ActorNamed(Level, ActorName);
        if (!Actor) return nullptr;
        TInlineComponentArray<T*> Components(Actor);
        // Ambiguous content requires an authored binding correction, not a first-component guess.
        return Components.Num() == 1 ? Components[0] : nullptr;
    }
}

bool BaddieCompanion::CopyCanonicalNativeDoorway(const FRoomPlan& Room, double FloorY,
    const FTransform& Instance, FNativeDoorwayPose& Out)
{
    Out = {};
    if (!FMath::IsFinite(FloorY) || !NativeApartment::RigidInstance(Instance)
        || !Room.bSourceMapped || !Room.Shell.IsSet() || !Room.Layout.Effective.IsSet()
        || !Room.Environment.Effective.IsSet() || Room.Environment.Effective.GetValue() != EEnvironment::Studio)
        return false;
    FVector Door, Approach;
    switch (Room.Layout.Effective.GetValue())
    {
    case ELayout::PinkBedroomV2:
        Door = FVector(-1.74, FloorY, -2.6); Approach = FVector(-1.74, FloorY, -2.25); break;
    case ELayout::PatStudioV1:
    case ELayout::PinkBedroomV1:
        Door = FVector(-1.6891, FloorY, .6931); Approach = FVector(-1.34, FloorY, .6931); break;
    default: return false; // Kitchen/bathroom/legacy standing points are NOT doorways.
    }
    const int32 Turns = Room.SavedDesign.IsSet() ? Room.SavedDesign.GetValue().Turns : 0;
    if (Turns < 0 || Turns > 3 || (Room.SavedDesign.IsSet()
        && (Room.SavedDesign->Version != 1 || Room.SavedDesign->Extension != 0))) return false;
    // Source doorway literals are unturned; shell/item poses in the plan are already designed.
    Out.DoorCm = Instance.TransformPosition(SourceToUnreal(TurnSourceShell(Door, Turns)));
    Out.PreferredApproachCm = Instance.TransformPosition(SourceToUnreal(TurnSourceShell(Approach, Turns)));
    if (Out.DoorCm.ContainsNaN() || Out.PreferredApproachCm.ContainsNaN()) { Out = {}; return false; }
    return true; // Measured preferred approach only, not source planRoomRoute or spawn clearance.
}

UBaddieCompanionNativeApartment::UBaddieCompanionNativeApartment()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(false);
    AddGap(BaddieCompanion::ENativeApartmentGap::MissingContent);
    AddGap(BaddieCompanion::ENativeApartmentGap::ScenePowerUnavailable);
}

void UBaddieCompanionNativeApartment::AddGap(BaddieCompanion::ENativeApartmentGap Gap)
{
    Status.Gaps.AddUnique(Gap);
}

void UBaddieCompanionNativeApartment::ClearPrivateContent()
{
    if (ContentGeneration == MAX_uint64) bContentGenerationExhausted = true;
    ++ContentGeneration;
    if (bClearing) return;
    TGuardValue<bool> Clearing(bClearing, true);
    ClearScenePower(EScenePowerClear::SuppressOutput);
    bool bFurnitureDestroyFailed = false;
    bool bCharacterDestroyFailed = false;
    if (CharacterAssetLoad.IsValid())
    {
        CharacterAssetLoad->CancelHandle();
        CharacterAssetLoad.Reset();
    }
    if (IsValid(PreparedCharacter.Get()))
    {
        const TWeakObjectPtr<ABaddieCompanionCharacterActor> Current(PreparedCharacter.Get());
        if (!Current->DestroyPrivateCharacter()) bCharacterDestroyFailed = true;
    }
    PreparedCharacter = nullptr; CharacterContent.Reset();
    if (FurnitureAssetLoad.IsValid())
    {
        FurnitureAssetLoad->CancelHandle();
        FurnitureAssetLoad.Reset();
    }
    for (const auto& OwnedFurniture : Furniture)
        if (auto* Actor = OwnedFurniture.Get(); IsValid(Actor))
        {
            if (!Actor->DestroyPrivateFurniture()) bFurnitureDestroyFailed = true;
        }
    Furniture.Reset(); FurniturePrepared.Reset(); FurnitureAssets.Reset();
    // No async lambdas retain a plan or an old request. All streams start invisible.
    for (const auto& OwnedStream : Streams)
    {
        auto* Stream = OwnedStream.Get();
        if (!IsValid(Stream)) continue;
        Stream->SetShouldBeVisible(false);
        if (ULevel* Level = Stream->GetLoadedLevel())
            for (AActor* Actor : Level->Actors)
            {
                if (!IsValid(Actor)) continue;
                // Native unloading is deferred; hide/stop private presentation immediately.
                Actor->SetActorHiddenInGame(true);
                Actor->SetActorEnableCollision(false);
                Actor->SetActorTickEnabled(false);
                TInlineComponentArray<UActorComponent*> Components(Actor);
                for (auto* Component : Components)
                {
                    Component->Deactivate();
                    Component->SetComponentTickEnabled(false);
                    if (auto* Primitive = Cast<UPrimitiveComponent>(Component)) Primitive->SetSimulatePhysics(false);
                    if (auto* Scene = Cast<USceneComponent>(Component)) Scene->SetVisibility(false, true);
                }
            }
        Stream->SetShouldBeLoaded(false);
        Stream->SetIsRequestingUnloadAndRemoval(true);
    }
    Streams.Reset(); AnchorsChecked.Reset(); Bindings.Reset();
    Presentation.Reset(); Source.Reset(); Status = {}; bDaylightHookAttempted = false;
    AddGap(BaddieCompanion::ENativeApartmentGap::MissingContent);
    AddGap(BaddieCompanion::ENativeApartmentGap::ScenePowerUnavailable);
    // A failed destroy still leaves an empty, inactive actor in a level requested for unload.
    if (bFurnitureDestroyFailed) AddGap(BaddieCompanion::ENativeApartmentGap::FurnitureUnavailable);
    if (bCharacterDestroyFailed) AddGap(BaddieCompanion::ENativeApartmentGap::CharacterUnavailable);
}

void UBaddieCompanionNativeApartment::EndPlay(const EEndPlayReason::Type Reason)
{
    bEndingPlay = true;
    ClearPrivateContent();
    Super::EndPlay(Reason);
}

void UBaddieCompanionNativeApartment::OnUnregister()
{
    ClearPrivateContent();
    Super::OnUnregister();
}

void UBaddieCompanionNativeApartment::ObserveValidPresentation(const BaddieCompanion::FHomePresentationPlan& Plan)
{
    if (Source.IsSet() && !BaddieCompanion::SamePresentationInput(Source.GetValue(), Plan.Source))
        ClearPrivateContent();
    using BaddieCompanion::ENativeApartmentGap;
    if (!Plan.Apartment.bCompleteHomeInventoryVerified) AddGap(ENativeApartmentGap::IncompleteInventory);
    AddGap(ENativeApartmentGap::AdmissionUnavailable);
    AddGap(ENativeApartmentGap::SavedPresentationUnavailable);
    AddGap(ENativeApartmentGap::DaylightUnverified);
    AddGap(ENativeApartmentGap::RenderAcceptanceOutstanding);
    AddGap(ENativeApartmentGap::ScenePowerUnavailable);
    // Do not retain another full plan on controller ticks unless actual content was prepared.
}

void UBaddieCompanionNativeApartment::ClearScenePower(EScenePowerClear Mode)
{
    if (PowerGeneration == MAX_uint64) bContentGenerationExhausted = true;
    ++PowerGeneration;
    ScenePower.Reset();
    AddGap(BaddieCompanion::ENativeApartmentGap::ScenePowerUnavailable);
    // A nested teardown wins over restoration, including after an earlier lamp was restored.
    if (Mode == EScenePowerClear::SuppressOutput) bPowerSuppressionRequested = true;
    bUseSavedLighting = false;
    if (bClearingPower) return;
    TGuardValue<bool> Clearing(bClearingPower, true);
    bPowerSuppressionRequested = Mode == EScenePowerClear::SuppressOutput;
    const uint64 Generation = ContentGeneration, Power = PowerGeneration;
    const auto CurrentFurniture = Furniture;
    for (const auto& Owned : CurrentFurniture)
        if (IsValid(Owned.Get())) Owned->SuppressPower();
    if (Mode == EScenePowerClear::SuppressOutput) return;
    const bool bRestored = !bPowerSuppressionRequested && Generation == ContentGeneration
        && Power == PowerGeneration && RestoreSavedLighting();
    if (bRestored) { bUseSavedLighting = true; return; }
    // Acceptance/polling is blocked throughout both passes. A callback cannot relight an
    // already visited lamp while another lamp is still being suppressed or disposed.
    for (const auto& Owned : CurrentFurniture)
        if (IsValid(Owned.Get())) Owned->SuppressPower();
    bUseSavedLighting = !bPowerSuppressionRequested && Generation == ContentGeneration
        && !bClearing && !bEndingPlay && !bContentGenerationExhausted && IsRegistered()
        && Source.IsSet() && Presentation.IsSet();
}

bool UBaddieCompanionNativeApartment::OwnsScenePowerFurniture(const ABaddieCompanionFurnitureActor* Actor) const
{
    if (!Presentation.IsSet() || !IsValid(Actor)
        || Actor->GetOwner() != GetOwner() || Actor->GetWorld() != GetWorld()
        || !Actor->bConfigured || Actor->bDiscarded || Actor->IsActorBeingDestroyed()) return false;
    const int32 Index = Bindings.IndexOfByPredicate([&](const auto& Binding)
        { return Binding.Endpoint.RoomId.Equals(Actor->RoomId, ESearchCase::CaseSensitive); });
    auto* Stream = Streams.IsValidIndex(Index) ? Streams[Index].Get() : nullptr;
    if (!IsValid(Stream) || Stream->GetLevelStreamingState() != ELevelStreamingState::LoadedVisible
        || !IsValid(Stream->GetLoadedLevel()) || Stream->GetLoadedLevel()->GetWorld() != GetWorld()
        || Stream->GetLoadedLevel() != Actor->GetLevel()
        || !FurniturePrepared.IsValidIndex(Index) || !FurniturePrepared[Index]) return false;
    const auto* Room = Presentation->Rooms.FindByPredicate([&](const auto& Value)
        { return Value.Id.Equals(Actor->RoomId, ESearchCase::CaseSensitive); });
    return Room && Room->Items.ContainsByPredicate([&](const auto& Item)
        { return Item.Id.Equals(Actor->ItemId, ESearchCase::CaseSensitive); });
}

bool UBaddieCompanionNativeApartment::RestoreSavedLighting()
{
    // Baseline restoration is not an accepted utility event. Only an already retained native
    // presentation may use it; no actor, mesh or level is made visible here.
    if (bPowerSuppressionRequested || bClearing || bEndingPlay || bContentGenerationExhausted
        || !IsRegistered() || !Source.IsSet() || !Presentation.IsSet()
        || !BaddieCompanion::SamePresentationInput(Source.GetValue(), Presentation->Source))
    { bPowerSuppressionRequested = true; return false; }
    const auto Actors = Furniture;
    for (const auto& Owned : Actors)
    {
        if (!OwnsScenePowerFurniture(Owned.Get())) { bPowerSuppressionRequested = true; return false; }
        // An ordinary utility clear can run inside a material/light setter. Retry through the
        // existing owner poll after that stack unwinds, never through a recursive setter.
        if (Owned->bChangingPresentation) return false;
    }
    const uint64 Generation = ContentGeneration, Power = PowerGeneration;
    for (const auto& Owned : Actors)
    {
        if (!OwnsScenePowerFurniture(Owned.Get())) { bPowerSuppressionRequested = true; return false; }
        const bool bRestored = Owned->ApplyScenePower(1.f);
        if (!bRestored || bPowerSuppressionRequested || Generation != ContentGeneration || Power != PowerGeneration)
            return false;
    }
    // A later setter can affect a level used by an earlier actor without replacing its IDs.
    for (const auto& Owned : Actors)
        if (!OwnsScenePowerFurniture(Owned.Get())) { bPowerSuppressionRequested = true; return false; }
    return true;
}

bool UBaddieCompanionNativeApartment::AcceptScenePower(
    const BaddieCompanion::FAcceptedApartmentScenePower& Accepted)
{
    check(IsInGameThread());
    if (bClearingPower || bApplyingPower) return false;
    // An old callback must not clear or relight a replacement apartment, even with identical IDs.
    if (Accepted.Apartment.Get() != this || Accepted.ContentGeneration != ContentGeneration
        || Accepted.PowerGeneration != PowerGeneration) return false;
    if (bClearing || bEndingPlay || bContentGenerationExhausted || !IsRegistered()
        || !Source.IsSet() || !Presentation.IsSet()
        || !BaddieCompanion::SamePresentationInput(Source.GetValue(), Accepted.Source))
    { ClearScenePower(EScenePowerClear::SuppressOutput); return false; }
    if (!Accepted.Scope.IsSupported())
    { ClearScenePower(EScenePowerClear::RestoreSavedLighting); return false; }
    // The sealed input has no current producer. Matching a read-only snapshot alone cannot mint it.
    int32 MatchingLevels = 0;
    for (int32 I = 0; I < Bindings.Num(); ++I)
        if (Bindings[I].Endpoint.RoomId.Equals(Accepted.Scope.RoomId, ESearchCase::CaseSensitive)
            && Streams.IsValidIndex(I) && IsValid(Streams[I].Get()) && IsValid(Streams[I]->GetLoadedLevel())
            && Streams[I]->GetLevelStreamingState() == ELevelStreamingState::LoadedVisible
            && FurniturePrepared.IsValidIndex(I) && FurniturePrepared[I]) ++MatchingLevels;
    if (MatchingLevels != 1 || !Presentation->Rooms.ContainsByPredicate([&](const auto& Room)
        { return Room.Id.Equals(Accepted.Scope.RoomId, ESearchCase::CaseSensitive); }))
    { ClearScenePower(EScenePowerClear::RestoreSavedLighting); return false; }
    float Factor = 0;
    const double Now = FPlatformTime::Seconds();
    if (!FMath::IsFinite(Accepted.SampledAtSeconds) || Accepted.SampledAtSeconds <= 0
        || !FMath::IsFinite(Now) || Now < Accepted.SampledAtSeconds
        || std::floor((Now - Accepted.SampledAtSeconds) * 1000) > 60000
        || !BaddieCompanion::ProjectScenePowerFactor(Accepted.Sample,
            Accepted.Sample.SampledAtRealMs, Accepted.bReducedMotion, Factor))
    { ClearScenePower(EScenePowerClear::RestoreSavedLighting); return false; }
    auto* Prior = ScenePower.FindByPredicate([&](const auto& Entry)
        { return Entry.Scope.RoomId.Equals(Accepted.Scope.RoomId, ESearchCase::CaseSensitive); });
    if (Prior)
    {
        // Replays cannot extend freshness. A changed visit/zone requires explicit owner invalidation.
        if (!Prior->Scope.Matches(Accepted.Scope))
        { ClearScenePower(EScenePowerClear::RestoreSavedLighting); return false; }
        if (Accepted.Sample.SampledAtRealMs < Prior->Sample.SampledAtRealMs
            || Accepted.Sample.ObservedAtRealMs < Prior->Sample.ObservedAtRealMs)
        { ClearScenePower(EScenePowerClear::RestoreSavedLighting); return false; }
        if (Accepted.Sample.SampledAtRealMs == Prior->Sample.SampledAtRealMs) return false;
        if (Accepted.Sample.SampledAtRealMs < Prior->LastDisplayRealMs)
        { ClearScenePower(EScenePowerClear::RestoreSavedLighting); return false; }
    }
    FRetainedScenePower Next;
    Next.Scope = Accepted.Scope; Next.Sample = Accepted.Sample;
    Next.SampledAtSeconds = Accepted.SampledAtSeconds;
    Next.LastDisplayRealMs = Accepted.Sample.SampledAtRealMs;
    Next.bReducedMotion = Accepted.bReducedMotion;
    if (Prior) *Prior = MoveTemp(Next);
    else ScenePower.Add(MoveTemp(Next));
    bUseSavedLighting = false;
    PollScenePower();
    return ScenePower.ContainsByPredicate([&](const auto& Entry) { return Entry.Scope.Matches(Accepted.Scope); });
}

void UBaddieCompanionNativeApartment::PollScenePower()
{
    if (bClearingPower || bApplyingPower) return;
    if (ScenePower.IsEmpty())
    {
        if (!bUseSavedLighting) return;
        bool bRestored = false;
        {
            // Keep checking ownership after fallback too. This does not mint a utility sample,
            // refresh its age or invalidate an incoming token on every ordinary controller tick.
            TGuardValue<bool> Clearing(bClearingPower, true);
            bRestored = RestoreSavedLighting();
        }
        if (!bRestored) ClearScenePower(bPowerSuppressionRequested
            ? EScenePowerClear::SuppressOutput : EScenePowerClear::RestoreSavedLighting);
        return;
    }
    if (bClearing || bEndingPlay || bContentGenerationExhausted || !IsRegistered()
        || !Source.IsSet() || !Presentation.IsSet()
        || !BaddieCompanion::SamePresentationInput(Source.GetValue(), Presentation->Source))
    { ClearScenePower(EScenePowerClear::SuppressOutput); return; }
    TGuardValue<bool> Applying(bApplyingPower, true);
    const uint64 Generation = ContentGeneration, Power = PowerGeneration;
    const double Now = FPlatformTime::Seconds();
    // Copy before UE setters. A callback can clear both retained arrays.
    const auto Samples = ScenePower;
    const auto Actors = Furniture;
    // Rooms without a fresh sample may still display restored saved lighting. A sample
    // for another room must not stop checking their ownership and loaded visibility.
    for (const auto& Owned : Actors)
        if (!OwnsScenePowerFurniture(Owned.Get()))
        { ClearScenePower(EScenePowerClear::SuppressOutput); return; }
    for (const auto& Entry : Samples)
    {
        const double ElapsedMs = std::floor((Now - Entry.SampledAtSeconds) * 1000);
        if (!FMath::IsFinite(ElapsedMs) || ElapsedMs < 0 || ElapsedMs > 60000)
        { ClearScenePower(EScenePowerClear::RestoreSavedLighting); return; }
        const int64 SharedNow = Entry.Sample.SampledAtRealMs + static_cast<int64>(ElapsedMs);
        float Factor = 0;
        if (SharedNow < Entry.LastDisplayRealMs || !BaddieCompanion::ProjectScenePowerFactor(
            Entry.Sample, SharedNow, Entry.bReducedMotion, Factor))
        { ClearScenePower(EScenePowerClear::RestoreSavedLighting); return; }
        const int32 Index = Bindings.IndexOfByPredicate([&](const auto& Binding)
            { return Binding.Endpoint.RoomId.Equals(Entry.Scope.RoomId, ESearchCase::CaseSensitive); });
        auto* Stream = Streams.IsValidIndex(Index) ? Streams[Index].Get() : nullptr;
        auto* Level = IsValid(Stream) ? Stream->GetLoadedLevel() : nullptr;
        // Hidden/removing/unloaded levels lose utility state without making content visible.
        if (!IsValid(Level) || Level->GetWorld() != GetWorld()
            || Stream->GetLevelStreamingState() != ELevelStreamingState::LoadedVisible)
        { ClearScenePower(EScenePowerClear::SuppressOutput); return; }
        const auto* Room = Presentation->Rooms.FindByPredicate([&](const auto& Value)
            { return Value.Id.Equals(Entry.Scope.RoomId, ESearchCase::CaseSensitive); });
        if (!Room) { ClearScenePower(EScenePowerClear::SuppressOutput); return; }
        // Do not hold Room across a native callback.
        const auto Items = Room->Items;
        for (const auto& Owned : Actors)
        {
            auto* Actor = Owned.Get();
            if (!IsValid(Actor)) { ClearScenePower(EScenePowerClear::SuppressOutput); return; }
            if (!Actor->RoomId.Equals(Entry.Scope.RoomId, ESearchCase::CaseSensitive)) continue;
            if (Actor->GetOwner() != GetOwner() || Actor->GetWorld() != GetWorld() || Actor->GetLevel() != Level
                || !Items.ContainsByPredicate([&](const auto& Item)
                    { return Item.Id.Equals(Actor->ItemId, ESearchCase::CaseSensitive); }))
            { ClearScenePower(EScenePowerClear::SuppressOutput); return; }
            const bool bApplied = Actor->ApplyScenePower(Factor);
            if (ContentGeneration != Generation || PowerGeneration != Power) return;
            if (!bApplied) { ClearScenePower(EScenePowerClear::SuppressOutput); return; }
        }
        auto* Retained = ScenePower.FindByPredicate([&](const auto& Value) { return Value.Scope.Matches(Entry.Scope); });
        if (!Retained) { ClearScenePower(EScenePowerClear::SuppressOutput); return; }
        Retained->LastDisplayRealMs = SharedNow;
    }
    // A setter in a sampled room can invalidate an unsampled room during this same poll.
    for (const auto& Owned : Actors)
        if (!OwnsScenePowerFurniture(Owned.Get()))
        { ClearScenePower(EScenePowerClear::SuppressOutput); return; }
}

bool UBaddieCompanionNativeApartment::PrepareContent(const BaddieCompanion::FHomePresentationPlan& Plan,
    const BaddieCompanion::FNativeApartmentContent& Content)
{
    if (bClearing || bEndingPlay || bContentGenerationExhausted || !IsRegistered()) return false;
    ClearPrivateContent();
    if (bContentGenerationExhausted) return false;
    ObserveValidPresentation(Plan);
    using namespace BaddieCompanion;
    using namespace BaddieCompanion::NativeApartment;
    if (!SamePresentationInput(Plan.Source, Content.Source) || Content.Levels.IsEmpty())
    {
        AddGap(ENativeApartmentGap::InvalidBinding);
        return false;
    }
    if (!ABaddieCompanionCharacterActor::AcceptsContent(Plan, Content.Character))
    { AddGap(ENativeApartmentGap::CharacterUnavailable); return false; }
    TArray<EItemKind> AssetKinds;
    if (Content.FurnitureAssets.Num() > 23) { AddGap(ENativeApartmentGap::InvalidBinding); return false; }
    for (const auto& Asset : Content.FurnitureAssets)
    {
        if (AssetKinds.Contains(Asset.Kind)) { AddGap(ENativeApartmentGap::InvalidBinding); return false; }
        AssetKinds.Add(Asset.Kind);
    }
    TArray<FSoftObjectPath> FurniturePaths;
    for (const auto& Room : Plan.Rooms)
        for (const auto& Item : Room.Items)
        {
            const auto* Asset = Item.Kind.Effective.IsSet() ? Content.FurnitureAssets.FindByPredicate([&](const auto& Entry)
                { return Entry.Kind == Item.Kind.Effective.GetValue(); }) : nullptr;
            if (!Room.bSourceMapped || !Room.Shell.IsSet() || !Item.PlacementPose.IsSet()
                || !Item.Dimensions.IsSet() || !Item.Volume.IsSet() || !Asset
                || !ABaddieCompanionFurnitureActor::AcceptsBinding(Item, *Asset))
            { AddGap(ENativeApartmentGap::FurnitureUnavailable); return false; }
            FurniturePaths.AddUnique(Asset->Mesh.ToSoftObjectPath());
        }
    // Exact endpoint coverage: no missing inventory synthesized into common-space content.
    TArray<FApartmentEndpoint> Required;
    for (const auto& Connection : Plan.Apartment.Connections)
        for (const auto& Endpoint : { Connection.From, Connection.To })
            if (!Required.ContainsByPredicate([&](const auto& Other) { return SameEndpoint(Other, Endpoint); }))
                Required.Add(Endpoint);
    if (Required.Num() != Content.Levels.Num()) { AddGap(ENativeApartmentGap::InvalidBinding); return false; }
    TArray<FApartmentEndpoint> Seen;
    int32 HomeLevels = 0;
    for (const auto& Binding : Content.Levels)
    {
        if (Binding.Endpoint.RoomId.Equals(Plan.Home.Id, ESearchCase::CaseSensitive)) ++HomeLevels;
        if (Binding.Level.IsNull()) { AddGap(ENativeApartmentGap::MissingContent); return false; }
        if (!RigidInstance(Binding.InstanceToApartment)
            || !Required.ContainsByPredicate([&](const auto& E) { return SameEndpoint(E, Binding.Endpoint); })
            || Seen.ContainsByPredicate([&](const auto& E) { return SameEndpoint(E, Binding.Endpoint); }))
        { AddGap(ENativeApartmentGap::InvalidBinding); return false; }
        Seen.Add(Binding.Endpoint);
        TArray<int32> DoorIndices;
        TArray<FName> AnchorActors;
        for (const auto& Door : Binding.Doors)
        {
            if (!Plan.Apartment.Connections.IsValidIndex(Door.ConnectionIndex)
                || Door.DoorActor.IsNone() || Door.ApproachActor.IsNone() || Door.DoorActor == Door.ApproachActor
                || DoorIndices.Contains(Door.ConnectionIndex) || AnchorActors.Contains(Door.DoorActor)
                || AnchorActors.Contains(Door.ApproachActor))
            { AddGap(ENativeApartmentGap::InvalidBinding); return false; }
            DoorIndices.Add(Door.ConnectionIndex);
            AnchorActors.Add(Door.DoorActor);
            AnchorActors.Add(Door.ApproachActor);
            const auto& Connection = Plan.Apartment.Connections[Door.ConnectionIndex];
            if (!SameEndpoint(Binding.Endpoint, Door.bFromEndpoint ? Connection.From : Connection.To))
            { AddGap(ENativeApartmentGap::InvalidBinding); return false; }
            FNativeDoorwayPose Pose;
            if (!ResolveLocalDoorway(Plan, Binding, Door, Pose))
            { AddGap(ENativeApartmentGap::DoorwayNotAuthored); return false; }
            if (Binding.InstanceToApartment.TransformPosition(Pose.DoorCm).ContainsNaN()
                || Binding.InstanceToApartment.TransformPosition(Pose.PreferredApproachCm).ContainsNaN())
            { AddGap(ENativeApartmentGap::InvalidBinding); return false; }
        }
        for (int32 I = 0; I < Plan.Apartment.Connections.Num(); ++I)
        {
            const auto& C = Plan.Apartment.Connections[I];
            if ((SameEndpoint(C.From, Binding.Endpoint) || SameEndpoint(C.To, Binding.Endpoint)) && !DoorIndices.Contains(I))
            { AddGap(ENativeApartmentGap::AnchorUnavailable); return false; }
        }
    }
    if (HomeLevels != 1) { AddGap(ENativeApartmentGap::CharacterUnavailable); return false; }
    // Content setup, topology and configuration are not consent or gameplay admission.
    // No admitted standalone owner exists yet. No caller-supplied admission boolean is accepted.
    constexpr bool bStandaloneApartmentAdmissionImplemented = false;
    if (!Plan.Apartment.bCompleteHomeInventoryVerified || !Plan.IsRenderReady()
        || !bStandaloneApartmentAdmissionImplemented) return false;
    if (!GetWorld() || !GetWorld()->IsGameWorld() || GetWorld()->GetNetMode() != NM_Standalone) return false;

    Source = Plan.Source; Presentation = Plan; Bindings = Content.Levels;
    FurnitureAssets = Content.FurnitureAssets;
    CharacterContent = Content.Character;
    Status = {};
    AddGap(ENativeApartmentGap::ScenePowerUnavailable);
    if (!UAssetManager::IsInitialized())
    { ClearPrivateContent(); AddGap(ENativeApartmentGap::CharacterUnavailable); return false; }
    TArray<FSoftObjectPath> CharacterPaths { Content.Character.ActorClass.ToSoftObjectPath() };
    for (const auto& Material : Content.Character.Materials)
    {
        CharacterPaths.AddUnique(Material.MeshAsset.ToSoftObjectPath());
        CharacterPaths.AddUnique(Material.AuthoredMaterial.ToSoftObjectPath());
    }
    for (const auto& Accessory : Content.Character.Accessories)
        CharacterPaths.AddUnique(Accessory.MeshAsset.ToSoftObjectPath());
    for (const auto& FaceNode : Content.Character.FaceNodes)
        for (const auto& Mesh : FaceNode.Meshes)
            CharacterPaths.AddUnique(Mesh.MeshAsset.ToSoftObjectPath());
    CharacterAssetLoad = UAssetManager::GetStreamableManager().RequestAsyncLoad(CharacterPaths);
    if (!CharacterAssetLoad.IsValid())
    { ClearPrivateContent(); AddGap(ENativeApartmentGap::CharacterUnavailable); return false; }
    Status.bCharacterAssetsPending = !CharacterAssetLoad->HasLoadCompleted();
    if (!FurniturePaths.IsEmpty())
    {
        // UE retains mesh dependencies until every native furniture instance owns its mesh.
        // No callbacks capture private snapshots; the existing controller poll owns completion.
        if (!UAssetManager::IsInitialized())
        { ClearPrivateContent(); AddGap(ENativeApartmentGap::FurnitureUnavailable); return false; }
        FurnitureAssetLoad = UAssetManager::GetStreamableManager().RequestAsyncLoad(FurniturePaths);
        if (!FurnitureAssetLoad.IsValid())
        { ClearPrivateContent(); AddGap(ENativeApartmentGap::FurnitureUnavailable); return false; }
        Status.bFurnitureAssetsPending = !FurnitureAssetLoad->HasLoadCompleted();
    }
    for (const auto& Binding : Bindings)
    {
        // Native packages bake source basis + saved shell design + authored fixed fixtures once.
        // Saved item actors are added separately with their existing room-local placements.
        ULevelStreamingDynamic::FLoadLevelInstanceParams Params(GetWorld(),
            Binding.Level.ToSoftObjectPath().GetLongPackageName(), Binding.InstanceToApartment);
        Params.bInitiallyVisible = false;
        Params.bAllowReuseExitingLevelStreaming = false;
        bool bLoaded = false;
        auto* Stream = ULevelStreamingDynamic::LoadLevelInstance(Params, bLoaded);
        if (Stream) { Streams.Add(Stream); AnchorsChecked.Add(false); FurniturePrepared.Add(false); }
        if (!bLoaded || !Stream)
        { ClearPrivateContent(); AddGap(ENativeApartmentGap::LoadFailed); return false; }
    }
    Status.PendingLevels = Streams.Num();
    // Do not set visible or ready here. Authored anchors, navigation and rendered acceptance are separate.
    AddGap(ENativeApartmentGap::RenderAcceptanceOutstanding);
    return true;
}

void UBaddieCompanionNativeApartment::PollNativeLevels()
{
    if (bClearing || bEndingPlay) return;
    const uint64 PollGeneration = ContentGeneration;
    if (CharacterAssetLoad.IsValid() && (CharacterAssetLoad->WasCanceled() || !CharacterAssetLoad->IsActive()))
    { ClearPrivateContent(); AddGap(BaddieCompanion::ENativeApartmentGap::CharacterUnavailable); return; }
    Status.bCharacterAssetsPending = CharacterAssetLoad.IsValid() && !CharacterAssetLoad->HasLoadCompleted();
    if (FurnitureAssetLoad.IsValid() && (FurnitureAssetLoad->WasCanceled() || !FurnitureAssetLoad->IsActive()))
    { ClearPrivateContent(); AddGap(BaddieCompanion::ENativeApartmentGap::FurnitureUnavailable); return; }
    Status.bFurnitureAssetsPending = FurnitureAssetLoad.IsValid() && !FurnitureAssetLoad->HasLoadCompleted();
    Status.PendingLevels = 0;
    for (int32 I = 0; I < Streams.Num(); ++I)
    {
        auto* Stream = Streams[I].Get();
        if (!IsValid(Stream) || Stream->GetLevelStreamingState() == ELevelStreamingState::FailedToLoad)
        { ClearPrivateContent(); AddGap(BaddieCompanion::ENativeApartmentGap::LoadFailed); return; }
        if (!Stream->GetLoadedLevel()) { ++Status.PendingLevels; continue; }
        if (!AnchorsChecked[I])
        {
            if (!ValidateLoadedAnchors(I))
            { ClearPrivateContent(); AddGap(BaddieCompanion::ENativeApartmentGap::AnchorUnavailable); return; }
            AnchorsChecked[I] = true;
        }
        if (!Status.bFurnitureAssetsPending && !FurniturePrepared[I])
        {
            const bool bFurnitureReady = PrepareRoomFurniture(I);
            if (ContentGeneration != PollGeneration) return;
            if (!bFurnitureReady)
            { ClearPrivateContent(); AddGap(BaddieCompanion::ENativeApartmentGap::FurnitureUnavailable); return; }
            FurniturePrepared[I] = true;
        }
        if (!Status.bCharacterAssetsPending && !Status.bCharacterPrepared && Presentation.IsSet()
            && Bindings[I].Endpoint.RoomId.Equals(Presentation->Home.Id, ESearchCase::CaseSensitive))
        {
            const bool bCharacterReady = PrepareHomeCharacter(I);
            if (ContentGeneration != PollGeneration) return;
            if (!bCharacterReady)
            { ClearPrivateContent(); AddGap(BaddieCompanion::ENativeApartmentGap::CharacterUnavailable); return; }
            Status.bCharacterPrepared = true;
        }
    }
    if (Status.bCharacterPrepared)
    {
        auto* Actor = PreparedCharacter.Get();
        const bool bCurrent = IsValid(Actor) && Actor->GetOwner() == GetOwner() && Actor->GetWorld() == GetWorld()
            && Streams.ContainsByPredicate([&](const auto& Stream)
                { return IsValid(Stream.Get()) && Stream->GetLoadedLevel() == Actor->GetLevel(); })
            && Actor->ReconcileLevelTransform();
        if (ContentGeneration != PollGeneration) return;
        if (!bCurrent)
        { ClearPrivateContent(); AddGap(BaddieCompanion::ENativeApartmentGap::CharacterUnavailable); return; }
        if (CharacterAssetLoad.IsValid())
        {
            CharacterAssetLoad->ReleaseHandle();
            CharacterAssetLoad.Reset();
        }
    }
    // Native setters can invalidate this owner. Do not iterate storage a callback can clear.
    const auto CurrentFurniture = Furniture;
    for (const auto& OwnedFurniture : CurrentFurniture)
    {
        auto* Actor = OwnedFurniture.Get();
        const bool bCurrent = IsValid(Actor) && Actor->GetOwner() == GetOwner() && Actor->GetWorld() == GetWorld()
            && Streams.ContainsByPredicate([&](const auto& Stream)
                { return IsValid(Stream.Get()) && Stream->GetLoadedLevel() == Actor->GetLevel(); })
            && Actor->ReconcileLevelTransform();
        if (ContentGeneration != PollGeneration) return;
        if (!bCurrent)
        { ClearPrivateContent(); AddGap(BaddieCompanion::ENativeApartmentGap::FurnitureUnavailable); return; }
    }
    PollScenePower();
    if (ContentGeneration != PollGeneration) return;
    if (!Streams.IsEmpty() && Status.PendingLevels == 0 && !Status.bFurnitureAssetsPending && FurnitureAssetLoad.IsValid())
    {
        FurnitureAssetLoad->ReleaseHandle();
        FurnitureAssetLoad.Reset();
    }
    if (!Streams.IsEmpty() && Status.PendingLevels == 0 && !Status.bFurnitureAssetsPending && !bDaylightHookAttempted)
    {
        // Invisible level content is not yet an illuminated scene. Retry only when UE makes it visible.
        bool bAllVisible = true;
        for (const auto& Stream : Streams)
            bAllVisible = bAllVisible && Stream->GetLevelStreamingState() == ELevelStreamingState::LoadedVisible;
        if (bAllVisible)
        {
            bDaylightHookAttempted = true;
            if (!RefreshAuthoredDaylight())
            {
                ClearPrivateContent();
                AddGap(BaddieCompanion::ENativeApartmentGap::DaylightUnverified);
            }
        }
    }
}

bool UBaddieCompanionNativeApartment::PrepareHomeCharacter(int32 Index)
{
    if (bClearing || bEndingPlay || !Presentation.IsSet() || !CharacterContent.IsSet() || PreparedCharacter
        || !Streams.IsValidIndex(Index) || !Bindings.IsValidIndex(Index)) return false;
    auto* Stream = Streams[Index].Get();
    auto* Level = IsValid(Stream) ? Stream->GetLoadedLevel() : nullptr;
    auto* Class = CharacterContent->ActorClass.Get();
    if (!IsValid(Level) || Level->GetWorld() != GetWorld()
        || !ABaddieCompanionCharacterActor::HasInertDefaults(Class)
        || !Bindings[Index].Endpoint.RoomId.Equals(Presentation->Home.Id, ESearchCase::CaseSensitive)) return false;
    // Spawn/asset callbacks must not retain references into a cleared owner.
    const uint64 Generation = ContentGeneration;
    const auto Plan = Presentation.GetValue();
    const auto Content = CharacterContent.GetValue();
    const auto Instance = Bindings[Index].InstanceToApartment;
    FActorSpawnParameters Params;
    Params.OverrideLevel = Level; Params.Owner = GetOwner(); Params.ObjectFlags |= RF_Transient;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Actor = GetWorld()->SpawnActor<ABaddieCompanionCharacterActor>(Class, FTransform::Identity, Params);
    if (!IsValid(Actor)) return false;
    if (Generation != ContentGeneration)
    {
        const TWeakObjectPtr<ABaddieCompanionCharacterActor> Current(Actor);
        if (!Current->DestroyPrivateCharacter())
            AddGap(BaddieCompanion::ENativeApartmentGap::CharacterUnavailable);
        return false;
    }
    PreparedCharacter = Actor;
    if (Actor->GetLevel() != Level || Actor->GetWorld() != GetWorld() || Actor->GetOwner() != GetOwner()) return false;
    const TWeakObjectPtr<ABaddieCompanionCharacterActor> Current(Actor);
    const bool bPrepared = Actor->Prepare(Plan, Content, Instance, Level->bAlreadyMovedActors);
    if (Generation != ContentGeneration)
    {
        if (Current.IsValid())
        {
            if (!Current->DestroyPrivateCharacter())
                AddGap(BaddieCompanion::ENativeApartmentGap::CharacterUnavailable);
        }
        return false;
    }
    return bPrepared;
}

bool UBaddieCompanionNativeApartment::PrepareRoomFurniture(int32 Index)
{
    using namespace BaddieCompanion;
    if (bClearing || bEndingPlay || !Presentation.IsSet()
        || !Streams.IsValidIndex(Index) || !Bindings.IsValidIndex(Index)) return false;
    auto* Stream = Streams[Index].Get();
    auto* Level = Stream ? Stream->GetLoadedLevel() : nullptr;
    if (!IsValid(Level) || !GetWorld() || Level->GetWorld() != GetWorld()) return false;
    const uint64 Generation = ContentGeneration;
    const auto Binding = Bindings[Index];
    if (Binding.Endpoint.RoomId.IsEmpty()) return true;
    const auto* Room = Presentation->Rooms.FindByPredicate([&](const auto& Entry)
        { return Entry.Id.Equals(Binding.Endpoint.RoomId, ESearchCase::CaseSensitive); });
    if (!Room) return false;
    const auto SavedRoom = *Room;
    const auto Assets = FurnitureAssets;
    for (const auto& Item : SavedRoom.Items)
    {
        const auto* Asset = Item.Kind.Effective.IsSet() ? Assets.FindByPredicate([&](const auto& Entry)
            { return Entry.Kind == Item.Kind.Effective.GetValue(); }) : nullptr;
        if (!Asset || !IsValid(Asset->Mesh.Get())) return false;
        FActorSpawnParameters Params;
        Params.OverrideLevel = Level;
        Params.Owner = GetOwner();
        Params.ObjectFlags |= RF_Transient;
        // Collision cannot silently move a saved item to a different coordinate.
        // This native actor starts hidden, non-colliding, non-replicated and without a tick.
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        auto* Actor = GetWorld()->SpawnActor<ABaddieCompanionFurnitureActor>(
            ABaddieCompanionFurnitureActor::StaticClass(), FTransform::Identity, Params);
        if (!IsValid(Actor)) return false;
        if (Generation != ContentGeneration)
        {
            if (!Actor->DestroyPrivateFurniture()) AddGap(ENativeApartmentGap::FurnitureUnavailable);
            return false;
        }
        Furniture.Add(Actor);
        if (Actor->GetLevel() != Level || Actor->GetWorld() != GetWorld() || Actor->GetOwner() != GetOwner()) return false;
        const TWeakObjectPtr<ABaddieCompanionFurnitureActor> Current(Actor);
        const bool bPrepared = Actor->Configure(SavedRoom, Item, *Asset, Binding.InstanceToApartment, Level->bAlreadyMovedActors);
        if (Generation != ContentGeneration)
        {
            if (Current.IsValid() && !Current->DestroyPrivateFurniture()) AddGap(ENativeApartmentGap::FurnitureUnavailable);
            return false;
        }
        if (!bPrepared) return false;
    }
    Status.PreparedFurniture = Furniture.Num();
    return true;
}

bool UBaddieCompanionNativeApartment::ValidateLoadedAnchors(int32 Index)
{
    using namespace BaddieCompanion;
    using namespace BaddieCompanion::NativeApartment;
    if (!Presentation.IsSet() || !Streams.IsValidIndex(Index) || !Bindings.IsValidIndex(Index)) return false;
    auto* Stream = Streams[Index].Get();
    auto* Level = Stream ? Stream->GetLoadedLevel() : nullptr;
    if (!Level) return false;
    const auto& Binding = Bindings[Index];
    TArray<FVector> DoorPositions;
    for (const auto& Door : Binding.Doors)
    {
        auto* Anchor = ActorNamed(Level, Door.DoorActor);
        auto* Approach = ActorNamed(Level, Door.ApproachActor);
        if (!Anchor || !Approach || Anchor->GetActorLocation().ContainsNaN()
            || Approach->GetActorLocation().ContainsNaN()) return false;
        const FVector DoorPosition = Anchor->GetActorLocation();
        if (DoorPositions.ContainsByPredicate([&](const FVector& Position) { return Position.Equals(DoorPosition, .1); }))
            return false;
        DoorPositions.Add(DoorPosition);
        // Validate every opening against its measured source or explicit authored local pose.
        FNativeDoorwayPose Expected;
        if (!ResolveLocalDoorway(Presentation.GetValue(), Binding, Door, Expected)) return false;
        // UE owns when instance transforms are applied during streaming.
        const FVector AnchorLocal = Level->bAlreadyMovedActors
            ? Binding.InstanceToApartment.InverseTransformPosition(Anchor->GetActorLocation()) : Anchor->GetActorLocation();
        const FVector ApproachLocal = Level->bAlreadyMovedActors
            ? Binding.InstanceToApartment.InverseTransformPosition(Approach->GetActorLocation()) : Approach->GetActorLocation();
        if (!AnchorLocal.Equals(Expected.DoorCm, .1)
            || !ApproachLocal.Equals(Expected.PreferredApproachCm, .1)) return false;
    }
    // Two endpoint anchors must coincide after whole-level transforms, not teleport across gaps.
    for (int32 C = 0; C < Presentation->Apartment.Connections.Num(); ++C)
    {
        TArray<FVector> Points;
        for (int32 I = 0; I < Bindings.Num(); ++I)
        {
            auto* OtherLevel = Streams[I] ? Streams[I]->GetLoadedLevel() : nullptr;
            if (!OtherLevel) continue;
            for (const auto& Door : Bindings[I].Doors)
                if (Door.ConnectionIndex == C)
                {
                    auto* Anchor = ActorNamed(OtherLevel, Door.DoorActor);
                    if (!Anchor) return false;
                    Points.Add(OtherLevel->bAlreadyMovedActors ? Anchor->GetActorLocation()
                        : Bindings[I].InstanceToApartment.TransformPosition(Anchor->GetActorLocation()));
                }
        }
        if (Points.Num() == 2 && !Points[0].Equals(Points[1], .1)) return false;
    }
    Status.BoundDoorways = 0;
    for (int32 I = 0; I < Bindings.Num(); ++I)
        if (I == Index || AnchorsChecked[I]) Status.BoundDoorways += Bindings[I].Doors.Num();
    return true; // Anchor agreement is not native collision/navmesh clearance.
}

bool UBaddieCompanionNativeApartment::CopyNavigationApproach(ACharacter& Character,
    int32 ConnectionIndex, bool bFromEndpoint, FVector& Out)
{
    Out = FVector::ZeroVector;
    if (Character.GetWorld() != GetWorld() || !Presentation.IsSet()) return false;
    auto* Movement = Character.GetCharacterMovement();
    auto* Navigation = UNavigationSystemV1::GetCurrent(GetWorld());
    if (!Movement || !Navigation) return false;
    for (int32 I = 0; I < Bindings.Num(); ++I)
    {
        if (!Streams[I] || Streams[I]->GetLevelStreamingState() != ELevelStreamingState::LoadedVisible) continue;
        for (const auto& Door : Bindings[I].Doors)
            if (Door.ConnectionIndex == ConnectionIndex && Door.bFromEndpoint == bFromEndpoint)
            {
                auto* Approach = BaddieCompanion::NativeApartment::ActorNamed(Streams[I]->GetLoadedLevel(), Door.ApproachActor);
                if (!Approach) return false;
                FNavLocation Projected;
                const auto& Agent = Movement->GetNavAgentPropertiesRef();
                auto* NavData = Navigation->GetNavDataForProps(Agent, Character.GetActorLocation());
                if (!NavData) return false;
                if (!Navigation->ProjectPointToNavigation(Approach->GetActorLocation(), Projected,
                    FVector(2, 2, 2), &Agent) || !Projected.Location.Equals(Approach->GetActorLocation(), 2)) return false;
                FPathFindingQuery Query(&Character, *NavData, Character.GetNavAgentLocation(),
                    Projected.Location, NavData->GetDefaultQueryFilter());
                Query.SetAllowPartialPaths(false).SetNavAgentProperties(Agent);
                if (!Navigation->TestPathSync(Query)) return false;
                Out = Projected.Location;
                return true; // Native navigable candidate only: not safe spawn, command or admission.
            }
    }
    return false;
}

bool UBaddieCompanionNativeApartment::RefreshAuthoredDaylight()
{
    using namespace BaddieCompanion::NativeApartment;
    for (int32 I = 0; I < Bindings.Num(); ++I)
    {
        if (Bindings[I].Endpoint.Space != BaddieCompanion::EApartmentSpace::LivingRoom
            || !Streams[I] || Streams[I]->GetLevelStreamingState() != ELevelStreamingState::LoadedVisible) continue;
        auto* Level = Streams[I]->GetLoadedLevel();
        const auto& B = Bindings[I];
        auto* Window = ComponentNamed<UPrimitiveComponent>(Level, B.WindowActor);
        auto* Sky = ComponentNamed<USkyLightComponent>(Level, B.SkyActor);
        auto* Sun = ComponentNamed<UDirectionalLightComponent>(Level, B.SunActor);
        auto* Atmosphere = ComponentNamed<USkyAtmosphereComponent>(Level, B.AtmosphereActor);
        auto* Exposure = ComponentNamed<UPostProcessComponent>(Level, B.ExposureActor);
        if (!Window || !Sky || !Sun || !Atmosphere || !Exposure || !ActorNamed(Level, B.SeatingViewActor)) return false;
        // Keep authored intensity, solar direction, window transmission and exposure values.
        // Queue a capture after native streaming, not a per-tick costly RecaptureSky().
        Sky->SetCaptureIsDirty();
        AddGap(BaddieCompanion::ENativeApartmentGap::DaylightUnverified);
        return true; // Native hook only; no claim about illumination or exterior view.
    }
    return false;
}

bool UBaddieCompanionNativeApartment::CopyRoomCamera(const FString& RoomId,
    BaddieCompanion::FNativeRoomCamera& Out)
{
    Out = {};
    if (!Presentation.IsSet()) return false;
    const auto* Room = Presentation->Rooms.FindByPredicate([&](const auto& R)
        { return R.Id.Equals(RoomId, ESearchCase::CaseSensitive); });
    if (!Room || !Room->Shell.IsSet()) return false;
    for (int32 I = 0; I < Bindings.Num(); ++I)
        if (Bindings[I].Endpoint.RoomId.Equals(RoomId, ESearchCase::CaseSensitive)
            && Streams[I] && Streams[I]->GetLevelStreamingState() == ELevelStreamingState::LoadedVisible)
        {
            // The source camera is already quarter-turned. Only apply the whole-room apartment transform.
            Out.SourceRoom = Room->Shell->Wide;
            Out.EyeApartmentCm = Bindings[I].InstanceToApartment.TransformPosition(Out.SourceRoom.EyeUnrealCm);
            Out.TargetApartmentCm = Bindings[I].InstanceToApartment.TransformPosition(Out.SourceRoom.TargetUnrealCm);
            if (Out.EyeApartmentCm.ContainsNaN() || Out.TargetApartmentCm.ContainsNaN()) { Out = {}; return false; }
            return true; // Existing controller/CameraManager owns view adoption; no new camera actor.
        }
    return false;
}
