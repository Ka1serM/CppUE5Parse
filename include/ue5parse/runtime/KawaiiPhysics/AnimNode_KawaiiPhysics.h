#pragma once

#include "ue5parse/runtime/AnimGraphRuntime/BoneControllers/AnimNode_SkeletalControlBase.h"

#include "AnimNode_KawaiiPhysics.generated.h"

namespace ue5parse::runtime {

// The KawaiiPhysics plugin's node. Its simulation is not run: the node passes its input pose through.
USTRUCT()
struct FAnimNode_KawaiiPhysics : public FAnimNode_SkeletalControlBase {
    GENERATED_BODY()
};

}
