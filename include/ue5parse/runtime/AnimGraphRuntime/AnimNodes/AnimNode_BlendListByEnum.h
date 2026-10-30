#pragma once

#include "ue5parse/runtime/AnimGraphRuntime/AnimNodes/AnimNode_BlendListBase.h"

#include "AnimNode_BlendListByEnum.generated.h"

namespace ue5parse::runtime {

// The child that an enum value maps to, else the first (UE's AnimNodes/AnimNode_BlendListByEnum.h).
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
