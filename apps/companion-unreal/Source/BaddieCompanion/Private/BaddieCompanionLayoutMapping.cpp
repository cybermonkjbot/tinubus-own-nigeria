#include "BaddieCompanionHomeMappingInternal.h"

namespace BaddieCompanion::HomeMapping::LayoutMapping
{
    FSupportPlan Support(ESurface Id, const TCHAR* SourceId, const TCHAR* Name,
        const FVector& Top, const FVector& Size, std::initializer_list<FVector> Anchors,
        const TCHAR* SupportingObstacle = nullptr)
    {
        FSupportPlan Out;
        Out.Id = Id; Out.SourceId = SourceId; Out.Name = Name;
        Out.Top = Box(Top, Size); Out.AllowedKinds = SupportKinds(Id);
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
        FLegacyMappingWork Work;
        FStudioShellMapping Mapping;
        Mapping.Begin(Out, bBedroom);
        Mapping.Advance(Work);
        return Out;
    }

    FRoomShellPlan KitchenBathroomShell(bool bKitchen)
    {
        FRoomShellPlan Out;
        FLegacyMappingWork Work;
        FKitchenBathroomMapping Mapping;
        Mapping.Begin(Out, bKitchen);
        Mapping.Advance(Work);
        return Out;
    }

    FRoomShellPlan LegacyShell(bool bTerrace)
    {
        FRoomShellPlan Out;
        FLegacyMappingWork Work;
        FLegacyShellMapping Mapping;
        Mapping.Begin(Out, bTerrace);
        Mapping.Advance(Work);
        return Out;
    }

    void ApplyDesign(FRoomShellPlan& Out, const TOptional<FHomeDesignPlan>& Design)
    {
        FLegacyMappingWork Work;
        FDesignMapping Mapping;
        Mapping.Begin(Out, Design);
        Mapping.Advance(Work);
    }
}

namespace BaddieCompanion::HomeMapping
{
    void FStudioShellMapping::Begin(FRoomShellPlan& Shell, bool bInBedroom)
    {
        Output = &Shell; bBedroom = bInBedroom; Phase = Index = Child = 0;
    }
    EMappingStep FStudioShellMapping::Advance(FMappingWork& Work)
    {
        if (!Output) return EMappingStep::Invalid;
        auto& Out = *Output;
        static constexpr double FloorX[] = { -.8, -.2, .4, 1, -1.3 };
        static constexpr double FloorZ[] = { 1.2, .6, 0, -.6, -1.2, -1.8 };
        while (Phase < 11)
        {
            uint64 Capacity = 0;
            if (Phase == 0) Capacity = 32 * sizeof(TCHAR);
            if (Phase == 1) Capacity = (Index + 1) * sizeof(FSourcePose);
            if (Phase == 2) Capacity = (Index + 1) * sizeof(FSupportPlan) + 64 * sizeof(TCHAR);
            if (Phase == 3) Capacity = (Child + 1) * sizeof(EItemKind);
            if (Phase == 4) Capacity = (Child + 1) * sizeof(FSourcePose);
            if (Phase == 5) Capacity = sizeof(FString) + 32 * sizeof(TCHAR);
            if (Phase == 6) Capacity = sizeof(FWallPlan) + 64 * sizeof(TCHAR);
            if (Phase >= 7 && Phase <= 9) Capacity = (Out.Obstacles.Num() + Index + 1) * sizeof(FLayoutObstacle) + 32 * sizeof(TCHAR);
            const auto Admission = Work.Admit(512, Capacity);
            if (Admission != EMappingStep::Complete) return Admission;
            switch (Phase)
            {
            case 0:
                Out.DerivedLayoutId = bBedroom ? TEXT("pink-bedroom-v01") : TEXT("pat-studio-v01");
                LayoutMapping::Bounds(Out, FVector(-1.6891, 0, -2.2606), FVector(1.6891, 0, 2.2606));
                break;
            case 1:
                if (Index < 30)
                {
                    static constexpr double BedroomX[] = { -.4, .2, -.95, .8, 1.35 };
                    static constexpr double BedroomZ[] = { -1.1, -.5, -1.7, .1, .7, 1.3 };
                    Out.StandingCandidates.Add(Pose(FVector(bBedroom ? BedroomX[Index % 5] : FloorX[Index % 5],
                        0, bBedroom ? BedroomZ[Index / 5] : FloorZ[Index / 5]), UE_DOUBLE_PI));
                    ++Index; continue;
                }
                break;
            case 2:
                {
                    auto& Support = Out.Supports.AddDefaulted_GetRef();
                    Support.Id = Index == 0 ? ESurface::Floor : Index == 1 ? ESurface::Console : ESurface::Shelf;
                    Support.SourceId = Index == 0 ? TEXT("floor") : Index == 1 ? TEXT("console") : TEXT("shelf");
                    Support.Name = Index == 0 ? TEXT("Floor") : Index == 1 && bBedroom ? TEXT("Dresser")
                        : Index == 1 ? TEXT("Console") : TEXT("Shelf");
                    if (Index == 0) Support.Top = Box(FVector::ZeroVector, FVector(3.3782, 0, 4.5212));
                    else if (Index == 1) Support.Top = bBedroom
                        ? Box(FVector(1.3009, .81, -1.3994), FVector(.50, 0, 1.18))
                        : Box(FVector(-.32335, .6, 1.88635), FVector(1.8001, 0, .4185));
                    else Support.Top = bBedroom
                        ? Box(FVector(-1.4191, 1.4225, -1.6094), FVector(.44, 0, .88))
                        : Box(FVector(-1.4196, .8177, 1.9881), FVector(.455, 0, .455));
                }
                break;
            case 3:
                {
                    EItemKind Kind;
                    if (SupportKindAt(Out.Supports.Last().Id, Child, Kind))
                    { Out.Supports.Last().AllowedKinds.Add(Kind); ++Child; continue; }
                }
                break;
            case 4:
                if (Index == 0 && Child < 30)
                {
                    // Both sourceFloor recipes use these anchors, including bedroom v01.
                    Out.Supports.Last().Anchors.Add(Pose(FVector(FloorX[Child % 5], 0, FloorZ[Child / 5])));
                    ++Child; continue;
                }
                if (Index != 0 && Child < (Index == 1 && bBedroom ? 2u : 1u))
                {
                    FVector Anchor;
                    if (Index == 1) Anchor = bBedroom ? FVector(1.285, 0, Child == 0 ? -1.65 : -1.12)
                        : FVector(-.1641, 0, 1.8956);
                    else Anchor = bBedroom ? FVector(-1.42, 0, -1.65) : FVector(-1.4191, 0, 1.9876);
                    Out.Supports.Last().Anchors.Add(Pose(Anchor)); ++Child; continue;
                }
                break;
            case 5:
                if (Index != 0)
                {
                    auto& Support = Out.Supports.Last();
                    Support.SupportingObstacleIds.Add(Index == 1 && bBedroom ? TEXT("dresser")
                        : Index == 1 ? TEXT("console") : TEXT("shelf"));
                    if (!bBedroom) Support.Clearance = Index == 1
                        ? Box(FVector(-.1641, .6, 1.8956), FVector(.41, .56, .41))
                        : Box(FVector(-1.4191, .8177, 1.9876), FVector(.41, .56, .41));
                }
                if (++Index < 3) { Phase = 2; Child = 0; continue; }
                Index = 0;
                break;
            case 6:
                LayoutMapping::Wall(Out, ESurface::WallFront, TEXT("wall-front"), TEXT("Console wall"),
                    FVector(0, 0, 2.2556), UE_DOUBLE_PI, 3.3, 2.7);
                break;
            case 7:
                if (bBedroom && Index < 5)
                {
                    switch (Index)
                    {
                    case 0: LayoutMapping::Obstacle(Out, TEXT("bed"), FVector(-.165, 0, 1.045), FVector(1.63, 1.25, 2.15)); break;
                    case 1: LayoutMapping::Obstacle(Out, TEXT("bedside-left"), FVector(-1.25, 0, 1.695), FVector(.46, .75, .49)); break;
                    case 2: LayoutMapping::Obstacle(Out, TEXT("bedside-right"), FVector(.94, 0, 1.695), FVector(.48, .75, .49)); break;
                    case 3: LayoutMapping::Obstacle(Out, TEXT("dresser"), FVector(1.285, 0, -1.4), FVector(.55, .81, 1.2)); break;
                    case 4: LayoutMapping::Obstacle(Out, TEXT("shelf"), FVector(-1.42, 0, -1.61), FVector(.44, 1.7, .88)); break;
                    }
                    ++Index; continue;
                }
                if (!bBedroom && Index < 12)
                {
                    struct FMeasured { const TCHAR* Id; double MinX, MaxX, MinY, MaxY, Height, Bottom; };
                    static const FMeasured Rows[] = {
                        { TEXT("chair"), 2.2003, 3.2187, .1324, 1.1943, .75, 0 },
                        { TEXT("console"), .4657, 2.2658, .165, .5835, .6, 0 },
                        { TEXT("shelf"), .023, .5326, .026, .5256, 1.775, 0 },
                        { TEXT("workstation"), 1.2633, 3.3684, 1.7087, 3.5597, .725, 0 },
                        { TEXT("desk-chair"), 1.6062, 2.3273, 2.3121, 3.0066, .9, 0 },
                        { TEXT("utility-rack"), 0, 1.2309, 2.362, 4.48, 1.968, 0 },
                        { TEXT("utility-products"), .0097, .3762, 2.106, 3.4696, 1.555, 0 },
                        { TEXT("bamboo"), 2.8585, 3.3513, .7452, 1.3084, 1.045, 0 },
                        { TEXT("console-nessino"), .6054, .9261, .1898, .5102, .223, .6 },
                        { TEXT("console-camera"), 1.0377, 1.2438, .3165, .495, .25, .6 },
                        { TEXT("console-trailing-plant"), 1.7976, 2.3108, .3604, .7457, .703, 0 },
                        { TEXT("console-fado"), 2.02, 2.271, .175, .469, .35, .6 }
                    };
                    const auto& Row = Rows[Index++];
                    LayoutMapping::Obstacle(Out, Row.Id, FVector((Row.MinX + Row.MaxX) / 2 - 1.6891,
                        Row.Bottom, 2.2606 - (Row.MinY + Row.MaxY) / 2),
                        FVector(Row.MaxX - Row.MinX, Row.Height, Row.MaxY - Row.MinY), true);
                    continue;
                }
                break;
            case 8:
                if (Index < static_cast<uint32>(Out.Obstacles.Num()))
                { Out.CameraObstacles.Add(Out.Obstacles[Index++]); continue; }
                break;
            case 9:
                if (!bBedroom && Index < 4)
                {
                    struct FCameraObstacle { const TCHAR* Id; FVector Bottom, Size; };
                    static const FCameraObstacle Rows[] = {
                        { TEXT("display-left"), FVector(-.43322, .85334, -.932245), FVector(.35654, .37997, .56155) },
                        { TEXT("display-center"), FVector(.604985, .8147, -1.00739), FVector(.50393, .36996, .44704) },
                        { TEXT("display-right"), FVector(1.31612, .81906, -.67432), FVector(.44654, .37359, .54316) },
                        { TEXT("softbox"), FVector(.61781, .63856, -1.060515), FVector(1.13406, 1.58299, 1.27697) }
                    };
                    const auto& Row = Rows[Index++];
                    Out.CameraObstacles.Add({ Row.Id, Box(Row.Bottom, Row.Size), Row.Bottom.Y });
                    continue;
                }
                break;
            case 10:
                if (Index == 0) Out.Wide = bBedroom
                    ? LayoutMapping::Camera(FVector(-.8, 1.65, -2.02), FVector(0, .8, .25))
                    : LayoutMapping::Camera(FVector(-.839, 1.588, -1.249), FVector(-.427, 1.284, 1.707));
                else if (Index == 1)
                {
                    if (!bBedroom) Out.PortraitWide = LayoutMapping::Camera(FVector(-.8, 1.8, -1.6), FVector(-.8, .45, 1.2));
                }
                else if (Index == 2) Out.Close = bBedroom
                    ? LayoutMapping::Camera(FVector(-.75, 1.35, -1.9), FVector(-.4, .75, -1.1))
                    : LayoutMapping::Camera(FVector(-.8, 1.45, -.65), FVector(-.8, .5, 1.2));
                else
                {
                    Out.PreferredSpawn = Pose(bBedroom ? FVector(-.4, 0, -1.1) : FVector(-.8, 0, 1.2), UE_DOUBLE_PI);
                    break;
                }
                ++Index; continue;
            }
            ++Phase;
            if (Phase < 3 || Phase > 5) Index = 0;
            Child = 0;
        }
        return EMappingStep::Complete;
    }
    void FLegacyShellMapping::Begin(FRoomShellPlan& Shell, bool bInTerrace)
    {
        Output = &Shell; bTerrace = bInTerrace; Phase = Index = Child = 0;
    }
    EMappingStep FLegacyShellMapping::Advance(FMappingWork& Work)
    {
        if (!Output) return EMappingStep::Invalid;
        auto& Out = *Output;
        while (Phase < 11)
        {
            uint64 Capacity = 0;
            if (Phase == 0) Capacity = 32 * sizeof(TCHAR);
            if (Phase == 1) Capacity = (Index + 1) * sizeof(FSourcePose);
            if (Phase == 2) Capacity = (Index + 1) * sizeof(FSupportPlan) + 64 * sizeof(TCHAR);
            if (Phase == 3) Capacity = (Child + 1) * sizeof(EItemKind);
            if (Phase == 4) Capacity = (Child + 1) * sizeof(FSourcePose);
            if (Phase == 5 || Phase == 9) Capacity = (Index + 1) * sizeof(FLayoutObstacle) + 32 * sizeof(TCHAR);
            if (Phase == 6 || Phase == 8) Capacity = (Out.Walls.Num() + 1) * sizeof(FWallPlan) + 64 * sizeof(TCHAR);
            if (Phase == 7) Capacity = (Index + 1) * sizeof(FWallOpening);
            const auto Admission = Work.Admit(512, Capacity);
            if (Admission != EMappingStep::Complete) return Admission;
            switch (Phase)
            {
            case 0:
                Out.DerivedLayoutId = TEXT("legacy-room-v1");
                Out.bLegacyPlacement = true; Out.ObjectScale = 1;
                Out.CharacterScale = .57; Out.CharacterRadius = .3; Out.CharacterHeight = 1.45;
                LayoutMapping::Bounds(Out, FVector(-2.25, 0, -1.65), FVector(2.25, 0, 1.85));
                break;
            case 1:
                if (Index < 15)
                {
                    static constexpr double X[] = { -.65, .65, 0, -1.2, 1.2 };
                    static constexpr double Z[] = { 1.4, .65, 1.05 };
                    Out.StandingCandidates.Add(Pose(FVector(X[Index % 5], 0, Z[Index / 5])));
                    ++Index; continue;
                }
                break;
            case 2:
                {
                    auto& Support = Out.Supports.AddDefaulted_GetRef();
                    Support.Id = Index == 0 ? ESurface::Floor : Index == 1 ? ESurface::Console : ESurface::Shelf;
                    Support.SourceId = Index == 0 ? TEXT("floor") : Index == 1 ? TEXT("console") : TEXT("shelf");
                    Support.Name = Index == 0 ? TEXT("Floor") : Index == 1 ? TEXT("Console") : TEXT("Shelf");
                    Support.Top = Index == 0 ? Box(FVector(0, 0, .1), FVector(4.5, 0, 3.5))
                        : Index == 1 ? Box(FVector(1.05, .78, -.72), FVector(1.75, 0, .9))
                        : Box(FVector(-.95, 1.48, -1.2), FVector(1.9, 0, .82));
                }
                break;
            case 3:
                {
                    EItemKind Kind;
                    if (SupportKindAt(Out.Supports.Last().Id, Child, Kind))
                    { Out.Supports.Last().AllowedKinds.Add(Kind); ++Child; continue; }
                }
                break;
            case 4:
                if (Index == 0 && Child < 15)
                {
                    static constexpr double X[] = { -1.45, -.7, 0.0, .75, 1.5 };
                    static constexpr double Z[] = { -.65, .25, 1.05 };
                    // Ground anchors use the original X-major ordering.
                    Out.Supports.Last().Anchors.Add(Pose(FVector(X[Child / 3], 0, Z[Child % 3])));
                    ++Child; continue;
                }
                if (Index != 0 && Child < 2)
                {
                    const double X = Index == 1 ? (Child == 0 ? .65 : 1.45) : (Child == 0 ? -1.4 : -.5);
                    Out.Supports.Last().Anchors.Add(Pose(FVector(X, 0, Index == 1 ? -.72 : -1.2)));
                    ++Child; continue;
                }
                if (++Index < 3) { Phase = 2; Child = 0; continue; }
                Index = 0;
                break;
            case 5:
                if (Index < 2)
                {
                    if (Index == 0) LayoutMapping::Obstacle(Out, TEXT("chair"), FVector(-1.65, 0, .35), FVector(.95, 1.2, 1.25));
                    else LayoutMapping::Obstacle(Out, TEXT("console"), FVector(1.05, 0, -.72), FVector(1.75, .78, .9));
                    ++Index; continue;
                }
                break;
            case 6:
                if (!bTerrace) LayoutMapping::Wall(Out, ESurface::WallBack, TEXT("wall-back"), TEXT("Back wall"),
                    FVector(0, 0, -1.985), 0, 5.1, 3.5);
                break;
            case 7:
                if (!bTerrace && Index < 2)
                {
                    FWallOpening Opening;
                    if (Index == 0) { Opening.U = .15; Opening.BottomY = .3; Opening.Width = 1.4; Opening.Height = 2.75; }
                    else { Opening.U = 1.65; Opening.BottomY = 1.45; Opening.Width = .75; Opening.Height = 1.05; }
                    Out.Walls.Last().Openings.Add(Opening); ++Index; continue;
                }
                break;
            case 8:
                if (!bTerrace) LayoutMapping::Wall(Out, ESurface::WallLeft, TEXT("wall-left"), TEXT("Left wall"),
                    FVector(-2.485, 0, .15), UE_DOUBLE_PI / 2, 4.5, 3.5);
                break;
            case 9:
                if (Index < static_cast<uint32>(Out.Obstacles.Num()))
                { Out.CameraObstacles.Add(Out.Obstacles[Index++]); continue; }
                break;
            case 10:
                if (Index == 0) Out.Wide = bTerrace ? LayoutMapping::Camera(FVector(5.2, 5.4, 13.8), FVector(0, .7, 0))
                    : LayoutMapping::Camera(FVector(5.8, 5.2, 12.8), FVector(0, 1, 0));
                else if (Index == 1) Out.Close = bTerrace ? LayoutMapping::Camera(FVector(1.8, 2.7, 6.8), FVector(0, 1.1, .5))
                    : LayoutMapping::Camera(FVector(1.6, 2.5, 6.5), FVector(0, 1.2, .7));
                else
                {
                    Out.PreferredSpawn = Pose(FVector(0, 0, .6)); Out.FieldOfView = 42; Out.CameraOrbitLimit.Reset();
                    Out.CameraPolicy = bTerrace ? ERoomCameraPolicy::OpenOrbit : ERoomCameraPolicy::LegacyOrbit;
                    break;
                }
                ++Index; continue;
            }
            ++Phase;
            if (Phase < 3 || Phase > 4) Index = 0;
            Child = 0;
        }
        return EMappingStep::Complete;
    }
    void FKitchenBathroomMapping::Begin(FRoomShellPlan& Shell, bool bInKitchen)
    {
        Output = &Shell; bKitchen = bInKitchen; Phase = Index = Child = 0;
    }
    EMappingStep FKitchenBathroomMapping::Advance(FMappingWork& Work)
    {
        if (!Output) return EMappingStep::Invalid;
        auto& Out = *Output;
        while (Phase < 14)
        {
            uint64 Capacity = 0;
            if (Phase == 0) Capacity = 32 * sizeof(TCHAR);
            if (Phase == 1) Capacity = (Index + 1) * sizeof(FSourcePose);
            if (Phase == 2 || Phase == 8) Capacity = (Out.Supports.Num() + 1) * sizeof(FSupportPlan) + 64 * sizeof(TCHAR);
            if (Phase == 3 || Phase == 9) Capacity = (Child + 1) * sizeof(EItemKind);
            if (Phase == 4 || Phase == 10) Capacity = (Child + 1) * sizeof(FSourcePose);
            if (Phase == 5 || Phase == 7) Capacity = (Out.Walls.Num() + 1) * sizeof(FWallPlan) + 64 * sizeof(TCHAR);
            if (Phase == 6) Capacity = (Index + 1) * sizeof(FWallOpening);
            if (Phase == 11 || Phase == 12) Capacity = (Index + 1) * sizeof(FLayoutObstacle) + 32 * sizeof(TCHAR);
            const auto Admission = Work.Admit(512, Capacity);
            if (Admission != EMappingStep::Complete) return Admission;
            switch (Phase)
            {
            case 0:
                Out.DerivedLayoutId = bKitchen ? TEXT("modern-kitchen-v01") : TEXT("bathroom-v01");
                LayoutMapping::Bounds(Out, FVector(-2.6, 0, -2.6), FVector(2.6, 0, 2.6));
                break;
            case 1:
                if (Index < 20)
                {
                    static constexpr double X[] = { -1.6, -.8, 0, .8, 1.6 };
                    static constexpr double Z[] = { 1.6, 1.1, -.9, -1.5 };
                    Out.StandingCandidates.Add(Pose(FVector(X[Index % 5], 0, Z[Index / 5]), UE_DOUBLE_PI));
                    ++Index; continue;
                }
                break;
            case 2:
                {
                    auto& Floor = Out.Supports.AddDefaulted_GetRef();
                    Floor.Id = ESurface::Floor; Floor.SourceId = TEXT("floor"); Floor.Name = TEXT("Floor");
                    Floor.Top = Box(FVector::ZeroVector, FVector(5.2, 0, 5.2));
                }
                break;
            case 3:
            case 9:
                {
                    EItemKind Kind;
                    if (SupportKindAt(Out.Supports.Last().Id, Child, Kind))
                    { Out.Supports.Last().AllowedKinds.Add(Kind); ++Child; continue; }
                }
                break;
            case 4:
                if (Child < static_cast<uint32>(Out.StandingCandidates.Num()))
                { Out.Supports.Last().Anchors.Add(Pose(Out.StandingCandidates[Child++].Metres)); continue; }
                break;
            case 5:
                // Back then left is the legacy result after its original Swap.
                LayoutMapping::Wall(Out, ESurface::WallBack, TEXT("wall-back"),
                    bKitchen ? TEXT("Counter wall") : TEXT("Vanity wall"), FVector(0, 0, -2.595), 0, 5.1, 2.8);
                break;
            case 6:
                if (Index < (bKitchen ? 2u : 1u))
                {
                    FWallOpening Opening;
                    if (!bKitchen) { Opening.U = .9; Opening.BottomY = 1.15; Opening.Width = 1.6; Opening.Height = 1.2; }
                    else if (Index == 0) { Opening.U = -.4; Opening.BottomY = 1.5; Opening.Width = 3.3; Opening.Height = 1.05; }
                    else { Opening.U = -.25; Opening.BottomY = .96; Opening.Width = 3.85; Opening.Height = .66; }
                    Out.Walls.Last().Openings.Add(Opening); ++Index; continue;
                }
                break;
            case 7:
                LayoutMapping::Wall(Out, ESurface::WallLeft, TEXT("wall-left"),
                    bKitchen ? TEXT("Left wall") : TEXT("Bath wall"), FVector(-2.595, 0, 0), UE_DOUBLE_PI / 2, 5.1, 2.8);
                break;
            case 8:
                {
                    auto& Console = Out.Supports.AddDefaulted_GetRef();
                    Console.Id = ESurface::Console; Console.SourceId = TEXT("console");
                    Console.Name = bKitchen ? TEXT("Counter") : TEXT("Vanity");
                    Console.Top = bKitchen ? Box(FVector(-1.35, .94, -2.18), FVector(1.2, 0, .55))
                        : Box(FVector(.9, .87, -2.18), FVector(1.6, 0, .55));
                }
                break;
            case 10:
                if (Child == 0)
                {
                    Out.Supports.Last().Anchors.Add(Pose(bKitchen ? FVector(-1.6, 0, -2.18) : FVector(1.5, 0, -2.18)));
                    ++Child; continue;
                }
                Out.Supports.Last().SupportingObstacleIds.Add(bKitchen ? TEXT("counter") : TEXT("vanity"));
                break;
            case 11:
                if (Index < (bKitchen ? 4u : 3u))
                {
                    if (bKitchen)
                    {
                        switch (Index)
                        {
                        case 0: LayoutMapping::Obstacle(Out, TEXT("island"), FVector::ZeroVector, FVector(2.1, .95, 1.05)); break;
                        case 1: LayoutMapping::Obstacle(Out, TEXT("counter"), FVector(-.25, 0, -2.2), FVector(3.85, .96, .7)); break;
                        case 2: LayoutMapping::Obstacle(Out, TEXT("fridge"), FVector(2.05, 0, -2.1), FVector(.8, 2.1, .85)); break;
                        case 3: LayoutMapping::Obstacle(Out, TEXT("island-seat"), FVector(1.4, 0, .15), FVector(.5, .85, .5)); break;
                        }
                    }
                    else
                    {
                        switch (Index)
                        {
                        case 0: LayoutMapping::Obstacle(Out, TEXT("bath"), FVector(-1.05, 0, .75), FVector(1.05, .62, 2.05)); break;
                        case 1: LayoutMapping::Obstacle(Out, TEXT("vanity"), FVector(.9, 0, -2.18), FVector(1.7, .9, .65)); break;
                        case 2: LayoutMapping::Obstacle(Out, TEXT("shower"), FVector(-1.8, 0, -1.7), FVector(1.2, 2.2, 1.4)); break;
                        }
                    }
                    ++Index; continue;
                }
                break;
            case 12:
                if (Index < static_cast<uint32>(Out.Obstacles.Num()))
                { Out.CameraObstacles.Add(Out.Obstacles[Index++]); continue; }
                break;
            case 13:
                if (Index == 0)
                    Out.PortraitWide = bKitchen ? LayoutMapping::Camera(FVector(0, 2.1, 2.42), FVector(0, .7, .95))
                        : LayoutMapping::Camera(FVector(.6, 2.4, 2.42), FVector(-.5, .55, .8));
                else if (Index == 1) Out.Wide = LayoutMapping::Camera(FVector(.9, 2.3, 2.5), FVector(0, .65, .45));
                else if (Index == 2) Out.Close = LayoutMapping::Camera(FVector(1.7, 1.65, 2.3), FVector(0, .7, 1.2));
                else
                {
                    Out.PreferredSpawn = Pose(FVector(0, 0, 1.6), UE_DOUBLE_PI);
                    Out.FieldOfView = 78; Out.CameraOrbitLimit = .8;
                    break;
                }
                ++Index; continue;
            }
            ++Phase; Index = Child = 0;
        }
        return EMappingStep::Complete;
    }
    void FDesignMapping::Begin(FRoomShellPlan& Shell, const TOptional<FHomeDesignPlan>& Design)
    {
        Output = &Shell; bHasDesign = Design.IsSet();
        Turns = Design.IsSet() ? Design->Turns : 0;
        Extension = Design.IsSet() ? Design->Extension : 0;
        Yaw = Turns * UE_DOUBLE_PI / 2;
        Phase = Index = Child = 0;
    }
    EMappingStep FDesignMapping::Advance(FMappingWork& Work)
    {
        if (!Output) return EMappingStep::Invalid;
        auto& Out = *Output;
        const auto RotateBox = [this](FSourceBox& Value)
        {
            const FVector Size = Value.DimensionsMetres;
            Value = Box(TurnSourceShell(Value.BottomCentreMetres, Turns),
                Turns % 2 ? FVector(Size.Z, Size.Y, Size.X) : Size);
        };
        while (Phase < 13)
        {
            uint64 Capacity = 0;
            if (Phase == 0 && bHasDesign) Capacity = (Out.DerivedLayoutId.Len() + 24) * sizeof(TCHAR);
            if (Phase == 5 && Extension != 0 && Index < 5)
                Capacity = (Out.StandingCandidates.Num() + 1) * sizeof(FSourcePose);
            const auto Admission = Work.Admit(256 + (Phase == 0 ? 4 * static_cast<uint64>(Out.DerivedLayoutId.Len()) : 0), Capacity);
            if (Admission != EMappingStep::Complete) return Admission;
            switch (Phase)
            {
            case 0:
                if (bHasDesign) Out.DerivedLayoutId += FString::Printf(TEXT(":home-v1:%d:%s"), Turns,
                    Extension == .6 ? TEXT("0.6") : Extension == 1.2 ? TEXT("1.2") : TEXT("0"));
                Out.ShellYawRadians = Yaw; Out.ShellYawDegrees = SourceYawToUnreal(Yaw);
                Out.ShellExtensionMetres = Extension;
                break;
            case 1:
                {
                    const FVector A = TurnSourceShell(FVector(Out.MinX, 0, Out.MinZ), Turns);
                    const FVector B = TurnSourceShell(FVector(Out.MaxX, 0, Out.MaxZ + Extension), Turns);
                    LayoutMapping::Bounds(Out, FVector(FMath::Min(A.X, B.X), 0, FMath::Min(A.Z, B.Z)),
                        FVector(FMath::Max(A.X, B.X), 0, FMath::Max(A.Z, B.Z)));
                }
                break;
            case 2:
                if (Index < static_cast<uint32>(Out.Obstacles.Num())) { RotateBox(Out.Obstacles[Index++].Box); continue; }
                break;
            case 3:
                if (Index < static_cast<uint32>(Out.CameraObstacles.Num())) { RotateBox(Out.CameraObstacles[Index++].Box); continue; }
                break;
            case 4:
                if (Index < static_cast<uint32>(Out.FloorPatches.Num())) { RotateBox(Out.FloorPatches[Index++]); continue; }
                break;
            case 5:
                if (Extension != 0 && Index < 5)
                {
                    static constexpr double X[] = { -1.6, -.8, 0, .8, 1.6 };
                    Out.StandingCandidates.Add(Pose(FVector(X[Index++], 0, 2 + Extension / 2), UE_DOUBLE_PI));
                    continue;
                }
                break;
            case 6:
                if (Index < static_cast<uint32>(Out.StandingCandidates.Num()))
                {
                    auto& Value = Out.StandingCandidates[Index++];
                    Value = Pose(TurnSourceShell(Value.Metres, Turns), Value.YawRadians + Yaw);
                    continue;
                }
                break;
            case 7:
                if (Index < static_cast<uint32>(Out.Supports.Num()))
                {
                    auto& Value = Out.Supports[Index];
                    if (Value.Id == ESurface::Floor)
                    { Value.Top.BottomCentreMetres.Z += Extension / 2; Value.Top.DimensionsMetres.Z += Extension; }
                    RotateBox(Value.Top);
                    if (Value.Clearance.IsSet()) RotateBox(Value.Clearance.GetValue());
                    Phase = 8; Child = 0; continue;
                }
                Phase = 9; Index = 0; continue;
            case 8:
                if (Child < static_cast<uint32>(Out.Supports[Index].Anchors.Num()))
                {
                    auto& Anchor = Out.Supports[Index].Anchors[Child++];
                    Anchor = Pose(TurnSourceShell(Anchor.Metres, Turns));
                    continue;
                }
                ++Index; Phase = 7; continue;
            case 9:
                if (Index < static_cast<uint32>(Out.Walls.Num()))
                {
                    auto& Value = Out.Walls[Index];
                    if (Value.Id == ESurface::WallLeft) { Value.Anchor.Metres.Z += Extension / 2; Value.Width += Extension; }
                    Value.Anchor = Pose(TurnSourceShell(Value.Anchor.Metres, Turns), Value.Anchor.YawRadians + Yaw);
                    Phase = 10; Child = 0; continue;
                }
                Phase = 11; Index = 0; continue;
            case 10:
                if (Child < static_cast<uint32>(Out.Walls[Index].Openings.Num()))
                {
                    auto& Value = Out.Walls[Index]; auto& Opening = Value.Openings[Child++];
                    const double C = FMath::Cos(Value.Anchor.YawRadians), S = FMath::Sin(Value.Anchor.YawRadians);
                    Opening.UnrealBounds = Box(Value.Anchor.Metres + FVector(C * Opening.U, Opening.BottomY, -S * Opening.U),
                        FVector(FMath::Abs(C) * Opening.Width, Opening.Height, FMath::Abs(S) * Opening.Width)).UnrealBounds;
                    continue;
                }
                ++Index; Phase = 9; continue;
            case 11:
                if (Index < 3)
                {
                    FCameraReference* Value = Index == 0 ? &Out.Wide : Index == 1 ? &Out.Close
                        : Out.PortraitWide.IsSet() ? &Out.PortraitWide.GetValue() : nullptr;
                    if (Value) *Value = LayoutMapping::Camera(TurnSourceShell(Value->EyeMetres + FVector(0, 0, Extension), Turns),
                        TurnSourceShell(Value->TargetMetres, Turns));
                    ++Index; continue;
                }
                break;
            case 12:
                Out.PreferredSpawn = Pose(TurnSourceShell(Out.PreferredSpawn.Metres, Turns), Out.PreferredSpawn.YawRadians + Yaw);
                break;
            }
            ++Phase; Index = Child = 0;
        }
        return EMappingStep::Complete;
    }
    void FRoomShellMapping::Begin(const FRoomPlan& Room, TOptional<FRoomShellPlan>& Shell)
    {
        Source = &Room; Output = &Shell; Phase = 0; bLegacy = false;
    }
    EMappingStep FRoomShellMapping::Advance(FMappingWork& Work)
    {
        if (!Source || !Output) return EMappingStep::Invalid;
        if (Phase == 0)
        {
            const auto Admission = Work.Admit(64);
            if (Admission != EMappingStep::Complete) return Admission;
            if (Output->IsSet()) return EMappingStep::Invalid;
            if (!Source->Environment.Effective.IsSet()) { Phase = 4; return EMappingStep::Complete; }
            bLegacy = !Source->Layout.Saved.IsSet();
            if (!bLegacy && (Source->Environment.Effective.GetValue() != EEnvironment::Studio
                || !Source->Layout.Effective.IsSet())) { Phase = 4; return EMappingStep::Complete; }
            if (!bLegacy)
            {
                Layout = Source->Layout.Effective.GetValue();
                if (Layout != ELayout::PinkBedroomV2 && Layout != ELayout::PatStudioV1
                    && Layout != ELayout::PinkBedroomV1 && Layout != ELayout::KitchenV1
                    && Layout != ELayout::BathroomV1) { Phase = 4; return EMappingStep::Complete; }
            }
            Phase = 1;
        }
        if (Phase == 1)
        {
            const auto Admission = Work.Admit(sizeof(FRoomShellPlan), sizeof(FRoomShellPlan));
            if (Admission != EMappingStep::Complete) return Admission;
            Output->Emplace();
            auto& Shell = Output->GetValue();
            if (bLegacy) Legacy.Begin(Shell, Source->Environment.Effective.GetValue() == EEnvironment::Terrace);
            else if (Layout == ELayout::PinkBedroomV2) Bedroom.Begin(Shell, Source->SavedDesign, true);
            else if (Layout == ELayout::KitchenV1 || Layout == ELayout::BathroomV1)
                KitchenBathroom.Begin(Shell, Layout == ELayout::KitchenV1);
            else Studio.Begin(Shell, Layout == ELayout::PinkBedroomV1);
            Phase = 2;
        }
        if (Phase == 2)
        {
            const auto Result = bLegacy ? Legacy.Advance(Work)
                : Layout == ELayout::PinkBedroomV2 ? Bedroom.Advance(Work)
                : Layout == ELayout::KitchenV1 || Layout == ELayout::BathroomV1 ? KitchenBathroom.Advance(Work)
                : Studio.Advance(Work);
            if (Result != EMappingStep::Complete) return Result;
            // V02 applies turns in its literal recipe; the other shells share
            // ApplyDesign, including the original extension metadata behavior.
            if (!bLegacy && Layout == ELayout::PinkBedroomV2) Phase = 4;
            else
            {
                const auto Admission = Work.Admit(128);
                if (Admission != EMappingStep::Complete) return Admission;
                Design.Begin(Output->GetValue(), Source->SavedDesign); Phase = 3;
            }
        }
        if (Phase == 3)
        {
            const auto Result = Design.Advance(Work);
            if (Result != EMappingStep::Complete) return Result;
            Phase = 4;
        }
        return EMappingStep::Complete;
    }
    TOptional<FRoomShellPlan> RoomShell(const FRoomPlan& Room)
    {
        TOptional<FRoomShellPlan> Out;
        FLegacyMappingWork Work;
        FRoomShellMapping Mapping;
        Mapping.Begin(Room, Out);
        Mapping.Advance(Work);
        return Out;
    }
}
