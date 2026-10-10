#include "SBaddieCompanionConnectionPanel.h"

#include "BaddieCompanionConnectionUi.h"
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

using BaddieCompanion::EConnectionState;

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
                                SAssignNew(KeepButton, SButton).ContentPadding(12)
                                .Text(FText::FromString(TEXT("Cancel")))
                                .OnClicked_Lambda([this]()
                                {
                                    if (auto* Controller = Owner.Get()) Controller->CancelConnectionAction();
                                    return FReply::Handled();
                                })
                            ]
                            + SVerticalBox::Slot().AutoHeight()
                            [
                                SAssignNew(ConfirmButton, SButton).ContentPadding(12)
                                .Text(FText::FromString(TEXT("Confirm")))
                                .OnClicked_Lambda([this]()
                                {
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
    AddButton(ConnectButton, TEXT("Connect to Baddie"), EBaddieConnectionAction::Connect);
    ConnectButton->SetEnabled(TAttribute<bool>::CreateLambda([this]() { return CanConnect() && ValidDeviceName(); }));
    AddButton(CopyButton, TEXT("Copy approval link"), EBaddieConnectionAction::CopyLink);
    AddButton(CancelButton, TEXT("Cancel connection"), EBaddieConnectionAction::Cancel);
    AddButton(DisconnectButton, TEXT("Disconnect"), EBaddieConnectionAction::Disconnect);
    AddButton(RepairButton, TEXT("Pair again"), EBaddieConnectionAction::Repair);
    AddButton(ResumeButton, TEXT("Resume inspection"), EBaddieConnectionAction::ResumeInspection);
    Controls = { DeviceName, ConnectButton, CopyButton, CancelButton, DisconnectButton, RepairButton, ResumeButton };
    PresentConnection(BaddieCompanion::FConnectionView(), nullptr);
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
    const BaddieCompanion::FConnectionView& View, const BaddieCompanion::FSnapshotSelection* Selection)
{
    const bool bChanged = bOffline || State != View.State || bHasSelection != (Selection != nullptr);
    bOffline = false;
    State = View.State;
    RetrySeconds = View.RetryAfterSeconds;
    bHasSelection = Selection != nullptr;
    if (bChanged) bFocusDefault = true;
    SetVisibility(EVisibility::Visible);
    SetVisible(MainSurface, true);
    SetText(Title, TEXT("Home"));
    FString Status;
    switch (State)
    {
    case EConnectionState::Disabled: Status = TEXT("Companion connections are disabled."); break;
    case EConnectionState::SignedOut: Status = TEXT("Not connected to Baddie."); break;
    case EConnectionState::RequestingCode: Status = TEXT("Requesting an approval link."); break;
    case EConnectionState::AwaitingApproval:
        Status = TEXT("Approval pending. Paste this link in Baddie's Companion connections and compare the code."); break;
    case EConnectionState::LoadingHome: Status = TEXT("Connected. Loading the saved Home."); break;
    case EConnectionState::Ready:
        Status = Selection ? (View.bStale ? TEXT("Connected. Showing the last valid read while retrying.")
            : TEXT("Connected. Read-only Home access.")) : TEXT("Home access is unavailable."); break;
    case EConnectionState::Expired: Status = View.Error.IsEmpty() ? TEXT("Connection expired.") : FString(); break;
    case EConnectionState::Unavailable: Status = TEXT("Connection unavailable."); break;
    }
    SetText(StatusText, Status);
    const bool bPending = BaddieCompanion::IsConnectionPending(State);
    const bool bReady = State == EConnectionState::Ready && Selection;
    const bool bCanRequest = !bPending && State != EConnectionState::Ready && State != EConnectionState::Disabled;
    SetVisible(DeviceName, bCanRequest);
    SetText(DeviceNameError, bCanRequest && !ValidDeviceName()
        ? TEXT("Use a device name of 1 to 80 characters without control characters.") : FString());
    SetText(DeviceText, View.DeviceName.IsEmpty() ? FString() : TEXT("Device: ") + View.DeviceName);
    const bool bApproval = State == EConnectionState::AwaitingApproval && View.RemainingSeconds > 0;
    SetText(ApprovalLink, bApproval ? View.ApprovalLink : FString());
    SetText(ComparisonCode, bApproval ? TEXT("Comparison code: ") + View.DisplayCode : FString());
    SetText(Lifetime, View.RemainingSeconds > 0 ? FString::Printf(TEXT("%s remaining: %d seconds"),
        bApproval ? TEXT("Approval") : TEXT("Connection"), FMath::CeilToInt(View.RemainingSeconds)) : FString());
    SetText(ErrorText, View.Error);
    SetText(RetryText, RetrySeconds > 0 ? FString::Printf(TEXT("Retry available in %d seconds."),
        FMath::CeilToInt(RetrySeconds)) : FString());
    SetText(HomeText, Selection ? TEXT("Home: ") + Selection->HomeRoom.Name : FString());
    SetText(CompanionText, Selection ? TEXT("Companion: ") + Selection->Character.Name : FString());
    SetText(ActiveRoomText, Selection ? (Selection->ActiveRoom.Id.Equals(Selection->HomeRoom.Id, ESearchCase::CaseSensitive)
        ? TEXT("Selected room: Home") : TEXT("Selected room: ") + Selection->ActiveRoom.Name) : FString());
    SetText(UnsupportedText, Selection
        ? TEXT("Saved Companion appearance and Home mapping are not supported yet. Gameplay entry is not available.") : FString());
    SetVisible(ConnectButton, bCanRequest);
    SetVisible(CopyButton, bApproval && !View.ApprovalLink.IsEmpty());
    SetVisible(CancelButton, bPending);
    SetVisible(DisconnectButton, bReady);
    SetVisible(RepairButton, bReady);
    SetVisible(ResumeButton, false);
}

void SBaddieCompanionConnectionPanel::PresentOffline(
    const BaddieCompanion::FSnapshotSelection* Selection, const FString& Status, bool bCaptured)
{
    if (!bOffline || bHasSelection != (Selection != nullptr) || (bWasCaptured && !bCaptured)) bFocusDefault = true;
    bOffline = true;
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

void SBaddieCompanionConnectionPanel::ShowConfirmation(EBaddieConnectionAction Action)
{
    ReturnFocus = Action == EBaddieConnectionAction::Repair ? RepairButton : DisconnectButton;
    bModal = true;
    MainSurface->SetEnabled(false);
    SetText(ModalTitle, Action == EBaddieConnectionAction::Repair ? TEXT("Pair again?") : TEXT("Disconnect from Baddie?"));
    SetText(ModalBody, Action == EBaddieConnectionAction::Repair
        ? TEXT("Clear this connection and request fresh approval for a Companion and Home.")
        : TEXT("Clear the shared Companion and Home from this game."));
    ConfirmButton->SetContent(SNew(STextBlock).Text(FText::FromString(
        Action == EBaddieConnectionAction::Repair ? TEXT("Pair again") : TEXT("Disconnect"))));
    SetVisible(ModalSurface, true);
    Focus(KeepButton);
    bFocusDefault = false;
}

void SBaddieCompanionConnectionPanel::DismissConfirmation()
{
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
        && State != EConnectionState::Disabled && State != EConnectionState::Ready
        && !BaddieCompanion::IsConnectionPending(State);
}

bool SBaddieCompanionConnectionPanel::ValidDeviceName() const
{
    if (!DeviceName.IsValid()) return false;
    return BaddieCompanion::IsValidConnectionDeviceName(DeviceName->GetText().ToString());
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
