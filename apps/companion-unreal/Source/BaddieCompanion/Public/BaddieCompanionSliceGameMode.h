#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BaddieCompanionSliceGameMode.generated.h"

class ABaddieCompanionHomeStage;

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
    bool bOfflinePreviewEnabled = false;
};
