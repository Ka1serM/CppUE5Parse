#pragma once

#include "Engine/Source/Runtime/AnimGraphRuntime/AnimNodes/AnimNode_BlendListBase.h"

#include "AnimNode_BlendListByEnum.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FAnimNode_BlendListByEnum : public FAnimNode_BlendListBase {
    GENERATED_BODY()
protected:
    int32 GetActiveChildIndex() override {
        const uint8 ActiveEnumValue = GET_ANIM_NODE_DATA(uint8, ActiveEnumValue);
        const TArray<int32>& EnumToPoseIndex = GET_ANIM_NODE_DATA(TArray<int32>, EnumToPoseIndex);
        return EnumToPoseIndex.IsValidIndex(ActiveEnumValue) ? EnumToPoseIndex[ActiveEnumValue] : 0;
    }
};

}
