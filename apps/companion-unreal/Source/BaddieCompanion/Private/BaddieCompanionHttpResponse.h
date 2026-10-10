#pragma once

#include "CoreMinimal.h"
#include "HAL/CriticalSection.h"
#include "Interfaces/IHttpRequest.h"

namespace BaddieCompanionHttpPrivate
{
    class FBoundedResponse;
    using FBoundedResponsePtr = TSharedPtr<FBoundedResponse, ESPMode::ThreadSafe>;

    // Request-owned receive storage. This does not enforce redirect or TLS policy.
    class FBoundedResponse final
    {
    public:
        explicit FBoundedResponse(int32 MaxBytes);
        ~FBoundedResponse();

        // Bind before ProcessRequest. Failure means the request must not be sent.
        static FBoundedResponsePtr Bind(const TSharedRef<IHttpRequest, ESPMode::ThreadSafe>& Request,
            int32 MaxBytes);
        bool HasRejectedData() const;
        // Call only after successful HTTP completion, never to accept a partial body.
        bool ReadCompletedText(FString& Out) const;

    private:
        void Receive(void* Data, int64& InOutLength);

        mutable FCriticalSection Mutex;
        TArray<uint8> Storage;
        int32 UsedBytes = 0;
        bool bRejected = false;
    };
}
