#include "BaddieCompanionPlayerVehiclesState.h"
#include "BaddieCompanionStateJson.h"
#include "BaddieCompanionMovementProtocolRead.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Policies/CondensedJsonPrintPolicy.h"

namespace BaddieCompanion
{
    struct FPlayerVehicleQuoteDocument
    {
        // Never exposed as a mutable JSON pointer; created from the private parsed tree.
        TSharedPtr<FJsonValue> Service;
    };
}

namespace BaddieCompanion
{
    namespace
    {
        using namespace StateJsonPrivate;
        constexpr FParseLimits Limits{40000000, 16, 20000};
        constexpr const TCHAR* ReadError = TEXT("Transport details couldn't load. Reload to retry.");

        // FString uses UTF-16 or UTF-32 depending on platform. Count source UTF-16
        // units so supplementary characters consume two units on either platform.
        int64 Units(const FString& Text)
        {
            int64 Count = 0;
            for (const TCHAR C : Text) Count += sizeof(TCHAR) > 2 && static_cast<uint32>(C) > 0xffff ? 2 : 1;
            return Count;
        }
        bool Text(const FValue& Value, FString& Out, int32 Maximum)
        { return String(Value, Out) && !Out.IsEmpty() && Units(Out) <= Maximum; }
        bool KeyText(const FString& Text)
        {
            if (Text.IsEmpty() || Text.Len() > 160) return false;
            for (int32 I = 0; I < Text.Len(); ++I)
            {
                const TCHAR C = Text[I];
                const bool AlphaNumeric = (C >= TEXT('A') && C <= TEXT('Z'))
                    || (C >= TEXT('a') && C <= TEXT('z')) || (C >= TEXT('0') && C <= TEXT('9'));
                if (!AlphaNumeric && (I == 0 || (C != TEXT('.') && C != TEXT('_') && C != TEXT(':') && C != TEXT('-')))) return false;
            }
            return true;
        }
        bool Key(const FValue& Value, FString& Out) { return String(Value, Out) && KeyText(Out); }
        bool BoundedInteger(const FValue& Value, int64& Out, int64 Maximum = MaxInteger - 1)
        { return Integer(Value, Out) && Out <= Maximum; }
        bool Literal(const FValue& Value, const TCHAR* Expected)
        { FString S; return String(Value, S) && Same(S, Expected); }

        // Bound every parsed value, including extensions accepted by shared driving
        // readers. No unbounded nested string/array reaches those readers.
        bool BoundedValue(const FValue& Value, int32 StringLimit, int32 Depth, int32& Nodes, int32 ArrayLimit = 36)
        {
            if (!Value.IsValid() || Depth > 16 || ++Nodes > 20000) return false;
            if (Value->Type == EJson::String) { FString S; return String(Value, S) && Units(S) <= StringLimit; }
            if (Value->Type == EJson::Number) { double N; return Number(Value, N); }
            if (Value->Type == EJson::Array)
            {
                if (Value->AsArray().Num() > ArrayLimit) return false;
                for (const auto& Item : Value->AsArray()) if (!BoundedValue(Item, StringLimit, Depth + 1, Nodes, ArrayLimit)) return false;
            }
            if (Value->Type == EJson::Object)
            {
                for (const auto& Pair : Value->AsObject()->Values)
                {
                    const FString Name(Pair.Key.Len(), *Pair.Key);
                    if (Units(Name) > 240 || !BoundedValue(Pair.Value, StringLimit, Depth + 1, Nodes, ArrayLimit)) return false;
                }
            }
            return Value->Type != EJson::None;
        }
        bool BoundedObject(const FObject& Object, int32 StringLimit)
        {
            int32 Nodes = 0;
            return Object.IsValid() && BoundedValue(MakeShared<FJsonValueObject>(Object), StringLimit, 0, Nodes);
        }
        bool ParseRoot(const FString& Json, FObject& Out)
        { return Units(Json) <= Limits.Characters && Parse(Json, Out, Limits) && BoundedObject(Out, 160000); }
        bool Model(const FValue& Value, FResidentVehicleModel& Out)
        {
            const auto O = Object(Value);
            if (!Only(O, {TEXT("catalogueId"), TEXT("catalogueRevision"), TEXT("modelId"), TEXT("kind"), TEXT("tankCapacityUnits"), TEXT("metresPerFuelUnit")})
                || !Key(Field(O, TEXT("catalogueId")), Out.CatalogueId) || !Same(Out.CatalogueId, TEXT("lagos-player-transport"))
                || !Integer(Field(O, TEXT("catalogueRevision")), Out.CatalogueRevision) || Out.CatalogueRevision != 1
                || !Key(Field(O, TEXT("modelId")), Out.ModelId)
                || !EnumValue(Field(O, TEXT("kind")), Out.Kind, {{TEXT("car"), EResidentVehicleKind::Car}, {TEXT("okada"), EResidentVehicleKind::Okada},
                    {TEXT("keke"), EResidentVehicleKind::Keke}, {TEXT("danfo"), EResidentVehicleKind::Danfo}})
                || !Integer(Field(O, TEXT("tankCapacityUnits")), Out.TankCapacityUnits)
                || !Integer(Field(O, TEXT("metresPerFuelUnit")), Out.MetresPerFuelUnit)) return false;
            const TCHAR* Id = nullptr;
            int64 Tank = 0, Metres = 0;
            switch (Out.Kind)
            {
            case EResidentVehicleKind::Car: Id = TEXT("car"); Tank = 60; Metres = 800; break;
            case EResidentVehicleKind::Okada: Id = TEXT("okada"); Tank = 16; Metres = 1000; break;
            case EResidentVehicleKind::Keke: Id = TEXT("keke"); Tank = 24; Metres = 700; break;
            case EResidentVehicleKind::Danfo: Id = TEXT("danfo"); Tank = 80; Metres = 500; break;
            }
            return Id && Same(Out.ModelId, Id) && Out.TankCapacityUnits == Tank && Out.MetresPerFuelUnit == Metres;
        }
        bool Location(const FValue& Value, FPlayerVehicleLocation& Out)
        {
            const auto O = Object(Value);
            if (!O.IsValid() || !String(Field(O, TEXT("worldKey")), Out.WorldKey) || !Same(Out.WorldKey, TEXT("lagos-v1"))) return false;
            if (Literal(Field(O, TEXT("kind")), TEXT("place")))
            {
                if (!Only(O, {TEXT("kind"), TEXT("worldKey"), TEXT("placeId"), TEXT("layoutId"), TEXT("homeOwnerId"), TEXT("homeRoomId")})
                    || !Key(Field(O, TEXT("placeId")), Out.PlaceId) || !Text(Field(O, TEXT("layoutId")), Out.LayoutId, 160000)
                    || !Nullable(Field(O, TEXT("homeOwnerId")), Out.HomeOwnerId, Key)
                    || !Nullable(Field(O, TEXT("homeRoomId")), Out.HomeRoomId, Key)) return false;
                return Same(Out.PlaceId, TEXT("own-home")) ? Out.HomeOwnerId.IsSet() && Out.HomeRoomId.IsSet()
                    : !Out.HomeOwnerId.IsSet() && !Out.HomeRoomId.IsSet();
            }
            if (!Literal(Field(O, TEXT("kind")), TEXT("road")) || !Only(O, {TEXT("kind"), TEXT("worldKey"), TEXT("point")})) return false;
            Out.Kind = EPlayerVehicleLocationKind::Road;
            const auto P = Object(Field(O, TEXT("point")));
            return Only(P, {TEXT("latitude"), TEXT("longitude")}) && Number(P, TEXT("latitude"), Out.Latitude)
                && Number(P, TEXT("longitude"), Out.Longitude) && FMath::Abs(Out.Latitude) <= 90 && FMath::Abs(Out.Longitude) <= 180;
        }
        bool Parked(const FValue& Value, FPlayerVehicleParked& Out)
        {
            const auto O = Object(Value);
            // Legacy parked strings may be empty. The enclosing bound limits allocation;
            // neither string establishes a current route or control identity.
            return BoundedObject(O, 160000) && Only(O, {TEXT("navigationRevision"), TEXT("pose"), TEXT("fuelMetres"), TEXT("instanceId")})
                && String(Field(O, TEXT("navigationRevision")), Out.NavigationRevision)
                && String(Field(O, TEXT("instanceId")), Out.InstanceId)
                && Number(O, TEXT("fuelMetres"), Out.FuelMetres) && Out.FuelMetres >= 0
                && MovementProtocolPrivate::ReadDrivingPose(Object(Field(O, TEXT("pose"))), Out.Pose);
        }
        bool Vehicle(const FValue& Value, FPlayerVehicle& Out)
        {
            const auto O = Object(Value);
            if (!Only(O, {TEXT("vehicleId"), TEXT("ownerUserId"), TEXT("model"), TEXT("revision"), TEXT("fuelUnits"), TEXT("condition"),
                TEXT("odometerMetres"), TEXT("location"), TEXT("activeTripId"), TEXT("forSale"), TEXT("forHire"), TEXT("asset"),
                TEXT("driving"), TEXT("drivingFuelMetres"), TEXT("drivingParked")})
                || !Key(Field(O, TEXT("vehicleId")), Out.VehicleId) || !Nullable(Field(O, TEXT("ownerUserId")), Out.OwnerUserId, Key)
                || !Model(Field(O, TEXT("model")), Out.Model) || !BoundedInteger(Field(O, TEXT("revision")), Out.Revision)
                || !BoundedInteger(Field(O, TEXT("fuelUnits")), Out.FuelUnits, Out.Model.TankCapacityUnits)
                || !BoundedInteger(Field(O, TEXT("condition")), Out.Condition, 100)
                || !BoundedInteger(Field(O, TEXT("odometerMetres")), Out.OdometerMetres)
                || !Location(Field(O, TEXT("location")), Out.Location)
                || !Nullable(Field(O, TEXT("activeTripId")), Out.ActiveTripId, [](const FValue& V, FString& S) { return Text(V, S, 240); })
                || !Boolean(Field(O, TEXT("forSale")), Out.bForSale) || !Boolean(Field(O, TEXT("forHire")), Out.bForHire)
                || !Field(O, TEXT("asset")).IsValid() || Field(O, TEXT("asset"))->Type != EJson::Null
                || (Out.OwnerUserId.IsSet() && (Out.bForSale || Out.bForHire))) return false;
            if (const auto Fuel = Field(O, TEXT("drivingFuelMetres")); Fuel.IsValid())
            {
                double N;
                if (!Number(Fuel, N) || N < 0 || N >= Out.Model.MetresPerFuelUnit) return false;
                Out.DrivingFuelMetres = N;
            }
            Out.bHasDrivingField = O->HasField(TEXT("driving"));
            if (Out.bHasDrivingField && !Nullable(Field(O, TEXT("driving")), Out.Driving,
                // Shared reader owns Identity versus Control semantics. Bound all
                // strings without narrowing navigationRevision to a control ID.
                [](const FValue& V, FWorldDrivingControl& D) { const auto Obj = Object(V); return BoundedObject(Obj, 160000)
                    && MovementProtocolPrivate::ReadDrivingControl(Obj, D); })) return false;
            Out.bHasDrivingParkedField = O->HasField(TEXT("drivingParked"));
            if (Out.bHasDrivingParkedField && !Nullable(Field(O, TEXT("drivingParked")), Out.DrivingParked, Parked)) return false;
            if (Out.Driving.IsSet())
            {
                const auto& D = Out.Driving.GetValue();
                if (Out.ActiveTripId.IsSet() || !Same(D.VehicleId, Out.VehicleId) || D.VehicleRevision != Out.Revision
                    || !Out.OwnerUserId.IsSet() || !Same(D.Driver.UserId, Out.OwnerUserId.GetValue())
                    || D.Occupants.Num() != 1 || Out.Model.Kind != EResidentVehicleKind::Keke) return false;
            }
            return true;
        }
        bool ContainsExact(const TArray<FString>& Values, const FString& Value)
        {
            for (const auto& Item : Values) if (Same(Item, Value)) return true;
            return false;
        }
        bool Owned(const TArray<FPlayerVehicle>& Values)
        {
            TArray<FString> Ids;
            TSet<EResidentVehicleKind> Kinds;
            for (const auto& V : Values)
            {
                if (!V.OwnerUserId.IsSet() || !Same(V.OwnerUserId.GetValue(), Values[0].OwnerUserId.GetValue())
                    || ContainsExact(Ids, V.VehicleId) || Kinds.Contains(V.Model.Kind)) return false;
                Ids.Add(V.VehicleId); Kinds.Add(V.Model.Kind);
            }
            return true;
        }
        bool Catalog(const FObject& O, FPlayerVehicleCatalog& Out)
        {
            if (!Only(O, {TEXT("owned"), TEXT("sale"), TEXT("hire")})
                || !ReadBoundedArray(Field(O, TEXT("owned")), Out.Owned, 4, Vehicle)
                || !ReadBoundedArray(Field(O, TEXT("sale")), Out.Sale, 16, Vehicle)
                || !ReadBoundedArray(Field(O, TEXT("hire")), Out.Hire, 16, Vehicle) || !Owned(Out.Owned)) return false;
            TArray<FString> Ids;
            for (const auto* List : {&Out.Owned, &Out.Sale, &Out.Hire})
                for (const auto& V : *List)
                {
                    if (ContainsExact(Ids, V.VehicleId)) return false;
                    Ids.Add(V.VehicleId);
                    if (List != &Out.Owned && (V.OwnerUserId.IsSet() || V.ActiveTripId.IsSet()
                        || V.bForSale != (List == &Out.Sale) || V.bForHire != (List == &Out.Hire))) return false;
                }
            return true;
        }
        bool Stranded(const FPlayerVehicle& V) { return V.Location.Kind == EPlayerVehicleLocationKind::Road && !V.ActiveTripId.IsSet(); }
        bool Recovery(const FObject& O, FPlayerVehicleRecovery& Out)
        {
            if (!Only(O, {TEXT("owned"), TEXT("targets")}) || !ReadBoundedArray(Field(O, TEXT("owned")), Out.Owned, 4, Vehicle)
                || !Owned(Out.Owned) || !ReadBoundedArray(Field(O, TEXT("targets")), Out.Targets, 4,
                    [](const FValue& V, FPlayerVehicleRecoveryTarget& T)
                    {
                        const auto R = Object(V);
                        return Only(R, {TEXT("vehicleId"), TEXT("vehicleRevision"), TEXT("placeId")})
                            && Key(Field(R, TEXT("vehicleId")), T.VehicleId) && Integer(Field(R, TEXT("vehicleRevision")), T.VehicleRevision)
                            && Key(Field(R, TEXT("placeId")), T.PlaceId) && T.PlaceId.StartsWith(TEXT("roadside:"), ESearchCase::CaseSensitive);
                    })) return false;
            int32 Count = 0;
            for (const auto& V : Out.Owned) if (Stranded(V)) ++Count;
            if (Count != Out.Targets.Num()) return false;
            TArray<FString> Vehicles, Places;
            for (const auto& T : Out.Targets)
            {
                if (ContainsExact(Vehicles, T.VehicleId) || ContainsExact(Places, T.PlaceId)) return false;
                Vehicles.Add(T.VehicleId); Places.Add(T.PlaceId);
                bool bMatch = false;
                for (const auto& V : Out.Owned) if (Stranded(V) && Same(V.VehicleId, T.VehicleId) && V.Revision == T.VehicleRevision) bMatch = true;
                if (!bMatch) return false;
            }
            return true;
        }
        bool ValidScope(const FPlayerVehicleScope& S)
        { return KeyText(S.GameRoomId) && KeyText(S.InstanceId) && KeyText(S.LifeId) && KeyText(S.ServiceId) && S.Revision >= 0 && S.Revision <= MaxInteger; }
        bool SameScope(const FPlayerVehicleScope& A, const FPlayerVehicleScope& B)
        { return Same(A.GameRoomId, B.GameRoomId) && Same(A.InstanceId, B.InstanceId) && Same(A.LifeId, B.LifeId) && Same(A.ServiceId, B.ServiceId) && A.Revision == B.Revision; }
        bool RequestId(const FString& S) { return S.Len() >= 16 && S.Len() <= 128 && KeyText(S); }
        bool Scope(const FValue& V, FPlayerVehicleScope& S)
        {
            const auto O = Object(V);
            return Only(O, {TEXT("gameRoomId"), TEXT("instanceId"), TEXT("lifeId"), TEXT("revision"), TEXT("serviceId")})
                && Key(Field(O, TEXT("gameRoomId")), S.GameRoomId) && Key(Field(O, TEXT("instanceId")), S.InstanceId)
                && Key(Field(O, TEXT("lifeId")), S.LifeId) && Key(Field(O, TEXT("serviceId")), S.ServiceId)
                && Integer(Field(O, TEXT("revision")), S.Revision);
        }
        bool SameOptionalText(const TOptional<FString>& A, const TOptional<FString>& B)
        { return A.IsSet() == B.IsSet() && (!A.IsSet() || Same(*A, *B)); }
        bool SameOptionalScope(const TOptional<FPlayerVehicleScope>& A, const TOptional<FPlayerVehicleScope>& B)
        { return A.IsSet() == B.IsSet() && (!A.IsSet() || SameScope(*A, *B)); }
        bool SameLocation(const FPlayerVehicleLocation& A, const FPlayerVehicleLocation& B)
        {
            if (A.Kind != B.Kind || !Same(A.WorldKey, B.WorldKey)) return false;
            return A.Kind == EPlayerVehicleLocationKind::Road ? A.Latitude == B.Latitude && A.Longitude == B.Longitude
                : Same(A.PlaceId, B.PlaceId) && Same(A.LayoutId, B.LayoutId) && SameOptionalText(A.HomeOwnerId, B.HomeOwnerId)
                    && SameOptionalText(A.HomeRoomId, B.HomeRoomId);
        }
        bool SameJson(const FValue& A, const FValue& B)
        {
            if (!A.IsValid() || !B.IsValid() || A->Type != B->Type) return false;
            if (A->Type == EJson::Null) return true;
            if (A->Type == EJson::String) return Same(A->AsString(), B->AsString());
            if (A->Type == EJson::Number) return A->AsNumber() == B->AsNumber();
            if (A->Type == EJson::Boolean) return A->AsBool() == B->AsBool();
            if (A->Type == EJson::Array)
            {
                if (A->AsArray().Num() != B->AsArray().Num()) return false;
                for (int32 I = 0; I < A->AsArray().Num(); ++I) if (!SameJson(A->AsArray()[I], B->AsArray()[I])) return false;
                return true;
            }
            if (A->Type != EJson::Object || A->AsObject()->Values.Num() != B->AsObject()->Values.Num()) return false;
            for (const auto& Left : A->AsObject()->Values)
            {
                bool bFound = false;
                const FString Name(Left.Key.Len(), *Left.Key);
                for (const auto& Right : B->AsObject()->Values)
                    if (Same(Name, FString(Right.Key.Len(), *Right.Key))) { bFound = SameJson(Left.Value, Right.Value); break; }
                if (!bFound) return false;
            }
            return true;
        }
        bool SameTerms(const FPlayerVehicleQuote& A, const FPlayerVehicleQuote& B)
        {
            return A.Document.IsValid() && B.Document.IsValid() && Same(A.Value.Id, B.Value.Id)
                && A.Value.IssuedAtMs == B.Value.IssuedAtMs && A.Value.ExpiresAtMs == B.Value.ExpiresAtMs
                && Same(A.Value.TermsFingerprint, B.Value.TermsFingerprint) && SameJson(A.Document->Service, B.Document->Service);
        }
        bool Eligible(const FPlayerVehicleQuote& Q)
        {
            return Q.Value.Service.Price.Unit == EGameEconomyPriceUnit::GameValueUnit
                && Same(Q.Value.Service.Price.UnitKey, TEXT("lagos-credit")) && Same(Q.Value.Service.Price.BankedSourceKey, TEXT("haul"));
        }
        FString Serialize(const FObject& O)
        {
            FString S;
            const auto Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&S);
            if (!FJsonSerializer::Serialize(O.ToSharedRef(), Writer)) return {};
            return S;
        }
        bool Quote(const FObject& O, FPlayerVehicleQuote& Q)
        {
            int32 Nodes = 0;
            if (!O.IsValid() || !BoundedValue(MakeShared<FJsonValueObject>(O), 160000, 0, Nodes, 20000)) return false;
            const FString Json = Serialize(O);
            FString Error;
            if (!DecodeGameEconomyQuote(Json, Q.Value, Error)) return false;
            auto Doc = MakeShared<FPlayerVehicleQuoteDocument>(); Doc->Service = Field(O, TEXT("service")); Q.Document = Doc;
            return true;
        }
        bool FuelPack(int64 Q) { return Q == 1 || Q == 5 || Q == 10 || Q == 20; }
        bool Confirmation(const FObject& O, FPlayerVehicleConfirmation& C)
        {
            if (!Only(O, {TEXT("quote"), TEXT("vehicleId"), TEXT("vehicleRevision"), TEXT("purpose"), TEXT("quantity"), TEXT("fuelBefore"),
                TEXT("fuelAfter"), TEXT("tankCapacityUnits"), TEXT("departureId"), TEXT("destination"), TEXT("routeMetres")})
                || !Quote(Object(Field(O, TEXT("quote"))), C.Quote) || !Key(Field(O, TEXT("vehicleId")), C.VehicleId)
                || !Integer(Field(O, TEXT("vehicleRevision")), C.VehicleRevision) || !Integer(Field(O, TEXT("quantity")), C.Quantity)
                || !Integer(Field(O, TEXT("tankCapacityUnits")), C.TankCapacityUnits, 1)
                || !Integer(Field(O, TEXT("fuelBefore")), C.FuelBefore) || C.FuelBefore > C.TankCapacityUnits
                || !Integer(Field(O, TEXT("fuelAfter")), C.FuelAfter) || C.FuelAfter > C.TankCapacityUnits
                || !EnumValue(Field(O, TEXT("purpose")), C.Purpose, {{TEXT("purchase"), EPlayerVehiclePurpose::Purchase},
                    {TEXT("fuel"), EPlayerVehiclePurpose::Fuel}, {TEXT("fare"), EPlayerVehiclePurpose::Fare}})
                || !Nullable(Field(O, TEXT("departureId")), C.DepartureId, Key)
                || !Nullable(Field(O, TEXT("destination")), C.Destination, Location)
                || !Nullable(Field(O, TEXT("routeMetres")), C.RouteMetres, [](const FValue& V, double& N) { return Number(V, N); })) return false;
            if (C.Purpose == EPlayerVehiclePurpose::Fare) return C.Quantity == 1 && C.DepartureId.IsSet() && RequestId(*C.DepartureId)
                && C.Destination.IsSet() && C.RouteMetres.IsSet() && *C.RouteMetres > 0 && *C.RouteMetres <= 1000000 && C.FuelAfter <= C.FuelBefore;
            return !C.DepartureId.IsSet() && !C.Destination.IsSet() && !C.RouteMetres.IsSet()
                && (C.Purpose == EPlayerVehiclePurpose::Purchase ? C.Quantity == 1 && C.FuelAfter == C.FuelBefore
                    : FuelPack(C.Quantity) && C.FuelAfter - C.FuelBefore == C.Quantity);
        }
        bool Target(const FValue& V, FPlayerVehicleRecoveryTarget& T)
        {
            const auto O = Object(V);
            return Only(O, {TEXT("vehicleId"), TEXT("vehicleRevision"), TEXT("placeId")}) && Key(Field(O, TEXT("vehicleId")), T.VehicleId)
                && Integer(Field(O, TEXT("vehicleRevision")), T.VehicleRevision) && Key(Field(O, TEXT("placeId")), T.PlaceId);
        }
        bool Departure(const FObject& O, FPlayerVehicleDeparture& D)
        {
            if (!Only(O, {TEXT("scope"), TEXT("departureId"), TEXT("vehicleId"), TEXT("vehicleRevision"), TEXT("mode"), TEXT("serviceId"),
                    TEXT("destination"), TEXT("routeMetres"), TEXT("expiresAt"), TEXT("worldTravel")})
                || !Scope(Field(O, TEXT("scope")), D.Scope) || !Key(Field(O, TEXT("departureId")), D.DepartureId) || !RequestId(D.DepartureId)
                || !Key(Field(O, TEXT("vehicleId")), D.VehicleId) || !Integer(Field(O, TEXT("vehicleRevision")), D.VehicleRevision)
                || !String(Field(O, TEXT("mode")), D.Mode) || !Same(D.Mode, TEXT("fare"))
                || !Key(Field(O, TEXT("serviceId")), D.ServiceId) || !Same(D.ServiceId, D.Scope.ServiceId)
                || !Location(Field(O, TEXT("destination")), D.Destination) || !Number(O, TEXT("routeMetres"), D.RouteMetres)
                || D.RouteMetres <= 0 || D.RouteMetres > 1000000 || !Integer(Field(O, TEXT("expiresAt")), D.ExpiresAtMs)) return false;
            if (O->HasField(TEXT("worldTravel")))
            {
                const auto W = Object(Field(O, TEXT("worldTravel"))); FPlayerVehicleWorldTravel Travel;
                if (!Only(W, {TEXT("placeId"), TEXT("roadsideTarget")}) || !Key(Field(W, TEXT("placeId")), Travel.PlaceId)) return false;
                if (W->HasField(TEXT("roadsideTarget")))
                {
                    FPlayerVehicleRecoveryTarget T;
                    if (!Target(Field(W, TEXT("roadsideTarget")), T)) return false;
                    Travel.RoadsideTarget = MoveTemp(T);
                }
                if (D.Destination.Kind == EPlayerVehicleLocationKind::Place)
                { if (!Same(Travel.PlaceId, D.Destination.PlaceId) || Travel.RoadsideTarget.IsSet()) return false; }
                else if (!Travel.RoadsideTarget.IsSet() || !Travel.PlaceId.StartsWith(TEXT("roadside:"), ESearchCase::CaseSensitive)
                    || !Same(Travel.RoadsideTarget->PlaceId, Travel.PlaceId)) return false;
                D.WorldTravel = MoveTemp(Travel);
            }
            return true;
        }
        bool FareMatches(const FPlayerVehicleConfirmation& C, const FPlayerVehicleDeparture& D)
        {
            return C.Purpose == EPlayerVehiclePurpose::Fare && C.DepartureId.IsSet() && Same(*C.DepartureId, D.DepartureId)
                && Same(C.VehicleId, D.VehicleId) && C.VehicleRevision == D.VehicleRevision && C.Destination.IsSet()
                && SameLocation(*C.Destination, D.Destination) && C.RouteMetres.IsSet() && *C.RouteMetres == D.RouteMetres;
        }
        bool SameDeparture(const TOptional<FPlayerVehicleDeparture>& A, const TOptional<FPlayerVehicleDeparture>& B)
        {
            if (A.IsSet() != B.IsSet()) return false;
            if (!A.IsSet()) return true;
            if (!SameScope(A->Scope, B->Scope) || !Same(A->DepartureId, B->DepartureId) || !Same(A->VehicleId, B->VehicleId)
                || A->VehicleRevision != B->VehicleRevision || !Same(A->Mode, B->Mode) || !Same(A->ServiceId, B->ServiceId)
                || A->ExpiresAtMs != B->ExpiresAtMs || A->RouteMetres != B->RouteMetres || !SameLocation(A->Destination, B->Destination)
                || A->WorldTravel.IsSet() != B->WorldTravel.IsSet()) return false;
            if (!A->WorldTravel.IsSet()) return true;
            const auto& L = *A->WorldTravel; const auto& R = *B->WorldTravel;
            if (!Same(L.PlaceId, R.PlaceId) || L.RoadsideTarget.IsSet() != R.RoadsideTarget.IsSet()) return false;
            return !L.RoadsideTarget.IsSet() || (Same(L.RoadsideTarget->VehicleId, R.RoadsideTarget->VehicleId)
                && L.RoadsideTarget->VehicleRevision == R.RoadsideTarget->VehicleRevision && Same(L.RoadsideTarget->PlaceId, R.RoadsideTarget->PlaceId));
        }
        template<typename T, typename F> bool Decode(const FString& Json, T& Out, FString& Error, F Read)
        {
            const bool bBounded = Units(Json) <= Limits.Characters && WithinParseLimits(Json, Limits);
            const FString Input = bBounded ? Json : FString();
            Out = T{}; Error = ReadError;
            FObject Root; T Candidate;
            if (!bBounded || !ParseRoot(Input, Root) || !Read(Root, Candidate)) return false;
            Out = MoveTemp(Candidate); Error.Reset(); return true;
        }
        bool CommandParseLimits(const FString& Json)
        {
            if (Units(Json) > Limits.Characters || !WithinParseLimits(Json, Limits)) return false;
            int32 Nodes = 0;
            for (int32 I = 0; I < Json.Len(); ++I)
            {
                const TCHAR C = Json[I];
                if (FChar::IsWhitespace(C) || C == TEXT(',') || C == TEXT(':') || C == TEXT('}') || C == TEXT(']')) continue;
                if (C == TEXT('"'))
                {
                    int64 Length = 0; bool bClosed = false;
                    while (++I < Json.Len())
                    {
                        const TCHAR S = Json[I];
                        if (S == TEXT('"')) { bClosed = true; break; }
                        if (S == TEXT('\\'))
                        {
                            if (++I >= Json.Len()) return false;
                            if (Json[I] == TEXT('u'))
                            {
                                if (I + 4 >= Json.Len()) return false;
                                I += 4;
                            }
                            ++Length;
                        }
                        else Length += sizeof(TCHAR) > 2 && static_cast<uint32>(S) > 0xffff ? 2 : 1;
                        if (Length > 160000) return false;
                    }
                    if (!bClosed) return false;
                    int32 Next = I + 1;
                    while (Next < Json.Len() && FChar::IsWhitespace(Json[Next])) ++Next;
                    if (Next < Json.Len() && Json[Next] == TEXT(':')) { if (Length > 240) return false; }
                    else if (++Nodes > 20000) return false;
                }
                else
                {
                    if (++Nodes > 20000) return false;
                    if (C != TEXT('{') && C != TEXT('['))
                        while (I + 1 < Json.Len() && !FChar::IsWhitespace(Json[I + 1]) && Json[I + 1] != TEXT(',')
                            && Json[I + 1] != TEXT('}') && Json[I + 1] != TEXT(']')) ++I;
                }
            }
            // Unreal remains responsible for JSON syntax and escape validation.
            return true;
        }
        template<typename T, typename F> bool DecodeCommandValue(const FString& Json, T& Out, FString& Error, F Read)
        {
            const bool bBounded = CommandParseLimits(Json);
            const FString Input = bBounded ? Json : FString();
            Out = T{}; Error = TEXT("Transport confirmation couldn't load. Reload to retry.");
            FObject Root; T Candidate; int32 Nodes = 0;
            if (!bBounded || !Parse(Input, Root, Limits)
                || !BoundedValue(MakeShared<FJsonValueObject>(Root), 160000, 0, Nodes, 20000) || !Read(Root, Candidate)) return false;
            Out = MoveTemp(Candidate); Error.Reset(); return true;
        }
        bool ValidLocation(const FPlayerVehicleLocation& L)
        {
            if (!Same(L.WorldKey, TEXT("lagos-v1"))) return false;
            if (L.Kind == EPlayerVehicleLocationKind::Road) return FMath::IsFinite(L.Latitude) && FMath::IsFinite(L.Longitude)
                && FMath::Abs(L.Latitude) <= 90 && FMath::Abs(L.Longitude) <= 180;
            return L.Kind == EPlayerVehicleLocationKind::Place && KeyText(L.PlaceId) && !L.LayoutId.IsEmpty() && Units(L.LayoutId) <= 160000
                && (Same(L.PlaceId, TEXT("own-home")) ? L.HomeOwnerId.IsSet() && L.HomeRoomId.IsSet()
                    && KeyText(*L.HomeOwnerId) && KeyText(*L.HomeRoomId) : !L.HomeOwnerId.IsSet() && !L.HomeRoomId.IsSet());
        }
        bool ValidDeparture(const FPlayerVehicleDeparture& D)
        {
            if (!ValidScope(D.Scope) || !RequestId(D.DepartureId) || !KeyText(D.VehicleId) || D.VehicleRevision < 0 || D.VehicleRevision > MaxInteger
                || !Same(D.Mode, TEXT("fare")) || !Same(D.ServiceId, D.Scope.ServiceId) || !ValidLocation(D.Destination)
                || !FMath::IsFinite(D.RouteMetres) || D.RouteMetres <= 0 || D.RouteMetres > 1000000 || D.ExpiresAtMs < 0 || D.ExpiresAtMs > MaxInteger) return false;
            if (!D.WorldTravel.IsSet()) return true;
            const auto& W = *D.WorldTravel;
            if (!KeyText(W.PlaceId)) return false;
            if (D.Destination.Kind == EPlayerVehicleLocationKind::Place) return Same(W.PlaceId, D.Destination.PlaceId) && !W.RoadsideTarget.IsSet();
            return W.RoadsideTarget.IsSet() && W.PlaceId.StartsWith(TEXT("roadside:"), ESearchCase::CaseSensitive)
                && Same(W.RoadsideTarget->PlaceId, W.PlaceId) && KeyText(W.RoadsideTarget->VehicleId)
                && W.RoadsideTarget->VehicleRevision >= 0 && W.RoadsideTarget->VehicleRevision <= MaxInteger;
        }
        void ScopeArguments(const FPlayerVehicleScope& S, const FObject& O)
        {
            O->SetStringField(TEXT("gameRoomId"), S.GameRoomId); O->SetStringField(TEXT("instanceId"), S.InstanceId);
            O->SetStringField(TEXT("lifeId"), S.LifeId); O->SetStringField(TEXT("serviceId"), S.ServiceId);
            O->SetNumberField(TEXT("revision"), S.Revision);
        }
        bool CommandArguments(const FPlayerVehicleCommandPacket& P, FString& Endpoint, FString& Arguments)
        {
            if (P.Fare.IsSet()) return false;
            const auto O = MakeShared<FJsonObject>();
            O->SetStringField(TEXT("requestId"), P.RequestId);
            if (P.Action.Kind != EPlayerVehicleActionKind::Cancel)
            { if (!P.Scope.IsSet()) return false; ScopeArguments(*P.Scope, O); }
            const TCHAR* Name = nullptr;
            switch (P.Action.Kind)
            {
            case EPlayerVehicleActionKind::PurchaseQuote:
            case EPlayerVehicleActionKind::FuelQuote:
            {
                Name = TEXT("requestQuote"); const auto S = MakeShared<FJsonObject>();
                S->SetStringField(TEXT("purpose"), P.Action.Kind == EPlayerVehicleActionKind::PurchaseQuote ? TEXT("purchase") : TEXT("fuel"));
                S->SetStringField(TEXT("vehicleId"), P.Action.Value);
                if (P.Action.Kind == EPlayerVehicleActionKind::FuelQuote) S->SetNumberField(TEXT("quantity"), P.Action.Quantity);
                O->SetObjectField(TEXT("selection"), S); break;
            }
            case EPlayerVehicleActionKind::FareQuote: Name = TEXT("requestFareQuote"); O->SetStringField(TEXT("departureId"), P.Action.Value); break;
            case EPlayerVehicleActionKind::Confirm: Name = TEXT("confirmQuote"); O->SetStringField(TEXT("quoteId"), P.Action.Value); break;
            case EPlayerVehicleActionKind::Cancel: Name = TEXT("cancelQuote"); O->SetStringField(TEXT("quoteId"), P.Action.Value); break;
            default: return false;
            }
            Endpoint = FString(TEXT("companionPlayerVehicles:")) + Name; Arguments = Serialize(O); return !Arguments.IsEmpty();
        }
    }

    bool DecodePlayerVehicleCatalog(const FString& Json, FPlayerVehicleCatalog& Out, FString& OutError)
    { return Decode(Json, Out, OutError, Catalog); }
    bool DecodePlayerVehicleRecovery(const FString& Json, FPlayerVehicleRecovery& Out, FString& OutError)
    { return Decode(Json, Out, OutError, Recovery); }
    bool DecodePlayerVehicleConfirmation(const FString& Json, FPlayerVehicleConfirmation& Out, FString& OutError)
    { return DecodeCommandValue(Json, Out, OutError, Confirmation); }
    bool DecodePlayerVehicleDeparture(const FString& Json, FPlayerVehicleDeparture& Out, FString& OutError)
    { return DecodeCommandValue(Json, Out, OutError, Departure); }
    bool DecodePlayerVehicleQuote(const FString& Json, FPlayerVehicleQuote& Out, FString& OutError)
    { return DecodeCommandValue(Json, Out, OutError, Quote); }
    EPlayerVehicleReadMethod FPlayerVehicleReadIntent::GetMethod() const
    { return GetKind() == EPlayerVehicleReadKind::Catalog ? EPlayerVehicleReadMethod::Mutation : EPlayerVehicleReadMethod::Query; }
    const TCHAR* FPlayerVehicleReadIntent::GetEndpoint() const
    { return GetKind() == EPlayerVehicleReadKind::Catalog ? TEXT("companionPlayerVehicles:catalog")
        : GetKind() == EPlayerVehicleReadKind::Recovery ? TEXT("companionPlayerVehicles:recovery") : TEXT("companionPlayerVehicles:confirmation"); }

    FPlayerVehiclesState::FPlayerVehiclesState() : Snapshot(MakeShared<FPlayerVehiclesSnapshot>()) {}
    void FPlayerVehiclesState::Retire()
    { Operation = FGuid::NewGuid(); Pending.Reset(); StagedCatalog.Reset(); }
    void FPlayerVehiclesState::Publish(FPlayerVehiclesSnapshot Next)
    { Snapshot = MakeShared<FPlayerVehiclesSnapshot>(MoveTemp(Next)); OnChanged.Broadcast(); }
    void FPlayerVehiclesState::BeginOwner(const FString& AccountId)
    {
        const bool bValid = KeyText(AccountId);
        Owner = FGuid::NewGuid(); Selection = FGuid::NewGuid(); Retire(); bActive = bValid;
        RetireConfirmation(); PendingCommand.Reset(); QuoteExpected.Reset(); bInFlight = false; Request = FGuid::NewGuid(); Attempt = FGuid::NewGuid();
        Publish({});
    }
    void FPlayerVehiclesState::Select(const TOptional<FPlayerVehicleScope>& Scope, bool bRecoveryOnly)
    {
        if (!bActive) return;
        const auto Selected = Scope;
        if (SameOptionalScope(Selected, Snapshot->Scope) && Snapshot->bRecoveryOnly == bRecoveryOnly) return;
        Selection = FGuid::NewGuid(); Retire(); RetireConfirmation();
        FPlayerVehiclesSnapshot Next; Next.Scope = Selected; Next.bRecoveryOnly = bRecoveryOnly;
        Next.QuoteId = Snapshot->QuoteId; Next.Confirmation = Snapshot->Confirmation;
        Next.bBusy = bInFlight; Next.bRetryable = PendingCommand.IsValid() && !bInFlight;
        if (Selected.IsSet() && !ValidScope(Selected.GetValue())) { Next.bReady = true; Next.Error = TEXT("Reopen transport for this visit."); }
        Publish(MoveTemp(Next));
    }
    bool FPlayerVehiclesState::Current(const FPlayerVehicleReadReceipt& R) const
    {
        if (R.Kind == EPlayerVehicleReadKind::Confirmation) return bActive && PendingConfirmation.IsSet()
            && R.Owner == Owner && R.Selection == Selection && R.Operation == QuoteRead && R.Stage == PendingConfirmation->Stage
            && Snapshot->QuoteId.IsSet() && Same(R.QuoteId, *Snapshot->QuoteId);
        return bActive && Pending.IsSet() && R.Owner == Owner && R.Selection == Selection && R.Operation == Operation
            && R.Stage == Pending->Stage && R.Kind == Pending->Kind;
    }
    FPlayerVehicleReadIntent FPlayerVehiclesState::Capture(EPlayerVehicleReadKind Kind)
    {
        FPlayerVehicleReadIntent Intent;
        Intent.Receipt.Owner = Owner; Intent.Receipt.Selection = Selection; Intent.Receipt.Operation = Operation;
        Intent.Receipt.Stage = FGuid::NewGuid(); Intent.Receipt.Kind = Kind;
        if (Kind == EPlayerVehicleReadKind::Catalog) Intent.Arguments = Snapshot->Scope;
        Pending = Intent.Receipt;
        return Intent;
    }
    TOptional<FPlayerVehicleReadIntent> FPlayerVehiclesState::StartRead()
    {
        if (!bActive || !Snapshot->Scope.IsSet() || !ValidScope(Snapshot->Scope.GetValue()) || Snapshot->Departure.IsSet()) return {};
        Retire();
        const auto Intent = Capture(Snapshot->bRecoveryOnly ? EPlayerVehicleReadKind::Recovery : EPlayerVehicleReadKind::Catalog);
        auto Next = *Snapshot; Next.bLoading = true; Publish(MoveTemp(Next));
        return Current(Intent.Receipt) ? TOptional<FPlayerVehicleReadIntent>(Intent) : TOptional<FPlayerVehicleReadIntent>();
    }
    TOptional<FPlayerVehicleReadIntent> FPlayerVehiclesState::AcceptCatalog(const FPlayerVehicleReadReceipt& Receipt, const FString& Json)
    {
        const auto R = Receipt;
        if (!Current(R) || R.Kind != EPlayerVehicleReadKind::Catalog) return {};
        FPlayerVehicleCatalog Candidate; FString Error;
        if (!DecodePlayerVehicleCatalog(Json, Candidate, Error)) { FailRead(R); return {}; }
        bool bRecovery = false;
        for (const auto& V : Candidate.Owned) if (Stranded(V)) bRecovery = true;
        if (bRecovery)
        {
            StagedCatalog = MoveTemp(Candidate);
            return Capture(EPlayerVehicleReadKind::Recovery);
        }
        auto Next = *Snapshot; Next.Catalog = MoveTemp(Candidate); Next.Recovery.Reset();
        Next.bReady = true; Next.bLoading = false; if (!PendingCommand.IsValid()) Next.Error.Reset(); Retire(); Publish(MoveTemp(Next));
        return {};
    }
    void FPlayerVehiclesState::AcceptRecovery(const FPlayerVehicleReadReceipt& Receipt, const FString& Json)
    {
        const auto R = Receipt;
        if (!Current(R) || R.Kind != EPlayerVehicleReadKind::Recovery) return;
        FPlayerVehicleRecovery Candidate; FString Error;
        if (!DecodePlayerVehicleRecovery(Json, Candidate, Error)) { FailRead(R); return; }
        auto Next = *Snapshot; Next.Catalog = MoveTemp(StagedCatalog); Next.Recovery = MoveTemp(Candidate);
        Next.bReady = true; Next.bLoading = false; if (!PendingCommand.IsValid()) Next.Error.Reset(); Retire(); Publish(MoveTemp(Next));
    }
    void FPlayerVehiclesState::FailRead(const FPlayerVehicleReadReceipt& Receipt)
    {
        const auto R = Receipt;
        if (!Current(R)) return;
        if (R.Kind == EPlayerVehicleReadKind::Confirmation)
        {
            RetireConfirmation(); auto Next = *Snapshot; Next.Error = TEXT("Transport confirmation couldn't load. Reload to retry.");
            Publish(MoveTemp(Next)); return;
        }
        auto Next = *Snapshot; Next.Catalog.Reset(); Next.Recovery.Reset(); Next.bReady = true; Next.bLoading = false; Next.Error = ReadError;
        Retire(); Publish(MoveTemp(Next));
    }
    void FPlayerVehiclesState::StopReads()
    {
        if (!bActive) return;
        Selection = FGuid::NewGuid(); Retire(); RetireConfirmation();
        FPlayerVehiclesSnapshot Next; Next.QuoteId = Snapshot->QuoteId; Next.Confirmation = Snapshot->Confirmation;
        Next.bBusy = bInFlight; Next.bRetryable = PendingCommand.IsValid() && !bInFlight; Publish(MoveTemp(Next));
    }
    void FPlayerVehiclesState::RetireConfirmation()
    {
        QuoteRead = FGuid::NewGuid(); PendingConfirmation.Reset(); ReadConfirmationScope.Reset(); ReadExpected.Reset();
    }
    TOptional<FPlayerVehicleReadIntent> FPlayerVehiclesState::CaptureConfirmation(const TOptional<FPlayerVehicleScope>& Scope)
    {
        if (!bActive || !Snapshot->QuoteId.IsSet()) return {};
        RetireConfirmation();
        FPlayerVehicleReadIntent Intent;
        Intent.Receipt.Owner = Owner; Intent.Receipt.Selection = Selection; Intent.Receipt.Operation = QuoteRead;
        Intent.Receipt.Stage = FGuid::NewGuid(); Intent.Receipt.Kind = EPlayerVehicleReadKind::Confirmation;
        Intent.Receipt.QuoteId = *Snapshot->QuoteId;
        ReadConfirmationScope = Scope; ReadExpected = QuoteExpected; PendingConfirmation = Intent.Receipt;
        return Intent;
    }
    void FPlayerVehiclesState::SelectDeparture(const TOptional<FPlayerVehicleDeparture>& DepartureValue)
    {
        if (!bActive) return;
        const auto D = DepartureValue;
        if (SameDeparture(D, Snapshot->Departure)) return;
        Selection = FGuid::NewGuid(); Retire(); RetireConfirmation();
        auto Next = *Snapshot; Next.Catalog.Reset(); Next.Recovery.Reset(); Next.bLoading = false;
        if (D.IsSet() && (!ValidDeparture(*D) || !Snapshot->Scope.IsSet() || !SameScope(D->Scope, *Snapshot->Scope)))
        {
            Next.Departure.Reset(); Next.ConfirmationScope.Reset(); Next.Error = TEXT("Reopen this departure.");
        }
        else
        {
            Next.Departure = D; Next.bReady = D.IsSet();
            if ((Next.Confirmation.IsSet() && Next.Confirmation->Purpose == EPlayerVehiclePurpose::Fare)
                || (QuoteExpected.IsValid() && QuoteExpected->Action.Kind == EPlayerVehicleActionKind::FareQuote)) Next.ConfirmationScope.Reset();
        }
        Publish(MoveTemp(Next));
    }
    TOptional<FPlayerVehicleReadIntent> FPlayerVehiclesState::Reload()
    {
        if (!bActive) return {};
        const FGuid O = Owner, V = Selection, CatalogStamp = Operation, QuoteStamp = QuoteRead, CommandStamp = Request;
        if (!PendingCommand.IsValid()) { auto Next = *Snapshot; Next.Error.Reset(); Publish(MoveTemp(Next)); }
        if (!bActive || O != Owner || V != Selection || CatalogStamp != Operation || QuoteStamp != QuoteRead || CommandStamp != Request) return {};
        return Snapshot->QuoteId.IsSet() ? CaptureConfirmation(Snapshot->ConfirmationScope) : StartRead();
    }
    TOptional<FPlayerVehicleReadIntent> FPlayerVehiclesState::RecoverQuote(const FString& QuoteIdValue)
    {
        const FString Id = QuoteIdValue;
        if (!bActive || PendingCommand.IsValid() || bInFlight || !KeyText(Id)
            || (Snapshot->QuoteId.IsSet() && !Same(*Snapshot->QuoteId, Id))) return {};
        RetireConfirmation(); QuoteExpected.Reset();
        auto Next = *Snapshot; Next.QuoteId = Id; Next.Confirmation.Reset(); Next.ConfirmationScope.Reset();
        // Capture before notifying; a reentrant reload/owner/view change invalidates this intent.
        Snapshot = MakeShared<FPlayerVehiclesSnapshot>(MoveTemp(Next));
        const auto Intent = CaptureConfirmation({}); OnChanged.Broadcast();
        return Intent.IsSet() && Current(Intent->Receipt) ? Intent : TOptional<FPlayerVehicleReadIntent>();
    }
    void FPlayerVehiclesState::AcceptConfirmation(const FPlayerVehicleReadReceipt& Receipt, const FString& Json)
    {
        const auto R = Receipt;
        if (R.Kind != EPlayerVehicleReadKind::Confirmation || !Current(R)) return;
        FPlayerVehicleConfirmation C; FString Error;
        if (!DecodePlayerVehicleConfirmation(Json, C, Error) || !Same(C.Quote.Value.Id, R.QuoteId)) { FailRead(R); return; }
        if (ReadExpected.IsValid())
        {
            const auto Expected = ReadExpected;
            const auto& E = *Expected;
            const bool bSelection = E.Action.Kind == EPlayerVehicleActionKind::PurchaseQuote || E.Action.Kind == EPlayerVehicleActionKind::FuelQuote;
            if ((E.IssuedQuote.IsSet() && !SameTerms(C.Quote, *E.IssuedQuote))
                || (bSelection && (!Same(C.VehicleId, E.Action.Value)
                    || C.Purpose != (E.Action.Kind == EPlayerVehicleActionKind::PurchaseQuote ? EPlayerVehiclePurpose::Purchase : EPlayerVehiclePurpose::Fuel)
                    || (E.Action.Kind == EPlayerVehicleActionKind::FuelQuote && C.Quantity != E.Action.Quantity)))
                || (E.Departure.IsSet() && !FareMatches(C, *E.Departure)))
            {
                RetireConfirmation(); auto Next = *Snapshot;
                Next.Error = E.Departure.IsSet() ? TEXT("Request new departure terms.") : TEXT("Request new transport terms.");
                Publish(MoveTemp(Next)); return;
            }
        }
        if (Snapshot->Confirmation.IsSet() && Same(Snapshot->Confirmation->Quote.Value.Id, R.QuoteId)
            && (C.Quote.Value.Revision < Snapshot->Confirmation->Quote.Value.Revision || !SameTerms(C.Quote, Snapshot->Confirmation->Quote)))
        { RetireConfirmation(); return; }
        auto Next = *Snapshot;
        if (C.Quote.Value.Status == EGameEconomyQuoteStatus::Fulfilled || C.Quote.Value.Status == EGameEconomyQuoteStatus::Cancelled)
        {
            QuoteExpected.Reset(); Next.QuoteId.Reset(); Next.Confirmation.Reset(); Next.ConfirmationScope.Reset();
            Next.Notice = C.Quote.Value.Status == EGameEconomyQuoteStatus::Fulfilled
                ? TEXT("Transport action already completed. Reload to see current vehicles.") : TEXT("Quote cancelled.");
        }
        else { Next.Confirmation = MoveTemp(C); Next.ConfirmationScope = ReadConfirmationScope; }
        if (!PendingCommand.IsValid()) Next.Error.Reset();
        RetireConfirmation(); Publish(MoveTemp(Next));
    }
    bool FPlayerVehiclesState::CurrentCommand(const FPlayerVehicleCommandReceipt& R) const
    { return bActive && bInFlight && PendingCommand.IsValid() && R.Owner == Owner && R.Request == Request && R.Attempt == Attempt; }
    FPlayerVehicleCommandStart FPlayerVehiclesState::Send(const FPlayerVehicleAction& ActionValue, const FString& RequestIdValue)
    {
        FPlayerVehicleCommandStart Out;
        const auto A = ActionValue; const FString Id = RequestIdValue;
        if (!bActive || bInFlight || PendingCommand.IsValid() || !RequestId(Id)
            || (A.Kind != EPlayerVehicleActionKind::Cancel && (!Snapshot->Scope.IsSet() || !ValidScope(*Snapshot->Scope)))) return Out;
        const auto C = Snapshot->Confirmation; const auto D = Snapshot->Departure;
        const bool bQuote = A.Kind == EPlayerVehicleActionKind::PurchaseQuote || A.Kind == EPlayerVehicleActionKind::FuelQuote;
        if (Snapshot->bRecoveryOnly && (bQuote || (A.Kind == EPlayerVehicleActionKind::Confirm && (!C.IsSet() || C->Purpose != EPlayerVehiclePurpose::Fare)))) return Out;
        if (Snapshot->QuoteId.IsSet() && A.Kind != EPlayerVehicleActionKind::Confirm && A.Kind != EPlayerVehicleActionKind::Cancel) return Out;
        if (A.Kind == EPlayerVehicleActionKind::Confirm || A.Kind == EPlayerVehicleActionKind::Cancel)
        {
            if (!KeyText(A.Value) || !Snapshot->QuoteId.IsSet() || !Same(A.Value, *Snapshot->QuoteId)) return Out;
            if (A.Kind == EPlayerVehicleActionKind::Confirm && (!C.IsSet() || !Same(C->Quote.Value.Id, A.Value)
                || !SameOptionalScope(Snapshot->ConfirmationScope, Snapshot->Scope)
                || C->Quote.Value.Status != EGameEconomyQuoteStatus::Quoted || !Eligible(C->Quote))) return Out;
        }
        else if (bQuote)
        { if (!KeyText(A.Value) || (A.Kind == EPlayerVehicleActionKind::FuelQuote && !FuelPack(A.Quantity))) return Out; }
        else if (A.Kind == EPlayerVehicleActionKind::FareQuote)
        { if (!D.IsSet() || !Same(D->DepartureId, A.Value) || !SameScope(D->Scope, *Snapshot->Scope)) return Out; }
        else return Out;
        if (A.Kind == EPlayerVehicleActionKind::Confirm && C->Purpose == EPlayerVehiclePurpose::Fare)
        {
            if (!D.IsSet() || !FareMatches(*C, *D) || !SameScope(D->Scope, *Snapshot->Scope)) return Out;
            Out.Result = EPlayerVehicleCommandPreparation::FareHostUnavailable;
            FPlayerVehicleFareCommit Fare; Fare.Scope = *Snapshot->Scope; Fare.Departure = *D; Fare.Confirmation = *C; Fare.RequestId = Id;
            Out.RequiredFareHost = MoveTemp(Fare);
            const FGuid O = Owner, V = Selection;
            auto Next = *Snapshot; Next.Error = TEXT("Fare travel is unavailable."); Publish(MoveTemp(Next));
            if (!bActive || O != Owner || V != Selection) return {};
            return Out;
        }
        auto Packet = MakeShared<FPlayerVehicleCommandPacket>(); Packet->Scope = Snapshot->Scope;
        Packet->Action = A; Packet->RequestId = Id; Packet->Confirmation = C;
        if (QuoteExpected.IsValid()) Packet->IssuedQuote = QuoteExpected->IssuedQuote;
        if (A.Kind == EPlayerVehicleActionKind::FareQuote) Packet->Departure = D;
        PendingCommand = Packet; Request = FGuid::NewGuid();
        return Submit(false);
    }
    FPlayerVehicleCommandStart FPlayerVehiclesState::Submit(bool bReplay)
    {
        FPlayerVehicleCommandStart Out;
        if (!bActive || bInFlight || !PendingCommand.IsValid()) return Out;
        FPlayerVehicleCommandIntent Intent;
        if (!CommandArguments(*PendingCommand, Intent.Endpoint, Intent.ArgumentsJson)) return Out;
        Attempt = FGuid::NewGuid(); Intent.Receipt.Owner = Owner; Intent.Receipt.Request = Request;
        Intent.Receipt.Attempt = Attempt; Intent.Receipt.View = Selection; Intent.Packet = PendingCommand;
        bInFlight = true;
        auto Next = *Snapshot; Next.bBusy = true; Next.bRetryable = false; Next.Error.Reset(); Next.Notice.Reset();
        Publish(MoveTemp(Next));
        if (!CurrentCommand(Intent.Receipt)) return Out;
        if (!bReplay && (Intent.Receipt.View != Selection || !SameOptionalScope(Intent.Packet->Scope, Snapshot->Scope)))
        {
            // Nothing was dispatched. Release only this captured attempt after reentry.
            PendingCommand.Reset(); bInFlight = false; Attempt = FGuid::NewGuid();
            auto Cleared = *Snapshot; Cleared.bBusy = false; Cleared.bRetryable = false; Publish(MoveTemp(Cleared));
            return Out;
        }
        Out.Result = EPlayerVehicleCommandPreparation::Ready; Out.Intent = MoveTemp(Intent); return Out;
    }
    FPlayerVehicleCommandStart FPlayerVehiclesState::Retry() { return Submit(true); }
    void FPlayerVehiclesState::FailCommand(const FPlayerVehicleCommandReceipt& Receipt, EPlayerVehicleCommandFailure Failure)
    {
        const auto R = Receipt; if (!CurrentCommand(R)) return;
        bInFlight = false; Attempt = FGuid::NewGuid();
        if (Failure == EPlayerVehicleCommandFailure::AuthoritativeRejection) PendingCommand.Reset();
        auto Next = *Snapshot; Next.bBusy = false; Next.bRetryable = PendingCommand.IsValid();
        Next.Error = TEXT("Transport action couldn't finish. Reload to check availability."); Publish(MoveTemp(Next));
    }
    FPlayerVehicleCommandResult FPlayerVehiclesState::AcceptCommand(const FPlayerVehicleCommandReceipt& Receipt, const FString& Json)
    {
        FPlayerVehicleCommandResult Out; const auto R = Receipt;
        if (!CurrentCommand(R)) return Out;
        FPlayerVehicleQuote Q; FString Error;
        const auto Packet = PendingCommand;
        const auto Kind = Packet->Action.Kind;
        if (!DecodePlayerVehicleQuote(Json, Q, Error) || !Eligible(Q)
            || ((Kind == EPlayerVehicleActionKind::Confirm || Kind == EPlayerVehicleActionKind::Cancel)
                && (!Same(Q.Value.Id, Packet->Action.Value) || (Packet->Confirmation.IsSet() && !SameTerms(Q, Packet->Confirmation->Quote))
                    || (Packet->IssuedQuote.IsSet() && !SameTerms(Q, *Packet->IssuedQuote))))
            || (Kind == EPlayerVehicleActionKind::Confirm && Q.Value.Status != EGameEconomyQuoteStatus::Fulfilled)
            || (Kind == EPlayerVehicleActionKind::Cancel && Q.Value.Status != EGameEconomyQuoteStatus::Cancelled))
        { FailCommand(R, EPlayerVehicleCommandFailure::Uncertain); return Out; }
        auto Accepted = MakeShared<FPlayerVehicleCommandPacket>(*Packet); Accepted->IssuedQuote = Q;
        const bool bRelevant = R.View == Selection && SameOptionalScope(Packet->Scope, Snapshot->Scope);
        PendingCommand.Reset(); bInFlight = false; Attempt = FGuid::NewGuid();
        auto Next = *Snapshot; Next.bBusy = false; Next.bRetryable = false;
        const bool bIssue = Kind == EPlayerVehicleActionKind::PurchaseQuote || Kind == EPlayerVehicleActionKind::FuelQuote || Kind == EPlayerVehicleActionKind::FareQuote;
        if (!bRelevant) Next.Notice = TEXT("Previous transport action resolved. Reload transport for this visit.");
        if (bIssue)
        {
            RetireConfirmation(); QuoteExpected = Accepted; Next.QuoteId = Q.Value.Id; Next.Confirmation.Reset();
            Next.ConfirmationScope = bRelevant ? Packet->Scope : TOptional<FPlayerVehicleScope>();
        }
        else if (bRelevant || (Next.QuoteId.IsSet() && Same(*Next.QuoteId, Q.Value.Id)))
        {
            RetireConfirmation(); QuoteExpected.Reset(); Next.QuoteId.Reset(); Next.Confirmation.Reset(); Next.ConfirmationScope.Reset();
            if (bRelevant) Next.Notice = Kind == EPlayerVehicleActionKind::Cancel ? TEXT("Quote cancelled.")
                : Packet->Fare.IsSet() ? TEXT("Fare confirmed.")
                : Packet->Confirmation.IsSet() && Packet->Confirmation->Purpose == EPlayerVehiclePurpose::Fuel ? TEXT("Vehicle refueled.") : TEXT("Vehicle purchased.");
        }
        // Install and capture follow-up before notification. No post-notification work
        // may restore an old quote, clear a newer request or schedule an old counter.
        Snapshot = MakeShared<FPlayerVehiclesSnapshot>(MoveTemp(Next));
        if (bIssue) Out.FollowUp = CaptureConfirmation(Snapshot->ConfirmationScope);
        const FGuid O = Owner, V = Selection, CompletedRequest = Request, CatalogStamp = Operation, QuoteStamp = QuoteRead;
        OnChanged.Broadcast();
        if (!bActive || O != Owner) return {};
        Out.Quote = Q;
        if (Out.FollowUp.IsSet() && !Current(Out.FollowUp->Receipt)) Out.FollowUp.Reset();
        if (!bIssue && bRelevant && !Packet->Fare.IsSet() && O == Owner && V == Selection && CompletedRequest == Request
            && CatalogStamp == Operation && QuoteStamp == QuoteRead && !PendingCommand.IsValid()
            && (!Packet->Confirmation.IsSet() || Packet->Confirmation->Purpose != EPlayerVehiclePurpose::Fare)) Out.FollowUp = StartRead();
        return Out;
    }
    void FPlayerVehiclesState::Dispose()
    {
        bActive = false; Owner = FGuid::NewGuid(); Selection = FGuid::NewGuid(); Retire(); RetireConfirmation();
        PendingCommand.Reset(); QuoteExpected.Reset(); bInFlight = false; Request = FGuid::NewGuid(); Attempt = FGuid::NewGuid(); Publish({});
    }
}
