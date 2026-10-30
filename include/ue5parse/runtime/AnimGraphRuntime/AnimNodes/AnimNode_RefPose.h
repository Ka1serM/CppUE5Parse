#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimNodeBase.h"

#include "AnimNode_RefPose.generated.h"

namespace ue5parse::runtime {

enum ERefPoseType : int32 { EIT_LocalSpace, EIT_Additive };

// The reference pose, or the additive identity (UE's AnimNodes/AnimNode_RefPose.h).
USTRUCT()
struct FAnimNode_RefPose : public FAnimNode_Base {
    GENERATED_BODY()
public:
    void Evaluate_AnyThread(FPoseContext& Output) override;
    ERefPoseType GetRefPoseType() const { return static_cast<ERefPoseType>(GET_ANIM_NODE_DATA(uint8, RefPoseType)); }
};

}
