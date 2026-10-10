#pragma once

#include "CoreMinimal.h"
#include "Misc/Guid.h"

namespace BaddieCompanion
{
    enum class EHaulPresence : uint8 { Missing, Null, Value };
    template <typename T> struct THaulField
    {
        EHaulPresence Presence = EHaulPresence::Missing;
        T Value{};
        bool HasValue() const { return Presence == EHaulPresence::Value; }
    };
    enum class EHaulStatus : uint8 { Carried, Banked, Forfeited };
    enum class EHaulSourceKind : uint8 { WorldLoot, Job, Delivery, Mission, Activity, PosWithdrawal, StreetTrade };
    enum class EHaulRewardKind : uint8 { Item, Money };
    enum class EHaulMoneyUnit : uint8 { GameValueUnit, RealCurrencyMinor };
    enum class EHaulPageStatus : uint8 { SplitRecommended, SplitRequired };

    struct FHaulSource
    {
        EHaulSourceKind Kind = EHaulSourceKind::WorldLoot;
        FString Key;
    };
    struct FHaulReward
    {
        EHaulRewardKind Kind = EHaulRewardKind::Item;
        FString ItemKey;
        int64 Quantity = 0;
        int64 Amount = 0;
        EHaulMoneyUnit Unit = EHaulMoneyUnit::GameValueUnit;
        FString UnitKey;
        FString Currency;
        FString FundingReference;
    };
    struct FHaulEntry
    {
        FString Id, OutingId;
        FHaulSource Source;
        FHaulReward Reward;
        EHaulStatus Status = EHaulStatus::Carried;
        double GrantedAtMs = 0;
        // Absent on an unchanged grant; explicit null is not valid on this endpoint.
        TOptional<int64> OriginalMoneyAmount;
    };
    struct FHaulOuting
    {
        FString Id, CharacterId, WorldId, InstanceId;
        EHaulStatus Status = EHaulStatus::Carried;
        int64 Revision = 0, EntryCount = 0;
        double DepartedAtMs = 0;
        TOptional<double> ResolvedAtMs;
    };
    struct FHaulSnapshot
    {
        // Null outing is valid. Missing outing is a decoding failure.
        TOptional<FHaulOuting> Outing;
        TArray<FHaulEntry> Entries;
        FString CashConversion = TEXT("disabled");
        FString Withdrawal = TEXT("unavailable");
        FString BankedTransfer = TEXT("disabled");
    };
    struct FHaulStoreSnapshot
    {
        FHaulSnapshot Wire;
        bool bReady = false;
        TOptional<FString> Error;
    };
    struct FHaulPage
    {
        TArray<FHaulEntry> Page;
        FString ContinueCursor;
        bool bIsDone = false;
        THaulField<FString> SplitCursor;
        THaulField<EHaulPageStatus> PageStatus;
    };
    struct FHaulBalance
    {
        // Canonical result?.bankedCredits ?? 0, with its fallback provenance retained.
        bool bNullResponse = false;
        THaulField<int64> BankedCredits;
        int64 GetCreditsOrZero() const { return BankedCredits.HasValue() ? BankedCredits.Value : 0; }
    };
    struct FHaulMoneyGroup
    {
        EHaulMoneyUnit Unit = EHaulMoneyUnit::GameValueUnit;
        // Currency for real minor units, unitKey for game value. Never converted.
        FString UnitKey;
        EHaulStatus Status = EHaulStatus::Carried;
        int64 Amount = 0;
        TArray<FString> SourceKeys, FundingReferences;
    };

    // Endpoint values, not transport envelopes. Atomic output and fixed diagnostics.
    BADDIECOMPANION_API bool DecodeHaulSnapshot(const FString& Json, FHaulSnapshot& Out, FString& OutError);
    BADDIECOMPANION_API bool DecodeHaulPage(const FString& Json, FHaulPage& Out, FString& OutError);
    BADDIECOMPANION_API bool DecodeHaulBalance(const FString& Json, FHaulBalance& Out, FString& OutError);
    BADDIECOMPANION_API bool HaulMoneyGroups(const TArray<FHaulEntry>& Entries,
        TArray<FHaulMoneyGroup>& Out, FString& OutError);

    // Opaque process-local scope. These values are neither credentials nor permissions.
    class FHaulObservationReceipt
    {
        friend class FHaulReadState;
        FGuid Generation;
        FString AccountId;
    };
    class FHaulCursor
    {
        friend class FHaulReadState;
        FHaulObservationReceipt Scope;
        FString Value;
    };
    class FHaulBankedReceipt
    {
        friend class FHaulReadState;
        FHaulObservationReceipt Scope;
        FGuid RequestId;
        TOptional<FString> Cursor;
        int32 Limit = 25;
    };
    class FHaulBalanceReceipt
    {
        friend class FHaulReadState;
        FHaulObservationReceipt Scope;
        FGuid RequestId;
    };
    struct FHaulBankedRead
    {
        FHaulPage Wire;
        FHaulCursor ContinueCursor;
        TOptional<FHaulCursor> SplitCursor;
    };
    enum class EHaulReadResult : uint8 { Accepted, StaleReceipt, InvalidResponse };

    // Single game-thread adapter. No networking, caches, timers, commands or authority.
    // The transport owns admission and listener counting, and marshals callbacks here.
    class BADDIECOMPANION_API FHaulReadState
    {
    public:
        FHaulReadState() = default;
        FHaulReadState(const FHaulReadState&) = delete;
        FHaulReadState& operator=(const FHaulReadState&) = delete;
        FHaulReadState(FHaulReadState&&) = delete;
        FHaulReadState& operator=(FHaulReadState&&) = delete;

        // Value copies contain no mutable JSON or aliases into retained state.
        FHaulStoreSnapshot GetSnapshot() const { return State; }
        FString GetAccountId() const { return AccountId; }
        bool SelectAccount(const FString& InAccountId);
        // Call for first listener; additional listeners share the same receipt.
        bool BeginObservation(FHaulObservationReceipt& Out);
        bool CaptureObservation(FHaulObservationReceipt& Out) const;
        // Last listener, reload and disposal clear data and retire every query/cursor.
        void EndObservation();
        void Reload();
        void Clear();
        void Dispose();
        EHaulReadResult Observe(const FHaulObservationReceipt& Receipt, const FString& Json);
        // Only a public, already-sanitized error message belongs here, never a raw response.
        bool ObserveFailure(const FHaulObservationReceipt& Receipt, const FString& PublicMessage);

        // No cursor starts at canonical null. Cursors from other accounts/generations fail.
        // Up to 64 pending reads; each accepted/failed response consumes its own receipt.
        bool CaptureBanked(FHaulBankedReceipt& Out, const TOptional<FHaulCursor>& Cursor = {}, int32 Limit = 25);
        bool EncodeBankedArguments(const FHaulBankedReceipt& Receipt, FString& OutJson) const;
        EHaulReadResult AcceptBanked(const FHaulBankedReceipt& Receipt, const FString& Json, FHaulBankedRead& Out);
        bool FailBanked(const FHaulBankedReceipt& Receipt);
        bool CaptureBalance(FHaulBalanceReceipt& Out);
        EHaulReadResult AcceptBalance(const FHaulBalanceReceipt& Receipt, const FString& Json, FHaulBalance& Out);
        bool FailBalance(const FHaulBalanceReceipt& Receipt);

    private:
        FHaulObservationReceipt Scope() const;
        bool Current(const FHaulObservationReceipt& Receipt) const;
        bool Current(const FHaulBankedReceipt& Receipt) const;
        bool Current(const FHaulBalanceReceipt& Receipt) const;
        void ResetGeneration();
        FHaulStoreSnapshot State;
        FString AccountId;
        FGuid Generation = FGuid::NewGuid();
        TSet<FGuid> PendingBanked, PendingBalance;
        bool bObserving = false, bDisposed = false;
    };
}
