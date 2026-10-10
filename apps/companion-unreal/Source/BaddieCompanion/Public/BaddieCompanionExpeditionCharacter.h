#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BaddieCompanionExpeditionCharacter.generated.h"

class UCameraComponent;
class UBaddieCompanionGameplaySession;
class UBaddieCompanionExpeditionMovementComponent;
class ABaddieCompanionExpeditionCharacter;

namespace BaddieCompanion
{
    enum class EExpeditionTerrain : uint8 { Ground, Mud, Wading };

    // A projection of the gameplay owner's CURRENT accepted encounter, never a grant.
    // The owner resolves admission, life/visit, revision, route clock and content readiness.
    struct FExpeditionAcceptedPresentation
    {
        FString MissionRunId;
        FString CharacterId;
        FString LifeId;
        FString InstanceId;
        EExpeditionTerrain Terrain = EExpeditionTerrain::Ground;
        // Final absolute limit after the server's terrain policy, not a multiplier.
        float MaxGroundSpeedCmPerSecond = 0;
        // Sampled by the existing movement owner in UE coordinates. Not user intent.
        FVector AcceptedVelocityCmPerSecond = FVector::ZeroVector;
        // Authored saved-Companion eye height above the capsule centre, in local cm.
        float EyeHeightCm = 0;
        bool bWeaponLowered = true;
        bool bAdsPresentationAllowed = false;
        FName LocomotionNoiseProfile;
        // Passed through for audio; this does not implement authoritative hearing.
        float LocomotionNoiseGain = 0;
    };

    DECLARE_DELEGATE_RetVal_TwoParams(bool, FReadExpeditionPresentation,
        const ABaddieCompanionExpeditionCharacter&, FExpeditionAcceptedPresentation&);
    DECLARE_DELEGATE_TwoParams(FSubmitExpeditionMoveIntent,
        const FVector2D& /* right, forward */, float /* control yaw degrees */);
}

USTRUCT(BlueprintType)
struct BADDIECOMPANION_API FBaddieExpeditionAnimationInputs
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Expedition")
    float GroundSpeedCmPerSecond = 0;
    UPROPERTY(BlueprintReadOnly, Category = "Expedition")
    bool bFalling = false;
    UPROPERTY(BlueprintReadOnly, Category = "Expedition")
    bool bWeaponLowered = true;
    UPROPERTY(BlueprintReadOnly, Category = "Expedition")
    bool bAds = false;
    UPROPERTY(BlueprintReadOnly, Category = "Expedition")
    bool bLocomotionNoiseActive = false;
    UPROPERTY(BlueprintReadOnly, Category = "Expedition")
    FName LocomotionNoiseProfile;
    UPROPERTY(BlueprintReadOnly, Category = "Expedition")
    float LocomotionNoiseGain = 0;
};

/** Encounter-only native presentation. No default possession, assets, transport or fire. */
UCLASS(NotBlueprintable, Transient)
class BADDIECOMPANION_API ABaddieCompanionExpeditionCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ABaddieCompanionExpeditionCharacter(const FObjectInitializer& ObjectInitializer);
    virtual void CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult) override;
    virtual FVector GetPawnViewLocation() const override;
    virtual void Restart() override;
    virtual void UnPossessed() override;

    // Controller owns mappings, touch arbitration, focus and combat. Units are input
    // deltas for mouse/touch; the rate variant takes mapped input units per second.
    void SubmitMoveInput(const FVector2D& RightForward);
    void AddLookInput(float Yaw, float Pitch);
    void AddLookRateInput(float YawPerSecond, float PitchPerSecond);
    void SetAdsRequested(bool bRequested);
    // Local preference only. ADS FOV may equal world FOV to avoid camera zoom.
    bool SetCameraFieldOfView(float WorldDegrees, float AdsDegrees);
    void ClearEncounterPresentation();

    UFUNCTION(BlueprintPure, Category = "Expedition")
    FBaddieExpeditionAnimationInputs GetExpeditionAnimationInputs();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
    // The parent gameplay owner supplies the adapter after actual encounter/control
    // acceptance. Possession, Home grants and arbitrary callers cannot bind it.
    friend class UBaddieCompanionGameplaySession;
    friend class UBaddieCompanionExpeditionMovementComponent;
    bool BindAcceptedEncounterSource(UObject& Owner,
        BaddieCompanion::FReadExpeditionPresentation ReadAccepted,
        BaddieCompanion::FSubmitExpeditionMoveIntent SubmitIntent);
    bool ReadCurrentPresentation(BaddieCompanion::FExpeditionAcceptedPresentation& Out);
    bool HasSource() const;
    void ApplyCameraAndStance(const BaddieCompanion::FExpeditionAcceptedPresentation& State);
    void UpdateLocomotionNoise(const BaddieCompanion::FExpeditionAcceptedPresentation& State);
    void StopNativeMovement();
    void PrepareAcceptedMovement();
    void FinishAcceptedMovement(float DeltaSeconds, const FVector& OldLocation);

    UPROPERTY()
    TObjectPtr<UCameraComponent> FirstPersonCamera;

    TWeakObjectPtr<UObject> PresentationOwner;
    TWeakObjectPtr<AController> PresentationController;
    BaddieCompanion::FReadExpeditionPresentation ReadAcceptedPresentation;
    BaddieCompanion::FSubmitExpeditionMoveIntent SubmitMovementIntent;
    TOptional<BaddieCompanion::FExpeditionAcceptedPresentation> AcceptedPresentation;
    FBaddieExpeditionAnimationInputs AnimationInputs;
    uint64 SourceSerial = 0; // Local reentrancy guard, never a wire token or admission.
    float WorldFieldOfView = 90;
    float AdsFieldOfView = 90;
    bool bAdsRequested = false;
    bool bReadingSource = false;
    bool bClearingPresentation = false;
    bool bApplyingMovement = false;
    bool bEndingPlay = false;
};
