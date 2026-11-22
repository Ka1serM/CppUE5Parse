#include "GameTypes/Fortnite/FortHomebaseNodeGameplayEffectDataTable.h"

#include <atomic>

#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"

namespace ue5parse::runtime {

UFortHomebaseNodeGameplayEffectDataTable::UFortHomebaseNodeGameplayEffectDataTable(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    static std::atomic<UScriptStruct*> Cache;
    RowStruct = FindNativeStruct(Cache, "HomebaseNodeGameplayEffectDataTableRow");
}

}
