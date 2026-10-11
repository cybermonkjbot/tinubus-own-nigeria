#pragma once

#include "CoreMinimal.h"
#include "BaddieCompanionResidentState.h"
#include "BaddieCompanionMovementProtocol.h"

namespace BaddieCompanion
{
    enum class EPlayerVehicleLocationKind : uint8 { Place, Road };
    struct FPlayerVehicleLocation
    {
        EPlayerVehicleLocationKind Kind = EPlayerVehicleLocationKind::Place;
        FString WorldKey, PlaceId, LayoutId;
        TOptional<FString> HomeOwnerId, HomeRoomId;
        // Source geographic coordinates, not an Unreal transform.
        double Latitude = 0, Longitude = 0;
    };
    struct FPlayerVehicleParked
    {
        FString NavigationRevision, InstanceId;
        FWorldDrivingPose Pose;
        double FuelMetres = 0;
    };
    struct FPlayerVehicle
    {
        FString VehicleId;
        TOptional<FString> OwnerUserId, ActiveTripId;
        FResidentVehicleModel Model;
        int64 Revision = 0, FuelUnits = 0, Condition = 0, OdometerMetres = 0;
        FPlayerVehicleLocation Location;
        bool bForSale = false, bForHire = false;
        // The required asset field is validated as null; no asset identity is invented.
        // Missing: false/unset. Explicit null: true/unset. Value: true/set.
        bool bHasDrivingField = false, bHasDrivingParkedField = false;
        TOptional<FWorldDrivingControl> Driving;
        TOptional<FPlayerVehicleParked> DrivingParked;
        // Optional, never nullable.
        TOptional<double> DrivingFuelMetres;
    };
    struct FPlayerVehicleCatalog
    {
        TArray<FPlayerVehicle> Owned, Sale, Hire;
    };
    struct FPlayerVehicleRecoveryTarget
    {
        FString VehicleId, PlaceId;
        int64 VehicleRevision = 0;
    };
    struct FPlayerVehicleRecovery
    {
        TArray<FPlayerVehicle> Owned;
        TArray<FPlayerVehicleRecoveryTarget> Targets;
    };
    struct FPlayerVehicleScope
    {
        FString GameRoomId, InstanceId, LifeId, ServiceId;
        int64 Revision = 0;
    };

    // Object roots only. Failure clears the whole output and returns a fixed diagnostic.
    // Input may alias text in an older output. Neither decoder grants gameplay access.
    BADDIECOMPANION_API bool DecodePlayerVehicleCatalog(const FString& Json,
        FPlayerVehicleCatalog& Out, FString& OutError);
    BADDIECOMPANION_API bool DecodePlayerVehicleRecovery(const FString& Json,
        FPlayerVehicleRecovery& Out, FString& OutError);

    enum class EPlayerVehicleReadKind : uint8 { Catalog, Recovery };
    enum class EPlayerVehicleReadMethod : uint8 { Mutation, Query };
    class FPlayerVehiclesState;
    // Opaque process-local freshness receipt. Copying it creates no permission.
    class FPlayerVehicleReadReceipt
    {
        friend class FPlayerVehiclesState;
        friend class FPlayerVehicleReadIntent;
        FGuid Owner, Selection, Operation, Stage;
        EPlayerVehicleReadKind Kind = EPlayerVehicleReadKind::Catalog;
    };
    class BADDIECOMPANION_API FPlayerVehicleReadIntent
    {
    public:
        EPlayerVehicleReadKind GetKind() const { return Receipt.Kind; }
        EPlayerVehicleReadMethod GetMethod() const;
        const TCHAR* GetEndpoint() const;
        const FPlayerVehicleReadReceipt& GetReceipt() const { return Receipt; }
        // Catalog has exactly five non-auth arguments. Recovery has none.
        const TOptional<FPlayerVehicleScope>& GetArguments() const { return Arguments; }
    private:
        friend class FPlayerVehiclesState;
        FPlayerVehicleReadReceipt Receipt;
        TOptional<FPlayerVehicleScope> Arguments;
    };
    struct FPlayerVehiclesSnapshot
    {
        TOptional<FPlayerVehicleScope> Scope;
        TOptional<FPlayerVehicleCatalog> Catalog;
        TOptional<FPlayerVehicleRecovery> Recovery;
        bool bRecoveryOnly = false, bReady = false, bLoading = false;
        TOptional<FString> Error;
    };

    // Single-thread caller-driven module. No transport, listener ownership or commands.
    // Capture intents before dispatch. StopReads BEFORE external cancellation, sheet
    // closure, last-observer removal, feature disabling or prepared departure.
    class BADDIECOMPANION_API FPlayerVehiclesState
    {
    public:
        FPlayerVehiclesState();
        FPlayerVehiclesState(const FPlayerVehiclesState&) = delete;
        FPlayerVehiclesState& operator=(const FPlayerVehiclesState&) = delete;
        void BeginOwner(const FString& AccountId);
        void Select(const TOptional<FPlayerVehicleScope>& Scope, bool bRecoveryOnly = false);
        TOptional<FPlayerVehicleReadIntent> StartRead();
        // Catalog success may return the sole required recovery continuation.
        // Stale, duplicate and wrong-kind ingress returns no continuation and changes nothing.
        TOptional<FPlayerVehicleReadIntent> AcceptCatalog(const FPlayerVehicleReadReceipt& Receipt, const FString& Json);
        void AcceptRecovery(const FPlayerVehicleReadReceipt& Receipt, const FString& Json);
        void FailRead(const FPlayerVehicleReadReceipt& Receipt);
        void StopReads();
        // Inert until a new explicit BeginOwner, including Select/StartRead.
        void Dispose();
        TSharedRef<const FPlayerVehiclesSnapshot> GetSnapshot() const { return Snapshot; }
        FSimpleMulticastDelegate OnChanged;
    private:
        bool Current(const FPlayerVehicleReadReceipt& Receipt) const;
        FPlayerVehicleReadIntent Capture(EPlayerVehicleReadKind Kind);
        void Retire();
        void Publish(FPlayerVehiclesSnapshot Next);
        FGuid Owner, Selection, Operation;
        bool bActive = false;
        TOptional<FPlayerVehicleReadReceipt> Pending;
        TOptional<FPlayerVehicleCatalog> StagedCatalog;
        TSharedRef<const FPlayerVehiclesSnapshot> Snapshot;
    };
}
