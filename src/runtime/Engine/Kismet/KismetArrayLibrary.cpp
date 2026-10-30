#include "ue5parse/runtime/Engine/Kismet/KismetArrayLibrary.h"

#include <string>

#include "ue5parse/runtime/CoreUObject/UObject/PropertyAccess.h"
#include "ue5parse/runtime/CoreUObject/UObject/ScriptMacros.h"
#include "ue5parse/runtime/CoreUObject/UObject/UnrealType.h"

// The wildcard array thunks of UE's KismetArrayLibrary: the array parameter is read as an expression, which leaves its
// property and address on the frame; an item parameter is read into a value of the array's inner type, or (an out
// parameter) by address.

namespace ue5parse::runtime {

namespace {

// The array of the next parameter; a missing array fails the context, as UE does.
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

// The address of the caller's variable of an out parameter.
void* StepOutAddress(FFrame& Stack) {
    Stack.MostRecentProperty = nullptr;
    Stack.MostRecentPropertyAddress = nullptr;
    Stack.StepCompiledIn(nullptr);
    return Stack.MostRecentPropertyAddress;
}

void ReportArrayError(FFrame& Stack, const std::string& Message) {
    Stack.KismetExecutionMessage("KismetArray", Message, EBlueprintExceptionType::AccessViolation);
}

std::string IndexMessage(const char* Action, int32 Index, const FArrayProperty* Property, int32 Length) {
    return std::string(Action) + " index " + std::to_string(Index) + " of array " + Property->GetName().ToUtf8() + " of length " +
           std::to_string(Length);
}

int32 FindItem(FScriptArrayHelper& ArrayHelper, const FProperty* InnerProp, const void* Item) {
    for (int32 Index = 0; Index < ArrayHelper.Num(); Index++)
        if (InnerProp->Identical(Item, ArrayHelper.GetRawPtr(Index))) return Index;
    return INDEX_NONE;
}

int32 AddItem(FScriptArrayHelper& ArrayHelper, const FProperty* InnerProp, const void* Item) {
    const int32 NewIndex = ArrayHelper.AddValue();
    InnerProp->CopySingleValueToScriptVM(ArrayHelper.GetRawPtr(NewIndex), Item);
    return NewIndex;
}

}

DEFINE_FUNCTION(UKismetArrayLibrary::execArray_Add) {
    FWildcardArray Array;
    if (!StepArray(Stack, Array)) return;
    FDefaultConstructedPropertyElement NewItem(Array.Property->Inner);
    Stack.StepCompiledIn(NewItem.GetObjAddress());
    P_FINISH;
    FScriptArrayHelper ArrayHelper(Array.Property, Array.Address);
    *static_cast<int32*>(RESULT_PARAM) = AddItem(ArrayHelper, Array.Property->Inner, NewItem.GetObjAddress());
}

DEFINE_FUNCTION(UKismetArrayLibrary::execArray_AddUnique) {
    FWildcardArray Array;
    if (!StepArray(Stack, Array)) return;
    FDefaultConstructedPropertyElement NewItem(Array.Property->Inner);
    Stack.StepCompiledIn(NewItem.GetObjAddress());
    P_FINISH;
    FScriptArrayHelper ArrayHelper(Array.Property, Array.Address);
    int32 NewIndex = INDEX_NONE;
    if (FindItem(ArrayHelper, Array.Property->Inner, NewItem.GetObjAddress()) == INDEX_NONE)
        NewIndex = AddItem(ArrayHelper, Array.Property->Inner, NewItem.GetObjAddress());
    *static_cast<int32*>(RESULT_PARAM) = NewIndex;
}

// Each index trades with a random index at or after it (UE's Fisher-Yates shuffle).
DEFINE_FUNCTION(UKismetArrayLibrary::execArray_Shuffle) {
    FWildcardArray Array;
    if (!StepArray(Stack, Array)) return;
    P_FINISH;
    FScriptArrayHelper ArrayHelper(Array.Property, Array.Address);
    const int32 LastIndex = ArrayHelper.Num() - 1;
    for (int32 Index = 0; Index <= LastIndex; ++Index) {
        const int32 SwapIndex = FMath::RandRange(Index, LastIndex);
        if (Index != SwapIndex) ArrayHelper.SwapValues(Index, SwapIndex);
    }
}

DEFINE_FUNCTION(UKismetArrayLibrary::execArray_Identical) {
    FWildcardArray ArrayA;
    if (!StepArray(Stack, ArrayA)) return;
    FWildcardArray ArrayB;
    if (!StepArray(Stack, ArrayB)) return;
    P_FINISH;
    *static_cast<bool*>(RESULT_PARAM) = ArrayA.Property->Identical(ArrayA.Address, ArrayB.Address);
}

DEFINE_FUNCTION(UKismetArrayLibrary::execArray_Append) {
    FWildcardArray Target;
    if (!StepArray(Stack, Target)) return;
    FWildcardArray Source;
    if (!StepArray(Stack, Source)) return;
    P_FINISH;
    FScriptArrayHelper TargetHelper(Target.Property, Target.Address);
    FScriptArrayHelper SourceHelper(Source.Property, Source.Address);
    const int32 SourceNum = SourceHelper.Num();
    if (SourceNum == 0) return;
    const int32 StartIndex = TargetHelper.AddValues(SourceNum);
    for (int32 Index = 0; Index < SourceNum; Index++)
        Target.Property->Inner->CopySingleValueToScriptVM(TargetHelper.GetRawPtr(StartIndex + Index), SourceHelper.GetRawPtr(Index));
}

DEFINE_FUNCTION(UKismetArrayLibrary::execArray_Insert) {
    FWildcardArray Array;
    if (!StepArray(Stack, Array)) return;
    FDefaultConstructedPropertyElement NewItem(Array.Property->Inner);
    Stack.StepCompiledIn(NewItem.GetObjAddress());
    P_GET_PROPERTY(FIntProperty, Index);
    P_FINISH;
    FScriptArrayHelper ArrayHelper(Array.Property, Array.Address);
    if (Index < 0 || Index > ArrayHelper.Num()) {
        ReportArrayError(Stack, IndexMessage("Attempted to insert an item at", Index, Array.Property, ArrayHelper.Num()));
        return;
    }
    ArrayHelper.InsertValues(Index, 1);
    Array.Property->Inner->CopySingleValueToScriptVM(ArrayHelper.GetRawPtr(Index), NewItem.GetObjAddress());
}

DEFINE_FUNCTION(UKismetArrayLibrary::execArray_Remove) {
    FWildcardArray Array;
    if (!StepArray(Stack, Array)) return;
    P_GET_PROPERTY(FIntProperty, IndexToRemove);
    P_FINISH;
    FScriptArrayHelper ArrayHelper(Array.Property, Array.Address);
    if (!ArrayHelper.IsValidIndex(IndexToRemove)) {
        ReportArrayError(Stack, IndexMessage("Attempted to remove an item from invalid", IndexToRemove, Array.Property, ArrayHelper.Num()));
        return;
    }
    ArrayHelper.RemoveValues(IndexToRemove, 1);
}

// Removes every occurrence.
DEFINE_FUNCTION(UKismetArrayLibrary::execArray_RemoveItem) {
    FWildcardArray Array;
    if (!StepArray(Stack, Array)) return;
    FDefaultConstructedPropertyElement Item(Array.Property->Inner);
    Stack.StepCompiledIn(Item.GetObjAddress());
    P_FINISH;
    FScriptArrayHelper ArrayHelper(Array.Property, Array.Address);
    bool bRemoved = false;
    for (int32 Index = ArrayHelper.Num() - 1; Index >= 0; Index--) {
        if (!Array.Property->Inner->Identical(Item.GetObjAddress(), ArrayHelper.GetRawPtr(Index))) continue;
        ArrayHelper.RemoveValues(Index, 1);
        bRemoved = true;
    }
    *static_cast<bool*>(RESULT_PARAM) = bRemoved;
}

DEFINE_FUNCTION(UKismetArrayLibrary::execArray_Clear) {
    FWildcardArray Array;
    if (!StepArray(Stack, Array)) return;
    P_FINISH;
    FScriptArrayHelper(Array.Property, Array.Address).EmptyValues();
}

DEFINE_FUNCTION(UKismetArrayLibrary::execArray_Resize) {
    FWildcardArray Array;
    if (!StepArray(Stack, Array)) return;
    P_GET_PROPERTY(FIntProperty, Size);
    P_FINISH;
    if (Size < 0) {
        ReportArrayError(Stack, "Attempted to resize an array to a negative size");
        return;
    }
    FScriptArrayHelper(Array.Property, Array.Address).Resize(Size);
}

DEFINE_FUNCTION(UKismetArrayLibrary::execArray_Reverse) {
    FWildcardArray Array;
    if (!StepArray(Stack, Array)) return;
    P_FINISH;
    FScriptArrayHelper ArrayHelper(Array.Property, Array.Address);
    for (int32 Front = 0, Back = ArrayHelper.Num() - 1; Front < Back; Front++, Back--) ArrayHelper.SwapValues(Front, Back);
}

DEFINE_FUNCTION(UKismetArrayLibrary::execArray_Length) {
    FWildcardArray Array;
    if (!StepArray(Stack, Array)) return;
    P_FINISH;
    *static_cast<int32*>(RESULT_PARAM) = FScriptArrayHelper(Array.Property, Array.Address).Num();
}

DEFINE_FUNCTION(UKismetArrayLibrary::execArray_IsEmpty) {
    FWildcardArray Array;
    if (!StepArray(Stack, Array)) return;
    P_FINISH;
    *static_cast<bool*>(RESULT_PARAM) = FScriptArrayHelper(Array.Property, Array.Address).Num() == 0;
}

DEFINE_FUNCTION(UKismetArrayLibrary::execArray_IsNotEmpty) {
    FWildcardArray Array;
    if (!StepArray(Stack, Array)) return;
    P_FINISH;
    *static_cast<bool*>(RESULT_PARAM) = FScriptArrayHelper(Array.Property, Array.Address).Num() > 0;
}

DEFINE_FUNCTION(UKismetArrayLibrary::execArray_LastIndex) {
    FWildcardArray Array;
    if (!StepArray(Stack, Array)) return;
    P_FINISH;
    *static_cast<int32*>(RESULT_PARAM) = FScriptArrayHelper(Array.Property, Array.Address).Num() - 1;
}

// An invalid index reports and gives the item type's default value.
DEFINE_FUNCTION(UKismetArrayLibrary::execArray_Get) {
    FWildcardArray Array;
    if (!StepArray(Stack, Array)) return;
    P_GET_PROPERTY(FIntProperty, Index);
    void* Item = StepOutAddress(Stack);
    P_FINISH;
    if (Item == nullptr) return;
    const FProperty* InnerProp = Array.Property->Inner;
    FScriptArrayHelper ArrayHelper(Array.Property, Array.Address);
    if (ArrayHelper.IsValidIndex(Index)) {
        InnerProp->CopySingleValueToScriptVM(Item, ArrayHelper.GetRawPtr(Index));
        return;
    }
    ReportArrayError(Stack, IndexMessage("Attempted to access", Index, Array.Property, ArrayHelper.Num()));
    InnerProp->ClearValue(Item);
}

// bSizeToFit grows the array to reach the index.
DEFINE_FUNCTION(UKismetArrayLibrary::execArray_Set) {
    FWildcardArray Array;
    if (!StepArray(Stack, Array)) return;
    P_GET_PROPERTY(FIntProperty, Index);
    FDefaultConstructedPropertyElement Item(Array.Property->Inner);
    Stack.StepCompiledIn(Item.GetObjAddress());
    P_GET_UBOOL(bSizeToFit);
    P_FINISH;
    FScriptArrayHelper ArrayHelper(Array.Property, Array.Address);
    if (!ArrayHelper.IsValidIndex(Index) && bSizeToFit && Index >= 0) ArrayHelper.Resize(Index + 1);
    if (!ArrayHelper.IsValidIndex(Index)) {
        ReportArrayError(Stack, IndexMessage("Attempted to set an invalid", Index, Array.Property, ArrayHelper.Num()));
        return;
    }
    Array.Property->Inner->CopySingleValueToScriptVM(ArrayHelper.GetRawPtr(Index), Item.GetObjAddress());
}

DEFINE_FUNCTION(UKismetArrayLibrary::execArray_Swap) {
    FWildcardArray Array;
    if (!StepArray(Stack, Array)) return;
    P_GET_PROPERTY(FIntProperty, FirstIndex);
    P_GET_PROPERTY(FIntProperty, SecondIndex);
    P_FINISH;
    FScriptArrayHelper ArrayHelper(Array.Property, Array.Address);
    if (FirstIndex == SecondIndex) return;
    if (!ArrayHelper.IsValidIndex(FirstIndex) || !ArrayHelper.IsValidIndex(SecondIndex)) {
        ReportArrayError(Stack, IndexMessage("Attempted to swap an invalid", ArrayHelper.IsValidIndex(FirstIndex) ? SecondIndex : FirstIndex,
                                             Array.Property, ArrayHelper.Num()));
        return;
    }
    ArrayHelper.SwapValues(FirstIndex, SecondIndex);
}

DEFINE_FUNCTION(UKismetArrayLibrary::execArray_Find) {
    FWildcardArray Array;
    if (!StepArray(Stack, Array)) return;
    FDefaultConstructedPropertyElement ItemToFind(Array.Property->Inner);
    Stack.StepCompiledIn(ItemToFind.GetObjAddress());
    P_FINISH;
    FScriptArrayHelper ArrayHelper(Array.Property, Array.Address);
    *static_cast<int32*>(RESULT_PARAM) = FindItem(ArrayHelper, Array.Property->Inner, ItemToFind.GetObjAddress());
}

DEFINE_FUNCTION(UKismetArrayLibrary::execArray_Contains) {
    FWildcardArray Array;
    if (!StepArray(Stack, Array)) return;
    FDefaultConstructedPropertyElement ItemToFind(Array.Property->Inner);
    Stack.StepCompiledIn(ItemToFind.GetObjAddress());
    P_FINISH;
    FScriptArrayHelper ArrayHelper(Array.Property, Array.Address);
    *static_cast<bool*>(RESULT_PARAM) = FindItem(ArrayHelper, Array.Property->Inner, ItemToFind.GetObjAddress()) != INDEX_NONE;
}

// Copies the array into the object's array property of the name.
DEFINE_FUNCTION(UKismetArrayLibrary::execSetArrayPropertyByName) {
    P_GET_OBJECT(UObject, OwnerObject);
    P_GET_PROPERTY(FNameProperty, ArrayPropertyName);
    FWildcardArray Source;
    if (!StepArray(Stack, Source)) return;
    P_FINISH;
    if (OwnerObject == nullptr) return;
    FArrayProperty* Property = FindFProperty<FArrayProperty>(OwnerObject->GetClass(), ArrayPropertyName);
    if (Property != nullptr) Property->CopyValuesInternal(Property->ContainerPtrToValuePtr<void>(OwnerObject), Source.Address, 1);
}

DEFINE_FUNCTION(UKismetArrayLibrary::execArray_IsValidIndex) {
    FWildcardArray Array;
    if (!StepArray(Stack, Array)) return;
    P_GET_PROPERTY(FIntProperty, IndexToTest);
    P_FINISH;
    *static_cast<bool*>(RESULT_PARAM) = FScriptArrayHelper(Array.Property, Array.Address).IsValidIndex(IndexToTest);
}

// An empty array gives index INDEX_NONE and the item type's default value.
DEFINE_FUNCTION(UKismetArrayLibrary::execArray_Random) {
    FWildcardArray Array;
    if (!StepArray(Stack, Array)) return;
    void* OutItem = StepOutAddress(Stack);
    P_GET_PROPERTY_REF(FIntProperty, OutIndex);
    P_FINISH;
    if (OutItem == nullptr) return;
    const FProperty* InnerProp = Array.Property->Inner;
    FScriptArrayHelper ArrayHelper(Array.Property, Array.Address);
    if (ArrayHelper.Num() == 0) {
        OutIndex = INDEX_NONE;
        InnerProp->ClearValue(OutItem);
        return;
    }
    OutIndex = FMath::RandRange(0, ArrayHelper.Num() - 1);
    InnerProp->CopySingleValueToScriptVM(OutItem, ArrayHelper.GetRawPtr(OutIndex));
}

}
