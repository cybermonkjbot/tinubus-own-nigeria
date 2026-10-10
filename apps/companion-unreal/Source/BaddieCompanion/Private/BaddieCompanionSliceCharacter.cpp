#include "BaddieCompanionSliceCharacter.h"

#include "BaddieCompanionHomeStage.h"
#include "BaddieCompanionSliceSession.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "UObject/ConstructorHelpers.h"

ABaddieCompanionSliceCharacter::ABaddieCompanionSliceCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    SetReplicates(false);
    SetReplicateMovement(false);
    GetCapsuleComponent()->InitCapsuleSize(34, 88);
    bUseControllerRotationYaw = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0, 540, 0);
    GetCharacterMovement()->MaxWalkSpeed = 220;

    PlaceholderBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OfflinePlaceholder"));
    PlaceholderBody->SetupAttachment(RootComponent);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    PlaceholderBody->SetStaticMesh(Sphere.Object);
    PlaceholderBody->SetRelativeScale3D(FVector(0.6, 0.6, 1.4));
    PlaceholderBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PlaceholderBody->SetCanEverAffectNavigation(false);
    PlaceholderBody->SetHiddenInGame(true);

    CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
    CameraArm->SetupAttachment(RootComponent);
    CameraArm->TargetArmLength = 600;
    CameraArm->TargetOffset = FVector(0, 0, 40);
    CameraArm->bUsePawnControlRotation = true;
    CameraArm->bInheritRoll = false;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraArm, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;
}

void ABaddieCompanionSliceCharacter::BeginPlay()
{
    Super::BeginPlay();
    if (auto* Session = GetGameInstance()->GetSubsystem<UBaddieCompanionSliceSession>())
        Session->OnSelectionChanged.AddUObject(this, &ABaddieCompanionSliceCharacter::ApplySelection);
    ApplySelection();
    ResetCamera();
}

void ABaddieCompanionSliceCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ClearInspectionPresentation();
    if (auto* Instance = GetGameInstance())
        if (auto* Session = Instance->GetSubsystem<UBaddieCompanionSliceSession>())
            Session->OnSelectionChanged.RemoveAll(this);
    Super::EndPlay(EndPlayReason);
}

void ABaddieCompanionSliceCharacter::ApplySelection()
{
    const auto* Session = GetGameInstance()->GetSubsystem<UBaddieCompanionSliceSession>();
    bSelectionReady = UBaddieCompanionSliceSession::IsOfflinePreviewAllowed(GetWorld())
        && Session && Session->GetSelection();
    PlaceholderBody->SetHiddenInGame(!bSelectionReady);
    SetActorEnableCollision(bSelectionReady);
    GetCharacterMovement()->StopMovementImmediately();
    if (bSelectionReady)
    {
        SetActorLocation(FVector(0, 0, 100), false, nullptr, ETeleportType::TeleportPhysics);
        GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    }
    else
    {
        ClearInspectionPresentation();
    }
}

void ABaddieCompanionSliceCharacter::ClearInspectionPresentation()
{
    bSelectionReady = false;
    PlaceholderBody->SetHiddenInGame(true);
    SetActorEnableCollision(false);
    ConsumeMovementInputVector();
    GetCharacterMovement()->StopMovementImmediately();
    GetCharacterMovement()->DisableMovement();
    SetActorLocation(FVector::ZeroVector, false, nullptr, ETeleportType::TeleportPhysics);
    SetActorRotation(FRotator::ZeroRotator);
    CameraArm->TargetArmLength = 600;
    if (Controller) Controller->SetControlRotation(FRotator::ZeroRotator);
}

void ABaddieCompanionSliceCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!UBaddieCompanionSliceSession::IsOfflinePreviewAllowed(GetWorld()))
        ClearInspectionPresentation();
    if (!bSelectionReady) return;

    const double Limit = ABaddieCompanionHomeStage::HalfExtentCm - 45.0;
    const FVector Position = GetActorLocation();
    const FVector Bounded(FMath::Clamp(Position.X, -Limit, Limit),
        FMath::Clamp(Position.Y, -Limit, Limit), Position.Z < -100 ? 100 : Position.Z);
    if (!Position.Equals(Bounded))
        SetActorLocation(Bounded, false, nullptr, ETeleportType::TeleportPhysics);
}

void ABaddieCompanionSliceCharacter::SetupPlayerInputComponent(UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input);
    Input->BindAxis(TEXT("MoveForward"), this, &ABaddieCompanionSliceCharacter::MoveForward);
    Input->BindAxis(TEXT("MoveRight"), this, &ABaddieCompanionSliceCharacter::MoveRight);
    Input->BindAxis(TEXT("Turn"), this, &ABaddieCompanionSliceCharacter::Turn);
    Input->BindAxis(TEXT("LookUp"), this, &ABaddieCompanionSliceCharacter::LookUp);
    Input->BindAxis(TEXT("TurnRate"), this, &ABaddieCompanionSliceCharacter::TurnRate);
    Input->BindAxis(TEXT("LookUpRate"), this, &ABaddieCompanionSliceCharacter::LookUpRate);
    Input->BindAxis(TEXT("Zoom"), this, &ABaddieCompanionSliceCharacter::Zoom);
    Input->BindAction(TEXT("ResetCamera"), IE_Pressed, this, &ABaddieCompanionSliceCharacter::ResetCamera);
}

void ABaddieCompanionSliceCharacter::MoveForward(float Value)
{
    if (bSelectionReady && Controller && !Controller->IsMoveInputIgnored())
        AddMovementInput(FRotationMatrix(FRotator(0, Controller->GetControlRotation().Yaw, 0)).GetUnitAxis(EAxis::X), Value);
}

void ABaddieCompanionSliceCharacter::MoveRight(float Value)
{
    if (bSelectionReady && Controller && !Controller->IsMoveInputIgnored())
        AddMovementInput(FRotationMatrix(FRotator(0, Controller->GetControlRotation().Yaw, 0)).GetUnitAxis(EAxis::Y), Value);
}

void ABaddieCompanionSliceCharacter::Turn(float Value)
{
    if (bSelectionReady) AddControllerYawInput(Value);
}

void ABaddieCompanionSliceCharacter::LookUp(float Value)
{
    if (bSelectionReady) AddControllerPitchInput(Value);
}

void ABaddieCompanionSliceCharacter::TurnRate(float Value)
{
    Turn(Value * 90 * GetWorld()->GetDeltaSeconds());
}

void ABaddieCompanionSliceCharacter::LookUpRate(float Value)
{
    LookUp(Value * 90 * GetWorld()->GetDeltaSeconds());
}

void ABaddieCompanionSliceCharacter::Zoom(float Value)
{
    if (bSelectionReady && Controller && !Controller->IsLookInputIgnored())
        CameraArm->TargetArmLength = FMath::Clamp(CameraArm->TargetArmLength - Value * 50, 250.0f, 800.0f);
}

void ABaddieCompanionSliceCharacter::ResetCamera()
{
    if (!bSelectionReady || (Controller && Controller->IsLookInputIgnored())) return;
    CameraArm->TargetArmLength = 600;
    if (Controller) Controller->SetControlRotation(FRotator(-30, -90, 0));
}
