#include "BaddieCompanionWorldSession.h"
#include "BaddieCompanionStateJson.h"
#include "BaddieCompanionWorldPlayerRead.h"
#include "HAL/PlatformTime.h"
#include "Containers/Ticker.h"

#include <cmath>

namespace BaddieCompanion
{
    struct FWorldDispatchArgumentsOwner
    {
        FJsonObject Arguments;
        uint64 References = 0;
    };
    FWorldDispatchArguments::FWorldDispatchArguments(FWorldDispatchArgumentsOwner* InOwner)
    {
        check(IsInGameThread());
        if (InOwner && InOwner->References != MAX_uint64) { Owner = InOwner; ++Owner->References; }
    }
    FWorldDispatchArguments::FWorldDispatchArguments(const FWorldDispatchArguments& Other)
        : FWorldDispatchArguments(Other.Owner) {}
    FWorldDispatchArguments::FWorldDispatchArguments(FWorldDispatchArguments&& Other) noexcept
        : Owner(Other.Owner) { Other.Owner = nullptr; }
    FWorldDispatchArguments& FWorldDispatchArguments::operator=(const FWorldDispatchArguments& Other)
    {
        if (this == &Other) return *this;
        FWorldDispatchArguments Next(Other); Reset(); Owner = Next.Owner; Next.Owner = nullptr; return *this;
    }
    FWorldDispatchArguments& FWorldDispatchArguments::operator=(FWorldDispatchArguments&& Other) noexcept
    {
        if (this == &Other) return *this;
        Reset(); Owner = Other.Owner; Other.Owner = nullptr; return *this;
    }
    FWorldDispatchArguments::~FWorldDispatchArguments() { Reset(); }
    void FWorldDispatchArguments::Reset()
    {
        check(IsInGameThread());
        if (Owner) { check(Owner->References); --Owner->References; Owner = nullptr; }
    }
    FJsonObject* FWorldDispatchArguments::operator->() const
    { check(IsInGameThread() && Owner); return &Owner->Arguments; }
    namespace
    {
        using namespace WorldObservationPrivate;
        using namespace StateJsonPrivate;
        constexpr uint64 MaxCounter = 9007199254740991ULL;
        constexpr uint8 MaxResultLookups = 3;
        // Includes fixed JSON fields, allocator slack and the reserved task.
        constexpr uint64 QualifiedDispatchAllocationOverheadBytes = 0;
        constexpr uint64 QualifiedDispatchConstructionWork = 0;
        constexpr uint64 QualifiedDispatchReleaseWork = 0;
        bool Exact(const FObject& O, std::initializer_list<const TCHAR*> Names)
        {
            if (!O || O->Values.Num() != static_cast<int32>(Names.size())) return false;
            for (const auto& Pair : O->Values) if (Pair.Key.Len() > 32) return false;
            return Only(O, Names);
        }
        bool Hex(const FString& S, int32 Count)
        {
            if (S.Len() != Count) return false;
            for (TCHAR C : S) if (!(C >= '0' && C <= '9') && !(C >= 'a' && C <= 'f')) return false;
            return true;
        }
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
        bool ShortText(const FObject& O, const TCHAR* Key, FString& Out, int32 Limit = 64)
        {
            const auto V = Field(O, Key);
            if (!V || V->Type != EJson::String || V->AsString().Len() > Limit) return false;
            Out = V->AsString(); return true;
        }
        bool ReadTicket(const FObject& O, EWorldRequest Expected, const FString& RequestId,
            uint64 RequestSequence, FString& Alias, int64& RetryMs)
        {
            FString Status, Generation, Id, CaptureKind; int64 V = 0, Seq = 0;
            return Exact(O, {TEXT("v"), TEXT("generation"), TEXT("capture"), TEXT("status"), TEXT("kind"),
                    TEXT("requestId"), TEXT("requestSequence"), TEXT("retryAfterMs")})
                && Integer(Field(O, TEXT("v")), V) && V == 1
                && ShortText(O, TEXT("generation"), Generation) && Same(Generation, ProtocolGeneration)
                && ShortText(O, TEXT("capture"), Alias, 32) && Hex(Alias, 32)
                && ShortText(O, TEXT("status"), Status) && (Same(Status, TEXT("pending")) || Same(Status, TEXT("captured")))
                && ShortText(O, TEXT("requestId"), Id, 32) && Hex(Id, 32) && Same(Id, RequestId)
                && Integer(Field(O, TEXT("requestSequence")), Seq, 1) && static_cast<uint64>(Seq) == RequestSequence
                && ShortText(O, TEXT("kind"), CaptureKind) && Same(CaptureKind, Kind(Expected))
                && Integer(Field(O, TEXT("retryAfterMs")), RetryMs, 0) && RetryMs <= 900000;
        }
        bool FixedFailure(const FString& Status)
        {
            for (const TCHAR* Code : {TEXT("disabled"), TEXT("denied"), TEXT("selection_changed"), TEXT("entry_lost"),
                TEXT("world_lost"), TEXT("invalid_request"), TEXT("not_found"), TEXT("request_conflict"), TEXT("result_expired"),
                TEXT("cancelled_capture"), TEXT("content_changed"), TEXT("observation_stale"), TEXT("resource_unavailable"),
                TEXT("unsupported_value"), TEXT("capture_failed"), TEXT("operation_rejected"), TEXT("busy")})
                if (Same(Status, Code)) return true;
            return false;
        }
    }

    // Reader state and peer projections have separate, still-unqualified allocator
    // reservations. These are not estimates of Unreal allocator/controller slack.
    static constexpr uint64 QualifiedSemanticDataBytes = 0;
    static constexpr uint64 QualifiedSemanticInitializationWork = 0;
    static constexpr uint64 QualifiedSemanticPeerInitializationWork = 0;
    static constexpr uint64 QualifiedStaticPreparationWork = 0;
    static constexpr uint64 QualifiedWorldSubscriptionWork = 0;
    static constexpr uint64 QualifiedVisitAllocationOverheadBytes = 0;
    static constexpr uint64 QualifiedVisitAllocationWork = 0;
    static constexpr uint64 QualifiedReplyScratchBytes = 0;
    static constexpr uint64 QualifiedReplyScratchWork = 0;
    static constexpr uint64 QualifiedSessionRetirementBytes = 0;
    static constexpr uint64 QualifiedSessionRetirementInitializationWork = 0;
    static constexpr uint64 QualifiedTransientReleaseWork = 0;
    static constexpr uint64 QualifiedEntryCopyWork = 0;
    static constexpr uint64 QualifiedEntryCopyBytes = 0;
    static constexpr uint64 QualifiedBodyBytes = 0;
    static constexpr uint64 QualifiedBudgetBytes = 0;
    static constexpr uint64 QualifiedBudgetInitializationWork = 0;
    static constexpr uint64 QualifiedRequestIdentityBytes = 0;
    static constexpr uint64 QualifiedRequestIdentityWork = 0;
    static constexpr uint64 QualifiedCaptureIdentityBytes = 0;
    static constexpr uint64 QualifiedPreparationIdentityBytes = 0;

    struct FWorldSession::FVisitIdentity
    {
        explicit FVisitIdentity(TSharedRef<WorldObservationPrivate::FBudget> InBudget) : Budget(MoveTemp(InBudget)) {}
        ~FVisitIdentity()
        {
            ensureMsgf(Values[0].IsEmpty() && Values[1].IsEmpty(), TEXT("World visit identity was not retired."));
            Budget->Release(ChargedBytes);
        }
        TSharedRef<WorldObservationPrivate::FBudget> Budget;
        FString Values[2];
        uint64 ChargedBytes = 0, ReleaseWork = 0;
        double Deadline = 0;
        bool bRetiring = false;
        WorldObservationPrivate::EStep Retire()
        {
            using namespace WorldObservationPrivate;
            if (!bRetiring)
            {
                ReleaseWork = (static_cast<uint64>(Values[0].Len()) + Values[1].Len() + 2) * sizeof(TCHAR);
                bRetiring = true;
            }
            while (ReleaseWork)
            {
                const uint64 Part = FMath::Min<uint64>(ReleaseWork, 1024);
                if (!Budget->Spend(Part, false)) return EStep::Pending;
                ReleaseWork -= Part;
            }
            if (!QualifiedTransientReleaseWork || QualifiedTransientReleaseWork > TickWorkLimit)
                return EStep::ResourceUnavailable;
            if (!Budget->Spend(QualifiedTransientReleaseWork, false)) return EStep::Pending;
            Values[0].Empty(); Values[1].Empty(); return EStep::Complete;
        }
    };

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
        FRef Selection, SelectedCharacter, SelectedHome, SelectedActiveRoom, RaceInstance, InstanceHome;
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
        bool bLifetimeHealthLost = false;
        EWorldRequest Operation = EWorldRequest::None;
        FRef ExpectedPlace, ExpectedEndpoint;
        TCHAR Joined[1024];
        uint32 JoinedCount = 0, AppendOffset = 0;
        const WorldObservationPrivate::FStringBlock* AppendBlock = nullptr;
        uint64 AppendRemaining = 0;
        bool bAppending = false;
        EStep Result = EStep::Pending;
        TUniquePtr<FVisitIdentity> EnterVisit;
        uint64 IdentityUnits[2]{}, IdentityAllocationWork = 0, IdentityCharge = 0, IdentityOffset = 0;
        const WorldObservationPrivate::FStringBlock* IdentityBlock = nullptr;
        uint8 IdentityIndex = 0;
        bool bIdentityPlanned = false;

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
        EStep PreparePlayerHeader();
        EStep MaterializeVisit();
        EStep Retire()
        {
            bCancelled = true;
            if (EnterVisit)
            {
                if (EnterVisit->Retire() != EStep::Complete || !WorldObservationPrivate::QualifiedRetirementDeleteWork
                    || !Storage->Budget->Spend(WorldObservationPrivate::QualifiedRetirementDeleteWork, false)) return EStep::Pending;
                EnterVisit.Reset();
            }
            while (Players)
            {
                if (Players->Retire(*Storage->Budget) != EStep::Complete) return EStep::Pending;
                if (!WorldObservationPrivate::QualifiedRetirementDeleteWork
                    || !Storage->Budget->Spend(WorldObservationPrivate::QualifiedRetirementDeleteWork, false)) return EStep::Pending;
                auto* Old = Players; Players = Players->Next;
                delete Old; Storage->Budget->Release(WorldReadPrivate::QualifiedSemanticPeerBytes);
            }
            return EStep::Complete;
        }
        EStep AllocatePlayer(FRef Raw)
        {
            if (!WorldReadPrivate::QualifiedSemanticPeerBytes || !QualifiedSemanticPeerInitializationWork
                || QualifiedSemanticPeerInitializationWork > WorldObservationPrivate::TickWorkLimit)
                return EStep::ResourceUnavailable;
            if (!Spend(QualifiedSemanticPeerInitializationWork)) return Yield();
            if (!Storage->Budget->Reserve(WorldReadPrivate::QualifiedSemanticPeerBytes)) return EStep::ResourceUnavailable;
            auto* Next = new FPlayerResult;
            Next->Raw = Raw; Next->bPeer = !bLocal;
            if (Tail) Tail->Next = Next; else Players = Next;
            Tail = CurrentPlayer = Next;
            return EStep::Complete;
        }
    };

    WorldObservationPrivate::EStep FWorldSession::FSemanticData::MaterializeVisit()
    {
        using namespace WorldObservationPrivate;
        using namespace WorldReadPrivate;
        if (!QualifiedVisitAllocationOverheadBytes) return EStep::ResourceUnavailable;
        if (!bIdentityPlanned)
        {
            if (!Spend(256)) return Yield();
            const auto* Id = Resolve(*Storage, Binding.Visit().Field(VisitId));
            const auto* Life = Resolve(*Storage, Binding.Visit().Field(VisitLife));
            if (!Id || !Life || Id->Kind != EWorldValueKind::String || Life->Kind != EWorldValueKind::String
                || !Id->Count || !Life->Count) return EStep::Invalid;
            // These are FString representation and retained-memory bounds, not
            // gameplay identity quotas. All UTF-16 units, including zero, survive.
            if (Id->Count >= MAX_int32 || Life->Count >= MAX_int32) return EStep::ResourceUnavailable;
            IdentityUnits[0] = Id->Count; IdentityUnits[1] = Life->Count;
            const uint64 Bytes = (Id->Count + Life->Count + 2) * sizeof(TCHAR);
            if (QualifiedVisitAllocationOverheadBytes > RetainedLimit - PageAllowance - sizeof(FVisitIdentity)
                || Bytes > RetainedLimit - PageAllowance - sizeof(FVisitIdentity) - QualifiedVisitAllocationOverheadBytes)
                return EStep::ResourceUnavailable;
            IdentityCharge = sizeof(FVisitIdentity) + QualifiedVisitAllocationOverheadBytes + Bytes;
            IdentityAllocationWork = sizeof(FVisitIdentity) + Bytes;
            bIdentityPlanned = true;
        }
        // Prepay initialization/allocation before allocating either destination.
        // Copying from segmented input below has its own per-block work charge.
        while (IdentityAllocationWork)
        {
            const uint64 Part = FMath::Min<uint64>(IdentityAllocationWork, 1024);
            if (!Spend(Part)) return Yield();
            IdentityAllocationWork -= Part;
        }
        if (!EnterVisit)
        {
            if (!QualifiedVisitAllocationWork || QualifiedVisitAllocationWork > TickWorkLimit)
                return EStep::ResourceUnavailable;
            if (!Spend(QualifiedVisitAllocationWork)) return Yield();
            if (!Storage->Budget->Reserve(IdentityCharge)) return EStep::ResourceUnavailable;
            EnterVisit = MakeUnique<FVisitIdentity>(Storage->Budget);
            EnterVisit->ChargedBytes = IdentityCharge;
            for (uint8 I = 0; I < 2; ++I)
            {
                auto& Units = EnterVisit->Values[I].GetCharArray();
                Units.SetNumUninitialized(static_cast<int32>(IdentityUnits[I] + 1));
                Units[static_cast<int32>(IdentityUnits[I])] = 0;
            }
        }
        while (IdentityIndex < 2)
        {
            if (!Spend(16)) return Yield();
            if (!IdentityOffset)
            {
                const auto* N = Resolve(*Storage, Binding.Visit().Field(IdentityIndex == 0 ? VisitId : VisitLife));
                if (!N) return EStep::Invalid;
                IdentityBlock = N->Text;
            }
            while (IdentityBlock)
            {
                const uint64 Count = IdentityBlock->Used;
                if (!Count || Count > 256 || Count > IdentityUnits[IdentityIndex] - IdentityOffset) return EStep::Invalid;
                if (!Spend(16 + Count * 2)) return Yield();
                auto& Units = EnterVisit->Values[IdentityIndex].GetCharArray();
                for (uint64 I = 0; I < Count; ++I)
                    Units[static_cast<int32>(IdentityOffset + I)] = static_cast<TCHAR>(IdentityBlock->Units[I]);
                IdentityOffset += Count; IdentityBlock = IdentityBlock->Next;
            }
            if (IdentityOffset != IdentityUnits[IdentityIndex]) return EStep::Invalid;
            ++IdentityIndex; IdentityOffset = 0;
        }
        return EStep::Complete;
    }

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
            case 5:
                if (bHome)
                {
                    S = Equal(Room.Fields[0], B.Field(BindingHome), Same);
                    if (S == EStep::Complete && Same) InstanceHome = Room.Fields[0];
                }
                break;
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
            if (!Spend(1024)) return Yield();
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
        const auto Prepared = PreparePlayerHeader();
        if (Prepared != EStep::Complete) return Prepared;
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

    WorldObservationPrivate::EStep FWorldSession::FSemanticData::PreparePlayerHeader()
    {
        using namespace WorldReadPrivate;
        while (Subphase < 8)
        {
            EStep Result = EStep::Complete;
            switch (Subphase)
            {
            case 0: Result = Read(EDomain::Pose, CurrentPlayer->Position.Ref, Endpoint); break;
            case 1:
                if (Player.Presence[12] == EWorldFieldPresence::Value)
                    Result = Read(EDomain::Journey, Player.Fields[12], Journey);
                break;
            case 2:
                if (Player.Presence[17] == EWorldFieldPresence::Value)
                    Result = Read(EDomain::Nightclub, Player.Fields[17], Activity);
                break;
            case 3:
                if (CurrentPlayer->Water.Ref.IsSet()) Result = Read(EDomain::Water, CurrentPlayer->Water.Ref, Water);
                break;
            case 4:
                if (!Spend(sizeof(FWorldPlayerHeader) + sizeof(FControlFacts) + 64)) return Yield();
                {
                    auto& Header = CurrentPlayer->Header;
                    Header.Revision = static_cast<int64>(Player.Numbers[6]);
                    Header.MoveSequence = static_cast<int64>(Player.Numbers[7]);
                    Header.ServerNowMs = static_cast<int64>(Binding.Receipt().Number(ReceiptNow));
                    Header.Status = static_cast<EWorldPlayerStatus>(Player.Choices[10]); Header.Health = Player.Numbers[11];
                    Header.Position = {Endpoint.Numbers[0], Endpoint.Numbers[1], Endpoint.Numbers[2]};
                    Header.Movement = Player.Presence[9]; Header.bControlUnavailable = CurrentPlayer->bControlUnavailable;
                    FControlFacts Facts;
                    Facts.Status = Header.Status; Facts.Health = Header.Health;
                    Facts.bJourneyTraveling = Player.Presence[12] == EWorldFieldPresence::Value && Journey.Choices[5] == 0;
                    Facts.bDriving = CurrentPlayer->Driving.Ref.IsSet();
                    Facts.bActiveWater = CurrentPlayer->Water.Ref.IsSet() && Water.Choices[15] == 0;
                    Facts.bTransit = Player.Presence[14] == EWorldFieldPresence::Value;
                    Facts.bDance = Player.Presence[17] == EWorldFieldPresence::Value && Activity.Choices[2] == 0;
                    Facts.bLoungeSeat = Player.Presence[17] == EWorldFieldPresence::Value && Activity.Choices[2] == 1
                        && Activity.Presence[5] == EWorldFieldPresence::Value;
                    Facts.bMovement = Header.Movement == EWorldFieldPresence::Value;
                    Facts.Driving = Player.Presence[15]; Facts.Water = Player.Presence[16];
                    Facts.Transit = Player.Presence[14]; Facts.Nightclub = Player.Presence[17];
                    // ValidateControls has compared every effective identity before PinPlayer.
                    Header.Controls = ControlObservations(Facts);
                    CurrentPlayer->SuppliedDriving = Facts.Driving; CurrentPlayer->SuppliedWater = Facts.Water;
                    Header.Driving = Facts.bDriving ? EWorldFieldPresence::Value
                        : Facts.Driving == EWorldFieldPresence::Null ? EWorldFieldPresence::Null : EWorldFieldPresence::Missing;
                    Header.Water = CurrentPlayer->Water.Ref.IsSet() ? EWorldFieldPresence::Value
                        : Facts.Water == EWorldFieldPresence::Null ? EWorldFieldPresence::Null : EWorldFieldPresence::Missing;
                }
                break;
            case 5: break;
            case 6: break;
            case 7: break;
            }
            if (Result != EStep::Complete) return Result;
            ++Subphase;
        }
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

    struct FWorldSession::FSessionRetirement
    {
        TSharedPtr<WorldObservationPrivate::FBudget> Budget;
        FWorldEntryIdentity Entries[2];
        FWorldDispatch Requests[2];
        FString Body, RequestIds[2], Captures[2], Preparations[2];
        TSharedPtr<FJsonObject> DecodedBody;
        TUniquePtr<WorldObservationPrivate::FPublicationPreparation> Publication;
        TUniquePtr<WorldObservationPrivate::FPages> Pages;
        TUniquePtr<FSemanticData> Semantic;
        TUniquePtr<FVisitIdentity> Visit;
        TSharedPtr<WorldObservationPrivate::FPublicationClock> Clock;
        TSharedPtr<WorldObservationPrivate::FStorage> Storage[3];
        uint64 Charge = 0, TransientCharge = 0;
        uint32 Field = 0;
        bool bOwnBodyPage = false;
        bool HasTransient() const
        {
            return Entries[0].CredentialGeneration || Entries[1].CredentialGeneration
                || Requests[0].Arguments || Requests[1].Arguments || DecodedBody || bOwnBodyPage
                || !Body.GetCharArray().IsEmpty() || !RequestIds[0].GetCharArray().IsEmpty()
                || !RequestIds[1].GetCharArray().IsEmpty() || !Captures[0].GetCharArray().IsEmpty()
                || !Captures[1].GetCharArray().IsEmpty() || !Preparations[0].GetCharArray().IsEmpty()
                || !Preparations[1].GetCharArray().IsEmpty();
        }
        void Hold(FString& Text, FString (&Slots)[2])
        {
            if (Text.GetCharArray().IsEmpty()) return;
            for (auto& Slot : Slots)
                if (Slot.GetCharArray().IsEmpty()) { Slot = MoveTemp(Text); Field = 0; return; }
            checkNoEntry();
        }
        WorldObservationPrivate::EStep Advance()
        {
            using namespace WorldObservationPrivate;
            if (HasTransient())
            {
                if (!QualifiedTransientReleaseWork || QualifiedTransientReleaseWork > TickWorkLimit)
                    return EStep::ResourceUnavailable;
                FString* Texts[] = { &Entries[0].GrantId, &Entries[0].UserId, &Entries[0].CharacterId, &Entries[0].HomeRoomId,
                    &Entries[0].GameRoomId, &Entries[0].ParticipantId, &Entries[0].CallSessionId, &Entries[0].ExpressionSessionId,
                    &Entries[0].ApprovingSessionId, &Entries[0].ReservationGeneration,
                    &Entries[1].GrantId, &Entries[1].UserId, &Entries[1].CharacterId, &Entries[1].HomeRoomId,
                    &Entries[1].GameRoomId, &Entries[1].ParticipantId, &Entries[1].CallSessionId, &Entries[1].ExpressionSessionId,
                    &Entries[1].ApprovingSessionId, &Entries[1].ReservationGeneration, &Body,
                    &RequestIds[0], &RequestIds[1], &Captures[0], &Captures[1], &Preparations[0], &Preparations[1] };
                while (Field < UE_ARRAY_COUNT(Texts))
                {
                    if (!Budget->Spend(QualifiedTransientReleaseWork, false)) return EStep::Pending;
                    Texts[Field++]->Empty();
                }
                if (!Budget->Spend(QualifiedTransientReleaseWork, false)) return EStep::Pending;
                Requests[0] = {}; Requests[1] = {}; DecodedBody.Reset();
                if (bOwnBodyPage) { Budget->ReleasePage(); bOwnBodyPage = false; }
                Entries[0] = {}; Entries[1] = {}; Field = 0;
                Budget->Release(TransientCharge); TransientCharge = 0;
            }
            if (Publication)
            {
                if (!Publication->Owner || Publication->Retire(*Publication->Owner) != EStep::Complete)
                    return EStep::Pending;
                if (!QualifiedRetirementDeleteWork || !Budget->Spend(QualifiedRetirementDeleteWork, false)) return EStep::Pending;
                const uint64 Bytes = Publication->Charge; Publication.Reset(); Budget->Release(Bytes);
            }
            if (Pages)
            {
                if (!QualifiedRetirementDeleteWork || !Budget->Spend(QualifiedRetirementDeleteWork, false))
                    return EStep::Pending;
                Pages.Reset();
            }
            if (Semantic)
            {
                if (Semantic->Retire() != EStep::Complete || !QualifiedRetirementDeleteWork
                    || !Budget->Spend(QualifiedRetirementDeleteWork, false)) return EStep::Pending;
                Semantic.Reset(); Budget->Release(QualifiedSemanticDataBytes);
            }
            if (Visit)
            {
                if (Visit->Retire() != EStep::Complete || !QualifiedRetirementDeleteWork
                    || !Budget->Spend(QualifiedRetirementDeleteWork, false)) return EStep::Pending;
                Visit.Reset();
            }
            if (Clock)
            {
                uint64 Bytes = 0;
                if (Clock.GetSharedReferenceCount() == 1)
                {
                    if (Clock->Retire(*Budget) != EStep::Complete) return EStep::Pending;
                    Bytes = Clock->Charge;
                }
                if (!QualifiedRetirementDeleteWork || !Budget->Spend(QualifiedRetirementDeleteWork, false)) return EStep::Pending;
                Clock.Reset(); Budget->Release(Bytes);
            }
            for (auto& Owner : Storage)
            {
                if (!Owner) continue;
                if (Owner->Retire() != EStep::Complete || !QualifiedRetirementDeleteWork
                    || !Budget->Spend(QualifiedRetirementDeleteWork, false)) return EStep::Pending;
                Owner.Reset();
            }
            return EStep::Complete;
        }
    };

    bool FWorldSession::InitializeRetirement()
    {
        using namespace WorldObservationPrivate;
        if (Retirement) return true;
        if (!Budget || !QualifiedSessionRetirementBytes || !QualifiedSessionRetirementInitializationWork
            || QualifiedSessionRetirementInitializationWork > TickWorkLimit || !EnsureRetirementTicket(Budget)) return false;
        RetirementSlot = ReserveRetirementTask(*Budget);
        if (RetirementSlot == INDEX_NONE) return false;
        if (!Budget->Spend(QualifiedSessionRetirementInitializationWork, false)
            || !Budget->Reserve(QualifiedSessionRetirementBytes))
        { ReleaseRetirementTask(*Budget, RetirementSlot); RetirementSlot = INDEX_NONE; return false; }
        Retirement = MakeUnique<FSessionRetirement>(); Retirement->Budget = Budget;
        Retirement->Charge = QualifiedSessionRetirementBytes; return true;
    }
    void FWorldSession::RetireBody()
    {
        if (Body.GetCharArray().IsEmpty() && !DecodedBody && !bOwnBodyPage) return;
        check(Retirement);
        if (!Body.GetCharArray().IsEmpty())
        {
            check(Retirement->Body.GetCharArray().IsEmpty());
            Retirement->Body = MoveTemp(Body);
            Retirement->Field = 0;
        }
        if (DecodedBody) RetireDecodedBody();
        else if (bOwnBodyPage) { Retirement->bOwnBodyPage = true; bOwnBodyPage = false; }
        Retirement->TransientCharge += BodyCharge; BodyCharge = 0;
    }
    void FWorldSession::RetireDecodedBody()
    {
        if (!DecodedBody) return;
        check(Retirement && !Retirement->DecodedBody);
        Retirement->DecodedBody = MoveTemp(DecodedBody);
        Retirement->bOwnBodyPage |= bOwnBodyPage; bOwnBodyPage = false;
        Retirement->Field = 0;
        Retirement->TransientCharge += BodyCharge; BodyCharge = 0;
    }
    void FWorldSession::RetireRequest()
    {
        if (!Request.Arguments) { Request = {}; return; }
        check(Retirement);
        for (auto& Retired : Retirement->Requests)
            if (!Retired.Arguments)
            { Retired = MoveTemp(Request); Request = {}; Retirement->Field = 0; return; }
        checkNoEntry();
    }
    void FWorldSession::RetireAttemptText(bool bEntry)
    {
        if (!Retirement) return;
        Retirement->Hold(RequestId, Retirement->RequestIds);
        Retirement->Hold(Capture, Retirement->Captures);
        Retirement->Hold(Preparation, Retirement->Preparations);
        Retirement->TransientCharge += RequestIdentityCharge + CaptureCharge + PreparationCharge;
        RequestIdentityCharge = CaptureCharge = PreparationCharge = 0;
        if (bEntry && Entry.CredentialGeneration)
        {
            for (auto& Retired : Retirement->Entries)
                if (!Retired.CredentialGeneration)
                {
                    Retired = MoveTemp(Entry); Entry = {};
                    Retirement->TransientCharge += QualifiedEntryCopyBytes;
                    Retirement->Field = 0;
                    return;
                }
            checkNoEntry();
        }
    }

    FWorldSession::FWorldSession() = default;
    FWorldSession::~FWorldSession()
    {
        Clear();
        if (Access)
        {
            using namespace WorldObservationPrivate;
            auto* Cell = Access.Release(); Cell->bAlive = false;
            QueueRetirementTask(*Cell->Budget, Cell->RetirementSlot, {Cell,
                [](void* Value, FBudget& Owner)
                {
                    if (static_cast<FAccess*>(Value)->WeakHandles) return EStep::Pending;
                    if (!QualifiedAccessReleaseWork || QualifiedAccessReleaseWork > TickWorkLimit)
                        return EStep::ResourceUnavailable;
                    return Owner.Spend(QualifiedAccessReleaseWork, false) ? EStep::Complete : EStep::Pending;
                }, [](void* Value) { delete static_cast<FAccess*>(Value); }, Cell->Charge});
        }
        if (Retirement)
        {
            using namespace WorldObservationPrivate;
            if (Publication)
            { check(!Retirement->Publication); Retirement->Publication = MoveTemp(Publication); }
            if (Semantic)
            { check(!Retirement->Semantic); Retirement->Semantic = MoveTemp(Semantic); }
            Retirement->Visit = MoveTemp(RetiringVisit); Retirement->Clock = MoveTemp(RetiringClock);
            for (uint32 I = 0; I < 3; ++I) Retirement->Storage[I] = MoveTemp(Retiring[I]);
            RetireDetached(MoveTemp(RetiringStatic));
            auto* Owner = Retirement.Release();
            QueueRetirementTask(*Budget, RetirementSlot, {Owner,
                [](void* Value, FBudget&) { return static_cast<FSessionRetirement*>(Value)->Advance(); },
                [](void* Value) { delete static_cast<FSessionRetirement*>(Value); }, Owner->Charge});
        }
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
            && std::isfinite(Now) && std::isfinite(Entry.Deadline) && Now < Entry.Deadline
            && (!Visit || (!Visit->bRetiring && std::isfinite(Visit->Deadline) && Now < Visit->Deadline));
    }
    void FWorldSession::Attach() { bAttached = true; }
    void FWorldSession::Detach() { bAttached = false; Clear(); }
    bool FWorldSession::HasRetirement() const
    {
        if (Budget && Budget->Retirement && Budget->Retirement->HasQueued()) return true;
        if (Retirement && (Retirement->HasTransient() || Retirement->Publication || Retirement->Pages || Retirement->Semantic)) return true;
        if (Publication) return true;
        if (Semantic && Semantic->bCancelled) return true;
        if (RetiringStatic || RetiringVisit || RetiringClock) return true;
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
        PendingNotification = {};
        bRestartPublication = false;
        PublicationRestartPhase = 0;
        Transport.Reset();
        State.Observation = {}; State.bHasObservation = State.bWalkingAvailable = false;
        if (State.Generation != MAX_uint64) ++State.Generation;
        State.Status = Status;
        bInFlight = false; RetireRequest(); Next = CaptureOperation = RequestedOperation = EWorldRequest::None;
        RetireBody();
        CancelSemantics();
        if (Publication) Publication->Cancel();
        if (Accepted && Accepted->Publication)
            Recovery.ClearWorldSubscription(Accepted->Publication->RetiredRecovery);
        else
        {
            FRecoveryReadView Empty;
            Recovery.ClearWorldSubscription(Empty);
            check(Empty.OwnerId.IsEmpty() && !Empty.Health);
        }
        RecoveryReceipt = {};
        if (ClockBaseline)
        {
            if (Accepted && Accepted->Publication)
            {
                check(!Accepted->Publication->RetiredClock);
                Accepted->Publication->RetiredClock = MoveTemp(ClockBaseline);
            }
            else
            {
                check(!RetiringClock);
                RetiringClock = MoveTemp(ClockBaseline);
            }
        }
        if (Pages) Retire(Pages->Cancel());
        Pages.Reset();
        if (Candidate) Retire(MoveTemp(Candidate));
        if (Accepted) Retire(MoveTemp(Accepted));
        RetireStatic();
        if (Visit)
        {
            // Admission waits for prior retirement, so there is never a third
            // visit owner or an allocation-address reuse within this generation.
            check(!RetiringVisit);
            RetiringVisit = MoveTemp(Visit);
        }
        RetireAttemptText(true);
        RevisionHighWater = MoveHighWater = -1;
        HealthHighWater = SavedHighWater = -1;
        TicketArrival = CandidateDeadline = NextAt = OriginalDispatchAt = 0;
        ReceiptExpiresAt = 0; bBindingValidated = false;
        AfterRetirement = EWorldRequest::None;
        SettledOperation = EWorldRequest::None; SettledRequestSequence = SettledOperationSequence = 0;
        SettledRequestId[0] = SettledCapture[0] = 0; bSettledAcknowledged = false;
        bAttemptActive = bReplacementUsed = bLostTicket = bPageRetried = false; ResultLookups = 0;
        bManifestBodyPending = false;
        if (OrderingVersion != MAX_uint64) ++OrderingVersion;
    }
    void FWorldSession::BeginEntry(const FWorldEntryIdentity& Identity)
    {
        Clear();
        LostHealthPresence = EWorldFieldPresence::Missing;
        bLostHealthDecodedNull = false;
        if (!bAttached || State.Generation == MAX_uint64) return;
        if (!WorldObservationPrivate::FPages::ResourcesQualified()
            || !QualifiedEntryCopyWork || !QualifiedEntryCopyBytes
            || QualifiedEntryCopyWork > WorldObservationPrivate::TickWorkLimit)
        { State.Status = EWorldReadStatus::ResourceUnavailable; return; }
        if (!Budget)
        {
            if (!QualifiedBudgetBytes || QualifiedBudgetBytes > WorldObservationPrivate::RetainedLimit
                || !QualifiedBudgetInitializationWork
                || QualifiedBudgetInitializationWork > WorldObservationPrivate::TickWorkLimit
                || !WorldObservationPrivate::QualifiedRetirementDeleteWork)
            { State.Status = EWorldReadStatus::ResourceUnavailable; return; }
            WorldObservationPrivate::FBudget InitialBudget;
            InitialBudget.BeginTick();
            if (!InitialBudget.Reserve(QualifiedBudgetBytes)
                || !InitialBudget.Spend(QualifiedBudgetInitializationWork, false))
            { State.Status = EWorldReadStatus::ResourceUnavailable; return; }
            InitialBudget.OwnerCharge = QualifiedBudgetBytes;
            // Both ledger writes precede allocation and survive shipping builds.
            Budget = MakeShared<WorldObservationPrivate::FBudget>(MoveTemp(InitialBudget));
        }
        if (!InitializeRetirement()) { State.Status = EWorldReadStatus::ResourceUnavailable; return; }
        // One new entry may wait beside the revoked entry. A further replacement
        // is refused before allocation until those two fixed owners drain.
        if (Retirement->Entries[0].CredentialGeneration && Retirement->Entries[1].CredentialGeneration)
        { State.Status = EWorldReadStatus::ResourceUnavailable; return; }
        if (!Budget->Spend(QualifiedEntryCopyWork)
            || !Budget->Reserve(QualifiedEntryCopyBytes)) return;
        Entry = Identity;
        if (Live())
        {
            if (HasRetirement()) AfterRetirement = EWorldRequest::Enter;
            else BeginCapture(EWorldRequest::Enter);
        }
    }
    void FWorldSession::Retry()
    {
        if (!Live() || bAttemptActive || bInFlight || !Body.IsEmpty() || DecodedBody
            || Next != EWorldRequest::None || AfterRetirement != EWorldRequest::None || HasRetirement() || Candidate) return;
        // Explicit reload starts new read work only after the previous request is
        // terminal. It cannot replay an enter/control operation or revive its lookup.
        if (Visit)
        {
            InvalidateContent();
            AfterRetirement = EWorldRequest::Prepare;
        }
    }
    bool FWorldSession::BeginCapture(EWorldRequest Operation, bool bContinueAttempt)
    {
        // Movement/presentation request argument ownership is not implemented yet.
        // Never construct an incomplete command if another prerequisite is enabled.
        if (Operation != EWorldRequest::Enter && Operation != EWorldRequest::Prepare
            && Operation != EWorldRequest::Observe) return false;
        const double Now = FPlatformTime::Seconds();
        if (!Live() || HasRetirement() || Candidate || bInFlight || !Body.IsEmpty() || DecodedBody
            || Next != EWorldRequest::None || RequestSequence >= MaxCounter
            || (Command(Operation) && OperationSequence >= MaxCounter)) return false;
        if (bContinueAttempt)
        {
            const bool bAfterEnter = Operation == EWorldRequest::Prepare && CaptureOperation == EWorldRequest::Enter;
            const bool bAfterPrepare = Operation == EWorldRequest::Observe && CaptureOperation == EWorldRequest::Prepare
                && !Preparation.IsEmpty() && Static && Static->Current(StaticContext());
            if (!bAttemptActive || (!bAfterEnter && !bAfterPrepare) || !Visit || !Budget || Budget->bExhausted
                || !std::isfinite(CandidateDeadline) || Now >= CandidateDeadline) return false;
        }
        else if (bAttemptActive || Operation == EWorldRequest::Observe) return false;
        if ((Operation == EWorldRequest::Enter) == static_cast<bool>(Visit)) return false;
        if (!WorldObservationPrivate::FPages::ResourcesQualified())
        { State.Status = EWorldReadStatus::ResourceUnavailable; return false; }
        if (!Budget) return false;
        if (!InitializeRetirement()) return false;
        if (Budget->Retirement && Budget->Retirement->HasQueued()) return false;
        if (!QualifiedRequestIdentityBytes || !QualifiedRequestIdentityWork
            || QualifiedRequestIdentityWork > WorldObservationPrivate::TickWorkLimit)
        { State.Status = EWorldReadStatus::ResourceUnavailable; return false; }
        if (!bContinueAttempt)
        {
            Budget->CandidateWork = 0; Budget->bExhausted = false;
            CandidateDeadline = FMath::Min(Entry.Deadline, Now + 900);
            if (Visit) CandidateDeadline = FMath::Min(CandidateDeadline, Visit->Deadline);
            bReplacementUsed = false;
            if (!Budget->Spend(512)) { State.Status = EWorldReadStatus::ResourceUnavailable; return false; }
        }
        if (!Budget->Spend(QualifiedRequestIdentityWork)
            || !Budget->Reserve(QualifiedRequestIdentityBytes)) return false;
        if (!RequestId.GetCharArray().IsEmpty())
        {
            Retirement->Hold(RequestId, Retirement->RequestIds);
            Retirement->TransientCharge += RequestIdentityCharge; RequestIdentityCharge = 0;
        }
        if (!Capture.GetCharArray().IsEmpty())
        {
            Retirement->Hold(Capture, Retirement->Captures);
            Retirement->TransientCharge += CaptureCharge; CaptureCharge = 0;
        }
        RequestIdentityCharge = QualifiedRequestIdentityBytes;
        Pages = MakeUnique<WorldObservationPrivate::FPages>(Budget.ToSharedRef());
        ++RequestSequence;
        if (Command(Operation)) ++OperationSequence;
        RequestId = FGuid::NewGuid().ToString(EGuidFormats::Digits);
        RequestedOperation = CaptureOperation = Operation; Next = Operation;
        TicketArrival = OriginalDispatchAt = 0; ResultLookups = 0; ReceiptExpiresAt = 0; bAttemptActive = true;
        bLostTicket = bPageRetried = bBindingValidated = bFinalBindComplete = false;
        NextAt = Now;
        State.Status = Operation == EWorldRequest::Prepare ? EWorldReadStatus::Preparing : EWorldReadStatus::Capturing;
        return true;
    }
    bool FWorldSession::Current(const FWorldDispatch& Dispatch) const
    {
        return Live() && bAttemptActive && FPlatformTime::Seconds() < CandidateDeadline
            && bInFlight && Dispatch.Lifetime == State.Generation
            && Dispatch.Serial == Request.Serial && Dispatch.Operation == Request.Operation;
    }
    bool FWorldSession::TakeDispatch(FWorldDispatch& Out)
    {
        Out = {};
        if (!Live() || !bAttemptActive || bInFlight || !Body.IsEmpty() || DecodedBody || Next == EWorldRequest::None
            || FPlatformTime::Seconds() < NextAt || DispatchSerial == MAX_uint64) return false;
        if (FPlatformTime::Seconds() >= CandidateDeadline)
        { Unavailable(EWorldReadStatus::Expired); return false; }
        if (Next == EWorldRequest::Bind && CaptureOperation != EWorldRequest::Observe)
        { Unavailable(EWorldReadStatus::InvalidResponse); return false; }
        if (Next == EWorldRequest::Result)
        {
            if (!bLostTicket || ResultLookups >= MaxResultLookups)
            { Unavailable(EWorldReadStatus::Unavailable); return false; }
        }
        const bool bVisitArguments = Next == RequestedOperation && Next != EWorldRequest::Enter;
        if (bVisitArguments && !Visit) { Unavailable(EWorldReadStatus::InvalidResponse); return false; }
        const uint64 IdentityUnits = bVisitArguments
            ? static_cast<uint64>(Visit->Values[0].Len()) + Visit->Values[1].Len() + 2 : 0;
        const uint64 CopyWork = 4096 + IdentityUnits * sizeof(TCHAR) * 2;
        // FString/JSON construction is indivisible here. Refuse a request whose
        // exact copies cannot fit the existing tick policy; never shorten an ID.
        if (!Budget || !QualifiedDispatchAllocationOverheadBytes || !QualifiedDispatchConstructionWork
            || QualifiedDispatchConstructionWork > WorldObservationPrivate::TickWorkLimit
            || CopyWork > WorldObservationPrivate::TickWorkLimit - QualifiedDispatchConstructionWork
            || !QualifiedDispatchReleaseWork
            || QualifiedDispatchReleaseWork > WorldObservationPrivate::TickWorkLimit
            || CopyWork > WorldObservationPrivate::TickWorkLimit)
        { Unavailable(EWorldReadStatus::ResourceUnavailable); return false; }
        if (!Budget->Spend(CopyWork + QualifiedDispatchConstructionWork))
        {
            if (Budget->bExhausted) Unavailable(EWorldReadStatus::ResourceUnavailable);
            return false;
        }
        const uint64 CopyBytes = IdentityUnits * sizeof(TCHAR) * 2;
        if (QualifiedDispatchAllocationOverheadBytes > WorldObservationPrivate::RetainedLimit
            || CopyBytes > WorldObservationPrivate::RetainedLimit - QualifiedDispatchAllocationOverheadBytes)
        { Unavailable(EWorldReadStatus::ResourceUnavailable); return false; }
        const uint64 Charge = QualifiedDispatchAllocationOverheadBytes + CopyBytes;
        if (Retirement && Retirement->Requests[0].Arguments && Retirement->Requests[1].Arguments) return false;
        RetireRequest();
        if (!Budget->Reserve(Charge)) { Unavailable(EWorldReadStatus::ResourceUnavailable); return false; }
        if (!EnsureRetirementTicket(Budget))
        { Budget->Release(Charge); Unavailable(EWorldReadStatus::ResourceUnavailable); return false; }
        const int32 RetirementSlot = ReserveRetirementTask(*Budget);
        if (RetirementSlot == INDEX_NONE)
        { Budget->Release(Charge); return false; }
        auto* ArgumentsOwner = new FWorldDispatchArgumentsOwner;
        const FWorldDispatchArguments Args(ArgumentsOwner);
        // Reference release does fixed metadata work only. Even an abandoned
        // dispatch remains charged until the task admits complete DOM deletion.
        QueueRetirementTask(*Budget, RetirementSlot, {ArgumentsOwner,
            [](void* Value, FBudget& Owner)
            {
                if (static_cast<FWorldDispatchArgumentsOwner*>(Value)->References) return EStep::Pending;
                return Owner.Spend(QualifiedDispatchReleaseWork, false) ? EStep::Complete : EStep::Pending;
            }, [](void* Value) { delete static_cast<FWorldDispatchArgumentsOwner*>(Value); }, Charge, true,
            [](void* Value) { return static_cast<FWorldDispatchArgumentsOwner*>(Value)->References == 0; }});
        Args->SetNumberField(TEXT("entrySequence"), Entry.EntrySequence);
        if (Next == RequestedOperation || Next == EWorldRequest::Result)
        {
            Args->SetStringField(TEXT("requestId"), RequestId);
            Args->SetNumberField(TEXT("requestSequence"), static_cast<double>(RequestSequence));
            if (Next != EWorldRequest::Result)
            {
                if (Command(Next)) Args->SetNumberField(TEXT("operationSequence"), static_cast<double>(OperationSequence));
                if (Next != EWorldRequest::Enter)
                { Args->SetStringField(TEXT("visitId"), Visit->Values[0]); Args->SetStringField(TEXT("lifeId"), Visit->Values[1]); }
                if (Next == EWorldRequest::Observe) Args->SetStringField(TEXT("preparation"), Preparation);
            }
        }
        else
        {
            Args->SetStringField(TEXT("capture"), Capture);
            if (Next == EWorldRequest::Page)
            {
                TCHAR Root[33]; uint64 Ordinal = 0;
                if (!Pages) return false;
                const auto Cursor = Pages->Next(Root, Ordinal);
                if (Cursor == EStep::Invalid || Cursor == EStep::ResourceUnavailable)
                { Unavailable(Cursor == EStep::Invalid ? EWorldReadStatus::InvalidResponse : EWorldReadStatus::ResourceUnavailable); return false; }
                if (Cursor != EStep::Complete) return false;
                Args->SetArrayField(TEXT("cursor"), {MakeShared<FJsonValueString>(FString(Root)),
                    MakeShared<FJsonValueNumber>(static_cast<double>(Ordinal))});
            }
        }
        Request = {Next, State.Generation, ++DispatchSerial, Args};
        if (Next == EWorldRequest::Result) ++ResultLookups;
        // Earlier than the credential owner's actual send is conservative for
        // receipt expiry. Result lookups and capture polling never replace it.
        if (Next == RequestedOperation && OriginalDispatchAt == 0) OriginalDispatchAt = FPlatformTime::Seconds();
        Next = EWorldRequest::None; bInFlight = true; Out = Request;
        return true;
    }
    void FWorldSession::Complete(const FWorldDispatch& Dispatch, FString&& InBody, double S, double R)
    {
        // The transport retains rejected input. This owner adopts its allocation
        // only after admission; stale callbacks never destroy it on this stack.
        if (!Current(Dispatch)) return;
        bInFlight = false;
        if (!std::isfinite(S) || !std::isfinite(R) || R < S || R > FPlatformTime::Seconds()
            || !std::isfinite(OriginalDispatchAt) || OriginalDispatchAt <= 0 || S < OriginalDispatchAt
            || InBody.IsEmpty() || InBody.Len() > WorldObservationPrivate::BodyLimit)
        { Unavailable(EWorldReadStatus::InvalidResponse); return; }
        if (!QualifiedBodyBytes || !Retirement || !Retirement->Body.GetCharArray().IsEmpty() || Retirement->DecodedBody
            || !Budget->Reserve(QualifiedBodyBytes))
        { Unavailable(EWorldReadStatus::ResourceUnavailable); return; }
        BodyCharge = QualifiedBodyBytes;
        SentAt = S; ReceivedAt = R; Body = MoveTemp(InBody);
    }
    void FWorldSession::Failed(const FWorldDispatch& Dispatch, bool bTransportUnavailable)
    {
        if (!Current(Dispatch)) return;
        bInFlight = false;
        if (bTransportUnavailable) { Unavailable(EWorldReadStatus::Unavailable); return; }
        if (Dispatch.Operation == EWorldRequest::Result
            || (Dispatch.Operation == RequestedOperation && (Command(RequestedOperation) || RequestedOperation == EWorldRequest::Observe)))
        {
            bLostTicket = true;
            RecoverResult(FPlatformTime::Seconds() + 1);
        }
        else if (Dispatch.Operation == EWorldRequest::Page && !bPageRetried)
        {
            bPageRetried = true;
            Next = EWorldRequest::Page; NextAt = FPlatformTime::Seconds() + 1;
        }
        else Unavailable(EWorldReadStatus::Unavailable);
    }
    bool FWorldSession::RecoverResult(double Earliest)
    {
        if (!bAttemptActive || !bLostTicket || (!Command(RequestedOperation) && RequestedOperation != EWorldRequest::Observe)
            || ResultLookups >= MaxResultLookups || !Live() || !std::isfinite(Earliest) || Earliest >= CandidateDeadline)
        { Unavailable(EWorldReadStatus::Unavailable); return false; }
        Next = EWorldRequest::Result; NextAt = Earliest; return true;
    }
    void FWorldSession::InvalidateContent()
    {
        // Keep entry/visit and request identity, but remove stale render/clock access
        // before retiring its storage. No outward notification can see the old view.
        if (Access) Access->Revoke();
        PendingNotification = {};
        Transport.Reset();
        if (!Preparation.GetCharArray().IsEmpty())
        {
            check(Retirement);
            Retirement->Hold(Preparation, Retirement->Preparations);
            Retirement->TransientCharge += PreparationCharge; PreparationCharge = 0;
        }
        // Stop borrowed page cursors before making their static owner retireable.
        if (Pages) Retire(Pages->Cancel());
        RetireStatic();
        State.Observation = {}; State.bHasObservation = State.bWalkingAvailable = false;
        if (Accepted)
        {
            // Retiring content must not erase accepted ordering within this visit.
            if (Accepted->PlayerRevision > RevisionHighWater)
            { RevisionHighWater = Accepted->PlayerRevision; MoveHighWater = Accepted->MoveSequence; }
            else if (Accepted->PlayerRevision == RevisionHighWater)
                MoveHighWater = FMath::Max(MoveHighWater, Accepted->MoveSequence);
            HealthHighWater = FMath::Max(HealthHighWater, Accepted->HealthRevision);
            SavedHighWater = FMath::Max(SavedHighWater, Accepted->SavedRevision);
            if (Accepted->Publication) Recovery.ClearWorldSubscription(Accepted->Publication->RetiredRecovery, false);
            Retire(MoveTemp(Accepted));
        }
        if (OrderingVersion != MAX_uint64) ++OrderingVersion;
    }
    void FWorldSession::Unavailable(EWorldReadStatus Status)
    {
        bRestartPublication = false; PublicationRestartPhase = 0;
        bInFlight = false; Next = EWorldRequest::None;
        AfterRetirement = EWorldRequest::None;
        bAttemptActive = bLostTicket = false;
        bManifestBodyPending = false;
        RetireBody(); RetireRequest();
        CancelSemantics();
        if (Publication) Publication->Cancel();
        if (Pages) Retire(Pages->Cancel());
        Pages.Reset();
        if (Candidate) Retire(MoveTemp(Candidate));
        RetireStatic(); RetireAttemptText(false);
        State.Status = Status;
        if (!Live() || !Access || !Access->Current())
        {
            if (Access) Access->Revoke();
            State.Observation = {}; State.bHasObservation = false;
        }
    }
    void FWorldSession::Tick(bool bRetirementOnly)
    {
        if (Budget) Budget->BeginTick();
        if (Retirement && Retirement->Advance() != WorldObservationPrivate::EStep::Complete) return;
        if (!RetirePublication()) return;
        if (!bRetirementOnly && bRestartPublication && !RestartPublication()) return;
        if (RetiringClock)
        {
            if (!Budget->Spend(32, false)) return;
            uint64 FinalCharge = 0;
            if (RetiringClock.GetSharedReferenceCount() == 1)
            {
                if (RetiringClock->Retire(*Budget) != WorldObservationPrivate::EStep::Complete) return;
                FinalCharge = RetiringClock->Charge;
            }
            if (!WorldObservationPrivate::QualifiedRetirementDeleteWork
                || !Budget->Spend(WorldObservationPrivate::QualifiedRetirementDeleteWork, false)) return;
            RetiringClock.Reset(); Budget->Release(FinalCharge);
        }
        RetireSemantics();
        if (RetiringStatic && RetiringStatic->Retire() == WorldObservationPrivate::EStep::Complete
            && WorldObservationPrivate::QualifiedRetirementDeleteWork
            && Budget->Spend(WorldObservationPrivate::QualifiedRetirementDeleteWork, false)) RetiringStatic.Reset();
        if (RetiringVisit && RetiringVisit->Retire() == WorldObservationPrivate::EStep::Complete
            && WorldObservationPrivate::QualifiedRetirementDeleteWork
            && Budget->Spend(WorldObservationPrivate::QualifiedRetirementDeleteWork, false)) RetiringVisit.Reset();
        for (auto& Storage : Retiring)
        {
            if (!Storage) continue;
            if (Storage->Retire() == WorldObservationPrivate::EStep::Complete
                && WorldObservationPrivate::QualifiedRetirementDeleteWork
                && Budget->Spend(WorldObservationPrivate::QualifiedRetirementDeleteWork, false)) Storage.Reset();
            break;
        }
        if (bRetirementOnly) return;
        if (!Live())
        {
            if (Entry.CredentialGeneration) Clear(EWorldReadStatus::Expired);
            return;
        }
        const double Now = FPlatformTime::Seconds();
        if (bAttemptActive && (!std::isfinite(Now) || Now >= CandidateDeadline))
        { Unavailable(EWorldReadStatus::Expired); return; }
        if (Static && FPlatformTime::Seconds() >= Static->Deadline)
        {
            InvalidateContent();
            if (bAttemptActive) { Unavailable(EWorldReadStatus::Expired); return; }
        }
        if (AfterRetirement != EWorldRequest::None && !HasRetirement())
        {
            if (!Budget || Budget->TickRemaining < 512) return;
            if (bAttemptActive && !Budget->Spend(512))
            {
                if (Budget->bExhausted) Unavailable(EWorldReadStatus::ResourceUnavailable);
                return;
            }
            const auto Following = AfterRetirement; AfterRetirement = EWorldRequest::None;
            if (!BeginCapture(Following, bAttemptActive)) Unavailable(EWorldReadStatus::Unavailable);
        }
        if (!Body.IsEmpty() || DecodedBody) ProcessBody();
        else if (Candidate && bFinalBindComplete && !bInFlight && Next == EWorldRequest::None) AdvancePublication();
        else if (Candidate && !bFinalBindComplete && !bInFlight && Next == EWorldRequest::None) AdvanceSemantics();
        else if (!Candidate && Pages && !bInFlight && Next == EWorldRequest::None) AdvancePages();
    }
    void FWorldSession::AcceptTicket(const TSharedPtr<FJsonObject>& Ticket, bool bReplacement)
    {
        using namespace StateJsonPrivate;
        const bool bOrigin = Request.Operation == RequestedOperation
            || (Request.Operation == EWorldRequest::Result && bLostTicket);
        const bool bPolling = !Capture.IsEmpty() && (Request.Operation == EWorldRequest::Manifest
            || Request.Operation == EWorldRequest::Page || Request.Operation == EWorldRequest::Bind);
        FString Alias; int64 RetryMs = 0;
        const auto Expected = bReplacement ? EWorldRequest::Prepare : CaptureOperation;
        if (!bAttemptActive || !ReadTicket(Ticket, Expected, RequestId, RequestSequence, Alias, RetryMs)
            || (!Capture.IsEmpty() && !Same(Capture, Alias))
            || (bReplacement && (!bOrigin || RequestedOperation != EWorldRequest::Observe))
            || (!bReplacement && ((!bOrigin && !bPolling) || (bOrigin && CaptureOperation != RequestedOperation))))
        { Unavailable(EWorldReadStatus::InvalidResponse); return; }
        if (bReplacement && CaptureOperation != EWorldRequest::Prepare)
        {
            // A committed ordinary ticket cannot later change kind. Only the
            // original Observe outcome can establish a replacement static capture.
            if (!Capture.IsEmpty() || Candidate || Semantic || TicketArrival > 0)
            { Unavailable(EWorldReadStatus::InvalidResponse); return; }
            InvalidateContent();
            if (bReplacementUsed)
            { Unavailable(EWorldReadStatus::ContentUnavailable); return; }
            bReplacementUsed = true; CaptureOperation = EWorldRequest::Prepare;
            State.Status = EWorldReadStatus::Preparing;
        }
        if (Capture.IsEmpty())
        {
            if (!QualifiedCaptureIdentityBytes || !Budget->Reserve(QualifiedCaptureIdentityBytes))
            { Unavailable(EWorldReadStatus::ResourceUnavailable); return; }
            CaptureCharge = QualifiedCaptureIdentityBytes; Capture = MoveTemp(Alias);
        }
        // A successful original dynamic capture, including pending, anchors its
        // own time. Preparation, lookup and capture polling cannot supply it.
        if (!bReplacement && Request.Operation == RequestedOperation && CaptureOperation == RequestedOperation
            && (RequestedOperation == EWorldRequest::Observe || Command(RequestedOperation)) && !bLostTicket)
        {
            TicketArrival = TicketArrival > 0 ? FMath::Min(TicketArrival, ReceivedAt) : ReceivedAt;
            if (RequestedOperation == EWorldRequest::Observe)
                CandidateDeadline = FMath::Min(CandidateDeadline, TicketArrival + 60);
        }
        // Page/bind polling must not restart an already assembled manifest/tree.
        Next = bPolling ? Request.Operation : EWorldRequest::Manifest;
        NextAt = ReceivedAt + static_cast<double>(RetryMs) / 1000;
        RetireDecodedBody();
    }
    void FWorldSession::ProcessBody()
    {
        using namespace StateJsonPrivate;
        using namespace WorldObservationPrivate;
        if (!Budget) { Unavailable(EWorldReadStatus::ResourceUnavailable); return; }
        if (bManifestBodyPending)
        {
            // The outer header was checked on the preceding slice. Begin admits
            // complete directory work before touching its temporary manifest.
            const EStep Step = Pages ? Pages->Begin(DecodedBody, Capture, Kind(CaptureOperation),
                CaptureOperation == EWorldRequest::Observe ? Static.Get() : nullptr, StaticContext(), Preparation) : EStep::Invalid;
            if (Step == EStep::Invalid || Step == EStep::ResourceUnavailable)
            { Unavailable(Step == EStep::Invalid ? EWorldReadStatus::InvalidResponse : EWorldReadStatus::ResourceUnavailable); return; }
            if (Pages->Manifest())
            { bManifestBodyPending = false; RetireDecodedBody(); State.Status = EWorldReadStatus::Receiving; }
            return;
        }
        if (!DecodedBody)
        {
            if (!bOwnBodyPage)
            {
                if (!Budget->ReservePage()) return;
                bOwnBodyPage = true;
            }
            const auto Decoded = FPages::DecodeBody(Body, DecodedBody, *Budget);
            if (Decoded == EStep::Pending) return;
            if (Decoded != EStep::Complete)
            { Unavailable(Decoded == EStep::ResourceUnavailable ? EWorldReadStatus::ResourceUnavailable : EWorldReadStatus::InvalidResponse); return; }
            check(Retirement && Retirement->Body.GetCharArray().IsEmpty());
            Retirement->Body = MoveTemp(Body);
            Retirement->Field = 0;
        }
        // Includes bounded reply-field temporaries, comparisons, allocations and
        // destruction on every return. Decoding has a separate page admission.
        if (!QualifiedReplyScratchBytes || !QualifiedReplyScratchWork || QualifiedReplyScratchWork > TickWorkLimit)
        { Unavailable(EWorldReadStatus::ResourceUnavailable); return; }
        if (!Budget->Spend(QualifiedReplyScratchWork))
        {
            if (Budget->bExhausted) Unavailable(EWorldReadStatus::ResourceUnavailable);
            return;
        }
        if (!Budget->Reserve(QualifiedReplyScratchBytes))
        { Unavailable(EWorldReadStatus::ResourceUnavailable); return; }
        FAdmittedScratch Scratch{*Budget, QualifiedReplyScratchBytes};
        const auto O = Object(Field(DecodedBody, TEXT("value")));
        FString Status, Outer;
        if (!Exact(DecodedBody, {TEXT("status"), TEXT("value")}) || !ShortText(DecodedBody, TEXT("status"), Outer)
            || !Same(Outer, TEXT("success")) || !ShortText(O, TEXT("status"), Status))
        { Unavailable(EWorldReadStatus::InvalidResponse); return; }
        if (Same(Status, TEXT("slow_down")))
        {
            int64 Delay = 0;
            if (!Exact(O, {TEXT("status"), TEXT("retryAfterMs")}) || !Integer(Field(O, TEXT("retryAfterMs")), Delay)
                || Delay <= 0 || Delay > 900000) { Unavailable(EWorldReadStatus::InvalidResponse); return; }
            const double Earliest = ReceivedAt + static_cast<double>(Delay) / 1000;
            // A throttled command is looked up, never replayed. A throttled
            // original read retains all its arguments and request identity.
            if (Command(Request.Operation) || Request.Operation == EWorldRequest::Result)
            {
                bLostTicket = true;
                if (!RecoverResult(Earliest)) return;
            }
            else { Next = Request.Operation; NextAt = Earliest; }
            RetireDecodedBody(); return;
        }
        if (Same(Status, TEXT("content_changed")) && O->Values.Num() != 1)
        {
            if (!Exact(O, {TEXT("status"), TEXT("preparation")}))
            { Unavailable(EWorldReadStatus::InvalidResponse); return; }
            // The exact changed-content shell revokes stale access even if its
            // proposed replacement ticket is malformed. No alias is promoted.
            InvalidateContent();
            AcceptTicket(Object(Field(O, TEXT("preparation"))), true); return;
        }
        if (Same(Status, TEXT("pending")) || Same(Status, TEXT("captured")))
        {
            AcceptTicket(O, false); return;
        }
        if (Request.Operation == EWorldRequest::Manifest && Same(Status, TEXT("manifest")))
        {
            bManifestBodyPending = true; return;
        }
        if (Request.Operation == EWorldRequest::Page && Same(Status, TEXT("page")))
        {
            const EStep Step = Pages ? Pages->Consume(DecodedBody, 0) : EStep::Invalid;
            if (Step == EStep::Invalid || Step == EStep::ResourceUnavailable)
                Unavailable(Step == EStep::Invalid ? EWorldReadStatus::InvalidResponse : EWorldReadStatus::ResourceUnavailable);
            else
            {
                if (Pages->RetainBody(DecodedBody, BodyCharge))
                { bOwnBodyPage = false; bPageRetried = false; }
            }
            return;
        }
        if (Request.Operation == EWorldRequest::Bind && Same(Status, TEXT("bound")))
        {
            const bool bAccepted = AcceptBinding(O);
            RetireDecodedBody();
            if (!bAccepted) Unavailable(EWorldReadStatus::InvalidResponse);
            else bFinalBindComplete = true;
            return;
        }
        if (Same(Status, TEXT("cancelled")))
        {
            FString Alias, Generation; int64 V = 0;
            if (!Exact(O, {TEXT("v"), TEXT("generation"), TEXT("capture"), TEXT("status")})
                || !Integer(Field(O, TEXT("v")), V) || V != 1
                || !ShortText(O, TEXT("generation"), Generation) || !Same(Generation, ProtocolGeneration)
                || !ShortText(O, TEXT("capture"), Alias, 32) || !Hex(Alias, 32)
                || Capture.IsEmpty() || !Same(Alias, Capture))
            { Unavailable(EWorldReadStatus::InvalidResponse); return; }
            Unavailable(EWorldReadStatus::Unavailable); return;
        }
        if (!Exact(O, {TEXT("status")}) || !FixedFailure(Status))
        { Unavailable(EWorldReadStatus::InvalidResponse); return; }
        // Validate failure framing before allowing it to clear ownership.
        if (Same(Status, TEXT("disabled")) || Same(Status, TEXT("denied")) || Same(Status, TEXT("entry_lost"))
            || Same(Status, TEXT("world_lost")) || Same(Status, TEXT("selection_changed")))
        { Clear(); return; }
        if (Same(Status, TEXT("content_changed")))
        {
            CancelSemantics(); InvalidateContent();
            Unavailable(EWorldReadStatus::ContentUnavailable); return;
        }
        // A failed original Observe/lookup can conceal a failed replacement.
        // Do not retain content/readiness whose unchanged state is now unknown.
        if (RequestedOperation == EWorldRequest::Observe && Capture.IsEmpty()) InvalidateContent();
        const auto Failure = Same(Status, TEXT("resource_unavailable")) ? EWorldReadStatus::ResourceUnavailable
            : Same(Status, TEXT("result_expired")) || Same(Status, TEXT("observation_stale")) ? EWorldReadStatus::Expired
            : EWorldReadStatus::Unavailable;
        Unavailable(Failure);
    }
    void FWorldSession::AdvancePages()
    {
        using namespace WorldObservationPrivate;
        const EStep Step = Pages->Advance();
        if (Step == EStep::Invalid || Step == EStep::ResourceUnavailable)
        { Unavailable(Step == EStep::Invalid ? EWorldReadStatus::InvalidResponse : EWorldReadStatus::ResourceUnavailable); return; }
        TCHAR Root[33]; uint64 Ordinal = 0;
        const auto Cursor = Pages->Next(Root, Ordinal);
        if (Cursor == EStep::Invalid || Cursor == EStep::ResourceUnavailable)
        { Unavailable(Cursor == EStep::Invalid ? EWorldReadStatus::InvalidResponse : EWorldReadStatus::ResourceUnavailable); return; }
        if (Cursor == EStep::Complete) { Next = EWorldRequest::Page; NextAt = FPlatformTime::Seconds(); return; }
        if (Step != EStep::Complete) return;
        Candidate = Pages->TakeComplete();
        AdvanceSemantics();
    }
    void FWorldSession::CancelSemantics()
    {
        bBindingValidated = bFinalBindComplete = false;
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
        if (!WorldObservationPrivate::QualifiedRetirementDeleteWork
            || !Semantic->Storage->Budget->Spend(WorldObservationPrivate::QualifiedRetirementDeleteWork, false)) return false;
        const auto OwnerBudget = Semantic->Storage->Budget;
        Semantic.Reset(); OwnerBudget->Release(QualifiedSemanticDataBytes); return true;
    }
    WorldObservationPrivate::FStaticContext FWorldSession::StaticContext() const
    {
        return {State.Generation, Entry.CredentialGeneration, Entry.EntrySequence, Visit.Get()};
    }
    void FWorldSession::RetireStatic()
    {
        if (!Static) return;
        Static->bAvailable = false;
        check(!RetiringStatic);
        RetiringStatic = MoveTemp(Static);
    }
    void FWorldSession::SeedOrdering()
    {
        if (OrderingVersion == MAX_uint64) return;
        ++OrderingVersion;
        if (Candidate->PlayerRevision > RevisionHighWater)
        { RevisionHighWater = Candidate->PlayerRevision; MoveHighWater = Candidate->MoveSequence; }
        else if (Candidate->PlayerRevision == RevisionHighWater)
            MoveHighWater = FMath::Max(MoveHighWater, Candidate->MoveSequence);
        HealthHighWater = FMath::Max(HealthHighWater, Candidate->HealthRevision);
        SavedHighWater = FMath::Max(SavedHighWater, Candidate->SavedRevision);
    }
    WorldObservationPrivate::EStep FWorldSession::RetainStatic()
    {
        using namespace WorldObservationPrivate;
        using namespace WorldReadPrivate;
        if (RetiringStatic) return EStep::Pending;
        if (!Budget->Spend(2048)) return Budget->bExhausted ? EStep::ResourceUnavailable : EStep::Pending;
        if (!Live() || !Visit || Static || !Candidate || !Semantic || Semantic->bCancelled
            || !Candidate->bComplete || !Candidate->bSemanticsComplete || Candidate->bRetiring
            || Candidate->SemanticGeneration != State.Generation || Candidate->Budget != Budget
            || Candidate->Manifest.PublishBy.IsSet() || !Same(Candidate->Manifest.Kind, TEXT("prepare"))
            || !Same(Candidate->Manifest.Capture, Capture) || !Hex(Capture, 32)) return EStep::Invalid;
        if (!std::isfinite(OriginalDispatchAt) || OriginalDispatchAt <= 0) return EStep::Invalid;
        const auto& Receipt = Semantic->Binding.Receipt();
        const double Duration = FMath::Min(static_cast<double>(Candidate->Manifest.PermissionDeadline), Receipt.Number(ReceiptExpires))
            - Receipt.Number(ReceiptNow);
        const double Deadline = FMath::Min(CandidateDeadline, FMath::Min(Visit->Deadline, OriginalDispatchAt + Duration / 1000));
        if (!std::isfinite(Duration) || Duration <= 0 || !std::isfinite(Deadline) || FPlatformTime::Seconds() >= Deadline)
            return EStep::Invalid;
        static constexpr uint8 Slots[] = {2, 16};
        for (uint8 Slot : Slots)
        {
            const auto* Root = Candidate->Roots[Slot];
            if (!Root || !Root->bVerified || !Root->bClosed || !Root->Owners
                || Root->Directory.Slot != Slot || Root->Directory.Presence != 2) return EStep::Invalid;
            if (Root->Owners == MAX_uint64) return EStep::ResourceUnavailable;
        }
        if (!QualifiedStaticOwnerBytes || !QualifiedPreparationIdentityBytes
            || !QualifiedStaticPreparationWork || QualifiedStaticPreparationWork > TickWorkLimit
            || QualifiedPreparationIdentityBytes > RetainedLimit
            || QualifiedStaticOwnerBytes > RetainedLimit - QualifiedPreparationIdentityBytes) return EStep::ResourceUnavailable;
        if (!Budget->Spend(QualifiedStaticPreparationWork))
            return Budget->bExhausted ? EStep::ResourceUnavailable : EStep::Pending;
        if (!Budget->Reserve(QualifiedStaticOwnerBytes + QualifiedPreparationIdentityBytes)) return EStep::ResourceUnavailable;
        Static = MakeUnique<FStaticRoots>(Budget.ToSharedRef());
        Static->Context = StaticContext(); Static->Deadline = Deadline;
        for (int32 I = 0; I < 32; ++I) Static->Preparation[I] = Capture[I];
        for (int32 I = 0; I < 2; ++I)
        { Static->Roots[I] = Candidate->Roots[Slots[I]]; ++Static->Roots[I]->Owners; }
        Static->bAvailable = true;
        Preparation = Capture;
        PreparationCharge = QualifiedPreparationIdentityBytes;
        CandidateDeadline = FMath::Min(CandidateDeadline, Deadline);
        SeedOrdering();
        return EStep::Complete;
    }
    WorldObservationPrivate::EStep FWorldSession::SettleCommand()
    {
        using namespace WorldObservationPrivate;
        using namespace WorldReadPrivate;
        if (!Budget->Spend(1024)) return Budget->bExhausted ? EStep::ResourceUnavailable : EStep::Pending;
        if (!Command(CaptureOperation) || CaptureOperation != RequestedOperation || !Semantic || Semantic->bCancelled
            || !Candidate->bSemanticsComplete || Candidate->SemanticGeneration != State.Generation
            || Candidate->Manifest.PublishBy.IsSet() || !Same(Candidate->Manifest.Kind, Kind(CaptureOperation))
            || !Same(Candidate->Manifest.Capture, Capture) || !Hex(Capture, 32) || !Hex(RequestId, 32)
            || RequestSequence <= SettledRequestSequence || OperationSequence <= SettledOperationSequence
            || !std::isfinite(OriginalDispatchAt) || OriginalDispatchAt <= 0) return EStep::Invalid;
        // ReceiptNow is the original operation's server anchor. A lookup may
        // recover its evidence, never start that receipt's duration at arrival.
        const auto& Receipt = Semantic->Binding.Receipt();
        const double Duration = FMath::Min(static_cast<double>(Candidate->Manifest.PermissionDeadline), Receipt.Number(ReceiptExpires))
            - Receipt.Number(ReceiptNow);
        const double Deadline = FMath::Min(CandidateDeadline, FMath::Min(Entry.Deadline, OriginalDispatchAt + Duration / 1000));
        if (!std::isfinite(Duration) || Duration <= 0 || !std::isfinite(Deadline) || FPlatformTime::Seconds() >= Deadline)
            return EStep::Invalid;
        if (CaptureOperation == EWorldRequest::Enter)
        {
            if (Visit || RetiringVisit) return EStep::Invalid;
            const auto Copied = Semantic->MaterializeVisit();
            if (Copied != EStep::Complete) return Copied;
        }
        if (!Budget->Spend(1024)) return Budget->bExhausted ? EStep::ResourceUnavailable : EStep::Pending;
        // Recheck after the final copying slice, before any identity or marks
        // become installed. The candidate itself is still wholly private.
        if (!Live() || Semantic->bCancelled || Semantic->Generation != State.Generation
            || FPlatformTime::Seconds() >= Deadline) return EStep::Invalid;
        if (CaptureOperation == EWorldRequest::Enter)
        {
            if (!QualifiedWorldSubscriptionWork || QualifiedWorldSubscriptionWork > TickWorkLimit)
                return EStep::ResourceUnavailable;
            if (!Budget->Spend(QualifiedWorldSubscriptionWork))
                return Budget->bExhausted ? EStep::ResourceUnavailable : EStep::Pending;
            if (!Recovery.BeginWorldSubscription(State.Generation, RecoveryReceipt)) return EStep::Invalid;
            Semantic->EnterVisit->Deadline = Deadline;
            Visit = MoveTemp(Semantic->EnterVisit);
        }
        SettledOperation = CaptureOperation; SettledRequestSequence = RequestSequence;
        SettledOperationSequence = OperationSequence;
        for (int32 I = 0; I < 32; ++I) { SettledRequestId[I] = RequestId[I]; SettledCapture[I] = Capture[I]; }
        bSettledAcknowledged = (CaptureOperation == EWorldRequest::Ready || CaptureOperation == EWorldRequest::PresentationFailed)
            && Semantic->bSameRound;
        CandidateDeadline = FMath::Min(CandidateDeadline, Deadline);
        SeedOrdering();
        return EStep::Complete;
    }
    void FWorldSession::AdvanceSemantics()
    {
        using namespace WorldObservationPrivate;
        const EStep Validated = ValidateCandidate();
        if (Validated == EStep::Pending) return;
        if (Validated != EStep::Complete)
        {
            if (Semantic && Semantic->bLifetimeHealthLost)
            {
                LostHealthPresence = Semantic->RootPresence[15];
                bLostHealthDecodedNull = Semantic->Health.IsNull();
                // Absence or a different life ends the installed visit. Revoke
                // before the credential owner's controller/notification poll.
                Clear(EWorldReadStatus::InvalidResponse);
                return;
            }
            Retire(MoveTemp(Candidate));
            Unavailable(Validated == EStep::ResourceUnavailable ? EWorldReadStatus::ResourceUnavailable : EWorldReadStatus::InvalidResponse);
            return;
        }
        if (CaptureOperation == EWorldRequest::Prepare)
        {
            const auto Retained = RetainStatic();
            if (Retained == EStep::Pending) return;
            if (Retained != EStep::Complete)
            { Unavailable(Retained == EStep::ResourceUnavailable ? EWorldReadStatus::ResourceUnavailable : EWorldReadStatus::InvalidResponse); return; }
            CancelSemantics();
            Retire(MoveTemp(Candidate));
            Pages.Reset();
            // Exactly one fresh dynamic capture follows this preparation. Failure
            // requires explicit reload; static bytes never create a clock sample.
            AfterRetirement = EWorldRequest::Observe;
            State.Status = EWorldReadStatus::Preparing;
            return;
        }
        if (Command(CaptureOperation))
        {
            const auto Settled = SettleCommand();
            if (Settled == EStep::Pending) return;
            if (Settled != EStep::Complete)
            { Unavailable(Settled == EStep::ResourceUnavailable ? EWorldReadStatus::ResourceUnavailable : EWorldReadStatus::InvalidResponse); return; }
            CancelSemantics();
            Retire(MoveTemp(Candidate)); Pages.Reset();
            if (CaptureOperation == EWorldRequest::Enter)
            { AfterRetirement = EWorldRequest::Prepare; State.Status = EWorldReadStatus::Preparing; }
            else
            {
                bAttemptActive = bLostTicket = false;
                // Settled does not mean a complete published world or readiness.
                State.Status = EWorldReadStatus::Unavailable;
            }
            return;
        }
        if (CaptureOperation != EWorldRequest::Observe)
        { Unavailable(EWorldReadStatus::InvalidResponse); return; }
        State.Status = EWorldReadStatus::Binding; Next = EWorldRequest::Bind;
    }

    bool FWorldSession::RetirePublication()
    {
        if (!Publication || !Publication->bCancelled) return true;
        if (!Publication->Owner || Publication->Retire(*Publication->Owner) != WorldObservationPrivate::EStep::Complete)
            return false;
        if (!WorldObservationPrivate::QualifiedRetirementDeleteWork
            || !Budget->Spend(WorldObservationPrivate::QualifiedRetirementDeleteWork, false)) return false;
        const uint64 Charge = Publication->Charge;
        Publication.Reset(); Budget->Release(Charge); return true;
    }

    bool FWorldSession::RestartPublication()
    {
        using namespace WorldObservationPrivate;
        if (!Candidate || !Candidate->bComplete || Candidate->bRetiring || !Live())
        { bRestartPublication = false; Unavailable(EWorldReadStatus::InvalidResponse); return false; }
        if (PublicationRestartPhase == 0 && !RetireSemantics()) return false;
        if (Candidate->ActiveBorrows || Candidate->ActivePreparations) return false;
        if (PublicationRestartPhase == 0 && Candidate->Publication)
        {
            const auto Result = Candidate->Publication->Retire(*Budget);
            if (Result != EStep::Complete)
            {
                if (Result != EStep::Pending) Unavailable(EWorldReadStatus::ResourceUnavailable);
                return false;
            }
            if (!QualifiedRetirementDeleteWork || !Budget->Spend(QualifiedRetirementDeleteWork, false)) return false;
            const uint64 Charge = Candidate->Publication->Charge;
            delete Candidate->Publication; Candidate->Publication = nullptr; Budget->Release(Charge);
        }
        while (PublicationRestartPhase == 0 && Candidate->Players)
        {
            auto* Player = Candidate->Players;
            if (Player->Retire(*Budget) != EStep::Complete || !QualifiedRetirementDeleteWork
                || !Budget->Spend(QualifiedRetirementDeleteWork, false)) return false;
            Candidate->Players = Player->Next; delete Player;
            Budget->Release(WorldReadPrivate::QualifiedSemanticPeerBytes);
        }
        if (PublicationRestartPhase == 0)
        {
            if (FStorage::ReleaseRoot(Candidate->RacingOwner, *Budget) != EStep::Complete) return false;
            Candidate->bSemanticsComplete = false; Candidate->SemanticGeneration = 0;
            PublicationRestartPhase = 1;
        }
        const auto Result = ValidateCandidate();
        if (Result == EStep::Pending) return false;
        if (Result != EStep::Complete)
        {
            bRestartPublication = false;
            if (Semantic && Semantic->bLifetimeHealthLost)
            {
                LostHealthPresence = Semantic->RootPresence[15];
                bLostHealthDecodedNull = Semantic->Health.IsNull();
                Clear(EWorldReadStatus::InvalidResponse); return false;
            }
            Unavailable(Result == EStep::ResourceUnavailable ? EWorldReadStatus::ResourceUnavailable : EWorldReadStatus::InvalidResponse);
            return false;
        }
        bRestartPublication = false;
        PublicationRestartPhase = 0;
        // Reconciliation invalidated only the private projection. The completed
        // Bind and its original arrival remain evidence for this same attempt.
        bFinalBindComplete = bBindingValidated = true;
        return true;
    }

    void FWorldSession::NotifyRecovery(FWorldObservationToken Token)
    {
        if (bInNotification && State.Observation.Token() == Token && State.Observation.IsCurrent())
            Recovery.NotifyWorldSubscription(RecoveryReceipt);
    }

    void FWorldSession::AdvancePublication()
    {
        using namespace WorldObservationPrivate;
        using namespace WorldReadPrivate;
        if (!Live() || !Visit || !Static || !Static->Current(StaticContext()) || !Candidate || !Semantic
            || Semantic->bCancelled || !Candidate->bComplete || !Candidate->bSemanticsComplete
            || Candidate->bRetiring || !bAttemptActive || !bFinalBindComplete || !bBindingValidated
            || RequestedOperation != EWorldRequest::Observe || CaptureOperation != EWorldRequest::Observe
            || !Candidate->Manifest.PublishBy.IsSet() || TicketArrival <= 0 || !std::isfinite(TicketArrival)
            || !Hex(RequestId, 32) || !Hex(Capture, 32) || !Hex(Candidate->Manifest.Digest, 64))
        { Unavailable(EWorldReadStatus::InvalidResponse); return; }
        if (Publication && Publication->bCancelled) return;
        if (!Publication)
        {
            if (!QualifiedPublicationPreparationBytes || !QualifiedPublicationInitializationWork || !QualifiedAccessBytes)
            { Unavailable(EWorldReadStatus::ResourceUnavailable); return; }
            if (QualifiedPublicationInitializationWork > TickWorkLimit)
            { Unavailable(EWorldReadStatus::ResourceUnavailable); return; }
            if (!Budget->Spend(QualifiedPublicationInitializationWork)) return;
            if (!Budget->Reserve(QualifiedPublicationPreparationBytes))
            { Unavailable(EWorldReadStatus::ResourceUnavailable); return; }
            Publication = MakeUnique<FPublicationPreparation>(); Publication->Charge = QualifiedPublicationPreparationBytes;
            auto& Prepared = *Publication;
            Prepared.Owner = Candidate.Get();
            Prepared.Evidence = {State.Generation, Entry.CredentialGeneration, RequestSequence, OperationSequence,
                OrderingVersion, PublicationSerial, Entry.EntrySequence, Visit.Get()};
            Prepared.Evidence.Static = Static.Get(); Prepared.Evidence.Semantic = Semantic.Get();
            Prepared.Evidence.SemanticGeneration = Candidate->SemanticGeneration;
            for (int32 I = 0; I < 32; ++I)
            { Prepared.Evidence.RequestId[I] = RequestId[I]; Prepared.Evidence.Capture[I] = Capture[I]; }
            for (int32 I = 0; I < 64; ++I) Prepared.Evidence.Directory[I] = Candidate->Manifest.Digest[I];
            Prepared.SelectedCharacter = Semantic->SelectedCharacter; Prepared.SelectedRoom = Semantic->SelectedHome;
            Prepared.InstanceHome = Semantic->InstanceHome;
            Prepared.OwnerId = Semantic->Binding.Binding().Field(BindingUser);
            Prepared.ExpectedLife = &Visit->Values[1]; Prepared.ExpectedGameRoom = &Entry.GameRoomId;
            Prepared.Baseline = &ClockBaseline; Prepared.RecoveryOwner = &Recovery; Prepared.RecoveryReceipt = RecoveryReceipt;
            Prepared.TicketArrival = TicketArrival;
            Prepared.ServerNowMs = static_cast<int64>(Semantic->Binding.Receipt().Number(ReceiptNow));
            Prepared.bOwnHome = Semantic->bHome;
        }
        auto& Prepared = *Publication;
        if (Prepared.Evidence.Ordering != OrderingVersion || Prepared.Evidence.AcceptedPublication != PublicationSerial)
        {
            Prepared.Cancel(); CancelSemantics(); bRestartPublication = true; PublicationRestartPhase = 0;
            return;
        }
        if (!Budget->Spend(320)) return;
        bool bExactAttempt = true;
        for (int32 I = 0; I < 32; ++I)
            bExactAttempt &= Prepared.Evidence.RequestId[I] == RequestId[I] && Prepared.Evidence.Capture[I] == Capture[I];
        for (int32 I = 0; I < 64; ++I)
            bExactAttempt &= Prepared.Evidence.Directory[I] == Candidate->Manifest.Digest[I];
        if (Prepared.Owner != Candidate.Get() || Prepared.Evidence.Lifetime != State.Generation
            || Prepared.Evidence.CredentialGeneration != Entry.CredentialGeneration
            || Prepared.Evidence.EntrySequence != Entry.EntrySequence || Prepared.Evidence.Visit != Visit.Get()
            || Prepared.Evidence.Static != Static.Get() || Prepared.Evidence.Semantic != Semantic.Get()
            || Prepared.Evidence.SemanticGeneration != Candidate->SemanticGeneration
            || Prepared.Evidence.RequestSequence != RequestSequence || Prepared.Evidence.OperationSequence != OperationSequence
            || Prepared.Evidence.Ordering != OrderingVersion || Prepared.Evidence.AcceptedPublication != PublicationSerial
            || !bExactAttempt || Prepared.TicketArrival != TicketArrival)
        { Unavailable(EWorldReadStatus::InvalidResponse); return; }
        const auto Started = Candidate->Publication ? EStep::Complete : Prepared.Begin(*Candidate);
        if (Started == EStep::Pending) return;
        if (Started != EStep::Complete)
        { Unavailable(Started == EStep::ResourceUnavailable ? EWorldReadStatus::ResourceUnavailable : EWorldReadStatus::InvalidResponse); return; }
        const auto Result = Prepared.Advance(*Candidate);
        if (Result == EStep::Pending) return;
        if (Result != EStep::Complete)
        { Unavailable(Result == EStep::ResourceUnavailable ? EWorldReadStatus::ResourceUnavailable : EWorldReadStatus::InvalidResponse); return; }
        if (!Access)
        {
            // The cell has no shared reference controller. Weak handles release
            // fixed metadata only; the reserved task performs final destruction.
            if (!QualifiedAccessChargeBytes || !QualifiedAccessInitializationWork || !QualifiedAccessReleaseWork
                || QualifiedAccessReleaseWork > TickWorkLimit
                || QualifiedAccessBytes > RetainedLimit
                || QualifiedAccessChargeBytes > RetainedLimit - QualifiedAccessBytes
                || QualifiedAccessInitializationWork > TickWorkLimit)
            { Unavailable(EWorldReadStatus::ResourceUnavailable); return; }
            if (!Budget->Spend(QualifiedAccessInitializationWork)) return;
            if (!EnsureRetirementTicket(Budget)) { Unavailable(EWorldReadStatus::ResourceUnavailable); return; }
            const int32 Slot = ReserveRetirementTask(*Budget);
            if (Slot == INDEX_NONE) return;
            const uint64 Charge = QualifiedAccessBytes + QualifiedAccessChargeBytes;
            if (!Budget->Reserve(Charge))
            { ReleaseRetirementTask(*Budget, Slot); Unavailable(EWorldReadStatus::ResourceUnavailable); return; }
            Access = MakeUnique<FAccess>(); Access->Budget = Budget;
            Access->Charge = Charge; Access->RetirementSlot = Slot;
        }
        const double Now = FPlatformTime::Seconds();
        const double Deadline = FMath::Min(CandidateDeadline, FMath::Min(Entry.Deadline,
            FMath::Min(Visit->Deadline, FMath::Min(Static->Deadline, TicketArrival + 60))));
        FWorldPresentationTime Time;
        auto& Payload = *Candidate->Publication;
        bool bRetirementRoom = false;
        for (const auto& Slot : Retiring) if (!Slot) bRetirementRoom = true;
        if (!Prepared.Complete() || !Payload.bComplete || !Payload.Clock || !bRetirementRoom || RetiringClock
            || (Accepted && Accepted->ActiveBorrows) || PublicationSerial == MAX_uint64 || OrderingVersion == MAX_uint64
            || !std::isfinite(Now) || !std::isfinite(Deadline) || Now >= Deadline || !Live()
            || !Recovery.CurrentWorldSubscription(RecoveryReceipt)
            || Prepared.Evidence.Ordering != OrderingVersion || Prepared.Evidence.AcceptedPublication != PublicationSerial
            || Candidate->SemanticGeneration != State.Generation || Semantic->Prior != Accepted
            || Candidate->PlayerRevision < RevisionHighWater
            || (Candidate->PlayerRevision == RevisionHighWater && Candidate->MoveSequence < MoveHighWater)
            || Candidate->HealthRevision < HealthHighWater || Candidate->SavedRevision < SavedHighWater)
        { Unavailable(EWorldReadStatus::InvalidResponse); return; }
        const uint64 ClockWork = QualifiedClockWork + 4 * static_cast<uint64>(Payload.Clock->GameRoomId.Len());
        if (!QualifiedClockWork || ClockWork > TickWorkLimit)
        { Unavailable(EWorldReadStatus::ResourceUnavailable); return; }
        if (!Budget->Spend(ClockWork)) return;
        if (!Payload.Clock->Value.CopyTime(Payload.Clock->GameRoomId, Time) || Time.RemainingMs <= 0
            || FPlatformTime::Seconds() >= Deadline)
        { Unavailable(EWorldReadStatus::Expired); return; }
        // Admission is adjacent to the swap, after all clock/provider work.
        if (!QualifiedPublicationCommitWork || QualifiedPublicationCommitWork > TickWorkLimit)
        { Unavailable(EWorldReadStatus::ResourceUnavailable); return; }
        if (!Budget->Spend(QualifiedPublicationCommitWork))
        {
            if (Budget->bExhausted) Unavailable(EWorldReadStatus::ResourceUnavailable);
            return;
        }
        const double CommitNow = FPlatformTime::Seconds();
        if (!std::isfinite(CommitNow) || CommitNow >= Deadline || !Live())
        { Unavailable(EWorldReadStatus::Expired); return; }
        if (!Retirement || Retirement->HasTransient() || Retirement->Publication || Retirement->Semantic || Retirement->Pages)
            return;
        // No callbacks, variable copies or variable destruction follow revocation.
        Access->Revoke(); State.bWalkingAvailable = false;
        TSharedPtr<FStorage> Previous = MoveTemp(Accepted);
        Recovery.CommitWorldSubscription(RecoveryReceipt, MoveTemp(Payload.NextRecovery), Payload.RetiredRecovery);
        if (Previous && Previous->Publication)
            Previous->Publication->RetiredRecovery = MoveTemp(Payload.RetiredRecovery);
        if (ClockBaseline)
        {
            RetiringClock = MoveTemp(ClockBaseline);
        }
        ClockBaseline = Payload.Clock;
        SeedOrdering(); ++PublicationSerial;
        Accepted = MoveTemp(Candidate);
        Access->Storage = Accepted; Access->Token = {State.Generation, PublicationSerial};
        Access->Deadline = Deadline; Access->bReadable = true;
        State.Observation.Access = FAccessWeak(Access.Get()); State.Observation.Captured = Access->Token;
        State.bHasObservation = true; State.Status = EWorldReadStatus::Accepted;
        bAttemptActive = bInFlight = bFinalBindComplete = bBindingValidated = bLostTicket = false;
        Next = AfterRetirement = EWorldRequest::None;
        PendingNotification = Access->Token;
        Prepared.Cancel(); CancelSemantics();
        Retirement->Publication = MoveTemp(Publication);
        Retirement->Semantic = MoveTemp(Semantic);
        Retirement->Pages = MoveTemp(Pages);
        RetireRequest(); RetireBody(); RetireAttemptText(false);
        CaptureOperation = RequestedOperation = EWorldRequest::None;
        bManifestBodyPending = bPageRetried = false;
        TicketArrival = CandidateDeadline = NextAt = OriginalDispatchAt = SentAt = ReceivedAt = 0;
        ReceiptExpiresAt = 0; ResultLookups = 0;
        if (Previous) Retire(MoveTemp(Previous));
    }

    WorldObservationPrivate::EStep FWorldSession::ValidateCandidate()
    {
        using namespace WorldObservationPrivate;
        using namespace WorldReadPrivate;
        if (!Live() || !Candidate || !Candidate->bComplete || Candidate->bRetiring || !Budget
            || Candidate->Budget != Budget || (Accepted && Accepted->Budget != Budget)) return EStep::Invalid;
        if ((CaptureOperation == EWorldRequest::Enter) == static_cast<bool>(Visit)) return EStep::Invalid;
        if (!Semantic)
        {
            if (!QualifiedSemanticDataBytes || !QualifiedSemanticInitializationWork
                || QualifiedSemanticInitializationWork > TickWorkLimit) return EStep::ResourceUnavailable;
            const uint64 Initialization = sizeof(FSemanticData)
                - 23 * (sizeof(FSemanticData::FRecord) - sizeof(FSemanticData::FRef))
                - FDomainRead::DeferredBytes();
            if (Initialization > TickWorkLimit - QualifiedSemanticInitializationWork) return EStep::ResourceUnavailable;
            if (!Budget->Spend(Initialization + QualifiedSemanticInitializationWork))
                return Budget->bExhausted ? EStep::ResourceUnavailable : EStep::Pending;
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
            const double Revision = B.Number(BindingRevision), Sequence = B.Number(BindingSequence);
            // Keep each sequence paired with the revision that established it.
            return (Revision > RevisionHighWater || (Revision == RevisionHighWater && Sequence >= MoveHighWater))
                && B.Number(BindingHealthRevision) >= HealthHighWater && B.Number(BindingSavedRevision) >= SavedHighWater
                && (!Accepted || ((Revision > Accepted->PlayerRevision
                    || (Revision == Accepted->PlayerRevision && Sequence >= Accepted->MoveSequence))
                    && B.Number(BindingHealthRevision) >= Accepted->HealthRevision
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
                if (D.Comparison == 1 && Visit)
                {
                    Step = D.Owner(V.Field(WorldReadPrivate::VisitId), Visit->Values[0], Same);
                    if (Step != EStep::Complete) return FinishStep(Step);
                    if (!Same) return FinishStep(EStep::Invalid);
                    ++D.Comparison;
                }
                if (Visit)
                {
                    Step = D.Owner(V.Field(VisitLife), Visit->Values[1], Same);
                    if (Step != EStep::Complete) return FinishStep(Step);
                    if (!Same)
                    {
                        D.bLifetimeHealthLost = CaptureOperation == EWorldRequest::Observe;
                        return FinishStep(EStep::Invalid);
                    }
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
                    if (D.RootPresence[15] != EWorldFieldPresence::Value)
                    { D.bLifetimeHealthLost = true; return FinishStep(EStep::Invalid); }
                    if (!D.bRecordWork)
                    {
                        if (!D.Spend(sizeof(FHealthRead))) return D.Yield();
                        D.Health.Begin(D.Roots[15]); D.bRecordWork = true;
                    }
                    Step = D.Health.Advance(*Candidate);
                    if (Step != EStep::Complete) return FinishStep(Step);
                    if (D.Health.IsNull())
                    { D.bLifetimeHealthLost = true; return FinishStep(EStep::Invalid); }
                    if (D.Health.Revision() != B.Number(BindingHealthRevision)) return FinishStep(EStep::Invalid);
                    Step = D.Equal(D.Health.Life(), V.Field(VisitLife), Same);
                    if (Step != EStep::Complete) return FinishStep(Step);
                    if (!Same) { D.bLifetimeHealthLost = true; return FinishStep(EStep::Invalid); }
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
                D.Subphase = D.Comparison = 0; D.Phase = 22;
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
        if (!Live() || CaptureOperation != EWorldRequest::Observe || RequestedOperation != EWorldRequest::Observe
            || !Candidate || !bBindingValidated || TicketArrival <= 0 || ReceiptExpiresAt <= 0) return false;
        const auto& M = Candidate->Manifest;
        FString Alias, Digest, Generation; int64 V = 0, At = 0, Permission = 0, Publish = 0, Checked = 0;
        if (!Exact(O, {TEXT("v"), TEXT("generation"), TEXT("capture"), TEXT("status"), TEXT("directoryDigest"),
                TEXT("capturedAt"), TEXT("permissionDeadline"), TEXT("publishBy"), TEXT("checkedAt")})
            || !Integer(Field(O, TEXT("v")), V) || V != 1 || !ShortText(O, TEXT("generation"), Generation)
            || !Same(Generation, ProtocolGeneration) || !ShortText(O, TEXT("capture"), Alias, 32) || !Same(Alias, M.Capture)
            || !ShortText(O, TEXT("directoryDigest"), Digest) || !Same(Digest, M.Digest)
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
    void FWorldSession::ContentLost(FWorldObservationToken ObservationToken)
    {
        if (!ObservationToken.IsSet() || ObservationToken != State.Observation.Token()) return;
        StopWalking();
        InvalidateContent();
        Unavailable(EWorldReadStatus::ContentUnavailable);
    }
}
