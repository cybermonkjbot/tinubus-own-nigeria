#include "BaddieCompanionWorldSession.h"
#include "BaddieCompanionStateJson.h"
#include "BaddieCompanionWorldPlayerRead.h"
#include "HAL/PlatformTime.h"

#include <cmath>

namespace BaddieCompanion
{
    namespace
    {
        using namespace WorldObservationPrivate;
        using namespace StateJsonPrivate;
        constexpr uint64 MaxCounter = 9007199254740991ULL;
        bool Exact(const FObject& O, std::initializer_list<const TCHAR*> Names)
        { return O.IsValid() && O->Values.Num() == static_cast<int32>(Names.size()) && Only(O, Names); }
        bool Hex(const FString& S, int32 Count)
        {
            if (S.Len() != Count) return false;
            for (TCHAR C : S) if (!(C >= '0' && C <= '9') && !(C >= 'a' && C <= 'f')) return false;
            return true;
        }
        bool Text(const FObject& O, const TCHAR* Key, FString& S)
        { return String(Field(O, Key), S); }
        const TCHAR* Kind(EWorldRequest Operation)
        {
            switch (Operation)
            {
            case EWorldRequest::Enter: return TEXT("enter");
            case EWorldRequest::Prepare: return TEXT("prepare");
            case EWorldRequest::Observe: return TEXT("observe");
            case EWorldRequest::Move: return TEXT("move");
            case EWorldRequest::Ready: return TEXT("ready");
            case EWorldRequest::PresentationFailed: return TEXT("presentationFailed");
            default: return TEXT("");
            }
        }
        bool Command(EWorldRequest O)
        {
            return O == EWorldRequest::Enter || O == EWorldRequest::Move
                || O == EWorldRequest::Ready || O == EWorldRequest::PresentationFailed;
        }
    }

    FWorldSession::~FWorldSession()
    {
        Clear();
        for (auto& Storage : Retiring) WorldObservationPrivate::RetireDetached(MoveTemp(Storage));
        Recovery.Dispose();
    }
    FWorldSessionView FWorldSession::View() const
    {
        auto Out = State;
        Out.bHasObservation = Live() && Out.Observation.IsCurrent();
        if (!Out.bHasObservation) Out.Observation = {};
        // Imported-content evidence, public transport and acknowledged presentation
        // have no producers in this source assignment. Reads cannot grant walking.
        Out.bWalkingAvailable = false;
        return Out;
    }
    bool FWorldSession::Live() const
    {
        const double Now = FPlatformTime::Seconds();
        return bAttached && Entry.CredentialGeneration && Entry.EntrySequence > 0
            && std::isfinite(Now) && std::isfinite(Entry.Deadline) && Now < Entry.Deadline;
    }
    void FWorldSession::Attach() { bAttached = true; }
    void FWorldSession::Detach() { bAttached = false; Clear(); }
    bool FWorldSession::HasRetirement() const
    {
        for (const auto& Storage : Retiring) if (Storage) return true;
        return false;
    }
    void FWorldSession::Retire(TSharedPtr<WorldObservationPrivate::FStorage> Storage)
    {
        if (!Storage) return;
        Storage->bComplete = false; Storage->bRetiring = true;
        for (const auto& Existing : Retiring) if (Existing == Storage) return;
        for (auto& Slot : Retiring)
            if (!Slot) { Slot = MoveTemp(Storage); return; }
        // New candidates are refused while this owner is occupied. No fourth
        // storage object may be admitted by this lifetime.
        checkNoEntry();
    }
    void FWorldSession::Clear(EWorldReadStatus Status)
    {
        if (Access) Access->Revoke();
        Access.Reset(); Transport.Reset();
        State.Observation = {}; State.bHasObservation = State.bWalkingAvailable = false;
        if (State.Generation != MAX_uint64) ++State.Generation;
        State.Status = Status;
        bInFlight = false; Request = {}; Next = CaptureOperation = EWorldRequest::None;
        Body.Reset(); DecodedBody.Reset();
        if (Pages) Retire(Pages->Cancel());
        Pages.Reset();
        if (Candidate) Retire(MoveTemp(Candidate));
        if (Accepted) Retire(MoveTemp(Accepted));
        Entry = {}; VisitId.Reset(); LifeId.Reset(); Preparation.Reset(); Capture.Reset(); RequestId.Reset();
        RevisionHighWater = MoveHighWater = -1;
        TicketArrival = CandidateDeadline = NextAt = 0;
        ReceiptExpiresAt = 0; bBindingValidated = false;
        bObserveAfterRetirement = false;
        // Outward health notification is last. Reentry sees no readable world.
        Recovery.Clear();
    }
    void FWorldSession::BeginEntry(const FWorldEntryIdentity& Identity)
    {
        Clear();
        if (!bAttached || State.Generation == MAX_uint64) return;
        Entry = Identity;
        if (Live()) BeginCapture(EWorldRequest::Enter);
    }
    void FWorldSession::Retry()
    {
        if (!Live() || bInFlight || Next != EWorldRequest::None || HasRetirement() || Candidate) return;
        // No mutation replay after an ambiguous completion. Only its retained result
        // may resolve the original operation until that lifetime is retired.
        if (bLostTicket && Command(CaptureOperation))
        { Next = EWorldRequest::Result; NextAt = FPlatformTime::Seconds(); return; }
        if (!VisitId.IsEmpty()) BeginCapture(EWorldRequest::Prepare);
    }
    bool FWorldSession::BeginCapture(EWorldRequest Operation)
    {
        // Movement/presentation request argument ownership is not implemented yet.
        // Never construct an incomplete command if another prerequisite is enabled.
        if (Operation != EWorldRequest::Enter && Operation != EWorldRequest::Prepare
            && Operation != EWorldRequest::Observe) return false;
        if (!Live() || HasRetirement() || Candidate || bInFlight || RequestSequence >= MaxCounter
            || (Command(Operation) && OperationSequence >= MaxCounter)) return false;
        if (!WorldObservationPrivate::FPages::ResourcesQualified())
        { State.Status = EWorldReadStatus::ResourceUnavailable; return false; }
        if (!Budget) Budget = MakeShared<WorldObservationPrivate::FBudget>();
        Budget->CandidateWork = 0; Budget->bExhausted = false;
        Pages = MakeUnique<WorldObservationPrivate::FPages>(Budget.ToSharedRef());
        ++RequestSequence;
        if (Command(Operation)) ++OperationSequence;
        RequestId = FGuid::NewGuid().ToString(EGuidFormats::Digits);
        Capture.Reset(); CaptureOperation = Operation; Next = Operation;
        TicketArrival = 0; CandidateDeadline = Entry.Deadline;
        bLostTicket = bPageRetried = bBindingValidated = false;
        NextAt = FPlatformTime::Seconds();
        State.Status = Operation == EWorldRequest::Prepare ? EWorldReadStatus::Preparing : EWorldReadStatus::Capturing;
        return true;
    }
    bool FWorldSession::Current(const FWorldDispatch& Dispatch) const
    {
        return Live() && bInFlight && Dispatch.Lifetime == State.Generation
            && Dispatch.Serial == Request.Serial && Dispatch.Operation == Request.Operation;
    }
    bool FWorldSession::TakeDispatch(FWorldDispatch& Out)
    {
        Out = {};
        if (!Live() || bInFlight || !Body.IsEmpty() || DecodedBody || Next == EWorldRequest::None
            || FPlatformTime::Seconds() < NextAt || DispatchSerial == MAX_uint64) return false;
        const auto Args = MakeShared<FJsonObject>();
        Args->SetNumberField(TEXT("entrySequence"), Entry.EntrySequence);
        if (Next == CaptureOperation || Next == EWorldRequest::Result)
        {
            Args->SetStringField(TEXT("requestId"), RequestId);
            Args->SetNumberField(TEXT("requestSequence"), static_cast<double>(RequestSequence));
            if (Next != EWorldRequest::Result)
            {
                if (Command(Next)) Args->SetNumberField(TEXT("operationSequence"), static_cast<double>(OperationSequence));
                if (Next != EWorldRequest::Enter)
                { Args->SetStringField(TEXT("visitId"), VisitId); Args->SetStringField(TEXT("lifeId"), LifeId); }
                if (Next == EWorldRequest::Observe) Args->SetStringField(TEXT("preparation"), Preparation);
            }
        }
        else
        {
            Args->SetStringField(TEXT("capture"), Capture);
            if (Next == EWorldRequest::Page)
            {
                FString Root; uint64 Ordinal = 0;
                if (!Pages || !Pages->Next(Root, Ordinal)) return false;
                Args->SetArrayField(TEXT("cursor"), {MakeShared<FJsonValueString>(Root),
                    MakeShared<FJsonValueNumber>(static_cast<double>(Ordinal))});
            }
        }
        Request = {Next, State.Generation, ++DispatchSerial, Args};
        Next = EWorldRequest::None; bInFlight = true; Out = Request;
        return true;
    }
    void FWorldSession::Complete(const FWorldDispatch& Dispatch, FString InBody, double S, double R)
    {
        if (!Current(Dispatch)) return;
        bInFlight = false;
        if (!std::isfinite(S) || !std::isfinite(R) || R < S || R > FPlatformTime::Seconds()
            || InBody.IsEmpty() || InBody.Len() > WorldObservationPrivate::BodyLimit)
        { Unavailable(EWorldReadStatus::InvalidResponse); return; }
        SentAt = S; ReceivedAt = R; Body = MoveTemp(InBody);
    }
    void FWorldSession::Failed(const FWorldDispatch& Dispatch, bool bTransportUnavailable)
    {
        if (!Current(Dispatch)) return;
        bInFlight = false;
        if (bTransportUnavailable) { Unavailable(EWorldReadStatus::Unavailable); return; }
        if (Command(Dispatch.Operation))
        {
            bLostTicket = true;
            Next = EWorldRequest::Result; NextAt = FPlatformTime::Seconds() + 1;
        }
        else if (Dispatch.Operation == EWorldRequest::Page && !bPageRetried)
        {
            bPageRetried = true;
            Next = EWorldRequest::Page; NextAt = FPlatformTime::Seconds() + 1;
        }
        else Unavailable(EWorldReadStatus::Unavailable);
    }
    void FWorldSession::Unavailable(EWorldReadStatus Status)
    {
        bInFlight = false; Next = EWorldRequest::None;
        bObserveAfterRetirement = false;
        Body.Reset(); DecodedBody.Reset();
        if (Pages) Retire(Pages->Cancel());
        Pages.Reset();
        if (Candidate) Retire(MoveTemp(Candidate));
        State.Status = Status;
        if (!Live() || !Access || !Access->Current())
        {
            if (Access) Access->Revoke();
            State.Observation = {}; State.bHasObservation = false;
        }
    }
    void FWorldSession::Tick()
    {
        if (Budget) Budget->BeginTick();
        for (auto& Storage : Retiring)
        {
            if (!Storage) continue;
            if (Storage->Retire() == WorldObservationPrivate::EStep::Complete) Storage.Reset();
            break;
        }
        if (!Live())
        {
            if (Entry.CredentialGeneration) Clear(EWorldReadStatus::Expired);
            return;
        }
        if (bObserveAfterRetirement && !HasRetirement())
        {
            bObserveAfterRetirement = false;
            BeginCapture(EWorldRequest::Observe);
        }
        const double Now = FPlatformTime::Seconds();
        if (CandidateDeadline > 0 && (!std::isfinite(Now) || Now >= CandidateDeadline))
        { Unavailable(EWorldReadStatus::Expired); return; }
        if (!Body.IsEmpty() || DecodedBody) ProcessBody();
        else if (Pages && !bInFlight && Next == EWorldRequest::None) AdvancePages();
    }
    void FWorldSession::ProcessBody()
    {
        using namespace StateJsonPrivate;
        using namespace WorldObservationPrivate;
        if (!Budget) { Unavailable(EWorldReadStatus::ResourceUnavailable); return; }
        if (!DecodedBody)
        {
            if (!Budget->ReservePage()) return;
            if (!FPages::DecodeBody(Body, DecodedBody, *Budget))
            { Unavailable(Budget->bExhausted ? EWorldReadStatus::ResourceUnavailable : EWorldReadStatus::InvalidResponse); return; }
            Body.Reset();
        }
        const auto O = Object(Field(DecodedBody, TEXT("value")));
        FString Status, Outer;
        if (!Exact(DecodedBody, {TEXT("status"), TEXT("value")}) || !Text(DecodedBody, TEXT("status"), Outer)
            || !Same(Outer, TEXT("success")) || !Text(O, TEXT("status"), Status))
        { Unavailable(EWorldReadStatus::InvalidResponse); return; }
        if (Same(Status, TEXT("slow_down")))
        {
            int64 Delay = 0;
            if (!Exact(O, {TEXT("status"), TEXT("retryAfterMs")}) || !Integer(Field(O, TEXT("retryAfterMs")), Delay)
                || Delay <= 0 || Delay > 900000) { Unavailable(EWorldReadStatus::InvalidResponse); return; }
            // Mutations are resolved by original identity even after throttling.
            Next = Command(Request.Operation) ? EWorldRequest::Result : Request.Operation;
            NextAt = ReceivedAt + static_cast<double>(Delay) / 1000;
            DecodedBody.Reset(); Budget->ReleasePage(); return;
        }
        if (Same(Status, TEXT("pending")) || Same(Status, TEXT("captured")))
        {
            FString Alias, Generation, Id, CaptureKind; int64 V = 0, Seq = 0, RetryMs = 0;
            if (!Exact(O, {TEXT("v"), TEXT("generation"), TEXT("capture"), TEXT("status"), TEXT("kind"),
                    TEXT("requestId"), TEXT("requestSequence"), TEXT("retryAfterMs")})
                || !Integer(Field(O, TEXT("v")), V) || V != 1 || !Text(O, TEXT("generation"), Generation)
                || !Same(Generation, ProtocolGeneration) || !Text(O, TEXT("capture"), Alias) || !Hex(Alias, 32)
                || !Text(O, TEXT("requestId"), Id) || !Same(Id, RequestId)
                || !Integer(Field(O, TEXT("requestSequence")), Seq) || static_cast<uint64>(Seq) != RequestSequence
                || !Text(O, TEXT("kind"), CaptureKind) || !Same(CaptureKind, Kind(CaptureOperation))
                || !Integer(Field(O, TEXT("retryAfterMs")), RetryMs) || RetryMs > 900000
                || (!Capture.IsEmpty() && !Same(Capture, Alias)))
            { Unavailable(EWorldReadStatus::InvalidResponse); return; }
            Capture = MoveTemp(Alias);
            // Only the original capture-producing completion anchors its serverNow.
            // Manifest polling and retained-result lookup never fill this field.
            if (Request.Operation == CaptureOperation && !bLostTicket)
                TicketArrival = TicketArrival > 0 ? FMath::Min(TicketArrival, ReceivedAt) : ReceivedAt;
            if (TicketArrival > 0 && CaptureOperation != EWorldRequest::Prepare)
                CandidateDeadline = FMath::Min(CandidateDeadline, TicketArrival + 60);
            Next = EWorldRequest::Manifest;
            NextAt = ReceivedAt + static_cast<double>(RetryMs) / 1000;
            DecodedBody.Reset(); Budget->ReleasePage(); return;
        }
        if (Request.Operation == EWorldRequest::Manifest && Same(Status, TEXT("manifest")))
        {
            const EStep Step = Pages ? Pages->Begin(DecodedBody, Capture, Kind(CaptureOperation)) : EStep::Invalid;
            if (Step == EStep::Invalid || Step == EStep::ResourceUnavailable)
            { Unavailable(Step == EStep::Invalid ? EWorldReadStatus::InvalidResponse : EWorldReadStatus::ResourceUnavailable); return; }
            if (Pages->Manifest()) { DecodedBody.Reset(); State.Status = EWorldReadStatus::Receiving; }
            return;
        }
        if (Request.Operation == EWorldRequest::Page && Same(Status, TEXT("page")))
        {
            const EStep Step = Pages ? Pages->Consume(DecodedBody, 0) : EStep::Invalid;
            DecodedBody.Reset();
            if (Step == EStep::Invalid || Step == EStep::ResourceUnavailable)
                Unavailable(Step == EStep::Invalid ? EWorldReadStatus::InvalidResponse : EWorldReadStatus::ResourceUnavailable);
            else bPageRetried = false;
            return;
        }
        if (Request.Operation == EWorldRequest::Bind && Same(Status, TEXT("bound")))
        {
            const bool bAccepted = AcceptBinding(O);
            DecodedBody.Reset(); Budget->ReleasePage();
            if (!bAccepted) Unavailable(EWorldReadStatus::InvalidResponse);
            return;
        }
        // Denial/loss is not a transient read error and cannot keep an old visit.
        if (Same(Status, TEXT("denied")) || Same(Status, TEXT("entry_lost"))
            || Same(Status, TEXT("world_lost")) || Same(Status, TEXT("selection_changed")))
        { Clear(); return; }
        Unavailable(Same(Status, TEXT("resource_unavailable"))
            ? EWorldReadStatus::ResourceUnavailable : EWorldReadStatus::InvalidResponse);
    }
    void FWorldSession::AdvancePages()
    {
        using namespace WorldObservationPrivate;
        const EStep Step = Pages->Advance();
        if (Step == EStep::Invalid || Step == EStep::ResourceUnavailable)
        { Unavailable(Step == EStep::Invalid ? EWorldReadStatus::InvalidResponse : EWorldReadStatus::ResourceUnavailable); return; }
        FString Root; uint64 Ordinal = 0;
        if (Pages->Next(Root, Ordinal)) { Next = EWorldRequest::Page; NextAt = FPlatformTime::Seconds(); return; }
        if (Step != EStep::Complete) return;
        Candidate = Pages->TakeComplete();
        const EStep Validated = ValidateCandidate();
        if (Validated != EStep::Complete)
        {
            Retire(MoveTemp(Candidate));
            Unavailable(Validated == EStep::ResourceUnavailable ? EWorldReadStatus::ResourceUnavailable : EWorldReadStatus::InvalidResponse);
            return;
        }
        if (CaptureOperation == EWorldRequest::Prepare)
        {
            Preparation = Capture;
            Retire(MoveTemp(Candidate));
            // Exactly one fresh dynamic capture follows this preparation. Failure
            // requires explicit reload; static bytes never create a clock sample.
            bObserveAfterRetirement = true;
            State.Status = EWorldReadStatus::Preparing;
            return;
        }
        State.Status = EWorldReadStatus::Binding; Next = EWorldRequest::Bind;
    }
    WorldObservationPrivate::EStep FWorldSession::ValidateCandidate()
    {
        // Existing object readers are now shared, but their arbitrary-size typed
        // projections and destruction are not yet accounted/resumable. Never join
        // the segmented instance or call them on a truncated synthetic document.
        // Keep the candidate unavailable until that adapter is implemented.
        bBindingValidated = false;
        return WorldObservationPrivate::EStep::ResourceUnavailable;
    }
    bool FWorldSession::AcceptBinding(const TSharedPtr<FJsonObject>& O)
    {
        using namespace StateJsonPrivate;
        using namespace WorldObservationPrivate;
        if (!Live() || !Candidate || !bBindingValidated || TicketArrival <= 0 || ReceiptExpiresAt <= 0) return false;
        const auto& M = Candidate->Manifest;
        FString Alias, Digest, Generation; int64 V = 0, At = 0, Permission = 0, Publish = 0, Checked = 0;
        if (!Exact(O, {TEXT("v"), TEXT("generation"), TEXT("capture"), TEXT("status"), TEXT("directoryDigest"),
                TEXT("capturedAt"), TEXT("permissionDeadline"), TEXT("publishBy"), TEXT("checkedAt")})
            || !Integer(Field(O, TEXT("v")), V) || V != 1 || !Text(O, TEXT("generation"), Generation)
            || !Same(Generation, ProtocolGeneration) || !Text(O, TEXT("capture"), Alias) || !Same(Alias, M.Capture)
            || !Text(O, TEXT("directoryDigest"), Digest) || !Same(Digest, M.Digest)
            || !Integer(Field(O, TEXT("capturedAt")), At) || At != M.CapturedAt
            || !Integer(Field(O, TEXT("permissionDeadline")), Permission) || Permission != M.PermissionDeadline
            || !Integer(Field(O, TEXT("publishBy")), Publish) || !M.PublishBy.IsSet() || Publish != M.PublishBy.GetValue()
            || !Integer(Field(O, TEXT("checkedAt")), Checked) || Checked < At
            || !std::isfinite(SentAt) || !std::isfinite(ReceivedAt) || ReceivedAt < SentAt) return false;
        const double RemainingMs = static_cast<double>(FMath::Min(FMath::Min(Permission, Publish), ReceiptExpiresAt) - Checked)
            - std::ceil((ReceivedAt - SentAt) * 1000);
        if (!std::isfinite(RemainingMs) || RemainingMs <= 0) return false;
        CandidateDeadline = FMath::Min(CandidateDeadline, FMath::Min(Entry.Deadline, ReceivedAt + RemainingMs / 1000));
        // Final binding alone is not publication. Typed identity, high-water and
        // content checks still have to commit an immutable revocable observation.
        return FPlatformTime::Seconds() < CandidateDeadline;
    }
    bool FWorldSession::Walk(const FMovementPoint&, EMovementPace)
    {
        // No ordinary movement admission exists yet. In particular a null route,
        // valid page chain or same-room Home read cannot supply it.
        return false;
    }
    void FWorldSession::StopWalking() { State.bWalkingAvailable = false; }
    void FWorldSession::ContentLost(uint64 ObservationGeneration)
    {
        if (ObservationGeneration != State.Observation.Generation()) return;
        StopWalking();
        State.Status = EWorldReadStatus::ContentUnavailable;
    }
}
