#include "BaddieCompanionWorldCharacter.h"
#include "BaddieCompanionGameplaySession.h"
#include "BaddieCompanionHomePresentation.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"

ABaddieCompanionWorldCharacter::ABaddieCompanionWorldCharacter()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = false; SetReplicateMovement(false);
    AutoPossessPlayer = EAutoReceiveInput::Disabled;
    AutoPossessAI = EAutoPossessAI::Disabled;
    SetActorHiddenInGame(true); SetActorEnableCollision(false);
    GetCharacterMovement()->DisableMovement();
    CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("WorldCameraArm"));
    CameraArm->SetupAttachment(GetRootComponent());
    CameraArm->bUsePawnControlRotation = true;
    WorldCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("WorldCamera"));
    WorldCamera->SetupAttachment(CameraArm, USpringArmComponent::SocketName);
    WorldCamera->SetAutoActivate(false);
}

bool ABaddieCompanionWorldCharacter::FollowAccepted(UBaddieCompanionGameplaySession& Session)
{
    using namespace BaddieCompanion;
    const auto View = Session.GetWorldView();
    TSharedPtr<const FWorldPlayerView> Player;
    TSharedPtr<const FWorldCurrentInstanceIdentity> Instance;
    FWorldPresentationTime Time;
    if (!View.bWalkingAvailable || !View.Observation.CopyPlayer(Player)
        || !View.Observation.CopyInstance(Instance) || !View.Observation.CopyTime(Time)
        || Time.RemainingMs <= 0 || Instance->Relation != EWorldInstanceRelation::CurrentVisit
        || !Player->InstanceId.Equals(Instance->Id, ESearchCase::CaseSensitive)
        || Player->Driving.Value.IsSet() || Player->WaterControl.Value.IsSet()
        || Player->Controls.bConflictingTransport || Player->Controls.bMissingControlFields
        || Player->Controls.bRecovering || Player->Controls.bTraveling || Player->Controls.bTransit
        || Player->Controls.bDanceActivity || Player->Controls.bLoungeSeat)
    { ClearWorldMovement(); return false; }
    if (Observation.Generation() != View.Observation.Generation())
    { ClearWorldMovement(); Observation = View.Observation; }
    if (!Player->Movement.IsSet())
    { GetCharacterMovement()->StopMovementImmediately(); LastRouteSequence = -1; return true; }
    const auto& Route = Player->Movement.GetValue();
    if (Route.RoomRevision != Player->Revision || Route.Sequence != Player->MoveSequence)
    { ClearWorldMovement(); return false; }
    // Only accepted server-time windows are sampled. No desired-position planner,
    // custom gait, physics simulation or local route prediction is introduced.
    if (LastRouteSequence != Route.Sequence)
    { LastSampleMs = Time.ServerNowMs; LastRouteSequence = Route.Sequence; }
    TArray<FMovementPose> Window;
    if (!MovementWindow(Route, LastSampleMs, Time.ServerNowMs, Window))
    { ClearWorldMovement(); return false; }
    for (const auto& Pose : Window)
    {
        if (!Observation.IsCurrent() || !Session.GetWorldView().bWalkingAvailable)
        { ClearWorldMovement(); return false; }
        const FVector Location = SourceToUnreal(FVector(Pose.X, Instance->Navigation.FloorY, Pose.Z))
            + FVector(0, 0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
        const FQuat Facing = FRotator(0, SourceYawToUnreal(Pose.YawRadians), 0).Quaternion();
        FHitResult Hit;
        GetCharacterMovement()->SafeMoveUpdatedComponent(Location - GetActorLocation(), Facing, true, Hit);
        if (Hit.IsValidBlockingHit())
        {
            GetCharacterMovement()->StopMovementImmediately();
            if (auto* Controller = Cast<APlayerController>(GetController())) Session.StopWorldWalk(Controller);
            return false;
        }
    }
    LastSampleMs = Time.ServerNowMs;
    return true;
}

void ABaddieCompanionWorldCharacter::ClearWorldMovement()
{
    Observation = {}; LastSampleMs = 0; LastRouteSequence = -1;
    ConsumeMovementInputVector();
    GetCharacterMovement()->StopMovementImmediately(); GetCharacterMovement()->DisableMovement();
    SetActorHiddenInGame(true); SetActorEnableCollision(false);
    WorldCamera->Deactivate();
}

void ABaddieCompanionWorldCharacter::EndPlay(const EEndPlayReason::Type Reason)
{
    ClearWorldMovement();
    Super::EndPlay(Reason);
}
