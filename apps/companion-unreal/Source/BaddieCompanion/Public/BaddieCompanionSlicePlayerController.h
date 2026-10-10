#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BaddieCompanionHomePresentation.h"
#include "BaddieCompanionSnapshot.h"
#include "BaddieCompanionSlicePlayerController.generated.h"

class SBaddieCompanionConnectionPanel;
class SWidget;
class UBaddieCompanionSession;
class UBaddieCompanionSliceSession;

enum class EBaddieConnectionAction : uint8 { Connect, Cancel, CopyLink, Disconnect, Repair, ResumeInspection };

UCLASS(NotBlueprintable)
class BADDIECOMPANION_API ABaddieCompanionSlicePlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaSeconds) override;
    // Client-only controls. No origin, approval token, proof or configuration accepted from UI.
    void RequestConnectionAction(EBaddieConnectionAction Action, const FString& DeviceName = FString());
    void ConfirmConnectionAction();
    void CancelConnectionAction();
    void HandleConnectionEscape();
    void RefreshConnectionPresentation();
    // Rechecks the live snapshot grant; clears Out on failure. Never grants gameplay access.
    bool CopyHomePresentation(BaddieCompanion::FHomePresentationPlan& Out);
    bool IsInspectionPointerCaptured() const { return bPointerCaptured; }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void OnPossess(APawn* InPawn) override;
    virtual void SetupInputComponent() override;

private:
    void TogglePointerCapture();
    void ApplyPointerCapture();
    void ClearPrivatePresentation();
    void ConnectionChanged();
    void OfflineSelectionChanged();
    void StartPairing(const FString& DeviceName);
    void StopConnection();
    bool HasOfflineSelection() const;

    TWeakObjectPtr<UBaddieCompanionSession> ConnectionSession;
    TWeakObjectPtr<UBaddieCompanionSliceSession> OfflineSession;
    FDelegateHandle ConnectionHandle;
    FDelegateHandle OfflineHandle;
    TSharedPtr<SBaddieCompanionConnectionPanel> ConnectionPanel;
    TSharedPtr<SWidget> ViewportPanel;
    // Process-memory only; preserve IDs and Home/active-room data without serialization or logging.
    TOptional<BaddieCompanion::FSnapshotSelection> RetainedSelection;
    BaddieCompanion::FHomePresentationCache HomePresentation;
    TOptional<EBaddieConnectionAction> PendingAction;
    FString PendingDeviceName;
    bool bOfflineInspection = false;
    bool bPointerCaptured = false;
    bool bEndingPlay = false;
};
