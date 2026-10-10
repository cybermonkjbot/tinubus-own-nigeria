#include "BaddieCompanionLifeState.h"
#include "BaddieCompanionStateJson.h"
#include <cmath>

namespace BaddieCompanion
{
    namespace LifePrivate
    {
        using namespace StateJsonPrivate;
        constexpr FParseLimits Limits { 262144, 32, 8192 };
        constexpr int32 TaskLimit = 8, MaxExperience = 300;
        const TCHAR* InvalidRead = TEXT("Care data couldn't load. Reopen your space to try again.");

        bool Text(const FValue& V, FString& Out, int32 Maximum = 100, bool bAllowEmpty = false)
        {
            return String(V, Out) && (bAllowEmpty || !Out.IsEmpty()) && Out.Len() <= Maximum;
        }
        bool Id(const FValue& V, FString& Out) { return Text(V, Out); }
        bool Reason(const FValue& V, FString& Out) { return Text(V, Out, 240); }
        bool RoomKey(const FValue& V, FString& Out) { return Text(V, Out, 5000, true); }
        // validCompanionLife does not cap these times at MAX_SAFE_INTEGER.
        bool Time(const FValue& V, double& Out) { return Number(V, Out) && Out >= 0; }
        template <typename T, typename F> bool Optional(const FValue& V, TOptional<T>& Out, F Read)
        {
            if (!V.IsValid()) return true;
            T Next;
            if (!Read(V, Next)) return false;
            Out = MoveTemp(Next);
            return true;
        }
        bool Kind(const FValue& V, ELifeActivityKind& Out)
        {
            return EnumValue(V, Out, {
                { TEXT("sleep"), ELifeActivityKind::Sleep }, { TEXT("sit"), ELifeActivityKind::Sit },
                { TEXT("play"), ELifeActivityKind::Play }, { TEXT("cook"), ELifeActivityKind::Cook },
                { TEXT("eat"), ELifeActivityKind::Eat }, { TEXT("bathe"), ELifeActivityKind::Bathe },
                { TEXT("work"), ELifeActivityKind::Work }, { TEXT("watch-tv"), ELifeActivityKind::WatchTv },
                { TEXT("read"), ELifeActivityKind::Read }, { TEXT("stretch"), ELifeActivityKind::Stretch },
                { TEXT("wash"), ELifeActivityKind::Wash } });
        }
        bool Phase(const FValue& V, ELifeActivityPhase& Out)
        {
            return EnumValue(V, Out, { { TEXT("approaching"), ELifeActivityPhase::Approaching },
                { TEXT("active"), ELifeActivityPhase::Active } });
        }
        bool Need(const FValue& V, double& Out) { return Number(V, Out) && Out >= 0 && Out <= 100; }
        bool Needs(const FValue& V, FLifeNeeds& Out)
        {
            const auto O = Object(V);
            return Only(O, { TEXT("hunger"), TEXT("energy"), TEXT("cleanliness"), TEXT("enjoyment") })
                && Need(Field(O, TEXT("hunger")), Out.Hunger) && Need(Field(O, TEXT("energy")), Out.Energy)
                && Need(Field(O, TEXT("cleanliness")), Out.Cleanliness) && Need(Field(O, TEXT("enjoyment")), Out.Enjoyment);
        }
        bool Activity(const FValue& V, FLifeActivity& Out)
        {
            const auto O = Object(V);
            return Only(O, { TEXT("id"), TEXT("kind"), TEXT("roomId"), TEXT("stationId"), TEXT("startedAt"),
                    TEXT("endsAt"), TEXT("phase"), TEXT("requestedAt"), TEXT("roomKey") })
                && Id(Field(O, TEXT("id")), Out.Id) && Kind(Field(O, TEXT("kind")), Out.Kind)
                && Id(Field(O, TEXT("roomId")), Out.RoomId) && Id(Field(O, TEXT("stationId")), Out.StationId)
                && Time(Field(O, TEXT("startedAt")), Out.StartedAtMs) && Number(O, TEXT("endsAt"), Out.EndsAtMs)
                && Out.EndsAtMs > Out.StartedAtMs && Out.EndsAtMs - Out.StartedAtMs <= 120000
                && Optional(Field(O, TEXT("phase")), Out.Phase, Phase)
                && Optional(Field(O, TEXT("requestedAt")), Out.RequestedAtMs, Time)
                && (!Out.RequestedAtMs.IsSet() || Out.RequestedAtMs.GetValue() <= Out.StartedAtMs)
                && Optional(Field(O, TEXT("roomKey")), Out.RoomKey, RoomKey);
        }
        bool Cooking(const FValue& V, FLifeTaskCooking& Out)
        {
            const auto O = Object(V);
            int64 Batches = 0;
            if (!Only(O, { TEXT("recipeKey"), TEXT("batches") })
                || !Text(Field(O, TEXT("recipeKey")), Out.RecipeKey, 80)
                || !Integer(Field(O, TEXT("batches")), Batches, 1) || Batches > 2) return false;
            Out.Batches = static_cast<int32>(Batches);
            return true;
        }
        bool Task(const FValue& V, FLifeTask& Out)
        {
            const auto O = Object(V);
            return Only(O, { TEXT("id"), TEXT("roomId"), TEXT("stationId"), TEXT("kind"), TEXT("status"),
                    TEXT("activityId"), TEXT("reason"), TEXT("cooking") })
                && Id(Field(O, TEXT("id")), Out.Id) && Text(Field(O, TEXT("roomId")), Out.RoomId, 80)
                && Text(Field(O, TEXT("stationId")), Out.StationId, 80) && Kind(Field(O, TEXT("kind")), Out.Kind)
                && EnumValue(Field(O, TEXT("status")), Out.Status, {
                    { TEXT("waiting"), ELifeTaskStatus::Waiting }, { TEXT("active"), ELifeTaskStatus::Active },
                    { TEXT("blocked"), ELifeTaskStatus::Blocked } })
                && Nullable(Field(O, TEXT("activityId")), Out.ActivityId, Id)
                && Nullable(Field(O, TEXT("reason")), Out.Reason, Reason)
                && Optional(Field(O, TEXT("cooking")), Out.Cooking, Cooking)
                && (!Out.Cooking.IsSet() || Out.Kind == ELifeActivityKind::Cook);
        }
        bool Queue(const FValue& V, FLifeTaskQueue& Out)
        {
            const auto O = Object(V);
            if (!Only(O, { TEXT("tasks"), TEXT("paused"), TEXT("reason") })
                || !Boolean(Field(O, TEXT("paused")), Out.bPaused)
                || !Nullable(Field(O, TEXT("reason")), Out.Reason, Reason)) return false;
            const auto Tasks = Field(O, TEXT("tasks"));
            if (!Tasks.IsValid() || Tasks->Type != EJson::Array || Tasks->AsArray().Num() > TaskLimit) return false;
            for (const auto& VTask : Tasks->AsArray())
            {
                FLifeTask Next;
                if (!Task(VTask, Next)) return false;
                for (const auto& Prior : Out.Tasks) if (Same(Prior.Id, Next.Id)) return false;
                if (Next.Status == ELifeTaskStatus::Active)
                {
                    if (!Out.Tasks.IsEmpty() || !Next.ActivityId.IsSet()) return false;
                }
                else if (Next.ActivityId.IsSet()) return false;
                Out.Tasks.Add(MoveTemp(Next));
            }
            return true;
        }
        bool Progress(const FValue& V, FLifeSkillProgress& Out)
        {
            const auto O = Object(V);
            int64 Experience = 0;
            if (!Only(O, { TEXT("experience") }) || !Integer(Field(O, TEXT("experience")), Experience)
                || Experience > MaxExperience) return false;
            Out.Experience = static_cast<int32>(Experience);
            return true;
        }
        bool Skills(const FValue& V, FLifeSkills& Out)
        {
            const auto O = Object(V);
            return Only(O, { TEXT("cooking"), TEXT("fighting"), TEXT("selfDefence"), TEXT("charisma"),
                    TEXT("fitness"), TEXT("hustle"), TEXT("dance"), TEXT("flirting") })
                && Progress(Field(O, TEXT("cooking")), Out.Cooking)
                && Optional(Field(O, TEXT("fighting")), Out.Fighting, Progress)
                && Optional(Field(O, TEXT("selfDefence")), Out.SelfDefence, Progress)
                && Optional(Field(O, TEXT("charisma")), Out.Charisma, Progress)
                && Optional(Field(O, TEXT("fitness")), Out.Fitness, Progress)
                && Optional(Field(O, TEXT("hustle")), Out.Hustle, Progress)
                && Optional(Field(O, TEXT("dance")), Out.Dance, Progress)
                && Optional(Field(O, TEXT("flirting")), Out.Flirting, Progress);
        }
        bool Life(const FObject& O, FLife& Out)
        {
            int64 Meals = 0;
            if (!Only(O, { TEXT("characterId"), TEXT("needs"), TEXT("updatedAt"), TEXT("revision"), TEXT("activity"),
                    TEXT("meals"), TEXT("reminders"), TEXT("taskQueue"), TEXT("skills") })
                || !Id(Field(O, TEXT("characterId")), Out.CharacterId) || !Needs(Field(O, TEXT("needs")), Out.Needs)
                || !Time(Field(O, TEXT("updatedAt")), Out.UpdatedAtMs) || !Integer(Field(O, TEXT("revision")), Out.Revision)
                || !Nullable(Field(O, TEXT("activity")), Out.Activity, Activity)
                || !Integer(Field(O, TEXT("meals")), Meals) || Meals > 3
                || !Boolean(Field(O, TEXT("reminders")), Out.bReminders)
                || !Optional(Field(O, TEXT("taskQueue")), Out.TaskQueue, Queue)
                || !Optional(Field(O, TEXT("skills")), Out.Skills, Skills)) return false;
            Out.Meals = static_cast<int32>(Meals);
            return true;
        }
        FLife Fresh(const FString& CharacterId, double ConnectedAtMs)
        {
            FLife Next;
            Next.CharacterId = CharacterId;
            Next.Needs.Hunger = 20;
            Next.Needs.Energy = 85;
            Next.Needs.Cleanliness = 90;
            Next.Needs.Enjoyment = 80;
            Next.UpdatedAtMs = ConnectedAtMs;
            // newCompanionLife has no taskQueue, no activity, zero meals/revision and reminders off.
            FLifeSkills InitialSkills;
            InitialSkills.Fighting = FLifeSkillProgress{};
            InitialSkills.SelfDefence = FLifeSkillProgress{};
            InitialSkills.Charisma = FLifeSkillProgress{};
            InitialSkills.Fitness = FLifeSkillProgress{};
            InitialSkills.Hustle = FLifeSkillProgress{};
            InitialSkills.Dance = FLifeSkillProgress{};
            InitialSkills.Flirting = FLifeSkillProgress{};
            Next.Skills = MoveTemp(InitialSkills);
            return Next;
        }
        bool SameScope(const FLifeReadScope& A, const FLifeReadScope& B)
        {
            return Same(A.AccountId, B.AccountId) && Same(A.CharacterId, B.CharacterId);
        }
    }

    bool DecodeLifeResponse(const FString& Json, TOptional<FLife>& Out, FString& OutError)
    {
        using namespace StateJsonPrivate;
        Out.Reset();
        OutError = LifePrivate::InvalidRead;
        FValue Null;
        if (ParseNull(Json, Null, LifePrivate::Limits)) { OutError.Reset(); return true; }
        FObject ObjectRoot;
        FLife Next;
        if (!Parse(Json, ObjectRoot, LifePrivate::Limits) || !LifePrivate::Life(ObjectRoot, Next)) return false;
        Out = MoveTemp(Next);
        OutError.Reset();
        return true;
    }
    bool FLifeReadState::Select(const FString& AccountId, const FString& CharacterId)
    {
        if (bDisposed) return false;
        const FLifeReadScope Next { AccountId, CharacterId };
        if (LifePrivate::SameScope(Selection, Next)) return true;
        EndSubscription();
        Selection = Next;
        View = FLifeReadView{};
        Changed.Broadcast();
        return true;
    }
    bool FLifeReadState::BeginSubscription(double ConnectedAtMs, FLifeReadReceipt& Out)
    {
        Out = FLifeReadReceipt{};
        EndSubscription();
        if (bDisposed || Selection.AccountId.IsEmpty() || Selection.CharacterId.IsEmpty()
            || Selection.CharacterId.Len() > 100 || !std::isfinite(ConnectedAtMs) || ConnectedAtMs < 0) return false;
        bSubscribed = true;
        Out.Generation = Generation;
        Out.Scope = Selection;
        Out.Fresh = MakeShared<FLife>(LifePrivate::Fresh(Selection.CharacterId, ConnectedAtMs));
        return true;
    }
    void FLifeReadState::EndSubscription()
    {
        Generation = FGuid::NewGuid();
        bSubscribed = false;
    }
    bool FLifeReadState::Current(const FLifeReadReceipt& Receipt) const
    {
        return !bDisposed && bSubscribed && Receipt.Generation.IsValid() && Receipt.Generation == Generation
            && Receipt.Scope.IsSet() && Receipt.Fresh.IsValid() && LifePrivate::SameScope(Selection, Receipt.Scope.GetValue());
    }
    void FLifeReadState::FailRead(const FString& Message)
    {
        View = FLifeReadView{};
        View.Error = Message;
        Changed.Broadcast();
    }
    ELifeReadResult FLifeReadState::Accept(const FLifeReadReceipt& Receipt, const FString& Json)
    {
        if (!Current(Receipt)) return ELifeReadResult::StaleReceipt;
        TOptional<FLife> Decoded;
        FString Error;
        if (!DecodeLifeResponse(Json, Decoded, Error))
        {
            FailRead(Error);
            return ELifeReadResult::InvalidResponse;
        }
        const bool bNull = !Decoded.IsSet();
        TSharedPtr<const FLife> Next = Receipt.Fresh;
        if (!bNull) Next = MakeShared<FLife>(MoveTemp(Decoded.GetValue()));
        if (!StateJsonPrivate::Same(Next->CharacterId, Selection.CharacterId)) return ELifeReadResult::CharacterMismatch;
        // Canonical comparison is to the CURRENT displayed life, not a permanent high-water mark.
        // A read failure clears life, so a later valid revision zero or null default can recover.
        if (Next->Revision < (View.Life.IsValid() ? View.Life->Revision : 0)) return ELifeReadResult::OlderRevision;
        View.Life = MoveTemp(Next);
        View.Origin = bNull ? ELifeReadOrigin::DefaultForNull : ELifeReadOrigin::SavedValue;
        View.Error.Reset();
        Changed.Broadcast();
        return ELifeReadResult::Accepted;
    }
    bool FLifeReadState::AcceptFailure(const FLifeReadReceipt& Receipt, const FLifePublicReadFailure& Failure)
    {
        if (!Current(Receipt)) return false;
        const FString Message = Failure.Message;
        FailRead(Message);
        return true;
    }
    void FLifeReadState::Clear()
    {
        if (bDisposed) return;
        EndSubscription();
        Selection = FLifeReadScope{};
        View = FLifeReadView{};
        Changed.Broadcast();
    }
    void FLifeReadState::Dispose()
    {
        if (bDisposed) return;
        bDisposed = true;
        EndSubscription();
        Selection = FLifeReadScope{};
        View = FLifeReadView{};
        Changed.Clear();
    }
}
