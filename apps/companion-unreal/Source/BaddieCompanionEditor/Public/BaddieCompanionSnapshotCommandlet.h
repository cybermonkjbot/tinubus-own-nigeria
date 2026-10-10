#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "BaddieCompanionSnapshotCommandlet.generated.h"

/** Offline snapshot inspection only. No sign-in, networking, imports or saved-state writes. */
UCLASS()
class BADDIECOMPANIONEDITOR_API UBaddieCompanionSnapshotCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    explicit UBaddieCompanionSnapshotCommandlet(const FObjectInitializer& ObjectInitializer);
    virtual int32 Main(const FString& Params) override;
};
