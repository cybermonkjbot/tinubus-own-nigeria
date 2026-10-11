#include "BaddieCompanionHomeMappingInternal.h"

namespace BaddieCompanion::HomeMapping
{
    void FItemMapping::Begin(const FMappingObject& Object, FFurniturePlan& Item,
        const FString& Path, TArray<FHomeMappingIssue>& Issues)
    {
        Source = &Object; Output = &Item; SourcePath = &Path; OutputIssues = &Issues;
        Phase = 0; Choice.Begin();
    }
    EMappingStep FItemMapping::Advance(FMappingWork& Work)
    {
        if (!Source || !Output || !SourcePath || !OutputIssues) return EMappingStep::Invalid;
        auto& Item = *Output;
        while (Phase < 10)
        {
            EMappingStep Result = EMappingStep::Complete;
            if (Phase < 8)
            {
                const auto Admission = Work.Admit(1024, Phase == 1 ? 81 * sizeof(TCHAR) : 0);
                if (Admission != EMappingStep::Complete) return Admission;
            }
            switch (Phase)
            {
            case 0: UnknownFields(*Source, ItemFields(), *SourcePath, *OutputIssues); break;
            case 1: if (!Text(*Source, TEXT("id"), 80, Item.Id)) return EMappingStep::Invalid; break;
            case 2: if (!Number(*Source, TEXT("x"), Item.X) || FMath::Abs(Item.X) > 4) return EMappingStep::Invalid; break;
            case 3: if (!Number(*Source, TEXT("z"), Item.Z) || FMath::Abs(Item.Z) > 4) return EMappingStep::Invalid; break;
            case 4: if (!Number(*Source, TEXT("rotation"), Item.Rotation) || FMath::Abs(Item.Rotation) > 10000) return EMappingStep::Invalid; break;
            case 5:
                if (!OptionalNumber(*Source, TEXT("y"), Item.Y)
                    || (Item.Y.IsSet() && (Item.Y.GetValue() < 0 || Item.Y.GetValue() > 4))) return EMappingStep::Invalid;
                break;
            case 6: if (!OptionalBool(*Source, TEXT("lit"), Item.Lit)) return EMappingStep::Invalid; break;
            case 7: Item.bEffectiveLit = Item.Lit.Get(true); break;
            case 8:
                Result = Choice.Advance<EItemKind>(Work, *Source, TEXT("kind"), Item.Kind, {
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
                }, true, *SourcePath, *OutputIssues);
                break;
            case 9:
                Result = Choice.Advance<ESurface>(Work, *Source, TEXT("surface"), Item.Surface, {
                    { TEXT("floor"), ESurface::Floor }, { TEXT("console"), ESurface::Console },
                    { TEXT("shelf"), ESurface::Shelf }, { TEXT("wall-back"), ESurface::WallBack },
                    { TEXT("wall-left"), ESurface::WallLeft }, { TEXT("wall-right"), ESurface::WallRight },
                    { TEXT("wall-front"), ESurface::WallFront }
                }, false, *SourcePath, *OutputIssues, ESurface::Floor);
                break;
            }
            if (Result != EMappingStep::Complete) return Result;
            ++Phase; Choice.Begin();
        }
        return EMappingStep::Complete;
    }
    void FRoomMapping::Begin(const FRoomSelection& InIdentity, const FRoomMappingInput& Input,
        FRoomPlan& Room, const FString& Path, TArray<FHomeMappingIssue>& Issues)
    {
        Identity = &InIdentity; Source = &Input; Output = &Room; SourcePath = &Path; OutputIssues = &Issues;
        Phase = Index = CompareIndex = ItemPhase = RetireField = 0;
        Version = Turns = Extension = 0; Choice.Begin();
    }
    EMappingStep FRoomMapping::Advance(FMappingWork& Work)
    {
        if (!Identity || !Source || !Output || !SourcePath || !OutputIssues) return EMappingStep::Invalid;
        auto& Out = *Output;
        auto& Issues = *OutputIssues;
        const auto& Object = Source->Room;
        const auto& Path = *SourcePath;
        while (Phase < 22)
        {
            EMappingStep Result = EMappingStep::Complete;
            switch (Phase)
            {
            case 0:
                Result = Work.Admit(1024);
                if (Result != EMappingStep::Complete) return Result;
                UnknownFields(Object, RoomFields(), Path, Issues);
                break;
            case 1: case 2:
                Result = Work.Admit(1024, (Phase == 1 ? 81 : 33) * sizeof(TCHAR));
                if (Result != EMappingStep::Complete) return Result;
                if (!Text(Object, Phase == 1 ? TEXT("id") : TEXT("name"), Phase == 1 ? 80 : 32,
                    Phase == 1 ? Out.Id : Out.Name)) return EMappingStep::Invalid;
                break;
            case 3:
                Result = Choice.Advance<EEnvironment>(Work, Object, TEXT("environmentId"), Out.Environment, {
                    { TEXT("studio"), EEnvironment::Studio }, { TEXT("terrace"), EEnvironment::Terrace }
                }, true, Path, Issues);
                break;
            case 4:
                Result = Choice.Advance<ELayout>(Work, Object, TEXT("layoutId"), Out.Layout, {
                    { TEXT("pat-studio-v01"), ELayout::PatStudioV1 }, { TEXT("pink-bedroom-v01"), ELayout::PinkBedroomV1 },
                    { TEXT("pink-bedroom-v02"), ELayout::PinkBedroomV2 }, { TEXT("modern-kitchen-v01"), ELayout::KitchenV1 },
                    { TEXT("bathroom-v01"), ELayout::BathroomV1 }
                }, false, Path, Issues);
                break;
            case 5:
                Result = Choice.Advance<EPalette>(Work, Object, TEXT("palette"), Out.Palette, {
                    { TEXT("rose"), EPalette::Rose }, { TEXT("sage"), EPalette::Sage }, { TEXT("midnight"), EPalette::Midnight }
                }, true, Path, Issues);
                break;
            case 6:
                Result = Choice.Advance<ELight>(Work, Object, TEXT("light"), Out.Light, {
                    { TEXT("daylight"), ELight::Daylight }, { TEXT("golden"), ELight::Golden }, { TEXT("evening"), ELight::Evening }
                }, true, Path, Issues);
                break;
            case 7:
                Result = Choice.Advance<ESharing>(Work, Object, TEXT("sharing"), Out.Sharing, {
                    { TEXT("private"), ESharing::Private }, { TEXT("host"), ESharing::Host }, { TEXT("collaborative"), ESharing::Collaborative }
                }, true, Path, Issues);
                break;
            case 8:
            {
                static const TCHAR* Names[] = { TEXT("wallColor"), TEXT("floorColor"), TEXT("upholsteryColor"), TEXT("rugColor") };
                TOptional<FString>* Colors[] = { &Out.WallColor, &Out.FloorColor, &Out.UpholsteryColor, &Out.RugColor };
                if (Index < UE_ARRAY_COUNT(Names))
                {
                    Result = Work.Admit(1024, 16 * sizeof(TCHAR));
                    if (Result != EMappingStep::Complete) return Result;
                    if (!OptionalColor(Object, Names[Index], *Colors[Index])) return EMappingStep::Invalid;
                    ++Index; continue;
                }
                break;
            }
            case 9:
                Result = Work.Admit(1024, (Issues.Num() + 1) * sizeof(FHomeMappingIssue) + (Path.Len() + 1) * sizeof(TCHAR));
                if (Result != EMappingStep::Complete) return Result;
                if (!Same(Out.Id, Identity->Id) || !Same(Out.Name, Identity->Name)
                    || !Same(Out.Environment.Saved.GetValue(), Identity->EnvironmentId)
                    || (Out.Layout.Saved.IsSet() ? !Same(Out.Layout.Saved.GetValue(), Identity->LayoutId) : !Identity->LayoutId.IsEmpty()))
                { Issue(Issues, EHomeIssue::IdentityMismatch, Path); return EMappingStep::Invalid; }
                if (Out.Environment.Effective.IsSet() && Out.Environment.Effective.GetValue() == EEnvironment::Terrace
                    && Out.Layout.Saved.IsSet()) return EMappingStep::Invalid;
                break;
            case 10:
                if (Source->bDesignPresent)
                {
                    Result = Work.Admit(1024, (Issues.Num() + 1) * sizeof(FHomeMappingIssue) + (Path.Len() + 12) * sizeof(TCHAR));
                    if (Result != EMappingStep::Complete) return Result;
                    bool bValid = Source->bDesignObject;
                    if (Index == 0) bValid = bValid && Number(Source->Design, TEXT("version"), Version) && Version == 1;
                    if (Index == 1) bValid = bValid && Number(Source->Design, TEXT("turns"), Turns)
                        && (Turns == 0 || Turns == 1 || Turns == 2 || Turns == 3);
                    if (Index == 2) bValid = bValid && Number(Source->Design, TEXT("extension"), Extension)
                        && (Extension == 0 || Extension == .6 || Extension == 1.2);
                    if (Index == 3) bValid = bValid && Out.Environment.Effective.IsSet()
                        && Out.Environment.Effective.GetValue() == EEnvironment::Studio && Out.Layout.Saved.IsSet()
                        && (Extension == 0 || (Out.Layout.Effective.IsSet()
                            && (Out.Layout.Effective.GetValue() == ELayout::KitchenV1 || Out.Layout.Effective.GetValue() == ELayout::BathroomV1)));
                    if (!bValid)
                    { Issue(Issues, EHomeIssue::InvalidDesign, Path + TEXT(".homeDesign")); return EMappingStep::Invalid; }
                    if (Index < 3) { ++Index; continue; }
                    UnknownFields(Source->Design, DesignFields(), Path + TEXT(".homeDesign"), Issues);
                    Out.SavedDesign = FHomeDesignPlan{ 1, static_cast<int32>(Turns), Extension };
                }
                break;
            case 11:
                if (!Source->bItemsArray || Source->ItemCount > 8) return EMappingStep::Invalid;
                if (Index < Source->ItemCount)
                {
                    if (ItemPhase == 0)
                    {
                        Result = Work.Admit(512, (Path.Len() + 24) * sizeof(TCHAR));
                        if (Result != EMappingStep::Complete) return Result;
                        ItemPath = FString::Printf(TEXT("%s.items[%d]"), *Path, static_cast<int32>(Index));
                        if (!Source->Items[Index].Json && !Source->Items[Index].Known) return EMappingStep::Invalid;
                        Item.Begin(Source->Items[Index], ItemOutput, ItemPath, Issues); ItemPhase = 1;
                    }
                    if (ItemPhase == 1)
                    {
                        Result = Item.Advance(Work);
                        if (Result != EMappingStep::Complete) return Result;
                        ItemPhase = 2;
                    }
                    while (CompareIndex < static_cast<uint32>(Out.Items.Num()))
                    {
                        Result = Work.Admit(256, (Issues.Num() + 1) * sizeof(FHomeMappingIssue) + (ItemPath.Len() + 4) * sizeof(TCHAR));
                        if (Result != EMappingStep::Complete) return Result;
                        if (Same(Out.Items[CompareIndex].Id, ItemOutput.Id))
                        { Issue(Issues, EHomeIssue::DuplicateItemId, ItemPath + TEXT(".id")); return EMappingStep::Invalid; }
                        ++CompareIndex;
                    }
                    Result = Work.Admit(128, (Out.Items.Num() + 1) * sizeof(FFurniturePlan));
                    if (Result != EMappingStep::Complete) return Result;
                    Out.Items.Add(MoveTemp(ItemOutput)); ItemOutput = {};
                    ++Index; CompareIndex = ItemPhase = 0; continue;
                }
                break;
            case 12:
                if (!Out.Layout.Saved.IsSet())
                {
                    Result = Work.Admit(512, (Issues.Num() + 1) * sizeof(FHomeMappingIssue) + (Path.Len() + 10) * sizeof(TCHAR));
                    if (Result != EMappingStep::Complete) return Result;
                    Issue(Issues, EHomeIssue::LegacyLayout, Path + TEXT(".layoutId"));
                }
                break;
            case 13:
                Result = Work.Admit(1024, sizeof(FCanonicalAssetReference) + 512 * sizeof(TCHAR));
                if (Result != EMappingStep::Complete) return Result;
                if (Out.Layout.Effective.IsSet()) Out.Asset = RoomReference(Out.Layout.Effective.GetValue());
                if (!Out.Layout.Saved.IsSet() && Out.Environment.Effective.IsSet()
                    && Out.Environment.Effective.GetValue() == EEnvironment::Terrace) Out.Asset = TerraceReference();
                break;
            case 14:
                if (!Out.Layout.Saved.IsSet() && Out.Environment.Effective.IsSet()
                    && Out.Environment.Effective.GetValue() == EEnvironment::Studio)
                {
                    Result = Work.Admit(512, (Issues.Num() + 1) * sizeof(FHomeMappingIssue) + (Path.Len() + 48) * sizeof(TCHAR));
                    if (Result != EMappingStep::Complete) return Result;
                    Out.SourceGeometryRecipe = TEXT("studioGeometry.makeStudio");
                    Issue(Issues, EHomeIssue::ShellRecipeUnported, Path + TEXT(".geometry"));
                }
                break;
            case 15:
                if (Index == 0) { Shell.Begin(Out, Out.Shell); Index = 1; }
                Result = Shell.Advance(Work);
                break;
            case 16:
                if (Out.Shell.IsSet())
                {
                    if (Index == 0) { Out.bSourceMapped = true; Furniture.Begin(Out, Path, Issues); Index = 1; }
                    Result = Furniture.Advance(Work);
                }
                break;
            default:
            {
                bool bEmit = true;
                EHomeIssue Code = EHomeIssue::MaterialRecipeUnported;
                const TCHAR* Suffix = TEXT(".materials");
                if (Phase == 17) { bEmit = Out.Shell.IsSet(); Code = EHomeIssue::PlacementValidationUnavailable; Suffix = TEXT(".items"); }
                if (Phase == 18) { bEmit = Out.Shell.IsSet(); Code = EHomeIssue::NavigationUnported; Suffix = TEXT(".spawn"); }
                if (Phase == 19) { bEmit = !Out.Shell.IsSet(); Code = EHomeIssue::LayoutNotMapped; Suffix = TEXT(".layoutId"); }
                if (Phase == 20) { bEmit = Out.Asset.IsSet(); Code = EHomeIssue::AssetImportUnavailable; Suffix = TEXT(".asset"); }
                if (bEmit)
                {
                    Result = Work.Admit(512, (Issues.Num() + 1) * sizeof(FHomeMappingIssue) + (Path.Len() + 16) * sizeof(TCHAR));
                    if (Result != EMappingStep::Complete) return Result;
                    Issue(Issues, Code, Path + Suffix);
                }
                break;
            }
            }
            if (Result != EMappingStep::Complete) return Result;
            ++Phase; Index = 0; Choice.Begin();
        }
        return EMappingStep::Complete;
    }
    EMappingStep FRoomMapping::Retire(FMappingWork& Work)
    {
        while (RetireField < 5)
        {
            FString* Text = nullptr;
            if (RetireField == 0) Text = &ItemOutput.Id;
            if (RetireField == 1 && ItemOutput.Kind.Saved.IsSet()) Text = &ItemOutput.Kind.Saved.GetValue();
            if (RetireField == 2 && ItemOutput.Surface.Saved.IsSet()) Text = &ItemOutput.Surface.Saved.GetValue();
            if (RetireField == 3) Text = &ItemOutput.SourceGeometryRecipe;
            if (RetireField == 4) Text = &ItemPath;
            const auto Admission = Text ? Work.ReleaseText(*Text) : Work.Admit(32);
            if (Admission != EMappingStep::Complete) return Admission;
            ++RetireField;
        }
        const auto Admission = Work.Admit(128);
        if (Admission != EMappingStep::Complete) return Admission;
        const auto FurnitureResult = Furniture.Retire(Work);
        if (FurnitureResult != EMappingStep::Complete) return FurnitureResult;
        ItemOutput = {}; Identity = nullptr; Source = nullptr; Output = nullptr;
        SourcePath = nullptr; OutputIssues = nullptr;
        return EMappingStep::Complete;
    }
    bool ReadRoom(const FRoomSelection& Identity, FRoomPlan& Out, const FString& Path,
        TArray<FHomeMappingIssue>& Issues)
    {
        Out = {};
        FObject Object;
        if (!ReadObject(Identity.RoomJson, Object)) return false;
        FRoomMappingInput Input;
        Input.Room = FMappingObject(Object);
        Input.bDesignPresent = Object->HasField(TEXT("homeDesign"));
        const TSharedPtr<FJsonObject>* Design = nullptr;
        Input.bDesignObject = Object->TryGetObjectField(TEXT("homeDesign"), Design) && Design && Design->IsValid();
        if (Input.bDesignObject) Input.Design = FMappingObject(*Design);
        const TArray<TSharedPtr<FJsonValue>>* Items = nullptr;
        Input.bItemsArray = Object->TryGetArrayField(TEXT("items"), Items) && Items;
        if (Input.bItemsArray)
        {
            Input.ItemCount = static_cast<uint32>(Items->Num());
            for (uint32 I = 0; I < FMath::Min<uint32>(Input.ItemCount, UE_ARRAY_COUNT(Input.Items)); ++I)
                if ((*Items)[I].IsValid() && (*Items)[I]->Type == EJson::Object)
                    Input.Items[I] = FMappingObject((*Items)[I]->AsObject());
        }
        FLegacyMappingWork Work;
        FRoomMapping Mapping;
        Mapping.Begin(Identity, Input, Out, Path, Issues);
        const bool bComplete = Mapping.Advance(Work) == EMappingStep::Complete;
        Mapping.Retire(Work);
        return bComplete;
    }
}
