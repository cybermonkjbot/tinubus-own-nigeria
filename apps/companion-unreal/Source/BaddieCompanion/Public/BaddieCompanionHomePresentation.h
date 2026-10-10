#pragma once

#include "CoreMinimal.h"
#include "BaddieCompanionSnapshot.h"

namespace BaddieCompanion
{
    // C++ process-memory plans only: no reflection, SaveGame, asset loads or gameplay authority.
    enum class EHomeIssue : uint8
    {
        InvalidDocument, IdentityMismatch, UnknownField, UnknownSelection, InvalidDesign,
        DuplicateItemId, LegacyLayout, LayoutNotMapped, MissingSurface, InvalidPlacement,
        AssetImportUnavailable, MaterialRecipeUnported, BaseRecipeUnported, FaceRecipeUnported,
        WardrobeRecipeUnported, AttachmentUnverified, PropRecipeUnported, MissingReference,
        NavigationUnported, PlacementValidationUnavailable
    };
    struct FHomeMappingIssue
    {
        EHomeIssue Code = EHomeIssue::InvalidDocument;
        // Fixed contract path, optionally an array index. Never a payload, credential or account ID.
        FString Field;
    };
    enum class ERecipeStatus : uint8 { Unported };
    enum class EImportStatus : uint8 { UnverifiedUnavailable };
    enum class ECharacterBase : uint8 { Bestie, Bunny, Bear, Cat, Fox, Panda };
    enum class ELook : uint8 { Rose, Ivory };
    enum class EGarment : uint8 { Bomber, TShirt, Shirt, ALineDress, PleatedDress, WrapDress, EveningGown };
    enum class EFabric : uint8 { Cotton, Linen, Knit, Denim, Satin, Velvet };
    enum class EPattern : uint8 { Plain, Stripes, Gingham, Dots };
    enum class EEyeShape : uint8 { Classic, Round, Almond, Sleepy };
    enum class EBrowStyle : uint8 { Classic, Soft, Bold, Curious };
    enum class ENoseShape : uint8 { Classic, Button, Wide };
    enum class EMouthShape : uint8 { Classic, Small, Wide };
    enum class ECheekStyle : uint8 { None, Blush, Freckles };
    enum class EMarkingStyle : uint8 { None, Muzzle, Mask, Stripes };
    enum class EEyewear : uint8 { None, Round };
    enum class EEnvironment : uint8 { Studio, Terrace };
    enum class EPalette : uint8 { Rose, Sage, Midnight };
    enum class ELight : uint8 { Daylight, Golden, Evening };
    enum class ESharing : uint8 { Private, Host, Collaborative };
    enum class ELayout : uint8 { PatStudioV1, PinkBedroomV1, PinkBedroomV2, KitchenV1, BathroomV1 };
    enum class ESurface : uint8 { Floor, Console, Shelf, WallBack, WallLeft, WallRight, WallFront };
    enum class EItemKind : uint8
    {
        Lamp, Stool, Plant, BotanicalPainting, AbstractPainting, LandscapePainting, WallClock, WallMirror,
        StandingFan, TowerFan, DeskFan, FloorLamp, DeskLamp, Armchair, Loveseat, Ottoman, CoffeeTable,
        SideTable, Bookcase, StorageBasket, BookStack, Candle, Succulent
    };
    enum class EPlacement : uint8 { Floor, Tabletop, Wall };

    template<typename T> struct TSourceChoice
    {
        // Saved presence is separate from a renderer default. Unknown saved values have no Effective.
        TOptional<FString> Saved;
        TOptional<T> Effective;
    };
    struct FFabricRecipe { double Roughness = 0, Bump = 0, Repeat = 0; };
    struct FAppearancePlan
    {
        TSourceChoice<ECharacterBase> Base;
        TSourceChoice<ELook> Look;
        TOptional<FString> BodyColor, TopColor, TrousersColor, FootwearColor,
            EyeColor, NoseColor, MarkingColor, PatternColor;
        TOptional<bool> AccessoryVisible, NecklaceVisible, HipChainVisible;
        TOptional<bool> EffectiveNecklaceVisible, EffectiveHipChainVisible;
        TSourceChoice<EGarment> Garment;
        TSourceChoice<EFabric> Fabric;
        // normalizeAppearance's picker default is cotton; rendering omission preserves authored fabric.
        bool bPreserveAuthoredFabric = true;
        TOptional<FFabricRecipe> FabricRecipe;
        TSourceChoice<EPattern> Pattern;
        TSourceChoice<EEyeShape> Eyes;
        TSourceChoice<EBrowStyle> Brows;
        TSourceChoice<ENoseShape> Nose;
        TSourceChoice<EMouthShape> Mouth;
        TSourceChoice<ECheekStyle> Cheeks;
        TSourceChoice<EMarkingStyle> Markings;
        TSourceChoice<EEyewear> Eyewear;
        // Render defaults, NOT normalizeAppearance's blanket colour constants.
        TOptional<FString> EffectiveBodyColor, EffectiveNoseColor;
        FString EffectiveMarkingColor, EffectivePatternColor;
        bool bCompleteKnownSelections = false;
        ERecipeStatus BaseRecipe = ERecipeStatus::Unported;
        ERecipeStatus FaceRecipe = ERecipeStatus::Unported;
        ERecipeStatus WardrobeRecipe = ERecipeStatus::Unported;
        // Shop equipment/ownership is not supplied by this appearance snapshot.
    };
    enum class EColorRule : uint8 { PreserveAuthored, ExplicitOverride, AutomaticBase, LookDefault, IrisLuminanceTint };
    struct FCharacterMaterialRequirement
    {
        FString SourceMaterialName, SourceField;
        EColorRule Rule = EColorRule::PreserveAuthored;
        TOptional<FString> TargetHex;
        ERecipeStatus Translation = ERecipeStatus::Unported;
    };
    struct FAttachmentRequirement
    {
        FString SourceNode, SourceRecipe;
        TOptional<bool> EffectiveVisible;
        // These are source identifiers, never manufactured Unreal socket names.
        EImportStatus AttachmentStatus = EImportStatus::UnverifiedUnavailable;
    };
    struct FCanonicalAssetReference
    {
        FString Key, Path, Sha256, Md5;
        int64 Bytes = 0;
        bool bPublished = false;
        EImportStatus GeometryImport = EImportStatus::UnverifiedUnavailable;
        EImportStatus Materials = EImportStatus::UnverifiedUnavailable;
        EImportStatus Rig = EImportStatus::UnverifiedUnavailable;
        EImportStatus Attachments = EImportStatus::UnverifiedUnavailable;
    };

    /** Runtime metres [x,y,z] -> UE cm [100*z,100*x,100*y]. No Blender offset applied here.
     * Source yaw turns +Z toward +X; UE yaw turns +X toward +Y: same positive sign.
     * Geometry import must bake this basis (including handedness/winding), not add another room turn.
     */
    BADDIECOMPANION_API FVector SourceToUnreal(const FVector& Metres);
    BADDIECOMPANION_API double SourceYawToUnreal(double Radians);
    BADDIECOMPANION_API FVector TurnSourceShell(const FVector& Metres, int32 Turns);
    struct FSourcePose
    {
        FVector Metres = FVector::ZeroVector;
        double YawRadians = 0;
        FVector UnrealCentimetres = FVector::ZeroVector;
        double UnrealYawDegrees = 0;
    };
    struct FSourceBox
    {
        // Centre horizontally, BOTTOM vertically; dimensions are width(X), height(Y), depth(Z).
        FVector BottomCentreMetres = FVector::ZeroVector;
        FVector DimensionsMetres = FVector::ZeroVector;
        FBox UnrealBounds = FBox(ForceInit);
    };
    struct FLayoutObstacle { FString Id; FSourceBox Box; TOptional<double> SavedBottom; };
    struct FWallOpening { double U = 0, BottomY = 0, Width = 0, Height = 0; FBox UnrealBounds = FBox(ForceInit); };
    struct FWallPlan
    {
        ESurface Id = ESurface::WallBack;
        FString SourceId, Name;
        FSourcePose Anchor;
        double Width = 0, MinY = 0, MaxY = 0;
        TArray<FWallOpening> Openings;
    };
    struct FSupportPlan
    {
        ESurface Id = ESurface::Floor;
        FString SourceId, Name;
        FSourceBox Top;
        TArray<EItemKind> AllowedKinds;
        TArray<FSourcePose> Anchors;
        TArray<FString> SupportingObstacleIds;
        TOptional<FSourceBox> Clearance;
    };
    struct FCameraReference { FVector EyeMetres, TargetMetres, EyeUnrealCm, TargetUnrealCm; };
    struct FBedroomShellPlan
    {
        FString DerivedLayoutId;
        double MinX = 0, MaxX = 0, MinZ = 0, MaxZ = 0;
        // Horizontal measured bounds, not an invented GLB vertical extent.
        FBox UnrealFloorBounds = FBox(ForceInit);
        double ObjectScale = .45, CharacterScale = .38, CharacterRadius = .24, CharacterHeight = 1.4;
        double ShellYawRadians = 0, ShellYawDegrees = 0;
        TArray<FLayoutObstacle> Obstacles;
        TArray<FSourceBox> FloorPatches;
        TArray<FWallPlan> Walls;
        TArray<FSupportPlan> Supports;
        TArray<FSourcePose> StandingCandidates;
        FSourcePose AuthoredSpawn;
        FCameraReference Wide, PortraitWide, Close;
        double FieldOfView = 64, CameraOrbitLimit = .12;
        // This is the authored preferred spawn only; personalEnvironment's swept navigation is unported.
        bool bSpawnClearanceVerified = false;
    };
    struct FHomeDesignPlan { int32 Version = 1, Turns = 0; double Extension = 0; };
    struct FItemDimensions
    {
        // Original catalog geometry units versus roomBody's authored silhouette/support dimensions.
        double CatalogRadius = 0, CatalogHeight = 0, Mass = 0;
        double RadiusMetres = 0, SupportRadiusMetres = 0, HeightMetres = 0;
        TOptional<double> WallWidthMetres, WallDepthMetres;
        EPlacement Placement = EPlacement::Floor;
    };
    enum class EItemPivot : uint8 { BottomSupportRoot, BottomWallAnchor };
    struct FFurniturePlan
    {
        FString Id;
        TSourceChoice<EItemKind> Kind;
        double X = 0, Z = 0, Rotation = 0;
        TSourceChoice<ESurface> Surface;
        TOptional<double> Y;
        TOptional<bool> Lit;
        bool bEffectiveLit = true;
        TOptional<FItemDimensions> Dimensions;
        // Support-derived pose only; absence is unresolved placement, presence is not collision clearance.
        TOptional<FSourcePose> PlacementPose;
        TOptional<FSourceBox> Volume;
        TOptional<double> WallLocalU;
        EItemPivot Pivot = EItemPivot::BottomSupportRoot;
        FString SourceGeometryRecipe;
        // No GLB asset key exists for makeRoomObject props.
        ERecipeStatus GeometryTranslation = ERecipeStatus::Unported;
        bool bPlacementValid = false;
    };
    struct FRoomPlan
    {
        FString Id, Name;
        TSourceChoice<EEnvironment> Environment;
        TSourceChoice<ELayout> Layout;
        TSourceChoice<EPalette> Palette;
        TSourceChoice<ELight> Light;
        TSourceChoice<ESharing> Sharing;
        TOptional<FString> WallColor, FloorColor, UpholsteryColor, RugColor;
        TOptional<FHomeDesignPlan> SavedDesign;
        TArray<FFurniturePlan> Items;
        TOptional<FCanonicalAssetReference> Asset;
        TOptional<FBedroomShellPlan> BedroomShell;
        // Layout data only. Item issues and imported-asset availability remain separate.
        bool bSourceMapped = false;
        ERecipeStatus MaterialTranslation = ERecipeStatus::Unported;
    };
    struct FHomePresentationPlan
    {
        // Preserve exact documents, IDs, names and revision in private process memory only.
        FSnapshotSelection Source;
        FRoomPlan Home, ActiveRoom;
        FAppearancePlan Appearance;
        FCanonicalAssetReference CharacterAsset;
        TArray<FCharacterMaterialRequirement> CharacterMaterials;
        TArray<FAttachmentRequirement> Attachments;
        TArray<FHomeMappingIssue> Issues;
        // Asset references and valid transforms never establish render readiness or admission.
        bool IsRenderReady() const { return false; }
    };
    // Pure translation, NOT a validity/admission boundary. Malformed input clears Out on failure.
    BADDIECOMPANION_API bool BuildHomePresentationPlan(const FSnapshotSelection& Input,
        FHomePresentationPlan& Out, TArray<FHomeMappingIssue>& OutIssues);
    BADDIECOMPANION_API bool SamePresentationInput(const FSnapshotSelection& A, const FSnapshotSelection& B);

    /** Owned only by the live controller. Exact-input caching, including failures, never revision-only. */
    class BADDIECOMPANION_API FHomePresentationCache
    {
    public:
        void Clear();
        bool Matches(const FSnapshotSelection& Input) const;
        bool HasInput() const { return LastInput.IsSet(); }
        void Update(const FSnapshotSelection& Input);
        bool CopyPlan(FHomePresentationPlan& Out) const;
        const FHomePresentationPlan* GetPlan() const { return Plan.IsSet() ? &Plan.GetValue() : nullptr; }
        const TArray<FHomeMappingIssue>& GetIssues() const { return Issues; }
    private:
        TOptional<FSnapshotSelection> LastInput;
        TOptional<FHomePresentationPlan> Plan;
        TArray<FHomeMappingIssue> Issues;
    };
}
