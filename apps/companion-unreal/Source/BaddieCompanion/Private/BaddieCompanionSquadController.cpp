#include "BaddieCompanionSquadController.h"

#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavFilters/NavigationQueryFilter.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "Templates/UnrealTemplate.h"

ABaddieCompanionSquadController::ABaddieCompanionSquadController()
{
    bReplicates = false;
    // Keep native controller/path-following ticks. No additional movement loop or scheduler.
}

bool ABaddieCompanionSquadController::CanControlPawn() const
{
    const auto* World = GetWorld();
    const auto* Character = Cast<ACharacter>(GetPawn());
    return !bEndingPlay && HasAuthority() && World && World->IsGameWorld()
        && World->GetNetMode() == NM_Standalone && IsValid(Character)
        && Character->GetController() == this && IsValid(Character->GetCharacterMovement())
        && Character->GetCharacterMovement()->MovementMode != MOVE_None
        && !Character->GetNavAgentLocation().ContainsNaN()
        && IsValid(GetPathFollowingComponent()) && !GetBrainComponent();
}

bool ABaddieCompanionSquadController::CanAcceptOrder() const
{
    if (!CanControlPawn()) return false;
    const auto* Following = GetPathFollowingComponent();
    // Do not take over a Behavior Tree or an unrelated native move request.
    return Following->GetStatus() == EPathFollowingStatus::Idle
        || (ActiveRequest.IsValid() && OrderedPawn.Get() == GetPawn()
            && Following->GetCurrentRequestId() == ActiveRequest);
}

void ABaddieCompanionSquadController::ClearOrders()
{
    TGuardValue<bool> Changing(bChangingOrder, true);
    ++OrderGeneration;
    const auto PreviousRequest = ActiveRequest;
    const bool bSamePawn = OrderedPawn.Get() == GetPawn();
    ActiveRequest = FAIRequestID::InvalidRequest;
    OrderedPawn.Reset();
    OrderView = {};
    // Invalidate first because AbortMove can deliver completion synchronously.
    const auto* Following = GetPathFollowingComponent();
    if (bSamePawn && PreviousRequest.IsValid() && Following
        && Following->GetCurrentRequestId() == PreviousRequest) Super::StopMovement();
}

bool ABaddieCompanionSquadController::OrderPosition(const FVector& DestinationCm, float AcceptanceRadiusCm)
{
    if (bChangingOrder || !CanAcceptOrder() || DestinationCm.ContainsNaN()
        || !FMath::IsFinite(AcceptanceRadiusCm) || AcceptanceRadiusCm <= 0) return false;
    auto* Navigation = UNavigationSystemV1::GetCurrent(GetWorld());
    const auto& Agent = GetNavAgentPropertiesRef();
    const auto* NavData = Navigation ? Navigation->GetNavDataForProps(Agent, GetNavAgentLocation()) : nullptr;
    if (!NavData) return false;
    const auto FilterClass = GetDefaultNavigationFilterClass();
    const auto Filter = UNavigationQueryFilter::GetQueryFilter(*NavData, this, FilterClass);
    FNavLocation Navigable;
    // MoveTo's AlreadyAtGoal branch runs before pathfinding. Validate the endpoint even then.
    // This check never replaces the supplied goal with a point on a different bank or floor.
    if (!Filter.IsValid()
        || !Navigation->ProjectPointToNavigation(DestinationCm, Navigable, FVector(2, 2, 2), NavData, Filter)
        || !Navigable.Location.Equals(DestinationCm, 2)) return false;
    TGuardValue<bool> Changing(bChangingOrder, true);
    const uint64 Generation = OrderGeneration + 1;
    ClearOrders();
    if (OrderGeneration != Generation || !CanAcceptOrder()) return false;
    auto* Pawn = GetPawn();
    OrderView.DestinationCm = DestinationCm;

    FAIMoveRequest Request(DestinationCm);
    Request.SetAcceptanceRadius(AcceptanceRadiusCm);
    Request.SetUsePathfinding(true);
    Request.SetAllowPartialPath(false);
    Request.SetRequireNavigableEndLocation(true);
    Request.SetProjectGoalLocation(false);
    Request.SetReachTestIncludesAgentRadius(false);
    Request.SetReachTestIncludesGoalRadius(false);
    Request.SetStartFromPreviousPath(false);
    Request.SetNavigationFilter(FilterClass);
    const auto Result = Super::MoveTo(Request);

    // Native completion listeners may revoke the encounter or unpossess during the request.
    if (OrderGeneration != Generation || GetPawn() != Pawn || !CanControlPawn())
    {
        if (Result.MoveId.IsValid() && GetCurrentMoveRequestID() == Result.MoveId) Super::StopMovement();
        OrderView = {};
        return false;
    }
    using BaddieCompanion::ESquadOrderState;
    if (Result.Code == EPathFollowingRequestResult::AlreadyAtGoal)
    {
        OrderView.State = ESquadOrderState::Arrived;
        return true;
    }
    if (Result.Code != EPathFollowingRequestResult::RequestSuccessful || !Result.MoveId.IsValid())
    {
        OrderView.State = ESquadOrderState::Unreachable;
        return false;
    }
    if (GetCurrentMoveRequestID() != Result.MoveId)
    {
        OrderView.State = ESquadOrderState::Interrupted;
        return false;
    }
    ActiveRequest = Result.MoveId;
    OrderedPawn = Pawn;
    OrderView.State = ESquadOrderState::Moving;
    return true;
}

bool ABaddieCompanionSquadController::HoldPosition()
{
    if (bChangingOrder || !CanAcceptOrder()) return false;
    TGuardValue<bool> Changing(bChangingOrder, true);
    const uint64 Generation = OrderGeneration + 1;
    ClearOrders();
    if (OrderGeneration != Generation || !CanAcceptOrder()) return false;
    OrderView.State = BaddieCompanion::ESquadOrderState::Held;
    return true;
}

void ABaddieCompanionSquadController::OnMoveCompleted(FAIRequestID RequestId, const FPathFollowingResult& Result)
{
    if (ActiveRequest.IsValid() && RequestId == ActiveRequest && OrderedPawn.Get() == GetPawn())
    {
        ActiveRequest = FAIRequestID::InvalidRequest;
        OrderedPawn.Reset();
        using BaddieCompanion::ESquadOrderState;
        OrderView.State = Result.Code == EPathFollowingResult::Success ? ESquadOrderState::Arrived
            : Result.Code == EPathFollowingResult::Aborted ? ESquadOrderState::Interrupted
            : ESquadOrderState::Unreachable;
    }
    Super::OnMoveCompleted(RequestId, Result);
}

void ABaddieCompanionSquadController::OnUnPossess()
{
    TGuardValue<bool> Changing(bChangingOrder, true);
    ClearOrders();
    Super::OnUnPossess();
}

void ABaddieCompanionSquadController::EndPlay(const EEndPlayReason::Type Reason)
{
    bEndingPlay = true;
    ClearOrders();
    Super::EndPlay(Reason);
}
