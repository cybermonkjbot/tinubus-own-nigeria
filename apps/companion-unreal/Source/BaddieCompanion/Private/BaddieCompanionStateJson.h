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

        // Shared allocation preflight for any root. Passing is not JSON syntax validation;
        // callers still use the UE serializer and enforce their expected root type.
        bool WithinParseLimits(const FString& Json, const FParseLimits& Limits);
        // Object roots only. Callers own raw-null handling and atomic publication.
        bool Parse(const FString& Json, FObject& Out, const FParseLimits& Limits);
        // Null roots only. UE validates the original input inside one array because its
        // reader requires a container root. Assigns Out only after successful validation.
        bool ParseNull(const FString& Json, FValue& Out, const FParseLimits& Limits);
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

        // Domain choice tables stay with callers. Assign only an exact string match.
        template <typename T> bool EnumValue(const FValue& Value, T& Out,
            std::initializer_list<TPair<const TCHAR*, T>> Choices)
        {
            FString Name;
            if (!String(Value, Name)) return false;
            for (const auto& Choice : Choices)
                if (Same(Name, Choice.Key)) { Out = Choice.Value; return true; }
            return false;
        }

        // Decode into a caller-owned fresh candidate. Appends in source order and
        // leaves partial output on failure; the enclosing decoder owns atomic publication.
        template <typename T, typename F> bool ReadBoundedArray(const FValue& Value,
            TArray<T>& Out, int32 Maximum, F Read)
        {
            if (!Value.IsValid() || Value->Type != EJson::Array) return false;
            const auto& Values = Value->AsArray();
            if (Values.Num() > Maximum) return false;
            for (const auto& Item : Values)
            {
                T Next;
                if (!Read(Item, Next)) return false;
                Out.Add(MoveTemp(Next));
            }
            return true;
        }

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
