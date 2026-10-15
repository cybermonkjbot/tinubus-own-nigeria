#include "BaddieCompanionWorldCommandLifecycle.h"

#include <cmath>

namespace BaddieCompanion
{
    namespace WorldCommandPrivate
    {
        constexpr uint64 QualifiedScopeReleaseWork = 0;
        constexpr uint64 QualifiedScopeComparisonWork = 0;
        bool SameText(const FString& A, const FString& B)
        { return A.Equals(B, ESearchCase::CaseSensitive); }
    }
    WorldObservationPrivate::EStep FWorldCommandScope::SameOwner(const FWorldCommandScope& Other,
        WorldObservationPrivate::FBudget& Budget, bool& bSame) const
    {
        using namespace WorldObservationPrivate;
        bSame = false;
        const FString* A[] = {&VisitId, &LifeId, &UserId, &CharacterId, &HomeId, &GameRoomId, &InstanceId};
        const FString* B[] = {&Other.VisitId, &Other.LifeId, &Other.UserId, &Other.CharacterId,
            &Other.HomeId, &Other.GameRoomId, &Other.InstanceId};
        uint64 Work = WorldCommandPrivate::QualifiedScopeComparisonWork;
        for (uint32 I = 0; I < UE_ARRAY_COUNT(A); ++I)
            Work += (static_cast<uint64>(A[I]->Len()) + static_cast<uint64>(B[I]->Len())) * sizeof(TCHAR);
        if (!WorldCommandPrivate::QualifiedScopeComparisonWork || Work > TickWorkLimit)
            return EStep::ResourceUnavailable;
        if (!Budget.Spend(Work)) return Budget.bExhausted ? EStep::ResourceUnavailable : EStep::Pending;
        bSame = Lifetime == Other.Lifetime && Credential == Other.Credential
            && VisitSerial == Other.VisitSerial && Entry == Other.Entry && Revision == Other.Revision
            && WorldCommandPrivate::SameText(VisitId, Other.VisitId)
            && WorldCommandPrivate::SameText(LifeId, Other.LifeId)
            && WorldCommandPrivate::SameText(UserId, Other.UserId)
            && WorldCommandPrivate::SameText(CharacterId, Other.CharacterId)
            && WorldCommandPrivate::SameText(HomeId, Other.HomeId)
            && WorldCommandPrivate::SameText(GameRoomId, Other.GameRoomId)
            && WorldCommandPrivate::SameText(InstanceId, Other.InstanceId);
        return EStep::Complete;
    }
    bool FWorldCommandScope::Current(uint64 Life, uint64 Credentials, uint64 Visit,
        int32 EntrySequence, double Now) const
    {
        return Lifetime == Life && Credential == Credentials && VisitSerial == Visit
            && Entry == EntrySequence && std::isfinite(Now) && std::isfinite(Deadline) && Now < Deadline;
    }
    WorldObservationPrivate::EStep FWorldCommandScope::Retire(WorldObservationPrivate::FBudget& Budget)
    {
        using namespace WorldObservationPrivate;
        if (!Charge) return EStep::Complete;
        if (!WorldCommandPrivate::QualifiedScopeReleaseWork
            || WorldCommandPrivate::QualifiedScopeReleaseWork > TickWorkLimit) return EStep::ResourceUnavailable;
        FString* Fields[] = {&VisitId, &LifeId, &UserId, &CharacterId, &HomeId, &GameRoomId, &InstanceId};
        for (auto* Text : Fields)
        {
            if (Text->GetCharArray().IsEmpty()) continue;
            if (!Budget.Spend(WorldCommandPrivate::QualifiedScopeReleaseWork, false)) return EStep::Pending;
            Text->Empty();
        }
        Budget.Release(Charge); Charge = 0;
        return EStep::Complete;
    }
    bool FWorldCommandLifecycle::AllocateMovement(int64& Out)
    {
        Out = 0;
        const int64 High = FMath::Max(AllocatedMovement, AcceptedMovement);
        if (High >= 1000000000) { Suppress(); return false; }
        AllocatedMovement = High + 1; Out = AllocatedMovement; return true;
    }
    void FWorldCommandLifecycle::Seed(int64 Sequence)
    {
        if (Sequence < 0 || Sequence > 1000000000) return;
        AcceptedMovement = FMath::Max(AcceptedMovement, Sequence);
        AllocatedMovement = FMath::Max(AllocatedMovement, Sequence);
    }
    bool FWorldCommandLifecycle::AdmitIntent(FWorldMoveIntent&& Intent)
    {
        if (bReleaseRequired || bSuppressed || Motion == MAX_uint64 || Epoch == MAX_uint64 || Unsent.Scope.Charge
            || !AllocateMovement(Intent.Sequence)) return false;
        Intent.Motion = ++Motion; Intent.bSet = true; Intent.bStop = false;
        Unsent = MoveTemp(Intent); MoveOutcome = EWorldCommandOutcome::None;
        bCompensationUsed = false; return true;
    }
    bool FWorldCommandLifecycle::AdmitStop(FWorldMoveIntent&& Intent)
    {
        bSuppressed = true;
        if ((CoveredMotion == Motion && CoveredLifetime == Intent.Scope.Lifetime
            && CoveredVisit == Intent.Scope.VisitSerial && CoveredRevision == Intent.Scope.Revision)
            || Stop.bSet || bStopInFlight || Stop.Scope.Charge || Epoch == MAX_uint64
            || !AllocateMovement(Intent.Sequence)) return false;
        Intent.Motion = Motion; Intent.bSet = Intent.bStop = true;
        Stop = MoveTemp(Intent); CoveredMotion = Motion;
        CoveredLifetime = Stop.Scope.Lifetime; CoveredVisit = Stop.Scope.VisitSerial;
        CoveredRevision = Stop.Scope.Revision;
        if (Epoch != MAX_uint64) ++Epoch;
        bCohortClosed = true; return true;
    }
    void FWorldCommandLifecycle::Receipt(double ReceivedAt)
    {
        if (std::isfinite(ReceivedAt) && ReceivedAt > 0)
            NextOrdinaryAt = FMath::Max(NextOrdinaryAt, ReceivedAt + 0.250);
    }
    void FWorldCommandLifecycle::Suppress()
    {
        bSuppressed = bReleaseRequired = true;
        Unsent.bSet = false;
    }
    void FWorldCommandLifecycle::TerminalClear()
    {
        Suppress(); Stop.bSet = false;
        bMoveInFlight = bStopInFlight = bNeedObserve = false;
        Presentation.bLost = true; Presentation.Phase = EWorldPresentationPhase::Empty;
        Presentation.Evidence = {}; Presentation.Pending = {}; Transfer = {};
        bCohortClosed = true; bFinalObserve = bDetached = false;
        CoveredMotion = MAX_uint64; CoveredLifetime = CoveredVisit = 0; CoveredRevision = -1;
        if (Epoch != MAX_uint64) ++Epoch;
    }
    void FWorldCommandLifecycle::SettleMove(uint64 Intent, int64 Requested, int64 Returned,
        bool bStop, EWorldCommandOutcome Outcome)
    {
        if (Returned < Requested && Outcome == EWorldCommandOutcome::Accepted)
            Outcome = AcceptedMovement >= Requested ? EWorldCommandOutcome::Superseded : EWorldCommandOutcome::SilentDecline;
        if (Returned >= Requested && Outcome == EWorldCommandOutcome::Accepted) Seed(Returned);
        if (bStop) bStopInFlight = false; else bMoveInFlight = false;
        bNeedObserve = true;
        if (Intent != Motion || AcceptedMovement > Requested) return;
        MoveOutcome = Outcome;
        if (Outcome != EWorldCommandOutcome::Accepted) Suppress();
    }
    bool FWorldCommandLifecycle::MayCompensate(uint64 Intent, int64 Requested, bool bScopeLive) const
    {
        return bScopeLive && Intent == Motion && AcceptedMovement < Requested
            && !bCompensationUsed && CoveredMotion != Intent && !Stop.bSet && !bStopInFlight;
    }
    void FWorldCommandLifecycle::LoseContent(FWorldObservationToken Token)
    {
        if (!Token.IsSet() || Presentation.Evidence.Token != Token || Presentation.bLost) return;
        Suppress(); Transfer = {}; Presentation.bLost = true;
        Presentation.Pending = {};
        if (Presentation.Phase == EWorldPresentationPhase::ReadyUnsent)
            Presentation.Phase = EWorldPresentationPhase::Empty;
        else if (Presentation.Phase == EWorldPresentationPhase::Acknowledged)
            Presentation.Phase = EWorldPresentationPhase::RevokeWaiting;
        if (Epoch != MAX_uint64) ++Epoch;
    }
    bool FWorldCommandLifecycle::Acknowledge(FWorldObservationToken Token, uint64 Content, bool bAcknowledged)
    {
        auto& Cycle = Presentation;
        if (Cycle.Phase != EWorldPresentationPhase::ReadySent) return false;
        if (Cycle.bLost) { Cycle.Phase = EWorldPresentationPhase::RevokeWaiting; return false; }
        if (Cycle.Evidence.Token != Token || Cycle.Evidence.Content != Content || !Cycle.Evidence.bTrusted)
        { Cycle.Phase = EWorldPresentationPhase::Failed; return false; }
        Cycle.Phase = bAcknowledged ? EWorldPresentationPhase::Acknowledged : EWorldPresentationPhase::Failed;
        return bAcknowledged;
    }
    void FWorldCommandLifecycle::SettleReadyFailure()
    {
        if (Presentation.Phase != EWorldPresentationPhase::ReadySent) return;
        Presentation.Phase = Presentation.bLost ? EWorldPresentationPhase::RevokeWaiting
            : EWorldPresentationPhase::Failed;
    }
    bool FWorldCommandLifecycle::MayReady() const
    {
        return !bMoveInFlight && !bStopInFlight && !Stop.bSet && !Unsent.bSet
            && Presentation.Phase == EWorldPresentationPhase::ReadyUnsent
            && Presentation.Evidence.bTrusted && !Presentation.bLost;
    }
    bool FWorldCommandLifecycle::MayRevoke() const
    { return !bStopInFlight && !Stop.bSet && Presentation.Phase == EWorldPresentationPhase::RevokeWaiting; }
    bool FWorldCommandLifecycle::MayMove(double Now) const
    {
        return std::isfinite(Now) && Now >= NextOrdinaryAt && Unsent.bSet
            && !bMoveInFlight && !bStopInFlight && !Stop.bSet && !bSuppressed && !bReleaseRequired
            && !bCohortClosed && Presentation.Phase == EWorldPresentationPhase::Acknowledged;
    }
}
