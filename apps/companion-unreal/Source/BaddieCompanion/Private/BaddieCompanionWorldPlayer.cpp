#include "BaddieCompanionWorldPlayer.h"
#include "BaddieCompanionWorldPlayerRead.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#include <cmath>
#include <initializer_list>

namespace BaddieCompanion
{
    namespace WorldReadPrivate
    {
        namespace
        {
            using K = EWorldValueKind;
            template <SIZE_T N> constexpr FFieldRule KeyRule(const TCHAR (&Name)[N], uint64 Max = 240,
                bool bRequired = true, bool bNullable = false)
            { return FFieldRule(Name, K::String, bRequired, bNullable).WithText({1, Max, true, true}); }
            template <SIZE_T N> constexpr FFieldRule TimeRule(const TCHAR (&Name)[N], bool Required = true, bool Nullable = false)
            { return FFieldRule(Name, K::Number, Required, Nullable).WithNumber(0, MaxSafeInteger); }
            template <SIZE_T N> constexpr FFieldRule CounterRule(const TCHAR (&Name)[N], double Max = MaxSafeInteger,
                bool Required = true, bool Nullable = false)
            { return FFieldRule(Name, K::Number, Required, Nullable).WithNumber(0, Max, true); }
            template <SIZE_T N> constexpr FFieldRule ObjectRule(const TCHAR (&Name)[N], bool Required = true, bool Nullable = false)
            { return FFieldRule(Name, K::Object, Required, Nullable); }
            template <SIZE_T N> constexpr FFieldRule ArrayRule(const TCHAR (&Name)[N], uint64 Min = 0, uint64 Max = MAX_uint64,
                bool Required = true)
            { return FFieldRule(Name, K::Array, Required).WithCount(Min, Max); }
            template <SIZE_T N> FDomainSchema RecordDomain(const FRecordSchema& Record, const FChildRule (&Children)[N])
            { return {&Record, Children, N}; }
            FDomainSchema RecordDomain(const FRecordSchema& Record) { return {&Record}; }
            FDomainSchema ArrayDomain(EDomain Element, uint64 Minimum = 0, uint64 Maximum = MAX_uint64)
            {
                FDomainSchema S; S.Kind = K::Array; S.Element = Element; S.MinCount = Minimum; S.MaxCount = Maximum; return S;
            }
            FDomainSchema NumberDomain(double Minimum = -DBL_MAX, double Maximum = DBL_MAX, bool bInteger = false)
            {
                FDomainSchema S; S.Kind = K::Number; S.Minimum = Minimum; S.Maximum = Maximum; S.bInteger = bInteger; return S;
            }
        }

        const FDomainSchema& DomainSchema(EDomain Domain)
        {
            switch (Domain)
            {
            case EDomain::Number: { static const auto S = NumberDomain(); return S; }
            case EDomain::Time: { static const auto S = NumberDomain(0, MaxSafeInteger); return S; }
            case EDomain::Lane: { static const auto S = NumberDomain(0, 3, true); return S; }
            case EDomain::ScenePoint: { static const auto S = ArrayDomain(EDomain::Number, 3, 3); return S; }
            case EDomain::Vertex: { static const auto S = ArrayDomain(EDomain::Number, 2, 2); return S; }
            case EDomain::Edge: { static const auto S = ArrayDomain(EDomain::Time, 2, 2); return S; }
            case EDomain::Points: { static const auto S = ArrayDomain(EDomain::ScenePoint); return S; }
            case EDomain::Vertices: { static const auto S = ArrayDomain(EDomain::Vertex, 3, 8); return S; }
            case EDomain::Edges: { static const auto S = ArrayDomain(EDomain::Edge); return S; }
            case EDomain::Obstacles: { static const auto S = ArrayDomain(EDomain::NavigationObstacle); return S; }
            case EDomain::GeoPoints: { static const auto S = ArrayDomain(EDomain::Geo, 2, 4096); return S; }
            case EDomain::Durations: { static const auto S = ArrayDomain(EDomain::Time); return S; }
            case EDomain::Lanes: { static const auto S = ArrayDomain(EDomain::Lane, 1, 24); return S; }
            case EDomain::DanceInputs: { static const auto S = ArrayDomain(EDomain::DanceInput, 0, 24); return S; }
            case EDomain::Pose:
            {
                static constexpr FFieldRule Rules[] = {FFieldRule(TEXT("x"), K::Number), FFieldRule(TEXT("z"), K::Number), FFieldRule(TEXT("yaw"), K::Number)};
                static constexpr FRecordSchema R(Rules, false); static const auto S = RecordDomain(R); return S;
            }
            case EDomain::Geo:
            {
                static constexpr FFieldRule Rules[] = {FFieldRule(TEXT("latitude"), K::Number).WithNumber(-90, 90),
                    FFieldRule(TEXT("longitude"), K::Number).WithNumber(-180, 180)};
                static constexpr FRecordSchema R(Rules, false); static const auto S = RecordDomain(R); return S;
            }
            case EDomain::Endpoint:
            {
                static constexpr FTextChoice Kinds[] = {TEXT("place"), TEXT("road")};
                static constexpr FFieldRule Rules[] = {KeyRule(TEXT("kind")).WithChoices(Kinds), KeyRule(TEXT("id"), 128),
                    ObjectRule(TEXT("point")), KeyRule(TEXT("vehicleId"), 160, false, true), CounterRule(TEXT("vehicleRevision"), MaxSafeInteger - 1, false, true)};
                static constexpr FRecordSchema R(Rules, false, [](const FRecordFacts& F)
                {
                    return F.Choices[0] == 0 ? F.Presences[3] == EWorldFieldPresence::Missing && F.Presences[4] == EWorldFieldPresence::Missing
                        : F.Counts[1] <= 85 && F.Presences[3] != EWorldFieldPresence::Missing && F.Presences[3] == F.Presences[4];
                });
                static constexpr FChildRule C[] = {{2, EDomain::Geo}}; static const auto S = RecordDomain(R, C); return S;
            }
            case EDomain::Route:
            {
                static constexpr FTextChoice Versions[] = {TEXT("lagos-corridors-v1"), TEXT("mapbox-directions-v5")};
                static constexpr FTextChoice Placements[] = {TEXT("proposed"), TEXT("road-network")};
                static constexpr FTextChoice Profiles[] = {TEXT("driving-traffic"), TEXT("walking")};
                static constexpr FFieldRule Rules[] = {KeyRule(TEXT("id")), KeyRule(TEXT("placement")).WithChoices(Placements),
                    KeyRule(TEXT("version")).WithChoices(Versions), FFieldRule(TEXT("lengthMetres"), K::Number).WithNumber(0, DBL_MAX),
                    ArrayRule(TEXT("points"), 2, 4096), ArrayRule(TEXT("segmentDurationsMs"), 0, MAX_uint64, false),
                    KeyRule(TEXT("profile"), 240, false).WithChoices(Profiles)};
                static constexpr FRecordSchema R(Rules, false, [](const FRecordFacts& F)
                {
                    if (F.Choices[1] != F.Choices[2]) return false;
                    return F.Choices[2] == 0 ? F.Presences[5] == EWorldFieldPresence::Missing && F.Presences[6] == EWorldFieldPresence::Missing
                        : F.Presences[5] == EWorldFieldPresence::Value && F.Counts[5] == F.Counts[4] - 1;
                });
                static constexpr FChildRule C[] = {{4, EDomain::GeoPoints}, {5, EDomain::Durations}};
                static const auto S = RecordDomain(R, C); return S;
            }
            case EDomain::JourneyTraffic:
            {
                static constexpr FTextChoice Providers[] = {TEXT("mapbox")};
                static constexpr FTextChoice Coverage[] = {TEXT("complete"), TEXT("partial"), TEXT("unavailable")};
                static constexpr FTextChoice Congestion[] = {TEXT("low"), TEXT("moderate"), TEXT("heavy"), TEXT("severe")};
                static constexpr FFieldRule Rules[] = {KeyRule(TEXT("provider")).WithChoices(Providers), TimeRule(TEXT("fetchedAtRealMs")),
                    TimeRule(TEXT("durationMs")), TimeRule(TEXT("typicalDurationMs"), true, true), KeyRule(TEXT("coverage")).WithChoices(Coverage),
                    KeyRule(TEXT("congestion"), 240, true, true).WithChoices(Congestion)};
                static constexpr FRecordSchema R(Rules, false); static const auto S = RecordDomain(R); return S;
            }
            case EDomain::Enforcement:
            {
                static constexpr FTextChoice Agencies[] = {TEXT("police"), TEXT("lastma"), TEXT("efcc"), TEXT("ndlea")};
                static constexpr FTextChoice Stops[] = {TEXT("checkpoint"), TEXT("roadside")};
                static constexpr FTextChoice Districts[] = {TEXT("town-centre"), TEXT("palm-courts"), TEXT("garden-quarter"), TEXT("south-market"), TEXT("marina"), TEXT("skyline"), TEXT("lagoon-gardens")};
                static constexpr FFieldRule Rules[] = {FFieldRule(TEXT("version"), K::Number).WithNumber(1, 1, true),
                    KeyRule(TEXT("agency")).WithChoices(Agencies), KeyRule(TEXT("stopKind")).WithChoices(Stops),
                    KeyRule(TEXT("district")).WithChoices(Districts), FFieldRule(TEXT("isNight"), K::Boolean)};
                static constexpr FRecordSchema R(Rules, false); static const auto S = RecordDomain(R); return S;
            }
            case EDomain::Checkpoint:
            {
                static constexpr FFieldRule Rules[] = {FFieldRule(TEXT("progress"), K::Number).WithNumber(0, 1),
                    TimeRule(TEXT("startsAt")), TimeRule(TEXT("endsAt")), TimeRule(TEXT("resumedAt"), false, true), ObjectRule(TEXT("enforcement"), false)};
                static constexpr FRecordSchema R(Rules, false, [](const FRecordFacts& F) { return F.Numbers[2] >= F.Numbers[1]; });
                static constexpr FChildRule C[] = {{4, EDomain::Enforcement}}; static const auto S = RecordDomain(R, C); return S;
            }
            case EDomain::JourneyTiming:
            {
                static constexpr FFieldRule Rules[] = {CounterRule(TEXT("rate"), 4), TimeRule(TEXT("anchorAt")),
                    FFieldRule(TEXT("anchorProgress"), K::Number).WithNumber(0, 1), TimeRule(TEXT("durationMs"))};
                static constexpr FRecordSchema R(Rules, false, [](const FRecordFacts& F)
                { return (F.Numbers[0] == 1 || F.Numbers[0] == 4) && F.Numbers[3] > 0; });
                static const auto S = RecordDomain(R); return S;
            }
            case EDomain::Journey:
            {
                static constexpr FTextChoice Statuses[] = {TEXT("traveling"), TEXT("arrived"), TEXT("interrupted")};
                static constexpr FTextChoice Vehicles[] = {TEXT("car"), TEXT("okada"), TEXT("keke"), TEXT("danfo"), TEXT("canoe"), TEXT("speedboat")};
                static constexpr FTextChoice Modes[] = {TEXT("walk"), TEXT("vehicle"), TEXT("boat")};
                static constexpr FFieldRule Rules[] = {KeyRule(TEXT("id")), KeyRule(TEXT("requestId")), KeyRule(TEXT("fromPlaceId")), KeyRule(TEXT("toPlaceId")),
                    ObjectRule(TEXT("route")), KeyRule(TEXT("status")).WithChoices(Statuses), KeyRule(TEXT("vehicle")).WithChoices(Vehicles),
                    TimeRule(TEXT("startedAt")), TimeRule(TEXT("arrivesAt")), ObjectRule(TEXT("checkpoint"), true, true),
                    ObjectRule(TEXT("travelTiming"), false), ObjectRule(TEXT("fromEndpoint"), false), ObjectRule(TEXT("toEndpoint"), false),
                    KeyRule(TEXT("travelMode"), 240, false).WithChoices(Modes), ObjectRule(TEXT("traffic"), false)};
                static constexpr FRecordSchema R(Rules, false, [](const FRecordFacts& F) { return F.Numbers[8] >= F.Numbers[7]; });
                static constexpr FChildRule C[] = {{4, EDomain::Route}, {9, EDomain::Checkpoint}, {10, EDomain::JourneyTiming},
                    {11, EDomain::Endpoint}, {12, EDomain::Endpoint}, {14, EDomain::JourneyTraffic}};
                static const auto S = RecordDomain(R, C); return S;
            }
            case EDomain::RoadsideExit:
            {
                static constexpr FFieldRule Rules[] = {KeyRule(TEXT("placeId")), ObjectRule(TEXT("route"))};
                static constexpr FRecordSchema R(Rules, false); static constexpr FChildRule C[] = {{1, EDomain::Route}};
                static const auto S = RecordDomain(R, C); return S;
            }
            case EDomain::Roadside:
            {
                static constexpr FFieldRule Rules[] = {ObjectRule(TEXT("endpoint")), KeyRule(TEXT("sourceJourneyId")), TimeRule(TEXT("stoppedAt")), ObjectRule(TEXT("exit"))};
                static constexpr FRecordSchema R(Rules, false); static constexpr FChildRule C[] = {{0, EDomain::Endpoint}, {3, EDomain::RoadsideExit}};
                static const auto S = RecordDomain(R, C); return S;
            }
            case EDomain::TransitExit:
            {
                static constexpr FFieldRule Rules[] = {TimeRule(TEXT("at")), ObjectRule(TEXT("point")), KeyRule(TEXT("stopId"), 240, true, true)};
                static constexpr FRecordSchema R(Rules, false); static constexpr FChildRule C[] = {{1, EDomain::Pose}};
                static const auto S = RecordDomain(R, C); return S;
            }
            case EDomain::TransitRide:
            {
                static constexpr FFieldRule Rules[] = {KeyRule(TEXT("id")), KeyRule(TEXT("vehicleId")), CounterRule(TEXT("seat")),
                    CounterRule(TEXT("visitRevision")), TimeRule(TEXT("boardedAt")), ObjectRule(TEXT("exit"), true, true)};
                static constexpr FRecordSchema R(Rules, false); static constexpr FChildRule C[] = {{5, EDomain::TransitExit}};
                static const auto S = RecordDomain(R, C); return S;
            }
            case EDomain::DanceInput:
            {
                static constexpr FFieldRule Rules[] = {KeyRule(TEXT("userId")), CounterRule(TEXT("beatIndex"), 23), CounterRule(TEXT("lane"), 3), TimeRule(TEXT("receivedAt"))};
                static constexpr FRecordSchema R(Rules, false); static const auto S = RecordDomain(R); return S;
            }
            case EDomain::DanceRound:
            {
                static constexpr FFieldRule Rules[] = {KeyRule(TEXT("id")), KeyRule(TEXT("instanceId")), KeyRule(TEXT("serviceId")), TimeRule(TEXT("startsAt")),
                    FFieldRule(TEXT("beatMs"), K::Number).WithNumber(250, 2000), FFieldRule(TEXT("windowMs"), K::Number).WithNumber(30, DBL_MAX),
                    CounterRule(TEXT("requiredHits"), 24), ArrayRule(TEXT("lanes"), 1, 24)};
                static constexpr FRecordSchema R(Rules, false, [](const FRecordFacts& F)
                { return F.Numbers[5] < F.Numbers[4] / 2 && F.Numbers[6] >= 1 && F.Numbers[6] <= F.Counts[7]; });
                static constexpr FChildRule C[] = {{7, EDomain::Lanes}}; static const auto S = RecordDomain(R, C); return S;
            }
            case EDomain::Nightclub:
            {
                static constexpr FTextChoice Kinds[] = {TEXT("dance"), TEXT("lounge")};
                static constexpr FTextChoice Difficulties[] = {TEXT("easy"), TEXT("standard"), TEXT("hard")};
                static constexpr FFieldRule Rules[] = {KeyRule(TEXT("id")), CounterRule(TEXT("visitRevision")), KeyRule(TEXT("kind")).WithChoices(Kinds),
                    KeyRule(TEXT("serviceId"), 240, false).When(2, 1), TimeRule(TEXT("joinedAt"), false).When(2, 1), KeyRule(TEXT("seatId"), 240, false).When(2, 1),
                    ObjectRule(TEXT("round"), false).When(2, 0), ArrayRule(TEXT("inputs"), 0, 24, false).When(2, 0), TimeRule(TEXT("stoppedAt"), false, true).When(2, 0),
                    KeyRule(TEXT("difficulty"), 240, false).WithChoices(Difficulties).When(2, 0), KeyRule(TEXT("skillCharacterId"), 240, false).When(2, 0),
                    FFieldRule(TEXT("skillsSettled"), K::Boolean, false).When(2, 0)};
                static constexpr FRecordSchema R(Rules, false, [](const FRecordFacts& F)
                {
                    return F.Choices[2] == 1 ? F.Presences[3] == EWorldFieldPresence::Value && F.Presences[4] == EWorldFieldPresence::Value
                        : F.Presences[6] == EWorldFieldPresence::Value && F.Presences[7] == EWorldFieldPresence::Value && F.Presences[8] != EWorldFieldPresence::Missing;
                });
                static constexpr FChildRule C[] = {{6, EDomain::DanceRound}, {7, EDomain::DanceInputs}};
                static const auto S = RecordDomain(R, C); return S;
            }
            case EDomain::Residence:
            {
                static constexpr FFieldRule Rules[] = {KeyRule(TEXT("cityVersion")), KeyRule(TEXT("parcelId")),
                    FFieldRule(TEXT("unit"), K::Number).WithNumber(1, MaxSafeInteger, true)};
                static constexpr FRecordSchema R(Rules, false); static const auto S = RecordDomain(R); return S;
            }
            case EDomain::NavigationObstacle:
            {
                static constexpr FFieldRule Rules[] = {FFieldRule(TEXT("minX"), K::Number), FFieldRule(TEXT("maxX"), K::Number),
                    FFieldRule(TEXT("minZ"), K::Number), FFieldRule(TEXT("maxZ"), K::Number), FFieldRule(TEXT("yaw"), K::Number, false),
                    ArrayRule(TEXT("polygon"), 3, 8, false)};
                static constexpr FRecordSchema R(Rules, false, [](const FRecordFacts& F)
                { return F.Numbers[0] <= F.Numbers[1] && F.Numbers[2] <= F.Numbers[3]
                    && (F.Presences[5] == EWorldFieldPresence::Missing || F.Presences[4] == EWorldFieldPresence::Missing || F.Numbers[4] == 0); });
                static constexpr FChildRule C[] = {{5, EDomain::Vertices}}; static const auto S = RecordDomain(R, C); return S;
            }
            case EDomain::StreetGraph:
            {
                static constexpr FFieldRule Rules[] = {ArrayRule(TEXT("points")), ArrayRule(TEXT("edges"))};
                static constexpr FRecordSchema R(Rules, false); static constexpr FChildRule C[] = {{0, EDomain::Points}, {1, EDomain::Edges}};
                static const auto S = RecordDomain(R, C); return S;
            }
            case EDomain::Navigation:
            {
                static constexpr FFieldRule Rules[] = {KeyRule(TEXT("revision"), 160000), ObjectRule(TEXT("bounds")),
                    FFieldRule(TEXT("radius"), K::Number), FFieldRule(TEXT("floorY"), K::Number), FFieldRule(TEXT("gridSize"), K::Number, false),
                    ObjectRule(TEXT("streetGraph"), false), ArrayRule(TEXT("obstacles"))};
                static constexpr FRecordSchema R(Rules, false, [](const FRecordFacts& F)
                { return F.Numbers[2] > 0 && (F.Presences[4] == EWorldFieldPresence::Missing || F.Numbers[4] > 0); });
                static constexpr FChildRule C[] = {{1, EDomain::NavigationObstacle}, {5, EDomain::StreetGraph}, {6, EDomain::Obstacles}};
                static const auto S = RecordDomain(R, C); return S;
            }
            case EDomain::Instance:
            {
                static constexpr FFieldRule Rules[] = {KeyRule(TEXT("id")), KeyRule(TEXT("placeId")), KeyRule(TEXT("seed")), KeyRule(TEXT("generatorVersion")),
                    KeyRule(TEXT("layoutId"), 160000), CounterRule(TEXT("sharedStartedAt")), ObjectRule(TEXT("roadside"), false),
                    ObjectRule(TEXT("layout"), true, true), ObjectRule(TEXT("room"), true, true), ObjectRule(TEXT("transitPlan"), false),
                    ObjectRule(TEXT("navigation")), ArrayRule(TEXT("spawns")), ArrayRule(TEXT("exit"), 3, 3)};
                static constexpr FRecordSchema R(Rules, false); static constexpr FChildRule C[] = {{6, EDomain::Endpoint}, {10, EDomain::Navigation},
                    {11, EDomain::Points}, {12, EDomain::ScenePoint}};
                static const auto S = RecordDomain(R, C); return S;
            }
            case EDomain::Player:
            {
                static constexpr FTextChoice Statuses[] = {TEXT("present"), TEXT("traveling"), TEXT("recovering")};
                static constexpr FTextChoice Weapons[] = {TEXT("bat"), TEXT("pistol")};
                static constexpr FFieldRule Rules[] = {KeyRule(TEXT("userId")), KeyRule(TEXT("characterId")), KeyRule(TEXT("gameRoomId")), KeyRule(TEXT("placeId")),
                    KeyRule(TEXT("instanceId")), KeyRule(TEXT("homeRoomId")), CounterRule(TEXT("revision")), CounterRule(TEXT("moveSequence"), 1000000000),
                    ObjectRule(TEXT("position")), ObjectRule(TEXT("movement"), true, true), KeyRule(TEXT("status")).WithChoices(Statuses),
                    FFieldRule(TEXT("health"), K::Number), ObjectRule(TEXT("journey"), true, true), ObjectRule(TEXT("roadside"), false, true),
                    ObjectRule(TEXT("transitRide"), false, true), ObjectRule(TEXT("driving"), false, true), ObjectRule(TEXT("waterControl"), false, true),
                    ObjectRule(TEXT("nightclub"), false, true), ObjectRule(TEXT("residence"), false, true), KeyRule(TEXT("drawnWeaponId"), 240, false, true).WithChoices(Weapons)};
                static constexpr FRecordSchema R(Rules); static constexpr FChildRule C[] = {{8, EDomain::Pose}, {9, EDomain::Movement}, {12, EDomain::Journey},
                    {13, EDomain::Roadside}, {14, EDomain::TransitRide}, {15, EDomain::Driving}, {16, EDomain::Water}, {17, EDomain::Nightclub}, {18, EDomain::Residence}};
                static const auto S = RecordDomain(R, C); return S;
            }
            default:
                if (Domain >= EDomain::Selection && Domain <= EDomain::Rooms) return SelectionDomainSchema(Domain);
                if (Domain >= EDomain::RaceInput && Domain <= EDomain::Racing) return RacingDomainSchema(Domain);
                return MovementDomainSchema(Domain);
            }
        }
    }

    namespace WorldPlayerPrivate
    {
        constexpr int64 MaxInteger = 9007199254740991LL;
        constexpr int32 MaxDocumentCharacters = 2 * 1024 * 1024;
        constexpr int32 MaxDocumentSeparators = 131072;
        constexpr int32 MaxLayoutIdentityCharacters = 160000;
        // Loose array ceilings implied by the document separator budget: four-field
        // boxes consume at least eight separators per entry, XYZ triples three.
        constexpr int32 MaxNavigationObstacles = (MaxDocumentSeparators + 1) / 8;
        constexpr int32 MaxInstanceSpawns = (MaxDocumentSeparators + 1) / 3;
        constexpr int32 MaxStreetPoints = 2048, MaxStreetEdges = 8192;
        using FObject = TSharedPtr<FJsonObject>;
        using FValue = TSharedPtr<FJsonValue>;
        bool Fields(const FObject& Object, WorldReadPrivate::EDomain Domain)
        { return WorldReadPrivate::ReadRecord(Object, *WorldReadPrivate::DomainSchema(Domain).Record); }

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
                else if (C == TEXT(',') || C == TEXT(':')) { if (++Separators > MaxDocumentSeparators) return false; }
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
                && WorldReadPrivate::FiniteNumber(Out);
        }
        bool Number(const FObject& Value, const TCHAR* Key, double& Out)
        {
            return Number(Field(Value, Key), Out);
        }
        bool Integer(const FValue& Value, int64& Out, int64 Maximum = MaxInteger)
        {
            double N = 0;
            if (!Number(Value, N) || !WorldReadPrivate::IntegerNumber(N, 0, Maximum)) return false;
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
            return Number(Value, Out) && WorldReadPrivate::TimeNumber(Out);
        }
        bool Time(const FObject& Value, const TCHAR* Key, double& Out)
        {
            return Time(Field(Value, Key), Out);
        }
        bool Key(const FValue& Value, FString& Out, int32 Maximum = 240)
        {
            return Value.IsValid() && Value->Type == EJson::String && Value->TryGetString(Out)
                && WorldReadPrivate::TextValue(Out, {1, static_cast<uint64>(Maximum), true, true});
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
            return Fields(O, WorldReadPrivate::EDomain::Pose) && Number(O, TEXT("x"), Out.X) && Number(O, TEXT("z"), Out.Z)
                && Number(O, TEXT("yaw"), Out.YawRadians);
        }
        bool ScenePoint(const FValue& Value, FWorldScenePoint& Out)
        {
            if (!Value.IsValid() || Value->Type != EJson::Array) return false;
            const auto& Values = Value->AsArray();
            return Values.Num() == 3 && Number(Values[0], Out.X)
                && Number(Values[1], Out.Y) && Number(Values[2], Out.Z);
        }
        bool ScenePoints(const FValue& Value, TArray<FWorldScenePoint>& Out, int32 Maximum)
        {
            if (!Value.IsValid() || Value->Type != EJson::Array) return false;
            const auto& Values = Value->AsArray();
            if (Values.Num() > Maximum) return false;
            Out.Reserve(Values.Num());
            for (const auto& Entry : Values)
            {
                FWorldScenePoint Next;
                if (!ScenePoint(Entry, Next)) return false;
                Out.Add(Next);
            }
            return true;
        }
        bool NavigationObstacle(const FValue& Value, FWorldNavigationObstacle& Out)
        {
            const auto O = Object(Value);
            if (!Fields(O, WorldReadPrivate::EDomain::NavigationObstacle)) return false;
            if (!Number(O, TEXT("minX"), Out.MinX) || !Number(O, TEXT("maxX"), Out.MaxX)
                || !Number(O, TEXT("minZ"), Out.MinZ) || !Number(O, TEXT("maxZ"), Out.MaxZ)
                || Out.MinX > Out.MaxX || Out.MinZ > Out.MaxZ
                || !Optional(O, TEXT("yaw"), Out.YawRadians, false, false,
                    [](const FValue& V, double& N) { return Number(V, N); })) return false;
            return Optional(O, TEXT("polygon"), Out.Polygon, false, false,
                [&Out](const FValue& V, TArray<FWorldNavigationVertex>& Vertices)
                {
                    if (!V.IsValid() || V->Type != EJson::Array
                        || (Out.YawRadians.Value.IsSet() && Out.YawRadians.Value.GetValue() != 0)) return false;
                    const auto& Values = V->AsArray();
                    if (Values.Num() < 3 || Values.Num() > 8) return false;
                    Vertices.Reserve(Values.Num());
                    for (const auto& Entry : Values)
                    {
                        if (!Entry.IsValid() || Entry->Type != EJson::Array) return false;
                        const auto& Pair = Entry->AsArray();
                        FWorldNavigationVertex Next;
                        if (Pair.Num() != 2 || !Number(Pair[0], Next.X) || !Number(Pair[1], Next.Z)
                            || Next.X < Out.MinX || Next.X > Out.MaxX
                            || Next.Z < Out.MinZ || Next.Z > Out.MaxZ) return false;
                        Vertices.Add(Next);
                    }
                    return true;
                });
        }
        bool StreetGraph(const FValue& Value, FWorldStreetGraph& Out)
        {
            const auto O = Object(Value);
            if (!Fields(O, WorldReadPrivate::EDomain::StreetGraph)) return false;
            if (!ScenePoints(Field(O, TEXT("points")), Out.Points, MaxStreetPoints)) return false;
            const auto EdgeValue = Field(O, TEXT("edges"));
            if (!EdgeValue.IsValid() || EdgeValue->Type != EJson::Array) return false;
            const auto& Edges = EdgeValue->AsArray();
            if (Edges.Num() > MaxStreetEdges) return false;
            Out.Edges.Reserve(Edges.Num());
            for (const auto& Entry : Edges)
            {
                if (!Entry.IsValid() || Entry->Type != EJson::Array) return false;
                const auto& Pair = Entry->AsArray();
                int64 From = 0, To = 0;
                if (Pair.Num() != 2 || !Integer(Pair[0], From, Out.Points.Num() - 1)
                    || !Integer(Pair[1], To, Out.Points.Num() - 1)) return false;
                FWorldStreetGraphEdge Next;
                Next.From = static_cast<int32>(From);
                Next.To = static_cast<int32>(To);
                Out.Edges.Add(Next);
            }
            return true;
        }
        bool Navigation(const FValue& Value, FWorldNavigation& Out)
        {
            const auto O = Object(Value);
            if (!Fields(O, WorldReadPrivate::EDomain::Navigation)) return false;
            if (!Key(O, TEXT("revision"), Out.Revision, MaxLayoutIdentityCharacters)
                || !NavigationObstacle(Field(O, TEXT("bounds")), Out.Bounds)
                || !Number(O, TEXT("radius"), Out.Radius) || Out.Radius <= 0
                || !Number(O, TEXT("floorY"), Out.FloorY)
                || !Optional(O, TEXT("gridSize"), Out.GridSize, false, false,
                    // Race circuits supply 3; the walking planner's .12..1 range is not a wire limit.
                    [](const FValue& V, double& N) { return Number(V, N) && N > 0; })
                || !Optional(O, TEXT("streetGraph"), Out.StreetGraph, false, false, StreetGraph)) return false;
            const auto ObstacleValue = Field(O, TEXT("obstacles"));
            if (!ObstacleValue.IsValid() || ObstacleValue->Type != EJson::Array) return false;
            const auto& Obstacles = ObstacleValue->AsArray();
            if (Obstacles.Num() > MaxNavigationObstacles) return false;
            Out.Obstacles.Reserve(Obstacles.Num());
            for (const auto& Entry : Obstacles)
            {
                FWorldNavigationObstacle Next;
                if (!NavigationObstacle(Entry, Next)) return false;
                Out.Obstacles.Add(MoveTemp(Next));
            }
            return true;
        }
        bool Geo(const FValue& Value, double& Latitude, double& Longitude)
        {
            const auto O = Object(Value);
            return Fields(O, WorldReadPrivate::EDomain::Geo) && Number(O, TEXT("latitude"), Latitude) && std::abs(Latitude) <= 90
                && Number(O, TEXT("longitude"), Longitude) && std::abs(Longitude) <= 180;
        }
        FString PlaceId(const FWorldTravelEndpoint& Endpoint)
        {
            return Endpoint.Kind == EWorldEndpointKind::Road ? TEXT("roadside:") + Endpoint.Id : Endpoint.Id;
        }
        bool Endpoint(const FValue& Value, FWorldTravelEndpoint& Out)
        {
            const auto O = Object(Value);
            if (!Fields(O, WorldReadPrivate::EDomain::Endpoint)) return false;
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
            if (!Fields(O, WorldReadPrivate::EDomain::Route)) return false;
            FString Placement;
            if (!Key(O, TEXT("id"), Out.Id) || !Key(O, TEXT("placement"), Placement)
                || !Enum(Field(O, TEXT("version")), Out.Kind,
                    {{TEXT("lagos-corridors-v1"), EWorldJourneyRouteKind::Corridor},
                     {TEXT("mapbox-directions-v5"), EWorldJourneyRouteKind::RoadNetwork}})
                || !Number(O, TEXT("lengthMetres"), Out.LengthMetres) || Out.LengthMetres < 0) return false;
            const TArray<FValue>* Points = nullptr;
            if (!O->TryGetArrayField(TEXT("points"), Points) || !Points || Points->Num() < 2 || Points->Num() > 4096) return false;
            Out.PointCount = Points->Num();
            Out.Points.Reserve(Points->Num());
            for (const auto& Point : *Points)
            {
                FWorldJourneyGeoPoint Next;
                if (!Geo(Point, Next.Latitude, Next.Longitude)) return false;
                Out.Points.Add(Next);
            }
            if (Out.Kind == EWorldJourneyRouteKind::Corridor)
                return Same(Placement, TEXT("proposed")) && !O->HasField(TEXT("segmentDurationsMs")) && !O->HasField(TEXT("profile"));
            const TArray<FValue>* Durations = nullptr;
            if (!Same(Placement, TEXT("road-network")) || !O->TryGetArrayField(TEXT("segmentDurationsMs"), Durations)
                || !Durations || Durations->Num() != Points->Num() - 1) return false;
            TArray<double> SegmentDurations;
            SegmentDurations.Reserve(Durations->Num());
            for (const auto& Duration : *Durations)
            {
                double N = 0;
                if (!Time(Duration, N)) return false;
                SegmentDurations.Add(N);
            }
            Out.SegmentDurationsMs.Presence = EWorldFieldPresence::Value;
            Out.SegmentDurationsMs.Value = MoveTemp(SegmentDurations);
            return Optional(O, TEXT("profile"), Out.Profile, false, false,
                [](const FValue& V, EWorldJourneyProfile& P) { return Enum(V, P,
                    {{TEXT("driving-traffic"), EWorldJourneyProfile::DrivingTraffic}, {TEXT("walking"), EWorldJourneyProfile::Walking}}); });
        }
        bool JourneyTraffic(const FValue& Value, FWorldJourneyTraffic& Out)
        {
            const auto O = Object(Value);
            if (!Fields(O, WorldReadPrivate::EDomain::JourneyTraffic)) return false;
            return Enum(Field(O, TEXT("provider")), Out.Provider, {{TEXT("mapbox"), EWorldTrafficProvider::Mapbox}})
                && Time(O, TEXT("fetchedAtRealMs"), Out.FetchedAtRealMs) && Time(O, TEXT("durationMs"), Out.DurationMs)
                && Optional(O, TEXT("typicalDurationMs"), Out.TypicalDurationMs, true, true,
                    [](const FValue& V, double& N) { return Time(V, N); })
                && Enum(Field(O, TEXT("coverage")), Out.Coverage,
                    {{TEXT("complete"), EWorldTrafficCoverage::Complete}, {TEXT("partial"), EWorldTrafficCoverage::Partial},
                     {TEXT("unavailable"), EWorldTrafficCoverage::Unavailable}})
                && Optional(O, TEXT("congestion"), Out.Congestion, true, true,
                    [](const FValue& V, EWorldTrafficCongestion& C) { return Enum(V, C,
                        {{TEXT("low"), EWorldTrafficCongestion::Low}, {TEXT("moderate"), EWorldTrafficCongestion::Moderate},
                         {TEXT("heavy"), EWorldTrafficCongestion::Heavy}, {TEXT("severe"), EWorldTrafficCongestion::Severe}}); });
        }
        bool JourneyEnforcement(const FValue& Value, FWorldJourneyEnforcement& Out)
        {
            const auto O = Object(Value);
            if (!Fields(O, WorldReadPrivate::EDomain::Enforcement)) return false;
            return Integer(O, TEXT("version"), Out.Version, 1) && Out.Version == 1
                && Enum(Field(O, TEXT("agency")), Out.Agency,
                    {{TEXT("police"), EWorldEnforcementAgency::Police}, {TEXT("lastma"), EWorldEnforcementAgency::Lastma},
                     {TEXT("efcc"), EWorldEnforcementAgency::Efcc}, {TEXT("ndlea"), EWorldEnforcementAgency::Ndlea}})
                && Enum(Field(O, TEXT("stopKind")), Out.StopKind,
                    {{TEXT("checkpoint"), EWorldEnforcementStopKind::Checkpoint}, {TEXT("roadside"), EWorldEnforcementStopKind::Roadside}})
                && Enum(Field(O, TEXT("district")), Out.District,
                    {{TEXT("town-centre"), EWorldEnforcementDistrict::TownCentre}, {TEXT("palm-courts"), EWorldEnforcementDistrict::PalmCourts},
                     {TEXT("garden-quarter"), EWorldEnforcementDistrict::GardenQuarter}, {TEXT("south-market"), EWorldEnforcementDistrict::SouthMarket},
                     {TEXT("marina"), EWorldEnforcementDistrict::Marina}, {TEXT("skyline"), EWorldEnforcementDistrict::Skyline},
                     {TEXT("lagoon-gardens"), EWorldEnforcementDistrict::LagoonGardens}})
                && Boolean(Field(O, TEXT("isNight")), Out.bIsNight);
        }
        bool Checkpoint(const FValue& Value, FWorldJourneyCheckpoint& Out)
        {
            const auto O = Object(Value);
            if (!Fields(O, WorldReadPrivate::EDomain::Checkpoint)) return false;
            return Number(O, TEXT("progress"), Out.Progress) && Out.Progress >= 0 && Out.Progress <= 1
                && Time(O, TEXT("startsAt"), Out.StartsAtMs) && Time(O, TEXT("endsAt"), Out.EndsAtMs)
                && Out.EndsAtMs >= Out.StartsAtMs
                && Optional(O, TEXT("resumedAt"), Out.ResumedAtMs, false, true,
                    [](const FValue& V, double& N) { return Time(V, N); })
                && Optional(O, TEXT("enforcement"), Out.Enforcement, false, false, JourneyEnforcement);
        }
        bool JourneyTiming(const FValue& Value, FWorldJourneyTiming& Out)
        {
            const auto O = Object(Value);
            if (!Fields(O, WorldReadPrivate::EDomain::JourneyTiming)) return false;
            return Integer(O, TEXT("rate"), Out.Rate, 4) && (Out.Rate == 1 || Out.Rate == 4)
                && Time(O, TEXT("anchorAt"), Out.AnchorAtMs)
                && Number(O, TEXT("anchorProgress"), Out.AnchorProgress) && Out.AnchorProgress >= 0 && Out.AnchorProgress <= 1
                && Time(O, TEXT("durationMs"), Out.DurationMs) && Out.DurationMs > 0;
        }
        bool Journey(const FValue& Value, FWorldJourneyView& Out)
        {
            const auto O = Object(Value);
            if (!Fields(O, WorldReadPrivate::EDomain::Journey)) return false;
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
                || !Optional(O, TEXT("traffic"), Out.Traffic, false, false, JourneyTraffic)) return false;
            return (!Out.FromEndpoint.Value.IsSet() || Same(PlaceId(Out.FromEndpoint.Value.GetValue()), Out.FromPlaceId))
                && (!Out.ToEndpoint.Value.IsSet() || Same(PlaceId(Out.ToEndpoint.Value.GetValue()), Out.ToPlaceId));
        }
        bool Roadside(const FValue& Value, FWorldRoadsideVisit& Out)
        {
            const auto O = Object(Value), Exit = Object(Field(O, TEXT("exit")));
            if (!Fields(O, WorldReadPrivate::EDomain::Roadside) || !Fields(Exit, WorldReadPrivate::EDomain::RoadsideExit)) return false;
            return Endpoint(Field(O, TEXT("endpoint")), Out.Endpoint) && Out.Endpoint.Kind == EWorldEndpointKind::Road
                && Key(O, TEXT("sourceJourneyId"), Out.SourceJourneyId) && Time(O, TEXT("stoppedAt"), Out.StoppedAtMs)
                && Key(Exit, TEXT("placeId"), Out.ExitPlaceId) && Route(Field(Exit, TEXT("route")), Out.ExitRoute);
        }
        bool Transit(const FValue& Value, FWorldTransitRide& Out)
        {
            const auto O = Object(Value);
            if (!Fields(O, WorldReadPrivate::EDomain::TransitRide)) return false;
            return Key(O, TEXT("id"), Out.Id) && Key(O, TEXT("vehicleId"), Out.VehicleId)
                && Integer(O, TEXT("seat"), Out.Seat) && Integer(O, TEXT("visitRevision"), Out.VisitRevision)
                && Time(O, TEXT("boardedAt"), Out.BoardedAtMs)
                && Optional(O, TEXT("exit"), Out.Exit, true, true, [](const FValue& V, FWorldTransitExit& E)
                {
                    const auto Exit = Object(V);
                    if (!Fields(Exit, WorldReadPrivate::EDomain::TransitExit)) return false;
                    return Time(Exit, TEXT("at"), E.AtMs) && Pose(Field(Exit, TEXT("point")), E.Point)
                        && OptionalKey(Exit, TEXT("stopId"), E.StopId, true, true);
                });
        }
        bool Nightclub(const FValue& Value, FWorldNightclubActivity& Out)
        {
            const auto O = Object(Value);
            if (!Fields(O, WorldReadPrivate::EDomain::Nightclub)) return false;
            if (!Key(O, TEXT("id"), Out.Id) || !Integer(O, TEXT("visitRevision"), Out.VisitRevision)
                || !Enum(Field(O, TEXT("kind")), Out.Kind,
                    {{TEXT("dance"), EWorldNightclubKind::Dance}, {TEXT("lounge"), EWorldNightclubKind::Lounge}})) return false;
            if (Out.Kind == EWorldNightclubKind::Lounge)
                return Key(O, TEXT("serviceId"), Out.ServiceId) && Time(O, TEXT("joinedAt"), Out.JoinedAtMs)
                    && OptionalKey(O, TEXT("seatId"), Out.SeatId);
            const auto R = Object(Field(O, TEXT("round")));
            if (!Fields(R, WorldReadPrivate::EDomain::DanceRound)) return false;
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
                if (!Fields(I, WorldReadPrivate::EDomain::DanceInput)) return false;
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
            if (!Fields(O, WorldReadPrivate::EDomain::Residence)) return false;
            return Key(O, TEXT("cityVersion"), Out.CityVersion) && Key(O, TEXT("parcelId"), Out.ParcelId)
                && Integer(O, TEXT("unit"), Out.Unit) && Out.Unit > 0;
        }
        bool PlayerFields(const FObject& O)
        {
            return Fields(O, WorldReadPrivate::EDomain::Player);
        }
        bool ConsistentPlayer(FWorldPlayerView& P)
        {
            WorldReadPrivate::FControlFacts Facts;
            Facts.Status = P.Status; Facts.Health = P.Health;
            Facts.bJourneyTraveling = P.Journey.Value.IsSet()
                && P.Journey.Value.GetValue().Status == EWorldJourneyStatus::Traveling;
            if (P.Status == EWorldPlayerStatus::Traveling && (!P.Journey.Value.IsSet()
                || P.Journey.Value.GetValue().Status != EWorldJourneyStatus::Traveling)) return false;
            if (P.TransitRide.Value.IsSet() && (P.Status != EWorldPlayerStatus::Present
                || P.TransitRide.Value.GetValue().VisitRevision != P.Revision)) return false;
            if (P.Nightclub.Value.IsSet())
            {
                const auto& N = P.Nightclub.Value.GetValue();
                if (P.Status != EWorldPlayerStatus::Present || !Same(P.PlaceId, TEXT("nightclub")) || N.VisitRevision != P.Revision
                    || (N.Kind == EWorldNightclubKind::Dance && !Same(N.Round.InstanceId, P.InstanceId))) return false;
                Facts.bDance = N.Kind == EWorldNightclubKind::Dance;
                Facts.bLoungeSeat = N.Kind == EWorldNightclubKind::Lounge && N.SeatId.Value.IsSet();
            }
            Facts.bDriving = P.Driving.Value.IsSet();
            Facts.bActiveWater = P.WaterControl.Value.IsSet() && P.WaterControl.Value.GetValue().Phase == EWorldWaterPhase::Active;
            Facts.bTransit = P.TransitRide.Value.IsSet();
            if (Facts.bDriving)
            {
                const auto& D = P.Driving.Value.GetValue();
                Facts.bTransportIdentityMismatch = P.Status != EWorldPlayerStatus::Present || !Same(D.GameRoomId, P.GameRoomId)
                    || !Same(D.InstanceId, P.InstanceId) || !Same(D.Driver.UserId, P.UserId) || !Same(D.Driver.CharacterId, P.CharacterId);
                for (const auto& Occupant : D.Occupants)
                    Facts.bTransportIdentityMismatch |= !Same(Occupant.UserId, P.UserId) || !Same(Occupant.CharacterId, P.CharacterId)
                        || !Same(Occupant.LifeId, D.Driver.LifeId);
            }
            if (P.WaterControl.Value.IsSet())
            {
                const auto& S = P.WaterControl.Value.GetValue().Scope;
                Facts.bTransportIdentityMismatch |= P.Status != EWorldPlayerStatus::Present || !Same(S.GameRoomId, P.GameRoomId)
                    || !Same(S.InstanceId, P.InstanceId) || !Same(S.PlaceId, P.PlaceId) || S.Revision != P.Revision
                    || !Same(S.UserId, P.UserId) || !Same(S.CharacterId, P.CharacterId);
            }
            // Retain mismatched/conflicting transport as an observation, never erase
            // it to null. Current life, previous receipt and instance checks need an owner.
            Facts.bMovement = P.Movement.IsSet();
            Facts.Driving = P.Driving.Presence; Facts.Water = P.WaterControl.Presence;
            Facts.Transit = P.TransitRide.Presence; Facts.Nightclub = P.Nightclub.Presence;
            P.Controls = WorldReadPrivate::ControlObservations(Facts);
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
        if (ServerNowMs < 1 || ServerNowMs > MaxInteger || !Parse(PlayerDocumentJson, Player)) return false;
        return ReadWorldPlayerObject(Player, ServerNowMs, Out, OutError);
    }

    bool ReadWorldPlayerObject(const TSharedPtr<FJsonObject>& Player, int64 ServerNowMs,
        TSharedPtr<const FWorldPlayerView>& Out, FString& OutError)
    {
        using namespace WorldPlayerPrivate;
        Out.Reset();
        OutError = TEXT("World player document is invalid or exceeds the decoder bounds.");
        if (ServerNowMs < 1 || ServerNowMs > MaxInteger || !Player.IsValid() || !PlayerFields(Player)) return false;
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
        return ReadWorldInstanceObject(O, Player, Out, OutError);
    }

    bool ReadWorldInstanceObject(const TSharedPtr<FJsonObject>& O, const FWorldPlayerView& Player,
        TSharedPtr<const FWorldCurrentInstanceIdentity>& Out, FString& OutError)
    {
        using namespace WorldPlayerPrivate;
        Out.Reset();
        OutError = TEXT("World instance identity is invalid or exceeds the decoder bounds.");
        if (!Fields(O, WorldReadPrivate::EDomain::Instance)) return false;
        auto Next = MakeShared<FWorldCurrentInstanceIdentity>();
        if (!Key(O, TEXT("id"), Next->Id) || !Key(O, TEXT("placeId"), Next->PlaceId)
            || !Key(O, TEXT("seed"), Next->Seed) || !Key(O, TEXT("generatorVersion"), Next->GeneratorVersion)
            || !Key(O, TEXT("layoutId"), Next->LayoutId, MaxLayoutIdentityCharacters)
            || !Integer(O, TEXT("sharedStartedAt"), Next->SharedStartedAtMs)
            || !Optional(O, TEXT("roadside"), Next->Roadside, false, false, Endpoint)
            || !ObjectPresence(O, TEXT("layout"), Next->Layout, true, true)
            || !ObjectPresence(O, TEXT("room"), Next->Room, true, true)
            || !ObjectPresence(O, TEXT("transitPlan"), Next->TransitPlan, false, false)) return false;
        OutError = TEXT("World instance navigation, spawns or exit is invalid or exceeds the decoder bounds.");
        if (!Navigation(Field(O, TEXT("navigation")), Next->Navigation)
            || !ScenePoints(Field(O, TEXT("spawns")), Next->Spawns, MaxInstanceSpawns)
            || !ScenePoint(Field(O, TEXT("exit")), Next->Exit)) return false;
        Next->NavigationRevision = Next->Navigation.Revision;
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
