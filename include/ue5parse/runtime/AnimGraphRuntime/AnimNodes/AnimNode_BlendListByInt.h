#pragma once

#include "ue5parse/runtime/AnimGraphRuntime/AnimNodes/AnimNode_BlendListBase.h"

#include "AnimNode_BlendListByInt.generated.h"

namespace ue5parse::runtime {

// The child of an index, clamped to the children (UE's AnimNodes/AnimNode_BlendListByInt.h).
USTRUCT()
struct FAnimNode_BlendListByInt : public FAnimNode_BlendListBase {
    GENERATED_BODY()
protected:
    int32 GetActiveChildIndex() override { return FMath::Clamp(GET_ANIM_NODE_DATA(int32, ActiveChildIndex), 0, BlendPose.Num() - 1); }
};

}
