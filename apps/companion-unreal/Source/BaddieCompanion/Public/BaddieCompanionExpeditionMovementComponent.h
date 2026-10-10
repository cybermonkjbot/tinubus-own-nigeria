#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "BaddieCompanionExpeditionMovementComponent.generated.h"

/** Checks the encounter projection before native physics. No custom physics or routes. */
UCLASS(NotBlueprintable, Transient)
class BADDIECOMPANION_API UBaddieCompanionExpeditionMovementComponent : public UCharacterMovementComponent
{
    GENERATED_BODY()

public:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;
};
