#include "BaddieCompanionResidentState.h"

#include "BaddieCompanionStateJson.h"
#include <cmath>

namespace BaddieCompanion
{
    namespace ResidentStatePrivate
    {
        using namespace StateJsonPrivate;
        // Native resource bounds, not rendering eligibility or provisioning rules.
        constexpr FParseLimits Limits{1048576, 16, 40000};
        constexpr int32 MaxKey = 256, MaxText = 4096, MaxResidents = 11;
        constexpr int32 MaxBindings = 22, MaxCorridorPoints = 256, MaxUnvaluedAssets = 32;

        bool Key(const FValue& Value, FString& Out)
        {
            return String(Value, Out) && !Out.IsEmpty() && Out.Len() <= MaxKey;
        }
        bool Text(const FValue& Value, FString& Out)
        {
            return String(Value, Out) && Out.Len() <= MaxText;
        }
        bool NonnegativeInteger(const FValue& Value, int64& Out) { return Integer(Value, Out); }
        bool SignedInteger(const FValue& Value, int64& Out) { return Integer(Value, Out, -MaxInteger); }
        bool Time(const FValue& Value, double& Out) { return Number(Value, Out) && ValidTime(Out); }
        bool AssetNpc(const FValue& Value, EResidentAssetNpc& Out)
        {
            return EnumValue<EResidentAssetNpc>(Value, Out,
                {{TEXT("mai-suya"), EResidentAssetNpc::MaiSuya}, {TEXT("akara-woman"), EResidentAssetNpc::AkaraWoman},
                 {TEXT("street-tout-shirt"), EResidentAssetNpc::StreetToutShirt}, {TEXT("street-tout-cap"), EResidentAssetNpc::StreetToutCap}});
        }
        bool TravelMode(const FValue& Value, EResidentTravelMode& Out)
        {
            return EnumValue<EResidentTravelMode>(Value, Out,
                {{TEXT("walk"), EResidentTravelMode::Walk}, {TEXT("car"), EResidentTravelMode::Car},
                 {TEXT("okada"), EResidentTravelMode::Okada}, {TEXT("keke"), EResidentTravelMode::Keke}, {TEXT("danfo"), EResidentTravelMode::Danfo}});
        }
        bool RoutineAction(const FValue& Value, EResidentRoutineAction& Out)
        {
            return EnumValue<EResidentRoutineAction>(Value, Out,
                {{TEXT("rest"), EResidentRoutineAction::Rest}, {TEXT("work"), EResidentRoutineAction::Work},
                 {TEXT("socialize"), EResidentRoutineAction::Socialize}});
        }
        bool Clock(const FValue& Value, FResidentClock& Out)
        {
            const FObject O = Object(Value);
            int64 Version = 0;
            return Only(O, {TEXT("version"), TEXT("worldKey"), TEXT("epochKey"), TEXT("epochRealMs"), TEXT("epochGameMinute"), TEXT("realMsPerGameMinute")})
                && Integer(Field(O, TEXT("version")), Version) && Version == 1
                && Key(Field(O, TEXT("worldKey")), Out.WorldKey) && Key(Field(O, TEXT("epochKey")), Out.EpochKey)
                && Integer(Field(O, TEXT("epochRealMs")), Out.EpochRealMs)
                && Integer(Field(O, TEXT("epochGameMinute")), Out.EpochGameMinute)
                && Integer(Field(O, TEXT("realMsPerGameMinute")), Out.RealMsPerGameMinute, 1);
        }
        bool Identity(const FValue& Value, FResidentIdentity& Out)
        {
            const FObject O = Object(Value), Appearance = Object(Field(O, TEXT("appearance")));
            int64 RosterVersion = 0;
            if (!Only(O, {TEXT("actorKind"), TEXT("id"), TEXT("worldKey"), TEXT("actorKey"), TEXT("rosterVersion"),
                TEXT("name"), TEXT("occupation"), TEXT("role"), TEXT("personality"), TEXT("appearance")})
                || !EnumValue<EResidentActorKind>(Field(O, TEXT("actorKind")), Out.ActorKind, {{TEXT("npc"), EResidentActorKind::Npc}})
                || !Key(Field(O, TEXT("id")), Out.Id) || !Key(Field(O, TEXT("worldKey")), Out.WorldKey)
                || !Integer(Field(O, TEXT("rosterVersion")), RosterVersion) || RosterVersion != 1
                || !Text(Field(O, TEXT("name")), Out.Name)
                || !EnumValue<EResidentKey>(Field(O, TEXT("actorKey")), Out.ActorKey,
                    {{TEXT("mai-suya"), EResidentKey::MaiSuya}, {TEXT("akara-woman"), EResidentKey::AkaraWoman},
                     {TEXT("courier"), EResidentKey::Courier}, {TEXT("danfo-driver"), EResidentKey::DanfoDriver},
                     {TEXT("medic"), EResidentKey::Medic}, {TEXT("tailor"), EResidentKey::Tailor},
                     {TEXT("street-akara-vendor"), EResidentKey::StreetAkaraVendor}, {TEXT("area-tout"), EResidentKey::AreaTout},
                     {TEXT("traffic-hawker"), EResidentKey::TrafficHawker}, {TEXT("pos-agent"), EResidentKey::PosAgent},
                     {TEXT("danfo-conductor"), EResidentKey::DanfoConductor}})
                || !EnumValue<EResidentOccupation>(Field(O, TEXT("occupation")), Out.Occupation,
                    {{TEXT("suya-cook"), EResidentOccupation::SuyaCook}, {TEXT("akara-cook"), EResidentOccupation::AkaraCook},
                     {TEXT("courier"), EResidentOccupation::Courier}, {TEXT("danfo-driver"), EResidentOccupation::DanfoDriver},
                     {TEXT("medic"), EResidentOccupation::Medic}, {TEXT("tailor"), EResidentOccupation::Tailor},
                     {TEXT("traffic-hawker"), EResidentOccupation::TrafficHawker}, {TEXT("pos-operator"), EResidentOccupation::PosOperator},
                     {TEXT("conductor"), EResidentOccupation::Conductor}, {TEXT("union-collector"), EResidentOccupation::UnionCollector}})
                || !EnumValue<EResidentRole>(Field(O, TEXT("role")), Out.Role,
                    {{TEXT("vendor"), EResidentRole::Vendor}, {TEXT("medic"), EResidentRole::Medic}, {TEXT("driver"), EResidentRole::Driver},
                     {TEXT("police"), EResidentRole::Police}, {TEXT("mission-contact"), EResidentRole::MissionContact},
                     {TEXT("host"), EResidentRole::Host}, {TEXT("resident"), EResidentRole::Resident}})
                || !Only(Appearance, {TEXT("seed"), TEXT("assetNpcId")})
                || !Integer(Field(Appearance, TEXT("seed")), Out.Appearance.Seed)
                || !Nullable(Field(Appearance, TEXT("assetNpcId")), Out.Appearance.AssetNpcId, AssetNpc)) return false;
            const FValue Personality = Field(O, TEXT("personality"));
            if (!Personality.IsValid() || Personality->Type != EJson::Array || Personality->AsArray().Num() != 2) return false;
            for (const auto& Trait : Personality->AsArray())
            {
                EResidentTrait Next;
                if (!EnumValue<EResidentTrait>(Trait, Next,
                    {{TEXT("sociable"), EResidentTrait::Sociable}, {TEXT("reserved"), EResidentTrait::Reserved},
                     {TEXT("patient"), EResidentTrait::Patient}, {TEXT("brisk"), EResidentTrait::Brisk},
                     {TEXT("practical"), EResidentTrait::Practical}, {TEXT("cautious"), EResidentTrait::Cautious}})) return false;
                Out.Personality.Add(Next);
            }
            return true;
        }
        bool PlaceFields(const FObject& O, FResidentPlace& Out)
        {
            return Key(Field(O, TEXT("placeId")), Out.PlaceId) && Key(Field(O, TEXT("instanceId")), Out.InstanceId);
        }
        bool Place(const FValue& Value, FResidentPlace& Out)
        {
            const FObject O = Object(Value);
            return Only(O, {TEXT("placeId"), TEXT("instanceId")}) && PlaceFields(O, Out);
        }
        bool Anchor(const FValue& Value, FResidentAnchor& Out)
        {
            const FObject O = Object(Value), Pose = Object(Field(O, TEXT("pose")));
            if (!Only(O, {TEXT("placeId"), TEXT("instanceId"), TEXT("pose")}) || !PlaceFields(O, Out.Place)
                || !Only(Pose, {TEXT("point"), TEXT("yaw")}) || !Number(Pose, TEXT("yaw"), Out.Pose.YawRadians)) return false;
            const FValue Point = Field(Pose, TEXT("point"));
            return Point.IsValid() && Point->Type == EJson::Array && Point->AsArray().Num() == 3
                && Number(Point->AsArray()[0], Out.Pose.X) && Number(Point->AsArray()[1], Out.Pose.Y)
                && Number(Point->AsArray()[2], Out.Pose.Z);
        }
        bool Activity(const FValue& Value, FResidentActivity& Out)
        {
            const FObject O = Object(Value);
            if (!EnumValue<EResidentActivityKind>(Field(O, TEXT("kind")), Out.Kind,
                {{TEXT("at-place"), EResidentActivityKind::AtPlace}, {TEXT("traveling"), EResidentActivityKind::Traveling}})) return false;
            if (Out.Kind == EResidentActivityKind::AtPlace)
            {
                FResidentAtPlaceActivity Next;
                if (!Only(O, {TEXT("kind"), TEXT("location"), TEXT("action"), TEXT("interactionId"), TEXT("sinceGameMinute")})
                    || !Anchor(Field(O, TEXT("location")), Next.Location)
                    || !EnumValue<EResidentAction>(Field(O, TEXT("action")), Next.Action,
                        {{TEXT("rest"), EResidentAction::Rest}, {TEXT("work"), EResidentAction::Work}, {TEXT("eat"), EResidentAction::Eat},
                         {TEXT("socialize"), EResidentAction::Socialize}, {TEXT("recover"), EResidentAction::Recover}})
                    || !Nullable(Field(O, TEXT("interactionId")), Next.InteractionId, Key)
                    || !Integer(Field(O, TEXT("sinceGameMinute")), Next.SinceGameMinute)) return false;
                Out.AtPlace = MoveTemp(Next);
            }
            else
            {
                FResidentTravelActivity Next;
                if (!Only(O, {TEXT("kind"), TEXT("journeyId"), TEXT("from"), TEXT("to"), TEXT("mode"), TEXT("vehicleId"), TEXT("sinceGameMinute")})
                    || !Key(Field(O, TEXT("journeyId")), Next.JourneyId) || !Place(Field(O, TEXT("from")), Next.From)
                    || !Place(Field(O, TEXT("to")), Next.To) || !TravelMode(Field(O, TEXT("mode")), Next.Mode)
                    || !Nullable(Field(O, TEXT("vehicleId")), Next.VehicleId, Key)
                    || !Integer(Field(O, TEXT("sinceGameMinute")), Next.SinceGameMinute)) return false;
                Out.Traveling = MoveTemp(Next);
            }
            return true;
        }
        bool Worth(const FValue& Value, FResidentWorth& Out)
        {
            const FObject O = Object(Value);
            if (!Only(O, {TEXT("unit"), TEXT("unitKey"), TEXT("knownBankedValue"), TEXT("knownCarriedValue"),
                TEXT("outstandingDebt"), TEXT("netWorth"), TEXT("carriedValue"), TEXT("unvaluedAssetIds")})
                || !EnumValue<EResidentValueUnit>(Field(O, TEXT("unit")), Out.Unit, {{TEXT("game-value-unit"), EResidentValueUnit::GameValueUnit}})
                || !Key(Field(O, TEXT("unitKey")), Out.UnitKey)
                || !Integer(Field(O, TEXT("knownBankedValue")), Out.KnownBankedValue)
                || !Integer(Field(O, TEXT("knownCarriedValue")), Out.KnownCarriedValue)
                || !Integer(Field(O, TEXT("outstandingDebt")), Out.OutstandingDebt)
                || !Nullable(Field(O, TEXT("netWorth")), Out.NetWorth, SignedInteger)
                || !Nullable(Field(O, TEXT("carriedValue")), Out.CarriedValue, NonnegativeInteger)) return false;
            const FValue Ids = Field(O, TEXT("unvaluedAssetIds"));
            if (!Ids.IsValid() || Ids->Type != EJson::Array || Ids->AsArray().Num() > MaxUnvaluedAssets) return false;
            for (const auto& Id : Ids->AsArray())
            {
                FString Next;
                if (!Key(Id, Next)) return false;
                Out.UnvaluedAssetIds.Add(MoveTemp(Next));
            }
            return true;
        }
        bool Movement(const FValue& Value, FMovementRoute& Out)
        {
            FString Error;
            // Reuse the existing decoder, including pace/initialSpeed omission and 32-point bound.
            // Its fixed internal diagnostic is not exposed as population content.
            return DecodeMovementRoute(Object(Value), Out, Error);
        }
        bool Corridor(const FValue& Value, FResidentCorridorRoute& Out)
        {
            const FObject O = Object(Value);
            if (!Only(O, {TEXT("id"), TEXT("version"), TEXT("placement"), TEXT("points"), TEXT("lengthMetres")})
                || !Key(Field(O, TEXT("id")), Out.Id)
                || !EnumValue<EResidentRouteVersion>(Field(O, TEXT("version")), Out.Version, {{TEXT("lagos-corridors-v1"), EResidentRouteVersion::LagosCorridorsV1}})
                || !EnumValue<EResidentRoutePlacement>(Field(O, TEXT("placement")), Out.Placement, {{TEXT("proposed"), EResidentRoutePlacement::Proposed}})
                || !Number(O, TEXT("lengthMetres"), Out.LengthMetres) || Out.LengthMetres < 0) return false;
            const FValue Points = Field(O, TEXT("points"));
            if (!Points.IsValid() || Points->Type != EJson::Array || Points->AsArray().Num() > MaxCorridorPoints) return false;
            for (const auto& Point : Points->AsArray())
            {
                const FObject P = Object(Point);
                FResidentGeoPoint Next;
                if (!Only(P, {TEXT("latitude"), TEXT("longitude")}) || !Number(P, TEXT("latitude"), Next.Latitude)
                    || !Number(P, TEXT("longitude"), Next.Longitude) || std::abs(Next.Latitude) > 90 || std::abs(Next.Longitude) > 180) return false;
                Out.Points.Add(Next);
            }
            return true;
        }
        bool VehicleAsset(const FValue& Value, FResidentVehicleAsset& Out)
        {
            const FObject O = Object(Value);
            return Only(O, {TEXT("key"), TEXT("sourceYaw")}) && Key(Field(O, TEXT("key")), Out.Key)
                && Number(O, TEXT("sourceYaw"), Out.SourceYawRadians);
        }
        bool Transport(const FValue& Value, FResidentJourneyTransport& Out)
        {
            const FObject O = Object(Value), Model = Object(Field(O, TEXT("model")));
            return Only(O, {TEXT("vehicleId"), TEXT("ownerResidentId"), TEXT("riderResidentId"), TEXT("model"), TEXT("asset")})
                && Key(Field(O, TEXT("vehicleId")), Out.VehicleId) && Key(Field(O, TEXT("ownerResidentId")), Out.OwnerResidentId)
                && Nullable(Field(O, TEXT("riderResidentId")), Out.RiderResidentId, Key)
                && Nullable(Field(O, TEXT("asset")), Out.Asset, VehicleAsset)
                && Only(Model, {TEXT("catalogueId"), TEXT("modelId"), TEXT("catalogueRevision"), TEXT("kind"), TEXT("tankCapacityUnits"), TEXT("metresPerFuelUnit")})
                && Key(Field(Model, TEXT("catalogueId")), Out.Model.CatalogueId) && Key(Field(Model, TEXT("modelId")), Out.Model.ModelId)
                && Integer(Field(Model, TEXT("catalogueRevision")), Out.Model.CatalogueRevision, 1)
                && EnumValue<EResidentVehicleKind>(Field(Model, TEXT("kind")), Out.Model.Kind,
                    {{TEXT("car"), EResidentVehicleKind::Car}, {TEXT("okada"), EResidentVehicleKind::Okada},
                     {TEXT("keke"), EResidentVehicleKind::Keke}, {TEXT("danfo"), EResidentVehicleKind::Danfo}})
                && Integer(Field(Model, TEXT("tankCapacityUnits")), Out.Model.TankCapacityUnits, 1) && Out.Model.TankCapacityUnits <= 1000000
                && Integer(Field(Model, TEXT("metresPerFuelUnit")), Out.Model.MetresPerFuelUnit, 1) && Out.Model.MetresPerFuelUnit <= 1000000;
        }
        bool Journey(const FValue& Value, FResidentJourney& Out)
        {
            const FObject O = Object(Value);
            if (!Only(O, {TEXT("id"), TEXT("from"), TEXT("to"), TEXT("mode"), TEXT("vehicleId"), TEXT("transport"), TEXT("timeBasis"),
                TEXT("departure"), TEXT("route"), TEXT("routeStartedAt"), TEXT("routeEndsAt"), TEXT("arrival"), TEXT("arrivesAt"), TEXT("arrivalAction")})
                || !Key(Field(O, TEXT("id")), Out.Id) || !Anchor(Field(O, TEXT("from")), Out.From) || !Anchor(Field(O, TEXT("to")), Out.To)
                || !TravelMode(Field(O, TEXT("mode")), Out.Mode) || !Nullable(Field(O, TEXT("vehicleId")), Out.VehicleId, Key)
                || !EnumValue<EResidentJourneyTimeBasis>(Field(O, TEXT("timeBasis")), Out.TimeBasis, {{TEXT("server-ms"), EResidentJourneyTimeBasis::ServerMs}})
                || !Movement(Field(O, TEXT("departure")), Out.Departure) || !Movement(Field(O, TEXT("arrival")), Out.Arrival)
                || !Corridor(Field(O, TEXT("route")), Out.Route) || !Time(Field(O, TEXT("routeStartedAt")), Out.RouteStartedAtRealMs)
                || !Time(Field(O, TEXT("routeEndsAt")), Out.RouteEndsAtRealMs) || !Time(Field(O, TEXT("arrivesAt")), Out.ArrivesAtRealMs)
                || !RoutineAction(Field(O, TEXT("arrivalAction")), Out.ArrivalAction)) return false;
            const FValue TransportValue = Field(O, TEXT("transport"));
            if (TransportValue.IsValid())
            {
                FResidentJourneyTransport Next;
                if (!Transport(TransportValue, Next)) return false;
                Out.Transport = MoveTemp(Next);
            }
            return true;
        }
        bool LocalMove(const FValue& Value, FResidentLocalMove& Out)
        {
            const FObject O = Object(Value);
            return Only(O, {TEXT("movement"), TEXT("to"), TEXT("action")}) && Movement(Field(O, TEXT("movement")), Out.Movement)
                && Anchor(Field(O, TEXT("to")), Out.To) && RoutineAction(Field(O, TEXT("action")), Out.Action);
        }
        bool ServiceActor(const FValue& Value, FResidentServiceActor& Out)
        {
            const FObject O = Object(Value);
            return Only(O, {TEXT("placeId"), TEXT("npcId")}) && Key(Field(O, TEXT("placeId")), Out.PlaceId) && Key(Field(O, TEXT("npcId")), Out.NpcId);
        }
        bool RenderAsset(const FValue& Value, FResidentRenderAsset& Out)
        {
            const FObject O = Object(Value);
            if (!Only(O, {TEXT("key"), TEXT("npcId"), TEXT("sourceYaw")}) || !Key(Field(O, TEXT("key")), Out.Key)
                || !AssetNpc(Field(O, TEXT("npcId")), Out.NpcId)) return false;
            const FValue Yaw = Field(O, TEXT("sourceYaw"));
            if (Yaw.IsValid())
            {
                double Next = 0;
                if (!Number(Yaw, Next)) return false;
                Out.SourceYawRadians = Next;
            }
            return true;
        }
        bool Resident(const FValue& Value, FResidentView& Out)
        {
            const FObject O = Object(Value), Health = Object(Field(O, TEXT("health"))), Needs = Object(Field(O, TEXT("needs")));
            if (!Only(O, {TEXT("identity"), TEXT("revision"), TEXT("clockEpochKey"), TEXT("updatedAtGameMinute"), TEXT("health"),
                TEXT("needs"), TEXT("worth"), TEXT("activity"), TEXT("journey"), TEXT("localMove"), TEXT("serviceActor"), TEXT("renderAsset")})
                || !Identity(Field(O, TEXT("identity")), Out.Identity) || !Integer(Field(O, TEXT("revision")), Out.Revision)
                || !Key(Field(O, TEXT("clockEpochKey")), Out.ClockEpochKey) || !Integer(Field(O, TEXT("updatedAtGameMinute")), Out.UpdatedAtGameMinute)
                || !Only(Health, {TEXT("health"), TEXT("maxHealth")}) || !Number(Health, TEXT("health"), Out.Health.Health)
                || !Number(Health, TEXT("maxHealth"), Out.Health.MaxHealth)
                || !Only(Needs, {TEXT("hunger"), TEXT("energy"), TEXT("cleanliness"), TEXT("enjoyment")})
                || !Number(Needs, TEXT("hunger"), Out.Needs.Hunger) || !Number(Needs, TEXT("energy"), Out.Needs.Energy)
                || !Number(Needs, TEXT("cleanliness"), Out.Needs.Cleanliness) || !Number(Needs, TEXT("enjoyment"), Out.Needs.Enjoyment)
                || !Worth(Field(O, TEXT("worth")), Out.Worth) || !Activity(Field(O, TEXT("activity")), Out.Activity)
                || !Nullable(Field(O, TEXT("journey")), Out.Journey, Journey)
                || !Nullable(Field(O, TEXT("localMove")), Out.LocalMove, LocalMove)
                || !Nullable(Field(O, TEXT("serviceActor")), Out.ServiceActor, ServiceActor)) return false;
            const FValue Asset = Field(O, TEXT("renderAsset"));
            if (Asset.IsValid())
            {
                if (!Nullable(Asset, Out.RenderAsset.Value, RenderAsset)) return false;
                Out.RenderAsset.Presence = Out.RenderAsset.Value.IsSet() ? EResidentFieldPresence::Value : EResidentFieldPresence::Null;
            }
            return true;
        }
        bool Population(const FObject& O, FResidentWorldView& Out)
        {
            if (!Only(O, {TEXT("worldKey"), TEXT("clock"), TEXT("nextAdvanceAtRealMs"), TEXT("residents"), TEXT("placeBindings")})
                || !Key(Field(O, TEXT("worldKey")), Out.WorldKey) || !Clock(Field(O, TEXT("clock")), Out.Clock)
                || !Same(Out.WorldKey, Out.Clock.WorldKey)
                || !Nullable(Field(O, TEXT("nextAdvanceAtRealMs")), Out.NextAdvanceAtRealMs, NonnegativeInteger)) return false;
            const FValue Residents = Field(O, TEXT("residents"));
            if (!Residents.IsValid() || Residents->Type != EJson::Array || Residents->AsArray().Num() > MaxResidents) return false;
            for (const auto& Value : Residents->AsArray())
            {
                FResidentView Next;
                if (!Resident(Value, Next)) return false;
                Out.Residents.Add(MoveTemp(Next));
            }
            const FValue Bindings = Field(O, TEXT("placeBindings"));
            if (Bindings.IsValid())
            {
                if (Bindings->Type != EJson::Array || Bindings->AsArray().Num() > MaxBindings) return false;
                TArray<FResidentPlaceBinding> NextBindings;
                for (const auto& Value : Bindings->AsArray())
                {
                    const FObject B = Object(Value);
                    FResidentPlaceBinding Next;
                    if (!Only(B, {TEXT("placeId"), TEXT("residentInstanceId"), TEXT("gameInstanceId"), TEXT("layoutId")})
                        || !Key(Field(B, TEXT("placeId")), Next.PlaceId) || !Key(Field(B, TEXT("residentInstanceId")), Next.ResidentInstanceId)
                        || !Key(Field(B, TEXT("gameInstanceId")), Next.GameInstanceId) || !Key(Field(B, TEXT("layoutId")), Next.LayoutId)) return false;
                    NextBindings.Add(MoveTemp(Next));
                }
                Out.PlaceBindings = MoveTemp(NextBindings);
            }
            return true;
        }
    }

    bool DecodeResidentWorld(const FString& Json, TSharedPtr<const FResidentWorldView>& Out, FString& OutError)
    {
        using namespace ResidentStatePrivate;
        // Preserve input/output alias safety without copying an oversized document.
        const FString Input = Json.Len() <= Limits.Characters ? Json : FString();
        Out.Reset(); OutError = TEXT("Resident population data is invalid.");
        FValue Null;
        if (ParseNull(Input, Null, Limits)) { OutError.Reset(); return true; }
        FObject Object;
        FResidentWorldView Next;
        if (!Parse(Input, Object, Limits) || !Population(Object, Next)) return false;
        Out = MakeShared<FResidentWorldView>(MoveTemp(Next)); OutError.Reset();
        return true;
    }
}
