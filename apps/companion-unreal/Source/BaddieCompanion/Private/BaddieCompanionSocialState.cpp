#include "BaddieCompanionSocialState.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include <cmath>
#include <initializer_list>

namespace BaddieCompanion
{
    namespace SocialStatePrivate
    {
        using FObject = TSharedPtr<FJsonObject>;
        using FValue = TSharedPtr<FJsonValue>;
        constexpr double MaxInteger = 9007199254740991.0;
        constexpr int32 MaxOffers = 12;
        constexpr int32 MaxPlayerWatches = 64;
        bool Same(const FString& A, const FString& B) { return A.Equals(B, ESearchCase::CaseSensitive); }
        bool Identifier(const FString& S)
        {
            if (S.IsEmpty() || S.Len() > 240 || !Same(S, S.TrimStartAndEnd())) return false;
            for (const TCHAR C : S) if (C < 32 || C == 127) return false;
            return true;
        }
        bool Clock(double N) { return std::isfinite(N) && N >= 0 && N <= 8640000000000000.0 && std::floor(N) == N; }
        bool Parse(const FString& Json, FValue& Out)
        {
            if (Json.IsEmpty() || Json.Len() > 256 * 1024) return false;
            bool bString = false, bEscape = false;
            int32 Depth = 0, Separators = 0;
            for (const TCHAR C : Json)
            {
                if (bString)
                {
                    if (bEscape) bEscape = false;
                    else if (C == TEXT('\\')) bEscape = true;
                    else if (C == TEXT('"')) bString = false;
                    continue;
                }
                if (C == TEXT('"')) bString = true;
                else if (C == TEXT('{') || C == TEXT('[')) { if (++Depth > 8) return false; }
                else if (C == TEXT('}') || C == TEXT(']')) { if (--Depth < 0) return false; }
                else if (C == TEXT(',') || C == TEXT(':')) { if (++Separators > 4096) return false; }
            }
            return !bString && Depth == 0 && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Out) && Out.IsValid();
        }
        FObject Object(const FValue& V, int32 Fields)
        {
            if (!V.IsValid() || V->Type != EJson::Object) return nullptr;
            const auto O = V->AsObject();
            return O.IsValid() && O->Values.Num() == Fields ? O : nullptr;
        }
        FValue Field(const FObject& O, const TCHAR* Key) { return O.IsValid() ? O->TryGetField(Key) : nullptr; }
        bool Text(const FValue& V, FString& Out)
        {
            return V.IsValid() && V->Type == EJson::String && V->TryGetString(Out) && Out.Len() <= 8192;
        }
        bool Id(const FValue& V, FString& Out) { return Text(V, Out) && Identifier(Out); }
        bool Number(const FValue& V, double& Out)
        {
            return V.IsValid() && V->Type == EJson::Number && V->TryGetNumber(Out)
                && std::isfinite(Out) && Out >= 0 && Out <= MaxInteger;
        }
        bool Integer(const FValue& V, int64& Out)
        {
            double N = 0;
            if (!Number(V, N) || std::floor(N) != N) return false;
            Out = static_cast<int64>(N);
            return true;
        }
        bool Boolean(const FValue& V, bool& Out)
        {
            return V.IsValid() && V->Type == EJson::Boolean && V->TryGetBool(Out);
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
            if (!V.IsValid()) return false;
            if (V->Type == EJson::Null) { Out.Presence = ESocialFieldPresence::Null; return true; }
            if (!Read(V, Out.Value)) return false;
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
}
