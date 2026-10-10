#include "SBaddieCompanionConnectionPanel.h"

#include "BaddieCompanionConnectionUi.h"
#include "BaddieCompanionHomePresentation.h"
#include "BaddieCompanionSlicePlayerController.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Text/TextLayout.h"
#include "Fonts/SlateFontInfo.h"
#include "InputCoreTypes.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

using ECompanionConnectionState = BaddieCompanion::EConnectionState;

namespace BaddieConnectionPanelPrivate
{
    // Keep native button behavior, but do not let a held press act on replacement UI state.
    class SRevisionButton : public SButton
    {
    public:
        void Construct(const SButton::FArguments& Args, TAttribute<uint64> InRevision)
        {
            Revision = InRevision;
            SButton::FArguments Guarded = Args;
            Guarded.OnPressed_Lambda([this, Press = Args._OnPressed]()
            {
                PressRevision = Revision.Get();
                Press.ExecuteIfBound();
            });
            Guarded.OnClicked_Lambda([this, Click = Args._OnClicked]()
            {
                // Native accessibility activation can invoke a click without a pointer/key press.
                const bool bCurrent = !PressRevision.IsSet() || PressRevision.GetValue() == Revision.Get();
                PressRevision.Reset();
                return bCurrent && Click.IsBound() ? Click.Execute() : FReply::Handled();
            });
            SButton::Construct(Guarded);
        }

        virtual FReply OnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event) override
        {
            const FReply Reply = SButton::OnMouseButtonUp(Geometry, Event);
            if (!IsPressed()) PressRevision.Reset();
            return Reply;
        }
        virtual FReply OnKeyUp(const FGeometry& Geometry, const FKeyEvent& Event) override
        {
            const FReply Reply = SButton::OnKeyUp(Geometry, Event);
            if (!IsPressed()) PressRevision.Reset();
            return Reply;
        }
        virtual void OnMouseCaptureLost(const FCaptureLostEvent& Event) override
        {
            PressRevision.Reset();
            SButton::OnMouseCaptureLost(Event);
        }
        virtual void OnFocusLost(const FFocusEvent& Event) override
        {
            PressRevision.Reset();
            SButton::OnFocusLost(Event);
        }
    private:
        TAttribute<uint64> Revision;
        TOptional<uint64> PressRevision;
    };
}

void SBaddieCompanionConnectionPanel::Construct(const FArguments& Args)
{
    Owner = Args._Owner;
    TSharedPtr<SVerticalBox> Content;
    ChildSlot
    [
        SNew(SOverlay)
        + SOverlay::Slot()
        [
            SAssignNew(MainSurface, SBorder)
            .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
            .BorderBackgroundColor(FLinearColor(0.025f, 0.025f, 0.035f, 0.96f))
            .Padding(16).HAlign(HAlign_Center)
            [
                SNew(SBox).WidthOverride(this, &SBaddieCompanionConnectionPanel::PanelWidth)
                [
                    SAssignNew(MainScroll, SScrollBox)
                    + SScrollBox::Slot()
                    [ SAssignNew(Content, SVerticalBox) ]
                ]
            ]
        ]
        + SOverlay::Slot()
        [
            SAssignNew(ModalSurface, SBorder)
            .Visibility(EVisibility::Collapsed)
            .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
            .BorderBackgroundColor(FLinearColor(0, 0, 0, 0.85f))
            .Padding(16).HAlign(HAlign_Center).VAlign(VAlign_Center)
            [
                SNew(SBox).WidthOverride(this, &SBaddieCompanionConnectionPanel::PanelWidth)
                .MaxDesiredHeight(this, &SBaddieCompanionConnectionPanel::ModalHeight)
                [
                    SNew(SBorder)
                    .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                    .BorderBackgroundColor(FLinearColor(0.06f, 0.06f, 0.075f, 1))
                    .Padding(16)
                    [
                        SAssignNew(ModalScroll, SScrollBox)
                        + SScrollBox::Slot()
                        [
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)
                            [ MakeText(ModalTitle, 24) ]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 16)
                            [ MakeText(ModalBody) ]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
                            [
                                SAssignNew(KeepButton, BaddieConnectionPanelPrivate::SRevisionButton,
                                    TAttribute<uint64>::CreateLambda([this]() { return ConfirmationRevision; })).ContentPadding(12)
                                .Text(FText::FromString(TEXT("Cancel")))
                                .OnClicked_Lambda([this]()
                                {
                                    if (bModal)
                                        if (auto* Controller = Owner.Get()) Controller->CancelConnectionAction();
                                    return FReply::Handled();
                                })
                            ]
                            + SVerticalBox::Slot().AutoHeight()
                            [
                                SAssignNew(ConfirmButton, BaddieConnectionPanelPrivate::SRevisionButton,
                                    TAttribute<uint64>::CreateLambda([this]() { return ConfirmationRevision; })).ContentPadding(12)
                                .Text(FText::FromString(TEXT("Confirm")))
                                .OnClicked_Lambda([this]()
                                {
                                    if (bModal)
                                        if (auto* Controller = Owner.Get()) Controller->ConfirmConnectionAction();
                                    return FReply::Handled();
                                })
                            ]
                        ]
                    ]
                ]
            ]
        ]
    ];

    const auto AddText = [this, &Content](TSharedPtr<STextBlock>& Target, int32 Size = 18)
    {
        Content->AddSlot().AutoHeight().Padding(0, 0, 0, 12)[MakeText(Target, Size)];
    };
    AddText(Title, 28);
    AddText(StatusText);
    AddText(DeviceText);
    Content->AddSlot().AutoHeight().Padding(0, 0, 0, 12)
    [
        SAssignNew(DeviceName, SEditableTextBox)
        .Text(FText::FromString(TEXT("Baddie on Windows")))
        .HintText(FText::FromString(TEXT("Device name")))
        .IsEnabled_Lambda([this]() { return CanConnect(); })
        .OnTextCommitted_Lambda([this](const FText&, ETextCommit::Type Commit)
        {
            if (Commit == ETextCommit::OnEnter && CanConnect() && ValidDeviceName())
                InvokeAction(EBaddieConnectionAction::Connect);
        })
    ];
    AddText(DeviceNameError);
    AddText(ComparisonCode, 24);
    AddText(ApprovalLink);
    AddText(Lifetime);
    AddText(ErrorText);
    AddText(RetryText);
    // Home first. No separate setup/empty cards or scene presented as a saved Home.
    AddText(HomeText, 24);
    AddText(CompanionText);
    AddText(ActiveRoomText);
    AddText(UnsupportedText);
    const auto AddButton = [this, &Content](TSharedPtr<SButton>& Target, const TCHAR* Label, EBaddieConnectionAction Action)
    {
        Content->AddSlot().AutoHeight().Padding(0, 0, 0, 8)[MakeButton(Target, Label, Action)];
    };
    AddButton(ConnectButton, TEXT("Connect Home only"), EBaddieConnectionAction::Connect);
    ConnectButton->SetEnabled(TAttribute<bool>::CreateLambda([this]() { return CanConnect() && ValidDeviceName(); }));
    AddButton(ApartmentButton, TEXT("Connect apartment"), EBaddieConnectionAction::ConnectApartment);
    ApartmentButton->SetEnabled(TAttribute<bool>::CreateLambda([this]() { return CanConnect() && ValidDeviceName(); }));
    AddButton(CopyButton, TEXT("Copy approval link"), EBaddieConnectionAction::CopyLink);
    AddButton(CancelButton, TEXT("Cancel connection"), EBaddieConnectionAction::Cancel);
    AddButton(DisconnectButton, TEXT("Disconnect"), EBaddieConnectionAction::Disconnect);
    AddButton(RepairButton, TEXT("Pair again"), EBaddieConnectionAction::Repair);
    AddButton(RepairApartmentButton, TEXT("Connect apartment"), EBaddieConnectionAction::RepairApartment);
    AddButton(ResumeButton, TEXT("Resume inspection"), EBaddieConnectionAction::ResumeInspection);
    Controls = { DeviceName, ConnectButton, ApartmentButton, CopyButton, CancelButton,
        DisconnectButton, RepairButton, RepairApartmentButton, ResumeButton };

    AddText(GameplayTitle, 24);
    AddText(GameplayStatus);
    AddText(GameplayLimits);
    AddText(GameplayDeviceText);
    Content->AddSlot().AutoHeight().Padding(0, 0, 0, 12)
    [
        SAssignNew(GameplayDeviceName, SEditableTextBox)
        .Text(FText::FromString(TEXT("Baddie on Windows")))
        .HintText(FText::FromString(TEXT("Gameplay device name")))
        .IsEnabled_Lambda([this]() { return CanUseGameplayControls() && BaddieCompanion::CanPairGameplay(GameplayView); })
    ];
    AddText(GameplayNameError);
    AddText(GameplayCode, 24);
    AddText(GameplayLink);
    AddText(GameplayLifetime);
    AddText(GameplayPresence);
    AddText(GameplayError);
    AddText(GameplayRetry);
    const auto AddGameplayButton = [this, &Content](TSharedPtr<SButton>& Target, const TCHAR* Label, EBaddieGameplayConnectionAction Action)
    {
        Content->AddSlot().AutoHeight().Padding(0, 0, 0, 8)[MakeGameplayButton(Target, Label, Action)];
    };
    AddGameplayButton(GameplayPairButton, TEXT("Connect gameplay entry"), EBaddieGameplayConnectionAction::Pair);
    GameplayPairButton->SetEnabled(TAttribute<bool>::CreateLambda([this]()
        { return CanUseGameplayControls() && BaddieCompanion::CanPairGameplay(GameplayView) && ValidGameplayDeviceName(); }));
    AddGameplayButton(GameplayCopyButton, TEXT("Copy gameplay approval link"), EBaddieGameplayConnectionAction::CopyLink);
    AddGameplayButton(GameplayCancelButton, TEXT("Cancel gameplay pairing"), EBaddieGameplayConnectionAction::CancelPairing);
    AddGameplayButton(GameplayEnterButton, TEXT("Enter city presence"), EBaddieGameplayConnectionAction::Enter);
    GameplayEnterButton->SetEnabled(TAttribute<bool>::CreateLambda([this]()
        { return CanUseGameplayControls() && BaddieCompanion::CanEnterGameplay(GameplayView); }));
    AddGameplayButton(GameplayCancelEntryButton, TEXT("Cancel entry"), EBaddieGameplayConnectionAction::CancelEntry);
    AddGameplayButton(GameplayLeaveButton, TEXT("Leave city presence"), EBaddieGameplayConnectionAction::Leave);
    AddGameplayButton(GameplayDisconnectButton, TEXT("Disconnect gameplay"), EBaddieGameplayConnectionAction::Disconnect);
    GameplayControls = { GameplayDeviceName, GameplayPairButton, GameplayCopyButton, GameplayCancelButton,
        GameplayEnterButton, GameplayCancelEntryButton, GameplayLeaveButton, GameplayDisconnectButton };
    Controls.Append(GameplayControls);
    PresentConnection(BaddieCompanion::FConnectionView(), nullptr);
    PresentGameplay(BaddieCompanion::FGameplayConnectionView(), 0);
}

TSharedRef<STextBlock> SBaddieCompanionConnectionPanel::MakeText(TSharedPtr<STextBlock>& Target, int32 Size)
{
    return SAssignNew(Target, STextBlock)
        .Font(FCoreStyle::GetDefaultFontStyle("Regular", Size))
        .ColorAndOpacity(FLinearColor::White)
        .WrapTextAt(this, &SBaddieCompanionConnectionPanel::TextWidth)
        .WrappingPolicy(ETextWrappingPolicy::AllowPerCharacterWrapping);
}

TSharedRef<SButton> SBaddieCompanionConnectionPanel::MakeButton(
    TSharedPtr<SButton>& Target, const TCHAR* Label, EBaddieConnectionAction Action)
{
    return SAssignNew(Target, SButton).ContentPadding(12)
        .OnClicked_Lambda([this, Action]() { return InvokeAction(Action); })
        [
            SNew(STextBlock).Text(FText::FromString(Label))
            .WrapTextAt(this, &SBaddieCompanionConnectionPanel::TextWidth)
            .WrappingPolicy(ETextWrappingPolicy::AllowPerCharacterWrapping)
        ];
}

TSharedRef<SButton> SBaddieCompanionConnectionPanel::MakeGameplayButton(
    TSharedPtr<SButton>& Target, const TCHAR* Label, EBaddieGameplayConnectionAction Action)
{
    return SAssignNew(Target, BaddieConnectionPanelPrivate::SRevisionButton,
        TAttribute<uint64>::CreateLambda([this]() { return GameplayRevision; })).ContentPadding(12)
        .IsEnabled_Lambda([this]() { return CanUseGameplayControls(); })
        .OnClicked_Lambda([this, Action]()
        {
            if (CanUseGameplayControls()) if (auto* Controller = Owner.Get())
                Controller->RequestGameplayConnectionAction(Action, GameplayRevision, GameplayDeviceName->GetText().ToString().TrimStartAndEnd());
            return FReply::Handled();
        })
        [ SNew(STextBlock).Text(FText::FromString(Label)).WrapTextAt(this, &SBaddieCompanionConnectionPanel::TextWidth) ];
}

void SBaddieCompanionConnectionPanel::SetText(const TSharedPtr<STextBlock>& Target, const FString& Text)
{
    if (!Target.IsValid()) return;
    // Avoid changing text/layout on every controller tick when the visible value is unchanged.
    if (Target->GetText().ToString() != Text) Target->SetText(FText::FromString(Text));
    SetVisible(Target, !Text.IsEmpty());
}

void SBaddieCompanionConnectionPanel::SetVisible(const TSharedPtr<SWidget>& Target, bool bVisible)
{
    if (Target.IsValid()) Target->SetVisibility(bVisible ? EVisibility::Visible : EVisibility::Collapsed);
}

void SBaddieCompanionConnectionPanel::PresentConnection(
    const BaddieCompanion::FConnectionView& View, const BaddieCompanion::FSnapshotSelection* Selection,
    const BaddieCompanion::FHomePresentationPlan* Presentation)
{
    const bool bChanged = bOffline || State != View.State || bHasSelection != (Selection != nullptr);
    bOffline = false;
    State = View.State;
    RetrySeconds = View.RetryAfterSeconds;
    bHasSelection = Selection != nullptr;
    if (bChanged) bFocusDefault = true;
    SetVisibility(EVisibility::Visible);
    SetVisible(MainSurface, true);
    const bool bApartment = View.Scope == BaddieCompanion::EConnectionScope::Apartment;
    SetText(Title, bApartment ? TEXT("Apartment") : TEXT("Home"));
    FString Status;
    switch (State)
    {
    case ECompanionConnectionState::Disabled: Status = TEXT("Companion connections are disabled."); break;
    case ECompanionConnectionState::SignedOut: Status = TEXT("Not connected to Baddie."); break;
    case ECompanionConnectionState::RequestingCode: Status = TEXT("Requesting an approval link."); break;
    case ECompanionConnectionState::AwaitingApproval:
        Status = TEXT("Approval pending. Paste this link in Baddie's Companion connections and compare the code."); break;
    case ECompanionConnectionState::LoadingHome:
        Status = bApartment ? TEXT("Connected. Loading the saved apartment rooms.")
            : TEXT("Connected. Loading the saved Home."); break;
    case ECompanionConnectionState::Ready:
        Status = Selection ? (View.bStale ? TEXT("Connected. Showing the last valid read while retrying.")
            : bApartment ? TEXT("Connected. Read-only apartment access.") : TEXT("Connected. Read-only Home access."))
            : TEXT("Saved room access is unavailable."); break;
    case ECompanionConnectionState::Expired: Status = View.Error.IsEmpty() ? TEXT("Connection expired.") : FString(); break;
    case ECompanionConnectionState::Unavailable: Status = TEXT("Connection unavailable."); break;
    }
    SetText(StatusText, Status);
    const bool bPending = BaddieCompanion::IsConnectionPending(State);
    const bool bReady = State == ECompanionConnectionState::Ready && Selection;
    const bool bCanRequest = !bPending && State != ECompanionConnectionState::Ready && State != ECompanionConnectionState::Disabled;
    SetVisible(DeviceName, bCanRequest);
    SetText(DeviceNameError, bCanRequest && !ValidDeviceName()
        ? TEXT("Use a device name of 1 to 80 characters without control characters.") : FString());
    SetText(DeviceText, View.DeviceName.IsEmpty() ? FString() : TEXT("Device: ") + View.DeviceName);
    const bool bApproval = State == ECompanionConnectionState::AwaitingApproval && View.RemainingSeconds > 0;
    SetText(ApprovalLink, bApproval ? View.ApprovalLink : FString());
    SetText(ComparisonCode, bApproval ? TEXT("Comparison code: ") + View.DisplayCode : FString());
    SetText(Lifetime, View.RemainingSeconds > 0 ? FString::Printf(TEXT("%s remaining: %d seconds"),
        bApproval ? TEXT("Approval") : TEXT("Connection"), FMath::CeilToInt(View.RemainingSeconds)) : FString());
    SetText(ErrorText, View.Error);
    SetText(RetryText, RetrySeconds > 0 ? FString::Printf(TEXT("Retry available in %d seconds."),
        FMath::CeilToInt(RetrySeconds)) : View.AutomaticRetryAfterSeconds > 0
            ? FString::Printf(TEXT("Retrying in %d seconds."), FMath::CeilToInt(View.AutomaticRetryAfterSeconds))
            : FString());
    SetText(HomeText, Selection ? TEXT("Home: ") + Selection->HomeRoom.Name : FString());
    SetText(CompanionText, Selection ? TEXT("Companion: ") + Selection->Character.Name : FString());
    SetText(ActiveRoomText, Selection ? (Selection->ActiveRoom.Id.Equals(Selection->HomeRoom.Id, ESearchCase::CaseSensitive)
        ? TEXT("Selected room: Home") : TEXT("Selected room: ") + Selection->ActiveRoom.Name) : FString());
    SetText(UnsupportedText, Selection
        ? (!Presentation ? TEXT("Saved Home or Companion data is invalid.")
            : !Presentation->Home.bSourceMapped ? TEXT("This saved Home layout is not supported yet.")
            : TEXT("Saved Home and Companion assets are not available yet.")) : FString());
    SetVisible(ConnectButton, bCanRequest);
    SetVisible(ApartmentButton, bCanRequest);
    SetVisible(CopyButton, bApproval && !View.ApprovalLink.IsEmpty());
    SetVisible(CancelButton, bPending);
    SetVisible(DisconnectButton, bReady);
    SetVisible(RepairButton, bReady);
    SetVisible(RepairApartmentButton, bReady && !bApartment);
    SetVisible(ResumeButton, false);
}

void SBaddieCompanionConnectionPanel::PresentGameplay(const BaddieCompanion::FGameplayConnectionView& View, uint64 Revision)
{
    using BaddieCompanion::EGameplayConnectionState;
    const bool bChanged = GameplayRevision != Revision;
    const bool bFocused = GameplayControls.ContainsByPredicate([](const TSharedPtr<SWidget>& Control)
        { return Control->HasKeyboardFocus() || Control->HasFocusedDescendants(); });
    GameplayView = View;
    GameplayRevision = Revision;
    if (bOffline) { ClearGameplayPresentation(); return; }
    if (bChanged && bFocused && !bModal) { bFocusDefault = true; bPreferGameplayFocus = true; }
    SetText(GameplayTitle, TEXT("Gameplay entry"));
    FString Status;
    switch (View.State)
    {
    case EGameplayConnectionState::Disabled: Status = TEXT("Gameplay entry is disabled."); break;
    case EGameplayConnectionState::SignedOut: Status = TEXT("Gameplay entry is not connected."); break;
    case EGameplayConnectionState::RequestingCode: Status = TEXT("Requesting gameplay approval."); break;
    case EGameplayConnectionState::AwaitingApproval: Status = TEXT("Approve gameplay entry in Baddie and compare the code."); break;
    case EGameplayConnectionState::Authorized: Status = TEXT("Gameplay entry approved. Ready to enter."); break;
    case EGameplayConnectionState::Entering: Status = TEXT("Requesting city presence."); break;
    case EGameplayConnectionState::Joined: Status = View.bStale ? TEXT("City presence was last confirmed; reconnecting.") : TEXT("City presence active."); break;
    case EGameplayConnectionState::Leaving: Status = TEXT("Ending city presence. Waiting for confirmation."); break;
    case EGameplayConnectionState::EntryLost: Status = TEXT("City presence ended. Waiting for entry cleanup."); break;
    case EGameplayConnectionState::Expired: Status = TEXT("Gameplay access expired."); break;
    case EGameplayConnectionState::Unavailable: Status = TEXT("Gameplay entry is unavailable."); break;
    }
    SetText(GameplayStatus, Status);
    SetText(GameplayLimits, View.State == EGameplayConnectionState::Disabled ? FString()
        : TEXT("City presence only. Movement, combat and voice are not available."));
    SetText(GameplayDeviceText, View.DeviceName.IsEmpty() ? FString() : TEXT("Gameplay device: ") + View.DeviceName);
    const bool bCanRequest = View.State == EGameplayConnectionState::SignedOut || View.State == EGameplayConnectionState::Expired
        || View.State == EGameplayConnectionState::Unavailable;
    const bool bApproval = View.State == EGameplayConnectionState::AwaitingApproval && View.GrantOrPairingRemainingSeconds > 0;
    SetVisible(GameplayDeviceName, bCanRequest);
    SetText(GameplayNameError, bCanRequest && !ValidGameplayDeviceName()
        ? TEXT("Use a device name of 1 to 80 characters without control characters.") : FString());
    SetText(GameplayCode, bApproval ? TEXT("Gameplay comparison code: ") + View.DisplayCode : FString());
    SetText(GameplayLink, bApproval ? View.ApprovalLink : FString());
    SetText(GameplayLifetime, View.GrantOrPairingRemainingSeconds > 0 ? FString::Printf(TEXT("%s expires in %d seconds."),
        bApproval ? TEXT("Gameplay approval") : TEXT("Gameplay access"), FMath::CeilToInt(View.GrantOrPairingRemainingSeconds)) : FString());
    SetText(GameplayPresence, View.State == EGameplayConnectionState::Joined && View.PresenceRemainingSeconds > 0
        ? FString::Printf(TEXT("%s presence expires in %d seconds."), View.bStale ? TEXT("Last confirmed") : TEXT("Current"),
            FMath::CeilToInt(View.PresenceRemainingSeconds)) : FString());
    SetText(GameplayError, View.Error);
    SetText(GameplayRetry, View.RetryAfterSeconds > 0
        ? FString::Printf(TEXT("Gameplay retry in %d seconds."), FMath::CeilToInt(View.RetryAfterSeconds)) : FString());
    SetVisible(GameplayPairButton, bCanRequest);
    SetVisible(GameplayCopyButton, bApproval && !View.ApprovalLink.IsEmpty());
    SetVisible(GameplayCancelButton, BaddieCompanion::IsGameplayPairing(View.State));
    SetVisible(GameplayEnterButton, View.State == EGameplayConnectionState::Authorized);
    SetVisible(GameplayCancelEntryButton, View.State == EGameplayConnectionState::Entering);
    SetVisible(GameplayLeaveButton, View.State == EGameplayConnectionState::Joined);
    SetVisible(GameplayDisconnectButton, BaddieCompanion::CanDisconnectGameplay(View));
}

void SBaddieCompanionConnectionPanel::ClearGameplayPresentation()
{
    GameplayView = {};
    GameplayRevision = 0;
    for (const auto& Text : { GameplayTitle, GameplayStatus, GameplayLimits, GameplayDeviceText, GameplayNameError,
        GameplayCode, GameplayLink, GameplayLifetime, GameplayPresence, GameplayError, GameplayRetry }) SetText(Text, FString());
    for (const auto& Control : GameplayControls) SetVisible(Control, false);
    if (GameplayDeviceName.IsValid()) GameplayDeviceName->SetText(FText::FromString(TEXT("Baddie on Windows")));
}

void SBaddieCompanionConnectionPanel::PresentOffline(
    const BaddieCompanion::FSnapshotSelection* Selection, const FString& Status, bool bCaptured)
{
    if (!bOffline || bHasSelection != (Selection != nullptr) || (bWasCaptured && !bCaptured)) bFocusDefault = true;
    bOffline = true;
    ClearGameplayPresentation();
    bWasCaptured = bCaptured;
    bHasSelection = Selection != nullptr;
    SetVisibility(bCaptured ? EVisibility::HitTestInvisible : EVisibility::Visible);
    SetVisible(MainSurface, !bCaptured);
    SetText(Title, TEXT("Offline Home inspection"));
    SetText(StatusText, Selection ? TEXT("Neutral inspection stage. Local movement only.")
        : Status.IsEmpty() ? TEXT("No offline snapshot is available.") : Status);
    SetText(HomeText, Selection ? TEXT("Saved Home: ") + Selection->HomeRoom.Name : FString());
    SetText(CompanionText, Selection ? TEXT("Saved Companion: ") + Selection->Character.Name : FString());
    SetText(ActiveRoomText, Selection ? (Selection->ActiveRoom.Id.Equals(Selection->HomeRoom.Id, ESearchCase::CaseSensitive)
        ? TEXT("Active room: Home") : TEXT("Active room: ") + Selection->ActiveRoom.Name) : FString());
    SetText(UnsupportedText, Selection
        ? TEXT("Saved appearance and Home layout are not mapped. This stage is not the saved Home; the primitive is not the saved Companion.")
        : FString());
    SetText(DeviceText, FString());
    SetText(DeviceNameError, FString());
    SetText(ApprovalLink, FString());
    SetText(ComparisonCode, FString());
    SetText(Lifetime, FString());
    SetText(ErrorText, FString());
    SetText(RetryText, FString());
    for (const auto& Control : Controls) SetVisible(Control, false);
    SetVisible(ResumeButton, Selection != nullptr);
}

void SBaddieCompanionConnectionPanel::ClearPrivatePresentation()
{
    ClearGameplayPresentation();
    ReturnFocus.Reset();
    bHasSelection = false;
    SetText(HomeText, FString());
    SetText(CompanionText, FString());
    SetText(ActiveRoomText, FString());
    SetText(UnsupportedText, FString());
    SetText(DeviceText, FString());
    SetText(DeviceNameError, FString());
    SetText(ApprovalLink, FString());
    SetText(ComparisonCode, FString());
    SetText(Lifetime, FString());
    SetText(ErrorText, FString());
    SetText(RetryText, FString());
    for (const auto& Control : Controls) SetVisible(Control, false);
    if (DeviceName.IsValid()) DeviceName->SetText(FText::FromString(TEXT("Baddie on Windows")));
    DismissConfirmation();
    bFocusDefault = true;
}

void SBaddieCompanionConnectionPanel::DetachOwner()
{
    ClearPrivatePresentation();
    Owner.Reset();
}

void SBaddieCompanionConnectionPanel::ShowConfirmation(
    EBaddieConnectionAction Action, BaddieCompanion::EConnectionScope Scope)
{
    ++ConfirmationRevision;
    const bool bRepair = Action == EBaddieConnectionAction::Repair || Action == EBaddieConnectionAction::RepairApartment;
    const bool bApartment = Scope == BaddieCompanion::EConnectionScope::Apartment;
    ReturnFocus = Action == EBaddieConnectionAction::RepairApartment ? RepairApartmentButton
        : bRepair ? RepairButton : DisconnectButton;
    bModal = true;
    MainSurface->SetEnabled(false);
    SetText(ModalTitle, Action == EBaddieConnectionAction::RepairApartment ? TEXT("Connect apartment?")
        : bRepair ? TEXT("Pair again?") : TEXT("Disconnect from Baddie?"));
    SetText(ModalBody, bRepair
        ? (bApartment ? TEXT("Clear this connection and request approval for a Companion and all saved apartment rooms.")
            : TEXT("Clear this connection and request fresh approval for a Companion and Home."))
        : (bApartment ? TEXT("Clear the shared Companion and apartment rooms from this game.")
            : TEXT("Clear the shared Companion and Home from this game.")));
    ConfirmButton->SetContent(SNew(STextBlock).Text(FText::FromString(
        Action == EBaddieConnectionAction::RepairApartment ? TEXT("Connect apartment")
            : bRepair ? TEXT("Pair again") : TEXT("Disconnect"))));
    SetVisible(ModalSurface, true);
    Focus(KeepButton);
    bFocusDefault = false;
}

void SBaddieCompanionConnectionPanel::ShowGameplayConfirmation(EBaddieGameplayConnectionAction Action)
{
    ++ConfirmationRevision;
    const bool bLeave = Action == EBaddieGameplayConnectionAction::Leave;
    ReturnFocus = bLeave ? GameplayLeaveButton : GameplayDisconnectButton;
    bModal = true;
    MainSurface->SetEnabled(false);
    SetText(ModalTitle, bLeave ? TEXT("Leave city presence?") : TEXT("Disconnect gameplay entry?"));
    SetText(ModalBody, bLeave ? TEXT("End this city presence. Gameplay approval remains until it expires.")
        : TEXT("End this city presence and remove gameplay approval from this device. Home and apartment read access are unchanged."));
    ConfirmButton->SetContent(SNew(STextBlock).Text(FText::FromString(bLeave ? TEXT("Leave") : TEXT("Disconnect")))
        .WrapTextAt(this, &SBaddieCompanionConnectionPanel::TextWidth));
    SetVisible(ModalSurface, true);
    Focus(KeepButton);
    bFocusDefault = false;
}

void SBaddieCompanionConnectionPanel::DismissConfirmation()
{
    ++ConfirmationRevision;
    const bool bWasOpen = bModal;
    bModal = false;
    SetVisible(ModalSurface, false);
    if (MainSurface.IsValid()) MainSurface->SetEnabled(true);
    SetText(ModalTitle, FString());
    SetText(ModalBody, FString());
    if (bWasOpen)
    {
        const auto Previous = ReturnFocus.Pin();
        if (IsFocusableControl(Previous)) { Focus(Previous); bFocusDefault = false; }
        else bFocusDefault = true;
    }
    ReturnFocus.Reset();
}

bool SBaddieCompanionConnectionPanel::CanConnect() const
{
    return !bOffline && !bModal && Owner.IsValid() && RetrySeconds <= 0
        && State != ECompanionConnectionState::Disabled && State != ECompanionConnectionState::Ready
        && !BaddieCompanion::IsConnectionPending(State);
}

bool SBaddieCompanionConnectionPanel::ValidDeviceName() const
{
    if (!DeviceName.IsValid()) return false;
    return BaddieCompanion::IsValidConnectionDeviceName(DeviceName->GetText().ToString());
}

bool SBaddieCompanionConnectionPanel::CanUseGameplayControls() const
{
    return !bOffline && !bModal && Owner.IsValid() && GameplayRevision != 0;
}

bool SBaddieCompanionConnectionPanel::ValidGameplayDeviceName() const
{
    return GameplayDeviceName.IsValid() && BaddieCompanion::IsValidConnectionDeviceName(GameplayDeviceName->GetText().ToString());
}

FReply SBaddieCompanionConnectionPanel::InvokeAction(EBaddieConnectionAction Action)
{
    if (auto* Controller = Owner.Get())
        Controller->RequestConnectionAction(Action, DeviceName->GetText().ToString().TrimStartAndEnd());
    return FReply::Handled();
}

FOptionalSize SBaddieCompanionConnectionPanel::PanelWidth() const
{
    const float Width = GetCachedGeometry().GetLocalSize().X;
    return FOptionalSize(FMath::Clamp(Width > 0 ? Width - 32 : 640.0f, 1.0f, 760.0f));
}

FOptionalSize SBaddieCompanionConnectionPanel::ModalHeight() const
{
    const float Height = GetCachedGeometry().GetLocalSize().Y;
    return FOptionalSize(FMath::Clamp(Height > 0 ? Height - 32 : 460.0f, 1.0f, 460.0f));
}

float SBaddieCompanionConnectionPanel::TextWidth() const
{
    const float Width = GetCachedGeometry().GetLocalSize().X;
    return FMath::Max(1.0f, FMath::Min(Width > 0 ? Width - 32 : 640.0f, 760.0f) - 64);
}

bool SBaddieCompanionConnectionPanel::IsFocusableControl(const TSharedPtr<SWidget>& Target) const
{
    return Target.IsValid() && Target->GetVisibility() == EVisibility::Visible && Target->IsEnabled();
}

void SBaddieCompanionConnectionPanel::Focus(const TSharedPtr<SWidget>& Target)
{
    if (!Target.IsValid() || !FSlateApplication::IsInitialized()) return;
    FSlateApplication::Get().SetKeyboardFocus(Target, EFocusCause::SetDirectly);
    const auto& Scroll = bModal ? ModalScroll : MainScroll;
    if (Scroll.IsValid() && Target != SharedThis(this)) Scroll->ScrollDescendantIntoView(Target, false);
}

void SBaddieCompanionConnectionPanel::FocusDefault()
{
    if (bModal) { Focus(KeepButton); return; }
    if (bPreferGameplayFocus && !bOffline)
        for (const auto& Control : GameplayControls)
            if (IsFocusableControl(Control)) { Focus(Control); bPreferGameplayFocus = false; return; }
    bPreferGameplayFocus = false;
    for (const auto& Control : Controls)
        if (IsFocusableControl(Control)) { Focus(Control); return; }
    Focus(SharedThis(this));
}

void SBaddieCompanionConnectionPanel::Tick(const FGeometry& Geometry, double CurrentTime, float DeltaTime)
{
    SCompoundWidget::Tick(Geometry, CurrentTime, DeltaTime);
    // Guard expiry at the Slate presentation boundary as well as controller tick and actions.
    if (auto* Controller = Owner.Get()) Controller->RefreshConnectionPresentation();
    if (MainSurface->GetVisibility() == EVisibility::Collapsed) return;
    if (bModal && !ModalSurface->HasKeyboardFocus() && !ModalSurface->HasFocusedDescendants()) Focus(KeepButton);
    else if (bFocusDefault) { bFocusDefault = false; FocusDefault(); }
}

FReply SBaddieCompanionConnectionPanel::OnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if (Event.GetKey() == EKeys::Escape || Event.GetKey() == EKeys::Gamepad_Special_Right)
    {
        if (auto* Controller = Owner.Get()) Controller->HandleConnectionEscape();
        return FReply::Handled();
    }
    if (Event.GetKey() == EKeys::Tab)
    {
        const TArray<TSharedPtr<SWidget>> Candidates = bModal
            ? TArray<TSharedPtr<SWidget>>{ KeepButton, ConfirmButton } : Controls;
        TArray<TSharedPtr<SWidget>> Targets;
        for (const auto& Candidate : Candidates) if (IsFocusableControl(Candidate)) Targets.Add(Candidate);
        if (Targets.IsEmpty()) return FReply::Handled();
        // Text boxes focus their inner editor, not necessarily the outer control.
        int32 Index = Targets.IndexOfByPredicate([](const TSharedPtr<SWidget>& Target)
        {
            return Target->HasKeyboardFocus() || Target->HasFocusedDescendants();
        });
        Index = Index == INDEX_NONE ? (Event.IsShiftDown() ? Targets.Num() - 1 : 0)
            : (Index + (Event.IsShiftDown() ? Targets.Num() - 1 : 1)) % Targets.Num();
        Focus(Targets[Index]);
        return FReply::Handled();
    }
    return SCompoundWidget::OnPreviewKeyDown(Geometry, Event);
}

FReply SBaddieCompanionConnectionPanel::OnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if (Event.GetKey() == EKeys::PageDown || Event.GetKey() == EKeys::PageUp
        || Event.GetKey() == EKeys::Up || Event.GetKey() == EKeys::Down)
    {
        const auto& Scroll = bModal ? ModalScroll : MainScroll;
        const bool bUp = Event.GetKey() == EKeys::PageUp || Event.GetKey() == EKeys::Up;
        const float Distance = Event.GetKey() == EKeys::Up || Event.GetKey() == EKeys::Down ? 48 : 300;
        if (Scroll.IsValid()) Scroll->SetScrollOffset(FMath::Max(0.0f, Scroll->GetScrollOffset() + (bUp ? -Distance : Distance)));
        return FReply::Handled();
    }
    if (Event.GetKey() == EKeys::Enter || Event.GetKey() == EKeys::SpaceBar)
    {
        FocusDefault();
        return FReply::Handled();
    }
    return SCompoundWidget::OnKeyDown(Geometry, Event);
}
