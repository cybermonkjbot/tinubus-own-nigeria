#pragma once

#include "CoreMinimal.h"

namespace BaddieCompanion
{
    enum class ETeamFieldPresence : uint8 { Missing, Null, Value };
    template <typename T> struct TTeamField
    {
        ETeamFieldPresence Presence = ETeamFieldPresence::Missing;
        T Value{};
        bool HasValue() const { return Presence == ETeamFieldPresence::Value; }
    };

    enum class ETeamMemberRole : uint8 { Admin, Member };
    enum class ETeamRoomSetting : uint8 { Studio, Bedroom, Kitchen, Terrace, TownSquare };
    enum class ETeamRoomStatus : uint8 { Starting, Open, Paused };
    enum class ETeamInvitationResponse : uint8 { Joined, Declined };

    struct FTeamMember
    {
        FString UserId;
        FString Name;
        ETeamMemberRole Role = ETeamMemberRole::Member;
        bool bIsYou = false;
        bool bAvailable = false;
        bool bJoined = false;
    };
    struct FTeamRoom
    {
        FString RoomId;
        // The public city's room thread need not be the selected crew thread.
        FString ThreadId;
        TTeamField<FString> CallSessionId;
        // The server-owned public city deliberately supplies an empty host ID.
        FString HostId;
        FString Name;
        ETeamRoomSetting Setting = ETeamRoomSetting::Studio;
        ETeamRoomStatus Status = ETeamRoomStatus::Starting;
        int64 Capacity = 0;
        double StartedAtMs = 0;
        double ExpiresAtMs = 0;
        bool bIsHost = false;
    };
    struct FTeamInvitation
    {
        FString InvitationId;
        FString CallSessionId;
        double ExpiresAtMs = 0;
        TTeamField<ETeamInvitationResponse> Response;
    };
    struct FTeamView
    {
        FString ThreadId;
        FString Name;
        // Display observations, not permission to issue native commands.
        bool bCanHost = false;
        bool bCanInvite = false;
        bool bCanJoin = false;
        TArray<FTeamMember> Members;
        TTeamField<FTeamRoom> Room;
        TTeamField<FTeamInvitation> Invitation;
    };
    struct FTeamCallEntry
    {
        FString RoomId;
        FString ThreadId;
        double StartedAtMs = 0;
    };

    BADDIECOMPANION_API bool DecodeTeamView(const FString& Json, FTeamView& Out, FString& OutError);
    BADDIECOMPANION_API bool DecodeTeamCallEntry(const FString& Json, TTeamField<FTeamCallEntry>& Out, FString& OutError);
    // Copies its input, preserving array order and every field except canonical expiry changes.
    // Invalid clock clears output. No timer, call leave or server write is performed.
    BADDIECOMPANION_API bool ExpireTeamView(const FTeamView& Team, double NowMs, FTeamView& Out);

    enum class ETeamsStateError : uint8 { None, InvalidResponse, InvalidClock, SubscriptionFailed, CommandFailed };
    enum class ETeamsObservationResult : uint8 { Applied, Ignored, Invalid };
    struct FTeamsSnapshot
    {
        TTeamField<FTeamView> Team;
        bool bReady = false;
        bool bBusy = false;
        ETeamsStateError Error = ETeamsStateError::None;
    };

    class FTeamsState;
    struct FTeamsObservation
    {
    private:
        friend class FTeamsState;
        TSharedPtr<uint8> Identity;
        TSharedPtr<uint8> Generation;
        FString ThreadId;
    };
    struct FTeamsCommandObservation
    {
    private:
        friend class FTeamsState;
        TSharedPtr<uint8> Identity;
        TSharedPtr<uint8> Request;
        FString SelectedAtStart;
    };

    // Single game-thread read owner. Transport and canonical action preflights remain external.
    class BADDIECOMPANION_API FTeamsState
    {
    public:
        FTeamsState();
        FTeamsState(const FTeamsState&) = delete;
        FTeamsState& operator=(const FTeamsState&) = delete;
        FTeamsState(FTeamsState&&) = delete;
        FTeamsState& operator=(FTeamsState&&) = delete;

        bool SetIdentity(const FString& IdentityScope);
        void Clear();
        void Dispose();
        bool Select(const FString& ThreadId);
        FString GetSelectedGroup() const { return SelectedGroup; }
        FTeamsSnapshot GetSnapshot() const { return State; }
        // Begin/reconnect retains data/error. Reload additionally clears the public error.
        bool BeginObservation(FTeamsObservation& Out);
        void Reload();
        void StopObserving();
        ETeamsObservationResult Observe(const FTeamsObservation& Token, const FString& Json, double NowMs);
        bool ObserveFailure(const FTeamsObservation& Token);
        ETeamsObservationResult ObserveClock(const FTeamsObservation& Token, double NowMs);

        // Only busy/error bookkeeping for an externally admitted explicit command. Never sends,
        // admits, joins, leaves or edits membership. Finish success cannot synthesize a team.
        bool BeginObservedCommand(FTeamsCommandObservation& Out);
        bool FinishObservedCommand(const FTeamsCommandObservation& Token, bool bSucceeded);

    private:
        bool IsCurrent(const FTeamsObservation& Token) const;
        void Fail(ETeamsStateError Error);
        void ResetSnapshot();

        FString IdentityScope;
        FString SelectedGroup;
        TSharedPtr<uint8> Identity;
        TSharedPtr<uint8> Generation;
        TSharedPtr<uint8> PendingCommand;
        FTeamsSnapshot State;
        bool bDisposed = false;
    };
}
