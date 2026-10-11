#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BaddieCompanionWorldObservation.h"
#include "BaddieCompanionWorldCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UBaddieCompanionGameplaySession;
class ABaddieCompanionSlicePlayerController;

/** Ordinary-world movement owner. Authored visuals remain separate CharacterActors. */
UCLASS(NotBlueprintable, Transient)
class BADDIECOMPANION_API ABaddieCompanionWorldCharacter : public ACharacter
{
    GENERATED_BODY()
public:
    ABaddieCompanionWorldCharacter();
protected:
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    friend class ABaddieCompanionSlicePlayerController;
    bool FollowAccepted(UBaddieCompanionGameplaySession& Session);
    void ClearWorldMovement();
    UPROPERTY(Transient)
    TObjectPtr<USpringArmComponent> CameraArm;
    UPROPERTY(Transient)
    TObjectPtr<UCameraComponent> WorldCamera;
    BaddieCompanion::FWorldObservation Observation;
    double LastSampleMs = 0;
    int64 LastRouteSequence = -1;
    BaddieCompanion::WorldObservationPrivate::FMovementIdentity RouteIdentity;
    BaddieCompanion::FMovementPose LastCommittedPose;
    bool bHasRouteIdentity = false;
};
