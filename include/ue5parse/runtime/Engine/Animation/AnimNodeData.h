#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/ScriptDelegates.h"
#include "ue5parse/runtime/CoreUObject/UObject/Object.h"

#include "AnimNodeData.generated.h"

namespace ue5parse::runtime {

// The cooked table that maps an animation node's fields to class constants or instance properties.
USTRUCT()
struct FAnimNodeData {
    GENERATED_BODY()
    UPROPERTY()
    FScriptInterface AnimClassInterface;
    UPROPERTY()
    TArray<uint32> Entries;
    UPROPERTY()
    int32 NodeIndex = INDEX_NONE;
    UPROPERTY()
    uint32 Flags = 0;

    static constexpr uint32 InvalidEntry = 0xffffffffu;
    static constexpr uint32 InstanceDataFlag = 0x80000000u;
    bool IsInstanceData(int32 Entry) const { return Entries.IsValidIndex(Entry) && Entries[Entry] != InvalidEntry &&
        (Entries[Entry] & InstanceDataFlag) != 0; }
    int32 GetDataIndex(int32 Entry) const { return Entries.IsValidIndex(Entry) && Entries[Entry] != InvalidEntry ? int32(Entries[Entry] & ~InstanceDataFlag) : INDEX_NONE; }
};

}
