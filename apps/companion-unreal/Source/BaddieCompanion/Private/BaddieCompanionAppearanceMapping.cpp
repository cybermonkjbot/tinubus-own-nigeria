#include "BaddieCompanionHomeMappingInternal.h"

namespace BaddieCompanion::HomeMapping
{
    bool ReadAppearance(const FCharacterSelection& Input, FAppearancePlan& Out,
        TArray<FCharacterMaterialRequirement>& Materials, TArray<FAttachmentRequirement>& Attachments,
        TArray<FHomeMappingIssue>& Issues)
    {
        FObject Object;
        if (Input.Id.Len() > 80 || Input.Id.TrimStartAndEnd().IsEmpty()
            || Input.Name.Len() > 20 || Input.Name.TrimStartAndEnd().IsEmpty()
            || !ReadObject(Input.AppearanceJson, Object)) return false;
        const FString Path = TEXT("character.appearance");
        UnknownFields(Object, { TEXT("baseId"), TEXT("look"), TEXT("bodyColor"), TEXT("topColor"),
            TEXT("trousersColor"), TEXT("footwearColor"), TEXT("eyeColor"), TEXT("noseColor"),
            TEXT("markingColor"), TEXT("patternColor"), TEXT("accessoryVisible"), TEXT("necklaceVisible"),
            TEXT("hipChainVisible"), TEXT("garmentStyle"), TEXT("fabricTexture"), TEXT("clothingPattern"),
            TEXT("eyeShape"), TEXT("browStyle"), TEXT("noseShape"), TEXT("mouthShape"), TEXT("cheekStyle"),
            TEXT("markingStyle"), TEXT("eyewear") }, Path, Issues);
        if (!Choice<ECharacterBase>(Object, TEXT("baseId"), Out.Base, {
                { TEXT("bestie"), ECharacterBase::Bestie }, { TEXT("bunny"), ECharacterBase::Bunny },
                { TEXT("bear"), ECharacterBase::Bear }, { TEXT("cat"), ECharacterBase::Cat },
                { TEXT("fox"), ECharacterBase::Fox }, { TEXT("panda"), ECharacterBase::Panda }
            }, false, Path, Issues, ECharacterBase::Bestie)
            || !Choice<ELook>(Object, TEXT("look"), Out.Look, {
                { TEXT("rose"), ELook::Rose }, { TEXT("ivory"), ELook::Ivory }
            }, true, Path, Issues)) return false;
        if (!Same(Out.Base.Saved.IsSet() ? Out.Base.Saved.GetValue() : TEXT("bestie"), Input.BaseId)
            || !Same(Out.Look.Saved.GetValue(), Input.Look))
        {
            Issue(Issues, EHomeIssue::IdentityMismatch, Path);
            return false;
        }
        if (!OptionalColor(Object, TEXT("bodyColor"), Out.BodyColor)
            || !OptionalColor(Object, TEXT("topColor"), Out.TopColor)
            || !OptionalColor(Object, TEXT("trousersColor"), Out.TrousersColor)
            || !OptionalColor(Object, TEXT("footwearColor"), Out.FootwearColor)
            || !OptionalColor(Object, TEXT("eyeColor"), Out.EyeColor)
            || !OptionalColor(Object, TEXT("noseColor"), Out.NoseColor)
            || !OptionalColor(Object, TEXT("markingColor"), Out.MarkingColor)
            || !OptionalColor(Object, TEXT("patternColor"), Out.PatternColor)
            || !OptionalBool(Object, TEXT("accessoryVisible"), Out.AccessoryVisible)
            || !OptionalBool(Object, TEXT("necklaceVisible"), Out.NecklaceVisible)
            || !OptionalBool(Object, TEXT("hipChainVisible"), Out.HipChainVisible)) return false;
        Out.EffectiveNecklaceVisible = Out.NecklaceVisible.Get(Out.AccessoryVisible.Get(true));
        Out.EffectiveHipChainVisible = Out.HipChainVisible.Get(Out.AccessoryVisible.Get(true));
        if (!Choice<EGarment>(Object, TEXT("garmentStyle"), Out.Garment, {
                { TEXT("bomber"), EGarment::Bomber }, { TEXT("t-shirt"), EGarment::TShirt },
                { TEXT("shirt"), EGarment::Shirt }, { TEXT("a-line-dress"), EGarment::ALineDress },
                { TEXT("pleated-dress"), EGarment::PleatedDress }, { TEXT("wrap-dress"), EGarment::WrapDress },
                { TEXT("evening-gown"), EGarment::EveningGown }
            }, false, Path, Issues, EGarment::Bomber)
            || !Choice<EFabric>(Object, TEXT("fabricTexture"), Out.Fabric, {
                { TEXT("cotton"), EFabric::Cotton }, { TEXT("linen"), EFabric::Linen },
                { TEXT("knit"), EFabric::Knit }, { TEXT("denim"), EFabric::Denim },
                { TEXT("satin"), EFabric::Satin }, { TEXT("velvet"), EFabric::Velvet }
            }, false, Path, Issues)
            || !Choice<EPattern>(Object, TEXT("clothingPattern"), Out.Pattern, {
                { TEXT("plain"), EPattern::Plain }, { TEXT("stripes"), EPattern::Stripes },
                { TEXT("gingham"), EPattern::Gingham }, { TEXT("dots"), EPattern::Dots }
            }, false, Path, Issues, EPattern::Plain)) return false;
        Out.bPreserveAuthoredFabric = !Out.Fabric.Saved.IsSet();
        if (Out.Fabric.Effective.IsSet())
        {
            switch (Out.Fabric.Effective.GetValue())
            {
            case EFabric::Cotton: Out.FabricRecipe = FFabricRecipe{ .88, .004, 10 }; break;
            case EFabric::Linen: Out.FabricRecipe = FFabricRecipe{ .96, .012, 6 }; break;
            case EFabric::Knit: Out.FabricRecipe = FFabricRecipe{ .9, .018, 4 }; break;
            case EFabric::Denim: Out.FabricRecipe = FFabricRecipe{ .84, .01, 9 }; break;
            case EFabric::Satin: Out.FabricRecipe = FFabricRecipe{ .28, .002, 14 }; break;
            case EFabric::Velvet: Out.FabricRecipe = FFabricRecipe{ .98, .007, 16 }; break;
            }
        }
        TOptional<EEyeShape> Eyes;
        TOptional<EBrowStyle> Brows;
        TOptional<ENoseShape> Nose;
        TOptional<EMarkingStyle> Markings;
        Out.EffectiveBodyColor = Out.BodyColor;
        Out.EffectiveNoseColor = Out.NoseColor;
        Out.EffectiveMarkingColor = Out.MarkingColor.Get(TEXT("#795342"));
        Out.EffectivePatternColor = Out.PatternColor.Get(TEXT("#eee9df"));
        if (Out.Base.Effective.IsSet())
        {
            Eyes = EEyeShape::Classic; Brows = EBrowStyle::Classic;
            Nose = ENoseShape::Classic; Markings = EMarkingStyle::None;
            const TCHAR* Body = nullptr;
            const TCHAR* NoseHex = nullptr;
            const TCHAR* MarkingHex = nullptr;
            switch (Out.Base.Effective.GetValue())
            {
            case ECharacterBase::Cat:
                Eyes = EEyeShape::Almond; Nose = ENoseShape::Button; Markings = EMarkingStyle::Stripes;
                Body = TEXT("#a3a6ac"); NoseHex = TEXT("#b97e74"); MarkingHex = TEXT("#50525b"); break;
            case ECharacterBase::Fox:
                Eyes = EEyeShape::Almond; Brows = EBrowStyle::Curious; Markings = EMarkingStyle::Muzzle;
                Body = TEXT("#cf8150"); NoseHex = TEXT("#393634"); MarkingHex = TEXT("#f4dfbe"); break;
            case ECharacterBase::Panda:
                Eyes = EEyeShape::Round; Nose = ENoseShape::Wide; Markings = EMarkingStyle::Mask;
                Body = TEXT("#eee9df"); NoseHex = TEXT("#393634"); MarkingHex = TEXT("#393634"); break;
            default: break; // Bestie/bunny/bear preserve authored body/nose when overrides are absent.
            }
            if (!Out.BodyColor.IsSet() && Body) Out.EffectiveBodyColor = FString(Body);
            if (!Out.NoseColor.IsSet() && NoseHex) Out.EffectiveNoseColor = FString(NoseHex);
            if (!Out.MarkingColor.IsSet() && MarkingHex) Out.EffectiveMarkingColor = MarkingHex;
        }
        if (!Choice<EEyeShape>(Object, TEXT("eyeShape"), Out.Eyes, {
                { TEXT("classic"), EEyeShape::Classic }, { TEXT("round"), EEyeShape::Round },
                { TEXT("almond"), EEyeShape::Almond }, { TEXT("sleepy"), EEyeShape::Sleepy }
            }, false, Path, Issues, Eyes)
            || !Choice<EBrowStyle>(Object, TEXT("browStyle"), Out.Brows, {
                { TEXT("classic"), EBrowStyle::Classic }, { TEXT("soft"), EBrowStyle::Soft },
                { TEXT("bold"), EBrowStyle::Bold }, { TEXT("curious"), EBrowStyle::Curious }
            }, false, Path, Issues, Brows)
            || !Choice<ENoseShape>(Object, TEXT("noseShape"), Out.Nose, {
                { TEXT("classic"), ENoseShape::Classic }, { TEXT("button"), ENoseShape::Button },
                { TEXT("wide"), ENoseShape::Wide }
            }, false, Path, Issues, Nose)
            || !Choice<EMouthShape>(Object, TEXT("mouthShape"), Out.Mouth, {
                { TEXT("classic"), EMouthShape::Classic }, { TEXT("small"), EMouthShape::Small },
                { TEXT("wide"), EMouthShape::Wide }
            }, false, Path, Issues, EMouthShape::Classic)
            || !Choice<ECheekStyle>(Object, TEXT("cheekStyle"), Out.Cheeks, {
                { TEXT("none"), ECheekStyle::None }, { TEXT("blush"), ECheekStyle::Blush },
                { TEXT("freckles"), ECheekStyle::Freckles }
            }, false, Path, Issues, ECheekStyle::None)
            || !Choice<EMarkingStyle>(Object, TEXT("markingStyle"), Out.Markings, {
                { TEXT("none"), EMarkingStyle::None }, { TEXT("muzzle"), EMarkingStyle::Muzzle },
                { TEXT("mask"), EMarkingStyle::Mask }, { TEXT("stripes"), EMarkingStyle::Stripes }
            }, false, Path, Issues, Markings)
            || !Choice<EEyewear>(Object, TEXT("eyewear"), Out.Eyewear, {
                { TEXT("none"), EEyewear::None }, { TEXT("round"), EEyewear::Round }
            }, false, Path, Issues, EEyewear::None)) return false;
        Out.bCompleteKnownSelections = Out.Base.Effective.IsSet() && Out.Look.Effective.IsSet()
            && Out.Garment.Effective.IsSet() && (!Out.Fabric.Saved.IsSet() || Out.Fabric.Effective.IsSet())
            && Out.Pattern.Effective.IsSet() && Out.Eyes.Effective.IsSet() && Out.Brows.Effective.IsSet()
            && Out.Nose.Effective.IsSet() && Out.Mouth.Effective.IsSet() && Out.Cheeks.Effective.IsSet()
            && Out.Markings.Effective.IsSet() && Out.Eyewear.Effective.IsSet();
        const auto Material = [&Materials](const TCHAR* Name, const TCHAR* Field,
            const TOptional<FString>& Hex, EColorRule Rule = EColorRule::ExplicitOverride)
        {
            Materials.Add({ Name, Field, Hex.IsSet() ? Rule : EColorRule::PreserveAuthored, Hex });
        };
        const auto BodyRule = Out.BodyColor.IsSet() ? EColorRule::ExplicitOverride : EColorRule::AutomaticBase;
        Material(TEXT("Peach cream fur"), TEXT("bodyColor"), Out.EffectiveBodyColor, BodyRule);
        Material(TEXT("Cream guard hairs"), TEXT("bodyColor"), Out.EffectiveBodyColor, BodyRule);
        TOptional<FString> Jacket = Out.TopColor, Ribbing = Out.TopColor;
        if (!Out.TopColor.IsSet() && Out.Look.Effective.IsSet())
        {
            const bool bIvory = Out.Look.Effective.GetValue() == ELook::Ivory;
            Jacket = FString(bIvory ? TEXT("#e7dfcc") : TEXT("#b97e74"));
            Ribbing = FString(bIvory ? TEXT("#c6bca5") : TEXT("#95685f"));
        }
        const auto TopRule = Out.TopColor.IsSet() ? EColorRule::ExplicitOverride : EColorRule::LookDefault;
        Material(TEXT("Jacket fabric"), TEXT("topColor"), Jacket, TopRule);
        Material(TEXT("Jacket ribbing"), TEXT("topColor"), Ribbing, TopRule);
        Material(TEXT("Charcoal twill"), TEXT("trousersColor"), Out.TrousersColor);
        Material(TEXT("Twill seams"), TEXT("trousersColor"), Out.TrousersColor);
        Material(TEXT("Ivory leather"), TEXT("footwearColor"), Out.FootwearColor);
        Material(TEXT("Rose leather"), TEXT("footwearColor"), Out.FootwearColor);
        Material(TEXT("Shoe sole"), TEXT("footwearColor"), Out.FootwearColor);
        Material(TEXT("Amber iris"), TEXT("eyeColor"), Out.EyeColor, EColorRule::IrisLuminanceTint);
        Material(TEXT("Peach nose"), TEXT("noseColor"), Out.EffectiveNoseColor,
            Out.NoseColor.IsSet() ? EColorRule::ExplicitOverride : EColorRule::AutomaticBase);
        // Ear velvet is handled by faceRig's marking recipe, not the body override.
        Attachments.Add({ TEXT("AccessoryNecklace"), TEXT("rig.accessoryVisibility"), Out.EffectiveNecklaceVisible });
        Attachments.Add({ TEXT("AccessoryChain"), TEXT("rig.accessoryVisibility"), Out.EffectiveHipChainVisible });
        TOptional<bool> Glasses;
        if (Out.Eyewear.Effective.IsSet()) Glasses = Out.Eyewear.Effective.GetValue() == EEyewear::Round;
        Attachments.Add({ TEXT("Head"), TEXT("faceRig.BestieGlasses"), Glasses });
        // All source binding identifiers are requirements, not confirmed imported nodes or UE sockets.
        for (const TCHAR* Node : { TEXT("Chest"), TEXT("Hips"), TEXT("DeformHips"),
            TEXT("DeformLegL"), TEXT("DeformLegR"), TEXT("ShoulderL"), TEXT("ShoulderR"),
            TEXT("ElbowL"), TEXT("ElbowR"), TEXT("HandL"), TEXT("HandR"),
            TEXT("DeformShoulderL"), TEXT("DeformShoulderR"), TEXT("DeformElbowL"), TEXT("DeformElbowR"),
            TEXT("LegL"), TEXT("LegR"), TEXT("KneeL"), TEXT("KneeR") })
            Attachments.Add({ Node, TEXT("wardrobeRig.BestieWardrobe"), {} });
        Issue(Issues, EHomeIssue::BaseRecipeUnported, Path + TEXT(".baseId"));
        Issue(Issues, EHomeIssue::FaceRecipeUnported, Path + TEXT(".face"));
        Issue(Issues, EHomeIssue::WardrobeRecipeUnported, Path + TEXT(".wardrobe"));
        Issue(Issues, EHomeIssue::MaterialRecipeUnported, Path + TEXT(".materials"));
        Issue(Issues, EHomeIssue::AttachmentUnverified, Path + TEXT(".attachments"));
        return true;
    }
}
