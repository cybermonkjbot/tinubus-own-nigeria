#include "BaddieCompanionTeamsState.h"
#include "BaddieCompanionStateJson.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

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
            // UE requires a container root; validate nullable endpoint values in the helper.
            if (Same(Json.TrimStartAndEnd(), TEXT("null")))
                return StateJsonPrivate::ParseNull(Json, Out, ParseLimits);
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
        bool EcmaTrimWhitespace(uint32 C)
        {
            // ECMAScript WhiteSpace and LineTerminator, not native whitespace classification.
            return (C >= 0x0009 && C <= 0x000D) || C == 0x0020 || C == 0x00A0 || C == 0x1680
                || (C >= 0x2000 && C <= 0x200A) || C == 0x2028 || C == 0x2029 || C == 0x202F
                || C == 0x205F || C == 0x3000 || C == 0xFEFF;
        }
        bool ReservationGeneration(const FValue& V, TOptional<FString>& Out)
        {
            if (!V.IsValid()) return true;
            FString Next;
            if (!StateJsonPrivate::String(V, Next)) return false;
            int32 Utf16Units = 0;
            bool bHasContent = false;
            for (const TCHAR C : Next)
            {
                const uint32 Unit = sizeof(TCHAR) == 2 ? static_cast<uint16>(C) : static_cast<uint32>(C);
                // Surrogate halves count once; a scalar in a wider TCHAR counts twice.
                Utf16Units += Unit > 0xFFFF ? 2 : 1;
                if (Utf16Units > 128) return false;
                bHasContent |= !EcmaTrimWhitespace(Unit);
            }
            if (!bHasContent) return false;
            Out = MoveTemp(Next);
            return true;
        }
        bool Admission(const FValue& V, FTeamAdmission& Out)
        {
            const auto Generation = Field(StateJsonPrivate::Object(V), TEXT("reservationGeneration"));
            const auto O = Object(V, Generation.IsValid() ? 7 : 6);
            return Id(Field(O, TEXT("roomId")), Out.RoomId) && Id(Field(O, TEXT("threadId")), Out.ThreadId)
                && Nullable(Field(O, TEXT("callSessionId")), Out.CallSessionId, Id)
                && Number(Field(O, TEXT("startedAt")), Out.StartedAtMs)
                && Number(Field(O, TEXT("expiresAt")), Out.ExpiresAtMs)
                && Boolean(Field(O, TEXT("startMuted")), Out.bStartMuted) && Out.bStartMuted
                && ReservationGeneration(Generation, Out.ReservationGeneration);
        }
        bool InviteResult(const FValue& V, FTeamInviteResult& Out)
        {
            const auto O = Object(V, 2);
            return StateJsonPrivate::ReadBoundedArray(Field(O, TEXT("invitedUserIds")), Out.InvitedUserIds, 12, Id)
                && Number(Field(O, TEXT("expiresAt")), Out.ExpiresAtMs);
        }
        const TCHAR* SettingName(ETeamRoomSetting Setting)
        {
            switch (Setting)
            {
            case ETeamRoomSetting::Studio: return TEXT("studio");
            case ETeamRoomSetting::Bedroom: return TEXT("bedroom");
            case ETeamRoomSetting::Kitchen: return TEXT("kitchen");
            case ETeamRoomSetting::Terrace: return TEXT("terrace");
            case ETeamRoomSetting::TownSquare: return TEXT("town-square");
            default: return nullptr;
            }
        }
        bool Entering(ETeamCommandKind Kind) { return Kind == ETeamCommandKind::Create || Kind == ETeamCommandKind::Join; }
        bool EncodeObject(const TSharedRef<FJsonObject>& ObjectValue, FString& Out)
        {
            return FJsonSerializer::Serialize(ObjectValue,
                TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Out));
        }
        bool EncodeCommand(const FTeamCommand& Command, FString& Operation, FString& Out)
        {
            const auto Args = MakeShared<FJsonObject>();
            if (Command.Kind == ETeamCommandKind::Create)
            {
                if (!Command.ThreadId.IsSet() || !Identifier(Command.ThreadId.GetValue()) || !Command.Setting.IsSet()
                    || !SettingName(Command.Setting.GetValue()) || !Command.Capacity.IsSet() || !std::isfinite(Command.Capacity.GetValue())
                    || Command.RoomId.IsSet() || Command.StartedAtMs.IsSet() || Command.UserIds.IsSet()) return false;
                Operation = TEXT("companionGameTeams:create");
                Args->SetStringField(TEXT("threadId"), Command.ThreadId.GetValue());
                Args->SetStringField(TEXT("setting"), SettingName(Command.Setting.GetValue()));
                Args->SetNumberField(TEXT("capacity"), Command.Capacity.GetValue());
            }
            else
            {
                if (!Command.RoomId.IsSet() || !Identifier(Command.RoomId.GetValue()) || !Command.StartedAtMs.IsSet()
                    || !std::isfinite(Command.StartedAtMs.GetValue()) || Command.ThreadId.IsSet()
                    || Command.Setting.IsSet() || Command.Capacity.IsSet()) return false;
                if (Command.Kind == ETeamCommandKind::Invite)
                {
                    if (!Command.UserIds.IsSet() || Command.UserIds.GetValue().Num() > 12) return false;
                    TArray<FValue> Ids;
                    for (const FString& UserId : Command.UserIds.GetValue())
                    {
                        if (!Identifier(UserId)) return false;
                        Ids.Add(MakeShared<FJsonValueString>(UserId));
                    }
                    Args->SetArrayField(TEXT("userIds"), Ids);
                    Operation = TEXT("companionGameTeams:invite");
                }
                else
                {
                    if (Command.UserIds.IsSet()) return false;
                    switch (Command.Kind)
                    {
                    case ETeamCommandKind::Join: Operation = TEXT("companionGameTeams:join"); break;
                    case ETeamCommandKind::Decline: Operation = TEXT("companionGameTeams:decline"); break;
                    case ETeamCommandKind::Leave: Operation = TEXT("companionGameTeams:leave"); break;
                    default: return false;
                    }
                }
                Args->SetStringField(TEXT("roomId"), Command.RoomId.GetValue());
                Args->SetNumberField(TEXT("startedAt"), Command.StartedAtMs.GetValue());
            }
            return EncodeObject(Args.ToSharedRef(), Out);
        }
        bool CallObservation(const FTeamCallObservation& Call)
        {
            if (Call.Status != ETeamCallStatus::Idle && Call.Status != ETeamCallStatus::Joining
                && Call.Status != ETeamCallStatus::Joined && Call.Status != ETeamCallStatus::Leaving) return false;
            if (Call.Room.Presence == ETeamFieldPresence::Null) return true;
            if (!Call.Room.HasValue()) return false;
            const auto& RoomValue = Call.Room.Value;
            if (!Identifier(RoomValue.ThreadId) || (RoomValue.GameRoomId.IsSet()
                && !RoomValue.GameRoomId.GetValue().IsEmpty() && !Identifier(RoomValue.GameRoomId.GetValue()))) return false;
            return RoomValue.CallSessionId.Presence == ETeamFieldPresence::Null
                || (RoomValue.CallSessionId.HasValue() && (RoomValue.CallSessionId.Value.IsEmpty() || Identifier(RoomValue.CallSessionId.Value)));
        }
        bool SameCallId(const TTeamField<FString>& A, const TTeamField<FString>& B)
        {
            return A.Presence == B.Presence && (A.Presence == ETeamFieldPresence::Null
                || (A.HasValue() && Same(A.Value, B.Value)));
        }
        bool InGame(const FTeamCallObservation& Call, const FString& RoomId)
        {
            return Call.Status == ETeamCallStatus::Joined && Call.Room.HasValue()
                && Call.Room.Value.GameRoomId.IsSet() && Same(Call.Room.Value.GameRoomId.GetValue(), RoomId);
        }
        const TCHAR* KeepCall = TEXT("Keep the current call, or leave it before joining the team game.");
        const TCHAR* OpenFailed = TEXT("The team game couldn't open. Try again.");
        const TCHAR* InvalidCommandResponse = TEXT("The team response is unavailable.");
        FString FailureText(const FTeamsPublicFailure& Failure)
        {
            return !Failure.Message.IsEmpty() && Failure.Message.Len() <= 8192
                ? Failure.Message : FString(TEXT("That change couldn't finish. Try again."));
        }
        struct FCommandPacket
        {
            FTeamCommand Command;
            FTeamCallObservation InitialCall;
            TOptional<FTeamCallRoomObservation> LeavingCall;
            FString Operation, ArgumentsJson;
        };
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

    // Retained by the original caller's sealed receipts when Clear detaches the displayed owner.
    // No list/queue retains retired work, and no credential is stored here.
    struct FTeamsCommandWork
    {
        TSharedPtr<uint8> Store, Identity, Request, Action;
        TSharedPtr<const TeamsStatePrivate::FCommandPacket> Packet;
        ETeamsCommandStage Stage = ETeamsCommandStage::Prepared;
        FString SelectedAtStart, Failure;
        TSharedPtr<const FTeamAdmission> Admission, Result;
        bool bFailed = false, bCleanupInCatch = false;
    };
    namespace TeamsStatePrivate
    {
        void Catch(FTeamsCommandWork& Work, FString Message)
        {
            Work.Result.Reset(); Work.Failure = MoveTemp(Message); Work.bFailed = true;
            Work.bCleanupInCatch = true;
            Work.Stage = Work.Admission.IsValid() ? ETeamsCommandStage::CleanupReady : ETeamsCommandStage::Finally;
        }
        void DisposedCleanup(FTeamsCommandWork& Work)
        {
            Work.Result.Reset(); Work.bCleanupInCatch = false; Work.Stage = ETeamsCommandStage::CleanupReady;
        }
        void CleanupFailed(FTeamsCommandWork& Work, FString Message)
        {
            // Failure of the disposal-path release enters catch, which makes one final cleanup.
            // Failure of that catch cleanup is swallowed; there is no retry loop.
            if (Work.bCleanupInCatch) Work.Stage = ETeamsCommandStage::Finally;
            else Catch(Work, MoveTemp(Message));
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
    bool DecodeTeamAdmission(const FString& Json, FTeamAdmission& Out, FString& OutError)
    {
        return TeamsStatePrivate::Decode(Json, Out, OutError, TeamsStatePrivate::Admission);
    }
    bool DecodeTeamInviteResult(const FString& Json, FTeamInviteResult& Out, FString& OutError)
    {
        return TeamsStatePrivate::Decode(Json, Out, OutError, TeamsStatePrivate::InviteResult);
    }
    bool DecodeTeamNullResult(const FString& Json, FString& OutError)
    {
        StateJsonPrivate::FValue Value;
        const bool bValid = StateJsonPrivate::ParseNull(Json, Value, TeamsStatePrivate::ParseLimits);
        OutError = bValid ? FString() : FString(TeamsStatePrivate::InvalidCommandResponse);
        return bValid;
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

    FTeamsState::FTeamsState() : StoreIdentity(MakeShared<uint8>(0)) { ResetSnapshot(); }
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
        OwnedCommand.Reset();
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
        State.CommandError.Reset();
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
        State.CommandError.Reset();
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
        State.CommandError.Reset();
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
        State.CommandError.Reset();
        return true;
    }
    bool FTeamsState::FinishObservedCommand(const FTeamsCommandObservation& Token, bool bSucceeded)
    {
        if (bDisposed || OwnedCommand.IsValid() || !Identity.IsValid() || Token.Identity != Identity || !PendingCommand.IsValid()
            || Token.Request != PendingCommand) return false;
        if (!bSucceeded && TeamsStatePrivate::Same(Token.SelectedAtStart, SelectedGroup)) State.Error = ETeamsStateError::CommandFailed;
        PendingCommand.Reset();
        State.bBusy = false;
        return true;
    }

    bool FTeamsState::OwnsWork(const TSharedPtr<FTeamsCommandWork>& Work) const
    {
        return Work.IsValid() && Work->Store == StoreIdentity;
    }
    bool FTeamsState::LiveWork(const TSharedPtr<FTeamsCommandWork>& Work) const
    {
        return OwnsWork(Work) && !bDisposed && Identity.IsValid() && Work->Identity == Identity
            && Work == OwnedCommand && PendingCommand.IsValid() && Work->Request == PendingCommand;
    }
    bool FTeamsState::CurrentAction(const FTeamsActionReceipt& Token) const
    {
        if (!OwnsWork(Token.Work) || !Token.Stage.IsValid() || Token.Work->Action != Token.Stage) return false;
        const auto Stage = Token.Work->Stage;
        return Stage == ETeamsCommandStage::MutationSent || Stage == ETeamsCommandStage::JoinSent
            || Stage == ETeamsCommandStage::LeaveCallSent || Stage == ETeamsCommandStage::CleanupSent;
    }
    void FTeamsState::SetCommandError(const FString& Message)
    {
        State.Error = ETeamsStateError::CommandFailed;
        State.CommandError = Message;
    }
    ETeamsPrepareResult FTeamsState::PrepareCommand(const FTeamCommand& Command,
        const FTeamCallObservation& InitialCall, FTeamsCommandReceipt& Out)
    {
        using namespace TeamsStatePrivate;
        Out = FTeamsCommandReceipt();
        if (bDisposed || !Identity.IsValid() || PendingCommand.IsValid() || State.bBusy) return ETeamsPrepareResult::Refused;
        FCommandPacket Packet;
        if (!EncodeCommand(Command, Packet.Operation, Packet.ArgumentsJson)) return ETeamsPrepareResult::InvalidInput;
        if (Command.Kind == ETeamCommandKind::Create && !Same(Command.ThreadId.GetValue(), SelectedGroup))
        {
            SetCommandError(TEXT("Choose this group before starting its game."));
            return ETeamsPrepareResult::PreflightFailed;
        }
        if (Command.Kind != ETeamCommandKind::Create && State.Team.HasValue() && State.Team.Value.Room.HasValue())
        {
            const auto& RoomValue = State.Team.Value.Room.Value;
            if (!Same(Command.RoomId.GetValue(), RoomValue.RoomId) || Command.StartedAtMs.GetValue() != RoomValue.StartedAtMs)
            {
                SetCommandError(TEXT("The game changed. Reopen its group to continue."));
                return ETeamsPrepareResult::PreflightFailed;
            }
        }
        if (!CallObservation(InitialCall)) return ETeamsPrepareResult::CallUnavailable;
        if (Entering(Command.Kind) && InitialCall.Status != ETeamCallStatus::Idle
            && !(InitialCall.Status == ETeamCallStatus::Joined && InitialCall.Room.HasValue()
                && InitialCall.Room.Value.GameRoomId.IsSet() && !InitialCall.Room.Value.GameRoomId.GetValue().IsEmpty()))
        {
            SetCommandError(KeepCall);
            return ETeamsPrepareResult::PreflightFailed;
        }
        Packet.Command = Command;
        Packet.InitialCall = InitialCall;
        if (Command.Kind == ETeamCommandKind::Leave && InGame(InitialCall, Command.RoomId.GetValue()))
            Packet.LeavingCall = InitialCall.Room.Value;
        const auto Work = MakeShared<FTeamsCommandWork>();
        Work->Store = StoreIdentity; Work->Identity = Identity; Work->Request = MakeShared<uint8>(0);
        Work->Packet = MakeShared<FCommandPacket>(MoveTemp(Packet));
        OwnedCommand = Work; PendingCommand = Work->Request;
        State.bBusy = true; State.Error = ETeamsStateError::None; State.CommandError.Reset();
        Out.Work = Work;
        return ETeamsPrepareResult::Prepared;
    }
    bool FTeamsState::ContinueCommandAfterNotification(const FTeamsCommandReceipt& Token)
    {
        if (!LiveWork(Token.Work) || Token.Work->Stage != ETeamsCommandStage::Prepared) return false;
        Token.Work->SelectedAtStart = SelectedGroup;
        Token.Work->Stage = ETeamsCommandStage::ReadyMutation;
        return true;
    }
    ETeamsCommandStage FTeamsState::GetCommandStage(const FTeamsCommandReceipt& Token) const
    {
        return OwnsWork(Token.Work) ? Token.Work->Stage : ETeamsCommandStage::Invalid;
    }
    ETeamsAdvanceResult FTeamsState::MarkNextAction(const FTeamsCommandReceipt& Token, const FTeamCallObservation& Call,
        FTeamsActionReceipt& OutReceipt, TSharedPtr<const FTeamsAction>& Out)
    {
        using namespace TeamsStatePrivate;
        OutReceipt = FTeamsActionReceipt(); Out.Reset();
        if (!OwnsWork(Token.Work) || !Token.Work->Packet.IsValid()) return ETeamsAdvanceResult::Rejected;
        auto& Work = *Token.Work;
        FTeamsAction Next;
        ETeamsCommandStage SentStage;
        switch (Work.Stage)
        {
        case ETeamsCommandStage::ReadyMutation:
            if (!LiveWork(Token.Work)) return ETeamsAdvanceResult::Rejected;
            Next.Kind = ETeamsActionKind::Mutation;
            Next.Operation = Work.Packet->Operation; Next.ArgumentsJson = Work.Packet->ArgumentsJson;
            SentStage = ETeamsCommandStage::MutationSent;
            break;
        case ETeamsCommandStage::AttachPresence:
            if (!LiveWork(Token.Work))
            {
                DisposedCleanup(Work);
                return ETeamsAdvanceResult::Advanced;
            }
            if (!CallObservation(Call)) return ETeamsAdvanceResult::NeedCallObservation;
            if (InGame(Call, Work.Admission->RoomId) && Same(Call.Room.Value.ThreadId, Work.Admission->ThreadId)
                && SameCallId(Call.Room.Value.CallSessionId, Work.Admission->CallSessionId))
            {
                Work.Stage = ETeamsCommandStage::AttachmentReturned;
                return ETeamsAdvanceResult::Advanced;
            }
            if (Call.Status != ETeamCallStatus::Idle)
            {
                Catch(Work, KeepCall);
                return ETeamsAdvanceResult::Advanced;
            }
            Next.Kind = ETeamsActionKind::JoinMutedCall; Next.Operation = TEXT("calls.join");
            Next.ThreadId = Work.Admission->ThreadId; Next.GameRoomId = Work.Admission->RoomId;
            Next.CallSessionId = Work.Admission->CallSessionId;
            SentStage = ETeamsCommandStage::JoinSent;
            break;
        case ETeamsCommandStage::CheckJoinedCall:
            if (!CallObservation(Call)) return ETeamsAdvanceResult::NeedCallObservation;
            if (!InGame(Call, Work.Admission->RoomId) || !Same(Call.Room.Value.ThreadId, Work.Admission->ThreadId)
                || !Call.Room.Value.CallSessionId.HasValue() || Call.Room.Value.CallSessionId.Value.IsEmpty()
                || (Work.Admission->CallSessionId.HasValue()
                    && !Same(Work.Admission->CallSessionId.Value, Call.Room.Value.CallSessionId.Value)))
            {
                Catch(Work, OpenFailed);
                return ETeamsAdvanceResult::Advanced;
            }
            {
                FTeamAdmission Attached = *Work.Admission;
                Attached.CallSessionId = Call.Room.Value.CallSessionId;
                Work.Admission = MakeShared<FTeamAdmission>(MoveTemp(Attached));
                Work.Stage = ETeamsCommandStage::AttachmentReturned;
                return ETeamsAdvanceResult::Advanced;
            }
        case ETeamsCommandStage::CheckLeavingCall:
            if (!Work.Packet->LeavingCall.IsSet())
            {
                Work.Stage = ETeamsCommandStage::Finally;
                return ETeamsAdvanceResult::Advanced;
            }
            if (!CallObservation(Call)) return ETeamsAdvanceResult::NeedCallObservation;
            if (!InGame(Call, Work.Packet->Command.RoomId.GetValue())
                || !Same(Call.Room.Value.ThreadId, Work.Packet->LeavingCall.GetValue().ThreadId)
                || !SameCallId(Call.Room.Value.CallSessionId, Work.Packet->LeavingCall.GetValue().CallSessionId))
            {
                Work.Stage = ETeamsCommandStage::Finally;
                return ETeamsAdvanceResult::Advanced;
            }
            // Like canonical explicit leave, this can drain after disposal, but only against
            // the original controller and all three captured call identifiers.
            Next.Kind = ETeamsActionKind::LeaveMatchingCall; Next.Operation = TEXT("calls.leave");
            Next.ThreadId = Call.Room.Value.ThreadId; Next.GameRoomId = Call.Room.Value.GameRoomId.GetValue();
            Next.CallSessionId = Call.Room.Value.CallSessionId;
            SentStage = ETeamsCommandStage::LeaveCallSent;
            break;
        case ETeamsCommandStage::CleanupReady:
            {
                if (!Work.Admission->ReservationGeneration.IsSet())
                {
                    Work.Stage = ETeamsCommandStage::Finally;
                    return ETeamsAdvanceResult::Advanced;
                }
                const auto Args = MakeShared<FJsonObject>();
                Args->SetStringField(TEXT("roomId"), Work.Admission->RoomId);
                Args->SetNumberField(TEXT("startedAt"), Work.Admission->StartedAtMs);
                Args->SetStringField(TEXT("reservationGeneration"), Work.Admission->ReservationGeneration.GetValue());
                if (!EncodeObject(Args.ToSharedRef(), Next.ArgumentsJson)) return ETeamsAdvanceResult::Rejected;
                Next.Kind = ETeamsActionKind::ReleaseReservation; Next.Operation = TEXT("companionGameTeams:releaseReservation");
                SentStage = ETeamsCommandStage::CleanupSent;
                break;
            }
        default: return ETeamsAdvanceResult::Rejected;
        }
        Work.Stage = SentStage; Work.Action = MakeShared<uint8>(0);
        OutReceipt.Work = Token.Work; OutReceipt.Stage = Work.Action;
        Out = MakeShared<FTeamsAction>(MoveTemp(Next));
        return ETeamsAdvanceResult::Dispatched;
    }
    ETeamsCompletionResult FTeamsState::AcceptMutationResult(const FTeamsActionReceipt& Token, const FString& Json)
    {
        using namespace TeamsStatePrivate;
        if (!CurrentAction(Token) || (Token.Work->Stage != ETeamsCommandStage::MutationSent
            && Token.Work->Stage != ETeamsCommandStage::CleanupSent)) return ETeamsCompletionResult::Rejected;
        auto& Work = *Token.Work;
        FString Error;
        if (Work.Stage == ETeamsCommandStage::CleanupSent)
        {
            const bool bValid = DecodeTeamNullResult(Json, Error);
            Work.Action.Reset();
            if (bValid) Work.Stage = ETeamsCommandStage::Finally;
            else CleanupFailed(Work, MoveTemp(Error));
            return bValid ? ETeamsCompletionResult::Accepted : ETeamsCompletionResult::InvalidResponse;
        }
        bool bValid;
        if (Entering(Work.Packet->Command.Kind))
        {
            FTeamAdmission AdmissionValue;
            bValid = DecodeTeamAdmission(Json, AdmissionValue, Error);
            if (bValid)
            {
                Work.Admission = MakeShared<FTeamAdmission>(MoveTemp(AdmissionValue));
                if (LiveWork(Token.Work)) Work.Stage = ETeamsCommandStage::AttachPresence;
                else DisposedCleanup(Work);
            }
        }
        else if (Work.Packet->Command.Kind == ETeamCommandKind::Invite)
        {
            FTeamInviteResult Invite;
            bValid = DecodeTeamInviteResult(Json, Invite, Error);
            if (bValid) Work.Stage = ETeamsCommandStage::Finally;
        }
        else
        {
            bValid = DecodeTeamNullResult(Json, Error);
            if (bValid) Work.Stage = Work.Packet->Command.Kind == ETeamCommandKind::Leave
                ? ETeamsCommandStage::CheckLeavingCall : ETeamsCommandStage::Finally;
        }
        Work.Action.Reset();
        if (!bValid) Catch(Work, MoveTemp(Error));
        return bValid ? ETeamsCompletionResult::Accepted : ETeamsCompletionResult::InvalidResponse;
    }
    bool FTeamsState::AcceptCallCompletion(const FTeamsActionReceipt& Token)
    {
        if (!CurrentAction(Token) || (Token.Work->Stage != ETeamsCommandStage::JoinSent
            && Token.Work->Stage != ETeamsCommandStage::LeaveCallSent)) return false;
        auto& Work = *Token.Work;
        Work.Action.Reset();
        Work.Stage = Work.Stage == ETeamsCommandStage::JoinSent ? ETeamsCommandStage::CheckJoinedCall : ETeamsCommandStage::Finally;
        return true;
    }
    bool FTeamsState::AcceptActionFailure(const FTeamsActionReceipt& Token, const FTeamsPublicFailure& Failure)
    {
        if (!CurrentAction(Token)) return false;
        auto& Work = *Token.Work;
        const FString Message = TeamsStatePrivate::FailureText(Failure);
        Work.Action.Reset();
        if (Work.Stage == ETeamsCommandStage::CleanupSent) TeamsStatePrivate::CleanupFailed(Work, Message);
        else TeamsStatePrivate::Catch(Work, Message);
        return true;
    }
    bool FTeamsState::CompleteAttachment(const FTeamsCommandReceipt& Token)
    {
        if (!OwnsWork(Token.Work) || Token.Work->Stage != ETeamsCommandStage::AttachmentReturned) return false;
        if (!LiveWork(Token.Work)) TeamsStatePrivate::DisposedCleanup(*Token.Work);
        else
        {
            Token.Work->Result = Token.Work->Admission;
            Token.Work->Stage = ETeamsCommandStage::Finally;
        }
        return true;
    }
    bool FTeamsState::AbandonUnsentCommand(const FTeamsCommandReceipt& Token)
    {
        if (!OwnsWork(Token.Work) || (Token.Work->Stage != ETeamsCommandStage::Prepared
            && Token.Work->Stage != ETeamsCommandStage::ReadyMutation)) return false;
        // No mutation or call stage was sent. No catch error, cleanup or invented outcome.
        Token.Work->Stage = ETeamsCommandStage::Finally;
        return true;
    }
    bool FTeamsState::FinishCommand(const FTeamsCommandReceipt& Token, TSharedPtr<const FTeamAdmission>& Out, bool& bNotify)
    {
        Out.Reset(); bNotify = false;
        if (!OwnsWork(Token.Work) || Token.Work->Stage != ETeamsCommandStage::Finally) return false;
        auto& Work = *Token.Work;
        Out = Work.Result;
        if (LiveWork(Token.Work))
        {
            if (Work.bFailed && TeamsStatePrivate::Same(SelectedGroup, Work.SelectedAtStart)) SetCommandError(Work.Failure);
            PendingCommand.Reset(); OwnedCommand.Reset(); State.bBusy = false; bNotify = true;
        }
        Work.Stage = ETeamsCommandStage::Finished;
        Work.Action.Reset(); Work.Packet.Reset(); Work.Admission.Reset(); Work.Result.Reset();
        Work.Identity.Reset(); Work.Request.Reset(); Work.SelectedAtStart.Reset(); Work.Failure.Reset();
        return true;
    }
}
