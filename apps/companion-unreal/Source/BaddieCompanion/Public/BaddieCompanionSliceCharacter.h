#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BaddieCompanionSliceCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;

/** Local-only inspection movement. Never sends movement, rewards or care writes. */
UCLASS(NotBlueprintable)
class BADDIECOMPANION_API ABaddieCompanionSliceCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ABaddieCompanionSliceCharacter();
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    void ClearInspectionPresentation();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void ApplySelection();
    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);
    void TurnRate(float Value);
    void LookUpRate(float Value);
    void Zoom(float Value);
    void ResetCamera();

    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> PlaceholderBody;
    UPROPERTY()
    TObjectPtr<USpringArmComponent> CameraArm;
    UPROPERTY()
    TObjectPtr<UCameraComponent> FollowCamera;

    bool bSelectionReady = false;
};
