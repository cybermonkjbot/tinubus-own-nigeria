#pragma once

#include "BaddieCompanionRecoveryState.h"

namespace BaddieCompanion
{
    enum class ENpcDialogueKey : uint8 { MamaVendor, DeliveryContact };
    enum class ENpcDialogueTopic : uint8 { Greeting, Menu, Work, Directions, Prices, Compliment };
    enum class ENpcDialogueReplyKey : uint8
    {
        Greet, WelcomeBack, Menu, Work, Directions, PriceBoundary, DeEscalate, Clarify, Compliment
    };
    enum class ENpcDialogueReplySource : uint8 { Laya, Scripted };
    struct FNpcDialogueTopic
    {
        ENpcDialogueTopic Id = ENpcDialogueTopic::Greeting;
        FString Label;
    };
    struct FNpcDialogueTarget
    {
        ENpcDialogueKey Key = ENpcDialogueKey::MamaVendor;
        FString Name, ActorIdentity, ServiceId, NpcId, PlaceId, InstanceId;
        int64 Revision = 0;
    };
    struct FNpcDialogueReply
    {
        FString RequestId, InstanceId, LifeId, Text, ActorIdentity, SpeakerName;
        int64 Revision = 0;
        ENpcDialogueKey NpcKey = ENpcDialogueKey::MamaVendor;
        ENpcDialogueReplyKey Reply = ENpcDialogueReplyKey::Clarify;
        ENpcDialogueReplySource Source = ENpcDialogueReplySource::Scripted;
        // Required wire field: null is distinct from a numeric confidence.
        TOptional<double> Confidence;
    };
    BADDIECOMPANION_API TArray<FNpcDialogueTopic> NpcDialogueTopics(ENpcDialogueKey Key);
    BADDIECOMPANION_API const TCHAR* NpcDialogueTopicId(ENpcDialogueTopic Topic);
    // Existing respond action's value, not a Convex envelope or stored legacy row.
    // Atomic, bounded, const publication. Clears Out on every failure; no reply text is generated.
    BADDIECOMPANION_API bool DecodeNpcDialogueReply(const FString& Json,
        TSharedPtr<const FNpcDialogueReply>& Out, FString& OutError);

    struct FNpcDialogueScope
    {
        FString UserId, CharacterId, GameRoomId, InstanceId, LifeId;
        int64 Revision = 0, MoveSequence = 0, RenderAttempt = 0;
        // Canonical scope uses ?? null. Missing population and explicit null collapse here.
        TOptional<FString> ResidentWorldKey, ResidentEpochKey;
    };
    enum class ENpcDialogueTargetResolution : uint8
    {
        Unimplemented, Unavailable, ResolvedAuthored, ResolvedResident
    };
    struct FNpcDialogueTargetObservation
    {
        // No native producer exists yet. ResolvedAuthored requires verified absence of
        // a resident mapping; a failed/unimplemented resident lookup must never use it.
        ENpcDialogueTargetResolution Resolution = ENpcDialogueTargetResolution::Unimplemented;
        TOptional<FNpcDialogueTarget> Target;
        FNpcDialogueScope Scope;
        double ResolvedAtServerMs = 0;
        TOptional<FString> ResidentId;
    };
    struct FNpcDialogueObservation
    {
        TSharedPtr<const FWorldPlayerView> Player;
        TSharedPtr<const FWorldCurrentInstanceIdentity> Instance;
        FRecoveryReadView Recovery;
        bool bWorldReady = false, bWorldBusy = false, bWorldHasError = false;
        TOptional<FString> RenderedInstanceId;
        int64 RenderAttempt = 0;
        TOptional<FString> ResidentWorldKey, ResidentEpochKey;
        // Fresh caller-aligned server time; the existing movement sampler is used once.
        double SampleAtServerMs = 0;
        // Caller must resolve the selected service against this exact snapshot/time,
        // using all canonical guards with requireRendered:false. Not an admission grant.
        FNpcDialogueTargetObservation Target;
    };
    // Fixed domain errors only. The future transport maps sanitized errors to these;
    // raw exceptions, request bodies, account credentials and arbitrary errors are never retained.
    enum class ENpcDialogueError : uint8
    {
        ReplyUnavailable, ConversationChanged, Loading, InvalidInput, InvalidContext,
        AccountUnavailable, GameEnded, DifferentVisitOrLife, ActivityBusy, FootballBusy,
        Walking, OutOfReach, ActorChanged, NpcUnavailable, NpcIdentityInvalid,
        ChangedRequest, PreviousReplyPending, NpcCooldown, AccountCooldown,
        MovedDuringRequest, ReplyExpired, SignInRequired, RateLimited,
        ServiceUnavailable, NetworkUnavailable, TemporarilyUnavailable
    };
    BADDIECOMPANION_API const TCHAR* NpcDialogueErrorText(ENpcDialogueError Error);
    struct FNpcDialogueView
    {
        TOptional<FNpcDialogueTarget> Target;
        bool bBusy = false;
        TOptional<ENpcDialogueError> Error;
        TSharedPtr<const FNpcDialogueReply> Reply;
    };
    class FNpcDialogueOwnerReceipt
    {
        friend class FNpcDialogueState;
        FGuid Generation;
    };
    struct FNpcDialogueSendArguments
    {
        // Exact non-auth respond arguments. No moveSequence/renderAttempt/resident epoch
        // is invented on the wire; those remain in the local receipt scope.
        FString GameRoomId, ServiceId, ActorIdentity, InstanceId, LifeId, RequestId, Message;
        int64 Revision = 0;
        TOptional<ENpcDialogueTopic> Topic;
    };
    class FNpcDialogueSendIntent
    {
    public:
        // Capture marker only, not currentness or permission to dispatch.
        bool IsValid() const { return DispatchId.IsValid(); }
        const FNpcDialogueSendArguments& GetArguments() const { return Arguments; }
    private:
        friend class FNpcDialogueState;
        FGuid OwnerGeneration, ConversationGeneration, DispatchId;
        FNpcDialogueScope Scope;
        FNpcDialogueTarget Target;
        FNpcDialogueSendArguments Arguments;
        bool bRequiresDispatchMark = false;
    };
    // Immutable non-auth action description. Copies retain one request, not another send.
    class FNpcDialogueSerializedCommand
    {
    public:
        bool IsValid() const { return Intent.IsValid() && !ArgumentsJson.IsEmpty(); }
        const TCHAR* GetFunctionName() const { return TEXT("companionNpcDialogue:respond"); }
        const FString& GetArgumentsJson() const { return ArgumentsJson; }
        const FNpcDialogueSendIntent& GetIntent() const { return Intent; }
        bool ShouldRetry() const { return false; }
    private:
        friend class FNpcDialogueState;
        FNpcDialogueSendIntent Intent;
        FString ArgumentsJson;
    };
    enum class ENpcDialogueOpenResult : uint8 { Opened, Ineligible, UnimplementedTarget };
    enum class ENpcDialogueSendResult : uint8 { IntentCaptured, Ignored, Loading, InvalidInput, UnimplementedTarget, SerializationFailed };
    enum class ENpcDialogueReceiptResult : uint8 { Accepted, ErrorAccepted, StaleReceipt, InvalidReply, IdentityMismatch };

    // Caller-driven value state, on one owning thread. No subscriptions, delegates,
    // timers, requests, actor binding, AI decisions, settlement or authority are created.
    // Fresh observations are required for each operation and each completion.
    class BADDIECOMPANION_API FNpcDialogueState
    {
    public:
        FNpcDialogueState() = default;
        FNpcDialogueState(const FNpcDialogueState&) = delete;
        FNpcDialogueState& operator=(const FNpcDialogueState&) = delete;
        const FNpcDialogueView& GetView() const { return View; }
        bool HasPendingIntent() const { return ActiveDispatch.IsValid(); }
        // Replaces the account lifetime, even for the same ID. Invalidates all old receipts.
        FNpcDialogueOwnerReceipt BeginOwner(const FString& UserId);
        void SetObserved(const FNpcDialogueOwnerReceipt& Owner, bool bObserved);
        // Mirrors world/recovery changed(). Rendering/busy loss alone does not close.
        void Invalidate(const FNpcDialogueOwnerReceipt& Owner, const FNpcDialogueObservation& Observation);
        ENpcDialogueOpenResult Open(const FNpcDialogueOwnerReceipt& Owner,
            const FNpcDialogueObservation& Observation, const FString& ServiceId);
        void Close();
        // Out clears on failure. Keep a separate copy of every unsettled intent until
        // its result/failure is supplied, including cancellation or a failed dispatch.
        ENpcDialogueSendResult SendMessage(const FNpcDialogueOwnerReceipt& Owner,
            const FNpcDialogueObservation& Observation, const FString& Message, FNpcDialogueSendIntent& Out);
        ENpcDialogueSendResult SendTopic(const FNpcDialogueOwnerReceipt& Owner,
            const FNpcDialogueObservation& Observation, ENpcDialogueTopic Topic, FNpcDialogueSendIntent& Out);
        // Same Capture order as legacy sends. Notify synchronously for IntentCaptured,
        // Loading, InvalidInput or SerializationFailed. Ignored/UnimplementedTarget may
        // still close an invalid selection; compare the view for that notification.
        ENpcDialogueSendResult PrepareSerializedMessage(const FNpcDialogueOwnerReceipt& Owner,
            const FNpcDialogueObservation& Observation, const FString& Message, FNpcDialogueSerializedCommand& Out);
        ENpcDialogueSendResult PrepareSerializedTopic(const FNpcDialogueOwnerReceipt& Owner,
            const FNpcDialogueObservation& Observation, ENpcDialogueTopic Topic, FNpcDialogueSerializedCommand& Out);
        // After the busy notification, recheck fresh canonical scope(false)/target.
        // On true, send immediately without another yield/notification. No permission.
        bool MarkCommandDispatched(const FNpcDialogueSerializedCommand& Command, const FNpcDialogueObservation& Observation);
        // Matching unsent ownership survives Close/observer loss. True releases only
        // its latch; notify if the visible busy value changed. Marked requests refuse.
        bool AbandonUnsentCommand(const FNpcDialogueSerializedCommand& Command);
        // Only marked packets complete here. Matching old-conversation completion can
        // release its latch but returns StaleReceipt without publishing into a new view.
        ENpcDialogueReceiptResult CompleteSerializedCommand(const FNpcDialogueSerializedCommand& Command,
            const FNpcDialogueObservation& Observation, const FString& Json);
        ENpcDialogueReceiptResult FailSerializedCommand(const FNpcDialogueSerializedCommand& Command,
            const FNpcDialogueObservation& Observation, ENpcDialogueError Error = ENpcDialogueError::ReplyUnavailable);
        // The matching completion releases the pending latch even after Close. A stale
        // receipt cannot clear a newer request or publish text/errors into another scope.
        ENpcDialogueReceiptResult AcceptReply(const FNpcDialogueSendIntent& Intent,
            const FNpcDialogueObservation& Observation, const FString& Json);
        ENpcDialogueReceiptResult AcceptFailure(const FNpcDialogueSendIntent& Intent,
            const FNpcDialogueObservation& Observation, ENpcDialogueError Error = ENpcDialogueError::ReplyUnavailable);
        void Dispose();
    private:
        bool CurrentOwner(const FNpcDialogueOwnerReceipt& Owner) const;
        bool ReadScope(const FNpcDialogueObservation& Observation, FNpcDialogueScope& Out) const;
        bool ReadTarget(const FNpcDialogueObservation& Observation, const FNpcDialogueScope& Scope,
            const FString& ServiceId, FNpcDialogueTarget& Out) const;
        bool SelectionMatches(const FNpcDialogueObservation& Observation) const;
        bool CurrentIntent(const FNpcDialogueSendIntent& Intent) const;
        bool CurrentSerialized(const FNpcDialogueSerializedCommand& Command) const;
        bool ConversationMatches(const FNpcDialogueSendIntent& Intent, const FNpcDialogueObservation& Observation) const;
        void RetireDispatch();
        bool FinishReceipt(const FNpcDialogueSendIntent& Intent, const FNpcDialogueObservation& Observation);
        ENpcDialogueReceiptResult AcceptCurrentReply(const FNpcDialogueSendIntent& Intent,
            const FNpcDialogueObservation& Observation, const FString& Json);
        ENpcDialogueReceiptResult AcceptCurrentFailure(const FNpcDialogueSendIntent& Intent,
            const FNpcDialogueObservation& Observation, ENpcDialogueError Error);
        ENpcDialogueSendResult PrepareSerialized(const FNpcDialogueOwnerReceipt& Owner,
            const FNpcDialogueObservation& Observation, const FString& Message,
            TOptional<ENpcDialogueTopic> Topic, FNpcDialogueSerializedCommand& Out);
        ENpcDialogueSendResult Capture(const FNpcDialogueOwnerReceipt& Owner,
            const FNpcDialogueObservation& Observation, const FString& Message,
            TOptional<ENpcDialogueTopic> Topic, FNpcDialogueSendIntent& Out);
        FString OwnerId;
        FGuid OwnerGeneration, ConversationGeneration, ActiveDispatch;
        TOptional<FNpcDialogueScope> SelectedScope;
        FNpcDialogueView View;
        bool bObserved = false, bDisposed = false;
        bool bActiveRequiresMark = false, bActiveMarked = false;
    };
}
