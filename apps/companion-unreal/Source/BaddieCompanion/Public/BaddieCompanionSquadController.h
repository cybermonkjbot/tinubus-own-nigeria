#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "BaddieCompanionSquadController.generated.h"

namespace BaddieCompanion
{
    enum class ESquadOrderState : uint8 { Idle, Moving, Arrived, Held, Unreachable, Interrupted };
    struct FSquadOrderView
    {
        ESquadOrderState State = ESquadOrderState::Idle;
        TOptional<FVector> DestinationCm;
    };
}

/** Native path-following for one encounter-owned squad member. Not assigned to the live/offline player.
 * The admitted mission owner must validate squad ownership and destinations before issuing orders,
 * then clear orders on encounter/access loss. This controller grants no backend gameplay authority.
 */
UCLASS(NotBlueprintable, Transient)
class BADDIECOMPANION_API ABaddieCompanionSquadController : public AAIController
{
    GENERATED_BODY()
public:
    ABaddieCompanionSquadController();
    // Destination is already resolved by mission input in UE world cm; no automatic goal projection.
    // Acceptance radius is authored per unit. No partial route, teleport, attack or mission advancement.
    bool OrderPosition(const FVector& DestinationCm, float AcceptanceRadiusCm);
    bool HoldPosition();
    void ClearOrders();
    const BaddieCompanion::FSquadOrderView& GetOrderView() const { return OrderView; }
protected:
    virtual void OnMoveCompleted(FAIRequestID RequestId, const FPathFollowingResult& Result) override;
    virtual void OnUnPossess() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    bool CanControlPawn() const;
    bool CanAcceptOrder() const;

    FAIRequestID ActiveRequest = FAIRequestID::InvalidRequest;
    TWeakObjectPtr<APawn> OrderedPawn;
    BaddieCompanion::FSquadOrderView OrderView;
    uint64 OrderGeneration = 0;
    bool bChangingOrder = false;
    bool bEndingPlay = false;
};
