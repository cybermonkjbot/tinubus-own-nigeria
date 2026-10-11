#include "BaddieCompanionGameplaySession.h"
#include "BaddieCompanionHttpResponse.h"
#include "BaddieCompanionWorldSession.h"
#include "BaddieCompanionSlicePlayerController.h"
#include "GameFramework/PlayerController.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformTime.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/CoreDelegates.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

#include <cmath>

namespace BaddieGameplaySessionPrivate
{
    using ERequest = BaddieCompanion::EGameplayRequest;
    using BaddieCompanionHttpPrivate::FBoundedResponse;
    using BaddieCompanionHttpPrivate::FBoundedResponsePtr;
    constexpr double PairingSeconds = 300;
    constexpr double GrantSeconds = 900;
    constexpr double PresenceSeconds = 180; // SPACE_GAME_JOIN_TTL_MS, ordinary renewal is 45s.
    constexpr int32 MaxResponseBytes = 32 * 1024;
    constexpr int32 MaxEntrySequence = 1000000000;
    constexpr double MaxSafeInteger = 9007199254740991.0;

    bool Same(const FString& A, const FString& B) { return A.Equals(B, ESearchCase::CaseSensitive); }

    bool Text(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key, int32 Max, FString& Out)
    {
        const auto Value = Object.IsValid() ? Object->TryGetField(Key) : nullptr;
        if (!Value.IsValid() || Value->Type != EJson::String || !Value->TryGetString(Out)
            || Out.IsEmpty() || Out.Len() > Max || !Same(Out, Out.TrimStartAndEnd())) return false;
        for (TCHAR Ch : Out) if (Ch < 32 || Ch == 127) return false;
        return true;
    }

    bool Number(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key, double& Out)
    {
        const auto Value = Object.IsValid() ? Object->TryGetField(Key) : nullptr;
        return Value.IsValid() && Value->Type == EJson::Number && Value->TryGetNumber(Out)
            && FMath::IsFinite(Out) && Out >= 0 && Out <= MaxSafeInteger && std::floor(Out) == Out;
    }

    bool Boolean(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key, bool& Out)
    {
        const auto Value = Object.IsValid() ? Object->TryGetField(Key) : nullptr;
        return Value.IsValid() && Value->Type == EJson::Boolean && Value->TryGetBool(Out);
    }

    bool Hex(const FString& Value, int32 Length)
    {
        if (Value.Len() != Length) return false;
        for (TCHAR Ch : Value)
            if (!((Ch >= TEXT('0') && Ch <= TEXT('9')) || (Ch >= TEXT('a') && Ch <= TEXT('f')))) return false;
        return true;
    }

    bool Scope(const TSharedPtr<FJsonObject>& Result)
    {
        FString Value;
        return Text(Result, TEXT("scope"), 32, Value) && Same(Value, TEXT("companion_gameplay"));
    }

    bool GrantToken(const TSharedPtr<FJsonObject>& Result, FString& Out)
    {
        return Text(Result, TEXT("gameToken"), 69, Out)
            && Out.StartsWith(TEXT("bcg1_"), ESearchCase::CaseSensitive) && Hex(Out.Mid(5), 64);
    }

    bool DeploymentOrigin(const FString& Origin)
    {
        if (!Origin.StartsWith(TEXT("https://"), ESearchCase::CaseSensitive)
            || !Origin.EndsWith(TEXT(".convex.cloud"), ESearchCase::CaseSensitive)) return false;
        const FString Host = Origin.Mid(8, Origin.Len() - 21);
        if (Host.IsEmpty() || Host.Len() > 63 || Host.StartsWith(TEXT("-")) || Host.EndsWith(TEXT("-"))) return false;
        for (TCHAR Ch : Host)
            if (!((Ch >= TEXT('a') && Ch <= TEXT('z')) || (Ch >= TEXT('0') && Ch <= TEXT('9')) || Ch == TEXT('-'))) return false;
        return true;
    }

    struct FDeadline
    {
        double Monotonic = 0;
        FDateTime Wall;
        mutable bool bExpired = false;
        bool IsSet() const { return Monotonic > 0; }
        double Remaining() const
        {
            if (!IsSet() || bExpired) return 0;
            const double Seconds = FMath::Min(Monotonic - FPlatformTime::Seconds(),
                (Wall - FDateTime::UtcNow()).GetTotalSeconds());
            if (!FMath::IsFinite(Seconds) || Seconds <= 0) { bExpired = true; return 0; }
            return Seconds;
        }
        bool Set(double Seconds, bool bShortenOnly = false)
        {
            if (IsSet())
            {
                const double Existing = Remaining();
                if (Existing <= 0) return false;
                if (bShortenOnly) Seconds = FMath::Min(Seconds, Existing);
            }
            if (!FMath::IsFinite(Seconds) || Seconds <= 0) return false;
            Monotonic = FPlatformTime::Seconds() + Seconds;
            Wall = FDateTime::UtcNow() + FTimespan::FromSeconds(Seconds);
            return true;
        }
    };

    bool Lifetime(const TSharedPtr<FJsonObject>& Result, const TCHAR* Key,
        double Maximum, double Elapsed, double& Seconds, double& ServerNow)
    {
        double Expires = 0;
        if (!Number(Result, Key, Expires) || !Number(Result, TEXT("serverNow"), ServerNow)
            || ServerNow <= 0 || Expires <= ServerNow || Expires - ServerNow > Maximum * 1000) return false;
        Seconds = (Expires - ServerNow) / 1000 - Elapsed;
        return FMath::IsFinite(Seconds) && Seconds > 0;
    }

    struct FCredentials
    {
        FString Origin, DeviceId, Proof, Token;
        TSharedRef<FJsonObject> Args(bool bGrant) const
        {
            const auto Result = MakeShared<FJsonObject>();
            Result->SetStringField(TEXT("deviceId"), DeviceId);
            Result->SetStringField(TEXT("deviceSecret"), Proof);
            if (bGrant) Result->SetStringField(TEXT("gameToken"), Token);
            return Result;
        }
    };

    const TCHAR* Path(ERequest Operation)
    {
        switch (Operation)
        {
        case ERequest::Begin: return TEXT("companionGameAccess:beginGameplay");
        case ERequest::Exchange: return TEXT("companionGameAccess:exchange");
        case ERequest::Enter: return TEXT("companionGameplay:enter");
        case ERequest::Heartbeat: return TEXT("companionGameplay:heartbeat");
        case ERequest::Leave: return TEXT("companionGameplay:leave");
        default: return TEXT("");
        }
    }

    FHttpRequestPtr MakeRequest(const FString& Origin,
        const TCHAR* Operation, const TSharedRef<FJsonObject>& Args, FBoundedResponsePtr& ResponseBody)
    {
        const auto Envelope = MakeShared<FJsonObject>();
        Envelope->SetStringField(TEXT("path"), Operation);
        Envelope->SetStringField(TEXT("format"), TEXT("convex_encoded_json"));
        Envelope->SetArrayField(TEXT("args"), { MakeShared<FJsonValueObject>(Args) });
        FString Body;
        FJsonSerializer::Serialize(Envelope, TJsonWriterFactory<>::Create(&Body));
        const auto Request = FHttpModule::Get().CreateRequest();
        Request->SetOption(HttpRequestOptions::EnableCookies, TEXT("false"));
        Request->SetURL(Origin + TEXT("/api/mutation"));
        Request->SetVerb(TEXT("POST"));
        Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
        Request->SetHeader(TEXT("Accept"), TEXT("application/json"));
        Request->SetHeader(TEXT("Cache-Control"), TEXT("no-store"));
        Request->SetTimeout(15.0f);
        Request->SetDelegateThreadPolicy(EHttpRequestDelegateThreadPolicy::CompleteOnGameThread);
        Request->SetContentAsString(Body);
        ResponseBody = FBoundedResponse::Bind(Request, MaxResponseBytes);
        if (!ResponseBody.IsValid()) return nullptr;
        return Request;
    }

    TSharedPtr<FJsonObject> Decode(const FHttpResponsePtr& Response, const FString& Origin,
        const FBoundedResponsePtr& ResponseBody)
    {
        FString Body;
        if (!Response.IsValid() || Response->GetResponseCode() != 200
            || !Same(Response->GetEffectiveURL(), Origin + TEXT("/api/mutation"))
            || !ResponseBody.IsValid() || !ResponseBody->ReadCompletedText(Body)) return nullptr;
        TSharedPtr<FJsonObject> Envelope;
        FString Status;
        const TSharedPtr<FJsonObject>* Value = nullptr;
        if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Body), Envelope)
            || !Text(Envelope, TEXT("status"), 16, Status) || !Same(Status, TEXT("success"))
            || !Envelope->TryGetObjectField(TEXT("value"), Value) || !Value || !Value->IsValid()) return nullptr;
        return *Value;
    }

    bool Retryable(const FHttpResponsePtr& Response, bool bSuccess)
    {
        if (!bSuccess || !Response.IsValid()) return true;
        const int32 Code = Response->GetResponseCode();
        return Code == 429 || Code == 500 || Code == 502 || Code == 503 || Code == 504;
    }

    // Bounded cleanup retains only the retired device proof/grant, never the session
    // UObject. Late completion cannot restore UI or touch a newer connection.
    void Cleanup(FCredentials Credentials, int32 Sequence, bool bDisconnect, int32 Attempt = 0, FDeadline Limit = {})
    {
        if (Credentials.Proof.IsEmpty() || !DeploymentOrigin(Credentials.Origin)) return;
        if (!Limit.IsSet()) Limit.Set(PairingSeconds);
        const double CleanupRemaining = Limit.Remaining();
        if (CleanupRemaining <= 0) return;
        const bool bGrant = !Credentials.Token.IsEmpty();
        const bool bLeave = bGrant && Sequence > 0;
        const auto Args = Credentials.Args(bGrant);
        if (bLeave) Args->SetNumberField(TEXT("entrySequence"), Sequence);
        FBoundedResponsePtr ResponseBody;
        const auto Request = MakeRequest(Credentials.Origin, bLeave ? TEXT("companionGameplay:leave")
            : bGrant ? TEXT("companionGameplay:disconnect") : TEXT("companionGameAccess:cancel"), Args, ResponseBody);
        if (!Request.IsValid()) return;
        Request->SetTimeout(static_cast<float>(FMath::Min(15.0, CleanupRemaining)));
        const auto Completed = MakeShared<bool>(false);
        Request->OnProcessRequestComplete().BindLambda(
            [Credentials, Sequence, bDisconnect, Attempt, bLeave, Limit, Completed, ResponseBody](FHttpRequestPtr, FHttpResponsePtr Response, bool bSuccess)
        {
            if (*Completed) return;
            *Completed = true;
            const auto Result = bSuccess ? Decode(Response, Credentials.Origin, ResponseBody) : nullptr;
            FString Status;
            double RetryMs = 0;
            const bool bLimited = Text(Result, TEXT("status"), 32, Status) && Same(Status, TEXT("slow_down"))
                && Number(Result, TEXT("retryAfterMs"), RetryMs) && RetryMs > 0 && RetryMs <= 300000;
            const double Delay = bLimited ? RetryMs / 1000 : 5;
            if (Attempt < 2 && Delay < Limit.Remaining() && !ResponseBody->HasRejectedData()
                && (bLimited || Retryable(Response, bSuccess)))
            {
                FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
                    [Credentials, Sequence, bDisconnect, Attempt, Limit](float)
                    { Cleanup(Credentials, Sequence, bDisconnect, Attempt + 1, Limit); return false; }),
                    static_cast<float>(Delay));
                return;
            }
            if (bLeave && bDisconnect) Cleanup(Credentials, 0, true, 0, Limit);
        });
        // Some backends complete synchronously on submission failure; dispatch once.
        if (!Request->ProcessRequest() && !*Completed)
            Request->OnProcessRequestComplete().ExecuteIfBound(Request, nullptr, false);
    }

    void CleanupLate(ERequest Operation, FCredentials Credentials, int32 Sequence,
        const TSharedPtr<FJsonObject>& Result)
    {
        FString Value;
        if (Operation == ERequest::Begin && Scope(Result)
            && Text(Result, TEXT("deviceSecret"), 64, Value) && Hex(Value, 64))
        {
            Credentials.Proof = MoveTemp(Value);
            Cleanup(MoveTemp(Credentials), 0, true);
        }
        else if (Operation == ERequest::Exchange && Scope(Result) && GrantToken(Result, Value))
        {
            Credentials.Token = MoveTemp(Value);
            Cleanup(MoveTemp(Credentials), 0, true);
        }
        else if (Operation == ERequest::Enter) Cleanup(MoveTemp(Credentials), Sequence, false);
    }

    struct FPresenceIdentity
    {
        FString UserId, CharacterId, HomeRoomId, RoomId, ThreadId, CallSessionId, ParticipantId, ExpressionSessionId;
        bool Read(const TSharedPtr<FJsonObject>& Result)
        {
            return Text(Result, TEXT("userId"), 128, UserId) && Text(Result, TEXT("characterId"), 80, CharacterId)
                && Text(Result, TEXT("homeRoomId"), 80, HomeRoomId) && Text(Result, TEXT("roomId"), 128, RoomId)
                && Text(Result, TEXT("threadId"), 128, ThreadId) && Text(Result, TEXT("callSessionId"), 128, CallSessionId)
                && Text(Result, TEXT("participantId"), 128, ParticipantId)
                && Text(Result, TEXT("expressionSessionId"), 128, ExpressionSessionId);
        }
    };
}

struct FBaddieCompanionGameplaySessionData
{
    using ERequest = BaddieCompanion::EGameplayRequest;
    BaddieGameplaySessionPrivate::FCredentials Credentials;
    BaddieGameplaySessionPrivate::FDeadline Pairing, Grant, Presence;
    BaddieGameplaySessionPrivate::FPresenceIdentity Identity;
    BaddieCompanion::FGameplayConnectionView View;
    BaddieCompanion::FWorldSession World;
    TWeakObjectPtr<APlayerController> WorldConsumer;
    FString GrantId, CrewThreadId, ApprovedUserId, ApprovedCharacterId, ApprovedHomeRoomId, LeaveMessage;
    FHttpRequestPtr Request;
    uint64 Generation = 0, WorldConsumerSerial = 0;
    int32 EntrySequence = 0;
    double NextAttempt = 0, BeginRetryAt = 0, LastPresenceServerNow = 0;
    ERequest NextOperation = ERequest::None;
    bool bEnabled = false, bEnding = false, bCoreActive = true, bForeground = true, bSlateActive = false, bEntryOpen = false;
};

using EGameplayState = BaddieCompanion::EGameplayConnectionState;
using EGameplayRequest = BaddieCompanion::EGameplayRequest;

UBaddieCompanionGameplaySession::UBaddieCompanionGameplaySession()
    : Data(MakePimpl<FBaddieCompanionGameplaySessionData>()) {}
UBaddieCompanionGameplaySession::~UBaddieCompanionGameplaySession() = default;

void UBaddieCompanionGameplaySession::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    // No config read and no automatic pairing, entry or reapproval.
    DeactivateHandle = FCoreDelegates::ApplicationWillDeactivateDelegate.AddUObject(this,
        &UBaddieCompanionGameplaySession::ApplicationDeactivated);
    ReactivateHandle = FCoreDelegates::ApplicationHasReactivatedDelegate.AddUObject(this,
        &UBaddieCompanionGameplaySession::ApplicationReactivated);
    BackgroundHandle = FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddUObject(this,
        &UBaddieCompanionGameplaySession::ApplicationBackgrounded);
    ForegroundHandle = FCoreDelegates::ApplicationHasEnteredForegroundDelegate.AddUObject(this,
        &UBaddieCompanionGameplaySession::ApplicationForegrounded);
    AttachSlateLifecycle();
    Ticker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this,
        &UBaddieCompanionGameplaySession::TickSession), 0.25f);
}

void UBaddieCompanionGameplaySession::Deinitialize()
{
    Data->bEnding = true;
    FTSTicker::RemoveTicker(Ticker);
    Ticker.Reset();
    FCoreDelegates::ApplicationWillDeactivateDelegate.Remove(DeactivateHandle);
    FCoreDelegates::ApplicationHasReactivatedDelegate.Remove(ReactivateHandle);
    FCoreDelegates::ApplicationWillEnterBackgroundDelegate.Remove(BackgroundHandle);
    FCoreDelegates::ApplicationHasEnteredForegroundDelegate.Remove(ForegroundHandle);
    if (FSlateApplication::IsInitialized())
        FSlateApplication::Get().OnApplicationActivationStateChanged().Remove(SlateActivationHandle);
    Data->bEnabled = false;
    Stop(EGameplayState::Disabled, TEXT(""));
    OnConnectionChanged.Clear();
    Super::Deinitialize();
}

void UBaddieCompanionGameplaySession::AttachSlateLifecycle()
{
    if (!SlateActivationHandle.IsValid() && FSlateApplication::IsInitialized())
    {
        Data->bSlateActive = FSlateApplication::Get().IsActive();
        SlateActivationHandle = FSlateApplication::Get().OnApplicationActivationStateChanged().AddUObject(
            this, &UBaddieCompanionGameplaySession::SlateActivationChanged);
    }
}

bool UBaddieCompanionGameplaySession::IsForeground() const
{
    return !Data->bEnding && Data->bCoreActive && Data->bForeground && Data->bSlateActive
        && FSlateApplication::IsInitialized() && FSlateApplication::Get().IsActive();
}

void UBaddieCompanionGameplaySession::ApplicationDeactivated() { Data->bCoreActive = false; FocusLost(); }
void UBaddieCompanionGameplaySession::ApplicationReactivated() { Data->bCoreActive = true; }
void UBaddieCompanionGameplaySession::ApplicationBackgrounded() { Data->bForeground = false; FocusLost(); }
void UBaddieCompanionGameplaySession::ApplicationForegrounded() { Data->bForeground = true; }
void UBaddieCompanionGameplaySession::SlateActivationChanged(bool bActive)
{
    Data->bSlateActive = bActive;
    if (!bActive) FocusLost();
}

void UBaddieCompanionGameplaySession::FocusLost()
{
    if (!Data->Credentials.DeviceId.IsEmpty())
        Stop(Data->bEnabled ? EGameplayState::SignedOut : EGameplayState::Disabled,
            TEXT("Gameplay connection ended while Baddie was inactive."));
}

void UBaddieCompanionGameplaySession::RetireRequest()
{
    ++Data->Generation;
    // Keep bounded HTTP completion alive so an unknown begin proof or exchanged
    // credential can be cleaned up even after cancellation or UObject destruction.
    Data->Request.Reset();
    Data->NextOperation = EGameplayRequest::None;
    Data->NextAttempt = 0;
}

bool UBaddieCompanionGameplaySession::ClearPrivateState()
{
    Data->World.Clear();
    RetireRequest();
    const uint64 Generation = Data->Generation;
    const auto Consumer = Data->WorldConsumer;
    Data->Credentials.DeviceId.Reset();
    Data->Credentials.Proof.Reset();
    Data->Credentials.Token.Reset();
    Data->GrantId.Reset();
    Data->CrewThreadId.Reset();
    Data->ApprovedUserId.Reset(); Data->ApprovedCharacterId.Reset(); Data->ApprovedHomeRoomId.Reset();
    Data->LeaveMessage.Reset();
    Data->Identity = {};
    Data->Pairing = {}; Data->Grant = {}; Data->Presence = {};
    Data->EntrySequence = 0;
    Data->LastPresenceServerNow = 0;
    Data->bEntryOpen = false;
    Data->View = {};
    // The credential clear is complete before native teardown can reenter.
    if (auto* Controller = Cast<ABaddieCompanionSlicePlayerController>(Consumer.Get()))
        Controller->ClearWorldPresentation();
    return Data->Generation == Generation;
}

void UBaddieCompanionGameplaySession::Stop(EGameplayState State, const TCHAR* Message)
{
    if (Data->Credentials.DeviceId.IsEmpty() && !Data->Request.IsValid()
        && Data->View.State == State && BaddieGameplaySessionPrivate::Same(Data->View.Error, Message)) return;
    const auto Credentials = Data->Credentials;
    const int32 Sequence = Data->bEntryOpen ? Data->EntrySequence : 0;
    const bool bCurrent = ClearPrivateState();
    BaddieGameplaySessionPrivate::Cleanup(Credentials, Sequence, true);
    if (!bCurrent) return;
    Data->View.State = State;
    Data->View.Error = Message;
    OnConnectionChanged.Broadcast();
}

void UBaddieCompanionGameplaySession::Configure(const FString& ConvexOrigin, bool bEnabled)
{
    if (Data->bEnding) return;
    // Clear before notification. A reentrant subscriber cannot reuse the old proof.
    const auto Previous = Data->Credentials;
    const int32 Sequence = Data->bEntryOpen ? Data->EntrySequence : 0;
    const bool bCurrent = ClearPrivateState();
    BaddieGameplaySessionPrivate::Cleanup(Previous, Sequence, true);
    if (!bCurrent || Data->bEnding) return;
    Data->Credentials.Origin = BaddieGameplaySessionPrivate::DeploymentOrigin(ConvexOrigin) ? ConvexOrigin : FString();
    Data->bEnabled = bEnabled && !Data->Credentials.Origin.IsEmpty();
    Data->View.State = Data->bEnabled ? EGameplayState::SignedOut : EGameplayState::Disabled;
    OnConnectionChanged.Broadcast();
}

void UBaddieCompanionGameplaySession::BeginPairing(const FString& DeviceName)
{
    if (!Data->bEnabled || !IsForeground() || FPlatformTime::Seconds() < Data->BeginRetryAt
        || !Data->Credentials.DeviceId.IsEmpty()) return;
    const FString Name = DeviceName.TrimStartAndEnd();
    bool bValid = !Name.IsEmpty() && Name.Len() <= 80;
    for (TCHAR Ch : Name) if (Ch < 32 || Ch == 127) bValid = false;
    if (!bValid)
    {
        Stop(EGameplayState::Unavailable, TEXT("Enter a device name between 1 and 80 characters."));
        return;
    }
    if (!ClearPrivateState() || Data->bEnding) return;
    Data->Credentials.DeviceId = FGuid::NewGuid().ToString(EGuidFormats::Digits);
    Data->View.DeviceName = Name;
    Data->View.State = EGameplayState::RequestingCode;
    Send(EGameplayRequest::Begin);
    OnConnectionChanged.Broadcast();
}

void UBaddieCompanionGameplaySession::BeginWorldPairing(const FString&)
{
    if (Data->bEnding || !IsForeground() || !Data->Credentials.DeviceId.IsEmpty()) return;
    // World consent/public registration and enforced credential-safe transport are
    // absent. Never fall back to beginGameplay or upgrade an entry-only grant.
    Data->World.Clear();
    Data->View.Scope = BaddieCompanion::EGameplayScope::World;
    Data->View.State = EGameplayState::Unavailable;
    Data->View.Error = TEXT("World access is unavailable.");
    OnConnectionChanged.Broadcast();
}

void UBaddieCompanionGameplaySession::Enter(const FString& CrewThreadId)
{
    if (!CheckLifetimes() || Data->View.State != EGameplayState::Authorized || Data->Request.IsValid()
        || Data->bEntryOpen || Data->EntrySequence >= BaddieGameplaySessionPrivate::MaxEntrySequence) return;
    if (CrewThreadId.Len() > 128 || CrewThreadId != CrewThreadId.TrimStartAndEnd()) return;
    for (TCHAR Ch : CrewThreadId) if (Ch < 32 || Ch == 127) return;
    ++Data->EntrySequence;
    Data->bEntryOpen = true;
    Data->CrewThreadId = CrewThreadId;
    Data->View.State = EGameplayState::Entering;
    Data->View.Error.Reset();
    Send(EGameplayRequest::Enter);
    OnConnectionChanged.Broadcast();
}

void UBaddieCompanionGameplaySession::Leave()
{
    if (Data->bEntryOpen && Data->View.State != EGameplayState::Leaving) BeginLeave(TEXT(""));
}

void UBaddieCompanionGameplaySession::BeginLeave(const TCHAR* Message)
{
    if (!Data->bEntryOpen || Data->Credentials.Token.IsEmpty()) return;
    Data->World.Clear();
    RetireRequest();
    const uint64 Generation = Data->Generation;
    const uint64 ConsumerSerial = Data->WorldConsumerSerial;
    const auto Consumer = Data->WorldConsumer;
    Data->Identity = {};
    Data->Presence = {};
    Data->View.State = EGameplayState::Leaving;
    Data->View.bStale = false;
    Data->LeaveMessage = Message;
    Data->View.Error = Message;
    if (auto* Controller = Cast<ABaddieCompanionSlicePlayerController>(Consumer.Get()))
        Controller->ClearWorldPresentation();
    if (Data->bEnding || Data->Generation != Generation || Data->WorldConsumerSerial != ConsumerSerial
        || Consumer != Data->WorldConsumer || Data->View.State != EGameplayState::Leaving) return;
    // Keep this sequence occupied until the server acknowledges its retirement.
    Send(EGameplayRequest::Leave);
    OnConnectionChanged.Broadcast();
}

void UBaddieCompanionGameplaySession::Disconnect()
{
    Stop(Data->bEnabled ? EGameplayState::SignedOut : EGameplayState::Disabled, TEXT(""));
}

BaddieCompanion::FGameplayConnectionView UBaddieCompanionGameplaySession::GetConnectionView() const
{
    auto View = Data->View;
    const auto World = Data->World.View();
    View.WorldGeneration = World.Generation;
    View.WorldStatus = World.Status;
    View.GrantOrPairingRemainingSeconds = Data->Grant.IsSet() ? Data->Grant.Remaining() : Data->Pairing.Remaining();
    View.PresenceRemainingSeconds = Data->Presence.Remaining();
    const double RetryAt = Data->View.Error.IsEmpty() ? Data->BeginRetryAt : FMath::Max(Data->BeginRetryAt, Data->NextAttempt);
    View.RetryAfterSeconds = FMath::Max(0.0, RetryAt - FPlatformTime::Seconds());
    if ((!IsForeground() && !Data->Credentials.DeviceId.IsEmpty())
        || ((Data->Grant.IsSet() || Data->Pairing.IsSet()) && View.GrantOrPairingRemainingSeconds <= 0))
    {
        View = {};
        View.State = EGameplayState::Expired;
    }
    else if (View.State == EGameplayState::Joined && View.PresenceRemainingSeconds <= 0)
    {
        View.State = EGameplayState::EntryLost;
        View.bStale = true;
    }
    return View;
}

BaddieCompanion::FWorldSessionView UBaddieCompanionGameplaySession::GetWorldView() const
{
    if (!IsForeground() || Data->View.Scope != BaddieCompanion::EGameplayScope::World
        || Data->View.State != EGameplayState::Joined || Data->Grant.Remaining() <= 0
        || Data->Presence.Remaining() <= 0 || !Data->WorldConsumer.IsValid())
    {
        auto View = Data->World.View();
        View.Observation = {}; View.bHasObservation = View.bWalkingAvailable = false;
        return View;
    }
    return Data->World.View();
}

void UBaddieCompanionGameplaySession::AttachWorldConsumer(APlayerController* Consumer)
{
    if (Data->bEnding || !IsValid(Consumer) || !Consumer->IsLocalController()
        || Consumer->GetGameInstance() != GetGameInstance()) return;
    if (Data->WorldConsumer.Get() == Consumer) return;
    if (Data->WorldConsumerSerial == MAX_uint64) return;
    const auto Previous = Data->WorldConsumer;
    const uint64 Generation = Data->Generation;
    const uint64 Serial = ++Data->WorldConsumerSerial;
    Data->World.Detach(); Data->WorldConsumer.Reset();
    if (auto* Controller = Cast<ABaddieCompanionSlicePlayerController>(Previous.Get()))
        Controller->ClearWorldPresentation();
    if (Data->bEnding || Data->Generation != Generation || Data->WorldConsumerSerial != Serial
        || Data->WorldConsumer.IsValid() || !IsValid(Consumer) || !Consumer->IsLocalController()
        || Consumer->GetGameInstance() != GetGameInstance()) return;
    Data->WorldConsumer = Consumer; Data->World.Attach();
}

void UBaddieCompanionGameplaySession::DetachWorldConsumer(APlayerController* Consumer)
{
    if (Data->WorldConsumer.Get() != Consumer) return;
    if (Data->WorldConsumerSerial != MAX_uint64) ++Data->WorldConsumerSerial;
    Data->WorldConsumer.Reset(); Data->World.Detach();
    if (auto* Controller = Cast<ABaddieCompanionSlicePlayerController>(Consumer))
        Controller->ClearWorldPresentation();
}

void UBaddieCompanionGameplaySession::ReloadWorld(APlayerController* Consumer)
{
    if (Data->WorldConsumer.Get() != Consumer || !CheckLifetimes()
        || Data->View.Scope != BaddieCompanion::EGameplayScope::World) return;
    Data->World.Retry();
    TickWorld();
}

bool UBaddieCompanionGameplaySession::RequestWorldWalk(APlayerController* Consumer,
    const BaddieCompanion::FMovementPoint& Target, BaddieCompanion::EMovementPace Pace)
{
    if (Data->WorldConsumer.Get() != Consumer || !GetWorldView().bWalkingAvailable) return false;
    return Data->World.Walk(Target, Pace);
}

void UBaddieCompanionGameplaySession::StopWorldWalk(APlayerController* Consumer)
{
    if (Data->WorldConsumer.Get() == Consumer) Data->World.StopWalking();
}

void UBaddieCompanionGameplaySession::WorldContentLost(APlayerController* Consumer, BaddieCompanion::FWorldObservationToken Token)
{
    if (Data->WorldConsumer.Get() == Consumer) Data->World.ContentLost(Token);
}

void UBaddieCompanionGameplaySession::TickWorld()
{
    const bool bLive = !Data->bEnding && CheckLifetimes()
        && Data->View.Scope == BaddieCompanion::EGameplayScope::World
        && Data->View.State == EGameplayState::Joined && Data->WorldConsumer.IsValid();
    const auto Consumer = Data->WorldConsumer;
    const uint64 Generation = Data->Generation;
    const uint64 ConsumerSerial = Data->WorldConsumerSerial;
    Data->World.Tick(!bLive);
    if (!bLive || Data->bEnding || !CheckLifetimes() || Generation != Data->Generation
        || ConsumerSerial != Data->WorldConsumerSerial || Consumer != Data->WorldConsumer) return;
    // No callable public world adapter or enforced engine transport exists. Fail
    // before reading device credentials or constructing an HTTP world request.
    if (Data->World.Next != BaddieCompanion::EWorldRequest::None)
        Data->World.Unavailable(BaddieCompanion::EWorldReadStatus::Unavailable);
    if (auto* Controller = Cast<ABaddieCompanionSlicePlayerController>(Consumer.Get()))
        Controller->RefreshWorldPresentation();
    if (Data->bEnding || Generation != Data->Generation || ConsumerSerial != Data->WorldConsumerSerial
        || Consumer != Data->WorldConsumer
        || !CheckLifetimes() || Data->View.Scope != BaddieCompanion::EGameplayScope::World
        || Data->View.State != EGameplayState::Joined) return;
    auto& World = Data->World;
    const auto Token = World.PendingNotification;
    if (!Token.IsSet() || World.bInNotification || !World.State.Observation.IsCurrent()
        || World.State.Observation.Token() != Token) return;
    using namespace BaddieCompanion::WorldObservationPrivate;
    if (!QualifiedNotificationWork || QualifiedNotificationWork > TickWorkLimit || !World.Budget
        || !World.Budget->Spend(QualifiedNotificationWork, false)) return;
    // Consume before delivery. Reentrant changes can set a newer pending token.
    World.PendingNotification = {};
    TGuardValue<bool> Guard(World.bInNotification, true);
    World.NotifyRecovery(Token);
    if (Data->bEnding || Generation != Data->Generation || ConsumerSerial != Data->WorldConsumerSerial
        || Consumer != Data->WorldConsumer
        || !CheckLifetimes() || !World.State.Observation.IsCurrent()
        || World.State.Observation.Token() != Token) return;
    OnConnectionChanged.Broadcast();
}

bool UBaddieCompanionGameplaySession::CheckLifetimes()
{
    if (!Data->bEnabled || !IsForeground())
    {
        FocusLost();
        return false;
    }
    if ((Data->Grant.IsSet() && Data->Grant.Remaining() <= 0)
        || (!Data->Grant.IsSet() && Data->Pairing.IsSet() && Data->Pairing.Remaining() <= 0))
    {
        Stop(EGameplayState::Expired, TEXT("Gameplay access expired. Connect again in Baddie."));
        return false;
    }
    if (Data->View.State == EGameplayState::Joined && Data->Presence.Remaining() <= 0)
    {
        BeginLeave(TEXT("Presence expired. Join again."));
        return false;
    }
    return true;
}

bool UBaddieCompanionGameplaySession::TickSession(float)
{
    AttachSlateLifecycle();
    const bool bLive = CheckLifetimes();
    TickWorld();
    if (!bLive || !CheckLifetimes() || Data->Request.IsValid() || Data->NextOperation == EGameplayRequest::None
        || FPlatformTime::Seconds() < Data->NextAttempt) return true;
    Send(Data->NextOperation);
    return true;
}

void UBaddieCompanionGameplaySession::Schedule(EGameplayRequest Operation, double DelaySeconds, const TCHAR* Message)
{
    Data->NextOperation = Operation;
    Data->NextAttempt = FPlatformTime::Seconds() + DelaySeconds;
    Data->View.Error = Message;
}

void UBaddieCompanionGameplaySession::TransportFailed(EGameplayRequest Operation)
{
    if (Operation == EGameplayRequest::Begin)
    {
        Data->BeginRetryAt = FPlatformTime::Seconds() + 5;
        Stop(EGameplayState::Unavailable, TEXT("Couldn't request gameplay access. Try again."));
        return;
    }
    if (!CheckLifetimes()) return;
    Data->View.bStale = Operation == EGameplayRequest::Heartbeat;
    Schedule(Operation, 5, TEXT("Connection interrupted."));
    OnConnectionChanged.Broadcast();
}

void UBaddieCompanionGameplaySession::Send(EGameplayRequest Operation)
{
    using namespace BaddieGameplaySessionPrivate;
    if (Data->Request.IsValid() || !Data->bEnabled || !IsForeground() || Operation == EGameplayRequest::None) return;
    if (Data->View.Scope != BaddieCompanion::EGameplayScope::EntryOnly) return;
    const auto Credentials = Data->Credentials;
    if (Credentials.DeviceId.IsEmpty() || (Operation != EGameplayRequest::Begin && Credentials.Proof.IsEmpty())) return;
    const bool bGrant = Operation == EGameplayRequest::Enter || Operation == EGameplayRequest::Heartbeat || Operation == EGameplayRequest::Leave;
    if (bGrant && Credentials.Token.IsEmpty()) return;
    const auto Args = Operation == EGameplayRequest::Begin ? MakeShared<FJsonObject>() : Credentials.Args(bGrant);
    if (Operation == EGameplayRequest::Begin)
    {
        Args->SetStringField(TEXT("deviceId"), Credentials.DeviceId);
        Args->SetStringField(TEXT("deviceName"), Data->View.DeviceName);
    }
    if (bGrant) Args->SetNumberField(TEXT("entrySequence"), Data->EntrySequence);
    if (Operation == EGameplayRequest::Enter && !Data->CrewThreadId.IsEmpty())
        Args->SetStringField(TEXT("crewThreadId"), Data->CrewThreadId);
    const uint64 Generation = Data->Generation;
    const int32 Sequence = Data->EntrySequence;
    const double Started = FPlatformTime::Seconds();
    const FDateTime WallStarted = FDateTime::UtcNow();
    const TWeakObjectPtr<UBaddieCompanionGameplaySession> WeakThis(this);
    FBoundedResponsePtr ResponseBody;
    const auto Request = MakeRequest(Credentials.Origin, Path(Operation), Args, ResponseBody);
    if (!Request.IsValid())
    {
        Stop(EGameplayState::Unavailable, TEXT("This gameplay connection is unavailable."));
        return;
    }
    Data->Request = Request;
    Data->NextOperation = EGameplayRequest::None;
    Data->NextAttempt = 0;
    Request->OnProcessRequestComplete().BindLambda(
        [WeakThis, Credentials, Generation, Sequence, Operation, Started, WallStarted, ResponseBody]
        (FHttpRequestPtr Incoming, FHttpResponsePtr Response, bool bSuccess)
    {
        const auto Result = bSuccess ? Decode(Response, Credentials.Origin, ResponseBody) : nullptr;
        auto* Self = WeakThis.Get();
        if (!Self || Self->Data->bEnding || Self->Data->Generation != Generation)
        {
            CleanupLate(Operation, Credentials, Sequence, Result);
            return;
        }
        // A duplicate completion inside the still-current entry must not send a
        // stale-cleanup leave for that same entry. Only retirement grants cleanup.
        if (Self->Data->Request != Incoming) return;
        Self->Data->Request.Reset();
        if (ResponseBody->HasRejectedData())
        {
            Self->Stop(EGameplayState::Unavailable, TEXT("The gameplay response was not accepted."));
            return;
        }
        if (Retryable(Response, bSuccess)) { Self->TransportFailed(Operation); return; }
        if (!Result.IsValid())
        {
            Self->Stop(EGameplayState::Unavailable, TEXT("The gameplay response was not accepted."));
            return;
        }
        const double Elapsed = FMath::Max(0.0, FMath::Max(FPlatformTime::Seconds() - Started,
            (FDateTime::UtcNow() - WallStarted).GetTotalSeconds()));
        Self->AcceptResult(Operation, Result, Elapsed);
    });
    if (!Request->ProcessRequest() && Data->Generation == Generation && Data->Request == Request)
    {
        Data->Request.Reset();
        TransportFailed(Operation);
    }
}

void UBaddieCompanionGameplaySession::AcceptResult(EGameplayRequest Operation,
    const TSharedPtr<FJsonObject>& Result, double ElapsedSeconds)
{
    using namespace BaddieGameplaySessionPrivate;
    // Recover only structurally valid, expected-scope credentials before rejecting
    // an expired/late receipt, so rejection still has proof-authenticated cleanup.
    FString Recovered;
    if (Operation == EGameplayRequest::Begin && Scope(Result)
        && Text(Result, TEXT("deviceSecret"), 64, Recovered) && Hex(Recovered, 64))
        Data->Credentials.Proof = Recovered;
    if (Operation == EGameplayRequest::Exchange && Scope(Result) && GrantToken(Result, Recovered))
        Data->Credentials.Token = Recovered;
    if (!CheckLifetimes()) return;

    bool bOk = false;
    if (Operation == EGameplayRequest::Leave && Boolean(Result, TEXT("ok"), bOk) && bOk
        && !Result->HasField(TEXT("status")))
    {
        Data->bEntryOpen = false;
        Data->CrewThreadId.Reset();
        Data->Identity = {};
        Data->Presence = {};
        Data->LastPresenceServerNow = 0;
        Data->View.State = EGameplayState::Authorized;
        Data->View.Error = MoveTemp(Data->LeaveMessage);
        Data->View.bStale = false;
        OnConnectionChanged.Broadcast();
        return;
    }

    FString Status;
    if (!Text(Result, TEXT("status"), 32, Status))
    { Stop(EGameplayState::Unavailable, TEXT("The gameplay response was not accepted.")); return; }
    if (Same(Status, TEXT("slow_down")))
    {
        double RetryMs = 0;
        if (!Number(Result, TEXT("retryAfterMs"), RetryMs) || RetryMs <= 0 || RetryMs > 300000)
        { Stop(EGameplayState::Unavailable, TEXT("The gameplay retry was not accepted.")); return; }
        if (Operation == EGameplayRequest::Begin)
        {
            Data->BeginRetryAt = FPlatformTime::Seconds() + RetryMs / 1000;
            Stop(EGameplayState::Unavailable, TEXT("Too many gameplay connection requests."));
            return;
        }
        Data->View.bStale = Operation == EGameplayRequest::Heartbeat;
        Schedule(Operation, FMath::Max(2.0, RetryMs / 1000), TEXT("Too many gameplay connection requests."));
        OnConnectionChanged.Broadcast();
        return;
    }
    if (Same(Status, TEXT("entry_lost"))
        && (Operation == EGameplayRequest::Enter || Operation == EGameplayRequest::Heartbeat))
    { BeginLeave(TEXT("The previous entry ended. Join again.")); return; }
    if (Same(Status, TEXT("disabled")))
    { Stop(EGameplayState::Disabled, TEXT("Gameplay connections are unavailable.")); return; }
    if (Same(Status, TEXT("expired")))
    { Stop(EGameplayState::Expired, TEXT("Gameplay access expired. Connect again in Baddie.")); return; }
    if (Same(Status, TEXT("denied")) || Same(Status, TEXT("selection_changed"))
        || Same(Status, TEXT("unavailable")) || Same(Status, TEXT("too_many_connections")))
    {
        Stop(EGameplayState::Unavailable, Same(Status, TEXT("selection_changed"))
            ? TEXT("The approved Companion or Home changed. Connect again in Baddie.")
            : Same(Status, TEXT("too_many_connections")) ? TEXT("Disconnect another Companion connection in Baddie first.")
            : TEXT("Gameplay access is unavailable. Connect again in Baddie."));
        return;
    }

    if (Operation == EGameplayRequest::Begin && Same(Status, TEXT("pending")))
    {
        FString Token, Code, Name;
        double PollMs = 0, Seconds = 0, ServerNow = 0;
        if (!Scope(Result) || !Hex(Data->Credentials.Proof, 64)
            || !Text(Result, TEXT("pairingToken"), 64, Token) || !Hex(Token, 64)
            || !Text(Result, TEXT("displayCode"), 8, Code) || !Hex(Code.ToLower(), 8)
            || !Text(Result, TEXT("deviceName"), 80, Name) || !Same(Name, Data->View.DeviceName)
            || !Number(Result, TEXT("pollIntervalMs"), PollMs) || PollMs < 2000 || PollMs > 60000
            || !Lifetime(Result, TEXT("expiresAt"), PairingSeconds, ElapsedSeconds, Seconds, ServerNow)
            || !Data->Pairing.Set(Seconds))
        { Stop(EGameplayState::Unavailable, TEXT("The gameplay pairing was not accepted.")); return; }
        Data->View.DisplayCode = MoveTemp(Code);
        Data->View.ApprovalLink = TEXT("baddie://companion/play?token=") + Token;
        Data->View.State = EGameplayState::AwaitingApproval;
        Schedule(EGameplayRequest::Exchange, PollMs / 1000, TEXT(""));
    }
    else if (Operation == EGameplayRequest::Exchange && Same(Status, TEXT("pending")))
    {
        double PollMs = 0;
        if ((Result->HasField(TEXT("scope")) && !Scope(Result))
            || !Data->Credentials.Token.IsEmpty()
            || !Number(Result, TEXT("pollIntervalMs"), PollMs) || PollMs < 2000 || PollMs > 60000)
        { Stop(EGameplayState::Unavailable, TEXT("The gameplay pairing was not accepted.")); return; }
        Schedule(EGameplayRequest::Exchange, PollMs / 1000, TEXT(""));
    }
    else if (Operation == EGameplayRequest::Exchange && Same(Status, TEXT("connected")))
    {
        FString GrantId, Token;
        double Seconds = 0, ServerNow = 0;
        if (!Scope(Result) || !GrantToken(Result, Token) || !Text(Result, TEXT("grantId"), 128, GrantId)
            || !Lifetime(Result, TEXT("expiresAt"), GrantSeconds, ElapsedSeconds, Seconds, ServerNow)
            || !Data->Grant.Set(Seconds))
        { Stop(EGameplayState::Unavailable, TEXT("The gameplay grant was not accepted.")); return; }
        Data->Credentials.Token = MoveTemp(Token);
        Data->GrantId = MoveTemp(GrantId);
        Data->Pairing = {};
        Data->View.DisplayCode.Reset(); Data->View.ApprovalLink.Reset();
        Data->View.State = EGameplayState::Authorized;
        // Explicit Enter is required. Approval itself never starts call presence.
        Data->View.Error.Reset();
    }
    else if (Operation == EGameplayRequest::Enter && Same(Status, TEXT("joined")))
    {
        FPresenceIdentity Identity;
        FString GrantId;
        bool bVoice = true;
        double Sequence = 0, Seconds = 0, GrantRemaining = 0, ServerNow = 0, Expires = 0, GrantExpires = 0;
        if (!Scope(Result) || !Boolean(Result, TEXT("voice"), bVoice) || bVoice
            || !Text(Result, TEXT("grantId"), 128, GrantId) || !Same(GrantId, Data->GrantId)
            || !Number(Result, TEXT("entrySequence"), Sequence) || Sequence != Data->EntrySequence
            || !Data->bEntryOpen || !Identity.Read(Result)
            || (!Data->ApprovedCharacterId.IsEmpty() && (!Same(Identity.UserId, Data->ApprovedUserId)
                || !Same(Identity.CharacterId, Data->ApprovedCharacterId) || !Same(Identity.HomeRoomId, Data->ApprovedHomeRoomId)))
            || !Number(Result, TEXT("expiresAt"), Expires) || !Number(Result, TEXT("grantExpiresAt"), GrantExpires)
            || Expires > GrantExpires
            || !Lifetime(Result, TEXT("grantExpiresAt"), GrantSeconds, ElapsedSeconds, GrantRemaining, ServerNow)
            || !Lifetime(Result, TEXT("expiresAt"), PresenceSeconds, ElapsedSeconds, Seconds, ServerNow)
            || !Data->Grant.Set(GrantRemaining, true)
            || !Data->Presence.Set(FMath::Min(Seconds, Data->Grant.Remaining())))
        { Stop(EGameplayState::Unavailable, TEXT("The gameplay entry was not accepted.")); return; }
        Data->ApprovedUserId = Identity.UserId;
        Data->ApprovedCharacterId = Identity.CharacterId;
        Data->ApprovedHomeRoomId = Identity.HomeRoomId;
        Data->Identity = MoveTemp(Identity);
        Data->LastPresenceServerNow = ServerNow;
        Data->View.State = EGameplayState::Joined;
        Data->View.bStale = false;
        Schedule(EGameplayRequest::Heartbeat, FMath::Max(2.0, FMath::Min(10.0, Data->Presence.Remaining() / 3)), TEXT(""));
    }
    else if (Operation == EGameplayRequest::Heartbeat && Same(Status, TEXT("ready")))
    {
        double Sequence = 0, Seconds = 0, ServerNow = 0, RetryMs = 0;
        if (Data->View.State != EGameplayState::Joined || !Data->bEntryOpen
            || !Number(Result, TEXT("entrySequence"), Sequence) || Sequence != Data->EntrySequence
            || !Boolean(Result, TEXT("ok"), bOk) || !Number(Result, TEXT("retryAfterMs"), RetryMs) || RetryMs > 300000
            || !Lifetime(Result, TEXT("expiresAt"), PresenceSeconds, ElapsedSeconds, Seconds, ServerNow))
        { Stop(EGameplayState::Unavailable, TEXT("The presence renewal was not accepted.")); return; }
        const bool bNewReceipt = ServerNow > Data->LastPresenceServerNow;
        // A declined or old heartbeat cannot extend a lease. Grant time never renews.
        if (!Data->Presence.Set(FMath::Min(Seconds, Data->Grant.Remaining()), !bOk || !bNewReceipt))
        { BeginLeave(TEXT("Presence expired. Join again.")); return; }
        Data->LastPresenceServerNow = FMath::Max(Data->LastPresenceServerNow, ServerNow);
        Data->View.bStale = !bOk || !bNewReceipt;
        const double Delay = bOk && bNewReceipt ? FMath::Min(10.0, Data->Presence.Remaining() / 3)
            : FMath::Max(2.0, RetryMs / 1000);
        Schedule(EGameplayRequest::Heartbeat, FMath::Max(2.0, Delay), Data->View.bStale ? TEXT("Presence renewal pending.") : TEXT(""));
    }
    else
    { Stop(EGameplayState::Unavailable, TEXT("The gameplay response was not accepted.")); return; }
    OnConnectionChanged.Broadcast();
}
