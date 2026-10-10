#include "BaddieCompanionSliceGameMode.h"

#include "BaddieCompanionHomeStage.h"
#include "BaddieCompanionSliceCharacter.h"
#include "BaddieCompanionSliceHUD.h"
#include "BaddieCompanionSlicePlayerController.h"
#include "BaddieCompanionSliceSession.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

ABaddieCompanionSliceGameMode::ABaddieCompanionSliceGameMode()
{
    // A read-only account grant must never spawn a replacement Companion or admit movement.
    DefaultPawnClass = nullptr;
    PlayerControllerClass = ABaddieCompanionSlicePlayerController::StaticClass();
    HUDClass = ABaddieCompanionSliceHUD::StaticClass();
}

void ABaddieCompanionSliceGameMode::StartPlay()
{
    Super::StartPlay();
    auto* Session = GetGameInstance()->GetSubsystem<UBaddieCompanionSliceSession>();
    if (!Session) return;
    Session->OnSelectionChanged.AddUObject(this, &ABaddieCompanionSliceGameMode::ApplySelection);
    Session->ClearSelection();

#if !UE_BUILD_SHIPPING
    bOfflinePreviewEnabled = UBaddieCompanionSliceSession::IsOfflinePreviewAllowed(GetWorld());
    FString Path;
    if (bOfflinePreviewEnabled
        && FParse::Value(FCommandLine::Get(), TEXT("CompanionOfflineSnapshot="), Path) && !Path.IsEmpty())
        Session->LoadOfflineSnapshot(Path);
#endif
}

void ABaddieCompanionSliceGameMode::ApplySelection()
{
    if (InspectionCharacter)
    {
        InspectionCharacter->ClearInspectionPresentation();
        if (auto* Controller = InspectionCharacter->GetController()) Controller->UnPossess();
        InspectionCharacter->Destroy();
        InspectionCharacter = nullptr;
    }
    if (Stage)
    {
        Stage->Destroy();
        Stage = nullptr;
    }
    const auto* Session = GetGameInstance()->GetSubsystem<UBaddieCompanionSliceSession>();
    if (bOfflinePreviewEnabled && UBaddieCompanionSliceSession::IsOfflinePreviewAllowed(GetWorld())
        && Session && Session->GetSelection())
    {
        Stage = GetWorld()->SpawnActor<ABaddieCompanionHomeStage>();
        auto* Controller = GetWorld()->GetFirstPlayerController();
        if (Stage && Controller && Controller->IsLocalController())
        {
            InspectionCharacter = GetWorld()->SpawnActor<ABaddieCompanionSliceCharacter>(
                FVector(0, 0, 100), FRotator::ZeroRotator);
            if (InspectionCharacter) Controller->Possess(InspectionCharacter);
        }
        if (Stage && !InspectionCharacter)
        {
            Stage->Destroy();
            Stage = nullptr;
        }
    }
}

void ABaddieCompanionSliceGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bOfflinePreviewEnabled = false;
    if (auto* Instance = GetGameInstance())
        if (auto* Session = Instance->GetSubsystem<UBaddieCompanionSliceSession>())
        {
            Session->OnSelectionChanged.RemoveAll(this);
            Session->ClearSelection();
        }
    ApplySelection();
    Super::EndPlay(EndPlayReason);
}
