#include "BaddieCompanionCombatState.h"
#include "BaddieCompanionStateJson.h"

namespace BaddieCompanion
{
    namespace CombatStatePrivate
    {
        using StateJsonPrivate::FObject;
        using StateJsonPrivate::FValue;
        using StateJsonPrivate::Same;
        using StateJsonPrivate::Object;
        using StateJsonPrivate::Field;
        using StateJsonPrivate::Only;
        using StateJsonPrivate::Parse;
        using StateJsonPrivate::String;
        using StateJsonPrivate::Number;
        using StateJsonPrivate::Integer;
        using StateJsonPrivate::Boolean;
        using StateJsonPrivate::Nullable;
        using StateJsonPrivate::ValidTime;
        constexpr StateJsonPrivate::FParseLimits ParseLimits{2 * 1024 * 1024, 32, 131072};
        bool Same(const TOptional<FString>& A, const TOptional<FString>& B)
        {
            return A.IsSet() == B.IsSet() && (!A.IsSet() || Same(A.GetValue(), B.GetValue()));
        }
        TOptional<FString> MatchId(const TSharedPtr<const FCombatMatchView>& Match)
        {
            return Match.IsValid() ? TOptional<FString>(Match->Id) : TOptional<FString>();
        }
        template <typename T> bool Enum(const FValue& V, T& Out, std::initializer_list<TPair<const TCHAR*, T>> Choices)
        {
            FString S;
            if (!String(V, S)) return false;
            for (const auto& Choice : Choices) if (Same(S, Choice.Key)) { Out = Choice.Value; return true; }
            return false;
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
        template <typename T, typename F> bool Array(const FValue& V, TArray<T>& Out, F Read, int32 Maximum = 43691)
        {
            if (!V.IsValid() || V->Type != EJson::Array || V->AsArray().Num() > Maximum) return false;
            Out.Reserve(V->AsArray().Num());
            for (const auto& Entry : V->AsArray())
            {
                T Next;
                if (!Read(Entry, Next)) return false;
                Out.Add(MoveTemp(Next));
            }
            return true;
        }
        bool Point(const FValue& V, FWorldScenePoint& Out)
        {
            if (!V.IsValid() || V->Type != EJson::Array) return false;
            const auto& A = V->AsArray();
            return A.Num() == 3 && Number(A[0], Out.X) && Number(A[1], Out.Y) && Number(A[2], Out.Z);
        }
        bool Points(const FValue& V, TArray<FWorldScenePoint>& Out) { return Array(V, Out, Point); }
        bool Bounds(const FValue& V, FWorldNavigationObstacle& Out)
        {
            const auto O = Object(V);
            return Only(O, {TEXT("minX"), TEXT("maxX"), TEXT("minZ"), TEXT("maxZ"), TEXT("yaw"), TEXT("polygon")})
                && Number(O, TEXT("minX"), Out.MinX) && Number(O, TEXT("maxX"), Out.MaxX)
                && Number(O, TEXT("minZ"), Out.MinZ) && Number(O, TEXT("maxZ"), Out.MaxZ)
                && Optional(O, TEXT("yaw"), Out.YawRadians, false, [](const FValue& A, double& N) { return Number(A, N); })
                && Optional(O, TEXT("polygon"), Out.Polygon, false,
                    [](const FValue& A, TArray<FWorldNavigationVertex>& Vertices)
                    {
                        return Array(A, Vertices, [](const FValue& P, FWorldNavigationVertex& Vertex)
                        {
                            return P.IsValid() && P->Type == EJson::Array && P->AsArray().Num() == 2
                                && Number(P->AsArray()[0], Vertex.X) && Number(P->AsArray()[1], Vertex.Z);
                        });
                    });
        }
        bool Navigation(const FValue& V, FWorldNavigation& Out)
        {
            const auto O = Object(V);
            return Only(O, {TEXT("revision"), TEXT("bounds"), TEXT("obstacles"), TEXT("radius"), TEXT("floorY"), TEXT("gridSize")})
                && String(Field(O, TEXT("revision")), Out.Revision) && Bounds(Field(O, TEXT("bounds")), Out.Bounds)
                && Array(Field(O, TEXT("obstacles")), Out.Obstacles, Bounds, 16384)
                && Number(O, TEXT("radius"), Out.Radius) && Number(O, TEXT("floorY"), Out.FloorY)
                && Optional(O, TEXT("gridSize"), Out.GridSize, false, [](const FValue& A, double& N) { return Number(A, N); });
        }
        bool Robbery(const FValue& V, FCombatRobberyView& Out)
        {
            const auto O = Object(V);
            return Only(O, {TEXT("id"), TEXT("toutId"), TEXT("phase"), TEXT("unitKey"), TEXT("amountCap"), TEXT("createdAt"),
                    TEXT("respondsAt"), TEXT("expiresAt"), TEXT("origin"), TEXT("escapeSince"), TEXT("resolvedAt"),
                    TEXT("surrenderedAmount"), TEXT("revision"), TEXT("purpose")})
                && String(Field(O, TEXT("id")), Out.Id) && String(Field(O, TEXT("toutId")), Out.ToutId)
                && Enum(Field(O, TEXT("phase")), Out.Phase, {
                    {TEXT("demanding"), ECombatRobberyPhase::Demanding}, {TEXT("fleeing"), ECombatRobberyPhase::Fleeing},
                    {TEXT("defending"), ECombatRobberyPhase::Defending}, {TEXT("surrendered"), ECombatRobberyPhase::Surrendered},
                    {TEXT("escaped"), ECombatRobberyPhase::Escaped}, {TEXT("repelled"), ECombatRobberyPhase::Repelled},
                    {TEXT("defeated"), ECombatRobberyPhase::Defeated}, {TEXT("interrupted"), ECombatRobberyPhase::Interrupted},
                    {TEXT("expired"), ECombatRobberyPhase::Expired}})
                && Enum(Field(O, TEXT("unitKey")), Out.UnitKey, {{TEXT("lagos-credit"), ECombatCreditUnit::LagosCredit}})
                && Number(O, TEXT("amountCap"), Out.AmountCap) && Number(O, TEXT("createdAt"), Out.CreatedAtMs)
                && Number(O, TEXT("respondsAt"), Out.RespondsAtMs) && Number(O, TEXT("expiresAt"), Out.ExpiresAtMs)
                && Point(Field(O, TEXT("origin")), Out.Origin)
                && Nullable(Field(O, TEXT("escapeSince")), Out.EscapeSinceMs, [](const FValue& A, double& N) { return Number(A, N); })
                && Nullable(Field(O, TEXT("resolvedAt")), Out.ResolvedAtMs, [](const FValue& A, double& N) { return Number(A, N); })
                && Number(O, TEXT("surrenderedAmount"), Out.SurrenderedAmount) && Integer(Field(O, TEXT("revision")), Out.Revision)
                && Optional(O, TEXT("purpose"), Out.Purpose, false, [](const FValue& A, ECombatRobberyPurpose& P)
                    { return Enum(A, P, {{TEXT("recovery"), ECombatRobberyPurpose::Recovery}}); });
        }
        bool Fighter(const FValue& V, FCombatFighterView& Out)
        {
            const auto O = Object(V);
            return Only(O, {TEXT("id"), TEXT("kind"), TEXT("characterId"), TEXT("position"), TEXT("yaw"), TEXT("pitch"),
                    TEXT("health"), TEXT("maxHealth"), TEXT("stamina"), TEXT("ammo"), TEXT("reserveAmmo"), TEXT("reloadAt"),
                    TEXT("status"), TEXT("joinedAt"), TEXT("lastInputSequence"), TEXT("lastAttackAt"), TEXT("protectedUntil"),
                    TEXT("kills"), TEXT("streetWeaponId"), TEXT("targetId"), TEXT("robbery"), TEXT("skillLevels")})
                && String(Field(O, TEXT("id")), Out.Id) && String(Field(O, TEXT("characterId")), Out.CharacterId)
                && Enum(Field(O, TEXT("kind")), Out.Kind, {{TEXT("player"), ECombatFighterKind::Player}, {TEXT("agbero"), ECombatFighterKind::Agbero}})
                && Point(Field(O, TEXT("position")), Out.Position) && Number(O, TEXT("yaw"), Out.YawRadians)
                && Number(O, TEXT("pitch"), Out.PitchRadians) && Number(O, TEXT("health"), Out.Health)
                && Number(O, TEXT("maxHealth"), Out.MaxHealth) && Number(O, TEXT("stamina"), Out.Stamina)
                && Number(O, TEXT("ammo"), Out.Ammo) && Number(O, TEXT("reserveAmmo"), Out.ReserveAmmo)
                && Nullable(Field(O, TEXT("reloadAt")), Out.ReloadAtMs, [](const FValue& A, double& N) { return Number(A, N); })
                && Enum(Field(O, TEXT("status")), Out.Status, {{TEXT("active"), ECombatFighterStatus::Active},
                    {TEXT("defeated"), ECombatFighterStatus::Defeated}, {TEXT("left"), ECombatFighterStatus::Left}})
                && Number(O, TEXT("joinedAt"), Out.JoinedAtMs) && Integer(Field(O, TEXT("lastInputSequence")), Out.LastInputSequence, -1)
                && Number(O, TEXT("lastAttackAt"), Out.LastAttackAtMs) && Number(O, TEXT("protectedUntil"), Out.ProtectedUntilMs)
                && Number(O, TEXT("kills"), Out.Kills)
                && Optional(O, TEXT("streetWeaponId"), Out.StreetWeaponId, true, [](const FValue& A, ECombatStreetWeapon& Weapon)
                    {
                        return Enum(A, Weapon, {{TEXT("bat"), ECombatStreetWeapon::Bat}, {TEXT("pistol"), ECombatStreetWeapon::Pistol},
                            {TEXT("revolver"), ECombatStreetWeapon::Revolver}, {TEXT("smg"), ECombatStreetWeapon::Smg},
                            {TEXT("shotgun"), ECombatStreetWeapon::Shotgun}, {TEXT("rifle"), ECombatStreetWeapon::Rifle}});
                    })
                && Optional(O, TEXT("targetId"), Out.TargetId, false, String)
                && Optional(O, TEXT("robbery"), Out.Robbery, true, Robbery)
                && Optional(O, TEXT("skillLevels"), Out.SkillLevels, false, [](const FValue& A, FCombatSkillLevels& Skills)
                    {
                        const auto S = Object(A);
                        return Only(S, {TEXT("fighting"), TEXT("selfDefence"), TEXT("fitness")})
                            && Number(S, TEXT("fighting"), Skills.Fighting) && Number(S, TEXT("selfDefence"), Skills.SelfDefence)
                            && Number(S, TEXT("fitness"), Skills.Fitness);
                    });
        }
        bool Event(const FValue& V, FCombatEventView& Out)
        {
            const auto O = Object(V);
            return Only(O, {TEXT("id"), TEXT("kind"), TEXT("at"), TEXT("actorId"), TEXT("targetId"), TEXT("shotPoint"), TEXT("blocked")})
                && Integer(Field(O, TEXT("id")), Out.Id)
                && Enum(Field(O, TEXT("kind")), Out.Kind, {{TEXT("attack"), ECombatEventKind::Attack}, {TEXT("hit"), ECombatEventKind::Hit},
                    {TEXT("defeat"), ECombatEventKind::Defeat}, {TEXT("reload"), ECombatEventKind::Reload},
                    {TEXT("finish"), ECombatEventKind::Finish}, {TEXT("left"), ECombatEventKind::Left}})
                && Number(O, TEXT("at"), Out.AtMs) && String(Field(O, TEXT("actorId")), Out.ActorId)
                && Nullable(Field(O, TEXT("targetId")), Out.TargetId, String)
                && Optional(O, TEXT("shotPoint"), Out.ShotPoint, false, Point)
                && Optional(O, TEXT("blocked"), Out.Blocked, false, Boolean);
        }
        bool Officer(const FValue& V, FCombatGunfireOfficer& Out)
        {
            const auto O = Object(V);
            return Only(O, {TEXT("id"), TEXT("from"), TEXT("route"), TEXT("returnRoute"), TEXT("responseAt"), TEXT("arrivesAt"),
                    TEXT("patrolOffsetMs"), TEXT("redirectedAt")})
                && String(Field(O, TEXT("id")), Out.Id) && Point(Field(O, TEXT("from")), Out.From)
                && Points(Field(O, TEXT("route")), Out.Route) && Points(Field(O, TEXT("returnRoute")), Out.ReturnRoute)
                && Number(O, TEXT("responseAt"), Out.ResponseAtMs) && Number(O, TEXT("arrivesAt"), Out.ArrivesAtMs)
                && Number(O, TEXT("patrolOffsetMs"), Out.PatrolOffsetMs)
                && Optional(O, TEXT("redirectedAt"), Out.RedirectedAtMs, false, [](const FValue& A, double& N) { return Number(A, N); });
        }
        bool Incident(const FValue& V, FCombatGunfireIncident& Out)
        {
            const auto O = Object(V);
            return Only(O, {TEXT("id"), TEXT("actorId"), TEXT("lifeId"), TEXT("point"), TEXT("startedAt"), TEXT("lastShotAt"), TEXT("officers")})
                && String(Field(O, TEXT("id")), Out.Id) && String(Field(O, TEXT("actorId")), Out.ActorId)
                && Optional(O, TEXT("lifeId"), Out.LifeId, false, String) && Point(Field(O, TEXT("point")), Out.Point)
                && Number(O, TEXT("startedAt"), Out.StartedAtMs) && Number(O, TEXT("lastShotAt"), Out.LastShotAtMs)
                && Array(Field(O, TEXT("officers")), Out.Officers, Officer);
        }
        bool Gunfire(const FValue& V, FCombatGunfireView& Out)
        {
            const auto O = Object(V);
            return Only(O, {TEXT("patrolOffsets"), TEXT("incidents")})
                && Array(Field(O, TEXT("incidents")), Out.Incidents, Incident)
                && Array(Field(O, TEXT("patrolOffsets")), Out.PatrolOffsets, [](const FValue& A, FCombatPatrolOffset& Offset)
                    {
                        const auto P = Object(A);
                        return Only(P, {TEXT("id"), TEXT("milliseconds")}) && String(Field(P, TEXT("id")), Offset.Id)
                            && Number(P, TEXT("milliseconds"), Offset.Milliseconds);
                    });
        }
        bool Tuning(const FValue& V, FCombatTuningView& Out)
        {
            const auto O = Object(V);
            return Only(O, {TEXT("health"), TEXT("stamina"), TEXT("speed"), TEXT("meleeDamage"), TEXT("meleeRange"), TEXT("meleeCooldownMs"),
                    TEXT("bulletDamage"), TEXT("bulletRange"), TEXT("fireCooldownMs"), TEXT("magazine"), TEXT("reserveAmmo"), TEXT("reloadMs"),
                    TEXT("npcSpeed"), TEXT("npcWindupMs"), TEXT("blockMultiplier"), TEXT("spawnProtectionMs"), TEXT("durationMs"), TEXT("target")})
                && Number(O, TEXT("health"), Out.Health) && Number(O, TEXT("stamina"), Out.Stamina) && Number(O, TEXT("speed"), Out.Speed)
                && Number(O, TEXT("meleeDamage"), Out.MeleeDamage) && Number(O, TEXT("meleeRange"), Out.MeleeRange)
                && Number(O, TEXT("meleeCooldownMs"), Out.MeleeCooldownMs) && Number(O, TEXT("bulletDamage"), Out.BulletDamage)
                && Number(O, TEXT("bulletRange"), Out.BulletRange) && Number(O, TEXT("fireCooldownMs"), Out.FireCooldownMs)
                && Number(O, TEXT("magazine"), Out.Magazine) && Number(O, TEXT("reserveAmmo"), Out.ReserveAmmo)
                && Number(O, TEXT("reloadMs"), Out.ReloadMs) && Number(O, TEXT("npcSpeed"), Out.NpcSpeed)
                && Number(O, TEXT("npcWindupMs"), Out.NpcWindupMs) && Number(O, TEXT("blockMultiplier"), Out.BlockMultiplier)
                && Number(O, TEXT("spawnProtectionMs"), Out.SpawnProtectionMs) && Number(O, TEXT("durationMs"), Out.DurationMs)
                && Number(O, TEXT("target"), Out.Target);
        }
        bool Match(const FValue& V, FCombatMatchView& Out)
        {
            const auto O = Object(V), Objective = Object(Field(O, TEXT("objective")));
            return Only(O, {TEXT("gunfire"), TEXT("missionRunId"), TEXT("id"), TEXT("instanceId"), TEXT("placeId"), TEXT("layoutId"),
                    TEXT("callSessionId"), TEXT("mode"), TEXT("phase"), TEXT("revision"), TEXT("simulatedAt"), TEXT("endsAt"),
                    TEXT("navigation"), TEXT("spawns"), TEXT("fighters"), TEXT("objective"), TEXT("events"), TEXT("tuning")})
                && String(Field(O, TEXT("id")), Out.Id) && !Out.Id.IsEmpty()
                && String(Field(O, TEXT("instanceId")), Out.InstanceId) && !Out.InstanceId.IsEmpty()
                && String(Field(O, TEXT("placeId")), Out.PlaceId) && String(Field(O, TEXT("layoutId")), Out.LayoutId)
                && Nullable(Field(O, TEXT("callSessionId")), Out.CallSessionId, String)
                && Optional(O, TEXT("missionRunId"), Out.MissionRunId, false, String)
                && Enum(Field(O, TEXT("mode")), Out.Mode, {{TEXT("melee"), ECombatMode::Melee}, {TEXT("arena"), ECombatMode::Arena}})
                && Enum(Field(O, TEXT("phase")), Out.Phase, {{TEXT("playing"), ECombatPhase::Playing},
                    {TEXT("finished"), ECombatPhase::Finished}, {TEXT("cancelled"), ECombatPhase::Cancelled}})
                && Integer(Field(O, TEXT("revision")), Out.Revision) && Number(O, TEXT("simulatedAt"), Out.SimulatedAtMs)
                && Number(O, TEXT("endsAt"), Out.EndsAtMs) && Navigation(Field(O, TEXT("navigation")), Out.Navigation)
                && Points(Field(O, TEXT("spawns")), Out.Spawns) && Array(Field(O, TEXT("fighters")), Out.Fighters, Fighter)
                && Only(Objective, {TEXT("target"), TEXT("winnerId")}) && Number(Objective, TEXT("target"), Out.ObjectiveTarget)
                && Nullable(Field(Objective, TEXT("winnerId")), Out.ObjectiveWinnerId, String)
                && Array(Field(O, TEXT("events")), Out.Events, Event) && Tuning(Field(O, TEXT("tuning")), Out.Tuning)
                && Optional(O, TEXT("gunfire"), Out.Gunfire, false, Gunfire);
        }
    }

    bool DecodeCombatResponse(const FString& Json, FCombatDecodedResponse& Out, FString& OutError)
    {
        using namespace CombatStatePrivate;
        Out = {}; OutError = TEXT("Combat response is unsupported or malformed.");
        FObject O;
        FCombatDecodedResponse Next;
        if (!Parse(Json, O, ParseLimits) || !Only(O, {TEXT("state"), TEXT("playerId"), TEXT("lifeId"), TEXT("serverTime")})
            || !String(Field(O, TEXT("playerId")), Next.PlayerId) || Next.PlayerId.IsEmpty()
            || !Nullable(Field(O, TEXT("lifeId")), Next.LifeId, String)
            || (Next.LifeId.IsSet() && Next.LifeId.GetValue().IsEmpty())) return false;
        const auto State = Field(O, TEXT("state")), ServerTime = Field(O, TEXT("serverTime"));
        if (!State.IsValid()) return false;
        if (State->Type != EJson::Null)
        {
            FCombatMatchView Candidate;
            if (!Match(State, Candidate)) return false;
            // The backend deliberately redacts every other fighter's robbery.
            for (const auto& Fighter : Candidate.Fighters)
                if (!Same(Fighter.Id, Next.PlayerId) && Fighter.Robbery.Presence == EWorldFieldPresence::Value) return false;
            Next.Match = MakeShared<FCombatMatchView>(MoveTemp(Candidate));
            Next.bHasMatch = true;
        }
        if (ServerTime.IsValid())
        {
            double N = 0;
            if (!Number(ServerTime, N) || !ValidTime(N)) return false;
            Next.ServerTimeMs = N;
        }
        Next.bValid = true; Out = MoveTemp(Next); OutError.Reset(); return true;
    }

    void FCombatReadState::Select(const FString& InstanceId, const FString& ExpectedPlayerId)
    {
        using namespace CombatStatePrivate;
        if (bDisposed || (Same(View.InstanceId, InstanceId) && Same(View.ExpectedPlayerId, ExpectedPlayerId))) return;
        const FString NextInstance = InstanceId, NextPlayer = ExpectedPlayerId;
        View = {}; View.InstanceId = NextInstance; View.ExpectedPlayerId = NextPlayer;
        Generation = FGuid::NewGuid(); ScopeGeneration = FGuid::NewGuid(); InputGeneration = FGuid::NewGuid();
        ActiveCommand.Invalidate(); bSubscribed = false;
        Changed.Broadcast();
    }
    FCombatReceipt FCombatReadState::Capture(ECombatReceiptKind Kind, double BeganAtMs) const
    {
        FCombatReceipt Receipt;
        if (!bSubscribed || bDisposed) return Receipt;
        Receipt.Generation = Generation; Receipt.ScopeGeneration = ScopeGeneration; Receipt.InputGeneration = InputGeneration;
        Receipt.Kind = Kind; Receipt.MatchId = CombatStatePrivate::MatchId(View.State); Receipt.LifeId = View.LifeId.Value;
        Receipt.BeganAtMs = BeganAtMs; return Receipt;
    }
    FCombatReceipt FCombatReadState::BeginSubscription()
    {
        if (bDisposed || View.InstanceId.IsEmpty() || View.ExpectedPlayerId.IsEmpty()) return {};
        Generation = FGuid::NewGuid(); InputGeneration = FGuid::NewGuid(); ActiveCommand.Invalidate();
        bSubscribed = true; View.bBusy = false;
        const auto Receipt = Capture(ECombatReceiptKind::Subscription);
        Changed.Broadcast(); return Receipt;
    }
    void FCombatReadState::EndSubscription()
    {
        if (bDisposed) return;
        Generation = FGuid::NewGuid(); InputGeneration = FGuid::NewGuid(); ActiveCommand.Invalidate();
        bSubscribed = false; View.bBusy = false; Changed.Broadcast();
    }
    TOptional<FCombatReceipt> FCombatReadState::CaptureCommand(double BeganAtMs)
    {
        if (!bSubscribed || bDisposed || View.bBusy || !CombatStatePrivate::ValidTime(BeganAtMs)) return {};
        InputGeneration = FGuid::NewGuid(); ActiveCommand = FGuid::NewGuid();
        auto Receipt = Capture(ECombatReceiptKind::Command, BeganAtMs); Receipt.CommandId = ActiveCommand;
        View.bBusy = true; View.Error.Reset(); Changed.Broadcast(); return Receipt;
    }
    TOptional<FCombatReceipt> FCombatReadState::CaptureInput(double BeganAtMs, bool bNeutralRelease) const
    {
        if (!bSubscribed || bDisposed || !View.State.IsValid() || !View.LifeId.Value.IsSet()
            || !CombatStatePrivate::ValidTime(BeganAtMs)) return {};
        return Capture(bNeutralRelease ? ECombatReceiptKind::NeutralRelease : ECombatReceiptKind::Input, BeganAtMs);
    }
    void FCombatReadState::InvalidateInputReceipts()
    {
        if (!bDisposed) InputGeneration = FGuid::NewGuid();
    }
    bool FCombatReadState::Current(const FCombatReceipt& Receipt) const
    {
        return !bDisposed && bSubscribed && Receipt.Generation.IsValid() && Receipt.Generation == Generation
            && (Receipt.Kind != ECombatReceiptKind::Command || (ActiveCommand.IsValid() && Receipt.CommandId == ActiveCommand));
    }
    bool FCombatReadState::CurrentInput(const FCombatReceipt& Receipt) const
    {
        using namespace CombatStatePrivate;
        return Receipt.InputGeneration == InputGeneration && Same(Receipt.MatchId, MatchId(View.State))
            && Same(Receipt.LifeId, View.LifeId.Value);
    }
    void FCombatReadState::FinishCommand(const FCombatReceipt& Receipt)
    {
        if (Receipt.Kind != ECombatReceiptKind::Command) return;
        ActiveCommand.Invalidate(); View.bBusy = false;
    }
    ECombatReadResult FCombatReadState::Accept(const FCombatReceipt& Receipt, const FCombatDecodedResponse& Response, double ReceivedAtMs)
    {
        using namespace CombatStatePrivate;
        if (!Current(Receipt)) return ECombatReadResult::StaleReceipt;
        const bool bInput = Receipt.Kind == ECombatReceiptKind::Input || Receipt.Kind == ECombatReceiptKind::NeutralRelease;
        if (bInput && !CurrentInput(Receipt)) return ECombatReadResult::StaleReceipt;
        const bool bMutation = Receipt.Kind != ECombatReceiptKind::Subscription;
        if (!Response.IsValid() || (bMutation && (!Response.GetServerTimeMs().IsSet() || !ValidTime(ReceivedAtMs))))
        {
            AcceptFailure(Receipt); return ECombatReadResult::InvalidResponse;
        }
        const auto& Match = Response.GetMatch();
        if (!Same(Response.GetPlayerId(), View.ExpectedPlayerId) || (Match.IsValid() && !Same(Match->InstanceId, View.InstanceId)))
        {
            AcceptFailure(Receipt); return ECombatReadResult::IdentityMismatch;
        }
        if (Receipt.Kind == ECombatReceiptKind::Command && Receipt.ScopeGeneration != ScopeGeneration
            && (!Same(MatchId(Match), MatchId(View.State)) || !Same(Response.GetLifeId(), View.LifeId.Value)))
        {
            FinishCommand(Receipt); Changed.Broadcast(); return ECombatReadResult::StaleReceipt;
        }
        // Canonical mutation timing is recorded before the same-match revision check.
        if (bMutation) View.ServerOffsetMs = Response.GetServerTimeMs().GetValue() - (Receipt.BeganAtMs / 2 + ReceivedAtMs / 2);
        FinishCommand(Receipt);
        if (View.State.IsValid() && Match.IsValid() && Same(View.State->Id, Match->Id) && Match->Revision < View.State->Revision)
        {
            if (bMutation) Changed.Broadcast();
            return ECombatReadResult::OlderRevision;
        }
        if (!Same(MatchId(Match), MatchId(View.State)) || !Same(Response.GetLifeId(), View.LifeId.Value))
        {
            ScopeGeneration = FGuid::NewGuid(); InputGeneration = FGuid::NewGuid();
        }
        View.State = Match; View.StatePresence = Match.IsValid() ? EWorldFieldPresence::Value : EWorldFieldPresence::Null;
        View.PlayerId = Response.GetPlayerId(); View.LifeId.Value = Response.GetLifeId();
        View.LifeId.Presence = View.LifeId.Value.IsSet() ? EWorldFieldPresence::Value : EWorldFieldPresence::Null;
        View.bReady = true; View.Error.Reset(); Changed.Broadcast(); return ECombatReadResult::Accepted;
    }
    bool FCombatReadState::AcceptFailure(const FCombatReceipt& Receipt)
    {
        if (!Current(Receipt)) return false;
        if (Receipt.Kind == ECombatReceiptKind::Input || Receipt.Kind == ECombatReceiptKind::NeutralRelease)
        {
            if (!CurrentInput(Receipt) || Receipt.Kind == ECombatReceiptKind::NeutralRelease) return false;
            View.Error = TEXT("That combat action failed.");
        }
        else if (Receipt.Kind == ECombatReceiptKind::Subscription)
        {
            View.bReady = true; View.Error = TEXT("Combat could not connect. Try again.");
        }
        else
        {
            if (Receipt.ScopeGeneration == ScopeGeneration) View.Error = TEXT("Combat could not connect. Try again.");
            FinishCommand(Receipt);
        }
        Changed.Broadcast(); return true;
    }
    void FCombatReadState::Clear()
    {
        if (bDisposed) return;
        View = {}; Generation = FGuid::NewGuid(); ScopeGeneration = FGuid::NewGuid(); InputGeneration = FGuid::NewGuid();
        ActiveCommand.Invalidate(); bSubscribed = false; Changed.Broadcast();
    }
    void FCombatReadState::Dispose()
    {
        if (bDisposed) return;
        bDisposed = true; bSubscribed = false; View = {};
        Generation.Invalidate(); ScopeGeneration.Invalidate(); InputGeneration.Invalidate(); ActiveCommand.Invalidate();
        Changed.Broadcast(); Changed.Clear();
    }
}
