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
        const bool bTransfer = Session.WorldTransferCurrent(Cast<APlayerController>(GetController()),
            Observation.Token(), View.Observation.Token());
        if (!bTransfer) ClearWorldMovement();
        const auto Current = Session.GetWorldView();
        if (!View.Observation.IsCurrent() || Current.Observation.Token() != View.Observation.Token()
            || !Current.bWalkingAvailable || (!bTransfer && Observation.Token().IsSet())) return false;
        Observation = View.Observation;
    }
    const auto Token = Observation.Token();
    if (Observation.AdmitNativeMovementWork(false) != EWorldReadResult::Ready) return false;
    if (Player.Movement != EWorldFieldPresence::Value)
    {
        GetCharacterMovement()->StopMovementImmediately();
        const auto Current = Session.GetWorldView();
        if (!Observation.IsCurrent() || Observation.Token() != Token
            || Current.Observation.Token() != Token || !Current.bWalkingAvailable) return false;
        LastRouteSequence = -1; bHasRouteIdentity = false; RouteIdentity = {}; return true;
    }
    // Only accepted server-time windows are sampled. No desired-position planner,
    // custom gait, physics simulation or local route prediction is introduced.
    WorldObservationPrivate::FMovementIdentity AcceptedRoute;
    if (Observation.ReadMovementIdentity(AcceptedRoute) != EWorldReadResult::Ready)
    { ClearWorldMovement(); return false; }
    if (!bHasRouteIdentity || !RouteIdentity.Same(AcceptedRoute))
    {
        RouteIdentity = AcceptedRoute; bHasRouteIdentity = true;
        LastSampleMs = AcceptedRoute.StartedAtMs; LastCommittedPose = AcceptedRoute.From;
        LastRouteSequence = AcceptedRoute.Sequence;
        // A new route starts at its validated origin. Never sweep from an old
        // route or silently skip its first corner during publication adoption.
        const FVector Origin = SourceToUnreal(FVector(LastCommittedPose.X, Instance.FloorY, LastCommittedPose.Z))
            + FVector(0, 0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
        SetActorLocationAndRotation(Origin, FRotator(0, SourceYawToUnreal(LastCommittedPose.YawRadians), 0),
            false, nullptr, ETeleportType::TeleportPhysics);
        if (!Observation.IsCurrent() || Observation.Token() != Token
            || Session.GetWorldView().Observation.Token() != Token) return false;
    }
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
            FMovementCollisionStop Receipt;
            FMovementPose Origin;
            const bool bReceipt = Observation.ReadCollisionStop(RouteIdentity, LastSampleMs,
                Time.ServerNowMs, Receipt, Origin) == EWorldReadResult::Ready;
            const FVector Committed = SourceToUnreal(FVector(LastCommittedPose.X, Instance.FloorY, LastCommittedPose.Z))
                + FVector(0, 0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
            SetActorLocationAndRotation(Committed, FRotator(0, SourceYawToUnreal(LastCommittedPose.YawRadians), 0),
                false, nullptr, ETeleportType::TeleportPhysics);
            if (!Observation.IsCurrent() || Observation.Token() != Token
                || Session.GetWorldView().Observation.Token() != Token) return false;
            GetCharacterMovement()->StopMovementImmediately();
            const auto AfterStop = Session.GetWorldView();
            if (!Observation.IsCurrent() || Observation.Token() != Token
                || AfterStop.Observation.Token() != Token || !AfterStop.bWalkingAvailable) return false;
            if (auto* Controller = Cast<APlayerController>(GetController()))
            {
                if (bReceipt) Session.WorldCollisionStop(Controller, Token, Receipt, Origin);
                else Session.StopWorldWalk(Controller);
            }
            return false;
        }
    }
    if (!Observation.IsCurrent() || Observation.Token() != Token
        || Session.GetWorldView().Observation.Token() != Token) return false;
    LastSampleMs = Time.ServerNowMs;
    if (Count) LastCommittedPose = Window[Count - 1];
    return true;
}

void ABaddieCompanionWorldCharacter::ClearWorldMovement()
{
    Observation = {}; LastSampleMs = 0; LastRouteSequence = -1;
    RouteIdentity = {}; LastCommittedPose = {}; bHasRouteIdentity = false;
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
