#pragma once

#include "BaddieCompanionWorldObservation.h"
#include "Dom/JsonObject.h"
#include "Templates/PimplPtr.h"

namespace BaddieCompanion::WorldReadPrivate { struct FPlayerResult; }

namespace BaddieCompanion::WorldObservationPrivate
{
    struct FPublicationPayload;
    struct FRetirementTicket;
    constexpr uint64 RetainedLimit = 67108864, CandidateWorkLimit = 67108864;
    constexpr uint32 TickWorkLimit = 32768, PageAllowance = 2097152;
    constexpr int32 BodyLimit = 32768, RecordBytesLimit = 28672, RootLimit = 20;
    // The pinned headers do not bound allocator slack, JSON DOM or crypto Pimpl
    // storage. Reject before candidate allocation until those bounds are qualified.
    constexpr bool bNativeAllocationAccountingQualified = false;
    constexpr const TCHAR* ProtocolGeneration = TEXT("80ab8251f1f8fc0d3ca780dd1ef61aed88db5d7fe5dd2c49cb70130d05662768");
    enum class EStep : uint8 { Pending, Complete, Invalid, ResourceUnavailable };
    // Complete owner reservations include allocator slack and retained text.
    // They are independent of the fixed object sizes and stay unqualified.
    constexpr uint64 QualifiedStorageBytes = 0, QualifiedStorageInitializationWork = 0;
    constexpr uint64 QualifiedRootBytes = 0, QualifiedRootInitializationWork = 0;
    constexpr uint64 QualifiedNodePageBytes = 0, QualifiedNodePageInitializationWork = 0;
    constexpr uint64 QualifiedStringBlockBytes = 0, QualifiedStringBlockInitializationWork = 0;

    struct FBudget
    {
        uint64 Retained = 0, CandidateWork = 0, OwnerCharge = 0;
        uint32 TickRemaining = TickWorkLimit;
        uint64 Frame = MAX_uint64;
        bool bExhausted = false, bPageReserved = false, bDecodedThisTick = false;
        FRetirementTicket* Retirement = nullptr;
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
    // Callers admit complete temporary allocation, copying and destruction work
    // before construction. This guard retains scratch bytes through unwinding.
    struct FAdmittedScratch
    {
        FBudget& Budget;
        uint64 Bytes;
        ~FAdmittedScratch() { Budget.Release(Bytes); }
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
        bool bClosed = false, bVerified = false;
        uint64 Owners = 1, RetireNode = 1;
        uint8 RetireText = 0;
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
        uint8 RetireManifestText = 0;
        bool bComplete = false, bRetiring = false;
        uint64 ChargedBytes = 0;
        // Logical totals include shared roots. Fetched totals count validated new
        // pages, not retries or wire framing received before validation.
        uint64 LogicalPages = 0, LogicalPageBytes = 0;
        uint64 FetchedPages = 0, FetchedPageBytes = 0, ReusedPages = 0, ReusedPageBytes = 0;
        WorldReadPrivate::FPlayerResult* Players = nullptr;
        FRoot* RacingOwner = nullptr;
        uint64 RacingNode = 0, RaceInstanceNode = 0;
        int64 RaceTick = 0;
        uint64 SemanticGeneration = 0;
        int64 PlayerRevision = -1, MoveSequence = -1, HealthRevision = -1, SavedRevision = -1;
        bool bSemanticsComplete = false;
        FPublicationPayload* Publication = nullptr;
        uint32 ActiveBorrows = 0;
        uint32 ActivePreparations = 0;
        static EStep ReleaseRoot(FRoot*& Root, FBudget& Budget);
        EStep Retire();
        FNode* Get(uint8 Root, uint64 Id) const;
    };
    struct FStaticContext
    {
        uint64 Lifetime = 0, CredentialGeneration = 0;
        int32 EntrySequence = 0;
        // Immutable, exactly materialized visit/life owned by this session.
        // The handle is never reused within Lifetime; it is not an identity hash.
        const void* Visit = nullptr;
    };
    constexpr uint64 QualifiedStaticOwnerBytes = 0;
    struct FStaticRoots
    {
        explicit FStaticRoots(TSharedRef<FBudget> InBudget) : Budget(MoveTemp(InBudget)) {}
        ~FStaticRoots();
        FStaticRoots(const FStaticRoots&) = delete;
        FStaticRoots& operator=(const FStaticRoots&) = delete;
        TSharedRef<FBudget> Budget;
        FRoot* Roots[2]{};
        FStaticContext Context;
        TCHAR Preparation[33]{};
        const TCHAR* Generation = ProtocolGeneration;
        double Deadline = 0;
        bool bAvailable = false;
        bool Current(const FStaticContext& Expected) const;
        EStep Retire();
    };
    struct FAccess
    {
        TSharedPtr<FBudget> Budget;
        TWeakPtr<FStorage> Storage;
        FWorldObservationToken Token;
        uint64 WeakHandles = 0, Charge = 0;
        int32 RetirementSlot = INDEX_NONE;
        double Deadline = 0;
        bool bReadable = false, bAlive = true;
        bool Current() const;
        void Revoke();
    };
    void RetireDetached(TSharedPtr<FStorage> Storage);
    void RetireDetached(TUniquePtr<FStaticRoots> Roots);
    constexpr uint64 QualifiedRetirementTicketBytes = 0;
    constexpr uint64 QualifiedRetirementTicketInitializationWork = 0;
    constexpr uint64 QualifiedRetirementTickerWork = 0;
    // Covers the complete object/controller deletion and ticker unregister,
    // including the budget's final internal strong release when it is last.
    constexpr uint64 QualifiedRetirementDeleteWork = 0;
    constexpr uint64 QualifiedDirectoryTextReleaseWork = 0;
    struct FRetirementTask
    {
        void* Owner = nullptr;
        EStep (*Advance)(void*, FBudget&) = nullptr;
        void (*Destroy)(void*) = nullptr;
        uint64 Charge = 0;
        bool bReserved = false;
        bool (*Ready)(void*) = nullptr;
    };
    struct FRetirementTicket
    {
        TSharedPtr<FBudget> Budget;
        TSharedPtr<FStorage> Storage[3];
        TUniquePtr<FStaticRoots> Static;
        FRetirementTask Tasks[8];
        uint64 Charge = 0;
        bool Empty() const;
        bool HasQueued() const;
        bool Tick();
    };
    bool EnsureRetirementTicket(TSharedPtr<FBudget> Budget);
    int32 ReserveRetirementTask(FBudget& Budget);
    void ReleaseRetirementTask(FBudget& Budget, int32 Slot);
    void QueueRetirementTask(FBudget& Budget, int32 Slot, FRetirementTask Task);

    // One bounded decoded body at a time. Credentials and actual HTTP stay outside.
    // Whole-root JSON reconstruction is deliberately absent from this interface.
    class FPages
    {
    public:
        explicit FPages(TSharedRef<FBudget> Budget);
        ~FPages();
        EStep Begin(const TSharedPtr<FJsonObject>& Envelope, const FString& Capture, const FString& Kind,
            const FStaticRoots* Reuse = nullptr, FStaticContext Context = {}, const FString& Preparation = FString());
        EStep Consume(const TSharedPtr<FJsonObject>& Envelope, uint32 NormalizedBodyBytes);
        bool RetainBody(TSharedPtr<FJsonObject>& Envelope, uint64& Charge);
        EStep Advance();
        EStep Next(TCHAR (&Root)[33], uint64& Ordinal) const;
        TSharedPtr<FStorage> TakeComplete();
        TSharedPtr<FStorage> Cancel();
        const FManifest* Manifest() const;
        static bool ResourcesQualified() { return bNativeAllocationAccountingQualified; }
        // Used before DOM construction so duplicate envelope keys cannot be lost.
        static EStep DecodeBody(const FString& Body, TSharedPtr<FJsonObject>& Out, FBudget& Budget);
    private:
        struct FData;
        TUniquePtr<FData> Data;
    };
}
