#include "BaddieCompanionMissionState.h"
#include "BaddieCompanionStateJson.h"

#include "Containers/SharedString.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace BaddieCompanion
{
    // Named for unity-build isolation from the other protocol decoders.
    namespace MissionStatePrivate
    {
        using StateJsonPrivate::FObject;
        using StateJsonPrivate::FValue;
        using StateJsonPrivate::Same;
        using StateJsonPrivate::Object;
        using StateJsonPrivate::Field;
        using StateJsonPrivate::Parse;
        using StateJsonPrivate::String;
        using StateJsonPrivate::Number;
        using StateJsonPrivate::Integer;
        using StateJsonPrivate::Boolean;
        using StateJsonPrivate::ValidTime;
        using StateJsonPrivate::MaxInteger;
        constexpr StateJsonPrivate::FParseLimits ParseLimits{256 * 1024, 16, 16384};
        constexpr int32 MaxIdCharacters = 240;
        constexpr int32 MaxTextCharacters = 8192;
        constexpr int32 MaxHistoryEntries = 128;

        bool Identifier(const FString& Value)
        {
            if (Value.IsEmpty() || Value.Len() > MaxIdCharacters || !Same(Value, Value.TrimStartAndEnd())) return false;
            for (const TCHAR C : Value) if (C < 32 || C == 127) return false;
            return true;
        }

        bool Text(const FValue& Value, FString& Out)
        {
            return String(Value, Out) && Out.Len() <= MaxTextCharacters;
        }
        bool Id(const FValue& Value, FString& Out)
        {
            return Text(Value, Out) && Identifier(Out);
        }
        bool Time(const FValue& Value, double& Out)
        {
            // Keep fractional public v.number() times; do not round server data.
            return Number(Value, Out) && ValidTime(Out);
        }

        template <typename T> bool Enum(const FValue& Value, T& Out,
            std::initializer_list<TPair<const TCHAR*, T>> Choices)
        {
            FString Name;
            if (!Id(Value, Name)) return false;
            for (const auto& Choice : Choices)
                if (Same(Name, Choice.Key)) { Out = Choice.Value; return true; }
            return false;
        }

        template <typename T, typename F> bool ReadField(const FObject& Parent, const TCHAR* Name,
            TMissionField<T>& Out, bool bRequired, bool bNullable, F Read)
        {
            const auto Value = Field(Parent, Name);
            if (!Value.IsValid()) return !bRequired;
            if (Value->Type == EJson::Null)
            {
                Out.Presence = EMissionFieldPresence::Null;
                return bNullable;
            }
            T Next{};
            if (!Read(Value, Next)) return false;
            Out.Presence = EMissionFieldPresence::Value;
            Out.Value = MoveTemp(Next);
            return true;
        }

        template <typename T, typename F> bool Array(const FValue& Value, TArray<T>& Out, int32 Maximum, F Read)
        {
            if (!Value.IsValid() || Value->Type != EJson::Array || Value->AsArray().Num() > Maximum) return false;
            Out.Reserve(Value->AsArray().Num());
            for (const auto& Entry : Value->AsArray())
            {
                T Next{};
                if (!Read(Entry, Next)) return false;
                Out.Add(MoveTemp(Next));
            }
            return true;
        }

        bool Participant(const FValue& Value, FMissionParticipant& Out)
        {
            const auto O = Object(Value);
            return Id(Field(O, TEXT("userId")), Out.UserId) && Text(Field(O, TEXT("name")), Out.Name);
        }
        bool Readiness(const FValue& Value, FMissionReadiness& Out)
        {
            const auto O = Object(Value);
            return Integer(Field(O, TEXT("revision")), Out.Revision)
                && Boolean(Field(O, TEXT("selfReady")), Out.bSelfReady)
                && Array(Field(O, TEXT("readyUserIds")), Out.ReadyUserIds, 4, Id)
                && Boolean(Field(O, TEXT("allReady")), Out.bAllReady);
        }
        bool Finding(const FValue& Value, FMissionFinding& Out)
        {
            const auto O = Object(Value);
            return Id(Field(O, TEXT("stepId")), Out.StepId) && Text(Field(O, TEXT("text")), Out.Text);
        }
        bool Findings(const FValue& Value, TArray<FMissionFinding>& Out)
        {
            return Array(Value, Out, MaxHistoryEntries, Finding);
        }
        bool Contributors(const FValue& Value, TArray<FString>& Out)
        {
            // Evidence can retain departed participants; do not intersect with the visible roster.
            return Array(Value, Out, MaxHistoryEntries * 4, Id);
        }
        bool Target(const FValue& Value, FMissionCheckpointTarget& Out)
        {
            const auto O = Object(Value);
            const auto Point = Field(O, TEXT("point"));
            if (!Point.IsValid() || Point->Type != EJson::Array || Point->AsArray().Num() != 3) return false;
            const auto& XYZ = Point->AsArray();
            return Id(Field(O, TEXT("placeId")), Out.PlaceId) && Text(Field(O, TEXT("label")), Out.Label)
                && Number(XYZ[0], Out.X) && Number(XYZ[1], Out.Y) && Number(XYZ[2], Out.Z);
        }
        bool Reward(const FValue& Value, FMissionPublicReward& Out)
        {
            const auto O = Object(Value);
            return Number(Field(O, TEXT("credits")), Out.Credits) && Out.Credits >= 0 && Out.Credits <= MaxInteger
                && Enum<EMissionRewardStatus>(Field(O, TEXT("status")), Out.Status, {
                    {TEXT("carried"), EMissionRewardStatus::Carried}, {TEXT("banked"), EMissionRewardStatus::Banked},
                    {TEXT("forfeited"), EMissionRewardStatus::Forfeited}});
        }
        bool Run(const FValue& Value, FMissionRunState& Out)
        {
            const auto O = Object(Value);
            return Id(Field(O, TEXT("id")), Out.Id) && Id(Field(O, TEXT("missionId")), Out.MissionId)
                && Id(Field(O, TEXT("leaderId")), Out.LeaderId)
                && ReadField(O, TEXT("crewThreadId"), Out.CrewThreadId, true, true, Id)
                && Integer(Field(O, TEXT("revision")), Out.Revision)
                && Integer(Field(O, TEXT("stepIndex")), Out.StepIndex)
                && Enum<EMissionRunPhase>(Field(O, TEXT("phase")), Out.Phase, {
                    {TEXT("lobby"), EMissionRunPhase::Lobby}, {TEXT("scene"), EMissionRunPhase::Scene},
                    {TEXT("objective"), EMissionRunPhase::Objective}, {TEXT("combat"), EMissionRunPhase::Combat},
                    {TEXT("failed"), EMissionRunPhase::Failed}, {TEXT("completed"), EMissionRunPhase::Completed}})
                && ReadField(O, TEXT("sceneStartedAt"), Out.SceneStartedAt, true, true, Time)
                && ReadField(O, TEXT("failure"), Out.Failure, true, true, Text)
                && Array(Field(O, TEXT("participants")), Out.Participants, 4, Participant)
                && ReadField(O, TEXT("canLead"), Out.CanLead, false, false, Boolean)
                && ReadField(O, TEXT("completionPending"), Out.CompletionPending, false, false, Boolean)
                && ReadField(O, TEXT("finding"), Out.Finding, false, true, Text)
                && ReadField(O, TEXT("readiness"), Out.Readiness, false, false, Readiness)
                && ReadField(O, TEXT("findings"), Out.Findings, false, false, Findings)
                && ReadField(O, TEXT("challengeContributors"), Out.ChallengeContributors, false, false, Contributors)
                && ReadField(O, TEXT("objectiveStartedAt"), Out.ObjectiveStartedAt, false, false, Time)
                && ReadField(O, TEXT("target"), Out.Target, false, true, Target)
                && ReadField(O, TEXT("reward"), Out.Reward, false, true, Reward);
        }
        bool RewardTimes(const FValue& Value, FMissionRewardTimes& Out)
        {
            if (!Value.IsValid() || Value->Type == EJson::Null) return true;
            const auto O = Object(Value);
            if (!O.IsValid() || O->Values.Num() > MaxHistoryEntries) return false;
            for (const auto& Entry : O->Values)
            {
                // UE 5.8 JSON keys use FSharedString. Copy their full length before validation.
                if (Entry.Key.Len() > MaxIdCharacters) return false;
                const FString MissionId(Entry.Key.Len(), *Entry.Key);
                double ReadyAt = 0;
                if (!Identifier(MissionId) || !Time(Entry.Value, ReadyAt)) return false;
                Out.Add(MissionId, ReadyAt);
            }
            return true;
        }

        bool SameRun(const TMissionField<FMissionRunState>& A, const TMissionField<FMissionRunState>& B)
        {
            // JS optional IDs compare equal when both runs are null/absent.
            return A.HasValue() == B.HasValue() && (!A.HasValue() || Same(A.Value.Id, B.Value.Id));
        }
        int64 Revision(const TMissionField<FMissionRunState>& RunValue)
        {
            return RunValue.HasValue() ? RunValue.Value.Revision : -1;
        }

        bool EncodeCommand(const FMissionCommand& Command, const FString& GameRoomId, FString& Out)
        {
            const TCHAR* Type = nullptr;
            switch (Command.Type)
            {
            case EMissionCommandType::Resume: Type = TEXT("resume"); break;
            case EMissionCommandType::Start: Type = TEXT("start"); break;
            case EMissionCommandType::Join: Type = TEXT("join"); break;
            case EMissionCommandType::SetReady: Type = TEXT("set-ready"); break;
            case EMissionCommandType::TransferLead: Type = TEXT("transfer-lead"); break;
            case EMissionCommandType::Choose: Type = TEXT("choose"); break;
            case EMissionCommandType::Begin: Type = TEXT("begin"); break;
            case EMissionCommandType::Advance: Type = TEXT("advance"); break;
            case EMissionCommandType::Skip: Type = TEXT("skip"); break;
            case EMissionCommandType::Retry: Type = TEXT("retry"); break;
            case EMissionCommandType::Leave: Type = TEXT("leave"); break;
            default: return false;
            }
            // Native revisions are integers by type; validate presence and the wire range.
            const auto ValidRevision = [](const TOptional<int64>& Value)
                { return Value.IsSet() && *Value >= 0 && *Value <= MaxInteger; };
            const auto Value = MakeShared<FJsonObject>();
            Value->SetStringField(TEXT("type"), Type);
            if (Command.Type == EMissionCommandType::Start)
            {
                if (!Identifier(Command.MissionId) || !Command.WithCrew.IsSet()) return false;
                Value->SetStringField(TEXT("missionId"), Command.MissionId);
                Value->SetBoolField(TEXT("withCrew"), *Command.WithCrew);
                if (Command.Pickup.IsSet())
                {
                    if (!Identifier(Command.Pickup->InstanceId) || !ValidRevision(Command.Pickup->Revision)) return false;
                    const auto Pickup = MakeShared<FJsonObject>();
                    Pickup->SetStringField(TEXT("instanceId"), Command.Pickup->InstanceId);
                    Pickup->SetNumberField(TEXT("revision"), static_cast<double>(*Command.Pickup->Revision));
                    Value->SetObjectField(TEXT("pickup"), Pickup);
                }
            }
            else if (Command.Type != EMissionCommandType::Resume)
            {
                if (!Identifier(Command.RunId)) return false;
                Value->SetStringField(TEXT("runId"), Command.RunId);
                if (Command.Type == EMissionCommandType::SetReady)
                {
                    if (!ValidRevision(Command.ReadinessRevision) || !Command.Ready.IsSet()) return false;
                    Value->SetNumberField(TEXT("readinessRevision"), static_cast<double>(*Command.ReadinessRevision));
                    Value->SetBoolField(TEXT("ready"), *Command.Ready);
                }
                else if (Command.Type != EMissionCommandType::Join)
                {
                    if (!ValidRevision(Command.Revision)) return false;
                    Value->SetNumberField(TEXT("revision"), static_cast<double>(*Command.Revision));
                    if (Command.Type == EMissionCommandType::TransferLead)
                    {
                        if (!Identifier(Command.UserId)) return false;
                        Value->SetStringField(TEXT("userId"), Command.UserId);
                    }
                    else if (Command.Type == EMissionCommandType::Choose)
                    {
                        if (!Identifier(Command.ChoiceId)) return false;
                        Value->SetStringField(TEXT("choiceId"), Command.ChoiceId);
                    }
                }
            }
            const auto Arguments = MakeShared<FJsonObject>();
            Arguments->SetStringField(TEXT("gameRoomId"), GameRoomId);
            Arguments->SetObjectField(TEXT("command"), Value);
            return FJsonSerializer::Serialize(Arguments, TJsonWriterFactory<>::Create(&Out));
        }
    }

    bool DecodeMissionSnapshot(const FString& Json, FMissionSnapshotWire& Out, FString& OutError)
    {
        using namespace MissionStatePrivate;
        FObject O;
        FMissionSnapshotWire Next;
        if (!Parse(Json, O, ParseLimits)
            || !ReadField(O, TEXT("run"), Next.Run, true, true, Run)
            || !ReadField(O, TEXT("joinable"), Next.Joinable, true, true, Run)
            || !Array(Field(O, TEXT("completed")), Next.Completed, MaxHistoryEntries, Id)
            || !RewardTimes(Field(O, TEXT("rewardReadyAt")), Next.RewardReadyAt)
            || !Boolean(Field(O, TEXT("canUseCrew")), Next.bCanUseCrew)
            || !Time(Field(O, TEXT("serverTime")), Next.ServerTimeMs))
        {
            Out = FMissionSnapshotWire{};
            OutError = TEXT("The mission snapshot is unavailable.");
            return false;
        }
        Out = MoveTemp(Next);
        OutError.Reset();
        return true;
    }

    bool FMissionState::Select(const FString& GameRoomId)
    {
        using namespace MissionStatePrivate;
        if (bDisposed || (!GameRoomId.IsEmpty() && !Identifier(GameRoomId))) return false;
        if (Same(GameRoomId, SelectedRoom)) return true;
        // Copy first: the caller may pass our current room by reference.
        const FString NextRoom = GameRoomId;
        RetireCallbacks();
        if (bDisposed) return false;
        SelectedRoom = NextRoom;
        State = FMissionLocalSnapshot{};
        return true;
    }

    void FMissionState::RetireCallbacks()
    {
        ReleaseMutation();
        bResumed = false;
        if (Generation == MAX_uint64)
        {
            bDisposed = true;
            SelectedRoom.Reset();
            State = FMissionLocalSnapshot{};
            return;
        }
        ++Generation;
        Updates = 0;
    }

    void FMissionState::ReleaseMutation()
    {
        CommandDispatch.Reset();
        bCommandDispatched = false;
        PendingRequest = 0;
        State.bBusy = false;
    }

    void FMissionState::Reload()
    {
        if (bDisposed) return;
        RetireCallbacks();
        State.Error = EMissionStateError::None;
    }

    void FMissionState::StopObserving()
    {
        if (!bDisposed) RetireCallbacks();
    }

    void FMissionState::Clear()
    {
        if (bDisposed) return;
        RetireCallbacks();
        SelectedRoom.Reset();
        State = FMissionLocalSnapshot{};
    }

    void FMissionState::Dispose()
    {
        if (bDisposed) return;
        Clear();
        bDisposed = true;
    }

    bool FMissionState::CaptureObservation(FMissionObservationToken& Out) const
    {
        Out = FMissionObservationToken{};
        if (bDisposed || SelectedRoom.IsEmpty()) return false;
        Out.Owner = TokenOwner;
        Out.Generation = Generation;
        Out.GameRoomId = SelectedRoom;
        return true;
    }

    bool FMissionState::IsCurrent(const FMissionObservationToken& Token) const
    {
        return !bDisposed && !SelectedRoom.IsEmpty() && Token.Owner == TokenOwner && Token.Generation == Generation
            && MissionStatePrivate::Same(Token.GameRoomId, SelectedRoom);
    }

    bool FMissionState::IsCurrent(const FMissionMutationReceipt& Receipt) const
    {
        return IsCurrent(Receipt.Observation) && State.bBusy && Receipt.RequestId != 0 && Receipt.RequestId == PendingRequest;
    }

    bool FMissionState::Accept(FMissionSnapshotWire&& Wire, double ReceivedAtMs)
    {
        using namespace MissionStatePrivate;
        if (Wire.Run.HasValue() && SameRun(Wire.Run, State.Wire.Run)
            && Wire.Run.Value.Revision < State.Wire.Run.Value.Revision) return false;
        if (Updates == MAX_uint64) { Dispose(); return false; }
        ++Updates;
        State.ServerOffsetMs = Wire.ServerTimeMs - ReceivedAtMs;
        State.Wire = MoveTemp(Wire);
        State.bReady = true;
        State.Error = EMissionStateError::None;
        return true;
    }

    EMissionResponseDisposition FMissionState::Observe(const FMissionObservationToken& Token,
        const FString& Json, double ReceivedAtMs)
    {
        FMissionResumeObservation Ignored;
        return Observe(Token, Json, ReceivedAtMs, Ignored);
    }

    EMissionResponseDisposition FMissionState::Observe(const FMissionObservationToken& Token,
        const FString& Json, double ReceivedAtMs, FMissionResumeObservation& Out)
    {
        Out = {};
        if (!IsCurrent(Token)) return EMissionResponseDisposition::Ignored;
        FMissionSnapshotWire Wire;
        FString Error;
        if (!MissionStatePrivate::ValidTime(ReceivedAtMs) || !DecodeMissionSnapshot(Json, Wire, Error))
        {
            State.bReady = true;
            State.Error = !MissionStatePrivate::ValidTime(ReceivedAtMs)
                ? EMissionStateError::InvalidReceiptTime : EMissionStateError::InvalidSnapshot;
            return EMissionResponseDisposition::Invalid;
        }
        const bool bObservedRun = Wire.Run.HasValue();
        const bool bApplied = Accept(MoveTemp(Wire), ReceivedAtMs);
        if (bObservedRun && IsCurrent(Token))
        {
            Out.Observation = Token;
            Out.Consumed = MakeShared<bool>(false);
        }
        return bApplied ? EMissionResponseDisposition::Applied : EMissionResponseDisposition::Retained;
    }

    bool FMissionState::ObserveFailure(const FMissionObservationToken& Token)
    {
        if (!IsCurrent(Token)) return false;
        State.bReady = true;
        State.Error = EMissionStateError::SubscriptionFailed;
        return true;
    }

    bool FMissionState::BeginObservedMutation(double BeganAtMs, FMissionMutationReceipt& Out)
    {
        Out = FMissionMutationReceipt{};
        if (bDisposed || State.bBusy || SelectedRoom.IsEmpty()) return false;
        if (!MissionStatePrivate::ValidTime(BeganAtMs))
        {
            State.Error = EMissionStateError::InvalidReceiptTime;
            return false;
        }
        if (RequestSerial == MAX_uint64 || !CaptureObservation(Out.Observation)) return false;
        Out.RequestId = ++RequestSerial;
        Out.ObservedUpdates = Updates;
        Out.BeganAtMs = BeganAtMs;
        PendingRequest = Out.RequestId;
        State.bBusy = true;
        State.Error = EMissionStateError::None;
        return true;
    }

    EMissionResponseDisposition FMissionState::FinishMutation(const FMissionMutationReceipt& Receipt,
        const FString& Json, double ReceivedAtMs)
    {
        using namespace MissionStatePrivate;
        if (!IsCurrent(Receipt) || (CommandDispatch.IsSet() && !bCommandDispatched)) return EMissionResponseDisposition::Ignored;
        FMissionSnapshotWire Wire;
        FString Error;
        if (!ValidTime(ReceivedAtMs) || !DecodeMissionSnapshot(Json, Wire, Error))
        {
            State.Error = !ValidTime(ReceivedAtMs) ? EMissionStateError::InvalidReceiptTime : EMissionStateError::InvalidSnapshot;
            ReleaseMutation();
            return EMissionResponseDisposition::Invalid;
        }
        EMissionResponseDisposition Result = EMissionResponseDisposition::Retained;
        if (Updates == Receipt.ObservedUpdates || (SameRun(Wire.Run, State.Wire.Run)
            && Revision(Wire.Run) >= Revision(State.Wire.Run)))
        {
            const double ServerTime = Wire.ServerTimeMs;
            if (Accept(MoveTemp(Wire), ReceivedAtMs)) Result = EMissionResponseDisposition::Applied;
            // Match the canonical owner's eligible-mutation midpoint even if Accept retained
            // an older same-run revision. Wall-clock changes do not become request ordering.
            if (!bDisposed) State.ServerOffsetMs = ServerTime - (Receipt.BeganAtMs + ReceivedAtMs) / 2;
        }
        ReleaseMutation();
        return Result;
    }

    bool FMissionState::FinishMutationFailure(const FMissionMutationReceipt& Receipt)
    {
        if (!IsCurrent(Receipt) || (CommandDispatch.IsSet() && !bCommandDispatched)) return false;
        State.Error = EMissionStateError::MutationFailed;
        ReleaseMutation();
        return true;
    }

    bool FMissionState::PrepareCommand(const FMissionCommand& Command, double BeganAtMs, FMissionMutationReceipt& Out)
    {
        Out = {};
        if (bDisposed || State.bBusy || SelectedRoom.IsEmpty()) return false;
        // Keep the accepted invalid-start-time diagnostic without minting a request.
        if (!MissionStatePrivate::ValidTime(BeganAtMs))
        {
            State.Error = EMissionStateError::InvalidReceiptTime;
            return false;
        }
        FMissionDispatch Dispatch;
        if (!MissionStatePrivate::EncodeCommand(Command, SelectedRoom, Dispatch.ArgumentsJson))
        {
            State.Error = EMissionStateError::InvalidCommand;
            return false;
        }
        if (!BeginObservedMutation(BeganAtMs, Out)) return false;
        CommandDispatch = MoveTemp(Dispatch);
        bCommandDispatched = false;
        return true;
    }

    bool FMissionState::PrepareObservedResume(const FMissionResumeObservation& Observation,
        double BeganAtMs, FMissionMutationReceipt& Out)
    {
        Out = {};
        if (!IsCurrent(Observation.Observation) || !Observation.Consumed.IsValid() || *Observation.Consumed) return false;
        // Consume the callback's check even on busy refusal. A later observation may
        // still try, but copies of this one cannot defer its check until a request ends.
        if (bResumed || State.bBusy)
        {
            *Observation.Consumed = true;
            return false;
        }
        FMissionCommand Resume;
        Resume.Type = EMissionCommandType::Resume;
        if (!PrepareCommand(Resume, BeganAtMs, Out)) return false;
        *Observation.Consumed = true;
        bResumed = true;
        return true;
    }

    bool FMissionState::MarkCommandDispatched(const FMissionMutationReceipt& Receipt, FMissionDispatch& Out)
    {
        Out = {};
        if (!IsCurrent(Receipt) || !CommandDispatch.IsSet() || bCommandDispatched) return false;
        Out = *CommandDispatch;
        bCommandDispatched = true;
        return true;
    }

    bool FMissionState::AbandonPreparedCommand(const FMissionMutationReceipt& Receipt)
    {
        if (!IsCurrent(Receipt) || !CommandDispatch.IsSet() || bCommandDispatched) return false;
        ReleaseMutation();
        return true;
    }
}
