#pragma once

#include "BaddieCompanionWorldPlayer.h"
#include "Misc/Guid.h"

namespace BaddieCompanion
{
    DECLARE_MULTICAST_DELEGATE(FCombatReadChanged);
    enum class ECombatMode : uint8 { Melee, Arena };
    enum class ECombatPhase : uint8 { Playing, Finished, Cancelled };
    enum class ECombatFighterKind : uint8 { Player, Agbero };
    enum class ECombatFighterStatus : uint8 { Active, Defeated, Left };
    enum class ECombatEventKind : uint8 { Attack, Hit, Defeat, Reload, Finish, Left };
    // WorldPlayer's drawn-weapon union has only bat/pistol and is not this contract.
    enum class ECombatStreetWeapon : uint8 { Bat, Pistol, Revolver, Smg, Shotgun, Rifle };
    enum class ECombatRobberyPhase : uint8
    {
        Demanding, Fleeing, Defending, Surrendered, Escaped, Repelled, Defeated, Interrupted, Expired
    };
    enum class ECombatRobberyPurpose : uint8 { Recovery };
    enum class ECombatCreditUnit : uint8 { LagosCredit };

    struct FCombatRobberyView
    {
        FString Id, ToutId;
        ECombatRobberyPhase Phase = ECombatRobberyPhase::Demanding;
        ECombatCreditUnit UnitKey = ECombatCreditUnit::LagosCredit;
        double AmountCap = 0, CreatedAtMs = 0, RespondsAtMs = 0, ExpiresAtMs = 0;
        FWorldScenePoint Origin;
        TOptional<double> EscapeSinceMs, ResolvedAtMs;
        double SurrenderedAmount = 0;
        int64 Revision = 0;
        TWorldPlayerField<ECombatRobberyPurpose> Purpose;
    };
    struct FCombatSkillLevels { double Fighting = 0, SelfDefence = 0, Fitness = 0; };
    struct FCombatFighterView
    {
        FString Id, CharacterId;
        ECombatFighterKind Kind = ECombatFighterKind::Player;
        FWorldScenePoint Position;
        double YawRadians = 0, PitchRadians = 0, Health = 0, MaxHealth = 0, Stamina = 0;
        double Ammo = 0, ReserveAmmo = 0;
        TOptional<double> ReloadAtMs;
        ECombatFighterStatus Status = ECombatFighterStatus::Active;
        double JoinedAtMs = 0, LastAttackAtMs = 0, ProtectedUntilMs = 0, Kills = 0;
        int64 LastInputSequence = -1;
        TWorldPlayerField<ECombatStreetWeapon> StreetWeaponId;
        TWorldPlayerField<FString> TargetId;
        TWorldPlayerField<FCombatRobberyView> Robbery;
        TWorldPlayerField<FCombatSkillLevels> SkillLevels;
    };
    struct FCombatEventView
    {
        int64 Id = 0;
        ECombatEventKind Kind = ECombatEventKind::Attack;
        double AtMs = 0;
        // Empty actorId is a canonical terminal-event sentinel.
        FString ActorId;
        TOptional<FString> TargetId;
        TWorldPlayerField<FWorldScenePoint> ShotPoint;
        TWorldPlayerField<bool> Blocked;
    };
    struct FCombatGunfireOfficer
    {
        FString Id;
        FWorldScenePoint From;
        TArray<FWorldScenePoint> Route, ReturnRoute;
        double ResponseAtMs = 0, ArrivesAtMs = 0, PatrolOffsetMs = 0;
        TWorldPlayerField<double> RedirectedAtMs;
    };
    struct FCombatGunfireIncident
    {
        FString Id, ActorId;
        TWorldPlayerField<FString> LifeId;
        FWorldScenePoint Point;
        double StartedAtMs = 0, LastShotAtMs = 0;
        TArray<FCombatGunfireOfficer> Officers;
    };
    struct FCombatPatrolOffset { FString Id; double Milliseconds = 0; };
    struct FCombatGunfireView
    {
        TArray<FCombatPatrolOffset> PatrolOffsets;
        TArray<FCombatGunfireIncident> Incidents;
    };
    struct FCombatTuningView
    {
        double Health = 0, Stamina = 0, Speed = 0, MeleeDamage = 0, MeleeRange = 0;
        double MeleeCooldownMs = 0, BulletDamage = 0, BulletRange = 0, FireCooldownMs = 0;
        double Magazine = 0, ReserveAmmo = 0, ReloadMs = 0, NpcSpeed = 0, NpcWindupMs = 0;
        double BlockMultiplier = 0, SpawnProtectionMs = 0, DurationMs = 0, Target = 0;
    };
    struct FCombatMatchView
    {
        FString Id, InstanceId, PlaceId, LayoutId;
        TOptional<FString> CallSessionId;
        TWorldPlayerField<FString> MissionRunId;
        ECombatMode Mode = ECombatMode::Melee;
        ECombatPhase Phase = ECombatPhase::Playing;
        int64 Revision = 0;
        double SimulatedAtMs = 0, EndsAtMs = 0;
        // Source metres and radians. No generated navigation or local simulation.
        FWorldNavigation Navigation;
        TArray<FWorldScenePoint> Spawns;
        TArray<FCombatFighterView> Fighters;
        double ObjectiveTarget = 0;
        TOptional<FString> ObjectiveWinnerId;
        TArray<FCombatEventView> Events;
        FCombatTuningView Tuning;
        TWorldPlayerField<FCombatGunfireView> Gunfire;
    };

    class FCombatDecodedResponse;
    // Receives the endpoint's value, not a Convex envelope. Failure resets Out.
    BADDIECOMPANION_API bool DecodeCombatResponse(const FString& Json,
        FCombatDecodedResponse& Out, FString& OutError);
    // Only the decoder can construct a valid response. No JSON or mutable shared data escapes.
    class BADDIECOMPANION_API FCombatDecodedResponse
    {
    public:
        bool IsValid() const
        {
            return bValid && bHasMatch == Match.IsValid() && !PlayerId.IsEmpty()
                && (!LifeId.IsSet() || !LifeId.GetValue().IsEmpty());
        }
        const TSharedPtr<const FCombatMatchView>& GetMatch() const { return Match; }
        const FString& GetPlayerId() const { return PlayerId; }
        const TOptional<FString>& GetLifeId() const { return LifeId; }
        const TOptional<double>& GetServerTimeMs() const { return ServerTimeMs; }
    private:
        friend bool DecodeCombatResponse(const FString&, FCombatDecodedResponse&, FString&);
        bool bValid = false;
        bool bHasMatch = false;
        TSharedPtr<const FCombatMatchView> Match;
        FString PlayerId;
        TOptional<FString> LifeId;
        TOptional<double> ServerTimeMs;
    };
    enum class ECombatReceiptKind : uint8 { Subscription, Command, Input, NeutralRelease };
    enum class ECombatReadResult : uint8 { Accepted, OlderRevision, StaleReceipt, InvalidResponse, IdentityMismatch };
    struct FCombatInputValue
    {
        int64 Sequence = 0;
        // Source move[0] is right, move[1] is forward. Angles remain radians.
        double MoveRight = 0, MoveForward = 0, YawRadians = 0, PitchRadians = 0;
        bool bAttack = false, bBlock = false, bReload = false;
    };
    class FCombatReceipt
    {
    public:
        // Bind cleanup to its original admitted adapter context, never the current selection.
        const FString& GetInstanceId() const { return InstanceId; }
        const FString& GetExpectedPlayerId() const { return ExpectedPlayerId; }
        const FGuid& GetSubscriptionGeneration() const { return Generation; }
    private:
        friend class FCombatReadState;
        FGuid Generation, ScopeGeneration, InputGeneration, RequestId;
        ECombatReceiptKind Kind = ECombatReceiptKind::Subscription;
        FString InstanceId, ExpectedPlayerId;
        TOptional<FString> MatchId, LifeId;
        double BeganAtMs = 0;
        bool bOwned = false;
    };
    class FCombatRequest
    {
    public:
        const FString& GetEndpoint() const { return Endpoint; }
        const FString& GetArgumentsJson() const { return ArgumentsJson; }
        const FCombatReceipt& GetReceipt() const { return Receipt; }
    private:
        friend class FCombatReadState;
        FCombatReceipt Receipt;
        FString Endpoint, ArgumentsJson;
    };
    struct FCombatReadView
    {
        FString InstanceId, ExpectedPlayerId;
        EWorldFieldPresence StatePresence = EWorldFieldPresence::Missing;
        TSharedPtr<const FCombatMatchView> State;
        TWorldPlayerField<FString> LifeId;
        TOptional<FString> PlayerId, Error;
        bool bReady = false, bBusy = false;
        double ServerOffsetMs = 0;
    };

    // Single-threaded receipt bookkeeping, not an authenticated session or command admission.
    // The future transport owns authorization, lifetime, subscriptions and all outbound work.
    class BADDIECOMPANION_API FCombatReadState
    {
    public:
        FCombatReadState() = default;
        FCombatReadState(const FCombatReadState&) = delete;
        FCombatReadState& operator=(const FCombatReadState&) = delete;
        const FCombatReadView& GetView() const { return View; }
        FCombatReadChanged& OnChanged() { return Changed; }
        void Select(const FString& InstanceId, const FString& ExpectedPlayerId);
        FCombatReceipt BeginSubscription();
        void EndSubscription();
        // Records an already externally admitted operation. Neither method sends or permits one.
        TOptional<FCombatReceipt> CaptureCommand(double BeganAtMs);
        TOptional<FCombatReceipt> CaptureInput(double BeganAtMs, bool bNeutralRelease = false) const;
        void InvalidateInputReceipts();
        TOptional<FCombatRequest> PrepareJoin(double BeganAtMs);
        TOptional<FCombatRequest> PrepareLeave(double BeganAtMs);
        TOptional<FCombatRequest> PrepareRobberyAction(const FString& Action, double BeganAtMs);
        TOptional<FCombatRequest> PrepareInput(const FCombatInputValue& Input, double BeganAtMs);
        // No send or notification. Stop captures cleanup; the adapter drains it after each
        // lifecycle/response operation, including stale or silent terminal results.
        void StopInputs();
        // Pending identity supports abandonment only. Preparing gives it a new receipt.
        TOptional<FCombatReceipt> GetPendingStop() const;
        TOptional<FCombatRequest> PreparePendingStop(double BeganAtMs);
        // Send the successful returned packet immediately, without yielding or notifying.
        // The actual dispatch wall time owns rate accounting and mutation midpoint timing.
        // Only captured neutral cleanup can dispatch after selection loss or Dispose.
        TOptional<FCombatRequest> DispatchRequest(const FCombatReceipt& Receipt, double DispatchedAtMs);
        bool AbandonRequest(const FCombatReceipt& Receipt);
        // Keep this owner alive until sent flights settle and cleanup is drained/abandoned.
        bool HasInputWork() const { return InputFlight.IsSet() || PendingStop.IsSet(); }
        ECombatReadResult Accept(const FCombatReceipt& Receipt, const FCombatDecodedResponse& Response,
            double ReceivedAtMs);
        bool AcceptFailure(const FCombatReceipt& Receipt);
        void Clear();
        void Dispose();
    private:
        FCombatReceipt Capture(ECombatReceiptKind Kind, double BeganAtMs = 0) const;
        bool Current(const FCombatReceipt& Receipt) const;
        bool CurrentInput(const FCombatReceipt& Receipt) const;
        bool CanComplete(const FCombatReceipt& Receipt) const;
        bool MatchesFlight(const FCombatReceipt& Receipt) const;
        const FCombatFighterView* DisplayedFighter() const;
        bool CanStartCommand(double BeganAtMs) const;
        bool StartCommand(const FCombatRequest& Request);
        TOptional<FCombatRequest> PrepareCommand(const TCHAR* Endpoint, double BeganAtMs, const FString& Action);
        struct FInputWork
        {
            FCombatRequest Request;
            FCombatInputValue Input;
            bool bDispatched = false, bCompleting = false;
        };
        bool EncodeInput(FInputWork& Work) const;
        void ResetSelectedInput();
        void RetireCommand();
        void FinishCommand(const FCombatReceipt& Receipt);
        void FinishInput(const FCombatReceipt& Receipt);
        ECombatReadResult ApplyResponse(const FCombatReceipt& Receipt, const FCombatDecodedResponse& Response, double ReceivedAtMs);
        bool ApplyFailure(const FCombatReceipt& Receipt);
        FCombatReadView View;
        FGuid Generation, ScopeGeneration, InputGeneration, ActiveCommand;
        TOptional<FCombatRequest> PreparedCommand;
        bool bCommandDispatched = false;
        TOptional<FInputWork> InputFlight, PendingStop;
        TOptional<FCombatInputValue> LastInput;
        TOptional<double> LastSendMs;
        int64 LastSequence = -1;
        bool bSubscribed = false, bDisposed = false;
        FCombatReadChanged Changed;
    };
}
