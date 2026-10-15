#include "BaddieCompanionHomeMappingInternal.h"

namespace BaddieCompanion::HomeMapping
{
    namespace
    {
        struct FBedroomObstacle
        {
            const TCHAR* Id;
            double X, Z, Width, Depth, Height, Bottom;
            bool bSavedBottom;
        };
        static const FBedroomObstacle Obstacles[] = {
            { TEXT("bed"), -.7, 1.2205, 1.9766, 2.3352, 1.5351, 0, false },
            { TEXT("bedside-left"), -2.06, 1.91, .4602, .5202, .9126, .04, true },
            { TEXT("bedside-right"), .58, 1.91, .4602, .5202, .9126, .04, true },
            { TEXT("wardrobe"), 2.05, .55, .6962, 3.4262, 2.8062, .0299, true },
            { TEXT("clothes-rack"), 2.0457, -.48, .6288, 1.2802, 2.4081, .256, true },
            { TEXT("workstation"), 1.1, -2.261, 1.7502, .6622, 1.3451, 0, false },
            { TEXT("desk-chair"), 1.1005, -1.52, .6292, .6268, 1.0837, 0, false },
            { TEXT("media-console"), -.53, -2.327, 1.3302, .5162, .7727, .0399, true },
            { TEXT("media-display"), -.5, -2.545, 1.4302, .0852, 1.2502, .9049, true },
            { TEXT("decor-shelf"), -2.1475, -1.48, .4852, .9002, .2665, 1.0, true },
            { TEXT("window-chair"), -1.71, -.75, .7002, .6102, 1.05, 0, false },
            { TEXT("side-table"), -1.62, -1.58, .4402, .4402, .5711, .009, true },
            { TEXT("mirror"), 2.36375, -1.44, .0476, .5102, 1.9501, .125, true },
            { TEXT("curtain-near-bed"), -2.2631, .7609, .0816, .4304, 2.7991, .0492, true },
            { TEXT("curtain-near-seat"), -2.2631, -.7091, .0816, .4304, 2.7991, .0492, true },
            { TEXT("window-sill"), -2.375, .025, .2202, 2.1902, .0351, .7675, true },
            { TEXT("bedside-indoor-plant"), 1.189, 1.856, .7866, .6854, 2.0338, 0, false },
            { TEXT("workstation-wall-painting"), 1.1, -2.5675, 1.21, .065, .91, 1.55, true }
        };
        struct FBedroomWall
        {
            ESurface Id;
            const TCHAR* SourceId;
            const TCHAR* Name;
            double X, Z, Angle, Width;
            bool bOpening;
            double U, Bottom, OpeningWidth, Height;
        };
        static const FBedroomWall Walls[] = {
            { ESurface::WallBack, TEXT("wall-back"), TEXT("Workstation wall"), 0, -2.595, 0, 4.7,
                true, -1.74, 0, 1.12, 2.3 },
            { ESurface::WallLeft, TEXT("wall-left"), TEXT("Window wall"), -2.395, 0, UE_DOUBLE_PI / 2, 5.1,
                true, -.025, .75, 2.3, 2.05 },
            { ESurface::WallRight, TEXT("wall-right"), TEXT("Wardrobe wall"), 2.395, 0, -UE_DOUBLE_PI / 2, 5.1,
                false, 0, 0, 0, 0 }
        };
    }

    void FBedroomMapping::Begin(FRoomShellPlan& Shell, const TOptional<FHomeDesignPlan>& Design,
        bool bCopyCameraObstacles)
    {
        Output = &Shell; bHasDesign = Design.IsSet();
        Turns = Design.IsSet() ? Design->Turns : 0;
        Yaw = Turns * UE_DOUBLE_PI / 2; bCameraCopy = bCopyCameraObstacles;
        Phase = Index = Child = 0;
    }

    EMappingStep FBedroomMapping::Advance(FMappingWork& Work)
    {
        if (!Output) return EMappingStep::Invalid;
        auto& Out = *Output;
        const auto Point = [this](double X, double Y, double Z)
        { return TurnSourceShell(FVector(X, Y, Z), Turns); };
        const auto ShellBox = [this, &Point](double X, double Y, double Z, double W, double H, double D)
        { return Box(Point(X, Y, Z), Turns % 2 ? FVector(D, H, W) : FVector(W, H, D)); };
        while (Phase < 15)
        {
            // Requested capacity is not allocator qualification. The world
            // adapter must reject unqualified allocations before this call.
            uint64 Capacity = 0;
            if (Phase == 0) Capacity = 64 * sizeof(TCHAR);
            if (Phase == 1) Capacity = (Out.StandingCandidates.Num() + 1) * sizeof(FSourcePose);
            if (Phase == 2) Capacity = (Out.Walls.Num() + 1) * sizeof(FWallPlan) + 128 * sizeof(TCHAR);
            if (Phase == 3) Capacity = sizeof(FWallOpening);
            if (Phase == 4) Capacity = sizeof(FSourceBox);
            if (Phase == 5) Capacity = (Out.Supports.Num() + 1) * sizeof(FSupportPlan) + 64 * sizeof(TCHAR);
            if (Phase == 6) Capacity = (Child + 1) * sizeof(EItemKind);
            if (Phase == 7) Capacity = (Child + 1) * sizeof(FSourcePose);
            if (Phase == 8) Capacity = sizeof(FSourcePose);
            if (Phase == 9) Capacity = sizeof(FString) + 32 * sizeof(TCHAR);
            if (Phase == 10 || Phase == 14)
                Capacity = (Index + 1) * sizeof(FLayoutObstacle) + 64 * sizeof(TCHAR);
            const auto Admission = Work.Admit(512, Capacity);
            if (Admission != EMappingStep::Complete) return Admission;
            switch (Phase)
            {
            case 0:
                {
                    Out.DerivedLayoutId = bHasDesign
                        ? FString::Printf(TEXT("pink-bedroom-v02:home-v1:%d:0"), Turns) : TEXT("pink-bedroom-v02");
                    Out.ShellYawRadians = Yaw; Out.ShellYawDegrees = SourceYawToUnreal(Yaw);
                    const FVector A = Point(-2.4, 0, -2.6), B = Point(2.4, 0, 2.6);
                    Out.MinX = FMath::Min(A.X, B.X); Out.MaxX = FMath::Max(A.X, B.X);
                    Out.MinZ = FMath::Min(A.Z, B.Z); Out.MaxZ = FMath::Max(A.Z, B.Z);
                    Out.UnrealFloorBounds = ShellBox(0, 0, 0, 4.8, 0, 5.2).UnrealBounds;
                }
                break;
            case 1:
                if (Index < 30)
                {
                    static constexpr double Z[] = { -.75, -1.3, -.25, .35, .95 };
                    static constexpr double X[] = { 0.0, .6, -.6, 1.05, -1.05, -1.75 };
                    Out.StandingCandidates.Add(Pose(Point(X[Index % 6], 0, Z[Index / 6]), UE_DOUBLE_PI + Yaw));
                    ++Index; continue;
                }
                break;
            case 2:
                if (Index < UE_ARRAY_COUNT(Walls))
                {
                    const auto& Row = Walls[Index];
                    auto& Wall = Out.Walls.AddDefaulted_GetRef();
                    Wall.Id = Row.Id; Wall.SourceId = Row.SourceId; Wall.Name = Row.Name;
                    Wall.Anchor = Pose(Point(Row.X, 0, Row.Z), Row.Angle + Yaw);
                    Wall.Width = Row.Width; Wall.MinY = .2; Wall.MaxY = 2.8;
                    Phase = 3; continue;
                }
                Phase = 4; Index = 0; continue;
            case 3:
                {
                    const auto& Row = Walls[Index];
                    auto& Wall = Out.Walls.Last();
                    if (Row.bOpening)
                    {
                        auto& Opening = Wall.Openings.AddDefaulted_GetRef();
                        Opening.U = Row.U; Opening.BottomY = Row.Bottom;
                        Opening.Width = Row.OpeningWidth; Opening.Height = Row.Height;
                        const double C = FMath::Cos(Wall.Anchor.YawRadians), S = FMath::Sin(Wall.Anchor.YawRadians);
                        const FVector Bottom = Wall.Anchor.Metres + FVector(C * Opening.U, Opening.BottomY, -S * Opening.U);
                        Opening.UnrealBounds = Box(Bottom, FVector(FMath::Abs(C) * Opening.Width,
                            Opening.Height, FMath::Abs(S) * Opening.Width)).UnrealBounds;
                    }
                    ++Index; Phase = 2; continue;
                }
            case 4:
                // Curved headboard is not a flat wall support. No wall-front.
                Out.FloorPatches.Add(ShellBox(-.7, .014, .25, 2.75, 0, 1.6));
                break;
            case 5:
                {
                    auto& Support = Out.Supports.AddDefaulted_GetRef();
                    if (Index == 0)
                    {
                        Support.Id = ESurface::Floor; Support.SourceId = TEXT("floor"); Support.Name = TEXT("Floor");
                        Support.Top = ShellBox(0, 0, 0, 4.8, 0, 5.2);
                    }
                    else if (Index == 1)
                    {
                        Support.Id = ESurface::Console; Support.SourceId = TEXT("console"); Support.Name = TEXT("Console");
                        Support.Top = ShellBox(-.53, .8125, -2.34, 1.33, 0, .49);
                    }
                    else
                    {
                        Support.Id = ESurface::Shelf; Support.SourceId = TEXT("shelf"); Support.Name = TEXT("Shelf");
                        Support.Top = ShellBox(-2.145, 1.2665, -1.48, .48, 0, .90);
                    }
                    Child = 0;
                }
                break;
            case 6:
                {
                    auto& Support = Out.Supports.Last();
                    EItemKind Kind;
                    if (SupportKindAt(Support.Id, Child, Kind))
                    { Support.AllowedKinds.Add(Kind); ++Child; continue; }
                    Child = 0;
                }
                break;
            case 7:
                if (Index == 0 && Child < static_cast<uint32>(Out.StandingCandidates.Num()))
                { Out.Supports.Last().Anchors.Add(Pose(Out.StandingCandidates[Child++].Metres)); continue; }
                break;
            case 8:
                if (Index != 0) Out.Supports.Last().Anchors.Add(Pose(Index == 1
                    ? Point(-.53, 0, -2.265) : Point(-2.145, 0, -1.48)));
                break;
            case 9:
                if (Index != 0)
                {
                    auto& Support = Out.Supports.Last();
                    Support.SupportingObstacleIds.Add(Index == 1 ? TEXT("media-console") : TEXT("decor-shelf"));
                    Support.Clearance = Index == 1 ? ShellBox(-.53, .8125, -2.265, .42, .56, .42)
                        : ShellBox(-2.145, 1.2665, -1.48, .42, .56, .42);
                }
                if (++Index < 3) { Phase = 5; Child = 0; continue; }
                Index = 0;
                break;
            case 10:
                if (Index < UE_ARRAY_COUNT(Obstacles))
                {
                    const auto& Row = Obstacles[Index++];
                    auto& Obstacle = Out.Obstacles.AddDefaulted_GetRef();
                    Obstacle.Id = Row.Id;
                    Obstacle.Box = ShellBox(Row.X, Row.Bottom, Row.Z, Row.Width, Row.Height, Row.Depth);
                    if (Row.bSavedBottom) Obstacle.SavedBottom = Row.Bottom;
                    continue;
                }
                break;
            case 11:
                Out.PreferredSpawn = Pose(Point(0, 0, -.75), UE_DOUBLE_PI + Yaw);
                break;
            case 12:
            case 13:
                {
                    const auto Camera = [this](FVector Eye, FVector Target)
                    {
                        Eye = TurnSourceShell(Eye, Turns); Target = TurnSourceShell(Target, Turns);
                        return FCameraReference{ Eye, Target, SourceToUnreal(Eye), SourceToUnreal(Target) };
                    };
                    if (Phase == 12)
                    {
                        if (Index == 0)
                        { Out.Wide = Camera(FVector(-1.92, 1.86, -2.22), FVector(-.15, .95, .45)); ++Index; continue; }
                        Out.PortraitWide = Camera(FVector(-1.92, 1.86, -2.22), FVector(0, .45, -.75));
                    }
                    else Out.Close = Camera(FVector(-.7, 1.6, -2.15), FVector(0, .5, -.75));
                }
                break;
            case 14:
                if (bCameraCopy && Index < static_cast<uint32>(Out.Obstacles.Num()))
                { Out.CameraObstacles.Add(Out.Obstacles[Index++]); continue; }
                break;
            }
            ++Phase;
            // Support phases share their row cursor. Other phases start fresh.
            if (Phase < 6 || Phase > 9) Index = 0;
            Child = 0;
        }
        return EMappingStep::Complete;
    }

    FRoomShellPlan BedroomShell(const TOptional<FHomeDesignPlan>& Design)
    {
        FRoomShellPlan Out;
        FLegacyMappingWork Work;
        FBedroomMapping Mapping;
        Mapping.Begin(Out, Design);
        Mapping.Advance(Work);
        return Out;
    }
}
