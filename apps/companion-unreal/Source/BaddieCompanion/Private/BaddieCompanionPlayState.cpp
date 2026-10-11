#include "BaddieCompanionPlayState.h"

#include "BaddieCompanionStateJson.h"
#include "Misc/DateTime.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include <cmath>
#include <type_traits>

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
                    || !ReadBoundedArray(Field(O, TEXT("notes")), Next.Notes, MaxSavedGameArray, Scalar)
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
                if (!ReadBoundedArray(Field(O, TEXT("sequence")), Next.Sequence, MaxSavedGameArray, Emote)
                    || !Number(O, TEXT("showingAt"), Next.ShowingAt) || !Number(O, TEXT("level"), Next.Level)
                    || !Nullable(Field(O, TEXT("leader")), Next.Leader, Text)
                    || !Nullable(Field(O, TEXT("lastMove")), Next.LastMove, LastMove)) return false;
                Out = MoveTemp(Next);
                return true;
            }
            case EKind::Mystery:
            {
                FPlayMysteryGame Next;
                if (!ReadBoundedArray(Field(O, TEXT("order")), Next.Order, MaxSavedGameArray, Scalar)
                    || !ReadBoundedArray(Field(O, TEXT("found")), Next.Found, MaxSavedGameArray, Scalar)
                    || !ReadBoundedArray(Field(O, TEXT("entered")), Next.Entered, MaxSavedGameArray, Scalar)
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
                if (!ReadBoundedArray(Field(O, TEXT("recipe")), Next.Recipe, MaxSavedGameArray, Scalar)
                    || !ReadBoundedArray(Field(O, TEXT("ingredients")), Next.Ingredients, MaxSavedGameArray, Scalar)
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
                    || !ReadBoundedArray(Field(O, TEXT("moves")), Next.Moves, MaxSavedGameArray, Emote)
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
                && ReadBoundedArray(Field(O, TEXT("players")), Out.Players, 8, Player)
                && Game(Field(O, TEXT("game")), Out.Game);
        }
        bool Memory(const FValue& Value, FPlayMemory& Out)
        {
            const auto O = Object(Value);
            int64 Anchor = 0;
            if (!O.IsValid() || !String(Field(O, TEXT("id")), Out.Id) || Out.Id.Len() > 100
                || !String(Field(O, TEXT("author")), Out.Author) || Out.Author.Len() > 100
                || !Integer(Field(O, TEXT("anchor")), Anchor) || Anchor > 5
                || !ReadBoundedArray(Field(O, TEXT("moves")), Out.Moves, 8, Emote) || Out.Moves.IsEmpty()
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
            || !StateJsonPrivate::ReadBoundedArray(StateJsonPrivate::Field(O, TEXT("memories")), Next.Memories, 12, Memory))
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

    namespace PlayCommandPrivate
    {
        using namespace StateJsonPrivate;
        constexpr const TCHAR* QueryError = TEXT("Room activity couldn't load.");
        constexpr const TCHAR* CommandError = TEXT("That move couldn't finish. Try again.");
        constexpr const TCHAR* UnavailableError = TEXT("This activity is no longer available.");

        double WallTimeMs()
        {
            const FDateTime Now = FDateTime::UtcNow();
            return static_cast<double>(Now.ToUnixTimestamp()) * 1000 + Now.GetMillisecond();
        }
        bool SafeRevision(double Value)
        {
            return std::isfinite(Value) && Value >= 0 && Value <= MaxInteger && std::floor(Value) == Value;
        }
        bool PlayIndex(double Value)
        {
            return std::isfinite(Value) && Value >= 0 && Value <= 5 && std::floor(Value) == Value;
        }
        FString PublicError(const TOptional<FString>& Message, const TCHAR* Fallback)
        {
            return Message.IsSet() && Message->Len() <= MaxText ? *Message : FString(Fallback);
        }
        bool SameOptionalText(const TOptional<FString>& A, const TOptional<FString>& B)
        {
            return A.IsSet() == B.IsSet() && (!A.IsSet() || Same(*A, *B));
        }
        bool SameScope(const TOptional<FPlayScope>& A, const TOptional<FPlayScope>& B)
        {
            if (A.IsSet() != B.IsSet()) return false;
            if (!A.IsSet()) return true;
            return Same(A->CharacterId, B->CharacterId) && Same(A->RoomId, B->RoomId)
                && SameOptionalText(A->CallSessionId, B->CallSessionId)
                && SameOptionalText(A->InvitationId, B->InvitationId)
                && A->RoomRevision.IsSet() == B->RoomRevision.IsSet()
                && (!A->RoomRevision.IsSet() || *A->RoomRevision == *B->RoomRevision
                    || (std::isnan(*A->RoomRevision) && std::isnan(*B->RoomRevision)));
        }
        FObject ScopeArguments(const FPlayScope& Scope)
        {
            // Resource/JSON checks only. Access, ID validity and room authority stay remote.
            if (Scope.CharacterId.Len() > MaxText || Scope.RoomId.Len() > MaxText
                || (Scope.CallSessionId.IsSet() && Scope.CallSessionId->Len() > MaxText)
                || (Scope.InvitationId.IsSet() && Scope.InvitationId->Len() > MaxText)
                || (Scope.RoomRevision.IsSet() && !std::isfinite(*Scope.RoomRevision))) return {};
            const auto O = MakeShared<FJsonObject>();
            O->SetStringField(TEXT("characterId"), Scope.CharacterId);
            O->SetStringField(TEXT("roomId"), Scope.RoomId);
            if (Scope.CallSessionId.IsSet()) O->SetStringField(TEXT("callSessionId"), *Scope.CallSessionId);
            if (Scope.InvitationId.IsSet()) O->SetStringField(TEXT("invitationId"), *Scope.InvitationId);
            if (Scope.RoomRevision.IsSet()) O->SetNumberField(TEXT("roomRevision"), *Scope.RoomRevision);
            return O;
        }
        bool SerializeArguments(const FObject& O, FString& Out)
        {
            return O.IsValid() && FJsonSerializer::Serialize(O.ToSharedRef(),
                TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Out))
                && WithinParseLimits(Out, PlayLimits);
        }
        const TCHAR* StartKind(EPlayStartKind Kind)
        {
            switch (Kind)
            {
            case EPlayStartKind::Mirror: return TEXT("mirror");
            case EPlayStartKind::Mystery: return TEXT("mystery");
            case EPlayStartKind::Style: return TEXT("style");
            case EPlayStartKind::Kitchen: return TEXT("kitchen");
            case EPlayStartKind::Memories: return TEXT("memories");
            default: return nullptr;
            }
        }
        const TCHAR* EmoteName(EPlayEmote Value)
        {
            switch (Value)
            {
            case EPlayEmote::Greeting: return TEXT("greeting");
            case EPlayEmote::Nod: return TEXT("nod");
            case EPlayEmote::Cheer: return TEXT("cheer");
            case EPlayEmote::Stretch: return TEXT("stretch");
            case EPlayEmote::Groove: return TEXT("groove");
            case EPlayEmote::Sway: return TEXT("sway");
            default: return nullptr;
            }
        }
        bool PaletteColor(const FString& Value)
        {
            for (const TCHAR* Color : {TEXT("#e7adbb"), TEXT("#ede4d5"), TEXT("#77856b"),
                TEXT("#343340"), TEXT("#b48755"), TEXT("#809bac")})
                if (Same(Value, Color)) return true;
            return false;
        }
        FObject CommandArguments(const FPlayCommand& Command)
        {
            if (Command.valueless_by_exception()) return {};
            const auto O = MakeShared<FJsonObject>();
            const bool bValid = std::visit([&O](const auto& Input)
            {
                using T = std::decay_t<decltype(Input)>;
                const TCHAR* Type = nullptr;
                if constexpr (std::is_same_v<T, FPlayJoinCommand>) Type = TEXT("join");
                else if constexpr (std::is_same_v<T, FPlayLeaveCommand>) Type = TEXT("leave");
                else if constexpr (std::is_same_v<T, FPlayReplayCommand>) Type = TEXT("replay");
                else if constexpr (std::is_same_v<T, FPlayLeadCommand>) Type = TEXT("lead");
                else if constexpr (std::is_same_v<T, FPlayPerformCommand>) Type = TEXT("perform");
                else if constexpr (std::is_same_v<T, FPlayWatchCommand>) Type = TEXT("watch");
                else if constexpr (std::is_same_v<T, FPlayChopCommand>) Type = TEXT("chop");
                else if constexpr (std::is_same_v<T, FPlayHeatCommand>) Type = TEXT("heat");
                else if constexpr (std::is_same_v<T, FPlayPlateCommand>) Type = TEXT("plate");
                else if constexpr (std::is_same_v<T, FPlayServeCommand>) Type = TEXT("serve");
                else if constexpr (std::is_same_v<T, FPlaySaveMemoryCommand>) Type = TEXT("save-memory");
                else if constexpr (std::is_same_v<T, FPlayStartCommand>)
                {
                    const TCHAR* Kind = StartKind(Input.Kind);
                    if (!Kind) return false;
                    Type = TEXT("start"); O->SetStringField(TEXT("kind"), Kind);
                    if (Input.Rush.IsSet()) O->SetBoolField(TEXT("rush"), *Input.Rush);
                }
                else if constexpr (std::is_same_v<T, FPlayTouchCommand>)
                {
                    if (!PlayIndex(Input.Target) || (Input.Power.IsSet()
                        && (!std::isfinite(*Input.Power) || *Input.Power < 0 || *Input.Power > 1))) return false;
                    Type = TEXT("touch"); O->SetNumberField(TEXT("target"), Input.Target);
                    if (Input.Power.IsSet()) O->SetNumberField(TEXT("power"), *Input.Power);
                }
                else if constexpr (std::is_same_v<T, FPlayEmoteCommand>)
                {
                    const TCHAR* Name = EmoteName(Input.Emote);
                    if (!Name) return false;
                    Type = TEXT("emote"); O->SetStringField(TEXT("emote"), Name);
                }
                else if constexpr (std::is_same_v<T, FPlayLookCommand>)
                {
                    const auto& L = Input.Look;
                    if ((L.Look != EPlayLook::Rose && L.Look != EPlayLook::Ivory)
                        || !PaletteColor(L.TopColor) || !PaletteColor(L.TrousersColor)
                        || !PaletteColor(L.FootwearColor)) return false;
                    const auto LookObject = MakeShared<FJsonObject>();
                    LookObject->SetStringField(TEXT("look"), L.Look == EPlayLook::Rose ? TEXT("rose") : TEXT("ivory"));
                    LookObject->SetStringField(TEXT("topColor"), L.TopColor);
                    LookObject->SetStringField(TEXT("trousersColor"), L.TrousersColor);
                    LookObject->SetStringField(TEXT("footwearColor"), L.FootwearColor);
                    Type = TEXT("look"); O->SetObjectField(TEXT("look"), LookObject);
                }
                else if constexpr (std::is_same_v<T, FPlayVoteCommand>)
                {
                    if (Input.PlayerId.Len() > 100) return false;
                    Type = TEXT("vote"); O->SetStringField(TEXT("playerId"), Input.PlayerId);
                }
                else if constexpr (std::is_same_v<T, FPlayIngredientCommand>)
                {
                    if (!PlayIndex(Input.Ingredient)) return false;
                    Type = TEXT("ingredient"); O->SetNumberField(TEXT("ingredient"), Input.Ingredient);
                }
                else if constexpr (std::is_same_v<T, FPlayRemoveMemoryCommand>)
                {
                    if (Input.MemoryId.Len() > 100) return false;
                    Type = TEXT("remove-memory"); O->SetStringField(TEXT("memoryId"), Input.MemoryId);
                }
                else return false;
                O->SetStringField(TEXT("type"), Type);
                return true;
            }, Command);
            return bValid ? FObject(O) : FObject();
        }
    }

    void FPlayState::Emit()
    {
        // Keep a callable alive across removal and visit newly registered listeners in order.
        // No array/map reference survives a callback. A replacement generation ends this emit.
        const FGuid EmittingSession = Session, EmittingGeneration = QueryGeneration;
        TSet<FGuid> Visited;
        while (!bDisposed && Session == EmittingSession && QueryGeneration == EmittingGeneration)
        {
            TSharedPtr<TFunction<void()>> Listener;
            for (const auto& Observer : Observers)
            {
                if (Visited.Contains(Observer.Id)) continue;
                Visited.Add(Observer.Id); Listener = Observer.Listener; break;
            }
            if (!Listener.IsValid()) return;
            (*Listener)();
        }
    }
    void FPlayState::RetireQuery()
    {
        QueryGeneration = FGuid::NewGuid();
        bQueryAvailable = false; bQueryTaken = false;
        Requests.Reset();
    }
    void FPlayState::Connect()
    {
        RetireQuery();
        // Future utility owner: clear/abort utility generation here, even without a scope.
        bQueryAvailable = !bDisposed && Scope.IsSet() && !Observers.IsEmpty();
    }
    FPlayObserverReceipt FPlayState::Subscribe(TFunction<void()> Listener)
    {
        FPlayObserverReceipt Receipt;
        if (bDisposed || !Listener) return Receipt;
        Receipt.Id = FGuid::NewGuid();
        FObserver Observer;
        Observer.Id = Receipt.Id; Observer.Listener = MakeShared<TFunction<void()>>(MoveTemp(Listener));
        Observers.Add(MoveTemp(Observer));
        if (!bQueryAvailable) Connect();
        return Receipt;
    }
    bool FPlayState::Unsubscribe(FPlayObserverReceipt Observer)
    {
        if (bDisposed || !Observer.Id.IsValid()) return false;
        const int32 Removed = Observers.RemoveAll([&Observer](const FObserver& Item) { return Item.Id == Observer.Id; });
        if (!Removed) return false;
        if (Observers.IsEmpty())
        {
            RetireQuery();
            // Future utility owner: disable and clear/abort, retaining previous observation.
            Snapshot.bBusy = false; // Canonical last-unsubscribe has no notification.
        }
        return true;
    }
    bool FPlayState::Select(TOptional<FPlayScope> Next)
    {
        if (bDisposed || PlayCommandPrivate::SameScope(Scope, Next)) return false;
        Scope = MoveTemp(Next); Snapshot = {};
        Connect(); Emit(); return true;
    }
    void FPlayState::Reconnect()
    {
        if (!bDisposed) Connect(); // Connect retains the displayed state and does not emit.
    }
    void FPlayState::ResetSession()
    {
        if (bDisposed) return;
        Session = FGuid::NewGuid();
        RetireQuery(); Scope.Reset(); Snapshot = {}; Observers.Reset();
        // Future utility owner: disable, clear/abort and discard previous observation.
    }
    void FPlayState::Dispose()
    {
        if (bDisposed) return;
        ResetSession(); bDisposed = true;
    }
    TOptional<FPlayQueryDispatch> FPlayState::TakeQuerySubscription()
    {
        if (bDisposed || !bQueryAvailable || bQueryTaken) return {};
        bQueryTaken = true;
        FPlayQueryDispatch Dispatch;
        Dispatch.Receipt.Session = Session; Dispatch.Receipt.Generation = QueryGeneration;
        if (!PlayCommandPrivate::SerializeArguments(PlayCommandPrivate::ScopeArguments(*Scope), Dispatch.ArgumentsJson))
        {
            FailQuery(PlayCommandPrivate::QueryError); return {};
        }
        return Dispatch;
    }
    bool FPlayState::IsCurrentQuery(FPlayQueryReceipt Receipt) const
    {
        return !bDisposed && bQueryAvailable && bQueryTaken
            && Receipt.Session == Session && Receipt.Generation == QueryGeneration;
    }
    void FPlayState::FailQuery(const FString& PublicMessage)
    {
        // Future utility owner: clear/abort without disabling or losing previous observation.
        // Deliberately retain request ownership in this generation while resetting busy.
        Snapshot = {}; Snapshot.bReady = true; Snapshot.Error = PublicMessage; Emit();
    }
    void FPlayState::Accept(const FPlayWireResult& Result)
    {
        if (Result.Disposition == EPlayWireDisposition::Accepted)
        {
            Snapshot.Wire = Result.Wire; Snapshot.bReady = true; Snapshot.Error.Reset();
        }
        Emit(); // A valid older round retains every wire/readiness/error field but still emits.
    }
    EPlayWireDisposition FPlayState::ObserveQuery(FPlayQueryReceipt Receipt, FString Json)
    {
        if (!IsCurrentQuery(Receipt)) return EPlayWireDisposition::Invalid;
        const auto Result = ReconcilePlayWire(Snapshot.Wire, Json);
        if (!Result.Wire.IsSet()) FailQuery(PlayCommandPrivate::QueryError);
        else Accept(Result);
        return Result.Disposition;
    }
    bool FPlayState::ObserveQueryFailure(FPlayQueryReceipt Receipt, TOptional<FString> PublicMessage)
    {
        if (!IsCurrentQuery(Receipt)) return false;
        const FString Error = PlayCommandPrivate::PublicError(PublicMessage, PlayCommandPrivate::QueryError);
        FailQuery(Error); return true;
    }
    FPlayState::FCommandRequest* FPlayState::FindCommand(const FPlayCommandReceipt& Receipt)
    {
        if (bDisposed || !Receipt.Id.IsValid() || Receipt.Session != Session || Receipt.Generation != QueryGeneration) return nullptr;
        return Requests.Find(Receipt.Id);
    }
    TOptional<FPlayCommandReceipt> FPlayState::BeginCommand(FPlayCommand Command)
    {
        using namespace PlayCommandPrivate;
        if (bDisposed || !Scope.IsSet() || Snapshot.bBusy || !Snapshot.bReady) return {};
        const auto CommandObject = CommandArguments(Command);
        const auto RoundBefore = Snapshot.Wire.IsSet() ? Snapshot.Wire->GetRound() : TOptional<FPlayRound>();
        const bool bRetiredRound = RoundBefore.IsSet() && std::holds_alternative<FPlayPlaygroundGame>(RoundBefore->Game);
        if (!CommandObject.IsValid() || (bRetiredRound && !std::holds_alternative<FPlayStartCommand>(Command)
            && !std::holds_alternative<FPlayRemoveMemoryCommand>(Command)))
        {
            Snapshot.Error = UnavailableError; Emit(); return {};
        }
        FPlayCommandReceipt Receipt;
        Receipt.Session = Session; Receipt.Generation = QueryGeneration; Receipt.Id = FGuid::NewGuid();
        FCommandRequest Request;
        Request.Dispatch.Receipt = Receipt;
        Request.BeganWallTime = WallTimeMs();
        Request.Dispatch.ActionId = FString::Printf(TEXT("play-%lld-%s"), static_cast<int64>(Request.BeganWallTime),
            *Receipt.Id.ToString(EGuidFormats::Digits));
        Requests.Add(Receipt.Id, Request);
        Snapshot.bBusy = true; Snapshot.Error.Reset(); Emit();
        // Reacquire after notification: query failure may admit another command, and scope/
        // session/observer replacement may have removed this request altogether.
        auto* Current = FindCommand(Receipt);
        if (!Current || !Scope.IsSet()) return {};
        const auto RoundNow = Snapshot.Wire.IsSet() ? Snapshot.Wire->GetRound() : TOptional<FPlayRound>();
        const double Revision = RoundNow.IsSet() ? RoundNow->Revision : 0;
        const auto Args = ScopeArguments(*Scope);
        FString Arguments;
        bool bValid = Args.IsValid() && SafeRevision(Revision);
        if (bValid)
        {
            Args->SetObjectField(TEXT("command"), CommandObject);
            if (RoundNow.IsSet()) Args->SetStringField(TEXT("expectedRoundId"), RoundNow->Id);
            else Args->SetField(TEXT("expectedRoundId"), MakeShared<FJsonValueNull>());
            Args->SetNumberField(TEXT("expectedRevision"), Revision);
            Args->SetStringField(TEXT("actionId"), Request.Dispatch.ActionId);
            bValid = SerializeArguments(Args, Arguments);
        }
        if (!bValid)
        {
            // Local preparation failure follows catch/finally without a pre-final emission.
            Requests.Remove(Receipt.Id); Snapshot.Error = CommandError; Snapshot.bBusy = false; Emit(); return {};
        }
        Current->Dispatch.ArgumentsJson = MoveTemp(Arguments);
        Current->Stage = ECommandStage::Prepared;
        return Receipt;
    }
    TOptional<FPlayCommandDispatch> FPlayState::MarkCommandDispatched(FPlayCommandReceipt Receipt)
    {
        auto* Request = FindCommand(Receipt);
        if (!Request || Request->Stage != ECommandStage::Prepared) return {};
        Request->Stage = ECommandStage::Dispatched;
        return Request->Dispatch;
    }
    bool FPlayState::AbandonPreparedCommand(FPlayCommandReceipt Receipt)
    {
        auto* Request = FindCommand(Receipt);
        if (!Request || Request->Stage != ECommandStage::Prepared) return false;
        Requests.Remove(Receipt.Id); Snapshot.bBusy = false; Emit(); return true;
    }
    TOptional<FPlayCommandCompletion> FPlayState::BeginCompletion(const FPlayCommandReceipt& Receipt)
    {
        auto* Request = FindCommand(Receipt);
        if (!Request || Request->Stage != ECommandStage::Dispatched) return {};
        Request->Stage = ECommandStage::CompletingFailure;
        Request->CompletionId = FGuid::NewGuid();
        FPlayCommandCompletion Completion;
        Completion.Receipt = Receipt; Completion.Id = Request->CompletionId;
        return Completion;
    }
    FPlayState::FCommandRequest* FPlayState::FindCompletion(const FPlayCommandCompletion& Completion)
    {
        auto* Request = FindCommand(Completion.Receipt);
        return Request && Completion.Id.IsValid() && Request->CompletionId == Completion.Id
            && (Request->Stage == ECommandStage::CompletingSuccess || Request->Stage == ECommandStage::CompletingFailure)
            ? Request : nullptr;
    }
    TOptional<FPlayCommandCompletion> FPlayState::ObserveCommand(FPlayCommandReceipt Receipt, FString Json)
    {
        using namespace PlayCommandPrivate;
        const double ReceivedWallTime = WallTimeMs();
        auto Completion = BeginCompletion(Receipt);
        if (!Completion.IsSet()) return {};
        auto* Request = FindCompletion(*Completion);
        const double BeganWallTime = Request->BeganWallTime;
        // Reuse the complete read validator, including validation before older-wire retention.
        // The second bounded parse reads serverTime; neither projection publishes on its own.
        const auto Result = ReconcilePlayWire(Snapshot.Wire, Json);
        FObject O;
        double ServerTime = 0;
        if (!Result.Wire.IsSet() || !Parse(Json, O, PlayLimits) || !Number(O, TEXT("serverTime"), ServerTime))
        {
            Snapshot.Error = CommandError; return Completion;
        }
        const double Offset = ServerTime - (BeganWallTime + ReceivedWallTime) / 2;
        if (!std::isfinite(Offset)) { Snapshot.Error = CommandError; return Completion; }
        Request->Stage = ECommandStage::CompletingSuccess;
        Completion->Disposition = Result.Disposition;
        Snapshot.ServerOffset = Offset;
        Accept(Result);
        // Only detached local data survives the notification. Finalize rechecks ownership.
        return Completion;
    }
    TOptional<FPlayCommandCompletion> FPlayState::ObserveCommandFailure(
        FPlayCommandReceipt Receipt, TOptional<FString> PublicMessage)
    {
        auto Completion = BeginCompletion(Receipt);
        if (!Completion.IsSet()) return {};
        Snapshot.Error = PlayCommandPrivate::PublicError(PublicMessage, PlayCommandPrivate::CommandError);
        return Completion; // No notification before canonical finally.
    }
    TOptional<FPlayCommandCompletion> FPlayState::ObserveCommandCompletionFailure(
        FPlayCommandCompletion Completion, TOptional<FString> PublicMessage)
    {
        auto* Request = FindCompletion(Completion);
        if (!Request || Request->Stage != ECommandStage::CompletingSuccess) return {};
        Request->Stage = ECommandStage::CompletingFailure; Request->CompletionId = FGuid::NewGuid();
        Completion.Id = Request->CompletionId; Completion.Disposition = EPlayWireDisposition::Invalid;
        Snapshot.Error = PlayCommandPrivate::PublicError(PublicMessage, PlayCommandPrivate::CommandError);
        return Completion;
    }
    bool FPlayState::FinalizeCommand(FPlayCommandCompletion Completion)
    {
        if (!FindCompletion(Completion)) return false;
        Requests.Remove(Completion.Receipt.Id);
        Snapshot.bBusy = false; Emit(); return true;
    }
}
