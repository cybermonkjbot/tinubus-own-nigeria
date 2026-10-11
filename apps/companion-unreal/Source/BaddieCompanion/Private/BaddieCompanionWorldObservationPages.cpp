#include "BaddieCompanionWorldObservationPages.h"
#include "BaddieCompanionStateJson.h"
#include "IPlatformCrypto.h"
#include "PlatformCryptoTypes.h"
#include "Modules/ModuleManager.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/PlatformTime.h"

#include <cmath>

namespace BaddieCompanion::WorldObservationPrivate
{
    namespace
    {
        using FValue = TSharedPtr<FJsonValue>;
        using FObject = TSharedPtr<FJsonObject>;
        constexpr uint64 SafeInteger = 9007199254740991ULL;
        constexpr int32 MaxPageRecords = 64; // A staged page never crosses a raw-block boundary.
        // The installed hasher hides its Pimpl allocation. No approved bound for
        // that allocation/context/allocator slack is present in the source pins.
        // Zero rejects before provider allocation, not after exceeding the budget.
        constexpr uint64 QualifiedProviderAllocationBytes = 0;

        bool Same(const FString& A, const FString& B) { return A.Equals(B, ESearchCase::CaseSensitive); }
        bool Hex(const FString& S, int32 Length)
        {
            if (S.Len() != Length) return false;
            for (TCHAR C : S) if (!(C >= '0' && C <= '9') && !(C >= 'a' && C <= 'f')) return false;
            return true;
        }
        bool Counter(const FValue& V, uint64& Out)
        {
            double N = 0;
            if (!V.IsValid() || V->Type != EJson::Number || !V->TryGetNumber(N)
                || !std::isfinite(N) || N < 0 || N > SafeInteger || std::floor(N) != N) return false;
            Out = static_cast<uint64>(N); return true;
        }
        bool Counter(const FObject& O, const TCHAR* Key, uint64& Out)
        { return O.IsValid() && Counter(O->TryGetField(Key), Out); }
        bool Text(const FValue& V, FString& Out, int32 Limit = 64)
        { return V.IsValid() && V->Type == EJson::String && V->TryGetString(Out) && Out.Len() <= Limit; }
        bool Text(const FObject& O, const TCHAR* Key, FString& Out, int32 Limit = 64)
        { return O.IsValid() && Text(O->TryGetField(Key), Out, Limit); }
        bool IsNull(const FValue& V) { return V.IsValid() && V->Type == EJson::Null; }
        bool Add(uint64& N, uint64 Delta, uint64 Limit)
        { if (N > Limit || Delta > Limit - N) return false; N += Delta; return true; }
        bool Fields(const FObject& O, std::initializer_list<const TCHAR*> Names)
        { return O.IsValid() && O->Values.Num() == static_cast<int32>(Names.size()) && StateJsonPrivate::Only(O, Names); }
        FObject Reply(const FObject& Envelope)
        {
            FString Status;
            if (!Fields(Envelope, {TEXT("status"), TEXT("value")})
                || !Text(Envelope, TEXT("status"), Status) || !Same(Status, TEXT("success"))) return nullptr;
            return StateJsonPrivate::Object(Envelope->TryGetField(TEXT("value")));
        }
        bool Header(const FObject& O, const FString& Capture, const TCHAR* Status)
        {
            uint64 V = 0; FString G, C, S;
            return Counter(O, TEXT("v"), V) && V == 1 && Text(O, TEXT("generation"), G)
                && Same(G, ProtocolGeneration) && Text(O, TEXT("capture"), C) && Hex(C, 32)
                && Same(C, Capture) && Text(O, TEXT("status"), S) && Same(S, Status);
        }
        bool JsonSlot(uint8 S) { return S == 2 || (S >= 3 && S <= 9) || S == 12; }
        bool Kind(const FString& K)
        {
            return Same(K, TEXT("prepare")) || Same(K, TEXT("observe")) || Same(K, TEXT("enter"))
                || Same(K, TEXT("move")) || Same(K, TEXT("ready")) || Same(K, TEXT("presentationFailed"));
        }
        bool ExpectedSlots(const FManifest& M)
        {
            if (Same(M.Kind, TEXT("observe")))
            {
                if (M.Count != 18) return false;
                for (int32 I = 0; I < 18; ++I) if (M.Entries[I].Slot != I) return false;
                return M.PublishBy.IsSet();
            }
            if (Same(M.Kind, TEXT("prepare")))
                return M.Count == 3 && M.Entries[0].Slot == 2 && M.Entries[1].Slot == 16
                    && M.Entries[2].Slot == 17 && !M.PublishBy.IsSet();
            return M.Count == 2 && M.Entries[0].Slot == 17
                && M.Entries[1].Slot == (Same(M.Kind, TEXT("enter")) || Same(M.Kind, TEXT("move")) ? 18 : 19)
                && M.PublishBy.IsSet();
        }
        struct FBytes
        {
            uint8* Data; int32 Capacity; int32 Used = 0; bool bValid = true;
            void Byte(uint8 C) { if (Used == Capacity) bValid = false; else if (bValid) Data[Used++] = C; }
            void Literal(const ANSICHAR* S) { while (*S) Byte(static_cast<uint8>(*S++)); }
            void Unsigned(uint64 N)
            {
                uint64 Divisor = 1;
                while (Divisor <= N / 10) Divisor *= 10;
                do { Byte('0' + static_cast<uint8>(N / Divisor)); N %= Divisor; Divisor /= 10; } while (Divisor);
            }
            void String(const FString& S)
            {
                Byte('"');
                for (TCHAR C : S) { if (C < 32 || C > 126 || C == '"' || C == '\\') bValid = false; else Byte(C); }
                Byte('"');
            }
            void BE(uint64 N, int32 Size) { for (int32 I = Size - 1; I >= 0; --I) Byte((N >> (I * 8)) & 255); }
            void Digest(const FString& S)
            {
                if (!Hex(S, 64)) { bValid = false; return; }
                const auto Digit = [](TCHAR C) { return C <= '9' ? C - '0' : C - 'a' + 10; };
                for (int32 I = 0; I < 64; I += 2) Byte((Digit(S[I]) << 4) | Digit(S[I + 1]));
            }
        };
        FString DigestString(const uint8 (&Digest)[32])
        {
            FString Out; Out.Reserve(64);
            for (uint8 Byte : Digest)
            { Out.AppendChar(TEXT("0123456789abcdef")[Byte >> 4]); Out.AppendChar(TEXT("0123456789abcdef")[Byte & 15]); }
            return Out;
        }
        bool Directory(const FValue& V, FDirectoryEntry& E)
        {
            if (!V.IsValid() || V->Type != EJson::Array || V->AsArray().Num() != 13) return false;
            const auto& A = V->AsArray(); uint64 Slot = 0, Presence = 0, Encoding = 0;
            if (!Counter(A[0], Slot) || Slot >= RootLimit || !Counter(A[1], Presence) || Presence > 2
                || !Counter(A[2], Encoding) || Encoding != (JsonSlot(Slot) ? 1ULL : 0ULL)) return false;
            E.Slot = Slot; E.Presence = Presence; E.Encoding = Encoding;
            uint64* Counts[] = { &E.Pages, &E.Records, &E.Nodes, &E.Members, &E.ArraySlots, &E.Units, &E.PageBytes };
            for (int32 I = 0; I < 7; ++I) if (!Counter(A[I + 6], *Counts[I])) return false;
            if (Presence != 2)
            {
                if (!IsNull(A[3]) || !IsNull(A[4]) || !IsNull(A[5])) return false;
                for (auto* N : Counts) if (*N != 0) return false;
                return true;
            }
            return Text(A[3], E.Alias) && Hex(E.Alias, 32) && Text(A[4], E.Digest) && Hex(E.Digest, 64)
                && Text(A[5], E.Tail) && Hex(E.Tail, 64) && E.Pages && E.Records && E.Nodes
                && E.Nodes <= E.Records && E.Pages <= E.Records && E.PageBytes >= E.Pages;
        }
        bool Record(const FValue& V, FBytes& B)
        {
            if (!V.IsValid() || V->Type != EJson::Array) return false;
            const auto& A = V->AsArray(); uint64 Tag = 0, Node = 0;
            if (A.Num() < 4 || !Counter(A[0], Tag) || Tag > 6 || !Counter(A[1], Node) || !Node) return false;
            B.Byte('['); B.Unsigned(Tag); B.Byte(','); B.Unsigned(Node);
            if (Tag == 3)
            {
                uint64 Offset = 0;
                if (A.Num() != 4 || !Counter(A[2], Offset) || !A[3].IsValid() || A[3]->Type != EJson::Array) return false;
                const auto& Units = A[3]->AsArray();
                if (Units.Num() < 1 || Units.Num() > 2048) return false;
                B.Byte(','); B.Unsigned(Offset); B.Literal(",[");
                for (int32 I = 0; I < Units.Num(); ++I)
                {
                    uint64 Unit = 0; if (!Counter(Units[I], Unit) || Unit > 65535) return false;
                    if (I) B.Byte(','); B.Unsigned(Unit);
                }
                B.Literal("]]"); return B.bValid;
            }
            if (A.Num() != (Tag == 4 ? 5 : 6)) return false;
            for (int32 I = 2; I <= 4; ++I)
            { uint64 N = 0; if (!Counter(A[I], N) || (I == 3 && N > 3)) return false; B.Byte(','); B.Unsigned(N); }
            if (Tag != 4)
            {
                B.Byte(',');
                if (Tag == 6)
                {
                    FString Bits; if (!Text(A[5], Bits) || !Hex(Bits, 16)) return false;
                    uint64 Raw = 0;
                    for (TCHAR C : Bits) Raw = (Raw << 4) | (C <= '9' ? C - '0' : C - 'a' + 10);
                    double Number = 0; FMemory::Memcpy(&Number, &Raw, sizeof(Number));
                    if (!std::isfinite(Number)) return false;
                    B.String(Bits);
                }
                else { uint64 N = 0; if (!Counter(A[5], N) || (Tag == 5 && N > 1)) return false; B.Unsigned(N); }
            }
            B.Byte(']'); return B.bValid;
        }
    }

    struct FPages::FData
    {
        explicit FData(TSharedRef<FBudget> InBudget) : Budget(MoveTemp(InBudget)) {}
        enum class EPhase : uint8 { Empty, ManifestHash, RootSeed, PageSeed, AwaitPage, Canonical, PageHash, RecordHash, Assemble, RootHash, Complete, Failed };
        TSharedRef<FBudget> Budget;
        TSharedPtr<FStorage> Storage;
        FObject Page;
        TUniquePtr<FEncryptionContext> Crypto;
        TOptional<FSHA256Hasher> Hasher;
        EPhase Phase = EPhase::Empty;
        int32 RootIndex = 0, RecordIndex = 0, CanonicalIndex = 0, CanonicalSize = 0;
        int32 Starts[MaxPageRecords]{}, Ends[MaxPageRecords]{};
        uint8 Canonical[RecordBytesLimit]{};
        uint8 Prefix[4096]{};
        int32 PrefixSize = 0, HashOffset = 0, HashSize = 0;
        const uint8* HashBytes = nullptr;
        FString PageDigest;
        uint64 PageNormalizedBytes = 0, Completing = 0, DuplicateKey = 0, Against = 0;
        FStringBlock* Left = nullptr;
        FStringBlock* Right = nullptr;
        int32 LeftOffset = 0, RightOffset = 0, UnitOffset = 0;
        bool bHashStarted = false, bPrefixHashed = false, bRecordStarted = false, bLastPage = false;
        EStep Failure = EStep::Invalid;

        void ReleaseCrypto()
        {
            Hasher.Reset();
            if (Crypto) { Crypto.Reset(); Budget->Release(QualifiedProviderAllocationBytes); }
            bHashStarted = false; bPrefixHashed = false; HashBytes = nullptr;
        }
        EStep Fail(EStep Why)
        { Failure = Why; Phase = EPhase::Failed; ReleaseCrypto(); Page.Reset(); Budget->ReleasePage(); return Why; }
        FRoot* Root() const
        { return Storage.IsValid() && RootIndex < Storage->Manifest.Count ? Storage->Roots[Storage->Manifest.Entries[RootIndex].Slot] : nullptr; }
        void StartHash(const uint8* Bytes, int32 Size, int32 SeedSize)
        { HashBytes = Bytes; HashSize = Size; PrefixSize = SeedSize; HashOffset = 0; bHashStarted = false; bPrefixHashed = false; }
        EStep Hash(FString& Out)
        {
            if (!QualifiedProviderAllocationBytes) return EStep::ResourceUnavailable;
            if (!Crypto)
            {
                auto* Provider = FModuleManager::GetModulePtr<IPlatformCrypto>(TEXT("PlatformCrypto"));
                if (!Provider || !Budget->Reserve(QualifiedProviderAllocationBytes)) return EStep::ResourceUnavailable;
                Crypto = Provider->CreateContext();
                if (!Crypto) { Budget->Release(QualifiedProviderAllocationBytes); return EStep::ResourceUnavailable; }
                Hasher.Emplace(Crypto->CreateSHA256Hasher());
            }
            if (!bHashStarted)
            {
                if (!Budget->Spend(128)) return EStep::Pending;
                if (Hasher->Init() != EPlatformCryptoResult::Success) return EStep::ResourceUnavailable;
                bHashStarted = true;
            }
            if (!bPrefixHashed)
            {
                if (!Budget->Spend(PrefixSize)) return EStep::Pending;
                if (PrefixSize && Hasher->Update(MakeArrayView(static_cast<const uint8*>(Prefix), PrefixSize)) != EPlatformCryptoResult::Success)
                    return EStep::ResourceUnavailable;
                bPrefixHashed = true;
            }
            while (HashOffset < HashSize)
            {
                const int32 Count = FMath::Min(1024, HashSize - HashOffset);
                if (!Budget->Spend(Count)) return EStep::Pending;
                if (Hasher->Update(MakeArrayView(HashBytes + HashOffset, Count)) != EPlatformCryptoResult::Success)
                    return EStep::ResourceUnavailable;
                HashOffset += Count;
            }
            if (!Budget->Spend(256)) return EStep::Pending;
            uint8 Digest[32]{};
            if (Hasher->Finalize(MakeArrayView(Digest)) != EPlatformCryptoResult::Success) return EStep::ResourceUnavailable;
            Out = DigestString(Digest); return EStep::Complete;
        }
        bool BeginRoot()
        {
            while (RootIndex < Storage->Manifest.Count && Storage->Manifest.Entries[RootIndex].Presence != 2) ++RootIndex;
            if (RootIndex == Storage->Manifest.Count) { Storage->bComplete = true; Phase = EPhase::Complete; return true; }
            const auto& E = Storage->Manifest.Entries[RootIndex];
            if (!Budget->Reserve(sizeof(FRoot) * 2 + 64)) return false;
            auto* R = new FRoot; R->Directory = E; Storage->Roots[E.Slot] = R;
            FBytes B{Prefix, sizeof(Prefix)}; B.Literal("baddie-world-records-v1"); B.BE(E.Slot, 4); B.BE(E.Encoding, 4);
            StartHash(nullptr, 0, B.Used); Phase = EPhase::RootSeed; return B.bValid;
        }
        EStep CloseNode()
        {
            auto* R = Root();
            if (DuplicateKey)
            {
                auto* Key = R->Get(DuplicateKey);
                while (Against != DuplicateKey)
                {
                    if (!Budget->Spend(1)) return EStep::Pending;
                    auto* Other = R->Get(Against);
                    if (!Other || Other->Role != 1) return EStep::Invalid;
                    if (!Left && !Right && Key->Count == Other->Count)
                    { Left = Key->Text; Right = Other->Text; LeftOffset = RightOffset = 0; }
                    bool bDifferent = Key->Count != Other->Count;
                    while (!bDifferent && Left && Right)
                    {
                        if (!Budget->Spend(3)) return EStep::Pending;
                        if (Left->Units[LeftOffset] != Right->Units[RightOffset]) { bDifferent = true; break; }
                        if (++LeftOffset == Left->Used) { Left = Left->Next; LeftOffset = 0; }
                        if (++RightOffset == Right->Used) { Right = Right->Next; RightOffset = 0; }
                    }
                    if (!bDifferent) return EStep::Invalid;
                    auto* Value = R->Get(Other->NextSibling);
                    if (!Value || !Value->NextSibling) return EStep::Invalid;
                    Against = Value->NextSibling; Left = Right = nullptr;
                }
                DuplicateKey = 0;
            }
            while (Completing)
            {
                if (!Budget->Spend(1)) return EStep::Pending;
                auto* N = R->Get(Completing); if (!N) return EStep::Invalid;
                if (!N->Parent) { R->bClosed = true; R->Parent = Completing = 0; return EStep::Complete; }
                auto* Parent = R->Get(N->Parent); if (!Parent) return EStep::Invalid;
                if (N->Role == 1)
                {
                    if (Parent->NextRole == 1)
                    {
                        Parent->NextRole = 2; DuplicateKey = Completing; Against = Parent->FirstChild;
                        return CloseNode();
                    }
                    R->Parent = Parent->Id; Completing = 0; return EStep::Complete;
                }
                ++Parent->Completed;
                if (Parent->Kind == EWorldValueKind::Object) Parent->NextRole = 1;
                if (Parent->Completed > Parent->Count) return EStep::Invalid;
                if (Parent->Completed == Parent->Count) { Completing = Parent->Id; R->Parent = Parent->Parent; }
                else { R->Parent = Parent->Id; Completing = 0; }
            }
            return EStep::Complete;
        }
        EStep Assemble()
        {
            auto* R = Root(); const auto& A = Page->GetArrayField(TEXT("records"))[RecordIndex]->AsArray();
            uint64 Tag = 0, Id = 0; Counter(A[0], Tag); Counter(A[1], Id);
            if (!bRecordStarted)
            {
                if (!Budget->Spend(512)) return EStep::Pending;
                if (Tag == 3)
                {
                    uint64 Offset = 0; Counter(A[2], Offset);
                    if (!R->OpenString || Id != R->OpenString || Offset != R->StringOffset) return EStep::Invalid;
                    auto* N = R->Get(Id);
                    if (!N || static_cast<uint64>(A[3]->AsArray().Num()) > N->Count - R->StringOffset) return EStep::Invalid;
                }
                else
                {
                    uint64 ParentId = 0, Role = 0, Slot = 0;
                    Counter(A[2], ParentId); Counter(A[3], Role); Counter(A[4], Slot);
                    if (R->bClosed || R->OpenString || Id != R->Nodes + 1 || Id > R->Directory.Nodes) return EStep::Invalid;
                    auto* Parent = R->Get(ParentId);
                    if (R->Nodes == 0)
                    { if (ParentId || Role || Slot) return EStep::Invalid; }
                    else if (!Parent || ParentId != R->Parent || Role != Parent->NextRole || Slot != Parent->Completed
                        || (Role == 1 && Tag != 2)) return EStep::Invalid;
                    const uint64 Index = (Id - 1) / 64;
                    if (Index >= FRoot::IndexCount) return EStep::ResourceUnavailable;
                    if (Index == R->InitializedIndex)
                    {
                        if (!Budget->Spend(sizeof(FNodePage))) return EStep::Pending;
                        if (!Budget->Reserve(sizeof(FNodePage) * 2 + 64)) return EStep::ResourceUnavailable;
                        R->Index[Index] = new FNodePage; ++R->InitializedIndex;
                    }
                    ++R->Nodes; auto* N = R->Get(Id);
                    N->Id = Id; N->Parent = ParentId; N->Role = Role; N->Slot = Slot; N->Kind = static_cast<EWorldValueKind>(Tag);
                    if (Tag < 3 || Tag == 5) Counter(A[5], N->Count);
                    if (Tag == 6)
                    {
                        FString Bits; Text(A[5], Bits);
                        for (TCHAR C : Bits) N->NumberBits = (N->NumberBits << 4) | (C <= '9' ? C - '0' : C - 'a' + 10);
                    }
                    if (Parent)
                    {
                        if (Parent->LastChild) R->Get(Parent->LastChild)->NextSibling = Id;
                        else Parent->FirstChild = Id;
                        Parent->LastChild = Id;
                    }
                    if (Tag == 0 || Tag == 1)
                    {
                        if (!Add(Tag == 0 ? R->Members : R->ArraySlots, N->Count,
                            Tag == 0 ? R->Directory.Members : R->Directory.ArraySlots)) return EStep::Invalid;
                        N->NextRole = Tag == 0 ? 1 : 3;
                        if (N->Count) R->Parent = Id; else Completing = Id;
                    }
                    else if (Tag == 2)
                    {
                        if (!Add(R->Units, N->Count, R->Directory.Units)) return EStep::Invalid;
                        if (N->Count) { R->OpenString = Id; R->StringOffset = 0; } else Completing = Id;
                    }
                    else Completing = Id;
                }
                bRecordStarted = true;
            }
            if (Tag == 3)
            {
                auto* N = R->Get(Id); const auto& Units = A[3]->AsArray();
                while (UnitOffset < Units.Num())
                {
                    if (!Budget->Spend(12)) return EStep::Pending;
                    if (!N->Tail || N->Tail->Used == 256)
                    {
                        if (!Budget->Spend(sizeof(FStringBlock))) return EStep::Pending;
                        if (!Budget->Reserve(sizeof(FStringBlock) * 2 + 64)) return EStep::ResourceUnavailable;
                        auto* Block = new FStringBlock;
                        if (N->Tail) N->Tail->Next = Block; else N->Text = Block;
                        N->Tail = Block;
                    }
                    uint64 Unit = 0; Counter(Units[UnitOffset++], Unit);
                    N->Tail->Units[N->Tail->Used++] = static_cast<uint16>(Unit); ++R->StringOffset;
                }
                if (R->StringOffset == N->Count) { R->OpenString = 0; Completing = Id; }
            }
            const auto Closed = CloseNode();
            if (Closed != EStep::Complete) return Closed;
            ++R->Records; ++RecordIndex; UnitOffset = 0; bRecordStarted = false;
            return EStep::Complete;
        }
    };

    FPages::FPages(TSharedRef<FBudget> Budget)
    {
        if (ResourcesQualified()) Data = MakePimpl<FData>(MoveTemp(Budget));
    }
    FPages::~FPages()
    {
        RetireDetached(Cancel());
    }
    bool FPages::DecodeBody(const FString& Body, FObject& Out, FBudget& Budget)
    {
        Out.Reset();
        if (!ResourcesQualified()) { Budget.bExhausted = true; return false; }
        if (!Budget.bPageReserved || Body.IsEmpty() || Body.Len() > BodyLimit
            || !StateJsonPrivate::WithinParseLimits(Body, {BodyLimit, 12, BodyLimit})) return false;
        for (TCHAR C : Body) if (C <= 0 || C > 127) return false;
        const uint64 Charge = static_cast<uint64>(Body.Len()) * 8;
        if (Charge > CandidateWorkLimit - Budget.CandidateWork) { Budget.bExhausted = true; return false; }
        Budget.CandidateWork += Charge;
        const auto Reader = TJsonReaderFactory<>::Create(Body);
        TArray<FString, TInlineAllocator<32>> Keys[2];
        int32 Objects = 0; uint64 Estimate = Body.Len() * 8 + 65536;
        EJsonNotation Notation;
        while (Reader->ReadNext(Notation))
        {
            if (!Add(Estimate, 96, PageAllowance)) { Budget.bExhausted = true; return false; }
            const FString& Identifier = Reader->GetIdentifier();
            if (Objects && !Identifier.IsEmpty() && Notation != EJsonNotation::ObjectEnd && Notation != EJsonNotation::ArrayEnd)
            {
                auto& Scope = Keys[Objects - 1];
                if (Identifier.Len() > 32 || Scope.Num() >= 32 || Scope.Contains(Identifier)) return false;
                Scope.Add(Identifier);
            }
            if (Notation == EJsonNotation::ObjectStart)
            { if (Objects == 2) return false; Keys[Objects++].Reset(); }
            else if (Notation == EJsonNotation::ObjectEnd) { if (!Objects) return false; --Objects; }
            else if (Notation == EJsonNotation::String && Reader->GetValueAsString().Len() > 64) return false;
            else if (Notation == EJsonNotation::Error) return false;
        }
        return Objects == 0 && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Body), Out) && Out.IsValid();
    }
    EStep FPages::Begin(const FObject& Envelope, const FString& Capture, const FString& CaptureKind)
    {
        if (!Data) return EStep::ResourceUnavailable;
        // Fixed directory precharge precedes all scans, comparisons and writing.
        if (!Data->Budget->Spend(TickWorkLimit))
            return Data->Budget->bExhausted ? Data->Fail(EStep::ResourceUnavailable) : EStep::Pending;
        if (Data->Phase != FData::EPhase::Empty || !Hex(Capture, 32) || !Kind(CaptureKind)) return Data->Fail(EStep::Invalid);
        const auto O = Reply(Envelope); FManifest M; uint64 At = 0, Deadline = 0;
        if (!Fields(O, {TEXT("v"), TEXT("generation"), TEXT("capture"), TEXT("status"), TEXT("kind"),
                TEXT("capturedAt"), TEXT("permissionDeadline"), TEXT("publishBy"), TEXT("directoryDigest"), TEXT("roots")})
            || !Header(O, Capture, TEXT("manifest")) || !Text(O, TEXT("kind"), M.Kind) || !Same(M.Kind, CaptureKind)
            || !Counter(O, TEXT("capturedAt"), At) || !At || !Counter(O, TEXT("permissionDeadline"), Deadline) || Deadline <= At
            || !Text(O, TEXT("directoryDigest"), M.Digest) || !Hex(M.Digest, 64)) return Data->Fail(EStep::Invalid);
        M.Capture = Capture; M.CapturedAt = At; M.PermissionDeadline = Deadline;
        const auto Publish = O->TryGetField(TEXT("publishBy"));
        if (!IsNull(Publish))
        { uint64 P = 0; if (!Counter(Publish, P) || P <= At || P > Deadline || P - At > 60000) return Data->Fail(EStep::Invalid); M.PublishBy = P; }
        const auto Roots = O->TryGetField(TEXT("roots"));
        if (!Roots.IsValid() || Roots->Type != EJson::Array || Roots->AsArray().Num() > 18) return Data->Fail(EStep::Invalid);
        for (const auto& V : Roots->AsArray())
        {
            auto& E = M.Entries[M.Count];
            if (!Directory(V, E) || (M.Count && M.Entries[M.Count - 1].Slot >= E.Slot)) return Data->Fail(EStep::Invalid);
            for (int32 I = 0; I < M.Count; ++I)
                if (E.Presence == 2 && Same(M.Entries[I].Alias, E.Alias)) return Data->Fail(EStep::Invalid);
            ++M.Count;
        }
        if (!ExpectedSlots(M)) return Data->Fail(EStep::Invalid);
        for (int32 I = 0; I < M.Count; ++I)
            if ((M.Entries[I].Slot == 17 || M.Entries[I].Slot == 16 || M.Entries[I].Slot >= 18)
                && M.Entries[I].Presence != 2) return Data->Fail(EStep::Invalid);
        if (!Data->Budget->Reserve(sizeof(FStorage) * 2 + 64)) return Data->Fail(EStep::ResourceUnavailable);
        Data->Storage = MakeShared<FStorage>(Data->Budget);
        Data->Storage->ChargedBytes = sizeof(FStorage) * 2 + 64;
        Data->Storage->Manifest = MoveTemp(M);
        FBytes B{Data->Canonical, sizeof(Data->Canonical)}; B.Byte('[');
        for (int32 I = 0; I < Data->Storage->Manifest.Count; ++I)
        {
            const auto& E = Data->Storage->Manifest.Entries[I];
            if (I) B.Byte(','); B.Byte('['); B.Unsigned(E.Slot); B.Byte(','); B.Unsigned(E.Presence); B.Byte(','); B.Unsigned(E.Encoding); B.Byte(',');
            if (E.Presence == 2) { B.String(E.Alias); B.Byte(','); B.String(E.Digest); B.Byte(','); B.String(E.Tail); }
            else B.Literal("null,null,null");
            for (uint64 N : { E.Pages, E.Records, E.Nodes, E.Members, E.ArraySlots, E.Units, E.PageBytes }) { B.Byte(','); B.Unsigned(N); }
            B.Byte(']');
        }
        B.Byte(']');
        if (!B.bValid) return Data->Fail(EStep::ResourceUnavailable);
        FBytes Seed{Data->Prefix, sizeof(Data->Prefix)}; Seed.Literal("baddie-world-directory-v1");
        Data->StartHash(Data->Canonical, B.Used, Seed.Used); Data->Phase = FData::EPhase::ManifestHash;
        Data->Budget->ReleasePage(); return EStep::Pending;
    }
    bool FPages::Next(FString& Alias, uint64& Ordinal) const
    {
        Alias.Reset(); Ordinal = 0;
        if (!Data) return false;
        if (Data->Phase != FData::EPhase::AwaitPage) return false;
        const auto* R = Data->Root(); if (!R) return false;
        Alias = R->Directory.Alias; Ordinal = R->Pages; return true;
    }
    EStep FPages::Consume(const FObject& Envelope, uint32)
    {
        if (!Data) return EStep::ResourceUnavailable;
        if (Data->Phase != FData::EPhase::AwaitPage || !Data->Storage.IsValid()) return Data->Fail(EStep::Invalid);
        auto* R = Data->Root(); const auto O = Reply(Envelope); FString RootAlias, Previous;
        uint64 Slot = 0, Ordinal = 0, Start = 0, Count = 0;
        if (!Fields(O, {TEXT("v"), TEXT("generation"), TEXT("capture"), TEXT("status"), TEXT("root"), TEXT("slot"),
            TEXT("ordinal"), TEXT("recordStart"), TEXT("recordCount"), TEXT("previous"), TEXT("digest"), TEXT("records"), TEXT("next")})
            || !Header(O, Data->Storage->Manifest.Capture, TEXT("page"))
            || !Text(O, TEXT("root"), RootAlias) || !Same(RootAlias, R->Directory.Alias)
            || !Counter(O, TEXT("slot"), Slot) || Slot != R->Directory.Slot
            || !Counter(O, TEXT("ordinal"), Ordinal) || Ordinal != R->Pages
            || !Counter(O, TEXT("recordStart"), Start) || Start != R->Records
            || !Counter(O, TEXT("recordCount"), Count) || !Count || Count > MaxPageRecords || Count > R->Directory.Records - R->Records
            || !Text(O, TEXT("previous"), Previous) || !Same(Previous, R->PageHash)
            || !Text(O, TEXT("digest"), Data->PageDigest) || !Hex(Data->PageDigest, 64)) return Data->Fail(EStep::Invalid);
        const auto Records = O->TryGetField(TEXT("records"));
        if (!Records.IsValid() || Records->Type != EJson::Array || Records->AsArray().Num() != Count) return Data->Fail(EStep::Invalid);
        Data->bLastPage = Ordinal + 1 == R->Directory.Pages;
        const auto NextValue = O->TryGetField(TEXT("next"));
        if (Data->bLastPage) { if (!IsNull(NextValue)) return Data->Fail(EStep::Invalid); }
        else
        {
            FString NextRoot; uint64 NextOrdinal = 0;
            if (!NextValue.IsValid() || NextValue->Type != EJson::Array || NextValue->AsArray().Num() != 2
                || !Text(NextValue->AsArray()[0], NextRoot) || !Same(NextRoot, RootAlias)
                || !Counter(NextValue->AsArray()[1], NextOrdinal) || NextOrdinal != Ordinal + 1) return Data->Fail(EStep::Invalid);
        }
        Data->Page = O; Data->CanonicalIndex = Data->RecordIndex = 0;
        Data->CanonicalSize = 1; Data->Canonical[0] = '['; Data->Phase = FData::EPhase::Canonical;
        return EStep::Pending;
    }
    EStep FPages::Advance()
    {
        if (!Data) return EStep::ResourceUnavailable;
        using P = FData::EPhase;
        if (Data->Budget->bExhausted) return Data->Fail(EStep::ResourceUnavailable);
        if (Data->Phase == P::Failed) return Data->Failure;
        auto* R = Data->Root(); FString Digest;
        if (Data->Phase == P::ManifestHash || Data->Phase == P::RootSeed || Data->Phase == P::PageSeed
            || Data->Phase == P::PageHash || Data->Phase == P::RecordHash || Data->Phase == P::RootHash)
        {
            const auto Result = Data->Hash(Digest);
            if (Result == EStep::Pending) return Result;
            if (Result != EStep::Complete) return Data->Fail(Result);
            if (Data->Phase == P::ManifestHash)
            {
                if (!Same(Digest, Data->Storage->Manifest.Digest)) return Data->Fail(EStep::Invalid);
                if (!Data->BeginRoot()) return Data->Fail(EStep::ResourceUnavailable);
            }
            else if (Data->Phase == P::RootSeed)
            {
                R->RecordHash = Digest; FBytes B{Data->Prefix, sizeof(Data->Prefix)};
                B.Literal("baddie-world-pages-v1"); for (TCHAR C : R->Directory.Alias) B.Byte(C);
                Data->StartHash(nullptr, 0, B.Used); Data->Phase = P::PageSeed;
            }
            else if (Data->Phase == P::PageSeed) { R->PageHash = Digest; Data->Phase = P::AwaitPage; }
            else if (Data->Phase == P::PageHash)
            {
                if (!Same(Digest, Data->PageDigest)) return Data->Fail(EStep::Invalid);
                R->PageHash = Digest;
                FBytes B{Data->Prefix, sizeof(Data->Prefix)}; B.Digest(R->RecordHash); B.BE(Data->Ends[0] - Data->Starts[0], 4);
                Data->StartHash(Data->Canonical + Data->Starts[0], Data->Ends[0] - Data->Starts[0], B.Used); Data->Phase = P::RecordHash;
            }
            else if (Data->Phase == P::RecordHash) { R->RecordHash = Digest; Data->Phase = P::Assemble; }
            else
            {
                if (!Same(Digest, R->Directory.Digest) || !Same(R->PageHash, R->Directory.Tail)) return Data->Fail(EStep::Invalid);
                ++Data->RootIndex; if (!Data->BeginRoot()) return Data->Fail(EStep::ResourceUnavailable);
            }
            return EStep::Pending;
        }
        if (Data->Phase == P::Canonical)
        {
            const auto& Records = Data->Page->GetArrayField(TEXT("records"));
            if (Data->CanonicalIndex < Records.Num())
            {
                const auto& V = Records[Data->CanonicalIndex];
                uint64 Cost = 512;
                if (V.IsValid() && V->Type == EJson::Array && V->AsArray().Num() == 4
                    && V->AsArray()[3].IsValid() && V->AsArray()[3]->Type == EJson::Array)
                {
                    if (V->AsArray()[3]->AsArray().Num() > 2048) return Data->Fail(EStep::Invalid);
                    Cost += V->AsArray()[3]->AsArray().Num() * 15;
                }
                if (!Data->Budget->Spend(Cost)) return EStep::Pending;
                FBytes B{Data->Canonical, sizeof(Data->Canonical), Data->CanonicalSize};
                if (Data->CanonicalIndex) B.Byte(',');
                Data->Starts[Data->CanonicalIndex] = B.Used;
                if (!Record(V, B)) return Data->Fail(EStep::Invalid);
                Data->Ends[Data->CanonicalIndex++] = B.Used; Data->CanonicalSize = B.Used;
                return EStep::Pending;
            }
            if (!Data->Budget->Spend(4096)) return EStep::Pending;
            FBytes B{Data->Canonical, sizeof(Data->Canonical), Data->CanonicalSize}; B.Byte(']');
            if (!B.bValid) return Data->Fail(EStep::Invalid);
            Data->CanonicalSize = B.Used;
            // Header strings are fixed ASCII aliases/digests; decimal counters use our writer.
            uint8 Framing[4096]; FBytes Frame{Framing, sizeof(Framing)};
            Frame.Literal("{\"status\":\"success\",\"value\":{\"v\":1,\"generation\":"); Frame.String(ProtocolGeneration);
            Frame.Literal(",\"capture\":"); Frame.String(Data->Storage->Manifest.Capture);
            Frame.Literal(",\"status\":\"page\",\"root\":"); Frame.String(R->Directory.Alias);
            Frame.Literal(",\"slot\":"); Frame.Unsigned(R->Directory.Slot); Frame.Literal(",\"ordinal\":"); Frame.Unsigned(R->Pages);
            Frame.Literal(",\"recordStart\":"); Frame.Unsigned(R->Records); Frame.Literal(",\"recordCount\":"); Frame.Unsigned(Records.Num());
            Frame.Literal(",\"previous\":"); Frame.String(R->PageHash); Frame.Literal(",\"digest\":"); Frame.String(Data->PageDigest);
            Frame.Literal(",\"records\":,\"next\":");
            if (Data->bLastPage) Frame.Literal("null");
            else { Frame.Byte('['); Frame.String(R->Directory.Alias); Frame.Byte(','); Frame.Unsigned(R->Pages + 1); Frame.Byte(']'); }
            Frame.Literal("}}");
            if (!Frame.bValid || Frame.Used + B.Used > BodyLimit) return Data->Fail(EStep::Invalid);
            Data->PageNormalizedBytes = Frame.Used + B.Used;
            FBytes Seed{Data->Prefix, sizeof(Data->Prefix)}; Seed.Digest(R->PageHash); Seed.BE(R->Pages, 8); Seed.BE(R->Records, 8); Seed.BE(B.Used, 4);
            Data->StartHash(Data->Canonical, B.Used, Seed.Used); Data->Phase = P::PageHash; return EStep::Pending;
        }
        if (Data->Phase == P::Assemble)
        {
            const auto Result = Data->Assemble();
            if (Result == EStep::Pending) return Result;
            if (Result != EStep::Complete) return Data->Fail(Result);
            const int32 Count = Data->Page->GetArrayField(TEXT("records")).Num();
            if (Data->RecordIndex < Count)
            {
                const int32 I = Data->RecordIndex;
                FBytes B{Data->Prefix, sizeof(Data->Prefix)}; B.Digest(R->RecordHash); B.BE(Data->Ends[I] - Data->Starts[I], 4);
                Data->StartHash(Data->Canonical + Data->Starts[I], Data->Ends[I] - Data->Starts[I], B.Used); Data->Phase = P::RecordHash;
                return EStep::Pending;
            }
            ++R->Pages;
            if (!Add(R->PageBytes, Data->PageNormalizedBytes, R->Directory.PageBytes)) return Data->Fail(EStep::Invalid);
            Data->Page.Reset(); Data->Budget->ReleasePage();
            if (!Data->bLastPage) { Data->Phase = P::AwaitPage; return EStep::Pending; }
            if (!R->bClosed || R->OpenString || R->Records != R->Directory.Records || R->Nodes != R->Directory.Nodes
                || R->Members != R->Directory.Members || R->ArraySlots != R->Directory.ArraySlots
                || R->Units != R->Directory.Units || R->PageBytes != R->Directory.PageBytes) return Data->Fail(EStep::Invalid);
            FBytes B{Data->Prefix, sizeof(Data->Prefix)}; B.Digest(R->RecordHash);
            for (uint64 N : {R->Records, R->Nodes, R->Members, R->ArraySlots, R->Units}) B.BE(N, 8);
            Data->StartHash(nullptr, 0, B.Used); Data->Phase = P::RootHash; return EStep::Pending;
        }
        return Data->Phase == P::Complete ? EStep::Complete : EStep::Pending;
    }
    TSharedPtr<FStorage> FPages::TakeComplete()
    {
        if (!Data) return nullptr;
        if (Data->Phase != FData::EPhase::Complete) return nullptr;
        auto Out = MoveTemp(Data->Storage); Data->Phase = FData::EPhase::Empty; return Out;
    }
    TSharedPtr<FStorage> FPages::Cancel()
    {
        if (!Data) return nullptr;
        auto Out = MoveTemp(Data->Storage); Data->Page.Reset(); Data->Budget->ReleasePage();
        Data->ReleaseCrypto();
        Data->RootIndex = Data->RecordIndex = Data->CanonicalIndex = Data->CanonicalSize = 0;
        Data->Completing = Data->DuplicateKey = Data->Against = 0;
        Data->Left = Data->Right = nullptr;
        Data->LeftOffset = Data->RightOffset = Data->UnitOffset = 0;
        Data->bRecordStarted = Data->bLastPage = false;
        Data->Phase = FData::EPhase::Empty; return Out;
    }
    const FManifest* FPages::Manifest() const { return Data && Data->Storage.IsValid() ? &Data->Storage->Manifest : nullptr; }
}
