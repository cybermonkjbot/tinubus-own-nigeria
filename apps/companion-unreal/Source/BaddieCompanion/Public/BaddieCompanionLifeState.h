#pragma once

#include "CoreMinimal.h"
#include "Misc/Guid.h"

namespace BaddieCompanion
{
    enum class ELifeActivityKind : uint8 { Sleep, Sit, Play, Cook, Eat, Bathe, Work, WatchTv, Read, Stretch, Wash };
    enum class ELifeActivityPhase : uint8 { Approaching, Active };
    struct FLifeNeeds
    {
        double Hunger = 0, Energy = 0, Cleanliness = 0, Enjoyment = 0;
    };
    struct FLifeActivity
    {
        FString Id, RoomId, StationId;
        ELifeActivityKind Kind = ELifeActivityKind::Sleep;
        // Finite wire milliseconds, not safe-integer counters or a locally advancing clock.
        double StartedAtMs = 0, EndsAtMs = 0;
        // Missing legacy fields are not replaced by active/zero/empty values. Null is invalid.
        TOptional<ELifeActivityPhase> Phase;
        TOptional<double> RequestedAtMs;
        TOptional<FString> RoomKey;
    };
    enum class ELifeTaskStatus : uint8 { Waiting, Active, Blocked };
    struct FLifeTaskCooking
    {
        FString RecipeKey;
        int32 Batches = 1;
    };
    struct FLifeTask
    {
        FString Id, RoomId, StationId;
        ELifeActivityKind Kind = ELifeActivityKind::Sleep;
        ELifeTaskStatus Status = ELifeTaskStatus::Waiting;
        // Required nullable wire fields. Unset means a decoded null, not omission.
        TOptional<FString> ActivityId, Reason;
        TOptional<FLifeTaskCooking> Cooking;
    };
    struct FLifeTaskQueue
    {
        // At most eight tasks in supplied order, with case-sensitive unique IDs.
        TArray<FLifeTask> Tasks;
        bool bPaused = false;
        TOptional<FString> Reason;
    };
    struct FLifeSkillProgress { int32 Experience = 0; };
    struct FLifeSkills
    {
        FLifeSkillProgress Cooking;
        // Cooking is required when skills is present. The other seven may be absent.
        TOptional<FLifeSkillProgress> Fighting, SelfDefence, Charisma, Fitness, Hustle, Dance, Flirting;
    };
    struct FLife
    {
        FString CharacterId;
        FLifeNeeds Needs;
        double UpdatedAtMs = 0;
        int64 Revision = 0;
        TOptional<FLifeActivity> Activity;
        int32 Meals = 0;
        bool bReminders = false;
        // Legacy absence stays distinct from empty queue or zero-valued skills. Null is invalid.
        TOptional<FLifeTaskQueue> TaskQueue;
        TOptional<FLifeSkills> Skills;
    };

    // Decodes companionLife:snapshot's value, not an envelope. True + unset Out means
    // decoded null; the caller must establish its authoritative query origin. Missing or
    // malformed input returns false and clears Out atomically.
    BADDIECOMPANION_API bool DecodeLifeResponse(const FString& Json, TOptional<FLife>& Out, FString& OutError);

    struct FLifeReadScope
    {
        // Caller-owned account identity, never a token or a grant. Not sent by this owner.
        FString AccountId, CharacterId;
    };
    class FLifeReadReceipt
    {
    public:
        FLifeReadReceipt() = default;
        const TOptional<FLifeReadScope>& GetScope() const { return Scope; }
    private:
        friend class FLifeReadState;
        FGuid Generation;
        TOptional<FLifeReadScope> Scope;
        TSharedPtr<const FLife> Fresh;
    };
    enum class ELifeReadOrigin : uint8 { Unavailable, SavedValue, DefaultForNull };
    struct FLifeReadView
    {
        TSharedPtr<const FLife> Life;
        // DefaultForNull is a captured local default, not a server save or action permission.
        ELifeReadOrigin Origin = ELifeReadOrigin::Unavailable;
        // Read errors only. Future pending-act errors must be composed separately.
        TOptional<FString> Error;
    };
    enum class ELifeReadResult : uint8 { Accepted, OlderRevision, CharacterMismatch, InvalidResponse, StaleReceipt };
    struct FLifePublicReadFailure
    {
        // Already-safe public text supplied by the transport. Never pass raw exceptions,
        // response envelopes or credentials. The read owner does not sanitize this string.
        FString Message;
    };
    DECLARE_MULTICAST_DELEGATE(FLifeReadChanged);

    // Single-threaded, caller-driven account/character read state. No care projection,
    // commands, retries, scheduling, transport or admission. Delegate binding is not observation.
    class BADDIECOMPANION_API FLifeReadState
    {
    public:
        FLifeReadState() = default;
        FLifeReadState(const FLifeReadState&) = delete;
        FLifeReadState& operator=(const FLifeReadState&) = delete;
        FLifeReadState(FLifeReadState&&) = delete;
        FLifeReadState& operator=(FLifeReadState&&) = delete;
        const FLifeReadView& GetView() const { return View; }
        const FLifeReadScope& GetSelection() const { return Selection; }
        FLifeReadChanged& OnChanged() { return Changed; }
        bool Select(const FString& AccountId, const FString& CharacterId);
        // Call on first observer or reconnect. Retires old callbacks and retains display.
        // ConnectedAtMs must be finite and nonnegative. Captures one newCompanionLife
        // equivalent for this observation; it is used only after decoding authoritative null.
        // False leaves Out invalid and retires prior callbacks without clearing presentation.
        bool BeginSubscription(double ConnectedAtMs, FLifeReadReceipt& Out);
        void EndSubscription();
        ELifeReadResult Accept(const FLifeReadReceipt& Receipt, const FString& Json);
        bool AcceptFailure(const FLifeReadReceipt& Receipt, const FLifePublicReadFailure& Failure);
        void Clear();
        void Dispose();
    private:
        bool Current(const FLifeReadReceipt& Receipt) const;
        void FailRead(const FString& Message);
        FLifeReadScope Selection;
        FLifeReadView View;
        FGuid Generation;
        bool bSubscribed = false, bDisposed = false;
        FLifeReadChanged Changed;
    };
}
