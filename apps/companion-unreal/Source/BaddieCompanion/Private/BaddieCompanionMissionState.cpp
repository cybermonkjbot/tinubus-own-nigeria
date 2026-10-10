#include "BaddieCompanionMissionState.h"

#include "Containers/SharedString.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#include <cmath>
#include <initializer_list>

namespace BaddieCompanion
{
    // Named for unity-build isolation from the other protocol decoders.
    namespace MissionStatePrivate
    {
        constexpr int64 MaxInteger = 9007199254740991LL;
        constexpr int32 MaxDocumentCharacters = 256 * 1024;
        constexpr int32 MaxSeparators = 16384;
        constexpr int32 MaxIdCharacters = 240;
        constexpr int32 MaxTextCharacters = 8192;
        constexpr int32 MaxHistoryEntries = 128;
        using FObject = TSharedPtr<FJsonObject>;
        using FValue = TSharedPtr<FJsonValue>;

        bool Same(const FString& A, const FString& B)
        {
            return A.Equals(B, ESearchCase::CaseSensitive);
        }

        bool Identifier(const FString& Value)
        {
            if (Value.IsEmpty() || Value.Len() > MaxIdCharacters || !Same(Value, Value.TrimStartAndEnd())) return false;
            for (const TCHAR C : Value) if (C < 32 || C == 127) return false;
            return true;
        }

        bool Parse(const FString& Json, FObject& Out)
        {
            if (Json.IsEmpty() || Json.Len() > MaxDocumentCharacters) return false;
            bool bString = false, bEscape = false;
            int32 Depth = 0, Separators = 0;
            for (const TCHAR C : Json)
            {
                if (bString)
                {
                    if (bEscape) bEscape = false;
                    else if (C == TEXT('\\')) bEscape = true;
                    else if (C == TEXT('"')) bString = false;
                    continue;
                }
                if (C == TEXT('"')) bString = true;
                else if (C == TEXT('{') || C == TEXT('[')) { if (++Depth > 16) return false; }
                else if (C == TEXT('}') || C == TEXT(']')) { if (--Depth < 0) return false; }
                else if (C == TEXT(',') || C == TEXT(':')) { if (++Separators > MaxSeparators) return false; }
            }
            if (bString || Depth != 0) return false;
            return FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Out) && Out.IsValid();
        }

        FObject Object(const FValue& Value)
        {
            return Value.IsValid() && Value->Type == EJson::Object ? Value->AsObject() : nullptr;
        }
        FValue Field(const FObject& Value, const TCHAR* Name)
        {
            return Value.IsValid() ? Value->TryGetField(Name) : nullptr;
        }
        bool Text(const FValue& Value, FString& Out)
        {
            return Value.IsValid() && Value->Type == EJson::String && Value->TryGetString(Out)
                && Out.Len() <= MaxTextCharacters;
        }
        bool Id(const FValue& Value, FString& Out)
        {
            return Text(Value, Out) && Identifier(Out);
        }
        bool Number(const FValue& Value, double& Out)
        {
            return Value.IsValid() && Value->Type == EJson::Number && Value->TryGetNumber(Out) && std::isfinite(Out);
        }
        bool Integer(const FValue& Value, int64& Out)
        {
            double N = 0;
            if (!Number(Value, N) || N < 0 || N > MaxInteger || std::floor(N) != N) return false;
            Out = static_cast<int64>(N);
            return true;
        }
        bool ValidTime(double Value)
        {
            return std::isfinite(Value) && Value >= 0 && Value <= MaxInteger;
        }
        bool Time(const FValue& Value, double& Out)
        {
            // Keep fractional public v.number() times; do not round server data.
            return Number(Value, Out) && ValidTime(Out);
        }
        bool Boolean(const FValue& Value, bool& Out)
        {
            return Value.IsValid() && Value->Type == EJson::Boolean && Value->TryGetBool(Out);
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
        bool RewardTimes(const FValue& Value, TMap<FString, double>& Out)
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
    }

    bool DecodeMissionSnapshot(const FString& Json, FMissionSnapshotWire& Out, FString& OutError)
    {
        using namespace MissionStatePrivate;
        FObject O;
        FMissionSnapshotWire Next;
        if (!Parse(Json, O)
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
        PendingRequest = 0;
        State.bBusy = false;
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
        return Accept(MoveTemp(Wire), ReceivedAtMs) ? EMissionResponseDisposition::Applied : EMissionResponseDisposition::Retained;
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
        if (bDisposed || State.bBusy || !MissionStatePrivate::ValidTime(BeganAtMs)
            || RequestSerial == MAX_uint64 || !CaptureObservation(Out.Observation)) return false;
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
        if (!IsCurrent(Receipt)) return EMissionResponseDisposition::Ignored;
        FMissionSnapshotWire Wire;
        FString Error;
        if (!ValidTime(ReceivedAtMs) || !DecodeMissionSnapshot(Json, Wire, Error))
        {
            State.Error = !ValidTime(ReceivedAtMs) ? EMissionStateError::InvalidReceiptTime : EMissionStateError::InvalidSnapshot;
            State.bBusy = false;
            PendingRequest = 0;
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
        State.bBusy = false;
        PendingRequest = 0;
        return Result;
    }

    bool FMissionState::FinishMutationFailure(const FMissionMutationReceipt& Receipt)
    {
        if (!IsCurrent(Receipt)) return false;
        State.Error = EMissionStateError::MutationFailed;
        State.bBusy = false;
        PendingRequest = 0;
        return true;
    }
}
