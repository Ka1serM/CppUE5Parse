#pragma once

#include "ue5parse/runtime/AnimGraphRuntime/AnimNodes/AnimNode_BlendListBase.h"

#include "AnimNode_BlendListByBool.generated.h"

namespace ue5parse::runtime {

// The first child when the value is true, the second when it is false (UE's AnimNodes/AnimNode_BlendListByBool.h).
USTRUCT()
struct FAnimNode_BlendListByBool : public FAnimNode_BlendListBase {
    GENERATED_BODY()
public:
    bool GetActiveValue() const { return GET_ANIM_NODE_DATA(bool, bActiveValue); }

protected:
    int32 GetActiveChildIndex() override { return GetActiveValue() ? 0 : 1; }
};

}
