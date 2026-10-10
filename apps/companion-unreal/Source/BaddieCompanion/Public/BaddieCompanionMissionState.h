#pragma once

#include "CoreMinimal.h"

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

    struct FMissionSnapshotWire
    {
        TMissionField<FMissionRunState> Run;
        TMissionField<FMissionRunState> Joinable;
        TArray<FString> Completed;
        // Missing/null rewardReadyAt follows the canonical store's empty-map fallback.
        TMap<FString, double> RewardReadyAt;
        bool bCanUseCrew = false;
        double ServerTimeMs = 0;
    };

    // Decodes the endpoint value, not a Convex envelope. Atomic output; fixed diagnostic.
    // Success establishes structure only. Unknown bounded mission/step IDs are preserved.
    BADDIECOMPANION_API bool DecodeMissionSnapshot(const FString& Json,
        FMissionSnapshotWire& Out, FString& OutError);

    enum class EMissionStateError : uint8 { None, InvalidSnapshot, InvalidReceiptTime, SubscriptionFailed, MutationFailed };
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

    // Single game-thread owner, with no requests, delegates, timers, actors or admission.
    // The future admitted caller owns observation lifetime and any explicit resume request.
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
        bool ObserveFailure(const FMissionObservationToken& Token);

        // Bookkeeping for an externally admitted request only. Does not send or authorize it.
        // Capture immediately before dispatch; a failed dispatch must call FinishMutationFailure.
        bool BeginObservedMutation(double BeganAtMs, FMissionMutationReceipt& Out);
        EMissionResponseDisposition FinishMutation(const FMissionMutationReceipt& Receipt,
            const FString& Json, double ReceivedAtMs);
        bool FinishMutationFailure(const FMissionMutationReceipt& Receipt);

    private:
        bool IsCurrent(const FMissionObservationToken& Token) const;
        bool IsCurrent(const FMissionMutationReceipt& Receipt) const;
        void RetireCallbacks();
        bool Accept(FMissionSnapshotWire&& Wire, double ReceivedAtMs);

        TSharedPtr<uint8> TokenOwner = MakeShared<uint8>(0);
        FMissionLocalSnapshot State;
        FString SelectedRoom;
        uint64 Generation = 1;
        uint64 Updates = 0;
        uint64 RequestSerial = 0;
        uint64 PendingRequest = 0;
        bool bDisposed = false;
    };
}
