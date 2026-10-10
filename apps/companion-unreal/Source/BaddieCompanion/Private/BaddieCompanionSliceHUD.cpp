#include "BaddieCompanionSliceHUD.h"

#include "BaddieCompanionSliceSession.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"

void ABaddieCompanionSliceHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas || !GEngine || !GetGameInstance()) return;
    const auto* Session = GetGameInstance()->GetSubsystem<UBaddieCompanionSliceSession>();
    if (!Session) return;

    // Wrap measured text instead of assuming a desktop viewport width.
    const float Scale = FMath::Clamp(Canvas->ClipX / 1000.0f, 0.8f, 1.25f);
    const float Margin = 20 * Scale;
    const float Width = FMath::Max(1.0f, Canvas->ClipX - Margin * 2);
    const float LineHeight = 22 * Scale;
    float Y = Margin;
    UFont* Font = GEngine->GetSmallFont();
    const auto DrawLine = [this, Font, Scale, Margin, Width, LineHeight, &Y](const FString& Text)
    {
        FString Line;
        const auto Flush = [&]()
        {
            DrawRect(FLinearColor(0.02f, 0.02f, 0.03f, 0.88f), Margin - 6, Y - 3, Width + 12, LineHeight);
            DrawText(Line, FLinearColor::White, Margin, Y, Font, Scale);
            Y += LineHeight;
            Line.Reset();
        };
        for (const TCHAR Character : Text)
        {
            const FString Candidate = Line + FString::Chr(Character);
            float TextWidth = 0, TextHeight = 0;
            GetTextSize(Candidate, TextWidth, TextHeight, Font, Scale);
            if (!Line.IsEmpty() && TextWidth > Width) Flush();
            Line.AppendChar(Character);
        }
        if (!Line.IsEmpty()) Flush();
    };

    DrawLine(TEXT("Offline Home preview"));
    if (const auto* Selection = Session->GetSelection())
    {
        DrawLine(FString::Printf(TEXT("Companion: %s | Home: %s"),
            *Selection->Character.Name, *Selection->HomeRoom.Name));
        DrawLine(TEXT("Approved appearance and saved Home art are not mapped. Local movement only."));
        DrawLine(TEXT("WASD / left stick: move | Mouse / right stick: camera | Wheel: zoom | R: reset | Esc / Start: pointer"));
    }
    else if (!Session->GetStatus().IsEmpty())
    {
        DrawLine(Session->GetStatus());
    }
    else
    {
        DrawLine(TEXT("Open with -CompanionOfflinePreview and -CompanionOfflineSnapshot=<local JSON file>."));
    }
}
