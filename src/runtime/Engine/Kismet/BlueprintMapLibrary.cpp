#include "ue5parse/runtime/Engine/Kismet/BlueprintMapLibrary.h"

#include "ue5parse/runtime/CoreUObject/UObject/ScriptMacros.h"
#include "ue5parse/runtime/CoreUObject/UObject/UnrealType.h"

// The wildcard map thunks of UE's BlueprintMapLibrary: the map parameter is read as an expression, which leaves its
// property and address on the frame; a key or value parameter is read into a value of the map's key or value type, or
// (an out parameter) by address.

namespace ue5parse::runtime {

namespace {

// The map of the next parameter; a missing map fails the context, as UE does.
struct FWildcardMap {
    void* Address = nullptr;
    FMapProperty* Property = nullptr;
};

bool StepMap(FFrame& Stack, FWildcardMap& Out) {
    Stack.MostRecentProperty = nullptr;
    Stack.MostRecentPropertyAddress = nullptr;
    Stack.StepCompiledIn(nullptr);
    Out.Address = Stack.MostRecentPropertyAddress;
    Out.Property = CastField<FMapProperty>(Stack.MostRecentProperty);
    if (Out.Property != nullptr && Out.Address != nullptr) return true;
    Stack.bArrayContextFailed = true;
    return false;
}

// The array of the next (out) parameter.
bool StepArray(FFrame& Stack, void*& Address, FArrayProperty*& Property) {
    Stack.MostRecentProperty = nullptr;
    Stack.MostRecentPropertyAddress = nullptr;
    Stack.StepCompiledIn(nullptr);
    Address = Stack.MostRecentPropertyAddress;
    Property = CastField<FArrayProperty>(Stack.MostRecentProperty);
    if (Property != nullptr && Address != nullptr) return true;
    Stack.bArrayContextFailed = true;
    return false;
}

// The address of the caller's variable of an out parameter.
void* StepOutAddress(FFrame& Stack) {
    Stack.MostRecentProperty = nullptr;
    Stack.MostRecentPropertyAddress = nullptr;
    Stack.StepCompiledIn(nullptr);
    return Stack.MostRecentPropertyAddress;
}

// Replaces the array with copies of the keys (or values) of the map, in the map's order.
void CopyPairPartsToArray(FScriptMapHelper& MapHelper, bool bValues, FArrayProperty* ArrayProperty, void* ArrayAddress) {
    FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayAddress);
    ArrayHelper.EmptyValues();
    for (int32 Index = 0, Remaining = MapHelper.Num(); Remaining > 0; Index++) {
        if (!MapHelper.IsValidIndex(Index)) continue;
        const int32 NewIndex = ArrayHelper.AddValue();
        ArrayProperty->Inner->CopySingleValueToScriptVM(ArrayHelper.GetRawPtr(NewIndex),
                                                        bValues ? MapHelper.GetValuePtr(Index) : MapHelper.GetKeyPtr(Index));
        Remaining--;
    }
}

}

// Sets the value of the key, adding the pair when the key is new.
DEFINE_FUNCTION(UBlueprintMapLibrary::execMap_Add) {
    FWildcardMap Map;
    if (!StepMap(Stack, Map)) return;
    FDefaultConstructedPropertyElement Key(Map.Property->KeyProp);
    Stack.StepCompiledIn(Key.GetObjAddress());
    FDefaultConstructedPropertyElement Value(Map.Property->ValueProp);
    Stack.StepCompiledIn(Value.GetObjAddress());
    P_FINISH;
    FScriptMapHelper(Map.Property, Map.Address).AddPair(Key.GetObjAddress(), Value.GetObjAddress());
}

DEFINE_FUNCTION(UBlueprintMapLibrary::execMap_Remove) {
    FWildcardMap Map;
    if (!StepMap(Stack, Map)) return;
    FDefaultConstructedPropertyElement Key(Map.Property->KeyProp);
    Stack.StepCompiledIn(Key.GetObjAddress());
    P_FINISH;
    *static_cast<bool*>(RESULT_PARAM) = FScriptMapHelper(Map.Property, Map.Address).RemovePair(Key.GetObjAddress());
}

// A missing key gives the value type's default value.
DEFINE_FUNCTION(UBlueprintMapLibrary::execMap_Find) {
    FWildcardMap Map;
    if (!StepMap(Stack, Map)) return;
    FDefaultConstructedPropertyElement Key(Map.Property->KeyProp);
    Stack.StepCompiledIn(Key.GetObjAddress());
    void* OutValue = StepOutAddress(Stack);
    P_FINISH;
    FScriptMapHelper MapHelper(Map.Property, Map.Address);
    const uint8* FoundValue = MapHelper.FindValueFromHash(Key.GetObjAddress());
    if (OutValue != nullptr) {
        if (FoundValue != nullptr) Map.Property->ValueProp->CopySingleValueToScriptVM(OutValue, FoundValue);
        else Map.Property->ValueProp->ClearValue(OutValue);
    }
    *static_cast<bool*>(RESULT_PARAM) = FoundValue != nullptr;
}

DEFINE_FUNCTION(UBlueprintMapLibrary::execMap_Contains) {
    FWildcardMap Map;
    if (!StepMap(Stack, Map)) return;
    FDefaultConstructedPropertyElement Key(Map.Property->KeyProp);
    Stack.StepCompiledIn(Key.GetObjAddress());
    P_FINISH;
    *static_cast<bool*>(RESULT_PARAM) = FScriptMapHelper(Map.Property, Map.Address).FindValueFromHash(Key.GetObjAddress()) != nullptr;
}

DEFINE_FUNCTION(UBlueprintMapLibrary::execMap_Keys) {
    FWildcardMap Map;
    if (!StepMap(Stack, Map)) return;
    void* ArrayAddress = nullptr;
    FArrayProperty* ArrayProperty = nullptr;
    if (!StepArray(Stack, ArrayAddress, ArrayProperty)) return;
    P_FINISH;
    FScriptMapHelper MapHelper(Map.Property, Map.Address);
    CopyPairPartsToArray(MapHelper, false, ArrayProperty, ArrayAddress);
}

DEFINE_FUNCTION(UBlueprintMapLibrary::execMap_Values) {
    FWildcardMap Map;
    if (!StepMap(Stack, Map)) return;
    void* ArrayAddress = nullptr;
    FArrayProperty* ArrayProperty = nullptr;
    if (!StepArray(Stack, ArrayAddress, ArrayProperty)) return;
    P_FINISH;
    FScriptMapHelper MapHelper(Map.Property, Map.Address);
    CopyPairPartsToArray(MapHelper, true, ArrayProperty, ArrayAddress);
}

DEFINE_FUNCTION(UBlueprintMapLibrary::execMap_Length) {
    FWildcardMap Map;
    if (!StepMap(Stack, Map)) return;
    P_FINISH;
    *static_cast<int32*>(RESULT_PARAM) = FScriptMapHelper(Map.Property, Map.Address).Num();
}

DEFINE_FUNCTION(UBlueprintMapLibrary::execMap_IsEmpty) {
    FWildcardMap Map;
    if (!StepMap(Stack, Map)) return;
    P_FINISH;
    *static_cast<bool*>(RESULT_PARAM) = FScriptMapHelper(Map.Property, Map.Address).Num() == 0;
}

DEFINE_FUNCTION(UBlueprintMapLibrary::execMap_IsNotEmpty) {
    FWildcardMap Map;
    if (!StepMap(Stack, Map)) return;
    P_FINISH;
    *static_cast<bool*>(RESULT_PARAM) = FScriptMapHelper(Map.Property, Map.Address).Num() > 0;
}

DEFINE_FUNCTION(UBlueprintMapLibrary::execMap_Clear) {
    FWildcardMap Map;
    if (!StepMap(Stack, Map)) return;
    P_FINISH;
    FScriptMapHelper(Map.Property, Map.Address).EmptyValues();
}

}
