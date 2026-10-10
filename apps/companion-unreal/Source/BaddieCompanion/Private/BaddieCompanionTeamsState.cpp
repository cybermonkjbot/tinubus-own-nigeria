#include "BaddieCompanionTeamsState.h"
#include "BaddieCompanionStateJson.h"

#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include <cmath>
#include <initializer_list>

namespace BaddieCompanion
{
    namespace TeamsStatePrivate
    {
        using StateJsonPrivate::FObject;
        using StateJsonPrivate::FValue;
        using StateJsonPrivate::Field;
        using StateJsonPrivate::Same;
        using StateJsonPrivate::Boolean;
        constexpr StateJsonPrivate::FParseLimits ParseLimits{256 * 1024, 8, 4096};
        bool Identifier(const FString& S)
        {
            if (S.IsEmpty() || S.Len() > 240 || !Same(S, S.TrimStartAndEnd())) return false;
            for (const TCHAR C : S) if (C < 32 || C == 127) return false;
            return true;
        }
        bool Clock(double N) { return std::isfinite(N) && N >= 0 && N <= 8640000000000000.0 && std::floor(N) == N; }
        bool Parse(const FString& Json, FValue& Out)
        {
            if (Json.IsEmpty() || Json.Len() > ParseLimits.Characters) return false;
            // Preserve the original reader's null/whitespace grammar before object-only Parse.
            if (Same(Json.TrimStartAndEnd(), TEXT("null")))
                return FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Out) && Out.IsValid();
            FObject Root;
            if (!StateJsonPrivate::Parse(Json, Root, ParseLimits)) return false;
            Out = MakeShared<FJsonValueObject>(Root);
            return true;
        }
        FObject Object(const FValue& V, int32 Fields)
        {
            const auto O = StateJsonPrivate::Object(V);
            return O.IsValid() && O->Values.Num() == Fields ? O : nullptr;
        }
        bool Text(const FValue& V, FString& Out)
        {
            return StateJsonPrivate::String(V, Out) && Out.Len() <= 8192;
        }
        bool Id(const FValue& V, FString& Out) { return Text(V, Out) && Identifier(Out); }
        bool HostId(const FValue& V, FString& Out) { return Text(V, Out) && (Out.IsEmpty() || Identifier(Out)); }
        bool Number(const FValue& V, double& Out)
        {
            return StateJsonPrivate::Number(V, Out) && StateJsonPrivate::ValidTime(Out);
        }
        bool Capacity(const FValue& V, int64& Out)
        {
            // The public city has capacity 50; the legacy command limit of 12 is not a read limit.
            return StateJsonPrivate::Integer(V, Out, 1);
        }
        template <typename T> bool Enum(const FValue& V, T& Out, std::initializer_list<TPair<const TCHAR*, T>> Choices)
        {
            FString Name;
            if (!Id(V, Name)) return false;
            for (const auto& Choice : Choices) if (Same(Name, Choice.Key)) { Out = Choice.Value; return true; }
            return false;
        }
        template <typename T, typename F> bool Nullable(const FValue& V, TTeamField<T>& Out, F Read)
        {
            TOptional<T> Next;
            if (!StateJsonPrivate::Nullable(V, Next, Read)) return false;
            if (!Next.IsSet()) { Out.Presence = ETeamFieldPresence::Null; return true; }
            Out.Value = MoveTemp(Next.GetValue());
            Out.Presence = ETeamFieldPresence::Value;
            return true;
        }
        bool Member(const FValue& V, FTeamMember& Out)
        {
            const auto O = Object(V, 6);
            return Id(Field(O, TEXT("userId")), Out.UserId) && Text(Field(O, TEXT("name")), Out.Name)
                && Enum<ETeamMemberRole>(Field(O, TEXT("role")), Out.Role,
                    {{TEXT("admin"), ETeamMemberRole::Admin}, {TEXT("member"), ETeamMemberRole::Member}})
                && Boolean(Field(O, TEXT("isYou")), Out.bIsYou) && Boolean(Field(O, TEXT("available")), Out.bAvailable)
                && Boolean(Field(O, TEXT("joined")), Out.bJoined);
        }
        bool Members(const FValue& V, TArray<FTeamMember>& Out)
        {
            if (!V.IsValid() || V->Type != EJson::Array || V->AsArray().Num() > 50) return false;
            for (const auto& Entry : V->AsArray())
            {
                FTeamMember Next;
                if (!Member(Entry, Next)) return false;
                Out.Add(MoveTemp(Next));
            }
            return true;
        }
        bool Room(const FValue& V, FTeamRoom& Out)
        {
            const auto O = Object(V, 11);
            return Id(Field(O, TEXT("roomId")), Out.RoomId) && Id(Field(O, TEXT("threadId")), Out.ThreadId)
                && Nullable(Field(O, TEXT("callSessionId")), Out.CallSessionId, Id)
                && HostId(Field(O, TEXT("hostId")), Out.HostId) && Text(Field(O, TEXT("name")), Out.Name)
                && Enum<ETeamRoomSetting>(Field(O, TEXT("setting")), Out.Setting,
                    {{TEXT("studio"), ETeamRoomSetting::Studio}, {TEXT("bedroom"), ETeamRoomSetting::Bedroom},
                    {TEXT("kitchen"), ETeamRoomSetting::Kitchen}, {TEXT("terrace"), ETeamRoomSetting::Terrace},
                    {TEXT("town-square"), ETeamRoomSetting::TownSquare}})
                && Enum<ETeamRoomStatus>(Field(O, TEXT("status")), Out.Status,
                    {{TEXT("starting"), ETeamRoomStatus::Starting}, {TEXT("open"), ETeamRoomStatus::Open}, {TEXT("paused"), ETeamRoomStatus::Paused}})
                && Capacity(Field(O, TEXT("capacity")), Out.Capacity)
                && Number(Field(O, TEXT("startedAt")), Out.StartedAtMs) && Number(Field(O, TEXT("expiresAt")), Out.ExpiresAtMs)
                && Boolean(Field(O, TEXT("isHost")), Out.bIsHost);
        }
        bool Response(const FValue& V, ETeamInvitationResponse& Out)
        {
            return Enum<ETeamInvitationResponse>(V, Out,
                {{TEXT("joined"), ETeamInvitationResponse::Joined}, {TEXT("declined"), ETeamInvitationResponse::Declined}});
        }
        bool Invitation(const FValue& V, FTeamInvitation& Out)
        {
            const auto O = Object(V, 4);
            return Id(Field(O, TEXT("invitationId")), Out.InvitationId) && Id(Field(O, TEXT("callSessionId")), Out.CallSessionId)
                && Number(Field(O, TEXT("expiresAt")), Out.ExpiresAtMs) && Nullable(Field(O, TEXT("response")), Out.Response, Response);
        }
        bool View(const FValue& V, FTeamView& Out)
        {
            const auto O = Object(V, 8);
            return Id(Field(O, TEXT("threadId")), Out.ThreadId) && Text(Field(O, TEXT("name")), Out.Name)
                && Boolean(Field(O, TEXT("canHost")), Out.bCanHost) && Boolean(Field(O, TEXT("canInvite")), Out.bCanInvite)
                && Boolean(Field(O, TEXT("canJoin")), Out.bCanJoin) && Members(Field(O, TEXT("members")), Out.Members)
                && Nullable(Field(O, TEXT("room")), Out.Room, Room) && Nullable(Field(O, TEXT("invitation")), Out.Invitation, Invitation);
        }
        bool CallEntry(const FValue& V, FTeamCallEntry& Out)
        {
            const auto O = Object(V, 3);
            return Id(Field(O, TEXT("roomId")), Out.RoomId) && Id(Field(O, TEXT("threadId")), Out.ThreadId)
                && Number(Field(O, TEXT("startedAt")), Out.StartedAtMs);
        }
        template <typename T, typename F> bool Decode(const FString& Json, T& Out, FString& OutError, F Read)
        {
            FValue Root;
            T Next{};
            if (!Parse(Json, Root) || !Read(Root, Next))
            {
                Out = T{};
                OutError = TEXT("The team response is unavailable.");
                return false;
            }
            Out = MoveTemp(Next);
            OutError.Reset();
            return true;
        }
        template <typename T> void SetNull(TTeamField<T>& FieldValue)
        {
            FieldValue = TTeamField<T>{};
            FieldValue.Presence = ETeamFieldPresence::Null;
        }
    }

    bool DecodeTeamView(const FString& Json, FTeamView& Out, FString& OutError)
    {
        return TeamsStatePrivate::Decode(Json, Out, OutError, TeamsStatePrivate::View);
    }
    bool DecodeTeamCallEntry(const FString& Json, TTeamField<FTeamCallEntry>& Out, FString& OutError)
    {
        return TeamsStatePrivate::Decode(Json, Out, OutError, [](const TeamsStatePrivate::FValue& V,
            TTeamField<FTeamCallEntry>& Next) { return TeamsStatePrivate::Nullable(V, Next, TeamsStatePrivate::CallEntry); });
    }
    bool ExpireTeamView(const FTeamView& Team, double NowMs, FTeamView& Out)
    {
        using namespace TeamsStatePrivate;
        if (!Clock(NowMs)) { Out = FTeamView{}; return false; }
        // Copy before assignment: in-place expiry is supported without changing any prior copy.
        FTeamView Next = Team;
        if (Next.Room.HasValue() && Next.Room.Value.ExpiresAtMs <= NowMs)
        {
            SetNull(Next.Room);
            SetNull(Next.Invitation);
            Next.bCanInvite = false;
            Next.bCanJoin = false;
            for (auto& MemberValue : Next.Members) MemberValue.bJoined = false;
        }
        else if (Next.Invitation.HasValue() && Next.Invitation.Value.ExpiresAtMs <= NowMs
            && !(Next.Invitation.Value.Response.HasValue() && Next.Invitation.Value.Response.Value == ETeamInvitationResponse::Joined))
        {
            SetNull(Next.Invitation);
            const bool bSelfJoined = Next.Members.ContainsByPredicate([](const FTeamMember& M) { return M.bIsYou && M.bJoined; });
            Next.bCanJoin = (Next.Room.HasValue() && Next.Room.Value.bIsHost) || (Next.bCanJoin && bSelfJoined);
        }
        Out = MoveTemp(Next);
        return true;
    }

    FTeamsState::FTeamsState() { ResetSnapshot(); }
    void FTeamsState::ResetSnapshot()
    {
        State = FTeamsSnapshot{};
        TeamsStatePrivate::SetNull(State.Team);
    }
    bool FTeamsState::SetIdentity(const FString& Scope)
    {
        if (bDisposed || (!Scope.IsEmpty() && !TeamsStatePrivate::Identifier(Scope))) return false;
        if (TeamsStatePrivate::Same(Scope, IdentityScope)) return true;
        const FString Next = Scope;
        Clear();
        IdentityScope = Next;
        if (!Next.IsEmpty()) Identity = MakeShared<uint8>(0);
        return true;
    }
    void FTeamsState::Clear()
    {
        Identity.Reset();
        Generation.Reset();
        PendingCommand.Reset();
        IdentityScope.Reset();
        SelectedGroup.Reset();
        ResetSnapshot();
    }
    void FTeamsState::Dispose() { Clear(); bDisposed = true; }
    bool FTeamsState::Select(const FString& ThreadId)
    {
        if (bDisposed || (!ThreadId.IsEmpty() && !TeamsStatePrivate::Identifier(ThreadId))) return false;
        if (TeamsStatePrivate::Same(ThreadId, SelectedGroup)) return true;
        SelectedGroup = ThreadId;
        StopObserving();
        const bool bBusy = State.bBusy;
        ResetSnapshot();
        State.bReady = ThreadId.IsEmpty();
        State.bBusy = bBusy;
        return true;
    }
    bool FTeamsState::BeginObservation(FTeamsObservation& Out)
    {
        Out = FTeamsObservation{};
        StopObserving();
        if (bDisposed || !Identity.IsValid() || SelectedGroup.IsEmpty()) return false;
        Generation = MakeShared<uint8>(0);
        Out.Identity = Identity;
        Out.Generation = Generation;
        Out.ThreadId = SelectedGroup;
        return true;
    }
    void FTeamsState::Reload()
    {
        if (bDisposed) return;
        StopObserving();
        State.Error = ETeamsStateError::None;
    }
    void FTeamsState::StopObserving() { Generation.Reset(); }
    bool FTeamsState::IsCurrent(const FTeamsObservation& Token) const
    {
        return !bDisposed && Identity.IsValid() && Generation.IsValid() && Token.Identity == Identity
            && Token.Generation == Generation && TeamsStatePrivate::Same(Token.ThreadId, SelectedGroup);
    }
    void FTeamsState::Fail(ETeamsStateError Error)
    {
        TeamsStatePrivate::SetNull(State.Team);
        State.bReady = true;
        State.Error = Error;
    }
    ETeamsObservationResult FTeamsState::Observe(const FTeamsObservation& Token, const FString& Json, double NowMs)
    {
        if (!IsCurrent(Token)) return ETeamsObservationResult::Ignored;
        if (!TeamsStatePrivate::Clock(NowMs)) { Fail(ETeamsStateError::InvalidClock); return ETeamsObservationResult::Invalid; }
        FTeamView Next;
        FString Error;
        if (!DecodeTeamView(Json, Next, Error) || !TeamsStatePrivate::Same(Next.ThreadId, SelectedGroup))
        {
            Fail(ETeamsStateError::InvalidResponse);
            return ETeamsObservationResult::Invalid;
        }
        ExpireTeamView(Next, NowMs, State.Team.Value);
        State.Team.Presence = ETeamFieldPresence::Value;
        State.bReady = true;
        State.Error = ETeamsStateError::None;
        return ETeamsObservationResult::Applied;
    }
    bool FTeamsState::ObserveFailure(const FTeamsObservation& Token)
    {
        if (!IsCurrent(Token)) return false;
        Fail(ETeamsStateError::SubscriptionFailed);
        return true;
    }
    ETeamsObservationResult FTeamsState::ObserveClock(const FTeamsObservation& Token, double NowMs)
    {
        if (!IsCurrent(Token)) return ETeamsObservationResult::Ignored;
        if (!TeamsStatePrivate::Clock(NowMs)) { Fail(ETeamsStateError::InvalidClock); return ETeamsObservationResult::Invalid; }
        if (State.Team.HasValue()) ExpireTeamView(State.Team.Value, NowMs, State.Team.Value);
        return ETeamsObservationResult::Applied;
    }
    bool FTeamsState::BeginObservedCommand(FTeamsCommandObservation& Out)
    {
        Out = FTeamsCommandObservation{};
        if (bDisposed || !Identity.IsValid() || State.bBusy) return false;
        PendingCommand = MakeShared<uint8>(0);
        Out.Identity = Identity;
        Out.Request = PendingCommand;
        Out.SelectedAtStart = SelectedGroup;
        State.bBusy = true;
        State.Error = ETeamsStateError::None;
        return true;
    }
    bool FTeamsState::FinishObservedCommand(const FTeamsCommandObservation& Token, bool bSucceeded)
    {
        if (bDisposed || !Identity.IsValid() || Token.Identity != Identity || !PendingCommand.IsValid()
            || Token.Request != PendingCommand) return false;
        if (!bSucceeded && TeamsStatePrivate::Same(Token.SelectedAtStart, SelectedGroup)) State.Error = ETeamsStateError::CommandFailed;
        PendingCommand.Reset();
        State.bBusy = false;
        return true;
    }
}
