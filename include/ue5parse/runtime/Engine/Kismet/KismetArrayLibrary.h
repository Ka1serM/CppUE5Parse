#pragma once

#include "ue5parse/runtime/Engine/Kismet/BlueprintFunctionLibrary.h"

#include "KismetArrayLibrary.generated.h"

namespace ue5parse::runtime {

// UKismetArrayLibrary: the array functions of Blueprints (UE's Kismet/KismetArrayLibrary.h). The functions take an
// array of any type: each runs through its custom thunk, which reads the array's property from the caller; the
// declared int32 types only name the parameters. Only the functions the cooked game calls are declared.
UCLASS()
class UKismetArrayLibrary : public UBlueprintFunctionLibrary {
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, CustomThunk)
    static int32 Array_Add(const TArray<int32>& TargetArray, const int32& NewItem);
    DECLARE_FUNCTION(execArray_Add);
    UFUNCTION(BlueprintCallable, CustomThunk)
    static int32 Array_AddUnique(const TArray<int32>& TargetArray, const int32& NewItem);
    DECLARE_FUNCTION(execArray_AddUnique);
    UFUNCTION(BlueprintCallable, CustomThunk)
    static void Array_Shuffle(const TArray<int32>& TargetArray);
    DECLARE_FUNCTION(execArray_Shuffle);
    UFUNCTION(BlueprintPure, CustomThunk)
    static bool Array_Identical(const TArray<int32>& ArrayA, const TArray<int32>& ArrayB);
    DECLARE_FUNCTION(execArray_Identical);
    UFUNCTION(BlueprintCallable, CustomThunk)
    static void Array_Append(const TArray<int32>& TargetArray, const TArray<int32>& SourceArray);
    DECLARE_FUNCTION(execArray_Append);
    UFUNCTION(BlueprintCallable, CustomThunk)
    static void Array_Insert(const TArray<int32>& TargetArray, const int32& NewItem, int32 Index);
    DECLARE_FUNCTION(execArray_Insert);
    UFUNCTION(BlueprintCallable, CustomThunk)
    static void Array_Remove(const TArray<int32>& TargetArray, int32 IndexToRemove);
    DECLARE_FUNCTION(execArray_Remove);
    UFUNCTION(BlueprintCallable, CustomThunk)
    static bool Array_RemoveItem(const TArray<int32>& TargetArray, const int32 &Item);
    DECLARE_FUNCTION(execArray_RemoveItem);
    UFUNCTION(BlueprintCallable, CustomThunk)
    static void Array_Clear(const TArray<int32>& TargetArray);
    DECLARE_FUNCTION(execArray_Clear);
    UFUNCTION(BlueprintCallable, CustomThunk)
    static void Array_Resize(const TArray<int32>& TargetArray, int32 Size);
    DECLARE_FUNCTION(execArray_Resize);
    UFUNCTION(BlueprintCallable, CustomThunk)
    static void Array_Reverse(const TArray<int32>& TargetArray);
    DECLARE_FUNCTION(execArray_Reverse);
    UFUNCTION(BlueprintPure, CustomThunk)
    static int32 Array_Length(const TArray<int32>& TargetArray);
    DECLARE_FUNCTION(execArray_Length);
    UFUNCTION(BlueprintPure, CustomThunk)
    static bool Array_IsEmpty(const TArray<int32>& TargetArray);
    DECLARE_FUNCTION(execArray_IsEmpty);
    UFUNCTION(BlueprintPure, CustomThunk)
    static bool Array_IsNotEmpty(const TArray<int32>& TargetArray);
    DECLARE_FUNCTION(execArray_IsNotEmpty);
    UFUNCTION(BlueprintPure, CustomThunk)
    static int32 Array_LastIndex(const TArray<int32>& TargetArray);
    DECLARE_FUNCTION(execArray_LastIndex);
    UFUNCTION(BlueprintPure, CustomThunk)
    static void Array_Get(const TArray<int32>& TargetArray, int32 Index, int32& Item);
    DECLARE_FUNCTION(execArray_Get);
    UFUNCTION(BlueprintCallable, CustomThunk)
    static void Array_Set(const TArray<int32>& TargetArray, int32 Index, const int32& Item, bool bSizeToFit);
    DECLARE_FUNCTION(execArray_Set);
    UFUNCTION(BlueprintCallable, CustomThunk)
    static void Array_Swap(const TArray<int32>& TargetArray, int32 FirstIndex, int32 SecondIndex);
    DECLARE_FUNCTION(execArray_Swap);
    UFUNCTION(BlueprintPure, CustomThunk)
    static int32 Array_Find(const TArray<int32>& TargetArray, const int32& ItemToFind);
    DECLARE_FUNCTION(execArray_Find);
    UFUNCTION(BlueprintPure, CustomThunk)
    static bool Array_Contains(const TArray<int32>& TargetArray, const int32& ItemToFind);
    DECLARE_FUNCTION(execArray_Contains);
    UFUNCTION(BlueprintCallable, CustomThunk)
    static void SetArrayPropertyByName(UObject* Object, FName PropertyName, const TArray<int32>& Value);
    DECLARE_FUNCTION(execSetArrayPropertyByName);
    UFUNCTION(BlueprintPure, CustomThunk)
    static bool Array_IsValidIndex(const TArray<int32>& TargetArray, int32 IndexToTest);
    DECLARE_FUNCTION(execArray_IsValidIndex);
    UFUNCTION(BlueprintPure, CustomThunk)
    static void Array_Random(const TArray<int32>& TargetArray, int32& OutItem, int32& OutIndex);
    DECLARE_FUNCTION(execArray_Random);
};

}
