#include "BaddieCompanionStateJson.h"

#include "Containers/SharedString.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include <cmath>

namespace BaddieCompanion
{
    namespace StateJsonPrivate
    {
        bool WithinParseLimits(const FString& Json, const FParseLimits& Limits)
        {
            if (Json.IsEmpty() || Json.Len() > Limits.Characters) return false;
            bool bString = false, bEscape = false;
            int32 Depth = 0, Separators = 0;
            for (const TCHAR C : Json)
            {
                if (bString)
                {
                    if (bEscape) bEscape = false;
                    else if (C == TEXT('\\')) bEscape = true;
                    else if (C == TEXT('"')) bString = false;
                    continue;
                }
                if (C == TEXT('"')) bString = true;
                else if (C == TEXT('{') || C == TEXT('[')) { if (++Depth > Limits.Depth) return false; }
                else if (C == TEXT('}') || C == TEXT(']')) { if (--Depth < 0) return false; }
                else if (C == TEXT(',') || C == TEXT(':')) { if (++Separators > Limits.Separators) return false; }
            }
            return !bString && Depth == 0;
        }
        bool Parse(const FString& Json, FObject& Out, const FParseLimits& Limits)
        {
            return WithinParseLimits(Json, Limits)
                && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Out) && Out.IsValid();
        }
        bool ParseNull(const FString& Json, FValue& Out, const FParseLimits& Limits)
        {
            if (!WithinParseLimits(Json, Limits) || !Same(Json.TrimStartAndEnd(), TEXT("null"))) return false;
            const FString Wrapped = FString(TEXT("[")) + Json + TEXT("]");
            TArray<FValue> Values;
            if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Wrapped), Values)
                || Values.Num() != 1 || !Values[0].IsValid() || Values[0]->Type != EJson::Null) return false;
            Out = MoveTemp(Values[0]);
            return true;
        }
        bool Same(const FString& A, const FString& B)
        {
            return A.Equals(B, ESearchCase::CaseSensitive);
        }
        FObject Object(const FValue& Value)
        {
            return Value.IsValid() && Value->Type == EJson::Object ? Value->AsObject() : nullptr;
        }
        FValue Field(const FObject& Parent, const TCHAR* Name)
        {
            return Parent.IsValid() ? Parent->TryGetField(Name) : nullptr;
        }
        bool Only(const FObject& Parent, std::initializer_list<const TCHAR*> Keys)
        {
            if (!Parent.IsValid()) return false;
            for (const auto& Pair : Parent->Values)
            {
                // UE 5.8 JSON keys use shared strings. Preserve their full length.
                const FString Name(Pair.Key.Len(), *Pair.Key);
                bool bKnown = false;
                for (const auto Key : Keys) if (Same(Name, Key)) { bKnown = true; break; }
                if (!bKnown) return false;
            }
            return true;
        }
        bool String(const FValue& Value, FString& Out)
        {
            return Value.IsValid() && Value->Type == EJson::String && Value->TryGetString(Out);
        }
        bool Number(const FValue& Value, double& Out)
        {
            return Value.IsValid() && Value->Type == EJson::Number && Value->TryGetNumber(Out) && std::isfinite(Out);
        }
        bool Number(const FObject& Parent, const TCHAR* Name, double& Out)
        {
            return Number(Field(Parent, Name), Out);
        }
        bool Boolean(const FValue& Value, bool& Out)
        {
            return Value.IsValid() && Value->Type == EJson::Boolean && Value->TryGetBool(Out);
        }
        bool Integer(const FValue& Value, int64& Out, int64 Minimum)
        {
            double N = 0;
            if (!Number(Value, N) || N < Minimum || N > MaxInteger || std::floor(N) != N) return false;
            Out = static_cast<int64>(N);
            return true;
        }
        bool ValidTime(double Value)
        {
            return std::isfinite(Value) && Value >= 0 && Value <= MaxInteger;
        }
    }
}
