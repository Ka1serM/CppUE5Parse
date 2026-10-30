#pragma once

#include "ue5parse/runtime/Engine/Kismet/BlueprintFunctionLibrary.h"
#include "ue5parse/runtime/Core/Containers/Set.h"

#include "BlueprintSetLibrary.generated.h"

namespace ue5parse::runtime {

// UBlueprintSetLibrary: the set functions of Blueprints (UE's Kismet/BlueprintSetLibrary.h), on sets of any type through
// their custom thunks. Only the functions the cooked game calls are declared.
UCLASS()
class UBlueprintSetLibrary : public UBlueprintFunctionLibrary {
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, CustomThunk)
    static void Set_Add(const TSet<int32>& TargetSet, const int32& NewItem);
    DECLARE_FUNCTION(execSet_Add);
    UFUNCTION(BlueprintCallable, CustomThunk)
    static void Set_AddItems(const TSet<int32>& TargetSet, const TArray<int32>& NewItems);
    DECLARE_FUNCTION(execSet_AddItems);
    UFUNCTION(BlueprintCallable, CustomThunk)
    static bool Set_Remove(const TSet<int32>& TargetSet, const int32& Item);
    DECLARE_FUNCTION(execSet_Remove);
    UFUNCTION(BlueprintPure, CustomThunk)
    static bool Set_IsNotEmpty(const TSet<int32>& TargetSet);
    DECLARE_FUNCTION(execSet_IsNotEmpty);
    UFUNCTION(BlueprintCallable, CustomThunk)
    static void Set_RemoveItems(const TSet<int32>& TargetSet, const TArray<int32>& Items);
    DECLARE_FUNCTION(execSet_RemoveItems);
    UFUNCTION(BlueprintCallable, CustomThunk)
    static void Set_ToArray(const TSet<int32>& A, TArray<int32>& Result);
    DECLARE_FUNCTION(execSet_ToArray);
    UFUNCTION(BlueprintCallable, CustomThunk)
    static void Set_Clear(const TSet<int32>& TargetSet);
    DECLARE_FUNCTION(execSet_Clear);
    UFUNCTION(BlueprintPure, CustomThunk)
    static int32 Set_Length(const TSet<int32>& TargetSet);
    DECLARE_FUNCTION(execSet_Length);
    UFUNCTION(BlueprintPure, CustomThunk)
    static bool Set_Contains(const TSet<int32>& TargetSet, const int32& ItemToFind);
    DECLARE_FUNCTION(execSet_Contains);
    UFUNCTION(BlueprintCallable, CustomThunk)
    static void Set_Intersection(const TSet<int32>& A, const TSet<int32>& B, TSet<int32>& Result );
    DECLARE_FUNCTION(execSet_Intersection);
    UFUNCTION(BlueprintCallable, CustomThunk)
    static void Set_Union(const TSet<int32>& A, const TSet<int32>& B, TSet<int32>& Result );
    DECLARE_FUNCTION(execSet_Union);
    UFUNCTION(BlueprintCallable, CustomThunk)
    static void Set_Difference(const TSet<int32>& A, const TSet<int32>& B, TSet<int32>& Result );
    DECLARE_FUNCTION(execSet_Difference);
};

}
