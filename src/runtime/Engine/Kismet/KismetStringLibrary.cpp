#include "ue5parse/runtime/Engine/Kismet/KismetStringLibrary.h"

#include <string>

// UE's Kismet/KismetStringLibrary.cpp (UE 5.3).

namespace ue5parse::runtime {

FString UKismetStringLibrary::Conv_IntToString(int32 InInt) { return FString(std::to_string(InInt)); }

FString UKismetStringLibrary::BuildString_Name(const FString& AppendTo, const FString& Prefix, FName InName, const FString& Suffix) {
    FString Result = AppendTo;
    Result += Prefix;
    Result += NameToString(InName);
    Result += Suffix;
    return Result;
}

FString UKismetStringLibrary::BuildString_Int(const FString& AppendTo, const FString& Prefix, int32 InInt, const FString& Suffix) {
    FString Result = AppendTo;
    Result += Prefix;
    Result += Conv_IntToString(InInt);
    Result += Suffix;
    return Result;
}

}
