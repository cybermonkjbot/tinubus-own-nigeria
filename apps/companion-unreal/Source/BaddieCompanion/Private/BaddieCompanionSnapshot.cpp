#include "BaddieCompanionSnapshot.h"

#include "Containers/StringConv.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
    constexpr int32 LibraryLimit = 12;
    constexpr double MaxSafeRevision = 9007199254740991.0;

    TSharedPtr<FJsonObject> ObjectField(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key)
    {
        const TSharedPtr<FJsonObject>* Value = nullptr;
        return Object.IsValid() && Object->TryGetObjectField(Key, Value) && Value
            ? *Value : nullptr;
    }

    bool ReadText(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key, int32 Limit, FString& Out)
    {
        const auto Value = Object.IsValid() ? Object->TryGetField(Key) : nullptr;
        return Value.IsValid() && Value->Type == EJson::String && Value->TryGetString(Out)
            && Out.Len() <= Limit && !Out.TrimStartAndEnd().IsEmpty();
    }

    bool ReadNumber(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key, double& Out)
    {
        const auto Value = Object.IsValid() ? Object->TryGetField(Key) : nullptr;
        return Value.IsValid() && Value->Type == EJson::Number && Value->TryGetNumber(Out);
    }

    bool ReadArray(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key,
        const TArray<TSharedPtr<FJsonValue>>*& Out)
    {
        return Object.IsValid() && Object->TryGetArrayField(Key, Out) && Out
            && Out->Num() > 0 && Out->Num() <= LibraryLimit;
    }

    bool WriteObject(const TSharedPtr<FJsonObject>& Object, FString& Out)
    {
        const auto Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Out);
        return FJsonSerializer::Serialize(Object.ToSharedRef(), Writer);
    }

    bool ReadCharacter(const TSharedPtr<FJsonObject>& Object, BaddieCompanion::FCharacterSelection& Out)
    {
        if (!ReadText(Object, TEXT("id"), 80, Out.Id)
            || !ReadText(Object, TEXT("name"), 20, Out.Name)) return false;

        const auto Appearance = ObjectField(Object, TEXT("appearance"));
        if (!ReadText(Appearance, TEXT("look"), 16, Out.Look)
            || (Out.Look != TEXT("rose") && Out.Look != TEXT("ivory"))) return false;

        // Legacy appearances omit baseId; the existing renderer uses Bestie.
        Out.BaseId = TEXT("bestie");
        if (Appearance->HasField(TEXT("baseId"))
            && !ReadText(Appearance, TEXT("baseId"), 16, Out.BaseId)) return false;
        if (Out.BaseId != TEXT("bestie") && Out.BaseId != TEXT("bunny")
            && Out.BaseId != TEXT("bear") && Out.BaseId != TEXT("cat")
            && Out.BaseId != TEXT("fox") && Out.BaseId != TEXT("panda")) return false;

        return WriteObject(Appearance, Out.AppearanceJson);
    }

    bool ReadRoom(const TSharedPtr<FJsonObject>& Object, BaddieCompanion::FRoomSelection& Out)
    {
        if (!ReadText(Object, TEXT("id"), 80, Out.Id)
            || !ReadText(Object, TEXT("name"), 32, Out.Name)
            || !ReadText(Object, TEXT("environmentId"), 16, Out.EnvironmentId)
            || (Out.EnvironmentId != TEXT("studio") && Out.EnvironmentId != TEXT("terrace"))) return false;
        if (Object->HasField(TEXT("layoutId"))
            && !ReadText(Object, TEXT("layoutId"), 80, Out.LayoutId)) return false;
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
    if (!ReadNumber(Library, TEXT("version"), Version) || Version != 1) return false;

    double Revision = 0;
    OutError = TEXT("The companion snapshot has an invalid revision.");
    if (!ReadNumber(Root, TEXT("revision"), Revision) || !FMath::IsFinite(Revision)
        || Revision < 0 || Revision > MaxSafeRevision) return false;
    const int64 IntegerRevision = static_cast<int64>(Revision);
    if (static_cast<double>(IntegerRevision) != Revision) return false;

    FString CharacterId, ActiveRoomId, HomeRoomId;
    OutError = TEXT("The companion snapshot has an invalid saved selection.");
    if (!ReadText(Library, TEXT("activeCharacterId"), 80, CharacterId)
        || !ReadText(Library, TEXT("activeRoomId"), 80, ActiveRoomId)
        || !ReadText(Library, TEXT("homeRoomId"), 80, HomeRoomId)) return false;

    const TArray<TSharedPtr<FJsonValue>>* Characters = nullptr;
    const TArray<TSharedPtr<FJsonValue>>* Rooms = nullptr;
    OutError = TEXT("The companion library must contain between one and twelve characters and rooms.");
    if (!ReadArray(Library, TEXT("characters"), Characters) || !ReadArray(Library, TEXT("rooms"), Rooms)) return false;

    FSnapshotSelection Next;
    Next.Revision = IntegerRevision;
    TSet<FString> CharacterIds, RoomIds;
    OutError = TEXT("The companion library contains an invalid or duplicate character.");
    for (const auto& Value : *Characters)
    {
        FCharacterSelection Character;
        if (!Value.IsValid() || Value->Type != EJson::Object
            || !ReadCharacter(Value->AsObject(), Character) || CharacterIds.Contains(Character.Id)) return false;
        CharacterIds.Add(Character.Id);
        if (Character.Id == CharacterId) Next.Character = MoveTemp(Character);
    }

    OutError = TEXT("The companion library contains an invalid or duplicate room.");
    for (const auto& Value : *Rooms)
    {
        FRoomSelection Room;
        if (!Value.IsValid() || Value->Type != EJson::Object
            || !ReadRoom(Value->AsObject(), Room) || RoomIds.Contains(Room.Id)) return false;
        RoomIds.Add(Room.Id);
        if (Room.Id == ActiveRoomId) Next.ActiveRoom = Room;
        if (Room.Id == HomeRoomId) Next.HomeRoom = Room;
    }

    OutError = TEXT("The selected companion or room is absent from the saved library.");
    if (!CharacterIds.Contains(CharacterId) || !RoomIds.Contains(ActiveRoomId) || !RoomIds.Contains(HomeRoomId)) return false;

    OutSelection = MoveTemp(Next);
    OutError.Reset();
    return true;
}
