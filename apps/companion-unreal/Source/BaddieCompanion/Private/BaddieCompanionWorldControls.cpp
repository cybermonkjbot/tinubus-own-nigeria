#include "BaddieCompanionWorldControls.h"

#include <cmath>

namespace BaddieCompanion
{
    namespace WorldControlsPrivate
    {
        constexpr int64 MaxSafeInteger = 9007199254740991LL;
        // companionPlayerDrivingRules.maxSpeed, in source metres per second.
        constexpr double MaxDrivingSpeed = 7;

        bool Same(const FString& A, const FString& B)
        {
            return A.Equals(B, ESearchCase::CaseSensitive);
        }

        bool ScopeKey(const FString& Value, int32 Maximum = 240)
        {
            return !Value.IsEmpty() && Value.Len() <= Maximum;
        }

        bool Key(const FString& Value)
        {
            return ScopeKey(Value) && Same(Value, Value.TrimStartAndEnd());
        }

        bool Integer(int64 Value)
        {
            return Value >= 0 && Value <= MaxSafeInteger;
        }

        bool Nonnegative(double Value)
        {
            return std::isfinite(Value) && Value >= 0;
        }

        bool InRange(double Value, double Minimum, double Maximum)
        {
            return std::isfinite(Value) && Value >= Minimum && Value <= Maximum;
        }

        bool LifeMatches(const FString& LifeId, const FString& CurrentLifeId)
        {
            return Key(LifeId) && (CurrentLifeId.IsEmpty() || Same(LifeId, CurrentLifeId));
        }

        bool ValidDriving(const FWorldDrivingControl& Driving, const FWorldPlayerView& Player,
            const FWorldCurrentInstanceIdentity& Instance, const FString& CurrentLifeId)
        {
            const auto& Pose = Driving.Pose;
            const auto& Driver = Driving.Driver;
            const bool bKnownMode = Driving.Mode == EWorldDrivingMode::Driving
                || Driving.Mode == EWorldDrivingMode::Stopped || Driving.Mode == EWorldDrivingMode::PulledOver
                || Driving.Mode == EWorldDrivingMode::Suspended;
            const bool bKnownSurface = Driving.Surface == EWorldDrivingSurface::CleanAsphalt
                || Driving.Surface == EWorldDrivingSurface::LateriteMud || Driving.Surface == EWorldDrivingSurface::Pothole
                || Driving.Surface == EWorldDrivingSurface::GutterWood;
            if (!Key(Driving.Id) || !Key(Driving.VehicleId) || !Key(Driving.RouteId)
                || !Same(Driving.GameRoomId, Player.GameRoomId) || !Same(Driving.InstanceId, Player.InstanceId)
                || !Same(Driving.NavigationRevision, Instance.NavigationRevision)
                || !Integer(Driving.Revision) || !Integer(Driving.Sequence) || !Integer(Driving.VehicleRevision)
                || !Integer(Driving.AcceptedAtMs) || !Integer(Driving.InputExpiresAtMs)
                || !std::isfinite(Pose.Position.X) || !std::isfinite(Pose.Position.Y)
                || !std::isfinite(Pose.Position.Z) || !std::isfinite(Pose.YawRadians)
                || !InRange(Driving.Speed, -MaxDrivingSpeed, MaxDrivingSpeed)
                || !Nonnegative(Driving.DrivenMetres) || !Nonnegative(Driving.FuelMetres)
                || !InRange(Driving.Throttle, 0, 1) || !InRange(Driving.Brake, 0, 1)
                || !InRange(Driving.Steering, -1, 1) || !bKnownMode || !bKnownSurface
                || !Same(Driver.UserId, Player.UserId) || !Same(Driver.CharacterId, Player.CharacterId)
                || !LifeMatches(Driver.LifeId, CurrentLifeId) || Driving.Occupants.Num() != 1) return false;
            // The decoder's occupant type represents only the driver role in seat zero.
            const auto& Occupant = Driving.Occupants[0];
            return Same(Occupant.UserId, Player.UserId) && Same(Occupant.CharacterId, Player.CharacterId)
                && Same(Occupant.LifeId, Driver.LifeId);
        }

        bool ValidWaterScope(const FWorldWaterScope& Scope)
        {
            return ScopeKey(Scope.GameRoomId) && ScopeKey(Scope.InstanceId) && ScopeKey(Scope.PlaceId)
                && ScopeKey(Scope.UserId) && ScopeKey(Scope.CharacterId) && ScopeKey(Scope.LifeId)
                && ScopeKey(Scope.LayoutId, 160000) && ScopeKey(Scope.LayoutRevision, 160000)
                // validCompanionWaterScope uses the stricter traversal integer bound.
                && Scope.Revision >= 0 && Scope.Revision < MaxSafeInteger - 1;
        }

        bool SameWaterScope(const FWorldWaterScope& A, const FWorldWaterScope& B)
        {
            return ValidWaterScope(A) && ValidWaterScope(B)
                && Same(A.GameRoomId, B.GameRoomId) && Same(A.InstanceId, B.InstanceId)
                && Same(A.PlaceId, B.PlaceId) && Same(A.LayoutId, B.LayoutId)
                && Same(A.LayoutRevision, B.LayoutRevision) && Same(A.UserId, B.UserId)
                && Same(A.CharacterId, B.CharacterId) && Same(A.LifeId, B.LifeId) && A.Revision == B.Revision;
        }

        bool ValidWater(const FWorldWaterControl& Water, const FWorldPlayerView& Player,
            const FWorldCurrentInstanceIdentity& Instance, const FString& CurrentLifeId)
        {
            const auto& Scope = Water.Scope;
            const bool bKnownPhase = Water.Phase == EWorldWaterPhase::Active || Water.Phase == EWorldWaterPhase::Landed;
            const bool bKnownFeedback = Water.Feedback == EWorldWaterFeedback::Clear
                || Water.Feedback == EWorldWaterFeedback::Hyacinth || Water.Feedback == EWorldWaterFeedback::Snagged
                || Water.Feedback == EWorldWaterFeedback::Blocked;
            return ValidWaterScope(Scope) && Key(Water.Id) && Key(Water.CraftId) && !Water.DomainId.IsEmpty()
                && Same(Scope.GameRoomId, Player.GameRoomId) && Same(Scope.InstanceId, Player.InstanceId)
                && Same(Scope.PlaceId, Player.PlaceId) && Same(Scope.LayoutId, Instance.LayoutId)
                && Same(Scope.LayoutRevision, Instance.NavigationRevision)
                && Same(Scope.UserId, Player.UserId) && Same(Scope.CharacterId, Player.CharacterId)
                && Scope.Revision == Player.Revision && LifeMatches(Scope.LifeId, CurrentLifeId)
                && Integer(Water.Sequence) && Integer(Water.CraftRevision) && Integer(Water.SimulatedAtMs)
                && Integer(Water.StartedAtMs) && Integer(Water.ExpiresAtMs) && Integer(Water.EndsAtMs)
                && Integer(Water.LastInputAtMs) && bKnownPhase
                && std::isfinite(Water.X) && std::isfinite(Water.Y) && std::isfinite(Water.Z)
                && std::isfinite(Water.YawRadians) && std::isfinite(Water.Velocity.X) && std::isfinite(Water.Velocity.Z)
                && InRange(Water.Throttle, -1, 1) && InRange(Water.Steer, -1, 1)
                && Water.StartedAtMs <= Water.SimulatedAtMs && Water.LastInputAtMs <= Water.SimulatedAtMs
                && Water.ExpiresAtMs <= Water.EndsAtMs && bKnownFeedback;
        }

        template <typename T> bool SelectField(const TWorldPlayerField<T>& Field, TOptional<T>& Out)
        {
            if (Field.Presence == EWorldFieldPresence::Value)
            {
                if (!Field.Value.IsSet()) return false;
                Out = Field.Value;
                return true;
            }
            return !Field.Value.IsSet() && (Field.Presence == EWorldFieldPresence::Missing
                || Field.Presence == EWorldFieldPresence::Null);
        }

        bool Conflicting(const FWorldTransportRead& Read, const FWorldPlayerView& Player)
        {
            const bool bDriving = Read.Driving.IsSet();
            const bool bActiveWater = Read.WaterControl.IsSet()
                && Read.WaterControl.GetValue().Phase == EWorldWaterPhase::Active;
            const bool bTravelingJourney = Player.Journey.Value.IsSet()
                && Player.Journey.Value.GetValue().Status == EWorldJourneyStatus::Traveling;
            return (bDriving && bActiveWater) || ((bDriving || bActiveWater)
                && (Player.Movement.IsSet() || Player.TransitRide.Value.IsSet() || bTravelingJourney));
        }

        bool ValidControls(const FWorldTransportRead& Read, const FWorldPlayerView& Player,
            const FWorldCurrentInstanceIdentity& Instance, const FString& CurrentLifeId)
        {
            return (!Read.Driving.IsSet() || ValidDriving(Read.Driving.GetValue(), Player, Instance, CurrentLifeId))
                && (!Read.WaterControl.IsSet() || ValidWater(Read.WaterControl.GetValue(), Player, Instance, CurrentLifeId));
        }
    }

    bool ReconcileWorldTransportRead(TSharedPtr<const FWorldPlayerView> Player,
        TSharedPtr<const FWorldCurrentInstanceIdentity> Instance,
        TSharedPtr<const FWorldTransportRead> Previous, FString CurrentLifeId,
        TSharedPtr<const FWorldTransportRead>& Out, FString& OutError)
    {
        using namespace WorldControlsPrivate;
        Out.Reset();
        OutError.Reset();
        if (!Player.IsValid()) return true;

        const auto Next = MakeShared<FWorldTransportRead>();
        Next->UserId = Player->UserId;
        Next->CharacterId = Player->CharacterId;
        Next->GameRoomId = Player->GameRoomId;
        Next->InstanceId = Player->InstanceId;
        Next->VisitRevision = Player->Revision;
        Next->Position = Player->Position;
        if (!Instance.IsValid() || Player->Status != EWorldPlayerStatus::Present || Player->Health <= 0
            || !Same(Player->InstanceId, Instance->Id))
        {
            // Suppress transport for this read; do not erase the received player or
            // its other owners and never turn suppression into walking permission.
            Out = Next;
            return true;
        }

        OutError = TEXT("The accepted transport control changed. Reload the world.");
        if (!SelectField(Player->Driving, Next->Driving) || !SelectField(Player->WaterControl, Next->WaterControl)
            || !ValidControls(*Next, *Player, *Instance, CurrentLifeId)) return false;
        OutError = TEXT("The character has conflicting transport occupancy.");
        if (Conflicting(*Next, *Player)) return false;

        if (Previous.IsValid() && Same(Previous->UserId, Player->UserId)
            && Same(Previous->CharacterId, Player->CharacterId) && Same(Previous->GameRoomId, Player->GameRoomId)
            && Same(Previous->InstanceId, Player->InstanceId) && Previous->VisitRevision == Player->Revision)
        {
            if (Next->Driving.IsSet() && Previous->Driving.IsSet())
            {
                const auto& Incoming = Next->Driving.GetValue();
                const auto& Prior = Previous->Driving.GetValue();
                if (Same(Prior.Id, Incoming.Id) && Same(Prior.Driver.LifeId, Incoming.Driver.LifeId)
                    && (Incoming.Revision < Prior.Revision || Incoming.Sequence < Prior.Sequence
                        || Incoming.AcceptedAtMs < Prior.AcceptedAtMs))
                {
                    Next->Driving = Prior;
                    Next->Position = Previous->Position;
                }
            }
            if (Next->WaterControl.IsSet() && Previous->WaterControl.IsSet())
            {
                const auto& Incoming = Next->WaterControl.GetValue();
                const auto& Prior = Previous->WaterControl.GetValue();
                if (Same(Prior.Id, Incoming.Id) && SameWaterScope(Prior.Scope, Incoming.Scope)
                    && (Incoming.Sequence < Prior.Sequence || Incoming.CraftRevision < Prior.CraftRevision
                        || Incoming.SimulatedAtMs < Prior.SimulatedAtMs))
                {
                    Next->WaterControl = Prior;
                    Next->Position = Previous->Position;
                }
            }
        }

        // Recheck the selected combination: retaining an older control must not reintroduce
        // active-water/driving occupancy or a navigation/life mismatch into the new read.
        OutError = TEXT("The accepted transport control changed. Reload the world.");
        if (!ValidControls(*Next, *Player, *Instance, CurrentLifeId)) return false;
        OutError = TEXT("The character has conflicting transport occupancy.");
        if (Conflicting(*Next, *Player)) return false;
        Out = Next;
        OutError.Reset();
        return true;
    }
}
