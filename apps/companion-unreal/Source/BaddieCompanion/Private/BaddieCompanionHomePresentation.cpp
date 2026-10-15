#include "BaddieCompanionHomePresentation.h"

#include "BaddieCompanionHomeMappingInternal.h"

#include "Containers/StringConv.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace BaddieCompanion
{
    FVector SourceToUnreal(const FVector& Metres) { return FVector(100 * Metres.Z, 100 * Metres.X, 100 * Metres.Y); }
    double SourceYawToUnreal(double Radians) { return Radians * 180.0 / UE_DOUBLE_PI; }
    FVector TurnSourceShell(const FVector& Metres, int32 Turns)
    {
        switch (Turns)
        {
        case 1: return FVector(Metres.Z, Metres.Y, -Metres.X);
        case 2: return FVector(-Metres.X, Metres.Y, -Metres.Z);
        case 3: return FVector(-Metres.Z, Metres.Y, Metres.X);
        default: return Metres;
        }
    }

    bool SamePresentationInput(const FSnapshotSelection& A, const FSnapshotSelection& B)
    {
        const auto CharacterSame = [](const FCharacterSelection& L, const FCharacterSelection& R)
        {
            return HomeMapping::Same(L.Id, R.Id) && HomeMapping::Same(L.Name, R.Name)
                && HomeMapping::Same(L.BaseId, R.BaseId) && HomeMapping::Same(L.Look, R.Look)
                && HomeMapping::Same(L.AppearanceJson, R.AppearanceJson);
        };
        if (A.Rooms.Num() != B.Rooms.Num()
            || A.HasCompleteApartmentInventory() != B.HasCompleteApartmentInventory()) return false;
        for (int32 Index = 0; Index < A.Rooms.Num(); ++Index)
            if (!HomeMapping::SameRoomSelection(A.Rooms[Index], B.Rooms[Index])) return false;
        return A.Revision == B.Revision && CharacterSame(A.Character, B.Character)
            && HomeMapping::SameRoomSelection(A.HomeRoom, B.HomeRoom)
            && HomeMapping::SameRoomSelection(A.ActiveRoom, B.ActiveRoom);
    }

    bool BuildHomePresentationPlan(const FSnapshotSelection& Input,
        FHomePresentationPlan& Out, TArray<FHomeMappingIssue>& OutIssues)
    {
        Out = {};
        OutIssues.Reset();
        FHomePresentationPlan Next;
        if (Input.Revision < 0 || Input.Revision > 9007199254740991LL
            || !HomeMapping::ReadAppearance(Input.Character, Next.Appearance,
                Next.CharacterMaterials, Next.Attachments, OutIssues)
            || !HomeMapping::ReadRoom(Input.HomeRoom, Next.Home, TEXT("home"), OutIssues)
            || !HomeMapping::ReadRoom(Input.ActiveRoom, Next.ActiveRoom, TEXT("activeRoom"), OutIssues))
        {
            HomeMapping::Issue(OutIssues, EHomeIssue::InvalidDocument, TEXT("selection"));
            return false;
        }
        // Same saved identity cannot carry two divergent room documents in one snapshot.
        if (HomeMapping::Same(Input.HomeRoom.Id, Input.ActiveRoom.Id)
            && !HomeMapping::SameRoomSelection(Input.HomeRoom, Input.ActiveRoom))
        {
            HomeMapping::Issue(OutIssues, EHomeIssue::IdentityMismatch, TEXT("activeRoom"));
            return false;
        }
        if (Input.Rooms.IsEmpty())
        {
            // Older C++ selection-only callers retain their known rooms, without inventing inventory.
            Next.Rooms.Add(Next.Home);
            if (!HomeMapping::Same(Input.HomeRoom.Id, Input.ActiveRoom.Id)) Next.Rooms.Add(Next.ActiveRoom);
        }
        else
        {
            bool bFoundHome = false, bFoundActive = false;
            if (Input.Rooms.Num() > 12)
            {
                HomeMapping::Issue(OutIssues, EHomeIssue::InvalidDocument, TEXT("rooms"));
                return false;
            }
            for (int32 Index = 0; Index < Input.Rooms.Num(); ++Index)
            {
                const auto& SourceRoom = Input.Rooms[Index];
                const FString Path = FString::Printf(TEXT("rooms[%d]"), Index);
                const bool bHome = HomeMapping::Same(SourceRoom.Id, Input.HomeRoom.Id);
                const bool bActive = HomeMapping::Same(SourceRoom.Id, Input.ActiveRoom.Id);
                if (Next.Rooms.ContainsByPredicate([&SourceRoom](const FRoomPlan& Room)
                    { return HomeMapping::Same(Room.Id, SourceRoom.Id); })
                    || (bHome && !HomeMapping::SameRoomSelection(SourceRoom, Input.HomeRoom))
                    || (bActive && !HomeMapping::SameRoomSelection(SourceRoom, Input.ActiveRoom)))
                {
                    HomeMapping::Issue(OutIssues, EHomeIssue::IdentityMismatch, Path);
                    return false;
                }
                FRoomPlan Room;
                if (bHome) { Room = Next.Home; bFoundHome = true; }
                else if (bActive) Room = Next.ActiveRoom;
                else if (!HomeMapping::ReadRoom(SourceRoom, Room, Path, OutIssues))
                {
                    HomeMapping::Issue(OutIssues, EHomeIssue::InvalidDocument, Path);
                    return false;
                }
                bFoundActive |= bActive;
                Next.Rooms.Add(MoveTemp(Room));
            }
            if (!bFoundHome || !bFoundActive)
            {
                HomeMapping::Issue(OutIssues, EHomeIssue::IdentityMismatch, TEXT("rooms"));
                return false;
            }
        }
        if (!BuildApartmentPlan(Next.Rooms, Next.Apartment))
        {
            HomeMapping::Issue(OutIssues, EHomeIssue::InvalidDocument, TEXT("apartment"));
            return false;
        }
        Next.Apartment.bCompleteHomeInventoryVerified = Input.HasCompleteApartmentInventory();
        Next.Source = Input;
        Next.CharacterAsset = HomeMapping::CharacterReference();
        HomeMapping::Issue(OutIssues, EHomeIssue::AssetImportUnavailable, TEXT("character.asset"));
        Next.Issues = OutIssues;
        Out = MoveTemp(Next);
        return true; // A typed, possibly unsupported plan, NEVER a render-ready result.
    }

    void FHomePresentationCache::Clear() { LastInput.Reset(); Plan.Reset(); Issues.Reset(); }
    bool FHomePresentationCache::Matches(const FSnapshotSelection& Input) const
    {
        return LastInput.IsSet() && SamePresentationInput(LastInput.GetValue(), Input);
    }
    void FHomePresentationCache::Update(const FSnapshotSelection& Input)
    {
        if (Matches(Input)) return;
        Clear(); // A failed/unsupported replacement must not retain the previous selection's plan.
        FHomePresentationPlan Next;
        if (BuildHomePresentationPlan(Input, Next, Issues)) Plan = MoveTemp(Next);
        LastInput = Input; // Exact negative cache too; cleared with the live grant, not persisted.
    }
    bool FHomePresentationCache::CopyPlan(FHomePresentationPlan& Out) const
    {
        Out = {};
        if (!Plan.IsSet()) return false;
        Out = Plan.GetValue();
        return true;
    }
}

namespace BaddieCompanion::HomeMapping
{
    namespace
    {
        bool IsColorValue(const FString& Value)
        {
            if (Value.Len() != 7 || Value[0] != TEXT('#')) return false;
            for (int32 I = 1; I < 7; ++I)
            {
                const TCHAR C = Value[I];
                if (!((C >= TEXT('0') && C <= TEXT('9')) || (C >= TEXT('a') && C <= TEXT('f'))
                    || (C >= TEXT('A') && C <= TEXT('F')))) return false;
            }
            return true;
        }
    }

    const WorldReadPrivate::FRecordSchema& AppearanceFields()
    {
        using namespace WorldReadPrivate;
        using K = EWorldValueKind;
        static constexpr FTextChoice Garments[] = { TEXT("bomber"), TEXT("t-shirt"), TEXT("shirt"),
            TEXT("a-line-dress"), TEXT("pleated-dress"), TEXT("wrap-dress"), TEXT("evening-gown") };
        static constexpr FTextChoice Fabrics[] = { TEXT("cotton"), TEXT("linen"), TEXT("knit"), TEXT("denim"), TEXT("satin"), TEXT("velvet") };
        static constexpr FTextChoice Patterns[] = { TEXT("plain"), TEXT("stripes"), TEXT("gingham"), TEXT("dots") };
        static constexpr FTextChoice Eyes[] = { TEXT("classic"), TEXT("round"), TEXT("almond"), TEXT("sleepy") };
        static constexpr FTextChoice Brows[] = { TEXT("classic"), TEXT("soft"), TEXT("bold"), TEXT("curious") };
        static constexpr FTextChoice Noses[] = { TEXT("classic"), TEXT("button"), TEXT("wide") };
        static constexpr FTextChoice Mouths[] = { TEXT("classic"), TEXT("small"), TEXT("wide") };
        static constexpr FTextChoice Cheeks[] = { TEXT("none"), TEXT("blush"), TEXT("freckles") };
        static constexpr FTextChoice Markings[] = { TEXT("none"), TEXT("muzzle"), TEXT("mask"), TEXT("stripes") };
        static constexpr FTextChoice Eyewear[] = { TEXT("none"), TEXT("round") };
        static const FFieldRule Fields[] = {
            SelectionDomainSchema(EDomain::Appearance).Record->Fields[0],
            SelectionDomainSchema(EDomain::Appearance).Record->Fields[1],
            FFieldRule(TEXT("bodyColor"), K::String, false).WithText({7, 7}),
            FFieldRule(TEXT("topColor"), K::String, false).WithText({7, 7}),
            FFieldRule(TEXT("trousersColor"), K::String, false).WithText({7, 7}),
            FFieldRule(TEXT("footwearColor"), K::String, false).WithText({7, 7}),
            FFieldRule(TEXT("eyeColor"), K::String, false).WithText({7, 7}),
            FFieldRule(TEXT("noseColor"), K::String, false).WithText({7, 7}),
            FFieldRule(TEXT("markingColor"), K::String, false).WithText({7, 7}),
            FFieldRule(TEXT("patternColor"), K::String, false).WithText({7, 7}),
            FFieldRule(TEXT("accessoryVisible"), K::Boolean, false),
            FFieldRule(TEXT("necklaceVisible"), K::Boolean, false),
            FFieldRule(TEXT("hipChainVisible"), K::Boolean, false),
            FFieldRule(TEXT("garmentStyle"), K::String, false).WithText({1, 80, false, false, true}).WithChoices(Garments),
            FFieldRule(TEXT("fabricTexture"), K::String, false).WithText({1, 80, false, false, true}).WithChoices(Fabrics),
            FFieldRule(TEXT("clothingPattern"), K::String, false).WithText({1, 80, false, false, true}).WithChoices(Patterns),
            FFieldRule(TEXT("eyeShape"), K::String, false).WithText({1, 80, false, false, true}).WithChoices(Eyes),
            FFieldRule(TEXT("browStyle"), K::String, false).WithText({1, 80, false, false, true}).WithChoices(Brows),
            FFieldRule(TEXT("noseShape"), K::String, false).WithText({1, 80, false, false, true}).WithChoices(Noses),
            FFieldRule(TEXT("mouthShape"), K::String, false).WithText({1, 80, false, false, true}).WithChoices(Mouths),
            FFieldRule(TEXT("cheekStyle"), K::String, false).WithText({1, 80, false, false, true}).WithChoices(Cheeks),
            FFieldRule(TEXT("markingStyle"), K::String, false).WithText({1, 80, false, false, true}).WithChoices(Markings),
            FFieldRule(TEXT("eyewear"), K::String, false).WithText({1, 80, false, false, true}).WithChoices(Eyewear)
        };
        static const FRecordSchema Schema(Fields, false);
        return Schema;
    }
    const WorldReadPrivate::FRecordSchema& RoomFields()
    {
        using namespace WorldReadPrivate;
        using K = EWorldValueKind;
        static constexpr FTextChoice Environments[] = { TEXT("studio"), TEXT("terrace") };
        static constexpr FTextChoice Layouts[] = { TEXT("pat-studio-v01"), TEXT("pink-bedroom-v01"),
            TEXT("pink-bedroom-v02"), TEXT("modern-kitchen-v01"), TEXT("bathroom-v01") };
        static constexpr FTextChoice Palettes[] = { TEXT("rose"), TEXT("sage"), TEXT("midnight") };
        static constexpr FTextChoice Lights[] = { TEXT("daylight"), TEXT("golden"), TEXT("evening") };
        static constexpr FTextChoice Sharing[] = { TEXT("private"), TEXT("host"), TEXT("collaborative") };
        static constexpr FFieldRule Fields[] = {
            FFieldRule(TEXT("id"), K::String).WithText({1, 80, false, false, true}),
            FFieldRule(TEXT("name"), K::String).WithText({1, 32, false, false, true}),
            FFieldRule(TEXT("environmentId"), K::String).WithText({1, 80, false, false, true}).WithChoices(Environments),
            FFieldRule(TEXT("layoutId"), K::String, false).WithText({1, 80, false, false, true}).WithChoices(Layouts),
            FFieldRule(TEXT("homeDesign"), K::Object, false),
            FFieldRule(TEXT("palette"), K::String).WithText({1, 80, false, false, true}).WithChoices(Palettes),
            FFieldRule(TEXT("light"), K::String).WithText({1, 80, false, false, true}).WithChoices(Lights),
            FFieldRule(TEXT("sharing"), K::String).WithText({1, 80, false, false, true}).WithChoices(Sharing),
            FFieldRule(TEXT("items"), K::Array).WithCount(0, 8),
            FFieldRule(TEXT("wallColor"), K::String, false).WithText({7, 7}),
            FFieldRule(TEXT("floorColor"), K::String, false).WithText({7, 7}),
            FFieldRule(TEXT("upholsteryColor"), K::String, false).WithText({7, 7}),
            FFieldRule(TEXT("rugColor"), K::String, false).WithText({7, 7})
        };
        static constexpr FRecordSchema Schema(Fields, false);
        return Schema;
    }
    const WorldReadPrivate::FRecordSchema& DesignFields()
    {
        using namespace WorldReadPrivate;
        using K = EWorldValueKind;
        static constexpr FFieldRule Fields[] = {
            FFieldRule(TEXT("version"), K::Number).WithNumber(1, 1),
            FFieldRule(TEXT("turns"), K::Number).WithNumber(0, 3, true),
            FFieldRule(TEXT("extension"), K::Number).WithNumber(0, 1.2)
        };
        static constexpr FRecordSchema Schema(Fields, false);
        return Schema;
    }
    const WorldReadPrivate::FRecordSchema& ItemFields()
    {
        using namespace WorldReadPrivate;
        using K = EWorldValueKind;
        static constexpr FTextChoice Kinds[] = { TEXT("lamp"), TEXT("stool"), TEXT("plant"), TEXT("botanical-painting"),
            TEXT("abstract-painting"), TEXT("landscape-painting"), TEXT("wall-clock"), TEXT("wall-mirror"),
            TEXT("standing-fan"), TEXT("tower-fan"), TEXT("desk-fan"), TEXT("floor-lamp"), TEXT("desk-lamp"),
            TEXT("armchair"), TEXT("loveseat"), TEXT("ottoman"), TEXT("coffee-table"), TEXT("side-table"),
            TEXT("bookcase"), TEXT("storage-basket"), TEXT("book-stack"), TEXT("candle"), TEXT("succulent") };
        static constexpr FTextChoice Surfaces[] = { TEXT("floor"), TEXT("console"), TEXT("shelf"), TEXT("wall-back"),
            TEXT("wall-left"), TEXT("wall-right"), TEXT("wall-front") };
        static constexpr FFieldRule Fields[] = {
            FFieldRule(TEXT("id"), K::String).WithText({1, 80, false, false, true}),
            FFieldRule(TEXT("kind"), K::String).WithText({1, 80, false, false, true}).WithChoices(Kinds),
            FFieldRule(TEXT("x"), K::Number).WithNumber(-4, 4),
            FFieldRule(TEXT("z"), K::Number).WithNumber(-4, 4),
            FFieldRule(TEXT("rotation"), K::Number).WithNumber(-10000, 10000),
            FFieldRule(TEXT("surface"), K::String, false).WithText({1, 80, false, false, true}).WithChoices(Surfaces),
            FFieldRule(TEXT("y"), K::Number, false).WithNumber(0, 4),
            FFieldRule(TEXT("lit"), K::Boolean, false)
        };
        static constexpr FRecordSchema Schema(Fields, false);
        return Schema;
    }
    void UnknownFields(const FMappingObject& Object, const WorldReadPrivate::FRecordSchema& Fields,
        const FString& Path, TArray<FHomeMappingIssue>& Issues)
    {
        if (!Object.Known)
        {
            if (!Object.Json) return;
            for (const auto& Pair : Object.Json->Values)
            {
                bool bKnown = false;
                for (uint32 I = 0; I < Fields.Count; ++I)
                    if (MakeStringView(Pair.Key).Equals(Fields.Fields[I].Name, ESearchCase::CaseSensitive))
                    { bKnown = true; break; }
                if (!bKnown) Issue(Issues, EHomeIssue::UnknownField, Path);
            }
        }
        else if (Object.Unknown && Object.Source.IsSet())
        {
            check(Object.Unknown->Count < UE_ARRAY_COUNT(Object.Unknown->Values));
            check(Path.Len() < 96);
            auto& Range = Object.Unknown->Values[Object.Unknown->Count++];
            Range.Object = Object.Source; Range.Schema = &Fields;
            Range.Count = Fields.Count; Range.BeforeIssue = static_cast<uint32>(Issues.Num());
            for (int32 I = 0; I < Path.Len(); ++I) Range.Path[I] = Path[I];
            Range.Path[Path.Len()] = 0;
        }
    }
    const FKnownField* FKnownObject::Find(const TCHAR* Name) const
    {
        for (uint32 I = 0; I < Count; ++I)
            if (Fields[I].Name && FCString::Strcmp(Fields[I].Name, Name) == 0) return &Fields[I];
        return nullptr;
    }
    bool FMappingObject::HasField(const TCHAR* Name) const
    {
        if (!Known) return Json && Json->HasField(Name);
        const auto* Field = Known->Find(Name);
        return Field && Field->Presence != EWorldFieldPresence::Missing;
    }
    bool Text(const FMappingObject& Object, const TCHAR* Field, int32 Max, FString& Out)
    {
        if (!Object.Known) return Text(Object.Json, Field, Max, Out);
        const auto* Value = Object.Known->Find(Field);
        if (!Value || Value->Presence != EWorldFieldPresence::Value || Value->Kind != EWorldValueKind::String
            || Value->Text.Len() > Max || Value->Text.TrimStartAndEnd().IsEmpty()) return false;
        Out = Value->Text; return true;
    }
    bool Number(const FMappingObject& Object, const TCHAR* Field, double& Out)
    {
        if (!Object.Known) return Number(Object.Json, Field, Out);
        const auto* Value = Object.Known->Find(Field);
        if (!Value || Value->Presence != EWorldFieldPresence::Value || Value->Kind != EWorldValueKind::Number
            || !FMath::IsFinite(Value->Number)) return false;
        Out = Value->Number; return true;
    }
    bool OptionalNumber(const FMappingObject& Object, const TCHAR* Field, TOptional<double>& Out)
    {
        if (!Object.Known) return OptionalNumber(Object.Json, Field, Out);
        Out.Reset();
        if (!Object.HasField(Field)) return true;
        double Value = 0;
        if (!Number(Object, Field, Value)) return false;
        Out = Value; return true;
    }
    bool OptionalBool(const FMappingObject& Object, const TCHAR* Field, TOptional<bool>& Out)
    {
        if (!Object.Known) return OptionalBool(Object.Json, Field, Out);
        Out.Reset();
        if (!Object.HasField(Field)) return true;
        const auto* Value = Object.Known->Find(Field);
        if (!Value || Value->Presence != EWorldFieldPresence::Value || Value->Kind != EWorldValueKind::Boolean) return false;
        Out = Value->Boolean; return true;
    }
    bool OptionalColor(const FMappingObject& Object, const TCHAR* Field, TOptional<FString>& Out)
    {
        if (!Object.Known) return OptionalColor(Object.Json, Field, Out);
        Out.Reset();
        if (!Object.HasField(Field)) return true;
        FString Value;
        if (!Text(Object, Field, 7, Value) || !IsColorValue(Value)) return false;
        Out = MoveTemp(Value); return true;
    }
    void UnknownFields(const FMappingObject& Object, std::initializer_list<const TCHAR*> Known,
        const FString& Path, TArray<FHomeMappingIssue>& Issues)
    {
        // World unknown members are enumerated from exact source, not copied into
        // these bounded slots or multiplied into an owning issue array.
        if (!Object.Known) UnknownFields(Object.Json, Known, Path, Issues);
        else if (Object.Unknown && Object.Names && Object.Source.IsSet())
        {
            check(Object.Unknown->Count < UE_ARRAY_COUNT(Object.Unknown->Values));
            check(Path.Len() < 96);
            auto& Range = Object.Unknown->Values[Object.Unknown->Count++];
            Range.Object = Object.Source; Range.Names = Object.Names;
            Range.Count = Object.NameCount; Range.BeforeIssue = static_cast<uint32>(Issues.Num());
            for (int32 I = 0; I < Path.Len(); ++I) Range.Path[I] = Path[I];
            Range.Path[Path.Len()] = 0;
        }
    }

    bool SupportKindAt(ESurface Surface, uint32 Index, EItemKind& Out)
    {
        static constexpr EItemKind Common[] = { EItemKind::Lamp, EItemKind::Plant, EItemKind::DeskFan,
            EItemKind::DeskLamp, EItemKind::BookStack, EItemKind::Candle, EItemKind::Succulent };
        static constexpr EItemKind Floor[] = { EItemKind::Stool, EItemKind::StandingFan,
            EItemKind::TowerFan, EItemKind::FloorLamp, EItemKind::Armchair, EItemKind::Loveseat,
            EItemKind::Ottoman, EItemKind::CoffeeTable, EItemKind::SideTable, EItemKind::Bookcase,
            EItemKind::StorageBasket };
        if (Index < UE_ARRAY_COUNT(Common)) { Out = Common[Index]; return true; }
        Index -= UE_ARRAY_COUNT(Common);
        if (Surface == ESurface::Floor && Index < UE_ARRAY_COUNT(Floor))
        { Out = Floor[Index]; return true; }
        return false;
    }
    TArray<EItemKind> SupportKinds(ESurface Surface)
    {
        TArray<EItemKind> Out;
        EItemKind Kind;
        for (uint32 Index = 0; SupportKindAt(Surface, Index, Kind); ++Index) Out.Add(Kind);
        return Out;
    }

    bool ReadObject(const FString& Json, FObject& Out)
    {
        Out.Reset();
        if (Json.IsEmpty() || Json.Len() > MaxSnapshotBytes
            || FTCHARToUTF8(*Json, Json.Len()).Length() > MaxSnapshotBytes) return false;
        return FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Out) && Out.IsValid();
    }
    bool Text(const FObject& Object, const TCHAR* Field, int32 Max, FString& Out)
    {
        const auto Value = Object.IsValid() ? Object->TryGetField(Field) : nullptr;
        return Value.IsValid() && Value->Type == EJson::String && Value->TryGetString(Out)
            && Out.Len() <= Max && !Out.TrimStartAndEnd().IsEmpty();
    }
    bool Number(const FObject& Object, const TCHAR* Field, double& Out)
    {
        const auto Value = Object.IsValid() ? Object->TryGetField(Field) : nullptr;
        return Value.IsValid() && Value->Type == EJson::Number && Value->TryGetNumber(Out) && FMath::IsFinite(Out);
    }
    bool OptionalNumber(const FObject& Object, const TCHAR* Field, TOptional<double>& Out)
    {
        Out.Reset();
        if (!Object->HasField(Field)) return true;
        double Value = 0;
        if (!Number(Object, Field, Value)) return false;
        Out = Value;
        return true;
    }
    bool OptionalBool(const FObject& Object, const TCHAR* Field, TOptional<bool>& Out)
    {
        Out.Reset();
        if (!Object->HasField(Field)) return true;
        const auto Value = Object->TryGetField(Field);
        bool Parsed = false;
        if (!Value.IsValid() || Value->Type != EJson::Boolean || !Value->TryGetBool(Parsed)) return false;
        Out = Parsed;
        return true;
    }
    bool OptionalColor(const FObject& Object, const TCHAR* Field, TOptional<FString>& Out)
    {
        Out.Reset();
        if (!Object->HasField(Field)) return true;
        FString Value;
        if (!Text(Object, Field, 7, Value) || !IsColorValue(Value)) return false;
        Out = Value; // Preserve supplied case, not a normalized replacement document.
        return true;
    }
    void UnknownFields(const FObject& Object, std::initializer_list<const TCHAR*> Known,
        const FString& Path, TArray<FHomeMappingIssue>& Issues)
    {
        for (const auto& Pair : Object->Values)
        {
            bool bKnown = false;
            for (const TCHAR* Key : Known)
                if (MakeStringView(Pair.Key).Equals(Key, ESearchCase::CaseSensitive)) { bKnown = true; break; }
            // Unknown key text is not copied into an issue (could itself contain private data).
            if (!bKnown) Issue(Issues, EHomeIssue::UnknownField, Path);
        }
    }
    FSourcePose Pose(const FVector& Point, double Yaw)
    {
        return { Point, Yaw, SourceToUnreal(Point), SourceYawToUnreal(Yaw) };
    }
    FSourceBox Box(const FVector& BottomCentre, const FVector& Dimensions)
    {
        FSourceBox Result;
        Result.BottomCentreMetres = BottomCentre;
        Result.DimensionsMetres = Dimensions;
        const FVector Centre = SourceToUnreal(BottomCentre + FVector(0, Dimensions.Y / 2, 0));
        const FVector Half = SourceToUnreal(Dimensions) / 2;
        Result.UnrealBounds = FBox(Centre - Half, Centre + Half);
        return Result;
    }
    FCanonicalAssetReference CharacterReference()
    {
        return { TEXT("bestieCharacterV27"),
            TEXT("bestie/characters/bestie-v27a-lossless.552e83aae0f42244.glb"),
            TEXT("552e83aae0f422440c913993f8ef25e2a7495406ad9bf86238e87445487806f2"),
            TEXT("787c634f30d73f440ba42a1fff875c46"), 15887680, true };
    }
    TOptional<FCanonicalAssetReference> RoomReference(ELayout Layout)
    {
        // Catalog references do not establish native geometry or material availability.
        switch (Layout)
        {
        case ELayout::PinkBedroomV2: return FCanonicalAssetReference{ TEXT("bestiePinkBedroomV2"),
            TEXT("bestie/rooms/pink-bedroom-v15-lossless.7765d65d8419ffa8.glb"),
            TEXT("7765d65d8419ffa86f9136f8fdb22640994af03dfa901b1332443f73463ae2e3"),
            TEXT("6f32b1978494f231715c6628de4b67d2"), 4829976, true };
        case ELayout::PatStudioV1: return FCanonicalAssetReference{ TEXT("bestiePatStudio"),
            TEXT("bestie/rooms/studio-v07-lossless.cf20f87f5f2eac92.glb"),
            TEXT("cf20f87f5f2eac92a306e121dca39917444455699ab8da80a4f458c5fb5d769c"),
            TEXT("f5d25bf6b3a2d6dec64f3119c4ae593a"), 19580572, true };
        case ELayout::PinkBedroomV1: return FCanonicalAssetReference{ TEXT("bestiePinkBedroom"),
            TEXT("bestie/rooms/pink-bedroom-v03.cef0a0cc8ec684f4.glb"),
            TEXT("cef0a0cc8ec684f47eb3186de71e837e0b02e48b938a82dd01b62fa108e187d9"),
            TEXT("c3d24ac57a91b67ccb0002c1831382df"), 1242604, false };
        case ELayout::KitchenV1: return FCanonicalAssetReference{ TEXT("bestieKitchen"),
            TEXT("bestie/rooms/modern-kitchen-v01.3d2f160c5a001827.glb"),
            TEXT("3d2f160c5a0018270a7762e38e029fe7e1eee20d98d4a51dd4805f5edac53308"),
            TEXT("7d0e22d8411df8819b783a58774d7c99"), 1377752, true };
        case ELayout::BathroomV1: return FCanonicalAssetReference{ TEXT("bestieBathroom"),
            TEXT("bestie/rooms/bathroom-v01.1715c195b8d09dad.glb"),
            TEXT("1715c195b8d09dadb980e1919500f1d84ed779bf016c94e7e8130dff77ac423b"),
            TEXT("4a5326e836ac16c566d33acf5f00b37a"), 795984, true };
        }
        return {};
    }
    FCanonicalAssetReference TerraceReference()
    {
        return { TEXT("bestieTerrace"), TEXT("bestie/rooms/terrace-v03-lossless.2765c88f90bd5e68.glb"),
            TEXT("2765c88f90bd5e689aa4f618d148be141543346569266b25c19c74cfe18920be"),
            TEXT("efae016b35b7b500d9db8be600329afa"), 3419968, true };
    }
}
