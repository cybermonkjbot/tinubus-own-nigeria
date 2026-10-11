#pragma once

#include "CoreMinimal.h"
#include "BaddieCompanionWorldPlayer.h"
#include "BaddieCompanionWorldClock.h"

class UBaddieCompanionWorldPresentation;
class ABaddieCompanionWorldCharacter;

namespace BaddieCompanion
{
    class FWorldSession;
    struct FSnapshotSelection;
    class FWorldValueRead;
    class FWorldPlayerRead;
    class FWorldInstanceRead;
    class FWorldSelectionRead;
    class FWorldHealthRead;
    class FWorldHomeProjectionRead;
    enum class EHomeIssue : uint8;
    namespace WorldObservationPrivate
    {
        struct FStorage; struct FAccess; class FHomeBorrow;
        // Weak metadata references never free the cell on a caller's stack.
        // The session's reserved retirement task owns its final destruction.
        class BADDIECOMPANION_API FAccessWeak
        {
        public:
            FAccessWeak() = default;
            explicit FAccessWeak(FAccess* InCell);
            FAccessWeak(const FAccessWeak& Other);
            FAccessWeak(FAccessWeak&& Other) noexcept;
            FAccessWeak& operator=(const FAccessWeak& Other);
            FAccessWeak& operator=(FAccessWeak&& Other) noexcept;
            ~FAccessWeak();
            FAccess* Pin() const;
        private:
            FAccess* Cell = nullptr;
            void Reset();
        };
    }

    enum class EWorldRoot : uint8
    {
        Player, Peers, Instance, Racing, Transit, DrivingCatalog, Road, Factions,
        Hawkers, Water, TransitAvailability, StreetScope, StreetTraffic,
        CheckpointStatus, FriendResidences, Health, Selection, Binding, Result, Presentation
    };
    enum class EWorldValueKind : uint8 { Object, Array, String, StringBlock, Null, Boolean, Number };
    enum class EWorldReadResult : uint8 { Ready, Pending, Unavailable };
    struct FWorldObservationToken
    {
        uint64 Lifetime = 0, Publication = 0;
        bool IsSet() const { return Lifetime && Publication; }
        bool operator==(const FWorldObservationToken& Other) const
        { return Lifetime == Other.Lifetime && Publication == Other.Publication; }
        bool operator!=(const FWorldObservationToken& Other) const { return !(*this == Other); }
    };
    enum class EWorldReadStatus : uint8
    {
        Unavailable, Preparing, Capturing, Receiving, Binding, Accepted,
        ResourceUnavailable, InvalidResponse, Expired, ContentUnavailable
    };
    struct FWorldNodeView
    {
        EWorldValueKind Kind = EWorldValueKind::Null;
        uint64 Id = 0, Parent = 0, FirstChild = 0, NextSibling = 0, Count = 0;
        uint64 Slot = 0, NumberBits = 0;
        uint8 Role = 0;
    };
    struct FWorldRootView
    {
        EWorldFieldPresence Presence = EWorldFieldPresence::Missing;
        bool bJsonDocument = false;
        uint64 Nodes = 0, StringUnits = 0;
    };
    struct FWorldStringCursor
    {
    private:
        friend class FWorldObservation;
        friend class FWorldValueRead;
        uint64 Generation = 0, Node = 0, Offset = 0;
        uint64 Publication = 0;
        uint8 Root = 0;
        const void* Owner = nullptr;
        const void* Block = nullptr;
    };

    // Revocable read handle. It owns no tree and exposes no authority or mutable state.
    // Node IDs are root-local. Output copies are bounded; no page buffers escape.
    class BADDIECOMPANION_API FWorldObservation
    {
    public:
        bool IsCurrent() const;
        uint64 Generation() const;
        uint64 PublicationSerial() const { return Captured.Publication; }
        FWorldObservationToken Token() const { return Captured; }
        bool ReadRoot(EWorldRoot Root, FWorldRootView& Out) const;
        EWorldReadResult ReadValue(EWorldRoot Root, FWorldValueRead& Out) const;
        bool ReadNode(EWorldRoot Root, uint64 Id, FWorldNodeView& Out) const;
        // A fresh cursor begins a string. At most 256 exact units per call.
        bool ReadString(EWorldRoot Root, uint64 Id, FWorldStringCursor& Cursor,
            uint16 (&Out)[256], int32& OutCount) const;
        EWorldReadResult ReadPlayer(FWorldPlayerRead& Out) const;
        EWorldReadResult ReadPeer(struct FWorldPeerCursor& Cursor, FWorldPlayerRead& Out, bool& bEnd) const;
        EWorldReadResult ReadRacing(FWorldValueRead& Out) const;
        EWorldReadResult ReadInstance(FWorldInstanceRead& Out) const;
        EWorldReadResult ReadSelection(FWorldSelectionRead& Out) const;
        EWorldReadResult ReadHealth(FWorldHealthRead& Out) const;
        EWorldReadResult ReadHomeProjection(FWorldHomeProjectionRead& Out) const;
        bool CopyTime(FWorldPresentationTime& Out) const;
    private:
        friend class ::UBaddieCompanionWorldPresentation;
        friend class ::ABaddieCompanionWorldCharacter;
        EWorldReadResult AdmitRead() const;
        EWorldReadResult BorrowHome(WorldObservationPrivate::FHomeBorrow& Out) const;
        EWorldReadResult AdmitNativeContentObservation() const;
        EWorldReadResult ReadMovementWindow(double FromMs, double ToMs,
            FMovementPose (&Out)[34], uint32& Count) const;
        friend class FWorldSession;
        friend class FWorldValueRead;
        friend class FWorldPlayerRead;
        friend class FWorldInstanceRead;
        friend class FWorldSelectionRead;
        friend class FWorldHealthRead;
        friend class FWorldHomeProjectionRead;
        WorldObservationPrivate::FAccessWeak Access;
        FWorldObservationToken Captured;
    };

    // Record field indices and child domains follow the shared domain schemas.
    // Raw traversal is also available for every extension and encoded root.
    enum class EWorldReadDomain : uint8
    {
        Number, Time, Lane, ScenePoint, Vertex, Edge,
        Pose, Geo, Endpoint, Route, JourneyTraffic, Enforcement, Checkpoint, JourneyTiming, Journey,
        RoadsideExit, Roadside, TransitExit, TransitRide, DanceInput, DanceRound, Nightclub, Residence,
        NavigationObstacle, StreetGraph, Navigation, Instance, Player,
        Points, Vertices, Edges, Obstacles, GeoPoints, Durations, Lanes, DanceInputs,
        Movement, MovementPoint, MovementPoints,
        Driving, DrivingPose, DrivingOccupant, DrivingOccupants, DrivingInput, DrivingContact,
        VehicleVector, VehicleOrientation, VehicleWheel, VehicleWheels, VehicleDynamics,
        Water, WaterScope, WaterInput, WaterSnag, WaterVelocity,
        RaceInput, RaceCar, RaceCars, Race, SelectedCar, Racing,
        Selection, Library, SavedCharacter, Appearance, SavedRoom, Characters, Rooms,
        Health, RecoveryDestination, HomeDesign, HomeItem, HomeItems, Raw
    };
    struct FWorldScalarRead
    {
        EWorldFieldPresence Presence = EWorldFieldPresence::Missing;
        EWorldValueKind Kind = EWorldValueKind::Null;
        double Number = 0;
        uint64 Count = 0;
        int32 Choice = -1;
        bool Boolean = false;
    };
    struct FWorldValuePresence
    {
        EWorldFieldPresence Supplied = EWorldFieldPresence::Missing;
        EWorldFieldPresence Effective = EWorldFieldPresence::Missing;
        bool bSuppressed = false, bControlUnavailable = false;
        bool bJsonDocument = false;
    };
    enum class EWorldPlayerField : uint32
    {
        UserId, CharacterId, GameRoomId, PlaceId, InstanceId, HomeRoomId, Revision, MoveSequence,
        Position, Movement, Status, Health, Journey, Roadside, TransitRide, Driving, WaterControl,
        Nightclub, Residence, DrawnWeapon
    };
    enum class EWorldInstanceField : uint32
    { Id, PlaceId, Seed, GeneratorVersion, LayoutId, SharedStartedAt, Roadside, Layout, Room, TransitPlan, Navigation, Spawns, Exit };
    enum class EWorldNavigationField : uint32
    { Revision, Bounds, Radius, FloorY, GridSize, StreetGraph, Obstacles };
    enum class EWorldMovementField : uint32
    { Version, Sequence, RoomRevision, StartedAt, From, Yaw, InitialSpeed, Pace, Path };
    enum class EWorldNightclubField : uint32
    { Id, VisitRevision, Kind, ServiceId, JoinedAt, SeatId, Round, Inputs, StoppedAt, Difficulty, SkillCharacterId, SkillsSettled };
    enum class EWorldHealthField : uint32
    { LifeId, Revision, Health, MaxHealth, Status, DeathKey, ProtectedUntil, Recovery };
    enum class EWorldRecoveryDestinationField : uint32 { PlaceId, InstanceId, Position, ReadyAt };
    enum class EWorldSelectionField : uint32 { Revision, Library };
    enum class EWorldLibraryField : uint32 { Version, ActiveCharacterId, ActiveRoomId, HomeRoomId, Characters, Rooms };
    enum class EWorldSavedCharacterField : uint32 { Id, Name, Appearance };
    enum class EWorldAppearanceField : uint32
    { Look, BaseId, BodyColor, TopColor, TrousersColor, FootwearColor, EyeColor, NoseColor,
        MarkingColor, PatternColor, AccessoryVisible, NecklaceVisible, HipChainVisible, GarmentStyle,
        FabricTexture, ClothingPattern, EyeShape, BrowStyle, NoseShape, MouthShape, CheekStyle, MarkingStyle, Eyewear };
    enum class EWorldSavedRoomField : uint32
    { Id, Name, EnvironmentId, LayoutId, HomeDesign, Palette, Light, Sharing, Items,
        WallColor, FloorColor, UpholsteryColor, RugColor };
    enum class EWorldHomeDesignField : uint32 { Version, Turns, Extension };
    enum class EWorldHomeItemField : uint32 { Id, Kind, X, Z, Rotation, Surface, Y, Lit };
    enum class EWorldPoseField : uint32 { X, Z, Yaw };
    enum class EWorldGeoField : uint32 { Latitude, Longitude };
    enum class EWorldEndpointField : uint32 { Kind, Id, Point, VehicleId, VehicleRevision };
    enum class EWorldRouteField : uint32 { Id, Placement, Version, LengthMetres, Points, SegmentDurationsMs, Profile };
    enum class EWorldJourneyTrafficField : uint32 { Provider, FetchedAtRealMs, DurationMs, TypicalDurationMs, Coverage, Congestion };
    enum class EWorldEnforcementField : uint32 { Version, Agency, StopKind, District, IsNight };
    enum class EWorldCheckpointField : uint32 { Progress, StartsAt, EndsAt, ResumedAt, Enforcement };
    enum class EWorldJourneyTimingField : uint32 { Rate, AnchorAt, AnchorProgress, DurationMs };
    enum class EWorldJourneyField : uint32
    { Id, RequestId, FromPlaceId, ToPlaceId, Route, Status, Vehicle, StartedAt, ArrivesAt, Checkpoint, TravelTiming, FromEndpoint, ToEndpoint, TravelMode, Traffic };
    enum class EWorldRoadsideExitField : uint32 { PlaceId, Route };
    enum class EWorldRoadsideField : uint32 { Endpoint, SourceJourneyId, StoppedAt, Exit };
    enum class EWorldTransitExitField : uint32 { At, Point, StopId };
    enum class EWorldTransitRideField : uint32 { Id, VehicleId, Seat, VisitRevision, BoardedAt, Exit };
    enum class EWorldDanceInputField : uint32 { UserId, BeatIndex, Lane, ReceivedAt };
    enum class EWorldDanceRoundField : uint32 { Id, InstanceId, ServiceId, StartsAt, BeatMs, WindowMs, RequiredHits, Lanes };
    enum class EWorldResidenceField : uint32 { CityVersion, ParcelId, Unit };
    enum class EWorldNavigationObstacleField : uint32 { MinX, MaxX, MinZ, MaxZ, Yaw, Polygon };
    enum class EWorldStreetGraphField : uint32 { Points, Edges };
    enum class EWorldMovementPointField : uint32 { X, Z };
    enum class EWorldDrivingField : uint32
    {
        Id, VehicleId, GameRoomId, InstanceId, NavigationRevision, RouteId, VehicleRevision, Revision,
        Sequence, AcceptedAt, InputExpiresAt, Driver, Pose, Speed, DrivenMetres, FuelMetres, Input,
        Occupants, Mode, Surface, Contact, Dynamics
    };
    enum class EWorldDrivingPoseField : uint32 { X, Y, Z, Yaw };
    enum class EWorldDrivingOccupantField : uint32 { UserId, CharacterId, LifeId, Role, Seat };
    enum class EWorldDrivingInputField : uint32 { Throttle, Brake, Steering };
    enum class EWorldDrivingContactField : uint32 { Id, BodyId, At, Speed, Pose };
    enum class EWorldVehicleVectorField : uint32 { X, Y, Z };
    enum class EWorldVehicleOrientationField : uint32 { X, Y, Z, W };
    enum class EWorldVehicleWheelField : uint32
    { AngularVelocity, Angle, Steering, Compression, NormalLoad, SlipRatio, SlipAngle, LongitudinalForce, LateralForce, Grounded };
    enum class EWorldVehicleDynamicsField : uint32
    { Version, Position, Velocity, AngularVelocity, Orientation, Steering, EngineOmega, ShiftRemaining, Gear, Wheels };
    enum class EWorldWaterField : uint32
    {
        Id, CraftId, DomainId, Scope, CraftRevision, Sequence, SimulatedAt, StartedAt, ExpiresAt, EndsAt,
        LastInputAt, Point, Velocity, Yaw, Input, Phase, Feedback, Snag, SessionId, LandingReceipt
    };
    enum class EWorldWaterScopeField : uint32 { GameRoomId, InstanceId, PlaceId, LayoutId, LayoutRevision, UserId, CharacterId, LifeId, Revision };
    enum class EWorldWaterInputField : uint32 { Throttle, Steer, Brake };
    enum class EWorldWaterSnagField : uint32 { ZoneId, ExposureMs, ClearMs, Held };
    enum class EWorldRaceInputField : uint32 { Throttle, Brake, Steering, Handbrake, Gear };
    enum class EWorldRaceCarField : uint32
    {
        UserId, Name, VehicleId, Transmission, X, Z, Yaw, Speed, Gear, Sequence, Input, InputAt, SimulatedAt,
        RecoveredAt, Distance, Progress, NextCheckpoint, Laps, LapStartedAt, LastLapMs, BestLapMs, FinishedAt, Status
    };
    enum class EWorldRaceField : uint32 { Id, InstanceId, HostId, Phase, StartsAt, EndsAt, UpdatedAt, Cars };
    enum class EWorldSelectedCarField : uint32 { VehicleId, Revision, FuelUnits, Condition, Here };
    enum class EWorldRacingField : uint32 { Race, Car, Eligibility };
    struct FWorldChoiceCursor
    {
    private:
        friend class FWorldValueRead;
        FWorldObservationToken Token;
        const void* Owner = nullptr;
        const void* Block = nullptr;
        uint64 Node = 0, Offset = 0;
        uint32 Choice = 0;
        bool bStarted = false, bComparing = false, bEqual = false;
    };
    struct FWorldFieldCursor
    {
    private:
        friend class FWorldValueRead;
        FWorldObservationToken Token;
        uint64 Record = 0, Key = 0, Offset = 0;
        uint32 Field = MAX_uint32;
        const void* Owner = nullptr;
        const void* Block = nullptr;
        bool bEqual = true, bStarted = false, bComparing = false;
    };
    struct FWorldSequenceCursor
    {
    private:
        friend class FWorldValueRead;
        FWorldObservationToken Token;
        uint64 Record = 0, Next = 0, Index = 0;
        const void* Owner = nullptr;
        bool bStarted = false;
    };
    struct FWorldPeerCursor
    {
    private:
        friend class FWorldObservation;
        FWorldObservationToken Token;
        const void* Next = nullptr;
        bool bStarted = false;
    };
    class BADDIECOMPANION_API FWorldValueRead
    {
    public:
        bool IsCurrent() const { return Observation.IsCurrent(); }
        EWorldReadResult Scalar(FWorldScalarRead& Out) const;
        EWorldReadResult Choice(FWorldChoiceCursor& Cursor, int32& Out) const;
        EWorldReadResult ReadNode(FWorldNodeView& Out) const;
        EWorldReadResult ReadPresence(FWorldValuePresence& Out) const;
        EWorldReadResult Field(uint32 Index, FWorldFieldCursor& Cursor, FWorldValueRead& Out) const;
        template<typename T> EWorldReadResult Field(T Index, FWorldFieldCursor& Cursor, FWorldValueRead& Out) const
        { return Field(static_cast<uint32>(Index), Cursor, Out); }
        EWorldReadResult Next(FWorldSequenceCursor& Cursor, FWorldValueRead& Out, bool& bEnd) const;
        EWorldReadResult ReadString(FWorldStringCursor& Cursor, uint16 (&Out)[256], int32& OutCount) const;
        FWorldValueRead Raw() const;
        EWorldReadDomain Domain() const { return Kind; }
    private:
        friend class FWorldObservation;
        friend class FWorldPlayerRead;
        friend class FWorldInstanceRead;
        friend class FWorldSelectionRead;
        friend class FWorldHealthRead;
        friend class FWorldHomeProjectionRead;
        FWorldObservation Observation;
        const void* Owner = nullptr;
        const void* Effective = nullptr;
        const void* SourceOwner = nullptr;
        const void* ChoiceRule = nullptr;
        uint64 Node = 0;
        uint64 SourceNode = 0;
        uint8 Root = 0;
        uint8 SourceRoot = 0;
        EWorldReadDomain Kind = EWorldReadDomain::Raw;
        EWorldFieldPresence Presence = EWorldFieldPresence::Missing;
        EWorldFieldPresence SourcePresence = EWorldFieldPresence::Missing;
        bool bMergedInputs = false, bHasSource = false, bDefaultBase = false;
        bool bJsonDocument = false;
    };
    struct FWorldPlayerHeader
    {
        int64 Revision = 0, MoveSequence = 0, ServerNowMs = 0;
        EWorldPlayerStatus Status = EWorldPlayerStatus::Present;
        double Health = 0;
        FMovementPose Position;
        FWorldPlayerControlObservations Controls;
        EWorldFieldPresence Movement = EWorldFieldPresence::Missing;
        EWorldFieldPresence Driving = EWorldFieldPresence::Missing, Water = EWorldFieldPresence::Missing;
        bool bControlUnavailable = true;
    };
    class BADDIECOMPANION_API FWorldPlayerRead
    {
    public:
        EWorldReadResult Header(FWorldPlayerHeader& Out) const;
        FWorldValueRead Value() const { return Record; }
    private:
        friend class FWorldObservation;
        FWorldValueRead Record;
    };
    struct FWorldInstanceHeader
    {
        int64 SharedStartedAtMs = 0;
        double FloorY = 0;
        EWorldInstanceRelation Relation = EWorldInstanceRelation::CurrentVisit;
        bool bPlayerInstanceMatches = false, bOwnHome = false, bTransportLayoutMismatch = false;
    };
    class BADDIECOMPANION_API FWorldInstanceRead
    {
    public:
        EWorldReadResult Header(FWorldInstanceHeader& Out) const;
        EWorldReadResult HomeRoomId(FWorldValueRead& Out) const;
        FWorldValueRead Value() const { return Record; }
    private:
        friend class FWorldObservation;
        FWorldValueRead Record;
    };
    class BADDIECOMPANION_API FWorldSelectionRead
    {
    public:
        FWorldValueRead Value() const { return Record; }
        bool HasCompleteApartmentInventory() const { return false; }
    private:
        friend class FWorldObservation;
        FWorldValueRead Record;
    };
    class BADDIECOMPANION_API FWorldHealthRead
    {
    public:
        FWorldValueRead Value() const { return Record; }
    private:
        friend class FWorldObservation;
        FWorldValueRead Record;
    };
    class BADDIECOMPANION_API FWorldHomeProjectionRead
    {
    public:
        bool IsCurrent() const { return Observation.IsCurrent(); }
        EWorldReadResult Outcome(bool& bMapped) const;
        EWorldReadResult NextIssue(struct FWorldHomeIssueCursor& Cursor,
            struct FWorldHomeIssue& Out, bool& bEnd) const;
    private:
        friend class FWorldObservation;
        FWorldObservation Observation;
    };
    struct FWorldHomeIssue
    {
        EHomeIssue Code{};
        TCHAR Path[96]{};
    };
    struct FWorldHomeIssueCursor
    {
    private:
        friend class FWorldHomeProjectionRead;
        FWorldObservationToken Token;
        uint32 Known = 0, Range = 0, Name = 0;
        uint64 Key = 0, Offset = 0;
        const void* Block = nullptr;
        bool bStarted = false, bRangeStarted = false, bComparing = false, bEqual = false;
    };

    struct FWorldSessionView
    {
        EWorldReadStatus Status = EWorldReadStatus::Unavailable;
        uint64 Generation = 0;
        FWorldObservation Observation;
        // These are owner-derived observations, never caller-set admission inputs.
        bool bHasObservation = false;
        bool bWalkingAvailable = false;
    };
}
