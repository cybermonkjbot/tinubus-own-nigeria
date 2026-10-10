#include "BaddieCompanionCombatState.h"
#include "BaddieCompanionStateJson.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

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
        constexpr int64 MaxSafeSequence = 9007199254740991LL;
        constexpr double InputPi = 3.14159265358979323846;
        bool ValidInput(const FCombatInputValue& Input)
        {
            return Input.Sequence >= 0 && Input.Sequence < MaxSafeSequence
                && FMath::IsFinite(Input.MoveRight) && FMath::Abs(Input.MoveRight) <= 1
                && FMath::IsFinite(Input.MoveForward) && FMath::Abs(Input.MoveForward) <= 1
                && FMath::IsFinite(Input.YawRadians) && FMath::Abs(Input.YawRadians) <= InputPi * 2
                && FMath::IsFinite(Input.PitchRadians) && FMath::Abs(Input.PitchRadians) <= InputPi / 2 - .05;
        }
        bool Neutral(const FCombatInputValue& Input)
        {
            return Input.MoveRight == 0 && Input.MoveForward == 0 && !Input.bAttack && !Input.bBlock && !Input.bReload;
        }
        FCombatInputValue Idle(int64 Sequence, const FCombatInputValue& Prior)
        {
            FCombatInputValue Input;
            Input.Sequence = Sequence; Input.YawRadians = Prior.YawRadians; Input.PitchRadians = Prior.PitchRadians;
            return Input;
        }
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
        StopInputs(); ResetSelectedInput();
        View = {}; View.InstanceId = NextInstance; View.ExpectedPlayerId = NextPlayer;
        Generation = FGuid::NewGuid(); ScopeGeneration = FGuid::NewGuid(); InputGeneration = FGuid::NewGuid();
        RetireCommand(); bSubscribed = false;
        Changed.Broadcast();
    }
    FCombatReceipt FCombatReadState::Capture(ECombatReceiptKind Kind, double BeganAtMs) const
    {
        FCombatReceipt Receipt;
        if (!bSubscribed || bDisposed) return Receipt;
        Receipt.Generation = Generation; Receipt.ScopeGeneration = ScopeGeneration; Receipt.InputGeneration = InputGeneration;
        Receipt.InstanceId = View.InstanceId; Receipt.ExpectedPlayerId = View.ExpectedPlayerId;
        Receipt.Kind = Kind; Receipt.MatchId = CombatStatePrivate::MatchId(View.State); Receipt.LifeId = View.LifeId.Value;
        Receipt.BeganAtMs = BeganAtMs; return Receipt;
    }
    FCombatReceipt FCombatReadState::BeginSubscription()
    {
        if (bDisposed || View.InstanceId.IsEmpty() || View.ExpectedPlayerId.IsEmpty()) return {};
        StopInputs();
        Generation = FGuid::NewGuid(); InputGeneration = FGuid::NewGuid(); RetireCommand();
        bSubscribed = true; View.bBusy = false;
        const auto Receipt = Capture(ECombatReceiptKind::Subscription);
        Changed.Broadcast(); return Receipt;
    }
    void FCombatReadState::EndSubscription()
    {
        if (bDisposed) return;
        StopInputs();
        Generation = FGuid::NewGuid(); InputGeneration = FGuid::NewGuid(); RetireCommand();
        bSubscribed = false; View.bBusy = false; Changed.Broadcast();
    }
    TOptional<FCombatReceipt> FCombatReadState::CaptureCommand(double BeganAtMs)
    {
        if (!CanStartCommand(BeganAtMs)) return {};
        FCombatRequest Request; Request.Receipt = Capture(ECombatReceiptKind::Command, BeganAtMs);
        Request.Receipt.RequestId = FGuid::NewGuid();
        return StartCommand(Request) ? TOptional<FCombatReceipt>(Request.Receipt) : TOptional<FCombatReceipt>();
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
    bool FCombatReadState::CanStartCommand(double BeganAtMs) const
    {
        return bSubscribed && !bDisposed && !View.bBusy && !View.InstanceId.IsEmpty()
            && !View.ExpectedPlayerId.IsEmpty() && CombatStatePrivate::ValidTime(BeganAtMs);
    }
    bool FCombatReadState::StartCommand(const FCombatRequest& Request)
    {
        // Request values and response scope were captured before this stop boundary.
        StopInputs();
        ActiveCommand = Request.Receipt.RequestId; PreparedCommand.Reset(); bCommandDispatched = false;
        if (Request.Receipt.bOwned) PreparedCommand = Request;
        View.bBusy = true; View.Error.Reset(); Changed.Broadcast();
        return Current(Request.Receipt);
    }
    const FCombatFighterView* FCombatReadState::DisplayedFighter() const
    {
        if (!View.State.IsValid() || !View.PlayerId.IsSet()) return nullptr;
        for (const auto& Fighter : View.State->Fighters)
            if (CombatStatePrivate::Same(Fighter.Id, View.PlayerId.GetValue())) return &Fighter;
        return nullptr;
    }
    TOptional<FCombatRequest> FCombatReadState::PrepareJoin(double BeganAtMs)
    {
        return PrepareCommand(TEXT("companionCombat:join"), BeganAtMs, {});
    }
    TOptional<FCombatRequest> FCombatReadState::PrepareLeave(double BeganAtMs)
    {
        return PrepareCommand(TEXT("companionCombat:leave"), BeganAtMs, {});
    }
    TOptional<FCombatRequest> FCombatReadState::PrepareRobberyAction(const FString& Action, double BeganAtMs)
    {
        if (!CombatStatePrivate::Same(Action, TEXT("defend")) && !CombatStatePrivate::Same(Action, TEXT("flee"))) return {};
        return PrepareCommand(TEXT("companionCombat:robberyAction"), BeganAtMs, Action);
    }
    TOptional<FCombatRequest> FCombatReadState::PrepareCommand(const TCHAR* Endpoint, double BeganAtMs, const FString& Action)
    {
        using namespace CombatStatePrivate;
        if (!CanStartCommand(BeganAtMs)) return {};
        const bool bJoin = Same(Endpoint, TEXT("companionCombat:join")), bRobbery = !Action.IsEmpty();
        if (!bJoin && (!View.State.IsValid() || !View.LifeId.Value.IsSet())) return {};
        const auto* Fighter = DisplayedFighter();
        if (bRobbery && (!Fighter || !Fighter->Robbery.Value.IsSet())) return {};
        FCombatRequest Request;
        Request.Receipt = Capture(ECombatReceiptKind::Command, BeganAtMs);
        Request.Receipt.bOwned = true; Request.Receipt.RequestId = FGuid::NewGuid(); Request.Endpoint = Endpoint;
        const auto Args = MakeShared<FJsonObject>();
        Args->SetStringField(TEXT("instanceId"), Request.Receipt.InstanceId);
        if (Request.Receipt.LifeId.IsSet()) Args->SetStringField(TEXT("lifeId"), Request.Receipt.LifeId.GetValue());
        if (!bJoin) Args->SetStringField(TEXT("matchId"), Request.Receipt.MatchId.GetValue());
        if (bRobbery)
        {
            const auto& Robbery = Fighter->Robbery.Value.GetValue();
            Args->SetStringField(TEXT("encounterId"), Robbery.Id);
            Args->SetNumberField(TEXT("revision"), static_cast<double>(Robbery.Revision));
            Args->SetStringField(TEXT("action"), Action);
        }
        if (!FJsonSerializer::Serialize(Args, TJsonWriterFactory<>::Create(&Request.ArgumentsJson))) return {};
        return StartCommand(Request) ? TOptional<FCombatRequest>(Request) : TOptional<FCombatRequest>();
    }
    bool FCombatReadState::EncodeInput(FInputWork& Work) const
    {
        const auto& Receipt = Work.Request.Receipt;
        if (!Receipt.MatchId.IsSet() || !Receipt.LifeId.IsSet() || !CombatStatePrivate::ValidInput(Work.Input)) return false;
        const auto Args = MakeShared<FJsonObject>(), Input = MakeShared<FJsonObject>();
        Args->SetStringField(TEXT("instanceId"), Receipt.InstanceId);
        Args->SetStringField(TEXT("matchId"), Receipt.MatchId.GetValue());
        Args->SetStringField(TEXT("lifeId"), Receipt.LifeId.GetValue());
        Input->SetNumberField(TEXT("sequence"), static_cast<double>(Work.Input.Sequence));
        TArray<TSharedPtr<FJsonValue>> Move;
        Move.Add(MakeShared<FJsonValueNumber>(Work.Input.MoveRight));
        Move.Add(MakeShared<FJsonValueNumber>(Work.Input.MoveForward));
        Input->SetArrayField(TEXT("move"), Move);
        Input->SetNumberField(TEXT("yaw"), Work.Input.YawRadians); Input->SetNumberField(TEXT("pitch"), Work.Input.PitchRadians);
        Input->SetBoolField(TEXT("attack"), Work.Input.bAttack); Input->SetBoolField(TEXT("block"), Work.Input.bBlock);
        Input->SetBoolField(TEXT("reload"), Work.Input.bReload); Args->SetObjectField(TEXT("input"), Input);
        Work.Request.Endpoint = TEXT("companionCombat:input"); Work.Request.ArgumentsJson.Reset();
        return FJsonSerializer::Serialize(Args, TJsonWriterFactory<>::Create(&Work.Request.ArgumentsJson));
    }
    TOptional<FCombatRequest> FCombatReadState::PrepareInput(const FCombatInputValue& Input, double BeganAtMs)
    {
        using namespace CombatStatePrivate;
        if (bDisposed || !bSubscribed || HasInputWork() || View.bBusy || View.InstanceId.IsEmpty()
            || !View.State.IsValid() || !View.LifeId.Value.IsSet() || View.State->Phase != ECombatPhase::Playing
            || !ValidInput(Input) || !ValidTime(BeganAtMs) || (LastSendMs.IsSet() && BeganAtMs - LastSendMs.GetValue() < 200)) return {};
        const auto* Fighter = DisplayedFighter();
        FInputWork Work; Work.Input = Input;
        Work.Input.Sequence = FMath::Max(Input.Sequence, FMath::Max(LastSequence + 1, Fighter ? Fighter->LastInputSequence + 1 : int64(0)));
        if (Work.Input.Sequence >= MaxSafeSequence - 1) return {};
        Work.Request.Receipt = Capture(ECombatReceiptKind::Input, BeganAtMs);
        Work.Request.Receipt.bOwned = true; Work.Request.Receipt.RequestId = FGuid::NewGuid();
        if (!EncodeInput(Work)) return {};
        InputFlight = Work;
        return Work.Request;
    }
    void FCombatReadState::ResetSelectedInput()
    {
        LastSequence = -1; LastInput.Reset(); LastSendMs.Reset();
    }
    void FCombatReadState::StopInputs()
    {
        using namespace CombatStatePrivate;
        if (bDisposed) return;
        InputGeneration = FGuid::NewGuid();
        // Unsent gameplay has no server-side controls to release. Retain prior sent input.
        if (InputFlight.IsSet() && !InputFlight->bDispatched && InputFlight->Request.Receipt.Kind == ECombatReceiptKind::Input)
            InputFlight.Reset();
        if (InputFlight.IsSet())
        {
            if (!Neutral(InputFlight->Input) && !PendingStop.IsSet())
            {
                FInputWork Stop;
                Stop.Request.Receipt = InputFlight->Request.Receipt;
                Stop.Request.Receipt.Kind = ECombatReceiptKind::NeutralRelease;
                Stop.Request.Receipt.RequestId = FGuid::NewGuid(); Stop.Request.Receipt.BeganAtMs = 0;
                Stop.Input = Idle(InputFlight->Input.Sequence + 1, InputFlight->Input);
                PendingStop = Stop;
                const auto& Scope = Stop.Request.Receipt;
                if (Same(Scope.InstanceId, View.InstanceId) && Same(Scope.ExpectedPlayerId, View.ExpectedPlayerId)
                    && Same(Scope.MatchId, MatchId(View.State)) && Same(Scope.LifeId, View.LifeId.Value))
                    LastSequence = FMath::Max(LastSequence, Stop.Input.Sequence);
            }
            LastInput.Reset(); return;
        }
        if (PendingStop.IsSet()) return;
        const auto* Fighter = DisplayedFighter();
        if (!Fighter || Fighter->Status != ECombatFighterStatus::Active || View.InstanceId.IsEmpty()
            || !View.State.IsValid() || !View.LifeId.Value.IsSet() || !LastInput.IsSet() || Neutral(LastInput.GetValue())) return;
        FInputWork Stop;
        Stop.Input = Idle(FMath::Max(LastSequence + 1, Fighter->LastInputSequence + 1), LastInput.GetValue());
        if (!ValidInput(Stop.Input)) return;
        Stop.Request.Receipt = Capture(ECombatReceiptKind::NeutralRelease);
        if (!Stop.Request.Receipt.Generation.IsValid()) return;
        Stop.Request.Receipt.bOwned = true; Stop.Request.Receipt.RequestId = FGuid::NewGuid();
        LastSequence = Stop.Input.Sequence; LastInput = Stop.Input; PendingStop = Stop;
    }
    TOptional<FCombatReceipt> FCombatReadState::GetPendingStop() const
    {
        return PendingStop.IsSet() ? TOptional<FCombatReceipt>(PendingStop->Request.Receipt) : TOptional<FCombatReceipt>();
    }
    TOptional<FCombatRequest> FCombatReadState::PreparePendingStop(double BeganAtMs)
    {
        // This drain deliberately survives Clear/Dispose. It cannot create gameplay input.
        if (InputFlight.IsSet() || !PendingStop.IsSet() || !CombatStatePrivate::ValidTime(BeganAtMs)) return {};
        FInputWork Work = PendingStop.GetValue(); Work.Request.Receipt.BeganAtMs = BeganAtMs;
        Work.Request.Receipt.RequestId = FGuid::NewGuid();
        if (!EncodeInput(Work)) return {};
        InputFlight = Work; PendingStop.Reset();
        return Work.Request;
    }
    bool FCombatReadState::MatchesFlight(const FCombatReceipt& Receipt) const
    {
        return Receipt.bOwned && Receipt.RequestId.IsValid() && InputFlight.IsSet()
            && Receipt.RequestId == InputFlight->Request.Receipt.RequestId;
    }
    TOptional<FCombatRequest> FCombatReadState::DispatchRequest(const FCombatReceipt& Receipt, double DispatchedAtMs)
    {
        using namespace CombatStatePrivate;
        if (!Receipt.bOwned || !ValidTime(DispatchedAtMs)) return {};
        if (Receipt.Kind == ECombatReceiptKind::Command)
        {
            if (!Current(Receipt) || bCommandDispatched) return {};
            // A no-flight stop precedes the command send. An existing sent flight instead
            // drains its queued stop on completion, without delaying the command.
            if ((PendingStop.IsSet() && !InputFlight.IsSet()) || (InputFlight.IsSet() && !InputFlight->bDispatched)) return {};
            PreparedCommand->Receipt.BeganAtMs = DispatchedAtMs;
            bCommandDispatched = true; return PreparedCommand.GetValue();
        }
        if (!MatchesFlight(Receipt) || InputFlight->bDispatched || InputFlight->bCompleting) return {};
        if (Receipt.Kind != ECombatReceiptKind::NeutralRelease
            && (!Current(Receipt) || !CurrentInput(Receipt) || View.bBusy || !View.State.IsValid()
                || View.State->Phase != ECombatPhase::Playing || (LastSendMs.IsSet() && DispatchedAtMs - LastSendMs.GetValue() < 200))) return {};
        InputFlight->Request.Receipt.BeganAtMs = DispatchedAtMs;
        InputFlight->bDispatched = true;
        if (Receipt.Kind == ECombatReceiptKind::Input)
        {
            LastSequence = InputFlight->Input.Sequence; LastSendMs = DispatchedAtMs; LastInput = InputFlight->Input;
        }
        return InputFlight->Request;
    }
    bool FCombatReadState::AbandonRequest(const FCombatReceipt& Receipt)
    {
        if (!Receipt.bOwned || !Receipt.RequestId.IsValid()) return false;
        if (Receipt.Kind == ECombatReceiptKind::Command)
        {
            if (!Current(Receipt) || bCommandDispatched) return false;
            RetireCommand(); Changed.Broadcast(); return true;
        }
        if (MatchesFlight(Receipt))
        {
            if (InputFlight->bDispatched || InputFlight->bCompleting) return false;
            InputFlight.Reset(); return true;
        }
        if (PendingStop.IsSet() && Receipt.RequestId == PendingStop->Request.Receipt.RequestId)
        {
            PendingStop.Reset(); return true;
        }
        return false;
    }
    bool FCombatReadState::Current(const FCombatReceipt& Receipt) const
    {
        return !bDisposed && bSubscribed && Receipt.Generation.IsValid() && Receipt.Generation == Generation
            && CombatStatePrivate::Same(Receipt.InstanceId, View.InstanceId)
            && CombatStatePrivate::Same(Receipt.ExpectedPlayerId, View.ExpectedPlayerId)
            && (Receipt.Kind != ECombatReceiptKind::Command || (ActiveCommand.IsValid() && Receipt.RequestId == ActiveCommand
                && Receipt.bOwned == PreparedCommand.IsSet()));
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
        RetireCommand();
    }
    void FCombatReadState::RetireCommand()
    {
        ActiveCommand.Invalidate(); PreparedCommand.Reset(); bCommandDispatched = false; View.bBusy = false;
    }
    bool FCombatReadState::CanComplete(const FCombatReceipt& Receipt) const
    {
        if (!Receipt.bOwned) return true;
        if (Receipt.Kind == ECombatReceiptKind::Command) return Current(Receipt) && bCommandDispatched;
        return MatchesFlight(Receipt) && InputFlight->bDispatched && !InputFlight->bCompleting;
    }
    void FCombatReadState::FinishInput(const FCombatReceipt& Receipt)
    {
        // Like submit's finally, release the original flight even if its response is stale.
        // Matching prevents a reentrant continuation from releasing newer work.
        if (MatchesFlight(Receipt) && InputFlight->bCompleting) InputFlight.Reset();
    }
    ECombatReadResult FCombatReadState::Accept(const FCombatReceipt& Receipt, const FCombatDecodedResponse& Response, double ReceivedAtMs)
    {
        if (!CanComplete(Receipt)) return ECombatReadResult::StaleReceipt;
        const bool bFlight = MatchesFlight(Receipt);
        const auto Completion = bFlight ? InputFlight->Request.Receipt
            : Receipt.bOwned ? PreparedCommand->Receipt : Receipt;
        if (bFlight) InputFlight->bCompleting = true;
        const auto Result = ApplyResponse(Completion, Response, ReceivedAtMs);
        if (bFlight) FinishInput(Completion);
        return Result;
    }
    ECombatReadResult FCombatReadState::ApplyResponse(const FCombatReceipt& Receipt, const FCombatDecodedResponse& Response, double ReceivedAtMs)
    {
        using namespace CombatStatePrivate;
        if (!Current(Receipt)) return ECombatReadResult::StaleReceipt;
        const bool bInput = Receipt.Kind == ECombatReceiptKind::Input || Receipt.Kind == ECombatReceiptKind::NeutralRelease;
        if (bInput && !CurrentInput(Receipt)) return ECombatReadResult::StaleReceipt;
        const bool bMutation = Receipt.Kind != ECombatReceiptKind::Subscription;
        if (!Response.IsValid() || (bMutation && (!Response.GetServerTimeMs().IsSet() || !ValidTime(ReceivedAtMs))))
        {
            ApplyFailure(Receipt); return ECombatReadResult::InvalidResponse;
        }
        const auto& Match = Response.GetMatch();
        if (!Same(Response.GetPlayerId(), View.ExpectedPlayerId) || (Match.IsValid() && !Same(Match->InstanceId, View.InstanceId)))
        {
            ApplyFailure(Receipt); return ECombatReadResult::IdentityMismatch;
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
            StopInputs(); ResetSelectedInput();
            ScopeGeneration = FGuid::NewGuid(); InputGeneration = FGuid::NewGuid();
        }
        View.State = Match; View.StatePresence = Match.IsValid() ? EWorldFieldPresence::Value : EWorldFieldPresence::Null;
        View.PlayerId = Response.GetPlayerId(); View.LifeId.Value = Response.GetLifeId();
        View.LifeId.Presence = View.LifeId.Value.IsSet() ? EWorldFieldPresence::Value : EWorldFieldPresence::Null;
        View.bReady = true; View.Error.Reset(); Changed.Broadcast(); return ECombatReadResult::Accepted;
    }
    bool FCombatReadState::AcceptFailure(const FCombatReceipt& Receipt)
    {
        if (!CanComplete(Receipt)) return false;
        const bool bFlight = MatchesFlight(Receipt);
        const auto Completion = bFlight ? InputFlight->Request.Receipt
            : Receipt.bOwned ? PreparedCommand->Receipt : Receipt;
        if (bFlight) InputFlight->bCompleting = true;
        const bool bApplied = ApplyFailure(Completion);
        if (bFlight) FinishInput(Completion);
        return bApplied;
    }
    bool FCombatReadState::ApplyFailure(const FCombatReceipt& Receipt)
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
        StopInputs(); ResetSelectedInput();
        View = {}; Generation = FGuid::NewGuid(); ScopeGeneration = FGuid::NewGuid(); InputGeneration = FGuid::NewGuid();
        RetireCommand(); bSubscribed = false; Changed.Broadcast();
    }
    void FCombatReadState::Dispose()
    {
        if (bDisposed) return;
        StopInputs(); ResetSelectedInput();
        bDisposed = true; bSubscribed = false; View = {};
        Generation.Invalidate(); ScopeGeneration.Invalidate(); InputGeneration.Invalidate(); RetireCommand();
        Changed.Broadcast(); Changed.Clear();
    }
}
