#pragma once

#include "Engine/Source/Runtime/AnimGraphRuntime/AnimNodes/AnimNode_BlendListBase.h"

#include "AnimNode_BlendListByBool.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FAnimNode_BlendListByBool : public FAnimNode_BlendListBase {
    GENERATED_BODY()
public:
    bool GetActiveValue() const { return GET_ANIM_NODE_DATA(bool, bActiveValue); }

protected:
    int32 GetActiveChildIndex() override { return GetActiveValue() ? 0 : 1; }
};

}
