#include "SBaddieCompanionAudioCompass.h"

#include "HAL/PlatformTime.h"
#include "Rendering/DrawElementTypes.h"
#include "Styling/WidgetStyle.h"

void SBaddieCompanionAudioCompass::Construct(const FArguments& Args)
{
    // Native Slate paint handles fading. No game ticker, timer, input capture or audio owner.
    ForceVolatile(true);
    Clear();
}

void SBaddieCompanionAudioCompass::Clear()
{
    PresentationGeneration = 0;
    ListenerYaw = 0;
    for (double& Expiry : ExpiresAt) Expiry = 0;
    SetVisibility(EVisibility::Collapsed);
}

void SBaddieCompanionAudioCompass::PresentView(uint64 Generation, double ListenerYawDegrees)
{
    if (Generation != 0 && (Generation < LastGeneration
        || (PresentationGeneration == 0 && Generation == LastGeneration))) return;
    if (Generation == 0 || !FMath::IsFinite(ListenerYawDegrees))
    {
        Clear();
        return;
    }
    if (Generation != PresentationGeneration) Clear();
    PresentationGeneration = Generation;
    LastGeneration = Generation;
    ListenerYaw = FMath::Fmod(ListenerYawDegrees, 360.0);
    SetVisibility(EVisibility::HitTestInvisible);
}

bool SBaddieCompanionAudioCompass::AddHeardCue(uint64 Generation, double WorldBearingDegrees,
    double RemainingSeconds)
{
    if (Generation == 0 || Generation != PresentationGeneration
        || !FMath::IsFinite(WorldBearingDegrees) || !FMath::IsFinite(RemainingSeconds)
        || RemainingSeconds <= 0) return false;

    const double Bearing = FMath::Fmod(WorldBearingDegrees, 360.0);
    const int32 Sector = (FMath::RoundToInt(Bearing / SectorDegrees) + SectorCount) % SectorCount;
    const double Expiry = FPlatformTime::Seconds() + FMath::Min(RemainingSeconds, MaximumLifetimeSeconds);
    ExpiresAt[Sector] = FMath::Max(ExpiresAt[Sector], Expiry);
    return true;
}

FVector2D SBaddieCompanionAudioCompass::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
    // Slate logical units; the HUD centers this around its own crosshair without covering it.
    return FVector2D(128, 128);
}

int32 SBaddieCompanionAudioCompass::OnPaint(const FPaintArgs& Args, const FGeometry& Geometry,
    const FSlateRect& CullingRect, FSlateWindowElementList& Elements, int32 LayerId,
    const FWidgetStyle& Style, bool bParentEnabled) const
{
    if (PresentationGeneration == 0 || !bParentEnabled) return LayerId;
    const FVector2D Size = Geometry.GetLocalSize();
    if (Size.ContainsNaN() || Size.X < 16 || Size.Y < 16) return LayerId;
    const double Radius = FMath::Min(42.0, FMath::Min(Size.X, Size.Y) * 0.5 - 8.0);
    const FVector2D Centre = Size * 0.5;
    const double Now = FPlatformTime::Seconds();
    const auto PaintGeometry = Geometry.ToPaintGeometry();
    bool bDrewCue = false;

    for (int32 Sector = 0; Sector < SectorCount; ++Sector)
    {
        const double Remaining = ExpiresAt[Sector] - Now;
        if (Remaining <= 0) continue;
        const float Fade = static_cast<float>(FMath::Clamp(Remaining / MaximumLifetimeSeconds, 0.0, 1.0));
        const float Opacity = Fade * Fade * (3.0f - 2.0f * Fade);
        const double RelativeBearing = Sector * SectorDegrees - ListenerYaw;
        TArray<FVector2f> Arc;
        Arc.Reserve(5);
        for (int32 Point = 0; Point < 5; ++Point)
        {
            const double Angle = FMath::DegreesToRadians(RelativeBearing - 10.0 + Point * 5.0);
            Arc.Emplace(static_cast<float>(Centre.X + FMath::Sin(Angle) * Radius),
                static_cast<float>(Centre.Y - FMath::Cos(Angle) * Radius));
        }
        // Same neutral mark for every permitted sound. Alpha indicates age, not enemy distance.
        FLinearColor Core = Style.GetColorAndOpacityTint();
        FLinearColor Halo(0.02f, 0.025f, 0.02f, Core.A);
        Halo.A *= Opacity * 0.32f;
        Core.A *= Opacity * 0.72f;
        FSlateDrawElement::MakeLines(Elements, LayerId, PaintGeometry, Arc,
            ESlateDrawEffect::None, Halo, true, 6.0f);
        FSlateDrawElement::MakeLines(Elements, LayerId + 1, PaintGeometry, MoveTemp(Arc),
            ESlateDrawEffect::None, Core, true, 2.0f);
        bDrewCue = true;
    }
    return bDrewCue ? LayerId + 1 : LayerId;
}
