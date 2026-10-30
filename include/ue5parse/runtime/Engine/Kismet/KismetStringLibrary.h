#pragma once

#include "ue5parse/runtime/Engine/Kismet/BlueprintFunctionLibrary.h"

#include "KismetStringLibrary.generated.h"

namespace ue5parse::runtime {

// UKismetStringLibrary: the string functions of Blueprints (UE's Kismet/KismetStringLibrary.h). Only the functions the
// cooked game calls are declared.
UCLASS()
class UKismetStringLibrary : public UBlueprintFunctionLibrary {
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure)
    static FString Concat_StrStr(const FString& A, const FString& B) { return A + B; }
    UFUNCTION(BlueprintPure)
    static FString Conv_IntToString(int32 InInt);
    UFUNCTION(BlueprintPure)
    static FName Conv_StringToName(const FString& InString) { return MakeName(InString); }
    UFUNCTION(BlueprintPure)
    static FString Conv_NameToString(FName InName) { return NameToString(InName); }
    UFUNCTION(BlueprintPure)
    static FString BuildString_Name(const FString& AppendTo, const FString& Prefix, FName InName, const FString& Suffix);
    UFUNCTION(BlueprintPure)
    static FString BuildString_Int(const FString& AppendTo, const FString& Prefix, int32 InInt, const FString& Suffix);
    UFUNCTION(BlueprintPure)
    static int32 Len(const FString& S) { return S.Len(); }
    UFUNCTION(BlueprintPure)
    static bool IsEmpty(const FString& InString) { return InString.Len() == 0; }
    UFUNCTION(BlueprintPure)
    static bool EqualEqual_StrStr(const FString& A, const FString& B) { return A == B; }
    UFUNCTION(BlueprintPure)
    static bool NotEqual_StrStr(const FString& A, const FString& B) { return !(A == B); }
    UFUNCTION(BlueprintPure)
    static bool NotEqual_StriStri(const FString& A, const FString& B) { return !A.Equals(B, ESearchCase::IgnoreCase); }
    UFUNCTION(BlueprintPure)
    static bool Contains(const FString& SearchIn, const FString& Substring, bool bUseCase = false, bool bSearchFromEnd = false) {
        return SearchIn.Contains(Substring, bUseCase ? ESearchCase::CaseSensitive : ESearchCase::IgnoreCase,
                                 bSearchFromEnd ? ESearchDir::FromEnd : ESearchDir::FromStart);
    }
};

}
