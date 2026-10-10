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

    enum class ESocialCommandKind : uint8 { None, Offer, Respond, TransferCredits };
    enum class ESocialEncounterStatus : uint8 { Unavailable, Offered, Qualified, Declined };
    struct FSocialEncounterResult
    {
        FString EncounterId;
        ESocialEncounterStatus Status = ESocialEncounterStatus::Unavailable;
        TSocialField<FString> ThreadId;
    };
    struct FSocialCreditTransferResult
    {
        bool bSuccess = false;
        double Credits = 0;
        double Kobo = 0;
        FString NairaLabel;
        FString RecipientHandle;
        FString RecipientName;
        FString ThreadId;
        FString MessageId;
    };

    BADDIECOMPANION_API bool DecodeSocialEncounterResult(const FString& Json,
        FSocialEncounterResult& Out, FString& OutError);
    BADDIECOMPANION_API bool DecodeSocialCreditTransferResult(const FString& Json,
        FSocialCreditTransferResult& Out, FString& OutError);

    struct FSocialOfferCommand
    {
        FString UserId;
        FString CallSessionId;
        double RoomRevision = 0;
    };
    struct FSocialRespondCommand
    {
        FString EncounterId;
        bool bAccept = false;
    };
    struct FSocialTransferCreditsCommand
    {
        FString RecipientUserId;
        double Credits = 0;
        // Missing and supplied empty strings differ. Neither is converted to null or trimmed.
        TOptional<FString> ContextPlace;
        TOptional<FString> Note;
    };

    namespace SocialCommandPrivate { struct FPacket; struct FLifetime; }
    class FSocialCommandRequest;
    enum class ESocialCommandStage : uint8 { Unavailable, Prepared, Dispatched, Finished, Abandoned, Discarded };
    enum class ESocialCommandFailure : uint8 { None, Unavailable, InvalidArguments, EncodingFailed, InvalidResponse, RequestFailed };

    class FSocialCommandOrigin
    {
    public:
        FString GetIdentityScope() const { return IdentityScope; }
    private:
        friend class FSocialState;
        friend class FSocialCommandRequest;
        TSharedPtr<uint8> Owner;
        TSharedPtr<uint8> Identity;
        FString IdentityScope;
    };
    struct FSocialCommandOutcome
    {
        FSocialCommandOrigin Origin;
        ESocialCommandKind Command = ESocialCommandKind::None;
        ESocialCommandFailure Error = ESocialCommandFailure::Unavailable;
        TOptional<FSocialEncounterResult> Encounter;
        TOptional<FSocialCreditTransferResult> Transfer;
    };

    // Immutable, bounded non-credential arguments. Getter copies cannot change a prepared call.
    // Policy getters describe the canonical future adapter, and execute nothing.
    class BADDIECOMPANION_API FSocialCommandPacket
    {
    public:
        bool IsValid() const { return Data.IsValid(); }
        ESocialCommandKind GetKind() const;
        FString GetEndpoint() const;
        FString GetArgumentsJson() const;
        FString GetIdentityScope() const;
        bool ShouldRetry() const { return false; }
        bool ShouldInvalidateQueryCache() const;
    private:
        friend class FSocialState;
        friend class FSocialCommandRequest;
        TSharedPtr<const SocialCommandPrivate::FPacket> Data;
    };
    class FSocialCommandReceipt
    {
    private:
        friend class FSocialState;
        friend class FSocialCommandRequest;
        TSharedPtr<SocialCommandPrivate::FLifetime> Lifetime;
    };

    // Game-thread-only one-shot return handle. Copies share the same terminal latch.
    // After dispatch this retains no packet, read owner, credentials, delegates or response.
    // Settlement can outlive FSocialState, but can only return to this request's caller.
    class BADDIECOMPANION_API FSocialCommandRequest
    {
    public:
        ESocialCommandStage GetStage() const;
        // Inspection before dispatch is not permission to send; MarkCommandDispatched is required.
        bool GetPreparedPacket(FSocialCommandPacket& Out) const;
        bool AbandonUnsent() const;
        // True means a terminal outcome was returned, including InvalidResponse/RequestFailed
        // or a valid transfer result with success:false. It never means money moved.
        bool Complete(const FSocialCommandReceipt& Receipt, const FString& Json, FSocialCommandOutcome& Out) const;
        bool CompleteFailure(const FSocialCommandReceipt& Receipt, FSocialCommandOutcome& Out) const;
        // Drops interest in a sent result, without cancelling, retrying or claiming no write.
        bool DiscardCompletion(const FSocialCommandReceipt& Receipt) const;
    private:
        friend class FSocialState;
        bool CanComplete(const FSocialCommandReceipt& Receipt) const;
        void CaptureOrigin(FSocialCommandOutcome& Out) const;
        TSharedPtr<SocialCommandPrivate::FLifetime> Lifetime;
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

        // Independent explicit intents. No busy latch, selected-place or listener prerequisite.
        bool PrepareOffer(const FSocialOfferCommand& Command, FSocialCommandRequest& Out, ESocialCommandFailure& OutError) const;
        bool PrepareRespond(const FSocialRespondCommand& Command, FSocialCommandRequest& Out, ESocialCommandFailure& OutError) const;
        bool PrepareTransferCredits(const FSocialTransferCreditsCommand& Command, FSocialCommandRequest& Out, ESocialCommandFailure& OutError) const;
        // Rechecks original owner and identity, then marks once. The separately admitted adapter
        // must send immediately after success without yielding or notifying. This sends nothing.
        bool MarkCommandDispatched(const FSocialCommandRequest& Request,
            FSocialCommandPacket& OutPacket, FSocialCommandReceipt& OutReceipt) const;
        // A presentation lifetime check only, never endpoint permission. Retired-origin results
        // still settle for their original caller, but must not be routed to a new identity's UI.
        bool MatchesCommandOrigin(const FSocialCommandOrigin& Origin) const;

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
        bool PrepareCommandPacket(ESocialCommandKind Kind, FString ArgumentsJson,
            FSocialCommandRequest& Out, ESocialCommandFailure& OutError) const;

        TSharedPtr<uint8> CommandOwner = MakeShared<uint8>(0);
        FString IdentityScope;
        FString SelectedPlace;
        TSharedPtr<uint8> Identity;
        TSharedPtr<uint8> OffersGeneration;
        FSocialSnapshot State;
        TArray<FPlayerWatch> Players;
        bool bDisposed = false;
    };
}
