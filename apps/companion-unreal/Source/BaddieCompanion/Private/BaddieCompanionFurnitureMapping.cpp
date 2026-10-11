#include "BaddieCompanionHomeMappingInternal.h"

namespace BaddieCompanion::HomeMapping
{
    namespace
    {
        struct FCatalogItem
        {
            EItemKind Kind;
            double Radius, Height, Mass;
            EPlacement Placement;
            double WallWidth = 0, WallDepth = 0;
        };
        // companionRoomItems catalog units. These describe saved props, not native collision shapes.
        const FCatalogItem Catalog[] = {
            { EItemKind::Lamp, .29, .93, .8, EPlacement::Tabletop },
            { EItemKind::Stool, .37, .6, 2.1, EPlacement::Floor },
            { EItemKind::Plant, .28, 1.18, 1.3, EPlacement::Tabletop },
            { EItemKind::BotanicalPainting, .65, 1.7, 1, EPlacement::Wall, 1.3, .12 },
            { EItemKind::AbstractPainting, .9, 1.25, 1, EPlacement::Wall, 1.8, .12 },
            { EItemKind::LandscapePainting, .95, 1.15, 1, EPlacement::Wall, 1.9, .12 },
            { EItemKind::WallClock, .48, .96, .7, EPlacement::Wall, .96, .14 },
            { EItemKind::WallMirror, .52, 1.55, 1.2, EPlacement::Wall, 1.04, .08 },
            { EItemKind::StandingFan, .55, 2.67, 2, EPlacement::Floor },
            { EItemKind::TowerFan, .34, 2.3, 1.8, EPlacement::Floor },
            { EItemKind::DeskFan, .37, .92, .8, EPlacement::Tabletop },
            { EItemKind::FloorLamp, .52, 3.05, 2, EPlacement::Floor },
            { EItemKind::DeskLamp, .32, .95, .8, EPlacement::Tabletop },
            { EItemKind::Armchair, 1.13, 1.8, 3, EPlacement::Floor },
            { EItemKind::Loveseat, 1.6, 1.7, 4, EPlacement::Floor },
            { EItemKind::Ottoman, .68, .78, 2, EPlacement::Floor },
            { EItemKind::CoffeeTable, 1.1, .82, 3, EPlacement::Floor },
            { EItemKind::SideTable, .57, 1.05, 2, EPlacement::Floor },
            { EItemKind::Bookcase, .88, 2.7, 4, EPlacement::Floor },
            { EItemKind::StorageBasket, .5, .85, 1, EPlacement::Floor },
            { EItemKind::BookStack, .34, .25, .6, EPlacement::Tabletop },
            { EItemKind::Candle, .2, .47, .5, EPlacement::Tabletop },
            { EItemKind::Succulent, .28, .48, .6, EPlacement::Tabletop }
        };

        FItemDimensions Dimensions(const FCatalogItem& Definition, const FRoomShellPlan& Shell)
        {
            FItemDimensions Out;
            Out.CatalogRadius = Definition.Radius; Out.CatalogHeight = Definition.Height;
            Out.Mass = Definition.Mass; Out.Placement = Definition.Placement;
            double Radius = Definition.Radius, Support = Radius, Height = Definition.Height;
            if (!Shell.bLegacyPlacement)
            {
                switch (Definition.Kind)
                {
                case EItemKind::Lamp: Radius = .35; Support = .27; Height = .93; break;
                case EItemKind::Plant: Radius = .44; Support = .25; Height = 1.22; break;
                case EItemKind::Stool: Radius = .37; Support = .33; Height = .6; break;
                default: break;
                }
            }
            Out.RadiusMetres = Radius * Shell.ObjectScale; Out.SupportRadiusMetres = Support * Shell.ObjectScale;
            Out.HeightMetres = Height * Shell.ObjectScale;
            if (Definition.Placement == EPlacement::Wall)
            {
                Out.WallWidthMetres = Definition.WallWidth * Shell.ObjectScale;
                Out.WallDepthMetres = Definition.WallDepth * Shell.ObjectScale;
            }
            return Out;
        }

        bool FitsSupport(const FFurniturePlan& Item, const FItemDimensions& Body,
            const FSupportPlan& Support, const FRoomShellPlan& Shell)
        {
            const auto& Centre = Support.Top.BottomCentreMetres;
            const auto& Size = Support.Top.DimensionsMetres;
            const double Radius = Support.Id == ESurface::Floor ? Body.RadiusMetres : Body.SupportRadiusMetres;
            // Older unassigned props retain the original ground-plane placement envelope.
            if (Shell.bLegacyPlacement && !Item.Surface.Saved.IsSet()
                && FMath::Abs(Item.X) <= 1.85 && Item.Z >= -1.15 && Item.Z <= 1.6) return true;
            if (FMath::Abs(Item.X - Centre.X) > Size.X / 2 - Radius
                || FMath::Abs(Item.Z - Centre.Z) > Size.Z / 2 - Radius) return false;
            if (Shell.bLegacyPlacement) return true;
            if (Item.X < Shell.MinX + Body.RadiusMetres || Item.X > Shell.MaxX - Body.RadiusMetres
                || Item.Z < Shell.MinZ + Body.RadiusMetres || Item.Z > Shell.MaxZ - Body.RadiusMetres) return false;
            if (!Support.Clearance.IsSet()) return true;
            const auto& Space = Support.Clearance.GetValue();
            return FMath::Abs(Item.X - Space.BottomCentreMetres.X) <= Space.DimensionsMetres.X / 2 - Body.RadiusMetres
                && FMath::Abs(Item.Z - Space.BottomCentreMetres.Z) <= Space.DimensionsMetres.Z / 2 - Body.RadiusMetres
                && Body.HeightMetres <= Space.DimensionsMetres.Y;
        }

        bool WallFits(const FFurniturePlan& Item, const FItemDimensions& Body, const FWallPlan& Wall, double& U)
        {
            if (Body.Placement != EPlacement::Wall || !Item.Y.IsSet()) return false;
            const double C = FMath::Cos(Wall.Anchor.YawRadians), S = FMath::Sin(Wall.Anchor.YawRadians);
            U = (Item.X - Wall.Anchor.Metres.X) * C - (Item.Z - Wall.Anchor.Metres.Z) * S;
            const double DX = Item.X - (Wall.Anchor.Metres.X + C * U);
            const double DZ = Item.Z - (Wall.Anchor.Metres.Z - S * U);
            const double Angle = Item.Rotation - Wall.Anchor.YawRadians;
            const double Width = Body.WallWidthMetres.GetValue();
            const double Y = Item.Y.GetValue();
            if (DX * DX + DZ * DZ > .001 * .001
                || FMath::Abs(FMath::Atan2(FMath::Sin(Angle), FMath::Cos(Angle))) > .001
                || FMath::Abs(U) + Width / 2 > Wall.Width / 2 || Y < Wall.MinY || Y + Body.HeightMetres > Wall.MaxY)
                return false;
            return true;
        }
        void PlaceWall(FFurniturePlan& Item, const FItemDimensions& Body, const FWallPlan& Wall, double U)
        {
            const double C = FMath::Cos(Wall.Anchor.YawRadians), S = FMath::Sin(Wall.Anchor.YawRadians);
            const double Width = Body.WallWidthMetres.GetValue(), Depth = Body.WallDepthMetres.GetValue();
            const double Y = Item.Y.GetValue();
            Item.Pivot = EItemPivot::BottomWallAnchor;
            Item.WallLocalU = U;
            Item.PlacementPose = Pose(FVector(Item.X, Y, Item.Z), Item.Rotation);
            Item.Volume = Box(FVector(Item.X + S * Depth / 2, Y, Item.Z + C * Depth / 2),
                FVector(Width * FMath::Abs(C) + Depth * FMath::Abs(S), Body.HeightMetres,
                    Width * FMath::Abs(S) + Depth * FMath::Abs(C)));
        }
    }

    void MapFurniture(FRoomPlan& Room, const FString& Path, TArray<FHomeMappingIssue>& Issues)
    {
        FLegacyMappingWork Work;
        FFurnitureMapping Mapping;
        Mapping.Begin(Room, Path, Issues);
        Mapping.Advance(Work);
        Mapping.Retire(Work);
    }

    EMappingStep FFurnitureMapping::Retire(FMappingWork& Work)
    {
        const auto Result = Work.ReleaseText(ItemPath);
        if (Result != EMappingStep::Complete) return Result;
        Output = nullptr; SourcePath = nullptr; OutputIssues = nullptr;
        Wall = nullptr; Support = nullptr;
        return EMappingStep::Complete;
    }
    void FFurnitureMapping::Begin(FRoomPlan& Room, const FString& Path, TArray<FHomeMappingIssue>& Issues)
    {
        Output = &Room; SourcePath = &Path; OutputIssues = &Issues;
        Phase = Index = Search = 0; Wall = nullptr; Support = nullptr;
    }

    EMappingStep FFurnitureMapping::Advance(FMappingWork& Work)
    {
        if (!Output || !SourcePath || !OutputIssues) return EMappingStep::Invalid;
        if (!Output->Shell.IsSet()) return EMappingStep::Complete;
        const auto& Shell = Output->Shell.GetValue();
        while (Index < static_cast<uint32>(Output->Items.Num()))
        {
            auto& Item = Output->Items[Index];
            if (!Item.Kind.Effective.IsSet())
            {
                const auto Admission = Work.Admit(32);
                if (Admission != EMappingStep::Complete) return Admission;
                ++Index; Phase = Search = 0; continue;
            }
            const auto Kind = Item.Kind.Effective.GetValue();
            if (Phase == 0)
            {
                const auto Admission = Work.Admit(64 + 4 * static_cast<uint64>(SourcePath->Len()),
                    (SourcePath->Len() + 24) * sizeof(TCHAR));
                if (Admission != EMappingStep::Complete) return Admission;
                ItemPath = FString::Printf(TEXT("%s.items[%d]"), **SourcePath, Index);
                Wall = nullptr; Support = nullptr; bAllowedKind = bDepression = false;
                Search = 0; Phase = 1;
            }
            if (Phase == 1)
            {
                while (Search < UE_ARRAY_COUNT(Catalog))
                {
                    const auto Admission = Work.Admit(64);
                    if (Admission != EMappingStep::Complete) return Admission;
                    if (Catalog[Search].Kind == Kind) { Body = Dimensions(Catalog[Search], Shell); break; }
                    ++Search;
                }
                Phase = Search == UE_ARRAY_COUNT(Catalog) ? 20 : 2;
                Search = 0;
            }
            if (Phase == 2)
            {
                const auto Admission = Work.Admit(128 + 2 * static_cast<uint64>(Item.Kind.Saved.GetValue().Len()),
                    (Item.Kind.Saved.GetValue().Len() + 32) * sizeof(TCHAR));
                if (Admission != EMappingStep::Complete) return Admission;
                Item.Dimensions = Body;
                Item.SourceGeometryRecipe = TEXT("studioGeometry.makeRoomObject:") + Item.Kind.Saved.GetValue();
                Phase = 21;
            }
            if (Phase >= 20 && Phase <= 23)
            {
                const auto Admission = Work.Admit(128 + 4 * static_cast<uint64>(ItemPath.Len()),
                    (OutputIssues->Num() + 1) * sizeof(FHomeMappingIssue) + (ItemPath.Len() + 16) * sizeof(TCHAR));
                if (Admission != EMappingStep::Complete) return Admission;
                if (Phase == 20) Issue(*OutputIssues, EHomeIssue::MissingReference, ItemPath + TEXT(".kind"));
                if (Phase == 21) Issue(*OutputIssues, EHomeIssue::PropRecipeUnported, ItemPath + TEXT(".geometry"));
                if (Phase == 22) Issue(*OutputIssues, EHomeIssue::MissingSurface, ItemPath + TEXT(".surface"));
                if (Phase == 23) Issue(*OutputIssues, EHomeIssue::InvalidPlacement, ItemPath);
                Phase = Phase == 21 ? 3 : 24;
            }
            if (Phase == 3)
            {
                const auto Admission = Work.Admit(32);
                if (Admission != EMappingStep::Complete) return Admission;
                if (!Item.Surface.Effective.IsSet()) Phase = 24;
                else if (!Item.Surface.Saved.IsSet() && Kind != EItemKind::Lamp && Kind != EItemKind::Stool && Kind != EItemKind::Plant) Phase = 22;
                else Phase = 4;
            }
            if (Phase == 4)
            {
                while (Search < static_cast<uint32>(Shell.Walls.Num()))
                {
                    const auto Admission = Work.Admit(32);
                    if (Admission != EMappingStep::Complete) return Admission;
                    const auto& Value = Shell.Walls[Search++];
                    if (Value.Id == Item.Surface.Effective.GetValue()) { Wall = &Value; break; }
                }
                Phase = 5; Search = 0;
            }
            if (Phase == 5)
            {
                while (Search < static_cast<uint32>(Shell.Supports.Num()))
                {
                    const auto Admission = Work.Admit(32);
                    if (Admission != EMappingStep::Complete) return Admission;
                    const auto& Value = Shell.Supports[Search++];
                    if (Value.Id == Item.Surface.Effective.GetValue()) { Support = &Value; break; }
                }
                Phase = Wall ? 6 : Support ? 9 : 22; Search = 0;
            }
            if (Phase == 6)
            {
                const auto Admission = Work.Admit(256);
                if (Admission != EMappingStep::Complete) return Admission;
                Phase = WallFits(Item, Body, *Wall, WallU) ? 7 : 23;
            }
            if (Phase == 7)
            {
                while (Search < static_cast<uint32>(Wall->Openings.Num()))
                {
                    const auto Admission = Work.Admit(64);
                    if (Admission != EMappingStep::Complete) return Admission;
                    const auto& Opening = Wall->Openings[Search++];
                    if (FMath::Abs(WallU - Opening.U) < (Body.WallWidthMetres.GetValue() + Opening.Width) / 2 + .02
                        && Item.Y.GetValue() < Opening.BottomY + Opening.Height
                        && Item.Y.GetValue() + Body.HeightMetres > Opening.BottomY) { Phase = 23; break; }
                }
                if (Phase == 7) Phase = 8;
            }
            if (Phase == 8)
            {
                const auto Admission = Work.Admit(256);
                if (Admission != EMappingStep::Complete) return Admission;
                PlaceWall(Item, Body, *Wall, WallU); Phase = 24;
            }
            if (Phase == 9)
            {
                while (Search < static_cast<uint32>(Support->AllowedKinds.Num()))
                {
                    const auto Admission = Work.Admit(16);
                    if (Admission != EMappingStep::Complete) return Admission;
                    if (Support->AllowedKinds[Search++] == Kind) { bAllowedKind = true; break; }
                }
                Phase = 10; Search = 0;
            }
            if (Phase == 10)
            {
                const auto Admission = Work.Admit(256);
                if (Admission != EMappingStep::Complete) return Admission;
                if (Body.Placement == EPlacement::Wall || Item.Y.IsSet() || !bAllowedKind || !FitsSupport(Item, Body, *Support, Shell)) Phase = 23;
                else { FloorY = FloorBase = Support->Top.BottomCentreMetres.Y; Phase = 11; }
            }
            if (Phase == 11)
            {
                if (Item.Surface.Effective.GetValue() == ESurface::Floor)
                    while (Search < static_cast<uint32>(Shell.FloorPatches.Num()))
                    {
                        const auto Admission = Work.Admit(128);
                        if (Admission != EMappingStep::Complete) return Admission;
                        const auto& Patch = Shell.FloorPatches[Search++];
                        const auto& Centre = Patch.BottomCentreMetres; const auto& Size = Patch.DimensionsMetres;
                        const double DX = FMath::Max(0.0, FMath::Abs(Item.X - Centre.X) - Size.X / 2);
                        const double DZ = FMath::Max(0.0, FMath::Abs(Item.Z - Centre.Z) - Size.Z / 2);
                        if (DX * DX + DZ * DZ > Body.SupportRadiusMetres * Body.SupportRadiusMetres) continue;
                        if (Centre.Y < FloorBase) { FloorY = FMath::Min(FloorY, Centre.Y); bDepression = true; }
                        else if (!bDepression) FloorY = FMath::Max(FloorY, Centre.Y);
                    }
                Phase = 12;
            }
            if (Phase == 12)
            {
                const auto Admission = Work.Admit(256);
                if (Admission != EMappingStep::Complete) return Admission;
                Item.PlacementPose = Pose(FVector(Item.X, FloorY, Item.Z), Item.Rotation);
                Item.Volume = Box(FVector(Item.X, FloorY, Item.Z), FVector(Body.RadiusMetres * 2, Body.HeightMetres, Body.RadiusMetres * 2));
                Phase = 24;
            }
            if (Phase == 24) { ++Index; Phase = Search = 0; }
        }
        return EMappingStep::Complete;
    }
}
