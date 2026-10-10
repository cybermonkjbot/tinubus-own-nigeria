#include "BaddieCompanionSocialState.h"
#include "BaddieCompanionStateJson.h"

#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include <cmath>
#include <initializer_list>

namespace BaddieCompanion
{
    namespace SocialStatePrivate
    {
        using StateJsonPrivate::FObject;
        using StateJsonPrivate::FValue;
        using StateJsonPrivate::Field;
        using StateJsonPrivate::Same;
        using StateJsonPrivate::Boolean;
        using StateJsonPrivate::Integer;
        constexpr StateJsonPrivate::FParseLimits ParseLimits{256 * 1024, 8, 4096};
        constexpr int32 MaxOffers = 12;
        constexpr int32 MaxPlayerWatches = 64;
        bool Identifier(const FString& S)
        {
            if (S.IsEmpty() || S.Len() > 240 || !Same(S, S.TrimStartAndEnd())) return false;
            for (const TCHAR C : S) if (C < 32 || C == 127) return false;
            return true;
        }
        bool Clock(double N) { return std::isfinite(N) && N >= 0 && N <= 8640000000000000.0 && std::floor(N) == N; }
        bool Parse(const FString& Json, FValue& Out)
        {
            if (Json.IsEmpty() || Json.Len() > ParseLimits.Characters) return false;
            const FString Trimmed = Json.TrimStartAndEnd();
            // UE requires a container root; validate nullable endpoint values in the helper.
            if (Same(Trimmed, TEXT("null")))
                return StateJsonPrivate::ParseNull(Json, Out, ParseLimits);
            if (!Trimmed.StartsWith(TEXT("[")))
            {
                FObject Root;
                if (!StateJsonPrivate::Parse(Json, Root, ParseLimits)) return false;
                Out = MakeShared<FJsonValueObject>(Root);
                return true;
            }
            // Offers use an array root; keep the same UE value parser and original input.
            return StateJsonPrivate::WithinParseLimits(Json, ParseLimits)
                && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Out) && Out.IsValid();
        }
        FObject Object(const FValue& V, int32 Fields)
        {
            const auto O = StateJsonPrivate::Object(V);
            return O.IsValid() && O->Values.Num() == Fields ? O : nullptr;
        }
        bool Text(const FValue& V, FString& Out)
        {
            return StateJsonPrivate::String(V, Out) && Out.Len() <= 8192;
        }
        bool Id(const FValue& V, FString& Out) { return Text(V, Out) && Identifier(Out); }
        bool Number(const FValue& V, double& Out)
        {
            return StateJsonPrivate::Number(V, Out) && StateJsonPrivate::ValidTime(Out);
        }
        template <typename T> bool Enum(const FValue& V, T& Out, std::initializer_list<TPair<const TCHAR*, T>> Choices)
        {
            FString Name;
            if (!Id(V, Name)) return false;
            for (const auto& Choice : Choices) if (Same(Name, Choice.Key)) { Out = Choice.Value; return true; }
            return false;
        }
        template <typename T, typename F> bool Nullable(const FValue& V, TSocialField<T>& Out, F Read)
        {
            TOptional<T> Next;
            if (!StateJsonPrivate::Nullable(V, Next, Read)) return false;
            if (!Next.IsSet()) { Out.Presence = ESocialFieldPresence::Null; return true; }
            Out.Value = MoveTemp(Next.GetValue());
            Out.Presence = ESocialFieldPresence::Value;
            return true;
        }
        bool MessageStatus(const FValue& V, ESocialMessageStatus& Out)
        {
            return Enum<ESocialMessageStatus>(V, Out, {{TEXT("accepted"), ESocialMessageStatus::Accepted},
                {TEXT("pending"), ESocialMessageStatus::Pending}, {TEXT("declined"), ESocialMessageStatus::Declined}});
        }
        bool Player(const FValue& V, FSocialPlayerAccess& Out)
        {
            const auto O = Object(V, 8);
            return Id(Field(O, TEXT("userId")), Out.UserId) && Text(Field(O, TEXT("handle")), Out.Handle)
                && Text(Field(O, TEXT("name")), Out.Name) && Nullable(Field(O, TEXT("threadId")), Out.ThreadId, Id)
                && Boolean(Field(O, TEXT("canMessage")), Out.bCanMessage)
                && Nullable(Field(O, TEXT("status")), Out.Status, MessageStatus)
                && Boolean(Field(O, TEXT("isMessageRequest")), Out.bIsMessageRequest)
                && Boolean(Field(O, TEXT("isPendingOutboundRequest")), Out.bIsPendingOutboundRequest);
        }
        bool Encounter(const FValue& V, FSocialEncounter& Out)
        {
            const auto O = Object(V, 7);
            return Id(Field(O, TEXT("id")), Out.Id) && Id(Field(O, TEXT("callSessionId")), Out.CallSessionId)
                && Id(Field(O, TEXT("peerUserId")), Out.PeerUserId) && Text(Field(O, TEXT("peerName")), Out.PeerName)
                && Text(Field(O, TEXT("place")), Out.Place) && Integer(Field(O, TEXT("roomRevision")), Out.RoomRevision)
                && Number(Field(O, TEXT("expiresAt")), Out.ExpiresAtMs);
        }
        bool Offers(const FValue& V, TArray<FSocialEncounter>& Out)
        {
            if (!V.IsValid() || V->Type != EJson::Array || V->AsArray().Num() > MaxOffers) return false;
            for (const auto& Entry : V->AsArray())
            {
                FSocialEncounter Next;
                if (!Encounter(Entry, Next)) return false;
                Out.Add(MoveTemp(Next));
            }
            return true;
        }
        bool Metric(const FValue& V, FSocialReputationMetric& Out)
        {
            const auto O = Object(V, 2);
            return Number(Field(O, TEXT("value")), Out.Value) && Boolean(Field(O, TEXT("exact")), Out.bExact);
        }
        bool Reputation(const FValue& V, FSocialReputation& Out)
        {
            const auto O = Object(V, 13);
            return Number(Field(O, TEXT("observedAt")), Out.ObservedAtMs)
                && Number(Field(O, TEXT("windowStartAt")), Out.WindowStartAtMs)
                && Number(Field(O, TEXT("windowEndAt")), Out.WindowEndAtMs)
                && Metric(Field(O, TEXT("followers")), Out.Followers)
                && Metric(Field(O, TEXT("following")), Out.Following)
                && Metric(Field(O, TEXT("receivedLikes")), Out.ReceivedLikes)
                && Metric(Field(O, TEXT("receivedComments")), Out.ReceivedComments)
                && Metric(Field(O, TEXT("givenLikes")), Out.GivenLikes)
                && Metric(Field(O, TEXT("givenComments")), Out.GivenComments)
                && Metric(Field(O, TEXT("publishedPosts")), Out.PublishedPosts)
                && Enum<ESocialReputationPolicy>(Field(O, TEXT("policy")), Out.Policy, {{TEXT("signals-only"), ESocialReputationPolicy::SignalsOnly}})
                && Enum<ESocialReputationRewards>(Field(O, TEXT("rewards")), Out.Rewards, {{TEXT("disabled"), ESocialReputationRewards::Disabled}})
                && Enum<ESocialReputationPrivileges>(Field(O, TEXT("privileges")), Out.Privileges, {{TEXT("unassigned"), ESocialReputationPrivileges::Unassigned}});
        }
        bool Context(const FValue& V, FSocialConversationContext& Out)
        {
            const auto O = Object(V, 2);
            return Text(Field(O, TEXT("place")), Out.Place) && Number(Field(O, TEXT("qualifiedAt")), Out.QualifiedAtMs);
        }
        template <typename T, typename F> bool Decode(const FString& Json, T& Out, FString& OutError, F Read)
        {
            FValue Root;
            T Next{};
            if (!Parse(Json, Root) || !Read(Root, Next))
            {
                Out = T{};
                OutError = TEXT("The social response is unavailable.");
                return false;
            }
            Out = MoveTemp(Next);
            OutError.Reset();
            return true;
        }
        void ExpireOffers(TArray<FSocialEncounter>& OffersValue, double NowMs)
        {
            // RemoveAll retains the canonical relative response order, unlike RemoveAllSwap.
            OffersValue.RemoveAll([NowMs](const FSocialEncounter& Offer) { return Offer.ExpiresAtMs <= NowMs; });
        }
    }

    bool DecodeSocialOffers(const FString& Json, TArray<FSocialEncounter>& Out, FString& OutError)
    {
        return SocialStatePrivate::Decode(Json, Out, OutError, SocialStatePrivate::Offers);
    }
    bool DecodeSocialPlayerAccess(const FString& Json, FSocialPlayerAccess& Out, FString& OutError)
    {
        return SocialStatePrivate::Decode(Json, Out, OutError, SocialStatePrivate::Player);
    }
    bool DecodeSocialReputation(const FString& Json, FSocialReputation& Out, FString& OutError)
    {
        return SocialStatePrivate::Decode(Json, Out, OutError, SocialStatePrivate::Reputation);
    }
    bool DecodeSocialConversationContext(const FString& Json, TSocialField<FSocialConversationContext>& Out, FString& OutError)
    {
        return SocialStatePrivate::Decode(Json, Out, OutError, [](const SocialStatePrivate::FValue& V,
            TSocialField<FSocialConversationContext>& Next) { return SocialStatePrivate::Nullable(V, Next, SocialStatePrivate::Context); });
    }

    bool FSocialState::SetIdentity(const FString& Scope)
    {
        if (bDisposed || (!Scope.IsEmpty() && !SocialStatePrivate::Identifier(Scope))) return false;
        if (SocialStatePrivate::Same(Scope, IdentityScope)) return true;
        const FString Next = Scope;
        Clear();
        IdentityScope = Next;
        if (!Next.IsEmpty()) Identity = MakeShared<uint8>(0);
        return true;
    }
    void FSocialState::Clear()
    {
        Identity.Reset();
        OffersGeneration.Reset();
        IdentityScope.Reset();
        SelectedPlace.Reset();
        Players.Reset();
        State = FSocialSnapshot{};
    }
    void FSocialState::Dispose() { Clear(); bDisposed = true; }
    bool FSocialState::Select(const FString& CallSessionId)
    {
        if (bDisposed || (!CallSessionId.IsEmpty() && !SocialStatePrivate::Identifier(CallSessionId))) return false;
        if (SocialStatePrivate::Same(CallSessionId, SelectedPlace)) return true;
        SelectedPlace = CallSessionId;
        StopOffersObservation();
        State = FSocialSnapshot{};
        return true;
    }
    bool FSocialState::BeginOffersObservation(FSocialOffersObservation& Out)
    {
        Out = FSocialOffersObservation{};
        StopOffersObservation();
        if (bDisposed || !Identity.IsValid() || SelectedPlace.IsEmpty()) return false;
        OffersGeneration = MakeShared<uint8>(0);
        Out.Identity = Identity;
        Out.Generation = OffersGeneration;
        Out.CallSessionId = SelectedPlace;
        return true;
    }
    void FSocialState::StopOffersObservation() { OffersGeneration.Reset(); }
    bool FSocialState::IsCurrent(const FSocialOffersObservation& Token) const
    {
        return !bDisposed && Identity.IsValid() && OffersGeneration.IsValid() && Token.Identity == Identity
            && Token.Generation == OffersGeneration && SocialStatePrivate::Same(Token.CallSessionId, SelectedPlace);
    }
    void FSocialState::FailOffers(ESocialStateError Error)
    {
        State = FSocialSnapshot{};
        State.bReady = true;
        State.Error = Error;
    }
    ESocialObservationResult FSocialState::ObserveOffers(const FSocialOffersObservation& Token, const FString& Json, double NowMs)
    {
        using namespace SocialStatePrivate;
        if (!IsCurrent(Token)) return ESocialObservationResult::Ignored;
        if (!Clock(NowMs)) { FailOffers(ESocialStateError::InvalidClock); return ESocialObservationResult::Invalid; }
        TArray<FSocialEncounter> Next;
        FString Error;
        if (!DecodeSocialOffers(Json, Next, Error)
            || Next.ContainsByPredicate([this](const FSocialEncounter& Offer) { return !Same(Offer.CallSessionId, SelectedPlace); }))
        {
            FailOffers(ESocialStateError::InvalidResponse);
            return ESocialObservationResult::Invalid;
        }
        ExpireOffers(Next, NowMs);
        State.Offers = MoveTemp(Next);
        State.bReady = true;
        State.Error = ESocialStateError::None;
        return ESocialObservationResult::Applied;
    }
    bool FSocialState::ObserveOffersFailure(const FSocialOffersObservation& Token)
    {
        if (!IsCurrent(Token)) return false;
        FailOffers(ESocialStateError::SubscriptionFailed);
        return true;
    }
    ESocialObservationResult FSocialState::ObserveClock(const FSocialOffersObservation& Token, double NowMs)
    {
        if (!IsCurrent(Token)) return ESocialObservationResult::Ignored;
        if (!SocialStatePrivate::Clock(NowMs)) { FailOffers(ESocialStateError::InvalidClock); return ESocialObservationResult::Invalid; }
        SocialStatePrivate::ExpireOffers(State.Offers, NowMs);
        return ESocialObservationResult::Applied;
    }
    bool FSocialState::BeginPlayerObservation(const FString& UserId, const FString& CallSessionId, FSocialPlayerObservation& Out)
    {
        Out = FSocialPlayerObservation{};
        if (bDisposed || !Identity.IsValid() || Players.Num() >= SocialStatePrivate::MaxPlayerWatches
            || !SocialStatePrivate::Identifier(UserId) || !SocialStatePrivate::Identifier(CallSessionId)) return false;
        FPlayerWatch Next;
        Next.Key = MakeShared<uint8>(0);
        Next.UserId = UserId;
        Next.CallSessionId = CallSessionId;
        Out.Identity = Identity;
        Out.Watch = Next.Key;
        Players.Add(MoveTemp(Next));
        return true;
    }
    int32 FSocialState::FindPlayer(const FSocialPlayerObservation& Token) const
    {
        if (bDisposed || !Identity.IsValid() || Token.Identity != Identity || !Token.Watch.IsValid()) return INDEX_NONE;
        return Players.IndexOfByPredicate([&Token](const FPlayerWatch& Watch) { return Watch.Key == Token.Watch; });
    }
    void FSocialState::StopPlayerObservation(const FSocialPlayerObservation& Token)
    {
        const int32 Index = FindPlayer(Token);
        if (Index != INDEX_NONE) Players.RemoveAt(Index);
    }
    ESocialObservationResult FSocialState::ObservePlayer(const FSocialPlayerObservation& Token, const FString& Json)
    {
        const int32 Index = FindPlayer(Token);
        if (Index == INDEX_NONE) return ESocialObservationResult::Ignored;
        auto& Watch = Players[Index];
        FSocialPlayerAccess Next;
        FString Error;
        if (!DecodeSocialPlayerAccess(Json, Next, Error) || !SocialStatePrivate::Same(Next.UserId, Watch.UserId))
        {
            Watch.Snapshot = FSocialPlayerSnapshot{};
            Watch.Snapshot.bReady = true;
            Watch.Snapshot.Error = ESocialStateError::InvalidResponse;
            return ESocialObservationResult::Invalid;
        }
        Watch.Snapshot = FSocialPlayerSnapshot{};
        Watch.Snapshot.bReady = true;
        Watch.Snapshot.Access.Presence = ESocialFieldPresence::Value;
        Watch.Snapshot.Access.Value = MoveTemp(Next);
        return ESocialObservationResult::Applied;
    }
    bool FSocialState::ObservePlayerFailure(const FSocialPlayerObservation& Token)
    {
        const int32 Index = FindPlayer(Token);
        if (Index == INDEX_NONE) return false;
        Players[Index].Snapshot = FSocialPlayerSnapshot{};
        Players[Index].Snapshot.bReady = true;
        Players[Index].Snapshot.Error = ESocialStateError::SubscriptionFailed;
        return true;
    }
    bool FSocialState::GetPlayerSnapshot(const FSocialPlayerObservation& Token, FSocialPlayerSnapshot& Out) const
    {
        const int32 Index = FindPlayer(Token);
        Out = Index == INDEX_NONE ? FSocialPlayerSnapshot{} : Players[Index].Snapshot;
        return Index != INDEX_NONE;
    }

    namespace SocialCommandPrivate
    {
        constexpr int32 MaxArgumentsCharacters = 128 * 1024;
        constexpr int32 MaxOptionalTextCharacters = 8192;

        struct FPacket
        {
            ESocialCommandKind Kind = ESocialCommandKind::None;
            FString ArgumentsJson;
            FString IdentityScope;
        };
        struct FLifetime
        {
            TSharedPtr<uint8> Owner;
            TSharedPtr<uint8> Identity;
            FString IdentityScope;
            ESocialCommandKind Kind = ESocialCommandKind::None;
            ESocialCommandStage Stage = ESocialCommandStage::Prepared;
            TSharedPtr<const FPacket> PreparedPacket;

            void Retire(ESocialCommandStage TerminalStage)
            {
                Stage = TerminalStage;
                PreparedPacket.Reset();
                Owner.Reset();
                Identity.Reset();
                IdentityScope.Reset();
                Kind = ESocialCommandKind::None;
            }
        };

        bool InputInteger(double Number, double Minimum)
        {
            return std::isfinite(Number) && Number >= Minimum && Number <= StateJsonPrivate::MaxInteger
                && std::floor(Number) == Number;
        }
        bool OptionalText(const TOptional<FString>& Text)
        {
            return !Text.IsSet() || Text.GetValue().Len() <= MaxOptionalTextCharacters;
        }
        bool Encode(const TSharedRef<FJsonObject>& Arguments, FString& Out)
        {
            const auto Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Out);
            if (!FJsonSerializer::Serialize(Arguments, Writer) || Out.Len() > MaxArgumentsCharacters)
            {
                Out.Reset();
                return false;
            }
            return true;
        }
        bool EncounterResult(const StateJsonPrivate::FValue& V, FSocialEncounterResult& Out)
        {
            using namespace SocialStatePrivate;
            const auto O = Object(V, 3);
            return Id(Field(O, TEXT("encounterId")), Out.EncounterId)
                && StateJsonPrivate::EnumValue<ESocialEncounterStatus>(Field(O, TEXT("status")), Out.Status,
                    {{TEXT("offered"), ESocialEncounterStatus::Offered}, {TEXT("qualified"), ESocialEncounterStatus::Qualified},
                    {TEXT("declined"), ESocialEncounterStatus::Declined}})
                && Nullable(Field(O, TEXT("threadId")), Out.ThreadId, Id);
        }
        bool CreditTransferResult(const StateJsonPrivate::FValue& V, FSocialCreditTransferResult& Out)
        {
            using namespace SocialStatePrivate;
            const auto O = Object(V, 8);
            return Boolean(Field(O, TEXT("success")), Out.bSuccess)
                // These are returned v.number() values, not local inputs or calculated money.
                && StateJsonPrivate::Number(Field(O, TEXT("credits")), Out.Credits)
                && StateJsonPrivate::Number(Field(O, TEXT("kobo")), Out.Kobo)
                && Text(Field(O, TEXT("nairaLabel")), Out.NairaLabel)
                && Text(Field(O, TEXT("recipientHandle")), Out.RecipientHandle)
                && Text(Field(O, TEXT("recipientName")), Out.RecipientName)
                && Id(Field(O, TEXT("threadId")), Out.ThreadId) && Id(Field(O, TEXT("messageId")), Out.MessageId);
        }
    }

    bool DecodeSocialEncounterResult(const FString& Json, FSocialEncounterResult& Out, FString& OutError)
    {
        return SocialStatePrivate::Decode(Json, Out, OutError, SocialCommandPrivate::EncounterResult);
    }
    bool DecodeSocialCreditTransferResult(const FString& Json, FSocialCreditTransferResult& Out, FString& OutError)
    {
        return SocialStatePrivate::Decode(Json, Out, OutError, SocialCommandPrivate::CreditTransferResult);
    }

    ESocialCommandKind FSocialCommandPacket::GetKind() const
    {
        return Data.IsValid() ? Data->Kind : ESocialCommandKind::None;
    }
    FString FSocialCommandPacket::GetEndpoint() const
    {
        switch (GetKind())
        {
        case ESocialCommandKind::Offer: return TEXT("companionGameSocial:offer");
        case ESocialCommandKind::Respond: return TEXT("companionGameSocial:respond");
        case ESocialCommandKind::TransferCredits: return TEXT("companionGameSocial:transferCredits");
        default: return {};
        }
    }
    FString FSocialCommandPacket::GetArgumentsJson() const { return Data.IsValid() ? Data->ArgumentsJson : FString{}; }
    FString FSocialCommandPacket::GetIdentityScope() const { return Data.IsValid() ? Data->IdentityScope : FString{}; }
    bool FSocialCommandPacket::ShouldInvalidateQueryCache() const { return GetKind() == ESocialCommandKind::TransferCredits; }

    ESocialCommandStage FSocialCommandRequest::GetStage() const
    {
        return Lifetime.IsValid() ? Lifetime->Stage : ESocialCommandStage::Unavailable;
    }
    bool FSocialCommandRequest::GetPreparedPacket(FSocialCommandPacket& Out) const
    {
        Out = FSocialCommandPacket{};
        if (GetStage() != ESocialCommandStage::Prepared || !Lifetime->PreparedPacket.IsValid()) return false;
        Out.Data = Lifetime->PreparedPacket;
        return true;
    }
    bool FSocialCommandRequest::AbandonUnsent() const
    {
        if (GetStage() != ESocialCommandStage::Prepared) return false;
        Lifetime->Retire(ESocialCommandStage::Abandoned);
        return true;
    }
    bool FSocialCommandRequest::CanComplete(const FSocialCommandReceipt& Receipt) const
    {
        return GetStage() == ESocialCommandStage::Dispatched && Receipt.Lifetime == Lifetime
            && Lifetime->Owner.IsValid() && Lifetime->Identity.IsValid();
    }
    void FSocialCommandRequest::CaptureOrigin(FSocialCommandOutcome& Out) const
    {
        Out.Origin.Owner = Lifetime->Owner;
        Out.Origin.Identity = Lifetime->Identity;
        Out.Origin.IdentityScope = Lifetime->IdentityScope;
        Out.Command = Lifetime->Kind;
    }
    bool FSocialCommandRequest::Complete(const FSocialCommandReceipt& Receipt, const FString& Json, FSocialCommandOutcome& Out) const
    {
        if (!CanComplete(Receipt)) { Out = FSocialCommandOutcome{}; return false; }
        FSocialCommandOutcome Next;
        CaptureOrigin(Next);
        FString Error;
        if (Next.Command == ESocialCommandKind::TransferCredits)
        {
            FSocialCreditTransferResult Value;
            if (DecodeSocialCreditTransferResult(Json, Value, Error))
            {
                Next.Transfer = MoveTemp(Value);
                Next.Error = ESocialCommandFailure::None;
            }
            else Next.Error = ESocialCommandFailure::InvalidResponse;
        }
        else
        {
            FSocialEncounterResult Value;
            if (DecodeSocialEncounterResult(Json, Value, Error))
            {
                Next.Encounter = MoveTemp(Value);
                Next.Error = ESocialCommandFailure::None;
            }
            else Next.Error = ESocialCommandFailure::InvalidResponse;
        }
        // No state owner is consulted or mutated. Sent Promises outlive their original view.
        Lifetime->Retire(ESocialCommandStage::Finished);
        Out = MoveTemp(Next);
        return true;
    }
    bool FSocialCommandRequest::CompleteFailure(const FSocialCommandReceipt& Receipt, FSocialCommandOutcome& Out) const
    {
        if (!CanComplete(Receipt)) { Out = FSocialCommandOutcome{}; return false; }
        FSocialCommandOutcome Next;
        CaptureOrigin(Next);
        Next.Error = ESocialCommandFailure::RequestFailed;
        Lifetime->Retire(ESocialCommandStage::Finished);
        Out = MoveTemp(Next);
        return true;
    }
    bool FSocialCommandRequest::DiscardCompletion(const FSocialCommandReceipt& Receipt) const
    {
        if (!CanComplete(Receipt)) return false;
        Lifetime->Retire(ESocialCommandStage::Discarded);
        return true;
    }

    bool FSocialState::PrepareCommandPacket(ESocialCommandKind Kind, FString ArgumentsJson,
        FSocialCommandRequest& Out, ESocialCommandFailure& OutError) const
    {
        Out = FSocialCommandRequest{};
        OutError = ESocialCommandFailure::Unavailable;
        if (bDisposed || !Identity.IsValid()) return false;
        auto Packet = MakeShared<SocialCommandPrivate::FPacket>();
        Packet->Kind = Kind;
        Packet->ArgumentsJson = MoveTemp(ArgumentsJson);
        Packet->IdentityScope = IdentityScope;
        auto Lifetime = MakeShared<SocialCommandPrivate::FLifetime>();
        Lifetime->Owner = CommandOwner;
        Lifetime->Identity = Identity;
        Lifetime->IdentityScope = IdentityScope;
        Lifetime->Kind = Kind;
        Lifetime->PreparedPacket = Packet;
        Out.Lifetime = MoveTemp(Lifetime);
        OutError = ESocialCommandFailure::None;
        return true;
    }
    bool FSocialState::PrepareOffer(const FSocialOfferCommand& Command, FSocialCommandRequest& Out, ESocialCommandFailure& OutError) const
    {
        Out = FSocialCommandRequest{};
        OutError = ESocialCommandFailure::Unavailable;
        if (bDisposed || !Identity.IsValid()) return false;
        if (!SocialStatePrivate::Identifier(Command.UserId) || !SocialStatePrivate::Identifier(Command.CallSessionId)
            || !SocialCommandPrivate::InputInteger(Command.RoomRevision, -static_cast<double>(StateJsonPrivate::MaxInteger)))
        {
            OutError = ESocialCommandFailure::InvalidArguments;
            return false;
        }
        const auto Args = MakeShared<FJsonObject>();
        Args->SetStringField(TEXT("userId"), Command.UserId);
        Args->SetStringField(TEXT("callSessionId"), Command.CallSessionId);
        Args->SetNumberField(TEXT("roomRevision"), Command.RoomRevision);
        FString Json;
        if (!SocialCommandPrivate::Encode(Args, Json)) { OutError = ESocialCommandFailure::EncodingFailed; return false; }
        return PrepareCommandPacket(ESocialCommandKind::Offer, MoveTemp(Json), Out, OutError);
    }
    bool FSocialState::PrepareRespond(const FSocialRespondCommand& Command, FSocialCommandRequest& Out, ESocialCommandFailure& OutError) const
    {
        Out = FSocialCommandRequest{};
        OutError = ESocialCommandFailure::Unavailable;
        if (bDisposed || !Identity.IsValid()) return false;
        if (!SocialStatePrivate::Identifier(Command.EncounterId)) { OutError = ESocialCommandFailure::InvalidArguments; return false; }
        const auto Args = MakeShared<FJsonObject>();
        Args->SetStringField(TEXT("encounterId"), Command.EncounterId);
        Args->SetBoolField(TEXT("accept"), Command.bAccept);
        FString Json;
        if (!SocialCommandPrivate::Encode(Args, Json)) { OutError = ESocialCommandFailure::EncodingFailed; return false; }
        return PrepareCommandPacket(ESocialCommandKind::Respond, MoveTemp(Json), Out, OutError);
    }
    bool FSocialState::PrepareTransferCredits(const FSocialTransferCreditsCommand& Command,
        FSocialCommandRequest& Out, ESocialCommandFailure& OutError) const
    {
        Out = FSocialCommandRequest{};
        OutError = ESocialCommandFailure::Unavailable;
        if (bDisposed || !Identity.IsValid()) return false;
        if (!SocialStatePrivate::Identifier(Command.RecipientUserId) || !SocialCommandPrivate::InputInteger(Command.Credits, 1)
            || !SocialCommandPrivate::OptionalText(Command.ContextPlace) || !SocialCommandPrivate::OptionalText(Command.Note))
        {
            OutError = ESocialCommandFailure::InvalidArguments;
            return false;
        }
        const auto Args = MakeShared<FJsonObject>();
        Args->SetStringField(TEXT("recipientUserId"), Command.RecipientUserId);
        Args->SetNumberField(TEXT("credits"), Command.Credits);
        if (Command.ContextPlace.IsSet()) Args->SetStringField(TEXT("contextPlace"), Command.ContextPlace.GetValue());
        if (Command.Note.IsSet()) Args->SetStringField(TEXT("note"), Command.Note.GetValue());
        FString Json;
        if (!SocialCommandPrivate::Encode(Args, Json)) { OutError = ESocialCommandFailure::EncodingFailed; return false; }
        return PrepareCommandPacket(ESocialCommandKind::TransferCredits, MoveTemp(Json), Out, OutError);
    }
    bool FSocialState::MarkCommandDispatched(const FSocialCommandRequest& Request,
        FSocialCommandPacket& OutPacket, FSocialCommandReceipt& OutReceipt) const
    {
        OutPacket = FSocialCommandPacket{};
        OutReceipt = FSocialCommandReceipt{};
        const auto& Lifetime = Request.Lifetime;
        if (bDisposed || !Identity.IsValid() || !Lifetime.IsValid() || Lifetime->Owner != CommandOwner
            || Lifetime->Identity != Identity || Lifetime->Stage != ESocialCommandStage::Prepared
            || !Lifetime->PreparedPacket.IsValid()) return false;
        OutPacket.Data = Lifetime->PreparedPacket;
        OutReceipt.Lifetime = Lifetime;
        Lifetime->Stage = ESocialCommandStage::Dispatched;
        // The original caller's request/receipt now retain only bounded lifetime metadata.
        Lifetime->PreparedPacket.Reset();
        return true;
    }
    bool FSocialState::MatchesCommandOrigin(const FSocialCommandOrigin& Origin) const
    {
        return !bDisposed && Identity.IsValid() && Origin.Owner == CommandOwner && Origin.Identity == Identity;
    }
}
