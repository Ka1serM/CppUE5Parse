#pragma once

#include "Engine/Source/Runtime/AnimGraphRuntime/BoneControllers/AnimNode_SkeletalControlBase.h"

#include "AnimNode_RigidBody.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FAnimNode_RigidBody : public FAnimNode_SkeletalControlBase {
    GENERATED_BODY()
};

}
