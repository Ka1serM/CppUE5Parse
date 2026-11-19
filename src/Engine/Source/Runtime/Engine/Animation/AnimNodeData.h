#pragma once

#include "Engine/Source/Runtime/Core/Containers/Map.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ScriptDelegates.h"

#include "AnimNodeData.generated.h"

namespace ue5parse::runtime {

class UAnimBlueprintGeneratedClass;
struct FAnimNode_Base;

namespace UE::Anim {

struct FNodeDataId;

}

UENUM()
enum class EAnimNodeDataFlags : uint32 {
    None = 0x00000000,
    HasInitialUpdateFunction = 0x00000001,
    HasBecomeRelevantFunction = 0x00000002,
    HasUpdateFunction = 0x00000004,
    AllFunctions = HasInitialUpdateFunction | HasBecomeRelevantFunction | HasUpdateFunction,
};

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

    const UAnimBlueprintGeneratedClass& GetAnimClass() const;
    int32 GetNodeIndex() const { return NodeIndex; }
    const void* GetData(UE::Anim::FNodeDataId InId, const FAnimNode_Base* InNode) const;
    const void* GetInstanceData(UE::Anim::FNodeDataId InId, const FAnimNode_Base* InNode) const;
};

USTRUCT()
struct FAnimNodeStructData {
    GENERATED_BODY()
    UPROPERTY()
    TMap<FName, int32> NameToIndexMap;
    UPROPERTY()
    int32 NumProperties = 0;
};

}
