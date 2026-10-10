#include "BaddieCompanionRacingState.h"
#include "BaddieCompanionStateJson.h"

namespace BaddieCompanion
{
    namespace RacingPrivate
    {
        using namespace StateJsonPrivate;
        // Character/car limits are canonical. Depth/separator limits bound UE JSON
        // allocation; the accepted typed object tree fits within both native limits.
        constexpr FParseLimits Limits { 32768, 16, 2048 };
        constexpr int32 MaxCars = 8;
        const TCHAR* InvalidRead = TEXT("Racing data couldn't load. Reload the world to try again.");

        bool Text(const FValue& V, FString& Out) { return String(V, Out); }
        bool BoundedText(const FValue& V, FString& Out, int32 Maximum)
        {
            return String(V, Out) && Out.Len() <= Maximum;
        }
        bool Numeric(const FValue& V, double& Out) { return Number(V, Out); }
        bool SignedInteger(const FValue& V, int64& Out) { return Integer(V, Out, -MaxInteger); }
        bool SmallInteger(const FValue& V, int32& Out, int32 Minimum, int32 Maximum)
        {
            int64 N = 0;
            if (!Integer(V, N, Minimum) || N > Maximum) return false;
            Out = static_cast<int32>(N);
            return true;
        }
        bool Input(const FValue& V, FRaceInput& Out)
        {
            const auto O = Object(V);
            return Only(O, { TEXT("throttle"), TEXT("brake"), TEXT("steering"), TEXT("handbrake"), TEXT("gear") })
                && Number(O, TEXT("throttle"), Out.Throttle) && Out.Throttle >= 0 && Out.Throttle <= 1
                && Number(O, TEXT("brake"), Out.Brake) && Out.Brake >= 0 && Out.Brake <= 1
                && Number(O, TEXT("steering"), Out.Steering) && Out.Steering >= -1 && Out.Steering <= 1
                && Boolean(Field(O, TEXT("handbrake")), Out.bHandbrake)
                && SmallInteger(Field(O, TEXT("gear")), Out.Gear, 1, 6);
        }
        bool Car(const FValue& V, FRaceCar& Out)
        {
            const auto O = Object(V);
            if (!Only(O, { TEXT("userId"), TEXT("name"), TEXT("vehicleId"), TEXT("transmission"), TEXT("x"), TEXT("z"),
                    TEXT("yaw"), TEXT("speed"), TEXT("gear"), TEXT("sequence"), TEXT("input"), TEXT("inputAt"),
                    TEXT("simulatedAt"), TEXT("recoveredAt"), TEXT("distance"), TEXT("progress"), TEXT("nextCheckpoint"),
                    TEXT("laps"), TEXT("lapStartedAt"), TEXT("lastLapMs"), TEXT("bestLapMs"), TEXT("finishedAt"), TEXT("status") })
                || !Text(Field(O, TEXT("userId")), Out.UserId) || !BoundedText(Field(O, TEXT("name")), Out.Name, 60)
                || !Text(Field(O, TEXT("vehicleId")), Out.VehicleId)
                || !EnumValue(Field(O, TEXT("transmission")), Out.Transmission, {
                    { TEXT("automatic"), ERaceTransmission::Automatic }, { TEXT("manual"), ERaceTransmission::Manual } })
                || !Number(O, TEXT("x"), Out.X) || Out.X < -155 || Out.X > 155
                || !Number(O, TEXT("z"), Out.Z) || Out.Z < -95 || Out.Z > 95
                || !Number(O, TEXT("yaw"), Out.Yaw) || !Number(O, TEXT("speed"), Out.Speed) || Out.Speed < 0 || Out.Speed > 36
                || !Number(O, TEXT("gear"), Out.Gear) || !Number(O, TEXT("sequence"), Out.Sequence)
                || !Input(Field(O, TEXT("input")), Out.Input)
                || !Number(O, TEXT("inputAt"), Out.InputAtMs) || !Number(O, TEXT("simulatedAt"), Out.SimulatedAtMs)
                || !Number(O, TEXT("distance"), Out.Distance) || !Number(O, TEXT("progress"), Out.Progress)
                || !SmallInteger(Field(O, TEXT("nextCheckpoint")), Out.NextCheckpoint, 0, 7)
                || !SmallInteger(Field(O, TEXT("laps")), Out.Laps, 0, 3)
                || !Number(O, TEXT("lapStartedAt"), Out.LapStartedAtMs)
                || !Nullable(Field(O, TEXT("lastLapMs")), Out.LastLapMs, Numeric)
                || !Nullable(Field(O, TEXT("bestLapMs")), Out.BestLapMs, Numeric)
                || !Nullable(Field(O, TEXT("finishedAt")), Out.FinishedAtMs, Numeric)
                || !EnumValue(Field(O, TEXT("status")), Out.Status, { { TEXT("ready"), ERaceCarStatus::Ready },
                    { TEXT("racing"), ERaceCarStatus::Racing }, { TEXT("finished"), ERaceCarStatus::Finished },
                    { TEXT("withdrawn"), ERaceCarStatus::Withdrawn } })) return false;
            const auto RecoveredAt = Field(O, TEXT("recoveredAt"));
            if (RecoveredAt.IsValid())
            {
                int64 Time = 0;
                if (!SignedInteger(RecoveredAt, Time)) return false;
                Out.RecoveredAtMs = Time;
            }
            return true;
        }
        bool Race(const FValue& V, FRaceState& Out)
        {
            const auto O = Object(V);
            if (!Only(O, { TEXT("id"), TEXT("instanceId"), TEXT("hostId"), TEXT("phase"), TEXT("startsAt"),
                    TEXT("endsAt"), TEXT("updatedAt"), TEXT("cars") })
                || !BoundedText(Field(O, TEXT("id")), Out.Id, 300) || !Text(Field(O, TEXT("instanceId")), Out.InstanceId)
                || !Text(Field(O, TEXT("hostId")), Out.HostId)
                || !EnumValue(Field(O, TEXT("phase")), Out.Phase, { { TEXT("lobby"), ERacePhase::Lobby },
                    { TEXT("countdown"), ERacePhase::Countdown }, { TEXT("racing"), ERacePhase::Racing },
                    { TEXT("finished"), ERacePhase::Finished }, { TEXT("cancelled"), ERacePhase::Cancelled } })
                || !Nullable(Field(O, TEXT("startsAt")), Out.StartsAtMs, SignedInteger)
                || !Nullable(Field(O, TEXT("endsAt")), Out.EndsAtMs, SignedInteger)
                || !SignedInteger(Field(O, TEXT("updatedAt")), Out.UpdatedAtMs)) return false;
            const auto Cars = Field(O, TEXT("cars"));
            if (!Cars.IsValid() || Cars->Type != EJson::Array || Cars->AsArray().Num() > MaxCars) return false;
            for (const auto& Value : Cars->AsArray())
            {
                FRaceCar Next;
                if (!Car(Value, Next)) return false;
                for (const auto& Prior : Out.Cars) if (Same(Prior.UserId, Next.UserId)) return false;
                Out.Cars.Add(MoveTemp(Next));
            }
            return true;
        }
        bool SelectedCar(const FValue& V, FRacingSelectedCar& Out)
        {
            const auto O = Object(V);
            return Only(O, { TEXT("vehicleId"), TEXT("revision"), TEXT("fuelUnits"), TEXT("condition"), TEXT("here") })
                && BoundedText(Field(O, TEXT("vehicleId")), Out.VehicleId, 240)
                && Number(O, TEXT("revision"), Out.Revision) && Number(O, TEXT("fuelUnits"), Out.FuelUnits)
                && Number(O, TEXT("condition"), Out.Condition) && Boolean(Field(O, TEXT("here")), Out.bHere);
        }
        bool Snapshot(const FObject& O, FRacingSnapshot& Out)
        {
            return Only(O, { TEXT("race"), TEXT("car"), TEXT("eligibility") })
                && Nullable(Field(O, TEXT("race")), Out.Race, Race)
                && Nullable(Field(O, TEXT("car")), Out.Car, SelectedCar)
                && EnumValue(Field(O, TEXT("eligibility")), Out.Eligibility, { { TEXT("eligible"), ERacingEligibility::Eligible },
                    { TEXT("no-car"), ERacingEligibility::NoCar }, { TEXT("car-away"), ERacingEligibility::CarAway },
                    { TEXT("car-busy"), ERacingEligibility::CarBusy }, { TEXT("no-fuel"), ERacingEligibility::NoFuel },
                    { TEXT("damaged"), ERacingEligibility::Damaged } });
        }
    }

    ERacingReadResult ReadRacingSnapshot(TSharedPtr<FJsonValue> RacingJson, FString ExpectedInstanceId,
        TSharedPtr<const FRacingSnapshot> Previous, TSharedPtr<const FRacingSnapshot>& Out, FString& OutError)
    {
        using namespace StateJsonPrivate;
        Out.Reset();
        OutError.Reset();
        if (!RacingJson.IsValid()) return ERacingReadResult::Absent;
        if (RacingJson->Type == EJson::Null) return ERacingReadResult::NullField;
        OutError = RacingPrivate::InvalidRead;
        FString Json;
        if (!String(RacingJson, Json)) return ERacingReadResult::InvalidField;
        if (Json.Len() > RacingPrivate::Limits.Characters) return ERacingReadResult::InvalidDocument;
        if (ExpectedInstanceId.IsEmpty()) return ERacingReadResult::InvalidInstance;
        FValue Null;
        if (ParseNull(Json, Null, RacingPrivate::Limits))
        {
            OutError.Reset();
            return ERacingReadResult::NullDocument;
        }
        FObject Root;
        FRacingSnapshot Next;
        if (!Parse(Json, Root, RacingPrivate::Limits) || !RacingPrivate::Snapshot(Root, Next))
            return ERacingReadResult::InvalidDocument;
        if (Next.Race.IsSet())
        {
            if (!Same(Next.Race->InstanceId, ExpectedInstanceId)) return ERacingReadResult::InstanceMismatch;
            // Validate all incoming fields first. Retain the COMPLETE prior snapshot,
            // including its selected car and eligibility, even across different race IDs.
            if (Previous.IsValid() && Previous->Race.IsSet() && Same(Previous->Race->InstanceId, ExpectedInstanceId)
                && Previous->Race->UpdatedAtMs > Next.Race->UpdatedAtMs)
            {
                Out = MoveTemp(Previous);
                OutError.Reset();
                return ERacingReadResult::RetainedOlderTick;
            }
        }
        Out = MakeShared<FRacingSnapshot>(MoveTemp(Next));
        OutError.Reset();
        return ERacingReadResult::Accepted;
    }
}
