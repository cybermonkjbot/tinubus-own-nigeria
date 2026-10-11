#pragma once

#include "BaddieCompanionSnapshot.h"

namespace BaddieCompanion
{
    struct FHomeInvitation
    {
        FString Id, Handle, RoomId, RoomName;
    };
    struct FHomeInvitations
    {
        TArray<FHomeInvitation> Received, Sent;
    };
    struct FHomeVisit
    {
        FRoomSelection Room;
        int64 Revision = 0;
        FString OwnerHandle;
    };
    // Exact endpoint values, not transport envelopes. Atomic immutable publication;
    // failure clears Out, including malformed/oversized input. No geometry or authority.
    BADDIECOMPANION_API bool DecodeHomeInvitations(const FString& Json, TSharedPtr<const FHomeInvitations>& Out);
    BADDIECOMPANION_API bool DecodeHomeVisit(const FString& Json, TSharedPtr<const FHomeVisit>& Out);

    enum class EHomesOperation : uint8 { Invitations, Room, SetInvitation, RevokeInvitation };
    enum class EHomesReadKind : uint8 { List, Visit };
    enum class EHomesError : uint8 { InvalidResponse, ListFailed, VisitFailed, InvitationFailed, RevokeFailed };
    BADDIECOMPANION_API const TCHAR* HomesErrorText(EHomesError Error);
    struct FHomesView
    {
        // Null is unavailable/not observed, never a confirmed empty success.
        TSharedPtr<const FHomeInvitations> Invitations;
        TSharedPtr<const FHomeVisit> Visiting;
        bool bLoadingList = false, bLoadingVisit = false, bBusy = false;
        TOptional<EHomesError> ListError, VisitError, CommandError;
    };
    class FHomesOwnerReceipt
    {
        friend class FHomesState;
        FGuid Generation;
    };
    class FHomesSubscription
    {
        friend class FHomesState;
        FGuid Owner, Generation;
        EHomesReadKind Kind = EHomesReadKind::List;
    };
    class FHomesIntent
    {
    public:
        // Captured description only, not currentness, permission or acknowledgement.
        bool IsValid() const { return Request.IsValid(); }
        EHomesOperation GetKind() const { return Kind; }
        const FString& GetOperation() const { return Operation; }
        const FString& GetArgumentsJson() const { return ArgumentsJson; }
    private:
        friend class FHomesState;
        FGuid Owner, Subscription, Request;
        EHomesOperation Kind = EHomesOperation::Invitations;
        FString Operation, ArgumentsJson;
    };
    enum class EHomesCompletion : uint8 { Accepted, Stale, InvalidResponse, Failed };

    // Single-thread caller-driven module. No subscriptions, requests, timers, credentials,
    // native permission, geometry, private apartment inventory or optimistic writes.
    // Existing account endpoints require the original non-TV account/session/device adapter;
    // Home/apartment/gameplay grants cannot dispatch these local descriptions.
    class BADDIECOMPANION_API FHomesState
    {
    public:
        FHomesState() = default;
        FHomesState(const FHomesState&) = delete;
        FHomesState& operator=(const FHomesState&) = delete;
        const FHomesView& GetView() const { return View; }
        // Every call replaces account/session lifetime, even for the same ID. Explicitly
        // starting a new owner is the only way to resume after Dispose.
        FHomesOwnerReceipt BeginOwner(const FString& UserId);
        FHomesSubscription BeginList(const FHomesOwnerReceipt& Owner);
        // Replaces/clears the old visit even for the same invitation. No list membership
        // prerequisite: the server room read decides current invitation access.
        FHomesSubscription BeginVisit(const FHomesOwnerReceipt& Owner, const FString& InvitationId);
        void LeaveVisit(const FHomesOwnerReceipt& Owner);
        // Stop before external unsubscribe. One stop cannot retire another subscription.
        void Stop(const FHomesSubscription& Subscription);
        // Last-observer removal/feature disablement clears both read lifetimes. Mutations
        // survive on their original account lifetime; caller owns actual abort/unsubscribe.
        void StopObserving(const FHomesOwnerReceipt& Owner);
        // Capture before request or ordered subscription ingress, before async processing.
        // New capture supersedes only the same read's previous receipt.
        bool Read(const FHomesSubscription& Subscription, FHomesIntent& Out);
        EHomesCompletion AcceptRead(const FHomesIntent& Intent, const FString& Json);
        EHomesCompletion AcceptReadFailure(const FHomesIntent& Intent);
        // One account mutation at a time. Disabled still uses setInvitation. No retries,
        // query invalidation or contact/entitlement/room-sharing authority is inferred.
        bool SetInvitation(const FHomesOwnerReceipt& Owner, const FString& Handle,
            const FString& RoomId, bool bEnabled, FHomesIntent& Out);
        bool RevokeInvitation(const FHomesOwnerReceipt& Owner, const FString& InvitationId, FHomesIntent& Out);
        // Exact null success settles only this command; lists/guest data await their reads.
        EHomesCompletion AcceptNullResult(const FHomesIntent& Intent, const FString& Json);
        EHomesCompletion AcceptCommandFailure(const FHomesIntent& Intent);
        bool Cancel(const FHomesIntent& Intent);
        void Dispose();
    private:
        bool CurrentOwner(const FHomesOwnerReceipt& Owner) const;
        bool CurrentSubscription(const FHomesSubscription& Subscription) const;
        bool CurrentRead(const FHomesIntent& Intent) const;
        bool CurrentCommand(const FHomesIntent& Intent) const;
        void StopList();
        void StopVisit();
        void FinishCommand();
        bool CaptureCommand(EHomesOperation Kind, const FString& First, const FString& Second,
            bool bEnabled, FHomesIntent& Out);
        FGuid OwnerGeneration, ListGeneration, VisitGeneration, ListRead, VisitRead;
        FString SelectedInvitation;
        TOptional<FHomesIntent> ActiveCommand;
        FHomesView View;
        bool bDisposed = false;
    };
}
