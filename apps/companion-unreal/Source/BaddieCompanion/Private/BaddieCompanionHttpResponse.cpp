#include "BaddieCompanionHttpResponse.h"

#include "Containers/StringConv.h"
#include "Misc/ScopeLock.h"

namespace BaddieCompanionHttpPrivate
{
    FBoundedResponse::FBoundedResponse(int32 MaxBytes)
    {
        Storage.SetNumUninitialized(MaxBytes);
    }

    FBoundedResponse::~FBoundedResponse()
    {
        if (UsedBytes > 0) FMemory::Memzero(Storage.GetData(), UsedBytes);
    }

    FBoundedResponsePtr FBoundedResponse::Bind(
        const TSharedRef<IHttpRequest, ESPMode::ThreadSafe>& Request, int32 MaxBytes)
    {
        if (MaxBytes <= 0) return nullptr;
        const auto Body = MakeShared<FBoundedResponse, ESPMode::ThreadSafe>(MaxBytes);
        if (!Request->SetResponseBodyReceiveStreamDelegateV2(FHttpRequestStreamDelegateV2::CreateLambda(
            [Body](void* Data, int64& InOutLength) { Body->Receive(Data, InOutLength); }))) return nullptr;
        return Body;
    }

    void FBoundedResponse::Receive(void* Data, int64& InOutLength)
    {
        FScopeLock Lock(&Mutex);
        // V2 aborts receiving when the processed length differs from the supplied length.
        if (bRejected || InOutLength < 0 || InOutLength > Storage.Num() - UsedBytes
            || (InOutLength > 0 && !Data))
        {
            bRejected = true;
            InOutLength = 0;
            return;
        }
        if (InOutLength == 0) return;
        FMemory::Memcpy(Storage.GetData() + UsedBytes, Data, static_cast<SIZE_T>(InOutLength));
        UsedBytes += static_cast<int32>(InOutLength);
    }

    bool FBoundedResponse::HasRejectedData() const
    {
        FScopeLock Lock(&Mutex);
        return bRejected;
    }

    bool FBoundedResponse::ReadCompletedText(FString& Out) const
    {
        Out.Reset();
        FScopeLock Lock(&Mutex);
        if (bRejected || UsedBytes == 0) return false;
        // Raw NUL is not JSON. Do not let string parsing ignore a trailing byte suffix.
        for (int32 Index = 0; Index < UsedBytes; ++Index)
            if (Storage[Index] == 0) return false;
        const FUTF8ToTCHAR Text(reinterpret_cast<const ANSICHAR*>(Storage.GetData()), UsedBytes);
        // Reject malformed UTF-8 rather than accepting replacement characters in private data.
        const FTCHARToUTF8 Encoded(Text.Get(), Text.Length());
        if (Encoded.Length() != UsedBytes
            || FMemory::Memcmp(Encoded.Get(), Storage.GetData(), UsedBytes) != 0) return false;
        Out = FString(Text.Length(), Text.Get());
        return true;
    }
}
