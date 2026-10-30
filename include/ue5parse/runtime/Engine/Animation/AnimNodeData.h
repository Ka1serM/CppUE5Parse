#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/ScriptDelegates.h"
#include "ue5parse/runtime/CoreUObject/UObject/Object.h"

#include "AnimNodeData.generated.h"

namespace ue5parse::runtime {

class UAnimBlueprintGeneratedClass;
struct FAnimNode_Base;

namespace UE::Anim {
struct FNodeDataId;
}

// The cooked table of one node (UE's Animation/AnimNodeData.h): for each property of the node's type, where its folded
// value is: a constant of the class, a value of the instance's mutable data, or none (the node holds it).
USTRUCT()
struct FAnimNodeData {
    GENERATED_BODY()
public:
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

    // The class that compiled the node: it holds the constants.
    const UAnimBlueprintGeneratedClass& GetAnimClass() const;
    int32 GetNodeIndex() const { return NodeIndex; }
    // The folded value of a property of the node.
    const void* GetData(UE::Anim::FNodeDataId InId, const FAnimNode_Base* InNode) const;
    // The folded value when the instance holds it, else null.
    const void* GetInstanceData(UE::Anim::FNodeDataId InId, const FAnimNode_Base* InNode) const;
};

}
