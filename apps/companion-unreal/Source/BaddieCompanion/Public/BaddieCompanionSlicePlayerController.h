#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BaddieCompanionHomePresentation.h"
#include "BaddieCompanionSession.h"
#include "BaddieCompanionSnapshot.h"
#include "BaddieCompanionNativeApartment.h"
#include "BaddieCompanionSlicePlayerController.generated.h"

class SBaddieCompanionConnectionPanel;
class SWidget;
class UBaddieCompanionSession;
class UBaddieCompanionSliceSession;

enum class EBaddieConnectionAction : uint8
{
    Connect, ConnectApartment, Cancel, CopyLink, Disconnect, Repair, RepairApartment, ResumeInspection
};

UCLASS(NotBlueprintable)
class BADDIECOMPANION_API ABaddieCompanionSlicePlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ABaddieCompanionSlicePlayerController();
    virtual void Tick(float DeltaSeconds) override;
    // Client-only controls. No origin, approval token, proof or configuration accepted from UI.
    void RequestConnectionAction(EBaddieConnectionAction Action, const FString& DeviceName = FString());
    void ConfirmConnectionAction();
    void CancelConnectionAction();
    void HandleConnectionEscape();
    void RefreshConnectionPresentation();
    // Rechecks the live snapshot grant; clears Out on failure. Never grants gameplay access.
    bool CopyHomePresentation(BaddieCompanion::FHomePresentationPlan& Out);
    bool CopyNativeApartmentStatus(BaddieCompanion::FNativeApartmentStatus& Out);
    // Trusted native content setup only. Currently blocked by missing presentation/admission.
    bool PrepareNativeApartment(const BaddieCompanion::FNativeApartmentContent& Content);
    bool CopyNativeRoomCamera(const FString& RoomId, BaddieCompanion::FNativeRoomCamera& Out);
    bool CopyNativeDoorwayApproach(int32 ConnectionIndex, bool bFromEndpoint, FVector& Out);
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
    void StartPairing(const FString& DeviceName, BaddieCompanion::EConnectionScope Scope);
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
    UPROPERTY(Transient)
    TObjectPtr<UBaddieCompanionNativeApartment> NativeApartment;
    TOptional<EBaddieConnectionAction> PendingAction;
    FString PendingDeviceName;
    BaddieCompanion::EConnectionScope PendingScope = BaddieCompanion::EConnectionScope::Home;
    bool bOfflineInspection = false;
    bool bPointerCaptured = false;
    bool bEndingPlay = false;
};
