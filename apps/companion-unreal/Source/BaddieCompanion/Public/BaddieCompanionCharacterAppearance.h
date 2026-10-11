#pragma once

#include "CoreMinimal.h"
#include "BaddieCompanionHomePresentation.h"

class ABaddieCompanionCharacterActor;
class USceneComponent;

namespace BaddieCompanion
{
    struct FNativeCharacterFaceMesh
    {
        FName MeshComponent;
        TSoftObjectPtr<UObject> MeshAsset;
    };
    struct FNativeCharacterFaceNode
    {
        // Exact unsanitized faceRig source name, not a bone, morph or native component name.
        FString SourceNode;
        FName Component, ParentComponent;
        // Required accepted pose after import, before appearance. Local axes and parent units
        // must retain source [x,y,z] -> UE [z,x,y], metres -> cm, with no baked face selection.
        // Preserve the source XYZ Euler convention. Socket/bone-driven roots are unsupported.
        TOptional<FTransform> AuthoredLocalPose;
        // Every drawable occurrence below this root, including the root if it is a mesh.
        // Content acceptance must establish source membership and skin/animation compatibility.
        TArray<FNativeCharacterFaceMesh> Meshes;
    };

    // Private one-shot actor state. Weak references borrow components, never own imported assets.
    // The actor's presentation guard must span Apply/Clear and defer destructive callbacks.
    class FNativeCharacterFaceAppearance
    {
        friend class ::ABaddieCompanionCharacterActor;
        static bool Accepts(const FAppearancePlan& Appearance, const TArray<FNativeCharacterFaceNode>& Bindings);
        bool Apply(ABaddieCompanionCharacterActor& Actor, const FAppearancePlan& Appearance,
            const TArray<FNativeCharacterFaceNode>& Bindings, TFunctionRef<bool()> IsCurrent);
        bool Matches(const ABaddieCompanionCharacterActor& Actor) const;
        bool InspectCurrent(const ABaddieCompanionCharacterActor& Actor, const FAppearancePlan& Appearance,
            const TArray<FNativeCharacterFaceNode>& Bindings) const;
        void AppendControlledComponentNames(TSet<FName>& Out) const;
        void Clear(ABaddieCompanionCharacterActor& Actor);

        struct FComponentState
        {
            TWeakObjectPtr<USceneComponent> Component, Parent;
            TWeakObjectPtr<UObject> MeshAsset;
            FName Name;
            FTransform LocalPose;
            bool bMesh = false;
        };
        struct FNodeState
        {
            // Root first, followed by every attachment descendant.
            TArray<FComponentState> Components;
            int32 Source = INDEX_NONE;
            FTransform Target;
            bool bApplied = false;
        };
        static bool MatchesNode(const ABaddieCompanionCharacterActor& Actor, const FNodeState& Node);
        TArray<FNodeState> Nodes;
        EEyeShape PreparedEyes = EEyeShape::Classic;
        EBrowStyle PreparedBrows = EBrowStyle::Classic;
        ENoseShape PreparedNose = ENoseShape::Classic;
        EMouthShape PreparedMouth = EMouthShape::Classic;
    };
}
