#pragma once

#include "Engine/Source/Runtime/AnimGraphRuntime/AnimNodes/AnimNode_BlendListBase.h"

#include "AnimNode_BlendListByInt.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FAnimNode_BlendListByInt : public FAnimNode_BlendListBase {
    GENERATED_BODY()
protected:
    int32 GetActiveChildIndex() override { return FMath::Clamp(GET_ANIM_NODE_DATA(int32, ActiveChildIndex), 0, BlendPose.Num() - 1); }
};

}
