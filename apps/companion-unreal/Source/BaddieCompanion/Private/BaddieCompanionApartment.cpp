#include "BaddieCompanionApartment.h"
#include "BaddieCompanionHomePresentation.h"
#include "BaddieCompanionHomeMappingInternal.h"

bool BaddieCompanion::BuildApartmentPlan(const TArray<FRoomPlan>& SuppliedRooms, FApartmentPlan& Out)
{
    Out = {};
    HomeMapping::FLegacyMappingWork Work;
    HomeMapping::FApartmentMapping Mapping;
    Mapping.Begin(SuppliedRooms);
    const bool bComplete = Mapping.Advance(Work, Out) == HomeMapping::EMappingStep::Complete;
    Mapping.Retire(Work);
    return bComplete;
}

void BaddieCompanion::HomeMapping::FApartmentMapping::Begin(const TArray<FRoomPlan>& Rooms)
{
    Source = &Rooms;
    Phase = RoomIndex = CompareIndex = RetireIndex = RetireField = 0;
}

BaddieCompanion::HomeMapping::EMappingStep BaddieCompanion::HomeMapping::FApartmentMapping::Advance(
    FMappingWork& Work, FApartmentPlan& Out)
{
    if (!Source || Source->IsEmpty() || Source->Num() > 12) return EMappingStep::Invalid;
    while (Phase < 7)
    {
        if (Phase == 0)
        {
            const auto Admission = Work.Admit(64, (Source->Num() + 2) * sizeof(FApartmentConnection));
            if (Admission != EMappingStep::Complete) return Admission;
            Next.Connections.Reserve(Source->Num() + 2);
            Phase = 1;
        }
        if (RoomIndex < static_cast<uint32>(Source->Num()))
        {
            const auto& Room = (*Source)[RoomIndex];
            if (Phase == 1)
            {
                const auto Admission = Work.Admit(32 + 4 * static_cast<uint64>(Room.Id.Len()));
                if (Admission != EMappingStep::Complete) return Admission;
                if (Room.Id.Len() > 80 || Room.Id.TrimStartAndEnd().IsEmpty()) return EMappingStep::Invalid;
                Phase = 2;
            }
            if (Phase == 2)
            {
                while (CompareIndex < RoomIndex)
                {
                    const auto Admission = Work.Admit(32 + 2 * static_cast<uint64>(Room.Id.Len()));
                    if (Admission != EMappingStep::Complete) return Admission;
                    if (Same((*Source)[CompareIndex].Id, Room.Id)) return EMappingStep::Invalid;
                    ++CompareIndex;
                }
                Phase = Same(Room.Id, TEXT("home-living")) ? 3 : 4;
            }
            if (Phase == 3)
            {
                const auto Admission = Work.Admit(32 + 2 * static_cast<uint64>(Room.Id.Len()),
                    (Room.Id.Len() + 1) * sizeof(TCHAR));
                if (Admission != EMappingStep::Complete) return Admission;
                Next.SavedLivingRoomId = Room.Id;
                Phase = 4;
            }
            if (Phase == 4)
            {
                const auto Admission = Work.Admit(64 + 2 * static_cast<uint64>(Room.Id.Len()),
                    (Room.Id.Len() + 1) * sizeof(TCHAR));
                if (Admission != EMappingStep::Complete) return Admission;
                if (Same(Room.Id, TEXT("home-living"))) Living.RoomId = Room.Id;
                else Next.Connections.Add({ { EApartmentSpace::SavedRoom, Room.Id }, { EApartmentSpace::Hallway, {} } });
                ++RoomIndex; CompareIndex = 0; Phase = 1;
            }
            continue;
        }
        if (Phase == 1) Phase = 5;
        if (Phase == 5 || Phase == 6)
        {
            const auto Admission = Work.Admit(64 + 2 * static_cast<uint64>(Living.RoomId.Len()),
                (Living.RoomId.Len() + 1) * sizeof(TCHAR));
            if (Admission != EMappingStep::Complete) return Admission;
            if (Phase == 5) Next.Connections.Add({ { EApartmentSpace::Hallway, {} }, Living });
            else Next.Connections.Add({ Living, { EApartmentSpace::ExteriorLanding, {} } });
            ++Phase;
        }
    }
    const auto Admission = Work.Admit(sizeof(FApartmentPlan));
    if (Admission != EMappingStep::Complete) return Admission;
    Out = MoveTemp(Next); Source = nullptr;
    return EMappingStep::Complete;
}

BaddieCompanion::HomeMapping::EMappingStep BaddieCompanion::HomeMapping::FApartmentMapping::Retire(
    FMappingWork& Work)
{
    while (RetireIndex < static_cast<uint32>(Next.Connections.Num()))
    {
        auto& Connection = Next.Connections[RetireIndex];
        const auto Result = Work.ReleaseText(RetireField == 0 ? Connection.From.RoomId : Connection.To.RoomId);
        if (Result != EMappingStep::Complete) return Result;
        if (++RetireField == 2) { ++RetireIndex; RetireField = 0; }
    }
    if (Next.SavedLivingRoomId.IsSet())
    {
        const auto Result = Work.ReleaseText(Next.SavedLivingRoomId.GetValue());
        if (Result != EMappingStep::Complete) return Result;
    }
    const auto Result = Work.ReleaseText(Living.RoomId);
    if (Result != EMappingStep::Complete) return Result;
    const auto Admission = Work.Admit(128 + static_cast<uint64>(Next.Connections.Num()));
    if (Admission != EMappingStep::Complete) return Admission;
    Next = {}; Living = { EApartmentSpace::LivingRoom, {} }; Source = nullptr;
    return EMappingStep::Complete;
}
