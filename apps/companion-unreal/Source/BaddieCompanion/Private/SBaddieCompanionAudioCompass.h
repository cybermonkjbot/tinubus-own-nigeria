#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SLeafWidget.h"

/** Presentation only. The mission/audio owner admits audible or equivalent accessible events.
 * Use a strictly increasing nonzero generation for each presentation context. Clear on access/encounter loss,
 * view changes, UI suppression or disabling cues; reject old callbacks before forwarding them.
 * No actor lookup, hearing simulation, enemy identity, gameplay input or account authority.
 */
class SBaddieCompanionAudioCompass : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SBaddieCompanionAudioCompass) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& Args);
    // UE world yaw in degrees. Zero or an invalid current heading clears; retired generations are ignored.
    void PresentView(uint64 Generation, double ListenerYawDegrees);
    // Once per accepted sound event, on the game/UI thread. Remaining lifetime comes from its owner,
    // never from actor proximity or a per-frame scan. Bearing is quantized and then discarded.
    bool AddHeardCue(uint64 Generation, double WorldBearingDegrees, double RemainingSeconds);
    void Clear();

protected:
    virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geometry,
        const FSlateRect& CullingRect, FSlateWindowElementList& Elements, int32 LayerId,
        const FWidgetStyle& Style, bool bParentEnabled) const override;

private:
    static constexpr int32 SectorCount = 8;
    static constexpr double SectorDegrees = 360.0 / SectorCount;
    static constexpr double MaximumLifetimeSeconds = 1.5;

    uint64 PresentationGeneration = 0;
    uint64 LastGeneration = 0;
    double ListenerYaw = 0;
    double ExpiresAt[SectorCount] = {};
};
