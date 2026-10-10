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
        for (const TCHAR* Field : { TEXT("roadside"), TEXT("transitRide"), TEXT("driving"),
            TEXT("waterControl"), TEXT("nightclub"), TEXT("residence") })
            if (!MovementProtocolPrivate::NullableObjectField(*Player, Field, false)) return false;
        // These checks preserve envelope shape only, not the nested domain schemas or their authority.
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
