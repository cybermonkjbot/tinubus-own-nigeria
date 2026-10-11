#include "BaddieCompanionHomeMappingInternal.h"

namespace BaddieCompanion::HomeMapping
{
    void FAppearanceMapping::Begin(const FCharacterSelection& InIdentity, FMappingObject InObject,
        FAppearancePlan& Appearance, TArray<FCharacterMaterialRequirement>& InMaterials,
        TArray<FAttachmentRequirement>& InAttachments, TArray<FHomeMappingIssue>& InIssues)
    {
        Identity = &InIdentity; Object = MoveTemp(InObject); Output = &Appearance;
        Materials = &InMaterials; Attachments = &InAttachments; Issues = &InIssues;
        Eyes.Reset(); Brows.Reset(); Nose.Reset(); Markings.Reset();
        Phase = Index = 0; Choice.Begin();
    }
    EMappingStep FAppearanceMapping::Advance(FMappingWork& Work)
    {
        if (!Identity || !Output || !Materials || !Attachments || !Issues) return EMappingStep::Invalid;
        const auto& Input = *Identity;
        auto& Out = *Output;
        auto& MaterialOutput = *Materials;
        auto& AttachmentOutput = *Attachments;
        auto& IssueOutput = *Issues;
        while (Phase < 29)
        {
        EMappingStep Result = EMappingStep::Complete;
        switch (Phase)
        {
        case 0:
        {
        const auto Admission = Work.Admit(1024, 64 * sizeof(TCHAR));
        if (Admission != EMappingStep::Complete) return Admission;
        if (Input.Id.Len() > 80 || Input.Id.TrimStartAndEnd().IsEmpty()
            || Input.Name.Len() > 20 || Input.Name.TrimStartAndEnd().IsEmpty()) return EMappingStep::Invalid;
        Path = TEXT("character.appearance");
        UnknownFields(Object, AppearanceFields(), Path, IssueOutput);
        break;
        }
        case 1:
        Result = Choice.Advance<ECharacterBase>(Work, Object, TEXT("baseId"), Out.Base, {
                { TEXT("bestie"), ECharacterBase::Bestie }, { TEXT("bunny"), ECharacterBase::Bunny },
                { TEXT("bear"), ECharacterBase::Bear }, { TEXT("cat"), ECharacterBase::Cat },
                { TEXT("fox"), ECharacterBase::Fox }, { TEXT("panda"), ECharacterBase::Panda }
            }, false, Path, IssueOutput, ECharacterBase::Bestie);
        break;
        case 2:
        Result = Choice.Advance<ELook>(Work, Object, TEXT("look"), Out.Look, {
                { TEXT("rose"), ELook::Rose }, { TEXT("ivory"), ELook::Ivory }
            }, true, Path, IssueOutput);
        break;
        case 3:
        {
        const auto Admission = Work.Admit(1024, (IssueOutput.Num() + 1) * sizeof(FHomeMappingIssue) + 64 * sizeof(TCHAR));
        if (Admission != EMappingStep::Complete) return Admission;
        if (!Same(Out.Base.Saved.IsSet() ? Out.Base.Saved.GetValue() : TEXT("bestie"), Input.BaseId)
            || !Same(Out.Look.Saved.GetValue(), Input.Look))
        {
            Issue(IssueOutput, EHomeIssue::IdentityMismatch, Path);
            return EMappingStep::Invalid;
        }
        break;
        }
        case 4:
        {
            static const TCHAR* Names[] = { TEXT("bodyColor"), TEXT("topColor"), TEXT("trousersColor"),
                TEXT("footwearColor"), TEXT("eyeColor"), TEXT("noseColor"), TEXT("markingColor"), TEXT("patternColor") };
            TOptional<FString>* Colors[] = { &Out.BodyColor, &Out.TopColor, &Out.TrousersColor,
                &Out.FootwearColor, &Out.EyeColor, &Out.NoseColor, &Out.MarkingColor, &Out.PatternColor };
            if (Index < UE_ARRAY_COUNT(Names))
            {
                const auto Admission = Work.Admit(1024, 16 * sizeof(TCHAR));
                if (Admission != EMappingStep::Complete) return Admission;
                if (!OptionalColor(Object, Names[Index], *Colors[Index])) return EMappingStep::Invalid;
                ++Index; continue;
            }
            break;
        }
        case 5:
        {
            static const TCHAR* Names[] = { TEXT("accessoryVisible"), TEXT("necklaceVisible"), TEXT("hipChainVisible") };
            TOptional<bool>* Values[] = { &Out.AccessoryVisible, &Out.NecklaceVisible, &Out.HipChainVisible };
            if (Index < UE_ARRAY_COUNT(Names))
            {
                const auto Admission = Work.Admit(512);
                if (Admission != EMappingStep::Complete) return Admission;
                if (!OptionalBool(Object, Names[Index], *Values[Index])) return EMappingStep::Invalid;
                ++Index; continue;
            }
            break;
        }
        case 6:
        {
        const auto Admission = Work.Admit(64);
        if (Admission != EMappingStep::Complete) return Admission;
        Out.EffectiveNecklaceVisible = Out.NecklaceVisible.Get(Out.AccessoryVisible.Get(true));
        Out.EffectiveHipChainVisible = Out.HipChainVisible.Get(Out.AccessoryVisible.Get(true));
        break;
        }
        case 7:
        Result = Choice.Advance<EGarment>(Work, Object, TEXT("garmentStyle"), Out.Garment, {
                { TEXT("bomber"), EGarment::Bomber }, { TEXT("t-shirt"), EGarment::TShirt },
                { TEXT("shirt"), EGarment::Shirt }, { TEXT("a-line-dress"), EGarment::ALineDress },
                { TEXT("pleated-dress"), EGarment::PleatedDress }, { TEXT("wrap-dress"), EGarment::WrapDress },
                { TEXT("evening-gown"), EGarment::EveningGown }
            }, false, Path, IssueOutput, EGarment::Bomber);
        break;
        case 8:
        Result = Choice.Advance<EFabric>(Work, Object, TEXT("fabricTexture"), Out.Fabric, {
                { TEXT("cotton"), EFabric::Cotton }, { TEXT("linen"), EFabric::Linen },
                { TEXT("knit"), EFabric::Knit }, { TEXT("denim"), EFabric::Denim },
                { TEXT("satin"), EFabric::Satin }, { TEXT("velvet"), EFabric::Velvet }
            }, false, Path, IssueOutput);
        break;
        case 9:
        Result = Choice.Advance<EPattern>(Work, Object, TEXT("clothingPattern"), Out.Pattern, {
                { TEXT("plain"), EPattern::Plain }, { TEXT("stripes"), EPattern::Stripes },
                { TEXT("gingham"), EPattern::Gingham }, { TEXT("dots"), EPattern::Dots }
            }, false, Path, IssueOutput, EPattern::Plain);
        break;
        case 10:
        {
        const auto Admission = Work.Admit(128);
        if (Admission != EMappingStep::Complete) return Admission;
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
        break;
        }
        case 11:
        {
        const auto Admission = Work.Admit(1024, 64 * sizeof(TCHAR));
        if (Admission != EMappingStep::Complete) return Admission;
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
        break;
        }
        case 12:
        Result = Choice.Advance<EEyeShape>(Work, Object, TEXT("eyeShape"), Out.Eyes, {
                { TEXT("classic"), EEyeShape::Classic }, { TEXT("round"), EEyeShape::Round },
                { TEXT("almond"), EEyeShape::Almond }, { TEXT("sleepy"), EEyeShape::Sleepy }
            }, false, Path, IssueOutput, Eyes);
        break;
        case 13:
        Result = Choice.Advance<EBrowStyle>(Work, Object, TEXT("browStyle"), Out.Brows, {
                { TEXT("classic"), EBrowStyle::Classic }, { TEXT("soft"), EBrowStyle::Soft },
                { TEXT("bold"), EBrowStyle::Bold }, { TEXT("curious"), EBrowStyle::Curious }
            }, false, Path, IssueOutput, Brows);
        break;
        case 14:
        Result = Choice.Advance<ENoseShape>(Work, Object, TEXT("noseShape"), Out.Nose, {
                { TEXT("classic"), ENoseShape::Classic }, { TEXT("button"), ENoseShape::Button },
                { TEXT("wide"), ENoseShape::Wide }
            }, false, Path, IssueOutput, Nose);
        break;
        case 15:
        Result = Choice.Advance<EMouthShape>(Work, Object, TEXT("mouthShape"), Out.Mouth, {
                { TEXT("classic"), EMouthShape::Classic }, { TEXT("small"), EMouthShape::Small },
                { TEXT("wide"), EMouthShape::Wide }
            }, false, Path, IssueOutput, EMouthShape::Classic);
        break;
        case 16:
        Result = Choice.Advance<ECheekStyle>(Work, Object, TEXT("cheekStyle"), Out.Cheeks, {
                { TEXT("none"), ECheekStyle::None }, { TEXT("blush"), ECheekStyle::Blush },
                { TEXT("freckles"), ECheekStyle::Freckles }
            }, false, Path, IssueOutput, ECheekStyle::None);
        break;
        case 17:
        Result = Choice.Advance<EMarkingStyle>(Work, Object, TEXT("markingStyle"), Out.Markings, {
                { TEXT("none"), EMarkingStyle::None }, { TEXT("muzzle"), EMarkingStyle::Muzzle },
                { TEXT("mask"), EMarkingStyle::Mask }, { TEXT("stripes"), EMarkingStyle::Stripes }
            }, false, Path, IssueOutput, Markings);
        break;
        case 18:
        Result = Choice.Advance<EEyewear>(Work, Object, TEXT("eyewear"), Out.Eyewear, {
                { TEXT("none"), EEyewear::None }, { TEXT("round"), EEyewear::Round }
            }, false, Path, IssueOutput, EEyewear::None);
        break;
        case 19:
        {
        const auto Admission = Work.Admit(128);
        if (Admission != EMappingStep::Complete) return Admission;
        Out.bCompleteKnownSelections = Out.Base.Effective.IsSet() && Out.Look.Effective.IsSet()
            && Out.Garment.Effective.IsSet() && (!Out.Fabric.Saved.IsSet() || Out.Fabric.Effective.IsSet())
            && Out.Pattern.Effective.IsSet() && Out.Eyes.Effective.IsSet() && Out.Brows.Effective.IsSet()
            && Out.Nose.Effective.IsSet() && Out.Mouth.Effective.IsSet() && Out.Cheeks.Effective.IsSet()
            && Out.Markings.Effective.IsSet() && Out.Eyewear.Effective.IsSet();
        break;
        }
        case 20:
        {
        if (Index >= 11) break;
        const auto Admission = Work.Admit(1024, (MaterialOutput.Num() + 1) * sizeof(FCharacterMaterialRequirement) + 128 * sizeof(TCHAR));
        if (Admission != EMappingStep::Complete) return Admission;
        const auto Material = [&MaterialOutput](const TCHAR* Name, const TCHAR* Field,
            const TOptional<FString>& Hex, EColorRule Rule = EColorRule::ExplicitOverride)
        {
            MaterialOutput.Add({ Name, Field, Hex.IsSet() ? Rule : EColorRule::PreserveAuthored, Hex });
        };
        const auto BodyRule = Out.BodyColor.IsSet() ? EColorRule::ExplicitOverride : EColorRule::AutomaticBase;
        if (Index == 0) Material(TEXT("Peach cream fur"), TEXT("bodyColor"), Out.EffectiveBodyColor, BodyRule);
        if (Index == 1) Material(TEXT("Cream guard hairs"), TEXT("bodyColor"), Out.EffectiveBodyColor, BodyRule);
        if (Index == 2 || Index == 3)
        {
        const auto TopRule = Out.TopColor.IsSet() ? EColorRule::ExplicitOverride : EColorRule::LookDefault;
        const TCHAR* Name = Index == 2 ? TEXT("Jacket fabric") : TEXT("Jacket ribbing");
        if (Out.TopColor.IsSet() || !Out.Look.Effective.IsSet())
            Material(Name, TEXT("topColor"), Out.TopColor, TopRule);
        else
        {
            const bool bIvory = Out.Look.Effective.GetValue() == ELook::Ivory;
            MaterialOutput.Add({ Name, TEXT("topColor"), TopRule,
                FString(Index == 2 ? (bIvory ? TEXT("#e7dfcc") : TEXT("#b97e74"))
                    : (bIvory ? TEXT("#c6bca5") : TEXT("#95685f"))) });
        }
        }
        if (Index == 4) Material(TEXT("Charcoal twill"), TEXT("trousersColor"), Out.TrousersColor);
        if (Index == 5) Material(TEXT("Twill seams"), TEXT("trousersColor"), Out.TrousersColor);
        if (Index == 6) Material(TEXT("Ivory leather"), TEXT("footwearColor"), Out.FootwearColor);
        if (Index == 7) Material(TEXT("Rose leather"), TEXT("footwearColor"), Out.FootwearColor);
        if (Index == 8) Material(TEXT("Shoe sole"), TEXT("footwearColor"), Out.FootwearColor);
        if (Index == 9) Material(TEXT("Amber iris"), TEXT("eyeColor"), Out.EyeColor, EColorRule::IrisLuminanceTint);
        if (Index == 10) Material(TEXT("Peach nose"), TEXT("noseColor"), Out.EffectiveNoseColor,
            Out.NoseColor.IsSet() ? EColorRule::ExplicitOverride : EColorRule::AutomaticBase);
        ++Index; continue;
        }
        case 21:
        {
        if (Index >= 3) break;
        const auto Admission = Work.Admit(512, (AttachmentOutput.Num() + 1) * sizeof(FAttachmentRequirement) + 96 * sizeof(TCHAR));
        if (Admission != EMappingStep::Complete) return Admission;
        // Ear velvet is handled by faceRig's marking recipe, not the body override.
        if (Index == 0) AttachmentOutput.Add({ TEXT("AccessoryNecklace"), TEXT("rig.accessoryVisibility"), Out.EffectiveNecklaceVisible });
        if (Index == 1) AttachmentOutput.Add({ TEXT("AccessoryChain"), TEXT("rig.accessoryVisibility"), Out.EffectiveHipChainVisible });
        TOptional<bool> Glasses;
        if (Out.Eyewear.Effective.IsSet()) Glasses = Out.Eyewear.Effective.GetValue() == EEyewear::Round;
        if (Index == 2) AttachmentOutput.Add({ TEXT("Head"), TEXT("faceRig.BestieGlasses"), Glasses });
        ++Index; continue;
        }
        case 22:
        {
        // All source binding identifiers are requirements, not confirmed imported nodes or UE sockets.
        static const TCHAR* Nodes[] = { TEXT("Chest"), TEXT("Hips"), TEXT("DeformHips"),
            TEXT("DeformLegL"), TEXT("DeformLegR"), TEXT("ShoulderL"), TEXT("ShoulderR"),
            TEXT("ElbowL"), TEXT("ElbowR"), TEXT("HandL"), TEXT("HandR"),
            TEXT("DeformShoulderL"), TEXT("DeformShoulderR"), TEXT("DeformElbowL"), TEXT("DeformElbowR"),
            TEXT("LegL"), TEXT("LegR"), TEXT("KneeL"), TEXT("KneeR") };
        if (Index < UE_ARRAY_COUNT(Nodes))
        {
            const auto Admission = Work.Admit(512, (AttachmentOutput.Num() + 1) * sizeof(FAttachmentRequirement) + 96 * sizeof(TCHAR));
            if (Admission != EMappingStep::Complete) return Admission;
            AttachmentOutput.Add({ Nodes[Index++], TEXT("wardrobeRig.BestieWardrobe"), {} }); continue;
        }
        break;
        }
        case 23:
        {
            if (Index >= 5) break;
            static const EHomeIssue Codes[] = { EHomeIssue::BaseRecipeUnported, EHomeIssue::FaceRecipeUnported,
                EHomeIssue::WardrobeRecipeUnported, EHomeIssue::MaterialRecipeUnported, EHomeIssue::AttachmentUnverified };
            static const TCHAR* Fields[] = { TEXT(".baseId"), TEXT(".face"), TEXT(".wardrobe"), TEXT(".materials"), TEXT(".attachments") };
            const auto Admission = Work.Admit(512, (IssueOutput.Num() + 1) * sizeof(FHomeMappingIssue) + 96 * sizeof(TCHAR));
            if (Admission != EMappingStep::Complete) return Admission;
            Issue(IssueOutput, Codes[Index], Path + Fields[Index]); ++Index; continue;
        }
        default: Phase = 29; return EMappingStep::Complete;
        }
        if (Result != EMappingStep::Complete) return Result;
        ++Phase; Index = 0; Choice.Begin();
        }
        return EMappingStep::Complete;
    }
    EMappingStep FAppearanceMapping::Retire(FMappingWork& Work)
    {
        const auto Result = Work.ReleaseText(Path);
        if (Result != EMappingStep::Complete) return Result;
        // The legacy adapter owns decoded JSON; world mapping uses only borrowed
        // known fields. Its segmented source reference never owns a root.
        const auto Admission = Work.Admit(128);
        if (Admission != EMappingStep::Complete) return Admission;
        Object = {}; Identity = nullptr; Output = nullptr;
        Materials = nullptr; Attachments = nullptr; Issues = nullptr;
        return EMappingStep::Complete;
    }
    bool ReadAppearance(const FCharacterSelection& Input, FAppearancePlan& Out,
        TArray<FCharacterMaterialRequirement>& Materials, TArray<FAttachmentRequirement>& Attachments,
        TArray<FHomeMappingIssue>& Issues)
    {
        FObject Object;
        if (Input.Id.Len() > 80 || Input.Id.TrimStartAndEnd().IsEmpty()
            || Input.Name.Len() > 20 || Input.Name.TrimStartAndEnd().IsEmpty()
            || !ReadObject(Input.AppearanceJson, Object)) return false;
        FLegacyMappingWork Work;
        FAppearanceMapping Mapping;
        Mapping.Begin(Input, FMappingObject(Object), Out, Materials, Attachments, Issues);
        const bool bComplete = Mapping.Advance(Work) == EMappingStep::Complete;
        Mapping.Retire(Work);
        return bComplete;
    }
    bool ReadAppearanceKnown(const FCharacterSelection& Input, const FKnownObject& Object,
        FAppearancePlan& Out, TArray<FCharacterMaterialRequirement>& Materials,
        TArray<FAttachmentRequirement>& Attachments, TArray<FHomeMappingIssue>& Issues)
    {
        FLegacyMappingWork Work;
        FAppearanceMapping Mapping;
        Mapping.Begin(Input, FMappingObject(Object), Out, Materials, Attachments, Issues);
        const bool bComplete = Mapping.Advance(Work) == EMappingStep::Complete;
        Mapping.Retire(Work);
        return bComplete;
    }
}
