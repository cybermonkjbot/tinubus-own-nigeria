#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BaddieCompanionSliceHUD.generated.h"

UCLASS(NotBlueprintable)
class BADDIECOMPANION_API ABaddieCompanionSliceHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;
};
