#include "BaddieCompanionSlicePlayerController.h"

#include "BaddieCompanionSession.h"
#include "BaddieCompanionSliceCharacter.h"
#include "BaddieCompanionSliceSession.h"
#include "SBaddieCompanionConnectionPanel.h"
#include "Components/InputComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerCameraManager.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Widgets/SWeakWidget.h"

namespace
{
    bool ValidDeviceName(const FString& DeviceName)
    {
        const FString Name = DeviceName.TrimStartAndEnd();
        if (Name.IsEmpty() || Name.Len() > 80) return false;
        for (const TCHAR Ch : Name) if (Ch < 32 || Ch == 127) return false;
        return true;
    }
}

void ABaddieCompanionSlicePlayerController::BeginPlay()
{
    Super::BeginPlay();
    if (!IsLocalController()) return;
    if (PlayerCameraManager)
    {
        PlayerCameraManager->ViewPitchMin = -65;
        PlayerCameraManager->ViewPitchMax = -15;
    }
    bOfflineInspection = UBaddieCompanionSliceSession::IsOfflinePreviewAllowed(GetWorld());
    ConnectionSession = GetGameInstance()->GetSubsystem<UBaddieCompanionSession>();
    OfflineSession = GetGameInstance()->GetSubsystem<UBaddieCompanionSliceSession>();
    if (auto* Session = ConnectionSession.Get())
        ConnectionHandle = Session->OnConnectionChanged.AddUObject(this,
            &ABaddieCompanionSlicePlayerController::ConnectionChanged);
    if (auto* Session = OfflineSession.Get())
    {
        OfflineHandle = Session->OnSelectionChanged.AddUObject(this,
            &ABaddieCompanionSlicePlayerController::OfflineSelectionChanged);
        if (!bOfflineInspection) Session->ClearSelection();
    }
    if (bOfflineInspection)
        if (auto* Session = ConnectionSession.Get()) Session->Disconnect();
    if (auto* Viewport = GetWorld()->GetGameViewport())
    {
        SAssignNew(ConnectionPanel, SBaddieCompanionConnectionPanel).Owner(this);
        ViewportPanel = SNew(SWeakWidget).PossiblyNullContent(ConnectionPanel);
        Viewport->AddViewportWidgetContent(ViewportPanel.ToSharedRef(), 10);
    }
    RefreshConnectionPresentation();
    ApplyPointerCapture();
}

void ABaddieCompanionSlicePlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    if (IsLocalController())
    {
        if (UBaddieCompanionSliceSession::IsOfflinePreviewAllowed(GetWorld()))
            SetControlRotation(FRotator(-30, -90, 0));
        ApplyPointerCapture();
    }
}

void ABaddieCompanionSlicePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bEndingPlay = true;
    if (auto* Session = ConnectionSession.Get()) Session->OnConnectionChanged.Remove(ConnectionHandle);
    if (auto* Session = OfflineSession.Get()) Session->OnSelectionChanged.Remove(OfflineHandle);
    ConnectionHandle.Reset();
    OfflineHandle.Reset();
    ClearPrivatePresentation();
    if (auto* Session = ConnectionSession.Get()) Session->Disconnect();
    if (auto* Session = OfflineSession.Get()) Session->ClearSelection();
    if (ViewportPanel.IsValid())
        if (auto* World = GetWorld())
            if (auto* Viewport = World->GetGameViewport())
                Viewport->RemoveViewportWidgetContent(ViewportPanel.ToSharedRef());
    if (ConnectionPanel.IsValid()) ConnectionPanel->DetachOwner();
    ViewportPanel.Reset();
    ConnectionPanel.Reset();
    ConnectionSession.Reset();
    OfflineSession.Reset();
    Super::EndPlay(EndPlayReason);
}

void ABaddieCompanionSlicePlayerController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (IsLocalController() && !bEndingPlay) RefreshConnectionPresentation();
}

void ABaddieCompanionSlicePlayerController::ConnectionChanged()
{
    if (bEndingPlay || !IsLocalController()) return;
    // Offline inspection is isolated even if trusted setup changes while its world is open.
    if (bOfflineInspection)
    {
        auto* Session = ConnectionSession.Get();
        if (Session && Session->GetConnectionView().State != BaddieCompanion::EConnectionState::SignedOut
            && Session->GetConnectionView().State != BaddieCompanion::EConnectionState::Disabled)
            Session->Disconnect();
    }
    else if (auto* Session = ConnectionSession.Get())
    {
        const auto State = Session->GetConnectionView().State;
        using BaddieCompanion::EConnectionState;
        if (State == EConnectionState::RequestingCode || State == EConnectionState::SignedOut
            || State == EConnectionState::Disabled || State == EConnectionState::Expired
            || State == EConnectionState::Unavailable)
            ClearPrivatePresentation();
    }
    RefreshConnectionPresentation();
}

void ABaddieCompanionSlicePlayerController::OfflineSelectionChanged()
{
    if (bEndingPlay || !bOfflineInspection) return;
    // Notification order must not disable a newly spawned inspection pawn.
    RetainedSelection.Reset();
    PendingAction.Reset();
    PendingDeviceName.Reset();
    bPointerCaptured = false;
    if (ConnectionPanel.IsValid()) ConnectionPanel->ClearPrivatePresentation();
    if (!HasOfflineSelection())
        if (auto* Character = Cast<ABaddieCompanionSliceCharacter>(GetPawn()))
            Character->ClearInspectionPresentation();
    ApplyPointerCapture();
    RefreshConnectionPresentation();
}

bool ABaddieCompanionSlicePlayerController::HasOfflineSelection() const
{
    const auto* Session = OfflineSession.Get();
    return bOfflineInspection && UBaddieCompanionSliceSession::IsOfflinePreviewAllowed(GetWorld())
        && Session && Session->GetSelection();
}

void ABaddieCompanionSlicePlayerController::RefreshConnectionPresentation()
{
    if (bEndingPlay || !IsLocalController()) return;
    if (bOfflineInspection)
    {
        // Never reached as a fallback for failed live access.
        const auto* Session = OfflineSession.Get();
        const auto* Selection = HasOfflineSelection() ? Session->GetSelection() : nullptr;
        if (!Selection && RetainedSelection.IsSet()) ClearPrivatePresentation();
        if (Selection) RetainedSelection = *Selection;
        if (ConnectionPanel.IsValid())
            ConnectionPanel->PresentOffline(Selection, Session ? Session->GetStatus() : FString(), bPointerCaptured);
        return;
    }
    auto* Session = ConnectionSession.Get();
    const auto View = Session ? Session->GetConnectionView() : BaddieCompanion::FConnectionView();
    BaddieCompanion::FSnapshotSelection Next;
    // Sole validity authority, checked between notifications and immediately before UI actions.
    const bool bValid = Session && Session->CopySnapshot(Next);
    if (!bValid)
    {
        if (RetainedSelection.IsSet() || PendingAction.IsSet()) ClearPrivatePresentation();
    }
    else
    {
        if (RetainedSelection.IsSet()
            && (!RetainedSelection.GetValue().Character.Id.Equals(Next.Character.Id, ESearchCase::CaseSensitive)
                || !RetainedSelection.GetValue().HomeRoom.Id.Equals(Next.HomeRoom.Id, ESearchCase::CaseSensitive)
                || !RetainedSelection.GetValue().ActiveRoom.Id.Equals(Next.ActiveRoom.Id, ESearchCase::CaseSensitive)))
            ClearPrivatePresentation();
        RetainedSelection = MoveTemp(Next);
    }
    if (ConnectionPanel.IsValid())
        ConnectionPanel->PresentConnection(View, RetainedSelection.IsSet() ? &RetainedSelection.GetValue() : nullptr);
}

void ABaddieCompanionSlicePlayerController::ClearPrivatePresentation()
{
    RetainedSelection.Reset();
    PendingAction.Reset();
    PendingDeviceName.Reset();
    bPointerCaptured = false;
    if (ConnectionPanel.IsValid()) ConnectionPanel->ClearPrivatePresentation();
    if (auto* Character = Cast<ABaddieCompanionSliceCharacter>(GetPawn()))
        Character->ClearInspectionPresentation();
    SetControlRotation(FRotator::ZeroRotator);
    if (!bEndingPlay) ApplyPointerCapture();
}

void ABaddieCompanionSlicePlayerController::StartPairing(const FString& DeviceName)
{
    // Clear before a synchronous notification or request from a new pairing.
    ClearPrivatePresentation();
    if (auto* Session = ConnectionSession.Get()) Session->BeginPairing(DeviceName);
    RefreshConnectionPresentation();
}

void ABaddieCompanionSlicePlayerController::StopConnection()
{
    ClearPrivatePresentation();
    if (auto* Session = ConnectionSession.Get()) Session->Disconnect();
    RefreshConnectionPresentation();
}

void ABaddieCompanionSlicePlayerController::RequestConnectionAction(
    EBaddieConnectionAction Action, const FString& DeviceName)
{
    RefreshConnectionPresentation();
    if (bEndingPlay || !IsLocalController()) return;
    if (Action == EBaddieConnectionAction::ResumeInspection)
    {
        if (HasOfflineSelection())
        {
            bPointerCaptured = true;
            ApplyPointerCapture();
            RefreshConnectionPresentation();
        }
        return;
    }
    if (bOfflineInspection) return;
    auto* Session = ConnectionSession.Get();
    if (!Session || PendingAction.IsSet()) return;
    const auto View = Session->GetConnectionView();
    using BaddieCompanion::EConnectionState;
    const bool bPending = View.State == EConnectionState::RequestingCode
        || View.State == EConnectionState::AwaitingApproval || View.State == EConnectionState::LoadingHome;
    const bool bConnected = View.State == EConnectionState::Ready && RetainedSelection.IsSet();
    if (Action == EBaddieConnectionAction::CopyLink)
    {
        if (View.State == EConnectionState::AwaitingApproval && View.RemainingSeconds > 0 && !View.ApprovalLink.IsEmpty())
            FPlatformApplicationMisc::ClipboardCopy(*View.ApprovalLink);
        return;
    }
    if (Action == EBaddieConnectionAction::Cancel && bPending) { StopConnection(); return; }
    if ((Action == EBaddieConnectionAction::Disconnect || Action == EBaddieConnectionAction::Repair) && bConnected)
    {
        PendingAction = Action;
        PendingDeviceName = View.DeviceName;
        if (ConnectionPanel.IsValid()) ConnectionPanel->ShowConfirmation(Action);
        return;
    }
    if (Action == EBaddieConnectionAction::Connect && ValidDeviceName(DeviceName) && !bPending && !bConnected
        && View.State != EConnectionState::Disabled && View.RetryAfterSeconds <= 0)
        StartPairing(DeviceName);
}

void ABaddieCompanionSlicePlayerController::ConfirmConnectionAction()
{
    // A locally expired/configuration-changed grant dismisses the modal before this action.
    RefreshConnectionPresentation();
    if (!PendingAction.IsSet()) return;
    const auto Action = PendingAction.GetValue();
    const FString Name = PendingDeviceName;
    if (Action == EBaddieConnectionAction::Repair) StartPairing(Name);
    else StopConnection();
}

void ABaddieCompanionSlicePlayerController::CancelConnectionAction()
{
    PendingAction.Reset();
    PendingDeviceName.Reset();
    if (ConnectionPanel.IsValid()) ConnectionPanel->DismissConfirmation();
}

void ABaddieCompanionSlicePlayerController::HandleConnectionEscape()
{
    if (PendingAction.IsSet()) { CancelConnectionAction(); return; }
    if (bOfflineInspection) { TogglePointerCapture(); return; }
    if (auto* Session = ConnectionSession.Get())
    {
        const auto View = Session->GetConnectionView();
        if (View.State == BaddieCompanion::EConnectionState::RequestingCode
            || View.State == BaddieCompanion::EConnectionState::AwaitingApproval
            || View.State == BaddieCompanion::EConnectionState::LoadingHome)
            StopConnection();
    }
}

void ABaddieCompanionSlicePlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindAction(TEXT("ReleasePointer"), IE_Pressed,
        this, &ABaddieCompanionSlicePlayerController::HandleConnectionEscape);
}

void ABaddieCompanionSlicePlayerController::TogglePointerCapture()
{
    bPointerCaptured = HasOfflineSelection() && !bPointerCaptured;
    ApplyPointerCapture();
    RefreshConnectionPresentation();
}

void ABaddieCompanionSlicePlayerController::ApplyPointerCapture()
{
    bPointerCaptured = bPointerCaptured && HasOfflineSelection();
    bShowMouseCursor = !bPointerCaptured;
    ResetIgnoreMoveInput();
    ResetIgnoreLookInput();
    SetIgnoreMoveInput(!bPointerCaptured);
    SetIgnoreLookInput(!bPointerCaptured);
    if (!bPointerCaptured)
        if (auto* Character = Cast<ACharacter>(GetPawn()))
        {
            Character->ConsumeMovementInputVector();
            Character->GetCharacterMovement()->StopMovementImmediately();
        }
    if (bPointerCaptured)
    {
        FInputModeGameOnly Mode;
        Mode.SetConsumeCaptureMouseDown(false);
        SetInputMode(Mode);
    }
    else
    {
        FInputModeUIOnly Mode;
        Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        if (ConnectionPanel.IsValid()) Mode.SetWidgetToFocus(ConnectionPanel);
        SetInputMode(Mode);
    }
}
