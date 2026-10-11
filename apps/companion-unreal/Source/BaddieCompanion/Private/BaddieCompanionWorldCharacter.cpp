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
    FWorldPlayerRead PlayerRead;
    FWorldInstanceRead InstanceRead;
    FWorldPlayerHeader Player;
    FWorldInstanceHeader Instance;
    FWorldPresentationTime Time;
    if (!View.bWalkingAvailable || View.Observation.ReadPlayer(PlayerRead) != EWorldReadResult::Ready
        || PlayerRead.Header(Player) != EWorldReadResult::Ready
        || View.Observation.ReadInstance(InstanceRead) != EWorldReadResult::Ready
        || InstanceRead.Header(Instance) != EWorldReadResult::Ready || !View.Observation.CopyTime(Time)
        || Time.RemainingMs <= 0 || Instance.Relation != EWorldInstanceRelation::CurrentVisit
        || !Instance.bPlayerInstanceMatches || Player.bControlUnavailable
        || Player.Driving == EWorldFieldPresence::Value || Player.Water == EWorldFieldPresence::Value
        || Player.Controls.bConflictingTransport || Player.Controls.bMissingControlFields
        || Player.Controls.bRecovering || Player.Controls.bTraveling || Player.Controls.bTransit
        || Player.Controls.bDanceActivity || Player.Controls.bLoungeSeat)
    { ClearWorldMovement(); return false; }
    if (Observation.Token() != View.Observation.Token())
    {
        ClearWorldMovement();
        const auto Current = Session.GetWorldView();
        if (!View.Observation.IsCurrent() || Current.Observation.Token() != View.Observation.Token()
            || !Current.bWalkingAvailable || Observation.Token().IsSet()) return false;
        Observation = View.Observation;
    }
    const auto Token = Observation.Token();
    if (Player.Movement != EWorldFieldPresence::Value)
    {
        GetCharacterMovement()->StopMovementImmediately();
        const auto Current = Session.GetWorldView();
        if (!Observation.IsCurrent() || Observation.Token() != Token
            || Current.Observation.Token() != Token || !Current.bWalkingAvailable) return false;
        LastRouteSequence = -1; return true;
    }
    // Only accepted server-time windows are sampled. No desired-position planner,
    // custom gait, physics simulation or local route prediction is introduced.
    if (LastRouteSequence != Player.MoveSequence)
    { LastSampleMs = Time.ServerNowMs; LastRouteSequence = Player.MoveSequence; }
    FMovementPose Window[34];
    uint32 Count = 0;
    if (Observation.ReadMovementWindow(LastSampleMs, Time.ServerNowMs, Window, Count) != EWorldReadResult::Ready)
    { ClearWorldMovement(); return false; }
    for (uint32 Index = 0; Index < Count; ++Index)
    {
        const auto& Pose = Window[Index];
        const auto BeforeMove = Session.GetWorldView();
        if (!Observation.IsCurrent() || Observation.Token() != Token
            || BeforeMove.Observation.Token() != Token || !BeforeMove.bWalkingAvailable)
        { ClearWorldMovement(); return false; }
        const FVector Location = SourceToUnreal(FVector(Pose.X, Instance.FloorY, Pose.Z))
            + FVector(0, 0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
        const FQuat Facing = FRotator(0, SourceYawToUnreal(Pose.YawRadians), 0).Quaternion();
        FHitResult Hit;
        GetCharacterMovement()->SafeMoveUpdatedComponent(Location - GetActorLocation(), Facing, true, Hit);
        const auto Current = Session.GetWorldView();
        if (!Observation.IsCurrent() || Observation.Token() != Token
            || Current.Observation.Token() != Token || !Current.bWalkingAvailable) return false;
        if (Hit.IsValidBlockingHit())
        {
            GetCharacterMovement()->StopMovementImmediately();
            const auto AfterStop = Session.GetWorldView();
            if (!Observation.IsCurrent() || Observation.Token() != Token
                || AfterStop.Observation.Token() != Token || !AfterStop.bWalkingAvailable) return false;
            if (auto* Controller = Cast<APlayerController>(GetController())) Session.StopWorldWalk(Controller);
            return false;
        }
    }
    if (!Observation.IsCurrent() || Observation.Token() != Token
        || Session.GetWorldView().Observation.Token() != Token) return false;
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
