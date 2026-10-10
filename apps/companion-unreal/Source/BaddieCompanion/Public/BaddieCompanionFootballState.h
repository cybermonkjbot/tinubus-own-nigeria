#pragma once

#include "CoreMinimal.h"
#include "Misc/Guid.h"

namespace BaddieCompanion
{
    enum class EFootballSide : uint8 { Home, Away };
    enum class EFootballMode : uint8 { Solo, Teams, Coop };
    enum class EFootballPhase : uint8 { Lobby, Playing, Restart, Finished, Cancelled };
    enum class EFootballWinner : uint8 { Home, Away, Draw };
    enum class EFootballEventKind : uint8 { Kickoff, Goal, Out, Save, Finished, Cancelled };

    // Source metres, with no Unreal-axis conversion. Wire numbers remain finite doubles;
    // decoding is not a gameplay validity check or permission to simulate a match.
    struct FFootballPoint { double X = 0, Z = 0; };
    struct FFootballPitch
    {
        FString Id, InstanceId;
        double Revision = 0, FloorY = 0;
        FFootballPoint Centre;
        double HalfWidth = 0, HalfLength = 0, GoalHalfWidth = 0, GoalHeight = 0;
    };
    struct FFootballPlayer
    {
        FString Id;
        EFootballSide Side = EFootballSide::Home;
        bool bBot = false, bGoalkeeper = false;
        FFootballPoint Position, Velocity;
        double Yaw = 0;
        bool bReady = false, bConnected = false;
        // ActionAtMs may be -1000 in a newly created match.
        double LastSequence = 0, ActionAtMs = 0, Goals = 0, Assists = 0;
    };
    struct FFootballBall
    {
        FFootballPoint Position, Velocity;
        double Height = 0, VerticalSpeed = 0;
        TOptional<FString> OwnerId, LastTouchId, LastPassId;
        double ClaimAfterMs = 0;
    };
    struct FFootballScore { double Home = 0, Away = 0; };
    struct FFootballObjective { double GoalsToWin = 0, DurationMs = 0; };
    struct FFootballEvent
    {
        double Id = 0;
        EFootballEventKind Kind = EFootballEventKind::Kickoff;
        TOptional<EFootballSide> Side;
        TOptional<FString> PlayerId;
    };
    struct FFootballMatch
    {
        FString Id;
        double Epoch = 0, Revision = 0;
        FFootballPitch Pitch;
        EFootballMode Mode = EFootballMode::Solo;
        EFootballPhase Phase = EFootballPhase::Lobby;
        TArray<FFootballPlayer> Players;
        FFootballBall Ball;
        FFootballScore Score;
        TOptional<EFootballWinner> Winner;
        TOptional<double> StartedAtMs, EndsAtMs;
        double SimulatedAtMs = 0;
        TOptional<double> RestartAtMs;
        EFootballSide RestartSide = EFootballSide::Home;
        FFootballObjective Objective;
        TOptional<FFootballEvent> LastEvent;
    };
    struct FFootballAdvancement
    {
        TOptional<FString> OwnerId;
        double OwnerUntilMs = 0;
        TOptional<double> NextAtMs;
        double ServerNowMs = 0;
    };
    struct FFootballSnapshot
    {
        TOptional<FString> PlayerId;
        bool bHost = false;
        TOptional<FString> CallSessionId;
        TOptional<FFootballMatch> Match;
        bool bReady = false, bBusy = false;
        TOptional<FString> Error;
        // Only this field may be absent. A present null advancement is invalid.
        TOptional<FFootballAdvancement> Advancement;
    };
    struct FFootballEntryPlayer
    {
        FString Id, Name;
        TOptional<EFootballSide> Side;
    };
    struct FFootballEntryRoster
    {
        FString PlayerId, InstanceId;
        double Revision = 0, Epoch = 0;
        TOptional<EFootballPhase> Phase;
        double ServerNowMs = 0;
        // Up to 50 entry participants, not the match's 12 players. Supplied order stays intact.
        TArray<FFootballEntryPlayer> Players;
    };

    // Endpoint values only, not transport envelopes. Roots must be objects. Every
    // TOptional except advancement represents a REQUIRED nullable field. Failure clears Out.
    BADDIECOMPANION_API bool DecodeFootballSnapshot(const FString& Json, TOptional<FFootballSnapshot>& Out, FString& OutError);
    BADDIECOMPANION_API bool DecodeFootballEntryRoster(const FString& Json, TOptional<FFootballEntryRoster>& Out, FString& OutError);

    struct FFootballReadScope
    {
        // Nonsecret caller identities. SessionIncarnation must change on session replacement,
        // even when account and room IDs do not. None of these establish server admission.
        FString AccountId, SessionIncarnation, RoomId;
    };
    class FFootballReadReceipt
    {
    public:
        FFootballReadReceipt() = default;
        const TOptional<FFootballReadScope>& GetScope() const { return Scope; }
        int64 GetCapturedNowMs() const { return CapturedNowMs; }
    private:
        friend class FFootballReadState;
        FGuid Generation;
        TOptional<FFootballReadScope> Scope;
        int64 CapturedNowMs = 0;
        bool bEntry = false;
    };
    enum class EFootballReadResult : uint8 { Accepted, OlderEpoch, OlderRevision, OlderAdvancement, InvalidResponse, StaleReceipt };
    struct FFootballPublicReadFailure
    {
        // Transport-sanitized public text only. Never raw exceptions, tokens or envelopes.
        // Unset requests the fallback; a present empty message stays empty.
        TOptional<FString> Message;
    };
    DECLARE_MULTICAST_DELEGATE(FFootballReadChanged);

    // Single-threaded read ownership only. The caller starts/stops actual queries and
    // tracks listener counts. Binding OnChanged does not start observation or transport.
    class BADDIECOMPANION_API FFootballReadState
    {
    public:
        FFootballReadState() = default;
        FFootballReadState(const FFootballReadState&) = delete;
        FFootballReadState& operator=(const FFootballReadState&) = delete;
        FFootballReadState(FFootballReadState&&) = delete;
        FFootballReadState& operator=(FFootballReadState&&) = delete;
        // Owner view uses local read Error and keeps bBusy false. Wire busy/error decode
        // fully, but never become an invented command latch or an accepted read failure.
        const FFootballSnapshot& GetView() const { return View; }
        const FFootballReadScope& GetSelection() const { return Selection; }
        FFootballReadChanged& OnChanged() { return Changed; }
        bool Select(const FFootballReadScope& Scope);
        // Unconditional reset, including same-ID session replacement. Retires all receipts.
        bool ResetSession(const FFootballReadScope& Scope);
        // Call on first match listener/reconnect. Captures now once, in the backend's
        // inclusive 0..4102444800000 range. Invalid intent retires the old match receipt.
        bool BeginMatchObservation(int64 CapturedNowMs, FFootballReadReceipt& Out);
        void EndMatchObservation();
        // Clears read error immediately. Reconnects only if match observation is active.
        bool ReloadMatch(int64 CapturedNowMs, FFootballReadReceipt& Out);
        EFootballReadResult AcceptMatch(const FFootballReadReceipt& Receipt, const FString& Json);
        bool AcceptMatchFailure(const FFootballReadReceipt& Receipt, const FFootballPublicReadFailure& Failure);
        // Each entry observer gets its own receipt. There is no shared roster or ordering
        // reducer. Route Out/Error only to the matching observer; stale callbacks clear Out.
        bool BeginEntryObservation(int64 CapturedNowMs, FFootballReadReceipt& Out);
        void EndEntryObservation(const FFootballReadReceipt& Receipt);
        EFootballReadResult AcceptEntry(const FFootballReadReceipt& Receipt, const FString& Json,
            TOptional<FFootballEntryRoster>& Out, FString& OutError) const;
        bool AcceptEntryFailure(const FFootballReadReceipt& Receipt, const FFootballPublicReadFailure& Failure,
            FString& OutError) const;
        void Clear();
        void Dispose();
    private:
        bool CurrentMatch(const FFootballReadReceipt& Receipt) const;
        bool CurrentEntry(const FFootballReadReceipt& Receipt) const;
        bool CanObserve(int64 CapturedNowMs) const;
        void FailMatch(const FString& Message);
        FFootballReadScope Selection;
        FFootballSnapshot View;
        FGuid MatchGeneration;
        TSet<FGuid> EntryGenerations;
        bool bMatchObserved = false, bHasApplied = false, bDisposed = false;
        FFootballReadChanged Changed;
    };
}
