#include "BaddieCompanionJobsState.h"
#include "BaddieCompanionStateJson.h"

#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace BaddieCompanion
{
    namespace JobsStatePrivate
    {
        using namespace StateJsonPrivate;
        constexpr FParseLimits ReadLimits{512 * 1024, 16, 16384};
        constexpr FParseLimits CommandLimits{2 * 1024 * 1024, 32, 131072};
        constexpr int64 MaxAmount = 1000000000;
        constexpr const TCHAR* WorldId = TEXT("lagos-v1");

        bool Text(const FValue& V, FString& Out, int32 Maximum = 160)
        {
            return String(V, Out) && Out.Len() <= Maximum;
        }
        bool Key(const FValue& V, FString& Out)
        {
            return Text(V, Out) && !Out.TrimStartAndEnd().IsEmpty();
        }
        bool Id(const FValue& V, FString& Out)
        {
            return Text(V, Out, 240) && !Out.IsEmpty();
        }
        bool Label(const FValue& V, FString& Out)
        {
            return Text(V, Out, 100) && !Out.TrimStartAndEnd().IsEmpty();
        }
        bool Literal(const FValue& V, const TCHAR* Expected)
        {
            FString S;
            return String(V, S) && Same(S, Expected);
        }
        bool Time(const FValue& V, double& Out)
        {
            return Number(V, Out) && ValidTime(Out);
        }
        bool Root(const FString& Json, FValue& Out, const FParseLimits& Limits = ReadLimits)
        {
            return WithinParseLimits(Json, Limits)
                && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Out) && Out.IsValid();
        }
        bool Contact(const FValue& V, FJobsContact& Out)
        {
            const auto O = Object(V);
            return Key(Field(O, TEXT("placeId")), Out.PlaceId) && Key(Field(O, TEXT("npcId")), Out.NpcId)
                && Key(Field(O, TEXT("serviceId")), Out.ServiceId);
        }
        bool SameContact(const FJobsContact& A, const FJobsContact& B)
        {
            return Same(A.PlaceId, B.PlaceId) && Same(A.NpcId, B.NpcId) && Same(A.ServiceId, B.ServiceId);
        }
        bool SameContact(const TOptional<FJobsContact>& A, const TOptional<FJobsContact>& B)
        {
            return A.IsSet() == B.IsSet() && (!A.IsSet() || SameContact(*A, *B));
        }
        bool GameValue(const FValue& V, FHaulReward& Out, int64 Minimum)
        {
            const auto O = Object(V);
            Out.Kind = EHaulRewardKind::Money;
            Out.Unit = EHaulMoneyUnit::GameValueUnit;
            return Literal(Field(O, TEXT("kind")), TEXT("money"))
                && Literal(Field(O, TEXT("unit")), TEXT("game-value-unit"))
                && Key(Field(O, TEXT("unitKey")), Out.UnitKey)
                && Integer(Field(O, TEXT("amount")), Out.Amount, Minimum) && Out.Amount <= MaxAmount;
        }
        bool Terms(const FValue& V, FJobsTerms& Out)
        {
            const auto O = Object(V), Parcel = Object(Field(O, TEXT("parcel"))), Reward = Object(Field(O, TEXT("reward")));
            int64 Opens = 0, Closes = 0;
            if (!Text(Field(O, TEXT("offerKey")), Out.OfferKey, 64) || Out.OfferKey.IsEmpty()
                || !Key(Field(O, TEXT("worldId")), Out.WorldId) || !Label(Field(O, TEXT("label")), Out.Label)
                || !Contact(Field(O, TEXT("employer")), Out.Employer) || !Contact(Field(O, TEXT("destination")), Out.Destination)
                || !Key(Field(Parcel, TEXT("key")), Out.ParcelKey) || !Label(Field(Parcel, TEXT("label")), Out.ParcelLabel)
                || !Literal(Field(O, TEXT("timeBasis")), TEXT("server-ms"))
                || !Integer(Field(O, TEXT("opensAt")), Opens) || !Integer(Field(O, TEXT("closesAt")), Closes)
                || Opens >= MaxInteger || Closes >= MaxInteger || Closes <= Opens || Closes - Opens > 2592000000LL
                || Same(Out.Employer.PlaceId, Out.Destination.PlaceId)
                || Same(Out.Employer.PlaceId, TEXT("own-home")) || Same(Out.Destination.PlaceId, TEXT("own-home"))) return false;
            for (const TCHAR C : Out.OfferKey)
                if (!((C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z') || (C >= '0' && C <= '9')
                    || C == '_' || C == '.' || C == ':' || C == '-')) return false;
            Out.OpensAtMs = static_cast<double>(Opens);
            Out.ClosesAtMs = static_cast<double>(Closes);
            const auto Trusted = Field(O, TEXT("requiresTrustedCourier"));
            if (Trusted.IsValid())
            {
                bool B = false;
                if (!Boolean(Trusted, B) || !B) return false;
                Out.RequiresTrustedCourier = true;
            }
            if (!EnumValue<EJobsRewardApproval>(Field(Reward, TEXT("status")), Out.RewardApproval,
                {{TEXT("unavailable"), EJobsRewardApproval::Unavailable}, {TEXT("approved"), EJobsRewardApproval::Approved}})) return false;
            if (Out.RewardApproval == EJobsRewardApproval::Unavailable)
                return Literal(Field(Reward, TEXT("reason")), TEXT("approval_required"));
            FHaulReward Value;
            if (!Key(Field(Reward, TEXT("approvalKey")), Out.ApprovalKey) || !GameValue(Field(Reward, TEXT("value")), Value, 1)) return false;
            Out.ApprovedReward = MoveTemp(Value);
            return true;
        }
        bool CourierReward(const FValue& V, FHaulEntry& Out)
        {
            // Only the courier projection's delivery/game-value branch. HaulState still owns
            // the general item, currency, source, outing, page and snapshot readers.
            const auto O = Object(V), Source = Object(Field(O, TEXT("source")));
            Out.Source.Kind = EHaulSourceKind::Delivery;
            if (!Id(Field(O, TEXT("id")), Out.Id) || !Id(Field(O, TEXT("outingId")), Out.OutingId)
                || !Literal(Field(Source, TEXT("kind")), TEXT("delivery")) || !Key(Field(Source, TEXT("key")), Out.Source.Key)
                || !GameValue(Field(O, TEXT("reward")), Out.Reward, 0) || !Time(Field(O, TEXT("grantedAt")), Out.GrantedAtMs)
                || !EnumValue<EHaulStatus>(Field(O, TEXT("status")), Out.Status, {{TEXT("carried"), EHaulStatus::Carried},
                    {TEXT("banked"), EHaulStatus::Banked}, {TEXT("forfeited"), EHaulStatus::Forfeited}})) return false;
            const auto Original = Field(O, TEXT("originalMoneyAmount"));
            if (Original.IsValid())
            {
                int64 Amount = 0;
                if (!Integer(Original, Amount, 1) || Amount > MaxAmount || Amount <= Out.Reward.Amount) return false;
                Out.OriginalMoneyAmount = Amount;
            }
            return true;
        }
        bool Job(const FValue& V, FJobsJob& Out)
        {
            const auto O = Object(V);
            if (!Id(Field(O, TEXT("id")), Out.Id) || !Id(Field(O, TEXT("offerId")), Out.OfferId)
                || !Terms(Field(O, TEXT("terms")), Out.Terms) || !Id(Field(O, TEXT("outingId")), Out.OutingId)
                || !Key(Field(O, TEXT("lifeId")), Out.LifeId) || !Integer(Field(O, TEXT("revision")), Out.Revision, 1)
                || !EnumValue<EJobsStatus>(Field(O, TEXT("status")), Out.Status, {{TEXT("accepted"), EJobsStatus::Accepted},
                    {TEXT("carrying"), EJobsStatus::Carrying}, {TEXT("delivered"), EJobsStatus::Delivered},
                    {TEXT("cancelled"), EJobsStatus::Cancelled}, {TEXT("failed"), EJobsStatus::Failed}})
                || !Nullable(Field(O, TEXT("failure")), Out.Failure, [](const FValue& F, EJobsFailure& Failure)
                    { return EnumValue<EJobsFailure>(F, Failure, {{TEXT("death"), EJobsFailure::Death},
                        {TEXT("outing-ended"), EJobsFailure::OutingEnded}, {TEXT("life-ended"), EJobsFailure::LifeEnded}}); })
                || !Time(Field(O, TEXT("acceptedAt")), Out.AcceptedAtMs)
                || !Nullable(Field(O, TEXT("collectedAt")), Out.CollectedAtMs, Time)
                || !Nullable(Field(O, TEXT("endedAt")), Out.EndedAtMs, Time)
                || !Nullable(Field(O, TEXT("reward")), Out.Reward, CourierReward)) return false;
            if ((Out.Status == EJobsStatus::Delivered) != Out.Reward.IsSet()) return false;
            if (Out.Reward.IsSet())
            {
                const auto& R = *Out.Reward;
                if (!Same(R.OutingId, Out.OutingId) || !Out.Terms.ApprovedReward.IsSet()
                    || !Same(R.Reward.UnitKey, Out.Terms.ApprovedReward->UnitKey)
                    || (R.OriginalMoneyAmount.IsSet() ? *R.OriginalMoneyAmount : R.Reward.Amount)
                        != Out.Terms.ApprovedReward->Amount) return false;
            }
            return true;
        }
        bool Offer(const FValue& V, FJobsOffer& Out)
        {
            const auto O = Object(V);
            return Id(Field(O, TEXT("id")), Out.Id) && Terms(Field(O, TEXT("terms")), Out.Terms)
                && Boolean(Field(O, TEXT("available")), Out.bAvailable)
                && Integer(Field(O, TEXT("availabilityRevision")), Out.AvailabilityRevision);
        }
        bool Resident(const FValue& V, FJobsResident& Out)
        {
            const auto O = Object(V);
            return Text(Field(O, TEXT("actorIdentity")), Out.ActorIdentity, 320) && !Out.ActorIdentity.IsEmpty()
                && Text(Field(O, TEXT("name")), Out.Name, 8192);
        }
        bool Goal(const FValue& V, FJobsGoal& Out)
        {
            const auto O = Object(V);
            return Boolean(Field(O, TEXT("selected")), Out.bSelected)
                && Nullable(Field(O, TEXT("resident")), Out.Resident, Resident)
                && Integer(Field(O, TEXT("deliveries")), Out.Deliveries) && Out.Deliveries <= 3
                && Integer(Field(O, TEXT("target")), Out.Target) && Out.Target == 3
                && Nullable(Field(O, TEXT("completedAt")), Out.CompletedAtMs, Time)
                && Boolean(Field(O, TEXT("inCurrentWorld")), Out.bInCurrentWorld)
                && Out.CompletedAtMs.IsSet() == (Out.Deliveries == 3);
        }
        bool Result(const FValue& V, FJobsCommandResult& Out)
        {
            const auto O = Object(V);
            return EnumValue<EJobsOutcome>(Field(O, TEXT("outcome")), Out.Feedback.Outcome, {
                    {TEXT("applied"), EJobsOutcome::Applied}, {TEXT("replayed"), EJobsOutcome::Replayed}, {TEXT("blocked"), EJobsOutcome::Blocked}})
                && Nullable(Field(O, TEXT("reason")), Out.Feedback.Reason, [](const FValue& F, EJobsBlockedReason& Reason)
                    { return EnumValue<EJobsBlockedReason>(F, Reason, {
                        {TEXT("stale_context"), EJobsBlockedReason::StaleContext}, {TEXT("not_found"), EJobsBlockedReason::NotFound},
                        {TEXT("wrong_world"), EJobsBlockedReason::WrongWorld}, {TEXT("wrong_contact"), EJobsBlockedReason::WrongContact},
                        {TEXT("offer_unavailable"), EJobsBlockedReason::OfferUnavailable}, {TEXT("reward_unavailable"), EJobsBlockedReason::RewardUnavailable},
                        {TEXT("active_job"), EJobsBlockedReason::ActiveJob}, {TEXT("already_attempted"), EJobsBlockedReason::AlreadyAttempted},
                        {TEXT("out_of_order"), EJobsBlockedReason::OutOfOrder}, {TEXT("job_closed"), EJobsBlockedReason::JobClosed},
                        {TEXT("outing_closed"), EJobsBlockedReason::OutingClosed}, {TEXT("haul_full"), EJobsBlockedReason::HaulFull},
                        {TEXT("source_used"), EJobsBlockedReason::SourceUsed}, {TEXT("trust_required"), EJobsBlockedReason::TrustRequired}}); })
                && Nullable(Field(O, TEXT("job")), Out.Job, Job);
        }
        bool Ended(const TOptional<FJobsJob>& J)
        {
            return J.IsSet() && (J->Status == EJobsStatus::Failed || (J->Reward.IsSet() && J->Reward->Status != EHaulStatus::Carried));
        }
        bool SameProgress(const TOptional<FJobsJob>& A, const TOptional<FJobsJob>& B)
        {
            if (A.IsSet() != B.IsSet()) return false;
            if (!A.IsSet()) return true;
            if (!Same(A->Id, B->Id) || A->Revision != B->Revision || A->Status != B->Status
                || A->Failure.IsSet() != B->Failure.IsSet() || (A->Failure.IsSet() && *A->Failure != *B->Failure)
                || A->Reward.IsSet() != B->Reward.IsSet()) return false;
            return !A->Reward.IsSet() || (Same(A->Reward->Id, B->Reward->Id) && A->Reward->Status == B->Reward->Status
                && A->Reward->Reward.Amount == B->Reward->Reward.Amount);
        }
        bool HasError(const TOptional<FString>& Error) { return Error.IsSet() && !Error->IsEmpty(); }
        FString SafeMessage(const FString& Message, const TCHAR* Fallback)
        {
            return Message.Len() <= 8192 ? Message : FString(Fallback);
        }
    }

    bool DecodeJobsJob(const FString& Json, TOptional<FJobsJob>& Out, FString& OutError)
    {
        using namespace JobsStatePrivate;
        FValue V;
        TOptional<FJobsJob> Next;
        // The engine reader rejects bare scalar roots; the shared helper validates null
        // inside a container without changing the endpoint's null semantics.
        if (!(Root(Json, V) || ParseNull(Json, V, ReadLimits)) || !Nullable(V, Next, Job))
        { Out.Reset(); OutError = TEXT("Courier details couldn't load. Reload to retry."); return false; }
        Out = MoveTemp(Next); OutError.Reset(); return true;
    }
    bool DecodeJobsOffers(const FString& Json, TArray<FJobsOffer>& Out, FString& OutError)
    {
        using namespace JobsStatePrivate;
        FValue V;
        TArray<FJobsOffer> Next;
        bool bValid = Root(Json, V) && V->Type == EJson::Array && V->AsArray().Num() <= 16;
        if (bValid) for (const auto& Item : V->AsArray())
        {
            FJobsOffer Value;
            if (!Offer(Item, Value)) { bValid = false; break; }
            Next.Add(MoveTemp(Value));
        }
        if (!bValid) { Out.Reset(); OutError = TEXT("Courier offers couldn't load. Reload to retry."); return false; }
        Out = MoveTemp(Next); OutError.Reset(); return true;
    }
    bool DecodeJobsGoal(const FString& Json, FJobsGoal& Out, FString& OutError)
    {
        using namespace JobsStatePrivate;
        FValue V;
        FJobsGoal Next;
        if (!Root(Json, V) || !Goal(V, Next))
        { Out = {}; OutError = TEXT("Courier goal couldn't load. Reload to retry."); return false; }
        Out = MoveTemp(Next); OutError.Reset(); return true;
    }
    bool DecodeJobsCommandResponse(const FString& Json, FJobsCommandResponse& Out, FString& OutError)
    {
        using namespace JobsStatePrivate;
        FObject O;
        FJobsCommandResponse Next;
        bool bValid = Parse(Json, O, CommandLimits) && Integer(Field(O, TEXT("serverNow")), Next.ServerNowMs, 1)
            && Result(Field(O, TEXT("result")), Next.Result);
        const auto Player = Object(Field(O, TEXT("player")));
        FString PlayerJson, Error;
        bValid = bValid && Player.IsValid()
            && FJsonSerializer::Serialize(Player.ToSharedRef(), TJsonWriterFactory<>::Create(&PlayerJson))
            && DecodeWorldPlayerDocument(PlayerJson, Next.ServerNowMs, Next.Player, Error);
        if (!bValid) { Out = {}; OutError = TEXT("The courier result couldn't be read."); return false; }
        Out = MoveTemp(Next); OutError.Reset(); return true;
    }

    bool FJobsReadState::Enabled() const
    {
        return !bDisposed && bObserving && !AccountId.IsEmpty() && !RoomId.IsEmpty();
    }
    void FJobsReadState::RefreshError()
    {
        View.Error = CommandError.IsSet() ? CommandError : JobError.IsSet() ? JobError : OffersError;
        View.bBusy = ActiveMutation.IsValid();
        View.bRetryable = Pending.IsSet() && !ActiveMutation.IsValid()
            && OriginMatchesContext(Pending->Origin) && CanWork();
    }
    void FJobsReadState::ClearView(bool bKeepGoal)
    {
        Generation = FGuid::NewGuid(); ContactGeneration = FGuid::NewGuid();
        JobGeneration = FGuid::NewGuid(); OffersGeneration = FGuid::NewGuid();
        Context.Reset(); SelectedService.Reset(); bJobActive = false; bOffersActive = false;
        JobError.Reset(); OffersError.Reset(); CommandError.Reset();
        const auto Goal = bKeepGoal ? View.Goal : TOptional<FJobsGoal>{};
        const auto GoalError = bKeepGoal ? View.GoalError : TOptional<FString>{};
        const bool bGoalReady = bKeepGoal && View.bGoalReady;
        const bool bGoalBusy = bKeepGoal && View.bGoalBusy;
        if (!bKeepGoal) { GoalGeneration = FGuid::NewGuid(); bGoalActive = false; GoalUserId.Reset(); }
        View = {}; View.Goal = Goal; View.GoalError = GoalError; View.bGoalReady = bGoalReady; View.bGoalBusy = bGoalBusy;
        RefreshError();
    }
    bool FJobsReadState::Select(const FString& InAccountId, const FString& GameRoomId)
    {
        using namespace JobsStatePrivate;
        if (bDisposed || InAccountId.Len() > 240 || GameRoomId.Len() > 240) return false;
        if (Same(AccountId, InAccountId) && Same(RoomId, GameRoomId)) return true;
        if (!Same(AccountId, InAccountId)) ClearOwnedRequests();
        Pending.Reset();
        AccountId = InAccountId; RoomId = GameRoomId; bBlocked = false; bMutationEligible = false;
        SourcesGeneration = FGuid::NewGuid(); Sources = {}; ClearView(false); return true;
    }
    bool FJobsReadState::BeginObservation(FJobsSourcesReceipt& Out)
    {
        Out = {};
        if (bDisposed || AccountId.IsEmpty() || RoomId.IsEmpty() || bBlocked) return false;
        if (!bObserving) { bObserving = true; SourcesGeneration = FGuid::NewGuid(); }
        return CaptureSources(Out);
    }
    bool FJobsReadState::CaptureSources(FJobsSourcesReceipt& Out) const
    {
        Out = {};
        if (!Enabled() || bBlocked) return false;
        Out.Generation = SourcesGeneration; return true;
    }
    void FJobsReadState::EndObservation()
    {
        if (bDisposed) return;
        bObserving = false; SourcesGeneration = FGuid::NewGuid(); Sources = {}; ClearView(false);
    }
    void FJobsReadState::Reload()
    {
        if (bDisposed) return;
        bBlocked = false; Generation = FGuid::NewGuid(); ContactGeneration = FGuid::NewGuid();
        JobGeneration = FGuid::NewGuid(); OffersGeneration = FGuid::NewGuid(); GoalGeneration = FGuid::NewGuid();
        bJobActive = false; bOffersActive = false; bGoalActive = false;
        JobError.Reset(); OffersError.Reset(); CommandError.Reset();
        View.Offers.Reset(); View.bReady = false; View.bOffersReady = false; View.Result.Reset();
        View.bGoalReady = false; View.GoalError.Reset(); Reconcile(); RefreshError();
    }
    void FJobsReadState::Clear()
    {
        if (bDisposed) return;
        ClearOwnedRequests();
        EndObservation(); AccountId.Reset(); RoomId.Reset(); bBlocked = false; bMutationEligible = false;
    }
    void FJobsReadState::Dispose()
    {
        if (bDisposed) return;
        Clear(); bDisposed = true; ActiveMutation.Invalidate(); ActiveOrigin = {}; View = {};
    }
    void FJobsReadState::Revoke(const FString& Message)
    {
        Pending.Reset();
        bBlocked = true; bMutationEligible = false;
        SourcesGeneration = FGuid::NewGuid(); Sources = {}; ClearView(false);
        JobError = JobsStatePrivate::SafeMessage(Message, TEXT("Courier access is unavailable."));
        View.bReady = true; RefreshError();
    }
    TOptional<FJobsReadState::FContext> FJobsReadState::ReadContext() const
    {
        using namespace JobsStatePrivate;
        const auto& P = Sources.Player;
        const auto& R = Sources.Recovery;
        const auto& H = Sources.Haul;
        if (!Enabled() || bBlocked || !Sources.bWorldReady || !P.IsValid() || !Same(P->GameRoomId, RoomId)
            || !R.bReady || HasError(R.Error) || !R.Health.IsValid() || !H.bReady || HasError(H.Error)
            || !H.Wire.Outing.IsSet() || !Same(H.Wire.Outing->WorldId, WorldId)) return {};
        FContext C;
        C.UserId = P->UserId; C.LifeId = R.Health->LifeId; C.OutingId = H.Wire.Outing->Id;
        C.HealthStatus = R.Health->Status; C.OutingStatus = H.Wire.Outing->Status;
        C.InstanceId = P->InstanceId; C.WorldRevision = P->Revision; C.PlayerStatus = P->Status; C.RenderAttempt = Sources.RenderAttempt;
        return C;
    }
    TOptional<FJobsContact> FJobsReadState::ResolveContact(const TOptional<FString>& ServiceId) const
    {
        using namespace JobsStatePrivate;
        const auto& P = Sources.Player;
        const auto& H = Sources.Recovery.Health;
        const auto& O = Sources.Haul.Wire.Outing;
        const auto& L = Sources.Layout;
        if (!ServiceId.IsSet() || ServiceId->IsEmpty() || ServiceId->Len() > 100 || !P.IsValid()
            || P->Status != EWorldPlayerStatus::Present || P->Health <= 0 || !H.IsValid()
            || H->Status != ECompanionHealthStatus::Active || H->Health <= 0 || !O.IsSet() || O->Status != EHaulStatus::Carried
            || !L.IsSet() || !Same(L->InstanceId, P->InstanceId) || !Same(L->PlaceId, P->PlaceId)
            || !Sources.RenderedInstanceId.IsSet() || !Same(*Sources.RenderedInstanceId, P->InstanceId)) return {};
        for (const auto& S : L->Services) if (Same(S.Id, *ServiceId))
        {
            if (!S.NpcId.IsSet() || S.NpcId->IsEmpty() || S.bPolicingOwner) return {};
            return FJobsContact{P->PlaceId, *S.NpcId, S.Id};
        }
        return {};
    }
    bool FJobsReadState::CanWork() const
    {
        return Context.IsSet() && Sources.Player.IsValid() && Sources.Player->Status == EWorldPlayerStatus::Present
            && Sources.Player->Health > 0 && Sources.Recovery.Health.IsValid()
            && Sources.Recovery.Health->Status == ECompanionHealthStatus::Active && Sources.Recovery.Health->Health > 0
            && Sources.Haul.Wire.Outing.IsSet() && Sources.Haul.Wire.Outing->Status == EHaulStatus::Carried
            && !Sources.bWorldBusy && !Sources.Recovery.bBusy;
    }
    bool FJobsReadState::OriginMatchesContext(const FJobsRequestOrigin& Origin) const
    {
        using namespace JobsStatePrivate;
        return Context.IsSet() && Same(Origin.AccountId, AccountId) && Same(Origin.GameRoomId, RoomId)
            && Same(Origin.UserId, Context->UserId) && Same(Origin.LifeId, Context->LifeId)
            && Same(Origin.OutingId, Context->OutingId);
    }
    bool FJobsReadState::ProvenOriginEnded(const FJobsRequestOrigin& Origin) const
    {
        using namespace JobsStatePrivate;
        const auto& P = Sources.Player;
        const auto& R = Sources.Recovery;
        const auto& H = Sources.Haul;
        return (Sources.bWorldReady && !HasError(Sources.WorldError)
                && (!P.IsValid() || !Same(P->UserId, Origin.UserId) || !Same(P->GameRoomId, Origin.GameRoomId) || P->Health <= 0))
            || (R.bReady && !HasError(R.Error) && (!R.Health.IsValid() || !Same(R.Health->LifeId, Origin.LifeId)
                || R.Health->Status == ECompanionHealthStatus::Hospital || R.Health->Health <= 0))
            || (H.bReady && !HasError(H.Error) && (!H.Wire.Outing.IsSet() || !Same(H.Wire.Outing->Id, Origin.OutingId)
                || H.Wire.Outing->Status != EHaulStatus::Carried));
    }
    void FJobsReadState::Reconcile()
    {
        using namespace JobsStatePrivate;
        if (!Enabled() || bBlocked) return;
        if (HasError(Sources.AccessFailure)) { Revoke(*Sources.AccessFailure); return; }
        const auto& P = Sources.Player;
        const auto& R = Sources.Recovery;
        const auto& H = Sources.Haul;
        // Trusted null/change cancels eligibility. A transport-error empty snapshot does not.
        if (bMutationEligible && ProvenOriginEnded(ActiveOrigin)) bMutationEligible = false;
        if (Pending.IsSet() && ProvenOriginEnded(Pending->Origin)) Pending.Reset();
        const auto Next = ReadContext();
        if (!Next.IsSet())
        {
            if (Context.IsSet()) ClearView(true);
            JobError = Sources.WorldError.IsSet() ? Sources.WorldError : R.Error.IsSet() ? R.Error : H.Error;
        }
        else
        {
            const bool bKeyChanged = !Context.IsSet() || !Same(Context->UserId, Next->UserId)
                || !Same(Context->LifeId, Next->LifeId) || !Same(Context->OutingId, Next->OutingId)
                || Context->HealthStatus != Next->HealthStatus || Context->OutingStatus != Next->OutingStatus;
            if (bKeyChanged) { ClearView(true); Context = Next; }
            if (!Same(Context->InstanceId, Next->InstanceId) || Context->WorldRevision != Next->WorldRevision
                || Context->PlayerStatus != Next->PlayerStatus || Context->RenderAttempt != Next->RenderAttempt)
            { SelectedService.Reset(); View.Result.Reset(); CommandError.Reset(); }
            Context = Next;
            if (Pending.IsSet() && !OriginMatchesContext(Pending->Origin)) Pending.Reset();
            const auto Contact = ResolveContact(SelectedService);
            if (!Contact.IsSet()) SelectedService.Reset();
            if (!SameContact(Contact, View.Contact))
            {
                ContactGeneration = FGuid::NewGuid(); OffersGeneration = FGuid::NewGuid(); bOffersActive = false;
                OffersError.Reset(); CommandError.Reset(); View.Contact = Contact;
                View.Offers.Reset(); View.bOffersReady = false; View.Result.Reset();
            }
        }
        // Goal has account/room scope, independent of life, outing and selected service.
        const FString NextGoalUser = Sources.bWorldReady && !HasError(Sources.WorldError) && P.IsValid()
            && Same(P->GameRoomId, RoomId) ? P->UserId : FString{};
        if (!Same(GoalUserId, NextGoalUser))
        {
            GoalGeneration = FGuid::NewGuid(); bGoalActive = false; GoalUserId = NextGoalUser;
            View.Goal.Reset(); View.bGoalReady = false; View.GoalError.Reset();
        }
        RefreshError();
    }
    bool FJobsReadState::ObserveSources(const FJobsSourcesReceipt& Receipt, const FJobsSources& InSources)
    {
        if (!Enabled() || bBlocked || Receipt.Generation != SourcesGeneration || InSources.RenderAttempt < 0
            || InSources.RenderAttempt > StateJsonPrivate::MaxInteger) return false;
        Sources = InSources; Reconcile(); return true;
    }
    bool FJobsReadState::SelectContact(const TOptional<FString>& ServiceId)
    {
        if (!Enabled() || bBlocked) return false;
        Reconcile();
        if (!Enabled() || bBlocked) return false;
        if (ServiceId.IsSet() && (!Context.IsSet() || !ResolveContact(ServiceId).IsSet()))
        {
            SelectedService.Reset(); Reconcile(); CommandError = TEXT("Choose an NPC service in the current place."); RefreshError(); return false;
        }
        SelectedService = ServiceId; Reconcile(); return true;
    }
    FGuid& FJobsReadState::ReadGeneration(EJobsReadKind Kind)
    {
        return Kind == EJobsReadKind::Job ? JobGeneration : Kind == EJobsReadKind::Offers ? OffersGeneration : GoalGeneration;
    }
    bool& FJobsReadState::ReadActive(EJobsReadKind Kind)
    {
        return Kind == EJobsReadKind::Job ? bJobActive : Kind == EJobsReadKind::Offers ? bOffersActive : bGoalActive;
    }
    bool FJobsReadState::CaptureRead(EJobsReadKind Kind, FJobsReadReceipt& Out)
    {
        using namespace JobsStatePrivate;
        Out = {};
        if (!Enabled() || bBlocked
            || (Kind != EJobsReadKind::Job && Kind != EJobsReadKind::Offers && Kind != EJobsReadKind::Goal)) return false;
        if ((Kind == EJobsReadKind::Job && (!Context.IsSet() || HasError(JobError)))
            || (Kind == EJobsReadKind::Offers && (!Context.IsSet() || !View.Contact.IsSet() || HasError(OffersError)))
            || (Kind == EJobsReadKind::Goal && (GoalUserId.IsEmpty() || HasError(View.GoalError)))) return false;
        if (!ReadActive(Kind)) { ReadGeneration(Kind) = FGuid::NewGuid(); ReadActive(Kind) = true; }
        Out.SourcesGeneration = SourcesGeneration; Out.ReadGeneration = ReadGeneration(Kind); Out.Kind = Kind; return true;
    }
    bool FJobsReadState::Current(const FJobsReadReceipt& R) const
    {
        if (!Enabled() || bBlocked || R.SourcesGeneration != SourcesGeneration) return false;
        if (R.Kind == EJobsReadKind::Job) return bJobActive && R.ReadGeneration == JobGeneration && Context.IsSet();
        if (R.Kind == EJobsReadKind::Offers) return bOffersActive && R.ReadGeneration == OffersGeneration && Context.IsSet() && View.Contact.IsSet();
        return R.Kind == EJobsReadKind::Goal && bGoalActive && R.ReadGeneration == GoalGeneration && !GoalUserId.IsEmpty();
    }
    bool FJobsReadState::ReceiveJob(TOptional<FJobsJob> Job, bool bFromQuery, uint64 UpdatesAtStart)
    {
        using namespace JobsStatePrivate;
        if (!Context.IsSet()) return false;
        if (Job.IsSet() && (!Same(Job->LifeId, Context->LifeId) || !Same(Job->OutingId, Context->OutingId)
            || !Same(Job->Terms.WorldId, WorldId))) Job.Reset();
        const auto& Previous = View.Job;
        const bool bOlder = Previous.IsSet() && (!Job.IsSet()
            || (Same(Previous->Id, Job->Id) && (Previous->Revision > Job->Revision || (!bFromQuery && Previous->Revision == Job->Revision)))
            || (!Same(Previous->Id, Job->Id) && (Previous->AcceptedAtMs > Job->AcceptedAtMs || (!bFromQuery && UpdatesAtStart != JobUpdates))));
        const auto& Latest = bOlder ? Previous : Job;
        const bool bEnded = Ended(Latest);
        if (bEnded) { bMutationEligible = false; Pending.Reset(); CommandError.Reset(); }
        if (!SameProgress(Previous, Latest) || bEnded) View.Result.Reset();
        if (!bOlder) View.Job = MoveTemp(Job);
        View.JobPresence = View.Job.IsSet() ? EWorldFieldPresence::Value : EWorldFieldPresence::Null;
        View.bReady = true;
        return !bOlder;
    }
    EJobsReadResult FJobsReadState::Observe(const FJobsReadReceipt& Receipt, const FString& Json)
    {
        using namespace JobsStatePrivate;
        if (!Current(Receipt)) return EJobsReadResult::StaleReceipt;
        FString Error;
        bool bValid = false, bAccepted = true;
        if (Receipt.Kind == EJobsReadKind::Job)
        {
            TOptional<FJobsJob> JobValue;
            bValid = DecodeJobsJob(Json, JobValue, Error);
            if (bValid)
            {
                if (JobUpdates == MAX_uint64) { Revoke(TEXT("Reload courier details.")); return EJobsReadResult::InvalidResponse; }
                ++JobUpdates; JobError.Reset(); bAccepted = ReceiveJob(MoveTemp(JobValue), true, JobUpdates);
            }
        }
        else if (Receipt.Kind == EJobsReadKind::Offers)
        {
            TArray<FJobsOffer> Offers;
            bValid = DecodeJobsOffers(Json, Offers, Error);
            if (bValid)
            {
                View.Offers.Reset();
                for (auto& O : Offers) if (Same(O.Terms.WorldId, WorldId) && SameContact(O.Terms.Employer, *View.Contact)) View.Offers.Add(MoveTemp(O));
                View.bOffersReady = true; OffersError.Reset();
            }
        }
        else
        {
            FJobsGoal Goal;
            bValid = DecodeJobsGoal(Json, Goal, Error);
            if (bValid) { View.Goal = MoveTemp(Goal); View.bGoalReady = true; View.GoalError.Reset(); }
        }
        if (!bValid) { ObserveFailure(Receipt, {Error, false, false}); return EJobsReadResult::InvalidResponse; }
        RefreshError(); return bAccepted ? EJobsReadResult::Accepted : EJobsReadResult::Retained;
    }
    bool FJobsReadState::ObserveFailure(const FJobsReadReceipt& Receipt, const FJobsPublicFailure& Failure)
    {
        if (!Current(Receipt)) return false;
        if (Failure.bAccessFailure) { Revoke(Failure.Message); return true; }
        ReadGeneration(Receipt.Kind) = FGuid::NewGuid(); ReadActive(Receipt.Kind) = false;
        const FString Message = JobsStatePrivate::SafeMessage(Failure.Message, TEXT("Courier details couldn't load. Reload to retry."));
        if (Receipt.Kind == EJobsReadKind::Job)
        { JobError = Message; View.Job.Reset(); View.JobPresence = EWorldFieldPresence::Missing; View.Result.Reset(); View.bReady = true; }
        else if (Receipt.Kind == EJobsReadKind::Offers)
        { OffersError = Message; View.Offers.Reset(); View.bOffersReady = true; }
        else { View.GoalError = Message; View.Goal.Reset(); View.bGoalReady = true; }
        RefreshError(); return true;
    }
    bool FJobsReadState::SameScope(const FJobsMutationReceipt& R) const
    {
        return Enabled() && !bBlocked && bMutationEligible && R.Generation == Generation && Context.IsSet()
            && OriginMatchesContext(R.Origin)
            && (!R.PendingId.IsValid() || (Pending.IsSet() && Pending->Id == R.PendingId));
    }
    bool FJobsReadState::SameVisit(const FJobsMutationReceipt& R) const
    {
        return SameScope(R) && JobsStatePrivate::Same(R.InstanceId, Context->InstanceId)
            && R.WorldRevision == Context->WorldRevision && R.PlayerStatus == Context->PlayerStatus && R.RenderAttempt == Context->RenderAttempt;
    }
    bool FJobsReadState::FeedbackCurrent(const FJobsMutationReceipt& R) const
    {
        using namespace JobsStatePrivate;
        // Separate from pending eligibility: canonical receipt completion clears pending first.
        return Enabled() && !bBlocked && R.Generation == Generation && Context.IsSet()
            && Same(R.InstanceId, Context->InstanceId) && R.WorldRevision == Context->WorldRevision
            && R.PlayerStatus == Context->PlayerStatus && R.RenderAttempt == Context->RenderAttempt
            && R.ContactGeneration == ContactGeneration && SameContact(R.Origin.Contact, View.Contact);
    }
    bool FJobsReadState::CaptureMutation(const FJobsRequestOrigin& Origin, FJobsMutationReceipt& Out)
    {
        Out = {};
        if (Pending.IsSet() || ActiveGoalMutation.IsValid()) return false;
        return BeginMutation(Origin, Out);
    }
    bool FJobsReadState::BeginMutation(const FJobsRequestOrigin& Origin, FJobsMutationReceipt& Out)
    {
        Out = {};
        if (!Enabled() || bBlocked || ActiveMutation.IsValid() || !CanWork()
            || ActiveGoalMutation.IsValid() || !OriginMatchesContext(Origin)) return false;
        Out.Id = FGuid::NewGuid(); Out.Generation = Generation; Out.ContactGeneration = ContactGeneration; Out.Origin = Origin;
        Out.InstanceId = Context->InstanceId; Out.WorldRevision = Context->WorldRevision;
        Out.PlayerStatus = Context->PlayerStatus; Out.RenderAttempt = Context->RenderAttempt; Out.JobUpdates = JobUpdates;
        ActiveMutation = Out.Id; ActiveOrigin = Origin; bMutationEligible = true;
        CommandError.Reset(); View.Result.Reset(); RefreshError(); return true;
    }
    void FJobsReadState::FinishReceipt()
    {
        ActivePendingId.Invalidate(); bCommandDispatched = false;
        ActiveMutation.Invalidate(); ActiveOrigin = {}; bMutationEligible = false; RefreshError();
    }
    EJobsReadResult FJobsReadState::ObserveMutation(const FJobsMutationReceipt& R, const FString& Json)
    {
        FJobsCompletionReceipt Ignored;
        return ObserveMutation(R, Json, Ignored);
    }
    EJobsReadResult FJobsReadState::ObserveMutation(const FJobsMutationReceipt& R, const FString& Json,
        FJobsCompletionReceipt& Out)
    {
        using namespace JobsStatePrivate;
        Out = {};
        if (!R.Id.IsValid() || R.Id != ActiveMutation || bDisposed) return EJobsReadResult::StaleReceipt;
        if (ActivePendingId.IsValid() && !bCommandDispatched) return EJobsReadResult::StaleReceipt;
        if (R.PendingId.IsValid()) Reconcile();
        if (!SameVisit(R)) { FinishReceipt(); return EJobsReadResult::StaleReceipt; }
        FJobsCommandResponse Wire;
        FString Error;
        if (!DecodeJobsCommandResponse(Json, Wire, Error))
        {
            if (FeedbackCurrent(R)) CommandError = TEXT("The courier result wasn't received. Retry the same action.");
            FinishReceipt(); return EJobsReadResult::InvalidResponse;
        }
        const auto& P = *Wire.Player;
        if (!Same(P.UserId, R.Origin.UserId) || !Same(P.GameRoomId, R.Origin.GameRoomId)
            || !Same(P.InstanceId, R.InstanceId) || P.Revision != R.WorldRevision || P.Status != R.PlayerStatus)
        { FinishReceipt(); return EJobsReadResult::StaleReceipt; }
        const bool bAccepted = !HasError(JobError) && ReceiveJob(Wire.Result.Job, false, R.JobUpdates);
        if (R.PendingId.IsValid() && Pending.IsSet() && Pending->Id == R.PendingId) Pending.Reset();
        if (!HasError(JobError) && FeedbackCurrent(R) && (Wire.Result.Feedback.Outcome == EJobsOutcome::Blocked
            || (View.Job.IsSet() && SameProgress(View.Job, Wire.Result.Job) && !Ended(View.Job))))
        {
            View.Result = Wire.Result.Feedback;
            Out.Mutation = R; Out.Result = FJobsCommandResult{Wire.Result.Feedback, View.Job};
        }
        FinishReceipt(); return bAccepted ? EJobsReadResult::Accepted : EJobsReadResult::Retained;
    }
    bool FJobsReadState::ObserveMutationFailure(const FJobsMutationReceipt& R, const FJobsPublicFailure& Failure)
    {
        if (!R.Id.IsValid() || R.Id != ActiveMutation || bDisposed) return false;
        if (ActivePendingId.IsValid() && !bCommandDispatched) return false;
        if (R.PendingId.IsValid()) Reconcile();
        if (SameScope(R) && Failure.bAccessFailure) Revoke(Failure.Message);
        else if (SameVisit(R))
        {
            if (R.PendingId.IsValid() && Failure.bDefinitivelyRejected && !R.bWasUncertain)
                Pending.Reset();
            if (FeedbackCurrent(R)) CommandError = Failure.bDefinitivelyRejected
                    ? JobsStatePrivate::SafeMessage(Failure.Message, TEXT("The courier action couldn't finish."))
                    : FString(TEXT("The courier result wasn't received. Retry the same action."));
        }
        FinishReceipt(); return true;
    }

    bool FJobsReadState::RejectCommand(const TCHAR* Message)
    {
        CommandError = Message; RefreshError(); return false;
    }
    bool FJobsReadState::PrepareCommand(const FJobsAction& Action, FJobsMutationReceipt& Out)
    {
        using namespace JobsStatePrivate;
        Out = {};
        Reconcile();
        if (!Enabled() || bBlocked || ActiveMutation.IsValid() || ActiveGoalMutation.IsValid()) return false;
        if (Pending.IsSet()) return RejectCommand(TEXT("Retry the pending courier action first."));
        if (!CanWork() || !View.bReady || HasError(JobError))
            return RejectCommand(TEXT("Wait for your current world, life and haul to load."));
        const auto Contact = ResolveContact(SelectedService);
        const auto& Job = View.Job;
        const TCHAR* Type = nullptr;
        switch (Action.Type)
        {
        case EJobsAction::Accept: Type = TEXT("accept"); break;
        case EJobsAction::Collect: Type = TEXT("collect"); break;
        case EJobsAction::Deliver: Type = TEXT("deliver"); break;
        case EJobsAction::Cancel: Type = TEXT("cancel"); break;
        default: return RejectCommand(TEXT("This courier job has changed."));
        }
        if (Action.Type == EJobsAction::Accept)
        {
            const FJobsOffer* Offer = View.Offers.FindByPredicate([&Action](const FJobsOffer& Value)
                { return Same(Value.Id, Action.Id); });
            if (!Contact.IsSet() || !View.bOffersReady || HasError(OffersError) || !Offer || !Offer->bAvailable
                || !SameContact(*Contact, Offer->Terms.Employer))
                return RejectCommand(TEXT("Choose an available offer at this contact."));
            if (Offer->Terms.RewardApproval != EJobsRewardApproval::Approved)
                return RejectCommand(TEXT("This courier reward is unavailable."));
            if (Job.IsSet() && (Job->Status == EJobsStatus::Accepted || Job->Status == EJobsStatus::Carrying))
                return RejectCommand(TEXT("Finish or cancel the current courier job first."));
        }
        else
        {
            const bool bNext = Job.IsSet() && ((Action.Type == EJobsAction::Collect && Job->Status == EJobsStatus::Accepted)
                || (Action.Type == EJobsAction::Deliver && Job->Status == EJobsStatus::Carrying)
                || (Action.Type == EJobsAction::Cancel && (Job->Status == EJobsStatus::Accepted || Job->Status == EJobsStatus::Carrying)));
            if (!bNext || !Same(Job->Id, Action.Id)) return RejectCommand(TEXT("This courier job has changed."));
            if (Action.Type != EJobsAction::Cancel && (!Contact.IsSet()
                || !SameContact(*Contact, Action.Type == EJobsAction::Collect ? Job->Terms.Employer : Job->Terms.Destination)))
                return RejectCommand(TEXT("Visit this job's current contact."));
        }
        // Read tokens can include MAX_SAFE_INTEGER; command validators stop one below it.
        // Reject unsupported capture without truncating or substituting any source token.
        if (Action.Id.IsEmpty() || Action.Id.Len() > 240 || Context->WorldRevision < 0 || Context->WorldRevision >= MaxInteger
            || Context->InstanceId.Len() > 160 || Context->InstanceId.TrimStartAndEnd().IsEmpty()
            || Context->LifeId.Len() > 160 || Context->LifeId.TrimStartAndEnd().IsEmpty()
            || (Action.Type != EJobsAction::Accept && (Job->Revision < 1 || Job->Revision >= MaxInteger)))
            return RejectCommand(TEXT("The courier action couldn't be prepared. Reload courier details."));

        FPendingCommand Request;
        Request.Id = FGuid::NewGuid();
        Request.Origin = {AccountId, RoomId, Context->UserId, Context->LifeId, Context->OutingId, Contact};
        Request.Dispatch.Endpoint = TEXT("companionJobs:command");
        const auto Observed = MakeShared<FJsonObject>();
        Observed->SetStringField(TEXT("instanceId"), Context->InstanceId);
        Observed->SetNumberField(TEXT("worldRevision"), static_cast<double>(Context->WorldRevision));
        Observed->SetStringField(TEXT("lifeId"), Context->LifeId);
        Observed->SetStringField(TEXT("outingId"), Context->OutingId);
        const auto Command = MakeShared<FJsonObject>();
        Command->SetStringField(TEXT("type"), Type);
        Command->SetStringField(TEXT("requestId"), FString(TEXT("courier-")) + Request.Id.ToString(EGuidFormats::DigitsWithHyphens));
        Command->SetObjectField(TEXT("observed"), Observed);
        if (Action.Type == EJobsAction::Accept) Command->SetStringField(TEXT("offerId"), Action.Id);
        else
        {
            Command->SetStringField(TEXT("jobId"), Job->Id);
            Command->SetNumberField(TEXT("jobRevision"), static_cast<double>(Job->Revision));
        }
        const auto Arguments = MakeShared<FJsonObject>();
        Arguments->SetStringField(TEXT("gameRoomId"), RoomId);
        if (Action.Type != EJobsAction::Cancel) Arguments->SetStringField(TEXT("serviceId"), Contact->ServiceId);
        Arguments->SetObjectField(TEXT("command"), Command);
        if (!FJsonSerializer::Serialize(Arguments, TJsonWriterFactory<>::Create(&Request.Dispatch.ArgumentsJson)))
            return RejectCommand(TEXT("The courier action couldn't be prepared. Reload courier details."));
        Pending = MoveTemp(Request);
        return PreparePending(Out);
    }
    bool FJobsReadState::PreparePending(FJobsMutationReceipt& Out)
    {
        Out = {};
        if (!Pending.IsSet() || !BeginMutation(Pending->Origin, Out)) return false;
        Out.PendingId = Pending->Id; Out.bWasUncertain = Pending->bUncertain;
        ActivePendingId = Pending->Id; bCommandDispatched = false;
        return true;
    }
    bool FJobsReadState::PrepareRetry(FJobsMutationReceipt& Out)
    {
        Reconcile();
        // No offer, stage or contact revalidation: the original request is immutable.
        return PreparePending(Out);
    }
    bool FJobsReadState::MarkCommandDispatched(const FJobsMutationReceipt& R, FJobsDispatch& Out)
    {
        Out = {};
        if (bDisposed || !R.Id.IsValid() || R.Id != ActiveMutation || !R.PendingId.IsValid()
            || R.PendingId != ActivePendingId || bCommandDispatched) return false;
        Reconcile();
        if (!SameVisit(R)) { FinishReceipt(); return false; }
        Out = Pending->Dispatch;
        Pending->bUncertain = true; bCommandDispatched = true;
        return true;
    }
    bool FJobsReadState::CancelPreparedCommand(const FJobsMutationReceipt& R)
    {
        if (bDisposed || !R.Id.IsValid() || R.Id != ActiveMutation || !R.PendingId.IsValid()
            || R.PendingId != ActivePendingId || bCommandDispatched) return false;
        // A cancelled setup cannot resolve either a new or previously uncertain request.
        FinishReceipt(); return true;
    }
    bool FJobsReadState::TakeCommandResult(FJobsCompletionReceipt& Completion, FJobsCommandResult& Out) const
    {
        using namespace JobsStatePrivate;
        const auto Captured = MoveTemp(Completion);
        Completion = {}; Out = {};
        if (!Captured.Result.IsSet() || !FeedbackCurrent(Captured.Mutation) || HasError(JobError)
            || !SameProgress(Captured.Result->Job, View.Job)
            || (Captured.Result->Feedback.Outcome != EJobsOutcome::Blocked && Ended(View.Job))) return false;
        Out = *Captured.Result; Out.Job = View.Job; return true;
    }

    bool FJobsReadState::PrepareGoalSelection(bool bSelected, FJobsGoalMutationReceipt& Out)
    {
        using namespace JobsStatePrivate;
        Out = {};
        Reconcile();
        if (!Enabled() || bBlocked || ActiveGoalMutation.IsValid() || ActiveMutation.IsValid() || Pending.IsSet()
            || !View.bGoalReady || HasError(View.GoalError) || !View.Goal.IsSet() || !View.Goal->Resident.IsSet()
            || !Sources.Player.IsValid() || !Same(Sources.Player->GameRoomId, RoomId)
            || (bSelected && !View.Goal->bInCurrentWorld)) return false;
        FJobsGoalMutationReceipt R;
        R.Id = FGuid::NewGuid(); R.Generation = Generation;
        R.AccountId = AccountId; R.GameRoomId = RoomId; R.GoalUserId = GoalUserId;
        R.Dispatch.Endpoint = TEXT("companionJobs:selectGoal");
        const auto Arguments = MakeShared<FJsonObject>();
        Arguments->SetStringField(TEXT("gameRoomId"), RoomId);
        Arguments->SetBoolField(TEXT("selected"), bSelected);
        Arguments->SetStringField(TEXT("actorIdentity"), View.Goal->Resident->ActorIdentity);
        if (!FJsonSerializer::Serialize(Arguments, TJsonWriterFactory<>::Create(&R.Dispatch.ArgumentsJson)))
        {
            View.GoalError = TEXT("The courier goal couldn't be saved. Reload its saved state before trying again.");
            return false;
        }
        ActiveGoalMutation = R.Id; bGoalDispatched = false;
        View.bGoalBusy = true; View.GoalError.Reset(); Out = MoveTemp(R); RefreshError(); return true;
    }
    bool FJobsReadState::CurrentGoalMutation(const FJobsGoalMutationReceipt& R) const
    {
        using namespace JobsStatePrivate;
        // Goal query generation and resident changes do not retire this account/room save.
        return Enabled() && !bBlocked && R.Generation == Generation && Same(R.AccountId, AccountId)
            && Same(R.GameRoomId, RoomId) && Same(R.GoalUserId, GoalUserId);
    }
    void FJobsReadState::FinishGoalMutation()
    {
        ActiveGoalMutation.Invalidate(); bGoalDispatched = false;
        View.bGoalBusy = false; RefreshError();
    }
    bool FJobsReadState::MarkGoalDispatched(const FJobsGoalMutationReceipt& R, FJobsDispatch& Out)
    {
        Out = {};
        if (bDisposed || !R.Id.IsValid() || R.Id != ActiveGoalMutation || bGoalDispatched) return false;
        Reconcile();
        if (!CurrentGoalMutation(R)) { FinishGoalMutation(); return false; }
        Out = R.Dispatch; bGoalDispatched = true; return true;
    }
    bool FJobsReadState::CancelPreparedGoal(const FJobsGoalMutationReceipt& R)
    {
        if (bDisposed || !R.Id.IsValid() || R.Id != ActiveGoalMutation || bGoalDispatched) return false;
        FinishGoalMutation(); return true;
    }
    EJobsReadResult FJobsReadState::ObserveGoalMutation(const FJobsGoalMutationReceipt& R, const FString& Json)
    {
        using namespace JobsStatePrivate;
        if (bDisposed || !R.Id.IsValid() || R.Id != ActiveGoalMutation || !bGoalDispatched) return EJobsReadResult::StaleReceipt;
        Reconcile();
        if (!CurrentGoalMutation(R)) { FinishGoalMutation(); return EJobsReadResult::StaleReceipt; }
        FValue Value;
        const bool bValid = ParseNull(Json, Value, {64, 2, 4});
        if (!bValid) View.GoalError = TEXT("The courier goal couldn't be saved. Reload its saved state before trying again.");
        // A successful save never publishes or clears query-owned goal/error state.
        FinishGoalMutation(); return bValid ? EJobsReadResult::Accepted : EJobsReadResult::InvalidResponse;
    }
    bool FJobsReadState::ObserveGoalMutationFailure(const FJobsGoalMutationReceipt& R, const FJobsPublicFailure& Failure)
    {
        if (bDisposed || !R.Id.IsValid() || R.Id != ActiveGoalMutation || !bGoalDispatched) return false;
        Reconcile();
        if (CurrentGoalMutation(R))
        {
            if (Failure.bAccessFailure) Revoke(Failure.Message);
            else View.GoalError = JobsStatePrivate::SafeMessage(Failure.Message,
                TEXT("The courier goal couldn't be saved. Reload its saved state before trying again."));
        }
        FinishGoalMutation(); return true;
    }
    void FJobsReadState::ClearOwnedRequests()
    {
        Pending.Reset();
        if (ActivePendingId.IsValid()) FinishReceipt();
        FinishGoalMutation();
    }
}
