#pragma once

#include "ue5parse/runtime/AnimGraphRuntime/BoneControllers/AnimNode_SkeletalControlBase.h"

#include "AnimNode_RigidBody.generated.h"

namespace ue5parse::runtime {

// The rigid body simulation (UE's BoneControllers/AnimNode_RigidBody.h) is not run: the node passes its input pose
// through.
USTRUCT()
struct FAnimNode_RigidBody : public FAnimNode_SkeletalControlBase {
    GENERATED_BODY()
};

}
