#pragma once

#include "CoreMinimal.h"

namespace BaddieCompanion
{
    enum class ESocialFieldPresence : uint8 { Missing, Null, Value };
    template <typename T> struct TSocialField
    {
        ESocialFieldPresence Presence = ESocialFieldPresence::Missing;
        T Value{};
        bool HasValue() const { return Presence == ESocialFieldPresence::Value; }
    };

    enum class ESocialMessageStatus : uint8 { Accepted, Pending, Declined };
    struct FSocialPlayerAccess
    {
        FString UserId;
        FString Handle;
        FString Name;
        TSocialField<FString> ThreadId;
        bool bCanMessage = false;
        TSocialField<ESocialMessageStatus> Status;
        bool bIsMessageRequest = false;
        bool bIsPendingOutboundRequest = false;
    };

    struct FSocialEncounter
    {
        FString Id;
        FString CallSessionId;
        FString PeerUserId;
        FString PeerName;
        FString Place;
        int64 RoomRevision = 0;
        double ExpiresAtMs = 0;
    };

    struct FSocialReputationMetric
    {
        double Value = 0;
        bool bExact = false;
    };
    enum class ESocialReputationPolicy : uint8 { SignalsOnly };
    enum class ESocialReputationRewards : uint8 { Disabled };
    enum class ESocialReputationPrivileges : uint8 { Unassigned };
    struct FSocialReputation
    {
        double ObservedAtMs = 0;
        double WindowStartAtMs = 0;
        double WindowEndAtMs = 0;
        FSocialReputationMetric Followers;
        FSocialReputationMetric Following;
        FSocialReputationMetric ReceivedLikes;
        FSocialReputationMetric ReceivedComments;
        FSocialReputationMetric GivenLikes;
        FSocialReputationMetric GivenComments;
        FSocialReputationMetric PublishedPosts;
        ESocialReputationPolicy Policy = ESocialReputationPolicy::SignalsOnly;
        ESocialReputationRewards Rewards = ESocialReputationRewards::Disabled;
        ESocialReputationPrivileges Privileges = ESocialReputationPrivileges::Unassigned;
    };
    struct FSocialConversationContext
    {
        FString Place;
        double QualifiedAtMs = 0;
    };

    // Endpoint values, not transport envelopes. Required nulls cannot be omitted.
    // All outputs clear on failure; decoding never establishes messaging or game permission.
    BADDIECOMPANION_API bool DecodeSocialOffers(const FString& Json, TArray<FSocialEncounter>& Out, FString& OutError);
    BADDIECOMPANION_API bool DecodeSocialPlayerAccess(const FString& Json, FSocialPlayerAccess& Out, FString& OutError);
    BADDIECOMPANION_API bool DecodeSocialReputation(const FString& Json, FSocialReputation& Out, FString& OutError);
    BADDIECOMPANION_API bool DecodeSocialConversationContext(const FString& Json,
        TSocialField<FSocialConversationContext>& Out, FString& OutError);

    enum class ESocialStateError : uint8 { None, InvalidResponse, InvalidClock, SubscriptionFailed };
    enum class ESocialObservationResult : uint8 { Applied, Ignored, Invalid };
    struct FSocialSnapshot
    {
        bool bReady = false;
        TArray<FSocialEncounter> Offers;
        ESocialStateError Error = ESocialStateError::None;
    };
    struct FSocialPlayerSnapshot
    {
        bool bReady = false;
        TSocialField<FSocialPlayerAccess> Access;
        ESocialStateError Error = ESocialStateError::None;
    };

    class FSocialState;
    struct FSocialOffersObservation
    {
    private:
        friend class FSocialState;
        TSharedPtr<uint8> Identity;
        TSharedPtr<uint8> Generation;
        FString CallSessionId;
    };
    struct FSocialPlayerObservation
    {
    private:
        friend class FSocialState;
        TSharedPtr<uint8> Identity;
        TSharedPtr<uint8> Watch;
    };

    // Game-thread value owner. No networking, credentials, delegates, timers or admission.
    // The external owner delivers callbacks in canonical subscription order.
    class BADDIECOMPANION_API FSocialState
    {
    public:
        FSocialState() = default;
        FSocialState(const FSocialState&) = delete;
        FSocialState& operator=(const FSocialState&) = delete;
        FSocialState(FSocialState&&) = delete;
        FSocialState& operator=(FSocialState&&) = delete;

        // A non-secret account/session-lifetime key. Clear on access loss even for the same user.
        bool SetIdentity(const FString& IdentityScope);
        void Clear();
        void Dispose();
        bool Select(const FString& CallSessionId);
        FString GetSelectedPlace() const { return SelectedPlace; }
        FSocialSnapshot GetSnapshot() const { return State; }

        // Begin also serves reload/resubscribe: retain data/error, replace the offers epoch.
        bool BeginOffersObservation(FSocialOffersObservation& Out);
        void StopOffersObservation();
        ESocialObservationResult ObserveOffers(const FSocialOffersObservation& Token, const FString& Json, double NowMs);
        bool ObserveOffersFailure(const FSocialOffersObservation& Token);
        ESocialObservationResult ObserveClock(const FSocialOffersObservation& Token, double NowMs);

        // Each watch is independent, including two watches for the same pair. Place selection,
        // offers reload and losing the last offers subscriber do not retire these watches.
        bool BeginPlayerObservation(const FString& UserId, const FString& CallSessionId, FSocialPlayerObservation& Out);
        void StopPlayerObservation(const FSocialPlayerObservation& Token);
        ESocialObservationResult ObservePlayer(const FSocialPlayerObservation& Token, const FString& Json);
        bool ObservePlayerFailure(const FSocialPlayerObservation& Token);
        bool GetPlayerSnapshot(const FSocialPlayerObservation& Token, FSocialPlayerSnapshot& Out) const;

    private:
        struct FPlayerWatch
        {
            TSharedPtr<uint8> Key;
            FString UserId;
            FString CallSessionId;
            FSocialPlayerSnapshot Snapshot;
        };
        bool IsCurrent(const FSocialOffersObservation& Token) const;
        int32 FindPlayer(const FSocialPlayerObservation& Token) const;
        void FailOffers(ESocialStateError Error);

        FString IdentityScope;
        FString SelectedPlace;
        TSharedPtr<uint8> Identity;
        TSharedPtr<uint8> OffersGeneration;
        FSocialSnapshot State;
        TArray<FPlayerWatch> Players;
        bool bDisposed = false;
    };
}
