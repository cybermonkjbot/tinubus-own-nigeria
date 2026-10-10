#include "BaddieCompanionSlicePlayerController.h"

#include "BaddieCompanionConnectionUi.h"
#include "BaddieCompanionSession.h"
#include "BaddieCompanionSliceCharacter.h"
#include "BaddieCompanionSliceSession.h"
#include "BaddieCompanionSliceHUD.h"
#include "SBaddieCompanionConnectionPanel.h"
#include "Components/InputComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "HAL/PlatformApplicationMisc.h"
#include "HAL/PlatformTime.h"
#include "Misc/CoreDelegates.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWeakWidget.h"

ABaddieCompanionSlicePlayerController::ABaddieCompanionSlicePlayerController()
{
    NativeApartment = CreateDefaultSubobject<UBaddieCompanionNativeApartment>(TEXT("NativeApartment"));
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
    MissionDeactivateHandle = FCoreDelegates::ApplicationWillDeactivateDelegate.AddUObject(this,
        &ABaddieCompanionSlicePlayerController::HandleMissionFocusLoss);
    MissionBackgroundHandle = FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddUObject(this,
        &ABaddieCompanionSlicePlayerController::HandleMissionFocusLoss);
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
    ClearMissionPresentation();
    Super::OnPossess(InPawn);
    if (IsLocalController())
    {
        if (UBaddieCompanionSliceSession::IsOfflinePreviewAllowed(GetWorld()))
            SetControlRotation(FRotator(-30, -90, 0));
        ApplyPointerCapture();
    }
}

void ABaddieCompanionSlicePlayerController::OnUnPossess()
{
    ClearMissionPresentation();
    Super::OnUnPossess();
}

void ABaddieCompanionSlicePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bEndingPlay = true;
    FCoreDelegates::ApplicationWillDeactivateDelegate.Remove(MissionDeactivateHandle);
    FCoreDelegates::ApplicationWillEnterBackgroundDelegate.Remove(MissionBackgroundHandle);
    ClearMissionPresentation();
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
    if (IsLocalController() && !bEndingPlay) RefreshMissionPresentation();
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
    ClearMissionPresentation();
    // Notification order must not disable a newly spawned inspection pawn.
    RetainedSelection.Reset();
    HomePresentation.Clear();
    if (NativeApartment) NativeApartment->ClearPrivateContent();
    PendingAction.Reset();
    PendingDeviceName.Reset();
    PendingScope = BaddieCompanion::EConnectionScope::Home;
    bPointerCaptured = false;
    if (ConnectionPanel.IsValid()) ConnectionPanel->ClearPrivatePresentation();
    if (!HasOfflineSelection())
        if (auto* ControlledCharacter = Cast<ABaddieCompanionSliceCharacter>(GetPawn()))
            ControlledCharacter->ClearInspectionPresentation();
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
        HomePresentation.Clear();
        if (NativeApartment && NativeApartment->HasPrivateContent()) NativeApartment->ClearPrivateContent();
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
        if (RetainedSelection.IsSet() || PendingAction.IsSet() || HomePresentation.HasInput()
            || (NativeApartment && NativeApartment->HasPrivateContent())) ClearPrivatePresentation();
    }
    else
    {
        // Same revision/IDs can still carry replacement room, design or appearance documents.
        if (NativeApartment && NativeApartment->HasPrivateContent() && !HomePresentation.Matches(Next))
            NativeApartment->ClearPrivateContent();
        if (RetainedSelection.IsSet()
            && (!RetainedSelection.GetValue().Character.Id.Equals(Next.Character.Id, ESearchCase::CaseSensitive)
                || !RetainedSelection.GetValue().HomeRoom.Id.Equals(Next.HomeRoom.Id, ESearchCase::CaseSensitive)
                || !RetainedSelection.GetValue().ActiveRoom.Id.Equals(Next.ActiveRoom.Id, ESearchCase::CaseSensitive)))
            ClearPrivatePresentation();
        RetainedSelection = MoveTemp(Next);
        HomePresentation.Update(RetainedSelection.GetValue());
        if (NativeApartment)
        {
            if (const auto* Plan = HomePresentation.GetPlan())
            {
                NativeApartment->ObserveValidPresentation(*Plan);
                NativeApartment->PollNativeLevels();
            }
            else NativeApartment->ClearPrivateContent();
        }
    }
    if (ConnectionPanel.IsValid() && !MissionToken.IsSet())
        ConnectionPanel->PresentConnection(View, RetainedSelection.IsSet() ? &RetainedSelection.GetValue() : nullptr,
            HomePresentation.GetPlan());
}

bool ABaddieCompanionSlicePlayerController::CopyHomePresentation(BaddieCompanion::FHomePresentationPlan& Out)
{
    Out = {};
    if (bEndingPlay || !IsLocalController() || bOfflineInspection)
    {
        HomePresentation.Clear();
        if (NativeApartment) NativeApartment->ClearPrivateContent();
        return false;
    }
    RefreshConnectionPresentation();
    return HomePresentation.CopyPlan(Out);
}

void ABaddieCompanionSlicePlayerController::ClearPrivatePresentation()
{
    ClearMissionPresentation();
    if (NativeApartment) NativeApartment->ClearPrivateContent();
    RetainedSelection.Reset();
    HomePresentation.Clear();
    PendingAction.Reset();
    PendingDeviceName.Reset();
    PendingScope = BaddieCompanion::EConnectionScope::Home;
    bPointerCaptured = false;
    if (ConnectionPanel.IsValid()) ConnectionPanel->ClearPrivatePresentation();
    if (auto* ControlledCharacter = Cast<ABaddieCompanionSliceCharacter>(GetPawn()))
        ControlledCharacter->ClearInspectionPresentation();
    SetControlRotation(FRotator::ZeroRotator);
    if (!bEndingPlay) ApplyPointerCapture();
}

bool ABaddieCompanionSlicePlayerController::CopyNativeApartmentStatus(BaddieCompanion::FNativeApartmentStatus& Out)
{
    Out = {};
    RefreshConnectionPresentation();
    if (bEndingPlay || !IsLocalController() || bOfflineInspection || !HomePresentation.GetPlan() || !NativeApartment)
        return false;
    Out = NativeApartment->GetStatus();
    return true;
}

bool ABaddieCompanionSlicePlayerController::PrepareNativeApartment(const BaddieCompanion::FNativeApartmentContent& Content)
{
    RefreshConnectionPresentation();
    if (bEndingPlay || !IsLocalController() || bOfflineInspection || !NativeApartment) return false;
    const auto* Plan = HomePresentation.GetPlan();
    if (!Plan) { NativeApartment->ClearPrivateContent(); return false; }
    return NativeApartment->PrepareContent(*Plan, Content);
}

bool ABaddieCompanionSlicePlayerController::CopyNativeRoomCamera(const FString& RoomId,
    BaddieCompanion::FNativeRoomCamera& Out)
{
    Out = {};
    RefreshConnectionPresentation();
    return !bEndingPlay && IsLocalController() && !bOfflineInspection && HomePresentation.GetPlan()
        && NativeApartment && NativeApartment->CopyRoomCamera(RoomId, Out);
}

bool ABaddieCompanionSlicePlayerController::CopyNativeDoorwayApproach(int32 Index, bool bFromEndpoint, FVector& Out)
{
    Out = FVector::ZeroVector;
    RefreshConnectionPresentation();
    auto* ControlledCharacter = Cast<ACharacter>(GetPawn());
    // Never use the inspection pawn's neutral-stage clamp/spawn in native apartment traversal.
    return !bEndingPlay && IsLocalController() && !bOfflineInspection && HomePresentation.GetPlan()
        && ControlledCharacter && !ControlledCharacter->IsA<ABaddieCompanionSliceCharacter>() && NativeApartment
        && NativeApartment->CopyNavigationApproach(*ControlledCharacter, Index, bFromEndpoint, Out);
}

void ABaddieCompanionSlicePlayerController::StartPairing(
    const FString& DeviceName, BaddieCompanion::EConnectionScope Scope)
{
    // Clear before a synchronous notification or request from a new pairing.
    ClearPrivatePresentation();
    if (auto* Session = ConnectionSession.Get()) Session->BeginPairing(DeviceName, Scope);
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
    // Existing account controls cannot share a pointer or a pending mission action.
    ClearMissionPresentation();
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
    const bool bPending = BaddieCompanion::IsConnectionPending(View.State);
    const bool bConnected = View.State == EConnectionState::Ready && RetainedSelection.IsSet();
    if (Action == EBaddieConnectionAction::CopyLink)
    {
        if (View.State == EConnectionState::AwaitingApproval && View.RemainingSeconds > 0 && !View.ApprovalLink.IsEmpty())
            FPlatformApplicationMisc::ClipboardCopy(*View.ApprovalLink);
        return;
    }
    if (Action == EBaddieConnectionAction::Cancel && bPending) { StopConnection(); return; }
    if ((Action == EBaddieConnectionAction::Disconnect || Action == EBaddieConnectionAction::Repair
        || Action == EBaddieConnectionAction::RepairApartment) && bConnected)
    {
        PendingAction = Action;
        PendingDeviceName = View.DeviceName;
        PendingScope = Action == EBaddieConnectionAction::RepairApartment
            ? BaddieCompanion::EConnectionScope::Apartment : View.Scope;
        if (ConnectionPanel.IsValid()) ConnectionPanel->ShowConfirmation(Action, PendingScope);
        return;
    }
    if ((Action == EBaddieConnectionAction::Connect || Action == EBaddieConnectionAction::ConnectApartment)
        && BaddieCompanion::IsValidConnectionDeviceName(DeviceName)
        && !bPending && !bConnected
        && View.State != EConnectionState::Disabled && View.RetryAfterSeconds <= 0)
        StartPairing(DeviceName, Action == EBaddieConnectionAction::ConnectApartment
            ? BaddieCompanion::EConnectionScope::Apartment : BaddieCompanion::EConnectionScope::Home);
}

void ABaddieCompanionSlicePlayerController::ConfirmConnectionAction()
{
    // A locally expired/configuration-changed grant dismisses the modal before this action.
    RefreshConnectionPresentation();
    if (!PendingAction.IsSet()) return;
    const auto Action = PendingAction.GetValue();
    const FString Name = PendingDeviceName;
    const auto Scope = PendingScope;
    if (Action == EBaddieConnectionAction::Repair || Action == EBaddieConnectionAction::RepairApartment)
        StartPairing(Name, Scope);
    else StopConnection();
}

void ABaddieCompanionSlicePlayerController::CancelConnectionAction()
{
    PendingAction.Reset();
    PendingDeviceName.Reset();
    PendingScope = BaddieCompanion::EConnectionScope::Home;
    if (ConnectionPanel.IsValid()) ConnectionPanel->DismissConfirmation();
}

void ABaddieCompanionSlicePlayerController::HandleConnectionEscape()
{
    if (MissionToken.IsSet()) { SetMissionModal(!bMissionModal); return; }
    if (PendingAction.IsSet()) { CancelConnectionAction(); return; }
    if (bOfflineInspection) { TogglePointerCapture(); return; }
    if (auto* Session = ConnectionSession.Get())
    {
        const auto View = Session->GetConnectionView();
        if (BaddieCompanion::IsConnectionPending(View.State))
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
    if (MissionToken.IsSet())
    {
        // Slate owns all expedition pointers/keys, forwarding only admitted native delegates.
        // Do not also deliver these events to a pawn's ordinary input bindings.
        bPointerCaptured = false;
        bShowMouseCursor = true;
        ResetIgnoreMoveInput();
        ResetIgnoreLookInput();
        // UI-only routing blocks raw pawn input. Native delegate adapters can still use
        // Character Movement and controller rotation without bypassing ignore-input flags.
        FInputModeUIOnly Mode;
        Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        if (auto* Hud = Cast<ABaddieCompanionSliceHUD>(GetHUD()))
            if (const auto Focus = Hud->GetMissionFocusWidget()) Mode.SetWidgetToFocus(Focus);
        SetInputMode(Mode);
        return;
    }
    bPointerCaptured = bPointerCaptured && HasOfflineSelection();
    bShowMouseCursor = !bPointerCaptured;
    ResetIgnoreMoveInput();
    ResetIgnoreLookInput();
    SetIgnoreMoveInput(!bPointerCaptured);
    SetIgnoreLookInput(!bPointerCaptured);
    if (!bPointerCaptured)
        if (auto* ControlledCharacter = Cast<ACharacter>(GetPawn()))
        {
            ControlledCharacter->ConsumeMovementInputVector();
            ControlledCharacter->GetCharacterMovement()->StopMovementImmediately();
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

bool ABaddieCompanionSlicePlayerController::CheckMissionAdmission()
{
    if (bEndingPlay || bClearingMission || !IsLocalController() || bOfflineInspection
        || UBaddieCompanionSliceSession::IsOfflinePreviewAllowed(GetWorld())
        || !GetPawn() || GetPawn()->IsA<ABaddieCompanionSliceCharacter>()
        || PendingAction.IsSet() || !MissionAdmission.IsBound()
        || !FSlateApplication::IsInitialized() || !FSlateApplication::Get().IsActive()) return false;
    const auto* Hud = Cast<ABaddieCompanionSliceHUD>(GetHUD());
    if (!Hud || !Hud->HasMissionFocus()) return false;
    BaddieCompanion::FMissionIdentity Identity;
    const auto Admission = MissionAdmission;
    if (!Admission.Execute(Identity) || !Identity.IsValid()) return false;
    return MissionToken.IsSet() && MissionToken->Identity == Identity;
}

bool ABaddieCompanionSlicePlayerController::AcceptMissionToken(
    const BaddieCompanion::FMissionPresentationToken& Token)
{
    if (bCancellingMissionInput) return false;
    if (!MissionToken.IsSet() || !(Token == MissionToken.GetValue())) return false;
    if (!CheckMissionAdmission()) { ClearMissionPresentation(); return false; }
    return !bMissionModal;
}

bool ABaddieCompanionSlicePlayerController::CopyMissionToken(BaddieCompanion::FMissionPresentationToken& Out)
{
    Out = {};
    if (!CheckMissionAdmission() || bMissionModal) return false;
    Out = MissionToken.GetValue();
    return true;
}

void ABaddieCompanionSlicePlayerController::RefreshMissionPresentation()
{
    if (bRefreshingMission || bClearingMission || bEndingPlay || !IsLocalController()) return;
    TGuardValue<bool> RefreshGuard(bRefreshingMission, true);
    auto* Hud = Cast<ABaddieCompanionSliceHUD>(GetHUD());
    if (!Hud) { ClearMissionPresentation(); return; }

    if (MissionToken.IsSet())
    {
        if (!CheckMissionAdmission() || !Hud->HasMissionFocus())
        {
            ClearMissionPresentation();
            return;
        }
    }
    else
    {
        // Closed until the separate admitted gameplay owner binds this callback.
        if (bOfflineInspection || UBaddieCompanionSliceSession::IsOfflinePreviewAllowed(GetWorld())
            || PendingAction.IsSet() || !GetPawn() || GetPawn()->IsA<ABaddieCompanionSliceCharacter>()
            || !MissionAdmission.IsBound() || !FSlateApplication::IsInitialized()
            || !FSlateApplication::Get().IsActive()) return;
        BaddieCompanion::FMissionIdentity Identity;
        const auto Admission = MissionAdmission;
        if (!Admission.Execute(Identity) || !Identity.IsValid()
            || Identity.Generation <= RetiredMissionGeneration) return;
        BaddieCompanion::FMissionPresentationToken Token;
        Token.Identity = MoveTemp(Identity);
        Token.Presentation = ++MissionPresentationGeneration;
        MissionToken = MoveTemp(Token);
        if (PlayerCameraManager)
        {
            SavedMissionPitchMin = PlayerCameraManager->ViewPitchMin;
            SavedMissionPitchMax = PlayerCameraManager->ViewPitchMax;
            bOwnsMissionPitchLimits = true;
            PlayerCameraManager->ViewPitchMin = -89;
            PlayerCameraManager->ViewPitchMax = 89;
        }
        if (ConnectionPanel.IsValid()) ConnectionPanel->SetVisibility(EVisibility::Collapsed);
        BaddieCompanion::FMissionHudView Initial;
        Initial.Token = MissionToken.GetValue();
        if (!Hud->PresentMission(Initial, GetControlRotation().Yaw)) { ClearMissionPresentation(); return; }
        ApplyPointerCapture();
    }

    const auto Token = MissionToken.GetValue();
    if (MissionSelection.IsSet() && MissionSelection->ValidUntilSeconds <= FPlatformTime::Seconds())
        InvalidateMissionAction();
    if (!MissionToken.IsSet() || !(Token == MissionToken.GetValue())) return;
    if (!HasMissionCombatPolicy())
    {
        StopMissionFire();
        bManualFireHeld = false;
        RequestMissionADS(Token, false);
        MissionCombat = {};
    }
    if (!MissionToken.IsSet() || !(Token == MissionToken.GetValue())) return;
    UpdateMissionFire();
    if (!MissionToken.IsSet() || !(Token == MissionToken.GetValue())) return;
    BaddieCompanion::FMissionHudView View;
    View.Token = Token;
    View.ActionState = MissionActionState;
    if (MissionSelection.IsSet())
    {
        View.Action = MissionSelection->Action;
        View.TargetId = MissionSelection->TargetId;
        View.TargetRevision = MissionSelection->Revision;
    }
    View.bCanAct = MissionSelection.IsSet() && !MissionRequest.IsSet() && MissionActionInput.IsBound()
        && MissionActionCancel.IsBound() && !bMissionModal;
    View.bCanFire = HasMissionCombatPolicy() && MissionFireInput.IsBound()
        && MissionCombat.bManualFireAvailable && MissionCombat.bWeaponRaised && MissionCombat.bFiringStance
        && !MissionSelection.IsSet() && !bMissionModal;
    View.bCanADS = HasMissionCombatPolicy() && MissionADSInput.IsBound() && MissionCombat.bADSAvailable
        && !MissionSelection.IsSet() && !bMissionModal;
    View.bModal = bMissionModal;
    View.bHipAssistance = bHipAssistance;
    Hud->PresentMission(View, GetControlRotation().Yaw);
}

void ABaddieCompanionSlicePlayerController::ClearMissionPresentation()
{
    if (bClearingMission || !MissionToken.IsSet()) return;
    TGuardValue<bool> ClearGuard(bClearingMission, true);
    RetiredMissionGeneration = FMath::Max(RetiredMissionGeneration, MissionToken->Identity.Generation);
    MissionToken.Reset(); // Retire before native callbacks can re-enter.
    CancelMissionInputs();
    InvalidateMissionAction();
    CompletedMissionSelection.Reset();
    MissionCombat = {};
    MissionActionSequence = MissionCombatSequence = MissionHearingSequence = 0;
    bMissionModal = false;
    if (bOwnsMissionPitchLimits && PlayerCameraManager)
    {
        PlayerCameraManager->ViewPitchMin = SavedMissionPitchMin;
        PlayerCameraManager->ViewPitchMax = SavedMissionPitchMax;
    }
    bOwnsMissionPitchLimits = false;
    if (auto* Hud = Cast<ABaddieCompanionSliceHUD>(GetHUD())) Hud->ClearMission();
    if (ConnectionPanel.IsValid()) ConnectionPanel->SetVisibility(EVisibility::Visible);
    if (!bEndingPlay) ApplyPointerCapture();
}

void ABaddieCompanionSlicePlayerController::HandleMissionFocusLoss()
{
    ClearMissionPresentation();
}

void ABaddieCompanionSlicePlayerController::InvalidateMissionAction()
{
    const auto Request = MissionRequest;
    const auto Cancel = PendingCancelDelegate;
    MissionRequest.Reset();
    PendingCancelDelegate.Unbind();
    MissionSelection.Reset();
    MissionActionState = BaddieCompanion::EMissionActionState::Hidden;
    if (Request.IsSet()) Cancel.ExecuteIfBound(Request.GetValue());
}

void ABaddieCompanionSlicePlayerController::PresentMissionAction(
    const BaddieCompanion::FMissionPresentationToken& Token, uint64 Sequence,
    const TOptional<BaddieCompanion::FMissionSelectedAction>& Selection)
{
    if (!AcceptMissionToken(Token) || Sequence <= MissionActionSequence) return;
    MissionActionSequence = Sequence;
    const bool bValid = Selection.IsSet() && !Selection->TargetId.IsEmpty()
        && Selection->bAcceptedReachable && FMath::IsFinite(Selection->ValidUntilSeconds)
        && Selection->ValidUntilSeconds > FPlatformTime::Seconds()
        && (Selection->Action == BaddieCompanion::EMissionAction::ClearBrush
            || Selection->Action == BaddieCompanion::EMissionAction::Untie);
    if (!bValid) { InvalidateMissionAction(); return; }
    if (CompletedMissionSelection.IsSet() && CompletedMissionSelection->SameTarget(Selection.GetValue()))
    {
        InvalidateMissionAction();
        return;
    }
    if (MissionSelection.IsSet() && MissionSelection->SameTarget(Selection.GetValue()))
    {
        MissionSelection = Selection; // Preserve pending/failure for this exact selected revision.
        return;
    }
    InvalidateMissionAction();
    CancelMissionInputs();
    if (!AcceptMissionToken(Token) || Sequence != MissionActionSequence) return;
    MissionSelection = Selection;
    MissionActionState = BaddieCompanion::EMissionActionState::Available;
}

void ABaddieCompanionSlicePlayerController::RequestMissionAction(
    const BaddieCompanion::FMissionPresentationToken& Token,
    const BaddieCompanion::FMissionSelectedAction& Expected, bool bBrushSwipe)
{
    if (!AcceptMissionToken(Token) || !MissionSelection.IsSet() || MissionRequest.IsSet()
        || !MissionSelection->SameTarget(Expected)
        || !MissionActionInput.IsBound() || !MissionActionCancel.IsBound()) return;
    if (MissionSelection->ValidUntilSeconds <= FPlatformTime::Seconds()) { InvalidateMissionAction(); return; }
    if (bBrushSwipe && MissionSelection->Action != BaddieCompanion::EMissionAction::ClearBrush) return;
    CancelMissionInputs();
    if (!AcceptMissionToken(Token) || !MissionSelection.IsSet() || MissionRequest.IsSet()
        || !MissionSelection->SameTarget(Expected) || MissionSelection->ValidUntilSeconds <= FPlatformTime::Seconds()) return;
    BaddieCompanion::FMissionActionRequest Request;
    Request.Token = Token;
    Request.Selection = MissionSelection.GetValue();
    Request.RequestId = ++MissionRequestSequence;
    MissionRequest = Request;
    PendingCancelDelegate = MissionActionCancel;
    MissionActionState = BaddieCompanion::EMissionActionState::Pending;
    const auto Submit = MissionActionInput;
    if (!Submit.Execute(Request)) CompleteMissionAction(Request, false);
}

void ABaddieCompanionSlicePlayerController::CompleteMissionAction(
    const BaddieCompanion::FMissionActionRequest& Request, bool bSucceeded)
{
    if (!AcceptMissionToken(Request.Token) || !MissionRequest.IsSet()
        || MissionRequest->RequestId != Request.RequestId || !MissionSelection.IsSet()
        || !MissionSelection->SameTarget(Request.Selection)) return;
    if (MissionSelection->ValidUntilSeconds <= FPlatformTime::Seconds()) { InvalidateMissionAction(); return; }
    MissionRequest.Reset();
    PendingCancelDelegate.Unbind();
    if (bSucceeded)
    {
        // Only authority can change the obstacle, captive, mission state or reward.
        CompletedMissionSelection = MissionSelection;
        MissionSelection.Reset();
        MissionActionState = BaddieCompanion::EMissionActionState::Hidden;
    }
    else MissionActionState = BaddieCompanion::EMissionActionState::Failed;
}

bool ABaddieCompanionSlicePlayerController::HasMissionCombatPolicy() const
{
    return FMath::IsFinite(MissionCombat.ValidUntilSeconds)
        && MissionCombat.ValidUntilSeconds > FPlatformTime::Seconds() && !MissionCombat.WeaponId.IsEmpty();
}

void ABaddieCompanionSlicePlayerController::PresentMissionCombat(
    const BaddieCompanion::FMissionPresentationToken& Token, uint64 Sequence,
    const BaddieCompanion::FMissionCombatPolicy& Policy)
{
    if (!AcceptMissionToken(Token) || Sequence <= MissionCombatSequence) return;
    MissionCombatSequence = Sequence;
    const bool bChangedTarget = !MissionCombat.TargetId.Equals(Policy.TargetId, ESearchCase::CaseSensitive)
        || !MissionCombat.WeaponId.Equals(Policy.WeaponId, ESearchCase::CaseSensitive);
    MissionCombat = Policy;
    if (bChangedTarget) StopMissionFire();
    if (!AcceptMissionToken(Token) || Sequence != MissionCombatSequence) return;
    if (!HasMissionCombatPolicy() || !Policy.bFiringStance || !Policy.bWeaponRaised)
    {
        // Lowered-weapon traversal removes combat ownership, not the movement/look gesture.
        bManualFireHeld = false;
        StopMissionFire();
        if (!AcceptMissionToken(Token) || Sequence != MissionCombatSequence) return;
        RequestMissionADS(Token, false);
    }
    else if (!Policy.bADSAvailable) RequestMissionADS(Token, false);
    if (AcceptMissionToken(Token) && Sequence == MissionCombatSequence) UpdateMissionFire();
}

bool ABaddieCompanionSlicePlayerController::PresentMissionHeardCue(
    const BaddieCompanion::FMissionPresentationToken& Token, uint64 Sequence,
    uint8 WorldSector, double ExpiresAtSeconds)
{
    if (!AcceptMissionToken(Token) || Sequence <= MissionHearingSequence) return false;
    MissionHearingSequence = Sequence;
    const double Remaining = ExpiresAtSeconds - FPlatformTime::Seconds();
    if (WorldSector >= 8 || !FMath::IsFinite(Remaining) || Remaining <= 0) return false;
    auto* Hud = Cast<ABaddieCompanionSliceHUD>(GetHUD());
    return Hud && Hud->AddMissionHeardCue(Token.Presentation, WorldSector, Remaining);
}

void ABaddieCompanionSlicePlayerController::RequestMissionMove(
    const BaddieCompanion::FMissionPresentationToken& Token, FVector2D Value)
{
    if (!AcceptMissionToken(Token) || Value.ContainsNaN() || !MissionMoveInput.IsBound()) return;
    HeldInputToken = Token;
    HeldMoveDelegate = MissionMoveInput;
    const auto Move = HeldMoveDelegate;
    Move.Execute(Token, Value.GetClampedToMaxSize(1.0));
}

void ABaddieCompanionSlicePlayerController::RequestMissionLook(
    const BaddieCompanion::FMissionPresentationToken& Token, FVector2D Delta)
{
    if (!AcceptMissionToken(Token) || Delta.ContainsNaN()) return;
    const auto Look = MissionLookInput;
    Look.ExecuteIfBound(Token, Delta);
}

void ABaddieCompanionSlicePlayerController::RequestMissionADS(
    const BaddieCompanion::FMissionPresentationToken& Token, bool bHeld)
{
    if (!bHeld)
    {
        if (!HeldInputToken.IsSet() || !(Token == HeldInputToken.GetValue()) || !bMissionADSHeld) return;
        ++MissionADSInputSerial;
        bMissionADSHeld = false;
        const auto Release = HeldADSDelegate;
        HeldADSDelegate.Unbind();
        if (Release.IsBound()) Release.Execute(Token, false);
        return;
    }
    if (!AcceptMissionToken(Token) || bMissionADSHeld || MissionSelection.IsSet()
        || !HasMissionCombatPolicy() || !MissionCombat.bADSAvailable || !MissionADSInput.IsBound()) return;
    const uint64 InputSerial = ++MissionADSInputSerial;
    const uint64 ActionSequence = MissionActionSequence;
    const uint64 PolicySequence = MissionCombatSequence;
    HeldInputToken = Token;
    bMissionADSHeld = true; // Suppress assistance before a synchronous fire-release callback.
    bManualFireHeld = false;
    StopMissionFire();
    const auto StillOwnsPress = [this, &Token, InputSerial]()
    {
        return InputSerial == MissionADSInputSerial && bMissionADSHeld
            && HeldInputToken.IsSet() && Token == HeldInputToken.GetValue();
    };
    // Fire release may synchronously cancel input or present an action without changing Token.
    if (!StillOwnsPress() || !AcceptMissionToken(Token) || MissionSelection.IsSet()
        || ActionSequence != MissionActionSequence || PolicySequence != MissionCombatSequence
        || !HasMissionCombatPolicy() || !MissionCombat.bADSAvailable || !MissionADSInput.IsBound())
    {
        if (StillOwnsPress()) { bMissionADSHeld = false; HeldADSDelegate.Unbind(); }
        return;
    }
    HeldADSDelegate = MissionADSInput;
    const auto ADS = HeldADSDelegate;
    if (!ADS.Execute(Token, true) && StillOwnsPress())
    {
        bMissionADSHeld = false;
        HeldADSDelegate.Unbind();
    }
}

void ABaddieCompanionSlicePlayerController::RequestMissionFire(
    const BaddieCompanion::FMissionPresentationToken& Token, bool bHeld)
{
    if (!bHeld)
    {
        if (!HeldInputToken.IsSet() || !(Token == HeldInputToken.GetValue())) return;
        bManualFireHeld = false;
        StopMissionFire();
        return;
    }
    if (!AcceptMissionToken(Token) || MissionSelection.IsSet() || !HasMissionCombatPolicy()
        || !MissionCombat.bManualFireAvailable || !MissionCombat.bWeaponRaised || !MissionCombat.bFiringStance) return;
    bManualFireHeld = true;
    UpdateMissionFire();
}

void ABaddieCompanionSlicePlayerController::StopMissionFire()
{
    const auto Source = HeldFireSource;
    const auto Token = HeldInputToken;
    const auto Fire = HeldFireDelegate;
    HeldFireSource.Reset();
    HeldFireDelegate.Unbind();
    if (Source.IsSet() && Token.IsSet() && Fire.IsBound()) Fire.Execute(Token.GetValue(), false, Source.GetValue());
}

void ABaddieCompanionSlicePlayerController::UpdateMissionFire()
{
    if (bUpdatingMissionFire || bCancellingMissionInput || !MissionToken.IsSet()) return;
    TGuardValue<bool> FireGuard(bUpdatingMissionFire, true);
    const auto Token = MissionToken.GetValue();
    const bool bAvailable = AcceptMissionToken(Token) && !MissionSelection.IsSet()
        && HasMissionCombatPolicy() && MissionFireInput.IsBound() && MissionCombat.bManualFireAvailable
        && MissionCombat.bWeaponRaised && MissionCombat.bFiringStance;
    const bool bAssist = bAvailable && bHipAssistance && !bMissionADSHeld && !MissionCombat.bADS
        && !MissionCombat.TargetId.IsEmpty() && MissionCombat.bVisible && MissionCombat.bEligibleHostile
        && MissionCombat.bUnderCrosshair && MissionCombat.bWithinWeaponCloseRange && MissionCombat.bWeaponAllowsAssistance;
    if (!bAvailable || (!bManualFireHeld && !bAssist)) { StopMissionFire(); return; }
    const auto Source = bManualFireHeld ? BaddieCompanion::EMissionFireSource::Manual
        : BaddieCompanion::EMissionFireSource::HipAssistance;
    if (HeldFireSource.IsSet() && HeldFireSource.GetValue() == Source) return;
    const uint64 PolicySequence = MissionCombatSequence;
    StopMissionFire();
    if (!AcceptMissionToken(Token) || PolicySequence != MissionCombatSequence || !MissionFireInput.IsBound()
        || (Source == BaddieCompanion::EMissionFireSource::Manual) != bManualFireHeld) return;
    HeldInputToken = Token;
    HeldFireSource = Source;
    HeldFireDelegate = MissionFireInput;
    const auto Fire = HeldFireDelegate;
    if (!Fire.Execute(Token, true, Source) && HeldInputToken.IsSet() && Token == HeldInputToken.GetValue()
        && HeldFireSource.IsSet() && PolicySequence == MissionCombatSequence)
    {
        HeldFireSource.Reset();
        HeldFireDelegate.Unbind();
        bManualFireHeld = false;
        // Do not retry an unavailable input every tick. A fresh combat policy is required.
        MissionCombat = {};
    }
}

void ABaddieCompanionSlicePlayerController::CancelMissionInputs()
{
    if (bCancellingMissionInput) return;
    TGuardValue<bool> CancelGuard(bCancellingMissionInput, true);
    ++MissionADSInputSerial;
    const auto Token = HeldInputToken;
    const auto Move = HeldMoveDelegate;
    const auto ADS = HeldADSDelegate;
    const bool bReleaseADS = bMissionADSHeld;
    bManualFireHeld = false;
    bMissionADSHeld = false;
    HeldMoveDelegate.Unbind();
    HeldADSDelegate.Unbind();
    StopMissionFire();
    HeldInputToken.Reset();
    if (auto* Hud = Cast<ABaddieCompanionSliceHUD>(GetHUD())) Hud->CancelMissionPointers();
    if (Token.IsSet())
    {
        Move.ExecuteIfBound(Token.GetValue(), FVector2D::ZeroVector);
        if (bReleaseADS && ADS.IsBound()) ADS.Execute(Token.GetValue(), false);
    }
}

void ABaddieCompanionSlicePlayerController::SetMissionModal(bool bOpen)
{
    if (!CheckMissionAdmission() || bMissionModal == bOpen) return;
    bMissionModal = bOpen;
    // Rotate before cancellation, so callbacks captured by controls or owners cannot reopen them.
    MissionToken->Presentation = ++MissionPresentationGeneration;
    CancelMissionInputs();
    InvalidateMissionAction();
    CompletedMissionSelection.Reset();
    MissionCombat = {};
    MissionActionSequence = MissionCombatSequence = MissionHearingSequence = 0;
    RefreshMissionPresentation();
    ApplyPointerCapture();
}

void ABaddieCompanionSlicePlayerController::SetMissionHipAssistance(bool bEnabled)
{
    if (!CheckMissionAdmission() || !bMissionModal) return;
    bHipAssistance = bEnabled;
    CancelMissionInputs();
}
