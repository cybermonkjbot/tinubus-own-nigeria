#pragma once

#include "CoreMinimal.h"
#include "BaddieCompanionSession.h"
#include "BaddieCompanionGameplaySession.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Layout/SBox.h"

class ABaddieCompanionSlicePlayerController;
enum class EBaddieConnectionAction : uint8;
enum class EBaddieGameplayConnectionAction : uint8;
class SButton;
class SBorder;
class SEditableTextBox;
class SScrollBox;
class STextBlock;
namespace BaddieCompanion { struct FHomePresentationPlan; }

/** One client-only connection/Home surface. No credentials, snapshot JSON, IDs or asset creation. */
class SBaddieCompanionConnectionPanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SBaddieCompanionConnectionPanel) {}
        SLATE_ARGUMENT(TWeakObjectPtr<ABaddieCompanionSlicePlayerController>, Owner)
    SLATE_END_ARGS()

    void Construct(const FArguments& Args);
    void PresentConnection(const BaddieCompanion::FConnectionView& View,
        const BaddieCompanion::FSnapshotSelection* Selection,
        const BaddieCompanion::FHomePresentationPlan* Presentation = nullptr);
    void PresentOffline(const BaddieCompanion::FSnapshotSelection* Selection,
        const FString& Status, bool bCaptured);
    void PresentGameplay(const BaddieCompanion::FGameplayConnectionView& View, uint64 Revision);
    void ClearGameplayPresentation();
    void ClearPrivatePresentation();
    void DetachOwner();
    void ShowConfirmation(EBaddieConnectionAction Action, BaddieCompanion::EConnectionScope Scope);
    void ShowGameplayConfirmation(EBaddieGameplayConnectionAction Action);
    void DismissConfirmation();
    virtual bool SupportsKeyboardFocus() const override { return true; }
    virtual FReply OnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
    virtual FReply OnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
    virtual void Tick(const FGeometry& Geometry, double CurrentTime, float DeltaTime) override;

private:
    TSharedRef<STextBlock> MakeText(TSharedPtr<STextBlock>& Target, int32 Size = 18);
    TSharedRef<SButton> MakeButton(TSharedPtr<SButton>& Target, const TCHAR* Label, EBaddieConnectionAction Action);
    TSharedRef<SButton> MakeGameplayButton(TSharedPtr<SButton>& Target, const TCHAR* Label, EBaddieGameplayConnectionAction Action);
    FReply InvokeAction(EBaddieConnectionAction Action);
    void SetText(const TSharedPtr<STextBlock>& Target, const FString& Text);
    void SetVisible(const TSharedPtr<SWidget>& Target, bool bVisible);
    void FocusDefault();
    void Focus(const TSharedPtr<SWidget>& Target);
    bool IsFocusableControl(const TSharedPtr<SWidget>& Target) const;
    bool CanConnect() const;
    bool ValidDeviceName() const;
    bool CanUseGameplayControls() const;
    bool ValidGameplayDeviceName() const;
    FOptionalSize PanelWidth() const;
    FOptionalSize ModalHeight() const;
    float TextWidth() const;

    TWeakObjectPtr<ABaddieCompanionSlicePlayerController> Owner;
    TSharedPtr<SBorder> MainSurface, ModalSurface;
    TSharedPtr<SScrollBox> MainScroll, ModalScroll;
    TSharedPtr<SEditableTextBox> DeviceName;
    TSharedPtr<SEditableTextBox> GameplayDeviceName;
    TSharedPtr<STextBlock> Title, StatusText, DeviceText, DeviceNameError, ApprovalLink, ComparisonCode, Lifetime,
        ErrorText, RetryText, HomeText, CompanionText, ActiveRoomText, UnsupportedText, ModalTitle, ModalBody;
    TSharedPtr<SButton> ConnectButton, ApartmentButton, CopyButton, CancelButton, DisconnectButton,
        RepairButton, RepairApartmentButton, ResumeButton, ConfirmButton, KeepButton;
    TArray<TSharedPtr<SWidget>> Controls;
    TArray<TSharedPtr<SWidget>> GameplayControls;
    TSharedPtr<STextBlock> GameplayTitle, GameplayStatus, GameplayLimits, GameplayDeviceText, GameplayNameError,
        GameplayCode, GameplayLink, GameplayLifetime, GameplayPresence, GameplayError, GameplayRetry;
    TSharedPtr<SButton> GameplayPairButton, GameplayCopyButton, GameplayCancelButton, GameplayEnterButton,
        GameplayCancelEntryButton, GameplayLeaveButton, GameplayDisconnectButton;
    BaddieCompanion::FGameplayConnectionView GameplayView;
    uint64 GameplayRevision = 0, ConfirmationRevision = 0;
    TWeakPtr<SWidget> ReturnFocus;
    BaddieCompanion::EConnectionState State = BaddieCompanion::EConnectionState::Disabled;
    double RetrySeconds = 0;
    bool bOffline = false, bHasSelection = false, bModal = false, bFocusDefault = true, bWasCaptured = false;
    bool bPreferGameplayFocus = false;
};
