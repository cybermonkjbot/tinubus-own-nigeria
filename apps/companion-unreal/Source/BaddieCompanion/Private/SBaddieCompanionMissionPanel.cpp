#include "SBaddieCompanionMissionPanel.h"

#include "BaddieCompanionSlicePlayerController.h"
#include "SBaddieCompanionAudioCompass.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "HAL/PlatformTime.h"
#include "InputCoreTypes.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSafeZone.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace BaddieMissionPanelPrivate
{
    BaddieCompanion::FMissionSelectedAction SelectionFromView(const BaddieCompanion::FMissionHudView& View)
    {
        BaddieCompanion::FMissionSelectedAction Selection;
        Selection.TargetId = View.TargetId;
        Selection.Revision = View.TargetRevision;
        Selection.Action = View.Action;
        return Selection;
    }
}

void SBaddieCompanionMissionPanel::Construct(const FArguments& Args)
{
    Owner = Args._Owner;
    TSharedPtr<SButton> MenuButton;
    TSharedPtr<SButton> ResumeButton;
    TSharedPtr<SCheckBox> AssistanceToggle;
    ChildSlot
    [
        SNew(SOverlay)
        + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
        [ SAssignNew(Compass, SBaddieCompanionAudioCompass) ]
        + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
        [
            SNew(STextBlock).Text(FText::FromString(TEXT("+")))
            .ColorAndOpacity(FLinearColor::White)
            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 20))
            .ShadowOffset(FVector2D(1, 1)).ShadowColorAndOpacity(FLinearColor::Black)
            .Visibility_Lambda([this]() { return View.bModal ? EVisibility::Collapsed : EVisibility::HitTestInvisible; })
        ]
        + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
        .Padding(this, &SBaddieCompanionMissionPanel::ActionOffset)
        [
            SNew(SBox).WidthOverride_Lambda([this]() { return FOptionalSize(ActionWidth()); })
            .MinDesiredHeight(48)
            [
                SAssignNew(ActionButton, SButton).ContentPadding(FMargin(12, 8))
                .IsEnabled_Lambda([this]() { return View.bCanAct; })
                .OnPressed_Lambda([this]() { BeginControl(); ActionPressView = View; })
                .OnClicked_Lambda([this]()
                {
                    const auto Press = ActionPressView;
                    ActionPressView.Reset();
                    if (Press.IsSet()) if (auto* Controller = Owner.Get())
                        Controller->RequestMissionAction(Press->Token, BaddieMissionPanelPrivate::SelectionFromView(Press.GetValue()));
                    return FReply::Handled();
                })
                [
                    SNew(STextBlock).Text(this, &SBaddieCompanionMissionPanel::ActionText)
                    .Justification(ETextJustify::Center)
                    .WrapTextAt_Lambda([this]() { return FMath::Max(1.0f, ActionWidth() - 24); })
                ]
            ]
        ]
        + SOverlay::Slot()
        [
            SNew(SSafeZone).Visibility_Lambda([this]() { return View.bModal ? EVisibility::Collapsed : EVisibility::SelfHitTestInvisible; })
            [
                SNew(SOverlay).Visibility(EVisibility::SelfHitTestInvisible)
                + SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(12)
                [
                    SNew(SBox).MinDesiredWidth(64).MinDesiredHeight(48)
                    [
                        SAssignNew(MenuButton, SButton).Text(FText::FromString(TEXT("Menu")))
                        .OnPressed_Lambda([this]() { OpenMenu(); })
                        .OnClicked(this, &SBaddieCompanionMissionPanel::OpenMenu)
                    ]
                ]
                + SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(20)
                [
                    SNew(SBox).WidthOverride(96).HeightOverride(96).Visibility(EVisibility::HitTestInvisible)
                    [
                        SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                        .BorderBackgroundColor(FLinearColor(0.03f, 0.03f, 0.04f, 0.55f))
                        .HAlign(HAlign_Center).VAlign(VAlign_Center)
                        [ SNew(STextBlock).Text(FText::FromString(TEXT("Move"))).ColorAndOpacity(FLinearColor::White) ]
                    ]
                ]
                + SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(12)
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
                    [
                        SNew(SBox).MinDesiredWidth(80).MinDesiredHeight(48)
                        [
                            SAssignNew(AimButton, SButton).Text(FText::FromString(TEXT("Aim"))).IsFocusable(false)
                            .IsEnabled_Lambda([this]() { return View.bCanADS; })
                            .OnPressed_Lambda([this]()
                            {
                                ADSPressToken = View.Token;
                                if (auto* Controller = Owner.Get()) Controller->RequestMissionADS(View.Token, true);
                            })
                            .OnReleased_Lambda([this]()
                            {
                                const auto Token = ADSPressToken;
                                ADSPressToken.Reset();
                                if (Token.IsSet()) if (auto* Controller = Owner.Get()) Controller->RequestMissionADS(Token.GetValue(), false);
                            })
                        ]
                    ]
                    + SVerticalBox::Slot().AutoHeight()
                    [
                        SNew(SBox).MinDesiredWidth(80).MinDesiredHeight(64)
                        [
                            SAssignNew(FireButton, SButton).Text(FText::FromString(TEXT("Fire"))).IsFocusable(false)
                            .IsEnabled_Lambda([this]() { return View.bCanFire; })
                            .OnPressed_Lambda([this]()
                            {
                                // Fire can be pressed while manually holding Aim. It never starts a look gesture.
                                FirePressToken = View.Token;
                                if (auto* Controller = Owner.Get()) Controller->RequestMissionFire(View.Token, true);
                            })
                            .OnReleased_Lambda([this]()
                            {
                                const auto Token = FirePressToken;
                                FirePressToken.Reset();
                                if (Token.IsSet()) if (auto* Controller = Owner.Get()) Controller->RequestMissionFire(Token.GetValue(), false);
                            })
                        ]
                    ]
                ]
            ]
        ]
        + SOverlay::Slot()
        [
            SAssignNew(Modal, SBorder).Visibility(EVisibility::Collapsed)
            .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
            .BorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.03f, 0.96f))
            .Padding(16).HAlign(HAlign_Center).VAlign(VAlign_Center)
            [
                SNew(SSafeZone)
                [
                    SNew(SBox).WidthOverride_Lambda([this]() { return FOptionalSize(FMath::Min(360.0f, ActionWidth())); })
                    .MaxDesiredHeight_Lambda([this]() { return FOptionalSize(FMath::Max(1.0f, static_cast<float>(GetCachedGeometry().GetLocalSize().Y) - 64.0f)); })
                    [
                        SNew(SScrollBox)
                        + SScrollBox::Slot()
                        [
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 16)
                            [
                                SAssignNew(AssistanceToggle, SCheckBox)
                                .IsChecked_Lambda([this]() { return View.bHipAssistance ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
                                .OnCheckStateChanged_Lambda([this](ECheckBoxState State)
                                {
                                    if (auto* Controller = Owner.Get()) Controller->SetMissionHipAssistance(State == ECheckBoxState::Checked);
                                })
                                [ SNew(STextBlock).Text(FText::FromString(TEXT("Hip-fire assistance"))).AutoWrapText(true) ]
                            ]
                            + SVerticalBox::Slot().AutoHeight()
                            [
                                SNew(SBox).MinDesiredHeight(48)
                                [
                                    SAssignNew(ResumeButton, SButton).Text(FText::FromString(TEXT("Resume")))
                                    .OnClicked_Lambda([this]()
                                    {
                                        if (auto* Controller = Owner.Get()) Controller->SetMissionModal(false);
                                        return FReply::Handled();
                                    })
                                ]
                            ]
                        ]
                    ]
                ]
            ]
        ]
    ];
    CapturingControls = { ActionButton, FireButton, AimButton, MenuButton, ResumeButton, AssistanceToggle };
    Clear();
}

void SBaddieCompanionMissionPanel::Present(const BaddieCompanion::FMissionHudView& Next, double ListenerYawDegrees)
{
    const bool bChanged = !(View.Token == Next.Token);
    const bool bNewAction = bChanged || View.TargetId != Next.TargetId || View.TargetRevision != Next.TargetRevision
        || View.Action != Next.Action || View.ActionState == BaddieCompanion::EMissionActionState::Hidden;
    if (bChanged) CancelPointers();
    View = Next;
    SetVisibility(EVisibility::Visible);
    Modal->SetVisibility(View.bModal ? EVisibility::Visible : EVisibility::Collapsed);
    if (View.bModal) Compass->Clear();
    else Compass->PresentView(View.Token.Presentation, ListenerYawDegrees);
    const bool bAction = !View.bModal && View.ActionState != BaddieCompanion::EMissionActionState::Hidden;
    if (!bAction && ActionButton->HasKeyboardFocus() && FSlateApplication::IsInitialized())
        FSlateApplication::Get().SetKeyboardFocus(SharedThis(this), EFocusCause::SetDirectly);
    ActionButton->SetVisibility(bAction ? EVisibility::Visible : EVisibility::Collapsed);
    if (bNewAction) ActionAppearedAt = FPlatformTime::Seconds();
}

bool SBaddieCompanionMissionPanel::AddHeardCue(uint64 Generation, uint8 WorldSector, double RemainingSeconds)
{
    return !View.bModal && Generation == View.Token.Presentation && WorldSector < 8
        && Compass->AddHeardCue(Generation, WorldSector * 45.0, RemainingSeconds);
}

void SBaddieCompanionMissionPanel::Clear()
{
    CancelPointers();
    View = {};
    if (Compass.IsValid()) Compass->Clear();
    SetVisibility(EVisibility::Collapsed);
}

void SBaddieCompanionMissionPanel::CancelPointers()
{
    const auto Releasing = MoveTemp(Pointers);
    Pointers.Reset();
    MoveKeys.Reset();
    StickMove = StickLook = FVector2D::ZeroVector;
    FirePressToken.Reset();
    ADSPressToken.Reset();
    ActionPressView.Reset();
    if (FSlateApplication::IsInitialized())
    {
        for (const auto& Pointer : Releasing)
            if (const auto User = FSlateApplication::Get().GetUser(static_cast<int32>(Pointer.Value.UserIndex)))
                if (User->GetPointerCaptor(Pointer.Key).Get() == this) User->ReleaseCapture(Pointer.Key);
        // Release only this panel's controls, never another modal or another UI owner's capture.
        FSlateApplication::Get().ForEachUser([this](FSlateUser& User)
        {
            const auto ReleaseOwned = [this, &User](uint32 Index)
            {
                const auto Captor = User.GetPointerCaptor(Index);
                if (Captor.IsValid() && (Captor.Get() == this || CapturingControls.Contains(Captor))) User.ReleaseCapture(Index);
            };
            for (uint32 Index = 0; Index < static_cast<uint32>(ETouchIndex::MAX_TOUCHES); ++Index) ReleaseOwned(Index);
            ReleaseOwned(FSlateApplication::CursorPointerIndex);
        });
    }
}

void SBaddieCompanionMissionPanel::BeginControl()
{
    if (auto* Controller = Owner.Get()) Controller->CancelMissionInputs();
}

FReply SBaddieCompanionMissionPanel::OpenMenu()
{
    if (auto* Controller = Owner.Get()) Controller->SetMissionModal(true);
    return FReply::Handled();
}

float SBaddieCompanionMissionPanel::ActionWidth() const
{
    return FMath::Clamp(static_cast<float>(GetCachedGeometry().GetLocalSize().X) - 32.0f, 1.0f, 280.0f);
}

FMargin SBaddieCompanionMissionPanel::ActionOffset() const
{
    // Centre alignment plus top padding places the action below the unobstructed 128-unit compass.
    return FMargin(0, 188, 0, 0);
}

FText SBaddieCompanionMissionPanel::ActionText() const
{
    using namespace BaddieCompanion;
    const bool bBrush = View.Action == EMissionAction::ClearBrush;
    if (View.ActionState == EMissionActionState::Pending)
        return FText::FromString(bBrush ? TEXT("Clearing brush...") : TEXT("Untying..."));
    if (View.ActionState == EMissionActionState::Failed)
        return FText::FromString(bBrush ? TEXT("Could not clear brush. Retry") : TEXT("Could not untie. Retry"));
    return FText::FromString(bBrush ? TEXT("Clear brush") : TEXT("Untie"));
}

void SBaddieCompanionMissionPanel::Tick(const FGeometry& Geometry, double CurrentTime, float DeltaTime)
{
    SCompoundWidget::Tick(Geometry, CurrentTime, DeltaTime);
    if (View.Token.Presentation == 0) return;
    if (auto* Controller = Owner.Get())
    {
        if (!View.bModal && !StickLook.IsNearlyZero())
            Controller->RequestMissionLook(View.Token, StickLook * FMath::Clamp(DeltaTime, 0.0f, 0.05f));
    }
    const float Alpha = static_cast<float>(FMath::Clamp((FPlatformTime::Seconds() - ActionAppearedAt) / 0.15, 0.0, 1.0));
    ActionButton->SetRenderOpacity(Alpha);
}

FReply SBaddieCompanionMissionPanel::BeginPointer(const FGeometry& Geometry, const FPointerEvent& Event)
{
    if (View.bModal || View.Token.Presentation == 0) return FReply::Handled();
    // Disabled or pending controls still consume pointers; they are not a look/swipe region.
    for (const auto& Control : { ActionButton, FireButton, AimButton })
        if (Control.IsValid() && Control->GetVisibility() != EVisibility::Collapsed
            && Control->GetCachedGeometry().IsUnderLocation(Event.GetScreenSpacePosition())) return FReply::Handled();
    const FVector2D Point = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
    FPointer Pointer;
    Pointer.UserIndex = Event.GetUserIndex();
    Pointer.Token = View.Token;
    Pointer.Selection = BaddieMissionPanelPrivate::SelectionFromView(View);
    Pointer.Origin = Pointer.Last = Point;
    if (Event.IsTouchEvent() && Point.X < Geometry.GetLocalSize().X * 0.4)
        Pointer.Kind = EPointerOwner::Move;
    else if (View.bCanAct && View.Action == BaddieCompanion::EMissionAction::ClearBrush)
        Pointer.Kind = EPointerOwner::Brush;
    else Pointer.Kind = EPointerOwner::Look;
    // A gesture keeps its original role. A swipe never also rotates or fires.
    for (const auto& Existing : Pointers)
        if (Existing.Value.Kind == Pointer.Kind) return FReply::Handled();
    Pointers.Add(Event.GetPointerIndex(), Pointer);
    return FReply::Handled().CaptureMouse(SharedThis(this));
}

FReply SBaddieCompanionMissionPanel::MovePointer(const FGeometry& Geometry, const FPointerEvent& Event)
{
    auto* Pointer = Pointers.Find(Event.GetPointerIndex());
    if (!Pointer || View.bModal) return FReply::Handled();
    const FVector2D Point = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
    const FPointer Captured = *Pointer;
    Pointer->Last = Point;
    if (auto* Controller = Owner.Get())
    {
        if (Captured.Kind == EPointerOwner::Move)
            Controller->RequestMissionMove(Captured.Token, FVector2D(Point.X - Captured.Origin.X, Captured.Origin.Y - Point.Y) / 48.0);
        else if (Captured.Kind == EPointerOwner::Look)
        {
            const double Width = FMath::Max(1.0, static_cast<double>(Geometry.GetLocalSize().X));
            Controller->RequestMissionLook(Captured.Token, (Point - Captured.Last) / Width);
        }
    }
    return FReply::Handled();
}

FReply SBaddieCompanionMissionPanel::EndPointer(const FGeometry& Geometry, const FPointerEvent& Event)
{
    FPointer Pointer;
    if (!Pointers.RemoveAndCopyValue(Event.GetPointerIndex(), Pointer)) return FReply::Handled().ReleaseMouseCapture();
    if (auto* Controller = Owner.Get())
    {
        if (Pointer.Kind == EPointerOwner::Move) Controller->RequestMissionMove(Pointer.Token, FVector2D::ZeroVector);
        else if (Pointer.Kind == EPointerOwner::Brush)
        {
            const FVector2D Travel = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition()) - Pointer.Origin;
            if (Travel.Size() >= 48) Controller->RequestMissionAction(Pointer.Token, Pointer.Selection, true);
        }
    }
    return FReply::Handled().ReleaseMouseCapture();
}

FReply SBaddieCompanionMissionPanel::OnTouchStarted(const FGeometry& G, const FPointerEvent& E) { return BeginPointer(G, E); }
FReply SBaddieCompanionMissionPanel::OnTouchMoved(const FGeometry& G, const FPointerEvent& E) { return MovePointer(G, E); }
FReply SBaddieCompanionMissionPanel::OnTouchEnded(const FGeometry& G, const FPointerEvent& E) { return EndPointer(G, E); }
FReply SBaddieCompanionMissionPanel::OnMouseButtonDown(const FGeometry& G, const FPointerEvent& E)
{
    return E.GetEffectingButton() == EKeys::LeftMouseButton ? BeginPointer(G, E) : FReply::Handled();
}
FReply SBaddieCompanionMissionPanel::OnMouseMove(const FGeometry& G, const FPointerEvent& E) { return MovePointer(G, E); }
FReply SBaddieCompanionMissionPanel::OnMouseButtonUp(const FGeometry& G, const FPointerEvent& E) { return EndPointer(G, E); }

void SBaddieCompanionMissionPanel::OnMouseCaptureLost(const FCaptureLostEvent& Event)
{
    SCompoundWidget::OnMouseCaptureLost(Event);
    // A normal release was removed in EndPointer. Losing an outstanding capture cancels input.
    if (Pointers.Contains(Event.PointerIndex))
        if (auto* Controller = Owner.Get()) Controller->ClearMissionPresentation();
}

void SBaddieCompanionMissionPanel::UpdateKeyboardMove()
{
    const FVector2D Value((MoveKeys.Contains(EKeys::D) ? 1 : 0) - (MoveKeys.Contains(EKeys::A) ? 1 : 0),
        (MoveKeys.Contains(EKeys::W) ? 1 : 0) - (MoveKeys.Contains(EKeys::S) ? 1 : 0));
    if (auto* Controller = Owner.Get()) Controller->RequestMissionMove(View.Token, Value);
}

FReply SBaddieCompanionMissionPanel::OnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if (Event.IsAltDown() || Event.IsControlDown() || Event.IsCommandDown())
        return SCompoundWidget::OnKeyDown(Geometry, Event);
    const FKey Key = Event.GetKey();
    auto* Controller = Owner.Get();
    if (!Controller) return FReply::Handled();
    if (Key == EKeys::Escape || Key == EKeys::Gamepad_Special_Right)
    {
        if (!Event.IsRepeat()) Controller->SetMissionModal(!View.bModal);
        return FReply::Handled();
    }
    if (View.bModal) return SCompoundWidget::OnKeyDown(Geometry, Event);
    const bool bGameplayKey = Key == EKeys::W || Key == EKeys::A || Key == EKeys::S || Key == EKeys::D
        || Key == EKeys::E || Key == EKeys::Gamepad_FaceButton_Bottom || Key == EKeys::SpaceBar
        || Key == EKeys::Gamepad_RightTrigger || Key == EKeys::LeftShift || Key == EKeys::Gamepad_LeftTrigger;
    if (!bGameplayKey) return SCompoundWidget::OnKeyDown(Geometry, Event);
    if (Key == EKeys::W || Key == EKeys::A || Key == EKeys::S || Key == EKeys::D)
    {
        MoveKeys.Add(Key); UpdateKeyboardMove();
    }
    else if (!Event.IsRepeat())
    {
        if (Key == EKeys::E || Key == EKeys::Gamepad_FaceButton_Bottom)
            Controller->RequestMissionAction(View.Token, BaddieMissionPanelPrivate::SelectionFromView(View));
        else if (Key == EKeys::SpaceBar || Key == EKeys::Gamepad_RightTrigger)
        {
            FirePressToken = View.Token; Controller->RequestMissionFire(View.Token, true);
        }
        else if (Key == EKeys::LeftShift || Key == EKeys::Gamepad_LeftTrigger)
        {
            ADSPressToken = View.Token; Controller->RequestMissionADS(View.Token, true);
        }
    }
    return FReply::Handled();
}

FReply SBaddieCompanionMissionPanel::OnKeyUp(const FGeometry& Geometry, const FKeyEvent& Event)
{
    const FKey Key = Event.GetKey();
    if (MoveKeys.Remove(Key)) UpdateKeyboardMove();
    if (auto* Controller = Owner.Get())
    {
        if ((Key == EKeys::SpaceBar || Key == EKeys::Gamepad_RightTrigger) && FirePressToken.IsSet())
        {
            const auto Token = FirePressToken.GetValue(); FirePressToken.Reset(); Controller->RequestMissionFire(Token, false);
        }
        if ((Key == EKeys::LeftShift || Key == EKeys::Gamepad_LeftTrigger) && ADSPressToken.IsSet())
        {
            const auto Token = ADSPressToken.GetValue(); ADSPressToken.Reset(); Controller->RequestMissionADS(Token, false);
        }
    }
    return FReply::Handled();
}

FReply SBaddieCompanionMissionPanel::OnAnalogValueChanged(const FGeometry& Geometry, const FAnalogInputEvent& Event)
{
    if (View.bModal) return FReply::Handled();
    const float Raw = Event.GetAnalogValue();
    if (!FMath::IsFinite(Raw)) return FReply::Handled();
    const float Value = FMath::Abs(Raw) < 0.15f ? 0 : FMath::Clamp(Raw, -1.0f, 1.0f);
    const FKey Key = Event.GetKey();
    if (Key == EKeys::Gamepad_LeftX) StickMove.X = Value;
    if (Key == EKeys::Gamepad_LeftY) StickMove.Y = Value;
    if (Key == EKeys::Gamepad_RightX) StickLook.X = Value;
    if (Key == EKeys::Gamepad_RightY) StickLook.Y = -Value;
    if (Key == EKeys::Gamepad_LeftX || Key == EKeys::Gamepad_LeftY)
        if (auto* Controller = Owner.Get()) Controller->RequestMissionMove(View.Token, StickMove);
    return FReply::Handled();
}
