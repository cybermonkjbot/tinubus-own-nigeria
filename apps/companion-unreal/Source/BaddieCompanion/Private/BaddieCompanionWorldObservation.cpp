#include "BaddieCompanionWorldObservation.h"
#include "BaddieCompanionWorldObservationPages.h"
#include "BaddieCompanionWorldPlayerRead.h"
#include "BaddieCompanionWorldPublication.h"
#include "BaddieCompanionSnapshot.h"
#include "Dom/JsonValue.h"
#include "Containers/Ticker.h"
#include "CoreGlobals.h"
#include "HAL/PlatformTime.h"

#include <cmath>

namespace BaddieCompanion::WorldReadPrivate
{
    using namespace WorldObservationPrivate;

    EStep FPlayerResult::Retire(FBudget& Budget)
    {
        if (!bPinned) return EStep::Complete;
        while (RetireField < 29)
        {
            if (!Budget.Spend(8, false)) return EStep::Pending;
            if (FStorage::ReleaseRoot(Reference(RetireField).Owner, Budget) != EStep::Complete) return EStep::Pending;
            ++RetireField;
        }
        return EStep::Complete;
    }

    bool TextValue(const FString& Value, const FTextRule& Rule)
    {
        const uint64 Count = static_cast<uint64>(Value.Len());
        if (!Rule.Length(Count)) return false;
        bool bNonWhitespace = false;
        for (uint64 I = 0; I < Count; ++I)
        {
            const auto Unit = static_cast<uint16>(Value[static_cast<int32>(I)]);
            if (!Rule.Unit(Unit, I, Count)) return false;
            bNonWhitespace |= !FChar::IsWhitespace(static_cast<TCHAR>(Unit));
        }
        return !Rule.bHasNonWhitespace || bNonWhitespace;
    }

    namespace
    {
        EStep Yield(const FStorage& Storage)
        {
            return Storage.Budget->bExhausted ? EStep::ResourceUnavailable : EStep::Pending;
        }
        bool Readable(const FStorage& Storage)
        {
            return Storage.bComplete && !Storage.bRetiring;
        }
    }

    void FTextRead::Cancel() { *this = FTextRead(); }
    void FTextRead::Begin(FValueRef Value, FTextRule InRule)
    {
        Cancel(); Left = Value; Rule = InRule; Status = EStep::Pending;
    }
    void FTextRead::Compare(FValueRef A, FValueRef B)
    {
        Begin(A, {}); Right = B; bCompare = true;
    }
    void FTextRead::Compare(FValueRef A, const FStorage& Previous, FValueRef B)
    {
        Compare(A, B); OtherOwner = &Previous;
    }
    void FTextRead::ComparePrefixed(FValueRef A, FValueRef B, const TCHAR* Prefix, uint64 PrefixLength)
    {
        Compare(A, B); Literal = Prefix; LiteralCount = PrefixLength;
    }
    void FTextRead::StartsWith(FValueRef A, const TCHAR* Prefix, uint64 Length)
    { Compare(A, Prefix, Length); bPrefixOnly = true; }
    void FTextRead::Compare(FValueRef A, const FString& B)
    {
        Compare(A, *B, static_cast<uint64>(B.Len()));
    }
    void FTextRead::Compare(FValueRef A, const TCHAR* B, uint64 Length)
    {
        Begin(A, {}); Literal = B; LiteralCount = Length; bCompare = true;
    }
    EStep FTextRead::Advance(const FStorage& Storage)
    {
        if (Owner && Owner != &Storage) return Status = EStep::Invalid;
        Owner = &Storage;
        if (!Readable(Storage)) return Status = EStep::Invalid;
        if (OtherOwner && (!Readable(*OtherOwner) || OtherOwner->Budget != Storage.Budget)) return Status = EStep::Invalid;
        if (Status != EStep::Pending) return Status;
        if (!bStarted)
        {
            // Each Get includes root and node-page index access plus descriptor reads.
            if (!Storage.Budget->Spend(16)) return Yield(Storage);
            const auto* A = Resolve(Storage, Left);
            if (!A || A->Kind != EWorldValueKind::String || !Rule.Length(A->Count))
                return Status = EStep::Invalid;
            Count = A->Count; LeftBlock = A->Text;
            if (bCompare)
            {
                uint64 OtherCount = LiteralCount;
                if (Right.IsSet())
                {
                    const auto* B = Resolve(OtherOwner ? *OtherOwner : Storage, Right);
                    if (!B || B->Kind != EWorldValueKind::String) return Status = EStep::Invalid;
                    if (LiteralCount > MAX_uint64 - B->Count) return Status = EStep::Invalid;
                    OtherCount = B->Count + LiteralCount; RightBlock = B->Text;
                }
                else if (!Literal) return Status = EStep::Invalid;
                if (bPrefixOnly && Count >= OtherCount) Count = OtherCount;
                if (Count != OtherCount) { bEqual = false; return Status = EStep::Complete; }
            }
            bStarted = true;
        }
        while (Offset < Count)
        {
            // Includes both unit reads, comparison/classification and block links.
            if (!Storage.Budget->Spend(8)) return Yield(Storage);
            if (!LeftBlock || LeftOffset >= LeftBlock->Used) return Status = EStep::Invalid;
            const uint16 A = LeftBlock->Units[LeftOffset++];
            if (!Rule.Unit(A, Offset, Count)) return Status = EStep::Invalid;
            bNonWhitespace |= !FChar::IsWhitespace(static_cast<TCHAR>(A));
            if (bCompare)
            {
                uint16 B = 0;
                if (Right.IsSet())
                {
                    if (Literal && Offset < LiteralCount) B = static_cast<uint16>(Literal[Offset]);
                    else
                    {
                        if (!RightBlock || RightOffset >= RightBlock->Used) return Status = EStep::Invalid;
                        B = RightBlock->Units[RightOffset++];
                        if (RightOffset == RightBlock->Used) { RightBlock = RightBlock->Next; RightOffset = 0; }
                    }
                }
                else B = static_cast<uint16>(Literal[Offset]);
                if (A != B) { bEqual = false; return Status = EStep::Complete; }
            }
            ++Offset;
            if (LeftOffset == LeftBlock->Used) { LeftBlock = LeftBlock->Next; LeftOffset = 0; }
        }
        if ((!bPrefixOnly && LeftBlock) || (Right.IsSet() && RightBlock) || (Rule.bHasNonWhitespace && !bNonWhitespace))
            return Status = EStep::Invalid;
        return Status = EStep::Complete;
    }

    void FObjectRead::Cancel() { *this = FObjectRead(); }
    void FObjectRead::Begin(FValueRef InObject, FFieldSlot* InSlots, uint32 InCount, bool bInRejectUnknown)
    {
        Cancel(); Object = InObject; Slots = InSlots; Count = InCount;
        bRejectUnknown = bInRejectUnknown; Status = EStep::Pending;
    }
    EStep FObjectRead::Advance(const FStorage& Storage)
    {
        if (!Readable(Storage)) return Status = EStep::Invalid;
        if (Status != EStep::Pending) return Status;
        if (!Slots && Count) return Status = EStep::Invalid;
        while (Cleared < Count)
        {
            if (!Storage.Budget->Spend(sizeof(FValueRef) + 8)) return Yield(Storage);
            if (!Slots[Cleared].Name || !Slots[Cleared].NameLength) return Status = EStep::Invalid;
            Slots[Cleared++].Value = {};
        }
        if (!bStarted)
        {
            if (!Storage.Budget->Spend(8)) return Yield(Storage);
            const auto* O = Resolve(Storage, Object);
            if (!O || O->Kind != EWorldValueKind::Object) return Status = EStep::Invalid;
            Key = O->FirstChild; bStarted = true;
        }
        while (Key)
        {
            if (bAdvancing)
            {
                if (!Storage.Budget->Spend(8)) return Yield(Storage);
                const auto* V = Resolve(Storage, Object.Child(Value));
                if (!V) return Status = EStep::Invalid;
                Key = V->NextSibling; Value = 0; Slot = 0; bComparing = false;
                bAdvancing = false; ++Seen;
                continue;
            }
            if (!Value)
            {
                if (!Storage.Budget->Spend(16)) return Yield(Storage);
                const auto* K = Resolve(Storage, Object.Child(Key));
                const auto* V = K ? Resolve(Storage, Object.Child(K->NextSibling)) : nullptr;
                if (!K || K->Kind != EWorldValueKind::String || K->Role != 1 || K->Parent != Object.Node
                    || !V || V->Role != 2 || V->Parent != Object.Node || V->Slot != Seen || K->Slot != Seen)
                    return Status = EStep::Invalid;
                Value = V->Id;
            }
            bool bFound = false;
            while (Slot < Count)
            {
                if (!bComparing)
                {
                    if (!Storage.Budget->Spend(2 * sizeof(FTextRead) + 64)) return Yield(Storage);
                    Comparison.Compare(Object.Child(Key), Slots[Slot].Name, Slots[Slot].NameLength);
                    bComparing = true;
                }
                const auto Result = Comparison.Advance(Storage);
                if (Result != EStep::Complete) return Result == EStep::Pending ? Result : Status = Result;
                if (Comparison.Equal())
                {
                    if (!Storage.Budget->Spend(2 * sizeof(FValueRef) + 8)) return Yield(Storage);
                    // Structural assembly rejects duplicates too; do not permit a
                    // second recognized field to replace the first in this adapter.
                    if (Slots[Slot].Value.IsSet()) return Status = EStep::Invalid;
                    Slots[Slot].Value = Object.Child(Value); bFound = true; break;
                }
                ++Slot; bComparing = false;
            }
            if (!bFound && bRejectUnknown) return Status = EStep::Invalid;
            bAdvancing = true;
        }
        while (Required < Count)
        {
            if (!Storage.Budget->Spend(3)) return Yield(Storage);
            const auto& S = Slots[Required++];
            if (S.bRequired && !S.Value.IsSet()) return Status = EStep::Invalid;
        }
        if (!Storage.Budget->Spend(8)) return Yield(Storage);
        const auto* O = Resolve(Storage, Object);
        return Status = O && Seen == O->Count ? EStep::Complete : EStep::Invalid;
    }

    EStep ReadNumber(const FStorage& Storage, FValueRef Value, double& Out)
    {
        if (!Readable(Storage)) return EStep::Invalid;
        if (!Storage.Budget->Spend(8 + sizeof(double))) return Yield(Storage);
        const auto* N = Resolve(Storage, Value);
        if (!N || N->Kind != EWorldValueKind::Number) return EStep::Invalid;
        double Number = 0;
        FMemory::Memcpy(&Number, &N->NumberBits, sizeof(Number));
        if (!FiniteNumber(Number)) return EStep::Invalid;
        Out = Number;
        return EStep::Complete;
    }
    EStep ReadKind(const FStorage& Storage, FValueRef Value, EWorldValueKind& Out)
    {
        if (!Readable(Storage)) return EStep::Invalid;
        if (!Storage.Budget->Spend(8)) return Yield(Storage);
        const auto* N = Resolve(Storage, Value);
        if (!N) return EStep::Invalid;
        Out = N->Kind;
        return EStep::Complete;
    }

    bool ReadRecord(const TSharedPtr<FJsonObject>& Object, const FRecordSchema& Schema)
    {
        if (!Object || Schema.Count > FRecordFacts::Capacity) return false;
        FRecordFacts Facts;
        if (Schema.bRejectUnknown)
        {
            for (const auto& Pair : Object->Values)
            {
                bool bKnown = false;
                for (uint32 I = 0; I < Schema.Count; ++I)
                {
                    const auto& Rule = Schema.Fields[I];
                    if (Pair.Key.Len() == Rule.NameLength
                        && FCString::Strncmp(*Pair.Key, Rule.Name, Rule.NameLength) == 0)
                    { bKnown = true; break; }
                }
                if (!bKnown) return false;
            }
        }
        for (uint32 I = 0; I < Schema.Count; ++I)
        {
            const auto& Rule = Schema.Fields[I];
            Facts.Choices[I] = -1;
            if (Rule.ConditionField != MAX_uint32)
            {
                if (Rule.ConditionField >= I) return false;
                if (Facts.Choices[Rule.ConditionField] != Rule.ConditionChoice) continue;
            }
            const auto Value = Object->TryGetField(Rule.Name);
            if (!Value) { if (Rule.bRequired) return false; continue; }
            if (Value->Type == EJson::Null)
            {
                if (!Rule.bNullable) return false;
                Facts.Presences[I] = EWorldFieldPresence::Null;
                continue;
            }
            Facts.Presences[I] = EWorldFieldPresence::Value;
            switch (Rule.Kind)
            {
            case EWorldValueKind::Object:
                if (Value->Type != EJson::Object || !Value->AsObject()) return false;
                break;
            case EWorldValueKind::Array:
                if (Value->Type != EJson::Array || static_cast<uint64>(Value->AsArray().Num()) < Rule.MinCount
                    || static_cast<uint64>(Value->AsArray().Num()) > Rule.MaxCount) return false;
                Facts.Counts[I] = Value->AsArray().Num();
                break;
            case EWorldValueKind::Number:
            {
                double N = 0;
                if (Value->Type != EJson::Number || !Value->TryGetNumber(N) || !Rule.Number(N)) return false;
                Facts.Numbers[I] = N;
                break;
            }
            case EWorldValueKind::String:
            {
                FString S;
                if (Value->Type != EJson::String || !Value->TryGetString(S) || !TextValue(S, Rule.Text)) return false;
                Facts.Counts[I] = S.Len();
                if (Rule.ChoiceCount)
                {
                    bool bKnown = false;
                    for (uint32 C = 0; C < Rule.ChoiceCount; ++C)
                        if (S.Len() == Rule.Choices[C].Count
                            && FCString::Strncmp(*S, Rule.Choices[C].Text, S.Len()) == 0)
                        { Facts.Choices[I] = C; bKnown = true; break; }
                    if (!bKnown) return false;
                }
                break;
            }
            case EWorldValueKind::Boolean:
                if (Value->Type != EJson::Boolean) return false;
                Facts.Numbers[I] = Value->AsBool() ? 1 : 0;
                break;
            default: return false;
            }
        }
        return !Schema.Check || Schema.Check(Facts);
    }

    void FRecordRead::Cancel()
    {
        // No owning values. Old slots are cleared incrementally by Initialize.
        Fields.Cancel(); Text.Cancel(); Schema = nullptr; Object = {};
        Initialized = Index = ChoiceIndex = 0; bTextStarted = false;
        Phase = EPhase::Done; Status = EStep::Invalid;
    }
    void FRecordRead::Begin(FValueRef InObject, const FRecordSchema& InSchema)
    {
        Cancel(); Object = InObject; Schema = &InSchema;
        Phase = EPhase::Initialize; Status = EStep::Pending;
    }
    EStep FRecordRead::Advance(const FStorage& Storage)
    {
        if (!Readable(Storage)) return Status = EStep::Invalid;
        if (Status != EStep::Pending) return Status;
        if (!Schema || Schema->Count > Capacity) return Status = EStep::Invalid;
        while (true)
        {
            if (Phase == EPhase::Initialize)
            {
                while (Initialized < Schema->Count)
                {
                    if (!Storage.Budget->Spend(sizeof(FFieldSlot) + sizeof(double) + sizeof(uint64) + sizeof(int32) + 1)) return Yield(Storage);
                    const auto& Rule = Schema->Fields[Initialized];
                    auto& Slot = Slots[Initialized];
                    Slot.Name = Rule.Name; Slot.NameLength = Rule.NameLength;
                    Slot.bRequired = Rule.bRequired; Slot.Value = {};
                    Facts.Numbers[Initialized] = 0; Facts.Choices[Initialized] = -1; Facts.Counts[Initialized] = 0;
                    Facts.Presences[Initialized++] = EWorldFieldPresence::Missing;
                }
                if (!Storage.Budget->Spend(2 * sizeof(FObjectRead) + 64)) return Yield(Storage);
                Fields.Begin(Object, Slots, Schema->Count, Schema->bRejectUnknown); Phase = EPhase::Fields;
            }
            if (Phase == EPhase::Fields)
            {
                const auto Result = Fields.Advance(Storage);
                if (Result != EStep::Complete) return Result == EStep::Pending ? Result : Status = Result;
                Phase = EPhase::Kind;
            }
            if (Index == Schema->Count)
            {
                // Schema callbacks inspect only this fixed set of scalar facts.
                // They cannot walk fields, compare strings or allocate projections.
                if (!Storage.Budget->Spend(Capacity * 8)) return Yield(Storage);
                Phase = EPhase::Done;
                return Status = !Schema->Check || Schema->Check(Facts) ? EStep::Complete : EStep::Invalid;
            }
            const auto& Rule = Schema->Fields[Index];
            if (Phase == EPhase::Kind)
            {
                if (!Storage.Budget->Spend(2 * sizeof(FTextRead) + 128)) return Yield(Storage);
                if (Rule.ConditionField != MAX_uint32)
                {
                    if (Rule.ConditionField >= Index) return Status = EStep::Invalid;
                    if (Facts.Choices[Rule.ConditionField] != Rule.ConditionChoice)
                    { Phase = EPhase::Advance; continue; }
                }
                const auto Value = Slots[Index].Value;
                if (!Value.IsSet()) { Phase = EPhase::Advance; }
                else
                {
                    const auto* Node = Resolve(Storage, Value);
                    if (!Node) return Status = EStep::Invalid;
                    if (Node->Kind == EWorldValueKind::Null)
                    {
                        if (!Rule.bNullable) return Status = EStep::Invalid;
                        Facts.Presences[Index] = EWorldFieldPresence::Null; Phase = EPhase::Advance;
                    }
                    else
                    {
                        if (Node->Kind != Rule.Kind) return Status = EStep::Invalid;
                        Facts.Presences[Index] = EWorldFieldPresence::Value; Phase = EPhase::Advance;
                        Facts.Counts[Index] = Node->Count;
                        if (Rule.Kind == EWorldValueKind::Number)
                        {
                            FMemory::Memcpy(&Facts.Numbers[Index], &Node->NumberBits, sizeof(double));
                            if (!Rule.Number(Facts.Numbers[Index])) return Status = EStep::Invalid;
                        }
                        else if (Rule.Kind == EWorldValueKind::Boolean) Facts.Numbers[Index] = Node->Count;
                        else if (Rule.Kind == EWorldValueKind::Array
                            && (Node->Count < Rule.MinCount || Node->Count > Rule.MaxCount)) return Status = EStep::Invalid;
                        else if (Rule.Kind == EWorldValueKind::String)
                        { Text.Begin(Value, Rule.Text); Phase = EPhase::Text; }
                    }
                }
            }
            if (Phase == EPhase::Text)
            {
                const auto Result = Text.Advance(Storage);
                if (Result != EStep::Complete) return Result == EStep::Pending ? Result : Status = Result;
                Phase = Rule.ChoiceCount ? EPhase::Choices : EPhase::Advance;
            }
            if (Phase == EPhase::Choices)
            {
                if (ChoiceIndex == Rule.ChoiceCount) return Status = EStep::Invalid;
                if (!bTextStarted)
                {
                    if (!Storage.Budget->Spend(2 * sizeof(FTextRead) + 64)) return Yield(Storage);
                    Text.Compare(Slots[Index].Value, Rule.Choices[ChoiceIndex].Text, Rule.Choices[ChoiceIndex].Count);
                    bTextStarted = true;
                }
                const auto Result = Text.Advance(Storage);
                if (Result != EStep::Complete) return Result == EStep::Pending ? Result : Status = Result;
                if (Text.Equal()) { Facts.Choices[Index] = ChoiceIndex; Phase = EPhase::Advance; }
                else { ++ChoiceIndex; bTextStarted = false; continue; }
            }
            if (Phase == EPhase::Advance)
            {
                if (!Storage.Budget->Spend(1)) return Yield(Storage);
                ++Index; ChoiceIndex = 0; bTextStarted = false; Phase = EPhase::Kind;
            }
        }
    }

    namespace
    {
        const FRecordSchema& BindingSchema()
        {
            using K = EWorldValueKind;
            static constexpr FFieldRule Rules[] = {
                FFieldRule(TEXT("receipt"), K::Object), FFieldRule(TEXT("userId"), K::String).WithText({1}),
                FFieldRule(TEXT("approvingSessionId"), K::String).WithText({1}),
                FFieldRule(TEXT("participantId"), K::String).WithText({1}),
                FFieldRule(TEXT("callSessionId"), K::String).WithText({1}),
                FFieldRule(TEXT("expressionSessionId"), K::String).WithText({1}),
                FFieldRule(TEXT("reservationGeneration"), K::String).WithText({1}),
                FFieldRule(TEXT("characterId"), K::String).WithText({1}),
                FFieldRule(TEXT("homeRoomId"), K::String).WithText({1}),
                FFieldRule(TEXT("gameRoomId"), K::String).WithText({1}),
                FFieldRule(TEXT("savedRevision"), K::Number).WithNumber(0, MaxSafeInteger, true),
                FFieldRule(TEXT("instanceId"), K::String).WithText({1}),
                FFieldRule(TEXT("playerRevision"), K::Number).WithNumber(0, MaxSafeInteger, true),
                FFieldRule(TEXT("moveSequence"), K::Number).WithNumber(0, MaxSafeInteger, true),
                FFieldRule(TEXT("healthRevision"), K::Number).WithNumber(0, MaxSafeInteger, true),
                FFieldRule(TEXT("presentationInstanceId"), K::String).WithText({1}),
                FFieldRule(TEXT("readyInstanceId"), K::String, true, true).WithText({1}),
                FFieldRule(TEXT("pendingHomeArrival"), K::Object, true, true),
                FFieldRule(TEXT("capturedAt"), K::Number).WithNumber(0, MaxSafeInteger, true),
                FFieldRule(TEXT("permissionDeadline"), K::Number).WithNumber(0, MaxSafeInteger, true),
                FFieldRule(TEXT("publishBy"), K::Number, true, true).WithNumber(0, MaxSafeInteger, true)
            };
            static constexpr FRecordSchema Schema(Rules);
            return Schema;
        }
        const FRecordSchema& ReceiptSchema()
        {
            using K = EWorldValueKind;
            static constexpr FTextChoice Scopes[] = {TEXT("companion_world_control")};
            static constexpr FFieldRule Rules[] = {
                FFieldRule(TEXT("scope"), K::String).WithChoices(Scopes),
                FFieldRule(TEXT("grantId"), K::String).WithText({1}),
                FFieldRule(TEXT("entrySequence"), K::Number).WithNumber(1, MaxSafeInteger, true),
                FFieldRule(TEXT("visit"), K::Object),
                FFieldRule(TEXT("expiresAt"), K::Number).WithNumber(1, MaxSafeInteger, true),
                FFieldRule(TEXT("serverNow"), K::Number).WithNumber(1, MaxSafeInteger, true)
            };
            static constexpr FRecordSchema Schema(Rules);
            return Schema;
        }
        const FRecordSchema& VisitSchema()
        {
            using K = EWorldValueKind;
            static constexpr FFieldRule Rules[] = {
                FFieldRule(TEXT("id"), K::String).WithText({1}), FFieldRule(TEXT("lifeId"), K::String).WithText({1}),
                FFieldRule(TEXT("instanceId"), K::String).WithText({1}),
                FFieldRule(TEXT("revision"), K::Number).WithNumber(0, MaxSafeInteger, true)
            };
            static constexpr FRecordSchema Schema(Rules);
            return Schema;
        }
        const FRecordSchema& PendingHomeSchema()
        {
            using K = EWorldValueKind;
            static constexpr FFieldRule Rules[] = {
                FFieldRule(TEXT("instanceId"), K::String).WithText({1}), FFieldRule(TEXT("eventKey"), K::String).WithText({1}),
                FFieldRule(TEXT("revision"), K::Number).WithNumber(0, MaxSafeInteger, true)
            };
            static constexpr FRecordSchema Schema(Rules);
            return Schema;
        }
    }
    void FBindingRead::Cancel()
    {
        BindingRecord.Cancel(); ReceiptRecord.Cancel(); VisitRecord.Cancel(); PendingRecord.Cancel();
        Phase = 0; Status = EStep::Invalid;
    }
    void FBindingRead::Begin(FValueRef Value)
    {
        Cancel(); BindingRecord.Begin(Value, BindingSchema()); Status = EStep::Pending;
    }
    EStep FBindingRead::Advance(const FStorage& Storage)
    {
        if (!Readable(Storage)) return Status = EStep::Invalid;
        if (Status != EStep::Pending) return Status;
        if (Phase == 0)
        {
            const auto Result = BindingRecord.Advance(Storage);
            if (Result != EStep::Complete) return Result == EStep::Pending ? Result : Status = Result;
            if (!Storage.Budget->Spend(FRecordRead::BeginWork())) return Yield(Storage);
            ReceiptRecord.Begin(BindingRecord.Field(BindingReceipt), ReceiptSchema()); Phase = 1;
        }
        if (Phase == 1)
        {
            const auto Result = ReceiptRecord.Advance(Storage);
            if (Result != EStep::Complete) return Result == EStep::Pending ? Result : Status = Result;
            if (!Storage.Budget->Spend(FRecordRead::BeginWork())) return Yield(Storage);
            VisitRecord.Begin(ReceiptRecord.Field(ReceiptVisit), VisitSchema()); Phase = 2;
        }
        if (Phase == 2)
        {
            const auto Result = VisitRecord.Advance(Storage);
            if (Result != EStep::Complete) return Result == EStep::Pending ? Result : Status = Result;
            if (BindingRecord.Presence(BindingPendingHome) == EWorldFieldPresence::Null) return Status = EStep::Complete;
            if (!Storage.Budget->Spend(FRecordRead::BeginWork())) return Yield(Storage);
            PendingRecord.Begin(BindingRecord.Field(BindingPendingHome), PendingHomeSchema()); Phase = 3;
        }
        const auto Result = PendingRecord.Advance(Storage);
        if (Result != EStep::Pending) Status = Result;
        return Result;
    }

    void FDomainRead::Cancel()
    {
        // Frames borrow immutable nodes and static schemas. Revocation only drops
        // the active depth; reinitialization clears used fields under the budget.
        Depth = 0; Status = EStep::Invalid;
        RacingInstance = {}; RacingUpdatedAt = 0;
        SelectedLibrary = SelectedCharacter = SelectedHome = SelectedActiveRoom = {};
    }
    void FDomainRead::Begin(FValueRef Value, EDomain Domain)
    {
        Cancel(); StartValue = Value; StartDomain = Domain;
        bBegin = true; Status = EStep::Pending;
    }
    EStep FDomainRead::Advance(const FStorage& Storage)
    {
        if (!Readable(Storage)) return Status = EStep::Invalid;
        if (Status != EStep::Pending) return Status;
        while (true)
        {
            if (!Storage.Budget->Spend(4)) return Yield(Storage);
            if (!InitializedFrames)
            {
                if (!Storage.Budget->Spend(sizeof(FFrame))) return Yield(Storage);
                new (Frames[0].Bytes) FFrame; ++InitializedFrames;
            }
            if (bBegin)
            {
                FrameAt(0).Value = StartValue; FrameAt(0).Domain = StartDomain;
                FrameAt(0).Phase = 0; bBegin = false;
            }
            auto& Frame = FrameAt(Depth);
            const auto& Schema = DomainSchema(Frame.Domain);
            if (Frame.Phase == 0)
            {
                if (!Storage.Budget->Spend(80 + (Schema.Record ? FRecordRead::BeginWork() : 0))) return Yield(Storage);
                const auto* Node = Resolve(Storage, Frame.Value);
                if (!Node || Node->Kind != Schema.Kind) return Status = EStep::Invalid;
                Frame.Child = 0; Frame.Index = 0; Frame.Next = Node->FirstChild; Frame.Count = Node->Count;
                Frame.Relation = 0; Frame.bComparing = false; Frame.Lanes = 0; Frame.MovementPointCount = 0;
                Frame.Selected = 0;
                if (Schema.Kind == EWorldValueKind::Number)
                {
                    double N = 0;
                    FMemory::Memcpy(&N, &Node->NumberBits, sizeof(N));
                    if (Schema.bInteger ? !IntegerNumber(N, Schema.Minimum, Schema.Maximum)
                        : !FiniteNumber(N) || N < Schema.Minimum || N > Schema.Maximum) return Status = EStep::Invalid;
                    Frame.Scalar = N;
                    Frame.Phase = 3;
                }
                else if (Schema.Kind == EWorldValueKind::Array)
                {
                    if (Node->Count < Schema.MinCount || Node->Count > Schema.MaxCount) return Status = EStep::Invalid;
                    Frame.Phase = 2;
                }
                else if (Schema.Kind == EWorldValueKind::Object && Schema.Record)
                { Frame.Record.Begin(Frame.Value, *Schema.Record); Frame.Phase = 1; }
                else return Status = EStep::Invalid;
            }
            if (Frame.Phase == 1)
            {
                const auto Result = Frame.Record.Advance(Storage);
                if (Result != EStep::Complete) return Result == EStep::Pending ? Result : Status = Result;
                Frame.Phase = 2;
            }
            if (Frame.Phase == 2)
            {
                // Admit a possible child frame before advancing any parent cursor.
                if (Depth + 1 < FrameCount && InitializedFrames <= Depth + 1)
                {
                    if (!Storage.Budget->Spend(sizeof(FFrame))) return Yield(Storage);
                    new (Frames[InitializedFrames].Bytes) FFrame; ++InitializedFrames;
                }
                FValueRef Child;
                EDomain ChildDomain = EDomain::Number;
                uint32 ParentField = MAX_uint32;
                if (Schema.Kind == EWorldValueKind::Array)
                {
                    if (Frame.Index == Frame.Count)
                    {
                        if (Frame.Next) return Status = EStep::Invalid;
                        Frame.Phase = 3;
                    }
                    else
                    {
                        if (!Storage.Budget->Spend(16)) return Yield(Storage);
                        const auto* Node = Resolve(Storage, Frame.Value.Child(Frame.Next));
                        if (!Node || Node->Role != 3 || Node->Parent != Frame.Value.Node || Node->Slot != Frame.Index)
                            return Status = EStep::Invalid;
                        Child = Frame.Value.Child(Node->Id); ChildDomain = Schema.Element;
                        Frame.Next = Node->NextSibling; ++Frame.Index;
                    }
                }
                else if (Frame.Child == Schema.ChildCount) Frame.Phase = 3;
                else
                {
                    if (!Storage.Budget->Spend(8)) return Yield(Storage);
                    const auto& Rule = Schema.Children[Frame.Child++];
                    if (Frame.Record.Presence(Rule.Field) != EWorldFieldPresence::Value) continue;
                    Child = Frame.Record.Field(Rule.Field); ChildDomain = Rule.Domain;
                    ParentField = Rule.Field;
                }
                if (Child.IsSet())
                {
                    if (Depth + 1 >= FrameCount) return Status = EStep::Invalid;
                    auto& Next = FrameAt(++Depth);
                    Next.Value = Child; Next.Domain = ChildDomain; Next.Phase = 0;
                    Next.ParentField = ParentField;
                    continue;
                }
            }
            if (Frame.Phase == 3)
            {
                const auto Result = Relations(Storage);
                if (Result != EStep::Complete) return Result == EStep::Pending ? Result : Status = Result;
                Frame.Phase = 4;
            }
            if (Frame.Phase == 4)
            {
                if (!Storage.Budget->Spend(32)) return Yield(Storage);
                if (!Depth) return Status = EStep::Complete;
                auto& Parent = FrameAt(Depth - 1);
                if (Schema.Kind == EWorldValueKind::Number && Parent.Index > 0 && Parent.Index <= 3
                    && (Parent.Domain == EDomain::ScenePoint || Parent.Domain == EDomain::Vertex
                        || Parent.Domain == EDomain::Edge || Parent.Domain == EDomain::WaterVelocity))
                    Parent.Tuple[Parent.Index - 1] = Frame.Scalar;
                --Depth;
            }
        }
    }

    EStep FDomainRead::Relations(const FStorage& Storage)
    {
        // All ancestor searches below are over the static grammar stack.
        if (!Storage.Budget->Spend(FrameCount * 8 + 32)) return Yield(Storage);
        auto& Frame = FrameAt(Depth);
        auto& R = Frame.Record;
        auto Ancestor = [&](EDomain Domain) -> FFrame*
        {
            for (uint32 I = Depth; I > 0; --I) if (FrameAt(I - 1).Domain == Domain) return &FrameAt(I - 1);
            return nullptr;
        };
        auto* Parent = Depth ? &FrameAt(Depth - 1) : nullptr;
        auto* Player = Ancestor(EDomain::Player);
        auto Match = [&](FValueRef A, FValueRef B, const TCHAR* Prefix = nullptr, uint64 PrefixCount = 0) -> EStep
        {
            if (!Frame.bComparing)
            {
                if (!Storage.Budget->Spend(2 * sizeof(FTextRead) + 64)) return Yield(Storage);
                if (Prefix) Frame.Comparison.ComparePrefixed(A, B, Prefix, PrefixCount);
                else Frame.Comparison.Compare(A, B);
                Frame.bComparing = true;
            }
            const auto Result = Frame.Comparison.Advance(Storage);
            if (Result != EStep::Complete) return Result;
            if (!Frame.Comparison.Equal()) return EStep::Invalid;
            Frame.bComparing = false; ++Frame.Relation;
            return EStep::Complete;
        };
        switch (Frame.Domain)
        {
        case EDomain::Edge:
        {
            const auto* Graph = Ancestor(EDomain::StreetGraph);
            return Graph && GraphEdge(Frame.Tuple[0], Frame.Tuple[1], Graph->Record.Count(0)) ? EStep::Complete : EStep::Invalid;
        }
        case EDomain::Vertex:
        {
            const auto* Obstacle = Ancestor(EDomain::NavigationObstacle);
            return Obstacle && NavigationVertex(Frame.Tuple[0], Frame.Tuple[1], Obstacle->Record.Number(0),
                Obstacle->Record.Number(1), Obstacle->Record.Number(2), Obstacle->Record.Number(3)) ? EStep::Complete : EStep::Invalid;
        }
        case EDomain::Endpoint:
        {
            if (Parent && Parent->Domain == EDomain::Roadside && R.Choice(0) != 1) return EStep::Invalid;
            if (Parent && Parent->Domain == EDomain::Journey && Frame.Relation == 0)
            {
                const auto Expected = Parent->Record.Field(Frame.ParentField == 11 ? 2 : 3);
                return R.Choice(0) == 1 ? Match(Expected, R.Field(1), TEXT("roadside:"), 9) : Match(Expected, R.Field(1));
            }
            return EStep::Complete;
        }
        case EDomain::Journey:
            return !Player || Player->Record.Choice(10) != 1 || R.Choice(5) == 0 ? EStep::Complete : EStep::Invalid;
        case EDomain::TransitRide:
            return !Player || (Player->Record.Choice(10) == 0 && R.Number(3) == Player->Record.Number(6)) ? EStep::Complete : EStep::Invalid;
        case EDomain::Nightclub:
        {
            if (!Player) return EStep::Complete;
            if (Player->Record.Choice(10) != 0 || R.Number(1) != Player->Record.Number(6)) return EStep::Invalid;
            if (!Frame.bComparing)
            {
                if (!Storage.Budget->Spend(2 * sizeof(FTextRead) + 64)) return Yield(Storage);
                Frame.Comparison.Compare(Player->Record.Field(3), TEXT("nightclub"), 9); Frame.bComparing = true;
            }
            const auto Result = Frame.Comparison.Advance(Storage);
            return Result == EStep::Complete && !Frame.Comparison.Equal() ? EStep::Invalid : Result;
        }
        case EDomain::DanceRound:
        {
            if (!Parent || Parent->Domain != EDomain::Nightclub) return EStep::Invalid;
            if (Frame.Relation == 0)
            {
                const auto Result = Match(R.Field(0), Parent->Record.Field(0));
                if (Result != EStep::Complete) return Result;
            }
            if (Player && Frame.Relation == 1)
            {
                const auto Result = Match(R.Field(1), Player->Record.Field(4));
                if (Result != EStep::Complete) return Result;
            }
            Parent->Lanes = R.Count(7);
            return EStep::Complete;
        }
        case EDomain::DanceInput:
        {
            const auto* Activity = Ancestor(EDomain::Nightclub);
            return Activity && DanceBeat(static_cast<int64>(R.Number(1)), Activity->Lanes) ? EStep::Complete : EStep::Invalid;
        }
        case EDomain::Pose:
            if (Parent && Parent->Domain == EDomain::Movement)
                Parent->MovementFrom = {R.Number(0), R.Number(1), R.Number(2)};
            return EStep::Complete;
        case EDomain::MovementPoint:
        {
            auto* Movement = Ancestor(EDomain::Movement);
            if (!Movement || Movement->MovementPointCount >= 32) return EStep::Invalid;
            Movement->MovementPoints[Movement->MovementPointCount++] = {R.Number(0), R.Number(1)};
            return EStep::Complete;
        }
        case EDomain::Movement:
        {
            if (Player && !MovementScope(static_cast<int64>(R.Number(1)), static_cast<int64>(R.Number(2)),
                static_cast<int64>(Player->Record.Number(7)), static_cast<int64>(Player->Record.Number(6)))) return EStep::Invalid;
            if (Frame.MovementPointCount != R.Count(8)) return EStep::Invalid;
            // MovementEndsAt is the legacy decoder's actual gameplay predicate.
            // Its temporary TArray and private prepared-segment allocations have
            // no qualified allocator bound yet. Keep that prerequisite explicit.
            constexpr uint64 QualifiedMovementTemporaryBytes = 0;
            constexpr uint32 QualifiedMovementTemporaryWork = 0;
            if (!QualifiedMovementTemporaryBytes || !QualifiedMovementTemporaryWork) return EStep::ResourceUnavailable;
            if (!Storage.Budget->Spend(QualifiedMovementTemporaryWork)) return Yield(Storage);
            if (!Storage.Budget->Reserve(QualifiedMovementTemporaryBytes)) return EStep::ResourceUnavailable;
            bool bValid = false;
            {
                FMovementRoute Route;
                Route.Sequence = static_cast<int64>(R.Number(1)); Route.RoomRevision = static_cast<int64>(R.Number(2));
                Route.StartedAtMs = R.Number(3); Route.From = Frame.MovementFrom; Route.YawRadians = R.Number(5);
                if (R.Presence(6) == EWorldFieldPresence::Value) Route.InitialSpeedMps = R.Number(6);
                if (R.Presence(7) == EWorldFieldPresence::Value) Route.Pace = static_cast<EMovementPace>(R.Choice(7) + 1);
                Route.Path.Reserve(Frame.MovementPointCount);
                for (uint32 I = 0; I < Frame.MovementPointCount; ++I) Route.Path.Add(Frame.MovementPoints[I]);
                double EndsAt = 0;
                bValid = MovementEndsAt(Route, EndsAt);
            }
            // The bounded temporary and sampler storage are destroyed before the
            // reservation is released. No typed route is published by this check.
            Storage.Budget->Release(QualifiedMovementTemporaryBytes);
            return bValid ? EStep::Complete : EStep::Invalid;
        }
        case EDomain::Player:
            return R.Choice(10) != 1 || R.Presence(12) == EWorldFieldPresence::Value ? EStep::Complete : EStep::Invalid;
        case EDomain::RaceCar:
        {
            if (!Parent || Parent->Domain != EDomain::RaceCars || Parent->Index < 1 || Parent->Index > 8) return EStep::Invalid;
            const uint32 PriorCount = static_cast<uint32>(Parent->Index - 1);
            while (Frame.Relation < PriorCount)
            {
                if (!Frame.bComparing)
                {
                    if (!Storage.Budget->Spend(2 * sizeof(FTextRead) + 64)) return Yield(Storage);
                    Frame.Comparison.Compare(R.Field(0), Parent->SeenUsers[Frame.Relation]); Frame.bComparing = true;
                }
                const auto Result = Frame.Comparison.Advance(Storage);
                if (Result != EStep::Complete) return Result;
                if (Frame.Comparison.Equal()) return EStep::Invalid;
                ++Frame.Relation; Frame.bComparing = false;
            }
            Parent->SeenUsers[PriorCount] = R.Field(0);
            return EStep::Complete;
        }
        case EDomain::Race:
            RacingInstance = R.Field(1); RacingUpdatedAt = static_cast<int64>(R.Number(6)); return EStep::Complete;
        case EDomain::SavedCharacter:
        case EDomain::SavedRoom:
        {
            auto* LibraryFrame = Ancestor(EDomain::Library);
            if (!Parent || !LibraryFrame || Parent->Index < 1 || Parent->Index > 12) return EStep::Invalid;
            const bool bCharacter = Frame.Domain == EDomain::SavedCharacter;
            const uint32 PriorCount = static_cast<uint32>(Parent->Index - 1);
            const uint32 Comparisons = PriorCount + (bCharacter ? 1 : 2);
            while (Frame.Relation < Comparisons)
            {
                const bool bDuplicate = Frame.Relation < PriorCount;
                const auto Against = bDuplicate ? Parent->SeenUsers[Frame.Relation]
                    : LibraryFrame->Record.Field(bCharacter ? 1 : Frame.Relation == PriorCount ? 2 : 3);
                if (!Frame.bComparing)
                {
                    if (!Storage.Budget->Spend(2 * sizeof(FTextRead) + 64)) return Yield(Storage);
                    Frame.Comparison.Compare(R.Field(0), Against); Frame.bComparing = true;
                }
                const auto Result = Frame.Comparison.Advance(Storage);
                if (Result != EStep::Complete) return Result;
                if (Frame.Comparison.Equal())
                {
                    if (bDuplicate) return EStep::Invalid;
                    if (bCharacter) { LibraryFrame->Selected |= 1; SelectedCharacter = Frame.Value; }
                    else if (Frame.Relation == PriorCount) { LibraryFrame->Selected |= 2; SelectedActiveRoom = Frame.Value; }
                    else { LibraryFrame->Selected |= 4; SelectedHome = Frame.Value; }
                }
                ++Frame.Relation; Frame.bComparing = false;
            }
            Parent->SeenUsers[PriorCount] = R.Field(0);
            return EStep::Complete;
        }
        case EDomain::Library:
            SelectedLibrary = Frame.Value; return Frame.Selected == 7 ? EStep::Complete : EStep::Invalid;
        default: return EStep::Complete;
        }
    }
}

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
        ensureMsgf(!Players && !RacingOwner && !Publication, TEXT("World semantic projections were not retired."));
        ensureMsgf(RetireManifestText == 3 + RootLimit * 3, TEXT("World manifest text was not retired."));
        Budget->Release(ChargedBytes);
    }
    FNode* FStorage::Get(uint8 Root, uint64 Id) const
    {
        return Root < RootLimit && Roots[Root] ? Roots[Root]->Get(Id) : nullptr;
    }
    EStep FStorage::ReleaseRoot(FRoot*& Root, FBudget& Budget)
    {
        if (!Root) return EStep::Complete;
        if (!Budget.Spend(8, false)) return EStep::Pending;
        if (Root->Owners > 1) { --Root->Owners; Root = nullptr; return EStep::Complete; }
        // Last-owner retirement can yield halfway through a string. Such a root
        // must never be pinned again, even while its storage is still allocated.
        Root->bVerified = false;
        while (Root->RetireNode <= Root->Nodes)
        {
            if (!Budget.Spend(8, false)) return EStep::Pending;
            auto* Node = Root->Get(Root->RetireNode);
            if (Node && Node->Text)
            {
                if (!QualifiedRetirementDeleteWork || QualifiedRetirementDeleteWork > TickWorkLimit)
                    return EStep::ResourceUnavailable;
                if (!Budget.Spend(QualifiedRetirementDeleteWork, false)) return EStep::Pending;
                auto* Block = Node->Text; Node->Text = Block->Next;
                delete Block; Budget.Release(QualifiedStringBlockBytes);
                continue;
            }
            if (Root->RetireNode % 64 == 0 || Root->RetireNode == Root->Nodes)
            {
                if (!QualifiedRetirementDeleteWork || QualifiedRetirementDeleteWork > TickWorkLimit)
                    return EStep::ResourceUnavailable;
                if (!Budget.Spend(QualifiedRetirementDeleteWork, false)) return EStep::Pending;
                const uint64 Index = (Root->RetireNode - 1) / 64;
                delete Root->Index[Index]; Root->Index[Index] = nullptr;
                Budget.Release(QualifiedNodePageBytes);
            }
            ++Root->RetireNode;
        }
        FString* Texts[] = {&Root->Directory.Alias, &Root->Directory.Digest, &Root->Directory.Tail,
            &Root->RecordHash, &Root->PageHash};
        while (Root->RetireText < UE_ARRAY_COUNT(Texts))
        {
            if (!QualifiedDirectoryTextReleaseWork || QualifiedDirectoryTextReleaseWork > TickWorkLimit)
                return EStep::ResourceUnavailable;
            if (!Budget.Spend(QualifiedDirectoryTextReleaseWork, false)) return EStep::Pending;
            Texts[Root->RetireText++]->Empty();
        }
        if (!QualifiedRetirementDeleteWork || !Budget.Spend(QualifiedRetirementDeleteWork, false)) return EStep::Pending;
        delete Root; Root = nullptr; Budget.Release(QualifiedRootBytes);
        return EStep::Complete;
    }
    EStep FStorage::Retire()
    {
        bRetiring = true; bComplete = bSemanticsComplete = false;
        if (ActiveBorrows || ActivePreparations) return EStep::Pending;
        if (Publication)
        {
            const auto Result = Publication->Retire(*Budget);
            if (Result != EStep::Complete) return Result;
            if (!QualifiedRetirementDeleteWork || !Budget->Spend(QualifiedRetirementDeleteWork, false)) return EStep::Pending;
            const uint64 Charge = Publication->Charge;
            delete Publication; Publication = nullptr; Budget->Release(Charge);
        }
        while (Players)
        {
            if (Players->Retire(*Budget) != EStep::Complete) return EStep::Pending;
            if (!QualifiedRetirementDeleteWork || !Budget->Spend(QualifiedRetirementDeleteWork, false)) return EStep::Pending;
            auto* Old = Players; Players = Old->Next; delete Old;
            Budget->Release(WorldReadPrivate::QualifiedSemanticPeerBytes);
        }
        if (ReleaseRoot(RacingOwner, *Budget) != EStep::Complete) return EStep::Pending;
        while (RetireRoot < RootLimit)
        {
            if (!Budget->Spend(1, false)) return EStep::Pending;
            if (ReleaseRoot(Roots[RetireRoot], *Budget) != EStep::Complete) return EStep::Pending;
            ++RetireRoot;
        }
        while (RetireManifestText < 3 + RootLimit * 3)
        {
            if (!QualifiedDirectoryTextReleaseWork || QualifiedDirectoryTextReleaseWork > TickWorkLimit)
                return EStep::ResourceUnavailable;
            if (!Budget->Spend(QualifiedDirectoryTextReleaseWork, false)) return EStep::Pending;
            FString* Text = nullptr;
            if (RetireManifestText < 3)
            {
                FString* Header[] = {&Manifest.Capture, &Manifest.Kind, &Manifest.Digest};
                Text = Header[RetireManifestText];
            }
            else
            {
                auto& Entry = Manifest.Entries[(RetireManifestText - 3) / 3];
                FString* Fields[] = {&Entry.Alias, &Entry.Digest, &Entry.Tail};
                Text = Fields[(RetireManifestText - 3) % 3];
            }
            Text->Empty(); ++RetireManifestText;
        }
        return EStep::Complete;
    }
    bool FRetirementTicket::Empty() const
    {
        for (const auto& Owner : Storage) if (Owner) return false;
        if (Static) return false;
        for (const auto& Task : Tasks) if (Task.bReserved || Task.Owner) return false;
        return true;
    }
    bool FRetirementTicket::HasQueued() const
    {
        for (const auto& Owner : Storage) if (Owner) return true;
        if (Static) return true;
        for (const auto& Task : Tasks)
            if (Task.Owner && (!Task.Ready || Task.Ready(Task.Owner))) return true;
        return false;
    }
    bool FRetirementTicket::Tick()
    {
        Budget->BeginTick();
        if (!Budget->Spend(QualifiedRetirementTickerWork, false)) return true;
        // Tasks can hold scoped preparation/semantic borrows. Drain them before
        // attempting the storage slots which those borrows keep alive.
        for (auto& Task : Tasks)
        {
            if (!Task.Owner) continue;
            // A metadata cell can wait on external weak handles. It must not
            // prevent independent payload owners from draining in this frame.
            if (Task.Advance && Task.Advance(Task.Owner, *Budget) != EStep::Complete) continue;
            if (!Budget->Spend(QualifiedRetirementDeleteWork, false)) return true;
            Task.Destroy(Task.Owner); Budget->Release(Task.Charge); Task = {};
        }
        if (Static)
        {
            if (Static->Retire() != EStep::Complete
                || !Budget->Spend(QualifiedRetirementDeleteWork, false)) return true;
            Static.Reset();
        }
        for (auto& Owner : Storage)
        {
            if (!Owner) continue;
            if (Owner->Retire() != EStep::Complete
                || !Budget->Spend(QualifiedRetirementDeleteWork, false)) return true;
            Owner.Reset();
        }
        return !Empty();
    }
    bool EnsureRetirementTicket(TSharedPtr<FBudget> Budget)
    {
        if (!Budget) return false;
        if (Budget->Retirement) return true;
        if (!QualifiedRetirementTicketBytes || !QualifiedRetirementTicketInitializationWork || !QualifiedRetirementTickerWork
            || !QualifiedRetirementDeleteWork || QualifiedRetirementTickerWork > TickWorkLimit
            || QualifiedRetirementDeleteWork > TickWorkLimit
            || QualifiedRetirementTicketInitializationWork > TickWorkLimit - QualifiedRetirementTickerWork) return false;
        if (!Budget->Spend(QualifiedRetirementTicketInitializationWork + QualifiedRetirementTickerWork, false)
            || !Budget->Reserve(QualifiedRetirementTicketBytes)) return false;
        auto* Ticket = new FRetirementTicket;
        Ticket->Budget = Budget; Ticket->Charge = QualifiedRetirementTicketBytes;
        Budget->Retirement = Ticket;
        FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Ticket](float)
        {
            if (Ticket->Tick()) return true;
            if (!Ticket->Budget->Spend(QualifiedRetirementDeleteWork, false)) return true;
            auto BudgetOwner = Ticket->Budget;
            const uint64 Charge = Ticket->Charge;
            BudgetOwner->Retirement = nullptr;
            delete Ticket; BudgetOwner->Release(Charge);
            if (BudgetOwner.GetSharedReferenceCount() == 1)
            { BudgetOwner->Release(BudgetOwner->OwnerCharge); BudgetOwner->OwnerCharge = 0; }
            return false;
        }));
        return true;
    }
    int32 ReserveRetirementTask(FBudget& Budget)
    {
        if (!Budget.Retirement || !Budget.Spend(64, false)) return INDEX_NONE;
        for (int32 I = 0; I < UE_ARRAY_COUNT(Budget.Retirement->Tasks); ++I)
        {
            auto& Task = Budget.Retirement->Tasks[I];
            if (!Task.bReserved && !Task.Owner) { Task.bReserved = true; return I; }
        }
        return INDEX_NONE;
    }
    void ReleaseRetirementTask(FBudget& Budget, int32 Slot)
    {
        check(Budget.Retirement && Slot >= 0 && Slot < UE_ARRAY_COUNT(Budget.Retirement->Tasks));
        auto& Task = Budget.Retirement->Tasks[Slot];
        check(Task.bReserved && !Task.Owner); Task = {};
    }
    void QueueRetirementTask(FBudget& Budget, int32 Slot, FRetirementTask Task)
    {
        check(Budget.Retirement && Slot >= 0 && Slot < UE_ARRAY_COUNT(Budget.Retirement->Tasks));
        auto& Target = Budget.Retirement->Tasks[Slot];
        check(Target.bReserved && !Target.Owner && Task.Owner && Task.Destroy);
        Task.bReserved = true; Target = Task;
    }
    void RetireDetached(TSharedPtr<FStorage> Owner)
    {
        if (!Owner) return;
        auto* Ticket = Owner->Budget->Retirement;
        check(Ticket);
        Owner->bRetiring = true; Owner->bComplete = false;
        for (const auto& Slot : Ticket->Storage) if (Slot == Owner) return;
        for (auto& Slot : Ticket->Storage) if (!Slot) { Slot = MoveTemp(Owner); return; }
        checkNoEntry();
    }
    FStaticRoots::~FStaticRoots()
    {
        ensureMsgf(!Roots[0] && !Roots[1], TEXT("World static roots were not retired."));
        Budget->Release(QualifiedStaticOwnerBytes);
    }
    bool FStaticRoots::Current(const FStaticContext& Expected) const
    {
        const double Now = FPlatformTime::Seconds();
        return bAvailable && Context.Lifetime && Context.CredentialGeneration && Context.EntrySequence > 0
            && Context.Visit && Context.Lifetime == Expected.Lifetime
            && Context.CredentialGeneration == Expected.CredentialGeneration
            && Context.EntrySequence == Expected.EntrySequence && Context.Visit == Expected.Visit
            && FCString::Strcmp(Generation, ProtocolGeneration) == 0
            && std::isfinite(Now) && std::isfinite(Deadline) && Now < Deadline;
    }
    EStep FStaticRoots::Retire()
    {
        bAvailable = false;
        for (auto& Root : Roots)
            if (FStorage::ReleaseRoot(Root, *Budget) != EStep::Complete) return EStep::Pending;
        return EStep::Complete;
    }
    void RetireDetached(TUniquePtr<FStaticRoots> Roots)
    {
        if (!Roots) return;
        Roots->bAvailable = false;
        auto* Ticket = Roots->Budget->Retirement;
        check(Ticket && !Ticket->Static);
        Ticket->Static = MoveTemp(Roots);
    }
    bool FAccess::Current() const
    {
        if (!Budget || !QualifiedAccessReadWork || QualifiedAccessReadWork > TickWorkLimit || !bAlive) return false;
        const double Now = FPlatformTime::Seconds();
        // Fixed currency metadata must not report revocation merely because a
        // payload read has to yield. Payload pinning has separate admission.
        return bReadable && Token.IsSet() && std::isfinite(Now) && std::isfinite(Deadline)
            && Now < Deadline;
    }
    FAccessWeak::FAccessWeak(FAccess* InCell)
    {
        check(IsInGameThread());
        if (InCell && InCell->WeakHandles != MAX_uint64)
        { Cell = InCell; ++Cell->WeakHandles; }
    }
    FAccessWeak::FAccessWeak(const FAccessWeak& Other) : FAccessWeak(Other.Cell) {}
    FAccessWeak::FAccessWeak(FAccessWeak&& Other) noexcept : Cell(Other.Cell) { Other.Cell = nullptr; }
    FAccessWeak& FAccessWeak::operator=(const FAccessWeak& Other)
    {
        if (this == &Other) return *this;
        FAccessWeak Next(Other); Reset(); Cell = Next.Cell; Next.Cell = nullptr; return *this;
    }
    FAccessWeak& FAccessWeak::operator=(FAccessWeak&& Other) noexcept
    {
        if (this == &Other) return *this;
        Reset(); Cell = Other.Cell; Other.Cell = nullptr; return *this;
    }
    FAccessWeak::~FAccessWeak() { Reset(); }
    void FAccessWeak::Reset()
    {
        check(IsInGameThread());
        if (Cell) { check(Cell->WeakHandles); --Cell->WeakHandles; Cell = nullptr; }
    }
    FAccess* FAccessWeak::Pin() const
    { check(IsInGameThread()); return Cell && Cell->bAlive ? Cell : nullptr; }
    void FAccess::Revoke()
    {
        bReadable = false;
        Storage.Reset(); Deadline = 0;
    }
}

namespace BaddieCompanion
{
    bool FWorldObservation::IsCurrent() const
    {
        const auto Current = Access.Pin();
        return Current && Current->Token == Captured && Current->Current();
    }
    EWorldReadResult FWorldObservation::AdmitRead() const
    {
        const auto Current = Access.Pin();
        if (!Current || Current->Token != Captured || !Current->Current()) return EWorldReadResult::Unavailable;
        if (!Current->Budget->Spend(WorldObservationPrivate::QualifiedAccessReadWork, false))
            return EWorldReadResult::Pending;
        const auto Owner = Current->Storage.Pin();
        return Owner && Owner->bComplete && !Owner->bRetiring && Owner->Publication && Owner->Publication->bComplete
            ? EWorldReadResult::Ready : EWorldReadResult::Unavailable;
    }
    uint64 FWorldObservation::Generation() const
    {
        return Captured.Lifetime;
    }
    bool FWorldObservation::ReadRoot(EWorldRoot Root, FWorldRootView& Out) const
    {
        Out = {};
        const auto Current = Access.Pin();
        if (AdmitRead() != EWorldReadResult::Ready) return false;
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
    EWorldReadResult FWorldObservation::ReadValue(EWorldRoot Root, FWorldValueRead& Out) const
    {
        Out = {};
        const auto Admission = AdmitRead();
        if (Admission != EWorldReadResult::Ready) return Admission;
        const auto Current = Access.Pin(); const auto Storage = Current->Storage.Pin();
        const uint8 Index = static_cast<uint8>(Root);
        if (Index >= WorldObservationPrivate::RootLimit) return EWorldReadResult::Unavailable;
        if (!Storage->Budget->Spend(sizeof(Out) + Storage->Manifest.Count + 32, false))
            return EWorldReadResult::Pending;
        Out.Observation = *this; Out.Root = Index; Out.Kind = EWorldReadDomain::Raw;
        Out.Owner = Storage->Roots[Index];
        bool bFound = false;
        for (int32 I = 0; I < Storage->Manifest.Count; ++I)
        {
            const auto& Entry = Storage->Manifest.Entries[I];
            if (Entry.Slot != Index) continue;
            Out.Presence = static_cast<EWorldFieldPresence>(Entry.Presence);
            Out.bJsonDocument = Entry.Encoding == 1; bFound = true; break;
        }
        if (!bFound) { Out = {}; return EWorldReadResult::Unavailable; }
        const auto* Node = Storage->Get(Index, 1);
        if (Node) Out.Node = Node->Id;
        else if (Out.Presence == EWorldFieldPresence::Value)
        { Out = {}; return EWorldReadResult::Unavailable; }
        return EWorldReadResult::Ready;
    }
    bool FWorldObservation::ReadNode(EWorldRoot Root, uint64 Id, FWorldNodeView& Out) const
    {
        Out = {};
        const auto Current = Access.Pin();
        if (AdmitRead() != EWorldReadResult::Ready) return false;
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
        if (AdmitRead() != EWorldReadResult::Ready)
        { if (!IsCurrent()) Cursor = {}; return false; }
        const auto Storage = Current->Storage.Pin();
        if (static_cast<uint8>(Root) >= WorldObservationPrivate::RootLimit
            || !Storage->Budget->Spend(32, false)) return false;
        const auto* Node = Storage->Get(static_cast<uint8>(Root), Id);
        if (!Node || Node->Kind != EWorldValueKind::String) return false;
        if (!Cursor.Generation)
        {
            Cursor.Generation = Captured.Lifetime; Cursor.Publication = Captured.Publication; Cursor.Node = Id;
            Cursor.Root = static_cast<uint8>(Root); Cursor.Owner = Storage->Roots[Cursor.Root]; Cursor.Block = Node->Text;
        }
        if (Cursor.Generation != Captured.Lifetime || Cursor.Publication != Captured.Publication || Cursor.Node != Id
            || Cursor.Root != static_cast<uint8>(Root) || Cursor.Owner != Storage->Roots[static_cast<uint8>(Root)]) return false;
        const auto* Block = static_cast<const WorldObservationPrivate::FStringBlock*>(Cursor.Block);
        if (!Block) return Cursor.Offset == Node->Count;
        if (!Storage->Budget->Spend(1 + Block->Used * 2, false)) return false;
        for (int32 I = 0; I < Block->Used; ++I) Out[I] = Block->Units[I];
        OutCount = Block->Used; Cursor.Offset += Block->Used; Cursor.Block = Block->Next;
        return true;
    }
    EWorldReadResult FWorldObservation::ReadPlayer(FWorldPlayerRead& Out) const
    {
        Out = {};
        const auto Current = Access.Pin();
        const auto Admission = AdmitRead();
        if (Admission != EWorldReadResult::Ready) return Admission;
        const auto Storage = Current->Storage.Pin();
        if (!Storage->Publication || !Storage->Publication->Local) return EWorldReadResult::Unavailable;
        if (!Storage->Budget->Spend(sizeof(Out), false)) return EWorldReadResult::Pending;
        const auto* Player = Storage->Publication->Local;
        Out.Record.Observation = *this; Out.Record.Owner = Player->Raw.Owner;
        Out.Record.Root = Player->Raw.Root; Out.Record.Node = Player->Raw.Node;
        Out.Record.Effective = Player; Out.Record.Kind = EWorldReadDomain::Player;
        Out.Record.Presence = EWorldFieldPresence::Value; return EWorldReadResult::Ready;
    }
    bool FWorldObservation::CopyTime(FWorldPresentationTime& Out) const
    {
        Out = {};
        const auto Current = Access.Pin();
        if (AdmitRead() != EWorldReadResult::Ready) return false;
        const auto Storage = Current->Storage.Pin();
        if (!Storage->Publication || !WorldObservationPrivate::QualifiedClockBytes
            || !WorldObservationPrivate::QualifiedClockWork) return false;
        const auto& Payload = *Storage->Publication;
        if (!Payload.Clock) return false;
        const uint64 Work = WorldObservationPrivate::QualifiedClockWork
            + 4 * static_cast<uint64>(Payload.Clock->GameRoomId.Len());
        if (Work > WorldObservationPrivate::TickWorkLimit || !Storage->Budget->Spend(Work, false)) return false;
        if (!Payload.Clock->Value.CopyTime(Payload.Clock->GameRoomId, Out) || Out.RemainingMs <= 0)
        { Out = {}; return false; }
        return true;
    }

    EWorldReadResult FWorldObservation::ReadPeer(FWorldPeerCursor& Cursor, FWorldPlayerRead& Out, bool& bEnd) const
    {
        Out = {}; bEnd = false;
        const auto Admission = AdmitRead();
        if (Admission != EWorldReadResult::Ready)
        { if (Admission == EWorldReadResult::Unavailable) Cursor = {}; return Admission; }
        const auto Current = Access.Pin(); const auto Storage = Current->Storage.Pin();
        if (!Storage->Publication) return EWorldReadResult::Unavailable;
        if (!Cursor.bStarted)
        {
            if (!Storage->Budget->Spend(sizeof(Cursor), false)) return EWorldReadResult::Pending;
            Cursor.Token = Captured; Cursor.Next = Storage->Players; Cursor.bStarted = true;
        }
        if (Cursor.Token != Captured) return EWorldReadResult::Unavailable;
        while (Cursor.Next)
        {
            if (!Storage->Budget->Spend(sizeof(Out) + 32, false)) return EWorldReadResult::Pending;
            const auto* Player = static_cast<const WorldReadPrivate::FPlayerResult*>(Cursor.Next);
            Cursor.Next = Player->Next;
            if (!Player->bPeer || !Player->bIncluded) continue;
            Out.Record.Observation = *this; Out.Record.Owner = Player->Raw.Owner;
            Out.Record.Root = Player->Raw.Root; Out.Record.Node = Player->Raw.Node;
            Out.Record.Effective = Player; Out.Record.Kind = EWorldReadDomain::Player;
            Out.Record.Presence = EWorldFieldPresence::Value; return EWorldReadResult::Ready;
        }
        bEnd = true; return EWorldReadResult::Ready;
    }

    EWorldReadResult FWorldObservation::ReadRacing(FWorldValueRead& Out) const
    {
        Out = {};
        const auto Admission = AdmitRead();
        if (Admission != EWorldReadResult::Ready) return Admission;
        const auto Current = Access.Pin(); const auto Storage = Current->Storage.Pin();
        if (!Storage->Budget->Spend(sizeof(Out) + 32, false)) return EWorldReadResult::Pending;
        Out.Observation = *this; Out.Kind = EWorldReadDomain::Racing; Out.Root = 3;
        Out.Owner = Storage->RacingOwner ? Storage->RacingOwner : Storage->Roots[3];
        Out.Node = Storage->RacingOwner ? Storage->RacingNode : 1;
        const auto* Node = WorldReadPrivate::Resolve(*Storage, {Out.Node, Out.Root,
            static_cast<WorldObservationPrivate::FRoot*>(const_cast<void*>(Out.Owner))});
        Out.Presence = !Node ? EWorldFieldPresence::Missing : Node->Kind == EWorldValueKind::Null
            ? EWorldFieldPresence::Null : EWorldFieldPresence::Value;
        Out.SourceNode = 1; Out.SourceRoot = 3; Out.SourceOwner = Storage->Roots[3]; Out.bHasSource = true;
        const auto& Directory = Storage->Manifest.Entries[3];
        Out.SourcePresence = Directory.Presence == 0 ? EWorldFieldPresence::Missing
            : Directory.Presence == 1 ? EWorldFieldPresence::Null : EWorldFieldPresence::Value;
        Out.bJsonDocument = Directory.Encoding != 0;
        if (!Storage->Roots[3]) Out.SourceNode = 0;
        if (!Storage->RacingOwner)
        { Out.Presence = Out.SourcePresence; if (!Storage->Roots[3]) Out.Node = 0; }
        return EWorldReadResult::Ready;
    }

    EWorldReadResult FWorldObservation::ReadInstance(FWorldInstanceRead& Out) const
    {
        Out = {};
        const auto Current = Access.Pin();
        const auto Admission = AdmitRead();
        if (Admission != EWorldReadResult::Ready) return Admission;
        const auto Storage = Current->Storage.Pin();
        if (!Storage->Publication) return EWorldReadResult::Unavailable;
        if (!Storage->Budget->Spend(sizeof(Out), false)) return EWorldReadResult::Pending;
        const auto Ref = Storage->Publication->InstanceRoot;
        Out.Record.Observation = *this; Out.Record.Owner = Ref.Owner; Out.Record.Root = Ref.Root;
        Out.Record.Node = Ref.Node; Out.Record.Kind = EWorldReadDomain::Instance;
        Out.Record.Presence = EWorldFieldPresence::Value; return EWorldReadResult::Ready;
    }
    EWorldReadResult FWorldObservation::ReadSelection(FWorldSelectionRead& Out) const
    {
        Out = {};
        const auto Current = Access.Pin();
        const auto Admission = AdmitRead();
        if (Admission != EWorldReadResult::Ready) return Admission;
        const auto Storage = Current->Storage.Pin();
        if (!Storage->Publication) return EWorldReadResult::Unavailable;
        if (!Storage->Budget->Spend(sizeof(Out), false)) return EWorldReadResult::Pending;
        const auto Ref = Storage->Publication->Selection;
        Out.Record.Observation = *this; Out.Record.Owner = Ref.Owner; Out.Record.Root = Ref.Root;
        Out.Record.Node = Ref.Node; Out.Record.Kind = EWorldReadDomain::Selection;
        Out.Record.Presence = EWorldFieldPresence::Value; return EWorldReadResult::Ready;
    }
    EWorldReadResult FWorldObservation::ReadHealth(FWorldHealthRead& Out) const
    {
        Out = {};
        const auto Admission = AdmitRead();
        if (Admission != EWorldReadResult::Ready) return Admission;
        const auto Current = Access.Pin(); const auto Storage = Current->Storage.Pin();
        if (!Storage->Publication) return EWorldReadResult::Unavailable;
        if (!Storage->Budget->Spend(sizeof(Out), false)) return EWorldReadResult::Pending;
        const auto Ref = Storage->Publication->Health;
        Out.Record.Observation = *this; Out.Record.Owner = Ref.Owner; Out.Record.Root = Ref.Root;
        Out.Record.Node = Ref.Node; Out.Record.Kind = EWorldReadDomain::Health;
        Out.Record.Presence = EWorldFieldPresence::Value; return EWorldReadResult::Ready;
    }
    EWorldReadResult FWorldObservation::ReadHomeProjection(FWorldHomeProjectionRead& Out) const
    {
        Out = {};
        const auto Admission = AdmitRead();
        if (Admission != EWorldReadResult::Ready) return Admission;
        const auto Current = Access.Pin(); const auto Storage = Current->Storage.Pin();
        if (!Storage->Publication) return EWorldReadResult::Unavailable;
        if (!Storage->Budget->Spend(sizeof(Out), false)) return EWorldReadResult::Pending;
        Out.Observation = *this; return EWorldReadResult::Ready;
    }
}
