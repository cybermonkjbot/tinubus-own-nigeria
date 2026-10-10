#include "BaddieCompanionExpeditionCharacter.h"

#include "BaddieCompanionExpeditionMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Templates/UnrealTemplate.h"

namespace BaddieExpeditionCharacterPrivate
{
    bool ValidIdentity(const FString& Value)
    {
        return !Value.IsEmpty() && Value.Len() <= 240 && Value.TrimStartAndEnd() == Value;
    }

    bool SameIdentity(const BaddieCompanion::FExpeditionAcceptedPresentation& A,
        const BaddieCompanion::FExpeditionAcceptedPresentation& B)
    {
        return A.MissionRunId.Equals(B.MissionRunId, ESearchCase::CaseSensitive)
            && A.CharacterId.Equals(B.CharacterId, ESearchCase::CaseSensitive)
            && A.LifeId.Equals(B.LifeId, ESearchCase::CaseSensitive)
            && A.InstanceId.Equals(B.InstanceId, ESearchCase::CaseSensitive);
    }

    bool ValidPresentation(const BaddieCompanion::FExpeditionAcceptedPresentation& State,
        float CapsuleHalfHeight)
    {
        using BaddieCompanion::EExpeditionTerrain;
        if (!ValidIdentity(State.MissionRunId) || !ValidIdentity(State.CharacterId)
            || !ValidIdentity(State.LifeId) || !ValidIdentity(State.InstanceId)
            || !FMath::IsFinite(State.MaxGroundSpeedCmPerSecond) || State.MaxGroundSpeedCmPerSecond < 0
            || !FMath::IsFinite(State.MaxGroundSpeedCmPerSecond * State.MaxGroundSpeedCmPerSecond)
            || State.AcceptedVelocityCmPerSecond.ContainsNaN() || State.AcceptedVelocityCmPerSecond.Z != 0
            || !FMath::IsFinite(State.AcceptedVelocityCmPerSecond.SizeSquared())
            || State.AcceptedVelocityCmPerSecond.Size() > State.MaxGroundSpeedCmPerSecond
            || !FMath::IsFinite(CapsuleHalfHeight) || !FMath::IsFinite(State.EyeHeightCm) || State.EyeHeightCm <= 0
            || State.EyeHeightCm >= CapsuleHalfHeight
            || !FMath::IsFinite(State.LocomotionNoiseGain) || State.LocomotionNoiseGain < 0) return false;
        return State.Terrain == EExpeditionTerrain::Ground
            || State.Terrain == EExpeditionTerrain::Mud || State.Terrain == EExpeditionTerrain::Wading;
    }
}

ABaddieCompanionExpeditionCharacter::ABaddieCompanionExpeditionCharacter(
    const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UBaddieCompanionExpeditionMovementComponent>(
        ACharacter::CharacterMovementComponentName))
{
    SetReplicates(false);
    SetReplicateMovement(false);
    AutoPossessPlayer = EAutoReceiveInput::Disabled;
    AutoPossessAI = EAutoPossessAI::Disabled;
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = true;
    bUseControllerRotationRoll = false;
    auto* Movement = GetCharacterMovement();
    Movement->bOrientRotationToMovement = false;
    Movement->bRunPhysicsWithNoController = false;
    // The accepted sampler already supplies acceleration/deceleration. Native
    // requested velocity retains collision/floor handling without a second ramp.
    Movement->bRequestedMoveUseAcceleration = false;
    Movement->DefaultLandMovementMode = MOVE_None;
    Movement->DefaultWaterMovementMode = MOVE_None;
    Movement->GetNavAgentPropertiesRef().bCanSwim = false;
    Movement->MaxWalkSpeed = 0;
    Movement->MaxWalkSpeedCrouched = 0;
    Movement->DisableMovement();
    SetActorEnableCollision(false);
    SetActorHiddenInGame(true);

    FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ExpeditionEyeCamera"));
    // Unlike the sample's head socket, capsule attachment introduces no mandatory
    // animation bob or roll, and requires no invented Companion skeleton binding.
    FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
    FirstPersonCamera->bUsePawnControlRotation = true;
    FirstPersonCamera->bAutoActivate = false;
    FirstPersonCamera->bEnableFirstPersonFieldOfView = false;
    FirstPersonCamera->bEnableFirstPersonScale = false;
}

void ABaddieCompanionExpeditionCharacter::BeginPlay()
{
    Super::BeginPlay();
    ClearEncounterPresentation();
}

void ABaddieCompanionExpeditionCharacter::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
    BaddieCompanion::FExpeditionAcceptedPresentation State;
    if (ReadCurrentPresentation(State)) ApplyCameraAndStance(State);
    Super::CalcCamera(DeltaTime, OutResult);
    OutResult.Rotation.Roll = 0;
}

FVector ABaddieCompanionExpeditionCharacter::GetPawnViewLocation() const
{
    return HasSource() && AcceptedPresentation.IsSet()
        ? FirstPersonCamera->GetComponentLocation() : Super::GetPawnViewLocation();
}

void ABaddieCompanionExpeditionCharacter::Restart()
{
    Super::Restart();
    ClearEncounterPresentation();
}

void ABaddieCompanionExpeditionCharacter::UnPossessed()
{
    ClearEncounterPresentation();
    Super::UnPossessed();
}

void ABaddieCompanionExpeditionCharacter::EndPlay(const EEndPlayReason::Type Reason)
{
    bEndingPlay = true;
    ClearEncounterPresentation();
    Super::EndPlay(Reason);
}

void ABaddieCompanionExpeditionCharacter::StopNativeMovement()
{
    ConsumeMovementInputVector();
    StopJumping();
    auto* Movement = GetCharacterMovement();
    Movement->StopActiveMovement();
    Movement->StopMovementImmediately();
    Movement->ClearAccumulatedForces();
}

void ABaddieCompanionExpeditionCharacter::ClearEncounterPresentation()
{
    if (bClearingPresentation) return;
    TGuardValue<bool> Clearing(bClearingPresentation, true);
    ++SourceSerial;
    ReadAcceptedPresentation.Unbind();
    SubmitMovementIntent.Unbind();
    PresentationOwner.Reset();
    PresentationController.Reset();
    AcceptedPresentation.Reset();
    AnimationInputs = {};
    bAdsRequested = false;
    StopNativeMovement();
    GetCharacterMovement()->MaxWalkSpeed = 0;
    GetCharacterMovement()->MaxWalkSpeedCrouched = 0;
    GetCharacterMovement()->DefaultLandMovementMode = MOVE_None;
    GetCharacterMovement()->DisableMovement();
    SetActorEnableCollision(false);
    SetActorHiddenInGame(true);
    FirstPersonCamera->Deactivate();
    FirstPersonCamera->SetRelativeLocation(FVector::ZeroVector);
    FirstPersonCamera->SetFieldOfView(WorldFieldOfView);
    // Do not reset a shared controller's rotation or change its view target.
}

bool ABaddieCompanionExpeditionCharacter::HasSource() const
{
    const auto* Player = Cast<APlayerController>(GetController());
    const auto* World = GetWorld();
    return !bEndingPlay && !bClearingPresentation && HasActorBegunPlay() && World && World->IsGameWorld()
        && World->GetNetMode() == NM_Standalone && IsValid(Player) && Player->IsLocalController()
        && !Player->GetControlRotation().ContainsNaN() && !GetActorLocation().ContainsNaN()
        && PresentationController.Get() == Player && PresentationOwner.IsValid()
        && ReadAcceptedPresentation.IsBound() && SubmitMovementIntent.IsBound()
        && GetCharacterMovement()->IsComponentTickEnabled();
}

bool ABaddieCompanionExpeditionCharacter::BindAcceptedEncounterSource(UObject& Owner,
    BaddieCompanion::FReadExpeditionPresentation ReadAccepted,
    BaddieCompanion::FSubmitExpeditionMoveIntent SubmitIntent)
{
    if (bReadingSource || bClearingPresentation || bApplyingMovement || bEndingPlay) return false;
    ClearEncounterPresentation();
    if (!IsValid(&Owner) || Owner.GetWorld() != GetWorld()
        || !ReadAccepted.IsBoundToObject(&Owner) || !SubmitIntent.IsBoundToObject(&Owner)) return false;
    PresentationOwner = &Owner;
    PresentationController = GetController();
    ReadAcceptedPresentation = MoveTemp(ReadAccepted);
    SubmitMovementIntent = MoveTemp(SubmitIntent);
    BaddieCompanion::FExpeditionAcceptedPresentation State;
    if (!ReadCurrentPresentation(State)) return false;
    PrepareAcceptedMovement();
    return HasSource();
}

bool ABaddieCompanionExpeditionCharacter::ReadCurrentPresentation(
    BaddieCompanion::FExpeditionAcceptedPresentation& Out)
{
    Out = {};
    if (bReadingSource) return false;
    if (!HasSource())
    {
        ClearEncounterPresentation();
        return false;
    }
    TGuardValue<bool> Reading(bReadingSource, true);
    const uint64 Serial = SourceSerial;
    // Copy before executing; the owner may synchronously clear/unpossess this pawn.
    const auto Reader = ReadAcceptedPresentation;
    BaddieCompanion::FExpeditionAcceptedPresentation Next;
    const bool bRead = Reader.Execute(*this, Next);
    if (Serial != SourceSerial) return false;
    if (!bRead || !HasSource()
        || !BaddieExpeditionCharacterPrivate::ValidPresentation(Next, GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight())
        || (AcceptedPresentation.IsSet()
            && !BaddieExpeditionCharacterPrivate::SameIdentity(AcceptedPresentation.GetValue(), Next)))
    {
        ClearEncounterPresentation();
        return false;
    }
    AcceptedPresentation = Next;
    Out = MoveTemp(Next);
    return true;
}

void ABaddieCompanionExpeditionCharacter::ApplyCameraAndStance(
    const BaddieCompanion::FExpeditionAcceptedPresentation& State)
{
    const uint64 Serial = SourceSerial;
    if (State.bWeaponLowered || !State.bAdsPresentationAllowed || Controller->IsLookInputIgnored())
        bAdsRequested = false;
    AnimationInputs.bWeaponLowered = State.bWeaponLowered;
    AnimationInputs.bAds = bAdsRequested && !State.bWeaponLowered && State.bAdsPresentationAllowed;
    FirstPersonCamera->SetRelativeLocation(FVector(0, 0, State.EyeHeightCm));
    if (Serial != SourceSerial || !HasSource()) return;
    FirstPersonCamera->SetFieldOfView(AnimationInputs.bAds ? AdsFieldOfView : WorldFieldOfView);
}

void ABaddieCompanionExpeditionCharacter::PrepareAcceptedMovement()
{
    if (bApplyingMovement || bClearingPresentation) return;
    TGuardValue<bool> Applying(bApplyingMovement, true);
    BaddieCompanion::FExpeditionAcceptedPresentation State;
    if (!ReadCurrentPresentation(State)) return;
    const uint64 Serial = SourceSerial;
    auto* Movement = GetCharacterMovement();
    if (Movement->MovementMode != MOVE_None && Movement->MovementMode != MOVE_Walking
        && Movement->MovementMode != MOVE_Falling)
    {
        ClearEncounterPresentation();
        return;
    }
    // Assignment restores normal terrain policy without stacking previous penalties.
    // Native landing must return to walking while this accepted source is current.
    Movement->DefaultLandMovementMode = MOVE_Walking;
    Movement->MaxWalkSpeed = State.MaxGroundSpeedCmPerSecond;
    Movement->MaxWalkSpeedCrouched = State.MaxGroundSpeedCmPerSecond;
    ApplyCameraAndStance(State);
    if (Serial != SourceSerial || !HasSource()) return;
    SetActorEnableCollision(true);
    if (Serial != SourceSerial || !HasSource()) return;
    FirstPersonCamera->Activate();
    if (Serial != SourceSerial || !HasSource()) return;
    // Saved visual content remains under its own owner, hidden by default here.
    if (Movement->MovementMode == MOVE_None) Movement->SetMovementMode(MOVE_Walking);
    if (Serial != SourceSerial || !HasSource()) return;
    ConsumeMovementInputVector();
    // Match RequestDirectMove's native near-zero rejection so a tiny request
    // cannot leave an earlier nonzero requested velocity in place.
    if (State.AcceptedVelocityCmPerSecond.SizeSquared() < UE_KINDA_SMALL_NUMBER)
    {
        Movement->StopActiveMovement();
        // Preserve native vertical falling motion while stopping horizontal motion.
        Movement->Velocity.X = 0;
        Movement->Velocity.Y = 0;
    }
    else
    {
        Movement->RequestDirectMove(State.AcceptedVelocityCmPerSecond, false);
    }
}

void ABaddieCompanionExpeditionCharacter::FinishAcceptedMovement(float DeltaSeconds, const FVector& OldLocation)
{
    BaddieCompanion::FExpeditionAcceptedPresentation State;
    if (!ReadCurrentPresentation(State)) return;
    const uint64 Serial = SourceSerial;
    auto* Movement = GetCharacterMovement();
    if (Movement->MovementMode != MOVE_Walking && Movement->MovementMode != MOVE_Falling)
    {
        ClearEncounterPresentation();
        return;
    }
    ApplyCameraAndStance(State);
    if (Serial != SourceSerial || !HasSource()) return;
    const double Distance = FVector::Dist2D(OldLocation, GetActorLocation());
    const double Speed = Movement->Velocity.Size2D();
    AnimationInputs.GroundSpeedCmPerSecond = FMath::IsFinite(DeltaSeconds) && DeltaSeconds > 0
        && FMath::IsFinite(Distance) && FMath::IsFinite(Speed)
        ? static_cast<float>(FMath::Min(Speed, Distance / DeltaSeconds)) : 0;
    AnimationInputs.bFalling = Movement->IsFalling();
    UpdateLocomotionNoise(State);
}

void ABaddieCompanionExpeditionCharacter::UpdateLocomotionNoise(
    const BaddieCompanion::FExpeditionAcceptedPresentation& State)
{
    AnimationInputs.bLocomotionNoiseActive = GetCharacterMovement()->IsMovingOnGround()
        && AnimationInputs.GroundSpeedCmPerSecond > UE_KINDA_SMALL_NUMBER
        && !State.AcceptedVelocityCmPerSecond.IsNearlyZero()
        && !State.LocomotionNoiseProfile.IsNone() && State.LocomotionNoiseGain > 0;
    AnimationInputs.LocomotionNoiseProfile = AnimationInputs.bLocomotionNoiseActive
        ? State.LocomotionNoiseProfile : NAME_None;
    AnimationInputs.LocomotionNoiseGain = AnimationInputs.bLocomotionNoiseActive ? State.LocomotionNoiseGain : 0;
}

void ABaddieCompanionExpeditionCharacter::SubmitMoveInput(const FVector2D& RightForward)
{
    BaddieCompanion::FExpeditionAcceptedPresentation State;
    if (!ReadCurrentPresentation(State)) return;
    const bool bValid = FMath::IsFinite(RightForward.X) && FMath::IsFinite(RightForward.Y)
        && FMath::IsFinite(RightForward.SizeSquared());
    const FVector2D Input = !bValid || Controller->IsMoveInputIgnored()
        ? FVector2D::ZeroVector : RightForward.GetClampedToMaxSize(1);
    const auto Submit = SubmitMovementIntent;
    Submit.Execute(Input, Controller->GetControlRotation().Yaw);
}

void ABaddieCompanionExpeditionCharacter::AddLookInput(float Yaw, float Pitch)
{
    BaddieCompanion::FExpeditionAcceptedPresentation State;
    if (!FMath::IsFinite(Yaw) || !FMath::IsFinite(Pitch) || !ReadCurrentPresentation(State)
        || Controller->IsLookInputIgnored()) return;
    AddControllerYawInput(Yaw);
    AddControllerPitchInput(Pitch);
}

void ABaddieCompanionExpeditionCharacter::AddLookRateInput(float YawPerSecond, float PitchPerSecond)
{
    const auto* World = GetWorld();
    if (World) AddLookInput(YawPerSecond * World->GetDeltaSeconds(), PitchPerSecond * World->GetDeltaSeconds());
}

void ABaddieCompanionExpeditionCharacter::SetAdsRequested(bool bRequested)
{
    bAdsRequested = false;
    BaddieCompanion::FExpeditionAcceptedPresentation State;
    if (!ReadCurrentPresentation(State)) return;
    bAdsRequested = bRequested && !Controller->IsLookInputIgnored()
        && State.bAdsPresentationAllowed && !State.bWeaponLowered;
    ApplyCameraAndStance(State);
}

bool ABaddieCompanionExpeditionCharacter::SetCameraFieldOfView(float WorldDegrees, float AdsDegrees)
{
    if (!FMath::IsFinite(WorldDegrees) || !FMath::IsFinite(AdsDegrees)
        || WorldDegrees <= 0 || WorldDegrees >= 180 || AdsDegrees <= 0 || AdsDegrees >= 180) return false;
    WorldFieldOfView = WorldDegrees;
    AdsFieldOfView = AdsDegrees;
    FirstPersonCamera->SetFieldOfView(AnimationInputs.bAds ? AdsFieldOfView : WorldFieldOfView);
    return true;
}

FBaddieExpeditionAnimationInputs ABaddieCompanionExpeditionCharacter::GetExpeditionAnimationInputs()
{
    BaddieCompanion::FExpeditionAcceptedPresentation State;
    if (!ReadCurrentPresentation(State)) return {};
    const uint64 Serial = SourceSerial;
    ApplyCameraAndStance(State);
    if (Serial != SourceSerial || !HasSource()) return {};
    if (GetVelocity().IsNearlyZero() || State.AcceptedVelocityCmPerSecond.IsNearlyZero())
    {
        AnimationInputs.GroundSpeedCmPerSecond = 0;
    }
    AnimationInputs.bFalling = GetCharacterMovement()->IsFalling();
    UpdateLocomotionNoise(State);
    return AnimationInputs;
}
