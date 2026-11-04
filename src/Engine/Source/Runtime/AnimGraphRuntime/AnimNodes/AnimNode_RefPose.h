#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimNodeBase.h"

#include "AnimNode_RefPose.generated.h"

namespace ue5parse::runtime {

enum ERefPoseType : int32 { EIT_LocalSpace, EIT_Additive };

USTRUCT()
struct FAnimNode_RefPose : public FAnimNode_Base {
    GENERATED_BODY()
public:
    void Evaluate_AnyThread(FPoseContext& Output) override;
    ERefPoseType GetRefPoseType() const { return static_cast<ERefPoseType>(GET_ANIM_NODE_DATA(uint8, RefPoseType)); }
};

}
