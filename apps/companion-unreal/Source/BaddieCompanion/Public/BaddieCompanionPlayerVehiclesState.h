#pragma once

#include "CoreMinimal.h"
#include "BaddieCompanionResidentState.h"
#include "BaddieCompanionMovementProtocol.h"
#include "BaddieCompanionGameEconomyState.h"

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
    // Opaque immutable structured service, including bounded extension values.
    struct FPlayerVehicleQuoteDocument;
    struct FPlayerVehicleQuote
    {
        FGameEconomyQuote Value;
        TSharedPtr<const FPlayerVehicleQuoteDocument> Document;
    };
    enum class EPlayerVehiclePurpose : uint8 { Purchase, Fuel, Fare };
    struct FPlayerVehicleConfirmation
    {
        FPlayerVehicleQuote Quote;
        FString VehicleId;
        int64 VehicleRevision = 0, Quantity = 0, FuelBefore = 0, FuelAfter = 0, TankCapacityUnits = 0;
        EPlayerVehiclePurpose Purpose = EPlayerVehiclePurpose::Purchase;
        TOptional<FString> DepartureId;
        TOptional<FPlayerVehicleLocation> Destination;
        TOptional<double> RouteMetres;
    };
    struct FPlayerVehicleWorldTravel
    {
        FString PlaceId;
        TOptional<FPlayerVehicleRecoveryTarget> RoadsideTarget;
    };
    struct FPlayerVehicleDeparture
    {
        FPlayerVehicleScope Scope;
        FString DepartureId, VehicleId, Mode, ServiceId;
        int64 VehicleRevision = 0, ExpiresAtMs = 0;
        FPlayerVehicleLocation Destination;
        double RouteMetres = 0;
        TOptional<FPlayerVehicleWorldTravel> WorldTravel;
    };
    struct FPlayerVehicleFareCommit
    {
        FPlayerVehicleScope Scope;
        FPlayerVehicleDeparture Departure;
        FPlayerVehicleConfirmation Confirmation;
        FString RequestId;
    };
    BADDIECOMPANION_API bool DecodePlayerVehicleConfirmation(const FString& Json, FPlayerVehicleConfirmation& Out, FString& OutError);
    BADDIECOMPANION_API bool DecodePlayerVehicleDeparture(const FString& Json, FPlayerVehicleDeparture& Out, FString& OutError);
    BADDIECOMPANION_API bool DecodePlayerVehicleQuote(const FString& Json, FPlayerVehicleQuote& Out, FString& OutError);

    // Object roots only. Failure clears the whole output and returns a fixed diagnostic.
    // Input may alias text in an older output. Neither decoder grants gameplay access.
    BADDIECOMPANION_API bool DecodePlayerVehicleCatalog(const FString& Json,
        FPlayerVehicleCatalog& Out, FString& OutError);
    BADDIECOMPANION_API bool DecodePlayerVehicleRecovery(const FString& Json,
        FPlayerVehicleRecovery& Out, FString& OutError);

    enum class EPlayerVehicleReadKind : uint8 { Catalog, Recovery, Confirmation };
    enum class EPlayerVehicleReadMethod : uint8 { Mutation, Query };
    class FPlayerVehiclesState;
    // Opaque process-local freshness receipt. Copying it creates no permission.
    class FPlayerVehicleReadReceipt
    {
        friend class FPlayerVehiclesState;
        friend class FPlayerVehicleReadIntent;
        FGuid Owner, Selection, Operation, Stage;
        FString QuoteId;
        EPlayerVehicleReadKind Kind = EPlayerVehicleReadKind::Catalog;
    };
    class BADDIECOMPANION_API FPlayerVehicleReadIntent
    {
    public:
        EPlayerVehicleReadKind GetKind() const { return Receipt.Kind; }
        EPlayerVehicleReadMethod GetMethod() const;
        const TCHAR* GetEndpoint() const;
        const FPlayerVehicleReadReceipt& GetReceipt() const { return Receipt; }
        // Catalog has five scope arguments; recovery has none; confirmation has quoteId.
        const TOptional<FPlayerVehicleScope>& GetArguments() const { return Arguments; }
        const FString& GetQuoteId() const { return Receipt.QuoteId; }
    private:
        friend class FPlayerVehiclesState;
        FPlayerVehicleReadReceipt Receipt;
        TOptional<FPlayerVehicleScope> Arguments;
    };
    enum class EPlayerVehicleActionKind : uint8 { PurchaseQuote, FuelQuote, FareQuote, Confirm, Cancel };
    struct FPlayerVehicleAction
    {
        EPlayerVehicleActionKind Kind = EPlayerVehicleActionKind::PurchaseQuote;
        // Vehicle ID for purchase/fuel, departure ID for fare, quote ID for confirm/cancel.
        FString Value;
        int64 Quantity = 0;
    };
    struct FPlayerVehicleCommandPacket
    {
        TOptional<FPlayerVehicleScope> Scope;
        FPlayerVehicleAction Action;
        FString RequestId;
        TOptional<FPlayerVehicleConfirmation> Confirmation;
        TOptional<FPlayerVehicleDeparture> Departure;
        TOptional<FPlayerVehicleFareCommit> Fare;
        TOptional<FPlayerVehicleQuote> IssuedQuote;
    };
    class FPlayerVehicleCommandReceipt
    {
        friend class FPlayerVehiclesState;
        FGuid Owner, Request, Attempt, View;
    };
    class FPlayerVehicleCommandIntent
    {
        friend class FPlayerVehiclesState;
    public:
        const FPlayerVehicleCommandReceipt& GetReceipt() const { return Receipt; }
        const FPlayerVehicleCommandPacket& GetPacket() const { return *Packet; }
        const FString& GetEndpoint() const { return Endpoint; }
        const FString& GetArgumentsJson() const { return ArgumentsJson; }
    private:
        FPlayerVehicleCommandReceipt Receipt;
        TSharedPtr<const FPlayerVehicleCommandPacket> Packet;
        FString Endpoint, ArgumentsJson;
    };
    enum class EPlayerVehicleCommandPreparation : uint8 { Unavailable, Ready, FareHostUnavailable };
    struct FPlayerVehicleCommandStart
    {
        EPlayerVehicleCommandPreparation Result = EPlayerVehicleCommandPreparation::Unavailable;
        TOptional<FPlayerVehicleCommandIntent> Intent;
        // Missing native host is explicit. This descriptor cannot dispatch or complete.
        TOptional<FPlayerVehicleFareCommit> RequiredFareHost;
    };
    enum class EPlayerVehicleCommandFailure : uint8 { Uncertain, AuthoritativeRejection };
    struct FPlayerVehicleCommandResult
    {
        TOptional<FPlayerVehicleQuote> Quote;
        TOptional<FPlayerVehicleReadIntent> FollowUp;
    };
    struct FPlayerVehiclesSnapshot
    {
        TOptional<FPlayerVehicleScope> Scope;
        TOptional<FPlayerVehicleCatalog> Catalog;
        TOptional<FPlayerVehicleRecovery> Recovery;
        TOptional<FString> QuoteId;
        TOptional<FPlayerVehicleConfirmation> Confirmation;
        TOptional<FPlayerVehicleScope> ConfirmationScope;
        TOptional<FPlayerVehicleDeparture> Departure;
        bool bRecoveryOnly = false, bReady = false, bLoading = false;
        bool bBusy = false, bRetryable = false;
        // No native prepared-travel host exists. This is not caller-configurable.
        bool bFareReady = false;
        TOptional<FString> Error, Notice;
    };

    // Single-thread caller-driven module. No transport, listener ownership or settlement.
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
        void SelectDeparture(const TOptional<FPlayerVehicleDeparture>& Departure);
        TOptional<FPlayerVehicleReadIntent> StartRead();
        TOptional<FPlayerVehicleReadIntent> Reload();
        TOptional<FPlayerVehicleReadIntent> RecoverQuote(const FString& QuoteId);
        void AcceptConfirmation(const FPlayerVehicleReadReceipt& Receipt, const FString& Json);
        FPlayerVehicleCommandStart Send(const FPlayerVehicleAction& Action, const FString& RequestId);
        FPlayerVehicleCommandStart Retry();
        FPlayerVehicleCommandResult AcceptCommand(const FPlayerVehicleCommandReceipt& Receipt, const FString& Json);
        void FailCommand(const FPlayerVehicleCommandReceipt& Receipt, EPlayerVehicleCommandFailure Failure);
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
        TOptional<FPlayerVehicleReadIntent> CaptureConfirmation(const TOptional<FPlayerVehicleScope>& Scope);
        void RetireConfirmation();
        bool CurrentCommand(const FPlayerVehicleCommandReceipt& Receipt) const;
        FPlayerVehicleCommandStart Submit(bool bReplay);
        void Retire();
        void Publish(FPlayerVehiclesSnapshot Next);
        FGuid Owner, Selection, Operation;
        bool bActive = false;
        TOptional<FPlayerVehicleReadReceipt> Pending;
        TOptional<FPlayerVehicleCatalog> StagedCatalog;
        FGuid QuoteRead, Request, Attempt;
        TOptional<FPlayerVehicleReadReceipt> PendingConfirmation;
        TOptional<FPlayerVehicleScope> ReadConfirmationScope;
        TSharedPtr<const FPlayerVehicleCommandPacket> ReadExpected, QuoteExpected, PendingCommand;
        bool bInFlight = false;
        TSharedRef<const FPlayerVehiclesSnapshot> Snapshot;
    };
}
