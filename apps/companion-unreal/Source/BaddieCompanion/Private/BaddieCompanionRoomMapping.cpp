#include "BaddieCompanionHomeMappingInternal.h"

namespace BaddieCompanion::HomeMapping
{
    namespace
    {
        bool ReadDesign(const FObject& Object, FRoomPlan& Room,
            const FString& Path, TArray<FHomeMappingIssue>& Issues)
        {
            if (!Object->HasField(TEXT("homeDesign"))) return true;
            const TSharedPtr<FJsonObject>* Design = nullptr;
            double Version = 0, Turns = 0, Extension = 0;
            if (!Object->TryGetObjectField(TEXT("homeDesign"), Design) || !Design || !Design->IsValid()
                || !Number(*Design, TEXT("version"), Version) || Version != 1
                || !Number(*Design, TEXT("turns"), Turns)
                || (Turns != 0 && Turns != 1 && Turns != 2 && Turns != 3)
                || !Number(*Design, TEXT("extension"), Extension)
                || (Extension != 0 && Extension != .6 && Extension != 1.2)
                || !Room.Environment.Effective.IsSet() || Room.Environment.Effective.GetValue() != EEnvironment::Studio
                || !Room.Layout.Saved.IsSet()
                || (Extension != 0 && (!Room.Layout.Effective.IsSet()
                    || (Room.Layout.Effective.GetValue() != ELayout::KitchenV1
                        && Room.Layout.Effective.GetValue() != ELayout::BathroomV1))))
            {
                Issue(Issues, EHomeIssue::InvalidDesign, Path + TEXT(".homeDesign"));
                return false;
            }
            UnknownFields(*Design, { TEXT("version"), TEXT("turns"), TEXT("extension") },
                Path + TEXT(".homeDesign"), Issues);
            Room.SavedDesign = FHomeDesignPlan{ 1, static_cast<int32>(Turns), Extension };
            return true;
        }

        bool ReadItem(const FObject& Object, FFurniturePlan& Item,
            const FString& Path, TArray<FHomeMappingIssue>& Issues)
        {
            UnknownFields(Object, { TEXT("id"), TEXT("kind"), TEXT("x"), TEXT("z"), TEXT("rotation"),
                TEXT("surface"), TEXT("y"), TEXT("lit") }, Path, Issues);
            if (!Text(Object, TEXT("id"), 80, Item.Id)
                || !Number(Object, TEXT("x"), Item.X) || FMath::Abs(Item.X) > 4
                || !Number(Object, TEXT("z"), Item.Z) || FMath::Abs(Item.Z) > 4
                || !Number(Object, TEXT("rotation"), Item.Rotation) || FMath::Abs(Item.Rotation) > 10000
                || !OptionalNumber(Object, TEXT("y"), Item.Y)
                || (Item.Y.IsSet() && (Item.Y.GetValue() < 0 || Item.Y.GetValue() > 4))
                || !OptionalBool(Object, TEXT("lit"), Item.Lit)) return false;
            Item.bEffectiveLit = Item.Lit.Get(true);
            return Choice<EItemKind>(Object, TEXT("kind"), Item.Kind, {
                    { TEXT("lamp"), EItemKind::Lamp }, { TEXT("stool"), EItemKind::Stool },
                    { TEXT("plant"), EItemKind::Plant }, { TEXT("botanical-painting"), EItemKind::BotanicalPainting },
                    { TEXT("abstract-painting"), EItemKind::AbstractPainting }, { TEXT("landscape-painting"), EItemKind::LandscapePainting },
                    { TEXT("wall-clock"), EItemKind::WallClock }, { TEXT("wall-mirror"), EItemKind::WallMirror },
                    { TEXT("standing-fan"), EItemKind::StandingFan }, { TEXT("tower-fan"), EItemKind::TowerFan },
                    { TEXT("desk-fan"), EItemKind::DeskFan }, { TEXT("floor-lamp"), EItemKind::FloorLamp },
                    { TEXT("desk-lamp"), EItemKind::DeskLamp }, { TEXT("armchair"), EItemKind::Armchair },
                    { TEXT("loveseat"), EItemKind::Loveseat }, { TEXT("ottoman"), EItemKind::Ottoman },
                    { TEXT("coffee-table"), EItemKind::CoffeeTable }, { TEXT("side-table"), EItemKind::SideTable },
                    { TEXT("bookcase"), EItemKind::Bookcase }, { TEXT("storage-basket"), EItemKind::StorageBasket },
                    { TEXT("book-stack"), EItemKind::BookStack }, { TEXT("candle"), EItemKind::Candle },
                    { TEXT("succulent"), EItemKind::Succulent }
                }, true, Path, Issues)
                && Choice<ESurface>(Object, TEXT("surface"), Item.Surface, {
                    { TEXT("floor"), ESurface::Floor }, { TEXT("console"), ESurface::Console },
                    { TEXT("shelf"), ESurface::Shelf }, { TEXT("wall-back"), ESurface::WallBack },
                    { TEXT("wall-left"), ESurface::WallLeft }, { TEXT("wall-right"), ESurface::WallRight },
                    { TEXT("wall-front"), ESurface::WallFront }
                }, false, Path, Issues, ESurface::Floor);
        }
    }

    bool ReadRoom(const FRoomSelection& Input, FRoomPlan& Out, const FString& Path,
        TArray<FHomeMappingIssue>& Issues)
    {
        Out = {};
        FObject Object;
        if (!ReadObject(Input.RoomJson, Object)) return false;
        UnknownFields(Object, { TEXT("id"), TEXT("name"), TEXT("environmentId"), TEXT("layoutId"),
            TEXT("homeDesign"), TEXT("palette"), TEXT("light"), TEXT("sharing"), TEXT("items"),
            TEXT("wallColor"), TEXT("floorColor"), TEXT("upholsteryColor"), TEXT("rugColor") }, Path, Issues);
        if (!Text(Object, TEXT("id"), 80, Out.Id) || !Text(Object, TEXT("name"), 32, Out.Name)
            || !Choice<EEnvironment>(Object, TEXT("environmentId"), Out.Environment, {
                { TEXT("studio"), EEnvironment::Studio }, { TEXT("terrace"), EEnvironment::Terrace }
            }, true, Path, Issues)
            || !Choice<ELayout>(Object, TEXT("layoutId"), Out.Layout, {
                { TEXT("pat-studio-v01"), ELayout::PatStudioV1 }, { TEXT("pink-bedroom-v01"), ELayout::PinkBedroomV1 },
                { TEXT("pink-bedroom-v02"), ELayout::PinkBedroomV2 }, { TEXT("modern-kitchen-v01"), ELayout::KitchenV1 },
                { TEXT("bathroom-v01"), ELayout::BathroomV1 }
            }, false, Path, Issues)
            || !Choice<EPalette>(Object, TEXT("palette"), Out.Palette, {
                { TEXT("rose"), EPalette::Rose }, { TEXT("sage"), EPalette::Sage }, { TEXT("midnight"), EPalette::Midnight }
            }, true, Path, Issues)
            || !Choice<ELight>(Object, TEXT("light"), Out.Light, {
                { TEXT("daylight"), ELight::Daylight }, { TEXT("golden"), ELight::Golden }, { TEXT("evening"), ELight::Evening }
            }, true, Path, Issues)
            || !Choice<ESharing>(Object, TEXT("sharing"), Out.Sharing, {
                { TEXT("private"), ESharing::Private }, { TEXT("host"), ESharing::Host }, { TEXT("collaborative"), ESharing::Collaborative }
            }, true, Path, Issues)
            || !OptionalColor(Object, TEXT("wallColor"), Out.WallColor)
            || !OptionalColor(Object, TEXT("floorColor"), Out.FloorColor)
            || !OptionalColor(Object, TEXT("upholsteryColor"), Out.UpholsteryColor)
            || !OptionalColor(Object, TEXT("rugColor"), Out.RugColor)) return false;
        if (!Same(Out.Id, Input.Id) || !Same(Out.Name, Input.Name)
            || !Same(Out.Environment.Saved.GetValue(), Input.EnvironmentId)
            || !Same(Out.Layout.Saved.Get(FString()), Input.LayoutId))
        {
            Issue(Issues, EHomeIssue::IdentityMismatch, Path);
            return false;
        }
        if (Out.Environment.Effective.IsSet() && Out.Environment.Effective.GetValue() == EEnvironment::Terrace
            && Out.Layout.Saved.IsSet()) return false;
        if (!ReadDesign(Object, Out, Path, Issues)) return false;
        const TArray<TSharedPtr<FJsonValue>>* Items = nullptr;
        if (!Object->TryGetArrayField(TEXT("items"), Items) || !Items || Items->Num() > 8) return false;
        for (int32 Index = 0; Index < Items->Num(); ++Index)
        {
            const FString ItemPath = FString::Printf(TEXT("%s.items[%d]"), *Path, Index);
            const auto& Value = (*Items)[Index];
            FFurniturePlan Item;
            if (!Value.IsValid() || Value->Type != EJson::Object
                || !ReadItem(Value->AsObject(), Item, ItemPath, Issues)) return false;
            if (Out.Items.ContainsByPredicate([&Item](const FFurniturePlan& Existing) { return Same(Existing.Id, Item.Id); }))
            {
                Issue(Issues, EHomeIssue::DuplicateItemId, ItemPath + TEXT(".id"));
                return false;
            }
            Out.Items.Add(MoveTemp(Item));
        }
        if (!Out.Layout.Saved.IsSet()) Issue(Issues, EHomeIssue::LegacyLayout, Path + TEXT(".layoutId"));
        if (Out.Layout.Effective.IsSet()) Out.Asset = RoomReference(Out.Layout.Effective.GetValue());
        if (Out.Layout.Effective.IsSet() && Out.Layout.Effective.GetValue() == ELayout::PinkBedroomV2
            && Out.Environment.Effective.IsSet() && Out.Environment.Effective.GetValue() == EEnvironment::Studio)
        {
            Out.BedroomShell = BedroomShell(Out.SavedDesign);
            Out.bSourceMapped = true;
            MapFurniture(Out, Path, Issues);
            Issue(Issues, EHomeIssue::PlacementValidationUnavailable, Path + TEXT(".items"));
            Issue(Issues, EHomeIssue::NavigationUnported, Path + TEXT(".spawn"));
        }
        else Issue(Issues, EHomeIssue::LayoutNotMapped, Path + TEXT(".layoutId"));
        if (Out.Asset.IsSet()) Issue(Issues, EHomeIssue::AssetImportUnavailable, Path + TEXT(".asset"));
        Issue(Issues, EHomeIssue::MaterialRecipeUnported, Path + TEXT(".materials"));
        return true;
    }
}
