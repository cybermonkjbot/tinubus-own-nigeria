#include "BaddieCompanionLifeState.h"
#include "BaddieCompanionStateJson.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include <cmath>

namespace BaddieCompanion
{
    namespace LifePrivate
    {
        using namespace StateJsonPrivate;
        constexpr FParseLimits Limits { 262144, 32, 8192 };
        constexpr int32 TaskLimit = 8, MaxExperience = 300;
        const TCHAR* InvalidRead = TEXT("Care data couldn't load. Reopen your space to try again.");
        const TCHAR* InvalidCare = TEXT("Care response could not be verified. Retry the previous action.");
        const TCHAR* InvalidTask = TEXT("Task data couldn't load. Reopen your space to try again.");
        const TCHAR* BlockedActivity = TEXT("The character couldn't reach this activity. Move the blocking furniture and try again.");

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
        bool BoundedText(const FString& Value, int32 Maximum, bool bAllowEmpty = false)
        {
            return (bAllowEmpty || !Value.IsEmpty()) && Value.Len() <= Maximum;
        }
        bool RequestId(const FString& Value) { return Value.Len() >= 8 && Value.Len() <= 100; }
        bool CommandScope(const FLifeReadScope& Scope, int64 Revision)
        {
            return !Scope.AccountId.IsEmpty() && BoundedText(Scope.CharacterId, 80)
                && Revision >= 0 && Revision < MaxInteger;
        }
        bool WriteCooking(const TOptional<FLifeTaskCooking>& Value, const TSharedRef<FJsonObject>& Parent)
        {
            if (!Value.IsSet()) return true;
            if (Value->Batches != 1 && Value->Batches != 2) return false;
            const auto O = MakeShared<FJsonObject>();
            O->SetStringField(TEXT("recipeKey"), Value->RecipeKey);
            O->SetNumberField(TEXT("batches"), Value->Batches);
            Parent->SetObjectField(TEXT("cooking"), O);
            return true;
        }
        TSharedRef<FJsonObject> RequestArguments(const FLifeReadScope& Scope, int64 Revision, const FString& Id)
        {
            const auto O = MakeShared<FJsonObject>();
            O->SetStringField(TEXT("characterId"), Scope.CharacterId);
            O->SetNumberField(TEXT("expectedRevision"), static_cast<double>(Revision));
            O->SetStringField(TEXT("requestId"), Id);
            return O;
        }
        bool EncodeCare(const FLifeReadScope& Scope, int64 Revision, const FLifeCareIntent& Intent, const FString& Id, FString& Json)
        {
            if (!CommandScope(Scope, Revision) || !RequestId(Id) || !BoundedText(Intent.RoomId, 80, true)
                || !BoundedText(Intent.StationId, 80, true)) return false;
            if (Intent.Action == ELifeCareAction::Start && Intent.Cooking.IsSet() && Intent.Cooking->RecipeKey.Len() > 80) return false;
            const TCHAR* Action = nullptr;
            switch (Intent.Action)
            {
            case ELifeCareAction::Start: Action = TEXT("start"); break;
            case ELifeCareAction::Cancel: Action = TEXT("cancel"); break;
            case ELifeCareAction::RemindersOn: Action = TEXT("reminders-on"); break;
            case ELifeCareAction::RemindersOff: Action = TEXT("reminders-off"); break;
            default: return false;
            }
            const auto O = RequestArguments(Scope, Revision, Id);
            O->SetStringField(TEXT("roomId"), Intent.RoomId);
            O->SetStringField(TEXT("stationId"), Intent.StationId);
            O->SetStringField(TEXT("action"), Action);
            if (!WriteCooking(Intent.Cooking, O)) return false;
            return FJsonSerializer::Serialize(O, TJsonWriterFactory<>::Create(&Json));
        }
        bool EncodeReport(const FLifeReadScope& Scope, const FLifeActivityReport& Report, FString& Json)
        {
            if (Scope.AccountId.IsEmpty() || !BoundedText(Scope.CharacterId, 80) || !RequestId(Report.ActivityId)) return false;
            const TCHAR* Status = nullptr;
            switch (Report.Status)
            {
            case ELifeReportStatus::Arrived: Status = TEXT("arrived"); break;
            case ELifeReportStatus::Blocked: Status = TEXT("blocked"); break;
            case ELifeReportStatus::Interrupted: Status = TEXT("interrupted"); break;
            default: return false;
            }
            const auto O = MakeShared<FJsonObject>();
            O->SetStringField(TEXT("characterId"), Scope.CharacterId);
            O->SetStringField(TEXT("activityId"), Report.ActivityId);
            O->SetStringField(TEXT("status"), Status);
            return FJsonSerializer::Serialize(O, TJsonWriterFactory<>::Create(&Json));
        }
        bool EncodeTask(const FLifeReadScope& Scope, int64 Revision, const FLifeTaskCommand& Command, const FString& Id, FString& Json)
        {
            if (!CommandScope(Scope, Revision) || !RequestId(Id)) return false;
            const auto Action = MakeShared<FJsonObject>();
            switch (Command.Type)
            {
            case ELifeTaskCommandType::Enqueue:
                if (!BoundedText(Command.RoomId, 80) || !BoundedText(Command.StationId, 80)
                    || (Command.Cooking.IsSet() && !BoundedText(Command.Cooking->RecipeKey, 80))
                    || !WriteCooking(Command.Cooking, Action)) return false;
                Action->SetStringField(TEXT("type"), TEXT("enqueue"));
                Action->SetStringField(TEXT("roomId"), Command.RoomId);
                Action->SetStringField(TEXT("stationId"), Command.StationId);
                break;
            case ELifeTaskCommandType::Pause: Action->SetStringField(TEXT("type"), TEXT("pause")); break;
            case ELifeTaskCommandType::Resume: Action->SetStringField(TEXT("type"), TEXT("resume")); break;
            case ELifeTaskCommandType::Stop: Action->SetStringField(TEXT("type"), TEXT("stop")); break;
            case ELifeTaskCommandType::Remove:
            case ELifeTaskCommandType::Retry:
            case ELifeTaskCommandType::Move:
                if (!BoundedText(Command.TaskId, 100)) return false;
                Action->SetStringField(TEXT("taskId"), Command.TaskId);
                if (Command.Type == ELifeTaskCommandType::Move)
                {
                    if (Command.Direction != ELifeTaskMoveDirection::Up && Command.Direction != ELifeTaskMoveDirection::Down) return false;
                    Action->SetStringField(TEXT("type"), TEXT("move"));
                    Action->SetStringField(TEXT("direction"), Command.Direction == ELifeTaskMoveDirection::Up ? TEXT("up") : TEXT("down"));
                }
                else Action->SetStringField(TEXT("type"), Command.Type == ELifeTaskCommandType::Remove ? TEXT("remove") : TEXT("retry"));
                break;
            case ELifeTaskCommandType::Advance:
                if (!BoundedText(Command.RoomId, 80)) return false;
                Action->SetStringField(TEXT("type"), TEXT("advance"));
                Action->SetStringField(TEXT("roomId"), Command.RoomId);
                break;
            default: return false;
            }
            const auto O = RequestArguments(Scope, Revision, Id);
            O->SetObjectField(TEXT("command"), Action);
            return FJsonSerializer::Serialize(O, TJsonWriterFactory<>::Create(&Json));
        }
        FString PublicFailure(const FLifeCommandFailure& Failure, const TCHAR* Fallback)
        {
            if (!Failure.PublicMessage.IsSet() || Failure.PublicMessage->Len() > 500) return Fallback;
            for (const TCHAR C : Failure.PublicMessage.GetValue())
                if ((C < 32 && C != TEXT('\n') && C != TEXT('\t')) || C == 127) return Fallback;
            return Failure.PublicMessage.GetValue();
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
        if (!StateJsonPrivate::Same(Selection.AccountId, AccountId)) ClearCommands();
        RetireSubscription();
        Selection = Next;
        View = FLifeReadView{};
        View.bBusy = bActInFlight;
        View.Retryable = PendingCare.IsValid();
        Changed.Broadcast();
        return true;
    }
    bool FLifeReadState::BeginSubscription(double ConnectedAtMs, FLifeReadReceipt& Out)
    {
        Out = FLifeReadReceipt{};
        RetireSubscription();
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
        RetireSubscription();
        // Canonical last-observer removal clears PUBLIC busy, not actInFlight.
        View.bBusy = false;
    }
    void FLifeReadState::RetireSubscription()
    {
        Generation = FGuid::NewGuid();
        bSubscribed = false;
        // Reports/tasks have no retained retry slot; old-generation callbacks cannot publish.
        Auxiliary.Reset();
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
        if (!PendingCare.IsValid()) View.Error.Reset();
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
    bool FLifeReadState::PrepareCare(const FLifeCareIntent& Intent, const FString& RequestId, FLifeCommandReceipt& Out)
    {
        Out = FLifeCommandReceipt{};
        if (bDisposed || View.bBusy || bActInFlight || PendingCare.IsValid() || !View.Life.IsValid()) return false;
        auto Request = MakeShared<FCareRequest>();
        Request->Scope = Selection;
        Request->Intent = Intent;
        Request->ExpectedRevision = View.Life->Revision;
        Request->RequestId = RequestId;
        if (!LifePrivate::EncodeCare(Request->Scope, Request->ExpectedRevision, Intent, RequestId, Request->ArgumentsJson)) return false;
        PendingCare = Request;
        bCareUncertain = false;
        return PrepareCareAttempt(false, Out);
    }
    bool FLifeReadState::PrepareCareRetry(FLifeCommandReceipt& Out)
    {
        Out = FLifeCommandReceipt{};
        return PendingCare.IsValid() && bCareUncertain && PrepareCareAttempt(true, Out);
    }
    bool FLifeReadState::PrepareCareAttempt(bool bReplay, FLifeCommandReceipt& Out)
    {
        if (bDisposed || bActInFlight || !PendingCare.IsValid()
            || !StateJsonPrivate::Same(Selection.AccountId, PendingCare->Scope.AccountId)) return false;
        FLifeCommandReceipt Receipt;
        Receipt.Id = FGuid::NewGuid();
        Receipt.Generation = Generation;
        Receipt.Scope = PendingCare->Scope;
        Receipt.Kind = ELifeCommandKind::Care;
        Receipt.RequestId = PendingCare->RequestId;
        Receipt.ArgumentsJson = PendingCare->ArgumentsJson;
        CareAttempt = Receipt;
        bActInFlight = true;
        bCareDispatched = false;
        bCareReplay = bReplay;
        View.bBusy = true;
        View.Error.Reset();
        View.Retryable = false;
        Changed.Broadcast();
        if (!CurrentCare(Receipt)) return false;
        if (!bCareReplay && !CurrentPresentation(Receipt))
        {
            AbandonPreparedCommand(Receipt);
            return false;
        }
        Out = MoveTemp(Receipt);
        return true;
    }
    bool FLifeReadState::CurrentCare(const FLifeCommandReceipt& Receipt) const
    {
        return !bDisposed && bActInFlight && PendingCare.IsValid() && Receipt.Id.IsValid()
            && Receipt.Kind == ELifeCommandKind::Care && CareAttempt.Id == Receipt.Id
            && StateJsonPrivate::Same(Selection.AccountId, PendingCare->Scope.AccountId);
    }
    bool FLifeReadState::CurrentPresentation(const FLifeCommandReceipt& Receipt) const
    {
        return !bDisposed && Receipt.Generation.IsValid() && Receipt.Generation == Generation
            && LifePrivate::SameScope(Selection, Receipt.Scope);
    }
    bool FLifeReadState::CaptureAuxiliary(const FString& ArgumentsJson, const FString& RequestId,
        ELifeCommandKind Kind, ELifeReportStatus Status, FLifeCommandReceipt& Out)
    {
        if (bDisposed || Auxiliary.Num() >= MaxAuxiliaryReceipts) return false;
        FAuxiliaryAttempt Entry;
        Entry.Receipt.Id = FGuid::NewGuid();
        Entry.Receipt.Generation = Generation;
        Entry.Receipt.Scope = Selection;
        Entry.Receipt.Kind = Kind;
        Entry.Receipt.ArgumentsJson = ArgumentsJson;
        Entry.Receipt.RequestId = RequestId;
        Entry.ReportStatus = Status;
        Out = Entry.Receipt;
        Auxiliary.Add(Out.Id, MoveTemp(Entry));
        return true;
    }
    const FLifeReadState::FAuxiliaryAttempt* FLifeReadState::FindAuxiliary(const FLifeCommandReceipt& Receipt) const
    {
        if (bDisposed || !Receipt.Id.IsValid()) return nullptr;
        const auto* Entry = Auxiliary.Find(Receipt.Id);
        return Entry && Entry->Receipt.Kind == Receipt.Kind ? Entry : nullptr;
    }
    bool FLifeReadState::PrepareReport(const FLifeActivityReport& Report, FLifeCommandReceipt& Out)
    {
        Out = FLifeCommandReceipt{};
        if (bDisposed || !View.Life.IsValid() || !View.Life->Activity.IsSet()
            || !StateJsonPrivate::Same(View.Life->Activity->Id, Report.ActivityId)) return false;
        FString Json;
        if (!LifePrivate::EncodeReport(Selection, Report, Json)) return false;
        // Reports do not set busy and may coexist with care and task requests.
        return CaptureAuxiliary(Json, FString(), ELifeCommandKind::Report, Report.Status, Out);
    }
    bool FLifeReadState::PrepareTask(const FLifeTaskCommand& Command, const FString& RequestId, FLifeCommandReceipt& Out)
    {
        Out = FLifeCommandReceipt{};
        if (bDisposed || View.bBusy || PendingCare.IsValid() || !View.Life.IsValid()) return false;
        FString Json;
        if (!LifePrivate::EncodeTask(Selection, View.Life->Revision, Command, RequestId, Json)) return false;
        FLifeCommandReceipt Receipt;
        if (!CaptureAuxiliary(Json, RequestId, ELifeCommandKind::Task, ELifeReportStatus::Arrived, Receipt)) return false;
        View.bBusy = true;
        View.Error.Reset();
        Changed.Broadcast();
        // A changed generation retires auxiliary receipts. No returning call may revive one.
        if (!FindAuxiliary(Receipt) || !CurrentPresentation(Receipt)) return false;
        Out = MoveTemp(Receipt);
        return true;
    }
    bool FLifeReadState::DispatchCommand(const FLifeCommandReceipt& Receipt)
    {
        if (Receipt.Kind == ELifeCommandKind::Care)
        {
            if (!CurrentCare(Receipt) || bCareDispatched) return false;
            if (!bCareReplay && !CurrentPresentation(CareAttempt))
            {
                AbandonPreparedCommand(Receipt);
                return false;
            }
            bCareDispatched = true;
            bCareUncertain = true;
            return true;
        }
        const auto* Entry = FindAuxiliary(Receipt);
        if (!Entry || Entry->bDispatched || !CurrentPresentation(Entry->Receipt)) return false;
        Auxiliary.FindChecked(Receipt.Id).bDispatched = true;
        return true;
    }
    bool FLifeReadState::AbandonPreparedCommand(const FLifeCommandReceipt& Receipt)
    {
        if (Receipt.Kind == ELifeCommandKind::Care)
        {
            if (!CurrentCare(Receipt) || bCareDispatched) return false;
            if (!bCareUncertain) PendingCare.Reset();
            FinishCare();
            return true;
        }
        const auto* Entry = FindAuxiliary(Receipt);
        if (!Entry || Entry->bDispatched) return false;
        const bool bFinishTask = Entry->Receipt.Kind == ELifeCommandKind::Task && CurrentPresentation(Entry->Receipt);
        Auxiliary.Remove(Receipt.Id);
        if (bFinishTask) { View.bBusy = false; Changed.Broadcast(); }
        return true;
    }
    bool FLifeReadState::PublishCommandLife(const FLifeCommandReceipt& Receipt, FLife&& Life)
    {
        if (!CurrentPresentation(Receipt) || Life.Revision < (View.Life.IsValid() ? View.Life->Revision : 0)) return false;
        View.Life = MakeShared<FLife>(MoveTemp(Life));
        View.Origin = ELifeReadOrigin::SavedValue;
        return true;
    }
    void FLifeReadState::FinishCare()
    {
        CareAttempt = FLifeCommandReceipt{};
        bActInFlight = bCareDispatched = bCareReplay = false;
        if (!PendingCare.IsValid()) bCareUncertain = false;
        View.bBusy = false;
        View.Retryable = PendingCare.IsValid();
        Changed.Broadcast();
        // No writes after notification: a listener may prepare a replacement request.
    }
    ELifeCommandResult FLifeReadState::CompleteCommand(const FLifeCommandReceipt& Receipt, const FString& Json)
    {
        const bool bCare = Receipt.Kind == ELifeCommandKind::Care;
        const auto* Entry = bCare ? nullptr : FindAuxiliary(Receipt);
        if (bCare ? !CurrentCare(Receipt) || !bCareDispatched : !Entry || !Entry->bDispatched)
            return ELifeCommandResult::StaleReceipt;
        const FLifeCommandReceipt Captured = bCare ? CareAttempt : Entry->Receipt;
        const ELifeReportStatus Status = Entry ? Entry->ReportStatus : ELifeReportStatus::Arrived;
        TOptional<FLife> Decoded;
        FString Error;
        const bool bReport = Captured.Kind == ELifeCommandKind::Report;
        if (!DecodeLifeResponse(Json, Decoded, Error) || (!Decoded.IsSet() && !bReport)
            || (Decoded.IsSet() && !StateJsonPrivate::Same(Decoded->CharacterId, Captured.Scope.CharacterId)))
        {
            FLifeCommandFailure Failure;
            Failure.PublicMessage = bCare ? LifePrivate::InvalidCare : bReport ? LifePrivate::InvalidRead : LifePrivate::InvalidTask;
            FailCommand(Receipt, Failure);
            return ELifeCommandResult::InvalidResponse;
        }
        if (bCare) PendingCare.Reset();
        else Auxiliary.Remove(Captured.Id);
        // Reports alone accept null. It neither publishes a fresh default nor clears an error.
        if (!Decoded.IsSet()) return ELifeCommandResult::NullResult;
        const bool bPublished = PublishCommandLife(Captured, MoveTemp(Decoded.GetValue()));
        if (bCare) FinishCare();
        else if (bReport)
        {
            if (bPublished)
            {
                if (Status == ELifeReportStatus::Blocked) View.Error = LifePrivate::BlockedActivity;
                else View.Error.Reset();
                Changed.Broadcast();
            }
        }
        else if (CurrentPresentation(Captured))
        {
            // Match task's generation-based finally, even if another task became possible
            // after a read failure cleared public busy during this request.
            View.bBusy = false;
            Changed.Broadcast();
        }
        return bPublished ? ELifeCommandResult::Applied : ELifeCommandResult::ResolvedWithoutPublication;
    }
    bool FLifeReadState::FailCommand(const FLifeCommandReceipt& Receipt, const FLifeCommandFailure& Failure)
    {
        if (Failure.Category != ELifeCommandFailureCategory::Definite && Failure.Category != ELifeCommandFailureCategory::Ambiguous) return false;
        if (Receipt.Kind == ELifeCommandKind::Care)
        {
            if (!CurrentCare(Receipt) || !bCareDispatched) return false;
            const bool bVisible = CurrentPresentation(CareAttempt);
            // A definite failed replay drops pending care, exactly like submitAct.
            if (Failure.Category == ELifeCommandFailureCategory::Definite) PendingCare.Reset();
            if (bVisible) View.Error = LifePrivate::PublicFailure(Failure, TEXT("Couldn't save this activity. Try again."));
            FinishCare();
            return true;
        }
        const auto* Entry = FindAuxiliary(Receipt);
        if (!Entry || !Entry->bDispatched) return false;
        const bool bVisible = CurrentPresentation(Entry->Receipt);
        const bool bTask = Entry->Receipt.Kind == ELifeCommandKind::Task;
        Auxiliary.Remove(Receipt.Id);
        if (bVisible)
        {
            View.Error = LifePrivate::PublicFailure(Failure, bTask ? TEXT("Couldn't update the tasks. Try again.")
                : TEXT("Couldn't update this activity. Try again."));
            if (bTask) View.bBusy = false;
            Changed.Broadcast();
        }
        return true;
    }
    void FLifeReadState::ClearCommands()
    {
        PendingCare.Reset();
        CareAttempt = FLifeCommandReceipt{};
        bActInFlight = bCareDispatched = bCareUncertain = bCareReplay = false;
        Auxiliary.Reset();
    }
    void FLifeReadState::Clear()
    {
        if (bDisposed) return;
        RetireSubscription();
        ClearCommands();
        Selection = FLifeReadScope{};
        View = FLifeReadView{};
        Changed.Broadcast();
    }
    void FLifeReadState::Dispose()
    {
        if (bDisposed) return;
        bDisposed = true;
        RetireSubscription();
        ClearCommands();
        Selection = FLifeReadScope{};
        View = FLifeReadView{};
        Changed.Clear();
    }
}
