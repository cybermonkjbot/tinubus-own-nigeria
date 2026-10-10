#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BaddieCompanionSlicePlayerController.generated.h"

UCLASS(NotBlueprintable)
class BADDIECOMPANION_API ABaddieCompanionSlicePlayerController : public APlayerController
{
    GENERATED_BODY()

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;

private:
    void TogglePointerCapture();
    void ApplyPointerCapture();
    bool bPointerCaptured = true;
};
