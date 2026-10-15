#include "BaddieCompanionResidentPopulation.h"

#include "HAL/PlatformTime.h"
#include <cmath>

namespace BaddieCompanion
{
    namespace ResidentPopulationPrivate
    {
        using namespace WorldObservationPrivate;
        // Complete allocator/control-block and operation bounds are unqualified.
        constexpr uint64 QualifiedOwnerBytes = 0, QualifiedOwnerInitWork = 0;
        constexpr uint64 QualifiedOwnerDeleteWork = 0, QualifiedDetachWork = 0;
        constexpr uint64 QualifiedScopeBytes = 0, QualifiedScopeWork = 0, QualifiedScopeReleaseWork = 0;
        constexpr uint64 QualifiedReceiptWork = 0, QualifiedPopulationBytes = 0;
        constexpr uint64 QualifiedTransferWork = 0, QualifiedPopulationReleaseWork = 0;
        constexpr uint64 QualifiedPreparationWork = 0, QualifiedCommitWork = 0;
        constexpr uint64 QualifiedWakeWork = 0, QualifiedAttemptWork = 0;
        constexpr uint64 QualifiedBorrowWork = 0, QualifiedBorrowReleaseWork = 0;
        constexpr uint64 QualifiedRetireWork = 0;
        constexpr int64 SafeInteger = 9007199254740991;

        bool Live(double Deadline)
        {
            const double Now = FPlatformTime::Seconds();
            return std::isfinite(Now) && std::isfinite(Deadline) && Now >= 0 && Now < Deadline;
        }
        bool TimeValid(const FWorldPresentationTime& Time)
        {
            return std::isfinite(Time.ServerNowMs) && Time.ServerNowMs > 0
                && Time.ServerNowMs <= SafeInteger && Time.RemainingMs > 0 && Time.RemainingMs <= 60000;
        }
        bool Next(uint64& Serial)
        {
            if (Serial == MAX_uint64) return false;
            ++Serial;
            return true;
        }
        bool TokenAfter(FWorldObservationToken Old, FWorldObservationToken NextToken)
        {
            return NextToken.IsSet() && (!Old.IsSet()
                || (NextToken.Lifetime == Old.Lifetime && NextToken.Publication > Old.Publication));
        }
    }

    using namespace ResidentPopulationPrivate;

    struct FResidentPopulation::FData
    {
        FBudget* Budget = nullptr;
        FString Room;
        uint64 Account = 0, Access = 0, Entry = 0, RoomGeneration = 0, Subscription = 0, Visit = 0;
        uint64 ScopeSerial = 0, ReadSerial = 0, DecodeSerial = 0, OperationSerial = 0;
        uint64 Version = 0, ChangeSerial = 0;
        uint64 AttemptedSerial = 0;
        uint64 DispatchSerial = 0;
        double DeadlineSeconds = 0;
        int32 RetirementSlot = INDEX_NONE;
        uint32 Borrows = 0;
        uint64 ScopeCharge = 0, CurrentCharge = 0, CandidateCharge = 0, RetiredCharge = 0;
        TSharedPtr<const FResidentWorldView> Population, Candidate, Retired;
        FWorldObservationToken Token, TokenHighWater;
        FPreparedChange Pending;
        FAttemptReceipt Operation;
        TOptional<int64> DeadlineMs, AttemptedMs;
        EPresence Presence = EPresence::Unavailable;
        EError Error = EError::None;
        ERetirement RetirementReason = ERetirement::PlayerAbsent;
        bool bSelected = false, bReadable = false, bDisposed = false, bDecodeReserved = false;
        bool bScopeRetiring = false;
    };

    bool FResidentPopulation::Qualified()
    {
        using namespace ResidentPopulationPrivate;
        const uint64 Work[] = {QualifiedOwnerInitWork, QualifiedOwnerDeleteWork, QualifiedDetachWork,
            QualifiedScopeWork, QualifiedScopeReleaseWork, QualifiedReceiptWork, QualifiedTransferWork,
            QualifiedPopulationReleaseWork, QualifiedPreparationWork, QualifiedCommitWork, QualifiedWakeWork,
            QualifiedAttemptWork, QualifiedBorrowWork, QualifiedBorrowReleaseWork, QualifiedRetireWork};
        if (!WorldObservationPrivate::FPages::ResourcesQualified()
            || !QualifiedOwnerBytes || !QualifiedScopeBytes || !QualifiedPopulationBytes
            || QualifiedOwnerBytes > WorldObservationPrivate::RetainedLimit
            || QualifiedScopeBytes > WorldObservationPrivate::RetainedLimit
            || QualifiedPopulationBytes > WorldObservationPrivate::RetainedLimit
            || !WorldObservationPrivate::QualifiedRetirementDeleteWork
            || WorldObservationPrivate::QualifiedRetirementDeleteWork > WorldObservationPrivate::TickWorkLimit) return false;
        for (uint64 Cost : Work) if (!Cost || Cost > WorldObservationPrivate::TickWorkLimit) return false;
        return QualifiedOwnerInitWork <= WorldObservationPrivate::TickWorkLimit - QualifiedDetachWork
            && QualifiedBorrowWork <= WorldObservationPrivate::TickWorkLimit - QualifiedBorrowReleaseWork
            && QualifiedPreparationWork <= WorldObservationPrivate::TickWorkLimit - QualifiedTransferWork
            && QualifiedPopulationReleaseWork <= WorldObservationPrivate::TickWorkLimit - QualifiedRetireWork
            && QualifiedScopeReleaseWork <= WorldObservationPrivate::TickWorkLimit - QualifiedRetireWork
            && QualifiedOwnerDeleteWork <= WorldObservationPrivate::TickWorkLimit - QualifiedRetireWork
            && WorldObservationPrivate::QualifiedRetirementDeleteWork
                <= WorldObservationPrivate::TickWorkLimit - QualifiedRetireWork - QualifiedOwnerDeleteWork;
    }
    FResidentPopulation::EStep FResidentPopulation::Admit(FData& Owner, uint64 Work, bool bCandidate)
    {
        if (!Qualified() || !Owner.Budget || !Work || Work > WorldObservationPrivate::TickWorkLimit)
            return EStep::ResourceUnavailable;
        if (!Owner.Budget->Spend(Work, bCandidate))
            return Owner.Budget->bExhausted ? EStep::ResourceUnavailable : EStep::Pending;
        return EStep::Complete;
    }
    FResidentPopulation::~FResidentPopulation() { Dispose(); }
    FResidentPopulation::EStep FResidentPopulation::Initialize(FBudget& Budget)
    {
        check(IsInGameThread());
        using namespace ResidentPopulationPrivate;
        if (Data || bDisposed) return EStep::Invalid;
        if (!Qualified() || !Budget.Retirement) return EStep::ResourceUnavailable;
        // Detach/queue work is prepaid so revocation never waits on a later frame.
        if (QualifiedOwnerInitWork + QualifiedDetachWork > WorldObservationPrivate::TickWorkLimit - 64)
            return EStep::ResourceUnavailable;
        if (!Budget.Spend(QualifiedOwnerInitWork + QualifiedDetachWork, false))
            return Budget.bExhausted ? EStep::ResourceUnavailable : EStep::Pending;
        if (!Budget.Reserve(QualifiedOwnerBytes)) return EStep::ResourceUnavailable;
        const int32 Slot = WorldObservationPrivate::ReserveRetirementTask(Budget);
        if (Slot == INDEX_NONE) { Budget.Release(QualifiedOwnerBytes); return EStep::Pending; }
        Data = new FData;
        Data->Budget = &Budget;
        Data->RetirementSlot = Slot;
        return EStep::Complete;
    }
    bool FResidentPopulation::Current(const FData& Owner, const FScopeReceipt& Scope)
    {
        return Owner.bSelected && !Owner.bDisposed && Live(Owner.DeadlineSeconds)
            && Live(Scope.DeadlineSeconds) && Scope.Room && Scope.Room->Len() > 0 && Scope.Room->Len() <= 256
            && Owner.Account == Scope.Account && Owner.Access == Scope.Access && Owner.Entry == Scope.Entry
            && Owner.RoomGeneration == Scope.RoomGeneration && Owner.Subscription == Scope.Subscription
            && Owner.Visit == Scope.Visit && Owner.Room.Equals(*Scope.Room, ESearchCase::CaseSensitive);
    }
    bool FResidentPopulation::Current(const FData& Owner, const FReadReceipt& Read)
    {
        return Owner.bSelected && !Owner.bDisposed && Live(Owner.DeadlineSeconds) && Live(Read.DeadlineSeconds)
            && Read.Scope && Owner.ScopeSerial == Read.Scope && Owner.Visit == Read.Visit
            && Read.Account == Owner.Account && Read.Access == Owner.Access && Read.Entry == Owner.Entry
            && Read.RoomGeneration == Owner.RoomGeneration && Read.Subscription == Owner.Subscription
            && Read.Serial && Owner.ReadSerial == Read.Serial;
    }
    bool FResidentPopulation::Current(const FData& Owner, const FAttemptReceipt& Attempt)
    {
        return Owner.bSelected && !Owner.bDisposed && Live(Owner.DeadlineSeconds)
            && Live(Attempt.PermissionDeadlineSeconds) && Attempt.Scope == Owner.ScopeSerial
            && Attempt.Visit == Owner.Visit && Attempt.Serial && Attempt.Serial == Owner.Operation.Serial
            && Attempt.Account == Owner.Account && Attempt.Access == Owner.Access && Attempt.Entry == Owner.Entry
            && Attempt.RoomGeneration == Owner.RoomGeneration && Attempt.Subscription == Owner.Subscription
            && Attempt.DispatchSerial == Owner.Operation.DispatchSerial
            && Attempt.DeadlineMs == Owner.Operation.DeadlineMs;
    }
    FResidentPopulation::EStep FResidentPopulation::SelectScope(const FScopeReceipt& Scope, EError& PreviousError)
    {
        check(IsInGameThread());
        PreviousError = Data ? Data->Error : EError::None;
        using namespace ResidentPopulationPrivate;
        if (!Data || Data->bDisposed) return EStep::ResourceUnavailable;
        auto Result = Admit(*Data, QualifiedScopeWork);
        if (Result != EStep::Complete) return Result;
        if (!Scope.Account || !Scope.Access || !Scope.Entry || !Scope.RoomGeneration || !Scope.Subscription
            || !Scope.Visit || !Scope.Room || Scope.Room->IsEmpty() || Scope.Room->Len() > 256
            || !Live(Scope.DeadlineSeconds)) return EStep::Invalid;
        const uint64 Old[] = {Data->Account, Data->Access, Data->Entry, Data->RoomGeneration, Data->Subscription};
        const uint64 New[] = {Scope.Account, Scope.Access, Scope.Entry, Scope.RoomGeneration, Scope.Subscription};
        int32 Order = 0;
        for (uint32 I = 0; I < UE_ARRAY_COUNT(Old); ++I)
            if (Old[I] != New[I]) { Order = New[I] > Old[I] ? 1 : -1; break; }
        if (Order < 0) return EStep::Invalid;
        if (Order == 0)
        {
            if (!Live(Data->DeadlineSeconds))
            { RetireScope(ERetirement::AccessLost); return EStep::Invalid; }
            if (!Data->bSelected || !Data->Room.Equals(*Scope.Room, ESearchCase::CaseSensitive)
                || Scope.Visit < Data->Visit || Data->Borrows || Data->Pending.Change != EChange::None)
                return EStep::Invalid;
            if (Scope.Visit != Data->Visit)
            {
                if (!Next(Data->Version)) { RetireScope(ERetirement::AccessLost); return EStep::Invalid; }
                Data->Visit = Scope.Visit;
                Data->Operation = {}; // Keep the attempted key, reject old visit completions.
                Data->Token = {}; Data->bReadable = false;
            }
            Data->DeadlineSeconds = Scope.DeadlineSeconds;
            return EStep::Complete;
        }
        if (Data->bSelected) RetireScope(ERetirement::RoomChanged);
        Result = AdvanceRetirement();
        if (Result != EStep::Complete) return Result;
        if (Data->bDecodeReserved || Data->Borrows || Data->Population || Data->Candidate || Data->Retired)
            return EStep::Pending;
        if (Data->ScopeSerial == MAX_uint64 || Data->Version == MAX_uint64)
        { RetireScope(ERetirement::AccessLost); return EStep::Invalid; }
        if (!Data->Budget->Reserve(QualifiedScopeBytes)) return EStep::ResourceUnavailable;
        Data->Room = *Scope.Room;
        Data->ScopeCharge = QualifiedScopeBytes;
        Data->Account = Scope.Account; Data->Access = Scope.Access; Data->Entry = Scope.Entry;
        Data->RoomGeneration = Scope.RoomGeneration; Data->Subscription = Scope.Subscription; Data->Visit = Scope.Visit;
        Data->DeadlineSeconds = Scope.DeadlineSeconds;
        ++Data->ScopeSerial; ++Data->Version;
        Data->Presence = EPresence::Unavailable; Data->Error = EError::None;
        Data->DeadlineMs.Reset(); Data->AttemptedMs.Reset(); Data->Operation = {};
        Data->AttemptedSerial = 0;
        Data->TokenHighWater = {};
        Data->bSelected = true; Data->bScopeRetiring = false;
        return EStep::Complete;
    }
    FResidentPopulation::EError FResidentPopulation::Revoke(FData& Owner, ERetirement Reason)
    {
        const auto PreviousError = Owner.Error;
        Owner.bReadable = false; Owner.Token = {}; Owner.bSelected = false;
        Owner.bScopeRetiring = true; Owner.RetirementReason = Reason;
        Owner.Pending = {}; Owner.Operation = {};
        Owner.Presence = EPresence::Unavailable; Owner.Error = EError::None;
        Owner.DeadlineMs.Reset(); Owner.AttemptedMs.Reset(); Owner.DeadlineSeconds = 0;
        Owner.AttemptedSerial = 0;
        return PreviousError;
    }
    FResidentPopulation::EError FResidentPopulation::RetireScope(ERetirement Reason)
    {
        check(IsInGameThread());
        // Owning pointers and Room remain charged until AdvanceRetirement.
        return Data ? Revoke(*Data, Reason) : EError::None;
    }
    void FResidentPopulation::Dispose()
    {
        check(IsInGameThread());
        bDisposed = true;
        if (!Data) return;
        RetireScope(ERetirement::Disposed);
        Data->bDisposed = true;
        WorldObservationPrivate::QueueRetirementTask(*Data->Budget, Data->RetirementSlot,
            {Data, &RetireData, &DeleteData, ResidentPopulationPrivate::QualifiedOwnerBytes});
        Data = nullptr;
    }
    FResidentPopulation::EStep FResidentPopulation::SetObservation(const FScopeReceipt& Scope, FWorldObservationToken Token)
    {
        check(IsInGameThread());
        if (!Data) return EStep::ResourceUnavailable;
        auto Result = Admit(*Data, ResidentPopulationPrivate::QualifiedCommitWork);
        if (Result != EStep::Complete) return Result;
        if (!Current(*Data, Scope) || Data->Borrows || Data->Pending.Change != EChange::None) return EStep::Invalid;
        if (Token.IsSet() && !ResidentPopulationPrivate::TokenAfter(Data->TokenHighWater, Token)) return EStep::Invalid;
        if (!ResidentPopulationPrivate::Next(Data->Version)) { RetireScope(ERetirement::AccessLost); return EStep::Invalid; }
        Data->bReadable = false; Data->Token = Token;
        if (Token.IsSet()) Data->TokenHighWater = Token;
        Data->bReadable = Token.IsSet();
        return EStep::Complete;
    }
    FResidentPopulation::EStep FResidentPopulation::CaptureRead(const FScopeReceipt& Scope, FReadReceipt& Out)
    {
        check(IsInGameThread()); Out = {};
        if (!Data) return EStep::ResourceUnavailable;
        auto Result = Admit(*Data, ResidentPopulationPrivate::QualifiedReceiptWork);
        if (Result != EStep::Complete) return Result;
        if (!Current(*Data, Scope)) return EStep::Invalid;
        if (!ResidentPopulationPrivate::Next(Data->ReadSerial)) { RetireScope(ERetirement::AccessLost); return EStep::Invalid; }
        Out.Scope = Data->ScopeSerial; Out.Visit = Data->Visit; Out.Serial = Data->ReadSerial;
        Out.Account = Data->Account; Out.Access = Data->Access; Out.Entry = Data->Entry;
        Out.RoomGeneration = Data->RoomGeneration; Out.Subscription = Data->Subscription;
        Out.DeadlineSeconds = FMath::Min(Scope.DeadlineSeconds, Data->DeadlineSeconds);
        return EStep::Complete;
    }
    FResidentPopulation::EStep FResidentPopulation::ReserveDecoded(const FReadReceipt& Read, FDecodedReservation& Out)
    {
        check(IsInGameThread());
        if (Out.Owner) return EStep::Invalid;
        if (!Data) return EStep::ResourceUnavailable;
        auto Result = Admit(*Data, ResidentPopulationPrivate::QualifiedTransferWork, true);
        if (Result != EStep::Complete) return Result;
        if (!Current(*Data, Read)) return EStep::Invalid;
        if (Data->bDecodeReserved || Data->Candidate || Data->Retired || Data->Pending.Change != EChange::None)
            return EStep::Pending;
        if (!ResidentPopulationPrivate::Next(Data->DecodeSerial)) { RetireScope(ERetirement::AccessLost); return EStep::Invalid; }
        if (!Data->Budget->Reserve(ResidentPopulationPrivate::QualifiedPopulationBytes)) return EStep::ResourceUnavailable;
        Data->CandidateCharge = ResidentPopulationPrivate::QualifiedPopulationBytes;
        Data->bDecodeReserved = true;
        Out.Owner = Data; Out.Serial = Data->DecodeSerial; Out.Read = Read;
        return EStep::Complete;
    }
    FResidentPopulation::EStep FResidentPopulation::Prepare(FData& Owner, EChange Kind,
        EPresence Presence, EError Error, FPreparedChange& Out)
    {
        if (Owner.Pending.Change != EChange::None || Owner.Borrows || Owner.Retired) return EStep::Pending;
        auto Result = Admit(Owner, ResidentPopulationPrivate::QualifiedPreparationWork, true);
        if (Result != EStep::Complete) return Result;
        if (Owner.Version == MAX_uint64 || !ResidentPopulationPrivate::Next(Owner.ChangeSerial))
        { Revoke(Owner, ERetirement::AccessLost); return EStep::Invalid; }
        Out = {}; Out.Scope = Owner.ScopeSerial; Out.Version = Owner.Version; Out.Serial = Owner.ChangeSerial;
        Out.Change = Kind; Out.Presence = Presence; Out.PreviousError = Owner.Error; Out.Error = Error;
        Out.Visit = Owner.Visit; Out.DeadlineSeconds = Owner.DeadlineSeconds;
        Out.Account = Owner.Account; Out.Access = Owner.Access; Out.Entry = Owner.Entry;
        Out.RoomGeneration = Owner.RoomGeneration; Out.Subscription = Owner.Subscription;
        Out.DeadlineMs = Owner.DeadlineMs;
        Owner.Pending = Out;
        return EStep::Complete;
    }
    FResidentPopulation::EStep FResidentPopulation::PrepareRead(FDecodedReservation& Reservation,
        TSharedPtr<const FResidentWorldView>& Decoded, FPreparedChange& Out)
    {
        check(IsInGameThread()); Out = {};
        if (!Data || Reservation.Owner != Data || !Data->bDecodeReserved || Reservation.Serial != Data->DecodeSerial)
            return EStep::Invalid;
        if (!Current(*Data, Reservation.Read)) return RejectDecoded(Reservation, Decoded);
        if (!Decoded || Decoded.GetSharedReferenceCount() != 1) return EStep::Invalid;
        auto Transfer = Admit(*Data, ResidentPopulationPrivate::QualifiedTransferWork, true);
        if (Transfer != EStep::Complete) return Transfer;
        auto Result = Prepare(*Data, EChange::Read, EPresence::Value, EError::None, Out);
        if (Result != EStep::Complete) return Result;
        const auto& View = *Decoded;
        // Structural decoding belongs to the immutable decoder, not this owner.
        if (View.WorldKey.Len() > 256 || View.Clock.EpochKey.Len() > 256)
        { Data->Pending = {}; Out = {}; return EStep::Invalid; }
        Out.DeadlineMs = View.NextAdvanceAtRealMs;
        Out.ReadSerial = Reservation.Read.Serial;
        Out.DeadlineSeconds = Reservation.Read.DeadlineSeconds;
        for (int32 I = 0; I < View.WorldKey.Len(); ++I) Out.WorldKey[I] = View.WorldKey[I];
        for (int32 I = 0; I < View.Clock.EpochKey.Len(); ++I) Out.EpochKey[I] = View.Clock.EpochKey[I];
        Data->Candidate = MoveTemp(Decoded);
        Data->bDecodeReserved = false;
        Reservation.Owner = nullptr; Reservation.Serial = 0; Reservation.Read = {};
        Data->Pending = Out;
        return EStep::Complete;
    }
    FResidentPopulation::EStep FResidentPopulation::RejectDecoded(FDecodedReservation& Reservation,
        TSharedPtr<const FResidentWorldView>& Decoded)
    {
        check(IsInGameThread());
        auto* Owner = Reservation.Owner;
        if (!Owner || !Owner->bDecodeReserved || Reservation.Serial != Owner->DecodeSerial) return EStep::Invalid;
        auto Result = Admit(*Owner, ResidentPopulationPrivate::QualifiedTransferWork);
        if (Result != EStep::Complete) return Result;
        if (Owner->Retired || (Decoded && Decoded.GetSharedReferenceCount() != 1)) return EStep::Pending;
        if (Decoded)
        { Owner->Retired = MoveTemp(Decoded); Owner->RetiredCharge = Owner->CandidateCharge; }
        else Owner->Budget->Release(Owner->CandidateCharge);
        Owner->CandidateCharge = 0; Owner->bDecodeReserved = false;
        Reservation.Owner = nullptr; Reservation.Serial = 0; Reservation.Read = {};
        return EStep::Invalid;
    }
    FResidentPopulation::FDecodedReservation::~FDecodedReservation()
    {
        // Explicit return prevents a callback stack from deleting charged data
        // or abandoning the reservation which pins its retirement task.
        check(!Owner);
    }
    FResidentPopulation::EStep FResidentPopulation::PrepareNull(const FReadReceipt& Read, FPreparedChange& Out)
    {
        check(IsInGameThread()); Out = {};
        if (!Data) return EStep::ResourceUnavailable;
        if (!Current(*Data, Read)) return EStep::Invalid;
        if (Data->bDecodeReserved || Data->Candidate) return EStep::Pending;
        auto Result = Prepare(*Data, EChange::Read, EPresence::KnownNull, EError::None, Out);
        if (Result == EStep::Complete)
        { Out.DeadlineMs.Reset(); Out.ReadSerial = Read.Serial; Out.DeadlineSeconds = Read.DeadlineSeconds; Data->Pending = Out; }
        return Result;
    }
    FResidentPopulation::EStep FResidentPopulation::PrepareFailure(const FReadReceipt& Read, EError Error, FPreparedChange& Out)
    {
        check(IsInGameThread()); Out = {};
        if (!Data) return EStep::ResourceUnavailable;
        if (!Current(*Data, Read)) return EStep::Invalid;
        if (Error != EError::ReadFailed && Error != EError::AccessLost) return EStep::Invalid;
        if (Data->bDecodeReserved || Data->Candidate) return EStep::Pending;
        auto Result = Prepare(*Data, Error == EError::AccessLost ? EChange::AccessLoss : EChange::ReadFailure,
            EPresence::Unavailable, Error, Out);
        if (Result == EStep::Complete)
        { Out.DeadlineMs.Reset(); Out.ReadSerial = Read.Serial; Out.DeadlineSeconds = Read.DeadlineSeconds; Data->Pending = Out; }
        return Result;
    }
    FResidentPopulation::EStep FResidentPopulation::Commit(const FPreparedChange& Change, FWorldObservationToken NextToken)
    {
        check(IsInGameThread());
        if (!Data) return EStep::ResourceUnavailable;
        auto Result = Admit(*Data, ResidentPopulationPrivate::QualifiedCommitWork);
        if (Result != EStep::Complete) return Result;
        if (!Data->bSelected || !Live(Data->DeadlineSeconds) || Data->Borrows || Data->Retired
            || Change.Change == EChange::None || Change.Scope != Data->ScopeSerial || Change.Version != Data->Version
            || Change.Serial != Data->Pending.Serial || Change.Change != Data->Pending.Change)
            return EStep::Invalid;
        if (Change.Account != Data->Account || Change.Access != Data->Access || Change.Entry != Data->Entry
            || Change.RoomGeneration != Data->RoomGeneration || Change.Subscription != Data->Subscription)
            return EStep::Invalid;
        const auto& Prepared = Data->Pending;
        if (!Live(Prepared.DeadlineSeconds) || Prepared.Visit != Data->Visit
            || (Prepared.ReadSerial && Prepared.ReadSerial != Data->ReadSerial)
            || (Prepared.OperationSerial && Prepared.OperationSerial != Data->Operation.Serial)) return EStep::Invalid;
        if (Change.Change != EChange::AccessLoss
            && !ResidentPopulationPrivate::TokenAfter(Data->TokenHighWater, NextToken)) return EStep::Invalid;
        if (Data->Version == MAX_uint64) { RetireScope(ERetirement::AccessLost); return EStep::Invalid; }
        // Parent must have prepared its aggregate errors/conflict masks already.
        Data->bReadable = false; Data->Token = {};
        const auto Pending = Data->Pending;
        if (Pending.Change == EChange::AccessLoss)
        { RetireScope(ERetirement::AccessLost); return EStep::Complete; }
        if (Pending.Change == EChange::Read || Pending.Change == EChange::ReadFailure)
        {
            Data->Retired = MoveTemp(Data->Population); Data->RetiredCharge = Data->CurrentCharge; Data->CurrentCharge = 0;
            Data->Population = MoveTemp(Data->Candidate); Data->CurrentCharge = Data->CandidateCharge; Data->CandidateCharge = 0;
            // Read failure removes availability, not the attempted deadline history.
            if (Pending.Change == EChange::Read && (Pending.DeadlineMs != Data->AttemptedMs || Pending.Presence == EPresence::KnownNull))
            { Data->AttemptedMs.Reset(); Data->AttemptedSerial = 0; }
            Data->DeadlineMs = Pending.DeadlineMs;
        }
        if (Pending.Change == EChange::AdvanceSuccess || Pending.Change == EChange::AdvanceFailure)
        {
            if (Pending.Change == EChange::AdvanceSuccess && Pending.ServerNowMs < Data->Operation.DeadlineMs
                && Data->AttemptedMs.IsSet() && Data->AttemptedMs.GetValue() == Data->Operation.DeadlineMs
                && Data->AttemptedSerial == Data->Operation.Serial)
            { Data->AttemptedMs.Reset(); Data->AttemptedSerial = 0; }
            Data->Operation = {};
        }
        Data->Presence = Pending.Presence; Data->Error = Pending.Error;
        ++Data->Version; Data->Pending = {};
        Data->Token = Data->TokenHighWater = NextToken; Data->bReadable = true;
        return EStep::Complete;
    }
    FResidentPopulation::EStep FResidentPopulation::CancelPrepared()
    {
        check(IsInGameThread());
        if (!Data) return EStep::ResourceUnavailable;
        auto Result = Admit(*Data, ResidentPopulationPrivate::QualifiedTransferWork);
        if (Result != EStep::Complete) return Result;
        if (Data->Retired) return EStep::Pending;
        if (Data->Candidate)
        { Data->Retired = MoveTemp(Data->Candidate); Data->RetiredCharge = Data->CandidateCharge; Data->CandidateCharge = 0; }
        Data->Pending = {};
        return EStep::Complete;
    }
    FResidentPopulation::EStep FResidentPopulation::ProposeWake(const FDispatchReceipt& Dispatch,
        const FWorldObservation& Rendered, const FWorldPresentationTime& Time, FWakeProposal& Out) const
    {
        check(IsInGameThread()); Out = {};
        if (!Data) return EStep::ResourceUnavailable;
        auto Result = Admit(*Data, ResidentPopulationPrivate::QualifiedWakeWork);
        if (Result != EStep::Complete) return Result;
        if (!Current(*Data, Dispatch.Scope) || !Dispatch.Serial || Dispatch.Serial <= Data->DispatchSerial
            || !Live(Dispatch.DeadlineSeconds) || !Data->bReadable || Data->Presence != EPresence::Value
            || !Data->DeadlineMs.IsSet() || !ResidentPopulationPrivate::TimeValid(Time)
            || !Rendered.IsCurrent() || Rendered.Token() != Data->Token) return EStep::Invalid;
        if (Data->Operation.Serial || Data->Pending.Change != EChange::None
            || Data->AttemptedMs == Data->DeadlineMs) return EStep::Pending;
        Out.Scope = Data->ScopeSerial; Out.Version = Data->Version;
        Out.DispatchSerial = Dispatch.Serial;
        Out.DeadlineMs = Data->DeadlineMs.GetValue(); Out.RenderToken = Rendered.Token();
        return EStep::Complete;
    }
    FResidentPopulation::EStep FResidentPopulation::BeginAttempt(const FDispatchReceipt& Dispatch,
        const FWakeProposal& Proposal, const FWorldObservation& Rendered,
        const FWorldPresentationTime& Time, FAttemptReceipt& Out)
    {
        check(IsInGameThread()); Out = {};
        FWakeProposal CurrentProposal;
        auto Result = ProposeWake(Dispatch, Rendered, Time, CurrentProposal);
        if (Result != EStep::Complete) return Result;
        Result = Admit(*Data, ResidentPopulationPrivate::QualifiedAttemptWork);
        if (Result != EStep::Complete) return Result;
        if (Proposal.Scope != CurrentProposal.Scope || Proposal.Version != CurrentProposal.Version
            || Proposal.DispatchSerial != CurrentProposal.DispatchSerial
            || Proposal.DeadlineMs != CurrentProposal.DeadlineMs || Proposal.RenderToken != CurrentProposal.RenderToken
            || Time.ServerNowMs < Proposal.DeadlineMs) return EStep::Invalid;
        if (!ResidentPopulationPrivate::Next(Data->OperationSerial)) { RetireScope(ERetirement::AccessLost); return EStep::Invalid; }
        Data->AttemptedMs = Proposal.DeadlineMs; // Before the parent can dispatch/reenter.
        Data->AttemptedSerial = Data->OperationSerial;
        Data->DispatchSerial = Dispatch.Serial;
        Out.Scope = Data->ScopeSerial; Out.Visit = Data->Visit; Out.Serial = Data->OperationSerial;
        Out.Account = Data->Account; Out.Access = Data->Access; Out.Entry = Data->Entry;
        Out.RoomGeneration = Data->RoomGeneration; Out.Subscription = Data->Subscription;
        Out.DeadlineMs = Proposal.DeadlineMs;
        Out.DispatchSerial = Dispatch.Serial;
        Out.PermissionDeadlineSeconds = FMath::Min(Dispatch.DeadlineSeconds,
            FMath::Min(Dispatch.Scope.DeadlineSeconds, Data->DeadlineSeconds));
        Data->Operation = Out;
        return EStep::Complete;
    }
    FResidentPopulation::EStep FResidentPopulation::CompleteAttempt(const FAttemptReceipt& Attempt,
        int64 ServerNowMs, EError Error, FPreparedChange& Out)
    {
        check(IsInGameThread()); Out = {};
        if (!Data) return EStep::ResourceUnavailable;
        if (!Current(*Data, Attempt)) return EStep::Invalid;
        if (Error != EError::None && Error != EError::AdvanceFailed && Error != EError::AccessLost) return EStep::Invalid;
        if (Error == EError::None && (ServerNowMs <= 0 || ServerNowMs > ResidentPopulationPrivate::SafeInteger)) return EStep::Invalid;
        auto Result = Prepare(*Data, Error == EError::AccessLost ? EChange::AccessLoss
            : Error == EError::None ? EChange::AdvanceSuccess : EChange::AdvanceFailure,
            Data->Presence, Error == EError::None ? Data->Error : Error, Out);
        if (Result == EStep::Complete)
        { Out.ServerNowMs = ServerNowMs; Out.OperationSerial = Attempt.Serial;
            Out.DeadlineSeconds = Attempt.PermissionDeadlineSeconds; Data->Pending = Out; }
        return Result;
    }
    void FResidentPopulation::CancelAttempt(const FAttemptReceipt& Attempt)
    {
        check(IsInGameThread());
        if (!Data || !Current(*Data, Attempt) || Data->Pending.Change != EChange::None) return;
        if (Admit(*Data, ResidentPopulationPrivate::QualifiedAttemptWork) != EStep::Complete) return;
        Data->Operation = {}; // Cancellation/uncertainty never releases the attempted key.
    }
    FResidentPopulation::EStep FResidentPopulation::BorrowResident(const FScopeReceipt& Scope,
        FWorldObservationToken Token, int32 Index, FBorrow& Out)
    {
        check(IsInGameThread());
        if (Out.Owner) return EStep::Invalid;
        if (!Data) return EStep::ResourceUnavailable;
        auto Result = Admit(*Data, ResidentPopulationPrivate::QualifiedBorrowWork + ResidentPopulationPrivate::QualifiedBorrowReleaseWork);
        if (Result != EStep::Complete) return Result;
        if (!Current(*Data, Scope) || !Data->bReadable || Token != Data->Token
            || Data->Presence != EPresence::Value || !Data->Population || !Data->Population->Residents.IsValidIndex(Index)
            || Data->Pending.Change != EChange::None || Data->Borrows == MAX_uint32) return EStep::Invalid;
        ++Data->Borrows; Out.Owner = Data; Out.Scope = Data->ScopeSerial; Out.Token = Token; Out.Index = Index;
        Out.DeadlineSeconds = FMath::Min(Scope.DeadlineSeconds, Data->DeadlineSeconds);
        return EStep::Complete;
    }
    const FResidentView* FResidentPopulation::FBorrow::Get() const
    {
        check(IsInGameThread());
        if (!Owner || Admit(*Owner, ResidentPopulationPrivate::QualifiedBorrowWork) != EStep::Complete)
            return nullptr;
        return Owner && Owner->bSelected && !Owner->bDisposed && Owner->bReadable
            && Live(DeadlineSeconds) && Live(Owner->DeadlineSeconds)
            && Owner->ScopeSerial == Scope && Owner->Token == Token && Owner->Population
            && Owner->Population->Residents.IsValidIndex(Index) ? &Owner->Population->Residents[Index] : nullptr;
    }
    FResidentPopulation::FBorrow::~FBorrow()
    {
        check(IsInGameThread());
        if (Owner) { check(Owner->Borrows); --Owner->Borrows; }
    }
    FResidentPopulation::EStep FResidentPopulation::ReleasePopulation(FData& Owner,
        TSharedPtr<const FResidentWorldView>& Value, uint64& Charge)
    {
        if (!Value) return Charge ? EStep::Invalid : EStep::Complete;
        if (Owner.Borrows || Value.GetSharedReferenceCount() != 1) return EStep::Pending;
        auto Result = Admit(Owner, ResidentPopulationPrivate::QualifiedPopulationReleaseWork);
        if (Result != EStep::Complete) return Result;
        Value.Reset(); // Complete nested tree/control-block deletion was admitted.
        Owner.Budget->Release(Charge); Charge = 0;
        return EStep::Complete;
    }
    FResidentPopulation::EStep FResidentPopulation::RetireData(void* Value, FBudget& Budget)
    {
        check(IsInGameThread());
        auto& Owner = *static_cast<FData*>(Value);
        if (Owner.Budget != &Budget) return EStep::Invalid;
        auto Result = Admit(Owner, ResidentPopulationPrivate::QualifiedRetireWork);
        if (Result != EStep::Complete) return Result;
        if (Owner.Borrows || Owner.bDecodeReserved) return EStep::Pending;
        Result = ReleasePopulation(Owner, Owner.Retired, Owner.RetiredCharge);
        if (Result != EStep::Complete) return Result;
        if (!Owner.bScopeRetiring)
            return Owner.bDisposed ? Admit(Owner, ResidentPopulationPrivate::QualifiedOwnerDeleteWork) : EStep::Complete;
        Result = ReleasePopulation(Owner, Owner.Candidate, Owner.CandidateCharge);
        if (Result != EStep::Complete) return Result;
        Result = ReleasePopulation(Owner, Owner.Population, Owner.CurrentCharge);
        if (Result != EStep::Complete) return Result;
        if (Owner.ScopeCharge)
        {
            Result = Admit(Owner, ResidentPopulationPrivate::QualifiedScopeReleaseWork);
            if (Result != EStep::Complete) return Result;
            Owner.Room.Empty(); Budget.Release(Owner.ScopeCharge); Owner.ScopeCharge = 0;
        }
        Owner.bScopeRetiring = false;
        if (Owner.bDisposed)
        {
            Result = Admit(Owner, ResidentPopulationPrivate::QualifiedOwnerDeleteWork);
            if (Result != EStep::Complete) return Result;
        }
        return EStep::Complete;
    }
    void FResidentPopulation::DeleteData(void* Value)
    {
        auto* Owner = static_cast<FData*>(Value);
        check(Owner->bDisposed && !Owner->Borrows && !Owner->bDecodeReserved && !Owner->ScopeCharge
            && !Owner->Population && !Owner->Candidate && !Owner->Retired);
        delete Owner;
    }
    FResidentPopulation::EStep FResidentPopulation::AdvanceRetirement()
    {
        check(IsInGameThread());
        return Data ? RetireData(Data, *Data->Budget) : EStep::Complete;
    }
    const TCHAR* FResidentPopulation::ErrorText(EError Error)
    {
        switch (Error)
        {
        case EError::None: return nullptr;
        case EError::ReadFailed: return TEXT("Residents couldn't load. Reload the world to retry.");
        case EError::AdvanceFailed: return TEXT("Resident activity couldn't update. Reload the world to retry.");
        case EError::AccessLost: return TEXT("World access is unavailable.");
        default: return TEXT("Resident activity is unavailable.");
        }
    }
}
