#include "BaddieCompanionExpeditionMovementComponent.h"

#include "BaddieCompanionExpeditionCharacter.h"

void UBaddieCompanionExpeditionMovementComponent::TickComponent(float DeltaTime,
    ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    auto* Expedition = Cast<ABaddieCompanionExpeditionCharacter>(GetOwner());
    if (!IsValid(Expedition))
    {
        StopMovementImmediately();
        DisableMovement();
        return;
    }
    Expedition->PrepareAcceptedMovement();
    const FVector OldLocation = Expedition->GetActorLocation();
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (IsValid(Expedition)) Expedition->FinishAcceptedMovement(DeltaTime, OldLocation);
}
