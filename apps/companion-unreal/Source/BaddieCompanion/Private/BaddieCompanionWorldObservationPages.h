#pragma once

#include "BaddieCompanionWorldObservation.h"
#include "Dom/JsonObject.h"
#include "Templates/PimplPtr.h"

namespace BaddieCompanion::WorldReadPrivate { struct FPlayerResult; }

namespace BaddieCompanion::WorldObservationPrivate
{
    constexpr uint64 RetainedLimit = 67108864, CandidateWorkLimit = 67108864;
    constexpr uint32 TickWorkLimit = 32768, PageAllowance = 2097152;
    constexpr int32 BodyLimit = 32768, RecordBytesLimit = 28672, RootLimit = 20;
    // The pinned headers do not bound allocator slack, JSON DOM or crypto Pimpl
    // storage. Reject before candidate allocation until those bounds are qualified.
    constexpr bool bNativeAllocationAccountingQualified = false;
    constexpr const TCHAR* ProtocolGeneration = TEXT("8958fe9754c82aa72cf0a987d6ae742327667ad882eb086341b954a2f979351a");
    enum class EStep : uint8 { Pending, Complete, Invalid, ResourceUnavailable };

    struct FBudget
    {
        uint64 Retained = 0, CandidateWork = 0;
        uint32 TickRemaining = TickWorkLimit;
        uint64 Frame = MAX_uint64;
        bool bExhausted = false, bPageReserved = false, bDecodedThisTick = false;
        void BeginTick();
        bool Spend(uint64 Units, bool bCandidate = true);
        bool Reserve(uint64 Bytes);
        void Release(uint64 Bytes);
        bool ReservePage();
        void ReleasePage();
    };
    struct FDirectoryEntry
    {
        uint8 Slot = 0, Presence = 0, Encoding = 0;
        FString Alias, Digest, Tail;
        uint64 Pages = 0, Records = 0, Nodes = 0, Members = 0, ArraySlots = 0, Units = 0, PageBytes = 0;
    };
    struct FManifest
    {
        FString Capture, Kind, Digest;
        int64 CapturedAt = 0, PermissionDeadline = 0;
        TOptional<int64> PublishBy;
        FDirectoryEntry Entries[RootLimit];
        int32 Count = 0;
    };
    struct FStringBlock
    {
        uint16 Units[256]{};
        uint16 Used = 0;
        FStringBlock* Next = nullptr;
    };
    struct FNode : FWorldNodeView
    {
        uint64 LastChild = 0, Completed = 0;
        uint8 NextRole = 0;
        FStringBlock* Text = nullptr;
        FStringBlock* Tail = nullptr;
    };
    // A node page has no recursively owning fields. The fixed index covers more
    // nodes than the retained budget can hold and never rehashes or resizes.
    struct FNodePage { FNode Nodes[64]; };
    struct FRoot
    {
        static constexpr uint64 IndexCount = 16384;
        FNodePage* Index[IndexCount];
        uint64 InitializedIndex = 0;
        FDirectoryEntry Directory;
        uint64 Nodes = 0, Records = 0, Members = 0, ArraySlots = 0, Units = 0, Pages = 0, PageBytes = 0;
        uint64 Parent = 0, OpenString = 0, StringOffset = 0;
        FString RecordHash, PageHash;
        bool bClosed = false;
        uint64 Owners = 1, RetireNode = 1;
        FNode* Get(uint64 Id) const;
    };
    struct FStorage
    {
        explicit FStorage(TSharedRef<FBudget> InBudget);
        ~FStorage();
        FStorage(const FStorage&) = delete;
        FStorage& operator=(const FStorage&) = delete;
        TSharedRef<FBudget> Budget;
        FManifest Manifest;
        FRoot* Roots[RootLimit]{};
        uint8 RetireRoot = 0;
        bool bComplete = false, bRetiring = false;
        uint64 ChargedBytes = 0;
        WorldReadPrivate::FPlayerResult* Players = nullptr;
        FRoot* RacingOwner = nullptr;
        uint64 RacingNode = 0, RaceInstanceNode = 0;
        int64 RaceTick = 0;
        uint64 SemanticGeneration = 0;
        int64 PlayerRevision = -1, MoveSequence = -1, HealthRevision = -1, SavedRevision = -1;
        bool bSemanticsComplete = false;
        static EStep ReleaseRoot(FRoot*& Root, FBudget& Budget);
        EStep Retire();
        FNode* Get(uint8 Root, uint64 Id) const;
    };
    struct FAccess
    {
        TWeakPtr<FStorage> Storage;
        TSharedPtr<const FWorldPlayerView> Player;
        TSharedPtr<const FWorldCurrentInstanceIdentity> Instance;
        TSharedPtr<const FSnapshotSelection> Selection;
        FWorldPresentationClock Clock;
        FString GameRoomId;
        uint64 Generation = 0;
        double Deadline = 0;
        bool bReadable = false;
        bool Current() const;
        void Revoke();
    };
    void RetireDetached(TSharedPtr<FStorage> Storage);

    // One bounded decoded body at a time. Credentials and actual HTTP stay outside.
    // Whole-root JSON reconstruction is deliberately absent from this interface.
    class FPages
    {
    public:
        explicit FPages(TSharedRef<FBudget> Budget);
        ~FPages();
        EStep Begin(const TSharedPtr<FJsonObject>& Envelope, const FString& Capture, const FString& Kind);
        EStep Consume(const TSharedPtr<FJsonObject>& Envelope, uint32 NormalizedBodyBytes);
        EStep Advance();
        bool Next(FString& Root, uint64& Ordinal) const;
        TSharedPtr<FStorage> TakeComplete();
        TSharedPtr<FStorage> Cancel();
        const FManifest* Manifest() const;
        static bool ResourcesQualified() { return bNativeAllocationAccountingQualified; }
        // Used before DOM construction so duplicate envelope keys cannot be lost.
        static bool DecodeBody(const FString& Body, TSharedPtr<FJsonObject>& Out, FBudget& Budget);
    private:
        struct FData;
        TPimplPtr<FData> Data;
    };
}
