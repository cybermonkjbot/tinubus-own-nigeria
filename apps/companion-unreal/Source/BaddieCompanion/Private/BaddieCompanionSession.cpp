#include "BaddieCompanionSession.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "HAL/PlatformTime.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/ConfigCacheIni.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

using BaddieCompanion::EConnectionState;

namespace
{
    constexpr double PairingSeconds = 300.0;
    constexpr double GrantSeconds = 900.0;
    constexpr int32 MaxResponseBytes = BaddieCompanion::MaxSnapshotBytes + 16 * 1024;
    const TCHAR* Scope = TEXT("companion_home_read");

    bool SameText(const FString& Left, const FString& Right)
    {
        return Left.Equals(Right, ESearchCase::CaseSensitive);
    }

    bool ReadText(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key, int32 Max, FString& Out)
    {
        const auto Value = Object.IsValid() ? Object->TryGetField(Key) : nullptr;
        return Value.IsValid() && Value->Type == EJson::String && Value->TryGetString(Out)
            && !Out.IsEmpty() && Out.Len() <= Max;
    }

    bool ReadNumber(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key, double& Out)
    {
        const auto Value = Object.IsValid() ? Object->TryGetField(Key) : nullptr;
        return Value.IsValid() && Value->Type == EJson::Number && Value->TryGetNumber(Out) && FMath::IsFinite(Out);
    }

    bool IsHex(const FString& Text, int32 Length)
    {
        if (Text.Len() != Length) return false;
        for (const TCHAR Ch : Text)
            if (!((Ch >= TEXT('0') && Ch <= TEXT('9')) || (Ch >= TEXT('a') && Ch <= TEXT('f')))) return false;
        return true;
    }

    bool IsDeploymentOrigin(const FString& Origin)
    {
        if (!Origin.StartsWith(TEXT("https://"), ESearchCase::CaseSensitive)
            || !Origin.EndsWith(TEXT(".convex.cloud"), ESearchCase::CaseSensitive)) return false;
        const FString Host = Origin.Mid(8, Origin.Len() - 8 - 13);
        if (Host.IsEmpty() || Host.Len() > 63 || Host.StartsWith(TEXT("-")) || Host.EndsWith(TEXT("-"))) return false;
        for (const TCHAR Ch : Host)
            if (!((Ch >= TEXT('a') && Ch <= TEXT('z')) || (Ch >= TEXT('0') && Ch <= TEXT('9')) || Ch == TEXT('-'))) return false;
        return true;
    }

    bool Encode(const TSharedRef<FJsonObject>& Object, FString& Out)
    {
        return FJsonSerializer::Serialize(Object, TJsonWriterFactory<>::Create(&Out));
    }

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> MakeRequest(
        const FString& Origin, const FString& Operation, const TSharedRef<FJsonObject>& Args)
    {
        const auto Envelope = MakeShared<FJsonObject>();
        Envelope->SetStringField(TEXT("path"), TEXT("companionGameAccess:") + Operation);
        Envelope->SetStringField(TEXT("format"), TEXT("convex_encoded_json"));
        TArray<TSharedPtr<FJsonValue>> Arguments;
        Arguments.Add(MakeShared<FJsonValueObject>(Args));
        Envelope->SetArrayField(TEXT("args"), Arguments);
        FString Body;
        Encode(Envelope, Body);
        const auto Request = FHttpModule::Get().CreateRequest();
        Request->SetURL(Origin + TEXT("/api/mutation"));
        Request->SetVerb(TEXT("POST"));
        Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
        Request->SetHeader(TEXT("Accept"), TEXT("application/json"));
        Request->SetHeader(TEXT("Cache-Control"), TEXT("no-store"));
        Request->SetTimeout(15.0f);
        Request->SetDelegateThreadPolicy(EHttpRequestDelegateThreadPolicy::CompleteOnGameThread);
        Request->SetContentAsString(Body);
        return Request;
    }

    double Elapsed(double Monotonic, const FDateTime& Wall)
    {
        return FMath::Max(0.0, FMath::Max(FPlatformTime::Seconds() - Monotonic,
            (FDateTime::UtcNow() - Wall).GetTotalSeconds()));
    }
}

struct FBaddieCompanionSessionData
{
    FString Origin, DeviceId, DeviceSecret, GameToken, GrantId;
    uint64 Generation = 0;
    bool bEnabled = false, bHasSnapshot = false, bRetryBackoff = false;
    double Deadline = 0, NextAttempt = 0, BeginRetryAt = 0;
    FDateTime WallDeadline;
    BaddieCompanion::FConnectionView View;
    BaddieCompanion::FSnapshotSelection Selection;
    FHttpRequestPtr Request;

    double Remaining(double Now = FPlatformTime::Seconds()) const
    {
        return FMath::Max(0.0, FMath::Min(Deadline - Now,
            (WallDeadline - FDateTime::UtcNow()).GetTotalSeconds()));
    }

    bool SetLifetime(const TSharedPtr<FJsonObject>& Result, double MaxSeconds, double ElapsedSeconds, bool bShortenOnly)
    {
        double ExpiresAt = 0, ServerNow = 0;
        if (!ReadNumber(Result, TEXT("expiresAt"), ExpiresAt) || !ReadNumber(Result, TEXT("serverNow"), ServerNow)
            || ServerNow <= 0 || ExpiresAt <= ServerNow || ExpiresAt - ServerNow > MaxSeconds * 1000.0) return false;
        double Seconds = (ExpiresAt - ServerNow) / 1000.0 - ElapsedSeconds;
        if (bShortenOnly) Seconds = FMath::Min(Seconds, Remaining());
        if (Seconds <= 0) return false;
        Deadline = FPlatformTime::Seconds() + Seconds;
        WallDeadline = FDateTime::UtcNow() + FTimespan::FromSeconds(Seconds);
        return true;
    }

    TSharedRef<FJsonObject> DeviceArgs() const
    {
        const auto Args = MakeShared<FJsonObject>();
        Args->SetStringField(TEXT("deviceId"), DeviceId);
        Args->SetStringField(TEXT("deviceSecret"), DeviceSecret);
        return Args;
    }
};

UBaddieCompanionSession::UBaddieCompanionSession() : Data(MakeUnique<FBaddieCompanionSessionData>()) {}
UBaddieCompanionSession::~UBaddieCompanionSession() = default;

void UBaddieCompanionSession::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    FString Origin;
    bool bEnabled = false;
    if (GConfig)
    {
        GConfig->GetString(TEXT("BaddieCompanion"), TEXT("ConvexOrigin"), Origin, GGameIni);
        GConfig->GetBool(TEXT("BaddieCompanion"), TEXT("bAccountLinkingEnabled"), bEnabled, GGameIni);
    }
    Configure(Origin, bEnabled);
    Ticker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UBaddieCompanionSession::TickSession), 0.25f);
}

void UBaddieCompanionSession::Deinitialize()
{
    FTSTicker::RemoveTicker(Ticker);
    Ticker.Reset();
    BestEffortDisconnect();
    ClearPrivateState();
    Data->bEnabled = false;
    Data->View.State = EConnectionState::Disabled;
    OnConnectionChanged.Broadcast();
    OnConnectionChanged.Clear();
    Super::Deinitialize();
}

void UBaddieCompanionSession::ClearPrivateState()
{
    ++Data->Generation;
    if (Data->Request.IsValid())
    {
        Data->Request->OnProcessRequestComplete().Unbind();
        Data->Request->CancelRequest();
        Data->Request.Reset();
    }
    Data->DeviceId.Reset(); Data->DeviceSecret.Reset(); Data->GameToken.Reset(); Data->GrantId.Reset();
    Data->Selection = {}; Data->View = {}; Data->bHasSnapshot = false; Data->bRetryBackoff = false;
    Data->Deadline = 0; Data->WallDeadline = FDateTime(); Data->NextAttempt = 0;
}

void UBaddieCompanionSession::Stop(EConnectionState State, const TCHAR* Message)
{
    ClearPrivateState();
    Data->View.State = State;
    Data->View.Error = Message;
    OnConnectionChanged.Broadcast();
}

void UBaddieCompanionSession::Configure(const FString& ConvexOrigin, bool bEnabled)
{
    BestEffortDisconnect();
    ClearPrivateState();
    Data->Origin = IsDeploymentOrigin(ConvexOrigin) ? ConvexOrigin : FString();
    Data->BeginRetryAt = 0;
    Data->bEnabled = bEnabled && !Data->Origin.IsEmpty();
    Data->View.State = Data->bEnabled ? EConnectionState::SignedOut : EConnectionState::Disabled;
    OnConnectionChanged.Broadcast();
}

void UBaddieCompanionSession::BeginPairing(const FString& DeviceName)
{
    if (!Data->bEnabled || FPlatformTime::Seconds() < Data->BeginRetryAt) return;
    BestEffortDisconnect();
    ClearPrivateState();
    const FString Name = DeviceName.TrimStartAndEnd();
    bool bValidName = !Name.IsEmpty() && Name.Len() <= 80;
    for (const TCHAR Ch : Name) if (Ch < 32 || Ch == 127) bValidName = false;
    if (!bValidName) { Stop(EConnectionState::Unavailable, TEXT("Enter a device name between 1 and 80 characters.")); return; }
    Data->DeviceId = FGuid::NewGuid().ToString(EGuidFormats::Digits);
    Data->View.DeviceName = Name;
    Data->View.State = EConnectionState::RequestingCode;
    const auto Args = MakeShared<FJsonObject>();
    Args->SetStringField(TEXT("deviceId"), Data->DeviceId);
    Args->SetStringField(TEXT("deviceName"), Name);
    Send(TEXT("begin"), Args);
    OnConnectionChanged.Broadcast();
}

void UBaddieCompanionSession::BestEffortDisconnect()
{
    if (Data->Origin.IsEmpty() || Data->DeviceSecret.IsEmpty()) return;
    const auto Args = Data->DeviceArgs();
    const bool bGrant = !Data->GameToken.IsEmpty();
    if (bGrant) Args->SetStringField(TEXT("gameToken"), Data->GameToken);
    const auto Request = MakeRequest(Data->Origin, bGrant ? TEXT("disconnect") : TEXT("cancel"), Args);
    // Detached, bounded cleanup cannot delay local sign-out or restore private state.
    Request->OnProcessRequestComplete().BindLambda([](FHttpRequestPtr, FHttpResponsePtr, bool) {});
    Request->ProcessRequest();
}

void UBaddieCompanionSession::Disconnect()
{
    BestEffortDisconnect();
    Stop(Data->bEnabled ? EConnectionState::SignedOut : EConnectionState::Disabled, TEXT(""));
}

BaddieCompanion::FConnectionView UBaddieCompanionSession::GetConnectionView() const
{
    auto View = Data->View;
    const double Now = FPlatformTime::Seconds();
    View.RemainingSeconds = Data->Remaining(Now);
    View.RetryAfterSeconds = FMath::Max(0.0, Data->BeginRetryAt - Now);
    const double AutomaticDelay = FMath::Max(0.0, Data->NextAttempt - Now);
    // Reuse the request owner's deadline. Do not display normal polling as an
    // error, or promise a retry that cannot happen within this grant/pairing.
    View.AutomaticRetryAfterSeconds = Data->bRetryBackoff && !Data->Request.IsValid()
        && AutomaticDelay < View.RemainingSeconds ? AutomaticDelay : 0;
    if (Data->Deadline > 0 && View.RemainingSeconds <= 0)
    {
        View = {};
        View.State = EConnectionState::Expired;
        View.Error = TEXT("Connection expired. Connect again in Baddie.");
    }
    return View;
}

bool UBaddieCompanionSession::CopySnapshot(BaddieCompanion::FSnapshotSelection& Out) const
{
    Out = {};
    if (!Data->bEnabled || !Data->bHasSnapshot || Data->GameToken.IsEmpty() || Data->Remaining() <= 0) return false;
    Out = Data->Selection;
    return true;
}

bool UBaddieCompanionSession::TickSession(float)
{
    if (Data->Deadline > 0 && Data->Remaining() <= 0)
    {
        Stop(EConnectionState::Expired, TEXT("Connection expired. Connect again in Baddie."));
        return true;
    }
    if (Data->Request.IsValid() || Data->DeviceSecret.IsEmpty() || FPlatformTime::Seconds() < Data->NextAttempt) return true;
    if (Data->GameToken.IsEmpty()) Send(TEXT("exchange"), Data->DeviceArgs());
    else RefreshSnapshot();
    return true;
}

void UBaddieCompanionSession::RefreshSnapshot()
{
    if (Data->Request.IsValid() || Data->GameToken.IsEmpty() || FPlatformTime::Seconds() < Data->NextAttempt) return;
    if (Data->Remaining() <= 0) { Stop(EConnectionState::Expired, TEXT("Connection expired. Connect again in Baddie.")); return; }
    const auto Args = Data->DeviceArgs();
    Args->SetStringField(TEXT("gameToken"), Data->GameToken);
    Send(TEXT("snapshot"), Args);
}

void UBaddieCompanionSession::TransportFailed(const FString& Operation)
{
    if (SameText(Operation, TEXT("begin")) || Data->Remaining() <= 0)
    {
        Stop(EConnectionState::Unavailable, TEXT("Couldn't connect. Request a new code and try again."));
        return;
    }
    Data->NextAttempt = FPlatformTime::Seconds() + 5.0;
    Data->bRetryBackoff = true;
    Data->View.bStale = Data->bHasSnapshot;
    Data->View.Error = TEXT("Connection interrupted.");
    OnConnectionChanged.Broadcast();
}

void UBaddieCompanionSession::Send(const FString& Operation, const TSharedRef<FJsonObject>& Args)
{
    if (!Data->bEnabled || Data->Request.IsValid()) return;
    const uint64 Generation = Data->Generation;
    const double Started = FPlatformTime::Seconds();
    const FDateTime WallStarted = FDateTime::UtcNow();
    const auto Request = MakeRequest(Data->Origin, Operation, Args);
    Data->Request = Request;
    Data->bRetryBackoff = false;
    Request->OnProcessRequestComplete().BindWeakLambda(this,
        [this, Generation, Operation, Started, WallStarted](FHttpRequestPtr Incoming, FHttpResponsePtr Response, bool bSuccess)
    {
        if (Generation != Data->Generation || Incoming != Data->Request) return;
        Data->Request.Reset();
        if (!bSuccess || !Response.IsValid()) { TransportFailed(Operation); return; }
        const int32 Code = Response->GetResponseCode();
        if (Code == 429 || Code == 500 || Code == 502 || Code == 503 || Code == 504) { TransportFailed(Operation); return; }
        if (Code != 200 || !Response->GetURL().Equals(Data->Origin + TEXT("/api/mutation"), ESearchCase::CaseSensitive)
            || Response->GetContent().Num() > MaxResponseBytes)
        { Stop(EConnectionState::Unavailable, TEXT("The connection response was not accepted.")); return; }
        TSharedPtr<FJsonObject> Envelope;
        const auto Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
        FString Status;
        const TSharedPtr<FJsonObject>* Value = nullptr;
        if (!FJsonSerializer::Deserialize(Reader, Envelope) || !ReadText(Envelope, TEXT("status"), 16, Status)
            || !SameText(Status, TEXT("success")) || !Envelope->TryGetObjectField(TEXT("value"), Value) || !Value || !Value->IsValid())
        { Stop(EConnectionState::Unavailable, TEXT("The connection response was not accepted.")); return; }
        AcceptResult(Operation, *Value, Elapsed(Started, WallStarted));
    });
    if (!Request->ProcessRequest() && Data->Request == Request)
    {
        Data->Request.Reset();
        TransportFailed(Operation);
    }
}

void UBaddieCompanionSession::AcceptResult(const FString& Operation, const TSharedPtr<FJsonObject>& Result, double ElapsedSeconds)
{
    FString Status;
    if (!ReadText(Result, TEXT("status"), 32, Status))
    { Stop(EConnectionState::Unavailable, TEXT("The connection response was not accepted.")); return; }
    if (SameText(Status, TEXT("disabled")))
    { Stop(EConnectionState::Disabled, TEXT("Companion connections are currently unavailable.")); return; }
    if (SameText(Status, TEXT("expired")))
    { Stop(EConnectionState::Expired, TEXT("Connection expired. Connect again in Baddie.")); return; }
    if (SameText(Status, TEXT("slow_down")))
    {
        double RetryMs = 0;
        if (!ReadNumber(Result, TEXT("retryAfterMs"), RetryMs) || RetryMs <= 0 || RetryMs > 300000)
        { Stop(EConnectionState::Unavailable, TEXT("Too many requests. Wait before connecting again.")); return; }
        if (SameText(Operation, TEXT("begin")))
        {
            ClearPrivateState();
            Data->BeginRetryAt = FPlatformTime::Seconds() + RetryMs / 1000.0;
            Data->View.State = EConnectionState::Unavailable;
            Data->View.Error = TEXT("Too many connection requests.");
            OnConnectionChanged.Broadcast();
            return;
        }
        Data->NextAttempt = FPlatformTime::Seconds() + FMath::Max(2.0, RetryMs / 1000.0);
        Data->bRetryBackoff = true;
        Data->View.bStale = Data->bHasSnapshot;
        Data->View.Error = TEXT("Too many connection requests.");
        OnConnectionChanged.Broadcast();
        return;
    }
    if (SameText(Status, TEXT("denied")) || SameText(Status, TEXT("selection_changed")) || SameText(Status, TEXT("unavailable")) || SameText(Status, TEXT("too_many_connections")))
    {
        Stop(EConnectionState::Unavailable, SameText(Status, TEXT("selection_changed"))
            ? TEXT("The shared Companion or Home changed. Connect again in Baddie.")
            : SameText(Status, TEXT("too_many_connections")) ? TEXT("Disconnect another Companion connection in Baddie first.")
            : TEXT("Access is no longer available. Connect again in Baddie."));
        return;
    }
    if (Data->Deadline > 0 && Data->Remaining() <= 0)
    { Stop(EConnectionState::Expired, TEXT("Connection expired. Connect again in Baddie.")); return; }
    if (SameText(Operation, TEXT("exchange")) && SameText(Status, TEXT("pending")))
    {
        double PollMs = 0;
        if (!ReadNumber(Result, TEXT("pollIntervalMs"), PollMs) || PollMs < 2000 || PollMs > 60000)
        { Stop(EConnectionState::Unavailable, TEXT("The connection response was not accepted.")); return; }
        Data->NextAttempt = FPlatformTime::Seconds() + PollMs / 1000.0;
        Data->View.Error.Reset();
        OnConnectionChanged.Broadcast();
        return;
    }
    FString ReturnedScope;
    if (!ReadText(Result, TEXT("scope"), 32, ReturnedScope) || !SameText(ReturnedScope, Scope))
    { Stop(EConnectionState::Unavailable, TEXT("The connection scope was not accepted.")); return; }
    if (SameText(Operation, TEXT("begin")) && SameText(Status, TEXT("pending")))
    {
        FString PairingToken, Proof, Code, DeviceName;
        double PollMs = 0;
        if (!ReadText(Result, TEXT("pairingToken"), 64, PairingToken) || !IsHex(PairingToken, 64)
            || !ReadText(Result, TEXT("deviceSecret"), 64, Proof) || !IsHex(Proof, 64)
            || !ReadText(Result, TEXT("displayCode"), 8, Code) || !IsHex(Code.ToLower(), 8)
            || !ReadText(Result, TEXT("deviceName"), 80, DeviceName) || !SameText(DeviceName, Data->View.DeviceName)
            || !ReadNumber(Result, TEXT("pollIntervalMs"), PollMs) || PollMs < 2000 || PollMs > 60000
            || !Data->SetLifetime(Result, PairingSeconds, ElapsedSeconds, false))
        { Stop(EConnectionState::Unavailable, TEXT("The connection request was not accepted.")); return; }
        Data->DeviceSecret = MoveTemp(Proof);
        Data->View.DisplayCode = MoveTemp(Code);
        Data->View.ApprovalLink = TEXT("baddie://companion/connect?token=") + PairingToken;
        Data->View.State = EConnectionState::AwaitingApproval;
        Data->NextAttempt = FPlatformTime::Seconds() + PollMs / 1000.0;
    }
    else if (SameText(Operation, TEXT("exchange")) && SameText(Status, TEXT("connected")))
    {
        FString Token, GrantId;
        if (!ReadText(Result, TEXT("gameToken"), 69, Token) || !Token.StartsWith(TEXT("bcg1_"), ESearchCase::CaseSensitive)
            || !IsHex(Token.Mid(5), 64) || !ReadText(Result, TEXT("grantId"), 128, GrantId)
            || !Data->SetLifetime(Result, GrantSeconds, ElapsedSeconds, false))
        { Stop(EConnectionState::Unavailable, TEXT("The game grant was not accepted.")); return; }
        Data->GameToken = MoveTemp(Token); Data->GrantId = MoveTemp(GrantId);
        Data->View.DisplayCode.Reset(); Data->View.ApprovalLink.Reset();
        Data->View.State = EConnectionState::LoadingHome;
        Data->NextAttempt = FPlatformTime::Seconds();
    }
    else if (SameText(Operation, TEXT("snapshot")) && SameText(Status, TEXT("ready")))
    {
        FString GrantId, Json, Error;
        const TSharedPtr<FJsonObject>* Library = nullptr;
        const TArray<TSharedPtr<FJsonValue>>* Characters = nullptr;
        const TArray<TSharedPtr<FJsonValue>>* Rooms = nullptr;
        BaddieCompanion::FSnapshotSelection Selection;
        if (!ReadText(Result, TEXT("grantId"), 128, GrantId) || !SameText(GrantId, Data->GrantId)
            || !Data->SetLifetime(Result, GrantSeconds, ElapsedSeconds, true)
            || !Result->TryGetObjectField(TEXT("library"), Library) || !Library || !Library->IsValid()
            || !(*Library)->TryGetArrayField(TEXT("characters"), Characters) || !Characters || Characters->Num() != 1
            || !(*Library)->TryGetArrayField(TEXT("rooms"), Rooms) || !Rooms || Rooms->Num() != 1
            || !Encode(Result.ToSharedRef(), Json) || !BaddieCompanion::ReadSnapshotSelection(Json, Selection, Error)
            || !SameText(Selection.ActiveRoom.Id, Selection.HomeRoom.Id)
            || (Data->bHasSnapshot && (!SameText(Selection.Character.Id, Data->Selection.Character.Id)
                || !SameText(Selection.HomeRoom.Id, Data->Selection.HomeRoom.Id) || Selection.Revision < Data->Selection.Revision)))
        { Stop(EConnectionState::Unavailable, TEXT("The saved Companion or Home is unavailable.")); return; }
        Data->Selection = MoveTemp(Selection); Data->bHasSnapshot = true;
        Data->View.State = EConnectionState::Ready; Data->View.bStale = false;
        Data->NextAttempt = FPlatformTime::Seconds() + 5.0;
    }
    else { Stop(EConnectionState::Unavailable, TEXT("The connection response was not accepted.")); return; }
    Data->View.Error.Reset();
    OnConnectionChanged.Broadcast();
}
