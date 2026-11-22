#include "Engine/Plugins/Runtime/CommonUI/Source/CommonUI/Input/CommonGenericInputActionDataTable.h"

#include <atomic>

#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"

namespace ue5parse::runtime {

UCommonGenericInputActionDataTable::UCommonGenericInputActionDataTable(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    static std::atomic<UScriptStruct*> Cache;
    RowStruct = FindNativeStruct(Cache, "CommonInputActionDataBase");
}

}
