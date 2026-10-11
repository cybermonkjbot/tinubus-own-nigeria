#pragma once

#include "BaddieCompanionWorldObservationPages.h"
#include "BaddieCompanionMovementProtocol.h"

namespace BaddieCompanion
{
    enum class EWorldRequest : uint8
    { None, Enter, Prepare, Observe, Move, Ready, PresentationFailed, Result, Manifest, Page, Bind };
    enum class EWorldLane : uint8 { Read, Move, Control, None };
    enum class EWorldCommandOutcome : uint8
    { None, Accepted, SilentDecline, Superseded, Rejected, Busy, RequestConflict,
      OperationRejected, ResultExpired, Unknown, Expired, Cancelled, ResourceUnavailable };

    struct FWorldCommandScope
    {
        uint64 Lifetime = 0, Credential = 0, Consumer = 0, VisitSerial = 0;
        int32 Entry = 0;
        int64 Revision = -1;
        FWorldObservationToken Publication;
        // Exact admitted copies. No credentials or accepted-storage references.
        FString VisitId, LifeId, UserId, CharacterId, HomeId, GameRoomId, InstanceId;
        double Deadline = 0;
        uint64 Charge = 0;
        WorldObservationPrivate::EStep SameOwner(const FWorldCommandScope& Other,
            WorldObservationPrivate::FBudget& Budget, bool& bSame) const;
        bool Current(uint64 Life, uint64 Credentials, uint64 Visit, int32 EntrySequence, double Now) const;
        WorldObservationPrivate::EStep Retire(WorldObservationPrivate::FBudget& Budget);
    };

    struct FWorldMoveIntent
    {
        FWorldCommandScope Scope;
        FMovementPose Destination;
        FMovementPose Origin;
        EMovementPace Pace = EMovementPace::Unpaced;
        FMovementPoint Path[32]{};
        uint32 PathCount = 0;
        FMovementCollisionStop Collision;
        uint64 Motion = 0;
        int64 Sequence = 0;
        bool bSet = false, bStop = false, bPath = false, bCollision = false, bCompensation = false;
    };

    struct FWorldNativeEvidence
    {
        FWorldObservationToken Token;
        uint64 Consumer = 0, Content = 0, Binding = 0;
        bool bTrusted = false;
    };
    enum class EWorldPresentationPhase : uint8
    { Empty, ReadyUnsent, ReadySent, Acknowledged, RevokeWaiting, RevokeSent, RevokeSettled, Failed };
    struct FWorldPresentationCycle
    {
        FWorldNativeEvidence Evidence, Pending;
        FWorldCommandScope Scope;
        uint64 Serial = 0;
        EWorldPresentationPhase Phase = EWorldPresentationPhase::Empty;
        bool bLost = false, bReinspection = false;
    };
    struct FWorldPublicationTransfer
    {
        FWorldObservationToken Before, After;
        uint64 Consumer = 0, Cycle = 0, Content = 0, Binding = 0, Epoch = 0;
        bool bPrepared = false;
    };

    // Scheduling and fixed outcomes only. The world session owns accepted state,
    // exact scope capture, parsing and the charged retirement implementation.
    struct FWorldCommandLifecycle
    {
        FWorldMoveIntent Unsent, Stop;
        FWorldPresentationCycle Presentation;
        FWorldPublicationTransfer Transfer;
        uint64 Motion = 0, CoveredMotion = MAX_uint64, Epoch = 0;
        uint64 CoveredLifetime = 0, CoveredVisit = 0;
        int64 CoveredRevision = -1;
        int64 AllocatedMovement = 0, AcceptedMovement = 0;
        double NextOrdinaryAt = 0;
        EWorldCommandOutcome MoveOutcome = EWorldCommandOutcome::None;
        bool bMoveInFlight = false, bStopInFlight = false, bNeedObserve = false;
        bool bSuppressed = true, bReleaseRequired = true, bCompensationUsed = false;
        bool bCohortClosed = false, bFinalObserve = false, bDetached = false;

        bool AllocateMovement(int64& Out);
        void Seed(int64 Sequence);
        bool AdmitIntent(FWorldMoveIntent&& Intent);
        bool AdmitStop(FWorldMoveIntent&& Intent);
        void Receipt(double ReceivedAt);
        void Suppress();
        void TerminalClear();
        void SettleMove(uint64 Intent, int64 Requested, int64 Returned, bool bStop, EWorldCommandOutcome Outcome);
        bool MayCompensate(uint64 Intent, int64 Requested, bool bScopeLive) const;
        void LoseContent(FWorldObservationToken Token);
        bool Acknowledge(FWorldObservationToken Token, uint64 Content, bool bAcknowledged);
        void SettleReadyFailure();
        bool MayReady() const;
        bool MayRevoke() const;
        bool MayMove(double Now) const;
    };
}
