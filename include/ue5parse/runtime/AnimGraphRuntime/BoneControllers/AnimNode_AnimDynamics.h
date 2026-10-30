#pragma once

#include "ue5parse/runtime/AnimGraphRuntime/BoneControllers/AnimNode_SkeletalControlBase.h"
#include "ue5parse/runtime/Engine/Animation/BoneReference.h"

#include "AnimNode_AnimDynamics.generated.h"

namespace ue5parse::runtime {

// A plane that a chain must stay in front of; relative to the driving bone when it has one (UE's
// BoneControllers/AnimNode_AnimDynamics.h).
USTRUCT()
struct FAnimPhysPlanarLimit {
    GENERATED_BODY()
public:
    UPROPERTY()
    FBoneReference DrivingBone;
    UPROPERTY()
    FTransform PlaneTransform;
};

// The physics simulation is not run: the node passes its input pose through.
USTRUCT()
struct FAnimNode_AnimDynamics : public FAnimNode_SkeletalControlBase {
    GENERATED_BODY()
};

}
