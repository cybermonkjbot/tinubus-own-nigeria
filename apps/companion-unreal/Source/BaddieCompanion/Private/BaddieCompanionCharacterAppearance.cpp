#include "BaddieCompanionCharacterAppearance.h"

#include "BaddieCompanionCharacterActor.h"
#include "Components/MeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkinnedAsset.h"
#include "Engine/StaticMesh.h"

namespace BaddieCompanion::CharacterFace
{
    constexpr int32 NodeCount = 8;
    constexpr int32 MaxComponents = 256;
    constexpr double PoseTolerance = 1.e-6;

    int32 SourceIndex(const FString& Name)
    {
        const TCHAR* Names[] = { TEXT("EyeL"), TEXT("EyeR"), TEXT("Groomed eyebrow L"),
            TEXT("Groomed eyebrow R"), TEXT("Nose"), TEXT("Smile"), TEXT("Mouth opening"),
            TEXT("Lower mouth warmth") };
        for (int32 I = 0; I < NodeCount; ++I)
            if (Name.Equals(Names[I], ESearchCase::CaseSensitive)) return I;
        return INDEX_NONE;
    }

    bool ValidPose(const FTransform& Pose)
    {
        const auto Scale = Pose.GetScale3D();
        return !Pose.ContainsNaN() && Pose.GetRotation().IsNormalized()
            && Scale.X > 0 && Scale.Y > 0 && Scale.Z > 0;
    }

    bool TargetPose(const FAppearancePlan& Appearance, int32 Index, const FTransform& Authored, FTransform& Out)
    {
        if (!Appearance.Eyes.Effective.IsSet() || !Appearance.Brows.Effective.IsSet()
            || !Appearance.Nose.Effective.IsSet() || !Appearance.Mouth.Effective.IsSet()) return false;
        double Width = 1, Height = 1, RotationZ = 0, PositionY = 0;
        if (Index == 0 || Index == 1)
        {
            switch (Appearance.Eyes.Effective.GetValue())
            {
            case EEyeShape::Classic: break;
            case EEyeShape::Round: Width = .97; Height = 1.16; break;
            case EEyeShape::Almond: Width = 1.08; Height = .83; RotationZ = Index == 0 ? -.08 : .08; break;
            case EEyeShape::Sleepy: Height = .72; break;
            default: return false;
            }
        }
        else if (Index == 2 || Index == 3)
        {
            switch (Appearance.Brows.Effective.GetValue())
            {
            case EBrowStyle::Classic: break;
            case EBrowStyle::Soft: Height = .7; break;
            case EBrowStyle::Bold: Height = 1.7; break;
            case EBrowStyle::Curious: if (Index == 3) { PositionY = .025; RotationZ = .15; } break;
            default: return false;
            }
        }
        else if (Index == 4)
        {
            switch (Appearance.Nose.Effective.GetValue())
            {
            case ENoseShape::Classic: break;
            case ENoseShape::Button: Width = .78; Height = 1.3; break;
            case ENoseShape::Wide: Width = 1.4; break;
            default: return false;
            }
        }
        else if (Index >= 5 && Index < NodeCount)
        {
            switch (Appearance.Mouth.Effective.GetValue())
            {
            case EMouthShape::Classic: break;
            case EMouthShape::Small: Width = .72; break;
            case EMouthShape::Wide: Width = 1.35; break;
            default: return false;
            }
        }
        else return false;
        Out = Authored;
        Out.SetScale3D(Authored.GetScale3D() * FVector(1, Width, Height));
        Out.SetTranslation(Authored.GetTranslation() + SourceToUnreal(FVector(0, PositionY, 0)));
        // faceRig adds to Euler Z after restoring the authored XYZ quaternion. That is
        // qAuthored * qZ(delta), not a UE yaw or a world-space quaternion premultiply.
        Out.SetRotation((Authored.GetRotation() * FQuat(FVector::ForwardVector, RotationZ)).GetNormalized());
        return ValidPose(Out);
    }

    UObject* MeshAsset(USceneComponent* Component)
    {
        if (auto* Static = Cast<UStaticMeshComponent>(Component)) return Static->GetStaticMesh();
        if (auto* Skinned = Cast<USkinnedMeshComponent>(Component)) return Skinned->GetSkinnedAsset();
        return nullptr;
    }

    bool ValidComponent(const ABaddieCompanionCharacterActor& Actor, const USceneComponent* Component)
    {
        return IsValid(Component) && Component->GetOwner() == &Actor && Component->GetWorld() == Actor.GetWorld()
            && Component->IsRegistered() && Component->GetMobility() == EComponentMobility::Movable
            && Component->GetAttachSocketName().IsNone() && !Component->IsUsingAbsoluteLocation()
            && !Component->IsUsingAbsoluteRotation() && !Component->IsUsingAbsoluteScale();
    }
}

bool BaddieCompanion::FNativeCharacterFaceAppearance::Accepts(const FAppearancePlan& Appearance,
    const TArray<FNativeCharacterFaceNode>& Bindings)
{
    using namespace CharacterFace;
    if (!Appearance.bCompleteKnownSelections || Bindings.Num() != NodeCount) return false;
    TSet<int32> Sources;
    TSet<FName> Roots, Meshes;
    for (const auto& Binding : Bindings)
    {
        const int32 Index = SourceIndex(Binding.SourceNode);
        FTransform Target;
        if (Index == INDEX_NONE || Sources.Contains(Index) || Binding.Component.IsNone()
            || Roots.Contains(Binding.Component) || Binding.ParentComponent.IsNone()
            || Binding.ParentComponent == Binding.Component || !Binding.AuthoredLocalPose.IsSet()
            || !ValidPose(Binding.AuthoredLocalPose.GetValue()) || Binding.Meshes.IsEmpty()
            || Binding.Meshes.Num() > MaxComponents
            || !TargetPose(Appearance, Index, Binding.AuthoredLocalPose.GetValue(), Target)) return false;
        Sources.Add(Index); Roots.Add(Binding.Component);
        for (const auto& Mesh : Binding.Meshes)
        {
            if (Mesh.MeshComponent.IsNone() || Mesh.MeshAsset.IsNull() || Meshes.Contains(Mesh.MeshComponent)) return false;
            Meshes.Add(Mesh.MeshComponent);
        }
    }
    return Meshes.Num() <= MaxComponents;
}

bool BaddieCompanion::FNativeCharacterFaceAppearance::MatchesNode(
    const ABaddieCompanionCharacterActor& Actor, const FNodeState& Node)
{
    using namespace CharacterFace;
    if (Node.Components.IsEmpty()) return false;
    const auto* Root = Node.Components[0].Component.Get();
    if (!ValidComponent(Actor, Root)) return false;
    TArray<USceneComponent*> Current;
    Root->GetChildrenComponents(true, Current);
    Current.Insert(Node.Components[0].Component.Get(), 0);
    if (Current.Num() != Node.Components.Num()) return false;
    for (int32 I = 0; I < Node.Components.Num(); ++I)
    {
        const auto& Saved = Node.Components[I];
        auto* Component = Saved.Component.Get();
        const auto& Expected = I == 0 && Node.bApplied ? Node.Target : Saved.LocalPose;
        if (!ValidComponent(Actor, Component) || !Current.Contains(Component)
            || Component->GetFName() != Saved.Name || !Saved.Parent.IsValid()
            || !ValidComponent(Actor, Saved.Parent.Get()) || Component->GetAttachParent() != Saved.Parent.Get()
            || !Component->GetRelativeTransform().Equals(Expected, PoseTolerance)
            || (Saved.bMesh && (!Saved.MeshAsset.IsValid() || MeshAsset(Component) != Saved.MeshAsset.Get()))) return false;
    }
    return true;
}

bool BaddieCompanion::FNativeCharacterFaceAppearance::Matches(const ABaddieCompanionCharacterActor& Actor) const
{
    if (Nodes.Num() != CharacterFace::NodeCount) return false;
    for (const auto& Node : Nodes)
        if (!MatchesNode(Actor, Node)) return false;
    return true;
}

bool BaddieCompanion::FNativeCharacterFaceAppearance::Apply(ABaddieCompanionCharacterActor& Actor,
    const FAppearancePlan& Appearance, const TArray<FNativeCharacterFaceNode>& Bindings,
    TFunctionRef<bool()> IsCurrent)
{
    using namespace CharacterFace;
    check(IsInGameThread());
    if (!Nodes.IsEmpty() || !IsCurrent() || !Accepts(Appearance, Bindings)) return false;
    TInlineComponentArray<USceneComponent*> AllComponents(&Actor);
    const auto FindComponent = [&](FName Name) -> USceneComponent*
    {
        USceneComponent* Found = nullptr;
        for (auto* Component : AllComponents)
            if (IsValid(Component) && Component->GetFName() == Name)
            {
                if (Found) return nullptr;
                Found = Component;
            }
        return Found;
    };
    TSet<USceneComponent*> Claimed;
    TSet<FName> ClaimedNames;
    TArray<FNodeState> Staged;
    for (const auto& Binding : Bindings)
    {
        auto* Root = FindComponent(Binding.Component);
        if (!ValidComponent(Actor, Root) || Root == Actor.GetRootComponent()
            || !ValidComponent(Actor, Root->GetAttachParent())
            || Root->GetAttachParent() != FindComponent(Binding.ParentComponent)
            || !Root->GetRelativeTransform().Equals(Binding.AuthoredLocalPose.GetValue(), PoseTolerance)) return false;
        TArray<USceneComponent*> Subtree;
        Root->GetChildrenComponents(true, Subtree);
        Subtree.Insert(Root, 0);
        if (Subtree.Num() > MaxComponents) return false;
        auto& Node = Staged.AddDefaulted_GetRef();
        if (!TargetPose(Appearance, SourceIndex(Binding.SourceNode), Root->GetRelativeTransform(), Node.Target)) return false;
        int32 MeshCount = 0;
        for (auto* Component : Subtree)
        {
            if (!ValidComponent(Actor, Component) || Claimed.Contains(Component)
                || ClaimedNames.Contains(Component->GetFName())
                || !ValidComponent(Actor, Component->GetAttachParent())
                || !ValidPose(Component->GetRelativeTransform())) return false;
            Claimed.Add(Component);
            ClaimedNames.Add(Component->GetFName());
            if (Claimed.Num() > MaxComponents) return false;
            auto& Saved = Node.Components.AddDefaulted_GetRef();
            Saved.Component = Component; Saved.Parent = Component->GetAttachParent(); Saved.Name = Component->GetFName();
            Saved.LocalPose = Component->GetRelativeTransform(); Saved.bMesh = Component->IsA<UMeshComponent>();
            if (Saved.bMesh)
            {
                const auto* Mesh = Binding.Meshes.FindByPredicate([&](const auto& Value)
                    { return Value.MeshComponent == Saved.Name; });
                auto* Asset = MeshAsset(Component);
                if (!Mesh || !IsValid(Asset) || Mesh->MeshAsset.Get() != Asset) return false;
                Saved.MeshAsset = Asset;
                ++MeshCount;
            }
        }
        if (MeshCount != Binding.Meshes.Num()) return false;
    }
    // Retain baselines before any callback can cancel preparation. Actor cleanup restores them
    // after the setter stack unwinds, including a partially completed multi-node application.
    Nodes = MoveTemp(Staged);
    for (auto& Node : Nodes)
    {
        if (!IsCurrent() || !Matches(Actor)) return false;
        Node.bApplied = true;
        Node.Components[0].Component->SetRelativeTransform(Node.Target, false, nullptr, ETeleportType::TeleportPhysics);
        if (!IsCurrent() || !Matches(Actor)) return false;
    }
    return true;
}

void BaddieCompanion::FNativeCharacterFaceAppearance::Clear(ABaddieCompanionCharacterActor& Actor)
{
    check(IsInGameThread());
    for (auto& Node : Nodes)
    {
        // Do not overwrite a replacement mesh, attachment, pose or another owner's changes.
        if (Node.bApplied && MatchesNode(Actor, Node))
        {
            Node.bApplied = false;
            Node.Components[0].Component->SetRelativeTransform(Node.Components[0].LocalPose,
                false, nullptr, ETeleportType::TeleportPhysics);
        }
    }
    Nodes.Reset();
}
