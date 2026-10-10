#include "BaddieCompanionPolicingState.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include <cmath>
#include <initializer_list>

namespace BaddieCompanion
{
    namespace PolicingStatePrivate
    {
        using FObject = TSharedPtr<FJsonObject>;
        using FValue = TSharedPtr<FJsonValue>;
        constexpr int32 MaxJsonCharacters = 256 * 1024;
        constexpr int32 MaxEntries = 128; // COMPANION_HAUL_MAX_ENTRIES
        constexpr double MaxInteger = 9007199254740991.0;
        constexpr double MaxDemand = 100; // companionEnforcementTuning.extortionMaximum
        bool Same(const FString& A, const FString& B) { return A.Equals(B, ESearchCase::CaseSensitive); }
        bool SafeNonnegativeInteger(double N)
        {
            return std::isfinite(N) && N >= 0 && N <= MaxInteger && std::floor(N) == N;
        }
        FObject Object(const FValue& V) { return V.IsValid() && V->Type == EJson::Object ? V->AsObject() : nullptr; }
        FValue Field(const FObject& O, const TCHAR* Key) { return O.IsValid() ? O->TryGetField(Key) : nullptr; }
        bool Only(const FObject& O, std::initializer_list<const TCHAR*> Keys)
        {
            if (!O.IsValid()) return false;
            for (const auto& Pair : O->Values)
            {
                bool bKnown = false;
                for (const auto Key : Keys) if (Same(Pair.Key, Key)) { bKnown = true; break; }
                if (!bKnown) return false;
            }
            return true;
        }
        bool Parse(const FString& Json, FObject& Out)
        {
            if (Json.IsEmpty() || Json.Len() > MaxJsonCharacters) return false;
            bool bString = false, bEscape = false;
            int32 Depth = 0, Separators = 0;
            for (TCHAR C : Json)
            {
                if (bString)
                {
                    if (bEscape) bEscape = false;
                    else if (C == TEXT('\\')) bEscape = true;
                    else if (C == TEXT('"')) bString = false;
                    continue;
                }
                if (C == TEXT('"')) bString = true;
                else if (C == TEXT('{') || C == TEXT('[')) { if (++Depth > 8) return false; }
                else if (C == TEXT('}') || C == TEXT(']')) { if (--Depth < 0) return false; }
                else if (C == TEXT(',') || C == TEXT(':')) { if (++Separators > 2048) return false; }
            }
            return !bString && Depth == 0 && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Out) && Out.IsValid();
        }
        bool String(const FValue& V, FString& Out)
        {
            // Preserve exact strings, including empty values permitted by the public validator.
            return V.IsValid() && V->Type == EJson::String && V->TryGetString(Out);
        }
        bool Number(const FValue& V, double& Out)
        {
            return V.IsValid() && V->Type == EJson::Number && V->TryGetNumber(Out) && std::isfinite(Out);
        }
        template <typename T> bool Enum(const FValue& V, T& Out, std::initializer_list<TPair<const TCHAR*, T>> Choices)
        {
            FString S;
            if (!String(V, S)) return false;
            for (const auto& Choice : Choices) if (Same(S, Choice.Key)) { Out = Choice.Value; return true; }
            return false;
        }
        template <typename T, typename F> bool Nullable(const FValue& V, TOptional<T>& Out, F Read)
        {
            if (!V.IsValid()) return false;
            if (V->Type == EJson::Null) return true;
            T Next;
            if (!Read(V, Next)) return false;
            Out = MoveTemp(Next); return true;
        }
        template <typename T, typename F> bool Optional(const FObject& O, const TCHAR* Key,
            TWorldPlayerField<T>& Out, bool bNullable, F Read)
        {
            const auto V = Field(O, Key);
            if (!V.IsValid()) return true;
            if (V->Type == EJson::Null) { Out.Presence = EWorldFieldPresence::Null; return bNullable; }
            T Next;
            if (!Read(V, Next)) return false;
            Out.Presence = EWorldFieldPresence::Value; Out.Value = MoveTemp(Next); return true;
        }
        bool Point(const FValue& V, FWorldScenePoint& Out)
        {
            return V.IsValid() && V->Type == EJson::Array && V->AsArray().Num() == 3
                && Number(V->AsArray()[0], Out.X) && Number(V->AsArray()[1], Out.Y) && Number(V->AsArray()[2], Out.Z);
        }
        bool Weapon(const FValue& V, ECombatStreetWeapon& Out)
        {
            return Enum(V, Out, {{TEXT("bat"), ECombatStreetWeapon::Bat}, {TEXT("pistol"), ECombatStreetWeapon::Pistol},
                {TEXT("revolver"), ECombatStreetWeapon::Revolver}, {TEXT("smg"), ECombatStreetWeapon::Smg},
                {TEXT("shotgun"), ECombatStreetWeapon::Shotgun}, {TEXT("rifle"), ECombatStreetWeapon::Rifle}});
        }
        bool Agency(const FValue& V, EWorldEnforcementAgency& Out)
        {
            return Enum(V, Out, {{TEXT("police"), EWorldEnforcementAgency::Police}, {TEXT("lastma"), EWorldEnforcementAgency::Lastma},
                {TEXT("efcc"), EWorldEnforcementAgency::Efcc}, {TEXT("ndlea"), EWorldEnforcementAgency::Ndlea}});
        }
        bool StopKind(const FValue& V, EWorldEnforcementStopKind& Out)
        {
            return Enum(V, Out, {{TEXT("checkpoint"), EWorldEnforcementStopKind::Checkpoint},
                {TEXT("roadside"), EWorldEnforcementStopKind::Roadside}});
        }
        bool Extortion(const FValue& V, FPoliceExtortion& Out)
        {
            const auto O = Object(V);
            if (!Only(O, {TEXT("amount"), TEXT("unitKey"), TEXT("status"), TEXT("outingId"), TEXT("entries")})
                || !Number(Field(O, TEXT("amount")), Out.Amount)
                || !Enum(Field(O, TEXT("unitKey")), Out.UnitKey, {{TEXT("lagos-credit"), ECombatCreditUnit::LagosCredit}})
                || !Enum(Field(O, TEXT("status")), Out.Status, {{TEXT("demanded"), EPoliceDemandStatus::Demanded},
                    {TEXT("paid"), EPoliceDemandStatus::Paid}, {TEXT("refused"), EPoliceDemandStatus::Refused}})
                || !String(Field(O, TEXT("outingId")), Out.OutingId)) return false;
            const auto Entries = Field(O, TEXT("entries"));
            if (!Entries.IsValid() || Entries->Type != EJson::Array || Entries->AsArray().Num() > MaxEntries) return false;
            Out.Entries.Reserve(Entries->AsArray().Num());
            for (const auto& Value : Entries->AsArray())
            {
                const auto Entry = Object(Value);
                FPoliceDemandEntry Next;
                if (!Only(Entry, {TEXT("entryId"), TEXT("amount")}) || !String(Field(Entry, TEXT("entryId")), Next.EntryId)
                    || !Number(Field(Entry, TEXT("amount")), Next.Amount)) return false;
                Out.Entries.Add(MoveTemp(Next));
            }
            // No deduplication, sum correction or balance/settlement inference.
            return true;
        }
        bool Encounter(const FObject& O, FPoliceEncounter& Out)
        {
            return Only(O, {TEXT("id"), TEXT("checkpointId"), TEXT("instanceId"), TEXT("officerId"), TEXT("phase"),
                    TEXT("reason"), TEXT("createdAt"), TEXT("resolvesAt"), TEXT("releaseAt"), TEXT("stationPlaceId"),
                    TEXT("stationInstanceId"), TEXT("position"), TEXT("evidenceKey"), TEXT("revision"), TEXT("weaponId"),
                    TEXT("journeyId"), TEXT("officerPosition"), TEXT("lastObservedAt"), TEXT("agency"), TEXT("stopKind"), TEXT("extortion")})
                && String(Field(O, TEXT("id")), Out.Id) && String(Field(O, TEXT("checkpointId")), Out.CheckpointId)
                && String(Field(O, TEXT("instanceId")), Out.InstanceId) && String(Field(O, TEXT("officerId")), Out.OfficerId)
                && Enum(Field(O, TEXT("phase")), Out.Phase, {{TEXT("searching"), EPoliceEncounterPhase::Searching},
                    {TEXT("checking"), EPoliceEncounterPhase::Checking}, {TEXT("cleared"), EPoliceEncounterPhase::Cleared},
                    {TEXT("arrested"), EPoliceEncounterPhase::Arrested}, {TEXT("detained"), EPoliceEncounterPhase::Detained},
                    {TEXT("released"), EPoliceEncounterPhase::Released}})
                && Enum(Field(O, TEXT("reason")), Out.Reason, {{TEXT("checkpoint"), EPoliceFinding::Checkpoint},
                    {TEXT("harassment"), EPoliceFinding::Harassment}, {TEXT("validated-assault"), EPoliceFinding::ValidatedAssault},
                    {TEXT("weapon-possession"), EPoliceFinding::WeaponPossession}, {TEXT("observed-weapon"), EPoliceFinding::ObservedWeapon},
                    {TEXT("gunfire"), EPoliceFinding::Gunfire}})
                && Number(Field(O, TEXT("createdAt")), Out.CreatedAtMs) && Number(Field(O, TEXT("resolvesAt")), Out.ResolvesAtMs)
                && Nullable(Field(O, TEXT("releaseAt")), Out.ReleaseAtMs, Number)
                && Nullable(Field(O, TEXT("stationPlaceId")), Out.StationPlaceId, String)
                && Nullable(Field(O, TEXT("stationInstanceId")), Out.StationInstanceId, String)
                && Point(Field(O, TEXT("position")), Out.Position) && String(Field(O, TEXT("evidenceKey")), Out.EvidenceKey)
                && Number(Field(O, TEXT("revision")), Out.Revision)
                && Optional(O, TEXT("weaponId"), Out.WeaponId, true, Weapon)
                && Optional(O, TEXT("journeyId"), Out.JourneyId, true, String)
                && Optional(O, TEXT("officerPosition"), Out.OfficerPosition, false, Point)
                && Optional(O, TEXT("lastObservedAt"), Out.LastObservedAtMs, false, Number)
                && Optional(O, TEXT("agency"), Out.Agency, false, Agency)
                && Optional(O, TEXT("stopKind"), Out.StopKind, false, StopKind)
                && Optional(O, TEXT("extortion"), Out.Extortion, true, Extortion);
        }
        FString FailureMessage(const TOptional<FString>& SafeMessage, bool bSubscription)
        {
            if (SafeMessage.IsSet() && SafeMessage.GetValue().Len() <= 1024) return SafeMessage.GetValue();
            return bSubscription ? TEXT("Policing could not connect. Try again.") : TEXT("The officer could not complete that action.");
        }
    }

    TOptional<FPoliceDemandConfirmation> CapturePoliceDemand(const FPoliceEncounter* Encounter)
    {
        using namespace PolicingStatePrivate;
        if (!Encounter || Encounter->Phase != EPoliceEncounterPhase::Checking
            || Encounter->Extortion.Presence != EWorldFieldPresence::Value || !Encounter->Extortion.Value.IsSet()) return {};
        const auto& Demand = Encounter->Extortion.Value.GetValue();
        if (Demand.Status != EPoliceDemandStatus::Demanded || Demand.UnitKey != ECombatCreditUnit::LagosCredit
            || !SafeNonnegativeInteger(Demand.Amount) || Demand.Amount < 1 || Demand.Amount > MaxDemand
            || !SafeNonnegativeInteger(Encounter->Revision) || !SafeNonnegativeInteger(Encounter->ResolvesAtMs)) return {};
        FPoliceDemandConfirmation Out;
        Out.EncounterId = Encounter->Id; Out.InstanceId = Encounter->InstanceId; Out.Revision = Encounter->Revision;
        Out.ResolvesAtMs = Encounter->ResolvesAtMs; Out.Amount = Demand.Amount; Out.UnitKey = Demand.UnitKey; Out.OutingId = Demand.OutingId;
        return Out;
    }
    bool MatchesPoliceDemand(const FPoliceEncounter* Encounter, const FPoliceDemandConfirmation& Confirmed)
    {
        using namespace PolicingStatePrivate;
        const auto Current = CapturePoliceDemand(Encounter);
        if (!Current.IsSet()) return false;
        const auto& Demand = Current.GetValue();
        return Same(Demand.EncounterId, Confirmed.EncounterId) && Same(Demand.InstanceId, Confirmed.InstanceId)
            && Demand.Revision == Confirmed.Revision && Demand.ResolvesAtMs == Confirmed.ResolvesAtMs
            && Demand.Amount == Confirmed.Amount && Demand.UnitKey == Confirmed.UnitKey && Same(Demand.OutingId, Confirmed.OutingId);
    }
    bool DecodePolicingResponse(const FString& Json, EPolicingResponseKind Kind, FPolicingDecodedResponse& Out, FString& OutError)
    {
        using namespace PolicingStatePrivate;
        Out = {}; OutError = TEXT("Policing response is unsupported or malformed.");
        if (Json.IsEmpty() || Json.Len() > MaxJsonCharacters
            || (Kind != EPolicingResponseKind::Subscription && Kind != EPolicingResponseKind::Mutation)) return false;
        FPolicingDecodedResponse Next; Next.Kind = Kind;
        if (Same(Json.TrimStartAndEnd(), TEXT("null")))
        {
            Next.bValid = true; Out = MoveTemp(Next); OutError.Reset(); return true;
        }
        if (Kind == EPolicingResponseKind::Mutation) return false;
        FObject O;
        FPoliceEncounter Candidate;
        if (!Parse(Json, O) || !Encounter(O, Candidate)) return false;
        Next.Encounter = MakeShared<FPoliceEncounter>(MoveTemp(Candidate));
        Next.bHasEncounter = true; Next.bValid = true; Out = MoveTemp(Next); OutError.Reset(); return true;
    }

    FString FPolicingPreparedCommand::GetFunctionName() const
    {
        switch (Action)
        {
        case EPolicingAction::Cooperate: return TEXT("companionPolicing:cooperate");
        case EPolicingAction::RequestRelease: return TEXT("companionPolicing:requestRelease");
        case EPolicingAction::PayExtortion:
        case EPolicingAction::RefuseExtortion: return TEXT("companionPolicing:respondExtortion");
        default: return {};
        }
    }
    TOptional<EPolicingExtortionResponse> FPolicingPreparedCommand::GetResponse() const
    {
        if (Action == EPolicingAction::PayExtortion) return EPolicingExtortionResponse::Pay;
        if (Action == EPolicingAction::RefuseExtortion) return EPolicingExtortionResponse::Refuse;
        return {};
    }
    void FPolicingReadState::SelectOwner(const FString& OwnerId)
    {
        if (bDisposed || PolicingStatePrivate::Same(View.OwnerId, OwnerId)) return;
        const FString NextOwner = OwnerId;
        Clear(); View.OwnerId = NextOwner;
    }
    FPolicingReceipt FPolicingReadState::BeginSubscription()
    {
        if (bDisposed || View.OwnerId.IsEmpty()) return {};
        Generation = FGuid::NewGuid(); ActiveCommand.Invalidate(); bSubscribed = true;
        View.Encounter.Reset(); View.bReady = false; View.bBusy = false; View.Error.Reset();
        FPolicingReceipt Receipt; Receipt.Generation = Generation; return Receipt;
    }
    void FPolicingReadState::EndSubscription()
    {
        if (bDisposed) return;
        Generation = FGuid::NewGuid(); ActiveCommand.Invalidate(); bSubscribed = false; View.bBusy = false;
    }
    bool FPolicingReadState::Current(const FPolicingReceipt& Receipt) const
    {
        return !bDisposed && bSubscribed && Receipt.Generation.IsValid() && Receipt.Generation == Generation
            && (Receipt.Kind == EPolicingResponseKind::Subscription || (ActiveCommand.IsValid() && Receipt.CommandId == ActiveCommand));
    }
    bool FPolicingReadState::SameEncounter(const FPolicingReceipt& Receipt) const
    {
        return View.Encounter.IsValid() && PolicingStatePrivate::Same(View.Encounter->Id, Receipt.EncounterId);
    }
    void FPolicingReadState::FinishCommand()
    {
        ActiveCommand.Invalidate(); View.bBusy = false;
    }
    EPolicingReadResult FPolicingReadState::Accept(const FPolicingReceipt& Receipt, const FPolicingDecodedResponse& Response)
    {
        if (!Current(Receipt)) return EPolicingReadResult::StaleReceipt;
        if (!Response.IsValid() || Response.GetKind() != Receipt.Kind)
        {
            AcceptFailure(Receipt); return EPolicingReadResult::InvalidResponse;
        }
        if (Receipt.Kind == EPolicingResponseKind::Subscription)
        {
            // Canonical subscription order wins, including lower revisions and changed IDs.
            View.Encounter = Response.GetEncounter(); View.bReady = true; View.Error.Reset();
            return EPolicingReadResult::Accepted;
        }
        const bool bSameEncounter = SameEncounter(Receipt);
        // A null acknowledgement never replaces the encounter or clears an intervening error.
        // Even an old encounter's completion releases busy in this same generation.
        FinishCommand();
        return bSameEncounter ? EPolicingReadResult::Accepted : EPolicingReadResult::StaleEncounter;
    }
    bool FPolicingReadState::AcceptFailure(const FPolicingReceipt& Receipt, const TOptional<FString>& SafeMessage)
    {
        if (!Current(Receipt)) return false;
        if (Receipt.Kind == EPolicingResponseKind::Subscription)
        {
            View.bReady = false; View.Error = PolicingStatePrivate::FailureMessage(SafeMessage, true);
        }
        else
        {
            if (SameEncounter(Receipt)) View.Error = PolicingStatePrivate::FailureMessage(SafeMessage, false);
            FinishCommand();
        }
        return true;
    }
    TOptional<FPolicingPreparedCommand> FPolicingReadState::PrepareCommand(EPolicingAction Action,
        const TOptional<FPoliceDemandConfirmation>& Confirmed)
    {
        if (bDisposed || !bSubscribed || View.bBusy) return {};
        if (Action != EPolicingAction::Cooperate && Action != EPolicingAction::RequestRelease
            && Action != EPolicingAction::PayExtortion && Action != EPolicingAction::RefuseExtortion) return {};
        if (Action == EPolicingAction::PayExtortion
            && (!View.bReady || !Confirmed.IsSet() || !MatchesPoliceDemand(View.Encounter.Get(), Confirmed.GetValue())))
        {
            View.Error = View.bReady ? TEXT("This demand changed. Cancel and review the current encounter.")
                : TEXT("Wait for the current encounter to load.");
            return {};
        }
        if (!View.bReady || !View.Encounter.IsValid()) return {};
        FPolicingPreparedCommand Command; Command.Action = Action;
        Command.Receipt.Generation = Generation; Command.Receipt.Kind = EPolicingResponseKind::Mutation;
        Command.Receipt.EncounterId = View.Encounter->Id;
        ActiveCommand = FGuid::NewGuid(); Command.Receipt.CommandId = ActiveCommand;
        View.bBusy = true; View.Error.Reset(); return Command;
    }
    bool FPolicingReadState::IsCurrentCommand(const FPolicingPreparedCommand& Command) const
    {
        return Command.Receipt.Kind == EPolicingResponseKind::Mutation && Current(Command.Receipt);
    }
    void FPolicingReadState::Clear()
    {
        if (bDisposed) return;
        View = {}; Generation = FGuid::NewGuid(); ActiveCommand.Invalidate(); bSubscribed = false;
    }
    void FPolicingReadState::Dispose()
    {
        if (bDisposed) return;
        Clear(); bDisposed = true; Generation.Invalidate();
    }
}
