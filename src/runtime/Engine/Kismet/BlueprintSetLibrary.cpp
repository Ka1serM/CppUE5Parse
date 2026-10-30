#include "ue5parse/runtime/Engine/Kismet/BlueprintSetLibrary.h"

#include "ue5parse/runtime/CoreUObject/UObject/ScriptMacros.h"
#include "ue5parse/runtime/CoreUObject/UObject/UnrealType.h"

// The wildcard set thunks of UE's BlueprintSetLibrary: the set parameter is read as an expression, which leaves its
// property and address on the frame; an item parameter is read into a value of the set's element type.

namespace ue5parse::runtime {

namespace {

// The set of the next parameter; a missing set fails the context, as UE does.
struct FWildcardSet {
    void* Address = nullptr;
    FSetProperty* Property = nullptr;
};

bool StepSet(FFrame& Stack, FWildcardSet& Out) {
    Stack.MostRecentProperty = nullptr;
    Stack.MostRecentPropertyAddress = nullptr;
    Stack.StepCompiledIn(nullptr);
    Out.Address = Stack.MostRecentPropertyAddress;
    Out.Property = CastField<FSetProperty>(Stack.MostRecentProperty);
    if (Out.Property != nullptr && Out.Address != nullptr) return true;
    Stack.bArrayContextFailed = true;
    return false;
}

struct FWildcardArray {
    void* Address = nullptr;
    FArrayProperty* Property = nullptr;
};

bool StepArray(FFrame& Stack, FWildcardArray& Out) {
    Stack.MostRecentProperty = nullptr;
    Stack.MostRecentPropertyAddress = nullptr;
    Stack.StepCompiledIn(nullptr);
    Out.Address = Stack.MostRecentPropertyAddress;
    Out.Property = CastField<FArrayProperty>(Stack.MostRecentProperty);
    if (Out.Property != nullptr && Out.Address != nullptr) return true;
    Stack.bArrayContextFailed = true;
    return false;
}

// Calls Visit with each element of the set, in the set's order.
template <typename F>
void ForEachElement(FScriptSetHelper& SetHelper, F&& Visit) {
    for (int32 Index = 0, Remaining = SetHelper.Num(); Remaining > 0; Index++) {
        if (!SetHelper.IsValidIndex(Index)) continue;
        Visit(SetHelper.GetElementPtr(Index));
        Remaining--;
    }
}

bool Contains(const FScriptSetHelper& SetHelper, const void* Item) { return SetHelper.FindElementIndex(Item) != INDEX_NONE; }

// The three set operations step A, B and the result the same way; the result is emptied first, as UE does.
template <typename F>
void SetOperation(FFrame& Stack, F&& Fill) {
    FWildcardSet A;
    if (!StepSet(Stack, A)) return;
    FWildcardSet B;
    if (!StepSet(Stack, B)) return;
    FWildcardSet Result;
    if (!StepSet(Stack, Result)) return;
    P_FINISH;
    FScriptSetHelper HelperA(A.Property, A.Address);
    FScriptSetHelper HelperB(B.Property, B.Address);
    FScriptSetHelper ResultHelper(Result.Property, Result.Address);
    ResultHelper.EmptyElements();
    Fill(HelperA, HelperB, ResultHelper);
}

}

DEFINE_FUNCTION(UBlueprintSetLibrary::execSet_Add) {
    FWildcardSet Set;
    if (!StepSet(Stack, Set)) return;
    FDefaultConstructedPropertyElement NewItem(Set.Property->ElementProp);
    Stack.StepCompiledIn(NewItem.GetObjAddress());
    P_FINISH;
    FScriptSetHelper(Set.Property, Set.Address).AddElement(NewItem.GetObjAddress());
}

DEFINE_FUNCTION(UBlueprintSetLibrary::execSet_AddItems) {
    FWildcardSet Set;
    if (!StepSet(Stack, Set)) return;
    FWildcardArray Items;
    if (!StepArray(Stack, Items)) return;
    P_FINISH;
    FScriptSetHelper SetHelper(Set.Property, Set.Address);
    FScriptArrayHelper ArrayHelper(Items.Property, Items.Address);
    for (int32 Index = 0; Index < ArrayHelper.Num(); Index++) SetHelper.AddElement(ArrayHelper.GetRawPtr(Index));
}

DEFINE_FUNCTION(UBlueprintSetLibrary::execSet_Remove) {
    FWildcardSet Set;
    if (!StepSet(Stack, Set)) return;
    FDefaultConstructedPropertyElement Item(Set.Property->ElementProp);
    Stack.StepCompiledIn(Item.GetObjAddress());
    P_FINISH;
    *static_cast<bool*>(RESULT_PARAM) = FScriptSetHelper(Set.Property, Set.Address).RemoveElement(Item.GetObjAddress());
}

DEFINE_FUNCTION(UBlueprintSetLibrary::execSet_IsNotEmpty) {
    FWildcardSet Set;
    if (!StepSet(Stack, Set)) return;
    P_FINISH;
    *static_cast<bool*>(RESULT_PARAM) = FScriptSetHelper(Set.Property, Set.Address).Num() > 0;
}

DEFINE_FUNCTION(UBlueprintSetLibrary::execSet_RemoveItems) {
    FWildcardSet Set;
    if (!StepSet(Stack, Set)) return;
    FWildcardArray Items;
    if (!StepArray(Stack, Items)) return;
    P_FINISH;
    FScriptSetHelper SetHelper(Set.Property, Set.Address);
    FScriptArrayHelper ArrayHelper(Items.Property, Items.Address);
    for (int32 Index = 0; Index < ArrayHelper.Num(); Index++) SetHelper.RemoveElement(ArrayHelper.GetRawPtr(Index));
}

// Appends the elements to the array, as UE does (the array is not emptied first).
DEFINE_FUNCTION(UBlueprintSetLibrary::execSet_ToArray) {
    FWildcardSet Set;
    if (!StepSet(Stack, Set)) return;
    FWildcardArray Result;
    if (!StepArray(Stack, Result)) return;
    P_FINISH;
    FScriptSetHelper SetHelper(Set.Property, Set.Address);
    FScriptArrayHelper ArrayHelper(Result.Property, Result.Address);
    ForEachElement(SetHelper, [&](const uint8* Element) {
        const int32 NewIndex = ArrayHelper.AddValue();
        Result.Property->Inner->CopySingleValueToScriptVM(ArrayHelper.GetRawPtr(NewIndex), Element);
    });
}

DEFINE_FUNCTION(UBlueprintSetLibrary::execSet_Clear) {
    FWildcardSet Set;
    if (!StepSet(Stack, Set)) return;
    P_FINISH;
    FScriptSetHelper(Set.Property, Set.Address).EmptyElements();
}

DEFINE_FUNCTION(UBlueprintSetLibrary::execSet_Length) {
    FWildcardSet Set;
    if (!StepSet(Stack, Set)) return;
    P_FINISH;
    *static_cast<int32*>(RESULT_PARAM) = FScriptSetHelper(Set.Property, Set.Address).Num();
}

DEFINE_FUNCTION(UBlueprintSetLibrary::execSet_Contains) {
    FWildcardSet Set;
    if (!StepSet(Stack, Set)) return;
    FDefaultConstructedPropertyElement ItemToFind(Set.Property->ElementProp);
    Stack.StepCompiledIn(ItemToFind.GetObjAddress());
    P_FINISH;
    *static_cast<bool*>(RESULT_PARAM) = Contains(FScriptSetHelper(Set.Property, Set.Address), ItemToFind.GetObjAddress());
}

DEFINE_FUNCTION(UBlueprintSetLibrary::execSet_Intersection) {
    SetOperation(Stack, [](FScriptSetHelper& A, FScriptSetHelper& B, FScriptSetHelper& Result) {
        ForEachElement(A, [&](const uint8* Element) {
            if (Contains(B, Element)) Result.AddElement(Element);
        });
    });
}

DEFINE_FUNCTION(UBlueprintSetLibrary::execSet_Union) {
    SetOperation(Stack, [](FScriptSetHelper& A, FScriptSetHelper& B, FScriptSetHelper& Result) {
        ForEachElement(A, [&](const uint8* Element) { Result.AddElement(Element); });
        ForEachElement(B, [&](const uint8* Element) { Result.AddElement(Element); });
    });
}

DEFINE_FUNCTION(UBlueprintSetLibrary::execSet_Difference) {
    SetOperation(Stack, [](FScriptSetHelper& A, FScriptSetHelper& B, FScriptSetHelper& Result) {
        ForEachElement(A, [&](const uint8* Element) {
            if (!Contains(B, Element)) Result.AddElement(Element);
        });
    });
}

}
