#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BaddieCompanionHomePresentation.h"
#include "BaddieCompanionSession.h"
#include "BaddieCompanionGameplaySession.h"
#include "BaddieCompanionSnapshot.h"
#include "BaddieCompanionNativeApartment.h"
#include "BaddieCompanionMissionPresentation.h"
#include "BaddieCompanionWorldPresentation.h"
#include "BaddieCompanionSlicePlayerController.generated.h"

class SBaddieCompanionConnectionPanel;
class SWidget;
class UBaddieCompanionSession;
class UBaddieCompanionSliceSession;
class ABaddieCompanionWorldCharacter;

enum class EBaddieConnectionAction : uint8
{
    Connect, ConnectApartment, Cancel, CopyLink, Disconnect, Repair, RepairApartment, ResumeInspection
};

enum class EBaddieGameplayConnectionAction : uint8
{
    Pair, PairWorld, ReloadWorld, CopyLink, CancelPairing, Enter, CancelEntry, Leave, Disconnect
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
    void RequestGameplayConnectionAction(EBaddieGameplayConnectionAction Action, uint64 ExpectedRevision,
        const FString& DeviceName = FString());
    void ConfirmConnectionAction();
    void CancelConnectionAction();
    void HandleConnectionEscape();
    void RefreshConnectionPresentation();
    // Rechecks the live snapshot grant; clears Out on failure. Never grants gameplay access.
    bool CopyHomePresentation(BaddieCompanion::FHomePresentationPlan& Out);
    bool CopyNativeApartmentStatus(BaddieCompanion::FNativeApartmentStatus& Out);
    // Trusted native content setup only. Currently blocked by missing presentation/admission.
    bool PrepareNativeApartment(const BaddieCompanion::FNativeApartmentContent& Content);
    bool PrepareWorldContent(const BaddieCompanion::FNativeWorldContent& Content);
    bool RequestWorldWalk(const BaddieCompanion::FMovementPoint& Target, BaddieCompanion::EMovementPace Pace);
    void StopWorldWalk();
    bool CopyNativeRoomCamera(const FString& RoomId, BaddieCompanion::FNativeRoomCamera& Out);
    bool CopyNativeDoorwayApproach(int32 ConnectionIndex, bool bFromEndpoint, FVector& Out);
    bool IsInspectionPointerCaptured() const { return bPointerCaptured; }

    // Trusted native integration only. Bind before admission; never from Home connection or UI.
    BaddieCompanion::FMissionAdmission MissionAdmission;
    BaddieCompanion::FMissionVectorInput MissionMoveInput;
    BaddieCompanion::FMissionVectorInput MissionLookInput;
    BaddieCompanion::FMissionADSInput MissionADSInput;
    BaddieCompanion::FMissionFireInput MissionFireInput;
    BaddieCompanion::FMissionActionInput MissionActionInput;
    BaddieCompanion::FMissionActionCancel MissionActionCancel;

    bool CopyMissionToken(BaddieCompanion::FMissionPresentationToken& Out);
    void RefreshMissionPresentation();
    void ClearMissionPresentation();
    // Monotonic sequence per presentation. Null selection explicitly invalidates the current target.
    void PresentMissionAction(const BaddieCompanion::FMissionPresentationToken& Token, uint64 Sequence,
        const TOptional<BaddieCompanion::FMissionSelectedAction>& Selection);
    void PresentMissionCombat(const BaddieCompanion::FMissionPresentationToken& Token, uint64 Sequence,
        const BaddieCompanion::FMissionCombatPolicy& Policy);
    // Hearing owner has already admitted this event. Only eight world-bearing sectors are accepted.
    bool PresentMissionHeardCue(const BaddieCompanion::FMissionPresentationToken& Token, uint64 Sequence,
        uint8 WorldSector, double ExpiresAtSeconds);
    void CompleteMissionAction(const BaddieCompanion::FMissionActionRequest& Request, bool bSucceeded);
    void RequestMissionAction(const BaddieCompanion::FMissionPresentationToken& Token,
        const BaddieCompanion::FMissionSelectedAction& Expected, bool bBrushSwipe = false);
    void RequestMissionMove(const BaddieCompanion::FMissionPresentationToken& Token, FVector2D Value);
    void RequestMissionLook(const BaddieCompanion::FMissionPresentationToken& Token, FVector2D Delta);
    void RequestMissionADS(const BaddieCompanion::FMissionPresentationToken& Token, bool bHeld);
    void RequestMissionFire(const BaddieCompanion::FMissionPresentationToken& Token, bool bHeld);
    void CancelMissionInputs();
    void SetMissionModal(bool bOpen);
    void SetMissionHipAssistance(bool bEnabled);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;
    virtual void SetupInputComponent() override;

private:
    friend class UBaddieCompanionGameplaySession;
    friend class UBaddieCompanionWorldPresentation;
    void TogglePointerCapture();
    void ApplyPointerCapture();
    void ClearPrivatePresentation();
    void ClearReadPresentation();
    void RefreshWorldPresentation();
    void ClearWorldPresentation();
    void WorldNativeContentLost(BaddieCompanion::FWorldObservationToken Token);
    void ConnectionChanged();
    void GameplayConnectionChanged();
    void InvalidateGameplayControls();
    bool HasConnectionUiFocus() const;
    void OfflineSelectionChanged();
    void StartPairing(const FString& DeviceName, BaddieCompanion::EConnectionScope Scope);
    void StopConnection();
    bool HasOfflineSelection() const;
    bool CheckMissionAdmission();
    bool AcceptMissionToken(const BaddieCompanion::FMissionPresentationToken& Token);
    void InvalidateMissionAction();
    void UpdateMissionFire();
    void StopMissionFire();
    bool HasMissionCombatPolicy() const;
    void HandleMissionFocusLoss();

    TWeakObjectPtr<UBaddieCompanionSession> ConnectionSession;
    TWeakObjectPtr<UBaddieCompanionGameplaySession> GameplaySession;
    TWeakObjectPtr<UBaddieCompanionSliceSession> OfflineSession;
    FDelegateHandle ConnectionHandle;
    FDelegateHandle GameplayConnectionHandle;
    FDelegateHandle OfflineHandle;
    TSharedPtr<SBaddieCompanionConnectionPanel> ConnectionPanel;
    TSharedPtr<SWidget> ViewportPanel;
    // Process-memory only; preserve IDs and Home/active-room data without serialization or logging.
    TOptional<BaddieCompanion::FSnapshotSelection> RetainedSelection;
    BaddieCompanion::FHomePresentationCache HomePresentation;
    UPROPERTY(Transient)
    TObjectPtr<UBaddieCompanionNativeApartment> NativeApartment;
    UPROPERTY(Transient)
    TObjectPtr<UBaddieCompanionWorldPresentation> WorldPresentation;
    BaddieCompanion::FWorldObservation WorldObservation;
    bool bRefreshingWorld = false, bClearingWorld = false;
    TOptional<EBaddieConnectionAction> PendingAction;
    TOptional<EBaddieGameplayConnectionAction> PendingGameplayAction;
    TOptional<BaddieCompanion::FGameplayConnectionView> LastGameplayView;
    uint64 GameplayUiRevision = 1;
    uint64 PendingGameplayRevision = 0;
    FString PendingDeviceName;
    BaddieCompanion::EConnectionScope PendingScope = BaddieCompanion::EConnectionScope::Home;
    bool bOfflineInspection = false;
    bool bPointerCaptured = false;
    bool bEndingPlay = false;
    bool bRefreshingConnection = false;
    bool bHadConnectionUiFocus = false;
    TOptional<BaddieCompanion::FMissionPresentationToken> MissionToken;
    TOptional<BaddieCompanion::FMissionSelectedAction> MissionSelection;
    TOptional<BaddieCompanion::FMissionSelectedAction> CompletedMissionSelection;
    TOptional<BaddieCompanion::FMissionActionRequest> MissionRequest;
    BaddieCompanion::FMissionCombatPolicy MissionCombat;
    BaddieCompanion::EMissionActionState MissionActionState = BaddieCompanion::EMissionActionState::Hidden;
    uint64 RetiredMissionGeneration = 0;
    uint64 MissionPresentationGeneration = 0;
    uint64 MissionActionSequence = 0;
    uint64 MissionCombatSequence = 0;
    uint64 MissionHearingSequence = 0;
    uint64 MissionRequestSequence = 0;
    uint64 MissionADSInputSerial = 0;
    bool bMissionModal = false;
    bool bHipAssistance = false;
    bool bManualFireHeld = false;
    bool bMissionADSHeld = false;
    bool bRefreshingMission = false;
    bool bClearingMission = false;
    bool bCancellingMissionInput = false;
    bool bUpdatingMissionFire = false;
    bool bOwnsMissionPitchLimits = false;
    float SavedMissionPitchMin = 0;
    float SavedMissionPitchMax = 0;
    TOptional<BaddieCompanion::FMissionPresentationToken> HeldInputToken;
    TOptional<BaddieCompanion::EMissionFireSource> HeldFireSource;
    BaddieCompanion::FMissionFireInput HeldFireDelegate;
    BaddieCompanion::FMissionADSInput HeldADSDelegate;
    BaddieCompanion::FMissionVectorInput HeldMoveDelegate;
    BaddieCompanion::FMissionActionCancel PendingCancelDelegate;
    FDelegateHandle MissionDeactivateHandle;
    FDelegateHandle MissionBackgroundHandle;
};
