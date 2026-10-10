#include "BaddieCompanionMovementProtocol.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

#include <cmath>

namespace BaddieCompanion
{
    namespace MovementProtocolPrivate
    {
        constexpr int64 MaxSequence = 1000000000;
        constexpr int64 MaxRevision = 9007199254740991LL;
        constexpr int32 MaxPathPoints = 32;
        constexpr double Pi = 3.14159265358979323846;

        bool ValidPosition(const FMovementPose& Position)
        {
            return std::isfinite(Position.X) && std::isfinite(Position.Z)
                && std::isfinite(Position.YawRadians) && std::abs(Position.YawRadians) <= Pi;
        }

        const TCHAR* PaceName(EMovementPace Pace)
        {
            switch (Pace)
            {
            case EMovementPace::Walk: return TEXT("walk");
            case EMovementPace::Run: return TEXT("run");
            case EMovementPace::Fast: return TEXT("fast");
            default: return nullptr;
            }
        }

        TSharedRef<FJsonObject> PositionObject(const FMovementPose& Position)
        {
            const auto Object = MakeShared<FJsonObject>();
            Object->SetNumberField(TEXT("x"), Position.X);
            Object->SetNumberField(TEXT("z"), Position.Z);
            Object->SetNumberField(TEXT("yaw"), Position.YawRadians);
            return Object;
        }

        bool EncodeRequest(const FMovementRequest& Request, bool bWorld,
            TSharedPtr<FJsonObject>& Out, FString& OutError)
        {
            Out.Reset();
            OutError = TEXT("Movement request has an invalid sequence, revision or destination.");
            if (Request.Sequence < 1 || Request.Sequence > MaxSequence
                || Request.RoomRevision < 0 || Request.RoomRevision > MaxRevision
                || !ValidPosition(Request.Destination)) return false;
            const TCHAR* Pace = PaceName(Request.Pace);
            OutError = TEXT("Movement request has an unsupported pace.");
            if (!Pace && Request.Pace != EMovementPace::Unpaced) return false;
            OutError = TEXT("Call movement does not accept a path or collision receipt.");
            if (!bWorld && (Request.Path.IsSet() || Request.Collision.IsSet())) return false;
            if (Request.Path.IsSet())
            {
                const auto& Path = Request.Path.GetValue();
                OutError = TEXT("Movement request has an invalid path.");
                if (Path.Num() > MaxPathPoints || (!Request.bStop && Path.IsEmpty())) return false;
                for (const auto& Point : Path)
                    if (!std::isfinite(Point.X) || !std::isfinite(Point.Z)) return false;
            }
            if (Request.Collision.IsSet())
            {
                const auto& Collision = Request.Collision.GetValue();
                OutError = TEXT("Movement request has an invalid collision receipt.");
                if (!Request.bStop || Collision.Sequence < 1 || Collision.Sequence > MaxSequence
                    || !std::isfinite(Collision.ElapsedMs) || Collision.ElapsedMs < 0) return false;
            }
            const auto Next = MakeShared<FJsonObject>();
            Next->SetNumberField(TEXT("sequence"), Request.Sequence);
            Next->SetNumberField(bWorld ? TEXT("revision") : TEXT("roomRevision"), Request.RoomRevision);
            Next->SetObjectField(bWorld ? TEXT("position") : TEXT("destination"), PositionObject(Request.Destination));
            // Calls omit a false stop flag; the world endpoint requires a boolean.
            if (bWorld || Request.bStop) Next->SetBoolField(TEXT("stop"), Request.bStop);
            if (Pace) Next->SetStringField(TEXT("pace"), Pace);
            if (Request.Path.IsSet())
            {
                TArray<TSharedPtr<FJsonValue>> Path;
                Path.Reserve(Request.Path.GetValue().Num());
                for (const auto& Point : Request.Path.GetValue())
                {
                    const auto Value = MakeShared<FJsonObject>();
                    Value->SetNumberField(TEXT("x"), Point.X);
                    Value->SetNumberField(TEXT("z"), Point.Z);
                    Path.Add(MakeShared<FJsonValueObject>(Value));
                }
                Next->SetArrayField(TEXT("path"), MoveTemp(Path));
            }
            if (Request.Collision.IsSet())
            {
                const auto Value = MakeShared<FJsonObject>();
                Value->SetNumberField(TEXT("sequence"), Request.Collision.GetValue().Sequence);
                Value->SetNumberField(TEXT("elapsedMs"), Request.Collision.GetValue().ElapsedMs);
                Next->SetObjectField(TEXT("collision"), Value);
            }
            Out = Next;
            OutError.Reset();
            return true;
        }

        bool ReadNumber(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key, double& Out)
        {
            const auto Value = Object.IsValid() ? Object->TryGetField(Key) : nullptr;
            return Value.IsValid() && Value->Type == EJson::Number
                && Value->TryGetNumber(Out) && std::isfinite(Out);
        }

        bool ReadNonnegativeInteger(const TSharedPtr<FJsonObject>& Object,
            const TCHAR* Key, int64 Maximum, int64& Out)
        {
            double Number = 0;
            if (!ReadNumber(Object, Key, Number) || Number < 0 || Number > Maximum
                || std::floor(Number) != Number) return false;
            Out = static_cast<int64>(Number);
            return true;
        }

        bool ReadIdentity(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key, FString& Out)
        {
            const auto Value = Object.IsValid() ? Object->TryGetField(Key) : nullptr;
            return Value.IsValid() && Value->Type == EJson::String && Value->TryGetString(Out)
                && !Out.TrimStartAndEnd().IsEmpty();
        }

        bool NullableObjectField(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key, bool bRequired)
        {
            if (!Object->HasField(Key)) return !bRequired;
            const auto Value = Object->TryGetField(Key);
            return Value.IsValid() && (Value->Type == EJson::Null
                || (Value->Type == EJson::Object && Value->AsObject().IsValid()));
        }

        bool ReadControlIdentity(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key,
            FString& Out, int32 MaximumLength = 240)
        {
            return ReadIdentity(Object, Key, Out) && Out.Len() <= MaximumLength
                && Out.Equals(Out.TrimStartAndEnd(), ESearchCase::CaseSensitive);
        }

        bool ReadBoolean(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key, bool& Out)
        {
            const auto Value = Object.IsValid() ? Object->TryGetField(Key) : nullptr;
            return Value.IsValid() && Value->Type == EJson::Boolean && Value->TryGetBool(Out);
        }

        bool ReadNumberTuple(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key,
            double* Out, int32 Count)
        {
            const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
            if (!Object.IsValid() || !Object->TryGetArrayField(Key, Values)
                || !Values || Values->Num() != Count) return false;
            for (int32 I = 0; I < Count; ++I)
            {
                const auto& Value = (*Values)[I];
                if (!Value.IsValid() || Value->Type != EJson::Number
                    || !Value->TryGetNumber(Out[I]) || !std::isfinite(Out[I])) return false;
            }
            return true;
        }

        bool ReadVehicleVector(const TSharedPtr<FJsonObject>& Object, FWorldVehicleVector& Out)
        {
            return ReadNumber(Object, TEXT("x"), Out.X) && ReadNumber(Object, TEXT("y"), Out.Y)
                && ReadNumber(Object, TEXT("z"), Out.Z);
        }

        bool ReadDrivingPose(const TSharedPtr<FJsonObject>& Object, FWorldDrivingPose& Out)
        {
            return ReadVehicleVector(Object, Out.Position) && ReadNumber(Object, TEXT("yaw"), Out.YawRadians);
        }

        bool ReadDrivingOccupant(const TSharedPtr<FJsonObject>& Object, FWorldDrivingOccupant& Out)
        {
            FString Role;
            double Seat = 0;
            return ReadControlIdentity(Object, TEXT("userId"), Out.UserId)
                && ReadControlIdentity(Object, TEXT("characterId"), Out.CharacterId)
                && ReadControlIdentity(Object, TEXT("lifeId"), Out.LifeId)
                && ReadIdentity(Object, TEXT("role"), Role) && Role.Equals(TEXT("driver"), ESearchCase::CaseSensitive)
                && ReadNumber(Object, TEXT("seat"), Seat) && Seat == 0;
        }

        bool ReadVehicleDynamics(const TSharedPtr<FJsonObject>& Object, FWorldVehicleDynamics& Out)
        {
            if (!Object.IsValid()) return false;
            double Version = 0;
            const TSharedPtr<FJsonObject>* Position = nullptr;
            const TSharedPtr<FJsonObject>* Velocity = nullptr;
            const TSharedPtr<FJsonObject>* AngularVelocity = nullptr;
            const TSharedPtr<FJsonObject>* Orientation = nullptr;
            const TArray<TSharedPtr<FJsonValue>>* Wheels = nullptr;
            if (!ReadNumber(Object, TEXT("version"), Version) || Version != 1
                || !Object->TryGetObjectField(TEXT("position"), Position) || !Position || !Position->IsValid()
                || !Object->TryGetObjectField(TEXT("velocity"), Velocity) || !Velocity || !Velocity->IsValid()
                || !Object->TryGetObjectField(TEXT("angularVelocity"), AngularVelocity) || !AngularVelocity || !AngularVelocity->IsValid()
                || !Object->TryGetObjectField(TEXT("orientation"), Orientation) || !Orientation || !Orientation->IsValid()
                || !ReadVehicleVector(*Position, Out.Position) || !ReadVehicleVector(*Velocity, Out.Velocity)
                || !ReadVehicleVector(*AngularVelocity, Out.AngularVelocity)
                || !ReadVehicleVector(*Orientation, Out.OrientationXYZ) || !ReadNumber(*Orientation, TEXT("w"), Out.OrientationW)
                || !ReadNumber(Object, TEXT("steering"), Out.Steering)
                || !ReadNumber(Object, TEXT("engineOmega"), Out.EngineOmega)
                || !ReadNumber(Object, TEXT("shiftRemaining"), Out.ShiftRemaining)
                || !ReadNonnegativeInteger(Object, TEXT("gear"), MaxRevision, Out.Gear)
                || !Object->TryGetArrayField(TEXT("wheels"), Wheels) || !Wheels) return false;
            // Current player driving is keke-only. Preserve its three authored wheel slots.
            // Physical limits and matching to an accepted vehicle configuration are not decoding.
            if (Wheels->Num() != 3) return false;
            Out.Wheels.Reserve(Wheels->Num());
            for (const auto& Item : *Wheels)
            {
                if (!Item.IsValid() || Item->Type != EJson::Object || !Item->AsObject().IsValid()) return false;
                const auto Wheel = Item->AsObject();
                FWorldVehicleWheel Value;
                if (!ReadNumber(Wheel, TEXT("angularVelocity"), Value.AngularVelocity)
                    || !ReadNumber(Wheel, TEXT("angle"), Value.Angle)
                    || !ReadNumber(Wheel, TEXT("steering"), Value.Steering)
                    || !ReadNumber(Wheel, TEXT("compression"), Value.Compression)
                    || !ReadNumber(Wheel, TEXT("normalLoad"), Value.NormalLoad)
                    || !ReadNumber(Wheel, TEXT("slipRatio"), Value.SlipRatio)
                    || !ReadNumber(Wheel, TEXT("slipAngle"), Value.SlipAngle)
                    || !ReadNumber(Wheel, TEXT("longitudinalForce"), Value.LongitudinalForce)
                    || !ReadNumber(Wheel, TEXT("lateralForce"), Value.LateralForce)
                    || !ReadBoolean(Wheel, TEXT("grounded"), Value.bGrounded)) return false;
                Out.Wheels.Add(MoveTemp(Value));
            }
            return true;
        }

        bool ReadDrivingControl(const TSharedPtr<FJsonObject>& Object, FWorldDrivingControl& Out)
        {
            if (!Object.IsValid()) return false;
            const TSharedPtr<FJsonObject>* Driver = nullptr;
            const TSharedPtr<FJsonObject>* Pose = nullptr;
            const TSharedPtr<FJsonObject>* Input = nullptr;
            const TArray<TSharedPtr<FJsonValue>>* Occupants = nullptr;
            if (!ReadControlIdentity(Object, TEXT("id"), Out.Id)
                || !ReadControlIdentity(Object, TEXT("vehicleId"), Out.VehicleId)
                || !ReadControlIdentity(Object, TEXT("gameRoomId"), Out.GameRoomId)
                || !ReadControlIdentity(Object, TEXT("instanceId"), Out.InstanceId)
                || !ReadIdentity(Object, TEXT("navigationRevision"), Out.NavigationRevision)
                || !ReadControlIdentity(Object, TEXT("routeId"), Out.RouteId)
                || !ReadNonnegativeInteger(Object, TEXT("vehicleRevision"), MaxRevision, Out.VehicleRevision)
                || !ReadNonnegativeInteger(Object, TEXT("revision"), MaxRevision, Out.Revision)
                || !ReadNonnegativeInteger(Object, TEXT("sequence"), MaxRevision, Out.Sequence)
                || !ReadNonnegativeInteger(Object, TEXT("acceptedAt"), MaxRevision, Out.AcceptedAtMs)
                || !ReadNonnegativeInteger(Object, TEXT("inputExpiresAt"), MaxRevision, Out.InputExpiresAtMs)
                || !Object->TryGetObjectField(TEXT("driver"), Driver) || !Driver || !Driver->IsValid()
                || !ReadDrivingOccupant(*Driver, Out.Driver)
                || !Object->TryGetObjectField(TEXT("pose"), Pose) || !Pose || !Pose->IsValid()
                || !ReadDrivingPose(*Pose, Out.Pose)
                || !ReadNumber(Object, TEXT("speed"), Out.Speed)
                || !ReadNumber(Object, TEXT("drivenMetres"), Out.DrivenMetres) || Out.DrivenMetres < 0
                || !ReadNumber(Object, TEXT("fuelMetres"), Out.FuelMetres) || Out.FuelMetres < 0
                || !Object->TryGetObjectField(TEXT("input"), Input) || !Input || !Input->IsValid()
                || !ReadNumber(*Input, TEXT("throttle"), Out.Throttle) || Out.Throttle < 0 || Out.Throttle > 1
                || !ReadNumber(*Input, TEXT("brake"), Out.Brake) || Out.Brake < 0 || Out.Brake > 1
                || !ReadNumber(*Input, TEXT("steering"), Out.Steering) || std::abs(Out.Steering) > 1
                || !Object->TryGetArrayField(TEXT("occupants"), Occupants) || !Occupants || Occupants->Num() != 1) return false;
            // The current contract permits exactly one driver; retain the supplied identity
            // independently, so the gameplay owner must still match it to the driver/player.
            for (const auto& Item : *Occupants)
            {
                FWorldDrivingOccupant Value;
                if (!Item.IsValid() || Item->Type != EJson::Object
                    || !ReadDrivingOccupant(Item->AsObject(), Value)) return false;
                Out.Occupants.Add(MoveTemp(Value));
            }
            FString Mode, Surface;
            if (!ReadIdentity(Object, TEXT("mode"), Mode) || !ReadIdentity(Object, TEXT("surface"), Surface)) return false;
            if (Mode.Equals(TEXT("driving"), ESearchCase::CaseSensitive)) Out.Mode = EWorldDrivingMode::Driving;
            else if (Mode.Equals(TEXT("stopped"), ESearchCase::CaseSensitive)) Out.Mode = EWorldDrivingMode::Stopped;
            else if (Mode.Equals(TEXT("pulled-over"), ESearchCase::CaseSensitive)) Out.Mode = EWorldDrivingMode::PulledOver;
            else if (Mode.Equals(TEXT("suspended"), ESearchCase::CaseSensitive)) Out.Mode = EWorldDrivingMode::Suspended;
            else return false;
            if (Surface.Equals(TEXT("clean_asphalt"), ESearchCase::CaseSensitive)) Out.Surface = EWorldDrivingSurface::CleanAsphalt;
            else if (Surface.Equals(TEXT("laterite_mud"), ESearchCase::CaseSensitive)) Out.Surface = EWorldDrivingSurface::LateriteMud;
            else if (Surface.Equals(TEXT("pothole"), ESearchCase::CaseSensitive)) Out.Surface = EWorldDrivingSurface::Pothole;
            else if (Surface.Equals(TEXT("gutter_wood"), ESearchCase::CaseSensitive)) Out.Surface = EWorldDrivingSurface::GutterWood;
            else return false;
            const auto Contact = Object->TryGetField(TEXT("contact"));
            if (!Contact.IsValid()) return false;
            if (Contact->Type != EJson::Null)
            {
                if (Contact->Type != EJson::Object || !Contact->AsObject().IsValid()) return false;
                const auto ContactObject = Contact->AsObject();
                const TSharedPtr<FJsonObject>* ContactPose = nullptr;
                FWorldDrivingContact Value;
                if (!ReadControlIdentity(ContactObject, TEXT("id"), Value.Id)
                    || !ReadControlIdentity(ContactObject, TEXT("bodyId"), Value.BodyId)
                    || !ReadNonnegativeInteger(ContactObject, TEXT("at"), MaxRevision, Value.AtMs)
                    || !ReadNumber(ContactObject, TEXT("speed"), Value.Speed)
                    || !ContactObject->TryGetObjectField(TEXT("pose"), ContactPose) || !ContactPose || !ContactPose->IsValid()
                    || !ReadDrivingPose(*ContactPose, Value.Pose)) return false;
                Out.Contact = MoveTemp(Value);
            }
            if (Object->HasField(TEXT("dynamics")))
            {
                const auto Dynamics = Object->TryGetField(TEXT("dynamics"));
                FWorldVehicleDynamics Value;
                // Optional is not nullable. Old records omit dynamics; do not manufacture it.
                if (!Dynamics.IsValid() || Dynamics->Type != EJson::Object
                    || !ReadVehicleDynamics(Dynamics->AsObject(), Value)) return false;
                Out.Dynamics = MoveTemp(Value);
            }
            return true;
        }

        bool ReadWaterControl(const TSharedPtr<FJsonObject>& Object, FWorldWaterControl& Out)
        {
            // worldWaterPublicControl excludes these fields. Do not retain a stored control
            // accidentally sent in place of its public projection, even when a field is null.
            if (!Object.IsValid() || Object->HasField(TEXT("sessionId"))
                || Object->HasField(TEXT("landingReceipt"))) return false;
            const TSharedPtr<FJsonObject>* Scope = nullptr;
            const TSharedPtr<FJsonObject>* Input = nullptr;
            if (!Object->TryGetObjectField(TEXT("scope"), Scope) || !Scope || !Scope->IsValid()
                || !Object->TryGetObjectField(TEXT("input"), Input) || !Input || !Input->IsValid()) return false;
            auto& S = Out.Scope;
            if (!ReadControlIdentity(Object, TEXT("id"), Out.Id)
                || !ReadControlIdentity(Object, TEXT("craftId"), Out.CraftId)
                || !ReadIdentity(Object, TEXT("domainId"), Out.DomainId)
                || !ReadControlIdentity(*Scope, TEXT("gameRoomId"), S.GameRoomId)
                || !ReadControlIdentity(*Scope, TEXT("instanceId"), S.InstanceId)
                || !ReadControlIdentity(*Scope, TEXT("placeId"), S.PlaceId)
                || !ReadControlIdentity(*Scope, TEXT("layoutId"), S.LayoutId, 160000)
                || !ReadControlIdentity(*Scope, TEXT("layoutRevision"), S.LayoutRevision, 160000)
                || !ReadControlIdentity(*Scope, TEXT("userId"), S.UserId)
                || !ReadControlIdentity(*Scope, TEXT("characterId"), S.CharacterId)
                || !ReadControlIdentity(*Scope, TEXT("lifeId"), S.LifeId)
                || !ReadNonnegativeInteger(*Scope, TEXT("revision"), MaxRevision, S.Revision)
                || !ReadNonnegativeInteger(Object, TEXT("craftRevision"), MaxRevision, Out.CraftRevision)
                || !ReadNonnegativeInteger(Object, TEXT("sequence"), MaxRevision, Out.Sequence)
                || !ReadNonnegativeInteger(Object, TEXT("simulatedAt"), MaxRevision, Out.SimulatedAtMs)
                || !ReadNonnegativeInteger(Object, TEXT("startedAt"), MaxRevision, Out.StartedAtMs)
                || !ReadNonnegativeInteger(Object, TEXT("expiresAt"), MaxRevision, Out.ExpiresAtMs)
                || !ReadNonnegativeInteger(Object, TEXT("endsAt"), MaxRevision, Out.EndsAtMs)
                || !ReadNonnegativeInteger(Object, TEXT("lastInputAt"), MaxRevision, Out.LastInputAtMs)) return false;
            double Point[3] = {}, Velocity[2] = {};
            if (!ReadNumberTuple(Object, TEXT("point"), Point, 3)
                || !ReadNumberTuple(Object, TEXT("velocity"), Velocity, 2)
                || !ReadNumber(Object, TEXT("yaw"), Out.YawRadians)
                || !ReadNumber(*Input, TEXT("throttle"), Out.Throttle) || std::abs(Out.Throttle) > 1
                || !ReadNumber(*Input, TEXT("steer"), Out.Steer) || std::abs(Out.Steer) > 1
                || !ReadBoolean(*Input, TEXT("brake"), Out.bBrake)) return false;
            Out.X = Point[0]; Out.Y = Point[1]; Out.Z = Point[2];
            Out.Velocity.X = Velocity[0]; Out.Velocity.Z = Velocity[1];
            FString Phase, Feedback;
            if (!ReadIdentity(Object, TEXT("phase"), Phase) || !ReadIdentity(Object, TEXT("feedback"), Feedback)) return false;
            if (Phase.Equals(TEXT("active"), ESearchCase::CaseSensitive)) Out.Phase = EWorldWaterPhase::Active;
            else if (Phase.Equals(TEXT("landed"), ESearchCase::CaseSensitive)) Out.Phase = EWorldWaterPhase::Landed;
            else return false;
            if (Feedback.Equals(TEXT("clear"), ESearchCase::CaseSensitive)) Out.Feedback = EWorldWaterFeedback::Clear;
            else if (Feedback.Equals(TEXT("hyacinth"), ESearchCase::CaseSensitive)) Out.Feedback = EWorldWaterFeedback::Hyacinth;
            else if (Feedback.Equals(TEXT("snagged"), ESearchCase::CaseSensitive)) Out.Feedback = EWorldWaterFeedback::Snagged;
            else if (Feedback.Equals(TEXT("blocked"), ESearchCase::CaseSensitive)) Out.Feedback = EWorldWaterFeedback::Blocked;
            else return false;
            const auto Snag = Object->TryGetField(TEXT("snag"));
            if (!Snag.IsValid()) return false;
            if (Snag->Type != EJson::Null)
            {
                if (Snag->Type != EJson::Object || !Snag->AsObject().IsValid()) return false;
                FWorldWaterSnag Value;
                if (!ReadControlIdentity(Snag->AsObject(), TEXT("zoneId"), Value.ZoneId)
                    || !ReadNumber(Snag->AsObject(), TEXT("exposureMs"), Value.ExposureMs) || Value.ExposureMs < 0
                    || !ReadNumber(Snag->AsObject(), TEXT("clearMs"), Value.ClearMs) || Value.ClearMs < 0
                    || !ReadBoolean(Snag->AsObject(), TEXT("held"), Value.bHeld)) return false;
                Out.Snag = MoveTemp(Value);
            }
            return true;
        }
    }

    bool EncodeCallMovementRequest(const FMovementRequest& Request,
        TSharedPtr<FJsonObject>& Out, FString& OutError)
    {
        return MovementProtocolPrivate::EncodeRequest(Request, false, Out, OutError);
    }

    bool EncodeWorldMovementRequest(const FMovementRequest& Request,
        TSharedPtr<FJsonObject>& Out, FString& OutError)
    {
        return MovementProtocolPrivate::EncodeRequest(Request, true, Out, OutError);
    }

    bool DecodeCallMovementResult(const TSharedPtr<FJsonObject>& Object,
        FCallMovementResult& Out, FString& OutError)
    {
        Out = {};
        FCallMovementResult Next;
        OutError = TEXT("Call movement response has an invalid result or revision.");
        const auto Ok = Object.IsValid() ? Object->TryGetField(TEXT("ok")) : nullptr;
        if (!Ok.IsValid() || Ok->Type != EJson::Boolean || !Ok->TryGetBool(Next.bOk)
            || !MovementProtocolPrivate::ReadNonnegativeInteger(Object, TEXT("sequence"), MovementProtocolPrivate::MaxSequence, Next.Sequence)
            || !MovementProtocolPrivate::ReadNonnegativeInteger(Object, TEXT("roomRevision"), MovementProtocolPrivate::MaxRevision, Next.RoomRevision)
            || !MovementProtocolPrivate::ReadNumber(Object, TEXT("retryAfterMs"), Next.RetryAfterMs) || Next.RetryAfterMs < 0) return false;

        OutError = TEXT("Call movement response has an invalid position.");
        const TSharedPtr<FJsonObject>* Position = nullptr;
        if (!Object->TryGetObjectField(TEXT("position"), Position) || !Position || !Position->IsValid()
            || !MovementProtocolPrivate::ReadNumber(*Position, TEXT("x"), Next.Position.X)
            || !MovementProtocolPrivate::ReadNumber(*Position, TEXT("z"), Next.Position.Z)
            || !MovementProtocolPrivate::ReadNumber(*Position, TEXT("yaw"), Next.Position.YawRadians)
            || !MovementProtocolPrivate::ValidPosition(Next.Position)) return false;

        OutError = TEXT("Call movement response has an invalid movement.");
        const auto Movement = Object->TryGetField(TEXT("movement"));
        if (!Movement.IsValid()) return false;
        if (Movement->Type != EJson::Null)
        {
            FMovementRoute Route;
            if (Movement->Type != EJson::Object
                || !DecodeMovementRoute(Movement->AsObject(), Route, OutError)) return false;
            OutError = TEXT("Call movement response and route identities do not match.");
            if (Route.Sequence != Next.Sequence || Route.RoomRevision != Next.RoomRevision) return false;
            Next.Movement = MoveTemp(Route);
        }
        if (Object->HasField(TEXT("reason")))
        {
            const auto Reason = Object->TryGetField(TEXT("reason"));
            FString Value;
            OutError = TEXT("Call movement response has an invalid reason.");
            if (!Reason.IsValid() || Reason->Type != EJson::String || !Reason->TryGetString(Value)) return false;
            Next.Reason = MoveTemp(Value);
        }
        Out = MoveTemp(Next);
        OutError.Reset();
        return true;
    }

    bool DecodeWorldMovementResult(const TSharedPtr<FJsonObject>& Object,
        FWorldMovementResult& Out, FString& OutError)
    {
        Out = {};
        FWorldMovementResult Next;
        const TSharedPtr<FJsonObject>* Player = nullptr;
        OutError = TEXT("World movement response has an invalid player or server time.");
        if (!Object.IsValid() || !Object->TryGetObjectField(TEXT("player"), Player) || !Player || !Player->IsValid()
            || !MovementProtocolPrivate::ReadNonnegativeInteger(Object, TEXT("serverNow"),
                MovementProtocolPrivate::MaxRevision, Next.ServerNowMs) || Next.ServerNowMs == 0) return false;

        OutError = TEXT("World movement response has an invalid player identity or revision.");
        if (!MovementProtocolPrivate::ReadIdentity(*Player, TEXT("userId"), Next.UserId)
            || !MovementProtocolPrivate::ReadIdentity(*Player, TEXT("characterId"), Next.CharacterId)
            || !MovementProtocolPrivate::ReadIdentity(*Player, TEXT("gameRoomId"), Next.GameRoomId)
            || !MovementProtocolPrivate::ReadIdentity(*Player, TEXT("placeId"), Next.PlaceId)
            || !MovementProtocolPrivate::ReadIdentity(*Player, TEXT("instanceId"), Next.InstanceId)
            || !MovementProtocolPrivate::ReadIdentity(*Player, TEXT("homeRoomId"), Next.HomeRoomId)
            || !MovementProtocolPrivate::ReadNonnegativeInteger(*Player, TEXT("revision"),
                MovementProtocolPrivate::MaxRevision, Next.Revision)
            || !MovementProtocolPrivate::ReadNonnegativeInteger(*Player, TEXT("moveSequence"),
                MovementProtocolPrivate::MaxSequence, Next.MoveSequence)) return false;

        OutError = TEXT("World movement response has an invalid player status or position.");
        FString Status;
        const TSharedPtr<FJsonObject>* Position = nullptr;
        if (!MovementProtocolPrivate::ReadIdentity(*Player, TEXT("status"), Status)
            || !MovementProtocolPrivate::ReadNumber(*Player, TEXT("health"), Next.Health)
            || !(*Player)->TryGetObjectField(TEXT("position"), Position) || !Position || !Position->IsValid()
            || !MovementProtocolPrivate::ReadNumber(*Position, TEXT("x"), Next.Position.X)
            || !MovementProtocolPrivate::ReadNumber(*Position, TEXT("z"), Next.Position.Z)
            || !MovementProtocolPrivate::ReadNumber(*Position, TEXT("yaw"), Next.Position.YawRadians)) return false;
        // worldPoint permits a finite yaw; decoding a returned position does not wrap or replan it.
        if (Status.Equals(TEXT("present"), ESearchCase::CaseSensitive)) Next.Status = EWorldPlayerStatus::Present;
        else if (Status.Equals(TEXT("traveling"), ESearchCase::CaseSensitive)) Next.Status = EWorldPlayerStatus::Traveling;
        else if (Status.Equals(TEXT("recovering"), ESearchCase::CaseSensitive)) Next.Status = EWorldPlayerStatus::Recovering;
        else return false;

        OutError = TEXT("World movement response has an invalid movement.");
        const auto Movement = (*Player)->TryGetField(TEXT("movement"));
        if (!Movement.IsValid()) return false;
        if (Movement->Type != EJson::Null)
        {
            FMovementRoute Route;
            if (Movement->Type != EJson::Object
                || !DecodeMovementRoute(Movement->AsObject(), Route, OutError)) return false;
            OutError = TEXT("World movement response and route identities do not match.");
            if (Route.Sequence != Next.MoveSequence || Route.RoomRevision != Next.Revision) return false;
            Next.Movement = MoveTemp(Route);
        }

        OutError = TEXT("World movement response has an invalid player state.");
        if (!MovementProtocolPrivate::NullableObjectField(*Player, TEXT("journey"), true)) return false;
        for (const TCHAR* Field : { TEXT("roadside"), TEXT("transitRide"),
            TEXT("nightclub"), TEXT("residence") })
            if (!MovementProtocolPrivate::NullableObjectField(*Player, Field, false)) return false;

        Next.bHasDrivingField = (*Player)->HasField(TEXT("driving"));
        if (Next.bHasDrivingField)
        {
            OutError = TEXT("World movement response has an invalid driving control.");
            const auto Driving = (*Player)->TryGetField(TEXT("driving"));
            if (!Driving.IsValid()) return false;
            if (Driving->Type != EJson::Null)
            {
                FWorldDrivingControl Value;
                if (Driving->Type != EJson::Object
                    || !MovementProtocolPrivate::ReadDrivingControl(Driving->AsObject(), Value)) return false;
                Next.Driving = MoveTemp(Value);
            }
        }
        Next.bHasWaterControlField = (*Player)->HasField(TEXT("waterControl"));
        if (Next.bHasWaterControlField)
        {
            OutError = TEXT("World movement response has an invalid public water control.");
            const auto Water = (*Player)->TryGetField(TEXT("waterControl"));
            if (!Water.IsValid()) return false;
            if (Water->Type != EJson::Null)
            {
                FWorldWaterControl Value;
                if (Water->Type != EJson::Object
                    || !MovementProtocolPrivate::ReadWaterControl(Water->AsObject(), Value)) return false;
                Next.WaterControl = MoveTemp(Value);
            }
        }
        OutError = TEXT("World movement response has an invalid player state.");
        // Remaining envelope checks do not decode the other domain schemas or their authority.
        if ((*Player)->HasField(TEXT("drawnWeaponId")))
        {
            const auto Weapon = (*Player)->TryGetField(TEXT("drawnWeaponId"));
            if (!Weapon.IsValid() || (Weapon->Type != EJson::Null && Weapon->Type != EJson::String)) return false;
        }
        const auto Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Next.PlayerDocumentJson);
        OutError = TEXT("World movement player document could not be retained.");
        if (!FJsonSerializer::Serialize(Player->ToSharedRef(), Writer)) return false;
        Out = MoveTemp(Next);
        OutError.Reset();
        return true;
    }
}
