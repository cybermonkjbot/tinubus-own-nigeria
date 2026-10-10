#include "BaddieCompanionHomeStage.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ABaddieCompanionHomeStage::ABaddieCompanionHomeStage()
{
    PrimaryActorTick.bCanEverTick = false;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("StageRoot")));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    const auto AddBlock = [this](const TCHAR* Name, const FVector& Position, const FVector& Scale)
    {
        auto* Block = CreateDefaultSubobject<UStaticMeshComponent>(Name);
        Block->SetupAttachment(RootComponent);
        Block->SetStaticMesh(Cube.Object);
        Block->SetRelativeLocation(Position);
        Block->SetRelativeScale3D(Scale);
        Block->SetCollisionProfileName(TEXT("BlockAll"));
        Block->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
        Block->SetCanEverAffectNavigation(false);
    };

    // Engine primitives are centimetres. These dimensions do not interpret saved room JSON.
    AddBlock(TEXT("Floor"), FVector(0, 0, -10), FVector(6, 6, 0.2));
    AddBlock(TEXT("BackWall"), FVector(0, 300, 30), FVector(6, 0.1, 0.6));
    AddBlock(TEXT("LeftWall"), FVector(-300, 0, 30), FVector(0.1, 6, 0.6));
    AddBlock(TEXT("RightWall"), FVector(300, 0, 30), FVector(0.1, 6, 0.6));

    auto* Light = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("InspectionLight"));
    Light->SetupAttachment(RootComponent);
    Light->SetMobility(EComponentMobility::Movable);
    Light->SetRelativeRotation(FRotator(-50, -30, 0));
    Light->SetIntensity(4.0f);
}
