#pragma once

#include "BaddieCompanionWorldPlayer.h"
#include "BaddieCompanionRecoveryState.h"
#include "BaddieCompanionRacingState.h"
#include "BaddieCompanionWorldObservationPages.h"

#include <cmath>
#include <cfloat>
#include <type_traits>

class FJsonObject;
class FJsonValue;

namespace BaddieCompanion
{
    namespace WorldReadPrivate
    {
        constexpr int64 MaxSafeInteger = 9007199254740991LL;

        // These predicates have no allocation, ownership or JSON dependency. Both
        // adapters apply them to the original value, never a coerced string/number.
        inline bool FiniteNumber(double Value) { return std::isfinite(Value); }
        inline bool IntegerNumber(double Value, double Minimum = 0, double Maximum = MaxSafeInteger)
        {
            return FiniteNumber(Value) && Value >= Minimum && Value <= Maximum
                && std::floor(Value) == Value;
        }
        inline bool TimeNumber(double Value)
        {
            return FiniteNumber(Value) && Value >= 0 && Value <= MaxSafeInteger;
        }
        inline bool GraphEdge(double From, double To, uint64 Points)
        { return Points > 0 && IntegerNumber(From, 0, Points - 1) && IntegerNumber(To, 0, Points - 1); }
        inline bool NavigationVertex(double X, double Z, double MinX, double MaxX, double MinZ, double MaxZ)
        { return FiniteNumber(X) && FiniteNumber(Z) && X >= MinX && X <= MaxX && Z >= MinZ && Z <= MaxZ; }
        inline bool DanceBeat(int64 Beat, uint64 Lanes) { return Beat >= 0 && static_cast<uint64>(Beat) < Lanes; }
        inline bool MovementScope(int64 Sequence, int64 Revision, int64 PlayerSequence, int64 PlayerRevision)
        { return Sequence == PlayerSequence && Revision == PlayerRevision; }
        inline bool RetainDriving(bool SameIdentity, int64 Revision, int64 Sequence, int64 At,
            int64 PriorRevision, int64 PriorSequence, int64 PriorAt)
        { return SameIdentity && (Revision < PriorRevision || Sequence < PriorSequence || At < PriorAt); }
        inline bool RetainWater(bool SameScope, int64 Sequence, int64 Revision, int64 At,
            int64 PriorSequence, int64 PriorRevision, int64 PriorAt)
        { return SameScope && (Sequence < PriorSequence || Revision < PriorRevision || At < PriorAt); }
        inline bool ConflictingTransport(bool Driving, bool ActiveWater, bool Movement, bool Transit, bool TravelingJourney)
        { return (Driving && ActiveWater) || ((Driving || ActiveWater) && (Movement || Transit || TravelingJourney)); }
        inline bool RetainRacing(bool SameInstance, int64 UpdatedAt, int64 PriorUpdatedAt)
        { return SameInstance && UpdatedAt < PriorUpdatedAt; }
        inline bool DrivingSpeed(double Speed) { return FiniteNumber(Speed) && Speed >= -7 && Speed <= 7; }
        inline bool WaterRevision(int64 Revision) { return Revision >= 0 && Revision < MaxSafeInteger - 1; }
        inline bool WaterTiming(int64 Started, int64 Input, int64 Simulated, int64 Expires, int64 Ends)
        { return Started <= Simulated && Input <= Simulated && Expires <= Ends; }

        struct FTextRule
        {
            uint64 Minimum = 0, Maximum = MAX_uint64;
            bool bTrimmed = false, bRejectControls = false, bHasNonWhitespace = false;
            bool Length(uint64 Count) const { return Count >= Minimum && Count <= Maximum; }
            bool Unit(uint16 Value, uint64 Offset, uint64 Count) const
            {
                return (!bRejectControls || Value >= 32)
                    && (!bTrimmed || (Offset != 0 && Offset + 1 != Count)
                        || !FChar::IsWhitespace(static_cast<TCHAR>(Value)));
            }
        };
        bool TextValue(const FString& Value, const FTextRule& Rule);

        // A borrowed reference is meaningful only for the reader's pinned storage.
        // Missing has no node; explicit null has a node of kind Null.
        struct FValueRef
        {
            uint64 Node = 0;
            uint8 Root = 0;
            WorldObservationPrivate::FRoot* Owner = nullptr;
            bool IsSet() const { return Node != 0; }
            bool operator==(const FValueRef& Other) const { return Node == Other.Node && Root == Other.Root && Owner == Other.Owner; }
            bool operator!=(const FValueRef& Other) const { return !(*this == Other); }
            FValueRef Child(uint64 Id) const { return {Id, Root, Owner}; }
        };
        inline const WorldObservationPrivate::FNode* Resolve(const WorldObservationPrivate::FStorage& Storage, FValueRef Value)
        { return Value.Owner ? Value.Owner->Get(Value.Node) : Storage.Get(Value.Root, Value.Node); }
        constexpr uint64 QualifiedSemanticPeerBytes = 0;
        struct FOrigin { FValueRef Ref; bool Previous = false; };
        struct FPlayerResult
        {
            FPlayerResult* Next = nullptr;
            FValueRef Raw;
            FOrigin Position, Driving, Water, Weapon, DanceInputs[24];
            uint32 DanceCount = 0, RetireField = 0;
            bool bPeer = false, bIncluded = true, bDanceMerged = false, bControlUnavailable = true, bPinned = false;
            FValueRef& Reference(uint32 Index)
            {
                if (!Index) return Raw;
                if (Index == 1) return Position.Ref;
                if (Index == 2) return Driving.Ref;
                if (Index == 3) return Water.Ref;
                if (Index == 4) return Weapon.Ref;
                return DanceInputs[Index - 5].Ref;
            }
            WorldObservationPrivate::EStep Retire(WorldObservationPrivate::FBudget& Budget);
        };
        struct FFieldSlot
        {
            const TCHAR* Name = nullptr;
            uint32 NameLength = 0;
            FValueRef Value;
            bool bRequired = false;
            FFieldSlot() = default;
            template <SIZE_T N> FFieldSlot(const TCHAR (&InName)[N], bool bInRequired = false)
                : Name(InName), NameLength(N - 1), bRequired(bInRequired) {}
        };

        // Each operation owns its continuation. Begin never scans or allocates.
        // Advance spends from the same budget as page assembly and retirement.
        class FTextRead
        {
        public:
            void Begin(FValueRef Value, FTextRule Rule);
            void Compare(FValueRef Left, FValueRef Right);
            void Compare(FValueRef Left, const WorldObservationPrivate::FStorage& Previous, FValueRef Right);
            void ComparePrefixed(FValueRef Left, FValueRef Right, const TCHAR* Prefix, uint64 PrefixLength);
            void StartsWith(FValueRef Left, const TCHAR* Prefix, uint64 Length);
            void Compare(FValueRef Left, const FString& Right);
            void Compare(FValueRef Left, const TCHAR* Right, uint64 Length);
            WorldObservationPrivate::EStep Advance(const WorldObservationPrivate::FStorage& Storage);
            void Cancel();
            bool Equal() const { return bEqual; }
        private:
            FValueRef Left, Right;
            const WorldObservationPrivate::FStorage* Owner = nullptr;
            const WorldObservationPrivate::FStorage* OtherOwner = nullptr;
            FTextRule Rule;
            const TCHAR* Literal = nullptr;
            const WorldObservationPrivate::FStringBlock* LeftBlock = nullptr;
            const WorldObservationPrivate::FStringBlock* RightBlock = nullptr;
            uint64 Offset = 0, Count = 0, LiteralCount = 0;
            uint16 LeftOffset = 0, RightOffset = 0;
            bool bStarted = false, bCompare = false, bEqual = true, bNonWhitespace = false, bPrefixOnly = false;
            WorldObservationPrivate::EStep Status = WorldObservationPrivate::EStep::Invalid;
        };
        class FObjectRead
        {
        public:
            // Slots must outlive this operation. Unknown fields can be retained
            // without traversing their value when the domain permits extensions.
            void Begin(FValueRef Object, FFieldSlot* Slots, uint32 Count, bool bRejectUnknown);
            WorldObservationPrivate::EStep Advance(const WorldObservationPrivate::FStorage& Storage);
            void Cancel();
        private:
            FValueRef Object;
            FFieldSlot* Slots = nullptr;
            uint32 Count = 0, Slot = 0, Required = 0, Cleared = 0;
            uint64 Key = 0, Value = 0, Seen = 0;
            FTextRead Comparison;
            bool bStarted = false, bComparing = false, bAdvancing = false, bRejectUnknown = false;
            WorldObservationPrivate::EStep Status = WorldObservationPrivate::EStep::Invalid;
        };
        WorldObservationPrivate::EStep ReadNumber(const WorldObservationPrivate::FStorage& Storage,
            FValueRef Value, double& Out);
        WorldObservationPrivate::EStep ReadKind(const WorldObservationPrivate::FStorage& Storage,
            FValueRef Value, EWorldValueKind& Out);

        struct FTextChoice
        {
            const TCHAR* Text;
            uint32 Count;
            template <SIZE_T N> constexpr FTextChoice(const TCHAR (&Value)[N]) : Text(Value), Count(N - 1) {}
        };
        struct FFieldRule
        {
            const TCHAR* Name;
            uint32 NameLength;
            EWorldValueKind Kind;
            bool bRequired = true, bNullable = false, bInteger = false;
            double Minimum = -DBL_MAX, Maximum = DBL_MAX;
            FTextRule Text;
            uint64 MinCount = 0, MaxCount = MAX_uint64;
            const FTextChoice* Choices = nullptr;
            uint32 ChoiceCount = 0;
            uint32 ConditionField = MAX_uint32;
            int32 ConditionChoice = -1;
            template <SIZE_T N> constexpr FFieldRule(const TCHAR (&InName)[N], EWorldValueKind InKind,
                bool bInRequired = true, bool bInNullable = false)
                : Name(InName), NameLength(N - 1), Kind(InKind), bRequired(bInRequired), bNullable(bInNullable) {}
            constexpr FFieldRule WithNumber(double Min, double Max, bool bWhole = false) const
            {
                auto Copy = *this; Copy.Minimum = Min; Copy.Maximum = Max; Copy.bInteger = bWhole; return Copy;
            }
            constexpr FFieldRule WithText(FTextRule Rule) const { auto Copy = *this; Copy.Text = Rule; return Copy; }
            constexpr FFieldRule WithCount(uint64 Min, uint64 Max) const
            { auto Copy = *this; Copy.MinCount = Min; Copy.MaxCount = Max; return Copy; }
            template <SIZE_T N> constexpr FFieldRule WithChoices(const FTextChoice (&Values)[N]) const
            { auto Copy = *this; Copy.Choices = Values; Copy.ChoiceCount = N; return Copy; }
            constexpr FFieldRule When(uint32 Field, int32 Choice) const
            { auto Copy = *this; Copy.ConditionField = Field; Copy.ConditionChoice = Choice; return Copy; }
            bool Number(double Value) const
            {
                return bInteger ? IntegerNumber(Value, Minimum, Maximum)
                    : FiniteNumber(Value) && Value >= Minimum && Value <= Maximum;
            }
        };
        struct FRecordFacts
        {
            static constexpr uint32 Capacity = 32;
            double Numbers[Capacity]{};
            uint64 Counts[Capacity]{};
            int32 Choices[Capacity]{};
            EWorldFieldPresence Presences[Capacity]{};
        };
        struct FRecordSchema
        {
            const FFieldRule* Fields;
            uint32 Count;
            bool bRejectUnknown;
            bool (*Check)(const FRecordFacts&) = nullptr;
            template <SIZE_T N> constexpr FRecordSchema(const FFieldRule (&Rules)[N], bool bExact = true,
                bool (*InCheck)(const FRecordFacts&) = nullptr)
                : Fields(Rules), Count(N), bRejectUnknown(bExact), Check(InCheck) {}
        };
        bool ReadRecord(const TSharedPtr<FJsonObject>& Object, const FRecordSchema& Schema);

        class FRecordRead
        {
        public:
            static constexpr uint32 Capacity = 32;
            static uint64 BeginWork() { return 2 * (sizeof(FObjectRead) + sizeof(FTextRead)) + 128; }
            void Begin(FValueRef Object, const FRecordSchema& Schema);
            WorldObservationPrivate::EStep Advance(const WorldObservationPrivate::FStorage& Storage);
            void Cancel();
            FValueRef Field(uint32 Index) const { return Schema && Index < Schema->Count ? Slots[Index].Value : FValueRef{}; }
            double Number(uint32 Index) const { return Schema && Index < Schema->Count ? Facts.Numbers[Index] : 0; }
            int32 Choice(uint32 Index) const { return Schema && Index < Schema->Count ? Facts.Choices[Index] : -1; }
            uint64 Count(uint32 Index) const { return Schema && Index < Schema->Count ? Facts.Counts[Index] : 0; }
            EWorldFieldPresence Presence(uint32 Index) const
            { return Schema && Index < Schema->Count ? Facts.Presences[Index] : EWorldFieldPresence::Missing; }
        private:
            FValueRef Object;
            const FRecordSchema* Schema = nullptr;
            FFieldSlot Slots[Capacity];
            FRecordFacts Facts;
            FObjectRead Fields;
            FTextRead Text;
            uint32 Initialized = 0, Index = 0, ChoiceIndex = 0;
            enum class EPhase : uint8 { Initialize, Fields, Kind, Text, Choices, Advance, Done };
            EPhase Phase = EPhase::Done;
            bool bTextStarted = false;
            WorldObservationPrivate::EStep Status = WorldObservationPrivate::EStep::Invalid;
        };

        const FRecordSchema& HealthSchema();
        const FRecordSchema& RecoveryDestinationSchema();
        class FHealthRead
        {
        public:
            void Begin(FValueRef Value);
            WorldObservationPrivate::EStep Advance(const WorldObservationPrivate::FStorage& Storage);
            void Cancel();
            bool IsNull() const { return bNull; }
            FValueRef Life() const { return Health.Field(0); }
            int64 Revision() const { return static_cast<int64>(Health.Number(1)); }
            double Value() const { return Health.Number(2); }
            int32 HealthStatus() const { return Health.Choice(4); }
            FValueRef Raw() const { return Root; }
        private:
            FValueRef Root;
            FRecordRead Health, Destination;
            uint64 Position = 0;
            uint8 Phase = 0, Coordinate = 0;
            bool bNull = false;
            WorldObservationPrivate::EStep Status = WorldObservationPrivate::EStep::Invalid;
        };

        enum EBindingField : uint32
        {
            BindingReceipt, BindingUser, BindingApprover, BindingParticipant, BindingCall,
            BindingExpression, BindingReservation, BindingCharacter, BindingHome, BindingGameRoom,
            BindingSavedRevision, BindingInstance, BindingRevision, BindingSequence, BindingHealthRevision,
            BindingPresentationInstance, BindingReadyInstance, BindingPendingHome,
            BindingCapturedAt, BindingPermissionDeadline, BindingPublishBy
        };
        enum EReceiptField : uint32 { ReceiptScope, ReceiptGrant, ReceiptEntry, ReceiptVisit, ReceiptExpires, ReceiptNow };
        enum EVisitField : uint32 { VisitId, VisitLife, VisitInstance, VisitRevision };
        class FBindingRead
        {
        public:
            void Begin(FValueRef Value);
            WorldObservationPrivate::EStep Advance(const WorldObservationPrivate::FStorage& Storage);
            void Cancel();
            const FRecordRead& Binding() const { return BindingRecord; }
            const FRecordRead& Receipt() const { return ReceiptRecord; }
            const FRecordRead& Visit() const { return VisitRecord; }
            const FRecordRead& PendingHome() const { return PendingRecord; }
        private:
            FRecordRead BindingRecord, ReceiptRecord, VisitRecord, PendingRecord;
            uint8 Phase = 0;
            WorldObservationPrivate::EStep Status = WorldObservationPrivate::EStep::Invalid;
        };

        enum class EDomain : uint8
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
            Count
        };
        struct FChildRule { uint32 Field; EDomain Domain; };
        struct FDomainSchema
        {
            const FRecordSchema* Record = nullptr;
            const FChildRule* Children = nullptr;
            uint32 ChildCount = 0;
            EWorldValueKind Kind = EWorldValueKind::Object;
            EDomain Element = EDomain::Number;
            uint64 MinCount = 0, MaxCount = MAX_uint64;
            double Minimum = -DBL_MAX, Maximum = DBL_MAX;
            bool bInteger = false;
        };
        const FDomainSchema& DomainSchema(EDomain Domain);
        const FDomainSchema& MovementDomainSchema(EDomain Domain);
        const FDomainSchema& RacingDomainSchema(EDomain Domain);
        const FDomainSchema& SelectionDomainSchema(EDomain Domain);

        // The calling owner must pin the same storage from Begin through completion
        // or Cancel. None of these readers retains ownership or grants publication.
        // The stack is bounded by the static domain grammar, not by the depth of
        // retained room/layout/mechanic values. Unknown bodies are never flattened.
        class FDomainRead
        {
        public:
            FDomainRead() {}
            static uint64 DeferredBytes() { return sizeof(Frames); }
            void Begin(FValueRef Value, EDomain Domain);
            WorldObservationPrivate::EStep Advance(const WorldObservationPrivate::FStorage& Storage);
            void Cancel();
            const FRecordRead& RootRecord() const { return FrameAt(0).Record; }
            FValueRef RaceInstance() const { return RacingInstance; }
            int64 RaceUpdatedAt() const { return RacingUpdatedAt; }
            FValueRef Library() const { return SelectedLibrary; }
            FValueRef Character() const { return SelectedCharacter; }
            FValueRef Home() const { return SelectedHome; }
            FValueRef ActiveRoom() const { return SelectedActiveRoom; }
        private:
            struct FFrame
            {
                FValueRef Value;
                EDomain Domain = EDomain::Number;
                FRecordRead Record;
                uint64 Next = 0, Count = 0, Index = 0;
                double Scalar = 0, Tuple[3]{};
                FValueRef SeenUsers[12];
                uint8 Selected = 0;
                uint64 Lanes = 0;
                FTextRead Comparison;
                uint32 Relation = 0, ParentField = MAX_uint32;
                bool bComparing = false;
                FMovementPoint MovementPoints[32];
                FMovementPose MovementFrom;
                uint32 MovementPointCount = 0;
                uint32 Child = 0;
                uint8 Phase = 0;
            };
            static constexpr uint32 FrameCount = 12;
            static_assert(std::is_trivially_destructible_v<FFrame>, "Deferred frames must not own resources.");
            struct FFrameSlot { alignas(FFrame) uint8 Bytes[sizeof(FFrame)]; };
            FFrameSlot Frames[FrameCount];
            FFrame& FrameAt(uint32 Index) { return *reinterpret_cast<FFrame*>(Frames[Index].Bytes); }
            const FFrame& FrameAt(uint32 Index) const { return *reinterpret_cast<const FFrame*>(Frames[Index].Bytes); }
            uint32 InitializedFrames = 0;
            FValueRef StartValue;
            EDomain StartDomain = EDomain::Number;
            bool bBegin = false;
            uint32 Depth = 0;
            FValueRef RacingInstance;
            int64 RacingUpdatedAt = 0;
            FValueRef SelectedLibrary, SelectedCharacter, SelectedHome, SelectedActiveRoom;
            WorldObservationPrivate::EStep Status = WorldObservationPrivate::EStep::Invalid;
            WorldObservationPrivate::EStep Relations(const WorldObservationPrivate::FStorage& Storage);
        };
    }

    // Semantic entry points shared with the legacy text decoders. The segmented
    // owner must qualify and charge a projection before allocating these objects.
    // These functions do not establish permission, timing or resource admission.
    bool ReadWorldPlayerObject(const TSharedPtr<FJsonObject>& Player, int64 ServerNowMs,
        TSharedPtr<const FWorldPlayerView>& Out, FString& OutError);
    bool ReadWorldInstanceObject(const TSharedPtr<FJsonObject>& Instance, const FWorldPlayerView& Player,
        TSharedPtr<const FWorldCurrentInstanceIdentity>& Out, FString& OutError);
    bool ReadWorldRecoveryValue(const TSharedPtr<FJsonValue>& Value, ERecoveryResponseKind Kind,
        FRecoveryDecodedResponse& Out, FString& OutError);
    ERacingReadResult ReadWorldRacingValue(EWorldFieldPresence Presence, TSharedPtr<FJsonValue> Value,
        FString ExpectedInstanceId, TSharedPtr<const FRacingSnapshot> Previous,
        TSharedPtr<const FRacingSnapshot>& Out, FString& OutError);
}
