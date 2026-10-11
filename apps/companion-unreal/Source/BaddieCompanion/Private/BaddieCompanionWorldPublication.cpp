#include "BaddieCompanionWorldPublication.h"

#include <cmath>

namespace BaddieCompanion::WorldObservationPrivate
{
    using namespace WorldReadPrivate;

    const FRecordSchema* ReadSchema(EWorldReadDomain Domain)
    {
        if (Domain == EWorldReadDomain::Appearance) return &HomeMapping::AppearanceFields();
        if (Domain == EWorldReadDomain::SavedRoom) return &HomeMapping::RoomFields();
        if (Domain == EWorldReadDomain::HomeDesign) return &HomeMapping::DesignFields();
        if (Domain == EWorldReadDomain::HomeItem) return &HomeMapping::ItemFields();
        if (Domain == EWorldReadDomain::Health) return &HealthSchema();
        if (Domain == EWorldReadDomain::RecoveryDestination) return &RecoveryDestinationSchema();
        if (Domain >= EWorldReadDomain::Health) return nullptr;
        return DomainSchema(static_cast<EDomain>(Domain)).Record;
    }
    EWorldReadDomain ChildDomain(EWorldReadDomain Parent, uint32 Field)
    {
        if (Parent == EWorldReadDomain::SavedRoom && Field == 4) return EWorldReadDomain::HomeDesign;
        if (Parent == EWorldReadDomain::SavedRoom && Field == 8) return EWorldReadDomain::HomeItems;
        if (Parent == EWorldReadDomain::Health && Field == 7) return EWorldReadDomain::RecoveryDestination;
        if (Parent == EWorldReadDomain::RecoveryDestination && Field == 2) return EWorldReadDomain::ScenePoint;
        if (Parent >= EWorldReadDomain::Health) return EWorldReadDomain::Raw;
        const auto& Schema = DomainSchema(static_cast<EDomain>(Parent));
        for (uint32 I = 0; I < Schema.ChildCount; ++I)
            if (Schema.Children[I].Field == Field) return static_cast<EWorldReadDomain>(Schema.Children[I].Domain);
        return EWorldReadDomain::Raw;
    }
    EWorldReadDomain ElementDomain(EWorldReadDomain Parent)
    {
        if (Parent == EWorldReadDomain::HomeItems) return EWorldReadDomain::HomeItem;
        if (Parent >= EWorldReadDomain::Health) return EWorldReadDomain::Raw;
        const auto& Schema = DomainSchema(static_cast<EDomain>(Parent));
        return Schema.Kind == EWorldValueKind::Array
            ? static_cast<EWorldReadDomain>(Schema.Element) : EWorldReadDomain::Raw;
    }
    void FPreparedText::Begin(FValueRef Value)
    {
        Ref = Value; Block = nullptr; Offset = Units = Charge = 0; bAllocated = false;
    }
    void FKnownHomeInput::Begin(FValueRef Object, const FRecordSchema& Schema)
    {
        Source = Object; Rules = &Schema; Phase = Index = RetireIndex = 0;
        bTextStarted = false;
    }
    EStep FKnownHomeInput::Advance(FStorage& Storage, HomeMapping::FKnownObject& Out)
    {
        if (!Rules || Rules->Count > UE_ARRAY_COUNT(Out.Fields)) return EStep::Invalid;
        if (Phase == 0)
        {
            if (!Storage.Budget->Spend(FRecordRead::BeginWork() + Rules->Count * sizeof(FFieldSlot)))
                return Storage.Budget->bExhausted ? EStep::ResourceUnavailable : EStep::Pending;
            for (uint32 I = 0; I < Rules->Count; ++I)
            {
                Slots[I].Name = Rules->Fields[I].Name; Slots[I].NameLength = Rules->Fields[I].NameLength;
                Slots[I].Value = {}; Slots[I].bRequired = false;
            }
            Fields.Begin(Source, Slots, Rules->Count, false); Phase = 1;
        }
        if (Phase == 1)
        {
            const auto Result = Fields.Advance(Storage);
            if (Result != EStep::Complete) return Result;
            Phase = 2;
        }
        while (Index < Rules->Count)
        {
            auto& Field = Out.Fields[Index];
            if (Phase == 2)
            {
                if (!Storage.Budget->Spend(128))
                    return Storage.Budget->bExhausted ? EStep::ResourceUnavailable : EStep::Pending;
                Field.Name = Rules->Fields[Index].Name; Field.Source = Slots[Index].Value;
                const auto* Value = Resolve(Storage, Field.Source);
                if (Field.Source.IsSet() && !Value) return EStep::Invalid;
                Field.Presence = !Value ? EWorldFieldPresence::Missing : Value->Kind == EWorldValueKind::Null
                    ? EWorldFieldPresence::Null : EWorldFieldPresence::Value;
                Field.Kind = Value ? Value->Kind : Rules->Fields[Index].Kind;
                if (Value && Value->Kind == EWorldValueKind::Number)
                    FMemory::Memcpy(&Field.Number, &Value->NumberBits, sizeof(double));
                Field.Boolean = Value && Value->Count != 0;
                // Mapping failure is not structural failure. Preserve the slot's
                // supplied kind/presence and let the shared mapper emit its issue
                // in order. Never copy an oversized or wrong-kind known value.
                if (Value && Value->Kind == EWorldValueKind::String
                    && (Rules->Fields[Index].Kind != EWorldValueKind::String
                        || Value->Count > Rules->Fields[Index].Text.Maximum || Value->Count == 0))
                { Phase = 4; }
                else Phase = 3;
            }
            if (Phase == 3 && Field.Presence == EWorldFieldPresence::Value && Field.Kind == EWorldValueKind::String)
            {
                if (!bTextStarted) { Text.Begin(Field.Source); bTextStarted = true; }
                const auto Result = Text.Advance(Storage, Field.Text, QualifiedHomeBytes);
                if (Result != EStep::Complete) return Result;
                Charge += Text.TakeCharge(); bTextStarted = false;
            }
            Phase = 4;
            if (!Storage.Budget->Spend(16))
                return Storage.Budget->bExhausted ? EStep::ResourceUnavailable : EStep::Pending;
            ++Index; Out.Count = Index; Phase = 2;
        }
        return EStep::Complete;
    }
    EStep FKnownHomeInput::Retire(FStorage& Storage, HomeMapping::FKnownObject& Out)
    {
        // The pending string's allocation is charged even before its field completes.
        Charge += Text.TakeCharge();
        while (RetireIndex < UE_ARRAY_COUNT(Out.Fields))
        {
            auto& Field = Out.Fields[RetireIndex];
            if (!Field.Text.GetCharArray().IsEmpty())
            {
                if (!QualifiedTextReleaseWork || QualifiedTextReleaseWork > TickWorkLimit)
                    return EStep::ResourceUnavailable;
                if (!Storage.Budget->Spend(QualifiedTextReleaseWork, false)) return EStep::Pending;
                Field.Text.Empty();
            }
            if (!Storage.Budget->Spend(32, false)) return EStep::Pending;
            Field.Source = {}; ++RetireIndex;
        }
        Fields.Cancel(); Rules = nullptr; Source = {}; Out.Count = 0;
        Storage.Budget->Release(Charge); Charge = 0;
        return EStep::Complete;
    }
    namespace
    {
        class FWorldMappingWork final : public HomeMapping::FMappingWork
        {
        public:
            FWorldMappingWork(FStorage& InStorage, bool bInRetiring = false)
                : Storage(InStorage), bRetiring(bInRetiring) {}
            EStep Admit(uint64 Work, uint64 Capacity = 0) override
            {
                if (!QualifiedHomeOperationWork || QualifiedHomeOperationWork > TickWorkLimit
                    || Work > TickWorkLimit - QualifiedHomeOperationWork
                    || (Capacity && !QualifiedHomeAllocationBytes)
                    || (bRetiring && !QualifiedHomeReleaseWork)) return EStep::ResourceUnavailable;
                if (!Storage.Budget->Spend(Work + QualifiedHomeOperationWork, !bRetiring))
                    return Storage.Budget->bExhausted && !bRetiring ? EStep::ResourceUnavailable : EStep::Pending;
                if (Capacity)
                {
                    if (QualifiedHomeAllocationBytes > RetainedLimit
                        || Capacity > RetainedLimit - QualifiedHomeAllocationBytes) return EStep::ResourceUnavailable;
                    const uint64 Bytes = Capacity + QualifiedHomeAllocationBytes;
                    if (!Storage.Budget->Reserve(Bytes)) return EStep::ResourceUnavailable;
                    Storage.Publication->HomeCharge += Bytes;
                }
                return EStep::Complete;
            }
            EStep ReleaseText(FString& Text) override
            {
                if (Text.GetCharArray().IsEmpty()) return EStep::Complete;
                if (!QualifiedTextReleaseWork || QualifiedTextReleaseWork > TickWorkLimit)
                    return EStep::ResourceUnavailable;
                if (!Storage.Budget->Spend(QualifiedTextReleaseWork, !bRetiring))
                    return Storage.Budget->bExhausted && !bRetiring ? EStep::ResourceUnavailable : EStep::Pending;
                Text.Empty(); return EStep::Complete;
            }
        private:
            FStorage& Storage;
            bool bRetiring;
        };
    }
    void FHomePreparation::Begin(FValueRef InCharacter, FValueRef InRoom)
    {
        Character = InCharacter; Room = InRoom;
        Phase = Index = ItemCount = RetirePhase = RetireIndex = 0;
        bStarted = bRoomFinished = bInputComplete = false; bMapped = true;
    }
    EStep FHomePreparation::Advance(FStorage& Storage)
    {
        if (!Storage.Publication || !QualifiedHomeBytes || !QualifiedHomeOperationWork
            || !QualifiedKnownObjectInitializationWork || QualifiedKnownObjectInitializationWork > TickWorkLimit)
            return EStep::ResourceUnavailable;
        auto& Payload = *Storage.Publication;
        FWorldMappingWork Work(Storage);
        while (InitializedKnown < 12)
        {
            if (!Storage.Budget->Spend(QualifiedKnownObjectInitializationWork))
                return Storage.Budget->bExhausted ? EStep::ResourceUnavailable : EStep::Pending;
            new (Known.Slots[InitializedKnown].Bytes) HomeMapping::FKnownObject;
            ++InitializedKnown;
        }
        while (Phase < 12)
        {
            EStep Result = EStep::Complete;
            if (Phase == 0 || Phase == 1 || Phase == 2 || Phase == 3)
            {
                const FRecordSchema* Schemas[] = { SelectionDomainSchema(EDomain::SavedCharacter).Record,
                    &HomeMapping::AppearanceFields(), &HomeMapping::RoomFields(), &HomeMapping::DesignFields() };
                const FValueRef Refs[] = { Character, Appearance, Room, Design };
                if (Phase == 3 && !bDesignObject) { ++Phase; bStarted = false; continue; }
                if (!bStarted) { Input.Begin(Refs[Phase], *Schemas[Phase]); bStarted = true; }
                if (!bInputComplete)
                {
                    Result = Input.Advance(Storage, Known[Phase]);
                    if (Result != EStep::Complete) return Result;
                    ScratchCharge += Input.TakeCharge(); bInputComplete = true;
                }
                if (!Storage.Budget->Spend(64)) return Storage.Budget->bExhausted ? EStep::ResourceUnavailable : EStep::Pending;
                if (Phase == 0) Appearance = Known[0].Fields[2].Source;
                if (Phase == 2)
                {
                    Design = Known[2].Fields[4].Source;
                    bDesignPresent = Known[2].Fields[4].Presence != EWorldFieldPresence::Missing;
                    bDesignObject = Known[2].Fields[4].Presence == EWorldFieldPresence::Value
                        && Known[2].Fields[4].Kind == EWorldValueKind::Object;
                    Items = Known[2].Fields[8].Source;
                    const auto* Array = Resolve(Storage, Items);
                    bItemsArray = Array && Array->Kind == EWorldValueKind::Array;
                    if (bItemsArray && Array->Count > MAX_uint32) return EStep::ResourceUnavailable;
                    ItemCount = bItemsArray ? static_cast<uint32>(Array->Count) : 0;
                    ItemNext = bItemsArray && ItemCount <= 8 ? Array->FirstChild : 0;
                }
            }
            else if (Phase == 4)
            {
                if (ItemCount <= 8 && Index < ItemCount)
                {
                    if (!bStarted)
                    {
                        if (!Storage.Budget->Spend(32)) return Storage.Budget->bExhausted ? EStep::ResourceUnavailable : EStep::Pending;
                        const auto* Item = Resolve(Storage, Items.Child(ItemNext));
                        if (!Item) return EStep::Invalid;
                        if (Item->Kind != EWorldValueKind::Object)
                        { ItemNext = Item->NextSibling; ++Index; continue; }
                        ItemRefs[Index] = Items.Child(ItemNext);
                        Input.Begin(ItemRefs[Index], HomeMapping::ItemFields()); bStarted = true;
                    }
                    if (!bInputComplete)
                    {
                        Result = Input.Advance(Storage, Known[4 + Index]);
                        if (Result != EStep::Complete) return Result;
                        ScratchCharge += Input.TakeCharge(); bInputComplete = true;
                    }
                    if (!Storage.Budget->Spend(32)) return Storage.Budget->bExhausted ? EStep::ResourceUnavailable : EStep::Pending;
                    const auto* Item = Resolve(Storage, Items.Child(ItemNext));
                    if (!Item) return EStep::Invalid;
                    ItemNext = Item->NextSibling; ++Index; bStarted = bInputComplete = false; continue;
                }
                if (ItemNext) return EStep::Invalid;
            }
            else if (Phase == 5)
            {
                if (Index < 8)
                {
                    Result = Work.Admit(512, 81 * sizeof(TCHAR));
                    if (Result != EStep::Complete) return Result;
                    Payload.bHomePreparationStarted = true;
                    switch (Index++)
                    {
                    case 0: CharacterIdentity.Id = Known[0].Fields[0].Text; break;
                    case 1: CharacterIdentity.Name = Known[0].Fields[1].Text; break;
                    case 2: CharacterIdentity.Look = Known[1].Fields[0].Text; break;
                    case 3: CharacterIdentity.BaseId = Known[1].Fields[1].Presence == EWorldFieldPresence::Missing
                        ? TEXT("bestie") : Known[1].Fields[1].Text; break;
                    case 4: RoomIdentity.Id = Known[2].Fields[0].Text; break;
                    case 5: RoomIdentity.Name = Known[2].Fields[1].Text; break;
                    case 6: RoomIdentity.EnvironmentId = Known[2].Fields[2].Text; break;
                    case 7: RoomIdentity.LayoutId = Known[2].Fields[3].Text; break;
                    }
                    continue;
                }
                if (Index < 8 + FMath::Min<uint32>(ItemCount, 8))
                {
                    Result = Work.Admit(128);
                    if (Result != EStep::Complete) return Result;
                    const uint32 I = Index++ - 8;
                    if (ItemRefs[I].IsSet())
                    {
                        RoomInput.Items[I] = HomeMapping::FMappingObject(Known[4 + I]);
                        RoomInput.Items[I].Unknown = &Payload.HomeUnknown;
                        RoomInput.Items[I].Source = ItemRefs[I];
                    }
                    continue;
                }
                Result = Work.Admit(256);
                if (Result != EStep::Complete) return Result;
                RoomInput.Room = HomeMapping::FMappingObject(Known[2]);
                RoomInput.Room.Unknown = &Payload.HomeUnknown; RoomInput.Room.Source = Room;
                RoomInput.bDesignPresent = bDesignPresent; RoomInput.bDesignObject = bDesignObject;
                RoomInput.Design = HomeMapping::FMappingObject(Known[3]);
                RoomInput.Design.Unknown = &Payload.HomeUnknown; RoomInput.Design.Source = Design;
                RoomInput.bItemsArray = bItemsArray; RoomInput.ItemCount = ItemCount;
                HomeMapping::FMappingObject Object(Known[1]);
                Object.Unknown = &Payload.HomeUnknown; Object.Source = Appearance;
                AppearanceMapping.Begin(CharacterIdentity, Object, Payload.Home.Appearance,
                    Payload.Home.CharacterMaterials, Payload.Home.Attachments, Payload.Home.Issues);
                bAppearanceStarted = true;
            }
            else if (Phase == 6)
            {
                Result = AppearanceMapping.Advance(Work);
                if (Result == EStep::Invalid) { bMapped = false; Phase = 11; continue; }
            }
            else if (Phase == 7 || Phase == 8)
            {
                if (!bStarted)
                {
                    Result = Work.Admit(128, 32 * sizeof(TCHAR));
                    if (Result != EStep::Complete) return Result;
                    Path = Phase == 7 ? TEXT("home") : TEXT("activeRoom");
                    RoomMapping.Begin(RoomIdentity, RoomInput, Phase == 7 ? Payload.Home.Home : Payload.Home.ActiveRoom,
                        Path, Payload.Home.Issues); bStarted = true; bRoomFinished = false;
                    bRoomStarted = true;
                }
                if (!bRoomFinished)
                {
                    Result = RoomMapping.Advance(Work);
                    if (Result == EStep::Invalid) { bMapped = false; Phase = 11; continue; }
                    if (Result != EStep::Complete) return Result;
                    bRoomFinished = true;
                }
                Result = RoomMapping.Retire(Work);
                if (Result == EStep::Complete) bRoomStarted = false;
            }
            else if (Phase == 9)
            {
                // The exact legacy-shaped duplicate is indivisible only after
                // separate allocator/copy qualification. Counts are not proof.
                if (!QualifiedHomeRoomCopyWork || !QualifiedHomeRoomCopyBytes
                    || QualifiedHomeRoomCopyWork > TickWorkLimit) return EStep::ResourceUnavailable;
                if (!Storage.Budget->Spend(QualifiedHomeRoomCopyWork))
                    return Storage.Budget->bExhausted ? EStep::ResourceUnavailable : EStep::Pending;
                if (!Storage.Budget->Reserve(QualifiedHomeRoomCopyBytes)) return EStep::ResourceUnavailable;
                Payload.HomeCharge += QualifiedHomeRoomCopyBytes;
                Payload.Home.Rooms.Add(Payload.Home.Home);
                ApartmentMapping.Begin(Payload.Home.Rooms);
                bApartmentStarted = true;
            }
            else if (Phase == 10)
            {
                Result = ApartmentMapping.Advance(Work, Payload.Home.Apartment);
                if (Result == EStep::Invalid) { bMapped = false; Phase = 11; continue; }
            }
            else if (Phase == 11)
            {
                Result = Work.Admit(1024, sizeof(FCanonicalAssetReference) + 512 * sizeof(TCHAR)
                    + (Payload.Home.Issues.Num() + 1) * sizeof(FHomeMappingIssue));
                if (Result != EStep::Complete) return Result;
                Payload.bHomePreparationStarted = true;
                if (bMapped)
                {
                    Payload.Home.CharacterAsset = HomeMapping::CharacterReference();
                    HomeMapping::Issue(Payload.Home.Issues, EHomeIssue::AssetImportUnavailable, TEXT("character.asset"));
                }
                else HomeMapping::Issue(Payload.Home.Issues, EHomeIssue::InvalidDocument, TEXT("selection"));
                Payload.Home.Apartment.bCompleteHomeInventoryVerified = false;
                Payload.bHomeMapped = bMapped; Payload.bHomeMappingFinished = true;
            }
            if (Result != EStep::Complete) return Result;
            ++Phase; Index = 0; bStarted = bInputComplete = false;
        }
        return EStep::Complete;
    }
    EStep FHomePreparation::Retire(FStorage& Storage)
    {
        FWorldMappingWork Work(Storage, true);
        ScratchCharge += Input.TakeRetirementCharge();
        while (RetirePhase < 5)
        {
            EStep Result = EStep::Complete;
            if (RetirePhase == 0 && bAppearanceStarted) Result = AppearanceMapping.Retire(Work);
            if (RetirePhase == 1 && bRoomStarted) Result = RoomMapping.Retire(Work);
            if (RetirePhase == 2 && bApartmentStarted) Result = ApartmentMapping.Retire(Work);
            if (RetirePhase == 3)
            {
                FString* Texts[] = { &CharacterIdentity.Id, &CharacterIdentity.Name, &CharacterIdentity.BaseId,
                    &CharacterIdentity.Look, &RoomIdentity.Id, &RoomIdentity.Name, &RoomIdentity.EnvironmentId,
                    &RoomIdentity.LayoutId, &Path };
                while (RetireIndex < UE_ARRAY_COUNT(Texts))
                {
                    Result = Work.ReleaseText(*Texts[RetireIndex]);
                    if (Result != EStep::Complete) return Result;
                    ++RetireIndex;
                }
            }
            if (RetirePhase == 4)
            {
                while (RetireIndex < InitializedKnown)
                {
                    auto& Object = Known[RetireIndex];
                    while (RetireField < UE_ARRAY_COUNT(Object.Fields))
                    {
                        if (!Storage.Budget->Spend(32, false)) return EStep::Pending;
                        Result = Work.ReleaseText(Object.Fields[RetireField].Text);
                        if (Result != EStep::Complete) return Result;
                        Object.Fields[RetireField].Source = {}; ++RetireField;
                    }
                    if (!Storage.Budget->Spend(sizeof(HomeMapping::FKnownObject), false)) return EStep::Pending;
                    Object.~FKnownObject(); ++RetireIndex; RetireField = 0;
                }
            }
            if (Result != EStep::Complete) return Result;
            ++RetirePhase; RetireIndex = 0;
        }
        if (!Storage.Budget->Spend(FRecordRead::BeginWork(), false)) return EStep::Pending;
        Input.Cancel();
        Storage.Budget->Release(ScratchCharge); ScratchCharge = 0;
        return EStep::Complete;
    }
    EStep FPreparedText::Advance(FStorage& Storage, FString& Out, uint64 QualifiedOverhead)
    {
        if (!Storage.Budget->Spend(16))
            return Storage.Budget->bExhausted ? EStep::ResourceUnavailable : EStep::Pending;
        const auto* Node = Resolve(Storage, Ref);
        if (!Node || Node->Kind != EWorldValueKind::String) return EStep::Invalid;
        if (!bAllocated)
        {
            if (!QualifiedOverhead || !QualifiedTextAllocationWork) return EStep::ResourceUnavailable;
            Units = Node->Count;
            if (QualifiedOverhead > RetainedLimit - sizeof(TCHAR) || Units > MAX_int32 - 1
                || Units > (RetainedLimit - QualifiedOverhead) / sizeof(TCHAR) - 1)
                return EStep::ResourceUnavailable;
            const uint64 Bytes = (Units + 1) * sizeof(TCHAR) + QualifiedOverhead;
            if (QualifiedTextAllocationWork > TickWorkLimit) return EStep::ResourceUnavailable;
            if (!Storage.Budget->Spend(QualifiedTextAllocationWork))
                return Storage.Budget->bExhausted ? EStep::ResourceUnavailable : EStep::Pending;
            if (!Storage.Budget->Reserve(Bytes)) return EStep::ResourceUnavailable;
            Charge = Bytes;
            Out.GetCharArray().SetNumUninitialized(static_cast<int32>(Units + 1));
            Out.GetCharArray()[static_cast<int32>(Units)] = 0;
            Block = Node->Text; bAllocated = true;
        }
        while (Block)
        {
            if (!Storage.Budget->Spend(2 * Block->Used + 16))
                return Storage.Budget->bExhausted ? EStep::ResourceUnavailable : EStep::Pending;
            if (Offset + Block->Used > Units) return EStep::Invalid;
            for (uint16 I = 0; I < Block->Used; ++I)
                Out.GetCharArray()[static_cast<int32>(Offset++)] = static_cast<TCHAR>(Block->Units[I]);
            Block = Block->Next;
        }
        return Offset == Units ? EStep::Complete : EStep::Invalid;
    }
    EStep FPreparedText::Retire(FBudget& Budget, FString& Out)
    {
        if (!bAllocated) return EStep::Complete;
        if (!QualifiedTextReleaseWork || QualifiedTextReleaseWork > TickWorkLimit) return EStep::ResourceUnavailable;
        if (!Budget.Spend(QualifiedTextReleaseWork, false)) return EStep::Pending;
        Out.Empty(); Budget.Release(Charge); Charge = 0; bAllocated = false;
        return EStep::Complete;
    }
    EStep FPublicationClock::Retire(FBudget& Budget)
    {
        if (!Charge) return EStep::Complete;
        if (!QualifiedClockWork || QualifiedClockWork > TickWorkLimit) return EStep::ResourceUnavailable;
        const uint64 Work = QualifiedClockWork + 4 * static_cast<uint64>(GameRoomId.Len());
        if (Work > TickWorkLimit) return EStep::ResourceUnavailable;
        if (!Budget.Spend(Work, false)) return EStep::Pending;
        Value.Clear(); GameRoomId.Empty();
        return EStep::Complete;
    }
    namespace
    {
        EStep RetireHomeText(FBudget& Budget, FString& Text)
        {
            if (Text.GetCharArray().IsEmpty()) return EStep::Complete;
            if (!QualifiedTextReleaseWork || QualifiedTextReleaseWork > TickWorkLimit)
                return EStep::ResourceUnavailable;
            if (!Budget.Spend(QualifiedTextReleaseWork, false)) return EStep::Pending;
            Text.Empty(); return EStep::Complete;
        }
        EStep RetireHomeOptional(FBudget& Budget, TOptional<FString>& Text)
        {
            return Text.IsSet() ? RetireHomeText(Budget, Text.GetValue()) : EStep::Complete;
        }
        EStep RetireHomeAsset(FBudget& Budget, FCanonicalAssetReference& Asset)
        {
            for (auto* Text : { &Asset.Key, &Asset.Path, &Asset.Sha256, &Asset.Md5 })
            {
                const auto Result = RetireHomeText(Budget, *Text);
                if (Result != EStep::Complete) return Result;
            }
            return EStep::Complete;
        }
        template<typename T> EStep RetireHomeArray(FBudget& Budget, TArray<T>& Array)
        {
            if (Array.Max() == 0) return EStep::Complete;
            if (!QualifiedHomeReleaseWork || QualifiedHomeReleaseWork > TickWorkLimit)
                return EStep::ResourceUnavailable;
            if (!Budget.Spend(QualifiedHomeReleaseWork, false)) return EStep::Pending;
            // Owning element members must already be drained by the caller.
            Array.Empty(); return EStep::Complete;
        }
    }
    EStep FHomeDataRetirement::Shell(FBudget& Budget, FRoomShellPlan& Value)
    {
        while (ShellPhase < 9)
        {
            if (!Budget.Spend(32, false)) return EStep::Pending;
            EStep Result = EStep::Complete;
            switch (ShellPhase)
            {
            case 0: Result = RetireHomeText(Budget, Value.DerivedLayoutId); break;
            case 1:
            case 2:
                {
                    auto& Rows = ShellPhase == 1 ? Value.Obstacles : Value.CameraObstacles;
                    while (ShellIndex < static_cast<uint32>(Rows.Num()))
                    {
                        Result = RetireHomeText(Budget, Rows[ShellIndex].Id);
                        if (Result != EStep::Complete) return Result;
                        if (!Budget.Spend(32, false)) return EStep::Pending;
                        ++ShellIndex;
                    }
                    Result = RetireHomeArray(Budget, Rows);
                }
                break;
            case 3:
                while (ShellIndex < static_cast<uint32>(Value.Walls.Num()))
                {
                    auto& Wall = Value.Walls[ShellIndex];
                    Result = RetireHomeText(Budget, Wall.SourceId);
                    if (Result != EStep::Complete) return Result;
                    Result = RetireHomeText(Budget, Wall.Name);
                    if (Result != EStep::Complete) return Result;
                    Result = RetireHomeArray(Budget, Wall.Openings);
                    if (Result != EStep::Complete) return Result;
                    if (!Budget.Spend(32, false)) return EStep::Pending;
                    ++ShellIndex;
                }
                Result = RetireHomeArray(Budget, Value.Walls);
                break;
            case 4:
                while (ShellIndex < static_cast<uint32>(Value.Supports.Num()))
                {
                    auto& Support = Value.Supports[ShellIndex];
                    Result = RetireHomeText(Budget, Support.SourceId);
                    if (Result != EStep::Complete) return Result;
                    Result = RetireHomeText(Budget, Support.Name);
                    if (Result != EStep::Complete) return Result;
                    Result = RetireHomeArray(Budget, Support.AllowedKinds);
                    if (Result != EStep::Complete) return Result;
                    Result = RetireHomeArray(Budget, Support.Anchors);
                    if (Result != EStep::Complete) return Result;
                    while (ShellField < static_cast<uint32>(Support.SupportingObstacleIds.Num()))
                    {
                        Result = RetireHomeText(Budget, Support.SupportingObstacleIds[ShellField]);
                        if (Result != EStep::Complete) return Result;
                        if (!Budget.Spend(32, false)) return EStep::Pending;
                        ++ShellField;
                    }
                    Result = RetireHomeArray(Budget, Support.SupportingObstacleIds);
                    if (Result != EStep::Complete) return Result;
                    if (!Budget.Spend(32, false)) return EStep::Pending;
                    ++ShellIndex; ShellField = 0;
                }
                Result = RetireHomeArray(Budget, Value.Supports);
                break;
            case 5: Result = RetireHomeArray(Budget, Value.FloorPatches); break;
            case 6: Result = RetireHomeArray(Budget, Value.StandingCandidates); break;
            case 7:
                if (!Budget.Spend(QualifiedHomeReleaseWork, false)) return EStep::Pending;
                Value = {};
                break;
            case 8: break;
            }
            if (Result != EStep::Complete) return Result;
            ++ShellPhase; ShellIndex = ShellField = 0;
        }
        return EStep::Complete;
    }
    EStep FHomeDataRetirement::Room(FBudget& Budget, FRoomPlan& Value)
    {
        while (RoomPhase < 6)
        {
            if (!Budget.Spend(32, false)) return EStep::Pending;
            EStep Result = EStep::Complete;
            if (RoomPhase == 0)
            {
                for (auto* Text : { &Value.Id, &Value.Name, &Value.SourceGeometryRecipe })
                {
                    Result = RetireHomeText(Budget, *Text);
                    if (Result != EStep::Complete) return Result;
                }
                TOptional<FString>* Texts[] = { &Value.Environment.Saved, &Value.Layout.Saved,
                    &Value.Palette.Saved, &Value.Light.Saved, &Value.Sharing.Saved, &Value.WallColor,
                    &Value.FloorColor, &Value.UpholsteryColor, &Value.RugColor };
                while (RoomField < UE_ARRAY_COUNT(Texts))
                {
                    Result = RetireHomeOptional(Budget, *Texts[RoomField]);
                    if (Result != EStep::Complete) return Result;
                    if (!Budget.Spend(32, false)) return EStep::Pending;
                    ++RoomField;
                }
            }
            else if (RoomPhase == 1)
            {
                while (RoomIndex < static_cast<uint32>(Value.Items.Num()))
                {
                    auto& Item = Value.Items[RoomIndex];
                    Result = RetireHomeText(Budget, Item.Id);
                    if (Result != EStep::Complete) return Result;
                    Result = RetireHomeText(Budget, Item.SourceGeometryRecipe);
                    if (Result != EStep::Complete) return Result;
                    Result = RetireHomeOptional(Budget, Item.Kind.Saved);
                    if (Result != EStep::Complete) return Result;
                    Result = RetireHomeOptional(Budget, Item.Surface.Saved);
                    if (Result != EStep::Complete) return Result;
                    if (!Budget.Spend(32, false)) return EStep::Pending;
                    ++RoomIndex;
                }
                Result = RetireHomeArray(Budget, Value.Items);
            }
            else if (RoomPhase == 2 && Value.Asset.IsSet()) Result = RetireHomeAsset(Budget, Value.Asset.GetValue());
            else if (RoomPhase == 3 && Value.Shell.IsSet()) Result = Shell(Budget, Value.Shell.GetValue());
            else if (RoomPhase == 4)
            {
                if (!Budget.Spend(QualifiedHomeReleaseWork, false)) return EStep::Pending;
                Value = {};
            }
            if (Result != EStep::Complete) return Result;
            ++RoomPhase; RoomIndex = RoomField = 0;
        }
        return EStep::Complete;
    }
    EStep FHomeDataRetirement::Advance(FBudget& Budget, FHomePresentationData& Data)
    {
        if (Phase == 10) return EStep::Complete;
        if (!QualifiedHomeBytes || !QualifiedHomeReleaseWork || QualifiedHomeReleaseWork > TickWorkLimit)
            return EStep::ResourceUnavailable;
        while (Phase < 10)
        {
            if (!Budget.Spend(32, false)) return EStep::Pending;
            EStep Result = EStep::Complete;
            if (Phase < 2) Result = Room(Budget, Phase == 0 ? Data.Home : Data.ActiveRoom);
            else if (Phase == 2)
            {
                while (Index < static_cast<uint32>(Data.Rooms.Num()))
                {
                    Result = Room(Budget, Data.Rooms[Index]);
                    if (Result != EStep::Complete) return Result;
                    if (!Budget.Spend(32, false)) return EStep::Pending;
                    ++Index; RoomPhase = RoomIndex = RoomField = ShellPhase = ShellIndex = ShellField = 0;
                }
                Result = RetireHomeArray(Budget, Data.Rooms);
            }
            else if (Phase == 3)
            {
                while (Index < static_cast<uint32>(Data.Apartment.Connections.Num()))
                {
                    auto& Connection = Data.Apartment.Connections[Index];
                    Result = RetireHomeText(Budget, Connection.From.RoomId);
                    if (Result != EStep::Complete) return Result;
                    Result = RetireHomeText(Budget, Connection.To.RoomId);
                    if (Result != EStep::Complete) return Result;
                    if (!Budget.Spend(32, false)) return EStep::Pending;
                    ++Index;
                }
                Result = RetireHomeArray(Budget, Data.Apartment.Connections);
                if (Result != EStep::Complete) return Result;
                Result = RetireHomeOptional(Budget, Data.Apartment.SavedLivingRoomId);
            }
            else if (Phase == 4)
            {
                auto& A = Data.Appearance;
                TOptional<FString>* Texts[] = { &A.Base.Saved, &A.Look.Saved, &A.BodyColor, &A.TopColor,
                    &A.TrousersColor, &A.FootwearColor, &A.EyeColor, &A.NoseColor, &A.MarkingColor,
                    &A.PatternColor, &A.Garment.Saved, &A.Fabric.Saved, &A.Pattern.Saved, &A.Eyes.Saved,
                    &A.Brows.Saved, &A.Nose.Saved, &A.Mouth.Saved, &A.Cheeks.Saved, &A.Markings.Saved,
                    &A.Eyewear.Saved, &A.EffectiveBodyColor, &A.EffectiveNoseColor };
                while (Field < UE_ARRAY_COUNT(Texts))
                {
                    Result = RetireHomeOptional(Budget, *Texts[Field]);
                    if (Result != EStep::Complete) return Result;
                    if (!Budget.Spend(32, false)) return EStep::Pending;
                    ++Field;
                }
                Result = RetireHomeText(Budget, A.EffectiveMarkingColor);
                if (Result != EStep::Complete) return Result;
                Result = RetireHomeText(Budget, A.EffectivePatternColor);
            }
            else if (Phase == 5) Result = RetireHomeAsset(Budget, Data.CharacterAsset);
            else if (Phase == 6)
            {
                while (Index < static_cast<uint32>(Data.CharacterMaterials.Num()))
                {
                    auto& Material = Data.CharacterMaterials[Index];
                    Result = RetireHomeText(Budget, Material.SourceMaterialName);
                    if (Result != EStep::Complete) return Result;
                    Result = RetireHomeText(Budget, Material.SourceField);
                    if (Result != EStep::Complete) return Result;
                    Result = RetireHomeOptional(Budget, Material.TargetHex);
                    if (Result != EStep::Complete) return Result;
                    if (!Budget.Spend(32, false)) return EStep::Pending;
                    ++Index;
                }
                Result = RetireHomeArray(Budget, Data.CharacterMaterials);
            }
            else if (Phase == 7)
            {
                while (Index < static_cast<uint32>(Data.Attachments.Num()))
                {
                    auto& Attachment = Data.Attachments[Index];
                    Result = RetireHomeText(Budget, Attachment.SourceNode);
                    if (Result != EStep::Complete) return Result;
                    Result = RetireHomeText(Budget, Attachment.SourceRecipe);
                    if (Result != EStep::Complete) return Result;
                    if (!Budget.Spend(32, false)) return EStep::Pending;
                    ++Index;
                }
                Result = RetireHomeArray(Budget, Data.Attachments);
            }
            else if (Phase == 8)
            {
                while (Index < static_cast<uint32>(Data.Issues.Num()))
                {
                    Result = RetireHomeText(Budget, Data.Issues[Index].Field);
                    if (Result != EStep::Complete) return Result;
                    if (!Budget.Spend(32, false)) return EStep::Pending;
                    ++Index;
                }
                Result = RetireHomeArray(Budget, Data.Issues);
            }
            else if (Phase == 9)
            {
                if (!Budget.Spend(QualifiedHomeReleaseWork, false)) return EStep::Pending;
                Data = {};
            }
            if (Result != EStep::Complete) return Result;
            ++Phase; Index = Field = 0;
            RoomPhase = RoomIndex = RoomField = ShellPhase = ShellIndex = ShellField = 0;
        }
        return EStep::Complete;
    }
    EStep FPublicationPayload::Retire(FBudget& Budget)
    {
        if (bHomePreparationStarted)
        {
            const auto Result = HomeRetirement.Advance(Budget, Home);
            if (Result != EStep::Complete) return Result;
            Budget.Release(HomeCharge); HomeCharge = 0;
            bHomePreparationStarted = false; bHomeMappingFinished = bHomeMapped = false;
            HomeUnknown.Count = 0;
        }
        if ((HealthValue || RetiredRecovery.Health || NextRecovery.Health) && !QualifiedHealthReleaseWork)
            return EStep::ResourceUnavailable;
        if (HealthValue)
        {
            const int32 OwnedReferences = 1 + (NextRecovery.Health ? 1 : 0) + (RetiredRecovery.Health ? 1 : 0);
            // A live Recovery view still sharing this record must move out
            // before retirement may erase any of its strings.
            if (HealthValue.GetSharedReferenceCount() != OwnedReferences) return EStep::Pending;
        }
        for (auto* Slot : { &RetiredClock, &Clock })
        {
            if (!*Slot) continue;
            if (!Budget.Spend(32, false)) return EStep::Pending;
            uint64 FinalCharge = 0;
            if (Slot->GetSharedReferenceCount() == 1)
            {
                const auto Result = (*Slot)->Retire(Budget);
                if (Result != EStep::Complete) return Result;
                FinalCharge = (*Slot)->Charge;
            }
            if (!QualifiedRetirementDeleteWork || !Budget.Spend(QualifiedRetirementDeleteWork, false)) return EStep::Pending;
            Slot->Reset();
            Budget.Release(FinalCharge);
        }
        if (HealthValue || RetiredRecovery.Health || NextRecovery.Health || !NextRecovery.OwnerId.IsEmpty()
            || !RetiredRecovery.OwnerId.IsEmpty())
        {
            if ((NextRecovery.Health && NextRecovery.Health.Get() != HealthValue.Get())
                || (RetiredRecovery.Health && RetiredRecovery.Health.Get() != HealthValue.Get())) return EStep::Invalid;
            while (HealthRetirePhase < 9)
            {
                FString* Value = nullptr;
                switch (HealthRetirePhase)
                {
                case 0: Value = &NextRecovery.OwnerId; break;
                case 1: Value = &RetiredRecovery.OwnerId; break;
                case 2: if (NextRecovery.Error.IsSet()) Value = &NextRecovery.Error.GetValue(); break;
                case 3: if (RetiredRecovery.Error.IsSet()) Value = &RetiredRecovery.Error.GetValue(); break;
                case 4: if (HealthValue) Value = &HealthValue->LifeId; break;
                case 5: if (HealthValue && HealthValue->DeathKey.IsSet()) Value = &HealthValue->DeathKey.GetValue(); break;
                case 6: if (HealthValue && HealthValue->Recovery.IsSet()) Value = &HealthValue->Recovery->PlaceId; break;
                case 7: if (HealthValue && HealthValue->Recovery.IsSet()) Value = &HealthValue->Recovery->InstanceId; break;
                case 8:
                    if (!QualifiedHealthReleaseWork || QualifiedHealthReleaseWork > TickWorkLimit) return EStep::ResourceUnavailable;
                    if (!Budget.Spend(QualifiedHealthReleaseWork, false)) return EStep::Pending;
                    NextRecovery = {}; RetiredRecovery = {}; HealthValue.Reset();
                    Budget.Release(HealthCharge); HealthCharge = 0;
                    break;
                }
                if (Value && !Value->GetCharArray().IsEmpty())
                {
                    if (!QualifiedTextReleaseWork || QualifiedTextReleaseWork > TickWorkLimit) return EStep::ResourceUnavailable;
                    if (!Budget.Spend(QualifiedTextReleaseWork, false)) return EStep::Pending;
                    Value->Empty();
                }
                ++HealthRetirePhase;
            }
        }
        if (Movement.IsSet())
        {
            if (!QualifiedMovementWindowWork || QualifiedMovementWindowWork > TickWorkLimit) return EStep::ResourceUnavailable;
            if (!Budget.Spend(QualifiedMovementWindowWork, false)) return EStep::Pending;
            Movement.Reset();
        }
        Budget.Release(NativeCharge); NativeCharge = 0;
        bComplete = false; return EStep::Complete;
    }
    EStep FPublicationPreparation::Begin(FStorage& Storage)
    {
        if (!QualifiedPublicationBytes || !QualifiedPublicationPreparationBytes
            || !QualifiedPayloadInitializationWork || QualifiedPayloadInitializationWork > TickWorkLimit)
            return EStep::ResourceUnavailable;
        if (!Storage.bComplete || !Storage.bSemanticsComplete || Storage.bRetiring || Storage.Publication
            || bCancelled || bOwnPreparation || Storage.ActivePreparations == MAX_uint32)
            return EStep::Invalid;
        if (!Storage.Budget->Spend(QualifiedPayloadInitializationWork))
            return Storage.Budget->bExhausted ? EStep::ResourceUnavailable : EStep::Pending;
        if (!Storage.Budget->Reserve(QualifiedPublicationBytes)) return EStep::ResourceUnavailable;
        Storage.Publication = new FPublicationPayload;
        ++Storage.ActivePreparations;
        bOwnPreparation = true;
        Storage.Publication->Charge = QualifiedPublicationBytes;
        auto* Local = Storage.Players;
        // Semantic allocation puts the local player first, before all peers.
        if (Local && (Local->bPeer || !Local->bIncluded)) return EStep::Invalid;
        if (!Local) return EStep::Invalid;
        Storage.Publication->Local = Local; LocalRaw = Local->Raw; Position = Local->Position.Ref;
        Instance = {1, 2, Storage.Roots[2]}; Selection = {1, 16, Storage.Roots[16]};
        Storage.Publication->InstanceRoot = Instance; Storage.Publication->Selection = Selection;
        Storage.Publication->Health = {1, 15, Storage.Roots[15]};
        Storage.Publication->Player = Local->Header;
        Storage.Publication->HomeRoom = InstanceHome;
        return EStep::Complete;
    }
    EStep FPublicationPreparation::Advance(FStorage& Storage)
    {
        if (bCancelled || !Storage.Publication || Storage.bRetiring) return EStep::Invalid;
        auto& Payload = *Storage.Publication;
        while (Phase < 4)
        {
            const EDomain Domains[] = {EDomain::Player, EDomain::Pose, EDomain::Instance, EDomain::Navigation};
            const FValueRef Refs[] = {LocalRaw, Position, Instance, Navigation};
            if (!bStarted)
            {
                if (!Storage.Budget->Spend(FRecordRead::BeginWork())) return EStep::Pending;
                Record.Begin(Refs[Phase], *DomainSchema(Domains[Phase]).Record); bStarted = true;
            }
            const auto Result = Record.Advance(Storage);
            if (Result != EStep::Complete) return Result;
            if (!Storage.Budget->Spend(256)) return EStep::Pending;
            switch (Phase)
            {
            case 0:
                Payload.Player.Revision = static_cast<int64>(Record.Number(6));
                Payload.Player.MoveSequence = static_cast<int64>(Record.Number(7));
                Payload.Player.Status = static_cast<EWorldPlayerStatus>(Record.Choice(10));
                Payload.Player.Health = Record.Number(11); Payload.Player.Movement = Record.Presence(9);
                Payload.Player.bControlUnavailable = Payload.Local->bControlUnavailable;
                MovementRef = Record.Field(9);
                break;
            case 1: Payload.Player.Position = {Record.Number(0), Record.Number(1), Record.Number(2)}; break;
            case 2:
                Payload.Instance.SharedStartedAtMs = static_cast<int64>(Record.Number(5));
                Payload.Instance.Relation = Payload.Player.Status == EWorldPlayerStatus::Traveling
                    ? EWorldInstanceRelation::TravelDestination : EWorldInstanceRelation::CurrentVisit;
                LayoutId = Record.Field(4); Navigation = Record.Field(10); break;
            case 3:
                Payload.Instance.FloorY = Record.Number(3); NavigationRevision = Record.Field(0); break;
            }
            ++Phase; bStarted = false;
        }
        const auto Yield = [&]() { return Storage.Budget->bExhausted ? EStep::ResourceUnavailable : EStep::Pending; };
        const auto Read = [&](FValueRef Value, const FRecordSchema& Schema, FRecordRead& Reader) -> EStep
        {
            if (!bStarted)
            {
                if (!Storage.Budget->Spend(FRecordRead::BeginWork())) return Yield();
                Reader.Begin(Value, Schema); bStarted = true;
            }
            return Reader.Advance(Storage);
        };
        while (Phase < 255)
        {
            EStep Result = EStep::Complete;
            if (Phase == 4)
            {
                while (TransportStage < 6)
                {
                    const bool bCurrent = Payload.Instance.Relation == EWorldInstanceRelation::CurrentVisit;
                    const bool bDriving = bCurrent && Payload.Local->Driving.Ref.IsSet();
                    const bool bWater = bCurrent && Payload.Local->Water.Ref.IsSet();
                    if (TransportStage == 0 && bDriving)
                    {
                        Result = Read(Payload.Local->Driving.Ref, *DomainSchema(EDomain::Driving).Record, Record);
                        if (Result == EStep::Complete) TransportNavigation = Record.Field(4);
                    }
                    else if (TransportStage == 2 && bWater)
                    {
                        Result = Read(Payload.Local->Water.Ref, *DomainSchema(EDomain::Water).Record, Record);
                        if (Result == EStep::Complete) WaterScope = Record.Field(3);
                    }
                    else if (TransportStage == 3 && bWater)
                    {
                        Result = Read(WaterScope, *DomainSchema(EDomain::WaterScope).Record, Record);
                        if (Result == EStep::Complete)
                        { WaterLayout = Record.Field(3); WaterRevision = Record.Field(4); }
                    }
                    else if ((TransportStage == 1 && bDriving) || (TransportStage >= 4 && bWater))
                    {
                        if (!bStarted)
                        {
                            if (!Storage.Budget->Spend(2 * sizeof(FTextRead) + 64)) return Yield();
                            Compare.Compare(TransportStage == 1 ? TransportNavigation
                                : TransportStage == 4 ? WaterLayout : WaterRevision,
                                TransportStage == 4 ? LayoutId : NavigationRevision);
                            bStarted = true;
                        }
                        Result = Compare.Advance(Storage);
                        if (Result == EStep::Complete) Payload.Instance.bTransportLayoutMismatch |= !Compare.Equal();
                    }
                    if (Result != EStep::Complete) return Result;
                    if (!Storage.Budget->Spend(32)) return Yield();
                    ++TransportStage; bStarted = false;
                }
                if (!Storage.Budget->Spend(128)) return Yield();
                Payload.Player.ServerNowMs = ServerNowMs;
                Payload.Instance.bOwnHome = bOwnHome;
                Payload.Instance.bPlayerInstanceMatches = Payload.Instance.Relation == EWorldInstanceRelation::CurrentVisit;
                if (Payload.Player.Movement == EWorldFieldPresence::Value)
                {
                    if (!QualifiedMovementWindowBytes || !QualifiedMovementWindowWork
                        || !QualifiedMovementRouteInitializationWork
                        || QualifiedMovementRouteInitializationWork > TickWorkLimit) return EStep::ResourceUnavailable;
                    if (!Storage.Budget->Spend(QualifiedMovementRouteInitializationWork)) return Yield();
                    if (!Storage.Budget->Reserve(QualifiedMovementWindowBytes)) return EStep::ResourceUnavailable;
                    Payload.NativeCharge += QualifiedMovementWindowBytes;
                    Payload.Movement.Emplace(); Payload.Movement->Path.Reserve(32);
                }
            }
            else if (Phase == 5 && Payload.Movement.IsSet())
            {
                Result = Read(MovementRef, *DomainSchema(EDomain::Movement).Record, Record);
                if (Result != EStep::Complete) return Result;
                if (!Storage.Budget->Spend(256)) return Yield();
                auto& Route = Payload.Movement.GetValue();
                Route.Version = static_cast<int32>(Record.Number(0)); Route.Sequence = static_cast<int64>(Record.Number(1));
                Route.RoomRevision = static_cast<int64>(Record.Number(2)); Route.StartedAtMs = Record.Number(3);
                Route.YawRadians = Record.Number(5);
                if (Record.Presence(6) == EWorldFieldPresence::Value) Route.InitialSpeedMps = Record.Number(6);
                Route.Pace = Record.Presence(7) == EWorldFieldPresence::Value
                    ? static_cast<EMovementPace>(Record.Choice(7) + 1) : EMovementPace::Unpaced;
                RouteFrom = Record.Field(4); RoutePath = Record.Field(8);
                const auto* Path = Resolve(Storage, RoutePath);
                if (!Path) return EStep::Invalid;
                RouteNext = Path->FirstChild;
            }
            else if (Phase == 6 && Payload.Movement.IsSet())
            {
                Result = Read(RouteFrom, *DomainSchema(EDomain::Pose).Record, Record);
                if (Result != EStep::Complete) return Result;
                if (!Storage.Budget->Spend(sizeof(FMovementPose))) return Yield();
                Payload.Movement->From = {Record.Number(0), Record.Number(1), Record.Number(2)};
            }
            else if (Phase == 7 && Payload.Movement.IsSet())
            {
                while (RouteNext)
                {
                    Result = Read(RoutePath.Child(RouteNext), *DomainSchema(EDomain::MovementPoint).Record, Record);
                    if (Result != EStep::Complete) return Result;
                    if (!Storage.Budget->Spend(sizeof(FMovementPoint) + 32)) return Yield();
                    const auto* Point = Resolve(Storage, RoutePath.Child(RouteNext));
                    if (!Point || Payload.Movement->Path.Num() >= 32) return EStep::Invalid;
                    Payload.Movement->Path.Add({Record.Number(0), Record.Number(1)});
                    RouteNext = Point->NextSibling; bStarted = false;
                }
                if (Payload.Movement->Sequence != Payload.Player.MoveSequence
                    || Payload.Movement->RoomRevision != Payload.Player.Revision) return EStep::Invalid;
            }
            else if (Phase == 8)
            {
                Result = Read(Payload.Health, HealthSchema(), HealthRecord);
                if (Result != EStep::Complete) return Result;
                HealthLife = HealthRecord.Field(0);
            }
            else if (Phase == 9)
            {
                if (!ExpectedLife) return EStep::Invalid;
                if (!bStarted)
                {
                    if (!Storage.Budget->Spend(2 * sizeof(FTextRead) + 64)) return Yield();
                    Compare.Compare(HealthLife, *ExpectedLife); bStarted = true;
                }
                Result = Compare.Advance(Storage);
                if (Result != EStep::Complete) return Result;
                if (!Compare.Equal()) return EStep::Invalid;
            }
            else if (Phase == 10)
            {
                if (!QualifiedHealthBytes || !QualifiedHealthInitializationWork
                    || QualifiedHealthInitializationWork > TickWorkLimit) return EStep::ResourceUnavailable;
                if (!Storage.Budget->Spend(QualifiedHealthInitializationWork)) return Yield();
                if (!Storage.Budget->Reserve(QualifiedHealthBytes)) return EStep::ResourceUnavailable;
                Payload.HealthCharge += QualifiedHealthBytes;
                Payload.HealthValue = MakeShared<FCompanionHealthView>();
                auto& Value = *Payload.HealthValue;
                Value.Revision = static_cast<int64>(HealthRecord.Number(1)); Value.Health = HealthRecord.Number(2);
                Value.MaxHealth = HealthRecord.Number(3); Value.Status = static_cast<ECompanionHealthStatus>(HealthRecord.Choice(4));
                Value.ProtectedUntilMs = HealthRecord.Number(6);
                if (HealthRecord.Presence(5) == EWorldFieldPresence::Value) Value.DeathKey.Emplace();
                if (HealthRecord.Presence(7) == EWorldFieldPresence::Value) Value.Recovery.Emplace();
                Payload.NextRecovery.Health = Payload.HealthValue; Payload.NextRecovery.HealthPresence = EWorldFieldPresence::Value;
                bHealthAllocated = true;
            }
            else if (Phase == 11)
            {
                while (TextStage < 3)
                {
                    const FValueRef Values[] = {OwnerId, HealthLife, HealthRecord.Field(5)};
                    if (TextStage == 2 && !Payload.HealthValue->DeathKey.IsSet()) { ++TextStage; continue; }
                    FString& Destination = TextStage == 0 ? Payload.NextRecovery.OwnerId
                        : TextStage == 1 ? Payload.HealthValue->LifeId : Payload.HealthValue->DeathKey.GetValue();
                    if (!bTextStarted) { Text.Begin(Values[TextStage]); bTextStarted = true; }
                    Result = Text.Advance(Storage, Destination, QualifiedHealthBytes);
                    if (Result != EStep::Complete) return Result;
                    Payload.HealthCharge += Text.TakeCharge();
                    ++TextStage; bTextStarted = false;
                }
            }
            else if (Phase == 12 && Payload.HealthValue->Recovery.IsSet())
            {
                Result = Read(HealthRecord.Field(7), RecoveryDestinationSchema(), DestinationRecord);
                if (Result != EStep::Complete) return Result;
                Payload.HealthValue->Recovery->ReadyAtMs = DestinationRecord.Number(3);
                const auto* PositionArray = Resolve(Storage, DestinationRecord.Field(2));
                if (!PositionArray) return EStep::Invalid;
                DestinationNext = PositionArray->FirstChild; TextStage = Coordinate = 0;
            }
            else if (Phase == 13 && Payload.HealthValue->Recovery.IsSet())
            {
                while (TextStage < 2)
                {
                    FString& Destination = TextStage == 0 ? Payload.HealthValue->Recovery->PlaceId : Payload.HealthValue->Recovery->InstanceId;
                    if (!bTextStarted) { Text.Begin(DestinationRecord.Field(TextStage)); bTextStarted = true; }
                    Result = Text.Advance(Storage, Destination, QualifiedHealthBytes);
                    if (Result != EStep::Complete) return Result;
                    Payload.HealthCharge += Text.TakeCharge();
                    ++TextStage; bTextStarted = false;
                }
                while (Coordinate < 3)
                {
                    if (!Storage.Budget->Spend(32)) return Yield();
                    const auto* Node = Resolve(Storage, DestinationRecord.Field(2).Child(DestinationNext));
                    if (!Node || Node->Kind != EWorldValueKind::Number) return EStep::Invalid;
                    double Value = 0; FMemory::Memcpy(&Value, &Node->NumberBits, sizeof(Value));
                    auto& PositionValue = Payload.HealthValue->Recovery->Position;
                    if (Coordinate == 0) PositionValue.X = Value;
                    else if (Coordinate == 1) PositionValue.Y = Value;
                    else PositionValue.Z = Value;
                    DestinationNext = Node->NextSibling; ++Coordinate;
                }
                if (DestinationNext) return EStep::Invalid;
            }
            else if (Phase == 14)
            {
                if (!Storage.Budget->Spend(128)) return Yield();
                if (!RecoveryOwner || !RecoveryOwner->StageWorldSubscription(RecoveryReceipt, Payload.NextRecovery)) return EStep::Invalid;
            }
            else if (Phase == 15)
            {
                if (!ExpectedGameRoom || !Baseline || !QualifiedClockBytes || !QualifiedClockWork) return EStep::ResourceUnavailable;
                const uint64 Units = static_cast<uint64>(ExpectedGameRoom->Len());
                if (QualifiedClockWork > TickWorkLimit || Units > (TickWorkLimit - QualifiedClockWork) / 8
                    || QualifiedClockBytes > RetainedLimit || Units + 1 > (RetainedLimit - QualifiedClockBytes) / 4)
                    return EStep::ResourceUnavailable;
                const uint64 Work = QualifiedClockWork + 8 * Units;
                if (!Storage.Budget->Spend(Work)) return Yield();
                const uint64 Bytes = QualifiedClockBytes + 4 * (Units + 1);
                if (!Storage.Budget->Reserve(Bytes)) return EStep::ResourceUnavailable;
                Payload.Clock = MakeShared<FPublicationClock>(); Payload.Clock->Charge = Bytes;
                Payload.Clock->GameRoomId = *ExpectedGameRoom;
                if (*Baseline) Payload.Clock->Value = (*Baseline)->Value;
                else { Payload.Clock->Value.Clear(); Payload.Clock->Value.SelectGameRoom(Payload.Clock->GameRoomId); }
                if (!Payload.Clock->Value.ObserveCaptured(Payload.Clock->GameRoomId, ServerNowMs, TicketArrival)) return EStep::Invalid;
                bClockPrepared = true;
            }
            else if (Phase == 16)
            {
                if (!bHomeStarted)
                {
                    if (!Storage.Budget->Spend(128)) return Yield();
                    Home.Begin(SelectedCharacter, SelectedRoom); bHomeStarted = true;
                }
                Result = Home.Advance(Storage);
                if (Result == EStep::Complete) Phase = 254;
            }
            if (Result != EStep::Complete) return Result;
            ++Phase; bStarted = false;
        }
        Payload.bComplete = true; return EStep::Complete;
    }
    EStep FPublicationPreparation::Retire(FStorage& Storage)
    {
        bCancelled = true;
        if (!bOwnPreparation && !bHomeStarted) return EStep::Complete;
        if (bHomeStarted)
        {
            const auto Result = Home.Retire(Storage);
            if (Result != EStep::Complete) return Result;
            bHomeStarted = false;
        }
        if (!Storage.Budget->Spend(FRecordRead::BeginWork(), false)) return EStep::Pending;
        if (Storage.Publication) Storage.Publication->HealthCharge += Text.TakeCharge();
        Record.Cancel(); HealthRecord.Cancel(); DestinationRecord.Cancel(); Health.Cancel(); Compare.Cancel();
        if (bOwnPreparation)
        {
            check(Storage.ActivePreparations);
            --Storage.ActivePreparations; bOwnPreparation = false;
        }
        return EStep::Complete;
    }
    FHomeBorrow::~FHomeBorrow()
    {
        if (Storage)
        {
            check(Storage->ActiveBorrows); --Storage->ActiveBorrows;
            Data = nullptr;
        }
    }
}

namespace BaddieCompanion
{
    using namespace WorldObservationPrivate;
    using namespace WorldReadPrivate;
    namespace
    {
        EWorldReadResult Pending(const FStorage&)
        {
            // Candidate exhaustion cannot revoke independently current accepted
            // reads. Consumer work uses only the shared frame allowance.
            return EWorldReadResult::Pending;
        }
        FValueRef Ref(const void* Owner, uint8 Root, uint64 Node)
        { return {Node, Root, static_cast<FRoot*>(const_cast<void*>(Owner))}; }
    }
    EWorldReadResult FWorldValueRead::Scalar(FWorldScalarRead& Out) const
    {
        Out = {};
        const auto Admission = Observation.AdmitRead();
        if (Admission != EWorldReadResult::Ready) return Admission;
        const auto Access = Observation.Access.Pin(); const auto Storage = Access->Storage.Pin();
        if (!Storage->Budget->Spend(sizeof(Out), false)) return Pending(*Storage);
        Out.Presence = Presence;
        if (bDefaultBase)
        { Out.Kind = EWorldValueKind::String; Out.Count = 6; Out.Choice = 0; return EWorldReadResult::Ready; }
        if (Presence == EWorldFieldPresence::Missing) return EWorldReadResult::Ready;
        if (Presence == EWorldFieldPresence::Null && !Node) return EWorldReadResult::Ready;
        const auto* Value = Resolve(*Storage, Ref(Owner, Root, Node));
        if (!Value) { Out = {}; return EWorldReadResult::Unavailable; }
        Out.Kind = Value->Kind; Out.Count = Value->Count; Out.Boolean = Value->Count != 0;
        if (bMergedInputs && Effective) Out.Count = static_cast<const FPlayerResult*>(Effective)->DanceCount;
        if (Value->Kind == EWorldValueKind::Number) FMemory::Memcpy(&Out.Number, &Value->NumberBits, sizeof(double));
        return EWorldReadResult::Ready;
    }
    FWorldValueRead FWorldValueRead::Raw() const
    {
        auto Out = *this;
        if (bHasSource)
        { Out.Node = SourceNode; Out.Root = SourceRoot; Out.Owner = SourceOwner; Out.Presence = SourcePresence; }
        Out.Kind = EWorldReadDomain::Raw; Out.Effective = nullptr; Out.bMergedInputs = false;
        Out.ChoiceRule = nullptr; Out.bHasSource = false; Out.bDefaultBase = false; return Out;
    }
    EWorldReadResult FWorldValueRead::ReadNode(FWorldNodeView& Out) const
    {
        Out = {};
        const auto Admission = Observation.AdmitRead();
        if (Admission != EWorldReadResult::Ready) return Admission;
        if (Presence == EWorldFieldPresence::Missing || bDefaultBase) return EWorldReadResult::Unavailable;
        const auto Access = Observation.Access.Pin(); const auto Storage = Access->Storage.Pin();
        if (!Storage->Budget->Spend(sizeof(Out) + 16, false)) return Pending(*Storage);
        const auto* Value = Resolve(*Storage, Ref(Owner, Root, Node));
        if (!Value) return EWorldReadResult::Unavailable;
        Out = *Value; return EWorldReadResult::Ready;
    }
    EWorldReadResult FWorldValueRead::ReadPresence(FWorldValuePresence& Out) const
    {
        Out = {};
        const auto Admission = Observation.AdmitRead();
        if (Admission != EWorldReadResult::Ready) return Admission;
        const auto Access = Observation.Access.Pin(); const auto Storage = Access->Storage.Pin();
        if (!Storage->Budget->Spend(sizeof(Out), false)) return Pending(*Storage);
        Out.Supplied = bHasSource ? SourcePresence : Presence; Out.Effective = Presence;
        Out.bJsonDocument = bJsonDocument;
        Out.bSuppressed = Out.Supplied == EWorldFieldPresence::Value && Presence == EWorldFieldPresence::Missing;
        Out.bControlUnavailable = Effective && static_cast<const FPlayerResult*>(Effective)->bControlUnavailable;
        return EWorldReadResult::Ready;
    }
    EWorldReadResult FWorldValueRead::Choice(FWorldChoiceCursor& Cursor, int32& Out) const
    {
        Out = -1;
        const auto Admission = Observation.AdmitRead();
        if (Admission != EWorldReadResult::Ready)
        { if (Admission == EWorldReadResult::Unavailable) Cursor = {}; return Admission; }
        const auto Access = Observation.Access.Pin(); const auto Storage = Access->Storage.Pin();
        const auto* Rule = static_cast<const FFieldRule*>(ChoiceRule);
        if (bDefaultBase)
        {
            if (!Storage->Budget->Spend(16, false)) return Pending(*Storage);
            Cursor = {}; Out = 0; return EWorldReadResult::Ready;
        }
        if (!Rule || Presence != EWorldFieldPresence::Value) return EWorldReadResult::Ready;
        if (!Storage->Budget->Spend(16, false)) return Pending(*Storage);
        const auto* Value = Resolve(*Storage, Ref(Owner, Root, Node));
        if (!Value || Value->Kind != EWorldValueKind::String) return EWorldReadResult::Unavailable;
        if (!Cursor.bStarted)
        { Cursor.Token = Observation.Token(); Cursor.Owner = Owner; Cursor.Node = Node; Cursor.bStarted = true; }
        if (Cursor.Token != Observation.Token() || Cursor.Owner != Owner || Cursor.Node != Node)
            return EWorldReadResult::Unavailable;
        while (Cursor.Choice < Rule->ChoiceCount)
        {
            const auto& Candidate = Rule->Choices[Cursor.Choice];
            if (!Cursor.bComparing)
            {
                if (!Storage->Budget->Spend(16, false)) return Pending(*Storage);
                Cursor.Offset = 0; Cursor.Block = Value->Text;
                Cursor.bEqual = Value->Count == Candidate.Count; Cursor.bComparing = true;
            }
            while (Cursor.bEqual && Cursor.Block)
            {
                const auto* Block = static_cast<const FStringBlock*>(Cursor.Block);
                if (!Storage->Budget->Spend(2 * Block->Used + 16, false)) return Pending(*Storage);
                for (uint16 I = 0; I < Block->Used; ++I)
                {
                    if (Cursor.Offset >= Candidate.Count || Block->Units[I] != static_cast<uint16>(Candidate.Text[Cursor.Offset]))
                        Cursor.bEqual = false;
                    ++Cursor.Offset;
                }
                Cursor.Block = Block->Next;
            }
            if (Cursor.bEqual)
            { Out = static_cast<int32>(Cursor.Choice); Cursor = {}; return EWorldReadResult::Ready; }
            if (!Storage->Budget->Spend(8, false)) return Pending(*Storage);
            ++Cursor.Choice; Cursor.bComparing = false;
        }
        Cursor = {}; return EWorldReadResult::Ready;
    }
    EWorldReadResult FWorldValueRead::Field(uint32 Index, FWorldFieldCursor& Cursor, FWorldValueRead& Out) const
    {
        Out = {};
        const auto Admission = Observation.AdmitRead();
        if (Admission != EWorldReadResult::Ready)
        { if (Admission == EWorldReadResult::Unavailable) Cursor = {}; return Admission; }
        const auto Access = Observation.Access.Pin(); const auto Storage = Access->Storage.Pin();
        const auto* Schema = ReadSchema(Kind);
        if (!Schema || Index >= Schema->Count) return EWorldReadResult::Unavailable;
        if (!Storage->Budget->Spend(32, false)) return Pending(*Storage);
        const auto* Object = Resolve(*Storage, Ref(Owner, Root, Node));
        if (!Object || Object->Kind != EWorldValueKind::Object) return EWorldReadResult::Unavailable;
        if (!Cursor.bStarted)
        {
            if (!Storage->Budget->Spend(sizeof(Cursor), false)) return Pending(*Storage);
            Cursor.Token = Observation.Token(); Cursor.Record = Node; Cursor.Owner = Owner;
            Cursor.Field = Index; Cursor.Key = Object->FirstChild; Cursor.bStarted = true;
        }
        if (Cursor.Token != Observation.Token() || Cursor.Record != Node || Cursor.Owner != Owner || Cursor.Field != Index)
            return EWorldReadResult::Unavailable;
        const auto& Rule = Schema->Fields[Index];
        while (Cursor.Key)
        {
            if (!Storage->Budget->Spend(32, false)) return Pending(*Storage);
            const auto* Key = Resolve(*Storage, Ref(Owner, Root, Cursor.Key));
            if (!Key || Key->Kind != EWorldValueKind::String || Key->Role != 1) return EWorldReadResult::Unavailable;
            if (!Cursor.bComparing)
            { Cursor.bEqual = Key->Count == Rule.NameLength; Cursor.Block = Key->Text; Cursor.bComparing = true; }
            while (Cursor.bEqual && Cursor.Block)
            {
                const auto* Block = static_cast<const FStringBlock*>(Cursor.Block);
                if (!Storage->Budget->Spend(2 * Block->Used + 16, false)) return Pending(*Storage);
                for (uint16 I = 0; I < Block->Used; ++I)
                {
                    if (Cursor.Offset >= Rule.NameLength || Block->Units[I] != static_cast<uint16>(Rule.Name[Cursor.Offset]))
                        Cursor.bEqual = false;
                    ++Cursor.Offset;
                }
                Cursor.Block = Block->Next;
            }
            const auto* Value = Resolve(*Storage, Ref(Owner, Root, Key->NextSibling));
            if (!Value) return EWorldReadResult::Unavailable;
            if (Cursor.bEqual)
            {
                if (!Storage->Budget->Spend(sizeof(Out), false)) return Pending(*Storage);
                Out = *this; Out.Node = Value->Id; Out.Kind = ChildDomain(Kind, Index);
                Out.Presence = Value->Kind == EWorldValueKind::Null ? EWorldFieldPresence::Null : EWorldFieldPresence::Value;
                Out.SourceNode = Out.Node; Out.SourceRoot = Root; Out.SourceOwner = Owner;
                Out.SourcePresence = Out.Presence; Out.bHasSource = true; Out.ChoiceRule = &Rule;
                Out.bMergedInputs = false; Out.bDefaultBase = false; Out.bJsonDocument = false;
                if (Effective && Kind == EWorldReadDomain::Player && (Index == 8 || Index == 15 || Index == 16 || Index == 19))
                {
                    const auto* Player = static_cast<const FPlayerResult*>(Effective);
                    const auto Selected = Index == 8 ? Player->Position.Ref : Index == 15 ? Player->Driving.Ref
                        : Index == 16 ? Player->Water.Ref : Player->Weapon.Ref;
                    if (Selected.IsSet())
                    {
                        const auto* SelectedNode = Resolve(*Storage, Selected);
                        if (!SelectedNode) { Out = {}; Cursor = {}; return EWorldReadResult::Unavailable; }
                        Out.Node = Selected.Node; Out.Root = Selected.Root; Out.Owner = Selected.Owner;
                        Out.Presence = SelectedNode->Kind == EWorldValueKind::Null ? EWorldFieldPresence::Null : EWorldFieldPresence::Value;
                    }
                    else
                    {
                        Out.Presence = Out.SourcePresence == EWorldFieldPresence::Null
                            ? EWorldFieldPresence::Null : EWorldFieldPresence::Missing;
                        if (Out.Presence == EWorldFieldPresence::Missing) Out.Node = 0;
                    }
                }
                if (Kind == EWorldReadDomain::Nightclub && Index == 7 && Effective)
                    Out.bMergedInputs = static_cast<const FPlayerResult*>(Effective)->bDanceMerged;
                Cursor = {}; return EWorldReadResult::Ready;
            }
            if (!Storage->Budget->Spend(16, false)) return Pending(*Storage);
            Cursor.Key = Value->NextSibling; Cursor.Offset = 0; Cursor.Block = nullptr; Cursor.bComparing = false;
        }
        if (!Storage->Budget->Spend(sizeof(Out), false)) return Pending(*Storage);
        Out = *this; Out.Node = 0; Out.Kind = ChildDomain(Kind, Index); Out.Presence = EWorldFieldPresence::Missing;
        Out.SourceNode = 0; Out.SourceRoot = Root; Out.SourceOwner = Owner;
        Out.SourcePresence = EWorldFieldPresence::Missing; Out.bHasSource = true; Out.ChoiceRule = &Rule;
        Out.bMergedInputs = false; Out.bDefaultBase = Kind == EWorldReadDomain::Appearance && Index == 1;
        Out.bJsonDocument = false;
        if (Out.bDefaultBase) Out.Presence = EWorldFieldPresence::Value;
        if (Effective && Kind == EWorldReadDomain::Player && Index == 19)
        {
            const auto Selected = static_cast<const FPlayerResult*>(Effective)->Weapon.Ref;
            if (Selected.IsSet())
            {
                const auto* SelectedNode = Resolve(*Storage, Selected);
                if (!SelectedNode) { Out = {}; Cursor = {}; return EWorldReadResult::Unavailable; }
                Out.Node = Selected.Node; Out.Root = Selected.Root; Out.Owner = Selected.Owner;
                Out.Presence = SelectedNode->Kind == EWorldValueKind::Null ? EWorldFieldPresence::Null : EWorldFieldPresence::Value;
            }
        }
        Cursor = {}; return EWorldReadResult::Ready;
    }
    EWorldReadResult FWorldValueRead::Next(FWorldSequenceCursor& Cursor, FWorldValueRead& Out, bool& bEnd) const
    {
        Out = {}; bEnd = false;
        const auto Admission = Observation.AdmitRead();
        if (Admission != EWorldReadResult::Ready)
        { if (Admission == EWorldReadResult::Unavailable) Cursor = {}; return Admission; }
        const auto Access = Observation.Access.Pin(); const auto Storage = Access->Storage.Pin();
        if (!Storage->Budget->Spend(sizeof(Out) + sizeof(Cursor), false)) return Pending(*Storage);
        const auto* Array = Resolve(*Storage, Ref(Owner, Root, Node));
        if (!Array || (Array->Kind != EWorldValueKind::Array && Array->Kind != EWorldValueKind::Object))
            return EWorldReadResult::Unavailable;
        if (!Cursor.bStarted)
        { Cursor.Token = Observation.Token(); Cursor.Record = Node; Cursor.Owner = Owner; Cursor.Next = Array->FirstChild; Cursor.bStarted = true; }
        if (Cursor.Token != Observation.Token() || Cursor.Record != Node || Cursor.Owner != Owner) return EWorldReadResult::Unavailable;
        if (bMergedInputs)
        {
            const auto* Player = static_cast<const FPlayerResult*>(Effective);
            if (Cursor.Index >= Player->DanceCount) { bEnd = true; return EWorldReadResult::Ready; }
            const auto Selected = Player->DanceInputs[Cursor.Index++].Ref;
            Out = *this; Out.Node = Selected.Node; Out.Root = Selected.Root; Out.Owner = Selected.Owner;
        }
        else
        {
            if (!Cursor.Next) { bEnd = true; return EWorldReadResult::Ready; }
            const auto* Value = Resolve(*Storage, Ref(Owner, Root, Cursor.Next));
            if (!Value) return EWorldReadResult::Unavailable;
            Out = *this; Out.Node = Value->Id; Cursor.Next = Value->NextSibling; ++Cursor.Index;
        }
        const auto* Element = Resolve(*Storage, Ref(Out.Owner, Out.Root, Out.Node));
        if (!Element) { Out = {}; return EWorldReadResult::Unavailable; }
        Out.Kind = ElementDomain(Kind);
        Out.Presence = Element->Kind == EWorldValueKind::Null ? EWorldFieldPresence::Null : EWorldFieldPresence::Value;
        Out.bMergedInputs = false; Out.bDefaultBase = false; Out.bJsonDocument = false;
        Out.bHasSource = false; Out.ChoiceRule = nullptr;
        return EWorldReadResult::Ready;
    }
    EWorldReadResult FWorldValueRead::ReadString(FWorldStringCursor& Cursor, uint16 (&Out)[256], int32& OutCount) const
    {
        OutCount = 0;
        const auto Admission = Observation.AdmitRead();
        if (Admission != EWorldReadResult::Ready)
        { if (Admission == EWorldReadResult::Unavailable) Cursor = {}; return Admission; }
        const auto Access = Observation.Access.Pin(); const auto Storage = Access->Storage.Pin();
        if (bDefaultBase)
        {
            if (!Storage->Budget->Spend(32, false)) return Pending(*Storage);
            if (!Cursor.Generation)
            {
                Cursor.Generation = Observation.Generation(); Cursor.Publication = Observation.PublicationSerial();
                Cursor.Node = Node; Cursor.Root = Root; Cursor.Owner = Owner;
            }
            if (Cursor.Generation != Observation.Generation() || Cursor.Publication != Observation.PublicationSerial()
                || Cursor.Node != Node || Cursor.Root != Root || Cursor.Owner != Owner) return EWorldReadResult::Unavailable;
            if (Cursor.Offset == 6) return EWorldReadResult::Ready;
            if (Cursor.Offset != 0) return EWorldReadResult::Unavailable;
            static constexpr TCHAR Default[] = TEXT("bestie");
            for (uint32 I = 0; I < 6; ++I) Out[I] = static_cast<uint16>(Default[I]);
            Cursor.Offset = 6; OutCount = 6; return EWorldReadResult::Ready;
        }
        if (!Storage->Budget->Spend(16, false)) return Pending(*Storage);
        const auto* Value = Resolve(*Storage, Ref(Owner, Root, Node));
        if (!Value || Value->Kind != EWorldValueKind::String) return EWorldReadResult::Unavailable;
        if (!Cursor.Generation)
        { Cursor.Generation = Observation.Generation(); Cursor.Publication = Observation.PublicationSerial(); Cursor.Node = Node; Cursor.Root = Root; Cursor.Owner = Owner; Cursor.Block = Value->Text; }
        if (Cursor.Generation != Observation.Generation() || Cursor.Publication != Observation.PublicationSerial()
            || Cursor.Node != Node || Cursor.Root != Root || Cursor.Owner != Owner) return EWorldReadResult::Unavailable;
        const auto* Block = static_cast<const FStringBlock*>(Cursor.Block);
        if (!Block) return Cursor.Offset == Value->Count ? EWorldReadResult::Ready : EWorldReadResult::Unavailable;
        if (!Storage->Budget->Spend(2 * Block->Used + 16, false)) return Pending(*Storage);
        for (uint16 I = 0; I < Block->Used; ++I) Out[I] = Block->Units[I];
        OutCount = Block->Used; Cursor.Offset += Block->Used; Cursor.Block = Block->Next; return EWorldReadResult::Ready;
    }
    EWorldReadResult FWorldPlayerRead::Header(FWorldPlayerHeader& Out) const
    {
        Out = {};
        const auto Admission = Record.Observation.AdmitRead();
        if (Admission != EWorldReadResult::Ready) return Admission;
        const auto Access = Record.Observation.Access.Pin(); const auto Storage = Access->Storage.Pin();
        if (!Storage->Publication || !Record.Effective) return EWorldReadResult::Unavailable;
        if (!Storage->Budget->Spend(sizeof(Out), false)) return Pending(*Storage);
        Out = static_cast<const FPlayerResult*>(Record.Effective)->Header; return EWorldReadResult::Ready;
    }
    EWorldReadResult FWorldInstanceRead::Header(FWorldInstanceHeader& Out) const
    {
        Out = {};
        const auto Admission = Record.Observation.AdmitRead();
        if (Admission != EWorldReadResult::Ready) return Admission;
        const auto Access = Record.Observation.Access.Pin(); const auto Storage = Access->Storage.Pin();
        if (!Storage->Publication) return EWorldReadResult::Unavailable;
        if (!Storage->Budget->Spend(sizeof(Out), false)) return Pending(*Storage);
        Out = Storage->Publication->Instance; return EWorldReadResult::Ready;
    }
    EWorldReadResult FWorldInstanceRead::HomeRoomId(FWorldValueRead& Out) const
    {
        Out = {};
        const auto Admission = Record.Observation.AdmitRead();
        if (Admission != EWorldReadResult::Ready) return Admission;
        const auto Access = Record.Observation.Access.Pin(); const auto Storage = Access->Storage.Pin();
        if (!Storage->Publication) return EWorldReadResult::Unavailable;
        if (!Storage->Budget->Spend(sizeof(Out) + 16, false)) return Pending(*Storage);
        const auto Value = Storage->Publication->HomeRoom;
        Out.Observation = Record.Observation; Out.Owner = Value.Owner; Out.Root = Value.Root; Out.Node = Value.Node;
        Out.Kind = EWorldReadDomain::Raw;
        Out.Presence = Value.IsSet() ? EWorldFieldPresence::Value : EWorldFieldPresence::Missing;
        return EWorldReadResult::Ready;
    }
    EWorldReadResult FWorldObservation::AdmitNativeContentObservation() const
    {
        const auto Admission = AdmitRead();
        if (Admission != EWorldReadResult::Ready) return Admission;
        if (!QualifiedNativeContentObservationWork
            || QualifiedNativeContentObservationWork > TickWorkLimit) return EWorldReadResult::Unavailable;
        const auto Current = Access.Pin(); const auto Storage = Current->Storage.Pin();
        if (!Storage->Budget->Spend(QualifiedNativeContentObservationWork, false)) return Pending(*Storage);
        return EWorldReadResult::Ready;
    }
    EWorldReadResult FWorldObservation::BorrowHome(FHomeBorrow& Out) const
    {
        if (Out.Storage) return EWorldReadResult::Unavailable;
        const auto Admission = AdmitRead();
        if (Admission != EWorldReadResult::Ready) return Admission;
        const auto Current = Access.Pin(); const auto Storage = Current->Storage.Pin();
        if (!Storage->Publication || !Storage->Publication->bComplete || !Storage->Publication->bHomeMapped
            || Storage->ActiveBorrows == MAX_uint32) return EWorldReadResult::Unavailable;
        // Admission covers acquisition, the fixed unwind, and its final strong
        // release. Storage retirement waits for this count before destruction.
        if (!QualifiedHomeBorrowWork || QualifiedHomeBorrowWork > TickWorkLimit)
            return EWorldReadResult::Unavailable;
        if (!Storage->Budget->Spend(QualifiedHomeBorrowWork, false)) return Pending(*Storage);
        ++Storage->ActiveBorrows; Out.Storage = Storage; Out.Data = &Storage->Publication->Home;
        return EWorldReadResult::Ready;
    }
    EWorldReadResult FWorldHomeProjectionRead::Outcome(bool& bMapped) const
    {
        bMapped = false;
        const auto Admission = Observation.AdmitRead();
        if (Admission != EWorldReadResult::Ready) return Admission;
        const auto Access = Observation.Access.Pin(); const auto Storage = Access->Storage.Pin();
        if (!Storage->Publication || !Storage->Publication->bHomeMappingFinished) return EWorldReadResult::Unavailable;
        if (!Storage->Budget->Spend(32, false)) return Pending(*Storage);
        bMapped = Storage->Publication->bHomeMapped;
        return EWorldReadResult::Ready;
    }
    EWorldReadResult FWorldHomeProjectionRead::NextIssue(FWorldHomeIssueCursor& Cursor,
        FWorldHomeIssue& Out, bool& bEnd) const
    {
        Out = {}; bEnd = false;
        const auto Admission = Observation.AdmitRead();
        if (Admission != EWorldReadResult::Ready)
        { if (Admission == EWorldReadResult::Unavailable) Cursor = {}; return Admission; }
        const auto Access = Observation.Access.Pin(); const auto Storage = Access->Storage.Pin();
        if (!Storage->Publication || !Storage->Publication->bHomeMappingFinished) return EWorldReadResult::Unavailable;
        if (!Storage->Budget->Spend(sizeof(Out) + 32, false)) return Pending(*Storage);
        const auto& Projection = *Storage->Publication;
        if (!Cursor.bStarted) { Cursor.Token = Observation.Token(); Cursor.bStarted = true; }
        if (Cursor.Token != Observation.Token()) return EWorldReadResult::Unavailable;
        while (Cursor.Range < Projection.HomeUnknown.Count)
        {
            const auto& Range = Projection.HomeUnknown.Values[Cursor.Range];
            if (Range.BeforeIssue > Cursor.Known) break;
            if (!Cursor.bRangeStarted)
            {
                if (!Storage->Budget->Spend(32, false)) return Pending(*Storage);
                const auto* Object = Resolve(*Storage, Range.Object);
                if (!Object || Object->Kind != EWorldValueKind::Object) return EWorldReadResult::Unavailable;
                Cursor.Key = Object->FirstChild; Cursor.bRangeStarted = true;
            }
            while (Cursor.Key)
            {
                if (!Storage->Budget->Spend(16, false)) return Pending(*Storage);
                const auto* Key = Resolve(*Storage, Range.Object.Child(Cursor.Key));
                if (!Key || Key->Kind != EWorldValueKind::String || Key->Role != 1) return EWorldReadResult::Unavailable;
                bool bKnown = false;
                while (Cursor.Name < Range.Count)
                {
                    const auto* Name = Range.Schema ? Range.Schema->Fields[Cursor.Name].Name : Range.Names[Cursor.Name];
                    if (!Cursor.bComparing)
                    {
                        if (!Storage->Budget->Spend(128, false)) return Pending(*Storage);
                        Cursor.Offset = 0; Cursor.Block = Key->Text;
                        Cursor.bEqual = Key->Count == static_cast<uint64>(FCString::Strlen(Name));
                        Cursor.bComparing = true;
                    }
                    while (Cursor.bEqual && Cursor.Block)
                    {
                        const auto* Block = static_cast<const FStringBlock*>(Cursor.Block);
                        if (!Storage->Budget->Spend(2 * Block->Used + 16, false)) return Pending(*Storage);
                        for (uint16 I = 0; I < Block->Used; ++I)
                        {
                            if (Block->Units[I] != static_cast<uint16>(Name[Cursor.Offset])) Cursor.bEqual = false;
                            ++Cursor.Offset;
                        }
                        Cursor.Block = Block->Next;
                    }
                    if (Cursor.bEqual) { bKnown = true; break; }
                    if (!Storage->Budget->Spend(16, false)) return Pending(*Storage);
                    ++Cursor.Name; Cursor.bComparing = false;
                }
                if (!Storage->Budget->Spend(sizeof(Out) + 32, false)) return Pending(*Storage);
                const auto* Value = Resolve(*Storage, Range.Object.Child(Key->NextSibling));
                if (!Value) return EWorldReadResult::Unavailable;
                Cursor.Key = Value->NextSibling; Cursor.Name = 0; Cursor.Offset = 0;
                Cursor.Block = nullptr; Cursor.bComparing = false;
                if (!bKnown)
                {
                    Out.Code = EHomeIssue::UnknownField;
                    for (uint32 I = 0; I < UE_ARRAY_COUNT(Out.Path); ++I) Out.Path[I] = Range.Path[I];
                    return EWorldReadResult::Ready;
                }
            }
            if (!Storage->Budget->Spend(16, false)) return Pending(*Storage);
            ++Cursor.Range; Cursor.bRangeStarted = false;
        }
        if (Cursor.Known < static_cast<uint32>(Projection.Home.Issues.Num()))
        {
            const auto& Issue = Projection.Home.Issues[Cursor.Known];
            if (Issue.Field.Len() >= 96) return EWorldReadResult::Unavailable;
            if (!Storage->Budget->Spend(2 * static_cast<uint64>(Issue.Field.Len()) + 32, false)) return Pending(*Storage);
            Out.Code = Issue.Code;
            for (int32 I = 0; I < Issue.Field.Len(); ++I) Out.Path[I] = Issue.Field[I];
            ++Cursor.Known; return EWorldReadResult::Ready;
        }
        bEnd = true; return EWorldReadResult::Ready;
    }
    EWorldReadResult FWorldObservation::ReadMovementWindow(double FromMs, double ToMs,
        FMovementPose (&Out)[34], uint32& Count) const
    {
        Count = 0;
        const auto Admission = AdmitRead();
        if (Admission != EWorldReadResult::Ready) return Admission;
        if (!QualifiedMovementWindowBytes || !QualifiedMovementWindowWork)
            return EWorldReadResult::Unavailable;
        const auto Current = Access.Pin(); const auto Storage = Current->Storage.Pin();
        if (!Storage->Publication || !Storage->Publication->Movement.IsSet()) return EWorldReadResult::Unavailable;
        if (QualifiedMovementWindowWork > TickWorkLimit) return EWorldReadResult::Unavailable;
        if (!Storage->Budget->Spend(QualifiedMovementWindowWork, false)) return Pending(*Storage);
        if (!Storage->Budget->Reserve(QualifiedMovementWindowBytes)) return EWorldReadResult::Unavailable;
        bool bValid = false;
        {
            TArray<FMovementPose> Window;
            bValid = MovementWindow(Storage->Publication->Movement.GetValue(), FromMs, ToMs, Window)
                && Window.Num() <= 34;
            if (bValid)
            {
                Count = static_cast<uint32>(Window.Num());
                for (uint32 I = 0; I < Count; ++I) Out[I] = Window[I];
            }
        }
        Storage->Budget->Release(QualifiedMovementWindowBytes);
        if (!bValid || !IsCurrent()) { Count = 0; return EWorldReadResult::Unavailable; }
        return EWorldReadResult::Ready;
    }
}
