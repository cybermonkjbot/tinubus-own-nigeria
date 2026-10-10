#include "BaddieCompanionHomeMappingInternal.h"

namespace BaddieCompanion::HomeMapping::LayoutMapping
{
    const TArray<EItemKind> TableKinds = { EItemKind::Lamp, EItemKind::Plant, EItemKind::DeskFan,
        EItemKind::DeskLamp, EItemKind::BookStack, EItemKind::Candle, EItemKind::Succulent };

    FSupportPlan Support(ESurface Id, const TCHAR* SourceId, const TCHAR* Name,
        const FVector& Top, const FVector& Size, std::initializer_list<FVector> Anchors,
        const TCHAR* SupportingObstacle = nullptr)
    {
        FSupportPlan Out;
        Out.Id = Id; Out.SourceId = SourceId; Out.Name = Name;
        Out.Top = Box(Top, Size); Out.AllowedKinds = TableKinds;
        if (Id == ESurface::Floor) Out.AllowedKinds.Append({ EItemKind::Stool, EItemKind::StandingFan,
            EItemKind::TowerFan, EItemKind::FloorLamp, EItemKind::Armchair, EItemKind::Loveseat,
            EItemKind::Ottoman, EItemKind::CoffeeTable, EItemKind::SideTable, EItemKind::Bookcase,
            EItemKind::StorageBasket });
        for (const auto& Anchor : Anchors) Out.Anchors.Add(Pose(Anchor));
        if (SupportingObstacle) Out.SupportingObstacleIds.Add(SupportingObstacle);
        return Out;
    }

    void Bounds(FRoomShellPlan& Out, const FVector& Minimum, const FVector& Maximum)
    {
        Out.MinX = Minimum.X; Out.MaxX = Maximum.X;
        Out.MinZ = Minimum.Z; Out.MaxZ = Maximum.Z;
        Out.UnrealFloorBounds = Box((Minimum + Maximum) / 2, Maximum - Minimum).UnrealBounds;
    }

    void Standing(FRoomShellPlan& Out, std::initializer_list<double> XValues,
        std::initializer_list<double> ZValues, double Yaw = UE_DOUBLE_PI)
    {
        for (double Z : ZValues) for (double X : XValues)
            Out.StandingCandidates.Add(Pose(FVector(X, 0, Z), Yaw));
    }

    void Floor(FRoomShellPlan& Out, const FVector& Top, const FVector& Size)
    {
        auto Value = Support(ESurface::Floor, TEXT("floor"), TEXT("Floor"), Top, Size, {});
        for (const auto& Candidate : Out.StandingCandidates) Value.Anchors.Add(Pose(Candidate.Metres));
        Out.Supports.Add(MoveTemp(Value));
    }

    void Wall(FRoomShellPlan& Out, ESurface Id, const TCHAR* SourceId, const TCHAR* Name,
        const FVector& Anchor, double Yaw, double Width, double MaxY,
        TArray<FWallOpening> Openings = {})
    {
        FWallPlan Value;
        Value.Id = Id; Value.SourceId = SourceId; Value.Name = Name;
        Value.Anchor = Pose(Anchor, Yaw); Value.Width = Width; Value.MinY = .2; Value.MaxY = MaxY;
        Value.Openings = MoveTemp(Openings); Out.Walls.Add(MoveTemp(Value));
    }

    void Obstacle(FRoomShellPlan& Out, const TCHAR* Id, const FVector& Bottom,
        const FVector& Size, bool bHasSavedBottom = false)
    {
        FLayoutObstacle Value;
        Value.Id = Id; Value.Box = Box(Bottom, Size);
        if (bHasSavedBottom) Value.SavedBottom = Bottom.Y;
        Out.Obstacles.Add(MoveTemp(Value));
    }

    FCameraReference Camera(const FVector& Eye, const FVector& Target)
    {
        return { Eye, Target, SourceToUnreal(Eye), SourceToUnreal(Target) };
    }

    FRoomShellPlan StudioShell(bool bBedroom)
    {
        FRoomShellPlan Out;
        Out.DerivedLayoutId = bBedroom ? TEXT("pink-bedroom-v01") : TEXT("pat-studio-v01");
        Bounds(Out, FVector(-1.6891, 0, -2.2606), FVector(1.6891, 0, 2.2606));
        Standing(Out, { -.8, -.2, .4, 1, -1.3 }, { 1.2, .6, 0, -.6, -1.2, -1.8 });
        // Both layouts reuse sourceFloor's anchors, even though bedroom standing candidates differ.
        Floor(Out, FVector::ZeroVector, FVector(3.3782, 0, 4.5212));
        Wall(Out, ESurface::WallFront, TEXT("wall-front"), TEXT("Console wall"),
            FVector(0, 0, 2.2556), UE_DOUBLE_PI, 3.3, 2.7);
        if (bBedroom)
        {
            Out.StandingCandidates.Reset();
            Standing(Out, { -.4, .2, -.95, .8, 1.35 }, { -1.1, -.5, -1.7, .1, .7, 1.3 });
            Out.Supports.Add(Support(ESurface::Console, TEXT("console"), TEXT("Dresser"),
                FVector(1.3009, .81, -1.3994), FVector(.50, 0, 1.18),
                { FVector(1.285, 0, -1.65), FVector(1.285, 0, -1.12) }, TEXT("dresser")));
            Out.Supports.Add(Support(ESurface::Shelf, TEXT("shelf"), TEXT("Shelf"),
                FVector(-1.4191, 1.4225, -1.6094), FVector(.44, 0, .88),
                { FVector(-1.42, 0, -1.65) }, TEXT("shelf")));
            Obstacle(Out, TEXT("bed"), FVector(-.165, 0, 1.045), FVector(1.63, 1.25, 2.15));
            Obstacle(Out, TEXT("bedside-left"), FVector(-1.25, 0, 1.695), FVector(.46, .75, .49));
            Obstacle(Out, TEXT("bedside-right"), FVector(.94, 0, 1.695), FVector(.48, .75, .49));
            Obstacle(Out, TEXT("dresser"), FVector(1.285, 0, -1.4), FVector(.55, .81, 1.2));
            Obstacle(Out, TEXT("shelf"), FVector(-1.42, 0, -1.61), FVector(.44, 1.7, .88));
            Out.Wide = Camera(FVector(-.8, 1.65, -2.02), FVector(0, .8, .25));
            Out.Close = Camera(FVector(-.75, 1.35, -1.9), FVector(-.4, .75, -1.1));
            Out.PreferredSpawn = Pose(FVector(-.4, 0, -1.1), UE_DOUBLE_PI);
        }
        else
        {
            auto Console = Support(ESurface::Console, TEXT("console"), TEXT("Console"),
                FVector(-.32335, .6, 1.88635), FVector(1.8001, 0, .4185),
                { FVector(-.1641, 0, 1.8956) }, TEXT("console"));
            Console.Clearance = Box(FVector(-.1641, .6, 1.8956), FVector(.41, .56, .41));
            Out.Supports.Add(MoveTemp(Console));
            auto Shelf = Support(ESurface::Shelf, TEXT("shelf"), TEXT("Shelf"),
                FVector(-1.4196, .8177, 1.9881), FVector(.455, 0, .455),
                { FVector(-1.4191, 0, 1.9876) }, TEXT("shelf"));
            Shelf.Clearance = Box(FVector(-1.4191, .8177, 1.9876), FVector(.41, .56, .41));
            Out.Supports.Add(MoveTemp(Shelf));
            // Convert measured source bounds once into the canonical normalized runtime contract.
            const auto Measured = [&Out](const TCHAR* Id, const FVector2D& XBounds,
                const FVector2D& SourceYBounds, double Height, double Bottom = 0)
            {
                Obstacle(Out, Id, FVector((XBounds.X + XBounds.Y) / 2 - 1.6891, Bottom,
                    2.2606 - (SourceYBounds.X + SourceYBounds.Y) / 2),
                    FVector(XBounds.Y - XBounds.X, Height, SourceYBounds.Y - SourceYBounds.X), true);
            };
            Measured(TEXT("chair"), FVector2D(2.2003, 3.2187), FVector2D(.1324, 1.1943), .75);
            Measured(TEXT("console"), FVector2D(.4657, 2.2658), FVector2D(.165, .5835), .6);
            Measured(TEXT("shelf"), FVector2D(.023, .5326), FVector2D(.026, .5256), 1.775);
            Measured(TEXT("workstation"), FVector2D(1.2633, 3.3684), FVector2D(1.7087, 3.5597), .725);
            Measured(TEXT("desk-chair"), FVector2D(1.6062, 2.3273), FVector2D(2.3121, 3.0066), .9);
            Measured(TEXT("utility-rack"), FVector2D(0, 1.2309), FVector2D(2.362, 4.48), 1.968);
            Measured(TEXT("utility-products"), FVector2D(.0097, .3762), FVector2D(2.106, 3.4696), 1.555);
            Measured(TEXT("bamboo"), FVector2D(2.8585, 3.3513), FVector2D(.7452, 1.3084), 1.045);
            Measured(TEXT("console-nessino"), FVector2D(.6054, .9261), FVector2D(.1898, .5102), .223, .6);
            Measured(TEXT("console-camera"), FVector2D(1.0377, 1.2438), FVector2D(.3165, .495), .25, .6);
            Measured(TEXT("console-trailing-plant"), FVector2D(1.7976, 2.3108), FVector2D(.3604, .7457), .703);
            Measured(TEXT("console-fado"), FVector2D(2.02, 2.271), FVector2D(.175, .469), .35, .6);
            Out.Wide = Camera(FVector(-.839, 1.588, -1.249), FVector(-.427, 1.284, 1.707));
            Out.PortraitWide = Camera(FVector(-.8, 1.8, -1.6), FVector(-.8, .45, 1.2));
            Out.Close = Camera(FVector(-.8, 1.45, -.65), FVector(-.8, .5, 1.2));
            Out.PreferredSpawn = Pose(FVector(-.8, 0, 1.2), UE_DOUBLE_PI);
        }
        Out.CameraObstacles = Out.Obstacles;
        if (!bBedroom)
        {
            const auto CameraObstacle = [&Out](const TCHAR* Id, const FVector& Bottom, const FVector& Size)
            { Out.CameraObstacles.Add({ Id, Box(Bottom, Size), Bottom.Y }); };
            CameraObstacle(TEXT("display-left"), FVector(-.43322, .85334, -.932245), FVector(.35654, .37997, .56155));
            CameraObstacle(TEXT("display-center"), FVector(.604985, .8147, -1.00739), FVector(.50393, .36996, .44704));
            CameraObstacle(TEXT("display-right"), FVector(1.31612, .81906, -.67432), FVector(.44654, .37359, .54316));
            CameraObstacle(TEXT("softbox"), FVector(.61781, .63856, -1.060515), FVector(1.13406, 1.58299, 1.27697));
        }
        return Out;
    }

    FRoomShellPlan KitchenBathroomShell(bool bKitchen)
    {
        FRoomShellPlan Out;
        Out.DerivedLayoutId = bKitchen ? TEXT("modern-kitchen-v01") : TEXT("bathroom-v01");
        Bounds(Out, FVector(-2.6, 0, -2.6), FVector(2.6, 0, 2.6));
        Standing(Out, { -1.6, -.8, 0, .8, 1.6 }, { 1.6, 1.1, -.9, -1.5 });
        Floor(Out, FVector::ZeroVector, FVector(5.2, 0, 5.2));
        Wall(Out, ESurface::WallLeft, TEXT("wall-left"), bKitchen ? TEXT("Left wall") : TEXT("Bath wall"),
            FVector(-2.595, 0, 0), UE_DOUBLE_PI / 2, 5.1, 2.8);
        if (bKitchen)
        {
            Wall(Out, ESurface::WallBack, TEXT("wall-back"), TEXT("Counter wall"), FVector(0, 0, -2.595),
                0, 5.1, 2.8, { { -.4, 1.5, 3.3, 1.05 }, { -.25, .96, 3.85, .66 } });
            Out.Supports.Add(Support(ESurface::Console, TEXT("console"), TEXT("Counter"),
                FVector(-1.35, .94, -2.18), FVector(1.2, 0, .55), { FVector(-1.6, 0, -2.18) }, TEXT("counter")));
            Obstacle(Out, TEXT("island"), FVector::ZeroVector, FVector(2.1, .95, 1.05));
            Obstacle(Out, TEXT("counter"), FVector(-.25, 0, -2.2), FVector(3.85, .96, .7));
            Obstacle(Out, TEXT("fridge"), FVector(2.05, 0, -2.1), FVector(.8, 2.1, .85));
            Obstacle(Out, TEXT("island-seat"), FVector(1.4, 0, .15), FVector(.5, .85, .5));
            Out.PortraitWide = Camera(FVector(0, 2.1, 2.42), FVector(0, .7, .95));
        }
        else
        {
            Wall(Out, ESurface::WallBack, TEXT("wall-back"), TEXT("Vanity wall"), FVector(0, 0, -2.595),
                0, 5.1, 2.8, { { .9, 1.15, 1.6, 1.2 } });
            Out.Supports.Add(Support(ESurface::Console, TEXT("console"), TEXT("Vanity"),
                FVector(.9, .87, -2.18), FVector(1.6, 0, .55), { FVector(1.5, 0, -2.18) }, TEXT("vanity")));
            Obstacle(Out, TEXT("bath"), FVector(-1.05, 0, .75), FVector(1.05, .62, 2.05));
            Obstacle(Out, TEXT("vanity"), FVector(.9, 0, -2.18), FVector(1.7, .9, .65));
            Obstacle(Out, TEXT("shower"), FVector(-1.8, 0, -1.7), FVector(1.2, 2.2, 1.4));
            Out.PortraitWide = Camera(FVector(.6, 2.4, 2.42), FVector(-.5, .55, .8));
        }
        // Preserve canonical wall order as well as IDs.
        Out.Walls.Swap(0, 1);
        Out.CameraObstacles = Out.Obstacles;
        Out.Wide = Camera(FVector(.9, 2.3, 2.5), FVector(0, .65, .45));
        Out.Close = Camera(FVector(1.7, 1.65, 2.3), FVector(0, .7, 1.2));
        Out.PreferredSpawn = Pose(FVector(0, 0, 1.6), UE_DOUBLE_PI);
        Out.FieldOfView = 78; Out.CameraOrbitLimit = .8;
        return Out;
    }

    FRoomShellPlan LegacyShell(bool bTerrace)
    {
        FRoomShellPlan Out;
        Out.DerivedLayoutId = TEXT("legacy-room-v1");
        Out.bLegacyPlacement = true; Out.ObjectScale = 1;
        Out.CharacterScale = .57; Out.CharacterRadius = .3; Out.CharacterHeight = 1.45;
        Bounds(Out, FVector(-2.25, 0, -1.65), FVector(2.25, 0, 1.85));
        Standing(Out, { -.65, .65, 0, -1.2, 1.2 }, { 1.4, .65, 1.05 }, 0);
        auto Ground = Support(ESurface::Floor, TEXT("floor"), TEXT("Floor"),
            FVector(0, 0, .1), FVector(4.5, 0, 3.5), {});
        for (double X : { -1.45, -.7, 0.0, .75, 1.5 }) for (double Z : { -.65, .25, 1.05 })
            Ground.Anchors.Add(Pose(FVector(X, 0, Z)));
        Out.Supports.Add(MoveTemp(Ground));
        Out.Supports.Add(Support(ESurface::Console, TEXT("console"), TEXT("Console"),
            FVector(1.05, .78, -.72), FVector(1.75, 0, .9),
            { FVector(.65, 0, -.72), FVector(1.45, 0, -.72) }));
        Out.Supports.Add(Support(ESurface::Shelf, TEXT("shelf"), TEXT("Shelf"),
            FVector(-.95, 1.48, -1.2), FVector(1.9, 0, .82),
            { FVector(-1.4, 0, -1.2), FVector(-.5, 0, -1.2) }));
        Obstacle(Out, TEXT("chair"), FVector(-1.65, 0, .35), FVector(.95, 1.2, 1.25));
        Obstacle(Out, TEXT("console"), FVector(1.05, 0, -.72), FVector(1.75, .78, .9));
        if (!bTerrace)
        {
            Wall(Out, ESurface::WallBack, TEXT("wall-back"), TEXT("Back wall"), FVector(0, 0, -1.985),
                0, 5.1, 3.5, { { .15, .3, 1.4, 2.75 }, { 1.65, 1.45, .75, 1.05 } });
            Wall(Out, ESurface::WallLeft, TEXT("wall-left"), TEXT("Left wall"), FVector(-2.485, 0, .15),
                UE_DOUBLE_PI / 2, 4.5, 3.5);
        }
        Out.CameraObstacles = Out.Obstacles;
        Out.Wide = bTerrace ? Camera(FVector(5.2, 5.4, 13.8), FVector(0, .7, 0))
            : Camera(FVector(5.8, 5.2, 12.8), FVector(0, 1, 0));
        Out.Close = bTerrace ? Camera(FVector(1.8, 2.7, 6.8), FVector(0, 1.1, .5))
            : Camera(FVector(1.6, 2.5, 6.5), FVector(0, 1.2, .7));
        Out.PreferredSpawn = Pose(FVector(0, 0, .6));
        Out.FieldOfView = 42; Out.CameraOrbitLimit.Reset();
        Out.CameraPolicy = bTerrace ? ERoomCameraPolicy::OpenOrbit : ERoomCameraPolicy::LegacyOrbit;
        return Out;
    }

    void ApplyDesign(FRoomShellPlan& Out, const TOptional<FHomeDesignPlan>& Design)
    {
        const int32 Turns = Design.IsSet() ? Design.GetValue().Turns : 0;
        const double Extension = Design.IsSet() ? Design.GetValue().Extension : 0;
        const double Yaw = Turns * UE_DOUBLE_PI / 2;
        if (Design.IsSet()) Out.DerivedLayoutId += FString::Printf(TEXT(":home-v1:%d:%s"), Turns,
            Extension == .6 ? TEXT("0.6") : Extension == 1.2 ? TEXT("1.2") : TEXT("0"));
        Out.ShellYawRadians = Yaw; Out.ShellYawDegrees = SourceYawToUnreal(Yaw);
        Out.ShellExtensionMetres = Extension;
        const FVector A = TurnSourceShell(FVector(Out.MinX, 0, Out.MinZ), Turns);
        const FVector B = TurnSourceShell(FVector(Out.MaxX, 0, Out.MaxZ + Extension), Turns);
        Bounds(Out, FVector(FMath::Min(A.X, B.X), 0, FMath::Min(A.Z, B.Z)),
            FVector(FMath::Max(A.X, B.X), 0, FMath::Max(A.Z, B.Z)));
        const auto RotateBox = [Turns](FSourceBox& Value)
        {
            const FVector Size = Value.DimensionsMetres;
            Value = Box(TurnSourceShell(Value.BottomCentreMetres, Turns),
                Turns % 2 ? FVector(Size.Z, Size.Y, Size.X) : Size);
        };
        for (auto& Value : Out.Obstacles) RotateBox(Value.Box);
        for (auto& Value : Out.CameraObstacles) RotateBox(Value.Box);
        for (auto& Value : Out.FloorPatches) RotateBox(Value);
        if (Extension != 0) Standing(Out, { -1.6, -.8, 0, .8, 1.6 }, { 2 + Extension / 2 });
        for (auto& Value : Out.StandingCandidates)
            Value = Pose(TurnSourceShell(Value.Metres, Turns), Value.YawRadians + Yaw);
        for (auto& Value : Out.Supports)
        {
            if (Value.Id == ESurface::Floor)
            {
                Value.Top.BottomCentreMetres.Z += Extension / 2;
                Value.Top.DimensionsMetres.Z += Extension;
            }
            RotateBox(Value.Top);
            if (Value.Clearance.IsSet()) RotateBox(Value.Clearance.GetValue());
            // Extra standing candidates do not alter the saved support's anchor list.
            for (auto& Anchor : Value.Anchors) Anchor = Pose(TurnSourceShell(Anchor.Metres, Turns));
        }
        for (auto& Value : Out.Walls)
        {
            if (Value.Id == ESurface::WallLeft)
            { Value.Anchor.Metres.Z += Extension / 2; Value.Width += Extension; }
            Value.Anchor = Pose(TurnSourceShell(Value.Anchor.Metres, Turns), Value.Anchor.YawRadians + Yaw);
            const double C = FMath::Cos(Value.Anchor.YawRadians), S = FMath::Sin(Value.Anchor.YawRadians);
            for (auto& Opening : Value.Openings)
                Opening.UnrealBounds = Box(Value.Anchor.Metres + FVector(C * Opening.U, Opening.BottomY, -S * Opening.U),
                    FVector(FMath::Abs(C) * Opening.Width, Opening.Height, FMath::Abs(S) * Opening.Width)).UnrealBounds;
        }
        const auto MoveCamera = [Turns, Extension](FCameraReference& Value)
        {
            Value = Camera(TurnSourceShell(Value.EyeMetres + FVector(0, 0, Extension), Turns),
                TurnSourceShell(Value.TargetMetres, Turns));
        };
        MoveCamera(Out.Wide); MoveCamera(Out.Close);
        if (Out.PortraitWide.IsSet()) MoveCamera(Out.PortraitWide.GetValue());
        Out.PreferredSpawn = Pose(TurnSourceShell(Out.PreferredSpawn.Metres, Turns), Out.PreferredSpawn.YawRadians + Yaw);
    }
}

namespace BaddieCompanion::HomeMapping
{
    TOptional<FRoomShellPlan> RoomShell(const FRoomPlan& Room)
    {
        if (!Room.Environment.Effective.IsSet()) return {};
        FRoomShellPlan Out;
        if (!Room.Layout.Saved.IsSet())
            Out = LayoutMapping::LegacyShell(Room.Environment.Effective.GetValue() == EEnvironment::Terrace);
        else
        {
            if (Room.Environment.Effective.GetValue() != EEnvironment::Studio || !Room.Layout.Effective.IsSet()) return {};
            switch (Room.Layout.Effective.GetValue())
            {
            case ELayout::PinkBedroomV2:
                Out = BedroomShell(Room.SavedDesign);
                Out.CameraObstacles = Out.Obstacles;
                return Out;
            case ELayout::PatStudioV1: Out = LayoutMapping::StudioShell(false); break;
            case ELayout::PinkBedroomV1: Out = LayoutMapping::StudioShell(true); break;
            case ELayout::KitchenV1: Out = LayoutMapping::KitchenBathroomShell(true); break;
            case ELayout::BathroomV1: Out = LayoutMapping::KitchenBathroomShell(false); break;
            default: return {};
            }
        }
        LayoutMapping::ApplyDesign(Out, Room.SavedDesign);
        return Out;
    }
}
