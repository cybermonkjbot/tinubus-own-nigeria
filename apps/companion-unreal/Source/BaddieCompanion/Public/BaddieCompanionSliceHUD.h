#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BaddieCompanionMissionPresentation.h"
#include "BaddieCompanionSliceHUD.generated.h"

class SBaddieCompanionMissionPanel;
class SWidget;

UCLASS(NotBlueprintable)
class BADDIECOMPANION_API ABaddieCompanionSliceHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;
    bool PresentMission(const BaddieCompanion::FMissionHudView& View, double ListenerYawDegrees);
    bool AddMissionHeardCue(uint64 Generation, uint8 WorldSector, double RemainingSeconds);
    void ClearMission();
    void CancelMissionPointers();
    bool HasMissionFocus() const;
    TSharedPtr<SWidget> GetMissionFocusWidget() const;
protected:
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    TSharedPtr<SBaddieCompanionMissionPanel> MissionPanel;
    TSharedPtr<SWidget> MissionViewportPanel;
};
