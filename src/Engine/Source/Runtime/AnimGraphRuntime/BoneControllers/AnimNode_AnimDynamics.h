#pragma once

#include "Engine/Source/Runtime/AnimGraphRuntime/BoneControllers/AnimNode_SkeletalControlBase.h"
#include "Engine/Source/Runtime/Engine/Animation/BoneReference.h"

#include "AnimNode_AnimDynamics.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FAnimPhysPlanarLimit {
    GENERATED_BODY()
public:
    UPROPERTY()
    FBoneReference DrivingBone;
    UPROPERTY()
    FTransform PlaneTransform;
};

USTRUCT()
struct FAnimNode_AnimDynamics : public FAnimNode_SkeletalControlBase {
    GENERATED_BODY()
};

}
