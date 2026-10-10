#include "BaddieCompanionSlicePlayerController.h"

#include "Components/InputComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerCameraManager.h"

void ABaddieCompanionSlicePlayerController::BeginPlay()
{
    Super::BeginPlay();
    if (!IsLocalController()) return;
    if (PlayerCameraManager)
    {
        PlayerCameraManager->ViewPitchMin = -65;
        PlayerCameraManager->ViewPitchMax = -15;
    }
    ApplyPointerCapture();
}

void ABaddieCompanionSlicePlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindAction(TEXT("ReleasePointer"), IE_Pressed,
        this, &ABaddieCompanionSlicePlayerController::TogglePointerCapture);
}

void ABaddieCompanionSlicePlayerController::TogglePointerCapture()
{
    bPointerCaptured = !bPointerCaptured;
    ApplyPointerCapture();
}

void ABaddieCompanionSlicePlayerController::ApplyPointerCapture()
{
    bShowMouseCursor = !bPointerCaptured;
    ResetIgnoreMoveInput();
    ResetIgnoreLookInput();
    SetIgnoreMoveInput(!bPointerCaptured);
    SetIgnoreLookInput(!bPointerCaptured);
    if (!bPointerCaptured)
        if (auto* Character = Cast<ACharacter>(GetPawn()))
            Character->GetCharacterMovement()->StopMovementImmediately();
    if (bPointerCaptured)
    {
        FInputModeGameOnly Mode;
        Mode.SetConsumeCaptureMouseDown(false);
        SetInputMode(Mode);
    }
    else
    {
        FInputModeGameAndUI Mode;
        Mode.SetHideCursorDuringCapture(false);
        SetInputMode(Mode);
    }
}
