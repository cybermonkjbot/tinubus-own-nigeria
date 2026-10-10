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

        double FloorHeight(const FFurniturePlan& Item, const FRoomShellPlan& Shell, double Base, double Radius)
        {
            double Height = Base;
            bool bDepression = false;
            for (const auto& Patch : Shell.FloorPatches)
            {
                const auto& Centre = Patch.BottomCentreMetres;
                const auto& Size = Patch.DimensionsMetres;
                const double DX = FMath::Max(0.0, FMath::Abs(Item.X - Centre.X) - Size.X / 2);
                const double DZ = FMath::Max(0.0, FMath::Abs(Item.Z - Centre.Z) - Size.Z / 2);
                if (DX * DX + DZ * DZ > Radius * Radius) continue;
                if (Centre.Y < Base) { Height = FMath::Min(Height, Centre.Y); bDepression = true; }
                else if (!bDepression) Height = FMath::Max(Height, Centre.Y);
            }
            return Height;
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

        bool MapWall(FFurniturePlan& Item, const FItemDimensions& Body, const FWallPlan& Wall)
        {
            if (Body.Placement != EPlacement::Wall || !Item.Y.IsSet()) return false;
            const double C = FMath::Cos(Wall.Anchor.YawRadians), S = FMath::Sin(Wall.Anchor.YawRadians);
            const double U = (Item.X - Wall.Anchor.Metres.X) * C - (Item.Z - Wall.Anchor.Metres.Z) * S;
            const double DX = Item.X - (Wall.Anchor.Metres.X + C * U);
            const double DZ = Item.Z - (Wall.Anchor.Metres.Z - S * U);
            const double Angle = Item.Rotation - Wall.Anchor.YawRadians;
            const double Width = Body.WallWidthMetres.GetValue(), Depth = Body.WallDepthMetres.GetValue();
            const double Y = Item.Y.GetValue();
            if (DX * DX + DZ * DZ > .001 * .001
                || FMath::Abs(FMath::Atan2(FMath::Sin(Angle), FMath::Cos(Angle))) > .001
                || FMath::Abs(U) + Width / 2 > Wall.Width / 2 || Y < Wall.MinY || Y + Body.HeightMetres > Wall.MaxY)
                return false;
            for (const auto& Opening : Wall.Openings)
                if (FMath::Abs(U - Opening.U) < (Width + Opening.Width) / 2 + .02
                    && Y < Opening.BottomY + Opening.Height && Y + Body.HeightMetres > Opening.BottomY) return false;
            Item.Pivot = EItemPivot::BottomWallAnchor;
            Item.WallLocalU = U;
            Item.PlacementPose = Pose(FVector(Item.X, Y, Item.Z), Item.Rotation);
            Item.Volume = Box(FVector(Item.X + S * Depth / 2, Y, Item.Z + C * Depth / 2),
                FVector(Width * FMath::Abs(C) + Depth * FMath::Abs(S), Body.HeightMetres,
                    Width * FMath::Abs(S) + Depth * FMath::Abs(C)));
            return true;
        }
    }

    void MapFurniture(FRoomPlan& Room, const FString& Path, TArray<FHomeMappingIssue>& Issues)
    {
        if (!Room.Shell.IsSet()) return;
        const auto& Shell = Room.Shell.GetValue();
        for (int32 Index = 0; Index < Room.Items.Num(); ++Index)
        {
            auto& Item = Room.Items[Index];
            const FString ItemPath = FString::Printf(TEXT("%s.items[%d]"), *Path, Index);
            if (!Item.Kind.Effective.IsSet()) continue;
            const auto Kind = Item.Kind.Effective.GetValue();
            const FCatalogItem* Definition = nullptr;
            for (const auto& Entry : Catalog) if (Entry.Kind == Kind) { Definition = &Entry; break; }
            if (!Definition) { Issue(Issues, EHomeIssue::MissingReference, ItemPath + TEXT(".kind")); continue; }
            const auto Body = Dimensions(*Definition, Shell);
            Item.Dimensions = Body;
            Item.SourceGeometryRecipe = TEXT("studioGeometry.makeRoomObject:") + Item.Kind.Saved.GetValue();
            Issue(Issues, EHomeIssue::PropRecipeUnported, ItemPath + TEXT(".geometry"));
            if (!Item.Surface.Effective.IsSet()) continue;
            if (!Item.Surface.Saved.IsSet() && Kind != EItemKind::Lamp && Kind != EItemKind::Stool && Kind != EItemKind::Plant)
            {
                Issue(Issues, EHomeIssue::MissingSurface, ItemPath + TEXT(".surface"));
                continue;
            }
            const auto SurfaceId = Item.Surface.Effective.GetValue();
            const auto* Wall = Shell.Walls.FindByPredicate([SurfaceId](const FWallPlan& Value) { return Value.Id == SurfaceId; });
            const auto* Support = Shell.Supports.FindByPredicate([SurfaceId](const FSupportPlan& Value) { return Value.Id == SurfaceId; });
            if (!Wall && !Support) { Issue(Issues, EHomeIssue::MissingSurface, ItemPath + TEXT(".surface")); continue; }
            if (Wall)
            {
                if (!MapWall(Item, Body, *Wall)) Issue(Issues, EHomeIssue::InvalidPlacement, ItemPath);
                continue;
            }
            if (Body.Placement == EPlacement::Wall || Item.Y.IsSet() || !Support->AllowedKinds.Contains(Kind)
                || !FitsSupport(Item, Body, *Support, Shell))
            {
                Issue(Issues, EHomeIssue::InvalidPlacement, ItemPath);
                continue;
            }
            const double Base = Support->Top.BottomCentreMetres.Y;
            const double Y = SurfaceId == ESurface::Floor ? FloorHeight(Item, Shell, Base, Body.SupportRadiusMetres) : Base;
            // Saved furniture already follows the saved shell's coordinates. Never turn or snap it again.
            Item.PlacementPose = Pose(FVector(Item.X, Y, Item.Z), Item.Rotation);
            Item.Volume = Box(FVector(Item.X, Y, Item.Z), FVector(Body.RadiusMetres * 2, Body.HeightMetres, Body.RadiusMetres * 2));
        }
    }
}
