#include "BaddieCompanionCharacterEquipment.h"

#include "BaddieCompanionCharacterActor.h"
#include "Components/SceneComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/SkinnedAsset.h"
#include "Engine/StaticMesh.h"
#include "Engine/StreamableManager.h"
#include "Materials/MaterialInterface.h"

namespace BaddieCompanion::CharacterEquipment
{
    struct FCatalog
    {
        const TCHAR* Id;
        const TCHAR* Key;
        const TCHAR* Path;
        const TCHAR* Sha;
        const TCHAR* Parent;
        int32 Slot, MeshCount;
        FVector Position;
        int32 MaterialIndices[6];
    };
    // Exact companionAssets identities and fittings.json positions. Hash-matched local GLB
    // headers have identity roots and direct static mesh children, with no skins or animations.
    const FCatalog Catalog[] = {
        { TEXT("ribbon-ball"), TEXT("bestieShopRibbonBall"), TEXT("bestie/shop/v1/ribbon-ball.a7dd51d14ac9e881.glb"),
          TEXT("a7dd51d14ac9e881fae48d60d672dc122d2a73a7b8c59bc84b22db9574866491"), TEXT("model"), 0, 4, FVector(.86, 0, .25), {0, 1, 2, 3} },
        { TEXT("stacking-rings"), TEXT("bestieShopStackingRings"), TEXT("bestie/shop/v1/stacking-rings.987b3dd9942b84df.glb"),
          TEXT("987b3dd9942b84dfa9deb654afc81e2b3658a8b62a72b4e9293fbd7211fd11a7"), TEXT("model"), 0, 5, FVector(.86, 0, .25), {0, 1, 2, 3, 4} },
        { TEXT("cuddle-bear"), TEXT("bestieShopCuddleBear"), TEXT("bestie/shop/v1/cuddle-bear.8f50d7d2d9b956b7.glb"),
          TEXT("8f50d7d2d9b956b7e603e6f0915fd77965a610168ce08715cdd93fc1ee2ee4ee"), TEXT("model"), 0, 6, FVector(.86, 0, .25), {0, 1, 1, 2, 3, 4} },
        { TEXT("round-glasses"), TEXT("bestieShopRoundGlasses"), TEXT("bestie/shop/v1/round-glasses.79ab93ea78694986.glb"),
          TEXT("79ab93ea786949864445c53f77d554d592d64b6f35e973a0ef530c26130ac1b0"), TEXT("Head"), 1, 4, FVector(0, .405, .515), {0, 1, 0, 2} },
        { TEXT("linen-scarf"), TEXT("bestieShopLinenScarf"), TEXT("bestie/shop/v1/linen-scarf.e9da1d1e5a09e2de.glb"),
          TEXT("e9da1d1e5a09e2de0ea1aac78d81ba68a38fbfe0201ea1af1e739dcaa16231d4"), TEXT("Chest"), 2, 4, FVector(0, .82, .03), {0, 1, 2, 3} },
        { TEXT("daisy-pin"), TEXT("bestieShopDaisyPin"), TEXT("bestie/shop/v1/daisy-pin.317a48a4002c62ec.glb"),
          TEXT("317a48a4002c62ec493a816c431f07791b0dfc746de1cf22cb8750aea3b133d7"), TEXT("Chest"), 3, 4, FVector(-.26, .6, .33), {0, 1, 2, 0} }
    };
    const FCatalog* Find(const FString& Id)
    {
        for (const auto& Item : Catalog)
            if (Id.Equals(Item.Id, ESearchCase::CaseSensitive)) return &Item;
        return nullptr;
    }
    bool ValidComponent(const ABaddieCompanionCharacterActor& Actor, const USceneComponent* Component)
    {
        return IsValid(Component) && Component->GetOwner() == &Actor && Component->GetWorld() == Actor.GetWorld()
            && Component->IsRegistered() && Component->GetMobility() == EComponentMobility::Movable
            && !Component->IsUsingAbsoluteLocation() && !Component->IsUsingAbsoluteRotation()
            && !Component->IsUsingAbsoluteScale();
    }
    UObject* AnchorAsset(USceneComponent* Component)
    {
        if (auto* Static = Cast<UStaticMeshComponent>(Component)) return Static->GetStaticMesh();
        if (auto* Skinned = Cast<USkinnedMeshComponent>(Component)) return Skinned->GetSkinnedAsset();
        return nullptr;
    }
    bool MaterialsMatch(const UStaticMeshComponent& Mesh, const FNativeEquipmentMesh& Binding)
    {
        const auto Names = Mesh.GetMaterialSlotNames();
        if (Names.Num() != Binding.Materials.Num() || !Mesh.OverrideMaterials.IsEmpty()) return false;
        TSet<FName> Seen;
        for (int32 I = 0; I < Names.Num(); ++I)
        {
            if (Seen.Contains(Names[I])) return false;
            Seen.Add(Names[I]);
            const auto* Slot = Binding.Materials.FindByPredicate([&](const auto& Value) { return Value.Slot == Names[I]; });
            if (!Slot || !IsValid(Slot->Material.Get()) || Mesh.GetMaterial(I) != Slot->Material.Get()) return false;
        }
        return true;
    }
}

bool BaddieCompanion::FNativeCharacterEquipment::BindGroup(ABaddieCompanionCharacterActor& Actor,
    FGroupState& Out, const TSet<FName>& ProtectedComponents, TSet<FName>& Claimed)
{
    using namespace CharacterEquipment;
    TInlineComponentArray<USceneComponent*> Components(&Actor);
    const auto Resolve = [&](FName Name) -> USceneComponent*
    {
        USceneComponent* Result = nullptr;
        if (Name.IsNone()) return nullptr;
        for (auto* Component : Components)
            if (IsValid(Component) && Component->GetFName() == Name)
            { if (Result) return nullptr; Result = Component; }
        return Result;
    };
    const auto& B = Out.Binding;
    auto* Root = Resolve(B.RootComponent);
    auto* Anchor = Resolve(B.AttachmentComponent);
    if (!ValidComponent(Actor, Root) || Root->GetClass() != USceneComponent::StaticClass()
        || Root == Actor.GetRootComponent() || ProtectedComponents.Contains(B.RootComponent)
        || Claimed.Contains(B.RootComponent) || !ValidComponent(Actor, Anchor) || Root == Anchor
        || (!B.AttachmentSocket.IsNone() && !Anchor->DoesSocketExist(B.AttachmentSocket))
        || (Cast<UMeshComponent>(Anchor) ? !IsValid(B.AttachmentMeshAsset.Get())
            || B.AttachmentMeshAsset.Get() != AnchorAsset(Anchor) : !B.AttachmentMeshAsset.IsNull())
        || !Root->GetRelativeTransform().Equals(FTransform::Identity, 1.e-6)
        || Root->GetAttachParent() != (Out.bGenerated ? Anchor : Actor.GetRootComponent())
        || Root->GetAttachSocketName() != (Out.bGenerated ? B.AttachmentSocket : NAME_None)) return false;
    Out.Root = Root; Out.Anchor = Anchor; Out.bAtAnchor = Out.bGenerated;
    for (auto* Parent = Anchor; Parent; Parent = Parent->GetAttachParent())
    {
        if (!ValidComponent(Actor, Parent) || Out.AnchorChain.Num() >= 256
            || Out.AnchorChain.Contains(TWeakObjectPtr<USceneComponent>(Parent))) return false;
        Out.AnchorChain.Add(Parent);
        if (Parent == Actor.GetRootComponent()) break;
    }
    if (Out.AnchorChain.IsEmpty() || Out.AnchorChain.Last().Get() != Actor.GetRootComponent()) return false;
    Claimed.Add(B.RootComponent);
    TArray<USceneComponent*> Children;
    Root->GetChildrenComponents(true, Children);
    if (Children.Num() != B.Meshes.Num()) return false;
    TSet<int32> SourceNodes;
    TMap<int32, FSoftObjectPath> SourceMaterials;
    for (const auto& Mesh : B.Meshes)
    {
        auto* Component = Cast<UStaticMeshComponent>(Resolve(Mesh.Component));
        if (!ValidComponent(Actor, Component) || Component->GetClass() != UStaticMeshComponent::StaticClass()
            || !Children.Contains(Component) || Component->GetAttachParent() != Root
            || !Component->GetAttachSocketName().IsNone() || Claimed.Contains(Mesh.Component)
            || ProtectedComponents.Contains(Mesh.Component) || Mesh.Mesh.IsNull() || !Mesh.LocalPose.IsSet()
            || Mesh.LocalPose->ContainsNaN() || !Mesh.LocalPose->GetRotation().IsNormalized()
            || Mesh.LocalPose->GetScale3D().GetMin() <= 0 || !Component->GetRelativeTransform().Equals(*Mesh.LocalPose, 1.e-6)
            || Component->GetVisibleFlag() || Component->bHiddenInGame || !Component->OverrideMaterials.IsEmpty()
            || Mesh.SourceNodeIndex < 0 || Mesh.SourceNodeIndex >= B.Meshes.Num() || SourceNodes.Contains(Mesh.SourceNodeIndex)
            || Mesh.Materials.Num() != 1) return false;
        const int32 ExpectedMaterial = Out.bGenerated ? 0 : Find(Out.ItemId)->MaterialIndices[Mesh.SourceNodeIndex];
        if (Mesh.Materials[0].SourceMaterialIndex != ExpectedMaterial) return false;
        TSet<FName> SlotsSeen;
        for (const auto& Material : Mesh.Materials)
        {
            if (Material.Slot.IsNone() || Material.Material.IsNull() || SlotsSeen.Contains(Material.Slot)) return false;
            const auto Path = Material.Material.ToSoftObjectPath();
            if (SourceMaterials.Contains(Material.SourceMaterialIndex) && SourceMaterials[Material.SourceMaterialIndex] != Path) return false;
            SourceMaterials.Add(Material.SourceMaterialIndex, Path);
            SlotsSeen.Add(Material.Slot);
        }
        if (Out.bGenerated ? !IsValid(Mesh.Mesh.Get()) || Component->GetStaticMesh() != Mesh.Mesh.Get()
            || !MaterialsMatch(*Component, Mesh) : Component->GetStaticMesh() != nullptr) return false;
        SourceNodes.Add(Mesh.SourceNodeIndex); Claimed.Add(Mesh.Component);
        FMeshState State;
        State.Binding = Mesh; State.Component = Component; State.bAssigned = Out.bGenerated;
        Out.Meshes.Add(MoveTemp(State));
    }
    return true;
}

bool BaddieCompanion::FNativeCharacterEquipment::Initialize(ABaddieCompanionCharacterActor& Actor,
    const FNativeCharacterEquipmentContent& Content, const TSet<FName>& ProtectedComponents,
    const TSet<FName>& FaceComponents, bool bWantsRoundFrames)
{
    using namespace CharacterEquipment;
    if (bInitialized || bCancelled || Content.Items.Num() > 6) return false;
    TSet<FName> Claimed;
    TArray<FGroupState> Next;
    TMap<FString, USceneComponent*> Parents;
    TMap<FString, FName> Sockets;
    for (const auto& Item : Content.Items)
    {
        const auto* Source = Find(Item.ItemId);
        if (!Source || !Item.SourceAssetKey.Equals(Source->Key, ESearchCase::CaseSensitive)
            || !Item.SourcePath.Equals(Source->Path, ESearchCase::CaseSensitive)
            || !Item.SourceSha256.Equals(Source->Sha, ESearchCase::CaseSensitive)
            || !Item.SourceParent.Equals(Source->Parent, ESearchCase::CaseSensitive)
            || Item.Group.Meshes.Num() != Source->MeshCount
            || Next.ContainsByPredicate([&](const auto& Value) { return Value.ItemId == Item.ItemId; })) return false;
        FGroupState Group;
        Group.ItemId = Item.ItemId; Group.Binding = Item.Group;
        Group.Target.SetTranslation(SourceToUnreal(Source->Position));
        if (!BindGroup(Actor, Group, ProtectedComponents, Claimed)) return false;
        if (Source->Slot == 0 && (Group.Anchor.Get() != Actor.GetRootComponent() || !Item.Group.AttachmentSocket.IsNone())) return false;
        if (Parents.Contains(Item.SourceParent) && (Parents[Item.SourceParent] != Group.Anchor.Get()
            || Sockets[Item.SourceParent] != Group.Binding.AttachmentSocket)) return false;
        Parents.Add(Item.SourceParent, Group.Anchor.Get()); Sockets.Add(Item.SourceParent, Group.Binding.AttachmentSocket);
        Next.Add(MoveTemp(Group));
    }
    TOptional<FGroupState> NextFrames;
    if (Content.GeneratedRoundFrames.IsSet())
    {
        const auto& Source = Content.GeneratedRoundFrames.GetValue();
        if (!Source.SourceRecipeSha256.Equals(TEXT("fc3dcb55e2e1d92cf9a0ea7c995c6b0b8a2a001366c6d59b51fa7ca2770e3eaa"), ESearchCase::CaseSensitive)
            || Source.Group.Meshes.Num() != 5) return false;
        FGroupState Group;
        Group.Binding = Source.Group; Group.bGenerated = true;
        if (!BindGroup(Actor, Group, ProtectedComponents, Claimed)
            || (Parents.Contains(TEXT("Head")) && (Parents[TEXT("Head")] != Group.Anchor.Get()
                || Sockets[TEXT("Head")] != Group.Binding.AttachmentSocket))) return false;
        NextFrames = MoveTemp(Group);
    }
    // No item may attach inside a controlled face or equipment subtree.
    const auto AnchorAllowed = [&](const FGroupState& Group)
    {
        for (const auto& Parent : Group.AnchorChain)
            if (!Parent.IsValid() || Claimed.Contains(Parent->GetFName()) || FaceComponents.Contains(Parent->GetFName())) return false;
        return true;
    };
    for (const auto& Group : Next) if (!AnchorAllowed(Group)) return false;
    if (NextFrames.IsSet() && !AnchorAllowed(*NextFrames)) return false;
    Groups = MoveTemp(Next); Frames = MoveTemp(NextFrames);
    bRoundFrames = bWantsRoundFrames; bInitialized = true;
    return true;
}

bool BaddieCompanion::FNativeCharacterEquipment::Matches(const ABaddieCompanionCharacterActor& Actor,
    const FGroupState& Group, bool bCheckMaterials) const
{
    using namespace CharacterEquipment;
    auto* Root = Group.Root.Get(); auto* Anchor = Group.Anchor.Get();
    if (!ValidComponent(Actor, Root) || !ValidComponent(Actor, Anchor)
        || Root->GetFName() != Group.Binding.RootComponent || Anchor->GetFName() != Group.Binding.AttachmentComponent
        || (!Group.Binding.AttachmentSocket.IsNone() && !Anchor->DoesSocketExist(Group.Binding.AttachmentSocket))
        || (Cast<UMeshComponent>(Anchor) && (!Group.Binding.AttachmentMeshAsset.IsValid()
            || AnchorAsset(Anchor) != Group.Binding.AttachmentMeshAsset.Get()))
        || Root->GetAttachParent() != (Group.bAtAnchor ? Anchor : Actor.GetRootComponent())
        || Root->GetAttachSocketName() != (Group.bAtAnchor ? Group.Binding.AttachmentSocket : NAME_None)
        || !Root->GetRelativeTransform().Equals(Group.Pose, 1.e-6)) return false;
    if (Group.AnchorChain.IsEmpty() || Group.AnchorChain[0].Get() != Anchor
        || Group.AnchorChain.Last().Get() != Actor.GetRootComponent()) return false;
    for (int32 I = 0; I < Group.AnchorChain.Num(); ++I)
    {
        auto* Component = Group.AnchorChain[I].Get();
        if (!ValidComponent(Actor, Component) || (I + 1 < Group.AnchorChain.Num()
            && Component->GetAttachParent() != Group.AnchorChain[I + 1].Get())) return false;
    }
    TArray<USceneComponent*> Children;
    Root->GetChildrenComponents(true, Children);
    if (Children.Num() != Group.Meshes.Num()) return false;
    for (const auto& Mesh : Group.Meshes)
    {
        auto* Component = Mesh.Component.Get();
        if (!ValidComponent(Actor, Component) || !Children.Contains(Component) || Component->GetAttachParent() != Root
            || Component->GetFName() != Mesh.Binding.Component || !Component->GetAttachSocketName().IsNone()
            || !Component->GetRelativeTransform().Equals(*Mesh.Binding.LocalPose, 1.e-6)
            || Component->bHiddenInGame || Component->GetVisibleFlag() != Mesh.bVisible || !Component->OverrideMaterials.IsEmpty()
            || (Mesh.bAssigned ? !Mesh.Binding.Mesh.IsValid() || Component->GetStaticMesh() != Mesh.Binding.Mesh.Get()
                || (bCheckMaterials && !MaterialsMatch(*Component, Mesh.Binding)) : Component->GetStaticMesh() != nullptr)) return false;
    }
    return true;
}

bool BaddieCompanion::FNativeCharacterEquipment::Install(ABaddieCompanionCharacterActor& Actor,
    FGroupState& Group, TFunctionRef<bool()> IsCurrent)
{
    if (!IsCurrent() || !Matches(Actor, Group) || Group.bAtAnchor) return false;
    for (auto& Mesh : Group.Meshes)
    {
        if (!IsValid(Mesh.Binding.Mesh.Get())) return false;
        // Storage survives callbacks. Clear can cancel now but cannot dispose components here.
        Mesh.bAssigned = true;
        Mesh.Component->SetStaticMesh(Mesh.Binding.Mesh.Get());
        if (!IsCurrent()) return false;
        if (Mesh.Component.IsValid() && Mesh.Component->GetStaticMesh() == nullptr) Mesh.bAssigned = false;
        if (!Mesh.bAssigned || !Matches(Actor, Group)) return false;
    }
    if (!Group.Root->AttachToComponent(Group.Anchor.Get(), FAttachmentTransformRules::KeepRelativeTransform,
        Group.Binding.AttachmentSocket)) return false;
    Group.bAtAnchor = true;
    if (!IsCurrent() || !Matches(Actor, Group)) return false;
    Group.Pose = Group.Target;
    Group.Root->SetRelativeTransform(Group.Target, false, nullptr, ETeleportType::TeleportPhysics);
    if (!IsCurrent() || !Matches(Actor, Group)) return false;
    for (auto& Mesh : Group.Meshes)
    {
        Mesh.bVisible = true;
        Mesh.Component->SetVisibility(true, false);
        if (!IsCurrent() || !Matches(Actor, Group)) return false;
    }
    return true;
}

bool BaddieCompanion::FNativeCharacterEquipment::Retire(ABaddieCompanionCharacterActor& Actor,
    FGroupState& Group, TFunctionRef<bool()> IsCurrent)
{
    // Rollback accepts failed material validation, but never clears a replacement mesh.
    if (!IsCurrent() || !Matches(Actor, Group, false)) return false;
    for (auto& Mesh : Group.Meshes)
    {
        Mesh.bVisible = false;
        Mesh.Component->SetVisibility(false, false);
        if (!IsCurrent() || !Matches(Actor, Group, false)) return false;
        if (!Group.bGenerated)
        {
            Mesh.bAssigned = false;
            Mesh.Component->SetStaticMesh(nullptr);
            if (!IsCurrent() || !Matches(Actor, Group, false)) return false;
        }
    }
    if (Group.bGenerated) return Matches(Actor, Group);
    if (!Group.Root->AttachToComponent(Actor.GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform)) return false;
    Group.bAtAnchor = false;
    if (!IsCurrent() || !Matches(Actor, Group)) return false;
    Group.Pose = FTransform::Identity;
    Group.Root->SetRelativeTransform(FTransform::Identity, false, nullptr, ETeleportType::TeleportPhysics);
    return IsCurrent() && Matches(Actor, Group);
}

BaddieCompanion::ECharacterEquipmentIssue BaddieCompanion::FNativeCharacterEquipment::Observe(
    ABaddieCompanionCharacterActor& Actor, const FCharacterEquipmentObservation& Observation,
    bool bRetryFailed, TFunctionRef<bool()> IsCurrent)
{
    using namespace CharacterEquipment;
    const TOptional<FString> Values[] = { Observation.Toy, Observation.Eyewear, Observation.Neckwear, Observation.Pin };
    if (!bInitialized || bCancelled || !IsCurrent()) return ECharacterEquipmentIssue::OwnerUnavailable;
    for (int32 I = 0; I < 4; ++I)
    {
        if (!Values[I].IsSet()) return ECharacterEquipmentIssue::ObservationUnknown;
        const auto* Item = Find(*Values[I]);
        if (!Values[I]->IsEmpty() && (!Item || Item->Slot != I)) return ECharacterEquipmentIssue::UnsupportedItem;
    }
    bObserved = true;
    for (int32 I = 0; I < 4; ++I)
    {
        auto& Slot = Slots[I];
        if (Slot.Requested.IsSet() && Slot.Requested->Equals(*Values[I], ESearchCase::CaseSensitive)
            && (!bRetryFailed || Slot.Issue == ECharacterEquipmentIssue::None)) continue;
        if (Slot.Load.IsValid()) { Slot.Load->CancelHandle(); Slot.Load.Reset(); }
        Slot.Pending = INDEX_NONE; Slot.Requested = Values[I];
        Slot.Issue = ECharacterEquipmentIssue::None;
        if (Values[I]->IsEmpty())
        {
            if (Slot.Attached != INDEX_NONE && !Retire(Actor, Groups[Slot.Attached], IsCurrent))
                return ECharacterEquipmentIssue::OwnerUnavailable;
            Slot.Attached = INDEX_NONE;
            continue;
        }
        if (Slot.Attached != INDEX_NONE && Groups[Slot.Attached].ItemId.Equals(*Values[I], ESearchCase::CaseSensitive)) continue;
        const int32 Index = Groups.IndexOfByPredicate([&](const auto& Group) { return Group.ItemId.Equals(*Values[I], ESearchCase::CaseSensitive); });
        if (Index == INDEX_NONE) { Slot.Issue = ECharacterEquipmentIssue::UnsupportedBinding; continue; }
        if (!UAssetManager::IsInitialized()) { Slot.Issue = ECharacterEquipmentIssue::AssetUnavailable; continue; }
        TArray<FSoftObjectPath> Paths;
        for (const auto& Mesh : Groups[Index].Meshes)
        {
            Paths.AddUnique(Mesh.Binding.Mesh.ToSoftObjectPath());
            for (const auto& Material : Mesh.Binding.Materials) Paths.AddUnique(Material.Material.ToSoftObjectPath());
        }
        Slot.Pending = Index;
        Slot.Load = UAssetManager::GetStreamableManager().RequestAsyncLoad(Paths);
        if (!Slot.Load.IsValid()) { Slot.Pending = INDEX_NONE; Slot.Issue = ECharacterEquipmentIssue::AssetUnavailable; }
    }
    return Poll(Actor, IsCurrent) ? ECharacterEquipmentIssue::None : ECharacterEquipmentIssue::OwnerUnavailable;
}

bool BaddieCompanion::FNativeCharacterEquipment::Poll(ABaddieCompanionCharacterActor& Actor, TFunctionRef<bool()> IsCurrent)
{
    if (!bInitialized || bCancelled || !IsCurrent()) return false;
    for (auto& Slot : Slots)
    {
        if (Slot.Attached != INDEX_NONE && !Matches(Actor, Groups[Slot.Attached])) return false;
        if (!Slot.Load.IsValid()) continue;
        if (Slot.Load->WasCanceled() || !Slot.Load->IsActive())
        {
            Slot.Load.Reset(); Slot.Pending = INDEX_NONE; Slot.Issue = ECharacterEquipmentIssue::AssetUnavailable;
            continue;
        }
        if (!Slot.Load->HasLoadCompleted()) continue;
        if (!Groups.IsValidIndex(Slot.Pending)) return false;
        auto& Candidate = Groups[Slot.Pending];
        bool bAssetsAvailable = true;
        for (const auto& Mesh : Candidate.Meshes)
        {
            bAssetsAvailable &= IsValid(Mesh.Binding.Mesh.Get());
            for (const auto& Material : Mesh.Binding.Materials) bAssetsAvailable &= IsValid(Material.Material.Get());
        }
        const bool bInstalled = bAssetsAvailable && Install(Actor, Candidate, IsCurrent);
        if (!IsCurrent()) return false;
        if (!bInstalled)
        {
            if (!Retire(Actor, Candidate, IsCurrent)) return false;
            Slot.Issue = bAssetsAvailable ? ECharacterEquipmentIssue::AttachmentUnavailable : ECharacterEquipmentIssue::AssetUnavailable;
        }
        else
        {
            const int32 Previous = Slot.Attached;
            Slot.Attached = Slot.Pending; Slot.Issue = ECharacterEquipmentIssue::None;
            if (Previous != INDEX_NONE && !Retire(Actor, Groups[Previous], IsCurrent)) return false;
        }
        Slot.Load->ReleaseHandle(); Slot.Load.Reset(); Slot.Pending = INDEX_NONE;
    }
    for (const auto& Slot : Slots)
        if (Slot.Attached != INDEX_NONE && !Matches(Actor, Groups[Slot.Attached])) return false;
    return SyncFrames(Actor, IsCurrent);
}

bool BaddieCompanion::FNativeCharacterEquipment::SyncFrames(ABaddieCompanionCharacterActor& Actor,
    TFunctionRef<bool()> IsCurrent)
{
    if (!Frames.IsSet())
    { FramesIssue = ECharacterEquipmentIssue::GeneratedEyewearUnavailable; return true; }
    if (!IsCurrent() || !Matches(Actor, *Frames)) return false;
    // The committed attachment is derived from native topology/mesh identity, never requested ID.
    const auto& Eyewear = Slots[1];
    const bool bAttached = Eyewear.Attached != INDEX_NONE
        && Groups[Eyewear.Attached].ItemId.Equals(TEXT("round-glasses"), ESearchCase::CaseSensitive)
        && Groups[Eyewear.Attached].bAtAnchor && Matches(Actor, Groups[Eyewear.Attached]);
    const bool bVisible = bObserved && bRoundFrames && !bAttached;
    const auto AttachmentsMatch = [&]()
    {
        for (const auto& Slot : Slots)
            if (Slot.Attached != INDEX_NONE && !Matches(Actor, Groups[Slot.Attached])) return false;
        return true;
    };
    for (auto& Mesh : Frames->Meshes)
    {
        Mesh.bVisible = bVisible;
        Mesh.Component->SetVisibility(bVisible, false);
        if (!IsCurrent() || !Matches(Actor, *Frames) || !AttachmentsMatch()) return false;
    }
    FramesIssue = bObserved ? ECharacterEquipmentIssue::None : ECharacterEquipmentIssue::ObservationUnknown;
    return true;
}

void BaddieCompanion::FNativeCharacterEquipment::Cancel()
{
    bCancelled = true;
    for (auto& Slot : Slots)
        if (Slot.Load.IsValid()) Slot.Load->CancelHandle();
}

void BaddieCompanion::FNativeCharacterEquipment::Clear(ABaddieCompanionCharacterActor& Actor)
{
    Cancel();
    // The actor defers this until native mutation stacks unwind. Its own disposal follows.
    const auto CleanupCurrent = [&]() { return true; };
    for (auto& Group : Groups) Retire(Actor, Group, CleanupCurrent);
    if (Frames.IsSet()) Retire(Actor, *Frames, CleanupCurrent);
    Groups.Reset(); Frames.Reset();
    for (auto& Slot : Slots) Slot = {};
    bObserved = false; bInitialized = false; bRoundFrames = false;
    FramesIssue = ECharacterEquipmentIssue::ObservationUnknown;
}

bool BaddieCompanion::FNativeCharacterEquipment::InspectCurrent(const ABaddieCompanionCharacterActor& Actor,
    const FNativeCharacterEquipmentContent& Content, bool bWantsRoundFrames) const
{
    // The caller reserves the complete native scan, temporary topology/material
    // arrays and their destruction. No target getter/allocation cost is qualified.
    constexpr uint64 QualifiedEquipmentInspectionWork = 0;
    if (!QualifiedEquipmentInspectionWork) return false;
    using namespace CharacterEquipment;
    if (!bInitialized || bCancelled || !bObserved || !IsValid(&Actor)
        || Actor.IsActorBeingDestroyed() || Groups.Num() > 6 || Content.Items.Num() != Groups.Num()
        || bWantsRoundFrames != bRoundFrames || Content.GeneratedRoundFrames.IsSet() != Frames.IsSet()) return false;
    const auto SameGroup = [](const FNativeEquipmentGroup& Supplied, const FGroupState& Prepared)
    {
        const auto& Saved = Prepared.Binding;
        if (Supplied.RootComponent != Saved.RootComponent || Supplied.AttachmentComponent != Saved.AttachmentComponent
            || Supplied.AttachmentSocket != Saved.AttachmentSocket || Supplied.AttachmentMeshAsset != Saved.AttachmentMeshAsset
            || Supplied.Meshes.Num() != Saved.Meshes.Num()) return false;
        for (int32 I = 0; I < Supplied.Meshes.Num(); ++I)
        {
            const auto& A = Supplied.Meshes[I]; const auto& B = Saved.Meshes[I];
            if (A.SourceNodeIndex != B.SourceNodeIndex || A.Component != B.Component || A.Mesh != B.Mesh
                || !A.LocalPose.IsSet() || !B.LocalPose.IsSet() || !A.LocalPose->Equals(*B.LocalPose, 1.e-6)
                || A.Materials.Num() != B.Materials.Num()) return false;
            for (int32 J = 0; J < A.Materials.Num(); ++J)
                if (A.Materials[J].SourceMaterialIndex != B.Materials[J].SourceMaterialIndex
                    || A.Materials[J].Slot != B.Materials[J].Slot || A.Materials[J].Material != B.Materials[J].Material) return false;
        }
        return true;
    };
    TSet<FString> SuppliedItems;
    for (const auto& Item : Content.Items)
    {
        const auto* Source = Find(Item.ItemId);
        const auto* Prepared = Groups.FindByPredicate([&](const auto& Group)
            { return Group.ItemId.Equals(Item.ItemId, ESearchCase::CaseSensitive); });
        if (!Source || !Prepared || SuppliedItems.Contains(Item.ItemId)
            || !Item.SourceAssetKey.Equals(Source->Key, ESearchCase::CaseSensitive)
            || !Item.SourcePath.Equals(Source->Path, ESearchCase::CaseSensitive)
            || !Item.SourceSha256.Equals(Source->Sha, ESearchCase::CaseSensitive)
            || !Item.SourceParent.Equals(Source->Parent, ESearchCase::CaseSensitive)
            || !SameGroup(Item.Group, *Prepared)) return false;
        SuppliedItems.Add(Item.ItemId);
    }
    if (Frames.IsSet() && (!Content.GeneratedRoundFrames->SourceRecipeSha256.Equals(
            TEXT("fc3dcb55e2e1d92cf9a0ea7c995c6b0b8a2a001366c6d59b51fa7ca2770e3eaa"), ESearchCase::CaseSensitive)
        || !SameGroup(Content.GeneratedRoundFrames->Group, *Frames))) return false;
    for (int32 I = 0; I < 4; ++I)
    {
        const auto& Slot = Slots[I];
        if (!Slot.Requested.IsSet() || Slot.Issue != ECharacterEquipmentIssue::None
            || Slot.Load.IsValid() || Slot.Pending != INDEX_NONE
            || (Slot.Attached != INDEX_NONE && !Groups.IsValidIndex(Slot.Attached))) return false;
        if (Slot.Requested->IsEmpty())
        { if (Slot.Attached != INDEX_NONE) return false; }
        else
        {
            const auto* Item = Find(*Slot.Requested);
            if (!Item || Item->Slot != I || Slot.Attached == INDEX_NONE
                || !Groups[Slot.Attached].ItemId.Equals(*Slot.Requested, ESearchCase::CaseSensitive)
                || !Groups[Slot.Attached].bAtAnchor) return false;
        }
    }
    for (int32 I = 0; I < Groups.Num(); ++I)
    {
        const auto& Group = Groups[I];
        const auto* Item = Find(Group.ItemId);
        if (!Item || Group.bGenerated || !Matches(Actor, Group)) return false;
        const bool bAttached = Slots[Item->Slot].Attached == I;
        if (Group.bAtAnchor != bAttached
            || !Group.Pose.Equals(bAttached ? Group.Target : FTransform::Identity, 1.e-6)) return false;
        for (const auto& Mesh : Group.Meshes)
            if (Mesh.bAssigned != bAttached || Mesh.bVisible != bAttached) return false;
    }
    const bool bAttachedRound = Slots[1].Attached != INDEX_NONE
        && Groups[Slots[1].Attached].ItemId.Equals(TEXT("round-glasses"), ESearchCase::CaseSensitive);
    const bool bExpectedFrames = bRoundFrames && !bAttachedRound;
    if (!Frames.IsSet())
        return !bExpectedFrames && FramesIssue == ECharacterEquipmentIssue::GeneratedEyewearUnavailable;
    if (FramesIssue != ECharacterEquipmentIssue::None || !Frames->bGenerated || !Frames->bAtAnchor
        || !Matches(Actor, *Frames)) return false;
    for (const auto& Mesh : Frames->Meshes)
        if (!Mesh.bAssigned || Mesh.bVisible != bExpectedFrames) return false;
    return bInitialized && !bCancelled && bObserved && !Actor.IsActorBeingDestroyed();
}

BaddieCompanion::FCharacterEquipmentStatus BaddieCompanion::FNativeCharacterEquipment::Status() const
{
    FCharacterEquipmentStatus Result;
    Result.bHasAcceptedObservation = bObserved; Result.GeneratedFrames = FramesIssue;
    for (const auto& Slot : Slots)
    {
        auto& Value = Result.Slots.AddDefaulted_GetRef();
        Value.RequestedId = Slot.Requested.Get(FString());
        if (Groups.IsValidIndex(Slot.Attached)) Value.AttachedId = Groups[Slot.Attached].ItemId;
        Value.bLoading = Slot.Pending != INDEX_NONE && Slot.Load.IsValid() && Slot.Load->IsActive();
        Value.Issue = Slot.Issue;
    }
    return Result;
}
