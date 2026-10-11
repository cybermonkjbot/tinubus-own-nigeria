#pragma once

#include "BaddieCompanionResidentState.h"
#include "BaddieCompanionWorldObservationPages.h"

namespace BaddieCompanion
{
    // Private bookkeeping only. FWorldSession must separately admit transport,
    // commit aggregate errors/conflict masks and deliver the composite token.
    class FResidentPopulation
    {
    public:
        FResidentPopulation() = default;
        ~FResidentPopulation();
        FResidentPopulation(const FResidentPopulation&) = delete;
        FResidentPopulation& operator=(const FResidentPopulation&) = delete;

    private:
        friend class FWorldSession;
        using EStep = WorldObservationPrivate::EStep;
        using FBudget = WorldObservationPrivate::FBudget;
        struct FData;
        class FDecodedReservation;
        class FDispatchReceipt;

        enum class EPresence : uint8 { Unavailable, KnownNull, Value };
        enum class EError : uint8 { None, ReadFailed, AdvanceFailed, AccessLost };
        enum class EChange : uint8 { None, Read, ReadFailure, AdvanceSuccess, AdvanceFailure, AccessLoss };
        enum class ERetirement : uint8
        { PlayerAbsent, RoomChanged, LastObserver, Reload, AccountChanged, AccessLost, Disposed };

        // Only the enclosing world fills these fields after its admission. Room
        // is borrowed for one call, never retained. Generations must not wrap.
        class FScopeReceipt
        {
            friend class FWorldSession;
            friend class FResidentPopulation;
            friend class FDispatchReceipt;
            uint64 Account = 0, Access = 0, Entry = 0, RoomGeneration = 0, Subscription = 0, Visit = 0;
            const FString* Room = nullptr;
            double DeadlineSeconds = 0;
            FScopeReceipt() = default;
        };
        // Parent mints only after its separate dispatch admission. This is not
        // an entitlement, credential or backend capability.
        class FDispatchReceipt
        {
            friend class FWorldSession;
            friend class FResidentPopulation;
            FScopeReceipt Scope;
            uint64 Serial = 0;
            double DeadlineSeconds = 0;
            FDispatchReceipt() = default;
        };
        class FReadReceipt
        {
            friend class FResidentPopulation;
            friend class FWorldSession;
            friend class FDecodedReservation;
            uint64 Scope = 0, Visit = 0, Serial = 0;
            uint64 Account = 0, Access = 0, Entry = 0, RoomGeneration = 0, Subscription = 0;
            double DeadlineSeconds = 0;
            FReadReceipt() = default;
        };
        class FDecodedReservation
        {
        public:
            ~FDecodedReservation();
            FDecodedReservation(const FDecodedReservation&) = delete;
            FDecodedReservation& operator=(const FDecodedReservation&) = delete;
        private:
            friend class FResidentPopulation;
            friend class FWorldSession;
            FData* Owner = nullptr;
            uint64 Serial = 0;
            FReadReceipt Read;
            FDecodedReservation() = default;
        };
        class FAttemptReceipt
        {
            friend struct FData;
            friend class FResidentPopulation;
            friend class FWorldSession;
            uint64 Scope = 0, Visit = 0, Serial = 0;
            uint64 Account = 0, Access = 0, Entry = 0, RoomGeneration = 0, Subscription = 0;
            int64 DeadlineMs = 0;
            uint64 DispatchSerial = 0;
            double PermissionDeadlineSeconds = 0;
            FAttemptReceipt() = default;
        };
        class FWakeProposal
        {
            friend class FResidentPopulation;
            friend class FWorldSession;
            uint64 Scope = 0, Version = 0;
            uint64 DispatchSerial = 0;
            int64 DeadlineMs = 0;
            FWorldObservationToken RenderToken;
            FWakeProposal() = default;
        };
        class FPreparedChange
        {
            friend struct FData;
            friend class FResidentPopulation;
            friend class FWorldSession;
            uint64 Scope = 0, Version = 0, Serial = 0;
            uint64 ReadSerial = 0, Visit = 0, OperationSerial = 0;
            uint64 Account = 0, Access = 0, Entry = 0, RoomGeneration = 0, Subscription = 0;
            double DeadlineSeconds = 0;
            EChange Change = EChange::None;
            EPresence Presence = EPresence::Unavailable;
            EError PreviousError = EError::None, Error = EError::None;
            TOptional<int64> DeadlineMs;
            // Fixed facts for parent comparisons. Null/failure leave both empty.
            TCHAR WorldKey[257]{}, EpochKey[257]{};
            int64 ServerNowMs = 0;
            FPreparedChange() = default;
        };
        class FBorrow
        {
        public:
            ~FBorrow();
            FBorrow(const FBorrow&) = delete;
            FBorrow& operator=(const FBorrow&) = delete;
        private:
            friend class FResidentPopulation;
            friend class FWorldSession;
            FBorrow() = default;
            FData* Owner = nullptr;
            uint64 Scope = 0;
            double DeadlineSeconds = 0;
            FWorldObservationToken Token;
            int32 Index = INDEX_NONE;
            const FResidentView* Get() const;
        };

        EStep Initialize(FBudget& Budget);
        EStep SelectScope(const FScopeReceipt& Scope, EError& PreviousError);
        EError RetireScope(ERetirement Reason);
        void Dispose();
        EStep SetObservation(const FScopeReceipt& Scope, FWorldObservationToken Token);
        EStep CaptureRead(const FScopeReceipt& Scope, FReadReceipt& Out);
        EStep ReserveDecoded(const FReadReceipt& Read, FDecodedReservation& Out);
        EStep PrepareRead(FDecodedReservation& Reservation,
            TSharedPtr<const FResidentWorldView>& Decoded, FPreparedChange& Out);
        EStep PrepareNull(const FReadReceipt& Read, FPreparedChange& Out);
        EStep PrepareFailure(const FReadReceipt& Read, EError Error, FPreparedChange& Out);
        EStep Commit(const FPreparedChange& Change, FWorldObservationToken NextToken);
        EStep CancelPrepared();
        // A reservation keeps its retired data owner alive after shell disposal.
        // Rejected/stale decoded results must return through this charged path.
        static EStep RejectDecoded(FDecodedReservation& Reservation,
            TSharedPtr<const FResidentWorldView>& Decoded);
        EStep ProposeWake(const FDispatchReceipt& Dispatch, const FWorldObservation& Rendered,
            const FWorldPresentationTime& Time, FWakeProposal& Out) const;
        EStep BeginAttempt(const FDispatchReceipt& Dispatch, const FWakeProposal& Proposal,
            const FWorldObservation& Rendered, const FWorldPresentationTime& Time, FAttemptReceipt& Out);
        EStep CompleteAttempt(const FAttemptReceipt& Attempt, int64 ServerNowMs,
            EError Error, FPreparedChange& Out);
        void CancelAttempt(const FAttemptReceipt& Attempt);
        EStep BorrowResident(const FScopeReceipt& Scope, FWorldObservationToken Token,
            int32 Index, FBorrow& Out);
        EStep AdvanceRetirement();
        static EStep RetireData(void* Owner, FBudget& Budget);
        static void DeleteData(void* Owner);
        static bool Qualified();
        static EError Revoke(FData& Data, ERetirement Reason);
        static bool Current(const FData& Data, const FScopeReceipt& Scope);
        static bool Current(const FData& Data, const FReadReceipt& Read);
        static bool Current(const FData& Data, const FAttemptReceipt& Attempt);
        static EStep Admit(FData& Data, uint64 Work, bool bCandidate = false);
        static EStep Prepare(FData& Data, EChange Kind, EPresence Presence, EError Error,
            FPreparedChange& Out);
        static EStep ReleasePopulation(FData& Data, TSharedPtr<const FResidentWorldView>& Value, uint64& Charge);
        static const TCHAR* ErrorText(EError Error);
        FData* Data = nullptr;
        bool bDisposed = false;
    };
}
