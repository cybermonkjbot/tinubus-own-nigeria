#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include <initializer_list>

namespace BaddieCompanion
{
    // Private helpers only. Domain schemas, presence types and admission stay with callers.
    namespace StateJsonPrivate
    {
        using FObject = TSharedPtr<FJsonObject>;
        using FValue = TSharedPtr<FJsonValue>;
        constexpr int64 MaxInteger = 9007199254740991LL;

        struct FParseLimits
        {
            int32 Characters;
            int32 Depth;
            int32 Separators;
        };

        // Object roots only. Callers own raw-null handling and atomic publication.
        bool Parse(const FString& Json, FObject& Out, const FParseLimits& Limits);
        bool Same(const FString& A, const FString& B);
        FObject Object(const FValue& Value);
        FValue Field(const FObject& Parent, const TCHAR* Name);
        // Opt-in rejection of unknown keys. This does not enforce required keys.
        bool Only(const FObject& Parent, std::initializer_list<const TCHAR*> Keys);
        // Exact strings, finite numbers and typed booleans. No coercion or trimming.
        bool String(const FValue& Value, FString& Out);
        bool Number(const FValue& Value, double& Out);
        bool Number(const FObject& Parent, const TCHAR* Name, double& Out);
        bool Boolean(const FValue& Value, bool& Out);
        bool Integer(const FValue& Value, int64& Out, int64 Minimum = 0);
        bool ValidTime(double Value);

        // Required nullable value into a fresh candidate. Missing fails; null succeeds
        // without assigning. Optional-field presence and null policy are not handled here.
        template <typename T, typename F> bool Nullable(const FValue& Value, TOptional<T>& Out, F Read)
        {
            if (!Value.IsValid()) return false;
            if (Value->Type == EJson::Null) return true;
            T Next;
            if (!Read(Value, Next)) return false;
            Out = MoveTemp(Next);
            return true;
        }
    }
}
