#pragma once

#include "BaddieCompanionHaulState.h"
#include "BaddieCompanionRecoveryState.h"

namespace BaddieCompanion
{
    struct FJobsContact
    {
        FString PlaceId, NpcId, ServiceId;
    };
    enum class EJobsRewardApproval : uint8 { Unavailable, Approved };
    struct FJobsTerms
    {
        FString OfferKey, WorldId, Label;
        FJobsContact Employer, Destination;
        FString ParcelKey, ParcelLabel;
        EJobsRewardApproval RewardApproval = EJobsRewardApproval::Unavailable;
        // Unavailable is exactly approval_required; approved value is money/game-value-unit.
        FString ApprovalKey;
        TOptional<FHaulReward> ApprovedReward;
        // Wire timeBasis is exactly server-ms. This is an acceptance window, not a deadline.
        double OpensAtMs = 0, ClosesAtMs = 0;
        // Missing is not explicit false. The wire accepts only missing or true.
        TOptional<bool> RequiresTrustedCourier;
    };
    enum class EJobsStatus : uint8 { Accepted, Carrying, Delivered, Cancelled, Failed };
    enum class EJobsFailure : uint8 { Death, OutingEnded, LifeEnded };
    struct FJobsJob
    {
        FString Id, OfferId, OutingId, LifeId;
        FJobsTerms Terms;
        int64 Revision = 0;
        EJobsStatus Status = EJobsStatus::Accepted;
        TOptional<EJobsFailure> Failure;
        double AcceptedAtMs = 0;
        TOptional<double> CollectedAtMs, EndedAtMs;
        TOptional<FHaulEntry> Reward;
    };
    struct FJobsOffer
    {
        FString Id;
        FJobsTerms Terms;
        bool bAvailable = false;
        int64 AvailabilityRevision = 0;
    };
    struct FJobsResident
    {
        FString ActorIdentity, Name;
    };
    struct FJobsGoal
    {
        bool bSelected = false, bInCurrentWorld = false;
        TOptional<FJobsResident> Resident;
        int64 Deliveries = 0, Target = 3;
        TOptional<double> CompletedAtMs;
    };
    enum class EJobsOutcome : uint8 { Applied, Replayed, Blocked };
    enum class EJobsBlockedReason : uint8
    {
        StaleContext, NotFound, WrongWorld, WrongContact, OfferUnavailable, RewardUnavailable,
        ActiveJob, AlreadyAttempted, OutOfOrder, JobClosed, OutingClosed, HaulFull, SourceUsed, TrustRequired
    };
    struct FJobsFeedback
    {
        EJobsOutcome Outcome = EJobsOutcome::Blocked;
        TOptional<EJobsBlockedReason> Reason;
    };
    struct FJobsCommandResult
    {
        FJobsFeedback Feedback;
        TOptional<FJobsJob> Job;
    };
    struct FJobsCommandResponse
    {
        TSharedPtr<const FWorldPlayerView> Player;
        int64 ServerNowMs = 0;
        FJobsCommandResult Result;
    };

    // Endpoint values, not Convex envelopes. A true return distinguishes known null/empty
    // from missing or invalid data. Failure atomically clears Out and returns fixed text.
    BADDIECOMPANION_API bool DecodeJobsJob(const FString& Json, TOptional<FJobsJob>& Out, FString& OutError);
    BADDIECOMPANION_API bool DecodeJobsOffers(const FString& Json, TArray<FJobsOffer>& Out, FString& OutError);
    BADDIECOMPANION_API bool DecodeJobsGoal(const FString& Json, FJobsGoal& Out, FString& OutError);
    BADDIECOMPANION_API bool DecodeJobsCommandResponse(const FString& Json, FJobsCommandResponse& Out, FString& OutError);

    // Missing from WorldPlayer: caller reads these from the CURRENT world store/renderer.
    // Services are layout observations, not NPC availability, proximity or permission.
    struct FJobsLayoutService
    {
        FString Id;
        TOptional<FString> NpcId;
        bool bPolicingOwner = false;
    };
    struct FJobsLayoutObservation
    {
        FString InstanceId, PlaceId;
        TArray<FJobsLayoutService> Services;
    };
    struct FJobsSources
    {
        // Trusted, bounded snapshots from this account's existing owners. Do not construct
        // readiness, health, outing or layout observations from unvalidated wire fields.
        bool bWorldReady = false, bWorldBusy = false;
        TOptional<FString> WorldError;
        TSharedPtr<const FWorldPlayerView> Player;
        int64 RenderAttempt = 0;
        TOptional<FString> RenderedInstanceId;
        TOptional<FJobsLayoutObservation> Layout;
        FRecoveryReadView Recovery;
        FHaulStoreSnapshot Haul;
        // First access failure from world/recovery/haul in canonical order, already sanitized
        // and classified by the account transport. Missing does not prove authorization.
        TOptional<FString> AccessFailure;
    };
    struct FJobsReadView
    {
        TOptional<FJobsJob> Job;
        EWorldFieldPresence JobPresence = EWorldFieldPresence::Missing;
        TOptional<FJobsContact> Contact;
        TArray<FJobsOffer> Offers;
        TOptional<FJobsGoal> Goal;
        TOptional<FJobsFeedback> Result;
        bool bReady = false, bOffersReady = false, bGoalReady = false, bBusy = false;
        TOptional<FString> Error, GoalError;
        // This read slice owns no pending command, retry decision or goal save.
    };
    enum class EJobsReadKind : uint8 { Job, Offers, Goal };
    enum class EJobsReadResult : uint8 { Accepted, Retained, StaleReceipt, InvalidResponse };
    class FJobsSourcesReceipt
    {
        friend class FJobsReadState;
        FGuid Generation;
    };
    class FJobsReadReceipt
    {
        friend class FJobsReadState;
        FGuid SourcesGeneration, ReadGeneration;
        EJobsReadKind Kind = EJobsReadKind::Job;
    };
    // Supplied by the external request owner, including the ORIGINAL contact on a retry.
    // Identifiers only. The command/request ID and uncertain retry lifecycle stay external.
    struct FJobsRequestOrigin
    {
        FString AccountId, GameRoomId, UserId, LifeId, OutingId;
        TOptional<FJobsContact> Contact;
    };
    class FJobsMutationReceipt
    {
        friend class FJobsReadState;
        FGuid Id, Generation, ContactGeneration;
        FJobsRequestOrigin Origin;
        FString InstanceId;
        int64 WorldRevision = 0, RenderAttempt = 0;
        EWorldPlayerStatus PlayerStatus = EWorldPlayerStatus::Present;
        uint64 JobUpdates = 0;
    };
    struct FJobsPublicFailure
    {
        // Public text from toSafeConvexError or its faithful transport equivalent.
        // The reducer caps length; it does not sanitize server payloads or classify errors.
        FString Message;
        bool bAccessFailure = false;
        // Already classified by the canonical transport. Ambiguous failures use fixed text.
        bool bDefinitivelyRejected = false;
    };

    // Account-private, single game-thread reducer. No requests, retry/goal scheduling,
    // ledger writes, movement, actor ownership, delegates or admission.
    class BADDIECOMPANION_API FJobsReadState
    {
    public:
        FJobsReadState() = default;
        FJobsReadState(const FJobsReadState&) = delete;
        FJobsReadState& operator=(const FJobsReadState&) = delete;
        FJobsReadState(FJobsReadState&&) = delete;
        FJobsReadState& operator=(FJobsReadState&&) = delete;
        FJobsReadView GetView() const { return View; }
        bool Select(const FString& AccountId, const FString& GameRoomId);
        bool BeginObservation(FJobsSourcesReceipt& Out);
        bool CaptureSources(FJobsSourcesReceipt& Out) const;
        void EndObservation();
        void Reload();
        void Clear();
        void Dispose();
        // Supply coherent latest stores before EACH query/mutation callback, as canonical sync does.
        bool ObserveSources(const FJobsSourcesReceipt& Receipt, const FJobsSources& Sources);
        bool SelectContact(const TOptional<FString>& ServiceId);
        // Opens a local slot or reuses its receipt. The caller owns actual subscription setup/stop.
        bool CaptureRead(EJobsReadKind Kind, FJobsReadReceipt& Out);
        EJobsReadResult Observe(const FJobsReadReceipt& Receipt, const FString& Json);
        bool ObserveFailure(const FJobsReadReceipt& Receipt, const FJobsPublicFailure& Failure);
        // Bookkeeping only, after external admission. No permission to issue a command.
        bool CaptureMutation(const FJobsRequestOrigin& Origin, FJobsMutationReceipt& Out);
        EJobsReadResult ObserveMutation(const FJobsMutationReceipt& Receipt, const FString& Json);
        bool ObserveMutationFailure(const FJobsMutationReceipt& Receipt, const FJobsPublicFailure& Failure);

    private:
        struct FContext
        {
            FString UserId, LifeId, OutingId, InstanceId;
            ECompanionHealthStatus HealthStatus = ECompanionHealthStatus::Active;
            EHaulStatus OutingStatus = EHaulStatus::Carried;
            EWorldPlayerStatus PlayerStatus = EWorldPlayerStatus::Present;
            int64 WorldRevision = 0, RenderAttempt = 0;
        };
        bool Enabled() const;
        bool Current(const FJobsReadReceipt& Receipt) const;
        bool SameScope(const FJobsMutationReceipt& Receipt) const;
        bool SameVisit(const FJobsMutationReceipt& Receipt) const;
        bool FeedbackCurrent(const FJobsMutationReceipt& Receipt) const;
        bool CanWork() const;
        TOptional<FContext> ReadContext() const;
        TOptional<FJobsContact> ResolveContact(const TOptional<FString>& ServiceId) const;
        void Reconcile();
        void ClearView(bool bKeepGoal);
        void Revoke(const FString& PublicMessage);
        void RefreshError();
        bool ReceiveJob(TOptional<FJobsJob> Job, bool bFromQuery, uint64 UpdatesAtStart);
        void FinishReceipt();
        FGuid& ReadGeneration(EJobsReadKind Kind);
        bool& ReadActive(EJobsReadKind Kind);

        FString AccountId, RoomId, GoalUserId;
        FJobsReadView View;
        FJobsSources Sources;
        TOptional<FContext> Context;
        TOptional<FString> SelectedService, JobError, OffersError, CommandError;
        FGuid SourcesGeneration = FGuid::NewGuid(), Generation = FGuid::NewGuid(), ContactGeneration = FGuid::NewGuid();
        FGuid JobGeneration = FGuid::NewGuid(), OffersGeneration = FGuid::NewGuid(), GoalGeneration = FGuid::NewGuid();
        FGuid ActiveMutation;
        FJobsRequestOrigin ActiveOrigin;
        uint64 JobUpdates = 0;
        bool bObserving = false, bDisposed = false, bBlocked = false, bMutationEligible = false;
        bool bJobActive = false, bOffersActive = false, bGoalActive = false;
    };
}
