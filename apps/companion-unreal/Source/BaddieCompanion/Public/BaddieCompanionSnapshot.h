#pragma once

#include "CoreMinimal.h"

namespace BaddieCompanion
{
    struct FCharacterSelection
    {
        FString Id;
        FString Name;
        FString BaseId;
        FString Look;
        // Preserve optional appearance fields until their Unreal asset mappings exist.
        FString AppearanceJson;
    };

    struct FRoomSelection
    {
        FString Id;
        FString Name;
        FString EnvironmentId;
        FString LayoutId;
        // Preserve saved furniture, colors and layout data without re-authoring it.
        FString RoomJson;
    };

    struct FSnapshotSelection
    {
        int64 Revision = 0;
        FCharacterSelection Character;
        FRoomSelection ActiveRoom;
        FRoomSelection HomeRoom;
        // Only rooms supplied by the caller's authenticated projection or explicit offline file.
        // A nonempty list does not establish a complete Home inventory or broader access.
        TArray<FRoomSelection> Rooms;
    };

    // This limit belongs to the standalone reader, not the embedded scene protocol.
    inline constexpr int32 MaxSnapshotBytes = 256 * 1024;

    /**
     * Read the selection from the decoded companions:snapshot result, not its
     * Convex transport envelope. Output is unchanged on failure.
     *
     * Checks the fields needed for selection, not furniture placement or ownership.
     * It does not authenticate an account, authorize gameplay or create saved state.
     * A future live transport must supply an authenticated, entitlement-filtered result.
     */
    BADDIECOMPANION_API bool ReadSnapshotSelection(
        const FString& Json, FSnapshotSelection& OutSelection, FString& OutError);
}
