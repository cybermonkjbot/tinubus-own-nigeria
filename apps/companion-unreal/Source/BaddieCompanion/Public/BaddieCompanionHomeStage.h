#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaddieCompanionHomeStage.generated.h"

/** Neutral offline inspection stage, not an imported or substituted saved Home layout. */
UCLASS(NotBlueprintable)
class BADDIECOMPANION_API ABaddieCompanionHomeStage : public AActor
{
    GENERATED_BODY()

public:
    ABaddieCompanionHomeStage();
    static constexpr float HalfExtentCm = 300.0f;
};
