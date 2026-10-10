#include "BaddieCompanionFootballState.h"
#include "BaddieCompanionStateJson.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace BaddieCompanion
{
    namespace FootballPrivate
    {
        using namespace StateJsonPrivate;
        // Native allocation bounds, not canonical gameplay validation. Match/entry counts
        // come from footballLimits.players and SPACE_MEMBER_LIMIT respectively.
        constexpr FParseLimits Limits { 262144, 24, 8192 };
        constexpr int32 MaxText = 4096, MatchPlayers = 12, EntryPlayers = 50;
        constexpr int64 MaxCapturedNowMs = 4102444800000LL;
        const TCHAR* InvalidMatch = TEXT("Football data couldn't load. Reconnect to continue.");
        const TCHAR* InvalidEntry = TEXT("Football players couldn't load. Reopen the roster to try again.");
        const TCHAR* Disconnected = TEXT("Football disconnected. Reconnect to continue.");
        const TCHAR* CommandFailed = TEXT("Couldn't update the match. Try again.");

        bool Text(const FValue& V, FString& Out) { return String(V, Out) && Out.Len() <= MaxText; }
        bool Numeric(const FValue& V, double& Out) { return Number(V, Out); }
        bool Side(const FValue& V, EFootballSide& Out)
        {
            return EnumValue(V, Out, { { TEXT("home"), EFootballSide::Home }, { TEXT("away"), EFootballSide::Away } });
        }
        bool Phase(const FValue& V, EFootballPhase& Out)
        {
            return EnumValue(V, Out, { { TEXT("lobby"), EFootballPhase::Lobby }, { TEXT("playing"), EFootballPhase::Playing },
                { TEXT("restart"), EFootballPhase::Restart }, { TEXT("finished"), EFootballPhase::Finished },
                { TEXT("cancelled"), EFootballPhase::Cancelled } });
        }
        bool Winner(const FValue& V, EFootballWinner& Out)
        {
            return EnumValue(V, Out, { { TEXT("home"), EFootballWinner::Home }, { TEXT("away"), EFootballWinner::Away },
                { TEXT("draw"), EFootballWinner::Draw } });
        }
        template <typename T, typename F> bool Optional(const FValue& V, TOptional<T>& Out, F Read)
        {
            if (!V.IsValid()) return true;
            T Next;
            if (!Read(V, Next)) return false;
            Out = MoveTemp(Next);
            return true;
        }
        bool Point(const FValue& V, FFootballPoint& Out)
        {
            const auto O = Object(V);
            return Only(O, { TEXT("x"), TEXT("z") }) && Number(O, TEXT("x"), Out.X) && Number(O, TEXT("z"), Out.Z);
        }
        bool Pitch(const FValue& V, FFootballPitch& Out)
        {
            const auto O = Object(V);
            return Only(O, { TEXT("id"), TEXT("instanceId"), TEXT("revision"), TEXT("floorY"), TEXT("centre"),
                    TEXT("halfWidth"), TEXT("halfLength"), TEXT("goalHalfWidth"), TEXT("goalHeight") })
                && Text(Field(O, TEXT("id")), Out.Id) && Text(Field(O, TEXT("instanceId")), Out.InstanceId)
                && Number(O, TEXT("revision"), Out.Revision) && Number(O, TEXT("floorY"), Out.FloorY)
                && Point(Field(O, TEXT("centre")), Out.Centre) && Number(O, TEXT("halfWidth"), Out.HalfWidth)
                && Number(O, TEXT("halfLength"), Out.HalfLength) && Number(O, TEXT("goalHalfWidth"), Out.GoalHalfWidth)
                && Number(O, TEXT("goalHeight"), Out.GoalHeight);
        }
        bool Player(const FValue& V, FFootballPlayer& Out)
        {
            const auto O = Object(V);
            return Only(O, { TEXT("id"), TEXT("side"), TEXT("bot"), TEXT("goalkeeper"), TEXT("position"), TEXT("velocity"),
                    TEXT("yaw"), TEXT("ready"), TEXT("connected"), TEXT("lastSequence"), TEXT("actionAt"), TEXT("goals"), TEXT("assists") })
                && Text(Field(O, TEXT("id")), Out.Id) && Side(Field(O, TEXT("side")), Out.Side)
                && Boolean(Field(O, TEXT("bot")), Out.bBot) && Boolean(Field(O, TEXT("goalkeeper")), Out.bGoalkeeper)
                && Point(Field(O, TEXT("position")), Out.Position) && Point(Field(O, TEXT("velocity")), Out.Velocity)
                && Number(O, TEXT("yaw"), Out.Yaw) && Boolean(Field(O, TEXT("ready")), Out.bReady)
                && Boolean(Field(O, TEXT("connected")), Out.bConnected) && Number(O, TEXT("lastSequence"), Out.LastSequence)
                && Number(O, TEXT("actionAt"), Out.ActionAtMs) && Number(O, TEXT("goals"), Out.Goals)
                && Number(O, TEXT("assists"), Out.Assists);
        }
        bool Ball(const FValue& V, FFootballBall& Out)
        {
            const auto O = Object(V);
            return Only(O, { TEXT("position"), TEXT("velocity"), TEXT("height"), TEXT("verticalSpeed"),
                    TEXT("ownerId"), TEXT("lastTouchId"), TEXT("lastPassId"), TEXT("claimAfter") })
                && Point(Field(O, TEXT("position")), Out.Position) && Point(Field(O, TEXT("velocity")), Out.Velocity)
                && Number(O, TEXT("height"), Out.Height) && Number(O, TEXT("verticalSpeed"), Out.VerticalSpeed)
                && Nullable(Field(O, TEXT("ownerId")), Out.OwnerId, Text)
                && Nullable(Field(O, TEXT("lastTouchId")), Out.LastTouchId, Text)
                && Nullable(Field(O, TEXT("lastPassId")), Out.LastPassId, Text)
                && Number(O, TEXT("claimAfter"), Out.ClaimAfterMs);
        }
        bool Score(const FValue& V, FFootballScore& Out)
        {
            const auto O = Object(V);
            return Only(O, { TEXT("home"), TEXT("away") }) && Number(O, TEXT("home"), Out.Home) && Number(O, TEXT("away"), Out.Away);
        }
        bool Objective(const FValue& V, FFootballObjective& Out)
        {
            const auto O = Object(V);
            return Only(O, { TEXT("goalsToWin"), TEXT("durationMs") })
                && Number(O, TEXT("goalsToWin"), Out.GoalsToWin) && Number(O, TEXT("durationMs"), Out.DurationMs);
        }
        bool Event(const FValue& V, FFootballEvent& Out)
        {
            const auto O = Object(V);
            return Only(O, { TEXT("id"), TEXT("kind"), TEXT("side"), TEXT("playerId") })
                && Number(O, TEXT("id"), Out.Id)
                && EnumValue(Field(O, TEXT("kind")), Out.Kind, { { TEXT("kickoff"), EFootballEventKind::Kickoff },
                    { TEXT("goal"), EFootballEventKind::Goal }, { TEXT("out"), EFootballEventKind::Out },
                    { TEXT("save"), EFootballEventKind::Save }, { TEXT("finished"), EFootballEventKind::Finished },
                    { TEXT("cancelled"), EFootballEventKind::Cancelled } })
                && Nullable(Field(O, TEXT("side")), Out.Side, Side)
                && Nullable(Field(O, TEXT("playerId")), Out.PlayerId, Text);
        }
        bool Match(const FValue& V, FFootballMatch& Out)
        {
            const auto O = Object(V);
            return Only(O, { TEXT("id"), TEXT("epoch"), TEXT("revision"), TEXT("pitch"), TEXT("mode"), TEXT("phase"),
                    TEXT("players"), TEXT("ball"), TEXT("score"), TEXT("winner"), TEXT("startedAt"), TEXT("endsAt"),
                    TEXT("simulatedAt"), TEXT("restartAt"), TEXT("restartSide"), TEXT("objective"), TEXT("lastEvent") })
                && Text(Field(O, TEXT("id")), Out.Id) && Number(O, TEXT("epoch"), Out.Epoch)
                && Number(O, TEXT("revision"), Out.Revision) && Pitch(Field(O, TEXT("pitch")), Out.Pitch)
                && EnumValue(Field(O, TEXT("mode")), Out.Mode, { { TEXT("solo"), EFootballMode::Solo },
                    { TEXT("teams"), EFootballMode::Teams }, { TEXT("coop"), EFootballMode::Coop } })
                && Phase(Field(O, TEXT("phase")), Out.Phase)
                && ReadBoundedArray(Field(O, TEXT("players")), Out.Players, MatchPlayers, Player)
                && Ball(Field(O, TEXT("ball")), Out.Ball) && Score(Field(O, TEXT("score")), Out.Score)
                && Nullable(Field(O, TEXT("winner")), Out.Winner, Winner)
                && Nullable(Field(O, TEXT("startedAt")), Out.StartedAtMs, Numeric)
                && Nullable(Field(O, TEXT("endsAt")), Out.EndsAtMs, Numeric)
                && Number(O, TEXT("simulatedAt"), Out.SimulatedAtMs)
                && Nullable(Field(O, TEXT("restartAt")), Out.RestartAtMs, Numeric)
                && Side(Field(O, TEXT("restartSide")), Out.RestartSide)
                && Objective(Field(O, TEXT("objective")), Out.Objective)
                && Nullable(Field(O, TEXT("lastEvent")), Out.LastEvent, Event);
        }
        bool Advancement(const FValue& V, FFootballAdvancement& Out)
        {
            const auto O = Object(V);
            return Only(O, { TEXT("ownerId"), TEXT("ownerUntil"), TEXT("nextAt"), TEXT("serverNow") })
                && Nullable(Field(O, TEXT("ownerId")), Out.OwnerId, Text)
                && Number(O, TEXT("ownerUntil"), Out.OwnerUntilMs)
                && Nullable(Field(O, TEXT("nextAt")), Out.NextAtMs, Numeric)
                && Number(O, TEXT("serverNow"), Out.ServerNowMs);
        }
        bool Snapshot(const FObject& O, FFootballSnapshot& Out)
        {
            return Only(O, { TEXT("playerId"), TEXT("host"), TEXT("callSessionId"), TEXT("match"), TEXT("ready"),
                    TEXT("busy"), TEXT("error"), TEXT("advancement") })
                && Nullable(Field(O, TEXT("playerId")), Out.PlayerId, Text) && Boolean(Field(O, TEXT("host")), Out.bHost)
                && Nullable(Field(O, TEXT("callSessionId")), Out.CallSessionId, Text)
                && Nullable(Field(O, TEXT("match")), Out.Match, Match) && Boolean(Field(O, TEXT("ready")), Out.bReady)
                && Boolean(Field(O, TEXT("busy")), Out.bBusy) && Nullable(Field(O, TEXT("error")), Out.Error, Text)
                && Optional(Field(O, TEXT("advancement")), Out.Advancement, Advancement);
        }
        bool EntryPlayer(const FValue& V, FFootballEntryPlayer& Out)
        {
            const auto O = Object(V);
            return Only(O, { TEXT("id"), TEXT("name"), TEXT("side") }) && Text(Field(O, TEXT("id")), Out.Id)
                && Text(Field(O, TEXT("name")), Out.Name) && Nullable(Field(O, TEXT("side")), Out.Side, Side);
        }
        bool EntryRoster(const FObject& O, FFootballEntryRoster& Out)
        {
            return Only(O, { TEXT("playerId"), TEXT("instanceId"), TEXT("revision"), TEXT("epoch"), TEXT("phase"),
                    TEXT("serverNow"), TEXT("players") })
                && Text(Field(O, TEXT("playerId")), Out.PlayerId) && Text(Field(O, TEXT("instanceId")), Out.InstanceId)
                && Number(O, TEXT("revision"), Out.Revision) && Number(O, TEXT("epoch"), Out.Epoch)
                && Nullable(Field(O, TEXT("phase")), Out.Phase, Phase) && Number(O, TEXT("serverNow"), Out.ServerNowMs)
                && ReadBoundedArray(Field(O, TEXT("players")), Out.Players, EntryPlayers, EntryPlayer);
        }
        bool SameScope(const FFootballReadScope& A, const FFootballReadScope& B)
        {
            return Same(A.AccountId, B.AccountId) && Same(A.SessionIncarnation, B.SessionIncarnation) && Same(A.RoomId, B.RoomId);
        }
        bool ValidScope(const FFootballReadScope& Scope)
        {
            return !Scope.AccountId.IsEmpty() && !Scope.SessionIncarnation.IsEmpty() && !Scope.RoomId.IsEmpty()
                && Scope.AccountId.Len() <= MaxText && Scope.SessionIncarnation.Len() <= MaxText && Scope.RoomId.Len() <= MaxText;
        }
        bool SamePlayerScope(const TOptional<FString>& A, const TOptional<FString>& B)
        {
            // The client uses playerId ?? "". Preserve null/empty equivalence for ordering
            // without losing the original nullable field in the displayed snapshot.
            return Same(A.Get(FString()), B.Get(FString()));
        }
        bool SameMatchScope(const FFootballSnapshot& A, const FFootballSnapshot& B)
        {
            if (A.Match.IsSet() != B.Match.IsSet() || !SamePlayerScope(A.PlayerId, B.PlayerId)) return false;
            return !A.Match.IsSet() || (Same(A.Match->Id, B.Match->Id) && A.Match->Epoch == B.Match->Epoch);
        }
        bool SameRevision(const FFootballSnapshot& A, const FFootballSnapshot& B)
        {
            return A.Match.IsSet() == B.Match.IsSet() && (!A.Match.IsSet() || A.Match->Revision == B.Match->Revision);
        }
    }

    bool DecodeFootballSnapshot(const FString& Json, TOptional<FFootballSnapshot>& Out, FString& OutError)
    {
        Out.Reset();
        OutError = FootballPrivate::InvalidMatch;
        StateJsonPrivate::FObject Root;
        FFootballSnapshot Next;
        if (!StateJsonPrivate::Parse(Json, Root, FootballPrivate::Limits) || !FootballPrivate::Snapshot(Root, Next)) return false;
        Out = MoveTemp(Next);
        OutError.Reset();
        return true;
    }
    bool DecodeFootballEntryRoster(const FString& Json, TOptional<FFootballEntryRoster>& Out, FString& OutError)
    {
        Out.Reset();
        OutError = FootballPrivate::InvalidEntry;
        StateJsonPrivate::FObject Root;
        FFootballEntryRoster Next;
        if (!StateJsonPrivate::Parse(Json, Root, FootballPrivate::Limits) || !FootballPrivate::EntryRoster(Root, Next)) return false;
        Out = MoveTemp(Next);
        OutError.Reset();
        return true;
    }
    bool FFootballReadState::Select(const FFootballReadScope& Scope)
    {
        if (bDisposed) return false;
        if (FootballPrivate::SameScope(Selection, Scope)) return true;
        return ResetSession(Scope);
    }
    bool FFootballReadState::ResetSession(const FFootballReadScope& Scope)
    {
        if (bDisposed) return false;
        // Copy first: callers may pass GetSelection().
        const FFootballReadScope Next = Scope;
        EndMatchObservation();
        RetireCommand();
        SessionGeneration = FGuid::NewGuid();
        EntryGenerations.Reset();
        Selection = Next;
        View = FFootballSnapshot{};
        bHasApplied = false;
        Changed.Broadcast();
        return true;
    }
    bool FFootballReadState::CanObserve(int64 CapturedNowMs) const
    {
        return !bDisposed && FootballPrivate::ValidScope(Selection)
            && CapturedNowMs >= 0 && CapturedNowMs <= FootballPrivate::MaxCapturedNowMs;
    }
    bool FFootballReadState::BeginMatchObservation(int64 CapturedNowMs, FFootballReadReceipt& Out)
    {
        Out = FFootballReadReceipt{};
        EndMatchObservation();
        if (!CanObserve(CapturedNowMs)) return false;
        bMatchObserved = true;
        Out.Generation = MatchGeneration;
        Out.Scope = Selection;
        Out.CapturedNowMs = CapturedNowMs;
        return true;
    }
    void FFootballReadState::EndMatchObservation()
    {
        // Canonical generation replacement does not clear command busy, even in flight.
        MatchGeneration = FGuid::NewGuid();
        bMatchObserved = false;
    }
    bool FFootballReadState::ReloadMatch(int64 CapturedNowMs, FFootballReadReceipt& Out)
    {
        Out = FFootballReadReceipt{};
        if (bDisposed) return false;
        View.Error.Reset();
        bool bConnected = false;
        if (bMatchObserved) bConnected = BeginMatchObservation(CapturedNowMs, Out);
        else EndMatchObservation();
        Changed.Broadcast();
        return bConnected;
    }
    bool FFootballReadState::CurrentMatch(const FFootballReadReceipt& Receipt) const
    {
        return !bDisposed && bMatchObserved && !Receipt.bEntry && Receipt.Generation.IsValid()
            && Receipt.Generation == MatchGeneration && Receipt.Scope.IsSet()
            && FootballPrivate::SameScope(Selection, Receipt.Scope.GetValue());
    }
    bool FFootballReadState::CurrentEntry(const FFootballReadReceipt& Receipt) const
    {
        return !bDisposed && Receipt.bEntry && Receipt.Generation.IsValid() && EntryGenerations.Contains(Receipt.Generation)
            && Receipt.Scope.IsSet() && FootballPrivate::SameScope(Selection, Receipt.Scope.GetValue());
    }
    void FFootballReadState::FailMatch(const FString& Message)
    {
        // Ordinary query failure retains the entire current match, ready and advancement.
        View.Error = Message;
        Changed.Broadcast();
    }
    EFootballReadResult FFootballReadState::AcceptMatch(const FFootballReadReceipt& Receipt, const FString& Json)
    {
        if (!CurrentMatch(Receipt)) return EFootballReadResult::StaleReceipt;
        TOptional<FFootballSnapshot> Decoded;
        FString Error;
        if (!DecodeFootballSnapshot(Json, Decoded, Error))
        {
            FailMatch(Error);
            return EFootballReadResult::InvalidResponse;
        }
        return ApplyMatch(MoveTemp(Decoded.GetValue()));
    }
    EFootballReadResult FFootballReadState::ApplyMatch(FFootballSnapshot Next)
    {
        if (View.Match.IsSet() && Next.Match.IsSet() && StateJsonPrivate::Same(View.Match->Id, Next.Match->Id)
            && Next.Match->Epoch < View.Match->Epoch) return EFootballReadResult::OlderEpoch;
        const bool bSameScope = bHasApplied && FootballPrivate::SameMatchScope(View, Next);
        if (bSameScope && View.Match.IsSet() && Next.Match.IsSet() && Next.Match->Revision < View.Match->Revision)
            return EFootballReadResult::OlderRevision;
        if (bSameScope && FootballPrivate::SameRevision(View, Next) && View.Advancement.IsSet() && Next.Advancement.IsSet()
            && Next.Advancement->ServerNowMs < View.Advancement->ServerNowMs) return EFootballReadResult::OlderAdvancement;
        // Neither query nor command wire data may take ownership of the local busy latch.
        Next.bBusy = View.bBusy;
        Next.Error.Reset();
        View = MoveTemp(Next);
        bHasApplied = true;
        Changed.Broadcast();
        return EFootballReadResult::Accepted;
    }
    bool FFootballReadState::AcceptMatchFailure(const FFootballReadReceipt& Receipt, const FFootballPublicReadFailure& Failure)
    {
        if (!CurrentMatch(Receipt)) return false;
        FailMatch(Failure.Message.IsSet() ? Failure.Message.GetValue() : FString(FootballPrivate::Disconnected));
        return true;
    }
    TOptional<FFootballCommandRequest> FFootballReadState::PrepareCommand(const FFootballCommand& Command)
    {
        using namespace FootballPrivate;
        if (bDisposed || View.bBusy || !View.Match.IsSet() || !ValidScope(Selection)) return {};
        // Read epochs stay finite doubles. Only this outbound boundary needs a safe integer,
        // and assertEpoch permits negative and zero values when authoritative state agrees.
        int64 Epoch = 0;
        if (!Integer(MakeShared<FJsonValueNumber>(View.Match->Epoch), Epoch, -9007199254740991LL)) return {};
        const auto Value = MakeShared<FJsonObject>();
        switch (Command.Type)
        {
        case EFootballCommandType::Ready:
            Value->SetStringField(TEXT("type"), TEXT("ready")); Value->SetBoolField(TEXT("ready"), Command.bReady); break;
        case EFootballCommandType::Side:
            if (Command.Side != EFootballSide::Home && Command.Side != EFootballSide::Away) return {};
            Value->SetStringField(TEXT("type"), TEXT("side"));
            Value->SetStringField(TEXT("side"), Command.Side == EFootballSide::Home ? TEXT("home") : TEXT("away")); break;
        case EFootballCommandType::Start: Value->SetStringField(TEXT("type"), TEXT("start")); break;
        case EFootballCommandType::Retry: Value->SetStringField(TEXT("type"), TEXT("retry")); break;
        case EFootballCommandType::Rejoin: Value->SetStringField(TEXT("type"), TEXT("rejoin")); break;
        case EFootballCommandType::Leave: Value->SetStringField(TEXT("type"), TEXT("leave")); break;
        case EFootballCommandType::End: Value->SetStringField(TEXT("type"), TEXT("end")); break;
        default: return {};
        }
        FFootballCommandRequest Request;
        Request.Type = Command.Type;
        Request.Receipt.QueryGeneration = MatchGeneration;
        Request.Receipt.SessionGeneration = SessionGeneration;
        Request.Receipt.RequestId = FGuid::NewGuid(); Request.Receipt.Scope = Selection;
        const auto Args = MakeShared<FJsonObject>();
        Args->SetStringField(TEXT("roomId"), Request.Receipt.Scope.RoomId);
        Args->SetNumberField(TEXT("epoch"), static_cast<double>(Epoch)); Args->SetObjectField(TEXT("command"), Value);
        if (!FJsonSerializer::Serialize(Args, TJsonWriterFactory<>::Create(&Request.ArgumentsJson))) return {};
        PendingCommand = Request; CommandStage = ECommandStage::Prepared;
        View.bBusy = true; View.Error.Reset(); Changed.Broadcast();
        // A query change intentionally leaves busy set. Its unsent receipt remains available
        // through GetPendingCommand; a replaced session must expose none of its old work.
        return CurrentCommand(Request.Receipt) ? TOptional<FFootballCommandRequest>(Request) : TOptional<FFootballCommandRequest>();
    }
    bool FFootballReadState::OwnsCommand(const FFootballCommandReceipt& Receipt) const
    {
        return !bDisposed && PendingCommand.IsSet() && Receipt.RequestId.IsValid()
            && Receipt.RequestId == PendingCommand->Receipt.RequestId && Receipt.SessionGeneration == SessionGeneration
            && Receipt.QueryGeneration == PendingCommand->Receipt.QueryGeneration
            && FootballPrivate::SameScope(Receipt.Scope, Selection);
    }
    bool FFootballReadState::CurrentCommand(const FFootballCommandReceipt& Receipt) const
    {
        // No bMatchObserved check: canonical send remains available without query listeners.
        return OwnsCommand(Receipt) && Receipt.QueryGeneration == MatchGeneration;
    }
    TOptional<FFootballCommandRequest> FFootballReadState::DispatchCommand(const FFootballCommandReceipt& Receipt)
    {
        if (!CurrentCommand(Receipt) || CommandStage != ECommandStage::Prepared) return {};
        CommandStage = ECommandStage::Dispatched;
        return PendingCommand.GetValue();
    }
    TOptional<FFootballCommandReceipt> FFootballReadState::GetPendingCommand() const
    {
        return PendingCommand.IsSet() ? TOptional<FFootballCommandReceipt>(PendingCommand->Receipt) : TOptional<FFootballCommandReceipt>();
    }
    bool FFootballReadState::AbandonCommand(const FFootballCommandReceipt& Receipt)
    {
        if (!OwnsCommand(Receipt) || CommandStage != ECommandStage::Prepared) return false;
        RetireCommand(); View.bBusy = false; Changed.Broadcast();
        return true;
    }
    void FFootballReadState::RetireCommand()
    {
        PendingCommand.Reset(); CommandStage = ECommandStage::Prepared;
    }
    bool FFootballReadState::CanCompleteCommand(const FFootballCommandReceipt& Receipt) const
    {
        return OwnsCommand(Receipt) && CommandStage == ECommandStage::Dispatched;
    }
    void FFootballReadState::FinishCommand(const FFootballCommandReceipt& Receipt)
    {
        // The apply/error notification may replace query generation, session or command.
        if (!OwnsCommand(Receipt)) return;
        const bool bCurrent = CurrentCommand(Receipt);
        RetireCommand();
        if (bCurrent) { View.bBusy = false; Changed.Broadcast(); }
    }
    TOptional<FFootballCommandSuccess> FFootballReadState::BeginCommandSuccess(const FFootballCommandReceipt& Receipt, const FString& Json)
    {
        if (!CanCompleteCommand(Receipt)) return {};
        CommandStage = ECommandStage::Completing;
        if (!CurrentCommand(Receipt)) { FinishCommand(Receipt); return {}; }
        FFootballCommandSuccess Success;
        FString Error;
        if (!DecodeFootballSnapshot(Json, Success.Snapshot, Error))
        {
            FailMatch(Error); FinishCommand(Receipt); return {};
        }
        Success.Receipt = Receipt;
        Success.bDeactivateInput = PendingCommand->Type == EFootballCommandType::Leave || PendingCommand->Type == EFootballCommandType::End;
        return Success;
    }
    bool FFootballReadState::IsCommandSuccessCurrent(const FFootballCommandSuccess& Success) const
    {
        return Success.Snapshot.IsSet() && CommandStage == ECommandStage::Completing && CurrentCommand(Success.Receipt);
    }
    EFootballReadResult FFootballReadState::AcceptCommandSuccess(const FFootballCommandSuccess& Success)
    {
        if (!Success.Snapshot.IsSet() || !OwnsCommand(Success.Receipt) || CommandStage != ECommandStage::Completing)
            return EFootballReadResult::StaleReceipt;
        if (!CurrentCommand(Success.Receipt)) { FinishCommand(Success.Receipt); return EFootballReadResult::StaleReceipt; }
        CommandStage = ECommandStage::Applying;
        const auto Result = ApplyMatch(Success.Snapshot.GetValue());
        FinishCommand(Success.Receipt);
        return Result;
    }
    bool FFootballReadState::AcceptCommandFailure(const FFootballCommandReceipt& Receipt, const FFootballPublicReadFailure& Failure)
    {
        if (!CanCompleteCommand(Receipt)) return false;
        CommandStage = ECommandStage::Completing;
        if (!CurrentCommand(Receipt)) { FinishCommand(Receipt); return false; }
        FailMatch(Failure.Message.IsSet() ? Failure.Message.GetValue() : FString(FootballPrivate::CommandFailed));
        FinishCommand(Receipt);
        return true;
    }
    bool FFootballReadState::BeginEntryObservation(int64 CapturedNowMs, FFootballReadReceipt& Out)
    {
        Out = FFootballReadReceipt{};
        if (!CanObserve(CapturedNowMs)) return false;
        Out.Generation = FGuid::NewGuid();
        Out.Scope = Selection;
        Out.CapturedNowMs = CapturedNowMs;
        Out.bEntry = true;
        EntryGenerations.Add(Out.Generation);
        return true;
    }
    void FFootballReadState::EndEntryObservation(const FFootballReadReceipt& Receipt)
    {
        if (CurrentEntry(Receipt)) EntryGenerations.Remove(Receipt.Generation);
    }
    EFootballReadResult FFootballReadState::AcceptEntry(const FFootballReadReceipt& Receipt, const FString& Json,
        TOptional<FFootballEntryRoster>& Out, FString& OutError) const
    {
        Out.Reset();
        OutError.Reset();
        if (!CurrentEntry(Receipt)) return EFootballReadResult::StaleReceipt;
        return DecodeFootballEntryRoster(Json, Out, OutError) ? EFootballReadResult::Accepted : EFootballReadResult::InvalidResponse;
    }
    bool FFootballReadState::AcceptEntryFailure(const FFootballReadReceipt& Receipt, const FFootballPublicReadFailure& Failure,
        FString& OutError) const
    {
        OutError.Reset();
        if (!CurrentEntry(Receipt)) return false;
        OutError = Failure.Message.IsSet() ? Failure.Message.GetValue() : FString(FootballPrivate::InvalidEntry);
        return true;
    }
    void FFootballReadState::Clear()
    {
        if (bDisposed) return;
        ResetSession(FFootballReadScope{});
    }
    void FFootballReadState::Dispose()
    {
        if (bDisposed) return;
        bDisposed = true;
        EndMatchObservation();
        RetireCommand(); SessionGeneration.Invalidate();
        EntryGenerations.Reset();
        Selection = FFootballReadScope{};
        View = FFootballSnapshot{};
        bHasApplied = false;
        Changed.Clear();
    }
}
