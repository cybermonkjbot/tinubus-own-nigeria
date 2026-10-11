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
    struct FTeamAdmission
    {
        FString RoomId, ThreadId;
        TTeamField<FString> CallSessionId;
        double StartedAtMs = 0, ExpiresAtMs = 0;
        bool bStartMuted = false;
        // Absent on legacy admissions. Retain the exact value for reservation-only cleanup.
        TOptional<FString> ReservationGeneration;
    };
    struct FTeamInviteResult
    {
        TArray<FString> InvitedUserIds;
        double ExpiresAtMs = 0;
    };

    BADDIECOMPANION_API bool DecodeTeamView(const FString& Json, FTeamView& Out, FString& OutError);
    BADDIECOMPANION_API bool DecodeTeamCallEntry(const FString& Json, TTeamField<FTeamCallEntry>& Out, FString& OutError);
    BADDIECOMPANION_API bool DecodeTeamAdmission(const FString& Json, FTeamAdmission& Out, FString& OutError);
    BADDIECOMPANION_API bool DecodeTeamInviteResult(const FString& Json, FTeamInviteResult& Out, FString& OutError);
    BADDIECOMPANION_API bool DecodeTeamNullResult(const FString& Json, FString& OutError);
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
        // Fixed command diagnostics or bounded, already-sanitized adapter text. Never raw errors.
        FString CommandError;
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

    enum class ETeamCommandKind : uint8 { Invalid, Create, Invite, Join, Decline, Leave };
    struct FTeamCommand
    {
        ETeamCommandKind Kind = ETeamCommandKind::Invalid;
        // Create only. Numeric capacity is not limited to the legacy twelve-player value.
        TOptional<FString> ThreadId;
        TOptional<ETeamRoomSetting> Setting;
        TOptional<double> Capacity;
        // All other commands. Preserve supplied finite numbers without rounding.
        TOptional<FString> RoomId;
        TOptional<double> StartedAtMs;
        // Invite only. Required, possibly empty; at most twelve, ordered and not deduplicated.
        TOptional<TArray<FString>> UserIds;
    };
    enum class ETeamCallStatus : uint8 { Missing, Idle, Joining, Joined, Leaving };
    struct FTeamCallRoomObservation
    {
        FString ThreadId;
        TTeamField<FString> CallSessionId;
        TOptional<FString> GameRoomId;
    };
    struct FTeamCallObservation
    {
        // A fresh projection from the original account's call controller, not native permission.
        // Missing status/room is unavailable, never an idle default. Room must be null or value.
        ETeamCallStatus Status = ETeamCallStatus::Missing;
        TTeamField<FTeamCallRoomObservation> Room;
    };
    struct FTeamsCommandWork;
    class FTeamsCommandReceipt
    {
        friend class FTeamsState;
        TSharedPtr<FTeamsCommandWork> Work;
    };
    class FTeamsActionReceipt
    {
        friend class FTeamsState;
        TSharedPtr<FTeamsCommandWork> Work;
        TSharedPtr<uint8> Stage;
    };
    enum class ETeamsCommandStage : uint8
    {
        Invalid, Prepared, ReadyMutation, MutationSent, AttachPresence, JoinSent,
        CheckJoinedCall, AttachmentReturned, CheckLeavingCall, LeaveCallSent,
        CleanupReady, CleanupSent, Finally, Finished
    };
    enum class ETeamsActionKind : uint8 { Mutation, JoinMutedCall, LeaveMatchingCall, ReleaseReservation };
    struct FTeamsAction
    {
        ETeamsActionKind Kind = ETeamsActionKind::Mutation;
        // Mutation/cleanup only: immutable non-auth UE JSON, retry:false, invalidateQueryCache:false.
        FString Operation, ArgumentsJson;
        // Call actions only. Join passes ThreadId, omits a null CallSessionId and uses audio:false.
        // Leave calls leave() without arguments; these fields identify the checked call, not chat.
        FString ThreadId, GameRoomId;
        TTeamField<FString> CallSessionId;
        bool bAudio = false;
    };
    struct FTeamsPublicFailure
    {
        // Already sanitized Error-instance message, or empty for the canonical non-Error fallback.
        FString Message;
    };
    // PreflightFailed publishes a fixed error for caller notification. Other refusals do not write.
    enum class ETeamsPrepareResult : uint8 { Prepared, Refused, PreflightFailed, InvalidInput, CallUnavailable };
    enum class ETeamsAdvanceResult : uint8 { Rejected, NeedCallObservation, Advanced, Dispatched };
    enum class ETeamsCompletionResult : uint8 { Accepted, InvalidResponse, Rejected };

    // Single game-thread owner. No transport, call controller, timer or native admission.
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

        // Capture inputs and initial call before publishing busy. Caller then notifies, and only
        // the continuation samples selectedAtStart. Prepared work cannot dispatch after clear.
        ETeamsPrepareResult PrepareCommand(const FTeamCommand& Command, const FTeamCallObservation& InitialCall,
            FTeamsCommandReceipt& Out);
        bool ContinueCommandAfterNotification(const FTeamsCommandReceipt& Token);
        ETeamsCommandStage GetCommandStage(const FTeamsCommandReceipt& Token) const;
        // Supply a freshly sampled original-controller call at AttachPresence, CheckJoinedCall
        // and CheckLeavingCall. Other stages ignore Call. Advanced means inspect the next stage.
        // Dispatched is a one-time mark: perform exactly Out immediately, without yield/notify.
        ETeamsAdvanceResult MarkNextAction(const FTeamsCommandReceipt& Token, const FTeamCallObservation& Call,
            FTeamsActionReceipt& OutReceipt, TSharedPtr<const FTeamsAction>& Out);
        // Mutation/cleanup endpoint values, not envelopes. Call completions have no JSON payload.
        ETeamsCompletionResult AcceptMutationResult(const FTeamsActionReceipt& Token, const FString& Json);
        bool AcceptCallCompletion(const FTeamsActionReceipt& Token);
        bool AcceptActionFailure(const FTeamsActionReceipt& Token, const FTeamsPublicFailure& Failure);
        // The await after attachPresence has its own continuation, even for same-game success.
        // Recheck disposal here before fixing the return value or exposing captured cleanup.
        // Once Finally is reached, call FinishCommand without an added yield/notification.
        bool CompleteAttachment(const FTeamsCommandReceipt& Token);
        bool AbandonUnsentCommand(const FTeamsCommandReceipt& Token);
        // Applies catch error, releases exactly the original busy owner, and seals the receipt.
        // Notify only when bNotify is true, after this returns; deliver Out only to original caller.
        bool FinishCommand(const FTeamsCommandReceipt& Token, TSharedPtr<const FTeamAdmission>& Out, bool& bNotify);
        // Keep this state object AND each sent receipt alive until finally, including after
        // Clear/Dispose. Detached work can drain captured cleanup but cannot alter a new view.

    private:
        bool IsCurrent(const FTeamsObservation& Token) const;
        void Fail(ETeamsStateError Error);
        void ResetSnapshot();
        bool OwnsWork(const TSharedPtr<FTeamsCommandWork>& Work) const;
        bool LiveWork(const TSharedPtr<FTeamsCommandWork>& Work) const;
        bool CurrentAction(const FTeamsActionReceipt& Token) const;
        void SetCommandError(const FString& Message);

        FString IdentityScope;
        FString SelectedGroup;
        TSharedPtr<uint8> Identity;
        TSharedPtr<uint8> Generation;
        TSharedPtr<uint8> PendingCommand;
        TSharedPtr<uint8> StoreIdentity;
        TSharedPtr<FTeamsCommandWork> OwnedCommand;
        FTeamsSnapshot State;
        bool bDisposed = false;
    };
}
