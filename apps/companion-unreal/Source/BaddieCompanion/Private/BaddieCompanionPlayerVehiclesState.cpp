#include "BaddieCompanionPlayerVehiclesState.h"
#include "BaddieCompanionStateJson.h"
#include "BaddieCompanionMovementProtocolRead.h"

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
        bool BoundedValue(const FValue& Value, int32 StringLimit, int32 Depth, int32& Nodes)
        {
            if (!Value.IsValid() || Depth > 16 || ++Nodes > 20000) return false;
            if (Value->Type == EJson::String) { FString S; return String(Value, S) && Units(S) <= StringLimit; }
            if (Value->Type == EJson::Number) { double N; return Number(Value, N); }
            if (Value->Type == EJson::Array)
            {
                if (Value->AsArray().Num() > 36) return false;
                for (const auto& Item : Value->AsArray()) if (!BoundedValue(Item, StringLimit, Depth + 1, Nodes)) return false;
            }
            if (Value->Type == EJson::Object)
            {
                for (const auto& Pair : Value->AsObject()->Values)
                {
                    const FString Name(Pair.Key.Len(), *Pair.Key);
                    if (Units(Name) > 240 || !BoundedValue(Pair.Value, StringLimit, Depth + 1, Nodes)) return false;
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
        template<typename T, typename F> bool Decode(const FString& Json, T& Out, FString& Error, F Read)
        {
            const bool bBounded = Units(Json) <= Limits.Characters && WithinParseLimits(Json, Limits);
            const FString Input = bBounded ? Json : FString();
            Out = T{}; Error = ReadError;
            FObject Root; T Candidate;
            if (!bBounded || !ParseRoot(Input, Root) || !Read(Root, Candidate)) return false;
            Out = MoveTemp(Candidate); Error.Reset(); return true;
        }
    }

    bool DecodePlayerVehicleCatalog(const FString& Json, FPlayerVehicleCatalog& Out, FString& OutError)
    { return Decode(Json, Out, OutError, Catalog); }
    bool DecodePlayerVehicleRecovery(const FString& Json, FPlayerVehicleRecovery& Out, FString& OutError)
    { return Decode(Json, Out, OutError, Recovery); }
    EPlayerVehicleReadMethod FPlayerVehicleReadIntent::GetMethod() const
    { return GetKind() == EPlayerVehicleReadKind::Catalog ? EPlayerVehicleReadMethod::Mutation : EPlayerVehicleReadMethod::Query; }
    const TCHAR* FPlayerVehicleReadIntent::GetEndpoint() const
    { return GetKind() == EPlayerVehicleReadKind::Catalog ? TEXT("companionPlayerVehicles:catalog") : TEXT("companionPlayerVehicles:recovery"); }

    FPlayerVehiclesState::FPlayerVehiclesState() : Snapshot(MakeShared<FPlayerVehiclesSnapshot>()) {}
    void FPlayerVehiclesState::Retire()
    { Operation = FGuid::NewGuid(); Pending.Reset(); StagedCatalog.Reset(); }
    void FPlayerVehiclesState::Publish(FPlayerVehiclesSnapshot Next)
    { Snapshot = MakeShared<FPlayerVehiclesSnapshot>(MoveTemp(Next)); OnChanged.Broadcast(); }
    void FPlayerVehiclesState::BeginOwner(const FString& AccountId)
    {
        const bool bValid = KeyText(AccountId);
        Owner = FGuid::NewGuid(); Selection = FGuid::NewGuid(); Retire(); bActive = bValid;
        Publish({});
    }
    void FPlayerVehiclesState::Select(const TOptional<FPlayerVehicleScope>& Scope, bool bRecoveryOnly)
    {
        if (!bActive) return;
        const auto Selected = Scope;
        if (Selected.IsSet() && ValidScope(Selected.GetValue()) && Snapshot->Scope.IsSet()
            && SameScope(Selected.GetValue(), Snapshot->Scope.GetValue()) && Snapshot->bRecoveryOnly == bRecoveryOnly) return;
        Selection = FGuid::NewGuid(); Retire();
        FPlayerVehiclesSnapshot Next; Next.Scope = Selected; Next.bRecoveryOnly = bRecoveryOnly;
        if (Selected.IsSet() && !ValidScope(Selected.GetValue())) { Next.bReady = true; Next.Error = TEXT("Reopen transport for this visit."); }
        Publish(MoveTemp(Next));
    }
    bool FPlayerVehiclesState::Current(const FPlayerVehicleReadReceipt& R) const
    {
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
        if (!bActive || !Snapshot->Scope.IsSet() || !ValidScope(Snapshot->Scope.GetValue())) return {};
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
        Next.bReady = true; Next.bLoading = false; Next.Error.Reset(); Retire(); Publish(MoveTemp(Next));
        return {};
    }
    void FPlayerVehiclesState::AcceptRecovery(const FPlayerVehicleReadReceipt& Receipt, const FString& Json)
    {
        const auto R = Receipt;
        if (!Current(R) || R.Kind != EPlayerVehicleReadKind::Recovery) return;
        FPlayerVehicleRecovery Candidate; FString Error;
        if (!DecodePlayerVehicleRecovery(Json, Candidate, Error)) { FailRead(R); return; }
        auto Next = *Snapshot; Next.Catalog = MoveTemp(StagedCatalog); Next.Recovery = MoveTemp(Candidate);
        Next.bReady = true; Next.bLoading = false; Next.Error.Reset(); Retire(); Publish(MoveTemp(Next));
    }
    void FPlayerVehiclesState::FailRead(const FPlayerVehicleReadReceipt& Receipt)
    {
        const auto R = Receipt;
        if (!Current(R)) return;
        auto Next = *Snapshot; Next.Catalog.Reset(); Next.Recovery.Reset(); Next.bReady = true; Next.bLoading = false; Next.Error = ReadError;
        Retire(); Publish(MoveTemp(Next));
    }
    void FPlayerVehiclesState::StopReads()
    {
        if (!bActive) return;
        Selection = FGuid::NewGuid(); Retire(); Publish({});
    }
    void FPlayerVehiclesState::Dispose()
    {
        bActive = false; Owner = FGuid::NewGuid(); Selection = FGuid::NewGuid(); Retire(); Publish({});
    }
}
