#include "BaddieCompanionSliceSession.h"

#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"

void UBaddieCompanionSliceSession::Deinitialize()
{
    ClearSelection();
    OnSelectionChanged.Clear();
    Super::Deinitialize();
}

BaddieCompanion::FOfflineSnapshotRequest UBaddieCompanionSliceSession::BeginOfflineRead()
{
    // Every replacement is a new scope. A failed replacement cannot retain a prior file's data.
    Generation = FGuid::NewGuid();
    ++Sequence;
    Selection.Reset();
    Status = TEXT("Loading offline snapshot.");
    State = BaddieCompanion::EOfflineSelectionState::Loading;
    const BaddieCompanion::FOfflineSnapshotRequest Request{ Generation, Sequence };
    OnSelectionChanged.Broadcast();
    return Request;
}

bool UBaddieCompanionSliceSession::IsCurrentRequest(
    const BaddieCompanion::FOfflineSnapshotRequest& Request) const
{
    return State == BaddieCompanion::EOfflineSelectionState::Loading
        && Request.Generation.IsValid() && Request.Generation == Generation
        && Request.Sequence == Sequence;
}

bool UBaddieCompanionSliceSession::CompleteOfflineRead(
    const BaddieCompanion::FOfflineSnapshotRequest& Request, const FString& Json)
{
    if (!IsCurrentRequest(Request)) return false;

    BaddieCompanion::FSnapshotSelection Next;
    FString Error;
    if (!BaddieCompanion::ReadSnapshotSelection(Json, Next, Error))
    {
        SetUnavailable(Error);
        return false;
    }

    Selection = MoveTemp(Next);
    State = BaddieCompanion::EOfflineSelectionState::Ready;
    Status.Reset();
    OnSelectionChanged.Broadcast();
    return true;
}

void UBaddieCompanionSliceSession::FailOfflineRead(
    const BaddieCompanion::FOfflineSnapshotRequest& Request)
{
    if (IsCurrentRequest(Request))
        SetUnavailable(TEXT("The offline snapshot could not be read. Check the file and reopen the preview."));
}

bool UBaddieCompanionSliceSession::LoadOfflineSnapshot(const FString& Path)
{
#if UE_BUILD_SHIPPING
    ClearSelection();
    SetUnavailable(TEXT("Offline snapshot inspection is not available in this build."));
    return false;
#else
    const auto Request = BeginOfflineRead();
    const int64 Size = IFileManager::Get().FileSize(*Path);
    if (Size <= 0 || Size > BaddieCompanion::MaxSnapshotBytes)
    {
        if (IsCurrentRequest(Request))
            SetUnavailable(TEXT("Use a non-empty offline snapshot file no larger than 256 KiB."));
        return false;
    }

    FString Json;
    if (!FFileHelper::LoadFileToString(Json, *Path))
    {
        FailOfflineRead(Request);
        return false;
    }
    return CompleteOfflineRead(Request, Json);
#endif
}

void UBaddieCompanionSliceSession::SetUnavailable(const FString& Message)
{
    Selection.Reset();
    State = BaddieCompanion::EOfflineSelectionState::Unavailable;
    Status = Message;
    OnSelectionChanged.Broadcast();
}

void UBaddieCompanionSliceSession::ClearSelection()
{
    Generation = FGuid::NewGuid();
    ++Sequence;
    Selection.Reset();
    State = BaddieCompanion::EOfflineSelectionState::Empty;
    Status.Reset();
    OnSelectionChanged.Broadcast();
}

const BaddieCompanion::FSnapshotSelection* UBaddieCompanionSliceSession::GetSelection() const
{
    return State == BaddieCompanion::EOfflineSelectionState::Ready && Selection.IsSet()
        ? &Selection.GetValue() : nullptr;
}
