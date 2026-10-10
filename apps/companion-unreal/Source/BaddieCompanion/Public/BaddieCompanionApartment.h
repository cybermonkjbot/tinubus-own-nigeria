#pragma once

#include "CoreMinimal.h"

namespace BaddieCompanion
{
    struct FRoomPlan;

    enum class EApartmentSpace : uint8 { SavedRoom, Hallway, LivingRoom, ExteriorLanding };
    struct FApartmentEndpoint
    {
        EApartmentSpace Space = EApartmentSpace::SavedRoom;
        // SavedRoom, or LivingRoom when home-living is supplied. Never an asset or level path.
        FString RoomId;
    };
    struct FApartmentConnection
    {
        FApartmentEndpoint From, To;
        // Intended two-way walking connection, not a teleport, route or admission grant.
        bool bNativeDoorwayBound = false;
    };
    struct FLivingRoomDaylightPlan
    {
        bool bLargeExteriorWindows = true;
        bool bDaylightAtHallwayEntrance = true;
        bool bExteriorViewFromSeating = true;
        // Native window geometry, sky/sun lighting and exposure still need authored bindings.
        bool bNativeLightingBound = false;
    };
    struct FApartmentPlan
    {
        // One private apartment for this supplied Home, never a corridor between different accounts.
        TArray<FApartmentConnection> Connections;
        // Missing from a narrow projection does not mean the user has no saved living room.
        TOptional<FString> SavedLivingRoomId;
        FLivingRoomDaylightPlan LivingRoomDaylight;
        // The existing companion_home_read projection contains only the pinned Home room.
        // Neither the number of supplied rooms nor this topology proves complete inventory.
        bool bCompleteHomeInventoryVerified = false;
        bool bNativeCommonSpacesBound = false;
        bool bExteriorAdmissionVerified = false;
        bool IsWalkThroughReady() const { return false; }
    };

    // Baddie connectivity only. UE levels, doors, navigation and lighting own the physical implementation.
    // Invalid or duplicate room IDs clear Out. No saved room, content or authority is created here.
    BADDIECOMPANION_API bool BuildApartmentPlan(const TArray<FRoomPlan>& SuppliedRooms, FApartmentPlan& Out);
}
