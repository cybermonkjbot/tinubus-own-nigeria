#include "BaddieCompanionWorldObservation.h"
#include "BaddieCompanionWorldObservationPages.h"
#include "BaddieCompanionSnapshot.h"
#include "Containers/Ticker.h"
#include "CoreGlobals.h"
#include "HAL/PlatformTime.h"

#include <cmath>

namespace BaddieCompanion::WorldObservationPrivate
{
    void FBudget::BeginTick()
    {
        if (Frame == GFrameCounter) return;
        Frame = GFrameCounter;
        TickRemaining = TickWorkLimit;
        bDecodedThisTick = false;
    }
    bool FBudget::Spend(uint64 Units, bool bCandidate)
    {
        if (bCandidate && (bExhausted || Units > CandidateWorkLimit - CandidateWork))
        { bExhausted = true; return false; }
        if (Units > TickRemaining) return false;
        TickRemaining -= static_cast<uint32>(Units);
        if (bCandidate) CandidateWork += Units;
        return true;
    }
    bool FBudget::Reserve(uint64 Bytes)
    {
        if (Bytes > RetainedLimit - Retained) { bExhausted = true; return false; }
        Retained += Bytes;
        return true;
    }
    void FBudget::Release(uint64 Bytes)
    {
        Retained = Bytes <= Retained ? Retained - Bytes : 0;
    }
    bool FBudget::ReservePage()
    {
        if (bPageReserved || bDecodedThisTick) return false;
        bPageReserved = true;
        bDecodedThisTick = true;
        return true;
    }
    void FBudget::ReleasePage() { bPageReserved = false; }

    FNode* FRoot::Get(uint64 Id) const
    {
        if (!Id || Id > Nodes || (Id - 1) / 64 >= InitializedIndex) return nullptr;
        const auto* Page = Index[(Id - 1) / 64];
        return Page ? const_cast<FNode*>(&Page->Nodes[(Id - 1) % 64]) : nullptr;
    }
    FStorage::FStorage(TSharedRef<FBudget> InBudget) : Budget(MoveTemp(InBudget)) {}
    FStorage::~FStorage()
    {
        // Every owner moves storage to RetireDetached before dropping its last reference.
        // No recursive tree deletion is permitted here.
        ensureMsgf(RetireRoot == RootLimit, TEXT("World observation storage was not retired."));
        Budget->Release(ChargedBytes);
    }
    FNode* FStorage::Get(uint8 Root, uint64 Id) const
    {
        return Root < RootLimit && Roots[Root] ? Roots[Root]->Get(Id) : nullptr;
    }
    EStep FStorage::Retire()
    {
        bRetiring = true; bComplete = false;
        while (RetireRoot < RootLimit)
        {
            if (!Budget->Spend(1, false)) return EStep::Pending;
            auto*& Root = Roots[RetireRoot];
            if (!Root) { ++RetireRoot; RetireNode = 1; continue; }
            if (RetireNode <= Root->Nodes)
            {
                auto* Node = Root->Get(RetireNode);
                if (Node && Node->Text)
                {
                    if (!Budget->Spend(sizeof(FStringBlock), false)) return EStep::Pending;
                    auto* Block = Node->Text;
                    Node->Text = Block->Next;
                    delete Block;
                    Budget->Release(sizeof(FStringBlock) * 2 + 64);
                    continue;
                }
                // Release a node page only after its last live node has been retired.
                if (RetireNode % 64 == 0 || RetireNode == Root->Nodes)
                {
                    if (!Budget->Spend(sizeof(FNodePage), false)) return EStep::Pending;
                    const uint64 Index = (RetireNode - 1) / 64;
                    delete Root->Index[Index]; Root->Index[Index] = nullptr;
                    Budget->Release(sizeof(FNodePage) * 2 + 64);
                }
                ++RetireNode;
                continue;
            }
            // Pointer index storage is trivial and has no owned element destructors.
            if (!Budget->Spend(256, false)) return EStep::Pending;
            delete Root; Root = nullptr;
            Budget->Release(sizeof(FRoot) * 2 + 64);
            ++RetireRoot; RetireNode = 1;
        }
        return EStep::Complete;
    }
    void RetireDetached(TSharedPtr<FStorage> Storage)
    {
        if (!Storage.IsValid()) return;
        Storage->bRetiring = true; Storage->bComplete = false;
        FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
            [Storage = MoveTemp(Storage)](float)
            {
                Storage->Budget->BeginTick();
                return Storage->Retire() != EStep::Complete;
            }));
    }
    bool FAccess::Current() const
    {
        const double Now = FPlatformTime::Seconds();
        const auto Owner = Storage.Pin();
        return bReadable && Generation != 0 && std::isfinite(Now) && std::isfinite(Deadline)
            && Now < Deadline && Owner.IsValid() && Owner->bComplete && !Owner->bRetiring;
    }
    void FAccess::Revoke()
    {
        bReadable = false;
        Storage.Reset(); Player.Reset(); Instance.Reset(); Selection.Reset();
        Clock.Clear(); GameRoomId.Reset(); Deadline = 0;
    }
}

namespace BaddieCompanion
{
    bool FWorldObservation::IsCurrent() const
    {
        const auto Current = Access.Pin();
        return Current.IsValid() && Current->Current();
    }
    uint64 FWorldObservation::Generation() const
    {
        const auto Current = Access.Pin();
        return Current.IsValid() && Current->Current() ? Current->Generation : 0;
    }
    bool FWorldObservation::ReadRoot(EWorldRoot Root, FWorldRootView& Out) const
    {
        Out = {};
        const auto Current = Access.Pin();
        if (!Current.IsValid() || !Current->Current()) return false;
        const auto Storage = Current->Storage.Pin();
        const uint8 Index = static_cast<uint8>(Root);
        if (Index >= WorldObservationPrivate::RootLimit
            || !Storage->Budget->Spend(Storage->Manifest.Count + 1, false)) return false;
        for (int32 I = 0; I < Storage->Manifest.Count; ++I)
        {
            const auto& Entry = Storage->Manifest.Entries[I];
            if (Entry.Slot != Index) continue;
            Out.Presence = static_cast<EWorldFieldPresence>(Entry.Presence);
            Out.bJsonDocument = Entry.Encoding == 1;
            Out.Nodes = Entry.Nodes; Out.StringUnits = Entry.Units;
            return true;
        }
        return false;
    }
    bool FWorldObservation::ReadNode(EWorldRoot Root, uint64 Id, FWorldNodeView& Out) const
    {
        Out = {};
        const auto Current = Access.Pin();
        if (!Current.IsValid() || !Current->Current()) return false;
        const auto Storage = Current->Storage.Pin();
        if (!Storage->Budget->Spend(sizeof(FWorldNodeView), false)) return false;
        const auto* Node = Storage->Get(static_cast<uint8>(Root), Id);
        if (!Node) return false;
        Out = *Node;
        return true;
    }
    bool FWorldObservation::ReadString(EWorldRoot Root, uint64 Id, FWorldStringCursor& Cursor,
        uint16 (&Out)[256], int32& OutCount) const
    {
        OutCount = 0;
        const auto Current = Access.Pin();
        if (!Current.IsValid() || !Current->Current()) { Cursor = {}; return false; }
        const auto Storage = Current->Storage.Pin();
        const auto* Node = Storage->Get(static_cast<uint8>(Root), Id);
        if (!Node || Node->Kind != EWorldValueKind::String) return false;
        if (!Cursor.Generation)
        {
            Cursor.Generation = Current->Generation; Cursor.Node = Id;
            Cursor.Root = static_cast<uint8>(Root); Cursor.Block = Node->Text;
        }
        if (Cursor.Generation != Current->Generation || Cursor.Node != Id
            || Cursor.Root != static_cast<uint8>(Root)) return false;
        const auto* Block = static_cast<const WorldObservationPrivate::FStringBlock*>(Cursor.Block);
        if (!Block) return Cursor.Offset == Node->Count;
        if (!Storage->Budget->Spend(1 + Block->Used * 2, false)) return false;
        for (int32 I = 0; I < Block->Used; ++I) Out[I] = Block->Units[I];
        OutCount = Block->Used; Cursor.Offset += Block->Used; Cursor.Block = Block->Next;
        return true;
    }
    bool FWorldObservation::CopyPlayer(TSharedPtr<const FWorldPlayerView>& Out) const
    {
        Out.Reset();
        const auto Current = Access.Pin();
        if (!Current.IsValid() || !Current->Current()) return false;
        Out = Current->Player;
        return Out.IsValid();
    }
    bool FWorldObservation::CopyTime(FWorldPresentationTime& Out) const
    {
        Out = {};
        const auto Current = Access.Pin();
        return Current.IsValid() && Current->Current()
            && Current->Clock.CopyTime(Current->GameRoomId, Out);
    }

    bool FWorldObservation::CopyInstance(TSharedPtr<const FWorldCurrentInstanceIdentity>& Out) const
    {
        Out.Reset();
        const auto Current = Access.Pin();
        if (!Current || !Current->Current()) return false;
        Out = Current->Instance;
        return Out.IsValid();
    }
    bool FWorldObservation::CopySelection(TSharedPtr<const FSnapshotSelection>& Out) const
    {
        Out.Reset();
        const auto Current = Access.Pin();
        if (!Current || !Current->Current()) return false;
        Out = Current->Selection;
        return Out.IsValid();
    }
}
