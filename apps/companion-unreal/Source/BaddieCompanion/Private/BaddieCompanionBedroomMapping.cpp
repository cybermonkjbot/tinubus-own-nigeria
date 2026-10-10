#include "BaddieCompanionHomeMappingInternal.h"

namespace BaddieCompanion::HomeMapping
{
    FBedroomShellPlan BedroomShell(const TOptional<FHomeDesignPlan>& Design)
    {
        FBedroomShellPlan Out;
        const int32 Turns = Design.IsSet() ? Design.GetValue().Turns : 0;
        const double Yaw = Turns * UE_DOUBLE_PI / 2;
        Out.DerivedLayoutId = Design.IsSet()
            ? FString::Printf(TEXT("pink-bedroom-v02:home-v1:%d:0"), Turns) : TEXT("pink-bedroom-v02");
        Out.ShellYawRadians = Yaw; Out.ShellYawDegrees = SourceYawToUnreal(Yaw);
        const auto Point = [Turns](double X, double Y, double Z) { return TurnSourceShell(FVector(X, Y, Z), Turns); };
        const auto ShellBox = [Turns](double X, double Y, double Z, double W, double H, double D)
        {
            return Box(TurnSourceShell(FVector(X, Y, Z), Turns), Turns % 2 ? FVector(D, H, W) : FVector(W, H, D));
        };
        const FVector A = Point(-2.4, 0, -2.6), B = Point(2.4, 0, 2.6);
        Out.MinX = FMath::Min(A.X, B.X); Out.MaxX = FMath::Max(A.X, B.X);
        Out.MinZ = FMath::Min(A.Z, B.Z); Out.MaxZ = FMath::Max(A.Z, B.Z);
        Out.UnrealFloorBounds = ShellBox(0, 0, 0, 4.8, 0, 5.2).UnrealBounds;
        for (double Z : { -.75, -1.3, -.25, .35, .95 })
            for (double X : { 0.0, .6, -.6, 1.05, -1.05, -1.75 })
                Out.StandingCandidates.Add(Pose(Point(X, 0, Z), UE_DOUBLE_PI + Yaw));
        const auto Wall = [&Out, &Point, Yaw](ESurface Id, const TCHAR* SourceId, const TCHAR* Name,
            double X, double Z, double Angle, double Width, TArray<FWallOpening> Openings)
        {
            FWallPlan Result;
            Result.Id = Id; Result.SourceId = SourceId; Result.Name = Name;
            Result.Anchor = Pose(Point(X, 0, Z), Angle + Yaw);
            Result.Width = Width; Result.MinY = .2; Result.MaxY = 2.8;
            for (auto& Opening : Openings)
            {
                const double C = FMath::Cos(Result.Anchor.YawRadians), S = FMath::Sin(Result.Anchor.YawRadians);
                const FVector Bottom = Result.Anchor.Metres + FVector(C * Opening.U, Opening.BottomY, -S * Opening.U);
                Opening.UnrealBounds = Box(Bottom, FVector(FMath::Abs(C) * Opening.Width,
                    Opening.Height, FMath::Abs(S) * Opening.Width)).UnrealBounds;
            }
            Result.Openings = MoveTemp(Openings); Out.Walls.Add(MoveTemp(Result));
        };
        Wall(ESurface::WallBack, TEXT("wall-back"), TEXT("Workstation wall"), 0, -2.595, 0, 4.7,
            { { -1.74, 0, 1.12, 2.3 } });
        Wall(ESurface::WallLeft, TEXT("wall-left"), TEXT("Window wall"), -2.395, 0, UE_DOUBLE_PI / 2, 5.1,
            { { -.025, .75, 2.3, 2.05 } });
        Wall(ESurface::WallRight, TEXT("wall-right"), TEXT("Wardrobe wall"), 2.395, 0, -UE_DOUBLE_PI / 2, 5.1, {});
        // Curved headboard is not a flat wall support. No synthetic wall-front.
        Out.FloorPatches.Add(ShellBox(-.7, .014, .25, 2.75, 0, 1.6));
        const TArray<EItemKind> TableKinds = { EItemKind::Lamp, EItemKind::Plant, EItemKind::DeskFan,
            EItemKind::DeskLamp, EItemKind::BookStack, EItemKind::Candle, EItemKind::Succulent };
        FSupportPlan Floor;
        Floor.Id = ESurface::Floor; Floor.SourceId = TEXT("floor"); Floor.Name = TEXT("Floor");
        Floor.Top = ShellBox(0, 0, 0, 4.8, 0, 5.2);
        Floor.AllowedKinds = TableKinds;
        Floor.AllowedKinds.Append({ EItemKind::Stool, EItemKind::StandingFan, EItemKind::TowerFan,
            EItemKind::FloorLamp, EItemKind::Armchair, EItemKind::Loveseat, EItemKind::Ottoman,
            EItemKind::CoffeeTable, EItemKind::SideTable, EItemKind::Bookcase, EItemKind::StorageBasket });
        for (const auto& Standing : Out.StandingCandidates) Floor.Anchors.Add(Pose(Standing.Metres));
        Out.Supports.Add(MoveTemp(Floor));
        const auto Support = [&Out, &ShellBox, &Point, &TableKinds](ESurface Id, const TCHAR* SourceId,
            const TCHAR* Name, double X, double Z, double Y, double W, double D,
            double AnchorX, double AnchorZ, const TCHAR* Obstacle)
        {
            FSupportPlan Result;
            Result.Id = Id; Result.SourceId = SourceId; Result.Name = Name;
            Result.Top = ShellBox(X, Y, Z, W, 0, D); Result.AllowedKinds = TableKinds;
            // Source anchors are horizontal x/z points, not an authored y/yaw.
            Result.Anchors.Add(Pose(Point(AnchorX, 0, AnchorZ)));
            Result.SupportingObstacleIds.Add(Obstacle);
            Result.Clearance = ShellBox(AnchorX, Y, AnchorZ, .42, .56, .42);
            Out.Supports.Add(MoveTemp(Result));
        };
        Support(ESurface::Console, TEXT("console"), TEXT("Console"), -.53, -2.34, .8125, 1.33, .49,
            -.53, -2.265, TEXT("media-console"));
        Support(ESurface::Shelf, TEXT("shelf"), TEXT("Shelf"), -2.145, -1.48, 1.2665, .48, .90,
            -2.145, -1.48, TEXT("decor-shelf"));
        const auto Obstacle = [&Out, &ShellBox](const TCHAR* Id, double X, double Z,
            double W, double D, double H, TOptional<double> Bottom = {})
        {
            Out.Obstacles.Add({ Id, ShellBox(X, Bottom.Get(0), Z, W, H, D), Bottom });
        };
        Obstacle(TEXT("bed"), -.7, 1.2205, 1.9766, 2.3352, 1.5351);
        Obstacle(TEXT("bedside-left"), -2.06, 1.91, .4602, .5202, .9126, .04);
        Obstacle(TEXT("bedside-right"), .58, 1.91, .4602, .5202, .9126, .04);
        Obstacle(TEXT("wardrobe"), 2.05, .55, .6962, 3.4262, 2.8062, .0299);
        Obstacle(TEXT("clothes-rack"), 2.0457, -.48, .6288, 1.2802, 2.4081, .256);
        Obstacle(TEXT("workstation"), 1.1, -2.261, 1.7502, .6622, 1.3451);
        Obstacle(TEXT("desk-chair"), 1.1005, -1.52, .6292, .6268, 1.0837);
        Obstacle(TEXT("media-console"), -.53, -2.327, 1.3302, .5162, .7727, .0399);
        Obstacle(TEXT("media-display"), -.5, -2.545, 1.4302, .0852, 1.2502, .9049);
        Obstacle(TEXT("decor-shelf"), -2.1475, -1.48, .4852, .9002, .2665, 1.0);
        Obstacle(TEXT("window-chair"), -1.71, -.75, .7002, .6102, 1.05);
        Obstacle(TEXT("side-table"), -1.62, -1.58, .4402, .4402, .5711, .009);
        Obstacle(TEXT("mirror"), 2.36375, -1.44, .0476, .5102, 1.9501, .125);
        Obstacle(TEXT("curtain-near-bed"), -2.2631, .7609, .0816, .4304, 2.7991, .0492);
        Obstacle(TEXT("curtain-near-seat"), -2.2631, -.7091, .0816, .4304, 2.7991, .0492);
        Obstacle(TEXT("window-sill"), -2.375, .025, .2202, 2.1902, .0351, .7675);
        Obstacle(TEXT("bedside-indoor-plant"), 1.189, 1.856, .7866, .6854, 2.0338);
        Obstacle(TEXT("workstation-wall-painting"), 1.1, -2.5675, 1.21, .065, .91, 1.55);
        Out.AuthoredSpawn = Pose(Point(0, 0, -.75), UE_DOUBLE_PI + Yaw);
        const auto Camera = [Turns](FVector Eye, FVector Target)
        {
            Eye = TurnSourceShell(Eye, Turns); Target = TurnSourceShell(Target, Turns);
            return FCameraReference{ Eye, Target, SourceToUnreal(Eye), SourceToUnreal(Target) };
        };
        Out.Wide = Camera(FVector(-1.92, 1.86, -2.22), FVector(-.15, .95, .45));
        Out.PortraitWide = Camera(FVector(-1.92, 1.86, -2.22), FVector(0, .45, -.75));
        Out.Close = Camera(FVector(-.7, 1.6, -2.15), FVector(0, .5, -.75));
        return Out;
    }
}
