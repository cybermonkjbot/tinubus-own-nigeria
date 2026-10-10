#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BaddieCompanionSliceGameMode.generated.h"

class ABaddieCompanionHomeStage;
class ABaddieCompanionSliceCharacter;

UCLASS(NotBlueprintable)
class BADDIECOMPANION_API ABaddieCompanionSliceGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ABaddieCompanionSliceGameMode();
    virtual void StartPlay() override;

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void ApplySelection();
    UPROPERTY()
    TObjectPtr<ABaddieCompanionHomeStage> Stage;
    UPROPERTY()
    TObjectPtr<ABaddieCompanionSliceCharacter> InspectionCharacter;
    bool bOfflinePreviewEnabled = false;
};
