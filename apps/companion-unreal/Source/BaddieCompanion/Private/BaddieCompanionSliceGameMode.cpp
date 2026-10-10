#include "BaddieCompanionSliceGameMode.h"

#include "BaddieCompanionHomeStage.h"
#include "BaddieCompanionSliceCharacter.h"
#include "BaddieCompanionSliceHUD.h"
#include "BaddieCompanionSlicePlayerController.h"
#include "BaddieCompanionSliceSession.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

ABaddieCompanionSliceGameMode::ABaddieCompanionSliceGameMode()
{
    DefaultPawnClass = ABaddieCompanionSliceCharacter::StaticClass();
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
    bOfflinePreviewEnabled = GetNetMode() == NM_Standalone
        && FParse::Param(FCommandLine::Get(), TEXT("CompanionOfflinePreview"));
    FString Path;
    if (bOfflinePreviewEnabled
        && FParse::Value(FCommandLine::Get(), TEXT("CompanionOfflineSnapshot="), Path) && !Path.IsEmpty())
        Session->LoadOfflineSnapshot(Path);
#endif
}

void ABaddieCompanionSliceGameMode::ApplySelection()
{
    if (Stage)
    {
        Stage->Destroy();
        Stage = nullptr;
    }
    const auto* Session = GetGameInstance()->GetSubsystem<UBaddieCompanionSliceSession>();
    if (bOfflinePreviewEnabled && Session && Session->GetSelection())
        Stage = GetWorld()->SpawnActor<ABaddieCompanionHomeStage>();
}

void ABaddieCompanionSliceGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (auto* Instance = GetGameInstance())
        if (auto* Session = Instance->GetSubsystem<UBaddieCompanionSliceSession>())
        {
            Session->OnSelectionChanged.RemoveAll(this);
            Session->ClearSelection();
        }
    Super::EndPlay(EndPlayReason);
}
