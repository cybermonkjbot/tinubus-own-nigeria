#include "BaddieCompanionWorldSession.h"
#include "BaddieCompanionStateJson.h"
#include "BaddieCompanionWorldPlayerRead.h"
#include "HAL/PlatformTime.h"
#include "Containers/Ticker.h"

#include <cmath>

namespace BaddieCompanion
{
    namespace
    {
        using namespace WorldObservationPrivate;
        using namespace StateJsonPrivate;
        constexpr uint64 MaxCounter = 9007199254740991ULL;
        bool Exact(const FObject& O, std::initializer_list<const TCHAR*> Names)
        { return O.IsValid() && O->Values.Num() == static_cast<int32>(Names.size()) && Only(O, Names); }
        bool Hex(const FString& S, int32 Count)
        {
            if (S.Len() != Count) return false;
            for (TCHAR C : S) if (!(C >= '0' && C <= '9') && !(C >= 'a' && C <= 'f')) return false;
            return true;
        }
        bool Text(const FObject& O, const TCHAR* Key, FString& S)
        { return String(Field(O, Key), S); }
        const TCHAR* Kind(EWorldRequest Operation)
        {
            switch (Operation)
            {
            case EWorldRequest::Enter: return TEXT("enter");
            case EWorldRequest::Prepare: return TEXT("prepare");
            case EWorldRequest::Observe: return TEXT("observe");
            case EWorldRequest::Move: return TEXT("move");
            case EWorldRequest::Ready: return TEXT("ready");
            case EWorldRequest::PresentationFailed: return TEXT("presentationFailed");
            default: return TEXT("");
            }
        }
        bool Command(EWorldRequest O)
        {
            return O == EWorldRequest::Enter || O == EWorldRequest::Move
                || O == EWorldRequest::Ready || O == EWorldRequest::PresentationFailed;
        }
    }

    // Reader state and peer projections have separate, still-unqualified allocator
    // reservations. These are not estimates of Unreal allocator/controller slack.
    static constexpr uint64 QualifiedSemanticDataBytes = 0;

    struct FWorldSession::FSemanticData
    {
        using FRef = WorldReadPrivate::FValueRef;
        using EStep = WorldObservationPrivate::EStep;
        using FOrigin = WorldReadPrivate::FOrigin;
        using FPlayerResult = WorldReadPrivate::FPlayerResult;
        FSemanticData() {}
        struct FRecord
        {
            // Capture initializes all slots before the summary becomes readable.
            FRecord() {}
            FRef Root;
            union { FRef Fields[32]; };
            double Numbers[32];
            uint64 Counts[32];
            int32 Choices[32];
            EWorldFieldPresence Presence[32];
        };
        TSharedPtr<WorldObservationPrivate::FStorage> Storage, Prior;
        WorldReadPrivate::FDomainRead Domain;
        WorldReadPrivate::FRecordRead Record;
        WorldReadPrivate::FBindingRead Binding;
        WorldReadPrivate::FHealthRead Health;
        WorldReadPrivate::FTextRead Text;
        FRecord Player, LocalPlayer, PriorPlayer, Instance, Library, Journey, Roadside, Endpoint, Navigation, Room;
        FRecord Driving, PriorDriving, Driver, Occupant, Water, PriorWater, Scope, PriorScope;
        FRecord Activity, PriorActivity, Round, PriorRound, Input;
        FPlayerResult* Players = nullptr;
        FPlayerResult* Tail = nullptr;
        FPlayerResult* CurrentPlayer = nullptr;
        const FPlayerResult* PreviousPlayer = nullptr;
        const FPlayerResult* PreviousCursor = nullptr;
        FOrigin Racing;
        FRef Roots[20];
        EWorldFieldPresence RootPresence[20]{};
        uint8 Encoding[20]{};
        FRef Selection, SelectedCharacter, SelectedHome, SelectedActiveRoom, RaceInstance;
        int64 RaceTick = 0;
        uint64 Generation = 0, Peer = 0, PeerIndex = 0;
        uint32 Phase = 0, Step = 0, Comparison = 0, CopyIndex = 0, Subphase = 0;
        uint32 InputIndex = 0, InputPass = 0;
        uint64 LaneNode = 0, PriorLaneNode = 0;
        double DanceTimes[24], DanceLanes[24];
        uint64 InputNode = 0;
        bool bWork = false, bRecordWork = false, bText = false, bCancelled = false;
        bool bLocal = true, bKeepDriving = false, bKeepWater = false, bSamePlayer = false;
        bool bPrepare = false, bHome = false, bRoad = false;
        bool bSuppressed = false, bSameRound = false;
        EWorldRequest Operation = EWorldRequest::None;
        FRef ExpectedPlace, ExpectedEndpoint;
        TCHAR Joined[1024];
        uint32 JoinedCount = 0, AppendOffset = 0;
        const WorldObservationPrivate::FStringBlock* AppendBlock = nullptr;
        uint64 AppendRemaining = 0;
        bool bAppending = false;
        EStep Result = EStep::Pending;

        EStep Yield() const { return Storage->Budget->bExhausted ? EStep::ResourceUnavailable : EStep::Pending; }
        bool Spend(uint64 Units) { return Storage->Budget->Spend(Units); }
        EStep Capture(const WorldReadPrivate::FRecordRead& Read, FRef Root, FRecord& Out)
        {
            while (CopyIndex < 32)
            {
                if (!Spend(2 * (sizeof(FRef) + sizeof(double) + sizeof(uint64) + sizeof(int32) + 1))) return Yield();
                new (&Out.Fields[CopyIndex]) FRef(Read.Field(CopyIndex)); Out.Numbers[CopyIndex] = Read.Number(CopyIndex);
                Out.Counts[CopyIndex] = Read.Count(CopyIndex); Out.Choices[CopyIndex] = Read.Choice(CopyIndex);
                Out.Presence[CopyIndex] = Read.Presence(CopyIndex); ++CopyIndex;
            }
            if (!Spend(2 * sizeof(FRef))) return Yield();
            Out.Root = Root; CopyIndex = 0; return EStep::Complete;
        }
        EStep Read(WorldReadPrivate::EDomain Kind, FRef Root, FRecord& Out, bool bChildren = false)
        {
            if (!bWork)
            {
                if (!Spend(bChildren ? sizeof(WorldReadPrivate::FDomainRead) - WorldReadPrivate::FDomainRead::DeferredBytes()
                    : WorldReadPrivate::FRecordRead::BeginWork())) return Yield();
                if (bChildren) Domain.Begin(Root, Kind);
                else Record.Begin(Root, *WorldReadPrivate::DomainSchema(Kind).Record);
                bWork = true;
            }
            const auto Result = bChildren ? Domain.Advance(*Storage) : Record.Advance(*Storage);
            if (Result != EStep::Complete) return Result;
            const auto Copied = Capture(bChildren ? Domain.RootRecord() : Record, Root, Out);
            if (Copied == EStep::Complete) bWork = false;
            return Copied;
        }
        EStep Equal(FRef A, FRef B, bool& Equal);
        EStep Literal(FRef A, const TCHAR* B, uint32 Length, bool& Equal);
        EStep Owner(FRef A, const FString& B, bool& Equal);
        EStep Append(FRef Value);
        EStep AppendLiteral(const TCHAR* Value, uint32 Count);
        EStep ReadSchema(const WorldReadPrivate::FRecordSchema& Schema, FRef Root, FRecord& Out)
        {
            if (!bWork)
            {
                if (!Spend(WorldReadPrivate::FRecordRead::BeginWork())) return Yield();
                Record.Begin(Root, Schema); bWork = true;
            }
            const auto Result = Record.Advance(*Storage);
            if (Result != EStep::Complete) return Result;
            const auto Copied = Capture(Record, Root, Out);
            if (Copied == EStep::Complete) bWork = false;
            return Copied;
        }
        EStep Reconcile();
        EStep RetainActivity();
        EStep InstanceIdentity(const FWorldEntryIdentity& Entry);
        EStep ReadRacing();
        EStep ReadRemaining();
        EStep ValidateControls();
        EStep PinPlayer();
        EStep Retire()
        {
            bCancelled = true;
            while (Players)
            {
                if (Players->Retire(*Storage->Budget) != EStep::Complete) return EStep::Pending;
                if (!Storage->Budget->Spend(sizeof(FPlayerResult), false)) return EStep::Pending;
                auto* Old = Players; Players = Players->Next;
                delete Old; Storage->Budget->Release(WorldReadPrivate::QualifiedSemanticPeerBytes);
            }
            return EStep::Complete;
        }
        EStep AllocatePlayer(FRef Raw)
        {
            if (!WorldReadPrivate::QualifiedSemanticPeerBytes) return EStep::ResourceUnavailable;
            if (!Spend(sizeof(FPlayerResult))) return Yield();
            if (!Storage->Budget->Reserve(WorldReadPrivate::QualifiedSemanticPeerBytes)) return EStep::ResourceUnavailable;
            auto* Next = new FPlayerResult;
            Next->Raw = Raw; Next->bPeer = !bLocal;
            if (Tail) Tail->Next = Next; else Players = Next;
            Tail = CurrentPlayer = Next;
            return EStep::Complete;
        }
    };

    WorldObservationPrivate::EStep FWorldSession::FSemanticData::Equal(FRef A, FRef B, bool& Equal)
    {
        if (!bText)
        {
            if (!Spend(2 * sizeof(WorldReadPrivate::FTextRead) + 64)) return Yield();
            Text.Compare(A, B);
            bText = true;
        }
        const auto Result = Text.Advance(*Storage);
        if (Result == EStep::Complete) { Equal = Text.Equal(); bText = false; }
        return Result;
    }
    WorldObservationPrivate::EStep FWorldSession::FSemanticData::Literal(FRef A, const TCHAR* B, uint32 Length, bool& Equal)
    {
        if (!bText)
        {
            if (!Spend(2 * sizeof(WorldReadPrivate::FTextRead) + 64)) return Yield();
            Text.Compare(A, B, Length); bText = true;
        }
        const auto Result = Text.Advance(*Storage);
        if (Result == EStep::Complete) { Equal = Text.Equal(); bText = false; }
        return Result;
    }
    WorldObservationPrivate::EStep FWorldSession::FSemanticData::Owner(FRef A, const FString& B, bool& Equal)
    {
        if (!bText)
        {
            if (!Spend(2 * sizeof(WorldReadPrivate::FTextRead) + 64)) return Yield();
            Text.Compare(A, B); bText = true;
        }
        const auto Result = Text.Advance(*Storage);
        if (Result == EStep::Complete) { Equal = Text.Equal(); bText = false; }
        return Result;
    }
    WorldObservationPrivate::EStep FWorldSession::FSemanticData::Append(FRef Value)
    {
        if (!bAppending)
        {
            if (!Spend(16)) return Yield();
            const auto* Node = WorldReadPrivate::Resolve(*Storage, Value);
            if (!Node || Node->Kind != EWorldValueKind::String || Node->Count > UE_ARRAY_COUNT(Joined) - JoinedCount) return EStep::Invalid;
            AppendBlock = Node->Text; AppendRemaining = Node->Count; AppendOffset = 0; bAppending = true;
        }
        while (AppendRemaining)
        {
            if (!Spend(8)) return Yield();
            if (!AppendBlock || AppendOffset >= AppendBlock->Used) return EStep::Invalid;
            Joined[JoinedCount++] = static_cast<TCHAR>(AppendBlock->Units[AppendOffset++]); --AppendRemaining;
            if (AppendOffset == AppendBlock->Used) { AppendOffset = 0; AppendBlock = AppendBlock->Next; }
        }
        if (AppendBlock) return EStep::Invalid;
        bAppending = false; return EStep::Complete;
    }
    WorldObservationPrivate::EStep FWorldSession::FSemanticData::AppendLiteral(const TCHAR* Value, uint32 Count)
    {
        if (Count > UE_ARRAY_COUNT(Joined) - JoinedCount) return EStep::Invalid;
        if (!Spend(Count * 2 + 1)) return Yield();
        for (uint32 I = 0; I < Count; ++I) Joined[JoinedCount++] = Value[I];
        return EStep::Complete;
    }

    WorldObservationPrivate::EStep FWorldSession::FSemanticData::InstanceIdentity(const FWorldEntryIdentity&)
    {
        using namespace WorldReadPrivate;
        const auto& B = Binding.Binding();
        const bool Traveling = !bPrepare && Player.Choices[10] == 1;
        while (Subphase < 27)
        {
            if (!Spend(24)) return Yield();
            EStep S = EStep::Complete; bool Same = true;
            switch (Subphase)
            {
            case 0: S = Read(EDomain::Navigation, Instance.Fields[10], Navigation); break;
            case 1: if (Traveling) S = Read(EDomain::Journey, Player.Fields[12], Journey); break;
            case 2:
                ExpectedPlace = bPrepare ? Instance.Fields[1] : Traveling ? Journey.Fields[3] : Player.Fields[3];
                S = Equal(Instance.Fields[1], ExpectedPlace, Same); break;
            case 3: S = Literal(Instance.Fields[1], TEXT("own-home"), 8, bHome); break;
            case 4:
                if (bHome)
                {
                    static constexpr FFieldRule F[] = {FFieldRule(TEXT("id"), EWorldValueKind::String).WithText({1, 240, true, true})};
                    static constexpr FRecordSchema Schema(F, false);
                    if (Instance.Presence[8] != EWorldFieldPresence::Value) return EStep::Invalid;
                    S = ReadSchema(Schema, Instance.Fields[8], Room);
                }
                break;
            case 5: if (bHome) S = Equal(Room.Fields[0], B.Field(BindingHome), Same); break;
            case 6: JoinedCount = 0; break;
            case 7: S = Append(B.Field(BindingGameRoom)); break;
            case 8: S = bHome ? AppendLiteral(TEXT(":own-home:"), 10) : AppendLiteral(TEXT(":"), 1); break;
            case 9: S = Append(bHome ? B.Field(BindingUser) : ExpectedPlace); break;
            case 10: if (bHome) S = AppendLiteral(TEXT(":"), 1); break;
            case 11: if (bHome) S = Append(Room.Fields[0]); break;
            case 12: S = Literal(Instance.Fields[0], Joined, JoinedCount, Same); break;
            case 13: if (!bPrepare && !Traveling) S = Equal(Instance.Fields[0], Player.Fields[4], Same); break;
            case 14:
                if (!bText)
                {
                    if (!Spend(2 * sizeof(FTextRead) + 64)) return Yield();
                    Text.StartsWith(ExpectedPlace, TEXT("roadside:"), 9); bText = true;
                }
                S = Text.Advance(*Storage);
                if (S == EStep::Complete) { bRoad = Text.Equal(); bText = false; }
                break;
            case 15:
                if (!bRoad)
                {
                    if (Instance.Presence[6] != EWorldFieldPresence::Missing) return EStep::Invalid;
                    Subphase = 27; return EStep::Complete;
                }
                if (Instance.Presence[6] != EWorldFieldPresence::Value) return EStep::Invalid;
                S = Read(EDomain::Endpoint, Instance.Fields[6], Endpoint); break;
            case 16:
                if (!bPrepare && !Traveling)
                {
                    if (Player.Presence[13] != EWorldFieldPresence::Value) return EStep::Invalid;
                    S = Read(EDomain::Roadside, Player.Fields[13], Roadside);
                }
                break;
            case 17:
                ExpectedEndpoint = bPrepare ? Endpoint.Root : Traveling ? Journey.Fields[12] : Roadside.Fields[0];
                if (!ExpectedEndpoint.IsSet()) return EStep::Invalid;
                S = Read(EDomain::Endpoint, ExpectedEndpoint, Scope); break;
            case 18:
                if (Endpoint.Choices[0] != 1 || Scope.Choices[0] != 1 || Endpoint.Presence[3] != Scope.Presence[3]
                    || Endpoint.Presence[4] != Scope.Presence[4]
                    || (Endpoint.Presence[4] == EWorldFieldPresence::Value && Endpoint.Numbers[4] != Scope.Numbers[4])
                    || Instance.Numbers[5] != 0 || Instance.Presence[7] != EWorldFieldPresence::Null
                    || Instance.Presence[8] != EWorldFieldPresence::Null) return EStep::Invalid;
                S = Equal(Endpoint.Fields[1], Scope.Fields[1], Same); break;
            case 19: S = Read(EDomain::Geo, Endpoint.Fields[2], Driver); break;
            case 20: S = Read(EDomain::Geo, Scope.Fields[2], Occupant); break;
            case 21:
                if (Driver.Numbers[0] != Occupant.Numbers[0] || Driver.Numbers[1] != Occupant.Numbers[1]) return EStep::Invalid;
                if (Endpoint.Presence[3] == EWorldFieldPresence::Value) S = Equal(Endpoint.Fields[3], Scope.Fields[3], Same);
                break;
            case 22:
                if (!bText)
                {
                    if (!Spend(2 * sizeof(FTextRead) + 64)) return Yield();
                    Text.ComparePrefixed(ExpectedPlace, Endpoint.Fields[1], TEXT("roadside:"), 9); bText = true;
                }
                S = Text.Advance(*Storage);
                if (S == EStep::Complete) { Same = Text.Equal(); bText = false; }
                break;
            case 23: S = Equal(Instance.Fields[2], Endpoint.Fields[1], Same); break;
            case 24: S = Literal(Instance.Fields[3], TEXT("roadside-v1"), 11, Same); break;
            case 25: S = Literal(Instance.Fields[4], TEXT("roadside-v1"), 11, Same); break;
            case 26: S = Literal(Navigation.Fields[0], TEXT("roadside-v1"), 11, Same); break;
            }
            if (S != EStep::Complete) return S;
            if (!Same) return EStep::Invalid;
            ++Subphase;
        }
        return EStep::Complete;
    }

    WorldObservationPrivate::EStep FWorldSession::FSemanticData::ValidateControls()
    {
        using namespace WorldReadPrivate;
        const bool HasDriving = CurrentPlayer->Driving.Ref.IsSet(), HasWater = CurrentPlayer->Water.Ref.IsSet();
        while (Step < 9)
        {
            if (!Spend(512)) return Yield();
            EStep S = EStep::Complete; bool Same = true;
            switch (Step)
            {
            case 0: if (HasDriving) S = Read(EDomain::Driving, CurrentPlayer->Driving.Ref, Driving); break;
            case 1: if (HasDriving) S = Read(EDomain::DrivingOccupant, Driving.Fields[11], Driver); break;
            case 2:
                if (HasDriving)
                {
                    const auto* A = Resolve(*Storage, Driving.Fields[17]);
                    if (!A || A->Count != 1) return EStep::Invalid;
                    S = Read(EDomain::DrivingOccupant, Driving.Fields[17].Child(A->FirstChild), Occupant);
                }
                break;
            case 3:
                if (HasDriving)
                {
                    if (!DrivingSpeed(Driving.Numbers[13])) return EStep::Invalid;
                    const FRef A[] = {Driving.Fields[2], Driving.Fields[3], Driving.Fields[4], Driver.Fields[0], Driver.Fields[1],
                        Occupant.Fields[0], Occupant.Fields[1], Occupant.Fields[2], Driver.Fields[2]};
                    const FRef B[] = {Player.Fields[2], Player.Fields[4], Navigation.Fields[0], Player.Fields[0], Player.Fields[1],
                        Player.Fields[0], Player.Fields[1], Driver.Fields[2], Binding.Visit().Field(VisitLife)};
                    while (Comparison < (bLocal ? 9u : 8u))
                    {
                        S = Equal(A[Comparison], B[Comparison], Same);
                        if (S != EStep::Complete) return S;
                        if (!Same) return EStep::Invalid;
                        ++Comparison;
                    }
                }
                break;
            case 4: if (HasWater) S = Read(EDomain::Water, CurrentPlayer->Water.Ref, Water); break;
            case 5: if (HasWater) S = Read(EDomain::WaterScope, Water.Fields[3], Scope); break;
            case 6:
                if (HasWater)
                {
                    if (!WaterRevision(static_cast<int64>(Scope.Numbers[8])) || Scope.Numbers[8] != Player.Numbers[6]
                        || !WaterTiming(Water.Numbers[7], Water.Numbers[10], Water.Numbers[6], Water.Numbers[8], Water.Numbers[9])) return EStep::Invalid;
                    const FRef A[] = {Scope.Fields[0], Scope.Fields[1], Scope.Fields[2], Scope.Fields[3], Scope.Fields[4], Scope.Fields[5], Scope.Fields[6], Scope.Fields[7]};
                    const FRef B[] = {Player.Fields[2], Player.Fields[4], Player.Fields[3], Instance.Fields[4], Navigation.Fields[0], Player.Fields[0], Player.Fields[1], Binding.Visit().Field(VisitLife)};
                    while (Comparison < (bLocal ? 8u : 7u))
                    {
                        S = Equal(A[Comparison], B[Comparison], Same);
                        if (S != EStep::Complete) return S;
                        if (!Same) return EStep::Invalid;
                        ++Comparison;
                    }
                }
                break;
            case 7:
                if (Player.Presence[12] == EWorldFieldPresence::Value) S = Read(EDomain::Journey, Player.Fields[12], Journey);
                break;
            case 8:
                if (ConflictingTransport(HasDriving, HasWater && Water.Choices[15] == 0,
                    Player.Presence[9] == EWorldFieldPresence::Value, Player.Presence[14] == EWorldFieldPresence::Value,
                    Player.Presence[12] == EWorldFieldPresence::Value && Journey.Choices[5] == 0)) return EStep::Invalid;
                break;
            }
            if (S != EStep::Complete) return S;
            Comparison = 0; ++Step;
        }
        Step = 0; return EStep::Complete;
    }

    WorldObservationPrivate::EStep FWorldSession::FSemanticData::Reconcile()
    {
        using namespace WorldReadPrivate;
        while (Subphase < 17)
        {
            if (!Spend(128)) return Yield();
            EStep S = EStep::Complete; bool Same = true;
            switch (Subphase)
            {
            case 0:
                CurrentPlayer->Position = {Player.Fields[8], false};
                CurrentPlayer->Weapon = {Player.Fields[19], false};
                PreviousPlayer = nullptr; PreviousCursor = Prior && Prior->bSemanticsComplete ? Prior->Players : nullptr;
                bSamePlayer = false; bKeepDriving = bKeepWater = false; Step = Comparison = 0;
                break;
            case 1:
                while (PreviousCursor)
                {
                    if (!Spend(16)) return Yield();
                    if (PreviousCursor->bPeer != !bLocal || !PreviousCursor->bIncluded) { PreviousCursor = PreviousCursor->Next; continue; }
                    if (Step == 0)
                    {
                        S = Read(EDomain::Player, PreviousCursor->Raw, PriorPlayer);
                        if (S != EStep::Complete) return S;
                        Step = 1;
                    }
                    S = Equal(Player.Fields[0], PriorPlayer.Fields[0], Same);
                    if (S != EStep::Complete) return S;
                    if (Same) PreviousPlayer = PreviousCursor;
                    PreviousCursor = PreviousCursor->Next; Step = 0;
                }
                break;
            case 2: if (PreviousPlayer) S = Read(EDomain::Player, PreviousPlayer->Raw, PriorPlayer); break;
            case 3:
                if (PreviousPlayer)
                {
                    if (!Comparison) bSamePlayer = Player.Numbers[6] == PriorPlayer.Numbers[6];
                    static constexpr uint32 Fields[] = {0, 1, 2, 4};
                    while (Comparison < 4)
                    {
                        S = Equal(Player.Fields[Fields[Comparison]], PriorPlayer.Fields[Fields[Comparison]], Same);
                        if (S != EStep::Complete) return S;
                        bSamePlayer &= Same; ++Comparison;
                    }
                }
                break;
            case 4:
                bSuppressed = !Instance.Root.IsSet() || Player.Choices[10] != 0 || Player.Numbers[11] <= 0;
                if (!bSuppressed)
                {
                    S = Equal(Player.Fields[4], Instance.Fields[0], Same);
                    if (S != EStep::Complete) return S;
                    bSuppressed = !Same;
                }
                if (bSuppressed) { Subphase = 17; return EStep::Complete; }
                CurrentPlayer->Driving = {Player.Presence[15] == EWorldFieldPresence::Value ? Player.Fields[15] : FRef{}, false};
                CurrentPlayer->Water = {Player.Presence[16] == EWorldFieldPresence::Value ? Player.Fields[16] : FRef{}, false};
                break;
            case 5: S = ValidateControls(); break;
            case 6:
                if (bSamePlayer && CurrentPlayer->Driving.Ref.IsSet() && PreviousPlayer->Driving.Ref.IsSet())
                    S = Read(EDomain::Driving, PreviousPlayer->Driving.Ref, PriorDriving);
                break;
            case 7:
                if (bSamePlayer && CurrentPlayer->Driving.Ref.IsSet() && PreviousPlayer->Driving.Ref.IsSet())
                    S = Read(EDomain::DrivingOccupant, PriorDriving.Fields[11], Endpoint);
                break;
            case 8:
                if (bSamePlayer && CurrentPlayer->Driving.Ref.IsSet() && PreviousPlayer->Driving.Ref.IsSet())
                {
                    if (!Comparison)
                    {
                        S = Equal(Driving.Fields[0], PriorDriving.Fields[0], Same);
                        if (S != EStep::Complete) return S;
                        bKeepDriving = Same; ++Comparison;
                    }
                    S = Equal(Driver.Fields[2], Endpoint.Fields[2], Same);
                    if (S != EStep::Complete) return S;
                    bKeepDriving = RetainDriving(bKeepDriving && Same, Driving.Numbers[7], Driving.Numbers[8], Driving.Numbers[9],
                        PriorDriving.Numbers[7], PriorDriving.Numbers[8], PriorDriving.Numbers[9]);
                }
                break;
            case 9:
                if (bSamePlayer && CurrentPlayer->Water.Ref.IsSet() && PreviousPlayer->Water.Ref.IsSet())
                    S = Read(EDomain::Water, PreviousPlayer->Water.Ref, PriorWater);
                break;
            case 10:
                if (bSamePlayer && CurrentPlayer->Water.Ref.IsSet() && PreviousPlayer->Water.Ref.IsSet())
                    S = Read(EDomain::WaterScope, PriorWater.Fields[3], PriorScope);
                break;
            case 11:
                if (bSamePlayer && CurrentPlayer->Water.Ref.IsSet() && PreviousPlayer->Water.Ref.IsSet())
                {
                    if (!Comparison)
                    {
                        S = Equal(Water.Fields[0], PriorWater.Fields[0], Same);
                        if (S != EStep::Complete) return S;
                        bKeepWater = Same && Scope.Numbers[8] == PriorScope.Numbers[8]; ++Comparison;
                    }
                    while (Comparison <= 8)
                    {
                        S = Equal(Scope.Fields[Comparison - 1], PriorScope.Fields[Comparison - 1], Same);
                        if (S != EStep::Complete) return S;
                        bKeepWater &= Same; ++Comparison;
                    }
                    bKeepWater = RetainWater(bKeepWater, Water.Numbers[5], Water.Numbers[4], Water.Numbers[6],
                        PriorWater.Numbers[5], PriorWater.Numbers[4], PriorWater.Numbers[6]);
                }
                break;
            case 12:
                if (bKeepDriving) CurrentPlayer->Driving = PreviousPlayer->Driving;
                if (bKeepWater) CurrentPlayer->Water = PreviousPlayer->Water;
                if (bKeepDriving || bKeepWater) CurrentPlayer->Position = PreviousPlayer->Position;
                break;
            case 13: S = ValidateControls(); break;
            case 14:
                // Controls remain unavailable to commands; these are effective read values only.
                CurrentPlayer->bControlUnavailable = true;
                break;
            case 15: break;
            case 16: break;
            }
            if (S != EStep::Complete) return S;
            Comparison = 0; ++Subphase;
        }
        return EStep::Complete;
    }

    WorldObservationPrivate::EStep FWorldSession::FSemanticData::RetainActivity()
    {
        using namespace WorldReadPrivate;
        while (Subphase < 18)
        {
            if (!Spend(128)) return Yield();
            EStep S = EStep::Complete; bool Same = true;
            switch (Subphase)
            {
            case 0:
                if (!bLocal)
                {
                    S = Literal(LocalPlayer.Fields[3], TEXT("own-home"), 8, Same);
                    if (S != EStep::Complete) return S;
                    CurrentPlayer->bIncluded = !Same && LocalPlayer.Choices[10] != 1 && Player.Choices[10] != 1;
                }
                break;
            case 1:
                if (!bLocal)
                {
                    S = Equal(Player.Fields[4], LocalPlayer.Fields[4], Same);
                    if (S != EStep::Complete) return S;
                    CurrentPlayer->bIncluded &= Same;
                }
                break;
            case 2:
                if (bLocal && PreviousPlayer && Player.Presence[19] == EWorldFieldPresence::Missing)
                {
                    S = Equal(Player.Fields[4], PriorPlayer.Fields[4], Same);
                    if (S != EStep::Complete) return S;
                    if (Same) CurrentPlayer->Weapon = PreviousPlayer->Weapon;
                }
                break;
            case 3:
                if (!bSamePlayer || Player.Choices[10] != PriorPlayer.Choices[10]
                    || Player.Presence[17] != EWorldFieldPresence::Value || PriorPlayer.Presence[17] != EWorldFieldPresence::Value)
                { Subphase = 18; return EStep::Complete; }
                break;
            case 4: S = Read(EDomain::Nightclub, Player.Fields[17], Activity); break;
            case 5: S = Read(EDomain::Nightclub, PriorPlayer.Fields[17], PriorActivity); break;
            case 6:
                if (Activity.Choices[2] != 0 || PriorActivity.Choices[2] != 0 || Activity.Numbers[1] != PriorActivity.Numbers[1])
                { Subphase = 18; return EStep::Complete; }
                break;
            case 7:
                S = Equal(Activity.Fields[0], PriorActivity.Fields[0], Same);
                if (S != EStep::Complete) return S;
                if (!Same) { Subphase = 18; return EStep::Complete; }
                break;
            case 8: S = Read(EDomain::DanceRound, Activity.Fields[6], Round); break;
            case 9: S = Read(EDomain::DanceRound, PriorActivity.Fields[6], PriorRound); break;
            case 10:
                if (Round.Numbers[3] != PriorRound.Numbers[3] || Round.Numbers[4] != PriorRound.Numbers[4]
                    || Round.Numbers[5] != PriorRound.Numbers[5] || Round.Numbers[6] != PriorRound.Numbers[6]
                    || Round.Counts[7] != PriorRound.Counts[7]) { Subphase = 18; return EStep::Complete; }
                while (Comparison < 2)
                {
                    S = Equal(Round.Fields[Comparison + 1], PriorRound.Fields[Comparison + 1], Same);
                    if (S != EStep::Complete) return S;
                    if (!Same) { Subphase = 18; return EStep::Complete; }
                    ++Comparison;
                }
                break;
            case 11:
                LaneNode = Resolve(*Storage, Round.Fields[7])->FirstChild;
                PriorLaneNode = Resolve(*Storage, PriorRound.Fields[7])->FirstChild;
                break;
            case 12:
                while (LaneNode || PriorLaneNode)
                {
                    if (!Spend(48)) return Yield();
                    const auto* A = Resolve(*Storage, Round.Fields[7].Child(LaneNode));
                    const auto* B = Resolve(*Storage, PriorRound.Fields[7].Child(PriorLaneNode));
                    if (!A || !B) return EStep::Invalid;
                    double Left = 0, Right = 0;
                    FMemory::Memcpy(&Left, &A->NumberBits, sizeof(double)); FMemory::Memcpy(&Right, &B->NumberBits, sizeof(double));
                    if (Left != Right) { Subphase = 18; return EStep::Complete; }
                    LaneNode = A->NextSibling; PriorLaneNode = B->NextSibling;
                }
                break;
            case 13:
                if (!Spend(24 * (sizeof(FOrigin) + 2 * sizeof(double)))) return Yield();
                for (uint32 I = 0; I < 24; ++I)
                { CurrentPlayer->DanceInputs[I] = {}; DanceTimes[I] = DBL_MAX; DanceLanes[I] = 4; }
                InputPass = InputIndex = Step = 0;
                InputNode = Resolve(*Storage, Activity.Fields[7])->FirstChild;
                break;
            case 14:
                while (InputPass < 2)
                {
                    if (!Spend(64)) return Yield();
                    const bool Merged = InputPass == 1 && PreviousPlayer->bDanceMerged;
                    if (InputIndex == 24 || (Merged ? InputIndex >= PreviousPlayer->DanceCount : InputNode == 0))
                    {
                        ++InputPass; InputIndex = Step = 0;
                        if (InputPass == 1) InputNode = Resolve(*Storage, PriorActivity.Fields[7])->FirstChild;
                        continue;
                    }
                    const FRef Ref = Merged ? PreviousPlayer->DanceInputs[InputIndex].Ref
                        : (InputPass ? PriorActivity : Activity).Fields[7].Child(InputNode);
                    if (Step == 0)
                    {
                        S = Read(EDomain::DanceInput, Ref, Input);
                        if (S != EStep::Complete) return S;
                        Step = 1;
                    }
                    if (Step == 1)
                    {
                        S = Equal(Player.Fields[0], Input.Fields[0], Same);
                        if (S != EStep::Complete) return S;
                        bSameRound = Same; Step = 2;
                    }
                    if (!Spend(64)) return Yield();
                    const int64 Beat = static_cast<int64>(Input.Numbers[1]);
                    if (bSameRound && DanceBeat(Beat, Round.Counts[7]) && IntegerNumber(Input.Numbers[2], 0, 3)
                        && FiniteNumber(Input.Numbers[3]) && (Input.Numbers[3] < DanceTimes[Beat]
                            || (Input.Numbers[3] == DanceTimes[Beat] && Input.Numbers[2] < DanceLanes[Beat])))
                    {
                        CurrentPlayer->DanceInputs[Beat] = {Ref, InputPass == 1};
                        DanceTimes[Beat] = Input.Numbers[3]; DanceLanes[Beat] = Input.Numbers[2];
                    }
                    if (!Merged) InputNode = Resolve(*Storage, Ref)->NextSibling;
                    ++InputIndex; Step = 0;
                }
                InputIndex = 0;
                break;
            case 15:
                while (InputIndex < 24)
                {
                    if (!Spend(sizeof(FOrigin) * 2)) return Yield();
                    const auto Ref = CurrentPlayer->DanceInputs[InputIndex++];
                    if (Ref.Ref.IsSet()) CurrentPlayer->DanceInputs[CurrentPlayer->DanceCount++] = Ref;
                }
                InputIndex = CurrentPlayer->DanceCount;
                break;
            case 16:
                while (InputIndex < 24)
                {
                    if (!Spend(sizeof(FOrigin))) return Yield();
                    CurrentPlayer->DanceInputs[InputIndex++] = {};
                }
                CurrentPlayer->bDanceMerged = true;
                break;
            case 17: break;
            }
            if (S != EStep::Complete) return S;
            Comparison = 0; ++Subphase;
        }
        return EStep::Complete;
    }

    WorldObservationPrivate::EStep FWorldSession::FSemanticData::PinPlayer()
    {
        if (CurrentPlayer->bPinned) return EStep::Complete;
        if (!Spend(29 * 64)) return Yield();
        for (uint32 I = 0; I < 29; ++I)
        {
            const auto& Ref = CurrentPlayer->Reference(I);
            if (Ref.IsSet() && (!Ref.Owner || Ref.Owner->Owners > MAX_uint64 - 29)) return EStep::ResourceUnavailable;
        }
        for (uint32 I = 0; I < 29; ++I)
        {
            auto& Ref = CurrentPlayer->Reference(I);
            if (Ref.IsSet()) ++Ref.Owner->Owners;
        }
        CurrentPlayer->bPinned = true;
        return EStep::Complete;
    }

    WorldObservationPrivate::EStep FWorldSession::FSemanticData::ReadRacing()
    {
        using namespace WorldReadPrivate;
        while (Subphase < 4)
        {
            if (!Spend(64)) return Yield();
            EStep S = EStep::Complete; bool Same = true;
            switch (Subphase)
            {
            case 0:
                Racing = {Roots[3], false}; RaceInstance = {}; RaceTick = 0;
                if (RootPresence[3] != EWorldFieldPresence::Value) { Subphase = 4; return EStep::Complete; }
                if (Encoding[3] != 1) return EStep::Invalid;
                if (const auto* N = Resolve(*Storage, Roots[3]); N && N->Kind == EWorldValueKind::Null)
                { Subphase = 4; return EStep::Complete; }
                break;
            case 1:
                S = Read(EDomain::Racing, Roots[3], Room, true);
                if (S != EStep::Complete) return S;
                RaceInstance = Domain.RaceInstance(); RaceTick = Domain.RaceUpdatedAt();
                if (!RaceInstance.IsSet()) { Subphase = 4; return EStep::Complete; }
                break;
            case 2:
                S = Equal(RaceInstance, Instance.Root.IsSet() ? Instance.Fields[0] : Binding.Binding().Field(BindingPresentationInstance), Same);
                if (S != EStep::Complete) return S;
                if (!Same) return EStep::Invalid;
                break;
            case 3:
                if (Prior && Prior->bSemanticsComplete && Prior->RacingOwner && Prior->RaceInstanceNode)
                {
                    S = Equal(RaceInstance, {Prior->RaceInstanceNode, 3, Prior->RacingOwner}, Same);
                    if (S != EStep::Complete) return S;
                    if (RetainRacing(Same, RaceTick, Prior->RaceTick))
                    {
                        Racing = {{Prior->RacingNode, 3, Prior->RacingOwner}, true};
                        RaceInstance = {Prior->RaceInstanceNode, 3, Prior->RacingOwner}; RaceTick = Prior->RaceTick;
                    }
                }
                break;
            }
            if (S != EStep::Complete) return S;
            ++Subphase;
        }
        return EStep::Complete;
    }

    WorldObservationPrivate::EStep FWorldSession::FSemanticData::ReadRemaining()
    {
        using namespace WorldReadPrivate;
        using K = EWorldValueKind;
        const auto& B = Binding.Binding();
        const bool Presentation = Operation == EWorldRequest::Ready || Operation == EWorldRequest::PresentationFailed;
        while (Subphase < 14)
        {
            if (!Spend(128)) return Yield();
            EStep S = EStep::Complete; bool Same = true;
            switch (Subphase)
            {
            case 0:
                // Complete structural storage retains all nine encoded documents,
                // including untranslated mechanics and decoded-null documents.
                for (uint32 I = 0; I < 20; ++I)
                {
                    const bool Encoded = I == 2 || I == 3 || I == 4 || I == 5 || I == 6 || I == 7 || I == 8 || I == 9 || I == 12;
                    if (RootPresence[I] == EWorldFieldPresence::Value && Encoding[I] != static_cast<uint8>(Encoded)) return EStep::Invalid;
                }
                if (Operation == EWorldRequest::Observe && RootPresence[13] == EWorldFieldPresence::Missing) return EStep::Invalid;
                break;
            case 1:
                if (RootPresence[10] == EWorldFieldPresence::Value)
                {
                    static constexpr FTextChoice Boarding[] = {TEXT("available")};
                    static constexpr FTextChoice Policy[] = {TEXT("lagos-city-seat-fares-v1")};
                    static constexpr FFieldRule F[] = {FFieldRule(TEXT("boarding"), K::String).WithChoices(Boarding),
                        FFieldRule(TEXT("farePolicy"), K::String).WithChoices(Policy)};
                    static constexpr FRecordSchema Schema(F);
                    S = ReadSchema(Schema, Roots[10], Room);
                }
                break;
            case 2:
                if (RootPresence[11] == EWorldFieldPresence::Value)
                {
                    static constexpr FFieldRule F[] = {FFieldRule(TEXT("worldKey"), K::String), FFieldRule(TEXT("epochKey"), K::String),
                        FFieldRule(TEXT("gameRoomId"), K::String), FFieldRule(TEXT("instanceId"), K::String),
                        FFieldRule(TEXT("layoutRevision"), K::String), FFieldRule(TEXT("sharedStartedAt"), K::Number)};
                    static constexpr FRecordSchema Schema(F);
                    S = ReadSchema(Schema, Roots[11], Scope);
                }
                break;
            case 3:
                if (RootPresence[11] == EWorldFieldPresence::Value)
                {
                    if (!Instance.Root.IsSet() || Scope.Numbers[5] != Instance.Numbers[5]) return EStep::Invalid;
                    const FRef Expected[] = {B.Field(BindingGameRoom), Instance.Fields[0], Navigation.Fields[0]};
                    while (Comparison < 3)
                    {
                        S = Equal(Scope.Fields[Comparison + 2], Expected[Comparison], Same);
                        if (S != EStep::Complete) return S;
                        if (!Same) return EStep::Invalid;
                        ++Comparison;
                    }
                }
                break;
            case 4:
                if (RootPresence[13] == EWorldFieldPresence::Value)
                {
                    static constexpr FTextChoice Phases[] = {TEXT("checking"), TEXT("cleared"), TEXT("searching"), TEXT("detained")};
                    static constexpr FFieldRule F[] = {FFieldRule(TEXT("journeyId"), K::String), FFieldRule(TEXT("phase"), K::String).WithChoices(Phases)};
                    static constexpr FRecordSchema Schema(F);
                    S = ReadSchema(Schema, Roots[13], Room);
                }
                break;
            case 5:
                if (RootPresence[13] == EWorldFieldPresence::Value)
                {
                    if (!LocalPlayer.Root.IsSet() || LocalPlayer.Presence[12] != EWorldFieldPresence::Value) return EStep::Invalid;
                    if (Step == 0)
                    {
                        S = Read(EDomain::Journey, LocalPlayer.Fields[12], Journey);
                        if (S != EStep::Complete) return S;
                        Step = 1;
                    }
                    S = Equal(Room.Fields[0], Journey.Fields[0], Same);
                    if (S != EStep::Complete) return S;
                    if (!Same) return EStep::Invalid;
                    Step = 0;
                }
                break;
            case 6:
                if (RootPresence[14] == EWorldFieldPresence::Null) return EStep::Invalid;
                InputNode = 0; PeerIndex = 0;
                if (RootPresence[14] == EWorldFieldPresence::Value)
                {
                    const auto* A = Resolve(*Storage, Roots[14]);
                    if (!A || A->Kind != K::Array) return EStep::Invalid;
                    InputNode = A->FirstChild;
                }
                break;
            case 7:
                while (InputNode)
                {
                    if (!Spend(64)) return Yield();
                    const FRef Ref = Roots[14].Child(InputNode);
                    const auto* N = Resolve(*Storage, Ref);
                    if (!N || N->Slot != PeerIndex || N->Role != 3) return EStep::Invalid;
                    if (Step == 0)
                    {
                        static constexpr FFieldRule F[] = {FFieldRule(TEXT("userId"), K::String).WithText({1}),
                            FFieldRule(TEXT("name"), K::String), FFieldRule(TEXT("residence"), K::Object)};
                        static constexpr FRecordSchema Schema(F);
                        S = ReadSchema(Schema, Ref, Room);
                        if (S != EStep::Complete) return S;
                        Step = 1;
                    }
                    S = Read(EDomain::Residence, Room.Fields[2], Scope, true);
                    if (S != EStep::Complete) return S;
                    InputNode = N->NextSibling; ++PeerIndex; Step = 0;
                }
                break;
            case 8:
                if (B.Presence(BindingPendingHome) == EWorldFieldPresence::Value)
                {
                    if (Binding.PendingHome().Number(2) != B.Number(BindingRevision)) return EStep::Invalid;
                    S = Equal(Binding.PendingHome().Field(0), B.Field(BindingInstance), Same);
                    if (S != EStep::Complete) return S;
                    if (!Same) return EStep::Invalid;
                }
                break;
            case 9:
                if (Presentation)
                {
                    static constexpr FFieldRule F[] = {FFieldRule(TEXT("instanceId"), K::String).WithText({1}),
                        FFieldRule(TEXT("revision"), K::Number).WithNumber(0, MaxSafeInteger, true),
                        FFieldRule(TEXT("readyInstanceId"), K::String, true, true).WithText({1}), FFieldRule(TEXT("acknowledged"), K::Boolean)};
                    static constexpr FRecordSchema Schema(F);
                    if (RootPresence[19] != EWorldFieldPresence::Value) return EStep::Invalid;
                    S = ReadSchema(Schema, Roots[19], Room);
                }
                break;
            case 10:
                if (Presentation)
                {
                    if (Room.Presence[2] != B.Presence(BindingReadyInstance)) return EStep::Invalid;
                    if (Room.Presence[2] == EWorldFieldPresence::Value)
                    {
                        S = Equal(Room.Fields[2], B.Field(BindingReadyInstance), Same);
                        if (S != EStep::Complete) return S;
                        if (!Same) return EStep::Invalid;
                    }
                }
                break;
            case 11:
                if (Presentation)
                {
                    S = Equal(Room.Fields[0], B.Field(BindingPresentationInstance), Same);
                    if (S != EStep::Complete) return S;
                    bSameRound = Same && Room.Numbers[1] == B.Number(BindingRevision);
                }
                break;
            case 12:
                if (Presentation)
                {
                    if (Operation == EWorldRequest::PresentationFailed) bSameRound &= Room.Presence[2] == EWorldFieldPresence::Null;
                    else
                    {
                        if (Room.Presence[2] != EWorldFieldPresence::Value || B.Presence(BindingPendingHome) != EWorldFieldPresence::Null) Same = false;
                        else
                        {
                            S = Equal(Room.Fields[0], Room.Fields[2], Same);
                            if (S != EStep::Complete) return S;
                        }
                        bSameRound &= Same;
                    }
                    const auto* Ack = Resolve(*Storage, Room.Fields[3]);
                    if (!Ack || (Ack->Count != 0) != bSameRound) return EStep::Invalid;
                }
                break;
            case 13: break;
            }
            if (S != EStep::Complete) return S;
            Comparison = 0; ++Subphase;
        }
        return EStep::Complete;
    }

    FWorldSession::FWorldSession() = default;
    FWorldSession::~FWorldSession()
    {
        Clear();
        if (Semantic)
        {
            FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Reader = Semantic.Release()](float)
            {
                Reader->Storage->Budget->BeginTick();
                if (Reader->Retire() != WorldObservationPrivate::EStep::Complete) return true;
                if (!Reader->Storage->Budget->Spend(128, false)) return true;
                const auto Budget = Reader->Storage->Budget;
                delete Reader; Budget->Release(QualifiedSemanticDataBytes); return false;
            }));
        }
        for (auto& Storage : Retiring) WorldObservationPrivate::RetireDetached(MoveTemp(Storage));
        Recovery.Dispose();
    }
    FWorldSessionView FWorldSession::View() const
    {
        auto Out = State;
        Out.bHasObservation = Live() && Out.Observation.IsCurrent();
        if (!Out.bHasObservation) Out.Observation = {};
        // Imported-content evidence, public transport and acknowledged presentation
        // have no producers in this source assignment. Reads cannot grant walking.
        Out.bWalkingAvailable = false;
        return Out;
    }
    bool FWorldSession::Live() const
    {
        const double Now = FPlatformTime::Seconds();
        return bAttached && Entry.CredentialGeneration && Entry.EntrySequence > 0
            && std::isfinite(Now) && std::isfinite(Entry.Deadline) && Now < Entry.Deadline;
    }
    void FWorldSession::Attach() { bAttached = true; }
    void FWorldSession::Detach() { bAttached = false; Clear(); }
    bool FWorldSession::HasRetirement() const
    {
        if (Semantic && Semantic->bCancelled) return true;
        for (const auto& Storage : Retiring) if (Storage) return true;
        return false;
    }
    void FWorldSession::Retire(TSharedPtr<WorldObservationPrivate::FStorage> Storage)
    {
        if (!Storage) return;
        Storage->bComplete = false; Storage->bRetiring = true;
        for (const auto& Existing : Retiring) if (Existing == Storage) return;
        for (auto& Slot : Retiring)
            if (!Slot) { Slot = MoveTemp(Storage); return; }
        // New candidates are refused while this owner is occupied. No fourth
        // storage object may be admitted by this lifetime.
        checkNoEntry();
    }
    void FWorldSession::Clear(EWorldReadStatus Status)
    {
        if (Access) Access->Revoke();
        Access.Reset(); Transport.Reset();
        State.Observation = {}; State.bHasObservation = State.bWalkingAvailable = false;
        if (State.Generation != MAX_uint64) ++State.Generation;
        State.Status = Status;
        bInFlight = false; Request = {}; Next = CaptureOperation = EWorldRequest::None;
        Body.Reset(); DecodedBody.Reset();
        CancelSemantics();
        if (Pages) Retire(Pages->Cancel());
        Pages.Reset();
        if (Candidate) Retire(MoveTemp(Candidate));
        if (Accepted) Retire(MoveTemp(Accepted));
        Entry = {}; VisitId.Reset(); LifeId.Reset(); Preparation.Reset(); Capture.Reset(); RequestId.Reset();
        RevisionHighWater = MoveHighWater = -1;
        TicketArrival = CandidateDeadline = NextAt = 0;
        ReceiptExpiresAt = 0; bBindingValidated = false;
        bObserveAfterRetirement = false;
        // Outward health notification is last. Reentry sees no readable world.
        Recovery.Clear();
    }
    void FWorldSession::BeginEntry(const FWorldEntryIdentity& Identity)
    {
        Clear();
        if (!bAttached || State.Generation == MAX_uint64) return;
        Entry = Identity;
        if (Live()) BeginCapture(EWorldRequest::Enter);
    }
    void FWorldSession::Retry()
    {
        if (!Live() || bInFlight || Next != EWorldRequest::None || HasRetirement() || Candidate) return;
        // No mutation replay after an ambiguous completion. Only its retained result
        // may resolve the original operation until that lifetime is retired.
        if (bLostTicket && Command(CaptureOperation))
        { Next = EWorldRequest::Result; NextAt = FPlatformTime::Seconds(); return; }
        if (!VisitId.IsEmpty()) BeginCapture(EWorldRequest::Prepare);
    }
    bool FWorldSession::BeginCapture(EWorldRequest Operation)
    {
        // Movement/presentation request argument ownership is not implemented yet.
        // Never construct an incomplete command if another prerequisite is enabled.
        if (Operation != EWorldRequest::Enter && Operation != EWorldRequest::Prepare
            && Operation != EWorldRequest::Observe) return false;
        if (!Live() || HasRetirement() || Candidate || bInFlight || RequestSequence >= MaxCounter
            || (Command(Operation) && OperationSequence >= MaxCounter)) return false;
        if (!WorldObservationPrivate::FPages::ResourcesQualified())
        { State.Status = EWorldReadStatus::ResourceUnavailable; return false; }
        if (!Budget) Budget = MakeShared<WorldObservationPrivate::FBudget>();
        Budget->CandidateWork = 0; Budget->bExhausted = false;
        Pages = MakeUnique<WorldObservationPrivate::FPages>(Budget.ToSharedRef());
        ++RequestSequence;
        if (Command(Operation)) ++OperationSequence;
        RequestId = FGuid::NewGuid().ToString(EGuidFormats::Digits);
        Capture.Reset(); CaptureOperation = Operation; Next = Operation;
        TicketArrival = 0; CandidateDeadline = Entry.Deadline;
        bLostTicket = bPageRetried = bBindingValidated = false;
        NextAt = FPlatformTime::Seconds();
        State.Status = Operation == EWorldRequest::Prepare ? EWorldReadStatus::Preparing : EWorldReadStatus::Capturing;
        return true;
    }
    bool FWorldSession::Current(const FWorldDispatch& Dispatch) const
    {
        return Live() && bInFlight && Dispatch.Lifetime == State.Generation
            && Dispatch.Serial == Request.Serial && Dispatch.Operation == Request.Operation;
    }
    bool FWorldSession::TakeDispatch(FWorldDispatch& Out)
    {
        Out = {};
        if (!Live() || bInFlight || !Body.IsEmpty() || DecodedBody || Next == EWorldRequest::None
            || FPlatformTime::Seconds() < NextAt || DispatchSerial == MAX_uint64) return false;
        const auto Args = MakeShared<FJsonObject>();
        Args->SetNumberField(TEXT("entrySequence"), Entry.EntrySequence);
        if (Next == CaptureOperation || Next == EWorldRequest::Result)
        {
            Args->SetStringField(TEXT("requestId"), RequestId);
            Args->SetNumberField(TEXT("requestSequence"), static_cast<double>(RequestSequence));
            if (Next != EWorldRequest::Result)
            {
                if (Command(Next)) Args->SetNumberField(TEXT("operationSequence"), static_cast<double>(OperationSequence));
                if (Next != EWorldRequest::Enter)
                { Args->SetStringField(TEXT("visitId"), VisitId); Args->SetStringField(TEXT("lifeId"), LifeId); }
                if (Next == EWorldRequest::Observe) Args->SetStringField(TEXT("preparation"), Preparation);
            }
        }
        else
        {
            Args->SetStringField(TEXT("capture"), Capture);
            if (Next == EWorldRequest::Page)
            {
                FString Root; uint64 Ordinal = 0;
                if (!Pages || !Pages->Next(Root, Ordinal)) return false;
                Args->SetArrayField(TEXT("cursor"), {MakeShared<FJsonValueString>(Root),
                    MakeShared<FJsonValueNumber>(static_cast<double>(Ordinal))});
            }
        }
        Request = {Next, State.Generation, ++DispatchSerial, Args};
        Next = EWorldRequest::None; bInFlight = true; Out = Request;
        return true;
    }
    void FWorldSession::Complete(const FWorldDispatch& Dispatch, FString InBody, double S, double R)
    {
        if (!Current(Dispatch)) return;
        bInFlight = false;
        if (!std::isfinite(S) || !std::isfinite(R) || R < S || R > FPlatformTime::Seconds()
            || InBody.IsEmpty() || InBody.Len() > WorldObservationPrivate::BodyLimit)
        { Unavailable(EWorldReadStatus::InvalidResponse); return; }
        SentAt = S; ReceivedAt = R; Body = MoveTemp(InBody);
    }
    void FWorldSession::Failed(const FWorldDispatch& Dispatch, bool bTransportUnavailable)
    {
        if (!Current(Dispatch)) return;
        bInFlight = false;
        if (bTransportUnavailable) { Unavailable(EWorldReadStatus::Unavailable); return; }
        if (Command(Dispatch.Operation))
        {
            bLostTicket = true;
            Next = EWorldRequest::Result; NextAt = FPlatformTime::Seconds() + 1;
        }
        else if (Dispatch.Operation == EWorldRequest::Page && !bPageRetried)
        {
            bPageRetried = true;
            Next = EWorldRequest::Page; NextAt = FPlatformTime::Seconds() + 1;
        }
        else Unavailable(EWorldReadStatus::Unavailable);
    }
    void FWorldSession::Unavailable(EWorldReadStatus Status)
    {
        bInFlight = false; Next = EWorldRequest::None;
        bObserveAfterRetirement = false;
        Body.Reset(); DecodedBody.Reset();
        CancelSemantics();
        if (Pages) Retire(Pages->Cancel());
        Pages.Reset();
        if (Candidate) Retire(MoveTemp(Candidate));
        State.Status = Status;
        if (!Live() || !Access || !Access->Current())
        {
            if (Access) Access->Revoke();
            State.Observation = {}; State.bHasObservation = false;
        }
    }
    void FWorldSession::Tick()
    {
        if (Budget) Budget->BeginTick();
        RetireSemantics();
        for (auto& Storage : Retiring)
        {
            if (!Storage) continue;
            if (Storage->Retire() == WorldObservationPrivate::EStep::Complete) Storage.Reset();
            break;
        }
        if (!Live())
        {
            if (Entry.CredentialGeneration) Clear(EWorldReadStatus::Expired);
            return;
        }
        if (bObserveAfterRetirement && !HasRetirement())
        {
            bObserveAfterRetirement = false;
            BeginCapture(EWorldRequest::Observe);
        }
        const double Now = FPlatformTime::Seconds();
        if (CandidateDeadline > 0 && (!std::isfinite(Now) || Now >= CandidateDeadline))
        { Unavailable(EWorldReadStatus::Expired); return; }
        if (!Body.IsEmpty() || DecodedBody) ProcessBody();
        else if (Candidate && !bInFlight && Next == EWorldRequest::None) AdvanceSemantics();
        else if (Pages && !bInFlight && Next == EWorldRequest::None) AdvancePages();
    }
    void FWorldSession::ProcessBody()
    {
        using namespace StateJsonPrivate;
        using namespace WorldObservationPrivate;
        if (!Budget) { Unavailable(EWorldReadStatus::ResourceUnavailable); return; }
        if (!DecodedBody)
        {
            if (!Budget->ReservePage()) return;
            if (!FPages::DecodeBody(Body, DecodedBody, *Budget))
            { Unavailable(Budget->bExhausted ? EWorldReadStatus::ResourceUnavailable : EWorldReadStatus::InvalidResponse); return; }
            Body.Reset();
        }
        const auto O = Object(Field(DecodedBody, TEXT("value")));
        FString Status, Outer;
        if (!Exact(DecodedBody, {TEXT("status"), TEXT("value")}) || !Text(DecodedBody, TEXT("status"), Outer)
            || !Same(Outer, TEXT("success")) || !Text(O, TEXT("status"), Status))
        { Unavailable(EWorldReadStatus::InvalidResponse); return; }
        if (Same(Status, TEXT("slow_down")))
        {
            int64 Delay = 0;
            if (!Exact(O, {TEXT("status"), TEXT("retryAfterMs")}) || !Integer(Field(O, TEXT("retryAfterMs")), Delay)
                || Delay <= 0 || Delay > 900000) { Unavailable(EWorldReadStatus::InvalidResponse); return; }
            // Mutations are resolved by original identity even after throttling.
            Next = Command(Request.Operation) ? EWorldRequest::Result : Request.Operation;
            NextAt = ReceivedAt + static_cast<double>(Delay) / 1000;
            DecodedBody.Reset(); Budget->ReleasePage(); return;
        }
        if (Same(Status, TEXT("pending")) || Same(Status, TEXT("captured")))
        {
            FString Alias, Generation, Id, CaptureKind; int64 V = 0, Seq = 0, RetryMs = 0;
            if (!Exact(O, {TEXT("v"), TEXT("generation"), TEXT("capture"), TEXT("status"), TEXT("kind"),
                    TEXT("requestId"), TEXT("requestSequence"), TEXT("retryAfterMs")})
                || !Integer(Field(O, TEXT("v")), V) || V != 1 || !Text(O, TEXT("generation"), Generation)
                || !Same(Generation, ProtocolGeneration) || !Text(O, TEXT("capture"), Alias) || !Hex(Alias, 32)
                || !Text(O, TEXT("requestId"), Id) || !Same(Id, RequestId)
                || !Integer(Field(O, TEXT("requestSequence")), Seq) || static_cast<uint64>(Seq) != RequestSequence
                || !Text(O, TEXT("kind"), CaptureKind) || !Same(CaptureKind, Kind(CaptureOperation))
                || !Integer(Field(O, TEXT("retryAfterMs")), RetryMs) || RetryMs > 900000
                || (!Capture.IsEmpty() && !Same(Capture, Alias)))
            { Unavailable(EWorldReadStatus::InvalidResponse); return; }
            Capture = MoveTemp(Alias);
            // Only the original capture-producing completion anchors its serverNow.
            // Manifest polling and retained-result lookup never fill this field.
            if (Request.Operation == CaptureOperation && !bLostTicket)
                TicketArrival = TicketArrival > 0 ? FMath::Min(TicketArrival, ReceivedAt) : ReceivedAt;
            if (TicketArrival > 0 && CaptureOperation != EWorldRequest::Prepare)
                CandidateDeadline = FMath::Min(CandidateDeadline, TicketArrival + 60);
            Next = EWorldRequest::Manifest;
            NextAt = ReceivedAt + static_cast<double>(RetryMs) / 1000;
            DecodedBody.Reset(); Budget->ReleasePage(); return;
        }
        if (Request.Operation == EWorldRequest::Manifest && Same(Status, TEXT("manifest")))
        {
            const EStep Step = Pages ? Pages->Begin(DecodedBody, Capture, Kind(CaptureOperation)) : EStep::Invalid;
            if (Step == EStep::Invalid || Step == EStep::ResourceUnavailable)
            { Unavailable(Step == EStep::Invalid ? EWorldReadStatus::InvalidResponse : EWorldReadStatus::ResourceUnavailable); return; }
            if (Pages->Manifest()) { DecodedBody.Reset(); State.Status = EWorldReadStatus::Receiving; }
            return;
        }
        if (Request.Operation == EWorldRequest::Page && Same(Status, TEXT("page")))
        {
            const EStep Step = Pages ? Pages->Consume(DecodedBody, 0) : EStep::Invalid;
            DecodedBody.Reset();
            if (Step == EStep::Invalid || Step == EStep::ResourceUnavailable)
                Unavailable(Step == EStep::Invalid ? EWorldReadStatus::InvalidResponse : EWorldReadStatus::ResourceUnavailable);
            else bPageRetried = false;
            return;
        }
        if (Request.Operation == EWorldRequest::Bind && Same(Status, TEXT("bound")))
        {
            const bool bAccepted = AcceptBinding(O);
            DecodedBody.Reset(); Budget->ReleasePage();
            if (!bAccepted) Unavailable(EWorldReadStatus::InvalidResponse);
            return;
        }
        // Denial/loss is not a transient read error and cannot keep an old visit.
        if (Same(Status, TEXT("denied")) || Same(Status, TEXT("entry_lost"))
            || Same(Status, TEXT("world_lost")) || Same(Status, TEXT("selection_changed")))
        { Clear(); return; }
        Unavailable(Same(Status, TEXT("resource_unavailable"))
            ? EWorldReadStatus::ResourceUnavailable : EWorldReadStatus::InvalidResponse);
    }
    void FWorldSession::AdvancePages()
    {
        using namespace WorldObservationPrivate;
        const EStep Step = Pages->Advance();
        if (Step == EStep::Invalid || Step == EStep::ResourceUnavailable)
        { Unavailable(Step == EStep::Invalid ? EWorldReadStatus::InvalidResponse : EWorldReadStatus::ResourceUnavailable); return; }
        FString Root; uint64 Ordinal = 0;
        if (Pages->Next(Root, Ordinal)) { Next = EWorldRequest::Page; NextAt = FPlatformTime::Seconds(); return; }
        if (Step != EStep::Complete) return;
        Candidate = Pages->TakeComplete();
        AdvanceSemantics();
    }
    void FWorldSession::CancelSemantics()
    {
        bBindingValidated = false;
        if (Semantic)
        {
            Semantic->bCancelled = true;
            // Borrowed cursors become unreadable at this flag. Their trivial
            // storage is released by the budgeted retirement, not reset here.
        }
    }
    bool FWorldSession::RetireSemantics()
    {
        if (!Semantic || !Semantic->bCancelled) return true;
        if (Semantic->Retire() != WorldObservationPrivate::EStep::Complete) return false;
        if (!Semantic->Storage->Budget->Spend(128, false)) return false;
        const auto OwnerBudget = Semantic->Storage->Budget;
        Semantic.Reset(); OwnerBudget->Release(QualifiedSemanticDataBytes); return true;
    }
    void FWorldSession::AdvanceSemantics()
    {
        using namespace WorldObservationPrivate;
        const EStep Validated = ValidateCandidate();
        if (Validated == EStep::Pending) return;
        if (Validated != EStep::Complete)
        {
            Retire(MoveTemp(Candidate));
            Unavailable(Validated == EStep::ResourceUnavailable ? EWorldReadStatus::ResourceUnavailable : EWorldReadStatus::InvalidResponse);
            return;
        }
        if (CaptureOperation == EWorldRequest::Prepare)
        {
            if (!Budget->Spend(128)) return;
            Preparation = Capture;
            CancelSemantics();
            Retire(MoveTemp(Candidate));
            // Exactly one fresh dynamic capture follows this preparation. Failure
            // requires explicit reload; static bytes never create a clock sample.
            bObserveAfterRetirement = true;
            State.Status = EWorldReadStatus::Preparing;
            return;
        }
        State.Status = EWorldReadStatus::Binding; Next = EWorldRequest::Bind;
    }
    WorldObservationPrivate::EStep FWorldSession::ValidateCandidate()
    {
        using namespace WorldObservationPrivate;
        using namespace WorldReadPrivate;
        if (!Live() || !Candidate || !Candidate->bComplete || Candidate->bRetiring || !Budget
            || Candidate->Budget != Budget || (Accepted && Accepted->Budget != Budget)) return EStep::Invalid;
        if (CaptureOperation != EWorldRequest::Enter && (VisitId.IsEmpty() || LifeId.IsEmpty())) return EStep::Invalid;
        if (!Semantic)
        {
            if (!QualifiedSemanticDataBytes) return EStep::ResourceUnavailable;
            const uint64 Initialization = sizeof(FSemanticData)
                - 23 * (sizeof(FSemanticData::FRecord) - sizeof(FSemanticData::FRef))
                - FDomainRead::DeferredBytes();
            if (Initialization > TickWorkLimit) return EStep::ResourceUnavailable;
            if (!Budget->Spend(Initialization)) return Budget->bExhausted ? EStep::ResourceUnavailable : EStep::Pending;
            if (!Budget->Reserve(QualifiedSemanticDataBytes)) return EStep::ResourceUnavailable;
            Semantic = MakeUnique<FSemanticData>();
            Semantic->Storage = Candidate; Semantic->Prior = Accepted; Semantic->Generation = State.Generation;
            Semantic->bPrepare = CaptureOperation == EWorldRequest::Prepare;
            Semantic->Operation = CaptureOperation;
        }
        auto& D = *Semantic;
        auto CurrentCandidate = [&]() { return Live() && !D.bCancelled && D.Storage == Candidate && D.Prior == Accepted
            && D.Generation == State.Generation && Candidate->bComplete && !Candidate->bRetiring
            && std::isfinite(CandidateDeadline) && FPlatformTime::Seconds() < CandidateDeadline
            && (!Accepted || (Accepted->bComplete && !Accepted->bRetiring && Accepted->bSemanticsComplete
                && Accepted->SemanticGeneration == State.Generation)); };
        auto CurrentHighWater = [&]()
        {
            const auto& B = D.Binding.Binding();
            const int64 Revision = FMath::Max(RevisionHighWater, Accepted ? Accepted->PlayerRevision : int64(-1));
            const int64 Sequence = FMath::Max(MoveHighWater, Accepted ? Accepted->MoveSequence : int64(-1));
            return B.Number(BindingRevision) >= Revision
                && (B.Number(BindingRevision) != Revision || B.Number(BindingSequence) >= Sequence)
                && (!Accepted || (B.Number(BindingHealthRevision) >= Accepted->HealthRevision
                    && B.Number(BindingSavedRevision) >= Accepted->SavedRevision));
        };
        auto FinishStep = [&](EStep Step) { if (Step != EStep::Pending) D.Result = Step; return Step; };
        if (!CurrentCandidate()) return EStep::Invalid;
        if (D.Result != EStep::Pending)
        {
            if (!D.Spend(128)) return D.Yield();
            return D.Result == EStep::Complete && !CurrentHighWater() ? EStep::Invalid : D.Result;
        }
        while (CurrentCandidate())
        {
            if (!D.Spend(512)) return D.Yield();
            EStep Step = EStep::Complete; bool Same = false;
            const auto& B = D.Binding.Binding(); const auto& R = D.Binding.Receipt(); const auto& V = D.Binding.Visit();
            if (D.Phase == 0)
            {
                if (!D.Spend(20 * 64 + sizeof(FBindingRead))) return D.Yield();
                for (int32 I = 0; I < Candidate->Manifest.Count; ++I)
                {
                    const auto& E = Candidate->Manifest.Entries[I];
                    D.RootPresence[E.Slot] = static_cast<EWorldFieldPresence>(E.Presence); D.Encoding[E.Slot] = E.Encoding;
                    if (E.Presence == 2) D.Roots[E.Slot] = {1, E.Slot, Candidate->Roots[E.Slot]};
                }
                if (D.RootPresence[17] != EWorldFieldPresence::Value) return FinishStep(EStep::Invalid);
                D.Binding.Begin(D.Roots[17]); D.Phase = 1;
            }
            if (D.Phase == 1)
            {
                Step = D.Binding.Advance(*Candidate);
                if (Step != EStep::Complete) return FinishStep(Step);
                D.Phase = 2;
            }
            if (D.Phase == 2)
            {
                if (!D.Spend(10 * (sizeof(FValueRef) + sizeof(FString*)))) return D.Yield();
                const FValueRef Refs[] = {R.Field(ReceiptGrant), B.Field(BindingUser), B.Field(BindingCharacter), B.Field(BindingHome),
                    B.Field(BindingGameRoom), B.Field(BindingParticipant), B.Field(BindingCall), B.Field(BindingExpression),
                    B.Field(BindingApprover), B.Field(BindingReservation)};
                const FString* Owned[] = {&Entry.GrantId, &Entry.UserId, &Entry.CharacterId, &Entry.HomeRoomId, &Entry.GameRoomId,
                    &Entry.ParticipantId, &Entry.CallSessionId, &Entry.ExpressionSessionId, &Entry.ApprovingSessionId, &Entry.ReservationGeneration};
                while (D.Comparison < UE_ARRAY_COUNT(Refs))
                {
                    Step = D.Owner(Refs[D.Comparison], *Owned[D.Comparison], Same);
                    if (Step != EStep::Complete) return FinishStep(Step);
                    if (!Same) return FinishStep(EStep::Invalid);
                    ++D.Comparison;
                }
                if (!D.Spend(64)) return D.Yield();
                const auto& M = Candidate->Manifest;
                if (R.Number(ReceiptEntry) != Entry.EntrySequence || V.Number(VisitRevision) != B.Number(BindingRevision)
                    || B.Number(BindingCapturedAt) != M.CapturedAt || B.Number(BindingPermissionDeadline) != M.PermissionDeadline
                    || (B.Presence(BindingPublishBy) == EWorldFieldPresence::Value) != M.PublishBy.IsSet()
                    || (M.PublishBy.IsSet() && B.Number(BindingPublishBy) != M.PublishBy.GetValue())
                    || R.Number(ReceiptExpires) <= R.Number(ReceiptNow) || M.PermissionDeadline <= M.CapturedAt
                    || (M.PublishBy.IsSet() && M.PublishBy.GetValue() <= M.CapturedAt)
                    || !CurrentHighWater()) return FinishStep(EStep::Invalid);
                D.Comparison = 0; D.Phase = 3;
            }
            if (D.Phase == 3)
            {
                if (D.Comparison == 0)
                {
                    Step = D.Equal(V.Field(VisitInstance), B.Field(BindingInstance), Same);
                    if (Step != EStep::Complete) return FinishStep(Step);
                    if (!Same) return FinishStep(EStep::Invalid);
                    ++D.Comparison;
                }
                if (D.Comparison == 1 && !VisitId.IsEmpty())
                {
                    Step = D.Owner(V.Field(WorldReadPrivate::VisitId), VisitId, Same);
                    if (Step != EStep::Complete) return FinishStep(Step);
                    if (!Same) return FinishStep(EStep::Invalid);
                    ++D.Comparison;
                }
                if (!LifeId.IsEmpty())
                {
                    Step = D.Owner(V.Field(VisitLife), LifeId, Same);
                    if (Step != EStep::Complete) return FinishStep(Step);
                    if (!Same) return FinishStep(EStep::Invalid);
                }
                D.Comparison = 0; D.Phase = 4;
            }
            if (D.Phase == 4)
            {
                if (D.bPrepare || CaptureOperation == EWorldRequest::Observe)
                {
                    if (D.RootPresence[16] != EWorldFieldPresence::Value) return FinishStep(EStep::Invalid);
                    Step = D.Read(EDomain::Selection, D.Roots[16], D.Room, true);
                    if (Step != EStep::Complete) return FinishStep(Step);
                    if (D.Room.Numbers[0] != B.Number(BindingSavedRevision)) return FinishStep(EStep::Invalid);
                    D.Selection = D.Roots[16]; D.SelectedCharacter = D.Domain.Character();
                    D.SelectedHome = D.Domain.Home(); D.SelectedActiveRoom = D.Domain.ActiveRoom();
                    D.Library.Root = D.Domain.Library(); D.Phase = 5;
                }
                else D.Phase = 7;
            }
            if (D.Phase == 5)
            {
                Step = D.Read(EDomain::Library, D.Library.Root, D.Library);
                if (Step != EStep::Complete) return FinishStep(Step);
                if (D.Library.Counts[4] != 1 || D.Library.Counts[5] != 1) return FinishStep(EStep::Invalid);
                D.Phase = 6;
            }
            if (D.Phase == 6)
            {
                static constexpr uint32 Fields[] = {1, 2, 3};
                static constexpr uint32 BindingFields[] = {BindingCharacter, BindingHome, BindingHome};
                while (D.Comparison < 3)
                {
                    Step = D.Equal(D.Library.Fields[Fields[D.Comparison]], B.Field(BindingFields[D.Comparison]), Same);
                    if (Step != EStep::Complete) return FinishStep(Step);
                    if (!Same) return FinishStep(EStep::Invalid);
                    ++D.Comparison;
                }
                D.Comparison = 0; D.Phase = 7;
            }
            if (D.Phase == 7)
            {
                if (CaptureOperation == EWorldRequest::Observe)
                {
                    if (D.RootPresence[15] != EWorldFieldPresence::Value) return FinishStep(EStep::Invalid);
                    if (!D.bRecordWork)
                    {
                        if (!D.Spend(sizeof(FHealthRead))) return D.Yield();
                        D.Health.Begin(D.Roots[15]); D.bRecordWork = true;
                    }
                    Step = D.Health.Advance(*Candidate);
                    if (Step != EStep::Complete) return FinishStep(Step);
                    if (D.Health.IsNull() || D.Health.Revision() != B.Number(BindingHealthRevision)) return FinishStep(EStep::Invalid);
                    Step = D.Equal(D.Health.Life(), V.Field(VisitLife), Same);
                    if (Step != EStep::Complete) return FinishStep(Step);
                    if (!Same) return FinishStep(EStep::Invalid);
                    D.bRecordWork = false;
                }
                D.Phase = 8;
            }
            if (D.Phase == 8)
            {
                if (D.bPrepare) { D.Phase = 11; continue; }
                if (CaptureOperation == EWorldRequest::Ready || CaptureOperation == EWorldRequest::PresentationFailed)
                { D.Phase = 20; continue; }
                if (CaptureOperation == EWorldRequest::Observe)
                {
                    if (D.RootPresence[0] != EWorldFieldPresence::Value) return FinishStep(EStep::Invalid);
                    D.Player.Root = D.Roots[0]; D.Phase = 9;
                }
                else
                {
                    static constexpr FFieldRule Fields[] = {FFieldRule(TEXT("player"), EWorldValueKind::Object),
                        FFieldRule(TEXT("serverNow"), EWorldValueKind::Number).WithNumber(1, MaxSafeInteger, true)};
                    static constexpr FRecordSchema Schema(Fields);
                    if (D.RootPresence[18] != EWorldFieldPresence::Value) return FinishStep(EStep::Invalid);
                    Step = D.ReadSchema(Schema, D.Roots[18], D.Room);
                    if (Step != EStep::Complete) return FinishStep(Step);
                    D.Player.Root = D.Room.Fields[0]; D.Phase = 9;
                }
            }
            if (D.Phase == 9)
            {
                Step = D.Read(EDomain::Player, D.Player.Root, D.Player, true);
                if (Step != EStep::Complete) return FinishStep(Step);
                if (D.Player.Numbers[6] != B.Number(BindingRevision) || D.Player.Numbers[7] != B.Number(BindingSequence)) return FinishStep(EStep::Invalid);
                D.Phase = 10;
            }
            if (D.Phase == 10)
            {
                static constexpr uint32 Fields[] = {0, 1, 2, 4, 5};
                static constexpr uint32 BindingFields[] = {BindingUser, BindingCharacter, BindingGameRoom, BindingInstance, BindingHome};
                while (D.Comparison < 5)
                {
                    Step = D.Equal(D.Player.Fields[Fields[D.Comparison]], B.Field(BindingFields[D.Comparison]), Same);
                    if (Step != EStep::Complete) return FinishStep(Step);
                    if (!Same) return FinishStep(EStep::Invalid);
                    ++D.Comparison;
                }
                D.Comparison = 0; D.Phase = 11;
                if (!D.Spend(2 * sizeof(D.LocalPlayer))) { D.Phase = 10; D.Comparison = 5; return D.Yield(); }
                D.LocalPlayer = D.Player;
            }
            if (D.Phase == 11)
            {
                if (!D.bPrepare && CaptureOperation != EWorldRequest::Observe) { D.Phase = 13; continue; }
                if (D.RootPresence[2] != EWorldFieldPresence::Value || D.Encoding[2] != 1) return FinishStep(EStep::Invalid);
                Step = D.Read(EDomain::Instance, D.Roots[2], D.Instance, true);
                if (Step != EStep::Complete) return FinishStep(Step);
                D.Phase = 12;
            }
            if (D.Phase == 12)
            {
                Step = D.InstanceIdentity(Entry);
                if (Step != EStep::Complete) return FinishStep(Step);
                Step = D.Equal(D.Instance.Fields[0], B.Field(BindingPresentationInstance), Same);
                if (Step != EStep::Complete) return FinishStep(Step);
                if (!Same) return FinishStep(EStep::Invalid);
                D.Subphase = D.Comparison = 0;
                D.Phase = D.bPrepare ? 20 : 13;
            }
            if (D.Phase == 13)
            {
                Step = D.AllocatePlayer(D.Player.Root);
                if (Step != EStep::Complete) return FinishStep(Step);
                D.Phase = 14;
            }
            if (D.Phase == 14)
            {
                Step = D.Reconcile();
                if (Step != EStep::Complete) return FinishStep(Step);
                D.Subphase = D.Comparison = 0; D.Phase = 15;
            }
            if (D.Phase == 15)
            {
                Step = D.RetainActivity();
                if (Step != EStep::Complete) return FinishStep(Step);
                D.Phase = 22;
            }
            if (D.Phase == 22)
            {
                Step = D.PinPlayer();
                if (Step != EStep::Complete) return FinishStep(Step);
                D.Subphase = D.Comparison = 0; D.Phase = CaptureOperation == EWorldRequest::Observe ? 16 : 19;
            }
            if (D.Phase == 16)
            {
                if (!D.Spend(24)) return D.Yield();
                if (D.bLocal)
                {
                    if (D.RootPresence[1] != EWorldFieldPresence::Value) return FinishStep(EStep::Invalid);
                    const auto* Peers = Candidate->Get(1, 1);
                    if (!Peers || Peers->Kind != EWorldValueKind::Array) return FinishStep(EStep::Invalid);
                    D.Peer = Peers->FirstChild; D.PeerIndex = 0; D.bLocal = false;
                }
                if (!D.Peer) { D.Phase = 19; continue; }
                const auto* Peer = Candidate->Get(1, D.Peer);
                if (!Peer || Peer->Slot != D.PeerIndex || Peer->Role != 3) return FinishStep(EStep::Invalid);
                D.Player.Root = D.Roots[1].Child(D.Peer); D.Phase = 17;
            }
            if (D.Phase == 17)
            {
                Step = D.Read(EDomain::Player, D.Player.Root, D.Player, true);
                if (Step != EStep::Complete) return FinishStep(Step);
                D.Phase = 18;
            }
            if (D.Phase == 18)
            {
                if (!D.Spend(24)) return D.Yield();
                const auto* Peer = Candidate->Get(1, D.Peer);
                D.Peer = Peer->NextSibling; ++D.PeerIndex;
                D.Phase = 13;
                continue;
            }
            if (D.Phase == 19)
            {
                Step = D.ReadRacing();
                if (Step != EStep::Complete) return FinishStep(Step);
                D.Subphase = D.Comparison = 0; D.Phase = 20;
            }
            if (D.Phase == 20)
            {
                Step = D.ReadRemaining();
                if (Step != EStep::Complete) return FinishStep(Step);
                D.Phase = 21;
            }
            if (D.Phase == 21)
            {
                if (!D.Spend(256)) return D.Yield();
                if (!CurrentCandidate()) return FinishStep(EStep::Invalid);
                if (!CurrentHighWater()) return FinishStep(EStep::Invalid);
                if (D.Racing.Ref.IsSet() && (!D.Racing.Ref.Owner || D.Racing.Ref.Owner->Owners == MAX_uint64)) return FinishStep(EStep::ResourceUnavailable);
                Candidate->Players = D.Players; D.Players = D.Tail = D.CurrentPlayer = nullptr;
                if (D.Racing.Ref.IsSet())
                {
                    Candidate->RacingOwner = D.Racing.Ref.Owner; ++Candidate->RacingOwner->Owners;
                    Candidate->RacingNode = D.Racing.Ref.Node; Candidate->RaceInstanceNode = D.RaceInstance.Node;
                    Candidate->RaceTick = D.RaceTick;
                }
                ReceiptExpiresAt = static_cast<int64>(R.Number(ReceiptExpires));
                bBindingValidated = true;
                Candidate->bSemanticsComplete = true;
                Candidate->SemanticGeneration = State.Generation;
                Candidate->PlayerRevision = static_cast<int64>(B.Number(BindingRevision));
                Candidate->MoveSequence = static_cast<int64>(B.Number(BindingSequence));
                Candidate->HealthRevision = static_cast<int64>(B.Number(BindingHealthRevision));
                Candidate->SavedRevision = static_cast<int64>(B.Number(BindingSavedRevision));
                D.Result = EStep::Complete;
                return EStep::Complete;
            }
        }
        return EStep::Invalid;
    }
    bool FWorldSession::AcceptBinding(const TSharedPtr<FJsonObject>& O)
    {
        using namespace StateJsonPrivate;
        using namespace WorldObservationPrivate;
        if (!Live() || !Candidate || !bBindingValidated || TicketArrival <= 0 || ReceiptExpiresAt <= 0) return false;
        const auto& M = Candidate->Manifest;
        FString Alias, Digest, Generation; int64 V = 0, At = 0, Permission = 0, Publish = 0, Checked = 0;
        if (!Exact(O, {TEXT("v"), TEXT("generation"), TEXT("capture"), TEXT("status"), TEXT("directoryDigest"),
                TEXT("capturedAt"), TEXT("permissionDeadline"), TEXT("publishBy"), TEXT("checkedAt")})
            || !Integer(Field(O, TEXT("v")), V) || V != 1 || !Text(O, TEXT("generation"), Generation)
            || !Same(Generation, ProtocolGeneration) || !Text(O, TEXT("capture"), Alias) || !Same(Alias, M.Capture)
            || !Text(O, TEXT("directoryDigest"), Digest) || !Same(Digest, M.Digest)
            || !Integer(Field(O, TEXT("capturedAt")), At) || At != M.CapturedAt
            || !Integer(Field(O, TEXT("permissionDeadline")), Permission) || Permission != M.PermissionDeadline
            || !Integer(Field(O, TEXT("publishBy")), Publish) || !M.PublishBy.IsSet() || Publish != M.PublishBy.GetValue()
            || !Integer(Field(O, TEXT("checkedAt")), Checked) || Checked < At
            || !std::isfinite(SentAt) || !std::isfinite(ReceivedAt) || ReceivedAt < SentAt) return false;
        const double RemainingMs = static_cast<double>(FMath::Min(FMath::Min(Permission, Publish), ReceiptExpiresAt) - Checked)
            - std::ceil((ReceivedAt - SentAt) * 1000);
        if (!std::isfinite(RemainingMs) || RemainingMs <= 0) return false;
        CandidateDeadline = FMath::Min(CandidateDeadline, FMath::Min(Entry.Deadline, ReceivedAt + RemainingMs / 1000));
        // Final binding alone is not publication. Typed identity, high-water and
        // content checks still have to commit an immutable revocable observation.
        return FPlatformTime::Seconds() < CandidateDeadline;
    }
    bool FWorldSession::Walk(const FMovementPoint&, EMovementPace)
    {
        // No ordinary movement admission exists yet. In particular a null route,
        // valid page chain or same-room Home read cannot supply it.
        return false;
    }
    void FWorldSession::StopWalking() { State.bWalkingAvailable = false; }
    void FWorldSession::ContentLost(uint64 ObservationGeneration)
    {
        if (ObservationGeneration != State.Observation.Generation()) return;
        StopWalking();
        State.Status = EWorldReadStatus::ContentUnavailable;
    }
}
