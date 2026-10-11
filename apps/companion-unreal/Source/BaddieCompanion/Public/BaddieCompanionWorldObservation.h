#pragma once

#include "CoreMinimal.h"
#include "BaddieCompanionWorldPlayer.h"
#include "BaddieCompanionWorldClock.h"

namespace BaddieCompanion
{
    class FWorldSession;
    struct FSnapshotSelection;
    namespace WorldObservationPrivate { struct FStorage; struct FAccess; }

    enum class EWorldRoot : uint8
    {
        Player, Peers, Instance, Racing, Transit, DrivingCatalog, Road, Factions,
        Hawkers, Water, TransitAvailability, StreetScope, StreetTraffic,
        CheckpointStatus, FriendResidences, Health, Selection, Binding, Result, Presentation
    };
    enum class EWorldValueKind : uint8 { Object, Array, String, StringBlock, Null, Boolean, Number };
    enum class EWorldReadStatus : uint8
    {
        Unavailable, Preparing, Capturing, Receiving, Binding, Accepted,
        ResourceUnavailable, InvalidResponse, Expired, ContentUnavailable
    };
    struct FWorldNodeView
    {
        EWorldValueKind Kind = EWorldValueKind::Null;
        uint64 Id = 0, Parent = 0, FirstChild = 0, NextSibling = 0, Count = 0;
        uint64 Slot = 0, NumberBits = 0;
        uint8 Role = 0;
    };
    struct FWorldRootView
    {
        EWorldFieldPresence Presence = EWorldFieldPresence::Missing;
        bool bJsonDocument = false;
        uint64 Nodes = 0, StringUnits = 0;
    };
    struct FWorldStringCursor
    {
    private:
        friend class FWorldObservation;
        uint64 Generation = 0, Node = 0, Offset = 0;
        uint8 Root = 0;
        const void* Block = nullptr;
    };

    // Revocable read handle. It owns no tree and exposes no authority or mutable state.
    // Node IDs are root-local. Output copies are bounded; no page buffers escape.
    class BADDIECOMPANION_API FWorldObservation
    {
    public:
        bool IsCurrent() const;
        uint64 Generation() const;
        bool ReadRoot(EWorldRoot Root, FWorldRootView& Out) const;
        bool ReadNode(EWorldRoot Root, uint64 Id, FWorldNodeView& Out) const;
        // A fresh cursor begins a string. At most 256 exact units per call.
        bool ReadString(EWorldRoot Root, uint64 Id, FWorldStringCursor& Cursor,
            uint16 (&Out)[256], int32& OutCount) const;
        bool CopyPlayer(TSharedPtr<const FWorldPlayerView>& Out) const;
        bool CopyInstance(TSharedPtr<const FWorldCurrentInstanceIdentity>& Out) const;
        bool CopySelection(TSharedPtr<const FSnapshotSelection>& Out) const;
        bool CopyTime(FWorldPresentationTime& Out) const;
    private:
        friend class FWorldSession;
        TWeakPtr<WorldObservationPrivate::FAccess> Access;
    };

    struct FWorldSessionView
    {
        EWorldReadStatus Status = EWorldReadStatus::Unavailable;
        uint64 Generation = 0;
        FWorldObservation Observation;
        // These are owner-derived observations, never caller-set admission inputs.
        bool bHasObservation = false;
        bool bWalkingAvailable = false;
    };
}
