#include "BaddieCompanionResidentPresentation.h"
#include "BaddieCompanionStateJson.h"

#include <cmath>

namespace BaddieCompanion
{
    namespace ResidentPresentationPrivate
    {
        using StateJsonPrivate::MaxInteger;
        using StateJsonPrivate::Same;
        using StateJsonPrivate::ValidTime;
        constexpr double Pi = 3.14159265358979323846;

        bool TrimSpace(uint32 C)
        {
            // ECMAScript trim whitespace and line terminators, not locale whitespace.
            return (C >= 0x09 && C <= 0x0d) || C == 0x20 || C == 0xa0 || C == 0x1680
                || (C >= 0x2000 && C <= 0x200a) || C == 0x2028 || C == 0x2029
                || C == 0x202f || C == 0x205f || C == 0x3000 || C == 0xfeff;
        }
        bool NonemptyString(const FString& Value)
        {
            return !Value.IsEmpty() && !TrimSpace(static_cast<uint32>(Value[0]))
                && !TrimSpace(static_cast<uint32>(Value[Value.Len() - 1]));
        }
        int64 Utf16Length(const FString& Value)
        {
            int64 Length = 0;
            for (const TCHAR C : Value)
                Length += sizeof(TCHAR) > 2 && static_cast<uint32>(C) > 0xffff ? 2 : 1;
            return Length;
        }
        bool NonnegativeInteger(int64 Value) { return Value >= 0 && Value <= MaxInteger; }
        bool ValidYaw(double Value) { return std::isfinite(Value) && std::abs(Value) <= Pi; }
        bool ValidPose(const FResidentPose& Pose)
        {
            return std::isfinite(Pose.X) && std::isfinite(Pose.Y) && std::isfinite(Pose.Z)
                && ValidYaw(Pose.YawRadians);
        }
        bool ValidPlace(const FResidentPlace& Place)
        {
            return NonemptyString(Place.PlaceId) && NonemptyString(Place.InstanceId);
        }
        bool ValidAnchor(const FResidentAnchor& Anchor) { return ValidPlace(Anchor.Place) && ValidPose(Anchor.Pose); }
        bool SamePlace(const FResidentPlace& A, const FResidentPlace& B)
        {
            return Same(A.PlaceId, B.PlaceId) && Same(A.InstanceId, B.InstanceId);
        }
        bool BoundPlace(const FResidentPlace& Place, const FResidentPlaceBinding& Binding)
        {
            return Same(Place.PlaceId, Binding.PlaceId) && Same(Place.InstanceId, Binding.ResidentInstanceId);
        }
        const TCHAR* ActorKey(EResidentKey Key)
        {
            switch (Key)
            {
            case EResidentKey::MaiSuya: return TEXT("mai-suya");
            case EResidentKey::AkaraWoman: return TEXT("akara-woman");
            case EResidentKey::Courier: return TEXT("courier");
            case EResidentKey::DanfoDriver: return TEXT("danfo-driver");
            case EResidentKey::Medic: return TEXT("medic");
            case EResidentKey::Tailor: return TEXT("tailor");
            case EResidentKey::StreetAkaraVendor: return TEXT("street-akara-vendor");
            case EResidentKey::AreaTout: return TEXT("area-tout");
            case EResidentKey::TrafficHawker: return TEXT("traffic-hawker");
            case EResidentKey::PosAgent: return TEXT("pos-agent");
            case EResidentKey::DanfoConductor: return TEXT("danfo-conductor");
            default: return nullptr;
            }
        }
        FString ResidentId(const FString& WorldKey, const TCHAR* Key)
        {
            return FString::Printf(TEXT("npc:%lld:"), static_cast<long long>(Utf16Length(WorldKey)))
                + WorldKey + TEXT(":") + Key;
        }
        bool AsciiAlphanumeric(TCHAR C)
        {
            return (C >= TEXT('A') && C <= TEXT('Z')) || (C >= TEXT('a') && C <= TEXT('z'))
                || (C >= TEXT('0') && C <= TEXT('9'));
        }
        bool BoundedKey(const FString& Value)
        {
            if (Value.IsEmpty() || Value.Len() > 160 || !AsciiAlphanumeric(Value[0])) return false;
            for (const TCHAR C : Value)
                if (!AsciiAlphanumeric(C) && C != TEXT('.') && C != TEXT('_') && C != TEXT(':') && C != TEXT('-')) return false;
            return true;
        }
        bool ResidentInWorld(const FString& Id, const FString& WorldKey)
        {
            if (!BoundedKey(Id)) return false;
            const EResidentKey Keys[] = { EResidentKey::MaiSuya, EResidentKey::AkaraWoman, EResidentKey::Courier,
                EResidentKey::DanfoDriver, EResidentKey::Medic, EResidentKey::Tailor, EResidentKey::StreetAkaraVendor,
                EResidentKey::AreaTout, EResidentKey::TrafficHawker, EResidentKey::PosAgent, EResidentKey::DanfoConductor };
            for (const EResidentKey Key : Keys) if (Same(Id, ResidentId(WorldKey, ActorKey(Key)))) return true;
            return false;
        }
        bool RoutineAction(EResidentRoutineAction Action)
        {
            switch (Action)
            {
            case EResidentRoutineAction::Rest:
            case EResidentRoutineAction::Work:
            case EResidentRoutineAction::Socialize: return true;
            default: return false;
            }
        }
        bool StationaryAction(EResidentAction Action)
        {
            switch (Action)
            {
            case EResidentAction::Rest:
            case EResidentAction::Work:
            case EResidentAction::Eat:
            case EResidentAction::Socialize:
            case EResidentAction::Recover: return true;
            default: return false;
            }
        }
        bool SameVehicleId(const TOptional<FString>& A, const TOptional<FString>& B)
        {
            return A.IsSet() == B.IsSet() && (!A.IsSet() || Same(A.GetValue(), B.GetValue()));
        }
        bool ValidTransport(const FResidentJourney& Journey, const FResidentIdentity& Identity)
        {
            EResidentVehicleKind ExpectedKind;
            switch (Journey.Mode)
            {
            case EResidentTravelMode::Walk: return !Journey.VehicleId.IsSet() && !Journey.Transport.IsSet();
            case EResidentTravelMode::Car: ExpectedKind = EResidentVehicleKind::Car; break;
            case EResidentTravelMode::Okada: ExpectedKind = EResidentVehicleKind::Okada; break;
            case EResidentTravelMode::Keke: ExpectedKind = EResidentVehicleKind::Keke; break;
            case EResidentTravelMode::Danfo: ExpectedKind = EResidentVehicleKind::Danfo; break;
            default: return false;
            }
            if (!Journey.VehicleId.IsSet() || !BoundedKey(Journey.VehicleId.GetValue()) || !Journey.Transport.IsSet()
                || !BoundedKey(Identity.WorldKey) || Utf16Length(Identity.WorldKey) > 80) return false;
            const FResidentJourneyTransport& Transport = Journey.Transport.GetValue();
            if (!Same(Transport.VehicleId, Journey.VehicleId.GetValue())
                || !ResidentInWorld(Transport.OwnerResidentId, Identity.WorldKey)
                || (Transport.RiderResidentId.IsSet() && !ResidentInWorld(Transport.RiderResidentId.GetValue(), Identity.WorldKey))
                || (!Same(Transport.OwnerResidentId, Identity.Id)
                    && (!Transport.RiderResidentId.IsSet() || !Same(Transport.RiderResidentId.GetValue(), Identity.Id)))) return false;
            const FResidentVehicleModel& Model = Transport.Model;
            if (Model.Kind != ExpectedKind || !BoundedKey(Model.CatalogueId) || !BoundedKey(Model.ModelId)
                || !NonnegativeInteger(Model.CatalogueRevision) || Model.CatalogueRevision < 1
                || Model.TankCapacityUnits < 1 || Model.TankCapacityUnits > 1000000
                || Model.MetresPerFuelUnit < 1 || Model.MetresPerFuelUnit > 1000000) return false;
            if (!Transport.Asset.IsSet()) return true;
            const FResidentVehicleAsset& Asset = Transport.Asset.GetValue();
            return NonemptyString(Asset.Key) && Utf16Length(Asset.Key) <= 160 && ValidYaw(Asset.SourceYawRadians);
        }
        bool ValidMovement(const FMovementRoute& Movement, double& EndsAt)
        {
            if (Movement.Version != 1 || Movement.Sequence < 1 || Movement.Sequence > 1000000000
                || !NonnegativeInteger(Movement.RoomRevision) || !ValidTime(Movement.StartedAtMs)
                || !std::isfinite(Movement.From.X) || !std::isfinite(Movement.From.Z)
                || !ValidYaw(Movement.From.YawRadians) || !ValidYaw(Movement.YawRadians)
                || (Movement.InitialSpeedMps.IsSet() && (!std::isfinite(Movement.InitialSpeedMps.GetValue())
                    || Movement.InitialSpeedMps.GetValue() < 0)) || Movement.Path.Num() > 32) return false;
            switch (Movement.Pace)
            {
            case EMovementPace::Unpaced:
            case EMovementPace::Walk:
            case EMovementPace::Run:
            case EMovementPace::Fast: break;
            default: return false;
            }
            for (const FMovementPoint& Point : Movement.Path)
                if (!std::isfinite(Point.X) || !std::isfinite(Point.Z)) return false;
            return MovementEndsAt(Movement, EndsAt) && ValidTime(EndsAt)
                && EndsAt >= Movement.StartedAtMs && EndsAt - Movement.StartedAtMs <= 90000;
        }
        bool StartsAtPose(const FMovementRoute& Movement, const FResidentPose& Pose)
        {
            return Movement.From.X == Pose.X && Movement.From.Z == Pose.Z && Movement.From.YawRadians == Pose.YawRadians;
        }
        bool EndsAtPose(const FMovementRoute& Movement, const FResidentPose& Pose)
        {
            const double X = Movement.Path.IsEmpty() ? Movement.From.X : Movement.Path.Last().X;
            const double Z = Movement.Path.IsEmpty() ? Movement.From.Z : Movement.Path.Last().Z;
            return X == Pose.X && Z == Pose.Z && Movement.YawRadians == Pose.YawRadians;
        }
        bool SamplePlaceMovement(const FMovementRoute& Movement, double FloorY, double Now, double EndsAt,
            FResidentPlaceSample& Out)
        {
            if (Now < Movement.StartedAtMs) return false;
            FMovementSample Sample;
            if (!SampleMovement(Movement, Now, Sample)) return false;
            Out.Pose = { Sample.Position.X, FloorY, Sample.Position.Z, Sample.Position.YawRadians };
            if (!ValidPose(Out.Pose) || !std::isfinite(Sample.SpeedMps)) return false;
            Out.bMoving = Sample.bMoving && Sample.SpeedMps > 0 && Now < EndsAt;
            Out.bWorking = false;
            return true;
        }
        bool ValidCorridor(const FResidentCorridorRoute& Route)
        {
            if (!NonemptyString(Route.Id) || Route.Version != EResidentRouteVersion::LagosCorridorsV1
                || Route.Placement != EResidentRoutePlacement::Proposed || !std::isfinite(Route.LengthMetres)
                || Route.LengthMetres <= 0 || Route.Points.Num() < 2 || Route.Points.Num() > 256) return false;
            for (const FResidentGeoPoint& Point : Route.Points)
                if (!std::isfinite(Point.Latitude) || std::abs(Point.Latitude) > 90
                    || !std::isfinite(Point.Longitude) || std::abs(Point.Longitude) > 180) return false;
            return true;
        }
        bool Sample(const FResidentView& Resident, const FResidentPlaceBinding& Binding, double Now,
            FResidentPlaceSample& Out)
        {
            const FResidentIdentity& Identity = Resident.Identity;
            const TCHAR* Key = ActorKey(Identity.ActorKey);
            if (!ValidTime(Now) || !NonemptyString(Binding.PlaceId) || !NonemptyString(Binding.ResidentInstanceId)
                || !NonemptyString(Binding.GameInstanceId) || !NonemptyString(Binding.LayoutId)
                || Identity.ActorKind != EResidentActorKind::Npc || Identity.RosterVersion != 1 || !Key
                || !NonemptyString(Identity.Id) || !NonemptyString(Identity.WorldKey) || !NonemptyString(Resident.ClockEpochKey)
                || !Same(Identity.Id, ResidentId(Identity.WorldKey, Key)) || !NonnegativeInteger(Resident.Revision)
                || !NonnegativeInteger(Resident.UpdatedAtGameMinute) || !std::isfinite(Resident.Health.Health)
                || !std::isfinite(Resident.Health.MaxHealth) || Resident.Health.Health <= 0
                || Resident.Health.Health > Resident.Health.MaxHealth) return false;
            const FResidentActivity& Activity = Resident.Activity;
            switch (Activity.Kind)
            {
            case EResidentActivityKind::AtPlace:
            {
                if (!Activity.AtPlace.IsSet() || Activity.Traveling.IsSet() || Resident.Journey.IsSet()) return false;
                const FResidentAtPlaceActivity& AtPlace = Activity.AtPlace.GetValue();
                if (!NonnegativeInteger(AtPlace.SinceGameMinute) || AtPlace.SinceGameMinute > Resident.UpdatedAtGameMinute
                    || !ValidAnchor(AtPlace.Location) || !BoundPlace(AtPlace.Location.Place, Binding)
                    || !StationaryAction(AtPlace.Action)
                    || (AtPlace.InteractionId.IsSet() && !NonemptyString(AtPlace.InteractionId.GetValue()))) return false;
                if (!Resident.LocalMove.IsSet())
                {
                    Out.Pose = AtPlace.Location.Pose;
                    Out.bMoving = false;
                    Out.bWorking = AtPlace.Action == EResidentAction::Work;
                    return true;
                }
                const FResidentLocalMove& Move = Resident.LocalMove.GetValue();
                double EndsAt = 0;
                if (AtPlace.InteractionId.IsSet() || !ValidAnchor(Move.To) || !SamePlace(AtPlace.Location.Place, Move.To.Place)
                    || !RoutineAction(Move.Action) || Move.To.Pose.Y != AtPlace.Location.Pose.Y
                    || !ValidMovement(Move.Movement, EndsAt) || !StartsAtPose(Move.Movement, AtPlace.Location.Pose)
                    || !EndsAtPose(Move.Movement, Move.To.Pose)) return false;
                return SamplePlaceMovement(Move.Movement, AtPlace.Location.Pose.Y, Now, EndsAt, Out);
            }
            case EResidentActivityKind::Traveling: break;
            default: return false;
            }
            if (!Activity.Traveling.IsSet() || Activity.AtPlace.IsSet() || Resident.LocalMove.IsSet() || !Resident.Journey.IsSet()) return false;
            const FResidentTravelActivity& Travel = Activity.Traveling.GetValue();
            const FResidentJourney& Journey = Resident.Journey.GetValue();
            double DepartureEnd = 0, ArrivalEnd = 0;
            if (!NonnegativeInteger(Travel.SinceGameMinute) || Travel.SinceGameMinute > Resident.UpdatedAtGameMinute
                || !NonemptyString(Journey.Id) || !Same(Travel.JourneyId, Journey.Id)
                || !ValidPlace(Travel.From) || !ValidPlace(Travel.To) || !ValidAnchor(Journey.From) || !ValidAnchor(Journey.To)
                || !SamePlace(Travel.From, Journey.From.Place) || !SamePlace(Travel.To, Journey.To.Place)
                || Same(Journey.From.Place.PlaceId, Journey.To.Place.PlaceId) || Same(Journey.From.Place.InstanceId, Journey.To.Place.InstanceId)
                || Journey.Mode != Travel.Mode || !SameVehicleId(Journey.VehicleId, Travel.VehicleId)
                || !ValidTransport(Journey, Identity) || Journey.TimeBasis != EResidentJourneyTimeBasis::ServerMs
                || !RoutineAction(Journey.ArrivalAction) || !ValidMovement(Journey.Departure, DepartureEnd)
                || !ValidMovement(Journey.Arrival, ArrivalEnd) || Journey.Departure.Sequence != Journey.Arrival.Sequence
                || Journey.Departure.RoomRevision != Journey.Arrival.RoomRevision || !StartsAtPose(Journey.Departure, Journey.From.Pose)
                || !EndsAtPose(Journey.Arrival, Journey.To.Pose) || !ValidTime(Journey.RouteStartedAtRealMs)
                || !ValidTime(Journey.RouteEndsAtRealMs) || !ValidTime(Journey.ArrivesAtRealMs)
                || DepartureEnd > Journey.RouteStartedAtRealMs || Journey.RouteEndsAtRealMs <= Journey.RouteStartedAtRealMs
                || Journey.Arrival.StartedAtMs != Journey.RouteEndsAtRealMs || ArrivalEnd > Journey.ArrivesAtRealMs
                || Now < Journey.Departure.StartedAtMs || !ValidCorridor(Journey.Route)) return false;
            if (Now < Journey.RouteStartedAtRealMs)
                return BoundPlace(Journey.From.Place, Binding)
                    && SamplePlaceMovement(Journey.Departure, Journey.From.Pose.Y, Now, DepartureEnd, Out);
            if (Now < Journey.RouteEndsAtRealMs) return false;
            return BoundPlace(Journey.To.Place, Binding)
                && SamplePlaceMovement(Journey.Arrival, Journey.To.Pose.Y, Now, ArrivalEnd, Out);
        }
    }

    bool SampleResidentInPlace(const FResidentView& Resident, const FResidentPlaceBinding& Binding,
        double ServerNowMs, FResidentPlaceSample& Out, FString& OutError)
    {
        FResidentPlaceSample Candidate;
        const bool bSampled = ResidentPresentationPrivate::Sample(Resident, Binding, ServerNowMs, Candidate);
        // No input is read after either caller-owned output is changed.
        Out = bSampled ? Candidate : FResidentPlaceSample();
        if (bSampled) OutError.Reset();
        else OutError = TEXT("Resident place sample is unavailable.");
        return bSampled;
    }
}
