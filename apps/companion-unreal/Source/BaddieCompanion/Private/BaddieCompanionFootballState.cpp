#include "BaddieCompanionFootballState.h"
#include "BaddieCompanionStateJson.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Internationalization/TextComparison.h"
#include "Containers/StringConv.h"
#include <cmath>
#include <limits>

namespace BaddieCompanion
{
    namespace FootballPrivate
    {
        using namespace StateJsonPrivate;
        // Native allocation bounds, not canonical gameplay validation. Match/entry counts
        // come from footballLimits.players and SPACE_MEMBER_LIMIT respectively.
        constexpr FParseLimits Limits { 262144, 24, 8192 };
        constexpr int32 MaxText = 4096, MatchPlayers = 12, EntryPlayers = 50;
        constexpr int64 MaxCapturedNowMs = 4102444800000LL;
        const TCHAR* InvalidMatch = TEXT("Football data couldn't load. Reconnect to continue.");
        const TCHAR* InvalidEntry = TEXT("Football players couldn't load. Reopen the roster to try again.");
        const TCHAR* Disconnected = TEXT("Football disconnected. Reconnect to continue.");
        const TCHAR* CommandFailed = TEXT("Couldn't update the match. Try again.");
        const TCHAR* OpenFailed = TEXT("Couldn't open football. Try again.");
        // Pinned footballLimits. Source milliseconds and metres, not Unreal centimetres.
        constexpr double InputMs = 200, InputRenewMs = 400, TickMs = 200;
        constexpr double PlayerRadius = .42, BallRadius = .16;

        double ClockMs(FDateTime Now) { return static_cast<double>(Now.ToUnixTimestamp()) * 1000 + Now.GetMillisecond(); }
        double Length(const FFootballPoint& P) { return std::hypot(P.X, P.Z); }
        double Distance(const FFootballPoint& A, const FFootballPoint& B) { return Length({ A.X - B.X, A.Z - B.Z }); }
        bool SafeInteger(double Value, int64 Minimum = -9007199254740991LL)
        {
            int64 Out = 0;
            return Integer(MakeShared<FJsonValueNumber>(Value), Out, Minimum);
        }
        bool FinitePoint(const FFootballPoint& P) { return FMath::IsFinite(P.X) && FMath::IsFinite(P.Z); }
        bool Moving(const FFootballInput& Input) { return Input.Destination.IsSet() || Length(Input.Move) > 0; }
        bool ValidInput(const FFootballInput& Input, const FFootballPitch& Pitch)
        {
            const bool bAction = Input.Action == EFootballAction::None || Input.Action == EFootballAction::Pass
                || Input.Action == EFootballAction::Shoot || Input.Action == EFootballAction::Challenge;
            return SafeInteger(Input.Sequence, 1) && FinitePoint(Input.Move) && FinitePoint(Input.Aim)
                && Length(Input.Move) <= 1.01 && Length(Input.Aim) <= 1.01
                && FMath::IsFinite(Input.Power) && Input.Power >= .1 && Input.Power <= 1 && bAction
                && (!Input.TargetPlayerId.IsSet() || Input.TargetPlayerId->Len() <= 160)
                && (!Input.Destination.IsSet() || (FinitePoint(Input.Destination.GetValue())
                    && FMath::Abs(Input.Destination->X - Pitch.Centre.X) <= Pitch.HalfWidth
                    && FMath::Abs(Input.Destination->Z - Pitch.Centre.Z) <= Pitch.HalfLength))
                && (!Input.CancelActions.Get(false) || (Input.Action == EFootballAction::None && !Input.Destination.IsSet()
                    && Length(Input.Move) == 0 && !Input.TargetPlayerId.IsSet()));
        }
        FObject PointJson(const FFootballPoint& P)
        {
            auto O = MakeShared<FJsonObject>(); O->SetNumberField(TEXT("x"), P.X); O->SetNumberField(TEXT("z"), P.Z); return O;
        }
        FObject InputJson(const FFootballInput& Input)
        {
            auto O = MakeShared<FJsonObject>();
            O->SetNumberField(TEXT("sequence"), Input.Sequence);
            O->SetObjectField(TEXT("move"), PointJson(Input.Move)); O->SetObjectField(TEXT("aim"), PointJson(Input.Aim));
            if (Input.Destination.IsSet()) O->SetObjectField(TEXT("destination"), PointJson(Input.Destination.GetValue()));
            else O->SetField(TEXT("destination"), MakeShared<FJsonValueNull>());
            const TCHAR* Action = Input.Action == EFootballAction::Pass ? TEXT("pass") : Input.Action == EFootballAction::Shoot ? TEXT("shoot")
                : Input.Action == EFootballAction::Challenge ? TEXT("challenge") : TEXT("none");
            O->SetStringField(TEXT("action"), Action);
            if (Input.TargetPlayerId.IsSet()) O->SetStringField(TEXT("targetPlayerId"), Input.TargetPlayerId.GetValue());
            else O->SetField(TEXT("targetPlayerId"), MakeShared<FJsonValueNull>());
            O->SetNumberField(TEXT("power"), Input.Power);
            if (Input.CancelActions.IsSet()) O->SetBoolField(TEXT("cancelActions"), Input.CancelActions.GetValue());
            return O;
        }
        bool Json(const FObject& O, FString& Out) { return FJsonSerializer::Serialize(O.ToSharedRef(), TJsonWriterFactory<>::Create(&Out)); }
        bool SameInput(const FFootballInput& A, const FFootballInput& B)
        {
            FString Left, Right; return Json(InputJson(A), Left) && Json(InputJson(B), Right) && Same(Left, Right);
        }
        const FFootballPlayer* CurrentPlayer(const FFootballSnapshot& Snapshot)
        {
            if (!Snapshot.Match.IsSet() || !Snapshot.PlayerId.IsSet()) return nullptr;
            return Snapshot.Match->Players.FindByPredicate([&](const FFootballPlayer& P) { return Same(P.Id, Snapshot.PlayerId.GetValue()); });
        }
        bool SameId(const TOptional<FString>& Id, const FString& Other) { return Id.IsSet() && Same(Id.GetValue(), Other); }
        bool SameNullableId(const TOptional<FString>& A, const TOptional<FString>& B)
        {
            return A.IsSet() == B.IsSet() && (!A.IsSet() || Same(A.GetValue(), B.GetValue()));
        }
        bool ValidEntry(const FFootballEntrySelection& Entry, bool bCheckRequest = true)
        {
            if (Entry.InstanceId.IsEmpty() || Entry.InstanceId.Len() > 160 || !SafeInteger(Entry.Revision, 0) || !SafeInteger(Entry.Epoch, 0)
                || Entry.Players.Num() < 1 || Entry.Players.Num() > MatchPlayers) return false;
            if (bCheckRequest)
            {
                if (Entry.RequestId.Len() < 8 || Entry.RequestId.Len() > 64) return false;
                for (TCHAR C : Entry.RequestId)
                    if (!((C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z') || (C >= '0' && C <= '9') || C == '_' || C == '-')) return false;
            }
            bool bHome = false, bAway = false;
            for (int32 I = 0; I < Entry.Players.Num(); ++I)
            {
                const auto& P = Entry.Players[I];
                if (P.Id.IsEmpty() || P.Id.Len() > 160 || (P.Side != EFootballSide::Home && P.Side != EFootballSide::Away)) return false;
                for (int32 J = 0; J < I; ++J) if (Same(P.Id, Entry.Players[J].Id)) return false;
                bHome |= P.Side == EFootballSide::Home; bAway |= P.Side == EFootballSide::Away;
            }
            switch (Entry.Mode)
            {
            case EFootballMode::Solo: return Entry.Players.Num() == 1 && bHome;
            case EFootballMode::Coop: return Entry.Players.Num() <= MatchPlayers - 4 && !bAway;
            case EFootballMode::Teams: return bHome && bAway;
            default: return false;
            }
        }
        bool SameEntry(const FFootballEntrySelection& A, const FFootballEntrySelection& B)
        {
            if (!Same(A.InstanceId, B.InstanceId) || A.Revision != B.Revision || A.Epoch != B.Epoch || A.Mode != B.Mode || A.Players.Num() != B.Players.Num()) return false;
            for (int32 I = 0; I < A.Players.Num(); ++I)
                if (!Same(A.Players[I].Id, B.Players[I].Id) || A.Players[I].Side != B.Players[I].Side) return false;
            return true; // RequestId deliberately excluded from manual-retry identity.
        }
        FObject EntryJson(const FFootballEntrySelection& Entry, const FString& RoomId)
        {
            auto Args = MakeShared<FJsonObject>();
            Args->SetStringField(TEXT("roomId"), RoomId); Args->SetStringField(TEXT("instanceId"), Entry.InstanceId);
            Args->SetNumberField(TEXT("revision"), Entry.Revision); Args->SetNumberField(TEXT("epoch"), Entry.Epoch);
            Args->SetStringField(TEXT("requestId"), Entry.RequestId);
            Args->SetStringField(TEXT("mode"), Entry.Mode == EFootballMode::Solo ? TEXT("solo") : Entry.Mode == EFootballMode::Teams ? TEXT("teams") : TEXT("coop"));
            TArray<TSharedPtr<FJsonValue>> Players;
            for (const auto& Player : Entry.Players)
            {
                auto P = MakeShared<FJsonObject>(); P->SetStringField(TEXT("id"), Player.Id);
                P->SetStringField(TEXT("side"), Player.Side == EFootballSide::Home ? TEXT("home") : TEXT("away"));
                Players.Add(MakeShared<FJsonValueObject>(P));
            }
            Args->SetArrayField(TEXT("players"), Players); return Args;
        }

        // Read-only dependency of footballNextAdvanceAt. This evaluates target distance
        // and action wakeups; it never produces NPC inputs or advances match state.
        enum class ENpcRole : uint8 { Keeper, Defender, Playmaker, Striker };
        struct FNpcStyle { ENpcRole Role; double Lane, Anticipation; };
        uint32 NpcSeed(const FString& Id)
        {
            uint32 Value = 2166136261u;
            const auto Units = StringCast<UTF16CHAR>(*Id, Id.Len());
            for (int32 I = 0; I < Units.Length(); ++I) Value = (Value ^ static_cast<uint32>(Units.Get()[I])) * 16777619u;
            return Value;
        }
        FNpcStyle NpcStyle(const FFootballPlayer& Player)
        {
            const uint32 Seed = NpcSeed(Player.Id);
            TArray<FString> Parts; Player.Id.ParseIntoArray(Parts, TEXT(":"), false);
            ENpcRole Role = static_cast<ENpcRole>(1 + Seed % 3);
            if (Parts.IsValidIndex(2))
            {
                if (Same(Parts[2], TEXT("defender"))) Role = ENpcRole::Defender;
                else if (Same(Parts[2], TEXT("playmaker"))) Role = ENpcRole::Playmaker;
                else if (Same(Parts[2], TEXT("striker"))) Role = ENpcRole::Striker;
            }
            if (Player.bGoalkeeper) Role = ENpcRole::Keeper;
            return { Role, (static_cast<int32>((Seed >> 4) % 3) - 1) * .48, .18 + (Seed % 5) * .05 };
        }
        double Clamp(double N, double Low, double High) { return FMath::Max(Low, FMath::Min(High, N)); }
        FFootballPoint BoundedPoint(const FFootballPitch& P, FFootballPoint Point)
        {
            constexpr double Margin = PlayerRadius + .2;
            return { Clamp(Point.X, P.Centre.X - P.HalfWidth + Margin, P.Centre.X + P.HalfWidth - Margin),
                Clamp(Point.Z, P.Centre.Z - P.HalfLength + Margin, P.Centre.Z + P.HalfLength - Margin) };
        }
        bool IdLess(const FString& A, const FString& B)
        {
            // NPC tie breaks use raw JS string order, unlike the opening roster's collation.
            return A.Compare(B, ESearchCase::CaseSensitive) < 0;
        }
        bool NpcsNeedAdvance(const FFootballMatch& Match, double NowMs)
        {
            if (Match.Phase != EFootballPhase::Playing) return false;
            TArray<const FFootballPlayer*> Active;
            for (const auto& P : Match.Players) if (P.bConnected && Active.Num() < MatchPlayers) Active.Add(&P);
            const FFootballPlayer* Owner = nullptr;
            for (const auto* P : Active) if (SameId(Match.Ball.OwnerId, P->Id)) { Owner = P; break; }
            const auto& Pitch = Match.Pitch;
            struct FNpcWake { FString Id; FFootballPoint Target; bool bAction = false; };
            TArray<FNpcWake> Wakes;
            const auto StoreWake = [&](const FString& Id, FFootballPoint Target, bool bAction) {
                auto* Prior = Wakes.FindByPredicate([&](const FNpcWake& W) { return Same(W.Id, Id); });
                if (Prior) { Prior->Target = Target; Prior->bAction = bAction; }
                else Wakes.Add({ Id, Target, bAction });
            };
            for (const auto* Player : Active)
            {
                if (!Player->bBot) continue;
                // The canonical predicate returns true for a bot carrier regardless of
                // its chosen kick/pass. Those choices are outside this dependency closure.
                if (SameId(Match.Ball.OwnerId, Player->Id)) return true;
                const auto Style = NpcStyle(*Player);
                const double Sign = Player->Side == EFootballSide::Home ? -1 : 1;
                FFootballPoint Target;
                bool bPursuing = false;
                if (Player->bGoalkeeper)
                {
                    Target = { Clamp(Match.Ball.Position.X, Pitch.Centre.X - Pitch.GoalHalfWidth + .3, Pitch.Centre.X + Pitch.GoalHalfWidth - .3),
                        Pitch.Centre.Z - Sign * (Pitch.HalfLength - 1.1) };
                    StoreWake(Player->Id, Target, false);
                    continue;
                }
                if (Owner && Owner->Side == Player->Side)
                {
                    const double Forward = Style.Role == ENpcRole::Striker ? 5 : Style.Role == ENpcRole::Defender ? -5 : 1.5;
                    const double Lane = Style.Lane != 0 ? Style.Lane : NpcSeed(Player->Id) % 2 ? .48 : -.48;
                    Target = BoundedPoint(Pitch, { Pitch.Centre.X + Lane * Pitch.HalfWidth, Owner->Position.Z + Sign * Forward });
                }
                else
                {
                    const auto TeamTarget = BoundedPoint(Pitch, { Match.Ball.Position.X + Match.Ball.Velocity.X * .28,
                        Match.Ball.Position.Z + Match.Ball.Velocity.Z * .28 });
                    const FFootballPlayer* Chaser = nullptr;
                    double BestDistance = std::numeric_limits<double>::infinity();
                    TArray<const FFootballPlayer*> Outfield, Threats;
                    for (const auto* Other : Active)
                    {
                        if (Other->Side == Player->Side && !Other->bGoalkeeper)
                        {
                            Outfield.Add(Other);
                            if (!Other->bBot) continue;
                            const double Score = Distance(Other->Position, TeamTarget) + (NpcStyle(*Other).Role == ENpcRole::Defender ? .7 : 0);
                            if (Score < BestDistance || (Score == BestDistance && Chaser && IdLess(Other->Id, Chaser->Id)))
                            { Chaser = Other; BestDistance = Score; }
                        }
                        else if (Other->Side != Player->Side && !Other->bGoalkeeper && (!Owner || !Same(Other->Id, Owner->Id))) Threats.Add(Other);
                    }
                    if (Chaser && Same(Chaser->Id, Player->Id))
                    {
                        Target = BoundedPoint(Pitch, Owner ? FFootballPoint { Owner->Position.X + Owner->Velocity.X * Style.Anticipation,
                            Owner->Position.Z + Owner->Velocity.Z * Style.Anticipation } : FFootballPoint {
                            Match.Ball.Position.X + Match.Ball.Velocity.X * Style.Anticipation, Match.Ball.Position.Z + Match.Ball.Velocity.Z * Style.Anticipation });
                        bPursuing = true;
                    }
                    else
                    {
                        const FFootballPoint OwnGoal { Pitch.Centre.X, Pitch.Centre.Z - Sign * Pitch.HalfLength };
                        Threats.StableSort([&](const FFootballPlayer& A, const FFootballPlayer& B) {
                            const double Delta = Distance(A.Position, OwnGoal) - Distance(B.Position, OwnGoal);
                            return Delta == 0 ? IdLess(A.Id, B.Id) : Delta < 0;
                        });
                        if (Chaser) Outfield.RemoveAll([&](const FFootballPlayer* P) { return Same(P->Id, Chaser->Id); });
                        Outfield.StableSort([](const FFootballPlayer& A, const FFootballPlayer& B) { return IdLess(A.Id, B.Id); });
                        const int32 Index = Outfield.IndexOfByPredicate([&](const FFootballPlayer* P) { return Same(P->Id, Player->Id); });
                        if (Owner && Threats.IsValidIndex(Index)) Target = { Threats[Index]->Position.X, Threats[Index]->Position.Z - Sign * 1.4 };
                        else Target = { Pitch.Centre.X + Style.Lane * Pitch.HalfWidth,
                            Pitch.Centre.Z + (Match.Ball.Position.Z - Pitch.Centre.Z) * .4 - Sign * Pitch.HalfLength * (Style.Role == ENpcRole::Defender ? .45 : .15) };
                        Target = BoundedPoint(Pitch, Target);
                    }
                }
                if (!bPursuing && Active.ContainsByPredicate([&](const FFootballPlayer* Other) {
                    return !Same(Other->Id, Player->Id) && Distance(Other->Position, Target) < 1.2;
                })) Target = BoundedPoint(Pitch, { Target.X + (NpcSeed(Player->Id) % 2 ? 1.4 : -1.4), Target.Z });
                const bool bChallenge = Owner && Owner->Side != Player->Side && Distance(Player->Position, Owner->Position) < 1.35
                    && NowMs - Player->ActionAtMs >= 650;
                StoreWake(Player->Id, Target, bChallenge);
            }
            // Canonical NPC Map uses the last value for duplicate IDs; broad read
            // decoding does not impose a new uniqueness rule on received players.
            for (const auto& P : Match.Players)
            {
                const auto* Wake = Wakes.FindByPredicate([&](const FNpcWake& W) { return Same(W.Id, P.Id); });
                if (Wake && (Wake->bAction || Distance(P.Position, Wake->Target) > (P.bGoalkeeper ? .08 : .1))) return true;
            }
            return false;
        }
        TOptional<double> NextAdvanceAt(const FFootballMatch& Match)
        {
            if (Match.Phase != EFootballPhase::Playing && Match.Phase != EFootballPhase::Restart) return {};
            if (Match.Phase == EFootballPhase::Restart)
            {
                const double Deadline = Match.EndsAtMs.Get(std::numeric_limits<double>::infinity());
                return FMath::Min(Deadline, Match.RestartAtMs.Get(Deadline));
            }
            bool bMoving = Length(Match.Ball.Velocity) > 0 || Match.Ball.Height > BallRadius || Match.Ball.VerticalSpeed != 0
                || Match.Players.ContainsByPredicate([](const FFootballPlayer& P) { return P.bConnected && Length(P.Velocity) > 0; })
                || NpcsNeedAdvance(Match, Match.SimulatedAtMs);
            for (int32 I = 0; !bMoving && I < Match.Players.Num(); ++I)
            {
                const auto& P = Match.Players[I]; if (!P.bConnected) continue;
                if (!Match.Ball.OwnerId.IsSet() && Match.Ball.Height <= 1.05 && Distance(P.Position, Match.Ball.Position) < (P.bGoalkeeper ? 1.05 : .7)) bMoving = true;
                for (int32 J = I + 1; !bMoving && J < Match.Players.Num(); ++J)
                    if (Match.Players[J].bConnected && Distance(P.Position, Match.Players[J].Position) < PlayerRadius * 2 - .001) bMoving = true;
            }
            if (bMoving) return Match.EndsAtMs.IsSet() ? FMath::Min(Match.EndsAtMs.GetValue(), Match.SimulatedAtMs + TickMs) : Match.SimulatedAtMs + TickMs;
            return Match.EndsAtMs;
        }

        bool Text(const FValue& V, FString& Out) { return String(V, Out) && Out.Len() <= MaxText; }
        bool Numeric(const FValue& V, double& Out) { return Number(V, Out); }
        bool Side(const FValue& V, EFootballSide& Out)
        {
            return EnumValue(V, Out, { { TEXT("home"), EFootballSide::Home }, { TEXT("away"), EFootballSide::Away } });
        }
        bool Phase(const FValue& V, EFootballPhase& Out)
        {
            return EnumValue(V, Out, { { TEXT("lobby"), EFootballPhase::Lobby }, { TEXT("playing"), EFootballPhase::Playing },
                { TEXT("restart"), EFootballPhase::Restart }, { TEXT("finished"), EFootballPhase::Finished },
                { TEXT("cancelled"), EFootballPhase::Cancelled } });
        }
        bool Winner(const FValue& V, EFootballWinner& Out)
        {
            return EnumValue(V, Out, { { TEXT("home"), EFootballWinner::Home }, { TEXT("away"), EFootballWinner::Away },
                { TEXT("draw"), EFootballWinner::Draw } });
        }
        template <typename T, typename F> bool Optional(const FValue& V, TOptional<T>& Out, F Read)
        {
            if (!V.IsValid()) return true;
            T Next;
            if (!Read(V, Next)) return false;
            Out = MoveTemp(Next);
            return true;
        }
        bool Point(const FValue& V, FFootballPoint& Out)
        {
            const auto O = Object(V);
            return Only(O, { TEXT("x"), TEXT("z") }) && Number(O, TEXT("x"), Out.X) && Number(O, TEXT("z"), Out.Z);
        }
        bool Pitch(const FValue& V, FFootballPitch& Out)
        {
            const auto O = Object(V);
            return Only(O, { TEXT("id"), TEXT("instanceId"), TEXT("revision"), TEXT("floorY"), TEXT("centre"),
                    TEXT("halfWidth"), TEXT("halfLength"), TEXT("goalHalfWidth"), TEXT("goalHeight") })
                && Text(Field(O, TEXT("id")), Out.Id) && Text(Field(O, TEXT("instanceId")), Out.InstanceId)
                && Number(O, TEXT("revision"), Out.Revision) && Number(O, TEXT("floorY"), Out.FloorY)
                && Point(Field(O, TEXT("centre")), Out.Centre) && Number(O, TEXT("halfWidth"), Out.HalfWidth)
                && Number(O, TEXT("halfLength"), Out.HalfLength) && Number(O, TEXT("goalHalfWidth"), Out.GoalHalfWidth)
                && Number(O, TEXT("goalHeight"), Out.GoalHeight);
        }
        bool Player(const FValue& V, FFootballPlayer& Out)
        {
            const auto O = Object(V);
            return Only(O, { TEXT("id"), TEXT("side"), TEXT("bot"), TEXT("goalkeeper"), TEXT("position"), TEXT("velocity"),
                    TEXT("yaw"), TEXT("ready"), TEXT("connected"), TEXT("lastSequence"), TEXT("actionAt"), TEXT("goals"), TEXT("assists") })
                && Text(Field(O, TEXT("id")), Out.Id) && Side(Field(O, TEXT("side")), Out.Side)
                && Boolean(Field(O, TEXT("bot")), Out.bBot) && Boolean(Field(O, TEXT("goalkeeper")), Out.bGoalkeeper)
                && Point(Field(O, TEXT("position")), Out.Position) && Point(Field(O, TEXT("velocity")), Out.Velocity)
                && Number(O, TEXT("yaw"), Out.Yaw) && Boolean(Field(O, TEXT("ready")), Out.bReady)
                && Boolean(Field(O, TEXT("connected")), Out.bConnected) && Number(O, TEXT("lastSequence"), Out.LastSequence)
                && Number(O, TEXT("actionAt"), Out.ActionAtMs) && Number(O, TEXT("goals"), Out.Goals)
                && Number(O, TEXT("assists"), Out.Assists);
        }
        bool Ball(const FValue& V, FFootballBall& Out)
        {
            const auto O = Object(V);
            return Only(O, { TEXT("position"), TEXT("velocity"), TEXT("height"), TEXT("verticalSpeed"),
                    TEXT("ownerId"), TEXT("lastTouchId"), TEXT("lastPassId"), TEXT("claimAfter") })
                && Point(Field(O, TEXT("position")), Out.Position) && Point(Field(O, TEXT("velocity")), Out.Velocity)
                && Number(O, TEXT("height"), Out.Height) && Number(O, TEXT("verticalSpeed"), Out.VerticalSpeed)
                && Nullable(Field(O, TEXT("ownerId")), Out.OwnerId, Text)
                && Nullable(Field(O, TEXT("lastTouchId")), Out.LastTouchId, Text)
                && Nullable(Field(O, TEXT("lastPassId")), Out.LastPassId, Text)
                && Number(O, TEXT("claimAfter"), Out.ClaimAfterMs);
        }
        bool Score(const FValue& V, FFootballScore& Out)
        {
            const auto O = Object(V);
            return Only(O, { TEXT("home"), TEXT("away") }) && Number(O, TEXT("home"), Out.Home) && Number(O, TEXT("away"), Out.Away);
        }
        bool Objective(const FValue& V, FFootballObjective& Out)
        {
            const auto O = Object(V);
            return Only(O, { TEXT("goalsToWin"), TEXT("durationMs") })
                && Number(O, TEXT("goalsToWin"), Out.GoalsToWin) && Number(O, TEXT("durationMs"), Out.DurationMs);
        }
        bool Event(const FValue& V, FFootballEvent& Out)
        {
            const auto O = Object(V);
            return Only(O, { TEXT("id"), TEXT("kind"), TEXT("side"), TEXT("playerId") })
                && Number(O, TEXT("id"), Out.Id)
                && EnumValue(Field(O, TEXT("kind")), Out.Kind, { { TEXT("kickoff"), EFootballEventKind::Kickoff },
                    { TEXT("goal"), EFootballEventKind::Goal }, { TEXT("out"), EFootballEventKind::Out },
                    { TEXT("save"), EFootballEventKind::Save }, { TEXT("finished"), EFootballEventKind::Finished },
                    { TEXT("cancelled"), EFootballEventKind::Cancelled } })
                && Nullable(Field(O, TEXT("side")), Out.Side, Side)
                && Nullable(Field(O, TEXT("playerId")), Out.PlayerId, Text);
        }
        bool Match(const FValue& V, FFootballMatch& Out)
        {
            const auto O = Object(V);
            return Only(O, { TEXT("id"), TEXT("epoch"), TEXT("revision"), TEXT("pitch"), TEXT("mode"), TEXT("phase"),
                    TEXT("players"), TEXT("ball"), TEXT("score"), TEXT("winner"), TEXT("startedAt"), TEXT("endsAt"),
                    TEXT("simulatedAt"), TEXT("restartAt"), TEXT("restartSide"), TEXT("objective"), TEXT("lastEvent") })
                && Text(Field(O, TEXT("id")), Out.Id) && Number(O, TEXT("epoch"), Out.Epoch)
                && Number(O, TEXT("revision"), Out.Revision) && Pitch(Field(O, TEXT("pitch")), Out.Pitch)
                && EnumValue(Field(O, TEXT("mode")), Out.Mode, { { TEXT("solo"), EFootballMode::Solo },
                    { TEXT("teams"), EFootballMode::Teams }, { TEXT("coop"), EFootballMode::Coop } })
                && Phase(Field(O, TEXT("phase")), Out.Phase)
                && ReadBoundedArray(Field(O, TEXT("players")), Out.Players, MatchPlayers, Player)
                && Ball(Field(O, TEXT("ball")), Out.Ball) && Score(Field(O, TEXT("score")), Out.Score)
                && Nullable(Field(O, TEXT("winner")), Out.Winner, Winner)
                && Nullable(Field(O, TEXT("startedAt")), Out.StartedAtMs, Numeric)
                && Nullable(Field(O, TEXT("endsAt")), Out.EndsAtMs, Numeric)
                && Number(O, TEXT("simulatedAt"), Out.SimulatedAtMs)
                && Nullable(Field(O, TEXT("restartAt")), Out.RestartAtMs, Numeric)
                && Side(Field(O, TEXT("restartSide")), Out.RestartSide)
                && Objective(Field(O, TEXT("objective")), Out.Objective)
                && Nullable(Field(O, TEXT("lastEvent")), Out.LastEvent, Event);
        }
        bool Advancement(const FValue& V, FFootballAdvancement& Out)
        {
            const auto O = Object(V);
            return Only(O, { TEXT("ownerId"), TEXT("ownerUntil"), TEXT("nextAt"), TEXT("serverNow") })
                && Nullable(Field(O, TEXT("ownerId")), Out.OwnerId, Text)
                && Number(O, TEXT("ownerUntil"), Out.OwnerUntilMs)
                && Nullable(Field(O, TEXT("nextAt")), Out.NextAtMs, Numeric)
                && Number(O, TEXT("serverNow"), Out.ServerNowMs);
        }
        bool Snapshot(const FObject& O, FFootballSnapshot& Out)
        {
            return Only(O, { TEXT("playerId"), TEXT("host"), TEXT("callSessionId"), TEXT("match"), TEXT("ready"),
                    TEXT("busy"), TEXT("error"), TEXT("advancement") })
                && Nullable(Field(O, TEXT("playerId")), Out.PlayerId, Text) && Boolean(Field(O, TEXT("host")), Out.bHost)
                && Nullable(Field(O, TEXT("callSessionId")), Out.CallSessionId, Text)
                && Nullable(Field(O, TEXT("match")), Out.Match, Match) && Boolean(Field(O, TEXT("ready")), Out.bReady)
                && Boolean(Field(O, TEXT("busy")), Out.bBusy) && Nullable(Field(O, TEXT("error")), Out.Error, Text)
                && Optional(Field(O, TEXT("advancement")), Out.Advancement, Advancement);
        }
        bool EntryPlayer(const FValue& V, FFootballEntryPlayer& Out)
        {
            const auto O = Object(V);
            return Only(O, { TEXT("id"), TEXT("name"), TEXT("side") }) && Text(Field(O, TEXT("id")), Out.Id)
                && Text(Field(O, TEXT("name")), Out.Name) && Nullable(Field(O, TEXT("side")), Out.Side, Side);
        }
        bool EntryRoster(const FObject& O, FFootballEntryRoster& Out)
        {
            return Only(O, { TEXT("playerId"), TEXT("instanceId"), TEXT("revision"), TEXT("epoch"), TEXT("phase"),
                    TEXT("serverNow"), TEXT("players") })
                && Text(Field(O, TEXT("playerId")), Out.PlayerId) && Text(Field(O, TEXT("instanceId")), Out.InstanceId)
                && Number(O, TEXT("revision"), Out.Revision) && Number(O, TEXT("epoch"), Out.Epoch)
                && Nullable(Field(O, TEXT("phase")), Out.Phase, Phase) && Number(O, TEXT("serverNow"), Out.ServerNowMs)
                && ReadBoundedArray(Field(O, TEXT("players")), Out.Players, EntryPlayers, EntryPlayer);
        }
        bool SameScope(const FFootballReadScope& A, const FFootballReadScope& B)
        {
            return Same(A.AccountId, B.AccountId) && Same(A.SessionIncarnation, B.SessionIncarnation) && Same(A.RoomId, B.RoomId);
        }
        bool ValidScope(const FFootballReadScope& Scope)
        {
            return !Scope.AccountId.IsEmpty() && !Scope.SessionIncarnation.IsEmpty() && !Scope.RoomId.IsEmpty()
                && Scope.AccountId.Len() <= MaxText && Scope.SessionIncarnation.Len() <= MaxText && Scope.RoomId.Len() <= MaxText;
        }
        bool SamePlayerScope(const TOptional<FString>& A, const TOptional<FString>& B)
        {
            // The client uses playerId ?? "". Preserve null/empty equivalence for ordering
            // without losing the original nullable field in the displayed snapshot.
            return Same(A.Get(FString()), B.Get(FString()));
        }
        bool SameMatchScope(const FFootballSnapshot& A, const FFootballSnapshot& B)
        {
            if (A.Match.IsSet() != B.Match.IsSet() || !SamePlayerScope(A.PlayerId, B.PlayerId)) return false;
            return !A.Match.IsSet() || (Same(A.Match->Id, B.Match->Id) && A.Match->Epoch == B.Match->Epoch);
        }
        bool SameRevision(const FFootballSnapshot& A, const FFootballSnapshot& B)
        {
            return A.Match.IsSet() == B.Match.IsSet() && (!A.Match.IsSet() || A.Match->Revision == B.Match->Revision);
        }
    }

    bool DecodeFootballSnapshot(const FString& Json, TOptional<FFootballSnapshot>& Out, FString& OutError)
    {
        Out.Reset();
        OutError = FootballPrivate::InvalidMatch;
        StateJsonPrivate::FObject Root;
        FFootballSnapshot Next;
        if (!StateJsonPrivate::Parse(Json, Root, FootballPrivate::Limits) || !FootballPrivate::Snapshot(Root, Next)) return false;
        Out = MoveTemp(Next);
        OutError.Reset();
        return true;
    }
    bool DecodeFootballEntryRoster(const FString& Json, TOptional<FFootballEntryRoster>& Out, FString& OutError)
    {
        Out.Reset();
        OutError = FootballPrivate::InvalidEntry;
        StateJsonPrivate::FObject Root;
        FFootballEntryRoster Next;
        if (!StateJsonPrivate::Parse(Json, Root, FootballPrivate::Limits) || !FootballPrivate::EntryRoster(Root, Next)) return false;
        Out = MoveTemp(Next);
        OutError.Reset();
        return true;
    }
    bool FFootballReadState::Select(const FFootballReadScope& Scope)
    {
        if (bDisposed || bFinalizing) return false;
        if (FootballPrivate::SameScope(Selection, Scope)) return true;
        return ResetSession(Scope);
    }
    bool FFootballReadState::ResetSession(const FFootballReadScope& Scope)
    {
        if (bDisposed || bFinalizing) return false;
        // Copy first: callers may pass GetSelection().
        const FFootballReadScope Next = Scope;
        InvalidateMatchObservation();
        ResetLifecycle();
        RetireCommand();
        SessionGeneration = FGuid::NewGuid();
        EntryGenerations.Reset();
        Selection = Next;
        View = FFootballSnapshot{};
        bHasApplied = false;
        Changed.Broadcast();
        return true;
    }
    bool FFootballReadState::CanObserve(int64 CapturedNowMs) const
    {
        return !bDisposed && !bFinalizing && FootballPrivate::ValidScope(Selection)
            && CapturedNowMs >= 0 && CapturedNowMs <= FootballPrivate::MaxCapturedNowMs;
    }
    bool FFootballReadState::BeginMatchObservation(int64 CapturedNowMs, FFootballReadReceipt& Out)
    {
        Out = FFootballReadReceipt{};
        if (bFinalizing) return false;
        InvalidateMatchObservation();
        if (!CanObserve(CapturedNowMs)) return false;
        bMatchObserved = true;
        Out.Generation = MatchGeneration;
        Out.Scope = Selection;
        Out.CapturedNowMs = CapturedNowMs;
        SyncPump();
        return true;
    }
    void FFootballReadState::EndMatchObservation()
    {
        if (bDisposed || bFinalizing) return;
        PrepareFinalEffects();
        InvalidateMatchObservation();
        InputEffect.Reset(); NeutralEffect.Reset(); PulseEffect.Reset(); bReleasePending = false;
        bPumpRunning = false;
    }
    void FFootballReadState::InvalidateMatchObservation()
    {
        // Canonical generation replacement does not clear command busy, even in flight.
        MatchGeneration = FGuid::NewGuid();
        bMatchObserved = false;
        bPumpRunning = false;
    }
    bool FFootballReadState::ReloadMatch(int64 CapturedNowMs, FFootballReadReceipt& Out)
    {
        Out = FFootballReadReceipt{};
        if (bDisposed || bFinalizing) return false;
        ResetInput();
        View.Error.Reset();
        bool bConnected = false;
        if (bMatchObserved) bConnected = BeginMatchObservation(CapturedNowMs, Out);
        else InvalidateMatchObservation();
        SyncPump();
        Changed.Broadcast();
        return bConnected;
    }
    bool FFootballReadState::CurrentMatch(const FFootballReadReceipt& Receipt) const
    {
        return !bDisposed && !bFinalizing && bMatchObserved && !Receipt.bEntry && Receipt.Generation.IsValid()
            && Receipt.Generation == MatchGeneration && Receipt.Scope.IsSet()
            && FootballPrivate::SameScope(Selection, Receipt.Scope.GetValue());
    }
    bool FFootballReadState::CurrentEntry(const FFootballReadReceipt& Receipt) const
    {
        return !bDisposed && !bFinalizing && Receipt.bEntry && Receipt.Generation.IsValid() && EntryGenerations.Contains(Receipt.Generation)
            && Receipt.Scope.IsSet() && FootballPrivate::SameScope(Selection, Receipt.Scope.GetValue());
    }
    void FFootballReadState::FailMatch(const FString& Message)
    {
        // Ordinary query failure retains the entire current match, ready and advancement.
        bActive = false; bPumpRunning = false; ResetInput();
        View.Error = Message;
        Changed.Broadcast();
    }
    EFootballReadResult FFootballReadState::AcceptMatch(const FFootballReadReceipt& Receipt, const FString& Json)
    {
        if (!CurrentMatch(Receipt)) return EFootballReadResult::StaleReceipt;
        TOptional<FFootballSnapshot> Decoded;
        FString Error;
        if (!DecodeFootballSnapshot(Json, Decoded, Error))
        {
            FailMatch(Error);
            return EFootballReadResult::InvalidResponse;
        }
        return ApplyMatch(MoveTemp(Decoded.GetValue()));
    }
    EFootballReadResult FFootballReadState::ApplyMatch(FFootballSnapshot Next)
    {
        if (View.Match.IsSet() && Next.Match.IsSet() && StateJsonPrivate::Same(View.Match->Id, Next.Match->Id)
            && Next.Match->Epoch < View.Match->Epoch) return EFootballReadResult::OlderEpoch;
        const bool bSameScope = bHasApplied && FootballPrivate::SameMatchScope(View, Next);
        if (bSameScope && View.Match.IsSet() && Next.Match.IsSet() && Next.Match->Revision < View.Match->Revision)
            return EFootballReadResult::OlderRevision;
        if (bSameScope && FootballPrivate::SameRevision(View, Next) && View.Advancement.IsSet() && Next.Advancement.IsSet()
            && Next.Advancement->ServerNowMs < View.Advancement->ServerNowMs) return EFootballReadResult::OlderAdvancement;
        if (!bSameScope)
        {
            ResetInput(); bSentMovement = false; bNeutralPending = false;
            if (NeutralEffect.IsSet() && NeutralEffect->Stage == FFootballEffectRequest::EStage::Prepared) NeutralEffect.Reset();
            const auto* Player = FootballPrivate::CurrentPlayer(Next);
            Sequence = Player ? Player->LastSequence : 0;
            ActionSequence = 0; LastSendAtMs.Reset();
        }
        if (View.Match.IsSet() != Next.Match.IsSet() || (View.Match.IsSet() && View.Match->Phase != Next.Match->Phase))
        {
            ResetInput(); bSentMovement = false; bNeutralPending = false;
            if (NeutralEffect.IsSet() && NeutralEffect->Stage == FFootballEffectRequest::EStage::Prepared) NeutralEffect.Reset();
        }
        const auto* Player = FootballPrivate::CurrentPlayer(Next);
        if ((Player ? Player->LastSequence : 0) >= ActionSequence) ActionSequence = 0;
        // Neither query nor command wire data may take ownership of the local busy latch.
        Next.bBusy = View.bBusy;
        Next.Error.Reset();
        View = MoveTemp(Next);
        bHasApplied = true;
        SyncPump();
        Changed.Broadcast();
        return EFootballReadResult::Accepted;
    }
    bool FFootballReadState::AcceptMatchFailure(const FFootballReadReceipt& Receipt, const FFootballPublicReadFailure& Failure)
    {
        if (!CurrentMatch(Receipt)) return false;
        FailMatch(Failure.Message.IsSet() ? Failure.Message.GetValue() : FString(FootballPrivate::Disconnected));
        return true;
    }
    TOptional<FFootballCommandRequest> FFootballReadState::PrepareCommand(const FFootballCommand& Command)
    {
        using namespace FootballPrivate;
        if (bDisposed || bFinalizing || View.bBusy || !View.Match.IsSet() || !ValidScope(Selection)) return {};
        // Read epochs stay finite doubles. Only this outbound boundary needs a safe integer,
        // and assertEpoch permits negative and zero values when authoritative state agrees.
        int64 Epoch = 0;
        if (!Integer(MakeShared<FJsonValueNumber>(View.Match->Epoch), Epoch, -9007199254740991LL)) return {};
        const auto Value = MakeShared<FJsonObject>();
        switch (Command.Type)
        {
        case EFootballCommandType::Ready:
            Value->SetStringField(TEXT("type"), TEXT("ready")); Value->SetBoolField(TEXT("ready"), Command.bReady); break;
        case EFootballCommandType::Side:
            if (Command.Side != EFootballSide::Home && Command.Side != EFootballSide::Away) return {};
            Value->SetStringField(TEXT("type"), TEXT("side"));
            Value->SetStringField(TEXT("side"), Command.Side == EFootballSide::Home ? TEXT("home") : TEXT("away")); break;
        case EFootballCommandType::Start: Value->SetStringField(TEXT("type"), TEXT("start")); break;
        case EFootballCommandType::Retry: Value->SetStringField(TEXT("type"), TEXT("retry")); break;
        case EFootballCommandType::Rejoin: Value->SetStringField(TEXT("type"), TEXT("rejoin")); break;
        case EFootballCommandType::Leave: Value->SetStringField(TEXT("type"), TEXT("leave")); break;
        case EFootballCommandType::End: Value->SetStringField(TEXT("type"), TEXT("end")); break;
        default: return {};
        }
        FFootballCommandRequest Request;
        Request.Type = Command.Type;
        Request.Receipt.QueryGeneration = MatchGeneration;
        Request.Receipt.SessionGeneration = SessionGeneration;
        Request.Receipt.RequestId = FGuid::NewGuid(); Request.Receipt.Scope = Selection;
        const auto Args = MakeShared<FJsonObject>();
        Args->SetStringField(TEXT("roomId"), Request.Receipt.Scope.RoomId);
        Args->SetNumberField(TEXT("epoch"), static_cast<double>(Epoch)); Args->SetObjectField(TEXT("command"), Value);
        if (!FJsonSerializer::Serialize(Args, TJsonWriterFactory<>::Create(&Request.ArgumentsJson))) return {};
        PendingCommand = Request; CommandStage = ECommandStage::Prepared;
        ResetInput(); bPumpRunning = false;
        View.bBusy = true; View.Error.Reset(); Changed.Broadcast();
        // A query change intentionally leaves busy set. Its unsent receipt remains available
        // through GetPendingCommand; a replaced session must expose none of its old work.
        return CurrentCommand(Request.Receipt) ? TOptional<FFootballCommandRequest>(Request) : TOptional<FFootballCommandRequest>();
    }
    bool FFootballReadState::OwnsCommand(const FFootballCommandReceipt& Receipt) const
    {
        return !bDisposed && !bFinalizing && PendingCommand.IsSet() && Receipt.RequestId.IsValid()
            && Receipt.RequestId == PendingCommand->Receipt.RequestId && Receipt.SessionGeneration == SessionGeneration
            && Receipt.QueryGeneration == PendingCommand->Receipt.QueryGeneration
            && FootballPrivate::SameScope(Receipt.Scope, Selection);
    }
    bool FFootballReadState::CurrentCommand(const FFootballCommandReceipt& Receipt) const
    {
        // No bMatchObserved check: canonical send remains available without query listeners.
        return OwnsCommand(Receipt) && Receipt.QueryGeneration == MatchGeneration;
    }
    TOptional<FFootballCommandRequest> FFootballReadState::DispatchCommand(const FFootballCommandReceipt& Receipt)
    {
        if (!CurrentCommand(Receipt) || CommandStage != ECommandStage::Prepared) return {};
        CommandStage = ECommandStage::Dispatched;
        return PendingCommand.GetValue();
    }
    TOptional<FFootballCommandReceipt> FFootballReadState::GetPendingCommand() const
    {
        return PendingCommand.IsSet() ? TOptional<FFootballCommandReceipt>(PendingCommand->Receipt) : TOptional<FFootballCommandReceipt>();
    }
    bool FFootballReadState::AbandonCommand(const FFootballCommandReceipt& Receipt)
    {
        if (!OwnsCommand(Receipt) || CommandStage != ECommandStage::Prepared) return false;
        RetireCommand(); View.bBusy = false; SyncPump(); Changed.Broadcast();
        return true;
    }
    void FFootballReadState::RetireCommand()
    {
        PendingCommand.Reset(); CommandStage = ECommandStage::Prepared;
    }
    bool FFootballReadState::CanCompleteCommand(const FFootballCommandReceipt& Receipt) const
    {
        return OwnsCommand(Receipt) && CommandStage == ECommandStage::Dispatched;
    }
    void FFootballReadState::FinishCommand(const FFootballCommandReceipt& Receipt)
    {
        // The apply/error notification may replace query generation, session or command.
        if (!OwnsCommand(Receipt)) return;
        const bool bCurrent = CurrentCommand(Receipt);
        RetireCommand();
        if (bCurrent) { View.bBusy = false; SyncPump(); Changed.Broadcast(); }
    }
    TOptional<FFootballCommandSuccess> FFootballReadState::BeginCommandSuccess(const FFootballCommandReceipt& Receipt, const FString& Json)
    {
        // Notifications may overwrite the caller's receipt with replacement work.
        const FFootballCommandReceipt CompletionReceipt = Receipt;
        if (!CanCompleteCommand(CompletionReceipt)) return {};
        CommandStage = ECommandStage::Completing;
        if (!CurrentCommand(CompletionReceipt)) { FinishCommand(CompletionReceipt); return {}; }
        FFootballCommandSuccess Success;
        FString Error;
        if (!DecodeFootballSnapshot(Json, Success.Snapshot, Error))
        {
            View.Error = Error; Changed.Broadcast(); FinishCommand(CompletionReceipt); return {};
        }
        Success.Receipt = CompletionReceipt;
        Success.bDeactivateInput = PendingCommand->Type == EFootballCommandType::Leave || PendingCommand->Type == EFootballCommandType::End;
        return Success;
    }
    bool FFootballReadState::IsCommandSuccessCurrent(const FFootballCommandSuccess& Success) const
    {
        return Success.Snapshot.IsSet() && CommandStage == ECommandStage::Completing && CurrentCommand(Success.Receipt);
    }
    EFootballReadResult FFootballReadState::AcceptCommandSuccess(const FFootballCommandSuccess& Success)
    {
        const FFootballCommandReceipt CompletionReceipt = Success.Receipt;
        if (!Success.Snapshot.IsSet() || !OwnsCommand(CompletionReceipt) || CommandStage != ECommandStage::Completing)
            return EFootballReadResult::StaleReceipt;
        if (!CurrentCommand(CompletionReceipt)) { FinishCommand(CompletionReceipt); return EFootballReadResult::StaleReceipt; }
        CommandStage = ECommandStage::Applying;
        if (Success.bDeactivateInput) bActive = false;
        const auto Result = ApplyMatch(Success.Snapshot.GetValue());
        FinishCommand(CompletionReceipt);
        return Result;
    }
    bool FFootballReadState::AcceptCommandFailure(const FFootballCommandReceipt& Receipt, const FFootballPublicReadFailure& Failure)
    {
        const FFootballCommandReceipt CompletionReceipt = Receipt;
        if (!CanCompleteCommand(CompletionReceipt)) return false;
        CommandStage = ECommandStage::Completing;
        if (!CurrentCommand(CompletionReceipt)) { FinishCommand(CompletionReceipt); return false; }
        View.Error = Failure.Message.IsSet() ? Failure.Message.GetValue() : FString(FootballPrivate::CommandFailed);
        Changed.Broadcast();
        FinishCommand(CompletionReceipt);
        return true;
    }
    const TCHAR* FFootballEffectRequest::GetEndpoint() const
    {
        switch (Receipt.GetKind())
        {
        case EFootballEffect::Open: return TEXT("companionFootball:open");
        case EFootballEffect::Input: case EFootballEffect::Neutral: return TEXT("companionFootball:input");
        default: return TEXT("companionFootball:pulse");
        }
    }
    TOptional<FFootballEffectRequest> FFootballReadState::PrepareOpen(const FFootballEntrySelection& Entry)
    {
        if (bDisposed || bFinalizing || View.bBusy || !FootballPrivate::ValidScope(Selection) || !FootballPrivate::ValidEntry(Entry, false)) return {};
        FFootballEntrySelection Captured = Entry;
        // UE default culture/comparison level, stable ties. Never normalize identifier bytes.
        Captured.Players.StableSort([](const FFootballEntrySeat& A, const FFootballEntrySeat& B) {
            return FTextComparison::CompareTo(A.Id, B.Id, ETextComparisonLevel::Default) < 0;
        });
        if (PendingEntry.IsSet() && FootballPrivate::SameEntry(PendingEntry.GetValue(), Captured)) Captured = PendingEntry.GetValue();
        if (!FootballPrivate::ValidEntry(Captured)) return {};
        FFootballEffectRequest Request; Request.Receipt = NewEffect(EFootballEffect::Open);
        if (!FootballPrivate::Json(FootballPrivate::EntryJson(Captured, Selection.RoomId), Request.ArgumentsJson)) return {};
        PendingEntry = MoveTemp(Captured); OpenEffect = Request;
        ResetInput(); View.bBusy = true; View.Error.Reset(); bPumpRunning = false;
        Changed.Broadcast();
        return CurrentEffect(Request.Receipt) ? TOptional<FFootballEffectRequest>(Request) : TOptional<FFootballEffectRequest>();
    }
    void FFootballReadState::FinishOpen(const FFootballEffectReceipt& Receipt)
    {
        if (!OwnsEffect(Receipt)) return;
        OpenEffect.Reset(); View.bBusy = false; SyncPump(); Changed.Broadcast();
    }
    FFootballOpenOutcome FFootballReadState::AcceptOpenSuccess(const FFootballEffectReceipt& Receipt, const FString& Json)
    {
        const FFootballEffectReceipt Captured = Receipt;
        FFootballOpenOutcome Outcome;
        if (Captured.Kind != EFootballEffect::Open || !ClaimEffect(Captured)) return Outcome;
        TOptional<FFootballSnapshot> Next; FString Error;
        if (!DecodeFootballSnapshot(Json, Next, Error))
        { Outcome.Result = EFootballReadResult::InvalidResponse; Outcome.Error = Error; }
        else
        {
            // Open differs from command: current-session success clears the retry even
            // after a query change, while only a current query can apply its snapshot.
            PendingEntry.Reset();
            if (CurrentEffect(Captured)) Outcome.Result = ApplyMatch(MoveTemp(Next.GetValue()));
        }
        FinishOpen(Captured);
        return Outcome;
    }
    FFootballOpenOutcome FFootballReadState::AcceptOpenFailure(const FFootballEffectReceipt& Receipt, const FFootballPublicReadFailure& Failure)
    {
        const FFootballEffectReceipt Captured = Receipt;
        FFootballOpenOutcome Outcome;
        if (Captured.Kind != EFootballEffect::Open || !ClaimEffect(Captured)) return Outcome;
        Outcome.Result = EFootballReadResult::InvalidResponse;
        Outcome.Error = Failure.Message.IsSet() ? Failure.Message.GetValue() : FString(FootballPrivate::OpenFailed);
        FinishOpen(Captured);
        return Outcome;
    }
    bool FFootballReadState::Playing() const
    {
        return View.bReady && (!View.Error.IsSet() || View.Error->IsEmpty()) && !View.bBusy && View.Match.IsSet()
            && (View.Match->Phase == EFootballPhase::Playing || View.Match->Phase == EFootballPhase::Restart)
            && View.PlayerId.IsSet() && View.Match->Players.ContainsByPredicate([&](const FFootballPlayer& Player) {
                return Player.bConnected && FootballPrivate::Same(Player.Id, View.PlayerId.GetValue());
            });
    }
    void FFootballReadState::SyncPump()
    {
        bPumpRunning = !bDisposed && !bFinalizing && bMatchObserved && Playing() && (bForeground || bActive || bNeutralPending);
    }
    TOptional<FTimespan> FFootballReadState::GetPumpInterval() const
    {
        return bPumpRunning ? TOptional<FTimespan>(FTimespan::FromMilliseconds(FootballPrivate::InputMs)) : TOptional<FTimespan>();
    }
    void FFootballReadState::ResetInput()
    {
        Latest = FFootballInput{}; Actions.Reset(); bDirty = false;
        bNeutralPending |= bSentMovement || ActionSequence > 0;
        // A prepared packet is not sent. Never dispatch old movement after focus/reset.
        if (InputEffect.IsSet() && InputEffect->Stage == FFootballEffectRequest::EStage::Prepared) InputEffect.Reset();
    }
    void FFootballReadState::ResetLifecycle()
    {
        OpenEffect.Reset(); InputEffect.Reset(); NeutralEffect.Reset(); PulseEffect.Reset(); PendingEntry.Reset();
        Latest = FFootballInput{}; Actions.Reset(); Sequence = 0; ActionSequence = 0; ServerOffsetMs = 0;
        LastSendAtMs.Reset(); LastPulseAtMs.Reset();
        bActive = bForeground = bDirty = bSentMovement = bNeutralPending = bReleasePending = bPumpRunning = false;
    }
    FFootballEffectReceipt FFootballReadState::NewEffect(EFootballEffect Kind) const
    {
        FFootballEffectReceipt Receipt;
        Receipt.Kind = Kind; Receipt.RequestId = FGuid::NewGuid(); Receipt.SessionGeneration = SessionGeneration;
        Receipt.QueryGeneration = MatchGeneration; Receipt.Scope = Selection; Receipt.PlayerId = View.PlayerId;
        if (View.Match.IsSet()) { Receipt.MatchId = View.Match->Id; Receipt.Epoch = View.Match->Epoch; }
        return Receipt;
    }
    TOptional<FFootballEffectRequest>* FFootballReadState::EffectSlot(EFootballEffect Kind)
    {
        switch (Kind)
        {
        case EFootballEffect::Open: return &OpenEffect;
        case EFootballEffect::Input: return &InputEffect;
        case EFootballEffect::Neutral: return &NeutralEffect;
        case EFootballEffect::Pulse: case EFootballEffect::Release: return &PulseEffect;
        default: return nullptr;
        }
    }
    const TOptional<FFootballEffectRequest>* FFootballReadState::EffectSlot(EFootballEffect Kind) const
    {
        switch (Kind)
        {
        case EFootballEffect::Open: return &OpenEffect;
        case EFootballEffect::Input: return &InputEffect;
        case EFootballEffect::Neutral: return &NeutralEffect;
        case EFootballEffect::Pulse: case EFootballEffect::Release: return &PulseEffect;
        default: return nullptr;
        }
    }
    bool FFootballReadState::OwnsEffect(const FFootballEffectReceipt& Receipt) const
    {
        const auto* Slot = EffectSlot(Receipt.Kind);
        return !bDisposed && Slot && Slot->IsSet() && Receipt.RequestId.IsValid()
            && Slot->GetValue().Receipt.RequestId == Receipt.RequestId && Slot->GetValue().Receipt.Kind == Receipt.Kind
            && Receipt.SessionGeneration == SessionGeneration && FootballPrivate::SameScope(Receipt.Scope, Selection);
    }
    bool FFootballReadState::CurrentEffect(const FFootballEffectReceipt& Receipt) const
    {
        if (!OwnsEffect(Receipt) || Receipt.QueryGeneration != MatchGeneration) return false;
        return Receipt.Kind == EFootballEffect::Open || (View.Match.IsSet() && FootballPrivate::Same(Receipt.MatchId, View.Match->Id)
            && Receipt.Epoch == View.Match->Epoch && FootballPrivate::SamePlayerScope(Receipt.PlayerId, View.PlayerId));
    }
    TArray<FFootballEffectRequest> FFootballReadState::GetPendingEffects() const
    {
        TArray<FFootballEffectRequest> Out;
        if (bDisposed || bFinalizing) return Out;
        // Cancel overtakes ordinary input. Include stale prepared packets for explicit abandonment.
        for (const auto* Slot : { &NeutralEffect, &OpenEffect, &InputEffect, &PulseEffect })
            if (Slot->IsSet() && Slot->GetValue().Stage == FFootballEffectRequest::EStage::Prepared) Out.Add(Slot->GetValue());
        return Out;
    }
    TOptional<FFootballEffectRequest> FFootballReadState::DispatchEffect(const FFootballEffectReceipt& Receipt)
    {
        if (!CurrentEffect(Receipt)) return {};
        auto& Request = EffectSlot(Receipt.Kind)->GetValue();
        if (Request.Stage != FFootballEffectRequest::EStage::Prepared || (bFinalizing && !Request.bFinal)) return {};
        if (Receipt.Kind == EFootballEffect::Input && (!bActive || !Playing() || NeutralEffect.IsSet())) return {};
        if (Receipt.Kind == EFootballEffect::Neutral && !Playing()) return {};
        if (Receipt.Kind == EFootballEffect::Pulse && (!bForeground || !Playing())) return {};
        Request.Stage = FFootballEffectRequest::EStage::Dispatched;
        return Request;
    }
    bool FFootballReadState::AbandonEffect(const FFootballEffectReceipt& Receipt)
    {
        const FFootballEffectReceipt Captured = Receipt;
        if (bFinalizing || !OwnsEffect(Captured) || EffectSlot(Captured.Kind)->GetValue().Stage != FFootballEffectRequest::EStage::Prepared) return false;
        if (Captured.Kind == EFootballEffect::Open) { FinishOpen(Captured); return true; }
        if (Captured.Kind == EFootballEffect::Pulse || Captured.Kind == EFootballEffect::Release)
        { FinishEffect(Captured, FootballPrivate::ClockMs(FDateTime::UtcNow())); return true; }
        EffectSlot(Captured.Kind)->Reset();
        // Unsent input is cancelled locally, never converted to a fictional failure.
        if (Captured.Kind == EFootballEffect::Input) ResetInput();
        if (Captured.Kind == EFootballEffect::Neutral && Captured.QueryGeneration == MatchGeneration
            && View.Match.IsSet() && Captured.Epoch == View.Match->Epoch && FootballPrivate::Same(Captured.MatchId, View.Match->Id)) bNeutralPending = true;
        SyncPump();
        return true;
    }
    bool FFootballReadState::ClaimEffect(const FFootballEffectReceipt& Receipt)
    {
        if (bFinalizing || !OwnsEffect(Receipt)) return false;
        auto& Request = EffectSlot(Receipt.Kind)->GetValue();
        if (Request.Stage != FFootballEffectRequest::EStage::Dispatched) return false;
        Request.Stage = FFootballEffectRequest::EStage::Completing;
        return true;
    }
    void FFootballReadState::FinishEffect(const FFootballEffectReceipt& Receipt, double NowMs)
    {
        if (!OwnsEffect(Receipt)) return;
        EffectSlot(Receipt.Kind)->Reset();
        if (Receipt.Kind == EFootballEffect::Pulse || Receipt.Kind == EFootballEffect::Release)
        {
            if (bReleasePending) { bReleasePending = false; PreparePulse(true, NowMs); }
        }
        else SyncPump();
    }
    void FFootballReadState::PrepareFlush(double NowMs)
    {
        using namespace FootballPrivate;
        if (bDisposed || !Playing() || !ValidScope(Selection) || !SafeInteger(View.Match->Epoch) || !FMath::IsFinite(NowMs)) return;
        const bool bNeutral = bNeutralPending;
        // This branch deliberately precedes both in-flight and rate guards.
        if (!bNeutral)
        {
            if (InputEffect.IsSet() || NeutralEffect.IsSet() || (LastSendAtMs.IsSet() && NowMs - LastSendAtMs.GetValue() < InputMs)) return;
            const auto* Player = CurrentPlayer(View);
            const bool bAction = bActive && Actions.Num() > 0 && (Player ? Player->LastSequence : 0) >= ActionSequence;
            const bool bRenew = bActive && Moving(Latest) && (!LastSendAtMs.IsSet() || NowMs - LastSendAtMs.GetValue() >= InputRenewMs);
            if (!(bActive && (bDirty || bAction || bRenew))) return;
            FFootballInput Next = bAction ? Actions[0] : Latest;
            if (bAction) Actions.RemoveAt(0);
            Next.Sequence = ++Sequence;
            if (!ValidInput(Next, View.Match->Pitch)) return;
            FFootballEffectRequest Request; Request.Receipt = NewEffect(EFootballEffect::Input);
            auto Args = MakeShared<FJsonObject>(); Args->SetStringField(TEXT("roomId"), Selection.RoomId);
            Args->SetNumberField(TEXT("epoch"), View.Match->Epoch); Args->SetObjectField(TEXT("input"), InputJson(Next));
            if (!Json(Args, Request.ArgumentsJson)) return;
            bDirty = false; bSentMovement = Moving(Next);
            if (Next.Action != EFootballAction::None) ActionSequence = Next.Sequence;
            InputEffect = MoveTemp(Request); LastSendAtMs = NowMs;
        }
        else
        {
            FFootballInput Next; Next.CancelActions = true; Next.Sequence = Sequence + 1;
            if (!ValidInput(Next, View.Match->Pitch)) return;
            FFootballEffectRequest Request; Request.Receipt = NewEffect(EFootballEffect::Neutral);
            auto Args = MakeShared<FJsonObject>(); Args->SetStringField(TEXT("roomId"), Selection.RoomId);
            Args->SetNumberField(TEXT("epoch"), View.Match->Epoch); Args->SetObjectField(TEXT("input"), InputJson(Next));
            if (!Json(Args, Request.ArgumentsJson)) return;
            Sequence = Next.Sequence; bNeutralPending = false; bSentMovement = false; ActionSequence = 0;
            NeutralEffect = MoveTemp(Request); LastSendAtMs = NowMs;
        }
    }
    void FFootballReadState::PreparePulse(bool bRelease, double NowMs)
    {
        using namespace FootballPrivate;
        if (bDisposed || !View.Match.IsSet() || !ValidScope(Selection) || !SafeInteger(View.Match->Epoch) || !FMath::IsFinite(NowMs)) return;
        if (PulseEffect.IsSet()) { bReleasePending |= bRelease; return; }
        if (!bRelease)
        {
            if (!bForeground || !Playing() || (LastPulseAtMs.IsSet() && NowMs - LastPulseAtMs.GetValue() < TickMs)) return;
            const auto NextAt = View.Advancement.IsSet() ? View.Advancement->NextAtMs : NextAdvanceAt(View.Match.GetValue());
            if (!NextAt.IsSet()) return;
            int32 Ordinal = 0, Peers = 0;
            for (const auto& P : View.Match->Players) if (!P.bBot && P.bConnected)
            {
                if (SameId(View.PlayerId, P.Id)) { Ordinal = Peers; break; }
                ++Peers;
            }
            const bool bOwn = View.Advancement.IsSet() && SameNullableId(View.Advancement->OwnerId, View.PlayerId);
            const double Due = bOwn ? NextAt.GetValue() : FMath::Max(NextAt.GetValue(), View.Advancement.IsSet() ? View.Advancement->OwnerUntilMs : 0) + Ordinal * TickMs;
            if (NowMs + ServerOffsetMs < Due) return;
        }
        else if (!View.Advancement.IsSet() || !SameNullableId(View.Advancement->OwnerId, View.PlayerId)) return;
        FFootballEffectRequest Request; Request.Receipt = NewEffect(bRelease ? EFootballEffect::Release : EFootballEffect::Pulse);
        auto Args = MakeShared<FJsonObject>(); Args->SetStringField(TEXT("roomId"), Selection.RoomId);
        Args->SetNumberField(TEXT("epoch"), View.Match->Epoch); Args->SetBoolField(TEXT("release"), bRelease);
        if (!Json(Args, Request.ArgumentsJson)) return;
        PulseEffect = MoveTemp(Request); LastPulseAtMs = NowMs;
    }
    void FFootballReadState::Pump(FDateTime Now)
    {
        if (bDisposed || bFinalizing || !bPumpRunning) return;
        PrepareFlush(FootballPrivate::ClockMs(Now)); PreparePulse(false, FootballPrivate::ClockMs(Now));
    }
    bool FFootballReadState::Input(const FFootballInput& Value, FDateTime Now)
    {
        if (bDisposed || bFinalizing || !bActive || !Playing()) return false;
        FFootballInput Next = Value; Next.Sequence = Sequence + 1;
        if (!FootballPrivate::ValidInput(Next, View.Match->Pitch)) return false;
        Next.Action = EFootballAction::None; Next.Sequence = 0;
        bDirty |= !FootballPrivate::SameInput(Next, Latest); Latest = Next;
        if (Value.Action != EFootballAction::None && Actions.Num() < 2)
        { Next.Action = Value.Action; Actions.Add(MoveTemp(Next)); }
        SyncPump(); PrepareFlush(FootballPrivate::ClockMs(Now)); return true;
    }
    void FFootballReadState::SetActive(bool bValue, FDateTime Now)
    {
        if (bDisposed || bFinalizing || bActive == bValue) return;
        ResetInput(); bActive = bValue; SyncPump(); PrepareFlush(FootballPrivate::ClockMs(Now));
    }
    void FFootballReadState::SetForeground(bool bValue, FDateTime Now)
    {
        if (bDisposed || bFinalizing || bForeground == bValue) return;
        bForeground = bValue;
        if (!bValue)
        {
            bActive = false; ResetInput(); PrepareFlush(FootballPrivate::ClockMs(Now)); PreparePulse(true, FootballPrivate::ClockMs(Now));
        }
        SyncPump(); if (bValue) PreparePulse(false, FootballPrivate::ClockMs(Now));
    }
    void FFootballReadState::Stop(FDateTime Now)
    {
        if (bDisposed || bFinalizing) return;
        ResetInput(); SyncPump(); PrepareFlush(FootballPrivate::ClockMs(Now));
    }
    void FFootballReadState::PrepareFinalEffects()
    {
        bActive = false; bForeground = false; ResetInput();
        const double NowMs = FootballPrivate::ClockMs(FDateTime::UtcNow());
        PrepareFlush(NowMs); PreparePulse(true, NowMs);
        TArray<FFootballEffectRequest> Effects;
        for (auto* Slot : { &NeutralEffect, &PulseEffect })
            if (Slot->IsSet() && Slot->GetValue().Stage == FFootballEffectRequest::EStage::Prepared
                && Slot->GetValue().Receipt.Kind != EFootballEffect::Pulse)
            { Slot->GetValue().bFinal = true; Effects.Add(Slot->GetValue()); }
        bFinalizing = true; bPumpRunning = false;
        // Copy the delegate: rebinding from the adapter cannot invalidate this call.
        const FFootballFinalEffects Dispatch = FinalEffects;
        Dispatch.ExecuteIfBound(Effects);
        bFinalizing = false;
        // Final packets only have authority inside the synchronous window above.
        for (auto* Slot : { &NeutralEffect, &PulseEffect })
            if (Slot->IsSet() && Slot->GetValue().bFinal && Slot->GetValue().Stage == FFootballEffectRequest::EStage::Prepared) Slot->Reset();
    }
    EFootballReadResult FFootballReadState::AcceptInputSuccess(const FFootballEffectReceipt& Receipt, const FString& Json)
    {
        const FFootballEffectReceipt Captured = Receipt;
        if ((Captured.Kind != EFootballEffect::Input && Captured.Kind != EFootballEffect::Neutral) || !ClaimEffect(Captured)) return EFootballReadResult::StaleReceipt;
        EFootballReadResult Result = EFootballReadResult::StaleReceipt;
        if (CurrentEffect(Captured))
        {
            StateJsonPrivate::FObject Root; double AckSequence = 0, Revision = 0, Epoch = 0;
            if (!StateJsonPrivate::Parse(Json, Root, FootballPrivate::Limits)
                || !StateJsonPrivate::Only(Root, { TEXT("sequence"), TEXT("revision"), TEXT("epoch") })
                || !StateJsonPrivate::Number(Root, TEXT("sequence"), AckSequence) || !StateJsonPrivate::Number(Root, TEXT("revision"), Revision)
                || !StateJsonPrivate::Number(Root, TEXT("epoch"), Epoch))
            { FailMatch(FootballPrivate::InvalidMatch); Result = EFootballReadResult::InvalidResponse; }
            else if (Epoch == Captured.Epoch) { Sequence = FMath::Max(Sequence, AckSequence); Result = EFootballReadResult::Accepted; }
        }
        FinishEffect(Captured, FootballPrivate::ClockMs(FDateTime::UtcNow()));
        return Result;
    }
    EFootballReadResult FFootballReadState::AcceptPulseSuccess(const FFootballEffectReceipt& Receipt, const FString& Json, FDateTime ReceivedAt)
    {
        const FFootballEffectReceipt Captured = Receipt;
        if ((Captured.Kind != EFootballEffect::Pulse && Captured.Kind != EFootballEffect::Release) || !ClaimEffect(Captured)) return EFootballReadResult::StaleReceipt;
        EFootballReadResult Result = EFootballReadResult::StaleReceipt;
        const double NowMs = FootballPrivate::ClockMs(ReceivedAt);
        if (CurrentEffect(Captured))
        {
            TOptional<FFootballSnapshot> Next; FString Error;
            if (!DecodeFootballSnapshot(Json, Next, Error))
            {
                if (Captured.Kind != EFootballEffect::Release) FailMatch(Error);
                Result = EFootballReadResult::InvalidResponse;
            }
            else
            {
                const auto Advancement = Next->Advancement;
                if (Advancement.IsSet()) ServerOffsetMs = Advancement->ServerNowMs - NowMs;
                Result = ApplyMatch(MoveTemp(Next.GetValue()));
                if (OwnsEffect(Captured) && Captured.QueryGeneration == MatchGeneration && !bForeground
                    && Advancement.IsSet() && FootballPrivate::SameNullableId(Advancement->OwnerId, View.PlayerId)) bReleasePending = true;
            }
        }
        FinishEffect(Captured, NowMs);
        return Result;
    }
    bool FFootballReadState::AcceptEffectFailure(const FFootballEffectReceipt& Receipt, const FFootballPublicReadFailure& Failure)
    {
        const FFootballEffectReceipt Captured = Receipt;
        if (Captured.Kind == EFootballEffect::Open || !ClaimEffect(Captured)) return false;
        const bool bCurrent = CurrentEffect(Captured);
        if (bCurrent && Captured.Kind != EFootballEffect::Release)
            FailMatch(Failure.Message.IsSet() ? Failure.Message.GetValue() : FString(FootballPrivate::Disconnected));
        FinishEffect(Captured, FootballPrivate::ClockMs(FDateTime::UtcNow()));
        return bCurrent;
    }
    bool FFootballReadState::BeginEntryObservation(int64 CapturedNowMs, FFootballReadReceipt& Out)
    {
        Out = FFootballReadReceipt{};
        if (!CanObserve(CapturedNowMs)) return false;
        Out.Generation = FGuid::NewGuid();
        Out.Scope = Selection;
        Out.CapturedNowMs = CapturedNowMs;
        Out.bEntry = true;
        EntryGenerations.Add(Out.Generation);
        return true;
    }
    void FFootballReadState::EndEntryObservation(const FFootballReadReceipt& Receipt)
    {
        if (CurrentEntry(Receipt)) EntryGenerations.Remove(Receipt.Generation);
    }
    EFootballReadResult FFootballReadState::AcceptEntry(const FFootballReadReceipt& Receipt, const FString& Json,
        TOptional<FFootballEntryRoster>& Out, FString& OutError) const
    {
        Out.Reset();
        OutError.Reset();
        if (!CurrentEntry(Receipt)) return EFootballReadResult::StaleReceipt;
        return DecodeFootballEntryRoster(Json, Out, OutError) ? EFootballReadResult::Accepted : EFootballReadResult::InvalidResponse;
    }
    bool FFootballReadState::AcceptEntryFailure(const FFootballReadReceipt& Receipt, const FFootballPublicReadFailure& Failure,
        FString& OutError) const
    {
        OutError.Reset();
        if (!CurrentEntry(Receipt)) return false;
        OutError = Failure.Message.IsSet() ? Failure.Message.GetValue() : FString(FootballPrivate::InvalidEntry);
        return true;
    }
    void FFootballReadState::Clear()
    {
        if (bDisposed || bFinalizing) return;
        ResetSession(FFootballReadScope{});
    }
    void FFootballReadState::Dispose()
    {
        if (bDisposed || bFinalizing) return;
        PrepareFinalEffects();
        bDisposed = true;
        InvalidateMatchObservation();
        ResetLifecycle();
        RetireCommand(); SessionGeneration.Invalidate();
        EntryGenerations.Reset();
        Selection = FFootballReadScope{};
        View = FFootballSnapshot{};
        bHasApplied = false;
        Changed.Clear();
        FinalEffects.Unbind();
    }
}
