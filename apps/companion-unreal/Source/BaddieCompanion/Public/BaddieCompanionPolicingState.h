#pragma once

#include "BaddieCompanionCombatState.h"

namespace BaddieCompanion
{
    enum class EPoliceEncounterPhase : uint8 { Searching, Checking, Cleared, Arrested, Detained, Released };
    enum class EPoliceFinding : uint8 { Checkpoint, Harassment, ValidatedAssault, WeaponPossession, ObservedWeapon, Gunfire };
    enum class EPoliceDemandStatus : uint8 { Demanded, Paid, Refused };
    enum class EPolicingAction : uint8 { Cooperate, RequestRelease, PayExtortion, RefuseExtortion };
    enum class EPolicingExtortionResponse : uint8 { Pay, Refuse };

    struct FPoliceDemandEntry
    {
        FString EntryId;
        double Amount = 0;
    };
    struct FPoliceExtortion
    {
        double Amount = 0;
        ECombatCreditUnit UnitKey = ECombatCreditUnit::LagosCredit;
        EPoliceDemandStatus Status = EPoliceDemandStatus::Demanded;
        FString OutingId;
        TArray<FPoliceDemandEntry> Entries;
    };
    struct FPoliceEncounter
    {
        FString Id, CheckpointId, InstanceId, OfficerId;
        EPoliceEncounterPhase Phase = EPoliceEncounterPhase::Searching;
        EPoliceFinding Reason = EPoliceFinding::Checkpoint;
        // Original server milliseconds and source ScenePoint metres, not a native clock/pose.
        double CreatedAtMs = 0, ResolvesAtMs = 0;
        TOptional<double> ReleaseAtMs;
        TOptional<FString> StationPlaceId, StationInstanceId;
        FWorldScenePoint Position;
        FString EvidenceKey;
        // Wire numbers stay doubles. Demand capture alone applies safe-integer checks.
        double Revision = 0;
        TWorldPlayerField<ECombatStreetWeapon> WeaponId;
        TWorldPlayerField<FString> JourneyId;
        TWorldPlayerField<FWorldScenePoint> OfficerPosition;
        TWorldPlayerField<double> LastObservedAtMs;
        TWorldPlayerField<EWorldEnforcementAgency> Agency;
        TWorldPlayerField<EWorldEnforcementStopKind> StopKind;
        TWorldPlayerField<FPoliceExtortion> Extortion;
    };

    // Local consent data, never settlement inputs or native command permission.
    struct FPoliceDemandConfirmation
    {
        FString EncounterId, InstanceId;
        double Revision = 0, ResolvesAtMs = 0, Amount = 0;
        ECombatCreditUnit UnitKey = ECombatCreditUnit::LagosCredit;
        FString OutingId;
    };
    BADDIECOMPANION_API TOptional<FPoliceDemandConfirmation> CapturePoliceDemand(const FPoliceEncounter* Encounter);
    BADDIECOMPANION_API bool MatchesPoliceDemand(const FPoliceEncounter* Encounter, const FPoliceDemandConfirmation& Confirmed);

    enum class EPolicingResponseKind : uint8 { Subscription, Mutation };
    class FPolicingDecodedResponse;
    // Endpoint value only: encounter/null for snapshot, exactly null for every mutation.
    // Decoder failure clears Out. No mutable JSON or shared encounter escapes.
    BADDIECOMPANION_API bool DecodePolicingResponse(const FString& Json, EPolicingResponseKind Kind,
        FPolicingDecodedResponse& Out, FString& OutError);
    class BADDIECOMPANION_API FPolicingDecodedResponse
    {
    public:
        bool IsValid() const { return bValid && bHasEncounter == Encounter.IsValid(); }
        EPolicingResponseKind GetKind() const { return Kind; }
        const TSharedPtr<const FPoliceEncounter>& GetEncounter() const { return Encounter; }
    private:
        friend bool DecodePolicingResponse(const FString&, EPolicingResponseKind, FPolicingDecodedResponse&, FString&);
        bool bValid = false, bHasEncounter = false;
        EPolicingResponseKind Kind = EPolicingResponseKind::Subscription;
        TSharedPtr<const FPoliceEncounter> Encounter;
    };
    class FPolicingReceipt
    {
        friend class FPolicingReadState;
        friend class FPolicingPreparedCommand;
        FGuid Generation, CommandId;
        EPolicingResponseKind Kind = EPolicingResponseKind::Subscription;
        FString EncounterId;
    };
    class BADDIECOMPANION_API FPolicingPreparedCommand
    {
    public:
        EPolicingAction GetAction() const { return Action; }
        const FString& GetEncounterId() const { return Receipt.EncounterId; }
        const FPolicingReceipt& GetReceipt() const { return Receipt; }
        // Canonical account endpoint names describe the existing contract, not an admitted route.
        FString GetFunctionName() const;
        TOptional<EPolicingExtortionResponse> GetResponse() const;
    private:
        friend class FPolicingReadState;
        EPolicingAction Action = EPolicingAction::Cooperate;
        FPolicingReceipt Receipt;
    };
    struct FPolicingReadView
    {
        FString OwnerId;
        TSharedPtr<const FPoliceEncounter> Encounter;
        bool bReady = false, bBusy = false;
        TOptional<FString> Error;
    };
    enum class EPolicingReadResult : uint8 { Accepted, StaleReceipt, StaleEncounter, InvalidResponse };

    // Single-threaded state only. The caller owns subscriber counts, authenticated scope,
    // transport and UI notifications. No automatic action, timers, simulation or replication.
    class BADDIECOMPANION_API FPolicingReadState
    {
    public:
        FPolicingReadState() = default;
        FPolicingReadState(const FPolicingReadState&) = delete;
        FPolicingReadState& operator=(const FPolicingReadState&) = delete;
        const FPolicingReadView& GetView() const { return View; }
        // Local account identity, not a credential. Clear on access loss even for the same ID.
        void SelectOwner(const FString& OwnerId);
        // First subscriber/reconnect starts a generation and clears encounter/readiness/error.
        FPolicingReceipt BeginSubscription();
        // Last subscriber removal retains displayed data/error but retires pending callbacks.
        void EndSubscription();
        EPolicingReadResult Accept(const FPolicingReceipt& Receipt, const FPolicingDecodedResponse& Response);
        // Pass only an already extracted, display-safe error message, never a transport body.
        bool AcceptFailure(const FPolicingReceipt& Receipt, const TOptional<FString>& SafeMessage = {});
        // Call immediately before an independently admitted dispatch. This only captures the
        // canonical arguments and marks busy. Failed/cancelled dispatch must finish its receipt.
        TOptional<FPolicingPreparedCommand> PrepareCommand(EPolicingAction Action,
            const TOptional<FPoliceDemandConfirmation>& Confirmed = {});
        // Recheck after caller/UI callbacks before dispatch; still not permission to send.
        bool IsCurrentCommand(const FPolicingPreparedCommand& Command) const;
        void Clear();
        void Dispose();
    private:
        bool Current(const FPolicingReceipt& Receipt) const;
        bool SameEncounter(const FPolicingReceipt& Receipt) const;
        void FinishCommand();
        FPolicingReadView View;
        FGuid Generation, ActiveCommand;
        bool bSubscribed = false, bDisposed = false;
    };
}
