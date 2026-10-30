#pragma once

#include "ue5parse/runtime/Engine/Kismet/BlueprintFunctionLibrary.h"
#include "ue5parse/runtime/Core/Containers/Map.h"

#include "BlueprintMapLibrary.generated.h"

namespace ue5parse::runtime {

// UBlueprintMapLibrary: the map functions of Blueprints (UE's Kismet/BlueprintMapLibrary.h), on maps of any type through
// their custom thunks. Only the functions the cooked game calls are declared.
UCLASS()
class UBlueprintMapLibrary : public UBlueprintFunctionLibrary {
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, CustomThunk)
    static void Map_Add(const TMap<int32, int32>& TargetMap, const int32& Key, const int32& Value);
    DECLARE_FUNCTION(execMap_Add);
    UFUNCTION(BlueprintCallable, CustomThunk)
    static bool Map_Remove(const TMap<int32, int32>& TargetMap, const int32& Key);
    DECLARE_FUNCTION(execMap_Remove);
    UFUNCTION(BlueprintPure, CustomThunk)
    static bool Map_Find(const TMap<int32, int32>& TargetMap, const int32& Key, int32& Value);
    DECLARE_FUNCTION(execMap_Find);
    UFUNCTION(BlueprintPure, CustomThunk)
    static bool Map_Contains(const TMap<int32, int32>& TargetMap, const int32& Key);
    DECLARE_FUNCTION(execMap_Contains);
    UFUNCTION(BlueprintCallable, CustomThunk)
    static void Map_Keys(const TMap<int32, int32>& TargetMap, TArray<int32>& Keys);
    DECLARE_FUNCTION(execMap_Keys);
    UFUNCTION(BlueprintCallable, CustomThunk)
    static void Map_Values(const TMap<int32, int32>& TargetMap, TArray<int32>& Values);
    DECLARE_FUNCTION(execMap_Values);
    UFUNCTION(BlueprintPure, CustomThunk)
    static int32 Map_Length(const TMap<int32, int32>& TargetMap);
    DECLARE_FUNCTION(execMap_Length);
    UFUNCTION(BlueprintPure, CustomThunk)
    static bool Map_IsEmpty(const TMap<int32, int32>& TargetMap);
    DECLARE_FUNCTION(execMap_IsEmpty);
    UFUNCTION(BlueprintPure, CustomThunk)
    static bool Map_IsNotEmpty(const TMap<int32, int32>& TargetMap);
    DECLARE_FUNCTION(execMap_IsNotEmpty);
    UFUNCTION(BlueprintCallable, CustomThunk)
    static void Map_Clear(const TMap<int32, int32>& TargetMap);
    DECLARE_FUNCTION(execMap_Clear);
};

}
