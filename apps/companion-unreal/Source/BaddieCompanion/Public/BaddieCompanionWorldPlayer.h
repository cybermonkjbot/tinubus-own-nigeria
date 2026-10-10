#pragma once

#include "BaddieCompanionMovementProtocol.h"

namespace BaddieCompanion
{
    enum class EWorldFieldPresence : uint8 { Missing, Null, Value };
    template <typename T> struct TWorldPlayerField
    {
        EWorldFieldPresence Presence = EWorldFieldPresence::Missing;
        TOptional<T> Value;
    };

    enum class EWorldEndpointKind : uint8 { Place, Road };
    struct FWorldTravelEndpoint
    {
        EWorldEndpointKind Kind = EWorldEndpointKind::Place;
        FString Id;
        double Latitude = 0, Longitude = 0;
        TWorldPlayerField<FString> VehicleId;
        TWorldPlayerField<int64> VehicleRevision;
    };
    enum class EWorldJourneyStatus : uint8 { Traveling, Arrived, Interrupted };
    enum class EWorldTravelMode : uint8 { Walk, Vehicle, Boat };
    enum class EWorldJourneyVehicle : uint8 { Car, Okada, Keke, Danfo, Canoe, Speedboat };
    enum class EWorldJourneyRouteKind : uint8 { Corridor, RoadNetwork };
    enum class EWorldJourneyProfile : uint8 { DrivingTraffic, Walking };
    struct FWorldJourneyGeoPoint
    {
        // Geographic degrees, not source metres or Unreal world coordinates.
        double Latitude = 0, Longitude = 0;
    };
    // Retained wire geometry only. No sampling, length repair or route admission.
    struct FWorldJourneyRouteIdentity
    {
        FString Id;
        EWorldJourneyRouteKind Kind = EWorldJourneyRouteKind::Corridor;
        TWorldPlayerField<EWorldJourneyProfile> Profile;
        int32 PointCount = 0;
        double LengthMetres = 0;
        TArray<FWorldJourneyGeoPoint> Points;
        // Missing on corridors; required value on road routes. Original real ms,
        // including fractional/zero durations, in the supplied segment order.
        TWorldPlayerField<TArray<double>> SegmentDurationsMs;
    };
    enum class EWorldTrafficProvider : uint8 { Mapbox };
    enum class EWorldTrafficCoverage : uint8 { Complete, Partial, Unavailable };
    enum class EWorldTrafficCongestion : uint8 { Low, Moderate, Heavy, Severe };
    struct FWorldJourneyTraffic
    {
        EWorldTrafficProvider Provider = EWorldTrafficProvider::Mapbox;
        double FetchedAtRealMs = 0, DurationMs = 0;
        TWorldPlayerField<double> TypicalDurationMs;
        EWorldTrafficCoverage Coverage = EWorldTrafficCoverage::Unavailable;
        TWorldPlayerField<EWorldTrafficCongestion> Congestion;
    };
    enum class EWorldEnforcementAgency : uint8 { Police, Lastma, Efcc, Ndlea };
    enum class EWorldEnforcementStopKind : uint8 { Checkpoint, Roadside };
    enum class EWorldEnforcementDistrict : uint8
    {
        TownCentre, PalmCourts, GardenQuarter, SouthMarket, Marina, Skyline, LagoonGardens
    };
    // The saved departure decision, not a current encounter or permission to stop/release.
    struct FWorldJourneyEnforcement
    {
        int64 Version = 0;
        EWorldEnforcementAgency Agency = EWorldEnforcementAgency::Police;
        EWorldEnforcementStopKind StopKind = EWorldEnforcementStopKind::Checkpoint;
        EWorldEnforcementDistrict District = EWorldEnforcementDistrict::TownCentre;
        bool bIsNight = false;
    };
    struct FWorldJourneyCheckpoint
    {
        double Progress = 0, StartsAtMs = 0, EndsAtMs = 0;
        TWorldPlayerField<double> ResumedAtMs;
        TWorldPlayerField<FWorldJourneyEnforcement> Enforcement;
    };
    struct FWorldJourneyTiming
    {
        int64 Rate = 0;
        double AnchorAtMs = 0, AnchorProgress = 0, DurationMs = 0;
    };
    struct FWorldJourneyView
    {
        FString Id, RequestId, FromPlaceId, ToPlaceId;
        FWorldJourneyRouteIdentity Route;
        EWorldJourneyVehicle Vehicle = EWorldJourneyVehicle::Car;
        EWorldJourneyStatus Status = EWorldJourneyStatus::Traveling;
        double StartedAtMs = 0, ArrivesAtMs = 0;
        TWorldPlayerField<FWorldJourneyCheckpoint> Checkpoint;
        TWorldPlayerField<FWorldJourneyTiming> TravelTiming;
        TWorldPlayerField<FWorldTravelEndpoint> FromEndpoint, ToEndpoint;
        TWorldPlayerField<EWorldTravelMode> TravelMode;
        // Optional departure estimate. Never refreshed or inferred from route timing.
        TWorldPlayerField<FWorldJourneyTraffic> Traffic;
    };
    struct FWorldRoadsideVisit
    {
        FWorldTravelEndpoint Endpoint;
        FString SourceJourneyId, ExitPlaceId;
        double StoppedAtMs = 0;
        FWorldJourneyRouteIdentity ExitRoute;
    };
    struct FWorldTransitExit
    {
        double AtMs = 0;
        FMovementPose Point;
        TWorldPlayerField<FString> StopId;
    };
    struct FWorldTransitRide
    {
        FString Id, VehicleId;
        int64 Seat = 0, VisitRevision = 0;
        double BoardedAtMs = 0;
        TWorldPlayerField<FWorldTransitExit> Exit;
    };
    enum class EWorldNightclubKind : uint8 { Dance, Lounge };
    enum class EWorldDanceDifficulty : uint8 { Easy, Standard, Hard };
    struct FWorldDanceRound
    {
        FString Id, InstanceId, ServiceId;
        double StartsAtMs = 0, BeatMs = 0, WindowMs = 0;
        int64 RequiredHits = 0;
        TArray<int64> Lanes;
    };
    struct FWorldDanceInput
    {
        FString UserId;
        int64 BeatIndex = 0, Lane = 0;
        double ReceivedAtMs = 0;
    };
    struct FWorldNightclubActivity
    {
        EWorldNightclubKind Kind = EWorldNightclubKind::Lounge;
        FString Id;
        int64 VisitRevision = 0;
        // Lounge fields. Seat omission is distinct from an occupied seat.
        FString ServiceId;
        double JoinedAtMs = 0;
        TWorldPlayerField<FString> SeatId;
        // Dance fields. No evaluation, score, inferred stop or synthesized difficulty.
        FWorldDanceRound Round;
        TArray<FWorldDanceInput> Inputs;
        TWorldPlayerField<EWorldDanceDifficulty> Difficulty;
        TWorldPlayerField<double> StoppedAtMs;
        TWorldPlayerField<FString> SkillCharacterId;
        TWorldPlayerField<bool> SkillsSettled;
    };
    struct FWorldResidenceIdentity
    {
        FString CityVersion, ParcelId;
        int64 Unit = 0;
    };
    enum class EWorldDrawnWeapon : uint8 { Bat, Pistol };

    // Observations, never permissions. Several may coexist. No flag means neither
    // walking permission nor absence of race/mission/combat/police/football ownership.
    struct FWorldPlayerControlObservations
    {
        bool bRecovering = false, bHealthDepleted = false, bTraveling = false;
        bool bDriving = false, bActiveWater = false, bTransit = false;
        bool bDanceActivity = false, bLoungeSeat = false;
        bool bMissingControlFields = false, bConflictingTransport = false;
        bool bTransportIdentityMismatch = false;
    };
    struct FWorldPlayerView
    {
        FString UserId, CharacterId, GameRoomId, PlaceId, InstanceId, HomeRoomId;
        int64 Revision = 0, MoveSequence = 0, ServerNowMs = 0;
        EWorldPlayerStatus Status = EWorldPlayerStatus::Present;
        double Health = 0;
        FMovementPose Position;
        TOptional<FMovementRoute> Movement;
        TWorldPlayerField<FWorldJourneyView> Journey;
        TWorldPlayerField<FWorldRoadsideVisit> Roadside;
        TWorldPlayerField<FWorldTransitRide> TransitRide;
        TWorldPlayerField<FWorldNightclubActivity> Nightclub;
        TWorldPlayerField<FWorldResidenceIdentity> Residence;
        TWorldPlayerField<EWorldDrawnWeapon> DrawnWeapon;
        TWorldPlayerField<FWorldDrivingControl> Driving;
        TWorldPlayerField<FWorldWaterControl> WaterControl;
        FWorldPlayerControlObservations Controls;
    };
    enum class EWorldInstanceRelation : uint8 { CurrentVisit, TravelDestination };
    struct FWorldCurrentInstanceIdentity
    {
        FString Id, PlaceId, Seed, GeneratorVersion, LayoutId, NavigationRevision;
        int64 SharedStartedAtMs = 0;
        EWorldInstanceRelation Relation = EWorldInstanceRelation::CurrentVisit;
        TWorldPlayerField<FWorldTravelEndpoint> Roadside;
        EWorldFieldPresence Layout = EWorldFieldPresence::Missing;
        EWorldFieldPresence Room = EWorldFieldPresence::Missing;
        EWorldFieldPresence TransitPlan = EWorldFieldPresence::Missing;
        TOptional<FString> HomeRoomId;
        // Only compares supplied control layout identities for a current visit.
        // False does not validate content, control freshness, life or domain admission.
        bool bTransportLayoutMismatch = false;
    };

    // Feed PlayerDocumentJson and ServerNowMs from the same movement result.
    // Reuses DecodeWorldMovementResult, including the driving/water decoders.
    // Publishes only const data; keeps no raw JSON, session, credentials or owner.
    // No route acceptance, clock update, actor binding, arrival or recovery is performed.
    // Failure resets Out even if it held an earlier valid view. Fixed errors contain no payload.
    BADDIECOMPANION_API bool DecodeWorldPlayerDocument(const FString& PlayerDocumentJson,
        int64 ServerNowMs, TSharedPtr<const FWorldPlayerView>& Out, FString& OutError);

    // Existing snapshot.instanceJson, not a new endpoint. Identity projection only;
    // successful decoding does not validate navigation geometry, art or generator support.
    // Traveling snapshots describe the destination, which is not the occupied instance.
    // Failure resets Out. Caller must drop prior views on scope/access loss.
    BADDIECOMPANION_API bool DecodeWorldCurrentInstanceIdentity(const FString& InstanceDocumentJson,
        const FWorldPlayerView& Player, TSharedPtr<const FWorldCurrentInstanceIdentity>& Out, FString& OutError);
}
