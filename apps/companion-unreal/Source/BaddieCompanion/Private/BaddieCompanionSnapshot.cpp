#include "BaddieCompanionSnapshot.h"
#include "BaddieCompanionWorldPlayerRead.h"

#include "Containers/StringConv.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace BaddieCompanion::WorldReadPrivate
{
    const FDomainSchema& SelectionDomainSchema(EDomain Domain)
    {
        using K = EWorldValueKind;
        constexpr FTextRule Id{1, 80, false, false, true};
        switch (Domain)
        {
        case EDomain::Selection:
        {
            static constexpr FFieldRule F[] = {FFieldRule(TEXT("revision"), K::Number).WithNumber(0, MaxSafeInteger, true), FFieldRule(TEXT("library"), K::Object)};
            static constexpr FRecordSchema R(F, false); static constexpr FChildRule C[] = {{1, EDomain::Library}};
            static const FDomainSchema S{&R, C, UE_ARRAY_COUNT(C)}; return S;
        }
        case EDomain::Library:
        {
            static constexpr FFieldRule F[] = {FFieldRule(TEXT("version"), K::Number).WithNumber(1, 1),
                FFieldRule(TEXT("activeCharacterId"), K::String).WithText(Id), FFieldRule(TEXT("activeRoomId"), K::String).WithText(Id),
                FFieldRule(TEXT("homeRoomId"), K::String).WithText(Id), FFieldRule(TEXT("characters"), K::Array).WithCount(1, 12),
                FFieldRule(TEXT("rooms"), K::Array).WithCount(1, 12)};
            static constexpr FRecordSchema R(F, false); static constexpr FChildRule C[] = {{4, EDomain::Characters}, {5, EDomain::Rooms}};
            static const FDomainSchema S{&R, C, UE_ARRAY_COUNT(C)}; return S;
        }
        case EDomain::SavedCharacter:
        {
            static constexpr FFieldRule F[] = {FFieldRule(TEXT("id"), K::String).WithText(Id),
                FFieldRule(TEXT("name"), K::String).WithText({1, 20, false, false, true}), FFieldRule(TEXT("appearance"), K::Object)};
            static constexpr FRecordSchema R(F, false); static constexpr FChildRule C[] = {{2, EDomain::Appearance}};
            static const FDomainSchema S{&R, C, UE_ARRAY_COUNT(C)}; return S;
        }
        case EDomain::Appearance:
        {
            static constexpr FTextChoice Looks[] = {TEXT("rose"), TEXT("ivory")};
            static constexpr FTextChoice Bases[] = {TEXT("bestie"), TEXT("bunny"), TEXT("bear"), TEXT("cat"), TEXT("fox"), TEXT("panda")};
            static constexpr FFieldRule F[] = {FFieldRule(TEXT("look"), K::String).WithText({1, 16, false, false, true}).WithChoices(Looks),
                FFieldRule(TEXT("baseId"), K::String, false).WithText({1, 16, false, false, true}).WithChoices(Bases)};
            static constexpr FRecordSchema R(F, false); static const FDomainSchema S{&R}; return S;
        }
        case EDomain::SavedRoom:
        {
            static constexpr FTextChoice Environments[] = {TEXT("studio"), TEXT("terrace")};
            static constexpr FFieldRule F[] = {FFieldRule(TEXT("id"), K::String).WithText(Id),
                FFieldRule(TEXT("name"), K::String).WithText({1, 32, false, false, true}),
                FFieldRule(TEXT("environmentId"), K::String).WithText({1, 16, false, false, true}).WithChoices(Environments),
                FFieldRule(TEXT("layoutId"), K::String, false).WithText(Id)};
            static constexpr FRecordSchema R(F, false); static const FDomainSchema S{&R}; return S;
        }
        case EDomain::Characters: { static const FDomainSchema S{nullptr, nullptr, 0, K::Array, EDomain::SavedCharacter, 1, 12}; return S; }
        case EDomain::Rooms: { static const FDomainSchema S{nullptr, nullptr, 0, K::Array, EDomain::SavedRoom, 1, 12}; return S; }
        default: { static const FDomainSchema S{nullptr, nullptr, 0, K::StringBlock}; return S; }
        }
    }
}

namespace
{
    using namespace BaddieCompanion::WorldReadPrivate;
    bool SameText(const FString& Left, const FString& Right)
    {
        return Left.Equals(Right, ESearchCase::CaseSensitive);
    }

    bool ContainsId(const TArray<FString>& Ids, const FString& Id)
    {
        return Ids.ContainsByPredicate([&Id](const FString& Existing) { return SameText(Existing, Id); });
    }

    TSharedPtr<FJsonObject> ObjectField(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key)
    {
        const TSharedPtr<FJsonObject>* Value = nullptr;
        return Object.IsValid() && Object->TryGetObjectField(Key, Value) && Value
            ? *Value : nullptr;
    }

    bool ReadText(const TSharedPtr<FJsonObject>& Object, EDomain Domain, uint32 Index, FString& Out)
    {
        const auto& Rule = DomainSchema(Domain).Record->Fields[Index];
        const auto Value = Object.IsValid() ? Object->TryGetField(Rule.Name) : nullptr;
        return Value.IsValid() && Value->Type == EJson::String && Value->TryGetString(Out)
            && TextValue(Out, Rule.Text);
    }

    bool ReadNumber(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key, double& Out)
    {
        const auto Value = Object.IsValid() ? Object->TryGetField(Key) : nullptr;
        return Value.IsValid() && Value->Type == EJson::Number && Value->TryGetNumber(Out);
    }

    bool ReadArray(const TSharedPtr<FJsonObject>& Object, uint32 Index,
        const TArray<TSharedPtr<FJsonValue>>*& Out)
    {
        const auto& Bounds = DomainSchema(EDomain::Library).Record->Fields[Index];
        return Object.IsValid() && Object->TryGetArrayField(Bounds.Name, Out) && Out
            && static_cast<uint64>(Out->Num()) >= Bounds.MinCount && static_cast<uint64>(Out->Num()) <= Bounds.MaxCount;
    }

    bool WriteObject(const TSharedPtr<FJsonObject>& Object, FString& Out)
    {
        const auto Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Out);
        return FJsonSerializer::Serialize(Object.ToSharedRef(), Writer);
    }

    bool ReadCharacter(const TSharedPtr<FJsonObject>& Object, BaddieCompanion::FCharacterSelection& Out)
    {
        using namespace BaddieCompanion::WorldReadPrivate;
        if (!ReadRecord(Object, *DomainSchema(EDomain::SavedCharacter).Record)) return false;
        if (!ReadText(Object, EDomain::SavedCharacter, 0, Out.Id)
            || !ReadText(Object, EDomain::SavedCharacter, 1, Out.Name)) return false;

        const auto Appearance = ObjectField(Object, TEXT("appearance"));
        if (!ReadRecord(Appearance, *DomainSchema(EDomain::Appearance).Record)) return false;
        if (!ReadText(Appearance, EDomain::Appearance, 0, Out.Look)) return false;

        // Legacy appearances omit baseId; the existing renderer uses Bestie.
        Out.BaseId = TEXT("bestie");
        if (Appearance->HasField(TEXT("baseId"))
            && !ReadText(Appearance, EDomain::Appearance, 1, Out.BaseId)) return false;

        return WriteObject(Appearance, Out.AppearanceJson);
    }

    bool ReadRoom(const TSharedPtr<FJsonObject>& Object, BaddieCompanion::FRoomSelection& Out)
    {
        using namespace BaddieCompanion::WorldReadPrivate;
        if (!ReadRecord(Object, *DomainSchema(EDomain::SavedRoom).Record)) return false;
        if (!ReadText(Object, EDomain::SavedRoom, 0, Out.Id)
            || !ReadText(Object, EDomain::SavedRoom, 1, Out.Name)
            || !ReadText(Object, EDomain::SavedRoom, 2, Out.EnvironmentId)) return false;
        if (Object->HasField(TEXT("layoutId"))
            && !ReadText(Object, EDomain::SavedRoom, 3, Out.LayoutId)) return false;
        return WriteObject(Object, Out.RoomJson);
    }
}

bool BaddieCompanion::ReadSnapshotSelection(
    const FString& Json, FSnapshotSelection& OutSelection, FString& OutError)
{
    // All errors are fixed text. Never echo the snapshot or authentication material.
    OutError = TEXT("The companion snapshot is too large or empty.");
    if (Json.IsEmpty() || Json.Len() > MaxSnapshotBytes
        || FTCHARToUTF8(*Json, Json.Len()).Length() > MaxSnapshotBytes) return false;

    TSharedPtr<FJsonObject> Root;
    const auto Reader = TJsonReaderFactory<>::Create(Json);
    OutError = TEXT("The companion snapshot is not a JSON object.");
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid()) return false;

    const auto Library = ObjectField(Root, TEXT("library"));
    OutError = TEXT("The companion library is missing or uses an unsupported version.");
    double Version = 0;
    if (!ReadNumber(Library, TEXT("version"), Version)
        || !WorldReadPrivate::DomainSchema(WorldReadPrivate::EDomain::Library).Record->Fields[0].Number(Version)) return false;

    double Revision = 0;
    OutError = TEXT("The companion snapshot has an invalid revision.");
    if (!ReadNumber(Root, TEXT("revision"), Revision)
        || !WorldReadPrivate::DomainSchema(WorldReadPrivate::EDomain::Selection).Record->Fields[0].Number(Revision)) return false;
    const int64 IntegerRevision = static_cast<int64>(Revision);
    if (static_cast<double>(IntegerRevision) != Revision) return false;

    FString CharacterId, ActiveRoomId, HomeRoomId;
    OutError = TEXT("The companion snapshot has an invalid saved selection.");
    if (!ReadText(Library, WorldReadPrivate::EDomain::Library, 1, CharacterId)
        || !ReadText(Library, WorldReadPrivate::EDomain::Library, 2, ActiveRoomId)
        || !ReadText(Library, WorldReadPrivate::EDomain::Library, 3, HomeRoomId)) return false;

    const TArray<TSharedPtr<FJsonValue>>* Characters = nullptr;
    const TArray<TSharedPtr<FJsonValue>>* Rooms = nullptr;
    OutError = TEXT("The companion library must contain between one and twelve characters and rooms.");
    if (!ReadArray(Library, 4, Characters) || !ReadArray(Library, 5, Rooms)) return false;

    FSnapshotSelection Next;
    Next.Revision = IntegerRevision;
    // Saved IDs follow JavaScript's case-sensitive equality, including duplicate detection.
    TArray<FString> CharacterIds, RoomIds;
    OutError = TEXT("The companion library contains an invalid or duplicate character.");
    for (const auto& Value : *Characters)
    {
        FCharacterSelection Character;
        if (!Value.IsValid() || Value->Type != EJson::Object
            || !ReadCharacter(Value->AsObject(), Character) || ContainsId(CharacterIds, Character.Id)) return false;
        CharacterIds.Add(Character.Id);
        if (SameText(Character.Id, CharacterId)) Next.Character = MoveTemp(Character);
    }

    OutError = TEXT("The companion library contains an invalid or duplicate room.");
    for (const auto& Value : *Rooms)
    {
        FRoomSelection Room;
        if (!Value.IsValid() || Value->Type != EJson::Object
            || !ReadRoom(Value->AsObject(), Room) || ContainsId(RoomIds, Room.Id)) return false;
        RoomIds.Add(Room.Id);
        if (SameText(Room.Id, ActiveRoomId)) Next.ActiveRoom = Room;
        if (SameText(Room.Id, HomeRoomId)) Next.HomeRoom = Room;
        Next.Rooms.Add(MoveTemp(Room));
    }

    OutError = TEXT("The selected companion or room is absent from the saved library.");
    if (!ContainsId(CharacterIds, CharacterId) || !ContainsId(RoomIds, ActiveRoomId) || !ContainsId(RoomIds, HomeRoomId)) return false;

    OutSelection = MoveTemp(Next);
    OutError.Reset();
    return true;
}
