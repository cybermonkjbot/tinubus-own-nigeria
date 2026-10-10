#pragma once

#include "CoreMinimal.h"
#include "BaddieCompanionMissionPresentation.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class ABaddieCompanionSlicePlayerController;
class SBaddieCompanionAudioCompass;
class SButton;
class SBorder;

class SBaddieCompanionMissionPanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SBaddieCompanionMissionPanel) {}
        SLATE_ARGUMENT(TWeakObjectPtr<ABaddieCompanionSlicePlayerController>, Owner)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args);
    void Present(const BaddieCompanion::FMissionHudView& Next, double ListenerYawDegrees);
    bool AddHeardCue(uint64 Generation, uint8 WorldSector, double RemainingSeconds);
    void Clear();
    void CancelPointers();
    void HandleControlCaptureLost();
    virtual bool SupportsKeyboardFocus() const override { return true; }
    virtual void Tick(const FGeometry& Geometry, double CurrentTime, float DeltaTime) override;
    virtual FReply OnTouchStarted(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FReply OnTouchMoved(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FReply OnTouchEnded(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FReply OnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FReply OnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FReply OnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
    virtual FReply OnKeyUp(const FGeometry& Geometry, const FKeyEvent& Event) override;
    virtual FReply OnAnalogValueChanged(const FGeometry& Geometry, const FAnalogInputEvent& Event) override;
    virtual void OnMouseCaptureLost(const FCaptureLostEvent& Event) override;

private:
    enum class EPointerOwner : uint8 { Move, Look, Brush };
    struct FPointer
    {
        EPointerOwner Kind = EPointerOwner::Look;
        uint32 UserIndex = 0;
        FVector2D Origin = FVector2D::ZeroVector;
        FVector2D Last = FVector2D::ZeroVector;
        BaddieCompanion::FMissionPresentationToken Token;
        BaddieCompanion::FMissionSelectedAction Selection;
    };
    FReply BeginPointer(const FGeometry& Geometry, const FPointerEvent& Event);
    FReply MovePointer(const FGeometry& Geometry, const FPointerEvent& Event);
    FReply EndPointer(const FGeometry& Geometry, const FPointerEvent& Event);
    void BeginControl();
    void UpdateKeyboardMove();
    FText ActionText() const;
    float ActionWidth() const;
    FMargin ActionOffset() const;
    FReply OpenMenu();

    TWeakObjectPtr<ABaddieCompanionSlicePlayerController> Owner;
    BaddieCompanion::FMissionHudView View;
    TSharedPtr<SBaddieCompanionAudioCompass> Compass;
    TSharedPtr<SButton> ActionButton;
    TSharedPtr<SButton> FireButton;
    TSharedPtr<SButton> AimButton;
    TSharedPtr<SBorder> Modal;
    TArray<TSharedPtr<SWidget>> CapturingControls;
    TMap<uint32, FPointer> Pointers;
    TSet<FKey> MoveKeys;
    FVector2D StickMove = FVector2D::ZeroVector;
    FVector2D StickLook = FVector2D::ZeroVector;
    TOptional<BaddieCompanion::FMissionPresentationToken> FirePressToken;
    TOptional<BaddieCompanion::FMissionPresentationToken> ADSPressToken;
    TOptional<BaddieCompanion::FMissionHudView> ActionPressView;
    double ActionAppearedAt = 0;
    bool bCancellingPointers = false;
};
