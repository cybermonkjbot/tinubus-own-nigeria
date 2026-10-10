#include "BaddieCompanionWorldPlayer.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#include <cmath>
#include <initializer_list>

namespace BaddieCompanion
{
    namespace WorldPlayerPrivate
    {
        constexpr int64 MaxInteger = 9007199254740991LL;
        constexpr int32 MaxDocumentCharacters = 2 * 1024 * 1024;
        constexpr int32 MaxLayoutIdentityCharacters = 160000;
        using FObject = TSharedPtr<FJsonObject>;
        using FValue = TSharedPtr<FJsonValue>;

        bool Same(const FString& A, const FString& B)
        {
            return A.Equals(B, ESearchCase::CaseSensitive);
        }

        // Bound nesting and token count before UE allocates the JSON tree. String
        // escapes matter here; braces inside a JSON string are not containers.
        bool Parse(const FString& Json, FObject& Out)
        {
            if (Json.IsEmpty() || Json.Len() > MaxDocumentCharacters) return false;
            bool bString = false, bEscape = false;
            int32 Depth = 0, Separators = 0;
            for (const TCHAR C : Json)
            {
                if (bString)
                {
                    if (bEscape) bEscape = false;
                    else if (C == TEXT('\\')) bEscape = true;
                    else if (C == TEXT('"')) bString = false;
                    continue;
                }
                if (C == TEXT('"')) bString = true;
                else if (C == TEXT('{') || C == TEXT('[')) { if (++Depth > 32) return false; }
                else if (C == TEXT('}') || C == TEXT(']')) { if (--Depth < 0) return false; }
                else if (C == TEXT(',') || C == TEXT(':')) { if (++Separators > 131072) return false; }
            }
            if (bString || Depth != 0) return false;
            return FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Out) && Out.IsValid();
        }

        FObject Object(const FValue& Value)
        {
            return Value.IsValid() && Value->Type == EJson::Object ? Value->AsObject() : nullptr;
        }
        FValue Field(const FObject& Value, const TCHAR* Key)
        {
            return Value.IsValid() ? Value->TryGetField(Key) : nullptr;
        }
        bool Number(const FValue& Value, double& Out)
        {
            return Value.IsValid() && Value->Type == EJson::Number && Value->TryGetNumber(Out)
                && std::isfinite(Out);
        }
        bool Number(const FObject& Value, const TCHAR* Key, double& Out)
        {
            return Number(Field(Value, Key), Out);
        }
        bool Integer(const FValue& Value, int64& Out, int64 Maximum = MaxInteger)
        {
            double N = 0;
            if (!Number(Value, N) || N < 0 || N > Maximum || std::floor(N) != N) return false;
            Out = static_cast<int64>(N);
            return true;
        }
        bool Integer(const FObject& Value, const TCHAR* Key, int64& Out, int64 Maximum = MaxInteger)
        {
            return Integer(Field(Value, Key), Out, Maximum);
        }
        // Journey/activity times are v.number(), including fractional legacy timing.
        bool Time(const FValue& Value, double& Out)
        {
            return Number(Value, Out) && Out >= 0 && Out <= MaxInteger;
        }
        bool Time(const FObject& Value, const TCHAR* Key, double& Out)
        {
            return Time(Field(Value, Key), Out);
        }
        bool Key(const FValue& Value, FString& Out, int32 Maximum = 240)
        {
            if (!Value.IsValid() || Value->Type != EJson::String || !Value->TryGetString(Out)
                || Out.IsEmpty() || Out.Len() > Maximum || !Same(Out, Out.TrimStartAndEnd())) return false;
            for (const TCHAR C : Out) if (C < 32) return false;
            return true;
        }
        bool Key(const FObject& Value, const TCHAR* Name, FString& Out, int32 Maximum = 240)
        {
            return Key(Field(Value, Name), Out, Maximum);
        }
        bool Boolean(const FValue& Value, bool& Out)
        {
            return Value.IsValid() && Value->Type == EJson::Boolean && Value->TryGetBool(Out);
        }
        template <typename T> bool Enum(const FValue& Value, T& Out,
            std::initializer_list<TPair<const TCHAR*, T>> Choices)
        {
            FString Name;
            if (!Key(Value, Name)) return false;
            for (const auto& Choice : Choices)
                if (Name.Equals(Choice.Key, ESearchCase::CaseSensitive)) { Out = Choice.Value; return true; }
            return false;
        }
        template <typename T, typename F> bool Optional(const FObject& Parent, const TCHAR* Name,
            TWorldPlayerField<T>& Out, bool bRequired, bool bNullable, F Read)
        {
            const auto Value = Field(Parent, Name);
            if (!Value.IsValid()) return !bRequired;
            if (Value->Type == EJson::Null)
            {
                Out.Presence = EWorldFieldPresence::Null;
                return bNullable;
            }
            T Next;
            if (!Read(Value, Next)) return false;
            Out.Presence = EWorldFieldPresence::Value;
            Out.Value = MoveTemp(Next);
            return true;
        }
        bool ObjectPresence(const FObject& Parent, const TCHAR* Name, EWorldFieldPresence& Out,
            bool bRequired, bool bNullable)
        {
            const auto Value = Field(Parent, Name);
            if (!Value.IsValid()) return !bRequired;
            if (Value->Type == EJson::Null) { Out = EWorldFieldPresence::Null; return bNullable; }
            if (!Object(Value).IsValid()) return false;
            Out = EWorldFieldPresence::Value;
            return true;
        }
        bool OptionalKey(const FObject& Parent, const TCHAR* Name, TWorldPlayerField<FString>& Out,
            bool bRequired = false, bool bNullable = false)
        {
            return Optional(Parent, Name, Out, bRequired, bNullable,
                [](const FValue& V, FString& S) { return Key(V, S); });
        }
        bool Pose(const FValue& Value, FMovementPose& Out)
        {
            const auto O = Object(Value);
            return Number(O, TEXT("x"), Out.X) && Number(O, TEXT("z"), Out.Z)
                && Number(O, TEXT("yaw"), Out.YawRadians);
        }
        bool Geo(const FValue& Value, double& Latitude, double& Longitude)
        {
            const auto O = Object(Value);
            return Number(O, TEXT("latitude"), Latitude) && std::abs(Latitude) <= 90
                && Number(O, TEXT("longitude"), Longitude) && std::abs(Longitude) <= 180;
        }
        FString PlaceId(const FWorldTravelEndpoint& Endpoint)
        {
            return Endpoint.Kind == EWorldEndpointKind::Road ? TEXT("roadside:") + Endpoint.Id : Endpoint.Id;
        }
        bool Endpoint(const FValue& Value, FWorldTravelEndpoint& Out)
        {
            const auto O = Object(Value);
            if (!Enum(Field(O, TEXT("kind")), Out.Kind,
                {{TEXT("place"), EWorldEndpointKind::Place}, {TEXT("road"), EWorldEndpointKind::Road}})
                || !Key(O, TEXT("id"), Out.Id, Out.Kind == EWorldEndpointKind::Road ? 85 : 128)
                || !Geo(Field(O, TEXT("point")), Out.Latitude, Out.Longitude)) return false;
            if (Out.Kind == EWorldEndpointKind::Place)
                return !O->HasField(TEXT("vehicleId")) && !O->HasField(TEXT("vehicleRevision"));
            return Optional(O, TEXT("vehicleId"), Out.VehicleId, true, true,
                    [](const FValue& V, FString& S) { return Key(V, S, 160); })
                && Optional(O, TEXT("vehicleRevision"), Out.VehicleRevision, true, true,
                    [](const FValue& V, int64& N) { return Integer(V, N, MaxInteger - 1); })
                && Out.VehicleId.Presence == Out.VehicleRevision.Presence;
        }
        bool Route(const FValue& Value, FWorldJourneyRouteIdentity& Out)
        {
            const auto O = Object(Value);
            FString Placement;
            if (!Key(O, TEXT("id"), Out.Id) || !Key(O, TEXT("placement"), Placement)
                || !Enum(Field(O, TEXT("version")), Out.Kind,
                    {{TEXT("lagos-corridors-v1"), EWorldJourneyRouteKind::Corridor},
                     {TEXT("mapbox-directions-v5"), EWorldJourneyRouteKind::RoadNetwork}})
                || !Number(O, TEXT("lengthMetres"), Out.LengthMetres) || Out.LengthMetres < 0) return false;
            const TArray<FValue>* Points = nullptr;
            if (!O->TryGetArrayField(TEXT("points"), Points) || !Points || Points->Num() < 2 || Points->Num() > 4096) return false;
            Out.PointCount = Points->Num();
            for (const auto& Point : *Points)
            {
                double Latitude = 0, Longitude = 0;
                if (!Geo(Point, Latitude, Longitude)) return false;
            }
            if (Out.Kind == EWorldJourneyRouteKind::Corridor)
                return Same(Placement, TEXT("proposed")) && !O->HasField(TEXT("segmentDurationsMs")) && !O->HasField(TEXT("profile"));
            const TArray<FValue>* Durations = nullptr;
            if (!Same(Placement, TEXT("road-network")) || !O->TryGetArrayField(TEXT("segmentDurationsMs"), Durations)
                || !Durations || Durations->Num() != Points->Num() - 1) return false;
            for (const auto& Duration : *Durations)
            {
                double N = 0;
                if (!Time(Duration, N)) return false;
            }
            return Optional(O, TEXT("profile"), Out.Profile, false, false,
                [](const FValue& V, EWorldJourneyProfile& P) { return Enum(V, P,
                    {{TEXT("driving-traffic"), EWorldJourneyProfile::DrivingTraffic}, {TEXT("walking"), EWorldJourneyProfile::Walking}}); });
        }
        bool Checkpoint(const FValue& Value, FWorldJourneyCheckpoint& Out)
        {
            const auto O = Object(Value);
            return Number(O, TEXT("progress"), Out.Progress) && Out.Progress >= 0 && Out.Progress <= 1
                && Time(O, TEXT("startsAt"), Out.StartsAtMs) && Time(O, TEXT("endsAt"), Out.EndsAtMs)
                && Out.EndsAtMs >= Out.StartsAtMs
                && Optional(O, TEXT("resumedAt"), Out.ResumedAtMs, false, true,
                    [](const FValue& V, double& N) { return Time(V, N); })
                && ObjectPresence(O, TEXT("enforcement"), Out.Enforcement, false, false);
        }
        bool JourneyTiming(const FValue& Value, FWorldJourneyTiming& Out)
        {
            const auto O = Object(Value);
            return Integer(O, TEXT("rate"), Out.Rate, 4) && (Out.Rate == 1 || Out.Rate == 4)
                && Time(O, TEXT("anchorAt"), Out.AnchorAtMs)
                && Number(O, TEXT("anchorProgress"), Out.AnchorProgress) && Out.AnchorProgress >= 0 && Out.AnchorProgress <= 1
                && Time(O, TEXT("durationMs"), Out.DurationMs) && Out.DurationMs > 0;
        }
        bool Journey(const FValue& Value, FWorldJourneyView& Out)
        {
            const auto O = Object(Value);
            if (!Key(O, TEXT("id"), Out.Id) || !Key(O, TEXT("requestId"), Out.RequestId)
                || !Key(O, TEXT("fromPlaceId"), Out.FromPlaceId) || !Key(O, TEXT("toPlaceId"), Out.ToPlaceId)
                || !Route(Field(O, TEXT("route")), Out.Route)
                || !Enum(Field(O, TEXT("status")), Out.Status, {{TEXT("traveling"), EWorldJourneyStatus::Traveling},
                    {TEXT("arrived"), EWorldJourneyStatus::Arrived}, {TEXT("interrupted"), EWorldJourneyStatus::Interrupted}})
                || !Enum(Field(O, TEXT("vehicle")), Out.Vehicle, {{TEXT("car"), EWorldJourneyVehicle::Car},
                    {TEXT("okada"), EWorldJourneyVehicle::Okada}, {TEXT("keke"), EWorldJourneyVehicle::Keke},
                    {TEXT("danfo"), EWorldJourneyVehicle::Danfo}, {TEXT("canoe"), EWorldJourneyVehicle::Canoe},
                    {TEXT("speedboat"), EWorldJourneyVehicle::Speedboat}})
                || !Time(O, TEXT("startedAt"), Out.StartedAtMs) || !Time(O, TEXT("arrivesAt"), Out.ArrivesAtMs)
                || Out.ArrivesAtMs < Out.StartedAtMs
                || !Optional(O, TEXT("checkpoint"), Out.Checkpoint, true, true, Checkpoint)
                || !Optional(O, TEXT("travelTiming"), Out.TravelTiming, false, false, JourneyTiming)
                || !Optional(O, TEXT("fromEndpoint"), Out.FromEndpoint, false, false, Endpoint)
                || !Optional(O, TEXT("toEndpoint"), Out.ToEndpoint, false, false, Endpoint)
                || !Optional(O, TEXT("travelMode"), Out.TravelMode, false, false,
                    [](const FValue& V, EWorldTravelMode& M) { return Enum(V, M, {{TEXT("walk"), EWorldTravelMode::Walk},
                        {TEXT("vehicle"), EWorldTravelMode::Vehicle}, {TEXT("boat"), EWorldTravelMode::Boat}}); })
                || !ObjectPresence(O, TEXT("traffic"), Out.Traffic, false, false)) return false;
            return (!Out.FromEndpoint.Value.IsSet() || Same(PlaceId(Out.FromEndpoint.Value.GetValue()), Out.FromPlaceId))
                && (!Out.ToEndpoint.Value.IsSet() || Same(PlaceId(Out.ToEndpoint.Value.GetValue()), Out.ToPlaceId));
        }
        bool Roadside(const FValue& Value, FWorldRoadsideVisit& Out)
        {
            const auto O = Object(Value), Exit = Object(Field(O, TEXT("exit")));
            return Endpoint(Field(O, TEXT("endpoint")), Out.Endpoint) && Out.Endpoint.Kind == EWorldEndpointKind::Road
                && Key(O, TEXT("sourceJourneyId"), Out.SourceJourneyId) && Time(O, TEXT("stoppedAt"), Out.StoppedAtMs)
                && Key(Exit, TEXT("placeId"), Out.ExitPlaceId) && Route(Field(Exit, TEXT("route")), Out.ExitRoute);
        }
        bool Transit(const FValue& Value, FWorldTransitRide& Out)
        {
            const auto O = Object(Value);
            return Key(O, TEXT("id"), Out.Id) && Key(O, TEXT("vehicleId"), Out.VehicleId)
                && Integer(O, TEXT("seat"), Out.Seat) && Integer(O, TEXT("visitRevision"), Out.VisitRevision)
                && Time(O, TEXT("boardedAt"), Out.BoardedAtMs)
                && Optional(O, TEXT("exit"), Out.Exit, true, true, [](const FValue& V, FWorldTransitExit& E)
                {
                    const auto Exit = Object(V);
                    return Time(Exit, TEXT("at"), E.AtMs) && Pose(Field(Exit, TEXT("point")), E.Point)
                        && OptionalKey(Exit, TEXT("stopId"), E.StopId, true, true);
                });
        }
        bool Nightclub(const FValue& Value, FWorldNightclubActivity& Out)
        {
            const auto O = Object(Value);
            if (!Key(O, TEXT("id"), Out.Id) || !Integer(O, TEXT("visitRevision"), Out.VisitRevision)
                || !Enum(Field(O, TEXT("kind")), Out.Kind,
                    {{TEXT("dance"), EWorldNightclubKind::Dance}, {TEXT("lounge"), EWorldNightclubKind::Lounge}})) return false;
            if (Out.Kind == EWorldNightclubKind::Lounge)
                return Key(O, TEXT("serviceId"), Out.ServiceId) && Time(O, TEXT("joinedAt"), Out.JoinedAtMs)
                    && OptionalKey(O, TEXT("seatId"), Out.SeatId);
            const auto R = Object(Field(O, TEXT("round")));
            auto& Round = Out.Round;
            const TArray<FValue>* Lanes = nullptr;
            const TArray<FValue>* Inputs = nullptr;
            if (!Key(R, TEXT("id"), Round.Id) || !Same(Round.Id, Out.Id) || !Key(R, TEXT("instanceId"), Round.InstanceId)
                || !Key(R, TEXT("serviceId"), Round.ServiceId) || !Time(R, TEXT("startsAt"), Round.StartsAtMs)
                || !Number(R, TEXT("beatMs"), Round.BeatMs) || Round.BeatMs < 250 || Round.BeatMs > 2000
                || !Number(R, TEXT("windowMs"), Round.WindowMs) || Round.WindowMs < 30 || Round.WindowMs >= Round.BeatMs / 2
                || !Integer(R, TEXT("requiredHits"), Round.RequiredHits, 24)
                || !R->TryGetArrayField(TEXT("lanes"), Lanes) || !Lanes || Lanes->IsEmpty() || Lanes->Num() > 24
                || Round.RequiredHits < 1 || Round.RequiredHits > Lanes->Num()
                || !O->TryGetArrayField(TEXT("inputs"), Inputs) || !Inputs || Inputs->Num() > 24) return false;
            for (const auto& Lane : *Lanes)
            {
                int64 N = 0;
                if (!Integer(Lane, N, 3)) return false;
                Round.Lanes.Add(N);
            }
            for (const auto& Input : *Inputs)
            {
                const auto I = Object(Input);
                FWorldDanceInput Next;
                if (!Key(I, TEXT("userId"), Next.UserId) || !Integer(I, TEXT("beatIndex"), Next.BeatIndex, Lanes->Num() - 1)
                    || !Integer(I, TEXT("lane"), Next.Lane, 3) || !Time(I, TEXT("receivedAt"), Next.ReceivedAtMs)) return false;
                Out.Inputs.Add(MoveTemp(Next));
            }
            return Optional(O, TEXT("stoppedAt"), Out.StoppedAtMs, true, true,
                    [](const FValue& V, double& N) { return Time(V, N); })
                && Optional(O, TEXT("difficulty"), Out.Difficulty, false, false,
                    [](const FValue& V, EWorldDanceDifficulty& D) { return Enum(V, D,
                        {{TEXT("easy"), EWorldDanceDifficulty::Easy}, {TEXT("standard"), EWorldDanceDifficulty::Standard},
                         {TEXT("hard"), EWorldDanceDifficulty::Hard}}); })
                && OptionalKey(O, TEXT("skillCharacterId"), Out.SkillCharacterId)
                && Optional(O, TEXT("skillsSettled"), Out.SkillsSettled, false, false, Boolean);
        }
        bool Residence(const FValue& Value, FWorldResidenceIdentity& Out)
        {
            const auto O = Object(Value);
            return Key(O, TEXT("cityVersion"), Out.CityVersion) && Key(O, TEXT("parcelId"), Out.ParcelId)
                && Integer(O, TEXT("unit"), Out.Unit) && Out.Unit > 0;
        }
        bool PlayerFields(const FObject& O)
        {
            // The public projection has no storage-only/session fields. Reject added
            // top-level fields until their effect on control ownership is reviewed.
            for (const auto& Pair : O->Values)
            {
                bool bKnown = false;
                for (const TCHAR* Name : {TEXT("userId"), TEXT("characterId"), TEXT("gameRoomId"), TEXT("placeId"),
                    TEXT("instanceId"), TEXT("homeRoomId"), TEXT("revision"), TEXT("moveSequence"), TEXT("position"),
                    TEXT("movement"), TEXT("status"), TEXT("health"), TEXT("journey"), TEXT("roadside"),
                    TEXT("transitRide"), TEXT("driving"), TEXT("waterControl"), TEXT("nightclub"), TEXT("residence"), TEXT("drawnWeaponId")})
                    if (Same(Pair.Key, Name)) { bKnown = true; break; }
                if (!bKnown) return false;
            }
            FString Ignored;
            for (const TCHAR* Name : {TEXT("userId"), TEXT("characterId"), TEXT("gameRoomId"), TEXT("placeId"), TEXT("instanceId"), TEXT("homeRoomId")})
                if (!Key(O, Name, Ignored)) return false;
            return true;
        }
        bool ConsistentPlayer(FWorldPlayerView& P)
        {
            auto& C = P.Controls;
            C.bRecovering = P.Status == EWorldPlayerStatus::Recovering;
            C.bHealthDepleted = P.Health <= 0;
            C.bTraveling = P.Status == EWorldPlayerStatus::Traveling
                || (P.Journey.Value.IsSet() && P.Journey.Value.GetValue().Status == EWorldJourneyStatus::Traveling);
            if (P.Status == EWorldPlayerStatus::Traveling && (!P.Journey.Value.IsSet()
                || P.Journey.Value.GetValue().Status != EWorldJourneyStatus::Traveling)) return false;
            if (P.TransitRide.Value.IsSet() && (P.Status != EWorldPlayerStatus::Present
                || P.TransitRide.Value.GetValue().VisitRevision != P.Revision)) return false;
            if (P.Nightclub.Value.IsSet())
            {
                const auto& N = P.Nightclub.Value.GetValue();
                if (P.Status != EWorldPlayerStatus::Present || !Same(P.PlaceId, TEXT("nightclub")) || N.VisitRevision != P.Revision
                    || (N.Kind == EWorldNightclubKind::Dance && !Same(N.Round.InstanceId, P.InstanceId))) return false;
                C.bDanceActivity = N.Kind == EWorldNightclubKind::Dance;
                C.bLoungeSeat = N.Kind == EWorldNightclubKind::Lounge && N.SeatId.Value.IsSet();
            }
            C.bDriving = P.Driving.Value.IsSet();
            C.bActiveWater = P.WaterControl.Value.IsSet() && P.WaterControl.Value.GetValue().Phase == EWorldWaterPhase::Active;
            C.bTransit = P.TransitRide.Value.IsSet();
            C.bMissingControlFields = P.Driving.Presence == EWorldFieldPresence::Missing
                || P.WaterControl.Presence == EWorldFieldPresence::Missing
                || P.TransitRide.Presence == EWorldFieldPresence::Missing || P.Nightclub.Presence == EWorldFieldPresence::Missing;
            C.bConflictingTransport = (C.bDriving && C.bActiveWater)
                || ((C.bDriving || C.bActiveWater) && (P.Movement.IsSet() || C.bTransit || C.bTraveling));
            if (C.bDriving)
            {
                const auto& D = P.Driving.Value.GetValue();
                C.bTransportIdentityMismatch = P.Status != EWorldPlayerStatus::Present || !Same(D.GameRoomId, P.GameRoomId)
                    || !Same(D.InstanceId, P.InstanceId) || !Same(D.Driver.UserId, P.UserId) || !Same(D.Driver.CharacterId, P.CharacterId);
                for (const auto& Occupant : D.Occupants)
                    C.bTransportIdentityMismatch |= !Same(Occupant.UserId, P.UserId) || !Same(Occupant.CharacterId, P.CharacterId)
                        || !Same(Occupant.LifeId, D.Driver.LifeId);
            }
            if (P.WaterControl.Value.IsSet())
            {
                const auto& S = P.WaterControl.Value.GetValue().Scope;
                C.bTransportIdentityMismatch |= P.Status != EWorldPlayerStatus::Present || !Same(S.GameRoomId, P.GameRoomId)
                    || !Same(S.InstanceId, P.InstanceId) || !Same(S.PlaceId, P.PlaceId) || S.Revision != P.Revision
                    || !Same(S.UserId, P.UserId) || !Same(S.CharacterId, P.CharacterId);
            }
            // Retain mismatched/conflicting transport as an observation, never erase
            // it to null. Current life, previous receipt and instance checks need an owner.
            return true;
        }
        bool SameEndpoint(const FWorldTravelEndpoint& A, const FWorldTravelEndpoint& B)
        {
            return A.Kind == B.Kind && Same(A.Id, B.Id) && A.Latitude == B.Latitude && A.Longitude == B.Longitude
                && A.VehicleId.Presence == B.VehicleId.Presence && A.VehicleRevision.Presence == B.VehicleRevision.Presence
                && A.VehicleId.Value.IsSet() == B.VehicleId.Value.IsSet()
                && A.VehicleRevision.Value.IsSet() == B.VehicleRevision.Value.IsSet()
                && (!A.VehicleId.Value.IsSet() || Same(A.VehicleId.Value.GetValue(), B.VehicleId.Value.GetValue()))
                && (!A.VehicleRevision.Value.IsSet() || A.VehicleRevision.Value.GetValue() == B.VehicleRevision.Value.GetValue());
        }
    }

    bool DecodeWorldPlayerDocument(const FString& PlayerDocumentJson, int64 ServerNowMs,
        TSharedPtr<const FWorldPlayerView>& Out, FString& OutError)
    {
        using namespace WorldPlayerPrivate;
        Out.Reset();
        OutError = TEXT("World player document is invalid or exceeds the decoder bounds.");
        FObject Player;
        if (ServerNowMs < 1 || ServerNowMs > MaxInteger || !Parse(PlayerDocumentJson, Player) || !PlayerFields(Player)) return false;
        const auto Envelope = MakeShared<FJsonObject>();
        Envelope->SetObjectField(TEXT("player"), Player);
        Envelope->SetNumberField(TEXT("serverNow"), ServerNowMs);
        FWorldMovementResult Base;
        if (!DecodeWorldMovementResult(Envelope, Base, OutError)) return false;
        auto Next = MakeShared<FWorldPlayerView>();
        Next->UserId = MoveTemp(Base.UserId); Next->CharacterId = MoveTemp(Base.CharacterId);
        Next->GameRoomId = MoveTemp(Base.GameRoomId); Next->PlaceId = MoveTemp(Base.PlaceId);
        Next->InstanceId = MoveTemp(Base.InstanceId); Next->HomeRoomId = MoveTemp(Base.HomeRoomId);
        Next->Revision = Base.Revision; Next->MoveSequence = Base.MoveSequence; Next->ServerNowMs = Base.ServerNowMs;
        Next->Status = Base.Status; Next->Health = Base.Health; Next->Position = Base.Position;
        Next->Movement = MoveTemp(Base.Movement);
        Next->Driving.Presence = !Base.bHasDrivingField ? EWorldFieldPresence::Missing
            : Base.Driving.IsSet() ? EWorldFieldPresence::Value : EWorldFieldPresence::Null;
        Next->Driving.Value = MoveTemp(Base.Driving);
        Next->WaterControl.Presence = !Base.bHasWaterControlField ? EWorldFieldPresence::Missing
            : Base.WaterControl.IsSet() ? EWorldFieldPresence::Value : EWorldFieldPresence::Null;
        Next->WaterControl.Value = MoveTemp(Base.WaterControl);
        OutError = TEXT("World player journey, visit or activity is invalid.");
        if (!Optional(Player, TEXT("journey"), Next->Journey, true, true, Journey)
            || !Optional(Player, TEXT("roadside"), Next->Roadside, false, true, Roadside)
            || !Optional(Player, TEXT("transitRide"), Next->TransitRide, false, true, Transit)
            || !Optional(Player, TEXT("nightclub"), Next->Nightclub, false, true, Nightclub)
            || !Optional(Player, TEXT("residence"), Next->Residence, false, true, Residence)
            || !Optional(Player, TEXT("drawnWeaponId"), Next->DrawnWeapon, false, true,
                [](const FValue& V, EWorldDrawnWeapon& W) { return Enum(V, W,
                    {{TEXT("bat"), EWorldDrawnWeapon::Bat}, {TEXT("pistol"), EWorldDrawnWeapon::Pistol}}); })
            || !ConsistentPlayer(*Next)) return false;
        Out = Next;
        OutError.Reset();
        return true;
    }

    bool DecodeWorldCurrentInstanceIdentity(const FString& InstanceDocumentJson, const FWorldPlayerView& Player,
        TSharedPtr<const FWorldCurrentInstanceIdentity>& Out, FString& OutError)
    {
        using namespace WorldPlayerPrivate;
        Out.Reset();
        OutError = TEXT("World instance identity is invalid or exceeds the decoder bounds.");
        FObject O;
        if (!Parse(InstanceDocumentJson, O)) return false;
        auto Next = MakeShared<FWorldCurrentInstanceIdentity>();
        if (!Key(O, TEXT("id"), Next->Id) || !Key(O, TEXT("placeId"), Next->PlaceId)
            || !Key(O, TEXT("seed"), Next->Seed) || !Key(O, TEXT("generatorVersion"), Next->GeneratorVersion)
            || !Key(O, TEXT("layoutId"), Next->LayoutId, MaxLayoutIdentityCharacters)
            || !Key(Object(Field(O, TEXT("navigation"))), TEXT("revision"), Next->NavigationRevision, MaxLayoutIdentityCharacters)
            || !Integer(O, TEXT("sharedStartedAt"), Next->SharedStartedAtMs)
            || !Optional(O, TEXT("roadside"), Next->Roadside, false, false, Endpoint)
            || !ObjectPresence(O, TEXT("layout"), Next->Layout, true, true)
            || !ObjectPresence(O, TEXT("room"), Next->Room, true, true)
            || !ObjectPresence(O, TEXT("transitPlan"), Next->TransitPlan, false, false)) return false;
        const FWorldTravelEndpoint* ExpectedEndpoint = nullptr;
        FString ExpectedPlace = Player.PlaceId;
        if (Player.Status == EWorldPlayerStatus::Traveling)
        {
            if (!Player.Journey.Value.IsSet() || Player.Journey.Value.GetValue().Status != EWorldJourneyStatus::Traveling) return false;
            const auto& Journey = Player.Journey.Value.GetValue();
            ExpectedPlace = Journey.ToPlaceId;
            if (Journey.ToEndpoint.Value.IsSet()) ExpectedEndpoint = &Journey.ToEndpoint.Value.GetValue();
            Next->Relation = EWorldInstanceRelation::TravelDestination;
        }
        else if (Player.Roadside.Value.IsSet()) ExpectedEndpoint = &Player.Roadside.Value.GetValue().Endpoint;
        OutError = TEXT("World instance identity does not match the player visit or destination.");
        if (!Same(Next->PlaceId, ExpectedPlace)) return false;
        if (Same(Next->PlaceId, TEXT("own-home")))
        {
            FString RoomId;
            if (Next->Room != EWorldFieldPresence::Value || !Key(Object(Field(O, TEXT("room"))), TEXT("id"), RoomId)
                || !Same(Next->Id, Player.GameRoomId + TEXT(":own-home:") + Player.UserId + TEXT(":") + RoomId)
                || (Next->Relation == EWorldInstanceRelation::CurrentVisit && !Same(RoomId, Player.HomeRoomId))) return false;
            Next->HomeRoomId = MoveTemp(RoomId);
        }
        else if (!Same(Next->Id, Player.GameRoomId + TEXT(":") + ExpectedPlace)) return false;
        if (Next->Relation == EWorldInstanceRelation::CurrentVisit && !Same(Next->Id, Player.InstanceId)) return false;
        if (ExpectedPlace.StartsWith(TEXT("roadside:"), ESearchCase::CaseSensitive))
        {
            if (!ExpectedEndpoint || ExpectedEndpoint->Kind != EWorldEndpointKind::Road
                || !Next->Roadside.Value.IsSet() || !SameEndpoint(*ExpectedEndpoint, Next->Roadside.Value.GetValue())
                || !Same(PlaceId(*ExpectedEndpoint), ExpectedPlace) || !Same(Next->Seed, ExpectedEndpoint->Id)
                || !Same(Next->GeneratorVersion, TEXT("roadside-v1")) || !Same(Next->LayoutId, TEXT("roadside-v1"))
                || !Same(Next->NavigationRevision, TEXT("roadside-v1")) || Next->SharedStartedAtMs != 0
                || Next->Layout != EWorldFieldPresence::Null || Next->Room != EWorldFieldPresence::Null) return false;
        }
        else if (Next->Roadside.Presence != EWorldFieldPresence::Missing) return false;
        if (Next->Relation == EWorldInstanceRelation::CurrentVisit)
        {
            if (Player.Driving.Value.IsSet())
                Next->bTransportLayoutMismatch = !Same(Player.Driving.Value.GetValue().NavigationRevision, Next->NavigationRevision);
            if (Player.WaterControl.Value.IsSet())
            {
                const auto& Scope = Player.WaterControl.Value.GetValue().Scope;
                Next->bTransportLayoutMismatch |= !Same(Scope.LayoutId, Next->LayoutId)
                    || !Same(Scope.LayoutRevision, Next->NavigationRevision);
            }
        }
        Out = Next;
        OutError.Reset();
        return true;
    }
}
