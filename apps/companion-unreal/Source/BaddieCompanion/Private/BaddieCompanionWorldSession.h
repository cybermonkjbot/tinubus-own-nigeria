#pragma once

#include "BaddieCompanionWorldObservationPages.h"
#include "BaddieCompanionRecoveryState.h"
#include "BaddieCompanionWorldControls.h"
#include "BaddieCompanionWorldPublication.h"
#include "BaddieCompanionWorldCommandLifecycle.h"

class UBaddieCompanionGameplaySession;

namespace BaddieCompanion
{
    struct FWorldDispatchArgumentsOwner;
    class FWorldDispatchArguments
    {
    public:
        FWorldDispatchArguments() = default;
        explicit FWorldDispatchArguments(FWorldDispatchArgumentsOwner* InOwner);
        FWorldDispatchArguments(const FWorldDispatchArguments& Other);
        FWorldDispatchArguments(FWorldDispatchArguments&& Other) noexcept;
        FWorldDispatchArguments& operator=(const FWorldDispatchArguments& Other);
        FWorldDispatchArguments& operator=(FWorldDispatchArguments&& Other) noexcept;
        ~FWorldDispatchArguments();
        explicit operator bool() const { return Owner != nullptr; }
        FJsonObject* operator->() const;
    private:
        FWorldDispatchArgumentsOwner* Owner = nullptr;
        void Reset();
    };
    struct FWorldDispatch
    {
        EWorldRequest Operation = EWorldRequest::None;
        uint64 Lifetime = 0, Serial = 0;
        FWorldDispatchArguments Arguments;
        EWorldLane Lane = EWorldLane::None;
        uint64 Epoch = 0;
    };
    struct FWorldAttempt
    {
        FWorldDispatch Dispatch;
        FWorldMoveIntent Intent;
        FWorldCommandScope Scope;
        FWorldNativeEvidence Native;
        EWorldRequest Requested = EWorldRequest::None, CaptureKind = EWorldRequest::None;
        EWorldRequest Next = EWorldRequest::None;
        EWorldCommandOutcome Outcome = EWorldCommandOutcome::None;
        FString RequestId, Capture, Body;
        uint64 Epoch = 0, RequestSequence = 0, OperationSequence = 0;
        uint64 PresentationSerial = 0;
        uint64 IdentityCharge = 0, CaptureCharge = 0, BodyCharge = 0;
        double OriginalAt = 0, TicketAt = 0, Deadline = 0, NextAt = 0, SentAt = 0, ReceivedAt = 0;
        uint8 Lookups = 0;
        bool bActive = false, bInFlight = false, bLost = false, bSettled = false;
    };

    // No device proof or token is copied into this subordinate lifetime.
    struct FWorldEntryIdentity
    {
        uint64 CredentialGeneration = 0;
        int32 EntrySequence = 0;
        FString GrantId, UserId, CharacterId, HomeRoomId, GameRoomId;
        FString ParticipantId, CallSessionId, ExpressionSessionId;
        FString ApprovingSessionId, ReservationGeneration;
        double Deadline = 0;
    };

    class FWorldSession
    {
    public:
        FWorldSession();
        ~FWorldSession();
        FWorldSession(const FWorldSession&) = delete;
        FWorldSession& operator=(const FWorldSession&) = delete;
        FWorldSessionView View() const;

    private:
        friend class ::UBaddieCompanionGameplaySession;
        void Attach();
        bool SessionLive() const;
        bool HasCommandWork() const;
        void FailDispatchBarrier();
        void Detach();
        void Clear(EWorldReadStatus Status = EWorldReadStatus::Unavailable);
        void BeginEntry(const FWorldEntryIdentity& Identity);
        void Retry();
        void Tick(bool bRetirementOnly = false);
        bool TakeDispatch(FWorldDispatch& Out);
        void Complete(const FWorldDispatch& Dispatch, FString&& Body, double SentAt, double ReceivedAt);
        void Failed(const FWorldDispatch& Dispatch, bool bTransportUnavailable = false);
        bool Walk(const FMovementPoint& Target, EMovementPace Pace);
        void StopWalking();
        void CollisionStop(FWorldObservationToken Token, const FMovementCollisionStop& Receipt,
            const FMovementPose& Origin);
        void ObserveNative(const FWorldNativeEvidence& Evidence);
        bool CurrentCycle(FWorldObservationToken Token) const;
        bool InputReleaseRequired() const { return Commands.bReleaseRequired; }
        bool TransferCurrent(FWorldObservationToken Before, FWorldObservationToken After) const;
        void ReleaseInput();
        void ContentLost(FWorldObservationToken ObservationToken);

        bool Live() const;
        bool Current(const FWorldDispatch& Dispatch) const;
        bool CaptureScope(FWorldCommandScope& Out);
        bool ScopeLive(const FWorldCommandScope& Scope) const;
        bool StartCommand(EWorldLane Lane, EWorldRequest Operation, FWorldMoveIntent* Intent);
        bool TakeCommandDispatch(EWorldLane Lane, FWorldDispatch& Out);
        void ActivateCommandWorker();
        void RetireCommand(EWorldLane Lane);
        bool RetireScope(FWorldCommandScope& Scope);
        void FinishCommand(EWorldCommandOutcome Outcome);
        void ScheduleCommands();
        bool BeginCapture(EWorldRequest Operation, bool bContinueAttempt = false);
        bool RecoverResult(double Earliest);
        void AcceptTicket(const TSharedPtr<FJsonObject>& Ticket, bool bReplacement);
        void InvalidateContent();
        void RetireStatic();
        WorldObservationPrivate::FStaticContext StaticContext() const;
        WorldObservationPrivate::EStep RetainStatic();
        WorldObservationPrivate::EStep SettleCommand();
        void SeedOrdering();
        void Unavailable(EWorldReadStatus Status, bool bPreserveStatic = false);
        void ProcessBody();
        void AdvancePages();
        void AdvanceSemantics();
        void AdvancePublication();
        bool RetirePublication();
        bool RestartPublication();
        void NotifyRecovery(FWorldObservationToken Token);
        void CancelSemantics();
        bool RetireSemantics();
        WorldObservationPrivate::EStep ValidateCandidate();
        bool AcceptBinding(const TSharedPtr<FJsonObject>& Reply);
        void Retire(TSharedPtr<WorldObservationPrivate::FStorage> Storage);
        bool InitializeRetirement();
        void RetireBody();
        void RetireDecodedBody();
        void RetireRequest();
        void RetireAttemptText(bool bEntry);
        void RetireRequestText();

        FWorldEntryIdentity Entry;
        FWorldSessionView State;
        FWorldCommandLifecycle Commands;
        FWorldAttempt CommandLanes[2];
        EWorldLane WorkerLane = EWorldLane::Read;
        uint64 ConsumerSerial = 0, VisitSerial = 0;
        TSharedPtr<WorldObservationPrivate::FBudget> Budget;
        TUniquePtr<WorldObservationPrivate::FPages> Pages;
        TSharedPtr<WorldObservationPrivate::FStorage> Candidate, Accepted;
        struct FSemanticData;
        struct FSessionRetirement;
        TUniquePtr<FSessionRetirement> Retirement;
        int32 RetirementSlot = INDEX_NONE;
        TUniquePtr<FSemanticData> Semantic;
        TUniquePtr<WorldObservationPrivate::FPublicationPreparation> Publication;
        struct FVisitIdentity;
        TUniquePtr<FVisitIdentity> Visit, RetiringVisit;
        TUniquePtr<WorldObservationPrivate::FStaticRoots> Static, RetiringStatic;
        // One retirement owner, with fixed slots for a prior retirement and the
        // accepted/candidate pair revoked together. It drains only one at a time.
        TSharedPtr<WorldObservationPrivate::FStorage> Retiring[3];
        bool HasRetirement() const;
        TUniquePtr<WorldObservationPrivate::FAccess> Access;
        FRecoveryReadState Recovery;
        FWorldRecoveryReceipt RecoveryReceipt;
        TSharedPtr<WorldObservationPrivate::FPublicationClock> ClockBaseline;
        TSharedPtr<WorldObservationPrivate::FPublicationClock> RetiringClock;
        uint64 PublicationSerial = 0, OrderingVersion = 0;
        FWorldObservationToken PendingNotification;
        bool bInNotification = false;
        bool bRestartPublication = false;
        uint8 PublicationRestartPhase = 0;
        TSharedPtr<const FWorldTransportRead> Transport;
        FWorldDispatch Request;
        EWorldRequest Next = EWorldRequest::None, CaptureOperation = EWorldRequest::None;
        // Replacement preparation changes CaptureOperation, never this request's identity/kind.
        EWorldRequest RequestedOperation = EWorldRequest::None;
        FString RequestId, Capture, Preparation;
        FString Body;
        TSharedPtr<FJsonObject> DecodedBody;
        uint64 BodyCharge = 0;
        bool bOwnBodyPage = false;
        uint64 RequestIdentityCharge = 0;
        uint64 CaptureCharge = 0, PreparationCharge = 0;
        uint64 RequestSequence = 0, DispatchSerial = 0, OperationSequence = 0;
        uint64 AllocatedRequestSequence = 0, AllocatedOperationSequence = 0;
        uint64 ReadLaneEpoch = 0;
        bool bCommandSettled = false, bCohortActive = false;
        int64 RevisionHighWater = -1, MoveHighWater = -1;
        int64 HealthHighWater = -1, SavedHighWater = -1;
        // Terminal metadata preserves supplied absence/null without publishing a
        // synthetic health record or retaining the invalid candidate's payload.
        EWorldFieldPresence LostHealthPresence = EWorldFieldPresence::Missing;
        bool bLostHealthDecodedNull = false;
        int64 ReceiptExpiresAt = 0;
        // Fixed operation evidence survives payload retirement, never publishes.
        EWorldRequest SettledOperation = EWorldRequest::None;
        uint64 SettledRequestSequence = 0, SettledOperationSequence = 0;
        TCHAR SettledRequestId[33]{}, SettledCapture[33]{};
        bool bSettledAcknowledged = false;
        double TicketArrival = 0, CandidateDeadline = 0, CohortDeadline = 0, NextAt = 0, SentAt = 0, ReceivedAt = 0;
        double OriginalDispatchAt = 0;
        bool bAttached = false, bInFlight = false, bLostTicket = false, bPageRetried = false;
        bool bBindingValidated = false;
        bool bFinalBindComplete = false, bManifestBodyPending = false;
        EWorldRequest AfterRetirement = EWorldRequest::None;
        bool bAttemptActive = false, bReplacementUsed = false;
        uint8 ResultLookups = 0;
    };
}
