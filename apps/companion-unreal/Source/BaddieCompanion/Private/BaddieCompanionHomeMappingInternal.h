#pragma once

#include "BaddieCompanionHomePresentation.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include <initializer_list>
#include "BaddieCompanionWorldPlayerRead.h"

namespace BaddieCompanion::HomeMapping
{
    using EMappingStep = WorldObservationPrivate::EStep;
    // Each operation requests work and retained capacity before touching output.
    // The legacy adapter has its existing synchronous, bounded input contract.
    class FMappingWork
    {
    public:
        virtual ~FMappingWork() = default;
        virtual EMappingStep Admit(uint64 Work, uint64 Capacity = 0) = 0;
        virtual EMappingStep ReleaseText(FString& Text)
        {
            const auto Result = Admit(128 + static_cast<uint64>(Text.Len()));
            if (Result != EMappingStep::Complete) return Result;
            Text.Empty(); return EMappingStep::Complete;
        }
    };
    class FLegacyMappingWork final : public FMappingWork
    {
    public:
        EMappingStep Admit(uint64, uint64 = 0) override { return EMappingStep::Complete; }
    };
    class FApartmentMapping
    {
    public:
        void Begin(const TArray<FRoomPlan>& Rooms);
        EMappingStep Advance(FMappingWork& Work, FApartmentPlan& Out);
        EMappingStep Retire(FMappingWork& Work);
    private:
        const TArray<FRoomPlan>* Source = nullptr;
        FApartmentPlan Next;
        FApartmentEndpoint Living{ EApartmentSpace::LivingRoom, {} };
        uint32 Phase = 0, RoomIndex = 0, CompareIndex = 0;
        uint32 RetireIndex = 0, RetireField = 0;
    };
    class FDesignMapping
    {
    public:
        void Begin(FRoomShellPlan& Shell, const TOptional<FHomeDesignPlan>& Design);
        EMappingStep Advance(FMappingWork& Work);
    private:
        FRoomShellPlan* Output = nullptr;
        int32 Turns = 0;
        double Extension = 0, Yaw = 0;
        bool bHasDesign = false;
        uint32 Phase = 0, Index = 0, Child = 0;
    };
    class FBedroomMapping
    {
    public:
        // Output is fresh and remains private until Complete. It owns every
        // partially appended member if admission yields or fails.
        void Begin(FRoomShellPlan& Shell, const TOptional<FHomeDesignPlan>& Design,
            bool bCopyCameraObstacles = false);
        EMappingStep Advance(FMappingWork& Work);
    private:
        FRoomShellPlan* Output = nullptr;
        int32 Turns = 0;
        double Yaw = 0;
        bool bHasDesign = false, bCameraCopy = false;
        uint32 Phase = 0, Index = 0, Child = 0;
    };
    class FKitchenBathroomMapping
    {
    public:
        void Begin(FRoomShellPlan& Shell, bool bKitchen);
        EMappingStep Advance(FMappingWork& Work);
    private:
        FRoomShellPlan* Output = nullptr;
        bool bKitchen = false;
        uint32 Phase = 0, Index = 0, Child = 0;
    };
    class FLegacyShellMapping
    {
    public:
        void Begin(FRoomShellPlan& Shell, bool bTerrace);
        EMappingStep Advance(FMappingWork& Work);
    private:
        FRoomShellPlan* Output = nullptr;
        bool bTerrace = false;
        uint32 Phase = 0, Index = 0, Child = 0;
    };
    class FStudioShellMapping
    {
    public:
        void Begin(FRoomShellPlan& Shell, bool bBedroom);
        EMappingStep Advance(FMappingWork& Work);
    private:
        FRoomShellPlan* Output = nullptr;
        bool bBedroom = false;
        uint32 Phase = 0, Index = 0, Child = 0;
    };
    class FRoomShellMapping
    {
    public:
        void Begin(const FRoomPlan& Room, TOptional<FRoomShellPlan>& Shell);
        EMappingStep Advance(FMappingWork& Work);
    private:
        const FRoomPlan* Source = nullptr;
        TOptional<FRoomShellPlan>* Output = nullptr;
        FBedroomMapping Bedroom;
        FKitchenBathroomMapping KitchenBathroom;
        FLegacyShellMapping Legacy;
        FStudioShellMapping Studio;
        FDesignMapping Design;
        uint32 Phase = 0;
        ELayout Layout = ELayout::PatStudioV1;
        bool bLegacy = false;
    };
    class FFurnitureMapping
    {
    public:
        void Begin(FRoomPlan& Room, const FString& Path, TArray<FHomeMappingIssue>& Issues);
        EMappingStep Advance(FMappingWork& Work);
        EMappingStep Retire(FMappingWork& Work);
    private:
        FRoomPlan* Output = nullptr;
        const FString* SourcePath = nullptr;
        TArray<FHomeMappingIssue>* OutputIssues = nullptr;
        const FWallPlan* Wall = nullptr;
        const FSupportPlan* Support = nullptr;
        FString ItemPath;
        FItemDimensions Body;
        uint32 Phase = 0, Index = 0, Search = 0;
        double WallU = 0, FloorY = 0, FloorBase = 0;
        bool bAllowedKind = false, bDepression = false;
    };
    using FObject = TSharedPtr<FJsonObject>;
    struct FKnownField
    {
        const TCHAR* Name = nullptr;
        EWorldFieldPresence Presence = EWorldFieldPresence::Missing;
        EWorldValueKind Kind = EWorldValueKind::Null;
        FString Text;
        double Number = 0;
        bool Boolean = false;
        WorldReadPrivate::FValueRef Source;
    };
    struct FKnownObject
    {
        FKnownField Fields[24];
        uint32 Count = 0;
        const FKnownField* Find(const TCHAR* Name) const;
    };
    struct FUnknownRange
    {
        WorldReadPrivate::FValueRef Object;
        const TCHAR* const* Names = nullptr;
        const WorldReadPrivate::FRecordSchema* Schema = nullptr;
        uint32 Count = 0, BeforeIssue = 0;
        TCHAR Path[96]{};
    };
    struct FUnknownRanges
    {
        FUnknownRange Values[24];
        uint32 Count = 0;
    };
    // The legacy adapter borrows decoded JSON. World mapping borrows only admitted
    // fixed known slots; arbitrary extensions remain in the segmented source.
    struct FMappingObject
    {
        FObject Json;
        const FKnownObject* Known = nullptr;
        FUnknownRanges* Unknown = nullptr;
        WorldReadPrivate::FValueRef Source;
        const TCHAR* const* Names = nullptr;
        uint32 NameCount = 0;
        FMappingObject() = default;
        FMappingObject(FObject InJson) : Json(MoveTemp(InJson)) {}
        explicit FMappingObject(const FKnownObject& InKnown) : Known(&InKnown) {}
        bool HasField(const TCHAR* Name) const;
    };
    bool Text(const FMappingObject& Object, const TCHAR* Field, int32 Max, FString& Out);
    bool Number(const FMappingObject& Object, const TCHAR* Field, double& Out);
    bool OptionalNumber(const FMappingObject& Object, const TCHAR* Field, TOptional<double>& Out);
    bool OptionalBool(const FMappingObject& Object, const TCHAR* Field, TOptional<bool>& Out);
    bool OptionalColor(const FMappingObject& Object, const TCHAR* Field, TOptional<FString>& Out);
    void UnknownFields(const FMappingObject& Object, std::initializer_list<const TCHAR*> Known,
        const FString& Path, TArray<FHomeMappingIssue>& Issues);
    const WorldReadPrivate::FRecordSchema& AppearanceFields();
    const WorldReadPrivate::FRecordSchema& RoomFields();
    const WorldReadPrivate::FRecordSchema& DesignFields();
    const WorldReadPrivate::FRecordSchema& ItemFields();
    void UnknownFields(const FMappingObject& Object, const WorldReadPrivate::FRecordSchema& Fields,
        const FString& Path, TArray<FHomeMappingIssue>& Issues);
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
    class FChoiceMapping
    {
    public:
        void Begin() { Phase = Index = 0; }
        template<typename T> EMappingStep Advance(FMappingWork& Work, const FMappingObject& Object,
            const TCHAR* Field, TSourceChoice<T>& Out, std::initializer_list<TChoiceEntry<T>> Entries,
            bool bRequired, const FString& Path, TArray<FHomeMappingIssue>& Issues, TOptional<T> Default = {})
        {
            if (Phase == 0)
            {
                const auto Admission = Work.Admit(512 + (Out.Saved.IsSet()
                    ? static_cast<uint64>(Out.Saved.GetValue().Len()) : 0));
                if (Admission != EMappingStep::Complete) return Admission;
                if (Out.Saved.IsSet())
                {
                    const auto Release = Work.ReleaseText(Out.Saved.GetValue());
                    if (Release != EMappingStep::Complete) return Release;
                }
                Out = {};
                if (!Object.HasField(Field))
                {
                    if (bRequired) return EMappingStep::Invalid;
                    Out.Effective = Default; Phase = 4; return EMappingStep::Complete;
                }
                Phase = 1;
            }
            if (Phase == 1)
            {
                const auto Admission = Work.Admit(1024, 81 * sizeof(TCHAR));
                if (Admission != EMappingStep::Complete) return Admission;
                Out.Saved.Emplace();
                if (!Text(Object, Field, 80, Out.Saved.GetValue()))
                {
                    Phase = 5;
                }
                else Phase = 2;
            }
            if (Phase == 5)
            {
                const auto Release = Work.ReleaseText(Out.Saved.GetValue());
                if (Release != EMappingStep::Complete) return Release;
                Out.Saved.Reset(); return EMappingStep::Invalid;
            }
            if (Phase == 2)
            {
                while (Index < Entries.size())
                {
                    const auto Admission = Work.Admit(256);
                    if (Admission != EMappingStep::Complete) return Admission;
                    const auto& Entry = *(Entries.begin() + Index++);
                    if (Same(Out.Saved.GetValue(), Entry.Name))
                    { Out.Effective = Entry.Value; Phase = 4; return EMappingStep::Complete; }
                }
                Phase = 3;
            }
            if (Phase == 3)
            {
                const auto Admission = Work.Admit(512, (Issues.Num() + 1) * sizeof(FHomeMappingIssue)
                    + (Path.Len() + FCString::Strlen(Field) + 2) * sizeof(TCHAR));
                if (Admission != EMappingStep::Complete) return Admission;
                Issue(Issues, EHomeIssue::UnknownSelection, Path + TEXT(".") + Field); Phase = 4;
            }
            return EMappingStep::Complete;
        }
    private:
        uint32 Phase = 0, Index = 0;
    };
    class FAppearanceMapping
    {
    public:
        void Begin(const FCharacterSelection& Identity, FMappingObject Object, FAppearancePlan& Appearance,
            TArray<FCharacterMaterialRequirement>& Materials, TArray<FAttachmentRequirement>& Attachments,
            TArray<FHomeMappingIssue>& Issues);
        EMappingStep Advance(FMappingWork& Work);
        EMappingStep Retire(FMappingWork& Work);
    private:
        const FCharacterSelection* Identity = nullptr;
        FMappingObject Object;
        FAppearancePlan* Output = nullptr;
        TArray<FCharacterMaterialRequirement>* Materials = nullptr;
        TArray<FAttachmentRequirement>* Attachments = nullptr;
        TArray<FHomeMappingIssue>* Issues = nullptr;
        FChoiceMapping Choice;
        FString Path;
        TOptional<EEyeShape> Eyes;
        TOptional<EBrowStyle> Brows;
        TOptional<ENoseShape> Nose;
        TOptional<EMarkingStyle> Markings;
        uint32 Phase = 0, Index = 0;
    };
    class FItemMapping
    {
    public:
        void Begin(const FMappingObject& Object, FFurniturePlan& Item, const FString& Path,
            TArray<FHomeMappingIssue>& Issues);
        EMappingStep Advance(FMappingWork& Work);
    private:
        const FMappingObject* Source = nullptr;
        FFurniturePlan* Output = nullptr;
        const FString* SourcePath = nullptr;
        TArray<FHomeMappingIssue>* OutputIssues = nullptr;
        FChoiceMapping Choice;
        uint32 Phase = 0;
    };
    struct FRoomMappingInput
    {
        FMappingObject Room, Design, Items[8];
        uint32 ItemCount = 0;
        bool bDesignPresent = false, bDesignObject = false, bItemsArray = false;
    };
    class FRoomMapping
    {
    public:
        // Inputs/output are borrowed until completion or retirement. World output
        // starts fresh; the synchronous legacy wrapper clears its own output.
        void Begin(const FRoomSelection& Identity, const FRoomMappingInput& Input,
            FRoomPlan& Room, const FString& Path, TArray<FHomeMappingIssue>& Issues);
        EMappingStep Advance(FMappingWork& Work);
        EMappingStep Retire(FMappingWork& Work);
    private:
        const FRoomSelection* Identity = nullptr;
        const FRoomMappingInput* Source = nullptr;
        FRoomPlan* Output = nullptr;
        const FString* SourcePath = nullptr;
        TArray<FHomeMappingIssue>* OutputIssues = nullptr;
        FChoiceMapping Choice;
        FItemMapping Item;
        FRoomShellMapping Shell;
        FFurnitureMapping Furniture;
        FFurniturePlan ItemOutput;
        FString ItemPath;
        double Version = 0, Turns = 0, Extension = 0;
        uint32 Phase = 0, Index = 0, CompareIndex = 0, ItemPhase = 0, RetireField = 0;
    };
    // Null/numeric fields are malformed, unknown strings are retained without a guessed default.
    template<typename T> bool Choice(const FMappingObject& Object, const TCHAR* Field, TSourceChoice<T>& Out,
        std::initializer_list<TChoiceEntry<T>> Entries, bool bRequired, const FString& Path,
        TArray<FHomeMappingIssue>& Issues, TOptional<T> Default = {})
    {
        FLegacyMappingWork Work;
        FChoiceMapping Mapping;
        Mapping.Begin();
        return Mapping.Advance<T>(Work, Object, Field, Out, Entries, bRequired, Path, Issues, Default)
            == EMappingStep::Complete;
    }

    FCanonicalAssetReference CharacterReference();
    TOptional<FCanonicalAssetReference> RoomReference(ELayout Layout);
    FCanonicalAssetReference TerraceReference();
    FSourcePose Pose(const FVector& Point, double Yaw = 0);
    FSourceBox Box(const FVector& BottomCentre, const FVector& Dimensions);
    TArray<EItemKind> SupportKinds(ESurface Surface);
    bool SupportKindAt(ESurface Surface, uint32 Index, EItemKind& Out);
    FRoomShellPlan BedroomShell(const TOptional<FHomeDesignPlan>& Design);
    TOptional<FRoomShellPlan> RoomShell(const FRoomPlan& Room);
    bool ReadAppearance(const FCharacterSelection& Input, FAppearancePlan& Out,
        TArray<FCharacterMaterialRequirement>& Materials, TArray<FAttachmentRequirement>& Attachments,
        TArray<FHomeMappingIssue>& Issues);
    bool ReadAppearanceKnown(const FCharacterSelection& Identity, const FKnownObject& Object,
        FAppearancePlan& Out, TArray<FCharacterMaterialRequirement>& Materials,
        TArray<FAttachmentRequirement>& Attachments, TArray<FHomeMappingIssue>& Issues);
    bool ReadRoom(const FRoomSelection& Input, FRoomPlan& Out, const FString& Path,
        TArray<FHomeMappingIssue>& Issues);
    void MapFurniture(FRoomPlan& Room, const FString& Path, TArray<FHomeMappingIssue>& Issues);
}
