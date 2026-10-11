#pragma once

#include "BaddieCompanionWorldPlayerRead.h"
#include "BaddieCompanionHomePresentation.h"
#include "BaddieCompanionHomeMappingInternal.h"

namespace BaddieCompanion::WorldObservationPrivate
{
    // Target allocator, control-block, mapper and callback costs are not qualified.
    constexpr uint64 QualifiedPublicationBytes = 0;
    constexpr uint64 QualifiedPublicationPreparationBytes = 0;
    constexpr uint64 QualifiedPublicationInitializationWork = 0;
    constexpr uint64 QualifiedPayloadInitializationWork = 0;
    constexpr uint64 QualifiedKnownObjectInitializationWork = 0;
    constexpr uint64 QualifiedPublicationCommitWork = 0;
    constexpr uint64 QualifiedAccessBytes = 0;
    constexpr uint64 QualifiedAccessInitializationWork = 0;
    constexpr uint64 QualifiedAccessChargeBytes = 0;
    constexpr uint64 QualifiedAccessReleaseWork = 0;
    constexpr uint64 QualifiedAccessReadWork = 0;
    constexpr uint64 QualifiedHealthBytes = 0;
    constexpr uint64 QualifiedHealthInitializationWork = 0;
    constexpr uint64 QualifiedClockBytes = 0;
    constexpr uint64 QualifiedHomeBytes = 0;
    constexpr uint64 QualifiedNotificationWork = 0;
    constexpr uint64 QualifiedMovementWindowBytes = 0;
    constexpr uint64 QualifiedMovementWindowWork = 0;
    constexpr uint64 QualifiedMovementRouteInitializationWork = 0;
    constexpr uint64 QualifiedTextAllocationWork = 0;
    constexpr uint64 QualifiedTextReleaseWork = 0;
    constexpr uint64 QualifiedClockWork = 0;
    constexpr uint64 QualifiedHealthReleaseWork = 0;
    constexpr uint64 QualifiedHomeReleaseWork = 0;
    constexpr uint64 QualifiedHomeOperationWork = 0;
    constexpr uint64 QualifiedHomeAllocationBytes = 0;
    constexpr uint64 QualifiedHomeRoomCopyWork = 0;
    constexpr uint64 QualifiedHomeRoomCopyBytes = 0;
    constexpr uint64 QualifiedNativeContentObservationWork = 0;
    constexpr uint64 QualifiedHomeBorrowWork = 0;

    class FHomeDataRetirement
    {
    public:
        EStep Advance(FBudget& Budget, FHomePresentationData& Data);
    private:
        EStep Room(FBudget& Budget, FRoomPlan& Value);
        EStep Shell(FBudget& Budget, FRoomShellPlan& Value);
        uint32 Phase = 0, Index = 0, Field = 0;
        uint32 RoomPhase = 0, RoomIndex = 0, RoomField = 0;
        uint32 ShellPhase = 0, ShellIndex = 0, ShellField = 0;
    };

    struct FPublicationClock
    {
        FWorldPresentationClock Value;
        FString GameRoomId;
        uint64 Charge = 0;
        EStep Retire(FBudget& Budget);
    };

    struct FPublicationPayload
    {
        WorldReadPrivate::FPlayerResult* Local = nullptr;
        FWorldPlayerHeader Player;
        FWorldInstanceHeader Instance;
        WorldReadPrivate::FValueRef Selection, Health, InstanceRoot, HomeRoom;
        TSharedPtr<FPublicationClock> Clock;
        TSharedPtr<FPublicationClock> RetiredClock;
        FRecoveryReadView RetiredRecovery;
        FRecoveryReadView NextRecovery;
        TSharedPtr<FCompanionHealthView> HealthValue;
        FHomePresentationData Home;
        HomeMapping::FUnknownRanges HomeUnknown;
        FHomeDataRetirement HomeRetirement;
        TOptional<FMovementRoute> Movement;
        bool bHomeMapped = false, bComplete = false;
        bool bHomeMappingFinished = false, bHomePreparationStarted = false;
        uint64 Charge = 0;
        uint32 RetirePhase = 0;
        uint64 NativeCharge = 0;
        uint64 HealthCharge = 0;
        uint64 HomeCharge = 0;
        uint32 HealthRetirePhase = 0;
        EStep Retire(FBudget& Budget);
    };

    class FHomeBorrow
    {
    public:
        FHomeBorrow() = default;
        ~FHomeBorrow();
        FHomeBorrow(const FHomeBorrow&) = delete;
        FHomeBorrow& operator=(const FHomeBorrow&) = delete;
    private:
        friend class BaddieCompanion::FWorldObservation;
        friend class ::UBaddieCompanionWorldPresentation;
        TSharedPtr<FStorage> Storage;
        const FHomePresentationData* Data = nullptr;
    };

    struct FPublicationEvidence
    {
        uint64 Lifetime = 0, CredentialGeneration = 0, RequestSequence = 0, OperationSequence = 0;
        uint64 Ordering = 0, AcceptedPublication = 0;
        int32 EntrySequence = 0;
        const void* Visit = nullptr;
        const void* Static = nullptr;
        const void* Semantic = nullptr;
        uint64 SemanticGeneration = 0;
        TCHAR RequestId[33]{}, Capture[33]{}, Directory[65]{};
    };

    // Exact text preparation. Reserve destination capacity before allocation;
    // copying resumes at the source block and initialized destination offset.
    class FPreparedText
    {
    public:
        void Begin(WorldReadPrivate::FValueRef Value);
        EStep Advance(FStorage& Storage, FString& Out, uint64 QualifiedOverhead);
        EStep Retire(FBudget& Budget, FString& Out);
        uint64 TakeCharge() { const uint64 Out = Charge; Charge = 0; return Out; }
    private:
        WorldReadPrivate::FValueRef Ref;
        const FStringBlock* Block = nullptr;
        uint64 Offset = 0, Units = 0, Charge = 0;
        bool bAllocated = false;
    };

    // Copies only bounded mapper fields. Unknown members stay in the source root.
    class FKnownHomeInput
    {
    public:
        void Begin(WorldReadPrivate::FValueRef Object, const WorldReadPrivate::FRecordSchema& Schema);
        EStep Advance(FStorage& Storage, HomeMapping::FKnownObject& Out);
        EStep Retire(FStorage& Storage, HomeMapping::FKnownObject& Out);
        uint64 TakeCharge() { const uint64 Value = Charge; Charge = 0; return Value; }
        uint64 TakeRetirementCharge() { Charge += Text.TakeCharge(); return TakeCharge(); }
        void Cancel() { Fields.Cancel(); Rules = nullptr; Source = {}; }
        uint64 Charge = 0;
    private:
        WorldReadPrivate::FObjectRead Fields;
        WorldReadPrivate::FFieldSlot Slots[24];
        WorldReadPrivate::FValueRef Source;
        const WorldReadPrivate::FRecordSchema* Rules = nullptr;
        FPreparedText Text;
        uint32 Phase = 0, Index = 0, RetireIndex = 0;
        bool bTextStarted = false;
    };
    class FHomePreparation
    {
    public:
        // User-provided construction leaves raw known slots uninitialized. Each
        // record is constructed only by the admitted initialization loop.
        FHomePreparation() {}
        void Begin(WorldReadPrivate::FValueRef Character, WorldReadPrivate::FValueRef Room);
        EStep Advance(FStorage& Storage);
        EStep Retire(FStorage& Storage);
        static uint64 DeferredBytes() { return sizeof(FKnownSlots); }
    private:
        struct FKnownSlots
        {
            struct FSlot { alignas(HomeMapping::FKnownObject) uint8 Bytes[sizeof(HomeMapping::FKnownObject)]; } Slots[12];
            HomeMapping::FKnownObject& operator[](uint32 Index)
            { return *reinterpret_cast<HomeMapping::FKnownObject*>(Slots[Index].Bytes); }
        };
        WorldReadPrivate::FValueRef Character, Room, Appearance, Design, Items;
        WorldReadPrivate::FValueRef ItemRefs[8];
        FKnownSlots Known;
        FKnownHomeInput Input;
        FCharacterSelection CharacterIdentity;
        FRoomSelection RoomIdentity;
        HomeMapping::FRoomMappingInput RoomInput;
        HomeMapping::FAppearanceMapping AppearanceMapping;
        HomeMapping::FRoomMapping RoomMapping;
        HomeMapping::FApartmentMapping ApartmentMapping;
        FString Path;
        uint64 ItemNext = 0;
        uint64 ScratchCharge = 0;
        uint32 Phase = 0, Index = 0, ItemCount = 0;
        uint32 RetirePhase = 0, RetireIndex = 0, RetireField = 0, InitializedKnown = 0;
        bool bStarted = false, bMapped = true, bRoomFinished = false, bInputComplete = false;
        bool bDesignPresent = false, bDesignObject = false, bItemsArray = false;
        bool bAppearanceStarted = false, bRoomStarted = false, bApartmentStarted = false;
    };

    class FPublicationPreparation
    {
    public:
        FPublicationPreparation() {}
        FPublicationEvidence Evidence;
        WorldReadPrivate::FValueRef SelectedCharacter, SelectedRoom, InstanceHome;
        WorldReadPrivate::FValueRef OwnerId, HealthLife;
        const FString* ExpectedLife = nullptr;
        const FString* ExpectedGameRoom = nullptr;
        const TSharedPtr<FPublicationClock>* Baseline = nullptr;
        const FRecoveryReadState* RecoveryOwner = nullptr;
        FWorldRecoveryReceipt RecoveryReceipt;
        double TicketArrival = 0;
        int64 ServerNowMs = 0;
        bool bOwnHome = false;
        EStep Begin(FStorage& Storage);
        EStep Advance(FStorage& Storage);
        EStep Retire(FStorage& Storage);
        void Cancel() { bCancelled = true; }
        bool Complete() const { return Phase == 255 && !bCancelled; }
        static uint64 InitializationWork() { return sizeof(FPublicationPreparation) - FHomePreparation::DeferredBytes(); }
        uint64 Charge = 0;
        FStorage* Owner = nullptr;
        bool bCancelled = false;
    private:
        WorldReadPrivate::FRecordRead Record, HealthRecord, DestinationRecord;
        WorldReadPrivate::FHealthRead Health;
        WorldReadPrivate::FTextRead Compare;
        WorldReadPrivate::FValueRef LocalRaw, Position, Instance, Navigation, Selection;
        WorldReadPrivate::FValueRef MovementRef, RouteFrom, RoutePath;
        WorldReadPrivate::FValueRef LayoutId, NavigationRevision, TransportNavigation, WaterScope, WaterLayout, WaterRevision;
        FPreparedText Text;
        FHomePreparation Home;
        uint64 RouteNext = 0, DestinationNext = 0;
        uint32 Coordinate = 0, TextStage = 0;
        uint8 TransportStage = 0;
        bool bTextStarted = false, bClockPrepared = false, bHealthAllocated = false, bHomeStarted = false;
        bool bOwnPreparation = false;
        uint32 Phase = 0;
        bool bStarted = false;
    };

    const WorldReadPrivate::FRecordSchema* ReadSchema(EWorldReadDomain Domain);
    EWorldReadDomain ChildDomain(EWorldReadDomain Parent, uint32 Field);
    EWorldReadDomain ElementDomain(EWorldReadDomain Parent);
}
