#pragma once

#include "CoreMinimal.h"
#include "Internationalization/LocKeyFuncs.h"

namespace BaddieCompanion
{
    enum class EMissionFieldPresence : uint8 { Missing, Null, Value };

    // Missing optional data stays distinct from an explicit null or a known false/zero.
    template <typename T> struct TMissionField
    {
        EMissionFieldPresence Presence = EMissionFieldPresence::Missing;
        T Value{};
        bool HasValue() const { return Presence == EMissionFieldPresence::Value; }
    };

    enum class EMissionRunPhase : uint8 { Lobby, Scene, Objective, Combat, Failed, Completed };
    enum class EMissionRewardStatus : uint8 { Carried, Banked, Forfeited };

    struct FMissionParticipant
    {
        FString UserId;
        FString Name;
    };

    struct FMissionReadiness
    {
        int64 Revision = 0;
        bool bSelfReady = false;
        TArray<FString> ReadyUserIds;
        bool bAllReady = false;
    };

    struct FMissionFinding
    {
        FString StepId;
        FString Text;
    };

    struct FMissionCheckpointTarget
    {
        FString PlaceId;
        // Canonical ScenePoint order and units, not UE world centimetres or an actor position.
        double X = 0;
        double Y = 0;
        double Z = 0;
        FString Label;
    };

    struct FMissionPublicReward
    {
        double Credits = 0;
        EMissionRewardStatus Status = EMissionRewardStatus::Carried;
    };

    struct FMissionRunState
    {
        FString Id;
        FString MissionId;
        FString LeaderId;
        TMissionField<FString> CrewThreadId;
        int64 Revision = 0;
        int64 StepIndex = 0;
        EMissionRunPhase Phase = EMissionRunPhase::Lobby;
        TMissionField<double> SceneStartedAt;
        TMissionField<FString> Failure;
        TArray<FMissionParticipant> Participants;
        // Public observations only. Neither flag is native command admission.
        TMissionField<bool> CanLead;
        TMissionField<bool> CompletionPending;
        TMissionField<FString> Finding;
        TMissionField<FMissionReadiness> Readiness;
        TMissionField<TArray<FMissionFinding>> Findings;
        TMissionField<TArray<FString>> ChallengeContributors;
        TMissionField<double> ObjectiveStartedAt;
        TMissionField<FMissionCheckpointTarget> Target;
        TMissionField<FMissionPublicReward> Reward;
    };

    using FMissionRewardTimes = TMap<FString, double, FDefaultSetAllocator, FLocKeyMapFuncs<double>>;

    struct FMissionSnapshotWire
    {
        TMissionField<FMissionRunState> Run;
        TMissionField<FMissionRunState> Joinable;
        TArray<FString> Completed;
        // Missing/null rewardReadyAt follows the canonical store's empty-map fallback.
        FMissionRewardTimes RewardReadyAt;
        bool bCanUseCrew = false;
        double ServerTimeMs = 0;
    };

    // Decodes the endpoint value, not a Convex envelope. Atomic output; fixed diagnostic.
    // Success establishes structure only. Unknown bounded mission/step IDs are preserved.
    BADDIECOMPANION_API bool DecodeMissionSnapshot(const FString& Json,
        FMissionSnapshotWire& Out, FString& OutError);

    enum class EMissionStateError : uint8 { None, InvalidSnapshot, InvalidReceiptTime, SubscriptionFailed, MutationFailed, InvalidCommand };
    enum class EMissionResponseDisposition : uint8 { Applied, Retained, Ignored, Invalid };

    struct FMissionLocalSnapshot
    {
        FMissionSnapshotWire Wire;
        bool bReady = false;
        bool bBusy = false;
        EMissionStateError Error = EMissionStateError::None;
        double ServerOffsetMs = 0;
    };

    class FMissionState;

    // Process-local callback metadata. Never a server token, credential or permission.
    struct FMissionObservationToken
    {
    private:
        friend class FMissionState;
        // Retained identity prevents old tokens matching a new owner at the same address.
        TSharedPtr<uint8> Owner;
        uint64 Generation = 0;
        FString GameRoomId;
    };

    struct FMissionMutationReceipt
    {
    private:
        friend class FMissionState;
        FMissionObservationToken Observation;
        uint64 RequestId = 0;
        uint64 ObservedUpdates = 0;
        double BeganAtMs = 0;
    };

    enum class EMissionCommandType : uint8
    {
        Resume, Start, Join, SetReady, TransferLead, Choose, Begin, Advance, Skip, Retry, Leave
    };
    struct FMissionPickupObservation
    {
        FString InstanceId;
        TOptional<int64> Revision;
    };
    struct FMissionCommand
    {
        EMissionCommandType Type = EMissionCommandType::Resume;
        // Only the selected tag's fields are validated and copied into the request.
        // Required revision/boolean absence is not silently converted to zero/false.
        FString MissionId, RunId, UserId, ChoiceId;
        TOptional<int64> Revision, ReadinessRevision;
        TOptional<bool> WithCrew, Ready;
        TOptional<FMissionPickupObservation> Pickup;
    };
    class FMissionDispatch
    {
        friend class FMissionState;
    public:
        FString GetEndpoint() const { return TEXT("companionMissions:send"); }
        FString GetArgumentsJson() const { return ArgumentsJson; }
    private:
        FString ArgumentsJson; // Captured room and command only, never credentials.
    };
    struct FMissionResumeObservation
    {
    private:
        friend class FMissionState;
        FMissionObservationToken Observation;
        // Copies share the one post-notification consumption check for this observation.
        TSharedPtr<bool> Consumed;
    };

    // Single game-thread owner with local request lifecycle only. No transport,
    // delegates, timers, actors, automatic retries or admission.
    class BADDIECOMPANION_API FMissionState
    {
    public:
        FMissionState() = default;
        FMissionState(const FMissionState&) = delete;
        FMissionState& operator=(const FMissionState&) = delete;
        FMissionState(FMissionState&&) = delete;
        FMissionState& operator=(FMissionState&&) = delete;

        const FMissionLocalSnapshot& GetSnapshot() const { return State; }
        const FString& GetSelectedRoom() const { return SelectedRoom; }

        // Empty selects no room. Same selection is a no-op; invalid input is rejected.
        bool Select(const FString& GameRoomId);
        // Retains displayed data, clears busy/error and invalidates every old callback.
        void Reload();
        // Equivalent to losing the last subscriber: retain data/error, retire callbacks.
        void StopObserving();
        // Use on identity/access loss, including a new account in the same room.
        void Clear();
        // Terminal and clears private presentation data as well as callback ownership.
        void Dispose();
        bool CaptureObservation(FMissionObservationToken& Out) const;

        EMissionResponseDisposition Observe(const FMissionObservationToken& Token,
            const FString& Json, double ReceivedAtMs);
        // Records run presence from this wire even if Accept retains an older same-run
        // update. Notify for Applied, then check resume through PrepareObservedResume.
        EMissionResponseDisposition Observe(const FMissionObservationToken& Token,
            const FString& Json, double ReceivedAtMs, FMissionResumeObservation& Out);
        bool ObserveFailure(const FMissionObservationToken& Token);

        // Bookkeeping for an externally admitted request only. Does not send or authorize it.
        // Capture immediately before dispatch; a failed dispatch must call FinishMutationFailure.
        bool BeginObservedMutation(double BeganAtMs, FMissionMutationReceipt& Out);
        EMissionResponseDisposition FinishMutation(const FMissionMutationReceipt& Receipt,
            const FString& Json, double ReceivedAtMs);
        bool FinishMutationFailure(const FMissionMutationReceipt& Receipt);

        // Prepare sets busy. Notify externally before calling MarkCommandDispatched.
        // The adapter must separately admit dispatch, disable transport retries and
        // leave query-cache invalidation disabled, matching the canonical mutation.
        bool PrepareCommand(const FMissionCommand& Command, double BeganAtMs, FMissionMutationReceipt& Out);
        // Call once immediately after handling/notification of the actual observation.
        // Busy refusal consumes that observation check, not the generation's eligibility.
        bool PrepareObservedResume(const FMissionResumeObservation& Observation,
            double BeganAtMs, FMissionMutationReceipt& Out);
        // Recheck receipt ownership immediately before actual dispatch; no callback,
        // notification or yield may intervene between a successful mark and sending.
        bool MarkCommandDispatched(const FMissionMutationReceipt& Receipt, FMissionDispatch& Out);
        bool AbandonPreparedCommand(const FMissionMutationReceipt& Receipt);
        // Dispatched commands complete through the existing FinishMutation/Failure pair.

    private:
        bool IsCurrent(const FMissionObservationToken& Token) const;
        bool IsCurrent(const FMissionMutationReceipt& Receipt) const;
        void RetireCallbacks();
        void ReleaseMutation();
        bool Accept(FMissionSnapshotWire&& Wire, double ReceivedAtMs);

        TSharedPtr<uint8> TokenOwner = MakeShared<uint8>(0);
        FMissionLocalSnapshot State;
        FString SelectedRoom;
        uint64 Generation = 1;
        uint64 Updates = 0;
        uint64 RequestSerial = 0;
        uint64 PendingRequest = 0;
        TOptional<FMissionDispatch> CommandDispatch;
        bool bCommandDispatched = false;
        bool bResumed = false;
        bool bDisposed = false;
    };
}
