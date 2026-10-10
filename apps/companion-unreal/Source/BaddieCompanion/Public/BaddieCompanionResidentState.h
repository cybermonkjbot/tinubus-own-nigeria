#pragma once

#include "BaddieCompanionMovementPresentation.h"

namespace BaddieCompanion
{
    enum class EResidentActorKind : uint8 { Npc };
    enum class EResidentKey : uint8
    {
        MaiSuya, AkaraWoman, Courier, DanfoDriver, Medic, Tailor, StreetAkaraVendor,
        AreaTout, TrafficHawker, PosAgent, DanfoConductor
    };
    enum class EResidentOccupation : uint8
    {
        SuyaCook, AkaraCook, Courier, DanfoDriver, Medic, Tailor, TrafficHawker,
        PosOperator, Conductor, UnionCollector
    };
    enum class EResidentRole : uint8 { Vendor, Medic, Driver, Police, MissionContact, Host, Resident };
    enum class EResidentTrait : uint8 { Sociable, Reserved, Patient, Brisk, Practical, Cautious };
    enum class EResidentAssetNpc : uint8 { MaiSuya, AkaraWoman, StreetToutShirt, StreetToutCap };
    struct FResidentAppearance
    {
        int64 Seed = 0;
        // Required nullable field. No substitute model is selected for null.
        TOptional<EResidentAssetNpc> AssetNpcId;
    };
    struct FResidentIdentity
    {
        EResidentActorKind ActorKind = EResidentActorKind::Npc;
        FString Id, WorldKey, Name;
        EResidentKey ActorKey = EResidentKey::MaiSuya;
        int32 RosterVersion = 1;
        EResidentOccupation Occupation = EResidentOccupation::SuyaCook;
        EResidentRole Role = EResidentRole::Resident;
        // Exactly two values, in source order after decoding.
        TArray<EResidentTrait> Personality;
        FResidentAppearance Appearance;
    };
    struct FResidentClock
    {
        int32 Version = 1;
        FString WorldKey, EpochKey;
        int64 EpochRealMs = 0, EpochGameMinute = 0, RealMsPerGameMinute = 0;
    };
    struct FResidentPose
    {
        // Source metres: X/Y/Z, with Y up. Not Unreal centimetres or remapped axes.
        double X = 0, Y = 0, Z = 0, YawRadians = 0;
    };
    struct FResidentPlace
    {
        FString PlaceId, InstanceId;
    };
    struct FResidentAnchor
    {
        FResidentPlace Place;
        FResidentPose Pose;
    };
    enum class EResidentAction : uint8 { Rest, Work, Eat, Socialize, Recover };
    enum class EResidentRoutineAction : uint8 { Rest, Work, Socialize };
    enum class EResidentTravelMode : uint8 { Walk, Car, Okada, Keke, Danfo };
    struct FResidentAtPlaceActivity
    {
        FResidentAnchor Location;
        EResidentAction Action = EResidentAction::Rest;
        TOptional<FString> InteractionId;
        int64 SinceGameMinute = 0;
    };
    struct FResidentTravelActivity
    {
        FString JourneyId;
        FResidentPlace From, To;
        EResidentTravelMode Mode = EResidentTravelMode::Walk;
        TOptional<FString> VehicleId;
        int64 SinceGameMinute = 0;
    };
    enum class EResidentActivityKind : uint8 { AtPlace, Traveling };
    struct FResidentActivity
    {
        EResidentActivityKind Kind = EResidentActivityKind::AtPlace;
        // Decoder sets exactly the branch named by Kind, never both.
        TOptional<FResidentAtPlaceActivity> AtPlace;
        TOptional<FResidentTravelActivity> Traveling;
    };
    enum class EResidentValueUnit : uint8 { GameValueUnit };
    struct FResidentWorth
    {
        EResidentValueUnit Unit = EResidentValueUnit::GameValueUnit;
        FString UnitKey;
        int64 KnownBankedValue = 0, KnownCarriedValue = 0, OutstandingDebt = 0;
        // Required nullable values. NetWorth may be negative. Null never means zero/free.
        TOptional<int64> NetWorth, CarriedValue;
        TArray<FString> UnvaluedAssetIds;
    };
    struct FResidentNeeds
    {
        double Hunger = 0, Energy = 0, Cleanliness = 0, Enjoyment = 0;
    };
    struct FResidentHealth
    {
        double Health = 0, MaxHealth = 0;
    };
    enum class EResidentVehicleKind : uint8 { Car, Okada, Keke, Danfo };
    struct FResidentVehicleModel
    {
        FString CatalogueId, ModelId;
        int64 CatalogueRevision = 0;
        EResidentVehicleKind Kind = EResidentVehicleKind::Car;
        int64 TankCapacityUnits = 0, MetresPerFuelUnit = 0;
    };
    struct FResidentVehicleAsset
    {
        FString Key;
        double SourceYawRadians = 0;
    };
    struct FResidentJourneyTransport
    {
        FString VehicleId, OwnerResidentId;
        TOptional<FString> RiderResidentId;
        FResidentVehicleModel Model;
        TOptional<FResidentVehicleAsset> Asset;
    };
    struct FResidentGeoPoint
    {
        // Geographic degrees, independent of place metres and Unreal coordinates.
        double Latitude = 0, Longitude = 0;
    };
    enum class EResidentRouteVersion : uint8 { LagosCorridorsV1 };
    enum class EResidentRoutePlacement : uint8 { Proposed };
    struct FResidentCorridorRoute
    {
        FString Id;
        EResidentRouteVersion Version = EResidentRouteVersion::LagosCorridorsV1;
        EResidentRoutePlacement Placement = EResidentRoutePlacement::Proposed;
        TArray<FResidentGeoPoint> Points;
        double LengthMetres = 0;
    };
    enum class EResidentJourneyTimeBasis : uint8 { ServerMs };
    struct FResidentJourney
    {
        FString Id;
        FResidentAnchor From, To;
        EResidentTravelMode Mode = EResidentTravelMode::Walk;
        TOptional<FString> VehicleId;
        // Optional object, not nullable. Older rows may omit it even for a vehicle journey.
        TOptional<FResidentJourneyTransport> Transport;
        EResidentJourneyTimeBasis TimeBasis = EResidentJourneyTimeBasis::ServerMs;
        FMovementRoute Departure, Arrival;
        FResidentCorridorRoute Route;
        double RouteStartedAtRealMs = 0, RouteEndsAtRealMs = 0, ArrivesAtRealMs = 0;
        EResidentRoutineAction ArrivalAction = EResidentRoutineAction::Rest;
    };
    struct FResidentLocalMove
    {
        FMovementRoute Movement;
        FResidentAnchor To;
        EResidentRoutineAction Action = EResidentRoutineAction::Rest;
    };
    struct FResidentServiceActor
    {
        FString PlaceId, NpcId;
    };
    struct FResidentRenderAsset
    {
        // Retained asset-registry key only. No registry lookup, import or model binding.
        FString Key;
        EResidentAssetNpc NpcId = EResidentAssetNpc::MaiSuya;
        // Optional number, not nullable. Missing stays missing; no yaw default is applied.
        TOptional<double> SourceYawRadians;
    };
    enum class EResidentFieldPresence : uint8 { Missing, Null, Value };
    struct FResidentRenderAssetField
    {
        EResidentFieldPresence Presence = EResidentFieldPresence::Missing;
        TOptional<FResidentRenderAsset> Value;
    };
    struct FResidentView
    {
        FResidentIdentity Identity;
        int64 Revision = 0;
        FString ClockEpochKey;
        int64 UpdatedAtGameMinute = 0;
        FResidentHealth Health;
        FResidentNeeds Needs;
        FResidentWorth Worth;
        FResidentActivity Activity;
        // Each wire key is required and nullable.
        TOptional<FResidentJourney> Journey;
        TOptional<FResidentLocalMove> LocalMove;
        TOptional<FResidentServiceActor> ServiceActor;
        FResidentRenderAssetField RenderAsset;
    };
    struct FResidentPlaceBinding
    {
        FString PlaceId, ResidentInstanceId, GameInstanceId, LayoutId;
    };
    struct FResidentWorldView
    {
        FString WorldKey;
        FResidentClock Clock;
        TOptional<int64> NextAdvanceAtRealMs;
        TArray<FResidentView> Residents;
        // Optional array: missing differs from empty; explicit null is invalid.
        TOptional<TArray<FResidentPlaceBinding>> PlaceBindings;
    };

    // Inner serialized companionWorld:residents value, or literal JSON null for an
    // authoritative null endpoint result. The transport must unwrap the endpoint string
    // and reject a missing envelope; empty text is invalid, never an absent population.
    // True + null Out means known null. False clears Out and returns fixed diagnostics.
    // Atomic, bounded, immutable publication. No sampling, subscription/clock ownership,
    // render readiness, target resolution, access grant or resident progression.
    BADDIECOMPANION_API bool DecodeResidentWorld(const FString& Json,
        TSharedPtr<const FResidentWorldView>& Out, FString& OutError);
}
