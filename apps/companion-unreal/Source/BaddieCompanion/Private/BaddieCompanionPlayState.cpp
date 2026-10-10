#include "BaddieCompanionPlayState.h"

#include "BaddieCompanionStateJson.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace BaddieCompanion
{
    namespace
    {
        using namespace StateJsonPrivate;
        constexpr FParseLimits PlayLimits{524288, 16, 16384};
        constexpr FParseLimits AppearanceLimits{32768, 8, 1024};
        constexpr int32 MaxText = 8192;
        constexpr int32 MaxSavedGameArray = 256;
        enum class EKind : uint8 { Playground, Mirror, Mystery, Style, Kitchen, Memories };

        bool Text(const FValue& Value, FString& Out)
        {
            return String(Value, Out) && Out.Len() <= MaxText;
        }
        bool Scalar(const FValue& Value, double& Out) { return Number(Value, Out); }
        bool Emote(const FValue& Value, EPlayEmote& Out)
        {
            return EnumValue(Value, Out, {{TEXT("greeting"), EPlayEmote::Greeting},
                {TEXT("nod"), EPlayEmote::Nod}, {TEXT("cheer"), EPlayEmote::Cheer},
                {TEXT("stretch"), EPlayEmote::Stretch}, {TEXT("groove"), EPlayEmote::Groove},
                {TEXT("sway"), EPlayEmote::Sway}});
        }
        template<typename T, typename F>
        bool Rows(const FValue& Value, TArray<T>& Out, int32 Max, F Read)
        {
            if (!Value.IsValid() || Value->Type != EJson::Array) return false;
            const auto& Values = Value->AsArray();
            if (Values.Num() > Max) return false;
            for (const auto& Item : Values)
            {
                T Next;
                if (!Read(Item, Next)) return false;
                Out.Add(MoveTemp(Next));
            }
            return true;
        }
        bool Look(const FValue& Value, FPlayLook& Out)
        {
            const auto O = Object(Value);
            return O.IsValid() && EnumValue(Field(O, TEXT("look")), Out.Look,
                {{TEXT("rose"), EPlayLook::Rose}, {TEXT("ivory"), EPlayLook::Ivory}})
                && Text(Field(O, TEXT("topColor")), Out.TopColor)
                && Text(Field(O, TEXT("trousersColor")), Out.TrousersColor)
                && Text(Field(O, TEXT("footwearColor")), Out.FootwearColor);
        }
        bool Player(const FValue& Value, FPlayPlayer& Out)
        {
            const auto O = Object(Value);
            if (!O.IsValid() || !Text(Field(O, TEXT("id")), Out.Id)
                || !Text(Field(O, TEXT("name")), Out.Name)
                || !Text(Field(O, TEXT("characterId")), Out.CharacterId)
                || !Number(O, TEXT("progress"), Out.Progress) || !Number(O, TEXT("score"), Out.Score)
                || !Nullable(Field(O, TEXT("look")), Out.Look, Look)
                || !Nullable(Field(O, TEXT("vote")), Out.Vote, Text)
                || !Text(Field(O, TEXT("lastActionId")), Out.LastActionId)
                || !Number(O, TEXT("lastActionAt"), Out.LastActionAt)) return false;
            const auto Watching = Field(O, TEXT("mirrorWatchingAt"));
            if (Watching.IsValid())
            {
                double At = 0;
                if (!Number(Watching, At)) return false;
                Out.MirrorWatchingAt = At;
            }
            const auto Appearance = Object(Field(O, TEXT("appearance")));
            // Preserve the complete object and absence. No appearance validator/defaults copied.
            return Appearance.IsValid()
                && FJsonSerializer::Serialize(Appearance.ToSharedRef(), TJsonWriterFactory<>::Create(&Out.AppearanceJson))
                && WithinParseLimits(Out.AppearanceJson, AppearanceLimits);
        }
        bool Flight(const FValue& Value, FPlayFlight& Out)
        {
            const auto O = Object(Value);
            return O.IsValid() && Number(O, TEXT("from"), Out.From) && Number(O, TEXT("to"), Out.To)
                && Number(O, TEXT("power"), Out.Power) && Number(O, TEXT("at"), Out.At)
                && Boolean(Field(O, TEXT("hit")), Out.bHit);
        }
        bool LastTouch(const FValue& Value, FPlayLastTouch& Out)
        {
            const auto O = Object(Value);
            return O.IsValid() && Number(O, TEXT("target"), Out.Target) && Number(O, TEXT("at"), Out.At)
                && Text(Field(O, TEXT("playerId")), Out.PlayerId);
        }
        bool LastMove(const FValue& Value, FPlayLastMove& Out)
        {
            const auto O = Object(Value);
            return O.IsValid() && Text(Field(O, TEXT("playerId")), Out.PlayerId)
                && Emote(Field(O, TEXT("emote")), Out.Emote) && Number(O, TEXT("at"), Out.At);
        }
        bool Game(const FValue& Value, FPlayGame& Out)
        {
            const auto O = Object(Value);
            EKind Kind;
            if (!O.IsValid() || !EnumValue(Field(O, TEXT("kind")), Kind,
                {{TEXT("playground"), EKind::Playground}, {TEXT("mirror"), EKind::Mirror},
                {TEXT("mystery"), EKind::Mystery}, {TEXT("style"), EKind::Style},
                {TEXT("kitchen"), EKind::Kitchen}, {TEXT("memories"), EKind::Memories}})) return false;
            switch (Kind)
            {
            case EKind::Playground:
            {
                FPlayPlaygroundGame Next;
                if (!EnumValue(Field(O, TEXT("toy")), Next.Toy,
                    {{TEXT("ball"), EPlayToy::Ball}, {TEXT("cushion"), EPlayToy::Cushion},
                    {TEXT("music"), EPlayToy::Music}, {TEXT("bubbles"), EPlayToy::Bubbles}})
                    || !Number(O, TEXT("hits"), Next.Hits) || !Number(O, TEXT("throws"), Next.Throws)
                    || !Rows(Field(O, TEXT("notes")), Next.Notes, MaxSavedGameArray, Scalar)
                    || !Nullable(Field(O, TEXT("flight")), Next.Flight, Flight)) return false;
                const auto Touch = Field(O, TEXT("lastTouch"));
                if (Touch.IsValid())
                {
                    FPlayLastTouch Item;
                    if (!LastTouch(Touch, Item)) return false;
                    Next.LastTouch = MoveTemp(Item);
                }
                Out = MoveTemp(Next);
                return true;
            }
            case EKind::Mirror:
            {
                FPlayMirrorGame Next;
                if (!Rows(Field(O, TEXT("sequence")), Next.Sequence, MaxSavedGameArray, Emote)
                    || !Number(O, TEXT("showingAt"), Next.ShowingAt) || !Number(O, TEXT("level"), Next.Level)
                    || !Nullable(Field(O, TEXT("leader")), Next.Leader, Text)
                    || !Nullable(Field(O, TEXT("lastMove")), Next.LastMove, LastMove)) return false;
                Out = MoveTemp(Next);
                return true;
            }
            case EKind::Mystery:
            {
                FPlayMysteryGame Next;
                if (!Rows(Field(O, TEXT("order")), Next.Order, MaxSavedGameArray, Scalar)
                    || !Rows(Field(O, TEXT("found")), Next.Found, MaxSavedGameArray, Scalar)
                    || !Rows(Field(O, TEXT("entered")), Next.Entered, MaxSavedGameArray, Scalar)
                    || !Boolean(Field(O, TEXT("unlocked")), Next.bUnlocked)) return false;
                Out = MoveTemp(Next);
                return true;
            }
            case EKind::Style:
            {
                FPlayStyleGame Next;
                if (!Number(O, TEXT("brief"), Next.Brief)
                    || !EnumValue(Field(O, TEXT("phase")), Next.Phase,
                        {{TEXT("dress"), EPlayStylePhase::Dress}, {TEXT("reveal"), EPlayStylePhase::Reveal}})
                    || !Nullable(Field(O, TEXT("revealedAt")), Next.RevealedAt, Scalar)) return false;
                Out = MoveTemp(Next);
                return true;
            }
            case EKind::Kitchen:
            {
                FPlayKitchenGame Next;
                if (!Rows(Field(O, TEXT("recipe")), Next.Recipe, MaxSavedGameArray, Scalar)
                    || !Rows(Field(O, TEXT("ingredients")), Next.Ingredients, MaxSavedGameArray, Scalar)
                    || !Boolean(Field(O, TEXT("chopped")), Next.bChopped)
                    || !Nullable(Field(O, TEXT("heatAt")), Next.HeatAt, Scalar)
                    || !Boolean(Field(O, TEXT("cooked")), Next.bCooked)
                    || !Boolean(Field(O, TEXT("plated")), Next.bPlated)
                    || !Number(O, TEXT("served"), Next.Served)
                    || !Boolean(Field(O, TEXT("rush")), Next.bRush)) return false;
                Out = MoveTemp(Next);
                return true;
            }
            case EKind::Memories:
            {
                FPlayMemoriesGame Next;
                if (!Number(O, TEXT("anchor"), Next.Anchor)
                    || !Rows(Field(O, TEXT("moves")), Next.Moves, MaxSavedGameArray, Emote)
                    || !Boolean(Field(O, TEXT("recording")), Next.bRecording)) return false;
                Out = MoveTemp(Next);
                return true;
            }
            }
            return false;
        }
        bool Round(const FValue& Value, FPlayRound& Out)
        {
            const auto O = Object(Value);
            return O.IsValid() && Text(Field(O, TEXT("id")), Out.Id)
                && Text(Field(O, TEXT("ownerId")), Out.OwnerId)
                && Number(O, TEXT("revision"), Out.Revision) && Number(O, TEXT("seed"), Out.Seed)
                && Number(O, TEXT("startedAt"), Out.StartedAt) && Number(O, TEXT("updatedAt"), Out.UpdatedAt)
                && Number(O, TEXT("expiresAt"), Out.ExpiresAt)
                && Rows(Field(O, TEXT("players")), Out.Players, 8, Player)
                && Game(Field(O, TEXT("game")), Out.Game);
        }
        bool Memory(const FValue& Value, FPlayMemory& Out)
        {
            const auto O = Object(Value);
            int64 Anchor = 0;
            if (!O.IsValid() || !String(Field(O, TEXT("id")), Out.Id) || Out.Id.Len() > 100
                || !String(Field(O, TEXT("author")), Out.Author) || Out.Author.Len() > 100
                || !Integer(Field(O, TEXT("anchor")), Anchor) || Anchor > 5
                || !Rows(Field(O, TEXT("moves")), Out.Moves, 8, Emote) || Out.Moves.IsEmpty()
                || !Number(O, TEXT("createdAt"), Out.CreatedAt)) return false;
            Out.Anchor = static_cast<int32>(Anchor);
            return true;
        }
    }

    TOptional<FPlayRound> FPlayWire::GetActiveRound() const
    {
        if (!Round.IsSet() || Round->Players.IsEmpty()
            || std::holds_alternative<FPlayPlaygroundGame>(Round->Game)) return {};
        return Round;
    }

    FPlayWireResult DecodePlayWire(const FString& Json)
    {
        FPlayWireResult Result;
        FPlayWire Next;
        StateJsonPrivate::FObject O;
        if (!StateJsonPrivate::Parse(Json, O, PlayLimits)
            || !StateJsonPrivate::Nullable(StateJsonPrivate::Field(O, TEXT("round")), Next.Round, Round)
            || !Text(StateJsonPrivate::Field(O, TEXT("playerId")), Next.PlayerId)
            || !Rows(StateJsonPrivate::Field(O, TEXT("memories")), Next.Memories, 12, Memory))
        {
            Result.Error = TEXT("Room activity couldn't load.");
            return Result;
        }
        Result.Disposition = EPlayWireDisposition::Accepted;
        Result.Wire = MoveTemp(Next);
        return Result;
    }

    FPlayWireResult ReconcilePlayWire(const TOptional<FPlayWire>& Previous, const FString& Json)
    {
        FPlayWireResult Result = DecodePlayWire(Json);
        if (!Result.Wire.IsSet()) return Result;
        const auto& Incoming = Result.Wire.GetValue();
        if (Previous.IsSet() && Previous->Round.IsSet() && Incoming.Round.IsSet()
            && StateJsonPrivate::Same(Previous->Round->Id, Incoming.Round->Id)
            && Incoming.Round->Revision < Previous->Round->Revision)
        {
            Result.Disposition = EPlayWireDisposition::Retained;
            Result.Wire = Previous; // Includes playerId and memories, never a partial merge.
        }
        return Result;
    }
}
