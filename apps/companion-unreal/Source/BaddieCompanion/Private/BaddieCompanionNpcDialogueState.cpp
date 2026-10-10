#include "BaddieCompanionNpcDialogueState.h"
#include "BaddieCompanionStateJson.h"

#include <cmath>

namespace BaddieCompanion
{
    namespace NpcDialogueStatePrivate
    {
        using StateJsonPrivate::FValue;
        using StateJsonPrivate::FObject;
        using StateJsonPrivate::Field;
        using StateJsonPrivate::Same;
        using StateJsonPrivate::String;
        using StateJsonPrivate::Number;
        using StateJsonPrivate::Integer;
        using StateJsonPrivate::Only;
        using StateJsonPrivate::Nullable;
        using StateJsonPrivate::EnumValue;
        constexpr StateJsonPrivate::FParseLimits ReplyLimits{16384, 4, 64};
        constexpr int32 MessageCharacters = 400, ReplyCharacters = 1024;

        int32 Utf16Length(const FString& Value)
        {
            int32 Length = 0;
            for (const TCHAR C : Value)
            {
                const int32 Units = static_cast<uint32>(C) > 0xffff ? 2 : 1;
                if (Length > MAX_int32 - Units) return MAX_int32;
                Length += Units;
            }
            return Length;
        }
        // ECMAScript trim whitespace, for blank-message/name checks only. Original text stays intact.
        bool HasText(const FString& Value)
        {
            for (const TCHAR C : Value)
            {
                const uint32 N = static_cast<uint32>(C);
                if (!((N >= 0x9 && N <= 0xd) || N == 0x20 || N == 0xa0 || N == 0x1680
                    || (N >= 0x2000 && N <= 0x200a) || N == 0x2028 || N == 0x2029 || N == 0x202f
                    || N == 0x205f || N == 0x3000 || N == 0xfeff)) return true;
            }
            return false;
        }
        bool Bounded(const FString& Value, int32 Maximum)
        {
            return !Value.IsEmpty() && Value.Len() <= Maximum && Utf16Length(Value) <= Maximum;
        }
        bool ReadString(const FObject& O, const TCHAR* Name, FString& Out, int32 Maximum)
        {
            return String(Field(O, Name), Out) && Bounded(Out, Maximum);
        }
        bool RequestId(const FString& Value)
        {
            if (!Bounded(Value, 100)) return false;
            for (const TCHAR C : Value)
                if (!((C >= TEXT('a') && C <= TEXT('z')) || (C >= TEXT('A') && C <= TEXT('Z'))
                    || (C >= TEXT('0') && C <= TEXT('9')) || C == TEXT('.') || C == TEXT('_')
                    || C == TEXT(':') || C == TEXT('-'))) return false;
            return true;
        }
        const TCHAR* ServiceId(ENpcDialogueKey Key)
        {
            switch (Key)
            {
            case ENpcDialogueKey::MamaVendor: return TEXT("mama-vendor");
            case ENpcDialogueKey::DeliveryContact: return TEXT("delivery-contact");
            default: return nullptr;
            }
        }
        bool ReplyAllowed(ENpcDialogueKey Npc, ENpcDialogueReplyKey Reply)
        {
            if (!ServiceId(Npc)) return false;
            switch (Reply)
            {
            case ENpcDialogueReplyKey::Menu:
            case ENpcDialogueReplyKey::PriceBoundary: return Npc == ENpcDialogueKey::MamaVendor;
            case ENpcDialogueReplyKey::Compliment: return Npc == ENpcDialogueKey::DeliveryContact;
            case ENpcDialogueReplyKey::Greet:
            case ENpcDialogueReplyKey::WelcomeBack:
            case ENpcDialogueReplyKey::Work:
            case ENpcDialogueReplyKey::Directions:
            case ENpcDialogueReplyKey::DeEscalate:
            case ENpcDialogueReplyKey::Clarify: return true;
            default: return false;
            }
        }
        bool SameOptional(const TOptional<FString>& A, const TOptional<FString>& B)
        {
            return A.IsSet() == B.IsSet() && (!A.IsSet() || Same(A.GetValue(), B.GetValue()));
        }
        bool SameScope(const FNpcDialogueScope& A, const FNpcDialogueScope& B)
        {
            return Same(A.UserId, B.UserId) && Same(A.CharacterId, B.CharacterId)
                && Same(A.GameRoomId, B.GameRoomId) && Same(A.InstanceId, B.InstanceId)
                && A.Revision == B.Revision && A.MoveSequence == B.MoveSequence && A.RenderAttempt == B.RenderAttempt
                && Same(A.LifeId, B.LifeId) && SameOptional(A.ResidentWorldKey, B.ResidentWorldKey)
                && SameOptional(A.ResidentEpochKey, B.ResidentEpochKey);
        }
        bool Rendered(const FNpcDialogueObservation& O)
        {
            return O.Player.IsValid() && O.RenderedInstanceId.IsSet()
                && Same(O.RenderedInstanceId.GetValue(), O.Player->InstanceId);
        }
    }

    const TCHAR* NpcDialogueTopicId(ENpcDialogueTopic Topic)
    {
        switch (Topic)
        {
        case ENpcDialogueTopic::Greeting: return TEXT("greeting");
        case ENpcDialogueTopic::Menu: return TEXT("menu");
        case ENpcDialogueTopic::Work: return TEXT("work");
        case ENpcDialogueTopic::Directions: return TEXT("directions");
        case ENpcDialogueTopic::Prices: return TEXT("prices");
        case ENpcDialogueTopic::Compliment: return TEXT("compliment");
        default: return nullptr;
        }
    }
    TArray<FNpcDialogueTopic> NpcDialogueTopics(ENpcDialogueKey Key)
    {
        if (!NpcDialogueStatePrivate::ServiceId(Key)) return {};
        TArray<FNpcDialogueTopic> Topics;
        Topics.Add({ENpcDialogueTopic::Greeting, TEXT("Hello")});
        if (Key == ENpcDialogueKey::MamaVendor) Topics.Add({ENpcDialogueTopic::Menu, TEXT("Menu")});
        Topics.Add({ENpcDialogueTopic::Work, TEXT("Delivery work")});
        Topics.Add({ENpcDialogueTopic::Directions, TEXT("Directions")});
        if (Key == ENpcDialogueKey::DeliveryContact) Topics.Add({ENpcDialogueTopic::Compliment, TEXT("Compliment")});
        if (Key == ENpcDialogueKey::MamaVendor) Topics.Add({ENpcDialogueTopic::Prices, TEXT("Prices")});
        return Topics;
    }
    bool DecodeNpcDialogueReply(const FString& Json, TSharedPtr<const FNpcDialogueReply>& Out, FString& OutError)
    {
        using namespace NpcDialogueStatePrivate;
        // Json may refer to text owned by the previous output or to OutError.
        const FString SourceJson = Json.Len() <= ReplyLimits.Characters ? Json : FString();
        Out.Reset(); OutError = TEXT("NPC reply is unsupported, malformed or exceeds the decoder bounds.");
        FObject O;
        if (!StateJsonPrivate::Parse(SourceJson, O, ReplyLimits)
            || !Only(O, {TEXT("requestId"), TEXT("instanceId"), TEXT("revision"), TEXT("lifeId"), TEXT("npcKey"),
                TEXT("reply"), TEXT("text"), TEXT("actorIdentity"), TEXT("speakerName"), TEXT("source"), TEXT("confidence")})) return false;
        auto Next = MakeShared<FNpcDialogueReply>();
        if (!ReadString(O, TEXT("requestId"), Next->RequestId, 100) || !RequestId(Next->RequestId)
            || !ReadString(O, TEXT("instanceId"), Next->InstanceId, 160)
            || !Integer(Field(O, TEXT("revision")), Next->Revision, 1)
            || !ReadString(O, TEXT("lifeId"), Next->LifeId, 100)
            || !ReadString(O, TEXT("actorIdentity"), Next->ActorIdentity, 320)
            || !ReadString(O, TEXT("speakerName"), Next->SpeakerName, 160) || !HasText(Next->SpeakerName)
            || !ReadString(O, TEXT("text"), Next->Text, ReplyCharacters) || !HasText(Next->Text)
            || !EnumValue(Field(O, TEXT("npcKey")), Next->NpcKey,
                {{TEXT("mama-vendor"), ENpcDialogueKey::MamaVendor}, {TEXT("delivery-contact"), ENpcDialogueKey::DeliveryContact}})
            || !EnumValue(Field(O, TEXT("reply")), Next->Reply,
                {{TEXT("greet"), ENpcDialogueReplyKey::Greet}, {TEXT("welcome-back"), ENpcDialogueReplyKey::WelcomeBack},
                 {TEXT("menu"), ENpcDialogueReplyKey::Menu}, {TEXT("work"), ENpcDialogueReplyKey::Work},
                 {TEXT("directions"), ENpcDialogueReplyKey::Directions}, {TEXT("price-boundary"), ENpcDialogueReplyKey::PriceBoundary},
                 {TEXT("de-escalate"), ENpcDialogueReplyKey::DeEscalate}, {TEXT("clarify"), ENpcDialogueReplyKey::Clarify},
                 {TEXT("compliment"), ENpcDialogueReplyKey::Compliment}})
            || !EnumValue(Field(O, TEXT("source")), Next->Source,
                {{TEXT("laya"), ENpcDialogueReplySource::Laya}, {TEXT("scripted"), ENpcDialogueReplySource::Scripted}})
            || !Nullable(Field(O, TEXT("confidence")), Next->Confidence,
                [](const FValue& V, double& N) { return Number(V, N); })
            || !ReplyAllowed(Next->NpcKey, Next->Reply)) return false;
        if (Next->Source == ENpcDialogueReplySource::Scripted)
        {
            if (Next->Confidence.IsSet()) return false;
        }
        else if (!Next->Confidence.IsSet() || Next->Confidence.GetValue() < .75 || Next->Confidence.GetValue() > 1) return false;
        Out = Next; OutError.Reset(); return true;
    }

    const TCHAR* NpcDialogueErrorText(ENpcDialogueError Error)
    {
        switch (Error)
        {
        case ENpcDialogueError::ConversationChanged: return TEXT("The conversation changed. Ask again.");
        case ENpcDialogueError::Loading: return TEXT("Wait for the place to finish loading.");
        case ENpcDialogueError::InvalidInput: return TEXT("Choose a topic or ask a question of up to 400 characters.");
        case ENpcDialogueError::InvalidContext: return TEXT("Choose a current NPC conversation.");
        case ENpcDialogueError::AccountUnavailable: return TEXT("Games are unavailable for this account.");
        case ENpcDialogueError::GameEnded: return TEXT("This game has ended.");
        case ENpcDialogueError::DifferentVisitOrLife: return TEXT("This conversation belongs to another visit or life.");
        case ENpcDialogueError::ActivityBusy: return TEXT("Finish the current activity before talking.");
        case ENpcDialogueError::FootballBusy: return TEXT("Leave the match before talking.");
        case ENpcDialogueError::Walking: return TEXT("Finish walking before talking.");
        case ENpcDialogueError::OutOfReach: return TEXT("Walk to this NPC first.");
        case ENpcDialogueError::ActorChanged: return TEXT("This NPC has changed. Reopen the conversation.");
        case ENpcDialogueError::NpcUnavailable: return TEXT("This NPC is away or busy. Try again when they're available.");
        case ENpcDialogueError::NpcIdentityInvalid: return TEXT("This NPC's identity needs review.");
        case ENpcDialogueError::ChangedRequest: return TEXT("This conversation request changed.");
        case ENpcDialogueError::PreviousReplyPending: return TEXT("The previous reply wasn't received. Send a new request after a moment.");
        case ENpcDialogueError::NpcCooldown: return TEXT("Wait a moment before asking this NPC again.");
        case ENpcDialogueError::AccountCooldown: return TEXT("Wait a moment before asking another NPC.");
        case ENpcDialogueError::MovedDuringRequest: return TEXT("The conversation changed. Ask again when you're beside the NPC.");
        case ENpcDialogueError::ReplyExpired: return TEXT("The reply took too long. Try again.");
        case ENpcDialogueError::SignInRequired: return TEXT("Please sign in again to continue.");
        case ENpcDialogueError::RateLimited: return TEXT("Please wait a moment before trying again.");
        case ENpcDialogueError::ServiceUnavailable: return TEXT("Service is temporarily unavailable. Please try again.");
        case ENpcDialogueError::NetworkUnavailable: return TEXT("Network unavailable. Check your connection and try again.");
        case ENpcDialogueError::TemporarilyUnavailable: return TEXT("This is temporarily unavailable. Please try again shortly.");
        default: return TEXT("The reply couldn't load. Try again.");
        }
    }

    FNpcDialogueOwnerReceipt FNpcDialogueState::BeginOwner(const FString& UserId)
    {
        if (bDisposed) return {};
        const FString NextOwner = NpcDialogueStatePrivate::Bounded(UserId, 240) ? UserId : FString();
        Close(); OwnerId.Reset(); OwnerGeneration.Invalidate(); ActiveDispatch.Invalidate(); bObserved = false;
        if (NextOwner.IsEmpty()) return {};
        OwnerId = NextOwner; OwnerGeneration = FGuid::NewGuid();
        FNpcDialogueOwnerReceipt Receipt; Receipt.Generation = OwnerGeneration; return Receipt;
    }
    bool FNpcDialogueState::CurrentOwner(const FNpcDialogueOwnerReceipt& Owner) const
    {
        return !bDisposed && Owner.Generation.IsValid() && Owner.Generation == OwnerGeneration;
    }
    void FNpcDialogueState::SetObserved(const FNpcDialogueOwnerReceipt& Owner, bool bNextObserved)
    {
        if (!CurrentOwner(Owner)) return;
        bObserved = bNextObserved;
        if (!bObserved) Close();
    }
    bool FNpcDialogueState::ReadScope(const FNpcDialogueObservation& O, FNpcDialogueScope& Out) const
    {
        using namespace NpcDialogueStatePrivate;
        if (bDisposed || !bObserved || OwnerId.IsEmpty() || !O.bWorldReady || O.bWorldHasError
            || !O.Player.IsValid() || !O.Instance.IsValid() || !O.Recovery.bReady || O.Recovery.Error.IsSet()
            || O.Recovery.HealthPresence != EWorldFieldPresence::Value || !O.Recovery.Health.IsValid()
            || !StateJsonPrivate::ValidTime(O.SampleAtServerMs)) return false;
        const auto& Player = *O.Player; const auto& Health = *O.Recovery.Health;
        if (!Same(OwnerId, Player.UserId) || !Same(O.Recovery.OwnerId, OwnerId)
            || Player.Status != EWorldPlayerStatus::Present || !std::isfinite(Player.Health) || Player.Health <= 0
            || Health.Status != ECompanionHealthStatus::Active || !std::isfinite(Health.Health) || Health.Health <= 0
            || O.Instance->Relation != EWorldInstanceRelation::CurrentVisit
            || !Same(O.Instance->Id, Player.InstanceId) || !Same(O.Instance->PlaceId, Player.PlaceId)
            || !Bounded(Player.CharacterId, 240) || !Bounded(Player.GameRoomId, 240)
            || !Bounded(Player.InstanceId, 160) || !Bounded(Health.LifeId, 100)
            || Player.Revision < 1 || Player.Revision > StateJsonPrivate::MaxInteger
            || Player.MoveSequence < 0 || Player.MoveSequence > 1000000000
            || O.RenderAttempt < 0 || O.RenderAttempt > StateJsonPrivate::MaxInteger
            || O.ResidentWorldKey.IsSet() != O.ResidentEpochKey.IsSet()
            || (O.ResidentWorldKey.IsSet() && (!Bounded(O.ResidentWorldKey.GetValue(), 80)
                || !Bounded(O.ResidentEpochKey.GetValue(), 160)))) return false;
        if (Player.Movement.IsSet())
        {
            FMovementSample Sample;
            if (!SampleMovement(Player.Movement.GetValue(), O.SampleAtServerMs, Sample) || Sample.bMoving) return false;
        }
        else if (!std::isfinite(Player.Position.X) || !std::isfinite(Player.Position.Z)) return false;
        Out.UserId = Player.UserId; Out.CharacterId = Player.CharacterId; Out.GameRoomId = Player.GameRoomId;
        Out.InstanceId = Player.InstanceId; Out.Revision = Player.Revision; Out.MoveSequence = Player.MoveSequence;
        Out.RenderAttempt = O.RenderAttempt; Out.LifeId = Health.LifeId;
        Out.ResidentWorldKey = O.ResidentWorldKey; Out.ResidentEpochKey = O.ResidentEpochKey;
        return true;
    }
    bool FNpcDialogueState::ReadTarget(const FNpcDialogueObservation& O, const FNpcDialogueScope& Scope,
        const FString& SelectedService, FNpcDialogueTarget& Out) const
    {
        using namespace NpcDialogueStatePrivate;
        const auto& Evidence = O.Target;
        if ((Evidence.Resolution != ENpcDialogueTargetResolution::ResolvedAuthored
                && Evidence.Resolution != ENpcDialogueTargetResolution::ResolvedResident)
            || !Evidence.Target.IsSet() || !SameScope(Evidence.Scope, Scope)
            || Evidence.ResolvedAtServerMs != O.SampleAtServerMs) return false;
        const auto& Target = Evidence.Target.GetValue();
        const TCHAR* ExpectedService = ServiceId(Target.Key);
        if (!ExpectedService || !Same(Target.ServiceId, ExpectedService) || !Same(Target.ServiceId, SelectedService)
            || !Same(Target.NpcId, Target.Key == ENpcDialogueKey::MamaVendor ? TEXT("mama-vendor-npc") : TEXT("delivery-contact-npc"))
            || !Same(Target.PlaceId, TEXT("mama-put")) || !Same(Target.PlaceId, O.Player->PlaceId)
            || !Same(Target.InstanceId, Scope.InstanceId) || Target.Revision != Scope.Revision
            || !Bounded(Target.ActorIdentity, 320) || !Bounded(Target.Name, 160) || !HasText(Target.Name)) return false;
        if (Evidence.Resolution == ENpcDialogueTargetResolution::ResolvedAuthored)
        {
            if (Evidence.ResidentId.IsSet()
                || !Same(Target.ActorIdentity, TEXT("authored:") + Scope.GameRoomId + TEXT(":") + Target.NpcId)
                || !Same(Target.Name, Target.Key == ENpcDialogueKey::MamaVendor ? TEXT("Food vendor") : TEXT("Delivery contact"))) return false;
        }
        else if (!Scope.ResidentWorldKey.IsSet() || !Evidence.ResidentId.IsSet()
            || !Bounded(Evidence.ResidentId.GetValue(), 160)
            || !Same(Target.ActorIdentity, Scope.ResidentWorldKey.GetValue() + TEXT(":") + Evidence.ResidentId.GetValue())) return false;
        Out = Target; return true;
    }
    bool FNpcDialogueState::SelectionMatches(const FNpcDialogueObservation& O) const
    {
        if (!SelectedScope.IsSet() || !View.Target.IsSet()) return false;
        FNpcDialogueScope Scope; FNpcDialogueTarget Target;
        return ReadScope(O, Scope) && NpcDialogueStatePrivate::SameScope(Scope, SelectedScope.GetValue())
            && ReadTarget(O, Scope, View.Target.GetValue().ServiceId, Target)
            && NpcDialogueStatePrivate::Same(Target.ActorIdentity, View.Target.GetValue().ActorIdentity);
    }
    void FNpcDialogueState::Invalidate(const FNpcDialogueOwnerReceipt& Owner, const FNpcDialogueObservation& O)
    {
        if (CurrentOwner(Owner) && SelectedScope.IsSet() && !SelectionMatches(O)) Close();
    }
    ENpcDialogueOpenResult FNpcDialogueState::Open(const FNpcDialogueOwnerReceipt& Owner,
        const FNpcDialogueObservation& O, const FString& SelectedService)
    {
        if (!CurrentOwner(Owner) || ActiveDispatch.IsValid() || O.bWorldBusy || O.Recovery.bBusy
            || !NpcDialogueStatePrivate::Rendered(O)) return ENpcDialogueOpenResult::Ineligible;
        FNpcDialogueScope Scope; FNpcDialogueTarget Target;
        if (!ReadScope(O, Scope)) return ENpcDialogueOpenResult::Ineligible;
        if (O.Target.Resolution == ENpcDialogueTargetResolution::Unimplemented) return ENpcDialogueOpenResult::UnimplementedTarget;
        if (!ReadTarget(O, Scope, SelectedService, Target)) return ENpcDialogueOpenResult::Ineligible;
        ConversationGeneration = FGuid::NewGuid(); SelectedScope = MoveTemp(Scope);
        View = {}; View.Target = MoveTemp(Target); return ENpcDialogueOpenResult::Opened;
    }
    void FNpcDialogueState::Close()
    {
        if (!SelectedScope.IsSet() && !View.Target.IsSet() && !View.Error.IsSet() && !View.Reply.IsValid()) return;
        ConversationGeneration = FGuid::NewGuid(); SelectedScope.Reset(); View = {};
    }
    ENpcDialogueSendResult FNpcDialogueState::Capture(const FNpcDialogueOwnerReceipt& Owner,
        const FNpcDialogueObservation& O, const FString& Message, TOptional<ENpcDialogueTopic> Topic, FNpcDialogueSendIntent& Out)
    {
        using namespace NpcDialogueStatePrivate;
        const bool bMessageFits = Message.Len() <= MessageCharacters && Utf16Length(Message) <= MessageCharacters;
        const FString OriginalMessage = bMessageFits ? Message : FString();
        Out = {};
        if (!CurrentOwner(Owner)) return ENpcDialogueSendResult::Ignored;
        Invalidate(Owner, O);
        if (!View.Target.IsSet() || !SelectedScope.IsSet())
            return O.Target.Resolution == ENpcDialogueTargetResolution::Unimplemented
                ? ENpcDialogueSendResult::UnimplementedTarget : ENpcDialogueSendResult::Ignored;
        if (ActiveDispatch.IsValid()) return ENpcDialogueSendResult::Ignored;
        if (O.bWorldBusy || O.Recovery.bBusy || !Rendered(O))
        {
            View.Error = ENpcDialogueError::Loading; return ENpcDialogueSendResult::Loading;
        }
        bool bValidInput = false;
        if (Topic.IsSet())
        {
            for (const auto& Choice : NpcDialogueTopics(View.Target.GetValue().Key))
                if (Choice.Id == Topic.GetValue()) { bValidInput = bMessageFits && OriginalMessage.IsEmpty(); break; }
        }
        else bValidInput = bMessageFits && HasText(OriginalMessage);
        if (!bValidInput) { View.Error = ENpcDialogueError::InvalidInput; return ENpcDialogueSendResult::InvalidInput; }
        FNpcDialogueSendIntent Intent;
        Intent.OwnerGeneration = OwnerGeneration; Intent.ConversationGeneration = ConversationGeneration;
        Intent.DispatchId = FGuid::NewGuid(); Intent.Scope = SelectedScope.GetValue(); Intent.Target = View.Target.GetValue();
        auto& Args = Intent.Arguments;
        Args.GameRoomId = Intent.Scope.GameRoomId; Args.ServiceId = Intent.Target.ServiceId;
        Args.ActorIdentity = Intent.Target.ActorIdentity; Args.InstanceId = Intent.Target.InstanceId;
        Args.Revision = Intent.Target.Revision; Args.LifeId = Intent.Scope.LifeId;
        Args.RequestId = TEXT("npc-") + Intent.DispatchId.ToString(EGuidFormats::Digits);
        Args.Message = OriginalMessage; Args.Topic = Topic;
        ActiveDispatch = Intent.DispatchId; View.bBusy = true; View.Error.Reset(); View.Reply.Reset();
        Out = MoveTemp(Intent); return ENpcDialogueSendResult::IntentCaptured;
    }
    ENpcDialogueSendResult FNpcDialogueState::SendMessage(const FNpcDialogueOwnerReceipt& Owner,
        const FNpcDialogueObservation& O, const FString& Message, FNpcDialogueSendIntent& Out)
    {
        return Capture(Owner, O, Message, {}, Out);
    }
    ENpcDialogueSendResult FNpcDialogueState::SendTopic(const FNpcDialogueOwnerReceipt& Owner,
        const FNpcDialogueObservation& O, ENpcDialogueTopic Topic, FNpcDialogueSendIntent& Out)
    {
        return Capture(Owner, O, TEXT(""), TOptional<ENpcDialogueTopic>(Topic), Out);
    }
    bool FNpcDialogueState::FinishReceipt(const FNpcDialogueSendIntent& Intent, const FNpcDialogueObservation& O)
    {
        if (bDisposed || !Intent.DispatchId.IsValid() || Intent.OwnerGeneration != OwnerGeneration
            || Intent.DispatchId != ActiveDispatch) return false;
        ActiveDispatch.Invalidate();
        const bool bSelectionMatches = SelectionMatches(O);
        if (Intent.ConversationGeneration != ConversationGeneration || !SelectedScope.IsSet()
            || !NpcDialogueStatePrivate::SameScope(Intent.Scope, SelectedScope.GetValue()) || !bSelectionMatches)
        {
            if (SelectedScope.IsSet() && !bSelectionMatches) Close();
            return false;
        }
        View.bBusy = false; return true;
    }
    ENpcDialogueReceiptResult FNpcDialogueState::AcceptReply(const FNpcDialogueSendIntent& Intent,
        const FNpcDialogueObservation& O, const FString& Json)
    {
        using namespace NpcDialogueStatePrivate;
        if (!FinishReceipt(Intent, O)) return ENpcDialogueReceiptResult::StaleReceipt;
        TSharedPtr<const FNpcDialogueReply> Reply; FString Error;
        if (!DecodeNpcDialogueReply(Json, Reply, Error))
        {
            View.Error = ENpcDialogueError::ConversationChanged; return ENpcDialogueReceiptResult::InvalidReply;
        }
        if (!Same(Reply->RequestId, Intent.Arguments.RequestId) || !Same(Reply->InstanceId, Intent.Target.InstanceId)
            || Reply->Revision != Intent.Target.Revision || !Same(Reply->LifeId, Intent.Scope.LifeId)
            || Reply->NpcKey != Intent.Target.Key || !Same(Reply->ActorIdentity, Intent.Target.ActorIdentity))
        {
            View.Error = ENpcDialogueError::ConversationChanged; return ENpcDialogueReceiptResult::IdentityMismatch;
        }
        View.Target.GetValue().Name = Reply->SpeakerName; View.Reply = MoveTemp(Reply);
        return ENpcDialogueReceiptResult::Accepted;
    }
    ENpcDialogueReceiptResult FNpcDialogueState::AcceptFailure(const FNpcDialogueSendIntent& Intent,
        const FNpcDialogueObservation& O, ENpcDialogueError Error)
    {
        if (!FinishReceipt(Intent, O)) return ENpcDialogueReceiptResult::StaleReceipt;
        View.Error = Error; return ENpcDialogueReceiptResult::ErrorAccepted;
    }
    void FNpcDialogueState::Dispose()
    {
        if (bDisposed) return;
        Close(); bDisposed = true; bObserved = false; OwnerId.Reset();
        OwnerGeneration.Invalidate(); ActiveDispatch.Invalidate();
    }
}
