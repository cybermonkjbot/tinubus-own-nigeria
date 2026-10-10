#pragma once

#include "BaddieCompanionHomePresentation.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include <initializer_list>

namespace BaddieCompanion::HomeMapping
{
    using FObject = TSharedPtr<FJsonObject>;
    inline bool Same(const FString& A, const FString& B) { return A.Equals(B, ESearchCase::CaseSensitive); }
    inline bool SameRoomSelection(const FRoomSelection& A, const FRoomSelection& B)
    {
        return Same(A.Id, B.Id) && Same(A.Name, B.Name) && Same(A.EnvironmentId, B.EnvironmentId)
            && Same(A.LayoutId, B.LayoutId) && Same(A.RoomJson, B.RoomJson);
    }
    inline void Issue(TArray<FHomeMappingIssue>& Issues, EHomeIssue Code, const FString& Field)
    {
        Issues.Add({ Code, Field });
    }
    bool ReadObject(const FString& Json, FObject& Out);
    bool Text(const FObject& Object, const TCHAR* Field, int32 Max, FString& Out);
    bool Number(const FObject& Object, const TCHAR* Field, double& Out);
    bool OptionalNumber(const FObject& Object, const TCHAR* Field, TOptional<double>& Out);
    bool OptionalBool(const FObject& Object, const TCHAR* Field, TOptional<bool>& Out);
    bool OptionalColor(const FObject& Object, const TCHAR* Field, TOptional<FString>& Out);
    void UnknownFields(const FObject& Object, std::initializer_list<const TCHAR*> Known,
        const FString& Path, TArray<FHomeMappingIssue>& Issues);

    template<typename T> struct TChoiceEntry { const TCHAR* Name; T Value; };
    // Null/numeric fields are malformed, unknown strings are retained without a guessed default.
    template<typename T> bool Choice(const FObject& Object, const TCHAR* Field, TSourceChoice<T>& Out,
        std::initializer_list<TChoiceEntry<T>> Entries, bool bRequired, const FString& Path,
        TArray<FHomeMappingIssue>& Issues, TOptional<T> Default = {})
    {
        Out = {};
        if (!Object->HasField(Field))
        {
            if (bRequired) return false;
            Out.Effective = Default;
            return true;
        }
        FString Value;
        if (!Text(Object, Field, 80, Value)) return false;
        Out.Saved = Value;
        for (const auto& Entry : Entries)
            if (Same(Value, Entry.Name)) { Out.Effective = Entry.Value; return true; }
        Issue(Issues, EHomeIssue::UnknownSelection, Path + TEXT(".") + Field);
        return true;
    }

    FCanonicalAssetReference CharacterReference();
    TOptional<FCanonicalAssetReference> RoomReference(ELayout Layout);
    FCanonicalAssetReference TerraceReference();
    FSourcePose Pose(const FVector& Point, double Yaw = 0);
    FSourceBox Box(const FVector& BottomCentre, const FVector& Dimensions);
    TArray<EItemKind> SupportKinds(ESurface Surface);
    FRoomShellPlan BedroomShell(const TOptional<FHomeDesignPlan>& Design);
    TOptional<FRoomShellPlan> RoomShell(const FRoomPlan& Room);
    bool ReadAppearance(const FCharacterSelection& Input, FAppearancePlan& Out,
        TArray<FCharacterMaterialRequirement>& Materials, TArray<FAttachmentRequirement>& Attachments,
        TArray<FHomeMappingIssue>& Issues);
    bool ReadRoom(const FRoomSelection& Input, FRoomPlan& Out, const FString& Path,
        TArray<FHomeMappingIssue>& Issues);
    void MapFurniture(FRoomPlan& Room, const FString& Path, TArray<FHomeMappingIssue>& Issues);
}
