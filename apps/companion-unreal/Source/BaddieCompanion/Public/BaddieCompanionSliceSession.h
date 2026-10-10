#pragma once

#include "CoreMinimal.h"
#include "BaddieCompanionSnapshot.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BaddieCompanionSliceSession.generated.h"

namespace BaddieCompanion
{
    enum class EOfflineSelectionState : uint8 { Empty, Loading, Ready, Unavailable };

    // An offline request identity, never an account grant or an authentication proof.
    struct FOfflineSnapshotRequest
    {
        FGuid Generation;
        uint64 Sequence = 0;
    };
}

DECLARE_MULTICAST_DELEGATE(FBaddieSliceSelectionChanged);

/** Game-thread-only transient offline inspection data. No credentials, network or SaveGame state. */
UCLASS()
class BADDIECOMPANION_API UBaddieCompanionSliceSession : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Deinitialize() override;
    static bool IsOfflinePreviewAllowed(const UWorld* World);

    BaddieCompanion::FOfflineSnapshotRequest BeginOfflineRead();
    bool CompleteOfflineRead(const BaddieCompanion::FOfflineSnapshotRequest& Request, const FString& Json);
    void FailOfflineRead(const BaddieCompanion::FOfflineSnapshotRequest& Request);
    bool LoadOfflineSnapshot(const FString& Path);
    void ClearSelection();

    BaddieCompanion::EOfflineSelectionState GetState() const { return State; }
    const FString& GetStatus() const { return Status; }
    const BaddieCompanion::FSnapshotSelection* GetSelection() const;
    FBaddieSliceSelectionChanged OnSelectionChanged;

private:
    bool IsCurrentRequest(const BaddieCompanion::FOfflineSnapshotRequest& Request) const;
    void SetUnavailable(const FString& Message);

    // Deliberately not reflected, serialized, exposed to Blueprint or logged.
    FGuid Generation;
    uint64 Sequence = 0;
    BaddieCompanion::EOfflineSelectionState State = BaddieCompanion::EOfflineSelectionState::Empty;
    TOptional<BaddieCompanion::FSnapshotSelection> Selection;
    FString Status;
};
