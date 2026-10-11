#pragma once

#include "CoreMinimal.h"
#include "BaddieCompanionWorldUtilities.h"
#include "Templates/Function.h"
#include <variant>

namespace BaddieCompanion
{
    enum class EPlayEmote : uint8 { Greeting, Nod, Cheer, Stretch, Groove, Sway };
    enum class EPlayLook : uint8 { Rose, Ivory };
    enum class EPlayToy : uint8 { Ball, Cushion, Music, Bubbles };
    enum class EPlayStylePhase : uint8 { Dress, Reveal };

    struct FPlayLook
    {
        EPlayLook Look = EPlayLook::Rose;
        FString TopColor, TrousersColor, FootwearColor;
    };
    struct FPlayPlayer
    {
        FString Id, Name, CharacterId;
        // Complete saved object, including field absence. Object/resource checks only;
        // not a validated CompanionAppearance or a render plan. Never persisted here.
        FString AppearanceJson;
        double Progress = 0, Score = 0;
        TOptional<FPlayLook> Look; // Required nullable on the wire.
        TOptional<FString> Vote; // Required nullable on the wire.
        FString LastActionId;
        double LastActionAt = 0;
        TOptional<double> MirrorWatchingAt; // Optional, but null is invalid.
    };
    struct FPlayFlight
    {
        double From = 0, To = 0, Power = 0, At = 0;
        bool bHit = false;
    };
    struct FPlayLastTouch
    {
        double Target = 0, At = 0;
        FString PlayerId;
    };
    // Saved compatibility only. Active-round projection always excludes this branch.
    struct FPlayPlaygroundGame
    {
        EPlayToy Toy = EPlayToy::Ball;
        double Hits = 0, Throws = 0;
        TArray<double> Notes;
        TOptional<FPlayFlight> Flight; // Required nullable.
        TOptional<FPlayLastTouch> LastTouch; // Optional, but null is invalid.
    };
    struct FPlayLastMove
    {
        FString PlayerId;
        EPlayEmote Emote = EPlayEmote::Greeting;
        double At = 0;
    };
    struct FPlayMirrorGame
    {
        TArray<EPlayEmote> Sequence;
        double ShowingAt = 0, Level = 0;
        TOptional<FString> Leader; // Required nullable.
        TOptional<FPlayLastMove> LastMove; // Required nullable.
    };
    struct FPlayMysteryGame
    {
        TArray<double> Order, Found, Entered;
        bool bUnlocked = false;
    };
    struct FPlayStyleGame
    {
        double Brief = 0;
        EPlayStylePhase Phase = EPlayStylePhase::Dress;
        TOptional<double> RevealedAt; // Required nullable.
    };
    struct FPlayKitchenGame
    {
        TArray<double> Recipe, Ingredients;
        bool bChopped = false;
        TOptional<double> HeatAt; // Required nullable.
        bool bCooked = false, bPlated = false;
        double Served = 0;
        bool bRush = false;
    };
    struct FPlayMemoriesGame
    {
        double Anchor = 0;
        TArray<EPlayEmote> Moves;
        bool bRecording = false;
    };
    // The active alternative is the saved kind. No inactive branch data or unknown kind.
    using FPlayGame = std::variant<FPlayPlaygroundGame, FPlayMirrorGame, FPlayMysteryGame,
        FPlayStyleGame, FPlayKitchenGame, FPlayMemoriesGame>;
    struct FPlayRound
    {
        FString Id, OwnerId;
        // Endpoint numbers stay finite doubles, not command-only integer/range rules.
        double Revision = 0, Seed = 0, StartedAt = 0, UpdatedAt = 0, ExpiresAt = 0;
        TArray<FPlayPlayer> Players;
        FPlayGame Game;
    };
    struct FPlayMemory
    {
        FString Id, Author;
        int32 Anchor = 0; // validPlayMemory's integer index 0..5.
        TArray<EPlayEmote> Moves;
        double CreatedAt = 0;
    };

    struct FPlayWireResult;
    // Immutable decoded value. Getters return detached copies, never parsed JSON pointers.
    // No account, room, invitation or call authority is represented here.
    class BADDIECOMPANION_API FPlayWire
    {
    public:
        FPlayWire(const FPlayWire&) = default;
        FPlayWire(FPlayWire&&) = default;
        FPlayWire& operator=(const FPlayWire&) = default;
        FPlayWire& operator=(FPlayWire&&) = default;
        TOptional<FPlayRound> GetRound() const { return Round; }
        FString GetPlayerId() const { return PlayerId; }
        TArray<FPlayMemory> GetMemories() const { return Memories; }
        // activePlayRound only: no expiry, membership, command or render eligibility check.
        TOptional<FPlayRound> GetActiveRound() const;
    private:
        FPlayWire() = default;
        friend FPlayWireResult DecodePlayWire(const FString& Json);
        friend FPlayWireResult ReconcilePlayWire(const TOptional<FPlayWire>& Previous, const FString& Json);
        TOptional<FPlayRound> Round;
        FString PlayerId;
        TArray<FPlayMemory> Memories;
    };
    enum class EPlayWireDisposition : uint8 { Invalid, Accepted, Retained };
    struct FPlayWireResult
    {
        EPlayWireDisposition Disposition = EPlayWireDisposition::Invalid;
        // Unset on any failure. An accepted wire with a null round remains set.
        TOptional<FPlayWire> Wire;
        FString Error; // Fixed diagnostic only; empty on accepted/retained results.
    };
    // Full object envelope required. All source times remain milliseconds.
    BADDIECOMPANION_API FPlayWireResult DecodePlayWire(const FString& Json);
    // Validates ALL incoming fields before revision comparison. Pure, with no retained owner.
    // Same ID + older revision retains the whole prior wire. All other valid input replaces it.
    // Caller must clear Previous across account/scope changes and authenticate every callback.
    BADDIECOMPANION_API FPlayWireResult ReconcilePlayWire(
        const TOptional<FPlayWire>& Previous, const FString& Json);

    struct FPlayScope
    {
        FString CharacterId, RoomId;
        TOptional<FString> CallSessionId, InvitationId;
        TOptional<double> RoomRevision;
    };
    struct FPlayRoomUtilityReceipt
    {
        int64 ServerNow = 0;
        FString CharacterId, RoomId, RoomKey;
        int64 RoomRevision = 0;
        TOptional<FString> InvitationId; // Required nullable on the wire.
        TOptional<FWorldUtilityObservation> Observation; // Required nullable on the wire.
        int64 RetryAfterMs = 0;
    };
    struct FPlayRoomUtilityState
    {
        FPlayRoomUtilityReceipt Receipt;
        // Original monotonic receipt time. Display projection must not refresh this age
        // or replace the retained observation with a projected sample.
        double ReceivedAtMs = 0;
    };
    struct FPlaySnapshot
    {
        // Unset until a wire is received, including a valid null-round wire.
        TOptional<FPlayWire> Wire;
        bool bBusy = false, bReady = false;
        TOptional<FString> Error;
        double ServerOffset = 0;
        TOptional<FPlayRoomUtilityState> RoomUtility;
        TOptional<FString> UtilityError;
    };

    // Playground and toy are representable only to reject retired caller input.
    enum class EPlayStartKind : uint8 { Mirror, Mystery, Style, Kitchen, Memories, Playground };
    struct FPlayStartCommand { EPlayStartKind Kind = EPlayStartKind::Mirror; TOptional<bool> Rush; };
    struct FPlayJoinCommand {};
    struct FPlayLeaveCommand {};
    struct FPlayReplayCommand {};
    struct FPlayLeadCommand {};
    struct FPlayPerformCommand {};
    struct FPlayWatchCommand {};
    struct FPlayChopCommand {};
    struct FPlayHeatCommand {};
    struct FPlayPlateCommand {};
    struct FPlayServeCommand {};
    struct FPlaySaveMemoryCommand {};
    struct FPlayTouchCommand { double Target = 0; TOptional<double> Power; };
    struct FPlayEmoteCommand { EPlayEmote Emote = EPlayEmote::Greeting; };
    struct FPlayLookCommand { FPlayLook Look; };
    struct FPlayVoteCommand { FString PlayerId; };
    struct FPlayIngredientCommand { double Ingredient = 0; };
    struct FPlayRemoveMemoryCommand { FString MemoryId; };
    struct FPlayRetiredToyCommand { EPlayToy Toy = EPlayToy::Ball; };
    // monostate represents malformed/unknown input, never an accepted command.
    using FPlayCommand = std::variant<std::monostate, FPlayStartCommand, FPlayJoinCommand,
        FPlayLeaveCommand, FPlayReplayCommand, FPlayLeadCommand, FPlayPerformCommand,
        FPlayWatchCommand, FPlayChopCommand, FPlayHeatCommand, FPlayPlateCommand,
        FPlayServeCommand, FPlaySaveMemoryCommand, FPlayTouchCommand, FPlayEmoteCommand,
        FPlayLookCommand, FPlayVoteCommand, FPlayIngredientCommand, FPlayRemoveMemoryCommand,
        FPlayRetiredToyCommand>;

    class FPlayObserverReceipt
    {
        friend class FPlayState;
        FGuid Id;
    };
    class FPlayQueryReceipt
    {
        friend class FPlayState;
        FGuid Session, Generation;
    };
    class FPlayQueryDispatch
    {
        friend class FPlayState;
    public:
        FString GetEndpoint() const { return TEXT("companionPlay:snapshot"); }
        FString GetArgumentsJson() const { return ArgumentsJson; }
        FPlayQueryReceipt GetReceipt() const { return Receipt; }
    private:
        FPlayQueryReceipt Receipt;
        FString ArgumentsJson;
    };
    class FPlayCommandReceipt
    {
        friend class FPlayState;
        FGuid Session, Generation, Id;
    };
    class FPlayCommandDispatch
    {
        friend class FPlayState;
    public:
        FString GetEndpoint() const { return TEXT("companionPlay:act"); }
        FString GetArgumentsJson() const { return ArgumentsJson; }
        FString GetActionId() const { return ActionId; }
        FPlayCommandReceipt GetReceipt() const { return Receipt; }
        bool ShouldRetry() const { return false; }
        bool ShouldInvalidateQueryCache() const { return false; }
    private:
        FPlayCommandReceipt Receipt;
        FString ArgumentsJson, ActionId;
    };
    class FPlayCommandCompletion
    {
        friend class FPlayState;
    public:
        // Invalid means a consumed failure, not a usable success wire.
        EPlayWireDisposition GetDisposition() const { return Disposition; }
    private:
        FPlayCommandReceipt Receipt;
        FGuid Id;
        EPlayWireDisposition Disposition = EPlayWireDisposition::Invalid;
    };

    class FPlayUtilityRequestReceipt
    {
        friend class FPlayState;
        FGuid Session, QueryGeneration, UtilityGeneration, Id;
    };
    class FPlayUtilityDispatch
    {
        friend class FPlayState;
    public:
        FString GetEndpoint() const { return TEXT("companionPlay:observeRoom"); }
        FString GetArgumentsJson() const { return ArgumentsJson; }
        FPlayUtilityRequestReceipt GetReceipt() const { return Receipt; }
        bool ShouldRetry() const { return false; }
        bool ShouldInvalidateQueryCache() const { return false; }
    private:
        FPlayUtilityRequestReceipt Receipt;
        FString ArgumentsJson;
    };
    class FPlayUtilityCompletion
    {
        friend class FPlayState;
    public:
        bool WasAccepted() const { return bAccepted; } // Includes valid null observation.
    private:
        FPlayUtilityRequestReceipt Receipt;
        FGuid Id;
        bool bAccepted = false;
    };

    // Account-private, single-game-thread owner. All getters/receipts are detached values.
    // Utility observations are retained data, not transport, permissions or scene power.
    // No credentials, timers, renewal scheduler or rendering are supplied.
    // Callbacks may reenter but must not throw or destroy this owner while a method runs.
    class BADDIECOMPANION_API FPlayState
    {
    public:
        FPlayState() = default;
        FPlayState(const FPlayState&) = delete;
        FPlayState& operator=(const FPlayState&) = delete;
        FPlayState(FPlayState&&) = delete;
        FPlayState& operator=(FPlayState&&) = delete;
        FPlaySnapshot GetSnapshot() const { return Snapshot; }
        TOptional<FPlayScope> GetScope() const { return Scope; }
        FPlayObserverReceipt Subscribe(TFunction<void()> Listener);
        bool Unsubscribe(FPlayObserverReceipt Observer);
        bool Select(TOptional<FPlayScope> Next);
        void Reconnect();
        // Unconditionally clears scope, private data, listeners and all receipts, even
        // when the next account/session reuses public IDs. Does not revive Dispose.
        void ResetSession();
        void Dispose();
        // Once per connection generation. The admitted caller owns subscription teardown;
        // stop its previous subscription whenever IsCurrentQuery becomes false.
        TOptional<FPlayQueryDispatch> TakeQuerySubscription();
        bool IsCurrentQuery(FPlayQueryReceipt Receipt) const;
        EPlayWireDisposition ObserveQuery(FPlayQueryReceipt Receipt, FString Json);
        // Only sanitized public text from an admitted transport, never raw bodies/errors.
        // Missing/oversized text uses fixed diagnostics; present empty text stays empty.
        bool ObserveQueryFailure(FPlayQueryReceipt Receipt, TOptional<FString> PublicMessage);
        // Validates, captures began time, sets busy/clears error and synchronously notifies.
        // Only after that notification does it capture scope/round into the packet.
        TOptional<FPlayCommandReceipt> BeginCommand(FPlayCommand Command);
        // Recheck and send immediately without a yield/notification. Once only, no retries.
        TOptional<FPlayCommandDispatch> MarkCommandDispatched(FPlayCommandReceipt Receipt);
        bool AbandonPreparedCommand(FPlayCommandReceipt Receipt);
        // Consumes a dispatched callback once. Success updates midpoint offset, accepts
        // or retains the whole wire, then notifies before finalization. Failure does not notify.
        TOptional<FPlayCommandCompletion> ObserveCommand(FPlayCommandReceipt Receipt, FString Json);
        TOptional<FPlayCommandCompletion> ObserveCommandFailure(
            FPlayCommandReceipt Receipt, TOptional<FString> PublicMessage);
        // A caller-side failure after success invalidates old completion copies. Applied
        // wire/offset survive; the replacement failure completion has no pre-final emission.
        TOptional<FPlayCommandCompletion> ObserveCommandCompletionFailure(
            FPlayCommandCompletion Completion, TOptional<FString> PublicMessage);
        // Call after success notification or failure handling. Removes only this request,
        // clears displayed busy and emits, even when a newer request is still outstanding.
        bool FinalizeCommand(FPlayCommandCompletion Completion);

        bool SetRoomUtilitiesActive(bool bActive);
        // Caller-driven attempts only. Uses monotonic milliseconds, a 15s pending window,
        // 45s retry pacing and the retained 5s re-entry floor. No busy/error command writes.
        TOptional<FPlayUtilityRequestReceipt> BeginRoomUtilityObservation();
        // One-request mutation transport stays outside this owner. Mark and send immediately
        // without yielding. Abort its handle when IsCurrentRoomUtilityRequest becomes false;
        // retirement here performs no network abort. Completion/finalization is separate.
        TOptional<FPlayUtilityDispatch> MarkRoomUtilityDispatched(FPlayUtilityRequestReceipt Receipt);
        bool IsCurrentRoomUtilityRequest(FPlayUtilityRequestReceipt Receipt) const;
        bool AbandonPreparedRoomUtilityRequest(FPlayUtilityRequestReceipt Receipt);
        // Current successes and failures emit before finalization. Malformed success uses
        // a fixed diagnostic. PublicMessage must already be sanitized, as for command failures.
        TOptional<FPlayUtilityCompletion> ObserveRoomUtility(FPlayUtilityRequestReceipt Receipt, FString Json);
        TOptional<FPlayUtilityCompletion> ObserveRoomUtilityFailure(
            FPlayUtilityRequestReceipt Receipt, TOptional<FString> PublicMessage);
        TOptional<FPlayUtilityCompletion> ObserveRoomUtilityCompletionFailure(
            FPlayUtilityCompletion Completion, TOptional<FString> PublicMessage);
        // Clears only the matching request. No notification or deadline/data changes.
        bool FinalizeRoomUtility(FPlayUtilityCompletion Completion);
    private:
        struct FObserver
        {
            FGuid Id;
            TSharedPtr<TFunction<void()>> Listener;
        };
        enum class ECommandStage : uint8 { PublishingBusy, Prepared, Dispatched, CompletingSuccess, CompletingFailure };
        struct FCommandRequest
        {
            FPlayCommandDispatch Dispatch;
            double BeganWallTime = 0;
            ECommandStage Stage = ECommandStage::PublishingBusy;
            FGuid CompletionId;
        };
        void Emit();
        void Connect();
        void RetireQuery();
        void FailQuery(const FString& PublicMessage);
        void Accept(const FPlayWireResult& Result);
        FCommandRequest* FindCommand(const FPlayCommandReceipt& Receipt);
        FCommandRequest* FindCompletion(const FPlayCommandCompletion& Completion);
        TOptional<FPlayCommandCompletion> BeginCompletion(const FPlayCommandReceipt& Receipt);
        enum class EUtilityStage : uint8 { Prepared, Dispatched, CompletingSuccess, CompletingFailure };
        struct FUtilityRequest
        {
            FPlayUtilityDispatch Dispatch;
            FString CharacterId, RoomId;
            int64 RoomRevision = 0;
            TOptional<FString> InvitationId;
            double BeganAtMs = 0;
            EUtilityStage Stage = EUtilityStage::Prepared;
            FGuid CompletionId;
        };
        bool UtilityEligible() const;
        void ClearUtilities();
        void FailUtility(const FString& PublicMessage);
        TOptional<FPlayUtilityCompletion> BeginUtilityCompletion(
            const FPlayUtilityRequestReceipt& Receipt, bool bAllowPrepared = false);
        bool IsCurrentUtilityCompletion(const FPlayUtilityCompletion& Completion) const;
        FPlaySnapshot Snapshot;
        TOptional<FPlayScope> Scope;
        TArray<FObserver> Observers;
        TMap<FGuid, FCommandRequest> Requests;
        FGuid Session = FGuid::NewGuid(), QueryGeneration = FGuid::NewGuid();
        bool bQueryAvailable = false, bQueryTaken = false, bDisposed = false;
        FGuid UtilityGeneration = FGuid::NewGuid();
        TOptional<FUtilityRequest> UtilityRequest;
        TOptional<FWorldUtilityObservation> PreviousUtilityObservation;
        double NextUtilityAtMs = 0, NextUtilityEntryAtMs = 0;
        bool bUtilityActive = false;
    };
}
