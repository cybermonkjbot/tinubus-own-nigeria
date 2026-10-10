#include "BaddieCompanionApartment.h"
#include "BaddieCompanionHomePresentation.h"

bool BaddieCompanion::BuildApartmentPlan(const TArray<FRoomPlan>& SuppliedRooms, FApartmentPlan& Out)
{
    Out = {};
    if (SuppliedRooms.IsEmpty() || SuppliedRooms.Num() > 12) return false;
    FApartmentPlan Next;
    FApartmentEndpoint Living{ EApartmentSpace::LivingRoom, {} };
    TArray<FString> Seen;
    for (const auto& Room : SuppliedRooms)
    {
        if (Room.Id.Len() > 80 || Room.Id.TrimStartAndEnd().IsEmpty()
            || Seen.ContainsByPredicate([&Room](const FString& Id)
                { return Id.Equals(Room.Id, ESearchCase::CaseSensitive); })) return false;
        Seen.Add(Room.Id);
        if (Room.Id.Equals(TEXT("home-living"), ESearchCase::CaseSensitive))
        {
            // Reuse the canonical starter living room regardless of its current display name.
            Next.SavedLivingRoomId = Room.Id;
            Living.RoomId = Room.Id;
        }
        else Next.Connections.Add({ { EApartmentSpace::SavedRoom, Room.Id }, { EApartmentSpace::Hallway, {} } });
    }
    Next.Connections.Add({ { EApartmentSpace::Hallway, {} }, Living });
    Next.Connections.Add({ Living, { EApartmentSpace::ExteriorLanding, {} } });
    Out = MoveTemp(Next);
    return true;
}
