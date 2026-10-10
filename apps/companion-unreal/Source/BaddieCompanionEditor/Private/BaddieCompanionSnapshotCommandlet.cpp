#include "BaddieCompanionSnapshotCommandlet.h"

#include "BaddieCompanionSnapshot.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"

DEFINE_LOG_CATEGORY_STATIC(LogBaddieSnapshot, Log, All);

UBaddieCompanionSnapshotCommandlet::UBaddieCompanionSnapshotCommandlet(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    IsClient = false;
    IsServer = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UBaddieCompanionSnapshotCommandlet::Main(const FString& Params)
{
    FString SnapshotPath;
    if (!FParse::Value(*Params, TEXT("Snapshot="), SnapshotPath) || SnapshotPath.IsEmpty())
    {
        UE_LOG(LogBaddieSnapshot, Error, TEXT("Supply -Snapshot=<local JSON file>. Never pass credentials."));
        return 1;
    }

    const int64 Size = IFileManager::Get().FileSize(*SnapshotPath);
    if (Size <= 0 || Size > BaddieCompanion::MaxSnapshotBytes)
    {
        UE_LOG(LogBaddieSnapshot, Error, TEXT("The snapshot file is missing, empty or larger than 256 KiB."));
        return 1;
    }

    FString Json;
    if (!FFileHelper::LoadFileToString(Json, *SnapshotPath))
    {
        UE_LOG(LogBaddieSnapshot, Error, TEXT("The snapshot file could not be read."));
        return 1;
    }

    BaddieCompanion::FSnapshotSelection Selection;
    FString Error;
    if (!BaddieCompanion::ReadSnapshotSelection(Json, Selection, Error))
    {
        UE_LOG(LogBaddieSnapshot, Error, TEXT("%s"), *Error);
        return 1;
    }

    UE_LOG(LogBaddieSnapshot, Display, TEXT("Companion selection decoded. Revision=%lld. Active room is Home=%s."),
        static_cast<long long>(Selection.Revision),
        Selection.ActiveRoom.Id.Equals(Selection.HomeRoom.Id, ESearchCase::CaseSensitive) ? TEXT("true") : TEXT("false"));
    return 0;
}
