#pragma once

#include "CoreMinimal.h"
#include <variant>

namespace BaddieCompanion
{
    enum class EPlayEmote : uint8 { Greeting, Nod, Cheer, Stretch, Groove, Sway };
    enum class EPlayLook : uint8 { Rose, Ivory };
    enum class EPlayToy : uint8 { Ball, Cushion, Music, Bubbles };
    enum class EPlayStylePhase : uint8 { Dress, Reveal };

    struct FPlayLook
    {
        EPlayLook Look = EPlayLook::Rose;
        FString TopColor, TrousersColor, FootwearColor;
    };
    struct FPlayPlayer
    {
        FString Id, Name, CharacterId;
        // Complete saved object, including field absence. Object/resource checks only;
        // not a validated CompanionAppearance or a render plan. Never persisted here.
        FString AppearanceJson;
        double Progress = 0, Score = 0;
        TOptional<FPlayLook> Look; // Required nullable on the wire.
        TOptional<FString> Vote; // Required nullable on the wire.
        FString LastActionId;
        double LastActionAt = 0;
        TOptional<double> MirrorWatchingAt; // Optional, but null is invalid.
    };
    struct FPlayFlight
    {
        double From = 0, To = 0, Power = 0, At = 0;
        bool bHit = false;
    };
    struct FPlayLastTouch
    {
        double Target = 0, At = 0;
        FString PlayerId;
    };
    // Saved compatibility only. Active-round projection always excludes this branch.
    struct FPlayPlaygroundGame
    {
        EPlayToy Toy = EPlayToy::Ball;
        double Hits = 0, Throws = 0;
        TArray<double> Notes;
        TOptional<FPlayFlight> Flight; // Required nullable.
        TOptional<FPlayLastTouch> LastTouch; // Optional, but null is invalid.
    };
    struct FPlayLastMove
    {
        FString PlayerId;
        EPlayEmote Emote = EPlayEmote::Greeting;
        double At = 0;
    };
    struct FPlayMirrorGame
    {
        TArray<EPlayEmote> Sequence;
        double ShowingAt = 0, Level = 0;
        TOptional<FString> Leader; // Required nullable.
        TOptional<FPlayLastMove> LastMove; // Required nullable.
    };
    struct FPlayMysteryGame
    {
        TArray<double> Order, Found, Entered;
        bool bUnlocked = false;
    };
    struct FPlayStyleGame
    {
        double Brief = 0;
        EPlayStylePhase Phase = EPlayStylePhase::Dress;
        TOptional<double> RevealedAt; // Required nullable.
    };
    struct FPlayKitchenGame
    {
        TArray<double> Recipe, Ingredients;
        bool bChopped = false;
        TOptional<double> HeatAt; // Required nullable.
        bool bCooked = false, bPlated = false;
        double Served = 0;
        bool bRush = false;
    };
    struct FPlayMemoriesGame
    {
        double Anchor = 0;
        TArray<EPlayEmote> Moves;
        bool bRecording = false;
    };
    // The active alternative is the saved kind. No inactive branch data or unknown kind.
    using FPlayGame = std::variant<FPlayPlaygroundGame, FPlayMirrorGame, FPlayMysteryGame,
        FPlayStyleGame, FPlayKitchenGame, FPlayMemoriesGame>;
    struct FPlayRound
    {
        FString Id, OwnerId;
        // Endpoint numbers stay finite doubles, not command-only integer/range rules.
        double Revision = 0, Seed = 0, StartedAt = 0, UpdatedAt = 0, ExpiresAt = 0;
        TArray<FPlayPlayer> Players;
        FPlayGame Game;
    };
    struct FPlayMemory
    {
        FString Id, Author;
        int32 Anchor = 0; // validPlayMemory's integer index 0..5.
        TArray<EPlayEmote> Moves;
        double CreatedAt = 0;
    };

    struct FPlayWireResult;
    // Immutable decoded value. Getters return detached copies, never parsed JSON pointers.
    // No account, room, invitation or call authority is represented here.
    class BADDIECOMPANION_API FPlayWire
    {
    public:
        FPlayWire(const FPlayWire&) = default;
        FPlayWire(FPlayWire&&) = default;
        FPlayWire& operator=(const FPlayWire&) = default;
        FPlayWire& operator=(FPlayWire&&) = default;
        TOptional<FPlayRound> GetRound() const { return Round; }
        FString GetPlayerId() const { return PlayerId; }
        TArray<FPlayMemory> GetMemories() const { return Memories; }
        // activePlayRound only: no expiry, membership, command or render eligibility check.
        TOptional<FPlayRound> GetActiveRound() const;
    private:
        FPlayWire() = default;
        friend FPlayWireResult DecodePlayWire(const FString& Json);
        friend FPlayWireResult ReconcilePlayWire(const TOptional<FPlayWire>& Previous, const FString& Json);
        TOptional<FPlayRound> Round;
        FString PlayerId;
        TArray<FPlayMemory> Memories;
    };
    enum class EPlayWireDisposition : uint8 { Invalid, Accepted, Retained };
    struct FPlayWireResult
    {
        EPlayWireDisposition Disposition = EPlayWireDisposition::Invalid;
        // Unset on any failure. An accepted wire with a null round remains set.
        TOptional<FPlayWire> Wire;
        FString Error; // Fixed diagnostic only; empty on accepted/retained results.
    };
    // Full object envelope required. All source times remain milliseconds.
    BADDIECOMPANION_API FPlayWireResult DecodePlayWire(const FString& Json);
    // Validates ALL incoming fields before revision comparison. Pure, with no retained owner.
    // Same ID + older revision retains the whole prior wire. All other valid input replaces it.
    // Caller must clear Previous across account/scope changes and authenticate every callback.
    BADDIECOMPANION_API FPlayWireResult ReconcilePlayWire(
        const TOptional<FPlayWire>& Previous, const FString& Json);
}
